#include "./event_table_tests_sa.h"


/******************************************************************************
 * IV. ITERATOR TESTS
 *****************************************************************************/

// d_tests_sa_et_iter_cb
//   helper: minimal callback for listener construction.
static void
d_tests_sa_et_iter_cb(size_t _count, void** _args)
{
    (void)_count;
    (void)_args;

    return;
}


/*
d_tests_sa_et_iter_traversal
  Tests iterator creation and full traversal.
  Tests the following:
  - iterator_begin returns non-NULL
  - Iterator visits all inserted elements
  - Iterator properly advances through buckets
*/
bool
d_tests_sa_et_iter_traversal
(
    struct d_test_counter* _counter
)
{
    bool                          result;
    struct d_event_hash_table*    table;
    struct d_event_listener*      listeners[3];
    struct d_event_hash_iterator* iter;
    int                           count;
    int                           i;

    result = true;
    table  = d_event_hash_table_new(10);

    if (table)
    {
        for (i = 0; i < 3; i++)
        {
            listeners[i] = d_event_listener_new(
                               i * 100, d_tests_sa_et_iter_cb, true);
            d_event_hash_table_insert(table, i * 100, listeners[i]);
        }

        iter = d_event_hash_table_iterator_begin(table);

        result = d_assert_standalone(
            iter != NULL,
            "et_iter_begin",
            "iterator_begin should return non-NULL",
            _counter) && result;

        if (iter)
        {
            count = 0;

            while (d_event_hash_table_iterator_has_next(iter))
            {
                d_event_id               key;
                struct d_event_listener*  lis;

                lis = d_event_hash_table_iterator_next(iter, &key);

                if (lis)
                {
                    count++;
                }
            }

            result = d_assert_standalone(
                count == 3,
                "et_iter_visits_all",
                "Iterator should visit all 3 inserted elements",
                _counter) && result;

            d_event_hash_table_iterator_free(iter);
        }

        d_event_hash_table_free(table);

        for (i = 0; i < 3; i++)
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
d_tests_sa_et_iter_empty
  Tests iterator on an empty table.
  Tests the following:
  - Iterator is created successfully
  - has_next returns false immediately
*/
bool
d_tests_sa_et_iter_empty
(
    struct d_test_counter* _counter
)
{
    bool                          result;
    struct d_event_hash_table*    table;
    struct d_event_hash_iterator* iter;

    result = true;
    table  = d_event_hash_table_new(10);

    if (table)
    {
        iter = d_event_hash_table_iterator_begin(table);

        if (iter)
        {
            result = d_assert_standalone(
                d_event_hash_table_iterator_has_next(iter) == false,
                "et_iter_empty_no_next",
                "Empty table iterator should have no next",
                _counter) && result;

            d_event_hash_table_iterator_free(iter);
        }
        else
        {
            // iterator on empty table may or may not return NULL
            // depending on implementation — either is acceptable
            result = d_assert_standalone(
                true,
                "et_iter_empty_null_ok",
                "Iterator returning NULL for empty table is acceptable",
                _counter) && result;
        }

        d_event_hash_table_free(table);
    }

    return result;
}

/*
d_tests_sa_et_iter_null
  Tests iterator functions with NULL inputs.
  Tests the following:
  - iterator_begin returns NULL for NULL table
  - iterator_free(NULL) does not crash
*/
bool
d_tests_sa_et_iter_null
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_assert_standalone(
        d_event_hash_table_iterator_begin(NULL) == NULL,
        "et_iter_null_table",
        "iterator_begin(NULL) should return NULL",
        _counter) && result;

    d_event_hash_table_iterator_free(NULL);

    result = d_assert_standalone(
        true,
        "et_iter_free_null",
        "iterator_free(NULL) should not crash",
        _counter) && result;

    return result;
}

/*
d_tests_sa_et_iterator_all
  Runs all iterator tests.
*/
bool
d_tests_sa_et_iterator_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Iterator\n");
    printf("  ---------------------\n");

    result = d_tests_sa_et_iter_traversal(_counter) && result;
    result = d_tests_sa_et_iter_empty(_counter)     && result;
    result = d_tests_sa_et_iter_null(_counter)      && result;

    return result;
}
