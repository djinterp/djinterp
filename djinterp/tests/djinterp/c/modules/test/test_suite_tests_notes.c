/*******************************************************************************
* djinterp [c][test]                                    test_suite_tests_notes.c
*
* test_suite.h's notes: where a note attaches, when the report prints it, and
* how it reads.
*
*
* path:      /tests/djinterp/c/modules/test/test_suite_tests_notes.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.07
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "./test_suite_tests.h"  // corresponding header

// std
#include <stdarg.h>  // va_list, va_start, va_end
#include <stdlib.h>  // free
#include <string.h>  // memset, strstr


//==============================================================================
// 4.  NOTES
//==============================================================================

/*
d_tests_test_suite_note_via
  Passes its arguments on to d_test_suite_vnote, as a helper with a format of
its own would.
*/
static bool
d_tests_test_suite_note_via(
    struct dj_test_suite* _suite,
    const char*           _format,
    ...
)
{
    va_list args;

    va_start(args,
             _format);

    const bool recorded = d_test_suite_vnote(_suite,
                                             _format,
                                             args);

    va_end(args);

    return recorded;
}

/*
d_tests_test_suite_note_attaches
  A note goes to the node opened last, and nowhere without one.
  Tests the following:
  - refused with no node open, a NULL suite, or a NULL format
  - accepted on a module, a block and a test, each at its depth
  - a note is not a check: a test with only a note has no checks
*/
bool
d_tests_test_suite_note_attaches(void)
{
    const char* none   = NULL;
    FILE*       stream = NULL;

    struct dj_test_suite* suite =
        d_tests_test_suite_quiet(D_TEST_SUITE_SHOW_CHECKS,
                                 &stream);

    D_TEST_REQUIRE(suite != NULL);
    D_TEST_REQUIRE(!d_test_suite_note(suite,
                                      "nothing is open"));
    D_TEST_REQUIRE(!d_test_suite_note(NULL,
                                      "no suite"));
    D_TEST_REQUIRE(d_test_suite_module(suite,
                                       "m"));
    D_TEST_REQUIRE(!d_test_suite_note(suite,
                                      none,
                                      0));
    D_TEST_REQUIRE(d_test_suite_note(suite,
                                     "on the module"));
    D_TEST_REQUIRE(d_test_suite_block(suite,
                                      "b"));
    D_TEST_REQUIRE(d_test_suite_note(suite,
                                     "on the block"));
    D_TEST_REQUIRE(d_test_suite_test(suite,
                                     "only_a_note"));
    D_TEST_REQUIRE(d_test_suite_note(suite,
                                     "on the test"));

    struct dj_test_summary summary = {0};
    int                    code    = 0;

    char* report = d_tests_test_suite_report(suite,
                                             stream,
                                             &code,
                                             &summary);

    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(summary.checks == 0);
    D_TEST_REQUIRE(code == D_TEST_SUITE_EXIT_FAILED);
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "[FAIL] only_a_note: no "
                                               "checks"));
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "\n  note on the module\n"));
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "\n    note on the block\n"));
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "\n          note on the "
                                               "test\n"));
    D_TEST_REQUIRE(!d_tests_test_suite_contains(report,
                                                "nothing is open"));
    D_TEST_REQUIRE(!d_tests_test_suite_contains(report,
                                                "no suite"));
    free(report);

    return true;
}

/*
d_tests_test_suite_note_test_shown
  A test's note prints when the test fails, or at the checks level.
  Tests the following:
  - failures and tests: the failing test's note, not the passing one's
  - checks: both
*/
bool
d_tests_test_suite_note_test_shown(void)
{
    static const int levels[3] = { D_TEST_SUITE_SHOW_FAILURES,
                                   D_TEST_SUITE_SHOW_TESTS,
                                   D_TEST_SUITE_SHOW_CHECKS };

    // once at each level of detail
    for (int i = 0; i < 3; i++)
    {
        FILE* stream = NULL;

        struct dj_test_suite* const suite = d_tests_test_suite_quiet(levels[i],
                                                                     &stream);

        D_TEST_REQUIRE(suite != NULL);
        D_TEST_REQUIRE(d_test_suite_test(suite,
                                         "passes"));
        D_TEST_REQUIRE(D_TEST_CHECK(suite,
                                    1 + 1 == 2));
        D_TEST_REQUIRE(d_test_suite_note(suite,
                                         "the passing note"));
        D_TEST_REQUIRE(d_test_suite_test(suite,
                                         "fails"));

        // a failing check, and a note on its value
        if (!D_TEST_CHECK(suite,
                          1 + 2 == 4))
        {
            D_TEST_REQUIRE(d_test_suite_note(suite,
                                             "got %d",
                                             1 + 2));
        }

        int code = 0;

        char* const report = d_tests_test_suite_report(suite,
                                                       stream,
                                                       &code,
                                                       NULL);

        D_TEST_REQUIRE(report != NULL);
        D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                                   "note got 3\n"));
        D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                                   "the passing note") ==
                       (levels[i] == D_TEST_SUITE_SHOW_CHECKS));
        free(report);
    }

    return true;
}

/*
d_tests_test_suite_note_heading_shown
  A module's or block's note prints when a test beneath it fails, or at the
checks level.
  Tests the following:
  - a module whose tests pass keeps its note out, at failures and tests
  - a module with a failing test prints its note, and its block's
  - checks: every heading's note
*/
bool
d_tests_test_suite_note_heading_shown(void)
{
    static const int levels[3] = { D_TEST_SUITE_SHOW_FAILURES,
                                   D_TEST_SUITE_SHOW_TESTS,
                                   D_TEST_SUITE_SHOW_CHECKS };

    // once at each level of detail
    for (int i = 0; i < 3; i++)
    {
        FILE* stream = NULL;

        struct dj_test_suite* const suite = d_tests_test_suite_quiet(levels[i],
                                                                     &stream);

        D_TEST_REQUIRE(suite != NULL);
        D_TEST_REQUIRE(d_test_suite_module(suite,
                                           "quiet"));
        D_TEST_REQUIRE(d_test_suite_note(suite,
                                         "the quiet profile"));
        D_TEST_REQUIRE(d_test_suite_test(suite,
                                         "holds"));
        D_TEST_REQUIRE(D_TEST_CHECK(suite,
                                    2 > 1));
        D_TEST_REQUIRE(d_test_suite_module(suite,
                                           "loud"));
        D_TEST_REQUIRE(d_test_suite_note(suite,
                                         "the loud profile"));
        D_TEST_REQUIRE(d_test_suite_block(suite,
                                          "blk"));
        D_TEST_REQUIRE(d_test_suite_note(suite,
                                         "the block's note"));
        D_TEST_REQUIRE(d_test_suite_test(suite,
                                         "breaks"));
        D_TEST_REQUIRE(!D_TEST_CHECK(suite,
                                     1 > 2));

        int code = 0;

        char* const report = d_tests_test_suite_report(suite,
                                                       stream,
                                                       &code,
                                                       NULL);

        D_TEST_REQUIRE(report != NULL);
        D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                                   "  note the loud "
                                                   "profile\n"));
        D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                                   "    note the block's "
                                                   "note\n"));
        D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                                   "the quiet profile") ==
                       (levels[i] == D_TEST_SUITE_SHOW_CHECKS));
        free(report);
    }

    return true;
}

/*
d_tests_test_suite_note_layout
  A note reads in place.
  Tests the following:
  - a note prints among its test's checks, in the order they were made
  - a note of several lines keeps them, each after the first indented under
    its text, and its final newline opens no empty line
*/
bool
d_tests_test_suite_note_layout(void)
{
    FILE* stream = NULL;

    struct dj_test_suite* suite =
        d_tests_test_suite_quiet(D_TEST_SUITE_SHOW_CHECKS,
                                 &stream);

    D_TEST_REQUIRE(suite != NULL);
    D_TEST_REQUIRE(d_test_suite_test(suite,
                                     "ordered"));
    D_TEST_REQUIRE(D_TEST_CHECK(suite,
                                10 == 10));
    D_TEST_REQUIRE(d_test_suite_note(suite,
                                     "between the two"));
    D_TEST_REQUIRE(D_TEST_CHECK(suite,
                                20 == 20));
    D_TEST_REQUIRE(d_test_suite_test(suite,
                                     "lines"));
    D_TEST_REQUIRE(D_TEST_CHECK(suite,
                                30 == 30));
    D_TEST_REQUIRE(d_test_suite_note(suite,
                                     "first line\nsecond line\n"));

    int code = 0;

    char* report = d_tests_test_suite_report(suite,
                                             stream,
                                             &code,
                                             NULL);

    D_TEST_REQUIRE(report != NULL);

    const char* first  = strstr(report,
                                "10 == 10");
    const char* note   = strstr(report,
                                "note between the two");
    const char* second = strstr(report,
                                "20 == 20");

    D_TEST_REQUIRE( (first != NULL) &&
                    (note != NULL)  &&
                    (second != NULL) );
    D_TEST_REQUIRE( (first < note) &&
                    (note < second) );
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "          note first line\n"
                                               "               second line"
                                               "\n"));
    D_TEST_REQUIRE(!d_tests_test_suite_contains(report,
                                                "second line\n"
                                                "               \n"));
    free(report);

    return true;
}

/*
d_tests_test_suite_note_format
  A note is formatted as printf formats.
  Tests the following:
  - its arguments, of several kinds
  - a note longer than any fixed buffer, kept whole
  - d_test_suite_vnote, given a helper's va_list
*/
bool
d_tests_test_suite_note_format(void)
{
    char wide[3001] = {0};

    memset(wide,
           'w',
           sizeof(wide) - 1);
    wide[sizeof(wide) - 1] = '\0';
    wide[sizeof(wide) - 2] = '!';

    FILE* stream = NULL;

    struct dj_test_suite* suite =
        d_tests_test_suite_quiet(D_TEST_SUITE_SHOW_CHECKS,
                                 &stream);

    D_TEST_REQUIRE(suite != NULL);
    D_TEST_REQUIRE(d_test_suite_test(suite,
                                     "formats"));
    D_TEST_REQUIRE(D_TEST_CHECK(suite,
                                1));
    D_TEST_REQUIRE(d_test_suite_note(suite,
                                     "%d-%s-%05.1f-%lu",
                                     7,
                                     "x",
                                     2.5,
                                     (unsigned long)40000));
    D_TEST_REQUIRE(d_test_suite_note(suite,
                                     "[%s]",
                                     wide));
    D_TEST_REQUIRE(d_tests_test_suite_note_via(suite,
                                               "via %s %d",
                                               "va",
                                               9));

    int code = 0;

    char* report = d_tests_test_suite_report(suite,
                                             stream,
                                             &code,
                                             NULL);

    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "note 7-x-002.5-40000\n"));
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               wide));
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "w!]\n"));
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "note via va 9\n"));
    free(report);

    return true;
}

/*
d_tests_test_suite_note_spec_case
  The inner spec test of d_tests_test_suite_note_spec: notes, then fails.
*/
static bool
d_tests_test_suite_note_spec_case(void)
{
    D_TEST_REQUIRE(D_TEST_NOTE("from the spec test, %d",
                               42));
    D_TEST_REQUIRE(1 + 1 == 3);

    return true;
}

/*
d_tests_test_suite_note_spec
  D_TEST_NOTE in a spec test.
  Tests the following:
  - the note goes to the spec test that made it, in the suite being run
  - it prints beneath that test when the test fails
*/
bool
d_tests_test_suite_note_spec(void)
{
    static const struct dj_test_case_spec  rows[1] =
    {
        { "noted", "notes, then fails", d_tests_test_suite_note_spec_case }
    };

    static const struct dj_test_block_spec blocks[1] =
    {
        { "inner", "one test", rows, 1 }
    };

    static const struct dj_test_module_spec module =
    {
        "inner_module", "a spec whose test notes", blocks, 1
    };

    FILE* stream = NULL;

    struct dj_test_suite* suite =
        d_tests_test_suite_quiet(D_TEST_SUITE_SHOW_TESTS,
                                 &stream);

    D_TEST_REQUIRE(suite != NULL);
    D_TEST_REQUIRE(d_test_suite_add_module(suite,
                                           &module));

    struct dj_test_summary summary = {0};
    int                    code    = 0;

    char* report = d_tests_test_suite_report(suite,
                                             stream,
                                             &code,
                                             &summary);

    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(summary.tests_failed == 1);
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "[FAIL] noted\n"
                                               "          note from the spec "
                                               "test, 42\n"));
    free(report);

    return true;
}
