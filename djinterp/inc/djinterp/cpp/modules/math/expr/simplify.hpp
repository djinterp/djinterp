/*******************************************************************************
* djinterp [cpp/math]                                               simplify.hpp
*
* Simplification: a term rewritten to a simpler one of the same value; the
* fifth layer's second rewrite.
*   THE RULES, each at a node whose operands are simplified already:
*     constants: an operator or a call whose operands are each an exact
*       number, true or false is its value as a value (evaluate.hpp), where
*       that value is exact, a number of its own, or a boolean, the constant
*       true or false: 1/3 + 1/6 is 1/2, sqrt(4) is 2, 1 < 2 is true. Where
*       it is inexact or an error, as sqrt(2), 25! and 1 / 0 are, the node
*       stays as it is;
*     powers: x ^ 1 is x, and x ^ 0 and 1 ^ x are 1;
*     sums: the terms of a chain of + and -, the unary - and + among them,
*       each an exact coefficient times its other factors, are collected by
*       those factors, in any order: their coefficients summed exactly, the
*       exact numbers among them summed into one, and a term whose
*       coefficient sums to 0 dropped. x + 0 is x, 0 - x is -x, -(-x) is x,
*       2 * x + 3 * x is 5 * x, x * y + y * x is 2 * x * y, x / 2 + x / 2 is
*       x, x + 1 + x is 2 * x + 1, x - x is 0;
*     products: the exact numbers among the factors of a chain of * and /
*       are multiplied into one coefficient, written first: 2 * x * 3 is
*       6 * x, 1 * x and x / 1 are x, and 0 * x is 0.
*   MATRICES do not commute: A * B is not B * A. A factor that holds a vector
* or a matrix, a literal or a matrix a fold made, keeps its place among the
* others that do, so that two terms are alike only where those stand in the
* same order, and A * B + B * A stays as it is; a number, a variable or any
* factor that holds no matrix commutes with every other, so that
* 2 * A * x + x * A is 3 * A * x, in its first term's order (S251). Nor does a
* rule make an error of matrices a value: A ^ 1 is A only where A is square by
* its shape, a literal or a matrix a fold made, a literal of blocks, [A, b],
* never, as its shape only its value tells (S261); and in a sum that holds a
* matrix the terms of coefficient 0 it drops are written as one 0, since a
* matrix plus a number is an error (S246): [1, 2] + 0 stays, and
* x - x + [1, 2] is 0 + [1, 2]. A variable is taken to be a number here, as a
* symbol is in SymPy until it is declared a matrix: bound to matrices,
* x * y + y * x is 2 * x * y all the same, x ^ 0 is 1, and what is kept,
* below, is kept for numbers.
*   QUATERNIONS do not commute either (S277): a factor that holds one, jm, km,
* a call of quat or a quaternion a fold made, keeps its place among the others
* that do, as a matrix does; and so, in a product that holds a quaternion,
* does a factor that holds a complex number, im, a call of complex or a
* complex number a fold made, as im jm is km and jm im -km. So jm * km +
* km * jm and im * jm - jm * im stay as they are, and an exact quaternion,
* or an exact complex number beside one, is a factor in its place, never
* part of the coefficient. A real number commutes with every factor, and so
* does a variable, taken for a real number here as above: 2 * jm * 3 is
* 6 * jm, and x * jm + jm * x is 2 * x * jm, which bound to a number that
* does not commute with jm, as im does not, is another value.
*   A sum or a product is collected once, at the root of its chain, and in the
* first order of its terms and factors. Each rule makes the term it rewrites
* smaller, or as small with fewer terms, as x + x and 2 * x are; so a pass of
* the rules is made, and made again, until one changes nothing, as
* parse/expression's rewrite_to_fixpoint makes its passes; the result is no
* larger than the term; and a term with nothing to simplify comes back as it
* was, x * 2 as x * 2.
*   THE VALUE KEPT. At every binding of its variables at which each part of
* the term has a finite value, a number or a boolean, as in the real
* numbers, the simplified term's value is the same: exactly, where the
* term's is exact, and the same real number where it is not, rounded
* perhaps otherwise, as 2 * x + 3 * x rounds once where it rounded twice.
* In a type, a constant folded is the type's nearest to its exact value,
* which may be far from what the type computes: 10 mod 0.001 is 0, where in
* double it is nearly 0.001. Where a part's value is an error, an infinity
* or a NaN, the simplified term's may differ, as in other computer algebra
* systems: x - x is 0 at any x, and 0 * (1 / x) is 0 at x = 0; pi - pi is
* the exact 0, so that 1 / (pi - pi), an infinity as a value, is a pole
* error after. A part drops only where it is well formed: each call of a
* function the standard table has, with a number of arguments it takes, and
* each operator on a number of operands it takes, so that an unknown name's
* or a wrong arity's error stays, as in 0 * f(x). A literal, or a matrix a
* fold made, is never well formed, as it has no value in a type, so that
* [1, 2] - [1, 2] is 0 * [1, 2], the matrix of zeros, and 0 * A stays: a
* term of matrices drops nothing, and keeps its value as a matrix.
*   STEPS. Step 18 (2f) filled expr_simplify, step 20 (3b) kept the order
* of its matrices, and step 21 (3c) took no literal of blocks for square;
* step 24 (3f) kept the order of its quaternions, and of complex numbers
* beside them (S277); step 25 (4a) adds what the derivative needs.
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/simplify.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.11.07
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  WHAT THE RULES ASK
    ------------------
    1.  Of a leaf
         1.  expr_exact_number, expr_is_exactly
         2.  expr_is_truth
    2.  Of a term
         1.  expr_holds_matrix
         2.  expr_holds_quaternion, expr_holds_complex_number
         3.  expr_square_matrix
         4.  expr_well_formed
2.  THE RULES
    -------------
    1.  Constants
         1.  expr_fold_constants
    2.  Powers
         1.  expr_power_identities
    3.  Sums and products
         1.  expr_factor, expr_monomial
         2.  expr_commutes, expr_factors_of
         3.  expr_summands_of
         4.  expr_same_factors
         5.  expr_monomial_term
         6.  expr_collect_sum
         7.  expr_collect_product
3.  SIMPLIFICATION
    --------------
    1.  A pass
         1.  expr_chain
         2.  expr_simplify_pass
    2.  The entry
         1.  expr_simplify
*/

#ifndef DJINTERP_MATH_EXPR_SIMPLIFY_HPP
#define DJINTERP_MATH_EXPR_SIMPLIFY_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>  // std::size_t
#include <vector>   // std::vector
// djinterp
#include "../../parse/expression/expression_ops.hpp"  // atoms, size,
                                                       // structural_equal
#include "../math_common.hpp"                          // NS_MATH
#include "../value.hpp"                                // value
#include "./evaluate.hpp"                              // expr_evaluate
#include "./expr_common.hpp"                           // expr_result
#include "./matrix.hpp"                                // expr_is_matrix
#include "./quaternion.hpp"                            // expr_is_quaternion
#include "./operator.hpp"                              // expr_op_kind
#include "./term.hpp"                                  // expr_term


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  WHAT THE RULES ASK
//==============================================================================


// 1.1    Of a leaf
//------------------------------------------------------------------------------
NS_INTERNAL

    // 1.1.1
    // expr_exact_number, expr_is_exactly
    //   functions: whether _term is a number of an exact value, an integer or
    // a rational, its value then written to _out; and whether _value is
    // exact and the whole number _whole.
    inline bool
    expr_exact_number(
        const expr_term& _term,
        value&           _out
    )
    {
        // a number, exact
        if ( (!expr_is_number(_term)) ||
             (!expr_atom_of(_term).number.is_exact()) )
        {
            return false;
        }

        _out = expr_atom_of(_term).number;

        return true;
    }

    inline bool
    expr_is_exactly(
        const value& _value,
        dj_math_imax _whole
    )
    {
        dj_math_imax whole = 0;

        return ( (_value.is_exact()) &&
                 (_value.to(whole)) &&
                 (whole == _whole) );
    }

    // 1.1.2
    // expr_is_truth
    //   function: whether _term is the constant true or the constant false.
    inline bool
    expr_is_truth(
        const expr_term& _term
    )
    {
        return ( (expr_is_constant(_term)) &&
                 ( (expr_atom_of(_term).text == "true") ||
                   (expr_atom_of(_term).text == "false") ) );
    }

NS_END  // internal

// 1.2    Of a term
//------------------------------------------------------------------------------
NS_INTERNAL

    // 1.2.1
    // expr_holds_matrix
    //   function: whether _term holds a vector or a matrix: a literal, or a
    // number a fold made that is a matrix (S251).
    inline bool
    expr_holds_matrix(
        const expr_term& _term
    )
    {
        return ::djinterp::evaluate<bool, expr_op, expr_atom>(
            _term,
            // a leaf: a matrix a fold made
            [](const expr_atom& _atom) -> bool
            {
                return ( (_atom.kind == expr_atom_kind::number) &&
                         (expr_is_matrix(_atom.number)) );
            },
            // an application: a literal, or of an operand that holds one
            [](const expr_op&           _op,
               const std::vector<bool>& _operands) -> bool
            {
                bool holds = ( (_op.kind == expr_op_kind::vector) ||
                               (_op.kind == expr_op_kind::matrix) );

                for (std::size_t i = 0; i < _operands.size(); ++i)
                {
                    holds = ( (holds) || (_operands[i]) );
                }

                return holds;
            });
    }

    // 1.2.2
    // expr_holds_quaternion, expr_holds_complex_number
    //   functions: whether _term holds a quaternion: jm or km, a call of
    // quat, or a quaternion a fold made (S277); and whether it holds a
    // complex number: im, a call of complex, or a complex number a fold
    // made, which commutes with every number but a quaternion, im jm being
    // km and jm im -km.
    inline bool
    expr_holds_quaternion(
        const expr_term& _term
    )
    {
        return ::djinterp::evaluate<bool, expr_op, expr_atom>(
            _term,
            // a leaf: jm, km, or a quaternion a fold made
            [](const expr_atom& _atom) -> bool
            {
                return ( ( (_atom.kind == expr_atom_kind::constant) &&
                           ( (_atom.text == "jm") ||
                             (_atom.text == "km") ) ) ||
                         ( (_atom.kind == expr_atom_kind::number) &&
                           (expr_is_quaternion(_atom.number)) ) );
            },
            // an application: a call of quat, or of an operand that holds one
            [](const expr_op&           _op,
               const std::vector<bool>& _operands) -> bool
            {
                bool holds = ( (_op.kind == expr_op_kind::call) &&
                               (_op.function == "quat") );

                for (std::size_t i = 0; i < _operands.size(); ++i)
                {
                    holds = ( (holds) || (_operands[i]) );
                }

                return holds;
            });
    }

    inline bool
    expr_holds_complex_number(
        const expr_term& _term
    )
    {
        return ::djinterp::evaluate<bool, expr_op, expr_atom>(
            _term,
            // a leaf: im, or a complex number a fold made
            [](const expr_atom& _atom) -> bool
            {
                return ( ( (_atom.kind == expr_atom_kind::constant) &&
                           (_atom.text == "im") ) ||
                         ( (_atom.kind == expr_atom_kind::number) &&
                           (expr_is_complex(_atom.number)) ) );
            },
            // an application: a call of complex, or of an operand that holds
            // one
            [](const expr_op&           _op,
               const std::vector<bool>& _operands) -> bool
            {
                bool holds = ( (_op.kind == expr_op_kind::call) &&
                               (_op.function == "complex") );

                for (std::size_t i = 0; i < _operands.size(); ++i)
                {
                    holds = ( (holds) || (_operands[i]) );
                }

                return holds;
            });
    }

    // 1.2.3
    // expr_square_matrix
    //   function: whether _term is a square matrix by its shape alone: a
    // literal of as many rows as each of its rows has elements, the vector
    // [x] among them, no element of which holds a matrix, a block; or a
    // square matrix a fold made. Of a term whose shape only a fold tells, as
    // A * B's, or a literal of blocks (S261), it says nothing: false.
    inline bool
    expr_square_matrix(
        const expr_term& _term
    )
    {
        // a leaf: square where it is a matrix a fold made, of as many rows
        // as columns
        if (!expr_is_application(_term))
        {
            const expr_result<expr_shape> shape =
                expr_shape_of(expr_atom_of(_term).number);

            return ( (shape.is_ok()) &&
                     (shape.value().rows == shape.value().columns) );
        }

        const std::vector<expr_term>& operands = expr_operands_of(_term);
        const expr_op_kind            kind     = expr_op_of(_term).kind;
        const bool                    vector   = (kind ==
                                                      expr_op_kind::vector);
        bool                          square   =
            ( vector ? (operands.size() == 1)
                     : ( (kind == expr_op_kind::matrix) &&
                         (!operands.empty()) ) );

        // each element: no block; each row of a matrix as long as its rows
        // are many
        for (std::size_t i = 0; ( (square) && (i < operands.size()) ); ++i)
        {
            const bool row = ( (kind == expr_op_kind::matrix) &&
                               (expr_is_application(operands[i])) &&
                               (expr_op_of(operands[i]).kind ==
                                    expr_op_kind::row) );
            const std::vector<expr_term> elements =
                ( row ? expr_operands_of(operands[i])
                      : std::vector<expr_term>(1,
                                               operands[i]) );

            square = ( (vector) ||
                       ( (row) &&
                         (elements.size() == operands.size()) ) );

            for (std::size_t j = 0; j < elements.size(); ++j)
            {
                square = ( (square) &&
                           (!expr_holds_matrix(elements[j])) );
            }
        }

        return square;
    }

    // 1.2.4
    // expr_well_formed
    //   function: whether _term's folds can fail only for want of a binding:
    // each call of a function the standard table has, with a number of
    // arguments it takes; each constant the table's; each operator of a kind
    // steps 14 to 20 filled, applied to its number of operands; and no
    // vector or matrix, which has no value in a type. The fold in double
    // fails for nothing else, so that this is whether it answers, each
    // variable of _term bound to 0, and _term holds no matrix.
    inline bool
    expr_well_formed(
        const expr_term& _term
    )
    {
        const std::vector<expr_atom> leaves =
            ::djinterp::atoms<expr_op, expr_atom>(_term);
        expr_bindings                zeros;

        // no matrix a fold made, which a type takes for a NaN
        if (expr_holds_matrix(_term))
        {
            return false;
        }

        // each variable bound
        for (std::size_t i = 0; i < leaves.size(); ++i)
        {
            if (leaves[i].kind == expr_atom_kind::variable)
            {
                zeros.at(leaves[i].text,
                         value(0));
            }
        }

        return expr_evaluate<double>(_term,
                                     zeros).is_ok();
    }

NS_END  // internal


//==============================================================================
// 2.  THE RULES
//==============================================================================
// Each takes a node whose operands are simplified, and gives the node it
// rewrites it to, or the node itself where the rule does not apply.


// 2.1    Constants
//------------------------------------------------------------------------------
NS_INTERNAL

    // 2.1.1
    // expr_fold_constants
    //   function: _term, an application, whose operands are each an exact
    // number, true or false, as its value as a value where that value is
    // exact, a number of its own, or a boolean, the constant true or false;
    // otherwise _term, as where its value is inexact or an error, or its
    // fold fails.
    inline expr_term
    expr_fold_constants(
        const expr_term& _term
    )
    {
        const std::vector<expr_term>& operands = expr_operands_of(_term);

        // each operand an exact number, true or false
        for (std::size_t i = 0; i < operands.size(); ++i)
        {
            value number;

            if ( (!expr_exact_number(operands[i], number)) &&
                 (!expr_is_truth(operands[i])) )
            {
                return _term;
            }
        }

        const expr_result<value> folded = expr_evaluate(_term,
                                                        expr_bindings(),
                                                        expr_context());
        bool                     truth  = false;

        // no value
        if (folded.is_err())
        {
            return _term;
        }

        // a boolean: its constant
        if (folded.value().to(truth))
        {
            return expr_constant( truth ? "true"
                                        : "false" );
        }

        return ( (folded.value().is_exact()) ? expr_number(folded.value())
                                             : _term );
    }

NS_END  // internal

// 2.2    Powers
//------------------------------------------------------------------------------
NS_INTERNAL

    // 2.2.1
    // expr_power_identities
    //   function: _term, a power, by the identities x ^ 1 = x, where x holds
    // no matrix or is a square one, as A ^ 1 is an error of any other
    // matrix (S248); and, where x is well formed, x ^ 0 = 1 and 1 ^ x = 1;
    // otherwise _term.
    inline expr_term
    expr_power_identities(
        const expr_term& _term
    )
    {
        // a power, of two operands
        if ( (!expr_is_application(_term)) ||
             (expr_op_of(_term).kind != expr_op_kind::pow) ||
             (expr_operands_of(_term).size() != 2) )
        {
            return _term;
        }

        const expr_term base     = expr_operands_of(_term)[0];
        const expr_term exponent = expr_operands_of(_term)[1];
        value           number;

        // x ^ 1, of no matrix or a square one
        if ( (expr_exact_number(exponent, number)) &&
             (expr_is_exactly(number, 1)) &&
             ( (!expr_holds_matrix(base)) ||
               (expr_square_matrix(base)) ) )
        {
            return base;
        }

        // x ^ 0
        if ( (expr_exact_number(exponent, number)) &&
             (expr_is_exactly(number, 0)) &&
             (expr_well_formed(base)) )
        {
            return expr_number(value(1));
        }

        // 1 ^ x
        if ( (expr_exact_number(base, number)) &&
             (expr_is_exactly(number, 1)) &&
             (expr_well_formed(exponent)) )
        {
            return expr_number(value(1));
        }

        return _term;
    }

NS_END  // internal

// 2.3    Sums and products
//------------------------------------------------------------------------------
NS_INTERNAL

    // 2.3.1
    // expr_factor, expr_monomial
    //   structs: a factor of a product, and whether it divides the product;
    // and a term of a sum, or a product, as its coefficient, an exact
    // number, times its other factors, in their order.
    struct expr_factor
    {
        expr_term term;
        bool      divides;

        expr_factor(
            const expr_term& _term,
            bool             _divides
        )
            : term(_term),
              divides(_divides)
        {}
    };

    struct expr_monomial
    {
        value                    coefficient;
        std::vector<expr_factor> factors;

        // _coefficient times no factor.
        explicit expr_monomial(
            const value& _coefficient
        )
            : coefficient(_coefficient),
              factors()
        {}
    };

    // 2.3.2
    // expr_factors_of
    //   function: the factors of _term, as a factor of a product, dividing
    // it where _divides, added to _monomial: through * and / and the unary -
    // and +, each exact number that commutes with the product's factors
    // multiplying the coefficient, or dividing it where it is not 0, each -
    // negating it, and each other term a factor. An exact quaternion never
    // commutes, nor an exact complex number in a product that holds a
    // quaternion, _quaternion (S277): each is a factor, in its place. False
    // where the coefficient is no longer exact, as where it passes the
    // family's integers.

    // expr_commutes
    //   function: whether the exact number _number commutes with every
    // factor of a product, which holds a quaternion where _quaternion: a
    // real number always, a complex one where no quaternion stands beside
    // it, a quaternion never.
    inline bool
    expr_commutes(
        const value& _number,
        bool         _quaternion
    )
    {
        return ( (!expr_is_quaternion(_number)) &&
                 ( (!_quaternion) ||
                   (!expr_is_complex(_number)) ) );
    }

    inline bool
    expr_factors_of(
        const expr_term& _term,
        bool             _divides,
        bool             _quaternion,
        expr_monomial&   _monomial
    )
    {
        value number;

        // an exact number that commutes: into the coefficient, but 0
        // dividing
        if ( (expr_exact_number(_term, number)) &&
             (expr_commutes(number, _quaternion)) )
        {
            if (!_divides)
            {
                _monomial.coefficient = _monomial.coefficient * number;
            }
            else if (!expr_is_exactly(number, 0))
            {
                _monomial.coefficient = _monomial.coefficient / number;
            }
            else
            {
                _monomial.factors.push_back(expr_factor(_term,
                                                        true));
            }

            return _monomial.coefficient.is_exact();
        }

        // an operator of the chain, of its operands
        if (expr_is_application(_term))
        {
            const expr_op_kind            kind     = expr_op_of(_term).kind;
            const std::vector<expr_term>& operands = expr_operands_of(_term);

            if ( ( (kind == expr_op_kind::mul) ||
                   (kind == expr_op_kind::div) ) &&
                 (operands.size() == 2) )
            {
                return ( (expr_factors_of(operands[0],
                                          _divides,
                                          _quaternion,
                                          _monomial)) &&
                         (expr_factors_of(operands[1],
                                          ( (kind == expr_op_kind::div)
                                                ? !_divides
                                                : _divides ),
                                          _quaternion,
                                          _monomial)) );
            }

            if ( ( (kind == expr_op_kind::neg) ||
                   (kind == expr_op_kind::pos) ) &&
                 (operands.size() == 1) )
            {
                if (kind == expr_op_kind::neg)
                {
                    _monomial.coefficient = -_monomial.coefficient;
                }

                return expr_factors_of(operands[0],
                                       _divides,
                                       _quaternion,
                                       _monomial);
            }
        }

        _monomial.factors.push_back(expr_factor(_term,
                                                _divides));

        return true;
    }

    // 2.3.3
    // expr_summands_of
    //   function: the terms of _term, as a term of a sum, negated where
    // _negative, added to _summands, each a monomial: through + and - and
    // the unary - and +. False where a coefficient is no longer exact.
    inline bool
    expr_summands_of(
        const expr_term&            _term,
        bool                        _negative,
        std::vector<expr_monomial>& _summands
    )
    {
        // an operator of the chain, of its operands
        if (expr_is_application(_term))
        {
            const expr_op_kind            kind     = expr_op_of(_term).kind;
            const std::vector<expr_term>& operands = expr_operands_of(_term);

            if ( ( (kind == expr_op_kind::add) ||
                   (kind == expr_op_kind::sub) ) &&
                 (operands.size() == 2) )
            {
                return ( (expr_summands_of(operands[0],
                                           _negative,
                                           _summands)) &&
                         (expr_summands_of(operands[1],
                                           ( (kind == expr_op_kind::sub)
                                                 ? !_negative
                                                 : _negative ),
                                           _summands)) );
            }

            if ( ( (kind == expr_op_kind::neg) ||
                   (kind == expr_op_kind::pos) ) &&
                 (operands.size() == 1) )
            {
                return expr_summands_of(operands[0],
                                        ( (kind == expr_op_kind::neg)
                                              ? !_negative
                                              : _negative ),
                                        _summands);
            }
        }

        expr_monomial summand(value( _negative ? -1
                                               : 1 ));

        // a term: its coefficient and its factors
        if (!expr_factors_of(_term,
                             false,
                             expr_holds_quaternion(_term),
                             summand))
        {
            return false;
        }

        _summands.push_back(summand);

        return true;
    }

    // 2.3.4
    // expr_same_factors
    //   function: whether _a and _b are the same factors: as many, each the
    // same as one of the other's, by parse/expression's structural equality,
    // and dividing as it does; those that keep their places
    // (expr_keeps_places) in the same order, as matrices and quaternions do
    // not commute, and the others in any order (S251, S277).
    inline bool
    expr_same_factor(
        const expr_factor& _a,
        const expr_factor& _b
    )
    {
        return ( (_a.divides == _b.divides) &&
                 (::djinterp::structural_equal<expr_op, expr_atom>(
                      _a.term,
                      _b.term)) );
    }

    // expr_keeps_places
    //   function: for each of _factors whether it keeps its place among
    // them: one that holds a matrix (S251) or a quaternion, and, among
    // factors one of which holds a quaternion, one that holds a complex
    // number, as im jm is km and jm im -km (S277); the others commute with
    // every factor, a variable among them, taken for a number until step 25
    // (4a) declares its domain (S251, S274).
    inline std::vector<bool>
    expr_keeps_places(
        const std::vector<expr_factor>& _factors
    )
    {
        std::vector<bool> keeps(_factors.size(),
                                false);
        bool              quaternion = false;

        // a matrix or a quaternion keeps its place
        for (std::size_t i = 0; i < _factors.size(); ++i)
        {
            const bool held = expr_holds_quaternion(_factors[i].term);

            quaternion = ( (quaternion) || (held) );
            keeps[i]   = ( (held) ||
                           (expr_holds_matrix(_factors[i].term)) );
        }

        // beside a quaternion, a complex number keeps its place too
        for (std::size_t i = 0; ( (quaternion) && (i < _factors.size()) );
             ++i)
        {
            keeps[i] = ( (keeps[i]) ||
                         (expr_holds_complex_number(_factors[i].term)) );
        }

        return keeps;
    }

    inline bool
    expr_same_factors(
        const std::vector<expr_factor>& _a,
        const std::vector<expr_factor>& _b
    )
    {
        // as many
        if (_a.size() != _b.size())
        {
            return false;
        }

        const std::vector<bool> matrix_a = expr_keeps_places(_a);
        const std::vector<bool> matrix_b = expr_keeps_places(_b);
        std::vector<bool>       taken(_b.size(),
                                      false);
        std::size_t             j = 0;

        // the factors that keep their places: the same, in the same order
        for (std::size_t i = 0; i < _a.size(); ++i)
        {
            if (!matrix_a[i])
            {
                continue;
            }

            while ( (j < _b.size()) &&
                    (!matrix_b[j]) )
            {
                ++j;
            }

            if ( (j == _b.size()) ||
                 (!expr_same_factor(_a[i],
                                    _b[j])) )
            {
                return false;
            }

            taken[j] = true;
            ++j;
        }

        // each other factor of _a's matched by one of _b's not taken yet,
        // which holds no matrix: of as many factors, _b then has no more of
        // those that do
        for (std::size_t i = 0; i < _a.size(); ++i)
        {
            bool matched = matrix_a[i];

            for (std::size_t k = 0; ( (k < _b.size()) && (!matched) ); ++k)
            {
                if ( (!taken[k]) &&
                     (!matrix_b[k]) &&
                     (expr_same_factor(_a[i],
                                       _b[k])) )
                {
                    taken[k] = true;
                    matched  = true;
                }
            }

            if (!matched)
            {
                return false;
            }
        }

        return true;
    }

    // expr_well_formed_factors
    //   function: whether each of _factors is well formed.
    inline bool
    expr_well_formed_factors(
        const std::vector<expr_factor>& _factors
    )
    {
        // each
        for (std::size_t i = 0; i < _factors.size(); ++i)
        {
            if (!expr_well_formed(_factors[i].term))
            {
                return false;
            }
        }

        return true;
    }

    // 2.3.5
    // expr_monomial_term
    //   function: _coefficient times _factors, as a term: the coefficient
    // first where it is neither 1 nor -1, then each factor in its order,
    // multiplying or dividing; a minus before them where it is -1, and 1
    // over the first where it divides; with no factor, the coefficient.
    inline expr_term
    expr_monomial_term(
        const value&                    _coefficient,
        const std::vector<expr_factor>& _factors
    )
    {
        const bool one       = expr_is_exactly(_coefficient, 1);
        const bool minus_one = expr_is_exactly(_coefficient, -1);
        expr_term  product;
        bool       begun     = false;

        // no factor: the coefficient
        if (_factors.empty())
        {
            return expr_number(_coefficient);
        }

        // the coefficient, where it is written
        if ( (!one) &&
             (!minus_one) )
        {
            product = expr_number(_coefficient);
            begun   = true;
        }

        // each factor, in its order
        for (std::size_t i = 0; i < _factors.size(); ++i)
        {
            const expr_op_kind kind = ( (_factors[i].divides)
                                            ? expr_op_kind::div
                                            : expr_op_kind::mul );

            if (begun)
            {
                product = expr_application(kind,
                                           product,
                                           _factors[i].term);
            }
            else
            {
                product = ( (_factors[i].divides)
                                ? expr_application(kind,
                                                   expr_number(value(1)),
                                                   _factors[i].term)
                                : _factors[i].term );
                begun   = true;
            }
        }

        return ( (minus_one) ? expr_application(expr_op_kind::neg,
                                                product)
                             : product );
    }

    // 2.3.6
    // expr_collect_sum
    //   function: _term, the root of a chain of + and -, with its terms
    // collected: grouped by their factors, in the order each group's first
    // stands, each group's coefficients summed; a group of coefficient 0
    // dropped, where its factors are well formed; and each group after the
    // first subtracted where its coefficient is negative. In a sum that
    // holds a matrix, the groups of coefficient 0 it drops are written as
    // one 0, where the first stood, since a matrix plus a number is an error
    // (S246): x - x + A is 0 + A, and A + 0 stays. The collected sum
    // where it is smaller than _term, or as small with fewer terms; _term
    // otherwise, or where a coefficient is no longer exact.
    inline expr_term
    expr_collect_sum(
        const expr_term& _term
    )
    {
        std::vector<expr_monomial> summands;
        std::vector<expr_monomial> groups;

        // the terms, each coefficient exact
        if (!expr_summands_of(_term,
                              false,
                              summands))
        {
            return _term;
        }

        // grouped by their factors
        for (std::size_t i = 0; i < summands.size(); ++i)
        {
            bool joined = false;

            for (std::size_t j = 0; ( (j < groups.size()) && (!joined) ); ++j)
            {
                if (expr_same_factors(groups[j].factors,
                                      summands[i].factors))
                {
                    groups[j].coefficient = ( groups[j].coefficient +
                                              summands[i].coefficient );
                    joined                = true;
                }
            }

            if (!joined)
            {
                groups.push_back(summands[i]);
            }
        }

        const value zero(0);
        expr_term   sum;
        bool        begun    = false;
        bool        matrices = false;
        bool        zeroed   = false;

        // whether a group holds a matrix
        for (std::size_t i = 0; i < groups.size(); ++i)
        {
            for (std::size_t j = 0; j < groups[i].factors.size(); ++j)
            {
                matrices = ( (matrices) ||
                             (expr_holds_matrix(groups[i].factors[j].term)) );
            }
        }

        // the sum of the groups, in their order
        for (std::size_t i = 0; i < groups.size(); ++i)
        {
            const expr_monomial& group = groups[i];
            expr_term            term;

            // a coefficient summed past exact
            if (!group.coefficient.is_exact())
            {
                return _term;
            }

            // a group of coefficient 0, well formed: dropped, or beside a
            // matrix the first such written as 0
            if ( (expr_is_exactly(group.coefficient, 0)) &&
                 (expr_well_formed_factors(group.factors)) )
            {
                if ( (!matrices) ||
                     (zeroed) )
                {
                    continue;
                }

                term   = expr_number(zero);
                zeroed = true;
            }
            else if ( (begun) &&
                      (group.coefficient < zero) )
            {
                term = expr_monomial_term(-group.coefficient,
                                          group.factors);
            }
            else
            {
                term = expr_monomial_term(group.coefficient,
                                          group.factors);
            }

            if (!begun)
            {
                sum   = term;
                begun = true;
            }
            else if (group.coefficient < zero)
            {
                sum = expr_application(expr_op_kind::sub,
                                       sum,
                                       term);
            }
            else
            {
                sum = expr_application(expr_op_kind::add,
                                       sum,
                                       term);
            }
        }

        // every group dropped
        if (!begun)
        {
            sum = expr_number(zero);
        }

        const std::size_t before = ::djinterp::size<expr_op, expr_atom>(_term);
        const std::size_t after  = ::djinterp::size<expr_op, expr_atom>(sum);

        return ( ( (after < before) ||
                   ( (after == before) &&
                     (groups.size() < summands.size()) ) ) ? sum
                                                           : _term );
    }

    // 2.3.7
    // expr_collect_product
    //   function: _term, the root of a chain of * and /, with the exact
    // numbers among its factors multiplied into one coefficient, written
    // first, and a coefficient of 0 the product 0, where the other factors
    // are well formed. The collected product where it is smaller than
    // _term; _term otherwise, or where the coefficient is no longer exact.
    inline expr_term
    expr_collect_product(
        const expr_term& _term
    )
    {
        expr_monomial product(value(1));
        expr_term     collected;

        // the factors, the coefficient exact
        if (!expr_factors_of(_term,
                             false,
                             expr_holds_quaternion(_term),
                             product))
        {
            return _term;
        }

        // 0, the other factors well formed
        if (expr_is_exactly(product.coefficient, 0))
        {
            if (!expr_well_formed_factors(product.factors))
            {
                return _term;
            }

            collected = expr_number(value(0));
        }
        else
        {
            collected = expr_monomial_term(product.coefficient,
                                           product.factors);
        }

        return ( (::djinterp::size<expr_op, expr_atom>(collected) <
                  ::djinterp::size<expr_op, expr_atom>(_term)) ? collected
                                                               : _term );
    }

NS_END  // internal


//==============================================================================
// 3.  SIMPLIFICATION
//==============================================================================


// 3.1    A pass
//------------------------------------------------------------------------------
NS_INTERNAL

    // 3.1.1
    // expr_chain
    //   enum: the chain a node's operator is in: a sum's, of + and - and the
    // unary - and +; a product's, of * and /; or none.
    enum class expr_chain
    {
        none    = 0,
        sum     = 1,
        product = 2
    };

    // expr_chain_of
    //   function: the chain of an operator of _kind.
    inline expr_chain
    expr_chain_of(
        expr_op_kind _kind
    )
    {
        // by kind
        switch (_kind)
        {
            case expr_op_kind::add:
            case expr_op_kind::sub:
            case expr_op_kind::neg:
            case expr_op_kind::pos:
                return expr_chain::sum;

            case expr_op_kind::mul:
            case expr_op_kind::div:
                return expr_chain::product;

            default:
                return expr_chain::none;
        }
    }

    // expr_simplify_passes
    //   constant: the most passes expr_simplify makes, as many as
    // rewrite_to_fixpoint's; each pass that changes the term makes it
    // smaller, or as small with fewer sums, so that a term reaches its fixed
    // point long before.
    D_CONSTEXPR_VAR std::size_t expr_simplify_passes = 1024;

    // 3.1.2
    // expr_simplify_pass
    //   function: one pass of the rules over _term, bottom up: its operands
    // first, then at its node the constants folded and the powers'
    // identities, and, where the node is the root of a chain, its parent's
    // chain, _parent, another, the chain collected.
    inline expr_term
    expr_simplify_pass(
        const expr_term& _term,
        expr_chain       _parent
    )
    {
        // a leaf
        if (!expr_is_application(_term))
        {
            return _term;
        }

        const expr_op                 op       = expr_op_of(_term);
        const std::vector<expr_term>& operands = expr_operands_of(_term);
        std::vector<expr_term>        simplified;

        simplified.reserve(operands.size());

        // the operands, each in this node's chain
        for (std::size_t i = 0; i < operands.size(); ++i)
        {
            simplified.push_back(expr_simplify_pass(operands[i],
                                                    expr_chain_of(op.kind)));
        }

        const expr_term node = expr_power_identities(
                                   expr_fold_constants(
                                       expr_application(op,
                                                        simplified)));

        // a leaf now, or no chain's root
        if (!expr_is_application(node))
        {
            return node;
        }

        const expr_chain chain = expr_chain_of(expr_op_of(node).kind);

        if ( (chain == expr_chain::none) ||
             (chain == _parent) )
        {
            return node;
        }

        return ( (chain == expr_chain::sum) ? expr_collect_sum(node)
                                            : expr_collect_product(node) );
    }

NS_END  // internal

// 3.2    The entry
//------------------------------------------------------------------------------
// 3.2.1
// expr_simplify
/**
 * @brief A term rewritten to a simpler one with the same value.
 *
 * @param[in] _term  the term.
 * @return _term with its constants folded, its powers' identities applied,
 *         and its sums' terms and its products' factors collected, until
 *         none of these changes it (the banner's rules); no larger than
 *         _term, and _term itself where nothing simplifies, or where it is a
 *         term made by default. Its value is _term's at every binding at
 *         which each part of _term has a finite value: the same exact
 *         number where that is exact, else the same real number.
 */
D_NODISCARD inline expr_term
expr_simplify(
    const expr_term& _term
)
{
    // a term made by default: nothing to rewrite
    if (_term.empty())
    {
        return _term;
    }

    expr_term   current = _term;
    expr_term   next    = internal::expr_simplify_pass(
                              current,
                              internal::expr_chain::none);
    std::size_t passes  = 1;

    // pass after pass, until one changes nothing
    while ( (!::djinterp::structural_equal<expr_op, expr_atom>(current,
                                                                next)) &&
            (passes < internal::expr_simplify_passes) )
    {
        current = next;
        next    = internal::expr_simplify_pass(current,
                                               internal::expr_chain::none);
        ++passes;
    }

    return next;
}


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_EXPR_SIMPLIFY_HPP
