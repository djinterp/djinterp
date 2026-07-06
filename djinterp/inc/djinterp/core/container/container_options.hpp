/******************************************************************************
* djinterp [container]                                   container_options.hpp
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
* wraps an `option_set<_Options...>` and re-exports its query surface.
*
* HOW IT IS USED:
*   Container modules consume an arbitrary user pack of options by
* inheriting `options_container_base<_Options...>` and querying per-axis
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
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.05
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Per-Axis Configuration Enums
      1. container_lifetime
      2. container_ordering
      3. container_bounds
      4. container_multiplicity
      5. container_structure
      6. container_storage_kind
      7. container_thread_safety
      8. container_backing
      9. container_iterability
II.   Per-Axis Option Keys        (container_axis)
III.  Canonical-Form Option Aliases
IV.   Type-Carrying Keys          (lock_policy)
V.    options_container_base
VI.   container_axis_value         (read-or-default axis lookup)
*/

#ifndef DJINTERP_CONTAINER_OPTIONS_
#define DJINTERP_CONTAINER_OPTIONS_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp
#include "../../core/djinterp.hpp"
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
//   enum: the option key for each configurable axis.  option<> keys are NTTPs
// and option_set resolves them with ==, so all keys share this one type rather
// than being distinct empty-tag structs.  `lock_policy` names the type-carrying
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
    lock_policy
};


#if D_ENV_LANG_IS_CPP17_OR_HIGHER


// ===========================================================================
// III. Canonical-Form Option Aliases
// ===========================================================================
//   Each wraps an axis position into option<container_axis::<axis>,
// integral_constant<enum, value>> - a fully-formed, NTTP-keyed option<> ready
// to drop into a container's _Options pack.

template<container_lifetime _V>
using container_opt_lifetime = option<
    container_axis::lifetime,
    std::integral_constant<container_lifetime, _V>>;

template<container_ordering _V>
using container_opt_ordering = option<
    container_axis::ordering,
    std::integral_constant<container_ordering, _V>>;

template<container_bounds _V>
using container_opt_bounds = option<
    container_axis::bounds,
    std::integral_constant<container_bounds, _V>>;

template<container_multiplicity _V>
using container_opt_multiplicity = option<
    container_axis::multiplicity,
    std::integral_constant<container_multiplicity, _V>>;

template<container_structure _V>
using container_opt_structure = option<
    container_axis::structure,
    std::integral_constant<container_structure, _V>>;

template<container_storage_kind _V>
using container_opt_storage_kind = option<
    container_axis::storage_kind,
    std::integral_constant<container_storage_kind, _V>>;

template<container_thread_safety _V>
using container_opt_thread_safety = option<
    container_axis::thread_safety,
    std::integral_constant<container_thread_safety, _V>>;

template<container_backing _V>
using container_opt_backing = option<
    container_axis::backing,
    std::integral_constant<container_backing, _V>>;

template<container_iterability _V>
using container_opt_iterability = option<
    container_axis::iterability,
    std::integral_constant<container_iterability, _V>>;


// ===========================================================================
// IV.  Type-Carrying Keys
// ===========================================================================

// container_opt_lock_policy
//   alias: `option<container_axis::lock_policy, _Policy>` for the lock-policy
// axis.  The value position is a class, not an integral_constant.  This key
// complements `container_axis::thread_safety` (enum-valued): the enum names a
// category, the class delivers the implementation.  When both appear in the
// same pack, lock_policy is the one that pins the exact type.
template<typename _Policy>
using container_opt_lock_policy = option<container_axis::lock_policy, _Policy>;


// ===========================================================================
// V.   options_container_base
// ===========================================================================

// options_container_base
//   class: the canonical base mixin every container in the framework inherits
// to gain the options surface.  It wraps an `option_set<_Options...>` and
// re-exports its query surface:
//     ::options_type          - the aggregated option_set<>
//     ::option_count          - number of options (after expansion)
//     ::has_option_v<_Key>    - whether an axis key is configured
//     ::has_option<_Key>()    - the same, in call form
//     ::option_t<_Key>        - the option<> at a key (or lookup_not_found)
//
//   Containers consume the surface by inheriting publicly:
//
//     template<typename... _Options>
//     class my_container
//         : public options_container_base<_Options...>
//     {
//         using contract_base = options_container_base<_Options...>;
//         // ::options_type, ::option_count, ::has_option_v<>, ::option_t<>
//     };
template<typename... _Options>
class options_container_base
{
public:
    // options_type
    //   the aggregated option set (its type-level face; the value-carrying
    // face is available on option_set itself under C++20).
    using options_type = option_set<_Options...>;

    // option_count
    //   number of options after structural expansion.
    static constexpr std::size_t option_count = options_type::size;

    // has_option_v
    //   whether the axis key _Key is present in the pack.
    template<auto _Key>
    static constexpr bool has_option_v =
        option_set_contains_v<options_type, _Key>;

    // has_option
    //   the call form of has_option_v.
    template<auto _Key>
    static constexpr bool
    has_option() noexcept
    {
        return option_set_contains_v<options_type, _Key>;
    }

    // option_t
    //   the option<> bound to _Key, or lookup_not_found when absent.
    template<auto _Key>
    using option_t = option_set_find_t<options_type, _Key>;

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
    template<typename _Opt>
    struct option_enum_value
    {
        using arg0 = std::tuple_element_t<0, typename _Opt::args_type>;

        static constexpr auto value = arg0::value;
    };

    // axis_value_pick
    //   helper: yields the found option's enum value when present, else the
    // supplied default.  The absent specialization never touches _Found's args,
    // so a lookup_not_found result is harmless.
    template<bool     _Present,
             typename _Found,
             auto     _Default>
    struct axis_value_pick
    {
        static constexpr auto value = _Default;
    };

    template<typename _Found,
             auto     _Default>
    struct axis_value_pick<true, _Found, _Default>
    {
        static constexpr auto value = option_enum_value<_Found>::value;
    };

NS_END  // internal


// container_axis_value
//   trait: the enum position configured for _Axis in _Set, or _Default when
// the axis is absent.  The modern replacement for the retired
// option_list_lookup_t<list, key, default>.
//
// Usage:
//   static constexpr container_structure s =
//       container_axis_value<options_type, container_axis::structure,
//                            container_structure::flat>::value;
template<typename       _Set,
         container_axis _Axis,
         auto           _Default>
struct container_axis_value
{
    static constexpr auto value =
        internal::axis_value_pick<
            option_set_contains_v<_Set, _Axis>,
            option_set_find_t<_Set, _Axis>,
            _Default>::value;
};

// container_axis_value_v
//   value: shorthand for container_axis_value<_Set, _Axis, _Default>::value.
template<typename       _Set,
         container_axis _Axis,
         auto           _Default>
inline constexpr auto container_axis_value_v =
    container_axis_value<_Set, _Axis, _Default>::value;


#else  // pre-C++17: the option layer is unavailable (auto NTTPs / inline vars)


// options_container_base (pre-C++17 fallback)
//   class: an empty base.  The option-carrying surface needs C++17; below it a
// container still builds, simply without ::options_type and the query members.
template<typename... _Options>
class options_container_base
{
protected:
    options_container_base()  = default;
    ~options_container_base() = default;
};


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_OPTIONS_
