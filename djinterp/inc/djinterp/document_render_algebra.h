/******************************************************************************
* djinterp [utility]                                  document_render_algebra.h
*
*   THE RENDER PROTOCOL.  One dialect is one `struct d_doc_render_algebra`: a
* context pointer and a per-constructor function pointer, folded over a
* document tree by d_doc_render (document_node.h).  This is the successor to
* the C++ document_renderer's virtual base, and the shape is not a design
* choice -- it mirrors the constructors of ch-documents.tex section 3, because
* a dialect is an F-algebra and an F-algebra is exactly one function per
* constructor.  Adding a verb here means adding a constructor there, and vice
* versa; they are the same edit.
*
*   ONE FOLD, MANY DIALECTS.  There is no markdown renderer and no markup
* renderer.  There is ONE traversal, and markdown and markup are two algebras
* over it.  A third dialect is a third algebra and no new traversal.  If you
* find yourself writing a second walk, something has gone wrong upstream.
*
*   ONE PRIMITIVE, MANY DEFAULTS.  Every member may be NULL.  A NULL member
* means "use the documented default", which is either a funnel into write_line
* or a no-op -- so a minimal dialect sets `write_line` and nothing else, and
* gets headings, paragraphs, list items and key-values as plain lines while
* silently dropping rules, spacing and tables.  `write_line` is the only member
* that must be non-NULL.  This reproduces the C++ base's default set exactly,
* including its byte output, without a vtable and without an init call: `= {0}`
* plus one assignment is a valid algebra.
*
*   TWO KINDS OF FAILURE, KEPT APART.  `enum d_doc_render_status` separates a
* FORMAL failure (the tree violates ch-documents.tex) from a MECHANICAL one
* (the sink is full).  Reporting them identically is a conformance bug that
* reads as correct behaviour, so they are different enumerators and
* d_doc_render_status_is_formal tells them apart.  Note what is NOT a failure:
* an unfilled slot and an unbound sequence emit nothing and return OK
* (ch-documents.tex, non-property 4).
*
*   MEASUREMENT IS NOT HERE, AND WILL NOT BE.  No verb takes or returns a
* width, a line count, a page number, or a font metric.  A dialect that
* paginates buffers what it needs and measures on its own side; the fold
* streams.  That separation is what lets the text dialects be byte-checkable
* with no typesetting back end present in the build.
*
*   CONFORMS TO: body-document.tex (Phi, the admissible dialects)
*             ch-documents.tex section on emission order
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
*   C89-clean apart from <stdint.h>.  No allocation.  Depends only on
* document_common.h.
*
*
* ---------------------------------------------------------------------------
*   FROZEN 2026-07-31.  This header is a published interface: another party
* implements against it.  To change it, change ch-documents.tex first -- the
* verb set is derived from the constructor set and has no independent
* existence -- then re-freeze.  Adding a verb is a change; changing a
* parameter is a change; changing a documented default is a change, because
* the defaults are observable in the bytes.
* ---------------------------------------------------------------------------
*
*
* TABLE OF CONTENTS
* =================
* I.     status
* II.    verb signatures
* III.   the algebra
* IV.    emission order
* V.     defaults
* VI.    helpers
* VII.   layout assertions
*
*
* path:      /inc/djinterp/c/util/document/document_render_algebra.h
* link(s):   TBA
* author(s): Agent B (structure)                           created: 2026.07.31
******************************************************************************/

#ifndef DJINTERP_C_UTIL_DOCUMENT_RENDER_ALGEBRA_
#define DJINTERP_C_UTIL_DOCUMENT_RENDER_ALGEBRA_ 1

#include <stddef.h>
#include <stdint.h>
#include "../../djinterp.h"
#include "./document_common.h"

// D_DOC_INLINE
//   macro: the inline keyword, where the dialect has one.  C99 and C++ both
// do; a pre-C99 C compiler gets a plain static and one copy per translation
// unit, which for four switches is not worth a feature test.
#if defined(__cplusplus) || (defined(__STDC_VERSION__) && \
                             (__STDC_VERSION__ >= 199901L))
    #define D_DOC_INLINE  inline
#else
    #define D_DOC_INLINE
#endif


D_EXTERN_C_BEGIN



// ===========================================================================
// I.     status
// ===========================================================================

// d_doc_render_status
//   enum: the outcome of a verb, and of the fold that drives it.  The fold
// stops at the first non-OK status and returns it unchanged, so a dialect can
// abort a render by returning one.
enum d_doc_render_status
{
    // success.
    D_DOC_RENDER_OK              = 0,

    // MECHANICAL failures: the structure was fine, the machinery was not.
    D_DOC_RENDER_SINK_FULL       = 1,  // bounded output exhausted
    D_DOC_RENDER_SINK_ERROR      = 2,  // the sink itself failed
    D_DOC_RENDER_DEPTH_EXCEEDED  = 3,  // deeper than D_INTERNAL_DOC_MAX_DEPTH

    // FORMAL failures: the structure violates ch-documents.tex.
    D_DOC_RENDER_MALFORMED       = 4,  // a child of the wrong sort, or a
                                       // non-zero count over a NULL array
    D_DOC_RENDER_NO_PRIMITIVE    = 5,  // write_line was NULL

    // the two-pass protocol's own formal failure: the discovered page count
    // needs more digits than the caller reserved, so pass two's extent would
    // differ from pass one's and the numbers would describe a document that
    // was never emitted.  Appended, never renumbered
    D_DOC_RENDER_RESERVATION     = 6,

    // the recording pass reported no anchors for a tree that contains them.
    // A DISTINCT failure from RESERVATION on purpose: the remedy for a blown
    // reservation is a wider one, and the remedy for this is to wire the
    // recorder up.  Reporting them alike would make an unwired seam look like
    // a tuning problem, which is how one shipped
    D_DOC_RENDER_RECORDER        = 7
};


// ===========================================================================
// II.    verb signatures
// ===========================================================================
//   One typedef per verb.  `_context` is the algebra's own context member,
// passed back unchanged, and is the sink: a dialect that accumulates bytes
// keeps its accumulator there.  Every `_attrs` is non-NULL and canonically
// ordered (document_common.h, ruling R2); every `const char*` is non-NULL and
// NUL-terminated, empty rather than absent.
//
//   NO `bool` AND NO `enum` IN A CROSSED SIGNATURE (amendment 2026-07-31,
// scope widened by ruling D3 on 2026-08-05).  This paragraph used to say these
// function pointers were "the only" declarations implemented in one language
// and invoked from the other.  That was true while the C++ face duplicated the
// kernel rather than deriving from it; ruling D2 ends that, and D3 therefore
// applied the same rule to EVERY public declaration in document_common.h,
// document_node.h, and document_text.h.  They are no longer the only ones --
// they were merely the first.  `bool` is the sharp case:
// on a tier where C's is emulated rather than `_Bool`, it is four bytes in C
// and one in C++ for the same declaration, and the call silently mis-marshals.
// An unfixed enum is the same argument one step weaker.  So every verb returns
// `int32_t` carrying a `d_doc_render_status` value, and every flag is
// `int32_t`.  This is `parity_test.md`'s "no option member is an enum or a
// bool" applied to a signature rather than to a struct field, and it is the
// same reason: goals section 4 determinacy, at a place where the failure is
// silent.  Nothing above the pointer boundary is affected -- the fold, the
// helpers, and every dialect body still speak the enum.

// i.     frame
typedef int32_t (*fn_doc_begin_document)(
    const struct d_doc_attributes* _attrs,
    void*                          _context);
typedef int32_t (*fn_doc_end_document)(
    void* _context);

// ii.    the generic named container
typedef int32_t (*fn_doc_begin_element)(
    const char*                    _name,
    const char*                    _text,
    const struct d_doc_attributes* _attrs,
    void*                          _context);
typedef int32_t (*fn_doc_end_element)(
    const char* _name,
    void*       _context);

// iii.   leaf blocks
typedef int32_t (*fn_doc_heading)(
    uint32_t                       _level,
    const char*                    _text,
    const struct d_doc_attributes* _attrs,
    void*                          _context);
typedef int32_t (*fn_doc_paragraph)(
    const char*                    _text,
    const struct d_doc_attributes* _attrs,
    void*                          _context);
typedef int32_t (*fn_doc_key_value)(
    const char*                    _key,
    const char*                    _value,
    const struct d_doc_attributes* _attrs,
    void*                          _context);
typedef int32_t (*fn_doc_rule)(
    const struct d_doc_attributes* _attrs,
    void*                          _context);
typedef int32_t (*fn_doc_space)(
    int32_t                        _millipoints,
    const struct d_doc_attributes* _attrs,
    void*                          _context);
typedef int32_t (*fn_doc_page_break)(
    void* _context);

// iv.    lists
typedef int32_t (*fn_doc_begin_list)(
    int32_t                        _ordered,
    const struct d_doc_attributes* _attrs,
    void*                          _context);
typedef int32_t (*fn_doc_begin_item)(
    const char*                    _text,
    const struct d_doc_attributes* _attrs,
    void*                          _context);
typedef int32_t (*fn_doc_end_item)(
    void* _context);
typedef int32_t (*fn_doc_end_list)(
    void* _context);

// v.     tables
//   begin_column_group / end_column_group bracket a header level ABOVE the
// columns.  `_span` is the number of LEAF columns the group covers, computed
// by the fold, so a dialect never walks the tree to find it.  Both may be
// NULL, and the default is to emit nothing -- which means a dialect that
// cannot express grouping still sees the flat column run beneath and produces
// exactly the bytes it produced before groups existed.
typedef int32_t (*fn_doc_begin_column_group)(
    const char*                    _label,
    uint32_t                       _span,
    const struct d_doc_attributes* _attrs,
    void*                          _context);
typedef int32_t (*fn_doc_end_column_group)(
    void* _context);
typedef int32_t (*fn_doc_begin_table)(
    const struct d_doc_attributes* _attrs,
    void*                          _context);
typedef int32_t (*fn_doc_column)(
    const char*                    _header,
    const struct d_doc_attributes* _attrs,
    void*                          _context);
typedef int32_t (*fn_doc_begin_row)(
    const struct d_doc_attributes* _attrs,
    void*                          _context);
typedef int32_t (*fn_doc_cell)(
    const char*                    _text,
    const struct d_doc_attributes* _attrs,
    void*                          _context);
typedef int32_t (*fn_doc_end_row)(
    void* _context);
typedef int32_t (*fn_doc_end_table)(
    void* _context);

// vi.    the one primitive
typedef int32_t (*fn_doc_write_line)(
    const char*                    _text,
    const struct d_doc_attributes* _attrs,
    void*                          _context);


// ===========================================================================
// III.   the algebra
// ===========================================================================

// d_doc_render_algebra
//   struct: one dialect.  Zero-initialise it, set `context` and `write_line`,
// and override whichever verbs the dialect improves.  Every function-pointer
// member may be NULL; see section V for what NULL means for each.
struct d_doc_render_algebra
{
    // context
    //   the dialect's own state, passed unchanged to every verb.  The fold
    // never reads it and never owns it.
    void*                  context;

    // reads / read_count
    //   the hint keys this dialect consults -- the `reads(phi)` component of
    // ch-documents.tex section 6.  Declaring it is what makes the tolerance
    // law testable: adding a key not in this list must not change one byte of
    // output.  A NULL `reads` means "unknown", which disables the check
    // rather than asserting the empty set.
    const char* const*     reads;
    uint32_t               read_count;
    uint32_t               reserved;      // pad; must be 0

    // -- frame --
    fn_doc_begin_document  begin_document;
    fn_doc_end_document    end_document;

    // -- the generic named container --
    fn_doc_begin_element   begin_element;
    fn_doc_end_element     end_element;

    // -- leaf blocks --
    fn_doc_heading         heading;
    fn_doc_paragraph       paragraph;
    fn_doc_key_value       key_value;
    fn_doc_rule            rule;
    fn_doc_space           space;
    fn_doc_page_break      page_break;

    // -- lists --
    fn_doc_begin_list      begin_list;
    fn_doc_begin_item      begin_item;
    fn_doc_end_item        end_item;
    fn_doc_end_list        end_list;

    // -- tables --
    fn_doc_begin_table     begin_table;
    fn_doc_begin_column_group begin_column_group;
    fn_doc_end_column_group   end_column_group;
    fn_doc_column          column;
    fn_doc_begin_row       begin_row;
    fn_doc_cell            cell;
    fn_doc_end_row         end_row;
    fn_doc_end_table       end_table;

    // -- the one primitive; the only member that must be set --
    fn_doc_write_line      write_line;
};


// ===========================================================================
// IV.    emission order
// ===========================================================================
//   Guaranteed by the fold for every algebra, on every input.  A consumer that
// must buffer -- one that resolves column widths before placing a row, say --
// may rely on all of it.
//
//     document   := begin_document block* end_document
//     element    := begin_element block* end_element
//     list       := begin_list item* end_list
//     item       := begin_item block* end_item
//     table      := begin_table header* row* end_table
//     header     := begin_column_group header* end_column_group | column
//     row        := begin_row cell* end_row
//
//   The table line is the one that matters and it is a corollary, not a
// promise: the Table constructor's argument is a product of columns THEN rows,
// and a fold visits arguments in declaration order, so no cell can precede the
// last column and no column can follow the first row.  There is no
// interleaving anywhere in this grammar.
//
//   Slot and Repeat appear nowhere above.  A residual hole emits nothing.


// ===========================================================================
// V.     defaults
// ===========================================================================
//   What the fold does for a NULL member.  These are observable in the bytes,
// so they are interface, not implementation.
//
//     begin_document, end_document        nothing
//     begin_element                       write_line(_text) when _text is
//                                         non-empty, else nothing
//     end_element                         nothing
//     heading                             write_line(_text)
//     paragraph                           write_line(_text)
//     key_value                           write_line("<key>: <value>")
//     rule                                nothing
//     space                               nothing
//     page_break                          nothing
//     begin_list, end_list                nothing
//     begin_item                          write_line(_text)
//     end_item                            nothing
//     begin_column_group, end_column_group
//                                         nothing -- the columns beneath are
//                                         still visited, so an ungrouping
//                                         dialect is byte-unchanged
//     the whole table group               nothing (the table is dropped
//                                         cleanly, including its cells)
//     write_line                          D_DOC_RENDER_NO_PRIMITIVE
//
//   The key_value default needs a scratch buffer for the joined line.  It is
// D_INTERNAL_DOC_KV_BUFFER bytes on the stack; a key/value pair that does
// not fit is truncated rather than allocated for, because the default exists
// to make a one-function dialect possible, not to be a good renderer.  A
// dialect that cares sets `key_value`.

// D_INTERNAL_DOC_KV_BUFFER
//   constant: the stack scratch the default key_value funnel joins into.
// Overridable through the dconfig cascade.


// ===========================================================================
// VI.    helpers
// ===========================================================================

// i.     status classification (the anti-conflation rule, made checkable)
//
//   DEFINED HERE, NOT IN A TRANSLATION UNIT, AND ON PURPOSE.  These four were
// declared here and defined in document_render_algebra.c, which meant a
// consumer handed this header alone -- exactly what the bridge contract ships
// -- got four undefined references at link time.  The report that found it
// called it the third instance of a declared-here-defined-elsewhere pattern.
//
//   The general rule this instance yields: A HELPER WHOSE ENTIRE BODY IS A
// SWITCH OVER A SHARED ENUM BELONGS IN THE SHARED HEADER.  It has no state, no
// dependencies, and nothing a translation unit adds; putting it in one buys
// nothing and costs a link dependency that a header-only consumer cannot
// satisfy.  Being unable to classify a status you were just handed, without
// linking a library, is a defect in the interface rather than in the build.

static D_DOC_INLINE bool
d_doc_render_status_is_ok(enum d_doc_render_status _status)
{
    return (_status == D_DOC_RENDER_OK);
}

// d_doc_render_status_is_formal
//   Whether the STRUCTURE violated the document definition, as opposed to the
// machinery failing.  A caller that retries on one must not retry on the
// other.
static D_DOC_INLINE bool
d_doc_render_status_is_formal(enum d_doc_render_status _status)
{
    return ( (_status == D_DOC_RENDER_MALFORMED)    ||
             (_status == D_DOC_RENDER_NO_PRIMITIVE)  ||
             (_status == D_DOC_RENDER_RESERVATION)   ||
             (_status == D_DOC_RENDER_RECORDER) );
}

static D_DOC_INLINE bool
d_doc_render_status_is_mechanical(enum d_doc_render_status _status)
{
    return ( (_status == D_DOC_RENDER_SINK_FULL)  ||
             (_status == D_DOC_RENDER_SINK_ERROR) ||
             (_status == D_DOC_RENDER_DEPTH_EXCEEDED) );
}

static D_DOC_INLINE const char*
d_doc_render_status_name(enum d_doc_render_status _status)
{
    switch (_status)
    {
        case D_DOC_RENDER_OK:             { return "ok";             }
        case D_DOC_RENDER_SINK_FULL:      { return "sink_full";      }
        case D_DOC_RENDER_SINK_ERROR:     { return "sink_error";     }
        case D_DOC_RENDER_DEPTH_EXCEEDED: { return "depth_exceeded"; }
        case D_DOC_RENDER_MALFORMED:      { return "malformed";      }
        case D_DOC_RENDER_NO_PRIMITIVE:   { return "no_primitive";   }
        case D_DOC_RENDER_RESERVATION:    { return "reservation";    }
        case D_DOC_RENDER_RECORDER:       { return "recorder";       }
        default:                          { return "unknown";        }
    }
}

// ii.    algebra validity and introspection
int32_t     d_doc_render_algebra_is_valid(
                const struct d_doc_render_algebra* _algebra);
int32_t     d_doc_render_algebra_reads(
                const struct d_doc_render_algebra* _algebra,
                const char*                        _key);


// ===========================================================================
// VII.   layout assertions
// ===========================================================================
//   The Layout law.  The verb block is asserted as a contiguous run of
// twenty-one function pointers so that a member added in the middle -- the
// likeliest accidental edit to a frozen interface -- fails to compile rather
// than silently reordering a published struct.

D_STATIC_ASSERT(offsetof(struct d_doc_render_algebra, begin_document) ==
                    ((2u * sizeof(void*)) + (2u * sizeof(uint32_t))),
                "d_doc_render_algebra header drift");
D_STATIC_ASSERT(offsetof(struct d_doc_render_algebra, write_line) ==
                    (offsetof(struct d_doc_render_algebra, begin_document) +
                     (22u * sizeof(fn_doc_end_document))),
                "d_doc_render_algebra verb drift -- a verb was added, removed, "
                "or reordered");
D_STATIC_ASSERT(sizeof(struct d_doc_render_algebra) ==
                    (offsetof(struct d_doc_render_algebra, begin_document) +
                     (23u * sizeof(fn_doc_end_document))),
                "d_doc_render_algebra layout drift");


D_EXTERN_C_END


#endif  // DJINTERP_C_UTIL_DOCUMENT_RENDER_ALGEBRA_
