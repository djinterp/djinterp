/******************************************************************************
* re_std [concepts]                                         assignable_from.hpp
*
*   _Lhs can be assigned from _Rhs, yielding _Lhs.
*
*   The lvalue-reference requirement on _Lhs is what stops this concept being
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
* immediate, localised compile error.  Test D_ENV_LANG_IS_CPP20_OR_HIGHER, or
* use the trait-shaped equivalents in re_std::type_traits, which reach C++98.
*
*
* path:      /inc/djinterp/re_std/concepts/assignable_from.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_CONCEPTS_ASSIGNABLE_FROM_
#define DJINTERP_RE_STD_CONCEPTS_ASSIGNABLE_FROM_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "../concepts/same_as.hpp"
#include "../concepts/common_reference_with.hpp"

NS_RESTD

// assignable_from
//   concept: _Rhs can be assigned to an lvalue _Lhs, yielding _Lhs.
template<typename _Lhs, typename _Rhs>
concept assignable_from
    =  is_lvalue_reference<_Lhs>::value
    && common_reference_with<
           const typename remove_reference<_Lhs>::type&,
           const typename remove_reference<_Rhs>::type&>
    && requires(_Lhs lhs, _Rhs&& rhs)
       {
           { lhs = static_cast<_Rhs&&>(rhs) } -> same_as<_Lhs>;
       };

NS_END  // re_std
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // DJINTERP_RE_STD_CONCEPTS_ASSIGNABLE_FROM_
