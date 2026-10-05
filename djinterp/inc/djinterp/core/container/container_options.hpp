/*******************************************************************************
* djinterp [core]                                          container_options.hpp
*
* Universal per-axis option keys, enums, and canonical aliases for
*   container configuration.
*
*   This header is the foundation for every container module's options
* surface.  Each universal classification axis from the framework's
* twelve-axis taxonomy that is *configurable* (rather than detection-
* only) gets:
*
*     1. an enum naming the valid positions on that axis,
*     2. an enumerator of `container_axis` that serves as the option key, and
*     3. a `container_opt_<axis>` alias that wraps a value of the enum
*        into a fully-formed `option<key, integral_constant<...>>`.
*
*   The nine axes covered here are: lifetime, ordering, bounds,
* multiplicity, structure, storage, thread_safety, backing, and
* iterability.  Detection-only axes (binary, database, text) are not
* exposed as configuration options because they are not selected by
* the user; they are observed from the container's structure.
*
*   This header also exports the `options_container_base` mixin, which
* every container in the framework inherits to gain the options surface
* (::options_type, ::option_count, ::has_option_v<>, ::option_t<>).  It
* wraps an `option_set<Options...>` and re-exports its query surface.
*
* HOW IT IS USED:
*   Container modules consume an arbitrary user pack of options by
* inheriting `options_container_base<Options...>` and querying per-axis
* keys.  A per-axis position is read with
* `container_axis_value_v<options_type, container_axis::<axis>, <default>>`,
* which yields the configured enum value or the supplied default when the
* axis is absent.  Axes that do not apply to a given container are silently
* ignored - no static_assert fires when a user passes
* `container_opt_thread_safety<...>` to a container with no lock policy hook.
*
* KEYS ARE NTTPs:
*   `option<>` keys are values (NTTPs), and `option_set` compares keys with
* `==`, so every key must share one type.  The nine axis keys are therefore
* enumerators of a single `container_axis` enum, not distinct tag structs.
* (This replaces the pre-2026.07 per-axis empty-tag keys, which predated the
* NTTP-keyed `option<>` and no longer form a valid key.)
*
* STANDARD:
*   The configuration ENUMS are available at the C++14 baseline.  The
* option-carrying layer (aliases, `options_container_base`, the value-lookup
* trait) is built on `option<>` / `option_set<>`, which require auto NTTPs
* and inline variables, so it is compiled only under C++17 and later; below
* that, `options_container_base` degrades to an empty base so a container's
* core remains buildable while its options surface is simply unavailable.
*
* SCOPE:
*   This header is intentionally CLI-agnostic.  String resolution,
* parsing, name tables, and any other human-facing translation are
* the responsibility of the (forthcoming) CLI axis.  Nothing in this
* header knows or cares that an enumerator might one day appear as
* user-typed text - the values here are pure compile-time tokens.
*
* DEPENDENCIES:
*   djinterp.hpp             - NS_*, D_ENV_* language macros
*   option/option.hpp        - option<...> for the canonical aliases (C++17+)
*   option/option_set.hpp    - option_set<...> + queries for the
*                              options_container_base surface (C++17+)
*
*
* path:      /inc/djinterp/core/container/container_options.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.05
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Per-Axis Configuration Enums
      ----------------------------
      1.    container_lifetime
      2.    container_ordering
      3.    container_bounds
      4.    container_multiplicity
      5.    container_structure
      6.    container_storage_kind
      7.    container_thread_safety
      8.    container_backing
      9.    container_iterability

II.   Per-Axis Option Keys        (container_axis)
      --------------------------------------------

III.  Canonical-Form Option Aliases
      -----------------------------

IV.   Type-Carrying Keys          (lock_policy, element_type, extent)
      ---------------------------------------------------------------

V.    options_container_base
      ----------------------

VI.   container_axis_value         (read-or-default axis lookup)
      ----------------------------------------------------------

VII.  container_axis_type          (read-or-default type-carrying lookup)
      -------------------------------------------------------------------
*/

#ifndef DJINTERP_CONTAINER_CONTAINER_OPTIONS_HPP
#define DJINTERP_CONTAINER_CONTAINER_OPTIONS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include "../option/option.hpp"       // option<>  (NTTP-keyed)
    #include "../option/option_set.hpp"   // option_set<> + option_set_contains / find
#endif


NS_DJINTERP


// ===========================================================================
// I.   Per-Axis Configuration Enums
// ===========================================================================

// container_lifetime
enum class container_lifetime
{
    constexpr_storage,
    immutable,
    mutable_storage
};


// container_ordering
enum class container_ordering
{
    unordered,
    ordered,
    sorted
};


// container_bounds
enum class container_bounds
{
    unbounded,
    bounded
};


// container_multiplicity
enum class container_multiplicity
{
    multi,
    unique
};


// container_structure
enum class container_structure
{
    flat,
    hierarchical
};


// container_storage_kind
enum class container_storage_kind
{
    static_storage,
    dynamic_storage,
    small_buffer,
    external
};


// container_thread_safety
enum class container_thread_safety
{
    none,
    atomic_only,
    exclusive,
    shared,
    timed,
    shared_timed
};


// container_backing
enum class container_backing
{
    fundamental,
    overlay
};


// container_iterability
enum class container_iterability
{
    iterable,
    non_iterable
};


// ===========================================================================
// II.  Per-Axis Option Keys
// ===========================================================================

// container_axis
//   enum: the option key for each configurable axis. option<> keys are NTTPs
// and option_set resolves them with ==, so all keys share this one type rather
// than being distinct empty-tag structs. `lock_policy` names the type-carrying
// key of Section IV.
enum class container_axis
{
    lifetime,
    ordering,
    bounds,
    multiplicity,
    structure,
    storage_kind,
    thread_safety,
    backing,
    iterability,
    lock_policy,

    // keys a container consumes only if it has the matching hook (Section
    // IV); array<> reads both, every other container ignores them
    element_type,
    extent
};


#if D_ENV_LANG_IS_CPP17_OR_HIGHER


// ===========================================================================
// III. Canonical-Form Option Aliases
// ===========================================================================
//   Each wraps an axis position into option<container_axis::<axis>,
// integral_constant<enum, value>> - a fully-formed, NTTP-keyed option<> ready
// to drop into a container's Options pack.

template<container_lifetime V>
using container_opt_lifetime = option<
    container_axis::lifetime,
    std::integral_constant<container_lifetime, V>>;

template<container_ordering V>
using container_opt_ordering = option<
    container_axis::ordering,
    std::integral_constant<container_ordering, V>>;

template<container_bounds V>
using container_opt_bounds = option<
    container_axis::bounds,
    std::integral_constant<container_bounds, V>>;

template<container_multiplicity V>
using container_opt_multiplicity = option<
    container_axis::multiplicity,
    std::integral_constant<container_multiplicity, V>>;

template<container_structure V>
using container_opt_structure = option<
    container_axis::structure,
    std::integral_constant<container_structure, V>>;

template<container_storage_kind V>
using container_opt_storage_kind = option<
    container_axis::storage_kind,
    std::integral_constant<container_storage_kind, V>>;

template<container_thread_safety V>
using container_opt_thread_safety = option<
    container_axis::thread_safety,
    std::integral_constant<container_thread_safety, V>>;

template<container_backing V>
using container_opt_backing = option<
    container_axis::backing,
    std::integral_constant<container_backing, V>>;

template<container_iterability V>
using container_opt_iterability = option<
    container_axis::iterability,
    std::integral_constant<container_iterability, V>>;


// ===========================================================================
// IV.  Type-Carrying Keys
// ===========================================================================

// container_opt_lock_policy
//   alias: `option<container_axis::lock_policy, Policy>` for the lock-policy
// axis. The value position is a class, not an integral_constant. This key
// complements `container_axis::thread_safety` (enum-valued): the enum names a
// category, the class delivers the implementation. When both appear in the
// same pack, lock_policy is the one that pins the exact type.
template<typename Policy>
using container_opt_lock_policy = option<container_axis::lock_policy, Policy>;

//   container_axis::element_type and container_axis::extent are the two keys
// of a container whose element type and capacity are options rather than
// template parameters -- array<>, whose module spells them array_type_key and
// array_extent_key and wraps them as array_opt_type<T> (the value position is
// the element type) and array_opt_extent<N> (an integral_constant<size_t, N>).
// They are container_axis enumerators, not keys of an array-local enum,
// because option_set takes one key type per set and an array's pack mixes
// them with the universal axes above.


// ===========================================================================
// V.   options_container_base
// ===========================================================================

// options_container_base
//   class: the canonical base mixin every container in the framework inherits
// to gain the options surface. It wraps an `option_set<Options...>` and
// re-exports its query surface:
//     ::options_type - the aggregated option_set<>
//     ::option_count - number of options (after expansion)
//     ::has_option_v<Key> - whether an axis key is configured
//     ::has_option<Key>() - the same, in call form
//     ::option_t<Key> - the option<> at a key (or lookup_not_found)
//
//   Containers consume the surface by inheriting publicly:
//
//     template<typename... Options>
//     class my_container
//         : public options_container_base<Options...>
//     {
//         using contract_base = options_container_base<Options...>;
//         // ::options_type, ::option_count, ::has_option_v<>, ::option_t<>
//     };
template<typename... Options>
class options_container_base
{
public:
    // options_type
    //   the aggregated option set (its type-level face; the value-carrying
    // face is available on option_set itself under C++20).
    using options_type = option_set<Options...>;

    // option_count
    //   number of options after structural expansion.
    static constexpr std::size_t option_count = options_type::size;

    // has_option_v
    //   whether the axis key Key is present in the pack.
    template<auto Key>
    static constexpr bool has_option_v =
        option_set_contains_v<options_type, Key>;

    // has_option
    //   the call form of has_option_v.
    template<auto Key>
    static constexpr bool
    has_option() noexcept
    {
        return option_set_contains_v<options_type, Key>;
    }

    // option_t
    //   the option<> bound to Key, or lookup_not_found when absent.
    template<auto Key>
    using option_t = option_set_find_t<options_type, Key>;

protected:
    options_container_base()  = default;
    ~options_container_base() = default;
};


// ===========================================================================
// VI.  container_axis_value
// ===========================================================================

NS_INTERNAL

    // option_enum_value
    //   helper: the enum position an axis option carries, read from the
    // integral_constant in its first arg slot.
    template<typename Opt>
    struct option_enum_value
    {
        using arg0 = std::tuple_element_t<0, typename Opt::args_type>;

        static constexpr auto value = arg0::value;
    };

    // axis_value_pick
    //   helper: yields the found option's enum value when present, else the
    // supplied default. The absent specialization never touches Found's args,
    // so a lookup_not_found result is harmless.
    template<bool     Present,
             typename Found,
             auto     Default>
    struct axis_value_pick
    {
        static constexpr auto value = Default;
    };

    template<typename Found,
             auto     Default>
    struct axis_value_pick<true, Found, Default>
    {
        static constexpr auto value = option_enum_value<Found>::value;
    };

NS_END  // internal


// container_axis_value
//   trait: the enum position configured for Axis in Set, or Default when
// the axis is absent. The modern replacement for the retired
// option_list_lookup_t<list, key, default>.
//
// Usage:
//   static constexpr container_structure s =
//       container_axis_value<options_type, container_axis::structure,
//                            container_structure::flat>::value;
template<typename       Set,
         container_axis Axis,
         auto           Default>
struct container_axis_value
{
    static constexpr auto value =
        internal::axis_value_pick<
            option_set_contains_v<Set, Axis>,
            option_set_find_t<Set, Axis>,
            Default>::value;
};

// container_axis_value_v
//   value: shorthand for container_axis_value<Set, Axis, Default>::value.
template<typename       Set,
         container_axis Axis,
         auto           Default>
inline constexpr auto container_axis_value_v =
    container_axis_value<Set, Axis, Default>::value;


// ===========================================================================
// VII. container_axis_type
// ===========================================================================

NS_INTERNAL

    // axis_type_pick
    //   helper: yields the type in the found option's first arg slot when
    // present, else Default. As with axis_value_pick, the absent
    // specialization never touches Found's args.
    template<bool     Present,
             typename Found,
             typename Default>
    struct axis_type_pick
    {
        using type = Default;
    };

    template<typename Found,
             typename Default>
    struct axis_type_pick<true, Found, Default>
    {
        using type = std::tuple_element_t<0, typename Found::args_type>;
    };

NS_END  // internal

// container_axis_type
//   trait: the class configured for a type-carrying Axis in Set (the type in
// its option's value position), or Default when the axis is absent. The
// type-carrying counterpart of container_axis_value, and the modern
// replacement for option_list_lookup_t on a type-valued key.
//
// Usage:
//   using policy = container_axis_type_t<options_type,
//                                        container_axis::lock_policy,
//                                        null_lock_policy>;
template<typename       Set,
         container_axis Axis,
         typename       Default>
struct container_axis_type
{
    using type = typename internal::axis_type_pick<
        option_set_contains_v<Set, Axis>,
        option_set_find_t<Set, Axis>,
        Default>::type;
};

// container_axis_type_t
//   type: shorthand for container_axis_type<Set, Axis, Default>::type.
template<typename       Set,
         container_axis Axis,
         typename       Default>
using container_axis_type_t =
    typename container_axis_type<Set, Axis, Default>::type;


#else  // pre-C++17: the option layer is unavailable (auto NTTPs / inline vars)


// options_container_base (pre-C++17 fallback)
//   class: an empty base. The option-carrying surface needs C++17; below it a
// container still builds, simply without ::options_type and the query members.
template<typename... Options>
class options_container_base
{
protected:
    options_container_base()  = default;
    ~options_container_base() = default;
};


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_CONTAINER_OPTIONS_HPP
