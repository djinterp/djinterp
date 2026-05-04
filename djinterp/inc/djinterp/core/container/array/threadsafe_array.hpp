/******************************************************************************
* djinterp                                                  threadsafe_array.hpp
*
* Lock-policy-protected concurrent array.
*   Composes the canonical `array<T, N, L, I>` with a chosen lock policy.
* All structural axes (capacity model, contiguity, element traits,
* lifetime, iterability, ordering) are inherited verbatim from the
* wrapped array; this module adds only axis 8 (thread safety) on top.
*
* TEMPLATE PARAMETERS:
*   _Type            — element type
*   _N            — extent (use dynamic_extent for dynamic-size)
*   _Lifetime     — array_lifetime::{mutable_lifetime, immutable_lifetime,
*                                    constexpr_lifetime}
*   _Iterability  — array_iterability::{iterable, non_iterable}
*   _Policy       — lock policy from lock_policy.hpp
*                   (default: null_lock_policy — zero overhead)
*
* THREE ACCESS TIERS:
*   Lock-free     — size(), version() through atomic_state.
*                   No synchronization cost.
*
*   Single-op     — at(i), set(i,v), assign(...).  Each acquires
*                   and releases its own lock per call.
*
*   Handle-based  — read_access() / write_access() return
*                   const_locked_ref / locked_ref RAII handles
*                   that hold a lock for the lifetime of the
*                   handle.  Use for batched operations.
*                   snapshot() copies under a read lock and then
*                   iterates without holding any lock.
*
* CONVENIENCE ALIASES:
*   mutex_array<T, N, L, I>    — exclusive_lock_policy
*   shared_array<T, N, L, I>   — shared_lock_policy (C++17)
*   timed_array<T, N, L, I>    — timed_lock_policy
*
* DEPENDENCIES:
*   array.hpp                       — wrapped container
*   threadsafe.hpp                  — lock policies, guards
*   threadsafe_container.hpp        — CRTP base, locked_ref,
*                                     atomic_state, snapshot_view
*   concurrency_strategy_traits.hpp — strategy tag types
*
* TABLE OF CONTENTS
* =================
* I.    threadsafe_array
*         a. Type aliases (forwarded from wrapped array)
*         b. Strategy tag and trait constants
*         c. Construction / Assignment
*         d. Lock-Free Queries
*         e. Handle-Based Access
*         f. Element Access (locked)
*         g. Bulk Operations (locked)
*         h. Optimistic Read
*         i. Snapshot
* II.   Convenience Aliases
* III.  Trait Specializations (axis preservation)
*
*
* path:      /inc/djinterp/container/array/threadsafe_array.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.26
******************************************************************************/

#ifndef DJINTERP_THREADSAFE_ARRAY_
#define DJINTERP_THREADSAFE_ARRAY_ 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../../sync/threadsafe.hpp"
#include "../threadsafe_container.hpp"
#include "../traits/concurrency_strategy_traits.hpp"
#include "./array.hpp"
#include "./array_traits.hpp"


NS_DJINTERP

// =============================================================================
// I.   threadsafe_array
// =============================================================================

template<typename          _Type,
         std::size_t       _N,
         array_lifetime    _Lifetime = array_lifetime::mutable_lifetime,
         array_iterability _Iterability = array_iterability::iterable,
         typename _Policy = default_lock_policy>
class threadsafe_array
    : public threadsafe_container_base<
          threadsafe_array<_Type, _N, _Lifetime, _Iterability, _Policy>,
          _Policy>
{
private:
    using base_type = threadsafe_container_base<
        threadsafe_array<_Type, _N, _Lifetime, _Iterability, _Policy>,
        _Policy>;

    using array_type = array<_Type, _N, _Lifetime, _Iterability>;

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
    using underlying_type        = array_type;
    using lock_policy_type       = _Policy;
    using mutex_type             = typename _Policy::mutex_type;
    using read_lock_type         = typename _Policy::read_lock_type;
    using write_lock_type        = typename _Policy::write_lock_type;

    // strategy tag — read by concurrency_strategy_traits
    using concurrency_strategy_tag = locked_strategy_tag;

    // axis re-export (inherited verbatim from the wrapped array)
    D_STATIC_CONSTEXPR size_type extent = array_type::extent;
    D_STATIC_CONSTEXPR
        array_lifetime lifetime    = array_type::lifetime;
    D_STATIC_CONSTEXPR
        array_iterability         iterability = array_type::iterability;

    // -------------------------------------------------------------
    // c. Construction / Assignment
    // -------------------------------------------------------------
    threadsafe_array() = default;

    template<typename... _Args>
    explicit threadsafe_array(_Args&&... _args)
        : m_data(std::forward<_Args>(_args)...)
    {}

    // copy: lock the source for the duration of the copy
    threadsafe_array(const threadsafe_array& _other)
    {
        typename _Policy::read_lock_type guard(
            _other.base_type::mutex());

        m_data = _other.m_data;
    }

    threadsafe_array& operator=(const threadsafe_array& _other)
    {
        if (this != &_other)
        {
            // lock both, lower-address first to prevent deadlock
            const threadsafe_array* first  = this;
            const threadsafe_array* second = &_other;

            if (second < first)
            {
                first  = &_other;
                second = this;
            }

            typename _Policy::write_lock_type g1(
                first->base_type::mutex());
            typename _Policy::write_lock_type g2(
                second->base_type::mutex());

            m_data = _other.m_data;
            m_state.increment_version();
        }

        return *this;
    }

    // move: not synchronizable in a portable way (the source's
    // mutex may be destroyed before this completes).  Disabled.
    threadsafe_array(threadsafe_array&&)            = delete;
    threadsafe_array& operator=(threadsafe_array&&) = delete;

    ~threadsafe_array() = default;

    // -------------------------------------------------------------
    // d. Lock-Free Queries
    // -------------------------------------------------------------
    // size() and version() use the atomic_state; no lock needed.

    size_type size_lockfree() const noexcept
    {
        return m_state.load_size();
    }

    std::uint64_t version() const noexcept
    {
        return m_state.load_version();
    }

    bool empty_lockfree() const noexcept
    {
        return ( m_state.load_size() == 0 );
    }

    // -------------------------------------------------------------
    // e. Handle-Based Access
    // -------------------------------------------------------------

    const_locked_ref<array_type, _Policy>
    read_access() const
    {
        return const_locked_ref<array_type, _Policy>(
            m_data, base_type::mutex());
    }

    locked_ref<array_type, _Policy>
    write_access()
    {
        return locked_ref<array_type, _Policy>(
            m_data, base_type::mutex());
    }

    // batch
    //   acquires a write lock and returns a batch_guard
    // for multi-operation atomicity.  Use with the
    // write_access() handle inside the batch scope.
    batch_guard<_Policy> batch()
    {
        return batch_guard<_Policy>(base_type::mutex());
    }

    // -------------------------------------------------------------
    // f. Element Access (locked)
    // -------------------------------------------------------------

    // size
    //   read-locked size query.  Prefer size_lockfree() when
    // exact-but-stale is acceptable.
    size_type size() const
    {
        typename _Policy::read_lock_type guard(
            base_type::mutex());

        return m_data.size();
    }

    bool empty() const
    {
        typename _Policy::read_lock_type guard(
            base_type::mutex());

        return ( m_data.size() == 0 );
    }

    // at
    //   returns a copy of element at index _i under a read
    // lock.  Returning by value (not reference) keeps the
    // caller from holding a stale pointer after the lock
    // releases.
    value_type at(size_type _i) const
    {
        typename _Policy::read_lock_type guard(
            base_type::mutex());

        return m_data[_i];
    }

    // set
    //   updates element _i to _v under a write lock and
    // bumps the version.
    void set(size_type         _i,
             const value_type& _v)
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        m_data[_i] = _v;
        m_state.increment_version();
    }

    // -------------------------------------------------------------
    // g. Bulk Operations (locked)
    // -------------------------------------------------------------

    // assign
    //   replaces the entire contents under a write lock.
    void assign(const array_type& _src)
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        m_data = _src;
        m_state.store_size(_src.size());
        m_state.increment_version();
    }

    // apply
    //   acquires a write lock, invokes _fn on the array,
    // bumps the version, returns the result.
    template<typename _Fn>
    auto apply(_Fn&& _fn)
        -> decltype(_fn(std::declval<array_type&>()))
    {
        typename _Policy::write_lock_type guard(
            base_type::mutex());

        m_state.increment_version();

        return std::forward<_Fn>(_fn)(m_data);
    }

    // apply_read
    //   read-locked counterpart of apply.  No version
    // bump because no mutation is supposed to occur.
    template<typename _Fn>
    auto apply_read(_Fn&& _fn) const
        -> decltype(_fn(std::declval<const array_type&>()))
    {
        typename _Policy::read_lock_type guard(
            base_type::mutex());

        return std::forward<_Fn>(_fn)(m_data);
    }

    // -------------------------------------------------------------
    // h. Optimistic Read
    // -------------------------------------------------------------

    // optimistic
    //   attempts up to _max_retries lock-free reads under
    // version-check protection.  Falls back to a real read
    // lock on the final attempt.  Use only for short read
    // closures.
    template<typename _Fn>
    auto optimistic(
        _Fn&&    _fn,
        unsigned _max_retries = 3) const
        -> decltype(_fn(std::declval<const array_type&>()))
    {
        return optimistic_read<array_type, _Policy>(
            m_state,
            m_data,
            base_type::mutex(),
            std::forward<_Fn>(_fn),
            _max_retries);
    }

    // -------------------------------------------------------------
    // i. Snapshot
    // -------------------------------------------------------------

    // snapshot
    //   copies the array under a read lock and returns a
    // snapshot_view that iterates without holding any lock.
    snapshot_view<array_type, _Policy> snapshot() const
    {
        return snapshot_view<array_type, _Policy>(
            m_data, base_type::mutex());
    }

private:
    array_type   m_data;
    atomic_state m_state;
};


// =============================================================================
// II.  Convenience Aliases
// =============================================================================

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// mutex_array
//   alias: threadsafe_array with exclusive_lock_policy.
template<typename _Type,
         std::size_t _N,
         array_lifetime _Lifetime    =
             array_lifetime::mutable_lifetime,
         array_iterability         _Iterability =
             array_iterability::iterable>
using mutex_array =
    threadsafe_array<_Type, _N, _Lifetime, _Iterability,
                     exclusive_lock_policy>;

// timed_array
//   alias: threadsafe_array with timed_lock_policy.
template<typename _Type,
         std::size_t _N,
         array_lifetime _Lifetime    =
             array_lifetime::mutable_lifetime,
         array_iterability         _Iterability =
             array_iterability::iterable>
using timed_array =
    threadsafe_array<_Type, _N, _Lifetime, _Iterability,
                     timed_lock_policy>;

#endif  // C++11

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// shared_array
//   alias: threadsafe_array with shared_lock_policy
// (multi-reader, single-writer).
template<typename _Type,
         std::size_t _N,
         array_lifetime _Lifetime    =
             array_lifetime::mutable_lifetime,
         array_iterability         _Iterability =
             array_iterability::iterable>
using shared_array =
    threadsafe_array<_Type, _N, _Lifetime, _Iterability,
                     shared_lock_policy>;

#endif  // C++17


// =============================================================================
// III. Trait Specializations (axis preservation)
// =============================================================================
// Delegate every structural classification to the wrapped
// array.  This is what guarantees axes 1-7 are preserved
// across the concurrency wrapping.


template<typename _Type, std::size_t _N,
         array_lifetime _L, array_iterability _I, typename _Policy>
struct is_contiguous_array<threadsafe_array<_Type, _N, _L, _I, _Policy>>
    : is_contiguous_array<array<_Type, _N, _L, _I>>
{};

template<typename _Type, std::size_t _N,
         array_lifetime _L, array_iterability _I, typename _Policy>
struct is_iterable_array<threadsafe_array<_Type, _N, _L, _I, _Policy>>
    : is_iterable_array<array<_Type, _N, _L, _I>>
{};

template<typename _Type, std::size_t _N,
         array_lifetime _L, array_iterability _I, typename _Policy>
struct has_static_extent<threadsafe_array<_Type, _N, _L, _I, _Policy>>
    : has_static_extent<array<_Type, _N, _L, _I>>
{};

template<typename _Type, std::size_t _N,
         array_lifetime _L, array_iterability _I, typename _Policy>
struct array_lifetime_of<threadsafe_array<_Type, _N, _L, _I, _Policy>>
    : array_lifetime_of<array<_Type, _N, _L, _I>>
{};





// =============================================================================
// IV.  Static Verification (axes 1-7 unchanged, axis 8 differs)
// =============================================================================
// Compile-time guarantee that wrapping does not drift any
// structural axis.  Any future change to either array's
// classification or threadsafe_array's specializations
// breaks the build until reconciled.

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

NS_INTERNAL

    using base = array<int, 16>;
    using ts   = threadsafe_array<int, 16>;

    // axes 1-7: must match
    static_assert(
        is_contiguous_array_v<ts> ==
        is_contiguous_array_v<base>,
        "threadsafe_array drifted on contiguity");

    static_assert(
        is_iterable_array_v<ts> ==
        is_iterable_array_v<base>,
        "threadsafe_array drifted on iterability");

    static_assert(
        has_static_extent_v<ts> ==
        has_static_extent_v<base>,
        "threadsafe_array drifted on extent class");

    static_assert(
        array_lifetime_of<ts>::value ==
        array_lifetime_of<base>::value,
        "threadsafe_array drifted on lifetime");

    // axis 8: must differ
    static_assert(
        is_locked_container_v<ts>,
        "threadsafe_array failed to register as locked");

    static_assert(
        !is_locked_container_v<base>,
        "plain array misclassified as locked");

NS_END  // internal
NS_END  // djinterp


#endif  // C++14


#endif  // DJINTERP_THREADSAFE_ARRAY_