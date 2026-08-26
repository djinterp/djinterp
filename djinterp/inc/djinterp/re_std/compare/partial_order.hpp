/******************************************************************************
* djinterp [re_std]                                              partial_order.hpp
*
* partial_order customisation point object header:
*   Per [cmp.alg]: a niebloid that produces a partial_ordering result
* for the inputs. Dispatch hierarchy:
*
*     (a) Floating-point special case — implicit on this path since
*         built-in <=> on floating-point already returns
*         partial_ordering.
*     (b) ADL partial_order(t, u) returning a value that constructs
*         partial_ordering.
*     (c) Built-in t <=> u with result castable to partial_ordering.
*     (d) Otherwise: ill-formed.
*
*   partial_order is the most permissive of the three _order
* niebloids in terms of which inputs it accepts — partial_ordering
* is the weakest category and every category converts to it. So a
* user type whose operator<=> returns strong_ordering or
* weak_ordering will still work with partial_order via path (c).
*
*   IMPLEMENTATION:
*   Same priority<N> + ADL-poison-pill pattern as strong_order /
* weak_order.
*
*   PORTABILITY:
*   Definition gated on D_ENV_LANG_IS_CPP20_OR_HIGHER.
*
*
* path:      /inc/djinterp/re_std/compare/partial_order.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.17
******************************************************************************/

#ifndef DJINTERP_RE_STD_COMPARE_PARTIAL_ORDER_
#define DJINTERP_RE_STD_COMPARE_PARTIAL_ORDER_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER


// djinterp
#include "./partial_ordering.hpp"


NS_RESTD


namespace _partial_order_cpo
{

    template<int _N>
    struct priority : priority<_N - 1> {};
    template<>
    struct priority<0> {};

    void partial_order() = delete;

    // (b) ADL path
    template<typename _T, typename _U>
    constexpr auto
    impl(
        priority<2>,
        _T&& _t,
        _U&& _u
    ) noexcept(noexcept(partial_ordering(partial_order(
                  static_cast<_T&&>(_t), static_cast<_U&&>(_u)))))
        -> decltype(partial_ordering(partial_order(
                        static_cast<_T&&>(_t), static_cast<_U&&>(_u))))
    {
        return partial_ordering(partial_order(
                   static_cast<_T&&>(_t), static_cast<_U&&>(_u)));
    }

    // (c) Built-in <=> path
    template<typename _T, typename _U>
    constexpr auto
    impl(
        priority<1>,
        _T&& _t,
        _U&& _u
    ) noexcept(noexcept(partial_ordering(
                  static_cast<_T&&>(_t) <=> static_cast<_U&&>(_u))))
        -> decltype(partial_ordering(
                        static_cast<_T&&>(_t) <=> static_cast<_U&&>(_u)))
    {
        return partial_ordering(
                   static_cast<_T&&>(_t) <=> static_cast<_U&&>(_u));
    }

}  // namespace _partial_order_cpo


namespace _partial_order_cpo_obj
{

    struct partial_order_fn
    {
        template<typename _T, typename _U>
        constexpr auto
        operator()(
            _T&& _t,
            _U&& _u
        ) const
            noexcept(noexcept(_partial_order_cpo::impl(
                _partial_order_cpo::priority<2>{},
                static_cast<_T&&>(_t),
                static_cast<_U&&>(_u))))
            -> decltype(_partial_order_cpo::impl(
                _partial_order_cpo::priority<2>{},
                static_cast<_T&&>(_t),
                static_cast<_U&&>(_u)))
        {
            return _partial_order_cpo::impl(
                _partial_order_cpo::priority<2>{},
                static_cast<_T&&>(_t),
                static_cast<_U&&>(_u));
        }
    };

}  // namespace _partial_order_cpo_obj


inline constexpr _partial_order_cpo_obj::partial_order_fn partial_order = {};


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


#endif  // DJINTERP_RE_STD_COMPARE_PARTIAL_ORDER_
