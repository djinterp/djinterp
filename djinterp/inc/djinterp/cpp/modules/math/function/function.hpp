/*******************************************************************************
* djinterp [cpp][math]                                              function.hpp
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
*       std::array, std::tuple and linalg::vector evaluation, the last a
*       coordinate system's point; and
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
* LEVELS (step 30, 5b)
*   C++11 is the floor, as for expression.hpp, and the header is empty below
* it. From C++11 every form, its operators and the builder are as above, and
* each is a constant expression wherever its operands' evaluation is: each
* constexpr function is one return, the index sequences re_std's. The _v
* variable templates are from C++14, where variable templates are.
*
* HOW TO USE
* ----------
*   Include it, or math.hpp, from C++11. Relations, logic, piecewise,
* vector-valued, parametric and implicit functions build on expression.hpp's
* nodes; math_function binds an expression to a coordinate system, and
* function_of(...).returns(...) builds one fluently.
*
* OPTIONS
* -------
*   None: nothing in this header is chosen at run time.
*
* CONFIGURATION
* -------------
*   None of its own: it compiles from C++11 and is empty below it (N1). The
* forms over std::tuple are from C++14.
*
*
* path:      /inc/djinterp/cpp/modules/math/function/function.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.06
*                                                            revised: 2026.11.24
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  INTERNAL SUPPORT
    ----------------
2.  DETECTION TRAITS
    ----------------
3.  RELATIONAL LAYER
    ----------------
4.  LOGICAL LAYER
    -------------
5.  PIECEWISE FUNCTIONS
    -------------------
6.  VECTOR-VALUED FUNCTIONS
    -----------------------
7.  PARAMETRIC CURVES AND SURFACES
    ------------------------------
8.  IMPLICIT FUNCTIONS
    ------------------
9.  MIGRATION ALIASES
    -----------------
10. COORDINATE AXIS VARIABLE SETS
    -----------------------------
11. NAMED FUNCTION WRAPPER
    ----------------------
12. FLUENT BUILDER
    --------------
*/

#ifndef DJINTERP_MATH_FUNCTION_FUNCTION_HPP
#define DJINTERP_MATH_FUNCTION_FUNCTION_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <array>        // std::array
#include <cstddef>      // std::size_t
#include <tuple>        // std::tuple, std::get, std::tuple_size
#include <type_traits>  // std::enable_if, std::decay, std::integral_constant
// djinterp
#include "../expression.hpp"             // expressions
#include "../coordinate/coordinate.hpp"  // is_coord_system
#include "../linear_algebra/vector.hpp"  // linalg::vector, a system's point
// re_std
#include "../../../../../re_std/cpp/utility/integer_sequence.hpp"       // index_
                                                                        // sequence
#include "../../../../../re_std/cpp/utility/make_integer_sequence.hpp"  // make_index_
                                                                        // sequence


NS_DJINTERP
NS_MATH

//==============================================================================
// 1.  INTERNAL SUPPORT
//==============================================================================

NS_INTERNAL

    // make_void / void_t
    //   alias: maps any type list to void (detection idiom). std::void_t is
    // C++17; this keeps the header usable from C++11. The alias names a
    // member of a class template, so that an ill-formed argument is a
    // substitution failure under every compiler (CWG 1558).
    template<typename...>
    struct make_void
    {
        typedef void type;
    };

    template<typename... Types>
    using void_t = typename make_void<Types...>::type;

    // max_size_t
    //   meta: compile-time maximum of a pack of std::size_t (0 for empty).
    template<std::size_t...>
    struct max_size_t;

    template<>
    struct max_size_t<>
    {
        static constexpr std::size_t value = 0;
    };

    template<std::size_t N>
    struct max_size_t<N>
    {
        static constexpr std::size_t value = N;
    };

    template<std::size_t N, std::size_t... Rest>
    struct max_size_t<N, Rest...>
    {
        static constexpr std::size_t value =
            (N > max_size_t<Rest...>::value)
                ? N
                : max_size_t<Rest...>::value;
    };

    // is_std_array / is_std_tuple / is_linalg_vector / first_is_container
    //   traits: recognise std::array, std::tuple and linalg::vector, and
    // detect the "single container argument" case used to disambiguate
    // math_function's scalar-pack operator() from its container overloads.
    template<typename>
    struct is_std_array : std::false_type {};

    template<typename T, std::size_t N>
    struct is_std_array<std::array<T, N>> : std::true_type {};

    template<typename>
    struct is_std_tuple : std::false_type {};

    template<typename... T>
    struct is_std_tuple<std::tuple<T...>> : std::true_type {};

    template<typename>
    struct is_linalg_vector : std::false_type {};

    template<typename T, std::size_t N>
    struct is_linalg_vector<linalg::vector<T, N>> : std::true_type {};

    template<typename... Args>
    struct first_is_container : std::false_type {};

    template<typename First, typename... Rest>
    struct first_is_container<First, Rest...>
        : std::integral_constant<bool,
              (sizeof...(Rest) == 0) &&
              ( is_std_array<typename std::decay<First>::type>::value ||
                is_std_tuple<typename std::decay<First>::type>::value ||
                is_linalg_vector<typename std::decay<First>::type>::value )>
    {
    };

    // has_dimension / system_dimension / arity_ok
    //   helpers for the system-aware builder. system_dimension yields the
    // coordinate system's dimension (or 0 if it has none); arity_ok checks a
    // declared parameter count against it, allowing dimensionless systems.
    template<typename S, typename = void>
    struct has_dimension : std::false_type {};

    template<typename S>
    struct has_dimension<S, void_t<decltype(S::dimension)>> : std::true_type {};

    template<typename S, bool = has_dimension<S>::value>
    struct system_dimension : std::integral_constant<std::size_t, 0>
    {
    };

    template<typename S>
    struct system_dimension<S, true>
        : std::integral_constant<std::size_t, S::dimension>
    {
    };

    template<typename S, std::size_t N>
    struct arity_ok
        : std::integral_constant<bool,
              (!has_dimension<S>::value) || (system_dimension<S>::value == N)>
    {
    };

    // eval_components
    //   evaluates each element of a component tuple against the same argument
    // pack, returning the values as a std::array<double, N>. Shared by the
    // vector-valued and parametric forms.
    template<typename Tuple, std::size_t... Is, typename... Args>
    D_CONSTEXPR std::array<double, sizeof...(Is)>
    eval_components
    (
        const Tuple&                  _components,
        re_std::index_sequence<Is...>,
        Args&&...                     _args
    )
    {
        return std::array<double, sizeof...(Is)>{{
            static_cast<double>(std::get<Is>(_components)(_args...))...
        }};
    }

NS_END  // internal


//==============================================================================
// 2.  DETECTION TRAITS
//==============================================================================
// folded in from function_traits.hpp.

// D_DETECT_MARKER_V
//   defines TRAIT##_v, the variable template of a trait, where variable
// templates exist (C++14); nothing below.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    #define D_DETECT_MARKER_V(TRAIT)                                            \
        template<typename T>                                                    \
        D_INLINE_VAR constexpr bool TRAIT##_v = TRAIT<T>::value;
#else
    #define D_DETECT_MARKER_V(TRAIT)
#endif

// D_DETECT_MARKER
//   defines a trait TRAIT<T> (plus TRAIT##_v from C++14) that is true when T
// carries a static constexpr member MEMBER whose value is true. This
// reproduces the marker-member detection contract of the original
// function_traits.hpp.
#define D_DETECT_MARKER(TRAIT, MEMBER)                                          \
    NS_INTERNAL                                                                 \
    template<typename T, typename = void>                                       \
    struct TRAIT##_has : std::false_type {};                                    \
    template<typename T>                                                        \
    struct TRAIT##_has<T, void_t<decltype(T::MEMBER)>> : std::true_type {};     \
    template<typename T, bool = TRAIT##_has<T>::value>                          \
    struct TRAIT##_impl : std::false_type {};                                   \
    template<typename T>                                                        \
    struct TRAIT##_impl<T, true>                                                \
        : std::integral_constant<bool, static_cast<bool>(T::MEMBER)> {};        \
    NS_END                                                                      \
    template<typename T>                                                        \
    struct TRAIT : internal::TRAIT##_impl<typename std::decay<T>::type> {};     \
    D_DETECT_MARKER_V(TRAIT)

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
template<typename Type>
struct is_coordinate_system : is_coord_system<Type>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type>
D_INLINE_VAR constexpr bool is_coordinate_system_v =
    is_coordinate_system<Type>::value;
#endif

// function_arity / function_value_type
//   convenience accessors for any expression, form, or named function.
template<typename Type>
struct function_arity
    : std::integral_constant<std::size_t,
                             std::decay<Type>::type::arity>
{
};

template<typename Type>
using function_value_type = typename std::decay<Type>::type::value_type;


//==============================================================================
// 3.  RELATIONAL LAYER
//==============================================================================

NS_INTERNAL

    // relational operation functors. Each returns bool.
    struct rel_lt { template<typename A, typename B>
        static D_CONSTEXPR bool apply(A _a, B _b) noexcept { return _a <  _b; } };
    struct rel_le { template<typename A, typename B>
        static D_CONSTEXPR bool apply(A _a, B _b) noexcept { return _a <= _b; } };
    struct rel_gt { template<typename A, typename B>
        static D_CONSTEXPR bool apply(A _a, B _b) noexcept { return _a >  _b; } };
    struct rel_ge { template<typename A, typename B>
        static D_CONSTEXPR bool apply(A _a, B _b) noexcept { return _a >= _b; } };
    struct rel_eq { template<typename A, typename B>
        static D_CONSTEXPR bool apply(A _a, B _b) noexcept { return _a == _b; } };
    struct rel_ne { template<typename A, typename B>
        static D_CONSTEXPR bool apply(A _a, B _b) noexcept { return _a != _b; } };

NS_END  // internal

// relation_node
//   expression: a boolean comparison of two sub-expressions. Carries the
// is_inequality marker so it is recognised as a relational/predicate form.
template<typename Op, typename Left, typename Right>
struct relation_node
    : expression_base<relation_node<Op, Left, Right>>
{
    using value_type = bool;

    Left  left;
    Right right;

    static constexpr std::size_t arity =
        internal::max_size_t<Left::arity, Right::arity>::value;
    static constexpr bool is_inequality = true;

    D_CONSTEXPR
    relation_node
    (
        Left  _l,
        Right _r
    ) noexcept
        : left(_l), right(_r)
    {
    }

    template<typename... Args>
    D_CONSTEXPR
    bool operator()(
        Args&&... _args
    ) const noexcept
    {
        return Op::apply(left(_args...), right(_args...));
    }
};

// Relational operators, defined once over the expression role: expr (.) expr,
// expr (.) scalar, and scalar (.) expr. Scalars are lifted to constants.
#define D_EXPR_RELATION_OP(SYM, TAG)                                            \
    template<typename L, typename R,                                            \
             typename std::enable_if<( is_expression<L>::value &&               \
                                       is_expression<R>::value ), int>::type =  0> \
    D_CONSTEXPR relation_node<internal::TAG, L, R>                              \
    operator SYM (L _l, R _r) noexcept                                          \
    { return relation_node<internal::TAG, L, R>(_l, _r); }                      \
                                                                                \
    template<typename L, typename S,                                           \
             typename std::enable_if<( is_expression<L>::value &&              \
                                       !is_expression<S>::value ), int>::type = 0> \
    D_CONSTEXPR auto operator SYM (L _l, S _s) noexcept                         \
        -> relation_node<internal::TAG, L, decltype(internal::as_expr(_s))>     \
    { return relation_node<internal::TAG, L,                                    \
                           decltype(internal::as_expr(_s))>(                    \
                 _l, internal::as_expr(_s)); }                                  \
                                                                                \
    template<typename S, typename R,                                            \
             typename std::enable_if<( !is_expression<S>::value &&              \
                                       is_expression<R>::value ), int>::type =  0> \
    D_CONSTEXPR auto operator SYM (S _s, R _r) noexcept                         \
        -> relation_node<internal::TAG, decltype(internal::as_expr(_s)), R>     \
    { return relation_node<internal::TAG,                                       \
                           decltype(internal::as_expr(_s)), R>(                 \
                 internal::as_expr(_s), _r); }

D_EXPR_RELATION_OP(<,  rel_lt)
D_EXPR_RELATION_OP(<=, rel_le)
D_EXPR_RELATION_OP(>,  rel_gt)
D_EXPR_RELATION_OP(>=, rel_ge)
D_EXPR_RELATION_OP(==, rel_eq)
D_EXPR_RELATION_OP(!=, rel_ne)


//==============================================================================
// 4.  LOGICAL LAYER
//==============================================================================

// logical_and_node / logical_or_node / logical_not_node
//   boolean-valued expressions combining predicates. Each carries the
// is_inequality marker (a predicate is a relational form).
template<typename Left, typename Right>
struct logical_and_node
    : expression_base<logical_and_node<Left, Right>>
{
    using value_type = bool;

    Left  left;
    Right right;

    static constexpr std::size_t arity =
        internal::max_size_t<Left::arity, Right::arity>::value;
    static constexpr bool is_inequality = true;

    D_CONSTEXPR logical_and_node(Left _l, Right _r) noexcept
        : left(_l), right(_r) {}

    template<typename... Args>
    D_CONSTEXPR
    bool operator()(
        Args&&... _args
    ) const noexcept
    { return static_cast<bool>(left(_args...)) &&
             static_cast<bool>(right(_args...)); }
};

template<typename Left, typename Right>
struct logical_or_node
    : expression_base<logical_or_node<Left, Right>>
{
    using value_type = bool;

    Left  left;
    Right right;

    static constexpr std::size_t arity =
        internal::max_size_t<Left::arity, Right::arity>::value;
    static constexpr bool is_inequality = true;

    D_CONSTEXPR logical_or_node(Left _l, Right _r) noexcept
        : left(_l), right(_r) {}

    template<typename... Args>
    D_CONSTEXPR
    bool operator()(
        Args&&... _args
    ) const noexcept
    { return static_cast<bool>(left(_args...)) ||
             static_cast<bool>(right(_args...)); }
};

template<typename Arg>
struct logical_not_node
    : expression_base<logical_not_node<Arg>>
{
    using value_type = bool;

    Arg arg;

    static constexpr std::size_t arity = Arg::arity;
    static constexpr bool is_inequality = true;

    D_CONSTEXPR explicit logical_not_node(Arg _a) noexcept : arg(_a) {}

    template<typename... Args>
    D_CONSTEXPR bool operator()(Args&&... _args) const noexcept
    { return !static_cast<bool>(arg(_args...)); }
};

template<typename L, typename R,
         typename std::enable_if<( is_expression<L>::value &&
                                   is_expression<R>::value ), int>::type = 0>
D_CONSTEXPR logical_and_node<L, R> operator&&(L _l, R _r) noexcept
{ return logical_and_node<L, R>(_l, _r); }

template<typename L, typename R,
         typename std::enable_if<( is_expression<L>::value &&
                                   is_expression<R>::value ), int>::type = 0>
D_CONSTEXPR logical_or_node<L, R> operator||(L _l, R _r) noexcept
{ return logical_or_node<L, R>(_l, _r); }

template<typename A,
         typename std::enable_if<is_expression<A>::value, int>::type = 0>
D_CONSTEXPR logical_not_node<A> operator!(A _a) noexcept
{ return logical_not_node<A>(_a); }

// Named combinators. land/lor/lnot mirror the operators; all_of/any_of fold a
// pack of predicates into a nested tree.
template<typename L, typename R>
D_CONSTEXPR logical_and_node<L, R> land(L _l, R _r) noexcept
{ return logical_and_node<L, R>(_l, _r); }

template<typename L, typename R>
D_CONSTEXPR logical_or_node<L, R> lor(L _l, R _r) noexcept
{ return logical_or_node<L, R>(_l, _r); }

template<typename A>
D_CONSTEXPR logical_not_node<A> lnot(A _a) noexcept
{ return logical_not_node<A>(_a); }

template<typename C>
D_CONSTEXPR C all_of(C _c) noexcept { return _c; }

template<typename C0, typename C1, typename... Rest>
D_CONSTEXPR auto all_of(C0 _c0, C1 _c1, Rest... _rest) noexcept
    -> decltype(land(_c0, all_of(_c1, _rest...)))
{ return land(_c0, all_of(_c1, _rest...)); }

template<typename C>
D_CONSTEXPR C any_of(C _c) noexcept { return _c; }

template<typename C0, typename C1, typename... Rest>
D_CONSTEXPR auto any_of(C0 _c0, C1 _c1, Rest... _rest) noexcept
    -> decltype(lor(_c0, any_of(_c1, _rest...)))
{ return lor(_c0, any_of(_c1, _rest...)); }


//==============================================================================
// 5.  PIECEWISE FUNCTIONS
//==============================================================================

// always_true_node
//   predicate that is always true; the condition of an otherwise() branch.
struct always_true_node : expression_base<always_true_node>
{
    using value_type = bool;
    static constexpr std::size_t arity = 0;
    static constexpr bool is_inequality = true;

    template<typename... Args>
    D_CONSTEXPR bool operator()(Args&&...) const noexcept { return true; }
};

// piece_t
//   one branch of a piecewise function: a predicate paired with a body
// expression. Not itself an expression node.
template<typename Cond, typename Body>
struct piece_t
{
    using condition_type  = Cond;
    using expression_type = Body;
    using value_type      = typename Body::value_type;

    Cond cond;
    Body body;

    static constexpr std::size_t arity =
        internal::max_size_t<Cond::arity, Body::arity>::value;

    D_CONSTEXPR piece_t(Cond _c, Body _b) noexcept : cond(_c), body(_b) {}

    template<typename... Args>
    D_CONSTEXPR bool test(Args&&... _args) const noexcept
    { return static_cast<bool>(cond(_args...)); }
};

// when
//   builds a branch: when(condition, body). The body may be a bare scalar.
template<typename Cond, typename Body>
D_CONSTEXPR
auto when(
    Cond _c,
    Body _b
) noexcept -> piece_t<Cond, decltype(internal::as_expr(_b))>
{
    return piece_t<Cond, decltype(internal::as_expr(_b))>(
        _c, internal::as_expr(_b));
}

// otherwise
//   builds the fallback branch (an always-true condition).
template<typename Body>
D_CONSTEXPR
auto otherwise(
    Body _b
) noexcept -> piece_t<always_true_node, decltype(internal::as_expr(_b))>
{
    return piece_t<always_true_node, decltype(internal::as_expr(_b))>(
        always_true_node{}, internal::as_expr(_b));
}

NS_INTERNAL

    // pw_run
    //   evaluates the first branch whose predicate holds; the final branch is
    // the fallback (its predicate is not tested). Two overloads, selected by
    // whether the index is the last, keep this C++11-clean (no if constexpr),
    // each one return.
    template<std::size_t I, typename Value, typename Tuple, typename... Args,
             typename std::enable_if<
                 (I + 1 < std::tuple_size<typename std::decay<Tuple>::type>::value),
                 int>::type = 0>
    D_CONSTEXPR Value pw_run(const Tuple& _t, Args&&... _a)
    {
        return std::get<I>(_t).test(_a...)
                   ? static_cast<Value>(std::get<I>(_t).body(_a...))
                   : pw_run<I + 1, Value>(_t, _a...);
    }

    template<std::size_t I, typename Value, typename Tuple, typename... Args,
             typename std::enable_if<
                 (I + 1 == std::tuple_size<typename std::decay<Tuple>::type>::value),
                 int>::type = 0>
    D_CONSTEXPR Value pw_run(const Tuple& _t, Args&&... _a)
    {
        return static_cast<Value>(std::get<I>(_t).body(_a...));
    }

NS_END  // internal

// piecewise_node
//   expression: a piecewise-defined function over a sequence of branches.
template<typename... Pieces>
struct piecewise_node
    : expression_base<piecewise_node<Pieces...>>
{
    using value_type  = typename std::common_type<typename Pieces::value_type...>::type;
    using pieces_type = std::tuple<Pieces...>;

    std::tuple<Pieces...> pieces;

    static constexpr std::size_t num_pieces = sizeof...(Pieces);
    static constexpr std::size_t arity =
        internal::max_size_t<Pieces::arity...>::value;
    static constexpr bool is_piecewise = true;

    D_CONSTEXPR explicit piecewise_node(Pieces... _p) noexcept : pieces(_p...) {}

    template<typename... Args>
    D_CONSTEXPR value_type operator()(Args&&... _args) const noexcept
    { return internal::pw_run<0, value_type>(pieces, _args...); }
};

template<typename... Pieces>
D_CONSTEXPR piecewise_node<Pieces...> piecewise(Pieces... _p) noexcept
{ return piecewise_node<Pieces...>(_p...); }


//==============================================================================
// 6.  VECTOR-VALUED FUNCTIONS
//==============================================================================

// vector_node
//   expression: f : R^n -> R^m given by m component expressions sharing the
// same inputs. operator() returns the m values as a std::array.
template<typename... Components>
struct vector_node
    : expression_base<vector_node<Components...>>
{
    using value_type      = std::array<double, sizeof...(Components)>;
    using components_type = std::tuple<Components...>;

    std::tuple<Components...> components;

    static constexpr std::size_t output_dimension = sizeof...(Components);
    static constexpr std::size_t arity =
        internal::max_size_t<Components::arity...>::value;
    static constexpr bool is_vector_valued = true;

    D_CONSTEXPR explicit vector_node(Components... _c) noexcept
        : components(_c...) {}

    template<std::size_t I>
    using component = typename std::tuple_element<I, components_type>::type;

    template<typename... Args>
    D_CONSTEXPR
    value_type operator()(
        Args&&... _args
    ) const noexcept
    {
        return internal::eval_components(
            components,
            re_std::make_index_sequence<sizeof...(Components)>{},
            _args...);
    }
};

// vec
//   builds a vector-valued expression; bare scalar components are lifted.
template<typename... Components>
D_CONSTEXPR
auto vec(
    Components... _c
) noexcept -> vector_node<decltype(internal::as_expr(_c))...>
{
    return vector_node<decltype(internal::as_expr(_c))...>(
        internal::as_expr(_c)...);
}

// vector_function
//   alias of vec, matching the original public name.
template<typename... Components>
D_CONSTEXPR auto vector_function(Components... _c) noexcept
    -> decltype(vec(_c...))
{ return vec(_c...); }


//==============================================================================
// 7.  PARAMETRIC CURVES AND SURFACES
//==============================================================================

// parametric_node
//   expression: a parametric mapping with a fixed parameter count. Carries
// both is_parametric (with parameter_count) and is_vector_valued markers.
template<std::size_t ParamCount, typename... Components>
struct parametric_node
    : expression_base<parametric_node<ParamCount, Components...>>
{
    using value_type      = std::array<double, sizeof...(Components)>;
    using components_type = std::tuple<Components...>;

    std::tuple<Components...> components;

    static constexpr std::size_t output_dimension = sizeof...(Components);
    static constexpr std::size_t parameter_count  = ParamCount;
    static constexpr std::size_t arity            = ParamCount;
    static constexpr bool is_parametric    = true;
    static constexpr bool is_vector_valued = true;

    D_CONSTEXPR explicit parametric_node(Components... _c) noexcept
        : components(_c...) {}

    template<typename... Args>
    D_CONSTEXPR
    value_type operator()(
        Args&&... _args
    ) const noexcept
    {
        return internal::eval_components(
            components,
            re_std::make_index_sequence<sizeof...(Components)>{},
            _args...);
    }
};

// parametric_curve  (one parameter t)
template<typename... Components>
D_CONSTEXPR
auto parametric_curve(
    Components... _c
) noexcept -> parametric_node<1, decltype(internal::as_expr(_c))...>
{
    return parametric_node<1, decltype(internal::as_expr(_c))...>(
        internal::as_expr(_c)...);
}

// parametric_surface  (two parameters u, v)
template<typename... Components>
D_CONSTEXPR
auto parametric_surface(
    Components... _c
) noexcept -> parametric_node<2, decltype(internal::as_expr(_c))...>
{
    return parametric_node<2, decltype(internal::as_expr(_c))...>(
        internal::as_expr(_c)...);
}


//==============================================================================
// 8.  IMPLICIT FUNCTIONS
//==============================================================================

// implicit_node
//   expression: a function defined implicitly by F(x, y, ...) = 0. operator()
// returns the residual F; contains() and sign() locate a point relative to
// the zero set.
template<typename Expr>
struct implicit_node
    : expression_base<implicit_node<Expr>>
{
    using expression_type = Expr;
    using value_type      = typename Expr::value_type;

    Expr expr;

    static constexpr std::size_t arity = Expr::arity;
    static constexpr bool is_implicit = true;

    D_CONSTEXPR explicit implicit_node(Expr _e) noexcept : expr(_e) {}

    template<typename... Args>
    D_CONSTEXPR value_type operator()(Args&&... _args) const noexcept
    { return expr(_args...); }

    // contains
    //   true when |F(args...)| < epsilon. (epsilon is first so it may precede
    // the variadic coordinate pack.)
    template<typename... Args>
    D_CONSTEXPR
    bool contains(
        value_type _epsilon,
        Args&&... _args
    ) const noexcept
    {
        return within(expr(_args...), _epsilon);
    }

    // sign
    //   -1, 0, or +1: which side of the zero set the point lies on.
    template<typename... Args>
    D_CONSTEXPR
    int sign(
        Args&&... _args
    ) const noexcept
    {
        return sign_of(expr(_args...));
    }

private:
    // within
    //   whether a residual lies strictly inside (-epsilon, epsilon); the
    // residual is evaluated once, by the caller.
    static D_CONSTEXPR
    bool within(
        value_type _v,
        value_type _epsilon
    ) noexcept
    {
        return (_v > -_epsilon) && (_v < _epsilon);
    }

    // sign_of
    //   -1, 0, or +1: a residual's sign.
    static D_CONSTEXPR
    int sign_of(
        value_type _v
    ) noexcept
    {
        return (_v < static_cast<value_type>(0)) ? -1
             : (_v > static_cast<value_type>(0)) ?  1
             :                                      0;
    }
};

template<typename Expr>
D_CONSTEXPR implicit_node<Expr> implicit(Expr _e) noexcept
{ return implicit_node<Expr>(_e); }

// level_set
//   the set { F(x,...) = level }, represented as implicit(F - level).
template<typename Expr, typename Level>
D_CONSTEXPR
auto level_set(
    Expr   _e,
    Level  _level
) noexcept -> implicit_node<decltype(_e - internal::as_expr(_level))>
{
    return implicit_node<decltype(_e - internal::as_expr(_level))>(
        _e - internal::as_expr(_level));
}


//==============================================================================
// 9.  MIGRATION ALIASES
//==============================================================================

// multivariable / mvar_*
//   the original positional-variable names, mapped onto variable_node.
template<typename ValueType, std::size_t ID>
using multivariable = variable_node<ID, ValueType>;

template<typename T = double> using mvar_x = variable_node<0, T>;
template<typename T = double> using mvar_y = variable_node<1, T>;
template<typename T = double> using mvar_z = variable_node<2, T>;
template<typename T = double> using mvar_w = variable_node<3, T>;
template<typename T = double> using mvar_u = variable_node<0, T>;
template<typename T = double> using mvar_v = variable_node<1, T>;


//==============================================================================
// 10. COORDINATE AXIS VARIABLE SETS
//==============================================================================
// correctly indexed per system.

// Per-system axis placeholders with the indices each system actually uses.
// Bringing the right set into scope keeps axis names unambiguous - in
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


//==============================================================================
// 11. NAMED FUNCTION WRAPPER
//==============================================================================

// math_function
//   the callable top-level wrapper: an expression bound to a coordinate
// system and a declared arity. Evaluable from a scalar pack, a std::array, a
// std::tuple or a linalg::vector (the latter three explode into the pack), so
// a coordinate point - the system's point_type, a linalg::vector -
// evaluates directly.
template<typename System, typename Expr, std::size_t Arity>
struct math_function
{
    using system_type       = System;
    using coordinate_system = System;
    using expression_type   = Expr;
    using value_type        = typename Expr::value_type;

    System system;
    Expr   expr;

    static constexpr std::size_t arity = Arity;
    static constexpr bool is_math_function = true;

    D_CONSTEXPR math_function(System _s, Expr _e) noexcept
        : system(_s), expr(_e) {}

    // scalar pack - excluded for a single array, tuple or vector argument
    // so the dedicated overloads below win unambiguously.
    template<typename... Args,
             typename std::enable_if<
                 !internal::first_is_container<Args...>::value, int>::type = 0>
    D_CONSTEXPR value_type operator()(Args&&... _args) const noexcept
    { return expr(_args...); }

    // std::array of coordinates
    template<typename T, std::size_t N>
    D_CONSTEXPR value_type operator()(const std::array<T, N>& _point) const noexcept
    { return eval_array(_point, re_std::make_index_sequence<N>{}); }

    // std::tuple of coordinates
    template<typename... TArgs>
    D_CONSTEXPR value_type operator()(const std::tuple<TArgs...>& _point) const noexcept
    { return eval_tuple(_point,
                        re_std::make_index_sequence<sizeof...(TArgs)>{}); }

    // linalg::vector of coordinates: a coordinate system's point
    template<typename T, std::size_t N>
    D_CONSTEXPR value_type
    operator()(const linalg::vector<T, N>& _point) const noexcept
    { return eval_array(_point, re_std::make_index_sequence<N>{}); }

private:
    // eval_array
    //   explodes an indexable point (a std::array or a linalg::vector).
    template<typename Point, std::size_t... Is>
    D_CONSTEXPR value_type
    eval_array(const Point&                  _p,
               re_std::index_sequence<Is...>) const noexcept
    { return expr(_p[Is]...); }

    template<typename Tuple, std::size_t... Is>
    D_CONSTEXPR value_type
    eval_tuple(const Tuple&                  _p,
               re_std::index_sequence<Is...>) const noexcept
    { return expr(std::get<Is>(_p)...); }
};


//==============================================================================
// 12. FLUENT BUILDER
//==============================================================================

// function_of
//   the builder stage after .of(...): the arity is fixed, awaiting .returns.
template<typename System, std::size_t Arity>
struct function_of
{
    System system;

    D_CONSTEXPR explicit function_of(System _s) noexcept : system(_s) {}

    template<typename Expr>
    D_CONSTEXPR math_function<System, Expr, Arity>
    returns(Expr _e) const noexcept
    { return math_function<System, Expr, Arity>(system, _e); }
};

// function_builder
//   the initial builder stage produced by function(system).
template<typename System>
struct function_builder
{
    System system;

    D_CONSTEXPR explicit function_builder(System _s) noexcept : system(_s) {}

    // .of(vars...) - declares the parameter list. The count must match the
    // system dimension when the system has one.
    template<typename... Vars>
    D_CONSTEXPR function_of<System, sizeof...(Vars)>
    of(Vars...) const noexcept
    {
        static_assert(internal::arity_ok<System, sizeof...(Vars)>::value,
                      "function(system).of(...): parameter count must match "
                      "the coordinate system dimension.");
        return function_of<System, sizeof...(Vars)>(system);
    }

    // .returns(expr) directly - arity is taken from the system dimension.
    template<typename Expr>
    D_CONSTEXPR
    math_function<System, Expr, internal::system_dimension<System>::value> returns(
        Expr _e
    ) const noexcept
    {
        return math_function<System, Expr,
                             internal::system_dimension<System>::value>(system, _e);
    }
};

// function
//   entry point: function(system) -> builder.
template<typename System>
D_CONSTEXPR function_builder<System> function(System _s) noexcept
{ return function_builder<System>(_s); }


#undef D_DETECT_MARKER
#undef D_DETECT_MARKER_V
#undef D_EXPR_RELATION_OP

NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_MATH_FUNCTION_FUNCTION_HPP
