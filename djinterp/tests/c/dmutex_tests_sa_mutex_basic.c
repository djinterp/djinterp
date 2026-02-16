#include ".\dmutex_tests_sa.h"


///////////////////////////////////////////////////////////////////////////////
// file-scope helper structs and worker functions for threaded tests
///////////////////////////////////////////////////////////////////////////////

// helper struct for trylock-busy test
struct d_test_trylock_busy_data
{
    d_mutex_t* mtx;
    int        trylock_result;
};

// helper: worker that attempts trylock on an already-locked mutex
static void*
d_test_trylock_busy_worker
(
    void* _arg
)
{
    struct d_test_trylock_busy_data* data;

    data = (struct d_test_trylock_busy_data*)_arg;
    data->trylock_result = d_mutex_trylock(data->mtx);

    // if we somehow got it, release it
    if (data->trylock_result == D_MUTEX_SUCCESS)
    {
        d_mutex_unlock(data->mtx);
    }

    return D_THREAD_SUCCESS;
}


// helper struct for timedlock test
struct d_test_timedlock_data
{
    d_mutex_t* mtx;
    int        timedlock_result;
};

// helper: worker that attempts timedlock with a short timeout
static void*
d_test_timedlock_worker
(
    void* _arg
)
{
    struct d_test_timedlock_data* data;
    struct timespec              ts;

    data = (struct d_test_timedlock_data*)_arg;

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
d_tests_sa_mutex_init
  Tests the d_mutex_init function.
  Tests the following:
  - successful initialization returns D_MUTEX_SUCCESS
  - initialized mutex is usable (can lock/unlock)
  - multiple mutexes can be initialized independently
*/
bool
d_tests_sa_mutex_init
(
    struct d_test_counter* _counter
)
{
    bool      result;
    d_mutex_t mtx;
    int       rc;

    result = true;

    // test 1: successful initialization
    rc     = d_mutex_init(&mtx);
    result = d_assert_standalone(
        rc == D_MUTEX_SUCCESS,
        "mutex_init_success",
        "d_mutex_init should return D_MUTEX_SUCCESS",
        _counter) && result;

    // test 2: initialized mutex is usable
    if (rc == D_MUTEX_SUCCESS)
    {
        int lock_rc;

        lock_rc = d_mutex_lock(&mtx);
        result  = d_assert_standalone(
            lock_rc == D_MUTEX_SUCCESS,
            "mutex_init_usable_lock",
            "Initialized mutex should be lockable",
            _counter) && result;

        if (lock_rc == D_MUTEX_SUCCESS)
        {
            d_mutex_unlock(&mtx);
        }

        d_mutex_destroy(&mtx);
    }

    // test 3: multiple mutexes initialized independently
    {
        d_mutex_t mtx_a;
        d_mutex_t mtx_b;
        int       rc_a;
        int       rc_b;

        rc_a = d_mutex_init(&mtx_a);
        rc_b = d_mutex_init(&mtx_b);

        result = d_assert_standalone(
            rc_a == D_MUTEX_SUCCESS && rc_b == D_MUTEX_SUCCESS,
            "mutex_init_multiple",
            "Multiple mutexes should initialize independently",
            _counter) && result;

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
d_tests_sa_mutex_destroy
  Tests the d_mutex_destroy function.
  Tests the following:
  - successful destruction of an initialized mutex
  - destruction returns D_MUTEX_SUCCESS
  - destroy after init/lock/unlock cycle
*/
bool
d_tests_sa_mutex_destroy
(
    struct d_test_counter* _counter
)
{
    bool      result;
    d_mutex_t mtx;
    int       rc;

    result = true;

    // test 1: basic init then destroy
    rc = d_mutex_init(&mtx);

    if (rc == D_MUTEX_SUCCESS)
    {
        rc     = d_mutex_destroy(&mtx);
        result = d_assert_standalone(
            rc == D_MUTEX_SUCCESS,
            "mutex_destroy_success",
            "d_mutex_destroy should return D_MUTEX_SUCCESS",
            _counter) && result;
    }

    // test 2: destroy after lock/unlock cycle
    rc = d_mutex_init(&mtx);

    if (rc == D_MUTEX_SUCCESS)
    {
        d_mutex_lock(&mtx);
        d_mutex_unlock(&mtx);

        rc     = d_mutex_destroy(&mtx);
        result = d_assert_standalone(
            rc == D_MUTEX_SUCCESS,
            "mutex_destroy_after_use",
            "Destroy after lock/unlock cycle should succeed",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_mutex_lock
  Tests the d_mutex_lock function.
  Tests the following:
  - successful lock on an unlocked mutex
  - lock returns D_MUTEX_SUCCESS
  - lock/unlock cycle repeated 100 times
*/
bool
d_tests_sa_mutex_lock
(
    struct d_test_counter* _counter
)
{
    bool      result;
    d_mutex_t mtx;
    int       rc;

    result = true;

    // test 1: successful lock
    rc = d_mutex_init(&mtx);

    if (rc == D_MUTEX_SUCCESS)
    {
        rc     = d_mutex_lock(&mtx);
        result = d_assert_standalone(
            rc == D_MUTEX_SUCCESS,
            "mutex_lock_success",
            "d_mutex_lock should return D_MUTEX_SUCCESS",
            _counter) && result;

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

        result = d_assert_standalone(
            all_ok,
            "mutex_lock_repeated",
            "100 lock/unlock cycles should all succeed",
            _counter) && result;

        d_mutex_destroy(&mtx);
    }

    return result;
}


/*
d_tests_sa_mutex_trylock
  Tests the d_mutex_trylock function.
  Tests the following:
  - trylock on an unlocked mutex returns D_MUTEX_SUCCESS
  - trylock on a locked mutex returns D_MUTEX_BUSY (from another thread)
  - trylock/unlock cycle works correctly
*/
bool
d_tests_sa_mutex_trylock
(
    struct d_test_counter* _counter
)
{
    bool      result;
    d_mutex_t mtx;
    int       rc;

    result = true;

    // test 1: trylock on unlocked mutex
    rc = d_mutex_init(&mtx);

    if (rc == D_MUTEX_SUCCESS)
    {
        rc     = d_mutex_trylock(&mtx);
        result = d_assert_standalone(
            rc == D_MUTEX_SUCCESS,
            "mutex_trylock_unlocked",
            "Trylock on unlocked mutex should return D_MUTEX_SUCCESS",
            _counter) && result;

        if (rc == D_MUTEX_SUCCESS)
        {
            d_mutex_unlock(&mtx);
        }

        d_mutex_destroy(&mtx);
    }

    // test 2: trylock reports BUSY when locked by another thread
    {
        d_mutex_t                       mtx2;
        struct d_test_trylock_busy_data shared;
        d_thread_t                      thd;

        rc = d_mutex_init(&mtx2);

        if (rc == D_MUTEX_SUCCESS)
        {
            shared.mtx            = &mtx2;
            shared.trylock_result = D_MUTEX_SUCCESS;

            // lock from main thread
            d_mutex_lock(&mtx2);

            rc = d_thread_create(&thd,
                                 (d_thread_func_t)d_test_trylock_busy_worker,
                                 &shared);

            if (rc == D_MUTEX_SUCCESS)
            {
                d_thread_join(thd, NULL);

                result = d_assert_standalone(
                    shared.trylock_result == D_MUTEX_BUSY,
                    "mutex_trylock_busy",
                    "Trylock on locked mutex should return "
                    "D_MUTEX_BUSY",
                    _counter) && result;
            }

            d_mutex_unlock(&mtx2);
            d_mutex_destroy(&mtx2);
        }
    }

    return result;
}


/*
d_tests_sa_mutex_unlock
  Tests the d_mutex_unlock function.
  Tests the following:
  - successful unlock after lock
  - unlock returns D_MUTEX_SUCCESS
  - mutex is reusable after unlock
*/
bool
d_tests_sa_mutex_unlock
(
    struct d_test_counter* _counter
)
{
    bool      result;
    d_mutex_t mtx;
    int       rc;

    result = true;

    rc = d_mutex_init(&mtx);

    if (rc == D_MUTEX_SUCCESS)
    {
        d_mutex_lock(&mtx);

        // test 1: unlock succeeds
        rc     = d_mutex_unlock(&mtx);
        result = d_assert_standalone(
            rc == D_MUTEX_SUCCESS,
            "mutex_unlock_success",
            "d_mutex_unlock should return D_MUTEX_SUCCESS",
            _counter) && result;

        // test 2: mutex reusable after unlock
        rc     = d_mutex_lock(&mtx);
        result = d_assert_standalone(
            rc == D_MUTEX_SUCCESS,
            "mutex_unlock_reusable",
            "Mutex should be lockable again after unlock",
            _counter) && result;

        if (rc == D_MUTEX_SUCCESS)
        {
            d_mutex_unlock(&mtx);
        }

        d_mutex_destroy(&mtx);
    }

    return result;
}


/*
d_tests_sa_mutex_timedlock
  Tests the d_mutex_timedlock function.
  Tests the following:
  - timedlock on unlocked mutex acquires immediately
  - timedlock returns D_MUTEX_SUCCESS on immediate acquisition
  - timedlock with short timeout on locked mutex returns D_MUTEX_TIMEDOUT
*/
bool
d_tests_sa_mutex_timedlock
(
    struct d_test_counter* _counter
)
{
    bool            result;
    d_mutex_t       mtx;
    int             rc;
    struct timespec timeout;

    result = true;

    // test 1: timedlock on unlocked mutex succeeds immediately
    rc = d_mutex_init(&mtx);

    if (rc == D_MUTEX_SUCCESS)
    {
        d_clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 1;

        rc     = d_mutex_timedlock(&mtx, &timeout);
        result = d_assert_standalone(
            rc == D_MUTEX_SUCCESS,
            "mutex_timedlock_unlocked",
            "Timedlock on unlocked mutex should succeed",
            _counter) && result;

        if (rc == D_MUTEX_SUCCESS)
        {
            d_mutex_unlock(&mtx);
        }

        d_mutex_destroy(&mtx);
    }

    // test 2: timedlock times out on locked mutex (from another thread)
    {
        d_mutex_t                    mtx2;
        struct d_test_timedlock_data shared;
        d_thread_t                   thd;

        rc = d_mutex_init(&mtx2);

        if (rc == D_MUTEX_SUCCESS)
        {
            shared.mtx              = &mtx2;
            shared.timedlock_result = D_MUTEX_SUCCESS;

            // lock from main thread
            d_mutex_lock(&mtx2);

            rc = d_thread_create(&thd,
                                 (d_thread_func_t)d_test_timedlock_worker,
                                 &shared);

            if (rc == D_MUTEX_SUCCESS)
            {
                d_thread_join(thd, NULL);

                result = d_assert_standalone(
                    shared.timedlock_result == D_MUTEX_TIMEDOUT,
                    "mutex_timedlock_timeout",
                    "Timedlock on locked mutex should time out",
                    _counter) && result;
            }

            d_mutex_unlock(&mtx2);
            d_mutex_destroy(&mtx2);
        }
    }

    return result;
}


/*
d_tests_sa_mutex_basic_all
  Aggregation function that runs all basic mutex tests.
*/
bool
d_tests_sa_mutex_basic_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Basic Mutex Operations\n");
    printf("  ----------------------------------\n");

    result = d_tests_sa_mutex_init(_counter) && result;
    result = d_tests_sa_mutex_destroy(_counter) && result;
    result = d_tests_sa_mutex_lock(_counter) && result;
    result = d_tests_sa_mutex_trylock(_counter) && result;
    result = d_tests_sa_mutex_unlock(_counter) && result;
    result = d_tests_sa_mutex_timedlock(_counter) && result;

    return result;
}
