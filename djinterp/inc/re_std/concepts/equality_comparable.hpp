/*******************************************************************************
* djinterp [re_std]                                      equality_comparable.hpp
*
* equality_comparable concept header:
*   Type supports == and != consistently.
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
* immediate, localised compile error.  Test RE_STD_LANG_IS_CPP20_OR_HIGHER, or
* use the trait-shaped equivalents in re_std::type_traits, which reach C++98.
*
*
* path:      /inc/re_std/concepts/equality_comparable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CONCEPTS_EQUALITY_COMPARABLE_HPP
#define RE_STD_CONCEPTS_EQUALITY_COMPARABLE_HPP 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "boolean_testable.hpp"

namespace re_std
{

namespace internal
{

    // weakly_equality_comparable_with
    //   concept: == and != are valid in both operand orders and both yield
    // something usable as a condition.
    template<typename TypeA, typename TypeB>
    concept weakly_equality_comparable_with
        = requires(const typename remove_reference<TypeA>::type& a,
                   const typename remove_reference<TypeB>::type& b)
          {
              { a == b } -> boolean_testable;
              { a != b } -> boolean_testable;
              { b == a } -> boolean_testable;
              { b != a } -> boolean_testable;
          };

}  // internal

// equality_comparable
//   concept: Type is comparable with itself using == and !=.
template<typename Type>
concept equality_comparable
    = internal::weakly_equality_comparable_with<Type, Type>;

}  // re_std
#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#endif  // RE_STD_CONCEPTS_EQUALITY_COMPARABLE_HPP
