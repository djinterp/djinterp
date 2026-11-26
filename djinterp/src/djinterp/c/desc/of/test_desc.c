/*******************************************************************************
* djinterp [c][desc]                                                 test_desc.c
*
*   desc's describers of DTest's C types: a run's summary and options, and
* a module's, a block's and a test's spec. For each, its plan, its reader,
* its units and its field list, and for a module's and a block's spec the
* contents a level down; and the layout rows as this library was built. A
* describer reads fields only, so this unit calls none of DTest's
* functions.
*
*
* path:      /src/djinterp/c/desc/of/test_desc.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

#include "../../../../../inc/djinterp/c/desc/of/test_desc.h"  // header

#if D_INTERNAL_DESC_OF_TEST

//==============================================================================
// 1.  THE SUMMARY
//==============================================================================

// the fields, in the field list's order
#define D_TEST_DESC_SUM_PASSED         0u
#define D_TEST_DESC_SUM_TESTS          1u
#define D_TEST_DESC_SUM_TESTS_PASSED   2u
#define D_TEST_DESC_SUM_TESTS_FAILED   3u
#define D_TEST_DESC_SUM_TESTS_SKIPPED  4u
#define D_TEST_DESC_SUM_CHECKS         5u
#define D_TEST_DESC_SUM_CHECKS_FAILED  6u
#define D_TEST_DESC_SUM_MODULES        7u

// the plan's bits
#define D_TEST_DESC_SUM_IF_PASSING     (1u << 0)
#define D_TEST_DESC_SUM_IF_FAILING     (1u << 1)
#define D_TEST_DESC_SUM_IF_EMPTY       (1u << 2)
#define D_TEST_DESC_SUM_IF_SKIPPED     (1u << 3)

/*
d_internal_test_desc_failing
  1 when a run failed a test or a check.
*/
static int
d_internal_test_desc_failing(
    const struct dj_test_summary* _summary
)
{
    return ( (_summary->tests_failed != 0u) ||
             (_summary->checks_failed != 0u) ) ? 1 : 0;
}

/*
d_internal_test_desc_summary_plan
  Passing, failing or empty at the line; the skips at the sentence.
*/
static uint32_t
d_internal_test_desc_summary_plan(
    const void* _object,
    uint32_t    _ring
)
{
    const struct dj_test_summary* summary =
        (const struct dj_test_summary*)_object;
    uint32_t bits = 0u;

    if (_ring >= D_DESC_RING_LINE)
    {
        if (d_internal_test_desc_failing(summary) != 0)
        {
            bits |= D_TEST_DESC_SUM_IF_FAILING;
        }
        else
        {
            bits |= (summary->tests == 0u) ? D_TEST_DESC_SUM_IF_EMPTY
                                           : D_TEST_DESC_SUM_IF_PASSING;
        }
    }

    if ( (_ring >= D_DESC_RING_SENTENCE) && (summary->tests_skipped != 0u) )
    {
        bits |= D_TEST_DESC_SUM_IF_SKIPPED;
    }

    return bits;
}

/*
d_internal_test_desc_summary_read
  Reads one field; the tests passed are those neither failed nor skipped.
*/
static struct dj_ostream_value
d_internal_test_desc_summary_read(
    const void* _object,
    uint32_t    _field
)
{
    const struct dj_test_summary* summary =
        (const struct dj_test_summary*)_object;
    size_t settled = summary->tests_failed + summary->tests_skipped;

    switch (_field)
    {
        case D_TEST_DESC_SUM_PASSED:
            return d_ostream_value_bool(
                (d_internal_test_desc_failing(summary) == 0) ? 1 : 0);
        case D_TEST_DESC_SUM_TESTS:
            return d_ostream_value_size(summary->tests);
        case D_TEST_DESC_SUM_TESTS_PASSED:
            return d_ostream_value_size( (summary->tests > settled)
                                             ? (summary->tests - settled)
                                             : 0u );
        case D_TEST_DESC_SUM_TESTS_FAILED:
            return d_ostream_value_size(summary->tests_failed);
        case D_TEST_DESC_SUM_TESTS_SKIPPED:
            return d_ostream_value_size(summary->tests_skipped);
        case D_TEST_DESC_SUM_CHECKS:
            return d_ostream_value_size(summary->checks);
        case D_TEST_DESC_SUM_CHECKS_FAILED:
            return d_ostream_value_size(summary->checks_failed);
        case D_TEST_DESC_SUM_MODULES:
            return d_ostream_value_size(summary->modules);
        default:
            return d_ostream_value_none();
    }
}

#if D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS
#if D_DESC_BUILT(LINE, STATE)
static const struct dj_desc_op D_TEST_DESC_OPS_PASSING[] =
    { D_DESC_T("passing"), D_DESC_END };
static const struct dj_desc_op D_TEST_DESC_OPS_FAILING[] =
    { D_DESC_T("failing"), D_DESC_END };
static const struct dj_desc_op D_TEST_DESC_OPS_EMPTY[] =
    { D_DESC_T("empty"), D_DESC_END };
static const struct dj_desc_op D_TEST_DESC_OPS_TESTS[] =
    { D_DESC_F(D_TEST_DESC_SUM_TESTS), D_DESC_T(" test"), D_DESC_PL,
      D_DESC_END };
static const struct dj_desc_op D_TEST_DESC_OPS_CHECKS[] =
    { D_DESC_F(D_TEST_DESC_SUM_CHECKS), D_DESC_T(" check"), D_DESC_PL,
      D_DESC_END };
#endif
#if D_DESC_BUILT(SENTENCE, HEALTH)
static const struct dj_desc_op D_TEST_DESC_OPS_FAILURES[] =
    { D_DESC_T("Its failures: "), D_DESC_F(D_TEST_DESC_SUM_TESTS_FAILED),
      D_DESC_T(" test"), D_DESC_PL, D_DESC_T(" and "),
      D_DESC_F(D_TEST_DESC_SUM_CHECKS_FAILED), D_DESC_T(" check"),
      D_DESC_PL, D_DESC_T("."), D_DESC_END };
#endif
#if D_DESC_BUILT(SENTENCE, STATE)
static const struct dj_desc_op D_TEST_DESC_OPS_SKIPPED[] =
    { D_DESC_T("It skipped "), D_DESC_F(D_TEST_DESC_SUM_TESTS_SKIPPED),
      D_DESC_T(" test"), D_DESC_PL, D_DESC_T("."), D_DESC_END };
#endif
#if D_DESC_BUILT(DETAILED, COST)
static const struct dj_desc_op D_TEST_DESC_OPS_MODULES[] =
    { D_DESC_T("It opened "), D_DESC_F(D_TEST_DESC_SUM_MODULES),
      D_DESC_T(" module"), D_DESC_PL, D_DESC_T("."), D_DESC_END };
#endif
static const struct dj_desc_op D_TEST_DESC_OPS_SUMMARY[] =
    { D_DESC_T("test summary"), D_DESC_END };

static const struct dj_desc_unit d_internal_test_desc_summary_units[] =
{
#if D_DESC_BUILT(LINE, STATE)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_CONDITION,
                D_TEST_DESC_SUM_IF_PASSING, 0u, D_TEST_DESC_OPS_PASSING),
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_CONDITION,
                D_TEST_DESC_SUM_IF_FAILING, 0u, D_TEST_DESC_OPS_FAILING),
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_CONDITION,
                D_TEST_DESC_SUM_IF_EMPTY, 0u, D_TEST_DESC_OPS_EMPTY),
#endif
    D_DESC_UNIT(D_DESC_RING_NAME, D_DESC_LAYER_TYPE, D_DESC_SLOT_NOUN,
                0u, 0u, D_TEST_DESC_OPS_SUMMARY),
#if D_DESC_BUILT(LINE, STATE)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_WITH,
                0u, D_TEST_DESC_SUM_IF_EMPTY, D_TEST_DESC_OPS_TESTS),
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_WITH,
                0u, D_TEST_DESC_SUM_IF_EMPTY, D_TEST_DESC_OPS_CHECKS),
#endif
#if D_DESC_BUILT(SENTENCE, HEALTH)
    D_DESC_UNIT(D_DESC_RING_SENTENCE, D_DESC_LAYER_HEALTH,
                D_DESC_SLOT_REMARK, D_TEST_DESC_SUM_IF_FAILING, 0u,
                D_TEST_DESC_OPS_FAILURES),
#endif
#if D_DESC_BUILT(SENTENCE, STATE)
    D_DESC_UNIT(D_DESC_RING_SENTENCE, D_DESC_LAYER_STATE,
                D_DESC_SLOT_REMARK, D_TEST_DESC_SUM_IF_SKIPPED, 0u,
                D_TEST_DESC_OPS_SKIPPED),
#endif
#if D_DESC_BUILT(DETAILED, COST)
    D_DESC_UNIT(D_DESC_RING_DETAILED, D_DESC_LAYER_COST,
                D_DESC_SLOT_REMARK, 0u, D_TEST_DESC_SUM_IF_EMPTY,
                D_TEST_DESC_OPS_MODULES),
#endif
};
#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS

#if D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS
static const struct dj_desc_field d_internal_test_desc_summary_fields[] =
{
    D_DESC_FIELD("passed", "passed", D_DESC_LAYER_HEALTH, 0u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("tests", "tests", D_DESC_LAYER_STATE, 0u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("tests_passed", "tests passed", D_DESC_LAYER_STATE, 1u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("tests_failed", "tests failed", D_DESC_LAYER_HEALTH, 0u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("tests_skipped", "tests skipped", D_DESC_LAYER_STATE, 1u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("checks", "checks", D_DESC_LAYER_STATE, 1u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("checks_failed", "checks failed", D_DESC_LAYER_HEALTH, 1u,
                 D_DESC_TRUTH_OBSERVED),
    D_DESC_FIELD("modules", "modules", D_DESC_LAYER_COST, 2u,
                 D_DESC_TRUTH_OBSERVED)
};
#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS

const struct dj_desc_type d_desc_type_dj_test_summary =
{
    "test_summary",
    "test summary",
    D_DESC_UNITS_TABLE(d_internal_test_desc_summary_units),
    D_DESC_FIELDS_TABLE(d_internal_test_desc_summary_fields),
    d_internal_test_desc_summary_plan,
    d_internal_test_desc_summary_read,
    NULL
};


//==============================================================================
// 2.  THE OPTIONS
//==============================================================================

#define D_TEST_DESC_OPT_SHOW              0u
#define D_TEST_DESC_OPT_TABLE             1u
#define D_TEST_DESC_OPT_COLOR             2u
#define D_TEST_DESC_OPT_FILTER            3u
#define D_TEST_DESC_OPT_STREAM            4u

#define D_TEST_DESC_OPT_IF_FILTER         (1u << 0)
#define D_TEST_DESC_OPT_IF_FAILURES       (1u << 1)
#define D_TEST_DESC_OPT_IF_TESTS          (1u << 2)
#define D_TEST_DESC_OPT_IF_CHECKS         (1u << 3)
#define D_TEST_DESC_OPT_IF_NO_TABLE       (1u << 4)
#define D_TEST_DESC_OPT_IF_STDERR         (1u << 5)
#define D_TEST_DESC_OPT_IF_OTHER          (1u << 6)

// what a run lists, by D_TEST_SUITE_SHOW_*
static const char* const D_TEST_DESC_SHOW_KEYS[] =
{
    "failures", "tests", "checks"
};

/*
d_internal_test_desc_stream
  Where a run writes: 0 stdout (or null, which is stdout), 1 stderr, 2 a
stream of the caller's.
*/
static unsigned
d_internal_test_desc_stream(
    const struct dj_test_suite_options* _options
)
{
    if ( (_options->stream == NULL) || (_options->stream == stdout) )
    {
        return 0u;
    }

    return (_options->stream == stderr) ? 1u : 2u;
}

/*
d_internal_test_desc_options_plan
  The filter at the line; what is listed, and the table, at the sentence;
the stream at the detail.
*/
static uint32_t
d_internal_test_desc_options_plan(
    const void* _object,
    uint32_t    _ring
)
{
    const struct dj_test_suite_options* options =
        (const struct dj_test_suite_options*)_object;
    uint32_t bits = 0u;
    unsigned stream;

    if ( (_ring >= D_DESC_RING_LINE) &&
         (options->filter != NULL) && (options->filter[0] != '\0') )
    {
        bits |= D_TEST_DESC_OPT_IF_FILTER;
    }

    if (_ring >= D_DESC_RING_SENTENCE)
    {
        bits |= (options->show == D_TEST_SUITE_SHOW_FAILURES)
                    ? D_TEST_DESC_OPT_IF_FAILURES : 0u;
        bits |= (options->show == D_TEST_SUITE_SHOW_TESTS)
                    ? D_TEST_DESC_OPT_IF_TESTS : 0u;
        bits |= (options->show == D_TEST_SUITE_SHOW_CHECKS)
                    ? D_TEST_DESC_OPT_IF_CHECKS : 0u;
        bits |= (!options->table) ? D_TEST_DESC_OPT_IF_NO_TABLE : 0u;
    }

    if (_ring >= D_DESC_RING_DETAILED)
    {
        stream = d_internal_test_desc_stream(options);
        bits  |= (stream == 1u) ? D_TEST_DESC_OPT_IF_STDERR : 0u;
        bits  |= (stream == 2u) ? D_TEST_DESC_OPT_IF_OTHER : 0u;
    }

    return bits;
}

/*
d_internal_test_desc_options_read
  Reads one field; a show past D_TEST_SUITE_SHOW_*'s, and an empty filter,
are none.
*/
static struct dj_ostream_value
d_internal_test_desc_options_read(
    const void* _object,
    uint32_t    _field
)
{
    static const char* const streams[] = { "stdout", "stderr", "other" };
    const struct dj_test_suite_options* options =
        (const struct dj_test_suite_options*)_object;

    switch (_field)
    {
        case D_TEST_DESC_OPT_SHOW:
            return ( (options->show >= 0) && (options->show <= 2) )
                       ? d_ostream_value_text(
                             D_TEST_DESC_SHOW_KEYS[options->show])
                       : d_ostream_value_none();
        case D_TEST_DESC_OPT_TABLE:
            return d_ostream_value_bool(options->table ? 1 : 0);
        case D_TEST_DESC_OPT_COLOR:
            return d_ostream_value_bool(options->color ? 1 : 0);
        case D_TEST_DESC_OPT_FILTER:
            return ( (options->filter != NULL) &&
                     (options->filter[0] != '\0') )
                       ? d_ostream_value_text(options->filter)
                       : d_ostream_value_none();
        case D_TEST_DESC_OPT_STREAM:
            return d_ostream_value_text(
                       streams[d_internal_test_desc_stream(options)]);
        default:
            return d_ostream_value_none();
    }
}

#if D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS
#if D_DESC_BUILT(LINE, POLICY)
static const struct dj_desc_op D_TEST_DESC_OPS_FILTER[] =
    { D_DESC_T("for the tests whose path holds "),
      D_DESC_F(D_TEST_DESC_OPT_FILTER), D_DESC_END };
#endif
#if D_DESC_BUILT(SENTENCE, POLICY)
static const struct dj_desc_op D_TEST_DESC_OPS_LIST_FAILURES[] =
    { D_DESC_T("It lists failures only."), D_DESC_END };
static const struct dj_desc_op D_TEST_DESC_OPS_LIST_TESTS[] =
    { D_DESC_T("It lists every test."), D_DESC_END };
static const struct dj_desc_op D_TEST_DESC_OPS_LIST_CHECKS[] =
    { D_DESC_T("It lists every check."), D_DESC_END };
static const struct dj_desc_op D_TEST_DESC_OPS_NO_TABLE[] =
    { D_DESC_T("It prints no results table."), D_DESC_END };
#endif
#if D_DESC_BUILT(DETAILED, PROVENANCE)
static const struct dj_desc_op D_TEST_DESC_OPS_STDOUT[] =
    { D_DESC_T("It writes to stdout."), D_DESC_END };
static const struct dj_desc_op D_TEST_DESC_OPS_STDERR[] =
    { D_DESC_T("It writes to stderr."), D_DESC_END };
static const struct dj_desc_op D_TEST_DESC_OPS_OTHER[] =
    { D_DESC_T("It writes to a stream of the caller's."), D_DESC_END };
#endif
static const struct dj_desc_op D_TEST_DESC_OPS_OPTIONS[] =
    { D_DESC_T("test run's options"), D_DESC_END };

static const struct dj_desc_unit d_internal_test_desc_options_units[] =
{
    D_DESC_UNIT(D_DESC_RING_NAME, D_DESC_LAYER_TYPE, D_DESC_SLOT_NOUN,
                0u, 0u, D_TEST_DESC_OPS_OPTIONS),
#if D_DESC_BUILT(LINE, POLICY)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_POLICY,
                D_DESC_SLOT_COMPLEMENT, D_TEST_DESC_OPT_IF_FILTER, 0u,
                D_TEST_DESC_OPS_FILTER),
#endif
#if D_DESC_BUILT(SENTENCE, POLICY)
    D_DESC_UNIT(D_DESC_RING_SENTENCE, D_DESC_LAYER_POLICY,
                D_DESC_SLOT_REMARK, D_TEST_DESC_OPT_IF_FAILURES, 0u,
                D_TEST_DESC_OPS_LIST_FAILURES),
    D_DESC_UNIT(D_DESC_RING_SENTENCE, D_DESC_LAYER_POLICY,
                D_DESC_SLOT_REMARK, D_TEST_DESC_OPT_IF_TESTS, 0u,
                D_TEST_DESC_OPS_LIST_TESTS),
    D_DESC_UNIT(D_DESC_RING_SENTENCE, D_DESC_LAYER_POLICY,
                D_DESC_SLOT_REMARK, D_TEST_DESC_OPT_IF_CHECKS, 0u,
                D_TEST_DESC_OPS_LIST_CHECKS),
    D_DESC_UNIT(D_DESC_RING_SENTENCE, D_DESC_LAYER_POLICY,
                D_DESC_SLOT_REMARK, D_TEST_DESC_OPT_IF_NO_TABLE, 0u,
                D_TEST_DESC_OPS_NO_TABLE),
#endif
#if D_DESC_BUILT(DETAILED, PROVENANCE)
    D_DESC_UNIT(D_DESC_RING_DETAILED, D_DESC_LAYER_PROVENANCE,
                D_DESC_SLOT_REMARK, 0u,
                D_TEST_DESC_OPT_IF_STDERR | D_TEST_DESC_OPT_IF_OTHER,
                D_TEST_DESC_OPS_STDOUT),
    D_DESC_UNIT(D_DESC_RING_DETAILED, D_DESC_LAYER_PROVENANCE,
                D_DESC_SLOT_REMARK, D_TEST_DESC_OPT_IF_STDERR, 0u,
                D_TEST_DESC_OPS_STDERR),
    D_DESC_UNIT(D_DESC_RING_DETAILED, D_DESC_LAYER_PROVENANCE,
                D_DESC_SLOT_REMARK, D_TEST_DESC_OPT_IF_OTHER, 0u,
                D_TEST_DESC_OPS_OTHER),
#endif
};
#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS

#if D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS
static const struct dj_desc_field d_internal_test_desc_options_fields[] =
{
    D_DESC_FIELD("show", "lists", D_DESC_LAYER_POLICY, 0u,
                 D_DESC_TRUTH_RUN_TIME),
    D_DESC_FIELD("table", "results table", D_DESC_LAYER_POLICY, 1u,
                 D_DESC_TRUTH_RUN_TIME),
    D_DESC_FIELD("color", "color", D_DESC_LAYER_POLICY, 3u,
                 D_DESC_TRUTH_RUN_TIME),
    D_DESC_FIELD("filter", "filter", D_DESC_LAYER_POLICY, 1u,
                 D_DESC_TRUTH_RUN_TIME),
    D_DESC_FIELD("stream", "writes to", D_DESC_LAYER_PROVENANCE, 2u,
                 D_DESC_TRUTH_RUN_TIME)
};
#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS

const struct dj_desc_type d_desc_type_dj_test_suite_options =
{
    "test_options",
    "test run's options",
    D_DESC_UNITS_TABLE(d_internal_test_desc_options_units),
    D_DESC_FIELDS_TABLE(d_internal_test_desc_options_fields),
    d_internal_test_desc_options_plan,
    d_internal_test_desc_options_read,
    NULL
};


//==============================================================================
// 3.  THE SPECS
//==============================================================================
//   A module's, a block's and a test's spec share their first two fields,
// a name and a descriptor, and so their first two units' texts.

#define D_TEST_DESC_SPEC_NAME           0u
#define D_TEST_DESC_SPEC_DESCRIPTOR     1u
#define D_TEST_DESC_SPEC_COUNT          2u   // a test's: runnable
#define D_TEST_DESC_SPEC_TESTS          3u   // a module's tests, all blocks'

#define D_TEST_DESC_SPEC_IF_NAMED       (1u << 0)
#define D_TEST_DESC_SPEC_IF_DESCRIBED   (1u << 1)
#define D_TEST_DESC_SPEC_IF_UNBOUND     (1u << 2)
#define D_TEST_DESC_SPEC_IF_EMPTY       (1u << 3)

/*
d_internal_test_desc_spec_bits
  The bits a name and a descriptor give.
*/
static uint32_t
d_internal_test_desc_spec_bits(
    const char* _name,
    const char* _descriptor,
    uint32_t    _ring
)
{
    uint32_t bits = 0u;

    if ( (_ring >= D_DESC_RING_LINE) && (_name != NULL) &&
         (_name[0] != '\0') )
    {
        bits |= D_TEST_DESC_SPEC_IF_NAMED;
    }

    if ( (_ring >= D_DESC_RING_SENTENCE) && (_descriptor != NULL) &&
         (_descriptor[0] != '\0') )
    {
        bits |= D_TEST_DESC_SPEC_IF_DESCRIBED;
    }

    return bits;
}

/*
d_internal_test_desc_module_tests
  The tests of a module's blocks, all of them.
*/
static size_t
d_internal_test_desc_module_tests(
    const struct dj_test_module_spec* _module
)
{
    size_t total = 0u;
    size_t index;

    for (index = 0u; (_module->blocks != NULL) && (index < _module->count);
         ++index)
    {
        total += _module->blocks[index].count;
    }

    return total;
}

/*
d_internal_test_desc_module_plan, d_internal_test_desc_block_plan,
d_internal_test_desc_case_plan
  Each spec's plan: its name and descriptor, and whether it is empty, or,
for a test, unbound.
*/
static uint32_t
d_internal_test_desc_module_plan(
    const void* _object,
    uint32_t    _ring
)
{
    const struct dj_test_module_spec* spec =
        (const struct dj_test_module_spec*)_object;
    uint32_t bits = d_internal_test_desc_spec_bits(spec->name,
                                                   spec->descriptor, _ring);

    if ( (_ring >= D_DESC_RING_LINE) && (spec->count == 0u) )
    {
        bits |= D_TEST_DESC_SPEC_IF_EMPTY;
    }

    return bits;
}

static uint32_t
d_internal_test_desc_block_plan(
    const void* _object,
    uint32_t    _ring
)
{
    const struct dj_test_block_spec* spec =
        (const struct dj_test_block_spec*)_object;
    uint32_t bits = d_internal_test_desc_spec_bits(spec->name,
                                                   spec->descriptor, _ring);

    if ( (_ring >= D_DESC_RING_LINE) && (spec->count == 0u) )
    {
        bits |= D_TEST_DESC_SPEC_IF_EMPTY;
    }

    return bits;
}

static uint32_t
d_internal_test_desc_case_plan(
    const void* _object,
    uint32_t    _ring
)
{
    const struct dj_test_case_spec* spec =
        (const struct dj_test_case_spec*)_object;
    uint32_t bits = d_internal_test_desc_spec_bits(spec->name,
                                                   spec->descriptor, _ring);

    if ( (_ring >= D_DESC_RING_LINE) && (spec->fn == NULL) )
    {
        bits |= D_TEST_DESC_SPEC_IF_UNBOUND;
    }

    return bits;
}

/*
d_internal_test_desc_module_read, d_internal_test_desc_block_read,
d_internal_test_desc_case_read
  Each spec's reader.
*/
static struct dj_ostream_value
d_internal_test_desc_module_read(
    const void* _object,
    uint32_t    _field
)
{
    const struct dj_test_module_spec* spec =
        (const struct dj_test_module_spec*)_object;

    switch (_field)
    {
        case D_TEST_DESC_SPEC_NAME:
            return d_ostream_value_text(spec->name);
        case D_TEST_DESC_SPEC_DESCRIPTOR:
            return d_ostream_value_text(spec->descriptor);
        case D_TEST_DESC_SPEC_COUNT:
            return d_ostream_value_size(spec->count);
        case D_TEST_DESC_SPEC_TESTS:
            return d_ostream_value_size(
                       d_internal_test_desc_module_tests(spec));
        default:
            return d_ostream_value_none();
    }
}

static struct dj_ostream_value
d_internal_test_desc_block_read(
    const void* _object,
    uint32_t    _field
)
{
    const struct dj_test_block_spec* spec =
        (const struct dj_test_block_spec*)_object;

    switch (_field)
    {
        case D_TEST_DESC_SPEC_NAME:
            return d_ostream_value_text(spec->name);
        case D_TEST_DESC_SPEC_DESCRIPTOR:
            return d_ostream_value_text(spec->descriptor);
        case D_TEST_DESC_SPEC_COUNT:
            return d_ostream_value_size(spec->count);
        default:
            return d_ostream_value_none();
    }
}

static struct dj_ostream_value
d_internal_test_desc_case_read(
    const void* _object,
    uint32_t    _field
)
{
    const struct dj_test_case_spec* spec =
        (const struct dj_test_case_spec*)_object;

    switch (_field)
    {
        case D_TEST_DESC_SPEC_NAME:
            return d_ostream_value_text(spec->name);
        case D_TEST_DESC_SPEC_DESCRIPTOR:
            return d_ostream_value_text(spec->descriptor);
        case D_TEST_DESC_SPEC_COUNT:
            return d_ostream_value_bool((spec->fn != NULL) ? 1 : 0);
        default:
            return d_ostream_value_none();
    }
}

/*
d_internal_test_desc_block_contents
  A block's tests, a fact each: its name, the test's own.
*/
static int
d_internal_test_desc_block_contents(
    const void*          _object,
    struct dj_desc_sink* _sink
)
{
    const struct dj_test_block_spec* spec =
        (const struct dj_test_block_spec*)_object;
    int                              stopped = 0;
    size_t                           index;

    if (d_desc_enter(_sink, "tests", "tests") == 0)
    {
        return (_sink->stopped != 0u) ? 1 : 0;
    }

    for (index = 0u;
         (spec->tests != NULL) && (index < spec->count) && (stopped == 0);
         ++index)
    {
        stopped = d_desc_emit(_sink, "test", NULL,
                              d_ostream_value_text(spec->tests[index].name),
                              D_DESC_LAYER_CONTENTS, 0u,
                              D_DESC_TRUTH_OBSERVED);
    }
    d_desc_leave(_sink);

    return stopped;
}

/*
d_internal_test_desc_module_contents
  A module's blocks, a level each, keyed by the block's name.
*/
static int
d_internal_test_desc_module_contents(
    const void*          _object,
    struct dj_desc_sink* _sink
)
{
    const struct dj_test_module_spec* spec =
        (const struct dj_test_module_spec*)_object;
    const struct dj_test_block_spec*  block;
    int                               stopped = 0;
    size_t                            index;

    for (index = 0u;
         (spec->blocks != NULL) && (index < spec->count) && (stopped == 0);
         ++index)
    {
        block = &spec->blocks[index];
        if (d_desc_enter(_sink, (block->name != NULL) ? block->name : "block",
                         block->descriptor) == 0)
        {
            return (_sink->stopped != 0u) ? 1 : 0;
        }
        stopped = (d_desc_facts(&d_desc_type_dj_test_block_spec, block,
                                _sink) > 0) ? 1 : 0;
        d_desc_leave(_sink);
    }

    return ( (stopped != 0) || (_sink->stopped != 0u) ) ? 1 : 0;
}

#if D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS
static const struct dj_desc_op D_TEST_DESC_OPS_MODULE[] =
    { D_DESC_T("test module"), D_DESC_END };
static const struct dj_desc_op D_TEST_DESC_OPS_BLOCK[] =
    { D_DESC_T("test block"), D_DESC_END };
static const struct dj_desc_op D_TEST_DESC_OPS_CASE[] =
    { D_DESC_T("test"), D_DESC_END };
#if D_DESC_BUILT(LINE, STATE)
static const struct dj_desc_op D_TEST_DESC_OPS_UNBOUND[] =
    { D_DESC_T("unbound"), D_DESC_END };
static const struct dj_desc_op D_TEST_DESC_OPS_BLOCKS[] =
    { D_DESC_F(D_TEST_DESC_SPEC_COUNT), D_DESC_T(" block"), D_DESC_PL,
      D_DESC_END };
static const struct dj_desc_op D_TEST_DESC_OPS_SPEC_TESTS[] =
    { D_DESC_F(D_TEST_DESC_SPEC_COUNT), D_DESC_T(" test"), D_DESC_PL,
      D_DESC_END };
#endif
#if D_DESC_BUILT(LINE, IDENTITY)
static const struct dj_desc_op D_TEST_DESC_OPS_NAMED[] =
    { D_DESC_T("named "), D_DESC_F(D_TEST_DESC_SPEC_NAME), D_DESC_END };
#endif
#if D_DESC_BUILT(SENTENCE, INTENT)
static const struct dj_desc_op D_TEST_DESC_OPS_DESCRIPTOR[] =
    { D_DESC_T("Its descriptor is \""),
      D_DESC_F(D_TEST_DESC_SPEC_DESCRIPTOR), D_DESC_T("\"."), D_DESC_END };
#endif
#if D_DESC_BUILT(DETAILED, COST)
static const struct dj_desc_op D_TEST_DESC_OPS_ALL_TESTS[] =
    { D_DESC_T("Its blocks hold "), D_DESC_F(D_TEST_DESC_SPEC_TESTS),
      D_DESC_T(" test"), D_DESC_PL, D_DESC_T("."), D_DESC_END };
#endif

// D_TEST_DESC_SPEC_UNITS
//   macro: the units each spec has, after its noun.
#if D_DESC_BUILT(LINE, IDENTITY)
    #define D_TEST_DESC_NAMED_UNIT                                             \
        D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_IDENTITY,                   \
                    D_DESC_SLOT_COMPLEMENT, D_TEST_DESC_SPEC_IF_NAMED, 0u,     \
                    D_TEST_DESC_OPS_NAMED),
#else
    #define D_TEST_DESC_NAMED_UNIT
#endif
#if D_DESC_BUILT(SENTENCE, INTENT)
    #define D_TEST_DESC_DESCRIPTOR_UNIT                                        \
        D_DESC_UNIT(D_DESC_RING_SENTENCE, D_DESC_LAYER_INTENT,                 \
                    D_DESC_SLOT_REMARK, D_TEST_DESC_SPEC_IF_DESCRIBED, 0u,     \
                    D_TEST_DESC_OPS_DESCRIPTOR),
#else
    #define D_TEST_DESC_DESCRIPTOR_UNIT
#endif
#define D_TEST_DESC_SPEC_UNITS                                                 \
    D_TEST_DESC_NAMED_UNIT                                                     \
    D_TEST_DESC_DESCRIPTOR_UNIT

static const struct dj_desc_unit d_internal_test_desc_module_units[] =
{
#if D_DESC_BUILT(LINE, STATE)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_CONDITION,
                D_TEST_DESC_SPEC_IF_EMPTY, 0u, D_TEST_DESC_OPS_EMPTY),
#endif
    D_DESC_UNIT(D_DESC_RING_NAME, D_DESC_LAYER_TYPE, D_DESC_SLOT_NOUN,
                0u, 0u, D_TEST_DESC_OPS_MODULE),
    D_TEST_DESC_SPEC_UNITS
#if D_DESC_BUILT(LINE, STATE)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_WITH,
                0u, D_TEST_DESC_SPEC_IF_EMPTY, D_TEST_DESC_OPS_BLOCKS),
#endif
#if D_DESC_BUILT(DETAILED, COST)
    D_DESC_UNIT(D_DESC_RING_DETAILED, D_DESC_LAYER_COST,
                D_DESC_SLOT_REMARK, 0u, D_TEST_DESC_SPEC_IF_EMPTY,
                D_TEST_DESC_OPS_ALL_TESTS),
#endif
};

static const struct dj_desc_unit d_internal_test_desc_block_units[] =
{
#if D_DESC_BUILT(LINE, STATE)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_CONDITION,
                D_TEST_DESC_SPEC_IF_EMPTY, 0u, D_TEST_DESC_OPS_EMPTY),
#endif
    D_DESC_UNIT(D_DESC_RING_NAME, D_DESC_LAYER_TYPE, D_DESC_SLOT_NOUN,
                0u, 0u, D_TEST_DESC_OPS_BLOCK),
    D_TEST_DESC_SPEC_UNITS
#if D_DESC_BUILT(LINE, STATE)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_WITH,
                0u, D_TEST_DESC_SPEC_IF_EMPTY, D_TEST_DESC_OPS_SPEC_TESTS),
#endif
};

static const struct dj_desc_unit d_internal_test_desc_case_units[] =
{
#if D_DESC_BUILT(LINE, STATE)
    D_DESC_UNIT(D_DESC_RING_LINE, D_DESC_LAYER_STATE, D_DESC_SLOT_CONDITION,
                D_TEST_DESC_SPEC_IF_UNBOUND, 0u, D_TEST_DESC_OPS_UNBOUND),
#endif
    D_DESC_UNIT(D_DESC_RING_NAME, D_DESC_LAYER_TYPE, D_DESC_SLOT_NOUN,
                0u, 0u, D_TEST_DESC_OPS_CASE),
    D_TEST_DESC_SPEC_UNITS
};
#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_UNITS

#if D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS
// D_TEST_DESC_SPEC_FIELDS
//   macro: the first two rows each spec's field list has.
#define D_TEST_DESC_SPEC_FIELDS                                                \
    D_DESC_FIELD("name", "named", D_DESC_LAYER_IDENTITY, 0u,                   \
                 D_DESC_TRUTH_BUILD),                                          \
    D_DESC_FIELD("descriptor", "descriptor", D_DESC_LAYER_INTENT, 1u,          \
                 D_DESC_TRUTH_BUILD)

static const struct dj_desc_field d_internal_test_desc_module_fields[] =
{
    D_TEST_DESC_SPEC_FIELDS,
    D_DESC_FIELD("block_count", "blocks", D_DESC_LAYER_STATE, 0u,
                 D_DESC_TRUTH_BUILD),
    D_DESC_FIELD("test_count", "tests", D_DESC_LAYER_COST, 2u,
                 D_DESC_TRUTH_BUILD)
};

static const struct dj_desc_field d_internal_test_desc_block_fields[] =
{
    D_TEST_DESC_SPEC_FIELDS,
    D_DESC_FIELD("test_count", "tests", D_DESC_LAYER_STATE, 0u,
                 D_DESC_TRUTH_BUILD)
};

static const struct dj_desc_field d_internal_test_desc_case_fields[] =
{
    D_TEST_DESC_SPEC_FIELDS,
    D_DESC_FIELD("runnable", "runnable", D_DESC_LAYER_STATE, 2u,
                 D_DESC_TRUTH_BUILD)
};
#endif  // D_CFG_DESC_ENABLED && D_CFG_DESC_FACTS

const struct dj_desc_type d_desc_type_dj_test_module_spec =
{
    "test_module",
    "test module",
    D_DESC_UNITS_TABLE(d_internal_test_desc_module_units),
    D_DESC_FIELDS_TABLE(d_internal_test_desc_module_fields),
    d_internal_test_desc_module_plan,
    d_internal_test_desc_module_read,
    d_internal_test_desc_module_contents
};

const struct dj_desc_type d_desc_type_dj_test_block_spec =
{
    "test_block",
    "test block",
    D_DESC_UNITS_TABLE(d_internal_test_desc_block_units),
    D_DESC_FIELDS_TABLE(d_internal_test_desc_block_fields),
    d_internal_test_desc_block_plan,
    d_internal_test_desc_block_read,
    d_internal_test_desc_block_contents
};

const struct dj_desc_type d_desc_type_dj_test_case_spec =
{
    "test_case",
    "test",
    D_DESC_UNITS_TABLE(d_internal_test_desc_case_units),
    D_DESC_FIELDS_TABLE(d_internal_test_desc_case_fields),
    d_internal_test_desc_case_plan,
    d_internal_test_desc_case_read,
    NULL
};

#endif  // D_INTERNAL_DESC_OF_TEST


//==============================================================================
// 4.  LAYOUT
//==============================================================================

/*
d_desc_of_test_layout
  DTest's layout rows as this library was built: the header's table,
compiled with the library's knobs.
*/
size_t
d_desc_of_test_layout(
    const struct dj_desc_of_layout** _rows
)
{
    return d_desc_of_test_layout_here(_rows);
}
