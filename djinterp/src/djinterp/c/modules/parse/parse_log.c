/*******************************************************************************
* djinterp [c][parse]                                                parse_log.c
*
* Parse's two tables of codes, defined, their registration, and the
* subscriber that writes parse's diagnostics as the log's records.
*
*
* path:      /src/djinterp/c/modules/parse/parse_log.c
* link(s):   /docs/djinterp/log/log_design.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
#include "../../../../../inc/djinterp/c/modules/parse/parse_log.h"  // its header


#if ( (D_INTERNAL_PARSE_LOG == 1) && RE_STD_HAS_INT64 )

// std
#include <stddef.h>  // NULL


//==============================================================================
// 1.  THE TABLES
//==============================================================================

// 1.1    Defined
//------------------------------------------------------------------------------
// parse's own knob decides whether the names, templates and facts are built,
// as log.h's section 2.4 has a module with one do
#undef D_LOG_TEXT
#if D_CFG_PARSE_LOG_CODES
    #define D_LOG_TEXT(text)  (text)
#else
    #define D_LOG_TEXT(text)  NULL
#endif

D_LOG_CODES_DEFINE(parse_expr, "parse.expr", D_PARSE_EXPR_CODES)
D_LOG_CODES_DEFINE(parse_core, "parse.core", D_PARSE_CORE_CODES)

// 1.2    Registering
//------------------------------------------------------------------------------
/*
d_parse_log_add
  d_log_module_add, for both tables.
*/
int
d_parse_log_add(void)
{
    const int expr = d_log_module_add(&d_log_module_parse_expr);
    const int core = d_log_module_add(&d_log_module_parse_core);

    return ( (expr == 0) && (core == 0) ) ? 0 : -1;
}

// 1.3    Finding a diagnostic's code
//------------------------------------------------------------------------------
/*
d_parse_log_code
  A table's row, by its place: each table numbers its rows from 0, in
order, which parse.c and diagnostic.c hold at compile time; the number is
checked here too.
*/
const struct dj_log_code*
d_parse_log_code(
    uint16_t _domain,
    uint16_t _code
)
{
    const struct dj_log_module* module = NULL;

    switch (_domain)
    {
        case D_PARSE_DIAG_DOMAIN_EXPR:
            module = &d_log_module_parse_expr;
            break;
        case D_PARSE_DIAG_DOMAIN_CORE:
        case D_PARSE_DIAG_DOMAIN_MACHINE:
        case D_PARSE_DIAG_DOMAIN_PROGRAM:
            module = &d_log_module_parse_core;
            break;
        default:
            return NULL;
    }

    if ( ((size_t)_code >= module->count) ||
         (module->codes[_code]->number != (int32_t)_code) )
    {
        return NULL;
    }

    return module->codes[_code];
}


//==============================================================================
// 2.  THE SUBSCRIBER
//==============================================================================

// 2.1    Attaching
//------------------------------------------------------------------------------
/*
d_parse_log_attach
  The sink's streaming hook, asking for facts alone, where parse.log is on.
*/
int
d_parse_log_attach(
    struct dj_parse_diag_sink* _sink
)
{
    struct dj_parse_diag_settings settings;

    if (!_sink)
    {
        return -1;
    }

    d_parse_diag_settings(&settings);

    // off for the run: nothing attached, no facts gathered for the log
    if (settings.log == 0u)
    {
        return 0;
    }

    if (d_parse_log_add() != 0)
    {
        return -1;
    }

    (void)d_parse_diag_sink_stream(_sink, d_parse_log_event, NULL,
                                   D_PARSE_DIAG_STREAM_FACTS);

    return 1;
}

/*
d_parse_log_detach
  The sink's streaming hook removed, where it is the subscriber.
*/
void
d_parse_log_detach(
    struct dj_parse_diag_sink* _sink
)
{
    if ( (_sink) &&
         (_sink->stream == d_parse_log_event) )
    {
        (void)d_parse_diag_sink_stream(_sink, NULL, NULL, 0u);
    }

    return;
}


// 2.2    The hook
//------------------------------------------------------------------------------
// d_internal_parse_log_keys
//   constant: the keys of several notes' levels, in order.
static const char* const d_internal_parse_log_keys[D_PARSE_DIAG_NOTES_MAX] =
{
    "0", "1", "2", "3", "4", "5", "6", "7"
};

/*
d_internal_parse_log_put
  One fact of a record, on the state layer, observed.
*/
static int
d_internal_parse_log_put(
    struct dj_desc_sink*    _sink,
    const char*             _key,
    struct dj_ostream_value _value
)
{
    return d_desc_emit(_sink, _key, NULL, _value,
                       (uint32_t)D_DESC_LAYER_STATE, 0u,
                       (uint32_t)D_DESC_TRUTH_OBSERVED);
}

/*
d_internal_parse_log_span
  A span's facts, flat: its offset and length, its line and column where they
were computed, and its source where it is not the primary.
*/
static void
d_internal_parse_log_span(
    struct dj_desc_sink*        _sink,
    const struct dj_parse_span* _span
)
{
    (void)d_internal_parse_log_put(_sink, "offset",
                                   d_ostream_value_ulong(
                                       (unsigned long)_span->offset));
    (void)d_internal_parse_log_put(_sink, "length",
                                   d_ostream_value_ulong(
                                       (unsigned long)_span->length));

    // 0 means not computed: absent, not zero
    if (_span->line > 0u)
    {
        (void)d_internal_parse_log_put(_sink, "line",
                                       d_ostream_value_ulong(
                                           (unsigned long)_span->line));
        (void)d_internal_parse_log_put(_sink, "column",
                                       d_ostream_value_ulong(
                                           (unsigned long)_span->column));
    }

    if (_span->source != 0u)
    {
        (void)d_internal_parse_log_put(_sink, "source",
                                       d_ostream_value_ulong(
                                           (unsigned long)_span->source));
    }

    return;
}

/*
d_internal_parse_log_facts
  A diagnostic's own facts, each by its name; a fact with no value is left
out, as a template's field writes nothing for it.
*/
static void
d_internal_parse_log_facts(
    struct dj_desc_sink*              _sink,
    const struct dj_parse_diag_event* _event
)
{
    for (uint32_t i = 0u; i < _event->fact_count; ++i)
    {
        const struct dj_parse_diag_fact* const f     = &_event->facts[i];
        struct dj_ostream_value                value = d_ostream_value_none();

        switch (f->kind)
        {
            case D_PARSE_DIAG_FACT_TEXT:
                value = d_ostream_value_text_n(f->text, (size_t)f->size);
                break;
            case D_PARSE_DIAG_FACT_INTEGER:
                value.kind     = D_OSTREAM_VALUE_INTEGER;
                value.low      = f->low;
                value.high     = f->high;
                value.negative = ( (f->negative != 0u) &&
                                   ((f->low | f->high) != 0u) ) ? 1 : 0;
                break;
            case D_PARSE_DIAG_FACT_FLAG:
                value = d_ostream_value_bool((f->low != 0u) ? 1 : 0);
                break;
            default:
                continue;
        }

        if ( (f->name) &&
             (d_internal_parse_log_put(_sink, f->name, value) != 0) )
        {
            return;
        }
    }

    return;
}

/*
d_internal_parse_log_note
  One note's level: its words and its span.
*/
static void
d_internal_parse_log_note(
    struct dj_desc_sink*              _sink,
    const char*                       _key,
    const struct dj_parse_diag_event* _note
)
{
    if (d_desc_enter(_sink, _key, NULL) == 0)
    {
        return;
    }

    if (_note->text)
    {
        (void)d_internal_parse_log_put(_sink, "text",
                                       d_ostream_value_text_n(
                                           _note->text, _note->text_size));
    }

    d_internal_parse_log_span(_sink, &_note->record->span);
    d_desc_leave(_sink);

    return;
}

/*
d_parse_log_event
  Opens a record of the diagnostic's code at its grade's level - which the
log's map and log.grade_<grade> set - adds its span, its facts, its own
words where it has no template, and its notes, and writes it.
*/
void
d_parse_log_event(
    void*                             _ctx,
    const struct dj_parse_diag_sink*  _sink,
    const struct dj_parse_diag_event* _event
)
{
    const struct dj_parse_diagnostic* d;
    const struct dj_log_code*         code;
    struct dj_log_record              record;
    struct dj_desc_sink*              sink;

    (void)_ctx;
    (void)_sink;

    if ( (!_event) ||
         (!_event->record) )
    {
        return;
    }

    d = _event->record;

    // a note given alone belongs to a diagnostic this hook did not see
    if ((d->flags & D_PARSE_DIAG_FLAG_CONTINUATION) != 0u)
    {
        return;
    }

    code = d_parse_log_code(d->domain, d->code);

    // no table's row, or a record the log does not want: nothing to build
    if ( (!code) ||
         (d_log_open(&record, code, 0u) == 0) )
    {
        return;
    }

    sink = d_log_sink(&record);

    d_internal_parse_log_span(sink, &d->span);
    d_internal_parse_log_facts(sink, _event);

    // words of its own are written beside its code's template
    if ( (!_event->pattern) &&
         (_event->text) )
    {
        (void)d_internal_parse_log_put(sink, "message",
                                       d_ostream_value_text_n(
                                           _event->text,
                                           _event->text_size));
    }

    // one note is a level of its own; several, a level of levels
    if (_event->note_count == 1u)
    {
        d_internal_parse_log_note(sink, "note", &_event->notes[0]);
    }
    else if ( (_event->note_count > 1u) &&
              (d_desc_enter(sink, "notes", NULL) != 0) )
    {
        for (uint32_t i = 0u;
             ( (i < _event->note_count) && (i < D_PARSE_DIAG_NOTES_MAX) );
             ++i)
        {
            d_internal_parse_log_note(sink, d_internal_parse_log_keys[i],
                                      &_event->notes[i]);
        }

        d_desc_leave(sink);
    }

    (void)d_log_close(&record);

    return;
}

#endif  // D_INTERNAL_PARSE_LOG && RE_STD_HAS_INT64
