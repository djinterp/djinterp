/***********************************************************************
* restd                                                       not_fn.hpp
*
* function: returns a callable that negates the result of `_F`.
*   `not_fn(f)(args...)` is `!invoke(f, args...)`. Replaces the
* deprecated C++98 `not1` / `not2` adaptors. Standard surface is C++17;
* restd back-ports it on top of `restd::invoke`.
*
*   Min standard: C++11. Standard made it constexpr in C++20 (P1065);
* restd makes the call operator constexpr from C++11 (single-return
* body).
*
*
* path:      /inc/restd/functional/not_fn.hpp
* link(s):   TBA
* author(s): restd                                       date: 2026.05.07
***********************************************************************/

#ifndef RESTD_FUNCTIONAL_NOT_FN_
#define RESTD_FUNCTIONAL_NOT_FN_ 1

#include "djinterp.hpp"

#if (D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES &&  \
     D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES)

#include "restd/type_traits/type_traits.hpp"
#include "restd/utility/forward.hpp"
#include "restd/functional/invoke.hpp"

namespace restd
{

NS_INTERNAL

    // not_fn_wrapper
    //   class: callable returned by not_fn. Stores the wrapped callable
    // by decayed value and negates the result of invoking it.
    template<typename _F>
    class not_fn_wrapper
    {
    private:
        // declared BEFORE operator(): a trailing return type is not a
        // complete-class context, so a member declared after it is not
        // yet visible there.
        _F m_f;

    public:
        template<typename _G>
        D_CONSTEXPR explicit not_fn_wrapper(
            _G&& _g
        )
            : m_f(restd::forward<_G>(_g))
        {}

        // call: lvalue this
        template<typename... _Args>
        D_CONSTEXPR auto
        operator()(
            _Args&&... _args
        ) -> decltype(!restd::invoke(m_f, restd::forward<_Args>(_args)...))
        {
            return !restd::invoke(m_f, restd::forward<_Args>(_args)...);
        }

        // call: const lvalue this
        template<typename... _Args>
        D_CONSTEXPR auto
        operator()(
            _Args&&... _args
        ) const -> decltype(!restd::invoke(m_f,
                                           restd::forward<_Args>(_args)...))
        {
            return !restd::invoke(m_f, restd::forward<_Args>(_args)...);
        }

    };

NS_END  // internal

// not_fn
//   function: factory wrapping `_F` so its negated result is returned.
template<typename _F>
D_CONSTEXPR internal::not_fn_wrapper<typename decay<_F>::type>
not_fn(
    _F&& _f
)
{
    return internal::not_fn_wrapper<typename decay<_F>::type>(
        restd::forward<_F>(_f));
}

} // namespace restd

#endif // variadic templates + rvalue references

#endif // RESTD_FUNCTIONAL_NOT_FN_
