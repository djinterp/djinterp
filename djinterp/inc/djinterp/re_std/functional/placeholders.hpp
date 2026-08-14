/******************************************************************************
* re_std [functional]                                            placeholders.hpp
*
*   placeholders::_1 ... _10, and the argument-resolution machinery that bind
* is built on.
*
*   WHAT A PLACEHOLDER ACTUALLY IS.
*   Not a value - a TYPE.  `_1` is an object of an empty type whose identity
* carries the index, and is_placeholder<T>::value recovers it.  Nothing about
* _1 is examined at run time; the substitution is entirely a compile-time
* selection of which call argument to forward.  That is why bind has no
* run-time dispatch cost beyond the indirect call itself.
*
*   THE FOUR ARGUMENT KINDS.
*   Every bound argument is resolved by one of four rules, selected by tag
* dispatch on its decayed type:
*
*     placeholder        substitute call argument N-1, forwarded
*     nested bind        invoke it with ALL the call arguments and substitute
*                        the result - this is what makes bind(f, bind(g, _1))
*                        mean f(g(x)) rather than f(g)
*     reference_wrapper  unwrap to the referenced object, so ref(x) defeats
*                        bind's usual decay-and-copy of bound arguments
*     anything else      pass the stored copy as an lvalue
*
*   The nested-bind rule is the one people forget exists, and it is why bind
* cannot simply store and forward: it must inspect each bound argument's type.
*
*   STD IS C++11; re_std IS C++11.  Variadic templates and the tuple that
* stores the bound arguments are both hard C++11 requirements.
*
*
* path:      /inc/djinterp/re_std/functional/placeholders.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_FUNCTIONAL_PLACEHOLDERS_
#define RESTD_FUNCTIONAL_PLACEHOLDERS_ 1

// re_std
#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "../tuple/tuple.hpp"
#include "./invoke.hpp"
#include "./reference_wrapper.hpp"
#include "./is_placeholder.hpp"
#include "./is_bind_expression.hpp"

NS_DJINTERP
NS_RESTD

NS_INTERNAL

    // placeholder_type
    //   struct: the type of placeholders::_N.  Empty; the index lives in the
    // type, not in any object.
    template<int _Index>
    struct placeholder_type
    {
        D_CONSTEXPR placeholder_type() D_NOEXCEPT {}
    };

    // ---- argument-kind tags ------------------------------------------
    struct bind_arg_placeholder {};
    struct bind_arg_nested      {};
    struct bind_arg_refwrap     {};
    struct bind_arg_plain       {};

    // is_reference_wrapper
    //   trait: true for re_std::reference_wrapper specialisations.  Not a std
    // trait, but bind needs the distinction and std's wording states it as a
    // special case rather than exposing a name for it.
    template<typename _Type>
    struct is_reference_wrapper : false_type {};

    template<typename _Type>
    struct is_reference_wrapper<reference_wrapper<_Type> > : true_type {};

    // bind_kind
    //   trait: which of the four resolution rules applies to a bound
    // argument.  Order matters - a placeholder is checked first because a
    // placeholder type could in principle also satisfy a later test.
    template<typename _Bound>
    struct bind_kind
    {
        typedef typename remove_cv<
            typename remove_reference<_Bound>::type>::type _Bare;

        typedef typename conditional<
            (is_placeholder<_Bare>::value > 0),
            bind_arg_placeholder,
            typename conditional<
                is_bind_expression<_Bare>::value,
                bind_arg_nested,
                typename conditional<
                    is_reference_wrapper<_Bare>::value,
                    bind_arg_refwrap,
                    bind_arg_plain>::type>::type>::type type;
    };

    // ---- resolution --------------------------------------------------

    // bind_resolve (placeholder)
    //   function: substitute the N-1'th call argument, forwarded with its
    // original value category.  The bound object itself is not read.
    template<typename _Bound, typename _CallTuple>
    D_CONSTEXPR_CPP14 auto bind_resolve(bind_arg_placeholder,
                                        _Bound&, _CallTuple&& call)
        -> decltype(re_std::get<
               is_placeholder<typename remove_cv<
                   typename remove_reference<_Bound>::type>::type>::value - 1>(
                       static_cast<_CallTuple&&>(call)))
    {
        return re_std::get<
            is_placeholder<typename remove_cv<
                typename remove_reference<_Bound>::type>::type>::value - 1>(
                    static_cast<_CallTuple&&>(call));
    }

    // bind_resolve (nested bind expression)
    //   function: invoke the nested expression with ALL the call arguments
    // and substitute its result.
    template<typename _Bound, typename _CallTuple>
    D_CONSTEXPR_CPP14 auto bind_resolve(bind_arg_nested,
                                        _Bound& bound, _CallTuple&& call)
        -> decltype(re_std::apply(bound, static_cast<_CallTuple&&>(call)))
    {
        return re_std::apply(bound, static_cast<_CallTuple&&>(call));
    }

    // bind_resolve (reference_wrapper)
    //   function: unwrap.  This is how ref(x) escapes bind's decay-and-copy
    // of bound arguments.
    template<typename _Bound, typename _CallTuple>
    D_CONSTEXPR auto bind_resolve(bind_arg_refwrap,
                                  _Bound& bound, _CallTuple&&)
        -> decltype(bound.get())
    {
        return bound.get();
    }

    // bind_resolve (plain value)
    //   function: pass the stored copy as an lvalue, const-qualified when the
    // bind object is.
    template<typename _Bound, typename _CallTuple>
    D_CONSTEXPR _Bound& bind_resolve(bind_arg_plain,
                                     _Bound& bound, _CallTuple&&)
    {
        return bound;
    }

NS_END  // internal


// is_placeholder<placeholder_type<_Index>>
//   trait: recovers the index carried by a placeholder's type.
template<int _Index>
struct is_placeholder<internal::placeholder_type<_Index> >
    : integral_constant<int, _Index>
{};


D_NAMESPACE(placeholders)

    //   std guarantees at least _1 through _10; re_std provides exactly that.
    // They are objects, not types, so that `bind(f, _1)` reads as a call.
    D_INLINE_VAR D_CONSTEXPR internal::placeholder_type<1>  _1  = {};
    D_INLINE_VAR D_CONSTEXPR internal::placeholder_type<2>  _2  = {};
    D_INLINE_VAR D_CONSTEXPR internal::placeholder_type<3>  _3  = {};
    D_INLINE_VAR D_CONSTEXPR internal::placeholder_type<4>  _4  = {};
    D_INLINE_VAR D_CONSTEXPR internal::placeholder_type<5>  _5  = {};
    D_INLINE_VAR D_CONSTEXPR internal::placeholder_type<6>  _6  = {};
    D_INLINE_VAR D_CONSTEXPR internal::placeholder_type<7>  _7  = {};
    D_INLINE_VAR D_CONSTEXPR internal::placeholder_type<8>  _8  = {};
    D_INLINE_VAR D_CONSTEXPR internal::placeholder_type<9>  _9  = {};
    D_INLINE_VAR D_CONSTEXPR internal::placeholder_type<10> _10 = {};

NS_END  // placeholders

NS_END  // re_std
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_FUNCTIONAL_PLACEHOLDERS_
