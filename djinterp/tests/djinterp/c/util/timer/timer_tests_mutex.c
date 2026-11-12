/*******************************************************************************
* djinterp [c][utility]                                      timer_tests_mutex.c
*
* The timer suite, block "mutex": timer_mutex.h's guarded timer,
* alone and under eight threads.
*
*
* path:      /tests/djinterp/c/util/timer/timer_tests_mutex.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.09
*******************************************************************************/
#include "./timer_tests.h"  // corresponding header


// D_TESTS_TIMER_THREADS / _STEPS
//   constant: how many threads add time, and how many steps each makes.
#define D_TESTS_TIMER_THREADS  8
#define D_TESTS_TIMER_STEPS    5000

/**
 * @brief A thread's work: D_TESTS_TIMER_STEPS increments of 3 ns.
 *
 * @param[in,out] _timer  the shared struct dj_timer_mutex.
 * @return D_THREAD_SUCCESS.
 */
static dj_thread_result_t
d_tests_timer_mutex_worker(
    void* _timer
)
{
    struct dj_timer_mutex* timer = (struct dj_timer_mutex*)_timer;

    // add time, one step at a time
    for (int i = 0; i < D_TESTS_TIMER_STEPS; ++i)
    {
        (void)d_timer_mutex_increment(timer,
                                      3);
    }

    return D_THREAD_SUCCESS;
}

/*
d_tests_timer_mutex_lifetime
  init and destroy.
  Tests the following:
  - init makes a ready timer with no limit
  - init_limited makes a ready, limited timer
  - destroy leaves the timer not ready, and a second destroy is harmless
*/
bool
d_tests_timer_mutex_lifetime(void)
{
    struct dj_timer_mutex timer = { 0 };

    D_TEST_REQUIRE(d_timer_mutex_init(&timer));
    D_TEST_REQUIRE(timer.ready);
    D_TEST_REQUIRE(!timer.timer.has_limit);
    d_timer_mutex_destroy(&timer);
    D_TEST_REQUIRE(!timer.ready);
    d_timer_mutex_destroy(&timer);

    D_TEST_REQUIRE(d_timer_mutex_init_limited(&timer,
                                              250));
    D_TEST_REQUIRE(timer.timer.has_limit);
    D_TEST_REQUIRE(timer.timer.limit == 250);
    D_TEST_REQUIRE(d_timer_mutex_remaining(&timer) == 250);
    d_timer_mutex_destroy(&timer);

    return true;
}

/*
d_tests_timer_mutex_operations
  One thread, timer.h's behaviour.
  Tests the following:
  - start, stop, increment, decrement and reset act as timer.h's
  - elapsed, running and expired read through the lock
  - a snapshot copies the timer, running state included
  - compare orders by elapsed time, NULL first
*/
bool
d_tests_timer_mutex_operations(void)
{
    struct dj_tests_timer_clock clock    = { 0 };
    struct dj_timer             snapshot = D_TIMER_INIT;
    struct dj_timer_mutex       a        = { 0 };
    struct dj_timer_mutex       b        = { 0 };

    D_TEST_REQUIRE(d_timer_mutex_init_limited(&a,
                                              1000));
    D_TEST_REQUIRE(d_timer_mutex_init(&b));
    d_timer_mutex_set_clock(&a,
                            d_tests_timer_clock_read,
                            &clock);

    D_TEST_REQUIRE(d_timer_mutex_start(&a));
    D_TEST_REQUIRE(!d_timer_mutex_start(&a));
    D_TEST_REQUIRE(d_timer_mutex_running(&a));
    clock.now = 400;
    D_TEST_REQUIRE(d_timer_mutex_snapshot(&a,
                                          &snapshot));
    D_TEST_REQUIRE(snapshot.running);
    D_TEST_REQUIRE(d_timer_elapsed(&snapshot) == 400);
    D_TEST_REQUIRE(d_timer_mutex_stop(&a));
    D_TEST_REQUIRE(!d_timer_mutex_stop(&a));
    D_TEST_REQUIRE(d_timer_mutex_elapsed(&a) == 400);

    D_TEST_REQUIRE(d_timer_mutex_increment(&a,
                                           700));
    D_TEST_REQUIRE(d_timer_mutex_expired(&a));
    D_TEST_REQUIRE(d_timer_mutex_decrement(&a,
                                           200));
    D_TEST_REQUIRE(d_timer_mutex_elapsed(&a) == 900);
    D_TEST_REQUIRE(!d_timer_mutex_expired(&a));

    D_TEST_REQUIRE(d_timer_mutex_increment(&b,
                                           900));
    D_TEST_REQUIRE(d_timer_mutex_compare(&a,
                                         &b) == 0);
    D_TEST_REQUIRE(d_timer_mutex_increment(&b,
                                           1));
    D_TEST_REQUIRE(d_timer_mutex_compare(&a,
                                         &b) == -1);
    D_TEST_REQUIRE(d_timer_mutex_compare(NULL,
                                         &b) == -1);

    d_timer_mutex_reset(&a);
    D_TEST_REQUIRE(d_timer_mutex_elapsed(&a) == 0);

    d_timer_mutex_destroy(&a);
    d_timer_mutex_destroy(&b);

    return true;
}

/*
d_tests_timer_mutex_merge
  Adding time from either form.
  Tests the following:
  - merge adds a guarded source's elapsed time
  - merge_timer adds a plain timer's
  - a source not ready gives nothing, and the merge reports it
*/
bool
d_tests_timer_mutex_merge(void)
{
    struct dj_timer       local = D_TIMER_INIT;
    struct dj_timer_mutex dest  = { 0 };
    struct dj_timer_mutex src   = { 0 };

    D_TEST_REQUIRE(d_timer_mutex_init(&dest));
    D_TEST_REQUIRE(d_timer_mutex_init(&src));
    D_TEST_REQUIRE(d_timer_mutex_increment(&dest,
                                           10));
    D_TEST_REQUIRE(d_timer_mutex_increment(&src,
                                           25));

    D_TEST_REQUIRE(d_timer_mutex_merge(&dest,
                                       &src));
    D_TEST_REQUIRE(d_timer_mutex_elapsed(&dest) == 35);

    D_TEST_REQUIRE(d_timer_increment(&local,
                                     5));
    D_TEST_REQUIRE(d_timer_mutex_merge_timer(&dest,
                                             &local));
    D_TEST_REQUIRE(d_timer_mutex_elapsed(&dest) == 40);

    d_timer_mutex_destroy(&src);
    D_TEST_REQUIRE(!d_timer_mutex_merge(&dest,
                                        &src));
    D_TEST_REQUIRE(d_timer_mutex_elapsed(&dest) == 40);

    d_timer_mutex_destroy(&dest);

    return true;
}

/*
d_tests_timer_mutex_threads
  Contention.
  Tests the following:
  - eight threads, each adding 3 ns 5000 times, leave exactly 120000 ns
*/
bool
d_tests_timer_mutex_threads(void)
{
    dj_thread_t           threads[D_TESTS_TIMER_THREADS] = { 0 };
    struct dj_timer_mutex timer                          = { 0 };
    int                   started                        = 0;

    D_TEST_REQUIRE(d_timer_mutex_init(&timer));

    // start them all
    for (int i = 0; i < D_TESTS_TIMER_THREADS; ++i)
    {
        // a thread that cannot start ends the starting
        if (d_thread_create(&threads[i],
                            d_tests_timer_mutex_worker,
                            &timer) != D_MUTEX_SUCCESS)
        {
            break;
        }

        ++started;
    }

    // and wait for every one that started
    for (int i = 0; i < started; ++i)
    {
        (void)d_thread_join(threads[i],
                            NULL);
    }

    D_TEST_REQUIRE(started == D_TESTS_TIMER_THREADS);
    D_TEST_REQUIRE(d_timer_mutex_elapsed(&timer) ==
                   (int64_t)3 * D_TESTS_TIMER_THREADS * D_TESTS_TIMER_STEPS);

    d_timer_mutex_destroy(&timer);

    return true;
}

/*
d_tests_timer_mutex_not_ready
  A destroyed timer, and NULL.
  Tests the following:
  - every operation on a timer not ready returns false or does nothing
  - every query on it, or on NULL, answers as a stopped timer at zero
  - a snapshot of it, or into NULL, is refused
*/
bool
d_tests_timer_mutex_not_ready(void)
{
    struct dj_timer       plain = D_TIMER_INIT;
    struct dj_timer_mutex timer = { 0 };

    D_TEST_REQUIRE(d_timer_mutex_init(&timer));
    D_TEST_REQUIRE(d_timer_mutex_increment(&timer,
                                           7));
    d_timer_mutex_destroy(&timer);

    D_TEST_REQUIRE(!d_timer_mutex_start(&timer));
    D_TEST_REQUIRE(!d_timer_mutex_stop(&timer));
    D_TEST_REQUIRE(!d_timer_mutex_increment(&timer,
                                            1));
    D_TEST_REQUIRE(!d_timer_mutex_decrement(&timer,
                                            1));
    D_TEST_REQUIRE(!d_timer_mutex_merge_timer(&timer,
                                              &plain));
    d_timer_mutex_reset(&timer);
    d_timer_mutex_set_clock(&timer,
                            NULL,
                            NULL);
    D_TEST_REQUIRE(timer.timer.elapsed == 7);

    D_TEST_REQUIRE(d_timer_mutex_elapsed(&timer) == 0);
    D_TEST_REQUIRE(d_timer_mutex_elapsed(NULL) == 0);
    D_TEST_REQUIRE(d_timer_mutex_remaining(NULL) == 0);
    D_TEST_REQUIRE(!d_timer_mutex_running(NULL));
    D_TEST_REQUIRE(!d_timer_mutex_expired(NULL));
    D_TEST_REQUIRE(!d_timer_mutex_snapshot(&timer,
                                           &plain));
    D_TEST_REQUIRE(!d_timer_mutex_snapshot(NULL,
                                           &plain));
    D_TEST_REQUIRE(!d_timer_mutex_init(NULL));
    D_TEST_REQUIRE(!d_timer_mutex_init_limited(NULL,
                                               0));
    D_TEST_REQUIRE(!d_timer_mutex_init_limited(&timer,
                                               -1));
    D_TEST_REQUIRE(!timer.ready);
    d_timer_mutex_destroy(NULL);

    return true;
}
