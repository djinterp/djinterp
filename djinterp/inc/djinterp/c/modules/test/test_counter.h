/*******************************************************************************
* djinterp [c][test]                                              test_counter.h
*
* DTest's counters for the C face: a test counter is the generic counter
* (c/util/counter/counter.h) plus the step one occurrence moves it by, and a
* tally is a set of test counters kept by key.
*
* NOTHING HERE KNOWS WHAT IS COUNTED
* ----------------------------------
*   The owner's ruling of 2026-11-09 made DTest default-agnostic: a tally counts
* under whichever keys its caller chooses -- assertions, tests, retries, bytes,
* anything -- and holds whichever counters its caller tracks. The fixed fields
* that came before (assertions_passed, assertions_total, tests_passed,
* tests_total; ruling #79's field form) are gone; a caller who wants those four
* tracks four keys of its own.
*
* DERIVED, THE C WAY
* ------------------
*   A struct dj_test_counter begins with a struct dj_counter, so a pointer to
* one is a pointer to the other, and every counter.h function works on it
* through d_test_counter_as_counter. The five operations are also given here
* under DTest's names, and recording one occurrence (d_test_counter_record)
* moves a counter by its step.
*
* COUNTED BY HAND
* ---------------
*   Nothing here knows about events. A suite records and increments as it goes;
* one that wants events to count for it binds the handlers of
* test_counter_event.h, the tie-in, which hands a tally key or a test counter
* off to the event core.
*
* CONFIGURED IN THREE LAYERS
* --------------------------
*   Every setting -- a counter's step and floor, a tally's limit and what it
* does when full -- has a D_CFG_* knob with a sensible default
* (cfg_test_counter.h); a run overrides it per counter or per tally through the
* options structs below; and, opt-in with D_CFG_TEST_COUNTER_MACHINE, a program
* that runs DTest as an abstract machine gets the same settings as an option_env
* schema (section 4).
*
* ZERO IS A TALLY
* ---------------
*   A zero-filled struct dj_test_tally is an empty one that uses the knobs'
* defaults, and its storage is D_CFG_TEST_TALLY_CAPACITY.
*
* HOW TO USE
* ----------
*   Count by hand; a zero-filled tally is ready to use, keyed by any dj_test_key
* the suite chooses:
*
*     struct dj_test_tally tally = { 0 };
*
*     d_test_tally_record(&tally, MY_RETRIES);        // one step
*     d_test_tally_increment(&tally, MY_BYTES, 512);
*     d_test_tally_count(&tally, MY_RETRIES);         // 1
*
*   To have events count instead, bind the handlers of test_counter_event.h.
*
* OPTIONS
* -------
*   struct dj_test_counter_options (a counter's step and floor) and struct
* dj_test_tally_options (a tally's limit, what it does when full, and the
* options of each counter it tracks), passed to d_test_counter_init_with and
* d_test_tally_init. Their _options_init functions start from the knobs. With
* D_CFG_TEST_COUNTER_MACHINE, the same settings are an option_env schema
* (d_test_counter_env_vars), so text, a script, the environment or the command
* line can set them.
*
* CONFIGURATION
* -------------
*   Set before dconfig.h is included, or with -D; each defaults in
* config/c/modules/test/cfg_test_counter.h: D_CFG_TEST_TALLY_CAPACITY (16),
* D_CFG_TEST_TALLY_WHEN_FULL (0), D_CFG_TEST_COUNTER_STEP (1),
* D_CFG_TEST_COUNTER_FLOOR (0), D_CFG_TEST_COUNTER_MACHINE (0).
*
*
* path:      /inc/djinterp/c/modules/test/test_counter.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.30
*                                                            revised: 2026.11.10
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Options
         1.  dj_test_tally_when_full
         2.  dj_test_counter_options
         3.  dj_test_tally_options
    2.  Keys
         1.  dj_test_key
    3.  The test counter
         1.  dj_test_counter
         2.  D_TEST_COUNTER_INIT
    4.  The tally
         1.  D_TEST_TALLY_CAPACITY
         2.  dj_test_tally_entry
         3.  dj_test_tally
2.  THE TEST COUNTER
    ----------------
    1.  Options
    2.  Initialization
    3.  Recording
    4.  The five operations
    5.  Queries
3.  THE TALLY
    ---------
    1.  Options and initialization
    2.  Tracking
    3.  Counting
    4.  Combining
4.  THE ABSTRACT MACHINE (OPT-IN)
    -----------------------------
    1.  The schema
         1.  D_TEST_COUNTER_ENV_VAR_COUNT
    2.  Reading a frame
*/

#ifndef DJINTERP_C_TEST_COUNTER_H
#define DJINTERP_C_TEST_COUNTER_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../djinterp.h"  // framework root
#include "../../../config/c/modules/test/cfg_test_counter.h"  // the knobs
#include "../../util/counter/counter.h"  // dj_counter
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // int64_t, RE_STD_HAS_INT64

// the abstract machine, only when a build opts in (section 4)
#if D_INTERNAL_TEST_COUNTER_MACHINE
    #include "../../option/option_env.h"  // dj_option_env
#endif  // D_INTERNAL_TEST_COUNTER_MACHINE


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Options
//------------------------------------------------------------------------------
// 1.1.1
// dj_test_tally_when_full
//   enum: what a full tally does with a key it cannot track.
enum dj_test_tally_when_full
{
    D_TEST_TALLY_FULL_REFUSE   = 0,  // tracking fails; counting it fails
    D_TEST_TALLY_FULL_OVERFLOW = 1   // counting it moves the overflow counter
};

// 1.1.2
// dj_test_counter_options
//   struct: how a test counter counts: what one occurrence moves it by, and
// whether it is held at or above zero.
struct dj_test_counter_options
{
    int64_t step;        // per occurrence; not 0
    bool    has_floor;   // held at or above zero
};

// 1.1.3
// dj_test_tally_options
//   struct: how a tally keeps its counters, and the options each new one
// starts with.
struct dj_test_tally_options
{
    size_t                         limit;      // keys at most; 1 to
                                               // D_TEST_TALLY_CAPACITY
    enum dj_test_tally_when_full   when_full;  // past the limit
    struct dj_test_counter_options counter;    // for each counter tracked
};

// 1.2    Keys
//------------------------------------------------------------------------------
// 1.2.1
// dj_test_key
//   type: what a tally tells its counters apart by. Any value the caller
// chooses: a constant of its own, a hash of a name, or an event core key when
// the suite counts events (test_counter_event.h).
typedef uint64_t dj_test_key;

// 1.3    The test counter
//------------------------------------------------------------------------------
// 1.3.1
// dj_test_counter
//   struct: a counter and the step one occurrence moves it by. `counter` is
// first, so the struct is a struct dj_counter as well.
struct dj_test_counter
{
    struct dj_counter counter;  // the count
    int64_t           step;     // per occurrence; not 0
};

// 1.3.2
// D_TEST_COUNTER_INIT
//   constant: an initializer for a test counter at 0 with the knobs' step and
// floor: what d_test_counter_init sets.
#if D_INTERNAL_TEST_COUNTER_FLOOR
    #define D_TEST_COUNTER_INIT                                                \
        { { 0, 0, { 0, D_MATH_IMAX_MAX, 0, 0u }, true },                       \
          D_INTERNAL_TEST_COUNTER_STEP }
#else
    #define D_TEST_COUNTER_INIT                                                \
        { D_COUNTER_INIT,                                                      \
          D_INTERNAL_TEST_COUNTER_STEP }
#endif  // D_INTERNAL_TEST_COUNTER_FLOOR

// 1.4    The tally
//------------------------------------------------------------------------------
// 1.4.1
// D_TEST_TALLY_CAPACITY
//   constant: how many keys one tally can hold: its storage.
#define D_TEST_TALLY_CAPACITY  D_INTERNAL_TEST_TALLY_CAPACITY

// 1.4.2
// dj_test_tally_entry
//   struct: one tracked key and its counter.
struct dj_test_tally_entry
{
    dj_test_key            key;      // what it counts
    struct dj_test_counter counter;  // the count
};

// 1.4.3
// dj_test_tally
//   struct: test counters kept by key, in the order they were first tracked.
// The array is part of the struct, so a counter's address never moves while
// the tally does not. Until has_options is set, by d_test_tally_init, the
// tally uses the knobs' defaults.
struct dj_test_tally
{
    size_t                       count;        // in use
    bool                         has_options;  // whether options applies
    struct dj_test_tally_options options;      // when has_options
    struct dj_counter            overflow;     // keys past the limit, under
                                               // FULL_OVERFLOW
    struct dj_test_tally_entry   entries[D_TEST_TALLY_CAPACITY];  // by key
};


//==============================================================================
// 2.  THE TEST COUNTER
//==============================================================================
// counter.h's contracts: NULL is accepted everywhere, a query on NULL
// answers 0, and an operation on NULL fails.


// 2.1    Options
//------------------------------------------------------------------------------
/**
 * @brief Sets counter options to the knobs' defaults.
 *
 * @param[out] _options  the options; `NULL` is ignored.
 * @post `step` is D_CFG_TEST_COUNTER_STEP and `has_floor` is
 *       D_CFG_TEST_COUNTER_FLOOR.
 */
void
d_test_counter_options_init(
    struct dj_test_counter_options* _options
);

// 2.2    Initialization
//------------------------------------------------------------------------------
/**
 * @brief Sets a test counter at 0 with the knobs' step and floor.
 *
 * @param[out] _counter  the counter to set; `NULL` is ignored.
 * @post `_counter` equals D_TEST_COUNTER_INIT.
 */
void
d_test_counter_init(
    struct dj_test_counter* _counter
);

/**
 * @brief Sets a test counter at 0 with the given options.
 *
 * @param[out] _counter  the counter to set.
 * @param[in]  _options  the options; `NULL` means the knobs' defaults.
 * @return `true` when set; `false` when `_counter` is `NULL` or the step is
 *         0, and then nothing changes.
 */
bool
d_test_counter_init_with(
    struct dj_test_counter*               _counter,
    const struct dj_test_counter_options* _options
);

// 2.3    Recording
//------------------------------------------------------------------------------
/**
 * @brief Records one occurrence: moves a test counter by its step.
 *
 * @param[in,out] _counter  the counter.
 * @return as d_counter_increment by the step; `false` for `NULL`.
 */
bool
d_test_counter_record(
    struct dj_test_counter* _counter
);

// 2.4    The five operations
//------------------------------------------------------------------------------
// counter.h's, on the test counter's counter.
/**
 * @brief Moves a test counter up by `_amount`.
 *
 * @param[in,out] _counter  the counter.
 * @param[in]     _amount   how far to move it; negative moves it down.
 * @return as d_counter_increment.
 */
bool
d_test_counter_increment(
    struct dj_test_counter* _counter,
    int64_t                 _amount
);

/**
 * @brief Moves a test counter down by `_amount`.
 *
 * @param[in,out] _counter  the counter.
 * @param[in]     _amount   how far to move it; negative moves it up.
 * @return as d_counter_decrement.
 */
bool
d_test_counter_decrement(
    struct dj_test_counter* _counter,
    int64_t                 _amount
);

/**
 * @brief Orders two test counters by value, whatever they count.
 *
 * @param[in] _a  the first counter; may be `NULL`.
 * @param[in] _b  the second counter; may be `NULL`.
 * @return as d_counter_compare.
 */
int
d_test_counter_compare(
    const struct dj_test_counter* _a,
    const struct dj_test_counter* _b
);

/**
 * @brief Adds `_source`'s movement since its start to `_destination`.
 *
 * @note What the two count need not match: merge combines counts, and the
 *       caller decides what they mean.
 *
 * @param[in,out] _destination  the counter that gains the movement.
 * @param[in]     _source       the counter whose movement is added.
 * @return as d_counter_merge.
 */
bool
d_test_counter_merge(
    struct dj_test_counter*       _destination,
    const struct dj_test_counter* _source
);

/**
 * @brief Returns a test counter to its start.
 *
 * @param[in,out] _counter  the counter; `NULL` is ignored.
 * @post The step is unchanged.
 */
void
d_test_counter_reset(
    struct dj_test_counter* _counter
);

// 2.5    Queries
//------------------------------------------------------------------------------
/**
 * @brief Returns a test counter's count.
 *
 * @param[in] _counter  the counter; may be `NULL`.
 * @return the value, or 0 for `NULL`.
 */
int64_t
d_test_counter_value(
    const struct dj_test_counter* _counter
);

/**
 * @brief Returns what one occurrence moves a test counter by.
 *
 * @param[in] _counter  the counter; may be `NULL`.
 * @return the step, or 0 for `NULL`.
 */
int64_t
d_test_counter_step(
    const struct dj_test_counter* _counter
);

/**
 * @brief Returns a test counter as the generic counter it begins with.
 *
 * @param[in] _counter  the counter; may be `NULL`.
 * @return `&_counter->counter`, for counter.h's functions, or `NULL` for
 *         `NULL`.
 */
struct dj_counter*
d_test_counter_as_counter(
    struct dj_test_counter* _counter
);


//==============================================================================
// 3.  THE TALLY
//==============================================================================
// A NULL tally tracks nothing and counts zero.


// 3.1    Options and initialization
//------------------------------------------------------------------------------
/**
 * @brief Sets tally options to the knobs' defaults.
 *
 * @param[out] _options  the options; `NULL` is ignored.
 * @post `limit` is D_TEST_TALLY_CAPACITY, `when_full` is
 *       D_CFG_TEST_TALLY_WHEN_FULL, and `counter` is
 *       d_test_counter_options_init's.
 */
void
d_test_tally_options_init(
    struct dj_test_tally_options* _options
);

/**
 * @brief Sets an empty tally with the given options.
 *
 * @param[out] _tally    the tally to set.
 * @param[in]  _options  the options; `NULL` means the knobs' defaults.
 * @return `true` when set; `false` when `_tally` is `NULL`, the limit is 0
 *         or above D_TEST_TALLY_CAPACITY, the full-tally policy is unknown,
 *         or the counter step is 0, and then nothing changes.
 * @pre No handler is bound to count into the tally.
 */
bool
d_test_tally_init(
    struct dj_test_tally*               _tally,
    const struct dj_test_tally_options* _options
);

/**
 * @brief Returns the options a tally runs with.
 *
 * @param[in]  _tally  the tally; may be `NULL`.
 * @param[out] _out    the options: the tally's own, or the knobs' defaults
 *                     for a zero-filled tally or `NULL`.
 */
void
d_test_tally_options_of(
    const struct dj_test_tally*   _tally,
    struct dj_test_tally_options* _out
);

// 3.2    Tracking
//------------------------------------------------------------------------------
/**
 * @brief Returns the counter for `_key`, tracking the key if it is new.
 *
 * @param[in,out] _tally  the tally.
 * @param[in]     _key    the key.
 * @return the key's counter, a new one at 0 with the tally's counter options
 *         when the key was not tracked; `NULL` when the tally has reached its
 *         limit, or is `NULL`.
 * @post A returned counter stays at its address until the tally is cleared.
 */
struct dj_test_counter*
d_test_tally_track(
    struct dj_test_tally* _tally,
    dj_test_key           _key
);

/**
 * @brief Returns the counter for `_key`, if it is tracked.
 *
 * @param[in] _tally  the tally; may be `NULL`.
 * @param[in] _key    the key.
 * @return the key's counter, or `NULL` when it is not tracked.
 */
struct dj_test_counter*
d_test_tally_find(
    struct dj_test_tally* _tally,
    dj_test_key           _key
);

/**
 * @brief Forgets every tracked key.
 *
 * @param[in,out] _tally  the tally; `NULL` is ignored.
 * @pre No handler is bound to a counter of the tally.
 * @post The tally is empty; every counter it returned is invalid.
 */
void
d_test_tally_clear(
    struct dj_test_tally* _tally
);

// 3.3    Counting
//------------------------------------------------------------------------------
/**
 * @brief Records one occurrence of `_key`: moves its counter by its step,
 *        tracking the key if it must.
 *
 * @param[in,out] _tally  the tally.
 * @param[in]     _key    the key.
 * @return as d_test_tally_increment, by the counter's step; past the limit,
 *         an overflowing tally moves its overflow by the tally's step.
 */
bool
d_test_tally_record(
    struct dj_test_tally* _tally,
    dj_test_key           _key
);

/**
 * @brief Moves the counter for `_key` up by `_amount`, tracking the key if it
 *        must.
 *
 * @param[in,out] _tally   the tally.
 * @param[in]     _key     the key.
 * @param[in]     _amount  how far to move it; negative moves it down.
 * @return `true` when the full amount applied; `false` when the tally is
 *         `NULL`, the step was cut, or the key could not be tracked and the
 *         tally refuses (under D_TEST_TALLY_FULL_OVERFLOW it moves the
 *         overflow counter instead, and reports that step).
 */
bool
d_test_tally_increment(
    struct dj_test_tally* _tally,
    dj_test_key           _key,
    int64_t               _amount
);

/**
 * @brief Returns the count for `_key`.
 *
 * @param[in] _tally  the tally; may be `NULL`.
 * @param[in] _key    the key.
 * @return the count, or 0 for a key not tracked.
 */
int64_t
d_test_tally_count(
    const struct dj_test_tally* _tally,
    dj_test_key                 _key
);

/**
 * @brief Returns what a tally counted past its limit.
 *
 * @param[in] _tally  the tally; may be `NULL`.
 * @return the overflow counter's value: 0 unless the tally overflows.
 */
int64_t
d_test_tally_overflow(
    const struct dj_test_tally* _tally
);

/**
 * @brief Returns every counter of a tally to its start.
 *
 * @param[in,out] _tally  the tally; `NULL` is ignored.
 * @post The keys tracked are unchanged; the overflow is zero.
 */
void
d_test_tally_reset(
    struct dj_test_tally* _tally
);

// 3.4    Combining
//------------------------------------------------------------------------------
/**
 * @brief Merges every counter of `_source` into `_destination`, key by key.
 *
 * @note A key `_destination` does not track yet is tracked first, or,
 *       past its limit, follows its full-tally policy; the overflow merges
 *       into the overflow. Merging a tally into itself doubles each count's
 *       movement once.
 *
 * @param[in,out] _destination  the tally merged into.
 * @param[in]     _source       the tally merged from.
 * @return `true` when every counter merged in full; `false` for a `NULL`
 *         argument, a full `_destination` that refuses, or a cut merge,
 *         after merging all it could.
 */
bool
d_test_tally_merge(
    struct dj_test_tally*       _destination,
    const struct dj_test_tally* _source
);


//==============================================================================
// 4.  THE ABSTRACT MACHINE (OPT-IN)
//==============================================================================
// With D_CFG_TEST_COUNTER_MACHINE, the settings above as variables of an
// option_env frame (c/option/option_env.h): a program declares the schema in
// its machine's frame, binds what it likes -- "tally_limit=8" by text, a
// script, the environment, the command line -- and reads the result into a
// struct dj_test_tally_options. Each variable's default is pinned to its
// knob, so a frame that binds nothing reads the knobs' defaults.

#if ( D_INTERNAL_TEST_COUNTER_MACHINE && RE_STD_HAS_INT64 )
    // 4.1    The schema
    //--------------------------------------------------------------------------
    // 4.1.1
    // D_TEST_COUNTER_ENV_VAR_COUNT
    //   constant: the rows of d_test_counter_env_vars: tally_limit,
    // tally_when_full, counter_step, counter_floor.
    #define D_TEST_COUNTER_ENV_VAR_COUNT  4u

    /**
     * @brief The counters' schema, for d_option_env_declare_all.
     *
     * @return the D_TEST_COUNTER_ENV_VAR_COUNT rows; constant, and borrowed
     *         by any frame that declares them.
     */
    const struct dj_option_env_var*
    d_test_counter_env_vars(void);

    // 4.2    Reading a frame
    //--------------------------------------------------------------------------
    /**
     * @brief Reads tally options from a frame that declares the schema.
     *
     * @param[in]  _env    the frame; any of the schema's variables it does
     *                     not declare keeps its knob's default.
     * @param[in]  _scope  the scope to read in; D_OPTION_ENV_GLOBAL for none.
     * @param[out] _out    the options.
     * @return `true` when read; `false` when `_out` is `NULL`.
     */
    bool
    d_test_tally_options_from_env(
        const struct dj_option_env*   _env,
        uint32_t                      _scope,
        struct dj_test_tally_options* _out
    );

#endif  // D_INTERNAL_TEST_COUNTER_MACHINE && RE_STD_HAS_INT64


D_EXTERN_C_END


#endif  // DJINTERP_C_TEST_COUNTER_H
