/*******************************************************************************
* djinterp [c]                                                            free.h
*
* `free<F,A>` and `cofree<F,A>`: the two recursive carriers of Table 7.2.
*   `free<F,A> = mu X. (A + F X)` is a tree whose leaves are either a HOLE
* carrying an `A` or a NODE carrying a label and its children. The parity
* assessment calls it "the single most valuable C type in this whole plan --
* templates, open documents, and the parser all reduce to it", and the reason is
* the hole: a free tree is a structure with typed gaps in it, and `bind` is
* substitution into those gaps.
*   `cofree<F,A>` is the dual: every node carries an annotation, there are no
* holes, and there is always at least one node. Where free is "a shape with gaps
* to fill", cofree is "a shape with a value attached everywhere" -- which is what
* an annotated syntax tree is.
*
* F IS A POLYNOMIAL FUNCTOR, SPELLED AS A LABEL PLUS CHILDREN. C cannot take a
* functor as a parameter, so `F X` is lowered to the shape every F in the corpus
* actually has: a node carries an opaque `label` (the F's own data, borrowed) and
* an array of children. A unary F gives one child, a binary F two, a rose tree n.
* Anything expressible as "a tag and a list of sub-terms" fits, which covers the
* parser and document shapes the assessment names.
*
* NOTHING ALLOCATES. Nodes come from a caller-owned `d_free_arena`, bumped and
* never freed individually, so building a tree costs one array the caller already
* had to size. Folding uses a caller-supplied scratch region under stack
* discipline, so a catamorphism over an n-ary tree needs no heap either.
*
* RECURSION IS BOUNDED BY TREE DEPTH. The traversals here recurse, and the depth
* of the recursion is the depth of the tree. That is bounded in every corpus use
* (a document, a parse) but is not bounded in general; `d_free_depth` exists so a
* caller handling untrusted input can check before descending.
*
* TRAVERSE IS THE NARROW SLICE, AND ONLY THAT. The assessment asks for exactly
* this: "C needs `traverse` for one `T` (the document/parse shape) against two
* applicatives. That is two instances, not a matrix." `d_free_traverse` is that
* -- `T` is `free`, and the applicative arrives as a `d_applicative` dictionary,
* so `maybe` and `result` both work with no further instances. The general
* `Traversable` protocol is NOT here, and the .tex has still not scoped the
* narrow slice, so this implements what the assessment describes rather than
* what the chapter currently states.
*
*
* path:      /inc/djinterp/c/functional/free.h
* link(s):   TBA
* author(s): TBA                                             created: 2026.07.30
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_C_FUNCTIONAL_FREE_H
#define DJINTERP_C_FUNCTIONAL_FREE_H 1

// std
#include <stddef.h>
#include <string.h>
// djinterp
#include "../djinterp.h"
#include "./functional_common.h"
#include "./reducer.h"
#include "./functor.h"
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


// d_free
//   struct: one node of a free tree -- a hole carrying a value, or a node
// carrying a label and its children.
// `value`, `label` and `children` are all borrowed; the arena owns the nodes and
// the caller owns everything they point at.
struct d_free
{
    bool           is_hole;      // which of the two arms is live
    const void*    value;        // when a hole: the `A`
    const void*    label;        // when a node: the F's own data
    struct d_free* children;     // when a node: the sub-terms
    size_t         child_count;  // when a node: how many
};

// d_free_arena
//   struct: caller-owned node storage, bumped and never individually freed.
// Rewinding the whole arena is the only deallocation, which is what makes tree
// building allocation-free.
struct d_free_arena
{
    struct d_free* nodes;     // caller-owned array
    size_t         capacity;  // nodes available
    size_t         used;      // nodes taken
};

// fn_free_on_node
//   function pointer: the node arm of a catamorphism.
// `_child_results` is the array of already-folded child results, `_count` of
// them, each `_result_size` bytes wide. Writing into `_out` completes the node.
typedef bool (*fn_free_on_node)(const void* _label,
                                const void* _child_results,
                                size_t      _count,
                                size_t      _result_size,
                                void*       _out,
                                void*       _context);

// fn_free_graft
//   function pointer: the Kleisli arrow of the free monad, `A -> free<F,B>`.
// Returns the subtree a hole is replaced by, allocated from `_arena`, or NULL to
// leave the hole standing with `_value` carried across.
typedef struct d_free* (*fn_free_graft)(const void*          _value,
                                        struct d_free_arena* _arena,
                                        void*                _context);

// d_cofree
//   struct: one node of a cofree tree -- an annotation and its children.
// There is no hole arm: a cofree tree always has at least this node, which is
// what makes `extract` total.
struct d_cofree
{
    const void*      annotation;   // the value attached at this node
    const void*      label;        // the F's own data
    struct d_cofree* children;     // the sub-terms
    size_t           child_count;  // how many
};

// d_cofree_arena
//   struct: caller-owned node storage for cofree trees.
struct d_cofree_arena
{
    struct d_cofree* nodes;     // caller-owned array
    size_t           capacity;  // nodes available
    size_t           used;      // nodes taken
};

// d_value_arena
//   struct: caller-owned bump storage for the values a map or traverse
// produces. A mapped tree needs somewhere to put the new `B`s, and C has no
// place to return them by value.
struct d_value_arena
{
    unsigned char* bytes;       // caller-owned storage
    size_t         value_size;  // width of one value
    size_t         capacity;    // values that fit
    size_t         used;        // values taken
};

// I.     arenas
bool     d_value_arena_init(struct d_value_arena* _arena,
                            void*                 _bytes,
                            size_t                _value_size,
                            size_t                _capacity);
void*    d_value_arena_take(struct d_value_arena* _arena);
bool     d_free_arena_init(struct d_free_arena* _arena,
                           struct d_free*       _nodes,
                           size_t               _capacity);
void     d_free_arena_rewind(struct d_free_arena* _arena);
bool     d_cofree_arena_init(struct d_cofree_arena* _arena,
                             struct d_cofree*       _nodes,
                             size_t                 _capacity);
void     d_cofree_arena_rewind(struct d_cofree_arena* _arena);

// II.    free: construction
struct d_free* d_free_hole(struct d_free_arena* _arena,
                           const void*          _value);
struct d_free* d_free_node(struct d_free_arena* _arena,
                           const void*          _label,
                           struct d_free*       _children,
                           size_t               _child_count);
bool           d_free_is_hole(const struct d_free* _tree);

// III.   free: shape
size_t   d_free_size(const struct d_free* _tree);
size_t   d_free_depth(const struct d_free* _tree);
size_t   d_free_hole_count(const struct d_free* _tree);

// IV.    free: the catamorphism, and what is built on it
bool     d_free_fold(const struct d_free* _tree,
                     fn_transformer       _on_hole,
                     fn_free_on_node      _on_node,
                     void*                _context,
                     void*                _out,
                     size_t               _result_size,
                     void*                _scratch,
                     size_t               _scratch_size);
size_t   d_free_drive_holes(const struct d_free*     _tree,
                            const struct d_reducer*  _reducer,
                            struct d_reducing_state* _state,
                            size_t                   _value_size);

// V.     free: Functor and Monad over the hole type
struct d_free* d_free_map(const struct d_free*  _tree,
                          struct d_free_arena*  _arena,
                          fn_transformer        _transform,
                          void*                 _context,
                          struct d_value_arena* _values);
struct d_free* d_free_bind(const struct d_free* _tree,
                           struct d_free_arena* _arena,
                           fn_free_graft        _graft,
                           void*                _context);

// VI.    free: traverse, the narrow tier 4 slice
//   The applicative is fixed to the failure-shaped ones -- `maybe` and
// `result`, the two the assessment scopes -- because for those, collecting the
// effects is exactly "every visit must succeed". A general applicative would
// need the rebuilt tree to live inside the carrier, which is the (T, F) matrix
// the assessment says C should avoid.
bool     d_free_traverse(const struct d_free*  _tree,
                         fn_transformer        _visit,
                         void*                 _context,
                         struct d_free_arena*  _arena,
                         struct d_value_arena* _values,
                         struct d_free**       _out_tree);

// VII.   cofree
struct d_cofree* d_cofree_node(struct d_cofree_arena* _arena,
                               const void*            _annotation,
                               const void*            _label,
                               struct d_cofree*       _children,
                               size_t                 _child_count);
const void*      d_cofree_extract(const struct d_cofree* _tree);
size_t           d_cofree_size(const struct d_cofree* _tree);
size_t           d_cofree_depth(const struct d_cofree* _tree);
size_t           d_cofree_drive(const struct d_cofree*   _tree,
                                const struct d_reducer*  _reducer,
                                struct d_reducing_state* _state,
                                size_t                   _annotation_size);

// VIII.  layout assertions (Layout law: one declaration, asserted in both)
D_STATIC_ASSERT(offsetof(struct d_free_arena, capacity) == sizeof(void*),
                "d_free_arena layout drift");
D_STATIC_ASSERT(offsetof(struct d_cofree, label) == sizeof(void*),
                "d_cofree layout drift");


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_FUNCTIONAL_FREE_H
