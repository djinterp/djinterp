/******************************************************************************
* djinterp [restd]                                                  weak_order.hpp
*
* weak_order customisation point object header:
*   Per [cmp.alg]: a niebloid that produces a weak_ordering result
* for the inputs. Dispatch hierarchy:
*
*     (a) Floating-point special case — NOT YET SHIPPED in restd
*         (deferred; same scope notes as strong_order).
*     (b) ADL weak_order(t, u) returning a value that constructs
*         weak_ordering.
*     (c) Built-in t <=> u with result castable to weak_ordering.
*     (d) Otherwise: ill-formed.
*
*   IMPLEMENTATION:
*   Same priority<N> + ADL-poison-pill pattern as strong_order.
* See strong_order.hpp for an extended commentary; this file mirrors
* it with weak_ordering as the result-cast target.
*
*   PORTABILITY:
*   Definition gated on D_ENV_LANG_IS_CPP20_OR_HIGHER.
*
*
* path:      /inc/djinterp/re_std/compare/weak_order.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.17
******************************************************************************/

#ifndef DJINTERP_RESTD_COMPARE_WEAK_ORDER_
#define DJINTERP_RESTD_COMPARE_WEAK_ORDER_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER


// djinterp
#include "./weak_ordering.hpp"


NS_RESTD


namespace _weak_order_cpo
{

    template<int _N>
    struct priority : priority<_N - 1> {};
    template<>
    struct priority<0> {};

    void weak_order() = delete;

    // (b) ADL path
    template<typename _T, typename _U>
    constexpr auto
    impl(
        priority<2>,
        _T&& _t,
        _U&& _u
    ) noexcept(noexcept(weak_ordering(weak_order(
                  static_cast<_T&&>(_t), static_cast<_U&&>(_u)))))
        -> decltype(weak_ordering(weak_order(
                        static_cast<_T&&>(_t), static_cast<_U&&>(_u))))
    {
        return weak_ordering(weak_order(
                   static_cast<_T&&>(_t), static_cast<_U&&>(_u)));
    }

    // (c) Built-in <=> path
    template<typename _T, typename _U>
    constexpr auto
    impl(
        priority<1>,
        _T&& _t,
        _U&& _u
    ) noexcept(noexcept(weak_ordering(
                  static_cast<_T&&>(_t) <=> static_cast<_U&&>(_u))))
        -> decltype(weak_ordering(
                        static_cast<_T&&>(_t) <=> static_cast<_U&&>(_u)))
    {
        return weak_ordering(
                   static_cast<_T&&>(_t) <=> static_cast<_U&&>(_u));
    }

}  // namespace _weak_order_cpo


namespace _weak_order_cpo_obj
{

    struct weak_order_fn
    {
        template<typename _T, typename _U>
        constexpr auto
        operator()(
            _T&& _t,
            _U&& _u
        ) const
            noexcept(noexcept(_weak_order_cpo::impl(
                _weak_order_cpo::priority<2>{},
                static_cast<_T&&>(_t),
                static_cast<_U&&>(_u))))
            -> decltype(_weak_order_cpo::impl(
                _weak_order_cpo::priority<2>{},
                static_cast<_T&&>(_t),
                static_cast<_U&&>(_u)))
        {
            return _weak_order_cpo::impl(
                _weak_order_cpo::priority<2>{},
                static_cast<_T&&>(_t),
                static_cast<_U&&>(_u));
        }
    };

}  // namespace _weak_order_cpo_obj


inline constexpr _weak_order_cpo_obj::weak_order_fn weak_order = {};


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


#endif  // DJINTERP_RESTD_COMPARE_WEAK_ORDER_
