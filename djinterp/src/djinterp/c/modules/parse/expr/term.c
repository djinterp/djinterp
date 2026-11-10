/*******************************************************************************
* djinterp [c/parse]                                                      term.c
*
* Expression core v2: definitions for term.h, the arena and its builders.
*   Growth goes through the foundation's one policy, d_parse_grow, and every
* count is checked before it is used as a size, so a runaway term is a refusal
* and never a wrap. Copying and comparing walk with explicit stacks: nothing
* here recurses, so a term of any depth is read in the same stack space.
*
*
* path:      /src/djinterp/c/modules/parse/expr/term.c
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "../../../../../../inc/djinterp/c/modules/parse/expr/term.h"  // corresponding header
// std
#include <stdlib.h>  // malloc, free
#include <string.h>  // memcpy, memset
// djinterp
#include "../../../../../../inc/djinterp/c/dckdint.h"                 // d_ckd_add, d_ckd_mul
#include "../../../../../../inc/djinterp/c/modules/parse/storage.h"  // d_parse_grow


// D_INTERNAL_EXPR_LOCAL_PAIRS
//   constant: the pairs d_parse_expr_term_equal compares on its own stack
// before it needs the heap: enough for any term a person writes.
#define D_INTERNAL_EXPR_LOCAL_PAIRS     128u


/*
d_parse_expr_term_init
  Caller storage is bound with its ownership flags clear, so growth is refused
and release leaves it alone.
*/
void
d_parse_expr_term_init(
    struct dj_parse_expr_term* _term,
    struct dj_parse_expr_node* _nodes,
    uint32_t                   _node_count,
    uint32_t*                  _args,
    uint32_t                   _arg_count
)
{
    // nothing to bind
    if (!_term)
    {
        return;
    }

    _term->nodes         = _nodes;
    _term->node_count    = 0u;
    _term->node_capacity = (_nodes) ? _node_count : 0u;
    _term->args          = _args;
    _term->arg_count     = 0u;
    _term->arg_capacity  = (_args) ? _arg_count : 0u;
    _term->root          = D_PARSE_EXPR_NO_NODE;
    _term->depth         = 0u;
    _term->flags         = 0u;

    return;
}

#if (D_INTERNAL_PARSE_EXPR_HEAP == 1)
/*
d_parse_expr_term_init_heap
  An owned term starts with no storage at all; the first build allocates.
*/
void
d_parse_expr_term_init_heap(
    struct dj_parse_expr_term* _term
)
{
    d_parse_expr_term_init(_term, NULL, 0u, NULL, 0u);

    // the term is ours to grow
    if (_term)
    {
        _term->flags = ( D_PARSE_EXPR_TERM_OWNS_NODES |
                         D_PARSE_EXPR_TERM_OWNS_ARGS );
    }

    return;
}
#endif  // D_INTERNAL_PARSE_EXPR_HEAP

/*
d_parse_expr_term_clear
  The storage and its ownership stay; only the counts and what they imply go.
*/
void
d_parse_expr_term_clear(
    struct dj_parse_expr_term* _term
)
{
    // nothing to clear
    if (!_term)
    {
        return;
    }

    _term->node_count = 0u;
    _term->arg_count  = 0u;
    _term->root       = D_PARSE_EXPR_NO_NODE;
    _term->depth      = 0u;
    _term->flags     &= ~(uint32_t)D_PARSE_EXPR_TERM_HAS_HOLES;

    return;
}

/*
d_parse_expr_term_release
  Each array is freed only where the term allocated it, which the two
ownership flags record separately: a caller may supply one and not the other.
*/
void
d_parse_expr_term_release(
    struct dj_parse_expr_term* _term
)
{
    // nothing to release
    if (!_term)
    {
        return;
    }

#if (D_INTERNAL_PARSE_EXPR_HEAP == 1)
    // free what the term allocated, and only that
    if ((_term->flags & D_PARSE_EXPR_TERM_OWNS_NODES) != 0u)
    {
        free(_term->nodes);
    }

    if ((_term->flags & D_PARSE_EXPR_TERM_OWNS_ARGS) != 0u)
    {
        free(_term->args);
    }
#endif  // D_INTERNAL_PARSE_EXPR_HEAP

    d_parse_expr_term_init(_term, NULL, 0u, NULL, 0u);

    return;
}

/**
 * @brief Makes room for one more node.
 *
 * @param[in,out] _term  the term.
 * @return true when there is room, false when the term is full and cannot
 *         grow, or its count would pass 32 bits.
 */
static bool
d_internal_expr_room_for_node(
    struct dj_parse_expr_term* _term
)
{
    size_t needed = 0u;

    // the count must stay below the index that means "no node"
    if ( (d_ckd_add(&needed, (size_t)_term->node_count, 1u)) ||
         (needed >= (size_t)D_PARSE_EXPR_NO_NODE) )
    {
        return false;
    }

    // the common case: it fits
    if ((uint32_t)needed <= _term->node_capacity)
    {
        return true;
    }

#if (D_INTERNAL_PARSE_EXPR_HEAP == 1)
    void* grown = d_parse_grow(
        _term->nodes,
        &_term->node_capacity,
        (uint32_t)needed,
        (uint32_t)sizeof(struct dj_parse_expr_node),
        ((_term->flags & D_PARSE_EXPR_TERM_OWNS_NODES) != 0u) ? 1 : 0);

    // check if the growth was possible
    if (!grown)
    {
        return false;
    }

    _term->nodes = (struct dj_parse_expr_node*)grown;

    return true;
#else
    return false;
#endif  // D_INTERNAL_PARSE_EXPR_HEAP
}

/**
 * @brief Makes room for more children's indices.
 *
 * @param[in,out] _term   the term.
 * @param[in]     _extra  how many more.
 * @return true when there is room, false otherwise.
 */
static bool
d_internal_expr_room_for_args(
    struct dj_parse_expr_term* _term,
    uint32_t                   _extra
)
{
    size_t needed = 0u;

    // the count must fit in 32 bits
    if ( (d_ckd_add(&needed, (size_t)_term->arg_count, (size_t)_extra)) ||
         (needed > (size_t)0xFFFFFFFFu) )
    {
        return false;
    }

    // the common case: it fits, or nothing is needed
    if ( (_extra == 0u) ||
         ((uint32_t)needed <= _term->arg_capacity) )
    {
        return true;
    }

#if (D_INTERNAL_PARSE_EXPR_HEAP == 1)
    void* grown = d_parse_grow(
        _term->args,
        &_term->arg_capacity,
        (uint32_t)needed,
        (uint32_t)sizeof(uint32_t),
        ((_term->flags & D_PARSE_EXPR_TERM_OWNS_ARGS) != 0u) ? 1 : 0);

    // check if the growth was possible
    if (!grown)
    {
        return false;
    }

    _term->args = (uint32_t*)grown;

    return true;
#else
    return false;
#endif  // D_INTERNAL_PARSE_EXPR_HEAP
}

/**
 * @brief Appends a node of a kind, with no children.
 *
 * @param[in,out] _term  the term.
 * @param[in]     _kind  its kind.
 * @return its index, or D_PARSE_EXPR_NO_NODE when there is no room.
 */
static uint32_t
d_internal_expr_append(
    struct dj_parse_expr_term* _term,
    uint8_t                    _kind
)
{
    // make room, or refuse
    if (!d_internal_expr_room_for_node(_term))
    {
        return D_PARSE_EXPR_NO_NODE;
    }

    const uint32_t             index = _term->node_count;
    struct dj_parse_expr_node* node  = &_term->nodes[index];

    memset(node, 0, sizeof(*node));
    node->kind  = _kind;
    node->depth = 1u;

    _term->node_count = index + 1u;

    // a term's depth is at least every node's
    if (_term->depth < 1u)
    {
        _term->depth = 1u;
    }

    return index;
}

/*
d_parse_expr_term_leaf
  A leaf's value lives in `first`, and its class in `op`, so a leaf costs no
storage beyond its node.
*/
uint32_t
d_parse_expr_term_leaf(
    struct dj_parse_expr_term* _term,
    uint32_t                   _class,
    uint32_t                   _value,
    uint32_t                   _offset,
    uint32_t                   _length
)
{
    // a term is required
    if (!_term)
    {
        return D_PARSE_EXPR_NO_NODE;
    }

    const uint32_t index = d_internal_expr_append(_term,
                                                  D_PARSE_EXPR_NODE_LEAF);

    // check if there was room
    if (index == D_PARSE_EXPR_NO_NODE)
    {
        return index;
    }

    _term->nodes[index].op     = _class;
    _term->nodes[index].first  = _value;
    _term->nodes[index].offset = _offset;
    _term->nodes[index].length = _length;

    return index;
}

/*
d_parse_expr_term_hole
  A hole is a point: the place an operand should have begun.
*/
uint32_t
d_parse_expr_term_hole(
    struct dj_parse_expr_term* _term,
    uint32_t                   _offset
)
{
    // a term is required
    if (!_term)
    {
        return D_PARSE_EXPR_NO_NODE;
    }

    const uint32_t index = d_internal_expr_append(_term,
                                                  D_PARSE_EXPR_NODE_HOLE);

    // check if there was room
    if (index == D_PARSE_EXPR_NO_NODE)
    {
        return index;
    }

    _term->nodes[index].offset = _offset;
    _term->flags              |= D_PARSE_EXPR_TERM_HAS_HOLES;

    return index;
}


/**
 * @brief Appends an application whose children are given through a map.
 *
 * @param[in,out] _term      the term.
 * @param[in]     _op        the operator's id.
 * @param[in]     _children  the children, as indices into `_map`, or into
 *                           the term itself where `_map` is NULL.
 * @param[in]     _map       the renumbering, or NULL for none.
 * @param[in]     _count     the number of children.
 * @param[in]     _offset    the span's first byte.
 * @param[in]     _length    the span's length.
 * @return the new node's index, or D_PARSE_EXPR_NO_NODE.
 *
 * Every child is checked to be a node already in the term before anything is
 * appended, which is what keeps the arena in post-order: a child built later
 * than its parent cannot be named. The depth is the deepest child's plus
 * one, saturating, so it is kept as nodes are built and never recomputed.
 */
static uint32_t
d_internal_expr_build_apply(
    struct dj_parse_expr_term* _term,
    uint32_t                   _op,
    const uint32_t*            _children,
    const uint32_t*            _map,
    uint32_t                   _count,
    uint32_t                   _offset,
    uint32_t                   _length
)
{
    uint32_t deepest = 0u;

    // each child must already be a node of this term
    for (uint32_t i = 0u; i < _count; ++i)
    {
        const uint32_t child = (_map) ? _map[_children[i]] : _children[i];

        if (child >= _term->node_count)
        {
            return D_PARSE_EXPR_NO_NODE;
        }

        if (_term->nodes[child].depth > deepest)
        {
            deepest = _term->nodes[child].depth;
        }
    }

    // room for the children and the node, or a refusal with nothing changed
    if ( (!d_internal_expr_room_for_args(_term, _count)) ||
         (!d_internal_expr_room_for_node(_term)) )
    {
        return D_PARSE_EXPR_NO_NODE;
    }

    const uint32_t first = _term->arg_count;

    for (uint32_t i = 0u; i < _count; ++i)
    {
        _term->args[first + i] = (_map) ? _map[_children[i]] : _children[i];
    }

    _term->arg_count = first + _count;

    const uint32_t index = d_internal_expr_append(_term,
                                                  D_PARSE_EXPR_NODE_APPLY);
    const uint32_t depth = (deepest >= D_PARSE_EXPR_NODE_DEPTH_MAX)
                           ? D_PARSE_EXPR_NODE_DEPTH_MAX
                           : deepest + 1u;

    _term->nodes[index].op     = _op;
    _term->nodes[index].first  = first;
    _term->nodes[index].count  = _count;
    _term->nodes[index].depth  = (uint16_t)depth;
    _term->nodes[index].offset = _offset;
    _term->nodes[index].length = _length;

    // the term's depth follows its deepest node
    if (_term->depth < depth)
    {
        _term->depth = depth;
    }

    return index;
}

/*
d_parse_expr_term_apply
  The public form names the children directly; the mapped form is the copy's.
*/
uint32_t
d_parse_expr_term_apply(
    struct dj_parse_expr_term* _term,
    uint32_t                   _op,
    const uint32_t*            _children,
    uint32_t                   _count,
    uint32_t                   _offset,
    uint32_t                   _length
)
{
    // a term is required, and children where a count is given
    if ( (!_term)                         ||
         ( (_count > 0u) && (!_children) ) )
    {
        return D_PARSE_EXPR_NO_NODE;
    }

    return d_internal_expr_build_apply(_term,
                                       _op,
                                       _children,
                                       NULL,
                                       _count,
                                       _offset,
                                       _length);
}

/**
 * @brief Marks the nodes a root reaches.
 *
 * @param[in]  _term  the term.
 * @param[in]  _root  the root, a node of the term.
 * @param[out] _mark  one byte per node up to the root, set to 1 where
 *                    reached and 0 elsewhere.
 *
 * Children precede their parents, so one pass from the root down to 0 marks
 * every node the root reaches, with no stack.
 */
static void
d_internal_expr_mark_reached(
    const struct dj_parse_expr_term* _term,
    uint32_t                         _root,
    unsigned char*                   _mark
)
{
    memset(_mark, 0, (size_t)_root + 1u);
    _mark[_root] = 1u;

    // each reached application reaches its children, which lie below it
    for (uint32_t i = _root + 1u; i > 0u; --i)
    {
        const struct dj_parse_expr_node* node = &_term->nodes[i - 1u];

        if ( (_mark[i - 1u] == 0u) ||
             (node->kind != D_PARSE_EXPR_NODE_APPLY) )
        {
            continue;
        }

        for (uint32_t c = 0u; c < node->count; ++c)
        {
            _mark[_term->args[node->first + c]] = 1u;
        }
    }

    return;
}

/**
 * @brief Copies the marked nodes in order, renumbering their children.
 *
 * @param[in,out] _to    the term to copy into.
 * @param[in]     _from  the term to copy from.
 * @param[in]     _root  the subtree's root in `_from`.
 * @param[in]     _mark  the nodes to copy.
 * @param[out]    _map   one index per node up to the root: each copied
 *                       node's index in `_to`.
 * @return the copy's root, or D_PARSE_EXPR_NO_NODE where `_to` is full.
 *
 * In arena order a node's children are copied before it, so their new
 * indices are in `_map` when it is built, and a node a chain shares is
 * copied once and shared again.
 */
static uint32_t
d_internal_expr_copy_marked(
    struct dj_parse_expr_term*       _to,
    const struct dj_parse_expr_term* _from,
    uint32_t                         _root,
    const unsigned char*             _mark,
    uint32_t*                        _map
)
{
    for (uint32_t i = 0u; i <= _root; ++i)
    {
        const struct dj_parse_expr_node* node = &_from->nodes[i];
        uint32_t                         made = D_PARSE_EXPR_NO_NODE;

        // only what the root reaches
        if (_mark[i] == 0u)
        {
            continue;
        }

        if (node->kind == D_PARSE_EXPR_NODE_APPLY)
        {
            made = d_internal_expr_build_apply(_to,
                                               node->op,
                                               &_from->args[node->first],
                                               _map,
                                               node->count,
                                               node->offset,
                                               node->length);
        }
        else if (node->kind == D_PARSE_EXPR_NODE_LEAF)
        {
            made = d_parse_expr_term_leaf(_to,
                                          node->op,
                                          node->first,
                                          node->offset,
                                          node->length);
        }
        else
        {
            made = d_parse_expr_term_hole(_to, node->offset);
        }

        // check if the copy had room
        if (made == D_PARSE_EXPR_NO_NODE)
        {
            return made;
        }

        _to->nodes[made].flags = node->flags;
        _map[i]                = made;
    }

    return _map[_root];
}

/*
d_parse_expr_term_copy_scratch
  One index and one byte per node up to the root, the indices first, so that
they are aligned where the scratch is.
*/
size_t
d_parse_expr_term_copy_scratch(
    uint32_t _node
)
{
    size_t nodes = 0u;
    size_t bytes = 0u;

    // a size that does not fit is none
    if ( (d_ckd_add(&nodes, (size_t)_node, 1u))                       ||
         (d_ckd_mul(&bytes, nodes, sizeof(uint32_t) + 1u)) )
    {
        return 0u;
    }

    return bytes;
}

/*
d_parse_expr_term_copy_with
  Two passes, a mark from the root down and a copy from 0 up, each linear, in
the caller's scratch: no allocation but the copy's own nodes.
*/
uint32_t
d_parse_expr_term_copy_with(
    struct dj_parse_expr_term*       _to,
    const struct dj_parse_expr_term* _from,
    uint32_t                         _node,
    void*                            _scratch,
    size_t                           _size
)
{
    // both terms, a node of `_from`, and room for the marks and the map
    if ( (!_to)                                                       ||
         (!_from)                                                     ||
         (_to == _from)                                               ||
         (_node >= _from->node_count)                                 ||
         (!_scratch)                                                  ||
         (_size < d_parse_expr_term_copy_scratch(_node))              ||
         (d_parse_expr_term_copy_scratch(_node) == 0u) )
    {
        return D_PARSE_EXPR_NO_NODE;
    }

    uint32_t*      map  = (uint32_t*)_scratch;
    unsigned char* mark = (unsigned char*)(map + (size_t)_node + 1u);

    d_internal_expr_mark_reached(_from, _node, mark);

    return d_internal_expr_copy_marked(_to, _from, _node, mark, map);
}

/*
d_parse_expr_term_copy
  As d_parse_expr_term_copy_with, its scratch from the heap; a build without
the heap copies nothing, and copies with d_parse_expr_term_copy_with.
*/
uint32_t
d_parse_expr_term_copy(
    struct dj_parse_expr_term*       _to,
    const struct dj_parse_expr_term* _from,
    uint32_t                         _node
)
{
    // both terms are required, and the node must be one of `_from`'s
    if ( (!_to)                          ||
         (!_from)                        ||
         (_to == _from)                  ||
         (_node >= _from->node_count) )
    {
        return D_PARSE_EXPR_NO_NODE;
    }

#if (D_INTERNAL_PARSE_EXPR_HEAP == 1)
    size_t mark_bytes = 0u;
    size_t map_bytes  = 0u;

    // refuse a scratch whose size does not fit
    if ( (d_ckd_add(&mark_bytes, (size_t)_node, 1u)) ||
         (d_ckd_mul(&map_bytes, mark_bytes, sizeof(uint32_t))) )
    {
        return D_PARSE_EXPR_NO_NODE;
    }

    unsigned char* mark = (unsigned char*)malloc(mark_bytes);
    uint32_t*      map  = (uint32_t*)malloc(map_bytes);
    uint32_t       root = D_PARSE_EXPR_NO_NODE;

    // check if the scratch was had
    if ( (mark) &&
         (map) )
    {
        d_internal_expr_mark_reached(_from, _node, mark);
        root = d_internal_expr_copy_marked(_to, _from, _node, mark, map);
    }

    free(mark);
    free(map);

    return root;
#else
    return D_PARSE_EXPR_NO_NODE;
#endif  // D_INTERNAL_PARSE_EXPR_HEAP
}

/**
 * @brief Checks one node against the invariants.
 *
 * @param[in] _term   the term.
 * @param[in] _index  the node's index.
 * @return true when it holds them.
 */
static bool
d_internal_expr_node_sound(
    const struct dj_parse_expr_term* _term,
    uint32_t                         _index
)
{
    const struct dj_parse_expr_node* node    = &_term->nodes[_index];
    uint32_t                         deepest = 0u;
    size_t                           end     = 0u;

    // a leaf and a hole are depth 1, with no children
    if ( (node->kind == D_PARSE_EXPR_NODE_LEAF) ||
         (node->kind == D_PARSE_EXPR_NODE_HOLE) )
    {
        return ( (node->depth == 1u) &&
                 ( (node->kind == D_PARSE_EXPR_NODE_LEAF) ||
                   (node->count == 0u) ) );
    }

    // an application's children lie inside `args`
    if ( (node->kind != D_PARSE_EXPR_NODE_APPLY)                       ||
         (d_ckd_add(&end, (size_t)node->first, (size_t)node->count))  ||
         (end > (size_t)_term->arg_count) )
    {
        return false;
    }

    // and before it, its depth one past the deepest of them
    for (uint32_t c = 0u; c < node->count; ++c)
    {
        const uint32_t child = _term->args[node->first + c];

        if (child >= _index)
        {
            return false;
        }

        if (_term->nodes[child].depth > deepest)
        {
            deepest = _term->nodes[child].depth;
        }
    }

    return ( (uint32_t)node->depth ==
             ( (deepest >= D_PARSE_EXPR_NODE_DEPTH_MAX)
               ? D_PARSE_EXPR_NODE_DEPTH_MAX
               : deepest + 1u ) );
}

/*
d_parse_expr_term_verify
  One pass in order; the post-order invariant is what lets each node be
checked against children already checked.
*/
bool
d_parse_expr_term_verify(
    const struct dj_parse_expr_term* _term
)
{
    // a term, with its arrays where it has nodes or children
    if ( (!_term)                                             ||
         ( (_term->node_count > 0u) && (!_term->nodes) )      ||
         ( (_term->arg_count > 0u) && (!_term->args) )        ||
         (_term->node_count > _term->node_capacity)           ||
         (_term->arg_count > _term->arg_capacity) )
    {
        return false;
    }

    bool holes = false;

    // every node sound, its depth within the term's
    for (uint32_t i = 0u; i < _term->node_count; ++i)
    {
        if ( (!d_internal_expr_node_sound(_term, i)) ||
             (_term->nodes[i].depth > _term->depth) )
        {
            return false;
        }

        holes = ( (holes) ||
                  (_term->nodes[i].kind == D_PARSE_EXPR_NODE_HOLE) );
    }

    // the root a node or none, and the holes flag true to the nodes
    return ( ( (_term->root == D_PARSE_EXPR_NO_NODE) ||
               (_term->root < _term->node_count) )                    &&
             (holes ==
              ((_term->flags & D_PARSE_EXPR_TERM_HAS_HOLES) != 0u)) );
}

/**
 * @brief Whether two nodes match in themselves, children aside.
 *
 * @param[in] _a      the first node.
 * @param[in] _b      the second node.
 * @param[in] _spans  whether spans and GROUPED count.
 * @return true when they match.
 */
static bool
d_internal_expr_nodes_match(
    const struct dj_parse_expr_node* _a,
    const struct dj_parse_expr_node* _b,
    bool                             _spans
)
{
    const uint8_t ignore = (_spans) ? 0u : D_PARSE_EXPR_NODE_GROUPED;
    const bool    leaf   = (_a->kind == D_PARSE_EXPR_NODE_LEAF);

    return ( (_a->kind == _b->kind)                                     &&
             (_a->op == _b->op)                                         &&
             (_a->count == _b->count)                                   &&
             ( (!leaf) || (_a->first == _b->first) )                    &&
             ((_a->flags & ~ignore) == (_b->flags & ~ignore))           &&
             ( (!_spans) ||
               ( (_a->offset == _b->offset) &&
                 (_a->length == _b->length) ) ) );
}

/**
 * @brief Pushes the pairs of two matching applications' children.
 *
 * @param[in]     _a         the first term.
 * @param[in]     _an        its application.
 * @param[in]     _b         the second term.
 * @param[in]     _bn        its application, of as many children.
 * @param[in,out] _stack     the stack of pairs; may be moved to the heap.
 * @param[in]     _local     the caller's own stack, never freed here.
 * @param[in,out] _capacity  the stack's room, in pairs.
 * @param[in,out] _top       the pairs on it.
 * @return true, or false where the stack could not grow, which the caller
 *         reads as "not shown equal".
 */
static bool
d_internal_expr_push_pairs(
    const struct dj_parse_expr_term* _a,
    const struct dj_parse_expr_node* _an,
    const struct dj_parse_expr_term* _b,
    const struct dj_parse_expr_node* _bn,
    uint32_t**                       _stack,
    const uint32_t*                  _local,
    uint32_t*                        _capacity,
    uint32_t*                        _top
)
{
    size_t needed = 0u;

    // the pairs must fit, the stack growing where it can
    if (d_ckd_add(&needed, (size_t)*_top, (size_t)_an->count))
    {
        return false;
    }

    if (needed > (size_t)*_capacity)
    {
#if (D_INTERNAL_PARSE_EXPR_HEAP == 1)
        size_t pairs = (needed > 2u * (size_t)*_capacity)
                       ? needed
                       : 2u * (size_t)*_capacity;
        size_t bytes = 0u;

        // refuse a stack whose size does not fit
        if ( (pairs > (size_t)0xFFFFFFFFu)                          ||
             (d_ckd_mul(&bytes, pairs, 2u * sizeof(uint32_t))) )
        {
            return false;
        }

        uint32_t* grown = (uint32_t*)malloc(bytes);

        // check if the stack could grow
        if (!grown)
        {
            return false;
        }

        memcpy(grown, *_stack, (size_t)*_top * 2u * sizeof(uint32_t));

        if (*_stack != _local)
        {
            free(*_stack);
        }

        *_stack    = grown;
        *_capacity = (uint32_t)pairs;
#else
        (void)_local;

        return false;
#endif  // D_INTERNAL_PARSE_EXPR_HEAP
    }

    for (uint32_t c = 0u; c < _an->count; ++c)
    {
        (*_stack)[2u * *_top]      = _a->args[_an->first + c];
        (*_stack)[2u * *_top + 1u] = _b->args[_bn->first + c];
        ++*_top;
    }

    return true;
}

/*
d_parse_expr_term_equal
  Depth first over pairs, on an explicit stack: its own 128 pairs, then the
heap when a term is broad and deep enough to need more. Each pair popped is
compared in itself and its children's pairs pushed.
*/
bool
d_parse_expr_term_equal(
    const struct dj_parse_expr_term* _a,
    uint32_t                         _a_node,
    const struct dj_parse_expr_term* _b,
    uint32_t                         _b_node,
    bool                             _spans
)
{
    // both nodes must exist
    if ( (!d_parse_expr_term_node(_a, _a_node)) ||
         (!d_parse_expr_term_node(_b, _b_node)) )
    {
        return false;
    }

    uint32_t  local[2u * D_INTERNAL_EXPR_LOCAL_PAIRS];
    uint32_t* stack    = local;
    uint32_t  capacity = D_INTERNAL_EXPR_LOCAL_PAIRS;
    uint32_t  top      = 1u;
    bool      same     = true;

    stack[0] = _a_node;
    stack[1] = _b_node;

    while ( (same) &&
            (top > 0u) )
    {
        --top;

        const struct dj_parse_expr_node* a = &_a->nodes[stack[2u * top]];
        const struct dj_parse_expr_node* b = &_b->nodes[stack[2u * top + 1u]];

        same = d_internal_expr_nodes_match(a, b, _spans);

        // a leaf or a hole has nothing below it
        if ( (!same) ||
             (a->kind != D_PARSE_EXPR_NODE_APPLY) )
        {
            continue;
        }

        same = d_internal_expr_push_pairs(_a, a, _b, b, &stack, local,
                                          &capacity, &top);
    }

#if (D_INTERNAL_PARSE_EXPR_HEAP == 1)
    // the heap's stack, where one was needed
    if (stack != local)
    {
        free(stack);
    }
#endif  // D_INTERNAL_PARSE_EXPR_HEAP

    return same;
}
