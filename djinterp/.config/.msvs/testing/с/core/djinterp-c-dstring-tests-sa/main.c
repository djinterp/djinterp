/******************************************************************************
* djinterp [test]                                                       main.c
*
*   Test runner for dstring module standalone tests.
*   Tests the d_string type and associated string operations.
*
*
* path:      \.config\.msvs\testing\core\djinterp-c-dstring-tests-sa\main.c
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/

#include "..\..\..\..\..\inc\c\test\test_standalone.h"
#include "..\..\..\..\..\tests\c\dstring_tests_sa.h"


/******************************************************************************
 * IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_dstring_status_items[] =
{
    { "[INFO]", "d_string type creation and destruction validated" },
    { "[INFO]", "String operations (copy, concat, compare) working correctly" },
    { "[INFO]", "Memory management verified for all string operations" },
    { "[INFO]", "Edge cases for empty and NULL strings handled properly" }
};

static const struct d_test_sa_note_item g_dstring_issues_items[] =
{
    { "[WARN]", "Large string operations may require optimization" },
    { "[NOTE]", "Unicode support is currently limited to ASCII" }
};

static const struct d_test_sa_note_item g_dstring_steps_items[] =
{
    { "[TODO]", "Add Unicode/UTF-8 string support" },
    { "[TODO]", "Implement string builder functionality" },
    { "[TODO]", "Add regular expression support" },
    { "[TODO]", "Create benchmarks for performance comparison" }
};

static const struct d_test_sa_note_item g_dstring_guidelines_items[] =
{
    { "[BEST]", "Always check return values from d_string functions" },
    { "[BEST]", "Free d_string objects when no longer needed" },
    { "[BEST]", "Use d_string_new_copy for safe string duplication" }
};

static const struct d_test_sa_note_section g_dstring_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_dstring_status_items) / sizeof(g_dstring_status_items[0]),
      g_dstring_status_items },
    { "KNOWN ISSUES",
      sizeof(g_dstring_issues_items) / sizeof(g_dstring_issues_items[0]),
      g_dstring_issues_items },
    { "NEXT STEPS",
      sizeof(g_dstring_steps_items) / sizeof(g_dstring_steps_items[0]),
      g_dstring_steps_items },
    { "BEST PRACTICES",
      sizeof(g_dstring_guidelines_items) / sizeof(g_dstring_guidelines_items[0]),
      g_dstring_guidelines_items }
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
                          "djinterp dstring Module",
                          "Comprehensive Testing of d_string Type and "
                          "Operations");

    // register the dstring module
    d_test_sa_runner_add_module(&runner,
                                "dstring",
                                "d_string type creation, manipulation, and "
                                "memory management",
                                d_tests_sa_dstring_all,
                                sizeof(g_dstring_notes) /
                                    sizeof(g_dstring_notes[0]),
                                g_dstring_notes);

    // execute all tests and return result
    return d_test_sa_runner_execute(&runner);
}