/******************************************************************************
* djinterp [test]                                                       main.c
*
*   Test runner for functional_common module unit tests.
*   Executes comprehensive standalone tests for all utility functions,
* higher-order functions (map, filter, fold, reduce), and quantifier functions
* (any, all, none, count_if, find_if).
*
*
* path:      \tests\functional\functional_common\main.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.10
******************************************************************************/
#include "..\..\..\..\..\tests\functional\functional_common_tests_sa.h"


// --- implementation notes ---
static const struct d_test_sa_note_item functional_common_notes_status[] =
{
    { D_TEST_SYMBOL_INFO,
      "All utility functions in functional_common.h are tested." },
    { D_TEST_SYMBOL_INFO,
      "Higher-order functions (map, fold, for_each, "
      "any, all, none, count_if, find_if) are tested." }
};

static const struct d_test_sa_note_item functional_common_notes_coverage[] =
{
    { D_TEST_SYMBOL_INFO,
      "100% branch coverage: NULL params, zero sizes, "
      "valid operations, edge cases." },
    { D_TEST_SYMBOL_INFO,
      "Fold direction verified via non-commutative "
      "digit concatenation." },
    { D_TEST_SYMBOL_INFO,
      "Complementary predicate pairs (is_null/is_not_null) "
      "cross-verified." }
};

static const struct d_test_sa_note_section functional_common_note_sections[] =
{
    {
        "CURRENT STATUS",
        sizeof(functional_common_notes_status)
            / sizeof(functional_common_notes_status[0]),
        functional_common_notes_status
    },
    {
        "COVERAGE NOTES",
        sizeof(functional_common_notes_coverage)
            / sizeof(functional_common_notes_coverage[0]),
        functional_common_notes_coverage
    }
};


/*
main
  Test runner entry point for the functional_common standalone tests.
Registers the functional_common module and executes all tests via the
unified test runner.

Parameter(s):
  none.
Return:
  0 on overall success (all tests passed), or 1 on any failure.
*/
int
main
(
    void
)
{
    struct d_test_sa_runner runner;

    // initialize the test runner
    d_test_sa_runner_init(&runner,
                          "functional_common",
                          "Standalone unit tests for "
                          "functional_common module");

    // register the functional_common test module
    d_test_sa_runner_add_module_counter(
        &runner,
        "functional_common",
        "Common functional programming utilities: "
        "identity, constant, comparison, predicate, "
        "map, fold, iteration, and query functions",
        d_tests_sa_functional_common_all,
        sizeof(functional_common_note_sections)
            / sizeof(functional_common_note_sections[0]),
        functional_common_note_sections);

    // configure runner options
    d_test_sa_runner_set_wait_for_input(&runner, true);
    d_test_sa_runner_set_show_notes(&runner, true);

    // execute and return result
    return d_test_sa_runner_execute(&runner);
}