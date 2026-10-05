/*******************************************************************************
* djinterp [re_std]                                                      tie.hpp
*
* tie factory header:
*   Creates a tuple of lvalue references to its arguments. Used
* primarily to destructure a tuple into existing variables:
*
*     int a, b;
*     tie(a, b) = make_tuple(42, 17);
*     // now a == 42, b == 17
*
*   tie can also collaborate with the re_std::ignore object to skip
* elements:
*
*     int a, c;
*     tie(a, ignore, c) = some_3_tuple;
*     // middle element discarded
*
*   PORTABILITY:
*   Requires variadic templates and rvalue references (C++11+).
*
*
* path:      /inc/re_std/tuple/tie.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.30
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TUPLE_TIE_HPP
#define RE_STD_TUPLE_TIE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if ( RE_STD_LANG_HAS_VARIADIC_TEMPLATES &&                            \
      RE_STD_LANG_HAS_RVALUE_REFERENCES )


// re_std
#include "./tuple.hpp"


namespace re_std
{


// =============================================================================
// I.   TIE
// =============================================================================

// tie
//   function: creates a tuple<Types&...> binding lvalue references
// to its arguments. Constexpr on C++14+.
template<typename... Types>
RE_STD_CONSTEXPR
tuple<Types&...>
tie(
    Types&... _args
) RE_STD_NOEXCEPT
{
    return tuple<Types&...>(_args...);
}


}  // re_std


#endif  // variadic templates && rvalue references


#endif  // RE_STD_TUPLE_TIE_HPP
