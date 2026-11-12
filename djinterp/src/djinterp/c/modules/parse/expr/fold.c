/*******************************************************************************
* djinterp [c/parse]                                                      fold.c
*
* Expression core v2: definitions for fold.h, folds and walks.
*   A fold marks what the root reaches in one pass down from it, then computes
* each marked node in one pass up from 0: children before parents, by the
* arena's post-order, with no stack. A walk keeps its own stack of nodes and
* their next child, its first 64 places in its frame and the rest on the heap.
*
*
* path:      /src/djinterp/c/modules/parse/expr/fold.c
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.12
*******************************************************************************/
#include "../../../../../../inc/djinterp/c/modules/parse/expr/fold.h"  // corresponding header
// std
#include <stdlib.h>  // malloc, realloc, free
#include <string.h>  // memcpy, memset
// djinterp
#include "../../../../../../inc/djinterp/c/dckdint.h"  // d_ckd_add, d_ckd_mul
#include "../../../../../../inc/djinterp/config/c/modules/parse/cfg_parse.h"  // D_INTERNAL_PARSE_EXPR_HEAP


// D_INTERNAL_FOLD_LOCAL_FRAMES
//   constant: the places a walk's stack has before it needs the heap.
#define D_INTERNAL_FOLD_LOCAL_FRAMES    64u

// dj_internal_walk_frame
//   struct: one node on a walk's stack, its parent, and its next child.
struct dj_internal_walk_frame
{
    uint32_t node;
    uint32_t parent;
    uint32_t next;
};


/**
 * @brief Marks the nodes a root reaches.
 *
 * @param[in]  _term   the term.
 * @param[in]  _root   the root.
 * @param[out] _marks  one byte per node up to the root.
 */
static void
d_internal_fold_mark(
    const struct dj_parse_expr_term* _term,
    uint32_t                         _root,
    unsigned char*                   _marks
)
{
    memset(_marks, 0, (size_t)_root + 1u);
    _marks[_root] = 1u;

    // children lie below their parents: one pass down marks them all
    for (uint32_t i = _root + 1u; i > 0u; --i)
    {
        const struct dj_parse_expr_node* node = &_term->nodes[i - 1u];

        if ( (_marks[i - 1u] == 0u) ||
             (node->kind != D_PARSE_EXPR_NODE_APPLY) )
        {
            continue;
        }

        for (uint32_t c = 0u; c < node->count; ++c)
        {
            _marks[_term->args[node->first + c]] = 1u;
        }
    }

    return;
}

/**
 * @brief Runs the algebra over the marked nodes in order.
 *
 * @param[in]     _term        the term.
 * @param[in]     _root        the root.
 * @param[in]     _value_size  a value's size.
 * @param[in,out] _values      the values.
 * @param[in]     _marks       the reached nodes.
 * @param[in]     _algebra     the algebra.
 * @param[in]     _ctx         its context.
 * @return 0, or the algebra's answer that stopped it.
 */
static int
d_internal_fold_run(
    const struct dj_parse_expr_term* _term,
    uint32_t                         _root,
    size_t                           _value_size,
    void*                            _values,
    const unsigned char*             _marks,
    dj_parse_expr_algebra            _algebra,
    void*                            _ctx
)
{
    struct dj_parse_expr_fold_view view;

    view.term       = _term;
    view.values     = _values;
    view.value_size = _value_size;

    for (uint32_t i = 0u; i <= _root; ++i)
    {
        // only what the root reaches, each once
        if (_marks[i] == 0u)
        {
            continue;
        }

        view.node = i;

        const int answer = _algebra(_ctx,
                                    &view,
                                    (char*)_values + ((size_t)i * _value_size));

        if (answer != 0)
        {
            return answer;
        }
    }

    return 0;
}

/*
d_parse_expr_fold
  The scratch the caller does not give is taken from the heap and freed before
returning; the values and the marks are both one per node up to the root, so
their sizes are checked before either is asked for.
*/
int
d_parse_expr_fold(
    const struct dj_parse_expr_term* _term,
    uint32_t                         _root,
    size_t                           _value_size,
    void*                            _values,
    unsigned char*                   _marks,
    dj_parse_expr_algebra            _algebra,
    void*                            _ctx,
    uint32_t                         _flags,
    void*                            _out
)
{
    size_t nodes = 0u;
    size_t bytes = 0u;

    // a node of a term, an algebra, somewhere for the answer, sizes that fit
    if ( (!d_parse_expr_term_node(_term, _root))                     ||
         (!_algebra)                                                 ||
         (!_out)                                                     ||
         (_value_size == 0u)                                         ||
         (d_ckd_add(&nodes, (size_t)_root, 1u))                      ||
         (d_ckd_mul(&bytes, nodes, _value_size)) )
    {
        return D_PARSE_EXPR_FOLD_BAD_ARGUMENT;
    }

    // no node made by default reaches an algebra unasked
    if ( ((_term->flags & D_PARSE_EXPR_TERM_HAS_HOLES) != 0u) &&
         ((_flags & D_PARSE_EXPR_FOLD_HOLES) == 0u) )
    {
        return D_PARSE_EXPR_FOLD_HAS_HOLES;
    }

    void*          values = _values;
    unsigned char* marks  = _marks;

#if (D_INTERNAL_PARSE_EXPR_HEAP == 1)
    values = (values) ? values : malloc(bytes);
    marks  = (marks) ? marks : (unsigned char*)malloc(nodes);
#endif  // D_INTERNAL_PARSE_EXPR_HEAP

    int answer = D_PARSE_EXPR_FOLD_NO_MEMORY;

    // check if the scratch was had
    if ( (values) &&
         (marks) )
    {
        d_internal_fold_mark(_term, _root, marks);
        answer = d_internal_fold_run(_term, _root, _value_size, values, marks,
                                     _algebra, _ctx);

        if (answer == 0)
        {
            memcpy(_out, (const char*)values + ((size_t)_root * _value_size),
                   _value_size);
        }
    }

#if (D_INTERNAL_PARSE_EXPR_HEAP == 1)
    // what was taken here is given back here
    if (values != _values)
    {
        free(values);
    }

    if (marks != _marks)
    {
        free(marks);
    }
#endif  // D_INTERNAL_PARSE_EXPR_HEAP

    return answer;
}

/**
 * @brief Pushes a node onto a walk's stack.
 *
 * @param[in,out] _stack     the stack; may move to the heap.
 * @param[in]     _local     the walk's own room, never freed here.
 * @param[in,out] _count     the places used.
 * @param[in,out] _capacity  the room.
 * @param[in]     _node      the node.
 * @param[in]     _parent    its parent.
 * @return true, or false where the stack could not grow.
 */
static bool
d_internal_walk_push(
    struct dj_internal_walk_frame**     _stack,
    const struct dj_internal_walk_frame* _local,
    uint32_t*                           _count,
    uint32_t*                           _capacity,
    uint32_t                            _node,
    uint32_t                            _parent
)
{
    // room, doubling onto the heap
    if (*_count >= *_capacity)
    {
#if (D_INTERNAL_PARSE_EXPR_HEAP == 1)
        size_t elements = 0u;
        size_t bytes    = 0u;

        if ( (d_ckd_mul(&elements, (size_t)*_capacity, 2u))          ||
             (elements > (size_t)0xFFFFFFFFu)                        ||
             (d_ckd_mul(&bytes, elements,
                        sizeof(struct dj_internal_walk_frame))) )
        {
            return false;
        }

        void* grown = (*_stack == _local) ? malloc(bytes)
                                          : realloc(*_stack, bytes);

        if (!grown)
        {
            return false;
        }

        if (*_stack == _local)
        {
            memcpy(grown, _local,
                   (size_t)*_count * sizeof(struct dj_internal_walk_frame));
        }

        *_stack    = (struct dj_internal_walk_frame*)grown;
        *_capacity = (uint32_t)elements;
#else
        (void)_local;

        return false;
#endif  // D_INTERNAL_PARSE_EXPR_HEAP
    }

    (*_stack)[*_count].node   = _node;
    (*_stack)[*_count].parent = _parent;
    (*_stack)[*_count].next   = 0u;
    ++*_count;

    return true;
}

/*
d_parse_expr_walk
  A node is entered when it is pushed; its frame then hands out its children
one at a time, and when none is left the node is left and popped. A skip sets
the next child past the last, so the node is left at once.
*/
bool
d_parse_expr_walk(
    const struct dj_parse_expr_term* _term,
    uint32_t                         _root,
    dj_parse_expr_visit_fn           _enter,
    dj_parse_expr_visit_fn           _leave,
    void*                            _ctx
)
{
    // a node of a term
    if (!d_parse_expr_term_node(_term, _root))
    {
        return false;
    }

    struct dj_internal_walk_frame  local[D_INTERNAL_FOLD_LOCAL_FRAMES];
    struct dj_internal_walk_frame* stack    = local;
    uint32_t                       count    = 0u;
    uint32_t                       capacity = D_INTERNAL_FOLD_LOCAL_FRAMES;
    uint32_t                       node     = _root;
    uint32_t                       parent   = D_PARSE_EXPR_NO_NODE;
    bool                           whole    = true;
    bool                           stopped  = false;

    while ( (!stopped) &&
            (whole) )
    {
        // enter the node in hand, if there is one
        if (node != D_PARSE_EXPR_NO_NODE)
        {
            const int answer = (_enter) ? _enter(_ctx, _term, node, parent)
                                        : D_PARSE_EXPR_WALK_CONTINUE;

            stopped = (answer == D_PARSE_EXPR_WALK_STOP);
            whole   = ( (stopped) ||
                        (d_internal_walk_push(&stack, local, &count,
                                              &capacity, node, parent)) );

            if ( (whole) &&
                 (!stopped) &&
                 (answer == D_PARSE_EXPR_WALK_SKIP) )
            {
                stack[count - 1u].next = 0xFFFFFFFFu;
            }

            node = D_PARSE_EXPR_NO_NODE;

            continue;
        }

        // the walk is done when its stack is
        if (count == 0u)
        {
            break;
        }

        struct dj_internal_walk_frame*   top  = &stack[count - 1u];
        const struct dj_parse_expr_node* here = &_term->nodes[top->node];
        const uint32_t                   kids =
            (here->kind == D_PARSE_EXPR_NODE_APPLY) ? here->count : 0u;

        if (top->next < kids)
        {
            parent = top->node;
            node   = _term->args[here->first + top->next];
            ++top->next;

            continue;
        }

        stopped = ( (_leave) &&
                    (_leave(_ctx, _term, top->node, top->parent) ==
                     D_PARSE_EXPR_WALK_STOP) );
        --count;
    }

#if (D_INTERNAL_PARSE_EXPR_HEAP == 1)
    // a stack that left the walk's own room
    if (stack != local)
    {
        free(stack);
    }
#endif  // D_INTERNAL_PARSE_EXPR_HEAP

    return whole;
}


//==============================================================================
// DIGESTING
//==============================================================================


// FNV-1a's 32-bit offset basis and prime
#define D_INTERNAL_EXPR_FNV_BASIS   2166136261u
#define D_INTERNAL_EXPR_FNV_PRIME   16777619u

/*
d_internal_expr_fnv
  Folds a 32-bit word into a digest, a byte at a time, low byte first, so the
digest is the same on every byte order.
*/
static uint32_t
d_internal_expr_fnv(
    uint32_t _digest,
    uint32_t _word
)
{
    for (unsigned shift = 0u; shift < 32u; shift += 8u)
    {
        _digest ^= (_word >> shift) & 0xFFu;
        _digest *= D_INTERNAL_EXPR_FNV_PRIME;
    }

    return _digest;
}

/*
d_internal_expr_digest_enter
  Folds a node in as the walk enters it.
*/
static int
d_internal_expr_digest_enter(
    void*                            _ctx,
    const struct dj_parse_expr_term* _term,
    uint32_t                         _node,
    uint32_t                         _parent
)
{
    uint32_t* const                  digest = (uint32_t*)_ctx;
    const struct dj_parse_expr_node* n      = &_term->nodes[_node];
    const uint32_t                   head   =
        (uint32_t)n->kind |
        ((uint32_t)(n->flags & ~D_PARSE_EXPR_NODE_GROUPED) << 8u);

    (void)_parent;

    *digest = d_internal_expr_fnv(*digest, head);
    *digest = d_internal_expr_fnv(*digest, n->op);
    *digest = d_internal_expr_fnv(*digest, n->count);
    *digest = d_internal_expr_fnv(*digest,
                                  (n->kind == D_PARSE_EXPR_NODE_LEAF)
                                  ? n->first
                                  : 0u);

    return D_PARSE_EXPR_WALK_CONTINUE;
}

/*
d_parse_expr_term_digest
  A walk with the digest as its context.
*/
uint32_t
d_parse_expr_term_digest(
    const struct dj_parse_expr_term* _term,
    uint32_t                         _root
)
{
    uint32_t digest = D_INTERNAL_EXPR_FNV_BASIS;

    if ( (!_term) || (_root >= _term->node_count) )
    {
        return 0u;
    }

    return (d_parse_expr_walk(_term, _root, d_internal_expr_digest_enter,
                              NULL, &digest))
           ? digest
           : 0u;
}
