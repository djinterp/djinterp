/******************************************************************************
* djinterp [options]                                          option_traits.hpp
*
*   The structural contract for option<>: `is_option<>` + `is_option_v`.
* Nothing else.
*
*   The pre-2026.05.27 version of this header also shipped tag-driven
* args-search machinery (find_arg, option_find_arg, option_has_arg,
* the value<> tag, option_value_tag, option_from_tuple).  All of that
* has been retired.  Options now carry an NTTP key followed by an
* OPAQUE arg pack - no framework-imposed structure - and the
* schema-build pipeline (option_builder.hpp) handles slot positioning
* via partitioning rather than by per-slot tag search.
*
*   If a user wants tag-driven args (e.g. a hand-rolled
* `find_my_arg<...>`), they may define it in their own project
* against their own tags.  The framework itself imposes nothing on
* an option's args after the key.
*
*
* path:      /inc/djinterp/core/option/option_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    is_option                   (option<> specialization detection)
*/

#ifndef DJINTERP_OPTION_TRAITS_
#define DJINTERP_OPTION_TRAITS_ 1

// std
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "./option.hpp"


NS_DJINTERP


// ===========================================================================
// I.   is_option
// ===========================================================================

// is_option
//   trait: true iff _Type is some option<_Key, _Args...>
// specialization.  Catches both the unary form (option<K>)
// and the args form (option<K, A, B, ...>) via a single
// _Args... pack that may be empty.
template<typename _Type>
struct is_option : std::false_type
{};

template<auto        _Key,
         typename... _Args>
struct is_option<option<_Key, _Args...>> : std::true_type
{};

template<typename _Type>
D_CONSTEXPR_INLINE bool is_option_v = is_option<clean_t<_Type>>::value;


NS_END  // djinterp


#endif  // DJINTERP_OPTION_TRAITS_
