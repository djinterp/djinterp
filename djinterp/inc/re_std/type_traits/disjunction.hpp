/*******************************************************************************
* djinterp [re_std]                                              disjunction.hpp
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
* path:      /inc/re_std/type_traits/disjunction.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_DISJUNCTION_HPP
#define RE_STD_TYPE_TRAITS_DISJUNCTION_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./conditional.hpp"
#include "./false_type.hpp"


// gate: variadic + alias templates
#if ( RE_STD_LANG_HAS_ALIAS_TEMPLATES &&                               \
      RE_STD_LANG_HAS_VARIADIC_TEMPLATES )


namespace re_std
{


// =============================================================================
// I.   DISJUNCTION
// =============================================================================

// disjunction
//   trait: empty pack -> false_type.
template<typename... Bn>
struct disjunction : false_type
{};

// disjunction<B1>
//   trait: single-trait base case -- inherit from B1.
template<typename B1>
struct disjunction<B1> : B1
{};

// disjunction<B1, Bn...>
//   trait: recursive case -- if B1 is true, inherit from it
// (short-circuit); otherwise recurse into the tail.
template<typename    B1,
         typename... Bn>
struct disjunction<B1, Bn...>
    : conditional<
          static_cast<bool>(B1::value),
          B1,
          disjunction<Bn...>
      >::type
{};


// =============================================================================
// II.  DISJUNCTION_V (C++14+ variable template)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // disjunction_v
    //   variable: convenience for disjunction<Bn...>::value.
    template<typename... Bn>
    RE_STD_CONSTEXPR bool disjunction_v = disjunction<Bn...>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // alias templates && variadic templates


#endif  // RE_STD_TYPE_TRAITS_DISJUNCTION_HPP
