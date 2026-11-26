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
* HOW TO USE
* ----------
*   Include it, or math.hpp, from C++11, and bring the placeholders in with
* using namespace djinterp::math::variables. Build an expression from
* constant(v), the placeholders and the operators, then call it, e(3.0), at
* compile time or at run time; reify (expr/reify.hpp) makes it a term of the
* run-time language.
*
* OPTIONS
* -------
*   None: nothing in this header is chosen at run time.
*
* CONFIGURATION
* -------------
*   None of its own: it compiles from C++11 and is empty below it (N1). Its
* transcendentals are the C core's _c kernels, constant expressions from C++14.
*
*
* path:      /inc/djinterp/cpp/modules/math/expression.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.11.24
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  EXPRESSION BASE (CRTP) + STRUCTURAL DETECTION
    ---------------------------------------------
2.  SMALL VECTOR VALUE TYPE
    -----------------------
3.  CONSTEXPR MATH KERNEL + OPERATION FUNCTORS (INTERNAL)
    -----------------------------------------------------
4.  LEAVES: CONSTANT / VALUE
    ------------------------
5.  LEAVES: VARIABLE (POSITIONAL BINDING)
    -------------------------------------
6.  SCALAR LIFTING HELPER (INTERNAL)
    --------------------------------
7.  NODES: BINARY / UNARY
    ---------------------
8.  OPERATORS
    ---------
9.  FREE COMBINATORS
    ----------------
*/

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

//==============================================================================
// 1.  EXPRESSION BASE (CRTP) + STRUCTURAL DETECTION
//==============================================================================

// expression_base
//   struct: CRTP base every expression node inherits. Presence of this base
// is what is_expression / expression_c detect.
template<typename Derived>
struct expression_base
{
    using derived_type = Derived;

    D_CONSTEXPR const Derived&
    self() const noexcept
    {
        return static_cast<const Derived&>(*this);
    }
};

// is_expression / is_expression_v
//   trait: detects an expression node (derives expression_base<itself>).
template<typename Type>
struct is_expression
    : std::is_base_of<expression_base<typename std::decay<Type>::type>,
                      typename std::decay<Type>::type>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type>
D_INLINE_VAR constexpr bool is_expression_v = is_expression<Type>::value;
#endif

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
// expression_c
//   concept: parallel to is_expression for constraint syntax.
template<typename Type>
concept expression_c = is_expression<Type>::value;
#endif


//==============================================================================
// 2.  SMALL VECTOR VALUE TYPE
//==============================================================================
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


//==============================================================================
// 3.  CONSTEXPR MATH KERNEL + OPERATION FUNCTORS (INTERNAL)
//==============================================================================

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
        template<typename A, typename B>
        static D_CONSTEXPR auto apply(A _a, B _b) -> decltype(_a + _b)
        {
            return _a + _b;
        }
    };

    struct op_sub
    {
        template<typename A, typename B>
        static D_CONSTEXPR auto apply(A _a, B _b) -> decltype(_a - _b)
        {
            return _a - _b;
        }
    };

    struct op_mul
    {
        template<typename A, typename B>
        static D_CONSTEXPR auto apply(A _a, B _b) -> decltype(_a * _b)
        {
            return _a * _b;
        }
    };

    struct op_div
    {
        template<typename A, typename B>
        static D_CONSTEXPR auto apply(A _a, B _b) -> decltype(_a / _b)
        {
            return _a / _b;
        }
    };

    struct op_dot
    {
        template<typename A, typename B>
        static D_CONSTEXPR double apply(A _a, B _b)
        {
            return vec_dot(_a, _b);
        }
    };

    // ---- unary operation functors -----------------------------------------

    // The transcendental functors are constant expressions from C++14, where
    // the C core's kernels are (D_MATH_FN), and run-time calls below it.
    struct op_neg
    {
        template<typename X>
        static D_CONSTEXPR X apply(X _x)
        {
            return -_x;
        }
    };

    struct fn_exp
    {
        template<typename X>
        static D_CONSTEXPR_CPP14 double apply(X _x)
        {
            return d_math_exp_c(static_cast<double>(_x));
        }
    };

    struct fn_sin
    {
        template<typename X>
        static D_CONSTEXPR_CPP14 double apply(X _x)
        {
            return d_math_sin_c(static_cast<double>(_x));
        }
    };

    struct fn_cos
    {
        template<typename X>
        static D_CONSTEXPR_CPP14 double apply(X _x)
        {
            return d_math_cos_c(static_cast<double>(_x));
        }
    };

    struct fn_tan
    {
        template<typename X>
        static D_CONSTEXPR_CPP14 double apply(X _x)
        {
            return d_math_tan_c(static_cast<double>(_x));
        }
    };

    struct fn_sqrt
    {
        template<typename X>
        static D_CONSTEXPR_CPP14 double apply(X _x)
        {
            return d_math_sqrt_c(static_cast<double>(_x));
        }
    };

    struct fn_abs
    {
        template<typename X>
        static D_CONSTEXPR double apply(X _x)
        {
            return expr_abs(static_cast<double>(_x));
        }
    };

NS_END  // internal


//==============================================================================
// 4.  LEAVES: CONSTANT / VALUE
//==============================================================================

// constant_node
//   struct: leaf holding a stored value; ignores evaluation arguments.
template<typename ValueType>
struct constant_node : expression_base<constant_node<ValueType>>
{
    using value_type = ValueType;

    value_type value;

    static constexpr std::size_t arity = 0;

    D_CONSTEXPR explicit
    constant_node(value_type _v)
        : value(_v)
    {
    }

    template<typename... Args>
    D_CONSTEXPR value_type
    operator()(Args&&...) const noexcept
    {
        return value;
    }
};

// constant
//   factory: leaf holding a scalar, a numeric literal or any other value
// (a vec3). The factory once also named value is gone: math::value
// (value.hpp) is the generic value's class.
template<typename T>
D_CONSTEXPR constant_node<typename std::decay<T>::type>
constant(T _v)
{
    return constant_node<typename std::decay<T>::type>(_v);
}


//==============================================================================
// 5.  LEAVES: VARIABLE (POSITIONAL BINDING)
//==============================================================================

NS_INTERNAL

    // expr_pick
    //   the Index-th of the arguments as ValueType: std::get and
    // std::forward_as_tuple are constant expressions only from C++14.
    template<std::size_t Index,
             typename    ValueType>
    struct expr_pick
    {
        template<typename A, typename... Rest>
        static D_CONSTEXPR ValueType
        get(const A&, const Rest&... _rest) noexcept
        {
            return expr_pick<(Index - 1), ValueType>::get(_rest...);
        }
    };

    template<typename ValueType>
    struct expr_pick<0, ValueType>
    {
        template<typename A, typename... Rest>
        static D_CONSTEXPR ValueType
        get(const A& _a, const Rest&...) noexcept
        {
            return static_cast<ValueType>(_a);
        }
    };

NS_END  // internal

// variable_node
//   struct: independent variable bound to evaluation-argument index Index.
template<std::size_t Index,
         typename    ValueType = double>
struct variable_node : expression_base<variable_node<Index, ValueType>>
{
    using value_type = ValueType;

    static constexpr std::size_t index = Index;
    static constexpr std::size_t arity = Index + 1;

    template<typename... Args>
    D_CONSTEXPR value_type
    operator()(Args&&... _args) const noexcept
    {
        static_assert((sizeof...(Args) > Index),
                      "variable_node: not enough arguments for this index.");

        return internal::expr_pick<Index, value_type>::get(_args...);
    }
};

// var
//   factory: generic positional variable.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<std::size_t Index, typename T = double>
D_INLINE_VAR constexpr variable_node<Index, T> var{};
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


//==============================================================================
// 6.  SCALAR LIFTING HELPER (INTERNAL)
//==============================================================================

NS_INTERNAL

    // expr_of
    //   trait: the node as_expr makes of a T: T where it is an expression,
    // and otherwise its constant_node.
    template<typename T>
    struct expr_of
    {
        typedef typename std::conditional<
            is_expression<T>::value,
            typename std::decay<T>::type,
            constant_node<typename std::decay<T>::type> >::type type;
    };

    // as_expr: pass expressions through; wrap non-expressions in constant().
    template<typename T>
    D_CONSTEXPR
    typename std::enable_if<is_expression<T>::value, T>::type
    as_expr(T _e)
    {
        return _e;
    }

    template<typename T>
    D_CONSTEXPR
    typename std::enable_if<!is_expression<T>::value,
                            constant_node<typename std::decay<T>::type>>::type
    as_expr(T _v)
    {
        return constant(_v);
    }

NS_END  // internal


//==============================================================================
// 7.  NODES: BINARY / UNARY
//==============================================================================

// binary_node
//   struct: binary operation over two sub-expressions, stored by value.
template<typename Op,
         typename Left,
         typename Right>
struct binary_node : expression_base<binary_node<Op, Left, Right>>
{
    using op_type    = Op;
    using left_type  = Left;
    using right_type = Right;
    using value_type = decltype(Op::apply(
        std::declval<typename Left::value_type>(),
        std::declval<typename Right::value_type>()));

    Left  left;
    Right right;

    static constexpr std::size_t arity =
        (Left::arity > Right::arity) ? Left::arity : Right::arity;

    D_CONSTEXPR
    binary_node(Left _l, Right _r)
        : left(_l)
        , right(_r)
    {
    }

    template<typename... Args>
    D_CONSTEXPR value_type
    operator()(Args&&... _args) const
    {
        return Op::apply(left(_args...), right(_args...));
    }
};

// unary_node
//   struct: unary operation over one sub-expression, stored by value.
template<typename Op,
         typename Arg>
struct unary_node : expression_base<unary_node<Op, Arg>>
{
    using op_type    = Op;
    using arg_type   = Arg;
    using value_type = decltype(Op::apply(
        std::declval<typename Arg::value_type>()));

    Arg arg;

    static constexpr std::size_t arity = Arg::arity;

    D_CONSTEXPR explicit
    unary_node(Arg _a)
        : arg(_a)
    {
    }

    template<typename... Args>
    D_CONSTEXPR value_type
    operator()(Args&&... _args) const
    {
        return Op::apply(arg(_args...));
    }
};


//==============================================================================
// 8.  OPERATORS
//==============================================================================
// defined once; cover every node type; auto-lift scalars.

// One macro emits the three overloads (expr.expr, expr.scalar, scalar.expr)
// for an operator. Constraints are on the `expression` role, never on concrete
// node types, so a new node composes with no new operators.
#define D_EXPR_BINARY_OP(SYM, TAG)                                            \
    template<typename L, typename R,                                          \
             typename std::enable_if<is_expression<L>::value &&               \
                                     is_expression<R>::value,                 \
                                     int>::type = 0>                          \
    D_CONSTEXPR binary_node<internal::TAG, L, R>                              \
    operator SYM (L _l, R _r)                                                 \
    {                                                                         \
        return binary_node<internal::TAG, L, R>(_l, _r);                      \
    }                                                                         \
    template<typename L, typename S,                                          \
             typename std::enable_if<is_expression<L>::value &&               \
                                     !is_expression<S>::value &&              \
                                     std::is_arithmetic<                      \
                                         typename std::decay<S>::type         \
                                     >::value, int>::type = 0>                \
    D_CONSTEXPR binary_node<internal::TAG, L,                                 \
                            constant_node<typename std::decay<S>::type> >     \
    operator SYM (L _l, S _s)                                                 \
    {                                                                         \
        return binary_node<internal::TAG, L,                                  \
                           constant_node<typename std::decay<S>::type> >(     \
            _l, constant(_s));                                                \
    }                                                                         \
    template<typename S, typename R,                                          \
             typename std::enable_if<std::is_arithmetic<                      \
                                         typename std::decay<S>::type         \
                                     >::value &&                              \
                                     is_expression<R>::value,                 \
                                     int>::type = 0>                          \
    D_CONSTEXPR binary_node<internal::TAG,                                    \
                            constant_node<typename std::decay<S>::type>, R>   \
    operator SYM (S _s, R _r)                                                 \
    {                                                                         \
        return binary_node<internal::TAG,                                     \
                           constant_node<typename std::decay<S>::type>,       \
                           R>(constant(_s), _r);                              \
    }

D_EXPR_BINARY_OP(+, op_add)
D_EXPR_BINARY_OP(-, op_sub)
D_EXPR_BINARY_OP(*, op_mul)
D_EXPR_BINARY_OP(/, op_div)

#undef D_EXPR_BINARY_OP

// unary minus
template<typename A,
         typename std::enable_if<is_expression<A>::value, int>::type = 0>
D_CONSTEXPR unary_node<internal::op_neg, A>
operator-(A _a)
{
    return unary_node<internal::op_neg, A>(_a);
}


//==============================================================================
// 9.  FREE COMBINATORS
//==============================================================================
// elementwise functions; auto-lift scalar arguments.

// One macro per elementwise unary function.
#define D_EXPR_UNARY_FN(NAME, TAG)                                            \
    template<typename A>                                                      \
    D_CONSTEXPR unary_node<internal::TAG,                                     \
                           typename internal::expr_of<A>::type>               \
    NAME (A _a)                                                               \
    {                                                                         \
        return unary_node<internal::TAG,                                      \
                          typename internal::expr_of<A>::type>(               \
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
template<typename A>
D_CONSTEXPR binary_node<internal::op_mul,
                        typename internal::expr_of<A>::type,
                        typename internal::expr_of<A>::type>
squared(A _a)
{
    return binary_node<internal::op_mul,
                       typename internal::expr_of<A>::type,
                       typename internal::expr_of<A>::type>(
        internal::as_expr(_a), internal::as_expr(_a));
}

// pow
//   combinator: base^exponent via exp(exponent * ln-free path) is omitted in
// this core; integer/expression power composes through repeated product when
// needed. Provided here as a * ... is out of scope for the slice.

// dot
//   combinator: dot product of two vector-valued sub-expressions -> scalar.
template<typename A, typename B>
D_CONSTEXPR binary_node<internal::op_dot,
                        typename internal::expr_of<A>::type,
                        typename internal::expr_of<B>::type>
dot(A _a, B _b)
{
    return binary_node<internal::op_dot,
                       typename internal::expr_of<A>::type,
                       typename internal::expr_of<B>::type>(
        internal::as_expr(_a), internal::as_expr(_b));
}


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_EXPRESSION_HPP
