/******************************************************************************
* re_std [functional]                                                    bind.hpp
*
*   bind(f, args...) and bind<R>(f, args...) - partial application with
* argument reordering.
*
*   BOUND ARGUMENTS ARE DECAYED AND STORED BY VALUE.
*   That is std's rule and it surprises people: bind(f, x) copies x, so
* mutating x afterwards does not change what f sees.  ref(x) / cref(x) opt out
* by storing a reference_wrapper, which the resolver unwraps - see
* placeholders.hpp for the four resolution rules.
*
*   CALL ARGUMENTS ARE FORWARDED, NOT STORED.
*   They are captured into a forward_as_tuple of references and each
* placeholder pulls its own out with the original value category intact.  The
* tuple is passed to every resolver, which is why a nested bind expression can
* see all of them rather than just the one in its own position.
*
*   WHY BOTH CONST AND NON-CONST operator().
*   The stored callable may itself be non-const-invocable, and the bound
* arguments are handed to it as lvalues whose constness follows the bind
* object's.  Providing only one would silently forbid half the legitimate
* uses; providing both means `const auto b = bind(...)` works when f allows it
* and fails clearly when it does not.
*
*   bind<R> EXISTS FOR A REASON, not just convenience: when the callable's
* return type cannot be deduced - or when you want an implicit conversion
* applied at the boundary - the deduced form has no way to express it.  It is
* a separate type so that the fixed return type is part of the signature.
*
*   STD IS C++11; re_std IS C++11.  Hard ceiling: variadic templates plus the
* tuple holding the bound arguments.
*
*
* path:      /inc/djinterp/re_std/functional/bind.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_FUNCTIONAL_BIND_
#define RESTD_FUNCTIONAL_BIND_ 1

// re_std
#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "../tuple/tuple.hpp"
#include "./invoke.hpp"
#include "./placeholders.hpp"
#include "./is_bind_expression.hpp"

NS_DJINTERP
NS_RESTD

NS_INTERNAL

    // bind_apply
    //   function: expand the stored bound arguments, resolve each against the
    // call arguments, and invoke.  Free rather than a member so that the const
    // and non-const operator() overloads share one definition - _BoundTuple
    // deduces as const when the bind object is.
    template<typename _Func, typename _BoundTuple,
             typename _CallTuple, size_t... _Index>
    D_CONSTEXPR_CPP14 auto bind_apply(_Func& func, _BoundTuple& bound,
                                      _CallTuple&& call,
                                      index_sequence<_Index...>)
        -> decltype(re_std::invoke(
               func,
               bind_resolve(
                   typename bind_kind<typename tuple_element<
                       _Index, _BoundTuple>::type>::type(),
                   re_std::get<_Index>(bound),
                   static_cast<_CallTuple&&>(call))...))
    {
        return re_std::invoke(
            func,
            bind_resolve(
                typename bind_kind<typename tuple_element<
                    _Index, _BoundTuple>::type>::type(),
                re_std::get<_Index>(bound),
                static_cast<_CallTuple&&>(call))...);
    }

    // bind_expression
    //   class: the object returned by bind().  Deduced return type.
    template<typename _Func, typename... _Bound>
    class bind_expression
    {
        typedef tuple<_Bound...>                       _BoundTuple;
        typedef make_index_sequence<sizeof...(_Bound)> _Indices;

        _Func       m_func;
        _BoundTuple m_bound;

    public:
        template<typename _F2, typename... _B2>
        D_CONSTEXPR explicit bind_expression(_F2&& f, _B2&&... b)
            : m_func(static_cast<_F2&&>(f)),
              m_bound(static_cast<_B2&&>(b)...)
        {}

        template<typename... _Args>
        D_CONSTEXPR_CPP14 auto operator()(_Args&&... args)
            -> decltype(bind_apply(
                   m_func, m_bound,
                   re_std::forward_as_tuple(static_cast<_Args&&>(args)...),
                   _Indices()))
        {
            return bind_apply(
                m_func, m_bound,
                re_std::forward_as_tuple(static_cast<_Args&&>(args)...),
                _Indices());
        }

        template<typename... _Args>
        D_CONSTEXPR auto operator()(_Args&&... args) const
            -> decltype(bind_apply(
                   m_func, m_bound,
                   re_std::forward_as_tuple(static_cast<_Args&&>(args)...),
                   _Indices()))
        {
            return bind_apply(
                m_func, m_bound,
                re_std::forward_as_tuple(static_cast<_Args&&>(args)...),
                _Indices());
        }
    };

    // bind_expression_r
    //   class: the object returned by bind<R>().  Return type is fixed, so
    // the result of the invocation is converted to _Result at the boundary.
    template<typename _Result, typename _Func, typename... _Bound>
    class bind_expression_r
    {
        typedef tuple<_Bound...>                       _BoundTuple;
        typedef make_index_sequence<sizeof...(_Bound)> _Indices;

        _Func       m_func;
        _BoundTuple m_bound;

    public:
        typedef _Result result_type;

        template<typename _F2, typename... _B2>
        D_CONSTEXPR explicit bind_expression_r(_F2&& f, _B2&&... b)
            : m_func(static_cast<_F2&&>(f)),
              m_bound(static_cast<_B2&&>(b)...)
        {}

        template<typename... _Args>
        D_CONSTEXPR_CPP14 _Result operator()(_Args&&... args)
        {
            return static_cast<_Result>(bind_apply(
                m_func, m_bound,
                re_std::forward_as_tuple(static_cast<_Args&&>(args)...),
                _Indices()));
        }

        template<typename... _Args>
        D_CONSTEXPR _Result operator()(_Args&&... args) const
        {
            return static_cast<_Result>(bind_apply(
                m_func, m_bound,
                re_std::forward_as_tuple(static_cast<_Args&&>(args)...),
                _Indices()));
        }
    };

NS_END  // internal


// is_bind_expression<...>
//   trait: marks both bind result types, so a nested bind is recognised by
// the resolver rather than being stored and passed through as a functor.
template<typename _Func, typename... _Bound>
struct is_bind_expression<internal::bind_expression<_Func, _Bound...> >
    : true_type
{};

template<typename _Result, typename _Func, typename... _Bound>
struct is_bind_expression<internal::bind_expression_r<_Result, _Func, _Bound...> >
    : true_type
{};


// bind
//   function: partially apply func, deducing the return type at each call.
template<typename _Func, typename... _Bound>
D_CONSTEXPR internal::bind_expression<typename decay<_Func>::type,
                                      typename decay<_Bound>::type...>
bind(_Func&& func, _Bound&&... bound)
{
    return internal::bind_expression<typename decay<_Func>::type,
                                     typename decay<_Bound>::type...>(
        static_cast<_Func&&>(func), static_cast<_Bound&&>(bound)...);
}

// bind
//   function: partially apply func with a fixed return type _Result.
template<typename _Result, typename _Func, typename... _Bound>
D_CONSTEXPR internal::bind_expression_r<_Result,
                                        typename decay<_Func>::type,
                                        typename decay<_Bound>::type...>
bind(_Func&& func, _Bound&&... bound)
{
    return internal::bind_expression_r<_Result,
                                       typename decay<_Func>::type,
                                       typename decay<_Bound>::type...>(
        static_cast<_Func&&>(func), static_cast<_Bound&&>(bound)...);
}

NS_END  // re_std
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_FUNCTIONAL_BIND_
