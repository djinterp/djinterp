/*******************************************************************************
* djinterp [c][test]                                        test_counter_event.c
*
* DTest's counter tie-in: the tally binding's handler records through
* test_counter.h, and a test counter's binding is counter_event.h's.
*
*
* path:      /src/djinterp/c/modules/test/test_counter_event.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.10
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "../../../../../inc/djinterp/c/modules/test/test_counter_event.h"  // corresponding header


/*
d_test_tally_event_init
  Both fields are written, so a binding needs no zeroing first.
*/
void
d_test_tally_event_init(
    struct dj_test_tally_event* _binding,
    struct dj_test_tally*       _tally,
    dj_test_key                 _key
)
{
    // parameter validation
    if (!_binding)
    {
        return;
    }

    _binding->tally = _tally;
    _binding->key   = _key;

    return;
}

/*
d_test_tally_event_apply
  One occurrence, recorded by the tally's own rules: the counter's step, or
past the limit, the tally's policy.
*/
int32_t
d_test_tally_event_apply(
    void* _payload,
    void* _binding
)
{
    const struct dj_test_tally_event* binding =
        (const struct dj_test_tally_event*)_binding;

    (void)_payload;

    // a tally to record into
    if (binding)
    {
        (void)d_test_tally_record(binding->tally,
                                  binding->key);
    }

    return D_VERDICT_PASS;
}

/*
d_test_tally_event_step
  The binding as the event core's (function, state) pair; binding it is the
caller's business.
*/
struct dj_event_step
d_test_tally_event_step(
    struct dj_test_tally_event* _binding
)
{
    return d_event_step_make(d_test_tally_event_apply,
                             _binding);
}

/*
d_test_counter_event_init
  An increment by the counter's step, through the plain counter's reference.
*/
void
d_test_counter_event_init(
    struct dj_counter_event* _binding,
    struct dj_test_counter*  _counter
)
{
    d_counter_event_init(_binding,
                         d_counter_ref(d_test_counter_as_counter(_counter)),
                         D_COUNTER_EVENT_INCREMENT,
                         d_test_counter_step(_counter));

    return;
}
