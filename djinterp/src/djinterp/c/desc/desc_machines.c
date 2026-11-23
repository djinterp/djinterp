/*******************************************************************************
* djinterp [c][desc]                                             desc_machines.c
*
*   desc's text, JSON and collecting machines, and describing to a stream as
* the options say. Every key and value a machine writes goes through the
* stream's writer of values (d_ostream_write_value), so JSON is escaped by
* the stream's one escaper; a text describer's words, which arrive in
* pieces, are escaped by it too, a piece at a time, without its quotes.
*
*
* path:      /src/djinterp/c/desc/desc_machines.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.17
*******************************************************************************/

#include "../../../../inc/djinterp/c/desc/desc_machines.h"  // corresponding
                                                             // header

// std
#include <string.h>  // strlen
// djinterp
#include "../../../../inc/djinterp/c/text/ostream_template.h"  // values


//==============================================================================
// WRITING
//==============================================================================

// D_INTERNAL_DESC_CHUNK
//   constant: the most bytes of a text describer's words escaped at once;
// six times it, and two quotes, fit the scratch stream.
#define D_INTERNAL_DESC_CHUNK  128u

// D_INTERNAL_DESC_PENDING, D_INTERNAL_DESC_QUOTED
//   constants: the text machine's `open` bits: a report line not yet ended,
// and a quoted text value not yet closed.
#define D_INTERNAL_DESC_PENDING  0x1u
#define D_INTERNAL_DESC_QUOTED   0x2u

// D_INTERNAL_DESC_WRITERS
//   constant: 1 when a machine that writes - text or JSON - is built.
#define D_INTERNAL_DESC_WRITERS                                                \
    ( D_DESC_MACHINE_BUILT(D_DESC_FORMAT_TEXT) ||                              \
      D_DESC_MACHINE_BUILT(D_DESC_FORMAT_JSON) )

#if D_INTERNAL_DESC_WRITERS
/*
d_internal_desc_value
  One value, raw or converted, by the stream.
*/
static void
d_internal_desc_value(
    struct dj_ostream*             _os,
    const struct dj_ostream_value* _value,
    uint32_t                       _convert
)
{
    struct dj_ostream_spec spec;

    d_ostream_spec_init(&spec);
    spec.convert = (uint8_t)_convert;
    (void)d_ostream_write_value(_os, _value, &spec);

    return;
}

#endif  // D_INTERNAL_DESC_WRITERS

#if D_CFG_DESC_TEXT && D_INTERNAL_DESC_WRITERS
/*
d_internal_desc_inner
  Words escaped as a JSON string's inside, without its quotes: each piece of
at most D_INTERNAL_DESC_CHUNK bytes, cut where a UTF-8 character starts, is
written by the stream's escaper into a scratch stream, and what lies between
its quotes is copied out.
*/
static void
d_internal_desc_inner(
    struct dj_ostream* _os,
    const char*        _text,
    size_t             _size
)
{
    char                    scratch[D_INTERNAL_DESC_CHUNK * 6u + 8u];
    struct dj_ostream       os;
    struct dj_ostream_fixed fixed;
    size_t                  at = 0u;

    while (at < _size)
    {
        size_t take = _size - at;
        size_t used;

        // a cut never falls inside a character: back off its continuations
        if (take > D_INTERNAL_DESC_CHUNK)
        {
            take = D_INTERNAL_DESC_CHUNK;

            while ( (take > 1u) &&
                    ((((unsigned char)_text[at + take]) & 0xC0u) == 0x80u) )
            {
                --take;
            }
        }

        (void)d_ostream_init_fixed(&os, &fixed, scratch, sizeof(scratch), 0);
        {
            const struct dj_ostream_value piece =
                d_ostream_value_text_n(_text + at, take);

            d_internal_desc_value(&os, &piece,
                                  (uint32_t)D_OSTREAM_CONVERT_JSON);
        }
        (void)d_ostream_close(&os);
        used = d_ostream_fixed_length(&fixed);

        if ( (used >= 2u) &&
             (d_ostream_fixed_truncated(&fixed) == 0) )
        {
            (void)d_ostream_write(_os, scratch + 1, used - 2u);
        }

        at += take;
    }

    return;
}
#endif  // D_CFG_DESC_TEXT && D_INTERNAL_DESC_WRITERS


//==============================================================================
// THE TEXT MACHINE
//==============================================================================

#if D_DESC_MACHINE_BUILT(D_DESC_FORMAT_TEXT)

// TRUTHS
//   constant: each kind of truth's words, for D_DESC_TEXT_SHOW_TRUTH.
static const char* const TRUTHS[] =
{
    "guaranteed", "implementation", "build", "run time", "observed"
};

// LAYERS
//   constant: each layer's name, for D_DESC_TEXT_SHOW_LAYER.
static const char* const LAYERS[] =
{
    "identity", "type", "state", "intent", "policy", "health",
    "provenance", "contents", "cost"
};

/*
d_internal_desc_text_close
  Ends what a text describer left open: a quoted value, or a report's line.
*/
static void
d_internal_desc_text_close(
    struct dj_desc_text_state* _state
)
{
    if ((_state->open & D_INTERNAL_DESC_QUOTED) != 0u)
    {
        (void)d_ostream_write_char(_state->os, '"');
    }

    if ((_state->open & D_INTERNAL_DESC_PENDING) != 0u)
    {
        (void)d_ostream_write_char(_state->os, '\n');
    }

    _state->open = 0u;

    return;
}

/*
d_internal_desc_text_path
  A line's key: the open levels' keys, then the fact's, joined by '.'.
*/
static void
d_internal_desc_text_path(
    struct dj_desc_text_state* _state,
    const char*                _key
)
{
    const uint32_t levels = (_state->depth < D_DESC_TEXT_PATH_MAX)
                                ? _state->depth : D_DESC_TEXT_PATH_MAX;
    uint32_t       level;

    if (_state->written != 0u)
    {
        (void)d_ostream_write_char(_state->os, ' ');
    }

    for (level = 0u; level < levels; ++level)
    {
        (void)d_ostream_write_cstr(_state->os, (_state->path[level] != NULL)
                                                   ? _state->path[level]
                                                   : "?");
        (void)d_ostream_write_char(_state->os, '.');
    }

    (void)d_ostream_write_cstr(_state->os, (_key != NULL) ? _key : "?");
    (void)d_ostream_write_char(_state->os, '=');
    ++_state->written;

    return;
}

/*
d_internal_desc_text_bare
  Whether a text needs no quotes in a line: not empty, and no space, control
character, '=', '"' or '\'.
*/
static int
d_internal_desc_text_bare(
    const struct dj_ostream_value* _value
)
{
    size_t at;

    if (_value->size == 0u)
    {
        return 0;
    }

    for (at = 0u; at < _value->size; ++at)
    {
        const unsigned char c = (unsigned char)_value->text[at];

        if ( (c <= 0x20u) || (c == 0x7Fu) ||
             (c == '=')   || (c == '"')   || (c == '\\') )
        {
            return 0;
        }
    }

    return 1;
}

/*
d_internal_desc_text_name
  A report's name for a fact or a level: the label, or the key when asked or
when there is none; with its layer before it when asked.
*/
static void
d_internal_desc_text_name(
    struct dj_desc_text_state* _state,
    const char*                _key,
    const char*                _label,
    uint32_t                   _layer,
    int                        _colon
)
{
    const int         keys = ( (_label == NULL) ||
                               ((_state->flags & D_DESC_TEXT_SHOW_KEYS) !=
                                0u) );
    const char* const name = (keys != 0) ? ((_key != NULL) ? _key : "?")
                                         : _label;
    const size_t      used = (size_t)_state->depth * 2u + strlen(name);

    (void)d_ostream_fill(_state->os, ' ', (size_t)_state->depth * 2u);

    if ( ((_state->flags & D_DESC_TEXT_SHOW_LAYER) != 0u) &&
         (_layer < (uint32_t)D_DESC_LAYER_COUNT) )
    {
        (void)d_ostream_write_char(_state->os, '[');
        (void)d_ostream_write_cstr(_state->os, LAYERS[_layer]);
        (void)d_ostream_write(_state->os, "] ", 2u);
    }

    (void)d_ostream_write_cstr(_state->os, name);

    if ( (_colon != 0) ||
         (_state->key_width == 0u) )
    {
        (void)d_ostream_write(_state->os, (_colon != 0) ? ":" : ": ",
                              (_colon != 0) ? 1u : 2u);
    }
    else
    {
        (void)d_ostream_fill(_state->os, ' ',
                             (used < (size_t)_state->key_width)
                                 ? ((size_t)_state->key_width - used) : 1u);
    }

    return;
}

static int
d_internal_desc_text_begin(
    void* _context
)
{
    struct dj_desc_text_state* const state =
        (struct dj_desc_text_state*)_context;

    state->depth   = 0u;
    state->written = 0u;
    state->open    = 0u;

    return 0;
}

static int
d_internal_desc_text_fact(
    void*                      _context,
    const struct dj_desc_fact* _fact
)
{
    struct dj_desc_text_state* const state =
        (struct dj_desc_text_state*)_context;

    d_internal_desc_text_close(state);

    if (state->preset == D_DESC_TEXT_LINE)
    {
        d_internal_desc_text_path(state, _fact->key);
        d_internal_desc_value(state->os, &_fact->value,
                              ( (_fact->value.kind ==
                                 (int32_t)D_OSTREAM_VALUE_TEXT) &&
                                (d_internal_desc_text_bare(&_fact->value)
                                 == 0) )
                                  ? (uint32_t)D_OSTREAM_CONVERT_JSON
                                  : (uint32_t)D_OSTREAM_CONVERT_RAW);
    }
    else
    {
        d_internal_desc_text_name(state, _fact->key, _fact->label,
                                  _fact->layer, 0);
        d_internal_desc_value(state->os, &_fact->value,
                              (uint32_t)D_OSTREAM_CONVERT_RAW);

        if ( ((state->flags & D_DESC_TEXT_SHOW_TRUTH) != 0u) &&
             (_fact->truth <= (uint8_t)D_DESC_TRUTH_OBSERVED) )
        {
            (void)d_ostream_write(state->os, " (", 2u);
            (void)d_ostream_write_cstr(state->os, TRUTHS[_fact->truth]);
            (void)d_ostream_write_char(state->os, ')');
        }

        (void)d_ostream_write_char(state->os, '\n');
    }

    return (d_ostream_is_ok(state->os) != 0) ? 0 : 1;
}

static int
d_internal_desc_text_enter(
    void*       _context,
    const char* _key,
    const char* _label,
    uint32_t    _depth
)
{
    struct dj_desc_text_state* const state =
        (struct dj_desc_text_state*)_context;

    d_internal_desc_text_close(state);
    state->depth = _depth;

    if (state->preset == D_DESC_TEXT_LINE)
    {
        if (_depth < D_DESC_TEXT_PATH_MAX)
        {
            state->path[_depth] = _key;
        }
    }
    else
    {
        d_internal_desc_text_name(state, _key, _label,
                                  (uint32_t)D_DESC_LAYER_CONTENTS, 1);
        (void)d_ostream_write_char(state->os, '\n');
    }

    state->depth = _depth + 1u;

    return (d_ostream_is_ok(state->os) != 0) ? 0 : 1;
}

static int
d_internal_desc_text_leave(
    void*    _context,
    uint32_t _depth
)
{
    struct dj_desc_text_state* const state =
        (struct dj_desc_text_state*)_context;

    d_internal_desc_text_close(state);
    state->depth = _depth;

    return 0;
}

#if D_CFG_DESC_TEXT
static int
d_internal_desc_text_text(
    void*       _context,
    const char* _text,
    size_t      _size
)
{
    struct dj_desc_text_state* const state =
        (struct dj_desc_text_state*)_context;

    if (_size == 0u)
    {
        return 0;
    }

    if (state->preset == D_DESC_TEXT_LINE)
    {
        if ((state->open & D_INTERNAL_DESC_QUOTED) == 0u)
        {
            d_internal_desc_text_path(state, "text");
            (void)d_ostream_write_char(state->os, '"');
            state->open |= D_INTERNAL_DESC_QUOTED;
        }

        d_internal_desc_inner(state->os, _text, _size);
    }
    else
    {
        if ((state->open & D_INTERNAL_DESC_PENDING) == 0u)
        {
            (void)d_ostream_fill(state->os, ' ', (size_t)state->depth * 2u);
        }

        (void)d_ostream_write(state->os, _text, _size);
        state->open = (_text[_size - 1u] != '\n') ? D_INTERNAL_DESC_PENDING
                                                  : 0u;
    }

    return (d_ostream_is_ok(state->os) != 0) ? 0 : 1;
}
#endif  // D_CFG_DESC_TEXT

static void
d_internal_desc_text_end(
    void* _context
)
{
    struct dj_desc_text_state* const state =
        (struct dj_desc_text_state*)_context;

    d_internal_desc_text_close(state);

    if ( (state->preset == D_DESC_TEXT_LINE)                   &&
         ((state->flags & D_DESC_TEXT_NEWLINE) != 0u)          &&
         (state->written != 0u) )
    {
        (void)d_ostream_write_char(state->os, '\n');
    }

    return;
}

// TEXT_MACHINE
//   constant: the text machine's operations.
static const struct dj_desc_machine TEXT_MACHINE =
{
    d_internal_desc_text_begin,
    d_internal_desc_text_fact,
    d_internal_desc_text_enter,
    d_internal_desc_text_leave,
#if D_CFG_DESC_TEXT
    d_internal_desc_text_text,
#else
    NULL,
#endif
    d_internal_desc_text_end
};

#endif  // D_DESC_MACHINE_BUILT(D_DESC_FORMAT_TEXT)

void
d_desc_text_init(
    struct dj_desc_text_state* _state,
    struct dj_ostream*         _os,
    uint32_t                   _preset,
    uint32_t                   _flags
)
{
    uint32_t level;

    if (_state == NULL)
    {
        return;
    }

    _state->os        = _os;
    _state->preset    = (_preset == D_DESC_TEXT_LINE) ? D_DESC_TEXT_LINE
                                                      : D_DESC_TEXT_REPORT;
    _state->flags     = _flags;
    _state->key_width = 20u;
    _state->depth     = 0u;
    _state->written   = 0u;
    _state->open      = 0u;

    for (level = 0u; level < D_DESC_TEXT_PATH_MAX; ++level)
    {
        _state->path[level] = NULL;
    }

    return;
}

const struct dj_desc_machine*
d_desc_text_machine(void)
{
#if D_DESC_MACHINE_BUILT(D_DESC_FORMAT_TEXT)
    return &TEXT_MACHINE;
#else
    return NULL;
#endif
}


//==============================================================================
// THE JSON MACHINE
//==============================================================================

#if D_DESC_MACHINE_BUILT(D_DESC_FORMAT_JSON)

// D_INTERNAL_DESC_JSON_LEVELS
//   constant: the levels the JSON machine opens; one more stops the sink.
#define D_INTERNAL_DESC_JSON_LEVELS  63u

/*
d_internal_desc_json_key
  A key as a JSON string.
*/
static void
d_internal_desc_json_key(
    struct dj_ostream* _os,
    const char*        _key
)
{
    const struct dj_ostream_value key =
        d_ostream_value_text((_key != NULL) ? _key : "");

    d_internal_desc_value(_os, &key, (uint32_t)D_OSTREAM_CONVERT_JSON);

    return;
}

/*
d_internal_desc_json_line
  In pretty output, a line break and the indent of a level.
*/
static void
d_internal_desc_json_line(
    struct dj_desc_json_state* _state,
    uint32_t                   _level
)
{
    if ((_state->flags & D_DESC_JSON_PRETTY) != 0u)
    {
        (void)d_ostream_write_char(_state->os, '\n');
        (void)d_ostream_fill(_state->os, ' ', (size_t)_level * 2u);
    }

    return;
}

/*
d_internal_desc_json_member
  Ends an open text value, then starts a member of the level open: a comma
unless it is the first, then its key and a colon.
*/
static void
d_internal_desc_json_member(
    struct dj_desc_json_state* _state,
    const char*                _key
)
{
    const uint64_t bit = (uint64_t)1u << _state->depth;

    if (_state->text != 0u)
    {
        (void)d_ostream_write_char(_state->os, '"');
        _state->text = 0u;
    }

    if ((_state->empty & bit) == 0u)
    {
        (void)d_ostream_write_char(_state->os, ',');
    }

    _state->empty &= ~bit;
    d_internal_desc_json_line(_state, _state->depth + 1u);
    d_internal_desc_json_key(_state->os, _key);
    (void)d_ostream_write(_state->os, ": ",
                          ((_state->flags & D_DESC_JSON_PRETTY) != 0u) ? 2u
                                                                       : 1u);

    return;
}

/*
d_internal_desc_json_value
  A value: an integer as a string when asked, a real that is not finite as
its name when asked, and otherwise as the stream's JSON writes it.
*/
static void
d_internal_desc_json_value(
    struct dj_desc_json_state*     _state,
    const struct dj_ostream_value* _value
)
{
    if ( (_value->kind == (int32_t)D_OSTREAM_VALUE_INTEGER) &&
         ((_state->flags & D_DESC_JSON_INTEGER_STRINGS) != 0u) )
    {
        (void)d_ostream_write_char(_state->os, '"');
        d_internal_desc_value(_state->os, _value,
                              (uint32_t)D_OSTREAM_CONVERT_RAW);
        (void)d_ostream_write_char(_state->os, '"');

        return;
    }

    // NaN is the one value unequal to itself; an infinity, past every finite
    if ( (_value->kind == (int32_t)D_OSTREAM_VALUE_REAL)             &&
         ((_state->flags & D_DESC_JSON_NONFINITE_STRINGS) != 0u)     &&
         ( (_value->real != _value->real) ||
           (_value->real > 1.7976931348623157e308) ||
           (_value->real < -1.7976931348623157e308) ) )
    {
        (void)d_ostream_write_cstr(_state->os,
                                   (_value->real != _value->real)
                                       ? "\"NaN\""
                                       : (_value->real > 0.0)
                                             ? "\"Infinity\""
                                             : "\"-Infinity\"");

        return;
    }

    d_internal_desc_value(_state->os, _value,
                          (uint32_t)D_OSTREAM_CONVERT_JSON);

    return;
}

static int
d_internal_desc_json_begin(
    void* _context
)
{
    struct dj_desc_json_state* const state =
        (struct dj_desc_json_state*)_context;

    state->depth = 0u;
    state->empty = 1u;
    state->text  = 0u;

    if ((state->flags & D_DESC_JSON_BARE) == 0u)
    {
        (void)d_ostream_write_char(state->os, '{');

        // the shape first, so a reader knows what follows
        if (state->shape != NULL)
        {
            const struct dj_ostream_value shape =
                d_ostream_value_text(state->shape);

            d_internal_desc_json_member(state, "format");
            d_internal_desc_json_value(state, &shape);
        }
    }

    return 0;
}

static int
d_internal_desc_json_fact(
    void*                      _context,
    const struct dj_desc_fact* _fact
)
{
    struct dj_desc_json_state* const state =
        (struct dj_desc_json_state*)_context;

    d_internal_desc_json_member(state, _fact->key);
    d_internal_desc_json_value(state, &_fact->value);

    return (d_ostream_is_ok(state->os) != 0) ? 0 : 1;
}

static int
d_internal_desc_json_enter(
    void*       _context,
    const char* _key,
    const char* _label,
    uint32_t    _depth
)
{
    struct dj_desc_json_state* const state =
        (struct dj_desc_json_state*)_context;

    (void)_label;
    state->depth = _depth;

    if (_depth >= D_INTERNAL_DESC_JSON_LEVELS)
    {
        return 1;
    }

    d_internal_desc_json_member(state, _key);
    (void)d_ostream_write_char(state->os, '{');
    state->depth  = _depth + 1u;
    state->empty |= (uint64_t)1u << state->depth;

    return (d_ostream_is_ok(state->os) != 0) ? 0 : 1;
}

static int
d_internal_desc_json_leave(
    void*    _context,
    uint32_t _depth
)
{
    struct dj_desc_json_state* const state =
        (struct dj_desc_json_state*)_context;
    const uint64_t bit = (uint64_t)1u << (_depth + 1u);

    if (state->text != 0u)
    {
        (void)d_ostream_write_char(state->os, '"');
        state->text = 0u;
    }

    if ((state->empty & bit) == 0u)
    {
        d_internal_desc_json_line(state, _depth + 1u);
    }

    (void)d_ostream_write_char(state->os, '}');
    state->depth = _depth;

    return (d_ostream_is_ok(state->os) != 0) ? 0 : 1;
}

#if D_CFG_DESC_TEXT
static int
d_internal_desc_json_text(
    void*       _context,
    const char* _text,
    size_t      _size
)
{
    struct dj_desc_json_state* const state =
        (struct dj_desc_json_state*)_context;

    if (state->text == 0u)
    {
        d_internal_desc_json_member(state, "text");
        (void)d_ostream_write_char(state->os, '"');
        state->text = 1u;
    }

    d_internal_desc_inner(state->os, _text, _size);

    return (d_ostream_is_ok(state->os) != 0) ? 0 : 1;
}
#endif  // D_CFG_DESC_TEXT

static void
d_internal_desc_json_end(
    void* _context
)
{
    struct dj_desc_json_state* const state =
        (struct dj_desc_json_state*)_context;

    if (state->text != 0u)
    {
        (void)d_ostream_write_char(state->os, '"');
        state->text = 0u;
    }

    // levels a stopped describer left open are closed, so the JSON is whole
    while (state->depth > 0u)
    {
        (void)d_internal_desc_json_leave(state, state->depth - 1u);
    }

    if ((state->flags & D_DESC_JSON_BARE) == 0u)
    {
        if ((state->empty & 1u) == 0u)
        {
            d_internal_desc_json_line(state, 0u);
        }

        (void)d_ostream_write_char(state->os, '}');
    }

    if ((state->flags & D_DESC_JSON_NEWLINE) != 0u)
    {
        (void)d_ostream_write_char(state->os, '\n');
    }

    return;
}

// JSON_MACHINE
//   constant: the JSON machine's operations.
static const struct dj_desc_machine JSON_MACHINE =
{
    d_internal_desc_json_begin,
    d_internal_desc_json_fact,
    d_internal_desc_json_enter,
    d_internal_desc_json_leave,
#if D_CFG_DESC_TEXT
    d_internal_desc_json_text,
#else
    NULL,
#endif
    d_internal_desc_json_end
};

#endif  // D_DESC_MACHINE_BUILT(D_DESC_FORMAT_JSON)

void
d_desc_json_init(
    struct dj_desc_json_state* _state,
    struct dj_ostream*         _os,
    uint32_t                   _flags
)
{
    if (_state == NULL)
    {
        return;
    }

    _state->os       = _os;
    _state->shape    = NULL;
    _state->flags    = _flags;
    _state->depth    = 0u;
    _state->empty    = 1u;
    _state->text     = 0u;
    _state->reserved = 0u;

    return;
}

void
d_desc_json_shape(
    struct dj_desc_json_state* _state,
    const char*                _shape
)
{
    if (_state != NULL)
    {
        _state->shape = _shape;
    }

    return;
}

const struct dj_desc_machine*
d_desc_json_machine(void)
{
#if D_DESC_MACHINE_BUILT(D_DESC_FORMAT_JSON)
    return &JSON_MACHINE;
#else
    return NULL;
#endif
}


//==============================================================================
// THE COLLECTING MACHINE
//==============================================================================

#if D_DESC_MACHINE_BUILT(D_DESC_FORMAT_COLLECT)

static int
d_internal_desc_collect_fact(
    void*                      _context,
    const struct dj_desc_fact* _fact
)
{
    struct dj_desc_collect_state* const state =
        (struct dj_desc_collect_state*)_context;

    if (state->count < state->capacity)
    {
        state->items[state->count].fact = *_fact;
        state->items[state->count].kind = D_DESC_ITEM_FACT;
        ++state->count;
    }
    else
    {
        ++state->dropped;
    }

    return 0;
}

static int
d_internal_desc_collect_enter(
    void*       _context,
    const char* _key,
    const char* _label,
    uint32_t    _depth
)
{
    struct dj_desc_collect_state* const state =
        (struct dj_desc_collect_state*)_context;

    // once full, nothing more is kept, so a level left out holds nothing
    if (state->count < state->capacity)
    {
        struct dj_desc_item* const item = &state->items[state->count];

        item->fact.key        = _key;
        item->fact.label      = _label;
        item->fact.value      = d_ostream_value_none();
        item->fact.depth      = _depth;
        item->fact.layer      = (uint16_t)D_DESC_LAYER_CONTENTS;
        item->fact.importance = 0u;
        item->fact.truth      = (uint8_t)D_DESC_TRUTH_OBSERVED;
        item->kind            = D_DESC_ITEM_ENTER;
        ++state->count;
    }

    return 0;
}

// COLLECT_MACHINE
//   constant: the collecting machine's operations; text arrives as facts.
static const struct dj_desc_machine COLLECT_MACHINE =
{
    NULL,
    d_internal_desc_collect_fact,
    d_internal_desc_collect_enter,
    NULL,
    NULL,
    NULL
};

#endif  // D_DESC_MACHINE_BUILT(D_DESC_FORMAT_COLLECT)

void
d_desc_collect_init(
    struct dj_desc_collect_state* _state,
    struct dj_desc_item*          _items,
    size_t                        _capacity
)
{
    if (_state == NULL)
    {
        return;
    }

    _state->items    = _items;
    _state->capacity = (_items != NULL) ? _capacity : 0u;
    _state->count    = 0u;
    _state->dropped  = 0u;

    return;
}

const struct dj_desc_machine*
d_desc_collect_machine(void)
{
#if D_DESC_MACHINE_BUILT(D_DESC_FORMAT_COLLECT)
    return &COLLECT_MACHINE;
#else
    return NULL;
#endif
}

/*
d_desc_replay
  Each level closed where the depth falls back, before the item that falls
back to it; the rest closed at the end.
*/
int
d_desc_replay(
    const struct dj_desc_collect_state* _state,
    const struct dj_desc_machine*       _machine,
    void*                               _context
)
{
    uint32_t depth = 0u;
    size_t   index;

    if ( (_state == NULL) ||
         (_machine == NULL) )
    {
        return 0;
    }

    for (index = 0u; index < _state->count; ++index)
    {
        const struct dj_desc_item* const item = &_state->items[index];

        while (depth > item->fact.depth)
        {
            --depth;

            if ( (_machine->leave != NULL) &&
                 (_machine->leave(_context, depth) != 0) )
            {
                return 1;
            }
        }

        if (item->kind == D_DESC_ITEM_ENTER)
        {
            if ( (_machine->enter != NULL) &&
                 (_machine->enter(_context, item->fact.key, item->fact.label,
                                  item->fact.depth) != 0) )
            {
                return 1;
            }

            depth = item->fact.depth + 1u;
        }
        else if ( (_machine->fact != NULL) &&
                  (_machine->fact(_context, &item->fact) != 0) )
        {
            return 1;
        }
    }

    while (depth > 0u)
    {
        --depth;

        if ( (_machine->leave != NULL) &&
             (_machine->leave(_context, depth) != 0) )
        {
            return 1;
        }
    }

    return 0;
}


//==============================================================================
// DESCRIBING TO A STREAM
//==============================================================================

int
d_desc_put_ostream(
    void*       _context,
    const char* _text,
    size_t      _size
)
{
    struct dj_ostream* const os = (struct dj_ostream*)_context;

    (void)d_ostream_write(os, _text, _size);

    return (d_ostream_is_ok(os) != 0) ? 0 : 1;
}

int
d_desc_output_init(
    struct dj_desc_output*     _output,
    struct dj_ostream*         _os,
    const struct dj_desc_type* _type,
    uint32_t                   _format,
    uint32_t                   _flags
)
{
    const uint32_t format = (_format == D_DESC_CHOICE_OPTION)
                                ? d_desc_option_format(_type) : _format;
    const struct dj_desc_machine* machine = NULL;
    void*                         context = NULL;

    if ( (_output == NULL) ||
         (_os == NULL) )
    {
        return -1;
    }

    if (format == D_DESC_FORMAT_TEXT)
    {
        machine = d_desc_text_machine();
        d_desc_text_init(&_output->as.text, _os, d_desc_option_text(_type),
                         _flags);
        context = &_output->as.text;
    }
    else if (format == D_DESC_FORMAT_JSON)
    {
        // desc.json_integers and desc.json_nonfinite add their switches
        machine = d_desc_json_machine();
        d_desc_json_init(&_output->as.json, _os,
                         _flags |
                         ((d_desc_option_json_integers(_type) != 0u)
                              ? D_DESC_JSON_INTEGER_STRINGS : 0u) |
                         ((d_desc_option_json_nonfinite(_type) != 0u)
                              ? D_DESC_JSON_NONFINITE_STRINGS : 0u));
        context = &_output->as.json;
    }

    if (machine == NULL)
    {
        d_desc_sink_init(&_output->sink, NULL, NULL);

        return -1;
    }

    d_desc_sink_init(&_output->sink, machine, context);
    _output->sink.layers = d_desc_option_layers();
    _output->format      = format;
    _output->reserved    = 0u;

    return 0;
}

void
d_desc_output_shape(
    struct dj_desc_output* _output,
    const char*            _shape
)
{
    if ( (_output != NULL) &&
         (_output->format == D_DESC_FORMAT_JSON) )
    {
        d_desc_json_shape(&_output->as.json, _shape);
    }

    return;
}

/*
d_internal_desc_flags
  The switches describe writes with: a line ends in a line break, and JSON
is one value a line.
*/
static uint32_t
d_internal_desc_flags(
    uint32_t _format
)
{
    return (_format == D_DESC_FORMAT_JSON) ? D_DESC_JSON_NEWLINE
                                           : D_DESC_TEXT_NEWLINE;
}

int
d_desc_write(
    struct dj_ostream*         _os,
    const struct dj_desc_type* _type,
    const void*                _object,
    uint32_t                   _style
)
{
    struct dj_desc_output out;
    uint32_t              style;
    const int             units = ( (D_CFG_DESC_UNITS) &&
                                    (_type != NULL)    &&
                                    (_type->unit_count != 0u) );
    const int             facts = ( (D_CFG_DESC_FACTS) &&
                                    (_type != NULL)    &&
                                    ( (_type->field_count != 0u) ||
                                      (_type->contents != NULL) ) );

    if ( (_os == NULL) || (_type == NULL) || (_object == NULL) )
    {
        return -1;
    }

    style = (_style == D_DESC_CHOICE_OPTION) ? d_desc_option_style(_type)
                                             : _style;
    style = ( (style == D_DESC_STYLE_UNITS) && (units == 0) && (facts != 0) )
                ? D_DESC_STYLE_FACTS
          : ( (style == D_DESC_STYLE_FACTS) && (facts == 0) && (units != 0) )
                ? D_DESC_STYLE_UNITS
                : style;

    if (style == D_DESC_STYLE_UNITS)
    {
        const int said = d_desc_say(_type, _object, D_DESC_RING_OPTION,
                                    D_DESC_LAYERS_OPTION, D_DESC_CAPITAL,
                                    d_desc_put_ostream, _os);

        if (said == 0)
        {
            (void)d_ostream_write_char(_os, '\n');
        }

        return (said < 0) ? -1 : ((d_ostream_is_ok(_os) != 0) ? 0 : 1);
    }

    if (d_desc_output_init(&out, _os, _type, D_DESC_CHOICE_OPTION,
                           d_internal_desc_flags(
                               d_desc_option_format(_type))) != 0)
    {
        return -1;
    }

    if (d_desc_run_facts(&out.sink, _type, _object) < 0)
    {
        return -1;
    }

    return (d_ostream_is_ok(_os) != 0) ? 0 : 1;
}

int
d_desc_write_with(
    struct dj_ostream*         _os,
    const struct dj_desc_type* _scope,
    fn_desc_describer          _facts,
    fn_desc_text_describer     _text,
    const void*                _object
)
{
    return d_desc_write_shaped(_os, _scope, _facts, _text, _object, NULL);
}

int
d_desc_write_shaped(
    struct dj_ostream*         _os,
    const struct dj_desc_type* _scope,
    fn_desc_describer          _facts,
    fn_desc_text_describer     _text,
    const void*                _object,
    const char*                _shape
)
{
    struct dj_desc_output out;
    uint32_t              form = d_desc_option_form(_scope);

    if ( (_os == NULL) ||
         ( (_facts == NULL) &&
           (_text == NULL) ) )
    {
        return -1;
    }

    form = ( (form == D_DESC_FORM_TEXT) && (_text == NULL) )
               ? D_DESC_FORM_FACTS
         : ( (form == D_DESC_FORM_FACTS) && (_facts == NULL) )
               ? D_DESC_FORM_TEXT
               : form;

    if (d_desc_output_init(&out, _os, _scope, D_DESC_CHOICE_OPTION,
                           d_internal_desc_flags(
                               d_desc_option_format(_scope))) != 0)
    {
        return -1;
    }

    d_desc_output_shape(&out, _shape);
    (void)((form == D_DESC_FORM_TEXT)
               ? d_desc_run_text(&out.sink, _text, _object)
               : d_desc_run(&out.sink, _facts, _object));

    return (d_ostream_is_ok(_os) != 0) ? 0 : 1;
}
