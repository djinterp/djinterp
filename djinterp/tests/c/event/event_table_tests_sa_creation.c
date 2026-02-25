#include "./event_table_tests_sa.h"


/******************************************************************************
 * I. CREATION AND DESTRUCTION TESTS
 *****************************************************************************/

// d_tests_sa_et_cb
//   helper: minimal callback for listener construction in table tests.
static void
d_tests_sa_et_cb(size_t _count, void** _args)
{
    (void)_count;
    (void)_args;

    return;
}


/*
d_tests_sa_et_new_valid
  Tests d_event_hash_table_new with a valid size.
  Tests the following:
  - Returns non-NULL
  - size is >= requested (rounded to prime)
  - count is 0
  - enabled_count is 0
  - buckets is non-NULL
*/
bool
d_tests_sa_et_new_valid
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_event_hash_table* table;

    result = true;
    table  = d_event_hash_table_new(50);

    result = d_assert_standalone(
        table != NULL,
        "et_new_valid_non_null",
        "d_event_hash_table_new(50) should return non-NULL",
        _counter) && result;

    if (table)
    {
        result = d_assert_standalone(
            table->size >= 50 &&
            table->count == 0 &&
            table->enabled_count == 0 &&
            table->buckets != NULL,
            "et_new_valid_fields",
            "size>=50, count=0, enabled_count=0, buckets non-NULL",
            _counter) && result;

        d_event_hash_table_free(table);
    }

    return result;
}

/*
d_tests_sa_et_new_zero
  Tests d_event_hash_table_new with zero size.
  Tests the following:
  - Uses default size, returns non-NULL with size > 0
*/
bool
d_tests_sa_et_new_zero
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_event_hash_table* table;

    result = true;
    table  = d_event_hash_table_new(0);

    result = d_assert_standalone(
        table != NULL && table->size > 0,
        "et_new_zero",
        "new(0) should use default size",
        _counter) && result;

    if (table)
    {
        d_event_hash_table_free(table);
    }

    return result;
}

/*
d_tests_sa_et_new_small
  Tests d_event_hash_table_new with very small size.
  Tests the following:
  - Rounds up to at least prime >= 2
*/
bool
d_tests_sa_et_new_small
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_event_hash_table* table;

    result = true;
    table  = d_event_hash_table_new(1);

    result = d_assert_standalone(
        table != NULL && table->size >= 2,
        "et_new_small",
        "new(1) should round up to prime >= 2",
        _counter) && result;

    if (table)
    {
        d_event_hash_table_free(table);
    }

    return result;
}

/*
d_tests_sa_et_new_large
  Tests d_event_hash_table_new with large size.
  Tests the following:
  - Creates table with size >= 1000
*/
bool
d_tests_sa_et_new_large
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_event_hash_table* table;

    result = true;
    table  = d_event_hash_table_new(1000);

    result = d_assert_standalone(
        table != NULL && table->size >= 1000,
        "et_new_large",
        "new(1000) should create table with size >= 1000",
        _counter) && result;

    if (table)
    {
        d_event_hash_table_free(table);
    }

    return result;
}

/*
d_tests_sa_et_new_default
  Tests d_event_hash_table_new_default.
  Tests the following:
  - Returns non-NULL
  - size >= D_EVENT_HASH_TABLE_DEFAULT_SIZE
  - count and enabled_count both 0
*/
bool
d_tests_sa_et_new_default
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_event_hash_table* table;

    result = true;
    table  = d_event_hash_table_new_default();

    result = d_assert_standalone(
        table != NULL,
        "et_new_default_non_null",
        "new_default should return non-NULL",
        _counter) && result;

    if (table)
    {
        result = d_assert_standalone(
            table->size >= D_EVENT_HASH_TABLE_DEFAULT_SIZE,
            "et_new_default_size",
            "Size should be >= D_EVENT_HASH_TABLE_DEFAULT_SIZE",
            _counter) && result;

        result = d_assert_standalone(
            table->count == 0 && table->enabled_count == 0,
            "et_new_default_counts",
            "count and enabled_count should be 0",
            _counter) && result;

        d_event_hash_table_free(table);
    }

    return result;
}

/*
d_tests_sa_et_free_valid
  Tests d_event_hash_table_free with a valid table.
  Tests the following:
  - Freeing a valid table does not crash
*/
bool
d_tests_sa_et_free_valid
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
        d_event_hash_table_free(table);
    }

    result = d_assert_standalone(
        true,
        "et_free_valid",
        "Freeing valid table should not crash",
        _counter) && result;

    return result;
}

/*
d_tests_sa_et_free_null
  Tests d_event_hash_table_free with NULL.
  Tests the following:
  - free(NULL) does not crash
*/
bool
d_tests_sa_et_free_null
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    d_event_hash_table_free(NULL);

    result = d_assert_standalone(
        true,
        "et_free_null",
        "free(NULL) should not crash",
        _counter) && result;

    return result;
}

/*
d_tests_sa_et_free_with_elements
  Tests d_event_hash_table_free on a table containing elements.
  Tests the following:
  - Freeing a populated table does not crash
  - Listeners survive table free (managed externally)
*/
bool
d_tests_sa_et_free_with_elements
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
        lis = d_event_listener_new(100, d_tests_sa_et_cb, true);

        if (lis)
        {
            d_event_hash_table_insert(table, 100, lis);
        }

        d_event_hash_table_free(table);

        result = d_assert_standalone(
            true,
            "et_free_with_elements",
            "Freeing populated table should not crash",
            _counter) && result;

        // listener is managed externally — must free separately
        if (lis)
        {
            d_event_listener_free(lis);
        }
    }

    return result;
}

/*
d_tests_sa_et_creation_all
  Runs all creation and destruction tests.
*/
bool
d_tests_sa_et_creation_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Creation and Destruction\n");
    printf("  -------------------------------------\n");

    result = d_tests_sa_et_new_valid(_counter)          && result;
    result = d_tests_sa_et_new_zero(_counter)           && result;
    result = d_tests_sa_et_new_small(_counter)          && result;
    result = d_tests_sa_et_new_large(_counter)          && result;
    result = d_tests_sa_et_new_default(_counter)        && result;
    result = d_tests_sa_et_free_valid(_counter)         && result;
    result = d_tests_sa_et_free_null(_counter)          && result;
    result = d_tests_sa_et_free_with_elements(_counter) && result;

    return result;
}
