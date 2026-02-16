#include ".\dmutex_tests_sa.h"


///////////////////////////////////////////////////////////////////////////////
// file-scope helper structs and worker functions
///////////////////////////////////////////////////////////////////////////////

// helper: simple thread function that sets a flag
static void*
d_test_thread_set_flag
(
    void* _arg
)
{
    volatile int* flag;

    flag  = (volatile int*)_arg;
    *flag = 1;

    return D_THREAD_SUCCESS;
}

// helper: thread function that returns a known value
static void*
d_test_thread_return_value
(
    void* _arg
)
{
    (void)_arg;

    return (void*)(uintptr_t)42;
}

// helper struct for mutex-protected counter increment
struct d_test_thread_increment_data
{
    d_mutex_t* mtx;
    int*       counter;
    int        iterations;
};

// helper: thread that increments a counter under mutex
static void*
d_test_thread_increment
(
    void* _arg
)
{
    struct d_test_thread_increment_data* data;
    int                                  i;

    data = (struct d_test_thread_increment_data*)_arg;

    for (i = 0; i < data->iterations; i++)
    {
        d_mutex_lock(data->mtx);
        (*(data->counter))++;
        d_mutex_unlock(data->mtx);
    }

    return D_THREAD_SUCCESS;
}

// helper struct for thread identity test
struct d_test_thread_id_data
{
    d_thread_t child_id;
};

// helper: thread that stores its own thread ID
static void*
d_test_thread_get_id
(
    void* _arg
)
{
    struct d_test_thread_id_data* data;

    data           = (struct d_test_thread_id_data*)_arg;
    data->child_id = d_thread_current();

    return D_THREAD_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
// test functions
///////////////////////////////////////////////////////////////////////////////


/*
d_tests_sa_thread_create_join
  Tests the d_thread_create and d_thread_join functions.
  Tests the following:
  - thread creation returns D_MUTEX_SUCCESS
  - thread function executes
  - thread join waits for completion
  - thread join retrieves result
  - multiple threads with shared mutex-protected counter
*/
bool
d_tests_sa_thread_create_join
(
    struct d_test_counter* _counter
)
{
    bool       result;
    d_thread_t thd;
    int        rc;
    int        flag;

    result = true;

    // test 1: create and join a simple thread
    flag = 0;
    rc   = d_thread_create(&thd, (d_thread_func_t)d_test_thread_set_flag, (void*)&flag);

    result = d_assert_standalone(
        rc == D_MUTEX_SUCCESS,
        "thread_create_success",
        "d_thread_create should return D_MUTEX_SUCCESS",
        _counter) && result;

    if (rc == D_MUTEX_SUCCESS)
    {
        rc = d_thread_join(thd, NULL);

        result = d_assert_standalone(
            rc == D_MUTEX_SUCCESS,
            "thread_join_success",
            "d_thread_join should return D_MUTEX_SUCCESS",
            _counter) && result;

        // test 2: thread function actually ran
        result = d_assert_standalone(
            flag == 1,
            "thread_create_executed",
            "Thread function should have set flag to 1",
            _counter) && result;
    }

    // test 3: join retrieves result value
    rc = d_thread_create(&thd, (d_thread_func_t)d_test_thread_return_value, NULL);

    if (rc == D_MUTEX_SUCCESS)
    {
        d_thread_result_t thread_result;

        thread_result = D_THREAD_ERROR;
        rc            = d_thread_join(thd, &thread_result);

        result = d_assert_standalone(
            rc == D_MUTEX_SUCCESS,
            "thread_join_result_success",
            "Join with result pointer should succeed",
            _counter) && result;

        result = d_assert_standalone(
            (uintptr_t)thread_result == 42,
            "thread_join_result_value",
            "Thread result should be 42",
            _counter) && result;
    }

    // test 4: multiple threads incrementing shared counter
    {
        d_mutex_t                           mtx;
        int                                 shared_counter;
        struct d_test_thread_increment_data data_a;
        struct d_test_thread_increment_data data_b;
        d_thread_t                          thd_a;
        d_thread_t                          thd_b;

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
                                 (d_thread_func_t)d_test_thread_increment,
                                 &data_a);

            if (rc == D_MUTEX_SUCCESS)
            {
                rc = d_thread_create(&thd_b,
                                 (d_thread_func_t)d_test_thread_increment,
                                     &data_b);

                if (rc == D_MUTEX_SUCCESS)
                {
                    d_thread_join(thd_a, NULL);
                    d_thread_join(thd_b, NULL);

                    result = d_assert_standalone(
                        shared_counter == 2000,
                        "thread_mutex_counter",
                        "Two threads x 1000 iterations = 2000",
                        _counter) && result;
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
d_tests_sa_thread_detach
  Tests the d_thread_detach function.
  Tests the following:
  - detach returns D_MUTEX_SUCCESS
  - detached thread still executes to completion
*/
bool
d_tests_sa_thread_detach
(
    struct d_test_counter* _counter
)
{
    bool            result;
    d_thread_t      thd;
    int             rc;
    volatile int    detach_flag;
    struct timespec sleep_dur;

    result      = true;
    detach_flag = 0;

    rc = d_thread_create(&thd,
                                 (d_thread_func_t)d_test_thread_set_flag,
                         (void*)&detach_flag);

    if (rc == D_MUTEX_SUCCESS)
    {
        rc     = d_thread_detach(thd);
        result = d_assert_standalone(
            rc == D_MUTEX_SUCCESS,
            "thread_detach_success",
            "d_thread_detach should return D_MUTEX_SUCCESS",
            _counter) && result;

        // give the detached thread time to run
        sleep_dur.tv_sec  = 0;
        sleep_dur.tv_nsec = 50 * D_TIME_NSEC_PER_MSEC;
        d_nanosleep(&sleep_dur, NULL);

        result = d_assert_standalone(
            detach_flag == 1,
            "thread_detach_ran",
            "Detached thread should have executed",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_thread_current
  Tests the d_thread_current function.
  Tests the following:
  - returns a valid thread identifier
  - calling twice on the same thread gives equal results
*/
bool
d_tests_sa_thread_current
(
    struct d_test_counter* _counter
)
{
    bool       result;
    d_thread_t self1;
    d_thread_t self2;

    result = true;

    self1 = d_thread_current();
    self2 = d_thread_current();

    result = d_assert_standalone(
        d_thread_equal(self1, self2) != 0,
        "thread_current_consistent",
        "Two calls to d_thread_current should return equal IDs",
        _counter) && result;

    return result;
}


/*
d_tests_sa_thread_equal
  Tests the d_thread_equal function.
  Tests the following:
  - same thread compares equal
  - different threads compare not equal
*/
bool
d_tests_sa_thread_equal
(
    struct d_test_counter* _counter
)
{
    bool       result;
    d_thread_t self;

    result = true;

    // test 1: thread equals itself
    self   = d_thread_current();
    result = d_assert_standalone(
        d_thread_equal(self, self) != 0,
        "thread_equal_self",
        "Thread should equal itself",
        _counter) && result;

    // test 2: main thread vs child thread are not equal
    {
        struct d_test_thread_id_data shared;
        d_thread_t                   thd;
        int                          rc;

        rc = d_thread_create(&thd,
                                 (d_thread_func_t)d_test_thread_get_id,
                             &shared);

        if (rc == D_MUTEX_SUCCESS)
        {
            d_thread_join(thd, NULL);

            result = d_assert_standalone(
                d_thread_equal(self, shared.child_id) == 0,
                "thread_equal_different",
                "Main and child thread IDs should differ",
                _counter) && result;
        }
    }

    return result;
}


/*
d_tests_sa_thread_yield
  Tests the d_thread_yield function.
  Tests the following:
  - yield does not crash
  - yield returns (does not hang)
*/
bool
d_tests_sa_thread_yield
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  i;

    result = true;

    // test 1: yield does not crash and returns
    d_thread_yield();

    result = d_assert_standalone(
        true,
        "thread_yield_no_crash",
        "d_thread_yield should not crash",
        _counter) && result;

    // test 2: multiple yields in succession
    for (i = 0; i < 10; i++)
    {
        d_thread_yield();
    }

    result = d_assert_standalone(
        true,
        "thread_yield_repeated",
        "Multiple yields should not crash",
        _counter) && result;

    return result;
}


/*
d_tests_sa_thread_sleep
  Tests the d_thread_sleep function.
  Tests the following:
  - sleep returns D_MUTEX_SUCCESS
  - sleep for a short duration actually sleeps (elapsed time check)
*/
bool
d_tests_sa_thread_sleep
(
    struct d_test_counter* _counter
)
{
    bool            result;
    struct timespec duration;
    struct timespec remaining;
    int             rc;

    result = true;

    // test 1: sleep for 10ms
    duration.tv_sec  = 0;
    duration.tv_nsec = 10 * D_TIME_NSEC_PER_MSEC;

    rc     = d_thread_sleep(&duration, &remaining);
    result = d_assert_standalone(
        rc == D_MUTEX_SUCCESS,
        "thread_sleep_success",
        "d_thread_sleep should return D_MUTEX_SUCCESS",
        _counter) && result;

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

        result = d_assert_standalone(
            elapsed_ms >= 15,
            "thread_sleep_elapsed",
            "Sleep of 20ms should result in >= 15ms elapsed",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_thread_ops_all
  Aggregation function that runs all thread operation tests.
*/
bool
d_tests_sa_thread_ops_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Thread Operations\n");
    printf("  -----------------------------\n");

    result = d_tests_sa_thread_create_join(_counter) && result;
    result = d_tests_sa_thread_detach(_counter) && result;
    result = d_tests_sa_thread_current(_counter) && result;
    result = d_tests_sa_thread_equal(_counter) && result;
    result = d_tests_sa_thread_yield(_counter) && result;
    result = d_tests_sa_thread_sleep(_counter) && result;

    return result;
}
