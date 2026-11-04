/*******************************************************************************
* djinterp [cpp/math]                                             substitute.hpp
*
* Substitution: a term with a variable put in place of by another term; the
* fifth layer's first rewrite.
*   SUBSTITUTION is parse/expression's substitute, one fold over the term:
* each leaf that is the variable becomes the term that replaces it, and every
* other node stays as it was. f.subs("x", t + 1) is roadmap Phase 2's
* example: x ^ 2 * sin(x) becomes (t + 1) ^ 2 * sin(t + 1). Only a variable
* is put in place of: a named constant, pi or true, and a function's name in
* a call are not variables, and stay. The replacement is put in whole, and
* not searched again, so that x by x + 1 is x + 1.
*   A TERM MADE BY DEFAULT has no node, and is no term of the language.
* Substitution in one gives it back; one put in place of a variable the term
* holds makes the result a term made by default, whose folds answer
* missing_operand, and where the term holds no such variable, the term comes
* back as it was.
*   STEPS. Step 18 (2f) filled expr_substitute; until then it gave the term
* unchanged.
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/substitute.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.24
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SUBSTITUTION
    ------------
    1.  What it asks
         1.  expr_holds_variable
    2.  The entry
         1.  expr_substitute
*/

#ifndef DJINTERP_MATH_EXPR_SUBSTITUTE_HPP
#define DJINTERP_MATH_EXPR_SUBSTITUTE_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../../parse/expression/expression_ops.hpp"  // evaluate,
                                                       // substitute
#include "../math_common.hpp"                          // NS_MATH
#include "./term.hpp"                                  // expr_term


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  SUBSTITUTION
//==============================================================================


// 1.1    What it asks
//------------------------------------------------------------------------------
NS_INTERNAL

    // 1.1.1
    // expr_holds_variable
    //   function: whether a leaf of _term is the variable named _variable.
    inline bool
    expr_holds_variable(
        const expr_term&   _term,
        const std::string& _variable
    )
    {
        return ::djinterp::evaluate<bool, expr_op, expr_atom>(
            _term,
            // a leaf: the variable, or not
            [&_variable](const expr_atom& _atom) -> bool
            {
                return ( (_atom.kind == expr_atom_kind::variable) &&
                         (_atom.text == _variable) );
            },
            // an application: one of its operands holds it
            [](const expr_op&           _op,
               const std::vector<bool>& _operands) -> bool
            {
                static_cast<void>(_op);

                // an operand holding it
                for (std::size_t i = 0; i < _operands.size(); ++i)
                {
                    if (_operands[i])
                    {
                        return true;
                    }
                }

                return false;
            });
    }

NS_END  // internal

// 1.2    The entry
//------------------------------------------------------------------------------
// 1.2.1
// expr_substitute
/**
 * @brief A term with a variable replaced by a term.
 *
 * @param[in] _term         the term.
 * @param[in] _variable     the name of the variable replaced.
 * @param[in] _replacement  the term put in its place.
 * @return _term with each leaf that is the variable _variable replaced by
 *         _replacement, every other node as it was; _term itself where it
 *         is a term made by default, or where _replacement is one and
 *         _term holds no such variable; a term made by default where
 *         _replacement is one and _term holds the variable.
 */
D_NODISCARD inline expr_term
expr_substitute(
    const expr_term&   _term,
    const std::string& _variable,
    const expr_term&   _replacement
)
{
    // a term made by default: no node to replace
    if (_term.empty())
    {
        return _term;
    }

    // none put in: no term, where the variable stands
    if (_replacement.empty())
    {
        return ( (internal::expr_holds_variable(_term,
                                                _variable)) ? expr_term()
                                                            : _term );
    }

    return ::djinterp::substitute<expr_op, expr_atom>(
        _term,
        // a leaf: the replacement where it is the variable, else itself
        [&_variable, &_replacement](const expr_atom& _atom) -> expr_term
        {
            // the variable
            if ( (_atom.kind == expr_atom_kind::variable) &&
                 (_atom.text == _variable) )
            {
                return _replacement;
            }

            return expr_leaf<expr_op, expr_atom>(_atom);
        });
}


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_EXPR_SUBSTITUTE_HPP
