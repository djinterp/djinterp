/*******************************************************************************
* djinterp [c][sync]                                         dmutex_tests_sync.c
*
*
* path:      /tests/djinterp/c/sync/dmutex_tests_sync.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.15
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "dmutex_tests.h"


///////////////////////////////////////////////////////////////////////////////
// file-scope helper data and worker functions
///////////////////////////////////////////////////////////////////////////////

// global state for call_once tests
static volatile int g_once_counter      = 0;
static volatile int g_once_conc_counter = 0;

static
void d_test_once_increment(void)
{
    g_once_counter++;
}

static
void d_test_once_conc_increment(void)
{
    g_once_conc_counter++;
}

// helper struct for concurrent call_once test
struct dj_test_once_conc_data
{
    dj_once_flag_t* flag;
};

// helper: worker that calls d_call_once
static
dj_thread_result_t d_test_once_conc_worker(
    void* _arg
)
{
    struct dj_test_once_conc_data* data;

    data = (struct dj_test_once_conc_data*)_arg;
    d_call_once(data->flag, d_test_once_conc_increment);

    return D_THREAD_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
// test functions
///////////////////////////////////////////////////////////////////////////////


/*
d_tests_call_once
  Tests the d_call_once function.
  Tests the following:
  - function is called exactly once
  - second call to d_call_once with same flag does not re-invoke
  - function side effects are visible after call
  - different flag allows re-invocation
*/
bool d_tests_call_once(
    struct dj_test_suite* _suite
)
{
    bool          result;
    dj_once_flag_t flag = D_ONCE_FLAG_INIT;

    d_test_suite_test(_suite, "call_once");

    result = true;

    // reset global counter
    g_once_counter = 0;

    // test 1: first call invokes the function
    d_call_once(&flag, d_test_once_increment);

    result = D_TEST_CHECK_MSG(_suite,
                              "call_once_first",
                              g_once_counter == 1,
                              "First d_call_once should invoke the function") && result;

    // test 2: second call with same flag does NOT re-invoke
    d_call_once(&flag, d_test_once_increment);

    result = D_TEST_CHECK_MSG(_suite,
                              "call_once_second_noop",
                              g_once_counter == 1,
                              "Second d_call_once should not re-invoke function") && result;

    // test 3: third call still no re-invoke
    d_call_once(&flag, d_test_once_increment);

    result = D_TEST_CHECK_MSG(_suite,
                              "call_once_third_noop",
                              g_once_counter == 1,
                              "Third d_call_once should not re-invoke function") && result;

    // test 4: different flag invokes again
    {
        dj_once_flag_t flag2 = D_ONCE_FLAG_INIT;

        d_call_once(&flag2, d_test_once_increment);

        result = D_TEST_CHECK_MSG(_suite,
                                  "call_once_different_flag",
                                  g_once_counter == 2,
                                  "Different flag should allow re-invocation") && result;
    }

    return result;
}


/*
d_tests_call_once_concurrent
  Tests d_call_once with multiple threads racing.
  Tests the following:
  - function is called exactly once even with concurrent callers
  - all threads see the side effects after their d_call_once returns
*/
bool d_tests_call_once_concurrent(
    struct dj_test_suite* _suite
)
{
    bool                         result;
    dj_once_flag_t                flag = D_ONCE_FLAG_INIT;
    struct dj_test_once_conc_data shared;
    dj_thread_t                   threads[8];
    int                          rc;
    int                          i;
    int                          created;

    d_test_suite_test(_suite, "call_once_concurrent");

    result = true;

    g_once_conc_counter = 0;
    shared.flag         = &flag;
    created             = 0;

    for (i = 0; i < 8; i++)
    {
        rc = d_thread_create(&threads[i],
                                 d_test_once_conc_worker,
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

    result = D_TEST_CHECK_MSG(_suite,
                              "call_once_concurrent_single",
                              g_once_conc_counter == 1,
                              "Function should be called exactly once across 8 threads") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "call_once_concurrent_threads_created",
                              created >= 2,
                              "At least 2 threads should have been created") && result;

    return result;
}


/*
d_tests_sync_all
  Aggregation function that runs all synchronization primitive tests.
*/
bool d_tests_sync_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "sync_all");

    result = true;

    result = d_tests_call_once(_suite) && result;
    result = d_tests_call_once_concurrent(_suite) && result;

    return result;
}
