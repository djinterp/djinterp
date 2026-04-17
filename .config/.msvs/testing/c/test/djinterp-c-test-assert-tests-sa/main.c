/******************************************************************************
* djinterp [test]                                                       main.c
*
*   Test runner for test assert module standalone tests.
*   Tests the d_assert types and associated operations, including constructors,
* comparison, search, and destructors.
*
*
* path:      /.config/.msvs/testing/c/djinterp-c-test-assert-tests-sa/main.c
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/
#include "../../../../../../inc/c/test/test_standalone.h"
#include "../../../../../../tests/c/test/assert_tests_sa.h"


/******************************************************************************
 * IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_assert_status_items[] =
{
    { "[INFO]", "d_assert type creation and destruction validated" },
    { "[INFO]", "Comparison operations (eq, neq, lt, gt) working correctly" },
    { "[INFO]", "String and array assertions verified" },
    { "[INFO]", "Memory management validated for all assertion operations" },
    { "[INFO]", "Edge cases for NULL and empty inputs handled properly" }
};

static const struct d_test_sa_note_item g_assert_issues_items[] =
{
    { "[NOTE]", "This module is a dependency of DTest framework" },
    { "[NOTE]", "Uses test_standalone.h rather than DTest for testing" }
};

static const struct d_test_sa_note_item g_assert_steps_items[] =
{
    { "[TODO]", "Add floating-point comparison assertions" },
    { "[TODO]", "Add range-based assertions (between, within)" },
    { "[TODO]", "Add collection assertions (contains, all_match)" },
    { "[TODO]", "Create performance benchmarks for assertion overhead" }
};

static const struct d_test_sa_note_item g_assert_guidelines_items[] =
{
    { "[BEST]", "Always free d_assert objects when no longer needed" },
    { "[BEST]", "Use appropriate assertion function for the data type" },
    { "[BEST]", "Provide descriptive pass and fail messages" },
    { "[BEST]", "Check assertion return value before accessing result" }
};

static const struct d_test_sa_note_section g_assert_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_assert_status_items) / sizeof(g_assert_status_items[0]),
      g_assert_status_items },
    { "KNOWN ISSUES",
      sizeof(g_assert_issues_items) / sizeof(g_assert_issues_items[0]),
      g_assert_issues_items },
    { "NEXT STEPS",
      sizeof(g_assert_steps_items) / sizeof(g_assert_steps_items[0]),
      g_assert_steps_items },
    { "BEST PRACTICES",
      sizeof(g_assert_guidelines_items) / sizeof(g_assert_guidelines_items[0]),
      g_assert_guidelines_items }
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
        "djinterp assert Module",
        "Comprehensive Testing of d_assert Type and "
        "Operations");

    // register the assert module
    d_test_sa_runner_add_module(&runner,
        "assert",
        "d_assert type creation, comparison, and "
        "memory management",
        d_tests_sa_assert_all,
        sizeof(g_assert_notes) /
        sizeof(g_assert_notes[0]),
        g_assert_notes);

    // execute all tests and return result
    return d_test_sa_runner_execute(&runner);
}