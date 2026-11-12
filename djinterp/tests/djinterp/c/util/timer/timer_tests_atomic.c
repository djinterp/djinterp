/*******************************************************************************
* djinterp [c][utility]                                     timer_tests_atomic.c
*
* The timer suite, block "atomic": timer_atomic.h's lock-free timer, with the
* two costs its folded state word brings -- the ceiling at
* D_TIMER_ATOMIC_MAX, and a decrement that takes from an open interval.
*
*
* path:      /tests/djinterp/c/util/timer/timer_tests_atomic.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#include "./timer_tests.h"  // corresponding header

// djinterp
#include "../../../../../inc/djinterp/c/sync/dmutex.h"  // d_thread_*


// D_TESTS_TIMER_ATOMIC_THREADS / _STEPS
//   constant: how many threads add time, and how many steps each makes.
#define D_TESTS_TIMER_ATOMIC_THREADS  8
#define D_TESTS_TIMER_ATOMIC_STEPS    5000

/**
 * @brief A thread's work: D_TESTS_TIMER_ATOMIC_STEPS increments of 3 ns.
 *
 * @param[in,out] _timer  the shared struct dj_timer_atomic.
 * @return D_THREAD_SUCCESS.
 */
static dj_thread_result_t
d_tests_timer_atomic_worker(
    void* _timer
)
{
    struct dj_timer_atomic* timer = (struct dj_timer_atomic*)_timer;

    // add time, one step at a time
    for (int i = 0; i < D_TESTS_TIMER_ATOMIC_STEPS; ++i)
    {
        (void)d_timer_atomic_increment(timer,
                                       3);
    }

    return D_THREAD_SUCCESS;
}

/**
 * @brief Runs D_TESTS_TIMER_ATOMIC_THREADS workers on one timer and waits.
 *
 * @param[in,out] _timer  the shared timer.
 * @return `true` when every thread was created and joined.
 */
static bool
d_tests_timer_atomic_run(
    struct dj_timer_atomic* _timer
)
{
    dj_thread_t threads[D_TESTS_TIMER_ATOMIC_THREADS] = { 0 };
    int         started                               = 0;
    bool        ok                                    = true;

    // start them all
    for (int i = 0; i < D_TESTS_TIMER_ATOMIC_THREADS; ++i)
    {
        // a thread that cannot start ends the starting
        if (d_thread_create(&threads[i],
                            d_tests_timer_atomic_worker,
                            _timer) != D_MUTEX_SUCCESS)
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
d_tests_timer_atomic_init
  The two init functions and the snapshot.
  Tests the following:
  - init gives a stopped timer at zero with no limit
  - init_limited sets the limit, and refuses a negative one
  - a snapshot reproduces the timer as a plain one: time, limit and clock
*/
bool
d_tests_timer_atomic_init(void)
{
    struct dj_tests_timer_clock clock = { 0 };
    struct dj_timer_atomic      timer = { 0 };
    struct dj_timer             copy  = D_TIMER_INIT;

    d_timer_atomic_init(&timer);
    D_TEST_REQUIRE(!d_timer_atomic_running(&timer));
    D_TEST_REQUIRE(d_timer_atomic_elapsed(&timer) == 0);
    D_TEST_REQUIRE(!d_timer_atomic_expired(&timer));
    D_TEST_REQUIRE(d_timer_atomic_remaining(&timer) == 0);

    D_TEST_REQUIRE(!d_timer_atomic_init_limited(&timer,
                                                -1));
    D_TEST_REQUIRE(d_timer_atomic_init_limited(&timer,
                                               500));
    d_timer_atomic_set_clock(&timer,
                             d_tests_timer_clock_read,
                             &clock);
    D_TEST_REQUIRE(d_timer_atomic_increment(&timer,
                                            40));

    D_TEST_REQUIRE(d_timer_atomic_snapshot(&timer,
                                           &copy));
    D_TEST_REQUIRE(copy.elapsed == 40);
    D_TEST_REQUIRE(!copy.running);
    D_TEST_REQUIRE(copy.has_limit);
    D_TEST_REQUIRE(copy.limit == 500);
    D_TEST_REQUIRE(copy.clock == d_tests_timer_clock_read);
    D_TEST_REQUIRE(copy.context == &clock);

    return true;
}

/*
d_tests_timer_atomic_start_stop
  Intervals, on a hand-wound clock.
  Tests the following:
  - a running timer's elapsed time includes its open interval
  - intervals accumulate over stops
  - a second start, or a stop while stopped, is refused and changes nothing
  - a snapshot of a running timer is running, at the time now
*/
bool
d_tests_timer_atomic_start_stop(void)
{
    struct dj_tests_timer_clock clock = { 100 };
    struct dj_timer_atomic      timer = { 0 };
    struct dj_timer             copy  = D_TIMER_INIT;

    d_timer_atomic_init(&timer);
    d_timer_atomic_set_clock(&timer,
                             d_tests_timer_clock_read,
                             &clock);

    D_TEST_REQUIRE(!d_timer_atomic_stop(&timer));
    D_TEST_REQUIRE(d_timer_atomic_start(&timer));
    D_TEST_REQUIRE(d_timer_atomic_running(&timer));
    D_TEST_REQUIRE(!d_timer_atomic_start(&timer));
    clock.now = 150;
    D_TEST_REQUIRE(d_timer_atomic_elapsed(&timer) == 50);

    D_TEST_REQUIRE(d_timer_atomic_snapshot(&timer,
                                           &copy));
    D_TEST_REQUIRE(copy.running);
    D_TEST_REQUIRE(d_timer_elapsed(&copy) == 50);

    clock.now = 170;
    D_TEST_REQUIRE(d_timer_atomic_stop(&timer));
    D_TEST_REQUIRE(!d_timer_atomic_running(&timer));
    D_TEST_REQUIRE(d_timer_atomic_elapsed(&timer) == 70);
    clock.now = 900;
    D_TEST_REQUIRE(d_timer_atomic_elapsed(&timer) == 70);

    D_TEST_REQUIRE(d_timer_atomic_start(&timer));
    clock.now = 930;
    D_TEST_REQUIRE(d_timer_atomic_stop(&timer));
    D_TEST_REQUIRE(d_timer_atomic_elapsed(&timer) == 100);

    return true;
}

/*
d_tests_timer_atomic_step
  Adding and removing time.
  Tests the following:
  - increment and decrement add and remove time, stopped or running
  - decrement takes from the open interval too, where timer.h's does not,
    and stops at zero, reporting the cut
  - the time saturates at D_TIMER_ATOMIC_MAX, reporting the cut
*/
bool
d_tests_timer_atomic_step(void)
{
    struct dj_tests_timer_clock clock = { 1000 };
    struct dj_timer_atomic      timer = { 0 };

    d_timer_atomic_init(&timer);
    d_timer_atomic_set_clock(&timer,
                             d_tests_timer_clock_read,
                             &clock);

    D_TEST_REQUIRE(d_timer_atomic_increment(&timer,
                                            10));
    D_TEST_REQUIRE(d_timer_atomic_decrement(&timer,
                                            -5));
    D_TEST_REQUIRE(d_timer_atomic_elapsed(&timer) == 15);

    D_TEST_REQUIRE(d_timer_atomic_start(&timer));
    clock.now = 1030;
    D_TEST_REQUIRE(d_timer_atomic_elapsed(&timer) == 45);
    D_TEST_REQUIRE(d_timer_atomic_decrement(&timer,
                                            40));
    D_TEST_REQUIRE(d_timer_atomic_elapsed(&timer) == 5);
    D_TEST_REQUIRE(!d_timer_atomic_decrement(&timer,
                                             100));
    D_TEST_REQUIRE(d_timer_atomic_elapsed(&timer) == 0);
    D_TEST_REQUIRE(d_timer_atomic_running(&timer));
    clock.now = 1040;
    D_TEST_REQUIRE(d_timer_atomic_elapsed(&timer) == 10);
    D_TEST_REQUIRE(d_timer_atomic_stop(&timer));

    D_TEST_REQUIRE(!d_timer_atomic_increment(&timer,
                                             INT64_MAX));
    D_TEST_REQUIRE(d_timer_atomic_elapsed(&timer) == D_TIMER_ATOMIC_MAX);

    return true;
}

/*
d_tests_timer_atomic_merge
  Ordering and merging.
  Tests the following:
  - compare orders by elapsed time, NULL first
  - merge adds an atomic timer's elapsed time; merge_timer a plain one's
  - a timer merged into itself doubles its time
  - reset stops and zeroes, and keeps the limit
*/
bool
d_tests_timer_atomic_merge(void)
{
    struct dj_timer_atomic a     = { 0 };
    struct dj_timer_atomic b     = { 0 };
    struct dj_timer        plain = D_TIMER_INIT;

    D_TEST_REQUIRE(d_timer_atomic_init_limited(&a,
                                               1000));
    d_timer_atomic_init(&b);
    D_TEST_REQUIRE(d_timer_atomic_increment(&a,
                                            20));
    D_TEST_REQUIRE(d_timer_atomic_increment(&b,
                                            30));

    D_TEST_REQUIRE(d_timer_atomic_compare(&a,
                                          &b) == -1);
    D_TEST_REQUIRE(d_timer_atomic_compare(&b,
                                          &a) == 1);
    D_TEST_REQUIRE(d_timer_atomic_compare(&a,
                                          &a) == 0);
    D_TEST_REQUIRE(d_timer_atomic_compare(NULL,
                                          &a) == -1);

    D_TEST_REQUIRE(d_timer_atomic_merge(&a,
                                        &b));
    D_TEST_REQUIRE(d_timer_atomic_elapsed(&a) == 50);
    D_TEST_REQUIRE(d_timer_atomic_elapsed(&b) == 30);

    d_timer_init(&plain);
    D_TEST_REQUIRE(d_timer_increment(&plain,
                                     7));
    D_TEST_REQUIRE(d_timer_atomic_merge_timer(&a,
                                              &plain));
    D_TEST_REQUIRE(d_timer_atomic_elapsed(&a) == 57);

    D_TEST_REQUIRE(d_timer_atomic_merge(&a,
                                        &a));
    D_TEST_REQUIRE(d_timer_atomic_elapsed(&a) == 114);

    d_timer_atomic_reset(&a);
    D_TEST_REQUIRE(d_timer_atomic_elapsed(&a) == 0);
    D_TEST_REQUIRE(!d_timer_atomic_running(&a));
    D_TEST_REQUIRE(d_timer_atomic_remaining(&a) == 1000);

    return true;
}

/*
d_tests_timer_atomic_limit
  A limited timer.
  Tests the following:
  - remaining counts down to the limit, and the timer expires on it
  - an expired timer will not start
  - a timer with no limit never expires
*/
bool
d_tests_timer_atomic_limit(void)
{
    struct dj_timer_atomic timer = { 0 };

    D_TEST_REQUIRE(d_timer_atomic_init_limited(&timer,
                                               100));
    D_TEST_REQUIRE(d_timer_atomic_remaining(&timer) == 100);
    D_TEST_REQUIRE(d_timer_atomic_increment(&timer,
                                            60));
    D_TEST_REQUIRE(d_timer_atomic_remaining(&timer) == 40);
    D_TEST_REQUIRE(!d_timer_atomic_expired(&timer));
    D_TEST_REQUIRE(d_timer_atomic_increment(&timer,
                                            40));
    D_TEST_REQUIRE(d_timer_atomic_expired(&timer));
    D_TEST_REQUIRE(d_timer_atomic_remaining(&timer) == 0);
    D_TEST_REQUIRE(!d_timer_atomic_start(&timer));

    d_timer_atomic_init(&timer);
    D_TEST_REQUIRE(d_timer_atomic_increment(&timer,
                                            1000000));
    D_TEST_REQUIRE(!d_timer_atomic_expired(&timer));

    return true;
}

/*
d_tests_timer_atomic_threads
  Contention.
  Tests the following:
  - eight threads, each adding 5000 steps of 3 ns, leave exactly 120000 ns
*/
bool
d_tests_timer_atomic_threads(void)
{
    const int64_t          total = (int64_t)D_TESTS_TIMER_ATOMIC_THREADS *
                                   D_TESTS_TIMER_ATOMIC_STEPS * 3;
    struct dj_timer_atomic timer = { 0 };

    d_timer_atomic_init(&timer);
    D_TEST_REQUIRE(d_tests_timer_atomic_run(&timer));
    D_TEST_REQUIRE(d_timer_atomic_elapsed(&timer) == total);

    return true;
}

/*
d_tests_timer_atomic_null
  Every function, given NULL.
  Tests the following:
  - the operations return false, and init, set_clock and reset do nothing
  - the queries answer as a stopped timer at zero
*/
bool
d_tests_timer_atomic_null(void)
{
    struct dj_timer_atomic timer = { 0 };
    struct dj_timer        plain = D_TIMER_INIT;

    d_timer_atomic_init(&timer);
    d_timer_atomic_init(NULL);
    d_timer_atomic_set_clock(NULL,
                             NULL,
                             NULL);
    d_timer_atomic_reset(NULL);
    D_TEST_REQUIRE(!d_timer_atomic_init_limited(NULL,
                                                1));
    D_TEST_REQUIRE(!d_timer_atomic_start(NULL));
    D_TEST_REQUIRE(!d_timer_atomic_stop(NULL));
    D_TEST_REQUIRE(!d_timer_atomic_increment(NULL,
                                             1));
    D_TEST_REQUIRE(!d_timer_atomic_decrement(NULL,
                                             1));
    D_TEST_REQUIRE(!d_timer_atomic_merge(NULL,
                                         &timer));
    D_TEST_REQUIRE(!d_timer_atomic_merge(&timer,
                                         NULL));
    D_TEST_REQUIRE(!d_timer_atomic_merge_timer(&timer,
                                               NULL));
    D_TEST_REQUIRE(!d_timer_atomic_snapshot(NULL,
                                            &plain));
    D_TEST_REQUIRE(!d_timer_atomic_snapshot(&timer,
                                            NULL));

    D_TEST_REQUIRE(d_timer_atomic_elapsed(NULL) == 0);
    D_TEST_REQUIRE(d_timer_atomic_remaining(NULL) == 0);
    D_TEST_REQUIRE(!d_timer_atomic_running(NULL));
    D_TEST_REQUIRE(!d_timer_atomic_expired(NULL));
    D_TEST_REQUIRE(d_timer_atomic_compare(NULL,
                                          NULL) == 0);

    return true;
}
