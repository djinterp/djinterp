/*******************************************************************************
* djinterp [c/parse]                                                      term.h
*
* Expression core v2: the term, a flat arena of nodes linked by index.
*   A term is what a text means once it is parsed: leaves holding a token's
* value, applications of an operator to their children, and holes where an
* operand was missing. The nodes sit in one array and their children's
* indices in another, with no pointer anywhere, so a term copies, hashes and
* serialises whole, as the foundation's programs and grammars do.
*   POST-ORDER. A node's children always have lower indices than the node
* itself; the parser builds bottom-up, so this holds by construction, and
* d_parse_expr_term_verify checks it. A fold is therefore one pass over the
* array in order, with no recursion and no stack.
*   A DAG WHERE A CHAIN SHARES. `a < b <= c` is `and(lt(a, b), le(b, c))` with
* `b` one node, the child of both links, so a fold in arena order computes it
* once. Nothing else shares.
*   NO NODE MADE BY DEFAULT. A missing operand is a hole at the place it should
* have stood, and the term says it holds one (D_PARSE_EXPR_TERM_HAS_HOLES).
*   Every value is 32 bits wide, so the header serves ISO strict C++98 on every
* target, where a 64-bit integer may not exist.
*
*
* path:      /inc/djinterp/c/modules/parse/expr/term.h
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.10
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Nodes
         1.  Node kinds
         2.  Node flags
         3.  dj_parse_expr_node
         4.  D_PARSE_EXPR_NODE_DEPTH_MAX
    2.  Terms
         1.  Term flags
         2.  dj_parse_expr_term
         3.  D_PARSE_EXPR_NO_NODE
2.  OPERATIONS
    ----------
    1.  Lifetime
    2.  Building
    3.  Observing
    4.  Checking and comparing
*/

#ifndef DJINTERP_C_PARSE_EXPR_TERM_H
#define DJINTERP_C_PARSE_EXPR_TERM_H 1

// std
#include <stddef.h>  // size_t, offsetof
// djinterp
#include "../../../djinterp.h"                              // framework root
#include "../../../../config/c/modules/parse/cfg_parse.h"   // D_INTERNAL_PARSE_EXPR_HEAP
// re_std
#include "../../../../../re_std/c/cstdint/dstdint.h"  // uint8_t, uint16_t,
                                                      // uint32_t


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Nodes
//------------------------------------------------------------------------------
// 1.1.1
// Node kinds
//   constant: what a node is: a leaf made from one token, an operator applied
// to its children, or a hole standing where an operand was missing.
#define D_PARSE_EXPR_NODE_LEAF          1u
#define D_PARSE_EXPR_NODE_APPLY         2u
#define D_PARSE_EXPR_NODE_HOLE          3u

// 1.1.2
// Node flags
//   constant: what a node remembers of how it was written. GROUPED: it was
// written in a transparent group, `(a + b)`, whose brackets its span covers;
// CHAIN: it joins the links of a chain, `a < b <= c`; JUXTAPOSED: its
// operator had no token, two operands side by side; FLATTENED: it collects a
// run of one flattening operator, `a | b | c`; ROWS: a list that was written
// in rows, `[1, 2; 3, 4]`.
#define D_PARSE_EXPR_NODE_GROUPED       0x01u
#define D_PARSE_EXPR_NODE_CHAIN         0x02u
#define D_PARSE_EXPR_NODE_JUXTAPOSED    0x04u
#define D_PARSE_EXPR_NODE_FLATTENED     0x08u
#define D_PARSE_EXPR_NODE_ROWS          0x10u

// 1.1.3
// dj_parse_expr_node
//   struct: one node, 24 bytes. `op` is an application's operator id, and a
// leaf's token class; `first` is the index in the term's `args` of an
// application's first child, and a leaf's value; `count` is an application's
// number of children. `depth` is the node's longest path to a leaf, 1 for a
// leaf, saturating at D_PARSE_EXPR_NODE_DEPTH_MAX. The span is in bytes of
// the text the node was read from; a node a builder made has the span its
// caller gave.
struct dj_parse_expr_node
{
    uint8_t  kind;
    uint8_t  flags;
    uint16_t depth;
    uint32_t op;
    uint32_t first;
    uint32_t count;
    uint32_t offset;
    uint32_t length;
};

// 1.1.4
// D_PARSE_EXPR_NODE_DEPTH_MAX
//   constant: where a node's depth saturates; so also the deepest guard a
// parse can keep short of none (D_PARSE_EXPR_DEPTH_UNBOUNDED).
#define D_PARSE_EXPR_NODE_DEPTH_MAX     0xFFFFu


// 1.2    Terms
//------------------------------------------------------------------------------
// 1.2.1
// Term flags
//   constant: OWNS_NODES and OWNS_ARGS: the arrays were allocated by the term
// and are freed by d_parse_expr_term_release; HAS_HOLES: some node is a hole.
#define D_PARSE_EXPR_TERM_OWNS_NODES    0x01u
#define D_PARSE_EXPR_TERM_OWNS_ARGS     0x02u
#define D_PARSE_EXPR_TERM_HAS_HOLES     0x04u

// 1.2.2
// dj_parse_expr_term
//   struct: the arena. Public-payload layout, as the sink's is: a term is the
// protocol between the parser, the folds, the renderer and a language's own
// passes, so its members are unprefixed and directly readable. `depth` is
// the greatest depth of any node, saturating as a node's does.
struct dj_parse_expr_term
{
    struct dj_parse_expr_node* nodes;
    uint32_t                   node_count;
    uint32_t                   node_capacity;
    uint32_t*                  args;
    uint32_t                   arg_count;
    uint32_t                   arg_capacity;
    uint32_t                   root;
    uint32_t                   depth;
    uint32_t                   flags;
};

// 1.2.3
// D_PARSE_EXPR_NO_NODE
//   constant: the index meaning "no node": an empty term's root, a builder's
// failure.
#define D_PARSE_EXPR_NO_NODE            0xFFFFFFFFu

// the layout the arena's users rely on, in every language and at every level
D_STATIC_ASSERT(sizeof(struct dj_parse_expr_node) == 24u,
                "dj_parse_expr_node must stay 24 bytes with no padding");
D_STATIC_ASSERT(offsetof(struct dj_parse_expr_node, op) == 4u,
                "dj_parse_expr_node's op must follow its 4 header bytes");
D_STATIC_ASSERT(offsetof(struct dj_parse_expr_node, length) == 20u,
                "dj_parse_expr_node's span must close the record");


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


//   C linkage for everything below, so a C++ translation unit links against
// the C archive. Both spellings expand to nothing under a C compiler.
D_EXTERN_C_BEGIN

// 2.1    Lifetime
//------------------------------------------------------------------------------
/**
 * @brief Binds a term to caller-supplied storage, empty.
 *
 * @param[out] _term       the term to initialise; may not be NULL.
 * @param[in]  _nodes      room for the nodes; may be NULL with a capacity 0.
 * @param[in]  _node_count how many nodes `_nodes` holds.
 * @param[in]  _args       room for the children's indices; may be NULL with
 *                         a capacity 0.
 * @param[in]  _arg_count  how many indices `_args` holds.
 * @post The term is empty, its root D_PARSE_EXPR_NO_NODE; it never grows.
 */
void     d_parse_expr_term_init(struct dj_parse_expr_term* _term,
                                struct dj_parse_expr_node* _nodes,
                                uint32_t                   _node_count,
                                uint32_t*                  _args,
                                uint32_t                   _arg_count);
#if (D_INTERNAL_PARSE_EXPR_HEAP == 1)
/**
 * @brief Initialises an empty term that allocates as it grows.
 *
 * @param[out] _term  the term to initialise; may not be NULL.
 * @post Nothing is allocated until the first node is built.
 */
void     d_parse_expr_term_init_heap(struct dj_parse_expr_term* _term);
#endif  // D_INTERNAL_PARSE_EXPR_HEAP
/**
 * @brief Empties a term, keeping its storage.
 *
 * @param[in,out] _term  the term; may be NULL.
 */
void     d_parse_expr_term_clear(struct dj_parse_expr_term* _term);
/**
 * @brief Frees whatever storage the term allocated, and empties it.
 *
 * @param[in,out] _term  the term; may be NULL.
 * @post Caller-supplied storage is untouched; the term is empty and bound to
 *       no storage.
 */
void     d_parse_expr_term_release(struct dj_parse_expr_term* _term);

// 2.2    Building
//------------------------------------------------------------------------------
/**
 * @brief Appends a leaf.
 *
 * @param[in,out] _term    the term; may not be NULL.
 * @param[in]     _class   the token class the leaf was read as.
 * @param[in]     _value   the token's value.
 * @param[in]     _offset  the span's first byte.
 * @param[in]     _length  the span's length in bytes.
 * @return the new node's index, or D_PARSE_EXPR_NO_NODE when the term is
 *         full and cannot grow.
 */
uint32_t d_parse_expr_term_leaf(struct dj_parse_expr_term* _term,
                                uint32_t                   _class,
                                uint32_t                   _value,
                                uint32_t                   _offset,
                                uint32_t                   _length);
/**
 * @brief Appends an application of an operator to existing nodes.
 *
 * @param[in,out] _term      the term; may not be NULL.
 * @param[in]     _op        the operator's id.
 * @param[in]     _children  the children's indices, each already in the
 *                           term; may be NULL when `_count` is 0.
 * @param[in]     _count     the number of children.
 * @param[in]     _offset    the span's first byte.
 * @param[in]     _length    the span's length in bytes.
 * @return the new node's index, or D_PARSE_EXPR_NO_NODE when a child is not
 *         in the term, or the term is full and cannot grow.
 */
uint32_t d_parse_expr_term_apply(struct dj_parse_expr_term* _term,
                                 uint32_t                   _op,
                                 const uint32_t*            _children,
                                 uint32_t                   _count,
                                 uint32_t                   _offset,
                                 uint32_t                   _length);
/**
 * @brief Appends a hole, the place of an operand that was missing.
 *
 * @param[in,out] _term    the term; may not be NULL.
 * @param[in]     _offset  where the operand should have stood.
 * @return the new node's index, or D_PARSE_EXPR_NO_NODE when the term is
 *         full and cannot grow.
 * @post The term carries D_PARSE_EXPR_TERM_HAS_HOLES.
 */
uint32_t d_parse_expr_term_hole(struct dj_parse_expr_term* _term,
                                uint32_t                   _offset);
/**
 * @brief Copies the subtree a node roots from one term into another.
 *
 * @param[in,out] _to    the term to copy into; may not be NULL, nor `_from`.
 * @param[in]     _from  the term to copy from.
 * @param[in]     _node  the subtree's root in `_from`.
 * @return the copy's root in `_to`, or D_PARSE_EXPR_NO_NODE when `_node` is
 *         not in `_from` or `_to` is full and cannot grow.
 * @note A node a chain shares is copied once and shared in the copy too. Its
 *       scratch is from the heap: a build without the heap copies with
 *       d_parse_expr_term_copy_with.
 */
uint32_t d_parse_expr_term_copy(struct dj_parse_expr_term*       _to,
                                const struct dj_parse_expr_term* _from,
                                uint32_t                         _node);
/**
 * @brief Copies as d_parse_expr_term_copy does, in the caller's scratch.
 *
 * @param[in,out] _to       the term to copy into.
 * @param[in]     _from     the term to copy from.
 * @param[in]     _node     the subtree's root in `_from`.
 * @param[in]     _scratch  d_parse_expr_term_copy_scratch(_node) bytes or
 *                          more, aligned as malloc's.
 * @param[in]     _size     its size in bytes.
 * @return the copy's root in `_to`, or D_PARSE_EXPR_NO_NODE where `_node` is
 *         not in `_from`, the scratch is too small, or `_to` is full.
 */
uint32_t d_parse_expr_term_copy_with(struct dj_parse_expr_term*       _to,
                                     const struct dj_parse_expr_term* _from,
                                     uint32_t                         _node,
                                     void*                            _scratch,
                                     size_t                           _size);
/**
 * @brief The scratch d_parse_expr_term_copy_with needs to copy a subtree.
 *
 * @param[in] _node  the subtree's root.
 * @return the bytes: one index and one byte per node up to the root; 0 where
 *         they do not fit a size_t.
 */
size_t   d_parse_expr_term_copy_scratch(uint32_t _node);

// 2.3    Observing
//------------------------------------------------------------------------------
// these read a term the caller has verified or the parser has built, and do
// no checking of their own beyond the index's range

/**
 * @brief The node at an index.
 *
 * @param[in] _term   the term; may be NULL.
 * @param[in] _index  the node's index.
 * @return the node, or NULL where the term or the index is invalid.
 */
D_INLINE
const struct dj_parse_expr_node* d_parse_expr_term_node(
    const struct dj_parse_expr_term* _term,
    uint32_t                         _index
)
{
    // reject a missing term or an index past its nodes
    if ( (!_term)                       ||
         (_index >= _term->node_count) )
    {
        return NULL;
    }

    return &_term->nodes[_index];
}

/**
 * @brief The index of an application's child.
 *
 * @param[in] _term   the term; may be NULL.
 * @param[in] _node   the application's index.
 * @param[in] _child  which child, from 0.
 * @return the child's index, or D_PARSE_EXPR_NO_NODE where there is none.
 */
D_INLINE
uint32_t d_parse_expr_term_child(
    const struct dj_parse_expr_term* _term,
    uint32_t                         _node,
    uint32_t                         _child
)
{
    const struct dj_parse_expr_node* node = d_parse_expr_term_node(_term,
                                                                   _node);

    // reject a missing node, a leaf or a hole, and a child past its count
    if ( (!node)                                  ||
         (node->kind != D_PARSE_EXPR_NODE_APPLY)  ||
         (_child >= node->count) )
    {
        return D_PARSE_EXPR_NO_NODE;
    }

    return _term->args[node->first + _child];
}

// 2.4    Checking and comparing
//------------------------------------------------------------------------------
/**
 * @brief Checks a term's invariants.
 *
 * @param[in] _term  the term; may be NULL.
 * @return true when every application's children lie before it, every child
 *         range lies inside `args`, every depth is its children's plus one,
 *         the root is a node or the term is empty, and HAS_HOLES says whether
 *         a hole is there; false otherwise, or for NULL.
 */
bool     d_parse_expr_term_verify(const struct dj_parse_expr_term* _term);
/**
 * @brief Whether two subtrees mean the same.
 *
 * @param[in] _a       the first term.
 * @param[in] _a_node  the first subtree's root.
 * @param[in] _b       the second term.
 * @param[in] _b_node  the second subtree's root.
 * @param[in] _spans   true to compare spans and the GROUPED flag too.
 * @return true when both have the same kinds, operators, values, children and
 *         flags (spans and GROUPED aside, unless `_spans`); false otherwise,
 *         or where either is not a node.
 * @note Iterative, with storage the size of the shallower subtree's depth;
 *       false when that storage cannot be had.
 */
bool     d_parse_expr_term_equal(const struct dj_parse_expr_term* _a,
                                 uint32_t                         _a_node,
                                 const struct dj_parse_expr_term* _b,
                                 uint32_t                         _b_node,
                                 bool                             _spans);

D_EXTERN_C_END


#endif  // DJINTERP_C_PARSE_EXPR_TERM_H
