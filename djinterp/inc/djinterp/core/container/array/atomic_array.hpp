/******************************************************************************
* djinterp                                                      atomic_array.hpp
*
* Lock-free atomic-element array.
*   Stores `std::atomic<T>` elements with element-level lock-free access.
* No mutex, no lock policy, no version counter at the container level —
* synchronization is per-element, leveraging the atomic primitives in
* std::atomic.
*
*   Distinct from threadsafe_array (which protects whole-array access
* under a lock policy) and from cow_array / rcu_array (which present
* whole-array snapshots).  atomic_array exposes per-slot atomic
* operations: load, store, fetch_add, compare_exchange.
*
* TEMPLATE PARAMETERS:
*   _T            — element type (must be lock-free atomic-compatible:
*                   trivially copyable, integral / pointer / aligned POD)
*   _N            — extent (must be static — atomic elements cannot be
*                   relocated, so dynamic_extent is unsupported)
*   _Lifetime     — array_lifetime tag
*   _Iterability  — array_iterability tag
*
* WHEN TO USE:
*   - Counter arrays (per-thread counters, histograms).
*   - Bitmaps and per-slot flags.
*   - Lock-free producer/consumer index arrays.
*   - Anywhere readers and writers need to operate on disjoint
*     elements without serialization.
*
* WHEN NOT TO USE:
*   - When the consistency guarantee must span multiple elements
*     atomically (use threadsafe_array's batch_guard or cow_array).
*   - For non-trivially-copyable element types (use threadsafe_array).
*   - When element-level operations need rich invariants (use
*     threadsafe_array with a lock policy).
*
* SEMANTICS:
*   - Each element is independently atomic.  A read of element i and
*     a write of element j (j != i) do not synchronize with each other.
*   - The array's *size* is fixed at compile time.  No reallocation
*     means no inter-element pointer invalidation.
*   - Iteration is supported but provides only per-element atomic
*     guarantees, not a coherent snapshot.
*
* DEPENDENCIES:
*   array_traits.hpp                — lifetime / iterability tags
*   concurrency_strategy_traits.hpp — strategy tag types
*   <atomic>                        — std::atomic
*
* TABLE OF CONTENTS
* =================
* I.    atomic_array
*         a. Type aliases
*         b. Strategy tag and trait constants
*         c. Construction
*         d. Element Atomic Access
*         e. Element Atomic Updates
*         f. Element CAS
*         g. Bulk Operations
*         h. Iteration (per-element atomic)
* II.   Trait Specializations (axis preservation)
* III.  Static Verification
*
*
* path:      /inc/djinterp/container/array/atomic_array.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.26
******************************************************************************/

#ifndef DJINTERP_ATOMIC_ARRAY_
#define DJINTERP_ATOMIC_ARRAY_ 1

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include "../../djinterp.hpp"
#include "../traits/concurrency_strategy_traits.hpp"
#include "./array_traits.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include <atomic>


NS_DJINTERP

// =============================================================================
// I.   atomic_array
// =============================================================================

template<typename                _T,
         std::size_t             _N,
         array_lifetime _Lifetime    =
             array_lifetime::mutable_lifetime,
         array_iterability         _Iterability =
             array_iterability::iterable>
class atomic_array
{
    static_assert(
        std::is_trivially_copyable<_T>::value,
        "atomic_array element type must be trivially copyable "
        "(std::atomic<T> requirement)");

public:
    // -------------------------------------------------------------
    // a. Type aliases
    // -------------------------------------------------------------
    using value_type             = _T;
    using atomic_value_type      = std::atomic<_T>;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using reference              = atomic_value_type&;
    using const_reference        = const atomic_value_type&;
    using pointer                = atomic_value_type*;
    using const_pointer          = const atomic_value_type*;
    using iterator               = atomic_value_type*;
    using const_iterator         = const atomic_value_type*;

    // -------------------------------------------------------------
    // b. Strategy tag and trait constants
    // -------------------------------------------------------------

    // strategy tag — read by concurrency_strategy_traits
    using concurrency_strategy_tag = atomic_strategy_tag;

    // axis re-export
    D_STATIC_CONSTEXPR size_type extent = _N;
    D_STATIC_CONSTEXPR
        array_lifetime lifetime    = _Lifetime;
    D_STATIC_CONSTEXPR
        array_iterability         iterability = _Iterability;

    // -------------------------------------------------------------
    // c. Construction
    // -------------------------------------------------------------
    atomic_array() noexcept
    {
        for (size_type i = 0; i < _N; ++i)
        {
            m_data[i].store(_T{},
                std::memory_order_relaxed);
        }
    }

    explicit atomic_array(const _T& _fill) noexcept
    {
        for (size_type i = 0; i < _N; ++i)
        {
            m_data[i].store(_fill,
                std::memory_order_relaxed);
        }
    }

    // non-copyable: std::atomic is non-copyable.  To clone,
    // load each element and construct a new array element-
    // wise (caller's responsibility).
    atomic_array(const atomic_array&)            = delete;
    atomic_array& operator=(const atomic_array&) = delete;

    atomic_array(atomic_array&&)            = delete;
    atomic_array& operator=(atomic_array&&) = delete;

    ~atomic_array() = default;

    // -------------------------------------------------------------
    // d. Element Atomic Access
    // -------------------------------------------------------------

    // load
    //   atomically reads element _i.
    _T load(
        size_type         _i,
        std::memory_order _order =
            std::memory_order_seq_cst) const noexcept
    {
        return m_data[_i].load(_order);
    }

    // store
    //   atomically writes element _i.
    void store(
        size_type         _i,
        _T                _v,
        std::memory_order _order =
            std::memory_order_seq_cst) noexcept
    {
        m_data[_i].store(_v, _order);
    }

    // exchange
    //   atomically replaces element _i, returning the old
    // value.
    _T exchange(
        size_type         _i,
        _T                _v,
        std::memory_order _order =
            std::memory_order_seq_cst) noexcept
    {
        return m_data[_i].exchange(_v, _order);
    }

    // -------------------------------------------------------------
    // e. Element Atomic Updates
    // -------------------------------------------------------------
    // These delegate to std::atomic's fetch_* family.  They
    // SFINAE-disable themselves when _T is not an integral
    // or pointer type (per std::atomic's requirements).

    _T fetch_add(
        size_type         _i,
        _T                _n,
        std::memory_order _order =
            std::memory_order_seq_cst) noexcept
    {
        return m_data[_i].fetch_add(_n, _order);
    }

    _T fetch_sub(
        size_type         _i,
        _T                _n,
        std::memory_order _order =
            std::memory_order_seq_cst) noexcept
    {
        return m_data[_i].fetch_sub(_n, _order);
    }

    _T fetch_and(
        size_type         _i,
        _T                _n,
        std::memory_order _order =
            std::memory_order_seq_cst) noexcept
    {
        return m_data[_i].fetch_and(_n, _order);
    }

    _T fetch_or(
        size_type         _i,
        _T                _n,
        std::memory_order _order =
            std::memory_order_seq_cst) noexcept
    {
        return m_data[_i].fetch_or(_n, _order);
    }

    _T fetch_xor(
        size_type         _i,
        _T                _n,
        std::memory_order _order =
            std::memory_order_seq_cst) noexcept
    {
        return m_data[_i].fetch_xor(_n, _order);
    }

    // -------------------------------------------------------------
    // f. Element CAS
    // -------------------------------------------------------------

    bool compare_exchange_weak(
        size_type         _i,
        _T&               _expected,
        _T                _desired,
        std::memory_order _success =
            std::memory_order_seq_cst,
        std::memory_order _failure =
            std::memory_order_seq_cst) noexcept
    {
        return m_data[_i].compare_exchange_weak(
            _expected, _desired, _success, _failure);
    }

    bool compare_exchange_strong(
        size_type         _i,
        _T&               _expected,
        _T                _desired,
        std::memory_order _success =
            std::memory_order_seq_cst,
        std::memory_order _failure =
            std::memory_order_seq_cst) noexcept
    {
        return m_data[_i].compare_exchange_strong(
            _expected, _desired, _success, _failure);
    }

    // -------------------------------------------------------------
    // g. Bulk Operations
    // -------------------------------------------------------------

    constexpr size_type size() const noexcept
    {
        return _N;
    }

    constexpr bool empty() const noexcept
    {
        return ( _N == 0 );
    }

    // fill
    //   sets every element to _v.  Per-element atomic;
    // NOT a coherent whole-array operation.
    void fill(
        const _T&         _v,
        std::memory_order _order =
            std::memory_order_seq_cst) noexcept
    {
        for (size_type i = 0; i < _N; ++i)
        {
            m_data[i].store(_v, _order);
        }
    }

    // is_lock_free
    //   reports whether the underlying atomic is lock-free
    // for this element type and platform.
    bool is_lock_free() const noexcept
    {
        return ( _N > 0 ? m_data[0].is_lock_free() : true );
    }

    // -------------------------------------------------------------
    // h. Iteration (per-element atomic)
    // -------------------------------------------------------------
    // Iterators yield atomic_value_type& — callers operate
    // on each slot through its atomic interface.  There is
    // no whole-array consistency.

    iterator begin() noexcept
    {
        return &m_data[0];
    }

    iterator end() noexcept
    {
        return &m_data[0] + _N;
    }

    const_iterator begin() const noexcept
    {
        return &m_data[0];
    }

    const_iterator end() const noexcept
    {
        return &m_data[0] + _N;
    }

    const_iterator cbegin() const noexcept { return begin(); }
    const_iterator cend()   const noexcept { return end();   }

    pointer data() noexcept             { return &m_data[0]; }
    const_pointer data() const noexcept { return &m_data[0]; }

private:
    // the atomic storage.  Not initialized to zero by
    // default — the constructor handles that explicitly.
    atomic_value_type m_data[_N == 0 ? 1 : _N];
};


// =============================================================================
// II.  Trait Specializations (axis preservation)
// =============================================================================
// atomic_array's element type at the trait level is _T
// (not std::atomic<_T>) — that's what callers care about
// for compatibility checks.  Storage layout is contiguous,
// extent is static, and iteration is provided.

NS_TRAITS

template<typename _T, std::size_t _N,
         array_lifetime _L, array_iterability _I>
struct is_contiguous_array<atomic_array<_T, _N, _L, _I>>
    : std::true_type
{};

template<typename _T, std::size_t _N,
         array_lifetime _L, array_iterability _I>
struct is_iterable_array<atomic_array<_T, _N, _L, _I>>
    : std::integral_constant<bool,
        ( _I == array_iterability::iterable )>
{};

template<typename _T, std::size_t _N,
         array_lifetime _L, array_iterability _I>
struct has_static_extent<atomic_array<_T, _N, _L, _I>>
    : std::true_type
{};

template<typename _T, std::size_t _N,
         array_lifetime _L, array_iterability _I>
struct array_lifetime_of<atomic_array<_T, _N, _L, _I>>
    : std::integral_constant<array_lifetime, _L>
{};

NS_END  // traits

NS_END  // djinterp


// =============================================================================
// III. Static Verification
// =============================================================================

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

namespace djinterp { namespace internal { namespace atomic_array_verify {

    using aa = atomic_array<int, 16>;

    // structural axes — must hold
    static_assert(
        is_contiguous_array_v<aa>,
        "atomic_array must be contiguous");

    static_assert(
        is_iterable_array_v<aa>,
        "atomic_array<...,iterable> must be iterable");

    static_assert(
        has_static_extent_v<aa>,
        "atomic_array always has static extent");

    // strategy
    static_assert(
        is_atomic_container_v<aa>,
        "atomic_array failed to register as atomic");

    static_assert(
        !is_locked_container_v<aa>,
        "atomic_array misclassified as locked");

    static_assert(
        !is_cow_container_v<aa>,
        "atomic_array misclassified as cow");

}}}  // namespace internal::atomic_array_verify

#endif  // C++14

#endif  // C++11


#endif  // DJINTERP_ATOMIC_ARRAY_
