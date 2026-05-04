/******************************************************************************
* djinterp [container]                                           cow_array.hpp
*
* Copy-on-write concurrent array.
*   Composes the canonical `array<T, N, L, I>` inside a cow_state, exposing
* snapshot-based read access and clone-on-write semantics.
*
*   Readers receive immutable_snapshot handles that share the underlying
* storage via reference counting.  When a writer mutates, the storage
* is cloned only if any snapshot still holds it — readers never see
* a torn state, and they never block writers.
*
* TEMPLATE PARAMETERS:
*   _T            — element type
*   _N            — extent (use dynamic_extent for dynamic-size)
*   _Lifetime     — array_lifetime::{mutable_lifetime, ...}
*   _Iterability  — array_iterability::{iterable, non_iterable}
*   _Policy       — lock policy that protects the cow_state
*                   (default: null_lock_policy)
*
* WHEN TO USE:
*   - Read-mostly workloads (config tables, lookup vectors).
*   - Small to medium arrays where a full clone is cheap.
*   - When readers must not block writers and must see a
*     consistent point-in-time view.
*
* WHEN NOT TO USE:
*   - Frequent writes (every write under contention clones).
*   - Very large arrays (clone cost dominates).
*   - Per-element atomic updates → use atomic_array instead.
*
* ACCESS PATTERNS:
*   read()                — returns const ref under read lock (short)
*   snapshot()            — returns immutable_snapshot (long-lived)
*   modify(fn)            — clone-if-shared, mutate, bump version
*   replace(new_value)    — atomically swap entire contents
*
* DEPENDENCIES:
*   array.hpp                       — wrapped container
*   threadsafe.hpp                  — cow_state, immutable_snapshot
*   concurrency_strategy_traits.hpp — strategy tag types
*
*
* path:      /inc/djinterp/container/array/cow_array.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.26
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    cow_array
        a. Type aliases (forwarded from wrapped array)
        b. Strategy tag and trait constants
        c. Construction
        d. Read Access
        e. Snapshot
        f. Write Access
        g. Version Query
II.   Trait Specializations (axis preservation)
III.  Static Verification
*/

#ifndef DJINTERP_COW_ARRAY_
#define DJINTERP_COW_ARRAY_ 1

// std
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../../sync/threadsafe.hpp"
#include "../traits/concurrency_strategy_traits.hpp"
#include "./array.hpp"
#include "./array_traits.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_DJINTERP

// =============================================================================
// I.   cow_array
// =============================================================================

template<typename          _T,
         std::size_t       _N,
         array_lifetime    _Lifetime    = array_lifetime::mutable_lifetime,
         array_iterability _Iterability = array_iterability::iterable,
         typename          _Policy      = default_lock_policy>
class cow_array
{
private:
    using array_type = array<_T, _N, _Lifetime, _Iterability>;
    using state_type = cow_state<array_type, _Policy>;

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
    using cow_state_type         = state_type;
    using snapshot_type          =
        immutable_snapshot<array_type>;

    // strategy tag — read by concurrency_strategy_traits
    using concurrency_strategy_tag = cow_strategy_tag;

    // axis re-export
    D_STATIC_CONSTEXPR size_type extent = array_type::extent;
    D_STATIC_CONSTEXPR
        array_lifetime lifetime    = array_type::lifetime;
    D_STATIC_CONSTEXPR
        array_iterability         iterability = array_type::iterability;

    // -------------------------------------------------------------
    // c. Construction
    // -------------------------------------------------------------
    cow_array() = default;

    explicit cow_array(const array_type& _initial)
        : m_state(_initial)
    {}

    explicit cow_array(array_type&& _initial)
        : m_state(std::move(_initial))
    {}

    // non-copyable (contains a mutex inside cow_state).
    // To copy, take a snapshot and construct from it.
    cow_array(const cow_array&)            = delete;
    cow_array& operator=(const cow_array&) = delete;

    cow_array(cow_array&&)            = delete;
    cow_array& operator=(cow_array&&) = delete;

    ~cow_array() = default;

    // -------------------------------------------------------------
    // d. Read Access
    // -------------------------------------------------------------

    // read
    //   returns a const reference to the current array
    // under a read lock.  The lock is held only for the
    // duration of the call — DO NOT retain the reference
    // beyond it.  For long-lived access, use snapshot().
    const array_type& read() const
    {
        return m_state.read();
    }

    // size / empty / at — convenience wrappers around read()
    size_type size() const
    {
        return m_state.read().size();
    }

    bool empty() const
    {
        return ( m_state.read().size() == 0 );
    }

    // at
    //   returns a copy of element _i (no lifetime concern).
    value_type at(size_type _i) const
    {
        return m_state.read()[_i];
    }

    // -------------------------------------------------------------
    // e. Snapshot
    // -------------------------------------------------------------

    // snapshot
    //   takes an immutable_snapshot of the current state.
    // The snapshot is reference-counted and independent of
    // any future mutations.  This is the canonical way to
    // iterate or perform extended reads.
    snapshot_type snapshot() const
    {
        return m_state.snapshot();
    }

    // -------------------------------------------------------------
    // f. Write Access
    // -------------------------------------------------------------

    // modify
    //   acquires a write lock, clones the array if shared
    // with any snapshot, invokes _fn on a mutable
    // reference, bumps the version.
    template<typename _Fn>
    auto modify(_Fn&& _fn)
        -> decltype(_fn(std::declval<array_type&>()))
    {
        return m_state.modify(std::forward<_Fn>(_fn));
    }

    // replace
    //   atomically replaces the entire array.  Existing
    // snapshots remain valid (they hold the previous
    // generation's data).
    void replace(const array_type& _new_value)
    {
        m_state.replace(_new_value);
    }

    void replace(array_type&& _new_value)
    {
        m_state.replace(std::move(_new_value));
    }

    // set
    //   convenience: clone-if-shared, write a single
    // element, bump the version.  Equivalent to a
    // single-element modify().
    void set(size_type         _i,
             const value_type& _v)
    {
        m_state.modify(
            [&](array_type& _a)
            {
                _a[_i] = _v;
            });
    }

    // -------------------------------------------------------------
    // g. Version Query
    // -------------------------------------------------------------

    std::uint64_t version() const noexcept
    {
        return m_state.version();
    }

    mutex_type& mutex() const noexcept
    {
        return m_state.mutex();
    }

private:
    state_type m_state;
};


// =============================================================================
// II.  Trait Specializations (axis preservation)
// =============================================================================


template<typename _T, std::size_t _N,
         array_lifetime _L, array_iterability _I, typename _Policy>
struct is_contiguous_array<cow_array<_T, _N, _L, _I, _Policy>>
    : is_contiguous_array<array<_T, _N, _L, _I>>
{};

template<typename _T, std::size_t _N,
         array_lifetime _L, array_iterability _I, typename _Policy>
struct is_iterable_array<cow_array<_T, _N, _L, _I, _Policy>>
    : is_iterable_array<array<_T, _N, _L, _I>>
{};

template<typename _T, std::size_t _N,
         array_lifetime _L, array_iterability _I, typename _Policy>
struct has_static_extent<cow_array<_T, _N, _L, _I, _Policy>>
    : has_static_extent<array<_T, _N, _L, _I>>
{};

template<typename _T, std::size_t _N,
         array_lifetime _L, array_iterability _I, typename _Policy>
struct array_lifetime_of<cow_array<_T, _N, _L, _I, _Policy>>
    : array_lifetime_of<array<_T, _N, _L, _I>>
{};




// =============================================================================
// III. Static Verification
// =============================================================================

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

NS_INTERNAL 

    using base = array<int, 16>;
    using cow  = cow_array<int, 16>;

    static_assert(
        is_contiguous_array_v<cow> == is_contiguous_array_v<base>,
        "cow_array drifted on contiguity");

    static_assert(
        is_iterable_array_v<cow> == is_iterable_array_v<base>,
        "cow_array drifted on iterability");

    static_assert(
        has_static_extent_v<cow> == has_static_extent_v<base>,
        "cow_array drifted on extent class");

    static_assert(
        array_lifetime_of<cow>::value == array_lifetime_of<base>::value,
        "cow_array drifted on lifetime");

    // axis 8 — should classify as cow
    static_assert(
        is_cow_container_v<cow>,
        "cow_array failed to register as cow");

    static_assert(
        !is_cow_container_v<base>,
        "plain array misclassified as cow");

NS_END  // internal

#endif  // C++14

NS_END  // djinterp

#endif  // C++11


#endif  // DJINTERP_COW_ARRAY_