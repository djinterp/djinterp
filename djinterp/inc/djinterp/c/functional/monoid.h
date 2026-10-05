/*******************************************************************************
* djinterp [c]                                                          monoid.h
*
* The monoid protocol: a semigroup with a two-sided identity, its
* identity-aware operations, and the standard numeric and boolean instances.
*   A monoid is a semigroup `T` carrying a distinguished `mempty` such that
* `mappend(mempty, x) == mappend(x, mempty) == x`. The identity is what makes an
* *empty* fold answerable: `d_semigroup_reduce` fails on an empty array because
* there is nothing to return, while `d_mconcat` answers `mempty`.
*   The relation "a monoid is a semigroup plus an identity" is expressed here as
* layout, not as documentation: `d_monoid` *contains* a `d_semigroup` as its
* first and only decorated member, at offset 0, so the forgetful map down to the
* semigroup is a cast and costs nothing. `d_monoid_semigroup` performs it, and
* the assertions at the foot of this header pin the cost: containment adds
* exactly one function pointer and no padding.
*
* INSTANCE WIDTHS. C++ spells these as `monoids::sum<T>`. C has no template
* parameter, so each instance names its width, and the widths provided are the
* two that do not lose information: `intmax_t` and `double`. This is the same
* choice `d_producer_range` makes in emitting `intmax_t`, and narrowing is the
* caller's business. A scalar is a monoid in more than one way, so — exactly as
* in the C++ `monoids` namespace — the instance name states which one is meant.
*
* NOT PROVIDED, AND WHY. `std::string` and `std::vector<T>` concatenation are
* monoid instances in the C++ face and are absent here: the C fork has no
* `d_string` and no `d_vector`, and inventing one for this header would be the
* wrong place to introduce it. Concatenation instances are blocked on the
* container substrate, not overlooked. `d_semigroup_first`/`_last` in
* semigroup.h are the deliberate counterexamples: associative, size-generic, and
* with no identity, so they are semigroups and never monoids.
*
*
* path:      /inc/djinterp/c/functional/monoid.h
* link(s):   TBA
* author(s): TBA                                             created: 2026.07.30
*                                                            revised: 2026.10.03
*******************************************************************************/

#ifndef DJINTERP_C_FUNCTIONAL_MONOID_H
#define DJINTERP_C_FUNCTIONAL_MONOID_H 1

// std
#include <float.h>
#include <stddef.h>
// djinterp
#include "../djinterp.h"
#include "./functional_common.h"
#include "./reducer.h"
#include "./producer.h"
#include "./semigroup.h"
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // fixed-width types, for users

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


// d_monoid
//   struct: forward declaration.
// The identity typedef below names this type, and a struct first named inside a
// parameter list would be a distinct, file-local type.
struct d_monoid;

// fn_mempty
//   function pointer: writes the identity element of a monoid.
// Fills `_identity` with `value_size` bytes, reading any instance
// configuration from `_monoid`.
typedef void (*fn_mempty)(const struct d_monoid* _monoid,
                          void*                  _identity);

// d_monoid
//   struct: a semigroup decorated with a two-sided identity.
// The contained semigroup is the first member, so the forgetful map to it is a
// cast at offset 0 and a monoid may be passed wherever a semigroup is wanted.
struct d_monoid
{
    struct d_semigroup semigroup;  // the underlying combine, at offset 0
    fn_mempty          mempty;     // writes the identity element
};

// I.     construction
struct d_monoid d_monoid_make(struct d_semigroup _semigroup,
                              fn_mempty          _mempty);
bool            d_monoid_is_valid(const struct d_monoid* _monoid);

// II.    forgetful map
const struct d_semigroup* d_monoid_semigroup(const struct d_monoid* _monoid);

// III.   operations
bool     d_mempty(const struct d_monoid* _monoid,
                  void*                  _identity);
bool     d_mconcat(const struct d_monoid* _monoid,
                   const void*            _elements,
                   size_t                 _count,
                   void*                  _out);
bool     d_mconcat_producer(const struct d_monoid* _monoid,
                            struct d_producer*     _producer,
                            void*                  _out,
                            void*                  _scratch);
bool     d_fold_monoid(const struct d_monoid* _monoid,
                       const void*            _elements,
                       size_t                 _count,
                       size_t                 _element_size,
                       fn_transformer         _to_monoid,
                       void*                  _context,
                       void*                  _out,
                       void*                  _scratch);

// IV.    reducer conversion
struct d_reducer d_monoid_reducer(const struct d_monoid* _monoid);

// V.     integer instances (intmax_t)
struct d_monoid d_monoid_sum_intmax(void);
struct d_monoid d_monoid_product_intmax(void);
struct d_monoid d_monoid_min_intmax(void);
struct d_monoid d_monoid_max_intmax(void);

// VI.    floating-point instances (double)
struct d_monoid d_monoid_sum_double(void);
struct d_monoid d_monoid_product_double(void);
struct d_monoid d_monoid_min_double(void);
struct d_monoid d_monoid_max_double(void);

// VII.   boolean instances
struct d_monoid d_monoid_all(void);
struct d_monoid d_monoid_any(void);

// VIII.  layout and cost assertions (Layout law and Cost law)
D_STATIC_ASSERT(offsetof(struct d_monoid, semigroup) == 0,
                "a monoid must forget to its semigroup at offset 0");
D_STATIC_ASSERT(offsetof(struct d_monoid, mempty) == sizeof(struct d_semigroup),
                "d_monoid layout drift");
D_STATIC_ASSERT(sizeof(struct d_monoid) ==
                    (sizeof(struct d_semigroup) + sizeof(fn_mempty)),
                "decorating a semigroup must cost exactly one pointer");


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_FUNCTIONAL_MONOID_H
