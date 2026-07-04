/******************************************************************************
* djinterp [container]                                   container_options.hpp
*
* Universal per-axis option keys, enums, and canonical aliases for
*   container configuration.
*
*   This header is the foundation for every container module's options
* surface.  Each universal classification axis from the framework's
* twelve-axis taxonomy that is *configurable* (rather than detection-
* only) gets a triplet:
*
*     1. an enum naming the valid positions on that axis,
*     2. an empty tag struct that serves as the option_list key, and
*     3. a `container_opt_<axis>` alias that wraps a value of the enum
*        into a fully-formed `option<key, integral_constant<...>>`.
*
*   The nine axes covered here are: lifetime, ordering, bounds,
* multiplicity, structure, storage, thread_safety, backing, and
* iterability.  Detection-only axes (binary, database, text) are not
* exposed as configuration options because they are not selected by
* the user; they are observed from the container's structure.
*
*   This header also exports the `options_container_base` alias,
* which every container in the framework inherits to gain the
* with_options surface (::options_type, ::option_count,
* ::has_option_v<>, ::option_t<>).  It is a thin alias for
* `with_options_pack<_Options...>`.
*
* HOW IT IS USED:
*   Container modules consume an arbitrary user pack of options by
* normalizing it through `normalize_options_t<...>` and then querying
* per-axis keys with `option_list_lookup_t<list, key, default>`.  Each
* axis defaults to the position designated in the howto guide
* (typically the cheapest / most-permissive value).  Axes that do not
* apply to a given container are silently ignored - no static_assert
* fires when a user passes `container_opt_thread_safety<...>` to a
* container that has no lock policy hook.
*
* SCOPE:
*   This header is intentionally CLI-agnostic.  String resolution,
* parsing, name tables, and any other human-facing translation are
* the responsibility of the (forthcoming) CLI axis.  Nothing in this
* header knows or cares that an enumerator might one day appear as
* user-typed text - the values here are pure compile-time tokens.
*
* DEPENDENCIES:
*   djinterp.hpp           - NS_*
*   options/options.hpp    - option<...> for the canonical aliases
*   options/with_options.hpp - with_options_pack<...> for the
*                              options_container_base alias
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
II.   Per-Axis Option Keys
III.  Canonical-Form Option Aliases
IV.   Type-Carrying Keys
      1. lock_policy_key  /  container_opt_lock_policy
V.    options_container_base
*/

#ifndef DJINTERP_CONTAINER_OPTIONS_
#define DJINTERP_CONTAINER_OPTIONS_ 1

// std
#include <type_traits>
// djinterp
#include "../../core/djinterp.hpp"
#include "../options/option.hpp"


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

struct container_lifetime_key      {};
struct container_ordering_key      {};
struct container_bounds_key        {};
struct container_multiplicity_key  {};
struct container_structure_key     {};
struct container_storage_kind_key  {};
struct container_thread_safety_key {};
struct container_backing_key       {};
struct container_iterability_key   {};


// ===========================================================================
// III. Canonical-Form Option Aliases
// ===========================================================================

template<container_lifetime _V>
using container_opt_lifetime = option<
    container_lifetime_key,
    std::integral_constant<container_lifetime, _V>>;

template<container_ordering _V>
using container_opt_ordering = option<
    container_ordering_key,
    std::integral_constant<container_ordering, _V>>;

template<container_bounds _V>
using container_opt_bounds = option<
    container_bounds_key,
    std::integral_constant<container_bounds, _V>>;

template<container_multiplicity _V>
using container_opt_multiplicity = option<
    container_multiplicity_key,
    std::integral_constant<container_multiplicity, _V>>;

template<container_structure _V>
using container_opt_structure = option<
    container_structure_key,
    std::integral_constant<container_structure, _V>>;

template<container_storage_kind _V>
using container_opt_storage_kind = option<
    container_storage_kind_key,
    std::integral_constant<container_storage_kind, _V>>;

template<container_thread_safety _V>
using container_opt_thread_safety = option<
    container_thread_safety_key,
    std::integral_constant<container_thread_safety, _V>>;

template<container_backing _V>
using container_opt_backing = option<
    container_backing_key,
    std::integral_constant<container_backing, _V>>;

template<container_iterability _V>
using container_opt_iterability = option<
    container_iterability_key,
    std::integral_constant<container_iterability, _V>>;


// ===========================================================================
// IV.  Type-Carrying Keys
// ===========================================================================

// lock_policy_key
//   tag: option key identifying the lock policy class for
// containers that consume one (e.g. threadsafe_array, cow_array).
// The associated value type is the policy class itself.  This key
// complements `container_thread_safety_key` (enum-valued): the
// enum names a category, the class delivers the implementation.
// When both keys appear in the same pack, the lock_policy_key
// wins because it pins down the exact type.
struct lock_policy_key
{};

// container_opt_lock_policy
//   alias: `option<lock_policy_key, _Policy>` for the lock-policy
// axis.  The value position is a class, not an integral_constant.
template<typename _Policy>
using container_opt_lock_policy = option<lock_policy_key, _Policy>;


// ===========================================================================
// V.   options_container_base
// ===========================================================================

// options_container_base
//   alias: the canonical base mixin every container in the
// framework inherits to gain the with_options surface
// (`::options_type`, `::option_count`, `::has_option_v<>`,
// `::option_t<>`).
//
//   This is a thin pass-through to `with_options_pack<_Options...>`
// - the alias exists so that container modules can document
// their inheritance with a name that says "this is the
// options-container contract" rather than the more generic
// "this is the with-options mixin pack form".
//
//   Containers consume the surface by inheriting publicly:
//
//     template<typename... _Options>
//     class my_container
//         : public options_container_base<_Options...>
//     {
//         using contract_base = options_container_base<_Options...>;
//         // ::options_type, ::option_count, ::has_option_v<>,
//         // ::option_t<> now visible to clients.
//     };
template<typename... _Options>
using options_container_base = with_options_pack<_Options...>;


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_OPTIONS_
