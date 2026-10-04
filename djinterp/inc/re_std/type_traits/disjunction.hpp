/******************************************************************************
* djinterp [re_std]                                            disjunction.hpp
*
* disjunction trait header:
*   Variadic logical OR over type traits. Inherits from the first trait
* that evaluates to true, or from the last trait if all are false. The
* short-circuiting behavior matches C++17 std::disjunction.
*
*     disjunction<>::value                                  -> false
*     disjunction<false_type>::value                         -> false
*     disjunction<false_type, false_type, false_type>::value -> false
*     disjunction<false_type, true_type, false_type>::value  -> true
*
*   PORTABILITY:
*   Requires alias templates and variadic templates (C++11+). Not
* available on C++98/03.
*
*
* path:      /inc/djinterp/re_std/type_traits/disjunction.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_DISJUNCTION_
#define DJINTERP_RE_STD_TYPE_TRAITS_DISJUNCTION_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./conditional.hpp"
#include "./false_type.hpp"


// gate: variadic + alias templates
#if ( D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES &&                               \
      D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES )


NS_RESTD


// =============================================================================
// I.   DISJUNCTION
// =============================================================================

// disjunction
//   trait: empty pack -> false_type.
template<typename... _Bn>
struct disjunction : false_type
{};

// disjunction<_B1>
//   trait: single-trait base case -- inherit from _B1.
template<typename _B1>
struct disjunction<_B1> : _B1
{};

// disjunction<_B1, _Bn...>
//   trait: recursive case -- if _B1 is true, inherit from it
// (short-circuit); otherwise recurse into the tail.
template<typename    _B1,
         typename... _Bn>
struct disjunction<_B1, _Bn...>
    : conditional<
          static_cast<bool>(_B1::value),
          _B1,
          disjunction<_Bn...>
      >::type
{};


// =============================================================================
// II.  DISJUNCTION_V (C++14+ variable template)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // disjunction_v
    //   variable: convenience for disjunction<_Bn...>::value.
    template<typename... _Bn>
    D_CONSTEXPR bool disjunction_v = disjunction<_Bn...>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // re_std


#endif  // alias templates && variadic templates


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_DISJUNCTION_
