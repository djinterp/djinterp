/*******************************************************************************
* djinterp [c]                                                         functor.h
*
* Tier 3, completed: Functor, Applicative and Monad as runtime dictionaries.
*
* A FINDING THAT CHANGES THE TIER-3 ESTIMATE. `parity_functional_types.md` rates
* these three harder than Foldable and Alternative because `map` "must name
* `F<A>`, `F<B>` generically". That is true of the C++ representation, where the
* element type is a template parameter and `maybe<int>` and `maybe<char>` are
* different types. It is NOT true of the representation tier 2 actually built: a
* `struct d_maybe` is a VIEW carrying its element width at runtime, so `F<A>` and
* `F<B>` are the same C type holding different widths, and `map` is
* `(const struct d_maybe*, struct d_maybe*, fn_transformer, void*)` -- which is
* the shape `d_maybe_map` already had.
*   So Functor, Applicative and Monad cost the same as Foldable here, and the
* macro monomorphisation the assessment offers as encoding (a) is not needed for
* them. What is genuinely lost is static type safety: nothing checks that the
* carrier handed to a dictionary is the one the dictionary expects. That cost is
* uniform across all five protocols and is stated once, here, rather than being
* discovered per-protocol.
*   The practical consequence is that `free<F,A>` is NOT blocked on an unmade
* encoding decision. It needs a functor, and a functor is a dictionary.
*
* APPLICATIVE SUPPLIES `pure` AND `map2`, NOT `ap`. Table 7.3 gives the
* operations as `pure` and `ap : F<A->B> -> F<A> -> F<B>`. The literal `ap`
* needs a carrier holding a FUNCTION, which C can express -- a `d_maybe` over an
* `fn_transformer` -- but which no caller wants to build. `map2`
* (`F<A> x F<B> -> F<C>`, Haskell's `liftA2`) is interdefinable with `ap` given
* Functor, is what applicative style is actually used for, and needs no carrier
* of function pointers. The deviation is in the presentation, not the algebra.
*
*
* path:      /inc/djinterp/c/functional/functor.h
* link(s):   TBA
* author(s): TBA                                             created: 2026.07.30
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_C_FUNCTIONAL_FUNCTOR_H
#define DJINTERP_C_FUNCTIONAL_FUNCTOR_H 1

// std
#include <stddef.h>
// djinterp
#include "../djinterp.h"
#include "./functional_common.h"
#include "./reducer.h"
#include "./maybe.h"
#include "./result.h"
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


// fn_functor_map
//   function pointer: `map : (A -> B) -> F<A> -> F<B>` for one carrier.
// The input and output carriers are the same C type holding different widths.
typedef bool (*fn_functor_map)(const void*    _in,
                               void*          _out,
                               fn_transformer _transform,
                               void*          _context);

// d_functor
//   struct: the Functor dictionary for one carrier.
struct d_functor
{
    fn_functor_map map;     // the structure-preserving mapping
};

// fn_applicative_pure
//   function pointer: `pure : A -> F<A>`, the minimal embedding.
typedef bool (*fn_applicative_pure)(void*       _out,
                                    const void* _value);

// fn_applicative_map2
//   function pointer: `map2 : F<A> x F<B> -> F<C>` under a binary function.
// The zipper runs only when both carriers hold a value; otherwise the result is
// whatever the carrier's own notion of absence is.
typedef bool (*fn_applicative_map2)(const void* _first,
                                    const void* _second,
                                    void*       _out,
                                    fn_zipper  _combine,
                                    void*       _context);

// d_applicative
//   struct: the Applicative dictionary for one carrier.
struct d_applicative
{
    fn_applicative_pure pure;   // the minimal embedding
    fn_applicative_map2 map2;   // the binary lift
};

// fn_monad_bind
//   function pointer: `bind : M<A> -> (A -> M<B>) -> M<B>` for one carrier.
typedef bool (*fn_monad_bind)(const void* _in,
                              void*       _out,
                              fn_kleisli  _arrow,
                              void*       _context);

// d_monad
//   struct: the Monad dictionary for one carrier.
// `unit` is the same operation as Applicative's `pure`; it is repeated here so
// a Monad is usable without also fetching the Applicative dictionary, which is
// the C spelling of the superclass relation C++ states with inheritance.
struct d_monad
{
    fn_applicative_pure unit;   // `A -> M<A>`
    fn_monad_bind       bind;   // sequencing
};

// I.     Functor instances
struct d_functor d_functor_maybe(void);
struct d_functor d_functor_result(void);
bool             d_functor_is_valid(const struct d_functor* _functor);

// II.    Applicative instances
struct d_applicative d_applicative_maybe(void);
struct d_applicative d_applicative_result(void);
bool                 d_applicative_is_valid(
                         const struct d_applicative* _applicative);

// III.   Monad instances
struct d_monad d_monad_maybe(void);
struct d_monad d_monad_result(void);
bool           d_monad_is_valid(const struct d_monad* _monad);

// IV.    generic operations
bool     d_functor_map(const struct d_functor* _functor,
                       const void*             _in,
                       void*                   _out,
                       fn_transformer          _transform,
                       void*                   _context);
bool     d_applicative_pure(const struct d_applicative* _applicative,
                            void*                       _out,
                            const void*                 _value);
bool     d_applicative_map2(const struct d_applicative* _applicative,
                            const void*                 _first,
                            const void*                 _second,
                            void*                       _out,
                            fn_zipper                  _combine,
                            void*                       _context);
bool     d_monad_unit(const struct d_monad* _monad,
                      void*                 _out,
                      const void*           _value);
bool     d_monad_bind(const struct d_monad* _monad,
                      const void*           _in,
                      void*                 _out,
                      fn_kleisli            _arrow,
                      void*                 _context);

// V.     layout assertions (Layout law: one declaration, asserted in both)
D_STATIC_ASSERT(sizeof(struct d_functor) == sizeof(fn_functor_map),
                "d_functor layout drift");
D_STATIC_ASSERT(offsetof(struct d_applicative, map2) ==
                    sizeof(fn_applicative_pure),
                "d_applicative layout drift");
D_STATIC_ASSERT(offsetof(struct d_monad, bind) == sizeof(fn_applicative_pure),
                "d_monad layout drift");


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_FUNCTIONAL_FUNCTOR_H
