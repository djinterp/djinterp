/******************************************************************************
* re_std [concepts]                                        boolean_testable.hpp
*
*   internal: _Type is usable in a boolean context, negation included.
*
*   Exposition-only in std, where it is spelled boolean-testable; re_std puts
* it in internal:: because it has no standard name a user may rely on.
*   The negation clause is the whole point.  A type that converts to bool but
* whose operator! returns something odd - a proxy, or a type that does not
* itself convert to bool - would pass a naive convertible_to<bool> check and then
* break inside any algorithm that writes `if (!pred(x))`.  Requiring that !b is
* ALSO boolean-testable closes that.
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
* path:      /inc/djinterp/re_std/concepts/boolean_testable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_CONCEPTS_BOOLEAN_TESTABLE_
#define DJINTERP_RE_STD_CONCEPTS_BOOLEAN_TESTABLE_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "../concepts/convertible_to.hpp"

NS_RESTD

NS_INTERNAL

    // boolean_testable_impl
    //   concept: bare convertibility to bool.
    template<typename _Type>
    concept boolean_testable_impl = convertible_to<_Type, bool>;

    // boolean_testable
    //   concept: usable as a condition, and so is its negation.
    template<typename _Type>
    concept boolean_testable
        =  boolean_testable_impl<_Type>
        && requires(_Type&& b)
           {
               { !static_cast<_Type&&>(b) } -> boolean_testable_impl;
           };

NS_END  // internal

NS_END  // re_std
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // DJINTERP_RE_STD_CONCEPTS_BOOLEAN_TESTABLE_
