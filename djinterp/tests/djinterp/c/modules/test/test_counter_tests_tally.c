/*******************************************************************************
* djinterp [c][test]                                  test_counter_tests_tally.c
*
* The test_counter suite, block "tally": test counters kept by key, with a
* limit and a policy for the keys past it.
*
*
* path:      /tests/djinterp/c/modules/test/test_counter_tests_tally.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#include "./test_counter_tests.h"  // corresponding header

// std
#include <string.h>  // memset


/*
d_tests_test_counter_tally_zero
  A tally needs no init.
  Tests the following:
  - a zero-filled tally tracks nothing and has no overflow
  - it runs on the knobs' options, and its counters step by the knob
  - it tracks and counts at once
*/
bool
d_tests_test_counter_tally_zero(void)
{
    struct dj_test_tally         tally = { 0 };
    struct dj_test_tally_options knobs = { 0 };
    struct dj_test_tally_options read  = { 0 };

    memset(&tally,
           0,
           sizeof(tally));
    d_test_tally_options_init(&knobs);
    d_test_tally_options_of(&tally,
                            &read);

    D_TEST_REQUIRE(tally.count == 0);
    D_TEST_REQUIRE(d_test_tally_overflow(&tally) == 0);
    D_TEST_REQUIRE(read.limit == knobs.limit);
    D_TEST_REQUIRE(read.when_full == knobs.when_full);
    D_TEST_REQUIRE(read.counter.step == knobs.counter.step);

    D_TEST_REQUIRE(d_test_tally_record(&tally,
                                       D_TESTS_TEST_COUNTER_KEY_PASSED));
    D_TEST_REQUIRE(d_test_tally_record(&tally,
                                       D_TESTS_TEST_COUNTER_KEY_PASSED));
    D_TEST_REQUIRE(tally.count == 1);
    D_TEST_REQUIRE(d_test_tally_count(&tally,
                                      D_TESTS_TEST_COUNTER_KEY_PASSED) ==
                   2 * knobs.counter.step);

    return true;
}

/*
d_tests_test_counter_tally_init
  Setting a tally's options.
  Tests the following:
  - init with NULL options takes the knobs', and empties the tally
  - init takes a limit from 1 to the capacity, either policy, and a nonzero
    step
  - it refuses a limit of 0 or above the capacity, an unknown policy and a
    step of 0, and then nothing changes
*/
bool
d_tests_test_counter_tally_init(void)
{
    struct dj_test_tally         tally   = { 0 };
    struct dj_test_tally_options options = { 0 };
    struct dj_test_tally_options read    = { 0 };

    memset(&tally,
           0,
           sizeof(tally));
    D_TEST_REQUIRE(d_test_tally_increment(&tally,
                                          D_TESTS_TEST_COUNTER_KEY_FAILED,
                                          1));
    D_TEST_REQUIRE(d_test_tally_init(&tally,
                                     NULL));
    D_TEST_REQUIRE(tally.count == 0);
    D_TEST_REQUIRE(tally.has_options);

    options = d_tests_test_counter_options(1,
                                           D_TEST_TALLY_FULL_OVERFLOW,
                                           -1);
    D_TEST_REQUIRE(d_test_tally_init(&tally,
                                     &options));
    options = d_tests_test_counter_options((size_t)D_TEST_TALLY_CAPACITY,
                                           D_TEST_TALLY_FULL_REFUSE,
                                           2);
    D_TEST_REQUIRE(d_test_tally_init(&tally,
                                     &options));
    D_TEST_REQUIRE(d_test_tally_record(&tally,
                                       D_TESTS_TEST_COUNTER_KEY_PASSED));

    options.limit = 0;
    D_TEST_REQUIRE(!d_test_tally_init(&tally,
                                      &options));
    options.limit = (size_t)D_TEST_TALLY_CAPACITY + 1;
    D_TEST_REQUIRE(!d_test_tally_init(&tally,
                                      &options));
    options = d_tests_test_counter_options(4,
                                           (enum dj_test_tally_when_full)2,
                                           1);
    D_TEST_REQUIRE(!d_test_tally_init(&tally,
                                      &options));
    options = d_tests_test_counter_options(4,
                                           D_TEST_TALLY_FULL_REFUSE,
                                           0);
    D_TEST_REQUIRE(!d_test_tally_init(&tally,
                                      &options));

    // the refusals left the tally as the last good init made it
    d_test_tally_options_of(&tally,
                            &read);
    D_TEST_REQUIRE(read.limit == (size_t)D_TEST_TALLY_CAPACITY);
    D_TEST_REQUIRE(read.counter.step == 2);
    D_TEST_REQUIRE(d_test_tally_count(&tally,
                                      D_TESTS_TEST_COUNTER_KEY_PASSED) == 2);

    return true;
}

/*
d_tests_test_counter_tally_track
  Tracking keys.
  Tests the following:
  - a new key gets a counter at 0 with the tally's counter options
  - tracking a key again, or finding it, gives the same counter
  - keys are kept in the order they were first tracked
  - a key not tracked is not found, and finding does not track it
  - at the limit a new key gets no counter
*/
bool
d_tests_test_counter_tally_track(void)
{
    struct dj_test_tally         tally   = { 0 };
    struct dj_test_tally_options options = { 0 };
    struct dj_test_counter*      passed  = NULL;
    struct dj_test_counter*      failed  = NULL;

    options = d_tests_test_counter_options(2,
                                           D_TEST_TALLY_FULL_REFUSE,
                                           7);
    D_TEST_REQUIRE(d_test_tally_init(&tally,
                                     &options));

    passed = d_test_tally_track(&tally,
                                D_TESTS_TEST_COUNTER_KEY_PASSED);
    D_TEST_REQUIRE(passed != NULL);
    D_TEST_REQUIRE(d_test_counter_value(passed) == 0);
    D_TEST_REQUIRE(d_test_counter_step(passed) == 7);

    D_TEST_REQUIRE(d_test_tally_find(&tally,
                                     D_TESTS_TEST_COUNTER_KEY_FAILED) ==
                   NULL);
    D_TEST_REQUIRE(tally.count == 1);

    failed = d_test_tally_track(&tally,
                                D_TESTS_TEST_COUNTER_KEY_FAILED);
    D_TEST_REQUIRE(failed != NULL);
    D_TEST_REQUIRE(failed != passed);
    D_TEST_REQUIRE(d_test_tally_track(&tally,
                                      D_TESTS_TEST_COUNTER_KEY_PASSED) ==
                   passed);
    D_TEST_REQUIRE(d_test_tally_find(&tally,
                                     D_TESTS_TEST_COUNTER_KEY_FAILED) ==
                   failed);
    D_TEST_REQUIRE(tally.count == 2);
    D_TEST_REQUIRE(tally.entries[0].key == D_TESTS_TEST_COUNTER_KEY_PASSED);
    D_TEST_REQUIRE(tally.entries[1].key == D_TESTS_TEST_COUNTER_KEY_FAILED);

    D_TEST_REQUIRE(d_test_tally_track(&tally,
                                      D_TESTS_TEST_COUNTER_KEY_SKIPPED) ==
                   NULL);
    D_TEST_REQUIRE(tally.count == 2);
    D_TEST_REQUIRE(d_test_tally_find(&tally,
                                     D_TESTS_TEST_COUNTER_KEY_PASSED) ==
                   passed);

    return true;
}

/*
d_tests_test_counter_tally_full
  Keys past the limit.
  Tests the following:
  - a refusing tally fails to count a key past its limit, and keeps nothing
    of it
  - an overflowing tally counts it in the overflow instead, by the amount,
    or by the counter step when recorded, and reports the step
  - the tracked keys count as before either way
*/
bool
d_tests_test_counter_tally_full(void)
{
    struct dj_test_tally         tally   = { 0 };
    struct dj_test_tally_options options = { 0 };

    options = d_tests_test_counter_options(1,
                                           D_TEST_TALLY_FULL_REFUSE,
                                           1);
    D_TEST_REQUIRE(d_test_tally_init(&tally,
                                     &options));
    D_TEST_REQUIRE(d_test_tally_record(&tally,
                                       D_TESTS_TEST_COUNTER_KEY_PASSED));
    D_TEST_REQUIRE(!d_test_tally_record(&tally,
                                        D_TESTS_TEST_COUNTER_KEY_FAILED));
    D_TEST_REQUIRE(!d_test_tally_increment(&tally,
                                           D_TESTS_TEST_COUNTER_KEY_FAILED,
                                           5));
    D_TEST_REQUIRE(d_test_tally_count(&tally,
                                      D_TESTS_TEST_COUNTER_KEY_FAILED) == 0);
    D_TEST_REQUIRE(d_test_tally_overflow(&tally) == 0);
    D_TEST_REQUIRE(d_test_tally_count(&tally,
                                      D_TESTS_TEST_COUNTER_KEY_PASSED) == 1);

    options = d_tests_test_counter_options(1,
                                           D_TEST_TALLY_FULL_OVERFLOW,
                                           3);
    D_TEST_REQUIRE(d_test_tally_init(&tally,
                                     &options));
    D_TEST_REQUIRE(d_test_tally_record(&tally,
                                       D_TESTS_TEST_COUNTER_KEY_PASSED));
    D_TEST_REQUIRE(d_test_tally_increment(&tally,
                                          D_TESTS_TEST_COUNTER_KEY_FAILED,
                                          5));
    D_TEST_REQUIRE(d_test_tally_record(&tally,
                                       D_TESTS_TEST_COUNTER_KEY_SKIPPED));
    D_TEST_REQUIRE(d_test_tally_overflow(&tally) == 8);
    D_TEST_REQUIRE(d_test_tally_count(&tally,
                                      D_TESTS_TEST_COUNTER_KEY_FAILED) == 0);
    D_TEST_REQUIRE(d_test_tally_count(&tally,
                                      D_TESTS_TEST_COUNTER_KEY_PASSED) == 3);
    D_TEST_REQUIRE(tally.count == 1);

    return true;
}

/*
d_tests_test_counter_tally_count
  Counting, resetting and clearing.
  Tests the following:
  - record counts one occurrence of a key; increment counts any amount
  - a key not tracked counts 0
  - reset returns every counter and the overflow to zero, and keeps the keys
  - clear forgets every key and zeroes the overflow, and keeps the options
*/
bool
d_tests_test_counter_tally_count(void)
{
    struct dj_test_tally         tally   = { 0 };
    struct dj_test_tally_options options = { 0 };

    options = d_tests_test_counter_options(2,
                                           D_TEST_TALLY_FULL_OVERFLOW,
                                           1);
    D_TEST_REQUIRE(d_test_tally_init(&tally,
                                     &options));

    D_TEST_REQUIRE(d_test_tally_record(&tally,
                                       D_TESTS_TEST_COUNTER_KEY_PASSED));
    D_TEST_REQUIRE(d_test_tally_increment(&tally,
                                          D_TESTS_TEST_COUNTER_KEY_PASSED,
                                          4));
    D_TEST_REQUIRE(d_test_tally_increment(&tally,
                                          D_TESTS_TEST_COUNTER_KEY_FAILED,
                                          -2));
    D_TEST_REQUIRE(d_test_tally_record(&tally,
                                       D_TESTS_TEST_COUNTER_KEY_ERROR));
    D_TEST_REQUIRE(d_test_tally_count(&tally,
                                      D_TESTS_TEST_COUNTER_KEY_PASSED) == 5);
    D_TEST_REQUIRE(d_test_tally_count(&tally,
                                      D_TESTS_TEST_COUNTER_KEY_FAILED) == -2);
    D_TEST_REQUIRE(d_test_tally_count(&tally,
                                      D_TESTS_TEST_COUNTER_KEY_SKIPPED) == 0);
    D_TEST_REQUIRE(d_test_tally_overflow(&tally) == 1);

    d_test_tally_reset(&tally);
    D_TEST_REQUIRE(tally.count == 2);
    D_TEST_REQUIRE(d_test_tally_count(&tally,
                                      D_TESTS_TEST_COUNTER_KEY_PASSED) == 0);
    D_TEST_REQUIRE(d_test_tally_count(&tally,
                                      D_TESTS_TEST_COUNTER_KEY_FAILED) == 0);
    D_TEST_REQUIRE(d_test_tally_overflow(&tally) == 0);

    D_TEST_REQUIRE(d_test_tally_record(&tally,
                                       D_TESTS_TEST_COUNTER_KEY_ERROR));
    d_test_tally_clear(&tally);
    D_TEST_REQUIRE(tally.count == 0);
    D_TEST_REQUIRE(d_test_tally_find(&tally,
                                     D_TESTS_TEST_COUNTER_KEY_PASSED) ==
                   NULL);
    D_TEST_REQUIRE(d_test_tally_overflow(&tally) == 0);
    D_TEST_REQUIRE(tally.has_options);
    D_TEST_REQUIRE(tally.options.limit == 2);

    return true;
}

/*
d_tests_test_counter_tally_merge
  Merging tallies.
  Tests the following:
  - each key's movement is added to the same key; a key new to the
    destination is tracked first
  - the source's overflow merges into the destination's
  - a tally merged into itself doubles each count once
  - past the destination's limit a refusing tally fails, after merging all
    it could; an overflowing one counts the key in its overflow
*/
bool
d_tests_test_counter_tally_merge(void)
{
    struct dj_test_tally         into    = { 0 };
    struct dj_test_tally         from    = { 0 };
    struct dj_test_tally         small   = { 0 };
    struct dj_test_tally_options options = { 0 };

    options = d_tests_test_counter_options(4,
                                           D_TEST_TALLY_FULL_OVERFLOW,
                                           1);
    D_TEST_REQUIRE(d_test_tally_init(&into,
                                     &options));
    D_TEST_REQUIRE(d_test_tally_init(&from,
                                     &options));
    D_TEST_REQUIRE(d_test_tally_increment(&into,
                                          D_TESTS_TEST_COUNTER_KEY_PASSED,
                                          2));
    D_TEST_REQUIRE(d_test_tally_increment(&from,
                                          D_TESTS_TEST_COUNTER_KEY_PASSED,
                                          3));
    D_TEST_REQUIRE(d_test_tally_increment(&from,
                                          D_TESTS_TEST_COUNTER_KEY_FAILED,
                                          4));
    D_TEST_REQUIRE(d_counter_increment(&from.overflow,
                                       6));

    D_TEST_REQUIRE(d_test_tally_merge(&into,
                                      &from));
    D_TEST_REQUIRE(d_test_tally_count(&into,
                                      D_TESTS_TEST_COUNTER_KEY_PASSED) == 5);
    D_TEST_REQUIRE(d_test_tally_count(&into,
                                      D_TESTS_TEST_COUNTER_KEY_FAILED) == 4);
    D_TEST_REQUIRE(d_test_tally_overflow(&into) == 6);
    D_TEST_REQUIRE(d_test_tally_count(&from,
                                      D_TESTS_TEST_COUNTER_KEY_PASSED) == 3);

    D_TEST_REQUIRE(d_test_tally_merge(&into,
                                      &into));
    D_TEST_REQUIRE(d_test_tally_count(&into,
                                      D_TESTS_TEST_COUNTER_KEY_PASSED) == 10);
    D_TEST_REQUIRE(d_test_tally_count(&into,
                                      D_TESTS_TEST_COUNTER_KEY_FAILED) == 8);
    D_TEST_REQUIRE(d_test_tally_overflow(&into) == 12);

    options = d_tests_test_counter_options(1,
                                           D_TEST_TALLY_FULL_REFUSE,
                                           1);
    D_TEST_REQUIRE(d_test_tally_init(&small,
                                     &options));
    D_TEST_REQUIRE(!d_test_tally_merge(&small,
                                       &from));
    D_TEST_REQUIRE(d_test_tally_count(&small,
                                      D_TESTS_TEST_COUNTER_KEY_PASSED) == 3);
    D_TEST_REQUIRE(d_test_tally_count(&small,
                                      D_TESTS_TEST_COUNTER_KEY_FAILED) == 0);

    options.when_full = D_TEST_TALLY_FULL_OVERFLOW;
    D_TEST_REQUIRE(d_test_tally_init(&small,
                                     &options));
    D_TEST_REQUIRE(d_test_tally_merge(&small,
                                      &from));
    D_TEST_REQUIRE(d_test_tally_count(&small,
                                      D_TESTS_TEST_COUNTER_KEY_PASSED) == 3);
    D_TEST_REQUIRE(d_test_tally_overflow(&small) == 10);

    return true;
}

/*
d_tests_test_counter_tally_null
  Every function, given NULL.
  Tests the following:
  - tracking and counting into NULL fail; merge with a NULL side fails
  - init, reset and clear of NULL do nothing; init refuses NULL
  - the queries answer 0 or NULL, and options_of NULL reads the knobs
*/
bool
d_tests_test_counter_tally_null(void)
{
    struct dj_test_tally         tally = { 0 };
    struct dj_test_tally_options knobs = { 0 };
    struct dj_test_tally_options read  = { 0 };

    memset(&tally,
           0,
           sizeof(tally));
    d_test_tally_options_init(&knobs);

    D_TEST_REQUIRE(!d_test_tally_init(NULL,
                                      NULL));
    d_test_tally_reset(NULL);
    d_test_tally_clear(NULL);
    D_TEST_REQUIRE(d_test_tally_track(NULL,
                                      D_TESTS_TEST_COUNTER_KEY_PASSED) ==
                   NULL);
    D_TEST_REQUIRE(d_test_tally_find(NULL,
                                     D_TESTS_TEST_COUNTER_KEY_PASSED) ==
                   NULL);
    D_TEST_REQUIRE(!d_test_tally_record(NULL,
                                        D_TESTS_TEST_COUNTER_KEY_PASSED));
    D_TEST_REQUIRE(!d_test_tally_increment(NULL,
                                           D_TESTS_TEST_COUNTER_KEY_PASSED,
                                           1));
    D_TEST_REQUIRE(!d_test_tally_merge(NULL,
                                       &tally));
    D_TEST_REQUIRE(!d_test_tally_merge(&tally,
                                       NULL));

    D_TEST_REQUIRE(d_test_tally_count(NULL,
                                      D_TESTS_TEST_COUNTER_KEY_PASSED) == 0);
    D_TEST_REQUIRE(d_test_tally_overflow(NULL) == 0);
    d_test_tally_options_of(NULL,
                            &read);
    D_TEST_REQUIRE(read.limit == knobs.limit);
    d_test_tally_options_of(&tally,
                            NULL);

    return true;
}
