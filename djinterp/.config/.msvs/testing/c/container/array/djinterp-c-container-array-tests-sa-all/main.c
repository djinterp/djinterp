/******************************************************************************
* djinterp [test]                                                       main.c
*
*   Combined test runner for the container/array subsystem standalone tests.
*   Runs all 3 modules:
*     1. array_common    - initialization, manipulation, search, resize,
*                          sort, and cleanup utility functions
*     2. array_filter    - single-op filters, in-place, chains, combinators,
*                          queries, result management, macros, builder
*     3. circular_array  - constructors, access, push/pop, bulk ops, query,
*                          search, conversion, utility, memory management
*
*
* path:      /.config/.msvs/testing/c/container/array/
*                djinterp-c-container-array-tests-sa-all/main.c
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/
#include "../../../../../../../inc/c/test/test_standalone.h"
#include "../../../../../../../tests/c/container/array/array_common_tests_sa.h"
#include "../../../../../../../tests/c/container/array/array_filter_tests_sa.h"
#include "../../../../../../../tests/c/container/array/circular_array_tests_sa.h"


/******************************************************************************
 * MODULE 1: array_common - IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_ac_status_items[] =
{
    { "[INFO]", "`array_common` initialization functions "
                "validated" },
    { "[INFO]", "Utility functions (alloc, append, "
                "calc_capacity) working correctly" },
    { "[INFO]", "Search functions (find, find_closest, "
                "contains) validated" },
    { "[INFO]", "Manipulation functions (insert, prepend, "
                "shift, reverse) tested" },
    { "[INFO]", "Resize validation (amount and factor) "
                "functions working" },
    { "[INFO]", "Sort function using qsort wrapper validated" },
    { "[INFO]", "Free functions (shallow and deep) handle "
                "cleanup correctly" }
};

static const struct d_test_sa_note_item g_ac_issues_items[] =
{
    { "[NOTE]", "d_array_common functions operate on raw "
                "memory buffers" },
    { "[NOTE]", "Caller is responsible for ensuring sufficient "
                "capacity" },
    { "[NOTE]", "Negative indexing supported via d_index type" },
    { "[WARN]", "d_array_common_free_elements_deep requires "
                "valid free function" }
};

static const struct d_test_sa_note_item g_ac_steps_items[] =
{
    { "[TODO]", "Add more edge case tests for boundary "
                "conditions" },
    { "[TODO]", "Implement stress tests for large arrays" },
    { "[TODO]", "Add memory leak detection tests" },
    { "[TODO]", "Create benchmarks comparing different "
                "operations" },
    { "[TODO]", "Consider adding SIMD-optimized variants" }
};

static const struct d_test_sa_note_item g_ac_guidelines_items[] =
{
    { "[BEST]", "Always validate element_size before "
                "operations" },
    { "[BEST]", "Use d_index_convert_safe for index "
                "conversion" },
    { "[BEST]", "Ensure capacity before insert/append "
                "operations" },
    { "[BEST]", "Check return values from all operations" }
};

static const struct d_test_sa_note_section g_ac_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_ac_status_items) / sizeof(g_ac_status_items[0]),
      g_ac_status_items },
    { "KNOWN ISSUES",
      sizeof(g_ac_issues_items) / sizeof(g_ac_issues_items[0]),
      g_ac_issues_items },
    { "NEXT STEPS",
      sizeof(g_ac_steps_items) / sizeof(g_ac_steps_items[0]),
      g_ac_steps_items },
    { "BEST PRACTICES",
      sizeof(g_ac_guidelines_items) / sizeof(g_ac_guidelines_items[0]),
      g_ac_guidelines_items }
};


/******************************************************************************
 * MODULE 2: array_filter - IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_af_status_items[] =
{
    { "[INFO]", "Result structure members and status enum "
                "validated" },
    { "[INFO]", "Take operations (first, last, nth, head, "
                "tail) verified" },
    { "[INFO]", "Skip operations (first, last, init, rest) "
                "verified" },
    { "[INFO]", "Range and slice with step/clamp semantics "
                "confirmed" },
    { "[INFO]", "Predicate-based where/where_not filters "
                "validated" },
    { "[INFO]", "Index-based selection with duplicates "
                "confirmed" },
    { "[INFO]", "Distinct and reverse transformations "
                "verified" },
    { "[INFO]", "In-place filter variants modify source "
                "array correctly" },
    { "[INFO]", "Chain and combinator application produces "
                "correct sets" },
    { "[INFO]", "Query functions (count, any, all, none, "
                "find) validated" },
    { "[INFO]", "Result management lifecycle (access, release, "
                "free) tested" },
    { "[INFO]", "Convenience macros expand with correct "
                "sizeof inference" },
    { "[INFO]", "Fluent builder BEGIN/END and multi-step "
                "chains verified" }
};

static const struct d_test_sa_note_item g_af_issues_items[] =
{
    { "[WARN]", "Combinator tests assume "
                "d_filter_chain_add_where_context stores "
                "context by pointer, not by copy" },
    { "[NOTE]", "Union result count depends on deduplication "
                "behavior" },
    { "[NOTE]", "Builder test references D_THEN_WHERE, "
                "D_THEN_SKIP_FIRST, D_THEN_TAKE_FIRST macros "
                "from filter.h" }
};

static const struct d_test_sa_note_item g_af_steps_items[] =
{
    { "[TODO]", "Add stress tests with large arrays "
                "(>10k elements)" },
    { "[TODO]", "Benchmark single-pass vs. multi-pass chain "
                "performance" },
    { "[TODO]", "Add tests for struct-valued elements "
                "(not just int)" },
    { "[TODO]", "Test filter operations on pointer arrays "
                "(ptr_array)" },
    { "[TODO]", "Add thread-safety tests for concurrent "
                "filter access" }
};

static const struct d_test_sa_note_item g_af_guidelines_items[] =
{
    { "[BEST]", "Always call d_array_filter_result_free "
                "after use" },
    { "[BEST]", "Use d_array_filter_result_release for "
                "ownership transfer" },
    { "[BEST]", "Prefer convenience macros for typed arrays "
                "over raw calls" },
    { "[BEST]", "Use in-place variants when caller owns "
                "the buffer" },
    { "[BEST]", "Combine predicates at the predicate level "
                "for single-pass" }
};

static const struct d_test_sa_note_section g_af_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_af_status_items) / sizeof(g_af_status_items[0]),
      g_af_status_items },
    { "KNOWN ISSUES",
      sizeof(g_af_issues_items) / sizeof(g_af_issues_items[0]),
      g_af_issues_items },
    { "NEXT STEPS",
      sizeof(g_af_steps_items) / sizeof(g_af_steps_items[0]),
      g_af_steps_items },
    { "BEST PRACTICES",
      sizeof(g_af_guidelines_items) / sizeof(g_af_guidelines_items[0]),
      g_af_guidelines_items }
};


/******************************************************************************
 * MODULE 3: circular_array - IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_ca_status_items[] =
{
    { "[INFO]", "All constructor variants (new, default, "
                "from_arr, from_args, copy, copy_resized, "
                "fill) validated" },
    { "[INFO]", "Element access (get, set, front, back, peek, "
                "peek_back) tested with wraparound" },
    { "[INFO]", "Push/pop operations (front, back, all, "
                "overwrite) verified for FIFO/LIFO semantics" },
    { "[INFO]", "Bulk operations (clear, fill, rotate, "
                "reverse, swap) validated" },
    { "[INFO]", "Query functions (is_empty, is_full, count, "
                "capacity, available_space) confirmed" },
    { "[INFO]", "Search functions (contains, find, find_last, "
                "count_value) tested" },
    { "[INFO]", "Conversion functions (to_linear_array, "
                "to_d_array, copy_to) produce correct output" },
    { "[INFO]", "Utility functions (sort, linearize) and "
                "memory management (free, free_deep) tested" }
};

static const struct d_test_sa_note_item g_ca_issues_items[] =
{
    { "[NOTE]", "d_circular_array_pop returns pointer into "
                "internal buffer - data consumed before next "
                "push" },
    { "[NOTE]", "push_overwrite silently overwrites oldest "
                "element when full" },
    { "[WARN]", "d_circular_array_free_deep requires valid "
                "free function for element cleanup" }
};

static const struct d_test_sa_note_section g_ca_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_ca_status_items) / sizeof(g_ca_status_items[0]),
      g_ca_status_items },
    { "KNOWN ISSUES",
      sizeof(g_ca_issues_items) / sizeof(g_ca_issues_items[0]),
      g_ca_issues_items }
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
                          "djinterp Container Array Subsystem",
                          "Combined Testing of array_common, "
                          "array_filter, and circular_array "
                          "Modules");

    // module 1: array_common
    d_test_sa_runner_add_module_counter(&runner,
                                        "array_common",
                                        "d_array_common utility functions "
                                        "for initialization, manipulation, "
                                        "search, resize, and cleanup "
                                        "operations",
                                        d_tests_sa_array_common_run_all,
                                        sizeof(g_ac_notes) /
                                            sizeof(g_ac_notes[0]),
                                        g_ac_notes);

    // module 2: array_filter
    d_test_sa_runner_add_module_counter(&runner,
                                        "array_filter",
                                        "d_array_filter result struct, "
                                        "single-op filters, in-place, "
                                        "chains, combinators, queries, "
                                        "result management, macros, "
                                        "builder",
                                        d_tests_sa_array_filter_run_all,
                                        sizeof(g_af_notes) /
                                            sizeof(g_af_notes[0]),
                                        g_af_notes);

    // module 3: circular_array
    d_test_sa_runner_add_module_counter(&runner,
                                        "circular_array",
                                        "d_circular_array constructors, "
                                        "element access, push/pop, bulk "
                                        "ops, query, search, conversion, "
                                        "utility, memory management",
                                        d_tests_sa_circular_array_run_all,
                                        sizeof(g_ca_notes) /
                                            sizeof(g_ca_notes[0]),
                                        g_ca_notes);

    return d_test_sa_runner_execute(&runner);
}