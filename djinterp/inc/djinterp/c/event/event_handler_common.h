/*******************************************************************************
* djinterp [c]                                            event_handler_common.h
*
* The handler step -- the shared C core (tier 0):
*   A handler is the single primitive of the event layer: given the ambient
* state S and an occurrence's payload A_e, it yields a verdict in
* P = {pass, consume}. C has no closures, so the state that C++ captures is
* carried explicitly as a void* alongside the function pointer; the pair is
* the step. This header declares the step, the sequencing monoid (seq with
* unit skip and left zero consume), and the two adapters that admit a plain
* void-returning callback as an always-pass handler.
*
*   Every combinator here composes over CALLER-PROVIDED storage. Nothing in
* this file allocates, so the whole monoid is available at the portability
* floor and before the container substrate exists.
*
* FORMAL CORRESPONDENCE ("Definition of an Event"):
*   handler    h : S x A_e -> S x P  -- struct d_event_step (fn + state)
*   sequencing h1 ; h2               -- d_event_step_seq
*   unit       skip(s,a) = (s,pass)  -- d_event_step_skip
*   left zero  consume               -- a step returning D_VERDICT_CONSUME
*                                       short-circuits the remainder
*
* MONOID LAWS (each has a conformance test; see the note in section V):
*   associativity  seq(seq(a,b),c)  == seq(a,seq(b,c))
*   left unit      seq(skip,a)      == a
*   right unit     seq(a,skip)      == a
*   left zero      seq(consume,a)   == consume, and a is NOT invoked
*
* PORTABLE ACROSS:
*   C99, C11, C17, C23  /  C++11, C++14, C++17, C++20, C++23, C++26
*
*
* path:      /inc/djinterp/c/event/event_handler_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.30
*                                                            revised: 2026.10.03
*******************************************************************************/

#ifndef DJINTERP_C_EVENT_EVENT_HANDLER_COMMON_H
#define DJINTERP_C_EVENT_EVENT_HANDLER_COMMON_H 1

// std
#include <stddef.h>
// djinterp
#include "../djinterp.h"
#include "../../config/core/event/cfg_event_handler.h"
#include "./event_common.h"
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // int32_t

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


D_EXTERN_C_BEGIN


///////////////////////////////////////////////////////////////////////////////
///        I.    THE STEP                                                   ///
///////////////////////////////////////////////////////////////////////////////

// fn_event_step
//   function pointer: the erased handler. `_payload` points at the packed
// payload block A_e; `_state` is the ambient state S, borrowed and opaque to
// the core.
//   RETURNS the pinned verdict code (D_VERDICT_PASS or D_VERDICT_CONSUME) as
// int32_t rather than as `enum d_verdict`, because C's `enum d_verdict` and
// C++'s `enum class verdict` are distinct types: a shared function-pointer
// type must name a type both languages can spell identically, and calling
// through a converted function pointer is undefined behaviour.
typedef int32_t (*fn_event_step)(void* _payload,
                                 void* _state);

// d_event_step
//   struct: one handler -- the pair (function, ambient state). A step with a
// NULL `fn` is the unit and always yields pass, which makes a zeroed step
// valid rather than a trap.
struct d_event_step
{
    fn_event_step fn;
    void*         state;
};

// d_event_step_make
//   function: builds a step from a function and its borrowed state.
D_STATIC_INLINE struct d_event_step
d_event_step_make(fn_event_step _fn,
                  void*         _state)
{
    struct d_event_step step;

    step.fn    = _fn;
    step.state = _state;

    return step;
}

// d_event_step_skip
//   function: the monoid unit. Ignores the payload and always yields pass.
// Folding a word is invariant under inserting or removing skip -- which is
// the algebraic reason masking a handler is well defined.
D_STATIC_INLINE struct d_event_step
d_event_step_skip(void)
{
    return d_event_step_make(NULL, NULL);
}

// d_event_step_is_skip
//   function: true if the step is the unit.
D_STATIC_INLINE bool
d_event_step_is_skip(const struct d_event_step* _step)
{
    return ( (!_step) ||
             (!_step->fn) );
}

// d_event_step_invoke
//   function: invokes a step over a payload block, normalizing the unit to
// pass. This is the ONE place the core calls a handler; dispatch, the fused
// word, and seq all route through it, so instrumentation and the unit
// convention have a single definition.
D_STATIC_INLINE int32_t
d_event_step_invoke(const struct d_event_step* _step,
                    void*                      _payload)
{
    if (d_event_step_is_skip(_step))
    {
        return (int32_t)D_VERDICT_PASS;
    }

    return _step->fn(_payload, _step->state);
}


///////////////////////////////////////////////////////////////////////////////
///        II.   THE VOID-CALLBACK ADAPTER                                  ///
///////////////////////////////////////////////////////////////////////////////
// The C++ face accepts a void-returning callable as an always-pass handler,
// because the monoid unit yields pass. The C equivalent is an adapter over
// fn_callback -- which is also the migration path for every handler written
// against the pre-core `struct d_event_listener`, whose callback was exactly
// `void (*)(void*)` with no verdict.

#if (D_INTERNAL_EVENT_HANDLER_ADAPTER == 1)

// d_event_callback_state
//   struct: the state block for a step adapted from a plain callback. The
// caller owns this object and must keep it alive for as long as the step is
// bound; the table can be asked to free it on unbind (see the state_free
// argument to d_event_table_bind).
struct d_event_callback_state
{
    fn_callback fn;
    void*       context;
};

// d_event_callback_step_fn
//   function: the fn_event_step that drives a d_event_callback_state. Invokes
// the callback with the state's own context -- NOT with the payload -- which
// reproduces the pre-core dispatch exactly: `listener->fn(event->context)`.
//   The payload is therefore ignored by this adapter. A handler that needs
// the payload must be written against fn_event_step directly.
int32_t d_event_callback_step_fn(void* _payload,
                                 void* _state);

// d_event_callback_state_init
//   function: initializes a callback state block in caller-provided storage.
D_STATIC_INLINE void
d_event_callback_state_init(struct d_event_callback_state* _state,
                            fn_callback                    _fn,
                            void*                          _context)
{
    if (!_state)
    {
        return;
    }

    _state->fn      = _fn;
    _state->context = _context;

    return;
}

// d_event_step_from_callback
//   function: builds an always-pass step over an initialized callback state.
D_STATIC_INLINE struct d_event_step
d_event_step_from_callback(struct d_event_callback_state* _state)
{
    if ( (!_state) ||
         (!_state->fn) )
    {
        return d_event_step_skip();
    }

    return d_event_step_make(&d_event_callback_step_fn, (void*)_state);
}


#endif  // D_INTERNAL_EVENT_HANDLER_ADAPTER


///////////////////////////////////////////////////////////////////////////////
///        III.  SEQUENCING (the monoid operation)                          ///
///////////////////////////////////////////////////////////////////////////////
// seq runs the first step; if it consumes, the second is NOT invoked and
// consume is returned; otherwise the second runs and its verdict is the
// result. Both stages see the same payload block, so a handler that mutates
// the payload is visible to its successors -- deliberately, since that is how
// the state S threads through a word.

#if (D_INTERNAL_EVENT_HANDLER_SEQ == 1)

// d_event_seq_state
//   struct: the state block for a sequenced pair. Composition in C is
// explicit storage: the caller supplies one of these per seq node, so
// building a word of n steps by repeated seq costs n-1 caller-owned nodes and
// no allocation.
struct d_event_seq_state
{
    struct d_event_step first;
    struct d_event_step second;
};

// d_event_seq_step_fn
//   function: the fn_event_step that drives a d_event_seq_state.
int32_t d_event_seq_step_fn(void* _payload,
                            void* _state);

// d_event_step_seq
//   function: sequences two steps into one (h1 ; h2), realizing the monoid
// operation over caller-provided node storage. With skip as unit and consume
// as left zero, repeated application folds a whole word into a single step.
//   Returns the unit if `_node` is NULL, so a storage failure degrades to a
// well-defined no-op rather than a trap.
D_STATIC_INLINE struct d_event_step
d_event_step_seq(struct d_event_seq_state* _node,
                 struct d_event_step       _first,
                 struct d_event_step       _second)
{
    if (!_node)
    {
        return d_event_step_skip();
    }

    _node->first  = _first;
    _node->second = _second;

    return d_event_step_make(&d_event_seq_step_fn, (void*)_node);
}


#endif  // D_INTERNAL_EVENT_HANDLER_SEQ


///////////////////////////////////////////////////////////////////////////////
///        IV.   FOLDING A WORD                                             ///
///////////////////////////////////////////////////////////////////////////////
// The inner fold, factored out of the registry so that the fused path and the
// erased path provably share it. Two implementations of one fold is exactly
// the divergence the fused/erased coherence law exists to rule out, so there
// is one.

// d_event_step_fold
//   function: folds seq over a contiguous array of steps against one payload
// block, stopping at the first consume (the left zero).
//   `_invoked_out` may be NULL; when supplied it receives the number of steps
// actually invoked -- the (N, +) component of the enriched dispatch.
// returns: the pinned verdict code of the folded word.
int32_t d_event_step_fold(const struct d_event_step* _steps,
                          size_t                     _count,
                          void*                      _payload,
                          size_t*                    _invoked_out);


///////////////////////////////////////////////////////////////////////////////
///        V.    LAYOUT ASSERTIONS                                          ///
///////////////////////////////////////////////////////////////////////////////
// The monoid laws are not asserted here -- they are universally quantified and
// therefore sampled, not checked. They belong in the conformance suite
// (property-based, over a shared generator, per AGENT_README.md section 6
// step 7), including the stated NON-property: seq(consume, a) must leave `a`
// UNINVOKED, so a test that only compares verdicts would pass against a wrong
// implementation.

#if (D_INTERNAL_EVENT_ASSERT_SIZES == 1)

    D_STATIC_ASSERT(sizeof(struct d_event_step) == 16,
                    "d_event_step layout drift");
#if (D_INTERNAL_EVENT_HANDLER_SEQ == 1)
    D_STATIC_ASSERT(sizeof(struct d_event_seq_state) == 32,
                    "d_event_seq_state layout drift");
#endif
#if (D_INTERNAL_EVENT_HANDLER_ADAPTER == 1)
    D_STATIC_ASSERT(sizeof(struct d_event_callback_state) == 16,
                    "d_event_callback_state layout drift");
#endif

#endif  // D_INTERNAL_EVENT_ASSERT_SIZES

#if (D_INTERNAL_EVENT_ASSERT_LAYOUT == 1)

D_STATIC_ASSERT(offsetof(struct d_event_step, fn) == 0,
                "d_event_step field drift");
#if (D_INTERNAL_EVENT_HANDLER_SEQ == 1)
D_STATIC_ASSERT(offsetof(struct d_event_seq_state, first) == 0,
                "d_event_seq_state field drift");
#endif

#endif  // D_INTERNAL_EVENT_ASSERT_LAYOUT


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_EVENT_EVENT_HANDLER_COMMON_H
