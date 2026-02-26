/******************************************************************************
* djinterp [test]                                                       main.c
*
*   Test runner for registry module standalone tests.
*   Tests the d_registry type and associated registry operations, as well
*   as registry_common shared utility functions.
*
*
* path:      /.config/.msvs/testing/c/container/registry/
*                djinterp-c-container-registry-tests-sa/main.c
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/
#include "../../../../../../../inc/c/test/test_standalone.h"
#include "../../../../../../../tests/c/container/registry/registry_tests_sa.h"


/******************************************************************************
 * IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_registry_status_items[] =
{
    { "[INFO]", "`registry` constructor functions validated" },
    { "[INFO]", "Primary lookup (get, binary search) working correctly" },
    { "[INFO]", "Row manipulation (add, add_rows, set, remove, remove_at, clear) tested" },
    { "[INFO]", "Alias functions (add, add_aliases, remove, clear, count) validated" },
    { "[INFO]", "Query functions (contains, index_of, at, count, capacity, is_empty) working" },
    { "[INFO]", "Lookup maintenance (rebuild_lookup, sort_lookup) tested" },
    { "[INFO]", "Iterator functions (new, filtered, has_next, next, reset) validated" },
    { "[INFO]", "Callback iteration (foreach, foreach_if) with early termination tested" },
    { "[INFO]", "Utility functions (reserve, shrink_to_fit, freeze, thaw, get_all_keys) working" },
    { "[INFO]", "Destructor (free) handles all registry states correctly" },
    { "[INFO]", "Internal comparison functions (case-sensitive, case-insensitive) validated" },
    { "[INFO]", "Registry common (d_registry_strcmp, schema_max_enum_key) tested" }
};

static const struct d_test_sa_note_item g_registry_issues_items[] =
{
    { "[NOTE]", "First member of row struct MUST be `const char*` key" },
    { "[NOTE]", "d_registry_set preserves canonical key (ignores key in replacement row)" },
    { "[NOTE]", "d_registry_add rejects duplicate keys; use set for updates" },
    { "[NOTE]", "d_registry_rebuild_lookup drops all aliases (canonical keys only)" },
    { "[NOTE]", "Sorted flag (D_REGISTRY_FLAG_SORTED) inserts rows in key order" },
    { "[NOTE]", "Caller must free array returned by d_registry_get_all_keys" },
    { "[NOTE]", "registry_common.h cvar struct is d_cvar_registry (avoids name collision)" },
    { "[WARN]", "d_registry_add_rows may partially succeed before a duplicate failure" }
};

static const struct d_test_sa_note_item g_registry_steps_items[] =
{
    { "[TODO]", "Add stress tests for large registries (thousands of rows)" },
    { "[TODO]", "Add memory leak detection tests" },
    { "[TODO]", "Test D_REGISTRY_DEFINE and D_REGISTRY_STATIC_INIT macros" },
    { "[TODO]", "Test D_REGISTRY_FOREACH macro iteration" },
    { "[TODO]", "Test D_REGISTRY_GET_VALUE macro field extraction" },
    { "[TODO]", "Benchmark binary search vs linear scan for various sizes" }
};

static const struct d_test_sa_note_item g_registry_guidelines_items[] =
{
    { "[BEST]", "Always check return values from add, set, and remove" },
    { "[BEST]", "Use d_registry_reserve before bulk insertions" },
    { "[BEST]", "Call d_registry_rebuild_lookup after manual row modifications" },
    { "[BEST]", "Use D_REGISTRY_FLAG_SORTED for registries requiring key-order iteration" },
    { "[BEST]", "Use d_registry_freeze to protect immutable registries" },
    { "[BEST]", "Set OWNS_ROWS + row_free for registries managing heap-allocated row data" }
};

static const struct d_test_sa_note_section g_registry_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_registry_status_items) / sizeof(g_registry_status_items[0]),
      g_registry_status_items },
    { "KNOWN ISSUES",
      sizeof(g_registry_issues_items) / sizeof(g_registry_issues_items[0]),
      g_registry_issues_items },
    { "NEXT STEPS",
      sizeof(g_registry_steps_items) / sizeof(g_registry_steps_items[0]),
      g_registry_steps_items },
    { "BEST PRACTICES",
      sizeof(g_registry_guidelines_items) / sizeof(g_registry_guidelines_items[0]),
      g_registry_guidelines_items }
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

    /* suppress unused parameter warnings */
    (void)_argc;
    (void)_argv;

    /* initialize the test runner */
    d_test_sa_runner_init(&runner,
                          "djinterp registry Module",
                          "Comprehensive Testing of d_registry Container "
                          "Operations and Lookup Functions");

    /* register the registry module */
    d_test_sa_runner_add_module_counter(&runner,
                                        "registry",
                                        "d_registry functions for "
                                        "construction, primary lookup, "
                                        "row manipulation, alias management, "
                                        "query operations, lookup maintenance, "
                                        "iteration, utility, destruction, "
                                        "internal comparison, and "
                                        "registry_common shared functions",
                                        d_tests_sa_registry_run_all,
                                        ( sizeof(g_registry_notes) /
                                            sizeof(g_registry_notes[0]) ),
                                        g_registry_notes);

    /* execute all tests and return result */
    return d_test_sa_runner_execute(&runner);
}