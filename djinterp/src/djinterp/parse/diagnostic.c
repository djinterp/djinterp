/******************************************************************************
* djinterp [parse]                                                diagnostic.c
*
*   Definitions for the non-inline declarations in diagnostic.h.
*
*
* path:      /src/djinterp/parse/diagnostic.c
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/
#include "../../../inc/djinterp/parse/diagnostic.h"  // corresponding header
// std
#include <stdio.h>    // printf, snprintf, vsnprintf
#include <string.h>   // memset, memcpy, strlen
#if (D_INTERNAL_PARSE_DIAG_FORMAT == 1)
#include <stdarg.h>   // va_list, va_start, va_end
#endif  // D_INTERNAL_PARSE_DIAG_FORMAT
#if (D_INTERNAL_PARSE_HEAP == 1)
#include <stdlib.h>   // malloc, free
#endif  // D_INTERNAL_PARSE_HEAP


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
static uint32_t
d_parse_diag_internal_store_message(
    struct d_parse_diag_sink* _sink,
    const char*               _message
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


/*
d_parse_diag_sink_init
  Initialises a sink over caller-supplied storage.
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
void
d_parse_diag_sink_init(
    struct d_parse_diag_sink*  _sink,
    struct d_parse_diagnostic* _items,
    uint32_t                   _item_count,
    char*                      _text,
    uint32_t                   _text_bytes
)
{
    if (!_sink)
    {
        return;
    }

    memset(_sink, 0, sizeof(*_sink));

    _sink->items         = _items;
    _sink->capacity      = (_items != NULL) ? _item_count : 0u;
    _sink->text          = _text;
    _sink->text_capacity = (_text != NULL) ? _text_bytes : 0u;
    _sink->min_severity  = (uint8_t)D_PARSE_SEVERITY_NOTE;

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
  - on success the sink owns both blocks and d_parse_diag_sink_release frees
    them; on failure the sink is left valid, empty, and owning nothing.
Return:
  0 on success; -1 if _sink is NULL or an allocation was refused.
*/
int
d_parse_diag_sink_init_heap(
    struct d_parse_diag_sink* _sink,
    uint32_t                  _item_count,
    uint32_t                  _text_bytes
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

    struct d_parse_diagnostic* records =
        (struct d_parse_diagnostic*)malloc((size_t)items * sizeof(*records));

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

    _sink->items         = records;
    _sink->capacity      = items;
    _sink->text          = arena;
    _sink->text_capacity = bytes;
    _sink->flags         = (uint8_t)(D_PARSE_DIAG_SINK_OWNS_ITEMS |
                                     D_PARSE_DIAG_SINK_OWNS_TEXT);

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
void
d_parse_diag_sink_hook(
    struct d_parse_diag_sink* _sink,
    d_parse_diag_hook         _hook,
    void*                     _ctx
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
d_parse_diag_sink_filter
  Sets the lowest severity the sink accepts.
NOTE:
  Filtered diagnostics are neither stored nor tallied, so raising the filter
above ERROR would make d_parse_diag_failed lie; the value is clamped to keep
that impossible.

Parameter(s):
  _sink:         the sink to configure; ignored if NULL.
  _minimum: a d_parse_severity value, clamped to [NOTE, ERROR].
Return:
  none.
*/
void
d_parse_diag_sink_filter(
    struct d_parse_diag_sink* _sink,
    int                       _minimum
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
  Empties a sink for reuse, keeping its storage and its configuration.

Parameter(s):
  _sink: the sink to empty; ignored if NULL.
Return:
  none.
*/
void
d_parse_diag_sink_reset(
    struct d_parse_diag_sink* _sink
)
{
    if (!_sink)
    {
        return;
    }

    _sink->count     = 0u;
    _sink->text_used = 0u;
    _sink->dropped   = 0u;
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
void
d_parse_diag_sink_release(
    struct d_parse_diag_sink* _sink
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
#endif  // D_INTERNAL_PARSE_DIAG_HEAP

    memset(_sink, 0, sizeof(*_sink));

    return;
}


/*
d_parse_diag_emit_flagged
  Accepts a diagnostic: tallies it, hands it to the hook, and stores it if
there is room.
NOTE:
  Acceptance and storage are separate outcomes. A sink that is full still
tallies, so a caller's later d_parse_diag_failed stays correct; the record is
counted in `dropped` and the sink is marked truncated.

Parameter(s):
  _sink:     the sink to emit into; may be NULL.
  _severity: a d_parse_severity value.
  _domain:   the emitting stage's domain.
  _code:     the condition, in that domain's private code space.
  _flags:    D_PARSE_DIAG_FLAG_* bits.
  _span:     where in the source the condition was found.
  _message:  human-readable text; may be NULL, and is copied if stored.
Return:
  0 if the diagnostic was accepted; -1 if it was rejected because the sink was
NULL or the severity was below the sink's filter.
*/
int
d_parse_diag_emit_flagged(
    struct d_parse_diag_sink* _sink,
    int                       _severity,
    uint16_t                  _domain,
    uint16_t                  _code,
    uint8_t                   _flags,
    struct d_parse_span       _span,
    const char*               _message
)
{
    // reject a missing sink or a severity outside the enum outright
    if ( (!_sink)            ||
         (_severity < 0)     ||
         (_severity >= (int)D_PARSE_SEVERITY_COUNT) )
    {
        return -1;
    }

    // apply the sink's filter before anything is counted
    if (_severity < (int)_sink->min_severity)
    {
        return -1;
    }

    _sink->tally[_severity]++;

    struct d_parse_diagnostic record;

    memset(&record, 0, sizeof(record));

    record.severity = (uint8_t)_severity;
    record.flags    = _flags;
    record.domain   = _domain;
    record.code     = _code;
    record.span     = _span;
    record.message  = D_PARSE_DIAG_NO_MESSAGE;

    // store the text first, so the hook sees a resolvable record
    if (_sink->count < _sink->capacity)
    {
        record.message = d_parse_diag_internal_store_message(_sink, _message);

        _sink->items[_sink->count] = record;
        _sink->count++;
    }
    else
    {
        _sink->dropped++;
        _sink->flags |= D_PARSE_DIAG_SINK_TRUNCATED;
    }

    // stream to the hook whether or not there was room to store the record
    if (_sink->hook)
    {
        _sink->hook(_sink->hook_ctx, _sink, &record);
    }

    return 0;
}


/*
d_parse_diag_emit
  Accepts a diagnostic with no flags set.

Parameter(s):
  _sink:     the sink to emit into; may be NULL.
  _severity: a d_parse_severity value.
  _domain:   the emitting stage's domain.
  _code:     the condition, in that domain's private code space.
  _span:     where in the source the condition was found.
  _message:  human-readable text; may be NULL.
Return:
  0 if the diagnostic was accepted; -1 otherwise.
*/
int
d_parse_diag_emit(
    struct d_parse_diag_sink* _sink,
    int                       _severity,
    uint16_t                  _domain,
    uint16_t                  _code,
    struct d_parse_span       _span,
    const char*               _message
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
  _severity: a d_parse_severity value.
  _domain:   the emitting stage's domain.
  _code:     the condition, in that domain's private code space.
  _span:     where in the source the condition was found.
  _format:   a printf format string; may be NULL, which emits no message.
Return:
  0 if the diagnostic was accepted; -1 otherwise.
*/
int
d_parse_diag_emitf(
    struct d_parse_diag_sink* _sink,
    int                       _severity,
    uint16_t                  _domain,
    uint16_t                  _code,
    struct d_parse_span       _span,
    const char*               _format,
    ...
)
{
    // a formatted call with no format is just an unformatted one
    if (!_format)
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
d_parse_severity_name
  The lowercase display name of a severity.

Parameter(s):
  _severity: a d_parse_severity value.
Return:
  A static string, or "?" when the value is out of range. Never NULL.
*/
const char*
d_parse_severity_name(
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
terminator, as snprintf reports it -- so a return of _size or more means the
output was truncated.
*/
size_t
d_parse_diag_format(
    const struct d_parse_diag_sink*  _sink,
    const struct d_parse_diagnostic* _diag,
    char*                            _out,
    size_t                           _size
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
void
d_parse_diag_print(
    const struct d_parse_diag_sink* _sink
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
        const struct d_parse_diagnostic* const diag =
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
