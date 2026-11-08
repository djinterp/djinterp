/*******************************************************************************
* djinterp [test]                                     dmutex_tests_mutex_basic.c
*
*
* path:      /tests/djinterp/c/sync/dmutex_tests_mutex_basic.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.15
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "dmutex_tests.h"


///////////////////////////////////////////////////////////////////////////////
// file-scope helper structs and worker functions for threaded tests
///////////////////////////////////////////////////////////////////////////////

// helper struct for trylock-busy test
struct dj_test_trylock_busy_data
{
    dj_mutex_t* mtx;
    int        trylock_result;
};

// helper: worker that attempts trylock on an already-locked mutex
static
dj_thread_result_t d_test_trylock_busy_worker(
    void* _arg
)
{
    struct dj_test_trylock_busy_data* data;

    data = (struct dj_test_trylock_busy_data*)_arg;
    data->trylock_result = d_mutex_trylock(data->mtx);

    // if we somehow got it, release it
    if (data->trylock_result == D_MUTEX_SUCCESS)
    {
        d_mutex_unlock(data->mtx);
    }

    return D_THREAD_SUCCESS;
}


// helper struct for timedlock test
struct dj_test_timedlock_data
{
    dj_mutex_t* mtx;
    int        timedlock_result;
};

// helper: worker that attempts timedlock with a short timeout
static
dj_thread_result_t d_test_timedlock_worker(
    void* _arg
)
{
    struct dj_test_timedlock_data* data;
    struct timespec              ts;

    data = (struct dj_test_timedlock_data*)_arg;

    // set a very short timeout (50ms in the future)
    d_clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_nsec += 50 * D_TIME_NSEC_PER_MSEC;
    d_timespec_normalize(&ts);

    data->timedlock_result = d_mutex_timedlock(data->mtx, &ts);

    // if we somehow got it, release it
    if (data->timedlock_result == D_MUTEX_SUCCESS)
    {
        d_mutex_unlock(data->mtx);
    }

    return D_THREAD_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
// test functions
///////////////////////////////////////////////////////////////////////////////


/*
d_tests_mutex_init
  Tests the d_mutex_init function.
  Tests the following:
  - successful initialization returns D_MUTEX_SUCCESS
  - initialized mutex is usable (can lock/unlock)
  - multiple mutexes can be initialized independently
*/
bool d_tests_mutex_init(
    struct dj_test_suite* _suite
)
{
    bool      result;
    dj_mutex_t mtx;
    int       rc;

    d_test_suite_test(_suite, "mutex_init");

    result = true;

    // test 1: successful initialization
    rc     = d_mutex_init(&mtx);
    result = D_TEST_CHECK_MSG(_suite,
                              "mutex_init_success",
                              rc == D_MUTEX_SUCCESS,
                              "d_mutex_init should return D_MUTEX_SUCCESS") && result;

    // test 2: initialized mutex is usable
    if (rc == D_MUTEX_SUCCESS)
    {
        int lock_rc;

        lock_rc = d_mutex_lock(&mtx);
        result  = D_TEST_CHECK_MSG(_suite,
                                   "mutex_init_usable_lock",
                                   lock_rc == D_MUTEX_SUCCESS,
                                   "Initialized mutex should be lockable") && result;

        if (lock_rc == D_MUTEX_SUCCESS)
        {
            d_mutex_unlock(&mtx);
        }

        d_mutex_destroy(&mtx);
    }

    // test 3: multiple mutexes initialized independently
    {
        dj_mutex_t mtx_a;
        dj_mutex_t mtx_b;
        int       rc_a;
        int       rc_b;

        rc_a = d_mutex_init(&mtx_a);
        rc_b = d_mutex_init(&mtx_b);

        result = D_TEST_CHECK_MSG(_suite,
                                  "mutex_init_multiple",
                                  rc_a == D_MUTEX_SUCCESS && rc_b == D_MUTEX_SUCCESS,
                                  "Multiple mutexes should initialize independently") && result;

        if (rc_a == D_MUTEX_SUCCESS)
        {
            d_mutex_destroy(&mtx_a);
        }

        if (rc_b == D_MUTEX_SUCCESS)
        {
            d_mutex_destroy(&mtx_b);
        }
    }

    return result;
}


/*
d_tests_mutex_destroy
  Tests the d_mutex_destroy function.
  Tests the following:
  - successful destruction of an initialized mutex
  - destruction returns D_MUTEX_SUCCESS
  - destroy after init/lock/unlock cycle
*/
bool d_tests_mutex_destroy(
    struct dj_test_suite* _suite
)
{
    bool      result;
    dj_mutex_t mtx;
    int       rc;

    d_test_suite_test(_suite, "mutex_destroy");

    result = true;

    // test 1: basic init then destroy
    rc = d_mutex_init(&mtx);

    if (rc == D_MUTEX_SUCCESS)
    {
        rc     = d_mutex_destroy(&mtx);
        result = D_TEST_CHECK_MSG(_suite,
                                  "mutex_destroy_success",
                                  rc == D_MUTEX_SUCCESS,
                                  "d_mutex_destroy should return D_MUTEX_SUCCESS") && result;
    }

    // test 2: destroy after lock/unlock cycle
    rc = d_mutex_init(&mtx);

    if (rc == D_MUTEX_SUCCESS)
    {
        d_mutex_lock(&mtx);
        d_mutex_unlock(&mtx);

        rc     = d_mutex_destroy(&mtx);
        result = D_TEST_CHECK_MSG(_suite,
                                  "mutex_destroy_after_use",
                                  rc == D_MUTEX_SUCCESS,
                                  "Destroy after lock/unlock cycle should succeed") && result;
    }

    return result;
}


/*
d_tests_mutex_lock
  Tests the d_mutex_lock function.
  Tests the following:
  - successful lock on an unlocked mutex
  - lock returns D_MUTEX_SUCCESS
  - lock/unlock cycle repeated 100 times
*/
bool d_tests_mutex_lock(
    struct dj_test_suite* _suite
)
{
    bool      result;
    dj_mutex_t mtx;
    int       rc;

    d_test_suite_test(_suite, "mutex_lock");

    result = true;

    // test 1: successful lock
    rc = d_mutex_init(&mtx);

    if (rc == D_MUTEX_SUCCESS)
    {
        rc     = d_mutex_lock(&mtx);
        result = D_TEST_CHECK_MSG(_suite,
                                  "mutex_lock_success",
                                  rc == D_MUTEX_SUCCESS,
                                  "d_mutex_lock should return D_MUTEX_SUCCESS") && result;

        if (rc == D_MUTEX_SUCCESS)
        {
            d_mutex_unlock(&mtx);
        }

        d_mutex_destroy(&mtx);
    }

    // test 2: lock/unlock cycle repeated multiple times
    rc = d_mutex_init(&mtx);

    if (rc == D_MUTEX_SUCCESS)
    {
        int  i;
        bool all_ok;

        all_ok = true;

        for (i = 0; i < 100; i++)
        {
            if (d_mutex_lock(&mtx) != D_MUTEX_SUCCESS)
            {
                all_ok = false;

                break;
            }

            if (d_mutex_unlock(&mtx) != D_MUTEX_SUCCESS)
            {
                all_ok = false;

                break;
            }
        }

        result = D_TEST_CHECK_MSG(_suite,
                                  "mutex_lock_repeated",
                                  all_ok,
                                  "100 lock/unlock cycles should all succeed") && result;

        d_mutex_destroy(&mtx);
    }

    return result;
}


/*
d_tests_mutex_trylock
  Tests the d_mutex_trylock function.
  Tests the following:
  - trylock on an unlocked mutex returns D_MUTEX_SUCCESS
  - trylock on a locked mutex returns D_MUTEX_BUSY (from another thread)
  - trylock/unlock cycle works correctly
*/
bool d_tests_mutex_trylock(
    struct dj_test_suite* _suite
)
{
    bool      result;
    dj_mutex_t mtx;
    int       rc;

    d_test_suite_test(_suite, "mutex_trylock");

    result = true;

    // test 1: trylock on unlocked mutex
    rc = d_mutex_init(&mtx);

    if (rc == D_MUTEX_SUCCESS)
    {
        rc     = d_mutex_trylock(&mtx);
        result = D_TEST_CHECK_MSG(_suite,
                                  "mutex_trylock_unlocked",
                                  rc == D_MUTEX_SUCCESS,
                                  "Trylock on unlocked mutex should return D_MUTEX_SUCCESS") && result;

        if (rc == D_MUTEX_SUCCESS)
        {
            d_mutex_unlock(&mtx);
        }

        d_mutex_destroy(&mtx);
    }

    // test 2: trylock reports BUSY when locked by another thread
    {
        dj_mutex_t                       mtx2;
        struct dj_test_trylock_busy_data shared;
        dj_thread_t                      thd;

        rc = d_mutex_init(&mtx2);

        if (rc == D_MUTEX_SUCCESS)
        {
            shared.mtx            = &mtx2;
            shared.trylock_result = D_MUTEX_SUCCESS;

            // lock from main thread
            d_mutex_lock(&mtx2);

            rc = d_thread_create(&thd,
                                 d_test_trylock_busy_worker,
                                 &shared);

            if (rc == D_MUTEX_SUCCESS)
            {
                d_thread_join(thd, NULL);

                result = D_TEST_CHECK_MSG(_suite,
                                          "mutex_trylock_busy",
                                          shared.trylock_result == D_MUTEX_BUSY,
                                          "Trylock on locked mutex should return " "D_MUTEX_BUSY") && result;
            }

            d_mutex_unlock(&mtx2);
            d_mutex_destroy(&mtx2);
        }
    }

    return result;
}


/*
d_tests_mutex_unlock
  Tests the d_mutex_unlock function.
  Tests the following:
  - successful unlock after lock
  - unlock returns D_MUTEX_SUCCESS
  - mutex is reusable after unlock
*/
bool d_tests_mutex_unlock(
    struct dj_test_suite* _suite
)
{
    bool      result;
    dj_mutex_t mtx;
    int       rc;

    d_test_suite_test(_suite, "mutex_unlock");

    result = true;

    rc = d_mutex_init(&mtx);

    if (rc == D_MUTEX_SUCCESS)
    {
        d_mutex_lock(&mtx);

        // test 1: unlock succeeds
        rc     = d_mutex_unlock(&mtx);
        result = D_TEST_CHECK_MSG(_suite,
                                  "mutex_unlock_success",
                                  rc == D_MUTEX_SUCCESS,
                                  "d_mutex_unlock should return D_MUTEX_SUCCESS") && result;

        // test 2: mutex reusable after unlock
        rc     = d_mutex_lock(&mtx);
        result = D_TEST_CHECK_MSG(_suite,
                                  "mutex_unlock_reusable",
                                  rc == D_MUTEX_SUCCESS,
                                  "Mutex should be lockable again after unlock") && result;

        if (rc == D_MUTEX_SUCCESS)
        {
            d_mutex_unlock(&mtx);
        }

        d_mutex_destroy(&mtx);
    }

    return result;
}


/*
d_tests_mutex_timedlock
  Tests the d_mutex_timedlock function.
  Tests the following:
  - timedlock on unlocked mutex acquires immediately
  - timedlock returns D_MUTEX_SUCCESS on immediate acquisition
  - timedlock with short timeout on locked mutex returns D_MUTEX_TIMEDOUT
*/
bool d_tests_mutex_timedlock(
    struct dj_test_suite* _suite
)
{
    bool            result;
    dj_mutex_t       mtx;
    int             rc;
    struct timespec timeout;

    d_test_suite_test(_suite, "mutex_timedlock");

    result = true;

    // test 1: timedlock on unlocked mutex succeeds immediately
    rc = d_mutex_init(&mtx);

    if (rc == D_MUTEX_SUCCESS)
    {
        d_clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 1;

        rc     = d_mutex_timedlock(&mtx, &timeout);
        result = D_TEST_CHECK_MSG(_suite,
                                  "mutex_timedlock_unlocked",
                                  rc == D_MUTEX_SUCCESS,
                                  "Timedlock on unlocked mutex should succeed") && result;

        if (rc == D_MUTEX_SUCCESS)
        {
            d_mutex_unlock(&mtx);
        }

        d_mutex_destroy(&mtx);
    }

    // test 2: timedlock times out on locked mutex (from another thread)
    {
        dj_mutex_t                    mtx2;
        struct dj_test_timedlock_data shared;
        dj_thread_t                   thd;

        rc = d_mutex_init(&mtx2);

        if (rc == D_MUTEX_SUCCESS)
        {
            shared.mtx              = &mtx2;
            shared.timedlock_result = D_MUTEX_SUCCESS;

            // lock from main thread
            d_mutex_lock(&mtx2);

            rc = d_thread_create(&thd,
                                 d_test_timedlock_worker,
                                 &shared);

            if (rc == D_MUTEX_SUCCESS)
            {
                d_thread_join(thd, NULL);

                result = D_TEST_CHECK_MSG(_suite,
                                          "mutex_timedlock_timeout",
                                          shared.timedlock_result == D_MUTEX_TIMEDOUT,
                                          "Timedlock on locked mutex should time out") && result;
            }

            d_mutex_unlock(&mtx2);
            d_mutex_destroy(&mtx2);
        }
    }

    return result;
}


/*
d_tests_mutex_basic_all
  Aggregation function that runs all basic mutex tests.
*/
bool d_tests_mutex_basic_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "mutex_basic_all");

    result = true;

    result = d_tests_mutex_init(_suite) && result;
    result = d_tests_mutex_destroy(_suite) && result;
    result = d_tests_mutex_lock(_suite) && result;
    result = d_tests_mutex_trylock(_suite) && result;
    result = d_tests_mutex_unlock(_suite) && result;
    result = d_tests_mutex_timedlock(_suite) && result;

    return result;
}
