/******************************************************************************
* djinterp [container]                           container_option_concepts.hpp
*
* Option-related container concepts:
*   C++20 concepts layered over container_option_traits.hpp. These concepts
* provide readable constraints for option-aware containers without replacing
* the existing SFINAE trait surface.
*
*   The concepts mirror the verified public trait surface from
* container_option_traits.hpp:
*   - option storage and option-entry detection
*   - configurability / option production / CLI parsing
*   - from_options factory detection
*   - configure/parse strategy wrappers
*   - shorthand concepts over container_option_class<T>
*
* 
* path:      /inc/djinterp/core/container/concepts/
*                container_option_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_CONTAINER_OPTION_CONCEPTS_
#define DJINTERP_CONTAINER_OPTION_CONCEPTS_ 1

#ifndef __cplusplus
    #error "container_option_concepts.hpp requires C++ compilation"
#endif

//djinterp
#include "../../djinterp.hpp"
#include "../traits/container_option_traits.hpp"


NS_DJINTERP

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

///////////////////////////////////////////////////////////////////////////////
// I.   option-storage concepts
///////////////////////////////////////////////////////////////////////////////

template<typename _Type>
concept option_set_container_type =
    is_option_set_container_v<_Type>;

template<typename _Type>
concept option_entry_elements_container =
    has_option_entry_elements_v<_Type>;

template<typename _Type>
concept named_option_elements_container =
    has_named_option_elements_v<_Type>;

template<typename _Type>
concept documented_option_elements_container =
    has_documented_option_elements_v<_Type>;

template<typename _Type>
concept constrained_option_elements_container =
    has_constrained_option_elements_v<_Type>;

template<typename _Type>
concept env_mapped_option_elements_container =
    has_env_mapped_option_elements_v<_Type>;

template<typename _Type>
concept option_storage_container =
    option_set_container_type<_Type> ||
    option_entry_elements_container<_Type>;

///////////////////////////////////////////////////////////////////////////////
// II.  configuration / production concepts
///////////////////////////////////////////////////////////////////////////////

template<typename _Type>
concept configurable_option_container =
    is_configurable_container_v<_Type>;

template<typename _Type>
concept option_producing_container =
    is_option_producing_container_v<_Type>;

template<typename _Type>
concept cli_capable_option_container =
    is_cli_capable_container_v<_Type>;

template<typename _Type>
concept from_options_factory_container =
    has_from_options_factory_v<_Type>;

template<typename _Type>
concept option_round_trip_container =
    is_option_round_trip_capable_v<_Type>;

template<typename _Type>
concept option_supported_container =
    has_any_option_support_v<_Type>;

///////////////////////////////////////////////////////////////////////////////
// III. strategy-oriented concepts
///////////////////////////////////////////////////////////////////////////////

template<typename _Type>
concept native_apply_option_container =
    ( container_option_configure_strategy_v<_Type> ==
      option_configure_strategy::native_apply );

template<typename _Type>
concept native_configure_option_container =
    ( container_option_configure_strategy_v<_Type> ==
      option_configure_strategy::native_configure );

template<typename _Type>
concept per_option_configure_container =
    ( container_option_configure_strategy_v<_Type> ==
      option_configure_strategy::per_option );

template<typename _Type>
concept option_set_merge_container =
    ( container_option_configure_strategy_v<_Type> ==
      option_configure_strategy::option_set_merge );

template<typename _Type>
concept native_args_parse_option_container =
    ( container_option_parse_strategy_v<_Type> ==
      option_parse_strategy::native_args );

template<typename _Type>
concept native_single_parse_option_container =
    ( container_option_parse_strategy_v<_Type> ==
      option_parse_strategy::native_single );

template<typename _Type>
concept native_config_parse_option_container =
    ( container_option_parse_strategy_v<_Type> ==
      option_parse_strategy::native_config );

template<typename _Type>
concept generic_kv_parse_option_container =
    ( container_option_parse_strategy_v<_Type> ==
      option_parse_strategy::generic_kv );

///////////////////////////////////////////////////////////////////////////////
// IV.  classification-based shorthand concepts
///////////////////////////////////////////////////////////////////////////////

template<typename _Type>
concept classified_option_container =
    container_option_class<_Type>::has_option_support;

template<typename _Type>
concept classified_cli_option_container =
    container_option_class<_Type>::is_cli_capable;

template<typename _Type>
concept classified_round_trip_option_container =
    container_option_class<_Type>::is_round_trip;

template<typename _Type>
concept richly_described_option_container =
    container_option_class<_Type>::has_option_entries &&
    ( container_option_class<_Type>::has_named_entries ||
      container_option_class<_Type>::has_documented_entries ||
      container_option_class<_Type>::has_constrained_entries );

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_OPTION_CONCEPTS_