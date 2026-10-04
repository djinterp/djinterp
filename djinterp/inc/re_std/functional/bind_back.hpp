/******************************************************************************
* re_std [functional]                                             bind_back.hpp
*
*   bind_back(f, args...) - partial application with the bound arguments placed
* after the call arguments.
*
*   THE SIMPLER, BETTER-BEHAVED HALF OF bind.
*   No placeholders, no reordering, and - crucially - NO nested-bind
* inspection: a bind expression passed to bind_back is stored and forwarded as an
* ordinary functor rather than being invoked with the call arguments.  That is
* deliberate in std, and it is why bind_back composes predictably where bind can
* surprise.  If you want bind's substitution behaviour, use bind.
*
*   Bound arguments are still decayed and stored by value, as with bind, so
* ref() / cref() remain the way to bind by reference.
*
*   STD IS C++23; re_std IS C++11 - the machinery needs only variadic
* templates and a tuple, both available since C++11.  std was simply late to
* add it, so this is a twelve-year back-port.
*
*
* path:      /inc/djinterp/re_std/functional/bind_back.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_FUNCTIONAL_BIND_BACK_
#define DJINTERP_RE_STD_FUNCTIONAL_BIND_BACK_ 1

// re_std
#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "../tuple/tuple.hpp"
#include "./invoke.hpp"

NS_RESTD

NS_INTERNAL

    // bind_back_t
    //   class: the object returned by bind_back().
    template<typename _Func, typename... _Bound>
    class bind_back_t
    {
        typedef make_index_sequence<sizeof...(_Bound)> _Indices;

        _Func            m_func;
        tuple<_Bound...> m_bound;

        //   Static so the const and non-const operator() overloads share one
        // definition; _Tup deduces as const when the object is.
        template<typename _F, typename _Tup, typename... _Args, size_t... _I>
        static D_CONSTEXPR_CPP14 auto expand(_F& f, _Tup& bound,
                                             index_sequence<_I...>,
                                             _Args&&... args)
            -> decltype(re_std::invoke(f, static_cast<_Args&&>(args)..., re_std::get<_I>(bound)...))
        {
            return re_std::invoke(f, static_cast<_Args&&>(args)..., re_std::get<_I>(bound)...);
        }

    public:
        template<typename _F2, typename... _B2>
        D_CONSTEXPR explicit bind_back_t(_F2&& f, _B2&&... b)
            : m_func(static_cast<_F2&&>(f)), m_bound(static_cast<_B2&&>(b)...)
        {}

        template<typename... _Args>
        D_CONSTEXPR_CPP14 auto operator()(_Args&&... args)
            -> decltype(expand(m_func, m_bound, _Indices(),
                               static_cast<_Args&&>(args)...))
        {
            return expand(m_func, m_bound, _Indices(),
                          static_cast<_Args&&>(args)...);
        }

        template<typename... _Args>
        D_CONSTEXPR auto operator()(_Args&&... args) const
            -> decltype(expand(m_func, m_bound, _Indices(),
                               static_cast<_Args&&>(args)...))
        {
            return expand(m_func, m_bound, _Indices(),
                          static_cast<_Args&&>(args)...);
        }
    };

NS_END  // internal

// bind_back
//   function: bind the trailing arguments of func.
template<typename _Func, typename... _Bound>
D_CONSTEXPR internal::bind_back_t<typename decay<_Func>::type,
                             typename decay<_Bound>::type...>
bind_back(_Func&& func, _Bound&&... bound)
{
    return internal::bind_back_t<typename decay<_Func>::type,
                            typename decay<_Bound>::type...>(
        static_cast<_Func&&>(func), static_cast<_Bound&&>(bound)...);
}

NS_END  // re_std
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_FUNCTIONAL_BIND_BACK_
