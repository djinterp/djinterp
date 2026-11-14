/*******************************************************************************
* djinterp [c][test]                                              test_counter.c
*
* DTest's C counters: the test counter forwards to counter.h; the tally is a
* linear search over a short array, which for the handful of keys a suite
* counts beats any index. Every setting resolves the same way: the options
* given, else the knobs'.
*
*
* path:      /src/djinterp/c/modules/test/test_counter.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "../../../../../inc/djinterp/c/modules/test/test_counter.h"  // corresponding header
#if ( D_INTERNAL_TEST_COUNTER_MACHINE && RE_STD_HAS_INT64 )
    // std
    #include <string.h>  // strlen
#endif  // D_INTERNAL_TEST_COUNTER_MACHINE && RE_STD_HAS_INT64


/**
 * @brief The interval a floored counter is held in: zero and up.
 *
 * @return [0, D_MATH_IMAX_MAX], closed and continuous.
 */
static struct dj_interval_imax
d_internal_test_counter_floor(void)
{
    return d_interval_imax_make(0,
                                D_MATH_IMAX_MAX,
                                D_INTERVAL_CLOSED,
                                0);
}

/*
d_test_counter_options_init
  The knobs, resolved by cfg_test_counter.h.
*/
void
d_test_counter_options_init(
    struct dj_test_counter_options* _options
)
{
    // parameter validation
    if (!_options)
    {
        return;
    }

    _options->step      = (int64_t)D_INTERNAL_TEST_COUNTER_STEP;
    _options->has_floor = (D_INTERNAL_TEST_COUNTER_FLOOR != 0);

    return;
}

/*
d_test_counter_init
  init_with and the knobs: the same counter D_TEST_COUNTER_INIT gives.
*/
void
d_test_counter_init(
    struct dj_test_counter* _counter
)
{
    (void)d_test_counter_init_with(_counter,
                                   NULL);

    return;
}

/*
d_test_counter_init_with
  Everything is checked before anything is written.
*/
bool
d_test_counter_init_with(
    struct dj_test_counter*               _counter,
    const struct dj_test_counter_options* _options
)
{
    struct dj_test_counter_options options = { 0, false };

    d_test_counter_options_init(&options);

    // the given options, where there are any
    if (_options)
    {
        options = *_options;
    }

    // parameter validation: a step of 0 counts nothing
    if ( (!_counter) ||
         (options.step == 0) )
    {
        return false;
    }

    const struct dj_interval_imax floor = d_internal_test_counter_floor();

    (void)d_counter_init_bounded(&_counter->counter,
                                 0,
                                 (options.has_floor) ? &floor : NULL);
    _counter->step = options.step;

    return true;
}

/*
d_test_counter_record
  One occurrence is one step; a zero-filled counter steps by the knob's.
*/
bool
d_test_counter_record(
    struct dj_test_counter* _counter
)
{
    return d_test_counter_increment(_counter,
                                    d_test_counter_step(_counter));
}

/*
d_test_counter_increment
  counter.h's, on the test counter's counter.
*/
bool
d_test_counter_increment(
    struct dj_test_counter* _counter,
    int64_t                 _amount
)
{
    return d_counter_increment(d_test_counter_as_counter(_counter),
                               _amount);
}

/*
d_test_counter_decrement
  counter.h's, on the test counter's counter.
*/
bool
d_test_counter_decrement(
    struct dj_test_counter* _counter,
    int64_t                 _amount
)
{
    return d_counter_decrement(d_test_counter_as_counter(_counter),
                               _amount);
}

/*
d_test_counter_compare
  counter.h's, on the test counters' counters: by value, NULL first.
*/
int
d_test_counter_compare(
    const struct dj_test_counter* _a,
    const struct dj_test_counter* _b
)
{
    return d_counter_compare( (_a) ? &_a->counter : NULL,
                              (_b) ? &_b->counter : NULL );
}

/*
d_test_counter_merge
  counter.h's, on the test counters' counters.
*/
bool
d_test_counter_merge(
    struct dj_test_counter*       _destination,
    const struct dj_test_counter* _source
)
{
    return d_counter_merge(d_test_counter_as_counter(_destination),
                           (_source) ? &_source->counter : NULL);
}

/*
d_test_counter_reset
  counter.h's, on the test counter's counter.
*/
void
d_test_counter_reset(
    struct dj_test_counter* _counter
)
{
    d_counter_reset(d_test_counter_as_counter(_counter));

    return;
}

/*
d_test_counter_value
  The counter's value.
*/
int64_t
d_test_counter_value(
    const struct dj_test_counter* _counter
)
{
    return (_counter) ? d_counter_value(&_counter->counter) : 0;
}

/*
d_test_counter_step
  The step set at init; a zero-filled counter has none yet, and takes the
knob's, so it counts as one set by D_TEST_COUNTER_INIT would.
*/
int64_t
d_test_counter_step(
    const struct dj_test_counter* _counter
)
{
    // no counter, no step
    if (!_counter)
    {
        return 0;
    }

    return (_counter->step != 0) ? _counter->step
                                 : (int64_t)D_INTERNAL_TEST_COUNTER_STEP;
}

/*
d_test_counter_as_counter
  The counter as counter.h sees it.
*/
struct dj_counter*
d_test_counter_as_counter(
    struct dj_test_counter* _counter
)
{
    return (_counter) ? &_counter->counter : NULL;
}

/*
d_test_tally_options_init
  The knobs, resolved by cfg_test_counter.h.
*/
void
d_test_tally_options_init(
    struct dj_test_tally_options* _options
)
{
    // parameter validation
    if (!_options)
    {
        return;
    }

    _options->limit     = (size_t)D_TEST_TALLY_CAPACITY;
    _options->when_full =
        (enum dj_test_tally_when_full)D_INTERNAL_TEST_TALLY_WHEN_FULL;
    d_test_counter_options_init(&_options->counter);

    return;
}

/*
d_test_tally_init
  Everything is checked before anything is written.
*/
bool
d_test_tally_init(
    struct dj_test_tally*               _tally,
    const struct dj_test_tally_options* _options
)
{
    struct dj_test_tally_options options = { 0, D_TEST_TALLY_FULL_REFUSE,
                                             { 0, false } };

    d_test_tally_options_init(&options);

    // the given options, where there are any
    if (_options)
    {
        options = *_options;
    }

    // parameter validation
    if ( (!_tally)                                  ||
         (options.limit == 0)                       ||
         (options.limit > D_TEST_TALLY_CAPACITY)    ||
         ( (options.when_full != D_TEST_TALLY_FULL_REFUSE) &&
           (options.when_full != D_TEST_TALLY_FULL_OVERFLOW) ) ||
         (options.counter.step == 0) )
    {
        return false;
    }

    _tally->count       = 0;
    _tally->has_options = true;
    _tally->options     = options;
    d_counter_init(&_tally->overflow,
                   0);

    return true;
}

/*
d_test_tally_options_of
  A zero-filled tally has not set has_options, and runs on the knobs.
*/
void
d_test_tally_options_of(
    const struct dj_test_tally*   _tally,
    struct dj_test_tally_options* _out
)
{
    // parameter validation
    if (!_out)
    {
        return;
    }

    // the tally's own, or the knobs'
    if ( (_tally) &&
         (_tally->has_options) )
    {
        *_out = _tally->options;
    }
    else
    {
        d_test_tally_options_init(_out);
    }

    return;
}

/*
d_test_tally_track
  Found, or added at the end; a full tally adds nothing.
*/
struct dj_test_counter*
d_test_tally_track(
    struct dj_test_tally* _tally,
    dj_test_key           _key
)
{
    struct dj_test_counter* found = d_test_tally_find(_tally,
                                                      _key);

    // tracked already, or no tally to track it in
    if ( (found) ||
         (!_tally) )
    {
        return found;
    }

    struct dj_test_tally_options options = { 0, D_TEST_TALLY_FULL_REFUSE,
                                             { 0, false } };

    d_test_tally_options_of(_tally,
                            &options);

    // at the limit: nowhere to track it
    if (_tally->count >= options.limit)
    {
        return NULL;
    }

    struct dj_test_tally_entry* added = &_tally->entries[_tally->count];

    // the tally's options were checked when set
    added->key = _key;
    (void)d_test_counter_init_with(&added->counter,
                                   &options.counter);
    ++_tally->count;

    return &added->counter;
}

/*
d_test_tally_find
  A linear search: a tally holds a handful of keys.
*/
struct dj_test_counter*
d_test_tally_find(
    struct dj_test_tally* _tally,
    dj_test_key           _key
)
{
    // nothing tracked in NULL
    if (!_tally)
    {
        return NULL;
    }

    // the first, and only, counter for the key
    for (size_t i = 0; i < _tally->count; ++i)
    {
        // this key's
        if (_tally->entries[i].key == _key)
        {
            return &_tally->entries[i].counter;
        }
    }

    return NULL;
}

/*
d_test_tally_clear
  Forgets every key; a handler still bound to one of its counters must be
unbound first, since the counter stops being one. The options stay.
*/
void
d_test_tally_clear(
    struct dj_test_tally* _tally
)
{
    // parameter validation
    if (!_tally)
    {
        return;
    }

    _tally->count = 0;
    d_counter_init(&_tally->overflow,
                   0);

    return;
}

/*
d_test_tally_record
  increment by the counter's step, or, past the limit, by the tally's.
*/
bool
d_test_tally_record(
    struct dj_test_tally* _tally,
    dj_test_key           _key
)
{
    struct dj_test_counter* counter = d_test_tally_track(_tally,
                                                         _key);

    // tracked: one occurrence of its key
    if (counter)
    {
        return d_test_counter_record(counter);
    }

    struct dj_test_tally_options options = { 0, D_TEST_TALLY_FULL_REFUSE,
                                             { 0, false } };

    d_test_tally_options_of(_tally,
                            &options);

    // untracked: as increment, by the step new counters would take
    return d_test_tally_increment(_tally,
                                  _key,
                                  options.counter.step);
}

/*
d_test_tally_increment
  Tracking and counting in one call, which is how most callers count. A key
that cannot be tracked goes to the overflow, when the tally overflows.
*/
bool
d_test_tally_increment(
    struct dj_test_tally* _tally,
    dj_test_key           _key,
    int64_t               _amount
)
{
    struct dj_test_counter* counter = d_test_tally_track(_tally,
                                                         _key);

    // tracked: counted under its key
    if (counter)
    {
        return d_test_counter_increment(counter,
                                        _amount);
    }

    struct dj_test_tally_options options = { 0, D_TEST_TALLY_FULL_REFUSE,
                                             { 0, false } };

    d_test_tally_options_of(_tally,
                            &options);

    // untracked: the overflow takes it, or the tally refuses it
    if ( (_tally) &&
         (options.when_full == D_TEST_TALLY_FULL_OVERFLOW) )
    {
        return d_counter_increment(&_tally->overflow,
                                   _amount);
    }

    return false;
}

/*
d_test_tally_count
  The key's count, or 0 when it is not tracked. The search is find's,
written again for a const tally.
*/
int64_t
d_test_tally_count(
    const struct dj_test_tally* _tally,
    dj_test_key                 _key
)
{
    // nothing counted in NULL
    if (!_tally)
    {
        return 0;
    }

    // the key's counter, if tracked
    for (size_t i = 0; i < _tally->count; ++i)
    {
        // this key's
        if (_tally->entries[i].key == _key)
        {
            return d_test_counter_value(&_tally->entries[i].counter);
        }
    }

    return 0;
}

/*
d_test_tally_overflow
  The overflow counter's value; it moves only under FULL_OVERFLOW.
*/
int64_t
d_test_tally_overflow(
    const struct dj_test_tally* _tally
)
{
    return (_tally) ? d_counter_value(&_tally->overflow) : 0;
}

/*
d_test_tally_reset
  Every counter back to its start, the overflow too; the keys stay
tracked.
*/
void
d_test_tally_reset(
    struct dj_test_tally* _tally
)
{
    // parameter validation
    if (!_tally)
    {
        return;
    }

    // each counter
    for (size_t i = 0; i < _tally->count; ++i)
    {
        d_test_counter_reset(&_tally->entries[i].counter);
    }

    d_counter_reset(&_tally->overflow);

    return;
}

/*
d_test_tally_merge
  Key by key. The source's count is read before the destination grows,
so merging a tally into itself walks only the counters it began with.
*/
bool
d_test_tally_merge(
    struct dj_test_tally*       _destination,
    const struct dj_test_tally* _source
)
{
    // parameter validation
    if ( (!_destination) ||
         (!_source) )
    {
        return false;
    }

    struct dj_test_tally_options options = { 0, D_TEST_TALLY_FULL_REFUSE,
                                             { 0, false } };
    const size_t                 count   = _source->count;
    bool                         ok      = true;

    d_test_tally_options_of(_destination,
                            &options);

    // merge each key, tracking the new ones
    for (size_t i = 0; i < count; ++i)
    {
        const struct dj_test_tally_entry* entry = &_source->entries[i];
        const struct dj_test_counter*     from  = &entry->counter;
        struct dj_test_counter*           into  = d_test_tally_track(
                                                      _destination,
                                                      entry->key);

        // past the limit, an overflowing tally keeps the count all the same
        if ( (!into) &&
             (options.when_full == D_TEST_TALLY_FULL_OVERFLOW) )
        {
            ok = ( (d_counter_merge(&_destination->overflow,
                                    &from->counter)) &&
                   (ok) );

            continue;
        }

        // a full tally, or a cut merge, spoils the result but not the rest
        if (!d_test_counter_merge(into,
                                  from))
        {
            ok = false;
        }
    }

    // what the source could not tell apart, the destination cannot either
    if (!d_counter_merge(&_destination->overflow,
                         &_source->overflow))
    {
        ok = false;
    }

    return ok;
}

// the abstract machine, only when a build opts in (test_counter.h, section 4)
#if ( D_INTERNAL_TEST_COUNTER_MACHINE && RE_STD_HAS_INT64 )
    // d_internal_test_tally_when_full_names
    //   constant: the names tally_when_full takes, with their values.
    static const struct dj_option_env_name
    d_internal_test_tally_when_full_names[] =
    {
        { "refuse",   (int64_t)D_TEST_TALLY_FULL_REFUSE },
        { "overflow", (int64_t)D_TEST_TALLY_FULL_OVERFLOW }
    };

    // d_internal_test_counter_env_vars
    //   constant: the schema, each default pinned to its knob.
    static const struct dj_option_env_var
    d_internal_test_counter_env_vars[D_TEST_COUNTER_ENV_VAR_COUNT] =
    {
        D_OPTION_ENV_INT_RANGE_KNOB("tally_limit",
                                    D_CFG_TEST_TALLY_CAPACITY,
                                    1,
                                    D_CFG_TEST_TALLY_CAPACITY,
                                    "keys one tally tracks at most"),
        D_OPTION_ENV_CHOICE_KNOB("tally_when_full",
                                 d_internal_test_tally_when_full_names,
                                 D_CFG_TEST_TALLY_WHEN_FULL,
                                 "a full tally refuses another key, or "
                                 "counts it in its overflow"),
        D_OPTION_ENV_INT_KNOB("counter_step",
                              D_CFG_TEST_COUNTER_STEP,
                              "how far one occurrence moves a test counter; "
                              "not 0"),
        D_OPTION_ENV_BOOL_KNOB("counter_floor",
                               D_CFG_TEST_COUNTER_FLOOR,
                               "hold test counters at or above zero")
    };

    /**
     * @brief Reads one of the schema's INT or CHOICE variables from a frame.
     *
     * @param[in] _env      the frame.
     * @param[in] _scope    the scope.
     * @param[in] _name     the variable's name.
     * @param[in] _default  what to give when the frame does not declare it.
     * @return the value in force, or `_default`.
     */
    static int64_t
    d_internal_test_counter_env_int(
        const struct dj_option_env* _env,
        uint32_t                    _scope,
        const char*                 _name,
        int64_t                     _default
    )
    {
        const struct dj_option_result found = d_option_env_find(_env,
                                                                _name,
                                                                strlen(_name));

        // a frame without the schema keeps the knob's default
        if (!found.is_ok)
        {
            return _default;
        }

        return d_option_env_int(_env,
                                _scope,
                                found.payload.value);
    }

    /**
     * @brief Reads one of the schema's BOOL variables from a frame.
     *
     * @param[in] _env      the frame.
     * @param[in] _scope    the scope.
     * @param[in] _name     the variable's name.
     * @param[in] _default  what to give when the frame does not declare it.
     * @return the value in force, or `_default`.
     */
    static bool
    d_internal_test_counter_env_bool(
        const struct dj_option_env* _env,
        uint32_t                    _scope,
        const char*                 _name,
        bool                        _default
    )
    {
        const struct dj_option_result found = d_option_env_find(_env,
                                                                _name,
                                                                strlen(_name));

        // a frame without the schema keeps the knob's default
        if (!found.is_ok)
        {
            return _default;
        }

        return d_option_env_bool(_env,
                                 _scope,
                                 found.payload.value);
    }

    /*
    d_test_counter_env_vars
      The table is constant, so frames may borrow it for as long as they live.
    */
    const struct dj_option_env_var*
    d_test_counter_env_vars(void)
    {
        return d_internal_test_counter_env_vars;
    }

    /*
    d_test_tally_options_from_env
      Starts from the knobs and takes each variable the frame declares; a step
    of 0, which the schema cannot rule out, keeps the knob's.
    */
    bool
    d_test_tally_options_from_env(
        const struct dj_option_env*   _env,
        uint32_t                      _scope,
        struct dj_test_tally_options* _out
    )
    {
        // parameter validation
        if (!_out)
        {
            return false;
        }

        d_test_tally_options_init(_out);

        // a missing frame reads as the knobs
        if (!_env)
        {
            return true;
        }

        const int64_t limit = d_internal_test_counter_env_int(
                                  _env,
                                  _scope,
                                  "tally_limit",
                                  (int64_t)_out->limit);
        const int64_t full  = d_internal_test_counter_env_int(
                                  _env,
                                  _scope,
                                  "tally_when_full",
                                  (int64_t)_out->when_full);
        const int64_t step  = d_internal_test_counter_env_int(
                                  _env,
                                  _scope,
                                  "counter_step",
                                  _out->counter.step);
        const bool    floor = d_internal_test_counter_env_bool(
                                  _env,
                                  _scope,
                                  "counter_floor",
                                  _out->counter.has_floor);

        _out->limit             = (size_t)limit;
        _out->when_full         = (enum dj_test_tally_when_full)full;
        _out->counter.step      = (step != 0) ? step : _out->counter.step;
        _out->counter.has_floor = floor;

        return true;
    }
#endif  // D_INTERNAL_TEST_COUNTER_MACHINE && RE_STD_HAS_INT64
