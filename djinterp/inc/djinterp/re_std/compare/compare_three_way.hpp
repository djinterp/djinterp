/******************************************************************************
* djinterp [restd]                                          compare_three_way.hpp
*
* compare_three_way function object header:
*   Per [cmp.object]: a stateless function object whose call operator
* returns `t <=> u`. The trailing-return-decltype gives the result
* the same type as the underlying three-way comparison; the noexcept
* specifier propagates from that expression.
*
*     compare_three_way{}(3, 5)    -> strong_ordering::less
*     compare_three_way{}(1.0, NAN) -> partial_ordering::unordered
*
*   Comparable to compare_three_way is used by ranges algorithms as
* a default comparator that yields three-way category results when
* the underlying operator<=> is available.
*
*   PORTABILITY:
*   The entire class definition is gated on D_ENV_LANG_IS_CPP20_OR_HIGHER
* — the call operator's body requires the operator<=> language
* feature. On C++11-17 the symbol does not exist. Users on lower
* tiers should guard with the same macro before referencing it.
*
*
* path:      /inc/djinterp/re_std/compare/compare_three_way.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.17
******************************************************************************/

#ifndef DJINTERP_RESTD_COMPARE_COMPARE_THREE_WAY_
#define DJINTERP_RESTD_COMPARE_COMPARE_THREE_WAY_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER


NS_RESTD


// =============================================================================
// I.   COMPARE_THREE_WAY
// =============================================================================

// compare_three_way
//   class: stateless function object wrapping operator<=>. Per the
// standard, the call operator is constexpr and noexcept-propagating.
// The trailing return uses decltype on the forwarded expression so
// the return type is whatever the underlying <=> yields (one of the
// three ordering categories, typically).
struct compare_three_way
{
    template<typename _T,
             typename _U>
    constexpr auto
    operator()(
        _T&& _t,
        _U&& _u
    ) const
        noexcept(noexcept(static_cast<_T&&>(_t) <=> static_cast<_U&&>(_u)))
        -> decltype(static_cast<_T&&>(_t) <=> static_cast<_U&&>(_u))
    {
        return static_cast<_T&&>(_t) <=> static_cast<_U&&>(_u);
    }

    // is_transparent — marker presence allows compare_three_way to
    // be used as a heterogeneous comparator with associative
    // containers (per the standard).
    typedef void is_transparent;
};


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


#endif  // DJINTERP_RESTD_COMPARE_COMPARE_THREE_WAY_
