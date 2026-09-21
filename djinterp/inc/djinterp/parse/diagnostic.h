/******************************************************************************
* djinterp [parse]                                                diagnostic.h
*
* The parse subframework's diagnostic channel.
*   A stage reports what it found -- where, how badly, and why -- into a sink
* the caller supplies. Every stage of a parsing or generation pipeline shares
* one sink, so a run produces one ordered report rather than a bool per stage
* and a message the caller cannot see.
*
*   Three properties make this usable as pipeline plumbing rather than as a
* logger. A diagnostic is a 32-byte POD holding an arena OFFSET, not a pointer,
* so a sink is memcpy-able, hashable, and serialisable as a unit. A sink runs
* on caller-supplied storage, so a stage that must not allocate does not. And
* a full sink keeps TALLYING what it can no longer STORE, so "did this run
* fail" stays correct after the storage is exhausted.
*
*   Domains partition the code space the way an op_set partitions an opcode
* space: every emitter owns its own numbering and no two emitters can collide.
* parse owns the domains below D_PARSE_DIAG_DOMAIN_PARSEGEN; parsegen numbers
* its stages upward from there; anything at or above D_PARSE_DIAG_DOMAIN_USER
* belongs to the application.
*
*   Requires: c/djinterp.h (qualifier kit) and config/parse/cfg_parse.h.
*
* path:      /inc/djinterp/parse/diagnostic.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CLASSIFICATION
    --------------
    1.  Severity
         1.  d_parse_severity
    2.  Domains
         1.  Domain identifiers
              a. D_PARSE_DIAG_DOMAIN_CORE
              b. D_PARSE_DIAG_DOMAIN_MACHINE
              c. D_PARSE_DIAG_DOMAIN_PROGRAM
              d. D_PARSE_DIAG_DOMAIN_PARSEGEN
              e. D_PARSE_DIAG_DOMAIN_USER
    3.  Core codes
         1.  d_parse_diag_code
2.  TYPES
    -----
    1.  Source location
         1.  d_parse_span
         2.  D_PARSE_SPAN_UNKNOWN
    2.  The diagnostic record
         1.  d_parse_diagnostic
         2.  Diagnostic flags
              a. D_PARSE_DIAG_FLAG_CONTINUATION
         3.  D_PARSE_DIAG_NO_MESSAGE
    3.  The sink
         1.  d_parse_diag_hook
         2.  d_parse_diag_sink
         3.  Sink flags
              a. D_PARSE_DIAG_SINK_OWNS_ITEMS
              b. D_PARSE_DIAG_SINK_OWNS_TEXT
              c. D_PARSE_DIAG_SINK_TRUNCATED
3.  OPERATIONS
    ----------
    1.  Span construction
    2.  Sink lifetime
    3.  Emission
    4.  Inspection
    5.  Rendering
*/

#ifndef DJINTERP_PARSE_DIAGNOSTIC_
#define DJINTERP_PARSE_DIAGNOSTIC_ 1

// std
#include <stddef.h>                         // size_t
#include <stdint.h>                         // uint8_t, uint16_t, uint32_t
// djinterp
#include "../c/djinterp.h"                  // framework root
#include "../config/parse/cfg_parse.h"      // D_INTERNAL_PARSE_DIAG_* knobs


//==============================================================================
// 1.  CLASSIFICATION
//==============================================================================
// What a diagnostic is, who emitted it, and which of that emitter's conditions
// it reports. The three together are the stable identity of a message; the
// text is for humans and may be reworded freely.


// 1.1    Severity
//------------------------------------------------------------------------------
// 1.1.1
// d_parse_severity
//   enum: how badly a diagnostic bears on the run. Ordered, so a sink filters
// with a single comparison and a caller asks "anything at or above error?".
// ERROR means the stage's output is unusable; FATAL additionally means the
// pipeline cannot continue far enough to find more.
enum d_parse_severity
{
    D_PARSE_SEVERITY_NOTE    = 0,
    D_PARSE_SEVERITY_WARNING = 1,
    D_PARSE_SEVERITY_ERROR   = 2,
    D_PARSE_SEVERITY_FATAL   = 3,
    D_PARSE_SEVERITY_COUNT   = 4
};


// 1.2    Domains
//------------------------------------------------------------------------------
// 1.2.1
// D_PARSE_DIAG_DOMAIN_CORE
//   constant: diagnostics emitted by the diagnostic facility itself.
#define D_PARSE_DIAG_DOMAIN_CORE        0u
// D_PARSE_DIAG_DOMAIN_MACHINE
//   constant: diagnostics emitted by the execution substrate (machine.h).
#define D_PARSE_DIAG_DOMAIN_MACHINE     1u
// D_PARSE_DIAG_DOMAIN_PROGRAM
//   constant: diagnostics emitted while verifying or reading a program
// (program.h).
#define D_PARSE_DIAG_DOMAIN_PROGRAM     2u
// D_PARSE_DIAG_DOMAIN_PARSEGEN
//   constant: the first domain reserved for the generator subsystem. parsegen
// numbers its frontends, families, passes, and backends upward from here.
#define D_PARSE_DIAG_DOMAIN_PARSEGEN    64u
// D_PARSE_DIAG_DOMAIN_USER
//   constant: the first domain djinterp will never claim. Application code
// numbers its own emitters upward from here.
#define D_PARSE_DIAG_DOMAIN_USER        1024u


// 1.3    Core codes
//------------------------------------------------------------------------------
// 1.3.1
// d_parse_diag_code
//   enum: the conditions D_PARSE_DIAG_DOMAIN_CORE and _MACHINE report. Every
// other domain numbers its own codes from zero; a code is meaningful only
// alongside the domain that emitted it.
enum d_parse_diag_code
{
    D_PARSE_DIAG_TRUNCATED     = 0,
    D_PARSE_DIAG_STEP_LIMIT    = 1,
    D_PARSE_DIAG_UNKNOWN_OP    = 2,
    D_PARSE_DIAG_BAD_ARGUMENT  = 3,
    D_PARSE_DIAG_OUT_OF_MEMORY = 4,
    D_PARSE_DIAG_FAMILY_MISMATCH = 5,
    D_PARSE_DIAG_BAD_TARGET      = 6,
    D_PARSE_DIAG_BAD_OPERAND     = 7,
    D_PARSE_DIAG_BAD_FORMAT      = 8,
    D_PARSE_DIAG_CAPACITY        = 9
};


//==============================================================================
// 2.  TYPES
//==============================================================================


// 2.1    Source location
//------------------------------------------------------------------------------
// 2.1.1
// d_parse_span
//   struct: a half-open byte range within one source, optionally carrying the
// line and column that range begins at. `source` identifies which text, so a
// pipeline reading several inputs -- an included grammar, a chained stage's
// output -- reports against the right one. Line and column are optional
// because computing them costs a scan the emitter may not want to pay;
// zero means "not computed", which is unambiguous as both are 1-based.
struct d_parse_span
{
    uint32_t source;
    uint32_t offset;
    uint32_t length;
    uint32_t line;
    uint32_t column;
};

// 2.1.2
// D_PARSE_SPAN_UNKNOWN
//   macro: brace initialiser for a span that points nowhere, for a diagnostic
// about a whole input rather than a place in it.
#define D_PARSE_SPAN_UNKNOWN            { 0u, 0u, 0u, 0u, 0u }


// 2.2    The diagnostic record
//------------------------------------------------------------------------------
// 2.2.1
// d_parse_diagnostic
//   struct: one reported condition. `message` is a byte OFFSET into the owning
// sink's arena rather than a pointer, which is what keeps the record a
// position-independent POD: an array of these plus its arena copies, hashes,
// and serialises with no fix-up. Read the text with d_parse_diag_message.
struct d_parse_diagnostic
{
    uint8_t             severity;
    uint8_t             flags;
    uint16_t            domain;
    uint16_t            code;
    uint16_t            reserved;
    struct d_parse_span span;
    uint32_t            message;
};

// 2.2.2
// D_PARSE_DIAG_FLAG_CONTINUATION
//   constant: this diagnostic elaborates the one before it -- the "note:
// declared here" that follows an error -- rather than standing alone. A
// renderer indents it; a caller counting problems skips it.
#define D_PARSE_DIAG_FLAG_CONTINUATION  0x01u

// 2.2.3
// D_PARSE_DIAG_NO_MESSAGE
//   constant: the `message` value meaning no text was stored, either because
// none was given or because the arena was full.
#define D_PARSE_DIAG_NO_MESSAGE         0xFFFFFFFFu


// 2.3    The sink
//------------------------------------------------------------------------------
// 2.3.1
// d_parse_diag_sink (forward)
//   struct: declared ahead of the hook so the tag the hook names is the file's
// tag and not one scoped to that prototype.
struct d_parse_diag_sink;

// d_parse_diag_hook
//   type: called once per accepted diagnostic, before storage is attempted, so
// a caller that wants diagnostics streamed rather than collected sees every
// one even after the sink fills. Must not emit into the same sink.
typedef void (*d_parse_diag_hook)(void*                            _ctx,
                                  const struct d_parse_diag_sink*  _sink,
                                  const struct d_parse_diagnostic* _diag);

// 2.3.2
// d_parse_diag_sink
//   struct: the collection point. Records land in `items` and their text in
// the `text` arena, both of which may be caller-supplied, heap-allocated, or
// absent. `tally` counts every ACCEPTED diagnostic whether or not it was
// stored, so d_parse_diag_failed answers correctly even for a sink with no
// storage at all -- which is the cheapest useful configuration: a pure
// pass/fail counter that allocates nothing and stores nothing.
//   Public-payload layout: the sink IS the protocol the stages share, so its
// members are unprefixed and directly readable.
struct d_parse_diag_sink
{
    struct d_parse_diagnostic* items;
    uint32_t                   count;
    uint32_t                   capacity;
    char*                      text;
    uint32_t                   text_used;
    uint32_t                   text_capacity;
    uint32_t                   tally[D_PARSE_SEVERITY_COUNT];
    uint32_t                   dropped;
    uint8_t                    min_severity;
    uint8_t                    flags;
    uint16_t                   reserved;
    d_parse_diag_hook          hook;
    void*                      hook_ctx;
};

// 2.3.3
// D_PARSE_DIAG_SINK_OWNS_ITEMS
//   constant: the record array was allocated by the sink and is freed by
// d_parse_diag_sink_release.
#define D_PARSE_DIAG_SINK_OWNS_ITEMS    0x01u
// D_PARSE_DIAG_SINK_OWNS_TEXT
//   constant: the message arena was allocated by the sink.
#define D_PARSE_DIAG_SINK_OWNS_TEXT     0x02u
// D_PARSE_DIAG_SINK_TRUNCATED
//   constant: at least one accepted diagnostic could not be stored, or at
// least one message did not fit the arena. The tallies remain exact.
#define D_PARSE_DIAG_SINK_TRUNCATED     0x04u


//==============================================================================
// 3.  OPERATIONS
//==============================================================================


//   C linkage for everything below, so a C++ translation unit can consume this
// header and link against the C archive. Both spellings expand to nothing
// under a C compiler, so a C-only build sees no trace of them.
D_EXTERN_C_BEGIN

// 3.1    Span construction
//------------------------------------------------------------------------------
/*
d_parse_span_make
  Builds a span over a byte range of the primary source, with no line or
column resolved.

Parameter(s):
  _offset: byte offset of the first character in the range.
  _length: length of the range in bytes; 0 denotes a point.
Return:
  The span.
*/
D_INLINE struct d_parse_span
d_parse_span_make(
    uint32_t _offset,
    uint32_t _length
)
{
    const struct d_parse_span span = { 0u, _offset, _length, 0u, 0u };

    return span;
}

/*
d_parse_span_located
  Builds a fully described span.

Parameter(s):
  _source: identifier of the text the range belongs to; 0 is the primary.
  _offset: byte offset of the first character in the range.
  _length: length of the range in bytes.
  _line:   1-based line of the first character; 0 if not computed.
  _column: 1-based column of the first character; 0 if not computed.
Return:
  The span.
*/
D_INLINE struct d_parse_span
d_parse_span_located(
    uint32_t _source,
    uint32_t _offset,
    uint32_t _length,
    uint32_t _line,
    uint32_t _column
)
{
    const struct d_parse_span span =
        { _source, _offset, _length, _line, _column };

    return span;
}

/*
d_parse_span_unknown
  Builds a span that points nowhere, for a diagnostic about an input as a
whole rather than a place within it.

Return:
  The span.
*/
D_INLINE struct d_parse_span
d_parse_span_unknown(void)
{
    const struct d_parse_span span = D_PARSE_SPAN_UNKNOWN;

    return span;
}

// 3.2    Sink lifetime
//------------------------------------------------------------------------------
void            d_parse_diag_sink_init(struct d_parse_diag_sink*  _sink,
                                       struct d_parse_diagnostic* _items,
                                       uint32_t                   _item_count,
                                       char*                      _text,
                                       uint32_t                   _text_bytes);
#if (D_INTERNAL_PARSE_DIAG_HEAP == 1)
D_NODISCARD int d_parse_diag_sink_init_heap(struct d_parse_diag_sink* _sink,
                                            uint32_t _item_count,
                                            uint32_t _text_bytes);
#endif  // D_INTERNAL_PARSE_DIAG_HEAP
void            d_parse_diag_sink_hook(struct d_parse_diag_sink* _sink,
                                       d_parse_diag_hook         _hook,
                                       void*                     _ctx);
void            d_parse_diag_sink_filter(struct d_parse_diag_sink* _sink,
                                         int                       _minimum);
void            d_parse_diag_sink_reset(struct d_parse_diag_sink* _sink);
void            d_parse_diag_sink_release(struct d_parse_diag_sink* _sink);

// 3.3    Emission
//------------------------------------------------------------------------------
int             d_parse_diag_emit(struct d_parse_diag_sink* _sink,
                                  int                       _severity,
                                  uint16_t                  _domain,
                                  uint16_t                  _code,
                                  struct d_parse_span       _span,
                                  const char*               _message);
int             d_parse_diag_emit_flagged(struct d_parse_diag_sink* _sink,
                                          int                       _severity,
                                          uint16_t                  _domain,
                                          uint16_t                  _code,
                                          uint8_t                   _flags,
                                          struct d_parse_span       _span,
                                          const char*               _message);
#if (D_INTERNAL_PARSE_DIAG_FORMAT == 1)
int             d_parse_diag_emitf(struct d_parse_diag_sink* _sink,
                                   int                       _severity,
                                   uint16_t                  _domain,
                                   uint16_t                  _code,
                                   struct d_parse_span       _span,
                                   const char*               _format,
                                   ...);
#endif  // D_INTERNAL_PARSE_DIAG_FORMAT

// 3.4    Inspection
//------------------------------------------------------------------------------
/*
d_parse_diag_at
  The stored diagnostic at an index.

Parameter(s):
  _sink:  the sink to read; may be NULL.
  _index: zero-based index into the stored records.
Return:
  A pointer to the record, or NULL if the sink or the index is invalid.
*/
D_INLINE const struct d_parse_diagnostic*
d_parse_diag_at(
    const struct d_parse_diag_sink* _sink,
    uint32_t                        _index
)
{
    // reject a missing sink, an unstored index, or storage that was never set
    if ( (!_sink)                ||
         (!_sink->items)         ||
         (_index >= _sink->count) )
    {
        return NULL;
    }

    return &_sink->items[_index];
}

/*
d_parse_diag_message
  The text of a diagnostic, resolved against the arena of the sink that holds
it.

Parameter(s):
  _sink: the sink the diagnostic was emitted into; may be NULL.
  _diag: the diagnostic to read; may be NULL.
Return:
  The message, or the empty string when none was stored. Never NULL, so a
caller may print the result unconditionally.
*/
D_INLINE const char*
d_parse_diag_message(
    const struct d_parse_diag_sink*  _sink,
    const struct d_parse_diagnostic* _diag
)
{
    // reject a missing sink, a missing record, an absent or truncated message,
    // and an offset that does not point into the live part of the arena
    if ( (!_sink)                                  ||
         (!_diag)                                  ||
         (!_sink->text)                            ||
         (_diag->message == D_PARSE_DIAG_NO_MESSAGE) ||
         (_diag->message >= _sink->text_used)      )
    {
        return "";
    }

    return _sink->text + _diag->message;
}

/*
d_parse_diag_tally
  How many diagnostics of one severity the sink accepted, whether or not it
had room to store them.

Parameter(s):
  _sink:     the sink to read; may be NULL.
  _severity: a d_parse_severity value.
Return:
  The count, or 0 for an invalid sink or severity.
*/
D_INLINE uint32_t
d_parse_diag_tally(
    const struct d_parse_diag_sink* _sink,
    int                             _severity
)
{
    // reject a missing sink or a severity outside the enum
    if ( (!_sink)              ||
         (_severity < 0)       ||
         (_severity >= D_PARSE_SEVERITY_COUNT) )
    {
        return 0u;
    }

    return _sink->tally[_severity];
}

/*
d_parse_diag_failed
  Whether anything was reported that makes the run's output unusable.
NOTE:
  This is the question a stage asks before consuming the previous stage's
output, and it stays correct on a sink with no storage.

Parameter(s):
  _sink: the sink to read; may be NULL, which reports no failure.
Return:
  A boolean value corresponding to either:
  - 1, if any error or fatal diagnostic was accepted, or
  - 0, otherwise.
*/
D_INLINE int
d_parse_diag_failed(
    const struct d_parse_diag_sink* _sink
)
{
    if (!_sink)
    {
        return 0;
    }

    return ( (_sink->tally[D_PARSE_SEVERITY_ERROR] > 0u) ||
             (_sink->tally[D_PARSE_SEVERITY_FATAL] > 0u) )
           ? 1
           : 0;
}

/*
d_parse_diag_truncated
  Whether the sink dropped a record or a message it accepted.

Parameter(s):
  _sink: the sink to read; may be NULL.
Return:
  A boolean value corresponding to either:
  - 1, if storage was exhausted at some point, or
  - 0, otherwise.
*/
D_INLINE int
d_parse_diag_truncated(
    const struct d_parse_diag_sink* _sink
)
{
    if (!_sink)
    {
        return 0;
    }

    return ((_sink->flags & D_PARSE_DIAG_SINK_TRUNCATED) != 0u) ? 1 : 0;
}

// 3.5    Rendering
//------------------------------------------------------------------------------
const char*     d_parse_severity_name(int _severity);
size_t          d_parse_diag_format(const struct d_parse_diag_sink*  _sink,
                                    const struct d_parse_diagnostic* _diag,
                                    char*                            _out,
                                    size_t                           _size);
void            d_parse_diag_print(const struct d_parse_diag_sink* _sink);

D_EXTERN_C_END


#endif  // DJINTERP_PARSE_DIAGNOSTIC_
