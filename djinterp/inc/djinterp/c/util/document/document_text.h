/******************************************************************************
* djinterp [utility]                                           document_text.h
*
*   THREE DIALECTS, ZERO NEW TRAVERSALS.  Plain text, Markdown, and markup are
* three `struct d_doc_render_algebra` values over the single fold in
* document_node.c.  There is no plain renderer, no markdown renderer, and no
* markup renderer -- there is one traversal and three algebras, exactly as
* `parity_documents.md` predicted and `ch-documents.tex` requires.  A fourth
* dialect adds an algebra and no code here.
*
*   THEY SHARE ONE STATE TYPE.  All three need the same things: a sink, list
* nesting, and where they are inside a table.  So `struct d_doc_text_state` is
* one struct and the three constructors differ only in which verbs they bind.
* That is not a saving; it is the evidence that the dialects really are three
* points in one space rather than three programs.
*
*   WHAT THESE DIALECTS DELIBERATELY DO NOT DO
*
*   No computed column widths.  The C++ `plain_document_renderer` lays tables
* out as aligned monospace columns, which requires buffering every row and
* taking a max over the data.  That is a WIDTH COMPUTATION, and the split
* contract reserves measurement to the other side.  These dialects stream: a
* cell is padded only to a `width` HINT the producer declared, never to an
* extent derived from the content.  Whether byte counting over a monospace
* grid falls inside or outside that contract is a question for the
* coordinator, not for this file to answer by growing an engine.  See the
* stage 1 handoff.
*
*   No points-to-lines conversion.  A `space` request is in points.  Turning
* points into blank lines needs a metric, and a metric is measurement, so a
* text dialect emits nothing for it (Markdown emits one newline, which is
* independent of the amount).
*
*   No floating-point in the bytes.  The markup dialect does not print the
* `space` amount, because formatting a double identically in two languages is
* the one place parity fails for environmental reasons and that policy is
* still open (`AGENT_README` section 9, question 4).  `<space/>` carries the
* constructor without betting on the open question.
*
*   CONFORMS TO: body-document.tex (streaming adequacy, and its limit)
*             ch-rendering.tex where it constrains a dialect
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
*   C99 apart from <stdint.h>.  No allocation.  No <stdio.h>, so no locale
* dependence anywhere in the byte path.
*
*
* path:      /inc/djinterp/c/util/document/document_text.h
* link(s):   TBA
* author(s): Agent B (structure)                           created: 2026.07.31
******************************************************************************/

#ifndef DJINTERP_C_UTIL_DOCUMENT_TEXT_
#define DJINTERP_C_UTIL_DOCUMENT_TEXT_ 1

#include <stddef.h>
#include <stdint.h>
#include "../../djinterp.h"
#include "djinterp/c/util/document/document_node.h"
#include "../../../core/util/sink_common.h"   // d_pack_sink

D_EXTERN_C_BEGIN



// D_INTERNAL_DOC_MAX_LIST_DEPTH
//   constant: how deep ordered-list numbering is tracked.  Beyond it a list
// still renders; it simply stops numbering and falls back to a bullet.
// Degrade, never error.  Overridable through the dconfig cascade.

// D_INTERNAL_DOC_MAX_TABLE_COLUMNS
//   constant: how many columns' alignment hints are remembered so that
// Markdown's delimiter row can carry them.  A wider table still renders; the
// columns past this simply take the default alignment.  Degrade, never error.

// D_INTERNAL_DOC_RULE_WIDTH
//   constant: the width of a rule carrying no `width` hint, in bytes.


// d_doc_table_buffer
//   struct: caller-owned storage for the ALIGNED plain dialect.  Holds one
// borrowed pointer per cell and one running width per column.
//
//   WHY THIS EXISTS AT ALL, AND WHY IT IS SEPARATE.  Aligning a monospace
// table means padding every cell to the widest in its column, which cannot be
// known until the last row has arrived.  So the aligned dialect is not the
// streaming one with a flag: it BUFFERS, and it is therefore a FOURTH
// ALGEBRA, not a mode of the third.  ch-documents.tex section 6 says as much
// -- streaming adequacy covers only dialects whose output is a concatenation
// in argument order, and this one is not.
//
//   The consequence is worth stating plainly: the aligned dialect can FAIL
// where the streaming one cannot, because it can run out of buffer.  That is
// the price of alignment and it is why both dialects exist rather than one.
//
//   MEASUREMENT, OR NOT.  Widths here are BYTE COUNTS of strings the dialect
// was handed -- no font, no page geometry, no flow state.  The split contract
// reserves metric measurement to the other side; counting the bytes of one's
// own input is a different thing, and is what a monospace grid is made of.
struct d_doc_table_buffer
{
    const char** cells;            // borrowed, one per cell, row-major
    uint32_t*    widths;           // one per column
    uint32_t     cell_capacity;
    uint32_t     cell_count;
    uint32_t     column_capacity;
    uint32_t     columns;
    uint32_t     rows;
    int32_t      overflowed;
};

void d_doc_table_buffer_init(struct d_doc_table_buffer* _buffer,
                             const char**               _cells,
                             uint32_t                   _cell_capacity,
                             uint32_t*                  _widths,
                             uint32_t                   _column_capacity);


// d_doc_text_state
//   struct: the state all three dialects carry in their context.  Zeroed by
// d_doc_text_state_init and never allocated.
struct d_doc_text_state
{
    // sink
    //   where the bytes go.  This IS d_pack_sink -- the framework's own
    // context-carrying byte consumer, adopted rather than paralleled.  The
    // stand-in this replaced was isolated behind exactly this one member, so
    // reconciling it touched no algebra.
    struct d_pack_sink sink;

    // list nesting
    uint32_t          list_depth;
    uint32_t          counters[D_INTERNAL_DOC_MAX_LIST_DEPTH];
    uint32_t          ordered_bits;

    // element nesting, for the markup dialect's close tags
    uint32_t          element_depth;

    // table position: how many columns were declared, how many cells the
    // current row has emitted, and how many bytes the header line ran to
    // (which is a count of THIS DIALECT'S OWN OUTPUT, not a measurement of
    // anyone's content).
    uint32_t          table_columns;
    uint32_t          table_cell;
    uint32_t          header_bytes;
    int32_t           in_table;
    int32_t           saw_row;

    // column_align
    //   one d_doc_align per declared column, remembered ONLY because
    // Markdown's delimiter row must repeat the header's alignment and the
    // fold has moved past the columns by then.  This is the single piece of
    // per-column state in the subframework, it is bounded, and it holds a
    // hint the producer declared -- not anything measured.
    uint8_t           column_align[D_INTERNAL_DOC_MAX_TABLE_COLUMNS];

    // table_buffer
    //   where the ALIGNED dialect accumulates.  NULL for the three streaming
    // dialects, which never touch it.  Borrowed; the caller owns the storage.
    struct d_doc_table_buffer* table_buffer;
};

// i.     state
void d_doc_text_state_init(struct d_doc_text_state* _state,
                           struct d_pack_sink       _sink);

// ii.    the three dialects
struct d_doc_render_algebra d_doc_plain_algebra(
                                struct d_doc_text_state* _state);
struct d_doc_render_algebra d_doc_plain_aligned_algebra(
                                struct d_doc_text_state* _state);
struct d_doc_render_algebra d_doc_markdown_algebra(
                                struct d_doc_text_state* _state);
struct d_doc_render_algebra d_doc_markup_algebra(
                                struct d_doc_text_state* _state);

// iii.   one-shot convenience
enum d_doc_render_status d_doc_render_block_to_buffer(
                             const struct d_doc_node* _block,
                             struct d_doc_render_algebra (*_dialect)(
                                 struct d_doc_text_state*),
                             char*                    _buffer,
                             size_t                   _capacity,
                             size_t*                  _out_used);
enum d_doc_render_status d_doc_render_to_buffer(
                             const struct d_doc_node* _root,
                             struct d_doc_render_algebra (*_dialect)(
                                 struct d_doc_text_state*),
                             char*                    _buffer,
                             size_t                   _capacity,
                             size_t*                  _out_used);


D_EXTERN_C_END


// ===========================================================================
//   layout assertions
// ===========================================================================
//   Both of these cross the boundary by pointer -- the C++ face binds a table
// buffer and the parity oracle shares a dialect state -- so both owe the
// Layout law, and neither had it until the coverage audit.

D_STATIC_ASSERT(offsetof(struct d_doc_table_buffer, widths) ==
                    sizeof(const char**),
                "d_doc_table_buffer layout drift");
D_STATIC_ASSERT(sizeof(struct d_doc_table_buffer) ==
                    ((2u * sizeof(void*)) + (5u * sizeof(uint32_t)) +
                     sizeof(int32_t)),
                "d_doc_table_buffer layout drift");
D_STATIC_ASSERT(offsetof(struct d_doc_text_state, list_depth) ==
                    sizeof(struct d_pack_sink),
                "d_doc_text_state layout drift");
D_STATIC_ASSERT(offsetof(struct d_doc_text_state, table_buffer) ==
                    (sizeof(struct d_doc_text_state) - sizeof(void*)),
                "d_doc_text_state layout drift");


#endif  // DJINTERP_C_UTIL_DOCUMENT_TEXT_
