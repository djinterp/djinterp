/*******************************************************************************
* djinterp [c/parse]                                       expr_tests_fixtures.c
*
* Expression core v2's C suite: the fixtures every section uses -- the
* checks, the test lexer, terms written as s-expressions, a parse made
* whole, and the four languages, each a table built on the core.
*
*
* path:      /tests/djinterp/c/modules/parse/expr/expr_tests_fixtures.c
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "./expr_tests.h"  // corresponding header

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
bool
d_tests_expr_expect(
    bool        _held,
    const char* _what,
    int         _line
)
{
    if (!_held)
    {
        D_TEST_NOTE("failed (line %d): %s", _line, _what);
    }

    return _held;
}


// OP_NAMES
//   constant: each operator's name, as the expected terms spell it.
static const char* const OP_NAMES[O_COUNT] =
{
    "?", "add", "sub", "mul", "div", "mod", "pow", "neg", "pos", "fact",
    "trans", "eq", "ne", "lt", "le", "gt", "ge", "and", "or", "not", "call",
    "index", "vector", "matrix", "row", "group", "abs", "seq", "pipe", "eqeq",
    "cmd", "concat", "cond", "if", "preinc", "postinc", "match", "assign",
    "in", "nomatch", "pipeget", "getline", "field", "predec", "postdec",
    "getfile"
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


// 1.3    The lexer
//------------------------------------------------------------------------------
/*
d_tests_expr_atom_value
  An atom's value: its place in ATOM_NAMES, or past them all.
*/
uint32_t
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
int
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
const char*
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


/*
d_tests_expr_parse
  Lexes and parses a text by a signature, the options given or the defaults,
the text given to the options for messages.
*/
int
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
struct dj_parse_expr_mixfix
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
void
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
bool
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
bool
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
bool
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
  `getline [var] [< file]` in operand position: the file is read by the
sub-parse, its floor at concatenation (90), so `getline < f x` is
`concat(getfile(f), x)` and `getline < f > 0` is `gt(getfile(f), 0)`.
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
    struct dj_parse_expr_term* term  = d_parse_expr_parser_term(_parser);
    uint32_t                   end   = _token->offset + _token->length;
    uint32_t                   kids[2];
    uint32_t                   count = 0u;
    bool                       file  = false;
    struct dj_parse_expr_token next;

    (void)_ctx;
    (void)_left;

    kids[0] = d_tests_expr_dawk_target(_parser, &end);
    count   = (kids[0] != D_PARSE_EXPR_NO_NODE) ? 1u : 0u;

    // `< file`: the file is one expression at concatenation's floor
    if ( (d_parse_expr_peek(_parser, D_PARSE_EXPR_OPERATOR, 0u,
                            &next) > 0) &&
         (next.cls == C_LT) )
    {
        file = true;
        (void)d_parse_expr_take(_parser, D_PARSE_EXPR_OPERATOR, 0u, &next);

        if (d_parse_expr_sub(_parser, 90, 0u, &kids[count]) < 0)
        {
            return -1;
        }

        end = term->nodes[kids[count]].offset +
              term->nodes[kids[count]].length;
        ++count;
    }

    *_out = d_parse_expr_term_apply(term, (file) ? O_GETFILE : O_GETLINE,
                                    kids, count, _token->offset,
                                    end - _token->offset);

    return (*_out != D_PARSE_EXPR_NO_NODE) ? 1 : -1;
}

/*
d_tests_expr_dawk_list
  dawk's check: a bracketed list, `(i, j)`, stands only on `in`'s left (lane
H, point 2; `print`'s argument list is the statement parser's).
*/
bool
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
bool
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

    // inside brackets `>` compares again, whatever the mode outside (F18)
    group.hole_mode = 1u;
    call.hole_mode  = 1u;
    index.hole_mode = 1u;

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
d_tests_expr_dawk_ladder
  One rung of DAWK_OPS: its operator and its token's role. `>` is there in
mode 1 alone, so `print`'s mode, 2, reads it as the end of its list (lane
H's point 5).
*/
static bool
d_tests_expr_dawk_ladder(
    struct dj_parse_expr_signature*     _s,
    const struct dj_tests_expr_dawk_op* _r
)
{
    struct dj_parse_expr_role role;

    memset(&role, 0, sizeof(role));
    role.cls      = _r->cls;
    role.position = (_r->form == D_PARSE_EXPR_FORM_PREFIX)
                    ? D_PARSE_EXPR_OPERAND
                    : D_PARSE_EXPR_OPERATOR;
    role.kind     = D_PARSE_EXPR_ROLE_OP;
    role.op       = _r->op;
    role.modes    = (_r->op == O_GT) ? 1u : 0u;

    return ( (d_parse_expr_define_op(_s, _r->op, _r->form, _r->precedence,
                                     _r->assoc, _r->flags, _r->spelling)) &&
             (d_parse_expr_define_role(_s, &role)) );
}

/*
d_tests_expr_dawk
  dawk's expressions, from lane H's review of 11-05: the ladder of DAWK_OPS,
concatenation by juxtaposition at 90, `| getline` at 80, the postfix steps
at 140 taken only after an lvalue, `getline` beginning no concatenated
operand, and the check that keeps a bracketed list before `in`.
*/
bool
d_tests_expr_dawk(
    struct dj_parse_expr_signature* _s
)
{
    bool ok = true;

    d_parse_expr_signature_init_heap(_s);

    for (size_t i = 0u; i < sizeof(DAWK_OPS) / sizeof(DAWK_OPS[0]); ++i)
    {
        ok = ( (ok) &&
               (d_tests_expr_dawk_ladder(_s, &DAWK_OPS[i])) );
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


/*
d_tests_expr_cases
  Each text must parse without an error, as the term given, into a term that
verifies.
*/
bool
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
            D_TEST_NOTE("'%s' read as %s (status %d), not %s",
                        _cases[i].text, got, run.status, _cases[i].term);
        }

        ok = (ok) && (held);
        d_parse_expr_term_release(&run.term);
    }

    return ok;
}


/*
d_tests_expr_errors
  Each text must fail with its error first, at its span, with its count of
errors, the sink holding the error at the result's index.
*/
bool
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
            D_TEST_NOTE("'%s': status %d, code %u at %u+%u, %u errors",
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
d_tests_expr_atom_name
  The round trip's atom writer: a leaf as its atom's name.
*/
bool
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
