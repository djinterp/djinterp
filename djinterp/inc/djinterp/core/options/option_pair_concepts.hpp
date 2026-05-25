/******************************************************************************
* djinterp [options]                                  option_pair_concepts.hpp
*
* Option-pair concepts:
*   C++20 concepts layered over option_pair_traits.hpp. These concepts provide
* readable constraints for individual option entries without replacing the
* existing SFINAE trait surface.
*
*   The concepts mirror the public trait surface from option_pair_traits.hpp:
*   - required key/value entry members
*   - optional metadata columns
*   - key and value type classification
*   - composite entry classifications
*   - cross-entry compatibility and superset checks
*
* 
* path:      /inc/djinterp/core/options/option_pair_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.17
******************************************************************************/

#ifndef DJINTERP_OPTION_PAIR_CONCEPTS_
#define DJINTERP_OPTION_PAIR_CONCEPTS_ 1

// djinterp
#include "../djinterp.hpp"
#include "./option_pair_traits.hpp"


NS_DJINTERP

// ===========================================================================
// I.   Required member concepts
// ===========================================================================

// option_keyed_entry
//   concept: constrains types exposing a key member.
template<typename _Type>
concept option_keyed_entry = has_key<_Type>::value;

// option_valued_entry
//   concept: constrains types exposing a value member.
template<typename _Type>
concept option_valued_entry = has_value<_Type>::value;

// option_entry_type
//   concept: constrains structural option entries with key and value.
template<typename _Type>
concept option_entry_type = is_option_entry<_Type>::value;

// canonical_option_pair_type
//   concept: constrains option_pair<_K, _V> specializations.
template<typename _Type>
concept canonical_option_pair_type = is_option_pair<_Type>::value;


// ===========================================================================
// II. Key / value classification concepts
// ===========================================================================

template<typename _Type>
concept string_key_option_entry =
    has_string_key<_Type>::value;

template<typename _Type>
concept enum_key_option_entry =
    has_enum_key<_Type>::value;

template<typename _Type>
concept scoped_enum_key_option_entry =
    has_scoped_enum_key<_Type>::value;

template<typename _Type>
concept integral_key_option_entry =
    has_integral_key<_Type>::value;

template<typename _Type>
concept comparable_key_option_entry =
    has_comparable_key<_Type>::value;

template<typename _Type>
concept equality_key_option_entry =
    has_equality_key<_Type>::value;

template<typename _Type>
concept boolean_value_option_entry =
    has_boolean_value<_Type>::value;

template<typename _Type>
concept arithmetic_value_option_entry =
    has_arithmetic_value<_Type>::value;

template<typename _Type>
concept string_value_option_entry =
    has_string_value<_Type>::value;

template<typename _Type>
concept any_value_option_entry =
    has_any_value<_Type>::value;

template<typename _Type>
concept enum_value_option_entry =
    has_enum_value<_Type>::value;

template<typename _Type>
concept pointer_value_option_entry =
    has_pointer_value<_Type>::value;

// ===========================================================================
// III.   Entry compatibility concepts
// ===========================================================================

template<typename _A,
         typename _B>
concept option_entries_share_key_type =
    options_share_key_type<_A, _B>::value;

template<typename _A,
         typename _B>
concept option_entries_share_value_type =
    options_share_value_type<_A, _B>::value;

template<typename _A,
         typename _B>
concept compatible_option_entries =
    options_are_compatible<_A, _B>::value;

NS_END  // djinterp


#endif  // DJINTERP_OPTION_PAIR_CONCEPTS_
