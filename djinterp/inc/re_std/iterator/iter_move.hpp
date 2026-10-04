/******************************************************************************
* djinterp [re_std]                                               iter_move.hpp
*
* iter_move header:
*   Provides the C++20 re_std::iter_move customisation point object
* (niebloid) and the iter_rvalue_reference_t<I> alias. In standard
* C++ these live in std::ranges:: and <iterator>; re_std keeps them
* in the flat re_std:: namespace to match the rest of the codebase.
*
*   The iter_move CPO is a function-object-based niebloid that
* dispatches in two priority stages:
*   1. If iter_move(it) is callable via ADL on the iterator's type
*      (user customisation), use that.
*   2. Otherwise return static_cast<iter_rvalue_reference_t<I>>(*it)
*      — i.e. a cast of the deref to its corresponding rvalue ref.
*
*   The "poison pill" iter_move() = delete declaration in the
* dispatcher namespace ensures ADL is the chosen mechanism rather
* than ordinary unqualified lookup finding some unrelated function.
*
*   PORTABILITY:
*   - C++11+ for the CPO and the alias.
*   - C++17+ uses an inline-constexpr instance; C++11/14 uses a
*     static-constexpr instance (the standard niebloid pattern).
*   - Priority-based overload resolution uses a small priority<N>
*     class-hierarchy idiom (no concepts required).
*
*
* path:      /inc/djinterp/re_std/ranges/iter_move.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ITERATOR_ITER_MOVE_
#define DJINTERP_RE_STD_ITERATOR_ITER_MOVE_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "./iterator_traits.hpp"


NS_RESTD


// ===========================================================================
// I.   ITER_MOVE NIEBLOID
// ===========================================================================

NS_INTERNAL

namespace _iter_move_fn
{
    // priority<N>
    //   class: linear hierarchy for prioritising overloads.
    // priority<N> is convertible to priority<M> for M <= N, so a
    // call expression that names priority<2> as its argument
    // matches the highest-priority overload that accepts the type.
    template<int _N>
    struct priority : priority<_N - 1>
    {};

    template<>
    struct priority<0>
    {};


    // poison pill — ensures the unqualified call iter_move(it)
    // inside _impl below is found via ADL on _I (rather than via
    // ordinary lookup picking up some unrelated function).
    void iter_move() = delete;


    // _impl, priority 2: ADL form. SFINAE-detects iter_move(it).
    template<typename _I>
    D_CONSTEXPR_INLINE
    auto
    _impl(
        _I&&             _it,
        priority<2>
    )
        -> decltype(iter_move(static_cast<_I&&>(_it)))
    {
        return iter_move(static_cast<_I&&>(_it));
    }


    // _impl, priority 1: default fallback. Cast *it to its rvalue
    // reference type. Mirrors the C++20 default for iter_move when
    // no user customisation is provided.
    template<typename _I>
    D_CONSTEXPR_INLINE
    auto
    _impl(
        _I&&             _it,
        priority<1>
    )
        -> typename conditional<
                is_lvalue_reference<decltype(*static_cast<_I&&>(_it))>::value,
                typename add_rvalue_reference<
                    typename remove_reference<
                        decltype(*static_cast<_I&&>(_it))
                    >::type
                >::type,
                decltype(*static_cast<_I&&>(_it))
           >::type
    {
        typedef decltype(*static_cast<_I&&>(_it)) deref_t;
        typedef typename conditional<
            is_lvalue_reference<deref_t>::value,
            typename add_rvalue_reference<
                typename remove_reference<deref_t>::type
            >::type,
            deref_t
        >::type result_t;
        return static_cast<result_t>(*static_cast<_I&&>(_it));
    }


    // fn — the callable type. operator() dispatches to the
    // highest-priority _impl that compiles for the given _I.
    struct fn
    {
        template<typename _I>
        D_CONSTEXPR_INLINE
        auto
        operator()(
            _I&&  _it
        ) const
            -> decltype(_impl(static_cast<_I&&>(_it), priority<2>()))
        {
            return _impl(static_cast<_I&&>(_it), priority<2>());
        }
    };
}  // namespace _iter_move_fn

NS_END  // internal


// iter_move
//   object: the CPO instance. Inline-constexpr on C++17+,
// static-constexpr on C++11/14.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
inline D_CONSTEXPR internal::_iter_move_fn::fn iter_move = internal::_iter_move_fn::fn();
#else
static D_CONSTEXPR internal::_iter_move_fn::fn iter_move = internal::_iter_move_fn::fn();
#endif


// ===========================================================================
// II.  ITER_RVALUE_REFERENCE_T
// ===========================================================================

// iter_rvalue_reference_t<_I>
//   alias: the type yielded by re_std::iter_move on iterator type _I.
template<typename _I>
struct iter_rvalue_reference
{
    typedef decltype(re_std::iter_move(declval<_I&>())) type;
};

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
template<typename _I>
using iter_rvalue_reference_t = typename iter_rvalue_reference<_I>::type;
#endif


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_ITERATOR_ITER_MOVE_
