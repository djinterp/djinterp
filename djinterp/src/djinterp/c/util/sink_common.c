/*******************************************************************************
* djinterp [c]                                                     sink_common.c
*
*   The spans and the sink.  Nothing here allocates, opens anything, or knows
* what the bytes mean -- which is what lets every producer in the framework
* share one destination abstraction instead of inventing its own.
*
*
* path:      /src/djinterp/c/util/sink_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.30
*                                                            revised: 2026.11.05
*******************************************************************************/
#include "../../../../inc/djinterp/c/util/sink_common.h"


// std
// c
#include <string.h>


// =============================================================================
// I.   SPANS
// =============================================================================

/*
d_pack_text_from_cstr
  A text span over a NUL-terminated string.  The only function in this file
that consults a terminator.
*/
struct dj_pack_text
d_pack_text_from_cstr(
    const char* _cstr
)
{
    struct dj_pack_text t;

    t.data   = _cstr;
    t.length = (_cstr != NULL) ? strlen(_cstr) : (size_t)0;

    return t;
}

/*
d_pack_text_from_span
  A text span over an explicit pointer and length.
*/
struct dj_pack_text
d_pack_text_from_span(
    const char* _data,
    size_t      _length
)
{
    struct dj_pack_text t;

    t.data   = _data;
    t.length = (_data != NULL) ? _length : (size_t)0;

    return t;
}

/*
d_pack_bytes_from_span
  A byte span over an explicit pointer and size.
*/
struct dj_pack_bytes
d_pack_bytes_from_span(
    const void* _data,
    size_t      _size
)
{
    struct dj_pack_bytes b;

    b.data = _data;
    b.size = (_data != NULL) ? _size : (size_t)0;

    return b;
}

/*
d_pack_text_equal
  Whether two text spans carry the same bytes.

  Length first, then memcmp -- never strcmp, because a span may contain an
interior NUL and may not be terminated at all.
*/
int
d_pack_text_equal(
    struct dj_pack_text _a,
    struct dj_pack_text _b
)
{
    if (_a.length != _b.length)
    {
        return 0;
    }
    if (_a.length == 0u)
    {
        return 1;
    }
    if ( (_a.data == NULL) ||
         (_b.data == NULL) )
    {
        return 0;
    }

    return (memcmp(_a.data, _b.data, _a.length) == 0) ? 1 : 0;
}

/*
d_pack_text_is_empty
  Whether a span carries no bytes.
*/
int
d_pack_text_is_empty(
    struct dj_pack_text _text
)
{
    return ( (_text.data == NULL) ||
             (_text.length == 0u) ) ? 1 : 0;
}



// =============================================================================
// II.  SINKS
// =============================================================================

/*
d_sink_emit
  Push bytes to a sink, reporting only whether the sink took all of them.

  Deliberately status-FREE.  Compression and archiving each have their own
result vocabulary, and a shared sink has no business knowing either; each maps
a 0 here onto its own "the sink refused" status in one line.  A short write is
a failure rather than a partial success, because a producer that cannot rewind
has no meaningful way to continue from one.
*/
int
d_sink_emit(
    struct dj_pack_sink _sink,
    const void*         _data,
    size_t              _size
)
{
    if (_sink.write == NULL)
    {
        return 0;
    }

    if (_size == 0u)
    {
        return 1;
    }

    return (_sink.write(_sink.context, _data, _size) == _size) ? 1 : 0;
}

/**
 * @brief The counting sink's write: accept everything, record the total.
 *
 * @param[in] _context  a struct dj_pack_counting_sink.
 * @param[in] _data     ignored.
 * @param[in] _size     the byte count to add.
 * @return _size always -- a counting sink never refuses.
 */
static size_t
d_internal_count_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    struct dj_pack_counting_sink* c = (struct dj_pack_counting_sink*)_context;

    (void)_data;

    if (c != NULL)
    {
        c->total += _size;
    }

    return _size;
}

/**
 * @brief The buffer sink's write: copy what fits, and keep counting what does
 *        not.
 *
 * @param[in] _context  a struct dj_pack_buffer_sink.
 * @param[in] _data     the bytes offered.
 * @param[in] _size     how many.
 * @return _size always; the caller checks `overflow` rather than the return.
 *
 * A short write would abort the transform, so this ACCEPTS every byte and
 * records the overflow instead.  That is what lets one failed pass report the
 * exact size to grow to: the encoder runs to completion, the sink knows the
 * full requirement, and the caller retries once rather than measuring
 * separately.
 */
static size_t
d_internal_buffer_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    struct dj_pack_buffer_sink* b = (struct dj_pack_buffer_sink*)_context;

    if (b == NULL)
    {
        return 0;
    }

    b->needed += _size;

    if ( (b->buffer != NULL) &&
         ((b->written + _size) <= b->capacity) )
    {
        if (_size != 0u)
        {
            memcpy(b->buffer + b->written, _data, _size);
        }

        b->written += _size;
    }
    else if (_size != 0u)
    {
        b->overflow = 1;
    }

    return _size;
}

/*
d_pack_counting_sink_init
  Reset a counting sink to zero.
*/
void
d_pack_counting_sink_init(
    struct dj_pack_counting_sink* _counter
)
{
    if (_counter != NULL)
    {
        _counter->total = 0u;
    }

    return;
}

/*
d_pack_buffer_sink_init
  Point a buffer sink at caller-owned memory.
*/
void
d_pack_buffer_sink_init(
    struct dj_pack_buffer_sink* _sink,
    void*                       _buffer,
    size_t                      _capacity
)
{
    if (_sink == NULL)
    {
        return;
    }

    _sink->buffer   = (unsigned char*)_buffer;
    _sink->capacity = _capacity;
    _sink->written  = 0u;
    _sink->needed   = 0u;
    _sink->overflow = 0;

    return;
}

/*
d_pack_sink_from_counter
  A sink driving a counting context.
*/
struct dj_pack_sink
d_pack_sink_from_counter(
    struct dj_pack_counting_sink* _counter
)
{
    struct dj_pack_sink s;

    s.write   = &d_internal_count_write;
    s.context = _counter;

    return s;
}

/*
d_pack_sink_from_buffer
  A sink driving a fixed-buffer context.
*/
struct dj_pack_sink
d_pack_sink_from_buffer(
    struct dj_pack_buffer_sink* _buffer
)
{
    struct dj_pack_sink s;

    s.write   = &d_internal_buffer_write;
    s.context = _buffer;

    return s;
}
