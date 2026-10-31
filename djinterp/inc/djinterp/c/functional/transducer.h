/*******************************************************************************
* djinterp [c]                                                      transducer.h
*
* Transducers: source-agnostic and sink-agnostic reducer transformers.
*   A transducer is a function from one reducer to another. Because the reducer
* it produces is still just a reducer, one `map | filter | take` chain runs over
* an array, over a producer, or over anything else a driver can push, and lands
* in any sink. That is the whole point of the shape: the pipeline is written
* once and is independent of both ends.
*   C++ builds the composition out of nested closures. C has no closures, so the
* chain is an explicit array of stages and the composition is index arithmetic:
* stage `i` hands its output to `d_transducer_emit(t, i + 1, ...)`, and the emit
* past the last stage is the sink. `d_transducer_into_reducer` collapses the
* whole chain back into a single `dj_reducer`, which is the C spelling of
* `into_reducer(xform, downstream)` and the reason nothing here allocates.
*   Stages carry mutable per-run state (`take`'s counter, `drop_while`'s latch),
* so a chain is not reentrant and must be handed to `d_transducer_reset` before
* it is driven a second time.
*   Reading order follows the C++ `compose`: `stages[0]` sees each value first.
* A `filter` stage never grows the stream, so a filtered chain satisfies
* `output_count <= input_count`; a `take(n)` chain satisfies
* `output_count <= n`.
*
*
* path:      /inc/djinterp/c/functional/transducer.h
* link(s):   TBA
* author(s): TBA                                             created: 2026.07.30
*                                                            revised: 2026.10.23
*******************************************************************************/

#ifndef DJINTERP_C_FUNCTIONAL_TRANSDUCER_H
#define DJINTERP_C_FUNCTIONAL_TRANSDUCER_H 1

// std
#include <stddef.h>
// djinterp
#include "../djinterp.h"
#include "./functional_common.h"
#include "./reducer.h"
#include "./producer.h"
// re_std
#include "../../../re_std/c/cstdint/dstdint.h"  // RE_STD_HAS_INT64: its floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict C++98
// wherever int64_t is `long long`: a 32-bit target, 64-bit Windows or macOS --
// the header compiles to nothing (the owner's ruling of 2026.10.03 on round 3's
// question 1, (a)).
#if RE_STD_HAS_INT64


// dj_transducer
//   struct: forward declaration.
// The step typedef below names this type, and a struct first named inside a
// parameter list would be a distinct, file-local type. Declaring it here is
// what makes `fn_transducer_step` refer to the real chain.
struct dj_transducer;

// dj_transducer_callable
//   union: the leaf callable a stage applies to each value.
// A union of function pointers rather than a `void*`, because C does not
// guarantee that a function pointer survives a round trip through an object
// pointer.
union dj_transducer_callable
{
    fn_predicate      predicate;  // filter, filter_not, take_while, drop_while
    fn_transformer    transform;  // map
    fn_consumer_const consume;    // tap
};

// fn_transducer_step
//   function pointer: applies one stage of a chain to one value.
// The stage emits downstream with `d_transducer_emit(_transducer, _index + 1,
// ...)`, or emits nothing at all to drop the value.
typedef void (*fn_transducer_step)(struct dj_transducer*     _transducer,
                                   size_t                   _index,
                                   struct dj_reducing_state* _state,
                                   const void*              _element);

// dj_transducer_stage
//   struct: one stage of a transducer chain.
// `seen` and `latched` are mutable run state, not configuration; they are
// cleared by `d_transducer_reset`. `scratch` is caller-owned output storage
// used only by mapping stages, and must be at least as large as the mapped
// value.
struct dj_transducer_stage
{
    fn_transducer_step          step;      // the stage behaviour
    union dj_transducer_callable callable;  // the leaf callable, if any
    void*                       context;   // context for the callable
    void*                       scratch;   // mapped-value buffer, if any
    size_t                      bound;     // n for take and drop
    size_t                      seen;      // mutable: values seen by the stage
    bool                        latched;   // mutable: drop_while latch
};

// dj_transducer
//   struct: a stage chain terminating in a sink.
// `stages` is a caller-owned array of `count` stages, applied in index order.
// The sink receives whatever survives the chain.
struct dj_transducer
{
    struct dj_transducer_stage* stages;    // caller-owned stage array
    size_t                     count;     // number of stages
    struct dj_reducer           sink;      // terminal reducer
};

// dj_array_sink
//   struct: sink state appending values into a bounded contiguous array.
// Values arriving after the array is full are counted in `overflow` and
// discarded, so a full sink is not an error but a reported condition.
struct dj_array_sink
{
    unsigned char* cursor;        // next write position
    size_t         remaining;     // free slots left
    size_t         element_size;  // stride between slots
    size_t         written;       // slots filled so far
    size_t         overflow;      // values dropped for want of space
};

// I.     stage factories
struct dj_transducer_stage d_transducer_map(
    fn_transformer _transform,
    void*          _context,
    void*          _scratch
);
struct dj_transducer_stage d_transducer_filter(
    fn_predicate _predicate,
    void*        _context
);
struct dj_transducer_stage d_transducer_filter_not(
    fn_predicate _predicate,
    void*        _context
);
struct dj_transducer_stage d_transducer_take(
    size_t _count
);
struct dj_transducer_stage d_transducer_drop(
    size_t _count
);
struct dj_transducer_stage d_transducer_take_while(
    fn_predicate _predicate,
    void*        _context
);
struct dj_transducer_stage d_transducer_drop_while(
    fn_predicate _predicate,
    void*        _context
);
struct dj_transducer_stage d_transducer_tap(
    fn_consumer_const _consume,
    void*             _context
);

// II.    chain construction
bool d_transducer_init(
    struct dj_transducer*       _transducer,
    struct dj_transducer_stage* _stages,
    size_t                      _count,
    struct dj_reducer           _sink
);
void d_transducer_reset(
    struct dj_transducer* _transducer
);
bool d_transducer_is_valid(
    const struct dj_transducer* _transducer
);

// III.   reducer conversion
struct dj_reducer d_transducer_into_reducer(
    struct dj_transducer* _transducer
);
void d_transducer_emit(
    struct dj_transducer*     _transducer,
    size_t                    _index,
    struct dj_reducing_state* _state,
    const void*               _element
);

// IV.    array sink
struct dj_reducer d_reducer_from_array_sink(
    struct dj_array_sink* _sink
);
bool d_array_sink_init(
    struct dj_array_sink* _sink,
    void*                 _out_array,
    size_t                _capacity,
    size_t                _element_size
);

// V.     drivers
size_t d_transduce_array(
    struct dj_transducer*     _transducer,
    struct dj_reducing_state* _state,
    const void*               _elements,
    size_t                    _count,
    size_t                    _element_size
);
size_t d_transduce_producer(
    struct dj_transducer*     _transducer,
    struct dj_producer*       _producer,
    struct dj_reducing_state* _state,
    void*                     _scratch
);
size_t d_transduce_into_array(
    struct dj_transducer_stage* _stages,
    size_t                      _stage_count,
    const void*                 _elements,
    size_t                      _count,
    size_t                      _element_size,
    void*                       _out_array,
    size_t                      _capacity,
    size_t*                     _out_written
);
size_t d_transduce_producer_to_consumer(
    struct dj_transducer* _transducer,
    struct dj_producer*   _producer,
    fn_consumer_const     _consume,
    void*                 _context,
    void*                 _scratch
);

// VI.    filter, as predicate plus sink
size_t d_filter_into_array(
    const void*  _elements,
    size_t       _count,
    size_t       _element_size,
    fn_predicate _predicate,
    void*        _context,
    void*        _out_array,
    size_t       _capacity
);
size_t d_filter_to_consumer(
    const void*       _elements,
    size_t            _count,
    size_t            _element_size,
    fn_predicate      _predicate,
    void*             _predicate_context,
    fn_consumer_const _consume,
    void*             _consumer_context
);

// VII.   layout assertions (Layout law: one declaration, asserted in both)
D_STATIC_ASSERT(offsetof(struct dj_transducer, count) == sizeof(void*),
                "dj_transducer layout drift");
D_STATIC_ASSERT(offsetof(struct dj_transducer_stage, callable) ==
                    sizeof(fn_transducer_step),
                "dj_transducer_stage layout drift");
D_STATIC_ASSERT(sizeof(union dj_transducer_callable) == sizeof(fn_predicate),
                "dj_transducer_callable must not exceed one function pointer");


#endif  // RE_STD_HAS_INT64

#endif  // DJINTERP_C_FUNCTIONAL_TRANSDUCER_H
