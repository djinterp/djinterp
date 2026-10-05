/*******************************************************************************
* djinterp [c]                                                     interpolate.c
*
* TBA
*
*
* path:      /src/djinterp/c/functional/interpolate.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/functional/interpolate.h"


/*
d_internal_interp_is_key_byte
  Reports whether a byte may appear in a sigil-syntax key. A key runs to the
first byte that is not alphanumeric or underscore, which is what lets `$name.`
end at the period without the syntax needing a closing delimiter.

Parameter(s):
  _byte: the byte to classify.
Return:
  A boolean value corresponding to whether the byte continues a key.
*/
static bool
d_internal_interp_is_key_byte
(
    char _byte
)
{
    // classified directly rather than through <ctype.h>, whose functions are
    // locale-dependent and would make the syntax vary by environment
    if ( ((_byte >= 'a') && (_byte <= 'z')) ||
         ((_byte >= 'A') && (_byte <= 'Z')) ||
         ((_byte >= '0') && (_byte <= '9')) ||
         (_byte == '_')                     )
    {
        return true;
    }

    return false;
}

/*
d_internal_interp_next_brace
  The brace scanner's pull step: yields one literal run or one key per call.
An unterminated opening delimiter is emitted as literal text rather than
treated as an error, so a malformed template degrades to its own source.

Parameter(s):
  _state: the `d_interp_scanner` to advance.
  _out:   destination for one `d_interp_piece`.
Return:
  A boolean value corresponding to whether a piece was produced.
*/
static bool
d_internal_interp_next_brace
(
    void* _state,
    void* _out
)
{
    struct d_interp_scanner* state;
    struct d_interp_piece*   piece;
    const char*              run;
    const char*              close;

    // a NULL state or destination is a caller error
    if ( (!_state) ||
         (!_out)   )
    {
        return false;
    }

    state = (struct d_interp_scanner*)_state;
    piece = (struct d_interp_piece*)_out;

    // exhaustion is the ordinary end of the text
    if (state->cursor >= state->end)
    {
        return false;
    }

    // a key begins here only if the delimiter is closed somewhere ahead
    if (*(state->cursor) == state->open)
    {
        close = state->cursor + 1;

        while ( (close < state->end)        &&
                (*close != state->close)    )
        {
            close = close + 1;
        }

        if (close < state->end)
        {
            piece->kind   = D_INTERP_KEY;
            piece->begin  = state->cursor + 1;
            piece->length = (size_t)(close - (state->cursor + 1));

            state->cursor = close + 1;

            return true;
        }
    }

    // otherwise this is a literal run up to the next opening delimiter
    run = state->cursor;

    if (*run == state->open)
    {
        run = run + 1;
    }

    while ( (run < state->end)          &&
            (*run != state->open)       )
    {
        run = run + 1;
    }

    piece->kind   = D_INTERP_LITERAL;
    piece->begin  = state->cursor;
    piece->length = (size_t)(run - state->cursor);

    state->cursor = run;

    return true;
}

/*
d_internal_interp_next_sigil
  The sigil scanner's pull step. A sigil not followed by a key byte is emitted
as literal text, which is how an escaped or bare sigil survives a pass.

Parameter(s):
  _state: the `d_interp_scanner` to advance.
  _out:   destination for one `d_interp_piece`.
Return:
  A boolean value corresponding to whether a piece was produced.
*/
static bool
d_internal_interp_next_sigil
(
    void* _state,
    void* _out
)
{
    struct d_interp_scanner* state;
    struct d_interp_piece*   piece;
    const char*              run;

    // a NULL state or destination is a caller error
    if ( (!_state) ||
         (!_out)   )
    {
        return false;
    }

    state = (struct d_interp_scanner*)_state;
    piece = (struct d_interp_piece*)_out;

    // exhaustion is the ordinary end of the text
    if (state->cursor >= state->end)
    {
        return false;
    }

    // a key begins at a sigil only when at least one key byte follows it
    if (*(state->cursor) == state->sigil)
    {
        run = state->cursor + 1;

        while ( (run < state->end)                          &&
                (d_internal_interp_is_key_byte(*run))       )
        {
            run = run + 1;
        }

        if (run > (state->cursor + 1))
        {
            piece->kind   = D_INTERP_KEY;
            piece->begin  = state->cursor + 1;
            piece->length = (size_t)(run - (state->cursor + 1));

            state->cursor = run;

            return true;
        }
    }

    // otherwise this is a literal run up to the next sigil
    run = state->cursor;

    if (*run == state->sigil)
    {
        run = run + 1;
    }

    while ( (run < state->end)          &&
            (*run != state->sigil)      )
    {
        run = run + 1;
    }

    piece->kind   = D_INTERP_LITERAL;
    piece->begin  = state->cursor;
    piece->length = (size_t)(run - state->cursor);

    state->cursor = run;

    return true;
}


/*
d_interp_brace_scanner
  Builds a scanner over brace syntax, `{key}`, as a producer of pieces.

Parameter(s):
  _state:  caller-owned scan state; must outlive the producer.
  _text:   the template text; borrowed, and must outlive the producer.
  _length: its length in bytes.
Return:
  A producer of `d_interp_piece`; the empty producer if the parameters were
unusable.
*/
struct d_producer
d_interp_brace_scanner
(
    struct d_interp_scanner* _state,
    const char*              _text,
    size_t                   _length
)
{
    // an unusable request degrades to the empty producer
    if ( (!_state) ||
         (!_text)  )
    {
        return d_producer_empty(sizeof(struct d_interp_piece));
    }

    _state->cursor = _text;
    _state->end    = _text + _length;
    _state->open   = '{';
    _state->close  = '}';
    _state->sigil  = 0;

    return d_producer_make(&d_internal_interp_next_brace,
                           _state,
                           sizeof(struct d_interp_piece));
}

/*
d_interp_sigil_scanner
  Builds a scanner over sigil syntax, `$key`, as a producer of pieces.

Parameter(s):
  _state:  caller-owned scan state; must outlive the producer.
  _text:   the template text; borrowed.
  _length: its length in bytes.
  _sigil:  the byte introducing a key; must be non-zero.
Return:
  A producer of `d_interp_piece`; the empty producer if the parameters were
unusable.
*/
struct d_producer
d_interp_sigil_scanner
(
    struct d_interp_scanner* _state,
    const char*              _text,
    size_t                   _length,
    char                     _sigil
)
{
    // a zero sigil would match the string terminator, so it is refused
    if ( (!_state)     ||
         (!_text)      ||
         (_sigil == 0) )
    {
        return d_producer_empty(sizeof(struct d_interp_piece));
    }

    _state->cursor = _text;
    _state->end    = _text + _length;
    _state->open   = 0;
    _state->close  = 0;
    _state->sigil  = _sigil;

    return d_producer_make(&d_internal_interp_next_sigil,
                           _state,
                           sizeof(struct d_interp_piece));
}

/*
d_interp_frame_make
  Binds a resolver to its context.

Parameter(s):
  _resolve: the lookup; may be NULL, yielding a frame that always misses.
  _context: context forwarded to `_resolve`; may be NULL.
Return:
  A frame by value.
*/
struct d_interp_frame
d_interp_frame_make
(
    fn_resolver _resolve,
    void*       _context
)
{
    struct d_interp_frame frame;

    frame.resolve = _resolve;
    frame.context = _context;

    return frame;
}

/*
d_interp_chain_init
  Composes an ordered set of frames.

Parameter(s):
  _chain:  the chain to initialise.
  _frames: caller-owned frame array; may be NULL only if `_count` is 0.
  _count:  how many frames.
Return:
  A boolean value corresponding to whether the chain was initialised.
*/
bool
d_interp_chain_init
(
    struct d_interp_chain*       _chain,
    const struct d_interp_frame* _frames,
    size_t                       _count
)
{
    // a chain claiming frames must have them
    if ( (!_chain)                       ||
         ( (!_frames) && (_count > 0) )  )
    {
        return false;
    }

    _chain->frames = _frames;
    _chain->count  = _count;

    return true;
}

/*
d_interp_chain_resolve
  Tries each frame in order and stops at the first hit. This is `alt` over the
resolution: a later frame is consulted only when every earlier one has missed,
which is what makes an N-frame chain cost no more than the frame that answers.

Parameter(s):
  _chain:      the frames to try.
  _key:        the placeholder key.
  _key_length: its length in bytes.
  _out_value:  receives the replacement on a hit.
  _out_length: receives its length on a hit.
Return:
  A boolean value corresponding to either:
  - true, if some frame answered, or
  - false, if every frame missed. A miss is not an error.
*/
bool
d_interp_chain_resolve
(
    const struct d_interp_chain* _chain,
    const char*                  _key,
    size_t                       _key_length,
    const char**                 _out_value,
    size_t*                      _out_length
)
{
    size_t index;

    // an unusable request misses
    if ( (!_chain)      ||
         (!_key)        ||
         (!_out_value)  ||
         (!_out_length) )
    {
        return false;
    }

    // the first frame to answer wins; the rest are never consulted
    for (index = 0; index < _chain->count; index = index + 1)
    {
        if (!_chain->frames[index].resolve)
        {
            continue;
        }

        if (_chain->frames[index].resolve(_key,
                                          _key_length,
                                          _out_value,
                                          _out_length,
                                          _chain->frames[index].context))
        {
            return true;
        }
    }

    return false;
}

/*
d_interp_sink_init
  Initialises a bounded output buffer.

Parameter(s):
  _sink:     the sink to initialise.
  _buffer:   caller-owned storage; may be NULL only if `_capacity` is 0.
  _capacity: its size in bytes.
Return:
  A boolean value corresponding to whether the sink was initialised.
*/
bool
d_interp_sink_init
(
    struct d_interp_sink* _sink,
    char*                 _buffer,
    size_t                _capacity
)
{
    // an unusable request leaves the sink untouched
    if ( (!_sink)                         ||
         ( (!_buffer) && (_capacity > 0) ) )
    {
        return false;
    }

    _sink->buffer   = _buffer;
    _sink->capacity = _capacity;
    _sink->written  = 0;
    _sink->overflow = 0;

    return true;
}

/*
d_interp_sink_append
  Appends bytes, taking as many as fit and counting the rest. A full sink is a
reported condition rather than a corruption or a silent truncation.

Parameter(s):
  _sink:   the sink to append to.
  _bytes:  the bytes to append.
  _length: how many.
Return:
  A boolean value corresponding to either:
  - true, if every byte fit, or
  - false, if some or all overflowed.
*/
bool
d_interp_sink_append
(
    struct d_interp_sink* _sink,
    const char*           _bytes,
    size_t                _length
)
{
    size_t room;
    size_t taken;

    // an unusable request appends nothing
    if ( (!_sink)  ||
         (!_bytes) )
    {
        return false;
    }

    // an empty run always fits
    if (_length == 0)
    {
        return true;
    }

    room  = _sink->capacity - _sink->written;
    taken = (_length < room) ? _length : room;

    // take what fits, and count what does not
    if (taken > 0)
    {
        memcpy(_sink->buffer + _sink->written, _bytes, taken);

        _sink->written = _sink->written + taken;
    }

    if (taken < _length)
    {
        _sink->overflow = _sink->overflow + (_length - taken);

        return false;
    }

    return true;
}

/*
d_interpolate
  The engine: one left-to-right pass, pulling pieces from the scanner, putting
each key through the chain, and appending to the sink.
  A key that no frame answers is written back exactly as it was scanned,
delimiters included, so the output remains a valid template. That is what makes
PARTIAL interpolation well-defined, and it is a reported outcome rather than a
failure.

Parameter(s):
  _scanner: the producer of pieces.
  _chain:   the resolver frames; may be NULL, in which case every key misses.
  _sink:    the output buffer.
  _report:  receives what the pass observed; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the pass completed with nothing overflowing, or
  - false, if the sink overflowed or the parameters were unusable.
*/
bool
d_interpolate
(
    struct d_producer*           _scanner,
    const struct d_interp_chain* _chain,
    struct d_interp_sink*        _sink,
    struct d_interp_report*      _report
)
{
    struct d_interp_piece  piece;
    struct d_interp_report report;
    const char*            value;
    size_t                 length;

    report.pieces   = 0;
    report.resolved = 0;
    report.missed   = 0;
    report.written  = 0;
    report.overflow = 0;

    // an unusable request interpolates nothing
    if ( (!d_producer_is_valid(_scanner)) ||
         (!_sink)                         )
    {
        if (_report)
        {
            *_report = report;
        }

        return false;
    }

    // one pass, one piece at a time, with no intermediate structure
    while (_scanner->next(_scanner->state, &piece))
    {
        report.pieces = report.pieces + 1;

        // a literal run is copied through untouched
        if (piece.kind == D_INTERP_LITERAL)
        {
            d_interp_sink_append(_sink, piece.begin, piece.length);

            continue;
        }

        value  = NULL;
        length = 0;

        // a hit substitutes; a miss puts the placeholder back verbatim
        if (d_interp_chain_resolve(_chain,
                                   piece.begin,
                                   piece.length,
                                   &value,
                                   &length))
        {
            report.resolved = report.resolved + 1;

            d_interp_sink_append(_sink, value, length);
        }
        else
        {
            report.missed = report.missed + 1;

            d_interp_sink_append(_sink, piece.begin - 1, 1);
            d_interp_sink_append(_sink, piece.begin, piece.length);

            // sigil syntax has no closing delimiter to restore
            if (piece.begin[-1] == '{')
            {
                d_interp_sink_append(_sink, "}", 1);
            }
        }
    }

    report.written  = _sink->written;
    report.overflow = _sink->overflow;

    // handing back the report is optional
    if (_report)
    {
        *_report = report;
    }

    return (report.overflow == 0);
}

/*
d_interpolate_text
  Interpolates brace-syntax text into a caller-owned buffer, which is the
common case with the scanner and sink supplied for the caller.

Parameter(s):
  _text:     the template text.
  _length:   its length in bytes.
  _chain:    the resolver frames.
  _out:      the destination buffer.
  _capacity: its size in bytes.
  _report:   receives what the pass observed; may be NULL.
Return:
  A boolean value corresponding to whether the pass completed without
overflowing.
*/
bool
d_interpolate_text
(
    const char*                  _text,
    size_t                       _length,
    const struct d_interp_chain* _chain,
    char*                        _out,
    size_t                       _capacity,
    struct d_interp_report*      _report
)
{
    struct d_interp_scanner state;
    struct d_interp_sink    sink;
    struct d_producer       scanner;

    // an unusable request interpolates nothing
    if (!d_interp_sink_init(&sink, _out, _capacity))
    {
        return false;
    }

    scanner = d_interp_brace_scanner(&state, _text, _length);

    return d_interpolate(&scanner, _chain, &sink, _report);
}
