/*******************************************************************************
* djinterp [c][test]                                  test_counter_tests_event.c
*
* The test_counter suite, block "event": test_counter_event.h's two bindings,
* bound on the event core as any handler is.
*
*
* path:      /tests/djinterp/c/modules/test/test_counter_tests_event.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#include "./test_counter_tests.h"  // corresponding header

// std
#include <string.h>  // memset


// D_TESTS_TEST_COUNTER_EVENT_*
//   constant: the events the tests fire. Event keys and tally keys are apart:
// a binding names the tally key it records, whatever event it is bound to.
#define D_TESTS_TEST_COUNTER_EVENT_PASS   ((dj_event_key)201)
#define D_TESTS_TEST_COUNTER_EVENT_FAIL   ((dj_event_key)202)
#define D_TESTS_TEST_COUNTER_EVENT_OTHER  ((dj_event_key)203)

/*
d_tests_test_counter_event_tally
  A tally binding.
  Tests the following:
  - init aims the binding at a tally and a key
  - each firing of the bound event records one occurrence of the key, which
    it tracks on the first
  - firing another event records nothing; after unbinding, neither does the
    bound one
  - apply passes the event on, whatever the payload
*/
bool
d_tests_test_counter_event_tally(void)
{
    struct dj_event_dispatcher dispatcher = { 0 };
    struct dj_test_tally       tally      = { 0 };
    struct dj_test_tally_event passes     = { 0 };
    struct dj_test_tally_event fails      = { 0 };
    dj_handler_id              id         = D_HANDLER_ID_NULL;
    dj_handler_id              other      = D_HANDLER_ID_NULL;
    int64_t                    step       = 0;

    memset(&tally,
           0,
           sizeof(tally));
    D_TEST_REQUIRE(d_test_tally_init(&tally,
                                     NULL));
    step = d_test_counter_step(d_test_tally_track(
                                   &tally,
                                   D_TESTS_TEST_COUNTER_KEY_ERROR));
    D_TEST_REQUIRE(d_event_dispatcher_init(&dispatcher,
                                           0,
                                           0,
                                           0,
                                           0) == D_EVENT_OK);

    d_test_tally_event_init(&passes,
                            &tally,
                            D_TESTS_TEST_COUNTER_KEY_PASSED);
    d_test_tally_event_init(&fails,
                            &tally,
                            D_TESTS_TEST_COUNTER_KEY_FAILED);
    D_TEST_REQUIRE(passes.tally == &tally);
    D_TEST_REQUIRE(passes.key == D_TESTS_TEST_COUNTER_KEY_PASSED);

    D_TEST_REQUIRE(d_event_dispatcher_bind(&dispatcher,
                                           D_TESTS_TEST_COUNTER_EVENT_PASS,
                                           d_test_tally_event_step(&passes),
                                           NULL,
                                           &id) == D_EVENT_OK);
    D_TEST_REQUIRE(d_event_dispatcher_bind(&dispatcher,
                                           D_TESTS_TEST_COUNTER_EVENT_FAIL,
                                           d_test_tally_event_step(&fails),
                                           NULL,
                                           &other) == D_EVENT_OK);

    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TEST_COUNTER_EVENT_PASS,
                                  NULL);
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TEST_COUNTER_EVENT_PASS,
                                  NULL);
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TEST_COUNTER_EVENT_FAIL,
                                  NULL);
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TEST_COUNTER_EVENT_OTHER,
                                  NULL);
    D_TEST_REQUIRE(d_test_tally_count(&tally,
                                      D_TESTS_TEST_COUNTER_KEY_PASSED) ==
                   2 * step);
    D_TEST_REQUIRE(d_test_tally_count(&tally,
                                      D_TESTS_TEST_COUNTER_KEY_FAILED) ==
                   step);

    D_TEST_REQUIRE(d_event_dispatcher_unbind(&dispatcher,
                                             id) == D_EVENT_OK);
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TEST_COUNTER_EVENT_PASS,
                                  NULL);
    D_TEST_REQUIRE(d_test_tally_count(&tally,
                                      D_TESTS_TEST_COUNTER_KEY_PASSED) ==
                   2 * step);

    D_TEST_REQUIRE(d_test_tally_event_apply(&step,
                                            &fails) == D_VERDICT_PASS);
    D_TEST_REQUIRE(d_test_tally_count(&tally,
                                      D_TESTS_TEST_COUNTER_KEY_FAILED) ==
                   2 * step);

    d_event_dispatcher_dispose(&dispatcher);

    return true;
}

/*
d_tests_test_counter_event_counter
  A test counter's binding, which is counter_event.h's.
  Tests the following:
  - init aims a counter_event.h binding at the test counter, to move it by
    its step, as an increment
  - each firing moves the test counter by that step
  - the step is read at init: a counter set up again keeps the old step in
    its binding until the binding is set again
*/
bool
d_tests_test_counter_event_counter(void)
{
    struct dj_event_dispatcher     dispatcher = { 0 };
    struct dj_test_counter_options by_four    = { 4, false };
    struct dj_test_counter_options by_one     = { 1, false };
    struct dj_test_counter         counter    = D_TEST_COUNTER_INIT;
    struct dj_counter_event        binding    = { 0 };
    dj_handler_id                  id         = D_HANDLER_ID_NULL;

    D_TEST_REQUIRE(d_test_counter_init_with(&counter,
                                            &by_four));
    D_TEST_REQUIRE(d_event_dispatcher_init(&dispatcher,
                                           0,
                                           0,
                                           0,
                                           0) == D_EVENT_OK);

    d_test_counter_event_init(&binding,
                              &counter);
    D_TEST_REQUIRE(binding.target.counter == &counter.counter);
    D_TEST_REQUIRE(binding.op == D_COUNTER_EVENT_INCREMENT);
    D_TEST_REQUIRE(binding.amount == 4);

    D_TEST_REQUIRE(d_event_dispatcher_bind(&dispatcher,
                                           D_TESTS_TEST_COUNTER_EVENT_PASS,
                                           d_counter_event_step(&binding),
                                           NULL,
                                           &id) == D_EVENT_OK);
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TEST_COUNTER_EVENT_PASS,
                                  NULL);
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TEST_COUNTER_EVENT_PASS,
                                  NULL);
    D_TEST_REQUIRE(d_test_counter_value(&counter) == 8);

    D_TEST_REQUIRE(d_test_counter_init_with(&counter,
                                            &by_one));
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TEST_COUNTER_EVENT_PASS,
                                  NULL);
    D_TEST_REQUIRE(d_test_counter_value(&counter) == 4);

    d_test_counter_event_init(&binding,
                              &counter);
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_TEST_COUNTER_EVENT_PASS,
                                  NULL);
    D_TEST_REQUIRE(d_test_counter_value(&counter) == 5);

    d_event_dispatcher_dispose(&dispatcher);

    return true;
}

/*
d_tests_test_counter_event_full
  A binding on a full tally.
  Tests the following:
  - a refusing tally loses the occurrence, and the event still passes on
  - an overflowing tally counts it in the overflow
*/
bool
d_tests_test_counter_event_full(void)
{
    struct dj_test_tally         tally   = { 0 };
    struct dj_test_tally_options options = { 0 };
    struct dj_test_tally_event   binding = { 0 };

    options = d_tests_test_counter_options(1,
                                           D_TEST_TALLY_FULL_REFUSE,
                                           1);
    D_TEST_REQUIRE(d_test_tally_init(&tally,
                                     &options));
    D_TEST_REQUIRE(d_test_tally_record(&tally,
                                       D_TESTS_TEST_COUNTER_KEY_PASSED));
    d_test_tally_event_init(&binding,
                            &tally,
                            D_TESTS_TEST_COUNTER_KEY_FAILED);

    D_TEST_REQUIRE(d_test_tally_event_apply(NULL,
                                            &binding) == D_VERDICT_PASS);
    D_TEST_REQUIRE(d_test_tally_count(&tally,
                                      D_TESTS_TEST_COUNTER_KEY_FAILED) == 0);
    D_TEST_REQUIRE(d_test_tally_overflow(&tally) == 0);

    options.when_full = D_TEST_TALLY_FULL_OVERFLOW;
    D_TEST_REQUIRE(d_test_tally_init(&tally,
                                     &options));
    D_TEST_REQUIRE(d_test_tally_record(&tally,
                                       D_TESTS_TEST_COUNTER_KEY_PASSED));
    D_TEST_REQUIRE(d_test_tally_event_apply(NULL,
                                            &binding) == D_VERDICT_PASS);
    D_TEST_REQUIRE(d_test_tally_event_apply(NULL,
                                            &binding) == D_VERDICT_PASS);
    D_TEST_REQUIRE(d_test_tally_overflow(&tally) == 2);

    return true;
}

/*
d_tests_test_counter_event_null
  Bindings with nothing to move.
  Tests the following:
  - a tally binding with no tally, or a NULL binding, passes and records
    nothing
  - a counter binding on no counter passes and moves nothing
  - init of a NULL binding does nothing
  - the step hands off the binding it was given
*/
bool
d_tests_test_counter_event_null(void)
{
    struct dj_test_tally_event tally_binding   = { 0 };
    struct dj_counter_event    counter_binding = { 0 };
    struct dj_event_step       step            = { 0 };

    d_test_tally_event_init(&tally_binding,
                            NULL,
                            D_TESTS_TEST_COUNTER_KEY_PASSED);
    D_TEST_REQUIRE(tally_binding.tally == NULL);
    D_TEST_REQUIRE(d_test_tally_event_apply(NULL,
                                            &tally_binding) ==
                   D_VERDICT_PASS);
    D_TEST_REQUIRE(d_test_tally_event_apply(NULL,
                                            NULL) == D_VERDICT_PASS);
    d_test_tally_event_init(NULL,
                            NULL,
                            D_TESTS_TEST_COUNTER_KEY_PASSED);

    d_test_counter_event_init(&counter_binding,
                              NULL);
    D_TEST_REQUIRE(!d_counter_ref_is_valid(counter_binding.target));
    D_TEST_REQUIRE(d_counter_event_apply(NULL,
                                         &counter_binding) ==
                   D_VERDICT_PASS);
    d_test_counter_event_init(NULL,
                              NULL);

    step = d_test_tally_event_step(&tally_binding);
    D_TEST_REQUIRE(step.state == &tally_binding);
    D_TEST_REQUIRE(step.fn != NULL);

    return true;
}
