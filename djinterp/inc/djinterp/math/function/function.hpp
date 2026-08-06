/******************************************************************************
* djinterp [math]                                                function.hpp
*
* Named functions, higher-order forms, and the fluent builder.
*   This header sits on top of expression.hpp (the value-holding expression
* core) and coordinate.hpp (the real coordinate systems). It provides:
*
*     - the detection layer folded in from the old function_traits.hpp
*       (is_inequality / is_piecewise / is_parametric / is_implicit /
*        is_vector_valued / is_function / is_coordinate_system, plus the
*        per-system predicates), driven by the same marker-member contract;
*     - a relational layer  (<, <=, >, >=, ==, !=) and a logical layer
*       (&&, ||, !) producing boolean-valued expressions;
*     - piecewise functions  (when / otherwise / piecewise);
*     - vector-valued functions  (vec / vector_function);
*     - parametric curves and surfaces  (parametric_curve / parametric_surface);
*     - implicit functions and level sets  (implicit / level_set);
*     - math_function, the callable top-level wrapper, with scalar-pack,
*       std::array, and std::tuple evaluation; and
*     - the system-aware builder  function(system).of(...).returns(EXPR).
*
* Unlike the original (compile-time-only, static evaluate), every form here
* holds its operands by value and exposes a single constexpr operator(), so
* the same object evaluates identically at compile time and at run time.
*
* MARKER CONTRACT (unchanged from function_traits.hpp):
*     Inequality:     static constexpr bool is_inequality    = true;
*     Piecewise:      static constexpr bool is_piecewise      = true;  pieces_type
*     Parametric:     static constexpr bool is_parametric     = true;  parameter_count
*     Implicit:       static constexpr bool is_implicit       = true;
*     Vector-valued:  static constexpr bool is_vector_valued  = true;  output_dimension
*     Named function: static constexpr bool is_math_function  = true;
*
* path:      /inc/djinterp/math/function/function.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       date: 2026.02.06
******************************************************************************/

#ifndef DJINTERP_MATH_FUNCTION_
#define DJINTERP_MATH_FUNCTION_ 1

#include <cstddef>
#include <array>
#include <tuple>
#include <utility>
#include <type_traits>

#include "../djinterp.hpp"
#include "./expression.hpp"
#include "./coordinate.hpp"


NS_DJINTERP
NS_MATH

// ============================================================================
// I.    INTERNAL SUPPORT
// ============================================================================

NS_INTERNAL

    // void_t
    //   alias: maps any type list to void (detection idiom). std::void_t is
    // C++17; this keeps the header usable from C++14.
    template<typename...>
    using void_t = void;

    // max_size_t
    //   meta: compile-time maximum of a pack of std::size_t (0 for empty).
    template<std::size_t...>
    struct max_size_t;

    template<>
    struct max_size_t<>
    {
        static constexpr std::size_t value = 0;
    };

    template<std::size_t _N>
    struct max_size_t<_N>
    {
        static constexpr std::size_t value = _N;
    };

    template<std::size_t _N, std::size_t... _Rest>
    struct max_size_t<_N, _Rest...>
    {
        static constexpr std::size_t value =
            (_N > max_size_t<_Rest...>::value)
                ? _N
                : max_size_t<_Rest...>::value;
    };

    // is_std_array / is_std_tuple / first_is_container
    //   traits: recognise std::array and std::tuple, and detect the
    // "single container argument" case used to disambiguate math_function's
    // scalar-pack operator() from its array/tuple overloads.
    template<typename>
    struct is_std_array : std::false_type {};

    template<typename _T, std::size_t _N>
    struct is_std_array<std::array<_T, _N>> : std::true_type {};

    template<typename>
    struct is_std_tuple : std::false_type {};

    template<typename... _T>
    struct is_std_tuple<std::tuple<_T...>> : std::true_type {};

    template<typename... _Args>
    struct first_is_container : std::false_type {};

    template<typename _First, typename... _Rest>
    struct first_is_container<_First, _Rest...>
        : std::integral_constant<bool,
              (sizeof...(_Rest) == 0) &&
              ( is_std_array<typename std::decay<_First>::type>::value ||
                is_std_tuple<typename std::decay<_First>::type>::value )>
    {
    };

    // has_dimension / system_dimension / arity_ok
    //   helpers for the system-aware builder. system_dimension yields the
    // coordinate system's dimension (or 0 if it has none); arity_ok checks a
    // declared parameter count against it, allowing dimensionless systems.
    template<typename _S, typename = void>
    struct has_dimension : std::false_type {};

    template<typename _S>
    struct has_dimension<_S, void_t<decltype(_S::dimension)>> : std::true_type {};

    template<typename _S, bool = has_dimension<_S>::value>
    struct system_dimension : std::integral_constant<std::size_t, 0>
    {
    };

    template<typename _S>
    struct system_dimension<_S, true>
        : std::integral_constant<std::size_t, _S::dimension>
    {
    };

    template<typename _S, std::size_t _N>
    struct arity_ok
        : std::integral_constant<bool,
              (!has_dimension<_S>::value) || (system_dimension<_S>::value == _N)>
    {
    };

    // eval_components
    //   evaluates each element of a component tuple against the same argument
    // pack, returning the values as a std::array<double, N>. Shared by the
    // vector-valued and parametric forms.
    template<typename _Tuple, std::size_t... _Is, typename... _Args>
    D_CONSTEXPR std::array<double, sizeof...(_Is)>
    eval_components
    (
        const _Tuple&             _components,
        std::index_sequence<_Is...>,
        _Args&&...                _args
    )
    {
        return std::array<double, sizeof...(_Is)>{{
            static_cast<double>(std::get<_Is>(_components)(_args...))...
        }};
    }

NS_END  // internal


// ============================================================================
// II.   DETECTION TRAITS  (folded in from function_traits.hpp)
// ============================================================================

// D_DETECT_MARKER
//   defines a trait TRAIT<T> (plus TRAIT##_v) that is true when T carries a
// static constexpr member MEMBER whose value is true. This reproduces the
// marker-member detection contract of the original function_traits.hpp.
#define D_DETECT_MARKER(TRAIT, MEMBER)                                          \
    NS_INTERNAL                                                                 \
    template<typename _T, typename = void>                                      \
    struct TRAIT##_has : std::false_type {};                                    \
    template<typename _T>                                                       \
    struct TRAIT##_has<_T, void_t<decltype(_T::MEMBER)>> : std::true_type {};   \
    template<typename _T, bool = TRAIT##_has<_T>::value>                        \
    struct TRAIT##_impl : std::false_type {};                                   \
    template<typename _T>                                                       \
    struct TRAIT##_impl<_T, true>                                               \
        : std::integral_constant<bool, static_cast<bool>(_T::MEMBER)> {};       \
    NS_END                                                                      \
    template<typename _T>                                                       \
    struct TRAIT : internal::TRAIT##_impl<typename std::decay<_T>::type> {};    \
    template<typename _T>                                                       \
    D_INLINE_VAR constexpr bool TRAIT##_v = TRAIT<_T>::value;

D_DETECT_MARKER(is_inequality,        is_inequality)
D_DETECT_MARKER(is_piecewise,         is_piecewise)
D_DETECT_MARKER(is_parametric,        is_parametric)
D_DETECT_MARKER(is_implicit,          is_implicit)
D_DETECT_MARKER(is_vector_valued,     is_vector_valued)
D_DETECT_MARKER(is_function,          is_math_function)

D_DETECT_MARKER(is_cartesian_system,  is_cartesian)
D_DETECT_MARKER(is_polar_system,      is_polar)
D_DETECT_MARKER(is_cylindrical_system, is_cylindrical)
D_DETECT_MARKER(is_spherical_system,  is_spherical)

// is_coordinate_system
//   trait: a structural coordinate system (delegates to coordinate.hpp).
template<typename _Type>
struct is_coordinate_system : is_coord_system<_Type>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename _Type>
D_INLINE_VAR constexpr bool is_coordinate_system_v =
    is_coordinate_system<_Type>::value;
#endif

// function_arity / function_value_type
//   convenience accessors for any expression, form, or named function.
template<typename _Type>
struct function_arity
    : std::integral_constant<std::size_t,
                             std::decay<_Type>::type::arity>
{
};

template<typename _Type>
using function_value_type = typename std::decay<_Type>::type::value_type;


// ============================================================================
// III.  RELATIONAL LAYER
// ============================================================================

NS_INTERNAL

    // relational operation functors. Each returns bool.
    struct rel_lt { template<typename _A, typename _B>
        static D_CONSTEXPR bool apply(_A _a, _B _b) noexcept { return _a <  _b; } };
    struct rel_le { template<typename _A, typename _B>
        static D_CONSTEXPR bool apply(_A _a, _B _b) noexcept { return _a <= _b; } };
    struct rel_gt { template<typename _A, typename _B>
        static D_CONSTEXPR bool apply(_A _a, _B _b) noexcept { return _a >  _b; } };
    struct rel_ge { template<typename _A, typename _B>
        static D_CONSTEXPR bool apply(_A _a, _B _b) noexcept { return _a >= _b; } };
    struct rel_eq { template<typename _A, typename _B>
        static D_CONSTEXPR bool apply(_A _a, _B _b) noexcept { return _a == _b; } };
    struct rel_ne { template<typename _A, typename _B>
        static D_CONSTEXPR bool apply(_A _a, _B _b) noexcept { return _a != _b; } };

NS_END  // internal

// relation_node
//   expression: a boolean comparison of two sub-expressions. Carries the
// is_inequality marker so it is recognised as a relational/predicate form.
template<typename _Op, typename _Left, typename _Right>
struct relation_node
    : expression_base<relation_node<_Op, _Left, _Right>>
{
    using value_type = bool;

    _Left  left;
    _Right right;

    static constexpr std::size_t arity =
        internal::max_size_t<_Left::arity, _Right::arity>::value;
    static constexpr bool is_inequality = true;

    D_CONSTEXPR
    relation_node
    (
        _Left  _l,
        _Right _r
    ) noexcept
        : left(_l), right(_r)
    {
    }

    template<typename... _Args>
    D_CONSTEXPR bool
    operator()
    (
        _Args&&... _args
    ) const noexcept
    {
        return _Op::apply(left(_args...), right(_args...));
    }
};

// Relational operators, defined once over the expression role: expr (.) expr,
// expr (.) scalar, and scalar (.) expr. Scalars are lifted to constants.
#define D_EXPR_RELATION_OP(SYM, TAG)                                            \
    template<typename _L, typename _R,                                          \
             typename std::enable_if<( is_expression<_L>::value &&              \
                                       is_expression<_R>::value ), int>::type = 0> \
    D_CONSTEXPR relation_node<internal::TAG, _L, _R>                            \
    operator SYM (_L _l, _R _r) noexcept                                        \
    { return relation_node<internal::TAG, _L, _R>(_l, _r); }                    \
                                                                                \
    template<typename _L, typename _S,                                          \
             typename std::enable_if<( is_expression<_L>::value &&              \
                                       !is_expression<_S>::value ), int>::type = 0> \
    D_CONSTEXPR auto operator SYM (_L _l, _S _s) noexcept                       \
        -> relation_node<internal::TAG, _L, decltype(internal::as_expr(_s))>    \
    { auto r = internal::as_expr(_s);                                           \
      return relation_node<internal::TAG, _L, decltype(r)>(_l, r); }            \
                                                                                \
    template<typename _S, typename _R,                                          \
             typename std::enable_if<( !is_expression<_S>::value &&             \
                                       is_expression<_R>::value ), int>::type = 0> \
    D_CONSTEXPR auto operator SYM (_S _s, _R _r) noexcept                       \
        -> relation_node<internal::TAG, decltype(internal::as_expr(_s)), _R>    \
    { auto l = internal::as_expr(_s);                                           \
      return relation_node<internal::TAG, decltype(l), _R>(l, _r); }

D_EXPR_RELATION_OP(<,  rel_lt)
D_EXPR_RELATION_OP(<=, rel_le)
D_EXPR_RELATION_OP(>,  rel_gt)
D_EXPR_RELATION_OP(>=, rel_ge)
D_EXPR_RELATION_OP(==, rel_eq)
D_EXPR_RELATION_OP(!=, rel_ne)


// ============================================================================
// IV.   LOGICAL LAYER
// ============================================================================

// logical_and_node / logical_or_node / logical_not_node
//   boolean-valued expressions combining predicates. Each carries the
// is_inequality marker (a predicate is a relational form).
template<typename _Left, typename _Right>
struct logical_and_node
    : expression_base<logical_and_node<_Left, _Right>>
{
    using value_type = bool;

    _Left  left;
    _Right right;

    static constexpr std::size_t arity =
        internal::max_size_t<_Left::arity, _Right::arity>::value;
    static constexpr bool is_inequality = true;

    D_CONSTEXPR logical_and_node(_Left _l, _Right _r) noexcept
        : left(_l), right(_r) {}

    template<typename... _Args>
    D_CONSTEXPR bool operator()(_Args&&... _args) const noexcept
    { return static_cast<bool>(left(_args...)) &&
             static_cast<bool>(right(_args...)); }
};

template<typename _Left, typename _Right>
struct logical_or_node
    : expression_base<logical_or_node<_Left, _Right>>
{
    using value_type = bool;

    _Left  left;
    _Right right;

    static constexpr std::size_t arity =
        internal::max_size_t<_Left::arity, _Right::arity>::value;
    static constexpr bool is_inequality = true;

    D_CONSTEXPR logical_or_node(_Left _l, _Right _r) noexcept
        : left(_l), right(_r) {}

    template<typename... _Args>
    D_CONSTEXPR bool operator()(_Args&&... _args) const noexcept
    { return static_cast<bool>(left(_args...)) ||
             static_cast<bool>(right(_args...)); }
};

template<typename _Arg>
struct logical_not_node
    : expression_base<logical_not_node<_Arg>>
{
    using value_type = bool;

    _Arg arg;

    static constexpr std::size_t arity = _Arg::arity;
    static constexpr bool is_inequality = true;

    D_CONSTEXPR explicit logical_not_node(_Arg _a) noexcept : arg(_a) {}

    template<typename... _Args>
    D_CONSTEXPR bool operator()(_Args&&... _args) const noexcept
    { return !static_cast<bool>(arg(_args...)); }
};

template<typename _L, typename _R,
         typename std::enable_if<( is_expression<_L>::value &&
                                   is_expression<_R>::value ), int>::type = 0>
D_CONSTEXPR logical_and_node<_L, _R> operator&&(_L _l, _R _r) noexcept
{ return logical_and_node<_L, _R>(_l, _r); }

template<typename _L, typename _R,
         typename std::enable_if<( is_expression<_L>::value &&
                                   is_expression<_R>::value ), int>::type = 0>
D_CONSTEXPR logical_or_node<_L, _R> operator||(_L _l, _R _r) noexcept
{ return logical_or_node<_L, _R>(_l, _r); }

template<typename _A,
         typename std::enable_if<is_expression<_A>::value, int>::type = 0>
D_CONSTEXPR logical_not_node<_A> operator!(_A _a) noexcept
{ return logical_not_node<_A>(_a); }

// Named combinators. land/lor/lnot mirror the operators; all_of/any_of fold a
// pack of predicates into a nested tree.
template<typename _L, typename _R>
D_CONSTEXPR logical_and_node<_L, _R> land(_L _l, _R _r) noexcept
{ return logical_and_node<_L, _R>(_l, _r); }

template<typename _L, typename _R>
D_CONSTEXPR logical_or_node<_L, _R> lor(_L _l, _R _r) noexcept
{ return logical_or_node<_L, _R>(_l, _r); }

template<typename _A>
D_CONSTEXPR logical_not_node<_A> lnot(_A _a) noexcept
{ return logical_not_node<_A>(_a); }

template<typename _C>
D_CONSTEXPR _C all_of(_C _c) noexcept { return _c; }

template<typename _C0, typename _C1, typename... _Rest>
D_CONSTEXPR auto all_of(_C0 _c0, _C1 _c1, _Rest... _rest) noexcept
    -> decltype(land(_c0, all_of(_c1, _rest...)))
{ return land(_c0, all_of(_c1, _rest...)); }

template<typename _C>
D_CONSTEXPR _C any_of(_C _c) noexcept { return _c; }

template<typename _C0, typename _C1, typename... _Rest>
D_CONSTEXPR auto any_of(_C0 _c0, _C1 _c1, _Rest... _rest) noexcept
    -> decltype(lor(_c0, any_of(_c1, _rest...)))
{ return lor(_c0, any_of(_c1, _rest...)); }


// ============================================================================
// V.    PIECEWISE FUNCTIONS
// ============================================================================

// always_true_node
//   predicate that is always true; the condition of an otherwise() branch.
struct always_true_node : expression_base<always_true_node>
{
    using value_type = bool;
    static constexpr std::size_t arity = 0;
    static constexpr bool is_inequality = true;

    template<typename... _Args>
    D_CONSTEXPR bool operator()(_Args&&...) const noexcept { return true; }
};

// piece_t
//   one branch of a piecewise function: a predicate paired with a body
// expression. Not itself an expression node.
template<typename _Cond, typename _Body>
struct piece_t
{
    using condition_type  = _Cond;
    using expression_type = _Body;
    using value_type      = typename _Body::value_type;

    _Cond cond;
    _Body body;

    static constexpr std::size_t arity =
        internal::max_size_t<_Cond::arity, _Body::arity>::value;

    D_CONSTEXPR piece_t(_Cond _c, _Body _b) noexcept : cond(_c), body(_b) {}

    template<typename... _Args>
    D_CONSTEXPR bool test(_Args&&... _args) const noexcept
    { return static_cast<bool>(cond(_args...)); }
};

// when
//   builds a branch: when(condition, body). The body may be a bare scalar.
template<typename _Cond, typename _Body>
D_CONSTEXPR auto when(_Cond _c, _Body _b) noexcept
    -> piece_t<_Cond, decltype(internal::as_expr(_b))>
{
    auto b = internal::as_expr(_b);
    return piece_t<_Cond, decltype(b)>(_c, b);
}

// otherwise
//   builds the fallback branch (an always-true condition).
template<typename _Body>
D_CONSTEXPR auto otherwise(_Body _b) noexcept
    -> piece_t<always_true_node, decltype(internal::as_expr(_b))>
{
    auto b = internal::as_expr(_b);
    return piece_t<always_true_node, decltype(b)>(always_true_node{}, b);
}

NS_INTERNAL

    // pw_run
    //   evaluates the first branch whose predicate holds; the final branch is
    // the fallback (its predicate is not tested). Two overloads, selected by
    // whether the index is the last, keep this C++14-clean (no if constexpr).
    template<std::size_t _I, typename _Value, typename _Tuple, typename... _Args,
             typename std::enable_if<
                 (_I + 1 < std::tuple_size<typename std::decay<_Tuple>::type>::value),
                 int>::type = 0>
    D_CONSTEXPR _Value pw_run(const _Tuple& _t, _Args&&... _a)
    {
        if (std::get<_I>(_t).test(_a...))
        {
            return static_cast<_Value>(std::get<_I>(_t).body(_a...));
        }
        return pw_run<_I + 1, _Value>(_t, _a...);
    }

    template<std::size_t _I, typename _Value, typename _Tuple, typename... _Args,
             typename std::enable_if<
                 (_I + 1 == std::tuple_size<typename std::decay<_Tuple>::type>::value),
                 int>::type = 0>
    D_CONSTEXPR _Value pw_run(const _Tuple& _t, _Args&&... _a)
    {
        return static_cast<_Value>(std::get<_I>(_t).body(_a...));
    }

NS_END  // internal

// piecewise_node
//   expression: a piecewise-defined function over a sequence of branches.
template<typename... _Pieces>
struct piecewise_node
    : expression_base<piecewise_node<_Pieces...>>
{
    using value_type  = typename std::common_type<typename _Pieces::value_type...>::type;
    using pieces_type = std::tuple<_Pieces...>;

    std::tuple<_Pieces...> pieces;

    static constexpr std::size_t num_pieces = sizeof...(_Pieces);
    static constexpr std::size_t arity =
        internal::max_size_t<_Pieces::arity...>::value;
    static constexpr bool is_piecewise = true;

    D_CONSTEXPR explicit piecewise_node(_Pieces... _p) noexcept : pieces(_p...) {}

    template<typename... _Args>
    D_CONSTEXPR value_type operator()(_Args&&... _args) const noexcept
    { return internal::pw_run<0, value_type>(pieces, _args...); }
};

template<typename... _Pieces>
D_CONSTEXPR piecewise_node<_Pieces...> piecewise(_Pieces... _p) noexcept
{ return piecewise_node<_Pieces...>(_p...); }


// ============================================================================
// VI.   VECTOR-VALUED FUNCTIONS
// ============================================================================

// vector_node
//   expression: f : R^n -> R^m given by m component expressions sharing the
// same inputs. operator() returns the m values as a std::array.
template<typename... _Components>
struct vector_node
    : expression_base<vector_node<_Components...>>
{
    using value_type      = std::array<double, sizeof...(_Components)>;
    using components_type = std::tuple<_Components...>;

    std::tuple<_Components...> components;

    static constexpr std::size_t output_dimension = sizeof...(_Components);
    static constexpr std::size_t arity =
        internal::max_size_t<_Components::arity...>::value;
    static constexpr bool is_vector_valued = true;

    D_CONSTEXPR explicit vector_node(_Components... _c) noexcept
        : components(_c...) {}

    template<std::size_t _I>
    using component = typename std::tuple_element<_I, components_type>::type;

    template<typename... _Args>
    D_CONSTEXPR value_type operator()(_Args&&... _args) const noexcept
    {
        return internal::eval_components(
            components,
            std::make_index_sequence<sizeof...(_Components)>{},
            _args...);
    }
};

// vec
//   builds a vector-valued expression; bare scalar components are lifted.
template<typename... _Components>
D_CONSTEXPR auto vec(_Components... _c) noexcept
    -> vector_node<decltype(internal::as_expr(_c))...>
{
    return vector_node<decltype(internal::as_expr(_c))...>(
        internal::as_expr(_c)...);
}

// vector_function
//   alias of vec, matching the original public name.
template<typename... _Components>
D_CONSTEXPR auto vector_function(_Components... _c) noexcept
    -> decltype(vec(_c...))
{ return vec(_c...); }


// ============================================================================
// VII.  PARAMETRIC CURVES AND SURFACES
// ============================================================================

// parametric_node
//   expression: a parametric mapping with a fixed parameter count. Carries
// both is_parametric (with parameter_count) and is_vector_valued markers.
template<std::size_t _ParamCount, typename... _Components>
struct parametric_node
    : expression_base<parametric_node<_ParamCount, _Components...>>
{
    using value_type      = std::array<double, sizeof...(_Components)>;
    using components_type = std::tuple<_Components...>;

    std::tuple<_Components...> components;

    static constexpr std::size_t output_dimension = sizeof...(_Components);
    static constexpr std::size_t parameter_count  = _ParamCount;
    static constexpr std::size_t arity            = _ParamCount;
    static constexpr bool is_parametric    = true;
    static constexpr bool is_vector_valued = true;

    D_CONSTEXPR explicit parametric_node(_Components... _c) noexcept
        : components(_c...) {}

    template<typename... _Args>
    D_CONSTEXPR value_type operator()(_Args&&... _args) const noexcept
    {
        return internal::eval_components(
            components,
            std::make_index_sequence<sizeof...(_Components)>{},
            _args...);
    }
};

// parametric_curve  (one parameter t)
template<typename... _Components>
D_CONSTEXPR auto parametric_curve(_Components... _c) noexcept
    -> parametric_node<1, decltype(internal::as_expr(_c))...>
{
    return parametric_node<1, decltype(internal::as_expr(_c))...>(
        internal::as_expr(_c)...);
}

// parametric_surface  (two parameters u, v)
template<typename... _Components>
D_CONSTEXPR auto parametric_surface(_Components... _c) noexcept
    -> parametric_node<2, decltype(internal::as_expr(_c))...>
{
    return parametric_node<2, decltype(internal::as_expr(_c))...>(
        internal::as_expr(_c)...);
}


// ============================================================================
// VIII. IMPLICIT FUNCTIONS
// ============================================================================

// implicit_node
//   expression: a function defined implicitly by F(x, y, ...) = 0. operator()
// returns the residual F; contains() and sign() locate a point relative to
// the zero set.
template<typename _Expr>
struct implicit_node
    : expression_base<implicit_node<_Expr>>
{
    using expression_type = _Expr;
    using value_type      = typename _Expr::value_type;

    _Expr expr;

    static constexpr std::size_t arity = _Expr::arity;
    static constexpr bool is_implicit = true;

    D_CONSTEXPR explicit implicit_node(_Expr _e) noexcept : expr(_e) {}

    template<typename... _Args>
    D_CONSTEXPR value_type operator()(_Args&&... _args) const noexcept
    { return expr(_args...); }

    // contains
    //   true when |F(args...)| < epsilon. (epsilon is first so it may precede
    // the variadic coordinate pack.)
    template<typename... _Args>
    D_CONSTEXPR bool contains(value_type _epsilon, _Args&&... _args) const noexcept
    {
        value_type v = expr(_args...);
        return (v > -_epsilon) && (v < _epsilon);
    }

    // sign
    //   -1, 0, or +1: which side of the zero set the point lies on.
    template<typename... _Args>
    D_CONSTEXPR int sign(_Args&&... _args) const noexcept
    {
        value_type v = expr(_args...);
        return (v < static_cast<value_type>(0)) ? -1
             : (v > static_cast<value_type>(0)) ?  1
             :                                      0;
    }
};

template<typename _Expr>
D_CONSTEXPR implicit_node<_Expr> implicit(_Expr _e) noexcept
{ return implicit_node<_Expr>(_e); }

// level_set
//   the set { F(x,...) = level }, represented as implicit(F - level).
template<typename _Expr, typename _Level>
D_CONSTEXPR auto level_set(_Expr _e, _Level _level) noexcept
    -> implicit_node<decltype(_e - internal::as_expr(_level))>
{
    auto residual = _e - internal::as_expr(_level);
    return implicit_node<decltype(residual)>(residual);
}


// ============================================================================
// IX.   MIGRATION ALIASES
// ============================================================================

// multivariable / mvar_*
//   the original positional-variable names, mapped onto variable_node.
template<typename _ValueType, std::size_t _ID>
using multivariable = variable_node<_ID, _ValueType>;

template<typename _T = double> using mvar_x = variable_node<0, _T>;
template<typename _T = double> using mvar_y = variable_node<1, _T>;
template<typename _T = double> using mvar_z = variable_node<2, _T>;
template<typename _T = double> using mvar_w = variable_node<3, _T>;
template<typename _T = double> using mvar_u = variable_node<0, _T>;
template<typename _T = double> using mvar_v = variable_node<1, _T>;


// ============================================================================
// X.    COORDINATE AXIS VARIABLE SETS  (correctly indexed per system)
// ============================================================================

// Per-system axis placeholders with the indices each system actually uses.
// Bringing the right set into scope keeps axis names unambiguous -- in
// particular phi is argument 1 in cylindrical but argument 2 in spherical.
namespace cartesian_axes
{
    D_INLINE_VAR constexpr variable_node<0> x{};
    D_INLINE_VAR constexpr variable_node<1> y{};
    D_INLINE_VAR constexpr variable_node<2> z{};
    D_INLINE_VAR constexpr variable_node<3> w{};
}

namespace polar_axes
{
    D_INLINE_VAR constexpr variable_node<0> r{};
    D_INLINE_VAR constexpr variable_node<1> theta{};
}

namespace cylindrical_axes
{
    D_INLINE_VAR constexpr variable_node<0> rho{};
    D_INLINE_VAR constexpr variable_node<1> phi{};
    D_INLINE_VAR constexpr variable_node<2> z{};
}

namespace spherical_axes
{
    D_INLINE_VAR constexpr variable_node<0> r{};
    D_INLINE_VAR constexpr variable_node<1> theta{};
    D_INLINE_VAR constexpr variable_node<2> phi{};
}


// ============================================================================
// XI.   NAMED FUNCTION WRAPPER
// ============================================================================

// math_function
//   the callable top-level wrapper: an expression bound to a coordinate
// system and a declared arity. Evaluable from a scalar pack, a std::array, or
// a std::tuple (the latter two explode into the pack), so a coordinate point
// -- which is just the system's point_type, a std::array -- evaluates directly.
template<typename _System, typename _Expr, std::size_t _Arity>
struct math_function
{
    using system_type       = _System;
    using coordinate_system = _System;
    using expression_type   = _Expr;
    using value_type        = typename _Expr::value_type;

    _System system;
    _Expr   expr;

    static constexpr std::size_t arity = _Arity;
    static constexpr bool is_math_function = true;

    D_CONSTEXPR math_function(_System _s, _Expr _e) noexcept
        : system(_s), expr(_e) {}

    // scalar pack -- excluded for a single array/tuple argument so the
    // dedicated overloads below win unambiguously.
    template<typename... _Args,
             typename std::enable_if<
                 !internal::first_is_container<_Args...>::value, int>::type = 0>
    D_CONSTEXPR value_type operator()(_Args&&... _args) const noexcept
    { return expr(_args...); }

    // std::array of coordinates
    template<typename _T, std::size_t _N>
    D_CONSTEXPR value_type operator()(const std::array<_T, _N>& _point) const noexcept
    { return eval_array(_point, std::make_index_sequence<_N>{}); }

    // std::tuple of coordinates
    template<typename... _TArgs>
    D_CONSTEXPR value_type operator()(const std::tuple<_TArgs...>& _point) const noexcept
    { return eval_tuple(_point, std::make_index_sequence<sizeof...(_TArgs)>{}); }

private:
    template<typename _T, std::size_t _N, std::size_t... _Is>
    D_CONSTEXPR value_type
    eval_array(const std::array<_T, _N>& _p, std::index_sequence<_Is...>) const noexcept
    { return expr(_p[_Is]...); }

    template<typename _Tuple, std::size_t... _Is>
    D_CONSTEXPR value_type
    eval_tuple(const _Tuple& _p, std::index_sequence<_Is...>) const noexcept
    { return expr(std::get<_Is>(_p)...); }
};


// ============================================================================
// XII.  FLUENT BUILDER
// ============================================================================

// function_of
//   the builder stage after .of(...): the arity is fixed, awaiting .returns.
template<typename _System, std::size_t _Arity>
struct function_of
{
    _System system;

    D_CONSTEXPR explicit function_of(_System _s) noexcept : system(_s) {}

    template<typename _Expr>
    D_CONSTEXPR math_function<_System, _Expr, _Arity>
    returns(_Expr _e) const noexcept
    { return math_function<_System, _Expr, _Arity>(system, _e); }
};

// function_builder
//   the initial builder stage produced by function(system).
template<typename _System>
struct function_builder
{
    _System system;

    D_CONSTEXPR explicit function_builder(_System _s) noexcept : system(_s) {}

    // .of(vars...) -- declares the parameter list. The count must match the
    // system dimension when the system has one.
    template<typename... _Vars>
    D_CONSTEXPR function_of<_System, sizeof...(_Vars)>
    of(_Vars...) const noexcept
    {
        static_assert(internal::arity_ok<_System, sizeof...(_Vars)>::value,
                      "function(system).of(...): parameter count must match "
                      "the coordinate system dimension.");
        return function_of<_System, sizeof...(_Vars)>(system);
    }

    // .returns(expr) directly -- arity is taken from the system dimension.
    template<typename _Expr>
    D_CONSTEXPR math_function<_System, _Expr, internal::system_dimension<_System>::value>
    returns(_Expr _e) const noexcept
    {
        return math_function<_System, _Expr,
                             internal::system_dimension<_System>::value>(system, _e);
    }
};

// function
//   entry point: function(system) -> builder.
template<typename _System>
D_CONSTEXPR function_builder<_System> function(_System _s) noexcept
{ return function_builder<_System>(_s); }


#undef D_DETECT_MARKER
#undef D_EXPR_RELATION_OP

NS_END  // math
NS_END  // djinterp


#endif  // DJINTERP_MATH_FUNCTION_
