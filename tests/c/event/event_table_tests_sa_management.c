#include "./event_table_tests_sa.h"


/******************************************************************************
 * III. TABLE MANAGEMENT TESTS
 *****************************************************************************/

// d_tests_sa_et_mgmt_cb
//   helper: minimal callback for listener construction.
static void
d_tests_sa_et_mgmt_cb(size_t _count, void** _args)
{
    (void)_count;
    (void)_args;

    return;
}


/*
d_tests_sa_et_size_basic
  Tests d_event_hash_table_size.
  Tests the following:
  - Returns >= requested size
  - Returns 0 for NULL table
*/
bool
d_tests_sa_et_size_basic
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_event_hash_table* table;

    result = true;
    table  = d_event_hash_table_new(50);

    if (table)
    {
        result = d_assert_standalone(
            d_event_hash_table_size(table) >= 50,
            "et_size_valid",
            "Size should be >= requested",
            _counter) && result;

        d_event_hash_table_free(table);
    }

    result = d_assert_standalone(
        d_event_hash_table_size(NULL) == 0,
        "et_size_null",
        "Size of NULL table should be 0",
        _counter) && result;

    return result;
}

/*
d_tests_sa_et_count_basic
  Tests d_event_hash_table_count.
  Tests the following:
  - Returns 0 for empty table
  - Returns correct count after inserts
  - Returns 0 for NULL table
*/
bool
d_tests_sa_et_count_basic
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_event_hash_table* table;
    struct d_event_listener*   l1;
    struct d_event_listener*   l2;

    result = true;
    table  = d_event_hash_table_new(10);

    if (table)
    {
        result = d_assert_standalone(
            d_event_hash_table_count(table) == 0,
            "et_count_empty",
            "Empty table should have count 0",
            _counter) && result;

        l1 = d_event_listener_new(100, d_tests_sa_et_mgmt_cb, true);
        l2 = d_event_listener_new(200, d_tests_sa_et_mgmt_cb, true);

        d_event_hash_table_insert(table, 100, l1);
        d_event_hash_table_insert(table, 200, l2);

        result = d_assert_standalone(
            d_event_hash_table_count(table) == 2,
            "et_count_after_insert",
            "Count should be 2 after two inserts",
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
    }

    result = d_assert_standalone(
        d_event_hash_table_count(NULL) == 0,
        "et_count_null",
        "Count of NULL table should be 0",
        _counter) && result;

    return result;
}

/*
d_tests_sa_et_enabled_count_mixed
  Tests d_event_hash_table_enabled_count with mixed enabled/disabled.
  Tests the following:
  - Correctly counts only enabled listeners
  - Returns 0 for NULL table
*/
bool
d_tests_sa_et_enabled_count_mixed
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_event_hash_table* table;
    struct d_event_listener*   l1;
    struct d_event_listener*   l2;
    struct d_event_listener*   l3;

    result = true;
    table  = d_event_hash_table_new(10);

    if (table)
    {
        l1 = d_event_listener_new(100, d_tests_sa_et_mgmt_cb, true);
        l2 = d_event_listener_new(200, d_tests_sa_et_mgmt_cb, false);
        l3 = d_event_listener_new(300, d_tests_sa_et_mgmt_cb, true);

        d_event_hash_table_insert(table, 100, l1);
        d_event_hash_table_insert(table, 200, l2);
        d_event_hash_table_insert(table, 300, l3);

        result = d_assert_standalone(
            d_event_hash_table_enabled_count(table) == 2,
            "et_enabled_count_mixed",
            "Should count only enabled listeners (2 of 3)",
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

    result = d_assert_standalone(
        d_event_hash_table_enabled_count(NULL) == 0,
        "et_enabled_count_null",
        "enabled_count of NULL table should be 0",
        _counter) && result;

    return result;
}

/*
d_tests_sa_et_load_factor_basic
  Tests d_event_hash_table_load_factor.
  Tests the following:
  - Returns 0.0 for empty table
  - Returns valid positive value after inserts
  - Returns 0.0 for NULL table
*/
bool
d_tests_sa_et_load_factor_basic
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_event_hash_table* table;
    struct d_event_listener*   listeners[5];
    int                        i;

    result = true;
    table  = d_event_hash_table_new(10);

    if (table)
    {
        result = d_assert_standalone(
            d_event_hash_table_load_factor(table) == 0.0,
            "et_load_factor_empty",
            "Empty table should have load factor 0.0",
            _counter) && result;

        for (i = 0; i < 5; i++)
        {
            listeners[i] = d_event_listener_new(i, d_tests_sa_et_mgmt_cb, true);
            d_event_hash_table_insert(table, i, listeners[i]);
        }

        result = d_assert_standalone(
            d_event_hash_table_load_factor(table) > 0.0 &&
            d_event_hash_table_load_factor(table) <= 1.0,
            "et_load_factor_populated",
            "Load factor should be between 0 and 1 after inserts",
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

    result = d_assert_standalone(
        d_event_hash_table_load_factor(NULL) == 0.0,
        "et_load_factor_null",
        "Load factor of NULL table should be 0.0",
        _counter) && result;

    return result;
}

/*
d_tests_sa_et_resize_valid
  Tests d_event_hash_table_resize.
  Tests the following:
  - Returns true
  - Size increases to >= new size
  - count is preserved
  - All elements remain accessible after resize
*/
bool
d_tests_sa_et_resize_valid
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_event_hash_table* table;
    struct d_event_listener*   listeners[5];
    size_t                     old_size;
    int                        i;
    bool                       all_found;

    result = true;
    table  = d_event_hash_table_new(10);

    if (table)
    {
        for (i = 0; i < 5; i++)
        {
            listeners[i] = d_event_listener_new(
                               i * 100, d_tests_sa_et_mgmt_cb, true);
            d_event_hash_table_insert(table, i * 100, listeners[i]);
        }

        old_size = table->size;

        result = d_assert_standalone(
            d_event_hash_table_resize(table, 50) == true,
            "et_resize_succeeds",
            "Resize should return true",
            _counter) && result;

        result = d_assert_standalone(
            table->size >= 50 && table->size > old_size,
            "et_resize_size_increased",
            "Table size should increase to >= 50",
            _counter) && result;

        result = d_assert_standalone(
            table->count == 5,
            "et_resize_count_preserved",
            "count should remain 5 after resize",
            _counter) && result;

        // verify all elements still accessible
        all_found = true;

        for (i = 0; i < 5; i++)
        {
            if (d_event_hash_table_lookup(table, i * 100) != listeners[i])
            {
                all_found = false;
                break;
            }
        }

        result = d_assert_standalone(
            all_found,
            "et_resize_elements_accessible",
            "All elements should remain accessible after resize",
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
d_tests_sa_et_resize_null
  Tests d_event_hash_table_resize with NULL table.
  Tests the following:
  - Returns false
*/
bool
d_tests_sa_et_resize_null
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_assert_standalone(
        d_event_hash_table_resize(NULL, 100) == false,
        "et_resize_null",
        "Resize on NULL table should return false",
        _counter) && result;

    return result;
}

/*
d_tests_sa_et_clear_valid
  Tests d_event_hash_table_clear.
  Tests the following:
  - count set to 0
  - enabled_count set to 0
  - Table size preserved
  - Elements no longer accessible
  NOTE: clear frees nodes but NOT listeners — callers must free
  listeners separately. The original test had a memory leak here.
*/
bool
d_tests_sa_et_clear_valid
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_event_hash_table* table;
    struct d_event_listener*   listeners[5];
    size_t                     old_size;
    int                        i;
    bool                       none_found;

    result = true;
    table  = d_event_hash_table_new(10);

    if (table)
    {
        for (i = 0; i < 5; i++)
        {
            listeners[i] = d_event_listener_new(i, d_tests_sa_et_mgmt_cb, true);
            d_event_hash_table_insert(table, i, listeners[i]);
        }

        old_size = table->size;

        d_event_hash_table_clear(table);

        result = d_assert_standalone(
            table->count == 0 && table->enabled_count == 0,
            "et_clear_counts",
            "count and enabled_count should be 0 after clear",
            _counter) && result;

        result = d_assert_standalone(
            table->size == old_size,
            "et_clear_size_preserved",
            "Table size should not change after clear",
            _counter) && result;

        // verify elements removed
        none_found = true;

        for (i = 0; i < 5; i++)
        {
            if (d_event_hash_table_lookup(table, i) != NULL)
            {
                none_found = false;
                break;
            }
        }

        result = d_assert_standalone(
            none_found,
            "et_clear_elements_gone",
            "All elements should be removed after clear",
            _counter) && result;

        d_event_hash_table_free(table);

        // clear frees nodes but NOT listeners — must free separately
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
d_tests_sa_et_clear_null
  Tests d_event_hash_table_clear with NULL table.
  Tests the following:
  - Does not crash
*/
bool
d_tests_sa_et_clear_null
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    d_event_hash_table_clear(NULL);

    result = d_assert_standalone(
        true,
        "et_clear_null",
        "clear(NULL) should not crash",
        _counter) && result;

    return result;
}

/*
d_tests_sa_et_management_all
  Runs all table management tests.
*/
bool
d_tests_sa_et_management_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Table Management\n");
    printf("  -----------------------------\n");

    result = d_tests_sa_et_size_basic(_counter)          && result;
    result = d_tests_sa_et_count_basic(_counter)         && result;
    result = d_tests_sa_et_enabled_count_mixed(_counter) && result;
    result = d_tests_sa_et_load_factor_basic(_counter)   && result;
    result = d_tests_sa_et_resize_valid(_counter)        && result;
    result = d_tests_sa_et_resize_null(_counter)         && result;
    result = d_tests_sa_et_clear_valid(_counter)         && result;
    result = d_tests_sa_et_clear_null(_counter)          && result;

    return result;
}
