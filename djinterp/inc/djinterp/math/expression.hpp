/******************************************************************************
* djinterp [math]                                               expression.hpp
*
* Unified compile-time / runtime mathematical expression core.
*   Expression nodes hold their operands and literals BY VALUE and every
* constructor, operator, combinator and evaluation call is D_CONSTEXPR. The
* same node objects therefore evaluate at compile time (in a constexpr
* context) and at runtime, with no change of code:
*
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
*   - Leaves: constant(v) / value(v) (store a value), variable<I>/named
*     placeholders (read the I-th evaluation argument, positional binding).
*   - Nodes: binary_node<Op,L,R>, unary_node<Op,A> store sub-trees by value.
*   - Operators (+ - * /) and unary minus are defined ONCE, constrained to
*     operands modelling `expression`, so every node type composes and adding
*     a new node type needs zero new operators. Scalars auto-lift to constant.
*   - Transcendentals evaluate via a constexpr math kernel at compile time and
*     via <cmath> at runtime (std::is_constant_evaluated dispatch).
*
* 
* path:      /inc/djinterp/math/expression.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/

#ifndef DJINTERP_MATH_EXPRESSION_
#define DJINTERP_MATH_EXPRESSION_ 1

// std
#include <cstddef>
#include <cmath>
#include <tuple>
#include <utility>
#include <type_traits>
// djinterp
#include "../core/djinterp.hpp"
#include "./math.hpp"


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
{};

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
// dot(value(vec3{...}), p)). Kept here so leaves can carry vector values.

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
    // ---- constexpr scalar kernel (compile-time fallback for <cmath>) -------

    // cabs
    inline D_CONSTEXPR double
    cabs(double _x) noexcept
    {
        return (_x < 0.0) ? -_x : _x;
    }

    // csqrt: Newton-Raphson.
    inline D_CONSTEXPR double
    csqrt(double _x) noexcept
    {
        if (_x <= 0.0)
        {
            return 0.0;
        }

        double g = _x;

        for (int i = 0; i < 100; ++i)
        {
            g = 0.5 * (g + _x / g);
        }

        return g;
    }

    // cexp: range-halve to |a| <= 0.5, Maclaurin series, then re-square.
    inline D_CONSTEXPR double
    cexp(double _x) noexcept
    {
        const bool neg = (_x < 0.0);
        double     a   = neg ? -_x : _x;
        int        k   = 0;

        while (a > 0.5)
        {
            a *= 0.5;
            ++k;
        }

        double term = 1.0;
        double sum  = 1.0;

        for (int n = 1; n < 24; ++n)
        {
            term *= a / static_cast<double>(n);
            sum  += term;
        }

        for (int i = 0; i < k; ++i)
        {
            sum *= sum;
        }

        return neg ? (1.0 / sum) : sum;
    }

    // creduce: bring an angle into approximately [-pi, pi].
    inline D_CONSTEXPR double
    creduce(double _x) noexcept
    {
        const double two_pi = 6.283185307179586476925286766559;
        const double q      = _x / two_pi;
        const double r      = (q >= 0.0) ? (q + 0.5) : (q - 0.5);
        const long long m   = static_cast<long long>(r);

        return _x - static_cast<double>(m) * two_pi;
    }

    // csin
    inline D_CONSTEXPR double
    csin(double _x) noexcept
    {
        const double y  = creduce(_x);
        const double y2 = y * y;
        double       term = y;
        double       sum  = y;

        for (int n = 1; n < 16; ++n)
        {
            term *= -y2 / static_cast<double>((2 * n) * (2 * n + 1));
            sum  += term;
        }

        return sum;
    }

    // ccos
    inline D_CONSTEXPR double
    ccos(double _x) noexcept
    {
        const double y  = creduce(_x);
        const double y2 = y * y;
        double       term = 1.0;
        double       sum  = 1.0;

        for (int n = 1; n < 16; ++n)
        {
            term *= -y2 / static_cast<double>((2 * n - 1) * (2 * n));
            sum  += term;
        }

        return sum;
    }

    // vec_dot
    //   helper: dot product of two vec3 values.
    inline D_CONSTEXPR double
    vec_dot(const vec3& _a, const vec3& _b) noexcept
    {
        return (_a.x * _b.x) + (_a.y * _b.y) + (_a.z * _b.z);
    }

    // ctan
    inline D_CONSTEXPR double
    ctan(double _x) noexcept
    {
        return csin(_x) / ccos(_x);
    }

    // ---- binary operation functors ----------------------------------------

    struct op_add
    {
        template<typename _A, typename _B>
        static D_CONSTEXPR auto apply(_A _a, _B _b) { return _a + _b; }
    };

    struct op_sub
    {
        template<typename _A, typename _B>
        static D_CONSTEXPR auto apply(_A _a, _B _b) { return _a - _b; }
    };

    struct op_mul
    {
        template<typename _A, typename _B>
        static D_CONSTEXPR auto apply(_A _a, _B _b) { return _a * _b; }
    };

    struct op_div
    {
        template<typename _A, typename _B>
        static D_CONSTEXPR auto apply(_A _a, _B _b) { return _a / _b; }
    };

    struct op_dot
    {
        template<typename _A, typename _B>
        static D_CONSTEXPR double apply(_A _a, _B _b) { return vec_dot(_a, _b); }
    };

    // ---- unary operation functors -----------------------------------------

    // D_MATH_DISPATCH: pick the constexpr kernel during constant evaluation and
    // libm at runtime where std::is_constant_evaluated is available (C++20);
    // otherwise always use the constexpr kernel (valid in both contexts).
#if D_ENV_CPP_FEATURE_STL_IS_CONSTANT_EVALUATED
    #define D_MATH_DISPATCH(KERNEL, STDFN, ARG) \
        (std::is_constant_evaluated() ? KERNEL(ARG) : STDFN(ARG))
#else
    #define D_MATH_DISPATCH(KERNEL, STDFN, ARG) (KERNEL(ARG))
#endif

    struct op_neg
    {
        template<typename _X>
        static D_CONSTEXPR _X apply(_X _x) { return -_x; }
    };

    struct fn_exp
    {
        template<typename _X>
        static D_CONSTEXPR double apply(_X _x)
        {
            return D_MATH_DISPATCH(cexp, std::exp, static_cast<double>(_x));
        }
    };

    struct fn_sin
    {
        template<typename _X>
        static D_CONSTEXPR double apply(_X _x)
        {
            return D_MATH_DISPATCH(csin, std::sin, static_cast<double>(_x));
        }
    };

    struct fn_cos
    {
        template<typename _X>
        static D_CONSTEXPR double apply(_X _x)
        {
            return D_MATH_DISPATCH(ccos, std::cos, static_cast<double>(_x));
        }
    };

    struct fn_tan
    {
        template<typename _X>
        static D_CONSTEXPR double apply(_X _x)
        {
            return D_MATH_DISPATCH(ctan, std::tan, static_cast<double>(_x));
        }
    };

    struct fn_sqrt
    {
        template<typename _X>
        static D_CONSTEXPR double apply(_X _x)
        {
            return D_MATH_DISPATCH(csqrt, std::sqrt, static_cast<double>(_x));
        }
    };

    struct fn_abs
    {
        template<typename _X>
        static D_CONSTEXPR double apply(_X _x)
        {
            return cabs(static_cast<double>(_x));
        }
    };

#undef D_MATH_DISPATCH

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
//   factory: scalar / numeric literal leaf.
template<typename _T>
D_CONSTEXPR constant_node<typename std::decay<_T>::type>
constant(_T _v)
{
    return constant_node<typename std::decay<_T>::type>(_v);
}

// value
//   factory: leaf wrapping an arbitrary value (e.g. a vec3).
template<typename _T>
D_CONSTEXPR constant_node<typename std::decay<_T>::type>
value(_T _v)
{
    return constant_node<typename std::decay<_T>::type>(_v);
}


// ===========================================================================
// V.   LEAVES: VARIABLE (positional binding)
// ===========================================================================

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

        return static_cast<value_type>(
            std::get<_Index>(
                std::forward_as_tuple(std::forward<_Args>(_args)...)));
    }
};

// var
//   factory: generic positional variable.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<std::size_t _Index, typename _T = double>
D_INLINE_VAR constexpr variable_node<_Index, _T> var{};
#endif

// Named scalar placeholders (positional indices). Different names may share an
// index (e.g. x and r are both argument 0); do not mix naming schemes within
// one function.
D_INLINE_VAR constexpr variable_node<0> x{};
D_INLINE_VAR constexpr variable_node<1> y{};
D_INLINE_VAR constexpr variable_node<2> z{};
D_INLINE_VAR constexpr variable_node<3> t{};

D_INLINE_VAR constexpr variable_node<0> r{};
D_INLINE_VAR constexpr variable_node<0> rho{};
D_INLINE_VAR constexpr variable_node<1> theta{};
D_INLINE_VAR constexpr variable_node<2> phi{};


// ===========================================================================
// VI.  SCALAR LIFTING HELPER (internal)
// ===========================================================================

NS_INTERNAL

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
    D_CONSTEXPR auto
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
    D_CONSTEXPR auto
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
    D_CONSTEXPR auto operator SYM (_L _l, _R _r)                              \
    {                                                                         \
        return binary_node<internal::TAG, _L, _R>(_l, _r);                    \
    }                                                                         \
    template<typename _L, typename _S,                                        \
             typename std::enable_if<is_expression<_L>::value &&              \
                                     !is_expression<_S>::value &&             \
                                     std::is_arithmetic<                      \
                                         typename std::decay<_S>::type        \
                                     >::value, int>::type = 0>                \
    D_CONSTEXPR auto operator SYM (_L _l, _S _s)                              \
    {                                                                         \
        return _l SYM constant(_s);                                          \
    }                                                                         \
    template<typename _S, typename _R,                                        \
             typename std::enable_if<std::is_arithmetic<                      \
                                         typename std::decay<_S>::type        \
                                     >::value &&                              \
                                     is_expression<_R>::value,                \
                                     int>::type = 0>                          \
    D_CONSTEXPR auto operator SYM (_S _s, _R _r)                              \
    {                                                                         \
        return constant(_s) SYM _r;                                          \
    }

D_EXPR_BINARY_OP(+, op_add)
D_EXPR_BINARY_OP(-, op_sub)
D_EXPR_BINARY_OP(*, op_mul)
D_EXPR_BINARY_OP(/, op_div)

#undef D_EXPR_BINARY_OP

// unary minus
template<typename _A,
         typename std::enable_if<is_expression<_A>::value, int>::type = 0>
D_CONSTEXPR auto
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
    D_CONSTEXPR auto NAME (_A _a)                                             \
    {                                                                         \
        auto e = internal::as_expr(_a);                                       \
        return unary_node<internal::TAG, decltype(e)>(e);                     \
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
D_CONSTEXPR auto
squared(_A _a)
{
    auto e = internal::as_expr(_a);
    return e * e;
}

// pow
//   combinator: base^exponent via exp(exponent * ln-free path) is omitted in
// this core; integer/expression power composes through repeated product when
// needed. Provided here as a * ... is out of scope for the slice.

// dot
//   combinator: dot product of two vector-valued sub-expressions -> scalar.
template<typename _A, typename _B>
D_CONSTEXPR auto
dot(_A _a, _B _b)
{
    auto ea = internal::as_expr(_a);
    auto eb = internal::as_expr(_b);

    return binary_node<internal::op_dot, decltype(ea), decltype(eb)>(ea, eb);
}


NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_EXPRESSION_
