/*******************************************************************************
* djinterp [net]                                         imap_tests_sa_support.c
*
* The IMAP suite's helpers: a sink that keeps what it is given up to a limit
* and refuses the rest, and comparisons of what it holds, and of spans, with
* C strings.
*
*
* path:      /tests/djinterp/net/imap/imap_tests_sa_support.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.18
*                                                            revised: 2026.10.18
*******************************************************************************/
#include "./imap_tests_sa.h"  // the suite
// std
#include <string.h>  // memcmp, memcpy, strlen


/*
d_tests_imap_capture_write
  A capture's sink: keeps what fits under the limit, and reports taking only
that much, which ends the writer's output.
*/
static size_t
d_tests_imap_capture_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    struct d_tests_imap_capture* capture = _context;
    const size_t                 room    = capture->limit - capture->length;
    const size_t                 taken   = (_size < room) ? _size : room;

    // nothing to copy from an empty write
    if (taken > 0u)
    {
        memcpy(capture->data + capture->length,
               _data,
               taken);
        capture->length += taken;
    }

    return taken;
}

/*
d_tests_imap_capture_init
  Empties the capture and holds its limit to the size of its storage.
*/
void
d_tests_imap_capture_init(
    struct d_tests_imap_capture* _capture,
    size_t                       _limit
)
{
    _capture->length = 0u;
    _capture->limit  = (_limit < sizeof(_capture->data))
                           ? _limit
                           : sizeof(_capture->data);

    return;
}

/*
d_tests_imap_capture_sink
  A sink over the capture.
*/
struct d_pack_sink
d_tests_imap_capture_sink(
    struct d_tests_imap_capture* _capture
)
{
    const struct d_pack_sink sink = { d_tests_imap_capture_write,
                                      _capture };

    return sink;
}

/*
d_tests_imap_capture_is
  Whether the capture holds exactly the octets of `_text`.
*/
bool
d_tests_imap_capture_is(
    const struct d_tests_imap_capture* _capture,
    const char*                        _text
)
{
    const size_t length = strlen(_text);

    return ( (_capture->length == length) &&
             (memcmp(_capture->data,
                     _text,
                     length) == 0) );
}

/*
d_tests_imap_span_is
  Whether a span holds exactly the octets of `_text`. A span without data,
and a NULL `_text`, are both empty.
*/
bool
d_tests_imap_span_is(
    struct d_pack_text _span,
    const char*        _text
)
{
    const size_t length = (_text != NULL) ? strlen(_text) : 0u;

    // two empty spans need no comparison
    if (length == 0u)
    {
        return ( (_span.data == NULL) ||
                 (_span.length == 0u) );
    }

    return ( (_span.data != NULL)       &&
             (_span.length == length)   &&
             (memcmp(_span.data,
                     _text,
                     length) == 0) );
}
