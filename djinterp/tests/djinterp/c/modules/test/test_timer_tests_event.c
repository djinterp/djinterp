/*******************************************************************************
* djinterp [c][test]                                    test_timer_tests_event.c
*
* The test_timer suite, block "event": test_timer_event.h's span, a start and
* a stop binding on one test timer, bound on the event core.
*
*
* path:      /tests/djinterp/c/modules/test/test_timer_tests_event.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#include "./test_timer_tests.h"  // corresponding header


// D_TESTS_TEST_TIMER_EVENT_*
//   constant: the events the tests fire.
#define D_TESTS_TEST_TIMER_EVENT_BEGIN  ((dj_event_key)301)
#define D_TESTS_TEST_TIMER_EVENT_END    ((dj_event_key)302)
#define D_TESTS_TEST_TIMER_EVENT_OTHER  ((dj_event_key)303)

/**
 * @brief Sets up a dispatcher with a span bound: `_start` on BEGIN and
 *        `_stop` on END.
 *
 * @param[out] _dispatcher  the dispatcher.
 * @param[in]  _start       the start binding.
 * @param[in]  _stop        the stop binding.
 * @return `true` when the dispatcher was set up and both were bound.
 */
static bool
d_tests_test_timer_event_bind(
    struct dj_event_dispatcher* _dispatcher,
    struct dj_timer_event*      _start,
    struct dj_timer_event*      _stop
)
{
    dj_handler_id id = D_HANDLER_ID_NULL;

    return ( (d_event_dispatcher_init(_dispatcher,
                                      0,
                                      0,
                                      0,
                                      0) == D_EVENT_OK) &&
             (d_event_dispatcher_bind(_dispatcher,
                                      D_TESTS_TEST_TIMER_EVENT_BEGIN,
                                      d_timer_event_step(_start),
                                      NULL,
                                      &id) == D_EVENT_OK) &&
             (d_event_dispatcher_bind(_dispatcher,
                                      D_TESTS_TEST_TIMER_EVENT_END,
                                      d_timer_event_step(_stop),
                                      NULL,
                                      &id) == D_EVENT_OK) );
}

/*
d_tests_test_timer_event_init
  Aiming the two bindings.
  Tests the following:
  - both aim at the timer the test timer wraps, through a plain timer's
    reference
  - one starts it and one stops it
*/
bool
d_tests_test_timer_event_init(void)
{
    struct dj_test_timer  timer = D_TEST_TIMER_INIT;
    struct dj_timer       plain = D_TIMER_INIT;
    struct dj_timer_event start = { 0 };
    struct dj_timer_event stop  = { 0 };

    d_test_timer_init(&timer);
    d_test_timer_event_init(&start,
                            &stop,
                            &timer);

    D_TEST_REQUIRE(start.target.timer == &timer.timer);
    D_TEST_REQUIRE(stop.target.timer == &timer.timer);
    D_TEST_REQUIRE(start.target.vtable == d_timer_ref(&plain).vtable);
    D_TEST_REQUIRE(start.op == D_TIMER_EVENT_START);
    D_TEST_REQUIRE(stop.op == D_TIMER_EVENT_STOP);

    return true;
}

/*
d_tests_test_timer_event_span
  Timing between two events.
  Tests the following:
  - the start event opens an interval at the clock's reading and the stop
    event closes it
  - spans accumulate; other events move nothing
*/
bool
d_tests_test_timer_event_span(void)
{
    struct dj_tests_test_timer_clock clock      = { 1000 };
    struct dj_event_dispatcher       dispatcher = { 0 };
    struct dj_test_timer             timer      = D_TEST_TIMER_INIT;
    struct dj_timer_event            start      = { 0 };
    struct dj_timer_event            stop       = { 0 };

    d_test_timer_init(&timer);
    d_test_timer_set_clock(&timer,
                           d_tests_test_timer_clock_read,
                           &clock);
    d_test_timer_event_init(&start,
                            &stop,
                            &timer);
    D_TEST_REQUIRE(d_tests_test_timer_event_bind(&dispatcher,
                                                 &start,
                                                 &stop));

    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TEST_TIMER_EVENT_BEGIN,
                                  NULL);
    D_TEST_REQUIRE(d_test_timer_running(&timer));
    clock.now = 1250;
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TEST_TIMER_EVENT_OTHER,
                                  NULL);
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TEST_TIMER_EVENT_END,
                                  NULL);
    D_TEST_REQUIRE(!d_test_timer_running(&timer));
    D_TEST_REQUIRE(d_test_timer_elapsed(&timer) == 250);

    clock.now = 2000;
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TEST_TIMER_EVENT_BEGIN,
                                  NULL);
    clock.now = 2050;
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TEST_TIMER_EVENT_END,
                                  NULL);
    D_TEST_REQUIRE(d_test_timer_elapsed(&timer) == 300);

    d_event_dispatcher_dispose(&dispatcher);

    return true;
}

/*
d_tests_test_timer_event_twice
  Events out of order.
  Tests the following:
  - a start that arrives while running does not move the interval's start
  - a stop that arrives while stopped adds no time
*/
bool
d_tests_test_timer_event_twice(void)
{
    struct dj_tests_test_timer_clock clock      = { 0 };
    struct dj_event_dispatcher       dispatcher = { 0 };
    struct dj_test_timer             timer      = D_TEST_TIMER_INIT;
    struct dj_timer_event            start      = { 0 };
    struct dj_timer_event            stop       = { 0 };

    d_test_timer_init(&timer);
    d_test_timer_set_clock(&timer,
                           d_tests_test_timer_clock_read,
                           &clock);
    d_test_timer_event_init(&start,
                            &stop,
                            &timer);
    D_TEST_REQUIRE(d_tests_test_timer_event_bind(&dispatcher,
                                                 &start,
                                                 &stop));

    clock.now = 10;
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TEST_TIMER_EVENT_END,
                                  NULL);
    D_TEST_REQUIRE(d_test_timer_elapsed(&timer) == 0);

    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TEST_TIMER_EVENT_BEGIN,
                                  NULL);
    clock.now = 40;
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TEST_TIMER_EVENT_BEGIN,
                                  NULL);
    clock.now = 70;
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TEST_TIMER_EVENT_END,
                                  NULL);
    D_TEST_REQUIRE(d_test_timer_elapsed(&timer) == 60);

    clock.now = 90;
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TEST_TIMER_EVENT_END,
                                  NULL);
    D_TEST_REQUIRE(d_test_timer_elapsed(&timer) == 60);

    d_event_dispatcher_dispose(&dispatcher);

    return true;
}

/*
d_tests_test_timer_event_null
  Bindings with nothing to time.
  Tests the following:
  - bindings aimed at no timer refer to none, and pass when applied
  - init with NULL bindings does nothing
*/
bool
d_tests_test_timer_event_null(void)
{
    struct dj_test_timer  timer = D_TEST_TIMER_INIT;
    struct dj_timer_event start = { 0 };
    struct dj_timer_event stop  = { 0 };

    d_test_timer_event_init(&start,
                            &stop,
                            NULL);
    D_TEST_REQUIRE(!d_timer_ref_is_valid(start.target));
    D_TEST_REQUIRE(!d_timer_ref_is_valid(stop.target));
    D_TEST_REQUIRE(d_timer_event_apply(NULL,
                                       &start) == D_VERDICT_PASS);
    D_TEST_REQUIRE(d_timer_event_apply(NULL,
                                       &stop) == D_VERDICT_PASS);

    d_test_timer_init(&timer);
    d_test_timer_event_init(NULL,
                            NULL,
                            &timer);
    d_test_timer_event_init(&start,
                            NULL,
                            &timer);
    D_TEST_REQUIRE(start.target.timer == &timer.timer);

    return true;
}
