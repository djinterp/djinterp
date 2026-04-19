/******************************************************************************
* djinterp [container]                                container_cli_traits.hpp
* 
* Container-aware CLI traits for the djinterp framework.
*   Bridges the container classification system with the CLI parsing
* stack to provide compile-time detection of how a container can be
* configured from command-line arguments.
*
*   The CLI pipeline has four decoupled stages:
*     1. Parse:    CLI string → parsed tokens
*                  (parser module — completely decoupled)
*     2. Resolve:  string token → enum key
*                  (key_map / option_table — string↔enum bridge)
*     3. Bind:     enum key + string value → typed option value
*                  (option_set / option_store)
*     4. Apply:    option_set → container configuration
*                  (container options module)
*
*   This header detects which stages a container supports,
* enabling the options and CLI modules to select the correct
* dispatch path without coupling the parser to the container
* or the container to the string representation.
*
*   All detection is purely structural SFINAE.
*
* DETECTED PROTOCOLS:
*   Key mapping:
*     - key_map_type alias  (bidirectional string↔enum map)
*     - key_map() method    (returns the key_map instance)
*     - option_table()      (returns string_kv lookup table)
*   CLI integration:
*     - options_type alias  (names the configuration enum)
*     - option_flags        (compile-time flag set)
*     - default_options     (fallback flag set)
*     - cli_option_defs()   (CLI option definition table)
*   Full pipeline:
*     - is_cli_constructible  (can be built from CLI args)
*     - is_cli_round_trip     (can export + reimport via CLI)
*
* DEPENDENCIES:
*   container_traits.hpp         - container classification
*   container_option_traits.hpp  - option integration
*   cli_traits.hpp               - CLI structural detection
*   kv_pair.hpp                  - key_map type
*
* TABLE OF CONTENTS
* =================
* I.      Key Mapping Protocol Detection
* II.     CLI Integration Detection (via cli_traits)
* III.    Full Pipeline Detection
* IV.     CLI Strategy Classification
* V.      Convenience Predicates
* VI.     Combined Classification
*
*
* path:      /inc/container/container_cli_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.03.23
******************************************************************************/

#ifndef DJINTERP_CONTAINER_CLI_TRAITS_
#define DJINTERP_CONTAINER_CLI_TRAITS_ 1

#include <cstddef>
#include <type_traits>
#include "../djinterp.hpp"
#include "../type_traits.hpp"
#include "container_traits.hpp"
#include "container_option_traits.hpp"
#include "../../cli/cli_traits.hpp"
#include "../../options/kv_pair.hpp"


NS_DJINTERP
NS_CONTAINER
NS_TRAITS

// =============================================================================
// I.   Key Mapping Protocol Detection
// =============================================================================
// The string↔enum bridge.  A container that participates in
// CLI configuration must provide a way to translate
// human-typed strings into typed enum keys and back.
//
// Detection priority:
//   1. key_map_type alias + key_map() method:
//      the container exposes a djinterp::key_map<E, N>
//      instance enabling bidirectional string↔enum lookup.
//   2. option_table() static method:
//      the container exposes a string_kv lookup table
//      (legacy / simpler path, unidirectional).
//   3. Element-level key names:
//      the container's option entries have string key
//      members (.long_name or .short_name) paired with
//      enum keys.

// has_key_map_type
//   type trait: true if the container exposes a
// key_map_type alias naming its bidirectional
// string↔enum map type.
D_TYPE_TRAIT_TRUE(has_key_map_type,
    typename _Type::key_map_type)

    // has_key_map_method
    //   type trait: true if the container has a key_map()
    // static or member method returning the key map.
    D_TYPE_TRAIT_TRUE(has_key_map_method,
        decltype(_Type::key_map()))

    // has_option_table_method
    //   type trait: true if the container has a static
    // option_table() method returning a string_kv lookup
    // table.
    // note: delegates detection to the pattern already
    // used by cli_traits::has_option_table, but re-detects
    // independently so container_cli_traits is
    // self-contained.
    D_TYPE_TRAIT_TRUE(has_option_table_method,
        decltype(_Type::option_table()))

    // has_key_name_method
    //   type trait: true if the container has a static or
    // member key_name(enum) method that converts an enum key
    // to its string representation.
    D_TYPE_TRAIT_TRUE(has_key_name_method,
        decltype(&_Type::key_name))

    // has_key_from_string_method
    //   type trait: true if the container has a static
    // key_from_string(const char*) method that converts a
    // string to an enum key.
    D_TYPE_TRAIT_TRUE(has_key_from_string_method,
        decltype(&_Type::key_from_string))

    // has_string_to_enum_bridge
    //   type trait: true if the container exposes any
    // mechanism for translating strings to enum keys.
    template<typename _Type>
struct has_string_to_enum_bridge
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        (has_key_map_type_v<clean_type> ||
            has_key_map_method_v<clean_type> ||
            has_option_table_method_v<clean_type> ||
            has_key_from_string_method_v<clean_type>);
};

template<typename _Type>
inline constexpr bool has_string_to_enum_bridge_v =
has_string_to_enum_bridge<_Type>::value;

// has_enum_to_string_bridge
//   type trait: true if the container exposes any
// mechanism for translating enum keys to strings.
template<typename _Type>
struct has_enum_to_string_bridge
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        (has_key_map_type_v<clean_type> ||
            has_key_map_method_v<clean_type> ||
            has_key_name_method_v<clean_type>);
};

template<typename _Type>
inline constexpr bool has_enum_to_string_bridge_v =
has_enum_to_string_bridge<_Type>::value;

// has_bidirectional_key_bridge
//   type trait: true if the container supports both
// string→enum and enum→string translation.
template<typename _Type>
struct has_bidirectional_key_bridge
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        (has_string_to_enum_bridge_v<clean_type> &&
            has_enum_to_string_bridge_v<clean_type>);
};

template<typename _Type>
inline constexpr bool
has_bidirectional_key_bridge_v =
has_bidirectional_key_bridge<_Type>::value;


// =============================================================================
// II.  CLI Integration Detection (via cli_traits)
// =============================================================================
// Detects whether the container exposes the structural
// members required by the CLI parser infrastructure.
// These delegate to the existing cli::traits detectors,
// applied to the container type.

// has_options_type_alias
//   type trait: true if container exposes an options_type
// alias naming its configuration enum.
// Delegates to cli::traits::has_options_type.
template<typename _Type>
struct has_options_type_alias
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        cli::traits::has_options_type<
        clean_type>::value;
};

template<typename _Type>
inline constexpr bool has_options_type_alias_v =
has_options_type_alias<_Type>::value;

// has_option_flags_field
//   type trait: true if container has a static constexpr
// option_flags member.
template<typename _Type>
struct has_option_flags_field
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        cli::traits::has_option_flags<
        clean_type>::value;
};

template<typename _Type>
inline constexpr bool has_option_flags_field_v =
has_option_flags_field<_Type>::value;

// has_default_options_field
//   type trait: true if container has a static constexpr
// default_options member.
template<typename _Type>
struct has_default_options_field
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        cli::traits::has_default_options<
        clean_type>::value;
};

template<typename _Type>
inline constexpr bool has_default_options_field_v =
has_default_options_field<_Type>::value;

// has_cli_option_defs_method
//   type trait: true if container has a static
// cli_option_defs() method returning a
// cli_option_table.
template<typename _Type>
struct has_cli_option_defs_method
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        cli::traits::has_cli_option_defs<
        clean_type>::value;
};

template<typename _Type>
inline constexpr bool has_cli_option_defs_method_v =
has_cli_option_defs_method<_Type>::value;

// is_option_constructible
//   type trait: true if container is parameterized by
// compile-time option flags (options_type + option_flags).
// Delegates to cli::traits::is_option_constructible.
template<typename _Type>
struct is_option_constructible
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        cli::traits::is_option_constructible<
        clean_type>::value;
};

template<typename _Type>
inline constexpr bool is_option_constructible_v =
is_option_constructible<_Type>::value;

// is_cli_configurable
//   type trait: true if container can be fully driven from
// CLI strings (option_constructible + default_options +
// option_table).
// Delegates to cli::traits::is_cli_configurable.
template<typename _Type>
struct is_cli_configurable
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        cli::traits::is_cli_configurable<
        clean_type>::value;
};

template<typename _Type>
inline constexpr bool is_cli_configurable_v =
is_cli_configurable<_Type>::value;

// is_self_describing
//   type trait: true if container provides both a string
// table and a count of recognized flag values (enables
// help text generation).
template<typename _Type>
struct is_self_describing
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        cli::traits::is_self_describing<
        clean_type>::value;
};

template<typename _Type>
inline constexpr bool is_self_describing_v =
is_self_describing<_Type>::value;


// =============================================================================
// III. Full Pipeline Detection
// =============================================================================
// Determines whether the complete CLI→container pipeline
// is available: parse strings, resolve to enum keys, bind
// values, apply to container.

// is_cli_constructible
//   type trait: true if a container can be constructed or
// fully configured from CLI arguments.
// Requires:
//   1. String→enum bridge (resolve stage).
//   2. Configurable (apply stage).
template<typename _Type>
struct is_cli_constructible
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        (has_string_to_enum_bridge_v<clean_type> &&
            (is_configurable_container_v<clean_type> ||
                is_cli_configurable_v<clean_type> ||
                has_from_options_factory_v<clean_type>));
};

template<typename _Type>
inline constexpr bool is_cli_constructible_v =
is_cli_constructible<_Type>::value;

// is_cli_exportable
//   type trait: true if a container can export its current
// configuration in a CLI-compatible string form.
// Requires:
//   1. Enum→string bridge (reverse resolve).
//   2. Option-producing (export stage).
template<typename _Type>
struct is_cli_exportable
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        (has_enum_to_string_bridge_v<clean_type> &&
            is_option_producing_container_v<
            clean_type>);
};

template<typename _Type>
inline constexpr bool is_cli_exportable_v =
is_cli_exportable<_Type>::value;

// is_cli_round_trip
//   type trait: true if a container can be configured from
// CLI arguments and can export that configuration back to
// CLI-compatible strings.
template<typename _Type>
struct is_cli_round_trip
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        (is_cli_constructible_v<clean_type> &&
            is_cli_exportable_v<clean_type>);
};

template<typename _Type>
inline constexpr bool is_cli_round_trip_v =
is_cli_round_trip<_Type>::value;


// =============================================================================
// IV.  CLI Strategy Classification
// =============================================================================

// --- resolve strategy ---
// How to translate CLI strings into typed keys.

// DCliResolveStrategy
//   enum: compile-time CLI key resolution strategy.
enum class DCliResolveStrategy
{
    // container has key_map — bidirectional lookup
    key_map,

    // container has option_table — string_kv lookup
    option_table,

    // container has key_from_string — explicit method
    explicit_method,

    // container's option entries have string keys
    // directly (no enum, string-keyed option_set)
    direct_string,

    // no resolution path available
    unsupported
};

NS_INTERNAL

template<typename _Type>
struct cli_resolve_strategy_impl
{
    using clean_type = clean_t<_Type>;

    static constexpr DCliResolveStrategy value =
        (has_key_map_type_v<clean_type> ||
            has_key_map_method_v<clean_type>)
        ? DCliResolveStrategy::key_map

        : has_option_table_method_v<clean_type>
        ? DCliResolveStrategy::option_table

        : has_key_from_string_method_v<clean_type>
        ? DCliResolveStrategy::
        explicit_method

        : (is_option_set_container_v<clean_type> &&
            has_named_option_elements_v<
            clean_type>)
        ? DCliResolveStrategy::direct_string

        : DCliResolveStrategy::unsupported;
};

NS_END  // internal

// container_cli_resolve_strategy
//   type trait: determines the best strategy for
// resolving CLI strings into container option keys.
template<typename _Type>
struct container_cli_resolve_strategy
{
    static constexpr DCliResolveStrategy value =
        internal::cli_resolve_strategy_impl<
        _Type>::value;
};

template<typename _Type>
inline constexpr DCliResolveStrategy
container_cli_resolve_strategy_v =
container_cli_resolve_strategy<_Type>::value;

// --- configure-from-CLI strategy ---
// Full pipeline: how to go from argc/argv to configured
// container.

// DCliConfigureStrategy
//   enum: compile-time full CLI→container strategy.
enum class DCliConfigureStrategy
{
    // container is cli_configurable — use the CLI parser
    // infrastructure directly
    native_cli,

    // container has cli_option_defs — build a cli_parser
    // from the definitions and apply
    via_defs,

    // container has key_map + is configurable — parse
    // strings, resolve via key_map, apply as options
    via_key_map,

    // container has parse_args — delegate directly
    native_parse,

    // container is a string-keyed option_set — parse
    // key=value strings directly
    direct_kv,

    // no CLI configuration path available
    unsupported
};

NS_INTERNAL

template<typename _Type>
struct cli_configure_strategy_impl
{
    using clean_type = clean_t<_Type>;

    static constexpr DCliConfigureStrategy value =
        is_cli_configurable_v<clean_type>
        ? DCliConfigureStrategy::native_cli

        : has_cli_option_defs_method_v<clean_type>
        ? DCliConfigureStrategy::via_defs

        : (has_string_to_enum_bridge_v<
            clean_type> &&
            is_configurable_container_v<
            clean_type>)
        ? DCliConfigureStrategy::via_key_map

        : has_parse_args_method_v<clean_type>
        ? DCliConfigureStrategy::native_parse

        : (is_option_set_container_v<clean_type> &&
            has_parse_option_method_v<clean_type>)
        ? DCliConfigureStrategy::direct_kv

        : DCliConfigureStrategy::unsupported;
};

NS_END  // internal

// container_cli_configure_strategy
//   type trait: determines the best full-pipeline strategy
// for configuring a container from CLI arguments.
template<typename _Type>
struct container_cli_configure_strategy
{
    static constexpr DCliConfigureStrategy value =
        internal::cli_configure_strategy_impl<
        _Type>::value;
};

template<typename _Type>
inline constexpr DCliConfigureStrategy
container_cli_configure_strategy_v =
container_cli_configure_strategy<
    _Type>::value;


// =============================================================================
// V.   Convenience Predicates
// =============================================================================

// has_any_cli_support
//   type trait: true if the container participates in the
// CLI ecosystem at any level.
template<typename _Type>
struct has_any_cli_support
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        (has_string_to_enum_bridge_v<clean_type> ||
            is_option_constructible_v<clean_type> ||
            is_cli_configurable_v<clean_type> ||
            has_cli_option_defs_method_v<clean_type> ||
            is_cli_constructible_v<clean_type>);
};

template<typename _Type>
inline constexpr bool has_any_cli_support_v =
has_any_cli_support<_Type>::value;


// =============================================================================
// VI.  Combined Classification
// =============================================================================

// container_cli_class
//   struct: complete CLI classification of a container
// type.  All members are static constexpr.
template<typename _Type>
struct container_cli_class
{
    // key mapping
    static constexpr bool has_key_map =
        has_key_map_type_v<_Type>;
    static constexpr bool has_key_map_method =
        has_key_map_method_v<_Type>;
    static constexpr bool has_option_table =
        has_option_table_method_v<_Type>;
    static constexpr bool has_key_name =
        has_key_name_method_v<_Type>;
    static constexpr bool has_key_from_string =
        has_key_from_string_method_v<_Type>;
    static constexpr bool has_str_to_enum =
        has_string_to_enum_bridge_v<_Type>;
    static constexpr bool has_enum_to_str =
        has_enum_to_string_bridge_v<_Type>;
    static constexpr bool has_bidirectional =
        has_bidirectional_key_bridge_v<_Type>;

    // CLI integration (from cli_traits)
    static constexpr bool has_options_type =
        has_options_type_alias_v<_Type>;
    static constexpr bool has_option_flags =
        has_option_flags_field_v<_Type>;
    static constexpr bool has_defaults =
        has_default_options_field_v<_Type>;
    static constexpr bool has_cli_defs =
        has_cli_option_defs_method_v<_Type>;
    static constexpr bool is_opt_constructible =
        is_option_constructible_v<_Type>;
    static constexpr bool is_cli_config =
        is_cli_configurable_v<_Type>;
    static constexpr bool is_self_desc =
        is_self_describing_v<_Type>;

    // full pipeline
    static constexpr bool is_constructible =
        is_cli_constructible_v<_Type>;
    static constexpr bool is_exportable =
        is_cli_exportable_v<_Type>;
    static constexpr bool is_round_trip =
        is_cli_round_trip_v<_Type>;

    // strategies
    static constexpr DCliResolveStrategy
        resolve_strategy =
        container_cli_resolve_strategy_v<_Type>;
    static constexpr DCliConfigureStrategy
        configure_strategy =
        container_cli_configure_strategy_v<
        _Type>;

    // aggregate
    static constexpr bool has_cli_support =
        has_any_cli_support_v<_Type>;
};


NS_END  // traits
NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_CLI_TRAITS_