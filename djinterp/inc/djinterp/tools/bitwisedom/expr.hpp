/*******************************************************************************
* bitwisedom [bitwisedom]                                               expr.hpp
*
* Integer expressions over symbols, as a C macro body spells them.
*   A symbol defined by an expression -- `READ | WRITE`, `~MODE & 0xFF`,
* `GENMASK(7, 4)` -- keeps its form all the way to the generated header,
* where it becomes `#define D_RW (D_READ | D_WRITE)`. So an expression is
* parsed once into a tree that can be evaluated against the sheet and
* printed back as C with every name replaced by its full macro name.
*   THE LANGUAGE is C's integer operators at C's precedence -- unary `~ - + !`,
* then `* / %`, `+ -`, `<< >>`, `&`, `^`, `|` -- over integer literals in
* every base C has (bits.hpp's parse_integer reads them), names, parentheses,
* and three functions: BIT(n), MASK(w) and GENMASK(high, low).
*   ARITHMETIC is unsigned and 64 bits wide, and wraps. A shift by 64 or more
* gives zero rather than C's undefined behaviour, and is reported; a division
* by zero is an error.
*   A SHEET WIDER THAN AN INTEGER evaluates its expressions over sets of bits
* instead (evaluate_wide): the bitwise operators, addition, subtraction and
* negation within the sheet's width, a literal of any width, BIT(n) past
* bit 63; a product or a shift needs one integer side, and a quotient two.
*   NOTHING HERE THROWS: a malformed expression is a parse result with an
* error and the position it was found at.
*
*
* path:      /inc/djinterp/tools/bitwisedom/expr.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.11
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE TREE
    --------
    1.  expr_op
    2.  expr_node
    3.  expression
2.  OPERATIONS
    ----------
    1.  Parsing
    2.  Evaluation
    3.  Printing
*/

#ifndef BITWISEDOM_EXPR_HPP
#define BITWISEDOM_EXPR_HPP 1

// FLOOR: below C++17 this file is empty, rather than an error.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>      // std::size_t
#include <functional>   // std::function
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <vector>       // std::vector
// djinterp
#include "djinterp/djinterp.hpp"  // framework root
// bitwisedom
#include "./bits.hpp"  // u64


namespace bitwisedom
{


//==============================================================================
// 1.  THE TREE
//==============================================================================


// 1.1    expr_op
//------------------------------------------------------------------------------
// 1.1.1
// expr_op
//   enum: what one node of an expression does.
enum class expr_op : int
{
    literal,
    name,
    call,
    negate,
    complement,
    logical_not,
    multiply,
    divide,
    modulo,
    add,
    subtract,
    shift_left,
    shift_right,
    bit_and,
    bit_xor,
    bit_or
};

// 1.2    expr_node
//------------------------------------------------------------------------------
// 1.2.1
// expr_node
//   struct: one node. `text` is a literal's spelling or a name; `args` are
// indices of the operands within the expression's node list.
struct expr_node
{
    expr_op          op    = expr_op::literal;
    u64              value = 0;
    bool             big   = false;   // a literal wider than 64 bits
    std::string      text;
    std::vector<int> args;
};

// 1.3    expression
//------------------------------------------------------------------------------
// 1.3.1
// expression
//   struct: a parsed expression -- its nodes, the root's index, and the
// error that stopped the parse, if one did (then `root` is -1).
struct expression
{
    std::vector<expr_node> nodes;
    int                    root = -1;
    std::string            error;
    std::size_t            error_at = 0;

    D_NODISCARD bool ok() const
    {
        return (root >= 0);
    }
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Parsing
//------------------------------------------------------------------------------
/**
 * @brief Parses an expression.
 *
 * @param[in] _text  the expression's text.
 * @return the tree, or `ok() == false` with the error and its position. An
 *         empty text is an error: there is nothing to evaluate.
 */
D_NODISCARD expression parse_expression(std::string_view _text);

// names -- every name an expression refers to, in order of appearance
D_NODISCARD std::vector<std::string> referenced_names(
    const expression& _expression);

// 2.2    Evaluation
//------------------------------------------------------------------------------
// 2.2.1
// name_lookup
//   type: resolves a name to its value; returns false and sets the message
// when it cannot.
using name_lookup = std::function<bool(const std::string& _name,
                                       u64&               _value,
                                       std::string&       _error)>;

// 2.2.2
// evaluation
//   struct: what came of evaluating an expression.
struct evaluation
{
    bool        ok        = false;
    u64         value     = 0;
    bool        overshift = false;
    std::string error;
};

/**
 * @brief Evaluates a parsed expression.
 *
 * @param[in] _expression  a parsed expression; one that failed to parse
 *                         evaluates to its parse error.
 * @param[in] _lookup      resolves each name the expression uses.
 * @return the 64-bit value, or `ok == false` and why: an unknown name, a
 *         division by zero, or the parse error. `overshift` reports a shift
 *         by 64 or more, which gave zero.
 */
D_NODISCARD evaluation evaluate(const expression&  _expression,
                                const name_lookup& _lookup);

// 2.2.3
// wide_lookup / wide_evaluation
//   the lookup and the result of an evaluation over sets of bits.
using wide_lookup = std::function<bool(const std::string& _name,
                                       wide&              _value,
                                       std::string&       _error)>;

struct wide_evaluation
{
    bool        ok        = false;
    wide        value;
    bool        overshift = false;
    std::string error;
};

/**
 * @brief Evaluates a parsed expression over sets of bits, within a width.
 *
 * @param[in] _expression  the tree.
 * @param[in] _lookup      resolves names to their bits.
 * @param[in] _width       the sheet's width: complements, negations and
 *                         sums are taken within it, and the result is cut
 *                         to it.
 * @return the bits, or an error: an unknown name, a product of two values
 *         wider than an integer, a quotient of any, BIT or GENMASK past
 *         the width. `overshift` reports a shift past the width.
 */
D_NODISCARD wide_evaluation evaluate_wide(const expression&  _expression,
                                          const wide_lookup& _lookup,
                                          int                _width);

// 2.3    Printing
//------------------------------------------------------------------------------
// 2.3.1
// literal_printer
//   type: prints a literal the way the generated header wants it.
using literal_printer = std::function<std::string(u64 _value)>;

// 2.3.2
// name_printer
//   type: prints a name: a symbol's local name becomes its macro name.
using name_printer = std::function<std::string(const std::string& _name)>;

/**
 * @brief Prints an expression back as C.
 *
 * @note Every binary operation is parenthesized, so the result means the
 *       same wherever a macro is expanded. A function call prints as the
 *       literal it evaluates to; a shift's count prints as written.
 *
 * @param[in] _expression  a parsed expression.
 * @param[in] _names       prints each name.
 * @param[in] _literals    prints each literal and each folded call.
 * @param[in] _lookup      resolves names inside a call, to fold it.
 * @return the C text, or the empty string for an expression that did not
 *         parse.
 */
D_NODISCARD std::string print_c(const expression&      _expression,
                                const name_printer&    _names,
                                const literal_printer& _literals,
                                const name_lookup&     _lookup);


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // BITWISEDOM_EXPR_HPP
