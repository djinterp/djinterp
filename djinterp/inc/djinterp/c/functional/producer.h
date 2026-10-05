/*******************************************************************************
* djinterp [c]                                                        producer.h
*
* Pull-based producers: a state struct plus a `next` step.
*   A producer is the dual of a consumer. Where a consumer absorbs elements one
* at a time, a producer emits them one at a time on demand: `next` writes the
* next value into a caller-supplied buffer and reports whether there was one.
* That single operation is the whole protocol, and it is what lets a producer
* stand in for an array anywhere a reducer is driven.
*   C++ spells the emitted type as a template parameter; C cannot, so a producer
* carries `value_size` and writes bytes. Integer ranges therefore emit
* `intmax_t`: it is the widest lowering of `range_helper<Int>` that does not
* fork the type per width, and narrowing is the caller's business.
*   Producers are stateful and one-shot: pulling consumes. Every state struct
* here is caller-owned, so nothing in this header allocates. Adapters
* (`take_n`, `filter`, `transform`, `concat`, …) borrow their inner producer by
* pointer, which must outlive the adapter.
*   Many producers are notionally infinite (`repeat`, `generate`, `iterate`) and
* become finite only when bounded by `take_n`. Draining an unbounded producer
* does not return.
*
*
* path:      /inc/djinterp/c/functional/producer.h
* link(s):   TBA
* author(s): TBA                                             created: 2026.07.30
*                                                            revised: 2026.10.03
*******************************************************************************/

#ifndef DJINTERP_C_FUNCTIONAL_PRODUCER_H
#define DJINTERP_C_FUNCTIONAL_PRODUCER_H 1

// std
#include <stddef.h>
// djinterp
#include "../djinterp.h"
#include "./functional_common.h"
#include "./reducer.h"
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // intmax_t

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


// fn_producer_next
//   function pointer: emits the next value of a producer.
// Writes into `_out` and returns whether a value was produced. Once it returns
// false it must keep returning false.
typedef bool (*fn_producer_next)(void* _state,
                                 void* _out);

// d_producer
//   struct: a pull-based source, being a state struct plus a `next` step.
// `state` is caller-owned and specific to the `next` implementation.
// `value_size` is the byte size of the values written into the out buffer.
struct d_producer
{
    fn_producer_next next;          // the pull step
    void*            state;         // caller-owned producer state
    size_t           value_size;    // byte size of each emitted value
};

// d_producer_step
//   struct: the result of one pull.
// `value` points at the caller's out buffer when `has_value` is true, and is
// NULL when the producer is exhausted.
struct d_producer_step
{
    bool  has_value;    // whether a value was produced
    void* value;        // the out buffer, or NULL on exhaustion
};

// d_producer_array_state
//   struct: state for a producer over a contiguous array.
struct d_producer_array_state
{
    const unsigned char* cursor;        // next element to emit
    size_t               remaining;     // elements left to emit
    size_t               element_size;  // stride between elements
};

// d_producer_range_state
//   struct: state for a half-open integer range emitting `intmax_t`.
// The range is [current, end) when `step` is positive and (end, current] when
// negative; a zero step emits nothing.
struct d_producer_range_state
{
    intmax_t current;   // next value to emit
    intmax_t end;       // exclusive bound
    intmax_t step;      // increment; may be negative
};

// d_producer_repeat_state
//   struct: state for a producer repeating one borrowed value.
// A `remaining` of `D_PRODUCER_UNBOUNDED` repeats forever.
struct d_producer_repeat_state
{
    const void* value;      // borrowed value, copied out on each pull
    size_t      value_size; // byte size of `value`
    size_t      remaining;  // pulls left, or D_PRODUCER_UNBOUNDED
};

// d_producer_generate_state
//   struct: state for a producer driven by an `fn_producer` callback.
struct d_producer_generate_state
{
    fn_producer generate;   // callback writing the next value
    void*       context;    // context forwarded to `generate`; may be NULL
};

// d_producer_iterate_state
//   struct: state for `x, f(x), f(f(x)), ...` over a borrowed seed buffer.
// `current` is caller-owned scratch holding the value emitted next.
struct d_producer_iterate_state
{
    void*          current;     // caller-owned buffer holding the next value
    size_t         value_size;  // byte size of the value
    fn_transformer step;        // advances `current` to its successor
    void*          context;     // context forwarded to `step`; may be NULL
    bool           primed;      // whether the seed has been emitted yet
};

// d_producer_take_state
//   struct: state bounding an inner producer to at most `remaining` values.
struct d_producer_take_state
{
    struct d_producer* inner;       // borrowed inner producer
    size_t             remaining;   // values left to pass through
};

// d_producer_drop_state
//   struct: state skipping the first `remaining` values of an inner producer.
struct d_producer_drop_state
{
    struct d_producer* inner;       // borrowed inner producer
    size_t             remaining;   // values left to discard
};

// d_producer_concat_state
//   struct: state emitting all of `first`, then all of `second`.
struct d_producer_concat_state
{
    struct d_producer* first;       // borrowed first producer
    struct d_producer* second;      // borrowed second producer
    bool               first_done;  // whether `first` is exhausted
};

// d_producer_filter_state
//   struct: state passing through only the values a predicate admits.
struct d_producer_filter_state
{
    struct d_producer* inner;       // borrowed inner producer
    fn_predicate       predicate;   // admission test
    void*              context;     // context for `predicate`; may be NULL
};

// d_producer_transform_state
//   struct: state mapping each value of an inner producer.
// `scratch` is caller-owned storage for the inner producer's value; it must be
// at least `inner->value_size` bytes.
struct d_producer_transform_state
{
    struct d_producer* inner;       // borrowed inner producer
    fn_transformer     transform;   // the mapping
    void*              context;     // context for `transform`; may be NULL
    void*              scratch;     // buffer for the inner value
};

// D_PRODUCER_UNBOUNDED
//   constant: sentinel `remaining` marking a producer that never exhausts.
#define D_PRODUCER_UNBOUNDED    ((size_t)-1)

// D_PRODUCER_STEP_NONE uses a compound literal, which is C-only; under C++ it
// is absent rather than ill-formed. A zero-initialised `d_producer_step` is the
// cross-dialect equivalent.
#if !defined(__cplusplus)

// D_PRODUCER_STEP_NONE
//   macro: builds the exhausted `d_producer_step`.
#define D_PRODUCER_STEP_NONE                                                \
    ( (struct d_producer_step){ false,                                      \
                                NULL } )

#endif  // !defined(__cplusplus)

// I.     construction
struct d_producer d_producer_make(fn_producer_next _next,
                                  void*            _state,
                                  size_t           _value_size);
bool              d_producer_is_valid(const struct d_producer* _producer);

// II.    sources
struct d_producer d_producer_empty(size_t _value_size);
struct d_producer d_producer_single(struct d_producer_repeat_state* _state,
                                   const void*                     _value,
                                   size_t                          _value_size);
struct d_producer d_producer_from_array(struct d_producer_array_state* _state,
                                        const void*                    _elements,
                                        size_t                         _count,
                                        size_t                         _element_size);
struct d_producer d_producer_range(struct d_producer_range_state* _state,
                                   intmax_t                       _start,
                                   intmax_t                       _end,
                                   intmax_t                       _step);
struct d_producer d_producer_iota(struct d_producer_range_state* _state,
                                  intmax_t                       _start,
                                  intmax_t                       _end);
struct d_producer d_producer_repeat(struct d_producer_repeat_state* _state,
                                    const void*                     _value,
                                    size_t                          _value_size);
struct d_producer d_producer_repeat_n(struct d_producer_repeat_state* _state,
                                      const void*                     _value,
                                      size_t                          _value_size,
                                      size_t                          _count);
struct d_producer d_producer_generate(struct d_producer_generate_state* _state,
                                      fn_producer                       _generate,
                                      void*                             _context,
                                      size_t                            _value_size);
struct d_producer d_producer_iterate(struct d_producer_iterate_state* _state,
                                     void*                            _seed,
                                     size_t                           _value_size,
                                     fn_transformer                   _step,
                                     void*                            _context);

// III.   adapters
struct d_producer d_producer_take_n(struct d_producer_take_state* _state,
                                    struct d_producer*            _inner,
                                    size_t                        _count);
struct d_producer d_producer_drop_n(struct d_producer_drop_state* _state,
                                    struct d_producer*            _inner,
                                    size_t                        _count);
struct d_producer d_producer_concat(struct d_producer_concat_state* _state,
                                    struct d_producer*              _first,
                                    struct d_producer*              _second);
struct d_producer d_producer_filter(struct d_producer_filter_state* _state,
                                    struct d_producer*              _inner,
                                    fn_predicate                    _predicate,
                                    void*                           _context);
struct d_producer d_producer_transform(struct d_producer_transform_state* _state,
                                       struct d_producer*                 _inner,
                                       fn_transformer                     _transform,
                                       void*                              _context,
                                       void*                              _scratch,
                                       size_t                             _value_size);

// IV.    pulling
struct d_producer_step d_producer_pull(struct d_producer* _producer,
                                       void*              _out);

// V.     drivers
size_t   d_producer_drive(struct d_producer*       _producer,
                          const struct d_reducer*  _reducer,
                          struct d_reducing_state* _state,
                          void*                    _scratch);
size_t   d_producer_collect(struct d_producer* _producer,
                            void*              _out_array,
                            size_t             _capacity);
size_t   d_producer_for_each(struct d_producer* _producer,
                             fn_consumer_const  _consume,
                             void*              _context,
                             void*              _scratch);
size_t   d_producer_fold(struct d_producer* _producer,
                         void*              _accumulator,
                         fn_fold            _step,
                         void*              _context,
                         void*              _scratch);

// VI.    sequence algebra
//   Table 7.2 of functional_types.tex lists the sequence carriers as instances
// of Semigroup, and Table 7.3 lists them as instances of Alternative. C has no
// trait specialization to register an instance with, so the registration is
// these two names: concatenation is the associative combine, and the empty
// producer is the identity. `d_producer_alt` and `d_producer_concat` are the
// same operation under the two vocabularies, and `d_producer_aempty` and
// `d_producer_empty` likewise.
struct d_producer d_producer_alt(struct d_producer_concat_state* _state,
                                 struct d_producer*              _first,
                                 struct d_producer*              _second);
struct d_producer d_producer_aempty(size_t _value_size);

// VII.   layout assertions (Layout law: one declaration, asserted in both)
D_STATIC_ASSERT(offsetof(struct d_producer, state) == sizeof(fn_producer_next),
                "d_producer layout drift");
D_STATIC_ASSERT(sizeof(struct d_producer) >=
                    (sizeof(fn_producer_next) + sizeof(void*) + sizeof(size_t)),
                "d_producer layout drift");


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_FUNCTIONAL_PRODUCER_H
