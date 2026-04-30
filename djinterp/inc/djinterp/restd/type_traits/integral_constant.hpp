/******************************************************************************
* djinterp [restd]                                       integral_constant.hpp
*
* integral_constant trait header:
*   Wraps a compile-time constant of arithmetic type. Base class for all
* boolean traits in restd. Provides:
*   - static const _Type value         - the wrapped constant.
*   - typedef _Type     value_type     - the constant's type.
*   - typedef integral_constant<_Type, _Value> type
*                                      - identity typedef.
*   - operator value_type() const      - implicit conversion to value.
*   - value_type operator()() const    - call operator (C++14+).
*
*   PORTABILITY:
*   - C++98/03: `static const`, out-of-class definition for ODR safety
*     when ::value is ODR-used (e.g. taken by reference).
*   - C++11+:   `static D_CONSTEXPR`, with implicit conversion and call
*     operator marked D_CONSTEXPR / D_NOEXCEPT.
*   - C++17+:   `static D_CONSTEXPR` is implicitly inline; out-of-class
*     definition becomes redundant but harmless.
*
*
* path:      /inc/djinterp/restd/type_traits/integral_constant.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_INTEGRAL_CONSTANT_
#define DJINTERP_RESTD_TYPE_TRAITS_INTEGRAL_CONSTANT_ 1

// djinterp
#include "../../core/djinterp.hpp"


NS_RESTD


// =============================================================================
// I.   INTEGRAL_CONSTANT
// =============================================================================

// integral_constant
//   trait: wraps a compile-time constant of arithmetic type _Type with
// value _Value. Foundation type for all boolean traits.
template<typename _Type,
         _Type    _Value>
struct integral_constant
{
    typedef _Type                                value_type;
    typedef integral_constant<_Type, _Value>     type;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    static D_CONSTEXPR _Type value = _Value;

    D_CONSTEXPR
    operator value_type() const D_NOEXCEPT
    {
        return value;
    }

    D_CONSTEXPR value_type
    operator()() const D_NOEXCEPT
    {
        return value;
    }
#else
    static const _Type value;

    operator value_type() const
    {
        return value;
    }
#endif
};


// =============================================================================
// II.  OUT-OF-CLASS DEFINITION OF ::value
// =============================================================================
// Required for ODR-safety on C++98/03 when ::value is taken by reference
// or address. Harmless on C++11+/C++17+ where inline variables make it
// redundant.

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    template<typename _Type,
             _Type    _Value>
    D_CONSTEXPR _Type integral_constant<_Type, _Value>::value;

#else

    template<typename _Type,
             _Type    _Value>
    const _Type integral_constant<_Type, _Value>::value = _Value;

#endif


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_INTEGRAL_CONSTANT_
