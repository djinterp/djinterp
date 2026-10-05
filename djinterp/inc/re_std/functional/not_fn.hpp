/*******************************************************************************
* djinterp [re_std]                                                   not_fn.hpp
*
* not_fn class header:
* function: returns a callable that negates the result of `F`.
*   `not_fn(f)(args...)` is `!invoke(f, args...)`. Replaces the
* deprecated C++98 `not1` / `not2` adaptors. Standard surface is C++17;
* re_std back-ports it on top of `re_std::invoke`.
*
*   Min standard: C++11. Standard made it constexpr in C++20 (P1065);
* re_std makes the call operator constexpr from C++11 (single-return
* body).
*
*
* path:      /inc/re_std/functional/not_fn.hpp
* link(s):   TBA
* author(s): re_std                                          created: 2026.05.07
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_FUNCTIONAL_NOT_FN_HPP
#define RE_STD_FUNCTIONAL_NOT_FN_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if (RE_STD_LANG_HAS_VARIADIC_TEMPLATES &&  \
     RE_STD_LANG_HAS_RVALUE_REFERENCES)

#include "re_std/type_traits/type_traits.hpp"
#include "re_std/utility/forward.hpp"
#include "re_std/functional/invoke.hpp"


namespace re_std
{

namespace internal
{

    // not_fn_wrapper
    //   class: callable returned by not_fn. Stores the wrapped callable
    // by decayed value and negates the result of invoking it.
    template<typename F>
    class not_fn_wrapper
    {
    private:
        // declared BEFORE operator(): a trailing return type is not a
        // complete-class context, so a member declared after it is not
        // yet visible there.
        F m_f;

    public:
        template<typename G>
        RE_STD_CONSTEXPR explicit not_fn_wrapper(
            G&& _g
        )
            : m_f(re_std::forward<G>(_g))
        {}

        // call: lvalue this
        //   constexpr from C++14 only: at C++11 constexpr on a member
        // function implies const, which would make this a redeclaration of
        // the const overload below.
        template<typename... Args>
        RE_STD_CONSTEXPR_CPP14 auto
        operator()(
            Args&&... _args
        ) -> decltype(!re_std::invoke(m_f, re_std::forward<Args>(_args)...))
        {
            return !re_std::invoke(m_f, re_std::forward<Args>(_args)...);
        }

        // call: const lvalue this
        template<typename... Args>
        RE_STD_CONSTEXPR auto
        operator()(
            Args&&... _args
        ) const -> decltype(!re_std::invoke(m_f,
                                           re_std::forward<Args>(_args)...))
        {
            return !re_std::invoke(m_f, re_std::forward<Args>(_args)...);
        }

    };

}  // internal

// not_fn
//   function: factory wrapping `F` so its negated result is returned.
template<typename F>
RE_STD_CONSTEXPR internal::not_fn_wrapper<typename decay<F>::type>
not_fn(
    F&& _f
)
{
    return internal::not_fn_wrapper<typename decay<F>::type>(
        re_std::forward<F>(_f));
}

}  // re_std
#endif // variadic templates + rvalue references

#endif  // RE_STD_FUNCTIONAL_NOT_FN_HPP
