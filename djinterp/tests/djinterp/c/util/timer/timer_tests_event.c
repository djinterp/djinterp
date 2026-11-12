/*******************************************************************************
* djinterp [c][utility]                                      timer_tests_event.c
*
* The timer suite, block "event": timer_event.h's bindings, fired through a
* real dispatcher from the event core, on a hand-wound clock.
*
*
* path:      /tests/djinterp/c/util/timer/timer_tests_event.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.09
*******************************************************************************/
#include "./timer_tests.h"  // corresponding header


// D_TESTS_TIMER_KEY_*
//   constant: the events the tests time.
#define D_TESTS_TIMER_KEY_OPEN   ((dj_event_key)201)
#define D_TESTS_TIMER_KEY_CLOSE  ((dj_event_key)202)
#define D_TESTS_TIMER_KEY_MORE   ((dj_event_key)203)
#define D_TESTS_TIMER_KEY_LESS   ((dj_event_key)204)
#define D_TESTS_TIMER_KEY_TOOK   ((dj_event_key)205)
#define D_TESTS_TIMER_KEY_CLEAR  ((dj_event_key)206)

/*
d_tests_timer_event_init
  The one init function, over every kind.
  Tests the following:
  - init sets the target, the operation and the amount
  - a reference to each kind carries that kind's table, so one binding type
    aims at any timer
*/
bool
d_tests_timer_event_init(void)
{
    struct dj_timer       plain   = D_TIMER_INIT;
    struct dj_timer_mutex shared  = { 0 };
    struct dj_timer_event binding = { 0 };

    D_TEST_REQUIRE(d_timer_mutex_init(&shared));

    d_timer_event_init(&binding,
                       d_timer_ref(&plain),
                       D_TIMER_EVENT_INCREMENT,
                       9);
    D_TEST_REQUIRE(binding.target.timer == &plain);
    D_TEST_REQUIRE(binding.target.vtable == d_timer_ref(&plain).vtable);
    D_TEST_REQUIRE(binding.op == D_TIMER_EVENT_INCREMENT);
    D_TEST_REQUIRE(binding.amount == 9);

    d_timer_event_init(&binding,
                       d_timer_mutex_ref(&shared),
                       D_TIMER_EVENT_STOP,
                       0);
    D_TEST_REQUIRE(binding.target.timer == &shared);
    D_TEST_REQUIRE(binding.target.vtable != d_timer_ref(&plain).vtable);
    D_TEST_REQUIRE(binding.op == D_TIMER_EVENT_STOP);

    d_timer_mutex_destroy(&shared);

    return true;
}

/*
d_tests_timer_event_span
  Timing between two events.
  Tests the following:
  - START on one key and STOP on another time the span between firings
  - spans accumulate
  - a second START while running, or a STOP while stopped, changes nothing
*/
bool
d_tests_timer_event_span(void)
{
    struct dj_tests_timer_clock clock      = { 100 };
    struct dj_event_dispatcher  dispatcher = { 0 };
    struct dj_timer             timer      = D_TIMER_INIT;
    struct dj_timer_event       open       = { 0 };
    struct dj_timer_event       close      = { 0 };

    D_TEST_REQUIRE(d_event_dispatcher_init(&dispatcher,
                                           0,
                                           0,
                                           0,
                                           0) ==
                   D_EVENT_OK);
    d_timer_set_clock(&timer,
                      d_tests_timer_clock_read,
                      &clock);

    d_timer_event_init(&open,
                       d_timer_ref(&timer),
                       D_TIMER_EVENT_START,
                       0);
    d_timer_event_init(&close,
                       d_timer_ref(&timer),
                       D_TIMER_EVENT_STOP,
                       0);
    D_TEST_REQUIRE(d_event_dispatcher_bind(&dispatcher,
                                           D_TESTS_TIMER_KEY_OPEN,
                                           d_timer_event_step(&open),
                                           NULL,
                                           NULL) == D_EVENT_OK);
    D_TEST_REQUIRE(d_event_dispatcher_bind(&dispatcher,
                                           D_TESTS_TIMER_KEY_CLOSE,
                                           d_timer_event_step(&close),
                                           NULL,
                                           NULL) == D_EVENT_OK);

    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TIMER_KEY_OPEN,
                                  NULL);
    clock.now = 160;
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TIMER_KEY_OPEN,
                                  NULL);
    clock.now = 175;
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TIMER_KEY_CLOSE,
                                  NULL);
    D_TEST_REQUIRE(d_timer_elapsed(&timer) == 75);

    clock.now = 500;
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TIMER_KEY_CLOSE,
                                  NULL);
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TIMER_KEY_OPEN,
                                  NULL);
    clock.now = 525;
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TIMER_KEY_CLOSE,
                                  NULL);
    D_TEST_REQUIRE(d_timer_elapsed(&timer) == 100);

    d_event_dispatcher_dispose(&dispatcher);

    return true;
}

/*
d_tests_timer_event_operations
  Moving time by event.
  Tests the following:
  - INCREMENT and DECREMENT add and remove the binding's amount
  - ADD adds the int64_t nanoseconds the payload holds, and a NULL payload
    adds nothing
  - RESET stops the timer and zeroes it
*/
bool
d_tests_timer_event_operations(void)
{
    struct dj_event_dispatcher dispatcher = { 0 };
    struct dj_timer            timer      = D_TIMER_INIT;
    struct dj_timer_event      more       = { 0 };
    struct dj_timer_event      less       = { 0 };
    struct dj_timer_event      took       = { 0 };
    struct dj_timer_event      clear      = { 0 };
    int64_t                    span       = 1234;

    D_TEST_REQUIRE(d_event_dispatcher_init(&dispatcher,
                                           0,
                                           0,
                                           0,
                                           0) ==
                   D_EVENT_OK);

    d_timer_event_init(&more,
                       d_timer_ref(&timer),
                       D_TIMER_EVENT_INCREMENT,
                       50);
    d_timer_event_init(&less,
                       d_timer_ref(&timer),
                       D_TIMER_EVENT_DECREMENT,
                       20);
    d_timer_event_init(&took,
                       d_timer_ref(&timer),
                       D_TIMER_EVENT_ADD,
                       0);
    d_timer_event_init(&clear,
                       d_timer_ref(&timer),
                       D_TIMER_EVENT_RESET,
                       0);
    D_TEST_REQUIRE(d_event_dispatcher_bind(&dispatcher,
                                           D_TESTS_TIMER_KEY_MORE,
                                           d_timer_event_step(&more),
                                           NULL,
                                           NULL) == D_EVENT_OK);
    D_TEST_REQUIRE(d_event_dispatcher_bind(&dispatcher,
                                           D_TESTS_TIMER_KEY_LESS,
                                           d_timer_event_step(&less),
                                           NULL,
                                           NULL) == D_EVENT_OK);
    D_TEST_REQUIRE(d_event_dispatcher_bind(&dispatcher,
                                           D_TESTS_TIMER_KEY_TOOK,
                                           d_timer_event_step(&took),
                                           NULL,
                                           NULL) == D_EVENT_OK);
    D_TEST_REQUIRE(d_event_dispatcher_bind(&dispatcher,
                                           D_TESTS_TIMER_KEY_CLEAR,
                                           d_timer_event_step(&clear),
                                           NULL,
                                           NULL) == D_EVENT_OK);

    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TIMER_KEY_MORE,
                                  NULL);
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TIMER_KEY_LESS,
                                  NULL);
    D_TEST_REQUIRE(d_timer_elapsed(&timer) == 30);

    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TIMER_KEY_TOOK,
                                  &span);
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TIMER_KEY_TOOK,
                                  NULL);
    D_TEST_REQUIRE(d_timer_elapsed(&timer) == 1264);

    D_TEST_REQUIRE(d_timer_start(&timer));
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TIMER_KEY_CLEAR,
                                  NULL);
    D_TEST_REQUIRE(!d_timer_running(&timer));
    D_TEST_REQUIRE(d_timer_elapsed(&timer) == 0);

    d_event_dispatcher_dispose(&dispatcher);

    return true;
}

/*
d_tests_timer_event_mutex
  A guarded target.
  Tests the following:
  - START, STOP and ADD drive a guarded timer as they drive a plain one
*/
bool
d_tests_timer_event_mutex(void)
{
    struct dj_tests_timer_clock clock      = { 0 };
    struct dj_event_dispatcher  dispatcher = { 0 };
    struct dj_timer_mutex       shared     = { 0 };
    struct dj_timer_event       open       = { 0 };
    struct dj_timer_event       close      = { 0 };
    struct dj_timer_event       took       = { 0 };
    int64_t                     span       = 8;

    D_TEST_REQUIRE(d_event_dispatcher_init(&dispatcher,
                                           0,
                                           0,
                                           0,
                                           0) ==
                   D_EVENT_OK);
    D_TEST_REQUIRE(d_timer_mutex_init(&shared));
    d_timer_mutex_set_clock(&shared,
                            d_tests_timer_clock_read,
                            &clock);

    d_timer_event_init(&open,
                       d_timer_mutex_ref(&shared),
                       D_TIMER_EVENT_START,
                       0);
    d_timer_event_init(&close,
                       d_timer_mutex_ref(&shared),
                       D_TIMER_EVENT_STOP,
                       0);
    d_timer_event_init(&took,
                       d_timer_mutex_ref(&shared),
                       D_TIMER_EVENT_ADD,
                       0);
    D_TEST_REQUIRE(d_event_dispatcher_bind(&dispatcher,
                                           D_TESTS_TIMER_KEY_OPEN,
                                           d_timer_event_step(&open),
                                           NULL,
                                           NULL) == D_EVENT_OK);
    D_TEST_REQUIRE(d_event_dispatcher_bind(&dispatcher,
                                           D_TESTS_TIMER_KEY_CLOSE,
                                           d_timer_event_step(&close),
                                           NULL,
                                           NULL) == D_EVENT_OK);
    D_TEST_REQUIRE(d_event_dispatcher_bind(&dispatcher,
                                           D_TESTS_TIMER_KEY_TOOK,
                                           d_timer_event_step(&took),
                                           NULL,
                                           NULL) == D_EVENT_OK);

    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TIMER_KEY_OPEN,
                                  NULL);
    clock.now = 40;
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TIMER_KEY_CLOSE,
                                  NULL);
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TIMER_KEY_TOOK,
                                  &span);
    D_TEST_REQUIRE(d_timer_mutex_elapsed(&shared) == 48);
    D_TEST_REQUIRE(!d_timer_mutex_running(&shared));

    d_timer_mutex_destroy(&shared);
    d_event_dispatcher_dispose(&dispatcher);

    return true;
}

/*
d_tests_timer_event_null
  The edges.
  Tests the following:
  - the step on a NULL binding, or one with no target, passes and does
    nothing
  - the step is d_timer_event_apply over the binding, which is all the
    tie-in hands off: binding it is the event core's
  - init on NULL does nothing
*/
bool
d_tests_timer_event_null(void)
{
    struct dj_timer_event binding = { 0 };
    struct dj_event_step  step    = { 0 };

    d_timer_event_init(&binding,
                       d_timer_ref(NULL),
                       D_TIMER_EVENT_START,
                       0);
    D_TEST_REQUIRE(!d_timer_ref_is_valid(binding.target));

    D_TEST_REQUIRE(d_timer_event_apply(NULL,
                                       NULL) == D_VERDICT_PASS);
    D_TEST_REQUIRE(d_timer_event_apply(NULL,
                                       &binding) == D_VERDICT_PASS);

    step = d_timer_event_step(&binding);
    D_TEST_REQUIRE(step.fn == d_timer_event_apply);
    D_TEST_REQUIRE(step.state == &binding);

    d_timer_event_init(NULL,
                       d_timer_ref(NULL),
                       D_TIMER_EVENT_RESET,
                       0);

    return true;
}
