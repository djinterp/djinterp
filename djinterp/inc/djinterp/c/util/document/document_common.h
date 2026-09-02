/******************************************************************************
* djinterp [c/util/document]                                 document_common.h
*
* THE HINT BAG.  A flat run of key/value pairs, borrowed, in canonical order.
*
* BORROWED, NOT OWNING, AND THAT IS THE REAL DIFFERENCE FROM THE C++ SIDE.
*   `djinterp::doc_attributes` is a `container_metadata<std::string,
* std::string>`: it owns its keys and values and grows itself.  This does
* neither.  `items` points into storage the caller already had -- a parse
* arena, a static array -- and the bag is a view over a contiguous run.
* Copying a `d_doc_attributes` copies three words and shares the run.
*   That divergence is structural rather than chosen: C has no owning string,
* and the parse arena exists precisely so that nothing here allocates.
*
* "CANONICAL" IS NOT A SECOND ORDERING, AND READING IT AS ONE IS THE TRAP.
*   It was described as though the C side sorted while the C++ side kept
* insertion order.  It does not.  `container_metadata::set` walks its vector,
* OVERWRITES IN PLACE when the key is already present, and appends otherwise
* -- so a repeated key keeps its FIRST position and takes its LAST value.
* Canonical form here is defined to be exactly that.
*   THIS IS NOT A PREFERENCE.  `layout_parse_parity.cpp` compares the two bags
* POSITIONALLY: `bags_agree` iterates the C++ container and indexes
* `items[i]` in step.  Any other order fails at the first repeated key.  The
* C++ container's behaviour is the specification; this implements it.
*
* CANONICALISE COMPACTS IN PLACE.  A run arrives as written, duplicates and
* all; the bag that comes back points at a PREFIX of that same run with the
* duplicates removed.  The caller then advances its arena by the bag's count
* rather than by what it wrote -- `layout_parse.c` does exactly that:
* `_arena->attr_used = _first + _out_bag->count;`.  Nothing is copied,
* nothing is freed.
*
* path:      /inc/djinterp/c/util/document/document_common.h
* link(s):   TBA
* author(s): TBA                                            created: 2026.08.23
*
* ---------------------------------------------------------------------------
* RELAY 79 -- THE NODE TIER LANDED HERE, AND ONE HALF OF IT WAS NOT TAKEN.
*
*   Sections I, III, IV, V and VI below arrived with the document node tier in
* pending/document_tier/ and are ADDITIVE: the kind taxonomy, the standard hint
* keys, the alignment interchange and the typed hint readers.  None of them
* depends on hint ORDER, which is why they could be taken as they stood.
*
*   THE ORDERING HALF WAS REFUSED, AND THIS IS THE FILE THAT RECORDS WHY.  That
* drop's own document_common declares "HINT ORDER IS LEXICOGRAPHIC, NOT
* INSERTION ORDER", implements d_doc_attributes_canonicalise as an insertion
* SORT, and implements d_doc_attributes_is_canonical as a test THAT THE BAG IS
* SORTED.  Measured against this tree: its canonicalise turns `zeta alpha
* title` into `alpha title zeta`, and its is_canonical answers 0 for the bag
* this tier calls canonical.
*
*   That is MERGE.md section 4a exactly -- the C side sorting to `alpha zeta`
* against the C++ side's insertion-ordered `zeta alpha` -- which relay 78
* closed by making the C side stop sorting.  Taking those three functions would
* have reopened it, failed the two shipped checks in layout_parse_laws.c that
* assert source order, and made document_node.c reject every well-formed bag it
* was handed, since it calls is_canonical under D_INTERNAL_DOC_CHECK_CANONICAL
* and returns D_DOC_RENDER_MALFORMED when it fails.
*
*   The drop carries the refutation of its own implementation.  Its section VI
* comment states -- correctly -- that `container_metadata` is INSERTION-ordered
* and that a C caller which sorted would disagree with it by a byte, and then
* names its sorting canonicalise as the fix.  The observation is right and the
* prescription inverts it.
*
*   So canonicalise, is_canonical and find below are THIS TREE'S, unchanged.
* d_doc_attr_key_compare is kept because d_doc_attr_find uses it for EQUALITY,
* and is deliberately not described as an ordering authority: canonicalise
* above is, and it does not sort.
* ---------------------------------------------------------------------------
******************************************************************************/

#ifndef DJINTERP_C_UTIL_DOCUMENT_COMMON_
#define DJINTERP_C_UTIL_DOCUMENT_COMMON_ 1

#include <stddef.h>
#include <stdint.h>
#include "../../djinterp.h"
#include "../../../config/core/util/cfg_document.h"   // D_INTERNAL_DOC_*

D_EXTERN_C_BEGIN

// ===========================================================================
// I.     node kinds
// ===========================================================================

// d_doc_node_kind
//   enum: the eleven constructors of ch-documents.tex section 3.  Ten are
// summands of the block sort; `slot` is the free monad's variable, carried as
// a twelfth tag on the same node so that a document and a template are one
// type (ruling R3).  Values are explicit and are part of the wire format:
// append only, never renumber.
enum d_doc_node_kind
{
    D_DOC_KIND_ELEMENT    = 0,  // named container: name, text, hints, blocks
    D_DOC_KIND_HEADING    = 1,  // level (>= 1) + text
    D_DOC_KIND_PARAGRAPH  = 2,  // running text
    D_DOC_KIND_KEY_VALUE  = 3,  // a labelled value: text is the key
    D_DOC_KIND_RULE       = 4,  // a horizontal separator
    D_DOC_KIND_SPACE      = 5,  // a vertical space REQUEST, in points
    D_DOC_KIND_PAGE_BREAK = 6,  // force following content onto a new page
    D_DOC_KIND_LIST       = 7,  // children are items
    D_DOC_KIND_TABLE      = 8,  // children are columns then rows, in that order
    D_DOC_KIND_REPEAT     = 9,  // a binder: children once per sequence item
    D_DOC_KIND_SLOT       = 10, // the hole; text is the slot name

    // the non-block sorts.  Sorting the signature is what makes "a row holds
    // only cells" a checkable claim rather than a convention, and what makes
    // the table emission order a corollary (ch-documents.tex, law 4).
    D_DOC_KIND_DOCUMENT   = 11, // the root: hints + blocks
    D_DOC_KIND_ITEM       = 12, // a list item: text + nested blocks
    D_DOC_KIND_COLUMN     = 13, // a table column header
    D_DOC_KIND_ROW        = 14, // a table row: children are cells
    D_DOC_KIND_CELL       = 15, // one cell of a row

    // column_group
    //   a header level ABOVE the columns: a label spanning the finer
    // positions beneath it.  Its children are columns, or further groups, so
    // a stack of header levels is an ordinary subtree and the SPAN IS
    // DERIVED (d_doc_column_span) rather than stored -- the same call made
    // for a table's column count, and the same shape `table_metadata.hpp`
    // already uses, where a level's extent is the sum of its cells' spans.
    D_DOC_KIND_COLUMN_GROUP = 16,

    D_DOC_KIND_COUNT      = 17
};

// d_doc_sort
//   enum: the six sorts of the many-sorted signature.  A node's kind
// determines its sort (d_doc_kind_sort), and a node's sort determines which
// sorts its children may have (d_doc_child_sort).
enum d_doc_sort
{
    D_DOC_SORT_DOCUMENT = 0,
    D_DOC_SORT_BLOCK    = 1,
    D_DOC_SORT_ITEM     = 2,
    D_DOC_SORT_COLUMN   = 3,
    D_DOC_SORT_ROW      = 4,
    D_DOC_SORT_CELL     = 5,

    D_DOC_SORT_COUNT    = 6
};



// d_doc_attr
//   struct: one key/value pair.  BOTH BORROWED -- the strings live wherever
// the caller put them, and must outlive every bag that names them.
struct d_doc_attr
{
    const char* key;
    const char* value;
};

// d_doc_attributes
//   struct: a view over a contiguous, canonical run of pairs.
//
//   `reserved` MUST BE ZERO.  It is not slack to ignore: it makes the struct
// the same shape on 32- and 64-bit targets instead of whatever the compiler
// chose, and gives a later field somewhere to go without moving `count`.
struct d_doc_attributes
{
    const struct d_doc_attr* items;     // borrowed run, canonical
    uint32_t                 count;     // pairs in it
    uint32_t                 reserved;  // must be 0
};

// D_DOC_ATTRIBUTES_EMPTY
//   The empty bag: a NULL run and a zero count.  This is what the root of a
// parsed document carries, matching `doc_attributes()` at the same position
// on the C++ side.  An empty bag is NOT the absence of a bag -- every node
// has one.
#define D_DOC_ATTRIBUTES_EMPTY { (const struct d_doc_attr*)0, 0u, 0u }

// d_doc_attributes_canonicalise
//   Compacts `_run` in place to canonical form and returns a bag over it.
//
//   FIRST POSITION, LAST VALUE.  `a=1 b=2 a=3` becomes `a=3 b=2`: the repeat
// does not move `a` to the end, and it does not lose the later value.  That
// is `container_metadata::set` written out, and it is the ONLY ordering
// authority in this tier -- a caller that sorted a run before handing it over
// would break parity rather than help it.
//
//   `_run` IS WRITTEN THROUGH, necessarily: dropping a duplicate means moving
// what follows it down.  A caller needing the original order needs its own
// copy, and none in the corpus does.
//
// Parameter(s):
//   _run:    the pairs as written; compacted in place.  May be NULL iff
//            _count is 0.
//   _count:  how many were written.
// Return:
//   a bag over the canonical prefix of _run.
struct d_doc_attributes d_doc_attributes_canonicalise(struct d_doc_attr* _run,
                                                      uint32_t           _count);

// d_doc_attributes_is_canonical
//   Whether `_bag` holds no repeated key.
//
//   THE POINT IS THE INVARIANT, NOT THE QUERY.  A bag reaching the
// interpreter with a duplicate renders one hint twice and nobody notices.
// This is what an assertion or a parity body calls to say the invariant held,
// not something a caller is expected to branch on.
//
// Return:
//   1 when canonical, 0 otherwise.  An empty bag is canonical.
int32_t d_doc_attributes_is_canonical(const struct d_doc_attributes* _bag);

// d_doc_attributes_find
//   The value bound to `_key`, or NULL.  Linear, like the C++ `find` it
// mirrors: a hint bag holds a handful of entries and an index would cost more
// than it saved.
const char* d_doc_attributes_find(const struct d_doc_attributes* _bag,
                                  const char*                    _key);

// ===========================================================================
// III.   standard hint keys
// ===========================================================================
//   The shared key names.  A convention, not a closed set: the hint space is
// open (ch-documents.tex, non-property 3), so a dialect may honour keys not
// listed here and must ignore keys it does not know.  Named as macros so both
// languages spell them identically and neither can typo one silently.

// D_DOC_ATTR_STYLE
//   constant: the name of a style the dialect resolves; a dialect with no
// style registry ignores it.
#define D_DOC_ATTR_STYLE        "style"

// D_DOC_ATTR_ALIGN
//   constant: horizontal alignment token -- "left", "center", "right",
// "justify" (see d_doc_align_from_string).
#define D_DOC_ATTR_ALIGN        "align"

// D_DOC_ATTR_FONT
//   constant: a font family or face name.
#define D_DOC_ATTR_FONT         "font"

// D_DOC_ATTR_SIZE
//   constant: a font size in points, as a decimal token.
#define D_DOC_ATTR_SIZE         "size"

// D_DOC_ATTR_BOLD
//   constant: boolean face flag (see d_doc_attr_flag).
#define D_DOC_ATTR_BOLD         "bold"

// D_DOC_ATTR_ITALIC
//   constant: boolean face flag.
#define D_DOC_ATTR_ITALIC       "italic"

// D_DOC_ATTR_COLOR
//   constant: foreground colour as "#RRGGBB" or a name.
#define D_DOC_ATTR_COLOR        "color"

// D_DOC_ATTR_BACKGROUND
//   constant: background colour as "#RRGGBB" or a name.
#define D_DOC_ATTR_BACKGROUND   "background"

// D_DOC_ATTR_WIDTH
//   constant: a preferred extent, interpreted by the dialect -- character
// cells for a text dialect, a length for a typeset one.
#define D_DOC_ATTR_WIDTH        "width"

// D_DOC_ATTR_WRAP
//   constant: whether over-long content may wrap ("true") or is clipped.
#define D_DOC_ATTR_WRAP         "wrap"

// D_DOC_ATTR_INDENT
//   constant: a leading indent in cells or levels, as a decimal token.
#define D_DOC_ATTR_INDENT       "indent"

// D_DOC_ATTR_LOCATOR
//   constant: a cross-reference anchor NAME.
//
//   REMOVED 2026-08-02, RESTORED THE SAME DAY.  The removal argued that a
// blessed `locator` was the only textual evidence contradicting the
// reference-free carrier, and that removing it cost nothing because the hint
// space is open.  Both halves of that were wrong, and the emission side's
// recorder is what shows it.
//
//   THE PREMISE WAS FALSE.  The removal rested on "nothing dereferences an
// anchor".  `d_pdf_record.h` states that the bridge calls
// d_pdf_recorder_anchor when it sees this hint -- which is a dereference, and
// is the entire mechanism of the two-pass protocol.
//
//   AND "IT COSTS NOTHING" WAS FALSE.  An open hint space means an
// unrecognised key is legal, not that two parties will spell it alike.  The
// moment a SECOND party reads a key, the shared #define is the only thing
// stopping one side writing "locator" and the other "anchor" -- a divergence
// no dialect would report, because both are legal keys that nobody reads.  A
// name in this list is a claim about the model; it is also the agreement.
//
//   THE CARRIER IS UNAFFECTED.  Dereferencing an anchor builds an EXTERNAL
// index from label to page.  No node comes to point at another node, so the
// closed document is still reference-free, still mu F, and equality is still
// structural.  What changed is that this key is now semantically load-bearing
// for one dialect -- which is exactly what reads(phi) exists to declare, and
// where it should be declared.
#define D_DOC_ATTR_LOCATOR      "locator"

// D_DOC_ATTR_COLSPAN
//   constant: columns a cell spans, as a decimal token.  A dialect that can
// express merges honours it; one that cannot ignores it and the producer pads
// the covered positions instead.
#define D_DOC_ATTR_COLSPAN      "colspan"

// D_DOC_ATTR_ROWSPAN
//   constant: rows a cell spans, as a decimal token.
#define D_DOC_ATTR_ROWSPAN      "rowspan"


// ===========================================================================
// IV.    alignment
// ===========================================================================

// d_doc_align
//   enum: horizontal alignment.  The `align` hint's decoded form, and the only
// hint given a first-class accessor -- every other key is read as a string or
// as a flag.
enum d_doc_align
{
    D_DOC_ALIGN_LEFT    = 0,
    D_DOC_ALIGN_CENTER  = 1,
    D_DOC_ALIGN_RIGHT   = 2,
    D_DOC_ALIGN_JUSTIFY = 3
};


// ===========================================================================
// V.     accessors
// ===========================================================================
//   All of these are total: an absent key yields the caller's fallback and
// nothing here can fail.  A hint that is present but unparseable is treated as
// absent, which is the tolerance rule (ch-documents.tex, law 3) applied to a
// malformed value rather than an unknown key.

// i.     kinds and sorts
enum d_doc_sort  d_doc_kind_sort(enum d_doc_node_kind _kind);
enum d_doc_sort  d_doc_child_sort(enum d_doc_node_kind _kind);
const char*      d_doc_kind_name(enum d_doc_node_kind _kind);
bool             d_doc_kind_takes_children(enum d_doc_node_kind _kind);
bool             d_doc_kind_is_open(enum d_doc_node_kind _kind);

// ii.    hint lookup
const char*      d_doc_attr_find(const struct d_doc_attributes* _attrs,
                                 const char*                    _key);
bool             d_doc_attr_has(const struct d_doc_attributes* _attrs,
                                const char*                    _key);
const char*      d_doc_attr_or(const struct d_doc_attributes* _attrs,
                               const char*                    _key,
                               const char*                    _fallback);
bool             d_doc_attr_flag(const struct d_doc_attributes* _attrs,
                                 const char*                    _key,
                                 bool                           _fallback);
uint32_t         d_doc_attr_uint(const struct d_doc_attributes* _attrs,
                                 const char*                    _key,
                                 uint32_t                       _fallback);
int32_t          d_doc_attr_fixed(const struct d_doc_attributes* _attrs,
                                  const char*                    _key,
                                  uint32_t                       _scale,
                                  int32_t                        _fallback);
int32_t          d_doc_attr_milli(const struct d_doc_attributes* _attrs,
                                  const char*                    _key,
                                  int32_t                        _fallback);
enum d_doc_align d_doc_attr_align(const struct d_doc_attributes* _attrs,
                                  enum d_doc_align               _fallback);

// iii.   key comparison
//   EQUALITY ONLY.  d_doc_attr_find uses this to match a key.  It is NOT
// the ordering authority in this tier -- d_doc_attributes_canonicalise is,
// and it does not sort.  See the relay 79 note in the banner.
int              d_doc_attr_key_compare(const char* _left,
                                        const char* _right);

// iv.    alignment interchange
const char*      d_doc_align_to_string(enum d_doc_align _align);
enum d_doc_align d_doc_align_from_string(const char*      _token,
                                         enum d_doc_align _fallback);


// ===========================================================================
// VI.    layout assertions
// ===========================================================================
//   The Layout law: one declaration, size and offset asserted in both
// dialects.  Stated relative to the pointer width so that they hold on every
// tier rather than only on LP64.

D_STATIC_ASSERT(offsetof(struct d_doc_attr, value) ==
                    sizeof(const char*),
                "d_doc_attr layout drift");
D_STATIC_ASSERT(sizeof(struct d_doc_attr) ==
                    (2u * sizeof(const char*)),
                "d_doc_attr layout drift");
D_STATIC_ASSERT(offsetof(struct d_doc_attributes, count) ==
                    sizeof(const struct d_doc_attr*),
                "d_doc_attributes layout drift");
D_STATIC_ASSERT(sizeof(struct d_doc_attributes) ==
                    (sizeof(const struct d_doc_attr*) +
                     (2u * sizeof(uint32_t))),
                "d_doc_attributes layout drift");


D_EXTERN_C_END

#endif  // DJINTERP_C_UTIL_DOCUMENT_COMMON_
