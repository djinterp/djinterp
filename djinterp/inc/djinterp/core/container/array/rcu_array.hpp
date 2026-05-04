/******************************************************************************
* djinterp                                                         rcu_array.hpp
*
* RCU (Read-Copy-Update) protected concurrent array.
*   Composes the canonical `array<T, N, L, I>` inside an rcu_protected,
* exposing reader-side critical sections backed by epoch tracking and
* deferred reclamation.
*
*   Readers enter an epoch, follow the data pointer, and exit.  No
* locks, no atomics on the read fast path beyond a single epoch
* register.  Writers copy the data, mutate the copy, atomically swap
* the published pointer, and retire the old copy for deferred
* reclamation once all in-flight readers have departed.
*
* TEMPLATE PARAMETERS:
*   _T            — element type
*   _N            — extent
*   _Lifetime     — array_lifetime tag
*   _Iterability  — array_iterability tag
*
* WHEN TO USE:
*   - Read-heavy workloads (>>50:1 read:write) where reader
*     latency must be minimal and bounded.
*   - The "swap the whole array" pattern: a periodically
*     recomputed lookup table, a feature flag set, etc.
*   - Workloads where readers operate on a snapshot of bounded
*     duration (no long-lived references across epochs).
*
* WHEN NOT TO USE:
*   - Frequent fine-grained writes (every write retires the
*     entire array; reclamation pressure dominates).
*   - Per-element mutation patterns → use threadsafe_array
*     or atomic_array.
*   - Readers that need to hold references across long
*     operations → use cow_array (snapshots are reference-counted
*     and survive epoch transitions).
*
* ACCESS PATTERNS:
*   read(fn)     — invokes fn(const array&) under an epoch guard
*   update(fn)   — copy → modify → atomic publish → retire old
*   snapshot()   — atomic snapshot pointer (zero-copy view)
*
* DEPENDENCIES:
*   array.hpp                       — wrapped container
*   threadsafe.hpp                  — rcu_protected, epoch_*
*   concurrency_strategy_traits.hpp — strategy tag types
*
* TABLE OF CONTENTS
* =================
* I.    rcu_array
*         a. Type aliases (forwarded from wrapped array)
*         b. Strategy tag and trait constants
*         c. Construction
*         d. Read Access (epoch-guarded)
*         e. Update (copy-modify-publish)
*         f. Snapshot
* II.   Trait Specializations (axis preservation)
* III.  Static Verification
*
*
* path:      /inc/djinterp/container/array/rcu_array.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.26
******************************************************************************/

#ifndef DJINTERP_RCU_ARRAY_
#define DJINTERP_RCU_ARRAY_ 1

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include "../../core/djinterp.hpp"
#include "../../sync/threadsafe.hpp"
#include "../meta/concurrency_strategy_traits.hpp"
#include "./array.hpp"
#include "./array_traits.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_DJINTERP

// =============================================================================
// I.   rcu_array
// =============================================================================

template<typename                _T,
         std::size_t             _N,
         array_lifetime _Lifetime    =
             array_lifetime::mutable_lifetime,
         array_iterability         _Iterability =
             array_iterability::iterable>
class rcu_array
{
private:
    using array_type     = array<_T, _N, _Lifetime, _Iterability>;
    using protected_type = rcu_protected<array_type>;

public:
    // -------------------------------------------------------------
    // a. Type aliases (forwarded from wrapped array)
    // -------------------------------------------------------------
    using value_type             = typename array_type::value_type;
    using size_type              = typename array_type::size_type;
    using difference_type        = typename array_type::difference_type;
    using reference              = typename array_type::reference;
    using const_reference        = typename array_type::const_reference;
    using pointer                = typename array_type::pointer;
    using const_pointer          = typename array_type::const_pointer;
    using iterator               = typename array_type::iterator;
    using const_iterator         = typename array_type::const_iterator;
    using reverse_iterator       = typename array_type::reverse_iterator;
    using const_reverse_iterator = typename array_type::const_reverse_iterator;

    // -------------------------------------------------------------
    // b. Strategy tag and trait constants
    // -------------------------------------------------------------
    using underlying_type     = array_type;
    using rcu_protected_type  = protected_type;

    // strategy tag — read by concurrency_strategy_traits
    using concurrency_strategy_tag = rcu_strategy_tag;

    // axis re-export
    D_STATIC_CONSTEXPR size_type extent = array_type::extent;
    D_STATIC_CONSTEXPR
        array_lifetime lifetime    = array_type::lifetime;
    D_STATIC_CONSTEXPR
        array_iterability         iterability = array_type::iterability;

    // -------------------------------------------------------------
    // c. Construction
    // -------------------------------------------------------------
    rcu_array() = default;

    explicit rcu_array(const array_type& _initial)
        : m_protected(_initial)
    {}

    explicit rcu_array(array_type&& _initial)
        : m_protected(std::move(_initial))
    {}

    // non-copyable / non-movable: contains an rcu_protected
    // which owns reclamation infrastructure that must not move.
    rcu_array(const rcu_array&)            = delete;
    rcu_array& operator=(const rcu_array&) = delete;

    rcu_array(rcu_array&&)            = delete;
    rcu_array& operator=(rcu_array&&) = delete;

    ~rcu_array() = default;

    // -------------------------------------------------------------
    // d. Read Access (epoch-guarded)
    // -------------------------------------------------------------

    // read
    //   invokes _fn(const array&) under an epoch_guard.
    // _fn must NOT retain the reference past return —
    // the data may be retired once the epoch closes.
    // For long-lived reads, use snapshot() instead.
    template<typename _Fn>
    auto read(_Fn&& _fn) const
        -> decltype(_fn(std::declval<const array_type&>()))
    {
        return m_protected.read(std::forward<_Fn>(_fn));
    }

    // size / at — convenience wrappers around read()
    // These return by value; the epoch guard is held only
    // during the brief read.
    size_type size() const
    {
        return m_protected.read(
            [](const array_type& _a) -> size_type
            {
                return _a.size();
            });
    }

    value_type at(size_type _i) const
    {
        return m_protected.read(
            [_i](const array_type& _a) -> value_type
            {
                return _a[_i];
            });
    }

    bool empty() const
    {
        return ( size() == 0 );
    }

    // -------------------------------------------------------------
    // e. Update (copy-modify-publish)
    // -------------------------------------------------------------

    // update
    //   the canonical RCU writer protocol:
    //     1. copy the current array
    //     2. invoke _fn(array&) on the copy
    //     3. atomically publish the new copy
    //     4. retire the old copy for deferred reclamation
    //
    // Concurrent readers see either the old or new copy
    // atomically — never an in-progress mutation.
    template<typename _Fn>
    void update(_Fn&& _fn)
    {
        m_protected.update(std::forward<_Fn>(_fn));
    }

    // replace
    //   wholesale replacement.  Equivalent to
    // update([&](array_type& a) { a = _new_value; })
    // but skips the copy of the old data.
    void replace(const array_type& _new_value)
    {
        m_protected.publish(_new_value);
    }

    void replace(array_type&& _new_value)
    {
        m_protected.publish(std::move(_new_value));
    }

    // -------------------------------------------------------------
    // f. Snapshot
    // -------------------------------------------------------------

    // snapshot
    //   returns a stable pointer-snapshot of the array.
    // Implementation depends on rcu_protected — typically
    // a reference-counted handle that survives epoch
    // transitions.  Use for long-lived reads.
    auto snapshot() const
        -> decltype(std::declval<const protected_type&>().snapshot())
    {
        return m_protected.snapshot();
    }

private:
    protected_type m_protected;
};


// =============================================================================
// II.  Trait Specializations (axis preservation)
// =============================================================================

NS_TRAITS

template<typename _T, std::size_t _N,
         array_lifetime _L, array_iterability _I>
struct is_contiguous_array<rcu_array<_T, _N, _L, _I>>
    : is_contiguous_array<array<_T, _N, _L, _I>>
{};

template<typename _T, std::size_t _N,
         array_lifetime _L, array_iterability _I>
struct is_iterable_array<rcu_array<_T, _N, _L, _I>>
    : is_iterable_array<array<_T, _N, _L, _I>>
{};

template<typename _T, std::size_t _N,
         array_lifetime _L, array_iterability _I>
struct has_static_extent<rcu_array<_T, _N, _L, _I>>
    : has_static_extent<array<_T, _N, _L, _I>>
{};

template<typename _T, std::size_t _N,
         array_lifetime _L, array_iterability _I>
struct array_lifetime_of<rcu_array<_T, _N, _L, _I>>
    : array_lifetime_of<array<_T, _N, _L, _I>>
{};

NS_END  // traits

NS_END  // djinterp


// =============================================================================
// III. Static Verification
// =============================================================================

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

namespace djinterp { namespace internal { namespace rcu_array_verify {

    using base = array<int, 16>;
    using rcu  = rcu_array<int, 16>;

    static_assert(
        is_contiguous_array_v<rcu> ==
        is_contiguous_array_v<base>,
        "rcu_array drifted on contiguity");

    static_assert(
        is_iterable_array_v<rcu> ==
        is_iterable_array_v<base>,
        "rcu_array drifted on iterability");

    static_assert(
        has_static_extent_v<rcu> ==
        has_static_extent_v<base>,
        "rcu_array drifted on extent class");

    static_assert(
        array_lifetime_of<rcu>::value ==
        array_lifetime_of<base>::value,
        "rcu_array drifted on lifetime");

    static_assert(
        is_rcu_container_v<rcu>,
        "rcu_array failed to register as rcu");

    static_assert(
        !is_rcu_container_v<base>,
        "plain array misclassified as rcu");

}}}  // namespace internal::rcu_array_verify

#endif  // C++14

#endif  // C++11


#endif  // DJINTERP_RCU_ARRAY_
