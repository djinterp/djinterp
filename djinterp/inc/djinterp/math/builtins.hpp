/******************************************************************************
* djinterp [math]                                                 builtins.hpp
*
* Compile-time built-in simple function types.
*   Harvested from the former math.hpp during consolidation. Provides
* small, self-contained function types (linear, step, sign, absolute
* value) modeling the expression protocol from expression.hpp so they
* compose into expression trees.
*
* NOTE (overlap):
*   linear<T, Slope, Intercept> overlaps conceptually with the polynomial
* form linear_poly<T, A, B> in expression.hpp. Both are retained: linear
* is the closed named form with slope/intercept accessors; linear_poly is
* the term-based tree form. Reconcile or alias if a single form is wanted.
*
* NOTE (differentiability):
*   step_function, sign_function, and abs_function are not differentiable
* everywhere, so no `derivative` typedef is provided for them.
*
* 
* path:      /inc/djinterp/math/builtins.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.02.04
******************************************************************************/

#ifndef DJINTERP_MATH_BUILTINS_
#define DJINTERP_MATH_BUILTINS_ 1

// std
#include <cstddef>
// djinterp
#include "../../core/djinterp.hpp"
#include "./math.hpp"
#include "./expression.hpp"


NS_DJINTERP  // djinterp
NS_MATH      // math

// ===========================================================================
// I.   Linear Function
// ===========================================================================

// linear
//   struct: linear function y = mx + b as an expression node.
template<typename  _ValueType,
         _ValueType _Slope,
         _ValueType _Intercept = static_cast<_ValueType>(0)>
struct linear : expression_base<linear<_ValueType, _Slope, _Intercept>>
{
    using value_type = _ValueType;

    static constexpr value_type  slope            = _Slope;
    static constexpr value_type  intercept        = _Intercept;
    static constexpr std::size_t degree           = (_Slope == 0) ? 0 : 1;
    static constexpr std::size_t arity            = 1;
    static constexpr bool        is_constant_expr = (_Slope == 0);

    template<typename _InputType>
    static constexpr value_type
    evaluate(_InputType _x) noexcept
    {
        return ( static_cast<value_type>(_Slope) *
                 static_cast<value_type>(_x) +
                 static_cast<value_type>(_Intercept) );
    }

    using derivative = constant<value_type, _Slope>;
};

// slope_only
//   type: linear function through the origin (y = mx).
template<typename _Type,
         _Type    _Slope>
using slope_only = linear<_Type, _Slope, static_cast<_Type>(0)>;


// ===========================================================================
// II.  Step Function
// ===========================================================================

// step_function
//   struct: Heaviside step function as an expression node.
template<typename  _ValueType,
         _ValueType _Threshold = static_cast<_ValueType>(0)>
struct step_function : expression_base<step_function<_ValueType, _Threshold>>
{
    using value_type = _ValueType;

    static constexpr value_type  threshold        = _Threshold;
    static constexpr std::size_t arity            = 1;
    static constexpr std::size_t degree           = 0;
    static constexpr bool        is_constant_expr = false;

    template<typename _InputType>
    static constexpr value_type
    evaluate(_InputType _x) noexcept
    {
        return (static_cast<value_type>(_x) >= threshold)
            ? static_cast<value_type>(1)
            : static_cast<value_type>(0);
    }
};

// step
//   type: convenience alias for a step function at the origin.
template<typename _Type = int>
using step = step_function<_Type, static_cast<_Type>(0)>;


// ===========================================================================
// III. Sign Function
// ===========================================================================

// sign_function
//   struct: signum function as an expression node.
template<typename _ValueType>
struct sign_function : expression_base<sign_function<_ValueType>>
{
    using value_type = _ValueType;

    static constexpr std::size_t arity            = 1;
    static constexpr std::size_t degree           = 0;
    static constexpr bool        is_constant_expr = false;

    template<typename _InputType>
    static constexpr value_type
    evaluate(_InputType _x) noexcept
    {
        value_type val = static_cast<value_type>(_x);

        // positive / negative / zero
        if (val < static_cast<value_type>(0))
        {
            return static_cast<value_type>(-1);
        }

        if (val > static_cast<value_type>(0))
        {
            return static_cast<value_type>(1);
        }

        return static_cast<value_type>(0);
    }
};

// sign
//   type: convenience alias for sign_function.
template<typename _Type = int>
using sign = sign_function<_Type>;


// ===========================================================================
// IV.  Absolute Value Function
// ===========================================================================

// abs_function
//   struct: absolute value function as an expression node.
template<typename _ValueType>
struct abs_function : expression_base<abs_function<_ValueType>>
{
    using value_type = _ValueType;

    static constexpr std::size_t arity            = 1;
    static constexpr std::size_t degree           = 0;
    static constexpr bool        is_constant_expr = false;

    template<typename _InputType>
    static constexpr value_type
    evaluate(_InputType _x) noexcept
    {
        value_type val = static_cast<value_type>(_x);

        return (val < static_cast<value_type>(0)) ? -val : val;
    }
};

// absolute
//   type: convenience alias for abs_function.
template<typename _Type = int>
using absolute = abs_function<_Type>;


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_BUILTINS_
