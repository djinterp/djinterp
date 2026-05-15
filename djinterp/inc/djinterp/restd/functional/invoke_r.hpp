/***********************************************************************
* restd                                                     invoke_r.hpp
*
* function: invoke with an explicit return type.
*   `invoke_r<R>(f, args...)` is `invoke(f, args...)` with the result
* implicitly converted to `R` (or discarded if `R` is `void`). Standard
* surface is C++23; restd back-ports it on top of `restd::invoke`.
*
*   The void overload is not `constexpr` on C++11 because C++11 forbids
* constexpr void functions; from C++14 it is `D_CONSTEXPR`-qualified.
*
*
* path:      /inc/restd/functional/invoke_r.hpp
* link(s):   TBA
* author(s): restd                                       date: 2026.05.07
***********************************************************************/

#ifndef RESTD_FUNCTIONAL_INVOKE_R_
#define RESTD_FUNCTIONAL_INVOKE_R_ 1

#include "djinterp.hpp"

#if (D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES &&  \
     D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES)

#include "restd/type_traits/type_traits.hpp"
#include "restd/utility/forward.hpp"
#include "restd/functional/invoke.hpp"

namespace restd
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
    return restd::invoke(restd::forward<_F>(_f),
                         restd::forward<_Args>(_args)...);
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
    static_cast<void>(restd::invoke(restd::forward<_F>(_f),
                                    restd::forward<_Args>(_args)...));
}

} // namespace restd

#endif // variadic templates + rvalue references

#endif // RESTD_FUNCTIONAL_INVOKE_R_
