/*******************************************************************************
* djinterp [c/parse]                                              t_parse_expr.c
*
* Expression core v2's C suite: the term, the signature, the parser, the
* renderer, folds and walks, each through the C API alone.
*   Four languages are built here on the core, each a table and nothing
* more: math's (calls, indexing, the transpose and the factorial, bars,
* literals with rows, chains, implied multiplication), the command language's
* (non-associative comparison, flattening pipelines and sequences, a command
* as a custom role), an awk-like one's (concatenation by juxtaposition, `?:`,
* `if ... then ... else`, `++` both ways, a role that a mode turns off, a call
* that needs no space before its bracket), and dawk's, from lane H's review
* of 11-05 (its whole ladder, `| getline`, `++` only after an lvalue, a
* bracketed list before `in`). Every case of the design's tables is a case
* here, with the term it must give.
*   A section is a function and its name, as the foundation's suites list
* them; the suite moves into DTest 2.0 once its API is frozen (Friday 11-06).
*
*
* path:      /tests/djinterp/c/modules/parse/expr/t_parse_expr.c
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.05
*******************************************************************************/
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <stdio.h>    // printf, snprintf
#include <string.h>   // memset, strcmp, strlen, strncmp
// djinterp
#include "../../../../../../inc/djinterp/c/modules/parse/expr/expr.h"  // the core
#include "../tests_section.h"                                         // dj_tests_section
// re_std
#include "../../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// 1.  FIXTURES
//==============================================================================


// 1.1    Checks
//------------------------------------------------------------------------------
/*
d_tests_expr_expect
  Reports a check that failed, with what it was and where, and folds it into
the section's result.
*/
static bool
d_tests_expr_expect(
    bool        _held,
    const char* _what,
    int         _line
)
{
    if (!_held)
    {
        printf("    failed (line %d): %s\n", _line, _what);
    }

    return _held;
}


// 1.2    The token classes and operators of the three languages
//------------------------------------------------------------------------------
enum dj_tests_expr_class
{
    C_NUM = 1, C_NAME, C_PLUS, C_MINUS, C_STAR, C_SLASH, C_CARET, C_BANG,
    C_QUOTE, C_EQ, C_NE, C_LT, C_LE, C_GT, C_GE, C_AND, C_OR, C_NOT, C_MOD,
    C_LP, C_RP, C_LB, C_RB, C_BAR, C_COMMA, C_SEMI, C_EQEQ, C_PIPE, C_OPTION,
    C_STRING, C_QMARK, C_COLON, C_IF, C_THEN, C_ELSE, C_INC, C_TILDE,
    C_DOLLAR, C_ANDAND, C_OROR, C_NOMATCH, C_MATCH, C_DEC, C_PERCENT, C_IN,
    C_GETLINE, C_APPEND, C_DIVEQ
};

enum dj_tests_expr_op
{
    O_ADD = 1, O_SUB, O_MUL, O_DIV, O_MOD, O_POW, O_NEG, O_POS, O_FACT,
    O_TRANS, O_EQ, O_NE, O_LT, O_LE, O_GT, O_GE, O_AND, O_OR, O_NOT, O_CALL,
    O_INDEX, O_VECTOR, O_MATRIX, O_ROW, O_GROUP, O_ABS, O_SEQ, O_PIPE,
    O_EQEQ, O_CMD, O_CONCAT, O_COND, O_IF, O_PREINC, O_POSTINC, O_MATCH,
    O_ASSIGN, O_IN, O_NOMATCH, O_PIPEGET, O_GETLINE, O_FIELD, O_PREDEC,
    O_POSTDEC, O_COUNT
};

// OP_NAMES
//   constant: each operator's name, as the expected terms spell it.
static const char* const OP_NAMES[O_COUNT] =
{
    "?", "add", "sub", "mul", "div", "mod", "pow", "neg", "pos", "fact",
    "trans", "eq", "ne", "lt", "le", "gt", "ge", "and", "or", "not", "call",
    "index", "vector", "matrix", "row", "group", "abs", "seq", "pipe", "eqeq",
    "cmd", "concat", "cond", "if", "preinc", "postinc", "match", "assign",
    "in", "nomatch", "pipeget", "getline", "field", "predec", "postdec"
};

// dj_tests_expr_spelling
//   struct: one spelling the test lexer knows, and its class.
struct dj_tests_expr_spelling
{
    const char* text;
    uint32_t    cls;
};

// SYMBOLS
//   constant: the punctuation the lexer reads, longest first where two
// begin alike.
static const struct dj_tests_expr_spelling SYMBOLS[] =
{
    { "==", C_EQEQ }, { "!=", C_NE }, { "<=", C_LE }, { ">=", C_GE },
    { "++", C_INC }, { "=~", C_TILDE }, { "+", C_PLUS }, { "-", C_MINUS },
    { "*", C_STAR }, { "/", C_SLASH }, { "^", C_CARET }, { "!", C_BANG },
    { "'", C_QUOTE }, { "=", C_EQ }, { "<", C_LT }, { ">", C_GT },
    { "(", C_LP }, { ")", C_RP }, { "[", C_LB }, { "]", C_RB },
    { "|", C_BAR }, { ",", C_COMMA }, { ";", C_SEMI }, { "?", C_QMARK },
    { ":", C_COLON }
};

// DAWK_SYMBOLS
//   constant: the punctuation only dawk's texts are read with, before the
// rest: math's `||x|` is two bars, dawk's `a || b` one operator.
static const struct dj_tests_expr_spelling DAWK_SYMBOLS[] =
{
    { "&&", C_ANDAND }, { "||", C_OROR }, { "!~", C_NOMATCH }, { "--", C_DEC },
    { ">>", C_APPEND }, { "/=", C_DIVEQ }, { "~", C_MATCH }, { "$", C_DOLLAR },
    { "%", C_PERCENT }
};

// WORDS
//   constant: the words the lexer reads as classes of their own.
static const struct dj_tests_expr_spelling WORDS[] =
{
    { "and", C_AND }, { "or", C_OR }, { "not", C_NOT }, { "mod", C_MOD },
    { "if", C_IF }, { "then", C_THEN }, { "else", C_ELSE }, { "in", C_IN },
    { "getline", C_GETLINE }
};

// ATOM_NAMES
//   constant: the atoms the lexer gives values to, by their index: a leaf's
// value is its text's place here, so a term read back from a rendering can
// be compared with the one rendered.
static const char* const ATOM_NAMES[] =
{
    "a", "b", "c", "d", "x", "y", "z", "f", "g", "v", "A", "B", "n", "sin",
    "0", "1", "2", "3", "4", "ls", "grep", "echo", "done", "-l", "--all",
    "i", "j", "arr", "cmd", "line"
};

#define ATOM_COUNT  (sizeof(ATOM_NAMES) / sizeof(ATOM_NAMES[0]))

// Lexer flags
//   constant: OPTIONS: `-l` and `--all` are options, the command language's;
// DAWK: dawk's punctuation is read too.
#define D_TESTS_EXPR_LEX_OPTIONS    0x01u
#define D_TESTS_EXPR_LEX_DAWK       0x02u


// 1.3    The lexer
//------------------------------------------------------------------------------
/*
d_tests_expr_atom_value
  An atom's value: its place in ATOM_NAMES, or past them all.
*/
static uint32_t
d_tests_expr_atom_value(
    const char* _text,
    size_t      _length
)
{
    for (uint32_t i = 0u; i < (uint32_t)ATOM_COUNT; ++i)
    {
        if ( (strlen(ATOM_NAMES[i]) == _length) &&
             (strncmp(ATOM_NAMES[i], _text, _length) == 0) )
        {
            return i;
        }
    }

    return (uint32_t)ATOM_COUNT;
}

/*
d_tests_expr_lex_symbol
  The first spelling of a table that the text at `_i` begins with: its class
into the token, and its length, or 0 where none.
*/
static size_t
d_tests_expr_lex_symbol(
    const char*                          _t,
    size_t                               _i,
    size_t                               _n,
    const struct dj_tests_expr_spelling* _table,
    size_t                               _count,
    struct dj_parse_expr_token*          _k
)
{
    for (size_t s = 0u; s < _count; ++s)
    {
        const size_t length = strlen(_table[s].text);

        if ( (_i + length <= _n) &&
             (strncmp(_table[s].text, _t + _i, length) == 0) )
        {
            _k->cls = _table[s].cls;

            return length;
        }
    }

    return 0u;
}

/*
d_tests_expr_lex_one
  Reads one token at `_i`: a number, a name or word, an option (`-l`,
`--all`, where options are on), or punctuation, longest first (dawk's first
where it is read). Returns its length, 0 for a byte nothing begins with.
*/
static size_t
d_tests_expr_lex_one(
    const char*                 _t,
    size_t                      _i,
    size_t                      _n,
    uint32_t                    _lexer,
    struct dj_parse_expr_token* _k
)
{
    size_t j = _i;

    if ( (_t[_i] >= '0') && (_t[_i] <= '9') )
    {
        while ( (j < _n) && ( ( (_t[j] >= '0') && (_t[j] <= '9') ) ||
                              (_t[j] == '.') ) )
        {
            ++j;
        }

        _k->cls = C_NUM;
    }
    else if ( ( (_t[_i] >= 'a') && (_t[_i] <= 'z') ) ||
              ( (_t[_i] >= 'A') && (_t[_i] <= 'Z') ) )
    {
        while ( (j < _n) && ( ( (_t[j] >= 'a') && (_t[j] <= 'z') ) ||
                              ( (_t[j] >= 'A') && (_t[j] <= 'Z') ) ||
                              ( (_t[j] >= '0') && (_t[j] <= '9') ) ) )
        {
            ++j;
        }

        _k->cls = C_NAME;

        for (size_t w = 0u; w < (sizeof(WORDS) / sizeof(WORDS[0])); ++w)
        {
            if ( (strlen(WORDS[w].text) == j - _i) &&
                 (strncmp(WORDS[w].text, _t + _i, j - _i) == 0) )
            {
                _k->cls = WORDS[w].cls;
            }
        }
    }
    else if ( ((_lexer & D_TESTS_EXPR_LEX_OPTIONS) != 0u) &&
              (_t[_i] == '-') && (_i + 1u < _n) &&
              (_t[_i + 1u] != ' ') )
    {
        while ( (j < _n) && (_t[j] != ' ') )
        {
            ++j;
        }

        _k->cls = C_OPTION;
    }
    else
    {
        if ((_lexer & D_TESTS_EXPR_LEX_DAWK) != 0u)
        {
            j = _i + d_tests_expr_lex_symbol(_t, _i, _n, DAWK_SYMBOLS,
                                             sizeof(DAWK_SYMBOLS) /
                                             sizeof(DAWK_SYMBOLS[0]), _k);
        }

        if (j == _i)
        {
            j = _i + d_tests_expr_lex_symbol(_t, _i, _n, SYMBOLS,
                                             sizeof(SYMBOLS) /
                                             sizeof(SYMBOLS[0]), _k);
        }
    }

    _k->value = d_tests_expr_atom_value(_t + _i, j - _i);

    return j - _i;
}

/*
d_tests_expr_lex
  The whole text into tokens, each marked where a space came before it.
Returns how many, or -1 at a byte nothing begins with or past the room.
*/
static int
d_tests_expr_lex(
    const char*                 _text,
    uint32_t                    _lexer,
    struct dj_parse_expr_token* _out,
    int                         _max
)
{
    const size_t n     = strlen(_text);
    size_t       i     = 0u;
    int          count = 0;
    bool         space = false;

    while (i < n)
    {
        // a text longer than the room is a fault of the test, said so
        if (count >= _max)
        {
            return -1;
        }

        if (_text[i] == ' ')
        {
            space = true;
            ++i;

            continue;
        }

        struct dj_parse_expr_token k;

        memset(&k, 0, sizeof(k));
        k.offset = (uint32_t)i;
        k.flags  = (space) ? D_PARSE_EXPR_TOKEN_SPACE_BEFORE : 0u;

        const size_t length = d_tests_expr_lex_one(_text, i, n, _lexer, &k);

        if (length == 0u)
        {
            return -1;
        }

        k.length     = (uint32_t)length;
        _out[count++] = k;
        i            += length;
        space         = false;
    }

    return count;
}


// 1.4    Terms as text
//------------------------------------------------------------------------------
// dj_tests_expr_text
//   struct: a buffer an s-expression is written into.
struct dj_tests_expr_text
{
    char   out[512];
    size_t length;
};

/*
d_tests_expr_put
  Appends to the buffer, cutting what does not fit.
*/
static void
d_tests_expr_put(
    struct dj_tests_expr_text* _b,
    const char*                _s
)
{
    const size_t length = strlen(_s);

    for (size_t i = 0u; ( (i < length) && (_b->length + 1u < 512u) ); ++i)
    {
        _b->out[_b->length++] = _s[i];
    }

    _b->out[_b->length] = '\0';

    return;
}

/*
d_tests_expr_sexpr_enter
  A walk's entry: a leaf as its atom's name, a hole as `_`, an application as
its operator's name and an open bracket; a comma before every child but the
first. A chain's join is marked `*`.
*/
static int
d_tests_expr_sexpr_enter(
    void*                            _ctx,
    const struct dj_parse_expr_term* _term,
    uint32_t                         _node,
    uint32_t                         _parent
)
{
    struct dj_tests_expr_text*       b    = (struct dj_tests_expr_text*)_ctx;
    const struct dj_parse_expr_node* node = &_term->nodes[_node];

    // a comma between siblings
    if ( (_parent != D_PARSE_EXPR_NO_NODE) &&
         (_term->args[_term->nodes[_parent].first] != _node) )
    {
        d_tests_expr_put(b, ", ");
    }

    if (node->kind == D_PARSE_EXPR_NODE_LEAF)
    {
        d_tests_expr_put(b, (node->first < ATOM_COUNT)
                            ? ATOM_NAMES[node->first]
                            : "#");
    }
    else if (node->kind == D_PARSE_EXPR_NODE_HOLE)
    {
        d_tests_expr_put(b, "_");
    }
    else
    {
        d_tests_expr_put(b, (node->op < O_COUNT) ? OP_NAMES[node->op] : "?");
        d_tests_expr_put(b, ((node->flags & D_PARSE_EXPR_NODE_CHAIN) != 0u)
                            ? "*(" : "(");
    }

    return D_PARSE_EXPR_WALK_CONTINUE;
}

/*
d_tests_expr_sexpr_leave
  A walk's exit: an application's close bracket.
*/
static int
d_tests_expr_sexpr_leave(
    void*                            _ctx,
    const struct dj_parse_expr_term* _term,
    uint32_t                         _node,
    uint32_t                         _parent
)
{
    (void)_parent;

    if (_term->nodes[_node].kind == D_PARSE_EXPR_NODE_APPLY)
    {
        d_tests_expr_put((struct dj_tests_expr_text*)_ctx, ")");
    }

    return D_PARSE_EXPR_WALK_CONTINUE;
}

/*
d_tests_expr_sexpr
  A subtree as an s-expression, by a walk.
*/
static const char*
d_tests_expr_sexpr(
    const struct dj_parse_expr_term* _term,
    uint32_t                         _root,
    struct dj_tests_expr_text*       _b
)
{
    _b->length = 0u;
    _b->out[0] = '\0';

    if (_root == D_PARSE_EXPR_NO_NODE)
    {
        return "<none>";
    }

    (void)d_parse_expr_walk(_term, _root, d_tests_expr_sexpr_enter,
                            d_tests_expr_sexpr_leave, _b);

    return _b->out;
}


// 1.5    A parse, whole
//------------------------------------------------------------------------------
// dj_tests_expr_run
//   struct: one parse and what it found, over storage of its own.
struct dj_tests_expr_run
{
    struct dj_parse_expr_token   tokens[4096];
    struct dj_parse_diagnostic   items[32];
    char                         text[4096];
    struct dj_parse_diag_sink    sink;
    struct dj_parse_expr_term    term;
    struct dj_parse_expr_result  result;
    struct dj_parse_expr_options options;
    int                          status;
};

/*
d_tests_expr_parse
  Lexes and parses a text by a signature, the options given or the defaults,
the text given to the options for messages.
*/
static int
d_tests_expr_parse(
    struct dj_tests_expr_run*             _run,
    const struct dj_parse_expr_signature* _sig,
    const char*                           _text,
    uint32_t                              _lexer
)
{
    struct dj_parse_expr_source       source;
    struct dj_parse_expr_array_source state;
    const int                         count = d_tests_expr_lex(_text,
                                                               _lexer,
                                                               _run->tokens,
                                                               4096);

    d_parse_diag_sink_init(&_run->sink, _run->items, 32u, _run->text,
                           (uint32_t)sizeof(_run->text));
    d_parse_expr_term_init_heap(&_run->term);
    d_parse_expr_source_array(&source, &state, _run->tokens,
                              (count > 0) ? (uint32_t)count : 0u,
                              (uint32_t)strlen(_text));
    _run->options.text        = _text;
    _run->options.text_length = strlen(_text);
    _run->status = d_parse_expr_parse(_sig, &source, &_run->options,
                                      &_run->term, &_run->sink,
                                      &_run->result);

    return _run->status;
}


// 1.6    The languages
//------------------------------------------------------------------------------
/*
d_tests_expr_mixfix_of
  A mixfix descriptor of two parts, zeroed but for them.
*/
static struct dj_parse_expr_mixfix
d_tests_expr_mixfix_of(
    uint8_t     _lead,
    uint32_t    _open,
    const char* _open_spelling,
    uint32_t    _close,
    const char* _close_spelling
)
{
    struct dj_parse_expr_mixfix m;

    memset(&m, 0, sizeof(m));
    m.lead             = _lead;
    m.parts            = 2u;
    m.part[0]          = _open;
    m.part[1]          = _close;
    m.part_spelling[0] = _open_spelling;
    m.part_spelling[1] = _close_spelling;

    return m;
}

/*
d_tests_expr_list_of
  Makes a mixfix descriptor's hole a list split at commas, in bounds.
*/
static void
d_tests_expr_list_of(
    struct dj_parse_expr_mixfix* _m,
    uint16_t                     _min,
    uint16_t                     _max
)
{
    _m->list_hole          = 1u;
    _m->separator          = C_COMMA;
    _m->separator_spelling = ",";
    _m->min_items          = _min;
    _m->max_items          = _max;

    return;
}

/*
d_tests_expr_two_numbers
  Math's juxtaposition rule: never between two numbers (S196).
*/
static bool
d_tests_expr_two_numbers(
    void*                             _ctx,
    const struct dj_parse_expr_term*  _term,
    uint32_t                          _left,
    const struct dj_parse_expr_token* _token
)
{
    (void)_ctx;

    return (!( (_term->nodes[_left].kind == D_PARSE_EXPR_NODE_LEAF) &&
               (_term->nodes[_left].op == C_NUM)                    &&
               (_token->cls == C_NUM) ));
}

/*
d_tests_expr_math_ops
  Math's prefix, infix and postfix operators and their tokens, as the
design's section 17.1 has them.
*/
static bool
d_tests_expr_math_ops(
    struct dj_parse_expr_signature* _s
)
{
    static const uint32_t    RELATIONS[6] = { O_EQ, O_NE, O_LT, O_LE, O_GT,
                                              O_GE };
    static const uint32_t    REL_CLASS[6] = { C_EQ, C_NE, C_LT, C_LE, C_GT,
                                              C_GE };
    static const char* const REL_TEXT[6]  = { "=", "!=", "<", "<=", ">",
                                              ">=" };
    bool                     ok           = true;

    ok = ( (ok) &&
           (d_parse_expr_define_atom(_s, C_NUM, 0u)) &&
           (d_parse_expr_define_atom(_s, C_NAME,
                                     D_PARSE_EXPR_ROLE_CALLABLE)) &&
           (d_parse_expr_define_op(_s, O_NEG, D_PARSE_EXPR_FORM_PREFIX, 70,
                                   D_PARSE_EXPR_ASSOC_RIGHT, 0u, "-")) &&
           (d_parse_expr_define_op(_s, O_POS, D_PARSE_EXPR_FORM_PREFIX, 70,
                                   D_PARSE_EXPR_ASSOC_RIGHT, 0u, "+")) &&
           (d_parse_expr_define_op(_s, O_ADD, D_PARSE_EXPR_FORM_INFIX, 50,
                                   D_PARSE_EXPR_ASSOC_LEFT, 0u, "+")) &&
           (d_parse_expr_define_op(_s, O_SUB, D_PARSE_EXPR_FORM_INFIX, 50,
                                   D_PARSE_EXPR_ASSOC_LEFT, 0u, "-")) &&
           (d_parse_expr_define_op(_s, O_MUL, D_PARSE_EXPR_FORM_INFIX, 60,
                                   D_PARSE_EXPR_ASSOC_LEFT, 0u, "*")) &&
           (d_parse_expr_define_op(_s, O_DIV, D_PARSE_EXPR_FORM_INFIX, 60,
                                   D_PARSE_EXPR_ASSOC_LEFT, 0u, "/")) &&
           (d_parse_expr_define_op(_s, O_MOD, D_PARSE_EXPR_FORM_INFIX, 60,
                                   D_PARSE_EXPR_ASSOC_LEFT,
                                   D_PARSE_EXPR_OP_WORD, "mod")) &&
           (d_parse_expr_define_op(_s, O_POW, D_PARSE_EXPR_FORM_INFIX, 80,
                                   D_PARSE_EXPR_ASSOC_RIGHT, 0u, "^")) &&
           (d_parse_expr_define_op(_s, O_TRANS, D_PARSE_EXPR_FORM_POSTFIX, 75,
                                   D_PARSE_EXPR_ASSOC_LEFT, 0u, "'")) &&
           (d_parse_expr_define_op(_s, O_FACT, D_PARSE_EXPR_FORM_POSTFIX, 90,
                                   D_PARSE_EXPR_ASSOC_LEFT, 0u, "!")) &&
           (d_parse_expr_define_op(_s, O_AND, D_PARSE_EXPR_FORM_INFIX, 20,
                                   D_PARSE_EXPR_ASSOC_LEFT,
                                   D_PARSE_EXPR_OP_WORD, "and")) &&
           (d_parse_expr_define_op(_s, O_OR, D_PARSE_EXPR_FORM_INFIX, 10,
                                   D_PARSE_EXPR_ASSOC_LEFT,
                                   D_PARSE_EXPR_OP_WORD, "or")) &&
           (d_parse_expr_define_op(_s, O_NOT, D_PARSE_EXPR_FORM_PREFIX, 30,
                                   D_PARSE_EXPR_ASSOC_RIGHT,
                                   D_PARSE_EXPR_OP_WORD, "not")) );

    // the relations: a chain at 40, joined by `and` (S213, S214)
    for (int i = 0; i < 6; ++i)
    {
        ok = ( (ok) &&
               (d_parse_expr_define_chain(_s, RELATIONS[i], 40, O_AND,
                                          REL_TEXT[i])) &&
               (d_parse_expr_define_token(_s, REL_CLASS[i],
                                          D_PARSE_EXPR_OPERATOR,
                                          RELATIONS[i], 0u)) );
    }

    return ( (ok) &&
             (d_parse_expr_define_token(_s, C_MINUS, D_PARSE_EXPR_OPERAND,
                                        O_NEG, 0u)) &&
             (d_parse_expr_define_token(_s, C_PLUS, D_PARSE_EXPR_OPERAND,
                                        O_POS, 0u)) &&
             (d_parse_expr_define_token(_s, C_MINUS, D_PARSE_EXPR_OPERATOR,
                                        O_SUB, 0u)) &&
             (d_parse_expr_define_token(_s, C_PLUS, D_PARSE_EXPR_OPERATOR,
                                        O_ADD, 0u)) &&
             (d_parse_expr_define_token(_s, C_STAR, D_PARSE_EXPR_OPERATOR,
                                        O_MUL, 0u)) &&
             (d_parse_expr_define_token(_s, C_SLASH, D_PARSE_EXPR_OPERATOR,
                                        O_DIV, 0u)) &&
             (d_parse_expr_define_token(_s, C_MOD, D_PARSE_EXPR_OPERATOR,
                                        O_MOD, 0u)) &&
             (d_parse_expr_define_token(_s, C_CARET, D_PARSE_EXPR_OPERATOR,
                                        O_POW, 0u)) &&
             (d_parse_expr_define_token(_s, C_QUOTE, D_PARSE_EXPR_OPERATOR,
                                        O_TRANS, 0u)) &&
             (d_parse_expr_define_token(_s, C_BANG, D_PARSE_EXPR_OPERATOR,
                                        O_FACT, 0u)) &&
             (d_parse_expr_define_token(_s, C_AND, D_PARSE_EXPR_OPERATOR,
                                        O_AND, 0u)) &&
             (d_parse_expr_define_token(_s, C_OR, D_PARSE_EXPR_OPERATOR,
                                        O_OR, 0u)) &&
             (d_parse_expr_define_token(_s, C_NOT, D_PARSE_EXPR_OPERAND,
                                        O_NOT, 0u)) );
}

/*
d_tests_expr_math
  Math's whole table on the core: section 17.1's, with implied
multiplication where asked (G5's option).
*/
static bool
d_tests_expr_math(
    struct dj_parse_expr_signature* _s,
    bool                            _implied
)
{
    struct dj_parse_expr_mixfix group  = d_tests_expr_mixfix_of(0u, C_LP, "(",
                                                                 C_RP, ")");
    struct dj_parse_expr_mixfix call   = d_tests_expr_mixfix_of(1u, C_LP, "(",
                                                                 C_RP, ")");
    struct dj_parse_expr_mixfix index  = d_tests_expr_mixfix_of(1u, C_LB, "[",
                                                                 C_RB, "]");
    struct dj_parse_expr_mixfix vector = d_tests_expr_mixfix_of(0u, C_LB, "[",
                                                                 C_RB, "]");
    struct dj_parse_expr_mixfix bars   = d_tests_expr_mixfix_of(0u, C_BAR, "|",
                                                                 C_BAR, "|");

    d_tests_expr_list_of(&call, 0u, D_PARSE_EXPR_ITEMS_UNBOUNDED);
    d_tests_expr_list_of(&index, 1u, 2u);
    d_tests_expr_list_of(&vector, 0u, D_PARSE_EXPR_ITEMS_UNBOUNDED);
    vector.row_separator          = C_SEMI;
    vector.row_separator_spelling = ";";
    vector.row_op                 = O_ROW;
    vector.rows_op                = O_MATRIX;

    d_parse_expr_signature_init_heap(_s);

    return ( (d_tests_expr_math_ops(_s)) &&
             (d_parse_expr_define_mixfix(_s, O_GROUP, 0, 0u,
                                         D_PARSE_EXPR_OP_TRANSPARENT,
                                         &group)) &&
             (d_parse_expr_define_mixfix(_s, O_CALL, 100,
                                         D_PARSE_EXPR_ASSOC_LEFT, 0u,
                                         &call)) &&
             (d_parse_expr_define_mixfix(_s, O_INDEX, 100,
                                         D_PARSE_EXPR_ASSOC_LEFT, 0u,
                                         &index)) &&
             (d_parse_expr_define_mixfix(_s, O_VECTOR, 0, 0u, 0u, &vector)) &&
             (d_parse_expr_define_mixfix(_s, O_ABS, 0, 0u, 0u, &bars)) &&
             (d_parse_expr_define_op(_s, O_ROW, D_PARSE_EXPR_FORM_PREFIX, 0,
                                     0u, 0u, "row")) &&
             (d_parse_expr_define_op(_s, O_MATRIX, D_PARSE_EXPR_FORM_PREFIX, 0,
                                     0u, 0u, "matrix")) &&
             (d_parse_expr_define_token(_s, C_LP, D_PARSE_EXPR_OPERAND,
                                        O_GROUP, 0u)) &&
             (d_parse_expr_define_token(_s, C_LP, D_PARSE_EXPR_OPERATOR,
                                        O_CALL,
                                        D_PARSE_EXPR_ROLE_CALLEE)) &&
             (d_parse_expr_define_token(_s, C_LB, D_PARSE_EXPR_OPERAND,
                                        O_VECTOR, 0u)) &&
             (d_parse_expr_define_token(_s, C_LB, D_PARSE_EXPR_OPERATOR,
                                        O_INDEX, 0u)) &&
             (d_parse_expr_define_token(_s, C_BAR, D_PARSE_EXPR_OPERAND,
                                        O_ABS, 0u)) &&
             ( (!_implied) ||
               (d_parse_expr_juxtapose(_s, O_MUL, d_tests_expr_two_numbers,
                                       NULL)) ) &&
             (d_parse_expr_signature_seal(_s, NULL)) );
}

/*
d_tests_expr_command
  The command language's custom role: a verb takes the options and operands
after it, each a leaf, as djinterp.peg's Command does.
*/
static int
d_tests_expr_command(
    void*                             _ctx,
    struct dj_parse_expr_parser*      _parser,
    const struct dj_parse_expr_token* _token,
    uint32_t                          _left,
    uint32_t*                         _out
)
{
    struct dj_parse_expr_term* term = d_parse_expr_parser_term(_parser);
    uint32_t                   parts[16];
    uint32_t                   count = 0u;
    uint32_t                   end   = _token->offset + _token->length;
    struct dj_parse_expr_token next;

    (void)_ctx;
    (void)_left;

    parts[count++] = d_parse_expr_term_leaf(term, _token->cls, _token->value,
                                            _token->offset, _token->length);

    // arguments, while the next token is one
    while ( (count < 16u)                                            &&
            (d_parse_expr_peek(_parser, D_PARSE_EXPR_OPERATOR, 0u,
                               &next) > 0)                           &&
            ( (next.cls == C_NAME) || (next.cls == C_OPTION) ||
              (next.cls == C_NUM) ) )
    {
        (void)d_parse_expr_take(_parser, D_PARSE_EXPR_OPERATOR, 0u, &next);
        parts[count++] = d_parse_expr_term_leaf(term, next.cls, next.value,
                                                next.offset, next.length);
        end            = next.offset + next.length;
    }

    *_out = d_parse_expr_term_apply(term, O_CMD, parts, count,
                                    _token->offset, end - _token->offset);

    return (*_out != D_PARSE_EXPR_NO_NODE) ? 1 : -1;
}

/*
d_tests_expr_cli
  The command language: djinterp.peg's section III on the core.
*/
static bool
d_tests_expr_cli(
    struct dj_parse_expr_signature* _s
)
{
    struct dj_parse_expr_mixfix group = d_tests_expr_mixfix_of(0u, C_LP, "(",
                                                                C_RP, ")");
    struct dj_parse_expr_role   cmd;
    static const uint32_t       COMPARE[3] = { C_EQEQ, C_LT, C_TILDE };
    static const uint32_t       COMPARE_OP[3] = { O_EQEQ, O_LT, O_MATCH };
    static const char* const    COMPARE_TEXT[3] = { "==", "<", "=~" };
    bool                        ok = true;

    memset(&cmd, 0, sizeof(cmd));
    cmd.cls      = C_NAME;
    cmd.position = D_PARSE_EXPR_OPERAND;
    cmd.kind     = D_PARSE_EXPR_ROLE_CUSTOM;
    cmd.custom   = d_tests_expr_command;

    d_parse_expr_signature_init_heap(_s);

    // the comparisons: 60, non-associative
    for (int i = 0; i < 3; ++i)
    {
        ok = ( (ok) &&
               (d_parse_expr_define_op(_s, COMPARE_OP[i],
                                       D_PARSE_EXPR_FORM_INFIX, 60,
                                       D_PARSE_EXPR_ASSOC_NONE, 0u,
                                       COMPARE_TEXT[i])) &&
               (d_parse_expr_define_token(_s, COMPARE[i],
                                          D_PARSE_EXPR_OPERATOR,
                                          COMPARE_OP[i], 0u)) );
    }

    // the command is an operator only the hook builds: no role begins it
    return ( (ok) &&
             (d_parse_expr_define_role(_s, &cmd)) &&
             (d_parse_expr_define_op(_s, O_CMD, D_PARSE_EXPR_FORM_INFIX, 90,
                                     D_PARSE_EXPR_ASSOC_LEFT,
                                     D_PARSE_EXPR_OP_FLATTEN, "")) &&
             (d_parse_expr_define_atom(_s, C_NUM, 0u)) &&
             (d_parse_expr_define_op(_s, O_SEQ, D_PARSE_EXPR_FORM_INFIX, 10,
                                     D_PARSE_EXPR_ASSOC_LEFT,
                                     D_PARSE_EXPR_OP_FLATTEN, ";")) &&
             (d_parse_expr_define_op(_s, O_PIPE, D_PARSE_EXPR_FORM_INFIX, 20,
                                     D_PARSE_EXPR_ASSOC_LEFT,
                                     D_PARSE_EXPR_OP_FLATTEN, "|")) &&
             (d_parse_expr_define_op(_s, O_OR, D_PARSE_EXPR_FORM_INFIX, 30,
                                     D_PARSE_EXPR_ASSOC_LEFT,
                                     D_PARSE_EXPR_OP_WORD, "or")) &&
             (d_parse_expr_define_op(_s, O_AND, D_PARSE_EXPR_FORM_INFIX, 40,
                                     D_PARSE_EXPR_ASSOC_LEFT,
                                     D_PARSE_EXPR_OP_WORD, "and")) &&
             (d_parse_expr_define_op(_s, O_NOT, D_PARSE_EXPR_FORM_PREFIX, 50,
                                     D_PARSE_EXPR_ASSOC_RIGHT,
                                     D_PARSE_EXPR_OP_WORD, "not")) &&
             (d_parse_expr_define_mixfix(_s, O_GROUP, 0, 0u,
                                         D_PARSE_EXPR_OP_TRANSPARENT,
                                         &group)) &&
             (d_parse_expr_define_token(_s, C_SEMI, D_PARSE_EXPR_OPERATOR,
                                        O_SEQ, 0u)) &&
             (d_parse_expr_define_token(_s, C_BAR, D_PARSE_EXPR_OPERATOR,
                                        O_PIPE, 0u)) &&
             (d_parse_expr_define_token(_s, C_OR, D_PARSE_EXPR_OPERATOR,
                                        O_OR, 0u)) &&
             (d_parse_expr_define_token(_s, C_AND, D_PARSE_EXPR_OPERATOR,
                                        O_AND, 0u)) &&
             (d_parse_expr_define_token(_s, C_NOT, D_PARSE_EXPR_OPERAND,
                                        O_NOT, 0u)) &&
             (d_parse_expr_define_token(_s, C_LP, D_PARSE_EXPR_OPERAND,
                                        O_GROUP, 0u)) &&
             (d_parse_expr_signature_seal(_s, NULL)) );
}

/*
d_tests_expr_awk_mixfix
  The awk-like language's mixfix operators: a group, a call with no space
before its bracket, `?:` and `if ... then ... else`.
*/
static bool
d_tests_expr_awk_mixfix(
    struct dj_parse_expr_signature* _s
)
{
    struct dj_parse_expr_mixfix group = d_tests_expr_mixfix_of(0u, C_LP, "(",
                                                                C_RP, ")");
    struct dj_parse_expr_mixfix call  = d_tests_expr_mixfix_of(1u, C_LP, "(",
                                                                C_RP, ")");
    struct dj_parse_expr_mixfix cond  = d_tests_expr_mixfix_of(1u, C_QMARK,
                                                                "?", C_COLON,
                                                                ":");
    struct dj_parse_expr_mixfix iff   = d_tests_expr_mixfix_of(0u, C_IF, "if",
                                                                C_THEN,
                                                                "then");

    d_tests_expr_list_of(&call, 0u, D_PARSE_EXPR_ITEMS_UNBOUNDED);
    cond.tail             = 1u;
    iff.parts             = 3u;
    iff.part[2]           = C_ELSE;
    iff.part_spelling[2]  = "else";
    iff.tail              = 1u;

    return ( (d_parse_expr_define_mixfix(_s, O_GROUP, 0, 0u,
                                         D_PARSE_EXPR_OP_TRANSPARENT,
                                         &group)) &&
             (d_parse_expr_define_mixfix(_s, O_CALL, 100,
                                         D_PARSE_EXPR_ASSOC_LEFT, 0u,
                                         &call)) &&
             (d_parse_expr_define_mixfix(_s, O_COND, 5,
                                         D_PARSE_EXPR_ASSOC_RIGHT, 0u,
                                         &cond)) &&
             (d_parse_expr_define_mixfix(_s, O_IF, 1,
                                         D_PARSE_EXPR_ASSOC_RIGHT, 0u,
                                         &iff)) &&
             (d_parse_expr_define_token(_s, C_LP, D_PARSE_EXPR_OPERAND,
                                        O_GROUP, 0u)) &&
             (d_parse_expr_define_token(_s, C_LP, D_PARSE_EXPR_OPERATOR,
                                        O_CALL,
                                        ( D_PARSE_EXPR_ROLE_CALLEE |
                                          D_PARSE_EXPR_ROLE_ADJACENT ))) &&
             (d_parse_expr_define_token(_s, C_QMARK, D_PARSE_EXPR_OPERATOR,
                                        O_COND, 0u)) &&
             (d_parse_expr_define_token(_s, C_IF, D_PARSE_EXPR_OPERAND, O_IF,
                                        0u)) );
}

/*
d_tests_expr_awk
  An awk-like language: concatenation by a flattening juxtaposition spelled
"", `>` a relation only in mode 1, `++` both ways.
*/
static bool
d_tests_expr_awk(
    struct dj_parse_expr_signature* _s
)
{
    struct dj_parse_expr_role gt;

    memset(&gt, 0, sizeof(gt));
    gt.cls      = C_GT;
    gt.position = D_PARSE_EXPR_OPERATOR;
    gt.kind     = D_PARSE_EXPR_ROLE_OP;
    gt.op       = O_GT;
    gt.modes    = 1u;

    d_parse_expr_signature_init_heap(_s);

    return ( (d_parse_expr_define_atom(_s, C_NUM, 0u)) &&
             (d_parse_expr_define_atom(_s, C_NAME,
                                       D_PARSE_EXPR_ROLE_CALLABLE)) &&
             (d_parse_expr_define_op(_s, O_CONCAT, D_PARSE_EXPR_FORM_INFIX,
                                     45, D_PARSE_EXPR_ASSOC_LEFT,
                                     D_PARSE_EXPR_OP_FLATTEN, "")) &&
             (d_parse_expr_define_op(_s, O_SUB, D_PARSE_EXPR_FORM_INFIX, 50,
                                     D_PARSE_EXPR_ASSOC_LEFT, 0u, "-")) &&
             (d_parse_expr_define_op(_s, O_NEG, D_PARSE_EXPR_FORM_PREFIX, 80,
                                     D_PARSE_EXPR_ASSOC_RIGHT, 0u, "-")) &&
             (d_parse_expr_define_op(_s, O_GT, D_PARSE_EXPR_FORM_INFIX, 40,
                                     D_PARSE_EXPR_ASSOC_NONE, 0u, ">")) &&
             (d_parse_expr_define_op(_s, O_PREINC, D_PARSE_EXPR_FORM_PREFIX,
                                     90, D_PARSE_EXPR_ASSOC_RIGHT, 0u,
                                     "++")) &&
             (d_parse_expr_define_op(_s, O_POSTINC, D_PARSE_EXPR_FORM_POSTFIX,
                                     95, D_PARSE_EXPR_ASSOC_LEFT, 0u,
                                     "++")) &&
             (d_tests_expr_awk_mixfix(_s)) &&
             (d_parse_expr_define_role(_s, &gt)) &&
             (d_parse_expr_define_token(_s, C_MINUS, D_PARSE_EXPR_OPERATOR,
                                        O_SUB, 0u)) &&
             (d_parse_expr_define_token(_s, C_MINUS, D_PARSE_EXPR_OPERAND,
                                        O_NEG, 0u)) &&
             (d_parse_expr_define_token(_s, C_INC, D_PARSE_EXPR_OPERAND,
                                        O_PREINC, 0u)) &&
             (d_parse_expr_define_token(_s, C_INC, D_PARSE_EXPR_OPERATOR,
                                        O_POSTINC, 0u)) &&
             (d_parse_expr_juxtapose(_s, O_CONCAT, NULL, NULL)) &&
             (d_parse_expr_signature_seal(_s, NULL)) );
}


/*
d_tests_expr_lvalue
  Whether an awk operand can be assigned: an ungrouped name, a field, or an
element.
*/
static bool
d_tests_expr_lvalue(
    const struct dj_parse_expr_term* _term,
    uint32_t                         _node
)
{
    const struct dj_parse_expr_node* node = &_term->nodes[_node];

    // a bracketed operand is a value, not a place
    if ((node->flags & D_PARSE_EXPR_NODE_GROUPED) != 0u)
    {
        return false;
    }

    return ( ( (node->kind == D_PARSE_EXPR_NODE_LEAF) &&
               (node->op == C_NAME) )                        ||
             ( (node->kind == D_PARSE_EXPR_NODE_APPLY) &&
               ( (node->op == O_FIELD) || (node->op == O_INDEX) ) ) );
}

/*
d_tests_expr_dawk_step
  dawk's `++` and `--` after an operand: taken where the operand can be
assigned, and declined elsewhere, so that the token begins the next operand
and is concatenated (lane H, 11-05, point 1: `1 ++i`).
*/
static int
d_tests_expr_dawk_step(
    void*                             _ctx,
    struct dj_parse_expr_parser*      _parser,
    const struct dj_parse_expr_token* _token,
    uint32_t                          _left,
    uint32_t*                         _out
)
{
    struct dj_parse_expr_term* term = d_parse_expr_parser_term(_parser);

    (void)_ctx;

    if (!d_tests_expr_lvalue(term, _left))
    {
        return 0;
    }

    const uint32_t start = term->nodes[_left].offset;

    *_out = d_parse_expr_term_apply(term,
                                    (_token->cls == C_INC) ? O_POSTINC
                                                           : O_POSTDEC,
                                    &_left, 1u, start,
                                    _token->offset + _token->length - start);

    return (*_out != D_PARSE_EXPR_NO_NODE) ? 1 : -1;
}

/*
d_tests_expr_dawk_target
  The optional target of a `getline`: a name, taken where one follows. dawk's
own reads any lvalue, by week 2's sub-parse.
*/
static uint32_t
d_tests_expr_dawk_target(
    struct dj_parse_expr_parser* _parser,
    uint32_t*                    _end
)
{
    struct dj_parse_expr_term* term = d_parse_expr_parser_term(_parser);
    struct dj_parse_expr_token next;

    if ( (d_parse_expr_peek(_parser, D_PARSE_EXPR_OPERAND, 0u,
                            &next) <= 0) ||
         (next.cls != C_NAME) )
    {
        return D_PARSE_EXPR_NO_NODE;
    }

    (void)d_parse_expr_take(_parser, D_PARSE_EXPR_OPERAND, 0u, &next);
    *_end = next.offset + next.length;

    return d_parse_expr_term_leaf(term, next.cls, next.value, next.offset,
                                  next.length);
}

/*
d_tests_expr_dawk_pipe
  `cmd | getline [var]`: a custom role on `|` that names its operator, so it
is given the whole concatenation before it; it declines where no `getline`
follows (lane H's points 3 and 4).
*/
static int
d_tests_expr_dawk_pipe(
    void*                             _ctx,
    struct dj_parse_expr_parser*      _parser,
    const struct dj_parse_expr_token* _token,
    uint32_t                          _left,
    uint32_t*                         _out
)
{
    struct dj_parse_expr_term* term = d_parse_expr_parser_term(_parser);
    struct dj_parse_expr_token next;
    uint32_t                   kids[2];
    uint32_t                   count = 0u;

    (void)_ctx;
    (void)_token;

    if ( (d_parse_expr_peek(_parser, D_PARSE_EXPR_OPERAND, 0u,
                            &next) <= 0) ||
         (next.cls != C_GETLINE) )
    {
        return 0;
    }

    (void)d_parse_expr_take(_parser, D_PARSE_EXPR_OPERAND, 0u, &next);

    uint32_t       end    = next.offset + next.length;
    const uint32_t target = d_tests_expr_dawk_target(_parser, &end);
    const uint32_t start  = term->nodes[_left].offset;

    kids[count++] = _left;

    if (target != D_PARSE_EXPR_NO_NODE)
    {
        kids[count++] = target;
    }

    *_out = d_parse_expr_term_apply(term, O_PIPEGET, kids, count, start,
                                    end - start);

    return (*_out != D_PARSE_EXPR_NO_NODE) ? 1 : -1;
}

/*
d_tests_expr_dawk_getline
  `getline [var]` in operand position; `< file` waits on the sub-parse.
*/
static int
d_tests_expr_dawk_getline(
    void*                             _ctx,
    struct dj_parse_expr_parser*      _parser,
    const struct dj_parse_expr_token* _token,
    uint32_t                          _left,
    uint32_t*                         _out
)
{
    struct dj_parse_expr_term* term   = d_parse_expr_parser_term(_parser);
    uint32_t                   end    = _token->offset + _token->length;
    const uint32_t             target = d_tests_expr_dawk_target(_parser,
                                                                 &end);

    (void)_ctx;
    (void)_left;

    *_out = d_parse_expr_term_apply(term, O_GETLINE, &target,
                                    (target != D_PARSE_EXPR_NO_NODE) ? 1u : 0u,
                                    _token->offset, end - _token->offset);

    return (*_out != D_PARSE_EXPR_NO_NODE) ? 1 : -1;
}

/*
d_tests_expr_dawk_list
  dawk's check: a bracketed list, `(i, j)`, stands only on `in`'s left (lane
H, point 2; `print`'s argument list is the statement parser's).
*/
static bool
d_tests_expr_dawk_list(
    void*                            _ctx,
    const struct dj_parse_expr_term* _term,
    uint32_t                         _node,
    struct dj_parse_diag_sink*       _sink
)
{
    const struct dj_parse_expr_node* node = &_term->nodes[_node];

    (void)_ctx;

    for (uint32_t i = 0u; i < node->count; ++i)
    {
        const struct dj_parse_expr_node* child =
            &_term->nodes[_term->args[node->first + i]];

        if ( (child->kind == D_PARSE_EXPR_NODE_APPLY) &&
             (child->op == O_GROUP)                   &&
             ( (node->op != O_IN) || (i != 0u) ) )
        {
            (void)d_parse_diag_emit(_sink, D_PARSE_SEVERITY_ERROR,
                                    (uint16_t)D_PARSE_DIAG_DOMAIN_USER, 8u,
                                    d_parse_span_make(child->offset,
                                                      child->length),
                                    "a bracketed list stands only before "
                                    "`in`");

            return false;
        }
    }

    return true;
}

// dj_tests_expr_dawk_op
//   struct: one row of dawk's table: an operator, its form, precedence,
// association, flags and spelling, and the class that begins it.
struct dj_tests_expr_dawk_op
{
    uint32_t    op;
    uint8_t     form;
    int32_t     precedence;
    uint8_t     assoc;
    uint16_t    flags;
    const char* spelling;
    uint32_t    cls;
};

// DAWK_OPS
//   constant: the ladder of lane H's point 3, lowest first, as dawk's
// dparse.c has it.
static const struct dj_tests_expr_dawk_op DAWK_OPS[] =
{
    { O_ASSIGN,  D_PARSE_EXPR_FORM_INFIX,   10, D_PARSE_EXPR_ASSOC_RIGHT,
      0u, "=", C_EQ },
    { O_OR,      D_PARSE_EXPR_FORM_INFIX,   30, D_PARSE_EXPR_ASSOC_LEFT,
      0u, "||", C_OROR },
    { O_AND,     D_PARSE_EXPR_FORM_INFIX,   40, D_PARSE_EXPR_ASSOC_LEFT,
      0u, "&&", C_ANDAND },
    { O_IN,      D_PARSE_EXPR_FORM_INFIX,   50, D_PARSE_EXPR_ASSOC_LEFT,
      D_PARSE_EXPR_OP_WORD, "in", C_IN },
    { O_MATCH,   D_PARSE_EXPR_FORM_INFIX,   60, D_PARSE_EXPR_ASSOC_LEFT,
      0u, "~", C_MATCH },
    { O_NOMATCH, D_PARSE_EXPR_FORM_INFIX,   60, D_PARSE_EXPR_ASSOC_LEFT,
      0u, "!~", C_NOMATCH },
    { O_LT,      D_PARSE_EXPR_FORM_INFIX,   70, D_PARSE_EXPR_ASSOC_NONE,
      0u, "<", C_LT },
    { O_LE,      D_PARSE_EXPR_FORM_INFIX,   70, D_PARSE_EXPR_ASSOC_NONE,
      0u, "<=", C_LE },
    { O_NE,      D_PARSE_EXPR_FORM_INFIX,   70, D_PARSE_EXPR_ASSOC_NONE,
      0u, "!=", C_NE },
    { O_EQEQ,    D_PARSE_EXPR_FORM_INFIX,   70, D_PARSE_EXPR_ASSOC_NONE,
      0u, "==", C_EQEQ },
    { O_GT,      D_PARSE_EXPR_FORM_INFIX,   70, D_PARSE_EXPR_ASSOC_NONE,
      0u, ">", C_GT },
    { O_GE,      D_PARSE_EXPR_FORM_INFIX,   70, D_PARSE_EXPR_ASSOC_NONE,
      0u, ">=", C_GE },
    { O_ADD,     D_PARSE_EXPR_FORM_INFIX,  100, D_PARSE_EXPR_ASSOC_LEFT,
      0u, "+", C_PLUS },
    { O_SUB,     D_PARSE_EXPR_FORM_INFIX,  100, D_PARSE_EXPR_ASSOC_LEFT,
      0u, "-", C_MINUS },
    { O_MUL,     D_PARSE_EXPR_FORM_INFIX,  110, D_PARSE_EXPR_ASSOC_LEFT,
      0u, "*", C_STAR },
    { O_DIV,     D_PARSE_EXPR_FORM_INFIX,  110, D_PARSE_EXPR_ASSOC_LEFT,
      0u, "/", C_SLASH },
    { O_MOD,     D_PARSE_EXPR_FORM_INFIX,  110, D_PARSE_EXPR_ASSOC_LEFT,
      0u, "%", C_PERCENT },
    { O_NOT,     D_PARSE_EXPR_FORM_PREFIX, 120, D_PARSE_EXPR_ASSOC_RIGHT,
      0u, "!", C_BANG },
    { O_POS,     D_PARSE_EXPR_FORM_PREFIX, 120, D_PARSE_EXPR_ASSOC_RIGHT,
      0u, "+", C_PLUS },
    { O_NEG,     D_PARSE_EXPR_FORM_PREFIX, 120, D_PARSE_EXPR_ASSOC_RIGHT,
      0u, "-", C_MINUS },
    { O_POW,     D_PARSE_EXPR_FORM_INFIX,  130, D_PARSE_EXPR_ASSOC_RIGHT,
      0u, "^", C_CARET },
    { O_PREINC,  D_PARSE_EXPR_FORM_PREFIX, 140, D_PARSE_EXPR_ASSOC_RIGHT,
      0u, "++", C_INC },
    { O_PREDEC,  D_PARSE_EXPR_FORM_PREFIX, 140, D_PARSE_EXPR_ASSOC_RIGHT,
      0u, "--", C_DEC },
    { O_FIELD,   D_PARSE_EXPR_FORM_PREFIX, 150, D_PARSE_EXPR_ASSOC_RIGHT,
      0u, "$", C_DOLLAR }
};

/*
d_tests_expr_dawk_custom
  Defines a custom role.
*/
static bool
d_tests_expr_dawk_custom(
    struct dj_parse_expr_signature* _s,
    uint32_t                        _class,
    uint8_t                         _position,
    uint16_t                        _flags,
    uint32_t                        _op,
    dj_parse_expr_custom_fn         _hook
)
{
    struct dj_parse_expr_role role;

    memset(&role, 0, sizeof(role));
    role.cls      = _class;
    role.position = _position;
    role.kind     = D_PARSE_EXPR_ROLE_CUSTOM;
    role.flags    = _flags;
    role.op       = _op;
    role.custom   = _hook;

    return d_parse_expr_define_role(_s, &role);
}

/*
d_tests_expr_dawk_brackets
  dawk's brackets: a group that is a list for more than one item, a call with
no space before its bracket, an index after a name, and `?:`.
*/
static bool
d_tests_expr_dawk_brackets(
    struct dj_parse_expr_signature* _s
)
{
    struct dj_parse_expr_mixfix group = d_tests_expr_mixfix_of(0u, C_LP, "(",
                                                                C_RP, ")");
    struct dj_parse_expr_mixfix call  = d_tests_expr_mixfix_of(1u, C_LP, "(",
                                                                C_RP, ")");
    struct dj_parse_expr_mixfix index = d_tests_expr_mixfix_of(1u, C_LB, "[",
                                                                C_RB, "]");
    struct dj_parse_expr_mixfix cond  = d_tests_expr_mixfix_of(1u, C_QMARK,
                                                                "?", C_COLON,
                                                                ":");

    d_tests_expr_list_of(&group, 1u, D_PARSE_EXPR_ITEMS_UNBOUNDED);
    d_tests_expr_list_of(&call, 0u, D_PARSE_EXPR_ITEMS_UNBOUNDED);
    d_tests_expr_list_of(&index, 1u, D_PARSE_EXPR_ITEMS_UNBOUNDED);
    cond.tail = 1u;

    return ( (d_parse_expr_define_mixfix(_s, O_GROUP, 0, 0u,
                                         D_PARSE_EXPR_OP_TRANSPARENT,
                                         &group)) &&
             (d_parse_expr_define_mixfix(_s, O_CALL, 160,
                                         D_PARSE_EXPR_ASSOC_LEFT, 0u,
                                         &call)) &&
             (d_parse_expr_define_mixfix(_s, O_INDEX, 160,
                                         D_PARSE_EXPR_ASSOC_LEFT, 0u,
                                         &index)) &&
             (d_parse_expr_define_mixfix(_s, O_COND, 20,
                                         D_PARSE_EXPR_ASSOC_RIGHT, 0u,
                                         &cond)) &&
             (d_parse_expr_define_token(_s, C_LP, D_PARSE_EXPR_OPERAND,
                                        O_GROUP, 0u)) &&
             (d_parse_expr_define_token(_s, C_LP, D_PARSE_EXPR_OPERATOR,
                                        O_CALL,
                                        ( D_PARSE_EXPR_ROLE_CALLEE |
                                          D_PARSE_EXPR_ROLE_ADJACENT ))) &&
             (d_parse_expr_define_token(_s, C_LB, D_PARSE_EXPR_OPERATOR,
                                        O_INDEX, D_PARSE_EXPR_ROLE_CALLEE)) &&
             (d_parse_expr_define_token(_s, C_QMARK, D_PARSE_EXPR_OPERATOR,
                                        O_COND, 0u)) );
}

/*
d_tests_expr_dawk
  dawk's expressions, from lane H's review of 11-05: the ladder of DAWK_OPS,
concatenation by juxtaposition at 90, `| getline` at 80, the postfix steps
at 140 taken only after an lvalue, `getline` beginning no concatenated
operand, and the check that keeps a bracketed list before `in`.
*/
static bool
d_tests_expr_dawk(
    struct dj_parse_expr_signature* _s
)
{
    bool ok = true;

    d_parse_expr_signature_init_heap(_s);

    for (size_t i = 0u; i < sizeof(DAWK_OPS) / sizeof(DAWK_OPS[0]); ++i)
    {
        const struct dj_tests_expr_dawk_op* r = &DAWK_OPS[i];

        ok = ( (ok) &&
               (d_parse_expr_define_op(_s, r->op, r->form, r->precedence,
                                       r->assoc, r->flags, r->spelling)) &&
               (d_parse_expr_define_token(
                    _s, r->cls,
                    (r->form == D_PARSE_EXPR_FORM_PREFIX)
                    ? D_PARSE_EXPR_OPERAND : D_PARSE_EXPR_OPERATOR,
                    r->op, 0u)) );
    }

    return ( (ok) &&
             (d_parse_expr_define_atom(_s, C_NUM, 0u)) &&
             (d_parse_expr_define_atom(_s, C_NAME,
                                       D_PARSE_EXPR_ROLE_CALLABLE)) &&
             (d_parse_expr_define_op(_s, O_PIPEGET, D_PARSE_EXPR_FORM_INFIX,
                                     80, D_PARSE_EXPR_ASSOC_LEFT, 0u,
                                     "| getline")) &&
             (d_parse_expr_define_op(_s, O_CONCAT, D_PARSE_EXPR_FORM_INFIX,
                                     90, D_PARSE_EXPR_ASSOC_LEFT,
                                     D_PARSE_EXPR_OP_FLATTEN, "")) &&
             (d_parse_expr_define_op(_s, O_POSTINC, D_PARSE_EXPR_FORM_POSTFIX,
                                     140, D_PARSE_EXPR_ASSOC_LEFT, 0u,
                                     "++")) &&
             (d_parse_expr_define_op(_s, O_POSTDEC, D_PARSE_EXPR_FORM_POSTFIX,
                                     140, D_PARSE_EXPR_ASSOC_LEFT, 0u,
                                     "--")) &&
             (d_tests_expr_dawk_brackets(_s)) &&
             (d_tests_expr_dawk_custom(_s, C_INC, D_PARSE_EXPR_OPERATOR, 0u,
                                       O_POSTINC, d_tests_expr_dawk_step)) &&
             (d_tests_expr_dawk_custom(_s, C_DEC, D_PARSE_EXPR_OPERATOR, 0u,
                                       O_POSTDEC, d_tests_expr_dawk_step)) &&
             (d_tests_expr_dawk_custom(_s, C_BAR, D_PARSE_EXPR_OPERATOR, 0u,
                                       O_PIPEGET, d_tests_expr_dawk_pipe)) &&
             (d_tests_expr_dawk_custom(_s, C_GETLINE, D_PARSE_EXPR_OPERAND,
                                       D_PARSE_EXPR_ROLE_NO_JUXTAPOSE, 0u,
                                       d_tests_expr_dawk_getline)) &&
             (d_parse_expr_juxtapose(_s, O_CONCAT, NULL, NULL)) &&
             (d_parse_expr_signature_seal(_s, NULL)) );
}


// 1.7    Cases
//------------------------------------------------------------------------------
// dj_tests_expr_case
//   struct: a text and the term it must read as.
struct dj_tests_expr_case
{
    const char* text;
    const char* term;
};

/*
d_tests_expr_cases
  Each text must parse without an error, as the term given, into a term that
verifies.
*/
static bool
d_tests_expr_cases(
    const struct dj_parse_expr_signature* _sig,
    const struct dj_tests_expr_case*      _cases,
    size_t                                _count,
    uint32_t                              _lexer,
    uint32_t                              _mode
)
{
    bool ok = true;

    for (size_t i = 0u; i < _count; ++i)
    {
        static struct dj_tests_expr_run run;
        struct dj_tests_expr_text       b;

        memset(&run, 0, sizeof(run));
        run.options.mode = _mode;
        (void)d_tests_expr_parse(&run, _sig, _cases[i].text, _lexer);

        const char* got  = d_tests_expr_sexpr(&run.term, run.result.root, &b);
        const bool  held = ( (run.status == D_PARSE_EXPR_OK)        &&
                             (strcmp(got, _cases[i].term) == 0)     &&
                             (d_parse_expr_term_verify(&run.term)) );

        if (!held)
        {
            printf("    '%s' read as %s (status %d), not %s\n",
                   _cases[i].text, got, run.status, _cases[i].term);
        }

        ok = (ok) && (held);
        d_parse_expr_term_release(&run.term);
    }

    return ok;
}


//==============================================================================
// 2.  SECTIONS
//==============================================================================


/*
d_tests_expr_term
  The arena and its builders.
  Tests the following:
  - leaves, applications and holes are appended in order, with their depths
  - an application of a child not yet in the term is refused
  - verify holds a built term and refuses a broken one
  - equal compares meaning, spans aside unless asked
  - copy keeps a shared node shared
  - caller storage, once full, refuses another node
*/
static bool
d_tests_expr_term(void)
{
    struct dj_parse_expr_term t;
    struct dj_parse_expr_term u;
    bool                      ok = true;

    d_parse_expr_term_init_heap(&t);
    d_parse_expr_term_init_heap(&u);

    const uint32_t a     = d_parse_expr_term_leaf(&t, C_NAME, 0u, 0u, 1u);
    const uint32_t b     = d_parse_expr_term_leaf(&t, C_NAME, 1u, 4u, 1u);
    const uint32_t pair[2] = { a, b };
    const uint32_t lt    = d_parse_expr_term_apply(&t, O_LT, pair, 2u, 0u, 5u);
    const uint32_t ahead[2] = { lt, 99u };

    ok = d_tests_expr_expect(ok && (a == 0u) && (b == 1u) && (lt == 2u),
                             "nodes are appended in order", __LINE__);
    ok = d_tests_expr_expect(ok && (t.nodes[lt].depth == 2u) &&
                             (t.depth == 2u), "depth is kept", __LINE__);
    ok = d_tests_expr_expect(ok && (d_parse_expr_term_apply(
                                 &t, O_AND, ahead, 2u, 0u, 1u) ==
                             D_PARSE_EXPR_NO_NODE),
                             "a child past the term is refused", __LINE__);

    const uint32_t shared[2] = { lt, lt };
    const uint32_t both  = d_parse_expr_term_apply(&t, O_AND, shared, 2u, 0u,
                                                   5u);

    t.root = both;
    ok = d_tests_expr_expect(ok && (d_parse_expr_term_verify(&t)),
                             "a built term verifies", __LINE__);

    const uint32_t copy = d_parse_expr_term_copy(&u, &t, both);

    ok = d_tests_expr_expect(ok && (copy == 3u) && (u.node_count == 4u),
                             "a shared node is copied once", __LINE__);
    ok = d_tests_expr_expect(ok && (d_parse_expr_term_equal(&t, both, &u,
                                                            copy, true)),
                             "the copy is equal, spans and all", __LINE__);
    u.nodes[0].offset = 7u;
    ok = d_tests_expr_expect(ok && (d_parse_expr_term_equal(&t, both, &u,
                                                            copy, false)) &&
                             (!d_parse_expr_term_equal(&t, both, &u, copy,
                                                       true)),
                             "spans count only when asked", __LINE__);
    t.args[0] = 3u;
    ok = d_tests_expr_expect(ok && (!d_parse_expr_term_verify(&t)),
                             "a child after its parent fails verify",
                             __LINE__);

    struct dj_parse_expr_node nodes[2];
    struct dj_parse_expr_term fixed;

    d_parse_expr_term_init(&fixed, nodes, 2u, NULL, 0u);
    (void)d_parse_expr_term_leaf(&fixed, C_NUM, 0u, 0u, 1u);
    (void)d_parse_expr_term_leaf(&fixed, C_NUM, 1u, 0u, 1u);
    ok = d_tests_expr_expect(ok && (d_parse_expr_term_leaf(&fixed, C_NUM, 2u,
                                                           0u, 1u) ==
                                    D_PARSE_EXPR_NO_NODE),
                             "full caller storage refuses a node", __LINE__);

    d_parse_expr_term_release(&t);
    d_parse_expr_term_release(&u);

    return ok;
}

/*
d_tests_expr_signature
  Sealing.
  Tests the following:
  - each of math's, the command language's and the awk-like tables seals
  - an operator defined twice, a chain whose join is no infix operator, a
    role of an undefined operator, a role whose operator does not fit its
    position, and a mixfix of one part each fail the seal, with a report
  - a sealed signature refuses more definitions, and finds by id and class
*/
static bool
d_tests_expr_signature(void)
{
    struct dj_parse_expr_signature s;
    struct dj_parse_diagnostic     items[8];
    char                           text[1024];
    struct dj_parse_diag_sink      sink;
    struct dj_parse_expr_mixfix    one = d_tests_expr_mixfix_of(0u, C_LP, "(",
                                                               C_RP, ")");
    uint32_t                       count = 0u;
    bool                           ok    = true;

    ok = d_tests_expr_expect(ok && (d_tests_expr_math(&s, true)),
                             "math's table seals", __LINE__);
    ok = d_tests_expr_expect(ok && (!d_parse_expr_define_atom(&s, C_NUM, 0u)),
                             "a sealed signature refuses more", __LINE__);
    ok = d_tests_expr_expect(ok && (d_parse_expr_find_op(&s, O_POW)) &&
                             (d_parse_expr_find_op(&s, O_POW)->precedence ==
                              80) &&
                             (d_parse_expr_find_roles(&s, C_LP,
                                                      D_PARSE_EXPR_OPERATOR,
                                                      &count)) &&
                             (count == 1u),
                             "operators and roles are found", __LINE__);
    d_parse_expr_signature_release(&s);
    ok = d_tests_expr_expect(ok && (d_tests_expr_cli(&s)),
                             "the command language seals", __LINE__);
    d_parse_expr_signature_release(&s);
    ok = d_tests_expr_expect(ok && (d_tests_expr_awk(&s)),
                             "the awk-like table seals", __LINE__);
    d_parse_expr_signature_release(&s);

    one.parts = 1u;
    d_parse_diag_sink_init(&sink, items, 8u, text, (uint32_t)sizeof(text));
    d_parse_expr_signature_init_heap(&s);
    (void)d_parse_expr_define_op(&s, O_ADD, D_PARSE_EXPR_FORM_INFIX, 50,
                                 D_PARSE_EXPR_ASSOC_LEFT, 0u, "+");
    (void)d_parse_expr_define_op(&s, O_ADD, D_PARSE_EXPR_FORM_INFIX, 50,
                                 D_PARSE_EXPR_ASSOC_LEFT, 0u, "+");
    (void)d_parse_expr_define_chain(&s, O_LT, 40, O_NOT, "<");
    (void)d_parse_expr_define_token(&s, C_STAR, D_PARSE_EXPR_OPERATOR, O_MUL,
                                    0u);
    (void)d_parse_expr_define_token(&s, C_PLUS, D_PARSE_EXPR_OPERAND, O_ADD,
                                    0u);
    (void)d_parse_expr_define_mixfix(&s, O_GROUP, 0, 0u, 0u, &one);
    ok = d_tests_expr_expect(ok && (!d_parse_expr_signature_seal(&s, &sink)),
                             "an unsound table does not seal", __LINE__);
    ok = d_tests_expr_expect(ok && (sink.count == 5u) &&
                             (items[0].domain == D_PARSE_DIAG_DOMAIN_EXPR) &&
                             (items[0].code ==
                              D_PARSE_EXPR_DIAG_BAD_SIGNATURE),
                             "each of its five faults is reported", __LINE__);
    d_parse_expr_signature_release(&s);

    return ok;
}

/*
d_tests_expr_math_cases
  Math's cases, the design's section 6.3, each as its term.
  Tests the following:
  - indexing, the transpose and the factorial after any operand (S252)
  - calls only after an ungrouped name, products otherwise (S201, S196)
  - bars opening and closing by where they stand (S207)
  - chains joined by `and`, a grouped relation no link (S214, S215)
  - logic below the relations (S213); literals with rows; empty lists
*/
static bool
d_tests_expr_math_cases(void)
{
    static const struct dj_tests_expr_case CASES[] =
    {
        { "-v[1]",            "neg(index(v, 1))" },
        { "2 ^ v[1]",         "pow(2, index(v, 1))" },
        { "A[1][2]",          "index(index(A, 1), 2)" },
        { "x'[1]",            "index(trans(x), 1)" },
        { "f(x)[1]",          "index(call(f, x), 1)" },
        { "(a + b)[1]",       "index(add(a, b), 1)" },
        { "A' ^ 2",           "pow(trans(A), 2)" },
        { "A ^ 2'",           "trans(pow(A, 2))" },
        { "-A'",              "neg(trans(A))" },
        { "x!'",              "trans(fact(x))" },
        { "2 ^ 3!",           "pow(2, fact(3))" },
        { "-3!",              "neg(fact(3))" },
        { "n!!",              "fact(fact(n))" },
        { "sin(x) ^ 2",       "pow(call(sin, x), 2)" },
        { "2 sin(x)",         "mul(2, call(sin, x))" },
        { "2(y)",             "mul(2, y)" },
        { "(x)(y)",           "mul(x, y)" },
        { "x(y)",             "call(x, y)" },
        { "|x| |y|",          "mul(abs(x), abs(y))" },
        { "||x| - 1|",        "abs(sub(abs(x), 1))" },
        { "a < b <= c",       "and*(lt(a, b), le(b, c))" },
        { "a < b < c < d",    "and*(and*(lt(a, b), lt(b, c)), lt(c, d))" },
        { "(a < b) = c",      "eq(lt(a, b), c)" },
        { "x > 0 and x < 1",  "and(gt(x, 0), lt(x, 1))" },
        { "not x = y",        "not(eq(x, y))" },
        { "[1, 2; 3, 4]",     "matrix(row(1, 2), row(3, 4))" },
        { "[a, b]",           "vector(a, b)" },
        { "[]",               "vector()" },
        { "f()",              "call(f)" },
        { "1/2x",             "mul(div(1, 2), x)" },
        { "-x ^ 2",           "neg(pow(x, 2))" },
        { "-x * y",           "mul(neg(x), y)" },
        { "a - b - c",        "sub(sub(a, b), c)" },
        { "a ^ b ^ c",        "pow(a, pow(b, c))" },
        { "a mod b * c",      "mul(mod(a, b), c)" }
    };
    struct dj_parse_expr_signature s;
    bool                           ok = d_tests_expr_math(&s, true);

    ok = d_tests_expr_expect(ok && (d_tests_expr_cases(
                                 &s, CASES, sizeof(CASES) / sizeof(CASES[0]),
                                 0u, 0u)),
                             "every case reads as its term", __LINE__);
    d_parse_expr_signature_release(&s);

    return ok;
}

/*
d_tests_expr_cli_cases
  The command language's cases, the design's section 6.4.
  Tests the following:
  - a command takes its options and operands as leaves
  - `|` and `;` flatten into one node each, `;` the looser
  - `not` binds above `and`; groups
  - an operator only a hook builds (the command) seals, and renders by its
    form: each case is written back as it was read (lane G, 11-05)
*/
static bool
d_tests_expr_cli_cases(void)
{
    static const struct dj_tests_expr_case CASES[] =
    {
        { "ls -l | grep x ; echo done",
          "seq(pipe(cmd(ls, -l), cmd(grep, x)), cmd(echo, done))" },
        { "a | b | c",          "pipe(cmd(a), cmd(b), cmd(c))" },
        { "not a and b",        "and(not(cmd(a)), cmd(b))" },
        { "(a ; b) | c",        "pipe(seq(cmd(a), cmd(b)), cmd(c))" },
        { "a == 1 or b =~ c",   "or(eqeq(cmd(a), 1), match(cmd(b), cmd(c)))" }
    };
    struct dj_parse_expr_signature s;
    bool                           ok = d_tests_expr_cli(&s);

    ok = d_tests_expr_expect(ok && (d_tests_expr_cases(
                                 &s, CASES, sizeof(CASES) / sizeof(CASES[0]),
                                 D_TESTS_EXPR_LEX_OPTIONS, 0u)),
                             "every case reads as its term", __LINE__);

    for (size_t i = 0u; ( (ok) && (i < sizeof(CASES) / sizeof(CASES[0])) );
         ++i)
    {
        static struct dj_tests_expr_run     run;
        struct dj_parse_expr_render_options plain;
        char                                text[128];

        memset(&run, 0, sizeof(run));
        memset(&plain, 0, sizeof(plain));
        (void)d_tests_expr_parse(&run, &s, CASES[i].text,
                                 D_TESTS_EXPR_LEX_OPTIONS);
        plain.text        = CASES[i].text;
        plain.text_length = strlen(CASES[i].text);
        (void)d_parse_expr_render_buffer(&s, &run.term, run.result.root,
                                         &plain, text, sizeof(text));
        ok = d_tests_expr_expect(strcmp(text, CASES[i].text) == 0,
                                 CASES[i].text, __LINE__);
        d_parse_expr_term_release(&run.term);
    }

    d_parse_expr_signature_release(&s);

    return ok;
}

/*
d_tests_expr_awk_cases
  The awk-like language's cases, the design's section 6.4.
  Tests the following:
  - concatenation by a flattening juxtaposition; `-` after an operand still
    subtracts
  - `?:` right-associative with a tail; `if ... then ... else` of three parts
  - `++` prefix and postfix by position
  - a call only with no space before its bracket
  - a role a mode turns off
*/
static bool
d_tests_expr_awk_cases(void)
{
    static const struct dj_tests_expr_case CASES[] =
    {
        { "a b c",                 "concat(a, b, c)" },
        { "a - 1",                 "sub(a, 1)" },
        { "a -1 b",                "concat(sub(a, 1), b)" },
        { "c ? a : d ? b : x",     "cond(c, a, cond(d, b, x))" },
        { "if c then a else b c",  "if(c, a, concat(b, c))" },
        { "++i i++",               "concat(preinc(i), postinc(i))" },
        { "f(x)",                  "call(f, x)" },
        { "f (x)",                 "concat(f, x)" },
        { "a > b",                 "gt(a, b)" }
    };
    static const struct dj_tests_expr_case MODE2[] =
    {
        { "a b",                   "concat(a, b)" }
    };
    struct dj_parse_expr_signature s;
    static struct dj_tests_expr_run run;
    bool                           ok = d_tests_expr_awk(&s);

    ok = d_tests_expr_expect(ok && (d_tests_expr_cases(
                                 &s, CASES, sizeof(CASES) / sizeof(CASES[0]),
                                 0u, 0u)),
                             "every case reads as its term", __LINE__);
    ok = d_tests_expr_expect(ok && (d_tests_expr_cases(&s, MODE2, 1u, 0u,
                                                       2u)),
                             "mode 2 reads what it has", __LINE__);

    memset(&run, 0, sizeof(run));
    run.options.mode = 2u;
    (void)d_tests_expr_parse(&run, &s, "a > b", 0u);
    ok = d_tests_expr_expect(
             (ok) && (run.status == D_PARSE_EXPR_ERROR) &&
             (run.result.code == D_PARSE_EXPR_DIAG_UNEXPECTED) &&
             (run.result.span.offset == 2u),
             "mode 2 has no `>`", __LINE__);
    d_parse_expr_term_release(&run.term);
    d_parse_expr_signature_release(&s);

    return ok;
}

// dj_tests_expr_error
//   struct: a text, the error it must give, where, and how many errors.
struct dj_tests_expr_error
{
    const char* text;
    uint16_t    code;
    uint32_t    offset;
    uint32_t    length;
    uint32_t    errors;
};

/*
d_tests_expr_errors
  Each text must fail with its error first, at its span, with its count of
errors, the sink holding the error at the result's index.
*/
static bool
d_tests_expr_errors(
    const struct dj_parse_expr_signature* _sig,
    const struct dj_tests_expr_error*     _cases,
    size_t                                _count,
    uint8_t                               _order
)
{
    bool ok = true;

    for (size_t i = 0u; i < _count; ++i)
    {
        static struct dj_tests_expr_run run;

        memset(&run, 0, sizeof(run));
        run.options.error_order = _order;
        (void)d_tests_expr_parse(&run, _sig, _cases[i].text, 0u);

        const struct dj_parse_diagnostic* told =
            d_parse_diag_at(&run.sink, run.result.diag_index);
        const bool held =
            ( (run.status == D_PARSE_EXPR_ERROR)                    &&
              (run.result.domain == D_PARSE_DIAG_DOMAIN_EXPR)       &&
              (run.result.code == _cases[i].code)                   &&
              (run.result.span.offset == _cases[i].offset)          &&
              (run.result.span.length == _cases[i].length)          &&
              (run.result.errors == _cases[i].errors)               &&
              (told)                                                &&
              (told->code == _cases[i].code)                        &&
              (d_parse_expr_term_verify(&run.term)) );

        if (!held)
        {
            printf("    '%s': status %d, code %u at %u+%u, %u errors\n",
                   _cases[i].text, run.status, (unsigned)run.result.code,
                   (unsigned)run.result.span.offset,
                   (unsigned)run.result.span.length,
                   (unsigned)run.result.errors);
        }

        ok = (ok) && (held);
        d_parse_expr_term_release(&run.term);
    }

    return ok;
}

/*
d_tests_expr_dawk_cases
  dawk's expressions, the cases of lane H's review of 11-05.
  Tests the following:
  - a postfix step after something that cannot be assigned begins the next
    operand instead (point 1), and `$` is above it (`$i++`, `$++i`)
  - a bracketed list is a node of its own, one item a group (point 2); the
    check refuses the list anywhere but before `in`
  - every level of the ladder, `| getline` between the relations and
    concatenation, its left the whole concatenation (point 3)
  - `getline` and `getline var` (point 4, the rest with the sub-parse)
  - `^` above a sign, its right operand free to begin with one (point 6)
  - a call only where `(` touches its name (point 7)
  - what may and may not begin a concatenated operand (point 8)
*/
static bool
d_tests_expr_dawk_cases(void)
{
    static const struct dj_tests_expr_case CASES[] =
    {
        { "1 ++i",               "concat(1, preinc(i))" },
        { "a ++b",               "concat(postinc(a), b)" },
        { "$i++",                "postinc(field(i))" },
        { "$++i",                "field(preinc(i))" },
        { "++$i",                "preinc(field(i))" },
        { "arr[i, j]--",         "postdec(index(arr, i, j))" },
        { "(a)++ b",             "concat(a, preinc(b))" },
        { "(i, j) in arr",       "in(group(i, j), arr)" },
        { "(a)",                 "a" },
        { "a = b = c",           "assign(a, assign(b, c))" },
        { "a = b ? c : d ? x : y", "assign(a, cond(b, c, cond(d, x, y)))" },
        { "a || b && c",         "or(a, and(b, c))" },
        { "a && b in arr",       "and(a, in(b, arr))" },
        { "a in arr && b",       "and(in(a, arr), b)" },
        { "a ~ b < c",           "match(a, lt(b, c))" },
        { "a !~ b",              "nomatch(a, b)" },
        { "a < b c",             "lt(a, concat(b, c))" },
        { "cmd | getline line > 0", "gt(pipeget(cmd, line), 0)" },
        { "a b | getline",       "pipeget(concat(a, b))" },
        { "a b + c",             "concat(a, add(b, c))" },
        { "a + b * c % d",       "add(a, mod(mul(b, c), d))" },
        { "getline line",        "getline(line)" },
        { "x = getline",         "assign(x, getline())" },
        { "-2^2",                "neg(pow(2, 2))" },
        { "2^-3",                "pow(2, neg(3))" },
        { "2^-3^2",              "pow(2, neg(pow(3, 2)))" },
        { "f(x)",                "call(f, x)" },
        { "f (x)",               "concat(f, x)" },
        { "a b -1",              "concat(a, sub(b, 1))" },
        { "a -1",                "sub(a, 1)" },
        { "!a b",                "concat(not(a), b)" },
        { "a !b",                "concat(a, not(b))" },
        { "a $1",                "concat(a, field(1))" },
        { "a (b)",               "concat(a, b)" }
    };
    static const struct dj_tests_expr_error ERRORS[] =
    {
        { "a < b < c", D_PARSE_EXPR_DIAG_NON_ASSOCIATIVE, 6u, 1u, 1u },
        { "a getline", D_PARSE_EXPR_DIAG_UNEXPECTED,      2u, 7u, 1u },
        { "a | b",     D_PARSE_EXPR_DIAG_UNEXPECTED,      2u, 1u, 1u }
    };
    struct dj_parse_expr_signature  s;
    static struct dj_tests_expr_run run;
    bool                            ok = d_tests_expr_dawk(&s);

    ok = d_tests_expr_expect(ok && (d_tests_expr_cases(
                                 &s, CASES, sizeof(CASES) / sizeof(CASES[0]),
                                 D_TESTS_EXPR_LEX_DAWK, 0u)),
                             "every case reads as its term", __LINE__);
    ok = d_tests_expr_expect(ok && (d_tests_expr_errors(
                                 &s, ERRORS,
                                 sizeof(ERRORS) / sizeof(ERRORS[0]), 0u)),
                             "each error is the one expected", __LINE__);

    // the check keeps a bracketed list before `in`, and refuses it elsewhere
    memset(&run, 0, sizeof(run));
    run.options.check = d_tests_expr_dawk_list;
    (void)d_tests_expr_parse(&run, &s, "(i, j) in arr",
                             D_TESTS_EXPR_LEX_DAWK);
    ok = d_tests_expr_expect(ok && (run.status == D_PARSE_EXPR_OK),
                             "a list before `in` is kept", __LINE__);
    d_parse_expr_term_release(&run.term);

    memset(&run, 0, sizeof(run));
    run.options.check = d_tests_expr_dawk_list;
    (void)d_tests_expr_parse(&run, &s, "x + (i, j)",
                             D_TESTS_EXPR_LEX_DAWK);
    ok = d_tests_expr_expect(
             (ok) && (run.status == D_PARSE_EXPR_ERROR) &&
             (run.result.domain == D_PARSE_DIAG_DOMAIN_USER) &&
             (run.result.code == 8u) && (run.result.span.offset == 4u) &&
             (run.result.span.length == 6u),
             "a list anywhere else is refused, at the list", __LINE__);
    d_parse_expr_term_release(&run.term);
    d_parse_expr_signature_release(&s);

    return ok;
}

// dj_tests_expr_stop
//   struct: a text read with D_PARSE_EXPR_OPTION_STOP, the term and status it
// must give, and the token it must end at.
struct dj_tests_expr_stop
{
    const char* text;
    const char* term;
    int         status;
    uint32_t    next_class;
    uint32_t    next_offset;
};

/*
d_tests_expr_stop
  The option that lets a statement's parser hand the core an expression and
take back what ends it (lane H's point 8), on dawk's table.
  Tests the following:
  - a token the expression cannot go on with ends it, unread, in `next`
  - a closing part with nothing open ends it too: an `if (`'s `)`
  - an operand missing before the token that ends it is an error, and the
    token still ends it
  - inside an open bracket the token is still an error: the bracket is
    never closed
  - without the option, the end of the input is `next`
*/
static bool
d_tests_expr_stop(void)
{
    static const struct dj_tests_expr_stop STOPS[] =
    {
        { "a + b ; c",  "add(a, b)",     D_PARSE_EXPR_OK,    C_SEMI,    6u },
        { "a > b ) x",  "gt(a, b)",      D_PARSE_EXPR_OK,    C_RP,      6u },
        { "a b getline", "concat(a, b)", D_PARSE_EXPR_OK,    C_GETLINE, 4u },
        { "a | b",      "a",             D_PARSE_EXPR_OK,    C_BAR,     2u },
        { "a + ;",      "add(a, _)",     D_PARSE_EXPR_ERROR, C_SEMI,    4u },
        { "(a ; b)",    "a",             D_PARSE_EXPR_ERROR, C_SEMI,    3u },
        { "a + b",      "add(a, b)",     D_PARSE_EXPR_OK,
          D_PARSE_EXPR_CLASS_END, 5u }
    };
    struct dj_parse_expr_signature s;
    bool                           ok = d_tests_expr_dawk(&s);

    for (size_t i = 0u; i < sizeof(STOPS) / sizeof(STOPS[0]); ++i)
    {
        static struct dj_tests_expr_run run;
        struct dj_tests_expr_text       b;

        memset(&run, 0, sizeof(run));
        run.options.flags = D_PARSE_EXPR_OPTION_STOP;
        (void)d_tests_expr_parse(&run, &s, STOPS[i].text,
                                 D_TESTS_EXPR_LEX_DAWK);

        const char* got  = d_tests_expr_sexpr(&run.term, run.result.root, &b);
        const bool  held =
            ( (run.status == STOPS[i].status)                        &&
              ( (run.status != D_PARSE_EXPR_OK) ||
                (strcmp(got, STOPS[i].term) == 0) )                  &&
              (run.result.next.cls == STOPS[i].next_class)           &&
              (run.result.next.offset == STOPS[i].next_offset) );

        if (!held)
        {
            printf("    '%s' read as %s (status %d), next %lu at %lu\n",
                   STOPS[i].text, got, run.status,
                   (unsigned long)run.result.next.cls,
                   (unsigned long)run.result.next.offset);
        }

        ok = (ok) && (held);
        d_parse_expr_term_release(&run.term);
    }

    // without the option, `;` is a stray
    {
        static struct dj_tests_expr_run run;

        memset(&run, 0, sizeof(run));
        (void)d_tests_expr_parse(&run, &s, "a + b ; c",
                                 D_TESTS_EXPR_LEX_DAWK);
        ok = d_tests_expr_expect(
                 (ok) && (run.status == D_PARSE_EXPR_ERROR) &&
                 (run.result.code == D_PARSE_EXPR_DIAG_UNEXPECTED) &&
                 (run.result.next.cls == D_PARSE_EXPR_CLASS_END),
                 "without the option the parse reads to the end", __LINE__);
        d_parse_expr_term_release(&run.term);
    }

    d_parse_expr_signature_release(&s);

    return ok;
}

// dj_tests_expr_modal
//   struct: a source over an array that records the mode each token was read
// in, as a lexer with raw blocks would be asked (lane G, 11-05).
struct dj_tests_expr_modal
{
    struct dj_parse_expr_array_source array;
    struct dj_parse_expr_source       inner;
    uint32_t                          modes[16];
    uint32_t                          count;
};

/*
d_tests_expr_modal_next
  The array source's next token, its mode recorded.
*/
static int
d_tests_expr_modal_next(
    void*                       _ctx,
    uint8_t                     _position,
    uint32_t                    _mode,
    struct dj_parse_expr_token* _out
)
{
    struct dj_tests_expr_modal* m = (struct dj_tests_expr_modal*)_ctx;

    if (m->count < 16u)
    {
        m->modes[m->count++] = _mode;
    }

    return m->inner.next(m->inner.ctx, _position, _mode, _out);
}

/*
d_tests_expr_raw
  A custom role on `[` that reads the next token in mode 4, as lane G's
block is read raw, and makes it a leaf.
*/
static int
d_tests_expr_raw(
    void*                             _ctx,
    struct dj_parse_expr_parser*      _parser,
    const struct dj_parse_expr_token* _token,
    uint32_t                          _left,
    uint32_t*                         _out
)
{
    struct dj_parse_expr_term* term = d_parse_expr_parser_term(_parser);
    struct dj_parse_expr_token raw;

    (void)_token;
    (void)_left;

    if (d_parse_expr_take(_parser, D_PARSE_EXPR_OPERAND, 4u, &raw) <= 0)
    {
        return -1;
    }

    // a hook that declines with the raw token still peeked is refused
    if (_ctx)
    {
        (void)d_parse_expr_peek(_parser, D_PARSE_EXPR_OPERATOR, 4u, &raw);

        return 0;
    }

    *_out = d_parse_expr_term_leaf(term, raw.cls, raw.value, raw.offset,
                                   raw.length);

    return (*_out != D_PARSE_EXPR_NO_NODE) ? 1 : -1;
}

/*
d_tests_expr_hooks
  The hook interface's reading in a mode of its own.
  Tests the following:
  - a token a hook takes in a mode is read from the source in that mode, and
    the tokens around it in the parse's
  - a hook that leaves a token it read in another mode stops the parse, as
    the parser cannot read it again
*/
static bool
d_tests_expr_hooks(void)
{
    struct dj_parse_expr_signature  s;
    static struct dj_tests_expr_run run;
    bool                            ok = true;

    for (int declines = 0; declines < 2; ++declines)
    {
        struct dj_tests_expr_modal m;
        struct dj_parse_expr_source source;
        struct dj_parse_expr_role   raw;
        const char* const           text  = "[ a b";
        const int                   count = d_tests_expr_lex(text, 0u,
                                                             run.tokens,
                                                             4096);

        memset(&m, 0, sizeof(m));
        memset(&raw, 0, sizeof(raw));
        memset(&run.result, 0, sizeof(run.result));
        raw.cls        = C_LB;
        raw.position   = D_PARSE_EXPR_OPERAND;
        raw.kind       = D_PARSE_EXPR_ROLE_CUSTOM;
        raw.custom     = d_tests_expr_raw;
        raw.custom_ctx = (declines) ? &m : NULL;
        d_parse_expr_signature_init_heap(&s);
        ok = ( (ok)                                                    &&
               (count == 3)                                            &&
               (d_parse_expr_define_atom(&s, C_NAME, 0u))              &&
               (d_parse_expr_define_role(&s, &raw))                    &&
               (d_parse_expr_define_op(&s, O_CONCAT,
                                       D_PARSE_EXPR_FORM_INFIX, 90,
                                       D_PARSE_EXPR_ASSOC_LEFT,
                                       D_PARSE_EXPR_OP_FLATTEN, ""))   &&
               (d_parse_expr_juxtapose(&s, O_CONCAT, NULL, NULL))      &&
               (d_parse_expr_signature_seal(&s, NULL)) );
        d_parse_expr_source_array(&m.inner, &m.array, run.tokens,
                                  (uint32_t)count, 5u);
        source.next = d_tests_expr_modal_next;
        source.ctx  = &m;
        d_parse_expr_term_init_heap(&run.term);
        run.status = d_parse_expr_parse(&s, &source, NULL, &run.term, NULL,
                                        &run.result);
        ok = d_tests_expr_expect(
                 (ok) &&
                 ( (declines)
                   ? ( (run.status == D_PARSE_EXPR_FATAL) &&
                       (run.result.domain == D_PARSE_DIAG_DOMAIN_CORE) &&
                       (run.result.code == D_PARSE_DIAG_BAD_ARGUMENT) )
                   : ( (run.status == D_PARSE_EXPR_OK) &&
                       (m.count >= 3u) && (m.modes[0] == 1u) &&
                       (m.modes[1] == 4u) && (m.modes[2] == 1u) ) ),
                 (declines) ? "a token left in another mode stops the parse"
                            : "a hook reads in its own mode", __LINE__);
        d_parse_expr_term_release(&run.term);
        d_parse_expr_signature_release(&s);
    }

    return ok;
}

/*
d_tests_expr_diagnostics
  Every code the parser reports, at its character.
  Tests the following:
  - unexpected, missing operand, unclosed, unopened, mismatched (an outer
    operator's close, and a close of another kind), non-associative, arity
  - the leftmost error is the error, as S209 has it; the structure-first
    order names a bracket's error first, as math's parser layer does
  - an unclosed bracket and a mismatched one carry their notes, each under
    its error's code; a refused pair's error names both operators
  - the language's check refuses a node, as its own error
*/
static bool
d_tests_expr_diagnostics(void)
{
    static const struct dj_tests_expr_error MATH[] =
    {
        { "1 + * 2",   D_PARSE_EXPR_DIAG_UNEXPECTED,      4u, 1u, 1u },
        { "2 3",       D_PARSE_EXPR_DIAG_UNEXPECTED,      2u, 1u, 1u },
        { "x not y",   D_PARSE_EXPR_DIAG_UNEXPECTED,      2u, 3u, 1u },
        { "1 +",       D_PARSE_EXPR_DIAG_MISSING_OPERAND, 3u, 0u, 1u },
        { "[1, ]",     D_PARSE_EXPR_DIAG_MISSING_OPERAND, 4u, 1u, 1u },
        { "x[]",       D_PARSE_EXPR_DIAG_MISSING_OPERAND, 2u, 1u, 1u },
        { "()",        D_PARSE_EXPR_DIAG_MISSING_OPERAND, 1u, 1u, 1u },
        { "(1 + 2",    D_PARSE_EXPR_DIAG_UNCLOSED,        0u, 1u, 1u },
        { "f(a, |b",   D_PARSE_EXPR_DIAG_UNCLOSED,        5u, 1u, 1u },
        { "1 + 2)",    D_PARSE_EXPR_DIAG_UNOPENED,        5u, 1u, 1u },
        { "(1 + 2]",   D_PARSE_EXPR_DIAG_MISMATCHED,      6u, 1u, 1u },
        { "f([1, 2)",  D_PARSE_EXPR_DIAG_MISMATCHED,      7u, 1u, 1u },
        { "A[1,2,3]",  D_PARSE_EXPR_DIAG_ARITY,           1u, 7u, 1u },
        { "1 + * 2)",  D_PARSE_EXPR_DIAG_UNEXPECTED,      4u, 1u, 2u },
        { "f((a, b))", D_PARSE_EXPR_DIAG_UNEXPECTED,      4u, 1u, 1u }
    };
    static const struct dj_tests_expr_error FIRST[] =
    {
        { "1 + * 2)",  D_PARSE_EXPR_DIAG_UNOPENED,        7u, 1u, 2u }
    };
    static const struct dj_tests_expr_error CLI[] =
    {
        { "a == b == c", D_PARSE_EXPR_DIAG_NON_ASSOCIATIVE, 7u, 2u, 1u },
        { "a < b == c",  D_PARSE_EXPR_DIAG_NON_ASSOCIATIVE, 6u, 2u, 1u }
    };
    struct dj_parse_expr_signature  math;
    struct dj_parse_expr_signature  cli;
    static struct dj_tests_expr_run run;
    bool                            ok = ( (d_tests_expr_math(&math, true)) &&
                                           (d_tests_expr_cli(&cli)) );

    ok = d_tests_expr_expect(ok && (d_tests_expr_errors(
                                 &math, MATH, sizeof(MATH) / sizeof(MATH[0]),
                                 D_PARSE_EXPR_ORDER_DEFAULT)),
                             "math's errors, leftmost first", __LINE__);
    ok = d_tests_expr_expect(ok && (d_tests_expr_errors(
                                 &math, FIRST, 1u,
                                 D_PARSE_EXPR_ORDER_STRUCTURE_FIRST)),
                             "structure first names the bracket", __LINE__);
    ok = d_tests_expr_expect(ok && (d_tests_expr_errors(
                                 &cli, CLI, sizeof(CLI) / sizeof(CLI[0]),
                                 D_PARSE_EXPR_ORDER_DEFAULT)),
                             "non-associative pairs are refused", __LINE__);

    // the notes: where the input ended, where the other operator stood
    memset(&run, 0, sizeof(run));
    (void)d_tests_expr_parse(&run, &math, "(1 + 2", 0u);
    ok = d_tests_expr_expect(ok && (run.sink.count == 2u) &&
                             (run.items[1].severity == D_PARSE_SEVERITY_NOTE) &&
                             ((run.items[1].flags &
                               D_PARSE_DIAG_FLAG_CONTINUATION) != 0u) &&
                             (run.items[1].span.offset == 6u) &&
                             (run.items[1].code == run.items[0].code),
                             "an unclosed bracket's note is at the end, "
                             "under its code", __LINE__);
    d_parse_expr_term_release(&run.term);
    memset(&run, 0, sizeof(run));
    (void)d_tests_expr_parse(&run, &cli, "a < b == c", 0u);
    ok = d_tests_expr_expect(
             (ok) && (run.sink.count == 2u) &&
             (run.items[1].span.offset == 2u) &&
             (run.items[1].code == D_PARSE_EXPR_DIAG_NON_ASSOCIATIVE) &&
             (strcmp(d_parse_diag_message(&run.sink, &run.items[0]),
                     "'==' cannot follow '<' without brackets") == 0),
             "a refused pair names both, its note at the first under its "
             "code", __LINE__);
    d_parse_expr_term_release(&run.term);

    d_parse_expr_signature_release(&math);
    d_parse_expr_signature_release(&cli);

    return ok;
}

/*
d_tests_expr_refuse_index
  A check hook: math's index takes one or two indices, and says so in its own
words (the core's bounds would too; this is the hook's path).
*/
static bool
d_tests_expr_refuse_index(
    void*                            _ctx,
    const struct dj_parse_expr_term* _term,
    uint32_t                         _node,
    struct dj_parse_diag_sink*       _sink
)
{
    const struct dj_parse_expr_node* node = &_term->nodes[_node];

    (void)_ctx;

    // a call of `f` with no argument is refused, everything else kept
    if ( (node->op != O_CALL) ||
         (node->count != 1u) )
    {
        return true;
    }

    (void)d_parse_diag_emit(_sink, D_PARSE_SEVERITY_ERROR,
                            (uint16_t)D_PARSE_DIAG_DOMAIN_USER, 7u,
                            d_parse_span_make(node->offset, node->length),
                            "a call needs an argument here");

    return false;
}

/*
d_tests_expr_limits
  The settings and the guards.
  Tests the following:
  - the check hook's refusal is the parse's error, in the hook's words
  - the depth guard stops a parse past it, by nesting or by a long chain,
    reported at the operator whose node passes it
  - a guard past the deepest a node records is refused; that deepest is kept
  - an allowance of errors past 65535 is refused
  - max_errors stops a parse at its allowance
  - with no sink, the result still names the error
  - a term on caller storage that fills is a fatal capacity failure
*/
static bool
d_tests_expr_limits(void)
{
    struct dj_parse_expr_signature  s;
    static struct dj_tests_expr_run run;
    bool                            ok = d_tests_expr_math(&s, true);

    memset(&run, 0, sizeof(run));
    run.options.check = d_tests_expr_refuse_index;
    (void)d_tests_expr_parse(&run, &s, "1 + f()", 0u);
    ok = d_tests_expr_expect(ok && (run.status == D_PARSE_EXPR_ERROR) &&
                             (run.result.domain == D_PARSE_DIAG_DOMAIN_USER) &&
                             (run.result.code == 7u) &&
                             (run.result.span.offset == 4u),
                             "a refusal is the hook's error", __LINE__);
    d_parse_expr_term_release(&run.term);

    memset(&run, 0, sizeof(run));
    run.options.max_depth = 3u;
    (void)d_tests_expr_parse(&run, &s, "((((x))))", 0u);
    ok = d_tests_expr_expect(ok && (run.status == D_PARSE_EXPR_FATAL) &&
                             (run.result.code == D_PARSE_EXPR_DIAG_TOO_DEEP),
                             "nesting past the guard stops", __LINE__);
    d_parse_expr_term_release(&run.term);

    memset(&run, 0, sizeof(run));
    run.options.max_depth = 3u;
    (void)d_tests_expr_parse(&run, &s, "a + b + c + d", 0u);
    ok = d_tests_expr_expect(ok && (run.status == D_PARSE_EXPR_FATAL) &&
                             (run.result.code == D_PARSE_EXPR_DIAG_TOO_DEEP) &&
                             (run.result.span.offset == 10u),
                             "a chain past the guard stops, at the operator "
                             "past it", __LINE__);
    d_parse_expr_term_release(&run.term);

    memset(&run, 0, sizeof(run));
    run.options.max_depth = D_PARSE_EXPR_NODE_DEPTH_MAX + 1u;
    (void)d_tests_expr_parse(&run, &s, "x", 0u);
    ok = d_tests_expr_expect(ok && (run.status == D_PARSE_EXPR_FATAL),
                             "a guard no node can keep is refused", __LINE__);
    d_parse_expr_term_release(&run.term);

    memset(&run, 0, sizeof(run));
    run.options.max_depth = D_PARSE_EXPR_NODE_DEPTH_MAX;
    (void)d_tests_expr_parse(&run, &s, "((x))", 0u);
    ok = d_tests_expr_expect(ok && (run.status == D_PARSE_EXPR_OK),
                             "the deepest guard is kept", __LINE__);
    d_parse_expr_term_release(&run.term);

    memset(&run, 0, sizeof(run));
    run.options.max_errors = D_PARSE_EXPR_ERRORS_MAX + 1u;
    (void)d_tests_expr_parse(&run, &s, "x", 0u);
    ok = d_tests_expr_expect(ok && (run.status == D_PARSE_EXPR_FATAL),
                             "an allowance of errors past its bound is "
                             "refused", __LINE__);
    d_parse_expr_term_release(&run.term);

    memset(&run, 0, sizeof(run));
    run.options.max_errors = 2u;
    (void)d_tests_expr_parse(&run, &s, "* * * * 1", 0u);
    ok = d_tests_expr_expect(ok && (run.result.errors == 2u) &&
                             (run.result.root == D_PARSE_EXPR_NO_NODE),
                             "the allowance of errors stops the parse",
                             __LINE__);
    d_parse_expr_term_release(&run.term);

    struct dj_parse_expr_token        tokens[8];
    struct dj_parse_expr_source       source;
    struct dj_parse_expr_array_source state;
    struct dj_parse_expr_term         fixed;
    struct dj_parse_expr_node         nodes[2];
    uint32_t                          args[2];
    struct dj_parse_expr_result       result;
    const int count = d_tests_expr_lex("1 + 2", 0u, tokens, 8);

    d_parse_expr_source_array(&source, &state, tokens, (uint32_t)count, 5u);
    d_parse_expr_term_init(&fixed, nodes, 2u, args, 2u);
    (void)d_parse_expr_parse(&s, &source, NULL, &fixed, NULL, &result);
    ok = d_tests_expr_expect(ok && (result.status == D_PARSE_EXPR_FATAL) &&
                             (result.domain == D_PARSE_DIAG_DOMAIN_CORE) &&
                             (result.code == D_PARSE_DIAG_CAPACITY) &&
                             (result.diag_index == 0xFFFFFFFFu),
                             "a full term is capacity, no sink needed",
                             __LINE__);

    d_parse_expr_signature_release(&s);

    return ok;
}

/*
d_tests_expr_atom_name
  The round trip's atom writer: a leaf as its atom's name.
*/
static bool
d_tests_expr_atom_name(
    void*                            _ctx,
    const struct dj_parse_expr_term* _term,
    uint32_t                         _node,
    dj_parse_expr_write_fn           _write,
    void*                            _write_ctx
)
{
    const char*  name   = ATOM_NAMES[_term->nodes[_node].first];
    const size_t length = strlen(name);

    (void)_ctx;

    return (_write(_write_ctx, name, length) == length);
}

/*
d_tests_expr_random
  The next of a fixed sequence (a 32-bit linear congruential generator), so
every run draws the same terms.
*/
static uint32_t
d_tests_expr_random(
    uint32_t* _state,
    uint32_t  _below
)
{
    *_state = (*_state * 1664525u) + 1013904223u;

    return (*_state >> 8) % _below;
}

/*
d_tests_expr_gen_leaf
  A random leaf: one of the names a to sin, or a number 0 to 4.
*/
static uint32_t
d_tests_expr_gen_leaf(
    struct dj_parse_expr_term* _t,
    uint32_t*                  _rng,
    bool                       _name
)
{
    const uint32_t pick = (_name) ? d_tests_expr_random(_rng, 14u)
                                  : d_tests_expr_random(_rng, 19u);

    return d_parse_expr_term_leaf(_t, (pick >= 14u) ? C_NUM : C_NAME, pick,
                                  0u, 0u);
}

static uint32_t d_tests_expr_gen(struct dj_parse_expr_term* _t,
                                 uint32_t*                  _rng,
                                 uint32_t                   _depth);

/*
d_tests_expr_gen_list
  A random list: a call's arguments, an index's, a vector's items, or a
matrix of two or three rows, as their node.
*/
static uint32_t
d_tests_expr_gen_list(
    struct dj_parse_expr_term* _t,
    uint32_t*                  _rng,
    uint32_t                   _depth,
    uint32_t                   _kind
)
{
    uint32_t       c[4];
    const uint32_t lead  = (_kind <= 1u) ? 1u : 0u;
    const uint32_t items = (_kind == 1u) ? 1u + d_tests_expr_random(_rng, 2u)
                                         : d_tests_expr_random(_rng, 4u);

    // a call's callee is a name; an index's operand anything
    if (lead)
    {
        c[0] = (_kind == 0u) ? d_tests_expr_gen_leaf(_t, _rng, true)
                             : d_tests_expr_gen(_t, _rng, _depth);
    }

    if (_kind == 3u)
    {
        const uint32_t rows = 2u + d_tests_expr_random(_rng, 2u);

        for (uint32_t r = 0u; r < rows; ++r)
        {
            uint32_t       e[3];
            const uint32_t n = 1u + d_tests_expr_random(_rng, 3u);

            for (uint32_t i = 0u; i < n; ++i)
            {
                e[i] = d_tests_expr_gen(_t, _rng, _depth);
            }

            c[r] = d_parse_expr_term_apply(_t, O_ROW, e, n, 0u, 0u);
        }

        const uint32_t m = d_parse_expr_term_apply(_t, O_MATRIX, c, rows, 0u,
                                                   0u);

        _t->nodes[m].flags |= D_PARSE_EXPR_NODE_ROWS;

        return m;
    }

    for (uint32_t i = 0u; i < items; ++i)
    {
        c[lead + i] = d_tests_expr_gen(_t, _rng, _depth);
    }

    return d_parse_expr_term_apply(_t,
                                   (_kind == 0u) ? O_CALL
                                 : (_kind == 1u) ? O_INDEX
                                                 : O_VECTOR,
                                   c, lead + items, 0u, 0u);
}

/*
d_tests_expr_gen_chain
  A random chain of one to three links, joined by `and` and flagged CHAIN,
each operand between two links one node.
*/
static uint32_t
d_tests_expr_gen_chain(
    struct dj_parse_expr_term* _t,
    uint32_t*                  _rng,
    uint32_t                   _depth
)
{
    static const uint32_t RELATIONS[6] = { O_EQ, O_NE, O_LT, O_LE, O_GT,
                                           O_GE };
    const uint32_t        links        = 1u + d_tests_expr_random(_rng, 3u);
    uint32_t              x[4];
    uint32_t              joined       = D_PARSE_EXPR_NO_NODE;

    for (uint32_t i = 0u; i <= links; ++i)
    {
        x[i] = d_tests_expr_gen(_t, _rng, _depth);
    }

    for (uint32_t i = 0u; i < links; ++i)
    {
        const uint32_t pair[2] = { x[i], x[i + 1u] };
        const uint32_t link    = d_parse_expr_term_apply(
            _t, RELATIONS[d_tests_expr_random(_rng, 6u)], pair, 2u, 0u, 0u);

        if (i == 0u)
        {
            joined = link;

            continue;
        }

        const uint32_t both[2] = { joined, link };

        joined = d_parse_expr_term_apply(_t, O_AND, both, 2u, 0u, 0u);
        _t->nodes[joined].flags |= D_PARSE_EXPR_NODE_CHAIN;
    }

    return joined;
}

/*
d_tests_expr_gen
  A random term of math's, of every form, to a depth.
*/
static uint32_t
d_tests_expr_gen(
    struct dj_parse_expr_term* _t,
    uint32_t*                  _rng,
    uint32_t                   _depth
)
{
    static const uint32_t BINARY[8] = { O_ADD, O_SUB, O_MUL, O_DIV, O_POW,
                                        O_MOD, O_AND, O_OR };
    static const uint32_t UNARY[5]  = { O_NEG, O_POS, O_FACT, O_TRANS,
                                        O_NOT };

    // leaves where the depth is spent, and now and then before
    if ( (_depth == 0u) ||
         (d_tests_expr_random(_rng, 4u) == 0u) )
    {
        return d_tests_expr_gen_leaf(_t, _rng, false);
    }

    const uint32_t form = d_tests_expr_random(_rng, 9u);

    if (form <= 1u)
    {
        const uint32_t pair[2] = { d_tests_expr_gen(_t, _rng, _depth - 1u),
                                   d_tests_expr_gen(_t, _rng, _depth - 1u) };

        return d_parse_expr_term_apply(_t,
                                       BINARY[d_tests_expr_random(_rng, 8u)],
                                       pair, 2u, 0u, 0u);
    }

    if (form <= 3u)
    {
        const uint32_t one = d_tests_expr_gen(_t, _rng, _depth - 1u);
        const uint32_t op  = (form == 2u)
                             ? UNARY[d_tests_expr_random(_rng, 5u)]
                             : O_ABS;

        return d_parse_expr_term_apply(_t, op, &one, 1u, 0u, 0u);
    }

    if (form <= 7u)
    {
        return d_tests_expr_gen_list(_t, _rng, _depth - 1u, form - 4u);
    }

    return d_tests_expr_gen_chain(_t, _rng, _depth - 1u);
}

/*
d_tests_expr_spans
  A leaf is its span's text, an empty span nothing, and a span the text does
not hold its value (lane G, 11-06: awk's empty block read raw).
*/
static bool
d_tests_expr_spans(
    const struct dj_parse_expr_signature* _s
)
{
    static const char* const            TEXT = "ab cd";
    struct dj_parse_expr_term           t;
    struct dj_parse_expr_render_options plain;
    char                                empty[8];
    char                                outside[8];

    memset(&plain, 0, sizeof(plain));
    plain.text        = TEXT;
    plain.text_length = strlen(TEXT);
    d_parse_expr_term_init_heap(&t);

    const uint32_t none = d_parse_expr_term_leaf(&t, C_NAME, 7u, 2u, 0u);
    const uint32_t far  = d_parse_expr_term_leaf(&t, C_NAME, 7u, 9u, 1u);
    const size_t   n    = d_parse_expr_render_buffer(_s, &t, none, &plain,
                                                     empty, sizeof(empty));
    const size_t   m    = d_parse_expr_render_buffer(_s, &t, far, &plain,
                                                     outside,
                                                     sizeof(outside));
    const bool     ok   = d_tests_expr_expect(
                              (n == 0u) && (empty[0] == '\0') && (m == 1u) &&
                              (strcmp(outside, "7") == 0),
                              "an empty span renders as nothing, a span "
                              "outside the text as its value", __LINE__);

    d_parse_expr_term_release(&t);

    return ok;
}

/*
d_tests_expr_roundtrip
  Rendering and reading back.
  Tests the following:
  - each of 3,000 random terms of every form, rendered, reads back as the
    same term, its brackets only where the parser needs them
  - the design's renderings: `- -x`, `A ^ (2')`, `x'[1]`, `(-x)[1]`
  - a buffer too small is cut and NUL-terminated, the whole length returned
  - a leaf is its span's text, nothing for an empty span, its value for a
    span the text does not hold
*/
static bool
d_tests_expr_roundtrip(void)
{
    struct dj_parse_expr_signature      s;
    struct dj_parse_expr_render_options o;
    static struct dj_tests_expr_run     run;
    char                                text[2048];
    uint32_t                            rng  = 2026u;
    uint32_t                            lost = 0u;
    bool                                ok   = d_tests_expr_math(&s, true);

    memset(&o, 0, sizeof(o));
    o.atom = d_tests_expr_atom_name;

    for (uint32_t i = 0u; ( (ok) && (i < 3000u) ); ++i)
    {
        struct dj_parse_expr_term t;

        d_parse_expr_term_init_heap(&t);

        const uint32_t root = d_tests_expr_gen(&t, &rng, 5u);
        const size_t   n    = d_parse_expr_render_buffer(&s, &t, root, &o,
                                                          text, sizeof(text));

        memset(&run, 0, sizeof(run));

        const bool same =
            ( (n < sizeof(text))                                       &&
              (d_tests_expr_parse(&run, &s, text, 0u) ==
               D_PARSE_EXPR_OK)                                        &&
              (d_parse_expr_term_equal(&t, root, &run.term,
                                       run.result.root, false)) );

        if (!same)
        {
            printf("    a random term rendered as '%s' read back otherwise\n",
                   text);
            ++lost;
            ok = (lost < 5u);
        }

        d_parse_expr_term_release(&run.term);
        d_parse_expr_term_release(&t);
    }

    ok = d_tests_expr_expect(ok && (lost == 0u),
                             "every random term reads back as itself",
                             __LINE__);

    // the renderings the design names
    static const char* const SHOWN[5][2] =
    {
        { "-(-x)", "- -x" }, { "A ^ (2')", "A ^ (2')" },
        { "(x')[1]", "x'[1]" }, { "(-x)[1]", "(-x)[1]" },
        { "(a + not b) <= n", "(a + not b) <= n" }
    };

    for (int i = 0; i < 5; ++i)
    {
        struct dj_parse_expr_render_options plain;

        memset(&run, 0, sizeof(run));
        memset(&plain, 0, sizeof(plain));
        (void)d_tests_expr_parse(&run, &s, SHOWN[i][0], 0u);
        plain.text        = SHOWN[i][0];
        plain.text_length = strlen(SHOWN[i][0]);
        (void)d_parse_expr_render_buffer(&s, &run.term, run.result.root,
                                         &plain, text, sizeof(text));
        ok = d_tests_expr_expect(ok && (strcmp(text, SHOWN[i][1]) == 0),
                                 SHOWN[i][1], __LINE__);

        if ( (ok) && (i == 3) )
        {
            const size_t whole = d_parse_expr_render_buffer(
                &s, &run.term, run.result.root, &plain, text, 4u);

            ok = d_tests_expr_expect((whole == 7u) &&
                                     (strcmp(text, "(-x") == 0),
                                     "a short buffer is cut", __LINE__);
        }

        d_parse_expr_term_release(&run.term);
    }

    ok = (ok) && (d_tests_expr_spans(&s));

    d_parse_expr_signature_release(&s);

    return ok;
}

// dj_tests_expr_eval
//   struct: an evaluation's context: how many leaves it folded.
struct dj_tests_expr_eval
{
    uint32_t leaves;
};

/*
d_tests_expr_eval
  An algebra: integer arithmetic over numbers 0 to 4, relations as 0 or 1,
`and` as both; anything else fails the fold.
*/
static int
d_tests_expr_eval(
    void*                                 _ctx,
    const struct dj_parse_expr_fold_view* _view,
    void*                                 _out)
{
    const struct dj_parse_expr_node* node = &_view->term->nodes[_view->node];
    long*                            out  = (long*)_out;

    if (node->kind == D_PARSE_EXPR_NODE_LEAF)
    {
        ++((struct dj_tests_expr_eval*)_ctx)->leaves;
        *out = (long)node->first - 14L;

        return (node->op == C_NUM) ? 0 : 1;
    }

    if (node->kind == D_PARSE_EXPR_NODE_HOLE)
    {
        *out = 0L;

        return 0;
    }

    const long a = *(const long*)d_parse_expr_fold_child(_view, 0u);
    const long b = (node->count > 1u)
                   ? *(const long*)d_parse_expr_fold_child(_view, 1u)
                   : 0L;

    *out = (node->op == O_ADD) ? (a + b)
         : (node->op == O_SUB) ? (a - b)
         : (node->op == O_MUL) ? (a * b)
         : (node->op == O_NEG) ? (-a)
         : (node->op == O_LT)  ? (long)(a < b)
         : (node->op == O_AND) ? (long)( (a != 0L) && (b != 0L) )
                               : 0L;

    return ( (node->op == O_ADD) || (node->op == O_SUB) ||
             (node->op == O_MUL) || (node->op == O_NEG) ||
             (node->op == O_LT)  || (node->op == O_AND) ) ? 0 : 2;
}

/*
d_tests_expr_skip_all
  A walk's entry that skips every node's children.
*/
static int
d_tests_expr_skip_all(
    void*                            _ctx,
    const struct dj_parse_expr_term* _term,
    uint32_t                         _node,
    uint32_t                         _parent
)
{
    (void)_term;
    (void)_node;
    (void)_parent;
    ++*(int*)_ctx;

    return D_PARSE_EXPR_WALK_SKIP;
}

/*
d_tests_expr_fold_walk
  Folds and walks.
  Tests the following:
  - a fold computes bottom-up in one pass, the operand a chain shares folded
    once
  - an algebra's failure stops the fold and is its answer
  - a term with holes is refused unless the fold asks for them
  - a walk's skip leaves a node's children unvisited, and leave still runs
*/
static bool
d_tests_expr_fold_walk(void)
{
    struct dj_parse_expr_signature  s;
    static struct dj_tests_expr_run run;
    struct dj_tests_expr_eval       eval;
    long                            value   = 0L;
    int                             entered = 0;
    bool                            ok      = d_tests_expr_math(&s, true);

    memset(&run, 0, sizeof(run));
    memset(&eval, 0, sizeof(eval));
    (void)d_tests_expr_parse(&run, &s, "-(1 + 2) * 3 + 4", 0u);
    ok = d_tests_expr_expect(ok && (d_parse_expr_fold(
                                 &run.term, run.result.root, sizeof(long),
                                 NULL, NULL, d_tests_expr_eval, &eval, 0u,
                                 &value) == 0) && (value == -5L),
                             "a fold computes", __LINE__);
    d_parse_expr_term_release(&run.term);

    memset(&run, 0, sizeof(run));
    memset(&eval, 0, sizeof(eval));
    (void)d_tests_expr_parse(&run, &s, "1 < 2 < 3", 0u);
    ok = d_tests_expr_expect(ok && (d_parse_expr_fold(
                                 &run.term, run.result.root, sizeof(long),
                                 NULL, NULL, d_tests_expr_eval, &eval, 0u,
                                 &value) == 0) && (value == 1L) &&
                             (eval.leaves == 3u),
                             "a shared operand is folded once", __LINE__);
    d_parse_expr_term_release(&run.term);

    memset(&run, 0, sizeof(run));
    (void)d_tests_expr_parse(&run, &s, "1 + x", 0u);
    ok = d_tests_expr_expect(ok && (d_parse_expr_fold(
                                 &run.term, run.result.root, sizeof(long),
                                 NULL, NULL, d_tests_expr_eval, &eval, 0u,
                                 &value) == 1),
                             "an algebra's failure is the answer", __LINE__);
    d_parse_expr_term_release(&run.term);

    memset(&run, 0, sizeof(run));
    (void)d_tests_expr_parse(&run, &s, "1 +", 0u);
    ok = d_tests_expr_expect(ok && (d_parse_expr_fold(
                                 &run.term, run.result.root, sizeof(long),
                                 NULL, NULL, d_tests_expr_eval, &eval, 0u,
                                 &value) == D_PARSE_EXPR_FOLD_HAS_HOLES) &&
                             (d_parse_expr_fold(
                                 &run.term, run.result.root, sizeof(long),
                                 NULL, NULL, d_tests_expr_eval, &eval,
                                 D_PARSE_EXPR_FOLD_HOLES, &value) == 0) &&
                             (value == 1L),
                             "holes are folded only when asked", __LINE__);
    ok = d_tests_expr_expect(ok && (d_parse_expr_walk(
                                 &run.term, run.result.root,
                                 d_tests_expr_skip_all, NULL, &entered)) &&
                             (entered == 1),
                             "a skip leaves the children unvisited",
                             __LINE__);
    d_parse_expr_term_release(&run.term);
    d_parse_expr_signature_release(&s);

    return ok;
}

/*
d_tests_expr_locate
  Lines and columns.
  Tests the following:
  - `\n`, `\r\n` and a lone `\r` each end one line
  - a column counts characters, a two-byte UTF-8 character as one
  - an offset past the end is the end
*/
static bool
d_tests_expr_locate(void)
{
    static const char TEXT[] = "ab\ncd\r\nef\rg\xc3\xa9";
    static const uint32_t WHERE[7][3] =
    {
        { 0u, 1u, 1u }, { 3u, 2u, 1u }, { 5u, 2u, 3u }, { 7u, 3u, 1u },
        { 10u, 4u, 1u }, { 13u, 4u, 3u }, { 99u, 4u, 3u }
    };
    bool ok = true;

    for (int i = 0; i < 7; ++i)
    {
        uint32_t line   = 0u;
        uint32_t column = 0u;

        d_parse_expr_locate(TEXT, sizeof(TEXT) - 1u, WHERE[i][0], &line,
                            &column);
        ok = d_tests_expr_expect(ok && (line == WHERE[i][1]) &&
                                 (column == WHERE[i][2]),
                                 "a line and a column", __LINE__);
    }

    return ok;
}


//==============================================================================
// 3.  THE SUITE
//==============================================================================


// d_tests_parse_expr_c
//   constant: the suite's sections, in the order they run.
static const struct dj_tests_section d_tests_parse_expr_c[] =
{
    { "parse_expr_term",        d_tests_expr_term        },
    { "parse_expr_signature",   d_tests_expr_signature   },
    { "parse_expr_math_cases",  d_tests_expr_math_cases  },
    { "parse_expr_cli_cases",   d_tests_expr_cli_cases   },
    { "parse_expr_awk_cases",   d_tests_expr_awk_cases   },
    { "parse_expr_dawk_cases",  d_tests_expr_dawk_cases  },
    { "parse_expr_stop",        d_tests_expr_stop        },
    { "parse_expr_hooks",       d_tests_expr_hooks       },
    { "parse_expr_diagnostics", d_tests_expr_diagnostics },
    { "parse_expr_limits",      d_tests_expr_limits      },
    { "parse_expr_roundtrip",   d_tests_expr_roundtrip   },
    { "parse_expr_fold_walk",   d_tests_expr_fold_walk   },
    { "parse_expr_locate",      d_tests_expr_locate      }
};

/*
main
  Runs every section, reporting each as it finishes, so a crash still shows
which one it was in.
*/
int
main(void)
{
    const size_t count = sizeof(d_tests_parse_expr_c) /
                         sizeof(d_tests_parse_expr_c[0]);
    int          passed = 0;
    int          failed = 0;

    printf("djinterp expression core v2: C tests\n");

    for (size_t i = 0u; i < count; ++i)
    {
        const bool held = d_tests_parse_expr_c[i].run();

        printf("  [%s] %s\n", (held) ? "PASS" : "FAIL",
               d_tests_parse_expr_c[i].name);

        if (held)
        {
            ++passed;
        }
        else
        {
            ++failed;
        }
    }

    printf("passed: %d   failed: %d\n", passed, failed);

    return (failed == 0) ? 0 : 1;
}
