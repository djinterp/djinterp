/******************************************************************************
* djinterp [dawk]                                                     dparse.c
*
*   Definitions for the non-inline declarations in dparse.h.
*     The precedence ladder is POSIX awk's, and two rungs of it are unusual.
* Concatenation is juxtaposition, so it binds by the absence of an operator and
* has to ask whether the next token could begin an expression at all. And the
* relational operators do not associate, so `a < b < c` is a syntax error
* rather than a comparison against a truth value.
*     `>` inside a print argument list is redirection, not comparison, so the
* expression parser carries a flag that stops there.
*
*
* path:      /src/djinterp/tools/dawk/dparse.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dparse.h"  // corresponding header
// std
#include <stdio.h>    // snprintf
#include <stdlib.h>   // malloc, realloc, free
#include <string.h>   // memcpy, strcmp, strlen
// djinterp
#include "../../../../inc/djinterp/tools/dawk/dregex.h"  // d_regex_compile


//==============================================================================
// 1.  PARSER STATE
//==============================================================================


// 1.1    State
//------------------------------------------------------------------------------
// 1.1.1
// d_internal_parser
//   struct: the scanner, one token of lookahead, and the program under
//   construction.  `no_gt` suppresses `>` as a comparison while a print
//   argument list is being read.  The saved slot is one token of extra
//   lookahead, needed in exactly one place: `for (` followed by a name is
//   the membership loop only when `in` comes next, and by then the name has
//   already been consumed.
struct d_internal_parser
{
    struct d_awk_lexer*   lexer;
    struct d_awk_token    token;
    char*                 token_text;
    size_t                token_length;
    struct d_awk_token    saved;
    char*                 saved_text;
    size_t                saved_length;
    bool                  has_saved;
    bool                  at_end;
    bool                  no_gt;
    bool                  failed;
    const char*           origin;
    struct d_awk_program* program;
};

static char g_error[256];


static struct d_awk_node* d_internal_expr(struct d_internal_parser* _p);
static struct d_awk_node* d_internal_power(struct d_internal_parser* _p);
static struct d_awk_node* d_internal_unary(struct d_internal_parser* _p);
static struct d_awk_node* d_internal_postfix(struct d_internal_parser* _p);
static struct d_awk_node* d_internal_concat(struct d_internal_parser* _p);
static struct d_awk_node* d_internal_statement(struct d_internal_parser* _p);
static struct d_awk_node* d_internal_block(struct d_internal_parser* _p);


/*
d_internal_error
  Records the first diagnostic and stops the parse.

Parameter(s):
  _p:       the parser.
  _message: the text of the diagnostic.
Return:
  none.
*/
static void
d_internal_error_at(
    struct d_internal_parser* _p,
    size_t                    _line,
    size_t                    _column,
    const char*               _message
)
{
    // Every diagnostic reads `file:line:col: syntax error: what`.  The
    // position leads so editors and CI parse it without configuration, and
    // the phrase `syntax error` is there because every awk emits it and
    // scripts grep for it.
    if (!_p->failed)
    {
        snprintf(g_error,
                 sizeof(g_error),
                 "%s:%zu:%zu: syntax error: %s",
                 _p->origin ? _p->origin : "-",
                 _line,
                 _column,
                 _message);
        _p->failed = true;
    }

    return;
}


/*
d_internal_error
  Records a diagnostic at the lookahead's position.

Parameter(s):
  _p:       the parser.
  _message: the text of the diagnostic.
Return:
  none.
*/
static void
d_internal_error(
    struct d_internal_parser* _p,
    const char*               _message
)
{
    d_internal_error_at(_p, _p->token.line, _p->token.column, _message);

    return;
}


/*
d_internal_advance
  Reads the next token into the lookahead slot, copying its text.

Parameter(s):
  _p: the parser.
Return:
  none.
*/
static bool
d_internal_take(
    struct d_internal_parser* _p,
    struct d_awk_token*       _out,
    char**                    _buffer,
    size_t*                   _capacity
)
{
    struct d_awk_token next;

    // a scanner that is finished produces nothing further
    if (!d_awk_lexer_next(_p->lexer, &next))
    {
        return false;
    }

    // the scanner reuses its scratch buffer, so the text must be copied
    if (next.length >= *_capacity)
    {
        char* grown = realloc(*_buffer, next.length + 1u);

        // report the failure rather than reading a stale buffer
        if (!grown)
        {
            d_internal_error(_p, "out of memory");
            return false;
        }

        *_buffer   = grown;
        *_capacity = next.length + 1u;
    }

    // a token without text leaves the buffer alone
    if (next.text)
    {
        memcpy(*_buffer, next.text, next.length);
        (*_buffer)[next.length] = '\0';
        next.text               = *_buffer;
    }

    *_out = next;

    return true;
}


/*
d_internal_peek_kind
  Returns the kind of the token after the lookahead, reading it if needed.

Parameter(s):
  _p: the parser.
Return:
  The kind, or D_AWK_TOK_EOF at end of input.
*/
static enum d_awk_token_kind
d_internal_peek_kind(
    struct d_internal_parser* _p
)
{
    // the second token is read once and then held
    if (!_p->has_saved)
    {
        // end of input reports the terminal kind rather than latching
        if (!d_internal_take(_p,
                             &_p->saved,
                             &_p->saved_text,
                             &_p->saved_length))
        {
            return D_AWK_TOK_EOF;
        }

        _p->has_saved = true;
    }

    return _p->saved.kind;
}


static void
d_internal_advance(
    struct d_internal_parser* _p
)
{
    struct d_awk_token next;

    // a token already read by the peek is consumed before the scanner is
    if (_p->has_saved)
    {
        _p->has_saved = false;

        // the saved text must be copied, since the buffers are distinct
        if (_p->saved.length >= _p->token_length)
        {
            char* grown = realloc(_p->token_text, _p->saved.length + 1u);

            // report the failure rather than reading a stale buffer
            if (!grown)
            {
                d_internal_error(_p, "out of memory");
                return;
            }

            _p->token_text   = grown;
            _p->token_length = _p->saved.length + 1u;
        }

        // a token without text leaves the buffer alone
        if (_p->saved.text)
        {
            memcpy(_p->token_text, _p->saved.text, _p->saved.length);
            _p->token_text[_p->saved.length] = '\0';
            _p->saved.text                   = _p->token_text;
        }

        _p->token = _p->saved;

        return;
    }

    // end of input latches, so the lookahead stays stable afterwards
    if (!d_awk_lexer_next(_p->lexer, &next))
    {
        const char* const message = d_awk_lexer_error(_p->lexer);

        // a scanner diagnostic carries its own position, not the lookahead's
        if (message)
        {
            d_internal_error_at(_p,
                                d_awk_lexer_line(_p->lexer),
                                d_awk_lexer_column(_p->lexer),
                                message);
        }

        _p->at_end            = true;
        _p->token.kind        = D_AWK_TOK_EOF;
        _p->token.text        = NULL;
        _p->token.length      = 0;

        return;
    }

    // the scanner reuses its scratch buffer, so the text must be copied
    if (next.length >= _p->token_length)
    {
        char* grown = realloc(_p->token_text, next.length + 1u);

        // report the failure rather than reading a stale buffer
        if (!grown)
        {
            d_internal_error(_p, "out of memory");
            return;
        }

        _p->token_text   = grown;
        _p->token_length = next.length + 1u;
    }

    // a token without text leaves the buffer alone
    if (next.text)
    {
        memcpy(_p->token_text, next.text, next.length);
        _p->token_text[next.length] = '\0';
        next.text                   = _p->token_text;
    }

    _p->token = next;

    return;
}


/*
d_internal_check
  Reports whether the lookahead has a given kind.

Parameter(s):
  _p:    the parser.
  _kind: the kind to test for.
Return:
  A boolean value corresponding to either:
  - true, if the lookahead matches, or
  - false, otherwise.
*/
static bool
d_internal_check(
    struct d_internal_parser* _p,
    enum d_awk_token_kind     _kind
)
{
    return ((!_p->at_end) && (_p->token.kind == _kind));
}


/*
d_internal_accept
  Consumes the lookahead when it has a given kind.

Parameter(s):
  _p:    the parser.
  _kind: the kind to accept.
Return:
  A boolean value corresponding to either:
  - true, if the token was consumed, or
  - false, otherwise.
*/
static bool
d_internal_accept(
    struct d_internal_parser* _p,
    enum d_awk_token_kind     _kind
)
{
    // only a matching lookahead is consumed
    if (!d_internal_check(_p, _kind))
    {
        return false;
    }

    d_internal_advance(_p);

    return true;
}


/*
d_internal_expect
  Consumes the lookahead, reporting a diagnostic when it does not match.

Parameter(s):
  _p:    the parser.
  _kind: the kind required.
Return:
  A boolean value corresponding to either:
  - true, if the token was consumed, or
  - false, otherwise.
*/
static bool
d_internal_expect(
    struct d_internal_parser* _p,
    enum d_awk_token_kind     _kind
)
{
    // a matching lookahead is simply consumed
    if (d_internal_accept(_p, _kind))
    {
        return true;
    }

    char message[128];

    snprintf(message,
             sizeof(message),
             "expected %s, found %s",
             d_awk_token_name(_kind),
             _p->at_end ? "end of input" : d_awk_token_name(_p->token.kind));
    d_internal_error(_p, message);

    return false;
}


/*
d_internal_skip_terminators
  Consumes any run of newlines and semicolons.

Parameter(s):
  _p: the parser.
Return:
  none.
*/
static void
d_internal_skip_terminators(
    struct d_internal_parser* _p
)
{
    while ( (d_internal_check(_p, D_AWK_TOK_NEWLINE)) ||
            (d_internal_check(_p, D_AWK_TOK_SEMI)) )
    {
        d_internal_advance(_p);
    }

    return;
}


//==============================================================================
// 2.  NODE CONSTRUCTION
//==============================================================================


/*
d_internal_node
  Allocates a node of a given kind.

Parameter(s):
  _p:    the parser, for diagnostics.
  _kind: the kind of node.
Return:
  The node, or NULL on failure.
*/
static struct d_awk_node*
d_internal_node(
    struct d_internal_parser* _p,
    enum d_awk_node_kind      _kind
)
{
    struct d_awk_node* node = calloc(1, sizeof(*node));

    // report the failure rather than returning a null child silently
    if (!node)
    {
        d_internal_error(_p, "out of memory");
        return NULL;
    }

    node->kind = _kind;
    node->line = _p->token.line;

    return node;
}


/*
d_internal_push
  Appends a child to a node's variadic list.

Parameter(s):
  _p:     the parser, for diagnostics.
  _node:  the node to extend.
  _child: the child to append.
Return:
  A boolean value corresponding to either:
  - true, if the child was appended, or
  - false, otherwise.
*/
static bool
d_internal_push(
    struct d_internal_parser* _p,
    struct d_awk_node*        _node,
    struct d_awk_node*        _child
)
{
    // a failed child never reaches the list
    if ((!_node) || (!_child))
    {
        return false;
    }

    // grow the list when it is full
    if (_node->count == _node->capacity)
    {
        const size_t capacity = (_node->capacity == 0)
                              ? 4u
                              : (_node->capacity * 2u);

        struct d_awk_node** grown =
            realloc(_node->list, capacity * sizeof(*grown));

        // report the failure rather than dropping the child
        if (!grown)
        {
            d_internal_error(_p, "out of memory");
            return false;
        }

        _node->list     = grown;
        _node->capacity = capacity;
    }

    _node->list[_node->count++] = _child;

    return true;
}


/*
d_internal_set_text
  Copies the lookahead's text onto a node.

Parameter(s):
  _p:    the parser.
  _node: the node to write to.
Return:
  A boolean value corresponding to either:
  - true, if the text was copied, or
  - false, otherwise.
*/
static bool
d_internal_set_text(
    struct d_internal_parser* _p,
    struct d_awk_node*        _node
)
{
    char* copy = malloc(_p->token.length + 1u);

    // report the failure rather than leaving a textless node
    if (!copy)
    {
        d_internal_error(_p, "out of memory");
        return false;
    }

    // a zero-length token still needs a terminated buffer
    if (_p->token.length > 0)
    {
        memcpy(copy, _p->token.text, _p->token.length);
    }

    copy[_p->token.length] = '\0';
    _node->text            = copy;
    _node->length          = _p->token.length;

    return true;
}


/*
d_awk_node_free
  Releases a node and everything below it.

Parameter(s):
  _node: the node to release; may be NULL.
Return:
  none.
*/
static void
d_internal_node_free(
    struct d_awk_node* _node
)
{
    if (_node)
    {
        d_internal_node_free(_node->a);
        d_internal_node_free(_node->b);
        d_internal_node_free(_node->c);
        d_internal_node_free(_node->d);

        for (size_t at = 0; at < _node->count; ++at)
        {
            d_internal_node_free(_node->list[at]);
        }

        d_regex_free(_node->regex);
        free(_node->list);
        free(_node->text);
        free(_node);
    }

    return;
}


//==============================================================================
// 3.  EXPRESSIONS
//==============================================================================
// The ladder runs, weakest binder first: assignment, conditional, or, and, in,
// match, relational, concatenation, additive, multiplicative, unary, power,
// increment, field, primary.


/*
d_internal_starts_expr
  Reports whether a token kind can begin an expression.
NOTE:
  This is what makes concatenation parseable: it binds by juxtaposition, so
  the only way to know a second operand follows is to ask whether the next
  token could start one.  `+` and `-` are excluded because the additive rung
  below has already taken them.

Parameter(s):
  _kind: the kind to test.
Return:
  A boolean value corresponding to either:
  - true, if an expression may begin here, or
  - false, otherwise.
*/
static bool
d_internal_starts_expr(
    enum d_awk_token_kind _kind
)
{
    switch (_kind)
    {
        case D_AWK_TOK_NUMBER:
        case D_AWK_TOK_STRING:
        case D_AWK_TOK_ERE:
        case D_AWK_TOK_NAME:
        case D_AWK_TOK_FUNC_NAME:
        case D_AWK_TOK_BUILTIN:
        case D_AWK_TOK_DOLLAR:
        case D_AWK_TOK_NOT:
        case D_AWK_TOK_LPAREN:
        case D_AWK_TOK_INCR:
        case D_AWK_TOK_DECR:
            return true;

        default:
            return false;
    }
}


/*
d_internal_is_lvalue
  Reports whether a node can be assigned to.

Parameter(s):
  _node: the node to test.
Return:
  A boolean value corresponding to either:
  - true, if the node is a variable, field or subscript, or
  - false, otherwise.
*/
static bool
d_internal_is_lvalue(
    const struct d_awk_node* _node
)
{
    // parameter validation first
    if (!_node)
    {
        return false;
    }

    return ( (_node->kind == D_AWK_N_VAR)   ||
             (_node->kind == D_AWK_N_FIELD) ||
             (_node->kind == D_AWK_N_INDEX) );
}


/*
d_internal_primary
  Parses a literal, name, call, field, parenthesised group or unary form.

Parameter(s):
  _p: the parser.
Return:
  The node, or NULL on failure.
*/
static struct d_awk_node*
d_internal_primary(
    struct d_internal_parser* _p
)
{
    // a numeric literal
    if (d_internal_check(_p, D_AWK_TOK_NUMBER))
    {
        struct d_awk_node* node = d_internal_node(_p, D_AWK_N_NUMBER);

        // abandon the parse when the node could not be held
        if (!node)
        {
            return NULL;
        }

        node->number = _p->token.number;
        d_internal_advance(_p);

        return node;
    }

    // a string literal
    if (d_internal_check(_p, D_AWK_TOK_STRING))
    {
        struct d_awk_node* node = d_internal_node(_p, D_AWK_N_STRING);

        // abandon the parse when the node could not be held
        if ((!node) || (!d_internal_set_text(_p, node)))
        {
            d_internal_node_free(node);
            return NULL;
        }

        d_internal_advance(_p);

        return node;
    }

    // a regular-expression literal, compiled once here
    if (d_internal_check(_p, D_AWK_TOK_ERE))
    {
        struct d_awk_node* node = d_internal_node(_p, D_AWK_N_REGEX);

        // abandon the parse when the node could not be held
        if ((!node) || (!d_internal_set_text(_p, node)))
        {
            d_internal_node_free(node);
            return NULL;
        }

        enum d_regex_status status = D_REGEX_OK;

        node->regex = d_regex_compile(node->text, &status);

        // a pattern that will not compile is a syntax error in the program
        if (!node->regex)
        {
            d_internal_error(_p, d_regex_status_text(status));
            d_internal_node_free(node);
            return NULL;
        }

        d_internal_advance(_p);

        return node;
    }

    // a field reference binds tighter than anything but grouping
    if (d_internal_accept(_p, D_AWK_TOK_DOLLAR))
    {
        struct d_awk_node* node = d_internal_node(_p, D_AWK_N_FIELD);

        // abandon the parse when the node could not be held
        if (!node)
        {
            return NULL;
        }

        node->a = d_internal_primary(_p);

        // abandon the parse when the index failed
        if (!node->a)
        {
            d_internal_node_free(node);
            return NULL;
        }

        return node;
    }

    // a prefix increment or decrement
    if ( (d_internal_check(_p, D_AWK_TOK_INCR)) ||
         (d_internal_check(_p, D_AWK_TOK_DECR)) )
    {
        struct d_awk_node* node = d_internal_node(_p, D_AWK_N_PREINCR);

        // abandon the parse when the node could not be held
        if (!node)
        {
            return NULL;
        }

        node->op = _p->token.kind;
        d_internal_advance(_p);
        node->a = d_internal_primary(_p);

        // the target of an increment must be assignable
        if (!d_internal_is_lvalue(node->a))
        {
            d_internal_error(_p, "increment requires a variable or field");
            d_internal_node_free(node);
            return NULL;
        }

        return node;
    }

    // a parenthesised group, or a parenthesised subscript list before `in`
    if (d_internal_accept(_p, D_AWK_TOK_LPAREN))
    {
        // Inside parentheses a `>` is a comparison again, even within a print
        // argument list: `print (a > b)` compares while `print a > b`
        // redirects.  Every bracketing construct below restores the flag the
        // same way.
        const bool outer_no_gt = _p->no_gt;

        _p->no_gt = false;

        struct d_awk_node* first = d_internal_expr(_p);

        // abandon the parse when the group failed
        if (!first)
        {
            return NULL;
        }

        // a comma inside makes it a subscript list, which only `in` accepts
        if (d_internal_check(_p, D_AWK_TOK_COMMA))
        {
            _p->no_gt = outer_no_gt;
            struct d_awk_node* node = d_internal_node(_p, D_AWK_N_IN);

            // abandon the parse when the node could not be held
            if ((!node) || (!d_internal_push(_p, node, first)))
            {
                d_internal_node_free(node);
                d_internal_node_free(first);
                return NULL;
            }

            while (d_internal_accept(_p, D_AWK_TOK_COMMA))
            {
                // abandon the parse when a subscript part failed
                if (!d_internal_push(_p, node, d_internal_expr(_p)))
                {
                    d_internal_node_free(node);
                    return NULL;
                }
            }

            _p->no_gt = outer_no_gt;

            // abandon the parse when the list is not closed
            if (!d_internal_expect(_p, D_AWK_TOK_RPAREN))
            {
                d_internal_node_free(node);
                return NULL;
            }

            // A parenthesised comma list is `(subscript) in array` when `in`
            // follows, and otherwise print's argument list -- `print (a, b)`.
            // Nothing else in the grammar accepts one, so it is carried as a
            // GROUPLIST that print flattens and every other context rejects.
            if (!d_internal_accept(_p, D_AWK_TOK_IN))
            {
                node->kind = D_AWK_N_GROUPLIST;
                return node;
            }

            node->b = d_internal_node(_p, D_AWK_N_VAR);

            // abandon the parse when the array name failed
            if ((!node->b) || (!d_internal_set_text(_p, node->b)))
            {
                d_internal_node_free(node);
                return NULL;
            }

            // the right operand of `in` must be a name
            if (!d_internal_expect(_p, D_AWK_TOK_NAME))
            {
                d_internal_node_free(node);
                return NULL;
            }

            return node;
        }

        _p->no_gt = outer_no_gt;

        // abandon the parse when the group is not closed
        if (!d_internal_expect(_p, D_AWK_TOK_RPAREN))
        {
            d_internal_node_free(first);
            return NULL;
        }

        return first;
    }

    // a builtin call, whose arity the interpreter checks
    if (d_internal_check(_p, D_AWK_TOK_BUILTIN))
    {
        struct d_awk_node* node = d_internal_node(_p, D_AWK_N_BUILTIN);

        // abandon the parse when the node could not be held
        if ((!node) || (!d_internal_set_text(_p, node)))
        {
            d_internal_node_free(node);
            return NULL;
        }

        d_internal_advance(_p);

        // length is the one builtin whose parentheses may be omitted
        if (d_internal_accept(_p, D_AWK_TOK_LPAREN))
        {
            const bool outer_no_gt = _p->no_gt;

            _p->no_gt = false;

            // an empty list is permitted, so the close is checked first
            if (!d_internal_check(_p, D_AWK_TOK_RPAREN))
            {
                do
                {
                    // abandon the parse when an argument failed
                    if (!d_internal_push(_p, node, d_internal_expr(_p)))
                    {
                        d_internal_node_free(node);
                        return NULL;
                    }
                }
                while (d_internal_accept(_p, D_AWK_TOK_COMMA));
            }

            _p->no_gt = outer_no_gt;

            // abandon the parse when the list is not closed
            if (!d_internal_expect(_p, D_AWK_TOK_RPAREN))
            {
                d_internal_node_free(node);
                return NULL;
            }
        }
        else if (strcmp(node->text, "length") != 0)
        {
            d_internal_error(_p, "builtin call requires an argument list");
            d_internal_node_free(node);
            return NULL;
        }

        return node;
    }

    // a user-defined function call
    if (d_internal_check(_p, D_AWK_TOK_FUNC_NAME))
    {
        struct d_awk_node* node = d_internal_node(_p, D_AWK_N_CALL);

        // abandon the parse when the node could not be held
        if ((!node) || (!d_internal_set_text(_p, node)))
        {
            d_internal_node_free(node);
            return NULL;
        }

        d_internal_advance(_p);

        // abandon the parse when the argument list does not open
        if (!d_internal_expect(_p, D_AWK_TOK_LPAREN))
        {
            d_internal_node_free(node);
            return NULL;
        }

        const bool outer_no_gt = _p->no_gt;

        _p->no_gt = false;

        // an empty list is permitted, so the close is checked first
        if (!d_internal_check(_p, D_AWK_TOK_RPAREN))
        {
            do
            {
                // abandon the parse when an argument failed
                if (!d_internal_push(_p, node, d_internal_expr(_p)))
                {
                    d_internal_node_free(node);
                    return NULL;
                }
            }
            while (d_internal_accept(_p, D_AWK_TOK_COMMA));
        }

        _p->no_gt = outer_no_gt;

        // abandon the parse when the list is not closed
        if (!d_internal_expect(_p, D_AWK_TOK_RPAREN))
        {
            d_internal_node_free(node);
            return NULL;
        }

        return node;
    }

    // a plain name, possibly subscripted
    if (d_internal_check(_p, D_AWK_TOK_NAME))
    {
        struct d_awk_node* name = d_internal_node(_p, D_AWK_N_VAR);

        // abandon the parse when the node could not be held
        if ((!name) || (!d_internal_set_text(_p, name)))
        {
            d_internal_node_free(name);
            return NULL;
        }

        d_internal_advance(_p);

        // a bracket makes it a subscript rather than a scalar
        if (d_internal_accept(_p, D_AWK_TOK_LBRACKET))
        {
            struct d_awk_node* node = d_internal_node(_p, D_AWK_N_INDEX);

            // abandon the parse when the node could not be held
            if (!node)
            {
                d_internal_node_free(name);
                return NULL;
            }

            node->a = name;

            const bool outer_no_gt = _p->no_gt;

            _p->no_gt = false;

            do
            {
                // abandon the parse when a subscript part failed
                if (!d_internal_push(_p, node, d_internal_expr(_p)))
                {
                    d_internal_node_free(node);
                    return NULL;
                }
            }
            while (d_internal_accept(_p, D_AWK_TOK_COMMA));

            _p->no_gt = outer_no_gt;

            // abandon the parse when the subscript is not closed
            if (!d_internal_expect(_p, D_AWK_TOK_RBRACKET))
            {
                d_internal_node_free(node);
                return NULL;
            }

            return node;
        }

        return name;
    }

    // getline, in the four forms that do not read from a pipe.  The tree is
    // built now and rejected by the interpreter, so a program using getline
    // still parses and the grammar can be exercised against real corpora.
    if (d_internal_check(_p, D_AWK_TOK_GETLINE))
    {
        struct d_awk_node* node = d_internal_node(_p, D_AWK_N_GETLINE);

        // abandon the parse when the node could not be held
        if (!node)
        {
            return NULL;
        }

        d_internal_advance(_p);

        // an optional target follows, and it must be assignable
        if ( (d_internal_check(_p, D_AWK_TOK_NAME)) ||
             (d_internal_check(_p, D_AWK_TOK_DOLLAR)) )
        {
            node->a = d_internal_postfix(_p);

            // abandon the parse when the target is not assignable
            if (!d_internal_is_lvalue(node->a))
            {
                d_internal_error(_p, "getline requires a variable or field");
                d_internal_node_free(node);
                return NULL;
            }
        }

        // a `<` here opens a source file rather than comparing
        if (d_internal_accept(_p, D_AWK_TOK_LT))
        {
            node->op = D_AWK_TOK_LT;
            node->b  = d_internal_concat(_p);

            // abandon the parse when the source failed
            if (!node->b)
            {
                d_internal_node_free(node);
                return NULL;
            }
        }

        return node;
    }

    d_internal_error(_p, "expected an expression");

    return NULL;
}


/*
d_internal_postfix
  Attaches any postfix increment or decrement to a primary.

Parameter(s):
  _p: the parser.
Return:
  The node, or NULL on failure.
*/
static struct d_awk_node*
d_internal_postfix(
    struct d_internal_parser* _p
)
{
    struct d_awk_node* node = d_internal_primary(_p);

    // abandon the parse when the operand failed
    if (!node)
    {
        return NULL;
    }

    // a postfix operator applies only to something assignable
    while ( (d_internal_is_lvalue(node))              &&
            ((d_internal_check(_p, D_AWK_TOK_INCR)) ||
             (d_internal_check(_p, D_AWK_TOK_DECR))) )
    {
        struct d_awk_node* wrapper = d_internal_node(_p, D_AWK_N_POSTINCR);

        // abandon the parse when the node could not be held
        if (!wrapper)
        {
            d_internal_node_free(node);
            return NULL;
        }

        wrapper->op = _p->token.kind;
        wrapper->a  = node;
        node        = wrapper;

        d_internal_advance(_p);
    }

    return node;
}


/*
d_internal_power
  Parses exponentiation, which associates to the right and binds tighter than
  unary minus.

Parameter(s):
  _p: the parser.
Return:
  The node, or NULL on failure.
*/
static struct d_awk_node*
d_internal_power(
    struct d_internal_parser* _p
)
{
    struct d_awk_node* left = d_internal_postfix(_p);

    // abandon the parse when the base failed
    if (!left)
    {
        return NULL;
    }

    // right association is expressed by recursing rather than looping
    if (d_internal_check(_p, D_AWK_TOK_CARET))
    {
        struct d_awk_node* node = d_internal_node(_p, D_AWK_N_BINARY);

        // abandon the parse when the node could not be held
        if (!node)
        {
            d_internal_node_free(left);
            return NULL;
        }

        node->op = D_AWK_TOK_CARET;
        node->a  = left;
        d_internal_advance(_p);
        node->b = d_internal_unary(_p);

        // abandon the parse when the exponent failed
        if (!node->b)
        {
            d_internal_node_free(node);
            return NULL;
        }

        return node;
    }

    return left;
}


/*
d_internal_binary_left
  Parses a left-associative rung of the precedence ladder.

Parameter(s):
  _p:     the parser.
  _next:  the parser for the rung below.
  _kinds: the operator kinds this rung accepts.
  _count: how many kinds there are.
Return:
  The node, or NULL on failure.
*/
static struct d_awk_node*
d_internal_binary_left(
    struct d_internal_parser*    _p,
    struct d_awk_node*         (*_next)(struct d_internal_parser*),
    const enum d_awk_token_kind* _kinds,
    size_t                       _count
)
{
    struct d_awk_node* left = _next(_p);

    // abandon the parse when the first operand failed
    if (!left)
    {
        return NULL;
    }

    // fold each further operand into a left-leaning tree
    while (!_p->at_end)
    {
        bool matched = false;

        for (size_t at = 0; at < _count; ++at)
        {
            if (_p->token.kind == _kinds[at])
            {
                matched = true;
                break;
            }
        }

        // a `>` inside a print argument list is redirection, not comparison
        if ( (matched)                                  &&
             (_p->no_gt)                                &&
             (_p->token.kind == D_AWK_TOK_GT) )
        {
            matched = false;
        }

        // the rung ends at the first token it does not accept
        if (!matched)
        {
            break;
        }

        struct d_awk_node* node = d_internal_node(_p, D_AWK_N_BINARY);

        // abandon the parse when the node could not be held
        if (!node)
        {
            d_internal_node_free(left);
            return NULL;
        }

        node->op = _p->token.kind;
        node->a  = left;
        d_internal_advance(_p);
        node->b = _next(_p);

        // abandon the parse when the operand failed
        if (!node->b)
        {
            d_internal_node_free(node);
            return NULL;
        }

        left = node;
    }

    return left;
}


/*
d_internal_unary
  Parses logical negation and arithmetic sign.
NOTE:
  Unary sits BELOW exponentiation in awk, so `-2 ^ 2` is `-(2 ^ 2)` and not
  `(-2) ^ 2`.  Putting the sign in the primary parser gets this wrong, which
  is why it has a rung of its own.

Parameter(s):
  _p: the parser.
Return:
  The node, or NULL on failure.
*/
static struct d_awk_node*
d_internal_unary(
    struct d_internal_parser* _p
)
{
    // a sign or negation wraps whatever the rung below produces
    if ( (d_internal_check(_p, D_AWK_TOK_NOT))   ||
         (d_internal_check(_p, D_AWK_TOK_MINUS)) ||
         (d_internal_check(_p, D_AWK_TOK_PLUS)) )
    {
        struct d_awk_node* node = d_internal_node(_p, D_AWK_N_UNARY);

        // abandon the parse when the node could not be held
        if (!node)
        {
            return NULL;
        }

        node->op = _p->token.kind;
        d_internal_advance(_p);
        node->a = d_internal_unary(_p);

        // abandon the parse when the operand failed
        if (!node->a)
        {
            d_internal_node_free(node);
            return NULL;
        }

        return node;
    }

    return d_internal_power(_p);
}


/*
d_internal_multiplicative
  Parses multiplication, division and remainder.

Parameter(s):
  _p: the parser.
Return:
  The node, or NULL on failure.
*/
static struct d_awk_node*
d_internal_multiplicative(
    struct d_internal_parser* _p
)
{
    static const enum d_awk_token_kind kinds[] =
    {
        D_AWK_TOK_STAR, D_AWK_TOK_SLASH, D_AWK_TOK_PERCENT
    };

    return d_internal_binary_left(_p, d_internal_unary, kinds, 3u);
}


/*
d_internal_additive
  Parses addition and subtraction.

Parameter(s):
  _p: the parser.
Return:
  The node, or NULL on failure.
*/
static struct d_awk_node*
d_internal_additive(
    struct d_internal_parser* _p
)
{
    static const enum d_awk_token_kind kinds[] =
    {
        D_AWK_TOK_PLUS, D_AWK_TOK_MINUS
    };

    return d_internal_binary_left(_p, d_internal_multiplicative, kinds, 2u);
}


/*
d_internal_concat
  Parses concatenation, which is juxtaposition and has no operator.

Parameter(s):
  _p: the parser.
Return:
  The node, or NULL on failure.
*/
static struct d_awk_node*
d_internal_concat(
    struct d_internal_parser* _p
)
{
    struct d_awk_node* left = d_internal_additive(_p);

    // abandon the parse when the first operand failed
    if (!left)
    {
        return NULL;
    }

    // another operand follows whenever the next token could begin one
    while ((!_p->at_end) && (d_internal_starts_expr(_p->token.kind)))
    {
        // `in` is a keyword, not the start of an operand
        if (_p->token.kind == D_AWK_TOK_IN)
        {
            break;
        }

        struct d_awk_node* node = d_internal_node(_p, D_AWK_N_CONCAT);

        // abandon the parse when the node could not be held
        if (!node)
        {
            d_internal_node_free(left);
            return NULL;
        }

        node->a = left;
        node->b = d_internal_additive(_p);

        // abandon the parse when the operand failed
        if (!node->b)
        {
            d_internal_node_free(node);
            return NULL;
        }

        left = node;
    }

    return left;
}


/*
d_internal_getline_pipe
  Parses `command | getline [lvalue]`.
NOTE:
  Placement is empirical rather than read off the POSIX precedence table,
  which appears to put Getline below the relational group.  A reference awk
  runs `while ("cmd" | getline line > 0)` as `("cmd" | getline line) > 0`, so
  the pipe binds tighter than a comparison and this rung sits directly above
  concatenation.
CAUTION:
  Two tokens of lookahead are required.  A bare `|` after an expression is
  print redirection, and only a following `getline` makes it this operator.

Parameter(s):
  _p: the parser.
Return:
  The node, or NULL on failure.
*/
static struct d_awk_node*
d_internal_getline_pipe(
    struct d_internal_parser* _p
)
{
    struct d_awk_node* left = d_internal_concat(_p);

    // abandon the parse when the command failed
    if (!left)
    {
        return NULL;
    }

    // only `| getline` is this operator; a bare `|` belongs to print
    while ( (d_internal_check(_p, D_AWK_TOK_PIPE)) &&
            (d_internal_peek_kind(_p) == D_AWK_TOK_GETLINE) )
    {
        struct d_awk_node* node = d_internal_node(_p, D_AWK_N_GETLINE);

        // abandon the parse when the node could not be held
        if (!node)
        {
            d_internal_node_free(left);
            return NULL;
        }

        node->op = D_AWK_TOK_PIPE;
        node->b  = left;

        d_internal_advance(_p);  // consume the pipe
        d_internal_advance(_p);  // consume getline

        // an optional target follows, and it must be assignable
        if ( (d_internal_check(_p, D_AWK_TOK_NAME)) ||
             (d_internal_check(_p, D_AWK_TOK_DOLLAR)) )
        {
            node->a = d_internal_postfix(_p);

            // abandon the parse when the target is not assignable
            if (!d_internal_is_lvalue(node->a))
            {
                d_internal_error(_p, "getline requires a variable or field");
                d_internal_node_free(node);
                return NULL;
            }
        }

        left = node;
    }

    return left;
}


/*
d_internal_relational
  Parses one comparison.
NOTE:
  The relational operators do not associate in awk, so at most one is read
  here and `a < b < c` is rejected rather than being read as a comparison
  against a truth value.

Parameter(s):
  _p: the parser.
Return:
  The node, or NULL on failure.
*/
static struct d_awk_node*
d_internal_relational(
    struct d_internal_parser* _p
)
{
    struct d_awk_node* left = d_internal_getline_pipe(_p);

    // abandon the parse when the first operand failed
    if (!left)
    {
        return NULL;
    }

    const enum d_awk_token_kind kind = _p->token.kind;
    const bool                  is_relational =
        ( (kind == D_AWK_TOK_LT) || (kind == D_AWK_TOK_LE) ||
          (kind == D_AWK_TOK_NE) || (kind == D_AWK_TOK_EQ) ||
          (kind == D_AWK_TOK_GE) ||
          ((kind == D_AWK_TOK_GT) && (!_p->no_gt)) );

    // a single comparison is read, and no more
    if ((_p->at_end) || (!is_relational))
    {
        return left;
    }

    struct d_awk_node* node = d_internal_node(_p, D_AWK_N_BINARY);

    // abandon the parse when the node could not be held
    if (!node)
    {
        d_internal_node_free(left);
        return NULL;
    }

    node->op = kind;
    node->a  = left;
    d_internal_advance(_p);
    node->b = d_internal_getline_pipe(_p);

    // abandon the parse when the operand failed
    if (!node->b)
    {
        d_internal_node_free(node);
        return NULL;
    }

    return node;
}


/*
d_internal_match
  Parses the regular-expression matching operators.

Parameter(s):
  _p: the parser.
Return:
  The node, or NULL on failure.
*/
static struct d_awk_node*
d_internal_match(
    struct d_internal_parser* _p
)
{
    struct d_awk_node* left = d_internal_relational(_p);

    // abandon the parse when the subject failed
    if (!left)
    {
        return NULL;
    }

    while ( (d_internal_check(_p, D_AWK_TOK_MATCH)) ||
            (d_internal_check(_p, D_AWK_TOK_NOMATCH)) )
    {
        struct d_awk_node* node = d_internal_node(_p, D_AWK_N_MATCH);

        // abandon the parse when the node could not be held
        if (!node)
        {
            d_internal_node_free(left);
            return NULL;
        }

        node->op = _p->token.kind;
        node->a  = left;
        d_internal_advance(_p);
        node->b = d_internal_relational(_p);

        // abandon the parse when the pattern failed
        if (!node->b)
        {
            d_internal_node_free(node);
            return NULL;
        }

        left = node;
    }

    return left;
}


/*
d_internal_in
  Parses the membership operator.

Parameter(s):
  _p: the parser.
Return:
  The node, or NULL on failure.
*/
static struct d_awk_node*
d_internal_in(
    struct d_internal_parser* _p
)
{
    struct d_awk_node* left = d_internal_match(_p);

    // abandon the parse when the subscript failed
    if (!left)
    {
        return NULL;
    }

    while (d_internal_accept(_p, D_AWK_TOK_IN))
    {
        struct d_awk_node* node = d_internal_node(_p, D_AWK_N_IN);

        // abandon the parse when the node could not be held
        if ((!node) || (!d_internal_push(_p, node, left)))
        {
            d_internal_node_free(node);
            d_internal_node_free(left);
            return NULL;
        }

        node->b = d_internal_node(_p, D_AWK_N_VAR);

        // abandon the parse when the array name failed
        if ((!node->b) || (!d_internal_set_text(_p, node->b)))
        {
            d_internal_node_free(node);
            return NULL;
        }

        // the right operand of `in` must be a name
        if (!d_internal_expect(_p, D_AWK_TOK_NAME))
        {
            d_internal_node_free(node);
            return NULL;
        }

        left = node;
    }

    return left;
}


/*
d_internal_and
  Parses logical conjunction.

Parameter(s):
  _p: the parser.
Return:
  The node, or NULL on failure.
*/
static struct d_awk_node*
d_internal_and(
    struct d_internal_parser* _p
)
{
    static const enum d_awk_token_kind kinds[] = { D_AWK_TOK_AND };

    return d_internal_binary_left(_p, d_internal_in, kinds, 1u);
}


/*
d_internal_or
  Parses logical disjunction.

Parameter(s):
  _p: the parser.
Return:
  The node, or NULL on failure.
*/
static struct d_awk_node*
d_internal_or(
    struct d_internal_parser* _p
)
{
    static const enum d_awk_token_kind kinds[] = { D_AWK_TOK_OR };

    return d_internal_binary_left(_p, d_internal_and, kinds, 1u);
}


/*
d_internal_ternary
  Parses the conditional operator, which associates to the right.

Parameter(s):
  _p: the parser.
Return:
  The node, or NULL on failure.
*/
static struct d_awk_node*
d_internal_ternary(
    struct d_internal_parser* _p
)
{
    struct d_awk_node* condition = d_internal_or(_p);

    // abandon the parse when the condition failed
    if (!condition)
    {
        return NULL;
    }

    // a question mark makes it conditional, and the arms recurse
    if (!d_internal_accept(_p, D_AWK_TOK_QUESTION))
    {
        return condition;
    }

    struct d_awk_node* node = d_internal_node(_p, D_AWK_N_TERNARY);

    // abandon the parse when the node could not be held
    if (!node)
    {
        d_internal_node_free(condition);
        return NULL;
    }

    node->a = condition;
    node->b = d_internal_ternary(_p);

    // abandon the parse when the first arm failed or the colon is missing
    if ((!node->b) || (!d_internal_expect(_p, D_AWK_TOK_COLON)))
    {
        d_internal_node_free(node);
        return NULL;
    }

    node->c = d_internal_ternary(_p);

    // abandon the parse when the second arm failed
    if (!node->c)
    {
        d_internal_node_free(node);
        return NULL;
    }

    return node;
}


/*
d_internal_expr
  Parses an expression, assignment included.

Parameter(s):
  _p: the parser.
Return:
  The node, or NULL on failure.
*/
static struct d_awk_node*
d_internal_expr(
    struct d_internal_parser* _p
)
{
    struct d_awk_node* left = d_internal_ternary(_p);

    // abandon the parse when the left side failed
    if (!left)
    {
        return NULL;
    }

    const enum d_awk_token_kind kind = _p->token.kind;
    const bool                  is_assign =
        ( (kind == D_AWK_TOK_ASSIGN)     || (kind == D_AWK_TOK_ADD_ASSIGN) ||
          (kind == D_AWK_TOK_SUB_ASSIGN) || (kind == D_AWK_TOK_MUL_ASSIGN) ||
          (kind == D_AWK_TOK_DIV_ASSIGN) || (kind == D_AWK_TOK_MOD_ASSIGN) ||
          (kind == D_AWK_TOK_POW_ASSIGN) );

    // an assignment is recognised only when the left side accepts one
    if ((_p->at_end) || (!is_assign))
    {
        return left;
    }

    // the target of an assignment must be assignable
    if (!d_internal_is_lvalue(left))
    {
        d_internal_error(_p, "assignment requires a variable or field");
        d_internal_node_free(left);
        return NULL;
    }

    struct d_awk_node* node = d_internal_node(_p, D_AWK_N_ASSIGN);

    // abandon the parse when the node could not be held
    if (!node)
    {
        d_internal_node_free(left);
        return NULL;
    }

    node->op = kind;
    node->a  = left;
    d_internal_advance(_p);
    node->b = d_internal_expr(_p);

    // abandon the parse when the value failed
    if (!node->b)
    {
        d_internal_node_free(node);
        return NULL;
    }

    return node;
}


//==============================================================================
// 4.  STATEMENTS
//==============================================================================


/*
d_internal_simple_or_block
  Parses the body of a control structure, skipping leading newlines.

Parameter(s):
  _p: the parser.
Return:
  The node, or NULL on failure.
*/
static struct d_awk_node*
d_internal_simple_or_block(
    struct d_internal_parser* _p
)
{
    d_internal_skip_terminators(_p);

    return d_internal_statement(_p);
}


/*
d_internal_print
  Parses a print or printf statement.

Parameter(s):
  _p:       the parser.
  _is_printf: true for printf, false for print.
Return:
  The node, or NULL on failure.
*/
static struct d_awk_node*
d_internal_print(
    struct d_internal_parser* _p,
    bool                      _is_printf
)
{
    struct d_awk_node* node = d_internal_node(_p,
                                              _is_printf ? D_AWK_N_PRINTF
                                                         : D_AWK_N_PRINT);

    // abandon the parse when the node could not be held
    if (!node)
    {
        return NULL;
    }

    d_internal_advance(_p);

    const bool saved = _p->no_gt;

    _p->no_gt = true;

    // an argument list is optional for print and required for printf
    if ( (!_p->at_end)                                    &&
         (_p->token.kind != D_AWK_TOK_NEWLINE)            &&
         (_p->token.kind != D_AWK_TOK_SEMI)               &&
         (_p->token.kind != D_AWK_TOK_RBRACE)             &&
         (_p->token.kind != D_AWK_TOK_GT)                 &&
         (_p->token.kind != D_AWK_TOK_APPEND)             &&
         (_p->token.kind != D_AWK_TOK_PIPE) )
    {
        do
        {
            // abandon the parse when an argument failed
            if (!d_internal_push(_p, node, d_internal_expr(_p)))
            {
                _p->no_gt = saved;
                d_internal_node_free(node);
                return NULL;
            }
        }
        while (d_internal_accept(_p, D_AWK_TOK_COMMA));
    }

    // a redirection target is parsed but not yet executable
    if ( (d_internal_check(_p, D_AWK_TOK_GT))     ||
         (d_internal_check(_p, D_AWK_TOK_APPEND)) ||
         (d_internal_check(_p, D_AWK_TOK_PIPE)) )
    {
        node->op = _p->token.kind;
        d_internal_advance(_p);
        node->b = d_internal_expr(_p);

        // abandon the parse when the target failed
        if (!node->b)
        {
            _p->no_gt = saved;
            d_internal_node_free(node);
            return NULL;
        }
    }

    _p->no_gt = saved;

    // `print (a, b)` parses as one parenthesised list; flatten it so the
    // arguments are the list's elements rather than the list itself
    if ( (node->count == 1u) &&
         (node->list[0]->kind == D_AWK_N_GROUPLIST) )
    {
        struct d_awk_node* const group = node->list[0];

        free(node->list);
        node->list     = group->list;
        node->count    = group->count;
        node->capacity = group->capacity;

        group->list  = NULL;
        group->count = 0;
        d_internal_node_free(group);
    }

    return node;
}


/*
d_internal_statement
  Parses one statement.

Parameter(s):
  _p: the parser.
Return:
  The node, or NULL on failure.
*/
static struct d_awk_node*
d_internal_statement(
    struct d_internal_parser* _p
)
{
    // a brace opens a nested block
    if (d_internal_check(_p, D_AWK_TOK_LBRACE))
    {
        return d_internal_block(_p);
    }

    // an empty statement is a bare terminator
    if ( (d_internal_check(_p, D_AWK_TOK_SEMI)) ||
         (d_internal_check(_p, D_AWK_TOK_NEWLINE)) )
    {
        struct d_awk_node* node = d_internal_node(_p, D_AWK_N_BLOCK);

        d_internal_advance(_p);

        return node;
    }

    if (d_internal_check(_p, D_AWK_TOK_PRINT))
    {
        return d_internal_print(_p, false);
    }

    if (d_internal_check(_p, D_AWK_TOK_PRINTF))
    {
        return d_internal_print(_p, true);
    }

    // if, with an optional else arm
    if (d_internal_accept(_p, D_AWK_TOK_IF))
    {
        struct d_awk_node* node = d_internal_node(_p, D_AWK_N_IF);

        // abandon the parse when the node could not be held
        if ((!node) || (!d_internal_expect(_p, D_AWK_TOK_LPAREN)))
        {
            d_internal_node_free(node);
            return NULL;
        }

        node->a = d_internal_expr(_p);

        // abandon the parse when the condition failed or is not closed
        if ((!node->a) || (!d_internal_expect(_p, D_AWK_TOK_RPAREN)))
        {
            d_internal_node_free(node);
            return NULL;
        }

        node->b = d_internal_simple_or_block(_p);

        // abandon the parse when the consequent failed
        if (!node->b)
        {
            d_internal_node_free(node);
            return NULL;
        }

        const size_t mark = _p->token.line;

        (void)mark;
        d_internal_skip_terminators(_p);

        // an else arm is optional and may follow any number of terminators
        if (d_internal_accept(_p, D_AWK_TOK_ELSE))
        {
            node->c = d_internal_simple_or_block(_p);

            // abandon the parse when the alternative failed
            if (!node->c)
            {
                d_internal_node_free(node);
                return NULL;
            }
        }

        return node;
    }

    // while
    if (d_internal_accept(_p, D_AWK_TOK_WHILE))
    {
        struct d_awk_node* node = d_internal_node(_p, D_AWK_N_WHILE);

        // abandon the parse when the node could not be held
        if ((!node) || (!d_internal_expect(_p, D_AWK_TOK_LPAREN)))
        {
            d_internal_node_free(node);
            return NULL;
        }

        node->a = d_internal_expr(_p);

        // abandon the parse when the condition failed or is not closed
        if ((!node->a) || (!d_internal_expect(_p, D_AWK_TOK_RPAREN)))
        {
            d_internal_node_free(node);
            return NULL;
        }

        node->b = d_internal_simple_or_block(_p);

        // abandon the parse when the body failed
        if (!node->b)
        {
            d_internal_node_free(node);
            return NULL;
        }

        return node;
    }

    // do ... while
    if (d_internal_accept(_p, D_AWK_TOK_DO))
    {
        struct d_awk_node* node = d_internal_node(_p, D_AWK_N_DO);

        // abandon the parse when the node could not be held
        if (!node)
        {
            return NULL;
        }

        node->a = d_internal_simple_or_block(_p);

        // abandon the parse when the body failed
        if (!node->a)
        {
            d_internal_node_free(node);
            return NULL;
        }

        d_internal_skip_terminators(_p);

        // abandon the parse when the trailing condition is malformed
        if ( (!d_internal_expect(_p, D_AWK_TOK_WHILE)) ||
             (!d_internal_expect(_p, D_AWK_TOK_LPAREN)) )
        {
            d_internal_node_free(node);
            return NULL;
        }

        node->b = d_internal_expr(_p);

        // abandon the parse when the condition failed or is not closed
        if ((!node->b) || (!d_internal_expect(_p, D_AWK_TOK_RPAREN)))
        {
            d_internal_node_free(node);
            return NULL;
        }

        return node;
    }

    // for, in both its three-clause and its membership forms
    if (d_internal_accept(_p, D_AWK_TOK_FOR))
    {
        // abandon the parse when the header does not open
        if (!d_internal_expect(_p, D_AWK_TOK_LPAREN))
        {
            return NULL;
        }

        // the membership form needs two tokens of lookahead to recognise,
        // since `for (i` may still turn out to be the three-clause form
        if ( (d_internal_check(_p, D_AWK_TOK_NAME)) &&
             (d_internal_peek_kind(_p) == D_AWK_TOK_IN) )
        {
            struct d_awk_node* name = d_internal_node(_p, D_AWK_N_VAR);

            // abandon the parse when the node could not be held
            if ((!name) || (!d_internal_set_text(_p, name)))
            {
                d_internal_node_free(name);
                return NULL;
            }

            d_internal_advance(_p);

            // the membership form is taken only when `in` actually follows
            if (d_internal_accept(_p, D_AWK_TOK_IN))
            {
                struct d_awk_node* node = d_internal_node(_p, D_AWK_N_FORIN);

                // abandon the parse when the node could not be held
                if (!node)
                {
                    d_internal_node_free(name);
                    return NULL;
                }

                node->a = name;
                node->b = d_internal_node(_p, D_AWK_N_VAR);

                // abandon the parse when the array name failed
                if ((!node->b) || (!d_internal_set_text(_p, node->b)))
                {
                    d_internal_node_free(node);
                    return NULL;
                }

                // abandon the parse when the header is malformed
                if ( (!d_internal_expect(_p, D_AWK_TOK_NAME)) ||
                     (!d_internal_expect(_p, D_AWK_TOK_RPAREN)) )
                {
                    d_internal_node_free(node);
                    return NULL;
                }

                node->d = d_internal_simple_or_block(_p);

                // abandon the parse when the body failed
                if (!node->d)
                {
                    d_internal_node_free(node);
                    return NULL;
                }

                return node;
            }

            d_internal_error(_p, "expected `in` after the loop variable");
            d_internal_node_free(name);

            return NULL;
        }

        struct d_awk_node* node = d_internal_node(_p, D_AWK_N_FOR);

        // abandon the parse when the node could not be held
        if (!node)
        {
            return NULL;
        }

        // each of the three clauses is optional
        if (!d_internal_check(_p, D_AWK_TOK_SEMI))
        {
            node->a = d_internal_expr(_p);

            // abandon the parse when the initialiser failed
            if (!node->a)
            {
                d_internal_node_free(node);
                return NULL;
            }
        }

        // abandon the parse when the first separator is missing
        if (!d_internal_expect(_p, D_AWK_TOK_SEMI))
        {
            d_internal_node_free(node);
            return NULL;
        }

        if (!d_internal_check(_p, D_AWK_TOK_SEMI))
        {
            node->b = d_internal_expr(_p);

            // abandon the parse when the condition failed
            if (!node->b)
            {
                d_internal_node_free(node);
                return NULL;
            }
        }

        // abandon the parse when the second separator is missing
        if (!d_internal_expect(_p, D_AWK_TOK_SEMI))
        {
            d_internal_node_free(node);
            return NULL;
        }

        if (!d_internal_check(_p, D_AWK_TOK_RPAREN))
        {
            node->c = d_internal_expr(_p);

            // abandon the parse when the step failed
            if (!node->c)
            {
                d_internal_node_free(node);
                return NULL;
            }
        }

        // abandon the parse when the header is not closed
        if (!d_internal_expect(_p, D_AWK_TOK_RPAREN))
        {
            d_internal_node_free(node);
            return NULL;
        }

        node->d = d_internal_simple_or_block(_p);

        // abandon the parse when the body failed
        if (!node->d)
        {
            d_internal_node_free(node);
            return NULL;
        }

        return node;
    }

    // the jump statements, three of which take an optional expression
    if ( (d_internal_check(_p, D_AWK_TOK_NEXT))     ||
         (d_internal_check(_p, D_AWK_TOK_NEXTFILE)) ||
         (d_internal_check(_p, D_AWK_TOK_BREAK))    ||
         (d_internal_check(_p, D_AWK_TOK_CONTINUE)) ||
         (d_internal_check(_p, D_AWK_TOK_EXIT))     ||
         (d_internal_check(_p, D_AWK_TOK_RETURN)) )
    {
        enum d_awk_node_kind kind = D_AWK_N_NEXT;

        switch (_p->token.kind)
        {
            case D_AWK_TOK_NEXTFILE: kind = D_AWK_N_NEXTFILE; break;
            case D_AWK_TOK_BREAK:    kind = D_AWK_N_BREAK;    break;
            case D_AWK_TOK_CONTINUE: kind = D_AWK_N_CONTINUE; break;
            case D_AWK_TOK_EXIT:     kind = D_AWK_N_EXIT;     break;
            case D_AWK_TOK_RETURN:   kind = D_AWK_N_RETURN;   break;
            default:                 kind = D_AWK_N_NEXT;     break;
        }

        struct d_awk_node* node = d_internal_node(_p, kind);

        // abandon the parse when the node could not be held
        if (!node)
        {
            return NULL;
        }

        d_internal_advance(_p);

        // exit and return take an optional value
        if ( ((kind == D_AWK_N_EXIT) || (kind == D_AWK_N_RETURN)) &&
             (!_p->at_end)                                        &&
             (d_internal_starts_expr(_p->token.kind)) )
        {
            node->a = d_internal_expr(_p);

            // abandon the parse when the value failed
            if (!node->a)
            {
                d_internal_node_free(node);
                return NULL;
            }
        }

        return node;
    }

    // delete, of one element or of a whole array
    if (d_internal_accept(_p, D_AWK_TOK_DELETE))
    {
        struct d_awk_node* name = d_internal_node(_p, D_AWK_N_VAR);

        // abandon the parse when the node could not be held
        if ((!name) || (!d_internal_set_text(_p, name)))
        {
            d_internal_node_free(name);
            return NULL;
        }

        // abandon the parse when no array name follows
        if (!d_internal_expect(_p, D_AWK_TOK_NAME))
        {
            d_internal_node_free(name);
            return NULL;
        }

        // a subscript makes it an element deletion rather than a clear
        if (d_internal_accept(_p, D_AWK_TOK_LBRACKET))
        {
            struct d_awk_node* node = d_internal_node(_p, D_AWK_N_DELETE);

            // abandon the parse when the node could not be held
            if (!node)
            {
                d_internal_node_free(name);
                return NULL;
            }

            node->a = name;

            do
            {
                // abandon the parse when a subscript part failed
                if (!d_internal_push(_p, node, d_internal_expr(_p)))
                {
                    d_internal_node_free(node);
                    return NULL;
                }
            }
            while (d_internal_accept(_p, D_AWK_TOK_COMMA));

            // abandon the parse when the subscript is not closed
            if (!d_internal_expect(_p, D_AWK_TOK_RBRACKET))
            {
                d_internal_node_free(node);
                return NULL;
            }

            return node;
        }

        struct d_awk_node* node = d_internal_node(_p, D_AWK_N_DELETE_ALL);

        // abandon the parse when the node could not be held
        if (!node)
        {
            d_internal_node_free(name);
            return NULL;
        }

        node->a = name;

        return node;
    }

    struct d_awk_node* node = d_internal_node(_p, D_AWK_N_EXPR_STMT);

    // abandon the parse when the node could not be held
    if (!node)
    {
        return NULL;
    }

    node->a = d_internal_expr(_p);

    // abandon the parse when the expression failed
    if (!node->a)
    {
        d_internal_node_free(node);
        return NULL;
    }

    return node;
}


/*
d_internal_block
  Parses a braced statement list.

Parameter(s):
  _p: the parser.
Return:
  The node, or NULL on failure.
*/
static struct d_awk_node*
d_internal_block(
    struct d_internal_parser* _p
)
{
    struct d_awk_node* node = d_internal_node(_p, D_AWK_N_BLOCK);

    // abandon the parse when the node could not be held
    if ((!node) || (!d_internal_expect(_p, D_AWK_TOK_LBRACE)))
    {
        d_internal_node_free(node);
        return NULL;
    }

    d_internal_skip_terminators(_p);

    // statements accumulate until the closing brace
    while ((!_p->at_end) && (!d_internal_check(_p, D_AWK_TOK_RBRACE)))
    {
        // abandon the parse when a statement failed
        if (!d_internal_push(_p, node, d_internal_statement(_p)))
        {
            d_internal_node_free(node);
            return NULL;
        }

        d_internal_skip_terminators(_p);
    }

    // abandon the parse when the block is not closed
    if (!d_internal_expect(_p, D_AWK_TOK_RBRACE))
    {
        d_internal_node_free(node);
        return NULL;
    }

    return node;
}


//==============================================================================
// 5.  PROGRAM
//==============================================================================


/*
d_internal_add_rule
  Appends a rule to the program.

Parameter(s):
  _p:    the parser.
  _rule: the rule to append.
Return:
  A boolean value corresponding to either:
  - true, if the rule was appended, or
  - false, otherwise.
*/
static bool
d_internal_add_rule(
    struct d_internal_parser* _p,
    const struct d_awk_rule*  _rule
)
{
    struct d_awk_program* program = _p->program;

    // grow the rule vector when it is full
    if (program->rule_count == program->rule_capacity)
    {
        const size_t capacity = (program->rule_capacity == 0)
                              ? 8u
                              : (program->rule_capacity * 2u);

        struct d_awk_rule* grown =
            realloc(program->rules, capacity * sizeof(*grown));

        // report the failure rather than dropping the rule
        if (!grown)
        {
            d_internal_error(_p, "out of memory");
            return false;
        }

        program->rules         = grown;
        program->rule_capacity = capacity;
    }

    program->rules[program->rule_count++] = *_rule;

    return true;
}


/*
d_internal_function
  Parses a function definition.

Parameter(s):
  _p: the parser.
Return:
  A boolean value corresponding to either:
  - true, if the definition was parsed, or
  - false, otherwise.
*/
static bool
d_internal_function(
    struct d_internal_parser* _p
)
{
    d_internal_advance(_p);  // consume `function`

    // the name may lex as a call when the parenthesis is adjacent
    if ( (!d_internal_check(_p, D_AWK_TOK_NAME)) &&
         (!d_internal_check(_p, D_AWK_TOK_FUNC_NAME)) )
    {
        d_internal_error(_p, "expected a function name");
        return false;
    }

    struct d_awk_function entry;

    memset(&entry, 0, sizeof(entry));

    entry.name = malloc(_p->token.length + 1u);

    // abandon the parse when the name could not be held
    if (!entry.name)
    {
        d_internal_error(_p, "out of memory");
        return false;
    }

    memcpy(entry.name, _p->token.text, _p->token.length);
    entry.name[_p->token.length] = '\0';
    d_internal_advance(_p);

    // abandon the parse when the parameter list does not open
    if (!d_internal_expect(_p, D_AWK_TOK_LPAREN))
    {
        free(entry.name);
        return false;
    }

    // an empty parameter list is permitted
    if (!d_internal_check(_p, D_AWK_TOK_RPAREN))
    {
        do
        {
            // a parameter must be a plain name
            if (!d_internal_check(_p, D_AWK_TOK_NAME))
            {
                d_internal_error(_p, "expected a parameter name");
                break;
            }

            char** grown = realloc(entry.params,
                                   (entry.param_count + 1u) *
                                   sizeof(*grown));

            // abandon the parse when the list could not grow
            if (!grown)
            {
                d_internal_error(_p, "out of memory");
                break;
            }

            entry.params = grown;
            entry.params[entry.param_count] = malloc(_p->token.length + 1u);

            // abandon the parse when the name could not be held
            if (!entry.params[entry.param_count])
            {
                d_internal_error(_p, "out of memory");
                break;
            }

            memcpy(entry.params[entry.param_count],
                   _p->token.text,
                   _p->token.length);
            entry.params[entry.param_count][_p->token.length] = '\0';
            entry.param_count++;

            d_internal_advance(_p);
        }
        while (d_internal_accept(_p, D_AWK_TOK_COMMA));
    }

    // abandon the parse when the list is not closed
    if ((_p->failed) || (!d_internal_expect(_p, D_AWK_TOK_RPAREN)))
    {
        for (size_t at = 0; at < entry.param_count; ++at)
        {
            free(entry.params[at]);
        }

        free(entry.params);
        free(entry.name);

        return false;
    }

    d_internal_skip_terminators(_p);
    entry.body = d_internal_block(_p);

    // abandon the parse when the body failed
    if (!entry.body)
    {
        for (size_t at = 0; at < entry.param_count; ++at)
        {
            free(entry.params[at]);
        }

        free(entry.params);
        free(entry.name);

        return false;
    }

    struct d_awk_program* program = _p->program;

    // grow the function vector when it is full
    if (program->function_count == program->function_capacity)
    {
        const size_t capacity = (program->function_capacity == 0)
                              ? 4u
                              : (program->function_capacity * 2u);

        struct d_awk_function* grown =
            realloc(program->functions, capacity * sizeof(*grown));

        // report the failure rather than dropping the definition
        if (!grown)
        {
            d_internal_error(_p, "out of memory");
            return false;
        }

        program->functions        = grown;
        program->function_capacity = capacity;
    }

    program->functions[program->function_count++] = entry;

    return true;
}


/*
d_awk_parse
  Parses a complete awk program.

Parameter(s):
  _source: the program text.
  _length: its length in bytes.
  _origin: the name used in diagnostics; may be NULL.
Return:
  The program, or NULL on failure, with the diagnostic in d_awk_parse_error.
*/
struct d_awk_program*
d_awk_parse(
    const char* _source,
    size_t      _length,
    const char* _origin
)
{
    g_error[0] = '\0';

    struct d_internal_parser parser;

    memset(&parser, 0, sizeof(parser));

    parser.origin  = _origin;
    parser.lexer   = d_awk_lexer_new(_source, _length, _origin);
    parser.program = calloc(1, sizeof(*parser.program));

    // abandon the parse when either handle could not be held
    if ((!parser.lexer) || (!parser.program))
    {
        d_awk_lexer_free(parser.lexer);
        free(parser.program);
        snprintf(g_error, sizeof(g_error), "out of memory");

        return NULL;
    }

    d_internal_advance(&parser);
    d_internal_skip_terminators(&parser);

    // items accumulate until the source is exhausted
    while ((!parser.at_end) && (!parser.failed))
    {
        // a function definition is not a rule
        if (d_internal_check(&parser, D_AWK_TOK_FUNCTION))
        {
            (void)d_internal_function(&parser);
            d_internal_skip_terminators(&parser);
            continue;
        }

        struct d_awk_rule rule;

        memset(&rule, 0, sizeof(rule));
        rule.kind = D_AWK_RULE_MAIN;

        // BEGIN and END take an action and no pattern
        if (d_internal_accept(&parser, D_AWK_TOK_BEGIN))
        {
            rule.kind = D_AWK_RULE_BEGIN;
        }
        else if (d_internal_accept(&parser, D_AWK_TOK_END))
        {
            rule.kind = D_AWK_RULE_END;
        }
        else if (!d_internal_check(&parser, D_AWK_TOK_LBRACE))
        {
            rule.pattern = d_internal_expr(&parser);

            // abandon the parse when the pattern failed
            if (!rule.pattern)
            {
                break;
            }

            // a comma makes it a range, whose second pattern ends it
            if (d_internal_accept(&parser, D_AWK_TOK_COMMA))
            {
                rule.pattern_end = d_internal_expr(&parser);

                // abandon the parse when the second pattern failed
                if (!rule.pattern_end)
                {
                    d_internal_node_free(rule.pattern);
                    break;
                }
            }
        }

        // an action is optional for a main rule and required for the others
        if (d_internal_check(&parser, D_AWK_TOK_LBRACE))
        {
            rule.action = d_internal_block(&parser);

            // abandon the parse when the action failed
            if (!rule.action)
            {
                d_internal_node_free(rule.pattern);
                d_internal_node_free(rule.pattern_end);
                break;
            }
        }
        else if (rule.kind != D_AWK_RULE_MAIN)
        {
            d_internal_error(&parser, "BEGIN and END require an action");
            d_internal_node_free(rule.pattern);
            break;
        }

        // abandon the parse when the rule could not be recorded
        if (!d_internal_add_rule(&parser, &rule))
        {
            d_internal_node_free(rule.pattern);
            d_internal_node_free(rule.pattern_end);
            d_internal_node_free(rule.action);
            break;
        }

        d_internal_skip_terminators(&parser);
    }

    const bool failed = parser.failed;

    free(parser.saved_text);
    free(parser.token_text);
    d_awk_lexer_free(parser.lexer);

    // report the failure rather than returning a partial program
    if (failed)
    {
        d_awk_program_free(parser.program);
        return NULL;
    }

    return parser.program;
}


/*
d_awk_program_free
  Releases a parsed program.

Parameter(s):
  _program: the program to release; may be NULL.
Return:
  none.
*/
void
d_awk_program_free(
    struct d_awk_program* _program
)
{
    if (_program)
    {
        for (size_t at = 0; at < _program->rule_count; ++at)
        {
            d_internal_node_free(_program->rules[at].pattern);
            d_internal_node_free(_program->rules[at].pattern_end);
            d_internal_node_free(_program->rules[at].action);
        }

        for (size_t at = 0; at < _program->function_count; ++at)
        {
            for (size_t p = 0; p < _program->functions[at].param_count; ++p)
            {
                free(_program->functions[at].params[p]);
            }

            free(_program->functions[at].params);
            free(_program->functions[at].name);
            d_internal_node_free(_program->functions[at].body);
        }

        free(_program->rules);
        free(_program->functions);
        free(_program);
    }

    return;
}


/*
d_awk_parse_error
  Returns the diagnostic from the most recent failed parse.

Parameter(s):
  none.
Return:
  The diagnostic, or NULL when the last parse succeeded.
*/
const char*
d_awk_parse_error(void)
{
    return (g_error[0] == '\0') ? NULL : g_error;
}
