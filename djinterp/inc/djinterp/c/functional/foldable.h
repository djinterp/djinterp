/*******************************************************************************
* djinterp [c]                                                        foldable.h
*
* Tier 3, first instalment: Foldable and Alternative as runtime dictionaries.
*   `parity_functional_types.md` frames the tier-3 choice as (a) macro
* monomorphisation or (b) a dictionary of function pointers over `void*`, and
* recommends both at different layers. It also names the right place to start:
* "Alternative and Foldable are the easy members here -- `alt` never changes
* `A`, and `fold_left`'s result leaves `F` entirely -- so they are the right two
* to build first as proof." This header is that proof, in encoding (b).
*
* WHY THESE TWO ARE EASY, PRECISELY. The obstruction at kind `* -> *` is having
* to name `F<A>` and `F<B>` in one signature. Foldable never does: `fold_left`
* returns an `Acc` that has left the carrier entirely, so the dictionary needs
* one operation over one carrier type. Alternative never does either: `alt` is
* `F<A> x F<A> -> F<A>`, so `A` is fixed throughout. Functor, Applicative and
* Monad all change `A`, which is why they are not here -- they need the second
* carrier type named, and that is the decision this instalment is meant to
* inform rather than pre-empt.
*
* ONE OPERATION IS ENOUGH FOR FOLDABLE, because tier 1 already supplies the
* driver. A `d_foldable` says only how to push a carrier's elements at a
* `d_reducer`; everything else -- folds, quantifiers, monoid summaries -- is the
* tier 1 spine applied through that one call. So `d_foldable_mconcat` folds ANY
* carrier under ANY monoid, and neither the carrier nor the monoid knows about
* the other. That is the payoff of the whole three-tier arrangement, and the
* conformance suite asserts it across four carriers.
*
* THE CARRIER IS ALWAYS A `const void*`, and what it points at is fixed per
* instance: `d_foldable_array` takes a `d_array_carrier`, `d_foldable_maybe` a
* `struct d_maybe`, and so on. That pairing is unchecked, which is the price of
* encoding (b) and the reason the assessment recommends monomorphisation for the
* closed set of carriers and dictionaries only at the boundary.
*
*
* path:      /inc/djinterp/c/functional/foldable.h
* link(s):   TBA
* author(s): TBA                                             created: 2026.07.30
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_C_FUNCTIONAL_FOLDABLE_H
#define DJINTERP_C_FUNCTIONAL_FOLDABLE_H 1

// std
#include <stddef.h>
// djinterp
#include "../djinterp.h"
#include "./functional_common.h"
#include "./reducer.h"
#include "./producer.h"
#include "./maybe.h"
#include "./result.h"
#include "./monoid.h"
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


// fn_foldable_drive
//   function pointer: pushes a carrier's elements at a reducer.
// Returns how many were delivered, which may be fewer than the carrier holds if
// the reducing state latched.
typedef size_t (*fn_foldable_drive)(const void*              _carrier,
                                    const struct d_reducer*  _reducer,
                                    struct d_reducing_state* _state);

// d_foldable
//   struct: the Foldable dictionary for one carrier.
// `element_size` is the width of the elements the carrier yields, which the
// generic operations need in order to size accumulators and scratch.
struct d_foldable
{
    fn_foldable_drive drive;        // how to push the carrier's elements
    size_t            element_size; // width of a yielded element
};

// fn_alt
//   function pointer: the Alternative choice `F<A> x F<A> -> F<A>`.
typedef bool (*fn_alt)(const void* _first,
                       const void* _second,
                       void*       _out);

// fn_aempty
//   function pointer: the Alternative identity.
typedef bool (*fn_aempty)(void* _out);

// d_alternative
//   struct: the Alternative dictionary for one carrier.
struct d_alternative
{
    fn_alt    alt;      // the choice
    fn_aempty aempty;   // its two-sided identity
};

// d_array_carrier
//   struct: the carrier a plain array presents to a Foldable dictionary.
// An array is three facts, not one pointer, so the instance needs somewhere to
// keep them.
struct d_array_carrier
{
    const void* elements;      // the first element
    size_t      count;         // how many
    size_t      element_size;  // stride between them
};

// d_producer_carrier
//   struct: the carrier a producer presents to a Foldable dictionary.
// A producer needs scratch to pull into, and the dictionary's one `void*` has
// nowhere else to keep it.
struct d_producer_carrier
{
    struct d_producer* producer;   // the borrowed source
    void*              scratch;    // buffer of at least `value_size` bytes
};

// I.     Foldable instances
struct d_foldable d_foldable_array(size_t _element_size);
struct d_foldable d_foldable_maybe(size_t _element_size);
struct d_foldable d_foldable_result(size_t _element_size);
struct d_foldable d_foldable_producer(size_t _element_size);
bool              d_foldable_is_valid(const struct d_foldable* _foldable);

// II.    generic operations over any Foldable
size_t   d_foldable_drive(const struct d_foldable* _foldable,
                          const void*              _carrier,
                          const struct d_reducer*  _reducer,
                          struct d_reducing_state* _state);
size_t   d_foldable_length(const struct d_foldable* _foldable,
                           const void*              _carrier);
bool     d_foldable_is_empty(const struct d_foldable* _foldable,
                             const void*              _carrier);
bool     d_foldable_fold(const struct d_foldable* _foldable,
                         const void*              _carrier,
                         void*                    _accumulator,
                         fn_fold                  _step,
                         void*                    _context);
bool     d_foldable_any(const struct d_foldable* _foldable,
                        const void*              _carrier,
                        fn_predicate             _predicate,
                        void*                    _context);
bool     d_foldable_all(const struct d_foldable* _foldable,
                        const void*              _carrier,
                        fn_predicate             _predicate,
                        void*                    _context);
bool     d_foldable_mconcat(const struct d_foldable* _foldable,
                            const void*              _carrier,
                            const struct d_monoid*   _monoid,
                            void*                    _out);

// III.   Alternative instances
struct d_alternative d_alternative_maybe(void);
bool                 d_alternative_is_valid(
                         const struct d_alternative* _alternative);

// IV.    generic operations over any Alternative
bool     d_alternative_alt(const struct d_alternative* _alternative,
                           const void*                 _first,
                           const void*                 _second,
                           void*                       _out);
bool     d_alternative_aempty(const struct d_alternative* _alternative,
                              void*                       _out);

// V.     layout assertions (Layout law: one declaration, asserted in both)
D_STATIC_ASSERT(offsetof(struct d_foldable, element_size) ==
                    sizeof(fn_foldable_drive),
                "d_foldable layout drift");
D_STATIC_ASSERT(offsetof(struct d_alternative, aempty) == sizeof(fn_alt),
                "d_alternative layout drift");
D_STATIC_ASSERT(offsetof(struct d_array_carrier, count) == sizeof(void*),
                "d_array_carrier layout drift");


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_FUNCTIONAL_FOLDABLE_H
