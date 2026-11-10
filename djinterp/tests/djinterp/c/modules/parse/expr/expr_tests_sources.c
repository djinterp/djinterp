/*******************************************************************************
* djinterp [c/parse]                                        expr_tests_sources.c
*
* Expression core v2's C suite, block "sources": the default scanner and
* the caller's scratch for the parser's stacks.
*
*
* path:      /tests/djinterp/c/modules/parse/expr/expr_tests_sources.c
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.10
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "./expr_tests.h"  // corresponding header

#if (D_INTERNAL_PARSE_EXPR_SCAN == 1)

// dj_tests_expr_scanned
//   struct: a token the scanner must give: its class, place and flags.
struct dj_tests_expr_scanned
{
    uint32_t cls;
    uint32_t offset;
    uint32_t length;
    uint32_t flags;
};

// dj_tests_expr_named_source
//   struct: the scanner's source, with each name's value set as the test
// lexer sets it, so that the s-expressions name their leaves.
struct dj_tests_expr_named_source
{
    struct dj_parse_expr_source inner;
    const char*                 text;
};

/*
d_tests_expr_named_next
  The scanner's next token, its value its atom's place in ATOM_NAMES.
*/
static int
d_tests_expr_named_next(
    void*                       _ctx,
    uint8_t                     _position,
    uint32_t                    _mode,
    struct dj_parse_expr_token* _out
)
{
    struct dj_tests_expr_named_source* n =
        (struct dj_tests_expr_named_source*)_ctx;
    const int got = n->inner.next(n->inner.ctx, _position, _mode, _out);

    if (got > 0)
    {
        _out->value = d_tests_expr_atom_value(n->text + _out->offset,
                                              _out->length);
    }

    return got;
}

/*
d_tests_expr_scan_signature
  The small signature the scanner's tests read by: names, numbers and
strings, `<` and `<=`, the word `and`, a prefix `-`, and a group, `_group`,
which must outlive the signature.
*/
static bool
d_tests_expr_scan_signature(
    struct dj_parse_expr_signature* _s,
    struct dj_parse_expr_mixfix*    _group
)
{
    *_group = d_tests_expr_mixfix_of(0u, C_LP, "(", C_RP, ")");
    d_tests_expr_list_of(_group, 1u, D_PARSE_EXPR_ITEMS_UNBOUNDED);
    d_parse_expr_signature_init_heap(_s);

    return ( (d_parse_expr_define_atom(_s, C_NAME, 0u))                    &&
             (d_parse_expr_define_atom(_s, C_NUM, 0u))                     &&
             (d_parse_expr_define_atom(_s, C_STRING, 0u))                  &&
             (d_parse_expr_define_op(_s, O_LT, D_PARSE_EXPR_FORM_INFIX, 10,
                                     D_PARSE_EXPR_ASSOC_NONE, 0u, "<"))    &&
             (d_parse_expr_define_op(_s, O_LE, D_PARSE_EXPR_FORM_INFIX, 10,
                                     D_PARSE_EXPR_ASSOC_NONE, 0u, "<="))   &&
             (d_parse_expr_define_op(_s, O_AND, D_PARSE_EXPR_FORM_INFIX, 5,
                                     D_PARSE_EXPR_ASSOC_LEFT,
                                     D_PARSE_EXPR_OP_WORD, "and"))         &&
             (d_parse_expr_define_op(_s, O_NEG, D_PARSE_EXPR_FORM_PREFIX,
                                     20, D_PARSE_EXPR_ASSOC_RIGHT, 0u,
                                     "-"))                                 &&
             (d_parse_expr_define_mixfix(_s, O_GROUP, 0, 0u, 0u, _group)) &&
             (d_parse_expr_define_token(_s, C_LT, D_PARSE_EXPR_OPERATOR,
                                        O_LT, 0u))                         &&
             (d_parse_expr_define_token(_s, C_LE, D_PARSE_EXPR_OPERATOR,
                                        O_LE, 0u))                         &&
             (d_parse_expr_define_token(_s, C_AND, D_PARSE_EXPR_OPERATOR,
                                        O_AND, 0u))                        &&
             (d_parse_expr_define_token(_s, C_MINUS, D_PARSE_EXPR_OPERAND,
                                        O_NEG, 0u))                        &&
             (d_parse_expr_define_token(_s, C_LP, D_PARSE_EXPR_OPERAND,
                                        O_GROUP, 0u))                      &&
             (d_parse_expr_signature_seal(_s, NULL)) );
}

/*
d_tests_expr_scan_holds
  Reads a token from a source for each expected, and holds it to its class,
place and flags; D_PARSE_EXPR_CLASS_END expects the end.
*/
static bool
d_tests_expr_scan_holds(
    struct dj_parse_expr_source*        _source,
    const struct dj_tests_expr_scanned* _want,
    size_t                              _count,
    const char*                         _what,
    int                                 _line
)
{
    bool ok = true;

    for (size_t i = 0u; ( (ok) && (i < _count) ); ++i)
    {
        struct dj_parse_expr_token k;
        const int                  got = _source->next(_source->ctx,
                                                       D_PARSE_EXPR_OPERAND,
                                                       1u, &k);
        const int                  want =
            (_want[i].cls == D_PARSE_EXPR_CLASS_END) ? 0 : 1;

        ok = d_tests_expr_expect( (got == want)                    &&
                                  (k.cls == _want[i].cls)          &&
                                  (k.offset == _want[i].offset)    &&
                                  (k.length == _want[i].length)    &&
                                  (k.flags == _want[i].flags),
                                  _what, _line);
    }

    return ok;
}

/*
d_tests_expr_scan_tokens
  The scanner over a text of every kind of token, by a small signature and
three words of the caller's.
*/
static bool
d_tests_expr_scan_tokens(void)
{
    static const char TEXT[] =
        "a<=b and android 1.5e3 2e (x, 'q\\'s') -\n getline \"open #";
    static const struct dj_parse_expr_scan_word MORE[] =
    {
        { "getline", C_GETLINE }, { "|", C_BAR }, { "<<", C_APPEND }
    };
    static const struct dj_tests_expr_scanned SHORT[] =
    {
        { C_NAME, 0u, 1u, 0u },     { C_APPEND, 1u, 2u, 0u },
        { C_NAME, 3u, 1u, 0u },     { C_NUM, 5u, 1u, 1u },
        { C_QUOTE, 6u, 1u, 0u },    { C_NAME, 7u, 1u, 0u }
    };
    static const struct dj_tests_expr_scanned WANT[] =
    {
        { C_NAME, 0u, 1u, 0u },     { C_LE, 1u, 2u, 0u },
        { C_NAME, 3u, 1u, 0u },     { C_AND, 5u, 3u, 1u },
        { C_NAME, 9u, 7u, 1u },     { C_NUM, 17u, 5u, 1u },
        { C_NUM, 23u, 1u, 1u },     { C_NAME, 24u, 1u, 0u },
        { C_LP, 26u, 1u, 1u },      { C_NAME, 27u, 1u, 0u },
        { C_COMMA, 28u, 1u, 0u },   { C_STRING, 30u, 6u, 1u },
        { C_RP, 36u, 1u, 0u },      { C_MINUS, 38u, 1u, 1u },
        { C_SEMI, 39u, 1u, 0u },    { C_GETLINE, 41u, 7u, 3u },
        { C_QUOTE, 49u, 7u, 1u },   { D_PARSE_EXPR_CLASS_END, 56u, 0u, 0u }
    };
    struct dj_parse_expr_signature   s;
    struct dj_parse_expr_mixfix      group;
    struct dj_parse_expr_scan_config config;
    struct dj_parse_expr_scanner     scanner;
    struct dj_parse_expr_source      source;
    struct dj_parse_expr_token       k;
    bool                             ok;

    memset(&config, 0, sizeof(config));
    config.name_class    = C_NAME;
    config.number_class  = C_NUM;
    config.string_class  = C_STRING;
    config.newline_class = C_SEMI;
    config.stray_class   = C_QUOTE;
    config.words         = MORE;
    config.word_count    = 3u;
    ok = ( (d_tests_expr_scan_signature(&s, &group)) &&
           (d_parse_expr_scan(&source, &scanner, &s, &config, TEXT,
                              sizeof(TEXT) - 1u)) );
    ok = ( (ok) &&
           (d_tests_expr_scan_holds(&source, WANT,
                                    sizeof(WANT) / sizeof(WANT[0]),
                                    "a token as the scanner must read it",
                                    __LINE__)) );

    // a caller's longer spelling before the signature's shorter one, and a
    // point no digit follows
    ok = d_tests_expr_expect(
             (ok) &&
             (d_parse_expr_scan(&source, &scanner, &s, &config, "x<<y 3.e",
                                8u)),
             "a scan of a second text", __LINE__);
    ok = ( (ok) &&
           (d_tests_expr_scan_holds(&source, SHORT,
                                    sizeof(SHORT) / sizeof(SHORT[0]),
                                    "a token of the second text",
                                    __LINE__)) );

    // with no class for a stray, a stray fails the source
    config.stray_class = 0u;
    ok = d_tests_expr_expect(
             (ok) &&
             (d_parse_expr_scan(&source, &scanner, &s, &config, "a # b", 5u)),
             "a scan of a text with a stray", __LINE__);
    ok = d_tests_expr_expect(
             (ok) &&
             (source.next(source.ctx, D_PARSE_EXPR_OPERAND, 1u, &k) == 1) &&
             (source.next(source.ctx, D_PARSE_EXPR_OPERATOR, 1u, &k) == -1),
             "a stray with no class fails the source", __LINE__);

    d_parse_expr_signature_release(&s);

    return ok;
}

/*
d_tests_expr_scan
  The default scanner (design section 10): its tokens, and parses through it
by dawk's table, whose spellings it reads.
  Tests the following:
  - names, whole; numbers with a point and an exponent, each only where a
    digit follows; strings with an escaped quote; a newline of its own class
    and the line break it leaves the next token; a stray, and a string
    never closed, as the stray class
  - the longest spelling first (`<=` before `<`), a word only as a whole
    name (`and`, not `android`), the caller's words beside the signature's
  - a stray with no class for it fails the source
  - through the parser: precedence, a word operator, the spacing a call's
    adjacency reads (`f(a)` and `f (a)`), and a custom role's word
*/
bool
d_tests_expr_scan(void)
{
    static const struct dj_parse_expr_scan_word MORE[] =
    {
        { "getline", C_GETLINE }, { "|", C_BAR }
    };
    static const struct dj_tests_expr_case CASES[] =
    {
        { "a + b * 2",       "add(a, mul(b, 2))"      },
        { "a b > c",         "gt(concat(a, b), c)"    },
        { "f(a, b)",         "call(f, a, b)"          },
        { "f (a)",           "concat(f, a)"           },
        { "i in arr",        "in(i, arr)"             },
        { "x = getline < f", "assign(x, getfile(f))"  },
        { "cmd | getline",   "pipeget(cmd)"           },
        { "-2^2",            "neg(pow(2, 2))"         }
    };
    struct dj_parse_expr_signature   s;
    struct dj_parse_expr_scan_config config;
    const bool                       tokens = d_tests_expr_scan_tokens();
    bool                             ok     = d_tests_expr_dawk(&s);

    memset(&config, 0, sizeof(config));
    config.name_class   = C_NAME;
    config.number_class = C_NUM;
    config.words        = MORE;
    config.word_count   = 2u;

    for (size_t i = 0u; i < sizeof(CASES) / sizeof(CASES[0]); ++i)
    {
        static struct dj_tests_expr_run   run;
        struct dj_parse_expr_scanner      scanner;
        struct dj_tests_expr_named_source named;
        struct dj_parse_expr_source       source;
        struct dj_tests_expr_text         b;
        const char*                       text = CASES[i].text;

        memset(&run, 0, sizeof(run));
        d_parse_diag_sink_init(&run.sink, run.items, 32u, run.text,
                               (uint32_t)sizeof(run.text));
        d_parse_expr_term_init_heap(&run.term);
        named.text      = text;
        source.next     = d_tests_expr_named_next;
        source.ctx      = &named;
        run.options.text        = text;
        run.options.text_length = strlen(text);

        const bool held =
            ( (d_parse_expr_scan(&named.inner, &scanner, &s, &config, text,
                                 strlen(text)))                             &&
              (d_parse_expr_parse(&s, &source, &run.options, &run.term,
                                  &run.sink, &run.result) ==
               D_PARSE_EXPR_OK)                                             &&
              (strcmp(d_tests_expr_sexpr(&run.term, run.result.root, &b),
                      CASES[i].term) == 0) );

        if (!held)
        {
            D_TEST_NOTE("'%s' scanned and read as %s", text,
                        d_tests_expr_sexpr(&run.term, run.result.root, &b));
        }

        ok = (held) && (ok);
        d_parse_expr_term_release(&run.term);
    }

    d_parse_expr_signature_release(&s);

    return ( (ok) && (tokens) );
}

#else

/*
d_tests_expr_scan
  The default scanner, compiled out (D_CFG_PARSE_EXPR_SCAN 0): skipped.
*/
bool
d_tests_expr_scan(void)
{
    D_TEST_SKIP("the scanner is compiled out (D_CFG_PARSE_EXPR_SCAN is 0)");
}

#endif  // D_INTERNAL_PARSE_EXPR_SCAN

/*
d_tests_expr_scratch
  The caller's scratch for the parser's stacks (design section 11).
  Tests the following:
  - a scratch of d_parse_expr_scratch_size(tokens) holds a parse that
    outgrows the parser's own rooms: a right chain of 400 operands, read as
    without it
  - a scratch smaller than the rooms is left alone, and the parse still
    reads; one large enough holds the stacks
  - the size of a count that cannot fit is 0, never a wrapped size
*/
bool
d_tests_expr_scratch(void)
{
    static char                     text[2048];
    static unsigned char            room[1u << 17];
    static struct dj_tests_expr_run run;
    struct dj_parse_expr_signature  s;
    struct dj_tests_expr_text       b;
    static char                     first[sizeof(b.out)];
    size_t                          at = 0u;
    bool                            ok = d_tests_expr_math(&s, true);

    // `2 ^ 2 ^ ... ^ 2`, right-associative, so every `^` stays open
    for (int i = 0; i < 400; ++i)
    {
        at += (size_t)snprintf(text + at, sizeof(text) - at, (i > 0) ? "^2"
                                                                     : "2");
    }

    const size_t need = d_parse_expr_scratch_size(800u);

    for (int pass = 0; pass < 3; ++pass)
    {
        memset(&run, 0, sizeof(run));
        memset(room, 0, sizeof(room));
        run.options.scratch      = (pass > 0) ? (void*)room : NULL;
        run.options.scratch_size = (pass == 1) ? need : 64u;

        const bool read =
            (d_tests_expr_parse(&run, &s, text, 0u) == D_PARSE_EXPR_OK);
        const char* got = d_tests_expr_sexpr(&run.term, run.result.root, &b);

        if (pass == 0)
        {
            (void)snprintf(first, sizeof(first), "%s", got);
        }

        // the stacks were in the scratch, or left it alone
        size_t used = 0u;

        for (size_t i = 0u; i < need; ++i)
        {
            used += (room[i] != 0u) ? 1u : 0u;
        }

        ok = d_tests_expr_expect(
                 (ok) && ( (pass == 1) == (used > 0u) ),
                 "the scratch is used where it is large enough", __LINE__);

        ok = d_tests_expr_expect(
                 (ok) && (read) && (strcmp(got, first) == 0),
                 (pass == 1) ? "a parse in the caller's scratch"
                             : "a parse beside a scratch too small",
                 __LINE__);
        d_parse_expr_term_release(&run.term);
    }

    ok = d_tests_expr_expect(
             (ok) && (need > 0u) && (need <= sizeof(room)) &&
             ( (sizeof(size_t) > 4u) ||
               (d_parse_expr_scratch_size(0xFFFFFFFFu) == 0u) ),
             "the scratch's size", __LINE__);
    d_parse_expr_signature_release(&s);

    return ok;
}
