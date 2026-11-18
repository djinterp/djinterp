/*******************************************************************************
* djinterp [cpp]                                                   rcu_array.hpp
*
* djinterp rcu_array.hpp
*
* RCU (Read-Copy-Update) protected concurrent array.
*   Composes the canonical `DArray<Options...>` inside an
* DRcuProtected, exposing reader-side critical sections backed by
* epoch tracking and deferred reclamation.
*
*   Readers enter an epoch, follow the data pointer, and exit.  No
* locks, no atomics on the read fast path beyond a single epoch
* register.  Writers copy the data, mutate the copy, atomically
* swap the published pointer, and retire the old copy for deferred
* reclamation once all in-flight readers have departed.
*
*   `DRcuArray<Options...>` follows the framework options-container
* contract: a single template parameter pack consumed by the
* wrapped `DArray<>`.  No lock policy is read from the pack - RCU
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
*   - Per-element mutation patterns -> use DThreadsafeArray or
*     DAtomicArray.
*   - Readers that need to hold references across long operations
*     -> use DCowArray (snapshots are reference-counted and survive
*     epoch transitions).
*
* ACCESS PATTERNS:
*   read(fn)     - invokes fn(const array&) under an epoch guard
*   update(fn)   - copy -> modify -> atomic publish -> retire old
*   replace(a)   - publish a new array whole -> retire old
*   snapshot()   - a copy of the current array, taken under an epoch
*                  guard, in a DImmutableSnapshot (as DCowArray's).
*                  A zero-copy snapshot would have to hold the read
*                  guard open, and DRcuProtected's guards don't nest:
*                  an inner read would end the outer one.
*
*   The wrapper is built on sync/rcu.hpp's DRcuProtected: read_lock()
* and read(guard) for readers, modify(fn) and update(value) for
* writers.
*
*   KNOWN LIMIT (2026.11.14): DRcuProtected registers one reader slot,
* which every thread's read uses.  Two threads reading at once can
* have the array reclaimed under one of them (a heap-use-after-free
* under ASan).  Until sync/rcu.hpp gives each thread its own slot,
* read from one thread at a time; writers are not limited.
*
* DEPENDENCIES:
*   array.hpp                       - wrapped container
*   container_options.hpp           - option-pack normalization
*   threadsafe.hpp                  - DRcuProtected, epoch_*
*   concurrency_strategy_traits.hpp - strategy tag types
*
*
* path:      /inc/djinterp/cpp/container/array/rcu_array.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.26
*                                                            revised: 2026.11.14
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
#include "../../djinterp.hpp"
#include "../../sync/cow.hpp"  // DImmutableSnapshot
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
class DRcuArray
{
private:
    using array_type     = DArray<Options...>;
    using protected_type = DRcuProtected<array_type>;

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
    using snapshot_type      = DImmutableSnapshot<array_type>;

    // strategy tag - read by concurrency_strategy_traits
    using concurrency_strategy_tag = rcu_strategy_tag;

    // axis re-export
    D_STATIC_CONSTEXPR size_type extent = array_type::extent;
    D_STATIC_CONSTEXPR
        DArrayLifetime    lifetime    = array_type::lifetime;
    D_STATIC_CONSTEXPR
        DArrayIterability iterability = array_type::iterability;
    D_STATIC_CONSTEXPR bool iterable  = array_type::iterable;

    // -------------------------------------------------------------
    // c. Construction
    // -------------------------------------------------------------
    // default: publishes a value-initialized array (DRcuProtected has no
    // default constructor; it always holds a published value).
    DRcuArray()
        : m_protected(array_type())
    {}

    explicit DRcuArray(const array_type& _initial)
        : m_protected(_initial)
    {}

    explicit DRcuArray(array_type&& _initial)
        : m_protected(std::move(_initial))
    {}

    // non-copyable / non-movable: contains an DRcuProtected which owns
    // reclamation infrastructure that must not move.
    DRcuArray(
        const DRcuArray&
    ) = delete;
    DRcuArray& operator=(
        const DRcuArray&
    ) = delete;

    DRcuArray(
        DRcuArray&&
    ) = delete;
    DRcuArray& operator=(
        DRcuArray&&
    ) = delete;

    ~DRcuArray() = default;

    // -------------------------------------------------------------
    // d. Read Access (epoch-guarded)
    // -------------------------------------------------------------

    // read
    //   invokes _fn(const array&) under an epoch_guard. _fn must NOT retain
    // the reference past return - the data may be retired once the epoch
    // closes. For long-lived reads, use snapshot() instead.
    template<typename Fn>
    auto read(
        Fn&& _fn
    ) const -> decltype(_fn(std::declval<const array_type&>()))
    {
        auto guard = m_protected.read_lock();

        return std::forward<Fn>(_fn)(m_protected.read(guard));
    }

    // size / at - convenience wrappers around read(). These return by
    // value; the epoch guard is held only during the brief read.
    size_type size() const
    {
        return read(
            [](const array_type& _a) -> size_type
            {
                return _a.size();
            });
    }

    value_type at(
        size_type _i
    ) const
    {
        return read(
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
    void update(
        Fn&& _fn
    )
    {
        m_protected.modify(std::forward<Fn>(_fn));
    }

    // replace
    //   wholesale replacement. Equivalent to update([&](array_type& a) { a =
    // _new_value; }) but skips the copy of the old data.
    void replace(
        const array_type& _new_value
    )
    {
        m_protected.update(_new_value);
    }

    void replace(
        array_type&& _new_value
    )
    {
        m_protected.update(std::move(_new_value));
    }

    // -------------------------------------------------------------
    // f. Snapshot
    // -------------------------------------------------------------

    // snapshot
    //   returns a copy of the current array, taken under an epoch guard and
    // held in a reference-counted DImmutableSnapshot. It survives later
    // updates and epoch transitions. Use for long-lived reads.
    snapshot_type snapshot() const
    {
        return read(
            [](const array_type& _a) -> snapshot_type
            {
                return snapshot_type(_a);
            });
    }

private:
    // mutable: a read takes an epoch guard (DRcuProtected::read_lock(),
    // non-const) without changing the published array.
    mutable protected_type m_protected;
};

// old name, until 0.9.0-rc.1
template<typename... Options>
using rcu_array = DRcuArray<Options...>;


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
struct is_contiguous_array<DRcuArray<Options...>>
    : is_contiguous_array<DArray<Options...>>
{};

template<typename... Options>
struct is_iterable_array<DRcuArray<Options...>>
    : is_iterable_array<DArray<Options...>>
{};

template<typename... Options>
struct has_static_extent<DRcuArray<Options...>>
    : has_static_extent<DArray<Options...>>
{};

template<typename... Options>
struct array_lifetime_of<DRcuArray<Options...>>
    : array_lifetime_of<DArray<Options...>>
{};

NS_END  // djinterp


// =============================================================================
// III. Static Verification
// =============================================================================

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

NS_DJINTERP namespace internal { namespace rcu_array_verify {

    using base = DArray<DArrayOptType<int>, DArrayOptExtent<16>>;
    using rcu  = DRcuArray<DArrayOptType<int>, DArrayOptExtent<16>>;

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

    // axis 8: rcu_array registers as an RCU container; array<> does not
    static_assert(
        is_rcu_container_v<rcu>,
        "rcu_array failed to register as rcu");

    static_assert(
        !is_rcu_container_v<base>,
        "plain array misclassified as rcu");

}}}  // namespace internal::rcu_array_verify

#endif  // C++14

#endif  // C++11

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_ARRAY_RCU_ARRAY_HPP
