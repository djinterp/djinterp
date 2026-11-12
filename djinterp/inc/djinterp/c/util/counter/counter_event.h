/*******************************************************************************
* djinterp [c][utility]                                          counter_event.h
*
* The event tie-in for counters: a handler that moves a counter each time an
* event fires. It couples an event core handler (c/event's struct
* dj_event_step) to any counter (counter.h's struct dj_counter_ref), and that
* is all it does: the caller binds the handler wherever handlers are bound.
*   A binding is a struct dj_counter_event -- which counter, which operation,
* by how much. d_counter_event_step hands it off as the event core's
* (function, state) step, as shown below.
*
* ONE BINDING FOR EVERY COUNTER
* -----------------------------
*   The binding acts through the reference's table, so it works the same on a
* plain, an atomic or a guarded counter, and on any other kind that provides a
* struct dj_counter_vtable. Nothing here includes a counter kind, and no counter
* includes this file.
*
* WHAT IS COUNTED IS THE CALLER'S CHOICE
* --------------------------------------
*   One event may move several counters (several handlers on its key) and one
* counter may be moved by several events (one handler on several keys).
*
* THE CALLER OWNS THE BINDING
* ---------------------------
*   The step keeps a pointer to it, so a binding must outlive every place its
* step is bound, or be unbound first.
*
* HOW TO USE
* ----------
*   Aim a binding at any counter, hand off its step, and bind the step where
* handlers are bound:
*
*     struct dj_counter_event hits;
*
*     d_counter_event_init(&hits,
*                          d_counter_atomic_ref(&shared),
*                          D_COUNTER_EVENT_INCREMENT,
*                          1);
*     d_event_dispatcher_bind(&dispatcher,
*                             KEY_HIT,
*                             d_counter_event_step(&hits),
*                             NULL,
*                             &id);
*
* OPTIONS
* -------
*   What firing does, per binding: the operation (D_COUNTER_EVENT_INCREMENT,
* _DECREMENT, _ADD or _RESET) and the amount for increment and decrement. ADD
* takes its amount from the occurrence's payload.
*
* CONFIGURATION
* -------------
*   None: no D_CFG_* knob reaches this header.
*
*
* path:      /inc/djinterp/c/util/counter/counter_event.h
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
         1.  dj_counter_event_op
    2.  The binding
         1.  dj_counter_event
2.  OPERATIONS
    ----------
    1.  Initialization
    2.  The handler
*/

#ifndef DJINTERP_C_UTIL_COUNTER_EVENT_H
#define DJINTERP_C_UTIL_COUNTER_EVENT_H 1

// djinterp
#include "../../djinterp.h"                    // framework root
#include "../../event/event_handler_common.h"  // dj_event_step, D_VERDICT_*
#include "./counter.h"                         // dj_counter_ref
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // int32_t, int64_t


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Operations
//------------------------------------------------------------------------------
// 1.1.1
// dj_counter_event_op
//   enum: what a binding does to its counter when its event fires.
enum dj_counter_event_op
{
    D_COUNTER_EVENT_INCREMENT = 0,  // move up by the binding's amount
    D_COUNTER_EVENT_DECREMENT = 1,  // move down by the binding's amount
    D_COUNTER_EVENT_ADD       = 2,  // move by the int64_t the payload holds
    D_COUNTER_EVENT_RESET     = 3   // return to the start
};

// 1.2    The binding
//------------------------------------------------------------------------------
// 1.2.1
// dj_counter_event
//   struct: one counter, one operation. A binding whose target is no counter
// does nothing when it fires.
struct dj_counter_event
{
    struct dj_counter_ref    target;  // the counter it moves
    enum dj_counter_event_op op;      // what firing does
    int64_t                  amount;  // for INCREMENT and DECREMENT
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================
// A binding is set once and then fired many times; nothing here allocates.


// 2.1    Initialization
//------------------------------------------------------------------------------
/**
 * @brief Aims a binding at a counter of any kind.
 *
 * @param[out] _binding  the binding to set; `NULL` is ignored.
 * @param[in]  _target   the counter it moves, from d_counter_ref,
 *                       d_counter_atomic_ref or d_counter_mutex_ref; a
 *                       reference to no counter makes a binding that moves
 *                       nothing.
 * @param[in]  _op       what firing does.
 * @param[in]  _amount   the step for D_COUNTER_EVENT_INCREMENT and
 *                       D_COUNTER_EVENT_DECREMENT; ignored by the others.
 */
void
d_counter_event_init(
    struct dj_counter_event* _binding,
    struct dj_counter_ref    _target,
    enum dj_counter_event_op _op,
    int64_t                  _amount
);

// 2.2    The handler
//------------------------------------------------------------------------------
/**
 * @brief Applies a binding's operation once; an fn_event_step.
 *
 * @param[in]     _payload  the occurrence's payload: for D_COUNTER_EVENT_ADD,
 *                          a pointer to the int64_t to add, and a `NULL`
 *                          payload adds nothing; ignored by the other
 *                          operations.
 * @param[in,out] _binding  the struct dj_counter_event; `NULL` does nothing.
 * @return D_VERDICT_PASS always: counting an event never stops it reaching
 *         the handlers bound after this one.
 */
int32_t
d_counter_event_apply(
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
d_counter_event_step(
    struct dj_counter_event* _binding
);


D_EXTERN_C_END


#endif  // DJINTERP_C_UTIL_COUNTER_EVENT_H
