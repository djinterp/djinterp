/*******************************************************************************
* djinterp [c][sync]                                       dmutex_tests_rwlock.c
*
*
* path:      /tests/djinterp/c/sync/dmutex_tests_rwlock.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.15
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "dmutex_tests.h"


///////////////////////////////////////////////////////////////////////////////
// file-scope helper structs and worker functions
///////////////////////////////////////////////////////////////////////////////

struct dj_test_tryrd_data
{
    struct dj_rwlock_t* rw;
    int                result_code;
};

static
dj_thread_result_t d_test_tryrd_worker(
    void* _arg
)
{
    struct dj_test_tryrd_data* data;
    data              = (struct dj_test_tryrd_data*)_arg;
    data->result_code = d_rwlock_tryrdlock(data->rw);
    if (data->result_code == D_MUTEX_SUCCESS)
    {
        d_rwlock_unlock(data->rw);
    }
    return D_THREAD_SUCCESS;
}

struct dj_test_trywr_data
{
    struct dj_rwlock_t* rw;
    int                result_code;
};

static
dj_thread_result_t d_test_trywr_worker(
    void* _arg
)
{
    struct dj_test_trywr_data* data;
    data              = (struct dj_test_trywr_data*)_arg;
    data->result_code = d_rwlock_trywrlock(data->rw);
    if (data->result_code == D_MUTEX_SUCCESS)
    {
        d_rwlock_unlock(data->rw);
    }
    return D_THREAD_SUCCESS;
}

struct dj_test_reader_data
{
    struct dj_rwlock_t* rw;
    const int*         shared_value;
    int                read_value;
    bool               acquired;
};

static
dj_thread_result_t d_test_reader_thread(
    void* _arg
)
{
    struct dj_test_reader_data* data;
    struct timespec            dur;
    data = (struct dj_test_reader_data*)_arg;
    if (d_rwlock_rdlock(data->rw) == D_MUTEX_SUCCESS)
    {
        data->read_value = *(data->shared_value);
        data->acquired   = true;
        dur.tv_sec  = 0;
        dur.tv_nsec = 10 * D_TIME_NSEC_PER_MSEC;
        d_nanosleep(&dur, NULL);
        d_rwlock_unlock(data->rw);
    }
    return D_THREAD_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
// test functions
///////////////////////////////////////////////////////////////////////////////


/*
d_tests_rwlock_init_destroy
  Tests the d_rwlock_init and d_rwlock_destroy functions.
  Tests the following:
  - successful init returns D_MUTEX_SUCCESS
  - successful destroy returns D_MUTEX_SUCCESS
  - multiple rwlocks can be initialized
*/
bool d_tests_rwlock_init_destroy(
    struct dj_test_suite* _suite
)
{
    bool              result;
    struct dj_rwlock_t rwlock;
    int               rc;

    d_test_suite_test(_suite, "rwlock_init_destroy");

    result = true;

    rc     = d_rwlock_init(&rwlock);
    result = D_TEST_CHECK_MSG(_suite,
                              "rwlock_init_success",
                              rc == D_MUTEX_SUCCESS,
                              "d_rwlock_init should return D_MUTEX_SUCCESS") && result;

    if (rc == D_MUTEX_SUCCESS)
    {
        rc     = d_rwlock_destroy(&rwlock);
        result = D_TEST_CHECK_MSG(_suite,
                                  "rwlock_destroy_success",
                                  rc == D_MUTEX_SUCCESS,
                                  "d_rwlock_destroy should return D_MUTEX_SUCCESS") && result;
    }

    {
        struct dj_rwlock_t rw_a;
        struct dj_rwlock_t rw_b;
        int               rc_a;
        int               rc_b;

        rc_a = d_rwlock_init(&rw_a);
        rc_b = d_rwlock_init(&rw_b);

        result = D_TEST_CHECK_MSG(_suite,
                                  "rwlock_init_multiple",
                                  rc_a == D_MUTEX_SUCCESS && rc_b == D_MUTEX_SUCCESS,
                                  "Multiple rwlocks should initialize independently") && result;

        if (rc_a == D_MUTEX_SUCCESS) { d_rwlock_destroy(&rw_a); }
        if (rc_b == D_MUTEX_SUCCESS) { d_rwlock_destroy(&rw_b); }
    }

    return result;
}


/*
d_tests_rwlock_rdlock
  Tests the d_rwlock_rdlock function.
  Tests the following:
  - read lock succeeds on unlocked rwlock
  - unlock after rdlock succeeds
*/
bool d_tests_rwlock_rdlock(
    struct dj_test_suite* _suite
)
{
    bool              result;
    struct dj_rwlock_t rwlock;
    int               rc;

    d_test_suite_test(_suite, "rwlock_rdlock");

    result = true;

    rc = d_rwlock_init(&rwlock);

    if (rc == D_MUTEX_SUCCESS)
    {
        rc     = d_rwlock_rdlock(&rwlock);
        result = D_TEST_CHECK_MSG(_suite,
                                  "rwlock_rdlock_success",
                                  rc == D_MUTEX_SUCCESS,
                                  "d_rwlock_rdlock should return D_MUTEX_SUCCESS") && result;

        if (rc == D_MUTEX_SUCCESS)
        {
            rc     = d_rwlock_unlock(&rwlock);
            result = D_TEST_CHECK_MSG(_suite,
                                      "rwlock_rdlock_unlock",
                                      rc == D_MUTEX_SUCCESS,
                                      "Unlock after rdlock should succeed") && result;
        }

        d_rwlock_destroy(&rwlock);
    }

    return result;
}


/*
d_tests_rwlock_wrlock
  Tests the d_rwlock_wrlock function.
  Tests the following:
  - write lock succeeds on unlocked rwlock
  - unlock after wrlock succeeds
*/
bool d_tests_rwlock_wrlock(
    struct dj_test_suite* _suite
)
{
    bool              result;
    struct dj_rwlock_t rwlock;
    int               rc;

    d_test_suite_test(_suite, "rwlock_wrlock");

    result = true;

    rc = d_rwlock_init(&rwlock);

    if (rc == D_MUTEX_SUCCESS)
    {
        rc     = d_rwlock_wrlock(&rwlock);
        result = D_TEST_CHECK_MSG(_suite,
                                  "rwlock_wrlock_success",
                                  rc == D_MUTEX_SUCCESS,
                                  "d_rwlock_wrlock should return D_MUTEX_SUCCESS") && result;

        if (rc == D_MUTEX_SUCCESS)
        {
            rc     = d_rwlock_unlock(&rwlock);
            result = D_TEST_CHECK_MSG(_suite,
                                      "rwlock_wrlock_unlock",
                                      rc == D_MUTEX_SUCCESS,
                                      "Unlock after wrlock should succeed") && result;
        }

        d_rwlock_destroy(&rwlock);
    }

    return result;
}


/*
d_tests_rwlock_tryrdlock
  Tests the d_rwlock_tryrdlock function.
  Tests the following:
  - tryrdlock on unlocked rwlock succeeds
  - tryrdlock with existing reader succeeds (multiple readers)
  - tryrdlock on write-locked rwlock returns D_MUTEX_BUSY
*/
bool d_tests_rwlock_tryrdlock(
    struct dj_test_suite* _suite
)
{
    bool              result;
    struct dj_rwlock_t rwlock;
    int               rc;

    d_test_suite_test(_suite, "rwlock_tryrdlock");

    result = true;

    rc = d_rwlock_init(&rwlock);

    if (rc == D_MUTEX_SUCCESS)
    {
        // test 1: tryrdlock on unlocked
        rc     = d_rwlock_tryrdlock(&rwlock);
        result = D_TEST_CHECK_MSG(_suite,
                                  "rwlock_tryrdlock_unlocked",
                                  rc == D_MUTEX_SUCCESS,
                                  "Tryrdlock on unlocked rwlock should succeed") && result;

        // test 2: second tryrdlock while first read held
        if (rc == D_MUTEX_SUCCESS)
        {
            int rc2;

            rc2    = d_rwlock_tryrdlock(&rwlock);
            result = D_TEST_CHECK_MSG(_suite,
                                      "rwlock_tryrdlock_concurrent_read",
                                      rc2 == D_MUTEX_SUCCESS,
                                      "Tryrdlock with existing reader should succeed") && result;

            if (rc2 == D_MUTEX_SUCCESS)
            {
                d_rwlock_unlock(&rwlock);
            }

            d_rwlock_unlock(&rwlock);
        }

        d_rwlock_destroy(&rwlock);
    }

    // test 3: tryrdlock on write-locked (from another thread)
    {
        struct dj_rwlock_t    rwlock2;
        struct dj_test_tryrd_data shared;
        dj_thread_t           thd;

        rc = d_rwlock_init(&rwlock2);

        if (rc == D_MUTEX_SUCCESS)
        {
            d_rwlock_wrlock(&rwlock2);

            shared.rw          = &rwlock2;
            shared.result_code = D_MUTEX_SUCCESS;

            rc = d_thread_create(&thd,
                                 d_test_tryrd_worker,
                                 &shared);

            if (rc == D_MUTEX_SUCCESS)
            {
                d_thread_join(thd, NULL);

                result = D_TEST_CHECK_MSG(_suite,
                                          "rwlock_tryrdlock_write_held",
                                          shared.result_code == D_MUTEX_BUSY,
                                          "Tryrdlock should fail when write-locked") && result;
            }

            d_rwlock_unlock(&rwlock2);
            d_rwlock_destroy(&rwlock2);
        }
    }

    return result;
}


/*
d_tests_rwlock_trywrlock
  Tests the d_rwlock_trywrlock function.
  Tests the following:
  - trywrlock on unlocked rwlock succeeds
  - trywrlock on read-locked rwlock returns D_MUTEX_BUSY
*/
bool d_tests_rwlock_trywrlock(
    struct dj_test_suite* _suite
)
{
    bool              result;
    struct dj_rwlock_t rwlock;
    int               rc;

    d_test_suite_test(_suite, "rwlock_trywrlock");

    result = true;

    rc = d_rwlock_init(&rwlock);

    if (rc == D_MUTEX_SUCCESS)
    {
        // test 1: trywrlock on unlocked
        rc     = d_rwlock_trywrlock(&rwlock);
        result = D_TEST_CHECK_MSG(_suite,
                                  "rwlock_trywrlock_unlocked",
                                  rc == D_MUTEX_SUCCESS,
                                  "Trywrlock on unlocked rwlock should succeed") && result;

        if (rc == D_MUTEX_SUCCESS)
        {
            d_rwlock_unlock(&rwlock);
        }

        // test 2: trywrlock when read-locked (from another thread)
        d_rwlock_rdlock(&rwlock);

        {
            struct dj_test_trywr_data shared;
            dj_thread_t               thd;

            shared.rw          = &rwlock;
            shared.result_code = D_MUTEX_SUCCESS;

            rc = d_thread_create(&thd,
                                 d_test_trywr_worker,
                                 &shared);

            if (rc == D_MUTEX_SUCCESS)
            {
                d_thread_join(thd, NULL);

                result = D_TEST_CHECK_MSG(_suite,
                                          "rwlock_trywrlock_read_held",
                                          shared.result_code == D_MUTEX_BUSY,
                                          "Trywrlock should fail when read-locked") && result;
            }
        }

        d_rwlock_unlock(&rwlock);
        d_rwlock_destroy(&rwlock);
    }

    return result;
}


/*
d_tests_rwlock_unlock
  Tests the d_rwlock_unlock function.
  Tests the following:
  - unlock after rdlock returns D_MUTEX_SUCCESS
  - unlock after wrlock returns D_MUTEX_SUCCESS
  - rwlock is reusable after unlock
*/
bool d_tests_rwlock_unlock(
    struct dj_test_suite* _suite
)
{
    bool              result;
    struct dj_rwlock_t rwlock;
    int               rc;

    d_test_suite_test(_suite, "rwlock_unlock");

    result = true;

    rc = d_rwlock_init(&rwlock);

    if (rc == D_MUTEX_SUCCESS)
    {
        // test 1: unlock after rdlock
        d_rwlock_rdlock(&rwlock);
        rc     = d_rwlock_unlock(&rwlock);
        result = D_TEST_CHECK_MSG(_suite,
                                  "rwlock_unlock_after_rd",
                                  rc == D_MUTEX_SUCCESS,
                                  "Unlock after rdlock should succeed") && result;

        // test 2: unlock after wrlock
        d_rwlock_wrlock(&rwlock);
        rc     = d_rwlock_unlock(&rwlock);
        result = D_TEST_CHECK_MSG(_suite,
                                  "rwlock_unlock_after_wr",
                                  rc == D_MUTEX_SUCCESS,
                                  "Unlock after wrlock should succeed") && result;

        // test 3: reusable
        rc     = d_rwlock_rdlock(&rwlock);
        result = D_TEST_CHECK_MSG(_suite,
                                  "rwlock_unlock_reusable",
                                  rc == D_MUTEX_SUCCESS,
                                  "Rwlock should be reusable after unlock") && result;

        if (rc == D_MUTEX_SUCCESS)
        {
            d_rwlock_unlock(&rwlock);
        }

        d_rwlock_destroy(&rwlock);
    }

    return result;
}


/*
d_tests_rwlock_timedrdlock
  Tests the d_rwlock_timedrdlock function.
  Tests the following:
  - timedrdlock on unlocked rwlock succeeds immediately
*/
bool d_tests_rwlock_timedrdlock(
    struct dj_test_suite* _suite
)
{
    bool              result;
    struct dj_rwlock_t rwlock;
    int               rc;
    struct timespec   timeout;

    d_test_suite_test(_suite, "rwlock_timedrdlock");

    result = true;

    rc = d_rwlock_init(&rwlock);

    if (rc == D_MUTEX_SUCCESS)
    {
        d_clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 1;

        rc     = d_rwlock_timedrdlock(&rwlock, &timeout);
        result = D_TEST_CHECK_MSG(_suite,
                                  "rwlock_timedrdlock_success",
                                  rc == D_MUTEX_SUCCESS,
                                  "Timedrdlock on unlocked rwlock should succeed") && result;

        if (rc == D_MUTEX_SUCCESS)
        {
            d_rwlock_unlock(&rwlock);
        }

        d_rwlock_destroy(&rwlock);
    }

    return result;
}


/*
d_tests_rwlock_timedwrlock
  Tests the d_rwlock_timedwrlock function.
  Tests the following:
  - timedwrlock on unlocked rwlock succeeds immediately
*/
bool d_tests_rwlock_timedwrlock(
    struct dj_test_suite* _suite
)
{
    bool              result;
    struct dj_rwlock_t rwlock;
    int               rc;
    struct timespec   timeout;

    d_test_suite_test(_suite, "rwlock_timedwrlock");

    result = true;

    rc = d_rwlock_init(&rwlock);

    if (rc == D_MUTEX_SUCCESS)
    {
        d_clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 1;

        rc     = d_rwlock_timedwrlock(&rwlock, &timeout);
        result = D_TEST_CHECK_MSG(_suite,
                                  "rwlock_timedwrlock_success",
                                  rc == D_MUTEX_SUCCESS,
                                  "Timedwrlock on unlocked rwlock should succeed") && result;

        if (rc == D_MUTEX_SUCCESS)
        {
            d_rwlock_unlock(&rwlock);
        }

        d_rwlock_destroy(&rwlock);
    }

    return result;
}


/*
d_tests_rwlock_concurrent_readers
  Tests concurrent reader access to a read-write lock.
  Tests the following:
  - multiple threads can hold read locks simultaneously
  - all readers see consistent data
*/
bool d_tests_rwlock_concurrent_readers(
    struct dj_test_suite* _suite
)
{
    bool                      result;
    struct dj_rwlock_t         rwlock;
    int                       shared_value;
    struct dj_test_reader_data rdata[4];
    dj_thread_t                threads[4];
    int                       rc;
    int                       i;
    int                       created;

    d_test_suite_test(_suite, "rwlock_concurrent_readers");

    result = true;

    rc = d_rwlock_init(&rwlock);

    if (rc != D_MUTEX_SUCCESS)
    {
        return result;
    }

    shared_value = 42;
    created      = 0;

    for (i = 0; i < 4; i++)
    {
        rdata[i].rw           = &rwlock;
        rdata[i].shared_value = &shared_value;
        rdata[i].read_value   = 0;
        rdata[i].acquired     = false;
    }

    for (i = 0; i < 4; i++)
    {
        rc = d_thread_create(&threads[i],
                                 d_test_reader_thread,
                             &rdata[i]);

        if (rc == D_MUTEX_SUCCESS)
        {
            created++;
        }
    }

    for (i = 0; i < created; i++)
    {
        d_thread_join(threads[i], NULL);
    }

    {
        bool all_acquired;
        bool all_correct;

        all_acquired = true;
        all_correct  = true;

        for (i = 0; i < created; i++)
        {
            if (!rdata[i].acquired)  { all_acquired = false; }
            if (rdata[i].read_value != 42) { all_correct = false; }
        }

        result = D_TEST_CHECK_MSG(_suite,
                                  "rwlock_concurrent_all_acquired",
                                  all_acquired,
                                  "All reader threads should acquire read lock") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "rwlock_concurrent_all_correct",
                                  all_correct,
                                  "All readers should read value 42") && result;
    }

    d_rwlock_destroy(&rwlock);

    return result;
}


/*
d_tests_rwlock_all
  Aggregation function that runs all read-write lock tests.
*/
bool d_tests_rwlock_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "rwlock_all");

    result = true;

    result = d_tests_rwlock_init_destroy(_suite) && result;
    result = d_tests_rwlock_rdlock(_suite) && result;
    result = d_tests_rwlock_wrlock(_suite) && result;
    result = d_tests_rwlock_tryrdlock(_suite) && result;
    result = d_tests_rwlock_trywrlock(_suite) && result;
    result = d_tests_rwlock_unlock(_suite) && result;
    result = d_tests_rwlock_timedrdlock(_suite) && result;
    result = d_tests_rwlock_timedwrlock(_suite) && result;
    result = d_tests_rwlock_concurrent_readers(_suite) && result;

    return result;
}
