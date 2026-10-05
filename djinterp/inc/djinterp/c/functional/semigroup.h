/*******************************************************************************
* djinterp [c]                                                       semigroup.h
*
* The semigroup protocol: one associative binary combine, and no identity.
*   A semigroup is a value type together with an associative operation
* `T x T -> T`. Where the fold unified "collapse a sequence" and the transducer
* unified "reshape a stream", the semigroup unifies the *combine*: numeric
* addition, taking a maximum, keeping the first of two values, and appending one
* buffer to another are all one operation under different names.
*   The operation is spelled `mappend` rather than `combine`, matching the C++
* face, because `combine` is already taken by the accumulator family's variadic
* parallel folds. Associativity is the only law, and it is the caller's promise:
* nothing here can check it, so test_functional_laws.c asserts it per instance.
*
* THREE DEPARTURES from a bare `void (*)(void*, const void*, const void*)`,
* each forced by C lacking the C++ face's template parameter:
*   1. `mappend` takes the semigroup as its first parameter. A C combine works
*      in bytes, so it needs the width, and the width cannot be a template
*      argument. Passing the receiver is how `fn_transducer_step` already
*      solves the same problem, and it lets `first` and `last` be genuinely
*      size-generic. The rejected alternative was a file-static width, which
*      two widths in one program would clobber.
*   2. `d_semigroup` carries `value_size`, for the same reason. This is the
*      framework's byte-level determinacy requirement, not a convenience.
*   3. `_result` is permitted to alias `_left`. That is what makes a semigroup
*      step *be* a fold step: `mappend(sg, acc, acc, x)` is exactly
*      `fn_fold(acc, x)`, so `d_semigroup_reducer` needs no scratch buffer and
*      no per-element copy. Every instance here honours it, and any new
*      instance must.
*
* Identity-bearing semigroups (monoids) and the standard numeric and boolean
* instances live in monoid.h. `first` and `last` are provided here because they
* are the canonical semigroups that have *no* identity, and the conformance
* tests use them to pin that distinction down.
*
*
* path:      /inc/djinterp/c/functional/semigroup.h
* link(s):   TBA
* author(s): TBA                                             created: 2026.07.30
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_C_FUNCTIONAL_SEMIGROUP_H
#define DJINTERP_C_FUNCTIONAL_SEMIGROUP_H 1

// std
#include <stddef.h>
#include <string.h>
// djinterp
#include "../djinterp.h"
#include "./functional_common.h"
#include "./reducer.h"


// d_semigroup
//   struct: forward declaration.
// The combine typedef below names this type, and a struct first named inside a
// parameter list would be a distinct, file-local type.
struct d_semigroup;

// fn_mappend
//   function pointer: the associative combine of a semigroup.
// Writes the combination of `_left` and `_right` into `_result`, reading the
// width and any instance configuration from `_semigroup`. `_result` is
// permitted to alias `_left`, and every implementation must support that.
typedef void (*fn_mappend)(const struct d_semigroup* _semigroup,
                           void*                     _result,
                           const void*               _left,
                           const void*               _right);

// d_semigroup
//   struct: an associative combine, its configuration, and its width.
// A semigroup makes no claim about an identity element; see `d_monoid`.
// `context` is for instances needing configuration beyond the width; an
// instance wanting a callable should point it at a caller-owned struct holding
// that callable, since C does not guarantee a function pointer survives a round
// trip through `void*`.
struct d_semigroup
{
    fn_mappend mappend;     // the associative combine
    void*      context;     // instance configuration; may be NULL
    size_t     value_size;  // byte size of the values combined
};

// I.     construction
struct d_semigroup d_semigroup_make(fn_mappend _mappend,
                                    void*      _context,
                                    size_t     _value_size);
bool               d_semigroup_is_valid(const struct d_semigroup* _semigroup);

// II.    operations
bool     d_mappend(const struct d_semigroup* _semigroup,
                   void*                     _result,
                   const void*               _left,
                   const void*               _right);
bool     d_semigroup_reduce(const struct d_semigroup* _semigroup,
                            const void*               _elements,
                            size_t                    _count,
                            void*                     _accumulator);

// III.   reducer conversion
struct d_reducer d_semigroup_reducer(const struct d_semigroup* _semigroup);

// IV.    size-generic instances having no identity
struct d_semigroup d_semigroup_first(size_t _value_size);
struct d_semigroup d_semigroup_last(size_t _value_size);

// V.     layout assertions (Layout law: one declaration, asserted in both)
D_STATIC_ASSERT(offsetof(struct d_semigroup, context) == sizeof(fn_mappend),
                "d_semigroup layout drift");
D_STATIC_ASSERT(offsetof(struct d_semigroup, value_size) ==
                    (sizeof(fn_mappend) + sizeof(void*)),
                "d_semigroup layout drift");


#endif  // DJINTERP_C_FUNCTIONAL_SEMIGROUP_H
