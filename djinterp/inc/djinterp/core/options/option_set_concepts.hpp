/******************************************************************************
* djinterp [options]                                   option_set_concepts.hpp
*
* Option-set concepts:
*   C++20 concepts layered over option_set_traits.hpp. These concepts provide
* readable constraints for option_set-like containers and option-configurable
* target types without replacing the existing SFINAE trait surface.
*
*   The concepts mirror the public trait surface from option_set_traits.hpp:
*   - option-set container protocol
*   - value homogeneity and mapped-type classification
*   - target-level option import / export / parse / default support
*   - configure strategy and constructibility
*   - static_option_set class identity and structural compile-time keys
*   - static-options constructibility and factory/apply detection
*
* 
* path:      /inc/djinterp/core/options/option_set_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.17
******************************************************************************/

#ifndef DJINTERP_OPTION_SET_CONCEPTS_
#define DJINTERP_OPTION_SET_CONCEPTS_ 1

// djinterp
#include "../djinterp.hpp"
#include "./option_set_traits.hpp"


NS_DJINTERP

// ===========================================================================
// I.   Container protocol concepts
// ===========================================================================

template<typename _Type>
concept option_set_key_typed = has_key_type_alias<_Type>::value;

template<typename _Type>
concept option_set_mapped_typed = has_mapped_type_alias<_Type>::value;

template<typename _Type>
concept option_set_value_typed = has_value_type_alias<_Type>::value;

template<typename _Type>
concept extent_option_set = has_extent<_Type>::value;

template<typename _Type>
concept findable_option_set = has_find_method<_Type>::value;

template<typename _Type>
concept contains_option_set = has_contains_method<_Type>::value;

template<typename _Type>
concept value_or_option_set = has_value_or_method<_Type>::value;

template<typename _Type>
concept sized_option_set = has_size_accessor<_Type>::value;

template<typename _Type>
concept empty_query_option_set = has_empty_method<_Type>::value;

template<typename _Type>
concept iterable_option_set_surface =
    ( has_begin_method<_Type>::value &&
      has_end_method<_Type>::value );

template<typename _Type>
concept backing_array_option_set = has_backing_array<_Type>::value;

template<typename _Type>
concept count_if_option_set = has_count_if_method<_Type>::value;

template<typename _Type>
concept data_access_option_set = has_data_method<_Type>::value;


// ===========================================================================
// II.  Composite option-set concepts
// ===========================================================================

template<typename _Type>
concept option_set_type = is_option_set<_Type>::value;

template<typename _Type>
concept iterable_option_set_type = is_iterable_option_set<_Type>::value;

template<typename _Type>
concept searchable_option_set_type = is_searchable_option_set<_Type>::value;

template<typename _Type>
concept static_option_set_type = is_static_option_set<_Type>::value;

template<typename _Type>
concept dynamic_option_set_type = is_dynamic_option_set<_Type>::value;

template<typename _Type>
concept sorted_option_set_type = is_sorted_option_set<_Type>::value;

// ===========================================================================
// III. Value homogeneity concepts
// ===========================================================================

template<typename _Type>
concept homogeneous_value_option_set = has_homogeneous_values<_Type>::value;

template<typename _Type>
concept heterogeneous_value_option_set = has_heterogeneous_values<_Type>::value;

template<typename _Type,
         typename _Value>
concept mapped_type_option_set = has_mapped_type_of<_Type, _Value>::value;

template<typename _Type>
concept arithmetic_value_option_set = all_values_are_arithmetic<_Type>::value;

template<typename _Type>
concept string_value_option_set = all_values_are_string<_Type>::value;

template<typename _Type>
concept boolean_value_option_set = all_values_are_boolean<_Type>::value;

template<typename _Type>
concept enum_value_option_set = all_values_are_enum<_Type>::value;

template<typename _Type>
concept integral_value_option_set = all_values_are_integral<_Type>::value;

template<typename _Type>
concept floating_value_option_set = all_values_are_floating<_Type>::value;


// ===========================================================================
// IV.  Target capability concepts
// ===========================================================================

template<typename _Type>
concept options_producing_target =
    is_option_producing<_Type>::value;

template<typename _Type>
concept option_parseable_target =
    is_option_parseable<_Type>::value;

template<typename _Type>
concept option_describable_target =
    is_option_describable<_Type>::value;

template<typename _Type>
concept static_default_options_target =
    has_static_default_options<_Type>::value;

template<typename _Type>
concept runtime_default_options_target =
    has_runtime_default_options<_Type>::value;

template<typename _Type>
concept constexpr_default_options_target =
    has_constexpr_default_options<_Type>::value;

template<typename _Type>
concept default_options_target =
    has_default_options<_Type>::value;

template<typename _Type,
         typename _OptionSet>
concept options_configurable_target =
    is_options_configurable<_Type, _OptionSet>::value;

template<typename _Type,
         typename _OptionSet>
concept native_apply_options_target =
    ( option_configure_strategy<_Type, _OptionSet>::value ==
      configure_strategy::native_apply );

template<typename _Type,
         typename _OptionSet>
concept native_configure_options_target =
    ( option_configure_strategy<_Type, _OptionSet>::value ==
      configure_strategy::native_configure );

template<typename _Type,
         typename _OptionSet>
concept per_option_configurable_target =
    ( option_configure_strategy<_Type, _OptionSet>::value ==
      configure_strategy::per_option );

template<typename _Type,
         typename _OptionSet>
concept unsupported_options_configure_target =
    ( option_configure_strategy<_Type, _OptionSet>::value ==
      configure_strategy::unsupported );


// ===========================================================================
// V.   Constructibility concepts
// ===========================================================================

template<typename _Type,
         typename _Key,
         typename _Value>
concept option_set_constructible_target =
    is_option_set_constructible<_Type, _Key, _Value>::value;

template<typename _Type,
         typename _Key,
         typename _Value>
concept option_list_constructible_target =
    is_option_list_constructible<_Type, _Key, _Value>::value;

template<typename _Type,
         typename _Key,
         typename _Value>
concept options_constructible_target =
    is_options_constructible<_Type, _Key, _Value>::value;


// ===========================================================================
// VI.  Static option set class concepts
// ===========================================================================
//   Mirror section IX of option_set_traits.hpp.

// static_option_set_class_type
//   concept: constrains `static_option_set<E...>` class
// instantiations.  Identity check, not structural — for the
// structural form see compile_time_keyed_option_set below.
template<typename _Type>
concept static_option_set_class_type =
    is_static_option_set_class<_Type>::value;

// compile_time_keyed_option_set
//   concept: constrains types exposing the compile-time
// type-keyed lookup surface (`template<K> get<K>()` plus
// `template<K> contains<K>::value`).  Includes both the canonical
// `static_option_set<E...>` and any user-defined type that mimics
// the surface.
template<typename _Type>
concept compile_time_keyed_option_set =
    has_compile_time_keys<_Type>::value;

// any_option_set_type
//   concept: aggregate — constrains types that are either dynamic
// option_set-like, a static_option_set class instance, or a
// compile-time-keyed structural equivalent.
template<typename _Type>
concept any_option_set_type =
    is_any_option_set<_Type>::value;


// ===========================================================================
// VII. Static-options constructibility concepts
// ===========================================================================
//   Mirror section X of option_set_traits.hpp.

template<typename _Type,
         typename _StaticSet>
concept static_option_set_constructible_target =
    is_static_option_set_constructible<_Type, _StaticSet>::value;

template<typename _Type>
concept any_static_option_set_constructible_target =
    is_any_static_option_set_constructible<_Type>::value;

template<typename _Type,
         typename _StaticSet>
concept from_static_options_factory_target =
    has_from_static_options_factory<_Type, _StaticSet>::value;

template<typename _Type,
         typename _StaticSet>
concept apply_static_options_target =
    has_apply_static_options_method<_Type, _StaticSet>::value;

template<typename _StaticSet>
concept static_default_constructible_set =
    is_static_default_constructible<_StaticSet>::value;

template<typename _StaticSet>
concept verified_entries_set =
    has_verifier_entries<_StaticSet>::value;

template<typename _Type,
         typename _Key,
         typename _Value,
         typename _StaticSet>
concept any_options_constructible_target =
    is_options_constructible_any<
        _Type, _Key, _Value, _StaticSet>::value;


NS_END  // djinterp


#endif  // DJINTERP_OPTION_SET_CONCEPTS_