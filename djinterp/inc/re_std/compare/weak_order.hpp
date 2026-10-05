/*******************************************************************************
* djinterp [re_std]                                               weak_order.hpp
*
* weak_order customisation point object header:
*   Per [cmp.alg]: a niebloid that produces a weak_ordering result
* for the inputs. Dispatch hierarchy:
*
*     (a) Floating-point special case — NOT YET SHIPPED in re_std
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
*   Definition gated on RE_STD_LANG_IS_CPP20_OR_HIGHER.
*
*
* path:      /inc/re_std/compare/weak_order.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.17
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_COMPARE_WEAK_ORDER_HPP
#define RE_STD_COMPARE_WEAK_ORDER_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP20_OR_HIGHER


// re_std
#include "./weak_ordering.hpp"


namespace re_std
{


namespace _weak_order_cpo
{

    template<int N>
    struct priority : priority<N - 1> {};
    template<>
    struct priority<0> {};

    void weak_order() = delete;

    // (b) ADL path
    template<typename T, typename U>
    constexpr auto
    impl(
        priority<2>,
        T&& _t,
        U&& _u
    ) noexcept(noexcept(weak_ordering(weak_order(
                  static_cast<T&&>(_t), static_cast<U&&>(_u)))))
        -> decltype(weak_ordering(weak_order(
                        static_cast<T&&>(_t), static_cast<U&&>(_u))))
    {
        return weak_ordering(weak_order(
                   static_cast<T&&>(_t), static_cast<U&&>(_u)));
    }

    // (c) Built-in <=> path
    template<typename T, typename U>
    constexpr auto
    impl(
        priority<1>,
        T&& _t,
        U&& _u
    ) noexcept(noexcept(weak_ordering(
                  static_cast<T&&>(_t) <=> static_cast<U&&>(_u))))
        -> decltype(weak_ordering(
                        static_cast<T&&>(_t) <=> static_cast<U&&>(_u)))
    {
        return weak_ordering(
                   static_cast<T&&>(_t) <=> static_cast<U&&>(_u));
    }

}  // namespace _weak_order_cpo


namespace _weak_order_cpo_obj
{

    struct weak_order_fn
    {
        template<typename T, typename U>
        constexpr auto
        operator()(
            T&& _t,
            U&& _u
        ) const
            noexcept(noexcept(_weak_order_cpo::impl(
                _weak_order_cpo::priority<2>{},
                static_cast<T&&>(_t),
                static_cast<U&&>(_u))))
            -> decltype(_weak_order_cpo::impl(
                _weak_order_cpo::priority<2>{},
                static_cast<T&&>(_t),
                static_cast<U&&>(_u)))
        {
            return _weak_order_cpo::impl(
                _weak_order_cpo::priority<2>{},
                static_cast<T&&>(_t),
                static_cast<U&&>(_u));
        }
    };

}  // namespace _weak_order_cpo_obj


inline constexpr _weak_order_cpo_obj::weak_order_fn weak_order = {};


}  // re_std


#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER


#endif  // RE_STD_COMPARE_WEAK_ORDER_HPP
