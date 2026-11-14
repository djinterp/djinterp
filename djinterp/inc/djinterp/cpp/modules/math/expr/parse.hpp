/*******************************************************************************
* djinterp [cpp/math]                                                  parse.hpp
*
* The parser layer: math text to a term, by parse/expression's precedence
* climbing over the operator table; the second half of the second layer.
*   WHAT IT ADDS to the shared parser (chunk 12's probe and chunk 27's
* findings, roadmap Phase 2): calls with argument lists, f(a, b); vector and
* matrix literals, [1, 2, 3] and [1, 2; 3, 4]; indexing, v[i] and A[i, j]; a
* unary minus told from a binary one by where it stands; and gate G5's
* options. A name before an open round bracket is a call, a name the standard
* constant table has is a constant, and any other name is a variable.
*   HOW IT PARSES. The tokens are read once before the shared parser sees
* them, each given its role: a number an atom, its value read exactly by
* literal.h (0.1 is 1/10, gate G3) and its text kept as its literal; a name a
* call's function, a constant, a variable, or one of the operators mod, and,
* or and not; + and - unary at the start, after an open bracket, a comma and
* an operator, and binary after an operand; * / ^ their operators, ! the
* factorial, ' the transpose, and = != < <= > >= the relations, the longer
* symbol first, so that 3!=6 is 3 != 6. With G5's implied multiplication on,
* a * stands between an operand's end and the next one's start, as tight as
* any *: 2x, 2(x + 1), (a)(b), x y, 2 pi, 2 sin(x); never between two numbers,
* never before a call's bracket, and never at a word, which is an operator.
* With G5's bars on, a | closes where an operand ends before it and a bar is
* the innermost group open, and opens anywhere else.
*   Then the brackets and the bars are matched, each closing the innermost
* open of its own kind. Then each group, a call, a pair of bars, a pair of
* round brackets, a literal or an index, is read as one (S215): its arguments,
* split at a call's commas, or its elements, at a literal's commas and
* semicolons, or its indices, at an index's commas, are parsed in turn, and
* the group stands in the climb as an atom, or an index as a postfix operator
* on the operand before it, put back as its term once the climb is done; |x|
* is the call abs(x), (a + b) the term a + b, [a, b, c] the term vector of its
* elements, [a, b; c, d] the term matrix of its rows, each the term row of its
* elements, and v[i] the term index(v, i). A group's work is done in functions
* of its own, outside the frame of the function that recurses into nested
* groups, so that each level of nesting costs that frame alone (S250). The
* climb is parse/expression's precedence climbing over the operator table
* (operator.hpp), so that sin(x) ^ 2 is (sin(x)) ^ 2, 2 ^ 3! is 2 ^ 6, and
* x > 0 and x < 1 is (x > 0) and (x < 1). A call names its function and is
* parsed whatever the name: the folds look it up (evaluate.hpp).
*   A CHAIN of relations, a < b <= c, is the conjunction of its links,
* a < b and b <= c, as in mathematics and in Python, each operand between
* two links in both (S214): a = b = c is a = b and b = c, and a < b > c is
* a < b and b > c. The relations are non-associative, so the climb groups a
* chain to the right; then its links are joined by and, left to right. A
* bracketed relation is one operand, and no link: (a < b) = c compares the
* boolean a < b with c.
*   PIECEWISE is read in the vocabulary of math/function/function.hpp:
* piecewise(when(c1, v1), when(c2, v2), otherwise(d)) is the call
* piecewise(c1, v1, c2, v2, d), each branch's arguments put in its place
* (S216), which render.hpp writes back in its branches. A when or an
* otherwise stands only as a whole argument of piecewise, and an otherwise
* only as its last.
*   A LITERAL stands where an operand starts: a [ after an operand's end is
* indexing, and never an implied product (S243). Its elements are any terms,
* [x, 2 * y], each a block of the literal's value, a number or a matrix,
* [A, b] and [A; B] among them (S261), and [] is the empty literal, the term
* vector of no element (S260). A matrix literal's rows of numbers alone have
* as many elements each, its first row's number; rows that may hold a
* matrix, of a variable, a constant or a literal, have their shapes only as
* values.
*   INDEXING, v[i] and A[i, j], is a [ after an operand's end, with one
* index or two, each any term, counted from 1 when it folds (evaluate.hpp,
* S249). It binds tighter than any operator, so that -v[1] is -(v[1]),
* 2 ^ v[1] is 2 ^ (v[1]), A[1][2] indexes the row A[1], and x'[1] is
* (x')[1]; f(x)[1] and (a + b)[1] index a call and a group (S252).
*   THE TRANSPOSE, A', is postfix, below ^ and above the unary minus
* (operator.hpp): A' ^ 2 is (A') ^ 2 and A ^ 2' is (A ^ 2)', as MATLAB
* reads both, -A' is -(A'), A'' is (A')', and x!' is (x!)' (S252).
*   ERRORS are expr_errors, each at its place in the text: the offset of the
* token where the parse went wrong, mapped back to its character, as chunk 27
* found a language must (the shared parser counts tokens). The first error the
* parse meets is the one given: a token no role fits, then a bracket or a bar,
* then the grammar, where of two errors the one further left is given. A
* bracket closed but never opened, or one never closed, the innermost, is
* unbalanced, as is a bar never closed, and a bracket closing where the
* innermost open is of another kind; the text ending, a bracket closing, a
* comma or a semicolon, where an operand must stand, a missing operand, an
* empty element [1, ] and an empty index x[] among them; a matrix literal's
* row of numbers alone of more or fewer elements than its first, also of
* numbers alone, a bad shape, at the semicolon or the bracket that ends the
* row; an index of three indices or more, and a
* branch of piecewise of a number of arguments it does not take, a wrong
* arity, at the index's [ and at the branch; any other token out of place, a
* comma outside a call's brackets, a literal's and an index's, a semicolon
* outside a literal's, and a ' where no operand ends before it, among them, an
* unexpected token, as are a when or an otherwise anywhere but as a whole
* argument of piecewise, an otherwise before its last, and an argument of
* piecewise that is no branch, each at its first token.
*   STEPS. Step 14 (2b) filled expr_parse for numbers, names and the
* arithmetic, step 15 (2c) for calls, constants, the factorial and the bars,
* step 16 (2d) for relations, logic, chains and piecewise, step 19 (3a) for
* the literals, step 20 (3b) for indexing and the transpose, and step 21
* (3c) for the empty literal and blocks: no token answers
* expr_status::not_yet. The names mod, and, or and not are operators: none
* of the four names a variable.
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/parse.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.11.07
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  PARSING
    -------
    1.  The tokens as the climber reads them
         1.  expr_parse_role
         2.  expr_parse_token
         3.  The grammar
    2.  The passes
         1.  Roles
         2.  Brackets and bars
         3.  The climb
         4.  Chains
         5.  Groups
    3.  The entries
         1.  expr_parse_tokens
         2.  expr_parse
*/

#ifndef DJINTERP_MATH_EXPR_PARSE_HPP
#define DJINTERP_MATH_EXPR_PARSE_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../../../../c/modules/math/literal.h"           // literal_to_value
#include "../../../functional/maybe.hpp"                  // maybe, just
#include "../../parse/expression/expression_ops.hpp"      // evaluate
#include "../../parse/expression/expression_parse.hpp"    // parse_expression
#include "../math_common.hpp"                             // NS_MATH
#include "../value.hpp"                                   // value
#include "./expr_common.hpp"                              // expr_result
#include "./function.hpp"                                 // the constants
#include "./operator.hpp"                                 // expr_signature
#include "./term.hpp"                                     // expr_term
#include "./token.hpp"                                    // expr_tokenize


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  PARSING
//==============================================================================


// 1.1    The tokens as the climber reads them
//------------------------------------------------------------------------------
NS_INTERNAL

    // 1.1.1
    // expr_parse_role
    //   enum: what a token is to the passes, and to parse/expression's
    // climber: an atom, or an operator. A call's function, its commas, the
    // round and the square brackets, the semicolons and the bars are read
    // before the climb, each group as an atom.
    enum class expr_parse_role
    {
        atom         = 1,    // a leaf: a number, a constant or a variable
        op           = 2,    // an operator, by its place: - unary or binary
        open         = 3,    // (
        close        = 4,    // )
        call         = 5,    // a call's function: a name before its (
        comma        = 6,    // ,
        bar_open     = 7,    // | that opens an absolute value
        bar_close    = 8,    // | that closes one
        square_open  = 9,    // [ that opens a literal
        square_close = 10,   // ]
        semicolon    = 11,   // ;
        index_open   = 12    // [ that opens an operand's indices (3b)
    };

    // 1.1.2
    // expr_parse_token
    //   struct: a token with its role: its atom, or its operator; its text,
    // for a message; and the offset of its first character in the text. A *
    // that implied multiplication adds has the text "*" and the offset of
    // the operand after it; a group read as an atom has its function's, its
    // first bar's or its open bracket's.
    struct expr_parse_token
    {
        expr_parse_role role;
        expr_atom       atom;
        expr_op         op;
        std::string     text;
        std::size_t     offset;

        expr_parse_token(
            expr_parse_role    _role,
            const std::string& _text,
            std::size_t        _offset
        )
            : role(_role),
              atom(),
              op(),
              text(_text),
              offset(_offset)
        {}
    };

    // 1.1.3
    // The grammar
    //   the four questions the climber asks of a token, as parse/expression's
    // expression_grammar takes them: its atom, its operator, and whether it
    // opens a bracket, and whether it closes one, which no token it is given
    // does: each group is an atom by then (S215).
    inline maybe<expr_atom>
    expr_atom_of_token(
        const expr_parse_token& _token
    )
    {
        // an atom
        if (_token.role == expr_parse_role::atom)
        {
            return just(_token.atom);
        }

        return nothing<expr_atom>();
    }

    inline maybe<expr_op>
    expr_op_of_token(
        const expr_parse_token& _token
    )
    {
        // an operator
        if (_token.role == expr_parse_role::op)
        {
            return just(_token.op);
        }

        return nothing<expr_op>();
    }

    inline bool
    expr_is_bracket_token(
        const expr_parse_token& _token
    )
    {
        static_cast<void>(_token);

        return false;
    }

NS_END  // internal

// 1.2    The passes
//------------------------------------------------------------------------------
NS_INTERNAL

    // 1.2.1
    // Roles
    //   each token given its role, left to right, as the banner says; a
    // token no role fits answers unexpected_token at its place.

    // expr_parse_unexpected
    //   function: the error for a token no role fits, at its offset.
    inline expr_error
    expr_parse_unexpected(
        const std::string& _text,
        std::size_t        _at
    )
    {
        return expr_error(expr_status::unexpected_token,
                          _at,
                          "unexpected '" + _text + "'");
    }

    // expr_ends_operand
    //   function: whether the last token given a role ends an operand, so
    // that a - after it is binary, a | after it may close a bar, a [ after it
    // is indexing, and a * stands between it and an operand where
    // multiplication is implied: an atom, a close, round or square, a
    // closing bar, the factorial or the transpose.
    inline bool
    expr_ends_operand(
        const std::vector<expr_parse_token>& _done
    )
    {
        // nothing before
        if (_done.empty())
        {
            return false;
        }

        const expr_parse_token& last = _done.back();

        return ( (last.role == expr_parse_role::atom)         ||
                 (last.role == expr_parse_role::close)        ||
                 (last.role == expr_parse_role::square_close) ||
                 (last.role == expr_parse_role::bar_close)    ||
                 ( (last.role == expr_parse_role::op) &&
                   ( (last.op.kind == expr_op_kind::factorial) ||
                     (last.op.kind == expr_op_kind::transpose) ) ) );
    }

    // expr_imply_product
    //   function: with implied multiplication on, a * before an operand
    // that starts at _token, where an operand ends before it: never between
    // two numbers.
    inline void
    expr_imply_product(
        std::vector<expr_parse_token>& _done,
        const expr_token&              _token,
        const expr_syntax&             _syntax
    )
    {
        // an operand's end, then this operand's start, and not two numbers
        if ( (_syntax.implied_multiplication) &&
             (expr_ends_operand(_done)) &&
             (!( (_token.kind == expr_token_kind::number) &&
                 (_done.back().role == expr_parse_role::atom) &&
                 (_done.back().atom.kind == expr_atom_kind::number) )) )
        {
            expr_parse_token times(expr_parse_role::op,
                                   "*",
                                   _token.offset);

            times.op = expr_op_kind::mul;
            _done.push_back(times);
        }

        return;
    }

    // expr_role_of_number
    //   function: a number token as an atom: its text one unsigned decimal
    // number, its value read exactly, and the text kept as its literal.
    inline expr_result<expr_parse_token>
    expr_role_of_number(
        const expr_token& _token
    )
    {
        // an unsigned decimal number, and nothing else
        if (!expr_unsigned_decimal(_token.text))
        {
            return err<expr_parse_token, expr_error>(
                expr_parse_unexpected(_token.text, _token.offset));
        }

        dj_math_value exact = d_math_value_none();

        // a decimal number is a literal: its value, exact where it can be
        static_cast<void>(d_math_literal_to_value(&exact,
                                                  _token.text.data(),
                                                  _token.text.size()));

        expr_parse_token atom(expr_parse_role::atom,
                              _token.text,
                              _token.offset);

        atom.atom = expr_atom(expr_atom_kind::number,
                              value::adopt(exact),
                              _token.text);

        return ok<expr_parse_token, expr_error>(atom);
    }

    // expr_word_of
    //   function: the operator a name spells, mod, and, or or not, or none.
    inline expr_op_kind
    expr_word_of(
        const std::string& _text
    )
    {
        return ( (_text == "mod") ? expr_op_kind::mod
               : (_text == "and") ? expr_op_kind::logical_and
               : (_text == "or")  ? expr_op_kind::logical_or
               : (_text == "not") ? expr_op_kind::logical_not
                                  : expr_op_kind::none );
    }

    // expr_role_of_name
    //   function: a name token as the operator mod, and, or or not; as a
    // call's function, before an open round bracket; as a constant, where
    // the standard constant table has the name; and as a variable otherwise.
    inline expr_result<expr_parse_token>
    expr_role_of_name(
        const expr_token& _token,
        const expr_token* _next
    )
    {
        expr_parse_token named(expr_parse_role::atom,
                               _token.text,
                               _token.offset);

        const expr_op_kind word = expr_word_of(_token.text);

        // an operator named by a word: mod, and the logic
        if (word != expr_op_kind::none)
        {
            named.role = expr_parse_role::op;
            named.op   = word;

            return ok<expr_parse_token, expr_error>(named);
        }

        // a call: a name before an open round bracket
        if ( (_next != D_NULLPTR) &&
             (_next->kind == expr_token_kind::open) &&
             (_next->text == "(") )
        {
            named.role = expr_parse_role::call;

            return ok<expr_parse_token, expr_error>(named);
        }

        // a constant, or a variable
        named.atom = expr_atom( ( (expr_standard_constants().find(_token.text)
                                       != D_NULLPTR)
                                      ? expr_atom_kind::constant
                                      : expr_atom_kind::variable ),
                                value(),
                                _token.text);

        return ok<expr_parse_token, expr_error>(named);
    }

    // expr_relation_of
    //   function: the relation a symbol spells, = != < <= > >=, or none.
    inline expr_op_kind
    expr_relation_of(
        const std::string& _text
    )
    {
        return ( (_text == "=")  ? expr_op_kind::eq
               : (_text == "!=") ? expr_op_kind::ne
               : (_text == "<")  ? expr_op_kind::lt
               : (_text == "<=") ? expr_op_kind::le
               : (_text == ">")  ? expr_op_kind::gt
               : (_text == ">=") ? expr_op_kind::ge
                                 : expr_op_kind::none );
    }

    // expr_role_of_symbol
    //   function: a symbol token as its operator: + and - by where they
    // stand, unary where no operand ends before them; * / ^ and the
    // relations their own; ! the factorial and ' the transpose, whose place
    // the climber judges; and where _bars, G5's bars, are on, a bar, closing
    // where an operand ends before it and _bar_inside, a bar being the
    // innermost group open, and opening otherwise.
    inline expr_result<expr_parse_token>
    expr_role_of_symbol(
        const expr_token&                    _token,
        const std::vector<expr_parse_token>& _done,
        bool                                 _bar_inside,
        bool                                 _bars
    )
    {
        expr_parse_token symbol(expr_parse_role::op,
                                _token.text,
                                _token.offset);
        const bool         unary    = !expr_ends_operand(_done);
        const expr_op_kind relation = expr_relation_of(_token.text);

        // the arithmetic, the relations, the factorial and the transpose
        if (relation != expr_op_kind::none)
        {
            symbol.op = relation;
        }
        else if (_token.text == "+")
        {
            symbol.op = (unary ? expr_op_kind::pos : expr_op_kind::add);
        }
        else if (_token.text == "-")
        {
            symbol.op = (unary ? expr_op_kind::neg : expr_op_kind::sub);
        }
        else if (_token.text == "*")
        {
            symbol.op = expr_op_kind::mul;
        }
        else if (_token.text == "/")
        {
            symbol.op = expr_op_kind::div;
        }
        else if (_token.text == "^")
        {
            symbol.op = expr_op_kind::pow;
        }
        else if (_token.text == "!")
        {
            symbol.op = expr_op_kind::factorial;
        }
        else if (_token.text == "'")
        {
            symbol.op = expr_op_kind::transpose;
        }
        else if ( (_token.text == "|") &&
                  (_bars) )
        {
            // a bar: closing the innermost after an operand, else opening
            symbol.role = ( ( (!unary) && (_bar_inside) )
                                ? expr_parse_role::bar_close
                                : expr_parse_role::bar_open );
        }
        else
        {
            return err<expr_parse_token, expr_error>(
                expr_parse_unexpected(_token.text, _token.offset));
        }

        return ok<expr_parse_token, expr_error>(symbol);
    }

    // expr_role_of_bracket
    //   function: a bracket token: ( opens and ) closes; [ opens a literal
    // where no operand ends before it, and after one opens the operand's
    // indices, never an implied product (S243); ] closes either; any other
    // text no bracket.
    inline expr_result<expr_parse_token>
    expr_role_of_bracket(
        const expr_token&                    _token,
        const std::vector<expr_parse_token>& _done
    )
    {
        const bool       opens = (_token.kind == expr_token_kind::open);
        expr_parse_token bracket(( opens ? expr_parse_role::open
                                         : expr_parse_role::close ),
                                 _token.text,
                                 _token.offset);

        // a round bracket of its kind
        if ( (_token.text == (opens ? "(" : ")")) )
        {
            return ok<expr_parse_token, expr_error>(bracket);
        }

        // a square one of its kind: indices after an operand, else a literal
        if ( (_token.text == (opens ? "[" : "]")) )
        {
            bracket.role = ( (!opens)                   ? expr_parse_role::
                                                              square_close
                           : (expr_ends_operand(_done)) ? expr_parse_role::
                                                              index_open
                                                        : expr_parse_role::
                                                              square_open );

            return ok<expr_parse_token, expr_error>(bracket);
        }

        return err<expr_parse_token, expr_error>(
            expr_parse_unexpected(_token.text, _token.offset));
    }

    // expr_role_of
    //   function: one token's role, by its kind; _next is the token after
    // it, or null for the last; _bar_inside whether a bar is the innermost
    // group open.
    inline expr_result<expr_parse_token>
    expr_role_of(
        const expr_token&                    _token,
        const expr_token*                    _next,
        const std::vector<expr_parse_token>& _done,
        bool                                 _bar_inside,
        const expr_syntax&                   _syntax
    )
    {
        // by kind
        switch (_token.kind)
        {
            case expr_token_kind::number:
                return expr_role_of_number(_token);

            case expr_token_kind::name:
                return expr_role_of_name(_token, _next);

            case expr_token_kind::symbol:
                return expr_role_of_symbol(_token,
                                           _done,
                                           _bar_inside,
                                           _syntax.absolute_bars);

            case expr_token_kind::open:
            case expr_token_kind::close:
                return expr_role_of_bracket(_token,
                                            _done);

            case expr_token_kind::comma:
                return ok<expr_parse_token, expr_error>(
                    expr_parse_token(expr_parse_role::comma,
                                     _token.text,
                                     _token.offset));

            case expr_token_kind::semicolon:
                return ok<expr_parse_token, expr_error>(
                    expr_parse_token(expr_parse_role::semicolon,
                                     _token.text,
                                     _token.offset));

            default:
                break;
        }

        // a token of no kind: the end is never asked here
        return err<expr_parse_token, expr_error>(
            expr_parse_unexpected(_token.text, _token.offset));
    }

    // expr_starts_operand
    //   function: whether a role starts an operand, so that a * may stand
    // before it: an atom, an open bracket, a call's function, an opening
    // bar; and a literal's open, which no operand's end stands before.
    inline bool
    expr_starts_operand(
        expr_parse_role _role
    )
    {
        return ( (_role == expr_parse_role::atom)        ||
                 (_role == expr_parse_role::open)        ||
                 (_role == expr_parse_role::call)        ||
                 (_role == expr_parse_role::bar_open)    ||
                 (_role == expr_parse_role::square_open) );
    }

    // expr_is_opener, expr_is_closer
    //   functions: whether a role opens a group, a bracket, round or square,
    // a literal's or indices', or a bar; and whether it closes one.
    inline bool
    expr_is_opener(
        expr_parse_role _role
    )
    {
        return ( (_role == expr_parse_role::open)        ||
                 (_role == expr_parse_role::bar_open)    ||
                 (_role == expr_parse_role::square_open) ||
                 (_role == expr_parse_role::index_open) );
    }

    inline bool
    expr_is_closer(
        expr_parse_role _role
    )
    {
        return ( (_role == expr_parse_role::close)     ||
                 (_role == expr_parse_role::bar_close) ||
                 (_role == expr_parse_role::square_close) );
    }

    // expr_closes
    //   function: whether a closer's role closes an opener's: ) a (, ] a [,
    // a literal's or indices', and a closing bar an opening one.
    inline bool
    expr_closes(
        expr_parse_role _closer,
        expr_parse_role _opener
    )
    {
        return ( ( (_closer == expr_parse_role::close) &&
                   (_opener == expr_parse_role::open) ) ||
                 ( (_closer == expr_parse_role::square_close) &&
                   ( (_opener == expr_parse_role::square_open) ||
                     (_opener == expr_parse_role::index_open) ) ) ||
                 ( (_closer == expr_parse_role::bar_close) &&
                   (_opener == expr_parse_role::bar_open) ) );
    }

    // expr_roles
    //   function: every token given its role, a * implied where the syntax
    // asks; the groups open, round or bars, followed so that a bar knows
    // whether it closes. An end token ends the tokens, and any token after
    // one is unexpected.
    inline expr_result<std::vector<expr_parse_token> >
    expr_roles(
        const std::vector<expr_token>& _tokens,
        const expr_syntax&             _syntax
    )
    {
        std::vector<expr_parse_token> done;
        std::vector<bool>             bars;

        // left to right
        for (std::size_t i = 0; i < _tokens.size(); ++i)
        {
            const expr_token& token = _tokens[i];
            const expr_token* next  = ( (i + 1 < _tokens.size())
                                            ? &_tokens[i + 1]
                                            : D_NULLPTR );

            // an end token: the last, or before an unexpected one
            if (token.kind == expr_token_kind::end)
            {
                if (next != D_NULLPTR)
                {
                    return err<std::vector<expr_parse_token>, expr_error>(
                        expr_parse_unexpected(next->text, next->offset));
                }

                break;
            }

            const expr_result<expr_parse_token> role =
                expr_role_of(token,
                             next,
                             done,
                             ( (!bars.empty()) && (bars.back()) ),
                             _syntax);

            // a token no role fits
            if (role.is_err())
            {
                return err<std::vector<expr_parse_token>, expr_error>(
                    role.error());
            }

            const expr_parse_role kind = role.value().role;

            // an operand's start, after an operand's end
            if (expr_starts_operand(kind))
            {
                expr_imply_product(done, token, _syntax);
            }

            // the groups open: a close ends a bracket, round or square, as
            // the brackets' pass checks
            if (expr_is_opener(kind))
            {
                bars.push_back(kind == expr_parse_role::bar_open);
            }
            else if ( (kind == expr_parse_role::bar_close) ||
                      ( ( (kind == expr_parse_role::close) ||
                          (kind == expr_parse_role::square_close) ) &&
                        (!bars.empty()) &&
                        (!bars.back()) ) )
            {
                bars.pop_back();
            }

            done.push_back(role.value());
        }

        return ok<std::vector<expr_parse_token>, expr_error>(done);
    }

    // 1.2.2
    // Brackets and bars
    //   function: each opening bracket's and bar's closing one, by index, the
    // others' entries the token count; or the error unbalanced at the first
    // close that opens nothing, or at the innermost open bracket or bar never
    // closed: one open where a close of another kind comes among them, a bar
    // open where a bracket closes, or a round bracket where a square one
    // does.

    inline expr_result<std::vector<std::size_t> >
    expr_match_groups(
        const std::vector<expr_parse_token>& _tokens
    )
    {
        std::vector<std::size_t> match(_tokens.size(), _tokens.size());
        std::vector<std::size_t> open;

        // left to right, the opens held until closed
        for (std::size_t i = 0; i < _tokens.size(); ++i)
        {
            const expr_parse_role role = _tokens[i].role;

            if (expr_is_opener(role))
            {
                open.push_back(i);

                continue;
            }

            if (!expr_is_closer(role))
            {
                continue;
            }

            // a close with nothing to close
            if (open.empty())
            {
                return err<std::vector<std::size_t>, expr_error>(
                    expr_error(expr_status::unbalanced,
                               _tokens[i].offset,
                               "'" + _tokens[i].text +
                                   "' closes no bracket"));
            }

            // a close of another kind than the innermost open: that open is
            // never closed
            if (!expr_closes(role,
                             _tokens[open.back()].role))
            {
                return err<std::vector<std::size_t>, expr_error>(
                    expr_error(expr_status::unbalanced,
                               _tokens[open.back()].offset,
                               "'" + _tokens[open.back()].text +
                                   "' is never closed"));
            }

            match[open.back()] = i;
            open.pop_back();
        }

        // an open never closed: the innermost
        if (!open.empty())
        {
            return err<std::vector<std::size_t>, expr_error>(
                expr_error(expr_status::unbalanced,
                           _tokens[open.back()].offset,
                           "'" + _tokens[open.back()].text +
                               "' is never closed"));
        }

        return ok<std::vector<std::size_t>, expr_error>(match);
    }

    // 1.2.3
    // The climb
    //   function: parse/expression's precedence climbing over the tokens and
    // the parsing table, and its error at its place: the end of the tokens,
    // where an operand must stand, a missing operand; any other token out of
    // place, an unexpected token. The tokens end at _closer, a comma or a
    // bracket or a bar, or at the text's end, _end, where _closer is null.
    // The climber meets no bracket: each group is one operand by then, and
    // an operand's indices a postfix operator.

    // expr_parsing_signature
    //   function: the table the climber reads, built once: the standard one,
    // and indexing, A[i, j], a postfix operator tighter than every other, so
    // that -v[1] is -(v[1]), A'[1] is (A')[1] and x ^ v[1] is x ^ (v[1]); its
    // indices are put back after the climb (expr_put_parts).
    inline const expr_signature&
    expr_parsing_signature()
    {
        static const expr_signature table =
            expr_signature(expr_signature::standard()).define(
                expr_op_kind::index,
                operator_descriptor(1,
                                    expr_precedence_index,
                                    DAssocLeft,
                                    DFixPostfix,
                                    "[]"));

        return table;
    }

    inline expr_result<expr_term>
    expr_climb(
        const std::vector<expr_parse_token>& _tokens,
        const expr_parse_token*              _closer,
        std::size_t                          _end
    )
    {
        const auto grammar = ::djinterp::make_expression_grammar<expr_op,
                                                                 expr_atom>(
            expr_parsing_signature(),
            &expr_atom_of_token,
            &expr_op_of_token,
            &expr_is_bracket_token,
            &expr_is_bracket_token);
        const parse::parse_result<expr_term> parsed =
            ::djinterp::parse_expression(_tokens, grammar);

        // the term
        if (parsed.ok())
        {
            return ok<expr_term, expr_error>(parsed.value());
        }

        const std::size_t at = parsed.error().offset();

        // the end, where an operand must stand: the text's, or the closer's
        if ( (parsed.error().status() == parse::DParseStatusEndOfInput) ||
             (at >= _tokens.size()) )
        {
            return err<expr_term, expr_error>(
                ( (_closer == D_NULLPTR)
                      ? expr_error(expr_status::missing_operand,
                                   _end,
                                   "an operand is missing at the end")
                      : expr_error(expr_status::missing_operand,
                                   _closer->offset,
                                   "an operand is missing before '" +
                                       _closer->text + "'") ));
        }

        return err<expr_term, expr_error>(
            expr_parse_unexpected(_tokens[at].text, _tokens[at].offset));
    }

    // 1.2.4
    // Chains
    //   a chain of relations, a < b <= c, read as the conjunction of its
    // links, a < b and b <= c, each operand between two links in both (S214).
    // The climb groups a chain to the right, as parse/expression groups a
    // non-associative operator, so that its relations stand on the right
    // spine of its first, a < (b <= c); and as the climb meets no bracket, a
    // relation it makes an operand of another is a link of the same chain,
    // never a bracketed relation, which is an atom until the climb is done.

    // expr_is_relation
    //   function: whether an operator is a relation, = != < <= > >=.
    inline bool
    expr_is_relation(
        const expr_op& _op
    )
    {
        return ( (_op.kind == expr_op_kind::eq) ||
                 (_op.kind == expr_op_kind::ne) ||
                 (_op.kind == expr_op_kind::lt) ||
                 (_op.kind == expr_op_kind::le) ||
                 (_op.kind == expr_op_kind::gt) ||
                 (_op.kind == expr_op_kind::ge) );
    }

    // expr_join_chains
    //   function: the climbed _term with each chain of relations joined: its
    // operands and relations taken down its right spine, each operand's own
    // chains joined, and its links, left to right, joined by and, which is
    // left-associative. Each node is visited once, so that a chain of n
    // links costs n.
    inline expr_term
    expr_join_chains(
        const expr_term& _term
    )
    {
        // a leaf: itself
        if (!expr_is_application(_term))
        {
            return _term;
        }

        const expr_op&                op       = expr_op_of(_term);
        const std::vector<expr_term>& operands = expr_operands_of(_term);

        // any operator but a relation: itself, of its operands joined
        if (!expr_is_relation(op))
        {
            std::vector<expr_term> joined;

            for (std::size_t i = 0; i < operands.size(); ++i)
            {
                joined.push_back(expr_join_chains(operands[i]));
            }

            return expr_apply<expr_op, expr_atom>(op,
                                                  joined);
        }

        std::vector<expr_term> chain;
        std::vector<expr_op>   relations;
        expr_term              rest = _term;

        // a relation: its chain, down the right spine
        while ( (expr_is_application(rest)) &&
                (expr_is_relation(expr_op_of(rest))) )
        {
            const expr_term next = expr_operands_of(rest)[1];

            relations.push_back(expr_op_of(rest));
            chain.push_back(expr_join_chains(expr_operands_of(rest)[0]));
            rest = next;
        }

        chain.push_back(expr_join_chains(rest));

        expr_term links = expr_apply<expr_op, expr_atom>(relations[0],
                                                         chain[0],
                                                         chain[1]);

        // the links, left to right
        for (std::size_t k = 1; k < relations.size(); ++k)
        {
            links = expr_apply<expr_op, expr_atom>(
                expr_op(expr_op_kind::logical_and),
                links,
                expr_apply<expr_op, expr_atom>(relations[k],
                                               chain[k],
                                               chain[k + 1]));
        }

        return links;
    }

    // 1.2.5
    // Groups
    //   each group, a call, a pair of bars, a pair of round brackets, a
    // literal or an operand's indices, read as one operand: its arguments,
    // its elements or its indices, parsed in turn. Each group but indices
    // stands in the climb as an atom, a variable of the name "\x01", which
    // no text has, its number the index of the group's term; indices stand
    // as the postfix operator index, its function "\x01" and that index. The
    // climb done, each such atom is put back as its term, and each such
    // operator as the term index of its operand and its indices. A call's
    // term is the call, a pair of bars' the call of abs, a pair of brackets'
    // what they hold, a vector literal's the term vector of its elements, a
    // matrix literal's the term matrix of its rows, each the term row of its
    // elements, and indices' the term index of them alone. The branches of
    // piecewise, when(c, v) and otherwise(d), are read into its arguments:
    // piecewise(c, v, d). A range's pass and a group's call each other for
    // each group a group holds, so the frames a nesting stacks are kept to
    // what both passes need: a group's end, its term and its errors, are in
    // functions of their own, which no nesting stacks (S250).

    // expr_range_state
    //   struct: what the pass over a range keeps: the tokens the climb
    // reads, each group's term, whether a relation stands among them, and
    // the first error met, where one was.
    struct expr_range_state
    {
        std::vector<expr_parse_token> flat;
        std::vector<expr_term>        parts;
        bool                          linked;
        bool                          failed;
        expr_error                    first;

        expr_range_state()
            : flat(),
              parts(),
              linked(false),
              failed(false),
              first()
        {}
    };

    // expr_group_reading
    //   struct: what the pass over a group keeps: a call's arguments, or a
    // literal's rows of elements, the first a vector's or indices'; and the
    // number of elements in the row being read and in the first row. It is
    // in the frame of every nesting's reading (S250), so whether a row is
    // surely of numbers alone (S261) is asked of its elements at its end.
    struct expr_group_reading
    {
        std::vector<expr_term>                arguments;
        std::vector<std::vector<expr_term> > rows;
        std::size_t                           slots;
        std::size_t                           width;

        expr_group_reading()
            : arguments(),
              rows(1),
              slots(0),
              width(0)
        {}
    };

    // expr_surely_scalar
    //   function: whether a term's value is surely no matrix: each of its
    // leaves a number, and none of its nodes a literal. A variable, or a
    // constant, may be bound to a matrix; an operator, a call or indexing of
    // numbers alone gives a number or an error.
    inline bool
    expr_surely_scalar(
        const expr_term& _term
    )
    {
        return ::djinterp::evaluate<bool, expr_op, expr_atom>(
            _term,
            // a leaf: a number
            [](const expr_atom& _atom) -> bool
            {
                return (_atom.kind == expr_atom_kind::number);
            },
            // an application: no literal, of operands surely no matrix
            [](const expr_op&           _op,
               const std::vector<bool>& _operands) -> bool
            {
                bool scalar = ( (_op.kind != expr_op_kind::vector) &&
                                (_op.kind != expr_op_kind::matrix) &&
                                (_op.kind != expr_op_kind::row) );

                for (std::size_t i = 0; i < _operands.size(); ++i)
                {
                    scalar = ( (scalar) &&
                               (_operands[i]) );
                }

                return scalar;
            });
    }

    // expr_row_surely_scalar
    //   function: whether each of a literal's row's elements is surely no
    // matrix (expr_surely_scalar).
    inline bool
    expr_row_surely_scalar(
        const std::vector<expr_term>& _row
    )
    {
        bool scalar = true;

        for (std::size_t i = 0; i < _row.size(); ++i)
        {
            scalar = ( (scalar) &&
                       (expr_surely_scalar(_row[i])) );
        }

        return scalar;
    }

    // expr_group_mark
    //   function: the mark a group's stand-in carries in the climb, "\x01",
    // which no text has: an atom's name, and the start of an index
    // operator's function, the group's number after it.
    inline std::string
    expr_group_mark()
    {
        return std::string(1,
                           '\x01');
    }

    // expr_elements_text
    //   function: a number of elements in words: 1 element, 2 elements.
    inline std::string
    expr_elements_text(
        std::size_t _count
    )
    {
        return std::to_string(_count) +
               ( (_count == 1) ? " element" : " elements" );
    }

    // expr_literal_of
    //   function: a literal's term, of its rows: the term vector of its one
    // row's elements; or, where a semicolon parts its rows, the term matrix
    // of its rows, each the term row of its elements.
    inline expr_term
    expr_literal_of(
        const std::vector<std::vector<expr_term> >& _rows
    )
    {
        // one row: a vector
        if (_rows.size() == 1)
        {
            return expr_application(expr_op_kind::vector,
                                    _rows[0]);
        }

        std::vector<expr_term> rows;

        // its rows
        for (std::size_t r = 0; r < _rows.size(); ++r)
        {
            rows.push_back(expr_application(expr_op_kind::row,
                                            _rows[r]));
        }

        return expr_application(expr_op_kind::matrix,
                                rows);
    }

    // expr_keep_first
    //   function: _error kept as the first, where no error was kept yet;
    // the passes meet errors left to right.
    inline void
    expr_keep_first(
        bool&             _failed,
        expr_error&       _first,
        const expr_error& _error
    )
    {
        if (!_failed)
        {
            _failed = true;
            _first  = _error;
        }

        return;
    }

    // expr_is_branch
    //   function: whether a name is a branch of piecewise's, when or
    // otherwise.
    inline bool
    expr_is_branch(
        const std::string& _name
    )
    {
        return ( (_name == "when") ||
                 (_name == "otherwise") );
    }

    // expr_add_branch
    //   function: the argument _argument of piecewise, which begins at the
    // token _at, read into _arguments: when(c, v) as c and v, otherwise(d)
    // as d, the last; or its error: wrong_arity for a branch of a number of
    // arguments it does not take, unexpected_token for an otherwise before
    // the last argument, or for an argument that is no branch. _last is
    // whether the argument is the last.
    inline bool
    expr_add_branch(
        std::vector<expr_term>& _arguments,
        const expr_term&        _argument,
        const expr_parse_token& _at,
        bool                    _last,
        expr_error&             _error
    )
    {
        const bool branch = ( (expr_is_application(_argument)) &&
                              (expr_op_of(_argument).kind ==
                                   expr_op_kind::call) &&
                              (expr_is_branch(
                                   expr_op_of(_argument).function)) );

        // an argument that is no branch
        if (!branch)
        {
            _error = expr_error(expr_status::unexpected_token,
                                _at.offset,
                                "an argument of piecewise is when(c, v) or "
                                "otherwise(v)");

            return false;
        }

        const std::vector<expr_term>& parts = expr_operands_of(_argument);
        const bool                    when  =
            (expr_op_of(_argument).function == "when");
        const std::size_t             takes = ( when ? 2 : 1 );

        // a branch of a number of arguments it does not take
        if (parts.size() != takes)
        {
            _error = expr_error(expr_status::wrong_arity,
                                _at.offset,
                                _at.text + " takes " +
                                    std::to_string(takes) +
                                    ( when ? " arguments, not "
                                           : " argument, not " ) +
                                    std::to_string(parts.size()));

            return false;
        }

        // an otherwise before the last argument
        if ( (!when) &&
             (!_last) )
        {
            _error = expr_error(expr_status::unexpected_token,
                                _at.offset,
                                "otherwise is the last branch of piecewise");

            return false;
        }

        _arguments.insert(_arguments.end(),
                          parts.begin(),
                          parts.end());

        return true;
    }

    // expr_keep_error
    //   function: _error kept as the range's first, where it has none yet.
    D_NOINLINE inline void
    expr_keep_error(
        expr_range_state& _state,
        const expr_error& _error
    )
    {
        expr_keep_first(_state.failed,
                        _state.first,
                        _error);

        return;
    }

    // expr_misplaced_branch
    //   function: a when or an otherwise, _token, anywhere but as a whole
    // argument of piecewise: the error unexpected_token, kept at its place.
    D_NOINLINE inline void
    expr_misplaced_branch(
        expr_range_state&       _state,
        const expr_parse_token& _token
    )
    {
        expr_keep_error(_state,
                        expr_error(expr_status::unexpected_token,
                                   _token.offset,
                                   _token.text + " stands only as a whole "
                                   "argument of piecewise"));

        return;
    }

    // expr_take_argument
    //   function: one argument, element or index of a group, which begins
    // at the token _start, read into _reading: its error kept; a literal's
    // element into the row being read; piecewise's argument, a branch, into
    // its arguments, else its error kept; any other into the arguments.
    // _last is whether it is the group's last.
    D_NOINLINE inline void
    expr_take_argument(
        expr_group_reading&           _reading,
        expr_range_state&             _state,
        const expr_result<expr_term>& _argument,
        const expr_parse_token&       _start,
        bool                          _square,
        bool                          _cases,
        bool                          _last
    )
    {
        expr_error branch_error;

        // its error, or where it goes
        if (_argument.is_err())
        {
            expr_keep_error(_state,
                            _argument.error());
        }
        else if (_square)
        {
            _reading.rows.back().push_back(_argument.value());
        }
        else if (!_cases)
        {
            _reading.arguments.push_back(_argument.value());
        }
        else if (!expr_add_branch(_reading.arguments,
                                  _argument.value(),
                                  _start,
                                  _last,
                                  branch_error))
        {
            expr_keep_error(_state,
                            branch_error);
        }

        // a literal's element counted in its row
        _reading.slots += ( _square ? 1 : 0 );

        return;
    }

    // expr_end_row
    //   function: a literal's row ended at _at, a semicolon or the close:
    // where it and the first are surely of numbers alone, of as many
    // elements as the first, parsed or not, or a bad shape at _at, while
    // blocks of matrices have their shapes only as values (S261); and past
    // a semicolon, another row begun.
    D_NOINLINE inline void
    expr_end_row(
        expr_group_reading&     _reading,
        expr_range_state&       _state,
        const expr_parse_token& _at,
        bool                    _more
    )
    {
        // the first row: its width
        if (_reading.rows.size() == 1)
        {
            _reading.width = _reading.slots;
        }

        // a row of numbers of another number of elements than the first's
        if ( (_reading.slots != _reading.width) &&
             (expr_row_surely_scalar(_reading.rows.front())) &&
             (expr_row_surely_scalar(_reading.rows.back())) )
        {
            expr_keep_error(_state,
                            expr_error(expr_status::bad_shape,
                                       _at.offset,
                                       "a row of " +
                                           expr_elements_text(
                                               _reading.slots) +
                                           ", where the first row has " +
                                           expr_elements_text(
                                               _reading.width)));
        }

        // past a semicolon, another row
        if (_more)
        {
            _reading.rows.push_back(std::vector<expr_term>());
            _reading.slots = 0;
        }

        return;
    }

    // expr_end_group
    //   function: a group read, the one _token opens, of kind _kind: its
    // term among the range's parts, and its stand-in among the tokens the
    // climb reads: the call; abs of what bars hold; the literal; what
    // brackets hold; and indices, of one index or two, else wrong_arity at
    // the open bracket, as the term index of them alone.
    D_NOINLINE inline void
    expr_end_group(
        expr_range_state&         _state,
        const expr_parse_token&   _token,
        expr_parse_role           _kind,
        const expr_group_reading& _reading
    )
    {
        const std::size_t index = _state.parts.size();

        // indices: one or two, standing as the operator index
        if (_kind == expr_parse_role::index_open)
        {
            const std::size_t count = _reading.arguments.size();

            if ( (count < 1) ||
                 (count > 2) )
            {
                expr_keep_error(_state,
                                expr_error(expr_status::wrong_arity,
                                           _token.offset,
                                           "indexing takes 1 or 2 indices, "
                                           "not " + std::to_string(count)));
            }

            expr_parse_token indices(expr_parse_role::op,
                                     _token.text,
                                     _token.offset);

            indices.op          = expr_op_kind::index;
            indices.op.function = expr_group_mark() + std::to_string(index);
            _state.parts.push_back(expr_application(expr_op_kind::index,
                                                    _reading.arguments));
            _state.flat.push_back(indices);

            return;
        }

        // any other group: its term, and the atom standing for it
        _state.parts.push_back(
            (_kind == expr_parse_role::call)
                ? expr_call(_token.text, _reading.arguments)
            : (_kind == expr_parse_role::bar_open)
                ? expr_call("abs", _reading.arguments)
            : (_kind == expr_parse_role::square_open)
                ? expr_literal_of(_reading.rows)
                : ( _reading.arguments.empty() ? expr_term()
                                               : _reading.arguments[0] ));

        expr_parse_token atom(expr_parse_role::atom,
                              _token.text,
                              _token.offset);

        atom.atom = expr_atom(expr_atom_kind::variable,
                              value(static_cast<dj_math_imax>(index)),
                              expr_group_mark());
        _state.flat.push_back(atom);

        return;
    }

    // expr_put_parts
    //   function: _term with each atom that stands for a group put back as
    // its term, of _parts, and each operator index that stands for indices
    // as the term index of its operand and the indices.
    inline expr_term
    expr_put_parts(
        const expr_term&              _term,
        const std::vector<expr_term>& _parts
    )
    {
        return ::djinterp::evaluate<expr_term, expr_op, expr_atom>(
            _term,
            // a leaf: a group's term, or itself
            [&_parts](const expr_atom& _atom) -> expr_term
            {
                dj_math_imax index = 0;

                // an atom that stands for a group
                if ( (_atom.kind == expr_atom_kind::variable) &&
                     (_atom.text == expr_group_mark()) &&
                     (_atom.number.to(index)) )
                {
                    return _parts[static_cast<std::size_t>(index)];
                }

                return expr_leaf<expr_op, expr_atom>(_atom);
            },
            // an application: indices put after their operand, or itself,
            // of its operands put back
            [&_parts](const expr_op&                _op,
                      const std::vector<expr_term>& _operands) -> expr_term
            {
                const std::string mark = expr_group_mark();

                // an operator that stands for indices
                if ( (_op.kind == expr_op_kind::index) &&
                     (_op.function.size() > mark.size()) &&
                     (_op.function.compare(0,
                                           mark.size(),
                                           mark) == 0) )
                {
                    const std::size_t index = static_cast<std::size_t>(
                        std::stoul(_op.function.substr(mark.size())));
                    std::vector<expr_term> operands = _operands;
                    const std::vector<expr_term>& indices =
                        expr_operands_of(_parts[index]);

                    operands.insert(operands.end(),
                                    indices.begin(),
                                    indices.end());

                    return expr_application(expr_op_kind::index,
                                            operands);
                }

                return expr_apply<expr_op, expr_atom>(_op, _operands);
            });
    }

    // expr_parse_range
    //   function: the tokens from _first to _last, each opener's close at its
    // index in _match, as one term: each group in them read as one operand,
    // its arguments, elements or indices in turn, a matrix literal's rows of
    // as many elements each, else a bad shape at the token that ends the
    // row of another number; then the climb; then its chains joined, where
    // a relation stands among the tokens, and its groups put back, where
    // there are any. The tokens end at _closer, or at the text's end, _end,
    // where _closer is null. _branch is whether the tokens are an argument
    // of piecewise, where a branch may stand, whole. Of an argument's error
    // and the climb's, the further left is given.
    inline expr_result<expr_term>
    expr_parse_range(
        const std::vector<expr_parse_token>& _tokens,
        const std::vector<std::size_t>&      _match,
        std::size_t                          _first,
        std::size_t                          _last,
        const expr_parse_token*              _closer,
        std::size_t                          _end,
        bool                                 _branch
    );

    // expr_read_group
    //   function: the group that _tokens[_at] opens, a call's function or an
    // opener, read into _state as one operand (expr_end_group), each of its
    // arguments, elements or indices parsed in turn; the index after its
    // close returned. _first, _last and _branch are its range's, for a
    // branch of piecewise, which stands only as a whole argument of it.
    D_NOINLINE inline std::size_t
    expr_read_group(
        const std::vector<expr_parse_token>& _tokens,
        const std::vector<std::size_t>&      _match,
        std::size_t                          _at,
        std::size_t                          _first,
        std::size_t                          _last,
        std::size_t                          _end,
        bool                                 _branch,
        expr_range_state&                    _state
    )
    {
        const expr_parse_token& token  = _tokens[_at];
        const bool              call   = (token.role == expr_parse_role::call);
        const bool              square = (token.role ==
                                              expr_parse_role::square_open);
        const bool              listed = ( (call) ||
                                           (square) ||
                                           (token.role ==
                                                expr_parse_role::index_open) );
        const bool              cases  = ( (call) &&
                                           (token.text == "piecewise") );
        const std::size_t       open   = ( call ? (_at + 1) : _at );
        const std::size_t       close  = _match[open];
        std::size_t             start  = open + 1;
        std::size_t             j      = start;
        bool                    more   = ( ( (!call) &&
                                             (!square) ) ||
                                           (start < close) );
        expr_group_reading      reading;

        // a branch anywhere but whole, as an argument of piecewise
        if ( (call) &&
             (expr_is_branch(token.text)) &&
             (!( (_branch) &&
                 (_at == _first) &&
                 (close + 1 == _last) )) )
        {
            expr_misplaced_branch(_state,
                                  token);
        }

        // the arguments, split at the commas a call's brackets or indices
        // hold directly, and a literal's elements, at the commas and the
        // semicolons its brackets hold directly; what bars or brackets
        // enclose, whole; a call of no argument, f(), and the empty literal,
        // [], none
        while (more)
        {
            const expr_parse_role role = _tokens[j].role;
            const bool            ends = ( (j == close) ||
                                           ( (listed) &&
                                             (role ==
                                                  expr_parse_role::comma) ) ||
                                           ( (square) &&
                                             (role ==
                                                  expr_parse_role::
                                                      semicolon) ) );

            // a nested group, whole
            if (!ends)
            {
                j = ( (expr_is_opener(role)) ? (_match[j] + 1)
                                             : (j + 1) );

                continue;
            }

            // an argument: an empty one's climb finds its operand missing
            // before its comma or close
            expr_take_argument(reading,
                               _state,
                               expr_parse_range(_tokens,
                                                _match,
                                                start,
                                                j,
                                                &_tokens[j],
                                                _end,
                                                cases),
                               _tokens[start],
                               square,
                               cases,
                               (j == close));

            // a literal's row ended, at a semicolon or the close
            if ( (square) &&
                 (role != expr_parse_role::comma) )
            {
                expr_end_row(reading,
                             _state,
                             _tokens[j],
                             (j < close));
            }

            // past a comma or a semicolon, another argument or element;
            // past the close, done
            more  = (j < close);
            start = j + 1;
            j     = start;
        }

        expr_end_group(_state,
                       token,
                       token.role,
                       reading);

        return (close + 1);
    }

    // expr_finish_range
    //   function: a range's groups read: the climb over the tokens _state
    // holds, the further left of its error and the groups' given; then its
    // chains joined, where a relation stands in it, and its groups put back,
    // where it has any: each a walk of the whole term, which a text of
    // neither, as a long sum, does without.
    D_NOINLINE inline expr_result<expr_term>
    expr_finish_range(
        const expr_range_state& _state,
        const expr_parse_token* _closer,
        std::size_t             _end
    )
    {
        const expr_result<expr_term> climbed = expr_climb(_state.flat,
                                                          _closer,
                                                          _end);

        // the further left of the two errors
        if ( (climbed.is_err()) &&
             ( (!_state.failed) ||
               (climbed.error().offset < _state.first.offset) ) )
        {
            return climbed;
        }

        if (_state.failed)
        {
            return err<expr_term, expr_error>(_state.first);
        }

        expr_term term = climbed.value();

        if (_state.linked)
        {
            term = expr_join_chains(term);
        }

        if (!_state.parts.empty())
        {
            term = expr_put_parts(term,
                                  _state.parts);
        }

        return ok<expr_term, expr_error>(term);
    }

    inline expr_result<expr_term>
    expr_parse_range(
        const std::vector<expr_parse_token>& _tokens,
        const std::vector<std::size_t>&      _match,
        std::size_t                          _first,
        std::size_t                          _last,
        const expr_parse_token*              _closer,
        std::size_t                          _end,
        bool                                 _branch
    )
    {
        expr_range_state state;

        for (std::size_t i = _first; i < _last; )
        {
            const expr_parse_token& token = _tokens[i];

            // a token the climb reads as it is, a relation marked
            if ( (token.role != expr_parse_role::call) &&
                 (!expr_is_opener(token.role)) )
            {
                state.linked = ( (state.linked) ||
                                 ( (token.role == expr_parse_role::op) &&
                                   (expr_is_relation(token.op)) ) );
                state.flat.push_back(token);
                ++i;

                continue;
            }

            // a group, read as one operand
            i = expr_read_group(_tokens,
                                _match,
                                i,
                                _first,
                                _last,
                                _end,
                                _branch,
                                state);
        }

        return expr_finish_range(state,
                                 _closer,
                                 _end);
    }

    // expr_parse_at
    //   function: the passes over the tokens, _end the offset of the text's
    // end.
    inline expr_result<expr_term>
    expr_parse_at(
        const std::vector<expr_token>& _tokens,
        const expr_syntax&             _syntax,
        std::size_t                    _end
    )
    {
        const expr_result<std::vector<expr_parse_token> > roles =
            expr_roles(_tokens, _syntax);

        // a token no role fits
        if (roles.is_err())
        {
            return err<expr_term, expr_error>(roles.error());
        }

        const expr_result<std::vector<std::size_t> > matched =
            expr_match_groups(roles.value());

        // a bracket or a bar unmatched
        if (matched.is_err())
        {
            return err<expr_term, expr_error>(matched.error());
        }

        return expr_parse_range(roles.value(),
                                matched.value(),
                                0,
                                roles.value().size(),
                                D_NULLPTR,
                                _end,
                                false);
    }

NS_END  // internal

// 1.3    The entries
//------------------------------------------------------------------------------
// 1.3.1
// expr_parse_tokens
/**
 * @brief Tokens to a term.
 *
 * @param[in] _tokens  the tokens, as expr_tokenize cuts them; an end token
 *                     ends them.
 * @param[in] _syntax  gate G5's options.
 * @return the term all the tokens mean; or the error unexpected_token,
 *         missing_operand, unbalanced, wrong_arity or bad_shape, at the
 *         offset of its token, the end being past the last token's text.
 */
D_NODISCARD inline expr_result<expr_term>
expr_parse_tokens(
    const std::vector<expr_token>& _tokens,
    const expr_syntax&             _syntax = expr_syntax()
)
{
    std::size_t end = 0;

    // the end: an end token's place, or past the last token's text
    for (std::size_t i = 0; i < _tokens.size(); ++i)
    {
        if (_tokens[i].kind == expr_token_kind::end)
        {
            end = _tokens[i].offset;

            break;
        }

        end = _tokens[i].offset + _tokens[i].text.size();
    }

    return internal::expr_parse_at(_tokens, _syntax, end);
}

// 1.3.2
// expr_parse
/**
 * @brief Math text to a term.
 *
 * @param[in] _text    the text.
 * @param[in] _syntax  gate G5's options.
 * @return the term the whole text means, each number a literal of its text;
 *         or the error bad_character, unexpected_token, missing_operand,
 *         unbalanced, wrong_arity or bad_shape, at its place in _text, the
 *         end being _text's length.
 */
D_NODISCARD inline expr_result<expr_term>
expr_parse(
    const std::string& _text,
    const expr_syntax& _syntax = expr_syntax()
)
{
    const expr_result<std::vector<expr_token> > tokens =
        expr_tokenize(_text, _syntax);

    // a character no token begins with
    if (tokens.is_err())
    {
        return err<expr_term, expr_error>(tokens.error());
    }

    return internal::expr_parse_at(tokens.value(), _syntax, _text.size());
}


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_EXPR_PARSE_HPP
