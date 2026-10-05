/*******************************************************************************
* djinterp [re_std]                                                 invoke_r.hpp
*
* invoke_r function header:
* function: invoke with an explicit return type.
*   `invoke_r<R>(f, args...)` is `invoke(f, args...)` with the result
* implicitly converted to `R` (or discarded if `R` is `void`). Standard
* surface is C++23; re_std back-ports it on top of `re_std::invoke`.
*
*   The void overload is not `constexpr` on C++11 because C++11 forbids
* constexpr void functions; from C++14 it is `RE_STD_CONSTEXPR`-qualified.
*
*
* path:      /inc/re_std/functional/invoke_r.hpp
* link(s):   TBA
* author(s): re_std                                          created: 2026.05.07
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_FUNCTIONAL_INVOKE_R_HPP
#define RE_STD_FUNCTIONAL_INVOKE_R_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if (RE_STD_LANG_HAS_VARIADIC_TEMPLATES &&  \
     RE_STD_LANG_HAS_RVALUE_REFERENCES)

#include "re_std/type_traits/type_traits.hpp"
#include "re_std/utility/forward.hpp"
#include "re_std/functional/invoke.hpp"

namespace re_std
{

// invoke_r
//   function: non-void return -- forwards through invoke and lets the
// implicit conversion to R do the work.
template<typename R,
         typename F,
         typename... Args>
RE_STD_CONSTEXPR
typename enable_if<(!is_same<R, void>::value), R>::type
invoke_r(
    F&&       _f,
    Args&&... _args
)
{
    return re_std::invoke(re_std::forward<F>(_f),
                         re_std::forward<Args>(_args)...);
}

// invoke_r
//   function: void return -- invokes for side effects and discards.
template<typename R,
         typename F,
         typename... Args>
#if RE_STD_LANG_IS_CPP14_OR_HIGHER
RE_STD_CONSTEXPR
#endif
typename enable_if<is_same<R, void>::value, void>::type
invoke_r(
    F&&       _f,
    Args&&... _args
)
{
    static_cast<void>(re_std::invoke(re_std::forward<F>(_f),
                                    re_std::forward<Args>(_args)...));
}

}  // re_std
#endif // variadic templates + rvalue references

#endif  // RE_STD_FUNCTIONAL_INVOKE_R_HPP
