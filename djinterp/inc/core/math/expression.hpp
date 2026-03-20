/******************************************************************************
* djinterp [maths]                                               expression.hpp
*
* Compile-time mathematical expression representation.
*   Provides template types for building complex mathematical expressions
*   through composition. All expression types can be nested arbitrarily.
*
* EXPRESSION HIERARCHY:
*   expression (base concept - anything evaluable)
*   ├── constant<T, V>           - literal value (42, 3.14)
*   ├── variable<T, ID>          - independent variable (x, y, z)
*   ├── term<Coeff, Var, Exp>    - coefficient * variable^exponent (4x³)
*   ├── polynomial<Terms...>     - sum of terms (4x³ + 2x - 1)
*   ├── binary_op<Op, L, R>      - binary operation (L op R)
*   │   ├── sum<L, R>            - addition
*   │   ├── difference<L, R>     - subtraction  
*   │   ├── product<L, R>        - multiplication
*   │   ├── quotient<L, R>       - division
*   │   └── power<Base, Exp>     - exponentiation
*   ├── unary_op<Op, Arg>        - unary operation
*   │   ├── negate<Arg>          - negation (-x)
*   │   └── reciprocal<Arg>      - reciprocal (1/x)
*   ├── function<F, Arg>         - function application (ln(x), sin(x))
*   └── rational_function<P, Q>  - ratio of polynomials (P(x)/Q(x))
*
* DESIGN PRINCIPLES:
*   1. Everything is an expression (can be evaluated)
*   2. Expressions compose (expressions contain expressions)
*   3. Structural SFINAE detection (no tags)
*   4. Compile-time evaluation where possible
*
* path:      \inc\maths\expression.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2024.04.24
******************************************************************************/

#ifndef DJINTERP_MATHS_EXPRESSION_
#define DJINTERP_MATHS_EXPRESSION_ 1

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <type_traits>
#include "..\env.h"
#include "..\cpp_features.h"
#include "..\djinterp.h"


NS_DJINTERP
NS_MATHS

// =============================================================================
// I.    FORWARD DECLARATIONS
// =============================================================================

template<typename _ValueType, _ValueType _Value>
struct constant;

template<typename _ValueType, std::size_t _ID>
struct variable;

template<typename _Coefficient, typename _Variable, typename _Exponent>
struct term;

template<typename... _Terms>
struct polynomial;


// =============================================================================
// II.   EXPRESSION BASE (CRTP)
// =============================================================================

// expression_base
//   struct: CRTP base for all expression types.
// Provides common interface and enables SFINAE detection.
template<typename _Derived>
struct expression_base
{
    using derived_type = _Derived;

    constexpr const _Derived& self() const noexcept
    {
        return static_cast<const _Derived&>(*this);
    }
};


// =============================================================================
// III.  CONSTANT EXPRESSION
// =============================================================================

// constant
//   struct: compile-time constant value expression.
// Examples: constant<int, 42>, constant<double, 0>
template<typename   _ValueType,
         _ValueType _Value>
struct constant : expression_base<constant<_ValueType, _Value>>
{
    using self_type  = constant<_ValueType, _Value>;
    using value_type = _ValueType;

    static constexpr value_type value = _Value;

    // expression properties
    static constexpr std::size_t arity      = 0;  // no variables
    static constexpr std::size_t degree     = 0;  // constant = degree 0
    static constexpr bool        is_constant_expr = true;

    template<typename _InputType>
    static constexpr value_type evaluate(_InputType) noexcept
    {
        return value;
    }

    // derivative of constant is zero
    using derivative = constant<value_type, static_cast<value_type>(0)>;
};

// Convenience aliases
template<typename _T = int>
using zero_constant = constant<_T, static_cast<_T>(0)>;

template<typename _T = int>
using one_constant = constant<_T, static_cast<_T>(1)>;


// =============================================================================
// IV.   VARIABLE EXPRESSION
// =============================================================================

// variable
//   struct: independent variable expression.
// _ID allows multiple distinct variables (x=0, y=1, z=2, etc.)
// Examples: variable<double, 0> for x, variable<double, 1> for y
template<typename   _ValueType,
         std::size_t _ID = 0>
struct variable : expression_base<variable<_ValueType, _ID>>
{
    using self_type  = variable<_ValueType, _ID>;
    using value_type = _ValueType;

    static constexpr std::size_t id = _ID;

    // expression properties
    static constexpr std::size_t arity      = 1;
    static constexpr std::size_t degree     = 1;  // x = degree 1
    static constexpr bool        is_constant_expr = false;

    template<typename _InputType>
    static constexpr value_type evaluate(_InputType _x) noexcept
    {
        return static_cast<value_type>(_x);
    }

    // derivative of x is 1
    using derivative = constant<value_type, static_cast<value_type>(1)>;
};

// Convenience aliases for common variables
template<typename _T = double>
using var_x = variable<_T, 0>;

template<typename _T = double>
using var_y = variable<_T, 1>;

template<typename _T = double>
using var_z = variable<_T, 2>;

template<typename _T = double>
using var_t = variable<_T, 3>;  // often used for time/parameter


// =============================================================================
// V.    BINARY OPERATIONS
// =============================================================================

NS_INTERNAL

    // Operation tags for binary operations
    struct op_add {};
    struct op_subtract {};
    struct op_multiply {};
    struct op_divide {};
    struct op_power {};

NS_END  // internal

// binary_op
//   struct: generic binary operation on two expressions.
template<typename _Op,
         typename _Left,
         typename _Right>
struct binary_op : expression_base<binary_op<_Op, _Left, _Right>>
{
    using self_type  = binary_op<_Op, _Left, _Right>;
    using left_type  = _Left;
    using right_type = _Right;
    using op_type    = _Op;
    using value_type = std::common_type_t<
        typename _Left::value_type,
        typename _Right::value_type
    >;

    // expression properties
    static constexpr std::size_t arity =
        (_Left::arity > _Right::arity) ? _Left::arity : _Right::arity;
    static constexpr bool is_constant_expr =
        _Left::is_constant_expr && _Right::is_constant_expr;
};

// sum
//   struct: addition of two expressions (L + R).
template<typename _Left,
         typename _Right>
struct sum : binary_op<internal::op_add, _Left, _Right>
{
    using base_type  = binary_op<internal::op_add, _Left, _Right>;
    using value_type = typename base_type::value_type;

    // degree of sum is max of operand degrees
    static constexpr std::size_t degree =
        (_Left::degree > _Right::degree) ? _Left::degree : _Right::degree;

    template<typename _InputType>
    static constexpr value_type evaluate(_InputType _x) noexcept
    {
        return _Left::evaluate(_x) + _Right::evaluate(_x);
    }

    // d/dx(f + g) = f' + g'
    using derivative = sum<typename _Left::derivative, typename _Right::derivative>;
};

// difference
//   struct: subtraction of two expressions (L - R).
template<typename _Left,
         typename _Right>
struct difference : binary_op<internal::op_subtract, _Left, _Right>
{
    using base_type  = binary_op<internal::op_subtract, _Left, _Right>;
    using value_type = typename base_type::value_type;

    static constexpr std::size_t degree =
        (_Left::degree > _Right::degree) ? _Left::degree : _Right::degree;

    template<typename _InputType>
    static constexpr value_type evaluate(_InputType _x) noexcept
    {
        return _Left::evaluate(_x) - _Right::evaluate(_x);
    }

    // d/dx(f - g) = f' - g'
    using derivative = difference<typename _Left::derivative, typename _Right::derivative>;
};

// product
//   struct: multiplication of two expressions (L * R).
template<typename _Left,
         typename _Right>
struct product : binary_op<internal::op_multiply, _Left, _Right>
{
    using base_type  = binary_op<internal::op_multiply, _Left, _Right>;
    using value_type = typename base_type::value_type;

    // degree of product is sum of operand degrees
    static constexpr std::size_t degree = _Left::degree + _Right::degree;

    template<typename _InputType>
    static constexpr value_type evaluate(_InputType _x) noexcept
    {
        return _Left::evaluate(_x) * _Right::evaluate(_x);
    }

    // d/dx(f * g) = f' * g + f * g'  (product rule)
    using derivative = sum<
        product<typename _Left::derivative, _Right>,
        product<_Left, typename _Right::derivative>
    >;
};

// quotient
//   struct: division of two expressions (L / R).
template<typename _Left,
         typename _Right>
struct quotient : binary_op<internal::op_divide, _Left, _Right>
{
    using base_type  = binary_op<internal::op_divide, _Left, _Right>;
    using value_type = typename base_type::value_type;

    // degree is undefined for general quotients; use 0 as placeholder
    static constexpr std::size_t degree = 0;

    template<typename _InputType>
    static constexpr value_type evaluate(_InputType _x) noexcept
    {
        return _Left::evaluate(_x) / _Right::evaluate(_x);
    }

    // d/dx(f / g) = (f' * g - f * g') / g²  (quotient rule)
    using derivative = quotient<
        difference<
            product<typename _Left::derivative, _Right>,
            product<_Left, typename _Right::derivative>
        >,
        product<_Right, _Right>
    >;
};

// power
//   struct: exponentiation (Base ^ Exponent).
// For now, exponent must be a constant integer for compile-time evaluation.
template<typename _Base,
         typename _Exponent>
struct power : binary_op<internal::op_power, _Base, _Exponent>
{
    using base_type  = binary_op<internal::op_power, _Base, _Exponent>;
    using value_type = typename base_type::value_type;

    // degree of x^n is n * degree(x)
    static constexpr std::size_t degree = 
        _Base::degree * static_cast<std::size_t>(_Exponent::value);

    template<typename _InputType>
    static constexpr value_type evaluate(_InputType _x) noexcept
    {
        // Compile-time power for integer exponents
        if constexpr (_Exponent::value == 0)
        {
            return static_cast<value_type>(1);
        }
        else if constexpr (_Exponent::value == 1)
        {
            return _Base::evaluate(_x);
        }
        else if constexpr (_Exponent::value == 2)
        {
            auto b = _Base::evaluate(_x);
            return b * b;
        }
        else if constexpr (_Exponent::value > 0)
        {
            auto b = _Base::evaluate(_x);
            value_type result = 1;
            for (auto i = _Exponent::value; i > 0; --i)
            {
                result *= b;
            }
            return result;
        }
        else  // negative exponent
        {
            auto b = _Base::evaluate(_x);
            value_type result = 1;
            for (auto i = -_Exponent::value; i > 0; --i)
            {
                result *= b;
            }
            return static_cast<value_type>(1) / result;
        }
    }

    // d/dx(f^n) = n * f^(n-1) * f'  (power rule + chain rule)
    // Note: simplified for constant integer exponents
};

// Convenience: x^n where n is an integer
template<typename _Base, std::intmax_t _N>
using power_n = power<_Base, constant<std::intmax_t, _N>>;


// =============================================================================
// VI.   UNARY OPERATIONS
// =============================================================================

NS_INTERNAL

    struct op_negate {};
    struct op_reciprocal {};

NS_END  // internal

// unary_op
//   struct: generic unary operation on an expression.
template<typename _Op,
         typename _Arg>
struct unary_op : expression_base<unary_op<_Op, _Arg>>
{
    using self_type  = unary_op<_Op, _Arg>;
    using arg_type   = _Arg;
    using op_type    = _Op;
    using value_type = typename _Arg::value_type;

    static constexpr std::size_t arity = _Arg::arity;
    static constexpr bool is_constant_expr = _Arg::is_constant_expr;
};

// negate
//   struct: negation of an expression (-Arg).
template<typename _Arg>
struct negate : unary_op<internal::op_negate, _Arg>
{
    using value_type = typename _Arg::value_type;

    static constexpr std::size_t degree = _Arg::degree;

    template<typename _InputType>
    static constexpr value_type evaluate(_InputType _x) noexcept
    {
        return -_Arg::evaluate(_x);
    }

    // d/dx(-f) = -f'
    using derivative = negate<typename _Arg::derivative>;
};

// reciprocal
//   struct: reciprocal of an expression (1/Arg).
template<typename _Arg>
struct reciprocal : unary_op<internal::op_reciprocal, _Arg>
{
    using value_type = typename _Arg::value_type;

    static constexpr std::size_t degree = 0;  // 1/x is not a polynomial

    template<typename _InputType>
    static constexpr value_type evaluate(_InputType _x) noexcept
    {
        return static_cast<value_type>(1) / _Arg::evaluate(_x);
    }

    // d/dx(1/f) = -f' / f²
    using derivative = quotient<
        negate<typename _Arg::derivative>,
        product<_Arg, _Arg>
    >;
};


// =============================================================================
// VII.  TERM (Coefficient * Variable ^ Exponent)
// =============================================================================

// term
//   struct: a single term in a polynomial: coeff * var ^ exp.
// Examples: 
//   term<constant<int,4>, var_x<>, constant<int,3>> = 4x³
//   term<constant<int,2>, var_x<>, constant<int,1>> = 2x
//   term<constant<int,5>, var_x<>, constant<int,0>> = 5 (constant term)
template<typename _Coefficient,
         typename _Variable,
         typename _Exponent>
struct term : expression_base<term<_Coefficient, _Variable, _Exponent>>
{
    using self_type        = term<_Coefficient, _Variable, _Exponent>;
    using coefficient_type = _Coefficient;
    using variable_type    = _Variable;
    using exponent_type    = _Exponent;
    using value_type       = typename _Coefficient::value_type;

    static constexpr auto coefficient = _Coefficient::value;
    static constexpr auto exponent    = _Exponent::value;

    // expression properties
    static constexpr std::size_t degree = 
        (coefficient == 0) ? 0 : static_cast<std::size_t>(exponent);
    static constexpr std::size_t arity = (exponent == 0) ? 0 : 1;
    static constexpr bool is_constant_expr = (exponent == 0);

    template<typename _InputType>
    static constexpr value_type evaluate(_InputType _x) noexcept
    {
        if constexpr (coefficient == 0)
        {
            return static_cast<value_type>(0);
        }
        else if constexpr (exponent == 0)
        {
            return static_cast<value_type>(coefficient);
        }
        else if constexpr (exponent == 1)
        {
            return static_cast<value_type>(coefficient) * 
                   static_cast<value_type>(_x);
        }
        else
        {
            return static_cast<value_type>(coefficient) * 
                   power<_Variable, _Exponent>::evaluate(_x);
        }
    }

    // d/dx(c * x^n) = c * n * x^(n-1)
    using derivative = term<
        constant<value_type, static_cast<value_type>(coefficient * exponent)>,
        _Variable,
        constant<std::intmax_t, (exponent > 0) ? exponent - 1 : 0>
    >;
};

// Convenience: make a term with integer coefficient and exponent
template<typename _VarType, std::intmax_t _Coeff, std::intmax_t _Exp>
using make_term = term<
    constant<std::intmax_t, _Coeff>,
    _VarType,
    constant<std::intmax_t, _Exp>
>;

// Convenience: common term patterns
template<typename _T, std::intmax_t _C>
using constant_term = term<constant<_T, _C>, var_x<_T>, constant<std::intmax_t, 0>>;

template<typename _T, std::intmax_t _C>
using linear_term = term<constant<_T, _C>, var_x<_T>, constant<std::intmax_t, 1>>;

template<typename _T, std::intmax_t _C>
using quadratic_term = term<constant<_T, _C>, var_x<_T>, constant<std::intmax_t, 2>>;

template<typename _T, std::intmax_t _C>
using cubic_term = term<constant<_T, _C>, var_x<_T>, constant<std::intmax_t, 3>>;


// =============================================================================
// VIII. POLYNOMIAL (Sum of Terms)
// =============================================================================

NS_INTERNAL

    // polynomial_eval_helper
    //   helper: evaluates sum of terms.
    template<typename... _Terms>
    struct polynomial_eval_helper;

    template<typename _First, typename... _Rest>
    struct polynomial_eval_helper<_First, _Rest...>
    {
        template<typename _InputType>
        static constexpr auto evaluate(_InputType _x) noexcept
        {
            return _First::evaluate(_x) + 
                   polynomial_eval_helper<_Rest...>::evaluate(_x);
        }
    };

    template<typename _Last>
    struct polynomial_eval_helper<_Last>
    {
        template<typename _InputType>
        static constexpr auto evaluate(_InputType _x) noexcept
        {
            return _Last::evaluate(_x);
        }
    };

    template<>
    struct polynomial_eval_helper<>
    {
        template<typename _InputType>
        static constexpr int evaluate(_InputType) noexcept
        {
            return 0;
        }
    };

    // polynomial_degree_helper
    //   helper: finds maximum degree among terms.
    template<typename... _Terms>
    struct polynomial_degree_helper;

    template<typename _First, typename... _Rest>
    struct polynomial_degree_helper<_First, _Rest...>
    {
        static constexpr std::size_t rest_degree = 
            polynomial_degree_helper<_Rest...>::value;
        static constexpr std::size_t value = 
            (_First::degree > rest_degree) ? _First::degree : rest_degree;
    };

    template<typename _Last>
    struct polynomial_degree_helper<_Last>
    {
        static constexpr std::size_t value = _Last::degree;
    };

    template<>
    struct polynomial_degree_helper<>
    {
        static constexpr std::size_t value = 0;
    };

NS_END  // internal

// polynomial
//   struct: sum of terms (general polynomial expression).
// Examples:
//   polynomial<cubic_term<int,4>, linear_term<int,2>, constant_term<int,-1>>
//     = 4x³ + 2x - 1
template<typename... _Terms>
struct polynomial : expression_base<polynomial<_Terms...>>
{
    using self_type  = polynomial<_Terms...>;
    using value_type = std::common_type_t<typename _Terms::value_type...>;

    static constexpr std::size_t num_terms = sizeof...(_Terms);
    static constexpr std::size_t degree = 
        internal::polynomial_degree_helper<_Terms...>::value;
    static constexpr std::size_t arity = (degree > 0) ? 1 : 0;
    static constexpr bool is_constant_expr = (degree == 0);

    template<typename _InputType>
    static constexpr value_type evaluate(_InputType _x) noexcept
    {
        return static_cast<value_type>(
            internal::polynomial_eval_helper<_Terms...>::evaluate(_x)
        );
    }

    // derivative is polynomial of term derivatives
    using derivative = polynomial<typename _Terms::derivative...>;
};

// Convenience: specific polynomial types
template<typename _T, std::intmax_t _A, std::intmax_t _B>
using linear_poly = polynomial<
    linear_term<_T, _A>,
    constant_term<_T, _B>
>;  // ax + b

template<typename _T, std::intmax_t _A, std::intmax_t _B, std::intmax_t _C>
using quadratic_poly = polynomial<
    quadratic_term<_T, _A>,
    linear_term<_T, _B>,
    constant_term<_T, _C>
>;  // ax² + bx + c

template<typename _T, std::intmax_t _A, std::intmax_t _B, 
         std::intmax_t _C, std::intmax_t _D>
using cubic_poly = polynomial<
    cubic_term<_T, _A>,
    quadratic_term<_T, _B>,
    linear_term<_T, _C>,
    constant_term<_T, _D>
>;  // ax³ + bx² + cx + d


// =============================================================================
// IX.   FUNCTION APPLICATION
// =============================================================================

NS_INTERNAL

    // Function type tags
    struct fn_identity {};
    struct fn_ln {};
    struct fn_log10 {};
    struct fn_log2 {};
    struct fn_exp {};
    struct fn_sqrt {};
    struct fn_cbrt {};
    struct fn_sin {};
    struct fn_cos {};
    struct fn_tan {};
    struct fn_asin {};
    struct fn_acos {};
    struct fn_atan {};
    struct fn_sinh {};
    struct fn_cosh {};
    struct fn_tanh {};
    struct fn_abs {};
    struct fn_floor {};
    struct fn_ceil {};

NS_END  // internal

// function_expr
//   struct: function applied to an expression.
// f(arg) where f is a mathematical function.
template<typename _FnTag,
         typename _Arg>
struct function_expr : expression_base<function_expr<_FnTag, _Arg>>
{
    using self_type  = function_expr<_FnTag, _Arg>;
    using fn_type    = _FnTag;
    using arg_type   = _Arg;
    using value_type = double;  // transcendental functions return double

    static constexpr std::size_t arity = _Arg::arity;
    static constexpr std::size_t degree = 0;  // transcendental, not polynomial
    static constexpr bool is_constant_expr = _Arg::is_constant_expr;
};

// Specialization: ln(arg)
template<typename _Arg>
struct function_expr<internal::fn_ln, _Arg> 
    : expression_base<function_expr<internal::fn_ln, _Arg>>
{
    using arg_type   = _Arg;
    using value_type = double;

    static constexpr std::size_t arity = _Arg::arity;
    static constexpr std::size_t degree = 0;
    static constexpr bool is_constant_expr = _Arg::is_constant_expr;

    template<typename _InputType>
    static constexpr value_type evaluate(_InputType _x) noexcept
    {
        return std::log(static_cast<double>(_Arg::evaluate(_x)));
    }

    // d/dx(ln(f)) = f' / f
    using derivative = quotient<typename _Arg::derivative, _Arg>;
};

// Specialization: exp(arg)
template<typename _Arg>
struct function_expr<internal::fn_exp, _Arg>
    : expression_base<function_expr<internal::fn_exp, _Arg>>
{
    using arg_type   = _Arg;
    using value_type = double;

    static constexpr std::size_t arity = _Arg::arity;
    static constexpr std::size_t degree = 0;
    static constexpr bool is_constant_expr = _Arg::is_constant_expr;

    template<typename _InputType>
    static constexpr value_type evaluate(_InputType _x) noexcept
    {
        return std::exp(static_cast<double>(_Arg::evaluate(_x)));
    }

    // d/dx(e^f) = f' * e^f
    using derivative = product<
        typename _Arg::derivative,
        function_expr<internal::fn_exp, _Arg>
    >;
};

// Specialization: sqrt(arg)
template<typename _Arg>
struct function_expr<internal::fn_sqrt, _Arg>
    : expression_base<function_expr<internal::fn_sqrt, _Arg>>
{
    using arg_type   = _Arg;
    using value_type = double;

    static constexpr std::size_t arity = _Arg::arity;
    static constexpr std::size_t degree = 0;
    static constexpr bool is_constant_expr = _Arg::is_constant_expr;

    template<typename _InputType>
    static constexpr value_type evaluate(_InputType _x) noexcept
    {
        return std::sqrt(static_cast<double>(_Arg::evaluate(_x)));
    }

    // d/dx(√f) = f' / (2√f)
    using derivative = quotient<
        typename _Arg::derivative,
        product<
            constant<int, 2>,
            function_expr<internal::fn_sqrt, _Arg>
        >
    >;
};

// Specialization: sin(arg)
template<typename _Arg>
struct function_expr<internal::fn_sin, _Arg>
    : expression_base<function_expr<internal::fn_sin, _Arg>>
{
    using arg_type   = _Arg;
    using value_type = double;

    static constexpr std::size_t arity = _Arg::arity;
    static constexpr std::size_t degree = 0;
    static constexpr bool is_constant_expr = _Arg::is_constant_expr;

    template<typename _InputType>
    static constexpr value_type evaluate(_InputType _x) noexcept
    {
        return std::sin(static_cast<double>(_Arg::evaluate(_x)));
    }

    // d/dx(sin(f)) = f' * cos(f)
    using derivative = product<
        typename _Arg::derivative,
        function_expr<internal::fn_cos, _Arg>
    >;
};

// Specialization: cos(arg)
template<typename _Arg>
struct function_expr<internal::fn_cos, _Arg>
    : expression_base<function_expr<internal::fn_cos, _Arg>>
{
    using arg_type   = _Arg;
    using value_type = double;

    static constexpr std::size_t arity = _Arg::arity;
    static constexpr std::size_t degree = 0;
    static constexpr bool is_constant_expr = _Arg::is_constant_expr;

    template<typename _InputType>
    static constexpr value_type evaluate(_InputType _x) noexcept
    {
        return std::cos(static_cast<double>(_Arg::evaluate(_x)));
    }

    // d/dx(cos(f)) = -f' * sin(f)
    using derivative = negate<product<
        typename _Arg::derivative,
        function_expr<internal::fn_sin, _Arg>
    >>;
};

// Specialization: tan(arg)
template<typename _Arg>
struct function_expr<internal::fn_tan, _Arg>
    : expression_base<function_expr<internal::fn_tan, _Arg>>
{
    using arg_type   = _Arg;
    using value_type = double;

    static constexpr std::size_t arity = _Arg::arity;
    static constexpr std::size_t degree = 0;
    static constexpr bool is_constant_expr = _Arg::is_constant_expr;

    template<typename _InputType>
    static constexpr value_type evaluate(_InputType _x) noexcept
    {
        return std::tan(static_cast<double>(_Arg::evaluate(_x)));
    }

    // d/dx(tan(f)) = f' * sec²(f) = f' / cos²(f)
    using derivative = quotient<
        typename _Arg::derivative,
        product<
            function_expr<internal::fn_cos, _Arg>,
            function_expr<internal::fn_cos, _Arg>
        >
    >;
};

// Convenience aliases for function application
template<typename _Arg>
using ln = function_expr<internal::fn_ln, _Arg>;

template<typename _Arg>
using exp_fn = function_expr<internal::fn_exp, _Arg>;

template<typename _Arg>
using sqrt_fn = function_expr<internal::fn_sqrt, _Arg>;

template<typename _Arg>
using sin_fn = function_expr<internal::fn_sin, _Arg>;

template<typename _Arg>
using cos_fn = function_expr<internal::fn_cos, _Arg>;

template<typename _Arg>
using tan_fn = function_expr<internal::fn_tan, _Arg>;


// =============================================================================
// X.    RATIONAL FUNCTION (P(x) / Q(x))
// =============================================================================

// rational_function
//   struct: ratio of two polynomial expressions.
// Examples:
//   rational_function<linear_poly<int,1,0>, quadratic_poly<int,1,0,1>>
//     = x / (x² + 1)
template<typename _Numerator,
         typename _Denominator>
struct rational_function 
    : expression_base<rational_function<_Numerator, _Denominator>>
{
    using self_type       = rational_function<_Numerator, _Denominator>;
    using numerator_type  = _Numerator;
    using denominator_type = _Denominator;
    using value_type      = std::common_type_t<
        typename _Numerator::value_type,
        typename _Denominator::value_type
    >;

    static constexpr std::size_t arity = 
        (_Numerator::arity > _Denominator::arity) 
            ? _Numerator::arity 
            : _Denominator::arity;

    // Degree of rational function (can be negative)
    static constexpr std::intmax_t degree_diff = 
        static_cast<std::intmax_t>(_Numerator::degree) - 
        static_cast<std::intmax_t>(_Denominator::degree);

    static constexpr std::size_t degree = 0;  // not a polynomial
    static constexpr bool is_constant_expr = 
        _Numerator::is_constant_expr && _Denominator::is_constant_expr;

    template<typename _InputType>
    static constexpr value_type evaluate(_InputType _x) noexcept
    {
        return static_cast<value_type>(_Numerator::evaluate(_x)) /
               static_cast<value_type>(_Denominator::evaluate(_x));
    }

    // d/dx(P/Q) = (P'Q - PQ') / Q²  (quotient rule)
    using derivative = rational_function<
        difference<
            product<typename _Numerator::derivative, _Denominator>,
            product<_Numerator, typename _Denominator::derivative>
        >,
        product<_Denominator, _Denominator>
    >;
};


// =============================================================================
// XI.   EXPRESSION COMPOSITION HELPERS
// =============================================================================

// compose
//   type: f(g(x)) - compose two expressions.
template<typename _Outer,
         typename _Inner>
struct compose : expression_base<compose<_Outer, _Inner>>
{
    using self_type  = compose<_Outer, _Inner>;
    using outer_type = _Outer;
    using inner_type = _Inner;
    using value_type = typename _Outer::value_type;

    static constexpr std::size_t arity = _Inner::arity;
    static constexpr std::size_t degree = _Outer::degree * _Inner::degree;
    static constexpr bool is_constant_expr = 
        _Outer::is_constant_expr || _Inner::is_constant_expr;

    template<typename _InputType>
    static constexpr value_type evaluate(_InputType _x) noexcept
    {
        return _Outer::evaluate(_Inner::evaluate(_x));
    }

    // d/dx(f(g(x))) = f'(g(x)) * g'(x)  (chain rule)
    // Note: this is a simplified version; full implementation would substitute
};


NS_END  // maths
NS_END  // djinterp


#endif  // DJINTERP_MATHS_EXPRESSION_
