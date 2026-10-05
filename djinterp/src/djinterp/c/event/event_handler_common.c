/*******************************************************************************
* djinterp [c]                                            event_handler_common.c
*
* The handler step -- tier 0 definitions.
*   Defines the step functions behind the void-callback adapter and the
* sequencing node, and the one fold of a word that dispatch and the fused
* word both use.
*
* path:      /src/djinterp/c/event/event_handler_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.10.03
*******************************************************************************/
#include "../../../../inc/djinterp/c/event/event_handler_common.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // int32_t


#if (D_INTERNAL_EVENT_HANDLER_ADAPTER == 1)

/*
d_event_callback_step_fn
  Calls the callback with the state's own context and never with the payload,
which is exactly the pre-core `listener->fn(event->context)`. A missing state
or callback behaves as the unit: it passes without a call.
*/
int32_t
d_event_callback_step_fn(
    void* _payload,
    void* _state
)
{
    const struct d_event_callback_state* const state = _state;

    (void)_payload;

    // no state or no callback: the unit
    if ( (!state) ||
         (!state->fn) )
    {
        return (int32_t)D_VERDICT_PASS;
    }

    state->fn(state->context);

    return (int32_t)D_VERDICT_PASS;
}

#endif  // D_INTERNAL_EVENT_HANDLER_ADAPTER


#if (D_INTERNAL_EVENT_HANDLER_SEQ == 1)

/*
d_event_seq_step_fn
  Runs the first stage and returns consume without touching the second when
the first consumes: consume is the left zero, and the conformance tests check
that the second stage is not merely outvoted but never called. The result is
normalized to a pinned code, so a handler returning some other value cannot
leak it through a seq node.
*/
int32_t
d_event_seq_step_fn(
    void* _payload,
    void* _state
)
{
    const struct d_event_seq_state* const node = _state;

    // a missing node is the unit
    if (!node)
    {
        return (int32_t)D_VERDICT_PASS;
    }

    // consume is the left zero: the second stage is never invoked
    if (D_VERDICT_IS_CONSUMED(d_event_step_invoke(&node->first, _payload)))
    {
        return (int32_t)D_VERDICT_CONSUME;
    }

    return D_VERDICT_IS_CONSUMED(d_event_step_invoke(&node->second, _payload))
        ? (int32_t)D_VERDICT_CONSUME
        : (int32_t)D_VERDICT_PASS;
}

#endif  // D_INTERNAL_EVENT_HANDLER_SEQ


/*
d_event_step_fold
  A skip letter is neither called nor counted, so the invocation count, like
the verdict, is unchanged by inserting or removing the unit. The count is
written even when _steps is NULL, so a caller never reads a stale value.
*/
int32_t
d_event_step_fold(
    const struct d_event_step* _steps,
    size_t                     _count,
    void*                      _payload,
    size_t*                    _invoked_out
)
{
    size_t  invoked = 0u;
    int32_t verdict = (int32_t)D_VERDICT_PASS;

    // fold seq over the word, stopping at the first consume
    for (size_t i = 0u; (_steps != NULL) && (i < _count); ++i)
    {
        // the unit passes without being invoked
        if (d_event_step_is_skip(&_steps[i]))
        {
            continue;
        }

        ++invoked;

        // consume is the left zero: the remainder is not invoked
        if (D_VERDICT_IS_CONSUMED(d_event_step_invoke(&_steps[i], _payload)))
        {
            verdict = (int32_t)D_VERDICT_CONSUME;

            break;
        }
    }

    // report the (N, +) component of the enriched result
    if (_invoked_out)
    {
        *_invoked_out = invoked;
    }

    return verdict;
}
