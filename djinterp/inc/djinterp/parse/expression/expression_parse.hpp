/******************************************************************************
* djinterp [expression]                                      expression_parse.hpp
*
*   The inverse of the renderer: text (already lexed into tokens) into an
* expression term, with operator precedence resolved from the very same
* operator_signature.  Parsing an operator grammar is precedence climbing (a
* Pratt parse), and precedence climbing cannot be a fixed combinator tree --
* its levels are *runtime* data read from the signature, not structure known
* at compile time -- so it is written directly over parser.hpp's cursor and
* result (parse_state / parse_result), the one cluster file that draws on the
* parse subframework.
*
*   THE CLIMB.  parse_expr(min) reads one operand (a leaf, a prefix
* application, or a parenthesized sub-expression -- the "start" forms), then
* folds in each following operator whose precedence is at least min: an infix
* operator parses its right operand at a climbed precedence (one past its own
* when left-associative, so equal operators group left; its own when right-
* associative, grouping right); a postfix operator wraps the operand in
* place.  An operator binding looser than min ends the current level and
* returns control to the caller.  It is exactly the shape the renderer
* parenthesizes for, run backwards.
*
*   WHAT THE LANGUAGE SUPPLIES.  Tokenization is the caller's -- this parser
* consumes a token sequence and asks four questions about a token, given as
* an expression_grammar: atom_of (is it an atom? then its value), op_of (does
* it name an operator? then its id, whose metadata the signature holds),
* is_open / is_close (the grouping brackets).  A token that is both an atom
* and an operator is taken as an atom.  The operator ids' precedence,
* associativity, and fixity come from the signature.
*
*   RESULT.  A parse_result<expression<OpId, Atom>>: the term on success, a
* parse_error (with offset and message) on an unexpected or missing token.
* The whole token sequence must be consumed; trailing tokens are an error.
*
*
* TABLE OF CONTENTS
* =================
* I.    GRAMMAR                              (expression_grammar / factory)
* II.   PRATT CORE                           (internal: nud / climb)
* III.  PARSE ENTRY                          (parse_expression)
*
*
* path:      /inc/djinterp/parse/expression/expression_parse.hpp
* link(s):   ch-parsing.tex, ch-synthesis.tex
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#ifndef DJINTERP_EXPRESSION_EXPRESSION_PARSE_
#define DJINTERP_EXPRESSION_EXPRESSION_PARSE_ 1

// std
#include <cstddef>
#include <string>
#include <type_traits>
#include <vector>
// djinterp
#include "./expression.hpp"
#include "../../core/functional/maybe.hpp"
#include "../../parse/parser/parser.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    GRAMMAR                                               ///
///////////////////////////////////////////////////////////////////////////////

// expression_grammar
//   struct: the language-specific hooks the precedence climber needs, over a
// shared operator_signature.  signature holds each operator's precedence /
// associativity / fixity; atom_of : Token -> maybe<Atom> recognizes and
// reads a leaf; op_of : Token -> maybe<OpId> names an operator; is_open /
// is_close : Token -> bool are the grouping brackets.  The recognizers are
// held by value; the signature by reference, so it must outlive the parse.
template<typename _OpId,
         typename _Atom,
         typename _Signature,
         typename _AtomOf,
         typename _OpOf,
         typename _IsOpen,
         typename _IsClose>
struct expression_grammar
{
    using op_id_type = _OpId;
    using atom_type  = _Atom;

    const _Signature& signature;
    _AtomOf           atom_of;
    _OpOf             op_of;
    _IsOpen           is_open;
    _IsClose          is_close;
};

// make_expression_grammar
//   function: assembles an expression_grammar, deducing the hook types.  The
// operator id and atom types are named explicitly (they cannot be recovered
// until a token is in hand), the rest follow from the arguments.
template<typename _OpId,
         typename _Atom,
         typename _Signature,
         typename _AtomOf,
         typename _OpOf,
         typename _IsOpen,
         typename _IsClose>
D_NODISCARD
expression_grammar<_OpId, _Atom, _Signature,
                   typename std::decay<_AtomOf>::type,
                   typename std::decay<_OpOf>::type,
                   typename std::decay<_IsOpen>::type,
                   typename std::decay<_IsClose>::type>
make_expression_grammar
(
    const _Signature& _signature,
    _AtomOf           _atom_of,
    _OpOf             _op_of,
    _IsOpen           _is_open,
    _IsClose          _is_close
)
{
    return expression_grammar<_OpId, _Atom, _Signature,
        typename std::decay<_AtomOf>::type,
        typename std::decay<_OpOf>::type,
        typename std::decay<_IsOpen>::type,
        typename std::decay<_IsClose>::type>{
            _signature, _atom_of, _op_of, _is_open, _is_close };
}


///////////////////////////////////////////////////////////////////////////////
///             II.   PRATT CORE                                            ///
///////////////////////////////////////////////////////////////////////////////
//   The precedence climber.  parse_nud reads a start-of-expression form;
// parse_climb reads one and folds in trailing operators down to a minimum
// precedence.  The two are mutually recursive (a prefix operand and a
// bracketed group re-enter the climb), so they are declared first.  Both are
// internal; parse_expression is the public entry.

NS_INTERNAL

    // expr_error
    //   helper: a failed parse_result carrying a parse_error.
    template<typename _Result>
    D_NODISCARD
    parse::parse_result<_Result>
    expr_error(
        parse::parse_status _status,
        std::size_t         _offset,
        const std::string&  _message
    )
    {
        return parse::parse_result<_Result>(
            parse::parse_error(_status, _offset, _message));
    }


    // -- forward declarations -----------------------------------------------

    template<typename _Token,
             typename _Grammar>
    D_NODISCARD
    parse::parse_result<expression<typename _Grammar::op_id_type,
                                   typename _Grammar::atom_type> >
    parse_nud(parse::parse_state<_Token>& _state, const _Grammar& _grammar);

    template<typename _Token,
             typename _Grammar>
    D_NODISCARD
    parse::parse_result<expression<typename _Grammar::op_id_type,
                                   typename _Grammar::atom_type> >
    parse_climb(parse::parse_state<_Token>& _state,
                const _Grammar&             _grammar,
                int                         _min_precedence);


    // -- parse_nud (start of an expression) ---------------------------------

    // parse_nud
    //   function: parses a form that begins an expression -- an atom (a
    // leaf), a prefix operator applied to an operand, or a parenthesized
    // sub-expression.
    template<typename _Token,
             typename _Grammar>
    D_NODISCARD
    parse::parse_result<expression<typename _Grammar::op_id_type,
                                   typename _Grammar::atom_type> >
    parse_nud
    (
        parse::parse_state<_Token>& _state,
        const _Grammar&             _grammar
    )
    {
        using op_id_t  = typename _Grammar::op_id_type;
        using atom_t   = typename _Grammar::atom_type;
        using expr_t   = expression<op_id_t, atom_t>;
        using result_t = parse::parse_result<expr_t>;

        if (_state.at_end())
        {
            return expr_error<expr_t>(
                parse::DParseStatusEndOfInput,
                _state.offset(),
                "expected an expression");
        }

        const _Token& _token = *_state.current();

        // an atom -- a leaf
        maybe<atom_t> _atom = _grammar.atom_of(_token);

        if (_atom.has_value())
        {
            _state.advance();

            return result_t(expr_leaf<op_id_t, atom_t>(_atom.value()));
        }

        // a bracketed group -- parse a full sub-expression, expect the close
        if (_grammar.is_open(_token))
        {
            _state.advance();

            result_t _inner = parse_climb<_Token, _Grammar>(_state, _grammar, 0);

            if (!_inner.ok())
            {
                return _inner;
            }

            if (_state.at_end() || !_grammar.is_close(*_state.current()))
            {
                return expr_error<expr_t>(
                    parse::DParseStatusMalformed,
                    _state.offset(),
                    "expected a closing bracket");
            }

            _state.advance();

            return _inner;
        }

        // a prefix operator -- applied to the operand that follows
        maybe<op_id_t> _op = _grammar.op_of(_token);

        if (_op.has_value())
        {
            const operator_descriptor* _descriptor =
                _grammar.signature.describe(_op.value());

            if ((_descriptor != nullptr) && (_descriptor->fix == DFixPrefix))
            {
                _state.advance();

                result_t _operand = parse_climb<_Token, _Grammar>(
                    _state, _grammar, _descriptor->precedence);

                if (!_operand.ok())
                {
                    return _operand;
                }

                return result_t(expr_apply<op_id_t, atom_t>(
                    _op.value(), _operand.value()));
            }
        }

        return expr_error<expr_t>(
            parse::DParseStatusMalformed,
            _state.offset(),
            "unexpected token in expression");
    }


    // -- parse_climb (operand, then trailing operators) ---------------------

    // parse_climb
    //   function: parses one operand, then folds in each following operator
    // whose precedence is at least _min_precedence -- infix operators taking
    // a right operand, postfix operators wrapping the accumulated left.
    template<typename _Token,
             typename _Grammar>
    D_NODISCARD
    parse::parse_result<expression<typename _Grammar::op_id_type,
                                   typename _Grammar::atom_type> >
    parse_climb
    (
        parse::parse_state<_Token>& _state,
        const _Grammar&             _grammar,
        int                         _min_precedence
    )
    {
        using op_id_t  = typename _Grammar::op_id_type;
        using atom_t   = typename _Grammar::atom_type;
        using expr_t   = expression<op_id_t, atom_t>;
        using result_t = parse::parse_result<expr_t>;

        result_t _left_result = parse_nud<_Token, _Grammar>(_state, _grammar);

        if (!_left_result.ok())
        {
            return _left_result;
        }

        expr_t _left = _left_result.value();

        // fold in trailing operators that bind at least as tightly as the
        // caller's floor
        for (;;)
        {
            if (_state.at_end())
            {
                break;
            }

            maybe<op_id_t> _op = _grammar.op_of(*_state.current());

            if (!_op.has_value())
            {
                break;
            }

            const operator_descriptor* _descriptor =
                _grammar.signature.describe(_op.value());

            if (_descriptor == nullptr)
            {
                break;
            }

            // infix -- take a right operand at the climbed precedence
            if (_descriptor->fix == DFixInfix)
            {
                if (_descriptor->precedence < _min_precedence)
                {
                    break;
                }

                _state.advance();

                // left-associative climbs one past its own precedence (so
                // equal operators group left); right-associative stays
                const int _right_min = _descriptor->precedence +
                    ((_descriptor->assoc == DAssocLeft) ? 1 : 0);

                result_t _right_result = parse_climb<_Token, _Grammar>(
                    _state, _grammar, _right_min);

                if (!_right_result.ok())
                {
                    return _right_result;
                }

                _left = expr_apply<op_id_t, atom_t>(
                    _op.value(), _left, _right_result.value());

                continue;
            }

            // postfix -- wrap the accumulated left
            if (_descriptor->fix == DFixPostfix)
            {
                if (_descriptor->precedence < _min_precedence)
                {
                    break;
                }

                _state.advance();

                _left = expr_apply<op_id_t, atom_t>(_op.value(), _left);

                continue;
            }

            // a prefix or nullary operator here does not continue the
            // expression
            break;
        }

        return result_t(_left);
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             III.  PARSE ENTRY                                           ///
///////////////////////////////////////////////////////////////////////////////

// parse_expression (over a cursor)
//   function: parses a whole expression from a parse_state, requiring the
// token sequence to be fully consumed.  Trailing tokens are an error.
template<typename _Token,
         typename _Grammar>
D_NODISCARD
parse::parse_result<expression<typename _Grammar::op_id_type,
                               typename _Grammar::atom_type> >
parse_expression
(
    parse::parse_state<_Token>& _state,
    const _Grammar&             _grammar
)
{
    using expr_t   = expression<typename _Grammar::op_id_type,
                                typename _Grammar::atom_type>;
    using result_t = parse::parse_result<expr_t>;

    result_t _result = internal::parse_climb<_Token, _Grammar>(
        _state, _grammar, 0);

    if (!_result.ok())
    {
        return _result;
    }

    // the whole token sequence must be consumed
    if (!_state.at_end())
    {
        return internal::expr_error<expr_t>(
            parse::DParseStatusMalformed,
            _state.offset(),
            "unexpected trailing tokens after expression");
    }

    return _result;
}

// parse_expression (over a token sequence)
//   function: parses a whole expression from a token vector -- seeds a
// cursor over it and defers to the cursor form.
template<typename _Token,
         typename _Grammar>
D_NODISCARD
parse::parse_result<expression<typename _Grammar::op_id_type,
                               typename _Grammar::atom_type> >
parse_expression
(
    const std::vector<_Token>& _tokens,
    const _Grammar&            _grammar
)
{
    parse::parse_state<_Token> _state(_tokens.data(), _tokens.size());

    return parse_expression<_Token, _Grammar>(_state, _grammar);
}


NS_END  // djinterp


#endif  // DJINTERP_EXPRESSION_EXPRESSION_PARSE_
