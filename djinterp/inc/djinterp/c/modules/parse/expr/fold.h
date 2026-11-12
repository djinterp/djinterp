/*******************************************************************************
* djinterp [c/parse]                                                      fold.h
*
* Expression core v2: folds and walks over a term.
*   A FOLD is an algebra applied bottom-up: each node's value computed from
* its children's. The arena is in post-order, so a fold is one pass over it in
* order -- no recursion, no stack -- visiting only the nodes the root reaches,
* each once, so the operand a chain shares is folded once. The values live in
* an array the caller sizes, one per node up to the root.
*   A WALK is depth first with an explicit stack: `enter` may skip a node's
* children, for a lazy reading of `and`'s second operand or of `piecewise`'s
* branches, and `leave` follows its children. A walk sees a shared node once
* for each parent, as a tree has it.
*   No node is made by default: a term with holes is refused by a fold unless
* its caller asks for them.
*
*
* path:      /inc/djinterp/c/modules/parse/expr/fold.h
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.12
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Folds
         1.  dj_parse_expr_fold_view
         2.  dj_parse_expr_algebra
         3.  Fold flags
    2.  Walks
         1.  Walk answers
         2.  dj_parse_expr_visit_fn
2.  OPERATIONS
    ----------
    1.  Folding
    2.  Walking
    3.  Digesting
*/

#ifndef DJINTERP_C_PARSE_EXPR_FOLD_H
#define DJINTERP_C_PARSE_EXPR_FOLD_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../../djinterp.h"  // framework root
#include "./term.h"             // dj_parse_expr_term
// re_std
#include "../../../../../re_std/c/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Folds
//------------------------------------------------------------------------------
// 1.1.1
// dj_parse_expr_fold_view
//   struct: what an algebra is given of one node: the term, the node's index,
// and the values computed so far, one of `value_size` bytes per node; a
// child's value is d_parse_expr_fold_child's.
struct dj_parse_expr_fold_view
{
    const struct dj_parse_expr_term* term;
    uint32_t                         node;
    const void*                      values;
    size_t                           value_size;
};

// 1.1.2
// dj_parse_expr_algebra
//   type: computes a node's value into `_out` from its children's. Returns
// 0, or anything else to stop the fold, which then returns it.
typedef int (*dj_parse_expr_algebra)(
    void*                                 _ctx,
    const struct dj_parse_expr_fold_view* _view,
    void*                                 _out);

// 1.1.3
// Fold flags
//   constant: HOLES lets a fold run over a term with holes, the algebra then
// given each hole as a node of its own.
#define D_PARSE_EXPR_FOLD_HOLES         0x01u

// the fold's own failures, beside an algebra's
#define D_PARSE_EXPR_FOLD_BAD_ARGUMENT  (-1)
#define D_PARSE_EXPR_FOLD_HAS_HOLES     (-2)
#define D_PARSE_EXPR_FOLD_NO_MEMORY     (-3)


// 1.2    Walks
//------------------------------------------------------------------------------
// 1.2.1
// Walk answers
//   constant: what `enter` says: go into the children, skip them (leave is
// still called), or stop the walk.
#define D_PARSE_EXPR_WALK_CONTINUE      0
#define D_PARSE_EXPR_WALK_SKIP          1
#define D_PARSE_EXPR_WALK_STOP          2

// 1.2.2
// dj_parse_expr_visit_fn
//   type: called on entering or leaving a node, with its parent
// (D_PARSE_EXPR_NO_NODE for the root). Returns a walk answer; `leave`'s
// STOP stops the walk too.
typedef int (*dj_parse_expr_visit_fn)(void*                            _ctx,
                                      const struct dj_parse_expr_term* _term,
                                      uint32_t                         _node,
                                      uint32_t                         _parent);


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


D_EXTERN_C_BEGIN

// 2.1    Folding
//------------------------------------------------------------------------------
/**
 * @brief A child's value, in a view an algebra was given.
 *
 * @param[in] _view   the view.
 * @param[in] _child  which child, from 0.
 * @return its value, or NULL where the node has no such child.
 */
D_INLINE
const void* d_parse_expr_fold_child(
    const struct dj_parse_expr_fold_view* _view,
    uint32_t                              _child
)
{
    const uint32_t index = d_parse_expr_term_child(_view->term, _view->node,
                                                   _child);

    // no such child
    if (index == D_PARSE_EXPR_NO_NODE)
    {
        return NULL;
    }

    return (const char*)_view->values + ((size_t)index * _view->value_size);
}

/**
 * @brief Folds the subtree a root reaches.
 *
 * @param[in]     _term        the term, verified or parsed.
 * @param[in]     _root        the subtree's root.
 * @param[in]     _value_size  the size of one value, not 0.
 * @param[in,out] _values      room for `_root + 1` values, or NULL to take
 *                             it from the heap where the heap is built in.
 * @param[in,out] _marks       room for `_root + 1` bytes, or NULL likewise.
 * @param[in]     _algebra     the algebra.
 * @param[in]     _ctx         its context.
 * @param[in]     _flags       D_PARSE_EXPR_FOLD_* flags.
 * @param[out]    _out         receives the root's value; may not be NULL.
 * @return 0; an algebra's own non-zero answer; or one of the fold's own
 *         failures, D_PARSE_EXPR_FOLD_BAD_ARGUMENT, _HAS_HOLES or _NO_MEMORY.
 */
int  d_parse_expr_fold(const struct dj_parse_expr_term* _term,
                       uint32_t                         _root,
                       size_t                           _value_size,
                       void*                            _values,
                       unsigned char*                   _marks,
                       dj_parse_expr_algebra            _algebra,
                       void*                            _ctx,
                       uint32_t                         _flags,
                       void*                            _out);

// 2.2    Walking
//------------------------------------------------------------------------------
/**
 * @brief Walks the subtree a root reaches, depth first.
 *
 * @param[in] _term   the term, verified or parsed.
 * @param[in] _root   the subtree's root.
 * @param[in] _enter  called before a node's children; may be NULL.
 * @param[in] _leave  called after them; may be NULL.
 * @param[in] _ctx    their context.
 * @return true when the walk ran to its end or was stopped by a visit; false
 *         where an argument is wrong or the stack could not grow.
 */
bool d_parse_expr_walk(const struct dj_parse_expr_term* _term,
                       uint32_t                         _root,
                       dj_parse_expr_visit_fn           _enter,
                       dj_parse_expr_visit_fn           _leave,
                       void*                            _ctx);

// 2.3    Digesting
//------------------------------------------------------------------------------
/**
 * @brief A subtree's digest: 32-bit FNV-1a over its nodes as a depth-first
 *        walk meets them, each node's kind, operator, number of children, a
 *        leaf's value, and its flags but GROUPED.
 *
 * Two subtrees d_parse_expr_term_equal finds equal, spans aside, have one
 * digest, whichever terms and indices they are at: the parity tests and
 * caches compare by it.
 *
 * @param[in] _term  the term, verified or parsed.
 * @param[in] _root  the subtree's root.
 * @return the digest; 0 where an argument is wrong or the walk's stack could
 *         not grow.
 */
uint32_t d_parse_expr_term_digest(const struct dj_parse_expr_term* _term,
                                  uint32_t                         _root);

D_EXTERN_C_END


#endif  // DJINTERP_C_PARSE_EXPR_FOLD_H
