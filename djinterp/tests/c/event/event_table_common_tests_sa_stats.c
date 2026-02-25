#include "./event_table_common_tests_sa.h"


/******************************************************************************
 * V. STATISTICS PRINT TESTS
 *****************************************************************************/

/*
d_tests_sa_event_hash_stats_print_valid
  Tests d_event_hash_table_print_stats with valid inputs.
  Tests the following:
  - Printing with valid stats and label does not crash
  - Printing with valid stats and NULL label does not crash
*/
bool
d_tests_sa_event_hash_stats_print_valid
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_event_hash_stats stats;

    result = true;

    stats.total_buckets        = 101;
    stats.used_buckets         = 75;
    stats.total_elements       = 150;
    stats.max_chain_length     = 4;
    stats.average_chain_length = 2.0;
    stats.load_factor          = 1.485;

    d_event_hash_table_stats_to_string(&stats, "Test Statistics");

    result = d_assert_standalone(
        true,
        "stats_print_valid",
        "Printing valid stats should not crash",
        _counter) && result;

    d_event_hash_table_stats_to_string(&stats, NULL);

    result = d_assert_standalone(
        true,
        "stats_print_null_label",
        "Printing with NULL label should not crash",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_hash_stats_print_null
  Tests d_event_hash_table_print_stats with NULL stats.
  Tests the following:
  - NULL stats pointer does not crash
*/
bool
d_tests_sa_event_hash_stats_print_null
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    d_event_hash_table_stats_to_string(NULL, "Null Test");

    result = d_assert_standalone(
        true,
        "stats_print_null_stats",
        "NULL stats pointer should not crash",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_hash_stats_all
  Runs all statistics tests.
*/
bool
d_tests_sa_event_hash_stats_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Statistics Printing\n");
    printf("  --------------------------------\n");

    result = d_tests_sa_event_hash_stats_print_valid(_counter) && result;
    result = d_tests_sa_event_hash_stats_print_null(_counter)  && result;

    return result;
}
