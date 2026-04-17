#include ".\dmutex_tests_sa.h"


///////////////////////////////////////////////////////////////////////////////
// file-scope helper data and worker functions
///////////////////////////////////////////////////////////////////////////////

// global state for call_once tests
static volatile int g_once_counter      = 0;
static volatile int g_once_conc_counter = 0;

static void
d_test_once_increment(void)
{
    g_once_counter++;
}

static void
d_test_once_conc_increment(void)
{
    g_once_conc_counter++;
}

// helper struct for concurrent call_once test
struct d_test_once_conc_data
{
    d_once_flag_t* flag;
};

// helper: worker that calls d_call_once
static void*
d_test_once_conc_worker
(
    void* _arg
)
{
    struct d_test_once_conc_data* data;

    data = (struct d_test_once_conc_data*)_arg;
    d_call_once(data->flag, d_test_once_conc_increment);

    return D_THREAD_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
// test functions
///////////////////////////////////////////////////////////////////////////////


/*
d_tests_sa_call_once
  Tests the d_call_once function.
  Tests the following:
  - function is called exactly once
  - second call to d_call_once with same flag does not re-invoke
  - function side effects are visible after call
  - different flag allows re-invocation
*/
bool
d_tests_sa_call_once
(
    struct d_test_counter* _counter
)
{
    bool          result;
    d_once_flag_t flag = D_ONCE_FLAG_INIT;

    result = true;

    // reset global counter
    g_once_counter = 0;

    // test 1: first call invokes the function
    d_call_once(&flag, d_test_once_increment);

    result = d_assert_standalone(
        g_once_counter == 1,
        "call_once_first",
        "First d_call_once should invoke the function",
        _counter) && result;

    // test 2: second call with same flag does NOT re-invoke
    d_call_once(&flag, d_test_once_increment);

    result = d_assert_standalone(
        g_once_counter == 1,
        "call_once_second_noop",
        "Second d_call_once should not re-invoke function",
        _counter) && result;

    // test 3: third call still no re-invoke
    d_call_once(&flag, d_test_once_increment);

    result = d_assert_standalone(
        g_once_counter == 1,
        "call_once_third_noop",
        "Third d_call_once should not re-invoke function",
        _counter) && result;

    // test 4: different flag invokes again
    {
        d_once_flag_t flag2 = D_ONCE_FLAG_INIT;

        d_call_once(&flag2, d_test_once_increment);

        result = d_assert_standalone(
            g_once_counter == 2,
            "call_once_different_flag",
            "Different flag should allow re-invocation",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_call_once_concurrent
  Tests d_call_once with multiple threads racing.
  Tests the following:
  - function is called exactly once even with concurrent callers
  - all threads see the side effects after their d_call_once returns
*/
bool
d_tests_sa_call_once_concurrent
(
    struct d_test_counter* _counter
)
{
    bool                         result;
    d_once_flag_t                flag = D_ONCE_FLAG_INIT;
    struct d_test_once_conc_data shared;
    d_thread_t                   threads[8];
    int                          rc;
    int                          i;
    int                          created;

    result = true;

    g_once_conc_counter = 0;
    shared.flag         = &flag;
    created             = 0;

    for (i = 0; i < 8; i++)
    {
        rc = d_thread_create(&threads[i],
                                 (d_thread_func_t)d_test_once_conc_worker,
                             &shared);

        if (rc == D_MUTEX_SUCCESS)
        {
            created++;
        }
    }

    for (i = 0; i < created; i++)
    {
        d_thread_join(threads[i], NULL);
    }

    result = d_assert_standalone(
        g_once_conc_counter == 1,
        "call_once_concurrent_single",
        "Function should be called exactly once across 8 threads",
        _counter) && result;

    result = d_assert_standalone(
        created >= 2,
        "call_once_concurrent_threads_created",
        "At least 2 threads should have been created",
        _counter) && result;

    return result;
}


/*
d_tests_sa_sync_all
  Aggregation function that runs all synchronization primitive tests.
*/
bool
d_tests_sa_sync_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Synchronization Primitives\n");
    printf("  --------------------------------------\n");

    result = d_tests_sa_call_once(_counter) && result;
    result = d_tests_sa_call_once_concurrent(_counter) && result;

    return result;
}
