/*******************************************************************************
* djinterp [re_std]                                              conjunction.hpp
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
* path:      /inc/re_std/type_traits/conjunction.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_CONJUNCTION_HPP
#define RE_STD_TYPE_TRAITS_CONJUNCTION_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./conditional.hpp"
#include "./true_type.hpp"


// gate: variadic + alias templates
#if ( RE_STD_LANG_HAS_ALIAS_TEMPLATES &&                               \
      RE_STD_LANG_HAS_VARIADIC_TEMPLATES )


namespace re_std
{


// =============================================================================
// I.   CONJUNCTION
// =============================================================================

// conjunction
//   trait: empty pack -> true_type.
template<typename... Bn>
struct conjunction : true_type
{};

// conjunction<B1>
//   trait: single-trait base case -- inherit from B1.
template<typename B1>
struct conjunction<B1> : B1
{};

// conjunction<B1, Bn...>
//   trait: recursive case -- if B1 is false, inherit from it
// (short-circuit); otherwise recurse into the tail. Substitution into
// the tail is suppressed when B1 is false because conditional selects
// B1 directly.
template<typename    B1,
         typename... Bn>
struct conjunction<B1, Bn...>
    : conditional<
          static_cast<bool>(B1::value),
          conjunction<Bn...>,
          B1
      >::type
{};


// =============================================================================
// II.  CONJUNCTION_V (C++14+ variable template)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // conjunction_v
    //   variable: convenience for conjunction<Bn...>::value.
    template<typename... Bn>
    RE_STD_CONSTEXPR bool conjunction_v = conjunction<Bn...>::value;

#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES


}  // re_std


#endif  // alias templates && variadic templates


#endif  // RE_STD_TYPE_TRAITS_CONJUNCTION_HPP
