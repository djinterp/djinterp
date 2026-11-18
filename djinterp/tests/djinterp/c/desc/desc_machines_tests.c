/*******************************************************************************
* djinterp [test]                                          desc_machines_tests.c
*
*   The suite of desc's machines and of describing to a stream, in DTest's C
* face. A toy type, a box of items, is described to the text machine in both
* presets, to the JSON machine in every switch, through the collecting
* machine and back, and as desc's options choose. Each test checks only what
* the build's knobs built.
*
*
* path:      /tests/djinterp/c/desc/desc_machines_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/

#include "./desc_tests.h"  // corresponding header

// std
#include <locale.h>  // setlocale, LC_NUMERIC
#include <string.h>  // strcmp
// djinterp
#include "../../../../inc/djinterp/c/desc/desc_machines.h"  // the machines
#include "../../../../inc/djinterp/c/desc/desc_options.h"   // the djin
#include "../../../../inc/djinterp/c/util/ostream/ostream.h"  // fixed streams


//==============================================================================
// THE BOX
//==============================================================================

// dj_tests_box
//   struct: the toy type: a name, a ratio, and up to three items.
struct dj_tests_box
{
    const char* name;
    double      ratio;
    size_t      count;
    const char* items[3];
};

/*
d_tests_box_read
  Reads one field: the count, the name, the ratio.
*/
static struct dj_ostream_value
d_tests_box_read(
    const void* _object,
    uint32_t    _field
)
{
    const struct dj_tests_box* box = (const struct dj_tests_box*)_object;

    switch (_field)
    {
        case 0u:
            return d_ostream_value_size(box->count);
        case 1u:
            return d_ostream_value_text(box->name);
        case 2u:
            return d_ostream_value_real(box->ratio);
        default:
            return d_ostream_value_none();
    }
}

// KEYS
//   constant: the items' keys, by position, so a level's keys are unique.
static const char* const KEYS[] = { "0", "1", "2" };

/*
d_tests_box_contents
  The items, one level down, each keyed by its position.
*/
static int
d_tests_box_contents(
    const void*          _object,
    struct dj_desc_sink* _sink
)
{
    const struct dj_tests_box* box     = (const struct dj_tests_box*)_object;
    int                        stopped = 0;
    size_t                     index;

    if (d_desc_enter(_sink, "items", "items") == 0)
    {
        return (_sink->stopped != 0u) ? 1 : 0;
    }

    for (index = 0u; (index < box->count) && (stopped == 0); ++index)
    {
        stopped = d_desc_emit(_sink, KEYS[index], NULL,
                              d_ostream_value_text(box->items[index]),
                              D_DESC_LAYER_CONTENTS, 0u,
                              D_DESC_TRUTH_OBSERVED);
    }

    d_desc_leave(_sink);

    return stopped;
}

/*
d_tests_box_words
  A text describer that writes in two pieces, with a quote, a line break, a
tab and two-byte characters, for the escaper.
*/
static int
d_tests_box_words(
    const void* _object,
    fn_desc_put _put,
    void*       _context
)
{
    (void)_object;

    if (_put(_context, "He said \"hi\"", 12u) != 0)
    {
        return 1;
    }

    return _put(_context, "\n\tthen \xC3\xA9t\xC3\xA9.", 13u);
}

#if D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS
static const struct dj_desc_op D_TESTS_BOX_NOUN[] =
    { D_DESC_T("box"), D_DESC_END };
#if D_DESC_BUILT(LINE, STATE)
static const struct dj_desc_op D_TESTS_BOX_ITEMS[] =
    { D_DESC_F(0u), D_DESC_T(" item"), D_DESC_PL, D_DESC_END };
#endif
#if D_DESC_BUILT(DETAILED, COST)
static const struct dj_desc_op D_TESTS_BOX_RATIO[] =
    { D_DESC_T("Its ratio is "), D_DESC_F(2u), D_DESC_T("."), D_DESC_END };
#endif

static const struct dj_desc_unit d_tests_box_units[] =
{
    D_DESC_UNIT(D_DESC_RING_NAME, D_DESC_LAYER_TYPE, D_DESC_SLOT_NOUN, 0u, 0u,
                D_TESTS_BOX_NOUN),
#if D_DESC_BUILT(LINE, STATE)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_WITH, 0u,
                0u, D_TESTS_BOX_ITEMS),
#endif
#if D_DESC_BUILT(DETAILED, COST)
    D_DESC_UNIT(D_DESC_RING_DETAILED, D_DESC_LAYER_COST, D_DESC_SLOT_REMARK,
                0u, 0u, D_TESTS_BOX_RATIO),
#endif
};
#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS

#if D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS
static const struct dj_desc_field d_tests_box_fields[] =
{
    D_DESC_FIELD("count", "items", D_DESC_LAYER_STATE, 0u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("name", "name", D_DESC_LAYER_IDENTITY, 0u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("ratio", "ratio", D_DESC_LAYER_COST, 1u, D_DESC_TRUTH_BUILD)
};
#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS

// d_desc_type_dj_tests_box
//   constant: the box's description.
static const struct dj_desc_type d_desc_type_dj_tests_box =
{
    "box",
    "box",
    D_DESC_UNITS_TABLE(d_tests_box_units),
    D_DESC_FIELDS_TABLE(d_tests_box_fields),
    NULL,
    d_tests_box_read,
    d_tests_box_contents
};

// D_TESTS_DESC_DEFAULTS
//   constant: 1 when this build's knobs that choose what describe writes are
// their defaults, so its output is the one expected.
#define D_TESTS_DESC_DEFAULTS                                                  \
    ( D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS && D_CFG_DESC_FACTS &&            \
      (D_CFG_DESC_RING_MAX == 3) && (D_CFG_DESC_LAYERS_BUILT == 0x1FF) &&      \
      (D_CFG_DESC_LAYERS == 0x1FF) && D_CFG_DESC_ARTICLES &&                   \
      (D_CFG_DESC_RING == 1) && (D_CFG_DESC_STYLE == 0) &&                     \
      (D_CFG_DESC_FORM == 0) && (D_CFG_DESC_FORMAT == 0) &&                    \
      (D_CFG_DESC_TEXT_PRESET == 0) && (D_CFG_DESC_MACHINES == 0x7) )

// D_TESTS_BOX_FACTS
//   constant: 1 when this build derives the box's facts as the expected
// outputs have them: every field, every layer, and its contents.
#define D_TESTS_BOX_FACTS                                                      \
    ( D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS )

// the box's facts as each machine writes them
#define D_TESTS_BOX_REPORT                                                     \
    "type                box\n"                                                \
    "items               2\n"                                                  \
    "name                crate a\n"                                            \
    "ratio               0.5\n"                                                \
    "items:\n"                                                                 \
    "  0                 nail\n"                                               \
    "  1                 screw \"x\"\n"
#define D_TESTS_BOX_LINE                                                       \
    "type=box count=2 name=\"crate a\" ratio=0.5 items.0=nail "                \
    "items.1=\"screw \\\"x\\\"\"\n"
#define D_TESTS_BOX_JSON                                                       \
    "{\"type\":\"box\",\"count\":2,\"name\":\"crate a\","                      \
    "\"ratio\":0.5,\"items\":{\"0\":\"nail\",\"1\":\"screw \\\"x\\\"\"}}"

/*
d_tests_box_fill
  A box of two items.
*/
static void
d_tests_box_fill(
    struct dj_tests_box* _box
)
{
    _box->name     = "crate a";
    _box->ratio    = 0.5;
    _box->count    = 2u;
    _box->items[0] = "nail";
    _box->items[1] = "screw \"x\"";
    _box->items[2] = NULL;

    return;
}

// dj_tests_out
//   struct: a fixed stream over a buffer, read once flushed.
struct dj_tests_out
{
    char                    text[1024];
    struct dj_ostream       os;
    struct dj_ostream_fixed fixed;
};

/*
d_tests_out_init
  An empty output.
*/
static struct dj_ostream*
d_tests_out_init(
    struct dj_tests_out* _out
)
{
    (void)d_ostream_init_fixed(&_out->os, &_out->fixed, _out->text,
                               sizeof(_out->text), 1);

    return &_out->os;
}

/*
d_tests_out_is
  Whether what was written is the text expected.
*/
static int
d_tests_out_is(
    struct dj_tests_out* _out,
    const char*          _expected
)
{
    (void)d_ostream_flush(&_out->os);

    return (strcmp(_out->text, _expected) == 0) ? 1 : 0;
}


//==============================================================================
// THE TESTS
//==============================================================================

/*
d_tests_desc_machines_text
  Checks the following:
  - the report: a line per fact, the label padded, a level per indent, the
    switches for keys and truths
  - the line: key=value pairs, a nested key dotted, text quoted where it
    must be, by the stream's JSON escaper, and a line break when asked
  - a text describer's pieces: as written in a report, one quoted value in
    a line
*/
static void
d_tests_desc_machines_text(
    struct dj_test_suite* _suite
)
{
#if D_DESC_MACHINE_BUILT(D_DESC_FORMAT_TEXT) && D_TESTS_BOX_FACTS
    struct dj_tests_box       box;
    struct dj_tests_out       out;
    struct dj_desc_text_state state;
    struct dj_desc_sink       sink;

    (void)d_test_suite_test(_suite, "the text machine: report and line");
    d_tests_box_fill(&box);

    d_desc_text_init(&state, d_tests_out_init(&out), D_DESC_TEXT_REPORT, 0u);
    d_desc_sink_init(&sink, d_desc_text_machine(), &state);
    D_TEST_CHECK(_suite, d_desc_run_facts(&sink, &d_desc_type_dj_tests_box,
                                          &box) == 0);
    D_TEST_CHECK(_suite, d_tests_out_is(&out, D_TESTS_BOX_REPORT));

    d_desc_text_init(&state, d_tests_out_init(&out), D_DESC_TEXT_REPORT,
                     D_DESC_TEXT_SHOW_KEYS | D_DESC_TEXT_SHOW_TRUTH);
    state.key_width = 0u;
    d_desc_sink_init(&sink, d_desc_text_machine(), &state);
    (void)d_desc_run_facts(&sink, &d_desc_type_dj_tests_box, &box);
    D_TEST_CHECK(_suite, d_tests_out_is(&out,
        "type: box (guaranteed)\ncount: 2 (observed)\n"
        "name: crate a (observed)\nratio: 0.5 (build)\nitems:\n"
        "  0: nail (observed)\n  1: screw \"x\" (observed)\n"));

    d_desc_text_init(&state, d_tests_out_init(&out), D_DESC_TEXT_LINE,
                     D_DESC_TEXT_NEWLINE);
    d_desc_sink_init(&sink, d_desc_text_machine(), &state);
    (void)d_desc_run_facts(&sink, &d_desc_type_dj_tests_box, &box);
    D_TEST_CHECK(_suite, d_tests_out_is(&out, D_TESTS_BOX_LINE));

#if D_CFG_DESC_TEXT
    d_desc_text_init(&state, d_tests_out_init(&out), D_DESC_TEXT_LINE, 0u);
    d_desc_sink_init(&sink, d_desc_text_machine(), &state);
    (void)d_desc_run_text(&sink, d_tests_box_words, &box);
    D_TEST_CHECK(_suite, d_tests_out_is(&out,
        "text=\"He said \\\"hi\\\"\\n\\tthen \xC3\xA9t\xC3\xA9.\""));

    d_desc_text_init(&state, d_tests_out_init(&out), D_DESC_TEXT_REPORT, 0u);
    d_desc_sink_init(&sink, d_desc_text_machine(), &state);
    (void)d_desc_run_text(&sink, d_tests_box_words, &box);
    D_TEST_CHECK(_suite, d_tests_out_is(&out,
        "He said \"hi\"\n\tthen \xC3\xA9t\xC3\xA9.\n"));
#endif
#else
    (void)_suite;
    (void)d_tests_box_words;
    (void)d_tests_box_fill;
    (void)d_tests_out_is;
#endif

    return;
}

/*
d_tests_desc_machines_json
  Checks the following:
  - compact JSON, a level an object, every key and value by the stream
  - pretty, with integers as strings and a closing line break
  - non-finite reals as strings when asked, else null
  - a text describer's pieces as one string, escaped by the stream
  - bare members, for a caller's own object; levels a stopped describer
    left open are closed
*/
static void
d_tests_desc_machines_json(
    struct dj_test_suite* _suite
)
{
#if D_DESC_MACHINE_BUILT(D_DESC_FORMAT_JSON) && D_TESTS_BOX_FACTS
    struct dj_tests_box       box;
    struct dj_tests_out       out;
    struct dj_desc_json_state state;
    struct dj_desc_sink       sink;
    double                    zero = 0.0;

    (void)d_test_suite_test(_suite, "the JSON machine");
    d_tests_box_fill(&box);

    d_desc_json_init(&state, d_tests_out_init(&out), 0u);
    d_desc_sink_init(&sink, d_desc_json_machine(), &state);
    D_TEST_CHECK(_suite, d_desc_run_facts(&sink, &d_desc_type_dj_tests_box,
                                          &box) == 0);
    D_TEST_CHECK(_suite, d_tests_out_is(&out, D_TESTS_BOX_JSON));

    d_desc_json_init(&state, d_tests_out_init(&out),
                     D_DESC_JSON_PRETTY | D_DESC_JSON_INTEGER_STRINGS |
                     D_DESC_JSON_NEWLINE);
    d_desc_sink_init(&sink, d_desc_json_machine(), &state);
    (void)d_desc_run_facts(&sink, &d_desc_type_dj_tests_box, &box);
    D_TEST_CHECK(_suite, d_tests_out_is(&out,
        "{\n  \"type\": \"box\",\n  \"count\": \"2\",\n"
        "  \"name\": \"crate a\",\n  \"ratio\": 0.5,\n  \"items\": {\n"
        "    \"0\": \"nail\",\n    \"1\": \"screw \\\"x\\\"\"\n  }\n}\n"));

    // non-finite reals: strings when asked, else null
    d_desc_json_init(&state, d_tests_out_init(&out),
                     D_DESC_JSON_NONFINITE_STRINGS);
    d_desc_sink_init(&sink, d_desc_json_machine(), &state);
    (void)d_desc_json_machine()->begin(&state);
    (void)d_desc_emit(&sink, "a", NULL, d_ostream_value_real(zero / zero),
                      0u, 0u, 0u);
    (void)d_desc_emit(&sink, "b", NULL, d_ostream_value_real(-1.0 / zero),
                      0u, 0u, 0u);
    (void)d_desc_emit(&sink, "c", NULL, d_ostream_value_real(1.5), 0u, 0u,
                      0u);
    d_desc_json_machine()->end(&state);
    D_TEST_CHECK(_suite, d_tests_out_is(&out,
        "{\"a\":\"NaN\",\"b\":\"-Infinity\",\"c\":1.5}"));
    d_desc_json_init(&state, d_tests_out_init(&out), 0u);
    d_desc_sink_init(&sink, d_desc_json_machine(), &state);
    (void)d_desc_json_machine()->begin(&state);
    (void)d_desc_emit(&sink, "a", NULL, d_ostream_value_real(zero / zero),
                      0u, 0u, 0u);
    d_desc_json_machine()->end(&state);
    D_TEST_CHECK(_suite, d_tests_out_is(&out, "{\"a\":null}"));

#if D_CFG_DESC_TEXT
    d_desc_json_init(&state, d_tests_out_init(&out), 0u);
    d_desc_sink_init(&sink, d_desc_json_machine(), &state);
    (void)d_desc_run_text(&sink, d_tests_box_words, &box);
    D_TEST_CHECK(_suite, d_tests_out_is(&out,
        "{\"text\":\"He said \\\"hi\\\"\\n\\tthen \xC3\xA9t\xC3\xA9.\"}"));
#endif

    // bare, and a level left open
    d_desc_json_init(&state, d_tests_out_init(&out), D_DESC_JSON_BARE);
    d_desc_sink_init(&sink, d_desc_json_machine(), &state);
    (void)d_desc_json_machine()->begin(&state);
    (void)d_desc_emit(&sink, "k", NULL, d_ostream_value_long(-3L), 0u, 0u,
                      0u);
    D_TEST_CHECK(_suite, d_desc_enter(&sink, "inner", NULL) == 1);
    (void)d_desc_emit(&sink, "t", NULL, d_ostream_value_bool(1), 0u, 0u, 0u);
    d_desc_json_machine()->end(&state);
    D_TEST_CHECK(_suite, d_tests_out_is(&out,
                                        "\"k\":-3,\"inner\":{\"t\":true}"));
#else
    (void)_suite;
#endif

    return;
}

/*
d_tests_count_enter, d_tests_count_leave
  A machine that counts levels open: +1 at each enter, -1 at each leave.
*/
static int
d_tests_count_enter(
    void*       _context,
    const char* _key,
    const char* _label,
    uint32_t    _depth
)
{
    (void)_key;
    (void)_label;
    (void)_depth;
    ++*(int*)_context;

    return 0;
}

static int
d_tests_count_leave(
    void*    _context,
    uint32_t _depth
)
{
    (void)_depth;
    --*(int*)_context;

    return 0;
}

/*
d_tests_desc_machines_collect
  Checks the following:
  - collected facts and levels, replayed, write what the describer would
  - past the array's room facts are counted, not kept, and what was kept
    still replays whole
*/
static void
d_tests_desc_machines_collect(
    struct dj_test_suite* _suite
)
{
#if D_DESC_MACHINE_BUILT(D_DESC_FORMAT_COLLECT) &&                            \
    D_DESC_MACHINE_BUILT(D_DESC_FORMAT_JSON) && D_TESTS_BOX_FACTS
    struct dj_tests_box          box;
    struct dj_tests_out          out;
    struct dj_desc_json_state    json;
    struct dj_desc_collect_state state;
    struct dj_desc_item          items[8];
    struct dj_desc_sink          sink;

    (void)d_test_suite_test(_suite, "the collecting machine and replay");
    d_tests_box_fill(&box);

    d_desc_collect_init(&state, items, 8u);
    d_desc_sink_init(&sink, d_desc_collect_machine(), &state);
    D_TEST_CHECK(_suite, d_desc_facts(&d_desc_type_dj_tests_box, &box,
                                      &sink) == 0);
    D_TEST_CHECK(_suite, (state.count == 7u) && (state.dropped == 0u) &&
                         (items[5].kind == D_DESC_ITEM_FACT) &&
                         (items[4].kind == D_DESC_ITEM_ENTER));

    d_desc_json_init(&json, d_tests_out_init(&out), 0u);
    (void)d_desc_json_machine()->begin(&json);
    D_TEST_CHECK(_suite, d_desc_replay(&state, d_desc_json_machine(),
                                       &json) == 0);
    d_desc_json_machine()->end(&json);
    D_TEST_CHECK(_suite, d_tests_out_is(&out, D_TESTS_BOX_JSON));

    // three kept: type, count, name; the ratio and both items counted
    d_desc_collect_init(&state, items, 3u);
    d_desc_sink_init(&sink, d_desc_collect_machine(), &state);
    (void)d_desc_facts(&d_desc_type_dj_tests_box, &box, &sink);
    D_TEST_CHECK(_suite, (state.count == 3u) && (state.dropped == 3u));
    d_desc_json_init(&json, d_tests_out_init(&out), 0u);
    (void)d_desc_json_machine()->begin(&json);
    (void)d_desc_replay(&state, d_desc_json_machine(), &json);
    d_desc_json_machine()->end(&json);
    D_TEST_CHECK(_suite, d_tests_out_is(&out,
        "{\"type\":\"box\",\"count\":2,\"name\":\"crate a\"}"));

    // every level opened is closed, the last at the end
    {
        static const struct dj_desc_machine COUNTING =
            { NULL, NULL, d_tests_count_enter, d_tests_count_leave, NULL,
              NULL };
        int levels = 0;

        d_desc_collect_init(&state, items, 8u);
        d_desc_sink_init(&sink, d_desc_collect_machine(), &state);
        (void)d_desc_facts(&d_desc_type_dj_tests_box, &box, &sink);
        D_TEST_CHECK(_suite, (d_desc_replay(&state, &COUNTING, &levels) ==
                              0) && (levels == 0));
    }

    // nothing to give, and nothing to give it to
    d_desc_collect_init(&state, NULL, 4u);
    D_TEST_CHECK(_suite, (state.capacity == 0u) &&
                         (d_desc_replay(&state, NULL, NULL) == 0));
#else
    (void)_suite;
#endif

    return;
}

/*
d_tests_desc_machines_options
  Checks the following:
  - describe's four options default to their knobs
  - d_desc_write writes prose by default, facts when the style says, and in
    the format a type's own scope chooses
  - d_desc_write_with runs the describer desc.form chooses, or the other
  - a number in prose is in the C form under a locale with a decimal comma
*/
static void
d_tests_desc_machines_options(
    struct dj_test_suite* _suite
)
{
    struct dj_tests_box box;
    struct dj_tests_out out;

    (void)d_test_suite_test(_suite, "describing as the options say");
    d_tests_box_fill(&box);

    D_TEST_CHECK(_suite,
        (d_desc_option_style(NULL) == (uint32_t)D_CFG_DESC_STYLE)   &&
        (d_desc_option_form(NULL) == (uint32_t)D_CFG_DESC_FORM)     &&
        (d_desc_option_format(NULL) == (uint32_t)D_CFG_DESC_FORMAT) &&
        (d_desc_option_text(NULL) == (uint32_t)D_CFG_DESC_TEXT_PRESET));
    D_TEST_CHECK(_suite, (d_desc_write(NULL, &d_desc_type_dj_tests_box, &box,
                                       0u) == -1) &&
                         (d_desc_write(d_tests_out_init(&out), NULL, &box,
                                       0u) == -1));

#if D_TESTS_DESC_DEFAULTS && D_DESC_OPTIONS_BUILT && D_TESTS_BOX_FACTS
    D_TEST_CHECK(_suite, d_desc_write(d_tests_out_init(&out),
                                      &d_desc_type_dj_tests_box, &box,
                                      D_DESC_CHOICE_OPTION) == 0);
    D_TEST_CHECK(_suite, d_tests_out_is(&out, "A box with 2 items\n"));
    D_TEST_CHECK(_suite, d_desc_write(d_tests_out_init(&out),
                                      &d_desc_type_dj_tests_box, &box,
                                      D_DESC_STYLE_FACTS) == 0);
    D_TEST_CHECK(_suite, d_tests_out_is(&out, D_TESTS_BOX_REPORT));

#if (D_CFG_DESC_SCOPES >= 2)
    // the box's own scope: JSON, as facts
    D_TEST_CHECK(_suite, d_desc_declare(&d_desc_type_dj_tests_box) == 0);
    D_TEST_CHECK(_suite, d_option_env_put(d_desc_djin(),
                                          "box.format=json").is_ok &&
                         d_option_env_put(d_desc_djin(),
                                          "box.style=facts").is_ok);
    D_TEST_CHECK(_suite, (d_desc_option_format(&d_desc_type_dj_tests_box) ==
                          D_DESC_FORMAT_JSON) &&
                         (d_desc_option_format(NULL) ==
                          D_DESC_FORMAT_TEXT));
    (void)d_desc_write(d_tests_out_init(&out), &d_desc_type_dj_tests_box,
                       &box, D_DESC_CHOICE_OPTION);
    D_TEST_CHECK(_suite, d_tests_out_is(&out, D_TESTS_BOX_JSON "\n"));
#endif
    D_TEST_CHECK(_suite, !d_option_env_put(d_desc_djin(),
                                           "format=yaml").is_ok);

    // the form: facts by default, text when asked, the other when absent
    (void)d_desc_write_with(d_tests_out_init(&out), NULL,
                            d_tests_box_contents, d_tests_box_words, &box);
    D_TEST_CHECK(_suite, d_tests_out_is(&out,
        "items:\n  0                 nail\n  1                 screw \"x\"\n"));
#if D_CFG_DESC_TEXT
    D_TEST_CHECK(_suite, d_option_env_put(d_desc_djin(), "form=text").is_ok);
    (void)d_desc_write_with(d_tests_out_init(&out), NULL,
                            d_tests_box_contents, d_tests_box_words, &box);
    D_TEST_CHECK(_suite, d_tests_out_is(&out,
        "He said \"hi\"\n\tthen \xC3\xA9t\xC3\xA9.\n"));
    (void)d_desc_write_with(d_tests_out_init(&out), NULL,
                            d_tests_box_contents, NULL, &box);
    D_TEST_CHECK(_suite, d_tests_out_is(&out,
        "items:\n  0                 nail\n  1                 screw \"x\"\n"));
#else
    D_TEST_CHECK(_suite, !d_option_env_put(d_desc_djin(), "form=text").is_ok);
#endif
    D_TEST_CHECK(_suite, d_desc_write_with(d_tests_out_init(&out), NULL,
                                           NULL, NULL, &box) == -1);

    // prose's numbers in the C form, whatever the locale
    D_TEST_CHECK(_suite, d_option_env_put(d_desc_djin(),
                                          "ring=detailed").is_ok);

    if (setlocale(LC_NUMERIC, "de_DE.UTF-8") != NULL)
    {
        (void)d_desc_write(d_tests_out_init(&out), &d_desc_type_dj_tests_box,
                           &box, D_DESC_STYLE_UNITS);
        D_TEST_CHECK(_suite, d_tests_out_is(&out,
            "A box with 2 items. Its ratio is 0.5.\n"));
        (void)setlocale(LC_NUMERIC, "C");
    }
    else
    {
        (void)d_test_suite_note(_suite, "de_DE.UTF-8 is not installed: the "
                                        "locale check did not run");
    }

    d_option_env_clear(d_desc_djin());
#endif

    return;
}


//==============================================================================
// THE SECTION
//==============================================================================

void
d_tests_desc_machines(
    struct dj_test_suite* _suite
)
{
    // in desc's module, which d_tests_desc opened
    (void)d_test_suite_block(_suite, "desc_machines");

    d_tests_desc_machines_text(_suite);
    d_tests_desc_machines_json(_suite);
    d_tests_desc_machines_collect(_suite);
    d_tests_desc_machines_options(_suite);

    return;
}
