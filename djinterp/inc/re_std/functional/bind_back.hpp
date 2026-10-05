/*******************************************************************************
* djinterp [re_std]                                                bind_back.hpp
*
* bind_back class header:
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
* path:      /inc/re_std/functional/bind_back.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_FUNCTIONAL_BIND_BACK_HPP
#define RE_STD_FUNCTIONAL_BIND_BACK_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "../utility/make_integer_sequence.hpp"
#include "../tuple/tuple.hpp"
#include "../tuple/tuple_get.hpp"
#include "./invoke.hpp"


namespace re_std
{

namespace internal
{

    // bind_back_t
    //   class: the object returned by bind_back().
    template<typename Func, typename... Bound>
    class bind_back_t
    {
        typedef make_index_sequence<sizeof...(Bound)> _Indices;

        Func            m_func;
        tuple<Bound...> m_bound;

        //   Static so the const and non-const operator() overloads share one
        // definition; Tup deduces as const when the object is.
        template<typename F, typename Tup, typename... Args, size_t... I>
        static RE_STD_CONSTEXPR_CPP14 auto expand(F& f, Tup& bound,
                                             index_sequence<I...>,
                                             Args&&... args)
            -> decltype(re_std::invoke(f, static_cast<Args&&>(args)..., re_std::get<I>(bound)...))
        {
            return re_std::invoke(f, static_cast<Args&&>(args)..., re_std::get<I>(bound)...);
        }

    public:
        template<typename F2, typename... B2>
        RE_STD_CONSTEXPR explicit bind_back_t(F2&& f, B2&&... b)
            : m_func(static_cast<F2&&>(f)), m_bound(static_cast<B2&&>(b)...)
        {}

        template<typename... Args>
        RE_STD_CONSTEXPR_CPP14 auto operator()(Args&&... args)
            -> decltype(expand(m_func, m_bound, _Indices(),
                               static_cast<Args&&>(args)...))
        {
            return expand(m_func, m_bound, _Indices(),
                          static_cast<Args&&>(args)...);
        }

        template<typename... Args>
        RE_STD_CONSTEXPR auto operator()(Args&&... args) const
            -> decltype(expand(m_func, m_bound, _Indices(),
                               static_cast<Args&&>(args)...))
        {
            return expand(m_func, m_bound, _Indices(),
                          static_cast<Args&&>(args)...);
        }
    };

}  // internal

// bind_back
//   function: bind the trailing arguments of func.
template<typename Func, typename... Bound>
RE_STD_CONSTEXPR internal::bind_back_t<typename decay<Func>::type,
                             typename decay<Bound>::type...>
bind_back(Func&& func, Bound&&... bound)
{
    return internal::bind_back_t<typename decay<Func>::type,
                            typename decay<Bound>::type...>(
        static_cast<Func&&>(func), static_cast<Bound&&>(bound)...);
}

}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_FUNCTIONAL_BIND_BACK_HPP
