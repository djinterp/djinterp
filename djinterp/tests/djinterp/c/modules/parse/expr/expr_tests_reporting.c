/*******************************************************************************
* djinterp [c][parse]                                     expr_tests_reporting.c
*
* Expression core v2's C suite, block "reporting": the diagnostics parse
* reports. parse.core's code table; a template written from its facts, and
* checked; facts stored with their record and read back; the streaming hook,
* given each diagnostic with its notes; what a sink's diagnostics carry, and
* the run's switches; and each message the parser writes, held to its row's
* template written from the facts it stored.
*
*
* path:      /tests/djinterp/c/modules/parse/expr/expr_tests_reporting.c
* link(s):   /docs/djinterp/c/modules/parse/expr/expression_core_v2.html
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
#include "./expr_tests.h"  // corresponding header


//==============================================================================
// 1.  PARSE.CORE'S TABLE
//==============================================================================

/*
dj_tests_expr_core_row
  struct: one row of parse.core's code table, as text.
*/
struct dj_tests_expr_core_row
{
    const char* module;
    int         number;
    const char* name;
    const char* cls;
    const char* grade;
    const char* text;
    const char* facts;
};

#define D_TESTS_EXPR_CORE_ROW(_module, _number, _name, _class, _grade,        \
                              _template, _facts)                              \
    { #_module, (_number), #_name, #_class, #_grade, (_template), #_facts },

static const struct dj_tests_expr_core_row D_TESTS_EXPR_CORE_ROWS[] =
{
    D_PARSE_CORE_CODES(D_TESTS_EXPR_CORE_ROW, parse_core)
};

/*
d_tests_expr_core_table
  parse.core's code table, as the codes manifest and the log read it.
  Tests the following:
  - a row for each core code, numbered 0 to 9 in order, each parse_core's
    and each name its own
  - each row's class and grade as the log's vocabulary agreed them: one
    class per fault, failed where a budget or memory ran out, rejected
    otherwise
  - every template is one parse writes as the log does, naming only its
    row's facts
*/
bool
d_tests_expr_core_table(void)
{
    static const char* const CLASSES[] =
    {
        "FORMAT_TRUNCATED", "RESOURCE_BUDGET",  "LOOKUP_NOT_FOUND",
        "ARGUMENT_INVALID", "RESOURCE_MEMORY",  "TYPE_MISMATCH",
        "POSITION_RANGE",   "LOOKUP_NOT_FOUND", "FORMAT_UNSUPPORTED",
        "RESOURCE_BUDGET"
    };
    static const char* const GRADES[] =
    {
        "REJECTED", "FAILED",   "REJECTED", "REJECTED", "FAILED",
        "REJECTED", "REJECTED", "REJECTED", "REJECTED", "FAILED"
    };
    const size_t count   = sizeof(D_TESTS_EXPR_CORE_ROWS) /
                           sizeof(D_TESTS_EXPR_CORE_ROWS[0]);
    bool         ordered = (count == (size_t)D_PARSE_DIAG_CAPACITY + 1u);
    bool         named   = true;
    bool         classed = true;
    bool         sound   = true;

    for (size_t i = 0u; ( (ordered) && (i < count) ); ++i)
    {
        const struct dj_tests_expr_core_row* row = &D_TESTS_EXPR_CORE_ROWS[i];
        const int32_t                        at  =
            d_parse_diag_template_check(row->text, row->facts);

        ordered = (row->number == (int)i);
        named   = (named) && (strcmp(row->module, "parse_core") == 0);

        for (size_t k = 0u; k < i; ++k)
        {
            named = (named) &&
                    (strcmp(row->name, D_TESTS_EXPR_CORE_ROWS[k].name) != 0);
        }

        classed = (classed) &&
                  (strcmp(row->cls, CLASSES[i]) == 0) &&
                  (strcmp(row->grade, GRADES[i]) == 0);

        if (at >= 0)
        {
            D_TEST_NOTE("row %d's template fails at byte %ld", row->number,
                        (long)at);
            sound = false;
        }
    }

    named = (named) &&
            (strcmp(D_TESTS_EXPR_CORE_ROWS[D_PARSE_DIAG_CAPACITY].name,
                    "capacity") == 0) &&
            (strcmp(D_TESTS_EXPR_CORE_ROWS[D_PARSE_DIAG_TRUNCATED].name,
                    "truncated") == 0);

    // each check is reported, whatever the one before it found
    bool ok = d_tests_expr_expect(ordered,
                                  "a row for each core code, in order",
                                  __LINE__);

    ok = (d_tests_expr_expect(named,
                              "each row parse_core's, its name its own",
                              __LINE__)) && (ok);
    ok = (d_tests_expr_expect(classed,
                              "each row's class and grade as agreed",
                              __LINE__)) && (ok);
    ok = (d_tests_expr_expect(sound,
                              "each template sound, naming only its facts",
                              __LINE__)) && (ok);

    return ok;
}


//==============================================================================
// 2.  TEMPLATES
//==============================================================================

#if (D_INTERNAL_PARSE_DIAG_TEXT == 1)

/*
d_tests_expr_writes
  Whether a template, written from some facts into a buffer of a size, gives
a text and reports a length.
*/
static bool
d_tests_expr_writes(
    const char*                      _pattern,
    const struct dj_parse_diag_fact* _facts,
    uint32_t                         _count,
    size_t                           _size,
    const char*                      _expected,
    size_t                           _length
)
{
    char         out[128];
    const size_t length = d_parse_diag_write(_pattern, _facts, _count, out,
                                             _size);

    if ( (length != _length) ||
         (strcmp(out, _expected) != 0) )
    {
        D_TEST_NOTE("\"%s\" wrote \"%s\" (%lu), not \"%s\" (%lu)", _pattern,
                    out, (unsigned long)length, _expected,
                    (unsigned long)_length);

        return false;
    }

    return true;
}

/*
d_tests_expr_diag_write
  A template written from its facts, as the log writes one.
  Tests the following:
  - text is copied, and a field writes its fact's value: text as its bytes,
    an integer in decimal at every magnitude to 2^64 - 1, signed, a flag as
    true or false, and nothing for no value or no fact
  - a block is shown where its fact holds ({?}) or does not ({^}), {:}
    starting the other branch, nested, and closed by {/} or {/name}
  - {{ and }} are braces; an unclosed brace is text; a field's
    specification is not parse's, and the field is written as {name} is
  - the length is snprintf's: the whole text's, the buffer cut and ended
*/
bool
d_tests_expr_diag_write(void)
{
    struct dj_parse_diag_fact facts[8];
    struct dj_parse_diag_fact big;
    bool                      ok;

    facts[0] = d_parse_diag_fact_text("t", "hi", 2u);
    facts[1] = d_parse_diag_fact_unsigned("n", 0ul);
    facts[2] = d_parse_diag_fact_unsigned("u", 4294967295ul);
    facts[3] = d_parse_diag_fact_signed("s", -5L);
    facts[4] = d_parse_diag_fact_flag("yes", 1);
    facts[5] = d_parse_diag_fact_flag("no", 0);
    facts[6] = d_parse_diag_fact_none("none");
    facts[7] = d_parse_diag_fact_text("empty", "", 0u);

    ok = d_tests_expr_expect(
             (d_tests_expr_writes("a {t} b", facts, 8u, 128u, "a hi b", 6u)) &&
             (d_tests_expr_writes("{n} {u} {s}", facts, 8u, 128u,
                                  "0 4294967295 -5", 15u)) &&
             (d_tests_expr_writes("{yes}/{no}", facts, 8u, 128u,
                                  "true/false", 10u)) &&
             (d_tests_expr_writes("[{none}{empty}{missing}]", facts, 8u, 128u,
                                  "[]", 2u)),
             "fields write their values, and nothing for none", __LINE__);

    // past 32 bits, and the most
    big      = d_parse_diag_fact_none("b");
    big.kind = (uint8_t)D_PARSE_DIAG_FACT_INTEGER;
    big.high = 1u;
    ok = d_tests_expr_expect(
             (ok) &&
             (d_tests_expr_writes("{b}", &big, 1u, 128u, "4294967296", 10u)),
             "2^32 in decimal", __LINE__);
    big.high = 0xFFFFFFFFu;
    big.low  = 0xFFFFFFFFu;
    ok = d_tests_expr_expect(
             (ok) &&
             (d_tests_expr_writes("{b}", &big, 1u, 128u,
                                  "18446744073709551615", 20u)),
             "2^64 - 1 in decimal", __LINE__);
    big.negative = 1u;
    big.high     = 0x80000000u;
    big.low      = 0u;
    ok = d_tests_expr_expect(
             (ok) &&
             (d_tests_expr_writes("{b}", &big, 1u, 128u,
                                  "-9223372036854775808", 20u)),
             "-2^63 in decimal", __LINE__);

    ok = d_tests_expr_expect(
             (ok) &&
             (d_tests_expr_writes("{?t}y{:}n{/}{?empty}y{:}n{/}{?none}y{/}",
                                  facts, 8u, 128u, "yn", 2u)) &&
             (d_tests_expr_writes("{?u}y{/}{?n}y{:}n{/}{?yes}y{/}{?no}y{/}",
                                  facts, 8u, 128u, "yny", 3u)) &&
             (d_tests_expr_writes("{^no}not{/no}{^yes}x{:}so{/yes}",
                                  facts, 8u, 128u, "notso", 5u)) &&
             (d_tests_expr_writes("{?yes}A{?no}B{:}C{?t}{t}{/}{/}D{:}E{/}!",
                                  facts, 8u, 128u, "AChiD!", 6u)) &&
             (d_tests_expr_writes("{?no}A{?yes}B{/}C{:}D{/}", facts, 8u,
                                  128u, "D", 1u)),
             "blocks shown as their facts hold, nested", __LINE__);

    ok = d_tests_expr_expect(
             (ok) &&
             (d_tests_expr_writes("{{t}} {t", facts, 8u, 128u, "{t} {t",
                                  6u)) &&
             (d_tests_expr_writes("<{t:>5}>", facts, 8u, 128u, "<hi>", 4u)),
             "braces, an unclosed brace, a specification", __LINE__);

    ok = d_tests_expr_expect(
             (ok) &&
             (d_tests_expr_writes("abcdef", NULL, 0u, 4u, "abc", 6u)) &&
             (d_tests_expr_writes("a {t} b", facts, 8u, 4u, "a h", 6u)) &&
             (d_parse_diag_write("a {t} b", facts, 8u, NULL, 0u) == 6u) &&
             (d_parse_diag_write(NULL, facts, 8u, NULL, 0u) == 0u),
             "the whole length, the buffer cut and ended", __LINE__);

    return ok;
}

#else

/*
d_tests_expr_diag_write
  No text is carried: skipped.
*/
bool
d_tests_expr_diag_write(void)
{
    D_TEST_SKIP(D_TESTS_EXPR_NO_TEXT);
}

#endif  // D_INTERNAL_PARSE_DIAG_TEXT

/*
d_tests_expr_diag_check
  A template's soundness, as parse and the log both need it.
  Tests the following:
  - fields, tests, branches, ends by name and braces, each naming its facts,
    pass
  - each refusal at its construct's byte: a name not among the facts, a
    specification or a conversion, a block never closed, an end with no
    block or of another name, a second {:}, a lone }, a block nested past
    eight
*/
bool
d_tests_expr_diag_check(void)
{
    static const struct
    {
        const char* pattern;
        const char* facts;
        int32_t     at;
    }
    CASES[] =
    {
        { "plain text",                         "()",        -1 },
        { "{a} and {{b}}",                      "(a)",       -1 },
        { "{?a}x{:}y{/}{^b}z{/b}",              "(a, b)",    -1 },
        { "{?a}{?b}{a}{/b}{/a}",                "(a, b)",    -1 },
        { "{b}",                                "(a)",        0 },
        { "x {a:>3}",                           "(a)",        2 },
        { "{a!json}",                           "(a)",        0 },
        { "{?a}open",                           "(a)",        8 },
        { "{/}",                                "()",         0 },
        { "{?a}x{/b}",                          "(a, b)",     5 },
        { "{?a}{:}{:}{/}",                      "(a)",        7 },
        { "a}",                                 "()",         1 },
        { "{}",                                 "()",         0 },
        { "{?a}{?a}{?a}{?a}{?a}{?a}{?a}{?a}{?a}", "(a)",     32 }
    };
    bool ok = true;

    for (size_t i = 0u; i < sizeof(CASES) / sizeof(CASES[0]); ++i)
    {
        const int32_t at = d_parse_diag_template_check(CASES[i].pattern,
                                                       CASES[i].facts);

        if (at != CASES[i].at)
        {
            D_TEST_NOTE("\"%s\": %ld, not %ld", CASES[i].pattern, (long)at,
                        (long)CASES[i].at);
            ok = false;
        }
    }

    ok = d_tests_expr_expect(ok, "each template's soundness at its byte",
                             __LINE__);
    ok = d_tests_expr_expect(
             (ok) &&
             (d_parse_diag_template_check(NULL, "()") == 0) &&
             (d_parse_diag_template_check("{a}", NULL) == 0),
             "no template, and no facts list, refused", __LINE__);

    return ok;
}


//==============================================================================
// 3.  FACTS
//==============================================================================

#if (D_INTERNAL_PARSE_DIAG_FACTS == 1)

/*
d_tests_expr_report_one
  Reports one error with four facts and a template into a sink.
*/
static int
d_tests_expr_report_one(
    struct dj_parse_diag_sink* _sink
)
{
    struct dj_parse_diag_fact   facts[4];
    struct dj_parse_diag_report report;

    facts[0] = d_parse_diag_fact_text("token", "+=", 2u);
    facts[1] = d_parse_diag_fact_unsigned("items", 7ul);
    facts[2] = d_parse_diag_fact_flag("at_end", 1);
    facts[3] = d_parse_diag_fact_none("open");

    memset(&report, 0, sizeof(report));
    report.severity   = (uint8_t)D_PARSE_SEVERITY_ERROR;
    report.domain     = (uint16_t)D_PARSE_DIAG_DOMAIN_EXPR;
    report.code       = (uint16_t)D_PARSE_EXPR_DIAG_ARITY;
    report.fact_count = 4u;
    report.span       = d_parse_span_make(3u, 2u);
    report.pattern    = "{token} x{items}";
    report.facts      = facts;

    return d_parse_diag_report(_sink, &report, 1u);
}

/*
d_tests_expr_diag_facts
  Facts stored with their record, in the sink's facts arena, and read back.
  Tests the following:
  - a record's facts are stored, its count and offset in the record, and
    read back by place and by name: text with its NUL, an integer, a flag,
    no value; a name it lacks, and a place past its last, read nothing
  - a sink with no room for facts stores none, and is not truncated
  - a sink with too little room stores none of the record's, is truncated,
    and still stores and tallies the record and its text
  - a reset empties the facts arena
*/
bool
d_tests_expr_diag_facts(void)
{
    struct dj_parse_diagnostic items[4];
    char                       text[256];
    char                       room[256];
    struct dj_parse_diag_sink  sink;
    struct dj_parse_diag_fact  fact;
    bool                       ok;

    d_parse_diag_sink_init(&sink, items, 4u, text, (uint32_t)sizeof(text));
    d_parse_diag_sink_facts(&sink, room, (uint32_t)sizeof(room));
    (void)d_parse_diag_sink_carry(&sink, D_PARSE_DIAG_CARRY_TEXT |
                                         D_PARSE_DIAG_CARRY_FACTS);

    ok = d_tests_expr_expect(
             (d_tests_expr_report_one(&sink) == 0) &&
             (sink.count == 1u) &&
             (items[0].fact_count == 4u) &&
             (items[0].facts == 0u) &&
             (sink.facts_used > 4u * 16u) &&
             (!d_parse_diag_truncated(&sink)),
             "a record's four facts are stored with it", __LINE__);

    ok = d_tests_expr_expect(
             (ok) &&
             (d_parse_diag_fact_at(&sink, &items[0], 0u, &fact) == 1) &&
             (strcmp(fact.name, "token") == 0) &&
             (fact.kind == (uint8_t)D_PARSE_DIAG_FACT_TEXT) &&
             (fact.size == 2u) &&
             (strcmp(fact.text, "+=") == 0) &&
             (d_parse_diag_fact_at(&sink, &items[0], 1u, &fact) == 1) &&
             (fact.kind == (uint8_t)D_PARSE_DIAG_FACT_INTEGER) &&
             (fact.low == 7u) && (fact.high == 0u) &&
             (d_parse_diag_fact_at(&sink, &items[0], 2u, &fact) == 1) &&
             (fact.kind == (uint8_t)D_PARSE_DIAG_FACT_FLAG) &&
             (fact.low == 1u) &&
             (d_parse_diag_fact_at(&sink, &items[0], 3u, &fact) == 1) &&
             (strcmp(fact.name, "open") == 0) &&
             (fact.kind == (uint8_t)D_PARSE_DIAG_FACT_NONE),
             "each fact read back by its place", __LINE__);

    ok = d_tests_expr_expect(
             (ok) &&
             (d_parse_diag_fact_find(&sink, &items[0], "at_end", &fact) ==
              2) &&
             (fact.low == 1u) &&
             (d_parse_diag_fact_find(&sink, &items[0], "first", NULL) ==
              -1) &&
             (d_parse_diag_fact_at(&sink, &items[0], 4u, NULL) == 0) &&
             (d_parse_diag_fact_at(NULL, &items[0], 0u, NULL) == 0) &&
             (d_parse_diag_fact_find(&sink, &items[0], NULL, NULL) == -1),
             "by name, and nothing past them", __LINE__);

#if (D_INTERNAL_PARSE_DIAG_TEXT == 1)
    ok = d_tests_expr_expect(
             (ok) &&
             (strcmp(d_parse_diag_message(&sink, &items[0]), "+= x7") == 0),
             "the text written from the template and the facts", __LINE__);
#endif  // D_INTERNAL_PARSE_DIAG_TEXT

    // no room: none stored, and nothing lost that was asked to be kept
    d_parse_diag_sink_facts(&sink, NULL, 0u);
    d_parse_diag_sink_reset(&sink);
    ok = d_tests_expr_expect(
             (ok) &&
             (d_tests_expr_report_one(&sink) == 0) &&
             (items[0].fact_count == 0u) &&
             (items[0].facts == D_PARSE_DIAG_NO_FACTS) &&
             (!d_parse_diag_truncated(&sink)),
             "a sink with no room keeps no facts, and is not truncated",
             __LINE__);

    // too little room: none of the record's, the record and text kept
    d_parse_diag_sink_facts(&sink, room, 40u);
    d_parse_diag_sink_reset(&sink);
    ok = d_tests_expr_expect(
             (ok) &&
             (d_tests_expr_report_one(&sink) == 0) &&
             (sink.count == 1u) &&
             (items[0].fact_count == 0u) &&
             (items[0].facts == D_PARSE_DIAG_NO_FACTS) &&
             (sink.facts_used == 0u) &&
             (d_parse_diag_truncated(&sink)) &&
             (d_parse_diag_tally(&sink, D_PARSE_SEVERITY_ERROR) == 1u) &&
             ( (D_INTERNAL_PARSE_DIAG_TEXT == 0) ||
               (items[0].message != D_PARSE_DIAG_NO_MESSAGE) ),
             "too little room: no facts, the record and its text kept",
             __LINE__);

    // a reset empties the facts arena too
    d_parse_diag_sink_facts(&sink, room, (uint32_t)sizeof(room));
    (void)d_tests_expr_report_one(&sink);
    d_parse_diag_sink_reset(&sink);
    ok = d_tests_expr_expect(
             (ok) && (sink.facts_used == 0u) && (sink.count == 0u),
             "a reset empties the facts arena", __LINE__);

    d_parse_diag_sink_release(&sink);

    return ok;
}

#else

/*
d_tests_expr_diag_facts
  No facts are carried: skipped.
*/
bool
d_tests_expr_diag_facts(void)
{
    D_TEST_SKIP(D_TESTS_EXPR_NO_FACTS);
}

#endif  // D_INTERNAL_PARSE_DIAG_FACTS


//==============================================================================
// 4.  HOOKS
//==============================================================================

// dj_tests_expr_seen
//   struct: what the hooks saw: the per-record calls, and the streaming
// hook's last event, copied.
struct dj_tests_expr_seen
{
    uint32_t records;
    uint32_t events;
    uint32_t records_at_event;
    uint16_t code;
    bool     text_given;
    char     text[128];
    uint32_t fact_count;
    char     first_fact[16];
    uint32_t note_count;
    bool     note_continues;
    char     note[64];
};

/*
d_tests_expr_seen_record
  The per-record hook: counts.
*/
static void
d_tests_expr_seen_record(
    void*                             _ctx,
    const struct dj_parse_diag_sink*  _sink,
    const struct dj_parse_diagnostic* _diag
)
{
    struct dj_tests_expr_seen* const seen = (struct dj_tests_expr_seen*)_ctx;

    (void)_sink;
    (void)_diag;
    ++seen->records;

    return;
}

/*
d_tests_expr_seen_event
  The streaming hook: copies what it was given.
*/
static void
d_tests_expr_seen_event(
    void*                             _ctx,
    const struct dj_parse_diag_sink*  _sink,
    const struct dj_parse_diag_event* _event
)
{
    struct dj_tests_expr_seen* const seen = (struct dj_tests_expr_seen*)_ctx;

    (void)_sink;
    ++seen->events;
    seen->records_at_event = seen->records;
    seen->code             = _event->record->code;
    seen->text_given       = (_event->text != NULL);
    seen->text[0]          = '\0';
    seen->fact_count       = _event->fact_count;
    seen->first_fact[0]    = '\0';
    seen->note_count       = _event->note_count;
    seen->note_continues   = false;
    seen->note[0]          = '\0';

    if (_event->text)
    {
        (void)snprintf(seen->text, sizeof(seen->text), "%.*s",
                       (int)_event->text_size, _event->text);
    }

    if ( (_event->facts) &&
         (_event->fact_count > 0u) )
    {
        (void)snprintf(seen->first_fact, sizeof(seen->first_fact), "%s",
                       _event->facts[0].name);
    }

    if (_event->note_count > 0u)
    {
        seen->note_continues = ((_event->notes[0].record->flags &
                                 D_PARSE_DIAG_FLAG_CONTINUATION) != 0u);

        if (_event->notes[0].text)
        {
            (void)snprintf(seen->note, sizeof(seen->note), "%.*s",
                           (int)_event->notes[0].text_size,
                           _event->notes[0].text);
        }
    }

    return;
}

/*
d_tests_expr_report_noted
  Reports an error with a template, a fact and a note of its own words, and
then as many more notes as asked.
*/
static int
d_tests_expr_report_noted(
    struct dj_parse_diag_sink* _sink,
    uint32_t                   _notes
)
{
    struct dj_parse_diag_report     reports[12];
    const struct dj_parse_diag_fact facts[1] =
    {
        d_parse_diag_fact_text("open", "(", 1u)
    };

    memset(reports, 0, sizeof(reports));
    reports[0].severity   = (uint8_t)D_PARSE_SEVERITY_ERROR;
    reports[0].domain     = (uint16_t)D_PARSE_DIAG_DOMAIN_EXPR;
    reports[0].code       = (uint16_t)D_PARSE_EXPR_DIAG_UNCLOSED;
    reports[0].fact_count = 1u;
    reports[0].span       = d_parse_span_make(0u, 1u);
    reports[0].pattern    = "'{open}' is never closed";
    reports[0].facts      = facts;

    for (uint32_t i = 1u; ( (i <= _notes) && (i < 12u) ); ++i)
    {
        reports[i].severity = (uint8_t)D_PARSE_SEVERITY_NOTE;
        reports[i].domain   = (uint16_t)D_PARSE_DIAG_DOMAIN_EXPR;
        reports[i].code     = (uint16_t)D_PARSE_EXPR_DIAG_UNCLOSED;
        reports[i].span     = d_parse_span_make(6u, 0u);
        reports[i].message  = "the expression ends here";
    }

    return d_parse_diag_report(_sink, reports, 1u + _notes);
}

/*
d_tests_expr_diag_stream
  The streaming hook, given each diagnostic once, with its notes.
  Tests the following:
  - it is called once for a diagnostic and its note, after the per-record
    hook was called for both, and given the note flagged a continuation,
    with its own words
  - asked for facts alone, it is given the facts and no text; asked for
    text too, the text the sink stored
  - on a sink that stores no text, the text is written for it
  - it is given only what the sink's diagnostics carry, and its
    installation says what that is
  - a diagnostic below the filter is not streamed, nor its notes; notes
    past D_PARSE_DIAG_NOTES_MAX are stored but not streamed
*/
bool
d_tests_expr_diag_stream(void)
{
    struct dj_parse_diagnostic items[16];
    char                       text[512];
    struct dj_parse_diag_sink  sink;
    struct dj_tests_expr_seen  seen;
    const uint8_t              built =
        d_parse_diag_sink_carry(NULL, D_PARSE_DIAG_CARRY_TEXT |
                                      D_PARSE_DIAG_CARRY_FACTS);
    const bool                 has_text  =
        ((built & D_PARSE_DIAG_CARRY_TEXT) != 0u);
    const bool                 has_facts =
        ((built & D_PARSE_DIAG_CARRY_FACTS) != 0u);
    uint8_t                    given;
    bool                       ok;

    memset(&seen, 0, sizeof(seen));
    d_parse_diag_sink_init(&sink, items, 16u, text, (uint32_t)sizeof(text));
    (void)d_parse_diag_sink_carry(&sink, built);
    d_parse_diag_sink_hook(&sink, d_tests_expr_seen_record, &seen);
    given = d_parse_diag_sink_stream(&sink, d_tests_expr_seen_event, &seen,
                                     D_PARSE_DIAG_STREAM_FACTS);

    ok = d_tests_expr_expect(
             (given == (built & D_PARSE_DIAG_STREAM_FACTS)) &&
             (d_tests_expr_report_noted(&sink, 1u) == 0) &&
             (seen.events == 1u) &&
             (seen.records == 2u) &&
             (seen.records_at_event == 2u) &&
             (seen.code == (uint16_t)D_PARSE_EXPR_DIAG_UNCLOSED) &&
             (seen.note_count == 1u) &&
             (seen.note_continues) &&
             ( (!has_text) ||
               (strcmp(seen.note, "the expression ends here") == 0) ),
             "once for a diagnostic and its note, after both records",
             __LINE__);

    ok = d_tests_expr_expect(
             (ok) &&
             (!seen.text_given) &&
             (seen.fact_count == ((has_facts) ? 1u : 0u)) &&
             ( (!has_facts) || (strcmp(seen.first_fact, "open") == 0) ),
             "asked for facts alone: the facts, and no text", __LINE__);

    // text too: the stored text
    given = d_parse_diag_sink_stream(&sink, d_tests_expr_seen_event, &seen,
                                     D_PARSE_DIAG_STREAM_TEXT |
                                     D_PARSE_DIAG_STREAM_FACTS);
    (void)d_tests_expr_report_noted(&sink, 0u);
    ok = d_tests_expr_expect(
             (ok) &&
             (given == built) &&
             (seen.events == 2u) &&
             (seen.note_count == 0u) &&
             (seen.text_given == has_text) &&
             ( (!has_text) ||
               (strcmp(seen.text, "'(' is never closed") == 0) ),
             "asked for text too: the text the sink stored", __LINE__);

    // a sink that stores nothing: the text written for the hook
    d_parse_diag_sink_init(&sink, NULL, 0u, NULL, 0u);
    (void)d_parse_diag_sink_carry(&sink, built);
    (void)d_parse_diag_sink_stream(&sink, d_tests_expr_seen_event, &seen,
                                   D_PARSE_DIAG_STREAM_TEXT);
    (void)d_tests_expr_report_noted(&sink, 1u);
    ok = d_tests_expr_expect(
             (ok) &&
             (seen.events == 3u) &&
             (seen.text_given == has_text) &&
             ( (!has_text) ||
               (strcmp(seen.text, "'(' is never closed") == 0) ) &&
             (seen.fact_count == 0u),
             "on a sink that stores no text, the text written for it",
             __LINE__);

    // carrying nothing, nothing is given, and the installation says so
    (void)d_parse_diag_sink_carry(&sink, 0u);
    given = d_parse_diag_sink_stream(&sink, d_tests_expr_seen_event, &seen,
                                     D_PARSE_DIAG_STREAM_TEXT |
                                     D_PARSE_DIAG_STREAM_FACTS);
    (void)d_tests_expr_report_noted(&sink, 1u);
    ok = d_tests_expr_expect(
             (ok) &&
             (given == 0u) &&
             (seen.events == 4u) &&
             (!seen.text_given) &&
             (seen.fact_count == 0u) &&
             (seen.note[0] == '\0'),
             "carrying nothing, given nothing, and told so", __LINE__);

    // the filter, and notes past the most streamed
    d_parse_diag_sink_init(&sink, items, 16u, NULL, 0u);
    (void)d_parse_diag_sink_stream(&sink, d_tests_expr_seen_event, &seen,
                                   0u);
    ok = d_tests_expr_expect(
             (ok) &&
             (d_tests_expr_report_noted(&sink, 10u) == 0) &&
             (seen.events == 5u) &&
             (seen.note_count == D_PARSE_DIAG_NOTES_MAX) &&
             (sink.count == 11u) &&
             (d_parse_diag_tally(&sink, D_PARSE_SEVERITY_NOTE) == 10u),
             "notes past the most are stored, not streamed", __LINE__);

    d_parse_diag_sink_init(&sink, items, 16u, NULL, 0u);
    (void)d_parse_diag_sink_stream(&sink, d_tests_expr_seen_event, &seen,
                                   0u);
    {
        struct dj_parse_diag_report warning;

        memset(&warning, 0, sizeof(warning));
        warning.severity = (uint8_t)D_PARSE_SEVERITY_NOTE;
        d_parse_diag_sink_filter(&sink, D_PARSE_SEVERITY_WARNING);
        ok = d_tests_expr_expect(
                 (ok) &&
                 (d_parse_diag_report(&sink, &warning, 1u) == -1) &&
                 (d_parse_diag_report(&sink, NULL, 1u) == -1) &&
                 (d_parse_diag_report(NULL, &warning, 1u) == -1) &&
                 (seen.events == 5u) &&
                 (sink.count == 0u),
                 "below the filter: not streamed, not stored", __LINE__);
    }

    return ok;
}


//==============================================================================
// 5.  WHAT IS CARRIED
//==============================================================================

// d_tests_expr_switches
//   variable: what the test's reader gives.
static struct dj_parse_diag_settings d_tests_expr_switches;

/*
d_tests_expr_read_switches
  A reader of the run's switches, as the djin's is.
*/
static void
d_tests_expr_read_switches(
    struct dj_parse_diag_settings* _settings
)
{
    *_settings = d_tests_expr_switches;

    return;
}

/*
d_tests_expr_diag_carry
  What a sink's diagnostics carry, and the run's switches.
  Tests the following:
  - a sink carries what the build has, and what is asked of it that the
    build lacks is not given
  - carrying nothing, a templated report stores neither text nor facts; a
    sink with no room for facts given a carry of facts stores none
  - the run's switches are the knobs' until a reader is installed, then the
    reader's of what the build has, and a new sink carries what they say
*/
bool
d_tests_expr_diag_carry(void)
{
    struct dj_parse_diagnostic    items[4];
    char                          text[256];
    char                          room[256];
    struct dj_parse_diag_sink     sink;
    struct dj_parse_diag_settings settings;
    const uint8_t                 built =
        (uint8_t)( ((D_INTERNAL_PARSE_DIAG_TEXT == 1)
                    ? D_PARSE_DIAG_CARRY_TEXT  : 0u) |
                   ((D_INTERNAL_PARSE_DIAG_FACTS == 1)
                    ? D_PARSE_DIAG_CARRY_FACTS : 0u) );
    struct dj_parse_diag_report   report;
    const struct dj_parse_diag_fact facts[1] =
    {
        d_parse_diag_fact_unsigned("n", 3ul)
    };
    bool                          ok;

    memset(&report, 0, sizeof(report));
    report.severity   = (uint8_t)D_PARSE_SEVERITY_ERROR;
    report.fact_count = 1u;
    report.pattern    = "n is {n}";
    report.facts      = facts;

    d_parse_diag_sink_init(&sink, items, 4u, text, (uint32_t)sizeof(text));
    d_parse_diag_sink_facts(&sink, room, (uint32_t)sizeof(room));

    ok = d_tests_expr_expect(
             (sink.carry == built) &&
             (d_parse_diag_sink_carry(&sink, 0xFFu) == built) &&
             (d_parse_diag_sink_carry(NULL, D_PARSE_DIAG_CARRY_TEXT) ==
              (built & D_PARSE_DIAG_CARRY_TEXT)),
             "a sink carries what the build has, and no more", __LINE__);

    (void)d_parse_diag_sink_carry(&sink, 0u);
    ok = d_tests_expr_expect(
             (ok) &&
             (d_parse_diag_report(&sink, &report, 1u) == 0) &&
             (items[0].message == D_PARSE_DIAG_NO_MESSAGE) &&
             (items[0].facts == D_PARSE_DIAG_NO_FACTS) &&
             (sink.text_used == 0u) &&
             (sink.facts_used == 0u),
             "carrying nothing, nothing stored", __LINE__);

    (void)d_parse_diag_sink_carry(&sink, built);
    ok = d_tests_expr_expect(
             (ok) &&
             (d_parse_diag_report(&sink, &report, 1u) == 0) &&
             ( (D_INTERNAL_PARSE_DIAG_TEXT == 0) ||
               (strcmp(d_parse_diag_message(&sink, &items[1]), "n is 3") ==
                0) ) &&
             (items[1].fact_count ==
              ((D_INTERNAL_PARSE_DIAG_FACTS == 1) ? 1u : 0u)),
             "carrying what the build has, it is stored", __LINE__);

    // the switches: the knobs', then the reader's, of what the build has
    d_parse_diag_settings_from(NULL);
    d_parse_diag_settings(&settings);
    ok = d_tests_expr_expect(
             (ok) &&
             (settings.text == (uint8_t)D_INTERNAL_PARSE_DIAG_TEXT) &&
             (settings.facts == (uint8_t)D_INTERNAL_PARSE_DIAG_FACTS) &&
             (settings.log == (uint8_t)D_INTERNAL_PARSE_LOG),
             "without a reader, the knobs", __LINE__);

    d_tests_expr_switches.text  = 0u;
    d_tests_expr_switches.facts = 1u;
    d_tests_expr_switches.log   = 1u;
    d_parse_diag_settings_from(d_tests_expr_read_switches);
    d_parse_diag_settings(&settings);
    d_parse_diag_sink_init(&sink, items, 4u, text, (uint32_t)sizeof(text));
    ok = d_tests_expr_expect(
             (ok) &&
             (settings.text == 0u) &&
             (settings.facts == (uint8_t)D_INTERNAL_PARSE_DIAG_FACTS) &&
             (settings.log == (uint8_t)D_INTERNAL_PARSE_LOG) &&
             (sink.carry == (built & D_PARSE_DIAG_CARRY_FACTS)),
             "with a reader, its switches, of what the build has", __LINE__);

    d_parse_diag_settings_from(NULL);
    d_parse_diag_sink_init(&sink, items, 4u, text, (uint32_t)sizeof(text));
    ok = d_tests_expr_expect((ok) && (sink.carry == built),
                             "the reader taken off, the knobs again",
                             __LINE__);

    return ok;
}


//==============================================================================
// 6.  ONE SOURCE
//==============================================================================

#if ( (D_INTERNAL_PARSE_EXPR_HEAP == 1)  &&                                   \
      (D_INTERNAL_PARSE_DIAG_TEXT == 1)  &&                                   \
      (D_INTERNAL_PARSE_DIAG_FACTS == 1) )

// a row's template, by its code
#define D_TESTS_EXPR_PATTERN_ROW(_module, _number, _name, _class, _grade,     \
                                 _template, _facts)                           \
    case (_number): return (_template);

/*
d_tests_expr_pattern_of
  The template parse.expr's table gives a code.
*/
static const char*
d_tests_expr_pattern_of(
    uint16_t _code
)
{
    switch (_code)
    {
        D_PARSE_EXPR_CODES(D_TESTS_EXPR_PATTERN_ROW, parse_expr)
        default:
            return NULL;
    }
}

/*
d_tests_expr_from_facts
  Whether every error a parse stored is its row's template written from the
facts it stored with it.
*/
static bool
d_tests_expr_from_facts(
    const struct dj_tests_expr_run* _run,
    const char*                     _text
)
{
    bool ok = (_run->sink.count > 0u);

    for (uint32_t i = 0u; ( (ok) && (i < _run->sink.count) ); ++i)
    {
        const struct dj_parse_diagnostic* d = &_run->items[i];
        struct dj_parse_diag_fact         facts[8];
        char                              out[256];
        uint32_t                          n = 0u;

        // notes have words of their own
        if ((d->flags & D_PARSE_DIAG_FLAG_CONTINUATION) != 0u)
        {
            continue;
        }

        while ( (n < 8u) &&
                (d_parse_diag_fact_at(&_run->sink, d, n, &facts[n]) == 1) )
        {
            ++n;
        }

        (void)d_parse_diag_write(d_tests_expr_pattern_of(d->code), facts, n,
                                 out, sizeof(out));

        ok = ( (d->domain == (uint16_t)D_PARSE_DIAG_DOMAIN_EXPR) &&
               (n == (uint32_t)d->fact_count)                     &&
               (strcmp(out, d_parse_diag_message(&_run->sink, d)) == 0) );

        if (!ok)
        {
            D_TEST_NOTE("'%s': \"%s\" from its facts, \"%s\" stored", _text,
                        out, d_parse_diag_message(&_run->sink, d));
        }
    }

    return ok;
}

/*
d_tests_expr_one_source
  Each message the parser writes is its row's template written from the
facts it stored: the text and the log come from one definition.
  Tests the following:
  - every error of math's and the command language's tables, each of rows
    0 to 6, the outer close and the unbounded list among them
  - the facts named as the row lists them: the token's own text, unquoted;
    the flags given false where they do not hold
*/
bool
d_tests_expr_one_source(void)
{
    static const char* const MATH[] =
    {
        "1 + * 2", "2 3", "x not y", "1 +", "[1, ]", "x[]", "()",
        "(1 + 2", "f(a, |b", "1 + 2)", "(1 + 2]", "f([1, 2)", "A[1,2,3]",
        "1 + * 2)", "f((a, b))"
    };
    static const char* const CLI[] =
    {
        "a == b == c", "a < b == c"
    };
    struct dj_parse_expr_signature  math;
    struct dj_parse_expr_signature  cli;
    static struct dj_tests_expr_run run;
    struct dj_parse_diag_fact       fact;
    bool                            ok = ( (d_tests_expr_math(&math, true)) &&
                                           (d_tests_expr_cli(&cli)) );

    for (size_t i = 0u; ( (ok) && (i < sizeof(MATH) / sizeof(MATH[0])) );
         ++i)
    {
        memset(&run, 0, sizeof(run));
        (void)d_tests_expr_parse(&run, &math, MATH[i], 0u);
        ok = d_tests_expr_from_facts(&run, MATH[i]);
        d_parse_expr_term_release(&run.term);
    }

    for (size_t i = 0u; ( (ok) && (i < sizeof(CLI) / sizeof(CLI[0])) ); ++i)
    {
        memset(&run, 0, sizeof(run));
        (void)d_tests_expr_parse(&run, &cli, CLI[i], 0u);
        ok = d_tests_expr_from_facts(&run, CLI[i]);
        d_parse_expr_term_release(&run.term);
    }

    ok = d_tests_expr_expect(ok, "every message, its template from its facts",
                             __LINE__);

    // the facts as the row lists them
    memset(&run, 0, sizeof(run));
    (void)d_tests_expr_parse(&run, &math, "f([1, 2)", 0u);
    ok = d_tests_expr_expect(
             (ok) &&
             (d_parse_diag_fact_find(&run.sink, &run.items[0], "token",
                                     &fact) == 0) &&
             (fact.size == 1u) && (fact.text[0] == ')') &&
             (d_parse_diag_fact_find(&run.sink, &run.items[0], "open",
                                     &fact) == 1) &&
             (fact.text[0] == '[') &&
             (d_parse_diag_fact_find(&run.sink, &run.items[0], "outer",
                                     &fact) == 2) &&
             (fact.text[0] == '('),
             "an outer close's token, open and outer, unquoted", __LINE__);
    d_parse_expr_term_release(&run.term);

    memset(&run, 0, sizeof(run));
    (void)d_tests_expr_parse(&run, &math, "1 +", 0u);
    ok = d_tests_expr_expect(
             (ok) &&
             (d_parse_diag_fact_find(&run.sink, &run.items[0], "at_end",
                                     &fact) == 1) &&
             (fact.kind == (uint8_t)D_PARSE_DIAG_FACT_FLAG) &&
             (fact.low == 1u) &&
             (d_parse_diag_fact_find(&run.sink, &run.items[0], "token",
                                     &fact) == 0) &&
             (fact.kind == (uint8_t)D_PARSE_DIAG_FACT_NONE),
             "the end: at_end holds, and the token has no text", __LINE__);
    d_parse_expr_term_release(&run.term);

    memset(&run, 0, sizeof(run));
    (void)d_tests_expr_parse(&run, &math, "1 + * 2", 0u);
    ok = d_tests_expr_expect(
             (ok) &&
             (d_parse_diag_fact_find(&run.sink, &run.items[0],
                                     "after_operand", &fact) == 1) &&
             (fact.kind == (uint8_t)D_PARSE_DIAG_FACT_FLAG) &&
             (fact.low == 0u),
             "a flag that does not hold is given false", __LINE__);
    d_parse_expr_term_release(&run.term);

    d_parse_expr_signature_release(&math);
    d_parse_expr_signature_release(&cli);

    return ok;
}

#else

/*
d_tests_expr_one_source
  The heap, the text or the facts compiled out: skipped.
*/
bool
d_tests_expr_one_source(void)
{
    D_TEST_SKIP(D_TESTS_EXPR_NO_SOURCE);
}

#endif  // the heap, the text and the facts
