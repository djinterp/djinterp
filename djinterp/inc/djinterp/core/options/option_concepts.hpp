/******************************************************************************
* djinterp [options]                                       option_concepts.hpp
*
* Option concepts:
*   C++20 concepts layered over option_traits.hpp. These concepts provide
* readable constraints for the option normalizer and option-list query
* machinery without replacing the existing SFINAE trait surface.
*
*   The concepts mirror the public trait surface from option_traits.hpp:
*   - structural option forms
*   - option_form classification
*   - normalized option-list queries
*   - shorthand profile concepts for accepted option-pack inputs
*
* 
* path:      /inc/djinterp/core/options/option_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.17
******************************************************************************/

#ifndef DJINTERP_OPTION_CONCEPTS_
#define DJINTERP_OPTION_CONCEPTS_ 1


// djinterp
#include "../djinterp.hpp"
#include "./option_traits.hpp"


NS_DJINTERP


// ===========================================================================
// I.   Structural form concepts
// ===========================================================================

// option_type
//   concept: constrains canonical option<_K, _V> entries.
template<typename _Type>
concept option_type =
    is_option<_Type>::value;

// option_list_type
//   concept: constrains canonical option_list<...> packs.
template<typename _Type>
concept option_list_type =
    is_option_list<_Type>::value;

// option_entry_like_type
//   concept: constrains structural option-entry-like types.
template<typename _Type>
concept option_entry_like_type =
    is_option_entry_like<_Type>::value;

// option_set_like_type
//   concept: constrains structural option-set-like runtime carriers.
template<typename _Type>
concept option_set_like_type =
    is_option_set_like<_Type>::value;

// option_container_like_type
//   concept: constrains iterable containers of option-entry-like elements.
template<typename _Type>
concept option_container_like_type =
    is_option_container_like<_Type>::value;

// option_wire_type
//   concept: constrains any recognized non-bare option wire form.
template<typename _Type>
concept option_wire_type =
    ( is_option<_Type>::value                ||
      is_option_list<_Type>::value           ||
      is_option_entry_like<_Type>::value     ||
      is_option_set_like<_Type>::value       ||
      is_option_container_like<_Type>::value );


// ===========================================================================
// II.  Form-classification concepts
// ===========================================================================

// option_list_form_type
//   concept: constrains types classified as option_list_form.
template<typename _Type>
concept option_list_form_type =
    ( classify_option_form<_Type>::value ==
      option_form::option_list_form );

// option_canonical_form_type
//   concept: constrains types classified as canonical option<_K, _V>.
template<typename _Type>
concept option_canonical_form_type =
    ( classify_option_form<_Type>::value ==
      option_form::option_canonical_form );

// option_set_form_type
//   concept: constrains types classified as option_set_form.
template<typename _Type>
concept option_set_form_type =
    ( classify_option_form<_Type>::value ==
      option_form::option_set_form );

// option_container_form_type
//   concept: constrains types classified as option_container_form.
template<typename _Type>
concept option_container_form_type =
    ( classify_option_form<_Type>::value ==
      option_form::option_container_form );

// option_entry_form_type
//   concept: constrains types classified as option_entry_form.
template<typename _Type>
concept option_entry_form_type =
    ( classify_option_form<_Type>::value ==
      option_form::option_entry_form );

// bare_key_option_form_type
//   concept: constrains types that fall through to bare_key_form.
template<typename _Type>
concept bare_key_option_form_type =
    ( classify_option_form<_Type>::value ==
      option_form::bare_key_form );

// runtime_carrier_option_form_type
//   concept: constrains runtime option carrier forms.
template<typename _Type>
concept runtime_carrier_option_form_type =
    ( option_set_form_type<_Type> ||
      option_container_form_type<_Type> );


// ===========================================================================
// III. Normalized-list query concepts
// ===========================================================================

// option_list_contains_key
//   concept: constrains normalized option lists containing _Key.
template<typename _List,
         typename _Key>
concept option_list_contains_key =
    option_list_contains<_List, _Key>::value;

// normalized_option_pack
//   concept: constrains packs whose normalized form is an option_list.
template<typename... _Options>
concept normalized_option_pack =
    is_option_list<typename normalize_options<_Options...>::type>::value;

// normalized_pack_contains_key
//   concept: constrains option packs whose normalized form contains _Key.
template<typename _Key,
         typename... _Options>
concept normalized_pack_contains_key =
    option_list_contains<
        typename normalize_options<_Options...>::type,
        _Key>::value;


NS_END  // djinterp


#endif  // DJINTERP_OPTION_CONCEPTS_