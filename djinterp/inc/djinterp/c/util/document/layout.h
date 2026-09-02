/******************************************************************************
* djinterp [c/util/document]                                          layout.h
*
* THE STRUCTURE TERM.  An annotated tree whose nodes are either an application
* of an operator to children, or a leaf carrying an atom.  Every node also
* carries a hint bag.
*
* THE CARRIER IS `d_cofree`, NOT A TYPE OF ITS OWN.  `free<F,A>` is a shape
* with holes and `cofree<F,A>` is a shape with a value at every node -- and a
* parsed document is the second: no gaps, an annotation everywhere, always at
* least one node.  So this module declares no tree type.  It declares what
* goes in `d_cofree::label` and the handful of functions that put it there and
* read it back.
*
* A LEAF HAS NO CHILDREN; AN APPLICATION MAY HAVE NONE, WHICH IS NOT THE SAME
* THING.  `section {}` is an empty application and `content "x"` is a leaf,
* and both have `child_count == 0`.  That is why `d_layout_label` carries an
* explicit `is_leaf` rather than letting the child count imply it, and why
* `d_layout_is_leaf` exists at all.  A comparison keyed on child count alone
* calls those two equal, which is the bug this arrangement exists to make
* impossible.
*
* NOTHING ALLOCATES.  Nodes, labels and bags all come from a caller-owned
* `d_layout_arena`, bumped and never individually freed.  THREE PARALLEL
* REGIONS AND NOT ONE, because the three have different lifetimes in the only
* way that matters here: a node is copied by value into its parent's child run
* while its label and bag stay put, so labels cannot live inside nodes.
*
* THE ATOM'S STRINGS ARE BORROWED, like everything else in this tier.  They
* point into the parse arena's text region, and a tree outliving that region
* is reading freed memory.  `d_layout_atom` is small and copyable by design:
* it is stored by value in the label.
*
* path:      /inc/djinterp/c/util/document/layout.h
* link(s):   TBA
* author(s): TBA                                            created: 2026.08.23
******************************************************************************/

#ifndef DJINTERP_C_UTIL_DOCUMENT_LAYOUT_
#define DJINTERP_C_UTIL_DOCUMENT_LAYOUT_ 1

#include <stddef.h>
#include <stdint.h>
#include "../../djinterp.h"
#include "../../functional/free.h"
#include "./document_common.h"

D_EXTERN_C_BEGIN


// d_layout_atom_kind
//   enum: which flavour of leaf.  THE NUMBERS ARE THE C++ ONES -- `layout_atom
// ::kind_t` declares body_ref, meta_ref, literal in that order, and the parity
// fixture casts one to the other directly (`static_cast<int32_t>(_cpp.atom()
// .kind) != _ca->kind`), so a renumbering here is a silent mis-comparison
// rather than a build error.
enum d_layout_atom_kind
{
    D_LAYOUT_ATOM_BODY_REF = 0,  // key names body content, resolved externally
    D_LAYOUT_ATOM_META_REF = 1,  // key names a metadata field
    D_LAYOUT_ATOM_LITERAL  = 2   // value is inline text, emitted verbatim
};

#define D_LAYOUT_ATOM_KIND_COUNT 3

// d_layout_atom
//   struct: what a leaf carries -- the boundary between structure and
// content.  A ref names content by KEY and is resolved at render time so the
// tree never embeds it; a literal carries its text directly.
//
//   ONLY ONE OF `key` / `value` IS LIVE, decided by `kind`, and the other is
// NULL.  The fixture reads it that way: literal takes `value`, everything
// else takes `key`.
struct d_layout_atom
{
    int32_t     kind;     // enum d_layout_atom_kind
    int32_t     reserved; // must be 0
    const char* key;      // body_ref / meta_ref: the name; else NULL
    const char* value;    // literal: the text; else NULL
};

// d_layout_label
//   struct: what sits in a node's `d_cofree::label`.  An application carries
// an operator; a leaf carries an atom; both carry the bag.
//
//   THE BAG IS A POINTER AND THE ATOM IS BY VALUE, which looks inconsistent
// and is not: a bag is a view over a run that the arena also owns, so two
// nodes can share one, while an atom is three words that nothing else refers
// to.
struct d_layout_label
{
    int32_t                        is_leaf;  // 1 leaf, 0 application
    int32_t                        op;       // application: the operator id
    struct d_layout_atom           atom;     // leaf: the atom
    const struct d_doc_attributes* bag;      // both: the hints, arena-owned
};

// d_layout_arena
//   struct: caller-owned storage for a tree.  Bumped, never individually
// freed; `d_layout_arena_rewind` is the only deallocation.
struct d_layout_arena
{
    struct d_cofree*         nodes;
    size_t                   node_capacity;
    size_t                   node_used;

    struct d_layout_label*   labels;
    size_t                   label_capacity;
    size_t                   label_used;

    struct d_doc_attributes* bags;
    size_t                   bag_capacity;
    size_t                   bag_used;
};


// I.     the arena
//   `d_layout_arena_init` answers 0 on a NULL arena or a NULL region with a
// non-zero capacity; every builder below answers NULL when its region is
// exhausted, which is how the parser reports D_LAYOUT_PARSE_OVERFLOW rather
// than writing past the end.
int32_t d_layout_arena_init(struct d_layout_arena*   _arena,
                            struct d_cofree*         _nodes,
                            size_t                   _node_capacity,
                            struct d_layout_label*   _labels,
                            size_t                   _label_capacity,
                            struct d_doc_attributes* _bags,
                            size_t                   _bag_capacity);
void    d_layout_arena_rewind(struct d_layout_arena* _arena);

// II.    atoms
//   Three constructors mirroring `layout_atom`'s three static factories.  By
// value, no arena: an atom is stored inside the label it belongs to.
struct d_layout_atom d_layout_body_ref(const char* _key);
struct d_layout_atom d_layout_meta_ref(const char* _key);
struct d_layout_atom d_layout_literal(const char* _value);

// III.   nodes
//   `d_layout_leaf` mirrors `leaf_node`, `d_layout_apply` mirrors
// `apply_node`.  Both take the bag BY VALUE and copy it into the arena, so a
// caller may build one on its own stack -- which `layout_parse.c` does.
//
//   `_children` IS COPIED, not referenced.  The parser builds a sequence in
// the frame of the block that owns it and hands the run over; the arena run
// outlives that frame.  Passing `_count` of 0 with a NULL `_children` builds
// the empty application, which is a real term and not an error.
struct d_cofree* d_layout_leaf(struct d_layout_arena*  _arena,
                               struct d_doc_attributes _bag,
                               struct d_layout_atom    _atom);
struct d_cofree* d_layout_apply(struct d_layout_arena*  _arena,
                                struct d_doc_attributes _bag,
                                int32_t                 _op,
                                const struct d_cofree*  _children,
                                size_t                  _count);

// IV.    observers
//   The read side, and the reason it is a function surface rather than field
// access: `d_cofree::label` is a `const void*` and every caller would
// otherwise write the same cast.  A wrong cast in one of them is a defect
// nothing catches.
//
//   ALL OF THESE TOLERATE A NULL NODE, answering the empty case -- 0, NULL,
// or an empty bag.  A tree walk that hits NULL has already failed somewhere
// else, and crashing in the observer hides where.
int32_t                        d_layout_is_leaf(const struct d_cofree* _node);
const struct d_layout_atom*    d_layout_atom_of(const struct d_cofree* _node);
int32_t                        d_layout_op_of(const struct d_cofree* _node);
const struct d_doc_attributes* d_layout_bag(const struct d_cofree* _node);
size_t                         d_layout_child_count(const struct d_cofree* _node);
const struct d_cofree*         d_layout_children(const struct d_cofree* _node);

// V.     layout assertions (Layout law: one declaration, asserted in both)
D_STATIC_ASSERT(D_LAYOUT_ATOM_LITERAL == 2,
                "the atom kinds are cast straight across from layout_atom::"
                "kind_t; renumbering makes the parity fixture compare the "
                "wrong two things without failing to build");
D_STATIC_ASSERT(offsetof(struct d_layout_label, op) == sizeof(int32_t),
                "d_layout_label layout drift");


D_EXTERN_C_END

#endif  // DJINTERP_C_UTIL_DOCUMENT_LAYOUT_
