/*******************************************************************************
* djinterp [cpp/math]                                                   term.hpp
*
* The term: what a text means, as parse/expression's expression over the run-
* time language's operators and atoms; the third layer.
*   AN ATOM is a number, a variable or a named constant. A number is held as
* a math::value, exact where it is a rational of the family (gate G3), and
* with the literal it was written as, so that a fold can round the literal
* once, from its text, to the type it asks for; a number a fold made has no
* literal. A variable and a named constant are held by name: a variable's
* value comes from the bindings (evaluate.hpp), a constant's from the
* constant table (function.hpp), at the type a fold asks for.
*   THE TERM, expr_term, is expression<expr_op, expr_atom>: a leaf holding an
* atom, or an operator applied to its operands, a call's to its arguments.
* parse/expression's folds, queries and transforms take it, naming its two
* types, as every caller of that module does (chunk 27, S168); this header's
* constructors and observers name them for the caller.
*   STEPS. Step 13 (2a) made this header whole: its types, constructors and
* observers are what every later step builds terms of.
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/term.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  ATOMS
    -----
    1.  The atom
         1.  expr_atom_kind
         2.  expr_atom
2.  TERMS
    -----
    1.  The term
         1.  expr_term
    2.  Constructors
         1.  Leaves
         2.  Applications
    3.  Observers
*/

#ifndef DJINTERP_MATH_EXPR_TERM_HPP
#define DJINTERP_MATH_EXPR_TERM_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <string>  // std::string
#include <vector>  // std::vector
// djinterp
#include "../../parse/expression/expression.hpp"  // expression, expr_leaf,
                                                   // expr_apply
#include "../math_common.hpp"                      // NS_MATH
#include "../value.hpp"                            // value
#include "./operator.hpp"                          // expr_op


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  ATOMS
//==============================================================================


// 1.1    The atom
//------------------------------------------------------------------------------
// 1.1.1
// expr_atom_kind
//   enum: what an atom is.
enum class expr_atom_kind
{
    number   = 1,   // 42, 0.1: a value, and the literal it was written as
    variable = 2,   // x: a name the bindings give a value
    constant = 3    // pi: a name the constant table gives a value
};

// 1.1.2
// expr_atom
//   struct: a leaf of a term: its kind; for a number its value, none for a
// name; and its text: a variable's or a constant's name, or the literal a
// number was written as, empty for a number a fold made. An atom made by
// default is a number with neither value nor text, as parse/expression puts
// in a leaf it makes by default. Two atoms are equal when their kinds and
// texts are, and their numbers are the same (expr_same_number).
struct expr_atom
{
    expr_atom_kind kind;
    value          number;
    std::string    text;

    // a number with neither value nor text.
    expr_atom()
        : kind(expr_atom_kind::number),
          number(),
          text()
    {}

    expr_atom(
        expr_atom_kind     _kind,
        const value&       _number,
        const std::string& _text
    )
        : kind(_kind),
          number(_number),
          text(_text)
    {}
};

NS_INTERNAL

    // expr_same_number
    //   function: whether two values are the same number, for two atoms to be
    // equal: of one kind, and then errors of one code, values equal by
    // value.h's exact order, or two NaNs, which that order leaves unordered.
    // So 1 and 1.0 differ, of two kinds, and 0.0 and -0.0 are the same.
    inline
    bool expr_same_number(
        const value& _a,
        const value& _b
    )
    {
        // two kinds
        if (_a.kind() != _b.kind())
        {
            return false;
        }

        // two errors, by their codes
        if (_a.kind() == D_MATH_VALUE_ERROR)
        {
            dj_math_value_error code_a = D_MATH_VALUE_ERROR_DOMAIN;
            dj_math_value_error code_b = D_MATH_VALUE_ERROR_DOMAIN;

            static_cast<void>(_a.to(code_a));
            static_cast<void>(_b.to(code_b));

            return (code_a == code_b);
        }

        // equal by value.h's order
        if (_a == _b)
        {
            return true;
        }

        // two NaNs, each unordered with itself
        return ( (!(_a == _a)) &&
                 (!(_b == _b)) );
    }

NS_END  // internal

// operator==, operator!= (expr_atom)
//   function: two atoms are equal when their kinds and texts are, and their
// numbers are the same number.
inline
bool operator==(
    const expr_atom& _a,
    const expr_atom& _b
)
{
    return ( (_a.kind == _b.kind) &&
             (_a.text == _b.text) &&
             (internal::expr_same_number(_a.number, _b.number)) );
}

inline
bool operator!=(
    const expr_atom& _a,
    const expr_atom& _b
)
{
    return !(_a == _b);
}


//==============================================================================
// 2.  TERMS
//==============================================================================


// 2.1    The term
//------------------------------------------------------------------------------
// 2.1.1
// expr_term
//   type: a term of the language: parse/expression's expression over its
// operators and atoms. A copy shares the copied term's nodes, which no
// operation changes; every rewrite makes a term of its own. A term made by
// default is empty, as mu's is: it has no root, and no observer may ask it;
// the constructors below make every other.
typedef expression<expr_op, expr_atom> expr_term;

// 2.2    Constructors
//------------------------------------------------------------------------------
// Each makes a term of its own, its nodes allocated as parse/expression's
// are (std::shared_ptr, through mu).

// 2.2.1
// Leaves
//   a number made by a fold, with no literal; a number as it was written;
// a variable; a named constant.
inline
expr_term expr_number(
    const value& _number
)
{
    return expr_leaf<expr_op, expr_atom>(
        expr_atom(expr_atom_kind::number, _number, std::string()));
}

inline
expr_term expr_number(
    const value&       _number,
    const std::string& _literal
)
{
    return expr_leaf<expr_op, expr_atom>(
        expr_atom(expr_atom_kind::number, _number, _literal));
}

inline
expr_term expr_variable(
    const std::string& _name
)
{
    return expr_leaf<expr_op, expr_atom>(
        expr_atom(expr_atom_kind::variable, value(), _name));
}

inline
expr_term expr_constant(
    const std::string& _name
)
{
    return expr_leaf<expr_op, expr_atom>(
        expr_atom(expr_atom_kind::constant, value(), _name));
}

// 2.2.2
// Applications
//   an operator applied to no operand, to one, to two, or to any number of
// them; and a call of a function, by name, on its arguments.
inline
expr_term expr_application(
    const expr_op& _op
)
{
    return expr_apply<expr_op, expr_atom>(_op);
}

inline
expr_term expr_application(
    const expr_op&   _op,
    const expr_term& _operand
)
{
    return expr_apply<expr_op, expr_atom>(_op, _operand);
}

inline
expr_term expr_application(
    const expr_op&   _op,
    const expr_term& _left,
    const expr_term& _right
)
{
    return expr_apply<expr_op, expr_atom>(_op, _left, _right);
}

inline
expr_term expr_application(
    const expr_op&                _op,
    const std::vector<expr_term>& _operands
)
{
    return expr_apply<expr_op, expr_atom>(_op, _operands);
}

inline
expr_term expr_call(
    const std::string&            _function,
    const std::vector<expr_term>& _arguments
)
{
    return expr_apply<expr_op, expr_atom>(expr_op::call(_function),
                                          _arguments);
}

// 2.3    Observers
//------------------------------------------------------------------------------
// What a term's root is; its atom, where it is a leaf; its operator and
// operands, where it is an application. expr_atom_of asks a leaf, and
// expr_op_of and expr_operands_of an application, as parse/expression's
// observers do.
inline
bool expr_is_number(
    const expr_term& _term
)
{
    return ( (is_leaf<expr_op, expr_atom>(_term)) &&
             (atom_of<expr_op, expr_atom>(_term).kind ==
                  expr_atom_kind::number) );
}

inline
bool expr_is_variable(
    const expr_term& _term
)
{
    return ( (is_leaf<expr_op, expr_atom>(_term)) &&
             (atom_of<expr_op, expr_atom>(_term).kind ==
                  expr_atom_kind::variable) );
}

inline
bool expr_is_constant(
    const expr_term& _term
)
{
    return ( (is_leaf<expr_op, expr_atom>(_term)) &&
             (atom_of<expr_op, expr_atom>(_term).kind ==
                  expr_atom_kind::constant) );
}

inline
bool expr_is_application(
    const expr_term& _term
)
{
    return !is_leaf<expr_op, expr_atom>(_term);
}

inline
const expr_atom& expr_atom_of(
    const expr_term& _term
)
{
    return atom_of<expr_op, expr_atom>(_term);
}

inline
const expr_op& expr_op_of(
    const expr_term& _term
)
{
    return operator_of<expr_op, expr_atom>(_term);
}

inline
const std::vector<expr_term>& expr_operands_of(
    const expr_term& _term
)
{
    return children_of<expr_op, expr_atom>(_term);
}


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_EXPR_TERM_HPP
