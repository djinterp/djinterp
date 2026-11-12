/*******************************************************************************
* djinterp [c][test]                                        test_counter_tests.h
*
* DTest's suite for its C counters, test_counter.h, and their event tie-in,
* test_counter_event.h, in the C spec form: the test counter, the tally, and
* the bindings, one block each.
*   Every expectation that depends on a knob (the step, the floor, the
* tally's capacity) is read from the knob, so the suite holds under any
* setting of cfg_test_counter.h.
*
*
* path:      /tests/djinterp/c/modules/test/test_counter_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FIXTURE
    -------
    1.  Keys and options
         1.  D_TESTS_TEST_COUNTER_KEY_*
         2.  d_tests_test_counter_options
2.  TESTS
    -----
    1.  The test counter
    2.  The tally
    3.  Events
3.  SPEC
    ----
    1.  The provider
*/

#ifndef DJINTERP_TESTS_C_TEST_COUNTER_TESTS_H
#define DJINTERP_TESTS_C_TEST_COUNTER_TESTS_H 1

// djinterp
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*
// tested
#include "../../../../../inc/djinterp/c/modules/test/test_counter.h"  // tally
#include "../../../../../inc/djinterp/c/modules/test/test_counter_event.h"
#include "../../../../../inc/djinterp/c/event/event_dispatcher_common.h"  // fire


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  FIXTURE
//==============================================================================


// 1.1    Keys and options
//------------------------------------------------------------------------------
// 1.1.1
// D_TESTS_TEST_COUNTER_KEY_*
//   constant: the tally keys the tests count under. Any values would do; small
// numbers read better in a failure.
#define D_TESTS_TEST_COUNTER_KEY_PASSED  ((dj_test_key)11)
#define D_TESTS_TEST_COUNTER_KEY_FAILED  ((dj_test_key)12)
#define D_TESTS_TEST_COUNTER_KEY_SKIPPED ((dj_test_key)13)
#define D_TESTS_TEST_COUNTER_KEY_ERROR   ((dj_test_key)14)

// 1.1.2
// d_tests_test_counter_options
//   function: tally options with the given limit and policy, and counters
// that step by _step with no floor.
D_STATIC_INLINE
struct dj_test_tally_options
d_tests_test_counter_options(
    size_t                       _limit,
    enum dj_test_tally_when_full _when_full,
    int64_t                      _step
)
{
    struct dj_test_tally_options options = { 0 };

    d_test_tally_options_init(&options);
    options.limit             = _limit;
    options.when_full         = _when_full;
    options.counter.step      = _step;
    options.counter.has_floor = false;

    return options;
}


//==============================================================================
// 2.  TESTS
//==============================================================================


// 2.1    The test counter
//------------------------------------------------------------------------------
bool
d_tests_test_counter_options_init(void);

bool
d_tests_test_counter_init(void);

bool
d_tests_test_counter_record(void);

bool
d_tests_test_counter_operations(void);

bool
d_tests_test_counter_null(void);


// 2.2    The tally
//------------------------------------------------------------------------------
bool
d_tests_test_counter_tally_zero(void);

bool
d_tests_test_counter_tally_init(void);

bool
d_tests_test_counter_tally_track(void);

bool
d_tests_test_counter_tally_full(void);

bool
d_tests_test_counter_tally_count(void);

bool
d_tests_test_counter_tally_merge(void);

bool
d_tests_test_counter_tally_null(void);


// 2.3    Events
//------------------------------------------------------------------------------
bool
d_tests_test_counter_event_tally(void);

bool
d_tests_test_counter_event_counter(void);

bool
d_tests_test_counter_event_full(void);

bool
d_tests_test_counter_event_null(void);


//==============================================================================
// 3.  SPEC
//==============================================================================


// 3.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE

// d_tests_test_counter_counter_rows
//   constant: the test counter block's rows.
static const struct dj_test_case_spec d_tests_test_counter_counter_rows[] =
{
    { "d_tests_test_counter_options_init",
      "the options start at the knobs: the step, the floor, the capacity",
      d_tests_test_counter_options_init },
    { "d_tests_test_counter_init",
      "init and D_TEST_COUNTER_INIT agree; init_with refuses a step of 0",
      d_tests_test_counter_init },
    { "d_tests_test_counter_record",
      "an occurrence moves by the step, either sign, held at the floor",
      d_tests_test_counter_record },
    { "d_tests_test_counter_operations",
      "the five operations are counter.h's, on the counter it begins with",
      d_tests_test_counter_operations },
    { "d_tests_test_counter_null",
      "every function takes NULL: queries answer 0, operations fail",
      d_tests_test_counter_null }
};

// d_tests_test_counter_tally_rows
//   constant: the tally block's rows.
static const struct dj_test_case_spec d_tests_test_counter_tally_rows[] =
{
    { "d_tests_test_counter_tally_zero",
      "a zero-filled tally is usable, on the knobs' options",
      d_tests_test_counter_tally_zero },
    { "d_tests_test_counter_tally_init",
      "init takes options and refuses a bad limit, policy or step",
      d_tests_test_counter_tally_init },
    { "d_tests_test_counter_tally_track",
      "keys are tracked once, in order, at addresses that do not move",
      d_tests_test_counter_tally_track },
    { "d_tests_test_counter_tally_full",
      "past the limit a key is refused, or counted in the overflow",
      d_tests_test_counter_tally_full },
    { "d_tests_test_counter_tally_count",
      "record and increment count by key; reset zeroes, clear forgets",
      d_tests_test_counter_tally_count },
    { "d_tests_test_counter_tally_merge",
      "merge adds key by key, tracks new keys, and carries the overflow",
      d_tests_test_counter_tally_merge },
    { "d_tests_test_counter_tally_null",
      "a NULL tally tracks nothing and counts zero",
      d_tests_test_counter_tally_null }
};

// d_tests_test_counter_event_rows
//   constant: the events block's rows.
static const struct dj_test_case_spec d_tests_test_counter_event_rows[] =
{
    { "d_tests_test_counter_event_tally",
      "a tally binding records its key once per firing of its event",
      d_tests_test_counter_event_tally },
    { "d_tests_test_counter_event_counter",
      "a test counter's binding moves it by its step per firing",
      d_tests_test_counter_event_counter },
    { "d_tests_test_counter_event_full",
      "a full tally loses the occurrence, or counts it in the overflow",
      d_tests_test_counter_event_full },
    { "d_tests_test_counter_event_null",
      "no tally, no counter or no binding: firing passes and moves nothing",
      d_tests_test_counter_event_null }
};

// d_tests_test_counter_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_test_counter_blocks[] =
{
    { "counter",
      "the test counter: a counter and its step",
      d_tests_test_counter_counter_rows,
      sizeof(d_tests_test_counter_counter_rows) /
          sizeof(d_tests_test_counter_counter_rows[0]) },
    { "tally",
      "test counters kept by key",
      d_tests_test_counter_tally_rows,
      sizeof(d_tests_test_counter_tally_rows) /
          sizeof(d_tests_test_counter_tally_rows[0]) },
    { "event",
      "test_counter_event.h's bindings, on the event core",
      d_tests_test_counter_event_rows,
      sizeof(d_tests_test_counter_event_rows) /
          sizeof(d_tests_test_counter_event_rows[0]) }
};

// d_tests_test_counter_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_test_counter_module =
{
    "test_counter",
    "c/modules/test: DTest's test counter, its tally, and their events",
    d_tests_test_counter_blocks,
    sizeof(d_tests_test_counter_blocks) /
        sizeof(d_tests_test_counter_blocks[0])
};

// d_tests_test_counter_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_test_counter_spec(void)
{
    return &d_tests_test_counter_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_TEST_COUNTER_TESTS_H
