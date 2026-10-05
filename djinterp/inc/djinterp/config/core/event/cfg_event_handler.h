/*******************************************************************************
* djinterp [config]                                          cfg_event_handler.h
*
*   Configuration for the handler step and the sequencing monoid: which
* combinators compile, which handler shapes are accepted, and whether the
* erased shim guards against an escaping C++ exception.
*
*   targets:  core/event/event_handler_common.h, event_handler.hpp
*             ->  D_INTERNAL_EVENT_HANDLER_ADAPTER,
*                 D_INTERNAL_EVENT_HANDLER_SEQ,
*                 D_INTERNAL_EVENT_HANDLER_ALLOW_VOID,
*                 D_INTERNAL_EVENT_HANDLER_GUARD
*   requires: cfg_common.h; cfg_event_common.h
*
*
* path:      /inc/djinterp/config/core/event/cfg_event_handler.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.31
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CORE_EVENT_CFG_EVENT_HANDLER_H
#define DJINTERP_CONFIG_CORE_EVENT_CFG_EVENT_HANDLER_H 1

// djinterp
// (0) root first, then the subframework root (aggregate and presets).
#include "../../cfg_common.h"
#include "cfg_event_common.h"


// ===========================================================================
//  1.  KNOBS
// ===========================================================================

// D_CFG_EVENT_HANDLER_CALLBACK_ADAPTER
//   brief: 1 compiles the adapter that presents a plain fn_callback as an
// always-pass handler. This is the migration path for every handler written
// against the retired d_event_listener, whose callback returned nothing.
// Defaults ON; a codebase with no pre-core handlers left can drop it.
#ifndef D_CFG_EVENT_HANDLER_CALLBACK_ADAPTER
#   ifdef D_CFG_EVENT_ALL
#       define D_CFG_EVENT_HANDLER_CALLBACK_ADAPTER D_CFG_EVENT_ALL
#   else
#       define D_CFG_EVENT_HANDLER_CALLBACK_ADAPTER 1
#   endif
#endif

// D_CFG_EVENT_HANDLER_SEQ
//   brief: 1 compiles the explicit sequencing combinator (d_event_step_seq
// and the C++ seq). Dispatch folds a word regardless -- the fold is the
// monoid operation and is never optional -- so turning this off removes only
// the ability to name a composed pair as a single step.
#ifndef D_CFG_EVENT_HANDLER_SEQ
#   ifdef D_CFG_EVENT_ALL
#       define D_CFG_EVENT_HANDLER_SEQ D_CFG_EVENT_ALL
#   else
#       define D_CFG_EVENT_HANDLER_SEQ 1
#   endif
#endif

// D_CFG_EVENT_HANDLER_ALLOW_VOID
//   brief: 1 accepts a void-returning callable as an always-pass handler,
// which is sound because the monoid unit yields pass. 0 requires every C++
// handler to state its verdict, turning an accidental fallthrough into a
// compile error. Does not affect C, where the return type is spelled out.
#ifndef D_CFG_EVENT_HANDLER_ALLOW_VOID
#   define D_CFG_EVENT_HANDLER_ALLOW_VOID 1
#endif

// D_CFG_EVENT_HANDLER_GUARD_EXCEPTIONS
//   brief: 1 wraps the C++ erased shim in a catch-all that converts an
// escaping exception into a verdict rather than letting it unwind through the
// C fold. Defaults OFF because the guard costs an EH region on every handler
// invocation and most builds either use noexcept handlers or want the throw.
//   Turn it on when C++ handlers that may throw are dispatched from a C
// caller: unwinding through a C frame is not something the core can make
// well-defined for you.
#ifndef D_CFG_EVENT_HANDLER_GUARD_EXCEPTIONS
#   define D_CFG_EVENT_HANDLER_GUARD_EXCEPTIONS 0
#endif

// D_CFG_EVENT_HANDLER_GUARD_VERDICT
//   brief: the verdict a guarded shim reports when a handler throws --
// 0 (pass, the default) lets the remaining word run, 1 (consume) stops it.
// Only consulted when D_CFG_EVENT_HANDLER_GUARD_EXCEPTIONS is 1.
#ifndef D_CFG_EVENT_HANDLER_GUARD_VERDICT
#   define D_CFG_EVENT_HANDLER_GUARD_VERDICT 0
#endif


// ===========================================================================
//  2.  VALIDATION
// ===========================================================================

#if !D_CFG_IS_ON(D_CFG_EVENT_HANDLER_CALLBACK_ADAPTER) &&                     \
    !D_CFG_IS_OFF(D_CFG_EVENT_HANDLER_CALLBACK_ADAPTER)
#   error "D_CFG_EVENT_HANDLER_CALLBACK_ADAPTER must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_EVENT_HANDLER_SEQ) &&                                  \
    !D_CFG_IS_OFF(D_CFG_EVENT_HANDLER_SEQ)
#   error "D_CFG_EVENT_HANDLER_SEQ must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_EVENT_HANDLER_ALLOW_VOID) &&                           \
    !D_CFG_IS_OFF(D_CFG_EVENT_HANDLER_ALLOW_VOID)
#   error "D_CFG_EVENT_HANDLER_ALLOW_VOID must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_EVENT_HANDLER_GUARD_EXCEPTIONS) &&                     \
    !D_CFG_IS_OFF(D_CFG_EVENT_HANDLER_GUARD_EXCEPTIONS)
#   error "D_CFG_EVENT_HANDLER_GUARD_EXCEPTIONS must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_EVENT_HANDLER_GUARD_VERDICT) &&                        \
    !D_CFG_IS_OFF(D_CFG_EVENT_HANDLER_GUARD_VERDICT)
#   error "D_CFG_EVENT_HANDLER_GUARD_VERDICT must be 0 or 1"
#endif


// ===========================================================================
//  3.  DERIVED VALUES
// ===========================================================================

// D_INTERNAL_EVENT_HANDLER_ADAPTER
//   brief: 1 when event_handler_common.h should declare the fn_callback
// adapter and its state block.
#define D_INTERNAL_EVENT_HANDLER_ADAPTER                                      \
    D_CFG_NORM(D_CFG_EVENT_HANDLER_CALLBACK_ADAPTER)

// D_INTERNAL_EVENT_HANDLER_SEQ
//   brief: 1 when the explicit sequencing combinator should be declared.
#define D_INTERNAL_EVENT_HANDLER_SEQ                                          \
    D_CFG_NORM(D_CFG_EVENT_HANDLER_SEQ)

// D_INTERNAL_EVENT_HANDLER_ALLOW_VOID
//   brief: 1 when handler_traits should accept a void return as always-pass.
#define D_INTERNAL_EVENT_HANDLER_ALLOW_VOID                                   \
    D_CFG_NORM(D_CFG_EVENT_HANDLER_ALLOW_VOID)

// D_INTERNAL_EVENT_HANDLER_GUARD
//   brief: 1 when the C++ erased shim should catch escaping exceptions. Read
// by event_handler.hpp.
#define D_INTERNAL_EVENT_HANDLER_GUARD                                        \
    D_CFG_NORM(D_CFG_EVENT_HANDLER_GUARD_EXCEPTIONS)

// D_INTERNAL_EVENT_HANDLER_GUARD_CODE
//   brief: the pinned verdict code a guarded shim returns on an exception.
#if D_CFG_IS_ON(D_CFG_EVENT_HANDLER_GUARD_VERDICT)
#   define D_INTERNAL_EVENT_HANDLER_GUARD_CODE 1
#else
#   define D_INTERNAL_EVENT_HANDLER_GUARD_CODE 0
#endif


#endif  // DJINTERP_CONFIG_CORE_EVENT_CFG_EVENT_HANDLER_H
