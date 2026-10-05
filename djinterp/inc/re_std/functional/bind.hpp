/*******************************************************************************
* djinterp [re_std]                                                     bind.hpp
*
* bind class header:
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
* path:      /inc/re_std/functional/bind.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_FUNCTIONAL_BIND_HPP
#define RE_STD_FUNCTIONAL_BIND_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "../tuple/tuple.hpp"
#include "../tuple/forward_as_tuple.hpp"
#include "./invoke.hpp"
#include "./placeholders.hpp"
#include "./is_bind_expression.hpp"


namespace re_std
{

namespace internal
{

    // bind_apply
    //   function: expand the stored bound arguments, resolve each against the
    // call arguments, and invoke.  Free rather than a member so that the const
    // and non-const operator() overloads share one definition - BoundTuple
    // deduces as const when the bind object is.
    template<typename Func, typename BoundTuple,
             typename CallTuple, size_t... Index>
    RE_STD_CONSTEXPR_CPP14 auto bind_apply(Func& func, BoundTuple& bound,
                                      CallTuple&& call,
                                      index_sequence<Index...>)
        -> decltype(re_std::invoke(
               func,
               bind_resolve(
                   typename bind_kind<typename tuple_element<
                       Index, BoundTuple>::type>::type(),
                   re_std::get<Index>(bound),
                   static_cast<CallTuple&&>(call))...))
    {
        return re_std::invoke(
            func,
            bind_resolve(
                typename bind_kind<typename tuple_element<
                    Index, BoundTuple>::type>::type(),
                re_std::get<Index>(bound),
                static_cast<CallTuple&&>(call))...);
    }

    // bind_expression
    //   class: the object returned by bind().  Deduced return type.
    template<typename Func, typename... Bound>
    class bind_expression
    {
        typedef tuple<Bound...>                       BoundTuple;
        typedef make_index_sequence<sizeof...(Bound)> _Indices;

        Func       m_func;
        BoundTuple m_bound;

    public:
        template<typename F2, typename... B2>
        RE_STD_CONSTEXPR explicit bind_expression(F2&& f, B2&&... b)
            : m_func(static_cast<F2&&>(f)),
              m_bound(static_cast<B2&&>(b)...)
        {}

        template<typename... Args>
        RE_STD_CONSTEXPR_CPP14 auto operator()(Args&&... args)
            -> decltype(bind_apply(
                   m_func, m_bound,
                   re_std::forward_as_tuple(static_cast<Args&&>(args)...),
                   _Indices()))
        {
            return bind_apply(
                m_func, m_bound,
                re_std::forward_as_tuple(static_cast<Args&&>(args)...),
                _Indices());
        }

        template<typename... Args>
        RE_STD_CONSTEXPR auto operator()(Args&&... args) const
            -> decltype(bind_apply(
                   m_func, m_bound,
                   re_std::forward_as_tuple(static_cast<Args&&>(args)...),
                   _Indices()))
        {
            return bind_apply(
                m_func, m_bound,
                re_std::forward_as_tuple(static_cast<Args&&>(args)...),
                _Indices());
        }
    };

    // bind_expression_r
    //   class: the object returned by bind<R>().  Return type is fixed, so
    // the result of the invocation is converted to Result at the boundary.
    template<typename Result, typename Func, typename... Bound>
    class bind_expression_r
    {
        typedef tuple<Bound...>                       BoundTuple;
        typedef make_index_sequence<sizeof...(Bound)> _Indices;

        Func       m_func;
        BoundTuple m_bound;

    public:
        typedef Result result_type;

        template<typename F2, typename... B2>
        RE_STD_CONSTEXPR explicit bind_expression_r(F2&& f, B2&&... b)
            : m_func(static_cast<F2&&>(f)),
              m_bound(static_cast<B2&&>(b)...)
        {}

        template<typename... Args>
        RE_STD_CONSTEXPR_CPP14 Result operator()(Args&&... args)
        {
            return static_cast<Result>(bind_apply(
                m_func, m_bound,
                re_std::forward_as_tuple(static_cast<Args&&>(args)...),
                _Indices()));
        }

        template<typename... Args>
        RE_STD_CONSTEXPR Result operator()(Args&&... args) const
        {
            return static_cast<Result>(bind_apply(
                m_func, m_bound,
                re_std::forward_as_tuple(static_cast<Args&&>(args)...),
                _Indices()));
        }
    };

}  // internal


// is_bind_expression<...>
//   trait: marks both bind result types, so a nested bind is recognised by
// the resolver rather than being stored and passed through as a functor.
template<typename Func, typename... Bound>
struct is_bind_expression<internal::bind_expression<Func, Bound...> >
    : true_type
{};

template<typename Result, typename Func, typename... Bound>
struct is_bind_expression<internal::bind_expression_r<Result, Func, Bound...> >
    : true_type
{};


// bind
//   function: partially apply func, deducing the return type at each call.
template<typename Func, typename... Bound>
RE_STD_CONSTEXPR internal::bind_expression<typename decay<Func>::type,
                                      typename decay<Bound>::type...>
bind(Func&& func, Bound&&... bound)
{
    return internal::bind_expression<typename decay<Func>::type,
                                     typename decay<Bound>::type...>(
        static_cast<Func&&>(func), static_cast<Bound&&>(bound)...);
}

// bind
//   function: partially apply func with a fixed return type Result.
template<typename Result, typename Func, typename... Bound>
RE_STD_CONSTEXPR internal::bind_expression_r<Result,
                                        typename decay<Func>::type,
                                        typename decay<Bound>::type...>
bind(Func&& func, Bound&&... bound)
{
    return internal::bind_expression_r<Result,
                                       typename decay<Func>::type,
                                       typename decay<Bound>::type...>(
        static_cast<Func&&>(func), static_cast<Bound&&>(bound)...);
}

}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_FUNCTIONAL_BIND_HPP
