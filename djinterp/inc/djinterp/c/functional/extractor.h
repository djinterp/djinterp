/*******************************************************************************
* djinterp [c]                                                       extractor.h
*
* Extractors (projections) and the contravariant mapping that consumes them.
*   An extractor reads one feature out of a value: `Source -> Target`. That is
* already `fn_transformer`, so this header adds no new callable shape. What it
* adds is the thing that makes extractors worth naming -- CONTRAMAP, which runs
* a projection BEFORE a context instead of after.
*
* WHY THIS PAIRS TWO C++ MODULES. `extractor.hpp` elevates key functions to
* first-class values; `contravariant.hpp` supplies the one operation that
* consumes them. They are the same idea from the two sides: a predicate over
* `Target` plus a projection `Source -> Target` gives a predicate over `Source`,
* and that composition is precisely `contramap`. Splitting them in C would put
* the projection in one header and its only consumer in another.
*
* PREDICATES AND COMPARATORS ARE CONTRAVARIANT, NOT COVARIANT. `map` on a
* producer transforms what comes OUT; `contramap` on a predicate transforms what
* goes IN. That is why `sort_by_key` and `filter_by_key` are contramap and not
* map, and why there is no `d_predicate_map`: a predicate produces a `bool` and
* there is nothing useful to do to it afterwards.
*
* THE BINDINGS ARE CALLER-OWNED AND CARRY SCRATCH. Projecting `Source` to
* `Target` needs somewhere to put the `Target`, and a C function pointer has no
* captures. So each binding holds the projection, its context, the downstream
* callable, its context, and a scratch slot -- and is itself passed as the
* `void* _context` of an ordinary `fn_predicate` or `fn_binary_predicate`. The
* result drops straight into `d_sorted`, `d_transducer_filter`, or anything else
* taking those types, with no new overloads anywhere.
*   A comparator binding needs TWO scratch slots, because a comparison projects
* both operands before comparing them.
*
*
* path:      /inc/djinterp/c/functional/extractor.h
* link(s):   TBA
* author(s): TBA                                             created: 2026.07.30
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_C_FUNCTIONAL_EXTRACTOR_H
#define DJINTERP_C_FUNCTIONAL_EXTRACTOR_H 1

// std
#include <stddef.h>
// djinterp
#include "../djinterp.h"
#include "./functional_common.h"


// d_extractor
//   struct: a projection bound to its context.
// The projection itself is an ordinary `fn_transformer`; this pairs it with the
// context so it can travel as one value.
struct d_extractor
{
    fn_transformer project;   // `Source -> Target`
    void*          context;   // context for `project`; may be NULL
};

// d_extractor_chain
//   struct: two projections applied in sequence, `Source -> Mid -> Target`.
// `scratch` holds the intermediate `Mid` and must be at least as wide as it.
struct d_extractor_chain
{
    struct d_extractor first;    // `Source -> Mid`
    struct d_extractor second;   // `Mid -> Target`
    void*              scratch;  // caller-owned slot for the intermediate
};

// d_contramap_predicate
//   struct: a predicate over `Target` read as a predicate over `Source`.
// Pass the whole struct as the `void* _context` of `d_contramap_test`, which is
// itself an `fn_predicate`.
struct d_contramap_predicate
{
    struct d_extractor extract;    // `Source -> Target`
    fn_predicate       predicate;  // the test over `Target`
    void*              context;    // context for `predicate`; may be NULL
    void*              scratch;    // caller-owned slot for the projected value
};

// d_contramap_comparator
//   struct: an ordering over `Target` read as an ordering over `Source`.
// Two scratch slots, because both operands are projected before comparison.
struct d_contramap_comparator
{
    struct d_extractor  extract;    // `Source -> Target`
    fn_binary_predicate precedes;   // the ordering over `Target`
    void*               context;    // context for `precedes`; may be NULL
    void*               left;       // caller-owned slot for the left projection
    void*               right;      // caller-owned slot for the right one
};

// I.     construction
struct d_extractor d_extractor_make(fn_transformer _project,
                                    void*          _context);
bool               d_extractor_is_valid(const struct d_extractor* _extractor);
bool     d_extractor_chain_init(struct d_extractor_chain* _chain,
                                struct d_extractor        _first,
                                struct d_extractor        _second,
                                void*                     _scratch);

// II.    application
bool     d_extract(const struct d_extractor* _extractor,
                   const void*               _source,
                   void*                     _target);
bool     d_extractor_chain_apply(const void* _source,
                                 void*       _target,
                                 void*       _chain);

// III.   contravariant mapping
bool     d_contramap_predicate_init(struct d_contramap_predicate* _binding,
                                    struct d_extractor            _extract,
                                    fn_predicate                  _predicate,
                                    void*                         _context,
                                    void*                         _scratch);
bool     d_contramap_test(const void* _element,
                          void*       _binding);
bool     d_contramap_comparator_init(struct d_contramap_comparator* _binding,
                                     struct d_extractor             _extract,
                                     fn_binary_predicate            _precedes,
                                     void*                          _context,
                                     void*                          _left,
                                     void*                          _right);
bool     d_contramap_precedes(const void* _left,
                              const void* _right,
                              void*       _binding);

// IV.    layout assertions (Layout law: one declaration, asserted in both)
D_STATIC_ASSERT(offsetof(struct d_extractor, context) == sizeof(fn_transformer),
                "d_extractor layout drift");
D_STATIC_ASSERT(offsetof(struct d_contramap_predicate, predicate) ==
                    sizeof(struct d_extractor),
                "d_contramap_predicate layout drift");


#endif  // DJINTERP_C_FUNCTIONAL_EXTRACTOR_H
