/*******************************************************************************
* djinterp [c][utility]                                        timer_tests_ref.c
*
* The timer suite, block "ref": timer.h's interface over any timer --
* struct dj_timer_vtable, struct dj_timer_ref and the d_timer_ref_* functions
* -- on each of the three kinds, and across them.
*
*
* path:      /tests/djinterp/c/util/timer/timer_tests_ref.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#include "./timer_tests.h"  // corresponding header

// std
#include <string.h>  // memset


// D_TESTS_TIMER_REF_KINDS
//   constant: the kinds of timer: plain, atomic and guarded.
#define D_TESTS_TIMER_REF_KINDS  3

/**
 * @brief Whether a kind's table has every slot set.
 *
 * @param[in] _vtable  the table; may be `NULL`.
 * @return `true` when it is a table and no slot is `NULL`.
 */
static bool
d_tests_timer_ref_is_full(
    const struct dj_timer_vtable* _vtable
)
{
    return ( (_vtable)            &&
             (_vtable->start)     &&
             (_vtable->stop)      &&
             (_vtable->increment) &&
             (_vtable->decrement) &&
             (_vtable->merge)     &&
             (_vtable->reset)     &&
             (_vtable->elapsed)   &&
             (_vtable->running)   &&
             (_vtable->snapshot) );
}

/*
d_tests_timer_ref_make
  Making a reference.
  Tests the following:
  - D_TIMER_REF_NONE and a zero-filled reference are no timer
  - each kind's maker refers to the timer it is given, through a table with
    every slot set, one table per kind
  - each maker given NULL gives no timer
*/
bool
d_tests_timer_ref_make(void)
{
    const struct dj_timer_ref none    = D_TIMER_REF_NONE;
    struct dj_timer_ref       filled  = D_TIMER_REF_NONE;
    struct dj_timer           plain   = D_TIMER_INIT;
    struct dj_timer_atomic    shared  = { 0 };
    struct dj_timer_mutex     guarded = { D_TIMER_INIT };
    struct dj_timer_ref       a       = D_TIMER_REF_NONE;
    struct dj_timer_ref       b       = D_TIMER_REF_NONE;
    struct dj_timer_ref       c       = D_TIMER_REF_NONE;

    memset(&filled,
           0,
           sizeof(filled));
    d_timer_atomic_init(&shared);
    D_TEST_REQUIRE(d_timer_mutex_init(&guarded));

    D_TEST_REQUIRE(!d_timer_ref_is_valid(none));
    D_TEST_REQUIRE(!d_timer_ref_is_valid(filled));

    a = d_timer_ref(&plain);
    b = d_timer_atomic_ref(&shared);
    c = d_timer_mutex_ref(&guarded);
    D_TEST_REQUIRE(d_timer_ref_is_valid(a));
    D_TEST_REQUIRE(d_timer_ref_is_valid(b));
    D_TEST_REQUIRE(d_timer_ref_is_valid(c));
    D_TEST_REQUIRE(a.timer == &plain);
    D_TEST_REQUIRE(b.timer == &shared);
    D_TEST_REQUIRE(c.timer == &guarded);
    D_TEST_REQUIRE(d_tests_timer_ref_is_full(a.vtable));
    D_TEST_REQUIRE(d_tests_timer_ref_is_full(b.vtable));
    D_TEST_REQUIRE(d_tests_timer_ref_is_full(c.vtable));
    D_TEST_REQUIRE(a.vtable != b.vtable);
    D_TEST_REQUIRE(b.vtable != c.vtable);
    D_TEST_REQUIRE(a.vtable != c.vtable);

    D_TEST_REQUIRE(!d_timer_ref_is_valid(d_timer_ref(NULL)));
    D_TEST_REQUIRE(!d_timer_ref_is_valid(d_timer_atomic_ref(NULL)));
    D_TEST_REQUIRE(!d_timer_ref_is_valid(d_timer_mutex_ref(NULL)));

    d_timer_mutex_destroy(&guarded);

    return true;
}

/*
d_tests_timer_ref_operations
  The operations and queries, through a reference to each kind, on a
  hand-wound clock.
  Tests the following:
  - start and stop open and close an interval, and a second of either is
    refused
  - elapsed includes an open interval; running says whether one is open
  - increment and decrement add and remove time, and stop at zero
  - reset stops and zeroes
  - a snapshot reads the time, the limit and the clock
*/
bool
d_tests_timer_ref_operations(void)
{
    struct dj_tests_timer_clock clock                         = { 1000 };
    struct dj_timer             plain                         = D_TIMER_INIT;
    struct dj_timer_atomic      shared                        = { 0 };
    struct dj_timer_mutex       guarded                       = { D_TIMER_INIT };
    struct dj_timer             copy                          = D_TIMER_INIT;
    struct dj_timer_ref         refs[D_TESTS_TIMER_REF_KINDS] = { 0 };

    D_TEST_REQUIRE(d_timer_init_limited(&plain,
                                        500));
    D_TEST_REQUIRE(d_timer_atomic_init_limited(&shared,
                                               500));
    D_TEST_REQUIRE(d_timer_mutex_init_limited(&guarded,
                                              500));
    d_timer_set_clock(&plain,
                      d_tests_timer_clock_read,
                      &clock);
    d_timer_atomic_set_clock(&shared,
                             d_tests_timer_clock_read,
                             &clock);
    d_timer_mutex_set_clock(&guarded,
                            d_tests_timer_clock_read,
                            &clock);
    refs[0] = d_timer_ref(&plain);
    refs[1] = d_timer_atomic_ref(&shared);
    refs[2] = d_timer_mutex_ref(&guarded);

    // the same moves on every kind
    for (int i = 0; i < D_TESTS_TIMER_REF_KINDS; ++i)
    {
        D_TEST_NOTE("kind %d (0 plain, 1 atomic, 2 guarded)",
                    i);

        clock.now = 1000;
        D_TEST_REQUIRE(!d_timer_ref_stop(refs[i]));
        D_TEST_REQUIRE(d_timer_ref_start(refs[i]));
        D_TEST_REQUIRE(!d_timer_ref_start(refs[i]));
        D_TEST_REQUIRE(d_timer_ref_running(refs[i]));
        clock.now = 1050;
        D_TEST_REQUIRE(d_timer_ref_elapsed(refs[i]) == 50);
        D_TEST_REQUIRE(d_timer_ref_remaining(refs[i]) == 450);
        D_TEST_REQUIRE(d_timer_ref_stop(refs[i]));
        D_TEST_REQUIRE(!d_timer_ref_running(refs[i]));
        clock.now = 2000;
        D_TEST_REQUIRE(d_timer_ref_elapsed(refs[i]) == 50);

        D_TEST_REQUIRE(d_timer_ref_increment(refs[i],
                                             10));
        D_TEST_REQUIRE(d_timer_ref_decrement(refs[i],
                                             20));
        D_TEST_REQUIRE(d_timer_ref_elapsed(refs[i]) == 40);
        D_TEST_REQUIRE(!d_timer_ref_decrement(refs[i],
                                              100));
        D_TEST_REQUIRE(d_timer_ref_elapsed(refs[i]) == 0);

        D_TEST_REQUIRE(d_timer_ref_increment(refs[i],
                                             500));
        D_TEST_REQUIRE(d_timer_ref_expired(refs[i]));
        D_TEST_REQUIRE(!d_timer_ref_start(refs[i]));

        memset(&copy,
               0,
               sizeof(copy));
        D_TEST_REQUIRE(d_timer_ref_snapshot(refs[i],
                                            &copy));
        D_TEST_REQUIRE(copy.elapsed == 500);
        D_TEST_REQUIRE(copy.has_limit);
        D_TEST_REQUIRE(copy.limit == 500);
        D_TEST_REQUIRE(copy.clock == d_tests_timer_clock_read);

        d_timer_ref_reset(refs[i]);
        D_TEST_REQUIRE(d_timer_ref_elapsed(refs[i]) == 0);
        D_TEST_REQUIRE(!d_timer_ref_running(refs[i]));
        D_TEST_REQUIRE(!d_timer_ref_expired(refs[i]));
    }

    d_timer_mutex_destroy(&guarded);

    return true;
}

/*
d_tests_timer_ref_compare
  Ordering timers of any kinds.
  Tests the following:
  - three-way by elapsed time across kinds
  - no timer orders before any timer, and equal to no timer
*/
bool
d_tests_timer_ref_compare(void)
{
    const struct dj_timer_ref none    = D_TIMER_REF_NONE;
    struct dj_timer           plain   = D_TIMER_INIT;
    struct dj_timer_atomic    shared  = { 0 };
    struct dj_timer_mutex     guarded = { D_TIMER_INIT };
    struct dj_timer_ref       a       = D_TIMER_REF_NONE;
    struct dj_timer_ref       b       = D_TIMER_REF_NONE;
    struct dj_timer_ref       c       = D_TIMER_REF_NONE;

    d_timer_init(&plain);
    d_timer_atomic_init(&shared);
    D_TEST_REQUIRE(d_timer_mutex_init(&guarded));
    a = d_timer_ref(&plain);
    b = d_timer_atomic_ref(&shared);
    c = d_timer_mutex_ref(&guarded);
    D_TEST_REQUIRE(d_timer_ref_increment(a,
                                         1));
    D_TEST_REQUIRE(d_timer_ref_increment(b,
                                         2));
    D_TEST_REQUIRE(d_timer_ref_increment(c,
                                         3));

    D_TEST_REQUIRE(d_timer_ref_compare(a,
                                       b) == -1);
    D_TEST_REQUIRE(d_timer_ref_compare(c,
                                       b) == 1);
    D_TEST_REQUIRE(d_timer_ref_compare(b,
                                       b) == 0);
    D_TEST_REQUIRE(d_timer_ref_increment(a,
                                         2));
    D_TEST_REQUIRE(d_timer_ref_compare(a,
                                       c) == 0);

    D_TEST_REQUIRE(d_timer_ref_compare(none,
                                       a) == -1);
    D_TEST_REQUIRE(d_timer_ref_compare(a,
                                       none) == 1);
    D_TEST_REQUIRE(d_timer_ref_compare(none,
                                       none) == 0);

    d_timer_mutex_destroy(&guarded);

    return true;
}

/*
d_tests_timer_ref_merge
  Merging timers of any kinds.
  Tests the following:
  - each kind merges each other kind's elapsed time
  - a guarded timer merged into itself doubles its time, without a deadlock
  - no timer on either side, or a source that cannot be read, fails and
    adds nothing
*/
bool
d_tests_timer_ref_merge(void)
{
    const struct dj_timer_ref none    = D_TIMER_REF_NONE;
    struct dj_timer           plain   = D_TIMER_INIT;
    struct dj_timer_atomic    shared  = { 0 };
    struct dj_timer_mutex     guarded = { D_TIMER_INIT };
    struct dj_timer_mutex     gone    = { D_TIMER_INIT };
    struct dj_timer_ref       a       = D_TIMER_REF_NONE;
    struct dj_timer_ref       b       = D_TIMER_REF_NONE;
    struct dj_timer_ref       c       = D_TIMER_REF_NONE;

    d_timer_init(&plain);
    d_timer_atomic_init(&shared);
    D_TEST_REQUIRE(d_timer_mutex_init(&guarded));
    a = d_timer_ref(&plain);
    b = d_timer_atomic_ref(&shared);
    c = d_timer_mutex_ref(&guarded);
    D_TEST_REQUIRE(d_timer_ref_increment(a,
                                         5));
    D_TEST_REQUIRE(d_timer_ref_increment(b,
                                         3));
    D_TEST_REQUIRE(d_timer_ref_increment(c,
                                         100));

    // atomic into plain, plain into guarded, guarded into atomic
    D_TEST_REQUIRE(d_timer_ref_merge(a,
                                     b));
    D_TEST_REQUIRE(d_timer_ref_elapsed(a) == 8);
    D_TEST_REQUIRE(d_timer_ref_merge(c,
                                     a));
    D_TEST_REQUIRE(d_timer_ref_elapsed(c) == 108);
    D_TEST_REQUIRE(d_timer_ref_merge(b,
                                     c));
    D_TEST_REQUIRE(d_timer_ref_elapsed(b) == 111);

    D_TEST_REQUIRE(d_timer_ref_merge(c,
                                     c));
    D_TEST_REQUIRE(d_timer_ref_elapsed(c) == 216);

    D_TEST_REQUIRE(!d_timer_ref_merge(none,
                                      a));
    D_TEST_REQUIRE(!d_timer_ref_merge(a,
                                      none));
    D_TEST_REQUIRE(!d_timer_ref_merge(a,
                                      d_timer_mutex_ref(&gone)));
    D_TEST_REQUIRE(d_timer_ref_elapsed(a) == 8);

    d_timer_mutex_destroy(&guarded);

    return true;
}

/*
d_tests_timer_ref_none
  A reference to no timer, and to a timer that cannot be read.
  Tests the following:
  - on no timer (no table, or no timer), the operations fail, reset does
    nothing and the queries answer as a stopped timer at zero
  - a snapshot needs somewhere to go
  - a guarded timer not set up is referred to, but cannot be moved or read
*/
bool
d_tests_timer_ref_none(void)
{
    const struct dj_timer_ref none     = D_TIMER_REF_NONE;
    struct dj_timer           plain    = D_TIMER_INIT;
    struct dj_timer           copy     = D_TIMER_INIT;
    struct dj_timer_mutex     gone     = { D_TIMER_INIT };
    struct dj_timer_ref       headless = D_TIMER_REF_NONE;
    struct dj_timer_ref       refs[2]  = { 0 };

    headless.vtable = d_timer_ref(&plain).vtable;
    refs[0]         = none;
    refs[1]         = headless;

    // no timer, either way
    for (int i = 0; i < 2; ++i)
    {
        D_TEST_REQUIRE(!d_timer_ref_is_valid(refs[i]));
        D_TEST_REQUIRE(!d_timer_ref_start(refs[i]));
        D_TEST_REQUIRE(!d_timer_ref_stop(refs[i]));
        D_TEST_REQUIRE(!d_timer_ref_increment(refs[i],
                                              1));
        D_TEST_REQUIRE(!d_timer_ref_decrement(refs[i],
                                              1));
        d_timer_ref_reset(refs[i]);
        D_TEST_REQUIRE(d_timer_ref_elapsed(refs[i]) == 0);
        D_TEST_REQUIRE(d_timer_ref_remaining(refs[i]) == 0);
        D_TEST_REQUIRE(!d_timer_ref_running(refs[i]));
        D_TEST_REQUIRE(!d_timer_ref_expired(refs[i]));
        D_TEST_REQUIRE(!d_timer_ref_snapshot(refs[i],
                                             &copy));
    }

    D_TEST_REQUIRE(!d_timer_ref_snapshot(d_timer_ref(&plain),
                                         NULL));

    D_TEST_REQUIRE(d_timer_ref_is_valid(d_timer_mutex_ref(&gone)));
    D_TEST_REQUIRE(!d_timer_ref_start(d_timer_mutex_ref(&gone)));
    D_TEST_REQUIRE(!d_timer_ref_snapshot(d_timer_mutex_ref(&gone),
                                         &copy));
    D_TEST_REQUIRE(d_timer_ref_elapsed(d_timer_mutex_ref(&gone)) == 0);

    return true;
}
