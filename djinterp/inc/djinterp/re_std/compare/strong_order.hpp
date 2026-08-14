/******************************************************************************
* djinterp [restd]                                                 strong_order.hpp
*
* strong_order customisation point object header:
*   Per [cmp.alg]: a niebloid that produces a strong_ordering result
* for the inputs. Dispatch hierarchy (from most-specific to least):
*
*     (a) Floating-point special case — NOT YET SHIPPED in restd
*         (see PORTABILITY below).
*     (b) ADL strong_order(t, u) returning a value that constructs
*         strong_ordering.
*     (c) Built-in t <=> u with result castable to strong_ordering.
*     (d) Otherwise: ill-formed.
*
*   The std-mandated FP special case yields a strong_ordering result
* even for floating-point inputs that compare unordered (NaN), by
* imposing a deterministic total order on the bit-pattern. Restd
* defers this to a future C5 phase — currently FP inputs route through
* path (c), which returns the partial_ordering from built-in <=>;
* that partial_ordering will fail the implicit conversion to
* strong_ordering when the value is `unordered`, surfacing as a
* compile error or runtime mismatch. Users wanting FP strong_order
* on restd should guard against NaN inputs themselves.
*
*   IMPLEMENTATION:
*   Mirrors the priority<N> + ADL-poison-pill pattern used by the
* ranges:: niebloids (Phase R24). The poison-pill `strong_order` in
* the dispatch namespace forces ADL discovery; user types may
* customise via a friend or namespace-scope strong_order.
*
*   PORTABILITY:
*   Definition gated on D_ENV_LANG_IS_CPP20_OR_HIGHER — the call
* operator's body requires operator<=> at parse time.
*
*
* path:      /inc/djinterp/re_std/compare/strong_order.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.17
******************************************************************************/

#ifndef DJINTERP_RESTD_COMPARE_STRONG_ORDER_
#define DJINTERP_RESTD_COMPARE_STRONG_ORDER_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER


// djinterp
#include "./strong_ordering.hpp"


NS_RESTD


// =============================================================================
// I.   INTERNAL: PRIORITY-DISPATCH + POISON PILL
// =============================================================================

namespace _strong_order_cpo
{

    // priority — same pattern as the ranges:: CPOs in Phase R24.
    // Higher N = more-specific overload preferred via the cast
    // priority<N>{} → priority<N-1>{} chain.
    template<int _N>
    struct priority
        : priority<_N - 1>
    {};

    template<>
    struct priority<0>
    {};

    // Poison-pill — forces ADL lookup for strong_order. Without
    // this, an unqualified strong_order(t, u) inside the CPO body
    // would find the CPO itself recursively.
    void strong_order() = delete;

    // (b) ADL path: prefer user-provided strong_order(t, u).
    template<typename _T, typename _U>
    constexpr auto
    impl(
        priority<2>,
        _T&& _t,
        _U&& _u
    ) noexcept(noexcept(strong_ordering(strong_order(
                  static_cast<_T&&>(_t), static_cast<_U&&>(_u)))))
        -> decltype(strong_ordering(strong_order(
                        static_cast<_T&&>(_t), static_cast<_U&&>(_u))))
    {
        return strong_ordering(strong_order(
                   static_cast<_T&&>(_t), static_cast<_U&&>(_u)));
    }

    // (c) Built-in <=> path.
    template<typename _T, typename _U>
    constexpr auto
    impl(
        priority<1>,
        _T&& _t,
        _U&& _u
    ) noexcept(noexcept(strong_ordering(
                  static_cast<_T&&>(_t) <=> static_cast<_U&&>(_u))))
        -> decltype(strong_ordering(
                        static_cast<_T&&>(_t) <=> static_cast<_U&&>(_u)))
    {
        return strong_ordering(
                   static_cast<_T&&>(_t) <=> static_cast<_U&&>(_u));
    }

}  // namespace _strong_order_cpo


// =============================================================================
// II.  STRONG_ORDER NIEBLOID
// =============================================================================

namespace _strong_order_cpo_obj
{

    struct strong_order_fn
    {
        template<typename _T, typename _U>
        constexpr auto
        operator()(
            _T&& _t,
            _U&& _u
        ) const
            noexcept(noexcept(_strong_order_cpo::impl(
                _strong_order_cpo::priority<2>{},
                static_cast<_T&&>(_t),
                static_cast<_U&&>(_u))))
            -> decltype(_strong_order_cpo::impl(
                _strong_order_cpo::priority<2>{},
                static_cast<_T&&>(_t),
                static_cast<_U&&>(_u)))
        {
            return _strong_order_cpo::impl(
                _strong_order_cpo::priority<2>{},
                static_cast<_T&&>(_t),
                static_cast<_U&&>(_u));
        }
    };

}  // namespace _strong_order_cpo_obj


// The niebloid instance. Inline-constexpr on C++17+, which is
// already implied by the C++20 outer gate.
inline constexpr _strong_order_cpo_obj::strong_order_fn strong_order = {};


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


#endif  // DJINTERP_RESTD_COMPARE_STRONG_ORDER_
