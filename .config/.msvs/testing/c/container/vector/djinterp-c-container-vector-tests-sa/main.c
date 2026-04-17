/******************************************************************************
* djinterp [test]                                                       main.c
*
*   Test runner for linked_node module standalone tests.
*   Tests the d_linked_node type and associated node operations.
*
*
* path:      /.config/.msvs/testing/c/container/vector/
*                djinterp-c-vector-tests-sa/main.c
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/
#include "../../../../../../../inc/c/test/test_standalone.h"
#include "../../../../../../../tests/c/container/vector/vector_tests_sa.h"


/******************************************************************************
 * IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_vector_status_items[] =
{
    { "[INFO]", "Constructor functions (new, new_default, from_array, etc.) validated" },
    { "[INFO]", "Capacity management (reserve, shrink_to_fit, ensure_capacity) working" },
    { "[INFO]", "Element manipulation (push, pop, insert, erase) tested" },
    { "[INFO]", "Append/prepend operations including vector-to-vector validated" },
    { "[INFO]", "Resize functions (resize, resize_fill) working correctly" },
    { "[INFO]", "Access functions (at, front, back, data, get, set) validated" },
    { "[INFO]", "Query functions (is_empty, is_full, size, capacity, element_size) tested" },
    { "[INFO]", "Search functions (find, find_last, contains, count_value) validated" },
    { "[INFO]", "Utility functions (swap, reverse, sort, copy_to) working" },
    { "[INFO]", "Destructor functions (free, free_deep) handle memory correctly" }
};

static const struct d_test_sa_note_item g_vector_issues_items[] =
{
    { "[NOTE]", "d_vector wraps d_vector_common functions with struct management" },
    { "[NOTE]", "All functions perform NULL checks on the vector pointer" },
    { "[NOTE]", "Negative indexing supported via d_index type" },
    { "[NOTE]", "append_vector and prepend_vector require matching element sizes" },
    { "[WARN]", "free_deep assumes element_size == sizeof(void*) for pointer elements" }
};

static const struct d_test_sa_note_item g_vector_steps_items[] =
{
    { "[TODO]", "Add stress tests for large vectors" },
    { "[TODO]", "Add memory leak detection with valgrind integration" },
    { "[TODO]", "Create performance benchmarks vs std::vector" },
    { "[TODO]", "Test edge cases with SIZE_MAX overflow scenarios" },
    { "[TODO]", "Add iterator function tests when implemented" }
};

static const struct d_test_sa_note_item g_vector_guidelines_items[] =
{
    { "[BEST]", "Always check return value of constructor functions for NULL" },
    { "[BEST]", "Use d_vector_free or d_vector_free_deep to avoid memory leaks" },
    { "[BEST]", "Call d_vector_ensure_capacity before bulk operations" },
    { "[BEST]", "Use d_vector_shrink_to_fit after removing many elements" },
    { "[BEST]", "Match element sizes when using append_vector/prepend_vector" }
};

static const struct d_test_sa_note_section g_vector_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_vector_status_items) / sizeof(g_vector_status_items[0]),
      g_vector_status_items },
    { "KNOWN ISSUES",
      sizeof(g_vector_issues_items) / sizeof(g_vector_issues_items[0]),
      g_vector_issues_items },
    { "NEXT STEPS",
      sizeof(g_vector_steps_items) / sizeof(g_vector_steps_items[0]),
      g_vector_steps_items },
    { "BEST PRACTICES",
      sizeof(g_vector_guidelines_items) / sizeof(g_vector_guidelines_items[0]),
      g_vector_guidelines_items }
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
                          "djinterp vector Module",
                          "Comprehensive Testing of d_vector Struct Wrapper "
                          "Functions and Operations");

    // register the vector module
    d_test_sa_runner_add_module_counter(&runner,
                                        "vector",
                                        "d_vector struct wrapper functions for "
                                        "constructors, capacity management, "
                                        "element manipulation, append/prepend, "
                                        "resize, access, query, search, utility, "
                                        "and destructor operations",
                                        d_tests_sa_vector_run_all,
                                        ( sizeof(g_vector_notes) /
                                            sizeof(g_vector_notes[0]) ),
                                        g_vector_notes);

    // execute all tests and return result
    return d_test_sa_runner_execute(&runner);
}