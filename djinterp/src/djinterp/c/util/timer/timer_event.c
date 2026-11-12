/*******************************************************************************
* djinterp [c][utility]                                            timer_event.c
*
* The timer tie-in's handler: one switch over the operation, applied through
* the target's table, whatever kind of timer it is.
*
*
* path:      /src/djinterp/c/util/timer/timer_event.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "../../../../../inc/djinterp/c/util/timer/timer_event.h"  // corresponding header


/*
d_timer_event_init
  Every field is written, so a binding needs no zeroing first.
*/
void
d_timer_event_init(
    struct dj_timer_event* _binding,
    struct dj_timer_ref    _target,
    enum dj_timer_event_op _op,
    int64_t                _amount
)
{
    // parameter validation
    if (!_binding)
    {
        return;
    }

    _binding->target = _target;
    _binding->op     = _op;
    _binding->amount = _amount;

    return;
}

/*
d_timer_event_apply
  ADD takes its amount from the occurrence; everything else from the binding.
A start on a running timer, or a stop on a stopped one, is a no-op here, as
it is in timer.h: an event that arrives twice does not corrupt the time.
*/
int32_t
d_timer_event_apply(
    void* _payload,
    void* _binding
)
{
    const struct dj_timer_event* binding =
        (const struct dj_timer_event*)_binding;

    // nothing bound, nothing to do
    if (!binding)
    {
        return D_VERDICT_PASS;
    }

    // one of timer.h's d_timer_ref_* functions per operation
    switch (binding->op)
    {
        case D_TIMER_EVENT_START:
        {
            (void)d_timer_ref_start(binding->target);

            break;
        }

        case D_TIMER_EVENT_STOP:
        {
            (void)d_timer_ref_stop(binding->target);

            break;
        }

        case D_TIMER_EVENT_INCREMENT:
        {
            (void)d_timer_ref_increment(binding->target,
                                        binding->amount);

            break;
        }

        case D_TIMER_EVENT_DECREMENT:
        {
            (void)d_timer_ref_decrement(binding->target,
                                        binding->amount);

            break;
        }

        case D_TIMER_EVENT_ADD:
        {
            // the nanoseconds the occurrence carries; none adds nothing
            const int64_t amount = (_payload)
                                       ? *(const int64_t*)_payload
                                       : 0;

            (void)d_timer_ref_increment(binding->target,
                                        amount);

            break;
        }

        case D_TIMER_EVENT_RESET:
        {
            d_timer_ref_reset(binding->target);

            break;
        }

        default:
        {
            // an operation this build does not know does nothing
            break;
        }
    }

    return D_VERDICT_PASS;
}

/*
d_timer_event_step
  The binding as the event core's (function, state) pair; binding it is the
caller's business.
*/
struct dj_event_step
d_timer_event_step(
    struct dj_timer_event* _binding
)
{
    return d_event_step_make(d_timer_event_apply,
                             _binding);
}
