/******************************************************************************
* djinterp [test]                                                       main.c
*
*   Test runner for registry_common unit tests.
*   Executes comprehensive tests for the registry_common module including
* string comparison and schema utility functions.
*
*
* path:      \tests\container\registry\main.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.05
******************************************************************************/
#include "..\..\..\..\..\..\inc\test\test_standalone.h"
#include "..\..\..\..\..\..\tests\container\registry\registry_common_tests_sa.h"


/******************************************************************************
 * IMPLEMENTATION NOTES
 *****************************************************************************/

// current status notes
static const struct d_test_sa_note_item g_status_notes[] =
{
    { "[INFO]", "All `d_registry_strcmp` functions tested" },
    { "[INFO]", "All `d_registry_schema_max_enum_key` functions tested" },
    { "[INFO]", "NULL handling verified for all functions" },
    { "[INFO]", "Edge cases including empty strings and UINT16_MAX tested" }
};

// known issues notes
static const struct d_test_sa_note_item g_issues_notes[] =
{
    { "[NOTE]", "ASCII-only case insensitivity (locale-independent)" },
    { "[NOTE]", "Schema aliases share enum_key values by design" }
};

// test coverage notes
static const struct d_test_sa_note_item g_coverage_notes[] =
{
    { "[PASS]", "d_registry_strcmp: 14 test functions" },
    { "[PASS]", "d_registry_schema_max_enum_key: 11 test functions" },
    { "[PASS]", "100% code coverage of registry_common.c" }
};

// note sections array
static const struct d_test_sa_note_section g_note_sections[] =
{
    { "CURRENT STATUS", 4, g_status_notes },
    { "KNOWN ISSUES",   2, g_issues_notes },
    { "TEST COVERAGE",  3, g_coverage_notes }
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

    // suppress unused parameter warnings
    (void)_argc;
    (void)_argv;

    // initialize the test runner
    d_test_sa_runner_init(&runner,
                          "registry_common",
                          "Unit tests for registry_common module");

    // register the registry_common test module
    d_test_sa_runner_add_module_counter(&runner,
                                        "registry_common",
                                        "String comparison and schema utilities",
                                        d_tests_sa_registry_common_run_all,
                                        G_NOTE_SECTION_COUNT,
                                        g_note_sections);

    // configure runner options
    d_test_sa_runner_set_wait_for_input(&runner, true);
    d_test_sa_runner_set_show_notes(&runner, true);

    // execute all tests and return result
    return d_test_sa_runner_execute(&runner);
}