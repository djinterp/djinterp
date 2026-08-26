/***********************************************************************
* re_std                                                      invoke.hpp
*
* function: standard INVOKE pseudo-operation.
*   Generalises function-call syntax across every callable shape:
*   1.  pointer-to-member-function on an object of the owning class
*       (or a derived class)              ->  (obj.*f)(args...)
*   2.  pointer-to-member-function on a reference_wrapper
*                                         ->  (rw.get().*f)(args...)
*   3.  pointer-to-member-function on anything else (a pointer, a smart
*       pointer)                          ->  ((*ptr).*f)(args...)
*   4.  pointer-to-member-data on an object  ->  obj.*f
*   5.  pointer-to-member-data on a reference_wrapper -> rw.get().*f
*   6.  pointer-to-member-data on a pointer  ->  (*ptr).*f
*   7.  any other callable (function ptr, lambda, function object)
*                                         ->  f(args...)
*
*   Each case is a separate overload; SFINAE on the trailing return
* type plus enable_if on the type relations elects the right one.
*
*   Min standard: C++11 (variadic templates + perfect forwarding).
* `D_CONSTEXPR` lifts to `constexpr` from C++11 onward; the standard
* did not make INVOKE constexpr until C++20 (P1065), so this header
* over-qualifies relative to std on C++11 / C++14 / C++17. That is
* deliberate -- re_std's "constexpr maximization" goal.
*
*
* path:      /inc/re_std/functional/invoke.hpp
* link(s):   TBA
* author(s): re_std                                      date: 2026.05.07
***********************************************************************/

#ifndef DJINTERP_RE_STD_FUNCTIONAL_INVOKE_
#define DJINTERP_RE_STD_FUNCTIONAL_INVOKE_ 1

#include "djinterp.hpp"

#if (D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES &&  \
     D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES)

#include "re_std/type_traits/type_traits.hpp"
#include "re_std/utility/forward.hpp"
// reference_wrapper.hpp includes this header at its tail. Including it
// here closes the cycle, but it is safe because of the include guards:
// whichever header the user opens first defines its types before the
// other one's body is parsed. We need the full definition of
// `is_reference_wrapper` (a forward declaration is not enough for the
// `::value` access in bullets 2 and 5).
#include "re_std/functional/is_reference_wrapper.hpp"

namespace re_std
{

NS_INTERNAL

    // -------------------------------------------------------------------
    // bullet 1: PMF, first arg derived from owning class -- (a1.*f)(args)
    // -------------------------------------------------------------------
    template<typename _F,
             typename _Class,
             typename _A1,
             typename... _Args>
    D_CONSTEXPR auto
    INVOKE(
        _F _Class::*_f,
        _A1&&       _a1,
        _Args&&...  _args
    ) -> typename enable_if<
            ( is_function<_F>::value &&
              is_base_of<_Class, typename decay<_A1>::type>::value ),
            decltype((re_std::forward<_A1>(_a1).*_f)
                         (re_std::forward<_Args>(_args)...))
         >::type
    {
        return (re_std::forward<_A1>(_a1).*_f)
                   (re_std::forward<_Args>(_args)...);
    }

    // -------------------------------------------------------------------
    // bullet 2: PMF, first arg is reference_wrapper -- (a1.get().*f)(args)
    // -------------------------------------------------------------------
    template<typename _F,
             typename _Class,
             typename _A1,
             typename... _Args>
    D_CONSTEXPR auto
    INVOKE(
        _F _Class::*_f,
        _A1&&       _a1,
        _Args&&...  _args
    ) -> typename enable_if<
            ( is_function<_F>::value &&
              is_reference_wrapper<typename decay<_A1>::type>::value ),
            decltype((_a1.get().*_f)
                         (re_std::forward<_Args>(_args)...))
         >::type
    {
        return (_a1.get().*_f)
                   (re_std::forward<_Args>(_args)...);
    }

    // -------------------------------------------------------------------
    // bullet 3: PMF, first arg is a pointer -- ((*a1).*f)(args)
    // -------------------------------------------------------------------
    template<typename _F,
             typename _Class,
             typename _A1,
             typename... _Args>
    D_CONSTEXPR auto
    INVOKE(
        _F _Class::*_f,
        _A1&&       _a1,
        _Args&&...  _args
    ) -> typename enable_if<
            ( is_function<_F>::value &&
              !is_base_of<_Class, typename decay<_A1>::type>::value &&
              !is_reference_wrapper<typename decay<_A1>::type>::value ),
            decltype(((*re_std::forward<_A1>(_a1)).*_f)
                         (re_std::forward<_Args>(_args)...))
         >::type
    {
        return ((*re_std::forward<_A1>(_a1)).*_f)
                   (re_std::forward<_Args>(_args)...);
    }

    // -------------------------------------------------------------------
    // bullet 4: PMD, first arg derived from owning class -- a1.*f
    // -------------------------------------------------------------------
    template<typename _F,
             typename _Class,
             typename _A1>
    D_CONSTEXPR auto
    INVOKE(
        _F _Class::*_f,
        _A1&&       _a1
    ) -> typename enable_if<
            ( !is_function<_F>::value &&
              is_base_of<_Class, typename decay<_A1>::type>::value ),
            decltype(re_std::forward<_A1>(_a1).*_f)
         >::type
    {
        return re_std::forward<_A1>(_a1).*_f;
    }

    // -------------------------------------------------------------------
    // bullet 5: PMD, first arg is reference_wrapper -- a1.get().*f
    // -------------------------------------------------------------------
    template<typename _F,
             typename _Class,
             typename _A1>
    D_CONSTEXPR auto
    INVOKE(
        _F _Class::*_f,
        _A1&&       _a1
    ) -> typename enable_if<
            ( !is_function<_F>::value &&
              is_reference_wrapper<typename decay<_A1>::type>::value ),
            decltype(_a1.get().*_f)
         >::type
    {
        return _a1.get().*_f;
    }

    // -------------------------------------------------------------------
    // bullet 6: PMD, first arg is a pointer -- (*a1).*f
    // -------------------------------------------------------------------
    template<typename _F,
             typename _Class,
             typename _A1>
    D_CONSTEXPR auto
    INVOKE(
        _F _Class::*_f,
        _A1&&       _a1
    ) -> typename enable_if<
            ( !is_function<_F>::value &&
              !is_base_of<_Class, typename decay<_A1>::type>::value &&
              !is_reference_wrapper<typename decay<_A1>::type>::value ),
            decltype((*re_std::forward<_A1>(_a1)).*_f)
         >::type
    {
        return (*re_std::forward<_A1>(_a1)).*_f;
    }

    // -------------------------------------------------------------------
    // bullet 7: anything callable -- f(args...)
    //   When F is a pointer-to-member, `forward<F>(f)(args...)` is ill-
    // formed (PMs cannot be called with `()`), so SFINAE on the return
    // type discards this overload and one of bullets 1-6 wins. For
    // ordinary callables the trailing return is well-formed and this
    // overload is the only viable one.
    // -------------------------------------------------------------------
    template<typename _F,
             typename... _Args>
    D_CONSTEXPR auto
    INVOKE(
        _F&&        _f,
        _Args&&...  _args
    ) -> decltype(re_std::forward<_F>(_f)
                      (re_std::forward<_Args>(_args)...))
    {
        return re_std::forward<_F>(_f)
                   (re_std::forward<_Args>(_args)...);
    }

NS_END  // internal

// invoke
//   function: public entry point. Forwards to the matching INVOKE
// overload chosen by the rules above.
template<typename _F,
         typename... _Args>
D_CONSTEXPR auto
invoke(
    _F&&        _f,
    _Args&&...  _args
) -> decltype(internal::INVOKE(re_std::forward<_F>(_f),
                               re_std::forward<_Args>(_args)...))
{
    return internal::INVOKE(re_std::forward<_F>(_f),
                            re_std::forward<_Args>(_args)...);
}

} // namespace re_std

#endif // variadic templates + rvalue references

#endif  // DJINTERP_RE_STD_FUNCTIONAL_INVOKE_
