#include "./event_table_tests_sa.h"


/******************************************************************************
 * V. STATISTICS TESTS
 *****************************************************************************/

// d_tests_sa_et_stat_cb
//   helper: minimal callback for listener construction.
static void
d_tests_sa_et_stat_cb(size_t _count, void** _args)
{
    (void)_count;
    (void)_args;

    return;
}


/*
d_tests_sa_et_stats_empty
  Tests d_event_hash_table_get_stats on empty table.
  Tests the following:
  - total_elements is 0
*/
bool
d_tests_sa_et_stats_empty
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_event_hash_table* table;
    struct d_event_hash_stats  stats;

    result = true;
    table  = d_event_hash_table_new(10);

    if (table)
    {
        stats = d_event_hash_table_get_stats(table);

        result = d_assert_standalone(
            stats.total_elements == 0,
            "et_stats_empty",
            "Empty table should report 0 total_elements",
            _counter) && result;

        d_event_hash_table_free(table);
    }

    return result;
}

/*
d_tests_sa_et_stats_populated
  Tests d_event_hash_table_get_stats on populated table.
  Tests the following:
  - total_elements matches inserted count
  - total_buckets matches table size
*/
bool
d_tests_sa_et_stats_populated
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_event_hash_table* table;
    struct d_event_listener*   listeners[5];
    struct d_event_hash_stats  stats;
    int                        i;

    result = true;
    table  = d_event_hash_table_new(10);

    if (table)
    {
        for (i = 0; i < 5; i++)
        {
            listeners[i] = d_event_listener_new(
                               i * 100, d_tests_sa_et_stat_cb, true);
            d_event_hash_table_insert(table, i * 100, listeners[i]);
        }

        stats = d_event_hash_table_get_stats(table);

        result = d_assert_standalone(
            stats.total_elements == 5,
            "et_stats_populated_elements",
            "Should report 5 total_elements",
            _counter) && result;

        result = d_assert_standalone(
            stats.total_buckets == table->size,
            "et_stats_populated_buckets",
            "total_buckets should match table->size",
            _counter) && result;

        d_event_hash_table_free(table);

        for (i = 0; i < 5; i++)
        {
            if (listeners[i])
            {
                d_event_listener_free(listeners[i]);
            }
        }
    }

    return result;
}

/*
d_tests_sa_et_stats_null
  Tests d_event_hash_table_get_stats with NULL table.
  Tests the following:
  - Returns zeroed stats
*/
bool
d_tests_sa_et_stats_null
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_event_hash_stats stats;

    result = true;
    stats  = d_event_hash_table_get_stats(NULL);

    result = d_assert_standalone(
        stats.total_elements == 0 && stats.total_buckets == 0,
        "et_stats_null",
        "NULL table should return zeroed stats",
        _counter) && result;

    return result;
}

/*
d_tests_sa_et_statistics_all
  Runs all statistics tests.
*/
bool
d_tests_sa_et_statistics_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Statistics\n");
    printf("  -----------------------\n");

    result = d_tests_sa_et_stats_empty(_counter)     && result;
    result = d_tests_sa_et_stats_populated(_counter)  && result;
    result = d_tests_sa_et_stats_null(_counter)       && result;

    return result;
}
