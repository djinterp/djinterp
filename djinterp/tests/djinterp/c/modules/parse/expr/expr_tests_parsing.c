/*******************************************************************************
* djinterp [c/parse]                                        expr_tests_parsing.c
*
* Expression core v2's C suite, block "parsing": the stop option, the
* sub-parse and hole modes, hooks, the diagnostics, and the limits.
*
*
* path:      /tests/djinterp/c/modules/parse/expr/expr_tests_parsing.c
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.18
*******************************************************************************/
#include "./expr_tests.h"  // corresponding header

/*
dj_tests_expr_code_row
  struct: one row of parse.expr's code table, as text, the way the codes
manifest and the log read it.
*/
struct dj_tests_expr_code_row
{
    const char* module;
    int         number;
    const char* name;
    const char* cls;
    const char* grade;
    const char* text;
    const char* facts;
};

#define D_TESTS_EXPR_CODE_ROW(_module, _number, _name, _class, _grade,        \
                              _template, _facts)                              \
    { #_module, (_number), #_name, #_class, #_grade, (_template), #_facts },

static const struct dj_tests_expr_code_row D_TESTS_EXPR_CODE_ROWS[] =
{
    D_PARSE_EXPR_CODES(D_TESTS_EXPR_CODE_ROW)
};

/*
d_tests_expr_has_fact
  Whether a row's list of facts, as its brackets spell it ("(a, b)"), holds a
name of a length.
*/
static bool
d_tests_expr_has_fact(
    const char* _facts,
    const char* _name,
    size_t      _length
)
{
    const char* at = _facts;

    while (*at != '\0')
    {
        size_t run = strcspn(at, "(), ");

        if ( (run == _length) &&
             (run > 0u)       &&
             (strncmp(at, _name, _length) == 0) )
        {
            return true;
        }

        at += (run > 0u) ? run : 1u;
    }

    return false;
}

/*
d_tests_expr_template_holds
  Whether every fact a row's template names, in braces, is among its facts.
*/
static bool
d_tests_expr_template_holds(
    const struct dj_tests_expr_code_row* _row
)
{
    const char* at = _row->text;

    while ( (at = strchr(at, '{')) != NULL )
    {
        const char* close = strchr(at, '}');

        if ( (!close) ||
             (!d_tests_expr_has_fact(_row->facts, at + 1,
                                     (size_t)(close - at - 1))) )
        {
            D_TEST_NOTE("row %d's template names a fact it does not carry",
                        _row->number);

            return false;
        }

        at = close + 1;
    }

    return true;
}

/*
d_tests_expr_code_table
  parse.expr's code table, as the codes manifest and the log read it.
  Tests the following:
  - a row for each code, numbered 0 to 9 in order, each parse_expr's and
    each name its own
  - every row graded rejected, in the syntax family or one of the three
    other classes its codes take (quantity.depth, constraint.violated,
    argument.invalid)
  - every fact a template names is among its row's facts
*/
bool
d_tests_expr_code_table(void)
{
    static const char* const CLASSES[] =
    {
        "SYNTAX_UNEXPECTED",   "SYNTAX_MISSING", "SYNTAX_UNBALANCED",
        "SYNTAX_AMBIGUOUS",    "SYNTAX_ARITY",   "QUANTITY_DEPTH",
        "CONSTRAINT_VIOLATED", "ARGUMENT_INVALID"
    };
    const size_t count   = sizeof(D_TESTS_EXPR_CODE_ROWS) /
                           sizeof(D_TESTS_EXPR_CODE_ROWS[0]);
    bool         ordered = (count == (size_t)D_PARSE_EXPR_DIAG_BAD_SIGNATURE +
                                     1u);
    bool         named   = true;
    bool         graded  = true;
    bool         facts   = true;

    for (size_t i = 0u; i < count; ++i)
    {
        const struct dj_tests_expr_code_row* row   = &D_TESTS_EXPR_CODE_ROWS[i];
        bool                                 known = false;

        ordered = (ordered) && (row->number == (int)i);
        named   = (named) && (strcmp(row->module, "parse_expr") == 0);

        for (size_t k = 0u; k < i; ++k)
        {
            named = (named) &&
                    (strcmp(row->name, D_TESTS_EXPR_CODE_ROWS[k].name) != 0);
        }

        for (size_t k = 0u; k < sizeof(CLASSES) / sizeof(CLASSES[0]); ++k)
        {
            known = (known) || (strcmp(row->cls, CLASSES[k]) == 0);
        }

        graded = (graded) && (known) && (strcmp(row->grade, "REJECTED") == 0);
        facts  = (facts) && (d_tests_expr_template_holds(row));
    }

    named = (named) &&
            (strcmp(D_TESTS_EXPR_CODE_ROWS[D_PARSE_EXPR_DIAG_MISMATCHED].name,
                    "mismatched") == 0) &&
            (strcmp(D_TESTS_EXPR_CODE_ROWS[D_PARSE_EXPR_DIAG_REFUSED].cls,
                    "CONSTRAINT_VIOLATED") == 0) &&
            (strcmp(D_TESTS_EXPR_CODE_ROWS[D_PARSE_EXPR_DIAG_REFUSED].facts,
                    "()") == 0);

    // each check is reported, whatever the one before it found
    bool ok = d_tests_expr_expect(ordered,
                                  "a row for each code, in order",
                                  __LINE__);

    ok = (d_tests_expr_expect(named,
                              "each row parse_expr's, its name its own",
                              __LINE__)) && (ok);
    ok = (d_tests_expr_expect(graded,
                              "each row rejected, in a known class",
                              __LINE__)) && (ok);
    ok = (d_tests_expr_expect(facts,
                              "each template names only its facts",
                              __LINE__)) && (ok);

    return ok;
}


#if (D_INTERNAL_PARSE_EXPR_HEAP == 1)

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
bool
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
            D_TEST_NOTE("'%s' read as %s (status %d), next %lu at %lu",
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

// dj_tests_expr_sub
//   struct: a text, the mode and flags it is read in, and what it must give:
// its status, its term where it is read whole, and the token it ends at.
struct dj_tests_expr_sub
{
    const char* text;
    uint32_t    mode;
    uint8_t     flags;
    const char* term;
    int         status;
    uint32_t    next_class;
    uint32_t    next_offset;
};

/*
d_tests_expr_subparse_one
  Reads one case of the sub-parse section and holds it to what it must give.
*/
static bool
d_tests_expr_subparse_one(
    const struct dj_parse_expr_signature* _sig,
    const struct dj_tests_expr_sub*       _case
)
{
    static struct dj_tests_expr_run run;
    struct dj_tests_expr_text       b;

    memset(&run, 0, sizeof(run));
    run.options.mode  = _case->mode;
    run.options.flags = _case->flags;
    (void)d_tests_expr_parse(&run, _sig, _case->text, D_TESTS_EXPR_LEX_DAWK);

    const char* got  = d_tests_expr_sexpr(&run.term, run.result.root, &b);
    const bool  held =
        ( (run.status == _case->status)                              &&
          ( (run.status != D_PARSE_EXPR_OK) ||
            ( (strcmp(got, _case->term) == 0) &&
              (d_parse_expr_term_verify(&run.term)) ) )              &&
          (run.result.next.cls == _case->next_class)                 &&
          (run.result.next.offset == _case->next_offset) );

    if (!held)
    {
        D_TEST_NOTE("'%s' read as %s (status %d), next %lu at %lu",
                    _case->text, got, run.status,
                    (unsigned long)run.result.next.cls,
                    (unsigned long)run.result.next.offset);
    }

    d_parse_expr_term_release(&run.term);

    return held;
}

/*
d_tests_expr_sub_declines
  A hook that runs a sub-parse, then declines: the tokens it read cannot be
read again, so the parse stops.
*/
static int
d_tests_expr_sub_declines(
    void*                             _ctx,
    struct dj_parse_expr_parser*      _parser,
    const struct dj_parse_expr_token* _token,
    uint32_t                          _left,
    uint32_t*                         _out
)
{
    uint32_t node = D_PARSE_EXPR_NO_NODE;

    (void)_ctx;
    (void)_token;
    (void)_left;
    (void)_out;
    (void)d_parse_expr_sub(_parser, D_PARSE_EXPR_FLOOR_NONE, 0u, &node);

    return 0;
}

/*
d_tests_expr_subparse
  The sub-parse (design 6.5) and the modes of mixfix holes (F18), on dawk's
table: `getline < file` reads its file by the sub-parse at concatenation's
floor, and `print`'s list is read in a mode without `>`, which a bracket's
holes give back (lane H's points 4 and 5).
  Tests the following:
  - the floor holds back concatenation and every looser operator, which then
    apply to the hook's node: `getline < f x`, `getline < f > 0`
  - an operator tighter than the floor is taken: `getline < f + 1`
  - inside a bracket of its own the floor holds nothing back; a bracket
    open around it is the enclosing parse's, whose separator ends it
  - an operand missing at the end, and a bracket the sub-parse never closes,
    are errors, counted as the parse's
  - in mode 2 `>` ends the expression; inside a bracket it compares, and
    after the bracket it ends it again; a sub-parse reads in the parse's mode
  - a hole mode of two bits is refused when the signature is sealed
  - a hook that declines after a sub-parse stops the parse
*/
bool
d_tests_expr_subparse(void)
{
    static const struct dj_tests_expr_sub CASES[] =
    {
        { "getline < f", 1u, 0u, "getfile(f)", D_PARSE_EXPR_OK,
          D_PARSE_EXPR_CLASS_END, 11u },
        { "getline line < f", 1u, 0u, "getfile(line, f)", D_PARSE_EXPR_OK,
          D_PARSE_EXPR_CLASS_END, 16u },
        { "getline < f x", 1u, 0u, "concat(getfile(f), x)", D_PARSE_EXPR_OK,
          D_PARSE_EXPR_CLASS_END, 13u },
        { "getline < (a b)", 1u, 0u, "getfile(concat(a, b))", D_PARSE_EXPR_OK,
          D_PARSE_EXPR_CLASS_END, 15u },
        { "getline < f + 1", 1u, 0u, "getfile(add(f, 1))", D_PARSE_EXPR_OK,
          D_PARSE_EXPR_CLASS_END, 15u },
        { "getline < f > 0", 1u, 0u, "gt(getfile(f), 0)", D_PARSE_EXPR_OK,
          D_PARSE_EXPR_CLASS_END, 15u },
        { "x = getline < f", 1u, 0u, "assign(x, getfile(f))",
          D_PARSE_EXPR_OK, D_PARSE_EXPR_CLASS_END, 15u },
        { "(getline < f) x", 1u, 0u, "concat(getfile(f), x)",
          D_PARSE_EXPR_OK, D_PARSE_EXPR_CLASS_END, 15u },
        { "(getline < f x)", 1u, 0u, "concat(getfile(f), x)",
          D_PARSE_EXPR_OK, D_PARSE_EXPR_CLASS_END, 15u },
        { "f(getline < g, x)", 1u, 0u, "call(f, getfile(g), x)",
          D_PARSE_EXPR_OK, D_PARSE_EXPR_CLASS_END, 17u },
        { "getline <", 1u, 0u, "", D_PARSE_EXPR_ERROR,
          D_PARSE_EXPR_CLASS_END, 9u },
        { "getline < (a", 1u, 0u, "", D_PARSE_EXPR_ERROR,
          D_PARSE_EXPR_CLASS_END, 12u },
        { "a > b", 2u, D_PARSE_EXPR_OPTION_STOP, "a", D_PARSE_EXPR_OK,
          C_GT, 2u },
        { "(a > b) > c", 2u, D_PARSE_EXPR_OPTION_STOP, "gt(a, b)",
          D_PARSE_EXPR_OK, C_GT, 8u },
        { "f(a > b)", 2u, D_PARSE_EXPR_OPTION_STOP, "call(f, gt(a, b))",
          D_PARSE_EXPR_OK, D_PARSE_EXPR_CLASS_END, 8u },
        { "a b > c", 2u, D_PARSE_EXPR_OPTION_STOP, "concat(a, b)",
          D_PARSE_EXPR_OK, C_GT, 4u },
        { "getline < f > 0", 2u, D_PARSE_EXPR_OPTION_STOP, "getfile(f)",
          D_PARSE_EXPR_OK, C_GT, 12u },
        { "a > b", 1u, D_PARSE_EXPR_OPTION_STOP, "gt(a, b)", D_PARSE_EXPR_OK,
          D_PARSE_EXPR_CLASS_END, 5u }
    };
    struct dj_parse_expr_signature s;
    bool                           ok = d_tests_expr_dawk(&s);

    for (size_t i = 0u; i < sizeof(CASES) / sizeof(CASES[0]); ++i)
    {
        ok = (d_tests_expr_subparse_one(&s, &CASES[i])) && (ok);
    }

    d_parse_expr_signature_release(&s);

    // a hole mode of two modes is refused
    {
        struct dj_parse_expr_signature bad;
        struct dj_parse_expr_mixfix    group =
            d_tests_expr_mixfix_of(0u, C_LP, "(", C_RP, ")");

        group.hole_mode = 3u;
        d_parse_expr_signature_init_heap(&bad);
        ok = d_tests_expr_expect(
                 (ok) &&
                 (d_parse_expr_define_atom(&bad, C_NAME, 0u)) &&
                 (d_parse_expr_define_mixfix(&bad, O_GROUP, 0, 0u,
                                             D_PARSE_EXPR_OP_TRANSPARENT,
                                             &group)) &&
                 (d_parse_expr_define_token(&bad, C_LP,
                                            D_PARSE_EXPR_OPERAND, O_GROUP,
                                            0u)) &&
                 (!d_parse_expr_signature_seal(&bad, NULL)),
                 "a hole mode of two bits is refused", __LINE__);
        d_parse_expr_signature_release(&bad);
    }

    // declining after a sub-parse is a bad argument, and fatal
    {
        static struct dj_tests_expr_run run;
        struct dj_parse_expr_signature  q;

        memset(&run, 0, sizeof(run));
        d_parse_expr_signature_init_heap(&q);
        ok = d_tests_expr_expect(
                 (ok) &&
                 (d_parse_expr_define_atom(&q, C_NAME, 0u)) &&
                 (d_tests_expr_dawk_custom(&q, C_QMARK, D_PARSE_EXPR_OPERAND,
                                           0u, 0u,
                                           d_tests_expr_sub_declines)) &&
                 (d_parse_expr_signature_seal(&q, NULL)) &&
                 (d_tests_expr_parse(&run, &q, "? a",
                                     D_TESTS_EXPR_LEX_DAWK) ==
                  D_PARSE_EXPR_FATAL),
                 "a hook may not decline after a sub-parse", __LINE__);
        d_parse_expr_term_release(&run.term);
        d_parse_expr_signature_release(&q);
    }

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
bool
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
d_tests_expr_message_of
  Parses a text by math's signature, with the text given to the options or
not, and whether the error's message is the one expected.
*/
static bool
d_tests_expr_message_of(
    const struct dj_parse_expr_signature* _sig,
    const char*                           _text,
    bool                                  _given,
    const char*                           _expected
)
{
    static struct dj_tests_expr_run   run;
    struct dj_parse_expr_source       source;
    struct dj_parse_expr_array_source state;
    const char*                       got  = "";
    bool                              held = false;
    int                               count;

    memset(&run, 0, sizeof(run));
    count = d_tests_expr_lex(_text, 0u, run.tokens, 4096);
    d_parse_diag_sink_init(&run.sink, run.items, 32u, run.text,
                           (uint32_t)sizeof(run.text));
    d_parse_expr_term_init_heap(&run.term);
    d_parse_expr_source_array(&source, &state, run.tokens,
                              (count > 0) ? (uint32_t)count : 0u,
                              (uint32_t)strlen(_text));
    run.options.text        = (_given) ? _text : NULL;
    run.options.text_length = (_given) ? strlen(_text) : 0u;
    run.status = d_parse_expr_parse(_sig, &source, &run.options, &run.term,
                                    &run.sink, &run.result);

    if (run.sink.count > 0u)
    {
        got = d_parse_diag_message(&run.sink, &run.items[0]);
    }

    held = ( (run.status == D_PARSE_EXPR_ERROR) &&
             (got)                              &&
             (strcmp(got, _expected) == 0) );

    if (!held)
    {
        D_TEST_NOTE("'%s'%s: \"%s\", not \"%s\"", _text,
                    (_given) ? "" : " without its text", (got) ? got : "",
                    _expected);
    }

    d_parse_expr_term_release(&run.term);

    return held;
}

/*
d_tests_expr_mismatched_messages
  A mismatched close names both brackets, and a parse given no text names
each token by its spelling in the signature.
*/
static bool
d_tests_expr_mismatched_messages(
    const struct dj_parse_expr_signature* _math,
    bool                                  _ok
)
{
    bool ok = _ok;

    ok = d_tests_expr_expect(ok && (d_tests_expr_message_of(
                                 _math, "(1 + 2]", true,
                                 "']' does not close '('")),
                             "a close of another kind names both brackets",
                             __LINE__);
    ok = d_tests_expr_expect(ok && (d_tests_expr_message_of(
                                 _math, "f([1, 2)", true,
                                 "')' closes '(' while '[' is open")),
                             "an outer operator's close names all three",
                             __LINE__);
    ok = d_tests_expr_expect(ok && (d_tests_expr_message_of(
                                 _math, "(1 + 2]", false,
                                 "']' does not close '('")),
                             "without the text, the spellings stand in",
                             __LINE__);
    ok = d_tests_expr_expect(ok && (d_tests_expr_message_of(
                                 _math, "f([1, 2)", false,
                                 "')' closes '(' while '[' is open")),
                             "without the text, a frame's opener is its first "
                             "part's spelling", __LINE__);
    ok = d_tests_expr_expect(ok && (d_tests_expr_message_of(
                                 _math, "1 + * 2", false,
                                 "unexpected '*'")),
                             "without the text, an operator's own spelling",
                             __LINE__);
    ok = d_tests_expr_expect(ok && (d_tests_expr_message_of(
                                 _math, "(1 + 2", false,
                                 "'(' is never closed")),
                             "without the text, an unclosed opener's "
                             "spelling", __LINE__);

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
  - a mismatched close names both brackets, and an outer operator's close
    all three; given no text, a message names each token by its spelling
  - the language's check refuses a node, as its own error
*/
bool
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

    ok = d_tests_expr_mismatched_messages(&math, ok);

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
bool
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

#else

/*
d_tests_expr_stop
  The heap compiled out: skipped.
*/
bool
d_tests_expr_stop(void)
{
    D_TEST_SKIP(D_TESTS_EXPR_NO_HEAP);
}

/*
d_tests_expr_subparse
  The heap compiled out: skipped.
*/
bool
d_tests_expr_subparse(void)
{
    D_TEST_SKIP(D_TESTS_EXPR_NO_HEAP);
}

/*
d_tests_expr_hooks
  The heap compiled out: skipped.
*/
bool
d_tests_expr_hooks(void)
{
    D_TEST_SKIP(D_TESTS_EXPR_NO_HEAP);
}

/*
d_tests_expr_diagnostics
  The heap compiled out: skipped.
*/
bool
d_tests_expr_diagnostics(void)
{
    D_TEST_SKIP(D_TESTS_EXPR_NO_HEAP);
}

/*
d_tests_expr_limits
  The heap compiled out: skipped.
*/
bool
d_tests_expr_limits(void)
{
    D_TEST_SKIP(D_TESTS_EXPR_NO_HEAP);
}

#endif  // D_INTERNAL_PARSE_EXPR_HEAP
