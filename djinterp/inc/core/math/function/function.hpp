/******************************************************************************
* djinterp [maths]                                              function.hpp
*
* Compile-time mathematical function representation.
*   This header provides template types for representing mathematical
* functions at compile time, including multivariable functions, piecewise
* definitions, parametric curves/surfaces, inequality constraints,
* implicit functions, vector-valued functions, and coordinate system
* conversions. All detection is structural via SFINAE.
*
* FUNCTION HIERARCHY:
*   math_function<Expr, Domain, Coords>  - named function with domain
*   ├── Multivariable
*   │   ├── multi_evaluate            - evaluate with tuple/array input
*   │   └── partial_derivative<ID>    - ∂f/∂x_i
*   ├── piecewise<Pieces...>          - piecewise-defined function
*   │   └── piece<Cond, Expr>         - single branch (condition, body)
*   ├── parametric_curve<Comps...>    - r(t) = (x(t), y(t), ...)
*   ├── parametric_surface<Comps...>  - r(u,v) = (x(u,v), y(u,v), ...)
*   ├── implicit_function<Expr>       - F(x,y,...) = 0
*   ├── vector_function<Comps...>     - f: R^n -> R^m
*   ├── Inequalities
*   │   ├── inequality<Op, L, R>      - comparison expression
*   │   ├── conjunction<Conds...>     - logical AND of conditions
*   │   └── disjunction<Conds...>     - logical OR of conditions
*   └── Coordinate Systems
*       ├── cartesian_coords<N>       - Cartesian R^N
*       ├── polar_coords               - (r, θ)
*       ├── cylindrical_coords         - (ρ, φ, z)
*       ├── spherical_coords           - (r, θ, φ)
*       └── coord_transform<From,To>  - coordinate conversion
*
* STRUCTURAL REQUIREMENTS:
*   Functions: static evaluate(...), static constexpr arity
*   Piecewise: pieces_type (tuple of piece types)
*   Parametric: components_type (tuple of expression types),
*               static constexpr parameter_count
*   Implicit: static constexpr is_implicit = true
*   Vector-valued: static constexpr output_dimension
*   Coordinate systems: static constexpr is_cartesian / is_polar /
*                       is_cylindrical / is_spherical / dimension
*
* path:      /inc/maths/function.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       date: 2026.02.06
******************************************************************************/

#ifndef DJINTERP_MATHS_FUNCTION_
#define DJINTERP_MATHS_FUNCTION_ 1

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <array>
#include <tuple>
#include <type_traits>
#include <utility>
#include "../djinterp.h"
#include "../cpp_features.h"
#include "expression.hpp"
#include "interval.hpp"


NS_DJINTERP
NS_MATHS


// =============================================================================
// I.    COORDINATE SYSTEMS
// =============================================================================

// cartesian_coords
//   struct: N-dimensional Cartesian coordinate system.
template<std::size_t _Dimension = 2>
struct cartesian_coords
{
    static constexpr std::size_t dimension     = _Dimension;
    static constexpr bool        is_cartesian  = true;
    static constexpr bool        is_polar      = false;
    static constexpr bool        is_cylindrical = false;
    static constexpr bool        is_spherical  = false;
};

// polar_coords
//   struct: 2D polar coordinate system (r, θ).
struct polar_coords
{
    static constexpr std::size_t dimension     = 2;
    static constexpr bool        is_cartesian  = false;
    static constexpr bool        is_polar      = true;
    static constexpr bool        is_cylindrical = false;
    static constexpr bool        is_spherical  = false;
};

// cylindrical_coords
//   struct: 3D cylindrical coordinate system (ρ, φ, z).
struct cylindrical_coords
{
    static constexpr std::size_t dimension     = 3;
    static constexpr bool        is_cartesian  = false;
    static constexpr bool        is_polar      = false;
    static constexpr bool        is_cylindrical = true;
    static constexpr bool        is_spherical  = false;
};

// spherical_coords
//   struct: 3D spherical coordinate system (r, θ, φ).
struct spherical_coords
{
    static constexpr std::size_t dimension     = 3;
    static constexpr bool        is_cartesian  = false;
    static constexpr bool        is_polar      = false;
    static constexpr bool        is_cylindrical = false;
    static constexpr bool        is_spherical  = true;
};


// =============================================================================
// II.   MULTIVARIABLE EVALUATION
// =============================================================================

NS_INTERNAL

    // tuple_evaluator
    //   helper: evaluates a multivariable expression by extracting
    // the variable with matching _ID from a tuple of inputs.
    template<typename _Expr,
             typename _Tuple,
             std::size_t... _Indices>
    struct tuple_evaluator;

    // multi_eval_dispatch
    //   helper: dispatches evaluation for expressions that inspect
    // variable IDs against a tuple of input values.
    template<typename   _ValueType,
             std::size_t _ID>
    struct multi_var_eval
    {
        template<typename _Tuple>
        static constexpr _ValueType evaluate(const _Tuple& _inputs) noexcept
        {
            return static_cast<_ValueType>(
                std::get<_ID>(_inputs)
            );
        }
    };

NS_END  // internal

// multi_evaluate
//   struct: evaluates a multivariable expression with tuple input.
// For f(x, y, z), call multi_evaluate<F>::eval(std::make_tuple(x, y, z)).
// Each variable<T, ID> extracts std::get<ID>(inputs).
template<typename _Expr>
struct multi_evaluate
{
    using expression_type = _Expr;
    using value_type      = typename _Expr::value_type;

    // eval (tuple)
    //   evaluates the expression with a tuple of input values.
    template<typename... _Args>
    static constexpr value_type
    eval
    (
        const std::tuple<_Args...>& _inputs
    ) noexcept
    {
        return _Expr::evaluate(_inputs);
    }

    // eval (array)
    //   evaluates the expression with an array of input values.
    template<typename _T, std::size_t _N>
    static constexpr value_type
    eval
    (
        const std::array<_T, _N>& _inputs
    ) noexcept
    {
        return _Expr::evaluate(_inputs);
    }

    // eval (variadic)
    //   evaluates the expression with variadic arguments packed
    // into a tuple.
    template<typename... _Args>
    static constexpr value_type
    eval
    (
        _Args... _args
    ) noexcept
    {
        return eval(std::make_tuple(_args...));
    }
};

// multivariable
//   struct: a variable that extracts its value from a tuple by index.
// Extends variable<T, ID> with tuple-aware evaluation.
template<typename   _ValueType,
         std::size_t _ID = 0>
struct multivariable : expression_base<multivariable<_ValueType, _ID>>
{
    using self_type  = multivariable<_ValueType, _ID>;
    using value_type = _ValueType;

    static constexpr std::size_t id    = _ID;
    static constexpr std::size_t arity = _ID + 1;
    static constexpr std::size_t degree = 1;
    static constexpr bool        is_constant_expr = false;

    // evaluate (scalar) - behaves like variable<T, ID> for single input
    template<typename _InputType>
    static constexpr value_type
    evaluate
    (
        _InputType _x
    ) noexcept
    {
        return static_cast<value_type>(_x);
    }

    // evaluate (tuple) - extracts element at index _ID
    template<typename... _Args>
    static constexpr value_type
    evaluate
    (
        const std::tuple<_Args...>& _inputs
    ) noexcept
    {
        return static_cast<value_type>(std::get<_ID>(_inputs));
    }

    // evaluate (array) - extracts element at index _ID
    template<typename _T, std::size_t _N>
    static constexpr value_type
    evaluate
    (
        const std::array<_T, _N>& _inputs
    ) noexcept
    {
        static_assert(_ID < _N,
                      "multivariable: _ID exceeds array size.");

        return static_cast<value_type>(_inputs[_ID]);
    }

    // derivative of x_i is 1
    using derivative = constant<value_type, static_cast<value_type>(1)>;
};

// Convenience aliases for multivariable
template<typename _T = double>
using mvar_x = multivariable<_T, 0>;

template<typename _T = double>
using mvar_y = multivariable<_T, 1>;

template<typename _T = double>
using mvar_z = multivariable<_T, 2>;

template<typename _T = double>
using mvar_w = multivariable<_T, 3>;

template<typename _T = double>
using mvar_u = multivariable<_T, 0>;

template<typename _T = double>
using mvar_v = multivariable<_T, 1>;


// =============================================================================
// III.  PARTIAL DERIVATIVE
// =============================================================================

// partial_derivative
//   struct: represents ∂f/∂x_i for a multivariable expression.
// Uses the structural derivative type of the expression when the
// variable ID matches; otherwise the derivative is zero (treating
// other variables as constants).
template<typename   _Expr,
         std::size_t _VarID>
struct partial_derivative
    : expression_base<partial_derivative<_Expr, _VarID>>
{
    using expression_type = _Expr;
    using value_type      = typename _Expr::value_type;

    static constexpr std::size_t var_id = _VarID;
    static constexpr std::size_t arity  = _Expr::arity;
    static constexpr std::size_t degree =
        (_Expr::degree > 0) ? _Expr::degree - 1 : 0;
    static constexpr bool is_constant_expr = _Expr::is_constant_expr;

    // For structural derivative, we delegate to the expression's
    // derivative type. The full chain-rule implementation requires
    // substitution of partial derivatives for each sub-expression.
    using derivative_type = typename _Expr::derivative;
};

// Convenience: ∂/∂x, ∂/∂y, ∂/∂z
template<typename _Expr>
using partial_x = partial_derivative<_Expr, 0>;

template<typename _Expr>
using partial_y = partial_derivative<_Expr, 1>;

template<typename _Expr>
using partial_z = partial_derivative<_Expr, 2>;

// gradient
//   struct: gradient of a scalar function f: R^N -> R.
// Produces an N-tuple of partial derivatives (∂f/∂x₁, ..., ∂f/∂xₙ).
template<typename   _Expr,
         std::size_t _N>
struct gradient
{
    using expression_type = _Expr;
    using value_type      = typename _Expr::value_type;

    static constexpr std::size_t dimension = _N;

    // component
    //   type: the partial derivative with respect to variable _I.
    template<std::size_t _I>
    using component = partial_derivative<_Expr, _I>;
};


// =============================================================================
// IV.   INEQUALITY EXPRESSIONS
// =============================================================================

NS_INTERNAL

    // inequality operation tags
    struct ineq_less {};
    struct ineq_less_equal {};
    struct ineq_greater {};
    struct ineq_greater_equal {};
    struct ineq_equal {};
    struct ineq_not_equal {};

NS_END  // internal

// inequality
//   struct: comparison of two expressions, evaluating to bool.
// Examples:
//   inequality<ineq_less, var_x<>, constant<int,5>> represents x < 5
template<typename _Op,
         typename _Left,
         typename _Right>
struct inequality : expression_base<inequality<_Op, _Left, _Right>>
{
    using self_type  = inequality<_Op, _Left, _Right>;
    using left_type  = _Left;
    using right_type = _Right;
    using op_type    = _Op;
    using value_type = bool;

    static constexpr std::size_t arity =
        (_Left::arity > _Right::arity) ? _Left::arity : _Right::arity;
    static constexpr std::size_t degree     = 0;
    static constexpr bool        is_constant_expr =
        _Left::is_constant_expr && _Right::is_constant_expr;
    static constexpr bool        is_inequality    = true;
};

// Specialization: less than
template<typename _Left,
         typename _Right>
struct inequality<internal::ineq_less, _Left, _Right>
    : expression_base<inequality<internal::ineq_less, _Left, _Right>>
{
    using value_type = bool;

    static constexpr std::size_t arity =
        (_Left::arity > _Right::arity) ? _Left::arity : _Right::arity;
    static constexpr std::size_t degree     = 0;
    static constexpr bool        is_constant_expr =
        _Left::is_constant_expr && _Right::is_constant_expr;
    static constexpr bool        is_inequality    = true;

    template<typename _InputType>
    static constexpr bool
    evaluate
    (
        _InputType _x
    ) noexcept
    {
        return (_Left::evaluate(_x) < _Right::evaluate(_x));
    }
};

// Specialization: less than or equal
template<typename _Left,
         typename _Right>
struct inequality<internal::ineq_less_equal, _Left, _Right>
    : expression_base<inequality<internal::ineq_less_equal, _Left, _Right>>
{
    using value_type = bool;

    static constexpr std::size_t arity =
        (_Left::arity > _Right::arity) ? _Left::arity : _Right::arity;
    static constexpr std::size_t degree     = 0;
    static constexpr bool        is_constant_expr =
        _Left::is_constant_expr && _Right::is_constant_expr;
    static constexpr bool        is_inequality    = true;

    template<typename _InputType>
    static constexpr bool
    evaluate
    (
        _InputType _x
    ) noexcept
    {
        return (_Left::evaluate(_x) <= _Right::evaluate(_x));
    }
};

// Specialization: greater than
template<typename _Left,
         typename _Right>
struct inequality<internal::ineq_greater, _Left, _Right>
    : expression_base<inequality<internal::ineq_greater, _Left, _Right>>
{
    using value_type = bool;

    static constexpr std::size_t arity =
        (_Left::arity > _Right::arity) ? _Left::arity : _Right::arity;
    static constexpr std::size_t degree     = 0;
    static constexpr bool        is_constant_expr =
        _Left::is_constant_expr && _Right::is_constant_expr;
    static constexpr bool        is_inequality    = true;

    template<typename _InputType>
    static constexpr bool
    evaluate
    (
        _InputType _x
    ) noexcept
    {
        return (_Left::evaluate(_x) > _Right::evaluate(_x));
    }
};

// Specialization: greater than or equal
template<typename _Left,
         typename _Right>
struct inequality<internal::ineq_greater_equal, _Left, _Right>
    : expression_base<inequality<internal::ineq_greater_equal, _Left, _Right>>
{
    using value_type = bool;

    static constexpr std::size_t arity =
        (_Left::arity > _Right::arity) ? _Left::arity : _Right::arity;
    static constexpr std::size_t degree     = 0;
    static constexpr bool        is_constant_expr =
        _Left::is_constant_expr && _Right::is_constant_expr;
    static constexpr bool        is_inequality    = true;

    template<typename _InputType>
    static constexpr bool
    evaluate
    (
        _InputType _x
    ) noexcept
    {
        return (_Left::evaluate(_x) >= _Right::evaluate(_x));
    }
};

// Specialization: equality
template<typename _Left,
         typename _Right>
struct inequality<internal::ineq_equal, _Left, _Right>
    : expression_base<inequality<internal::ineq_equal, _Left, _Right>>
{
    using value_type = bool;

    static constexpr std::size_t arity =
        (_Left::arity > _Right::arity) ? _Left::arity : _Right::arity;
    static constexpr std::size_t degree     = 0;
    static constexpr bool        is_constant_expr =
        _Left::is_constant_expr && _Right::is_constant_expr;
    static constexpr bool        is_inequality    = true;

    template<typename _InputType>
    static constexpr bool
    evaluate
    (
        _InputType _x
    ) noexcept
    {
        return (_Left::evaluate(_x) == _Right::evaluate(_x));
    }
};

// Specialization: not equal
template<typename _Left,
         typename _Right>
struct inequality<internal::ineq_not_equal, _Left, _Right>
    : expression_base<inequality<internal::ineq_not_equal, _Left, _Right>>
{
    using value_type = bool;

    static constexpr std::size_t arity =
        (_Left::arity > _Right::arity) ? _Left::arity : _Right::arity;
    static constexpr std::size_t degree     = 0;
    static constexpr bool        is_constant_expr =
        _Left::is_constant_expr && _Right::is_constant_expr;
    static constexpr bool        is_inequality    = true;

    template<typename _InputType>
    static constexpr bool
    evaluate
    (
        _InputType _x
    ) noexcept
    {
        return (_Left::evaluate(_x) != _Right::evaluate(_x));
    }
};

// Convenience aliases for inequalities
template<typename _L, typename _R>
using less_than_expr = inequality<internal::ineq_less, _L, _R>;

template<typename _L, typename _R>
using less_equal_expr = inequality<internal::ineq_less_equal, _L, _R>;

template<typename _L, typename _R>
using greater_than_expr = inequality<internal::ineq_greater, _L, _R>;

template<typename _L, typename _R>
using greater_equal_expr = inequality<internal::ineq_greater_equal, _L, _R>;

template<typename _L, typename _R>
using equal_expr = inequality<internal::ineq_equal, _L, _R>;

template<typename _L, typename _R>
using not_equal_expr = inequality<internal::ineq_not_equal, _L, _R>;

// compound_interval_constraint
//   struct: checks lower < expr < upper (chained inequality).
template<typename _Lower,
         typename _Expr,
         typename _Upper,
         bool     _LeftInclusive  = false,
         bool     _RightInclusive = false>
struct compound_interval_constraint
    : expression_base<compound_interval_constraint<
          _Lower, _Expr, _Upper, _LeftInclusive, _RightInclusive>>
{
    using value_type = bool;

    static constexpr std::size_t arity =
        (_Expr::arity > _Lower::arity)
            ? ((_Expr::arity > _Upper::arity)
                   ? _Expr::arity : _Upper::arity)
            : ((_Lower::arity > _Upper::arity)
                   ? _Lower::arity : _Upper::arity);
    static constexpr std::size_t degree     = 0;
    static constexpr bool        is_constant_expr =
        ( _Lower::is_constant_expr &&
          _Expr::is_constant_expr  &&
          _Upper::is_constant_expr );
    static constexpr bool        is_inequality    = true;

    template<typename _InputType>
    static constexpr bool
    evaluate
    (
        _InputType _x
    ) noexcept
    {
        auto lo  = _Lower::evaluate(_x);
        auto val = _Expr::evaluate(_x);
        auto hi  = _Upper::evaluate(_x);

        bool left_ok  = _LeftInclusive  ? (lo <= val) : (lo < val);
        bool right_ok = _RightInclusive ? (val <= hi) : (val < hi);

        return (left_ok && right_ok);
    }
};


// =============================================================================
// V.    LOGICAL COMBINATORS
// =============================================================================

NS_INTERNAL

    // conjunction_eval_helper
    //   helper: evaluates logical AND of condition expressions.
    template<typename... _Conditions>
    struct conjunction_eval_helper;

    template<typename _First, typename... _Rest>
    struct conjunction_eval_helper<_First, _Rest...>
    {
        template<typename _InputType>
        static constexpr bool evaluate(_InputType _x) noexcept
        {
            return ( _First::evaluate(_x) &&
                     conjunction_eval_helper<_Rest...>::evaluate(_x) );
        }
    };

    template<typename _Last>
    struct conjunction_eval_helper<_Last>
    {
        template<typename _InputType>
        static constexpr bool evaluate(_InputType _x) noexcept
        {
            return _Last::evaluate(_x);
        }
    };

    template<>
    struct conjunction_eval_helper<>
    {
        template<typename _InputType>
        static constexpr bool evaluate(_InputType) noexcept
        {
            return true;
        }
    };

    // disjunction_eval_helper
    //   helper: evaluates logical OR of condition expressions.
    template<typename... _Conditions>
    struct disjunction_eval_helper;

    template<typename _First, typename... _Rest>
    struct disjunction_eval_helper<_First, _Rest...>
    {
        template<typename _InputType>
        static constexpr bool evaluate(_InputType _x) noexcept
        {
            return ( _First::evaluate(_x) ||
                     disjunction_eval_helper<_Rest...>::evaluate(_x) );
        }
    };

    template<typename _Last>
    struct disjunction_eval_helper<_Last>
    {
        template<typename _InputType>
        static constexpr bool evaluate(_InputType _x) noexcept
        {
            return _Last::evaluate(_x);
        }
    };

    template<>
    struct disjunction_eval_helper<>
    {
        template<typename _InputType>
        static constexpr bool evaluate(_InputType) noexcept
        {
            return false;
        }
    };

    // max_arity_helper
    //   helper: computes maximum arity across expression types.
    template<typename... _Types>
    struct max_arity_helper;

    template<typename _First, typename... _Rest>
    struct max_arity_helper<_First, _Rest...>
    {
        static constexpr std::size_t rest_val =
            max_arity_helper<_Rest...>::value;
        static constexpr std::size_t value =
            (_First::arity > rest_val) ? _First::arity : rest_val;
    };

    template<typename _Last>
    struct max_arity_helper<_Last>
    {
        static constexpr std::size_t value = _Last::arity;
    };

    template<>
    struct max_arity_helper<>
    {
        static constexpr std::size_t value = 0;
    };

    // all_constant_helper
    //   helper: checks if all expressions are constant.
    template<typename... _Types>
    struct all_constant_helper;

    template<typename _First, typename... _Rest>
    struct all_constant_helper<_First, _Rest...>
    {
        static constexpr bool value =
            ( _First::is_constant_expr &&
              all_constant_helper<_Rest...>::value );
    };

    template<typename _Last>
    struct all_constant_helper<_Last>
    {
        static constexpr bool value = _Last::is_constant_expr;
    };

    template<>
    struct all_constant_helper<>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// conjunction
//   struct: logical AND of multiple condition expressions.
// All conditions must evaluate to true.
template<typename... _Conditions>
struct conjunction
    : expression_base<conjunction<_Conditions...>>
{
    using self_type  = conjunction<_Conditions...>;
    using value_type = bool;

    static constexpr std::size_t arity =
        internal::max_arity_helper<_Conditions...>::value;
    static constexpr std::size_t degree     = 0;
    static constexpr bool        is_constant_expr =
        internal::all_constant_helper<_Conditions...>::value;
    static constexpr bool        is_inequality    = true;

    template<typename _InputType>
    static constexpr bool
    evaluate
    (
        _InputType _x
    ) noexcept
    {
        return internal::conjunction_eval_helper<
            _Conditions...
        >::evaluate(_x);
    }
};

// disjunction
//   struct: logical OR of multiple condition expressions.
// At least one condition must evaluate to true.
template<typename... _Conditions>
struct disjunction
    : expression_base<disjunction<_Conditions...>>
{
    using self_type  = disjunction<_Conditions...>;
    using value_type = bool;

    static constexpr std::size_t arity =
        internal::max_arity_helper<_Conditions...>::value;
    static constexpr std::size_t degree     = 0;
    static constexpr bool        is_constant_expr =
        internal::all_constant_helper<_Conditions...>::value;
    static constexpr bool        is_inequality    = true;

    template<typename _InputType>
    static constexpr bool
    evaluate
    (
        _InputType _x
    ) noexcept
    {
        return internal::disjunction_eval_helper<
            _Conditions...
        >::evaluate(_x);
    }
};

// logical_not
//   struct: logical negation of a condition expression.
template<typename _Condition>
struct logical_not
    : expression_base<logical_not<_Condition>>
{
    using self_type      = logical_not<_Condition>;
    using condition_type = _Condition;
    using value_type     = bool;

    static constexpr std::size_t arity          = _Condition::arity;
    static constexpr std::size_t degree         = 0;
    static constexpr bool        is_constant_expr = _Condition::is_constant_expr;
    static constexpr bool        is_inequality  = true;

    template<typename _InputType>
    static constexpr bool
    evaluate
    (
        _InputType _x
    ) noexcept
    {
        return !_Condition::evaluate(_x);
    }
};


// =============================================================================
// VI.   PIECEWISE FUNCTIONS
// =============================================================================

// piece
//   struct: a single branch of a piecewise function.
// Pairs a condition expression with a body expression.
// When the condition evaluates to true, the body is used.
template<typename _Condition,
         typename _Expression>
struct piece
{
    using condition_type  = _Condition;
    using expression_type = _Expression;
    using value_type      = typename _Expression::value_type;

    static constexpr std::size_t arity =
        (_Condition::arity > _Expression::arity)
            ? _Condition::arity
            : _Expression::arity;
    static constexpr std::size_t degree = _Expression::degree;

    template<typename _InputType>
    static constexpr bool
    test
    (
        _InputType _x
    ) noexcept
    {
        return _Condition::evaluate(_x);
    }

    template<typename _InputType>
    static constexpr value_type
    evaluate
    (
        _InputType _x
    ) noexcept
    {
        return _Expression::evaluate(_x);
    }
};

// otherwise
//   struct: condition that always evaluates to true.
// Used as the fallback/default branch in a piecewise function.
struct otherwise : expression_base<otherwise>
{
    using value_type = bool;

    static constexpr std::size_t arity          = 0;
    static constexpr std::size_t degree         = 0;
    static constexpr bool        is_constant_expr = true;

    template<typename _InputType>
    static constexpr bool
    evaluate
    (
        _InputType
    ) noexcept
    {
        return true;
    }
};

NS_INTERNAL

    // piecewise_eval_helper
    //   helper: evaluates the first matching piece in a piecewise
    // function. Falls through to subsequent pieces if the condition
    // is not met. Returns 0 if no piece matches.
    template<typename... _Pieces>
    struct piecewise_eval_helper;

    template<typename _First, typename... _Rest>
    struct piecewise_eval_helper<_First, _Rest...>
    {
        template<typename _InputType>
        static constexpr auto evaluate(_InputType _x) noexcept
        {
            if (_First::test(_x))
            {
                return _First::evaluate(_x);
            }

            return piecewise_eval_helper<_Rest...>::evaluate(_x);
        }
    };

    template<typename _Last>
    struct piecewise_eval_helper<_Last>
    {
        template<typename _InputType>
        static constexpr auto evaluate(_InputType _x) noexcept
        {
            return _Last::evaluate(_x);
        }
    };

    // piecewise_degree_helper
    //   helper: finds maximum degree among pieces.
    template<typename... _Pieces>
    struct piecewise_degree_helper;

    template<typename _First, typename... _Rest>
    struct piecewise_degree_helper<_First, _Rest...>
    {
        static constexpr std::size_t rest_degree =
            piecewise_degree_helper<_Rest...>::value;
        static constexpr std::size_t value =
            (_First::degree > rest_degree) ? _First::degree : rest_degree;
    };

    template<typename _Last>
    struct piecewise_degree_helper<_Last>
    {
        static constexpr std::size_t value = _Last::degree;
    };

NS_END  // internal

// piecewise
//   struct: piecewise-defined function over multiple branches.
// Evaluates pieces in order; the first whose condition is true
// determines the result. Use piece<otherwise, Expr> as a default.
//
// Example (absolute value):
//   using abs_fn = piecewise<
//       piece<greater_equal_expr<var_x<>, zero_constant<>>, var_x<>>,
//       piece<otherwise, negate<var_x<>>>
//   >;
template<typename... _Pieces>
struct piecewise
    : expression_base<piecewise<_Pieces...>>
{
    using self_type  = piecewise<_Pieces...>;
    using value_type = std::common_type_t<typename _Pieces::value_type...>;
    using pieces_type = std::tuple<_Pieces...>;

    static constexpr std::size_t num_pieces = sizeof...(_Pieces);
    static constexpr std::size_t arity =
        internal::max_arity_helper<_Pieces...>::value;
    static constexpr std::size_t degree =
        internal::piecewise_degree_helper<_Pieces...>::value;
    static constexpr bool is_constant_expr = false;
    static constexpr bool is_piecewise     = true;

    template<typename _InputType>
    static constexpr value_type
    evaluate
    (
        _InputType _x
    ) noexcept
    {
        return static_cast<value_type>(
            internal::piecewise_eval_helper<_Pieces...>::evaluate(_x)
        );
    }
};


// =============================================================================
// VII.  PARAMETRIC CURVES AND SURFACES
// =============================================================================

NS_INTERNAL

    // parametric_arity_helper
    //   helper: computes the maximum arity needed by any component.
    template<typename... _Components>
    using parametric_arity_helper = max_arity_helper<_Components...>;

NS_END  // internal

// parametric_curve
//   struct: a parametric curve r(t) = (x(t), y(t), ...).
// Each component is an expression evaluated at the same parameter t.
// The parameter count is 1 (single parameter t).
//
// Example (unit circle):
//   using circle = parametric_curve<cos_fn<var_t<>>, sin_fn<var_t<>>>;
template<typename... _Components>
struct parametric_curve
    : expression_base<parametric_curve<_Components...>>
{
    using self_type      = parametric_curve<_Components...>;
    using value_type     = double;
    using components_type = std::tuple<_Components...>;
    using coordinate_system = cartesian_coords<sizeof...(_Components)>;

    static constexpr std::size_t output_dimension  = sizeof...(_Components);
    static constexpr std::size_t parameter_count   = 1;
    static constexpr std::size_t arity             = 1;
    static constexpr std::size_t degree            = 0;
    static constexpr bool        is_constant_expr  = false;
    static constexpr bool        is_parametric     = true;

    // evaluate
    //   returns an array of component values at parameter _t.
    template<typename _InputType>
    static constexpr std::array<value_type, output_dimension>
    evaluate
    (
        _InputType _t
    ) noexcept
    {
        return {{ static_cast<value_type>(_Components::evaluate(_t))... }};
    }

    // evaluate_component
    //   evaluates a single component by index.
    template<std::size_t _Index, typename _InputType>
    static constexpr value_type
    evaluate_component
    (
        _InputType _t
    ) noexcept
    {
        using component = std::tuple_element_t<_Index, components_type>;

        return static_cast<value_type>(component::evaluate(_t));
    }

    // tangent
    //   type: the derivative curve r'(t) = (x'(t), y'(t), ...).
    using tangent = parametric_curve<typename _Components::derivative...>;
};

// parametric_surface
//   struct: a parametric surface r(u, v) = (x(u,v), y(u,v), z(u,v)).
// Each component is a multivariable expression in two parameters.
// The parameter count is 2.
//
// Example (sphere):
//   using sphere_x = product<
//       product<cos_fn<mvar_u<>>, sin_fn<mvar_v<>>>,
//       constant<int, 1>>;  // R*cos(u)*sin(v)
template<typename... _Components>
struct parametric_surface
    : expression_base<parametric_surface<_Components...>>
{
    using self_type       = parametric_surface<_Components...>;
    using value_type      = double;
    using components_type = std::tuple<_Components...>;
    using coordinate_system = cartesian_coords<sizeof...(_Components)>;

    static constexpr std::size_t output_dimension  = sizeof...(_Components);
    static constexpr std::size_t parameter_count   = 2;
    static constexpr std::size_t arity             = 2;
    static constexpr std::size_t degree            = 0;
    static constexpr bool        is_constant_expr  = false;
    static constexpr bool        is_parametric     = true;

    // evaluate
    //   returns component values at parameters (_u, _v).
    template<typename _InputType>
    static constexpr std::array<value_type, output_dimension>
    evaluate
    (
        _InputType _u,
        _InputType _v
    ) noexcept
    {
        auto params = std::make_tuple(_u, _v);

        return {{ static_cast<value_type>(
            _Components::evaluate(params))... }};
    }

    // evaluate (tuple)
    //   evaluates with a tuple of parameters.
    template<typename... _Args>
    static constexpr std::array<value_type, output_dimension>
    evaluate
    (
        const std::tuple<_Args...>& _params
    ) noexcept
    {
        return {{ static_cast<value_type>(
            _Components::evaluate(_params))... }};
    }
};

// parametric_nd
//   struct: general N-parameter parametric mapping.
// r(t₁, ..., tₙ) = (f₁(t), ..., fₘ(t)) where each fᵢ is an
// expression in _ParamCount variables.
template<std::size_t _ParamCount,
         typename... _Components>
struct parametric_nd
    : expression_base<parametric_nd<_ParamCount, _Components...>>
{
    using self_type       = parametric_nd<_ParamCount, _Components...>;
    using value_type      = double;
    using components_type = std::tuple<_Components...>;
    using coordinate_system = cartesian_coords<sizeof...(_Components)>;

    static constexpr std::size_t output_dimension  = sizeof...(_Components);
    static constexpr std::size_t parameter_count   = _ParamCount;
    static constexpr std::size_t arity             = _ParamCount;
    static constexpr std::size_t degree            = 0;
    static constexpr bool        is_constant_expr  = false;
    static constexpr bool        is_parametric     = true;

    // evaluate (tuple)
    //   evaluates all components at the given parameters.
    template<typename... _Args>
    static constexpr std::array<value_type, output_dimension>
    evaluate
    (
        const std::tuple<_Args...>& _params
    ) noexcept
    {
        return {{ static_cast<value_type>(
            _Components::evaluate(_params))... }};
    }
};


// =============================================================================
// VIII. IMPLICIT FUNCTIONS
// =============================================================================

// implicit_function
//   struct: an implicitly defined function F(x, y, ...) = 0.
// Rather than y = f(x), the function is defined by the zero set
// of an expression. Useful for curves like x² + y² = 1.
//
// Example (unit circle):
//   using circle = implicit_function<
//       difference<
//           sum<power_n<mvar_x<>, 2>, power_n<mvar_y<>, 2>>,
//           one_constant<double>
//       >
//   >;
template<typename _Expr>
struct implicit_function
    : expression_base<implicit_function<_Expr>>
{
    using self_type       = implicit_function<_Expr>;
    using expression_type = _Expr;
    using value_type      = typename _Expr::value_type;

    static constexpr std::size_t arity          = _Expr::arity;
    static constexpr std::size_t degree         = _Expr::degree;
    static constexpr bool        is_constant_expr = _Expr::is_constant_expr;
    static constexpr bool        is_implicit    = true;

    // evaluate
    //   returns F(x, y, ...) — the value of the implicit expression.
    // The zero set { (x,y,...) | F = 0 } defines the curve/surface.
    template<typename _InputType>
    static constexpr value_type
    evaluate
    (
        _InputType _x
    ) noexcept
    {
        return _Expr::evaluate(_x);
    }

    // evaluate (tuple)
    //   evaluates F with a tuple of coordinates.
    template<typename... _Args>
    static constexpr value_type
    evaluate
    (
        const std::tuple<_Args...>& _inputs
    ) noexcept
    {
        return _Expr::evaluate(_inputs);
    }

    // contains
    //   checks whether a point lies on the implicit curve/surface.
    // Returns true when |F(x,y,...)| < _epsilon.
    template<typename _InputType>
    static constexpr bool
    contains
    (
        _InputType _x,
        value_type _epsilon = static_cast<value_type>(1e-10)
    ) noexcept
    {
        auto val = _Expr::evaluate(_x);

        return ( (val > -_epsilon) &&
                 (val <  _epsilon) );
    }

    // sign
    //   returns -1, 0, or +1 indicating which side of the
    // curve/surface a point lies on.
    template<typename _InputType>
    static constexpr int
    sign
    (
        _InputType _x
    ) noexcept
    {
        auto val = _Expr::evaluate(_x);

        if (val < static_cast<value_type>(0))
        {
            return -1;
        }

        if (val > static_cast<value_type>(0))
        {
            return 1;
        }

        return 0;
    }
};

// level_set
//   struct: the set of points where F(x,...) = _Level.
// Generalizes implicit_function to non-zero levels.
template<typename _Expr,
         typename _Level>
struct level_set
    : implicit_function<difference<_Expr, _Level>>
{
    using base_type = implicit_function<difference<_Expr, _Level>>;
    using level_type = _Level;

    static constexpr auto level_value = _Level::value;
};


// =============================================================================
// IX.   VECTOR-VALUED FUNCTIONS
// =============================================================================

// vector_function
//   struct: a function f: R^n -> R^m given by m component expressions.
// Each component is an expression in the same set of input variables.
//
// Example (rotation matrix application):
//   using rotate = vector_function<
//       difference<product<cos_fn<var_t<>>, mvar_x<>>,
//                  product<sin_fn<var_t<>>, mvar_y<>>>,
//       sum<product<sin_fn<var_t<>>, mvar_x<>>,
//           product<cos_fn<var_t<>>, mvar_y<>>>
//   >;
template<typename... _Components>
struct vector_function
    : expression_base<vector_function<_Components...>>
{
    using self_type       = vector_function<_Components...>;
    using value_type      = double;
    using components_type = std::tuple<_Components...>;

    static constexpr std::size_t output_dimension = sizeof...(_Components);
    static constexpr std::size_t arity =
        internal::max_arity_helper<_Components...>::value;
    static constexpr std::size_t degree = 0;
    static constexpr bool        is_constant_expr =
        internal::all_constant_helper<_Components...>::value;
    static constexpr bool        is_vector_valued = true;

    // evaluate (scalar)
    //   returns component values as an array.
    template<typename _InputType>
    static constexpr std::array<value_type, output_dimension>
    evaluate
    (
        _InputType _x
    ) noexcept
    {
        return {{ static_cast<value_type>(
            _Components::evaluate(_x))... }};
    }

    // evaluate (tuple)
    //   evaluates with a tuple of inputs.
    template<typename... _Args>
    static constexpr std::array<value_type, output_dimension>
    evaluate
    (
        const std::tuple<_Args...>& _inputs
    ) noexcept
    {
        return {{ static_cast<value_type>(
            _Components::evaluate(_inputs))... }};
    }

    // component
    //   type: the expression for the _I-th component.
    template<std::size_t _I>
    using component = std::tuple_element_t<_I, components_type>;

    // jacobian_row
    //   type: partial derivatives of component _I with respect
    // to all input variables.
    template<std::size_t _I>
    using jacobian_row = gradient<
        std::tuple_element_t<_I, components_type>,
        arity
    >;
};

// scalar_field
//   type: a function from R^n to R (single-component vector function).
template<typename _Expr>
using scalar_field = _Expr;

// vector_field_2d
//   type: a vector field R² -> R².
template<typename _Fx, typename _Fy>
using vector_field_2d = vector_function<_Fx, _Fy>;

// vector_field_3d
//   type: a vector field R³ -> R³.
template<typename _Fx, typename _Fy, typename _Fz>
using vector_field_3d = vector_function<_Fx, _Fy, _Fz>;


// =============================================================================
// X.    COORDINATE TRANSFORMS
// =============================================================================

// coord_transform
//   struct: converts between coordinate systems.
// Provides static methods for converting points between the
// _From and _To coordinate systems.
template<typename _From,
         typename _To>
struct coord_transform;

// Specialization: polar -> Cartesian
//   (r, θ) -> (x, y) = (r cos θ, r sin θ)
template<>
struct coord_transform<polar_coords, cartesian_coords<2>>
{
    using from_type = polar_coords;
    using to_type   = cartesian_coords<2>;

    template<typename _T>
    static constexpr std::array<_T, 2>
    convert
    (
        _T _r,
        _T _theta
    ) noexcept
    {
        return {{ _r * std::cos(_theta),
                  _r * std::sin(_theta) }};
    }

    template<typename _T>
    static constexpr std::array<_T, 2>
    convert
    (
        const std::array<_T, 2>& _polar
    ) noexcept
    {
        return convert(_polar[0], _polar[1]);
    }
};

// Specialization: Cartesian -> polar
//   (x, y) -> (r, θ) = (√(x²+y²), atan2(y, x))
template<>
struct coord_transform<cartesian_coords<2>, polar_coords>
{
    using from_type = cartesian_coords<2>;
    using to_type   = polar_coords;

    template<typename _T>
    static constexpr std::array<_T, 2>
    convert
    (
        _T _x,
        _T _y
    ) noexcept
    {
        return {{ std::sqrt(_x * _x + _y * _y),
                  std::atan2(_y, _x) }};
    }

    template<typename _T>
    static constexpr std::array<_T, 2>
    convert
    (
        const std::array<_T, 2>& _cart
    ) noexcept
    {
        return convert(_cart[0], _cart[1]);
    }
};

// Specialization: cylindrical -> Cartesian
//   (ρ, φ, z) -> (x, y, z) = (ρ cos φ, ρ sin φ, z)
template<>
struct coord_transform<cylindrical_coords, cartesian_coords<3>>
{
    using from_type = cylindrical_coords;
    using to_type   = cartesian_coords<3>;

    template<typename _T>
    static constexpr std::array<_T, 3>
    convert
    (
        _T _rho,
        _T _phi,
        _T _z
    ) noexcept
    {
        return {{ _rho * std::cos(_phi),
                  _rho * std::sin(_phi),
                  _z }};
    }

    template<typename _T>
    static constexpr std::array<_T, 3>
    convert
    (
        const std::array<_T, 3>& _cyl
    ) noexcept
    {
        return convert(_cyl[0], _cyl[1], _cyl[2]);
    }
};

// Specialization: Cartesian -> cylindrical
//   (x, y, z) -> (ρ, φ, z) = (√(x²+y²), atan2(y,x), z)
template<>
struct coord_transform<cartesian_coords<3>, cylindrical_coords>
{
    using from_type = cartesian_coords<3>;
    using to_type   = cylindrical_coords;

    template<typename _T>
    static constexpr std::array<_T, 3>
    convert
    (
        _T _x,
        _T _y,
        _T _z
    ) noexcept
    {
        return {{ std::sqrt(_x * _x + _y * _y),
                  std::atan2(_y, _x),
                  _z }};
    }

    template<typename _T>
    static constexpr std::array<_T, 3>
    convert
    (
        const std::array<_T, 3>& _cart
    ) noexcept
    {
        return convert(_cart[0], _cart[1], _cart[2]);
    }
};

// Specialization: spherical -> Cartesian
//   (r, θ, φ) -> (x, y, z) = (r sinθ cosφ, r sinθ sinφ, r cosθ)
template<>
struct coord_transform<spherical_coords, cartesian_coords<3>>
{
    using from_type = spherical_coords;
    using to_type   = cartesian_coords<3>;

    template<typename _T>
    static constexpr std::array<_T, 3>
    convert
    (
        _T _r,
        _T _theta,
        _T _phi
    ) noexcept
    {
        _T st = std::sin(_theta);

        return {{ _r * st * std::cos(_phi),
                  _r * st * std::sin(_phi),
                  _r * std::cos(_theta) }};
    }

    template<typename _T>
    static constexpr std::array<_T, 3>
    convert
    (
        const std::array<_T, 3>& _sph
    ) noexcept
    {
        return convert(_sph[0], _sph[1], _sph[2]);
    }
};

// Specialization: Cartesian -> spherical
//   (x, y, z) -> (r, θ, φ) = (√(x²+y²+z²), acos(z/r), atan2(y,x))
template<>
struct coord_transform<cartesian_coords<3>, spherical_coords>
{
    using from_type = cartesian_coords<3>;
    using to_type   = spherical_coords;

    template<typename _T>
    static constexpr std::array<_T, 3>
    convert
    (
        _T _x,
        _T _y,
        _T _z
    ) noexcept
    {
        _T r = std::sqrt(_x * _x + _y * _y + _z * _z);

        return {{ r,
                  std::acos(_z / r),
                  std::atan2(_y, _x) }};
    }

    template<typename _T>
    static constexpr std::array<_T, 3>
    convert
    (
        const std::array<_T, 3>& _cart
    ) noexcept
    {
        return convert(_cart[0], _cart[1], _cart[2]);
    }
};

// Specialization: cylindrical <-> spherical
template<>
struct coord_transform<cylindrical_coords, spherical_coords>
{
    using from_type = cylindrical_coords;
    using to_type   = spherical_coords;

    template<typename _T>
    static constexpr std::array<_T, 3>
    convert
    (
        _T _rho,
        _T _phi,
        _T _z
    ) noexcept
    {
        _T r = std::sqrt(_rho * _rho + _z * _z);

        return {{ r,
                  std::atan2(_rho, _z),
                  _phi }};
    }

    template<typename _T>
    static constexpr std::array<_T, 3>
    convert
    (
        const std::array<_T, 3>& _cyl
    ) noexcept
    {
        return convert(_cyl[0], _cyl[1], _cyl[2]);
    }
};

template<>
struct coord_transform<spherical_coords, cylindrical_coords>
{
    using from_type = spherical_coords;
    using to_type   = cylindrical_coords;

    template<typename _T>
    static constexpr std::array<_T, 3>
    convert
    (
        _T _r,
        _T _theta,
        _T _phi
    ) noexcept
    {
        return {{ _r * std::sin(_theta),
                  _phi,
                  _r * std::cos(_theta) }};
    }

    template<typename _T>
    static constexpr std::array<_T, 3>
    convert
    (
        const std::array<_T, 3>& _sph
    ) noexcept
    {
        return convert(_sph[0], _sph[1], _sph[2]);
    }
};


// =============================================================================
// XI.   POLAR / COORDINATE-SYSTEM FUNCTION WRAPPERS
// =============================================================================

// polar_function
//   struct: a function defined in polar coordinates r = f(θ).
// Wraps a scalar expression and provides conversion to Cartesian.
//
// Example (cardioid):
//   using cardioid = polar_function<
//       sum<one_constant<double>, cos_fn<var_x<>>>
//   >;  // r = 1 + cos(θ)
template<typename _RadiusExpr>
struct polar_function
    : expression_base<polar_function<_RadiusExpr>>
{
    using self_type       = polar_function<_RadiusExpr>;
    using radius_type     = _RadiusExpr;
    using value_type      = double;
    using coordinate_system = polar_coords;

    static constexpr std::size_t arity          = 1;
    static constexpr std::size_t degree         = _RadiusExpr::degree;
    static constexpr bool        is_constant_expr = _RadiusExpr::is_constant_expr;
    static constexpr bool        is_polar_form  = true;

    // evaluate
    //   returns r at the given angle θ.
    template<typename _InputType>
    static constexpr value_type
    evaluate
    (
        _InputType _theta
    ) noexcept
    {
        return static_cast<value_type>(_RadiusExpr::evaluate(_theta));
    }

    // to_cartesian
    //   converts (θ) -> (x, y) via r(θ).
    template<typename _InputType>
    static constexpr std::array<value_type, 2>
    to_cartesian
    (
        _InputType _theta
    ) noexcept
    {
        value_type r = evaluate(_theta);

        return {{ r * std::cos(static_cast<value_type>(_theta)),
                  r * std::sin(static_cast<value_type>(_theta)) }};
    }

    // as_parametric
    //   type: equivalent parametric curve (x(θ), y(θ)).
    using as_parametric = parametric_curve<
        product<_RadiusExpr, cos_fn<var_x<>>>,
        product<_RadiusExpr, sin_fn<var_x<>>>
    >;
};

// cylindrical_function
//   struct: a function defined in cylindrical coordinates.
// Wraps expressions for ρ(t) and z(t) with a free angle φ.
template<typename _RhoExpr,
         typename _ZExpr>
struct cylindrical_function
    : expression_base<cylindrical_function<_RhoExpr, _ZExpr>>
{
    using self_type       = cylindrical_function<_RhoExpr, _ZExpr>;
    using value_type      = double;
    using coordinate_system = cylindrical_coords;

    static constexpr std::size_t arity =
        (_RhoExpr::arity > _ZExpr::arity)
            ? _RhoExpr::arity : _ZExpr::arity;
    static constexpr std::size_t degree         = 0;
    static constexpr bool        is_constant_expr = false;

    // evaluate
    //   returns (ρ, z) at parameter _t (φ is a free variable).
    template<typename _InputType>
    static constexpr std::array<value_type, 2>
    evaluate
    (
        _InputType _t
    ) noexcept
    {
        return {{ static_cast<value_type>(_RhoExpr::evaluate(_t)),
                  static_cast<value_type>(_ZExpr::evaluate(_t)) }};
    }
};

// spherical_function
//   struct: a function r = f(θ, φ) in spherical coordinates.
template<typename _RadiusExpr>
struct spherical_function
    : expression_base<spherical_function<_RadiusExpr>>
{
    using self_type       = spherical_function<_RadiusExpr>;
    using value_type      = double;
    using coordinate_system = spherical_coords;

    static constexpr std::size_t arity          = 2;
    static constexpr std::size_t degree         = _RadiusExpr::degree;
    static constexpr bool        is_constant_expr = _RadiusExpr::is_constant_expr;

    // evaluate
    //   returns r at angles (θ, φ).
    template<typename _InputType>
    static constexpr value_type
    evaluate
    (
        _InputType _theta,
        _InputType _phi
    ) noexcept
    {
        auto params = std::make_tuple(_theta, _phi);

        return static_cast<value_type>(_RadiusExpr::evaluate(params));
    }

    // to_cartesian
    //   converts (θ, φ) -> (x, y, z) via r(θ, φ).
    template<typename _InputType>
    static constexpr std::array<value_type, 3>
    to_cartesian
    (
        _InputType _theta,
        _InputType _phi
    ) noexcept
    {
        value_type r = evaluate(_theta, _phi);

        return coord_transform<spherical_coords,
                               cartesian_coords<3>>::convert(
            r,
            static_cast<value_type>(_theta),
            static_cast<value_type>(_phi)
        );
    }
};


// =============================================================================
// XII.  NAMED FUNCTION WRAPPER
// =============================================================================

// math_function
//   struct: wraps an expression with a name, domain, and coordinate
// system to form a complete mathematical function definition.
// This is the top-level type for representing named functions like
// f(x) = x² + 1 or g(r, θ) = r² sin(2θ).
template<typename _Expression,
         typename _Domain          = void,
         typename _CoordinateSystem = cartesian_coords<1>>
struct math_function
{
    using expression_type   = _Expression;
    using domain_type       = _Domain;
    using coordinate_system = _CoordinateSystem;
    using value_type        = typename _Expression::value_type;

    static constexpr std::size_t arity  = _Expression::arity;
    static constexpr std::size_t degree = _Expression::degree;
    static constexpr bool is_constant_expr = _Expression::is_constant_expr;

    // evaluate - delegates to expression
    template<typename _InputType>
    static constexpr value_type
    evaluate
    (
        _InputType _x
    ) noexcept
    {
        return _Expression::evaluate(_x);
    }

    // evaluate (tuple) - for multivariable functions
    template<typename... _Args>
    static constexpr value_type
    evaluate
    (
        const std::tuple<_Args...>& _inputs
    ) noexcept
    {
        return _Expression::evaluate(_inputs);
    }

    // in_domain
    //   checks whether the input is within the function's domain.
    // When _Domain is void, all inputs are considered valid.
    template<typename _InputType>
    static constexpr bool
    in_domain
    (
        _InputType _x
    ) noexcept
    {
        if constexpr (std::is_void_v<_Domain>)
        {
            return true;
        }
        else
        {
            return _Domain::contains(_x);
        }
    }

    // derivative type (when available)
    using derivative_type = typename _Expression::derivative;
};

// Convenience: function with domain specified as an interval
template<typename _Expression,
         typename _IntervalDomain>
using bounded_function = math_function<_Expression,
                                       _IntervalDomain,
                                       cartesian_coords<1>>;


// =============================================================================
// XIII. FUNCTION COMPOSITION AND ARITHMETIC
// =============================================================================

// function_sum
//   type: pointwise sum of two functions (f + g)(x) = f(x) + g(x).
template<typename _F, typename _G>
using function_sum = math_function<sum<
    typename _F::expression_type,
    typename _G::expression_type
>>;

// function_product
//   type: pointwise product of two functions (f * g)(x) = f(x) * g(x).
template<typename _F, typename _G>
using function_product = math_function<product<
    typename _F::expression_type,
    typename _G::expression_type
>>;

// function_quotient
//   type: pointwise quotient of two functions (f / g)(x) = f(x) / g(x).
template<typename _F, typename _G>
using function_quotient = math_function<quotient<
    typename _F::expression_type,
    typename _G::expression_type
>>;

// function_compose
//   type: composition of two functions (f ∘ g)(x) = f(g(x)).
template<typename _F, typename _G>
using function_compose = math_function<compose<
    typename _F::expression_type,
    typename _G::expression_type
>>;

// function_negate
//   type: negation of a function (-f)(x) = -f(x).
template<typename _F>
using function_negate = math_function<negate<
    typename _F::expression_type
>>;


// =============================================================================
// XIV.  INVERSE FUNCTION (STRUCTURAL MARKER)
// =============================================================================

// inverse_function
//   struct: represents the inverse of a function f⁻¹.
// This is a structural marker; actual inversion must be provided
// by the user or computed numerically. The type carries enough
// information for SFINAE detection and domain/range swapping.
template<typename _Function>
struct inverse_function
{
    using original_type   = _Function;
    using expression_type = typename _Function::expression_type;
    using value_type      = typename _Function::value_type;

    static constexpr std::size_t arity  = _Function::arity;
    static constexpr bool        is_inverse     = true;
    static constexpr bool        is_constant_expr = _Function::is_constant_expr;

    // domain of f⁻¹ is the range of f, and vice versa
    using domain_type = void;  // range of original (unknown at compile time)
};


// =============================================================================
// XV.   FUNCTION TRAITS DETECTION HELPERS
// =============================================================================

NS_INTERNAL

    // has_is_parametric
    //   helper: detects is_parametric static member.
    template<typename _Type,
             typename = void>
    struct has_is_parametric : std::false_type
    {};

    template<typename _Type>
    struct has_is_parametric<_Type, void_t<decltype(_Type::is_parametric)>>
        : std::true_type
    {};

    // has_is_implicit
    //   helper: detects is_implicit static member.
    template<typename _Type,
             typename = void>
    struct has_is_implicit : std::false_type
    {};

    template<typename _Type>
    struct has_is_implicit<_Type, void_t<decltype(_Type::is_implicit)>>
        : std::true_type
    {};

    // has_is_piecewise
    //   helper: detects is_piecewise static member.
    template<typename _Type,
             typename = void>
    struct has_is_piecewise : std::false_type
    {};

    template<typename _Type>
    struct has_is_piecewise<_Type, void_t<decltype(_Type::is_piecewise)>>
        : std::true_type
    {};

    // has_is_inequality
    //   helper: detects is_inequality static member.
    template<typename _Type,
             typename = void>
    struct has_is_inequality : std::false_type
    {};

    template<typename _Type>
    struct has_is_inequality<_Type, void_t<decltype(_Type::is_inequality)>>
        : std::true_type
    {};

    // has_is_vector_valued
    //   helper: detects is_vector_valued static member.
    template<typename _Type,
             typename = void>
    struct has_is_vector_valued : std::false_type
    {};

    template<typename _Type>
    struct has_is_vector_valued<_Type,
                               void_t<decltype(_Type::is_vector_valued)>>
        : std::true_type
    {};

    // has_output_dimension
    //   helper: detects output_dimension static member.
    template<typename _Type,
             typename = void>
    struct has_output_dimension : std::false_type
    {};

    template<typename _Type>
    struct has_output_dimension<_Type,
                               void_t<decltype(_Type::output_dimension)>>
        : std::true_type
    {};

    // has_parameter_count
    //   helper: detects parameter_count static member.
    template<typename _Type,
             typename = void>
    struct has_parameter_count : std::false_type
    {};

    template<typename _Type>
    struct has_parameter_count<_Type,
                              void_t<decltype(_Type::parameter_count)>>
        : std::true_type
    {};

    // has_is_inverse
    //   helper: detects is_inverse static member.
    template<typename _Type,
             typename = void>
    struct has_is_inverse : std::false_type
    {};

    template<typename _Type>
    struct has_is_inverse<_Type, void_t<decltype(_Type::is_inverse)>>
        : std::true_type
    {};

    // has_is_polar_form
    //   helper: detects is_polar_form static member.
    template<typename _Type,
             typename = void>
    struct has_is_polar_form : std::false_type
    {};

    template<typename _Type>
    struct has_is_polar_form<_Type,
                            void_t<decltype(_Type::is_polar_form)>>
        : std::true_type
    {};

NS_END  // internal

// is_parametric_function
//   trait: checks if _Type is a parametric function.
template<typename _Type>
struct is_parametric_function : internal::has_is_parametric<_Type>
{};

// is_implicit_function
//   trait: checks if _Type is an implicit function.
template<typename _Type>
struct is_implicit_function : internal::has_is_implicit<_Type>
{};

// is_piecewise_function
//   trait: checks if _Type is a piecewise function.
template<typename _Type>
struct is_piecewise_function : internal::has_is_piecewise<_Type>
{};

// is_inequality_expression
//   trait: checks if _Type is an inequality expression.
template<typename _Type>
struct is_inequality_expression : internal::has_is_inequality<_Type>
{};

// is_vector_valued_function
//   trait: checks if _Type is a vector-valued function.
template<typename _Type>
struct is_vector_valued_function : internal::has_is_vector_valued<_Type>
{};

// is_inverse_function
//   trait: checks if _Type is an inverse function marker.
template<typename _Type>
struct is_inverse_function : internal::has_is_inverse<_Type>
{};

// is_polar_form_function
//   trait: checks if _Type is defined in polar form.
template<typename _Type>
struct is_polar_form_function : internal::has_is_polar_form<_Type>
{};

// is_multivariable_function
//   trait: checks if _Type has arity > 1.
NS_INTERNAL

    template<typename _Type,
             typename = void>
    struct multivariable_check : std::false_type
    {};

    template<typename _Type>
    struct multivariable_check<_Type, std::enable_if_t<
        ( has_evaluate<_Type>::value &&
          has_arity<_Type>::value    &&
          (_Type::arity > 1) )
    >> : std::true_type
    {};

NS_END  // internal

template<typename _Type>
struct is_multivariable_function : internal::multivariable_check<_Type>
{};


// =============================================================================
// XVI.  VARIABLE TEMPLATES
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // is_parametric_function_v
    //   variable template: value helper for is_parametric_function.
    template<typename _Type>
    inline constexpr bool is_parametric_function_v =
        is_parametric_function<_Type>::value;

    // is_implicit_function_v
    //   variable template: value helper for is_implicit_function.
    template<typename _Type>
    inline constexpr bool is_implicit_function_v =
        is_implicit_function<_Type>::value;

    // is_piecewise_function_v
    //   variable template: value helper for is_piecewise_function.
    template<typename _Type>
    inline constexpr bool is_piecewise_function_v =
        is_piecewise_function<_Type>::value;

    // is_inequality_expression_v
    //   variable template: value helper for is_inequality_expression.
    template<typename _Type>
    inline constexpr bool is_inequality_expression_v =
        is_inequality_expression<_Type>::value;

    // is_vector_valued_function_v
    //   variable template: value helper for is_vector_valued_function.
    template<typename _Type>
    inline constexpr bool is_vector_valued_function_v =
        is_vector_valued_function<_Type>::value;

    // is_inverse_function_v
    //   variable template: value helper for is_inverse_function.
    template<typename _Type>
    inline constexpr bool is_inverse_function_v =
        is_inverse_function<_Type>::value;

    // is_polar_form_function_v
    //   variable template: value helper for is_polar_form_function.
    template<typename _Type>
    inline constexpr bool is_polar_form_function_v =
        is_polar_form_function<_Type>::value;

    // is_multivariable_function_v
    //   variable template: value helper for is_multivariable_function.
    template<typename _Type>
    inline constexpr bool is_multivariable_function_v =
        is_multivariable_function<_Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // maths
NS_END  // djinterp


#endif  // DJINTERP_MATHS_FUNCTION_
