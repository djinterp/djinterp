#include "./event_table_tests_sa.h"


/******************************************************************************
 * VI. INTEGRATION, STRESS, AND EDGE CASE TESTS
 *****************************************************************************/

// d_tests_sa_et_adv_cb
//   helper: minimal callback for listener construction.
static void
d_tests_sa_et_adv_cb(size_t _count, void** _args)
{
    (void)_count;
    (void)_args;

    return;
}


/*
d_tests_sa_et_integration
  Tests a complex insert/lookup/remove workflow.
  Tests the following:
  - All 10 elements found after insert
  - count reflects removals after removing 5
*/
bool
d_tests_sa_et_integration
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_event_hash_table* table;
    struct d_event_listener*   listeners[10];
    int                        i;
    bool                       all_found;

    result = true;
    table  = d_event_hash_table_new(20);

    if (table)
    {
        // insert 10 listeners (alternating enabled/disabled)
        for (i = 0; i < 10; i++)
        {
            listeners[i] = d_event_listener_new(
                               i * 100, d_tests_sa_et_adv_cb, i % 2 == 0);
            d_event_hash_table_insert(table, i * 100, listeners[i]);
        }

        // verify all inserted
        all_found = true;

        for (i = 0; i < 10; i++)
        {
            if (!d_event_hash_table_contains(table, i * 100))
            {
                all_found = false;
                break;
            }
        }

        result = d_assert_standalone(
            all_found,
            "et_integ_all_inserted",
            "All 10 elements should be found after insert",
            _counter) && result;

        // remove first 5
        for (i = 0; i < 5; i++)
        {
            d_event_hash_table_remove(table, i * 100);
        }

        result = d_assert_standalone(
            table->count == 5,
            "et_integ_removal_count",
            "count should be 5 after removing 5 elements",
            _counter) && result;

        d_event_hash_table_free(table);

        for (i = 0; i < 10; i++)
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
d_tests_sa_et_stress
  Stress-tests with 100 insertions and full verification.
  Tests the following:
  - count reaches 100 after all insertions
  - All 100 elements remain accessible by lookup
*/
bool
d_tests_sa_et_stress
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_event_hash_table* table;
    struct d_event_listener*   listeners[100];
    int                        i;
    int                        found_count;

    result = true;
    table  = d_event_hash_table_new(50);

    if (table)
    {
        for (i = 0; i < 100; i++)
        {
            listeners[i] = d_event_listener_new(
                               i, d_tests_sa_et_adv_cb, true);
            d_event_hash_table_insert(table, i, listeners[i]);
        }

        result = d_assert_standalone(
            table->count == 100,
            "et_stress_count",
            "count should be 100 after 100 insertions",
            _counter) && result;

        // verify all accessible
        found_count = 0;

        for (i = 0; i < 100; i++)
        {
            if (d_event_hash_table_lookup(table, i) != NULL)
            {
                found_count++;
            }
        }

        result = d_assert_standalone(
            found_count == 100,
            "et_stress_all_accessible",
            "All 100 elements should be retrievable",
            _counter) && result;

        d_event_hash_table_free(table);

        for (i = 0; i < 100; i++)
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
d_tests_sa_et_collision
  Tests collision handling with a very small table.
  Tests the following:
  - All 3 elements accessible despite likely hash collisions
  - count is correct
*/
bool
d_tests_sa_et_collision
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_event_hash_table* table;
    struct d_event_listener*   l1;
    struct d_event_listener*   l2;
    struct d_event_listener*   l3;
    bool                       all_found;

    result = true;
    table  = d_event_hash_table_new(2);

    if (table)
    {
        l1 = d_event_listener_new(1, d_tests_sa_et_adv_cb, true);
        l2 = d_event_listener_new(2, d_tests_sa_et_adv_cb, true);
        l3 = d_event_listener_new(3, d_tests_sa_et_adv_cb, true);

        d_event_hash_table_insert(table, 1, l1);
        d_event_hash_table_insert(table, 2, l2);
        d_event_hash_table_insert(table, 3, l3);

        result = d_assert_standalone(
            table->count == 3,
            "et_collision_count",
            "Collision handling should keep correct count",
            _counter) && result;

        all_found =
            (d_event_hash_table_lookup(table, 1) == l1) &&
            (d_event_hash_table_lookup(table, 2) == l2) &&
            (d_event_hash_table_lookup(table, 3) == l3);

        result = d_assert_standalone(
            all_found,
            "et_collision_all_distinct",
            "Collided elements should remain individually accessible",
            _counter) && result;

        d_event_hash_table_free(table);

        if (l1)
        {
            d_event_listener_free(l1);
        }

        if (l2)
        {
            d_event_listener_free(l2);
        }

        if (l3)
        {
            d_event_listener_free(l3);
        }
    }

    return result;
}

/*
d_tests_sa_et_auto_resize
  Tests that insert triggers automatic resize at load factor threshold.
  Tests the following:
  - Table size grows beyond initial after enough inserts
*/
bool
d_tests_sa_et_auto_resize
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_event_hash_table* table;
    struct d_event_listener*   listeners[10];
    size_t                     initial_size;
    int                        i;

    result = true;
    table  = d_event_hash_table_new(5);

    if (table)
    {
        initial_size = table->size;

        for (i = 0; i < 10; i++)
        {
            listeners[i] = d_event_listener_new(
                               i * 100, d_tests_sa_et_adv_cb, true);
            d_event_hash_table_insert(table, i * 100, listeners[i]);
        }

        result = d_assert_standalone(
            table->size > initial_size,
            "et_auto_resize",
            "Table should auto-resize when load factor exceeded",
            _counter) && result;

        d_event_hash_table_free(table);

        for (i = 0; i < 10; i++)
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
d_tests_sa_et_advanced_all
  Runs all integration, stress, and edge case tests.
*/
bool
d_tests_sa_et_advanced_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Integration, Stress, and Edge Cases\n");
    printf("  ------------------------------------------------\n");

    result = d_tests_sa_et_integration(_counter) && result;
    result = d_tests_sa_et_stress(_counter)      && result;
    result = d_tests_sa_et_collision(_counter)    && result;
    result = d_tests_sa_et_auto_resize(_counter)  && result;

    return result;
}
