/******************************************************************************
* djinterp [test]                                                       main.c
*
*   Test runner for test_standalone module standalone tests.
*   Tests the test_standalone framework itself including assertion macros,
* constants, test counters, test objects, results structures, note structures,
* module entries, runner structures, function pointer types, the assertion
* function, template substitution, runner functions, and utility functions.
*
*
* path:      \.config\.msvs\testing\test\djinterp-c-test-standalone-tests-sa\main.c
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/
#include "..\..\..\..\..\..\inc\c\test\test_standalone.h"
#include "..\..\..\..\..\..\tests\c\test\test_standalone_tests_sa.h"


/******************************************************************************
 * IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_test_standalone_status_items[] =
{
    { "[INFO]", "Assertion macros (TRUE, FALSE, NULL, NOT_NULL, EQUAL, STR_EQUAL) validated" },
    { "[INFO]", "Object constants (LEAF, INTERIOR) and formatting constants verified"        },
    { "[INFO]", "Test counter struct and operations (reset, add) tested"                     },
    { "[INFO]", "Test object creation (new_leaf, new_interior), add_child, and free tested"  },
    { "[INFO]", "Results structures (module_results, suite_results) validated" },
    { "[INFO]", "Note structures (note_item, note_section) validated" },
    { "[INFO]", "Module entry and runner structures tested" },
    { "[INFO]", "Function pointer types (fn_print_object, fn_print_object_file) tested" },
    { "[INFO]", "d_assert_standalone function behavior verified" },
    { "[INFO]", "Template substitution with various scenarios tested" },
    { "[INFO]", "Runner functions (init, add_module, set_wait, set_notes, cleanup) tested" },
    { "[INFO]", "Utility functions (get_elapsed_time) validated" }
};

static const struct d_test_sa_note_item g_test_standalone_issues_items[] =
{
    { "[NOTE]", "D_ASSERT_STR_EQUAL uses strcmp() - ensure strings are valid" },
    { "[NOTE]", "d_test_object_free recursively frees all children" },
    { "[NOTE]", "Runner defaults: wait_for_input=true, show_notes=true" },
    { "[NOTE]", "D_TEST_SA_MAX_MODULES limits total registered modules to 64" },
    { "[WARN]", "Template substitution allocates memory - caller must free" }
};

static const struct d_test_sa_note_item g_test_standalone_steps_items[] =
{
    { "[TODO]", "Add tests for d_test_sa_runner_execute (complex integration)" },
    { "[TODO]", "Add tests for output formatting functions" },
    { "[TODO]", "Add tests for d_test_standalone_output_console" },
    { "[TODO]", "Add tests for d_test_standalone_output_file" },
    { "[TODO]", "Add tests for d_test_sa_print_timestamp" }
};

static const struct d_test_sa_note_item g_test_standalone_guidelines_items[] =
{
    { "[BEST]", "Use D_ASSERT_* macros for tree-based test objects" },
    { "[BEST]", "Use d_assert_standalone for counter-based tests" },
    { "[BEST]", "Always free test objects with d_test_object_free" },
    { "[BEST]", "Initialize runners with d_test_sa_runner_init before use" },
    { "[BEST]", "Call d_test_sa_runner_cleanup after execution" }
};

static const struct d_test_sa_note_section g_test_standalone_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_test_standalone_status_items) / sizeof(g_test_standalone_status_items[0]),
      g_test_standalone_status_items },
    { "KNOWN ISSUES",
      sizeof(g_test_standalone_issues_items) / sizeof(g_test_standalone_issues_items[0]),
      g_test_standalone_issues_items },
    { "NEXT STEPS",
      sizeof(g_test_standalone_steps_items) / sizeof(g_test_standalone_steps_items[0]),
      g_test_standalone_steps_items },
    { "BEST PRACTICES",
      sizeof(g_test_standalone_guidelines_items) / sizeof(g_test_standalone_guidelines_items[0]),
      g_test_standalone_guidelines_items }
};


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
                          "djinterp test_standalone Module",
                          "Comprehensive Testing of test_standalone Framework "
                          "Components and Functions");

    // register the test_standalone module
    d_test_sa_runner_add_module_counter(&runner,
                                        "test_standalone",
                                        "test_standalone framework including "
                                        "assertion macros, constants, counter "
                                        "operations, test objects, results and "
                                        "note structures, runner structures and "
                                        "functions, function pointers, template "
                                        "substitution, and utility functions",
                                        d_tests_sa_standalone_run_all,
                                        ( sizeof(g_test_standalone_notes) /
                                            sizeof(g_test_standalone_notes[0]) ),
                                        g_test_standalone_notes);

    // execute all tests and return result
    return d_test_sa_runner_execute(&runner);
}