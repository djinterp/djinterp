/*******************************************************************************
* djinterp [c][desc]                                        desc_of_test_tests.c
*
*   The suite of desc's describers of DTest's C types (desc/of/test_desc.h),
* in DTest's C face: each type against its describer's record of it; then
* a run's summary and options, and a module's, a block's and a test's
* spec, each in facts and in each ring, a spec's contents included.
* Expectations follow the knobs a build has.
*
*
* path:      /tests/djinterp/c/desc/desc_of_test_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

#include "./desc_of_tests.h"  // the describers' tests' helpers

// std
#include <string.h>  // memset
// djinterp
#include "../../../../inc/djinterp/c/desc/of/test_desc.h"  // the describers


//==============================================================================
// THE TESTS
//==============================================================================

/*
d_tests_desc_of_test_layout
  Checks the following:
  - each type against its record, and the describers' build against this
    program's, as d_tests_of_layout does
*/
static void
d_tests_desc_of_test_layout(
    struct dj_test_suite* _suite
)
{
    const struct dj_desc_of_layout* rows  = NULL;
    size_t count = d_desc_of_test_layout_here(&rows);
    size_t index = count;
    int    agree = d_desc_of_test_agrees(&index);

    d_tests_of_layout(_suite, "DTest", D_INTERNAL_DESC_OF_TEST, rows, count,
                      agree, index);

    return;
}

#if D_INTERNAL_DESC_OF_TEST

/*
d_tests_of_pass, d_tests_of_skip
  A spec's two tests: one bound, one left unbound.
*/
static bool
d_tests_of_pass(void)
{
    return true;
}

static const struct dj_test_case_spec d_tests_of_cases[] =
{
    { "d_tests_one", "#1: one holds", d_tests_of_pass },
    { "d_tests_two", "", NULL }
};

static const struct dj_test_block_spec d_tests_of_blocks[] =
{
    { "register", "the register", d_tests_of_cases, 2u }
};

static const struct dj_test_module_spec d_tests_of_module =
{
    "dtime", "dtime's suite", d_tests_of_blocks, 1u
};

/*
d_tests_desc_of_test_summary
  Checks the following:
  - a summary's facts, the tests passed derived from the failed and the
    skipped
  - passing, failing and empty in prose, with the failures and skips in
    the sentence and the modules in the detail
*/
static void
d_tests_desc_of_test_summary(
    struct dj_test_suite* _suite
)
{
    struct dj_test_summary  summary;
    struct dj_tests_of_keep keep;
    char                    text[256];

    (void)d_test_suite_test(_suite, "a run's summary");
    memset(&summary, 0, sizeof(summary));
    summary.modules = 1u;
    summary.tests   = 30u;
    summary.checks  = 275u;
#if D_TESTS_OF_FULL_BUILD
    (void)d_tests_of_say(D_DESC_OF(dj_test_summary), &summary,
                         D_DESC_RING_DETAILED, text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "a passing run", text,
                    D_TESTS_OF_A "passing test summary with 30 tests and 275 "
                    "checks. It opened 1 module.");
#endif

    summary.tests_failed  = 3u;
    summary.checks_failed = 4u;
    summary.tests_skipped = 1u;
    D_TESTS_FACT(_suite, (d_tests_of_facts(D_DESC_OF(dj_test_summary),
                                           &summary, &keep) == 0) &&
                         d_tests_of_flag(&keep, "passed", 0u) &&
                         d_tests_of_is(&keep, "tests_passed", 0u, 26u) &&
                         d_tests_of_is(&keep, "checks_failed", 0u, 4u));
#if D_TESTS_OF_FULL_BUILD
    (void)d_tests_of_say(D_DESC_OF(dj_test_summary), &summary,
                         D_DESC_RING_SENTENCE, text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "a failing run", text,
                    D_TESTS_OF_A "failing test summary with 30 tests and 275 "
                    "checks. Its failures: 3 tests and 4 checks. It skipped "
                    "1 test.");
    memset(&summary, 0, sizeof(summary));
    (void)d_tests_of_say(D_DESC_OF(dj_test_summary), &summary,
                         D_DESC_RING_DETAILED, text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "an empty run", text,
                    D_TESTS_OF_AN "empty test summary");
#else
    (void)text;
#endif

    return;
}

/*
d_tests_desc_of_test_options
  Checks the following:
  - the defaults: every test listed, the table on, stdout, no filter
  - a filter is the line's; a missing table and stderr are said
*/
static void
d_tests_desc_of_test_options(
    struct dj_test_suite* _suite
)
{
    struct dj_test_suite_options options;
    struct dj_tests_of_keep      keep;
    char                         text[320];

    (void)d_test_suite_test(_suite, "a run's options");
    d_test_suite_options_init(&options);
    D_TESTS_FACT(_suite, (d_tests_of_facts(D_DESC_OF(dj_test_suite_options),
                                           &options, &keep) == 0) &&
                         d_tests_of_text(&keep, "show", "tests") &&
                         d_tests_of_flag(&keep, "table", 1u) &&
                         d_tests_of_text(&keep, "filter", NULL) &&
                         d_tests_of_text(&keep, "stream", "stdout"));
#if D_TESTS_OF_FULL_BUILD
    (void)d_tests_of_say(D_DESC_OF(dj_test_suite_options), &options,
                         D_DESC_RING_DETAILED, text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "the defaults", text,
                    D_TESTS_OF_A "test run's options. It lists every test. "
                    "It writes to stdout.");
    options.filter = "desc";
    options.table  = false;
    options.stream = stderr;
    (void)d_tests_of_say(D_DESC_OF(dj_test_suite_options), &options,
                         D_DESC_RING_DETAILED, text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "a filter, no table, stderr", text,
                    D_TESTS_OF_A "test run's options for the tests whose "
                    "path holds desc. It lists every test. It prints no "
                    "results table. It writes to stderr.");
#else
    (void)text;
#endif

    return;
}

/*
d_tests_desc_of_test_specs
  Checks the following:
  - a module's facts count its blocks and, across them, its tests; its
    contents are each block, keyed by its name, with the block's tests'
    names a level further down
  - a module's, a block's and a test's prose; a test without its
    function is unbound
*/
static void
d_tests_desc_of_test_specs(
    struct dj_test_suite* _suite
)
{
    struct dj_tests_of_keep    keep;
    const struct dj_desc_fact* fact;
    char                       text[256];

    (void)d_test_suite_test(_suite, "a module's, a block's and a test's spec");
    D_TESTS_FACT(_suite, (d_tests_of_facts(D_DESC_OF(dj_test_module_spec),
                                           &d_tests_of_module, &keep) == 0) &&
                         d_tests_of_text(&keep, "name", "dtime") &&
                         d_tests_of_is(&keep, "block_count", 0u, 1u) &&
                         d_tests_of_is(&keep, "test_count", 0u, 2u) &&
                         d_tests_of_is(&keep, "test_count", 1u, 2u));
    fact = d_tests_of_find(&keep, "test", 2u);
    D_TESTS_FACT(_suite, (fact != NULL) && (fact->value.text != NULL) &&
                         (strcmp(fact->value.text, "d_tests_one") == 0) &&
                         (keep.enters == 2u));
#if D_TESTS_OF_FULL_BUILD
    (void)d_tests_of_say(D_DESC_OF(dj_test_module_spec), &d_tests_of_module,
                         D_DESC_RING_DETAILED, text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "a module", text,
                    D_TESTS_OF_A "test module named dtime, with 1 block. Its "
                    "descriptor is \"dtime's suite\". Its blocks hold 2 "
                    "tests.");
    (void)d_tests_of_say(D_DESC_OF(dj_test_block_spec), &d_tests_of_blocks[0],
                         D_DESC_RING_LINE, text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "a block", text,
                    D_TESTS_OF_A "test block named register, with 2 tests");
    (void)d_tests_of_say(D_DESC_OF(dj_test_case_spec), &d_tests_of_cases[0],
                         D_DESC_RING_SENTENCE, text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "a test", text,
                    D_TESTS_OF_A "test named d_tests_one. Its descriptor is "
                    "\"#1: one holds\".");
    (void)d_tests_of_say(D_DESC_OF(dj_test_case_spec), &d_tests_of_cases[1],
                         D_DESC_RING_SENTENCE, text, sizeof(text));
    D_TESTS_OF_SAYS(_suite, "an unbound test", text,
                    D_TESTS_OF_AN "unbound test named d_tests_two");
#else
    (void)text;
#endif

    return;
}

/*
d_tests_desc_of_test_declare
  Checks the following:
  - each type's name is an identifier, so it declares to desc
*/
static void
d_tests_desc_of_test_declare(
    struct dj_test_suite* _suite
)
{
    (void)d_test_suite_test(_suite, "each type declares by its name");
    D_TEST_CHECK(_suite,
                 (d_desc_declare(D_DESC_OF(dj_test_summary)) == 0)       &&
                 (d_desc_declare(D_DESC_OF(dj_test_suite_options)) == 0) &&
                 (d_desc_declare(D_DESC_OF(dj_test_module_spec)) == 0)   &&
                 (d_desc_declare(D_DESC_OF(dj_test_block_spec)) == 0)    &&
                 (d_desc_declare(D_DESC_OF(dj_test_case_spec)) == 0));

    return;
}

#endif  // D_INTERNAL_DESC_OF_TEST


/*
d_tests_desc_of_test
  The suite's sixth section: the block of desc/of/test_desc.h, in the
module d_tests_desc opened.
*/
void
d_tests_desc_of_test(
    struct dj_test_suite* _suite
)
{
    (void)d_test_suite_block(_suite, "desc_of_test");

    d_tests_desc_of_test_layout(_suite);
#if D_INTERNAL_DESC_OF_TEST
    d_tests_desc_of_test_summary(_suite);
    d_tests_desc_of_test_options(_suite);
    d_tests_desc_of_test_specs(_suite);
    d_tests_desc_of_test_declare(_suite);
#endif

    return;
}
