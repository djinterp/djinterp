/*******************************************************************************
* djinterp [c][test]                                        test_counter_event.h
*
* The event tie-in for DTest's counters: handlers that record an occurrence
* into a tally key, or move a test counter by its step, each time an event
* fires. It couples the event core's handler (c/event's struct dj_event_step)
* to test_counter.h and does nothing else: the caller binds the handlers
* wherever handlers are bound. Neither test_counter.h nor DTest's runners
* include this file.
*   The tally key and the event key are separate choices; a suite that likes
* may use the one as the other.
*
* ONE TEST COUNTER
* ----------------
*   A test counter is a counter, so its handler is
* c/util/counter/counter_event.h's, aimed at it by d_test_counter_event_init
* with the counter's step.
*
* THE CALLER OWNS THE BINDING
* ---------------------------
*   A step keeps a pointer to its binding, and the binding to its tally or
* counter, so both must outlive every place the step is bound, or it is unbound
* first.
*
* HOW TO USE
* ----------
*   Count a tally key per occurrence of an event:
*
*     struct dj_test_tally_event retries;
*
*     d_test_tally_event_init(&retries,
*                             &tally,
*                             MY_RETRIES);
*     d_event_dispatcher_bind(&dispatcher,
*                             KEY_RETRY,
*                             d_test_tally_event_step(&retries),
*                             NULL,
*                             &id);
*
*   For one test counter, d_test_counter_event_init aims counter_event.h's
* binding at it by its step; hand that off with d_counter_event_step.
*
* OPTIONS
* -------
*   Which tally and key a binding records; a test counter's binding takes the
* counter's step.
*
* CONFIGURATION
* -------------
*   None: no D_CFG_* knob reaches this header.
*
*
* path:      /inc/djinterp/c/modules/test/test_counter_event.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.10
*                                                            revised: 2026.11.10
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  The tally binding
         1.  dj_test_tally_event
2.  OPERATIONS
    ----------
    1.  A tally key
    2.  A test counter
*/

#ifndef DJINTERP_C_TEST_COUNTER_EVENT_H
#define DJINTERP_C_TEST_COUNTER_EVENT_H 1

// djinterp
#include "../../djinterp.h"                       // framework root
#include "../../event/event_handler_common.h"     // dj_event_step
#include "../../util/counter/counter_event.h"     // dj_counter_event
#include "./test_counter.h"                       // dj_test_tally
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // int32_t


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    The tally binding
//------------------------------------------------------------------------------
// 1.1.1
// dj_test_tally_event
//   struct: one tally, one key: each firing records one occurrence of the
// key. A binding with no tally does nothing when it fires.
struct dj_test_tally_event
{
    struct dj_test_tally* tally;  // the tally recorded into
    dj_test_key           key;    // the key each firing records
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================
// A binding is set once and then fired many times; nothing here allocates.


// 2.1    A tally key
//------------------------------------------------------------------------------
/**
 * @brief Aims a binding at one key of a tally.
 *
 * @param[out] _binding  the binding to set; `NULL` is ignored.
 * @param[in]  _tally    the tally; may be `NULL`, and then firing records
 *                       nothing.
 * @param[in]  _key      the key each firing records; it need not be tracked
 *                       yet, since recording tracks it.
 */
void
d_test_tally_event_init(
    struct dj_test_tally_event* _binding,
    struct dj_test_tally*       _tally,
    dj_test_key                 _key
);

/**
 * @brief Records one occurrence of the binding's key; an fn_event_step.
 *
 * @note A tally that refuses the key (full, under D_TEST_TALLY_FULL_REFUSE)
 *       loses the occurrence, as d_test_tally_record does.
 *
 * @param[in]     _payload  the occurrence's payload; ignored.
 * @param[in,out] _binding  the struct dj_test_tally_event; `NULL` does
 *                          nothing.
 * @return D_VERDICT_PASS always: counting an event never stops it reaching
 *         the handlers bound after this one.
 */
int32_t
d_test_tally_event_apply(
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
d_test_tally_event_step(
    struct dj_test_tally_event* _binding
);

// 2.2    A test counter
//------------------------------------------------------------------------------
/**
 * @brief Aims a counter_event.h binding at a test counter: each firing moves
 *        it by its step.
 *
 * @note The step is read now; a counter set up again with another step needs
 *       its binding set again. Hand the binding off with
 *       d_counter_event_step.
 *
 * @param[out] _binding  the binding to set; `NULL` is ignored.
 * @param[in]  _counter  the test counter; may be `NULL`, and then firing
 *                       moves nothing.
 */
void
d_test_counter_event_init(
    struct dj_counter_event* _binding,
    struct dj_test_counter*  _counter
);


D_EXTERN_C_END


#endif  // DJINTERP_C_TEST_COUNTER_EVENT_H
