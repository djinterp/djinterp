/*******************************************************************************
* djinterp [core]                                                  recursion.hpp
*
* Recursion schemes over a Functor: mu, cata, ana, hylo.
*   A recursive datatype is the least fixed point of a functor:
* D ≅ μF, where one "layer" of structure is F(X) and the recursive
* positions are the X-holes.  Given
*
*     project : D → F(D)          peel one layer (children still D)
*     embed   : F(D) → D          build one layer
*
* every fold and unfold over D factors through F:
*
*     cata[φ] : μF → A            φ : F(A) → A     (a fold / catamorphism)
*     ana[ψ]  : A → μF            ψ : A → F(A)     (an unfold / anamorphism)
*     hylo    : A → B             refold, no μF materialised in between
*
*   cata is the universal fold: it peels a layer with project, folds
* every child recursively to an A, then collapses the resulting F(A)
* with the algebra φ.  ana is its mirror.  Because both are expressed
* purely through F's Functor instance (functor_map) plus project /
* embed, ONE definition folds every recursive type in the framework
* — a parse tree, an expression IR, an annotated AST — and the only
* thing that varies between "render", "evaluate", "type-check", and
* "optimise" is the algebra handed in.  That is the whole point: the
* structural recursion is written once, here; each interpretation is
* an F-algebra written elsewhere.
*
*   STRUCTURE MAP, NOT INHERITANCE.  A type C is made foldable by
* specialising recursive_traits<C> with its base functor and a
* project; it is made buildable by specialising corecursive_traits<C>
* with embed.  A native carrier (say std::vector) participates
* without being rewritten as a fixed point — project just exposes
* its head/tail as an F-layer.  The canonical fixed point mu<F> is
* provided here with blanket instances (project = out, embed = In),
* so code that wants an explicit initial algebra has one, and code
* that has its own recursive type keeps it.
*
*   RELATION TO THE OTHER FOLDS.  foldable.hpp's fold_left folds a
* *sequence* (one spine); free.hpp's fold_free interprets a *free
* monad* (a program tree with a fixed Pure/Roll shape); cofree.hpp
* is the dual annotated tree.  cata here is the *structural* fold
* over an arbitrary μF and subsumes the shape-specific ones: a
* sequence is μ(1 + A×X), a free monad is μ over its instruction
* functor.  The three coexist because the specific folds are cheaper
* to state for their shapes; cata is the general instrument.
*
* CONTENTS
*   I.    mu<F>                       the least fixed point μF
*   II.   recursive_traits<C>         base functor + project (fold side)
*         corecursive_traits<C>       base functor + embed   (build side)
*   III.  is_recursive / is_corecursive + concepts
*   IV.   recursive_traits<mu<F>> / corecursive_traits<mu<F>>  instances
*   V.    cata[φ]                     the universal fold
*   VI.   ana[ψ]                      the universal unfold
*   VII.  hylo                        refold (cata ∘ ana, deforested)
*
*
* path:      /inc/djinterp/core/functional/recursion.hpp
* link(s):   ch-parsing.tex, ch-recursion.tex
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.30
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_RECURSION_HPP
#define DJINTERP_FUNCTIONAL_RECURSION_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <memory>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "./functor.hpp"


NS_DJINTERP


// ================================================================
//  I.   mu<F>  —  the least fixed point μF
// ================================================================

// mu
//   class: the least fixed point of the functor F.  A value is one
// F-layer whose recursive holes contain further mu<F> values —
// In : F(μF) → μF is the initial algebra, out : μF → F(μF) its
// inverse.  The layer is held behind a shared_ptr so the recursive
// type has finite size and a type-erased destructor (which lets the
// class be named before F<mu<F>> is complete — the standard
// fixed-point encoding).
//
//   F is a single-argument template-template parameter (the same
// shape functional::free and the poly_* functors use), and must be
// a registered Functor for mu<F> to be foldable via cata.
template<template<typename> class F>
class mu
{
public:
    // layer_type
    //   alias: one unrolled layer, F(μF).  Naming this type does
    // not instantiate F<mu<F>> — instantiation is deferred to In
    // / out bodies, by which point mu<F> is complete.
    using layer_type = F<mu<F> >;

    mu()
        : m_out()
    {}

    // In
    //   factory: the initial algebra F(μF) → μF.  Boxes one layer.
    D_NODISCARD
    static mu
    In(
        const layer_type& _layer
    )
    {
        mu _m;
        _m.m_out = std::make_shared<layer_type>(_layer);
        return _m;
    }

    // out
    //   accessor: the anamorphism's structure map μF → F(μF).
    // Precondition: the value is non-empty (constructed via In).
    D_NODISCARD
    const layer_type&
    out() const
    {
        return (*m_out);
    }

    // empty
    //   method: true for a default-constructed mu (no layer).
    D_NODISCARD
    bool
    empty() const D_NOEXCEPT
    {
        return (!m_out);
    }

private:
    std::shared_ptr<layer_type> m_out;
};


// ================================================================
//  II.  recursive_traits / corecursive_traits
// ================================================================

// recursive_traits
//   trait: the fold-side structure of a recursive carrier C.  A
// conforming specialisation provides
//
//     using is_specialized = std::true_type;
//     template<typename A> using base = F<A>;   // the base functor
//     static base<C> project(const C&);         // peel one layer
//
// where F is the base functor (F(A) with the recursive holes at A)
// and project exposes C as one F-layer whose children are still
// C.  base<A> must be a registered Functor.  The primary is left
// undefined so cata on a non-recursive type is a clean error.
template<typename Carrier,
         typename Enable = void>
struct recursive_traits;


// corecursive_traits
//   trait: the build-side structure of a carrier C.  A conforming
// specialisation provides
//
//     using is_specialized = std::true_type;
//     template<typename A> using base = F<A>;
//     static C embed(const base<C>&);           // build one layer
//
// the mirror of recursive_traits: embed assembles a C from one
// F-layer whose children are already C.  Used by ana.
template<typename Carrier,
         typename Enable = void>
struct corecursive_traits;


// ================================================================
//  III. is_recursive / is_corecursive + concepts
// ================================================================

NS_INTERNAL

    // is_recursive_helper
    template<typename Type>
    struct is_recursive_helper
    {
    private:
        template<typename T>
        static auto test(int)
            -> decltype(
                typename recursive_traits<T>::is_specialized{},
                std::true_type{});

        template<typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<Type>(0));
    };

    // is_corecursive_helper
    template<typename Type>
    struct is_corecursive_helper
    {
    private:
        template<typename T>
        static auto test(int)
            -> decltype(
                typename corecursive_traits<T>::is_specialized{},
                std::true_type{});

        template<typename>
        static std::false_type test(...);

    public:
        using type = decltype(test<Type>(0));
    };

NS_END  // internal

// is_recursive
//   trait: true iff Type has a recursive_traits specialisation.
template<typename Type>
struct is_recursive
    : internal::is_recursive_helper<
          typename std::decay<Type>::type>::type
{};

// is_corecursive
//   trait: true iff Type has a corecursive_traits specialisation.
template<typename Type>
struct is_corecursive
    : internal::is_corecursive_helper<
          typename std::decay<Type>::type>::type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    static constexpr bool is_recursive_v =
        is_recursive<Type>::value;

    template<typename Type>
    static constexpr bool is_corecursive_v =
        is_corecursive<Type>::value;
#endif

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // Recursive
    //   concept: Type has a fold-side structure map.
    template<typename Type>
    concept Recursive = is_recursive<Type>::value;

    // Corecursive
    //   concept: Type has a build-side structure map.
    template<typename Type>
    concept Corecursive = is_corecursive<Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// ================================================================
//  IV.  mu<F> instances
// ================================================================

// recursive_traits<mu<F>>
//   specialisation: the canonical fixed point folds via out.
template<template<typename> class F>
struct recursive_traits<mu<F>, void>
{
    using is_specialized = std::true_type;

    template<typename A>
    using base = F<A>;

    static
    F<mu<F> >
    project(
        const mu<F>& _m
    )
    {
        return _m.out();
    }
};

// corecursive_traits<mu<F>>
//   specialisation: the canonical fixed point builds via In.
template<template<typename> class F>
struct corecursive_traits<mu<F>, void>
{
    using is_specialized = std::true_type;

    template<typename A>
    using base = F<A>;

    static
    mu<F>
    embed(
        const F<mu<F> >& _layer
    )
    {
        return mu<F>::In(_layer);
    }
};


// ================================================================
//  V.   cata[φ]  —  the universal fold
// ================================================================

// cata
//   function: the catamorphism cata[φ] : μF → A for an F-algebra
// φ : F(A) → A.  Peels one layer with project, folds every child
// recursively to an A (rebuilding the layer at A), then collapses
// with φ.
//
//       cata[φ] = φ ∘ fmap(cata[φ]) ∘ project
//
//   Result (A) is supplied explicitly so the algebra's carrier is
// pinned without a circular deduction; Carrier and Algebra follow
// from the arguments.  The algebra may be any callable
// F(A) → A — a lambda, a functor object, or a free function.
//
//   Requires recursive_traits<Carrier> and that the base functor
// applied to Result is a registered Functor (so functor_map can
// rebuild the layer).
template<typename Result,
         typename Carrier,
         typename Algebra>
D_NODISCARD
Result
cata(
    Algebra         _algebra,
    const Carrier& _value
)
{
    using rec = recursive_traits<Carrier>;

    // peel: F(Carrier)
    typename rec::template base<Carrier> layer =
        rec::project(_value);

    // fold children: F(Carrier) --fmap(cata)--> F(Result)
    auto folded =
        ::djinterp::functor_map(
            layer,
            [_algebra](const Carrier& _child) -> Result
            {
                return ::djinterp::cata<Result, Carrier, Algebra>(
                    _algebra, _child);
            });

    // collapse this layer with the algebra
    return _algebra(folded);
}


// ================================================================
//  VI.  ana[ψ]  —  the universal unfold
// ================================================================

// ana
//   function: the anamorphism ana[ψ] : A → μF for an F-coalgebra
// ψ : A → F(A).  Runs the coalgebra to obtain one seeded layer,
// expands every seed recursively into a Carrier, then builds the
// layer with embed.
//
//       ana[ψ] = embed ∘ fmap(ana[ψ]) ∘ ψ
//
//   Carrier (the μF being built) is supplied explicitly; Seed and
// Coalgebra follow from the arguments.  This is the formal dual of
// cata, and the total counterpart of a parser: `parse` is the
// *partial* unfold Σ* ⇀ μF (defined only on the language), whereas
// ana over a total coalgebra always produces a value.
//
//   Requires corecursive_traits<Carrier> and that the base functor
// applied to Carrier is a registered Functor.
template<typename Carrier,
         typename Seed,
         typename Coalgebra>
D_NODISCARD
Carrier
ana(
    Coalgebra    _coalgebra,
    const Seed& _seed
)
{
    using corec = corecursive_traits<Carrier>;

    // seed one layer: F(Seed)
    typename corec::template base<Seed> layer =
        _coalgebra(_seed);

    // expand seeds: F(Seed) --fmap(ana)--> F(Carrier)
    auto expanded =
        ::djinterp::functor_map(
            layer,
            [_coalgebra](const Seed& _s) -> Carrier
            {
                return ::djinterp::ana<Carrier, Seed, Coalgebra>(
                    _coalgebra, _s);
            });

    // build this layer
    return corec::embed(expanded);
}


// ================================================================
//  VII. hylo  —  refold without materialising μF
// ================================================================

// hylo
//   function: the hylomorphism hylo[φ, ψ] : A → B — an ana
// immediately consumed by a cata, fused so no intermediate μF is
// built (deforestation).  Given an F-coalgebra ψ : A → F(A) and an
// F-algebra φ : F(B) → B,
//
//       hylo[φ, ψ] = φ ∘ fmap(hylo[φ, ψ]) ∘ ψ
//
//   The base functor F is supplied explicitly as a template-
// template parameter because no carrier is present to recover it
// from; Result (B), Seed (A) and the two operations follow from
// the arguments.  Termination rests on ψ eventually producing
// leaf layers (no recursive holes), exactly as for ana.
template<typename Result,
         typename Seed,
         template<typename> class F,
         typename Algebra,
         typename Coalgebra>
D_NODISCARD
Result
hylo(
    Algebra      _algebra,
    Coalgebra    _coalgebra,
    const Seed& _seed
)
{
    // seed one layer: F(Seed)
    F<Seed> layer = _coalgebra(_seed);

    // refold children directly to Result: F(Seed) --fmap(hylo)--> F(Result)
    auto folded =
        ::djinterp::functor_map(
            layer,
            [_algebra, _coalgebra](const Seed& _s) -> Result
            {
                return ::djinterp::hylo<
                    Result, Seed, F, Algebra, Coalgebra>(
                        _algebra, _coalgebra, _s);
            });

    return _algebra(folded);
}


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_FUNCTIONAL_RECURSION_HPP
