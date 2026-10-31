/*******************************************************************************
* djinterp [cpp/math]                                               evaluate.hpp
*
* The folds to numbers: a term's value at its variables' values, in a
* floating-point type or as a math::value; the fourth layer's first half.
*   TWO FOLDS. expr_evaluate<T>, for float, double and long double, is typed
* and fast: each literal rounded once, from its text, to T (gate G3), each
* function the C core's in T. expr_evaluate with an expr_context gives a
* math::value, exact where the value layer is: 1/3 + 1/6 is exactly 1/2.
* Both are one fold of parse/expression's over the term.
*   BINDINGS give the variables their values, by name: at("x", 2.0) binds
* one, and its at(...) the next. A variable no binding gives a value is an
* error, unbound_variable; an error of arithmetic is a value, a NaN in T or
* an error value, as math::value's are.
*   THE CONTEXT will carry what a computation at a chosen precision needs:
* a big float's precision, the rounding mode and the error policy (roadmap
* Phase 1). It has no field yet, since no kind reads one (as S70 left
* value.h), and gains each with the first kind that does (Phase 8).
*   IN A TYPE. + - * / are the type's own; ^ is the C core's pow in it, the
* C library's (power.h); a mod b is the floored remainder, which has b's
* sign, as in Python, MATLAB and Mathematica: the C core's fmod, plus b
* where the two signs differ, and 0 with b's sign; n! is the factorial of a
* whole n >= 0, a NaN for any other n. A variable is its bound value's
* nearest in T, and a boolean 1 or 0; an error or none, which have neither,
* is a NaN, as arithmetic's errors are in T. A constant is the constant
* table's value in T, true and false 1 and 0; a call, its function's
* evaluator in T (function.hpp). The relations are C's comparisons, 1 where
* one holds and 0 where it does not, none holding of a NaN but !=; and, or
* and not read 0 as false, a NaN as unknown and any other number as true,
* as Kleene's logic: 0 and anything is 0, 1 or anything is 1. x' is x, as
* a number is its own transpose, and indexing, which only a matrix takes, is
* a NaN, as a domain error is in T (S249).
*   AS A VALUE. + - * / are value.h's, exact for exact operands. ^ is exact
* for an exact base and an exact whole exponent, by repeated squaring, and
* for an exact rational exponent p/q where the base's terms are whole q-th
* powers: 4 ^ 0.5 is 2, 8 ^ (1/3) is 2. Otherwise its value is the C core's
* pow in the kind the tower gives: the wider of the inexact operands', a
* long double where both are exact, as a root's is. An exact negative base
* to a power not whole is a domain error, exact 0 to a negative one a pole.
* mod is exact for exact operands, a - b floor(a / b), and a domain error
* where b is exactly 0; otherwise in the tower's kind, as ^. n! is exact for
* an exact whole n >= 0, past the family G2's long double, and a domain
* error for any other exact n. Errors pass, the first; a boolean or none in
* arithmetic is a domain error; every result carries its operands' flags,
* as value.h's do. A constant is the constant table's value; a call, its
* function's evaluator as a value, exact wherever the function's value is
* rational (function.hpp, whose arithmetic ^, mod and ! are). A relation is
* a boolean: of two numbers by value.h's exact order, so that 0.1 + 0.2 = 0.3
* holds, none holding of a NaN but !=; of two booleans, = and != by their
* truth; of anything else a domain error. and, or and not take booleans,
* Kleene's: false and anything is false, true or anything is true, an error
* among the others too; otherwise an error passes, the first, and what is no
* boolean is a domain error. (function.hpp has both.)
*   LITERALS. As a value, a vector literal [a, b, c] is the matrix of its
* elements beside one another, and a matrix literal [a, b; c, d] that of its
* rows one above another, each element a block (S261, matrix.hpp): a number
* is one of 1 by 1, so that a literal of numbers alone is the matrix of
* them, as expr_vector and expr_matrix make it, each element kept as it is,
* as values where any element is exact, and densely where every one is a
* float, a double or a long double (S239); a matrix is one of its own shape,
* [A, b] A with b beside it, and the empty matrix one that is skipped; [] is
* the empty matrix (S260). An element's error passes, the first; an element
* that is no number and no matrix, a boolean or none, is a domain error
* (S241), and so are blocks that do not fit, of other numbers of rows beside
* one another or of columns one above another, as in the arithmetic of
* shapes (S246): a row of more or fewer numbers than the first among them,
* which the parser reports where it can, a term made by hand where it
* cannot.
* In a type a literal has no value: bad_shape (S242), an operand's error
* coming first, as at any node: a vector's first element's error, and a
* matrix's first row's, each row a literal of its own; a variable bound to a
* matrix is a NaN there, as one bound to none is.
*   MATRICES, as values, take + - * / as the matrix kind has them (S246):
* A + B and A - B of one shape, -A, A * s, s * A and A / s, and A * B where
* A's columns are B's rows; any other is a domain error. A ^ n is the power
* of a square matrix to a whole n, a negative one its inverse's, a pole for
* a singular matrix (S248, S259); A' the transpose, and
* of a number the number; A[i, j] an element, v[i] a vector's, A[i] the row
* i of another matrix, each index from 1 and whole (S249); and A = B and
* A != B compare two matrices element by element (S242). An error passes
* first, as anywhere, and indexing a number, or by a number not whole or
* out of the matrix, is a domain error (matrix.hpp has them all). A term
* made by hand that indexes by none or by three is wrong_arity.
*   CALLS AND CONSTANTS are looked up in the standard tables, when a fold
* meets them: a function or a constant neither table has is unknown_name,
* and a call of a number of arguments its function does not take
* wrong_arity. An argument's error passes, the first, before either. Linear
* algebra's functions take matrices as values, and numbers as scalars in a
* type (function.hpp, S256).
*   STEPS. Step 14 (2b) filled both folds for numbers, variables and the
* arithmetic, step 15 (2c) for calls, constants and the factorial, step 16
* (2d) for the relations and logic, piecewise a call, step 19 (3a) for the
* literals, step 20 (3b) for indexing, the transpose and the operators on
* matrices, and step 21 (3c) for blocks and the empty matrix: no node of the
* language answers expr_status::not_yet.
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/evaluate.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  WHAT A FOLD IS GIVEN
    --------------------
    1.  Bindings
         1.  expr_bindings
         2.  at
    2.  The context
         1.  expr_context
2.  THE OPERATIONS
    --------------
    1.  What a fold cannot compute
         1.  expr_fold_error
         2.  expr_function_of, expr_constant_of
    2.  In a floating-point type
         1.  The leaves
         2.  The operators and the calls
    3.  As a value
         1.  The operators
3.  THE FOLDS
    ---------
    1.  In a floating-point type
         1.  expr_evaluate<Type>
    2.  As a value
         1.  expr_evaluate
*/

#ifndef DJINTERP_MATH_EXPR_EVALUATE_HPP
#define DJINTERP_MATH_EXPR_EVALUATE_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>  // std::size_t
#include <limits>   // std::numeric_limits
#include <string>   // std::string
#include <utility>  // std::pair
#include <vector>   // std::vector
// djinterp
#include "../../parse/expression/expression_ops.hpp"    // evaluate
#include "../math_common.hpp"                           // NS_MATH
#include "../value.hpp"                                 // value
#include "./expr_common.hpp"                            // expr_result
#include "./function.hpp"                               // the tables, and
                                                        // the arithmetic
#include "./matrix.hpp"                                 // expr_matrix
#include "./operator.hpp"                               // expr_signature
#include "./term.hpp"                                   // expr_term
#include "./token.hpp"                                  // expr_read_decimal


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  WHAT A FOLD IS GIVEN
//==============================================================================


// 1.1    Bindings
//------------------------------------------------------------------------------
// 1.1.1
// expr_bindings
//   class: the values of variables, by name, at most one each: what a fold
// reads a variable's value from. at binds a name, or binds it anew, and
// returns the bindings, so that bindings chain: at("x", 1).at("y", 2).
class expr_bindings
{
public:
    // no binding.
    expr_bindings()
        : m_entries()
    {}

    // at
    //   binds _name to _value, in place of any value it had. Returns the
    // bindings, for the next.
    expr_bindings& at(
        const std::string& _name,
        const value&       _value
    )
    {
        // a value bound to the name already
        for (std::size_t i = 0; i < m_entries.size(); ++i)
        {
            if (m_entries[i].first == _name)
            {
                m_entries[i].second = _value;

                return *this;
            }
        }

        m_entries.push_back(std::pair<std::string, value>(_name, _value));

        return *this;
    }

    // find
    //   the value bound to _name, or null where none is; the pointer is
    // good until the bindings next change.
    D_NODISCARD const value* find(
        const std::string& _name
    ) const
    {
        // the binding of the name
        for (std::size_t i = 0; i < m_entries.size(); ++i)
        {
            if (m_entries[i].first == _name)
            {
                return &m_entries[i].second;
            }
        }

        return D_NULLPTR;
    }

    D_NODISCARD std::size_t size() const D_NOEXCEPT
    {
        return m_entries.size();
    }

    D_NODISCARD bool empty() const D_NOEXCEPT
    {
        return m_entries.empty();
    }

private:
    std::vector<std::pair<std::string, value> > m_entries;
};

// 1.1.2
// at
//   function: bindings of one variable, _name to _value, to which at adds
// more: at("x", 2.0).at("y", 3).
D_NODISCARD inline expr_bindings at(
    const std::string& _name,
    const value&       _value
)
{
    expr_bindings bindings;
    bindings.at(_name, _value);

    return bindings;
}

// 1.2    The context
//------------------------------------------------------------------------------
// 1.2.1
// expr_context
//   struct: what a fold to a value computes under. No field yet: a big
// float's precision, the rounding mode and the error policy come with the
// first kind that reads them.
struct expr_context
{
    expr_context() D_NOEXCEPT
    {}
};



//==============================================================================
// 2.  THE OPERATIONS
//==============================================================================
// What each fold does at a node: its errors, and what it computes there; the
// arithmetic itself, beyond value.h's, is function.hpp's.


// 2.1    What a fold cannot compute
//------------------------------------------------------------------------------
// 2.1.1
// expr_fold_error
//   function: the error of a node a fold does not compute: an operator
// applied to a number of operands it does not take, wrong_arity; an
// operator of no kind the language has, unknown_name. From step 20 (3b)
// every kind of the language is filled, so that no node answers not_yet.
NS_INTERNAL

    // expr_fold_error
    //   function: as the item says, for an application of _op to _operands
    // operands, which no fold computes.
    inline
    expr_error expr_fold_error(
        const expr_op& _op,
        std::size_t    _operands
    )
    {
        const operator_descriptor* descriptor =
            expr_signature::standard().describe(_op);

        // a kind of no step
        if (descriptor == D_NULLPTR)
        {
            return expr_error(expr_status::unknown_name,
                              expr_no_offset,
                              "no operator of the language is of kind " +
                                  std::to_string(static_cast<int>(_op.kind)));
        }

        return expr_error(expr_status::wrong_arity,
                          expr_no_offset,
                          std::string("'") + descriptor->spelling +
                              "' takes " + std::to_string(descriptor->arity) +
                              ( (descriptor->arity == 1) ? " operand, not "
                                                         : " operands, not " ) +
                              std::to_string(_operands));
    }

    // expr_is_literal
    //   function: whether an operator is a literal's, vector, matrix or row.
    inline
    bool expr_is_literal(
        expr_op_kind _kind
    )
    {
        return ( (_kind == expr_op_kind::vector) ||
                 (_kind == expr_op_kind::matrix) ||
                 (_kind == expr_op_kind::row) );
    }

    // expr_unbound
    //   function: the error of a variable no binding gives a value.
    inline
    expr_error expr_unbound(
        const std::string& _name
    )
    {
        return expr_error(expr_status::unbound_variable,
                          expr_no_offset,
                          "the variable " + _name + " is bound to no value");
    }

    // expr_no_term
    //   function: the error of a term made by default, which is no term and
    // has no value: missing_operand.
    inline
    expr_error expr_no_term()
    {
        return expr_error(expr_status::missing_operand,
                          expr_no_offset,
                          "a term made by default has no value");
    }

    // expr_takes
    //   function: whether an operator of the table's takes _operands
    // operands, its arity: two for the binary, one for the unary, the
    // factorial, the transpose and not.
    inline
    bool expr_takes(
        expr_op_kind _kind,
        std::size_t  _operands
    )
    {
        const operator_descriptor* descriptor =
            expr_signature::standard().describe(expr_op(_kind));

        return ( (descriptor != D_NULLPTR) &&
                 (descriptor->arity == _operands) );
    }

    // expr_index_error
    //   function: the error of indexing of a number of indices it does not
    // take, one or two: wrong_arity. (Only a term made by hand has one; the
    // parser reads none.)
    inline
    expr_error expr_index_error(
        std::size_t _operands
    )
    {
        const std::size_t indices = ( (_operands > 0) ? (_operands - 1) : 0 );

        return expr_error(expr_status::wrong_arity,
                          expr_no_offset,
                          "indexing takes 1 or 2 indices, not " +
                              std::to_string(indices));
    }

    // expr_index_takes
    //   function: whether indexing takes _operands operands: the indexed
    // one, and one index or two.
    inline
    bool expr_index_takes(
        std::size_t _operands
    )
    {
        return ( (_operands == 2) ||
                 (_operands == 3) );
    }

    // 2.1.2
    // expr_function_of, expr_constant_of
    //   functions: a call's function, the standard table's entry of its name
    // where it takes the call's arguments; and a constant's entry. Where
    // there is none, the error: unknown_name for a name the table has not,
    // wrong_arity for a call of a number of arguments its function does not
    // take.

    // expr_arguments_text
    //   function: the numbers of arguments a standard function takes, in
    // words: 1 argument, 2 arguments, at least 1 argument. Each takes one
    // number of them, or any number from its least.
    inline
    std::string expr_arguments_text(
        const expr_function& _function
    )
    {
        const std::size_t least = _function.min_arity;

        return ( (_function.max_arity == expr_variadic) ? "at least "
                                                         : "" ) +
               std::to_string(least) +
               ( (least == 1) ? " argument" : " arguments" );
    }

    inline
    expr_result<const expr_function*> expr_function_of(
        const expr_op& _call,
        std::size_t    _arguments
    )
    {
        const expr_function* function =
            expr_standard_functions().find(_call.function);

        // a name of no function
        if (function == D_NULLPTR)
        {
            return err<const expr_function*, expr_error>(
                expr_error(expr_status::unknown_name,
                           expr_no_offset,
                           "no function of the language is named " +
                               _call.function));
        }

        // a number of arguments it does not take
        if (!function->takes(_arguments))
        {
            return err<const expr_function*, expr_error>(
                expr_error(expr_status::wrong_arity,
                           expr_no_offset,
                           _call.function + " takes " +
                               expr_arguments_text(*function) + ", not " +
                               std::to_string(_arguments)));
        }

        return ok<const expr_function*, expr_error>(function);
    }

    inline
    expr_result<const expr_named_constant*> expr_constant_of(
        const std::string& _name
    )
    {
        const expr_named_constant* constant =
            expr_standard_constants().find(_name);

        // a name of no constant
        if (constant == D_NULLPTR)
        {
            return err<const expr_named_constant*, expr_error>(
                expr_error(expr_status::unknown_name,
                           expr_no_offset,
                           "no constant of the language is named " + _name));
        }

        return ok<const expr_named_constant*, expr_error>(constant);
    }

NS_END  // internal

// 2.2    In a floating-point type
//------------------------------------------------------------------------------
NS_INTERNAL

    // 2.2.1
    // The leaves
    //   a number in Type: its literal rounded once, by decimal.h
    // (expr_read_decimal), where the literal is a decimal number, and its
    // value's nearest otherwise; and a constant's value in Type.

    // expr_number_in
    //   function: a number atom in Type: its literal rounded once, or its
    // value's nearest.
    template<typename Type>
    Type expr_number_in(
        const expr_atom& _atom
    )
    {
        Type read = Type(0);

        // a decimal literal, rounded once
        if ( (!_atom.text.empty()) &&
             (expr_read_decimal(read, _atom.text)) )
        {
            return read;
        }

        return expr_value_in<Type>(_atom.number);
    }

    // expr_constant_in
    //   function: a constant's value in Type, as its entry holds it.
    template<typename Type>
    Type expr_constant_in(
        const expr_named_constant& _constant
    );

    template<>
    inline float
    expr_constant_in<float>(
        const expr_named_constant& _constant
    )
    {
        return _constant.as_float;
    }

    template<>
    inline double
    expr_constant_in<double>(
        const expr_named_constant& _constant
    )
    {
        return _constant.as_double;
    }

    template<>
    inline long double
    expr_constant_in<long double>(
        const expr_named_constant& _constant
    )
    {
        return _constant.as_long_double;
    }

    // 2.2.2
    // The operators and the calls
    //   an operator of steps 14 to 16 applied to its operands in Type; and a
    // function's evaluator in Type, as its entry holds it.

    // expr_apply_in
    //   function: an operator of steps 14 to 16 applied to its operands in
    // Type, which it takes in their number.
    template<typename Type>
    Type expr_apply_in(
        expr_op_kind             _kind,
        const std::vector<Type>& _x
    )
    {
        // by kind
        switch (_kind)
        {
            case expr_op_kind::add:       return (_x[0] + _x[1]);
            case expr_op_kind::sub:       return (_x[0] - _x[1]);
            case expr_op_kind::mul:       return (_x[0] * _x[1]);
            case expr_op_kind::div:       return (_x[0] / _x[1]);
            case expr_op_kind::mod:       return expr_mod_in(_x[0], _x[1]);
            case expr_op_kind::pow:       return expr_pow_in(_x[0], _x[1]);
            case expr_op_kind::neg:       return (-_x[0]);
            case expr_op_kind::transpose: return _x[0];
            case expr_op_kind::factorial: return expr_factorial_in(_x[0]);
            case expr_op_kind::eq:
            case expr_op_kind::ne:
            case expr_op_kind::lt:
            case expr_op_kind::le:
            case expr_op_kind::gt:
            case expr_op_kind::ge:
                return expr_relation_in(_kind, _x[0], _x[1]);
            case expr_op_kind::logical_and: return expr_all_in(_x.data(), 2);
            case expr_op_kind::logical_or:  return expr_any_in(_x.data(), 2);
            case expr_op_kind::logical_not: return expr_not_in(_x[0]);
            case expr_op_kind::pos:
            default:
                break;
        }

        return (+_x[0]);
    }

    // expr_evaluator_in
    //   function: a function's evaluator in Type.
    template<typename Type>
    expr_evaluator<Type> expr_evaluator_in(
        const expr_function& _function
    );

    template<>
    inline expr_evaluator<float>
    expr_evaluator_in<float>(
        const expr_function& _function
    )
    {
        return _function.evaluate_float;
    }

    template<>
    inline expr_evaluator<double>
    expr_evaluator_in<double>(
        const expr_function& _function
    )
    {
        return _function.evaluate_double;
    }

    template<>
    inline expr_evaluator<long double>
    expr_evaluator_in<long double>(
        const expr_function& _function
    )
    {
        return _function.evaluate_long_double;
    }

NS_END  // internal

// 2.3    As a value
//------------------------------------------------------------------------------
NS_INTERNAL

    // 2.3.1
    // The operators
    //   function: an operator of steps 14 to 16 applied to its operands as
    // values, which it takes in their number: value.h's + - * / and -,
    // function.hpp's ^, mod and !, its relations and its logic; + an error or
    // a number itself, and a domain error for what is neither.
    inline
    value expr_apply_value(
        expr_op_kind              _kind,
        const std::vector<value>& _x
    )
    {
        // by kind
        switch (_kind)
        {
            case expr_op_kind::add: return (_x[0] + _x[1]);
            case expr_op_kind::sub: return (_x[0] - _x[1]);
            case expr_op_kind::mul: return (_x[0] * _x[1]);
            case expr_op_kind::div: return (_x[0] / _x[1]);
            case expr_op_kind::mod: return expr_value_mod(_x[0], _x[1]);
            case expr_op_kind::pow: return expr_value_pow(_x[0], _x[1]);
            case expr_op_kind::neg: return (-_x[0]);
            case expr_op_kind::transpose: return expr_transpose_value(_x[0]);
            case expr_op_kind::factorial:
                return expr_value_fn(expr_fn::factorial, _x.data(), 1);
            case expr_op_kind::eq:
            case expr_op_kind::ne:
            case expr_op_kind::lt:
            case expr_op_kind::le:
            case expr_op_kind::gt:
            case expr_op_kind::ge:
                return expr_relation_value(_kind, _x[0], _x[1]);
            case expr_op_kind::logical_and:
                return expr_all_value(_x.data(), 2);
            case expr_op_kind::logical_or:
                return expr_any_value(_x.data(), 2);
            case expr_op_kind::logical_not:
                return expr_not_value(_x[0]);
            case expr_op_kind::pos:
            default:
                break;
        }

        // + of a number, a matrix or an error, itself
        if ( (_x[0].is_number()) ||
             (expr_is_matrix(_x[0])) ||
             (_x[0].kind() == D_MATH_VALUE_ERROR) )
        {
            return _x[0];
        }

        return value(D_MATH_VALUE_ERROR_DOMAIN);
    }

NS_END  // internal


//==============================================================================
// 3.  THE FOLDS
//==============================================================================
// Each is one fold of parse/expression's over the term, its carrier the
// answer of a step: an operand's error passes, the first, and an operator
// is applied only to the operands it takes, a function only to the
// arguments it takes.


// 3.1    In a floating-point type
//------------------------------------------------------------------------------
// 3.1.1
// expr_evaluate<Type>
/**
 * @brief The value of a term in float, double or long double.
 *
 * @param[in] _term      the term.
 * @param[in] _bindings  the values of its variables.
 * @return the term's value in Type, each literal rounded once from its text
 *         and each operation and function as the banner says; or the error
 *         unbound_variable; unknown_name for a function or a constant the
 *         standard tables have not, or an operator of no kind the language
 *         has; wrong_arity for an operator or a function applied to a
 *         number of operands it does not take; bad_shape for a vector or a
 *         matrix, which has no value in a type; or missing_operand for a
 *         term made by default.
 */
template<typename Type>
D_NODISCARD expr_result<Type> expr_evaluate(
    const expr_term&     _term,
    const expr_bindings& _bindings = expr_bindings()
)
{
    D_STATIC_ASSERT(internal::expr_floating<Type>::value,
                    "expr_evaluate<Type>: Type is float, double or long "
                    "double; a math::value comes from expr_evaluate with "
                    "an expr_context.");

    using answer = expr_result<Type>;

    // no term: a term made by default
    if (_term.empty())
    {
        return err<Type, expr_error>(internal::expr_no_term());
    }

    return ::djinterp::evaluate<answer, expr_op, expr_atom>(
        _term,
        // a leaf: a number, a constant, or a bound variable
        [&_bindings](const expr_atom& _atom) -> answer
        {
            // by kind
            if (_atom.kind == expr_atom_kind::number)
            {
                return ok<Type, expr_error>(
                    internal::expr_number_in<Type>(_atom));
            }

            if (_atom.kind == expr_atom_kind::constant)
            {
                const expr_result<const expr_named_constant*> constant =
                    internal::expr_constant_of(_atom.text);

                // a name of no constant
                if (constant.is_err())
                {
                    return err<Type, expr_error>(constant.error());
                }

                return ok<Type, expr_error>(
                    internal::expr_constant_in<Type>(*constant.value()));
            }

            const value* bound = _bindings.find(_atom.text);

            // a variable no binding gives a value
            if (bound == D_NULLPTR)
            {
                return err<Type, expr_error>(
                    internal::expr_unbound(_atom.text));
            }

            return ok<Type, expr_error>(internal::expr_value_in<Type>(*bound));
        },
        // an application: its operands' first error, or the operation
        [](const expr_op& _op, const std::vector<answer>& _operands) -> answer
        {
            std::vector<Type> x;

            // an operand's error passes
            for (std::size_t i = 0; i < _operands.size(); ++i)
            {
                if (_operands[i].is_err())
                {
                    return _operands[i];
                }

                x.push_back(_operands[i].value());
            }

            // a literal: no value in a type
            if (internal::expr_is_literal(_op.kind))
            {
                return err<Type, expr_error>(
                    expr_error(expr_status::bad_shape,
                               expr_no_offset,
                               "expr_evaluate<Type>: a vector or a matrix "
                               "has no value in a type"));
            }

            // indexing: in a type no operand is a matrix, so a NaN
            if (_op.kind == expr_op_kind::index)
            {
                return ( internal::expr_index_takes(x.size())
                             ? ok<Type, expr_error>(
                                   std::numeric_limits<Type>::quiet_NaN())
                             : err<Type, expr_error>(
                                   internal::expr_index_error(x.size())) );
            }

            // a call: its function's evaluator, on the arguments it takes
            if (_op.kind == expr_op_kind::call)
            {
                const expr_result<const expr_function*> function =
                    internal::expr_function_of(_op, x.size());

                if (function.is_err())
                {
                    return err<Type, expr_error>(function.error());
                }

                return ok<Type, expr_error>(
                    internal::expr_evaluator_in<Type>(*function.value())(
                        x.data(),
                        x.size()));
            }

            // an operator of the table's, on the operands it takes
            if (internal::expr_takes(_op.kind,
                                     x.size()))
            {
                return ok<Type, expr_error>(
                    internal::expr_apply_in<Type>(_op.kind, x));
            }

            return err<Type, expr_error>(
                internal::expr_fold_error(_op,
                                          x.size()));
        });
}

// 3.2    As a value
//------------------------------------------------------------------------------
// 3.2.1
// expr_evaluate
/**
 * @brief The value of a term as a math::value, exact where the value layer
 *        is.
 *
 * @param[in] _term      the term.
 * @param[in] _bindings  the values of its variables.
 * @param[in] _context   what the fold computes under.
 * @return the term's value: exact where every operation and function on
 *         exact operands is, an error value where arithmetic fails; or the
 *         error unbound_variable; unknown_name for a function or a constant
 *         the standard tables have not, or an operator of no kind the
 *         language has; wrong_arity for an operator or a function applied
 *         to a number of operands it does not take; bad_shape for a vector
 *         or a matrix of no element, or a matrix whose rows are not of as
 *         many elements each; or missing_operand for a term made by
 *         default.
 */
D_NODISCARD inline expr_result<value> expr_evaluate(
    const expr_term&     _term,
    const expr_bindings& _bindings,
    const expr_context&  _context
)
{
    using answer = expr_result<value>;

    static_cast<void>(_context);

    // no term: a term made by default
    if (_term.empty())
    {
        return err<value, expr_error>(internal::expr_no_term());
    }

    return ::djinterp::evaluate<answer, expr_op, expr_atom>(
        _term,
        // a leaf: a number, a constant, or a bound variable
        [&_bindings](const expr_atom& _atom) -> answer
        {
            // by kind
            if (_atom.kind == expr_atom_kind::number)
            {
                return ok<value, expr_error>(_atom.number);
            }

            if (_atom.kind == expr_atom_kind::constant)
            {
                const expr_result<const expr_named_constant*> constant =
                    internal::expr_constant_of(_atom.text);

                // a name of no constant
                if (constant.is_err())
                {
                    return err<value, expr_error>(constant.error());
                }

                return ok<value, expr_error>(constant.value()->as_value);
            }

            const value* bound = _bindings.find(_atom.text);

            // a variable no binding gives a value
            if (bound == D_NULLPTR)
            {
                return err<value, expr_error>(
                    internal::expr_unbound(_atom.text));
            }

            return ok<value, expr_error>(*bound);
        },
        // an application: its operands' first error, or the operation
        [](const expr_op& _op, const std::vector<answer>& _operands) -> answer
        {
            std::vector<value> x;

            // an operand's error passes
            for (std::size_t i = 0; i < _operands.size(); ++i)
            {
                if (_operands[i].is_err())
                {
                    return _operands[i];
                }

                x.push_back(_operands[i].value());
            }

            // a literal: a vector's or a row's blocks beside one another,
            // a matrix's rows one above another (S261)
            if (_op.kind == expr_op_kind::matrix)
            {
                return ok<value, expr_error>(
                    internal::expr_matrix_stacked(x));
            }

            if (internal::expr_is_literal(_op.kind))
            {
                return ok<value, expr_error>(
                    internal::expr_matrix_beside(x));
            }

            // indexing: an element or a row of the matrix
            if (_op.kind == expr_op_kind::index)
            {
                return ( internal::expr_index_takes(x.size())
                             ? ok<value, expr_error>(
                                   internal::expr_matrix_index(x))
                             : err<value, expr_error>(
                                   internal::expr_index_error(x.size())) );
            }

            // a call: its function's evaluator, on the arguments it takes
            if (_op.kind == expr_op_kind::call)
            {
                const expr_result<const expr_function*> function =
                    internal::expr_function_of(_op, x.size());

                if (function.is_err())
                {
                    return err<value, expr_error>(function.error());
                }

                return ok<value, expr_error>(
                    function.value()->evaluate_value(x.data(), x.size()));
            }

            // an operator of the table's, on the operands it takes
            if (internal::expr_takes(_op.kind,
                                     x.size()))
            {
                return ok<value, expr_error>(
                    internal::expr_apply_value(_op.kind, x));
            }

            return err<value, expr_error>(
                internal::expr_fold_error(_op,
                                          x.size()));
        });
}


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_EXPR_EVALUATE_HPP
