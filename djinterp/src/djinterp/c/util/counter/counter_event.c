/*******************************************************************************
* djinterp [c][utility]                                          counter_event.c
*
* The counter tie-in's handler: one switch over the operation, applied
* through the target's table, whatever kind of counter it is.
*
*
* path:      /src/djinterp/c/util/counter/counter_event.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "../../../../../inc/djinterp/c/util/counter/counter_event.h"  // corresponding header


/*
d_counter_event_init
  Every field is written, so a binding needs no zeroing first.
*/
void
d_counter_event_init(
    struct dj_counter_event* _binding,
    struct dj_counter_ref    _target,
    enum dj_counter_event_op _op,
    int64_t                  _amount
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
d_counter_event_apply
  The operation picks the amount and its direction; the target's table picks
the counter functions. A cut step is still a counted event, so the verdict is
pass whatever the counter reports: a full counter does not silence the
handlers bound after it. An operation outside the enum, which a binding
written by hand could hold, does nothing.
*/
int32_t
d_counter_event_apply(
    void* _payload,
    void* _binding
)
{
    const struct dj_counter_event* binding =
        (const struct dj_counter_event*)_binding;

    // nothing bound, nothing to do
    if (!binding)
    {
        return D_VERDICT_PASS;
    }

    // one of counter.h's d_counter_ref_* functions per operation
    switch (binding->op)
    {
        case D_COUNTER_EVENT_INCREMENT:
        {
            (void)d_counter_ref_increment(binding->target,
                                          binding->amount);

            break;
        }

        case D_COUNTER_EVENT_DECREMENT:
        {
            (void)d_counter_ref_decrement(binding->target,
                                          binding->amount);

            break;
        }

        case D_COUNTER_EVENT_ADD:
        {
            // the amount the occurrence carries; none adds nothing
            const int64_t amount = (_payload)
                                       ? *(const int64_t*)_payload
                                       : 0;

            (void)d_counter_ref_increment(binding->target,
                                          amount);

            break;
        }

        case D_COUNTER_EVENT_RESET:
        {
            d_counter_ref_reset(binding->target);

            break;
        }

        default:
        {
            // an operation this build does not know moves nothing
            break;
        }
    }

    return D_VERDICT_PASS;
}

/*
d_counter_event_step
  The binding as the event core's (function, state) pair; binding it is the
caller's business.
*/
struct dj_event_step
d_counter_event_step(
    struct dj_counter_event* _binding
)
{
    return d_event_step_make(d_counter_event_apply,
                             _binding);
}
