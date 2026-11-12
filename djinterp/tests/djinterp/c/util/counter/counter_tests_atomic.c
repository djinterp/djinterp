/*******************************************************************************
* djinterp [c][utility]                                   counter_tests_atomic.c
*
* The counter suite, block "atomic": counter_atomic.h's lock-free
* counter, alone and under eight threads.
*
*
* path:      /tests/djinterp/c/util/counter/counter_tests_atomic.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "./counter_tests.h"  // corresponding header

// djinterp
#include "../../../../../inc/djinterp/c/sync/dmutex.h"  // d_thread_*


// D_TESTS_COUNTER_THREADS / _STEPS
//   constant: how many threads count, and how many steps each makes.
#define D_TESTS_COUNTER_THREADS  8
#define D_TESTS_COUNTER_STEPS    20000

/**
 * @brief A thread's work: D_TESTS_COUNTER_STEPS increments by one.
 *
 * @param[in,out] _counter  the shared struct dj_counter_atomic.
 * @return D_THREAD_SUCCESS.
 */
static dj_thread_result_t
d_tests_counter_atomic_worker(
    void* _counter
)
{
    struct dj_counter_atomic* counter =
        (struct dj_counter_atomic*)_counter;

    // count, one step at a time
    for (int i = 0; i < D_TESTS_COUNTER_STEPS; ++i)
    {
        (void)d_counter_atomic_increment(counter,
                                         1);
    }

    return D_THREAD_SUCCESS;
}

/**
 * @brief Runs D_TESTS_COUNTER_THREADS workers on one counter and waits.
 *
 * @param[in,out] _counter  the shared counter.
 * @return `true` when every thread was created and joined.
 */
static bool
d_tests_counter_atomic_run(
    struct dj_counter_atomic* _counter
)
{
    dj_thread_t threads[D_TESTS_COUNTER_THREADS] = { 0 };
    int         started                          = 0;
    bool        ok                               = true;

    // start them all
    for (int i = 0; i < D_TESTS_COUNTER_THREADS; ++i)
    {
        // a thread that cannot start ends the starting
        if (d_thread_create(&threads[i],
                            d_tests_counter_atomic_worker,
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
d_tests_counter_atomic_init
  The two init functions.
  Tests the following:
  - init sets the value and the start, with no bounds
  - init_bounded sets the bounds, and refuses what the plain counter refuses
  - a snapshot reproduces the plain counter
*/
bool
d_tests_counter_atomic_init(void)
{
    const struct d_interval_imax range    = d_tests_counter_range(0,
                                                                  10);
    struct dj_counter_atomic     counter  = { 0 };
    struct dj_counter            snapshot = D_COUNTER_INIT;

    d_counter_atomic_init(&counter,
                          4);
    D_TEST_REQUIRE(d_counter_atomic_value(&counter) == 4);
    D_TEST_REQUIRE(counter.initial == 4);
    D_TEST_REQUIRE(!counter.has_bounds);

    D_TEST_REQUIRE(d_counter_atomic_init_bounded(&counter,
                                                 10,
                                                 &range));
    D_TEST_REQUIRE(!d_counter_atomic_init_bounded(&counter,
                                                  11,
                                                  &range));
    D_TEST_REQUIRE(d_counter_atomic_value(&counter) == 10);

    D_TEST_REQUIRE(d_counter_atomic_snapshot(&counter,
                                             &snapshot));
    D_TEST_REQUIRE(snapshot.value == 10);
    D_TEST_REQUIRE(snapshot.initial == 10);
    D_TEST_REQUIRE(snapshot.has_bounds);
    D_TEST_REQUIRE(snapshot.bounds.upper == 10);

    return true;
}

/*
d_tests_counter_atomic_step
  One thread, the plain counter's behaviour.
  Tests the following:
  - increment and decrement move and stop at the bounds as counter.h's do
  - at_min and at_max follow
  - compare orders by value, NULL first
  - reset returns to the start; delta is the movement
*/
bool
d_tests_counter_atomic_step(void)
{
    const struct d_interval_imax range = d_tests_counter_range(0,
                                                               10);
    struct dj_counter_atomic     a     = { 0 };
    struct dj_counter_atomic     b     = { 0 };

    D_TEST_REQUIRE(d_counter_atomic_init_bounded(&a,
                                                 5,
                                                 &range));
    D_TEST_REQUIRE(d_counter_atomic_increment(&a,
                                              3));
    D_TEST_REQUIRE(d_counter_atomic_value(&a) == 8);
    D_TEST_REQUIRE(d_counter_atomic_delta(&a) == 3);
    D_TEST_REQUIRE(!d_counter_atomic_increment(&a,
                                               3));
    D_TEST_REQUIRE(d_counter_atomic_at_max(&a));
    D_TEST_REQUIRE(!d_counter_atomic_decrement(&a,
                                               INT64_MIN));
    D_TEST_REQUIRE(d_counter_atomic_value(&a) == 10);
    D_TEST_REQUIRE(!d_counter_atomic_decrement(&a,
                                               20));
    D_TEST_REQUIRE(d_counter_atomic_at_min(&a));

    d_counter_atomic_init(&b,
                          1);
    D_TEST_REQUIRE(d_counter_atomic_compare(&a,
                                            &b) == -1);
    D_TEST_REQUIRE(d_counter_atomic_compare(&b,
                                            &a) == 1);
    D_TEST_REQUIRE(d_counter_atomic_increment(&a,
                                              1));
    D_TEST_REQUIRE(d_counter_atomic_compare(&a,
                                            &b) == 0);
    D_TEST_REQUIRE(d_counter_atomic_compare(NULL,
                                            &a) == -1);

    d_counter_atomic_reset(&a);
    D_TEST_REQUIRE(d_counter_atomic_value(&a) == 5);
    D_TEST_REQUIRE(d_counter_atomic_delta(&a) == 0);

    return true;
}

/*
d_tests_counter_atomic_merge
  Movement, from either form.
  Tests the following:
  - merge adds an atomic source's movement since its start
  - merge_counter adds a plain counter's the same way
  - the destination's bounds hold, and the cut is reported
*/
bool
d_tests_counter_atomic_merge(void)
{
    const struct d_interval_imax range = d_tests_counter_range(0,
                                                               25);
    struct dj_counter_atomic     dest  = { 0 };
    struct dj_counter_atomic     src   = { 0 };
    struct dj_counter            local = D_COUNTER_INIT;

    d_counter_atomic_init(&dest,
                          10);
    d_counter_atomic_init(&src,
                          10);
    D_TEST_REQUIRE(d_counter_atomic_increment(&src,
                                              4));
    D_TEST_REQUIRE(d_counter_atomic_merge(&dest,
                                          &src));
    D_TEST_REQUIRE(d_counter_atomic_value(&dest) == 14);

    d_counter_init(&local,
                   100);
    D_TEST_REQUIRE(d_counter_increment(&local,
                                       6));
    D_TEST_REQUIRE(d_counter_atomic_merge_counter(&dest,
                                                  &local));
    D_TEST_REQUIRE(d_counter_atomic_value(&dest) == 20);

    D_TEST_REQUIRE(d_counter_atomic_init_bounded(&dest,
                                                 20,
                                                 &range));
    D_TEST_REQUIRE(!d_counter_atomic_merge_counter(&dest,
                                                   &local));
    D_TEST_REQUIRE(d_counter_atomic_value(&dest) == 25);

    return true;
}

/*
d_tests_counter_atomic_threads
  Contention.
  Tests the following:
  - eight threads, each counting 20000, leave exactly 160000
  - under a ceiling below that, the value stops on it and never past it
*/
bool
d_tests_counter_atomic_threads(void)
{
    const int64_t                total   = (int64_t)D_TESTS_COUNTER_THREADS *
                                           D_TESTS_COUNTER_STEPS;
    const struct d_interval_imax range   = d_tests_counter_range(0,
                                                                 1000);
    struct dj_counter_atomic     counter = { 0 };

    d_counter_atomic_init(&counter,
                          0);
    D_TEST_REQUIRE(d_tests_counter_atomic_run(&counter));
    D_TEST_REQUIRE(d_counter_atomic_value(&counter) == total);

    D_TEST_REQUIRE(d_counter_atomic_init_bounded(&counter,
                                                 0,
                                                 &range));
    D_TEST_REQUIRE(d_tests_counter_atomic_run(&counter));
    D_TEST_REQUIRE(d_counter_atomic_value(&counter) == 1000);
    D_TEST_REQUIRE(d_counter_atomic_at_max(&counter));

    return true;
}

/*
d_tests_counter_atomic_null
  Every function, given NULL.
  Tests the following:
  - the operations return false, and init and reset do nothing
  - the queries answer as an empty counter
*/
bool
d_tests_counter_atomic_null(void)
{
    struct dj_counter_atomic counter = { 0 };
    struct dj_counter        plain   = D_COUNTER_INIT;

    d_counter_atomic_init(&counter,
                          0);
    d_counter_atomic_init(NULL,
                          1);
    d_counter_atomic_reset(NULL);
    D_TEST_REQUIRE(!d_counter_atomic_init_bounded(NULL,
                                                  0,
                                                  NULL));
    D_TEST_REQUIRE(!d_counter_atomic_increment(NULL,
                                               1));
    D_TEST_REQUIRE(!d_counter_atomic_decrement(NULL,
                                               1));
    D_TEST_REQUIRE(!d_counter_atomic_merge(NULL,
                                           &counter));
    D_TEST_REQUIRE(!d_counter_atomic_merge(&counter,
                                           NULL));
    D_TEST_REQUIRE(!d_counter_atomic_merge_counter(&counter,
                                                   NULL));
    D_TEST_REQUIRE(!d_counter_atomic_snapshot(NULL,
                                              &plain));
    D_TEST_REQUIRE(!d_counter_atomic_snapshot(&counter,
                                              NULL));

    D_TEST_REQUIRE(d_counter_atomic_value(NULL) == 0);
    D_TEST_REQUIRE(d_counter_atomic_delta(NULL) == 0);
    D_TEST_REQUIRE(!d_counter_atomic_at_min(NULL));
    D_TEST_REQUIRE(!d_counter_atomic_at_max(NULL));

    return true;
}
