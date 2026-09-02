/******************************************************************************
* djinterp [utility]                                           document_text.c
*
*   Three algebras.  Read them side by side: each is a table of small emitters
* and none of them walks anything.  The walk is in document_node.c and there
* is exactly one of it.
*
*
* path:      /src/djinterp/c/util/document/document_text.c
* link(s):   TBA
* author(s): Agent B (structure)                           created: 2026.07.31
******************************************************************************/

#include "djinterp/c/util/document/document_text.h"

// ===========================================================================
// 0.     emission helpers
// ===========================================================================
//   Thin conveniences over d_sink_emit, which reports only whether the sink
// took every byte.  They are STATIC and they are dialect business: the shared
// sink header offers one primitive on purpose, and a repeat-fill or a decimal
// formatter is not a sink concept.  If a second subframework wants them, they
// belong in sink_common.h -- not copied here and there.
//
//   d_sink_emit answers 1 or 0.  A zero becomes SINK_FULL, which is
// MECHANICAL; a caller wanting to distinguish "full" from "broken" reads the
// buffer sink's own overflow and needed fields, which survive the render.

static int32_t
doc_emit_bytes(
    const struct d_pack_sink* _sink,
    const char*               _bytes,
    size_t                    _length
)
{
    if (!_sink)
    {
        return (int32_t)D_DOC_RENDER_SINK_ERROR;
    }

    return d_sink_emit((*_sink), _bytes, _length)
         ? (int32_t)D_DOC_RENDER_OK
         : (int32_t)D_DOC_RENDER_SINK_FULL;
}

static int32_t
doc_emit_text(
    const struct d_pack_sink* _sink,
    const char*               _text
)
{
    size_t length;

    if (!_text)
    {
        return (int32_t)D_DOC_RENDER_OK;
    }

    length = 0u;

    while (_text[length])
    {
        ++length;
    }

    return doc_emit_bytes(_sink, _text, length);
}

static int32_t
doc_emit_char(
    const struct d_pack_sink* _sink,
    char                      _byte
)
{
    return doc_emit_bytes(_sink, &_byte, 1u);
}

static int32_t
doc_emit_repeat(
    const struct d_pack_sink* _sink,
    char                      _byte,
    size_t                    _count
)
{
    char    block[32];
    size_t  index;
    size_t  chunk;
    int32_t status;

    for (index = 0u; index < sizeof(block); ++index)
    {
        block[index] = _byte;
    }

    status = (int32_t)D_DOC_RENDER_OK;

    // chunked, so a wide pad costs one call per 32 bytes rather than one per
    // byte, without storage proportional to the count
    while ( (_count > 0u) &&
            (status == (int32_t)D_DOC_RENDER_OK) )
    {
        chunk  = (_count > sizeof(block)) ? sizeof(block) : _count;
        status = doc_emit_bytes(_sink, block, chunk);
        _count -= chunk;
    }

    return status;
}

static int32_t
doc_emit_uint(
    const struct d_pack_sink* _sink,
    uint32_t                  _value
)
{
    char   digits[12];
    size_t used;
    size_t index;
    char   swap;

    used = 0u;

    // written backwards then reversed: no snprintf, so no locale anywhere in
    // the byte path
    do
    {
        digits[used] = (char)('0' + (int)(_value % 10u));
        _value      /= 10u;
        ++used;
    }
    while ( (_value > 0u) &&
            (used < sizeof(digits)) );

    for (index = 0u; index < (used / 2u); ++index)
    {
        swap                      = digits[index];
        digits[index]             = digits[used - 1u - index];
        digits[used - 1u - index] = swap;
    }

    return doc_emit_bytes(_sink, digits, used);
}




// doc_state
//   helper: the context, typed.
static struct d_doc_text_state*
doc_state(
    void* _context
)
{
    return (struct d_doc_text_state*)_context;
}

// doc_len
//   helper: byte length of a borrowed string, NULL reading as zero.
static size_t
doc_len(
    const char* _text
)
{
    size_t length;

    length = 0u;

    if (_text)
    {
        while (_text[length])
        {
            ++length;
        }
    }

    return length;
}

// doc_ordered_at
//   helper: whether the list at _depth is ordered.  One bit per level, so
// nesting an unordered list inside an ordered one restores the right marker
// on the way back out.
static int32_t
doc_ordered_at(
    const struct d_doc_text_state* _state,
    uint32_t                       _depth
)
{
    if ( (_depth == 0u) ||
         (_depth > D_INTERNAL_DOC_MAX_LIST_DEPTH) )
    {
        return 0;
    }

    return (_state->ordered_bits & (1u << (_depth - 1u))) ? 1 : 0;
}

// doc_push_list / doc_pop_list
//   helper: enter and leave one level of list nesting.  Past the tracked
// depth a list still renders, unnumbered -- degrade, never error.
static void
doc_push_list(
    struct d_doc_text_state* _state,
    int32_t                  _ordered
)
{
    ++_state->list_depth;

    if (_state->list_depth <= D_INTERNAL_DOC_MAX_LIST_DEPTH)
    {
        _state->counters[_state->list_depth - 1u] = 0u;

        if (_ordered)
        {
            _state->ordered_bits |= (1u << (_state->list_depth - 1u));
        }
        else
        {
            _state->ordered_bits &= ~(1u << (_state->list_depth - 1u));
        }
    }

    return;
}

static void
doc_pop_list(
    struct d_doc_text_state* _state
)
{
    if (_state->list_depth > 0u)
    {
        --_state->list_depth;
    }

    return;
}

// doc_next_number
//   helper: the next ordinal at the current depth, or 0 when the depth is
// past what is tracked.
static uint32_t
doc_next_number(
    struct d_doc_text_state* _state
)
{
    if ( (_state->list_depth == 0u) ||
         (_state->list_depth > D_INTERNAL_DOC_MAX_LIST_DEPTH) )
    {
        return 0u;
    }

    ++_state->counters[_state->list_depth - 1u];

    return _state->counters[_state->list_depth - 1u];
}


// ===========================================================================
// I.     plain text
// ===========================================================================
//   The console dialect.  Streams; pads only to a declared `width` hint.

static const char* const g_plain_reads[] =
{
    D_DOC_ATTR_ALIGN,
    D_DOC_ATTR_INDENT,
    D_DOC_ATTR_WIDTH
};

// plain_pad
//   helper: emit _text, then pad to the `width` HINT when one is declared and
// the text is shorter.  A cell wider than its hint is never truncated, and a
// cell with no hint is never padded -- so no output here depends on any other
// cell's content, which is what keeps this a stream.
static int32_t
plain_pad(
    const struct d_pack_sink*       _sink,
    const char*                    _text,
    const struct d_doc_attributes* _attrs
)
{
    uint32_t width;
    size_t   length;
    int32_t  status;

    status = doc_emit_text(_sink, _text);

    if (status != (int32_t)D_DOC_RENDER_OK)
    {
        return status;
    }

    width  = d_doc_attr_uint(_attrs, D_DOC_ATTR_WIDTH, 0u);
    length = doc_len(_text);

    if ((size_t)width > length)
    {
        return doc_emit_repeat(_sink, ' ', (size_t)width - length);
    }

    return (int32_t)D_DOC_RENDER_OK;
}

static int32_t
plain_write_line(
    const char*                    _text,
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    struct d_doc_text_state* state;
    uint32_t                 indent;
    int32_t                  status;

    state  = doc_state(_context);
    indent = d_doc_attr_uint(_attrs, D_DOC_ATTR_INDENT, 0u);
    status = (int32_t)D_DOC_RENDER_OK;

    if (indent > 0u)
    {
        status = doc_emit_repeat(&state->sink, ' ', (size_t)indent);
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_text(&state->sink, _text);
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_char(&state->sink, '\n');
    }

    return status;
}

static int32_t
plain_heading(
    uint32_t                       _level,
    const char*                    _text,
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    struct d_doc_text_state* state;
    int32_t                  status;

    (void)_attrs;
    state  = doc_state(_context);
    status = doc_emit_text(&state->sink, _text);

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_char(&state->sink, '\n');
    }

    // the underline is as long as the title, which is a count of bytes this
    // dialect was handed -- not a measurement of how wide it will appear
    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_repeat(&state->sink,
                                   (_level <= 1u) ? '=' : '-',
                                   doc_len(_text));
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_char(&state->sink, '\n');
    }

    return status;
}

static int32_t
plain_rule(
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    struct d_doc_text_state* state;
    uint32_t                 width;
    int32_t                  status;

    state  = doc_state(_context);
    width  = d_doc_attr_uint(_attrs, D_DOC_ATTR_WIDTH, D_INTERNAL_DOC_RULE_WIDTH);
    status = doc_emit_repeat(&state->sink, '-', (size_t)width);

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_char(&state->sink, '\n');
    }

    return status;
}

static int32_t
plain_page_break(
    void* _context
)
{
    return doc_emit_char(&doc_state(_context)->sink, '\f');
}

static int32_t
plain_begin_list(
    int32_t                        _ordered,
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    (void)_attrs;
    doc_push_list(doc_state(_context), _ordered);

    return (int32_t)D_DOC_RENDER_OK;
}

static int32_t
plain_end_list(
    void* _context
)
{
    doc_pop_list(doc_state(_context));

    return (int32_t)D_DOC_RENDER_OK;
}

static int32_t
plain_begin_item(
    const char*                    _text,
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    struct d_doc_text_state* state;
    uint32_t                 number;
    int32_t                  status;

    (void)_attrs;
    state = doc_state(_context);

    status = doc_emit_repeat(&state->sink, ' ',
                               (size_t)(2u * (state->list_depth > 0u
                                              ? state->list_depth - 1u
                                              : 0u)));

    if (status != (int32_t)D_DOC_RENDER_OK)
    {
        return status;
    }

    if (doc_ordered_at(state, state->list_depth))
    {
        number = doc_next_number(state);

        // a depth past the tracked range yields 0, which falls back to a
        // bullet rather than printing "0."
        if (number > 0u)
        {
            status = doc_emit_uint(&state->sink, number);

            if (status == (int32_t)D_DOC_RENDER_OK)
            {
                status = doc_emit_text(&state->sink, ". ");
            }
        }
        else
        {
            status = doc_emit_text(&state->sink, "- ");
        }
    }
    else
    {
        status = doc_emit_text(&state->sink, "- ");
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_text(&state->sink, _text);
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_char(&state->sink, '\n');
    }

    return status;
}

static int32_t
plain_begin_table(
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    struct d_doc_text_state* state;

    (void)_attrs;
    state                = doc_state(_context);
    state->table_columns = 0u;
    state->header_bytes  = 0u;
    state->in_table      = 1;
    state->saw_row       = 0;

    return (int32_t)D_DOC_RENDER_OK;
}

static int32_t
plain_column(
    const char*                    _header,
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    struct d_doc_text_state* state;
    uint32_t                 width;
    size_t                   length;
    int32_t                  status;

    state  = doc_state(_context);
    status = (int32_t)D_DOC_RENDER_OK;

    if (state->table_columns > 0u)
    {
        status               = doc_emit_text(&state->sink, " | ");
        state->header_bytes += 3u;
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = plain_pad(&state->sink, _header, _attrs);
    }

    width  = d_doc_attr_uint(_attrs, D_DOC_ATTR_WIDTH, 0u);
    length = doc_len(_header);

    state->header_bytes += (uint32_t)(((size_t)width > length) ? (size_t)width
                                                               : length);
    ++state->table_columns;

    return status;
}

static int32_t
plain_begin_row(
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    struct d_doc_text_state* state;
    int32_t                  status;

    (void)_attrs;
    state           = doc_state(_context);
    state->table_cell = 0u;
    status          = (int32_t)D_DOC_RENDER_OK;

    // the header rule is emitted once, on the first row, so a header-only
    // table is a legitimate document that simply has no rule under it
    if (!state->saw_row)
    {
        state->saw_row = 1;

        if (state->table_columns > 0u)
        {
            status = doc_emit_char(&state->sink, '\n');

            if (status == (int32_t)D_DOC_RENDER_OK)
            {
                status = doc_emit_repeat(&state->sink, '-',
                                           (size_t)state->header_bytes);
            }
        }
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_char(&state->sink, '\n');
    }

    return status;
}

static int32_t
plain_cell(
    const char*                    _text,
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    struct d_doc_text_state* state;
    int32_t                  status;

    state  = doc_state(_context);
    status = (int32_t)D_DOC_RENDER_OK;

    if (state->table_cell > 0u)
    {
        status = doc_emit_text(&state->sink, " | ");
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = plain_pad(&state->sink, _text, _attrs);
    }

    ++state->table_cell;

    return status;
}

static int32_t
plain_end_table(
    void* _context
)
{
    struct d_doc_text_state* state;

    state           = doc_state(_context);
    state->in_table = 0;

    return doc_emit_char(&state->sink, '\n');
}

struct d_doc_render_algebra
d_doc_plain_algebra(
    struct d_doc_text_state* _state
)
{
    struct d_doc_render_algebra algebra;

    algebra.context        = _state;
    algebra.reads          = g_plain_reads;
    algebra.read_count     = (uint32_t)(sizeof(g_plain_reads) /
                                        sizeof(g_plain_reads[0]));
    algebra.reserved       = 0u;

    algebra.begin_document = NULL;
    algebra.end_document   = NULL;
    algebra.begin_element  = NULL;   // default: character data as one line
    algebra.end_element    = NULL;
    algebra.heading        = plain_heading;
    algebra.paragraph      = NULL;   // default: one line
    algebra.key_value      = NULL;   // default: "key: value"
    algebra.rule           = plain_rule;
    algebra.space          = NULL;   // points need a metric; see the header
    algebra.page_break     = plain_page_break;
    algebra.begin_list     = plain_begin_list;
    algebra.begin_item     = plain_begin_item;
    algebra.end_item       = NULL;
    algebra.end_list       = plain_end_list;
    algebra.begin_column_group = NULL;   // groups are not
    algebra.end_column_group   = NULL;   // expressible; columns still flow
    algebra.begin_table    = plain_begin_table;
    algebra.column         = plain_column;
    algebra.begin_row      = plain_begin_row;
    algebra.cell           = plain_cell;
    algebra.end_row        = NULL;
    algebra.end_table      = plain_end_table;
    algebra.write_line     = plain_write_line;

    return algebra;
}


// ===========================================================================
// II-b.  plain text, ALIGNED
// ===========================================================================
//   The fourth algebra.  Identical to plain everywhere except the table
// group, where it buffers cell pointers, takes the maximum byte length per
// column, and emits the whole grid at end_table.
//
//   It shares every other verb with the streaming dialect, which is the
// evidence that this is one dialect family and not two programs: the diff
// between plain and plain-aligned is six function pointers.

/*
d_doc_table_buffer_init
  Binds an aligned dialect's scratch to caller-owned storage.

Parameter(s):
  _buffer:          the buffer.
  _cells:           storage for one borrowed pointer per cell.
  _cell_capacity:   how many.
  _widths:          storage for one running width per column.
  _column_capacity: how many.
Return:
  none.
*/
void
d_doc_table_buffer_init(
    struct d_doc_table_buffer* _buffer,
    const char**               _cells,
    uint32_t                   _cell_capacity,
    uint32_t*                  _widths,
    uint32_t                   _column_capacity
)
{
    if (!_buffer)
    {
        return;
    }

    _buffer->cells           = _cells;
    _buffer->widths          = _widths;
    _buffer->cell_capacity   = _cell_capacity;
    _buffer->column_capacity = _column_capacity;
    _buffer->cell_count      = 0u;
    _buffer->columns         = 0u;
    _buffer->rows            = 0u;
    _buffer->overflowed      = 0;

    return;
}

// aligned_note_width
//   helper: widen column _column to fit _text.  The width is the byte length
// of a string this dialect was handed, or the caller's `width` hint, whichever
// is larger -- a declared minimum, never a maximum.
static void
aligned_note_width(
    struct d_doc_table_buffer*     _buffer,
    uint32_t                       _column,
    const char*                    _text,
    const struct d_doc_attributes* _attrs
)
{
    uint32_t wanted;

    if ( (!_buffer->widths) ||
         (_column >= _buffer->column_capacity) )
    {
        return;
    }

    wanted = (uint32_t)doc_len(_text);

    {
        const uint32_t hinted = d_doc_attr_uint(_attrs, D_DOC_ATTR_WIDTH, 0u);

        if (hinted > wanted)
        {
            wanted = hinted;
        }
    }

    if (wanted > _buffer->widths[_column])
    {
        _buffer->widths[_column] = wanted;
    }

    return;
}

// aligned_store
//   helper: keep one borrowed cell pointer.  The strings live in the tree, or
// in the expansion arena, both of which outlive the render -- so nothing is
// copied here.
static int32_t
aligned_store(
    struct d_doc_table_buffer* _buffer,
    const char*                _text
)
{
    if ( (!_buffer->cells) ||
         (_buffer->cell_count >= _buffer->cell_capacity) )
    {
        _buffer->overflowed = 1;

        return (int32_t)D_DOC_RENDER_SINK_FULL;
    }

    _buffer->cells[_buffer->cell_count] = _text ? _text : "";
    ++_buffer->cell_count;

    return (int32_t)D_DOC_RENDER_OK;
}

static int32_t
aligned_begin_table(
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    struct d_doc_text_state* state;
    uint32_t                 index;

    (void)_attrs;
    state = doc_state(_context);

    if (!state->table_buffer)
    {
        // no buffer bound: this dialect cannot align without one, and
        // silently producing an unaligned table would make the caller's
        // choice of dialect a lie
        return (int32_t)D_DOC_RENDER_SINK_ERROR;
    }

    state->table_buffer->cell_count = 0u;
    state->table_buffer->columns    = 0u;
    state->table_buffer->rows       = 0u;
    state->table_buffer->overflowed = 0;

    for (index = 0u; index < state->table_buffer->column_capacity; ++index)
    {
        state->table_buffer->widths[index] = 0u;
    }

    return (int32_t)D_DOC_RENDER_OK;
}

static int32_t
aligned_column(
    const char*                    _header,
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    struct d_doc_text_state* state;
    int32_t                  status;

    state  = doc_state(_context);
    status = aligned_store(state->table_buffer, _header);

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        aligned_note_width(state->table_buffer,
                           state->table_buffer->columns, _header, _attrs);
        ++state->table_buffer->columns;
    }

    return status;
}

static int32_t
aligned_begin_row(
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    (void)_attrs;
    doc_state(_context)->table_cell = 0u;

    return (int32_t)D_DOC_RENDER_OK;
}

static int32_t
aligned_cell(
    const char*                    _text,
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    struct d_doc_text_state* state;
    int32_t                  status;

    state  = doc_state(_context);
    status = aligned_store(state->table_buffer, _text);

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        aligned_note_width(state->table_buffer, state->table_cell,
                           _text, _attrs);
        ++state->table_cell;
    }

    return status;
}

static int32_t
aligned_end_row(
    void* _context
)
{
    struct d_doc_text_state* state;
    uint32_t                 short_by;

    state = doc_state(_context);

    // a short row is padded with empty cells so the grid stays rectangular.
    // A ragged table is a producer's mistake; refusing to render it would
    // help nobody
    short_by = (state->table_cell < state->table_buffer->columns)
             ? (state->table_buffer->columns - state->table_cell)
             : 0u;

    while (short_by > 0u)
    {
        const int32_t status = aligned_store(state->table_buffer, "");

        if (status != (int32_t)D_DOC_RENDER_OK)
        {
            return status;
        }

        --short_by;
    }

    ++state->table_buffer->rows;

    return (int32_t)D_DOC_RENDER_OK;
}

// aligned_emit_row
//   helper: one buffered row, cells padded to their column widths, joined by
// " | ".  Trailing padding on the last column is dropped, because a line that
// ends in spaces differs by bytes from one that does not and nothing is
// gained by it.
static int32_t
aligned_emit_row(
    struct d_doc_text_state* _state,
    uint32_t                 _row
)
{
    struct d_doc_table_buffer* buffer;
    const char*                text;
    uint32_t                   column;
    uint32_t                   width;
    size_t                     length;
    int32_t                    status;

    buffer = _state->table_buffer;
    status = (int32_t)D_DOC_RENDER_OK;

    for (column = 0u;
         (column < buffer->columns) &&
             (status == (int32_t)D_DOC_RENDER_OK);
         ++column)
    {
        if (column > 0u)
        {
            status = doc_emit_text(&_state->sink, " | ");

            if (status != (int32_t)D_DOC_RENDER_OK)
            {
                break;
            }
        }

        text   = buffer->cells[(_row * buffer->columns) + column];
        width  = buffer->widths[column];
        length = doc_len(text);
        status = doc_emit_text(&_state->sink, text);

        if ( (status == (int32_t)D_DOC_RENDER_OK) &&
             ((column + 1u) < buffer->columns)    &&
             ((size_t)width > length) )
        {
            status = doc_emit_repeat(&_state->sink, ' ',
                                       (size_t)width - length);
        }
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_char(&_state->sink, '\n');
    }

    return status;
}

static int32_t
aligned_end_table(
    void* _context
)
{
    struct d_doc_text_state*   state;
    struct d_doc_table_buffer* buffer;
    uint32_t                   rule;
    uint32_t                   column;
    uint32_t                   row;
    int32_t                    status;

    state  = doc_state(_context);
    buffer = state->table_buffer;

    if (buffer->columns == 0u)
    {
        return doc_emit_char(&state->sink, '\n');
    }

    // the header is buffered row 0, so the whole grid is one loop over
    // rows + 1 -- and the rule sits between the first and the rest
    status = aligned_emit_row(state, 0u);

    if (status != (int32_t)D_DOC_RENDER_OK)
    {
        return status;
    }

    rule = 0u;

    for (column = 0u; column < buffer->columns; ++column)
    {
        rule += buffer->widths[column];
    }

    rule += (buffer->columns - 1u) * 3u;   // the " | " joins

    status = doc_emit_repeat(&state->sink, '-', (size_t)rule);

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_char(&state->sink, '\n');
    }

    for (row = 1u;
         (row <= buffer->rows) &&
             (status == (int32_t)D_DOC_RENDER_OK);
         ++row)
    {
        status = aligned_emit_row(state, row);
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_char(&state->sink, '\n');
    }

    return status;
}

struct d_doc_render_algebra
d_doc_plain_aligned_algebra(
    struct d_doc_text_state* _state
)
{
    struct d_doc_render_algebra algebra;

    // everything but the table group is the streaming dialect, unchanged --
    // the diff between plain and plain-aligned is six function pointers
    algebra             = d_doc_plain_algebra(_state);
    algebra.begin_table = aligned_begin_table;
    algebra.column      = aligned_column;
    algebra.begin_row   = aligned_begin_row;
    algebra.cell        = aligned_cell;
    algebra.end_row     = aligned_end_row;
    algebra.end_table   = aligned_end_table;

    return algebra;
}


// ===========================================================================
// III.   Markdown
// ===========================================================================

static const char* const g_markdown_reads[] =
{
    D_DOC_ATTR_ALIGN
};

static int32_t
md_write_line(
    const char*                    _text,
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    struct d_doc_text_state* state;
    int32_t                  status;

    (void)_attrs;
    state  = doc_state(_context);
    status = doc_emit_text(&state->sink, _text);

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_text(&state->sink, "\n\n");
    }

    return status;
}

static int32_t
md_heading(
    uint32_t                       _level,
    const char*                    _text,
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    struct d_doc_text_state* state;
    int32_t                  status;

    (void)_attrs;
    state = doc_state(_context);

    // Markdown has six levels; deeper headings clamp rather than emitting a
    // run of hashes no parser recognises.  Degrade, never error
    status = doc_emit_repeat(&state->sink, '#',
                               (size_t)((_level > 6u) ? 6u
                                        : (_level < 1u) ? 1u : _level));

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_char(&state->sink, ' ');
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_text(&state->sink, _text);
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_text(&state->sink, "\n\n");
    }

    return status;
}

static int32_t
md_key_value(
    const char*                    _key,
    const char*                    _value,
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    struct d_doc_text_state* state;
    int32_t                  status;

    (void)_attrs;
    state  = doc_state(_context);
    status = doc_emit_text(&state->sink, "**");

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_text(&state->sink, _key);
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_text(&state->sink, "**: ");
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_text(&state->sink, _value);
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_text(&state->sink, "\n\n");
    }

    return status;
}

static int32_t
md_rule(
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    (void)_attrs;

    return doc_emit_text(&doc_state(_context)->sink, "---\n\n");
}

static int32_t
md_space(
    int32_t                        _millipoints,
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    // the amount is ignored on purpose: converting points to blank lines
    // needs a metric, and this side has none
    (void)_millipoints;
    (void)_attrs;

    return doc_emit_char(&doc_state(_context)->sink, '\n');
}

static int32_t
md_begin_list(
    int32_t                        _ordered,
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    (void)_attrs;
    doc_push_list(doc_state(_context), _ordered);

    return (int32_t)D_DOC_RENDER_OK;
}

static int32_t
md_end_list(
    void* _context
)
{
    struct d_doc_text_state* state;

    state = doc_state(_context);
    doc_pop_list(state);

    // one blank line after the outermost list only, so a nested list does not
    // break its parent's block
    if (state->list_depth == 0u)
    {
        return doc_emit_char(&state->sink, '\n');
    }

    return (int32_t)D_DOC_RENDER_OK;
}

static int32_t
md_begin_item(
    const char*                    _text,
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    struct d_doc_text_state* state;
    uint32_t                 number;
    int32_t                  status;

    (void)_attrs;
    state = doc_state(_context);

    status = doc_emit_repeat(&state->sink, ' ',
                               (size_t)(2u * (state->list_depth > 0u
                                              ? state->list_depth - 1u
                                              : 0u)));

    if (status != (int32_t)D_DOC_RENDER_OK)
    {
        return status;
    }

    number = doc_ordered_at(state, state->list_depth) ? doc_next_number(state)
                                                      : 0u;

    if (number > 0u)
    {
        status = doc_emit_uint(&state->sink, number);

        if (status == (int32_t)D_DOC_RENDER_OK)
        {
            status = doc_emit_text(&state->sink, ". ");
        }
    }
    else
    {
        status = doc_emit_text(&state->sink, "- ");
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_text(&state->sink, _text);
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_char(&state->sink, '\n');
    }

    return status;
}

static int32_t
md_begin_table(
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    struct d_doc_text_state* state;

    (void)_attrs;
    state                = doc_state(_context);
    state->table_columns = 0u;
    state->saw_row       = 0;
    state->in_table      = 1;

    return doc_emit_char(&state->sink, '|');
}

// md_alignment_marker
//   helper: the delimiter-row cell for one column's `align` hint.  This is
// the only place any dialect here reads a hint to change its SYNTAX rather
// than its spacing, and it is why Markdown declares `align` in its reads set.
static const char*
md_alignment_marker(
    enum d_doc_align _align
)
{
    switch (_align)
    {
        case D_DOC_ALIGN_CENTER: { return " :---: "; }
        case D_DOC_ALIGN_RIGHT:  { return " ---: ";  }
        default:                 { return " --- ";   }
    }
}

static int32_t
md_column(
    const char*                    _header,
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    struct d_doc_text_state* state;
    int32_t                  status;

    state  = doc_state(_context);
    status = doc_emit_char(&state->sink, ' ');

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_text(&state->sink, _header);
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_text(&state->sink, " |");
    }

    // the alignment hint is remembered here because the delimiter row is
    // written after the fold has moved past the columns.  Bounded, and it
    // stores a declared hint rather than anything derived from the data
    if (state->table_columns < D_INTERNAL_DOC_MAX_TABLE_COLUMNS)
    {
        state->column_align[state->table_columns] =
            (uint8_t)d_doc_attr_align(_attrs, D_DOC_ALIGN_LEFT);
    }

    ++state->table_columns;

    return status;
}

// md_delimiter_row
//   helper: the `| --- | :---: |` row Markdown requires between the header
// and the body.  Emitted on the first row, or at end_table for a header-only
// table, because either way the fold guarantees every column has arrived.
static int32_t
md_delimiter_row(
    struct d_doc_text_state* _state
)
{
    uint32_t index;
    int32_t  status;

    status = doc_emit_text(&_state->sink, "\n|");

    for (index = 0u;
         (index < _state->table_columns) &&
             (status == (int32_t)D_DOC_RENDER_OK);
         ++index)
    {
        status = doc_emit_text(
                     &_state->sink,
                     md_alignment_marker(
                         (enum d_doc_align)
                         ((index < D_INTERNAL_DOC_MAX_TABLE_COLUMNS)
                              ? _state->column_align[index]
                              : (uint8_t)D_DOC_ALIGN_LEFT)));

        if (status == (int32_t)D_DOC_RENDER_OK)
        {
            status = doc_emit_char(&_state->sink, '|');
        }
    }

    _state->saw_row = 1;

    return status;
}

static int32_t
md_begin_row(
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    struct d_doc_text_state* state;
    int32_t                  status;

    (void)_attrs;
    state             = doc_state(_context);
    state->table_cell = 0u;
    status            = (int32_t)D_DOC_RENDER_OK;

    if (!state->saw_row)
    {
        status = md_delimiter_row(state);

        if (status == (int32_t)D_DOC_RENDER_OK)
        {
            status = doc_emit_char(&state->sink, '\n');
        }
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_char(&state->sink, '|');
    }

    return status;
}

static int32_t
md_cell(
    const char*                    _text,
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    struct d_doc_text_state* state;
    int32_t                  status;

    (void)_attrs;
    state  = doc_state(_context);
    status = doc_emit_char(&state->sink, ' ');

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_text(&state->sink, _text);
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_text(&state->sink, " |");
    }

    ++state->table_cell;

    return status;
}

static int32_t
md_end_row(
    void* _context
)
{
    return doc_emit_char(&doc_state(_context)->sink, '\n');
}

static int32_t
md_end_table(
    void* _context
)
{
    struct d_doc_text_state* state;
    int32_t                  status;

    state  = doc_state(_context);
    status = (int32_t)D_DOC_RENDER_OK;

    // a header-only table is a legitimate document, and it still needs its
    // delimiter row or no Markdown reader will see a table at all
    if (!state->saw_row)
    {
        status = md_delimiter_row(state);

        if (status == (int32_t)D_DOC_RENDER_OK)
        {
            status = doc_emit_char(&state->sink, '\n');
        }
    }

    state->in_table = 0;

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_char(&state->sink, '\n');
    }

    return status;
}

struct d_doc_render_algebra
d_doc_markdown_algebra(
    struct d_doc_text_state* _state
)
{
    struct d_doc_render_algebra algebra;

    algebra.context        = _state;
    algebra.reads          = g_markdown_reads;
    algebra.read_count     = (uint32_t)(sizeof(g_markdown_reads) /
                                        sizeof(g_markdown_reads[0]));
    algebra.reserved       = 0u;

    algebra.begin_document = NULL;
    algebra.end_document   = NULL;
    algebra.begin_element  = NULL;
    algebra.end_element    = NULL;
    algebra.heading        = md_heading;
    algebra.paragraph      = NULL;   // default funnels to md_write_line
    algebra.key_value      = md_key_value;
    algebra.rule           = md_rule;
    algebra.space          = md_space;
    algebra.page_break     = NULL;   // Markdown is continuous; nothing to say
    algebra.begin_list     = md_begin_list;
    algebra.begin_item     = md_begin_item;
    algebra.end_item       = NULL;
    algebra.end_list       = md_end_list;
    algebra.begin_column_group = NULL;   // groups are not
    algebra.end_column_group   = NULL;   // expressible; columns still flow
    algebra.begin_table    = md_begin_table;
    algebra.column         = md_column;
    algebra.begin_row      = md_begin_row;
    algebra.cell           = md_cell;
    algebra.end_row        = md_end_row;
    algebra.end_table      = md_end_table;
    algebra.write_line     = md_write_line;

    return algebra;
}


// ===========================================================================
// IV.    markup
// ===========================================================================
//   The one omnivorous dialect: it emits every hint as an attribute, so it
// declares NO reads set.  That is not an omission -- `reads == NULL` means
// "unknown", and the tolerance law is stated only over dialects that declare
// one.  A dialect that really does read everything must not claim otherwise.

// markup_escape
//   helper: emit _text with the five XML-significant bytes replaced.  Applied
// to every string that reaches the byte stream, attribute values included.
static int32_t
markup_escape(
    const struct d_pack_sink* _sink,
    const char*              _text
)
{
    const char* run;
    const char* cursor;
    const char* entity;
    int32_t     status;

    if (!_text)
    {
        return (int32_t)D_DOC_RENDER_OK;
    }

    run    = _text;
    cursor = _text;
    status = (int32_t)D_DOC_RENDER_OK;

    while ( (*cursor) &&
            (status == (int32_t)D_DOC_RENDER_OK) )
    {
        entity = NULL;

        switch (*cursor)
        {
            case '&':  { entity = "&amp;";  break; }
            case '<':  { entity = "&lt;";   break; }
            case '>':  { entity = "&gt;";   break; }
            case '"':  { entity = "&quot;"; break; }
            case '\'': { entity = "&apos;"; break; }
            default:   { break; }
        }

        if (entity)
        {
            status = doc_emit_bytes(_sink, run, (size_t)(cursor - run));

            if (status == (int32_t)D_DOC_RENDER_OK)
            {
                status = doc_emit_text(_sink, entity);
            }

            run = cursor + 1;
        }

        ++cursor;
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_bytes(_sink, run, (size_t)(cursor - run));
    }

    return status;
}

// markup_attrs
//   helper: emit a hint bag as attributes, in the bag's own order -- which is
// canonical by ruling R2, and is therefore the same in both languages without
// this function sorting anything.
static int32_t
markup_attrs(
    const struct d_pack_sink*       _sink,
    const struct d_doc_attributes* _attrs
)
{
    uint32_t index;
    int32_t  status;

    status = (int32_t)D_DOC_RENDER_OK;

    if ( (!_attrs) ||
         (!_attrs->items) )
    {
        return status;
    }

    for (index = 0u;
         (index < _attrs->count) &&
             (status == (int32_t)D_DOC_RENDER_OK);
         ++index)
    {
        status = doc_emit_char(_sink, ' ');

        if (status == (int32_t)D_DOC_RENDER_OK)
        {
            status = markup_escape(_sink, _attrs->items[index].key);
        }

        if (status == (int32_t)D_DOC_RENDER_OK)
        {
            status = doc_emit_text(_sink, "=\"");
        }

        if (status == (int32_t)D_DOC_RENDER_OK)
        {
            status = markup_escape(_sink, _attrs->items[index].value);
        }

        if (status == (int32_t)D_DOC_RENDER_OK)
        {
            status = doc_emit_char(_sink, '"');
        }
    }

    return status;
}

// markup_open / markup_close
//   helper: an open or close tag with optional attributes.
static int32_t
markup_open(
    const struct d_pack_sink*       _sink,
    const char*                    _name,
    const struct d_doc_attributes* _attrs
)
{
    int32_t status;

    status = doc_emit_char(_sink, '<');

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_text(_sink, _name);
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = markup_attrs(_sink, _attrs);
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_char(_sink, '>');
    }

    return status;
}

static int32_t
markup_close(
    const struct d_pack_sink* _sink,
    const char*              _name
)
{
    int32_t status;

    status = doc_emit_text(_sink, "</");

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_text(_sink, _name);
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_char(_sink, '>');
    }

    return status;
}

// markup_wrapped
//   helper: <name attrs>escaped text</name>, the shape of every leaf.
static int32_t
markup_wrapped(
    const struct d_pack_sink*       _sink,
    const char*                    _name,
    const char*                    _text,
    const struct d_doc_attributes* _attrs
)
{
    int32_t status;

    status = markup_open(_sink, _name, _attrs);

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = markup_escape(_sink, _text);
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = markup_close(_sink, _name);
    }

    return status;
}

static int32_t
markup_write_line(
    const char*                    _text,
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    return markup_wrapped(&doc_state(_context)->sink, "paragraph",
                          _text, _attrs);
}

static int32_t
markup_begin_document(
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    return markup_open(&doc_state(_context)->sink, "document", _attrs);
}

static int32_t
markup_end_document(
    void* _context
)
{
    return markup_close(&doc_state(_context)->sink, "document");
}

static int32_t
markup_begin_element(
    const char*                    _name,
    const char*                    _text,
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    struct d_doc_text_state* state;
    int32_t                  status;

    state  = doc_state(_context);
    status = markup_open(&state->sink,
                         (_name && _name[0]) ? _name : "element",
                         _attrs);

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = markup_escape(&state->sink, _text);
    }

    ++state->element_depth;

    return status;
}

static int32_t
markup_end_element(
    const char* _name,
    void*       _context
)
{
    struct d_doc_text_state* state;

    state = doc_state(_context);

    if (state->element_depth > 0u)
    {
        --state->element_depth;
    }

    return markup_close(&state->sink,
                        (_name && _name[0]) ? _name : "element");
}

static int32_t
markup_heading(
    uint32_t                       _level,
    const char*                    _text,
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    struct d_doc_text_state* state;
    int32_t                  status;

    state  = doc_state(_context);
    status = doc_emit_text(&state->sink, "<heading level=\"");

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_uint(&state->sink, _level);
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_char(&state->sink, '"');
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = markup_attrs(&state->sink, _attrs);
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_char(&state->sink, '>');
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = markup_escape(&state->sink, _text);
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = markup_close(&state->sink, "heading");
    }

    return status;
}

static int32_t
markup_key_value(
    const char*                    _key,
    const char*                    _value,
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    struct d_doc_text_state* state;
    int32_t                  status;

    state  = doc_state(_context);
    status = doc_emit_text(&state->sink, "<key_value key=\"");

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = markup_escape(&state->sink, _key);
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_char(&state->sink, '"');
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = markup_attrs(&state->sink, _attrs);
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_char(&state->sink, '>');
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = markup_escape(&state->sink, _value);
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = markup_close(&state->sink, "key_value");
    }

    return status;
}

static int32_t
markup_rule(
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    struct d_doc_text_state* state;
    int32_t                  status;

    state  = doc_state(_context);
    status = doc_emit_text(&state->sink, "<rule");

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = markup_attrs(&state->sink, _attrs);
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_text(&state->sink, "/>");
    }

    return status;
}

static int32_t
markup_space(
    int32_t                        _millipoints,
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    struct d_doc_text_state* state;
    int32_t                  status;

    // the amount IS emitted now.  It was withheld while the field was a
    // double, because printing one identically in two languages is the open
    // floating-point question; as fixed point it is an integer and the
    // question does not arise
    state  = doc_state(_context);
    status = doc_emit_text(&state->sink, "<space millipoints=\"");

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_uint(&state->sink,
                                 (uint32_t)((_millipoints < 0) ? 0
                                                               : _millipoints));
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = markup_attrs(&state->sink, _attrs);
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_text(&state->sink, "\"/>");
    }

    return status;
}

static int32_t
markup_page_break(
    void* _context
)
{
    return doc_emit_text(&doc_state(_context)->sink, "<page_break/>");
}

static int32_t
markup_begin_list(
    int32_t                        _ordered,
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    struct d_doc_text_state* state;
    int32_t                  status;

    state  = doc_state(_context);
    status = doc_emit_text(&state->sink,
                             _ordered ? "<list ordered=\"true\""
                                      : "<list ordered=\"false\"");

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = markup_attrs(&state->sink, _attrs);
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_char(&state->sink, '>');
    }

    doc_push_list(state, _ordered);

    return status;
}

static int32_t
markup_end_list(
    void* _context
)
{
    struct d_doc_text_state* state;

    state = doc_state(_context);
    doc_pop_list(state);

    return markup_close(&state->sink, "list");
}

static int32_t
markup_begin_item(
    const char*                    _text,
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    struct d_doc_text_state* state;
    int32_t                  status;

    state  = doc_state(_context);
    status = markup_open(&state->sink, "item", _attrs);

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = markup_escape(&state->sink, _text);
    }

    return status;
}

static int32_t
markup_end_item(
    void* _context
)
{
    return markup_close(&doc_state(_context)->sink, "item");
}

static int32_t
markup_begin_table(
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    return markup_open(&doc_state(_context)->sink, "table", _attrs);
}

static int32_t
markup_begin_column_group(
    const char*                    _label,
    uint32_t                       _span,
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    struct d_doc_text_state* state;
    int32_t                  status;

    state  = doc_state(_context);
    status = doc_emit_text(&state->sink, "<column_group label=\"");

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = markup_escape(&state->sink, _label);
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_text(&state->sink, "\" span=\"");
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_uint(&state->sink, _span);
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_char(&state->sink, '"');
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = markup_attrs(&state->sink, _attrs);
    }

    if (status == (int32_t)D_DOC_RENDER_OK)
    {
        status = doc_emit_char(&state->sink, '>');
    }

    return status;
}

static int32_t
markup_end_column_group(
    void* _context
)
{
    return markup_close(&doc_state(_context)->sink, "column_group");
}

static int32_t
markup_column(
    const char*                    _header,
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    return markup_wrapped(&doc_state(_context)->sink, "column",
                          _header, _attrs);
}

static int32_t
markup_begin_row(
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    return markup_open(&doc_state(_context)->sink, "row", _attrs);
}

static int32_t
markup_cell(
    const char*                    _text,
    const struct d_doc_attributes* _attrs,
    void*                          _context
)
{
    return markup_wrapped(&doc_state(_context)->sink, "cell", _text, _attrs);
}

static int32_t
markup_end_row(
    void* _context
)
{
    return markup_close(&doc_state(_context)->sink, "row");
}

static int32_t
markup_end_table(
    void* _context
)
{
    return markup_close(&doc_state(_context)->sink, "table");
}

struct d_doc_render_algebra
d_doc_markup_algebra(
    struct d_doc_text_state* _state
)
{
    struct d_doc_render_algebra algebra;

    algebra.context        = _state;

    // omnivorous: it emits every hint, so it declares no reads set rather
    // than claiming a false one
    algebra.reads          = NULL;
    algebra.read_count     = 0u;
    algebra.reserved       = 0u;

    algebra.begin_document = markup_begin_document;
    algebra.end_document   = markup_end_document;
    algebra.begin_element  = markup_begin_element;
    algebra.end_element    = markup_end_element;
    algebra.heading        = markup_heading;
    algebra.paragraph      = NULL;   // default funnels to markup_write_line
    algebra.key_value      = markup_key_value;
    algebra.rule           = markup_rule;
    algebra.space          = markup_space;
    algebra.page_break     = markup_page_break;
    algebra.begin_list     = markup_begin_list;
    algebra.begin_item     = markup_begin_item;
    algebra.end_item       = markup_end_item;
    algebra.end_list       = markup_end_list;
    algebra.begin_table    = markup_begin_table;
    algebra.begin_column_group = markup_begin_column_group;
    algebra.end_column_group   = markup_end_column_group;
    algebra.column         = markup_column;
    algebra.begin_row      = markup_begin_row;
    algebra.cell           = markup_cell;
    algebra.end_row        = markup_end_row;
    algebra.end_table      = markup_end_table;
    algebra.write_line     = markup_write_line;

    return algebra;
}


// ===========================================================================
// V.     convenience
// ===========================================================================

/*
d_doc_text_state_init
  Zeroes a dialect state and binds it to a sink.

Parameter(s):
  _state: the state.
  _sink:  where the bytes go.
Return:
  none.
*/
void
d_doc_text_state_init(
    struct d_doc_text_state* _state,
    struct d_pack_sink       _sink
)
{
    uint32_t index;

    if (!_state)
    {
        return;
    }

    _state->sink          = _sink;
    _state->list_depth    = 0u;
    _state->ordered_bits  = 0u;
    _state->element_depth = 0u;
    _state->table_columns = 0u;
    _state->table_cell    = 0u;
    _state->header_bytes  = 0u;
    _state->in_table      = 0;
    _state->saw_row       = 0;
    _state->table_buffer  = NULL;

    for (index = 0u; index < D_INTERNAL_DOC_MAX_LIST_DEPTH; ++index)
    {
        _state->counters[index] = 0u;
    }

    for (index = 0u; index < D_INTERNAL_DOC_MAX_TABLE_COLUMNS; ++index)
    {
        _state->column_align[index] = (uint8_t)D_DOC_ALIGN_LEFT;
    }

    return;
}

/*
d_doc_render_block_to_buffer
  As d_doc_render_to_buffer, without the document frame -- what a fixture uses
to check one constructor in isolation.

Parameter(s):
  as d_doc_render_to_buffer, with _block any node of block sort.
Return:
  as d_doc_render_to_buffer.
*/
enum d_doc_render_status
d_doc_render_block_to_buffer(
    const struct d_doc_node* _block,
    struct d_doc_render_algebra (*_dialect)(struct d_doc_text_state*),
    char*                    _buffer,
    size_t                   _capacity,
    size_t*                  _out_used
)
{
    struct d_pack_buffer_sink   bounded;
    struct d_doc_text_state     state;
    struct d_doc_render_algebra algebra;
    enum d_doc_render_status    status;

    if ( (!_dialect) ||
         (!_buffer)  ||
         (_capacity == 0u) )
    {
        return D_DOC_RENDER_SINK_ERROR;
    }

    d_pack_buffer_sink_init(&bounded, _buffer, _capacity - 1u);
    d_doc_text_state_init(&state, d_pack_sink_from_buffer(&bounded));

    algebra = _dialect(&state);
    status  = d_doc_render_block(_block, &algebra);

    _buffer[bounded.written] = '\0';

    if (_out_used)
    {
        (*_out_used) = bounded.written;
    }

    //   THE OVERFLOW FLAG IS THE ONLY PLACE THE TRUNCATION IS RECORDED, and
    // until relay 79 nothing read it.  Two correct contracts that did not
    // compose: d_internal_buffer_write ACCEPTS every byte and sets `overflow`,
    // documented as "the caller checks `overflow` rather than the return", so
    // that one failed pass can report the exact size to grow to.  d_sink_emit
    // therefore always answers 1, and document_text's writer maps only a 0 onto
    // SINK_FULL.  The signal was produced and dropped.
    //   MEASURED: a 56-byte paragraph into an 8-, 16- or 32-byte buffer
    // returned D_DOC_RENDER_OK with used=1 -- a newline, the text gone.  That
    // is the "document silently missing content" failure this tier names in
    // three separate comments, reachable through its own one-shot entry point.
    //   SINK_FULL is MECHANICAL, so a caller learns the tree was fine and the
    // buffer was not, and `bounded.needed` says how big to make it.
    if (bounded.overflow && (status == D_DOC_RENDER_OK))
    {
        return D_DOC_RENDER_SINK_FULL;
    }

    return status;
}

/*
d_doc_render_to_buffer
  Renders a document through one dialect into a caller-owned buffer.  The
whole of a text render in one call, and the shape both language faces use, so
that a parity fixture is one line on each side.

Parameter(s):
  _root:      the document.
  _dialect:   one of d_doc_plain_algebra / _markdown_ / _markup_.
  _buffer:    caller-owned storage.
  _capacity:  its size, including the terminator.
  _out_used:  bytes written, excluding the terminator; may be NULL.
Return:
  D_DOC_RENDER_OK, or the first non-OK status.  On SINK_FULL the buffer holds
a valid, truncated prefix.
*/
enum d_doc_render_status
d_doc_render_to_buffer(
    const struct d_doc_node* _root,
    struct d_doc_render_algebra (*_dialect)(struct d_doc_text_state*),
    char*                    _buffer,
    size_t                   _capacity,
    size_t*                  _out_used
)
{
    struct d_pack_buffer_sink   bounded;
    struct d_doc_text_state     state;
    struct d_doc_render_algebra algebra;
    enum d_doc_render_status    status;

    if ( (!_dialect) ||
         (!_buffer)  ||
         (_capacity == 0u) )
    {
        return D_DOC_RENDER_SINK_ERROR;
    }

    // one byte is held back so the result is a valid C string.  d_pack_sink
    // is length-authoritative and does not terminate -- correct for a codec,
    // and not what a text caller wants, so the terminator is added here
    // rather than pushed into the shared sink
    d_pack_buffer_sink_init(&bounded, _buffer, _capacity - 1u);
    d_doc_text_state_init(&state, d_pack_sink_from_buffer(&bounded));

    algebra = _dialect(&state);
    status  = d_doc_render(_root, &algebra);

    _buffer[bounded.written] = '\0';

    if (_out_used)
    {
        (*_out_used) = bounded.written;
    }

    //   Same repair as d_doc_render_block_to_buffer above, and for the same
    // reason: the buffer sink records truncation in `overflow` and nothing read
    // it, so a document that did not fit reported OK.  See that function for
    // the measurement and the two contracts involved.
    if (bounded.overflow && (status == D_DOC_RENDER_OK))
    {
        return D_DOC_RENDER_SINK_FULL;
    }

    return status;
}
