/******************************************************************************
* djinterp [utility]                                            layout_parse.h
*
*   The TEXTUAL construction surface, ported.  A block-form DSL parsed into the
* same annotated term the declarative and procedural surfaces build:
*
*       section "Introduction" style="lead" {
*           content "intro.body"
*           section "Motivation" { content "intro.motivation" }
*       }
*
*   WHY THIS PORT WAS UNBLOCKED, AND WHY THE DEFERRAL WAS WRONG.  D7 deferred
* this file "until parse is redone", on the reading that it depends on the parse
* subframework.  It does not.  Reading the C++ file by symbol rather than by its
* include line: of everything `parse.hpp` offers -- the CRTP combinators, the
* erased `parser<R, E>` handle, the grammar and token tiers -- `layout_parse.hpp`
* uses `parse_state<char>` (a POD of data/length/offset with at_end, current and
* advance), `parse_result<T>`, and two status codes.  Nothing else.  Its own
* banner says as much: it is "a recursive descent ... rather than a
* self-referential combinator tree".
*   So the dependency is on a cursor over a char buffer, which C spells in nine
* lines, not on the machinery being redone.  The deferral was asserted from the
* include line rather than from the symbols used -- the same error the handoff
* records four times over.
*
*   THE CURSOR IS RESTATED, NOT WRAPPED.  `d_layout_parse_state` mirrors
* `parse::parse_state<char>` field for field, and the assertions in section VII
* pin the correspondence.  This is the one place the port declares a type the
* C++ side also declares, and it is deliberate: wrapping would mean this C file
* including a C++ header, which is the dependency the port exists to remove.
*
*   TWO REGIONS, BECAUSE PARSING PRODUCES.  A bag BORROWS its entries and a
* `d_layout_atom` BORROWS its strings, so a pass that computes rather than reads
* has nowhere in `d_layout_arena` to put what it computes.  `layout_interpret.h`
* met this first and answered it with `d_layout_number_arena`; this is the same
* answer for the same reason, and the two are deliberately the same shape.
*
*   BAGS COME OUT CANONICAL.  The C++ parser appends attributes in the order it
* scanned them, which is source order.  A bag emitted in source order and a bag
* emitted canonically are the same bag and different byte streams, which is
* precisely the divergence `D_INTERNAL_DOC_CHECK_CANONICAL` warns about and the
* parity law caught at `element_attrs_helper`.  So every bag this parser builds
* goes through `d_doc_attributes_canonicalise` before it reaches a node --
* never a second sort written here.
*
*
* TABLE OF CONTENTS
* =================
* I.    THE CURSOR                          (d_layout_parse_state)
* II.   DIAGNOSTICS                         (status, error)
* III.  THE GRAMMAR                         (the dialect's parse table)
* IV.   THE ARENA                           (what parsing produces)
* V.    ENTRY                               (d_layout_parse_document)
* VI.   SCANNING                            (exposed for the fixture)
* VII.  LAYOUT ASSERTIONS
*
*
* path:      \inc\djinterp\c\util\document\layout_parse.h
* link(s):   ch-synthesis.tex, ch-parsing.tex
* author(s): TBA                                            created: 2026.08.07
******************************************************************************/

#ifndef DJINTERP_C_UTIL_DOCUMENT_LAYOUT_PARSE_
#define DJINTERP_C_UTIL_DOCUMENT_LAYOUT_PARSE_ 1

#include <stddef.h>
#include <stdint.h>
#include "../../djinterp.h"
#include "../../functional/free.h"
#include "./document_common.h"
#include "./layout.h"

D_EXTERN_C_BEGIN


// ===========================================================================
// I.     the cursor
// ===========================================================================

// d_layout_parse_state
//   struct: a cursor over the source text.  Mirrors `parse::parse_state<char>`
// field for field and in field ORDER, so the two are the same layout and the
// assertions in section VII can say so.
//
//   `data` is BORROWED and must outlive both the parse and the tree it
// produces, because a literal atom points into it -- see the arena's note on
// why scanned tokens are copied and source spans are not.
struct d_layout_parse_state
{
    const char* data;
    size_t      length;
    size_t      offset;
};

//   The four operations the recursive descent actually uses.  Declared rather
// than macroed so a fixture can take their addresses, and so the C++ side's
// semantics -- advance CLAMPS, current is NULL at end -- are stated once in a
// definition rather than three times at call sites.
void        d_layout_parse_state_init(struct d_layout_parse_state* _state,
                                      const char*                  _data,
                                      size_t                       _length);
size_t      d_layout_parse_remaining(const struct d_layout_parse_state* _state);
int32_t     d_layout_parse_at_end(const struct d_layout_parse_state* _state);
const char* d_layout_parse_current(const struct d_layout_parse_state* _state);
void        d_layout_parse_advance(struct d_layout_parse_state* _state,
                                   size_t                       _count);


// ===========================================================================
// II.    diagnostics
// ===========================================================================

// d_layout_parse_status
//   enum: the outcome codes.  The three the recursive descent can produce are
// given the SAME NUMERIC VALUES as `parse::DParseStatus*`, because a caller
// bridging the two tiers will compare them and a renumbering here would be a
// silent mistranslation.  The two codes this parser never returns are listed
// anyway, at their C++ values, so the correspondence is total rather than
// partial.
enum d_layout_parse_status
{
    D_LAYOUT_PARSE_SUCCESS      = 0,
    D_LAYOUT_PARSE_FAILURE      = 1,
    D_LAYOUT_PARSE_END_OF_INPUT = 2,
    D_LAYOUT_PARSE_OVERFLOW     = 3,   /* arena exhausted; C++ has no arena */
    D_LAYOUT_PARSE_MALFORMED    = 4,   /* reserved; not produced here */

    D_LAYOUT_PARSE_STATUS_COUNT = 5
};

// d_layout_parse_error
//   struct: where it went wrong and what was expected there.
//
//   `message` is a STRING LITERAL with static storage, never arena text.  The
// C++ side builds messages by concatenation ("unknown construct '" + keyword +
// "'"), which needs an allocator; the offset already names the keyword by
// position, so the concatenation buys a nicer message at the cost of the one
// property this tier promises -- that nothing allocates.  The keyword is
// reported separately instead, as a span into the source.
struct d_layout_parse_error
{
    int32_t     status;       // enum d_layout_parse_status
    uint32_t    reserved;     // pad; must be 0
    size_t      offset;       // where in the source
    const char* message;      // static literal, never NULL
    const char* token;        // borrowed span into the source, or NULL
    size_t      token_length; // length of that span
};

const char* d_layout_parse_status_name(int32_t _status);


// ===========================================================================
// III.   the grammar
// ===========================================================================

// d_layout_block_word / d_layout_leaf_word
//   struct: one row of the dialect's parse table.  The C++ side spells these
// as std::pair inside a std::vector; C spells them as arrays of named structs,
// which is the same table with the field names written down.
struct d_layout_block_word
{
    const char* keyword;
    int32_t     op;
    int32_t     reserved;   // pad; must be 0
};

struct d_layout_leaf_word
{
    const char* keyword;
    int32_t     kind;       // enum d_layout_atom_kind
    int32_t     reserved;   // pad; must be 0
};

// d_layout_grammar
//   struct: the dialect's parse configuration.  THE PARSER NAMES NO CONSTRUCT.
// Everything construct-specific arrives here, exactly as the interpreter takes
// its signature -- which is what lets one parser serve every dialect.
//
//   `arg_key` names the bag entry a block's positional argument fills; when it
// is NULL the parser uses "title", which is the C++ default constructor's value
// rather than a preference invented here.
struct d_layout_grammar
{
    const struct d_layout_block_word* block_words;
    size_t                            block_count;

    const struct d_layout_leaf_word*  leaf_words;
    size_t                            leaf_count;

    int32_t                           root_op;
    int32_t                           reserved;   // pad; must be 0
    const char*                       arg_key;    // NULL => "title"
};

//   Table lookup, exposed because a dialect wants to validate its own table and
// because the fixture asserts first-match-wins on a duplicated keyword.
const struct d_layout_block_word*
d_layout_grammar_block_for(const struct d_layout_grammar* _grammar,
                           const char*                    _keyword,
                           size_t                         _keyword_length);

const struct d_layout_leaf_word*
d_layout_grammar_leaf_for(const struct d_layout_grammar* _grammar,
                          const char*                    _keyword,
                          size_t                         _keyword_length);


// ===========================================================================
// IV.    the arena
// ===========================================================================

// d_layout_parse_arena
//   struct: caller-owned storage for what PARSING produces -- the hint entries
// of the bags it builds, and NUL-terminated copies of the tokens it scans.
//
//   WHY TOKENS ARE COPIED RATHER THAN POINTED AT.  A `d_layout_atom` holds
// `const char*` and every consumer reads it with the str* functions, so it must
// be NUL-terminated.  A span of the source is not: `section "Introduction"` has
// a quote where the terminator would go.  Writing the terminator INTO the source
// would mean a parser that mutates its input, which also makes the source
// unusable for a second parse and unusable at all when it is a string literal.
// So scanned tokens are copied out, and this region is where they land.
//
//   THE SOURCE IS STILL BORROWED AND STILL MUST OUTLIVE THE TREE, because the
// error struct's `token` is a span into it.  Two borrowings with different
// lifetimes would be the trap `layout_interpret.h` records for number tables;
// here they are the same lifetime, and stating it is cheaper than discovering it.
struct d_layout_parse_arena
{
    struct d_doc_attr* attrs;          // caller-owned hint entries
    size_t             attr_capacity;
    size_t             attr_used;

    char*              text;           // caller-owned token storage
    size_t             text_capacity;
    size_t             text_used;
};

int32_t d_layout_parse_arena_init(struct d_layout_parse_arena* _arena,
                                  struct d_doc_attr*           _attrs,
                                  size_t                       _attr_capacity,
                                  char*                        _text,
                                  size_t                       _text_capacity);

void    d_layout_parse_arena_rewind(struct d_layout_parse_arena* _arena);


// D_LAYOUT_PARSE_MAX_SIBLINGS
//   constant: the most blocks one sequence may hold.
//
//   WHY A CONSTANT AND NOT A PARAMETER.  A sequence's children are gathered
// into a contiguous run before `d_layout_apply` copies them into the tree, and
// that run lives in the recursing frame -- so its bound must be a constant
// expression.  The alternative is a caller-supplied pool shared by every
// sequence, which turns "this document nests too deep" into "this document is
// too big somewhere", reported at whichever sequence happened to be last.
//   A document needing wider fan-out nests a further construct; that is what
// the root op is for.  Raise this and the frame grows by
// sizeof(struct d_cofree) per step, per level of nesting, so it is a real
// budget rather than a free knob.
#define D_LAYOUT_PARSE_MAX_SIBLINGS  64u


// ===========================================================================
// V.     entry
// ===========================================================================

//   Parses a top-level sequence of blocks and wraps it in the grammar's root
// construct.  Fails on any malformed block, on trailing input, and on either
// arena running out.
//
//   `_error` may be NULL when the caller only wants the yes/no.  On success it
// is zeroed rather than left stale, because a caller that checks the error
// struct instead of the return value is the failure this costs one memset to
// prevent.
int32_t d_layout_parse_document(struct d_layout_parse_state*   _state,
                                const struct d_layout_grammar* _grammar,
                                struct d_layout_arena*         _tree_arena,
                                struct d_layout_parse_arena*   _parse_arena,
                                struct d_cofree**              _out_tree,
                                struct d_layout_parse_error*   _error);

//   The convenience overload: parses a NUL-terminated source string.  Builds
// the cursor and delegates, so the two cannot diverge.
int32_t d_layout_parse_source(const char*                    _source,
                              const struct d_layout_grammar* _grammar,
                              struct d_layout_arena*         _tree_arena,
                              struct d_layout_parse_arena*   _parse_arena,
                              struct d_cofree**              _out_tree,
                              struct d_layout_parse_error*   _error);


// ===========================================================================
// VI.    scanning
// ===========================================================================
//   Exposed because the fixture tests them directly.  The C++ side keeps these
// in an internal namespace; C has no such thing, and hiding them behind `static`
// would mean the fixture could only reach them through whole-document parses --
// which is how the "cursor reversing its children" mutation escaped in the first
// place.  Named with the module prefix so the exposure is not accidental.

int32_t d_layout_parse_is_space(char _c);
int32_t d_layout_parse_is_ident(char _c);
void    d_layout_parse_skip_spaces(struct d_layout_parse_state* _state);

//   A run of identifier characters after leading whitespace.  Returns the run's
// LENGTH and sets `_out_begin` to its first character; a length of 0 means no
// identifier was there, and the cursor is left after the whitespace either way.
size_t  d_layout_parse_scan_identifier(struct d_layout_parse_state* _state,
                                       const char**                 _out_begin);

//   A "double-quoted" literal after leading whitespace, copied into the arena
// and returned NUL-terminated.  Returns NULL without consuming when no quote
// opens; returns NULL having consumed when the literal is unterminated or the
// arena is full, which the caller distinguishes by `_out_status`.
const char* d_layout_parse_scan_string(struct d_layout_parse_state* _state,
                                       struct d_layout_parse_arena* _arena,
                                       int32_t*                     _out_status);


// ===========================================================================
// VII.   layout assertions
// ===========================================================================
//   The cursor is the one type this port restates rather than wraps, so its
// correspondence to `parse::parse_state<char>` is pinned here rather than
// trusted.  A C++ translation unit that includes both headers asserts the
// sizes agree; this side asserts the field order, which is what a
// reinterpreting bridge would actually depend on.
D_STATIC_ASSERT(offsetof(struct d_layout_parse_state, length) ==
                    sizeof(const char*),
                "d_layout_parse_state layout drift -- length must follow data");
D_STATIC_ASSERT(offsetof(struct d_layout_parse_state, offset) ==
                    offsetof(struct d_layout_parse_state, length) +
                        sizeof(size_t),
                "d_layout_parse_state layout drift -- offset must follow length");

//   The status codes are the C++ ones.  Asserted as values rather than trusted
// to the comment above them, because a renumbering is exactly the edit that
// looks harmless.
D_STATIC_ASSERT(D_LAYOUT_PARSE_SUCCESS      == 0 &&
                D_LAYOUT_PARSE_FAILURE      == 1 &&
                D_LAYOUT_PARSE_END_OF_INPUT == 2,
                "d_layout_parse_status drift from parse::DParseStatus*");

//   The two table rows are pointer-then-int32-then-pad; a caller building a
// table as a brace initialiser depends on that order.
D_STATIC_ASSERT(offsetof(struct d_layout_block_word, op) == sizeof(const char*),
                "d_layout_block_word layout drift");
D_STATIC_ASSERT(offsetof(struct d_layout_leaf_word, kind) == sizeof(const char*),
                "d_layout_leaf_word layout drift");

//   NO `d_layout_parse_attributes` ENTRY POINT, DELIBERATELY.  Parsing a bare
// attribute list without a construct around it is expressible, and the C++ side
// exposes the scanner that would do it -- but a bag with no node is not a term,
// and offering it would invite a caller to build bags outside the canonicalising
// path.  A caller wanting one parses a leaf construct and reads its bag.

D_EXTERN_C_END

#endif  // DJINTERP_C_UTIL_DOCUMENT_LAYOUT_PARSE_
