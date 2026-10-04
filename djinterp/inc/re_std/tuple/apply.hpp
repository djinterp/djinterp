/******************************************************************************
* djinterp [re_std]                                                   apply.hpp
*
* apply function header:
*   Invokes a callable with the elements of a tuple-like object as
* arguments. C++17 standard library function, shimmed to C++11+.
*
*     auto sum = [](int a, int b, int c){ return a + b + c; };
*     apply(sum, make_tuple(1, 2, 3));   // -> 6
*
*   IMPLEMENTATION:
*   Expands the tuple with re_std::make_index_sequence and dispatches
* through re_std::invoke.
*
*   INVOKE DELEGATION (completed 2026-08-25):
*   The call now goes through re_std::invoke rather than a direct
* `f(args...)`, which is what makes pointer-to-member callables work:
*
*     struct P { int x; int scaled(int k) const { return x * k; } };
*     P p{6};
*     apply(&P::scaled, make_tuple(p, 7));   // -> 42
*     apply(&P::x,      make_tuple(p));      // -> 6
*
* Both forms are ill-formed with a direct call, since a pointer to
* member cannot be invoked with (). Plain function pointers, function
* objects and lambdas are unaffected.
*
*   PORTABILITY:
*   Requires variadic templates and rvalue references (C++11+).
*
*
* path:      /inc/djinterp/re_std/tuple/apply.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_RE_STD_TUPLE_APPLY_
#define DJINTERP_RE_STD_TUPLE_APPLY_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if ( D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES &&                            \
      D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES )


// std
#include <cstddef>
// djinterp
#include "./tuple.hpp"
#include "./tuple_size.hpp"
#include "./tuple_get.hpp"
#include "../type_traits/remove_reference.hpp"
#include "../utility/forward.hpp"
#include "../utility/integer_sequence.hpp"
#include "../utility/make_integer_sequence.hpp"
#include "../functional/invoke.hpp"


NS_RESTD


// =============================================================================
// I.   APPLY
// =============================================================================

NS_INTERNAL

    // apply_impl
    //   helper: expands the index pack and hands the elements to
    // re_std::invoke, which selects the right INVOKE form for the
    // callable (ordinary call, pointer-to-member-function, or
    // pointer-to-member-data).
    template<typename       _F,
             typename       _Tup,
             std::size_t... _Is>
    D_CONSTEXPR
    auto
    apply_impl(
        _F&&    _f,
        _Tup&&  _t,
        re_std::index_sequence<_Is...>
    ) -> decltype(re_std::invoke(re_std::forward<_F>(_f),
                                 get<_Is>(static_cast<_Tup&&>(_t))...))
    {
        return re_std::invoke(re_std::forward<_F>(_f),
                              get<_Is>(static_cast<_Tup&&>(_t))...);
    }

NS_END  // internal


// apply
//   function: invokes _f with the elements of _t as arguments.
template<typename _F,
         typename _Tup>
D_CONSTEXPR
auto
apply(
    _F&&    _f,
    _Tup&&  _t
)
    -> decltype(internal::apply_impl(
        static_cast<_F&&>(_f),
        static_cast<_Tup&&>(_t),
        re_std::make_index_sequence<
            tuple_size<typename remove_reference<_Tup>::type>::value
        >()))
{
    return internal::apply_impl(
        static_cast<_F&&>(_f),
        static_cast<_Tup&&>(_t),
        re_std::make_index_sequence<
            tuple_size<typename remove_reference<_Tup>::type>::value
        >());
}


NS_END  // re_std


#endif  // variadic templates && rvalue references


#endif  // DJINTERP_RE_STD_TUPLE_APPLY_
