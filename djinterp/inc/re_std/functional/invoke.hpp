/*******************************************************************************
* djinterp [re_std]                                                   invoke.hpp
*
* invoke function header:
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
* `RE_STD_CONSTEXPR` lifts to `constexpr` from C++11 onward; the standard
* did not make INVOKE constexpr until C++20 (P1065), so this header
* over-qualifies relative to std on C++11 / C++14 / C++17. That is
* deliberate -- re_std's "constexpr maximization" goal.
*
*
* path:      /inc/re_std/functional/invoke.hpp
* link(s):   TBA
* author(s): re_std                                          created: 2026.05.07
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_FUNCTIONAL_INVOKE_HPP
#define RE_STD_FUNCTIONAL_INVOKE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if (RE_STD_LANG_HAS_VARIADIC_TEMPLATES &&  \
     RE_STD_LANG_HAS_RVALUE_REFERENCES)

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

namespace internal
{

    // -------------------------------------------------------------------
    // bullet 1: PMF, first arg derived from owning class -- (a1.*f)(args)
    // -------------------------------------------------------------------
    template<typename F,
             typename Class,
             typename A1,
             typename... Args>
    RE_STD_CONSTEXPR auto
    INVOKE(
        F Class::*_f,
        A1&&       _a1,
        Args&&...  _args
    ) -> typename enable_if<
            ( is_function<F>::value &&
              is_base_of<Class, typename decay<A1>::type>::value ),
            decltype((re_std::forward<A1>(_a1).*_f)
                         (re_std::forward<Args>(_args)...))
         >::type
    {
        return (re_std::forward<A1>(_a1).*_f)
                   (re_std::forward<Args>(_args)...);
    }

    // -------------------------------------------------------------------
    // bullet 2: PMF, first arg is reference_wrapper -- (a1.get().*f)(args)
    // -------------------------------------------------------------------
    template<typename F,
             typename Class,
             typename A1,
             typename... Args>
    RE_STD_CONSTEXPR auto
    INVOKE(
        F Class::*_f,
        A1&&       _a1,
        Args&&...  _args
    ) -> typename enable_if<
            ( is_function<F>::value &&
              is_reference_wrapper<typename decay<A1>::type>::value ),
            decltype((_a1.get().*_f)
                         (re_std::forward<Args>(_args)...))
         >::type
    {
        return (_a1.get().*_f)
                   (re_std::forward<Args>(_args)...);
    }

    // -------------------------------------------------------------------
    // bullet 3: PMF, first arg is a pointer -- ((*a1).*f)(args)
    // -------------------------------------------------------------------
    template<typename F,
             typename Class,
             typename A1,
             typename... Args>
    RE_STD_CONSTEXPR auto
    INVOKE(
        F Class::*_f,
        A1&&       _a1,
        Args&&...  _args
    ) -> typename enable_if<
            ( is_function<F>::value &&
              !is_base_of<Class, typename decay<A1>::type>::value &&
              !is_reference_wrapper<typename decay<A1>::type>::value ),
            decltype(((*re_std::forward<A1>(_a1)).*_f)
                         (re_std::forward<Args>(_args)...))
         >::type
    {
        return ((*re_std::forward<A1>(_a1)).*_f)
                   (re_std::forward<Args>(_args)...);
    }

    // -------------------------------------------------------------------
    // bullet 4: PMD, first arg derived from owning class -- a1.*f
    // -------------------------------------------------------------------
    template<typename F,
             typename Class,
             typename A1>
    RE_STD_CONSTEXPR auto
    INVOKE(
        F Class::*_f,
        A1&&       _a1
    ) -> typename enable_if<
            ( !is_function<F>::value &&
              is_base_of<Class, typename decay<A1>::type>::value ),
            decltype(re_std::forward<A1>(_a1).*_f)
         >::type
    {
        return re_std::forward<A1>(_a1).*_f;
    }

    // -------------------------------------------------------------------
    // bullet 5: PMD, first arg is reference_wrapper -- a1.get().*f
    // -------------------------------------------------------------------
    template<typename F,
             typename Class,
             typename A1>
    RE_STD_CONSTEXPR auto
    INVOKE(
        F Class::*_f,
        A1&&       _a1
    ) -> typename enable_if<
            ( !is_function<F>::value &&
              is_reference_wrapper<typename decay<A1>::type>::value ),
            decltype(_a1.get().*_f)
         >::type
    {
        return _a1.get().*_f;
    }

    // -------------------------------------------------------------------
    // bullet 6: PMD, first arg is a pointer -- (*a1).*f
    // -------------------------------------------------------------------
    template<typename F,
             typename Class,
             typename A1>
    RE_STD_CONSTEXPR auto
    INVOKE(
        F Class::*_f,
        A1&&       _a1
    ) -> typename enable_if<
            ( !is_function<F>::value &&
              !is_base_of<Class, typename decay<A1>::type>::value &&
              !is_reference_wrapper<typename decay<A1>::type>::value ),
            decltype((*re_std::forward<A1>(_a1)).*_f)
         >::type
    {
        return (*re_std::forward<A1>(_a1)).*_f;
    }

    // -------------------------------------------------------------------
    // bullet 7: anything callable -- f(args...)
    //   When F is a pointer-to-member, `forward<F>(f)(args...)` is ill-
    // formed (PMs cannot be called with `()`), so SFINAE on the return
    // type discards this overload and one of bullets 1-6 wins. For
    // ordinary callables the trailing return is well-formed and this
    // overload is the only viable one.
    // -------------------------------------------------------------------
    template<typename F,
             typename... Args>
    RE_STD_CONSTEXPR auto
    INVOKE(
        F&&        _f,
        Args&&...  _args
    ) -> decltype(re_std::forward<F>(_f)
                      (re_std::forward<Args>(_args)...))
    {
        return re_std::forward<F>(_f)
                   (re_std::forward<Args>(_args)...);
    }

}  // internal

// invoke
//   function: public entry point. Forwards to the matching INVOKE
// overload chosen by the rules above.
template<typename F,
         typename... Args>
RE_STD_CONSTEXPR auto
invoke(
    F&&        _f,
    Args&&...  _args
) -> decltype(internal::INVOKE(re_std::forward<F>(_f),
                               re_std::forward<Args>(_args)...))
{
    return internal::INVOKE(re_std::forward<F>(_f),
                            re_std::forward<Args>(_args)...);
}

}  // re_std
#endif // variadic templates + rvalue references

#endif  // RE_STD_FUNCTIONAL_INVOKE_HPP
