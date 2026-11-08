/*******************************************************************************
* djinterp [test]                                 dmutex_tests_mutex_recursive.c
*
*
* path:      /tests/djinterp/c/sync/dmutex_tests_mutex_recursive.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.15
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "dmutex_tests.h"


///////////////////////////////////////////////////////////////////////////////
// file-scope helper structs and worker functions for threaded tests
///////////////////////////////////////////////////////////////////////////////

// helper struct for reentrant release verification
struct dj_test_reentrant_data
{
    dj_recursive_mutex_t* mtx;
    bool                 acquired;
};

// helper: worker that tries to acquire a recursive mutex from another thread
static
dj_thread_result_t d_test_reentrant_worker(
    void* _arg
)
{
    struct dj_test_reentrant_data* data;

    data = (struct dj_test_reentrant_data*)_arg;

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
d_tests_recursive_mutex_init
  Tests the d_recursive_mutex_init function.
  Tests the following:
  - successful initialization returns D_MUTEX_SUCCESS
  - initialized recursive mutex is usable
*/
bool d_tests_recursive_mutex_init(
    struct dj_test_suite* _suite
)
{
    bool                result;
    dj_recursive_mutex_t rmtx;
    int                 rc;

    d_test_suite_test(_suite, "recursive_mutex_init");

    result = true;

    // test 1: successful initialization
    rc     = d_recursive_mutex_init(&rmtx);
    result = D_TEST_CHECK_MSG(_suite,
                              "recursive_init_success",
                              rc == D_MUTEX_SUCCESS,
                              "d_recursive_mutex_init should return D_MUTEX_SUCCESS") && result;

    // test 2: initialized mutex is usable
    if (rc == D_MUTEX_SUCCESS)
    {
        int lock_rc;

        lock_rc = d_recursive_mutex_lock(&rmtx);
        result  = D_TEST_CHECK_MSG(_suite,
                                   "recursive_init_usable",
                                   lock_rc == D_MUTEX_SUCCESS,
                                   "Initialized recursive mutex should be lockable") && result;

        if (lock_rc == D_MUTEX_SUCCESS)
        {
            d_recursive_mutex_unlock(&rmtx);
        }

        d_recursive_mutex_destroy(&rmtx);
    }

    return result;
}


/*
d_tests_recursive_mutex_destroy
  Tests the d_recursive_mutex_destroy function.
  Tests the following:
  - successful destruction returns D_MUTEX_SUCCESS
  - destroy after lock/unlock cycle
*/
bool d_tests_recursive_mutex_destroy(
    struct dj_test_suite* _suite
)
{
    bool                result;
    dj_recursive_mutex_t rmtx;
    int                 rc;

    d_test_suite_test(_suite, "recursive_mutex_destroy");

    result = true;

    // test 1: basic init then destroy
    rc = d_recursive_mutex_init(&rmtx);

    if (rc == D_MUTEX_SUCCESS)
    {
        rc     = d_recursive_mutex_destroy(&rmtx);
        result = D_TEST_CHECK_MSG(_suite,
                                  "recursive_destroy_success",
                                  rc == D_MUTEX_SUCCESS,
                                  "d_recursive_mutex_destroy should return D_MUTEX_SUCCESS") && result;
    }

    // test 2: destroy after use
    rc = d_recursive_mutex_init(&rmtx);

    if (rc == D_MUTEX_SUCCESS)
    {
        d_recursive_mutex_lock(&rmtx);
        d_recursive_mutex_unlock(&rmtx);

        rc     = d_recursive_mutex_destroy(&rmtx);
        result = D_TEST_CHECK_MSG(_suite,
                                  "recursive_destroy_after_use",
                                  rc == D_MUTEX_SUCCESS,
                                  "Destroy after lock/unlock should succeed") && result;
    }

    return result;
}


/*
d_tests_recursive_mutex_lock
  Tests the d_recursive_mutex_lock function.
  Tests the following:
  - successful lock returns D_MUTEX_SUCCESS
  - lock/unlock cycle
*/
bool d_tests_recursive_mutex_lock(
    struct dj_test_suite* _suite
)
{
    bool                result;
    dj_recursive_mutex_t rmtx;
    int                 rc;

    d_test_suite_test(_suite, "recursive_mutex_lock");

    result = true;

    rc = d_recursive_mutex_init(&rmtx);

    if (rc == D_MUTEX_SUCCESS)
    {
        rc     = d_recursive_mutex_lock(&rmtx);
        result = D_TEST_CHECK_MSG(_suite,
                                  "recursive_lock_success",
                                  rc == D_MUTEX_SUCCESS,
                                  "d_recursive_mutex_lock should return D_MUTEX_SUCCESS") && result;

        if (rc == D_MUTEX_SUCCESS)
        {
            d_recursive_mutex_unlock(&rmtx);
        }

        d_recursive_mutex_destroy(&rmtx);
    }

    return result;
}


/*
d_tests_recursive_mutex_trylock
  Tests the d_recursive_mutex_trylock function.
  Tests the following:
  - trylock on unlocked recursive mutex succeeds
  - trylock on already-locked (by same thread) recursive mutex succeeds
*/
bool d_tests_recursive_mutex_trylock(
    struct dj_test_suite* _suite
)
{
    bool                result;
    dj_recursive_mutex_t rmtx;
    int                 rc;

    d_test_suite_test(_suite, "recursive_mutex_trylock");

    result = true;

    rc = d_recursive_mutex_init(&rmtx);

    if (rc == D_MUTEX_SUCCESS)
    {
        // test 1: trylock on unlocked mutex
        rc     = d_recursive_mutex_trylock(&rmtx);
        result = D_TEST_CHECK_MSG(_suite,
                                  "recursive_trylock_unlocked",
                                  rc == D_MUTEX_SUCCESS,
                                  "Trylock on unlocked recursive mutex should succeed") && result;

        // test 2: trylock again (re-entrant) should also succeed
        if (rc == D_MUTEX_SUCCESS)
        {
            int rc2;

            rc2    = d_recursive_mutex_trylock(&rmtx);
            result = D_TEST_CHECK_MSG(_suite,
                                      "recursive_trylock_reentrant",
                                      rc2 == D_MUTEX_SUCCESS,
                                      "Recursive trylock should succeed on same thread") && result;

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
d_tests_recursive_mutex_unlock
  Tests the d_recursive_mutex_unlock function.
  Tests the following:
  - successful unlock returns D_MUTEX_SUCCESS
  - mutex usable after unlock
*/
bool d_tests_recursive_mutex_unlock(
    struct dj_test_suite* _suite
)
{
    bool                result;
    dj_recursive_mutex_t rmtx;
    int                 rc;

    d_test_suite_test(_suite, "recursive_mutex_unlock");

    result = true;

    rc = d_recursive_mutex_init(&rmtx);

    if (rc == D_MUTEX_SUCCESS)
    {
        d_recursive_mutex_lock(&rmtx);

        // test 1: unlock succeeds
        rc     = d_recursive_mutex_unlock(&rmtx);
        result = D_TEST_CHECK_MSG(_suite,
                                  "recursive_unlock_success",
                                  rc == D_MUTEX_SUCCESS,
                                  "d_recursive_mutex_unlock should return D_MUTEX_SUCCESS") && result;

        // test 2: mutex reusable after unlock
        rc     = d_recursive_mutex_lock(&rmtx);
        result = D_TEST_CHECK_MSG(_suite,
                                  "recursive_unlock_reusable",
                                  rc == D_MUTEX_SUCCESS,
                                  "Recursive mutex should be lockable after unlock") && result;

        if (rc == D_MUTEX_SUCCESS)
        {
            d_recursive_mutex_unlock(&rmtx);
        }

        d_recursive_mutex_destroy(&rmtx);
    }

    return result;
}


/*
d_tests_recursive_mutex_reentrant
  Tests recursive (re-entrant) locking behavior.
  Tests the following:
  - same thread can lock recursive mutex multiple times
  - must unlock the same number of times to fully release
  - another thread can acquire after all unlocks complete
*/
bool d_tests_recursive_mutex_reentrant(
    struct dj_test_suite* _suite
)
{
    bool                result;
    dj_recursive_mutex_t rmtx;
    int                 rc;

    d_test_suite_test(_suite, "recursive_mutex_reentrant");

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

        result = D_TEST_CHECK_MSG(_suite,
                                  "recursive_reentrant_lock_5",
                                  all_locked,
                                  "Should be able to lock recursive mutex 5 times") && result;

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

        result = D_TEST_CHECK_MSG(_suite,
                                  "recursive_reentrant_unlock_5",
                                  all_unlocked,
                                  "Should be able to unlock recursive mutex 5 times") && result;

        // test 3: verify fully released by acquiring from another thread
        {
            struct dj_test_reentrant_data shared;
            dj_thread_t                   thd;

            shared.mtx      = &rmtx;
            shared.acquired = false;

            rc = d_thread_create(&thd,
                                 d_test_reentrant_worker,
                                 &shared);

            if (rc == D_MUTEX_SUCCESS)
            {
                d_thread_join(thd, NULL);

                result = D_TEST_CHECK_MSG(_suite,
                                          "recursive_reentrant_released",
                                          shared.acquired,
                                          "Another thread should acquire after all " "unlocks") && result;
            }
        }

        d_recursive_mutex_destroy(&rmtx);
    }

    return result;
}


/*
d_tests_mutex_recursive_all
  Aggregation function that runs all recursive mutex tests.
*/
bool d_tests_mutex_recursive_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "mutex_recursive_all");

    result = true;

    result = d_tests_recursive_mutex_init(_suite) && result;
    result = d_tests_recursive_mutex_destroy(_suite) && result;
    result = d_tests_recursive_mutex_lock(_suite) && result;
    result = d_tests_recursive_mutex_trylock(_suite) && result;
    result = d_tests_recursive_mutex_unlock(_suite) && result;
    result = d_tests_recursive_mutex_reentrant(_suite) && result;

    return result;
}
