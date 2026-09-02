/******************************************************************************
* djinterp [c/util/document]                                          layout.c
*
*   Implementation of the structure term.  Compiled by both languages from
* this one source.
*
*   EVERY BUILDER ANSWERS NULL RATHER THAN WRITING PAST ITS REGION, and every
* observer answers the empty case rather than dereferencing NULL.  Those two
* rules are the whole of the error handling here: the parser turns a NULL from
* a builder into D_LAYOUT_PARSE_OVERFLOW and there is nothing else to report.
*
* path:      /src/djinterp/c/util/document/layout.c
* author(s): TBA                                            created: 2026.08.23
******************************************************************************/

#include "djinterp/c/util/document/layout.h"

D_EXTERN_C_BEGIN


/* =============================================================================
   I.   THE ARENA
   ============================================================================= */

int32_t
d_layout_arena_init
(
    struct d_layout_arena*   _arena,
    struct d_cofree*         _nodes,
    size_t                   _node_capacity,
    struct d_layout_label*   _labels,
    size_t                   _label_capacity,
    struct d_doc_attributes* _bags,
    size_t                   _bag_capacity
)
{
    if (!_arena)
    {
        return 0;
    }

    /*   A NULL REGION WITH A NON-ZERO CAPACITY IS THE CALLER'S BUG, and it is
       worth refusing here rather than at the first bump: the failure would
       otherwise surface as an overflow at some unrelated node, hundreds of
       lines into a parse.  A NULL region with a zero capacity is fine -- an
       arena that can hold no bags is legal if nothing asks for one. */
    if ((!_nodes  && _node_capacity  != 0u) ||
        (!_labels && _label_capacity != 0u) ||
        (!_bags   && _bag_capacity   != 0u))
    {
        return 0;
    }

    _arena->nodes          = _nodes;
    _arena->node_capacity  = _node_capacity;
    _arena->node_used      = 0u;

    _arena->labels         = _labels;
    _arena->label_capacity = _label_capacity;
    _arena->label_used     = 0u;

    _arena->bags           = _bags;
    _arena->bag_capacity   = _bag_capacity;
    _arena->bag_used       = 0u;

    return 1;
}


void
d_layout_arena_rewind
(
    struct d_layout_arena* _arena
)
{
    if (!_arena)
    {
        return;
    }

    /*   ALL THREE REGIONS, and this is the reason rewind exists as a function
       rather than as three assignments at each call site: rewinding nodes and
       forgetting the labels leaves an arena that hands out fresh nodes
       pointing at stale labels, which reads as a tree with the previous
       parse's operators in it. */
    _arena->node_used  = 0u;
    _arena->label_used = 0u;
    _arena->bag_used   = 0u;

    return;
}


/*   ONE BUMP HELPER PER REGION, because the three have different element
   types and C has no way to write it once without a macro that would hide the
   capacity test. */
static struct d_layout_label*
d_internal_layout_take_label_
(
    struct d_layout_arena* _arena
)
{
    struct d_layout_label* _at = 0;

    if (!_arena || _arena->label_used >= _arena->label_capacity)
    {
        return (struct d_layout_label*)0;
    }

    _at = _arena->labels + _arena->label_used;
    ++_arena->label_used;

    return _at;
}


static struct d_doc_attributes*
d_internal_layout_take_bag_
(
    struct d_layout_arena* _arena
)
{
    struct d_doc_attributes* _at = 0;

    if (!_arena || _arena->bag_used >= _arena->bag_capacity)
    {
        return (struct d_doc_attributes*)0;
    }

    _at = _arena->bags + _arena->bag_used;
    ++_arena->bag_used;

    return _at;
}


static struct d_cofree*
d_internal_layout_take_nodes_
(
    struct d_layout_arena* _arena,
    size_t                 _count
)
{
    struct d_cofree* _at = 0;

    if (!_arena)
    {
        return (struct d_cofree*)0;
    }

    /*   THE ADDITION IS ORDERED TO NOT OVERFLOW.  `used + count > capacity`
       wraps on a large count and admits the allocation; comparing against the
       remaining space cannot. */
    if (_count > _arena->node_capacity - _arena->node_used)
    {
        return (struct d_cofree*)0;
    }

    _at = _arena->nodes + _arena->node_used;
    _arena->node_used += _count;

    return _at;
}


/* =============================================================================
   II.  ATOMS
   ============================================================================= */

/*   THE UNUSED ARM IS NULLED, not left indeterminate.  The fixture reads
   `value` for a literal and `key` for everything else, and a stale pointer in
   the other arm would be invisible until something walked both. */

struct d_layout_atom
d_layout_body_ref
(
    const char* _key
)
{
    struct d_layout_atom _a;

    _a.kind     = (int32_t)D_LAYOUT_ATOM_BODY_REF;
    _a.reserved = 0;
    _a.key      = _key;
    _a.value    = (const char*)0;

    return _a;
}


struct d_layout_atom
d_layout_meta_ref
(
    const char* _key
)
{
    struct d_layout_atom _a;

    _a.kind     = (int32_t)D_LAYOUT_ATOM_META_REF;
    _a.reserved = 0;
    _a.key      = _key;
    _a.value    = (const char*)0;

    return _a;
}


struct d_layout_atom
d_layout_literal
(
    const char* _value
)
{
    struct d_layout_atom _a;

    _a.kind     = (int32_t)D_LAYOUT_ATOM_LITERAL;
    _a.reserved = 0;
    _a.key      = (const char*)0;
    _a.value    = _value;

    return _a;
}


/* =============================================================================
   III. NODES
   ============================================================================= */

/*   THE ORDER OF THE THREE BUMPS IS LOAD-BEARING WHEN ONE FAILS.  A label
   taken and then a node refused leaves the label stranded -- the arena has
   moved on and nothing points at it.  That is accepted rather than unwound:
   a builder that failed means the parse is about to abort with an overflow,
   and the arena is about to be rewound wholesale.  Unwinding partially would
   be a second bookkeeping path exercised only on the failure route, which is
   the path least likely to be tested. */

static struct d_cofree*
d_internal_layout_node_
(
    struct d_layout_arena*  _arena,
    struct d_doc_attributes _bag,
    struct d_layout_label   _label,
    const struct d_cofree*  _children,
    size_t                  _count
)
{
    struct d_cofree*         _node     = 0;
    struct d_layout_label*   _label_at = 0;
    struct d_doc_attributes* _bag_at   = 0;
    struct d_cofree*         _kids     = 0;
    size_t                   _i        = 0;

    if (!_arena)
    {
        return (struct d_cofree*)0;
    }

    _bag_at = d_internal_layout_take_bag_(_arena);

    if (!_bag_at)
    {
        return (struct d_cofree*)0;
    }

    *_bag_at = _bag;

    _label_at = d_internal_layout_take_label_(_arena);

    if (!_label_at)
    {
        return (struct d_cofree*)0;
    }

    _label.bag = _bag_at;
    *_label_at = _label;

    /*   THE NODE AND ITS CHILDREN ARE ONE ALLOCATION, the node first.  A
       parent copies a contiguous run of already-built children out of the
       caller's frame, so the run has to be contiguous in the arena too --
       taking them separately would interleave with whatever the recursion
       built in between. */
    _node = d_internal_layout_take_nodes_(_arena, (size_t)1 + _count);

    if (!_node)
    {
        return (struct d_cofree*)0;
    }

    _kids = (_count != 0u) ? (_node + 1) : (struct d_cofree*)0;

    for (_i = 0u; _i < _count; ++_i)
    {
        _kids[_i] = _children[_i];
    }

    _node->annotation  = (const void*)_bag_at;
    _node->label       = (const void*)_label_at;
    _node->children    = _kids;
    _node->child_count = _count;

    return _node;
}


struct d_cofree*
d_layout_leaf
(
    struct d_layout_arena*  _arena,
    struct d_doc_attributes _bag,
    struct d_layout_atom    _atom
)
{
    struct d_layout_label _label;

    _label.is_leaf = 1;
    _label.op      = 0;
    _label.atom    = _atom;
    _label.bag     = (const struct d_doc_attributes*)0;   /* filled below */

    /*   A LEAF NEVER HAS CHILDREN, which is not the same as an application
       that happens to have none -- see the header.  The zero here is the
       type's rule, not this call's data. */
    return d_internal_layout_node_(_arena, _bag, _label,
                                   (const struct d_cofree*)0, 0u);
}


struct d_cofree*
d_layout_apply
(
    struct d_layout_arena*  _arena,
    struct d_doc_attributes _bag,
    int32_t                 _op,
    const struct d_cofree*  _children,
    size_t                  _count
)
{
    struct d_layout_label _label;

    /*   A NULL RUN WITH A NON-ZERO COUNT IS A CALLER BUG and is refused; a
       NULL run with a zero count is the empty application, which is a real
       term. */
    if (!_children && _count != 0u)
    {
        return (struct d_cofree*)0;
    }

    _label.is_leaf   = 0;
    _label.op        = _op;
    _label.atom      = d_layout_body_ref((const char*)0);
    _label.bag       = (const struct d_doc_attributes*)0;   /* filled below */

    return d_internal_layout_node_(_arena, _bag, _label, _children, _count);
}


/* =============================================================================
   IV.  OBSERVERS
   ============================================================================= */

/*   THE EMPTY ATOM IS A FILE-SCOPE CONSTANT so `d_layout_atom_of` can answer
   a pointer for a NULL or non-leaf node without returning the address of a
   local.  It is a body_ref with no key, which is what a default-constructed
   atom on the C++ side is closest to; nothing reads it except a caller that
   already ignored `d_layout_is_leaf`. */
static const struct d_layout_atom d_layout_atom_none_ =
{
    (int32_t)D_LAYOUT_ATOM_BODY_REF, 0, (const char*)0, (const char*)0
};

static const struct d_doc_attributes d_layout_bag_none_ = D_DOC_ATTRIBUTES_EMPTY;


static const struct d_layout_label*
d_internal_layout_label_of_
(
    const struct d_cofree* _node
)
{
    if (!_node)
    {
        return (const struct d_layout_label*)0;
    }

    return (const struct d_layout_label*)_node->label;
}


int32_t
d_layout_is_leaf
(
    const struct d_cofree* _node
)
{
    const struct d_layout_label* _l = d_internal_layout_label_of_(_node);

    return _l ? _l->is_leaf : 0;
}


const struct d_layout_atom*
d_layout_atom_of
(
    const struct d_cofree* _node
)
{
    const struct d_layout_label* _l = d_internal_layout_label_of_(_node);

    /*   NEVER NULL.  The fixture dereferences this immediately after checking
       `d_layout_is_leaf`, and a caller that got the order wrong should read
       an empty atom rather than fault. */
    if (!_l || !_l->is_leaf)
    {
        return &d_layout_atom_none_;
    }

    return &_l->atom;
}


int32_t
d_layout_op_of
(
    const struct d_cofree* _node
)
{
    const struct d_layout_label* _l = d_internal_layout_label_of_(_node);

    return (_l && !_l->is_leaf) ? _l->op : 0;
}


const struct d_doc_attributes*
d_layout_bag
(
    const struct d_cofree* _node
)
{
    const struct d_layout_label* _l = d_internal_layout_label_of_(_node);

    if (!_l || !_l->bag)
    {
        return &d_layout_bag_none_;
    }

    return _l->bag;
}


size_t
d_layout_child_count
(
    const struct d_cofree* _node
)
{
    return _node ? _node->child_count : (size_t)0;
}


const struct d_cofree*
d_layout_children
(
    const struct d_cofree* _node
)
{
    return _node ? _node->children : (const struct d_cofree*)0;
}


D_EXTERN_C_END
