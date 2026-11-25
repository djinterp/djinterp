/*******************************************************************************
* djinterp [cpp][math]                                            expression.hpp
*
* Unified compile-time / runtime mathematical expression core.
*   Expression nodes hold their operands and literals BY VALUE and every
* constructor, operator, combinator and evaluation call is D_CONSTEXPR. The
* same node objects therefore evaluate at compile time (in a constexpr
* context) and at runtime, with no change of code:
*
*     using namespace djinterp::math::variables;          // x, y, ... phi
*     constexpr auto e = constant(2.0) * squared(x);   // compile time
*     constexpr double a = e(3.0);                     // == 18.0, constant-expr
*     auto e2 = constant(2.0) * squared(x);             // runtime
*     double b = e2(3.0);                              // == 18.0, at runtime
*
* This supersedes the former value-as-non-type-template-parameter model
* (constant<T, V>), which was compile-time only.
*
* DESIGN
*   - expression_base<Derived> (CRTP) tags every node; is_expression / the
*     `expression_c` concept detect nodes structurally.
*   - Leaves: constant(v) (store a value), variable<I>/named
*     placeholders (read the I-th evaluation argument, positional binding).
*   - Nodes: binary_node<Op,L,R>, unary_node<Op,A> store sub-trees by value.
*   - Operators (+ - * /) and unary minus are defined ONCE, constrained to
*     operands modelling `expression`, so every node type composes and adding
*     a new node type needs zero new operators. Scalars auto-lift to constant.
*   - Transcendentals evaluate by the C core's correctly rounded or
*     faithful `_c` kernels, d_math_exp_c, d_math_sin_c, d_math_cos_c,
*     d_math_tan_c and d_math_sqrt_c, at compile time and at run time alike,
*     so that one call has one answer at every level (N3, S430); abs is
*     exact.
*
* LEVELS (step 29, 5a; S430)
*   C++11 is the floor, and the header is empty below it (N1). From C++11
* the nodes, the leaves, + - * / and unary minus, squared, abs and dot are
* constant expressions; exp, sin, cos, tan and sqrt are from C++14, where the
* C core's kernels are (D_MATH_FN), and evaluate at run time below it. The
* named placeholders x, y, z, t, r, rho, theta and phi live in the namespace
* djinterp::math::variables, so that no local of those names in a function
* of djinterp::math shadows them (S403, ruled 2026.11.16); var<I, T>, a
* variable template, is from C++14. On a 32-bit x87, where double is
* evaluated wider, exp is refused at compile time, as the double family's
* correctly rounded functions are (S418, S426).
*
*
* path:      /inc/djinterp/cpp/modules/math/expression.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.11.20
*******************************************************************************/

#ifndef DJINTERP_MATH_EXPRESSION_HPP
#define DJINTERP_MATH_EXPRESSION_HPP 1

// djinterp
#include "../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>      // std::size_t
#include <type_traits>  // std::enable_if, std::is_base_of, std::decay
// djinterp
#include "../../../c/modules/math/elementary.h"   // d_math_sqrt_c, sin_c, cos_c
#include "../../../c/modules/math/exponential.h"  // d_math_exp_c
#include "../../../c/modules/math/tangent.h"      // d_math_tan_c


// D_INLINE_VAR: `inline` for namespace-scope constexpr variables under C++17+
// (one shared, ODR-merged object). Under C++14 it is empty: a namespace-scope
// constexpr variable already has internal linkage, so each TU gets its own
// copy with no ODR conflict. (Provided by the djinterp env header when
// present.)
#ifndef D_INLINE_VAR
    #if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
        #define D_INLINE_VAR inline
    #else
        #define D_INLINE_VAR
    #endif
#endif


NS_DJINTERP
NS_MATH

// ===========================================================================
// I.   EXPRESSION BASE (CRTP) + STRUCTURAL DETECTION
// ===========================================================================

// expression_base
//   struct: CRTP base every expression node inherits. Presence of this base
// is what is_expression / expression_c detect.
template<typename _Derived>
struct expression_base
{
    using derived_type = _Derived;

    D_CONSTEXPR const _Derived&
    self() const noexcept
    {
        return static_cast<const _Derived&>(*this);
    }
};

// is_expression / is_expression_v
//   trait: detects an expression node (derives expression_base<itself>).
template<typename _Type>
struct is_expression
    : std::is_base_of<expression_base<typename std::decay<_Type>::type>,
                      typename std::decay<_Type>::type>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename _Type>
D_INLINE_VAR constexpr bool is_expression_v = is_expression<_Type>::value;
#endif

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
// expression_c
//   concept: parallel to is_expression for constraint syntax.
template<typename _Type>
concept expression_c = is_expression<_Type>::value;
#endif


// ===========================================================================
// II.  SMALL VECTOR VALUE TYPE
// ===========================================================================
// A minimal literal vector usable as an expression value_type (e.g. for
// dot(constant(vec3{...}), p)). Kept here so leaves can carry vector values.

// vec3
//   struct: literal 3-vector with constexpr access and arithmetic.
struct vec3
{
    double x;
    double y;
    double z;

    D_CONSTEXPR double
    operator[](std::size_t _i) const noexcept
    {
        return (_i == 0) ? x : (_i == 1) ? y : z;
    }

    friend D_CONSTEXPR vec3
    operator-(const vec3& _v) noexcept
    {
        return vec3{ -_v.x, -_v.y, -_v.z };
    }
};


// ===========================================================================
// III. CONSTEXPR MATH KERNEL + OPERATION FUNCTORS (internal)
// ===========================================================================

NS_INTERNAL

    // ---- scalar kernels -------------------------------------------------
    // exp, sin, cos, tan and sqrt are the C core's `_c` kernels, one answer
    // at compile time and at run time (N3, S430); the kernels this header
    // once had of its own gave sqrt(1e300) as 7.89e269 and exp up to 10,800
    // ulps off, and a C++20 program the C library's answer at run time and
    // theirs in a constant expression.

    // expr_abs
    //   |_x|, exact: -0 gives +0, as fabs does; a NaN passes.
    inline D_CONSTEXPR double
    expr_abs(double _x) noexcept
    {
        return (_x < 0.0) ? -_x : ( (_x == 0.0) ? 0.0 : _x );
    }

    // vec_dot
    //   helper: dot product of two vec3 values.
    inline D_CONSTEXPR double
    vec_dot(const vec3& _a, const vec3& _b) noexcept
    {
        return (_a.x * _b.x) + (_a.y * _b.y) + (_a.z * _b.z);
    }

    // ---- binary operation functors ----------------------------------------

    struct op_add
    {
        template<typename _A, typename _B>
        static D_CONSTEXPR auto apply(_A _a, _B _b) -> decltype(_a + _b)
        {
            return _a + _b;
        }
    };

    struct op_sub
    {
        template<typename _A, typename _B>
        static D_CONSTEXPR auto apply(_A _a, _B _b) -> decltype(_a - _b)
        {
            return _a - _b;
        }
    };

    struct op_mul
    {
        template<typename _A, typename _B>
        static D_CONSTEXPR auto apply(_A _a, _B _b) -> decltype(_a * _b)
        {
            return _a * _b;
        }
    };

    struct op_div
    {
        template<typename _A, typename _B>
        static D_CONSTEXPR auto apply(_A _a, _B _b) -> decltype(_a / _b)
        {
            return _a / _b;
        }
    };

    struct op_dot
    {
        template<typename _A, typename _B>
        static D_CONSTEXPR double apply(_A _a, _B _b)
        {
            return vec_dot(_a, _b);
        }
    };

    // ---- unary operation functors -----------------------------------------

    // The transcendental functors are constant expressions from C++14, where
    // the C core's kernels are (D_MATH_FN), and run-time calls below it.
    struct op_neg
    {
        template<typename _X>
        static D_CONSTEXPR _X apply(_X _x)
        {
            return -_x;
        }
    };

    struct fn_exp
    {
        template<typename _X>
        static D_CONSTEXPR_CPP14 double apply(_X _x)
        {
            return d_math_exp_c(static_cast<double>(_x));
        }
    };

    struct fn_sin
    {
        template<typename _X>
        static D_CONSTEXPR_CPP14 double apply(_X _x)
        {
            return d_math_sin_c(static_cast<double>(_x));
        }
    };

    struct fn_cos
    {
        template<typename _X>
        static D_CONSTEXPR_CPP14 double apply(_X _x)
        {
            return d_math_cos_c(static_cast<double>(_x));
        }
    };

    struct fn_tan
    {
        template<typename _X>
        static D_CONSTEXPR_CPP14 double apply(_X _x)
        {
            return d_math_tan_c(static_cast<double>(_x));
        }
    };

    struct fn_sqrt
    {
        template<typename _X>
        static D_CONSTEXPR_CPP14 double apply(_X _x)
        {
            return d_math_sqrt_c(static_cast<double>(_x));
        }
    };

    struct fn_abs
    {
        template<typename _X>
        static D_CONSTEXPR double apply(_X _x)
        {
            return expr_abs(static_cast<double>(_x));
        }
    };

NS_END  // internal


// ===========================================================================
// IV.  LEAVES: CONSTANT / VALUE
// ===========================================================================

// constant_node
//   struct: leaf holding a stored value; ignores evaluation arguments.
template<typename _ValueType>
struct constant_node : expression_base<constant_node<_ValueType>>
{
    using value_type = _ValueType;

    value_type value;

    static constexpr std::size_t arity = 0;

    D_CONSTEXPR explicit
    constant_node(value_type _v)
        : value(_v)
    {
    }

    template<typename... _Args>
    D_CONSTEXPR value_type
    operator()(_Args&&...) const noexcept
    {
        return value;
    }
};

// constant
//   factory: leaf holding a scalar, a numeric literal or any other value
// (a vec3). The factory once also named value is gone: math::value
// (value.hpp) is the generic value's class.
template<typename _T>
D_CONSTEXPR constant_node<typename std::decay<_T>::type>
constant(_T _v)
{
    return constant_node<typename std::decay<_T>::type>(_v);
}


// ===========================================================================
// V.   LEAVES: VARIABLE (positional binding)
// ===========================================================================

NS_INTERNAL

    // expr_pick
    //   the _Index-th of the arguments as _ValueType: std::get and
    // std::forward_as_tuple are constant expressions only from C++14.
    template<std::size_t _Index,
             typename    _ValueType>
    struct expr_pick
    {
        template<typename _A, typename... _Rest>
        static D_CONSTEXPR _ValueType
        get(const _A&, const _Rest&... _rest) noexcept
        {
            return expr_pick<(_Index - 1), _ValueType>::get(_rest...);
        }
    };

    template<typename _ValueType>
    struct expr_pick<0, _ValueType>
    {
        template<typename _A, typename... _Rest>
        static D_CONSTEXPR _ValueType
        get(const _A& _a, const _Rest&...) noexcept
        {
            return static_cast<_ValueType>(_a);
        }
    };

NS_END  // internal

// variable_node
//   struct: independent variable bound to evaluation-argument index _Index.
template<std::size_t _Index,
         typename    _ValueType = double>
struct variable_node : expression_base<variable_node<_Index, _ValueType>>
{
    using value_type = _ValueType;

    static constexpr std::size_t index = _Index;
    static constexpr std::size_t arity = _Index + 1;

    template<typename... _Args>
    D_CONSTEXPR value_type
    operator()(_Args&&... _args) const noexcept
    {
        static_assert((sizeof...(_Args) > _Index),
                      "variable_node: not enough arguments for this index.");

        return internal::expr_pick<_Index, value_type>::get(_args...);
    }
};

// var
//   factory: generic positional variable.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<std::size_t _Index, typename _T = double>
D_INLINE_VAR constexpr variable_node<_Index, _T> var{};
#endif

// variables
//   namespace: the named scalar placeholders (positional indices), in a
// namespace of their own, so that a local x or theta in a function of
// djinterp::math shadows nothing (S403, ruled 2026.11.16); bring them in
// with `using namespace djinterp::math::variables;`. Different names may
// share an index (e.g. x and r are both argument 0); do not mix naming
// schemes within one function. (function.hpp's cartesian_axes and the rest
// give each coordinate system's names at its own indices.)
namespace variables
{
    D_INLINE_VAR constexpr variable_node<0> x{};
    D_INLINE_VAR constexpr variable_node<1> y{};
    D_INLINE_VAR constexpr variable_node<2> z{};
    D_INLINE_VAR constexpr variable_node<3> t{};

    D_INLINE_VAR constexpr variable_node<0> r{};
    D_INLINE_VAR constexpr variable_node<0> rho{};
    D_INLINE_VAR constexpr variable_node<1> theta{};
    D_INLINE_VAR constexpr variable_node<2> phi{};
}  // namespace variables


// ===========================================================================
// VI.  SCALAR LIFTING HELPER (internal)
// ===========================================================================

NS_INTERNAL

    // expr_of
    //   trait: the node as_expr makes of a _T: _T where it is an expression,
    // and otherwise its constant_node.
    template<typename _T>
    struct expr_of
    {
        typedef typename std::conditional<
            is_expression<_T>::value,
            typename std::decay<_T>::type,
            constant_node<typename std::decay<_T>::type> >::type type;
    };

    // as_expr: pass expressions through; wrap non-expressions in constant().
    template<typename _T>
    D_CONSTEXPR
    typename std::enable_if<is_expression<_T>::value, _T>::type
    as_expr(_T _e)
    {
        return _e;
    }

    template<typename _T>
    D_CONSTEXPR
    typename std::enable_if<!is_expression<_T>::value,
                            constant_node<typename std::decay<_T>::type>>::type
    as_expr(_T _v)
    {
        return constant(_v);
    }

NS_END  // internal


// ===========================================================================
// VII. NODES: BINARY / UNARY
// ===========================================================================

// binary_node
//   struct: binary operation over two sub-expressions, stored by value.
template<typename _Op,
         typename _Left,
         typename _Right>
struct binary_node : expression_base<binary_node<_Op, _Left, _Right>>
{
    using op_type    = _Op;
    using left_type  = _Left;
    using right_type = _Right;
    using value_type = decltype(_Op::apply(
        std::declval<typename _Left::value_type>(),
        std::declval<typename _Right::value_type>()));

    _Left  left;
    _Right right;

    static constexpr std::size_t arity =
        (_Left::arity > _Right::arity) ? _Left::arity : _Right::arity;

    D_CONSTEXPR
    binary_node(_Left _l, _Right _r)
        : left(_l)
        , right(_r)
    {
    }

    template<typename... _Args>
    D_CONSTEXPR value_type
    operator()(_Args&&... _args) const
    {
        return _Op::apply(left(_args...), right(_args...));
    }
};

// unary_node
//   struct: unary operation over one sub-expression, stored by value.
template<typename _Op,
         typename _Arg>
struct unary_node : expression_base<unary_node<_Op, _Arg>>
{
    using op_type    = _Op;
    using arg_type   = _Arg;
    using value_type = decltype(_Op::apply(
        std::declval<typename _Arg::value_type>()));

    _Arg arg;

    static constexpr std::size_t arity = _Arg::arity;

    D_CONSTEXPR explicit
    unary_node(_Arg _a)
        : arg(_a)
    {
    }

    template<typename... _Args>
    D_CONSTEXPR value_type
    operator()(_Args&&... _args) const
    {
        return _Op::apply(arg(_args...));
    }
};


// ===========================================================================
// VIII. OPERATORS (defined once; cover every node type; auto-lift scalars)
// ===========================================================================

// One macro emits the three overloads (expr.expr, expr.scalar, scalar.expr)
// for an operator. Constraints are on the `expression` role, never on concrete
// node types, so a new node composes with no new operators.
#define D_EXPR_BINARY_OP(SYM, TAG)                                            \
    template<typename _L, typename _R,                                        \
             typename std::enable_if<is_expression<_L>::value &&              \
                                     is_expression<_R>::value,                \
                                     int>::type = 0>                          \
    D_CONSTEXPR binary_node<internal::TAG, _L, _R>                            \
    operator SYM (_L _l, _R _r)                                               \
    {                                                                         \
        return binary_node<internal::TAG, _L, _R>(_l, _r);                    \
    }                                                                         \
    template<typename _L, typename _S,                                        \
             typename std::enable_if<is_expression<_L>::value &&              \
                                     !is_expression<_S>::value &&             \
                                     std::is_arithmetic<                      \
                                         typename std::decay<_S>::type        \
                                     >::value, int>::type = 0>                \
    D_CONSTEXPR binary_node<internal::TAG, _L,                                \
                            constant_node<typename std::decay<_S>::type> >    \
    operator SYM (_L _l, _S _s)                                               \
    {                                                                         \
        return binary_node<internal::TAG, _L,                                 \
                           constant_node<typename std::decay<_S>::type> >(    \
            _l, constant(_s));                                                \
    }                                                                         \
    template<typename _S, typename _R,                                        \
             typename std::enable_if<std::is_arithmetic<                      \
                                         typename std::decay<_S>::type        \
                                     >::value &&                              \
                                     is_expression<_R>::value,                \
                                     int>::type = 0>                          \
    D_CONSTEXPR binary_node<internal::TAG,                                    \
                            constant_node<typename std::decay<_S>::type>, _R> \
    operator SYM (_S _s, _R _r)                                               \
    {                                                                         \
        return binary_node<internal::TAG,                                     \
                           constant_node<typename std::decay<_S>::type>,      \
                           _R>(constant(_s), _r);                             \
    }

D_EXPR_BINARY_OP(+, op_add)
D_EXPR_BINARY_OP(-, op_sub)
D_EXPR_BINARY_OP(*, op_mul)
D_EXPR_BINARY_OP(/, op_div)

#undef D_EXPR_BINARY_OP

// unary minus
template<typename _A,
         typename std::enable_if<is_expression<_A>::value, int>::type = 0>
D_CONSTEXPR unary_node<internal::op_neg, _A>
operator-(_A _a)
{
    return unary_node<internal::op_neg, _A>(_a);
}


// ===========================================================================
// IX.  FREE COMBINATORS (elementwise functions; auto-lift scalar arguments)
// ===========================================================================

// One macro per elementwise unary function.
#define D_EXPR_UNARY_FN(NAME, TAG)                                            \
    template<typename _A>                                                     \
    D_CONSTEXPR unary_node<internal::TAG,                                     \
                           typename internal::expr_of<_A>::type>              \
    NAME (_A _a)                                                              \
    {                                                                         \
        return unary_node<internal::TAG,                                      \
                          typename internal::expr_of<_A>::type>(              \
            internal::as_expr(_a));                                           \
    }

D_EXPR_UNARY_FN(exp,  fn_exp)
D_EXPR_UNARY_FN(sin,  fn_sin)
D_EXPR_UNARY_FN(cos,  fn_cos)
D_EXPR_UNARY_FN(tan,  fn_tan)
D_EXPR_UNARY_FN(sqrt, fn_sqrt)
D_EXPR_UNARY_FN(abs,  fn_abs)

#undef D_EXPR_UNARY_FN

// squared
//   combinator: a * a (no transcendental kernel needed). Named `squared`
//   rather than `square` so it does not collide with the geometry square<>
//   shape, which shares the djinterp::math namespace.
template<typename _A>
D_CONSTEXPR binary_node<internal::op_mul,
                        typename internal::expr_of<_A>::type,
                        typename internal::expr_of<_A>::type>
squared(_A _a)
{
    return binary_node<internal::op_mul,
                       typename internal::expr_of<_A>::type,
                       typename internal::expr_of<_A>::type>(
        internal::as_expr(_a), internal::as_expr(_a));
}

// pow
//   combinator: base^exponent via exp(exponent * ln-free path) is omitted in
// this core; integer/expression power composes through repeated product when
// needed. Provided here as a * ... is out of scope for the slice.

// dot
//   combinator: dot product of two vector-valued sub-expressions -> scalar.
template<typename _A, typename _B>
D_CONSTEXPR binary_node<internal::op_dot,
                        typename internal::expr_of<_A>::type,
                        typename internal::expr_of<_B>::type>
dot(_A _a, _B _b)
{
    return binary_node<internal::op_dot,
                       typename internal::expr_of<_A>::type,
                       typename internal::expr_of<_B>::type>(
        internal::as_expr(_a), internal::as_expr(_b));
}


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_EXPRESSION_HPP
