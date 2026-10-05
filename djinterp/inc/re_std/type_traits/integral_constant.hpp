/*******************************************************************************
* djinterp [re_std]                                        integral_constant.hpp
*
* integral_constant trait header:
*   Wraps a compile-time constant of arithmetic type. Base class for all
* boolean traits in re_std. Provides:
*   - static const Type value         - the wrapped constant.
*   - typedef Type     value_type     - the constant's type.
*   - typedef integral_constant<Type, Value> type
*                                      - identity typedef.
*   - operator value_type() const      - implicit conversion to value.
*   - value_type operator()() const    - call operator (C++14+).
*
*   PORTABILITY:
*   - C++98/03: `static const`, out-of-class definition for ODR safety
*     when ::value is ODR-used (e.g. taken by reference).
*   - C++11+:   `static RE_STD_CONSTEXPR`, with implicit conversion and call
*     operator marked RE_STD_CONSTEXPR / RE_STD_NOEXCEPT.
*   - C++17+:   `static RE_STD_CONSTEXPR` is implicitly inline; out-of-class
*     definition becomes redundant but harmless.
*
*
* path:      /inc/re_std/type_traits/integral_constant.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_INTEGRAL_CONSTANT_HPP
#define RE_STD_TYPE_TRAITS_INTEGRAL_CONSTANT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


namespace re_std
{


// =============================================================================
// I.   INTEGRAL_CONSTANT
// =============================================================================

// integral_constant
//   trait: wraps a compile-time constant of arithmetic type Type with
// value Value. Foundation type for all boolean traits.
template<typename Type,
         Type    Value>
struct integral_constant
{
    typedef Type                                value_type;
    typedef integral_constant<Type, Value>     type;

#if RE_STD_LANG_IS_CPP11_OR_HIGHER
    static RE_STD_CONSTEXPR Type value = Value;

    RE_STD_CONSTEXPR
    operator value_type() const RE_STD_NOEXCEPT
    {
        return value;
    }

    RE_STD_CONSTEXPR value_type
    operator()() const RE_STD_NOEXCEPT
    {
        return value;
    }
#else
    static const Type value;

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

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    template<typename Type,
             Type    Value>
    RE_STD_CONSTEXPR Type integral_constant<Type, Value>::value;

#else

    template<typename Type,
             Type    Value>
    const Type integral_constant<Type, Value>::value = Value;

#endif


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_INTEGRAL_CONSTANT_HPP
