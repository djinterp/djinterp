/*******************************************************************************
* djinterp [test]                                            dmutex_tests_cond.c
*
*
* path:      /tests/djinterp/c/sync/dmutex_tests_cond.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.15
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "dmutex_tests.h"


///////////////////////////////////////////////////////////////////////////////
// file-scope helper structs and worker functions
///////////////////////////////////////////////////////////////////////////////

// shared data for signal/wait tests
struct dj_test_cond_signal_data
{
    dj_mutex_t* mtx;
    dj_cond_t*  cond;
    bool       ready;
    int        value;
};

// helper: waiter thread that blocks until ready is set
static
dj_thread_result_t d_test_cond_waiter(
    void* _arg
)
{
    struct dj_test_cond_signal_data* data;

    data = (struct dj_test_cond_signal_data*)_arg;

    d_mutex_lock(data->mtx);

    while (!data->ready)
    {
        d_cond_wait(data->cond, data->mtx);
    }

    data->value = 42;

    d_mutex_unlock(data->mtx);

    return D_THREAD_SUCCESS;
}

// shared data for broadcast tests
struct dj_test_cond_broadcast_data
{
    dj_mutex_t*   mtx;
    dj_cond_t*    cond;
    bool         ready;
    volatile int woken_count;
};

// helper: broadcast waiter thread
static
dj_thread_result_t d_test_cond_broadcast_waiter(
    void* _arg
)
{
    struct dj_test_cond_broadcast_data* data;

    data = (struct dj_test_cond_broadcast_data*)_arg;

    d_mutex_lock(data->mtx);

    while (!data->ready)
    {
        d_cond_wait(data->cond, data->mtx);
    }

    data->woken_count++;

    d_mutex_unlock(data->mtx);

    return D_THREAD_SUCCESS;
}

// helper: signaler thread for timedwait test
static
dj_thread_result_t d_test_cond_timedwait_signaler(
    void* _arg
)
{
    struct dj_test_cond_signal_data* data;
    struct timespec                 sleep_dur;

    data = (struct dj_test_cond_signal_data*)_arg;

    // wait briefly then signal
    sleep_dur.tv_sec  = 0;
    sleep_dur.tv_nsec = 20 * D_TIME_NSEC_PER_MSEC;
    d_nanosleep(&sleep_dur, NULL);

    d_mutex_lock(data->mtx);
    data->ready = true;
    d_cond_signal(data->cond);
    d_mutex_unlock(data->mtx);

    return D_THREAD_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
// test functions
///////////////////////////////////////////////////////////////////////////////


/*
d_tests_cond_init_destroy
  Tests the d_cond_init and d_cond_destroy functions.
  Tests the following:
  - successful init returns D_MUTEX_SUCCESS
  - successful destroy returns D_MUTEX_SUCCESS
  - multiple condition variables can be initialized
*/
bool d_tests_cond_init_destroy(
    struct dj_test_suite* _suite
)
{
    bool     result;
    dj_cond_t cond;
    int      rc;

    d_test_suite_test(_suite, "cond_init_destroy");

    result = true;

    // test 1: init succeeds
    rc     = d_cond_init(&cond);
    result = D_TEST_CHECK_MSG(_suite,
                              "cond_init_success",
                              rc == D_MUTEX_SUCCESS,
                              "d_cond_init should return D_MUTEX_SUCCESS") && result;

    // test 2: destroy succeeds
    if (rc == D_MUTEX_SUCCESS)
    {
        rc     = d_cond_destroy(&cond);
        result = D_TEST_CHECK_MSG(_suite,
                                  "cond_destroy_success",
                                  rc == D_MUTEX_SUCCESS,
                                  "d_cond_destroy should return D_MUTEX_SUCCESS") && result;
    }

    // test 3: multiple init/destroy cycles
    {
        dj_cond_t cond_a;
        dj_cond_t cond_b;
        int      rc_a;
        int      rc_b;

        rc_a = d_cond_init(&cond_a);
        rc_b = d_cond_init(&cond_b);

        result = D_TEST_CHECK_MSG(_suite,
                                  "cond_init_multiple",
                                  rc_a == D_MUTEX_SUCCESS && rc_b == D_MUTEX_SUCCESS,
                                  "Multiple condition variables should initialize") && result;

        if (rc_a == D_MUTEX_SUCCESS)
        {
            d_cond_destroy(&cond_a);
        }

        if (rc_b == D_MUTEX_SUCCESS)
        {
            d_cond_destroy(&cond_b);
        }
    }

    return result;
}


/*
d_tests_cond_signal
  Tests the d_cond_signal function.
  Tests the following:
  - signal returns D_MUTEX_SUCCESS
  - signal wakes one waiting thread
  - signaled thread resumes and sees updated state
*/
bool d_tests_cond_signal(
    struct dj_test_suite* _suite
)
{
    bool                           result;
    dj_mutex_t                      mtx;
    dj_cond_t                       cond;
    struct dj_test_cond_signal_data shared;
    dj_thread_t                     thd;
    int                            rc;
    struct timespec                sleep_dur;

    d_test_suite_test(_suite, "cond_signal");

    result = true;

    rc = d_mutex_init(&mtx);

    if (rc != D_MUTEX_SUCCESS)
    {
        return result;
    }

    rc = d_cond_init(&cond);

    if (rc != D_MUTEX_SUCCESS)
    {
        d_mutex_destroy(&mtx);

        return result;
    }

    shared.mtx   = &mtx;
    shared.cond  = &cond;
    shared.ready = false;
    shared.value = 0;

    rc = d_thread_create(&thd, d_test_cond_waiter, &shared);

    if (rc == D_MUTEX_SUCCESS)
    {
        // give waiter time to enter wait
        sleep_dur.tv_sec  = 0;
        sleep_dur.tv_nsec = 20 * D_TIME_NSEC_PER_MSEC;
        d_nanosleep(&sleep_dur, NULL);

        // signal the waiter
        d_mutex_lock(&mtx);
        shared.ready = true;

        rc     = d_cond_signal(&cond);
        result = D_TEST_CHECK_MSG(_suite,
                                  "cond_signal_success",
                                  rc == D_MUTEX_SUCCESS,
                                  "d_cond_signal should return D_MUTEX_SUCCESS") && result;

        d_mutex_unlock(&mtx);

        d_thread_join(thd, NULL);

        // waiter saw the signal and set value
        result = D_TEST_CHECK_MSG(_suite,
                                  "cond_signal_woke_thread",
                                  shared.value == 42,
                                  "Signaled thread should have set value to 42") && result;
    }

    d_cond_destroy(&cond);
    d_mutex_destroy(&mtx);

    return result;
}


/*
d_tests_cond_broadcast
  Tests the d_cond_broadcast function.
  Tests the following:
  - broadcast returns D_MUTEX_SUCCESS
  - broadcast wakes all waiting threads
*/
bool d_tests_cond_broadcast(
    struct dj_test_suite* _suite
)
{
    bool                              result;
    dj_mutex_t                         mtx;
    dj_cond_t                          cond;
    struct dj_test_cond_broadcast_data shared;
    dj_thread_t                        threads[4];
    int                               rc;
    int                               i;
    int                               created;
    struct timespec                   sleep_dur;

    d_test_suite_test(_suite, "cond_broadcast");

    result = true;

    rc = d_mutex_init(&mtx);

    if (rc != D_MUTEX_SUCCESS)
    {
        return result;
    }

    rc = d_cond_init(&cond);

    if (rc != D_MUTEX_SUCCESS)
    {
        d_mutex_destroy(&mtx);

        return result;
    }

    shared.mtx         = &mtx;
    shared.cond        = &cond;
    shared.ready       = false;
    shared.woken_count = 0;

    // create 4 waiting threads
    created = 0;

    for (i = 0; i < 4; i++)
    {
        rc = d_thread_create(&threads[i],
                                 d_test_cond_broadcast_waiter,
                             &shared);

        if (rc == D_MUTEX_SUCCESS)
        {
            created++;
        }
    }

    if (created > 0)
    {
        // give waiters time to enter wait
        sleep_dur.tv_sec  = 0;
        sleep_dur.tv_nsec = 50 * D_TIME_NSEC_PER_MSEC;
        d_nanosleep(&sleep_dur, NULL);

        // broadcast to all
        d_mutex_lock(&mtx);
        shared.ready = true;

        rc     = d_cond_broadcast(&cond);
        result = D_TEST_CHECK_MSG(_suite,
                                  "cond_broadcast_success",
                                  rc == D_MUTEX_SUCCESS,
                                  "d_cond_broadcast should return D_MUTEX_SUCCESS") && result;

        d_mutex_unlock(&mtx);

        // join all threads
        for (i = 0; i < created; i++)
        {
            d_thread_join(threads[i], NULL);
        }

        result = D_TEST_CHECK_MSG(_suite,
                                  "cond_broadcast_all_woken",
                                  shared.woken_count == created,
                                  "All waiting threads should have woken") && result;
    }

    d_cond_destroy(&cond);
    d_mutex_destroy(&mtx);

    return result;
}


/*
d_tests_cond_wait
  Tests the d_cond_wait function.
  Tests the following:
  - wait releases the mutex while waiting
  - wait reacquires the mutex after being signaled
  - wait with predicate loop pattern works correctly
*/
bool d_tests_cond_wait(
    struct dj_test_suite* _suite
)
{
    bool                           result;
    dj_mutex_t                      mtx;
    dj_cond_t                       cond;
    struct dj_test_cond_signal_data shared;
    dj_thread_t                     thd;
    int                            rc;
    struct timespec                sleep_dur;

    d_test_suite_test(_suite, "cond_wait");

    result = true;

    rc = d_mutex_init(&mtx);

    if (rc != D_MUTEX_SUCCESS)
    {
        return result;
    }

    rc = d_cond_init(&cond);

    if (rc != D_MUTEX_SUCCESS)
    {
        d_mutex_destroy(&mtx);

        return result;
    }

    shared.mtx   = &mtx;
    shared.cond  = &cond;
    shared.ready = false;
    shared.value = 0;

    rc = d_thread_create(&thd, d_test_cond_waiter, &shared);

    if (rc == D_MUTEX_SUCCESS)
    {
        // allow the waiter to block
        sleep_dur.tv_sec  = 0;
        sleep_dur.tv_nsec = 20 * D_TIME_NSEC_PER_MSEC;
        d_nanosleep(&sleep_dur, NULL);

        // test 1: main thread can acquire the mutex while waiter
        // is in cond_wait (proving wait released the mutex)
        rc = d_mutex_trylock(&mtx);
        result = D_TEST_CHECK_MSG(_suite,
                                  "cond_wait_releases_mutex",
                                  rc == D_MUTEX_SUCCESS,
                                  "Wait should release the mutex for other threads") && result;

        if (rc == D_MUTEX_SUCCESS)
        {
            shared.ready = true;
            d_cond_signal(&cond);
            d_mutex_unlock(&mtx);
        }

        d_thread_join(thd, NULL);

        // test 2: waiter reacquired mutex and updated value
        result = D_TEST_CHECK_MSG(_suite,
                                  "cond_wait_reacquires_mutex",
                                  shared.value == 42,
                                  "Waiter should have reacquired mutex and set value") && result;
    }

    d_cond_destroy(&cond);
    d_mutex_destroy(&mtx);

    return result;
}


/*
d_tests_cond_timedwait
  Tests the d_cond_timedwait function.
  Tests the following:
  - timedwait returns D_MUTEX_TIMEDOUT when not signaled
  - timedwait returns D_MUTEX_SUCCESS when signaled before timeout
*/
bool d_tests_cond_timedwait(
    struct dj_test_suite* _suite
)
{
    bool            result;
    dj_mutex_t       mtx;
    dj_cond_t        cond;
    int             rc;
    struct timespec timeout;

    d_test_suite_test(_suite, "cond_timedwait");

    result = true;

    rc = d_mutex_init(&mtx);

    if (rc != D_MUTEX_SUCCESS)
    {
        return result;
    }

    rc = d_cond_init(&cond);

    if (rc != D_MUTEX_SUCCESS)
    {
        d_mutex_destroy(&mtx);

        return result;
    }

    // test 1: timedwait times out when not signaled
    d_mutex_lock(&mtx);

    d_clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_nsec += 50 * D_TIME_NSEC_PER_MSEC;
    d_timespec_normalize(&timeout);

    rc     = d_cond_timedwait(&cond, &mtx, &timeout);
    result = D_TEST_CHECK_MSG(_suite,
                              "cond_timedwait_timeout",
                              rc == D_MUTEX_TIMEDOUT,
                              "Timedwait without signal should return D_MUTEX_TIMEDOUT") && result;

    d_mutex_unlock(&mtx);

    // test 2: timedwait succeeds when signaled in time
    {
        struct dj_test_cond_signal_data shared;
        dj_thread_t                     thd;

        shared.mtx   = &mtx;
        shared.cond  = &cond;
        shared.ready = false;
        shared.value = 0;

        rc = d_thread_create(&thd,
                                 d_test_cond_timedwait_signaler,
                             &shared);

        if (rc == D_MUTEX_SUCCESS)
        {
            d_mutex_lock(&mtx);

            // set generous timeout (2 seconds)
            d_clock_gettime(CLOCK_REALTIME, &timeout);
            timeout.tv_sec += 2;

            while (!shared.ready)
            {
                rc = d_cond_timedwait(&cond, &mtx, &timeout);

                if (rc == D_MUTEX_TIMEDOUT)
                {
                    break;
                }
            }

            result = D_TEST_CHECK_MSG(_suite,
                                      "cond_timedwait_signaled",
                                      shared.ready && rc != D_MUTEX_TIMEDOUT,
                                      "Timedwait should succeed when signaled in time") && result;

            d_mutex_unlock(&mtx);
            d_thread_join(thd, NULL);
        }
    }

    d_cond_destroy(&cond);
    d_mutex_destroy(&mtx);

    return result;
}


/*
d_tests_cond_all
  Aggregation function that runs all condition variable tests.
*/
bool d_tests_cond_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "cond_all");

    result = true;

    result = d_tests_cond_init_destroy(_suite) && result;
    result = d_tests_cond_signal(_suite) && result;
    result = d_tests_cond_broadcast(_suite) && result;
    result = d_tests_cond_wait(_suite) && result;
    result = d_tests_cond_timedwait(_suite) && result;

    return result;
}
