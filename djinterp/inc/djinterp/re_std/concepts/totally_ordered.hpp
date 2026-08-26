/******************************************************************************
* re_std [concepts]                                         totally_ordered.hpp
*
*   _Type is equality-comparable and totally ordered by <, >, <=, >=.
*
*   The four relational operators are checked in addition to equality, again
* in both operand orders.  Note this is a SYNTACTIC concept: it cannot verify
* that the ordering is actually total, only that the operators exist and return
* something usable as a condition.  A type with an inconsistent operator< will
* satisfy totally_ordered and then misbehave at run time - std has the same
* limitation and says so.
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
* path:      /inc/djinterp/re_std/concepts/totally_ordered.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_CONCEPTS_TOTALLY_ORDERED_
#define DJINTERP_RE_STD_CONCEPTS_TOTALLY_ORDERED_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "../concepts/equality_comparable.hpp"
#include "../concepts/boolean_testable.hpp"

NS_RESTD

NS_INTERNAL

    // partially_ordered_with
    //   concept: the four relational operators are valid in both orders.
    template<typename _TypeA, typename _TypeB>
    concept partially_ordered_with
        = requires(const typename remove_reference<_TypeA>::type& a,
                   const typename remove_reference<_TypeB>::type& b)
          {
              { a <  b } -> boolean_testable;
              { a >  b } -> boolean_testable;
              { a <= b } -> boolean_testable;
              { a >= b } -> boolean_testable;
              { b <  a } -> boolean_testable;
              { b >  a } -> boolean_testable;
              { b <= a } -> boolean_testable;
              { b >= a } -> boolean_testable;
          };

NS_END  // internal

// totally_ordered
//   concept: _Type is equality_comparable and relationally ordered.
template<typename _Type>
concept totally_ordered
    =  equality_comparable<_Type>
    && internal::partially_ordered_with<_Type, _Type>;

NS_END  // re_std
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // DJINTERP_RE_STD_CONCEPTS_TOTALLY_ORDERED_
