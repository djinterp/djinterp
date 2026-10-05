/*******************************************************************************
* djinterp [c]                                                           maybe.h
*
* The `maybe` carrier: nothing, or exactly one value.
*   Table 7.2 of functional_types.tex gives `maybe<T>` as a kind `* -> *`
* carrier holding 0--1 elements, modelling Functor, Applicative, Monad,
* Foldable, Traversable and Alternative. This header supplies the carrier and
* the instances that do not require naming a type constructor generically; see
* the note on kinds below for what is deliberately absent.
*
* STORAGE AND VIEWS. C++ spells the element type as a template parameter, so a
* `maybe<T>` holds its T by value. C cannot, so the two halves are separated:
*   - `D_MAYBE_DECLARE(name, type)` emits a struct holding the tag and the value
*     BY VALUE, so a maybe cell is byte-determinate and has a known `sizeof`,
*     as goal 4 requires.
*   - `struct d_maybe` is a type-erased VIEW onto such a cell: three words
*     pointing at the tag, the value slot, and the slot's width. Every generic
*     operation takes a view, so there is exactly one implementation of `map`
*     rather than one per element type.
* A view borrows; it never owns. The cell must outlive every view onto it.
*
* ON KINDS, AND WHAT IS NOT HERE. The parity assessment places Functor,
* Applicative, Monad, Foldable and Alternative at tier 3 because C cannot name
* `F<A>` and `F<B>` generically. That is a statement about code generic over the
* CARRIER. Over a KNOWN carrier the operations are ordinary: `d_maybe_map` takes
* two views whose widths differ and a transform between them, and nothing
* higher-kinded is required. So the instances here are concrete instances, not a
* Functor protocol -- a `struct d_functor` that any carrier could implement is
* the tier-3 work and is not attempted here.
*   `traverse` is absent for the reason the parity assessment gives: it composes
* two constructors, and the corpus needs only a narrow slice of it. That slice
* has not yet been scoped in the .tex, so scoping it is a prerequisite rather
* than something to guess at.
*
* FOLDABLE IS THE TIER 1 SPINE. A maybe is a sequence of 0 or 1 elements, so
* its Foldable instance is not new machinery: `d_maybe_drive` pushes the cell
* through an ordinary `d_reducer`, and `d_maybe_producer` presents it as an
* ordinary `d_producer`. Any fold, quantifier or transducer chain already
* written against those works on a maybe unchanged.
*
*
* path:      /inc/djinterp/c/functional/maybe.h
* link(s):   TBA
* author(s): TBA                                             created: 2026.07.30
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_C_FUNCTIONAL_MAYBE_H
#define DJINTERP_C_FUNCTIONAL_MAYBE_H 1

// std
#include <stddef.h>
#include <string.h>
// djinterp
#include "../djinterp.h"
#include "./functional_common.h"
#include "./reducer.h"
#include "./producer.h"
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


// d_maybe
//   struct: a borrowed, type-erased view onto a maybe cell.
// `has_value` and `value` point into a cell declared with D_MAYBE_DECLARE (or
// any equivalent caller-owned storage). The view owns nothing.
struct d_maybe
{
    bool*  has_value;   // the cell's tag
    void*  value;       // the cell's value slot
    size_t value_size;  // byte width of the value slot
};

// D_MAYBE_FIELDS
//   macro: THE declaration of a maybe cell's layout, and the only one.
// Goal 4 requires a shared type to be declared once and compiled by both
// languages. A C++ face cannot reuse `D_MAYBE_DECLARE` inside a template --
// the macro names the struct -- so the FIELDS are factored out here, and both
// the C macro below and the C++ template in functional_face.hpp expand this
// same text. Neither language restates the members.
#define D_MAYBE_FIELDS(type)                                                \
    bool has_value;                                                         \
    type value;

// D_MAYBE_DECLARE
//   macro: declares a maybe cell type holding `type` by value.
// Use it at file or block scope:  D_MAYBE_DECLARE(maybe_int, int);
#define D_MAYBE_DECLARE(name,                                               \
                        type)                                               \
    struct name                                                             \
    {                                                                       \
        D_MAYBE_FIELDS(type)                                                \
    }

// D_MAYBE_VIEW
//   macro: builds a view onto a cell declared with D_MAYBE_DECLARE.
// The width is taken from the cell's own value member, so the view can never
// disagree with the storage it describes.
#define D_MAYBE_VIEW(cell)                                                  \
    d_maybe_view(&(cell).has_value,                                         \
                 &(cell).value,                                             \
                 sizeof((cell).value))

// I.     construction
struct d_maybe d_maybe_view(bool*  _has_value,
                            void*  _value,
                            size_t _value_size);
bool           d_maybe_is_valid(const struct d_maybe* _maybe);

// II.    introduction (the `unit` of Table 7.2)
bool     d_maybe_just(struct d_maybe* _maybe,
                      const void*     _value);
bool     d_maybe_nothing(struct d_maybe* _maybe);

// III.   inspection
bool     d_maybe_is_just(const struct d_maybe* _maybe);
bool     d_maybe_is_nothing(const struct d_maybe* _maybe);
bool     d_maybe_value_or(const struct d_maybe* _maybe,
                          void*                 _out,
                          const void*           _fallback);

// IV.    Functor and Monad, over this carrier
bool     d_maybe_map(const struct d_maybe* _in,
                     struct d_maybe*       _out,
                     fn_transformer        _transform,
                     void*                 _context);
bool     d_maybe_bind(const struct d_maybe* _in,
                      struct d_maybe*       _out,
                      fn_kleisli            _arrow,
                      void*                 _context);
bool     d_maybe_filter(struct d_maybe* _maybe,
                        fn_predicate    _predicate,
                        void*           _context);

// V.     Alternative
bool     d_maybe_alt(const struct d_maybe* _first,
                     const struct d_maybe* _second,
                     struct d_maybe*       _out);
bool     d_maybe_aempty(struct d_maybe* _out);

// VI.    Foldable, via the tier 1 spine
size_t   d_maybe_drive(const struct d_maybe*    _maybe,
                       const struct d_reducer*  _reducer,
                       struct d_reducing_state* _state);
bool     d_maybe_fold(const struct d_maybe* _maybe,
                      void*                 _accumulator,
                      fn_fold               _step,
                      void*                 _context);
struct d_producer d_maybe_producer(struct d_producer_array_state* _state,
                                   const struct d_maybe*          _maybe);

// VII.   layout assertions (Layout law: one declaration, asserted in both)
D_STATIC_ASSERT(offsetof(struct d_maybe, value) == sizeof(bool*),
                "d_maybe layout drift");
D_STATIC_ASSERT(offsetof(struct d_maybe, value_size) ==
                    (sizeof(bool*) + sizeof(void*)),
                "d_maybe layout drift");


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_FUNCTIONAL_MAYBE_H
