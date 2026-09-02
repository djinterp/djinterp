/******************************************************************************
* djinterp [utility]                                           document_node.h
*
*   THE TREE, AND THE ONE FOLD OVER IT.  `struct d_doc_node` is the document
* of ch-documents.tex: the monomorphised free monad `Free F A`, where F is the
* ten-constructor block signature and A is the set of slot names.  A DOCUMENT
* IS A CLOSED TEMPLATE -- there is no second type.  The same node serves the
* builder's tree, the template skeleton, and the parser's output, which is the
* single-construction claim of ch-parsing section 2 taken literally.
*
*   WHY MONOMORPHISED (ch-documents.tex, ruling R4).  The general `Free F A`
* carrier is a separate work item and C does not get type-level genericity
* (goals section 11).  It is not needed here: F is CLOSED -- eleven
* constructors, fixed by the chapter -- so the free monad over it is an
* ordinary tagged node, and the three operations freeness forces are three
* ordinary functions.  Building the general carrier first would be notation.
*
*   NOTHING HERE ALLOCATES.  A node borrows its strings, its hints, and its
* child array; the caller owns all three.  Substitution, which genuinely must
* build nodes, takes a caller-supplied arena.  This is goals section 3
* ("storage strategy is chosen by the caller") and it is also why the whole of
* stage 0 is unblocked by the container substrate: a tree that borrows needs
* no vector, and a fold that streams needs no string.
*
*   THE FOLD IS WRITTEN ONCE.  d_doc_render is the only traversal in the
* subframework.  Markdown, markup, and plain text are three algebras over it,
* not three renderers.  A fourth dialect adds an algebra and no code here.
*
*   CONFORMS TO: body-document.tex (Struct(F), mu F, the free monad)
*             ch-documents.tex sections on the constructors and expansion
*
*   This line exists because it is almost the only one of its kind.  Of 269
* headers across every package in this conversion, exactly two name the
* document they implement -- `free.h`, which says it follows an assessment
* rather than the chapter, and this file.  A reader cannot tell a header
* built to a specification from one built to something else unless the
* header says, and silence is the overwhelming default.  Naming the source
* costs one line and is the only thing that makes the question answerable.
*
*   PORTABILITY:
*   C89-clean apart from <stdint.h>.  Depends on document_common.h and
* document_render_algebra.h.
*
*
* TABLE OF CONTENTS
* =================
* I.     the node
* II.    field roles by kind
* III.   construction
* IV.    inspection
* V.     substitution (the free monad's bind)
* VI.    expansion (repeat + interpolation)
* VII.   the fold
* VIII.  layout assertions
*
*
* path:      /inc/djinterp/c/util/document/document_node.h
* link(s):   TBA
* author(s): Agent B (structure)                           created: 2026.07.31
******************************************************************************/

#ifndef DJINTERP_C_UTIL_DOCUMENT_NODE_
#define DJINTERP_C_UTIL_DOCUMENT_NODE_ 1

#include <stddef.h>
#include <stdint.h>
#include "../../djinterp.h"
#include "djinterp/c/util/document/document_common.h"
#include "djinterp/c/util/document/document_render_algebra.h"

D_EXTERN_C_BEGIN



// D_INTERNAL_DOC_MAX_DEPTH
//   constant: the deepest nesting the fold will follow before returning
// D_DOC_RENDER_DEPTH_EXCEEDED.  A bound, not a limit of the format: it exists
// so that a malformed or cyclic tree cannot exhaust the stack.  Overridable
// through the dconfig cascade.

// D_INTERNAL_DOC_MAX_SCOPES
//   constant: how deep repeats may nest before expansion stops binding new
// scopes.  A deeper repeat still unrolls; its tokens simply fall through to
// the scalar lookup.  Degrade, never error.

// D_DOC_POINTS
//   macro: whole points as millipoints, so a caller writes D_DOC_POINTS(12)
// rather than 12000 and the unit is impossible to get wrong.
#define D_DOC_POINTS(n)  ((int32_t)(n) * 1000)

// D_DOC_FLAG_ORDERED
//   constant: set on a list whose items are numbered.  Meaningless on every
// other kind.
#define D_DOC_FLAG_ORDERED  0x00000001u


// ===========================================================================
// I.     the node
// ===========================================================================

// d_doc_node
//   struct: one node of a document.  Flat rather than a union, for the same
// reason the C++ face is flat: eleven shapes over one field set costs a few
// unused words per node and buys one traversal, one layout assertion, and one
// serialised form.  Every pointer BORROWS; the node owns nothing and may be
// copied freely.  A zero-initialised node is a well-formed empty paragraph.
struct d_doc_node
{
    // attrs
    //   the hint bag, canonically ordered.  Borrowed.
    struct d_doc_attributes         attrs;

    // children
    //   a borrowed array of `child_count` borrowed node pointers.  NULL when
    // child_count is zero; a non-zero count over a NULL array is malformed.
    const struct d_doc_node* const* children;

    // text / secondary
    //   the node's two string fields; see section II for what each holds per
    // kind.  Borrowed and NUL-terminated; NULL reads as empty.
    const char*                     text;
    const char*                     secondary;

    // millipoints
    //   the vertical space REQUEST, on D_DOC_KIND_SPACE, in THOUSANDTHS OF A
    // POINT.  A request, never a measurement: nothing in this subframework
    // knows how tall a rendered node is.
    //
    //   IT IS AN INTEGER ON PURPOSE.  This was a double.  It was the only
    // floating-point field in the subframework, and therefore the only one
    // that could not be byte-compared across two languages, printed
    // identically by both, or compared for equality without a caveat -- for a
    // quantity nobody needs sub-milli precision in.  Fixed point removes the
    // whole question rather than deferring it to the floating-point policy.
    int32_t                         millipoints;

    uint32_t                        child_count;
    uint32_t                        level;   // heading depth; 1 is outermost
    uint32_t                        kind;    // enum d_doc_node_kind
    uint32_t                        flags;   // D_DOC_FLAG_*
    uint32_t                        reserved;  // pad; must be 0
};


// ===========================================================================
// II.    field roles by kind
// ===========================================================================
//   `text` is always the node's PRIMARY string -- the one that identifies or
// labels it.  `secondary` is always the payload that goes with it.  Reading
// the table below is the fastest way to understand the node.
//
//     kind        text            secondary       children
//     ---------------------------------------------------------------------
//     element     the name        character data  blocks
//     heading     the title       --              --
//     paragraph   the text        --              --
//     key_value   the key         the value       --
//     rule        --              --              --
//     space       --              --              --   (millipoints)
//     page_break  --              --              --
//     list        --              --              items
//     table       --              --              columns, THEN rows
//     repeat      sequence name   scope name      blocks
//     slot        slot name       --              --
//     document    --              --              blocks
//     item        the text        --              blocks (nested)
//     column      the header      --              --
//     column_grp  the label       --              columns and/or groups
//     row         --              --              cells
//     cell        the text        --              --
//
//   A table's children are columns first and rows second.  That ordering is
// the emission-order guarantee of document_render_algebra.h section IV, and
// d_doc_node_is_well_sorted checks it.


// ===========================================================================
// III.   construction
// ===========================================================================
//   Each returns a node by value; none allocates, none copies a string, and
// none validates its children beyond the count/pointer agreement.  Build
// bottom-up: children must outlive the parent.

// i.     leaves
struct d_doc_node d_doc_heading(uint32_t                _level,
                                const char*             _text,
                                struct d_doc_attributes _attrs);
struct d_doc_node d_doc_paragraph(const char*             _text,
                                  struct d_doc_attributes _attrs);
struct d_doc_node d_doc_key_value(const char*             _key,
                                  const char*             _value,
                                  struct d_doc_attributes _attrs);
struct d_doc_node d_doc_rule(struct d_doc_attributes _attrs);
struct d_doc_node d_doc_space(int32_t                 _millipoints,
                              struct d_doc_attributes _attrs);
struct d_doc_node d_doc_page_break(struct d_doc_attributes _attrs);
struct d_doc_node d_doc_column(const char*             _header,
                               struct d_doc_attributes _attrs);
struct d_doc_node d_doc_cell(const char*             _text,
                             struct d_doc_attributes _attrs);

// ii.    the hole
struct d_doc_node d_doc_slot(const char* _name);

// iii.   containers
struct d_doc_node d_doc_document(struct d_doc_attributes         _attrs,
                                 const struct d_doc_node* const* _blocks,
                                 uint32_t                        _count);
struct d_doc_node d_doc_element(const char*                     _name,
                                const char*                     _text,
                                struct d_doc_attributes         _attrs,
                                const struct d_doc_node* const* _blocks,
                                uint32_t                        _count);
struct d_doc_node d_doc_list(bool                            _ordered,
                             struct d_doc_attributes         _attrs,
                             const struct d_doc_node* const* _items,
                             uint32_t                        _count);
struct d_doc_node d_doc_item(const char*                     _text,
                             struct d_doc_attributes         _attrs,
                             const struct d_doc_node* const* _blocks,
                             uint32_t                        _count);
struct d_doc_node d_doc_column_group(const char*                     _label,
                                     struct d_doc_attributes         _attrs,
                                     const struct d_doc_node* const* _children,
                                     uint32_t                        _count);
struct d_doc_node d_doc_row(struct d_doc_attributes         _attrs,
                            const struct d_doc_node* const* _cells,
                            uint32_t                        _count);
struct d_doc_node d_doc_table(struct d_doc_attributes         _attrs,
                              const struct d_doc_node* const* _columns_rows,
                              uint32_t                        _column_count,
                              uint32_t                        _row_count);
struct d_doc_node d_doc_repeat(const char*                     _sequence,
                               const char*                     _scope,
                               struct d_doc_attributes         _attrs,
                               const struct d_doc_node* const* _blocks,
                               uint32_t                        _count);


// ===========================================================================
// III-b. the annotation (d_doc_node is a cofree, and this names it)
// ===========================================================================
//   `layout.hpp` states the document-structure term as
//
//       layout_doc<OpId, Atom> = cofree<expr_layer<OpId, Atom, _>, doc_attributes>
//
// -- an annotated term: every node carries a doc_attributes bag, and the
// carrier is the cofree comonad over the layer functor.
//
//   THIS NODE IS THE SAME CONSTRUCTION OVER A DIFFERENT FUNCTOR, and it was
// built that way without the word being used.  Cofree(F, A) = A x F(Cofree);
// d_doc_node is `attrs` (the A) crossed with a tagged layer of the document
// functor whose holes are more d_doc_nodes.  So:
//
//       d_doc_node  ==  cofree(F_doc, d_doc_attributes)     monomorphised
//
//   THE PROPERTY THAT MAKES IT COFREE RATHER THAN "A TREE WITH SOME
// ATTRIBUTES" is that the annotation is TOTAL: every node has one, every
// constructor sets one, and there is no node from which an annotation cannot
// be read.  That is the counit, and d_doc_extract is it.  If a constructor
// ever grows a path that leaves `attrs` uninitialised, the carrier stops
// being a cofree and the layout correspondence stops holding -- which is why
// there is a fixture asserting totality across every constructor rather than
// a comment asserting it here.

const struct d_doc_attributes*
         d_doc_extract(const struct d_doc_node* _node);


// ===========================================================================
// IV.    inspection
// ===========================================================================

// i.     structural predicates
bool     d_doc_node_is_closed(const struct d_doc_node* _node);
bool     d_doc_node_is_well_sorted(const struct d_doc_node* _node);
bool     d_doc_node_equal(const struct d_doc_node* _left,
                          const struct d_doc_node* _right);

// ii.    measures of the TREE (never of its rendering)
uint32_t d_doc_node_depth(const struct d_doc_node* _node);
uint32_t d_doc_node_count(const struct d_doc_node* _node);
uint32_t d_doc_node_slot_count(const struct d_doc_node* _node);
uint32_t d_doc_node_anchor_count(const struct d_doc_node* _node);

// iii.   the table split
uint32_t d_doc_column_span(const struct d_doc_node* _column_or_group);
uint32_t d_doc_table_column_count(const struct d_doc_node* _table);
uint32_t d_doc_table_row_count(const struct d_doc_node* _table);


// ===========================================================================
// V.     substitution (the free monad's bind)
// ===========================================================================
//   Filling a slot builds nodes, so it needs storage.  The caller supplies it:
// an arena of nodes and an arena of child links, both caller-owned, both
// sized by the caller.  Exhausting either is a MECHANICAL failure and is
// reported as one.  Nothing here calls malloc.

// d_doc_arena
//   struct: caller-supplied storage for a substitution result.  `nodes` holds
// the rebuilt nodes and `links` the child arrays they point into.  Reset by
// setting both counts to zero.
struct d_doc_arena
{
    struct d_doc_node*        nodes;
    const struct d_doc_node** links;

    // bytes / byte_capacity / byte_count
    //   scratch for INTERPOLATED strings.  Expansion is the one operation
    // that manufactures text rather than rearranging it, and it manufactures
    // it by accumulation -- so it needs caller-supplied bytes, not an owned
    // growable string.  A string with no `{` in it is not copied at all; the
    // expanded node borrows the original pointer, so a template with no
    // tokens consumes nothing here.
    char*                     bytes;

    uint32_t                  node_capacity;
    uint32_t                  node_count;
    uint32_t                  link_capacity;
    uint32_t                  link_count;
    uint32_t                  byte_capacity;
    uint32_t                  byte_count;
};

// fn_doc_valuation
//   function pointer: slot NAME -> the tree it stands for, or NULL to leave
// the hole unfilled.  Returning NULL is not a failure (ch-documents.tex,
// non-property 4); the slot survives and renders as nothing.
// Note: `_context` may be NULL.
typedef const struct d_doc_node* (*fn_doc_valuation)(const char* _name,
                                                     void*       _context);

void                     d_doc_arena_init(struct d_doc_arena*       _arena,
                                          struct d_doc_node*        _nodes,
                                          uint32_t                  _node_capacity,
                                          const struct d_doc_node** _links,
                                          uint32_t                  _link_capacity,
                                          char*                     _bytes,
                                          uint32_t                  _byte_capacity);
// d_doc_subst
//   NOT REBUILT ON d_free_bind, AND THAT IS A DECISION RATHER THAN AN
// OVERSIGHT.  `free.h`'s d_free_bind is the same contract as this -- a graft
// arrow, NULL leaving the hole standing -- so the overlap is real and was
// found deliberately.  It is not taken because d_free's children are held BY
// VALUE and contiguous inside their parent, so adopting it means copying every
// subtree into an arena at construction, and this subsystem's nodes borrow.
// Revisit when someone is in that file for another reason; it is not worth a
// stage on its own.  (The render fold is a different matter and cannot use
// d_free_fold at all -- see stage 7.)
enum d_doc_render_status d_doc_subst(const struct d_doc_node*  _tree,
                                     fn_doc_valuation          _valuation,
                                     void*                     _context,
                                     struct d_doc_arena*       _arena,
                                     const struct d_doc_node** _out);
void                     d_doc_arena_reset(struct d_doc_arena* _arena);


// ===========================================================================
// VI.    expansion (repeat + interpolation)
// ===========================================================================
//   `expand` of ch-documents.tex section 7: unroll every repeat against a
// bound sequence and resolve every `{token}` against the environment, yielding
// a tree with no binders and no tokens.
//
//   NOT GATED ON AN OWNED STRING, contrary to the stage 1 note this replaces.
// Interpolation is ACCUMULATION -- it appends literal runs and looked-up
// values into a buffer -- and accumulation runs into caller-supplied bytes as
// happily as into a growable string.  So expansion takes the same arena
// substitution does, with one extra region.  This is `parity_test.md`'s own
// argument about its stage 7, re-measured here and found to hold.
//
//   TOKEN SYNTAX, matching the C++ template exactly:
//     {name}         a scalar
//     {scope.field}  a field of the current item of an enclosing repeat
//     {{ and }}      literal braces
//   An unbound name interpolates as EMPTY, never as an error.  A brace that
// opens nothing is emitted verbatim, so a document about JSON is still a
// document.
//
//   SCOPES NEST.  repeat("modules", "mod") binds `mod`; a repeat inside it
// binds its own scope, and an inner scope shadows an outer one of the same
// name.  A token matching no scope falls through to the scalar lookup.
//
//   SECOND IMPLEMENTATION -- RESOLVED 2026-08-03 AS A PERMANENT EXCEPTION.
//   `c/text/text_template.h` is a 414-line C token engine that predates this
// one, and this interpolator was written without reference to it.  That was a
// real fault.  The reconciliation was measured rather than argued, and the
// answer is that the two do not merge.  The reasons, both load-bearing:
//
//   1. INCOMPATIBLE MEMORY MODELS.  Every render path in text_template.c
// allocates -- `d_text_template_render` takes a caller-owned buffer and still
// calls d_str_interp_context_new() internally, as does render_length.  23
// allocation sites in the file.  This subsystem allocates nowhere: the kernel
// is caller-supplied arenas throughout, which is what lets a document be
// rendered with no allocator present at all.  Adapting to an allocating engine
// would spend that property, and it is not a small one.
//
//   2. DIFFERENT NESTING SEMANTICS, not merely different syntax.  That engine
// does express per-item scope -- d_text_template_list_fn hands each item a
// fresh sub-template with auto-keys injected -- but it CLEARS prior bindings
// per item, so an inner list cannot see an outer scope.  This one keeps outer
// scopes visible and lets an inner scope shadow, which is what
// `{m.name}` inside a repeat inside a repeat requires.  Those are different
// languages, and picking one is a semantic decision rather than a merge.
//
//   SO: TWO ENGINES, ON PURPOSE, WITH THE REASON WRITTEN DOWN.  That is a
// worse outcome than one engine and a better one than two engines nobody
// decided on.  If the document kernel ever acquires an allocator, reason 1
// lapses and this should be re-measured; reason 2 would still need a ruling.
//
//   HINTS ARE NOT TOKEN-BEARING.  Only `text` and `secondary` are
// interpolated.  Hints are structure, not content -- so an attribute value
// containing braces survives untouched, in both languages.

// fn_doc_scalar
//   function pointer: a plain token name -> its value.  An unbound name
// yields NULL, which interpolates as EMPTY and never as an error -- the same
// rule the C++ template states and the same rule non-property 4 gives for a
// hole.
// Note: `_context` may be NULL.
typedef const char* (*fn_doc_scalar)(const char* _name,
                                     void*       _context);

// fn_doc_count
//   function pointer: a sequence name -> how many items it has.  A sequence
// that does not resolve yields zero, which unrolls to nothing.  "No items" is
// the correct rendering of an absent sequence, not a failure.
typedef uint32_t (*fn_doc_count)(const char* _sequence,
                                 void*       _context);

// fn_doc_field
//   function pointer: one field of one item of a sequence.  Called when a
// token resolves inside a repeat's scope: `{mod.name}` under
// repeat("modules", "mod") at index i asks for ("modules", i, "name").
// Unbound yields NULL, which interpolates as empty.
typedef const char* (*fn_doc_field)(const char* _sequence,
                                    uint32_t    _index,
                                    const char* _field,
                                    void*       _context);

// d_doc_environment
//   struct: everything expansion needs beyond the tree.  Three lookups and a
// context; no ownership, no allocation, and no iteration protocol -- the
// caller's data stays wherever it already is.
struct d_doc_environment
{
    fn_doc_scalar scalar;
    fn_doc_count  count;
    fn_doc_field  field;
    void*         context;
};

enum d_doc_render_status d_doc_expand(const struct d_doc_node*        _tree,
                                      const struct d_doc_environment* _env,
                                      struct d_doc_arena*             _arena,
                                      const struct d_doc_node**       _out);
const char*              d_doc_interpolate(const char*                     _text,
                                           const struct d_doc_environment* _env,
                                           struct d_doc_arena*             _arena);


// ===========================================================================
// VII.   the two-pass protocol
// ===========================================================================
//   A table of contents carrying page numbers cannot be instantiated before
// layout, because page assignment depends on the document's extent -- which
// includes the table of contents.  Rendering is A fold, not ONE fold, so two
// are permitted; what was missing is that nothing SPECIFIED them, and a naive
// single pass silently produces wrong or absent numbers.
//
//   THE CIRCULARITY IS NARROWER THAN IT LOOKS.  The extent depends on the
// numbers' WIDTHS, not their VALUES.  Reserve the width and pass two's extent
// is unchanged by construction, so pass one's assignments still hold and the
// numbers describe the document that is actually emitted.
//
//     1. reserve   every page number renders as `page_digits` placeholder
//                  bytes, and the entry count is known from the tree
//     2. discover  fold with a RECORDING algebra -- the emission side's --
//                  which emits no bytes and fills a page map
//     3. verify    fold again with the real numbers and a second recorder;
//                  the two maps must be bit-identical
//     4. emit      fold the verified tree for real
//
//   STEP 3 IS THE POINT.  Without it the obligation "the two passes must agree
// on layout" is a hope.  With it, it is a comparison that fails loudly.  It
// costs one extra fold, which is why it is a knob and not a constant.
//
//   THE ONE FAILURE MODE REPORTS.  If the discovered page count needs more
// digits than were reserved, every number after the boundary would shift.
// That returns D_DOC_RENDER_RESERVATION rather than emitting a document whose
// contents page is quietly wrong -- the same discipline as every other bound
// in this subsystem.

// d_doc_page_entry
//   struct: one anchor and the page it landed on.
struct d_doc_page_entry
{
    const char* anchor;
    uint32_t    page;
    uint32_t    reserved;   // pad; must be 0
};

// d_doc_page_map
//   struct: what a recording pass produces and an emitting pass consumes.
// Caller-owned storage, like every other accumulator here.
struct d_doc_page_map
{
    struct d_doc_page_entry* entries;
    uint32_t                 capacity;
    uint32_t                 count;
    uint32_t                 page_digits;   // the RESERVED width
    uint32_t                 max_page;
};

void     d_doc_page_map_init(struct d_doc_page_map*   _map,
                             struct d_doc_page_entry* _entries,
                             uint32_t                 _capacity,
                             uint32_t                 _page_digits);
bool     d_doc_page_map_record(struct d_doc_page_map* _map,
                               const char*            _anchor,
                               uint32_t               _page);
uint32_t d_doc_page_map_find(const struct d_doc_page_map* _map,
                             const char*                  _anchor);
// d_doc_page_map_agree
//   TWO AGREEMENT FUNCTIONS EXIST AND THEY ARE NOT DUPLICATES.  The emission
// side's d_pdf_recorder_agree compares what was MEASURED; this compares the
// TRANSCRIPT the record pass copied into a page map.  If the adapter between
// them is faithful the two verdicts coincide, and if it is not, this one is
// the wrong answer.  So: theirs is authoritative, this one is what the driver
// can check without knowing their types, and the adapter is the trust
// boundary between the two.  The anchor-count guard in d_doc_two_pass exists
// because the commonest unfaithful adapter is one that copies nothing.
bool     d_doc_page_map_agree(const struct d_doc_page_map* _left,
                              const struct d_doc_page_map* _right);
bool     d_doc_page_map_fits(const struct d_doc_page_map* _map);
uint32_t d_doc_page_map_digits(uint32_t _value);
const char*
         d_doc_page_placeholder(const struct d_doc_page_map* _map);

// fn_doc_record_pass
//   function pointer: fold _tree with a RECORDING dialect and fill _map.
// Supplied by the emission side, because page assignment is measurement and
// measurement is theirs.  It must place nothing and must measure exactly as
// the emitting pass does -- a discovery pass that measured differently would
// measure the wrong document.
typedef enum d_doc_render_status (*fn_doc_record_pass)(
    const struct d_doc_node* _tree,
    struct d_doc_page_map*   _map,
    void*                    _context);

// d_doc_page_scope
//   struct: the environment wrapper the driver instantiates page numbers
// through.  Names beginning `page.` resolve from the map -- as the fixed-width
// PLACEHOLDER during discovery and as the real number during emission -- and
// every other name falls through to the caller's environment untouched.
struct d_doc_page_scope
{
    const struct d_doc_environment* base;
    const struct d_doc_page_map*    map;
    int32_t                         resolved;   // 0 = placeholder, 1 = real
    int32_t                         reserved;   // pad; must be 0
};

struct d_doc_environment
         d_doc_page_environment(struct d_doc_page_scope* _scope);

enum d_doc_render_status d_doc_two_pass(
                             const struct d_doc_node*           _template,
                             const struct d_doc_environment*    _env,
                             struct d_doc_arena*                _arena,
                             fn_doc_record_pass                 _record,
                             void*                              _record_context,
                             struct d_doc_page_map*             _discover,
                             struct d_doc_page_map*             _verify,
                             const struct d_doc_render_algebra* _emit);


// ===========================================================================
// VIII.  the fold
// ===========================================================================
//   The catamorphism.  One traversal, every dialect.  Visits a constructor's
// arguments in declaration order, which is what makes the emission order of
// document_render_algebra.h section IV a corollary rather than a convention.
//
//   Stops at the first non-OK status and returns it unchanged, so a dialect
// aborts a render by returning one.  A residual slot or an unbound repeat
// emits nothing and does NOT stop the fold.

enum d_doc_render_status d_doc_render(
                             const struct d_doc_node*           _root,
                             const struct d_doc_render_algebra* _algebra);
enum d_doc_render_status d_doc_render_block(
                             const struct d_doc_node*           _block,
                             const struct d_doc_render_algebra* _algebra);


// ===========================================================================
// IX.    layout assertions
// ===========================================================================
//   The Layout law: one declaration, size and offset asserted in both
// dialects.  Written against the pointer and hint-bag widths so that they hold
// on every tier rather than only on LP64.

//   COVERAGE, NOT JUST PRESENCE.  Until 2026-08-02 the Layout law was
// asserted for d_doc_attr, d_doc_attributes, d_doc_node and
// d_doc_render_algebra -- four of the eight structs that cross the language
// boundary -- and stated as though it held for all of them.  d_doc_arena,
// d_doc_environment, d_doc_text_state and d_doc_table_buffer had no assertion
// in EITHER dialect, so both could have agreed on a drifted layout and nothing
// would have failed.  A struct with no assertion generates no failing test,
// which is the same shape as a missing wrapper generating none.  The four
// below close it; the other two are asserted in document_text.h.

D_STATIC_ASSERT(offsetof(struct d_doc_node, children) ==
                    sizeof(struct d_doc_attributes),
                "d_doc_node layout drift");
D_STATIC_ASSERT(offsetof(struct d_doc_node, secondary) ==
                    (offsetof(struct d_doc_node, children) +
                     (2u * sizeof(const char*))),
                "d_doc_node layout drift");
D_STATIC_ASSERT(sizeof(struct d_doc_node) ==
                    (offsetof(struct d_doc_node, millipoints) +
                     (6u * sizeof(uint32_t))),
                "d_doc_node layout drift -- a field was added or reordered");
D_STATIC_ASSERT(offsetof(struct d_doc_node, reserved) ==
                    (sizeof(struct d_doc_node) - sizeof(uint32_t)),
                "d_doc_node tail padding must be the declared reserved field");

D_STATIC_ASSERT(offsetof(struct d_doc_arena, links) ==
                    sizeof(struct d_doc_node*),
                "d_doc_arena layout drift");
D_STATIC_ASSERT(sizeof(struct d_doc_arena) ==
                    ((3u * sizeof(void*)) + (6u * sizeof(uint32_t))),
                "d_doc_arena layout drift -- a region was added or reordered");
D_STATIC_ASSERT(sizeof(struct d_doc_environment) ==
                    (3u * sizeof(fn_doc_scalar)) + sizeof(void*),
                "d_doc_environment layout drift");


D_EXTERN_C_END


#endif  // DJINTERP_C_UTIL_DOCUMENT_NODE_
