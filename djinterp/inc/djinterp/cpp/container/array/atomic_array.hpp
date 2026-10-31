/*******************************************************************************
* djinterp [cpp]                                                atomic_array.hpp
*
* djinterp atomic_array.hpp
*
* Lock-free atomic-element array.
*   Stores `std::atomic<T>` elements with element-level lock-free
* access.  No mutex, no lock policy, no version counter at the
* container level - synchronization is per-element, leveraging the
* atomic primitives in std::atomic.
*
*   Distinct from DThreadsafeArray (which protects whole-array
* access under a lock policy) and from DCowArray / rcu_array
* (which present whole-array snapshots).  DAtomicArray exposes
* per-slot atomic operations: load, store, fetch_add,
* compare_exchange.
*
*   `DAtomicArray<Options...>` follows the framework
* options-container contract: a single template parameter pack.
* Element type and extent are read from the same option keys that
* drive `array<>`; the lifetime / iterability axes are honoured
* but in practice DAtomicArray is always lock-free-mutable
* (an immutable DAtomicArray is a contradiction).  No lock policy
* is consumed - if a lock-policy option appears in the pack, it is
* silently ignored.
*
*
* path:      /inc/djinterp/cpp/container/array/atomic_array.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.26
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    DAtomicArray
      ------------
      a. Type aliases
      b. Strategy tag and trait constants
      c.    Construction
            d. Element Atomic Access
            e. Element Atomic Updates
            f. Element CAS
            g. Bulk Operations
            h. Iteration (per-element atomic)

II.   Trait Specializations (axis preservation)
      -----------------------------------------

III.  Static Verification
      -------------------
*/

#ifndef DJINTERP_CONTAINER_ARRAY_ATOMIC_ARRAY_HPP
#define DJINTERP_CONTAINER_ARRAY_ATOMIC_ARRAY_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"
#include "../traits/concurrency_strategy_traits.hpp"
#include "../container_options.hpp"
#include "./array.hpp"
#include "./array_options.hpp"
#include "./array_traits.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <atomic>


NS_DJINTERP


// =============================================================================
// I.   DAtomicArray
// =============================================================================

template<typename... Options>
class DAtomicArray
{
private:
    using options_type = option_set<Options...>;

    // Reuse the array<> resolver for element type / extent / lifetime /
    // iterability. DAtomicArray's storage is
    // std::atomic<element_type>, but the element_type itself is what callers
    // care about for compatibility checks.
    using axes_resolver = internal::DArrayAxesResolver<
        options_type,
        0>;

    static_assert(
        !std::is_same<typename axes_resolver::element_type,
                      void>::value,
        "djinterp::atomic_array: an `array_type_key` option (or "
        "`array_opt_type<T>`) must be present in the options pack "
        "to identify the element type.");

public:
    // -------------------------------------------------------------
    // a. Type aliases
    // -------------------------------------------------------------
    using value_type        = typename axes_resolver::element_type;
    using atomic_value_type = std::atomic<value_type>;
    using size_type         = std::size_t;
    using difference_type   = std::ptrdiff_t;
    using reference         = atomic_value_type&;
    using const_reference   = const atomic_value_type&;
    using pointer           = atomic_value_type*;
    using const_pointer     = const atomic_value_type*;
    using iterator          = atomic_value_type*;
    using const_iterator    = const atomic_value_type*;

    static_assert(
        std::is_trivially_copyable<value_type>::value,
        "atomic_array element type must be trivially copyable "
        "(std::atomic<T> requirement)");

    // -------------------------------------------------------------
    // b. Strategy tag and trait constants
    // -------------------------------------------------------------

    // strategy tag - read by concurrency_strategy_traits
    using concurrency_strategy_tag = atomic_strategy_tag;

    // axis re-export
    D_STATIC_CONSTEXPR size_type         extent      = axes_resolver::extent;
    D_STATIC_CONSTEXPR DArrayLifetime    lifetime    = axes_resolver::lifetime;
    D_STATIC_CONSTEXPR DArrayIterability iterability = axes_resolver::iterability;
    D_STATIC_CONSTEXPR bool              iterable    = axes_resolver::iterable;

    // -------------------------------------------------------------
    // c. Construction
    // -------------------------------------------------------------
    DAtomicArray() noexcept
    {
        for (size_type i = 0; i < extent; ++i)
        {
            m_data[i].store(value_type{},
                std::memory_order_relaxed);
        }
    }

    explicit
    DAtomicArray(
        const value_type& _fill
    ) noexcept
    {
        for (size_type i = 0; i < extent; ++i)
        {
            m_data[i].store(_fill,
                std::memory_order_relaxed);
        }
    }

    // non-copyable: std::atomic is non-copyable.
    DAtomicArray(
        const DAtomicArray&
    ) = delete;
    DAtomicArray& operator=(
        const DAtomicArray&
    ) = delete;

    DAtomicArray(
        DAtomicArray&&
    ) = delete;
    DAtomicArray& operator=(
        DAtomicArray&&
    ) = delete;

    ~DAtomicArray() = default;

    // -------------------------------------------------------------
    // d. Element Atomic Access
    // -------------------------------------------------------------
    value_type load(
        size_type         _i,
        std::memory_order _order = std::memory_order_seq_cst
    ) const noexcept
    {
        return m_data[_i].load(_order);
    }

    void store(
        size_type         _i,
        value_type        _v,
        std::memory_order _order = std::memory_order_seq_cst
    ) noexcept
    {
        m_data[_i].store(_v, _order);
    }

    value_type exchange(
        size_type         _i,
        value_type        _v,
        std::memory_order _order = std::memory_order_seq_cst
    ) noexcept
    {
        return m_data[_i].exchange(_v, _order);
    }

    // -------------------------------------------------------------
    // e. Element Atomic Updates
    // -------------------------------------------------------------
    value_type fetch_add(
        size_type         _i,
        value_type        _n,
        std::memory_order _order = std::memory_order_seq_cst
    ) noexcept
    {
        return m_data[_i].fetch_add(_n, _order);
    }

    value_type fetch_sub(
        size_type         _i,
        value_type        _n,
        std::memory_order _order = std::memory_order_seq_cst
    ) noexcept
    {
        return m_data[_i].fetch_sub(_n, _order);
    }

    value_type fetch_and(
        size_type         _i,
        value_type        _n,
        std::memory_order _order = std::memory_order_seq_cst
    ) noexcept
    {
        return m_data[_i].fetch_and(_n, _order);
    }

    value_type fetch_or(
        size_type         _i,
        value_type        _n,
        std::memory_order _order = std::memory_order_seq_cst
    ) noexcept
    {
        return m_data[_i].fetch_or(_n, _order);
    }

    value_type fetch_xor(
        size_type         _i,
        value_type        _n,
        std::memory_order _order = std::memory_order_seq_cst
    ) noexcept
    {
        return m_data[_i].fetch_xor(_n, _order);
    }

    // -------------------------------------------------------------
    // f. Element CAS
    // -------------------------------------------------------------
    bool compare_exchange_weak(
        size_type         _i,
        value_type&       _expected,
        value_type        _desired,
        std::memory_order _success = std::memory_order_seq_cst,
        std::memory_order _failure = std::memory_order_seq_cst
    ) noexcept
    {
        return m_data[_i].compare_exchange_weak(
            _expected, _desired, _success, _failure);
    }

    bool compare_exchange_strong(
        size_type         _i,
        value_type&       _expected,
        value_type        _desired,
        std::memory_order _success = std::memory_order_seq_cst,
        std::memory_order _failure = std::memory_order_seq_cst
    ) noexcept
    {
        return m_data[_i].compare_exchange_strong(
            _expected, _desired, _success, _failure);
    }

    // -------------------------------------------------------------
    // g. Bulk Operations
    // -------------------------------------------------------------
    constexpr
    size_type size() const noexcept
    {
        return extent;
    }

    constexpr
    bool empty() const noexcept
    {
        return ( extent == 0 );
    }

    // fill
    void fill(
        const value_type& _v,
        std::memory_order _order = std::memory_order_seq_cst
    ) noexcept
    {
        for (size_type i = 0; i < extent; ++i)
        {
            m_data[i].store(_v, _order);
        }
    }

    bool is_lock_free() const noexcept
    {
        return ( extent > 0 ? m_data[0].is_lock_free() : true );
    }

    // -------------------------------------------------------------
    // h. Iteration (per-element atomic)
    // -------------------------------------------------------------
    iterator       begin()       noexcept { return &m_data[0]; }
    iterator       end()         noexcept { return &m_data[0] + extent; }
    const_iterator begin() const noexcept { return &m_data[0]; }
    const_iterator end()   const noexcept { return &m_data[0] + extent; }
    const_iterator cbegin() const noexcept { return begin(); }
    const_iterator cend()   const noexcept { return end();   }

    pointer       data() noexcept       { return &m_data[0]; }
    const_pointer data() const noexcept { return &m_data[0]; }

private:
    static constexpr size_type _storage_n = (extent == 0 ? 1 : extent);

    atomic_value_type m_data[_storage_n];
};


// =============================================================================
// II.  Trait Specializations (axis preservation)
// =============================================================================
//   DAtomicArray's element type at the trait level is the resolved
// `value_type` (not `std::atomic<value_type>`).  Storage layout is
// contiguous, extent is static, iteration is provided when the
// iterability axis says so.
//
//   The primaries for `is_contiguous_array`, `is_iterable_array`,
// `has_static_extent`, and `array_lifetime_of` live directly in
// `djinterp::` (see array_traits.hpp), NOT in `djinterp::`.
// The specializations below match the primaries' namespace and so
// don't trigger the Microsoft-template "specialization outside
// enclosing namespace" warning.

template<typename... Options>
struct is_contiguous_array<DAtomicArray<Options...>>
    : std::true_type
{};

template<typename... Options>
struct is_iterable_array<DAtomicArray<Options...>>
    : std::integral_constant<bool,
        DAtomicArray<Options...>::iterable>
{};

template<typename... Options>
struct has_static_extent<DAtomicArray<Options...>>
    : std::true_type
{};

template<typename... Options>
struct array_lifetime_of<DAtomicArray<Options...>>
    : std::integral_constant<DArrayLifetime,
                             DAtomicArray<Options...>::lifetime>
{};


NS_END  // djinterp


// =============================================================================
// III. Static Verification
// =============================================================================

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

NS_DJINTERP namespace internal { namespace atomic_array_verify {

    using aa = DAtomicArray<array_opt_type<int>, DArrayOptExtent<16>>;

    // structural axes - must hold
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

}}}  // namespace djinterp::internal::atomic_array_verify

#endif  // C++14

#endif  // C++11

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_ARRAY_ATOMIC_ARRAY_HPP
