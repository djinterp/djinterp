/******************************************************************************
* djinterp [restd]                                             conjunction.hpp
*
* conjunction trait header:
*   Variadic logical AND over type traits. Inherits from the first trait
* that evaluates to false, or from the last trait if all are true. The
* short-circuiting behavior matches C++17 std::conjunction:
* substitution is not performed past the first false trait.
*
*     conjunction<>::value                                -> true
*     conjunction<true_type>::value                        -> true
*     conjunction<true_type, true_type, true_type>::value  -> true
*     conjunction<true_type, false_type, true_type>::value -> false
*
*   PORTABILITY:
*   Requires alias templates and variadic templates (C++11+). Not
* available on C++98/03; consumer code should be gated on the
* corresponding feature macros.
*
*
* path:      /inc/djinterp/restd/type_traits/conjunction.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_CONJUNCTION_
#define DJINTERP_RESTD_TYPE_TRAITS_CONJUNCTION_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./conditional.hpp"
#include "./true_type.hpp"


// gate: variadic + alias templates
#if ( D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES &&                               \
      D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES )


NS_RESTD


// =============================================================================
// I.   CONJUNCTION
// =============================================================================

// conjunction
//   trait: empty pack -> true_type.
template<typename... _Bn>
struct conjunction : true_type
{};

// conjunction<_B1>
//   trait: single-trait base case -- inherit from _B1.
template<typename _B1>
struct conjunction<_B1> : _B1
{};

// conjunction<_B1, _Bn...>
//   trait: recursive case -- if _B1 is false, inherit from it
// (short-circuit); otherwise recurse into the tail. Substitution into
// the tail is suppressed when _B1 is false because conditional selects
// _B1 directly.
template<typename    _B1,
         typename... _Bn>
struct conjunction<_B1, _Bn...>
    : conditional<
          static_cast<bool>(_B1::value),
          conjunction<_Bn...>,
          _B1
      >::type
{};


// =============================================================================
// II.  CONJUNCTION_V (C++14+ variable template)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // conjunction_v
    //   variable: convenience for conjunction<_Bn...>::value.
    template<typename... _Bn>
    D_CONSTEXPR bool conjunction_v = conjunction<_Bn...>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // restd


#endif  // alias templates && variadic templates


#endif  // DJINTERP_RESTD_TYPE_TRAITS_CONJUNCTION_
