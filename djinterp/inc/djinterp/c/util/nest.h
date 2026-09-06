/******************************************************************************
* djinterp [util]                                                      nest.h
*
* Describing the shape of an n-ary node -- where its payload sits, where its
* links sit, and what a link means -- so that one walker serves every node
* type. Tier 0 -- compiled by BOTH faces.
*
*
* WHAT THIS IS, BY ANALOGY
* ========================
*   kv.h describes WHERE A DATUM SITS inside a record and how to touch it
* without an alignment, aliasing or endian fault. This header does the same
* job one level up: it describes where a node's NEIGHBOURS sit and how to turn
* the bytes at those offsets back into a node. The payload field is literally
* a `d_kv_field`; a link is that field plus a kind.
*
*   THE POINT IS THAT A TREE'S SHAPE IS DATA. `d_file_tree_node` keeps a
* parent pointer, a `struct d_file_tree_node** children` and a `size_t count`.
* A classic n-ary node keeps first-child and next-sibling. An arena-backed
* node keeps two uint32_t indices. A memory-mapped node keeps two self-
* relative offsets so a subtree may be written to disk and read back at a
* different address. These are four walkers today. They are one walker and
* four descriptors here.
*
*
* THE THREE AXES
* ==============
*   PAYLOAD -- inline or indirect. An inline payload is the bytes at the
* field; an indirect one is the bytes the pointer at the field leads to. A
* node may also have no payload at all, which is a pure-topology node and is
* spelled with a zero-width field.
*
*   LINK REPRESENTATION -- five kinds, in D_NEST_KIND_*. A pointer link is a
* node address. An index link is a subscript into an arena. An offset link is
* a byte displacement from the arena base; a self-offset link is a
* displacement from the node itself, which is the one form that survives being
* relocated without the arena. A resolver link is computed by a function.
*
*   LINK POSITION -- every link carries its own offset, so the payload and the
* links may sit anywhere in the node and in any order. Nothing here assumes a
* member leads, which is the assumption `D_REGISTRY_ROW_KEY` made about keys
* and the reason it could not be reused.
*
*
* SENTINELS, WHICH ARE THE PART THAT BITES
* ========================================
*   A POINTER link is absent when NULL. An OFFSET or SELF_OFFSET link is
* absent when ZERO, because a node cannot be its own neighbour and zero is
* therefore free -- WITHOUT THAT CONVENTION A ZEROED NODE POINTS AT ITSELF and
* the first walk never terminates. An INDEX link is absent when zero by
* default, matching D_TEST_NO_CALLABLE and D_TEST_NO_KEY, or when all-ones if
* the descriptor sets D_NEST_FLAG_INDEX_SENTINEL_MAX -- the two conventions
* both exist in the wild and neither can be inferred from a width.
*
*   THIS IS THE FOURTH RESERVED-ZERO CONVENTION in the tree, after
* d_test_key_id, d_test_callable_id and d_option's interned key. It is spelled
* explicitly here rather than assumed, but the right end state is the shared
* interned-handle vocabulary, and this header should defer to it when that
* lands.
*
*
* WHAT IS NOT CHECKED
* ===================
*   THAT THE GRAPH IS A TREE. Nothing here detects a cycle. A sibling chain
* that loops walks forever, exactly as a hand-written walker would, so the
* view carries `walk_limit` and every chain walk honours it. Zero means
* unbounded and is the right setting only for a structure whose acyclicity is
* an invariant maintained elsewhere -- which is the same standing this module
* gives D_LOOKUP_FLAG_SORTED, a claim the module trusts and cannot verify.
*
*
* A KNOWN DUPLICATION, NAMED RATHER THAN HIDDEN
* =============================================
*   D_NEST_FLAG_PAYLOAD_INDIRECT and lookup.h's D_LOOKUP_FLAG_KEY_IS_POINTER
* are the same bit of information: does this field hold the datum, or a
* pointer to it? That is a property of the FIELD, not of the search or of the
* walk, so it belongs in kv.h as D_KV_FLAG_INDIRECT with both consumers
* deferring to it. It is defined twice for now because promoting it edits
* three headers and this one was asked for alone. The change is small and
* should be made before a fourth consumer invents a third spelling.
*
*
* path:      /inc/djinterp/c/util/nest.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.05
******************************************************************************/

#ifndef DJINTERP_C_NEST_
#define DJINTERP_C_NEST_ 1

// std
// c
#include <stddef.h>
#include <stdint.h>
#include <string.h>
// djinterp
#include "../djinterp.h"
#include "../meta/kv.h"


D_EXTERN_C_BEGIN


// I.     link kinds and slots

// D_NEST_KIND_NONE
//   constant: the slot does not exist on this node type. Resolving it always
// yields NULL, which is how a node without a parent pointer is described
// rather than special-cased.
#define D_NEST_KIND_NONE                ((uint16_t)0u)

// D_NEST_KIND_POINTER
//   constant: the field holds the neighbour's address. Absent when NULL.
#define D_NEST_KIND_POINTER             ((uint16_t)1u)

// D_NEST_KIND_INDEX
//   constant: the field holds an unsigned subscript into the view's arena.
// Absent at the sentinel -- zero, or all-ones under
// D_NEST_FLAG_INDEX_SENTINEL_MAX.
#define D_NEST_KIND_INDEX               ((uint16_t)2u)

// D_NEST_KIND_OFFSET
//   constant: the field holds a signed byte displacement from the ARENA BASE.
// Absent at zero. Survives the arena being relocated; does not survive one
// node being copied out of it.
#define D_NEST_KIND_OFFSET              ((uint16_t)3u)

// D_NEST_KIND_SELF_OFFSET
//   constant: the field holds a signed byte displacement from THE NODE
// ITSELF. Absent at zero. The only kind that survives a subtree being written
// to disk and read back at a different address, which is why a mapped tree
// uses it and pays the extra add on every step.
#define D_NEST_KIND_SELF_OFFSET         ((uint16_t)4u)

// D_NEST_KIND_RESOLVER
//   constant: the neighbour is computed. The function comes from the node's
// own field when the link has a non-zero width, and from the view otherwise --
// one kind, two sources, disambiguated by whether there is a field to read.
#define D_NEST_KIND_RESOLVER            ((uint16_t)5u)

// D_NEST_KIND_COUNT
//   constant: one past the last defined kind. A descriptor carrying anything
// at or above this is malformed and resolves to NULL.
#define D_NEST_KIND_COUNT               ((uint16_t)6u)

// D_NEST_SLOT_PARENT
//   constant: the upward link. Absent on a root and on any node type that
// does not keep one.
#define D_NEST_SLOT_PARENT              ((uint32_t)0u)

// D_NEST_SLOT_FIRST_CHILD
//   constant: the first child, in the child/sibling shape.
#define D_NEST_SLOT_FIRST_CHILD         ((uint32_t)1u)

// D_NEST_SLOT_LAST_CHILD
//   constant: the last child, kept by node types that append in constant
// time. Optional; absent means "walk from first".
#define D_NEST_SLOT_LAST_CHILD          ((uint32_t)2u)

// D_NEST_SLOT_NEXT_SIBLING
//   constant: the next sibling, in the child/sibling shape.
#define D_NEST_SLOT_NEXT_SIBLING        ((uint32_t)3u)

// D_NEST_SLOT_PREV_SIBLING
//   constant: the previous sibling. Optional; its presence is what makes a
// sibling chain doubly linked.
#define D_NEST_SLOT_PREV_SIBLING        ((uint32_t)4u)

// D_NEST_SLOT_COUNT
//   constant: the number of slots a descriptor carries.
#define D_NEST_SLOT_COUNT               ((uint32_t)5u)

// D_NEST_FLAG_NONE
//   constant: no flags.
#define D_NEST_FLAG_NONE                ((uint16_t)0u)

// D_NEST_FLAG_PAYLOAD_INDIRECT
//   constant: the payload field holds a POINTER TO the payload rather than
// the payload itself -- `void* user_data`, not `char data[32]`.
#define D_NEST_FLAG_PAYLOAD_INDIRECT    ((uint16_t)(1u << 0))

// D_NEST_FLAG_INDEX_SENTINEL_MAX
//   constant: an index link is absent at the all-ones value for its width
// rather than at zero. Set this for a zero-based arena; leave it clear for a
// one-based arena with index zero reserved.
#define D_NEST_FLAG_INDEX_SENTINEL_MAX  ((uint16_t)(1u << 1))

// D_NEST_FLAG_ARRAY_INDIRECT
//   constant: the child array field holds a POINTER TO the array rather than
// the array itself -- `struct node** children`, not `struct node* kids[8]`.
#define D_NEST_FLAG_ARRAY_INDIRECT      ((uint16_t)(1u << 2))

// D_NEST_FLAG_MASK
//   constant: every flag this version defines. Bits outside are reserved and
// written zero.
#define D_NEST_FLAG_MASK                ((uint16_t)0x0007u)

// fn_nest_resolve
//   type: a computed link. Returns the neighbour at `_slot` of `_node`, or
// NULL when there is none.
//   THE ONLY typedef IN THIS HEADER, and permitted because it names a function
// pointer. It exists because a function pointer CANNOT PORTABLY BE CARRIED IN
// A void*: conversion between object and function pointer types is not
// guaranteed by either standard, so a resolver stored in a node is read into
// this type and never into kv.h's pointer load.
typedef const void* (*fn_nest_resolve)(
    const void* _node,
    uint32_t    _slot,
    void*       _context
);


// II.    the link descriptor

// d_nest_link
//   struct: one neighbour slot -- where it sits, how wide it is, and what the
// bytes mean.
struct d_nest_link
{
    struct d_kv_field field;    // where the link sits within the node
    uint16_t          kind;     // D_NEST_KIND_*
    uint16_t          pad;      // reserved; written zero
};

// D_NEST_LINK_INIT
//   macro: the absent slot. A descriptor built from this and never touched
// describes a node with no neighbours, which resolves cleanly rather than
// faulting.
#define D_NEST_LINK_INIT    { D_KV_FIELD_INIT, D_NEST_KIND_NONE, 0u }

// D_NEST_LINK_OF
//   macro: a link from a member of a standard-layout node. The offset comes
// from offsetof and the width from sizeof, so a node whose links sit after a
// padded payload is walked correctly without the caller computing anything.
#define D_NEST_LINK_OF(record, member, kind)                                   \
{                                                                              \
    D_KV_FIELD_OF(record, member, 0), (uint16_t)(kind), 0u                     \
}

// D_NEST_LINK_OF_SIGNED
//   macro: the same for an offset link, whose field is a signed displacement
// and must be sign-extended on the way out of a narrow slot.
#define D_NEST_LINK_OF_SIGNED(record, member, kind)                            \
{                                                                              \
    D_KV_FIELD_OF_SIGNED(record, member, 0), (uint16_t)(kind), 0u              \
}

// d_nest_link_make
//   function: a link from loose parts, for a node layout discovered at run
// time rather than declared in a struct.
D_NODISCARD D_INLINE struct d_nest_link
d_nest_link_make(
    uint32_t _offset,
    uint32_t _size,
    uint16_t _kind,
    bool     _is_signed
)
{
    struct d_nest_link link = D_NEST_LINK_INIT;

    // a kind this version does not define describes nothing
    if (_kind >= D_NEST_KIND_COUNT)
    {
        return link;
    }

    link.field = d_kv_field_make(_offset,
                                 _size,
                                 (d_type_info16)0,
                                 _is_signed ? D_KV_FLAG_SIGNED
                                            : D_KV_FLAG_NONE);
    link.kind  = _kind;

    return link;
}

// d_nest_link_is_present
//   function: whether the slot exists on this node type at all. A RESOLVER
// link with no field is still present, because the view supplies the function.
D_NODISCARD D_INLINE bool
d_nest_link_is_present(
    const struct d_nest_link* _link
)
{
    // an absent kind is absent whatever its field says
    if ( (!_link) ||
         (_link->kind == D_NEST_KIND_NONE) ||
         (_link->kind >= D_NEST_KIND_COUNT) )
    {
        return false;
    }

    // a resolver needs no field of its own
    if (_link->kind == D_NEST_KIND_RESOLVER)
    {
        return true;
    }

    return !d_kv_field_is_empty(&_link->field);
}


// III.   the node descriptor

// d_nest_children
//   struct: the child-array shape -- an array of links and a count beside it.
// This is `d_file_tree_node`'s { children, count } pair described rather than
// walked by hand.
//   ZERO-WIDTH `array` MEANS THE NODE HAS NO CHILD ARRAY, which is how the
// child/sibling shape is spelled: it reaches its children through
// D_NEST_SLOT_FIRST_CHILD instead.
struct d_nest_children
{
    struct d_kv_field array;    // the array member, or the pointer to it
    struct d_kv_field count;    // where the child count sits
    uint32_t          stride;   // bytes between consecutive array slots
    uint16_t          kind;     // D_NEST_KIND_* for one array slot
    uint16_t          flags;    // D_NEST_FLAG_ARRAY_INDIRECT
};

// D_NEST_CHILDREN_INIT
//   macro: no child array.
#define D_NEST_CHILDREN_INIT                                                   \
{                                                                              \
    D_KV_FIELD_INIT, D_KV_FIELD_INIT, 0u, D_NEST_KIND_NONE, D_NEST_FLAG_NONE   \
}

// d_nest_desc
//   struct: the complete shape of one node type. PURE LAYOUT -- it holds no
// pointer, no arena and no runtime state, so it is a static const per node
// type and the compiler folds every walk through it.
//   `node_size` is the node's sizeof and doubles as the per-node capacity
// handed to kv.h, so a link field that would read past the end of a node is
// refused by the same bounds check that guards every other read.
struct d_nest_desc
{
    uint32_t               node_size;               // sizeof(node)
    uint16_t               flags;                   // D_NEST_FLAG_*
    uint16_t               pad;                     // reserved; written zero
    struct d_kv_field      payload;                 // where the payload sits
    struct d_nest_link     links[D_NEST_SLOT_COUNT];
    struct d_nest_children children;
};

// D_NEST_DESC_INIT
//   macro: a node with a size and nothing else -- no payload, no links, no
// children. Every accessor against it answers cleanly.
#define D_NEST_DESC_INIT(record)                                               \
{                                                                              \
    (uint32_t)sizeof(record), D_NEST_FLAG_NONE, 0u, D_KV_FIELD_INIT,           \
    { D_NEST_LINK_INIT, D_NEST_LINK_INIT, D_NEST_LINK_INIT,                    \
      D_NEST_LINK_INIT, D_NEST_LINK_INIT },                                    \
    D_NEST_CHILDREN_INIT                                                       \
}

// D_NEST_DESC_CHILD_SIBLING
//   macro: the classic n-ary shape -- one first-child link and one
// next-sibling link, arbitrary arity at two links a node.
#define D_NEST_DESC_CHILD_SIBLING(record, payload_member, payload_info,        \
                                  parent_member, first_child_member,           \
                                  next_sibling_member, kind, node_flags)       \
{                                                                              \
    (uint32_t)sizeof(record),                                                  \
    (uint16_t)((node_flags) & D_NEST_FLAG_MASK), 0u,                           \
    D_KV_FIELD_OF(record, payload_member, payload_info),                       \
    { D_NEST_LINK_OF(record, parent_member, kind),                             \
      D_NEST_LINK_OF(record, first_child_member, kind),                        \
      D_NEST_LINK_INIT,                                                        \
      D_NEST_LINK_OF(record, next_sibling_member, kind),                       \
      D_NEST_LINK_INIT },                                                      \
    D_NEST_CHILDREN_INIT                                                       \
}

// D_NEST_DESC_CHILD_ARRAY
//   macro: the array shape -- a contiguous run of child links and a count.
// This is `d_file_tree_node`; `array_member` is the `struct node** children`
// and `count_member` is the `size_t count` beside it, so `node_flags` will
// normally carry D_NEST_FLAG_ARRAY_INDIRECT.
#define D_NEST_DESC_CHILD_ARRAY(record, payload_member, payload_info,          \
                                parent_member, array_member, count_member,     \
                                slot_type, kind, node_flags)                   \
{                                                                              \
    (uint32_t)sizeof(record),                                                  \
    (uint16_t)((node_flags) & D_NEST_FLAG_MASK), 0u,                           \
    D_KV_FIELD_OF(record, payload_member, payload_info),                       \
    { D_NEST_LINK_OF(record, parent_member, kind),                             \
      D_NEST_LINK_INIT, D_NEST_LINK_INIT,                                      \
      D_NEST_LINK_INIT, D_NEST_LINK_INIT },                                    \
    { D_KV_FIELD_OF(record, array_member, 0),                                  \
      D_KV_FIELD_OF(record, count_member, 0),                                  \
      (uint32_t)sizeof(slot_type), (uint16_t)(kind),                           \
      (uint16_t)((node_flags) & D_NEST_FLAG_ARRAY_INDIRECT) }                  \
}

// d_nest_desc_link
//   function: the descriptor for one slot, or NULL when the slot number is
// out of range.
D_NODISCARD D_INLINE const struct d_nest_link*
d_nest_desc_link(
    const struct d_nest_desc* _desc,
    uint32_t                  _slot
)
{
    // a slot this version does not define describes nothing
    if ( (!_desc) ||
         (_slot >= D_NEST_SLOT_COUNT) )
    {
        return NULL;
    }

    return &_desc->links[_slot];
}

// d_nest_desc_has_children_array
//   function: whether the node reaches its children through an array rather
// than through a sibling chain.
D_NODISCARD D_INLINE bool
d_nest_desc_has_children_array(
    const struct d_nest_desc* _desc
)
{
    return ( (_desc != NULL) &&
             (!d_kv_field_is_empty(&_desc->children.array)) &&
             (_desc->children.stride != 0u) );
}


// IV.    the bound view
//   The descriptor says what a node looks like. The view says where the nodes
// are. They are separate because the first is a compile-time constant per node
// type and the second is runtime state, and merging them would make every
// descriptor un-const.

// d_nest_view
//   struct: a descriptor bound to an arena and a resolver context. The arena
// is BORROWED and is only consulted by INDEX and OFFSET links; a pointer-
// linked tree leaves it NULL.
struct d_nest_view
{
    const struct d_nest_desc* desc;          // borrowed layout
    const void*               arena;         // base for INDEX / OFFSET links
    void*                     context;       // handed to a resolver
    fn_nest_resolve           resolve;       // view-level resolver, or NULL
    size_t                    arena_count;   // nodes in the arena; 0 unknown
    size_t                    walk_limit;    // max chain steps; 0 unbounded
    uint32_t                  arena_stride;  // bytes per arena node
    uint16_t                  flags;         // reserved; written zero
    uint16_t                  pad;           // reserved; written zero
};

// D_NEST_VIEW_INIT
//   macro: an unbound view. Every accessor against it answers NULL or zero.
#define D_NEST_VIEW_INIT                                                       \
{                                                                              \
    NULL, NULL, NULL, NULL, 0u, 0u, 0u, 0u, 0u                                 \
}

// d_nest_view_make
//   function: bind a descriptor to an arena. `_arena` and `_arena_count` may
// be NULL and zero for a pointer-linked structure, which consults neither.
//   THE ARENA STRIDE DEFAULTS TO THE NODE SIZE, and differs only when the
// nodes are embedded in something larger.
D_NODISCARD D_INLINE struct d_nest_view
d_nest_view_make(
    const struct d_nest_desc* _desc,
    const void*               _arena,
    size_t                    _arena_count,
    uint32_t                  _arena_stride,
    size_t                    _walk_limit
)
{
    struct d_nest_view view = D_NEST_VIEW_INIT;

    // a view without a layout describes nothing
    if ( (!_desc) ||
         (_desc->node_size == 0u) )
    {
        return view;
    }

    view.desc         = _desc;
    view.arena        = _arena;
    view.arena_count  = _arena_count;
    view.arena_stride = (_arena_stride != 0u) ? _arena_stride
                                              : _desc->node_size;
    view.walk_limit   = _walk_limit;

    return view;
}

// d_nest_view_is_valid
//   function: whether the view carries a layout it can walk.
D_NODISCARD D_INLINE bool
d_nest_view_is_valid(
    const struct d_nest_view* _view
)
{
    return ( (_view != NULL) &&
             (_view->desc != NULL) &&
             (_view->desc->node_size != 0u) );
}

// d_nest_walk_limit
//   function: the step budget for a chain walk. The view's explicit limit if
// it set one, else the arena size if it knows one, else zero for unbounded.
//   A CHAIN THAT LOOPS IS NOT DETECTED, only bounded. Acyclicity is the
// structure's invariant, and a walker that verified it would cost a visited
// set on every traversal.
D_NODISCARD D_INLINE size_t
d_nest_walk_limit(
    const struct d_nest_view* _view
)
{
    // an unbound view walks nothing, so any budget serves
    if (!d_nest_view_is_valid(_view))
    {
        return 0u;
    }

    // an explicit budget wins over an inferred one
    if (_view->walk_limit != 0u)
    {
        return _view->walk_limit;
    }

    return _view->arena_count;
}


// V.     link resolution

// d_nest_arena_at
//   function: the arena node at `_index`, bounded by `arena_count` when the
// view knows one. NULL when the view has no arena, which is what a
// pointer-linked structure gets if it is handed an index link by mistake.
D_NODISCARD D_INLINE const void*
d_nest_arena_at(
    const struct d_nest_view* _view,
    uint64_t                  _index
)
{
    // an index means nothing without an arena to subscript
    if ( (!d_nest_view_is_valid(_view)) ||
         (!_view->arena)                ||
         (_view->arena_stride == 0u) )
    {
        return NULL;
    }

    // refuse an index past the end when the extent is known
    if ( (_view->arena_count != 0u) &&
         (_index >= (uint64_t)_view->arena_count) )
    {
        return NULL;
    }

    return (const void*)(((const unsigned char*)_view->arena) +
                         (size_t)(_index * (uint64_t)_view->arena_stride));
}

// d_internal_nest_index_sentinel
//   function: the index value that means "no neighbour" for a slot of this
// width, under the descriptor's convention.
//   THE ALL-ONES SENTINEL IS COMPUTED FROM THE FIELD'S WIDTH, not from the
// carrier's: a uint16_t slot is absent at 0xFFFF, and comparing it against
// UINT64_MAX would never match. The eight-byte case is separate because
// shifting by the full width of the carrier is undefined.
D_NODISCARD D_INLINE uint64_t
d_internal_nest_index_sentinel(
    uint32_t _size,
    uint16_t _flags
)
{
    // the zero convention does not depend on the width
    if ((_flags & D_NEST_FLAG_INDEX_SENTINEL_MAX) == 0u)
    {
        return 0u;
    }

    // a full-width slot's all-ones value cannot be reached by shifting
    if (_size >= 8u)
    {
        return ~(uint64_t)0;
    }

    return ((((uint64_t)1) << (_size * 8u)) - 1u);
}

// d_internal_nest_resolve_kind
//   function: turn the bytes of one link field into a neighbour address. The
// switch is the whole module: everything else is bookkeeping around it.
//   `_capacity` BOUNDS THE BLOCK THE LINK SITS IN, which is the node for an
// ordinary slot and the child array for an array slot. Passing the node's size
// for an array that lives elsewhere would refuse every child past the node's
// own width.
D_NODISCARD D_INLINE const void*
d_internal_nest_resolve_kind(
    const struct d_nest_view* _view,
    const void*               _base,
    uint32_t                  _capacity,
    const struct d_nest_link* _link,
    uint32_t                  _slot
)
{
    uint64_t        raw_unsigned;
    int64_t         raw_signed;
    fn_nest_resolve resolver;

    // a resolver reads its function from the node, or from the view when the
    // node keeps none
    if (_link->kind == D_NEST_KIND_RESOLVER)
    {
        resolver = _view->resolve;

        if (!d_kv_field_is_empty(&_link->field))
        {
            //   A FUNCTION POINTER IS NOT AN OBJECT POINTER. kv.h's pointer
            // load returns void*, and converting that to a function type is
            // not guaranteed by either standard, so the bytes are copied
            // straight into the function pointer type instead.
            if (!d_kv_fits(_link->field.offset,
                           (uint32_t)sizeof(fn_nest_resolve),
                           _capacity))
            {
                return NULL;
            }

            memcpy(&resolver,
                   ((const unsigned char*)_base) + _link->field.offset,
                   sizeof(fn_nest_resolve));
        }

        if (!resolver)
        {
            return NULL;
        }

        return resolver(_base, _slot, _view->context);
    }

    // a pointer link is the neighbour's address outright
    if (_link->kind == D_NEST_KIND_POINTER)
    {
        return d_kv_load_pointer(_base, _link->field.offset, _capacity);
    }

    // an index link subscripts the arena, absent at its sentinel
    if (_link->kind == D_NEST_KIND_INDEX)
    {
        raw_unsigned = d_kv_load_unsigned(_base,
                                          _link->field.offset,
                                          _link->field.size,
                                          _capacity);

        if (raw_unsigned ==
                d_internal_nest_index_sentinel(_link->field.size,
                                               _view->desc->flags))
        {
            return NULL;
        }

        return d_nest_arena_at(_view, raw_unsigned);
    }

    raw_signed = d_kv_load_signed(_base,
                                  _link->field.offset,
                                  _link->field.size,
                                  _capacity);

    // zero is the absent displacement, because no node is its own neighbour
    if (raw_signed == 0)
    {
        return NULL;
    }

    // a self-relative displacement is measured from the node
    if (_link->kind == D_NEST_KIND_SELF_OFFSET)
    {
        return (const void*)(((const unsigned char*)_base) + raw_signed);
    }

    // an arena-relative displacement needs an arena to measure from
    if (!_view->arena)
    {
        return NULL;
    }

    return (const void*)(((const unsigned char*)_view->arena) + raw_signed);
}

// d_nest_resolve
//   function: the neighbour at `_slot` of `_node`, or NULL when there is none.
// THE ONE ENTRY POINT every accessor below is written in terms of.
D_NODISCARD D_INLINE const void*
d_nest_resolve(
    const struct d_nest_view* _view,
    const void*               _node,
    uint32_t                  _slot
)
{
    const struct d_nest_link* link;

    // an unbound view or an absent node has no neighbours
    if ( (!d_nest_view_is_valid(_view)) ||
         (!_node) )
    {
        return NULL;
    }

    link = d_nest_desc_link(_view->desc, _slot);

    // a slot this node type does not keep resolves cleanly to nothing
    if (!d_nest_link_is_present(link))
    {
        return NULL;
    }

    return d_internal_nest_resolve_kind(_view,
                                        _node,
                                        _view->desc->node_size,
                                        link,
                                        _slot);
}

// d_nest_parent
//   function: the node's parent, or NULL for a root or a node type that keeps
// no upward link.
D_NODISCARD D_INLINE const void*
d_nest_parent(
    const struct d_nest_view* _view,
    const void*               _node
)
{
    return d_nest_resolve(_view, _node, D_NEST_SLOT_PARENT);
}

// d_nest_first_child
//   function: the node's first child in the sibling shape.
D_NODISCARD D_INLINE const void*
d_nest_first_child(
    const struct d_nest_view* _view,
    const void*               _node
)
{
    return d_nest_resolve(_view, _node, D_NEST_SLOT_FIRST_CHILD);
}

// d_nest_next_sibling
//   function: the next node in the sibling chain.
D_NODISCARD D_INLINE const void*
d_nest_next_sibling(
    const struct d_nest_view* _view,
    const void*               _node
)
{
    return d_nest_resolve(_view, _node, D_NEST_SLOT_NEXT_SIBLING);
}

// d_nest_prev_sibling
//   function: the previous node in the sibling chain, on a doubly linked one.
D_NODISCARD D_INLINE const void*
d_nest_prev_sibling(
    const struct d_nest_view* _view,
    const void*               _node
)
{
    return d_nest_resolve(_view, _node, D_NEST_SLOT_PREV_SIBLING);
}

// d_nest_is_root
//   function: whether the node has no parent. A node type that keeps no
// parent link reports every node as a root, which is accurate: it cannot tell.
D_NODISCARD D_INLINE bool
d_nest_is_root(
    const struct d_nest_view* _view,
    const void*               _node
)
{
    return (d_nest_parent(_view, _node) == NULL);
}


// VI.    payload access

// d_nest_payload
//   function: the address of the node's payload, honouring
// D_NEST_FLAG_PAYLOAD_INDIRECT. An inline payload is the bytes at the field;
// an indirect one is what the pointer at the field leads to.
D_NODISCARD D_INLINE const void*
d_nest_payload(
    const struct d_nest_view* _view,
    const void*               _node
)
{
    // a node with no payload field is pure topology
    if ( (!d_nest_view_is_valid(_view))                 ||
         (!_node)                                       ||
         (d_kv_field_is_empty(&_view->desc->payload)) )
    {
        return NULL;
    }

    // an indirect payload is a pointer stored in the node; load it, do not
    // cast the node
    if ((_view->desc->flags & D_NEST_FLAG_PAYLOAD_INDIRECT) != 0u)
    {
        return d_kv_load_pointer(_node,
                                 _view->desc->payload.offset,
                                 _view->desc->node_size);
    }

    return d_kv_at_const(_node, _view->desc->payload.offset);
}

// d_nest_payload_size
//   function: the payload's declared width. For an indirect payload this is
// the width of what the pointer leads to as the descriptor declares it, which
// the descriptor cannot verify and the caller must have got right.
D_NODISCARD D_INLINE uint32_t
d_nest_payload_size(
    const struct d_nest_view* _view
)
{
    // an unbound view carries no payload width
    if (!d_nest_view_is_valid(_view))
    {
        return 0u;
    }

    return _view->desc->payload.size;
}

// d_nest_payload_read
//   function: copy an INLINE payload out. Refuses an indirect one, because
// the node's capacity does not bound bytes that live somewhere else and
// copying them under it would be a bounds check that checks nothing.
D_NODISCARD D_INLINE bool
d_nest_payload_read(
    const struct d_nest_view* _view,
    const void*               _node,
    void*                     _out,
    size_t                    _out_size
)
{
    // refuse an unbound view, an absent node, or a payload with no width
    if ( (!d_nest_view_is_valid(_view))                 ||
         (!_node)                                       ||
         (d_kv_field_is_empty(&_view->desc->payload)) )
    {
        return false;
    }

    // an indirect payload is not bounded by the node and is not copied here
    if ((_view->desc->flags & D_NEST_FLAG_PAYLOAD_INDIRECT) != 0u)
    {
        return false;
    }

    return d_kv_field_read(&_view->desc->payload,
                           _node,
                           _view->desc->node_size,
                           _out,
                           _out_size);
}


// VII.   children
//   Both shapes answer the same two questions, so a caller walking a tree need
// not know which shape it has.

// d_nest_child_count
//   function: how many children the node has. Read from the count member in
// the array shape; walked in the sibling shape, bounded by the view's step
// budget.
D_NODISCARD D_INLINE size_t
d_nest_child_count(
    const struct d_nest_view* _view,
    const void*               _node
)
{
    const void* child;
    size_t      count;
    size_t      limit;

    // an unbound view or an absent node has no children
    if ( (!d_nest_view_is_valid(_view)) ||
         (!_node) )
    {
        return 0u;
    }

    // the array shape keeps the count beside the array
    if (d_nest_desc_has_children_array(_view->desc))
    {
        return (size_t)d_kv_load_unsigned(_node,
                                          _view->desc->children.count.offset,
                                          _view->desc->children.count.size,
                                          _view->desc->node_size);
    }

    count = 0u;
    limit = d_nest_walk_limit(_view);
    child = d_nest_first_child(_view, _node);

    // walk the sibling chain, stopping at the budget if there is one
    while (child)
    {
        ++count;

        if ( (limit != 0u) &&
             (count >= limit) )
        {
            break;
        }

        child = d_nest_next_sibling(_view, child);
    }

    return count;
}

// d_nest_child_at
//   function: the child at `_index`. Constant time in the array shape and
// linear in the sibling shape, which is the cost of the shape rather than of
// this function -- a caller iterating every child uses D_NEST_FOR_EACH_CHILD
// and pays it once.
D_NODISCARD D_INLINE const void*
d_nest_child_at(
    const struct d_nest_view* _view,
    const void*               _node,
    size_t                    _index
)
{
    struct d_nest_link slot_link;
    const void*        child;
    const void*        array;
    size_t             count;
    size_t             step;
    size_t             limit;
    uint64_t           slot_offset;
    uint64_t           capacity;

    // an unbound view or an absent node has no children
    if ( (!d_nest_view_is_valid(_view)) ||
         (!_node) )
    {
        return NULL;
    }

    // the array shape subscripts directly
    if (d_nest_desc_has_children_array(_view->desc))
    {
        count = d_nest_child_count(_view, _node);

        if (_index >= count)
        {
            return NULL;
        }

        //   AN INDIRECT ARRAY IS A POINTER IN THE NODE; a direct one is the
        // slots themselves. The distinction is exactly kv.h's indirect field
        // and is why the flag exists on the descriptor.
        //   THE CAPACITY CHANGES WITH THE BASE, and that is the point. An
        // indirect array lives outside the node, so bounding its reads by the
        // node's size would refuse every child past the node's own width;
        // its extent is the count member times the slot stride. A direct
        // array is inside the node and keeps the node's bound.
        if ((_view->desc->children.flags & D_NEST_FLAG_ARRAY_INDIRECT) != 0u)
        {
            array = d_kv_load_pointer(_node,
                                      _view->desc->children.array.offset,
                                      _view->desc->node_size);
            slot_offset = 0u;
            capacity    = (uint64_t)count *
                              (uint64_t)_view->desc->children.stride;
        }
        else
        {
            array       = _node;
            slot_offset = (uint64_t)_view->desc->children.array.offset;
            capacity    = (uint64_t)_view->desc->node_size;
        }

        if (!array)
        {
            return NULL;
        }

        slot_offset += (uint64_t)_index *
                           (uint64_t)_view->desc->children.stride;

        // an array too large to address with a 32-bit offset is refused
        // rather than wrapped
        if ( (slot_offset > (uint64_t)UINT32_MAX) ||
             (capacity > (uint64_t)UINT32_MAX) )
        {
            return NULL;
        }

        slot_link = d_nest_link_make(
            (uint32_t)slot_offset,
            _view->desc->children.stride,
            _view->desc->children.kind,
            ( (_view->desc->children.kind == D_NEST_KIND_OFFSET) ||
              (_view->desc->children.kind == D_NEST_KIND_SELF_OFFSET) ));

        return d_internal_nest_resolve_kind(_view,
                                            array,
                                            (uint32_t)capacity,
                                            &slot_link,
                                            D_NEST_SLOT_FIRST_CHILD);
    }

    step  = 0u;
    limit = d_nest_walk_limit(_view);
    child = d_nest_first_child(_view, _node);

    // walk the sibling chain to the index, stopping at the budget
    while (child)
    {
        if (step == _index)
        {
            return child;
        }

        ++step;

        if ( (limit != 0u) &&
             (step >= limit) )
        {
            break;
        }

        child = d_nest_next_sibling(_view, child);
    }

    return NULL;
}

// d_nest_is_leaf
//   function: whether the node has no children, in either shape.
D_NODISCARD D_INLINE bool
d_nest_is_leaf(
    const struct d_nest_view* _view,
    const void*               _node
)
{
    return (d_nest_child_at(_view, _node, 0u) == NULL);
}


// VIII.  iteration

// D_NEST_FOR_EACH_CHILD
//   macro: walk every child of a node. `index_var` must be a `size_t` and
// `var` a `const void*`, both already declared.
//   THE ARRAY SHAPE PAYS A SUBSCRIPT A STEP AND THE SIBLING SHAPE PAYS A
// WALK, so this is quadratic on a sibling chain. Use
// D_NEST_FOR_EACH_SIBLING_CHILD when the shape is known to be a chain and the
// child count is large.
#define D_NEST_FOR_EACH_CHILD(view, node, index_var, var)                      \
    for ((index_var) = 0u,                                                     \
             (var) = d_nest_child_at((view), (node), 0u);                      \
         (var) != NULL;                                                        \
         ++(index_var),                                                        \
             (var) = d_nest_child_at((view), (node), (index_var)))

// D_NEST_FOR_EACH_SIBLING_CHILD
//   macro: walk the sibling chain in linear total time. VALID ONLY ON THE
// SIBLING SHAPE -- on an array node the first-child link is absent and the
// loop body never runs, which is a silent no-op rather than a diagnostic, and
// is why the general form above is the default.
#define D_NEST_FOR_EACH_SIBLING_CHILD(view, node, var)                         \
    for ((var) = d_nest_first_child((view), (node));                           \
         (var) != NULL;                                                        \
         (var) = d_nest_next_sibling((view), (var)))

// D_NEST_FOR_EACH_ANCESTOR
//   macro: walk from a node's parent to its root.
#define D_NEST_FOR_EACH_ANCESTOR(view, node, var)                              \
    for ((var) = d_nest_parent((view), (node));                                \
         (var) != NULL;                                                        \
         (var) = d_nest_parent((view), (var)))


// IX.    layout assertions
//   d_nest_desc IS PURE LAYOUT and holds no pointer or size_t, so its exact
// size and offsets are the same on every target and are asserted literally.
// d_nest_view holds four pointers and two size_t, so its offsets are not, and
// what is asserted there is member ordering and the absence of interior
// padding -- a literal there would assert the host rather than the layout.

D_STATIC_ASSERT(sizeof(struct d_nest_link) == 16,
                "d_nest_link layout drift: expected 16 bytes");
D_STATIC_ASSERT(offsetof(struct d_nest_link, field) == 0,
                "d_nest_link layout drift: field must lead");
D_STATIC_ASSERT(offsetof(struct d_nest_link, kind) == 12,
                "d_nest_link layout drift: kind");

D_STATIC_ASSERT(sizeof(struct d_nest_children) == 32,
                "d_nest_children layout drift: expected 32 bytes");
D_STATIC_ASSERT(offsetof(struct d_nest_children, array) == 0,
                "d_nest_children layout drift: array must lead");
D_STATIC_ASSERT(offsetof(struct d_nest_children, stride) == 24,
                "d_nest_children layout drift: stride");

D_STATIC_ASSERT(offsetof(struct d_nest_desc, node_size) == 0,
                "d_nest_desc layout drift: node_size must lead");
D_STATIC_ASSERT(sizeof(struct d_nest_desc) ==
                    ( sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t) +
                      sizeof(struct d_kv_field) +
                      (D_NEST_SLOT_COUNT * sizeof(struct d_nest_link)) +
                      sizeof(struct d_nest_children) ),
                "d_nest_desc: a member width has introduced padding");

D_STATIC_ASSERT(offsetof(struct d_nest_view, desc) == 0,
                "d_nest_view layout drift: desc must lead");
D_STATIC_ASSERT(offsetof(struct d_nest_view, arena) <
                    offsetof(struct d_nest_view, context),
                "d_nest_view layout drift: arena must precede context");
D_STATIC_ASSERT(sizeof(struct d_nest_view) ==
                    ( sizeof(const struct d_nest_desc*) + sizeof(const void*) +
                      sizeof(void*) + sizeof(fn_nest_resolve) +
                      sizeof(size_t) + sizeof(size_t) + sizeof(uint32_t) +
                      sizeof(uint16_t) + sizeof(uint16_t) ),
                "d_nest_view: a member width has introduced padding");

//   THE NODE SIZE MUST SERVE AS A kv CAPACITY. Every link read is bounded by
// it, so a narrower type here would turn those bounds checks into decoration.
D_STATIC_ASSERT(sizeof(((struct d_nest_desc*)0)->node_size) >=
                    sizeof(uint32_t),
                "d_nest_desc: node_size cannot bound a node");

//   THE SLOT NUMBERS INDEX THE ARRAY. d_nest_desc_link trusts the relation, so
// it is stated rather than assumed.
D_STATIC_ASSERT(D_NEST_SLOT_PREV_SIBLING < D_NEST_SLOT_COUNT,
                "d_nest_desc: a slot number is outside the link array");

//   THE FLAG SET IS CLOSED. The mask is only meaningful while every defined
// flag sits inside it.
D_STATIC_ASSERT(( D_NEST_FLAG_PAYLOAD_INDIRECT   |
                  D_NEST_FLAG_INDEX_SENTINEL_MAX |
                  D_NEST_FLAG_ARRAY_INDIRECT ) == D_NEST_FLAG_MASK,
                "d_nest_desc: a flag has escaped D_NEST_FLAG_MASK");


D_EXTERN_C_END


#endif  // DJINTERP_C_NEST_
