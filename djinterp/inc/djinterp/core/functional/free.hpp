/******************************************************************************
* djinterp [functional]                                              free.hpp
*
* Free monad: turn any functor into a monad (C++).
*   The free monad over a functor F is the monad you get "for free" from F
* alone -- a tree whose leaves are pure values and whose internal nodes are a
* layer of F holding sub-trees:
*
*     Free<F, A> = Pure A | Roll (F (Free<F, A>))
*
* It lets you build a program as data -- a sequence of F-shaped instructions --
* and interpret it later with fold_free, separating description from execution.
* Where the rest of this layer abstracted operations a type already had, Free is
* a *construction*: given any F that is a Functor, Free<F, _> is a Monad, with
* unit = Pure and bind grafting the continuation onto every leaf.
*
*   C++ has no higher-kinded types, so F is taken as a single-argument
* template-template parameter (e.g. maybe), and the self-reference in Roll is
* carried through std::shared_ptr -- shared_ptr<free> is a complete type even
* while free is being defined, so F<shared_ptr<free>> instantiates for any F.
* The recursion therefore lives on the heap; Free is not constexpr. F must be a
* registered Functor (functor_traits), since bind, map, and fold_free all map
* over the F layer.
*
*   Free<F, A> registers monad_traits and functor_traits, so it is a first-class
* monad here: every monad combinator (bind / map / then / kleisli_compose /
* lift_m2, and the | pipeline) works on it. lift_free injects one F-layer of
* instructions; fold_free runs the program against an algebra F<R> -> R.
*
* USAGE:
*   using namespace djinterp;
*   free<maybe, int> prog =
*       monad_bind(lift_free(just(10)),
*                  [](int x){ return free<maybe, int>::pure(x + 1); });
*
*   int total = fold_free(prog,
*                         [](int a){ return a; },                 // Pure a -> a
*                         [](const maybe<int>& m){                // F<R> -> R
*                             return m.value_or(0);
*                         });                                     // 11
*
* 
* path:      /inc/djinterp/core/functional/free.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.12
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    THE FREE MONAD TYPE                          (free<F, A>)
II.   FREE OPERATIONS                               (map / bind / lift / fold)
      1.  forward declarations
      2.  per-layer step helpers                    (internal)
      3.  free_map / free_bind / lift_free / fold_free
III.  TYPECLASS REGISTRATION                        (functor_traits, monad_traits)
*/


#ifndef DJINTERP_FUNCTIONAL_FREE_
#define DJINTERP_FUNCTIONAL_FREE_ 1

// std
#include <memory>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"
#include "./functor.hpp"
#include "./monad.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    THE FREE MONAD TYPE  (free<F, A>)                     ///
///////////////////////////////////////////////////////////////////////////////

// free
//   class: the free monad over a single-argument functor _F. A node is either
// Pure (a leaf value of type _A) or Roll (a layer _F<shared_ptr<free>> of child
// nodes). _A and _F<shared_ptr<free>> must be default-constructible (both
// branches are stored as members rather than a union, to keep the type simple);
// the self-reference is carried by shared_ptr, so the tree lives on the heap.
template<template<typename> class _F,
         typename _A>
class free
{
public:
    using value_type = _A;
    using layer_type = _F<std::shared_ptr<free> >;

    // pure
    //   factory: a leaf holding a bare value (the monadic unit).
    static
    free pure(
        const _A& _value
    )
    {
        free _node;
        _node.m_is_pure = true;
        _node.m_value   = _value;

        return _node;
    }

    // roll
    //   factory: an internal node holding one F-layer of child nodes.
    static
    free roll(
        const layer_type& _layer
    )
    {
        free _node;
        _node.m_is_pure = false;
        _node.m_layer   = _layer;

        return _node;
    }

    D_NODISCARD
    bool is_pure() const
    {
        return m_is_pure;
    }

    D_NODISCARD
    const _A& pure_value() const
    {
        return m_value;
    }

    D_NODISCARD
    const layer_type& layer() const
    {
        return m_layer;
    }

private:
    bool       m_is_pure;
    _A         m_value;   // valid when m_is_pure
    layer_type m_layer;   // valid when !m_is_pure

    free()
        : m_is_pure(true)
        , m_value()
        , m_layer()
    {}
};


///////////////////////////////////////////////////////////////////////////////
///             II.   FREE OPERATIONS                                       ///
///////////////////////////////////////////////////////////////////////////////
//   map, bind, lift, and fold_free. Each Roll case maps over the F-layer
// (functor_map, so F must be a registered Functor) and recurses into the child
// nodes through their shared_ptr. The mapping step at each layer is a named
// functor (not a lambda) so the recursion is well-formed on every floor,
// including C++11. Because the step helpers and the operations are mutually
// recursive, the operations are forward-declared first, the helpers defined
// next (complete, so they can be aggregate-initialised), and the operations
// defined last.

// -- 1. forward declarations ------------------------------------------------

template<template<typename> class _F,
         typename _A,
         typename _Function>
D_NODISCARD
free<_F, typename std::decay<decltype(std::declval<_Function&>()(
                          std::declval<const _A&>()))>::type>
free_map(const free<_F, _A>& _node, _Function _function);

template<template<typename> class _F,
         typename _A,
         typename _Continuation>
D_NODISCARD
typename std::decay<decltype(std::declval<_Continuation&>()(
    std::declval<const _A&>()))>::type
free_bind(const free<_F, _A>& _node, _Continuation _continuation);

template<template<typename> class _F,
         typename _A,
         typename _OnPure,
         typename _OnImpure>
D_NODISCARD
typename std::decay<decltype(std::declval<_OnPure&>()(
    std::declval<const _A&>()))>::type
fold_free(const free<_F, _A>& _node, _OnPure _on_pure, _OnImpure _on_impure);


// -- 2. per-layer step helpers (internal) -----------------------------------

NS_INTERNAL

    // free_lift_step
    //   helper: the per-element step of lift_free -- wraps a bare value A into
    // a Pure leaf node behind a shared_ptr. (F<A> -> F<shared_ptr<free>>.)
    template<template<typename> class _F,
             typename _A>
    struct free_lift_step
    {
        std::shared_ptr<free<_F, _A> > operator()(
            const _A& _value
        ) const
        {
            return std::make_shared<free<_F, _A> >(
                free<_F, _A>::pure(_value));
        }
    };

    // free_map_step
    //   helper: the per-child step of free_map -- recurse into a child and
    // re-wrap the mapped node in a shared_ptr.
    template<template<typename> class _F,
             typename _A,
             typename _B,
             typename _Function>
    struct free_map_step
    {
        _Function function;

        std::shared_ptr<free<_F, _B> > operator()(
            const std::shared_ptr<free<_F, _A> >& _child
        ) const
        {
            return std::make_shared<free<_F, _B> >(
                ::djinterp::free_map(*_child, function));
        }
    };

    // free_bind_step
    //   helper: the per-child step of free_bind -- recurse into a child with
    // the continuation.
    template<template<typename> class _F,
             typename _A,
             typename _B,
             typename _Continuation>
    struct free_bind_step
    {
        _Continuation continuation;

        std::shared_ptr<free<_F, _B> > operator()(
            const std::shared_ptr<free<_F, _A> >& _child
        ) const
        {
            return std::make_shared<free<_F, _B> >(
                ::djinterp::free_bind(*_child, continuation));
        }
    };

    // free_fold_step
    //   helper: the per-child step of fold_free -- recurse into a child and
    // return the folded R. Algebra handlers held by pointer (not copied down
    // the tree).
    template<template<typename> class _F,
             typename _A,
             typename _R,
             typename _OnPure,
             typename _OnImpure>
    struct free_fold_step
    {
        _OnPure*   on_pure;
        _OnImpure* on_impure;

        _R operator()(
            const std::shared_ptr<free<_F, _A> >& _child
        ) const
        {
            return ::djinterp::fold_free(*_child, *on_pure, *on_impure);
        }
    };

NS_END  // internal


// -- 3. operation definitions -----------------------------------------------

// free_map
//   function: functorial map over a free monad -- (A -> B) applied to every
// leaf, the F-layers preserved. free<F, B> from free<F, A>.
template<template<typename> class _F,
         typename _A,
         typename _Function>
D_NODISCARD
free<_F, typename std::decay<decltype(std::declval<_Function&>()(
                          std::declval<const _A&>()))>::type>
free_map
(
    const free<_F, _A>& _node,
    _Function           _function
)
{
    using mapped_t = typename std::decay<decltype(
        std::declval<_Function&>()(std::declval<const _A&>()))>::type;
    using target_t = free<_F, mapped_t>;

    if (_node.is_pure())
    {
        return target_t::pure(_function(_node.pure_value()));
    }

    return target_t::roll(::djinterp::functor_map(
        _node.layer(),
        internal::free_map_step<_F, _A, mapped_t, _Function>{_function}));
}


// free_bind
//   function: monadic bind -- grafts k : A -> free<F, B> onto every leaf. For a
// Pure leaf this is k(a); for a Roll layer the continuation is pushed into each
// child. free<F, B> from free<F, A>.
template<template<typename> class _F,
         typename _A,
         typename _Continuation>
D_NODISCARD
typename std::decay<decltype(std::declval<_Continuation&>()(
    std::declval<const _A&>()))>::type
free_bind
(
    const free<_F, _A>& _node,
    _Continuation       _continuation
)
{
    using target_t = typename std::decay<decltype(
        std::declval<_Continuation&>()(std::declval<const _A&>()))>::type;
    using mapped_t = typename target_t::value_type;

    if (_node.is_pure())
    {
        return _continuation(_node.pure_value());
    }

    return target_t::roll(::djinterp::functor_map(
        _node.layer(),
        internal::free_bind_step<_F, _A, mapped_t, _Continuation>{
            _continuation}));
}


// lift_free
//   function: injects one layer of instructions -- F<A> -> free<F, A> -- by
// wrapping each element of the F-layer in a Pure leaf.
template<template<typename> class _F,
         typename _A>
D_NODISCARD
free<_F, _A>
lift_free
(
    const _F<_A>& _fa
)
{
    return free<_F, _A>::roll(::djinterp::functor_map(
        _fa,
        internal::free_lift_step<_F, _A>{}));
}


// fold_free
//   function: the interpreter. Collapses a free<F, A> to a single R using
//   _on_pure : A -> R at the leaves and _on_impure : F<R> -> R at the layers.
// This is where a program built in the free monad is finally run.
template<template<typename> class _F,
         typename _A,
         typename _OnPure,
         typename _OnImpure>
D_NODISCARD
typename std::decay<decltype(std::declval<_OnPure&>()(
    std::declval<const _A&>()))>::type
fold_free
(
    const free<_F, _A>& _node,
    _OnPure             _on_pure,
    _OnImpure           _on_impure
)
{
    using result_t = typename std::decay<decltype(
        std::declval<_OnPure&>()(std::declval<const _A&>()))>::type;

    if (_node.is_pure())
    {
        return _on_pure(_node.pure_value());
    }

    return _on_impure(::djinterp::functor_map(
        _node.layer(),
        internal::free_fold_step<_F, _A, result_t, _OnPure, _OnImpure>{
            &_on_pure, &_on_impure}));
}


///////////////////////////////////////////////////////////////////////////////
///             III.  TYPECLASS REGISTRATION                                ///
///////////////////////////////////////////////////////////////////////////////
//   free<F, A> is a Monad. Registering monad_traits makes every monad
// combinator in this layer apply to a free program directly -- and, through the
// functor monad-bridge in functor.hpp, free is automatically a Functor too
// (its derived map is exactly free_map), so no separate functor_traits is
// registered here (doing so would collide with the bridge).

// monad_traits<free<_F, _A>>
template<template<typename> class _F,
         typename _A>
struct monad_traits<free<_F, _A> >
{
    using is_specialized = std::true_type;
    using value_type     = _A;

    template<typename _To>
    using rebind = free<_F, _To>;

    template<typename _Value>
    static
    free<_F, typename std::decay<_Value>::type> unit(
        _Value&& _value
    )
    {
        return free<_F, typename std::decay<_Value>::type>::pure(
            std::forward<_Value>(_value));
    }

    template<typename _Free,
             typename _Continuation>
    static
    auto bind(
        _Free&&       _node,
        _Continuation _continuation
    )
    -> decltype(::djinterp::free_bind(_node, _continuation))
    {
        return ::djinterp::free_bind(_node, _continuation);
    }
};


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_FREE_
