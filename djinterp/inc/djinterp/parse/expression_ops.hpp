/******************************************************************************
* djinterp [expression]                                        expression_ops.hpp
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
*   REQUIREMENTS.  Everything needs _Atom and _OpId copyable (the same as the
* term).  structural_equal and rewrite_to_fixpoint additionally need
* operator== on both; the map_* transforms need their mapping function
* callable on a const _Atom& / const _OpId&.  Transforms return a fresh term
* (the dynamic term is heap-backed, so this allocates per node).
*
*
* TABLE OF CONTENTS
* =================
* I.    FOLD                                  (evaluate)
* II.   STRUCTURAL QUERIES
*       1.  size / depth
*       2.  atoms / operators                 (free-variable / operator collect)
*       3.  structural_equal
*       4.  contains_operator / contains_atom
* III.  TRANSFORMS
*       1.  map_atoms / map_operators
*       2.  transform_bottom_up
*       3.  rewrite_to_fixpoint
*       4.  substitute
*
*
* path:      /inc/djinterp/parse/expression_ops.hpp
* link(s):   ch-recursion.tex
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#ifndef DJINTERP_EXPRESSION_EXPRESSION_OPS_
#define DJINTERP_EXPRESSION_EXPRESSION_OPS_ 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "./expression.hpp"
#include "../functional/recursion.hpp"
#include "../functional/foldable.hpp"
#include "../functional/semigroup.hpp"
#include "../functional/monoid.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    FOLD                                                  ///
///////////////////////////////////////////////////////////////////////////////

// evaluate
//   function: folds an expression to a _Result -- the catamorphism in its
// most-used shape.  _on_leaf : Atom -> R interprets a leaf; _on_apply :
// (OpId, const vector<R>&) -> R collapses an application from its already-
// folded children.  Every interpretation is one of these; the structural
// recursion is cata's.
template<typename _Result,
         typename _OpId,
         typename _Atom,
         typename _OnLeaf,
         typename _OnApply>
D_NODISCARD
_Result
evaluate
(
    const expression<_OpId, _Atom>& _expression,
    _OnLeaf                         _on_leaf,
    _OnApply                        _on_apply
)
{
    return cata<_Result>(
        [_on_leaf, _on_apply]
        (const expr_layer<_OpId, _Atom, _Result>& _layer) -> _Result
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
template<typename _OpId,
         typename _Atom>
D_NODISCARD
std::size_t
size
(
    const expression<_OpId, _Atom>& _expression
)
{
    return evaluate<std::size_t>(
        _expression,
        [](const _Atom&) -> std::size_t
        {
            return 1;
        },
        [](const _OpId&, const std::vector<std::size_t>& _children)
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
template<typename _OpId,
         typename _Atom>
D_NODISCARD
std::size_t
depth
(
    const expression<_OpId, _Atom>& _expression
)
{
    return evaluate<std::size_t>(
        _expression,
        [](const _Atom&) -> std::size_t
        {
            return 1;
        },
        [](const _OpId&, const std::vector<std::size_t>& _children)
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
template<typename _OpId,
         typename _Atom>
D_NODISCARD
std::vector<_Atom>
atoms
(
    const expression<_OpId, _Atom>& _expression
)
{
    return evaluate<std::vector<_Atom> >(
        _expression,
        [](const _Atom& _atom) -> std::vector<_Atom>
        {
            std::vector<_Atom> _single;
            _single.push_back(_atom);

            return _single;
        },
        [](const _OpId&, const std::vector<std::vector<_Atom> >& _children)
            -> std::vector<_Atom>
        {
            return mconcat(_children);
        });
}

// operators
//   function: every operator id in the term, each application contributing
// its own id ahead of its children's.
template<typename _OpId,
         typename _Atom>
D_NODISCARD
std::vector<_OpId>
operators
(
    const expression<_OpId, _Atom>& _expression
)
{
    return evaluate<std::vector<_OpId> >(
        _expression,
        [](const _Atom&) -> std::vector<_OpId>
        {
            return std::vector<_OpId>();
        },
        [](const _OpId& _op, const std::vector<std::vector<_OpId> >& _children)
            -> std::vector<_OpId>
        {
            std::vector<_OpId> _head;
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
// operator== on _Atom and _OpId.
template<typename _OpId,
         typename _Atom>
D_NODISCARD
bool
structural_equal
(
    const expression<_OpId, _Atom>& _left,
    const expression<_OpId, _Atom>& _right
)
{
    const expr_layer<_OpId, _Atom, expression<_OpId, _Atom> >& _l =
        _left.out();
    const expr_layer<_OpId, _Atom, expression<_OpId, _Atom> >& _r =
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
template<typename _OpId,
         typename _Atom>
D_NODISCARD
bool
contains_operator
(
    const expression<_OpId, _Atom>& _expression,
    const _OpId&                    _target
)
{
    return evaluate<bool>(
        _expression,
        [](const _Atom&) -> bool
        {
            return false;
        },
        [_target](const _OpId& _op, const std::vector<bool>& _children) -> bool
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
template<typename _OpId,
         typename _Atom>
D_NODISCARD
bool
contains_atom
(
    const expression<_OpId, _Atom>& _expression,
    const _Atom&                    _target
)
{
    return evaluate<bool>(
        _expression,
        [_target](const _Atom& _atom) -> bool
        {
            return (_atom == _target);
        },
        [](const _OpId&, const std::vector<bool>& _children) -> bool
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
template<typename _OpId,
         typename _Atom,
         typename _Function>
D_NODISCARD
expression<_OpId, typename std::decay<decltype(
    std::declval<_Function&>()(std::declval<const _Atom&>()))>::type>
map_atoms
(
    const expression<_OpId, _Atom>& _expression,
    _Function                       _function
)
{
    using new_atom = typename std::decay<decltype(
        std::declval<_Function&>()(std::declval<const _Atom&>()))>::type;

    return evaluate<expression<_OpId, new_atom> >(
        _expression,
        [_function](const _Atom& _atom) -> expression<_OpId, new_atom>
        {
            return expr_leaf<_OpId, new_atom>(_function(_atom));
        },
        [](const _OpId&                                       _op,
           const std::vector<expression<_OpId, new_atom> >&    _children)
            -> expression<_OpId, new_atom>
        {
            return expr_apply<_OpId, new_atom>(_op, _children);
        });
}

// map_operators
//   function: rebuilds the term with every operator id replaced by
// _function(op), changing the operator type; atoms and shape are preserved.
// The new operator type is deduced from the function's result.
template<typename _OpId,
         typename _Atom,
         typename _Function>
D_NODISCARD
expression<typename std::decay<decltype(
    std::declval<_Function&>()(std::declval<const _OpId&>()))>::type, _Atom>
map_operators
(
    const expression<_OpId, _Atom>& _expression,
    _Function                       _function
)
{
    using new_op = typename std::decay<decltype(
        std::declval<_Function&>()(std::declval<const _OpId&>()))>::type;

    return evaluate<expression<new_op, _Atom> >(
        _expression,
        [](const _Atom& _atom) -> expression<new_op, _Atom>
        {
            return expr_leaf<new_op, _Atom>(_atom);
        },
        [_function](const _OpId&                              _op,
                    const std::vector<expression<new_op, _Atom> >& _children)
            -> expression<new_op, _Atom>
        {
            return expr_apply<new_op, _Atom>(_function(_op), _children);
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
template<typename _OpId,
         typename _Atom,
         typename _Rule>
D_NODISCARD
expression<_OpId, _Atom>
transform_bottom_up
(
    const expression<_OpId, _Atom>& _expression,
    _Rule                           _rule
)
{
    return evaluate<expression<_OpId, _Atom> >(
        _expression,
        [_rule](const _Atom& _atom) -> expression<_OpId, _Atom>
        {
            return _rule(expr_leaf<_OpId, _Atom>(_atom));
        },
        [_rule](const _OpId&                                   _op,
                const std::vector<expression<_OpId, _Atom> >&    _children)
            -> expression<_OpId, _Atom>
        {
            return _rule(expr_apply<_OpId, _Atom>(_op, _children));
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
// set.  Requires operator== on _Atom and _OpId (via structural_equal).
template<typename _OpId,
         typename _Atom,
         typename _Rule>
D_NODISCARD
expression<_OpId, _Atom>
rewrite_to_fixpoint
(
    const expression<_OpId, _Atom>& _expression,
    _Rule                           _rule,
    std::size_t                     _max_passes = 1024
)
{
    expression<_OpId, _Atom> _current = _expression;

    for (std::size_t _pass = 0; _pass < _max_passes; ++_pass)
    {
        expression<_OpId, _Atom> _next =
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
template<typename _OpId,
         typename _Atom,
         typename _Binding>
D_NODISCARD
expression<_OpId, _Atom>
substitute
(
    const expression<_OpId, _Atom>& _expression,
    _Binding                        _binding
)
{
    return evaluate<expression<_OpId, _Atom> >(
        _expression,
        [_binding](const _Atom& _atom) -> expression<_OpId, _Atom>
        {
            return _binding(_atom);
        },
        [](const _OpId&                                   _op,
           const std::vector<expression<_OpId, _Atom> >&    _children)
            -> expression<_OpId, _Atom>
        {
            return expr_apply<_OpId, _Atom>(_op, _children);
        });
}


NS_END  // djinterp


#endif  // DJINTERP_EXPRESSION_EXPRESSION_OPS_
