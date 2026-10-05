/*******************************************************************************
* djinterp [core]                                                  cow_array.hpp
*
* Copy-on-write concurrent array.
*   Composes the canonical `array<Options...>` inside a cow_state,
* exposing snapshot-based read access and clone-on-write semantics.
*
*   Readers receive immutable_snapshot handles that share the
* underlying storage via reference counting.  When a writer mutates,
* the storage is cloned only if any snapshot still holds it -
* readers never see a torn state, and they never block writers.
*
*   `cow_array<Options...>` follows the framework options-container
* contract: a single template parameter pack consumed by the wrapped
* `array<>`, with the lock policy that protects the cow_state read
* from the same pack's lock-policy option (default:
* `null_lock_policy` for COW since per-snapshot isolation already
* removes most contention).
*
*
* path:      /inc/djinterp/core/container/array/cow_array.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.26
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    cow_array
      ---------
      a. Type aliases (forwarded from wrapped array)
      b. Strategy tag and trait constants
      c.    Construction
            d. Read Access
            e. Snapshot
            f. Write Access
            g. Version Query

II.   Trait Specializations (axis preservation)
      -----------------------------------------

III.  Static Verification
      -------------------
*/

#ifndef DJINTERP_CONTAINER_ARRAY_COW_ARRAY_HPP
#define DJINTERP_CONTAINER_ARRAY_COW_ARRAY_HPP 1

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
// re_std
#include "../../../../re_std/cstdint/cstdint.hpp"  // re_std::uint64_t




NS_DJINTERP

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// =============================================================================
// I.   cow_array
// =============================================================================

template<typename... Options>
class cow_array
{
private:
    using options_type = option_set<Options...>;

    using array_type = array<Options...>;

public:
    // cow_array intentionally defaults to `null_lock_policy` rather than
    // `default_lock_policy`: COW writers serialize naturally through the
    // cow_state's atomic publish step.
    using lock_policy_type = container_axis_type_t<
        options_type,
        container_axis::lock_policy,
        null_lock_policy>;

private:
    using state_type = cow_state<array_type, lock_policy_type>;

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
    using cow_state_type         = state_type;
    using snapshot_type          = immutable_snapshot<array_type>;

    // strategy tag - read by concurrency_strategy_traits
    using concurrency_strategy_tag = cow_strategy_tag;

    // axis re-export
    D_STATIC_CONSTEXPR size_type         extent      = array_type::extent;
    D_STATIC_CONSTEXPR array_lifetime    lifetime    = array_type::lifetime;
    D_STATIC_CONSTEXPR array_iterability iterability = array_type::iterability;
    D_STATIC_CONSTEXPR bool              iterable    = array_type::iterable;

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

    cow_array(const cow_array&)            = delete;
    cow_array& operator=(const cow_array&) = delete;

    cow_array(cow_array&&)            = delete;
    cow_array& operator=(cow_array&&) = delete;

    ~cow_array() = default;

    // -------------------------------------------------------------
    // d. Read Access
    // -------------------------------------------------------------
    const array_type&
    read() const
    {
        return m_state.read();
    }

    size_type
    size() const
    {
        return m_state.read().size();
    }

    bool
    empty() const
    {
        return ( m_state.read().size() == 0 );
    }

    value_type
    at(
        size_type _i
    ) const
    {
        return m_state.read()[_i];
    }

    // -------------------------------------------------------------
    // e. Snapshot
    // -------------------------------------------------------------
    snapshot_type
    snapshot() const
    {
        return m_state.snapshot();
    }

    // -------------------------------------------------------------
    // f. Write Access
    // -------------------------------------------------------------
    template<typename Fn>
    auto modify(
        Fn&& _fn
    )
        -> decltype(_fn(std::declval<array_type&>()))
    {
        return m_state.modify(std::forward<Fn>(_fn));
    }

    void replace(
        const array_type& _new_value
    )
    {
        m_state.replace(_new_value);
    }

    void replace(
        array_type&& _new_value
    )
    {
        m_state.replace(std::move(_new_value));
    }

    void set(
        size_type         _i,
        const value_type& _v
    )
    {
        m_state.modify(
            [&](array_type& _a)
            {
                _a[_i] = _v;
            }
        );
    }

    // -------------------------------------------------------------
    // g. Version Query
    // -------------------------------------------------------------
    re_std::uint64_t
    version() const noexcept
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
//   The cow_array's positional axes are inherited from the wrapped
// array<...>, so each trait specialization defers to the trait's
// value on the inner array<...>.

template<typename... Options>
struct is_contiguous_array<cow_array<Options...>>
    : is_contiguous_array<array<Options...>>
{};

template<typename... Options>
struct is_iterable_array<cow_array<Options...>>
    : is_iterable_array<array<Options...>>
{};

template<typename... Options>
struct has_static_extent<cow_array<Options...>>
    : has_static_extent<array<Options...>>
{};

template<typename... Options>
struct array_lifetime_of<cow_array<Options...>>
    : array_lifetime_of<array<Options...>>
{};


// =============================================================================
// III. Static Verification
// =============================================================================

NS_INTERNAL

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

    using cow_base = array<array_opt_type<int>, array_opt_extent<16>>;
    using cow_test = cow_array<array_opt_type<int>, array_opt_extent<16>>;

    static_assert(
        is_contiguous_array_v<cow_test> == is_contiguous_array_v<cow_base>,
        "cow_array drifted on contiguity");

    static_assert(
        is_iterable_array_v<cow_test> == is_iterable_array_v<cow_base>,
        "cow_array drifted on iterability");

    static_assert(
        has_static_extent_v<cow_test> == has_static_extent_v<cow_base>,
        "cow_array drifted on extent class");

    static_assert(
        array_lifetime_of<cow_test>::value == array_lifetime_of<cow_base>::value,
        "cow_array drifted on lifetime");

    static_assert(
        is_cow_container_v<cow_test>,
        "cow_array failed to register as cow");

    static_assert(
        !is_cow_container_v<cow_base>,
        "plain array misclassified as cow");
#endif  // C++14

NS_END  // internal

#endif  // C++11

NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_ARRAY_COW_ARRAY_HPP
