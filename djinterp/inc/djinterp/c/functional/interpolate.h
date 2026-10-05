/*******************************************************************************
* djinterp [c]                                                     interpolate.h
*
* The type-agnostic interpolation engine: one left-to-right pass, three
* policies.
*   The C++ face factors find-and-replace out of `text_template` and
* `binary_template` so neither owns it, and splits the variation into three
* independent axes. The C lowering keeps that split exactly, and every one of
* the three turns out to be something this subframework already has:
*
*     scanner  -- WHERE the placeholders are. A pull cursor yielding one piece
*                 (a literal run or a key) per step. That is a `d_producer`.
*     resolver -- WHAT a key becomes. A lookup that may MISS. That is a
*                 `maybe`, and chaining frames so a later one runs only when an
*                 earlier one misses is `alt`, the Alternative instance.
*     sink     -- HOW output is assembled. Literal runs and resolved values
*                 arriving one at a time. That is a `d_reducer`.
*
* So the engine is a producer driven through a fold into a sink, and it needed
* no new machinery -- which is the strongest evidence so far that the tier 1--3
* work was factored correctly rather than merely completed.
*
* A MISS LEAVES THE PLACEHOLDER UNTOUCHED. This is the property that makes
* PARTIAL interpolation well-defined: a template resolved against an incomplete
* frame comes back as a valid template with the unresolved placeholders still in
* it, ready for a second pass. It is not an error condition and is not reported
* as one; `d_interpolate` counts misses separately so a caller who wants
* strictness can check, and one who wants partial application can ignore it.
*
* CHAINING IS ASSOCIATIVE AND COLLAPSES TO ONE PASS. `d_interp_chain` tries each
* frame in order and stops at the first hit, so N chained frames still traverse
* the input once. Associativity is asserted in the conformance suite rather than
* assumed: chaining (a,b) then c must equal chaining a then (b,c).
*
* NOTHING ALLOCATES. The scanner is a producer over the caller's own text, and
* the sink appends into a caller-owned buffer, reporting overflow rather than
* growing. Output is written segment by segment with no intermediate structure,
* which is the same property the C++ engine claims.
*
*
* path:      /inc/djinterp/c/functional/interpolate.h
* link(s):   TBA
* author(s): TBA                                             created: 2026.07.30
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_C_FUNCTIONAL_INTERPOLATE_H
#define DJINTERP_C_FUNCTIONAL_INTERPOLATE_H 1

// std
#include <stddef.h>
#include <string.h>
// djinterp
#include "../djinterp.h"
#include "./functional_common.h"
#include "./producer.h"
#include "./reducer.h"
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


// d_interp_piece_kind
//   enum: which of the two things a scanner step yielded.
enum d_interp_piece_kind
{
    D_INTERP_LITERAL = 0,   // a run of text to copy through unchanged
    D_INTERP_KEY     = 1    // a placeholder key to resolve
};

// d_interp_piece
//   struct: one scanner step -- a literal run or a key.
// `begin` points into the caller's own text; nothing is copied at scan time.
struct d_interp_piece
{
    enum d_interp_piece_kind kind;    // literal or key
    const char*              begin;   // start of the run, in the source text
    size_t                   length;  // its length in bytes
};

// d_interp_scanner
//   struct: the scanner's pull state.
// `open` and `close` give brace syntax (`{key}`); setting `sigil` to a non-zero
// byte selects sigil syntax (`$key`), where a key runs to the first byte that
// is not alphanumeric or underscore.
struct d_interp_scanner
{
    const char* cursor;     // next byte to examine
    const char* end;        // one past the last byte
    char        open;       // opening delimiter, brace syntax
    char        close;      // closing delimiter, brace syntax
    char        sigil;      // sigil byte, or 0 for brace syntax
};

// fn_resolver
//   function pointer: the lookup a key is put through.
// On a hit, points `_out_value` at the replacement and sets `_out_length`, and
// returns true. On a MISS returns false, which leaves the placeholder standing.
// The replacement is borrowed and must outlive the interpolation.
// Note: `_context` may be NULL.
typedef bool (*fn_resolver)(const char*  _key,
                            size_t       _key_length,
                            const char** _out_value,
                            size_t*      _out_length,
                            void*        _context);

// d_interp_frame
//   struct: one resolver together with its context.
struct d_interp_frame
{
    fn_resolver resolve;    // the lookup
    void*       context;    // context forwarded to `resolve`; may be NULL
};

// d_interp_chain
//   struct: an ordered set of frames, tried until one hits.
// This is `alt` over the resolution `maybe`: the first hit wins and later frames
// are not consulted, so a chain costs no more than the frame that answers.
struct d_interp_chain
{
    const struct d_interp_frame* frames;   // caller-owned array
    size_t                       count;    // how many
};

// d_interp_sink
//   struct: a bounded output buffer.
// A full sink counts what it could not take rather than writing out of bounds,
// so overflow is a reported condition and not a corruption.
struct d_interp_sink
{
    char*  buffer;      // caller-owned storage
    size_t capacity;    // bytes available
    size_t written;     // bytes taken
    size_t overflow;    // bytes that did not fit
};

// d_interp_report
//   struct: what one interpolation pass observed.
struct d_interp_report
{
    size_t pieces;      // scanner steps taken
    size_t resolved;    // keys that hit
    size_t missed;      // keys that missed, and were left standing
    size_t written;     // bytes written to the sink
    size_t overflow;    // bytes that did not fit
};

// I.     scanners (the WHERE policy), as producers of pieces
struct d_producer d_interp_brace_scanner(struct d_interp_scanner* _state,
                                         const char*              _text,
                                         size_t                   _length);
struct d_producer d_interp_sigil_scanner(struct d_interp_scanner* _state,
                                         const char*              _text,
                                         size_t                   _length,
                                         char                     _sigil);

// II.    resolvers (the WHAT policy)
struct d_interp_frame d_interp_frame_make(fn_resolver _resolve,
                                          void*       _context);
bool     d_interp_chain_init(struct d_interp_chain*       _chain,
                             const struct d_interp_frame* _frames,
                             size_t                       _count);
bool     d_interp_chain_resolve(const struct d_interp_chain* _chain,
                                const char*                  _key,
                                size_t                       _key_length,
                                const char**                 _out_value,
                                size_t*                      _out_length);

// III.   sinks (the HOW policy)
bool     d_interp_sink_init(struct d_interp_sink* _sink,
                            char*                 _buffer,
                            size_t                _capacity);
bool     d_interp_sink_append(struct d_interp_sink* _sink,
                              const char*           _bytes,
                              size_t                _length);

// IV.    the engine
bool     d_interpolate(struct d_producer*           _scanner,
                       const struct d_interp_chain* _chain,
                       struct d_interp_sink*        _sink,
                       struct d_interp_report*      _report);
bool     d_interpolate_text(const char*                  _text,
                            size_t                       _length,
                            const struct d_interp_chain* _chain,
                            char*                        _out,
                            size_t                       _capacity,
                            struct d_interp_report*      _report);

// V.     layout assertions (Layout law: one declaration, asserted in both)
D_STATIC_ASSERT(offsetof(struct d_interp_frame, context) == sizeof(fn_resolver),
                "d_interp_frame layout drift");
D_STATIC_ASSERT(offsetof(struct d_interp_chain, count) == sizeof(void*),
                "d_interp_chain layout drift");


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_FUNCTIONAL_INTERPOLATE_H
