#include ".\dmutex_tests_sa.h"


///////////////////////////////////////////////////////////////////////////////
// file-scope helper structs and worker functions
///////////////////////////////////////////////////////////////////////////////

// helper struct for per-thread TSS isolation test
struct d_test_tss_thread_data
{
    d_tss_t key;
    int     child_value;
    void*   child_got;
};

// helper: child thread that reads then writes its own TSS value
static void*
d_test_tss_child_worker
(
    void* _arg
)
{
    struct d_test_tss_thread_data* data;

    data = (struct d_test_tss_thread_data*)_arg;

    // child sees its own TSS (initially NULL)
    data->child_got = d_tss_get(data->key);

    // child sets its own TSS
    d_tss_set(data->key, &data->child_value);

    return D_THREAD_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
// test functions
///////////////////////////////////////////////////////////////////////////////


/*
d_tests_sa_tss_create_delete
  Tests the d_tss_create and d_tss_delete functions.
  Tests the following:
  - successful creation returns D_MUTEX_SUCCESS
  - successful deletion returns D_MUTEX_SUCCESS
  - creation with NULL destructor succeeds
  - multiple keys can be created
*/
bool
d_tests_sa_tss_create_delete
(
    struct d_test_counter* _counter
)
{
    bool    result;
    d_tss_t key;
    int     rc;

    result = true;

    // test 1: create with NULL destructor
    rc     = d_tss_create(&key, NULL);
    result = d_assert_standalone(
        rc == D_MUTEX_SUCCESS,
        "tss_create_null_dtor",
        "d_tss_create with NULL destructor should succeed",
        _counter) && result;

    if (rc == D_MUTEX_SUCCESS)
    {
        // test 2: delete succeeds
        rc     = d_tss_delete(key);
        result = d_assert_standalone(
            rc == D_MUTEX_SUCCESS,
            "tss_delete_success",
            "d_tss_delete should return D_MUTEX_SUCCESS",
            _counter) && result;
    }

    // test 3: multiple keys
    {
        d_tss_t key_a;
        d_tss_t key_b;
        int     rc_a;
        int     rc_b;

        rc_a = d_tss_create(&key_a, NULL);
        rc_b = d_tss_create(&key_b, NULL);

        result = d_assert_standalone(
            rc_a == D_MUTEX_SUCCESS && rc_b == D_MUTEX_SUCCESS,
            "tss_create_multiple",
            "Multiple TSS keys should be creatable",
            _counter) && result;

        if (rc_a == D_MUTEX_SUCCESS)
        {
            d_tss_delete(key_a);
        }

        if (rc_b == D_MUTEX_SUCCESS)
        {
            d_tss_delete(key_b);
        }
    }

    return result;
}


/*
d_tests_sa_tss_get_set
  Tests the d_tss_get and d_tss_set functions.
  Tests the following:
  - get on unset key returns NULL
  - set then get returns the set value
  - overwriting a value works correctly
  - setting NULL clears the value
*/
bool
d_tests_sa_tss_get_set
(
    struct d_test_counter* _counter
)
{
    bool    result;
    d_tss_t key;
    int     rc;
    void*   val;

    result = true;

    rc = d_tss_create(&key, NULL);

    if (rc == D_MUTEX_SUCCESS)
    {
        // test 1: get on freshly created key returns NULL
        val    = d_tss_get(key);
        result = d_assert_standalone(
            val == NULL,
            "tss_get_initial_null",
            "Get on unset key should return NULL",
            _counter) && result;

        // test 2: set then get
        {
            int  data;
            int* retrieved;

            data = 12345;
            rc   = d_tss_set(key, &data);

            result = d_assert_standalone(
                rc == D_MUTEX_SUCCESS,
                "tss_set_success",
                "d_tss_set should return D_MUTEX_SUCCESS",
                _counter) && result;

            retrieved = (int*)d_tss_get(key);

            result = d_assert_standalone(
                retrieved == &data && *retrieved == 12345,
                "tss_get_after_set",
                "Get should return the pointer set previously",
                _counter) && result;
        }

        // test 3: overwrite value
        {
            int  new_data;
            int* retrieved;

            new_data  = 99999;
            d_tss_set(key, &new_data);
            retrieved = (int*)d_tss_get(key);

            result = d_assert_standalone(
                retrieved == &new_data && *retrieved == 99999,
                "tss_overwrite",
                "Overwritten TSS value should be retrievable",
                _counter) && result;
        }

        // test 4: set NULL to clear
        d_tss_set(key, NULL);
        val = d_tss_get(key);

        result = d_assert_standalone(
            val == NULL,
            "tss_set_null_clear",
            "Setting NULL should clear the TSS value",
            _counter) && result;

        d_tss_delete(key);
    }

    return result;
}


/*
d_tests_sa_tss_per_thread
  Tests that TSS values are per-thread.
  Tests the following:
  - main thread and child thread have independent TSS values
  - setting TSS in child does not affect main thread's TSS value
*/
bool
d_tests_sa_tss_per_thread
(
    struct d_test_counter* _counter
)
{
    bool    result;
    d_tss_t key;
    int     rc;

    result = true;

    rc = d_tss_create(&key, NULL);

    if (rc == D_MUTEX_SUCCESS)
    {
        struct d_test_tss_thread_data shared;
        d_thread_t                    thd;
        int                           main_value;

        main_value = 111;

        // set main thread's TSS
        d_tss_set(key, &main_value);

        shared.key         = key;
        shared.child_value = 222;
        shared.child_got   = NULL;

        rc = d_thread_create(&thd,
                                 (d_thread_func_t)d_test_tss_child_worker,
                             &shared);

        if (rc == D_MUTEX_SUCCESS)
        {
            d_thread_join(thd, NULL);

            // test 1: child thread started with NULL TSS
            result = d_assert_standalone(
                shared.child_got == NULL,
                "tss_per_thread_child_null",
                "Child thread should see NULL for unset TSS",
                _counter) && result;

            // test 2: main thread's TSS is unaffected
            {
                int* main_got;

                main_got = (int*)d_tss_get(key);

                result = d_assert_standalone(
                    main_got == &main_value &&
                    *main_got == 111,
                    "tss_per_thread_main_intact",
                    "Main thread TSS should be unaffected by "
                    "child",
                    _counter) && result;
            }
        }

        d_tss_delete(key);
    }

    return result;
}


/*
d_tests_sa_tss_all
  Aggregation function that runs all TSS tests.
*/
bool
d_tests_sa_tss_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Thread-Specific Storage\n");
    printf("  -----------------------------------\n");

    result = d_tests_sa_tss_create_delete(_counter) && result;
    result = d_tests_sa_tss_get_set(_counter) && result;
    result = d_tests_sa_tss_per_thread(_counter) && result;

    return result;
}
