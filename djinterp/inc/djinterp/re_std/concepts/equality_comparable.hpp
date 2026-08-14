/******************************************************************************
* re_std [concepts]                                     equality_comparable.hpp
*
*   _Type supports == and != consistently.
*
*   Built on the internal weakly_equality_comparable_with helper so that the
* homogeneous and heterogeneous forms share one definition of what "has == and
* !=" means.  All four operand orders are required, because a type may define
* operator== as a member taking const& and leave the reversed form ill-formed
* pre-C++20.
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
* path:      /inc/djinterp/re_std/concepts/equality_comparable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_CONCEPTS_EQUALITY_COMPARABLE_
#define RESTD_CONCEPTS_EQUALITY_COMPARABLE_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "../concepts/boolean_testable.hpp"

NS_DJINTERP
NS_RESTD

NS_INTERNAL

    // weakly_equality_comparable_with
    //   concept: == and != are valid in both operand orders and both yield
    // something usable as a condition.
    template<typename _TypeA, typename _TypeB>
    concept weakly_equality_comparable_with
        = requires(const typename remove_reference<_TypeA>::type& a,
                   const typename remove_reference<_TypeB>::type& b)
          {
              { a == b } -> boolean_testable;
              { a != b } -> boolean_testable;
              { b == a } -> boolean_testable;
              { b != a } -> boolean_testable;
          };

NS_END  // internal

// equality_comparable
//   concept: _Type is comparable with itself using == and !=.
template<typename _Type>
concept equality_comparable
    = internal::weakly_equality_comparable_with<_Type, _Type>;

NS_END  // re_std
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // RESTD_CONCEPTS_EQUALITY_COMPARABLE_
