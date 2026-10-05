/*******************************************************************************
* djinterp [re_std]                                          assignable_from.hpp
*
* assignable_from concept header:
*   Lhs can be assigned from Rhs, yielding Lhs.
*
*   The lvalue-reference requirement on Lhs is what stops this concept being
* satisfied by an assignment to a temporary, which compiles for class types but
* almost never means what the caller intended.  The trailing same_as check pins
* the RESULT type: an assignment operator returning void or a proxy does not
* satisfy assignable_from, because generic code chains assignments.
*
*   C++20 ONLY - AND THAT IS NOT A GAP.
*   `concept` is a core language keyword with no builtin behind it, so unlike
* re_std's intrinsic-backed traits there is nothing to detect and nothing to
* back-port.  Below C++20 this header is EMPTY rather than degraded: a concept
* that does not exist cannot give a wrong answer, and naming one is an
* immediate, localised compile error.  Test RE_STD_LANG_IS_CPP20_OR_HIGHER, or
* use the trait-shaped equivalents in re_std::type_traits, which reach C++98.
*
*
* path:      /inc/re_std/concepts/assignable_from.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CONCEPTS_ASSIGNABLE_FROM_HPP
#define RE_STD_CONCEPTS_ASSIGNABLE_FROM_HPP 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "same_as.hpp"
#include "common_reference_with.hpp"

namespace re_std
{

// assignable_from
//   concept: Rhs can be assigned to an lvalue Lhs, yielding Lhs.
template<typename Lhs, typename Rhs>
concept assignable_from
    =  is_lvalue_reference<Lhs>::value
    && common_reference_with<
           const typename remove_reference<Lhs>::type&,
           const typename remove_reference<Rhs>::type&>
    && requires(Lhs lhs, Rhs&& rhs)
       {
           { lhs = static_cast<Rhs&&>(rhs) } -> same_as<Lhs>;
       };

}  // re_std
#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#endif  // RE_STD_CONCEPTS_ASSIGNABLE_FROM_HPP
