/*******************************************************************************
* djinterp [c][parse]                                               diagnostic.h
*
* The parse subframework's diagnostic channel.
*   A stage reports what it found - where, how badly, and why - into a sink
* the caller supplies. Every stage of a parsing or generation pipeline shares
* one sink, so a run produces one ordered report rather than a bool per stage
* and a message the caller cannot see.
*
*   Three properties make this usable as pipeline plumbing rather than as a
* logger. A diagnostic is a 36-byte POD holding arena OFFSETS, not pointers,
* so a sink is memcpy-able, hashable, and serialisable as a unit. A sink runs
* on caller-supplied storage, so a stage that must not allocate does not. And
* a full sink keeps TALLYING what it can no longer STORE, so "did this run
* fail" stays correct after the storage is exhausted.
*
*   A diagnostic carries its FACTS - the values its message names: a token's
* text, a count - and its text is WRITTEN FROM A TEMPLATE over them, the one
* its code's table gives (D_PARSE_CORE_CODES here, D_PARSE_EXPR_CODES in
* expr/parse.h), so the text a sink stores and the record a log writes come
* from one definition. What a diagnostic carries is chosen three ways: a
* build's knobs (D_CFG_PARSE_DIAG_TEXT, _FACTS), a run's rows of the djin
* `parse` (diag_text, diag_facts, through d_parse_register), and, for a
* streaming hook, the flags it asks with. Text is written only where a sink
* stores it or a hook asks for it.
*
*   Domains partition the code space the way an op_set partitions an opcode
* space: every emitter owns its own numbering and no two emitters can collide.
* parse owns the domains below D_PARSE_DIAG_DOMAIN_PARSEGEN; parsegen numbers
* its stages upward from there; anything at or above D_PARSE_DIAG_DOMAIN_USER
* belongs to the application.
*
*   Requires: c/djinterp.h (qualifier kit) and config/parse/cfg_parse.h.
*
*
* path:      /inc/djinterp/c/modules/parse/diagnostic.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.19
*                                                            revised: 2026.11.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CLASSIFICATION
    --------------
    1.  Severity
         1.  dj_parse_severity
    2.  Domains
         1.  Domain identifiers
              a. D_PARSE_DIAG_DOMAIN_CORE
              b. D_PARSE_DIAG_DOMAIN_MACHINE
              c. D_PARSE_DIAG_DOMAIN_PROGRAM
              d. D_PARSE_DIAG_DOMAIN_LEX
              e. D_PARSE_DIAG_DOMAIN_EXPR
              f. D_PARSE_DIAG_DOMAIN_PARSEGEN
              g. D_PARSE_DIAG_DOMAIN_USER
    3.  Core codes
         1.  dj_parse_diag_code
         2.  D_PARSE_CORE_CODES
2.  TYPES
    -----
    1.  Source location
         1.  dj_parse_span
         2.  D_PARSE_SPAN_UNKNOWN
    2.  The diagnostic record
         1.  dj_parse_diagnostic
         2.  Diagnostic flags
              a. D_PARSE_DIAG_FLAG_CONTINUATION
         3.  D_PARSE_DIAG_NO_MESSAGE
         4.  D_PARSE_DIAG_NO_FACTS
    3.  Facts
         1.  dj_parse_diag_fact_kind
         2.  dj_parse_diag_fact
         3.  dj_parse_diag_fact_entry
    4.  Reports and events
         1.  dj_parse_diag_report
         2.  dj_parse_diag_event
         3.  D_PARSE_DIAG_NOTES_MAX
    5.  The sink
         1.  dj_parse_diag_hook
         2.  dj_parse_diag_stream_fn
         3.  dj_parse_diag_sink
         4.  Sink flags
              a. D_PARSE_DIAG_SINK_OWNS_ITEMS
              b. D_PARSE_DIAG_SINK_OWNS_TEXT
              c. D_PARSE_DIAG_SINK_TRUNCATED
              d. D_PARSE_DIAG_SINK_OWNS_FACTS
         5.  What a diagnostic carries
              a. D_PARSE_DIAG_CARRY_TEXT
              b. D_PARSE_DIAG_CARRY_FACTS
         6.  What a streaming hook asks for
              a. D_PARSE_DIAG_STREAM_TEXT
              b. D_PARSE_DIAG_STREAM_FACTS
    6.  Settings
         1.  dj_parse_diag_settings
         2.  dj_parse_diag_settings_fn
3.  OPERATIONS
    ----------
    1.  Span construction
    2.  Fact construction
    3.  Sink lifetime
    4.  Emission
    5.  Inspection
    6.  Templates
    7.  Settings
    8.  Rendering
*/

#ifndef DJINTERP_C_PARSE_DIAGNOSTIC_H
#define DJINTERP_C_PARSE_DIAGNOSTIC_H 1

// std
#include <stddef.h>                        // size_t
#include <string.h>                        // memset
// djinterp
#include "../../djinterp.h"              // framework root
#include "../../../config/c/modules/parse/cfg_parse.h"  // D_INTERNAL_PARSE_DIAG_* knobs
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // uint32_t, uint8_t, uint16_t,
                                              // int32_t


//==============================================================================
// 1.  CLASSIFICATION
//==============================================================================
// What a diagnostic is, who emitted it, and which of that emitter's conditions
// it reports. The three together are the stable identity of a message; the
// text is for humans and may be reworded freely.


// 1.1    Severity
//------------------------------------------------------------------------------
// 1.1.1
// dj_parse_severity
//   enum: how badly a diagnostic bears on the run. Ordered, so a sink filters
// with a single comparison and a caller asks "anything at or above error?".
// ERROR means the stage's output is unusable; FATAL additionally means the
// pipeline cannot continue far enough to find more.
enum dj_parse_severity
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
// D_PARSE_DIAG_DOMAIN_LEX
//   constant: diagnostics emitted by the C and C++ scanner and literal decoder
// (lex/lex_scan.h, lex/lex_literal.h). Codes are enum dj_lex_error.
#define D_PARSE_DIAG_DOMAIN_LEX         3u
// D_PARSE_DIAG_DOMAIN_EXPR
//   constant: diagnostics emitted by expression core v2 (expr/signature.h,
// expr/parse.h). Codes are enum dj_parse_expr_diag.
#define D_PARSE_DIAG_DOMAIN_EXPR        4u
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
// dj_parse_diag_code
//   enum: the conditions D_PARSE_DIAG_DOMAIN_CORE and _MACHINE report. Every
// other domain numbers its own codes from zero; a code is meaningful only
// alongside the domain that emitted it.
enum dj_parse_diag_code
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

// 1.3.2
// D_PARSE_CORE_CODES
//   macro: parse.core's code table, one row per code above, in the log's form
// (log.h section 2.4), as expr/parse.h's D_PARSE_EXPR_CODES is for
// parse.expr. A row is X(m, number, name, class, grade, template, facts):
// the module's token, as the table is given it; the code's value; its name
// under parse.core; its class in the shared vocabulary and its grade, as
// tokens; the template its message is written from; and the facts that
// template may name. The codes are reported in the core, machine and program
// domains alike, so a reader keys on the code. The table needs no header: it
// is text until expanded. diagnostic.c holds each row's name and number to
// the enum at compile time.
//   Where an emitter gives words of its own rather than a template (the
// machine's and the program's, until they move onto the table), a sink
// stores those words, and the log writes the row's template with the
// emitter's words as the fact `message`.
#define D_PARSE_CORE_CODES(X, m)                                              \
    X(m, 0, truncated,       FORMAT_TRUNCATED,   REJECTED,                    \
      "the input ends before it is complete",                                 \
      ())                                                                     \
    X(m, 1, step_limit,      RESOURCE_BUDGET,    FAILED,                      \
      "the run passed its step limit",                                        \
      ())                                                                     \
    X(m, 2, unknown_op,      LOOKUP_NOT_FOUND,   REJECTED,                    \
      "no operator is registered for an opcode",                              \
      ())                                                                     \
    X(m, 3, bad_argument,    ARGUMENT_INVALID,   REJECTED,                    \
      "an argument is not valid",                                             \
      ())                                                                     \
    X(m, 4, out_of_memory,   RESOURCE_MEMORY,    FAILED,                      \
      "out of memory",                                                        \
      ())                                                                     \
    X(m, 5, family_mismatch, TYPE_MISMATCH,      REJECTED,                    \
      "the program was built for another operator family",                   \
      ())                                                                     \
    X(m, 6, bad_target,      POSITION_RANGE,     REJECTED,                    \
      "a target lies outside the program",                                    \
      ())                                                                     \
    X(m, 7, bad_operand,     LOOKUP_NOT_FOUND,   REJECTED,                    \
      "an operand names nothing the program holds",                           \
      ())                                                                     \
    X(m, 8, bad_format,      FORMAT_UNSUPPORTED, REJECTED,                    \
      "the buffer is not a program of a known format",                        \
      ())                                                                     \
    X(m, 9, capacity,        RESOURCE_BUDGET,    FAILED,                      \
      "{?what}the {what}'s{:}the{/} storage is full",                         \
      (what))


//==============================================================================
// 2.  TYPES
//==============================================================================


// 2.1    Source location
//------------------------------------------------------------------------------
// 2.1.1
// dj_parse_span
//   struct: a half-open byte range within one source, optionally carrying the
// line and column that range begins at. `source` identifies which text, so a
// pipeline reading several inputs - an included grammar, a chained stage's
// output - reports against the right one. Line and column are optional
// because computing them costs a scan the emitter may not want to pay;
// zero means "not computed", which is unambiguous as both are 1-based.
struct dj_parse_span
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
// dj_parse_diagnostic
//   struct: one reported condition. `message` is a byte OFFSET into the owning
// sink's text arena, and `facts` one into its facts arena, of `fact_count`
// entries (2.3.3), rather than pointers, which is what keeps the record a
// position-independent POD: an array of these plus its arenas copies,
// hashes, and serialises with no fix-up. Read the text with
// d_parse_diag_message and the facts with d_parse_diag_fact_at.
struct dj_parse_diagnostic
{
    uint8_t              severity;
    uint8_t              flags;
    uint16_t             domain;
    uint16_t             code;
    uint16_t             fact_count;
    struct dj_parse_span span;
    uint32_t             message;
    uint32_t             facts;
};

// the layout a sink's readers and writers rely on, at every level
D_STATIC_ASSERT(sizeof(struct dj_parse_diagnostic) == 36u,
                "dj_parse_diagnostic must stay nine 32-bit words");

// 2.2.2
// D_PARSE_DIAG_FLAG_CONTINUATION
//   constant: this diagnostic elaborates the one before it - the "note:
// declared here" that follows an error - rather than standing alone. A
// renderer indents it; a caller counting problems skips it.
#define D_PARSE_DIAG_FLAG_CONTINUATION  0x01u

// 2.2.3
// D_PARSE_DIAG_NO_MESSAGE
//   constant: the `message` value meaning no text was stored, either because
// none was given or because the arena was full.
#define D_PARSE_DIAG_NO_MESSAGE         0xFFFFFFFFu

// 2.2.4
// D_PARSE_DIAG_NO_FACTS
//   constant: the `facts` value meaning no facts were stored, because none
// were given, none are carried, or the facts arena was full.
#define D_PARSE_DIAG_NO_FACTS           0xFFFFFFFFu


// 2.3    Facts
//------------------------------------------------------------------------------
// 2.3.1
// dj_parse_diag_fact_kind
//   enum: what a fact's value is. A template's field writes TEXT as its
// bytes, an INTEGER in decimal, a FLAG as true or false, and NONE as nothing;
// a template's test {?name} holds for text not empty, an integer not zero, a
// true flag, and never for NONE - as the template language has it
// (text/ostream_template.h), so a log writes a record's text as a sink does.
enum dj_parse_diag_fact_kind
{
    D_PARSE_DIAG_FACT_NONE    = 0,
    D_PARSE_DIAG_FACT_TEXT    = 1,
    D_PARSE_DIAG_FACT_INTEGER = 2,
    D_PARSE_DIAG_FACT_FLAG    = 3
};

// 2.3.2
// dj_parse_diag_fact
//   struct: one named value. As an emitter gives it, the name and the text
// are borrowed for the call; as d_parse_diag_fact_at reads it back, they
// point into the sink's facts arena, where each ends in a NUL. An integer is
// a magnitude, `high` and `low`, and a sign, so the struct is the same in C
// and every C++, whether or not a build can spell a 64-bit type.
struct dj_parse_diag_fact
{
    const char* name;      // its name, as its row's facts list it
    const char* text;      // TEXT: the bytes; need not end in a NUL
    uint32_t    size;      // TEXT: their count
    uint32_t    low;       // INTEGER: magnitude, bits 0-31; FLAG: 0 or 1
    uint32_t    high;      // INTEGER: magnitude, bits 32-63
    uint8_t     kind;      // enum dj_parse_diag_fact_kind
    uint8_t     negative;  // INTEGER: 1 when below zero
    uint16_t    reserved;  // 0
};

// 2.3.3
// dj_parse_diag_fact_entry
//   struct: a stored fact, as the facts arena holds it: each field an offset
// or a value, so the arena is position-independent as the records are. The
// arena holds a record's entries one after another, then each name and each
// text, NUL-terminated. Entries are written and read with memcpy, so the
// arena needs no alignment. Read facts with d_parse_diag_fact_at.
struct dj_parse_diag_fact_entry
{
    uint32_t name;      // the name's offset in the facts arena
    uint32_t value;     // TEXT: the bytes' offset; INTEGER: low; FLAG: 0, 1
    uint32_t extra;     // TEXT: their count; INTEGER: high
    uint8_t  kind;      // enum dj_parse_diag_fact_kind
    uint8_t  negative;  // INTEGER: 1 when below zero
    uint16_t reserved;  // 0
};

D_STATIC_ASSERT(sizeof(struct dj_parse_diag_fact_entry) == 16u,
                "dj_parse_diag_fact_entry must stay four 32-bit words");


// 2.4    Reports and events
//------------------------------------------------------------------------------
// 2.4.1
// dj_parse_diag_report
//   struct: one diagnostic as an emitter gives it to d_parse_diag_report:
// its classification and span, its facts, and either a template its text is
// written from (`pattern`, its code's row's) or words of its own (`message`).
// A pattern wins where both are given. Everything is borrowed for the call.
struct dj_parse_diag_report
{
    uint8_t                          severity;    // dj_parse_severity
    uint8_t                          flags;       // D_PARSE_DIAG_FLAG_*
    uint16_t                         domain;
    uint16_t                         code;
    uint16_t                         fact_count;
    struct dj_parse_span             span;
    const char*                      pattern;     // a template; may be NULL
    const char*                      message;     // literal; may be NULL
    const struct dj_parse_diag_fact* facts;       // may be NULL with 0
};

// 2.4.2
// dj_parse_diag_event
//   struct: one diagnostic, with the notes that continue it, as a streaming
// hook is given it. `record` is the record the sink made, stored or not. Its
// text is given where the hook asked for it (D_PARSE_DIAG_STREAM_TEXT) and
// it is carried; a diagnostic without a pattern is also given its words
// wherever text is carried, since they are all it says. Its facts are given
// where the hook asked for them and they are carried. `pattern` is its
// template, or NULL. Everything is borrowed for the hook's call.
struct dj_parse_diag_event
{
    const struct dj_parse_diagnostic* record;
    const char*                       pattern;
    const char*                       text;        // NULL where not given
    size_t                            text_size;
    const struct dj_parse_diag_fact*  facts;       // NULL where not given
    uint32_t                          fact_count;
    uint32_t                          note_count;
    const struct dj_parse_diag_event* notes;       // NULL with 0
};

// 2.4.3
// D_PARSE_DIAG_NOTES_MAX
//   constant: the most notes a streaming hook is given with one diagnostic.
// A report's notes past it are tallied, stored and given to the per-record
// hook, but not streamed.
#define D_PARSE_DIAG_NOTES_MAX          8u


// 2.5    The sink
//------------------------------------------------------------------------------
// 2.5.1
// dj_parse_diag_sink (forward)
//   struct: declared ahead of the hooks so the tag they name is the file's
// tag and not one scoped to their prototypes.
struct dj_parse_diag_sink;

// dj_parse_diag_hook
//   type: called once per accepted diagnostic, after storage is attempted, so
// a caller that wants diagnostics streamed rather than collected sees every
// one even after the sink fills. Must not emit into the same sink.
typedef void (*dj_parse_diag_hook)(void*                            _ctx,
                                  const struct dj_parse_diag_sink*  _sink,
                                  const struct dj_parse_diagnostic* _diag);

// 2.5.2
// dj_parse_diag_stream_fn
//   type: a streaming hook: called once per accepted diagnostic, with its
// notes, its facts and its text as it asked (d_parse_diag_sink_stream), after
// every per-record hook call for them. Must not emit into the same sink.
typedef void (*dj_parse_diag_stream_fn)(
    void*                             _ctx,
    const struct dj_parse_diag_sink*  _sink,
    const struct dj_parse_diag_event* _event);

// 2.5.3
// dj_parse_diag_sink
//   struct: the collection point. Records land in `items`, their text in the
// `text` arena and their facts in the `facts` arena, each of which may be
// caller-supplied, heap-allocated, or absent. `tally` counts every ACCEPTED
// diagnostic whether or not it was stored, so d_parse_diag_failed answers
// correctly even for a sink with no storage at all - which is the cheapest
// useful configuration: a pure pass/fail counter that allocates nothing and
// stores nothing. `carry` says what its diagnostics carry (2.5.5), and
// `stream_flags` what its streaming hook asks for (2.5.6).
//   Public-payload layout: the sink IS the protocol the stages share, so its
// members are unprefixed and directly readable.
struct dj_parse_diag_sink
{
    struct dj_parse_diagnostic* items;
    uint32_t                   count;
    uint32_t                   capacity;
    char*                      text;
    uint32_t                   text_used;
    uint32_t                   text_capacity;
    uint32_t                   tally[D_PARSE_SEVERITY_COUNT];
    uint32_t                   dropped;
    uint8_t                    min_severity;
    uint8_t                    flags;
    uint8_t                    carry;
    uint8_t                    stream_flags;
    dj_parse_diag_hook          hook;
    void*                      hook_ctx;
    char*                      facts;
    uint32_t                   facts_used;
    uint32_t                   facts_capacity;
    dj_parse_diag_stream_fn     stream;
    void*                      stream_ctx;
};

// 2.5.4
// D_PARSE_DIAG_SINK_OWNS_ITEMS
//   constant: the record array was allocated by the sink and is freed by
// d_parse_diag_sink_release.
#define D_PARSE_DIAG_SINK_OWNS_ITEMS    0x01u
// D_PARSE_DIAG_SINK_OWNS_TEXT
//   constant: the message arena was allocated by the sink.
#define D_PARSE_DIAG_SINK_OWNS_TEXT     0x02u
// D_PARSE_DIAG_SINK_TRUNCATED
//   constant: at least one accepted diagnostic could not be stored, or at
// least one message or one record's facts did not fit its arena. The tallies
// remain exact.
#define D_PARSE_DIAG_SINK_TRUNCATED     0x04u
// D_PARSE_DIAG_SINK_OWNS_FACTS
//   constant: the facts arena was allocated by the sink.
#define D_PARSE_DIAG_SINK_OWNS_FACTS    0x08u

// 2.5.5
// D_PARSE_DIAG_CARRY_TEXT, D_PARSE_DIAG_CARRY_FACTS
//   constant: what a sink's diagnostics carry: their text, their facts. A
// sink starts with what the run's settings say (d_parse_diag_settings), of
// what the build has (D_CFG_PARSE_DIAG_TEXT, _FACTS); d_parse_diag_sink_carry
// changes it for that sink.
#define D_PARSE_DIAG_CARRY_TEXT         0x01u
#define D_PARSE_DIAG_CARRY_FACTS        0x02u

// 2.5.6
// D_PARSE_DIAG_STREAM_TEXT, D_PARSE_DIAG_STREAM_FACTS
//   constant: what a streaming hook asks to be given: each diagnostic's text,
// written for it where the sink does not store it; each diagnostic's facts.
// They are the carry bits' values, and a hook is given only what the sink's
// diagnostics carry.
#define D_PARSE_DIAG_STREAM_TEXT        D_PARSE_DIAG_CARRY_TEXT
#define D_PARSE_DIAG_STREAM_FACTS       D_PARSE_DIAG_CARRY_FACTS


// 2.6    Settings
//------------------------------------------------------------------------------
// 2.6.1
// dj_parse_diag_settings
//   struct: the run's switches for parse's diagnostics, each 0 or 1: whether
// they carry their text, whether they carry their facts, and whether parse's
// log unit writes them (parse_log.h). In force: the djin `parse`'s rows
// diag_text, diag_facts and log once d_parse_register has run, and the knobs
// otherwise; a switch whose facility the build lacks reads 0.
struct dj_parse_diag_settings
{
    uint8_t text;
    uint8_t facts;
    uint8_t log;
    uint8_t reserved;
};

// 2.6.2
// dj_parse_diag_settings_fn
//   type: a reader of the run's switches, which d_parse_register installs
// (settings.h); it fills every member.
typedef void (*dj_parse_diag_settings_fn)(
    struct dj_parse_diag_settings* _settings);


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
D_INLINE
struct dj_parse_span d_parse_span_make(
    uint32_t _offset,
    uint32_t _length
)
{
    const struct dj_parse_span span = { 0u, _offset, _length, 0u, 0u };

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
D_INLINE
struct dj_parse_span d_parse_span_located(
    uint32_t _source,
    uint32_t _offset,
    uint32_t _length,
    uint32_t _line,
    uint32_t _column
)
{
    const struct dj_parse_span span =
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
D_INLINE
struct dj_parse_span d_parse_span_unknown(void)
{
    const struct dj_parse_span span = D_PARSE_SPAN_UNKNOWN;

    return span;
}

// 3.2    Fact construction
//------------------------------------------------------------------------------
//   None of these can fail. The name and the text are borrowed until the
// report is made.

/*
d_parse_diag_fact_none
  A fact with no value: a field writes nothing, and a test of it fails.

Parameter(s):
  _name: the fact's name.
Return:
  The fact.
*/
D_INLINE
struct dj_parse_diag_fact d_parse_diag_fact_none(
    const char* _name
)
{
    struct dj_parse_diag_fact fact;

    memset(&fact, 0, sizeof(fact));
    fact.name = _name;

    return fact;
}

/*
d_parse_diag_fact_text
  A fact whose value is text of a known length.

Parameter(s):
  _name: the fact's name.
  _text: the bytes; NULL makes a fact with no value.
  _size: how many.
Return:
  The fact.
*/
D_INLINE
struct dj_parse_diag_fact d_parse_diag_fact_text(
    const char* _name,
    const char* _text,
    uint32_t    _size
)
{
    struct dj_parse_diag_fact fact = d_parse_diag_fact_none(_name);

    // a null pointer has no text to borrow
    if (_text != NULL)
    {
        fact.text = _text;
        fact.size = _size;
        fact.kind = (uint8_t)D_PARSE_DIAG_FACT_TEXT;
    }

    return fact;
}

/*
d_parse_diag_fact_unsigned
  A fact whose value is an unsigned integer.

Parameter(s):
  _name:  the fact's name.
  _value: the value.
Return:
  The fact.
*/
D_INLINE
struct dj_parse_diag_fact d_parse_diag_fact_unsigned(
    const char*   _name,
    unsigned long _value
)
{
    struct dj_parse_diag_fact fact = d_parse_diag_fact_none(_name);

    // two shifts of 16, so a 32-bit unsigned long shifts to 0, never too far
    fact.kind = (uint8_t)D_PARSE_DIAG_FACT_INTEGER;
    fact.low  = (uint32_t)(_value & 0xFFFFFFFFul);
    fact.high = (uint32_t)(((_value >> 16) >> 16) & 0xFFFFFFFFul);

    return fact;
}

/*
d_parse_diag_fact_signed
  A fact whose value is a signed integer.

Parameter(s):
  _name:  the fact's name.
  _value: the value.
Return:
  The fact.
*/
D_INLINE
struct dj_parse_diag_fact d_parse_diag_fact_signed(
    const char* _name,
    long        _value
)
{
    // the magnitude is taken in unsigned arithmetic, which LONG_MIN survives
    struct dj_parse_diag_fact fact =
        d_parse_diag_fact_unsigned(_name,
                                   (_value < 0L)
                                   ? (0ul - (unsigned long)_value)
                                   : (unsigned long)_value);

    fact.negative = (_value < 0L) ? 1u : 0u;

    return fact;
}

/*
d_parse_diag_fact_flag
  A fact whose value is true or false.

Parameter(s):
  _name: the fact's name.
  _flag: nonzero for true.
Return:
  The fact.
*/
D_INLINE
struct dj_parse_diag_fact d_parse_diag_fact_flag(
    const char* _name,
    int         _flag
)
{
    struct dj_parse_diag_fact fact = d_parse_diag_fact_none(_name);

    fact.kind = (uint8_t)D_PARSE_DIAG_FACT_FLAG;
    fact.low  = (_flag != 0) ? 1u : 0u;

    return fact;
}

// 3.3    Sink lifetime
//------------------------------------------------------------------------------
void d_parse_diag_sink_init(
    struct dj_parse_diag_sink*  _sink,
    struct dj_parse_diagnostic* _items,
    uint32_t                    _item_count,
    char*                       _text,
    uint32_t                    _text_bytes
);
#if (D_INTERNAL_PARSE_DIAG_HEAP == 1)
D_NODISCARD int d_parse_diag_sink_init_heap(
    struct dj_parse_diag_sink* _sink,
    uint32_t                   _item_count,
    uint32_t                   _text_bytes
);
#endif  // D_INTERNAL_PARSE_DIAG_HEAP
void d_parse_diag_sink_facts(
    struct dj_parse_diag_sink* _sink,
    char*                      _facts,
    uint32_t                   _facts_bytes
);
uint8_t d_parse_diag_sink_carry(
    struct dj_parse_diag_sink* _sink,
    uint8_t                    _carry
);
void d_parse_diag_sink_hook(
    struct dj_parse_diag_sink* _sink,
    dj_parse_diag_hook         _hook,
    void*                      _ctx
);
uint8_t d_parse_diag_sink_stream(
    struct dj_parse_diag_sink* _sink,
    dj_parse_diag_stream_fn    _stream,
    void*                      _ctx,
    uint8_t                    _flags
);
void d_parse_diag_sink_filter(
    struct dj_parse_diag_sink* _sink,
    int                        _minimum
);
void d_parse_diag_sink_reset(
    struct dj_parse_diag_sink* _sink
);
void d_parse_diag_sink_release(
    struct dj_parse_diag_sink* _sink
);

// 3.4    Emission
//------------------------------------------------------------------------------
int d_parse_diag_report(
    struct dj_parse_diag_sink*         _sink,
    const struct dj_parse_diag_report* _reports,
    uint32_t                           _count
);
int d_parse_diag_emit(
    struct dj_parse_diag_sink* _sink,
    int                        _severity,
    uint16_t                   _domain,
    uint16_t                   _code,
    struct dj_parse_span       _span,
    const char*                _message
);
int d_parse_diag_emit_flagged(
    struct dj_parse_diag_sink* _sink,
    int                        _severity,
    uint16_t                   _domain,
    uint16_t                   _code,
    uint8_t                    _flags,
    struct dj_parse_span       _span,
    const char*                _message
);
#if (D_INTERNAL_PARSE_DIAG_FORMAT == 1)
int d_parse_diag_emitf(
    struct dj_parse_diag_sink* _sink,
    int                        _severity,
    uint16_t                   _domain,
    uint16_t                   _code,
    struct dj_parse_span       _span,
    const char*                _format,
    ...
);
#endif  // D_INTERNAL_PARSE_DIAG_FORMAT

// 3.5    Inspection
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
D_INLINE
const struct dj_parse_diagnostic* d_parse_diag_at(
    const struct dj_parse_diag_sink* _sink,
    uint32_t                         _index
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
D_INLINE
const char* d_parse_diag_message(
    const struct dj_parse_diag_sink*  _sink,
    const struct dj_parse_diagnostic* _diag
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
  _severity: a dj_parse_severity value.
Return:
  The count, or 0 for an invalid sink or severity.
*/
D_INLINE
uint32_t d_parse_diag_tally(
    const struct dj_parse_diag_sink* _sink,
    int                              _severity
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
D_INLINE
int d_parse_diag_failed(
    const struct dj_parse_diag_sink* _sink
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
D_INLINE
int d_parse_diag_truncated(
    const struct dj_parse_diag_sink* _sink
)
{
    if (!_sink)
    {
        return 0;
    }

    return ((_sink->flags & D_PARSE_DIAG_SINK_TRUNCATED) != 0u) ? 1 : 0;
}

int32_t d_parse_diag_find(
    const struct dj_parse_diag_sink* _sink,
    uint16_t                         _domain,
    uint16_t                         _code,
    uint32_t                         _from
);
int d_parse_diag_fact_at(
    const struct dj_parse_diag_sink*  _sink,
    const struct dj_parse_diagnostic* _diag,
    uint32_t                          _index,
    struct dj_parse_diag_fact*        _out
);
int32_t d_parse_diag_fact_find(
    const struct dj_parse_diag_sink*  _sink,
    const struct dj_parse_diagnostic* _diag,
    const char*                       _name,
    struct dj_parse_diag_fact*        _out
);

// 3.6    Templates
//------------------------------------------------------------------------------
#if (D_INTERNAL_PARSE_DIAG_TEXT == 1)
size_t d_parse_diag_write(
    const char*                      _pattern,
    const struct dj_parse_diag_fact* _facts,
    uint32_t                         _fact_count,
    char*                            _out,
    size_t                           _size
);
#endif  // D_INTERNAL_PARSE_DIAG_TEXT
int32_t d_parse_diag_template_check(
    const char* _pattern,
    const char* _facts
);

// 3.7    Settings
//------------------------------------------------------------------------------
void d_parse_diag_settings_from(
    dj_parse_diag_settings_fn _reader
);
void d_parse_diag_settings(
    struct dj_parse_diag_settings* _settings
);

// 3.8    Rendering
//------------------------------------------------------------------------------
const char* d_parse_severity_name(
    int _severity
);
size_t d_parse_diag_format(
    const struct dj_parse_diag_sink*  _sink,
    const struct dj_parse_diagnostic* _diag,
    char*                             _out,
    size_t                            _size
);
void d_parse_diag_print(
    const struct dj_parse_diag_sink* _sink
);

D_EXTERN_C_END


#endif  // DJINTERP_C_PARSE_DIAGNOSTIC_H
