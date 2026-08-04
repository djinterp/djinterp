/******************************************************************************
* djinterp [utility]                                            sink_common.c
*
*   The spans and the sink.  Nothing here allocates, opens anything, or knows
* what the bytes mean -- which is what lets every producer in the framework
* share one destination abstraction instead of inventing its own.
*
* path:      /src/djinterp/core/util/sink_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.30
******************************************************************************/

// c
#include <string.h>
// djinterp
#include "./sink_common.h"


// =============================================================================
// I.   SPANS
// =============================================================================

/*
d_pack_text_from_cstr
  A text span over a NUL-terminated string.  The only function in this file
that consults a terminator.

Parameter(s):
  _cstr: the string; may be null, which yields the empty span.
Return:
  The span.
*/
struct d_pack_text
d_pack_text_from_cstr(
    const char* _cstr
)
{
    struct d_pack_text t;

    t.data   = _cstr;
    t.length = (_cstr != NULL) ? strlen(_cstr) : (size_t)0;

    return t;
}

/*
d_pack_text_from_span
  A text span over an explicit pointer and length.

Parameter(s):
  _data:   the bytes; may be null when _length is 0.
  _length: how many.
Return:
  The span.
*/
struct d_pack_text
d_pack_text_from_span(
    const char* _data,
    size_t      _length
)
{
    struct d_pack_text t;

    t.data   = _data;
    t.length = (_data != NULL) ? _length : (size_t)0;

    return t;
}

/*
d_pack_bytes_from_span
  A byte span over an explicit pointer and size.

Parameter(s):
  _data: the bytes; may be null when _size is 0.
  _size: how many.
Return:
  The span.
*/
struct d_pack_bytes
d_pack_bytes_from_span(
    const void* _data,
    size_t      _size
)
{
    struct d_pack_bytes b;

    b.data = _data;
    b.size = (_data != NULL) ? _size : (size_t)0;

    return b;
}

/*
d_pack_text_equal
  Whether two text spans carry the same bytes.

  Length first, then memcmp -- never strcmp, because a span may contain an
interior NUL and may not be terminated at all.

Parameter(s):
  _a: the first span.
  _b: the second span.
Return:
  1 when the two are byte-identical, 0 otherwise.  Two empty spans are equal
whatever their pointers.
*/
int
d_pack_text_equal(
    struct d_pack_text _a,
    struct d_pack_text _b
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

Parameter(s):
  _text: the span.
Return:
  1 when the span is null or zero-length, 0 otherwise.
*/
int
d_pack_text_is_empty(
    struct d_pack_text _text
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

Parameter(s):
  _sink: the destination.
  _data: the bytes.
  _size: how many.
Return:
  1 when the sink accepted every byte, 0 otherwise.  A zero-size write always
succeeds and never calls through.
*/
int
d_sink_emit(
    struct d_pack_sink _sink,
    const void*        _data,
    size_t             _size
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
