/*******************************************************************************
* djinterp [c][utility]                                            timer_event.h
*
* The event tie-in for timers: a handler that starts, stops, moves or resets a
* timer each time an event fires. It couples an event core handler (c/event's
* struct dj_event_step) to any timer (timer.h's struct dj_timer_ref), and that
* is all it does: the caller binds the handler wherever handlers are bound.
* counter_event.h's design, for time.
*   A binding is a struct dj_timer_event -- which timer, which operation, by
* how much. d_timer_event_step hands it off as the event core's (function,
* state) step. Timing a span between two events is two bindings on one
* timer, start and stop, each bound to its own event, as shown below.
*
* ONE BINDING FOR EVERY TIMER
* ---------------------------
*   The binding acts through the reference's table, so it works the same on a
* plain, an atomic or a guarded timer, and on any other kind that provides a
* struct dj_timer_vtable. Nothing here includes a timer kind, and no timer
* includes this file.
*
* THE CALLER OWNS THE BINDING
* ---------------------------
*   The step keeps a pointer to it, so a binding must outlive every place its
* step is bound, or be unbound first.
*
* HOW TO USE
* ----------
*   Timing the span between two events is a START and a STOP binding on one
* timer, each bound to its own key:
*
*     struct dj_timer_event open;
*     struct dj_timer_event close;
*
*     d_timer_event_init(&open,
*                        d_timer_ref(&span),
*                        D_TIMER_EVENT_START,
*                        0);
*     d_timer_event_init(&close,
*                        d_timer_ref(&span),
*                        D_TIMER_EVENT_STOP,
*                        0);
*     d_event_dispatcher_bind(&d, KEY_OPEN, d_timer_event_step(&open),
*                             NULL, &open_id);
*     d_event_dispatcher_bind(&d, KEY_CLOSE, d_timer_event_step(&close),
*                             NULL, &close_id);
*
* OPTIONS
* -------
*   What firing does, per binding: the operation (D_TIMER_EVENT_START, _STOP,
* _INCREMENT, _DECREMENT, _ADD or _RESET) and the nanoseconds for increment and
* decrement. ADD takes its nanoseconds from the payload.
*
* CONFIGURATION
* -------------
*   None: no D_CFG_* knob reaches this header.
*
*
* path:      /inc/djinterp/c/util/timer/timer_event.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.10
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Operations
         1.  dj_timer_event_op
    2.  The binding
         1.  dj_timer_event
2.  OPERATIONS
    ----------
    1.  Initialization
    2.  The handler
*/

#ifndef DJINTERP_C_UTIL_TIMER_EVENT_H
#define DJINTERP_C_UTIL_TIMER_EVENT_H 1

// djinterp
#include "../../djinterp.h"                    // framework root
#include "../../event/event_handler_common.h"  // dj_event_step, D_VERDICT_*
#include "./timer.h"                           // dj_timer_ref
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // int32_t, int64_t


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Operations
//------------------------------------------------------------------------------
// 1.1.1
// dj_timer_event_op
//   enum: what a binding does to its timer when its event fires.
enum dj_timer_event_op
{
    D_TIMER_EVENT_START     = 0,  // open an interval
    D_TIMER_EVENT_STOP      = 1,  // close the open interval
    D_TIMER_EVENT_INCREMENT = 2,  // add the binding's amount, in ns
    D_TIMER_EVENT_DECREMENT = 3,  // remove the binding's amount, in ns
    D_TIMER_EVENT_ADD       = 4,  // add the int64_t ns the payload holds
    D_TIMER_EVENT_RESET     = 5   // stop, and return to zero
};

// 1.2    The binding
//------------------------------------------------------------------------------
// 1.2.1
// dj_timer_event
//   struct: one timer, one operation. A binding whose target is no timer does
// nothing when it fires.
struct dj_timer_event
{
    struct dj_timer_ref    target;  // the timer it acts on
    enum dj_timer_event_op op;      // what firing does
    int64_t                amount;  // for INCREMENT and DECREMENT, in ns
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================
// A binding is set once and then fired many times; nothing here allocates.


// 2.1    Initialization
//------------------------------------------------------------------------------
/**
 * @brief Aims a binding at a timer of any kind.
 *
 * @param[out] _binding  the binding to set; `NULL` is ignored.
 * @param[in]  _target   the timer it acts on, from d_timer_ref,
 *                       d_timer_atomic_ref or d_timer_mutex_ref; a reference
 *                       to no timer makes a binding that does nothing.
 * @param[in]  _op       what firing does.
 * @param[in]  _amount   the nanoseconds for D_TIMER_EVENT_INCREMENT and
 *                       D_TIMER_EVENT_DECREMENT; ignored by the others.
 */
void
d_timer_event_init(
    struct dj_timer_event* _binding,
    struct dj_timer_ref    _target,
    enum dj_timer_event_op _op,
    int64_t                _amount
);

// 2.2    The handler
//------------------------------------------------------------------------------
/**
 * @brief Applies a binding's operation once; an fn_event_step.
 *
 * @param[in]     _payload  the occurrence's payload: for D_TIMER_EVENT_ADD,
 *                          a pointer to the int64_t nanoseconds to add, and a
 *                          `NULL` payload adds nothing; ignored by the other
 *                          operations.
 * @param[in,out] _binding  the struct dj_timer_event; `NULL` does nothing.
 * @return D_VERDICT_PASS always: timing an event never stops it reaching the
 *         handlers bound after this one.
 */
int32_t
d_timer_event_apply(
    void* _payload,
    void* _binding
);

/**
 * @brief Hands a binding off as the event core's (function, state) step.
 *
 * @param[in] _binding  the binding the step applies; the caller's.
 * @return the step, to bind as any handler is bound.
 */
struct dj_event_step
d_timer_event_step(
    struct dj_timer_event* _binding
);


D_EXTERN_C_END


#endif  // DJINTERP_C_UTIL_TIMER_EVENT_H
