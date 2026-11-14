/*******************************************************************************
* djinterp [c][utility]                                    counter_tests_event.c
*
* The counter suite, block "event": counter_event.h's bindings, fired through
* a real dispatcher from the event core.
*
*
* path:      /tests/djinterp/c/util/counter/counter_tests_event.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "./counter_tests.h"  // corresponding header


// D_TESTS_COUNTER_KEY_*
//   constant: the events the tests count. Any keys would do, the event core's
// name hashes (d_event_key_of_name) among them; small numbers read better in
// a failure.
#define D_TESTS_COUNTER_KEY_HIT    ((dj_event_key)101)
#define D_TESTS_COUNTER_KEY_MISS   ((dj_event_key)102)
#define D_TESTS_COUNTER_KEY_UNDO   ((dj_event_key)103)
#define D_TESTS_COUNTER_KEY_BYTES  ((dj_event_key)104)
#define D_TESTS_COUNTER_KEY_CLEAR  ((dj_event_key)105)

/*
d_tests_counter_event_init
  The one init function, over every kind.
  Tests the following:
  - init sets the target, the operation and the amount
  - a reference to each kind carries that kind's table, so one binding type
    aims at any counter
*/
bool
d_tests_counter_event_init(void)
{
    struct dj_counter        plain   = D_COUNTER_INIT;
    struct dj_counter_atomic shared  = { 0 };
    struct dj_counter_event  binding = { 0 };

    d_counter_atomic_init(&shared,
                          0);

    d_counter_event_init(&binding,
                         d_counter_ref(&plain),
                         D_COUNTER_EVENT_DECREMENT,
                         7);
    D_TEST_REQUIRE(binding.target.counter == &plain);
    D_TEST_REQUIRE(binding.target.vtable == d_counter_ref(&plain).vtable);
    D_TEST_REQUIRE(binding.op == D_COUNTER_EVENT_DECREMENT);
    D_TEST_REQUIRE(binding.amount == 7);

    d_counter_event_init(&binding,
                         d_counter_atomic_ref(&shared),
                         D_COUNTER_EVENT_RESET,
                         0);
    D_TEST_REQUIRE(binding.target.counter == &shared);
    D_TEST_REQUIRE(binding.target.vtable != d_counter_ref(&plain).vtable);
    D_TEST_REQUIRE(binding.op == D_COUNTER_EVENT_RESET);

    return true;
}

/*
d_tests_counter_event_counts
  Counting an event.
  Tests the following:
  - each firing of the bound key moves the counter by the amount
  - firing another key moves nothing
  - after unbinding, firing moves nothing
*/
bool
d_tests_counter_event_counts(void)
{
    struct dj_event_dispatcher dispatcher = { 0 };
    struct dj_counter          hits       = D_COUNTER_INIT;
    struct dj_counter_event    binding    = { 0 };
    dj_handler_id              id         = D_HANDLER_ID_NULL;

    D_TEST_REQUIRE(d_event_dispatcher_init(&dispatcher,
                                           0,
                                           0,
                                           0,
                                           0) ==
                   D_EVENT_OK);

    d_counter_event_init(&binding,
                         d_counter_ref(&hits),
                         D_COUNTER_EVENT_INCREMENT,
                         2);
    D_TEST_REQUIRE(d_event_dispatcher_bind(&dispatcher,
                                           D_TESTS_COUNTER_KEY_HIT,
                                           d_counter_event_step(&binding),
                                           NULL,
                                           &id) == D_EVENT_OK);
    D_TEST_REQUIRE(id != D_HANDLER_ID_NULL);

    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_COUNTER_KEY_HIT,
                                  NULL);
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_COUNTER_KEY_HIT,
                                  NULL);
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_COUNTER_KEY_MISS,
                                  NULL);
    D_TEST_REQUIRE(hits.value == 4);

    D_TEST_REQUIRE(d_event_dispatcher_unbind(&dispatcher,
                                             id) == D_EVENT_OK);
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_COUNTER_KEY_HIT,
                                  NULL);
    D_TEST_REQUIRE(hits.value == 4);

    d_event_dispatcher_dispose(&dispatcher);

    return true;
}

/*
d_tests_counter_event_operations
  The other three operations.
  Tests the following:
  - DECREMENT moves down by the amount
  - ADD moves by the int64_t the payload holds, either sign, and a NULL
    payload moves nothing
  - RESET returns the counter to its start
*/
bool
d_tests_counter_event_operations(void)
{
    struct dj_event_dispatcher dispatcher = { 0 };
    struct dj_counter          tally      = D_COUNTER_INIT;
    struct dj_counter_event    undo       = { 0 };
    struct dj_counter_event    bytes      = { 0 };
    struct dj_counter_event    clear      = { 0 };
    int64_t                    size       = 40;

    D_TEST_REQUIRE(d_event_dispatcher_init(&dispatcher,
                                           0,
                                           0,
                                           0,
                                           0) ==
                   D_EVENT_OK);

    d_counter_init(&tally,
                   10);
    d_counter_event_init(&undo,
                         d_counter_ref(&tally),
                         D_COUNTER_EVENT_DECREMENT,
                         3);
    d_counter_event_init(&bytes,
                         d_counter_ref(&tally),
                         D_COUNTER_EVENT_ADD,
                         0);
    d_counter_event_init(&clear,
                         d_counter_ref(&tally),
                         D_COUNTER_EVENT_RESET,
                         0);
    D_TEST_REQUIRE(d_event_dispatcher_bind(&dispatcher,
                                           D_TESTS_COUNTER_KEY_UNDO,
                                           d_counter_event_step(&undo),
                                           NULL,
                                           NULL) == D_EVENT_OK);
    D_TEST_REQUIRE(d_event_dispatcher_bind(&dispatcher,
                                           D_TESTS_COUNTER_KEY_BYTES,
                                           d_counter_event_step(&bytes),
                                           NULL,
                                           NULL) == D_EVENT_OK);
    D_TEST_REQUIRE(d_event_dispatcher_bind(&dispatcher,
                                           D_TESTS_COUNTER_KEY_CLEAR,
                                           d_counter_event_step(&clear),
                                           NULL,
                                           NULL) == D_EVENT_OK);

    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_COUNTER_KEY_UNDO,
                                  NULL);
    D_TEST_REQUIRE(tally.value == 7);

    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_COUNTER_KEY_BYTES,
                                  &size);
    D_TEST_REQUIRE(tally.value == 47);
    size = -50;
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_COUNTER_KEY_BYTES,
                                  &size);
    D_TEST_REQUIRE(tally.value == -3);
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_COUNTER_KEY_BYTES,
                                  NULL);
    D_TEST_REQUIRE(tally.value == -3);

    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_COUNTER_KEY_CLEAR,
                                  NULL);
    D_TEST_REQUIRE(tally.value == 10);

    d_event_dispatcher_dispose(&dispatcher);

    return true;
}

/*
d_tests_counter_event_atomic
  An atomic target.
  Tests the following:
  - INCREMENT, ADD and RESET move an atomic counter as they move a plain
    one
*/
bool
d_tests_counter_event_atomic(void)
{
    struct dj_event_dispatcher dispatcher = { 0 };
    struct dj_counter_atomic   shared     = { 0 };
    struct dj_counter_event    hit        = { 0 };
    struct dj_counter_event    bytes      = { 0 };
    struct dj_counter_event    clear      = { 0 };
    int64_t                    size       = 9;

    D_TEST_REQUIRE(d_event_dispatcher_init(&dispatcher,
                                           0,
                                           0,
                                           0,
                                           0) ==
                   D_EVENT_OK);

    d_counter_atomic_init(&shared,
                          1);
    d_counter_event_init(&hit,
                         d_counter_atomic_ref(&shared),
                         D_COUNTER_EVENT_INCREMENT,
                         1);
    d_counter_event_init(&bytes,
                         d_counter_atomic_ref(&shared),
                         D_COUNTER_EVENT_ADD,
                         0);
    d_counter_event_init(&clear,
                         d_counter_atomic_ref(&shared),
                         D_COUNTER_EVENT_RESET,
                         0);
    D_TEST_REQUIRE(d_event_dispatcher_bind(&dispatcher,
                                           D_TESTS_COUNTER_KEY_HIT,
                                           d_counter_event_step(&hit),
                                           NULL,
                                           NULL) == D_EVENT_OK);
    D_TEST_REQUIRE(d_event_dispatcher_bind(&dispatcher,
                                           D_TESTS_COUNTER_KEY_BYTES,
                                           d_counter_event_step(&bytes),
                                           NULL,
                                           NULL) == D_EVENT_OK);
    D_TEST_REQUIRE(d_event_dispatcher_bind(&dispatcher,
                                           D_TESTS_COUNTER_KEY_CLEAR,
                                           d_counter_event_step(&clear),
                                           NULL,
                                           NULL) == D_EVENT_OK);

    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_COUNTER_KEY_HIT,
                                  NULL);
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_COUNTER_KEY_BYTES,
                                  &size);
    D_TEST_REQUIRE(d_counter_atomic_value(&shared) == 11);

    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_COUNTER_KEY_CLEAR,
                                  NULL);
    D_TEST_REQUIRE(d_counter_atomic_value(&shared) == 1);

    d_event_dispatcher_dispose(&dispatcher);

    return true;
}

/*
d_tests_counter_event_fan
  Many to many.
  Tests the following:
  - one key bound to two counters moves both on one firing
  - one counter bound to two keys moves on either
  - a counter at its ceiling still lets the next binding run
*/
bool
d_tests_counter_event_fan(void)
{
    const struct dj_interval_imax ceiling   = d_tests_counter_range(0,
                                                                    1);
    struct dj_event_dispatcher   dispatcher = { 0 };
    struct dj_counter            capped     = D_COUNTER_INIT;
    struct dj_counter            all        = D_COUNTER_INIT;
    struct dj_counter_event      cap_hit    = { 0 };
    struct dj_counter_event      all_hit    = { 0 };
    struct dj_counter_event      all_miss   = { 0 };

    D_TEST_REQUIRE(d_event_dispatcher_init(&dispatcher,
                                           0,
                                           0,
                                           0,
                                           0) ==
                   D_EVENT_OK);
    D_TEST_REQUIRE(d_counter_init_bounded(&capped,
                                          0,
                                          &ceiling));

    d_counter_event_init(&cap_hit,
                         d_counter_ref(&capped),
                         D_COUNTER_EVENT_INCREMENT,
                         1);
    d_counter_event_init(&all_hit,
                         d_counter_ref(&all),
                         D_COUNTER_EVENT_INCREMENT,
                         1);
    d_counter_event_init(&all_miss,
                         d_counter_ref(&all),
                         D_COUNTER_EVENT_INCREMENT,
                         1);
    D_TEST_REQUIRE(d_event_dispatcher_bind(&dispatcher,
                                           D_TESTS_COUNTER_KEY_HIT,
                                           d_counter_event_step(&cap_hit),
                                           NULL,
                                           NULL) == D_EVENT_OK);
    D_TEST_REQUIRE(d_event_dispatcher_bind(&dispatcher,
                                           D_TESTS_COUNTER_KEY_HIT,
                                           d_counter_event_step(&all_hit),
                                           NULL,
                                           NULL) == D_EVENT_OK);
    D_TEST_REQUIRE(d_event_dispatcher_bind(&dispatcher,
                                           D_TESTS_COUNTER_KEY_MISS,
                                           d_counter_event_step(&all_miss),
                                           NULL,
                                           NULL) == D_EVENT_OK);

    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_COUNTER_KEY_HIT,
                                  NULL);
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_COUNTER_KEY_HIT,
                                  NULL);
    (void)d_event_dispatcher_fire(&dispatcher,
                                  D_TESTS_COUNTER_KEY_MISS,
                                  NULL);
    D_TEST_REQUIRE(capped.value == 1);
    D_TEST_REQUIRE(all.value == 3);

    d_event_dispatcher_dispose(&dispatcher);

    return true;
}

/*
d_tests_counter_event_null
  The edges.
  Tests the following:
  - the step on a NULL binding, or one with no target, passes and does
    nothing
  - the step is d_counter_event_apply over the binding, which is all the
    tie-in hands off: binding it is the event core's
  - init on NULL does nothing
*/
bool
d_tests_counter_event_null(void)
{
    struct dj_counter_event binding = { 0 };
    struct dj_event_step    step    = { 0 };

    d_counter_event_init(&binding,
                         d_counter_ref(NULL),
                         D_COUNTER_EVENT_INCREMENT,
                         1);
    D_TEST_REQUIRE(!d_counter_ref_is_valid(binding.target));

    D_TEST_REQUIRE(d_counter_event_apply(NULL,
                                         NULL) == D_VERDICT_PASS);
    D_TEST_REQUIRE(d_counter_event_apply(NULL,
                                         &binding) == D_VERDICT_PASS);

    step = d_counter_event_step(&binding);
    D_TEST_REQUIRE(step.fn == d_counter_event_apply);
    D_TEST_REQUIRE(step.state == &binding);

    d_counter_event_init(NULL,
                         d_counter_ref(NULL),
                         D_COUNTER_EVENT_RESET,
                         0);

    return true;
}
