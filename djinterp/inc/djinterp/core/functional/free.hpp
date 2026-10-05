/*******************************************************************************
* djinterp [core]                                                       free.hpp
*
* Free monad: turn any functor into a monad (C++).
*   The free monad over a functor F is the dual of the cofree comonad. Where
* Cofree is always a value paired with a layer, Free is Pure-or-Roll:
*
*     Free<F, A> = Pure A  |  Roll (F (Free<F, A>))
*
* A value is either a finished result (Pure) or one F-shaped layer whose
* positions hold the rest of the computation (Roll). It models a tree with
* results at its leaves: a program built from F's instructions, or a template
* whose unfilled slots are the leaves. Given any F that is a Functor, Free<F, _>
* is a Monad: bind grafts a continuation onto every Pure leaf.
*   As with Cofree, C++'s lack of higher-kinded types means F is a single-
* argument template-template parameter, and the self-reference is carried by
* std::shared_ptr (complete while free is being defined), so the tree lives on
* the heap and is not constexpr. F must be a registered Functor (functor_traits)
* because fold, bind and lift all map over the F layer.
*   Free<F, A> registers monad_traits, so monad_bind, monad_map and, through the
* monad bridge in functor.hpp, functor_map all work on it directly. fold_free
* interprets a free value against an algebra -- the dual of unfold_cofree
* building a cofree from a seed.
*
*
* path:      /inc/djinterp/core/functional/free.hpp
* link(s):   ch-recursion.tex
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.22
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE FREE MONAD TYPE
    -------------------
    1.  free<F, A>
2.  FREE OPERATIONS
    ---------------
    1.  Interpretation
         1.  fold_free
    2.  Construction
         1.  free_bind
         2.  free_map
         3.  lift_free
3.  TYPECLASS REGISTRATION
    ----------------------
    1.  monad_traits<free<F, A>>
*/

#ifndef DJINTERP_FUNCTIONAL_FREE_HPP
#define DJINTERP_FUNCTIONAL_FREE_HPP 1

// djinterp
#include "../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: this header's floor

// the free monad is built on std::variant and std::in_place_index, both
// C++17; below C++17 the header is absent (degrade, never error)
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>      // std::size_t
#include <memory>       // std::shared_ptr, std::make_shared
#include <type_traits>  // std::decay, std::true_type
#include <utility>      // std::declval, std::in_place_index
#include <variant>      // std::variant, std::get
// djinterp
#include "../../djinterp.hpp"  // framework root
#include "./functor.hpp"       // functor_map, functor_traits
#include "./monad.hpp"         // monad_traits


NS_DJINTERP


//==============================================================================
// 1.  THE FREE MONAD TYPE
//==============================================================================


// 1.1    free<F, A>
//------------------------------------------------------------------------------
// 1.1.1
// free
//   class: the free monad over a single-argument functor F. A node is either a
// Pure leaf holding an A, or a Roll holding one layer F<shared_ptr<free>> of
// child nodes. The two cases are exclusive, so neither A nor the layer needs a
// value in the other case.
template<template<typename> class F,
         typename                 A>
class free
{
public:
    using value_type = A;
    using layer_type = F<std::shared_ptr<free> >;

    // free (default)
    //   constructor: a Pure leaf holding a value-initialized A. Public so a
    // free can sit in a container that default-constructs its elements.
    free()
        : m_node(std::in_place_index<0>)
    {}

    // pure
    //   factory: a finished computation -- a leaf holding _value.
    static
    free pure(
        const A& _value
    )
    {
        return free(std::in_place_index<0>,
                    _value);
    }

    // roll
    //   factory: one layer of F whose positions hold the rest of the tree.
    static
    free roll(
        const layer_type& _layer
    )
    {
        return free(std::in_place_index<1>,
                    _layer);
    }

    // is_pure
    //   accessor: true for a Pure leaf, false for a Roll layer.
    D_NODISCARD
    bool is_pure() const
    {
        return (m_node.index() == 0);
    }

    // pure_value
    //   accessor: the leaf's value.
    //   @pre is_pure()
    D_NODISCARD
    const A& pure_value() const
    {
        return std::get<0>(m_node);
    }

    // layer
    //   accessor: the Roll's layer of child nodes.
    //   @pre !is_pure()
    D_NODISCARD
    const layer_type& layer() const
    {
        return std::get<1>(m_node);
    }

private:
    // free (in place)
    //   constructor: builds the alternative at Index directly, so neither A
    // nor the layer has to be default-constructible.
    template<std::size_t Index,
             typename    Value>
    free(
        std::in_place_index_t<Index> _index,
        const Value&                 _value
    )
        : m_node(_index,
                 _value)
    {}

    std::variant<A, layer_type> m_node;
};


//==============================================================================
// 2.  FREE OPERATIONS
//==============================================================================
// Each operation maps over the F layer with functor_map and recurses into the
// children through their shared_ptr. Each names its own result type, so the
// recursion never asks the compiler to deduce a return type from itself.


// 2.1    Interpretation
//------------------------------------------------------------------------------
// 2.1.1
// fold_free
//   function: interprets a free value against an algebra, bottom-up. A Pure
// leaf goes to _on_pure; a Roll has its children folded first and the
// resulting F<R> goes to _on_roll. R is whatever _on_pure returns.
template<template<typename> class F,
         typename                 A,
         typename                 OnPure,
         typename                 OnRoll>
D_NODISCARD
typename std::decay<decltype(
    std::declval<const OnPure&>()(std::declval<const A&>()))>::type
fold_free(
    const free<F, A>& _free,
    const OnPure&     _on_pure,
    const OnRoll&     _on_roll
)
{
    using child_t = std::shared_ptr<free<F, A> >;

    // a leaf is interpreted directly
    if (_free.is_pure())
    {
        return _on_pure(_free.pure_value());
    }

    return _on_roll(::djinterp::functor_map(
        _free.layer(),
        [&_on_pure, &_on_roll](const child_t& _child)
        {
            return fold_free(*_child,
                             _on_pure,
                             _on_roll);
        }));
}


// 2.2    Construction
//------------------------------------------------------------------------------
// 2.2.1
// free_bind
//   function: monadic bind -- replaces every Pure leaf a with _continue(a),
// which must return a free<F, B>. The F layers are rebuilt around the grafted
// subtrees; a continuation that returns pure(a) leaves the tree unchanged.
template<template<typename> class F,
         typename                 A,
         typename                 Continue>
D_NODISCARD
typename std::decay<decltype(
    std::declval<const Continue&>()(std::declval<const A&>()))>::type
free_bind(
    const free<F, A>& _free,
    const Continue&   _continue
)
{
    using result_t = typename std::decay<decltype(
        _continue(std::declval<const A&>()))>::type;
    using child_t  = std::shared_ptr<free<F, A> >;

    // a leaf is replaced by its continuation
    if (_free.is_pure())
    {
        return _continue(_free.pure_value());
    }

    return result_t::roll(::djinterp::functor_map(
        _free.layer(),
        [&_continue](const child_t& _child)
        {
            return std::make_shared<result_t>(free_bind(*_child,
                                                         _continue));
        }));
}

// 2.2.2
// free_map
//   function: applies _function to every Pure leaf, leaving the F layers
// untouched -- functorial map, as bind followed by pure.
template<template<typename> class F,
         typename                 A,
         typename                 Function>
D_NODISCARD
free<F, typename std::decay<decltype(
    std::declval<const Function&>()(std::declval<const A&>()))>::type>
free_map(
    const free<F, A>& _free,
    const Function&   _function
)
{
    using mapped_t = typename std::decay<decltype(
        _function(std::declval<const A&>()))>::type;

    return free_bind(_free,
                     [&_function](const A& _value)
                     {
                         return free<F, mapped_t>::pure(_function(_value));
                     });
}

// 2.2.3
// lift_free
//   function: lifts one F layer into a free value: Roll with every position a
// Pure leaf. The smallest program -- one instruction whose results are final.
template<template<typename> class F,
         typename                 A>
D_NODISCARD
free<F, A>
lift_free(
    const F<A>& _layer
)
{
    return free<F, A>::roll(::djinterp::functor_map(
        _layer,
        [](const A& _value)
        {
            return std::make_shared<free<F, A> >(free<F, A>::pure(_value));
        }));
}


//==============================================================================
// 3.  TYPECLASS REGISTRATION
//==============================================================================
// Only monad_traits is registered. functor.hpp's monad bridge derives the
// functor instance from it, exactly as for maybe and result; registering
// functor_traits here as well would make the two specializations ambiguous.


// 3.1    monad_traits<free<F, A>>
//------------------------------------------------------------------------------
// 3.1.1
// monad_traits<free<F, A>>
//   trait: unit is pure, bind is free_bind.
template<template<typename> class F,
         typename                 A>
struct monad_traits<free<F, A> >
{
    using is_specialized = std::true_type;
    using value_type     = A;

    template<typename To>
    using rebind = free<F, To>;

    // unit
    //   function: lifts a value into a Pure leaf.
    static
    free<F, A> unit(
        const A& _value
    )
    {
        return free<F, A>::pure(_value);
    }

    // bind
    //   function: grafts _continue onto every Pure leaf.
    template<typename Free,
             typename Continue>
    static
    auto bind(
        Free&&   _free,
        Continue _continue
    )
    -> decltype(free_bind(_free, _continue))
    {
        return free_bind(_free,
                         _continue);
    }
};


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_FUNCTIONAL_FREE_HPP
