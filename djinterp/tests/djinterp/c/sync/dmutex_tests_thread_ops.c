/*******************************************************************************
* djinterp [c][sync]                                   dmutex_tests_thread_ops.c
*
*
* path:      /tests/djinterp/c/sync/dmutex_tests_thread_ops.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.15
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "dmutex_tests.h"


///////////////////////////////////////////////////////////////////////////////
// file-scope helper structs and worker functions
///////////////////////////////////////////////////////////////////////////////

// helper: simple thread function that sets a flag
static
dj_thread_result_t d_test_thread_set_flag(
    void* _arg
)
{
    volatile int* flag;

    flag  = (volatile int*)_arg;
    *flag = 1;

    return D_THREAD_SUCCESS;
}

// helper: thread function that returns a known value
static
dj_thread_result_t d_test_thread_return_value(
    void* _arg
)
{
    (void)_arg;

    return (dj_thread_result_t)(uintptr_t)42;
}

// helper struct for mutex-protected counter increment
struct dj_test_thread_increment_data
{
    dj_mutex_t* mtx;
    int*       counter;
    int        iterations;
};

// helper: thread that increments a counter under mutex
static
dj_thread_result_t d_test_thread_increment(
    void* _arg
)
{
    struct dj_test_thread_increment_data* data;
    int                                  i;

    data = (struct dj_test_thread_increment_data*)_arg;

    for (i = 0; i < data->iterations; i++)
    {
        d_mutex_lock(data->mtx);
        (*(data->counter))++;
        d_mutex_unlock(data->mtx);
    }

    return D_THREAD_SUCCESS;
}

// helper struct for thread identity test
struct dj_test_thread_id_data
{
    dj_thread_t child_id;
};

// helper: thread that stores its own thread ID
static
dj_thread_result_t d_test_thread_get_id(
    void* _arg
)
{
    struct dj_test_thread_id_data* data;

    data           = (struct dj_test_thread_id_data*)_arg;
    data->child_id = d_thread_current();

    return D_THREAD_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
// test functions
///////////////////////////////////////////////////////////////////////////////


/*
d_tests_thread_create_join
  Tests the d_thread_create and d_thread_join functions.
  Tests the following:
  - thread creation returns D_MUTEX_SUCCESS
  - thread function executes
  - thread join waits for completion
  - thread join retrieves result
  - multiple threads with shared mutex-protected counter
*/
bool d_tests_thread_create_join(
    struct dj_test_suite* _suite
)
{
    bool       result;
    dj_thread_t thd;
    int        rc;
    int        flag;

    d_test_suite_test(_suite, "thread_create_join");

    result = true;

    // test 1: create and join a simple thread
    flag = 0;
    rc   = d_thread_create(&thd, d_test_thread_set_flag, (void*)&flag);

    result = D_TEST_CHECK_MSG(_suite,
                              "thread_create_success",
                              rc == D_MUTEX_SUCCESS,
                              "d_thread_create should return D_MUTEX_SUCCESS") && result;

    if (rc == D_MUTEX_SUCCESS)
    {
        rc = d_thread_join(thd, NULL);

        result = D_TEST_CHECK_MSG(_suite,
                                  "thread_join_success",
                                  rc == D_MUTEX_SUCCESS,
                                  "d_thread_join should return D_MUTEX_SUCCESS") && result;

        // test 2: thread function actually ran
        result = D_TEST_CHECK_MSG(_suite,
                                  "thread_create_executed",
                                  flag == 1,
                                  "Thread function should have set flag to 1") && result;
    }

    // test 3: join retrieves result value
    rc = d_thread_create(&thd, d_test_thread_return_value, NULL);

    if (rc == D_MUTEX_SUCCESS)
    {
        dj_thread_result_t thread_result;

        thread_result = D_THREAD_ERROR;
        rc            = d_thread_join(thd, &thread_result);

        result = D_TEST_CHECK_MSG(_suite,
                                  "thread_join_result_success",
                                  rc == D_MUTEX_SUCCESS,
                                  "Join with result pointer should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "thread_join_result_value",
                                  (uintptr_t)thread_result == 42,
                                  "Thread result should be 42") && result;
    }

    // test 4: multiple threads incrementing shared counter
    {
        dj_mutex_t                           mtx;
        int                                 shared_counter;
        struct dj_test_thread_increment_data data_a;
        struct dj_test_thread_increment_data data_b;
        dj_thread_t                          thd_a;
        dj_thread_t                          thd_b;

        shared_counter = 0;
        rc             = d_mutex_init(&mtx);

        if (rc == D_MUTEX_SUCCESS)
        {
            data_a.mtx        = &mtx;
            data_a.counter    = &shared_counter;
            data_a.iterations = 1000;

            data_b.mtx        = &mtx;
            data_b.counter    = &shared_counter;
            data_b.iterations = 1000;

            rc = d_thread_create(&thd_a,
                                 d_test_thread_increment,
                                 &data_a);

            if (rc == D_MUTEX_SUCCESS)
            {
                rc = d_thread_create(&thd_b,
                                 d_test_thread_increment,
                                     &data_b);

                if (rc == D_MUTEX_SUCCESS)
                {
                    d_thread_join(thd_a, NULL);
                    d_thread_join(thd_b, NULL);

                    result = D_TEST_CHECK_MSG(_suite,
                                              "thread_mutex_counter",
                                              shared_counter == 2000,
                                              "Two threads x 1000 iterations = 2000") && result;
                }
                else
                {
                    d_thread_join(thd_a, NULL);
                }
            }

            d_mutex_destroy(&mtx);
        }
    }

    return result;
}


/*
d_tests_thread_detach
  Tests the d_thread_detach function.
  Tests the following:
  - detach returns D_MUTEX_SUCCESS
  - detached thread still executes to completion
*/
bool d_tests_thread_detach(
    struct dj_test_suite* _suite
)
{
    bool            result;
    dj_thread_t      thd;
    int             rc;
    volatile int    detach_flag;
    struct timespec sleep_dur;

    d_test_suite_test(_suite, "thread_detach");

    result      = true;
    detach_flag = 0;

    rc = d_thread_create(&thd,
                                 d_test_thread_set_flag,
                         (void*)&detach_flag);

    if (rc == D_MUTEX_SUCCESS)
    {
        rc     = d_thread_detach(thd);
        result = D_TEST_CHECK_MSG(_suite,
                                  "thread_detach_success",
                                  rc == D_MUTEX_SUCCESS,
                                  "d_thread_detach should return D_MUTEX_SUCCESS") && result;

        // give the detached thread time to run
        sleep_dur.tv_sec  = 0;
        sleep_dur.tv_nsec = 50 * D_TIME_NSEC_PER_MSEC;
        d_nanosleep(&sleep_dur, NULL);

        result = D_TEST_CHECK_MSG(_suite,
                                  "thread_detach_ran",
                                  detach_flag == 1,
                                  "Detached thread should have executed") && result;
    }

    return result;
}


/*
d_tests_thread_current
  Tests the d_thread_current function.
  Tests the following:
  - returns a valid thread identifier
  - calling twice on the same thread gives equal results
*/
bool d_tests_thread_current(
    struct dj_test_suite* _suite
)
{
    bool       result;
    dj_thread_t self1;
    dj_thread_t self2;

    d_test_suite_test(_suite, "thread_current");

    result = true;

    self1 = d_thread_current();
    self2 = d_thread_current();

    result = D_TEST_CHECK_MSG(_suite,
                              "thread_current_consistent",
                              d_thread_equal(self1, self2) != 0,
                              "Two calls to d_thread_current should return equal IDs") && result;

    return result;
}


/*
d_tests_thread_equal
  Tests the d_thread_equal function.
  Tests the following:
  - same thread compares equal
  - different threads compare not equal
*/
bool d_tests_thread_equal(
    struct dj_test_suite* _suite
)
{
    bool       result;
    dj_thread_t self;

    d_test_suite_test(_suite, "thread_equal");

    result = true;

    // test 1: thread equals itself
    self   = d_thread_current();
    result = D_TEST_CHECK_MSG(_suite,
                              "thread_equal_self",
                              d_thread_equal(self, self) != 0,
                              "Thread should equal itself") && result;

    // test 2: main thread vs child thread are not equal
    {
        struct dj_test_thread_id_data shared;
        dj_thread_t                   thd;
        int                          rc;

        rc = d_thread_create(&thd,
                                 d_test_thread_get_id,
                             &shared);

        if (rc == D_MUTEX_SUCCESS)
        {
            d_thread_join(thd, NULL);

            result = D_TEST_CHECK_MSG(_suite,
                                      "thread_equal_different",
                                      d_thread_equal(self, shared.child_id) == 0,
                                      "Main and child thread IDs should differ") && result;
        }
    }

    return result;
}


/*
d_tests_thread_yield
  Tests the d_thread_yield function.
  Tests the following:
  - yield does not crash
  - yield returns (does not hang)
*/
bool d_tests_thread_yield(
    struct dj_test_suite* _suite
)
{
    bool result;
    int  i;

    d_test_suite_test(_suite, "thread_yield");

    result = true;

    // test 1: yield does not crash and returns
    d_thread_yield();

    result = D_TEST_CHECK_MSG(_suite,
                              "thread_yield_no_crash",
                              true,
                              "d_thread_yield should not crash") && result;

    // test 2: multiple yields in succession
    for (i = 0; i < 10; i++)
    {
        d_thread_yield();
    }

    result = D_TEST_CHECK_MSG(_suite,
                              "thread_yield_repeated",
                              true,
                              "Multiple yields should not crash") && result;

    return result;
}


/*
d_tests_thread_sleep
  Tests the d_thread_sleep function.
  Tests the following:
  - sleep returns D_MUTEX_SUCCESS
  - sleep for a short duration actually sleeps (elapsed time check)
*/
bool d_tests_thread_sleep(
    struct dj_test_suite* _suite
)
{
    bool            result;
    struct timespec duration;
    struct timespec remaining;
    int             rc;

    d_test_suite_test(_suite, "thread_sleep");

    result = true;

    // test 1: sleep for 10ms
    duration.tv_sec  = 0;
    duration.tv_nsec = 10 * D_TIME_NSEC_PER_MSEC;

    rc     = d_thread_sleep(&duration, &remaining);
    result = D_TEST_CHECK_MSG(_suite,
                              "thread_sleep_success",
                              rc == D_MUTEX_SUCCESS,
                              "d_thread_sleep should return D_MUTEX_SUCCESS") && result;

    // test 2: verify actual time elapsed
    {
        int64_t         before;
        int64_t         after;
        int64_t         elapsed_ms;
        struct timespec sleep_dur;

        before = d_monotonic_time_ms();

        sleep_dur.tv_sec  = 0;
        sleep_dur.tv_nsec = 20 * D_TIME_NSEC_PER_MSEC;
        d_thread_sleep(&sleep_dur, NULL);

        after      = d_monotonic_time_ms();
        elapsed_ms = after - before;

        result = D_TEST_CHECK_MSG(_suite,
                                  "thread_sleep_elapsed",
                                  elapsed_ms >= 15,
                                  "Sleep of 20ms should result in >= 15ms elapsed") && result;
    }

    return result;
}


/*
d_tests_thread_ops_all
  Aggregation function that runs all thread operation tests.
*/
bool d_tests_thread_ops_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "thread_ops_all");

    result = true;

    result = d_tests_thread_create_join(_suite) && result;
    result = d_tests_thread_detach(_suite) && result;
    result = d_tests_thread_current(_suite) && result;
    result = d_tests_thread_equal(_suite) && result;
    result = d_tests_thread_yield(_suite) && result;
    result = d_tests_thread_sleep(_suite) && result;

    return result;
}
