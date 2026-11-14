/*******************************************************************************
* djinterp [c][utility]                                          counter_tests.h
*
* The counter family's suite in DTest's C face (c/modules/test/test_suite.h):
* counter.h's bounds arithmetic and counter, counter_atomic.h's lock-free
* form, counter_event.h's bindings, counter_mutex.h's guarded form, and
* counter.h's interface over any counter, one block each.
*
*
* path:      /tests/djinterp/c/util/counter/counter_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.11
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FIXTURE
    -------
    1.  Ranges
         1.  d_tests_counter_range
2.  TESTS
    -----
    1.  Bounds
    2.  The counter
    3.  The atomic counter
    4.  Events
    5.  The guarded counter
    6.  Any counter
3.  SPEC
    ----
    1.  The provider
*/

#ifndef DJINTERP_TESTS_C_UTIL_COUNTER_TESTS_H
#define DJINTERP_TESTS_C_UTIL_COUNTER_TESTS_H 1

// djinterp
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*
// tested
#include "../../../../../inc/djinterp/c/util/counter/counter.h"  // plain
#include "../../../../../inc/djinterp/c/util/counter/counter_atomic.h"  // CAS
#include "../../../../../inc/djinterp/c/util/counter/counter_mutex.h"  // locked
#include "../../../../../inc/djinterp/c/util/counter/counter_event.h"  // events
#include "../../../../../inc/djinterp/c/event/event_dispatcher_common.h"  // fire


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  FIXTURE
//==============================================================================


// 1.1    Ranges
//------------------------------------------------------------------------------
// 1.1.1
// d_tests_counter_range
//   function: the closed, continuous interval [_lower, _upper], the bounds
// most tests use.
D_STATIC_INLINE
struct dj_interval_imax
d_tests_counter_range(
    int64_t _lower,
    int64_t _upper
)
{
    return d_interval_imax_make((dj_math_imax)_lower,
                                (dj_math_imax)_upper,
                                D_INTERVAL_CLOSED,
                                0);
}


//==============================================================================
// 2.  TESTS
//==============================================================================


// 2.1    Bounds
//------------------------------------------------------------------------------
bool
d_tests_counter_bounds_open(void);

bool
d_tests_counter_bounds_add(void);

bool
d_tests_counter_bounds_sub(void);

// 2.2    The counter
//------------------------------------------------------------------------------
bool
d_tests_counter_zero(void);

bool
d_tests_counter_init(void);

bool
d_tests_counter_step(void);

bool
d_tests_counter_compare(void);

bool
d_tests_counter_merge(void);

bool
d_tests_counter_reset(void);

bool
d_tests_counter_null(void);

// 2.3    The atomic counter
//------------------------------------------------------------------------------
bool
d_tests_counter_atomic_init(void);

bool
d_tests_counter_atomic_step(void);

bool
d_tests_counter_atomic_merge(void);

bool
d_tests_counter_atomic_threads(void);

bool
d_tests_counter_atomic_null(void);

// 2.4    Events
//------------------------------------------------------------------------------
bool
d_tests_counter_event_init(void);

bool
d_tests_counter_event_counts(void);

bool
d_tests_counter_event_operations(void);

bool
d_tests_counter_event_atomic(void);

bool
d_tests_counter_event_fan(void);

bool
d_tests_counter_event_null(void);


// 2.5    The guarded counter
//------------------------------------------------------------------------------
bool
d_tests_counter_mutex_lifetime(void);

bool
d_tests_counter_mutex_step(void);

bool
d_tests_counter_mutex_merge(void);

bool
d_tests_counter_mutex_threads(void);

bool
d_tests_counter_mutex_null(void);


// 2.6    Any counter
//------------------------------------------------------------------------------
bool
d_tests_counter_ref_make(void);

bool
d_tests_counter_ref_operations(void);

bool
d_tests_counter_ref_compare(void);

bool
d_tests_counter_ref_merge(void);

bool
d_tests_counter_ref_none(void);


//==============================================================================
// 3.  SPEC
//==============================================================================


// 3.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE

// d_tests_counter_bounds_rows
//   constant: the bounds block's rows.
static const struct dj_test_case_spec d_tests_counter_bounds_rows[] =
{
    { "d_tests_counter_bounds_open",
      "an open end holds one integer inside; one side limits one side",
      d_tests_counter_bounds_open },
    { "d_tests_counter_bounds_add",
      "a sum held in int64_t and then in the bounds, with the cut reported",
      d_tests_counter_bounds_add },
    { "d_tests_counter_bounds_sub",
      "the mirror, INT64_MIN included, which has no negation",
      d_tests_counter_bounds_sub }
};

// d_tests_counter_counter_rows
//   constant: the counter block's rows.
static const struct dj_test_case_spec d_tests_counter_counter_rows[] =
{
    { "d_tests_counter_zero",
      "a zero-filled counter is D_COUNTER_INIT's: 0, unbounded, usable",
      d_tests_counter_zero },
    { "d_tests_counter_init",
      "init and init_bounded set every field; bad bounds or start refused",
      d_tests_counter_init },
    { "d_tests_counter_step",
      "increment and decrement move either way and stop at the bounds",
      d_tests_counter_step },
    { "d_tests_counter_compare",
      "three-way by value, NULL first, with no overflow at the extremes",
      d_tests_counter_compare },
    { "d_tests_counter_merge",
      "a merge adds the source's movement since its start, not its value",
      d_tests_counter_merge },
    { "d_tests_counter_reset",
      "reset returns to the start and keeps the bounds",
      d_tests_counter_reset },
    { "d_tests_counter_null",
      "every function takes NULL: queries answer 0, operations fail",
      d_tests_counter_null }
};

// d_tests_counter_atomic_rows
//   constant: the atomic counter block's rows.
static const struct dj_test_case_spec d_tests_counter_atomic_rows[] =
{
    { "d_tests_counter_atomic_init",
      "init and init_bounded refuse what the plain counter refuses",
      d_tests_counter_atomic_init },
    { "d_tests_counter_atomic_step",
      "steps, compare and reset behave as the plain counter's",
      d_tests_counter_atomic_step },
    { "d_tests_counter_atomic_merge",
      "merge from an atomic or a plain counter adds its movement",
      d_tests_counter_atomic_merge },
    { "d_tests_counter_atomic_threads",
      "eight threads counting at once lose no step and cross no bound",
      d_tests_counter_atomic_threads },
    { "d_tests_counter_atomic_null",
      "every function takes NULL: queries answer 0, operations fail",
      d_tests_counter_atomic_null }
};

// d_tests_counter_event_rows
//   constant: the events block's rows.
static const struct dj_test_case_spec d_tests_counter_event_rows[] =
{
    { "d_tests_counter_event_init",
      "init aims one binding type at a counter of any kind",
      d_tests_counter_event_init },
    { "d_tests_counter_event_counts",
      "a bound counter counts its key's firings and no other key's",
      d_tests_counter_event_counts },
    { "d_tests_counter_event_operations",
      "decrement, add-the-payload and reset, each on its own key",
      d_tests_counter_event_operations },
    { "d_tests_counter_event_atomic",
      "a binding moves an atomic counter the same way",
      d_tests_counter_event_atomic },
    { "d_tests_counter_event_fan",
      "one key moves several counters; several keys move one",
      d_tests_counter_event_fan },
    { "d_tests_counter_event_null",
      "no target, or a NULL binding, passes; the step is the hand-off",
      d_tests_counter_event_null }
};

// d_tests_counter_mutex_rows
//   constant: the guarded counter block's rows.
static const struct dj_test_case_spec d_tests_counter_mutex_rows[] =
{
    { "d_tests_counter_mutex_lifetime",
      "init readies it; a refused init or destroy leaves it not ready",
      d_tests_counter_mutex_lifetime },
    { "d_tests_counter_mutex_step",
      "steps, compare and reset behave as the plain counter's",
      d_tests_counter_mutex_step },
    { "d_tests_counter_mutex_merge",
      "merge from a guarded or plain counter, or itself, adds its movement",
      d_tests_counter_mutex_merge },
    { "d_tests_counter_mutex_threads",
      "eight threads counting at once lose no step and cross no bound",
      d_tests_counter_mutex_threads },
    { "d_tests_counter_mutex_null",
      "every function takes NULL: queries answer 0, operations fail",
      d_tests_counter_mutex_null }
};

// d_tests_counter_ref_rows
//   constant: the any-counter block's rows.
static const struct dj_test_case_spec d_tests_counter_ref_rows[] =
{
    { "d_tests_counter_ref_make",
      "each kind's maker refers through its own full table; NULL is none",
      d_tests_counter_ref_make },
    { "d_tests_counter_ref_operations",
      "steps, reset, value and snapshot act the same on every kind",
      d_tests_counter_ref_operations },
    { "d_tests_counter_ref_compare",
      "three-way across kinds, no counter first",
      d_tests_counter_ref_compare },
    { "d_tests_counter_ref_merge",
      "every kind merges every kind, itself included; a cut reports",
      d_tests_counter_ref_merge },
    { "d_tests_counter_ref_none",
      "no counter, or one not set up: operations fail, queries answer 0",
      d_tests_counter_ref_none }
};

// d_tests_counter_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_counter_blocks[] =
{
    { "bounds",
      "the saturating arithmetic every counter shares",
      d_tests_counter_bounds_rows,
      sizeof(d_tests_counter_bounds_rows) /
          sizeof(d_tests_counter_bounds_rows[0]) },
    { "counter",
      "counter.h's counter and its five operations",
      d_tests_counter_counter_rows,
      sizeof(d_tests_counter_counter_rows) /
          sizeof(d_tests_counter_counter_rows[0]) },
    { "atomic",
      "counter_atomic.h's lock-free counter",
      d_tests_counter_atomic_rows,
      sizeof(d_tests_counter_atomic_rows) /
          sizeof(d_tests_counter_atomic_rows[0]) },
    { "event",
      "counter_event.h's handlers, bound on the event core",
      d_tests_counter_event_rows,
      sizeof(d_tests_counter_event_rows) /
          sizeof(d_tests_counter_event_rows[0]) },
    { "mutex",
      "counter_mutex.h's guarded counter",
      d_tests_counter_mutex_rows,
      sizeof(d_tests_counter_mutex_rows) /
          sizeof(d_tests_counter_mutex_rows[0]) },
    { "ref",
      "counter.h's interface over any counter",
      d_tests_counter_ref_rows,
      sizeof(d_tests_counter_ref_rows) /
          sizeof(d_tests_counter_ref_rows[0]) }
};

// d_tests_counter_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_counter_module =
{
    "counter",
    "c/util/counter: the generic counter, atomic, guarded, by event and by ref",
    d_tests_counter_blocks,
    sizeof(d_tests_counter_blocks) / sizeof(d_tests_counter_blocks[0])
};

// d_tests_counter_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_counter_spec(void)
{
    return &d_tests_counter_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_UTIL_COUNTER_TESTS_H
