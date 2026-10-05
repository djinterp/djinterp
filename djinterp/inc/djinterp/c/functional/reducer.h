/*******************************************************************************
* djinterp [c]                                                         reducer.h
*
* The reducer spine: accumulator state, the reducing step, and its drivers.
*   Every dataflow shape in this subframework is one of three things: a reducer
* (a step that folds one element into an accumulator), something that turns a
* reducer into another reducer (a transducer), or a driver that pushes a source
* through a reducer. This header defines the reducer and the drivers; producer.h
* defines the sources, transducer.h defines the reducer-to-reducer morphisms.
*   `d_reducing_state` carries the accumulator together with an early-termination
* latch, which is what makes a short-circuiting fold expressible: a quantifier
* is a fold whose step sets `done`. Drivers stop as soon as the latch is set.
*   All drivers are allocation-free. The caller owns the accumulator, the state,
* and every binding struct passed in here.
*
*
* path:      /inc/djinterp/c/functional/reducer.h
* link(s):   TBA
* author(s): TBA                                             created: 2026.07.30
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_C_FUNCTIONAL_REDUCER_H
#define DJINTERP_C_FUNCTIONAL_REDUCER_H 1

// std
#include <stddef.h>
// djinterp
#include "../djinterp.h"
#include "./functional_common.h"


// d_reducing_state
//   struct: an accumulator paired with an early-termination latch.
// `accumulator` is caller-owned storage of whatever type the reducer folds
// into; the reducer and the accumulator must agree on that type. `done` is
// set by a step to ask the driver to stop; drivers must honour it and must
// never clear it.
struct d_reducing_state
{
    void* accumulator;      // caller-owned accumulator storage
    bool  done;             // early-termination latch; set by a step
};

// fn_reducer_step
//   function pointer: folds one element into a reducing state.
// The step may set `_state->done` to terminate the drive early.
// Note: `_context` may be NULL.
typedef void (*fn_reducer_step)(struct d_reducing_state* _state,
                                const void*              _element,
                                void*                    _context);

// d_reducer
//   struct: a reducing step together with its bound context.
// This is the unit that drivers consume and that transducers transform. It is
// deliberately two words wide so it can be passed and returned by value.
struct d_reducer
{
    fn_reducer_step step;   // the folding step
    void*           context;// bound context for `step`; may be NULL
};

// fn_fold
//   function pointer: the plain fold step, `(Acc x A) -> Acc`.
// Writes the combined result back through `_accumulator`. This is the pure
// shape, with no access to the termination latch; lift it with
// `d_reducer_from_fold` to obtain a `d_reducer`.
// Note: `_context` may be NULL.
typedef void (*fn_fold)(void*       _accumulator,
                        const void* _element,
                        void*       _context);

// fn_zipper
//   function pointer: combines one element of each of two sources into one
// output. Returns whether an output was produced.
// Declared here, in the substrate, because three modules need this shape and
// three declarations of one shape is the failure mode goal 4 exists to prevent.
// Note: `_context` may be NULL.
typedef bool (*fn_zipper)(const void* _left,
                          const void* _right,
                          void*       _output,
                          void*       _context);

// fn_kleisli
//   function pointer: the arrow `A -> M<B>` of any monadic carrier.
// Writes through the output carrier, which is how C returns a carrier without a
// template parameter; the callee knows which carrier `_out_carrier` points at.
// It must leave that carrier definite.
// Note: `_context` may be NULL.
typedef void (*fn_kleisli)(const void* _value,
                           void*       _out_carrier,
                           void*       _context);

// d_fold_binding
//   struct: binds an `fn_fold` to its context so it can become a reducer.
// Caller-owned; must outlive every reducer derived from it.
struct d_fold_binding
{
    fn_fold step;           // the plain fold step
    void*   context;        // context forwarded to `step`; may be NULL
};

// d_consumer_binding
//   struct: binds an `fn_consumer_const` to its context so it can be a sink.
// A consumer is a reducer over the unit accumulator: it ignores the
// accumulator entirely and never terminates early.
struct d_consumer_binding
{
    fn_consumer_const consume;  // the sink
    void*             context;  // context forwarded to `consume`; may be NULL
};

// d_quantifier_kind
//   enum: which quantifier a `d_quantifier` folds.
enum d_quantifier_kind
{
    D_QUANTIFIER_ANY  = 0,  // exists an element satisfying the predicate
    D_QUANTIFIER_ALL  = 1,  // every element satisfies it (vacuously true)
    D_QUANTIFIER_NONE = 2   // no element satisfies it (vacuously true)
};

// d_quantifier
//   struct: a predicate plus the quantifier to fold it under.
// Caller-owned; must outlive every reducer derived from it. The accumulator
// of the derived reducer is a `bool`, which `d_quantifier_seed` initialises.
struct d_quantifier
{
    fn_predicate           predicate;   // the test applied to each element
    void*                  context;     // context for `predicate`; may be NULL
    enum d_quantifier_kind kind;        // which quantifier to fold
};

// the two initialiser macros below use compound literals, which are C-only:
// ISO C++ forbids them, so under a C++ compiler they are absent rather than
// ill-formed. `d_reducer_make` and `d_reducing_state_init` are the cross-dialect
// path and are available in both. (The same applies to the existing
// predicate.h combinator macros, which currently lack this guard.)
#if !defined(__cplusplus)

// D_REDUCER_INIT
//   macro: builds a `d_reducer` from a step and a context.
#define D_REDUCER_INIT(step,                                                \
                       context)                                             \
    ( (struct d_reducer){ (step),                                           \
                          (context) } )

// D_REDUCING_STATE_INIT
//   macro: builds a fresh, not-yet-terminated `d_reducing_state`.
#define D_REDUCING_STATE_INIT(accumulator)                                  \
    ( (struct d_reducing_state){ (accumulator),                             \
                                 false } )

#endif  // !defined(__cplusplus)

// I.     reducing state
void     d_reducing_state_init(struct d_reducing_state* _state,
                               void*                    _accumulator);
void     d_reducing_state_mark_done(struct d_reducing_state* _state);
bool     d_reducing_state_is_done(const struct d_reducing_state* _state);
void*    d_reducing_state_accumulator(const struct d_reducing_state* _state);

// II.    reducer construction
struct d_reducer d_reducer_make(fn_reducer_step _step,
                                void*           _context);
struct d_reducer d_reducer_from_fold(struct d_fold_binding* _binding);
struct d_reducer d_reducer_from_consumer(struct d_consumer_binding* _binding);
struct d_reducer d_reducer_from_quantifier(struct d_quantifier* _quantifier);
bool             d_reducer_is_valid(const struct d_reducer* _reducer);

// III.   single-element application
void     d_reducer_step_once(const struct d_reducer*  _reducer,
                             struct d_reducing_state* _state,
                             const void*              _element);

// IV.    drivers
size_t   d_reducer_drive_array(const struct d_reducer*  _reducer,
                               struct d_reducing_state* _state,
                               const void*              _elements,
                               size_t                   _count,
                               size_t                   _element_size);
size_t   d_reducer_drive_array_reverse(const struct d_reducer*  _reducer,
                                       struct d_reducing_state* _state,
                                       const void*              _elements,
                                       size_t                   _count,
                                       size_t                   _element_size);

// V.     eager folds
bool     d_fold_left(const void*    _elements,
                     size_t         _count,
                     size_t         _element_size,
                     void*          _accumulator,
                     fn_fold        _step,
                     void*          _context);
bool     d_fold_right(const void*   _elements,
                      size_t        _count,
                      size_t        _element_size,
                      void*         _accumulator,
                      fn_fold       _step,
                      void*         _context);

// VI.    quantifiers
bool     d_quantifier_seed(enum d_quantifier_kind _kind);
bool     d_reducer_any(const void*  _elements,
                       size_t       _count,
                       size_t       _element_size,
                       fn_predicate _predicate,
                       void*        _context);
bool     d_reducer_all(const void*  _elements,
                       size_t       _count,
                       size_t       _element_size,
                       fn_predicate _predicate,
                       void*        _context);
bool     d_reducer_none(const void* _elements,
                        size_t      _count,
                        size_t      _element_size,
                        fn_predicate _predicate,
                        void*       _context);
size_t   d_reducer_count_if(const void*  _elements,
                            size_t       _count,
                            size_t       _element_size,
                            fn_predicate _predicate,
                            void*        _context);

// VII.   layout assertions (Layout law: one declaration, asserted in both)
D_STATIC_ASSERT(offsetof(struct d_reducer, context) ==
                    sizeof(fn_reducer_step),
                "d_reducer layout drift");
D_STATIC_ASSERT(offsetof(struct d_fold_binding, context) == sizeof(fn_fold),
                "d_fold_binding layout drift");
D_STATIC_ASSERT(sizeof(struct d_reducing_state) >= sizeof(void*),
                "d_reducing_state layout drift");


#endif  // DJINTERP_C_FUNCTIONAL_REDUCER_H
