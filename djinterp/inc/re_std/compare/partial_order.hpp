/*******************************************************************************
* djinterp [re_std]                                            partial_order.hpp
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
*   Definition gated on RE_STD_LANG_IS_CPP20_OR_HIGHER.
*
*
* path:      /inc/re_std/compare/partial_order.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.17
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_COMPARE_PARTIAL_ORDER_HPP
#define RE_STD_COMPARE_PARTIAL_ORDER_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP20_OR_HIGHER


// re_std
#include "./partial_ordering.hpp"


namespace re_std
{


namespace _partial_order_cpo
{

    template<int N>
    struct priority : priority<N - 1> {};
    template<>
    struct priority<0> {};

    void partial_order() = delete;

    // (b) ADL path
    template<typename T, typename U>
    constexpr auto
    impl(
        priority<2>,
        T&& _t,
        U&& _u
    ) noexcept(noexcept(partial_ordering(partial_order(
                  static_cast<T&&>(_t), static_cast<U&&>(_u)))))
        -> decltype(partial_ordering(partial_order(
                        static_cast<T&&>(_t), static_cast<U&&>(_u))))
    {
        return partial_ordering(partial_order(
                   static_cast<T&&>(_t), static_cast<U&&>(_u)));
    }

    // (c) Built-in <=> path
    template<typename T, typename U>
    constexpr auto
    impl(
        priority<1>,
        T&& _t,
        U&& _u
    ) noexcept(noexcept(partial_ordering(
                  static_cast<T&&>(_t) <=> static_cast<U&&>(_u))))
        -> decltype(partial_ordering(
                        static_cast<T&&>(_t) <=> static_cast<U&&>(_u)))
    {
        return partial_ordering(
                   static_cast<T&&>(_t) <=> static_cast<U&&>(_u));
    }

}  // namespace _partial_order_cpo


namespace _partial_order_cpo_obj
{

    struct partial_order_fn
    {
        template<typename T, typename U>
        constexpr auto
        operator()(
            T&& _t,
            U&& _u
        ) const
            noexcept(noexcept(_partial_order_cpo::impl(
                _partial_order_cpo::priority<2>{},
                static_cast<T&&>(_t),
                static_cast<U&&>(_u))))
            -> decltype(_partial_order_cpo::impl(
                _partial_order_cpo::priority<2>{},
                static_cast<T&&>(_t),
                static_cast<U&&>(_u)))
        {
            return _partial_order_cpo::impl(
                _partial_order_cpo::priority<2>{},
                static_cast<T&&>(_t),
                static_cast<U&&>(_u));
        }
    };

}  // namespace _partial_order_cpo_obj


inline constexpr _partial_order_cpo_obj::partial_order_fn partial_order = {};


}  // re_std


#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER


#endif  // RE_STD_COMPARE_PARTIAL_ORDER_HPP
