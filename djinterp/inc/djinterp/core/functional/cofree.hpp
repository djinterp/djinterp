/*******************************************************************************
* djinterp [core]                                                     cofree.hpp
*
* Cofree comonad: turn any functor into a comonad (C++).
*   The cofree comonad over a functor F is the dual of the free monad -- where
* Free was Pure-or-Roll, Cofree is *always* a value paired with a layer:
*
*     Cofree<F, A> = A :< F (Cofree<F, A>)
*
* Every node carries a head value of type A and a layer of F holding child
* nodes; there is no empty case, so a focus can always be read. It models an
* annotated / labelled tree (or, with F = maybe, a non-empty stream): the value
* "here", and the F-shaped continuations "around". Given any F that is a
* Functor, Cofree<F, _> is a Comonad, with extract reading the head and extend
* re-decorating every node from its whole sub-tree.
*
*   As with Free, C++'s lack of higher-kinded types means F is a single-argument
* template-template parameter and the self-reference is carried by
* std::shared_ptr (shared_ptr<cofree> is complete while cofree is being
* defined), so the tree lives on the heap and is not constexpr. F must be a
* registered Functor (functor_traits), since map, extend, and the unfold builder
* all map over the F layer.
*
*   Cofree<F, A> registers comonad_traits, functor_traits, and recursive_traits,
* so the generic extract / extend / duplicate (comonad.hpp), functor_map, and
* the universal cata (recursion.hpp) all work on it directly. unfold_cofree
* coiteratively builds a cofree from a seed -- the dual of fold_free
* interpreting one.
*
*   FOLDABLE, TOO: THE ANNOTATED LAYER. One unrolled layer of a cofree is its
* annotation paired with an F-layer of children -- the environment pairing
*
*     env_layer<B, F, X>  =  B x F<X>
*
* which is exactly the base functor of cofree<F, B>. Registering
* recursive_traits<cofree<F, B>> over that base is what lets the universal cata
* fold an ANNOTATED tree, the case recursion.hpp's own "an annotated AST" refers
* to; annotated_cata is the ergonomic face, taking a binary algebra
* (B, F<R>) -> R so a node's annotation and its already-folded children arrive
* side by side. One fold, many algebras -- now annotated: render, number,
* collect, and type-check over a labelled tree are each one annotated_cata with
* a different algebra, the structural recursion still written once.
*
*   WHY THESE LIVE HERE. A trait specialisation belongs with the type it
* specialises or with the trait itself, never in a third header that may or may
* not be in the include set: a translation unit seeing cofree.hpp must get the
* same is_recursive<cofree<...>> answer as every other, and a separate
* registration header cannot guarantee that. Keeping all three of cofree's
* instances in one place also matches how the rest of the layer is arranged --
* recursion.hpp registers mu's traits, cofree.hpp registers cofree's.
*
*   The env_layer machinery itself is generic: it names no client, so any
* functor F gains a foldable annotated tree from this one header.
*
* USAGE:
*   using namespace djinterp;
*   // build a non-empty descending chain with F = maybe: 3 :< just(2 :< just(1 :< nothing))
*   struct head_of { int operator()(int s) const { return s; } };
*   struct next_of { maybe<int> operator()(int s) const {
*       return s > 1 ? just(s - 1) : nothing<int>(); } };
*   cofree<maybe, int> chain = unfold_cofree<maybe>(3, head_of{}, next_of{});
*
*   int here = extract(chain);                    // 3
*   auto deeper = duplicate(chain);               // cofree<maybe, cofree<maybe,int>>
*   auto doubled = functor_map(chain, [](int x){ return x * 2; });  // 6 :< just(4 :< ...)
*
*   // fold the annotated tree: sum the heads, children already folded to int
*   int total = annotated_cata<int>(
*       [](int _head, const maybe<int>& _folded) -> int
*       {
*           return _head + (_folded.has_value() ? _folded.value() : 0);
*       },
*       chain);                                   // 6
*
*
* path:      /inc/djinterp/core/functional/cofree.hpp
* link(s):   ch-recursion.tex
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.12
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    THE COFREE COMONAD TYPE                      (cofree<F, A>)
      -----------------------------------------------------------

II.   COFREE OPERATIONS                             (map / extend / unfold)
      ---------------------------------------------------------------------
      1.    forward declarations
      2.    per-layer step helpers                    (internal)
      3.    cofree_map / cofree_extend / unfold_cofree

III.  TYPECLASS REGISTRATION                        (functor_traits, comonad_traits)
      ------------------------------------------------------------------------------

IV.   THE ANNOTATED LAYER                           (env_layer<B, F, X> + its Functor)
      --------------------------------------------------------------------------------

V.    RECURSION REGISTRATION                        (recursive_traits<cofree<F, B>>)
      ------------------------------------------------------------------------------

VI.   ANNOTATED FOLD                                (annotated_cata)
      --------------------------------------------------------------
*/


#ifndef DJINTERP_FUNCTIONAL_COFREE_HPP
#define DJINTERP_FUNCTIONAL_COFREE_HPP 1

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
#include "./comonad.hpp"
#include "./recursion.hpp"   // recursive_traits, cata -- sections V and VI


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    THE COFREE COMONAD TYPE  (cofree<F, A>)               ///
///////////////////////////////////////////////////////////////////////////////

// cofree
//   class: the cofree comonad over a single-argument functor F. Every node is
// a head value of type A together with a layer F<shared_ptr<cofree>> of child
// nodes -- there is no empty case. A and F<shared_ptr<cofree>> must be
// default-constructible; the self-reference is carried by shared_ptr, so the
// tree lives on the heap.
template<template<typename> class F,
         typename A>
class cofree
{
public:
    using value_type = A;
    using layer_type = F<std::shared_ptr<cofree> >;

    // cofree (default)
    //   a default node (default head, default layer). Public because
    // duplicate yields cofree<F, cofree<F, A>>, whose head member is itself a
    // cofree and must be default-constructible from outside its own class.
    cofree()
        : m_head()
        , m_layer()
    {}

    // make
    //   factory: a node with the given head and layer of children.
    static
    cofree make(
        const A&         _head,
        const layer_type& _layer
    )
    {
        cofree _node;
        _node.m_head  = _head;
        _node.m_layer = _layer;

        return _node;
    }

    // head
    //   the value carried at this node (what extract returns).
    D_NODISCARD
    const A& head() const
    {
        return m_head;
    }

    // unwrap
    //   the layer of child nodes around this one (the "tail").
    D_NODISCARD
    const layer_type& unwrap() const
    {
        return m_layer;
    }

private:
    A          m_head;
    layer_type m_layer;
};


///////////////////////////////////////////////////////////////////////////////
///             II.   COFREE OPERATIONS                                     ///
///////////////////////////////////////////////////////////////////////////////
//   map, extend, and the unfold builder. Each maps over the F-layer
// (functor_map, so F must be a registered Functor) and recurses into the child
// nodes through their shared_ptr. As in free.hpp the step helpers and the
// operations are mutually recursive, so the operations are forward-declared,
// the helpers defined complete, and the operations defined last.

// -- 1. forward declarations ------------------------------------------------

template<template<typename> class F,
         typename A,
         typename Function>
D_NODISCARD
cofree<F, typename std::decay<decltype(std::declval<Function&>()(
                            std::declval<const A&>()))>::type>
cofree_map(const cofree<F, A>& _node, Function _function);

template<template<typename> class F,
         typename A,
         typename Function>
D_NODISCARD
cofree<F, typename std::decay<decltype(std::declval<Function&>()(
                            std::declval<const cofree<F, A>&>()))>::type>
cofree_extend(const cofree<F, A>& _node, Function _function);

template<template<typename> class F,
         typename Seed,
         typename HeadFn,
         typename LayerFn>
D_NODISCARD
cofree<F, typename std::decay<decltype(std::declval<HeadFn&>()(
                            std::declval<const Seed&>()))>::type>
unfold_cofree(const Seed& _seed, HeadFn _head_fn, LayerFn _layer_fn);


// -- 2. per-layer step helpers (internal) -----------------------------------

NS_INTERNAL

    // cofree_map_step
    //   helper: per-child step of cofree_map -- recurse and re-wrap.
    template<template<typename> class F,
             typename A,
             typename B,
             typename Function>
    struct cofree_map_step
    {
        Function function;

        std::shared_ptr<cofree<F, B> > operator()(
            const std::shared_ptr<cofree<F, A> >& _child
        ) const
        {
            return std::make_shared<cofree<F, B> >(
                ::djinterp::cofree_map(*_child, function));
        }
    };

    // cofree_extend_step
    //   helper: per-child step of cofree_extend -- recurse with the whole-node
    // function.
    template<template<typename> class F,
             typename A,
             typename B,
             typename Function>
    struct cofree_extend_step
    {
        Function function;

        std::shared_ptr<cofree<F, B> > operator()(
            const std::shared_ptr<cofree<F, A> >& _child
        ) const
        {
            return std::make_shared<cofree<F, B> >(
                ::djinterp::cofree_extend(*_child, function));
        }
    };

    // cofree_unfold_step
    //   helper: per-seed step of unfold_cofree -- grow a child node from a
    // seed. Holds both builder functions.
    template<template<typename> class F,
             typename Seed,
             typename A,
             typename HeadFn,
             typename LayerFn>
    struct cofree_unfold_step
    {
        HeadFn   head_fn;
        LayerFn layer_fn;

        std::shared_ptr<cofree<F, A> > operator()(
            const Seed& _seed
        ) const
        {
            return std::make_shared<cofree<F, A> >(
                ::djinterp::unfold_cofree<F>(_seed, head_fn, layer_fn));
        }
    };

NS_END  // internal


// -- 3. operation definitions -----------------------------------------------

// cofree_map
//   function: functorial map -- (A -> B) applied to the head of every node,
// the F-layers preserved. cofree<F, B> from cofree<F, A>.
template<template<typename> class F,
         typename A,
         typename Function>
D_NODISCARD
cofree<F, typename std::decay<decltype(std::declval<Function&>()(
                            std::declval<const A&>()))>::type>
cofree_map
(
    const cofree<F, A>& _node,
    Function              _function
)
{
    using mapped_t = typename std::decay<decltype(
        std::declval<Function&>()(std::declval<const A&>()))>::type;

    return cofree<F, mapped_t>::make(
        _function(_node.head()),
        ::djinterp::functor_map(
            _node.unwrap(),
            internal::cofree_map_step<F, A, mapped_t, Function>{_function}));
}


// cofree_extend
//   function: the co-bind -- re-decorates every node with f applied to that
// node's whole sub-tree (f : cofree<F, A> -> B). cofree<F, B> from
// cofree<F, A>. The head becomes f(node); each child is extended in turn.
template<template<typename> class F,
         typename A,
         typename Function>
D_NODISCARD
cofree<F, typename std::decay<decltype(std::declval<Function&>()(
                            std::declval<const cofree<F, A>&>()))>::type>
cofree_extend
(
    const cofree<F, A>& _node,
    Function              _function
)
{
    using mapped_t = typename std::decay<decltype(
        std::declval<Function&>()(std::declval<const cofree<F, A>&>()))>::type;

    return cofree<F, mapped_t>::make(
        _function(_node),
        ::djinterp::functor_map(
            _node.unwrap(),
            internal::cofree_extend_step<F, A, mapped_t, Function>{
                _function}));
}


// unfold_cofree
//   function: coiteratively builds a cofree from a seed -- _head_fn : S -> A
// gives each node's value, _layer_fn : S -> F<S> gives the seeds of its
// children. The dual of fold_free. Terminates iff _layer_fn eventually yields
// an empty F-layer (e.g. nothing, for F = maybe).
template<template<typename> class F,
         typename Seed,
         typename HeadFn,
         typename LayerFn>
D_NODISCARD
cofree<F, typename std::decay<decltype(std::declval<HeadFn&>()(
                            std::declval<const Seed&>()))>::type>
unfold_cofree
(
    const Seed& _seed,
    HeadFn       _head_fn,
    LayerFn      _layer_fn
)
{
    using head_t = typename std::decay<decltype(
        std::declval<HeadFn&>()(std::declval<const Seed&>()))>::type;

    return cofree<F, head_t>::make(
        _head_fn(_seed),
        ::djinterp::functor_map(
            _layer_fn(_seed),
            internal::cofree_unfold_step<F, Seed, head_t, HeadFn, LayerFn>{
                _head_fn, _layer_fn}));
}


///////////////////////////////////////////////////////////////////////////////
///             III.  TYPECLASS REGISTRATION                                ///
///////////////////////////////////////////////////////////////////////////////
//   cofree<F, A> is a Functor and a Comonad. Comonad is not derivable from any
// monad bridge, so both are registered explicitly -- making the generic
// functor_map and extract / extend / duplicate apply to a cofree directly.
// Its third instance, recursive_traits, needs the base functor of section IV
// and so is registered in section V.

// functor_traits<cofree<F, A>>
template<template<typename> class F,
         typename A>
struct functor_traits<cofree<F, A>, void>
{
    using is_specialized = std::true_type;
    using value_type     = A;

    template<typename To>
    using rebind = cofree<F, To>;

    template<typename Cofree,
             typename Function>
    static
    auto map(
        Cofree&& _node,
        Function _function
    )
    -> decltype(::djinterp::cofree_map(_node, _function))
    {
        return ::djinterp::cofree_map(_node, _function);
    }
};


// comonad_traits<cofree<F, A>>
template<template<typename> class F,
         typename A>
struct comonad_traits<cofree<F, A>, void>
{
    using is_specialized = std::true_type;
    using value_type     = A;

    static
    A extract(
        const cofree<F, A>& _node
    )
    {
        return _node.head();
    }

    template<typename Cofree,
             typename Function>
    static
    auto extend(
        Cofree&& _node,
        Function _function
    )
    -> decltype(::djinterp::cofree_extend(_node, _function))
    {
        return ::djinterp::cofree_extend(_node, _function);
    }
};


///////////////////////////////////////////////////////////////////////////////
///             IV.   THE ANNOTATED LAYER  (env_layer<B, F, X>)             ///
///////////////////////////////////////////////////////////////////////////////
//   The base functor of cofree: one unrolled layer is an annotation paired with
// an F-layer whose recursive holes are still open. Mapping it maps ONLY those
// holes -- the annotation is structure carried alongside, not contents -- which
// is what lets cata rebuild an annotated layer at the result type while it
// folds. Generic in F, so nothing here names a client.

// env_layer
//   struct: an annotation `ann` of type B paired with an F-layer whose
// recursive holes are X. The fixed point of env_layer<B, F, _> re-adds an
// annotation at every node, which is precisely what cofree does (the "EnvT"
// pairing). A plain aggregate, so it is cheap and has no invariants of its own.
template<typename                 B,
         template<typename> class F,
         typename                 X>
struct env_layer
{
    B       ann;
    F<X>  layer;
};


// functor_traits<env_layer<B, F, X>>
//   applies the function to each recursive hole (through the inner F-layer),
// yielding an env_layer over the mapped hole type; the annotation passes
// through unchanged. Delegates to functor_map on the inner layer, so F must
// itself be a registered Functor -- the same demand cofree already makes.
template<typename                 B,
         template<typename> class F,
         typename                 X>
struct functor_traits<env_layer<B, F, X>, void>
{
    using is_specialized = std::true_type;
    using value_type     = X;

    template<typename To>
    using rebind = env_layer<B, F, To>;

    template<typename Env,
             typename Function>
    static
    env_layer<B, F, typename std::decay<decltype(std::declval<Function&>()(
        std::declval<const X&>()))>::type>
    map(
        Env&&    _env,
        Function _function
    )
    {
        using to_type = typename std::decay<decltype(
            std::declval<Function&>()(std::declval<const X&>()))>::type;

        return env_layer<B, F, to_type>{
            _env.ann,
            ::djinterp::functor_map(_env.layer, _function) };
    }
};


///////////////////////////////////////////////////////////////////////////////
///             V.    RECURSION REGISTRATION                                ///
///////////////////////////////////////////////////////////////////////////////
//   Register cofree as a foldable carrier: its base functor is env_layer, and
// project reads one node as its head paired with its layer, dereferencing the
// shared_ptr children so the recursive holes become bare cofree values (what
// cata expects). With this, cata<R>(phi, some_cofree) works for any registered
// F, with phi : env_layer<B, F, R> -> R.

// recursive_traits<cofree<F, B>>
template<template<typename> class F,
         typename                 B>
struct recursive_traits<cofree<F, B>, void>
{
    using is_specialized = std::true_type;

    template<typename X>
    using base = env_layer<B, F, X>;

    // project
    //   peel one layer: (head, F(cofree)). cofree stores its children as
    // shared_ptr<cofree>, so the layer is mapped through a dereference to
    // present bare cofree holes -- the recursive positions cata folds.
    static
    env_layer<B, F, cofree<F, B> >
    project(
        const cofree<F, B>& _node
    )
    {
        return env_layer<B, F, cofree<F, B> >{
            _node.head(),
            ::djinterp::functor_map(
                _node.unwrap(),
                [](const std::shared_ptr<cofree<F, B> >& _child)
                    -> cofree<F, B>
                {
                    return (*_child);
                }) };
    }
};


///////////////////////////////////////////////////////////////////////////////
///             VI.   ANNOTATED FOLD                                        ///
///////////////////////////////////////////////////////////////////////////////

// annotated_cata
//   function: fold a cofree<F, B> with a BINARY algebra
// phi : (B, F<Result>) -> Result. A thin face over cata: it wraps phi so it
// receives the annotation and the folded layer separately instead of a single
// env_layer, which is how every annotated interpretation wants to read a node
// -- "given this node's annotation and its already-folded children, produce a
// result". Result is supplied explicitly (as for cata); F and B follow from
// the cofree, and the algebra from its call.
template<typename                 Result,
         template<typename> class F,
         typename                 B,
         typename                 BinaryAlgebra>
D_NODISCARD
Result
annotated_cata(
    BinaryAlgebra         _algebra,
    const cofree<F, B>& _value
)
{
    return ::djinterp::cata<Result>(
        [_algebra](const env_layer<B, F, Result>& _env) -> Result
        {
            // _env.layer is F<Result> -- the children already folded to R
            return _algebra(_env.ann, _env.layer);
        },
        _value);
}


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_FUNCTIONAL_COFREE_HPP
