/******************************************************************************
* djinterp [test]                                                       main.c
*
*   Combined test runner for the registry container standalone tests.
*   Runs both modules:
*     1. registry        - constructors, lookup, row manipulation, aliases,
*                          queries, lookup maintenance, iterators, utilities,
*                          destructor, internal comparison
*     2. registry_common - shared strcmp, schema max_enum_key
*
*
* path:      /.config/.msvs/testing/container/registry/
*            djinterp-c-registry-tests-sa-all/main.c
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/
#include "../../../../../../../inc/c/test/test_standalone.h"
#include "../../../../../../../tests/c/container/registry/registry_common_tests_sa.h"
#include "../../../../../../../tests/c/container/registry/registry_tests_sa.h"


/******************************************************************************
 * MODULE 1 WRAPPER
 *
 *   d_tests_sa_registry_run_all includes registry_common tests via
 *   d_tests_sa_registry_common_all (section XI in registry_tests_sa.h).
 *   This wrapper runs only registry-specific sections (I - X) so
 *   module 2 can run the full registry_common tests independently
 *   through its own header.
 *****************************************************************************/

static bool
d_tests_sa_registry_core_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_tests_sa_registry_constructors_all(_counter) && result;
    result = d_tests_sa_registry_lookup_all(_counter)       && result;
    result = d_tests_sa_registry_row_manipulation_all(_counter) && result;
    result = d_tests_sa_registry_aliases_all(_counter)      && result;
    result = d_tests_sa_registry_queries_all(_counter)      && result;
    result = d_tests_sa_registry_lookup_maint_all(_counter) && result;
    result = d_tests_sa_registry_iterators_all(_counter)    && result;
    result = d_tests_sa_registry_utility_all(_counter)      && result;
    result = d_tests_sa_registry_destructor_all(_counter)   && result;
    result = d_tests_sa_registry_comparison_all(_counter)   && result;

    return result;
}


/******************************************************************************
 * MODULE 1: registry - IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_reg_status_items[] =
{
    { "[INFO]", "d_registry stores user-defined row structs in "
                "a flat array - first member MUST be const char* "
                "key" },
    { "[INFO]", "lookup is a separate sorted array of "
                "d_registry_lookup_entry for binary search by "
                "key or alias" },
    { "[INFO]", "d_registry_new_with_capacity zero-initializes "
                "the struct via memset before reserve calls - "
                "prevents access violations on error paths" },
    { "[INFO]", "d_registry_add rejects duplicate keys and NULL "
                "keys; automatically grows row and lookup arrays" },
    { "[INFO]", "d_registry_set preserves the canonical key in "
                "the row - only non-key fields are overwritten" },
    { "[INFO]", "d_registry_remove_at shifts rows and adjusts "
                "all lookup indices above the removed index" },
    { "[INFO]", "OWNS_ROWS flag causes row_free to be called for "
                "each row during clear and free" },
    { "[INFO]", "STATIC_ROWS flag prevents row/lookup array "
                "modification; d_registry_free only frees the "
                "struct itself" },
    { "[INFO]", "FROZEN flag is a soft lock that prevents all "
                "mutations; thaw re-enables them" },
    { "[INFO]", "aliases are non-canonical lookup entries pointing "
                "to the same row index as the primary key" },
    { "[INFO]", "d_registry_rebuild_lookup drops all aliases and "
                "reconstructs from row keys only" },
    { "[INFO]", "iterator supports optional filter predicate; "
                "has_next scans ahead for filtered iterators" }
};

static const struct d_test_sa_note_item g_reg_issues_items[] =
{
    { "[NOTE]", "d_registry_new_copy duplicates lookup entries "
                "including aliases - alias key pointers reference "
                "the original strings (not deep-copied)" },
    { "[NOTE]", "d_registry_add_alias on a static registry is "
                "allowed (only FROZEN rejects it) since aliases "
                "modify lookup, not the row array" },
    { "[NOTE]", "d_registry_clear on a static registry drops "
                "lookup_count only - row count and array are "
                "preserved" }
};

static const struct d_test_sa_note_section g_reg_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_reg_status_items) / sizeof(g_reg_status_items[0]),
      g_reg_status_items },
    { "KNOWN ISSUES",
      sizeof(g_reg_issues_items) / sizeof(g_reg_issues_items[0]),
      g_reg_issues_items }
};


/******************************************************************************
 * MODULE 2: registry_common - IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_rc_status_items[] =
{
    { "[INFO]", "d_registry_strcmp provides ASCII-only case-"
                "insensitive comparison (locale-independent)" },
    { "[INFO]", "NULL handling: both NULL => 0, NULL < non-NULL "
                "consistently across case-sensitive and "
                "case-insensitive modes" },
    { "[INFO]", "d_registry_schema_max_enum_key scans schema "
                "rows to find the maximum enum_key for dense "
                "value array sizing" },
    { "[INFO]", "d_cvar_registry is separate from d_registry - "
                "both headers may be included in the same "
                "translation unit" }
};

static const struct d_test_sa_note_item g_rc_issues_items[] =
{
    { "[NOTE]", "d_registry_strcmp delegates to stdlib strcmp for "
                "case-sensitive mode - behavior matches platform "
                "strcmp for non-ASCII bytes" },
    { "[NOTE]", "d_registry_schema_max_enum_key returns 0 for "
                "both NULL schema and zero count - callers must "
                "distinguish if 0 is a valid enum_key" }
};

static const struct d_test_sa_note_section g_rc_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_rc_status_items) / sizeof(g_rc_status_items[0]),
      g_rc_status_items },
    { "KNOWN ISSUES",
      sizeof(g_rc_issues_items) / sizeof(g_rc_issues_items[0]),
      g_rc_issues_items }
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

    (void)_argc;
    (void)_argv;

    d_test_sa_runner_init(&runner,
                          "djinterp Registry Container",
                          "Combined Testing of registry and "
                          "registry_common Modules");

    // module 1: registry
    d_test_sa_runner_add_module_counter(&runner,
                                        "registry",
                                        "d_registry_new, "
                                        "new_with_capacity, "
                                        "new_copy, new_from_array, "
                                        "get, add, add_rows, set, "
                                        "remove, remove_at, clear, "
                                        "add_alias, add_aliases, "
                                        "remove_alias, clear_aliases, "
                                        "alias_count, contains, "
                                        "index_of, at, count, "
                                        "capacity, is_empty, "
                                        "rebuild_lookup, sort_lookup, "
                                        "iterator_new, "
                                        "iterator_filtered, "
                                        "iterator_has_next, "
                                        "iterator_next, "
                                        "iterator_reset, "
                                        "foreach, foreach_if, "
                                        "reserve, reserve_lookup, "
                                        "shrink_to_fit, "
                                        "freeze, thaw, get_all_keys, "
                                        "free, lookup_compare, "
                                        "lookup_compare_nocase",
                                        d_tests_sa_registry_core_all,
                                        sizeof(g_reg_notes) /
                                            sizeof(g_reg_notes[0]),
                                        g_reg_notes);

    // module 2: registry_common
    d_test_sa_runner_add_module_counter(&runner,
                                        "registry_common",
                                        "d_registry_strcmp, "
                                        "d_registry_schema_max_enum_key",
                                        d_tests_sa_registry_common_run_all,
                                        sizeof(g_rc_notes) /
                                            sizeof(g_rc_notes[0]),
                                        g_rc_notes);

    return d_test_sa_runner_execute(&runner);
}