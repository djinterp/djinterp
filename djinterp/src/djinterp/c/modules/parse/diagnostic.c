/*******************************************************************************
* djinterp [c][parse]                                               diagnostic.c
*
*   Definitions for the non-inline declarations in diagnostic.h.
*
*
* path:      /src/djinterp/c/modules/parse/diagnostic.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.19
*                                                            revised: 2026.11.04
*******************************************************************************/
#include "../../../../../inc/djinterp/c/modules/parse/diagnostic.h"  // corresponding header
// std
#include <stdio.h>   // printf, snprintf, vsnprintf
#include <string.h>  // memset, memcpy, strlen, strcspn, strncmp
#if (D_INTERNAL_PARSE_DIAG_FORMAT == 1)
#include <stdarg.h>  // va_list, va_start, va_end
#endif  // D_INTERNAL_PARSE_DIAG_FORMAT
#if (D_INTERNAL_PARSE_HEAP == 1)
#include <stdlib.h>  // malloc, free
#endif  // D_INTERNAL_PARSE_HEAP
// djinterp
#include "../../../../../inc/djinterp/c/dckdint.h"  // d_ckd_add, d_ckd_mul


//==============================================================================
// 1.  THE CORE TABLE, HELD TO THE ENUM
//==============================================================================
// the table is expanded with a token of its own for m, so a row that writes a
// module of its own in place of m does not compile; each row's name names its
// constant, and each row's number must be that constant's value

#define D_INTERNAL_DIAG_MODULE_d_internal_m    1
#define D_INTERNAL_DIAG_CODE_truncated         D_PARSE_DIAG_TRUNCATED
#define D_INTERNAL_DIAG_CODE_step_limit        D_PARSE_DIAG_STEP_LIMIT
#define D_INTERNAL_DIAG_CODE_unknown_op        D_PARSE_DIAG_UNKNOWN_OP
#define D_INTERNAL_DIAG_CODE_bad_argument      D_PARSE_DIAG_BAD_ARGUMENT
#define D_INTERNAL_DIAG_CODE_out_of_memory     D_PARSE_DIAG_OUT_OF_MEMORY
#define D_INTERNAL_DIAG_CODE_family_mismatch   D_PARSE_DIAG_FAMILY_MISMATCH
#define D_INTERNAL_DIAG_CODE_bad_target        D_PARSE_DIAG_BAD_TARGET
#define D_INTERNAL_DIAG_CODE_bad_operand       D_PARSE_DIAG_BAD_OPERAND
#define D_INTERNAL_DIAG_CODE_bad_format        D_PARSE_DIAG_BAD_FORMAT
#define D_INTERNAL_DIAG_CODE_capacity          D_PARSE_DIAG_CAPACITY

#define D_INTERNAL_DIAG_CODE_HOLDS(_module, _number, _name, _class, _grade,   \
                                   _template, _facts)                         \
    && (D_INTERNAL_DIAG_MODULE_##_module)                                     \
    && ((_number) == (int)D_INTERNAL_DIAG_CODE_##_name)
#define D_INTERNAL_DIAG_CODE_COUNT(_module, _number, _name, _class, _grade,   \
                                   _template, _facts)                         \
    + 1

D_STATIC_ASSERT( (1 D_PARSE_CORE_CODES(D_INTERNAL_DIAG_CODE_HOLDS,
                                        d_internal_m)),
                 "each row writes m, numbered as its constant" );
D_STATIC_ASSERT( ( (0 D_PARSE_CORE_CODES(D_INTERNAL_DIAG_CODE_COUNT,
                                          d_internal_m)) ==
                   ((int)D_PARSE_DIAG_CAPACITY + 1) ),
                 "the core table has a row for each code" );


//==============================================================================
// 2.  INTERNALS
//==============================================================================

// D_INTERNAL_PARSE_DIAG_DEPTH
//   constant: the deepest a template's blocks may nest.
#define D_INTERNAL_PARSE_DIAG_DEPTH  8u

// d_internal_parse_diag_reader
//   variable: the reader of the run's switches, or NULL for the knobs'.
static dj_parse_diag_settings_fn d_internal_parse_diag_reader = NULL;

/*
d_internal_parse_diag_built
  What the build lets a diagnostic carry, as D_PARSE_DIAG_CARRY_* bits.
*/
static uint8_t
d_internal_parse_diag_built(void)
{
    return (uint8_t)( ((D_INTERNAL_PARSE_DIAG_TEXT == 1)
                       ? D_PARSE_DIAG_CARRY_TEXT  : 0u) |
                      ((D_INTERNAL_PARSE_DIAG_FACTS == 1)
                       ? D_PARSE_DIAG_CARRY_FACTS : 0u) );
}

#if (D_INTERNAL_PARSE_DIAG_TEXT == 1)

/*
d_internal_parse_diag_value_true
  Whether a fact holds, as a template's test reads it: text not empty, an
integer not zero, a true flag; never a fact with no value, nor none at all.
*/
static bool
d_internal_parse_diag_value_true(
    const struct dj_parse_diag_fact* _fact
)
{
    if (!_fact)
    {
        return false;
    }

    switch (_fact->kind)
    {
        case D_PARSE_DIAG_FACT_TEXT:
            return (_fact->size != 0u);
        case D_PARSE_DIAG_FACT_INTEGER:
            return ((_fact->low | _fact->high) != 0u);
        case D_PARSE_DIAG_FACT_FLAG:
            return (_fact->low != 0u);
        default:
            return false;
    }
}

/*
d_internal_parse_diag_lookup
  The fact of a name among the given ones, or NULL.
*/
static const struct dj_parse_diag_fact*
d_internal_parse_diag_lookup(
    const struct dj_parse_diag_fact* _facts,
    uint32_t                         _count,
    const char*                      _name,
    size_t                           _length
)
{
    for (uint32_t i = 0u; ( (_facts) && (i < _count) ); ++i)
    {
        const char* const name = (_facts[i].name) ? _facts[i].name : "";

        if ( (strlen(name) == _length) &&
             (strncmp(name, _name, _length) == 0) )
        {
            return &_facts[i];
        }
    }

    return NULL;
}

#endif  // D_INTERNAL_PARSE_DIAG_TEXT

/*
d_internal_parse_diag_listed
  Whether a facts list, as a table writes it ("(a, b)"), holds a name.
*/
static bool
d_internal_parse_diag_listed(
    const char* _facts,
    const char* _name,
    size_t      _length
)
{
    const char* at = _facts;

    while ( (at) && (*at != '\0') )
    {
        const size_t run = strcspn(at, "(), \t");

        if ( (run == _length) &&
             (run > 0u)       &&
             (strncmp(at, _name, _length) == 0) )
        {
            return true;
        }

        at += (run > 0u) ? run : 1u;
    }

    return false;
}

// dj_internal_parse_diag_tag
//   struct: one brace construct of a template, as it was read: its kind
// ('=' a field, '?' and '^' a test, ':' the other branch, '/' a block's end,
// '{' and '}' an escaped brace, 0 none), its name, and where it ends.
struct dj_internal_parse_diag_tag
{
    char        kind;
    const char* name;
    size_t      length;
    size_t      end;
    bool        plain;
};

/*
d_internal_parse_diag_tag_at
  Reads the construct a template has at a brace: `{{`, `}}`, `{?name}`,
`{^name}`, `{:}`, `{/}`, `{/name}` or `{name}`. `plain` is false where a
field has a specification or a conversion after its name (`{name:spec}`,
`{name!conv}`), which the template language has and parse's writer does
not; such a field is written as `{name}` is.
*/
static struct dj_internal_parse_diag_tag
d_internal_parse_diag_tag_at(
    const char* _pattern,
    size_t      _at
)
{
    struct dj_internal_parse_diag_tag tag;
    const char* const                 p = _pattern + _at;

    tag.kind   = 0;
    tag.name   = NULL;
    tag.length = 0u;
    tag.end    = _at + 1u;
    tag.plain  = true;

    // escaped braces
    if ( (p[0] == '{') && (p[1] == '{') )
    {
        tag.kind = '{';
        tag.end  = _at + 2u;

        return tag;
    }

    if (p[0] == '}')
    {
        tag.kind = '}';
        tag.end  = _at + ( (p[1] == '}') ? 2u : 1u );

        return tag;
    }

    {
        const char* const close = strchr(p, '}');
        const char*       name  = p + 1;

        // an unclosed brace is text
        if (!close)
        {
            return tag;
        }

        tag.end = _at + (size_t)(close - p) + 1u;

        if ( (*name == '?') || (*name == '^') ||
             (*name == ':') || (*name == '/') )
        {
            tag.kind = *name;
            ++name;
        }
        else
        {
            tag.kind = '=';
        }

        tag.name   = name;
        tag.length = strcspn(name, ":!}");

        // a field's specification or conversion is not parse's
        if (name + tag.length != close)
        {
            tag.plain = ( (tag.kind == ':') && (name == close) );
        }
    }

    return tag;
}


//==============================================================================
// 3.  SINKS
//==============================================================================

#if (D_INTERNAL_PARSE_DIAG_TEXT == 1)

/*
d_parse_diag_internal_store_message
  Copies a message into the sink's arena and returns the offset it landed at.
NOTE:
  A message that does not fit is dropped rather than truncated: a half sentence
reads as a bug in the emitter, and the record itself survives either way.

Parameter(s):
  _sink:    the sink whose arena receives the text.
  _message: the text to copy; may be NULL.
Return:
  The byte offset of the stored copy, or D_PARSE_DIAG_NO_MESSAGE when there was
no message or no room for it.
*/
static
uint32_t d_parse_diag_internal_store_message(
    struct dj_parse_diag_sink* _sink,
    const char*                _message
)
{
    // nothing to store, and nowhere to store it, are the same answer
    if ( (!_message)      ||
         (!_sink->text)   ||
         (_sink->text_capacity == 0u) )
    {
        return D_PARSE_DIAG_NO_MESSAGE;
    }

    const size_t length = strlen(_message);
    const size_t needed = length + 1u;
    const size_t free_bytes = (size_t)(_sink->text_capacity - _sink->text_used);

    // mark the sink truncated rather than storing a partial message
    if (needed > free_bytes)
    {
        _sink->flags |= D_PARSE_DIAG_SINK_TRUNCATED;

        return D_PARSE_DIAG_NO_MESSAGE;
    }

    const uint32_t offset = _sink->text_used;

    memcpy(_sink->text + offset, _message, needed);
    _sink->text_used = offset + (uint32_t)needed;

    return offset;
}

#endif  // D_INTERNAL_PARSE_DIAG_TEXT

/*
d_parse_diag_internal_store_text
  Stores a report's text, written from its pattern or copied from its
message, where the sink's diagnostics carry text and it has an arena.
NOTE:
  As for a message, text that does not fit is dropped whole, and the sink is
marked truncated.

Parameter(s):
  _sink:   the sink.
  _report: the report.
Return:
  The offset of the stored text, or D_PARSE_DIAG_NO_MESSAGE.
*/
static
uint32_t d_parse_diag_internal_store_text(
    struct dj_parse_diag_sink*         _sink,
    const struct dj_parse_diag_report* _report
)
{
    if ( ((_sink->carry & D_PARSE_DIAG_CARRY_TEXT) == 0u) ||
         (!_sink->text)                                   ||
         (_sink->text_capacity == 0u) )
    {
        return D_PARSE_DIAG_NO_MESSAGE;
    }

#if (D_INTERNAL_PARSE_DIAG_TEXT == 1)
    // written in place, from the template, and kept only if it all fit
    if (_report->pattern)
    {
        const uint32_t offset     = _sink->text_used;
        const size_t   free_bytes =
            (size_t)(_sink->text_capacity - _sink->text_used);
        const size_t   length     = d_parse_diag_write(_report->pattern,
                                                       _report->facts,
                                                       _report->fact_count,
                                                       _sink->text + offset,
                                                       free_bytes);

        if (length + 1u > free_bytes)
        {
            _sink->flags |= D_PARSE_DIAG_SINK_TRUNCATED;

            return D_PARSE_DIAG_NO_MESSAGE;
        }

        _sink->text_used = offset + (uint32_t)(length + 1u);

        return offset;
    }

    return d_parse_diag_internal_store_message(_sink, _report->message);
#else
    (void)_report;

    return D_PARSE_DIAG_NO_MESSAGE;
#endif  // D_INTERNAL_PARSE_DIAG_TEXT
}

/*
d_parse_diag_internal_store_facts
  Stores a report's facts in the sink's facts arena, where its diagnostics
carry facts and it has one: the entries, then each name and text with a NUL.
NOTE:
  A record's facts are stored whole or not at all; where they do not fit, the
sink is marked truncated.

Parameter(s):
  _sink:   the sink.
  _report: the report.
  _stored: receives how many were stored.
Return:
  The offset of the first entry, or D_PARSE_DIAG_NO_FACTS.
*/
static
uint32_t d_parse_diag_internal_store_facts(
    struct dj_parse_diag_sink*         _sink,
    const struct dj_parse_diag_report* _report,
    uint16_t*                          _stored
)
{
    *_stored = 0u;

    if ( ((_sink->carry & D_PARSE_DIAG_CARRY_FACTS) == 0u) ||
         (!_sink->facts)                                   ||
         (_sink->facts_capacity == 0u)                     ||
         (!_report->facts)                                 ||
         (_report->fact_count == 0u) )
    {
        return D_PARSE_DIAG_NO_FACTS;
    }

    const uint32_t count = (uint32_t)_report->fact_count;
    size_t         need  = (size_t)count *
                           sizeof(struct dj_parse_diag_fact_entry);
    bool           over  = false;

    // the room: entries, then names and texts, each with its NUL
    for (uint32_t i = 0u; i < count; ++i)
    {
        const struct dj_parse_diag_fact* const f    = &_report->facts[i];
        const char* const                      name = (f->name) ? f->name : "";

        over = (over) || (d_ckd_add(&need, need, strlen(name) + 1u));

        if (f->kind == (uint8_t)D_PARSE_DIAG_FACT_TEXT)
        {
            over = (over) ||
                   (d_ckd_add(&need, need, (size_t)f->size + 1u));
        }
    }

    if ( (over) ||
         (need > (size_t)(_sink->facts_capacity - _sink->facts_used)) )
    {
        _sink->flags |= D_PARSE_DIAG_SINK_TRUNCATED;

        return D_PARSE_DIAG_NO_FACTS;
    }

    const uint32_t base = _sink->facts_used;
    uint32_t       at   = base + count *
                          (uint32_t)sizeof(struct dj_parse_diag_fact_entry);

    for (uint32_t i = 0u; i < count; ++i)
    {
        const struct dj_parse_diag_fact* const f    = &_report->facts[i];
        const char* const                      name = (f->name) ? f->name : "";
        const size_t                           size = strlen(name) + 1u;
        struct dj_parse_diag_fact_entry        entry;

        memset(&entry, 0, sizeof(entry));
        entry.name     = at;
        entry.kind     = f->kind;
        entry.negative = f->negative;
        memcpy(_sink->facts + at, name, size);
        at += (uint32_t)size;

        switch (f->kind)
        {
            case D_PARSE_DIAG_FACT_TEXT:
                entry.value = at;
                entry.extra = f->size;

                if (f->size > 0u)
                {
                    memcpy(_sink->facts + at, f->text, (size_t)f->size);
                }

                _sink->facts[at + f->size] = '\0';
                at += f->size + 1u;
                break;
            case D_PARSE_DIAG_FACT_INTEGER:
                entry.value = f->low;
                entry.extra = f->high;
                break;
            case D_PARSE_DIAG_FACT_FLAG:
                entry.value = (f->low != 0u) ? 1u : 0u;
                break;
            default:
                entry.kind = (uint8_t)D_PARSE_DIAG_FACT_NONE;
                break;
        }

        memcpy(_sink->facts + base +
                   i * (uint32_t)sizeof(struct dj_parse_diag_fact_entry),
               &entry, sizeof(entry));
    }

    _sink->facts_used = at;
    *_stored          = (uint16_t)count;

    return base;
}


/*
d_parse_diag_sink_init
  Initialises a sink over caller-supplied storage, its diagnostics carrying
what the run's settings say (d_parse_diag_settings), with no room for facts
(d_parse_diag_sink_facts gives it some).
NOTE:
  Both storage arguments are optional and independent. A sink with no record
array still tallies every accepted diagnostic, which is the cheapest useful
configuration: a pass/fail counter that allocates nothing.

Parameter(s):
  _sink:          the sink to initialise; ignored if NULL.
  _items:         storage for diagnostic records; may be NULL.
  _item_count: how many records _items holds.
  _text:          storage for message text; may be NULL.
  _text_bytes: how many bytes _text holds.
Return:
  none.
*/
void d_parse_diag_sink_init(
    struct dj_parse_diag_sink*  _sink,
    struct dj_parse_diagnostic* _items,
    uint32_t                    _item_count,
    char*                       _text,
    uint32_t                    _text_bytes
)
{
    if (!_sink)
    {
        return;
    }

    struct dj_parse_diag_settings settings;

    memset(_sink, 0, sizeof(*_sink));
    d_parse_diag_settings(&settings);

    _sink->items         = _items;
    _sink->capacity      = (_items != NULL) ? _item_count : 0u;
    _sink->text          = _text;
    _sink->text_capacity = (_text != NULL) ? _text_bytes : 0u;
    _sink->min_severity  = (uint8_t)D_PARSE_SEVERITY_NOTE;
    _sink->carry         = (uint8_t)( ((settings.text != 0u)
                                       ? D_PARSE_DIAG_CARRY_TEXT  : 0u) |
                                      ((settings.facts != 0u)
                                       ? D_PARSE_DIAG_CARRY_FACTS : 0u) );

    return;
}


#if (D_INTERNAL_PARSE_DIAG_HEAP == 1)

/*
d_parse_diag_sink_init_heap
  Initialises a sink over storage it allocates and owns.

Parameter(s):
  _sink:          the sink to initialise; ignored if NULL.
  _item_count: records to reserve; 0 selects D_PARSE_DIAG_DEFAULT_ITEMS.
  _text_bytes: arena bytes to reserve; 0 selects D_PARSE_DIAG_DEFAULT_TEXT.
Post-condition(s):
  - on success the sink owns its blocks - the records, the text arena and,
    where facts are built, a facts arena of D_PARSE_DIAG_DEFAULT_FACTS bytes
    - and d_parse_diag_sink_release frees them; on failure the sink is left
    valid, empty, and owning nothing.
Return:
  0 on success; -1 if _sink is NULL or an allocation was refused.
*/
int d_parse_diag_sink_init_heap(
    struct dj_parse_diag_sink* _sink,
    uint32_t                   _item_count,
    uint32_t                   _text_bytes
)
{
    if (!_sink)
    {
        return -1;
    }

    d_parse_diag_sink_init(_sink, NULL, 0u, NULL, 0u);

    const uint32_t items = (_item_count > 0u)
                           ? _item_count
                           : (uint32_t)D_PARSE_DIAG_DEFAULT_ITEMS;
    const uint32_t bytes = (_text_bytes > 0u)
                           ? _text_bytes
                           : (uint32_t)D_PARSE_DIAG_DEFAULT_TEXT;

    size_t record_bytes = 0u;

    // refuse a size that does not fit in a size_t
    if (d_ckd_mul(&record_bytes,
                  (size_t)items,
                  sizeof(struct dj_parse_diagnostic)))
    {
        return -1;
    }

    struct dj_parse_diagnostic* records =
        (struct dj_parse_diagnostic*)malloc(record_bytes);

    // check if the record allocation was successful
    if (!records)
    {
        return -1;
    }

    char* arena = (char*)malloc((size_t)bytes);

    // check if the arena allocation was successful
    if (!arena)
    {
        free(records);

        return -1;
    }

    char* facts = NULL;

#if (D_INTERNAL_PARSE_DIAG_FACTS == 1)
    facts = (char*)malloc((size_t)D_PARSE_DIAG_DEFAULT_FACTS);

    // check if the facts arena's allocation was successful
    if (!facts)
    {
        free(arena);
        free(records);

        return -1;
    }
#endif  // D_INTERNAL_PARSE_DIAG_FACTS

    _sink->items          = records;
    _sink->capacity       = items;
    _sink->text           = arena;
    _sink->text_capacity  = bytes;
    _sink->facts          = facts;
    _sink->facts_capacity = (facts != NULL)
                            ? (uint32_t)D_PARSE_DIAG_DEFAULT_FACTS
                            : 0u;
    _sink->flags          = (uint8_t)( D_PARSE_DIAG_SINK_OWNS_ITEMS |
                                       D_PARSE_DIAG_SINK_OWNS_TEXT  |
                                       ( (facts != NULL)
                                         ? D_PARSE_DIAG_SINK_OWNS_FACTS
                                         : 0u ) );

    return 0;
}

#endif  // D_INTERNAL_PARSE_DIAG_HEAP


/*
d_parse_diag_sink_hook
  Installs the callback invoked for each accepted diagnostic.

Parameter(s):
  _sink: the sink to configure; ignored if NULL.
  _hook: the callback; NULL removes any installed hook.
  _ctx:  passed back to the callback unchanged.
Return:
  none.
*/
void d_parse_diag_sink_hook(
    struct dj_parse_diag_sink* _sink,
    dj_parse_diag_hook         _hook,
    void*                      _ctx
)
{
    if (!_sink)
    {
        return;
    }

    _sink->hook     = _hook;
    _sink->hook_ctx = _ctx;

    return;
}


/*
d_parse_diag_sink_facts
  Gives a sink caller-supplied room for its diagnostics' facts, replacing any
it had; a facts arena the sink allocated is freed.
NOTE:
  Call it before the first diagnostic: facts stored in the arena it replaces
are no longer readable. A sink keeps no facts, where its build or its run
carries none, whatever room it has.

Parameter(s):
  _sink:        the sink; ignored if NULL.
  _facts:       the room; NULL leaves the sink with none.
  _facts_bytes: how many bytes it holds.
Return:
  none.
*/
void d_parse_diag_sink_facts(
    struct dj_parse_diag_sink* _sink,
    char*                      _facts,
    uint32_t                   _facts_bytes
)
{
    if (!_sink)
    {
        return;
    }

#if (D_INTERNAL_PARSE_DIAG_HEAP == 1)
    // free only what this sink allocated
    if ( (_sink->facts != NULL) &&
         ((_sink->flags & D_PARSE_DIAG_SINK_OWNS_FACTS) != 0u) )
    {
        free(_sink->facts);
    }
#endif  // D_INTERNAL_PARSE_DIAG_HEAP

    _sink->facts          = _facts;
    _sink->facts_used     = 0u;
    _sink->facts_capacity = (_facts != NULL) ? _facts_bytes : 0u;
    _sink->flags          = (uint8_t)(_sink->flags &
                                      ~D_PARSE_DIAG_SINK_OWNS_FACTS);

    return;
}


/*
d_parse_diag_sink_carry
  Sets what a sink's diagnostics carry, of what the build has.

Parameter(s):
  _sink:  the sink; ignored if NULL.
  _carry: D_PARSE_DIAG_CARRY_* bits.
Return:
  What they will carry: _carry's bits the build has. A bit asked for and not
returned is one the build lacks (D_CFG_PARSE_DIAG_TEXT, _FACTS at 0).
*/
uint8_t d_parse_diag_sink_carry(
    struct dj_parse_diag_sink* _sink,
    uint8_t                    _carry
)
{
    const uint8_t carry = (uint8_t)(_carry & d_internal_parse_diag_built());

    if (_sink)
    {
        _sink->carry = carry;
    }

    return carry;
}


/*
d_parse_diag_sink_stream
  Installs the streaming hook, called once per accepted diagnostic with its
notes, and given what it asks for of what the sink's diagnostics carry.

Parameter(s):
  _sink:   the sink; ignored if NULL.
  _stream: the hook; NULL removes any installed.
  _ctx:    passed back to the hook unchanged.
  _flags:  D_PARSE_DIAG_STREAM_* bits: what it asks to be given.
Return:
  What it will be given: _flags's bits the sink's diagnostics carry. A bit
asked for and not returned is one the build or the run leaves out.
*/
uint8_t d_parse_diag_sink_stream(
    struct dj_parse_diag_sink* _sink,
    dj_parse_diag_stream_fn    _stream,
    void*                      _ctx,
    uint8_t                    _flags
)
{
    if (!_sink)
    {
        return 0u;
    }

    _sink->stream       = _stream;
    _sink->stream_ctx   = (_stream != NULL) ? _ctx : NULL;
    _sink->stream_flags = (_stream != NULL)
                          ? (uint8_t)(_flags & (D_PARSE_DIAG_STREAM_TEXT |
                                                D_PARSE_DIAG_STREAM_FACTS))
                          : 0u;

    return (uint8_t)(_sink->stream_flags & _sink->carry);
}


/*
d_parse_diag_sink_filter
  Sets the lowest severity the sink accepts.
NOTE:
  Filtered diagnostics are neither stored nor tallied, so raising the filter
above ERROR would make d_parse_diag_failed lie; the value is clamped to keep
that impossible.

Parameter(s):
  _sink:         the sink to configure; ignored if NULL.
  _minimum: a dj_parse_severity value, clamped to [NOTE, ERROR].
Return:
  none.
*/
void d_parse_diag_sink_filter(
    struct dj_parse_diag_sink* _sink,
    int                        _minimum
)
{
    if (!_sink)
    {
        return;
    }

    int level = _minimum;

    // clamp below, so a negative value cannot disable the enum ordering
    if (level < (int)D_PARSE_SEVERITY_NOTE)
    {
        level = (int)D_PARSE_SEVERITY_NOTE;
    }

    // clamp above, so errors and fatals are always accepted and always counted
    if (level > (int)D_PARSE_SEVERITY_ERROR)
    {
        level = (int)D_PARSE_SEVERITY_ERROR;
    }

    _sink->min_severity = (uint8_t)level;

    return;
}


/*
d_parse_diag_sink_reset
  Empties a sink for reuse, keeping its storage and its configuration: its
filter, what it carries, and its hooks.

Parameter(s):
  _sink: the sink to empty; ignored if NULL.
Return:
  none.
*/
void d_parse_diag_sink_reset(
    struct dj_parse_diag_sink* _sink
)
{
    if (!_sink)
    {
        return;
    }

    _sink->count      = 0u;
    _sink->text_used  = 0u;
    _sink->facts_used = 0u;
    _sink->dropped    = 0u;
    _sink->flags     = (uint8_t)(_sink->flags & ~D_PARSE_DIAG_SINK_TRUNCATED);

    memset(_sink->tally, 0, sizeof(_sink->tally));

    return;
}


/*
d_parse_diag_sink_release
  Releases any storage the sink owns and leaves it empty.
NOTE:
  Safe on a zeroed sink, on one over caller storage, and on one already
released.

Parameter(s):
  _sink: the sink to release; ignored if NULL.
Return:
  none.
*/
void d_parse_diag_sink_release(
    struct dj_parse_diag_sink* _sink
)
{
    if (!_sink)
    {
        return;
    }

#if (D_INTERNAL_PARSE_DIAG_HEAP == 1)
    // free only what this sink allocated; caller storage is never touched
    if ( (_sink->items != NULL) &&
         ((_sink->flags & D_PARSE_DIAG_SINK_OWNS_ITEMS) != 0u) )
    {
        free(_sink->items);
    }

    if ( (_sink->text != NULL) &&
         ((_sink->flags & D_PARSE_DIAG_SINK_OWNS_TEXT) != 0u) )
    {
        free(_sink->text);
    }

    if ( (_sink->facts != NULL) &&
         ((_sink->flags & D_PARSE_DIAG_SINK_OWNS_FACTS) != 0u) )
    {
        free(_sink->facts);
    }
#endif  // D_INTERNAL_PARSE_DIAG_HEAP

    memset(_sink, 0, sizeof(*_sink));

    return;
}


/*
d_parse_diag_internal_accept
  Accepts one report: tallies it, stores it with its text and facts where
there is room, and hands the record to the per-record hook.

Parameter(s):
  _sink:   the sink.
  _report: the report, its severity checked.
  _note:   whether it continues the one before it.
  _record: receives the record as the sink made it.
Return:
  none.
*/
static void
d_parse_diag_internal_accept(
    struct dj_parse_diag_sink*         _sink,
    const struct dj_parse_diag_report* _report,
    bool                               _note,
    struct dj_parse_diagnostic*        _record
)
{
    uint16_t stored = 0u;

    _sink->tally[_report->severity]++;

    memset(_record, 0, sizeof(*_record));

    _record->severity = _report->severity;
    _record->flags    = (uint8_t)( _report->flags |
                                   ( (_note)
                                     ? D_PARSE_DIAG_FLAG_CONTINUATION
                                     : 0u ) );
    _record->domain   = _report->domain;
    _record->code     = _report->code;
    _record->span     = _report->span;
    _record->message  = D_PARSE_DIAG_NO_MESSAGE;
    _record->facts    = D_PARSE_DIAG_NO_FACTS;

    // store the text and the facts first, so the hooks see a resolvable
    // record
    if (_sink->count < _sink->capacity)
    {
        _record->message    = d_parse_diag_internal_store_text(_sink,
                                                               _report);
        _record->facts      = d_parse_diag_internal_store_facts(_sink,
                                                                _report,
                                                                &stored);
        _record->fact_count = stored;

        _sink->items[_sink->count] = *_record;
        _sink->count++;
    }
    else
    {
        _sink->dropped++;
        _sink->flags |= D_PARSE_DIAG_SINK_TRUNCATED;
    }

    // to the per-record hook whether or not there was room to store it
    if (_sink->hook)
    {
        _sink->hook(_sink->hook_ctx, _sink, _record);
    }

    return;
}

/*
d_parse_diag_internal_event
  Fills a streaming hook's event for one record: its text and its facts as
the hook asked, of what the sink's diagnostics carry.

Parameter(s):
  _sink:    the sink.
  _report:  the report the record was made from.
  _record:  the record, kept for the hook's call.
  _event:   the event to fill.
  _written: the stack's room for text written for the hook.
  _used:    how much of it is used; advanced past what is written here.
Return:
  none.
*/
static void
d_parse_diag_internal_event(
    const struct dj_parse_diag_sink*   _sink,
    const struct dj_parse_diag_report* _report,
    const struct dj_parse_diagnostic*  _record,
    struct dj_parse_diag_event*        _event,
    char*                              _written,
    size_t*                            _used
)
{
    const uint8_t wanted = (uint8_t)(_sink->stream_flags & _sink->carry);

    memset(_event, 0, sizeof(*_event));
    _event->record  = _record;
    _event->pattern = _report->pattern;

    if ((_sink->carry & D_PARSE_DIAG_CARRY_TEXT) != 0u)
    {
        if (!_report->pattern)
        {
            // words of its own are all it says
            _event->text      = _report->message;
            _event->text_size = (_report->message)
                                ? strlen(_report->message)
                                : 0u;
        }
        else if ((wanted & D_PARSE_DIAG_STREAM_TEXT) != 0u)
        {
            if (_record->message != D_PARSE_DIAG_NO_MESSAGE)
            {
                _event->text      = _sink->text + _record->message;
                _event->text_size = strlen(_event->text);
            }
#if (D_INTERNAL_PARSE_DIAG_TEXT == 1)
            else if (*_used < (size_t)D_PARSE_DIAG_MESSAGE_MAX)
            {
                const size_t room   = (size_t)D_PARSE_DIAG_MESSAGE_MAX -
                                      *_used;
                const size_t length =
                    d_parse_diag_write(_report->pattern, _report->facts,
                                       _report->fact_count,
                                       _written + *_used, room);

                _event->text      = _written + *_used;
                _event->text_size = (length < room) ? length : room - 1u;
                *_used           += _event->text_size + 1u;
            }
#else
            (void)_written;
            (void)_used;
#endif  // D_INTERNAL_PARSE_DIAG_TEXT
        }
    }

    if ( ((wanted & D_PARSE_DIAG_STREAM_FACTS) != 0u) &&
         (_report->facts)                             &&
         (_report->fact_count > 0u) )
    {
        _event->facts      = _report->facts;
        _event->fact_count = (uint32_t)_report->fact_count;
    }

    return;
}


/*
d_parse_diag_report
  Accepts a diagnostic and the notes that continue it: tallies each, stores
each where there is room, with its text and its facts as the sink's
diagnostics carry them, hands each to the per-record hook, and then hands
the diagnostic, with its notes, to the streaming hook.
NOTE:
  Acceptance and storage are separate outcomes. A sink that is full still
tallies, so a caller's later d_parse_diag_failed stays correct; the record is
counted in `dropped` and the sink is marked truncated.
  Text is written from a report's pattern only where it is stored, or where
the streaming hook asked for it; for the hook, a text the sink does not store
is written on the stack, in D_PARSE_DIAG_MESSAGE_MAX bytes for a diagnostic
and its notes together.

Parameter(s):
  _sink:    the sink to report into; may be NULL.
  _reports: the diagnostic, then its notes, each flagged a continuation here.
  _count:   how many; 1 for a diagnostic with no notes.
Return:
  0 if the diagnostic was accepted; -1 if it was rejected, with its notes,
because the sink was NULL, the reports were absent, or its severity was out
of range or below the sink's filter.
*/
int d_parse_diag_report(
    struct dj_parse_diag_sink*         _sink,
    const struct dj_parse_diag_report* _reports,
    uint32_t                           _count
)
{
    struct dj_parse_diagnostic records[1u + D_PARSE_DIAG_NOTES_MAX];
    struct dj_parse_diag_event events[1u + D_PARSE_DIAG_NOTES_MAX];
    char                       written[D_PARSE_DIAG_MESSAGE_MAX];
    size_t                     used     = 0u;
    uint32_t                   streamed = 0u;

    // reject a missing sink, no reports, or a severity outside the enum
    if ( (!_sink)                                                  ||
         (!_reports)                                               ||
         (_count == 0u)                                            ||
         (_reports[0].severity >= (uint8_t)D_PARSE_SEVERITY_COUNT) )
    {
        return -1;
    }

    // apply the sink's filter before anything is counted
    if (_reports[0].severity < _sink->min_severity)
    {
        return -1;
    }

    for (uint32_t i = 0u; i < _count; ++i)
    {
        struct dj_parse_diagnostic record;

        // a note passes the same filter its diagnostic did
        if ( (_reports[i].severity >= (uint8_t)D_PARSE_SEVERITY_COUNT) ||
             (_reports[i].severity < _sink->min_severity) )
        {
            continue;
        }

        d_parse_diag_internal_accept(_sink, &_reports[i], (i > 0u),
                                     &record);

        // kept for the streaming hook, with what it asked for
        if ( (_sink->stream) &&
             (streamed < 1u + D_PARSE_DIAG_NOTES_MAX) )
        {
            records[streamed] = record;
            d_parse_diag_internal_event(_sink, &_reports[i],
                                        &records[streamed],
                                        &events[streamed], written, &used);
            ++streamed;
        }
    }

    // the diagnostic, with its notes, after every per-record call
    if ( (_sink->stream) &&
         (streamed > 0u) )
    {
        events[0].note_count = streamed - 1u;
        events[0].notes      = (streamed > 1u) ? &events[1] : NULL;

        _sink->stream(_sink->stream_ctx, _sink, &events[0]);
    }

    return 0;
}


/*
d_parse_diag_emit_flagged
  Accepts a diagnostic with words of its own and no facts, as
d_parse_diag_report does.

Parameter(s):
  _sink:     the sink to emit into; may be NULL.
  _severity: a dj_parse_severity value.
  _domain:   the emitting stage's domain.
  _code:     the condition, in that domain's private code space.
  _flags:    D_PARSE_DIAG_FLAG_* bits.
  _span:     where in the source the condition was found.
  _message:  human-readable text; may be NULL, and is copied if stored.
Return:
  0 if the diagnostic was accepted; -1 if it was rejected because the sink was
NULL or the severity was below the sink's filter.
*/
int d_parse_diag_emit_flagged(
    struct dj_parse_diag_sink* _sink,
    int                        _severity,
    uint16_t                   _domain,
    uint16_t                   _code,
    uint8_t                    _flags,
    struct dj_parse_span       _span,
    const char*                _message
)
{
    struct dj_parse_diag_report report;

    // reject a severity outside the enum before it is narrowed
    if ( (_severity < 0) ||
         (_severity >= (int)D_PARSE_SEVERITY_COUNT) )
    {
        return -1;
    }

    memset(&report, 0, sizeof(report));

    report.severity = (uint8_t)_severity;
    report.flags    = _flags;
    report.domain   = _domain;
    report.code     = _code;
    report.span     = _span;
    report.message  = _message;

    return d_parse_diag_report(_sink, &report, 1u);
}


/*
d_parse_diag_emit
  Accepts a diagnostic with no flags set.

Parameter(s):
  _sink:     the sink to emit into; may be NULL.
  _severity: a dj_parse_severity value.
  _domain:   the emitting stage's domain.
  _code:     the condition, in that domain's private code space.
  _span:     where in the source the condition was found.
  _message:  human-readable text; may be NULL.
Return:
  0 if the diagnostic was accepted; -1 otherwise.
*/
int d_parse_diag_emit(
    struct dj_parse_diag_sink* _sink,
    int                        _severity,
    uint16_t                   _domain,
    uint16_t                   _code,
    struct dj_parse_span       _span,
    const char*                _message
)
{
    return d_parse_diag_emit_flagged(_sink,
                                     _severity,
                                     _domain,
                                     _code,
                                     0u,
                                     _span,
                                     _message);
}


#if (D_INTERNAL_PARSE_DIAG_FORMAT == 1)

/*
d_parse_diag_emitf
  Accepts a diagnostic whose message is formatted printf-style.
NOTE:
  Formatting happens on the stack into a D_PARSE_DIAG_MESSAGE_MAX buffer; a
longer message is truncated there, which is the one place truncation is
preferred to loss because the caller asked for interpolation.

Parameter(s):
  _sink:     the sink to emit into; may be NULL.
  _severity: a dj_parse_severity value.
  _domain:   the emitting stage's domain.
  _code:     the condition, in that domain's private code space.
  _span:     where in the source the condition was found.
  _format:   a printf format string; may be NULL, which emits no message.
Return:
  0 if the diagnostic was accepted; -1 otherwise.
*/
int d_parse_diag_emitf(
    struct dj_parse_diag_sink* _sink,
    int                        _severity,
    uint16_t                   _domain,
    uint16_t                   _code,
    struct dj_parse_span       _span,
    const char*                _format,
    ...
)
{
    // a formatted call with no format is just an unformatted one, and so is
    // any where no text is carried
    if ( (!_format) ||
         (D_INTERNAL_PARSE_DIAG_TEXT == 0) )
    {
        return d_parse_diag_emit(_sink,
                                 _severity,
                                 _domain,
                                 _code,
                                 _span,
                                 NULL);
    }

    char buffer[D_PARSE_DIAG_MESSAGE_MAX];

    va_list args;

    va_start(args, _format);
    (void)vsnprintf(buffer, sizeof(buffer), _format, args);
    va_end(args);

    buffer[sizeof(buffer) - 1u] = '\0';

    return d_parse_diag_emit(_sink,
                             _severity,
                             _domain,
                             _code,
                             _span,
                             buffer);
}

#endif  // D_INTERNAL_PARSE_DIAG_FORMAT


/*
d_parse_diag_find
  The first stored diagnostic reporting a given condition.
NOTE:
  A condition is its domain AND its code: codes are numbered from zero within
each domain, so the same code in two domains is two different conditions, and
matching on the code alone would find the wrong one. Only stored records are
searched; a diagnostic dropped by a full sink is still tallied but cannot be
found, which is the same distinction d_parse_diag_truncated reports.

Parameter(s):
  _sink:   the sink to search; may be NULL.
  _domain: the emitting stage's domain.
  _code:   the condition, in that domain's code space.
  _from:   the index to start searching at, so a caller can find every match.
Return:
  The index of the first match at or after _from, or -1 when there is none.
*/
int32_t d_parse_diag_find(
    const struct dj_parse_diag_sink* _sink,
    uint16_t                         _domain,
    uint16_t                         _code,
    uint32_t                         _from
)
{
    if ( (!_sink)        ||
         (!_sink->items) )
    {
        return -1;
    }

    // compare the pair, never the code alone
    for (uint32_t index = _from; index < _sink->count; index++)
    {
        if ( (_sink->items[index].domain == _domain) &&
             (_sink->items[index].code == _code)     )
        {
            return (int32_t)index;
        }
    }

    return -1;
}


/*
d_parse_diag_fact_at
  A stored diagnostic's fact, by its place among the record's facts.

Parameter(s):
  _sink:  the sink the diagnostic was stored in; may be NULL.
  _diag:  the diagnostic; may be NULL.
  _index: the fact's place, from 0.
  _out:   receives the fact, its name and text pointing into the sink's facts
          arena; may be NULL to ask only whether there is one.
Return:
  1 when the record has a fact there; 0 otherwise, _out then untouched.
*/
int d_parse_diag_fact_at(
    const struct dj_parse_diag_sink*  _sink,
    const struct dj_parse_diagnostic* _diag,
    uint32_t                          _index,
    struct dj_parse_diag_fact*        _out
)
{
    const size_t                    stride =
        sizeof(struct dj_parse_diag_fact_entry);
    struct dj_parse_diag_fact_entry entry;
    size_t                          end    = 0u;

    // reject what the record does not hold, or an offset past the arena's use
    if ( (!_sink)                                                       ||
         (!_diag)                                                       ||
         (!_sink->facts)                                                ||
         (_diag->facts == D_PARSE_DIAG_NO_FACTS)                        ||
         (_index >= (uint32_t)_diag->fact_count)                        ||
         (d_ckd_add(&end, (size_t)_diag->facts,
                    ((size_t)_index + 1u) * stride))                    ||
         (end > (size_t)_sink->facts_used) )
    {
        return 0;
    }

    memcpy(&entry, _sink->facts + _diag->facts + (size_t)_index * stride,
           stride);

    if ( (entry.name >= _sink->facts_used) ||
         ( (entry.kind == (uint8_t)D_PARSE_DIAG_FACT_TEXT) &&
           ( (entry.value > _sink->facts_used) ||
             (entry.extra > _sink->facts_used - entry.value) ) ) )
    {
        return 0;
    }

    if (_out)
    {
        memset(_out, 0, sizeof(*_out));
        _out->name     = _sink->facts + entry.name;
        _out->kind     = entry.kind;
        _out->negative = entry.negative;

        switch (entry.kind)
        {
            case D_PARSE_DIAG_FACT_TEXT:
                _out->text = _sink->facts + entry.value;
                _out->size = entry.extra;
                break;
            case D_PARSE_DIAG_FACT_INTEGER:
                _out->low  = entry.value;
                _out->high = entry.extra;
                break;
            case D_PARSE_DIAG_FACT_FLAG:
                _out->low  = entry.value;
                break;
            default:
                break;
        }
    }

    return 1;
}


/*
d_parse_diag_fact_find
  A stored diagnostic's fact, by its name.

Parameter(s):
  _sink: the sink the diagnostic was stored in; may be NULL.
  _diag: the diagnostic; may be NULL.
  _name: the fact's name; may be NULL, which finds none.
  _out:  receives the fact, as d_parse_diag_fact_at gives it; may be NULL.
Return:
  The fact's place among the record's facts, or -1 where it has none of the
name.
*/
int32_t d_parse_diag_fact_find(
    const struct dj_parse_diag_sink*  _sink,
    const struct dj_parse_diagnostic* _diag,
    const char*                       _name,
    struct dj_parse_diag_fact*        _out
)
{
    struct dj_parse_diag_fact fact;

    if ( (!_name) ||
         (!_diag) )
    {
        return -1;
    }

    for (uint32_t i = 0u; i < (uint32_t)_diag->fact_count; ++i)
    {
        if ( (d_parse_diag_fact_at(_sink, _diag, i, &fact) == 1) &&
             (strcmp(fact.name, _name) == 0) )
        {
            if (_out)
            {
                *_out = fact;
            }

            return (int32_t)i;
        }
    }

    return -1;
}


//==============================================================================
// 4.  TEMPLATES
//==============================================================================

#if (D_INTERNAL_PARSE_DIAG_TEXT == 1)

// dj_internal_parse_diag_out
//   struct: a buffer written as snprintf writes one: what does not fit is
// counted, and the text always ends in a NUL where there is room for one.
struct dj_internal_parse_diag_out
{
    char*  out;
    size_t size;
    size_t length;
};

/*
d_internal_parse_diag_put
  Writes bytes to a buffer, counting all of them.
*/
static void
d_internal_parse_diag_put(
    struct dj_internal_parse_diag_out* _o,
    const char*                        _bytes,
    size_t                             _count
)
{
    if ( (_o->size > 0u) &&
         (_o->length < _o->size - 1u) )
    {
        const size_t room = _o->size - 1u - _o->length;
        const size_t n    = (_count < room) ? _count : room;

        if (n > 0u)
        {
            memcpy(_o->out + _o->length, _bytes, n);
        }
    }

    _o->length += _count;

    return;
}

/*
d_internal_parse_diag_put_value
  Writes a fact's value as a template's field does: text as its bytes, an
integer in decimal, a flag as true or false, and nothing for none.
*/
static void
d_internal_parse_diag_put_value(
    struct dj_internal_parse_diag_out* _o,
    const struct dj_parse_diag_fact*   _fact
)
{
    if (!_fact)
    {
        return;
    }

    switch (_fact->kind)
    {
        case D_PARSE_DIAG_FACT_TEXT:
            if (_fact->text)
            {
                d_internal_parse_diag_put(_o, _fact->text,
                                          (size_t)_fact->size);
            }
            break;
        case D_PARSE_DIAG_FACT_INTEGER:
        {
            // four 16-bit limbs, most significant first, divided by ten
            // until none is left: no 64-bit type needed
            uint32_t limb[4];
            char     digits[24];
            size_t   n = 0u;

            limb[0] = _fact->high >> 16;
            limb[1] = _fact->high & 0xFFFFu;
            limb[2] = _fact->low >> 16;
            limb[3] = _fact->low & 0xFFFFu;

            do
            {
                uint32_t rest = 0u;

                for (size_t k = 0u; k < 4u; ++k)
                {
                    const uint32_t part = (rest << 16) | limb[k];

                    limb[k] = part / 10u;
                    rest    = part % 10u;
                }

                digits[n++] = (char)('0' + (int)rest);
            }
            while ((limb[0] | limb[1] | limb[2] | limb[3]) != 0u);

            if ( (_fact->negative != 0u) &&
                 ((_fact->low | _fact->high) != 0u) )
            {
                d_internal_parse_diag_put(_o, "-", 1u);
            }

            while (n > 0u)
            {
                --n;
                d_internal_parse_diag_put(_o, &digits[n], 1u);
            }

            break;
        }
        case D_PARSE_DIAG_FACT_FLAG:
            if (_fact->low != 0u)
            {
                d_internal_parse_diag_put(_o, "true", 4u);
            }
            else
            {
                d_internal_parse_diag_put(_o, "false", 5u);
            }
            break;
        default:
            break;
    }

    return;
}

// dj_internal_parse_diag_blocks
//   struct: a template's open blocks as it is written: for each, whether
// what encloses it is shown, whether its fact holds as its test asks, and
// whether its other branch has begun.
struct dj_internal_parse_diag_blocks
{
    bool     shown[D_INTERNAL_PARSE_DIAG_DEPTH + 1u];
    bool     holds[D_INTERNAL_PARSE_DIAG_DEPTH + 1u];
    bool     other[D_INTERNAL_PARSE_DIAG_DEPTH + 1u];
    uint32_t depth;
};

/*
d_internal_parse_diag_visible
  Whether text at the blocks' depth is shown: everything enclosing it is,
and it is in the branch its block's test chose.
*/
static bool
d_internal_parse_diag_visible(
    const struct dj_internal_parse_diag_blocks* _b
)
{
    return ( (_b->shown[_b->depth]) &&
             (_b->holds[_b->depth] != _b->other[_b->depth]) );
}

/*
d_internal_parse_diag_construct
  Writes one brace construct of a template, or opens, turns or closes a
block by it.
*/
static void
d_internal_parse_diag_construct(
    struct dj_internal_parse_diag_out*       _o,
    struct dj_internal_parse_diag_blocks*    _b,
    const char*                              _pattern,
    size_t                                   _at,
    const struct dj_internal_parse_diag_tag* _tag,
    const struct dj_parse_diag_fact*         _facts,
    uint32_t                                 _fact_count
)
{
    const bool                       visible =
        d_internal_parse_diag_visible(_b);
    const struct dj_parse_diag_fact* fact    =
        (_tag->name)
        ? d_internal_parse_diag_lookup(_facts, _fact_count, _tag->name,
                                       _tag->length)
        : NULL;

    switch (_tag->kind)
    {
        case '{':
        case '}':
            if (visible)
            {
                d_internal_parse_diag_put(_o, (_tag->kind == '{') ? "{" : "}",
                                          1u);
            }
            break;
        case '=':
            if (visible)
            {
                d_internal_parse_diag_put_value(_o, fact);
            }
            break;
        case '?':
        case '^':
            if (_b->depth < D_INTERNAL_PARSE_DIAG_DEPTH)
            {
                const bool test = d_internal_parse_diag_value_true(fact);

                ++_b->depth;
                _b->shown[_b->depth] = visible;
                _b->holds[_b->depth] = (_tag->kind == '?') ? test : !test;
                _b->other[_b->depth] = false;
            }
            else if (visible)
            {
                d_internal_parse_diag_put(_o, _pattern + _at,
                                          _tag->end - _at);
            }
            break;
        case ':':
            if (_b->depth > 0u)
            {
                _b->other[_b->depth] = true;
            }
            break;
        case '/':
            if (_b->depth > 0u)
            {
                --_b->depth;
            }
            break;
        default:
            // an unclosed brace is text
            if (visible)
            {
                d_internal_parse_diag_put(_o, _pattern + _at, 1u);
            }
            break;
    }

    return;
}

/*
d_parse_diag_write
  Writes a template, given its facts, into a buffer.
NOTE:
  The template language's text and blocks, as text/ostream_template.h has
them: text is copied; {name} writes the fact's value; {?name}...{/} is shown
where the fact holds and {^name}...{/} where it does not, {:} starting the
other branch, and a block may be closed as {/name}; {{ and }} are braces. A
field's specification or conversion ({name:spec}, {name!conv}) is not
parse's: the field is written as {name} is, and d_parse_diag_template_check
refuses it. A name with no fact given writes nothing and does not hold.
Blocks nest D_INTERNAL_PARSE_DIAG_DEPTH deep; deeper ones are written as
text.

Parameter(s):
  _pattern:    the template; NULL writes nothing.
  _facts:      its facts; may be NULL with 0.
  _fact_count: how many.
  _out:        the buffer; may be NULL when _size is 0.
  _size:       its size in bytes, including the terminator.
Return:
  The length the whole text would have, excluding the terminator, as
snprintf reports it - so a return of _size or more means it was cut.
*/
size_t d_parse_diag_write(
    const char*                      _pattern,
    const struct dj_parse_diag_fact* _facts,
    uint32_t                         _fact_count,
    char*                            _out,
    size_t                           _size
)
{
    struct dj_internal_parse_diag_out    o;
    struct dj_internal_parse_diag_blocks blocks;
    size_t                               at = 0u;

    o.out    = _out;
    o.size   = _size;
    o.length = 0u;

    memset(&blocks, 0, sizeof(blocks));
    blocks.shown[0] = true;
    blocks.holds[0] = true;

    while ( (_pattern) &&
            (_pattern[at] != '\0') )
    {
        const size_t run = strcspn(_pattern + at, "{}");

        // text up to the next brace
        if (run > 0u)
        {
            if (d_internal_parse_diag_visible(&blocks))
            {
                d_internal_parse_diag_put(&o, _pattern + at, run);
            }

            at += run;

            continue;
        }

        {
            const struct dj_internal_parse_diag_tag tag =
                d_internal_parse_diag_tag_at(_pattern, at);

            d_internal_parse_diag_construct(&o, &blocks, _pattern, at, &tag,
                                            _facts, _fact_count);
            at = tag.end;
        }
    }

    if (_size > 0u)
    {
        _out[(o.length < _size) ? o.length : _size - 1u] = '\0';
    }

    return o.length;
}

#endif  // D_INTERNAL_PARSE_DIAG_TEXT


/*
d_parse_diag_template_check
  Whether a template is one parse writes as the log does: each construct one
d_parse_diag_write has, each name among its facts, its blocks closed in
order, each {/name} closing its own, one {:} a block, and none nested past
D_INTERNAL_PARSE_DIAG_DEPTH.

Parameter(s):
  _pattern: the template; NULL is refused at 0.
  _facts:   its facts, as a code table lists them: "(token, open)"; NULL
            or "()" for none.
Return:
  -1 when it is; otherwise the byte offset of the first construct that is
not, or of the end where a block is never closed.
*/
int32_t d_parse_diag_template_check(
    const char* _pattern,
    const char* _facts
)
{
    const char* names[D_INTERNAL_PARSE_DIAG_DEPTH + 1u];
    size_t      lengths[D_INTERNAL_PARSE_DIAG_DEPTH + 1u];
    bool        other[D_INTERNAL_PARSE_DIAG_DEPTH + 1u];
    uint32_t    depth = 0u;
    size_t      at    = 0u;

    if (!_pattern)
    {
        return 0;
    }

    while (_pattern[at] != '\0')
    {
        const size_t run = strcspn(_pattern + at, "{}");

        if (run > 0u)
        {
            at += run;

            continue;
        }

        {
            const struct dj_internal_parse_diag_tag tag =
                d_internal_parse_diag_tag_at(_pattern, at);
            bool                                    ok  = tag.plain;

            switch (tag.kind)
            {
                case '{':
                    break;
                case '}':
                    // a lone close is no construct
                    ok = (_pattern[at + 1u] == '}');
                    break;
                case '=':
                    ok = (ok) &&
                         (tag.length > 0u) &&
                         (d_internal_parse_diag_listed(_facts, tag.name,
                                                       tag.length));
                    break;
                case '?':
                case '^':
                    ok = (ok) &&
                         (depth < D_INTERNAL_PARSE_DIAG_DEPTH) &&
                         (tag.length > 0u) &&
                         (d_internal_parse_diag_listed(_facts, tag.name,
                                                       tag.length));

                    if (ok)
                    {
                        ++depth;
                        names[depth]   = tag.name;
                        lengths[depth] = tag.length;
                        other[depth]   = false;
                    }
                    break;
                case ':':
                    ok = (ok) &&
                         (tag.length == 0u) &&
                         (depth > 0u) &&
                         (!other[depth]);

                    if (ok)
                    {
                        other[depth] = true;
                    }
                    break;
                case '/':
                    ok = (ok) &&
                         (depth > 0u) &&
                         ( (tag.length == 0u) ||
                           ( (tag.length == lengths[depth]) &&
                             (strncmp(tag.name, names[depth],
                                      tag.length) == 0) ) );

                    if (ok)
                    {
                        --depth;
                    }
                    break;
                default:
                    ok = false;
                    break;
            }

            if (!ok)
            {
                return (int32_t)at;
            }

            at = tag.end;
        }
    }

    return (depth == 0u) ? -1 : (int32_t)at;
}


//==============================================================================
// 5.  SETTINGS
//==============================================================================

/*
d_parse_diag_settings_from
  Installs the reader of the run's switches, as d_parse_register does.

Parameter(s):
  _reader: the reader; NULL puts the switches back on their knobs.
Return:
  none.
*/
void d_parse_diag_settings_from(
    dj_parse_diag_settings_fn _reader
)
{
    d_internal_parse_diag_reader = _reader;

    return;
}


/*
d_parse_diag_settings
  The run's switches for parse's diagnostics: the knobs', then the reader's
where one is installed, of what the build has.

Parameter(s):
  _settings: receives them; ignored if NULL.
Return:
  none.
*/
void d_parse_diag_settings(
    struct dj_parse_diag_settings* _settings
)
{
    if (!_settings)
    {
        return;
    }

    _settings->text     = (uint8_t)D_INTERNAL_PARSE_DIAG_TEXT;
    _settings->facts    = (uint8_t)D_INTERNAL_PARSE_DIAG_FACTS;
    _settings->log      = (uint8_t)D_INTERNAL_PARSE_LOG;
    _settings->reserved = 0u;

    if (d_internal_parse_diag_reader)
    {
        d_internal_parse_diag_reader(_settings);
    }

    // a switch reads on only where the build has its facility
    _settings->text  = (uint8_t)( (_settings->text != 0u) &&
                                  (D_INTERNAL_PARSE_DIAG_TEXT == 1) );
    _settings->facts = (uint8_t)( (_settings->facts != 0u) &&
                                  (D_INTERNAL_PARSE_DIAG_FACTS == 1) );
    _settings->log   = (uint8_t)( (_settings->log != 0u) &&
                                  (D_INTERNAL_PARSE_LOG == 1) );

    return;
}


//==============================================================================
// 6.  RENDERING
//==============================================================================

/*
d_parse_severity_name
  The lowercase display name of a severity.

Parameter(s):
  _severity: a dj_parse_severity value.
Return:
  A static string, or "?" when the value is out of range. Never NULL.
*/
const char* d_parse_severity_name(
    int _severity
)
{
    static const char* const names[D_PARSE_SEVERITY_COUNT] =
    {
        "note", "warning", "error", "fatal"
    };

    if ( (_severity < 0) ||
         (_severity >= (int)D_PARSE_SEVERITY_COUNT) )
    {
        return "?";
    }

    return names[_severity];
}


/*
d_parse_diag_format
  Renders one diagnostic as a single line, without a trailing newline.
NOTE:
  The form is `line:column: severity: message [domain/code]`, with the position
omitted when the span carries none. A caller wanting another layout reads the
record's fields directly; this is the default, not the only one.

Parameter(s):
  _sink: the sink holding the diagnostic's message text; may be NULL.
  _diag: the diagnostic to render; may be NULL.
  _out:  the buffer to write into; may be NULL when _size is 0.
  _size: the size of _out in bytes, including the terminator.
Return:
  The number of characters the full line would occupy, excluding the
terminator, as snprintf reports it - so a return of _size or more means the
output was truncated.
*/
size_t d_parse_diag_format(
    const struct dj_parse_diag_sink*  _sink,
    const struct dj_parse_diagnostic* _diag,
    char*                             _out,
    size_t                            _size
)
{
    if (!_diag)
    {
        return 0u;
    }

    const char* const severity = d_parse_severity_name((int)_diag->severity);
    const char* const message  = d_parse_diag_message(_sink, _diag);

    int written = 0;

    // a span with a resolved line prints a position; one without does not
    if (_diag->span.line > 0u)
    {
        written = snprintf(_out,
                           _size,
                           "%u:%u: %s: %s [%u/%u]",
                           (unsigned)_diag->span.line,
                           (unsigned)_diag->span.column,
                           severity,
                           message,
                           (unsigned)_diag->domain,
                           (unsigned)_diag->code);
    }
    else
    {
        written = snprintf(_out,
                           _size,
                           "%s: %s [%u/%u]",
                           severity,
                           message,
                           (unsigned)_diag->domain,
                           (unsigned)_diag->code);
    }

    return (written < 0) ? 0u : (size_t)written;
}


/*
d_parse_diag_print
  Writes every stored diagnostic to stdout, one per line, followed by a note
if storage was exhausted. Diagnostics and build checks only.

Parameter(s):
  _sink: the sink to print; ignored if NULL.
Return:
  none.
*/
void d_parse_diag_print(
    const struct dj_parse_diag_sink* _sink
)
{
    if (!_sink)
    {
        return;
    }

    char line[D_PARSE_DIAG_MESSAGE_MAX];

    // render every record the sink managed to store
    for (uint32_t index = 0u; index < _sink->count; index++)
    {
        const struct dj_parse_diagnostic* const diag =
            d_parse_diag_at(_sink, index);

        (void)d_parse_diag_format(_sink, diag, line, sizeof(line));

        printf("%s%s\n",
               ((diag->flags & D_PARSE_DIAG_FLAG_CONTINUATION) != 0u)
               ? "  "
               : "",
               line);
    }

    // say so when the report is incomplete, rather than letting it read as one
    if (_sink->dropped > 0u)
    {
        printf("... and %u more not stored\n", (unsigned)_sink->dropped);
    }

    return;
}
