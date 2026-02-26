/******************************************************************************
* djinterp [test]                                                       main.c
*
*   Combined test runner for the functional subsystem standalone tests.
*   Runs all 6 modules:
*     1. functional_common - identity, constant, comparison, predicate,
*                            fold, iteration, and query utilities
*     2. pipeline          - creation, chainable operations, finalization
*     3. predicate         - AND, OR, XOR, NOT combinators (constructor,
*                            evaluation, macros)
*     4. compose           - transformer composition, partial application,
*                            composition macros
*     5. filter            - operations, chains, combinators, execution,
*                            utilities, iterator, builder, macros
*     6. fn_builder        - creation, fluent operations, execution, cleanup
*
*
* path:      /.config/.msvs/testing/functional/djinterp-c-functional-tests-sa-all/main.c
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/
#include "../../../../../../inc/c/test/test_standalone.h"
#include "../../../../../../tests/c/functional/functional_common_tests_sa.h"
#include "../../../../../../tests/c/functional/pipeline_tests_sa.h"
#include "../../../../../../tests/c/functional/predicate_tests_sa.h"
#include "../../../../../../tests/c/functional/compose_tests_sa.h"
#include "../../../../../../tests/c/functional/filter_tests_sa.h"
#include "../../../../../../tests/c/functional/fn_builder_tests_sa.h"


/******************************************************************************
 * MODULE 1: functional_common - IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_fc_status_items[] =
{
    { "[INFO]", "All utility functions in functional_common.h "
                "are tested" },
    { "[INFO]", "Higher-order functions (map, fold, for_each, "
                "any, all, none, count_if, find_if) are tested" },
    { "[INFO]", "Identity transformer returns input unchanged; "
                "identity predicate always returns true" },
    { "[INFO]", "Constant true/false predicates provide fixed "
                "return values for combinator testing" }
};

static const struct d_test_sa_note_item g_fc_coverage_items[] =
{
    { "[INFO]", "100% branch coverage: NULL params, zero sizes, "
                "valid operations, edge cases" },
    { "[INFO]", "Fold direction verified via non-commutative "
                "digit concatenation" },
    { "[INFO]", "Complementary predicate pairs (is_null/"
                "is_not_null) cross-verified" }
};

static const struct d_test_sa_note_section g_fc_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_fc_status_items) / sizeof(g_fc_status_items[0]),
      g_fc_status_items },
    { "COVERAGE NOTES",
      sizeof(g_fc_coverage_items) / sizeof(g_fc_coverage_items[0]),
      g_fc_coverage_items }
};


/******************************************************************************
 * MODULE 2: pipeline - IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_pl_status_items[] =
{
    { "[INFO]", "d_pipeline_begin wraps source array with "
                "size and element_size metadata" },
    { "[INFO]", "d_pipeline_begin_copy allocates and copies "
                "source data for ownership semantics" },
    { "[INFO]", "Chainable operations (map, filter, fold, "
                "for_each, take, skip) compose in sequence" },
    { "[INFO]", "d_pipeline_end finalizes and returns result "
                "array; d_pipeline_free releases resources" }
};

static const struct d_test_sa_note_item g_pl_issues_items[] =
{
    { "[NOTE]", "Pipeline does not own source data unless "
                "created via begin_copy - caller manages "
                "source lifetime" }
};

static const struct d_test_sa_note_section g_pl_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_pl_status_items) / sizeof(g_pl_status_items[0]),
      g_pl_status_items },
    { "KNOWN ISSUES",
      sizeof(g_pl_issues_items) / sizeof(g_pl_issues_items[0]),
      g_pl_issues_items }
};


/******************************************************************************
 * MODULE 3: predicate - IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_pr_status_items[] =
{
    { "[INFO]", "d_predicate combinators support AND, OR, XOR, "
                "and NOT operations on fn_predicate functions" },
    { "[INFO]", "Constructor (_new) variants allocate combinator "
                "state; eval variants execute in-place" },
    { "[INFO]", "Both context and non-context (simple) variants "
                "are provided for all combinators" },
    { "[INFO]", "Compound literal macros provide stack-allocated "
                "combinator convenience" }
};

static const struct d_test_sa_note_item g_pr_issues_items[] =
{
    { "[NOTE]", "Predicate combinators with NULL function "
                "pointers are rejected at construction time" }
};

static const struct d_test_sa_note_section g_pr_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_pr_status_items) / sizeof(g_pr_status_items[0]),
      g_pr_status_items },
    { "KNOWN ISSUES",
      sizeof(g_pr_issues_items) / sizeof(g_pr_issues_items[0]),
      g_pr_issues_items }
};


/******************************************************************************
 * MODULE 4: compose - IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_co_status_items[] =
{
    { "[INFO]", "d_compose_new chains two fn_transformer "
                "functions into a single composed transformer" },
    { "[INFO]", "d_compose_apply executes the composed chain "
                "with intermediate buffer management" },
    { "[INFO]", "d_partial_consumer_new binds a context to an "
                "fn_consumer for deferred application" },
    { "[INFO]", "Macros D_COMPOSE_FN, D_MAKE_COMPOSABLE, and "
                "D_MAKE_PREDICATE_FROM provide convenience "
                "wrappers" }
};

static const struct d_test_sa_note_item g_co_issues_items[] =
{
    { "[NOTE]", "Composed transformers allocate intermediate "
                "buffers - caller must free via d_compose_free" },
    { "[NOTE]", "Partial consumers do not own their context - "
                "caller manages context lifetime" }
};

static const struct d_test_sa_note_section g_co_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_co_status_items) / sizeof(g_co_status_items[0]),
      g_co_status_items },
    { "KNOWN ISSUES",
      sizeof(g_co_issues_items) / sizeof(g_co_issues_items[0]),
      g_co_issues_items }
};


/******************************************************************************
 * MODULE 5: filter - IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_fi_status_items[] =
{
    { "[INFO]", "Filter operations include take, skip, range/"
                "slice, where, indices, distinct, and reverse" },
    { "[INFO]", "Filter chains compose multiple operations "
                "into sequential filtering pipelines" },
    { "[INFO]", "Set combinators (union, intersection, "
                "difference) combine filter results" },
    { "[INFO]", "Iterator interface provides lazy traversal "
                "over filtered elements" },
    { "[INFO]", "Fluent builder enables chainable filter "
                "construction with finalize step" }
};

static const struct d_test_sa_note_item g_fi_issues_items[] =
{
    { "[NOTE]", "Filter operations allocate result arrays - "
                "caller must free via d_filter_result_free" },
    { "[NOTE]", "Filter chain does not own individual "
                "operations - caller manages op lifetime" }
};

static const struct d_test_sa_note_section g_fi_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_fi_status_items) / sizeof(g_fi_status_items[0]),
      g_fi_status_items },
    { "KNOWN ISSUES",
      sizeof(g_fi_issues_items) / sizeof(g_fi_issues_items[0]),
      g_fi_issues_items }
};


/******************************************************************************
 * MODULE 6: fn_builder - IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_fb_status_items[] =
{
    { "[INFO]", "d_fn_builder_new creates a chainable function "
                "builder with initial capacity" },
    { "[INFO]", "Fluent operations (map, filter, and_then, "
                "where) append steps to the builder chain" },
    { "[INFO]", "Execution uses ping-pong buffer strategy for "
                "sequential transform application" },
    { "[INFO]", "Builder grows capacity automatically when "
                "step count exceeds initial allocation" }
};

static const struct d_test_sa_note_item g_fb_issues_items[] =
{
    { "[NOTE]", "Builder does not own transformer/predicate "
                "function pointers - caller manages lifetime" },
    { "[NOTE]", "Execute returns false on first transform "
                "failure - partial results are discarded" }
};

static const struct d_test_sa_note_section g_fb_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_fb_status_items) / sizeof(g_fb_status_items[0]),
      g_fb_status_items },
    { "KNOWN ISSUES",
      sizeof(g_fb_issues_items) / sizeof(g_fb_issues_items[0]),
      g_fb_issues_items }
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
                          "djinterp Functional Subsystem",
                          "Combined Testing of functional_common, "
                          "pipeline, predicate, compose, filter, "
                          "and fn_builder Modules");

    // module 1: functional_common
    d_test_sa_runner_add_module_counter(&runner,
                                        "functional_common",
                                        "Common functional programming "
                                        "utilities: identity, constant, "
                                        "comparison, predicate, map, "
                                        "fold, iteration, and query "
                                        "functions",
                                        d_tests_sa_functional_common_all,
                                        sizeof(g_fc_notes) /
                                            sizeof(g_fc_notes[0]),
                                        g_fc_notes);

    // module 2: pipeline
    d_test_sa_runner_add_module_counter(&runner,
                                        "pipeline",
                                        "d_pipeline_begin, begin_copy, "
                                        "map, filter, fold, for_each, "
                                        "take, skip, chaining, end, "
                                        "free",
                                        d_tests_sa_pipeline_all,
                                        sizeof(g_pl_notes) /
                                            sizeof(g_pl_notes[0]),
                                        g_pl_notes);

    // module 3: predicate
    d_test_sa_runner_add_module_counter(&runner,
                                        "predicate",
                                        "d_predicate_and, or, xor, not "
                                        "(_new, _eval, macro variants), "
                                        "simple macro variants",
                                        d_tests_sa_predicate_run_all,
                                        sizeof(g_pr_notes) /
                                            sizeof(g_pr_notes[0]),
                                        g_pr_notes);

    // module 4: compose
    d_test_sa_runner_add_module_counter(&runner,
                                        "compose",
                                        "d_compose_new, apply, free, "
                                        "d_partial_consumer_new, apply, "
                                        "free, D_COMPOSE_FN, "
                                        "D_MAKE_COMPOSABLE, "
                                        "D_MAKE_PREDICATE_FROM",
                                        d_tests_sa_compose_run_all,
                                        sizeof(g_co_notes) /
                                            sizeof(g_co_notes[0]),
                                        g_co_notes);

    // module 5: filter
    d_test_sa_runner_add_module_counter(&runner,
                                        "filter",
                                        "filter operations, chain "
                                        "management, combinators, "
                                        "execution, utilities, "
                                        "iterator, builder, macros",
                                        d_tests_sa_filter_run_all,
                                        sizeof(g_fi_notes) /
                                            sizeof(g_fi_notes[0]),
                                        g_fi_notes);

    // module 6: fn_builder
    d_test_sa_runner_add_module_counter(&runner,
                                        "fn_builder",
                                        "d_fn_builder_new, map, filter, "
                                        "and_then, where, chaining, "
                                        "grow, execute, free",
                                        d_tests_sa_fn_builder_all,
                                        sizeof(g_fb_notes) /
                                            sizeof(g_fb_notes[0]),
                                        g_fb_notes);

    return d_test_sa_runner_execute(&runner);
}