/*******************************************************************************
* djinterp [re_std]                                                    apply.hpp
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
* path:      /inc/re_std/tuple/apply.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.30
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TUPLE_APPLY_HPP
#define RE_STD_TUPLE_APPLY_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if ( RE_STD_LANG_HAS_VARIADIC_TEMPLATES &&                            \
      RE_STD_LANG_HAS_RVALUE_REFERENCES )


// std
#include <cstddef>
// re_std
#include "./tuple.hpp"
#include "./tuple_size.hpp"
#include "./tuple_get.hpp"
#include "../type_traits/remove_reference.hpp"
#include "../utility/forward.hpp"
#include "../utility/integer_sequence.hpp"
#include "../utility/make_integer_sequence.hpp"
#include "../functional/invoke.hpp"


namespace re_std
{


// =============================================================================
// I.   APPLY
// =============================================================================

namespace internal
{

    // apply_impl
    //   function: expands the index pack and hands the elements to
    // re_std::invoke, which selects the right INVOKE form for the
    // callable (ordinary call, pointer-to-member-function, or
    // pointer-to-member-data).
    template<typename       F,
             typename       Tup,
             std::size_t... Is>
    RE_STD_CONSTEXPR
    auto
    apply_impl(
        F&&    _f,
        Tup&&  _t,
        re_std::index_sequence<Is...>
    ) -> decltype(re_std::invoke(re_std::forward<F>(_f),
                                 get<Is>(static_cast<Tup&&>(_t))...))
    {
        return re_std::invoke(re_std::forward<F>(_f),
                              get<Is>(static_cast<Tup&&>(_t))...);
    }

}  // internal


// apply
//   function: invokes _f with the elements of _t as arguments.
template<typename F,
         typename Tup>
RE_STD_CONSTEXPR
auto
apply(
    F&&    _f,
    Tup&&  _t
)
    -> decltype(internal::apply_impl(
        static_cast<F&&>(_f),
        static_cast<Tup&&>(_t),
        re_std::make_index_sequence<
            tuple_size<typename remove_reference<Tup>::type>::value
        >()))
{
    return internal::apply_impl(
        static_cast<F&&>(_f),
        static_cast<Tup&&>(_t),
        re_std::make_index_sequence<
            tuple_size<typename remove_reference<Tup>::type>::value
        >());
}


}  // re_std


#endif  // variadic templates && rvalue references


#endif  // RE_STD_TUPLE_APPLY_HPP
