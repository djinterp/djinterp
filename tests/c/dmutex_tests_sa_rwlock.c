#include ".\dmutex_tests_sa.h"


///////////////////////////////////////////////////////////////////////////////
// file-scope helper structs and worker functions
///////////////////////////////////////////////////////////////////////////////

struct d_test_tryrd_data
{
    struct d_rwlock_t* rw;
    int                result_code;
};

static void*
d_test_tryrd_worker(void* _arg)
{
    struct d_test_tryrd_data* data;
    data              = (struct d_test_tryrd_data*)_arg;
    data->result_code = d_rwlock_tryrdlock(data->rw);
    if (data->result_code == D_MUTEX_SUCCESS)
    {
        d_rwlock_unlock(data->rw);
    }
    return D_THREAD_SUCCESS;
}

struct d_test_trywr_data
{
    struct d_rwlock_t* rw;
    int                result_code;
};

static void*
d_test_trywr_worker(void* _arg)
{
    struct d_test_trywr_data* data;
    data              = (struct d_test_trywr_data*)_arg;
    data->result_code = d_rwlock_trywrlock(data->rw);
    if (data->result_code == D_MUTEX_SUCCESS)
    {
        d_rwlock_unlock(data->rw);
    }
    return D_THREAD_SUCCESS;
}

struct d_test_reader_data
{
    struct d_rwlock_t* rw;
    const int*         shared_value;
    int                read_value;
    bool               acquired;
};

static void*
d_test_reader_thread(void* _arg)
{
    struct d_test_reader_data* data;
    struct timespec            dur;
    data = (struct d_test_reader_data*)_arg;
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
d_tests_sa_rwlock_init_destroy
  Tests the d_rwlock_init and d_rwlock_destroy functions.
  Tests the following:
  - successful init returns D_MUTEX_SUCCESS
  - successful destroy returns D_MUTEX_SUCCESS
  - multiple rwlocks can be initialized
*/
bool
d_tests_sa_rwlock_init_destroy
(
    struct d_test_counter* _counter
)
{
    bool              result;
    struct d_rwlock_t rwlock;
    int               rc;

    result = true;

    rc     = d_rwlock_init(&rwlock);
    result = d_assert_standalone(
        rc == D_MUTEX_SUCCESS,
        "rwlock_init_success",
        "d_rwlock_init should return D_MUTEX_SUCCESS",
        _counter) && result;

    if (rc == D_MUTEX_SUCCESS)
    {
        rc     = d_rwlock_destroy(&rwlock);
        result = d_assert_standalone(
            rc == D_MUTEX_SUCCESS,
            "rwlock_destroy_success",
            "d_rwlock_destroy should return D_MUTEX_SUCCESS",
            _counter) && result;
    }

    {
        struct d_rwlock_t rw_a;
        struct d_rwlock_t rw_b;
        int               rc_a;
        int               rc_b;

        rc_a = d_rwlock_init(&rw_a);
        rc_b = d_rwlock_init(&rw_b);

        result = d_assert_standalone(
            rc_a == D_MUTEX_SUCCESS && rc_b == D_MUTEX_SUCCESS,
            "rwlock_init_multiple",
            "Multiple rwlocks should initialize independently",
            _counter) && result;

        if (rc_a == D_MUTEX_SUCCESS) { d_rwlock_destroy(&rw_a); }
        if (rc_b == D_MUTEX_SUCCESS) { d_rwlock_destroy(&rw_b); }
    }

    return result;
}


/*
d_tests_sa_rwlock_rdlock
  Tests the d_rwlock_rdlock function.
  Tests the following:
  - read lock succeeds on unlocked rwlock
  - unlock after rdlock succeeds
*/
bool
d_tests_sa_rwlock_rdlock
(
    struct d_test_counter* _counter
)
{
    bool              result;
    struct d_rwlock_t rwlock;
    int               rc;

    result = true;

    rc = d_rwlock_init(&rwlock);

    if (rc == D_MUTEX_SUCCESS)
    {
        rc     = d_rwlock_rdlock(&rwlock);
        result = d_assert_standalone(
            rc == D_MUTEX_SUCCESS,
            "rwlock_rdlock_success",
            "d_rwlock_rdlock should return D_MUTEX_SUCCESS",
            _counter) && result;

        if (rc == D_MUTEX_SUCCESS)
        {
            rc     = d_rwlock_unlock(&rwlock);
            result = d_assert_standalone(
                rc == D_MUTEX_SUCCESS,
                "rwlock_rdlock_unlock",
                "Unlock after rdlock should succeed",
                _counter) && result;
        }

        d_rwlock_destroy(&rwlock);
    }

    return result;
}


/*
d_tests_sa_rwlock_wrlock
  Tests the d_rwlock_wrlock function.
  Tests the following:
  - write lock succeeds on unlocked rwlock
  - unlock after wrlock succeeds
*/
bool
d_tests_sa_rwlock_wrlock
(
    struct d_test_counter* _counter
)
{
    bool              result;
    struct d_rwlock_t rwlock;
    int               rc;

    result = true;

    rc = d_rwlock_init(&rwlock);

    if (rc == D_MUTEX_SUCCESS)
    {
        rc     = d_rwlock_wrlock(&rwlock);
        result = d_assert_standalone(
            rc == D_MUTEX_SUCCESS,
            "rwlock_wrlock_success",
            "d_rwlock_wrlock should return D_MUTEX_SUCCESS",
            _counter) && result;

        if (rc == D_MUTEX_SUCCESS)
        {
            rc     = d_rwlock_unlock(&rwlock);
            result = d_assert_standalone(
                rc == D_MUTEX_SUCCESS,
                "rwlock_wrlock_unlock",
                "Unlock after wrlock should succeed",
                _counter) && result;
        }

        d_rwlock_destroy(&rwlock);
    }

    return result;
}


/*
d_tests_sa_rwlock_tryrdlock
  Tests the d_rwlock_tryrdlock function.
  Tests the following:
  - tryrdlock on unlocked rwlock succeeds
  - tryrdlock with existing reader succeeds (multiple readers)
  - tryrdlock on write-locked rwlock returns D_MUTEX_BUSY
*/
bool
d_tests_sa_rwlock_tryrdlock
(
    struct d_test_counter* _counter
)
{
    bool              result;
    struct d_rwlock_t rwlock;
    int               rc;

    result = true;

    rc = d_rwlock_init(&rwlock);

    if (rc == D_MUTEX_SUCCESS)
    {
        // test 1: tryrdlock on unlocked
        rc     = d_rwlock_tryrdlock(&rwlock);
        result = d_assert_standalone(
            rc == D_MUTEX_SUCCESS,
            "rwlock_tryrdlock_unlocked",
            "Tryrdlock on unlocked rwlock should succeed",
            _counter) && result;

        // test 2: second tryrdlock while first read held
        if (rc == D_MUTEX_SUCCESS)
        {
            int rc2;

            rc2    = d_rwlock_tryrdlock(&rwlock);
            result = d_assert_standalone(
                rc2 == D_MUTEX_SUCCESS,
                "rwlock_tryrdlock_concurrent_read",
                "Tryrdlock with existing reader should succeed",
                _counter) && result;

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
        struct d_rwlock_t    rwlock2;
        struct d_test_tryrd_data shared;
        d_thread_t           thd;

        rc = d_rwlock_init(&rwlock2);

        if (rc == D_MUTEX_SUCCESS)
        {
            d_rwlock_wrlock(&rwlock2);

            shared.rw          = &rwlock2;
            shared.result_code = D_MUTEX_SUCCESS;

            rc = d_thread_create(&thd,
                                 (d_thread_func_t)d_test_tryrd_worker,
                                 &shared);

            if (rc == D_MUTEX_SUCCESS)
            {
                d_thread_join(thd, NULL);

                result = d_assert_standalone(
                    shared.result_code == D_MUTEX_BUSY,
                    "rwlock_tryrdlock_write_held",
                    "Tryrdlock should fail when write-locked",
                    _counter) && result;
            }

            d_rwlock_unlock(&rwlock2);
            d_rwlock_destroy(&rwlock2);
        }
    }

    return result;
}


/*
d_tests_sa_rwlock_trywrlock
  Tests the d_rwlock_trywrlock function.
  Tests the following:
  - trywrlock on unlocked rwlock succeeds
  - trywrlock on read-locked rwlock returns D_MUTEX_BUSY
*/
bool
d_tests_sa_rwlock_trywrlock
(
    struct d_test_counter* _counter
)
{
    bool              result;
    struct d_rwlock_t rwlock;
    int               rc;

    result = true;

    rc = d_rwlock_init(&rwlock);

    if (rc == D_MUTEX_SUCCESS)
    {
        // test 1: trywrlock on unlocked
        rc     = d_rwlock_trywrlock(&rwlock);
        result = d_assert_standalone(
            rc == D_MUTEX_SUCCESS,
            "rwlock_trywrlock_unlocked",
            "Trywrlock on unlocked rwlock should succeed",
            _counter) && result;

        if (rc == D_MUTEX_SUCCESS)
        {
            d_rwlock_unlock(&rwlock);
        }

        // test 2: trywrlock when read-locked (from another thread)
        d_rwlock_rdlock(&rwlock);

        {
            struct d_test_trywr_data shared;
            d_thread_t               thd;

            shared.rw          = &rwlock;
            shared.result_code = D_MUTEX_SUCCESS;

            rc = d_thread_create(&thd,
                                 (d_thread_func_t)d_test_trywr_worker,
                                 &shared);

            if (rc == D_MUTEX_SUCCESS)
            {
                d_thread_join(thd, NULL);

                result = d_assert_standalone(
                    shared.result_code == D_MUTEX_BUSY,
                    "rwlock_trywrlock_read_held",
                    "Trywrlock should fail when read-locked",
                    _counter) && result;
            }
        }

        d_rwlock_unlock(&rwlock);
        d_rwlock_destroy(&rwlock);
    }

    return result;
}


/*
d_tests_sa_rwlock_unlock
  Tests the d_rwlock_unlock function.
  Tests the following:
  - unlock after rdlock returns D_MUTEX_SUCCESS
  - unlock after wrlock returns D_MUTEX_SUCCESS
  - rwlock is reusable after unlock
*/
bool
d_tests_sa_rwlock_unlock
(
    struct d_test_counter* _counter
)
{
    bool              result;
    struct d_rwlock_t rwlock;
    int               rc;

    result = true;

    rc = d_rwlock_init(&rwlock);

    if (rc == D_MUTEX_SUCCESS)
    {
        // test 1: unlock after rdlock
        d_rwlock_rdlock(&rwlock);
        rc     = d_rwlock_unlock(&rwlock);
        result = d_assert_standalone(
            rc == D_MUTEX_SUCCESS,
            "rwlock_unlock_after_rd",
            "Unlock after rdlock should succeed",
            _counter) && result;

        // test 2: unlock after wrlock
        d_rwlock_wrlock(&rwlock);
        rc     = d_rwlock_unlock(&rwlock);
        result = d_assert_standalone(
            rc == D_MUTEX_SUCCESS,
            "rwlock_unlock_after_wr",
            "Unlock after wrlock should succeed",
            _counter) && result;

        // test 3: reusable
        rc     = d_rwlock_rdlock(&rwlock);
        result = d_assert_standalone(
            rc == D_MUTEX_SUCCESS,
            "rwlock_unlock_reusable",
            "Rwlock should be reusable after unlock",
            _counter) && result;

        if (rc == D_MUTEX_SUCCESS)
        {
            d_rwlock_unlock(&rwlock);
        }

        d_rwlock_destroy(&rwlock);
    }

    return result;
}


/*
d_tests_sa_rwlock_timedrdlock
  Tests the d_rwlock_timedrdlock function.
  Tests the following:
  - timedrdlock on unlocked rwlock succeeds immediately
*/
bool
d_tests_sa_rwlock_timedrdlock
(
    struct d_test_counter* _counter
)
{
    bool              result;
    struct d_rwlock_t rwlock;
    int               rc;
    struct timespec   timeout;

    result = true;

    rc = d_rwlock_init(&rwlock);

    if (rc == D_MUTEX_SUCCESS)
    {
        d_clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 1;

        rc     = d_rwlock_timedrdlock(&rwlock, &timeout);
        result = d_assert_standalone(
            rc == D_MUTEX_SUCCESS,
            "rwlock_timedrdlock_success",
            "Timedrdlock on unlocked rwlock should succeed",
            _counter) && result;

        if (rc == D_MUTEX_SUCCESS)
        {
            d_rwlock_unlock(&rwlock);
        }

        d_rwlock_destroy(&rwlock);
    }

    return result;
}


/*
d_tests_sa_rwlock_timedwrlock
  Tests the d_rwlock_timedwrlock function.
  Tests the following:
  - timedwrlock on unlocked rwlock succeeds immediately
*/
bool
d_tests_sa_rwlock_timedwrlock
(
    struct d_test_counter* _counter
)
{
    bool              result;
    struct d_rwlock_t rwlock;
    int               rc;
    struct timespec   timeout;

    result = true;

    rc = d_rwlock_init(&rwlock);

    if (rc == D_MUTEX_SUCCESS)
    {
        d_clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 1;

        rc     = d_rwlock_timedwrlock(&rwlock, &timeout);
        result = d_assert_standalone(
            rc == D_MUTEX_SUCCESS,
            "rwlock_timedwrlock_success",
            "Timedwrlock on unlocked rwlock should succeed",
            _counter) && result;

        if (rc == D_MUTEX_SUCCESS)
        {
            d_rwlock_unlock(&rwlock);
        }

        d_rwlock_destroy(&rwlock);
    }

    return result;
}


/*
d_tests_sa_rwlock_concurrent_readers
  Tests concurrent reader access to a read-write lock.
  Tests the following:
  - multiple threads can hold read locks simultaneously
  - all readers see consistent data
*/
bool
d_tests_sa_rwlock_concurrent_readers
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    struct d_rwlock_t         rwlock;
    int                       shared_value;
    struct d_test_reader_data rdata[4];
    d_thread_t                threads[4];
    int                       rc;
    int                       i;
    int                       created;

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
                                 (d_thread_func_t)d_test_reader_thread,
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

        result = d_assert_standalone(
            all_acquired,
            "rwlock_concurrent_all_acquired",
            "All reader threads should acquire read lock",
            _counter) && result;

        result = d_assert_standalone(
            all_correct,
            "rwlock_concurrent_all_correct",
            "All readers should read value 42",
            _counter) && result;
    }

    d_rwlock_destroy(&rwlock);

    return result;
}


/*
d_tests_sa_rwlock_all
  Aggregation function that runs all read-write lock tests.
*/
bool
d_tests_sa_rwlock_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Read-Write Locks\n");
    printf("  ----------------------------\n");

    result = d_tests_sa_rwlock_init_destroy(_counter) && result;
    result = d_tests_sa_rwlock_rdlock(_counter) && result;
    result = d_tests_sa_rwlock_wrlock(_counter) && result;
    result = d_tests_sa_rwlock_tryrdlock(_counter) && result;
    result = d_tests_sa_rwlock_trywrlock(_counter) && result;
    result = d_tests_sa_rwlock_unlock(_counter) && result;
    result = d_tests_sa_rwlock_timedrdlock(_counter) && result;
    result = d_tests_sa_rwlock_timedwrlock(_counter) && result;
    result = d_tests_sa_rwlock_concurrent_readers(_counter) && result;

    return result;
}
