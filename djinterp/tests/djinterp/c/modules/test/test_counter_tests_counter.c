/*******************************************************************************
* djinterp [c][test]                                test_counter_tests_counter.c
*
* The test_counter suite, block "counter": the test counter, a counter.h
* counter and the step one occurrence moves it by.
*
*
* path:      /tests/djinterp/c/modules/test/test_counter_tests_counter.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#include "./test_counter_tests.h"  // corresponding header


/*
d_tests_test_counter_options_init
  The options' defaults.
  Tests the following:
  - counter options take the step and the floor from the knobs
  - tally options take the capacity as the limit, the knob's policy, and
    the counter options
  - NULL is ignored
*/
bool
d_tests_test_counter_options_init(void)
{
    struct dj_test_counter_options counter = { 0, false };
    struct dj_test_tally_options   tally   = { 0 };

    d_test_counter_options_init(&counter);
    D_TEST_REQUIRE(counter.step == (int64_t)D_INTERNAL_TEST_COUNTER_STEP);
    D_TEST_REQUIRE(counter.has_floor ==
                   (D_INTERNAL_TEST_COUNTER_FLOOR != 0));
    D_TEST_REQUIRE(counter.step != 0);

    d_test_tally_options_init(&tally);
    D_TEST_REQUIRE(tally.limit == (size_t)D_TEST_TALLY_CAPACITY);
    D_TEST_REQUIRE((int)tally.when_full ==
                   (int)D_INTERNAL_TEST_TALLY_WHEN_FULL);
    D_TEST_REQUIRE(tally.counter.step == counter.step);
    D_TEST_REQUIRE(tally.counter.has_floor == counter.has_floor);

    d_test_counter_options_init(NULL);
    d_test_tally_options_init(NULL);

    return true;
}

/*
d_tests_test_counter_init
  Setting up a test counter.
  Tests the following:
  - init sets it at 0 with the knobs' step and floor, as
    D_TEST_COUNTER_INIT does
  - init_with takes a step and a floor; the floor is [0, max]
  - init_with NULL options is init
  - init_with refuses a step of 0 and leaves the counter as it was
*/
bool
d_tests_test_counter_init(void)
{
    struct dj_test_counter_options knobs   = { 0, false };
    struct dj_test_counter_options custom  = { 3, true };
    struct dj_test_counter_options zero    = { 0, false };
    struct dj_test_counter         from    = D_TEST_COUNTER_INIT;
    struct dj_test_counter         counter = D_TEST_COUNTER_INIT;

    d_test_counter_options_init(&knobs);

    d_test_counter_init(&counter);
    D_TEST_REQUIRE(d_test_counter_value(&counter) == 0);
    D_TEST_REQUIRE(d_test_counter_step(&counter) == knobs.step);
    D_TEST_REQUIRE(counter.counter.has_bounds == knobs.has_floor);
    D_TEST_REQUIRE(from.step == counter.step);
    D_TEST_REQUIRE(from.counter.value == counter.counter.value);
    D_TEST_REQUIRE(from.counter.has_bounds == counter.counter.has_bounds);

    D_TEST_REQUIRE(d_test_counter_init_with(&counter,
                                            &custom));
    D_TEST_REQUIRE(d_test_counter_step(&counter) == 3);
    D_TEST_REQUIRE(counter.counter.has_bounds);
    D_TEST_REQUIRE(counter.counter.bounds.lower == 0);
    D_TEST_REQUIRE(counter.counter.bounds.upper == D_MATH_IMAX_MAX);

    D_TEST_REQUIRE(!d_test_counter_init_with(&counter,
                                             &zero));
    D_TEST_REQUIRE(d_test_counter_step(&counter) == 3);

    D_TEST_REQUIRE(d_test_counter_init_with(&counter,
                                            NULL));
    D_TEST_REQUIRE(d_test_counter_step(&counter) == knobs.step);
    D_TEST_REQUIRE(counter.counter.has_bounds == knobs.has_floor);

    return true;
}

/*
d_tests_test_counter_record
  One occurrence at a time.
  Tests the following:
  - record moves the counter by its step
  - a negative step counts down; with a floor it stops at zero and reports
    the cut, without one it goes below
*/
bool
d_tests_test_counter_record(void)
{
    struct dj_test_counter_options by_three  = { 3, false };
    struct dj_test_counter_options down      = { -2, true };
    struct dj_test_counter_options unfloored = { -2, false };
    struct dj_test_counter         counter   = D_TEST_COUNTER_INIT;

    D_TEST_REQUIRE(d_test_counter_init_with(&counter,
                                            &by_three));

    for (int i = 0; i < 4; ++i)
    {
        D_TEST_REQUIRE(d_test_counter_record(&counter));
    }

    D_TEST_REQUIRE(d_test_counter_value(&counter) == 12);

    D_TEST_REQUIRE(d_test_counter_init_with(&counter,
                                            &down));
    D_TEST_REQUIRE(!d_test_counter_record(&counter));
    D_TEST_REQUIRE(d_test_counter_value(&counter) == 0);

    D_TEST_REQUIRE(d_test_counter_init_with(&counter,
                                            &unfloored));
    D_TEST_REQUIRE(d_test_counter_record(&counter));
    D_TEST_REQUIRE(d_test_counter_record(&counter));
    D_TEST_REQUIRE(d_test_counter_value(&counter) == -4);

    return true;
}

/*
d_tests_test_counter_operations
  The five operations and the queries.
  Tests the following:
  - increment and decrement move by any amount, whatever the step
  - compare orders by value
  - merge adds the source's movement, whatever the two count
  - reset returns to zero and keeps the step
  - as_counter is the counter the struct begins with, for counter.h
*/
bool
d_tests_test_counter_operations(void)
{
    struct dj_test_counter_options options = { 5, false };
    struct dj_test_counter         a       = D_TEST_COUNTER_INIT;
    struct dj_test_counter         b       = D_TEST_COUNTER_INIT;

    D_TEST_REQUIRE(d_test_counter_init_with(&a,
                                            &options));
    D_TEST_REQUIRE(d_test_counter_init_with(&b,
                                            &options));

    D_TEST_REQUIRE(d_test_counter_increment(&a,
                                            7));
    D_TEST_REQUIRE(d_test_counter_decrement(&a,
                                            2));
    D_TEST_REQUIRE(d_test_counter_value(&a) == 5);
    D_TEST_REQUIRE(d_test_counter_record(&b));
    D_TEST_REQUIRE(d_test_counter_record(&b));

    D_TEST_REQUIRE(d_test_counter_compare(&a,
                                          &b) == -1);
    D_TEST_REQUIRE(d_test_counter_compare(&b,
                                          &a) == 1);
    D_TEST_REQUIRE(d_test_counter_compare(&a,
                                          &a) == 0);

    D_TEST_REQUIRE(d_test_counter_merge(&a,
                                        &b));
    D_TEST_REQUIRE(d_test_counter_value(&a) == 15);
    D_TEST_REQUIRE(d_test_counter_value(&b) == 10);

    d_test_counter_reset(&a);
    D_TEST_REQUIRE(d_test_counter_value(&a) == 0);
    D_TEST_REQUIRE(d_test_counter_step(&a) == 5);

    D_TEST_REQUIRE(d_test_counter_as_counter(&a) == &a.counter);
    D_TEST_REQUIRE(d_counter_increment(d_test_counter_as_counter(&a),
                                       4));
    D_TEST_REQUIRE(d_test_counter_value(&a) == 4);

    return true;
}

/*
d_tests_test_counter_null
  Every function, given NULL.
  Tests the following:
  - the operations return false, and init and reset do nothing
  - the queries answer 0, and as_counter NULL
*/
bool
d_tests_test_counter_null(void)
{
    struct dj_test_counter counter = D_TEST_COUNTER_INIT;

    d_test_counter_init(NULL);
    d_test_counter_reset(NULL);
    D_TEST_REQUIRE(!d_test_counter_init_with(NULL,
                                             NULL));
    D_TEST_REQUIRE(!d_test_counter_record(NULL));
    D_TEST_REQUIRE(!d_test_counter_increment(NULL,
                                             1));
    D_TEST_REQUIRE(!d_test_counter_decrement(NULL,
                                             1));
    D_TEST_REQUIRE(!d_test_counter_merge(NULL,
                                         &counter));
    D_TEST_REQUIRE(!d_test_counter_merge(&counter,
                                         NULL));

    D_TEST_REQUIRE(d_test_counter_value(NULL) == 0);
    D_TEST_REQUIRE(d_test_counter_step(NULL) == 0);
    D_TEST_REQUIRE(d_test_counter_compare(NULL,
                                          NULL) == 0);
    D_TEST_REQUIRE(d_test_counter_compare(NULL,
                                          &counter) == -1);
    D_TEST_REQUIRE(d_test_counter_as_counter(NULL) == NULL);

    return true;
}
