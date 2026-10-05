/*******************************************************************************
* djinterp [core]                                                  rcu_array.hpp
*
* djinterp rcu_array.hpp
*
* RCU (Read-Copy-Update) protected concurrent array.
*   Composes the canonical `array<Options...>` inside an
* rcu_protected, exposing reader-side critical sections backed by
* epoch tracking and deferred reclamation.
*
*   Readers enter an epoch, follow the data pointer, and exit.  No
* locks, no atomics on the read fast path beyond a single epoch
* register.  Writers copy the data, mutate the copy, atomically
* swap the published pointer, and retire the old copy for deferred
* reclamation once all in-flight readers have departed.
*
*   `rcu_array<Options...>` follows the framework options-container
* contract: a single template parameter pack consumed by the
* wrapped `array<>`.  No lock policy is read from the pack - RCU
* has its own internal synchronization protocol; if
* a lock-policy option appears in the pack, it is silently ignored.
*
* WHEN TO USE:
*   - Read-heavy workloads (>>50:1 read:write) where reader latency
*     must be minimal and bounded.
*   - The "swap the whole array" pattern: a periodically recomputed
*     lookup table, a feature flag set, etc.
*   - Workloads where readers operate on a snapshot of bounded
*     duration (no long-lived references across epochs).
*
* WHEN NOT TO USE:
*   - Frequent fine-grained writes (every write retires the entire
*     array; reclamation pressure dominates).
*   - Per-element mutation patterns -> use threadsafe_array or
*     atomic_array.
*   - Readers that need to hold references across long operations
*     -> use cow_array (snapshots are reference-counted and survive
*     epoch transitions).
*
* ACCESS PATTERNS:
*   read(fn)     - invokes fn(const array&) under an epoch guard
*   update(fn)   - copy -> modify -> atomic publish -> retire old
*   snapshot()   - atomic snapshot pointer (zero-copy view)
*
* DEPENDENCIES:
*   array.hpp                       - wrapped container
*   container_options.hpp           - option-pack normalization
*   threadsafe.hpp                  - rcu_protected, epoch_*
*   concurrency_strategy_traits.hpp - strategy tag types
*
*
* path:      /inc/djinterp/core/container/array/rcu_array.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.26
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    rcu_array
      ---------
      a. Type aliases (forwarded from wrapped array)
      b. Strategy tag and trait constants
      c.    Construction
            d. Read Access (epoch-guarded)
            e. Update (copy-modify-publish)
            f. Snapshot

II.   Trait Specializations (axis preservation)
      -----------------------------------------

III.  Static Verification
      -------------------
*/

#ifndef DJINTERP_CONTAINER_ARRAY_RCU_ARRAY_HPP
#define DJINTERP_CONTAINER_ARRAY_RCU_ARRAY_HPP 1

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
#include "../traits/concurrency_strategy_traits.hpp"
#include "./array.hpp"
#include "./array_options.hpp"
#include "./array_traits.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_DJINTERP


// =============================================================================
// I.   rcu_array
// =============================================================================

template<typename... Options>
class rcu_array
{
private:
    using array_type     = array<Options...>;
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
    using underlying_type    = array_type;
    using rcu_protected_type = protected_type;

    // strategy tag - read by concurrency_strategy_traits
    using concurrency_strategy_tag = rcu_strategy_tag;

    // axis re-export
    D_STATIC_CONSTEXPR size_type extent = array_type::extent;
    D_STATIC_CONSTEXPR
        array_lifetime    lifetime    = array_type::lifetime;
    D_STATIC_CONSTEXPR
        array_iterability iterability = array_type::iterability;

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

    // non-copyable / non-movable: contains an rcu_protected which owns
    // reclamation infrastructure that must not move.
    rcu_array(const rcu_array&)            = delete;
    rcu_array& operator=(const rcu_array&) = delete;

    rcu_array(rcu_array&&)            = delete;
    rcu_array& operator=(rcu_array&&) = delete;

    ~rcu_array() = default;

    // -------------------------------------------------------------
    // d. Read Access (epoch-guarded)
    // -------------------------------------------------------------

    // read
    //   invokes _fn(const array&) under an epoch_guard. _fn must NOT retain
    // the reference past return - the data may be retired once the epoch
    // closes. For long-lived reads, use snapshot() instead.
    template<typename Fn>
    auto read(Fn&& _fn) const
        -> decltype(_fn(std::declval<const array_type&>()))
    {
        return m_protected.read(std::forward<Fn>(_fn));
    }

    // size / at - convenience wrappers around read() These return by value;
    // the epoch guard is held only during the brief read.
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
    //     4. retire the old copy for deferred reclamation Concurrent readers
    // see either the old or new copy atomically - never an in-progress
    // mutation.
    template<typename Fn>
    void update(Fn&& _fn)
    {
        m_protected.update(std::forward<Fn>(_fn));
    }

    // replace
    //   wholesale replacement. Equivalent to update([&](array_type& a) { a =
    // _new_value; }) but skips the copy of the old data.
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
    //   returns a stable pointer-snapshot of the array. Typically a
    // reference-counted handle that survives epoch transitions. Use for
    // long-lived reads.
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
//   The primaries for `is_contiguous_array`, `is_iterable_array`,
// `has_static_extent`, and `array_lifetime_of` live directly in
// `djinterp::` (see array_traits.hpp), NOT in `djinterp::`.
// The specializations below match the primaries' namespace and so
// don't trigger the Microsoft-template "specialization outside
// enclosing namespace" warning - and, more importantly, actually
// specialize the right primary instead of declaring an unrelated
// template under `djinterp::` that the primaries never
// see.  (This was an inadvertent bug in earlier revisions; cf.
// atomic_array.hpp and cow_array.hpp, which already place these
// specs in `djinterp::`.)

template<typename... Options>
struct is_contiguous_array<rcu_array<Options...>>
    : is_contiguous_array<array<Options...>>
{};

template<typename... Options>
struct is_iterable_array<rcu_array<Options...>>
    : is_iterable_array<array<Options...>>
{};

template<typename... Options>
struct has_static_extent<rcu_array<Options...>>
    : has_static_extent<array<Options...>>
{};

template<typename... Options>
struct array_lifetime_of<rcu_array<Options...>>
    : array_lifetime_of<array<Options...>>
{};

NS_END  // djinterp


// =============================================================================
// III. Static Verification
// =============================================================================

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

NS_DJINTERP namespace internal { namespace rcu_array_verify {

    using base = array<array_opt_type<int>, array_opt_extent<16>>;
    using rcu  = rcu_array<array_opt_type<int>, array_opt_extent<16>>;

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

    // ------------------------------------------------------------
    //   The two checks below depend on `is_rcu_container_v`, which
    // must live in concurrency_strategy_traits.hpp alongside its
    // already-defined siblings `is_atomic_container_v`,
    // `is_locked_container_v`, and `is_cow_container_v`.  Until
    // that trait is added, the checks are guarded behind
    // `DJINTERP_HAS_IS_RCU_CONTAINER` so the rest of the static-
    // verification block continues to compile.
    //
    //   The drop-in definition (mirrors the other three traits):
    //
    //     template<typename Type, typename = void>
    //     struct is_rcu_container : std::false_type {};
    //
    //     template<typename Type>
    //     struct is_rcu_container<Type,
    //         void_t<typename Type::concurrency_strategy_tag>>
    //         : std::is_same<typename
    //               Type::concurrency_strategy_tag,
    //               rcu_strategy_tag>
    //     {};
    //
    //     #if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    //     template<typename Type>
    //     constexpr bool is_rcu_container_v =
    //         is_rcu_container<Type>::value;
    //     #endif
    //
    //   Once that lands, define DJINTERP_HAS_IS_RCU_CONTAINER (in
    // the same header, immediately after the trait) and these
    // checks reactivate.
#if defined(DJINTERP_HAS_IS_RCU_CONTAINER)
    static_assert(
        is_rcu_container_v<rcu>,
        "rcu_array failed to register as rcu");

    static_assert(
        !is_rcu_container_v<base>,
        "plain array misclassified as rcu");
#endif

}}}  // namespace internal::rcu_array_verify

#endif  // C++14

#endif  // C++11

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_ARRAY_RCU_ARRAY_HPP
