#include "./event_table_tests_sa.h"


/******************************************************************************
 * II. CORE OPERATIONS TESTS
 *****************************************************************************/

// d_tests_sa_et_ops_cb
//   helper: minimal callback for listener construction.
static void
d_tests_sa_et_ops_cb(size_t _count, void** _args)
{
    (void)_count;
    (void)_args;

    return;
}


/*
d_tests_sa_et_insert_valid
  Tests d_event_hash_table_insert with valid enabled listener.
  Tests the following:
  - Returns true
  - count increases to 1
  - enabled_count increases to 1
*/
bool
d_tests_sa_et_insert_valid
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_event_hash_table* table;
    struct d_event_listener*   lis;
    bool                       ins;

    result = true;
    table  = d_event_hash_table_new(10);

    if (table)
    {
        lis = d_event_listener_new(100, d_tests_sa_et_ops_cb, true);
        ins = d_event_hash_table_insert(table, 100, lis);

        result = d_assert_standalone(
            ins == true,
            "et_insert_valid_returns_true",
            "Insert with valid params should return true",
            _counter) && result;

        result = d_assert_standalone(
            table->count == 1 && table->enabled_count == 1,
            "et_insert_valid_counts",
            "count=1, enabled_count=1 after insert",
            _counter) && result;

        d_event_hash_table_free(table);

        if (lis)
        {
            d_event_listener_free(lis);
        }
    }

    return result;
}

/*
d_tests_sa_et_insert_disabled
  Tests d_event_hash_table_insert with a disabled listener.
  Tests the following:
  - count increases
  - enabled_count does NOT increase
*/
bool
d_tests_sa_et_insert_disabled
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_event_hash_table* table;
    struct d_event_listener*   l_en;
    struct d_event_listener*   l_dis;

    result = true;
    table  = d_event_hash_table_new(10);

    if (table)
    {
        l_en  = d_event_listener_new(100, d_tests_sa_et_ops_cb, true);
        l_dis = d_event_listener_new(200, d_tests_sa_et_ops_cb, false);

        d_event_hash_table_insert(table, 100, l_en);
        d_event_hash_table_insert(table, 200, l_dis);

        result = d_assert_standalone(
            table->count == 2 && table->enabled_count == 1,
            "et_insert_disabled",
            "Disabled listener should not increase enabled_count",
            _counter) && result;

        d_event_hash_table_free(table);

        if (l_en)
        {
            d_event_listener_free(l_en);
        }

        if (l_dis)
        {
            d_event_listener_free(l_dis);
        }
    }

    return result;
}

/*
d_tests_sa_et_insert_update
  Tests d_event_hash_table_insert with duplicate key (update path).
  Tests the following:
  - Returns true (update succeeds)
  - count remains unchanged
  - enabled_count updates when replacement changes enabled flag
*/
bool
d_tests_sa_et_insert_update
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_event_hash_table* table;
    struct d_event_listener*   l_orig;
    struct d_event_listener*   l_repl;
    bool                       ins;

    result = true;
    table  = d_event_hash_table_new(10);

    if (table)
    {
        // insert enabled listener
        l_orig = d_event_listener_new(100, d_tests_sa_et_ops_cb, true);
        d_event_hash_table_insert(table, 100, l_orig);

        // replace with disabled listener at same key
        l_repl = d_event_listener_new(100, d_tests_sa_et_ops_cb, false);
        ins    = d_event_hash_table_insert(table, 100, l_repl);

        result = d_assert_standalone(
            ins == true && table->count == 1,
            "et_insert_update_count",
            "Update should succeed and count should remain 1",
            _counter) && result;

        result = d_assert_standalone(
            table->enabled_count == 0,
            "et_insert_update_enabled",
            "enabled_count should decrease when replacing enabled->disabled",
            _counter) && result;

        d_event_hash_table_free(table);

        if (l_orig)
        {
            d_event_listener_free(l_orig);
        }

        if (l_repl)
        {
            d_event_listener_free(l_repl);
        }
    }

    return result;
}

/*
d_tests_sa_et_insert_null
  Tests d_event_hash_table_insert with NULL parameters.
  Tests the following:
  - NULL table returns false
  - NULL listener returns false
*/
bool
d_tests_sa_et_insert_null
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_event_hash_table* table;
    struct d_event_listener*   lis;

    result = true;

    // NULL table
    lis = d_event_listener_new(300, d_tests_sa_et_ops_cb, true);

    result = d_assert_standalone(
        d_event_hash_table_insert(NULL, 300, lis) == false,
        "et_insert_null_table",
        "Insert with NULL table should return false",
        _counter) && result;

    if (lis)
    {
        d_event_listener_free(lis);
    }

    // NULL listener
    table = d_event_hash_table_new(10);

    if (table)
    {
        result = d_assert_standalone(
            d_event_hash_table_insert(table, 400, NULL) == false,
            "et_insert_null_listener",
            "Insert with NULL listener should return false",
            _counter) && result;

        d_event_hash_table_free(table);
    }

    return result;
}

/*
d_tests_sa_et_lookup_found
  Tests d_event_hash_table_lookup finding existing keys.
  Tests the following:
  - Returns correct listener for each key
*/
bool
d_tests_sa_et_lookup_found
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
        l1 = d_event_listener_new(100, d_tests_sa_et_ops_cb, true);
        l2 = d_event_listener_new(200, d_tests_sa_et_ops_cb, true);

        d_event_hash_table_insert(table, 100, l1);
        d_event_hash_table_insert(table, 200, l2);

        result = d_assert_standalone(
            d_event_hash_table_lookup(table, 100) == l1,
            "et_lookup_found_first",
            "Lookup should return correct listener for key 100",
            _counter) && result;

        result = d_assert_standalone(
            d_event_hash_table_lookup(table, 200) == l2,
            "et_lookup_found_second",
            "Lookup should return correct listener for key 200",
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

    return result;
}

/*
d_tests_sa_et_lookup_missing
  Tests d_event_hash_table_lookup with non-existent key.
  Tests the following:
  - Returns NULL for missing key
*/
bool
d_tests_sa_et_lookup_missing
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_event_hash_table* table;

    result = true;
    table  = d_event_hash_table_new(10);

    if (table)
    {
        result = d_assert_standalone(
            d_event_hash_table_lookup(table, 999) == NULL,
            "et_lookup_missing",
            "Lookup of non-existent key should return NULL",
            _counter) && result;

        d_event_hash_table_free(table);
    }

    return result;
}

/*
d_tests_sa_et_lookup_null
  Tests d_event_hash_table_lookup with NULL table.
  Tests the following:
  - Returns NULL
*/
bool
d_tests_sa_et_lookup_null
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_assert_standalone(
        d_event_hash_table_lookup(NULL, 100) == NULL,
        "et_lookup_null",
        "Lookup on NULL table should return NULL",
        _counter) && result;

    return result;
}

/*
d_tests_sa_et_remove_valid
  Tests d_event_hash_table_remove with existing key.
  Tests the following:
  - Returns true
  - count decreases
  - enabled_count decreases for enabled listener
  - Removed element no longer found by lookup
*/
bool
d_tests_sa_et_remove_valid
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_event_hash_table* table;
    struct d_event_listener*   l1;
    struct d_event_listener*   l2;
    bool                       rm;

    result = true;
    table  = d_event_hash_table_new(10);

    if (table)
    {
        l1 = d_event_listener_new(100, d_tests_sa_et_ops_cb, true);
        l2 = d_event_listener_new(200, d_tests_sa_et_ops_cb, false);

        d_event_hash_table_insert(table, 100, l1);
        d_event_hash_table_insert(table, 200, l2);

        rm = d_event_hash_table_remove(table, 100);

        result = d_assert_standalone(
            rm == true,
            "et_remove_valid_returns_true",
            "Remove of existing key should return true",
            _counter) && result;

        result = d_assert_standalone(
            table->count == 1 && table->enabled_count == 0,
            "et_remove_valid_counts",
            "count=1, enabled_count=0 after removing enabled listener",
            _counter) && result;

        result = d_assert_standalone(
            d_event_hash_table_lookup(table, 100) == NULL,
            "et_remove_valid_gone",
            "Removed element should not be found by lookup",
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

    return result;
}

/*
d_tests_sa_et_remove_missing_and_null
  Tests d_event_hash_table_remove with missing key and NULL table.
  Tests the following:
  - Missing key returns false
  - NULL table returns false
*/
bool
d_tests_sa_et_remove_missing_and_null
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_event_hash_table* table;

    result = true;

    table = d_event_hash_table_new(10);

    if (table)
    {
        result = d_assert_standalone(
            d_event_hash_table_remove(table, 999) == false,
            "et_remove_missing",
            "Remove of non-existent key should return false",
            _counter) && result;

        d_event_hash_table_free(table);
    }

    result = d_assert_standalone(
        d_event_hash_table_remove(NULL, 100) == false,
        "et_remove_null",
        "Remove on NULL table should return false",
        _counter) && result;

    return result;
}

/*
d_tests_sa_et_contains_basic
  Tests d_event_hash_table_contains.
  Tests the following:
  - Returns true for existing key
  - Returns false for missing key
  - Returns false for NULL table
*/
bool
d_tests_sa_et_contains_basic
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_event_hash_table* table;
    struct d_event_listener*   lis;

    result = true;
    table  = d_event_hash_table_new(10);

    if (table)
    {
        lis = d_event_listener_new(100, d_tests_sa_et_ops_cb, true);
        d_event_hash_table_insert(table, 100, lis);

        result = d_assert_standalone(
            d_event_hash_table_contains(table, 100) == true,
            "et_contains_found",
            "Contains should return true for existing key",
            _counter) && result;

        result = d_assert_standalone(
            d_event_hash_table_contains(table, 999) == false,
            "et_contains_missing",
            "Contains should return false for missing key",
            _counter) && result;

        d_event_hash_table_free(table);

        if (lis)
        {
            d_event_listener_free(lis);
        }
    }

    result = d_assert_standalone(
        d_event_hash_table_contains(NULL, 100) == false,
        "et_contains_null",
        "Contains on NULL table should return false",
        _counter) && result;

    return result;
}

/*
d_tests_sa_et_operations_all
  Runs all core operations tests.
*/
bool
d_tests_sa_et_operations_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Core Operations\n");
    printf("  ----------------------------\n");

    result = d_tests_sa_et_insert_valid(_counter)            && result;
    result = d_tests_sa_et_insert_disabled(_counter)         && result;
    result = d_tests_sa_et_insert_update(_counter)           && result;
    result = d_tests_sa_et_insert_null(_counter)             && result;
    result = d_tests_sa_et_lookup_found(_counter)            && result;
    result = d_tests_sa_et_lookup_missing(_counter)          && result;
    result = d_tests_sa_et_lookup_null(_counter)             && result;
    result = d_tests_sa_et_remove_valid(_counter)            && result;
    result = d_tests_sa_et_remove_missing_and_null(_counter) && result;
    result = d_tests_sa_et_contains_basic(_counter)          && result;

    return result;
}
