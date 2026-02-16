/******************************************************************************
* djinterp [test]                                                       main.c
*
*   Test runner for `min_enum_map.h` module standalone tests.
*   Tests the `d_min_enum_map` type and associated map operations.
*
*
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/

#include "..\..\..\..\..\..\inc\c\test\test_standalone.h"
#include "..\..\..\..\..\..\tests\c\container\map\min_enum_map_tests_sa.h"


/******************************************************************************
 * IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_min_enum_map_status_items[] =
{
    { "[INFO]", "d_min_enum_map creation and destruction validated" },
    { "[INFO]", "Map operations (put, get, remove, contains) working correctly" },
    { "[INFO]", "Binary search maintains O(log n) lookup performance" },
    { "[INFO]", "Sorted order maintained through all operations" },
    { "[INFO]", "Merge operation with overwrite policies validated" },
    { "[INFO]", "Entry macros provide convenient initialization" }
};

static const struct d_test_sa_note_item g_min_enum_map_issues_items[] =
{
    { "[NOTE]", "d_min_enum_map_free does not free value pointers" },
    { "[NOTE]", "Caller must manage value memory separately" },
    { "[NOTE]", "Merge operation allocates new array (not in-place)" },
    { "[WARN]", "INT_MIN reserved as sentinel value for entry arrays" }
};

static const struct d_test_sa_note_item g_min_enum_map_steps_items[] =
{
    { "[TODO]", "Add iterator interface for traversal" },
    { "[TODO]", "Implement bulk insert from array" },
    { "[TODO]", "Add foreach callback function" },
    { "[TODO]", "Create benchmarks vs hash maps" },
    { "[TODO]", "Consider optimization for small maps (<8 entries)" }
};

static const struct d_test_sa_note_item g_min_enum_map_guidelines_items[] =
{
    { "[BEST]", "Always free value pointers before freeing map" },
    { "[BEST]", "Use entry macros for static initialization" },
    { "[BEST]", "Pre-allocate capacity if final size is known" },
    { "[BEST]", "Check return values from put/remove operations" },
    { "[BEST]", "Avoid INT_MIN as a key (reserved for sentinel)" }
};

static const struct d_test_sa_note_section g_min_enum_map_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_min_enum_map_status_items) / sizeof(g_min_enum_map_status_items[0]),
      g_min_enum_map_status_items },
    { "KNOWN ISSUES",
      sizeof(g_min_enum_map_issues_items) / sizeof(g_min_enum_map_issues_items[0]),
      g_min_enum_map_issues_items },
    { "NEXT STEPS",
      sizeof(g_min_enum_map_steps_items) / sizeof(g_min_enum_map_steps_items[0]),
      g_min_enum_map_steps_items },
    { "BEST PRACTICES",
      sizeof(g_min_enum_map_guidelines_items) / sizeof(g_min_enum_map_guidelines_items[0]),
      g_min_enum_map_guidelines_items }
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
                          "djinterp min_enum_map Module",
                          "Comprehensive Testing of d_min_enum_map Type and "
                          "Operations");

    // register the min_enum_map module
    d_test_sa_runner_add_module(&runner,
                                "min_enum_map",
                                "d_min_enum_map sorted map with enum keys, "
                                "binary search, and merge operations",
                                d_tests_min_enum_map_run_all,
                                sizeof(g_min_enum_map_notes) /
                                    sizeof(g_min_enum_map_notes[0]),
                                g_min_enum_map_notes);

    // execute all tests and return result
    return d_test_sa_runner_execute(&runner);
}