/***********************************************************************
* re_std                                                    invoke_r.hpp
*
* function: invoke with an explicit return type.
*   `invoke_r<R>(f, args...)` is `invoke(f, args...)` with the result
* implicitly converted to `R` (or discarded if `R` is `void`). Standard
* surface is C++23; re_std back-ports it on top of `re_std::invoke`.
*
*   The void overload is not `constexpr` on C++11 because C++11 forbids
* constexpr void functions; from C++14 it is `D_CONSTEXPR`-qualified.
*
*
* path:      /inc/djinterp/re_std/functional/invoke_r.hpp
* link(s):   TBA
* author(s): re_std                                      date: 2026.05.07
***********************************************************************/

#ifndef DJINTERP_RE_STD_FUNCTIONAL_INVOKE_R_
#define DJINTERP_RE_STD_FUNCTIONAL_INVOKE_R_ 1

#include "djinterp.hpp"

#if (D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES &&  \
     D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES)

#include "re_std/type_traits/type_traits.hpp"
#include "re_std/utility/forward.hpp"
#include "re_std/functional/invoke.hpp"

namespace re_std
{

// invoke_r
//   function: non-void return -- forwards through invoke and lets the
// implicit conversion to _R do the work.
template<typename _R,
         typename _F,
         typename... _Args>
D_CONSTEXPR
typename enable_if<(!is_same<_R, void>::value), _R>::type
invoke_r(
    _F&&       _f,
    _Args&&... _args
)
{
    return re_std::invoke(re_std::forward<_F>(_f),
                         re_std::forward<_Args>(_args)...);
}

// invoke_r
//   function: void return -- invokes for side effects and discards.
template<typename _R,
         typename _F,
         typename... _Args>
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
D_CONSTEXPR
#endif
typename enable_if<is_same<_R, void>::value, void>::type
invoke_r(
    _F&&       _f,
    _Args&&... _args
)
{
    static_cast<void>(re_std::invoke(re_std::forward<_F>(_f),
                                    re_std::forward<_Args>(_args)...));
}

} // namespace re_std

#endif // variadic templates + rvalue references

#endif  // DJINTERP_RE_STD_FUNCTIONAL_INVOKE_R_
