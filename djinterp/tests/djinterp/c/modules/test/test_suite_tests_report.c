/*******************************************************************************
* djinterp [test]                                      test_suite_tests_report.c
*
* test_suite.h's suite, section 3: exit codes, the results table, report
* detail and options.
*
*
* path:      /tests/djinterp/c/modules/test/test_suite_tests_report.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.05
*******************************************************************************/
#include "./test_suite_tests.h"  // corresponding header

// std
#include <stdlib.h>  // free
#include <string.h>  // strcmp


/*
d_tests_test_suite_exit_codes
  The run returns 0, 1 or 2.
  Tests the following:
  - a passing run returns D_TEST_SUITE_EXIT_PASSED (0)
  - a failing one D_TEST_SUITE_EXIT_FAILED (1)
  - a NULL suite D_TEST_SUITE_EXIT_ERROR (2)
*/
bool
d_tests_test_suite_exit_codes(void)
{
    struct dj_test_suite* suite;
    FILE*                 stream;
    char*                 report;
    int                   code;

    D_TEST_REQUIRE(D_TEST_SUITE_EXIT_PASSED == 0);
    D_TEST_REQUIRE(D_TEST_SUITE_EXIT_FAILED == 1);
    D_TEST_REQUIRE(D_TEST_SUITE_EXIT_ERROR == 2);

    suite = d_tests_test_suite_quiet(D_TEST_SUITE_SHOW_TESTS, &stream);
    D_TEST_REQUIRE(suite != NULL);
    D_TEST_REQUIRE(d_test_suite_test(suite, "t"));
    D_TEST_REQUIRE(D_TEST_CHECK(suite, true));
    report = d_tests_test_suite_report(suite, stream, &code, NULL);
    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(code == D_TEST_SUITE_EXIT_PASSED);
    D_TEST_REQUIRE(d_tests_test_suite_contains(report, "result:  PASSED"));
    free(report);

    suite = d_tests_test_suite_quiet(D_TEST_SUITE_SHOW_TESTS, &stream);
    D_TEST_REQUIRE(suite != NULL);
    D_TEST_REQUIRE(d_test_suite_test(suite, "t"));
    D_TEST_REQUIRE(!D_TEST_CHECK(suite, false));
    report = d_tests_test_suite_report(suite, stream, &code, NULL);
    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(code == D_TEST_SUITE_EXIT_FAILED);
    D_TEST_REQUIRE(d_tests_test_suite_contains(report, "result:  FAILED"));
    free(report);

    D_TEST_REQUIRE(d_test_suite_run(NULL, NULL) == D_TEST_SUITE_EXIT_ERROR);

    return true;
}

/*
d_tests_test_suite_empty_is_failure
  An empty test fails, and so does a run with no test.
  Tests the following:
  - a test opened with no check is FAIL, "no checks"
  - a suite with no test at all returns D_TEST_SUITE_EXIT_FAILED
*/
bool
d_tests_test_suite_empty_is_failure(void)
{
    struct dj_test_suite*  suite;
    struct dj_test_summary summary;
    FILE*                  stream;
    char*                  report;
    int                    code;

    suite = d_tests_test_suite_quiet(D_TEST_SUITE_SHOW_TESTS, &stream);
    D_TEST_REQUIRE(suite != NULL);
    D_TEST_REQUIRE(d_test_suite_test(suite, "empty"));
    report = d_tests_test_suite_report(suite, stream, &code, &summary);
    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(summary.tests_failed == 1);
    D_TEST_REQUIRE(code == D_TEST_SUITE_EXIT_FAILED);
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "[FAIL] empty: no checks"));
    free(report);

    suite = d_tests_test_suite_quiet(D_TEST_SUITE_SHOW_TESTS, &stream);
    D_TEST_REQUIRE(suite != NULL);
    report = d_tests_test_suite_report(suite, stream, &code, NULL);
    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(code == D_TEST_SUITE_EXIT_FAILED);
    free(report);

    return true;
}

/*
d_tests_test_suite_table_format
  The results table has test_defaults.hpp's caption, columns and rule.
  Tests the following:
  - the caption names the module
  - the header row's columns, padded to the widest cell
  - the rule is a dash line as wide as the header row
  - rows number from 1 within the module
*/
bool
d_tests_test_suite_table_format(void)
{
    struct dj_test_suite* suite;
    FILE*                 stream;
    char*                 report;
    int                   code;

    suite = d_tests_test_suite_quiet(D_TEST_SUITE_SHOW_TESTS, &stream);
    D_TEST_REQUIRE(suite != NULL);
    D_TEST_REQUIRE(d_test_suite_module(suite, "mod"));
    D_TEST_REQUIRE(d_test_suite_test(suite, "alpha"));
    D_TEST_REQUIRE(d_test_suite_describe(suite, "first"));
    D_TEST_REQUIRE(D_TEST_CHECK(suite, true));
    D_TEST_REQUIRE(d_test_suite_test(suite, "b"));
    D_TEST_REQUIRE(!D_TEST_CHECK(suite, false));

    report = d_tests_test_suite_report(suite, stream, &code, NULL);
    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                       "\n|-- results table (mod) --|\n"));
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                       "| RESULT | # | TEST  | DESCRIPTION |\n"
                       "|----------------------------------|\n"
                       "| PASS   | 1 | alpha | first       |\n"
                       "| FAIL   | 2 | b     |             |\n"));
    free(report);

    return true;
}

/*
d_tests_test_suite_table_escape
  A table cell turns | into / and a newline into a space, and drops a
carriage return, as pipe_escape does.
  Tests the following:
  - a name holding all three is written escaped
*/
bool
d_tests_test_suite_table_escape(void)
{
    struct dj_test_suite* suite;
    FILE*                 stream;
    char*                 report;
    int                   code;

    suite = d_tests_test_suite_quiet(D_TEST_SUITE_SHOW_TESTS, &stream);
    D_TEST_REQUIRE(suite != NULL);
    D_TEST_REQUIRE(d_test_suite_module(suite, "m"));
    D_TEST_REQUIRE(d_test_suite_test(suite, "a|b\nc\rd"));
    D_TEST_REQUIRE(D_TEST_CHECK(suite, true));

    report = d_tests_test_suite_report(suite, stream, &code, NULL);
    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(d_tests_test_suite_contains(report, "| a/b cd |"));
    free(report);

    return true;
}

/*
d_tests_test_suite_show_levels
  Each level of detail lists what it says.
  Tests the following:
  - failures: the failing test, not the passing one
  - tests: both
  - checks: the passing check's text as well
*/
bool
d_tests_test_suite_show_levels(void)
{
    static const int levels[3] = { D_TEST_SUITE_SHOW_FAILURES,
                                   D_TEST_SUITE_SHOW_TESTS,
                                   D_TEST_SUITE_SHOW_CHECKS };
    struct dj_test_suite_options options;
    struct dj_test_suite*        suite;
    FILE*                        stream;
    char*                        report;
    int                          code;
    int                          i;

    for (i = 0; i < 3; i++)
    {
        suite = d_tests_test_suite_quiet(levels[i], &stream);
        D_TEST_REQUIRE(suite != NULL);
        d_test_suite_options_init(&options);
        options.show   = levels[i];
        options.table  = false;
        options.stream = stream;
        d_test_suite_set_options(suite, &options);
        D_TEST_REQUIRE(d_test_suite_test(suite, "passes"));
        D_TEST_REQUIRE(D_TEST_CHECK(suite, 40 + 2 == 42));
        D_TEST_REQUIRE(d_test_suite_test(suite, "fails"));
        D_TEST_REQUIRE(!D_TEST_CHECK(suite, 1 > 2));

        report = d_tests_test_suite_report(suite, stream, &code, NULL);
        D_TEST_REQUIRE(report != NULL);
        D_TEST_REQUIRE(d_tests_test_suite_contains(report, "[FAIL] fails"));
        D_TEST_REQUIRE(d_tests_test_suite_contains(report, "[PASS] passes") ==
                       (levels[i] != D_TEST_SUITE_SHOW_FAILURES));
        D_TEST_REQUIRE(d_tests_test_suite_contains(report, "40 + 2 == 42") ==
                       (levels[i] == D_TEST_SUITE_SHOW_CHECKS));
        D_TEST_REQUIRE(!d_tests_test_suite_contains(report, "results table"));
        free(report);
    }

    return true;
}

/*
d_tests_test_suite_options_defaults
  The defaults.
  Tests the following:
  - every test listed, the table on, no color, stdout, no filter
  - init accepts NULL
*/
bool
d_tests_test_suite_options_defaults(void)
{
    struct dj_test_suite_options options;

    d_test_suite_options_init(&options);
    D_TEST_REQUIRE(options.show == D_TEST_SUITE_SHOW_TESTS);
    D_TEST_REQUIRE(options.table);
    D_TEST_REQUIRE(!options.color);
    D_TEST_REQUIRE(options.stream == NULL);
    D_TEST_REQUIRE(options.filter == NULL);
    d_test_suite_options_init(NULL);

    return true;
}

/*
d_tests_test_suite_options_parse
  Each argument sets its option.
  Tests the following:
  - --failures, --checks, --no-table and --filter= each set theirs
  - an empty --filter= leaves no filter
  - --help returns 1; an unknown argument returns -1, each printing the usage
    to the options' stream
*/
bool
d_tests_test_suite_options_parse(void)
{
    struct dj_test_suite_options options;
    FILE* sink;
    char  prog[]     = "runner";
    char  failures[] = "--failures";
    char  checks[]   = "--checks";
    char  no_table[] = "--no-table";
    char  filter[]   = "--filter=dstring/split";
    char  empty[]    = "--filter=";
    char  help[]     = "--help";
    char  bogus[]    = "--bogus";
    char* argv1[5];
    char* argv2[2];
    char* argv3[2];

    argv1[0] = prog;
    argv1[1] = failures;
    argv1[2] = no_table;
    argv1[3] = filter;
    argv1[4] = NULL;
    sink     = tmpfile();
    D_TEST_REQUIRE(sink != NULL);
    d_test_suite_options_init(&options);
    options.stream = sink;  // the usage goes here, not to the console
    D_TEST_REQUIRE(d_test_suite_options_parse(&options, 4, argv1) == 0);
    D_TEST_REQUIRE(options.show == D_TEST_SUITE_SHOW_FAILURES);
    D_TEST_REQUIRE(!options.table);
    D_TEST_REQUIRE( (options.filter != NULL) &&
                    (strcmp(options.filter, "dstring/split") == 0) );

    argv1[1] = checks;
    argv1[2] = empty;
    D_TEST_REQUIRE(d_test_suite_options_parse(&options, 3, argv1) == 0);
    D_TEST_REQUIRE(options.show == D_TEST_SUITE_SHOW_CHECKS);
    D_TEST_REQUIRE(options.filter == NULL);

    // --help prints the usage to stdout and returns 1
    argv2[0] = prog;
    argv2[1] = help;
    D_TEST_REQUIRE(d_test_suite_options_parse(&options, 2, argv2) == 1);

    // a bad argument prints the usage to stderr and returns -1
    argv3[0] = prog;
    argv3[1] = bogus;
    D_TEST_REQUIRE(d_test_suite_options_parse(&options, 2, argv3) == -1);
    fclose(sink);

    return true;
}

/*
d_tests_test_suite_many_checks
  Passing checks are counted, not kept.
  Tests the following:
  - a hundred thousand checks are all counted
  - the report lists none of them at the default detail
*/
bool
d_tests_test_suite_many_checks(void)
{
    struct dj_test_suite*  suite;
    struct dj_test_summary summary;
    FILE*                  stream;
    char*                  report;
    int                    code;
    long                   i;

    suite = d_tests_test_suite_quiet(D_TEST_SUITE_SHOW_TESTS, &stream);
    D_TEST_REQUIRE(suite != NULL);
    D_TEST_REQUIRE(d_test_suite_test(suite, "many"));

    for (i = 0; i < 100000L; i++)
    {
        D_TEST_CHECK(suite, i >= 0);
    }

    report = d_tests_test_suite_report(suite, stream, &code, &summary);
    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(summary.checks == 100000u);
    D_TEST_REQUIRE(code == D_TEST_SUITE_EXIT_PASSED);
    D_TEST_REQUIRE(!d_tests_test_suite_contains(report, "i >= 0"));
    free(report);

    return true;
}

/*
d_tests_test_suite_str_equal
  The NULL-safe string equality checks are written with.
  Tests the following:
  - equal and unequal strings
  - two NULLs are equal; one NULL is not
  - an array argument is compared by content
*/
bool
d_tests_test_suite_str_equal(void)
{
    char buffer[8] = "abc";

    D_TEST_REQUIRE(d_test_suite_str_equal("abc", "abc"));
    D_TEST_REQUIRE(!d_test_suite_str_equal("abc", "abd"));
    D_TEST_REQUIRE(d_test_suite_str_equal(NULL, NULL));
    D_TEST_REQUIRE(!d_test_suite_str_equal(NULL, "abc"));
    D_TEST_REQUIRE(!d_test_suite_str_equal("abc", NULL));
    D_TEST_REQUIRE(d_test_suite_str_equal(buffer, "abc"));

    return true;
}
