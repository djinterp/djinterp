/*******************************************************************************
* djinterp [c]                                           event_registry_common.h
*
* The registry and its folds -- the shared C core (tier 0):
*   The subscription layer: a registry rho is a table read as a family of
* words indexed by kappa. This header declares the two folds the note
* defines -- dispatch (the inner fold of seq over the masked word, for one
* occurrence) and run (the outer fold of dispatch over a homogeneous trace) --
* together with their enriched results and the staging operation that
* evaluates a static word once into a single closed step.
*
*   Dispatch and the fused word share ONE fold, d_event_step_fold, rather than
* each carrying its own loop. That is deliberate: the fused/erased coherence
* law says the two paths agree, and the cheapest way to keep a law true is to
* leave only one thing that could break it.
*
* FORMAL CORRESPONDENCE ("Definition of an Event"):
*   registry  rho in prod_e H_e*  -- struct d_event_registry
*   dispatch  delta_rho           -- d_event_registry_dispatch: the inner fold
*                                    of seq over mask_m(rho_e)
*   run       run_rho             -- d_event_registry_run: the outer fold over
*                                    a trace
*   enriched dispatch (count, P)  -- struct d_dispatch_result
*   merge     rho (+) rho'        -- d_event_registry_merge
*   staging   hat-h, once         -- d_event_registry_compile -> d_event_word
*
* SNAPSHOT SEMANTICS:
*   dispatch folds the word in force AT THE START of the occurrence. A handler
* that binds or unbinds during its own dispatch does not change the word being
* folded; the edit is visible to the next occurrence. compile() takes the
* snapshot earlier still -- at compile time -- so a d_event_word is unaffected
* by every later registry edit. Binding time is the whole difference between
* the two paths, and it is the reason drive() and run() are separate calls.
*
* PORTABLE ACROSS:
*   C99, C11, C17, C23  /  C++11, C++14, C++17, C++20, C++23, C++26
*
*
* path:      /inc/djinterp/c/event/event_registry_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.30
*                                                            revised: 2026.10.03
*******************************************************************************/

#ifndef DJINTERP_C_EVENT_EVENT_REGISTRY_COMMON_H
#define DJINTERP_C_EVENT_EVENT_REGISTRY_COMMON_H 1

// std
#include <stddef.h>
// djinterp
#include "../djinterp.h"
#include "../../config/core/event/cfg_event_registry.h"
#include "./event_common.h"
#include "./event_handler_common.h"
#include "./event_table_common.h"
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // int32_t, uint32_t

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


D_EXTERN_C_BEGIN


///////////////////////////////////////////////////////////////////////////////
///        I.    ENRICHED RESULTS                                           ///
///////////////////////////////////////////////////////////////////////////////

// d_dispatch_result
//   struct: the result of dispatching one occurrence -- the pair
// (count, final verdict). Evaluating dispatch in the product with (N, +)
// counts the letters actually invoked; `outcome` is the verdict of the folded
// word. `outcome` is int32_t carrying a pinned D_VERDICT_* code, not an enum,
// because the record's byte layout is part of the wire format.
struct d_dispatch_result
{
    size_t  invoked;
    int32_t outcome;
    int32_t reserved;   // must be 0
};

// d_run_result
//   struct: the aggregate of folding dispatch over a trace.
struct d_run_result
{
    size_t occurrences;
    size_t handlers_invoked;
    size_t consumed_count;
};

// d_drive_result
//   struct: the aggregate of driving a trace through a fused word. The fused
// path is deliberately NOT instrumented per handler -- that is the erased
// path's role -- so only occurrence and consume counts are reported.
struct d_drive_result
{
    size_t occurrences;
    size_t consumed_count;
};

// d_dispatch_result_consumed
//   function: true if the occurrence was consumed by one of its handlers.
D_STATIC_INLINE bool
d_dispatch_result_consumed(const struct d_dispatch_result* _result)
{
    return ( (_result != NULL) &&
             D_VERDICT_IS_CONSUMED(_result->outcome) );
}

// d_dispatch_result_empty
//   function: the result of dispatching against an empty word -- zero
// invocations, verdict pass. This is what dispatch returns for a key with no
// handlers, and it is NOT an error: an alphabet summand with no subscribers
// is a well-formed state.
D_STATIC_INLINE struct d_dispatch_result
d_dispatch_result_empty(void)
{
    struct d_dispatch_result result;

    result.invoked  = 0u;
    result.outcome  = (int32_t)D_VERDICT_PASS;
    result.reserved = 0;

    return result;
}


///////////////////////////////////////////////////////////////////////////////
///        II.   THE REGISTRY                                               ///
///////////////////////////////////////////////////////////////////////////////

// d_event_registry
//   struct: the registry rho. It is exactly one table; the type exists so
// that "the store" and "the thing you fold" are separately nameable, and so
// that the C++ event_registry has a C member to contain rather than a second
// set of fields.
struct d_event_registry
{
    struct d_event_table table;
};

// d_event_registry_init_fixed
//   function: initializes a registry over caller-provided table storage. No
// allocation. Arguments as for d_event_table_init_fixed.
int d_event_registry_init_fixed(struct d_event_registry* _registry,
                                struct d_event_entry*    _entries,
                                size_t                   _capacity,
                                uint32_t*                _bucket_head,
                                uint32_t*                _bucket_tail,
                                size_t                   _bucket_count);

// d_event_registry_init
//   function: initializes an allocating registry. A zero argument selects the
// corresponding default.
int d_event_registry_init(struct d_event_registry* _registry,
                          size_t                   _capacity,
                          size_t                   _bucket_count);

// d_event_registry_dispose
//   function: releases the registry's table.
void d_event_registry_dispose(struct d_event_registry* _registry);

// d_event_registry_table
//   function: the underlying table, for the storage operations the registry
// does not re-declare (enable, disable, contains, iteration, statistics).
D_STATIC_INLINE struct d_event_table*
d_event_registry_table(struct d_event_registry* _registry)
{
    return (_registry ? &_registry->table : NULL);
}

// d_event_registry_table_const
//   function: const overload of d_event_registry_table.
D_STATIC_INLINE const struct d_event_table*
d_event_registry_table_const(const struct d_event_registry* _registry)
{
    return (_registry ? &_registry->table : NULL);
}


///////////////////////////////////////////////////////////////////////////////
///        III.  BIND                                                       ///
///////////////////////////////////////////////////////////////////////////////

// d_event_registry_bind
//   function: appends a letter to the word for `_key`. Forwards to
// d_event_table_bind; declared here so that a caller working against the
// registry never needs to reach for the table.
int d_event_registry_bind(struct d_event_registry* _registry,
                          d_event_key              _key,
                          struct d_event_step      _step,
                          fn_free                  _state_free,
                          d_handler_id*            _id_out);

// d_event_registry_unbind
//   function: removes the letter named by `_id`.
int d_event_registry_unbind(struct d_event_registry* _registry,
                            d_handler_id             _id);

#if (D_INTERNAL_EVENT_REGISTRY_MERGE == 1)

// d_event_registry_merge
//   function: pointwise concatenation rho (+) rho'. Forwards to
// d_event_table_merge; the empty registry is the identity.
int d_event_registry_merge(struct d_event_registry*       _registry,
                           const struct d_event_registry* _other,
                           size_t*                        _merged_out);


#endif  // D_INTERNAL_EVENT_REGISTRY_MERGE


///////////////////////////////////////////////////////////////////////////////
///        IV.   DISPATCH -- the inner fold                                 ///
///////////////////////////////////////////////////////////////////////////////

// d_event_registry_dispatch
//   function: evaluates the effective word for `_key` over one occurrence.
// Enabled letters are invoked in word order; disabled letters are masked out
// and act as skip; the first consume cuts off the remainder.
//   `_payload` points at the packed payload block A_e and may be NULL for an
// event of arity 0. It is passed through untouched: the core never reads it.
// returns: the enriched (count, verdict) result. An absent word yields
// d_dispatch_result_empty().
struct d_dispatch_result d_event_registry_dispatch(
    struct d_event_registry* _registry,
    d_event_key              _key,
    void*                    _payload);

// d_event_registry_dispatch_occurrence
//   function: dispatch spelled over an occurrence pair (e, a) rather than a
// loose key and pointer. Equivalent to
// d_event_registry_dispatch(r, o->key, o->payload.data).
struct d_dispatch_result d_event_registry_dispatch_occurrence(
    struct d_event_registry*         _registry,
    const struct d_event_occurrence* _occurrence);


///////////////////////////////////////////////////////////////////////////////
///        V.    RUN -- the outer fold                                      ///
///////////////////////////////////////////////////////////////////////////////

#if (D_INTERNAL_EVENT_REGISTRY_RUN == 1)

// d_event_registry_run
//   function: folds dispatch over a homogeneous trace of `_key` occurrences.
// The trace is a contiguous array of payload blocks -- `_first` is the base,
// `_stride` the distance between consecutive blocks, `_count` their number --
// which is the C spelling of the C++ [first, last) iterator pair.
//   Each block is dispatched IN PLACE, so a handler taking a mutable payload
// mutates the caller's array. That differs from the C++ face, which copied
// each element per occurrence; the copy is a C++ convenience that requires a
// copy constructor, so it belongs in the face and not in the core. A caller
// wanting per-occurrence isolation copies into a scratch block and dispatches
// that.
// returns: the aggregate run result.
struct d_run_result d_event_registry_run(struct d_event_registry* _registry,
                                         d_event_key              _key,
                                         void*                    _first,
                                         size_t                   _stride,
                                         size_t                   _count);


#endif  // D_INTERNAL_EVENT_REGISTRY_RUN


///////////////////////////////////////////////////////////////////////////////
///        VI.   STAGING -- the fused word                                  ///
///////////////////////////////////////////////////////////////////////////////

#if (D_INTERNAL_EVENT_REGISTRY_STAGING == 1)

// D_EVENT_WORD_FLAG_OWNS_STORAGE
//   constant: set when the word allocated its own step array.
#define D_EVENT_WORD_FLAG_OWNS_STORAGE      ((uint32_t)0x00000001u)

// d_event_word
//   struct: a static registry word for one key, evaluated once into a single
// closed step (hat-h). Holds a snapshot of the enabled letters, in order,
// taken at compile time; folding it costs no table lookup and no re-reading
// of the registry.
//   A d_event_word does NOT track the registry. Later binds, unbinds, and
// mask changes are invisible to it. That is the point of staging, and it is
// also the precondition the coherence law attaches to: the law holds for a
// registry held fixed between the snapshot and the runs.
struct d_event_word
{
    struct d_event_step* steps;
    size_t               count;
    size_t               capacity;
    d_event_key          key;
    uint32_t             flags;
    uint32_t             reserved;   // must be 0
};

// d_event_registry_compile_into
//   function: snapshots the enabled letters of the word for `_key` into
// caller-provided step storage. No allocation.
//   `_steps` must hold `_capacity` steps. If the word is longer than
// `_capacity` the snapshot is TRUNCATED and D_EVENT_ERR_CAPACITY is returned
// with `_word_out` holding the prefix that fitted -- a truncated word is a
// different word, so this is a mechanical failure the caller must not ignore.
// returns: D_EVENT_OK, D_EVENT_ERR_NULL, or D_EVENT_ERR_CAPACITY.
int d_event_registry_compile_into(const struct d_event_registry* _registry,
                                  d_event_key                    _key,
                                  struct d_event_step*           _steps,
                                  size_t                         _capacity,
                                  struct d_event_word*           _word_out);

#if (D_INTERNAL_EVENT_REGISTRY_STAGING_ALLOC == 1)

// d_event_registry_compile
//   function: as d_event_registry_compile_into, but the word allocates and
// owns its step array. Release it with d_event_word_dispose.
int d_event_registry_compile(const struct d_event_registry* _registry,
                             d_event_key                    _key,
                             struct d_event_word*           _word_out);

#endif  // D_INTERNAL_EVENT_REGISTRY_STAGING_ALLOC

// d_event_word_dispose
//   function: releases a word's step array if it owns one. Safe on a zeroed
// word and safe to call twice.
void d_event_word_dispose(struct d_event_word* _word);

// d_event_word_size
//   function: the number of letters fused into the word.
D_STATIC_INLINE size_t
d_event_word_size(const struct d_event_word* _word)
{
    return (_word ? _word->count : 0u);
}

// d_event_word_run_one
//   function: folds the snapshotted word over one payload block, stopping at
// the first consume. `_invoked_out` may be NULL.
// returns: the pinned verdict code.
int32_t d_event_word_run_one(const struct d_event_word* _word,
                             void*                      _payload,
                             size_t*                    _invoked_out);

// d_event_word_drive
//   function: runs a homogeneous trace through the fused word as a single
// loop -- the fused path's outer fold. Trace arguments as for
// d_event_registry_run.
//
//   COHERENCE LAW: for a registry rho held fixed -- no bind, unbind, enable,
// or disable between the snapshot and the runs --
//
//       d_event_registry_compile(&rho, k, &w);
//       d_event_word_drive(&w, first, stride, n).occurrences
//           == d_event_registry_run(&rho, k, first, stride, n).occurrences
//       d_event_word_drive(&w, first, stride, n).consumed_count
//           == d_event_registry_run(&rho, k, first, stride, n).consumed_count
//
// with identical per-occurrence side effects in identical order. This is the
// operational form of the fused/erased coherence proposition and the contract
// any fused build must preserve.
struct d_drive_result d_event_word_drive(const struct d_event_word* _word,
                                         void*                      _first,
                                         size_t                     _stride,
                                         size_t                     _count);


#endif  // D_INTERNAL_EVENT_REGISTRY_STAGING


///////////////////////////////////////////////////////////////////////////////
///        VII.  LAYOUT ASSERTIONS                                          ///
///////////////////////////////////////////////////////////////////////////////

#if (D_INTERNAL_EVENT_ASSERT_SIZES == 1)

    D_STATIC_ASSERT(sizeof(struct d_dispatch_result) == 16,
                    "d_dispatch_result layout drift");
#if (D_INTERNAL_EVENT_REGISTRY_RUN == 1)
    D_STATIC_ASSERT(sizeof(struct d_run_result) == 24,
                    "d_run_result layout drift");
#endif
#if (D_INTERNAL_EVENT_REGISTRY_STAGING == 1)
    D_STATIC_ASSERT(sizeof(struct d_drive_result) == 16,
                    "d_drive_result layout drift");
#endif
    D_STATIC_ASSERT(sizeof(struct d_event_registry) == 80,
                    "d_event_registry layout drift");
#if (D_INTERNAL_EVENT_REGISTRY_STAGING == 1)
    D_STATIC_ASSERT(sizeof(struct d_event_word) == 40,
                    "d_event_word layout drift");
#endif
    D_STATIC_ASSERT(sizeof(struct d_event_registry) ==
                    sizeof(struct d_event_table),
                    "the registry must add no state to the table");

#endif  // D_INTERNAL_EVENT_ASSERT_SIZES

#if (D_INTERNAL_EVENT_ASSERT_LAYOUT == 1)

D_STATIC_ASSERT(offsetof(struct d_dispatch_result, invoked) == 0,
                "d_dispatch_result field drift");
D_STATIC_ASSERT(offsetof(struct d_event_registry, table) == 0,
                "d_event_registry field drift");
#if (D_INTERNAL_EVENT_REGISTRY_STAGING == 1)
D_STATIC_ASSERT(offsetof(struct d_event_word, steps) == 0,
                "d_event_word field drift");
#endif

#endif  // D_INTERNAL_EVENT_ASSERT_LAYOUT


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_EVENT_EVENT_REGISTRY_COMMON_H
