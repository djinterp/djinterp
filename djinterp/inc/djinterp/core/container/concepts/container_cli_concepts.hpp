/******************************************************************************
* djinterp [container]                              container_cli_concepts.hpp
*
* Container CLI concepts:
*   C++20 concepts layered over container_cli_traits.hpp. These concepts
* provide readable constraints for CLI-configurable containers without
* replacing the existing SFINAE trait surface.
*   The concepts mirror the verified public trait surface from
* container_cli_traits.hpp:
*   - key-map and string↔enum bridge detection
*   - CLI integration traits delegated from cli_traits
*   - full CLI construction / export / round-trip traits
*   - resolve/configure strategy wrappers
*   - shorthand concepts over container_cli_class<T>
*
* 
* path:      /inc/djinterp/core/container/concepts/container_cli_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_CONTAINER_CLI_CONCEPTS_
#define DJINTERP_CONTAINER_CLI_CONCEPTS_ 1

#ifndef __cplusplus
    #error "container_cli_concepts.hpp requires C++ compilation"
#endif

//djinterp
#include "../../djinterp.hpp"
#include "../traits/container_cli_traits.hpp"


NS_DJINTERP

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

///////////////////////////////////////////////////////////////////////////////
// I.   KEY-MAPPING / BRIDGE CONCEPTS
///////////////////////////////////////////////////////////////////////////////

// cli_key_map_typed_container
//   concept: constrains containers exposing key_map_type.
template<typename _Type>
concept cli_key_map_typed_container =
    has_key_map_type_v<_Type>;

// cli_key_map_container
//   concept: constrains containers exposing key_map().
template<typename _Type>
concept cli_key_map_container =
    has_key_map_method_v<_Type>;

// cli_option_table_container
//   concept: constrains containers exposing option_table().
template<typename _Type>
concept cli_option_table_container =
    has_option_table_method_v<_Type>;

// cli_key_name_container
//   concept: constrains containers exposing key_name(...).
template<typename _Type>
concept cli_key_name_container =
    has_key_name_method_v<_Type>;

// cli_key_from_string_container
//   concept: constrains containers exposing key_from_string(const char*).
template<typename _Type>
concept cli_key_from_string_container =
    has_key_from_string_method_v<_Type>;

// cli_string_to_enum_bridge_container
//   concept: constrains containers supporting string→enum resolution.
template<typename _Type>
concept cli_string_to_enum_bridge_container =
    has_string_to_enum_bridge_v<_Type>;

// cli_enum_to_string_bridge_container
//   concept: constrains containers supporting enum→string export.
template<typename _Type>
concept cli_enum_to_string_bridge_container =
    has_enum_to_string_bridge_v<_Type>;

// cli_bidirectional_key_bridge_container
//   concept: constrains containers supporting both bridge directions.
template<typename _Type>
concept cli_bidirectional_key_bridge_container =
    has_bidirectional_key_bridge_v<_Type>;


///////////////////////////////////////////////////////////////////////////////
// II.  CLI-INTEGRATION CONCEPTS
///////////////////////////////////////////////////////////////////////////////

// cli_options_typed_container
//   concept: constrains containers exposing options_type via cli_traits.
template<typename _Type>
concept cli_options_typed_container =
    has_options_type_alias_v<_Type>;

// cli_option_flags_container
//   concept: constrains containers exposing option_flags.
template<typename _Type>
concept cli_option_flags_container =
    has_option_flags_field_v<_Type>;

// cli_defaults_container
//   concept: constrains containers exposing default_options.
template<typename _Type>
concept cli_defaults_container =
    has_default_options_field_v<_Type>;

// cli_option_defs_container
//   concept: constrains containers exposing cli_option_defs().
template<typename _Type>
concept cli_option_defs_container =
    has_cli_option_defs_method_v<_Type>;

// cli_option_constructible_container
//   concept: constrains containers recognized as option-constructible.
template<typename _Type>
concept cli_option_constructible_container =
    is_option_constructible_v<_Type>;

// cli_configurable_container_type
//   concept: constrains containers recognized as CLI-configurable.
template<typename _Type>
concept cli_configurable_container_type =
    is_cli_configurable_v<_Type>;

// cli_self_describing_container
//   concept: constrains containers recognized as self-describing.
template<typename _Type>
concept cli_self_describing_container =
    is_self_describing_v<_Type>;


///////////////////////////////////////////////////////////////////////////////
// III. FULL-PIPELINE CONCEPTS
///////////////////////////////////////////////////////////////////////////////

// cli_constructible_container
//   concept: constrains containers constructible/configurable from CLI args.
template<typename _Type>
concept cli_constructible_container =
    is_cli_constructible_v<_Type>;

// cli_exportable_container
//   concept: constrains containers exportable to CLI-compatible strings.
template<typename _Type>
concept cli_exportable_container =
    is_cli_exportable_v<_Type>;

// cli_round_trip_container
//   concept: constrains containers supporting CLI import and export.
template<typename _Type>
concept cli_round_trip_container =
    is_cli_round_trip_v<_Type>;

// cli_supported_container
//   concept: constrains containers participating in the CLI stack at any level.
template<typename _Type>
concept cli_supported_container =
    has_any_cli_support_v<_Type>;


///////////////////////////////////////////////////////////////////////////////
// IV.  STRATEGY-ORIENTED CONCEPTS
///////////////////////////////////////////////////////////////////////////////

// key_map_resolve_cli_container
//   concept: constrains containers resolved through key_map.
template<typename _Type>
concept key_map_resolve_cli_container =
    ( container_cli_resolve_strategy_v<_Type> ==
      cli_resolve_strategy::key_map );

// option_table_resolve_cli_container
//   concept: constrains containers resolved through option_table().
template<typename _Type>
concept option_table_resolve_cli_container =
    ( container_cli_resolve_strategy_v<_Type> ==
      cli_resolve_strategy::option_table );

// explicit_method_resolve_cli_container
//   concept: constrains containers resolved through key_from_string().
template<typename _Type>
concept explicit_method_resolve_cli_container =
    ( container_cli_resolve_strategy_v<_Type> ==
      cli_resolve_strategy::explicit_method );

// native_cli_configure_container
//   concept: constrains containers configured through native cli_traits support.
template<typename _Type>
concept native_cli_configure_container =
    ( container_cli_configure_strategy_v<_Type> ==
      cli_configure_strategy::native_cli );

// defs_cli_configure_container
//   concept: constrains containers configured through cli_option_defs().
template<typename _Type>
concept defs_cli_configure_container =
    ( container_cli_configure_strategy_v<_Type> ==
      cli_configure_strategy::via_defs );

// key_map_cli_configure_container
//   concept: constrains containers configured through string→enum resolution.
template<typename _Type>
concept key_map_cli_configure_container =
    ( container_cli_configure_strategy_v<_Type> ==
      cli_configure_strategy::via_key_map );


///////////////////////////////////////////////////////////////////////////////
// V.   CLASSIFICATION-BASED SHORTHAND CONCEPTS
///////////////////////////////////////////////////////////////////////////////

// classified_cli_container
//   concept: shorthand for any type recognized by container_cli_class as
// having CLI support.
template<typename _Type>
concept classified_cli_container =
    container_cli_class<_Type>::has_cli_support;

// fully_classified_cli_container
//   concept: shorthand for containers with key bridge, CLI integration,
/// and full round-trip support.
template<typename _Type>
concept fully_classified_cli_container =
    container_cli_class<_Type>::has_bidirectional &&
    container_cli_class<_Type>::is_constructible  &&
    container_cli_class<_Type>::is_exportable     &&
    container_cli_class<_Type>::is_round_trip;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_CLI_CONCEPTS_