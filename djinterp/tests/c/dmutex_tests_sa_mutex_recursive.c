#include ".\dmutex_tests_sa.h"


///////////////////////////////////////////////////////////////////////////////
// file-scope helper structs and worker functions for threaded tests
///////////////////////////////////////////////////////////////////////////////

// helper struct for reentrant release verification
struct d_test_reentrant_data
{
    d_recursive_mutex_t* mtx;
    bool                 acquired;
};

// helper: worker that tries to acquire a recursive mutex from another thread
static void*
d_test_reentrant_worker
(
    void* _arg
)
{
    struct d_test_reentrant_data* data;

    data = (struct d_test_reentrant_data*)_arg;

    if (d_recursive_mutex_trylock(data->mtx) == D_MUTEX_SUCCESS)
    {
        data->acquired = true;
        d_recursive_mutex_unlock(data->mtx);
    }

    return D_THREAD_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
// test functions
///////////////////////////////////////////////////////////////////////////////


/*
d_tests_sa_recursive_mutex_init
  Tests the d_recursive_mutex_init function.
  Tests the following:
  - successful initialization returns D_MUTEX_SUCCESS
  - initialized recursive mutex is usable
*/
bool
d_tests_sa_recursive_mutex_init
(
    struct d_test_counter* _counter
)
{
    bool                result;
    d_recursive_mutex_t rmtx;
    int                 rc;

    result = true;

    // test 1: successful initialization
    rc     = d_recursive_mutex_init(&rmtx);
    result = d_assert_standalone(
        rc == D_MUTEX_SUCCESS,
        "recursive_init_success",
        "d_recursive_mutex_init should return D_MUTEX_SUCCESS",
        _counter) && result;

    // test 2: initialized mutex is usable
    if (rc == D_MUTEX_SUCCESS)
    {
        int lock_rc;

        lock_rc = d_recursive_mutex_lock(&rmtx);
        result  = d_assert_standalone(
            lock_rc == D_MUTEX_SUCCESS,
            "recursive_init_usable",
            "Initialized recursive mutex should be lockable",
            _counter) && result;

        if (lock_rc == D_MUTEX_SUCCESS)
        {
            d_recursive_mutex_unlock(&rmtx);
        }

        d_recursive_mutex_destroy(&rmtx);
    }

    return result;
}


/*
d_tests_sa_recursive_mutex_destroy
  Tests the d_recursive_mutex_destroy function.
  Tests the following:
  - successful destruction returns D_MUTEX_SUCCESS
  - destroy after lock/unlock cycle
*/
bool
d_tests_sa_recursive_mutex_destroy
(
    struct d_test_counter* _counter
)
{
    bool                result;
    d_recursive_mutex_t rmtx;
    int                 rc;

    result = true;

    // test 1: basic init then destroy
    rc = d_recursive_mutex_init(&rmtx);

    if (rc == D_MUTEX_SUCCESS)
    {
        rc     = d_recursive_mutex_destroy(&rmtx);
        result = d_assert_standalone(
            rc == D_MUTEX_SUCCESS,
            "recursive_destroy_success",
            "d_recursive_mutex_destroy should return D_MUTEX_SUCCESS",
            _counter) && result;
    }

    // test 2: destroy after use
    rc = d_recursive_mutex_init(&rmtx);

    if (rc == D_MUTEX_SUCCESS)
    {
        d_recursive_mutex_lock(&rmtx);
        d_recursive_mutex_unlock(&rmtx);

        rc     = d_recursive_mutex_destroy(&rmtx);
        result = d_assert_standalone(
            rc == D_MUTEX_SUCCESS,
            "recursive_destroy_after_use",
            "Destroy after lock/unlock should succeed",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_recursive_mutex_lock
  Tests the d_recursive_mutex_lock function.
  Tests the following:
  - successful lock returns D_MUTEX_SUCCESS
  - lock/unlock cycle
*/
bool
d_tests_sa_recursive_mutex_lock
(
    struct d_test_counter* _counter
)
{
    bool                result;
    d_recursive_mutex_t rmtx;
    int                 rc;

    result = true;

    rc = d_recursive_mutex_init(&rmtx);

    if (rc == D_MUTEX_SUCCESS)
    {
        rc     = d_recursive_mutex_lock(&rmtx);
        result = d_assert_standalone(
            rc == D_MUTEX_SUCCESS,
            "recursive_lock_success",
            "d_recursive_mutex_lock should return D_MUTEX_SUCCESS",
            _counter) && result;

        if (rc == D_MUTEX_SUCCESS)
        {
            d_recursive_mutex_unlock(&rmtx);
        }

        d_recursive_mutex_destroy(&rmtx);
    }

    return result;
}


/*
d_tests_sa_recursive_mutex_trylock
  Tests the d_recursive_mutex_trylock function.
  Tests the following:
  - trylock on unlocked recursive mutex succeeds
  - trylock on already-locked (by same thread) recursive mutex succeeds
*/
bool
d_tests_sa_recursive_mutex_trylock
(
    struct d_test_counter* _counter
)
{
    bool                result;
    d_recursive_mutex_t rmtx;
    int                 rc;

    result = true;

    rc = d_recursive_mutex_init(&rmtx);

    if (rc == D_MUTEX_SUCCESS)
    {
        // test 1: trylock on unlocked mutex
        rc     = d_recursive_mutex_trylock(&rmtx);
        result = d_assert_standalone(
            rc == D_MUTEX_SUCCESS,
            "recursive_trylock_unlocked",
            "Trylock on unlocked recursive mutex should succeed",
            _counter) && result;

        // test 2: trylock again (re-entrant) should also succeed
        if (rc == D_MUTEX_SUCCESS)
        {
            int rc2;

            rc2    = d_recursive_mutex_trylock(&rmtx);
            result = d_assert_standalone(
                rc2 == D_MUTEX_SUCCESS,
                "recursive_trylock_reentrant",
                "Recursive trylock should succeed on same thread",
                _counter) && result;

            if (rc2 == D_MUTEX_SUCCESS)
            {
                d_recursive_mutex_unlock(&rmtx);
            }

            d_recursive_mutex_unlock(&rmtx);
        }

        d_recursive_mutex_destroy(&rmtx);
    }

    return result;
}


/*
d_tests_sa_recursive_mutex_unlock
  Tests the d_recursive_mutex_unlock function.
  Tests the following:
  - successful unlock returns D_MUTEX_SUCCESS
  - mutex usable after unlock
*/
bool
d_tests_sa_recursive_mutex_unlock
(
    struct d_test_counter* _counter
)
{
    bool                result;
    d_recursive_mutex_t rmtx;
    int                 rc;

    result = true;

    rc = d_recursive_mutex_init(&rmtx);

    if (rc == D_MUTEX_SUCCESS)
    {
        d_recursive_mutex_lock(&rmtx);

        // test 1: unlock succeeds
        rc     = d_recursive_mutex_unlock(&rmtx);
        result = d_assert_standalone(
            rc == D_MUTEX_SUCCESS,
            "recursive_unlock_success",
            "d_recursive_mutex_unlock should return D_MUTEX_SUCCESS",
            _counter) && result;

        // test 2: mutex reusable after unlock
        rc     = d_recursive_mutex_lock(&rmtx);
        result = d_assert_standalone(
            rc == D_MUTEX_SUCCESS,
            "recursive_unlock_reusable",
            "Recursive mutex should be lockable after unlock",
            _counter) && result;

        if (rc == D_MUTEX_SUCCESS)
        {
            d_recursive_mutex_unlock(&rmtx);
        }

        d_recursive_mutex_destroy(&rmtx);
    }

    return result;
}


/*
d_tests_sa_recursive_mutex_reentrant
  Tests recursive (re-entrant) locking behavior.
  Tests the following:
  - same thread can lock recursive mutex multiple times
  - must unlock the same number of times to fully release
  - another thread can acquire after all unlocks complete
*/
bool
d_tests_sa_recursive_mutex_reentrant
(
    struct d_test_counter* _counter
)
{
    bool                result;
    d_recursive_mutex_t rmtx;
    int                 rc;

    result = true;

    rc = d_recursive_mutex_init(&rmtx);

    if (rc == D_MUTEX_SUCCESS)
    {
        int  i;
        bool all_locked;
        bool all_unlocked;

        // test 1: lock 5 times recursively
        all_locked = true;

        for (i = 0; i < 5; i++)
        {
            if (d_recursive_mutex_lock(&rmtx) != D_MUTEX_SUCCESS)
            {
                all_locked = false;

                break;
            }
        }

        result = d_assert_standalone(
            all_locked,
            "recursive_reentrant_lock_5",
            "Should be able to lock recursive mutex 5 times",
            _counter) && result;

        // test 2: unlock 5 times
        all_unlocked = true;

        for (i = 0; i < 5; i++)
        {
            if (d_recursive_mutex_unlock(&rmtx) != D_MUTEX_SUCCESS)
            {
                all_unlocked = false;

                break;
            }
        }

        result = d_assert_standalone(
            all_unlocked,
            "recursive_reentrant_unlock_5",
            "Should be able to unlock recursive mutex 5 times",
            _counter) && result;

        // test 3: verify fully released by acquiring from another thread
        {
            struct d_test_reentrant_data shared;
            d_thread_t                   thd;

            shared.mtx      = &rmtx;
            shared.acquired = false;

            rc = d_thread_create(&thd,
                                 (d_thread_func_t)d_test_reentrant_worker,
                                 &shared);

            if (rc == D_MUTEX_SUCCESS)
            {
                d_thread_join(thd, NULL);

                result = d_assert_standalone(
                    shared.acquired,
                    "recursive_reentrant_released",
                    "Another thread should acquire after all "
                    "unlocks",
                    _counter) && result;
            }
        }

        d_recursive_mutex_destroy(&rmtx);
    }

    return result;
}


/*
d_tests_sa_mutex_recursive_all
  Aggregation function that runs all recursive mutex tests.
*/
bool
d_tests_sa_mutex_recursive_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Recursive Mutex Operations\n");
    printf("  --------------------------------------\n");

    result = d_tests_sa_recursive_mutex_init(_counter) && result;
    result = d_tests_sa_recursive_mutex_destroy(_counter) && result;
    result = d_tests_sa_recursive_mutex_lock(_counter) && result;
    result = d_tests_sa_recursive_mutex_trylock(_counter) && result;
    result = d_tests_sa_recursive_mutex_unlock(_counter) && result;
    result = d_tests_sa_recursive_mutex_reentrant(_counter) && result;

    return result;
}
