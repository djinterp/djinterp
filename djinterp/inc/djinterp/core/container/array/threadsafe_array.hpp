/*******************************************************************************
* djinterp [core]                                           threadsafe_array.hpp
*
* djinterp threadsafe_array.hpp
*
* Lock-policy-protected concurrent array.
*   Composes the canonical `array<Options...>` with a chosen lock
* policy.  All structural axes (capacity model, contiguity, element
* traits, lifetime, iterability, ordering) are inherited verbatim
* from the wrapped array; this module adds only axis 8 (thread
* safety) on top.
*
*   `threadsafe_array<Options...>` follows the framework
* options-container contract: a single template parameter pack,
* normalized via `with_options_pack`, with axes resolved from the
* same option_list that drives the wrapped `array<>`.  The lock
* policy is read from the pack's lock-policy option
* (`container_opt_lock_policy<P>`); when absent, the default is
* `default_lock_policy`.
*
* THREE ACCESS TIERS:
*   Lock-free     - size(), version() through atomic_state.
*                   No synchronization cost.
*   Single-op     - at(i), set(i,v), assign(...).  Each acquires
*                   and releases its own lock per call.
*   Handle-based  - read_access() / write_access() return
*                   const_locked_ref / locked_ref RAII handles
*                   that hold a lock for the lifetime of the
*                   handle.  Use for batched operations.
*                   snapshot() copies under a read lock and then
*                   iterates without holding any lock.
*
* CONVENIENCE ALIASES:
*   mutex_array<Options...>     - exclusive_lock_policy
*   shared_array<Options...>    - shared_lock_policy (C++17)
*   timed_array<Options...>     - timed_lock_policy
*
* DEPENDENCIES:
*   array.hpp                       - wrapped container
*   container_options.hpp           - container_axis, container_opt_*
*   options/option_traits.hpp       - normalize_options_t,
*                                     option_list_lookup_t
*   threadsafe.hpp                  - lock policies, guards
*   threadsafe_container.hpp        - CRTP base, locked_ref,
*                                     atomic_state, snapshot_view
*   concurrency_strategy_traits.hpp - strategy tag types
*
*
* path:      /inc/djinterp/core/container/array/threadsafe_array.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.26
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Internal lock-policy resolver
      -----------------------------

II.   threadsafe_array
      ----------------
      a. Type aliases (forwarded from wrapped array)
      b. Strategy tag and trait constants
      c.    Construction / Assignment
            d. Lock-Free Queries
            e. Handle-Based Access
            f. Element Access (locked)
            g. Bulk Operations (locked)
            h. Optimistic Read
      i.    Snapshot

III.  Convenience Aliases
      -------------------

IV.   Trait Specializations (axis preservation)
      -----------------------------------------

V.    Static Verification
      -------------------
*/

#ifndef DJINTERP_CONTAINER_ARRAY_THREADSAFE_ARRAY_HPP
#define DJINTERP_CONTAINER_ARRAY_THREADSAFE_ARRAY_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../../djinterp.hpp"
#include "../../sync/threadsafe.hpp"
#include "../container_options.hpp"
#include "../threadsafe_container.hpp"
#include "../traits/concurrency_strategy_traits.hpp"
#include "./array.hpp"
#include "./array_options.hpp"
#include "./array_traits.hpp"
// re_std
#include "../../../../re_std/cstdint/cstdint.hpp"  // re_std::uint64_t


NS_DJINTERP


// =============================================================================
// I.   Internal lock-policy resolver
// =============================================================================

NS_INTERNAL

    // resolve_lock_policy
    //   helper: looks up a lock policy class in an option_set under
    // `container_axis::lock_policy`, falling back to `default_lock_policy`
    // when absent. Used by every wrapper that consumes a lock policy via the
    // options-pack form.
    template<typename Set>
    struct resolve_lock_policy
    {
        using type = container_axis_type_t<
            Set,
            container_axis::lock_policy,
            default_lock_policy>;
    };

    template<typename Set>
    using resolve_lock_policy_t =
        typename resolve_lock_policy<Set>::type;

NS_END  // internal


// =============================================================================
// II.  threadsafe_array
// =============================================================================

NS_INTERNAL

    // ts_strip_lp_helper
    //   metafunction: filters `option<container_axis::lock_policy, _>` entries
    // out of a
    // parameter pack and emits an `array<Filtered...>`. Used by
    // `threadsafe_array` to strip the lock-policy option that the convenience
    // aliases (mutex_array, timed_array, shared_array) append to `Options...`
    // before forming the underlying array's type.
    //
    //   Why filter only the lock policy, not canonicalize every axis?
    //   - The user-facing test contract for
    //     `wrapper::underlying_type` is type-equality with
    //     `array<the user's exact options>`. Replacing the
    //     user's pack with a re-emitted four-option canonical
    //     form breaks that contract whenever the user
    //     passes fewer than four options (e.g.
    //     `threadsafe_array<array_opt_type<int>,
    //                       array_opt_extent<8>>::
    //                       underlying_type` is expected to
    //     equal `array<array_opt_type<int>,
    //                  array_opt_extent<8>>`).
    //   - At the same time, the lambda-parameter contract
    //     for `apply()` / `apply_read()` / `optimistic()` /
    //     `assign()` requires that `array_type` does NOT
    //     gain a lock_policy option just because a
    //     convenience alias appended one. Filtering only
    //     that key satisfies both contracts simultaneously.

    template<typename Option>
    struct ts_is_lock_policy_option : std::false_type
    {};

    template<typename Value>
    struct ts_is_lock_policy_option<
        option<container_axis::lock_policy, Value>>
        : std::true_type
    {};

    // ts_kept
    //   type: the accumulator of the recursion below -- a bare list of the
    // options kept so far, which only ts_strip_lp_impl unpacks.
    template<typename... Kept>
    struct ts_kept
    {};

    // accumulator-style recursion: `Accum` collects the non-lock-policy
    // options seen so far; `Rest...` is the unexamined tail of the original
    // pack.
    template<typename Accum, typename... Rest>
    struct ts_strip_lp_impl;

    template<typename... A>
    struct ts_strip_lp_impl<ts_kept<A...>>
    {
        using type = array<A...>;
    };

    template<typename... A,
             typename First,
             typename... Rest>
    struct ts_strip_lp_impl<ts_kept<A...>,
                            First, Rest...>
    {
        using type = typename std::conditional<
            ts_is_lock_policy_option<First>::value,
            ts_strip_lp_impl<ts_kept<A...>, Rest...>,
            ts_strip_lp_impl<ts_kept<A..., First>,
                             Rest...>
        >::type::type;
    };

    template<typename... Options>
    using ts_array_without_lp_t =
        typename ts_strip_lp_impl<ts_kept<>,
                                  Options...>::type;

NS_END  // internal


template<typename... Options>
class threadsafe_array
    : public threadsafe_container_base<
          threadsafe_array<Options...>,
          internal::resolve_lock_policy_t<
              option_set<Options...>>>
{
private:
    using options_type = option_set<Options...>;

    //   array_type is the user's pack verbatim with one exception: the
    // lock-policy option (when present, typically appended by
    // mutex_array / timed_array / shared_array) is filtered out. See
    // `internal::ts_strip_lp_impl` above for the rationale.
    using array_type =
        internal::ts_array_without_lp_t<Options...>;

public:
    using lock_policy_type =
        internal::resolve_lock_policy_t<options_type>;

private:
    using base_type = threadsafe_container_base<
        threadsafe_array<Options...>,
        lock_policy_type>;

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
    using mutex_type             = typename lock_policy_type::mutex_type;
    using read_lock_type         = typename lock_policy_type::read_lock_type;
    using write_lock_type        = typename lock_policy_type::write_lock_type;

    // strategy tag - read by concurrency_strategy_traits
    using concurrency_strategy_tag = locked_strategy_tag;

    // axis re-export (inherited verbatim from the wrapped array)
    D_STATIC_CONSTEXPR size_type         extent      = array_type::extent;
    D_STATIC_CONSTEXPR array_lifetime    lifetime    = array_type::lifetime;
    D_STATIC_CONSTEXPR array_iterability iterability = array_type::iterability;
    D_STATIC_CONSTEXPR bool              iterable    = array_type::iterable;

    // -------------------------------------------------------------
    // c. Construction / Assignment
    // -------------------------------------------------------------
    threadsafe_array() = default;

    template<typename... Args>
    explicit threadsafe_array(
        Args&&... _args
    )
        : m_data(std::forward<Args>(_args)...)
    {}

    // copy: lock the source for the duration of the copy
    threadsafe_array(
        const threadsafe_array& _other
    )
    {
        read_lock_type guard(_other.base_type::mutex());

        m_data = _other.m_data;
    }

    threadsafe_array&
    operator=(
        const threadsafe_array& _other
    )
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

            write_lock_type g1(first->base_type::mutex());
            write_lock_type g2(second->base_type::mutex());

            m_data = _other.m_data;
            m_state.increment_version();
        }

        return *this;
    }

    // move: not synchronizable in a portable way.  Disabled.
    threadsafe_array(threadsafe_array&&)            = delete;
    threadsafe_array& operator=(threadsafe_array&&) = delete;

    ~threadsafe_array() = default;

    // -------------------------------------------------------------
    // d. Lock-Free Queries
    // -------------------------------------------------------------
    //   `size_lockfree` / `empty_lockfree` read the
    // atomic_state's dynamic size counter without
    // taking the mutex.  For static-extent containers
    // the counter only advances when `assign()` runs, so
    // it reports 0 for a default-constructed instance.
    //
    //   The unsuffixed `size` / `empty` siblings below
    // take the mutex and report the underlying array's
    // actual size (= the static extent N for any
    // fixed-extent instantiation), which is what most
    // callers want when they ask "how big is this
    // array?".  Tests rely on both APIs and the two
    // returning different values for a default-
    // constructed instance.
    size_type
    size_lockfree() const noexcept
    {
        return m_state.load_size();
    }

    re_std::uint64_t
    version() const noexcept
    {
        return m_state.load_version();
    }

    bool
    empty_lockfree() const noexcept
    {
        return ( m_state.load_size() == 0 );
    }

    // size
    //   accessor: returns the underlying array's size (= static `extent` for
    // fixed-extent instantiations). Takes a read lock, mirroring
    // cow_array::size() and the rest of the framework's "locked" accessors.
    size_type
    size() const
    {
        read_lock_type guard(base_type::mutex());

        return m_data.size();
    }

    // empty
    //   accessor: true iff `size() == 0`. Locked, for the same reason as
    // `size()`.
    bool
    empty() const
    {
        read_lock_type guard(base_type::mutex());

        return ( m_data.size() == 0 );
    }

    // -------------------------------------------------------------
    // e. Handle-Based Access
    // -------------------------------------------------------------
    const_locked_ref<array_type, lock_policy_type>
    read_access() const
    {
        return const_locked_ref<array_type, lock_policy_type>(
            m_data, base_type::mutex());
    }

    locked_ref<array_type, lock_policy_type>
    write_access()
    {
        return locked_ref<array_type, lock_policy_type>(
            m_data, base_type::mutex());
    }

    batch_guard<lock_policy_type>
    batch()
    {
        return batch_guard<lock_policy_type>(base_type::mutex());
    }

    // -------------------------------------------------------------
    // f. Element Access (locked)
    // -------------------------------------------------------------
    value_type
    at(size_type _i) const
    {
        read_lock_type guard(base_type::mutex());

        return m_data[_i];
    }

    void
    set(
        size_type         _i,
        const value_type& _v
    )
    {
        write_lock_type guard(base_type::mutex());

        m_data[_i] = _v;
        m_state.increment_version();
    }

    // -------------------------------------------------------------
    // g. Bulk Operations (locked)
    // -------------------------------------------------------------
    void assign(
        const array_type& _src
    )
    {
        write_lock_type guard(base_type::mutex());

        m_data = _src;
        m_state.store_size(_src.size());
        m_state.increment_version();
    }

    template<typename Fn>
    auto apply(
        Fn&& _fn
    )
        -> decltype(_fn(std::declval<array_type&>()))
    {
        write_lock_type guard(base_type::mutex());

        m_state.increment_version();

        return std::forward<Fn>(_fn)(m_data);
    }

    template<typename Fn>
    auto apply_read(
        Fn&& _fn
    ) const
        -> decltype(_fn(std::declval<const array_type&>()))
    {
        read_lock_type guard(base_type::mutex());

        return std::forward<Fn>(_fn)(m_data);
    }

    // -------------------------------------------------------------
    // h. Optimistic Read
    // -------------------------------------------------------------
    template<typename Fn>
    auto optimistic(
        Fn&&    _fn,
        unsigned _max_retries = 3
    ) const
        -> decltype(_fn(std::declval<const array_type&>()))
    {
        return optimistic_read<array_type, lock_policy_type>(
            m_state,
            m_data,
            base_type::mutex(),
            std::forward<Fn>(_fn),
            _max_retries);
    }

    // -------------------------------------------------------------
    // i. Snapshot
    // -------------------------------------------------------------
    snapshot_view<array_type, lock_policy_type>
    snapshot() const
    {
        return snapshot_view<array_type, lock_policy_type>(
            m_data, base_type::mutex());
    }

private:
    array_type   m_data;
    atomic_state m_state;
};


// =============================================================================
// III. Convenience Aliases
// =============================================================================
// Each alias appends a lock-policy option to the user's pack. A pack that
// already carries one would hold the key twice, which option_set rejects at
// compile time ("all keys must be unique"); for a policy of your own, name
// threadsafe_array<..., container_opt_lock_policy<P>> directly.

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// mutex_array
//   alias: threadsafe_array with exclusive_lock_policy.
template<typename... Options>
using mutex_array = threadsafe_array<
    Options...,
    container_opt_lock_policy<exclusive_lock_policy>>;

// timed_array
//   alias: threadsafe_array with timed_lock_policy.
template<typename... Options>
using timed_array = threadsafe_array<
    Options...,
    container_opt_lock_policy<timed_lock_policy>>;

#endif  // C++11

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// shared_array
//   alias: threadsafe_array with shared_lock_policy.
template<typename... Options>
using shared_array = threadsafe_array<
    Options...,
    container_opt_lock_policy<shared_lock_policy>>;

#endif  // C++17


// =============================================================================
// IV.  Trait Specializations (axis preservation)
// =============================================================================
// Delegate every structural classification to the wrapped array.
// This is what guarantees axes 1-7 are preserved across the
// concurrency wrapping.  Specializations live directly in
// `djinterp::` because that's where the primaries live (see
// array_traits.hpp); NS_TRAITS wrapping would be wrong.

template<typename... Options>
struct is_contiguous_array<threadsafe_array<Options...>>
    : is_contiguous_array<array<Options...>>
{};

template<typename... Options>
struct is_iterable_array<threadsafe_array<Options...>>
    : is_iterable_array<array<Options...>>
{};

template<typename... Options>
struct has_static_extent<threadsafe_array<Options...>>
    : has_static_extent<array<Options...>>
{};

template<typename... Options>
struct array_lifetime_of<threadsafe_array<Options...>>
    : array_lifetime_of<array<Options...>>
{};


// =============================================================================
// V.   Static Verification (axes 1-7 unchanged, axis 8 differs)
// =============================================================================

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

NS_INTERNAL

    // Use distinct alias names from the cow_array internal
    // verification block so that both headers can be included in the same
    // translation unit without redefinition ambiguity. (Identical `using`
    // aliases are technically permitted, but distinct names make the intent
    // obvious.)
    using ts_base = array<array_opt_type<int>, array_opt_extent<16>>;
    using ts_test = threadsafe_array<array_opt_type<int>,
                                     array_opt_extent<16>>;

    // axes 1-7: must match
    static_assert(
        is_contiguous_array_v<ts_test> ==
        is_contiguous_array_v<ts_base>,
        "threadsafe_array drifted on contiguity");

    static_assert(
        is_iterable_array_v<ts_test> ==
        is_iterable_array_v<ts_base>,
        "threadsafe_array drifted on iterability");

    static_assert(
        has_static_extent_v<ts_test> ==
        has_static_extent_v<ts_base>,
        "threadsafe_array drifted on extent class");

    static_assert(
        array_lifetime_of<ts_test>::value ==
        array_lifetime_of<ts_base>::value,
        "threadsafe_array drifted on lifetime");

    // axis 8: must differ
    static_assert(
        is_locked_container_v<ts_test>,
        "threadsafe_array failed to register as locked");

    static_assert(
        !is_locked_container_v<ts_base>,
        "plain array misclassified as locked");

NS_END  // internal
NS_END  // djinterp


#endif  // C++14

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_ARRAY_THREADSAFE_ARRAY_HPP
