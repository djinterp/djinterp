/*******************************************************************************
* djinterp [c][utility]                                    counter_tests_mutex.c
*
* The counter suite, block "mutex": counter_mutex.h's guarded counter, its
* lifetime and its five operations under a lock.
*
*
* path:      /tests/djinterp/c/util/counter/counter_tests_mutex.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#include "./counter_tests.h"  // corresponding header

// djinterp
#include "../../../../../inc/djinterp/c/sync/dmutex.h"  // d_thread_*


// D_TESTS_COUNTER_MUTEX_THREADS / _STEPS
//   constant: how many threads count, and how many steps each makes.
#define D_TESTS_COUNTER_MUTEX_THREADS  8
#define D_TESTS_COUNTER_MUTEX_STEPS    5000

/**
 * @brief A thread's work: D_TESTS_COUNTER_MUTEX_STEPS increments by one.
 *
 * @param[in,out] _counter  the shared struct dj_counter_mutex.
 * @return D_THREAD_SUCCESS.
 */
static dj_thread_result_t
d_tests_counter_mutex_worker(
    void* _counter
)
{
    struct dj_counter_mutex* counter = (struct dj_counter_mutex*)_counter;

    // count, one step at a time
    for (int i = 0; i < D_TESTS_COUNTER_MUTEX_STEPS; ++i)
    {
        (void)d_counter_mutex_increment(counter,
                                        1);
    }

    return D_THREAD_SUCCESS;
}

/**
 * @brief Runs D_TESTS_COUNTER_MUTEX_THREADS workers on one counter and waits.
 *
 * @param[in,out] _counter  the shared counter.
 * @return `true` when every thread was created and joined.
 */
static bool
d_tests_counter_mutex_run(
    struct dj_counter_mutex* _counter
)
{
    dj_thread_t threads[D_TESTS_COUNTER_MUTEX_THREADS] = { 0 };
    int         started                                = 0;
    bool        ok                                     = true;

    // start them all
    for (int i = 0; i < D_TESTS_COUNTER_MUTEX_THREADS; ++i)
    {
        // a thread that cannot start ends the starting
        if (d_thread_create(&threads[i],
                            d_tests_counter_mutex_worker,
                            _counter) != D_MUTEX_SUCCESS)
        {
            ok = false;

            break;
        }

        ++started;
    }

    // and wait for every one that started
    for (int i = 0; i < started; ++i)
    {
        // a failed join fails the run
        if (d_thread_join(threads[i],
                          NULL) != D_MUTEX_SUCCESS)
        {
            ok = false;
        }
    }

    return ok;
}

/*
d_tests_counter_mutex_lifetime
  Setting up and releasing a guarded counter.
  Tests the following:
  - init readies the counter at its start, with no bounds
  - init_bounded sets the bounds, and refuses what the plain counter refuses,
    leaving the counter not ready
  - after destroy the counter is not ready: operations fail, queries answer 0
  - destroying twice, or a counter never set up, does nothing
*/
bool
d_tests_counter_mutex_lifetime(void)
{
    const struct d_interval_imax range   = d_tests_counter_range(0,
                                                                 10);
    struct dj_counter_mutex      counter = { D_COUNTER_INIT };
    struct dj_counter            copy    = D_COUNTER_INIT;

    D_TEST_REQUIRE(d_counter_mutex_init(&counter,
                                        -3));
    D_TEST_REQUIRE(counter.ready);
    D_TEST_REQUIRE(d_counter_mutex_value(&counter) == -3);
    D_TEST_REQUIRE(d_counter_mutex_snapshot(&counter,
                                            &copy));
    D_TEST_REQUIRE(copy.initial == -3);
    D_TEST_REQUIRE(!copy.has_bounds);
    d_counter_mutex_destroy(&counter);
    D_TEST_REQUIRE(!counter.ready);

    D_TEST_REQUIRE(d_counter_mutex_init_bounded(&counter,
                                                10,
                                                &range));
    D_TEST_REQUIRE(d_counter_mutex_at_max(&counter));
    d_counter_mutex_destroy(&counter);

    D_TEST_REQUIRE(!d_counter_mutex_init_bounded(&counter,
                                                 11,
                                                 &range));
    D_TEST_REQUIRE(!counter.ready);
    D_TEST_REQUIRE(!d_counter_mutex_increment(&counter,
                                              1));
    D_TEST_REQUIRE(d_counter_mutex_value(&counter) == 0);

    D_TEST_REQUIRE(d_counter_mutex_init(&counter,
                                        5));
    d_counter_mutex_destroy(&counter);
    d_counter_mutex_destroy(&counter);
    D_TEST_REQUIRE(!d_counter_mutex_increment(&counter,
                                              1));
    D_TEST_REQUIRE(!d_counter_mutex_decrement(&counter,
                                              1));
    D_TEST_REQUIRE(!d_counter_mutex_snapshot(&counter,
                                             &copy));
    D_TEST_REQUIRE(d_counter_mutex_value(&counter) == 0);
    D_TEST_REQUIRE(d_counter_mutex_delta(&counter) == 0);

    return true;
}

/*
d_tests_counter_mutex_step
  The five operations, one thread.
  Tests the following:
  - increment and decrement move either way and stop at the bounds, and
    report the cut
  - compare orders by value, a counter not ready first
  - reset returns to the start and keeps the bounds; delta measures from it
*/
bool
d_tests_counter_mutex_step(void)
{
    const struct d_interval_imax range = d_tests_counter_range(0,
                                                               10);
    struct dj_counter_mutex      a     = { D_COUNTER_INIT };
    struct dj_counter_mutex      b     = { D_COUNTER_INIT };
    struct dj_counter_mutex      gone  = { D_COUNTER_INIT };

    D_TEST_REQUIRE(d_counter_mutex_init_bounded(&a,
                                                4,
                                                &range));
    D_TEST_REQUIRE(d_counter_mutex_init(&b,
                                        4));

    D_TEST_REQUIRE(d_counter_mutex_increment(&a,
                                             5));
    D_TEST_REQUIRE(d_counter_mutex_value(&a) == 9);
    D_TEST_REQUIRE(!d_counter_mutex_increment(&a,
                                              5));
    D_TEST_REQUIRE(d_counter_mutex_value(&a) == 10);
    D_TEST_REQUIRE(d_counter_mutex_at_max(&a));
    D_TEST_REQUIRE(d_counter_mutex_increment(&a,
                                             -3));
    D_TEST_REQUIRE(d_counter_mutex_value(&a) == 7);
    D_TEST_REQUIRE(!d_counter_mutex_decrement(&a,
                                              20));
    D_TEST_REQUIRE(d_counter_mutex_value(&a) == 0);
    D_TEST_REQUIRE(d_counter_mutex_at_min(&a));
    D_TEST_REQUIRE(d_counter_mutex_delta(&a) == -4);

    D_TEST_REQUIRE(d_counter_mutex_compare(&a,
                                           &b) == -1);
    D_TEST_REQUIRE(d_counter_mutex_compare(&b,
                                           &a) == 1);
    D_TEST_REQUIRE(d_counter_mutex_compare(&b,
                                           &b) == 0);
    D_TEST_REQUIRE(d_counter_mutex_compare(&gone,
                                           &a) == -1);
    D_TEST_REQUIRE(d_counter_mutex_compare(&gone,
                                           NULL) == 0);

    d_counter_mutex_reset(&a);
    D_TEST_REQUIRE(d_counter_mutex_value(&a) == 4);
    D_TEST_REQUIRE(d_counter_mutex_delta(&a) == 0);
    D_TEST_REQUIRE(!d_counter_mutex_increment(&a,
                                              7));
    D_TEST_REQUIRE(d_counter_mutex_value(&a) == 10);

    d_counter_mutex_destroy(&a);
    d_counter_mutex_destroy(&b);

    return true;
}

/*
d_tests_counter_mutex_merge
  Adding another counter's movement.
  Tests the following:
  - merge from a guarded counter adds its movement since its start
  - merge_counter does the same from a plain counter
  - a counter merged into itself doubles its movement, without a deadlock
  - a source not ready gives nothing, and the merge fails
*/
bool
d_tests_counter_mutex_merge(void)
{
    struct dj_counter_mutex into   = { D_COUNTER_INIT };
    struct dj_counter_mutex from   = { D_COUNTER_INIT };
    struct dj_counter_mutex gone   = { D_COUNTER_INIT };
    struct dj_counter       plain  = D_COUNTER_INIT;

    D_TEST_REQUIRE(d_counter_mutex_init(&into,
                                        100));
    D_TEST_REQUIRE(d_counter_mutex_init(&from,
                                        10));
    D_TEST_REQUIRE(d_counter_mutex_increment(&from,
                                             3));

    D_TEST_REQUIRE(d_counter_mutex_merge(&into,
                                         &from));
    D_TEST_REQUIRE(d_counter_mutex_value(&into) == 103);
    D_TEST_REQUIRE(d_counter_mutex_value(&from) == 13);

    d_counter_init(&plain,
                   -5);
    D_TEST_REQUIRE(d_counter_decrement(&plain,
                                       2));
    D_TEST_REQUIRE(d_counter_mutex_merge_counter(&into,
                                                 &plain));
    D_TEST_REQUIRE(d_counter_mutex_value(&into) == 101);

    D_TEST_REQUIRE(d_counter_mutex_merge(&into,
                                         &into));
    D_TEST_REQUIRE(d_counter_mutex_value(&into) == 102);

    D_TEST_REQUIRE(!d_counter_mutex_merge(&into,
                                          &gone));
    D_TEST_REQUIRE(!d_counter_mutex_merge(&gone,
                                          &from));
    D_TEST_REQUIRE(d_counter_mutex_value(&into) == 102);

    d_counter_mutex_destroy(&into);
    d_counter_mutex_destroy(&from);

    return true;
}

/*
d_tests_counter_mutex_threads
  Contention.
  Tests the following:
  - eight threads, each counting 5000, leave exactly 40000
  - under a ceiling below that, the value stops on it and never past it
*/
bool
d_tests_counter_mutex_threads(void)
{
    const int64_t                total   =
        (int64_t)D_TESTS_COUNTER_MUTEX_THREADS * D_TESTS_COUNTER_MUTEX_STEPS;
    const struct d_interval_imax range   = d_tests_counter_range(0,
                                                                 1000);
    struct dj_counter_mutex      counter = { D_COUNTER_INIT };

    D_TEST_REQUIRE(d_counter_mutex_init(&counter,
                                        0));
    D_TEST_REQUIRE(d_tests_counter_mutex_run(&counter));
    D_TEST_REQUIRE(d_counter_mutex_value(&counter) == total);
    d_counter_mutex_destroy(&counter);

    D_TEST_REQUIRE(d_counter_mutex_init_bounded(&counter,
                                                0,
                                                &range));
    D_TEST_REQUIRE(d_tests_counter_mutex_run(&counter));
    D_TEST_REQUIRE(d_counter_mutex_value(&counter) == 1000);
    D_TEST_REQUIRE(d_counter_mutex_at_max(&counter));
    d_counter_mutex_destroy(&counter);

    return true;
}

/*
d_tests_counter_mutex_null
  Every function, given NULL.
  Tests the following:
  - the operations return false, and destroy and reset do nothing
  - the queries answer as an empty counter
*/
bool
d_tests_counter_mutex_null(void)
{
    struct dj_counter_mutex counter = { D_COUNTER_INIT };
    struct dj_counter       plain   = D_COUNTER_INIT;

    D_TEST_REQUIRE(d_counter_mutex_init(&counter,
                                        0));

    D_TEST_REQUIRE(!d_counter_mutex_init(NULL,
                                         0));
    D_TEST_REQUIRE(!d_counter_mutex_init_bounded(NULL,
                                                 0,
                                                 NULL));
    d_counter_mutex_destroy(NULL);
    d_counter_mutex_reset(NULL);
    D_TEST_REQUIRE(!d_counter_mutex_increment(NULL,
                                              1));
    D_TEST_REQUIRE(!d_counter_mutex_decrement(NULL,
                                              1));
    D_TEST_REQUIRE(!d_counter_mutex_merge(NULL,
                                          &counter));
    D_TEST_REQUIRE(!d_counter_mutex_merge(&counter,
                                          NULL));
    D_TEST_REQUIRE(!d_counter_mutex_merge_counter(&counter,
                                                  NULL));
    D_TEST_REQUIRE(!d_counter_mutex_merge_counter(NULL,
                                                  &plain));
    D_TEST_REQUIRE(!d_counter_mutex_snapshot(NULL,
                                             &plain));
    D_TEST_REQUIRE(!d_counter_mutex_snapshot(&counter,
                                             NULL));

    D_TEST_REQUIRE(d_counter_mutex_value(NULL) == 0);
    D_TEST_REQUIRE(d_counter_mutex_delta(NULL) == 0);
    D_TEST_REQUIRE(!d_counter_mutex_at_min(NULL));
    D_TEST_REQUIRE(!d_counter_mutex_at_max(NULL));
    D_TEST_REQUIRE(d_counter_mutex_compare(NULL,
                                           NULL) == 0);

    d_counter_mutex_destroy(&counter);

    return true;
}
