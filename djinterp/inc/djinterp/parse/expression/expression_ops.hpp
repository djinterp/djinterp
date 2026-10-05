/*******************************************************************************
* djinterp [parse]                                            expression_ops.hpp
*
*   The verbs over an expression term.  expression.hpp defined the noun --
* the term is mu<expr_layer>, and cata (recursion.hpp) already folds it.
* This header supplies the operations every expression language reuses, all
* of them one catamorphism or a walk over it, so no language re-derives the
* recursion.
*
*   ONE FOLD, MANY ALGEBRAS.  evaluate is cata in its most-used shape: a leaf
* handler A -> R and an application handler (OpId, [R]) -> R, the second
* seeing children already folded to R.  Evaluate a math expression, test a
* predicate, render, cost, type-check -- each is one call with a different
* pair of handlers.  The structural queries and the shape-preserving
* transforms below are themselves evaluate instances; only the binary
* structural_equal and the iterated rewrite_to_fixpoint step outside it.
*
*   BUILT ON THE FUNCTIONAL LAYER.  The collecting queries fold a node's
* children through the functional protocols rather than by hand: a child
* count sums via fold_left (foldable.hpp), a free-variable list concatenates
* via mconcat over the vector monoid (monoid.hpp / semigroup.hpp).  The
* recursion is cata's; the per-layer combine is the algebra's.
*
*   REQUIREMENTS.  Everything needs Atom and OpId copyable (the same as the
* term).  structural_equal and rewrite_to_fixpoint additionally need
* operator== on both; the map_* transforms need their mapping function
* callable on a const Atom& / const OpId&.  Transforms return a fresh term
* (the dynamic term is heap-backed, so this allocates per node).
*
*
* path:      /inc/djinterp/parse/expression/expression_ops.hpp
* link(s):   ch-recursion.tex
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    FOLD                                  (evaluate)
      ------------------------------------------------

II.   STRUCTURAL QUERIES
      ------------------
      1.    size / depth
      2.    atoms / operators                 (free-variable / operator collect)
      3.    structural_equal
      4.    contains_operator / contains_atom

III.  TRANSFORMS
      ----------
      1.    map_atoms / map_operators
      2.    transform_bottom_up
      3.    rewrite_to_fixpoint
      4.    substitute
*/

#ifndef DJINTERP_PARSE_EXPRESSION_EXPRESSION_OPS_HPP
#define DJINTERP_PARSE_EXPRESSION_EXPRESSION_OPS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "./expression.hpp"
#include "../../core/functional/recursion.hpp"
#include "../../core/functional/foldable.hpp"
#include "../../core/functional/semigroup.hpp"
#include "../../core/functional/monoid.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    FOLD                                                  ///
///////////////////////////////////////////////////////////////////////////////

// evaluate
//   function: folds an expression to a Result -- the catamorphism in its
// most-used shape.  _on_leaf : Atom -> R interprets a leaf; _on_apply :
// (OpId, const vector<R>&) -> R collapses an application from its already-
// folded children.  Every interpretation is one of these; the structural
// recursion is cata's.
template<typename Result,
         typename OpId,
         typename Atom,
         typename OnLeaf,
         typename OnApply>
D_NODISCARD
Result
evaluate
(
    const expression<OpId, Atom>& _expression,
    OnLeaf                          _on_leaf,
    OnApply                         _on_apply
)
{
    return cata<Result>(
        [_on_leaf, _on_apply]
        (const expr_layer<OpId, Atom, Result>& _layer) -> Result
        {
            if (_layer.is_leaf())
            {
                return _on_leaf(_layer.atom());
            }

            return _on_apply(_layer.op(), _layer.children());
        },
        _expression);
}


///////////////////////////////////////////////////////////////////////////////
///             II.   STRUCTURAL QUERIES                                    ///
///////////////////////////////////////////////////////////////////////////////

// =================================================================
//  1. size / depth
// =================================================================

// size
//   function: the total number of nodes -- leaves and applications alike.
template<typename OpId,
         typename Atom>
D_NODISCARD
std::size_t
size
(
    const expression<OpId, Atom>& _expression
)
{
    return evaluate<std::size_t>(
        _expression,
        [](const Atom&) -> std::size_t
        {
            return 1;
        },
        [](const OpId&, const std::vector<std::size_t>& _children)
            -> std::size_t
        {
            // this node plus the sizes of its sub-terms
            return std::size_t(1) + fold_left(
                _children,
                std::size_t(0),
                [](std::size_t _acc, std::size_t _child) -> std::size_t
                {
                    return (_acc + _child);
                });
        });
}

// depth
//   function: the height of the term -- 1 for a leaf or a nullary
// application, else 1 plus the deepest child.
template<typename OpId,
         typename Atom>
D_NODISCARD
std::size_t
depth
(
    const expression<OpId, Atom>& _expression
)
{
    return evaluate<std::size_t>(
        _expression,
        [](const Atom&) -> std::size_t
        {
            return 1;
        },
        [](const OpId&, const std::vector<std::size_t>& _children)
            -> std::size_t
        {
            return std::size_t(1) + fold_left(
                _children,
                std::size_t(0),
                [](std::size_t _acc, std::size_t _child) -> std::size_t
                {
                    return (_child > _acc) ? _child : _acc;
                });
        });
}


// =================================================================
//  2. atoms / operators
// =================================================================

// atoms
//   function: every atom in the term, in left-to-right leaf order -- the
// free variables when the atom type is a variable.  Child atom-lists are
// concatenated through the vector monoid (mconcat).
template<typename OpId,
         typename Atom>
D_NODISCARD
std::vector<Atom>
atoms
(
    const expression<OpId, Atom>& _expression
)
{
    return evaluate<std::vector<Atom> >(
        _expression,
        [](const Atom& _atom) -> std::vector<Atom>
        {
            std::vector<Atom> _single;
            _single.push_back(_atom);

            return _single;
        },
        [](const OpId&, const std::vector<std::vector<Atom> >& _children)
            -> std::vector<Atom>
        {
            return mconcat(_children);
        });
}

// operators
//   function: every operator id in the term, each application contributing
// its own id ahead of its children's.
template<typename OpId,
         typename Atom>
D_NODISCARD
std::vector<OpId>
operators
(
    const expression<OpId, Atom>& _expression
)
{
    return evaluate<std::vector<OpId> >(
        _expression,
        [](const Atom&) -> std::vector<OpId>
        {
            return std::vector<OpId>();
        },
        [](const OpId& _op, const std::vector<std::vector<OpId> >& _children)
            -> std::vector<OpId>
        {
            std::vector<OpId> _head;
            _head.push_back(_op);

            return mappend(_head, mconcat(_children));
        });
}


// =================================================================
//  3. structural_equal
// =================================================================

// structural_equal
//   function: whether two terms have the same shape -- identical leaf atoms,
// identical operators, identical child sequences, recursively.  A binary
// recursion (cata folds one term), so it is written directly.  Requires
// operator== on Atom and OpId.
template<typename OpId,
         typename Atom>
D_NODISCARD
bool
structural_equal
(
    const expression<OpId, Atom>& _left,
    const expression<OpId, Atom>& _right
)
{
    const expr_layer<OpId, Atom, expression<OpId, Atom> >& _l =
        _left.out();
    const expr_layer<OpId, Atom, expression<OpId, Atom> >& _r =
        _right.out();

    if (_l.is_leaf() != _r.is_leaf())
    {
        return false;
    }

    if (_l.is_leaf())
    {
        return (_l.atom() == _r.atom());
    }

    if (!(_l.op() == _r.op()))
    {
        return false;
    }

    if (_l.children().size() != _r.children().size())
    {
        return false;
    }

    // shapes agree at this node -- compare sub-terms pairwise
    for (std::size_t _i = 0; _i < _l.children().size(); ++_i)
    {
        if (!structural_equal(_l.children()[_i], _r.children()[_i]))
        {
            return false;
        }
    }

    return true;
}


// =================================================================
//  4. contains_operator / contains_atom
// =================================================================

// contains_operator
//   function: whether the given operator id appears anywhere in the term.
template<typename OpId,
         typename Atom>
D_NODISCARD
bool
contains_operator
(
    const expression<OpId, Atom>& _expression,
    const OpId&                    _target
)
{
    return evaluate<bool>(
        _expression,
        [](const Atom&) -> bool
        {
            return false;
        },
        [_target](const OpId& _op, const std::vector<bool>& _children) -> bool
        {
            if (_op == _target)
            {
                return true;
            }

            return fold_left(
                _children,
                false,
                [](bool _acc, bool _child) -> bool
                {
                    return (_acc || _child);
                });
        });
}

// contains_atom
//   function: whether the given atom appears at any leaf of the term.
template<typename OpId,
         typename Atom>
D_NODISCARD
bool
contains_atom
(
    const expression<OpId, Atom>& _expression,
    const Atom&                    _target
)
{
    return evaluate<bool>(
        _expression,
        [_target](const Atom& _atom) -> bool
        {
            return (_atom == _target);
        },
        [](const OpId&, const std::vector<bool>& _children) -> bool
        {
            return fold_left(
                _children,
                false,
                [](bool _acc, bool _child) -> bool
                {
                    return (_acc || _child);
                });
        });
}


///////////////////////////////////////////////////////////////////////////////
///             III.  TRANSFORMS                                            ///
///////////////////////////////////////////////////////////////////////////////

// =================================================================
//  1. map_atoms / map_operators
// =================================================================

// map_atoms
//   function: rebuilds the term with every atom replaced by _function(atom),
// changing the atom type; operators and shape are preserved.  The new atom
// type is deduced from the function's result.
template<typename OpId,
         typename Atom,
         typename Function>
D_NODISCARD
expression<OpId, typename std::decay<decltype(
    std::declval<Function&>()(std::declval<const Atom&>()))>::type>
map_atoms
(
    const expression<OpId, Atom>& _expression,
    Function                        _function
)
{
    using new_atom = typename std::decay<decltype(
        std::declval<Function&>()(std::declval<const Atom&>()))>::type;

    return evaluate<expression<OpId, new_atom> >(
        _expression,
        [_function](const Atom& _atom) -> expression<OpId, new_atom>
        {
            return expr_leaf<OpId, new_atom>(_function(_atom));
        },
        [](const OpId&                                       _op,
           const std::vector<expression<OpId, new_atom> >&    _children)
            -> expression<OpId, new_atom>
        {
            return expr_apply<OpId, new_atom>(_op, _children);
        });
}

// map_operators
//   function: rebuilds the term with every operator id replaced by
// _function(op), changing the operator type; atoms and shape are preserved.
// The new operator type is deduced from the function's result.
template<typename OpId,
         typename Atom,
         typename Function>
D_NODISCARD
expression<typename std::decay<decltype(
    std::declval<Function&>()(std::declval<const OpId&>()))>::type, Atom>
map_operators
(
    const expression<OpId, Atom>& _expression,
    Function                        _function
)
{
    using new_op = typename std::decay<decltype(
        std::declval<Function&>()(std::declval<const OpId&>()))>::type;

    return evaluate<expression<new_op, Atom> >(
        _expression,
        [](const Atom& _atom) -> expression<new_op, Atom>
        {
            return expr_leaf<new_op, Atom>(_atom);
        },
        [_function](const OpId&                              _op,
                    const std::vector<expression<new_op, Atom> >& _children)
            -> expression<new_op, Atom>
        {
            return expr_apply<new_op, Atom>(_function(_op), _children);
        });
}


// =================================================================
//  2. transform_bottom_up
// =================================================================

// transform_bottom_up
//   function: rewrites the term by applying _rule to every node after its
// children have been rewritten -- a single bottom-up pass.  _rule is any
// expression -> expression (a simplification step, a normalization, a
// constant fold); returning its argument unchanged is a no-op at that node.
template<typename OpId,
         typename Atom,
         typename Rule>
D_NODISCARD
expression<OpId, Atom>
transform_bottom_up
(
    const expression<OpId, Atom>& _expression,
    Rule                            _rule
)
{
    return evaluate<expression<OpId, Atom> >(
        _expression,
        [_rule](const Atom& _atom) -> expression<OpId, Atom>
        {
            return _rule(expr_leaf<OpId, Atom>(_atom));
        },
        [_rule](const OpId&                                   _op,
                const std::vector<expression<OpId, Atom> >&    _children)
            -> expression<OpId, Atom>
        {
            return _rule(expr_apply<OpId, Atom>(_op, _children));
        });
}


// =================================================================
//  3. rewrite_to_fixpoint
// =================================================================

// rewrite_to_fixpoint
//   function: applies transform_bottom_up repeatedly until a pass changes
// nothing (the fixed point) or _max_passes is reached -- the standard driver
// for a set of simplification rules.  Convergence is the caller's
// responsibility; the pass cap guards a non-terminating (non-confluent) rule
// set.  Requires operator== on Atom and OpId (via structural_equal).
template<typename OpId,
         typename Atom,
         typename Rule>
D_NODISCARD
expression<OpId, Atom>
rewrite_to_fixpoint
(
    const expression<OpId, Atom>& _expression,
    Rule                            _rule,
    std::size_t                     _max_passes = 1024
)
{
    expression<OpId, Atom> _current = _expression;

    for (std::size_t _pass = 0; _pass < _max_passes; ++_pass)
    {
        expression<OpId, Atom> _next =
            transform_bottom_up(_current, _rule);

        // a fixed point -- the rule left every node unchanged this pass
        if (structural_equal(_current, _next))
        {
            return _current;
        }

        _current = _next;
    }

    return _current;
}


// =================================================================
//  4. substitute
// =================================================================

// substitute
//   function: rebuilds the term with every leaf atom replaced by the term
// _binding(atom) -- variable substitution, the cata form of a free-monad
// bind.  _binding is total: it returns expr_leaf(atom) for an atom it does
// not rebind (leaving it in place).  Operators and application shape are
// preserved around the substituted leaves.
template<typename OpId,
         typename Atom,
         typename Binding>
D_NODISCARD
expression<OpId, Atom>
substitute
(
    const expression<OpId, Atom>& _expression,
    Binding                         _binding
)
{
    return evaluate<expression<OpId, Atom> >(
        _expression,
        [_binding](const Atom& _atom) -> expression<OpId, Atom>
        {
            return _binding(_atom);
        },
        [](const OpId&                                   _op,
           const std::vector<expression<OpId, Atom> >&    _children)
            -> expression<OpId, Atom>
        {
            return expr_apply<OpId, Atom>(_op, _children);
        });
}


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_PARSE_EXPRESSION_EXPRESSION_OPS_HPP
