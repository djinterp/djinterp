// enable POSIX features for clock_gettime
#if !defined(_WIN32) && !defined(_WIN64)
    #define _POSIX_C_SOURCE 199309L
#endif

#include "../../../inc/c/test/test_stats.h"

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#else
    #include <time.h>
    #include <sys/time.h>
#endif


/******************************************************************************
 * COUNTER UTILITY FUNCTIONS
 *****************************************************************************/

/*
d_test_stat_counter_reset
  Resets all fields of a d_test_stat_counter to zero.

Parameter(s):
  _counter: pointer to the counter to reset
Return:
  none
*/
void
d_test_stat_counter_reset
(
    struct d_test_stat_counter* _counter
)
{
    if (!_counter)
    {
        return;
    }

    _counter->skipped = 0;
    _counter->passed  = 0;
    _counter->failed  = 0;
    _counter->run     = 0;

    return;
}

/*
d_test_stat_counter_add
  Adds the values from one counter to another.

Parameter(s):
  _dest: destination counter to add to
  _src:  source counter to add from
Return:
  none
*/
void
d_test_stat_counter_add
(
    struct d_test_stat_counter*       _dest,
    const struct d_test_stat_counter* _src
)
{
    if ( (!_dest) ||
         (!_src) )
    {
        return;
    }

    _dest->skipped += _src->skipped;
    _dest->passed  += _src->passed;
    _dest->failed  += _src->failed;
    _dest->run     += _src->run;

    return;
}

/*
d_test_stat_counter_total
  Returns the total count (run + skipped) from a counter.

Parameter(s):
  _counter: pointer to the counter
Return:
  Total count of all items tracked by this counter.
*/
size_t
d_test_stat_counter_total
(
    const struct d_test_stat_counter* _counter
)
{
    if (!_counter)
    {
        return 0;
    }

    return _counter->run + _counter->skipped;
}


/******************************************************************************
 * INTERNAL HELPERS
 *****************************************************************************/

/*
d_internal_get_time_ms
  Gets the current time in milliseconds.

Parameter(s):
  none
Return:
  Current time in milliseconds as a double.
*/
static double
d_internal_get_time_ms
(
    void
)
{
#if defined(_WIN32) || defined(_WIN64)
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);

    return (double)counter.QuadPart * 1000.0 / (double)frequency.QuadPart;
#else
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
#endif
}


/******************************************************************************
 * STATISTICS UTILITY FUNCTIONS
 *****************************************************************************/

/*
d_test_statistics_reset
  Resets all fields of a d_test_statistics structure to zero.

Parameter(s):
  _stats: pointer to the statistics structure to reset
Return:
  none
*/
void
d_test_statistics_reset
(
    struct d_test_statistics* _stats
)
{
    if (!_stats)
    {
        return;
    }

    d_test_stat_counter_reset(&_stats->asserts);
    d_test_stat_counter_reset(&_stats->test_fns);
    d_test_stat_counter_reset(&_stats->tests);
    d_test_stat_counter_reset(&_stats->blocks);
    d_test_stat_counter_reset(&_stats->modules);

    _stats->max_depth     = 0;
    _stats->current_depth = 0;
    _stats->total_time_ms = 0.0;
    _stats->start_time_ms = 0.0;
    _stats->end_time_ms   = 0.0;

    return;
}

/*
d_test_statistics_add
  Adds the values from one statistics structure to another.

Parameter(s):
  _dest: destination statistics to add to
  _src:  source statistics to add from
Return:
  none
*/
void
d_test_statistics_add
(
    struct d_test_statistics*       _dest,
    const struct d_test_statistics* _src
)
{
    if ( (!_dest) ||
         (!_src) )
    {
        return;
    }

    d_test_stat_counter_add(&_dest->asserts,  &_src->asserts);
    d_test_stat_counter_add(&_dest->test_fns, &_src->test_fns);
    d_test_stat_counter_add(&_dest->tests,    &_src->tests);
    d_test_stat_counter_add(&_dest->blocks,   &_src->blocks);
    d_test_stat_counter_add(&_dest->modules,  &_src->modules);

    // update max depth if source has a deeper max
    if (_src->max_depth > _dest->max_depth)
    {
        _dest->max_depth = _src->max_depth;
    }

    _dest->total_time_ms += _src->total_time_ms;

    return;
}

/*
d_test_statistics_total_run
  Returns the total number of items run across all categories.

Parameter(s):
  _stats: pointer to the statistics structure
Return:
  Total count of all items that were run.
*/
size_t
d_test_statistics_total_run
(
    const struct d_test_statistics* _stats
)
{
    if (!_stats)
    {
        return 0;
    }

    return _stats->asserts.run  +
           _stats->test_fns.run +
           _stats->tests.run    +
           _stats->blocks.run   +
           _stats->modules.run;
}

/*
d_test_statistics_total_passed
  Returns the total number of items passed across all categories.

Parameter(s):
  _stats: pointer to the statistics structure
Return:
  Total count of all items that passed.
*/
size_t
d_test_statistics_total_passed
(
    const struct d_test_statistics* _stats
)
{
    if (!_stats)
    {
        return 0;
    }

    return _stats->asserts.passed  +
           _stats->test_fns.passed +
           _stats->tests.passed    +
           _stats->blocks.passed   +
           _stats->modules.passed;
}

/*
d_test_statistics_total_failed
  Returns the total number of items failed across all categories.

Parameter(s):
  _stats: pointer to the statistics structure
Return:
  Total count of all items that failed.
*/
size_t
d_test_statistics_total_failed
(
    const struct d_test_statistics* _stats
)
{
    if (!_stats)
    {
        return 0;
    }

    return _stats->asserts.failed  +
           _stats->test_fns.failed +
           _stats->tests.failed    +
           _stats->blocks.failed   +
           _stats->modules.failed;
}

/*
d_test_statistics_total_skipped
  Returns the total number of items skipped across all categories.

Parameter(s):
  _stats: pointer to the statistics structure
Return:
  Total count of all items that were skipped.
*/
size_t
d_test_statistics_total_skipped
(
    const struct d_test_statistics* _stats
)
{
    if (!_stats)
    {
        return 0;
    }

    return _stats->asserts.skipped  +
           _stats->test_fns.skipped +
           _stats->tests.skipped    +
           _stats->blocks.skipped   +
           _stats->modules.skipped;
}

/*
d_test_statistics_all_passed
  Checks if all tests passed (no failures across any category).

Parameter(s):
  _stats: pointer to the statistics structure
Return:
  true if no failures occurred, false otherwise.
*/
bool
d_test_statistics_all_passed
(
    const struct d_test_statistics* _stats
)
{
    if (!_stats)
    {
        return false;
    }

    return d_test_statistics_total_failed(_stats) == 0;
}


/******************************************************************************
 * TIMING FUNCTIONS
 *****************************************************************************/

/*
d_test_statistics_start_timer
  Records the start time for test execution timing.

Parameter(s):
  _stats: pointer to the statistics structure
Return:
  none
*/
void
d_test_statistics_start_timer
(
    struct d_test_statistics* _stats
)
{
    if (!_stats)
    {
        return;
    }

    _stats->start_time_ms = d_internal_get_time_ms();

    return;
}

/*
d_test_statistics_stop_timer
  Records the end time and calculates total elapsed time.

Parameter(s):
  _stats: pointer to the statistics structure
Return:
  none
*/
void
d_test_statistics_stop_timer
(
    struct d_test_statistics* _stats
)
{
    if (!_stats)
    {
        return;
    }

    _stats->end_time_ms   = d_internal_get_time_ms();
    _stats->total_time_ms = _stats->end_time_ms - _stats->start_time_ms;

    return;
}


/******************************************************************************
 * RATE CALCULATION FUNCTIONS
 *****************************************************************************/

/*
d_test_stat_counter_pass_rate
  Calculates the pass rate as a percentage (0.0 to 100.0).

Parameter(s):
  _counter: pointer to the counter structure
Return:
  Pass rate percentage, or 0.0 if no tests were run.
*/
double
d_test_stat_counter_pass_rate
(
    const struct d_test_stat_counter* _counter
)
{
    if ( (!_counter) ||
         (_counter->run == 0) )
    {
        return 0.0;
    }

    return (double)_counter->passed / (double)_counter->run * 100.0;
}


/*
d_test_stat_counter_assert_rate
  Calculates the assertion pass rate as a percentage (0.0 to 100.0).

Parameter(s):
  _stats: pointer to the statistics structure
Return:
  Assertion pass rate percentage, or 0.0 if no assertions were run.
*/
double
d_test_stat_counter_assert_rate
(
    const struct d_test_statistics* _stats
)
{
    if ( (!_stats) ||
         (_stats->asserts.run == 0) )
    {
        return 0.0;
    }

    return (double)_stats->asserts.passed / (double)_stats->asserts.run * 100.0;
}