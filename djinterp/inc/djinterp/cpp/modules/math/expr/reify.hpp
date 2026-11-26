/*******************************************************************************
* djinterp [cpp][math]                                                 reify.hpp
*
* reify: a compile-time expression made a term of the run-time language, so
* that it can be printed, simplified, substituted and evaluated by name, and
* differentiated where the language differentiates; roadmap step 32, 5d.
*   THE BRIDGE. expression.hpp's nodes are types, their operands held by
* value, and their variables positions; the language's term (term.hpp) is a
* tree of operators and atoms, its variables names. reify(e) walks e's type
* and builds the term node for node, in the same shape, nothing rewritten:
*     constant(v)            a number (below)
*     variable_node<I>       the variable named for index I
*     + - * /, unary -       the operators add, sub, mul, div and neg
*     exp sin cos tan sqrt   calls of the functions of those names
*     abs, log, atan         (log and atan from calculus/elementary.hpp)
*     dot(a, b)              a call of dot
*     < <= > >= == !=        the relations, == as =
*     && || ! (function.hpp) and, or, not; otherwise's condition true
*     piecewise              a call of piecewise, its branches in order
*     vec(c, ...)            a vector literal of its components
*     rational<N, D>         the exact fraction N/D (C++14, as constants.hpp)
* What the language has no term for, a parametric or an implicit function,
* and arithmetic in an integer type, whose division and overflow C++ takes
* as the language does not, is refused at compile time, with a message.
*   NAMES. A variable's name is given by its index: x, y, z and t for 0 to
* 3, as math::variables names them, by reify(e); or the names given, in
* index order, by reify(e, "r", "theta"), as many as e reads at least. Both
* are checked at compile time: e reads no index past the names it has. A
* name the constant table holds (pi, e, im, jm, km, true, false) evaluates
* as the variable it is in the term, but reads back as the constant from
* the term's text.
*   NUMBERS. A constant whose value is an integer, of magnitude below 2^53
* and not -0, is that exact integer, as the language reads 2 from its text,
* so that the simplifier collects a derivative's coefficients; any other
* float, double or long double is its value in its type, which the term's
* text writes as the shortest decimal that reads back to it; an integer is
* exact, a bool the constant true or false, and a vec3 a vector literal of
* three numbers.
*   VALUES. In double, a reified term's value is the expression's wherever
* its arithmetic, sqrt and abs are all it holds, bit for bit; the
* language's exp, sin, cos, tan, log and atan are the plain functions, whose
* answers ruling 12 decides, and the compile-time face's the C core's `_c`
* kernels (N3), which agree to the last bit where the plain ones are
* correctly rounded. A relation is 1 or 0 there, a NaN where an operand is
* one. A piecewise's last branch is its otherwise, as the compile-time face
* evaluates it, its condition never tested.
*   LEVELS. From C++11, where both faces are, and empty below it (N1).
* reify builds a run-time term, and so is no constant expression; the
* rational overload is from C++14, as constants.hpp's rational is.
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/reify.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  PARTS
    -----
    1.  Numbers
         1.  expr_reify_floating
         2.  expr_reify_number
    2.  Operators
         1.  expr_reify_op
    3.  Nodes
         1.  expr_reify
2.  THE ENTRIES
    -----------
    1.  reify
*/

#ifndef DJINTERP_MATH_EXPR_REIFY_HPP
#define DJINTERP_MATH_EXPR_REIFY_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cmath>        // std::signbit
#include <cstddef>      // std::size_t
#include <string>       // std::string
#include <tuple>        // std::get, std::tuple_size
#include <type_traits>  // std::enable_if, std::is_integral, std::decay
#include <vector>       // std::vector
// djinterp
#include "../../../../../re_std/cpp/utility/integer_sequence.hpp"       // index_
                                                                       // sequence
#include "../../../../../re_std/cpp/utility/make_integer_sequence.hpp"  // make_index_
                                                                       // sequence
#include "../calculus/elementary.hpp"   // fn_log, fn_atan
#include "../constants.hpp"             // rational (C++14)
#include "../expression.hpp"            // the compile-time nodes
#include "../function/function.hpp"     // relations, logic, piecewise, vec
#include "../math_common.hpp"           // NS_MATH
#include "../value.hpp"                 // value
#include "./operator.hpp"               // expr_op, expr_op_kind
#include "./term.hpp"                   // expr_term and its constructors


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  PARTS
//==============================================================================

NS_INTERNAL

    // expr_reify_names
    //   type: the variables' names, by index.
    typedef std::vector<std::string> expr_reify_names;

    // expr_reify_false
    //   trait: false, for a type, so that a static_assert fails only where
    // its template is made.
    template<typename _Type>
    struct expr_reify_false
    {
        static const bool value = false;
    };


// 1.1    Numbers
//------------------------------------------------------------------------------
    // 1.1.1
    // expr_reify_floating
    //   function: a float, double or long double as a number: the exact
    // integer where it is one, of magnitude below 2^53 and not -0; its value
    // in its type otherwise, NaNs and infinities among them.
    template<typename _Float>
    value
    expr_reify_floating(
        _Float _x
    )
    {
        const _Float limit = static_cast<_Float>(9007199254740992.0);

        // an integer below 2^53, -0 apart
        if ( (_x > -limit) &&
             (_x < limit)  &&
             (_x == static_cast<_Float>(static_cast<long long>(_x))) &&
             (!( (_x == static_cast<_Float>(0)) && (std::signbit(_x)) )) )
        {
            return value(static_cast<long long>(_x));
        }

        return value(_x);
    }

    // 1.1.2
    // expr_reify_number
    //   trait: a constant's term, by its type: an arithmetic type's number,
    // a bool's constant, a vec3's vector literal; any other refused.
    template<typename _Type,
             typename = void>
    struct expr_reify_number
    {
        static_assert(expr_reify_false<_Type>::value,
                      "reify: a constant of a type the language has no "
                      "number for; a constant is an integer, a float, a "
                      "double, a long double, a bool or a vec3.");
    };

    template<typename _Type>
    struct expr_reify_number<_Type,
                             typename std::enable_if<
                                 std::is_integral<_Type>::value &&
                                 !std::is_same<_Type, bool>::value>::type>
    {
        static expr_term
        make(
            _Type _v
        )
        {
            return expr_number(value(_v));
        }
    };

    template<typename _Type>
    struct expr_reify_number<_Type,
                             typename std::enable_if<
                                 std::is_floating_point<_Type>::value>::type>
    {
        static expr_term
        make(
            _Type _v
        )
        {
            return expr_number(expr_reify_floating(_v));
        }
    };

    template<>
    struct expr_reify_number<bool, void>
    {
        static expr_term
        make(
            bool _v
        )
        {
            return expr_constant( _v ? "true" : "false" );
        }
    };

    template<>
    struct expr_reify_number<vec3, void>
    {
        static expr_term
        make(
            const vec3& _v
        )
        {
            std::vector<expr_term> elements;

            elements.push_back(expr_number(expr_reify_floating(_v.x)));
            elements.push_back(expr_number(expr_reify_floating(_v.y)));
            elements.push_back(expr_number(expr_reify_floating(_v.z)));

            return expr_application(expr_op_kind::vector, elements);
        }
    };


// 1.2    Operators
//------------------------------------------------------------------------------
    // 1.2.1
    // expr_reify_op
    //   trait: an operation functor's term over its operands' terms: an
    // operator of the language, or a call of its function; any other
    // refused.
    template<typename _Op>
    struct expr_reify_op
    {
        static_assert(expr_reify_false<_Op>::value,
                      "reify: an operation the language has no operator or "
                      "function for.");
    };

    // the operators, and a call of one or two arguments by name
    template<expr_op_kind _Kind>
    struct expr_reify_kind
    {
        static expr_term
        make(
            const expr_term& _a
        )
        {
            return expr_application(_Kind, _a);
        }

        static expr_term
        make(
            const expr_term& _a,
            const expr_term& _b
        )
        {
            return expr_application(_Kind, _a, _b);
        }
    };

    template<typename _Name>
    struct expr_reify_call
    {
        static expr_term
        make(
            const expr_term& _a
        )
        {
            return expr_call(_Name::text(), std::vector<expr_term>(1, _a));
        }

        static expr_term
        make(
            const expr_term& _a,
            const expr_term& _b
        )
        {
            std::vector<expr_term> arguments;

            arguments.push_back(_a);
            arguments.push_back(_b);

            return expr_call(_Name::text(), arguments);
        }
    };

    #define D_EXPR_REIFY_NAME(NAME)                                           \
        struct expr_reify_name_##NAME                                         \
        {                                                                     \
            static const char* text() { return #NAME; }                       \
        };

    D_EXPR_REIFY_NAME(exp)
    D_EXPR_REIFY_NAME(sin)
    D_EXPR_REIFY_NAME(cos)
    D_EXPR_REIFY_NAME(tan)
    D_EXPR_REIFY_NAME(sqrt)
    D_EXPR_REIFY_NAME(abs)
    D_EXPR_REIFY_NAME(log)
    D_EXPR_REIFY_NAME(atan)
    D_EXPR_REIFY_NAME(dot)

    #undef D_EXPR_REIFY_NAME

    template<> struct expr_reify_op<op_add>
        : expr_reify_kind<expr_op_kind::add> {};
    template<> struct expr_reify_op<op_sub>
        : expr_reify_kind<expr_op_kind::sub> {};
    template<> struct expr_reify_op<op_mul>
        : expr_reify_kind<expr_op_kind::mul> {};
    template<> struct expr_reify_op<op_div>
        : expr_reify_kind<expr_op_kind::div> {};
    template<> struct expr_reify_op<op_neg>
        : expr_reify_kind<expr_op_kind::neg> {};
    template<> struct expr_reify_op<op_dot>
        : expr_reify_call<expr_reify_name_dot> {};
    template<> struct expr_reify_op<fn_exp>
        : expr_reify_call<expr_reify_name_exp> {};
    template<> struct expr_reify_op<fn_sin>
        : expr_reify_call<expr_reify_name_sin> {};
    template<> struct expr_reify_op<fn_cos>
        : expr_reify_call<expr_reify_name_cos> {};
    template<> struct expr_reify_op<fn_tan>
        : expr_reify_call<expr_reify_name_tan> {};
    template<> struct expr_reify_op<fn_sqrt>
        : expr_reify_call<expr_reify_name_sqrt> {};
    template<> struct expr_reify_op<fn_abs>
        : expr_reify_call<expr_reify_name_abs> {};
    template<> struct expr_reify_op<fn_log>
        : expr_reify_call<expr_reify_name_log> {};
    template<> struct expr_reify_op<fn_atan>
        : expr_reify_call<expr_reify_name_atan> {};
    template<> struct expr_reify_op<rel_lt>
        : expr_reify_kind<expr_op_kind::lt> {};
    template<> struct expr_reify_op<rel_le>
        : expr_reify_kind<expr_op_kind::le> {};
    template<> struct expr_reify_op<rel_gt>
        : expr_reify_kind<expr_op_kind::gt> {};
    template<> struct expr_reify_op<rel_ge>
        : expr_reify_kind<expr_op_kind::ge> {};
    template<> struct expr_reify_op<rel_eq>
        : expr_reify_kind<expr_op_kind::eq> {};
    template<> struct expr_reify_op<rel_ne>
        : expr_reify_kind<expr_op_kind::ne> {};


// 1.3    Nodes
//------------------------------------------------------------------------------
    // 1.3.1
    // expr_reify
    //   trait: a node's term, its operands' made first, in order, with the
    // variables' names given; any node the language has no term for
    // refused.
    template<typename _Node>
    struct expr_reify
    {
        static_assert(expr_reify_false<_Node>::value,
                      "reify: a node the language has no term for, as a "
                      "parametric or an implicit function.");
    };

    // the term of a node, by its type
    template<typename _Node>
    expr_term
    expr_reify_of(
        const _Node&            _node,
        const expr_reify_names& _names
    )
    {
        return expr_reify<typename std::decay<_Node>::type>::make(_node,
                                                                  _names);
    }

    // a constant: its number
    template<typename _ValueType>
    struct expr_reify<constant_node<_ValueType> >
    {
        static expr_term
        make(
            const constant_node<_ValueType>& _node,
            const expr_reify_names&
        )
        {
            return expr_reify_number<_ValueType>::make(_node.value);
        }
    };

    // a variable: its name, by its index
    template<std::size_t _Index,
             typename    _ValueType>
    struct expr_reify<variable_node<_Index, _ValueType> >
    {
        static expr_term
        make(
            const variable_node<_Index, _ValueType>&,
            const expr_reify_names& _names
        )
        {
            return expr_variable(_names[_Index]);
        }
    };

    // a binary operation: its operator over its two operands
    template<typename _Op,
             typename _Left,
             typename _Right>
    struct expr_reify<binary_node<_Op, _Left, _Right> >
    {
        static_assert(!std::is_integral<
                          typename binary_node<_Op, _Left,
                                               _Right>::value_type>::value,
                      "reify: arithmetic in an integer type, whose division "
                      "and overflow the language has no term for; make a "
                      "constant a double, constant(2.0).");

        static expr_term
        make(
            const binary_node<_Op, _Left, _Right>& _node,
            const expr_reify_names&                _names
        )
        {
            return expr_reify_op<_Op>::make(expr_reify_of(_node.left, _names),
                                            expr_reify_of(_node.right,
                                                          _names));
        }
    };

    // a unary operation: its operator or function over its operand
    template<typename _Op,
             typename _Arg>
    struct expr_reify<unary_node<_Op, _Arg> >
    {
        static_assert(!std::is_integral<
                          typename unary_node<_Op, _Arg>::value_type>::value,
                      "reify: arithmetic in an integer type, which the "
                      "language has no term for; make a constant a double, "
                      "constant(2.0).");

        static expr_term
        make(
            const unary_node<_Op, _Arg>& _node,
            const expr_reify_names&      _names
        )
        {
            return expr_reify_op<_Op>::make(expr_reify_of(_node.arg,
                                                          _names));
        }
    };

    // a relation: its operator over its two operands
    template<typename _Op,
             typename _Left,
             typename _Right>
    struct expr_reify<relation_node<_Op, _Left, _Right> >
    {
        static expr_term
        make(
            const relation_node<_Op, _Left, _Right>& _node,
            const expr_reify_names&                  _names
        )
        {
            return expr_reify_op<_Op>::make(expr_reify_of(_node.left, _names),
                                            expr_reify_of(_node.right,
                                                          _names));
        }
    };

    // and, or, not
    template<typename _Left,
             typename _Right>
    struct expr_reify<logical_and_node<_Left, _Right> >
    {
        static expr_term
        make(
            const logical_and_node<_Left, _Right>& _node,
            const expr_reify_names&                _names
        )
        {
            return expr_application(expr_op_kind::logical_and,
                                    expr_reify_of(_node.left, _names),
                                    expr_reify_of(_node.right, _names));
        }
    };

    template<typename _Left,
             typename _Right>
    struct expr_reify<logical_or_node<_Left, _Right> >
    {
        static expr_term
        make(
            const logical_or_node<_Left, _Right>& _node,
            const expr_reify_names&               _names
        )
        {
            return expr_application(expr_op_kind::logical_or,
                                    expr_reify_of(_node.left, _names),
                                    expr_reify_of(_node.right, _names));
        }
    };

    template<typename _Arg>
    struct expr_reify<logical_not_node<_Arg> >
    {
        static expr_term
        make(
            const logical_not_node<_Arg>& _node,
            const expr_reify_names&       _names
        )
        {
            return expr_application(expr_op_kind::logical_not,
                                    expr_reify_of(_node.arg, _names));
        }
    };

    // otherwise's condition: the constant true
    template<>
    struct expr_reify<always_true_node>
    {
        static expr_term
        make(
            const always_true_node&,
            const expr_reify_names&
        )
        {
            return expr_constant("true");
        }
    };

    // expr_reify_pieces
    //   the arguments of piecewise from a piecewise node's branches: each
    // branch's condition and body, the last's body alone, its otherwise.
    template<std::size_t _I,
             std::size_t _Count,
             bool        _Last = ((_I + 1) == _Count)>
    struct expr_reify_pieces
    {
        template<typename _Tuple>
        static void
        add(
            const _Tuple&            _pieces,
            const expr_reify_names&  _names,
            std::vector<expr_term>&  _arguments
        )
        {
            _arguments.push_back(expr_reify_of(std::get<_I>(_pieces).cond,
                                               _names));
            _arguments.push_back(expr_reify_of(std::get<_I>(_pieces).body,
                                               _names));
            expr_reify_pieces<(_I + 1), _Count>::add(_pieces,
                                                     _names,
                                                     _arguments);

            return;
        }
    };

    template<std::size_t _I,
             std::size_t _Count>
    struct expr_reify_pieces<_I, _Count, true>
    {
        template<typename _Tuple>
        static void
        add(
            const _Tuple&            _pieces,
            const expr_reify_names&  _names,
            std::vector<expr_term>&  _arguments
        )
        {
            _arguments.push_back(expr_reify_of(std::get<_I>(_pieces).body,
                                               _names));

            return;
        }
    };

    // a piecewise: a call of piecewise, its branches in order
    template<typename... _Pieces>
    struct expr_reify<piecewise_node<_Pieces...> >
    {
        static expr_term
        make(
            const piecewise_node<_Pieces...>& _node,
            const expr_reify_names&           _names
        )
        {
            std::vector<expr_term> arguments;

            expr_reify_pieces<0, sizeof...(_Pieces)>::add(_node.pieces,
                                                          _names,
                                                          arguments);

            return expr_call("piecewise", arguments);
        }
    };

    // expr_reify_components
    //   a vector node's components' terms, in order.
    template<typename _Tuple,
             std::size_t... _Indices>
    std::vector<expr_term>
    expr_reify_components(
        const _Tuple&            _components,
        const expr_reify_names&  _names,
        re_std::index_sequence<_Indices...>
    )
    {
        std::vector<expr_term> elements;
        const int expand[] = { 0,
            ( elements.push_back(expr_reify_of(std::get<_Indices>(_components),
                                               _names)), 0 )... };

        static_cast<void>(expand);
        static_cast<void>(_names);

        return elements;
    }

    // a vector: a vector literal of its components
    template<typename... _Components>
    struct expr_reify<vector_node<_Components...> >
    {
        static expr_term
        make(
            const vector_node<_Components...>& _node,
            const expr_reify_names&            _names
        )
        {
            return expr_application(
                expr_op_kind::vector,
                expr_reify_components(
                    _node.components,
                    _names,
                    re_std::make_index_sequence<sizeof...(_Components)>()));
        }
    };

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    // a rational: the exact fraction, an integer where its denominator is 1
    template<re_std::intmax_t _Numerator,
             re_std::intmax_t _Denominator>
    struct expr_reify<rational<_Numerator, _Denominator> >
    {
        static expr_term
        make(
            const rational<_Numerator, _Denominator>&,
            const expr_reify_names&
        )
        {
            typedef rational<_Numerator, _Denominator> fraction;

            dj_rational exact;

            exact.num = static_cast<dj_math_imax>(fraction::numerator);
            exact.den = static_cast<dj_math_imax>(fraction::denominator);

            return ( (exact.den == 1)
                     ? expr_number(value(static_cast<long long>(exact.num)))
                     : expr_number(value(exact)) );
        }
    };
#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

NS_END  // internal


//==============================================================================
// 2.  THE ENTRIES
//==============================================================================

// 2.1    reify
//------------------------------------------------------------------------------
// 2.1.1
// reify
/**
 * @brief A compile-time expression as a term of the run-time language, its
 *        variables x, y, z and t by index.
 *
 * @param[in] _e  the expression; it reads no index past 3, which a
 *                static_assert checks.
 * @return the term, of _e's shape, each node its operator, call or atom, as
 *         the banner says; printed by expr_render, wrapped by math::expr.
 */
template<typename _Expr,
         typename std::enable_if<is_expression<_Expr>::value,
                                 int>::type = 0>
D_NODISCARD expr_term
reify(
    const _Expr& _e
)
{
    static_assert((std::decay<_Expr>::type::arity <= 4),
                  "reify: an expression that reads an index past 3, which "
                  "x, y, z and t do not name; give its names, "
                  "reify(e, \"a\", \"b\", ...).");

    internal::expr_reify_names names;

    names.push_back("x");
    names.push_back("y");
    names.push_back("z");
    names.push_back("t");

    return internal::expr_reify_of(_e, names);
}

/**
 * @brief A compile-time expression as a term of the run-time language, its
 *        variables named in index order.
 *
 * @param[in] _e      the expression; it reads no index past the names
 *                    given, which a static_assert checks.
 * @param[in] _first  the name of index 0, then _rest's of 1, 2, ...: each
 *                    anything a std::string is made from.
 * @return the term, as reify(_e)'s, each variable of index i the i-th name.
 */
template<typename    _Expr,
         typename    _First,
         typename... _Rest,
         typename std::enable_if<is_expression<_Expr>::value,
                                 int>::type = 0>
D_NODISCARD expr_term
reify(
    const _Expr&    _e,
    const _First&   _first,
    const _Rest&... _rest
)
{
    static_assert((std::decay<_Expr>::type::arity <= (1 + sizeof...(_Rest))),
                  "reify: an expression that reads an index past the names "
                  "given; give a name for each index it reads.");

    const std::string given[] = { std::string(_first), std::string(_rest)... };

    return internal::expr_reify_of(
        _e,
        internal::expr_reify_names(given, given + (1 + sizeof...(_Rest))));
}


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_EXPR_REIFY_HPP
