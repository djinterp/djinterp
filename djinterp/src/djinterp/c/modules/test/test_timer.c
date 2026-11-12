/*******************************************************************************
* djinterp [c][test]                                                test_timer.c
*
* DTest's timer: timer.h's functions, forwarded to the timer it wraps.
*
*
* path:      /src/djinterp/c/modules/test/test_timer.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "../../../../../inc/djinterp/c/modules/test/test_timer.h"  // corresponding header


/*
d_test_timer_init
  timer.h's, on the wrapped timer.
*/
void
d_test_timer_init(
    struct dj_test_timer* _timer
)
{
    d_timer_init(d_test_timer_as_timer(_timer));

    return;
}

/*
d_test_timer_set_clock
  timer.h's, on the wrapped timer.
*/
void
d_test_timer_set_clock(
    struct dj_test_timer* _timer,
    fn_timer_clock        _clock,
    void*                 _context
)
{
    d_timer_set_clock(d_test_timer_as_timer(_timer),
                      _clock,
                      _context);

    return;
}

/*
d_test_timer_start
  timer.h's, on the wrapped timer.
*/
bool
d_test_timer_start(
    struct dj_test_timer* _timer
)
{
    return d_timer_start(d_test_timer_as_timer(_timer));
}

/*
d_test_timer_stop
  timer.h's, on the wrapped timer.
*/
bool
d_test_timer_stop(
    struct dj_test_timer* _timer
)
{
    return d_timer_stop(d_test_timer_as_timer(_timer));
}

/*
d_test_timer_increment
  timer.h's, on the wrapped timer.
*/
bool
d_test_timer_increment(
    struct dj_test_timer* _timer,
    int64_t               _amount
)
{
    return d_timer_increment(d_test_timer_as_timer(_timer),
                             _amount);
}

/*
d_test_timer_decrement
  timer.h's, on the wrapped timer.
*/
bool
d_test_timer_decrement(
    struct dj_test_timer* _timer,
    int64_t               _amount
)
{
    return d_timer_decrement(d_test_timer_as_timer(_timer),
                             _amount);
}

/*
d_test_timer_compare
  timer.h's, on the wrapped timers.
*/
int
d_test_timer_compare(
    const struct dj_test_timer* _a,
    const struct dj_test_timer* _b
)
{
    return d_timer_compare( (_a) ? &_a->timer : NULL,
                            (_b) ? &_b->timer : NULL );
}

/*
d_test_timer_merge
  timer.h's, on the wrapped timers.
*/
bool
d_test_timer_merge(
    struct dj_test_timer*       _destination,
    const struct dj_test_timer* _source
)
{
    return d_timer_merge(d_test_timer_as_timer(_destination),
                         (_source) ? &_source->timer : NULL);
}

/*
d_test_timer_reset
  timer.h's, on the wrapped timer.
*/
void
d_test_timer_reset(
    struct dj_test_timer* _timer
)
{
    d_timer_reset(d_test_timer_as_timer(_timer));

    return;
}

/*
d_test_timer_elapsed
  timer.h's, in nanoseconds.
*/
int64_t
d_test_timer_elapsed(
    const struct dj_test_timer* _timer
)
{
    return (_timer) ? d_timer_elapsed(&_timer->timer) : 0;
}

/*
d_test_timer_elapsed_ms
  The nanoseconds as milliseconds, for a report's "took 1.234 ms".
*/
double
d_test_timer_elapsed_ms(
    const struct dj_test_timer* _timer
)
{
    return (double)d_test_timer_elapsed(_timer) / 1000000.0;
}

/*
d_test_timer_running
  timer.h's, on the wrapped timer.
*/
bool
d_test_timer_running(
    const struct dj_test_timer* _timer
)
{
    return ( (_timer) &&
             (d_timer_running(&_timer->timer)) );
}

/*
d_test_timer_as_timer
  The timer as timer.h sees it.
*/
struct dj_timer*
d_test_timer_as_timer(
    struct dj_test_timer* _timer
)
{
    return (_timer) ? &_timer->timer : NULL;
}
