/***********************************************************************
* re_std                                         is_bind_expression.hpp
*
* trait: detects bind-expression types.
*   Yields `true_type` when `_Type` is the result of `re_std::bind`. The
* primary template is `false_type`; `re_std::bind` (when shipped) will
* specialize it for its result type. The trait is also part of the
* customisation point for user-defined binders: a user can specialize
* this trait so that their own binder's result objects are recognised
* by `bind`.
*
*
* path:      /inc/djinterp/re_std/functional/is_bind_expression.hpp
* link(s):   TBA
* author(s): re_std                                      date: 2026.05.07
***********************************************************************/

#ifndef DJINTERP_RE_STD_FUNCTIONAL_IS_BIND_EXPRESSION_
#define DJINTERP_RE_STD_FUNCTIONAL_IS_BIND_EXPRESSION_ 1

#include "djinterp.hpp"
#include "re_std/type_traits/type_traits.hpp"

namespace re_std
{

// is_bind_expression
//   trait: primary template; false for arbitrary types.
template<typename _Type>
struct is_bind_expression : false_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// is_bind_expression_v (C++17+)
template<typename _Type>
D_CONSTEXPR bool is_bind_expression_v = is_bind_expression<_Type>::value;

#endif // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

} // namespace re_std

#endif  // DJINTERP_RE_STD_FUNCTIONAL_IS_BIND_EXPRESSION_
