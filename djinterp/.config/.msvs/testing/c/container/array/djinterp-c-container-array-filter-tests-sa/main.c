/******************************************************************************
* djinterp [test]                                                       main.c
*
*   Test runner for array_filter unit tests.
*   Executes comprehensive tests for the array_filter module including
* result validation, single-operation filters, in-place operations,
* chain and combinator application, query functions, result management,
* convenience macros, and fluent builder helpers.
*
*
* path:      .config/.msvs/testing/c/container/array/
*                djinterp-c-container-array-filter-tests-sa/main.c
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/
#include "../../../../../../../inc/c/test/test_standalone.h"
#include "../../../../../../../tests/c/container/array/array_filter_tests_sa.h"


/******************************************************************************
 * IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_status_notes[] =
{
    { "[INFO]", "All single-operation filter functions tested (16 functions)" },
    { "[INFO]", "All in-place filter operations tested (5 functions)" },
    { "[INFO]", "All chain and combinator functions tested (4 functions)" },
    { "[INFO]", "All query functions tested (6 functions)" },
    { "[INFO]", "All result management functions tested (5 functions)" },
    { "[INFO]", "NULL handling and edge cases verified for all functions" }
};

static const struct d_test_sa_note_item g_issues_notes[] =
{
    { "[NOTE]", "O(n^2) distinct; suitable for moderate data sizes" },
    { "[NOTE]", "Combinator union does not deduplicate overlapping results" },
    { "[NOTE]", "source_indices tracking is optional (may be NULL)" }
};

static const struct d_test_sa_note_item g_coverage_notes[] =
{
    { "[PASS]", "Result struct and enum validation: 2 test functions" },
    { "[PASS]", "Single-operation filters: 16 test functions" },
    { "[PASS]", "In-place operations: 5 test functions" },
    { "[PASS]", "Chain and combinator application: 4 test functions" },
    { "[PASS]", "Query functions: 6 test functions" },
    { "[PASS]", "Result management: 5 test functions" },
    { "[PASS]", "Convenience macros: 8 test functions" },
    { "[PASS]", "Fluent builder helpers: 3 test functions" },
    { "[PASS]", "49 test functions across 8 sections" }
};

static const struct d_test_sa_note_section g_note_sections[] =
{
    { "CURRENT STATUS", 6, g_status_notes },
    { "KNOWN ISSUES",   3, g_issues_notes },
    { "TEST COVERAGE",  9, g_coverage_notes }
};

#define G_NOTE_SECTION_COUNT (sizeof(g_note_sections) / sizeof(g_note_sections[0]))


/******************************************************************************
 * MAIN ENTRY POINT
 *****************************************************************************/

int
main
(
    int    _argc,
    char** _argv
)
{
    struct d_test_sa_runner runner;

    (void)_argc;
    (void)_argv;

    d_test_sa_runner_init(&runner,
                          "array_filter",
                          "Unit tests for array_filter module");

    d_test_sa_runner_add_module_counter(&runner,
                                        "array_filter",
                                        "Filter operations for contiguous data",
                                        d_tests_sa_array_filter_run_all,
                                        G_NOTE_SECTION_COUNT,
                                        g_note_sections);

    d_test_sa_runner_set_wait_for_input(&runner, true);
    d_test_sa_runner_set_show_notes(&runner, true);

    return d_test_sa_runner_execute(&runner);
}