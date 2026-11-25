/*******************************************************************************
* djinterp [c][parse]                                          parse_log_tests.c
*
* Parse's log suite: its fixtures, and its tests; where the unit is compiled
* out, the same tests, skipped.
*
*
* path:      /tests/djinterp/c/modules/parse/log/parse_log_tests.c
* link(s):   /docs/djinterp/log/log_design.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
#include "./parse_log_tests.h"  // corresponding header


#if ( (D_INTERNAL_PARSE_LOG == 1) && RE_STD_HAS_INT64 )


//==============================================================================
// FIXTURES
//==============================================================================

// D_TESTS_PARSE_LOG_LINES
//   constant: 1 where the lines the tests expect can be written: the log's
// codes carry their names and templates, and diagnostics their facts and
// text.
#if ( (D_INTERNAL_PARSE_DIAG_TEXT == 1)  &&                                   \
      (D_INTERNAL_PARSE_DIAG_FACTS == 1) &&                                   \
      D_CFG_PARSE_LOG_CODES )
    #define D_TESTS_PARSE_LOG_LINES  1
#else
    #define D_TESTS_PARSE_LOG_LINES  0
#endif

// D_TESTS_PARSE_LOG_NO_LINES
//   constant: what a test of the lines says it skipped for.
#define D_TESTS_PARSE_LOG_NO_LINES                                            \
    "the lines need text, facts and the log's codes' templates "              \
    "(D_CFG_PARSE_DIAG_TEXT, _FACTS or D_CFG_PARSE_LOG_CODES is 0)"

#if (D_TESTS_PARSE_LOG_LINES == 1)

// D_TESTS_PARSE_LOG_TIME
//   constant: the fixed clock's time: 2025-11-13T15:04:05.123456789Z.
#define D_TESTS_PARSE_LOG_TIME                                                \
    ((uint64_t)1763046245u * 1000000000u + 123456789u)

// D_TESTS_PARSE_LOG_AT
//   constant: how each line the text writer writes at that time begins.
#define D_TESTS_PARSE_LOG_AT  "2025-11-13T15:04:05.123Z "

/*
d_tests_parse_log_clock
  The fixed clock.
*/
static uint64_t
d_tests_parse_log_clock(
    void* _context
)
{
    (void)_context;

    return D_TESTS_PARSE_LOG_TIME;
}

// dj_tests_parse_log_out
//   struct: the log's destination in a test: a fixed stream over a buffer.
struct dj_tests_parse_log_out
{
    char                    text[4096];
    struct dj_ostream       os;
    struct dj_ostream_fixed fixed;
};

static struct dj_tests_parse_log_out d_tests_parse_log_out;

/*
d_tests_parse_log_reset
  The log at its defaults, writing text to an empty buffer at the fixed
time, with parse's tables registered.
*/
static bool
d_tests_parse_log_reset(void)
{
    if (d_log_djin() != NULL)
    {
        d_option_env_clear(d_log_djin());
    }

    memset(d_tests_parse_log_out.text, 0, sizeof(d_tests_parse_log_out.text));
    (void)d_ostream_init_fixed(&d_tests_parse_log_out.os,
                               &d_tests_parse_log_out.fixed,
                               d_tests_parse_log_out.text,
                               sizeof(d_tests_parse_log_out.text), 1);
    d_log_set_stream(&d_tests_parse_log_out.os);
    d_log_set_writer(NULL, NULL);
    d_log_set_clock(d_tests_parse_log_clock, NULL);

    return (d_parse_log_add() == 0);
}

#endif  // D_TESTS_PARSE_LOG_LINES

#if (D_TESTS_PARSE_LOG_LINES == 1)

/*
d_tests_parse_log_wrote
  Whether the log wrote the text expected; prints both when not.
*/
static bool
d_tests_parse_log_wrote(
    const char* _expected
)
{
    (void)d_ostream_flush(&d_tests_parse_log_out.os);

    if (strcmp(d_tests_parse_log_out.text, _expected) == 0)
    {
        return true;
    }

    printf("      wrote:    [%s]\n      expected: [%s]\n",
           d_tests_parse_log_out.text, _expected);

    return false;
}

#endif  // D_TESTS_PARSE_LOG_LINES

#if (D_TESTS_PARSE_LOG_LINES == 1)

// a row's template, by its code
#define D_TESTS_PARSE_LOG_PATTERN_ROW(_module, _number, _name, _class,        \
                                      _grade, _template, _facts)              \
    case (_number): return (_template);

/*
d_tests_parse_log_pattern
  The template parse.expr's table gives a code, as parse.c writes from it.
*/
static const char*
d_tests_parse_log_pattern(
    uint16_t _code
)
{
    switch (_code)
    {
        D_PARSE_EXPR_CODES(D_TESTS_PARSE_LOG_PATTERN_ROW, parse_expr)
        default:
            return NULL;
    }
}

/*
d_tests_parse_log_report
  Reports a diagnostic of parse.expr, its template and its facts, and as many
notes as asked, as parse.c does.
*/
static int
d_tests_parse_log_report(
    struct dj_parse_diag_sink*       _sink,
    uint16_t                         _code,
    uint32_t                         _offset,
    uint32_t                         _length,
    const struct dj_parse_diag_fact* _facts,
    uint16_t                         _count,
    uint32_t                         _notes
)
{
    static const char* const WORDS[3] =
    {
        "the innermost open operator begins here",
        "the first operator is here",
        "the expression ends here"
    };
    struct dj_parse_diag_report reports[4];

    memset(reports, 0, sizeof(reports));
    reports[0].severity   = (uint8_t)D_PARSE_SEVERITY_ERROR;
    reports[0].domain     = (uint16_t)D_PARSE_DIAG_DOMAIN_EXPR;
    reports[0].code       = _code;
    reports[0].fact_count = _count;
    reports[0].span       = d_parse_span_make(_offset, _length);
    reports[0].pattern    = d_tests_parse_log_pattern(_code);
    reports[0].facts      = _facts;

    for (uint32_t i = 1u; ( (i <= _notes) && (i < 4u) ); ++i)
    {
        reports[i].severity = (uint8_t)D_PARSE_SEVERITY_NOTE;
        reports[i].domain   = (uint16_t)D_PARSE_DIAG_DOMAIN_EXPR;
        reports[i].code     = _code;
        reports[i].span     = d_parse_span_make(i * 10u, 1u);
        reports[i].message  = WORDS[i - 1u];
    }

    return d_parse_diag_report(_sink, reports, 1u + _notes);
}

#endif  // D_TESTS_PARSE_LOG_LINES


//==============================================================================
// TESTS
//==============================================================================

/*
d_tests_parse_log_tables
  Parse's two tables, as the log checks and registers a module's.
  Tests the following:
  - each template compiles against its facts (d_log_module_check), as the
    log writes it
  - each module named as the log knows it, ten codes apiece
  - registered, twice without harm, so the log finds a code by its module
    and number
  - a diagnostic's code: parse.expr's for the expression domain,
    parse.core's for the core, machine and program domains, none for
    another domain or past a table
*/
bool
d_tests_parse_log_tables(void)
{
    const struct dj_log_code*        bad = NULL;
    struct dj_ostream_template_error error;

    memset(&error, 0, sizeof(error));

    D_TEST_REQUIRE(d_log_module_check(&d_log_module_parse_expr, &bad,
                                      &error) == D_OSTREAM_TEMPLATE_OK);
    D_TEST_REQUIRE(d_log_module_check(&d_log_module_parse_core, &bad,
                                      &error) == D_OSTREAM_TEMPLATE_OK);
    D_TEST_REQUIRE(strcmp(d_log_module_parse_expr.name, "parse.expr") == 0);
    D_TEST_REQUIRE(strcmp(d_log_module_parse_core.name, "parse.core") == 0);
    D_TEST_REQUIRE(d_log_module_parse_expr.count == 10u);
    D_TEST_REQUIRE(d_log_module_parse_core.count == 10u);

    D_TEST_REQUIRE(d_parse_log_add() == 0);
    D_TEST_REQUIRE(d_parse_log_add() == 0);
    D_TEST_REQUIRE(d_log_code_find("parse.expr", 10u, 4) ==
                   &d_log_code_parse_expr_mismatched);
    D_TEST_REQUIRE(d_log_code_find("parse.core", 10u, 9) ==
                   &d_log_code_parse_core_capacity);

    D_TEST_REQUIRE(d_parse_log_code(D_PARSE_DIAG_DOMAIN_EXPR,
                                    D_PARSE_EXPR_DIAG_MISMATCHED) ==
                   &d_log_code_parse_expr_mismatched);
    D_TEST_REQUIRE(d_parse_log_code(D_PARSE_DIAG_DOMAIN_MACHINE,
                                    D_PARSE_DIAG_STEP_LIMIT) ==
                   &d_log_code_parse_core_step_limit);
    D_TEST_REQUIRE(d_parse_log_code(D_PARSE_DIAG_DOMAIN_PROGRAM,
                                    D_PARSE_DIAG_BAD_FORMAT) ==
                   &d_log_code_parse_core_bad_format);
    D_TEST_REQUIRE(d_parse_log_code(D_PARSE_DIAG_DOMAIN_CORE,
                                    D_PARSE_DIAG_CAPACITY) ==
                   &d_log_code_parse_core_capacity);
    D_TEST_REQUIRE(d_parse_log_code(D_PARSE_DIAG_DOMAIN_LEX, 0u) == NULL);
    D_TEST_REQUIRE(d_parse_log_code(D_PARSE_DIAG_DOMAIN_EXPR, 10u) == NULL);

    return true;
}

/*
d_tests_parse_log_records
  A diagnostic as one record of its code.
  Tests the following:
  - attached, the subscriber writes one record per diagnostic, at its
    grade's level, its body its code's template written from its facts -
    the text the sink stored - then its span and its facts
  - a flag is written as true or false, an integer in decimal, and a fact
    with no value is left out
  - an outer close's three tokens, and an unbounded list's count
*/
bool
d_tests_parse_log_records(void)
{
#if (D_TESTS_PARSE_LOG_LINES == 1)
    struct dj_parse_diagnostic items[8];
    char                       text[512];
    struct dj_parse_diag_sink  sink;
    struct dj_parse_diag_fact  facts[4];

    D_TEST_REQUIRE(d_tests_parse_log_reset());

    d_parse_diag_sink_init(&sink, items, 8u, text, (uint32_t)sizeof(text));
    D_TEST_REQUIRE(d_parse_log_attach(&sink) == 1);
    D_TEST_REQUIRE(sink.stream == d_parse_log_event);
    D_TEST_REQUIRE(sink.stream_flags == D_PARSE_DIAG_STREAM_FACTS);

    facts[0] = d_parse_diag_fact_text("token", "*", 1u);
    facts[1] = d_parse_diag_fact_flag("after_operand", 0);
    D_TEST_REQUIRE(d_tests_parse_log_report(&sink,
                                            D_PARSE_EXPR_DIAG_UNEXPECTED,
                                            4u, 1u, facts, 2u, 0u) == 0);
    D_TEST_REQUIRE(strcmp(d_parse_diag_message(&sink, &items[0]),
                          "unexpected '*'") == 0);
    D_TEST_REQUIRE(d_tests_parse_log_wrote(
        D_TESTS_PARSE_LOG_AT "warning parse.expr.unexpected: unexpected '*' "
        "offset=4 length=1 token=* after_operand=false\n"));

    D_TEST_REQUIRE(d_tests_parse_log_reset());
    facts[0] = d_parse_diag_fact_text("token", ")", 1u);
    facts[1] = d_parse_diag_fact_text("open", "[", 1u);
    facts[2] = d_parse_diag_fact_text("outer", "(", 1u);
    D_TEST_REQUIRE(d_tests_parse_log_report(&sink,
                                            D_PARSE_EXPR_DIAG_MISMATCHED,
                                            7u, 1u, facts, 3u, 0u) == 0);
    D_TEST_REQUIRE(d_tests_parse_log_wrote(
        D_TESTS_PARSE_LOG_AT "warning parse.expr.mismatched: ')' closes '(' "
        "while '[' is open offset=7 length=1 token=) open=[ outer=(\n"));

    D_TEST_REQUIRE(d_tests_parse_log_reset());
    facts[0] = d_parse_diag_fact_unsigned("items", 0ul);
    facts[1] = d_parse_diag_fact_unsigned("min", 1ul);
    facts[2] = d_parse_diag_fact_flag("unbounded", 1);
    facts[3] = d_parse_diag_fact_none("max");
    D_TEST_REQUIRE(d_tests_parse_log_report(&sink, D_PARSE_EXPR_DIAG_ARITY,
                                            1u, 2u, facts, 4u, 0u) == 0);
    D_TEST_REQUIRE(d_tests_parse_log_wrote(
        D_TESTS_PARSE_LOG_AT "warning parse.expr.arity: 0 items, where at "
        "least 1 are needed offset=1 length=2 items=0 min=1 "
        "unbounded=true\n"));

    d_parse_diag_sink_release(&sink);

    return true;
#else
    D_TEST_SKIP(D_TESTS_PARSE_LOG_NO_LINES);
#endif  // D_TESTS_PARSE_LOG_LINES
}

/*
d_tests_parse_log_notes
  A diagnostic's notes in its one record.
  Tests the following:
  - one note is a level "note", with its words and its span
  - several are a level "notes", one level each, keyed "0", "1", in order
*/
bool
d_tests_parse_log_notes(void)
{
#if (D_TESTS_PARSE_LOG_LINES == 1)
    struct dj_parse_diag_sink       sink;
    const struct dj_parse_diag_fact open[1] =
    {
        d_parse_diag_fact_text("open", "(", 1u)
    };

    D_TEST_REQUIRE(d_tests_parse_log_reset());

    d_parse_diag_sink_init(&sink, NULL, 0u, NULL, 0u);
    D_TEST_REQUIRE(d_parse_log_attach(&sink) == 1);

    D_TEST_REQUIRE(d_tests_parse_log_report(&sink, D_PARSE_EXPR_DIAG_UNCLOSED,
                                            0u, 1u, open, 1u, 1u) == 0);
    D_TEST_REQUIRE(d_tests_parse_log_wrote(
        D_TESTS_PARSE_LOG_AT "warning parse.expr.unclosed: '(' is never "
        "closed offset=0 length=1 open=( note.text=\"the innermost open "
        "operator begins here\" note.offset=10 note.length=1\n"));

    D_TEST_REQUIRE(d_tests_parse_log_reset());
    D_TEST_REQUIRE(d_tests_parse_log_report(&sink, D_PARSE_EXPR_DIAG_UNCLOSED,
                                            0u, 1u, open, 1u, 2u) == 0);
    D_TEST_REQUIRE(d_tests_parse_log_wrote(
        D_TESTS_PARSE_LOG_AT "warning parse.expr.unclosed: '(' is never "
        "closed offset=0 length=1 open=( notes.0.text=\"the innermost open "
        "operator begins here\" notes.0.offset=10 notes.0.length=1 "
        "notes.1.text=\"the first operator is here\" notes.1.offset=20 "
        "notes.1.length=1\n"));

    return true;
#else
    D_TEST_SKIP(D_TESTS_PARSE_LOG_NO_LINES);
#endif  // D_TESTS_PARSE_LOG_LINES
}

/*
d_tests_parse_log_left_out
  What the subscriber writes beside a template, and what it does not write.
  Tests the following:
  - a core diagnostic in the emitter's own words: its code's template, and
    the words as the fact `message`; a line and a column where computed
  - not written: a note given alone, a diagnostic of a domain with no table,
    and a record the log does not want (log.parse_expr.level)
*/
bool
d_tests_parse_log_left_out(void)
{
#if (D_TESTS_PARSE_LOG_LINES == 1)
    struct dj_parse_diag_sink sink;

    D_TEST_REQUIRE(d_tests_parse_log_reset());

    d_parse_diag_sink_init(&sink, NULL, 0u, NULL, 0u);
    D_TEST_REQUIRE(d_parse_log_attach(&sink) == 1);

    D_TEST_REQUIRE(d_parse_diag_emit(&sink, D_PARSE_SEVERITY_FATAL,
                                     (uint16_t)D_PARSE_DIAG_DOMAIN_MACHINE,
                                     (uint16_t)D_PARSE_DIAG_STEP_LIMIT,
                                     d_parse_span_located(0u, 5u, 0u, 2u, 3u),
                                     "step limit exceeded") == 0);
    D_TEST_REQUIRE(d_tests_parse_log_wrote(
        D_TESTS_PARSE_LOG_AT "error parse.core.step_limit: the run passed "
        "its step limit offset=5 length=0 line=2 column=3 message=\"step "
        "limit exceeded\"\n"));

    // nothing of these
    D_TEST_REQUIRE(d_tests_parse_log_reset());
    D_TEST_REQUIRE(d_parse_diag_emit_flagged(
                       &sink, D_PARSE_SEVERITY_NOTE,
                       (uint16_t)D_PARSE_DIAG_DOMAIN_EXPR,
                       (uint16_t)D_PARSE_EXPR_DIAG_UNCLOSED,
                       D_PARSE_DIAG_FLAG_CONTINUATION,
                       d_parse_span_make(0u, 0u), "alone") == 0);
    D_TEST_REQUIRE(d_parse_diag_emit(&sink, D_PARSE_SEVERITY_ERROR,
                                     (uint16_t)D_PARSE_DIAG_DOMAIN_LEX, 0u,
                                     d_parse_span_make(0u, 1u),
                                     "a lexer's") == 0);
    D_TEST_REQUIRE( (d_log_djin() == NULL) ||
                    (d_option_env_put(d_log_djin(),
                                      "parse_expr.level=fatal").is_ok) );
    D_TEST_REQUIRE(d_parse_diag_emit(&sink, D_PARSE_SEVERITY_ERROR,
                                     (uint16_t)D_PARSE_DIAG_DOMAIN_EXPR,
                                     (uint16_t)D_PARSE_EXPR_DIAG_REFUSED,
                                     d_parse_span_make(0u, 1u), NULL) == 0);
    D_TEST_REQUIRE(d_tests_parse_log_wrote(""));

    (void)d_tests_parse_log_reset();

    return true;
#else
    D_TEST_SKIP(D_TESTS_PARSE_LOG_NO_LINES);
#endif  // D_TESTS_PARSE_LOG_LINES
}

#if (D_TESTS_PARSE_LOG_LINES == 1)

// dj_tests_parse_log_case
//   struct: one diagnostic of a row, with up to four facts.
struct dj_tests_parse_log_case
{
    uint16_t                  domain;
    uint16_t                  code;
    uint16_t                  count;
    struct dj_parse_diag_fact facts[4];
};

#endif  // D_TESTS_PARSE_LOG_LINES


#if (D_TESTS_PARSE_LOG_LINES == 1)

/*
d_tests_parse_log_body
  The body of the one line the log wrote: after its code's ": ", up to its
first fact, " offset=".
*/
static bool
d_tests_parse_log_body(
    char*  _out,
    size_t _size
)
{
    const char* const line = d_tests_parse_log_out.text;
    const char*       from = strstr(line, ": ");
    const char*       to   = (from) ? strstr(from, " offset=") : NULL;
    size_t            n;

    (void)d_ostream_flush(&d_tests_parse_log_out.os);

    if ( (!from) || (!to) )
    {
        return false;
    }

    from += 2;
    n     = (size_t)(to - from);

    if (n >= _size)
    {
        return false;
    }

    memcpy(_out, from, n);
    _out[n] = '\0';

    return true;
}

#endif  // D_TESTS_PARSE_LOG_LINES

/*
d_tests_parse_log_agree
  The text parse stores and the body the log writes, each from the same row's
template and facts, for every row and each wording of it.
  Tests the following:
  - each of parse.expr's ten rows, with its flags held and not, its tokens
    named and not ("a token"), the outer close, the unbounded list
  - parse.core's capacity, with what named and not
  - for each, the log's body is the sink's text, byte for byte: parse's
    writer and the log's agree on the template language they share
*/
bool
d_tests_parse_log_agree(void)
{
#if (D_TESTS_PARSE_LOG_LINES == 1)
    struct dj_tests_parse_log_case cases[24];
    size_t                         n = 0u;
    struct dj_parse_diagnostic     items[4];
    char                           text[1024];
    char                           body[256];
    struct dj_parse_diag_sink      sink;
    bool                           ok = true;

    memset(cases, 0, sizeof(cases));

#define D_TESTS_CASE(_domain, _code)                                          \
    ( cases[n].domain = (uint16_t)(_domain),                                  \
      cases[n].code   = (uint16_t)(_code),                                    \
      &cases[n++] )
#define D_TESTS_FACT(_case, _fact)                                            \
    ( (_case)->facts[(_case)->count++] = (_fact) )

    {
        struct dj_tests_parse_log_case* c;

        c = D_TESTS_CASE(D_PARSE_DIAG_DOMAIN_EXPR, 0);
        D_TESTS_FACT(c, d_parse_diag_fact_text("token", "x", 1u));
        D_TESTS_FACT(c, d_parse_diag_fact_flag("after_operand", 0));
        c = D_TESTS_CASE(D_PARSE_DIAG_DOMAIN_EXPR, 0);
        D_TESTS_FACT(c, d_parse_diag_fact_text("token", "not", 3u));
        D_TESTS_FACT(c, d_parse_diag_fact_flag("after_operand", 1));
        c = D_TESTS_CASE(D_PARSE_DIAG_DOMAIN_EXPR, 0);
        D_TESTS_FACT(c, d_parse_diag_fact_none("token"));
        D_TESTS_FACT(c, d_parse_diag_fact_flag("after_operand", 1));
        c = D_TESTS_CASE(D_PARSE_DIAG_DOMAIN_EXPR, 1);
        D_TESTS_FACT(c, d_parse_diag_fact_text("token", ",", 1u));
        D_TESTS_FACT(c, d_parse_diag_fact_flag("at_end", 0));
        c = D_TESTS_CASE(D_PARSE_DIAG_DOMAIN_EXPR, 1);
        D_TESTS_FACT(c, d_parse_diag_fact_none("token"));
        D_TESTS_FACT(c, d_parse_diag_fact_flag("at_end", 1));
        c = D_TESTS_CASE(D_PARSE_DIAG_DOMAIN_EXPR, 1);
        D_TESTS_FACT(c, d_parse_diag_fact_none("token"));
        D_TESTS_FACT(c, d_parse_diag_fact_flag("at_end", 0));
        c = D_TESTS_CASE(D_PARSE_DIAG_DOMAIN_EXPR, 2);
        D_TESTS_FACT(c, d_parse_diag_fact_text("open", "(", 1u));
        c = D_TESTS_CASE(D_PARSE_DIAG_DOMAIN_EXPR, 2);
        D_TESTS_FACT(c, d_parse_diag_fact_none("open"));
        c = D_TESTS_CASE(D_PARSE_DIAG_DOMAIN_EXPR, 3);
        D_TESTS_FACT(c, d_parse_diag_fact_text("token", ")", 1u));
        c = D_TESTS_CASE(D_PARSE_DIAG_DOMAIN_EXPR, 4);
        D_TESTS_FACT(c, d_parse_diag_fact_text("token", "]", 1u));
        D_TESTS_FACT(c, d_parse_diag_fact_text("open", "(", 1u));
        c = D_TESTS_CASE(D_PARSE_DIAG_DOMAIN_EXPR, 4);
        D_TESTS_FACT(c, d_parse_diag_fact_text("token", ")", 1u));
        D_TESTS_FACT(c, d_parse_diag_fact_text("open", "[", 1u));
        D_TESTS_FACT(c, d_parse_diag_fact_text("outer", "(", 1u));
        c = D_TESTS_CASE(D_PARSE_DIAG_DOMAIN_EXPR, 4);
        D_TESTS_FACT(c, d_parse_diag_fact_none("token"));
        D_TESTS_FACT(c, d_parse_diag_fact_none("open"));
        c = D_TESTS_CASE(D_PARSE_DIAG_DOMAIN_EXPR, 5);
        D_TESTS_FACT(c, d_parse_diag_fact_text("second", "==", 2u));
        D_TESTS_FACT(c, d_parse_diag_fact_text("first", "<", 1u));
        c = D_TESTS_CASE(D_PARSE_DIAG_DOMAIN_EXPR, 6);
        D_TESTS_FACT(c, d_parse_diag_fact_unsigned("items", 3ul));
        D_TESTS_FACT(c, d_parse_diag_fact_unsigned("min", 1ul));
        D_TESTS_FACT(c, d_parse_diag_fact_unsigned("max", 2ul));
        D_TESTS_FACT(c, d_parse_diag_fact_flag("unbounded", 0));
        c = D_TESTS_CASE(D_PARSE_DIAG_DOMAIN_EXPR, 6);
        D_TESTS_FACT(c, d_parse_diag_fact_unsigned("items", 0ul));
        D_TESTS_FACT(c, d_parse_diag_fact_unsigned("min", 1ul));
        D_TESTS_FACT(c, d_parse_diag_fact_flag("unbounded", 1));
        c = D_TESTS_CASE(D_PARSE_DIAG_DOMAIN_EXPR, 7);
        D_TESTS_FACT(c, d_parse_diag_fact_unsigned("limit", 1000ul));
        (void)D_TESTS_CASE(D_PARSE_DIAG_DOMAIN_EXPR, 8);
        c = D_TESTS_CASE(D_PARSE_DIAG_DOMAIN_EXPR, 9);
        D_TESTS_FACT(c, d_parse_diag_fact_text("reason",
                                               "a mixfix has no parts",
                                               21u));
        D_TESTS_FACT(c, d_parse_diag_fact_unsigned("id", 4294967295ul));
        c = D_TESTS_CASE(D_PARSE_DIAG_DOMAIN_CORE, D_PARSE_DIAG_CAPACITY);
        D_TESTS_FACT(c, d_parse_diag_fact_text("what", "expression", 10u));
        (void)D_TESTS_CASE(D_PARSE_DIAG_DOMAIN_CORE, D_PARSE_DIAG_CAPACITY);
        (void)D_TESTS_CASE(D_PARSE_DIAG_DOMAIN_CORE,
                           D_PARSE_DIAG_OUT_OF_MEMORY);
    }

#undef D_TESTS_FACT
#undef D_TESTS_CASE

    for (size_t i = 0u; i < n; ++i)
    {
        struct dj_parse_diag_report     report;
        const struct dj_log_code* const code =
            d_parse_log_code(cases[i].domain, cases[i].code);

        D_TEST_REQUIRE(d_tests_parse_log_reset());
        d_parse_diag_sink_init(&sink, items, 4u, text,
                               (uint32_t)sizeof(text));
        D_TEST_REQUIRE(d_parse_log_attach(&sink) == 1);
        D_TEST_REQUIRE( (code) && (code->text) );

        memset(&report, 0, sizeof(report));
        report.severity   = (uint8_t)D_PARSE_SEVERITY_ERROR;
        report.domain     = cases[i].domain;
        report.code       = cases[i].code;
        report.fact_count = cases[i].count;
        report.pattern    = code->text;
        report.facts      = cases[i].facts;

        D_TEST_REQUIRE(d_parse_diag_report(&sink, &report, 1u) == 0);

        if ( (!d_tests_parse_log_body(body, sizeof(body))) ||
             (strcmp(body, d_parse_diag_message(&sink, &items[0])) != 0) )
        {
            printf("      case %lu: the log wrote [%s], the sink holds "
                   "[%s]\n", (unsigned long)i, d_tests_parse_log_out.text,
                   d_parse_diag_message(&sink, &items[0]));
            ok = false;
        }
    }

    D_TEST_REQUIRE(ok);
    D_TEST_REQUIRE(n == 21u);

    return true;
#else
    D_TEST_SKIP(D_TESTS_PARSE_LOG_NO_LINES);
#endif  // D_TESTS_PARSE_LOG_LINES
}

// d_tests_parse_log_switches
//   variable: what the test's reader gives.
static struct dj_parse_diag_settings d_tests_parse_log_switches;

/*
d_tests_parse_log_read
  A reader of the run's switches, as the djin's is.
*/
static void
d_tests_parse_log_read(
    struct dj_parse_diag_settings* _settings
)
{
    *_settings = d_tests_parse_log_switches;

    return;
}

/*
d_tests_parse_log_other
  A streaming hook of a program's own, which does nothing.
*/
static void
d_tests_parse_log_other(
    void*                             _ctx,
    const struct dj_parse_diag_sink*  _sink,
    const struct dj_parse_diag_event* _event
)
{
    (void)_ctx;
    (void)_sink;
    (void)_event;

    return;
}

/*
d_tests_parse_log_switch
  The switch parse.log, and taking the subscriber off.
  Tests the following:
  - parse.log off: attaching attaches nothing, and the sink is unchanged
  - no sink: refused
  - detaching takes the subscriber off, and leaves another hook in place
*/
bool
d_tests_parse_log_switch(void)
{
    struct dj_parse_diag_sink sink;

    d_parse_diag_sink_init(&sink, NULL, 0u, NULL, 0u);

    d_tests_parse_log_switches.text  = 1u;
    d_tests_parse_log_switches.facts = 1u;
    d_tests_parse_log_switches.log   = 0u;
    d_parse_diag_settings_from(d_tests_parse_log_read);

    const int off = d_parse_log_attach(&sink);

    d_parse_diag_settings_from(NULL);

    D_TEST_REQUIRE(off == 0);
    D_TEST_REQUIRE(sink.stream == NULL);
    D_TEST_REQUIRE(d_parse_log_attach(NULL) == -1);

    D_TEST_REQUIRE(d_parse_log_attach(&sink) == 1);
    d_parse_log_detach(&sink);
    D_TEST_REQUIRE(sink.stream == NULL);

    // another hook stays where it is
    (void)d_parse_diag_sink_stream(&sink, d_tests_parse_log_other, NULL, 0u);
    d_parse_log_detach(&sink);
    d_parse_log_detach(NULL);
    D_TEST_REQUIRE(sink.stream == d_tests_parse_log_other);

    return true;
}


#else


// D_TESTS_PARSE_LOG_WITHOUT
//   constant: the reason each test gives where the unit is compiled out.
#define D_TESTS_PARSE_LOG_WITHOUT                                             \
    "parse's log unit is compiled out (D_CFG_PARSE_LOG 0, or no 64-bit "      \
    "integer)"

/*
d_tests_parse_log_*
  Each test, skipped, saying why.
*/
bool
d_tests_parse_log_tables(void)
{
    D_TEST_SKIP(D_TESTS_PARSE_LOG_WITHOUT);
}

bool
d_tests_parse_log_records(void)
{
    D_TEST_SKIP(D_TESTS_PARSE_LOG_WITHOUT);
}

bool
d_tests_parse_log_notes(void)
{
    D_TEST_SKIP(D_TESTS_PARSE_LOG_WITHOUT);
}

bool
d_tests_parse_log_left_out(void)
{
    D_TEST_SKIP(D_TESTS_PARSE_LOG_WITHOUT);
}

bool
d_tests_parse_log_agree(void)
{
    D_TEST_SKIP(D_TESTS_PARSE_LOG_WITHOUT);
}

bool
d_tests_parse_log_switch(void)
{
    D_TEST_SKIP(D_TESTS_PARSE_LOG_WITHOUT);
}


#endif  // D_INTERNAL_PARSE_LOG && RE_STD_HAS_INT64
