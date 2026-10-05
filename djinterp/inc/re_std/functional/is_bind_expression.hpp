/*******************************************************************************
* djinterp [re_std]                                       is_bind_expression.hpp
*
* is_bind_expression trait header:
* trait: detects bind-expression types.
*   Yields `true_type` when `Type` is the result of `re_std::bind`. The
* primary template is `false_type`; `re_std::bind` (when shipped) will
* specialize it for its result type. The trait is also part of the
* customisation point for user-defined binders: a user can specialize
* this trait so that their own binder's result objects are recognised
* by `bind`.
*
*
* path:      /inc/re_std/functional/is_bind_expression.hpp
* link(s):   TBA
* author(s): re_std                                          created: 2026.05.07
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_FUNCTIONAL_IS_BIND_EXPRESSION_HPP
#define RE_STD_FUNCTIONAL_IS_BIND_EXPRESSION_HPP 1

// FLOOR, FOR NOW: below C++11 this header is empty rather than an error
// (README rule 5; re_std omits rather than degrades). The owner's ruling:
// compile at every level first; port to C++98 only where something needs it.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "re_std/type_traits/type_traits.hpp"

namespace re_std
{

// is_bind_expression
//   trait: primary template; false for arbitrary types.
template<typename Type>
struct is_bind_expression : false_type
{};

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

// is_bind_expression_v (C++17+)
template<typename Type>
RE_STD_CONSTEXPR bool is_bind_expression_v = is_bind_expression<Type>::value;

#endif // RE_STD_LANG_HAS_VARIABLE_TEMPLATES

}  // re_std

#endif  // floor, for now


#endif  // RE_STD_FUNCTIONAL_IS_BIND_EXPRESSION_HPP
