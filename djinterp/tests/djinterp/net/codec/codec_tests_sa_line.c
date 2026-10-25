/*******************************************************************************
* djinterp [net]                                           codec_tests_sa_line.c
*
* Standalone tests of codec_line.h: lines found and split, CR LF and bare LF
* alike, the checks for line breaks, and the line buffer -- each of its
* operations, then streams of generated lines read through it in pieces of
* every size, against the lines they were made of.
*
*
* path:      /tests/djinterp/net/codec/codec_tests_sa_line.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.12
*******************************************************************************/
#include "./codec_tests_sa.h"  // the suite
// std
#include <string.h>  // memcmp, memcpy, strlen
// djinterp
#include "../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text


// D_TESTS_CODEC_LINE_STREAM
//   constant: the bytes a generated stream may take.
#define D_TESTS_CODEC_LINE_STREAM 4096u

// D_TESTS_CODEC_LINE_COUNT
//   constant: the lines a generated stream may hold.
#define D_TESTS_CODEC_LINE_COUNT 256u

// d_tests_codec_line_stream
//   struct: a generated stream and the lines it was made of: where each
// line's text starts, its length, and whether CR LF ended it.
struct d_tests_codec_line_stream
{
    char   bytes[D_TESTS_CODEC_LINE_STREAM];
    size_t size;
    size_t starts[D_TESTS_CODEC_LINE_COUNT];
    size_t lengths[D_TESTS_CODEC_LINE_COUNT];
    bool   crlf[D_TESTS_CODEC_LINE_COUNT];
    size_t count;
};

/*
d_tests_codec_line_text
  A span over a NUL-terminated string.
*/
static struct d_pack_text
d_tests_codec_line_text(
    const char* _text
)
{
    const struct d_pack_text text = { _text,
                                      strlen(_text) };

    return text;
}

/*
d_tests_codec_line_is
  Whether `_line` is exactly `_expected`, `_size` bytes with its terminator,
ended by CR LF when `_crlf`.
*/
static bool
d_tests_codec_line_is(
    const struct d_net_line* _line,
    const char*              _expected,
    size_t                   _size,
    bool                     _crlf
)
{
    return ( (_line->text.length == strlen(_expected)) &&
             (memcmp(_line->text.data,
                     _expected,
                     _line->text.length) == 0)         &&
             (_line->size == _size)                    &&
             (_line->crlf == _crlf) );
}

/*
d_tests_codec_found
  Whether d_net_line_find finds `_expected` in `_text`, searching from
`_from`.
*/
static bool
d_tests_codec_found(
    const char* _text,
    size_t      _from,
    const char* _expected,
    size_t      _size,
    bool        _crlf
)
{
    struct d_net_line line = { { NULL, 0u }, 0u, false };

    return ( (d_net_line_find(d_tests_codec_line_text(_text),
                              _from,
                              &line)) &&
             (d_tests_codec_line_is(&line,
                                    _expected,
                                    _size,
                                    _crlf)) );
}

/*
d_tests_sa_codec_line_find
  Tests the following:
  - an LF ends a line, and a CR just before it belongs to the terminator
  - the CR counts though it lies before where the search starts
  - no LF, or nothing left to search, finds nothing and leaves the line alone
  - a missing argument, or a span NULL but not empty, finds nothing
  It runs past 75 lines because its checks share one line and its broken
spans.
*/
bool
d_tests_sa_codec_line_find(
    struct d_test_counter* _counter
)
{
    const char* const        TEST   = "line finding";
    const struct d_pack_text broken = { NULL,
                                        3u };
    struct d_net_line        line   = { { "x", 1u }, 9u, true };
    bool                     result = true;

    result = d_tests_net_assert(
                 ( (d_tests_codec_found("abc\r\ndef",
                                        0u,
                                        "abc",
                                        5u,
                                        true))  &&
                   (d_tests_codec_found("abc\ndef",
                                        0u,
                                        "abc",
                                        4u,
                                        false)) &&
                   (d_tests_codec_found("\r\n",
                                        0u,
                                        "",
                                        2u,
                                        true))  &&
                   (d_tests_codec_found("\n",
                                        0u,
                                        "",
                                        1u,
                                        false)) &&
                   (d_tests_codec_found("a\rb\n",
                                        0u,
                                        "a\rb",
                                        4u,
                                        false)) ),
                 TEST,
                 "an LF ends a line; a CR before it is the terminator's",
                 _counter) && result;
    result = d_tests_net_assert(
                 ( (d_tests_codec_found("ab\r\n",
                                        3u,
                                        "ab",
                                        4u,
                                        true)) &&
                   (d_tests_codec_found("ab\r\n",
                                        2u,
                                        "ab",
                                        4u,
                                        true)) ),
                 TEST,
                 "a CR before the search's start still counts",
                 _counter) && result;
    result = d_tests_net_assert(
                 ( (!d_net_line_find(d_tests_codec_line_text("abc"),
                                     0u,
                                     &line)) &&
                   (!d_net_line_find(d_tests_codec_line_text("ab\n"),
                                     3u,
                                     &line)) &&
                   (!d_net_line_find(d_tests_codec_line_text("ab\n"),
                                     9u,
                                     &line)) &&
                   (line.size == 9u)         &&
                   (line.crlf) ),
                 TEST,
                 "no LF, or nothing to search: nothing found",
                 _counter) && result;
    result = d_tests_net_assert(
                 ( (!d_net_line_find(broken,
                                     0u,
                                     &line)) &&
                   (!d_net_line_find(d_tests_codec_line_text("a\n"),
                                     0u,
                                     NULL)) ),
                 TEST,
                 "a broken span or no line: nothing found",
                 _counter) && result;

    return result;
}

/*
d_tests_codec_splits_to
  Whether d_net_line_next splits `_length` bytes at `_text` into exactly the
`_count` lines `_lines` lists, then finds no more.
*/
static bool
d_tests_codec_splits_to(
    const char*        _text,
    size_t             _length,
    const char* const* _lines,
    size_t             _count
)
{
    struct d_pack_text cursor = { _text,
                                  _length };
    struct d_pack_text line   = { NULL,
                                  0u };

    // each line, in order
    for (size_t i = 0u; i < _count; i++)
    {
        if ( (!d_net_line_next(&cursor,
                               &line))          ||
             (line.length != strlen(_lines[i])) ||
             (memcmp(line.data,
                     _lines[i],
                     line.length) != 0) )
        {
            return false;
        }
    }

    return ( (!d_net_line_next(&cursor,
                               &line)) &&
             (cursor.length == 0u)     &&
             (cursor.data == _text + _length) );
}

/*
d_tests_sa_codec_line_next
  Tests the following:
  - CR LF and bare LF split alike, and the last line needs no LF
  - a CR that ends the last line goes with it; one before a CR LF stays
  - a trailing LF starts no line; two LFs make an empty line between
  - an empty or broken cursor, or a missing argument, gives no line
*/
bool
d_tests_sa_codec_line_next(
    struct d_test_counter* _counter
)
{
    const char* const        TEST     = "line splitting";
    static const char* const MIXED[]  = { "a", "b", "c" };
    static const char* const ONE[]    = { "a" };
    static const char* const EMPTY[]  = { "", "" };
    static const char* const KEPT[]   = { "a\r", "" };
    struct d_pack_text       none     = { NULL,
                                          4u };
    struct d_pack_text       line     = { NULL,
                                          0u };
    bool                     result   = true;

    result = d_tests_net_assert(
                 ( (d_tests_codec_splits_to("a\r\nb\nc",
                                            6u,
                                            MIXED,
                                            3u)) &&
                   (d_tests_codec_splits_to("a\r",
                                            2u,
                                            ONE,
                                            1u)) &&
                   (d_tests_codec_splits_to("a\r\n",
                                            3u,
                                            ONE,
                                            1u)) &&
                   (d_tests_codec_splits_to("a\n",
                                            2u,
                                            ONE,
                                            1u)) ),
                 TEST,
                 "CR LF and LF alike, the last line's LF optional",
                 _counter) && result;
    result = d_tests_net_assert(
                 ( (d_tests_codec_splits_to("\n\n",
                                            2u,
                                            EMPTY,
                                            2u)) &&
                   (d_tests_codec_splits_to("a\r\r\n\r",
                                            5u,
                                            KEPT,
                                            2u)) ),
                 TEST,
                 "empty lines, and a CR kept inside a line",
                 _counter) && result;
    result = d_tests_net_assert(
                 ( (d_tests_codec_splits_to("",
                                            0u,
                                            NULL,
                                            0u)) &&
                   (!d_net_line_next(&none,
                                     &line))     &&
                   (!d_net_line_next(NULL,
                                     &line))     &&
                   (!d_net_line_next(&none,
                                     NULL)) ),
                 TEST,
                 "no line from nothing",
                 _counter) && result;

    return result;
}

/*
d_tests_codec_chomps_to
  Whether d_net_line_chomp narrows `_text` to its first `_length` bytes.
*/
static bool
d_tests_codec_chomps_to(
    const char* _text,
    size_t      _length
)
{
    const struct d_pack_text text    = d_tests_codec_line_text(_text);
    const struct d_pack_text chomped = d_net_line_chomp(text);

    return ( (chomped.data == text.data) &&
             (chomped.length == _length) );
}

/*
d_tests_sa_codec_line_checks
  Tests the following:
  - chomping drops every CR and LF at the end and nothing else
  - a break is a CR or an LF; clean text holds no CR, LF, or NUL
  - an empty span is clean and breakless; a broken one is neither
*/
bool
d_tests_sa_codec_line_checks(
    struct d_test_counter* _counter
)
{
    const char* const        TEST   = "line safety";
    const struct d_pack_text nul    = { "a\0b",
                                        3u };
    const struct d_pack_text none   = { NULL,
                                        0u };
    const struct d_pack_text broken = { NULL,
                                        2u };
    bool                     result = true;

    result = d_tests_net_assert(
                 ( (d_tests_codec_chomps_to("abc\r\n\r\n",
                                            3u)) &&
                   (d_tests_codec_chomps_to("ab \r\n",
                                            3u)) &&
                   (d_tests_codec_chomps_to("\r\n",
                                            0u)) &&
                   (d_tests_codec_chomps_to("a\rb",
                                            3u)) &&
                   (d_net_line_chomp(broken).length == 2u) ),
                 TEST,
                 "chomping: the line-ending bytes at the end",
                 _counter) && result;
    result = d_tests_net_assert(
                 ( (!d_net_line_has_break(d_tests_codec_line_text("plain"))) &&
                   (d_net_line_has_break(d_tests_codec_line_text("a\rb")))   &&
                   (d_net_line_has_break(d_tests_codec_line_text("a\nb")))   &&
                   (!d_net_line_has_break(nul))                              &&
                   (!d_net_line_has_break(none))                             &&
                   (d_net_line_has_break(broken)) ),
                 TEST,
                 "a break is a CR or an LF",
                 _counter) && result;
    result = d_tests_net_assert(
                 ( (d_net_line_is_clean(d_tests_codec_line_text("plain"))) &&
                   (!d_net_line_is_clean(d_tests_codec_line_text("a\rb"))) &&
                   (!d_net_line_is_clean(d_tests_codec_line_text("a\nb"))) &&
                   (!d_net_line_is_clean(nul))                             &&
                   (d_net_line_is_clean(none))                             &&
                   (!d_net_line_is_clean(broken)) ),
                 TEST,
                 "clean text holds no CR, LF, or NUL",
                 _counter) && result;

    return result;
}

/*
d_tests_codec_line_put
  Copies `_text` into a buffer's room, reporting whether it all fit.
*/
static bool
d_tests_codec_line_put(
    struct d_net_line_buffer* _buffer,
    char*                     _storage,
    const char*               _text
)
{
    size_t       room   = 0u;
    char* const  free   = d_net_line_buffer_room(_buffer,
                                                 _storage,
                                                 &room);
    const size_t length = strlen(_text);

    // what does not fit is not put
    if (length > room)
    {
        return false;
    }

    memcpy(free,
           _text,
           length);
    d_net_line_buffer_commit(_buffer,
                             length);

    return true;
}

/*
d_tests_sa_codec_line_buffer
  Tests the following:
  - a line arrives in pieces, its CR in one read and its LF in the next
  - a buffer full of one line says so, and spilling empties it
  - a CR at the end stays through a spill, to meet its LF
  - bytes held move to the front when they reach the end, searched or not
  It runs past 75 lines because one line buffer carries its state from each
check to the next.
*/
bool
d_tests_sa_codec_line_buffer(
    struct d_test_counter* _counter
)
{
    const char* const        TEST    = "line buffer";
    char                     storage[8] = { 0 };
    struct d_net_line_buffer buffer  = { 0u, 0u, 0u, 0u };
    struct d_net_line        line    = { { NULL, 0u }, 0u, false };
    struct d_pack_text       spilled = { NULL,
                                         0u };
    bool                     result  = true;

    d_net_line_buffer_init(&buffer,
                           sizeof(storage));
    result = d_tests_net_assert(
                 ( (d_net_line_buffer_find(&buffer,
                                           storage,
                                           &line) == D_NET_LINE_MORE)  &&
                   (d_tests_codec_line_put(&buffer,
                                           storage,
                                           "ab\r"))                    &&
                   (d_net_line_buffer_find(&buffer,
                                           storage,
                                           &line) == D_NET_LINE_MORE)  &&
                   (buffer.scanned == 3u)                              &&
                   (d_tests_codec_line_put(&buffer,
                                           storage,
                                           "\nc"))                     &&
                   (d_net_line_buffer_find(&buffer,
                                           storage,
                                           &line) == D_NET_LINE_FOUND) &&
                   (d_tests_codec_line_is(&line,
                                          "ab",
                                          4u,
                                          true)) ),
                 TEST,
                 "a line in two reads, its CR LF split between them",
                 _counter) && result;
    d_net_line_buffer_consume(&buffer,
                              line.size);
    result = d_tests_net_assert(
                 ( (d_net_line_buffer_pending(&buffer) == 1u)         &&
                   (d_tests_codec_line_put(&buffer,
                                           storage,
                                           "123"))                    &&
                   (d_net_line_buffer_find(&buffer,
                                           storage,
                                           &line) == D_NET_LINE_MORE) &&
                   (d_tests_codec_line_put(&buffer,
                                           storage,
                                           "4567"))                   &&
                   (d_net_line_buffer_find(&buffer,
                                           storage,
                                           &line) == D_NET_LINE_FULL) ),
                 TEST,
                 "a buffer full of one line, moved to the front first",
                 _counter) && result;
    spilled = d_net_line_buffer_spill(&buffer,
                                      storage);
    result = d_tests_net_assert(
                 ( (spilled.length == 8u)                     &&
                   (memcmp(spilled.data,
                           "c1234567",
                           8u) == 0)                          &&
                   (d_net_line_buffer_pending(&buffer) == 0u) &&
                   (d_tests_codec_line_put(&buffer,
                                           storage,
                                           "xyz\r")) ),
                 TEST,
                 "a spill takes everything held",
                 _counter) && result;
    spilled = d_net_line_buffer_spill(&buffer,
                                      storage);
    result = d_tests_net_assert(
                 ( (spilled.length == 3u)                              &&
                   (d_net_line_buffer_pending(&buffer) == 1u)          &&
                   (d_tests_codec_line_put(&buffer,
                                           storage,
                                           "\n"))                      &&
                   (d_net_line_buffer_find(&buffer,
                                           storage,
                                           &line) == D_NET_LINE_FOUND) &&
                   (d_tests_codec_line_is(&line,
                                          "",
                                          2u,
                                          true)) ),
                 TEST,
                 "a CR at the end stays, to meet its LF",
                 _counter) && result;

    return result;
}

/*
d_tests_sa_codec_line_room
  Tests the following:
  - bytes held move to the front once they reach the storage's end, and the
    search goes on from where it stopped
  - a commit past the room, or a consume past what is held, stops at it
  - a missing buffer or storage holds nothing and has no room
*/
bool
d_tests_sa_codec_line_room(
    struct d_test_counter* _counter
)
{
    const char* const        TEST   = "line buffer room";
    char                     storage[8] = { 0 };
    struct d_net_line_buffer buffer = { 0u, 0u, 0u, 0u };
    struct d_net_line        line   = { { NULL, 0u }, 0u, false };
    size_t                   room   = 9u;
    bool                     result = true;

    d_net_line_buffer_init(&buffer,
                           sizeof(storage));
    (void)d_tests_codec_line_put(&buffer,
                                 storage,
                                 "abcd\nefg");
    (void)d_net_line_buffer_find(&buffer,
                                 storage,
                                 &line);
    d_net_line_buffer_consume(&buffer,
                              line.size);
    result = d_tests_net_assert(
                 ( (d_net_line_buffer_find(&buffer,
                                           storage,
                                           &line) == D_NET_LINE_MORE)  &&
                   (d_net_line_buffer_room(&buffer,
                                           storage,
                                           &room) == storage + 3)      &&
                   (room == 5u)                                        &&
                   (memcmp(storage,
                           "efg",
                           3u) == 0)                                   &&
                   (d_tests_codec_line_put(&buffer,
                                           storage,
                                           "h\n"))                     &&
                   (d_net_line_buffer_find(&buffer,
                                           storage,
                                           &line) == D_NET_LINE_FOUND) &&
                   (d_tests_codec_line_is(&line,
                                          "efgh",
                                          5u,
                                          false)) ),
                 TEST,
                 "the bytes held move to the front at the end",
                 _counter) && result;
    d_net_line_buffer_commit(&buffer,
                             100u);
    result = d_tests_net_assert(
                 ( (d_net_line_buffer_pending(&buffer) == 8u) &&
                   (d_net_line_buffer_held(&buffer,
                                           storage).data == storage) ),
                 TEST,
                 "a commit stops at the room",
                 _counter) && result;
    d_net_line_buffer_consume(&buffer,
                              100u);
    result = d_tests_net_assert(
                 ( (d_net_line_buffer_pending(&buffer) == 0u)         &&
                   (buffer.start == 0u)                               &&
                   (d_net_line_buffer_find(NULL,
                                           storage,
                                           &line) == D_NET_LINE_FULL) &&
                   (d_net_line_buffer_room(&buffer,
                                           NULL,
                                           &room) == NULL)            &&
                   (d_net_line_buffer_spill(NULL,
                                            storage).length == 0u)    &&
                   (d_net_line_buffer_held(NULL,
                                           storage).length == 0u)     &&
                   (d_net_line_buffer_pending(NULL) == 0u) ),
                 TEST,
                 "a consume stops at what is held; nothing missing works",
                 _counter) && result;

    return result;
}

/*
d_tests_sa_codec_line_edges
  Tests the following:
  - the answers a search gives keep their values: FOUND 0, MORE 1, FULL 2
  - a break or a NUL that is the text's first byte counts
  - an empty text finds nothing, and neither does a search from its end
  - an empty line is found first after init, and first after the buffer
    emptied
  - a buffer given no storage, or nowhere to put the line, reports itself full
  It runs past 75 lines because its edge cases share one set of spans.
*/
bool
d_tests_sa_codec_line_edges(
    struct d_test_counter* _counter
)
{
    const char* const        TEST       = "line edges";
    const struct d_pack_text lead_cr    = { "\rab",
                                            3u };
    const struct d_pack_text lead_lf    = { "\nab",
                                            3u };
    const struct d_pack_text lead_nul   = { "\0ab",
                                            3u };
    const struct d_pack_text none       = { NULL,
                                            0u };
    char                     storage[8] = { 0 };
    struct d_net_line_buffer buffer     = { 0u, 0u, 0u, 0u };
    struct d_net_line        line       = { { NULL, 0u }, 0u, false };
    bool                     result     = true;

    result = d_tests_net_assert(
                 ( ((int)D_NET_LINE_FOUND == 0)     &&
                   ((int)D_NET_LINE_MORE == 1)      &&
                   ((int)D_NET_LINE_FULL == 2)      &&
                   (d_net_line_has_break(lead_cr))  &&
                   (d_net_line_has_break(lead_lf))  &&
                   (!d_net_line_is_clean(lead_cr))  &&
                   (!d_net_line_is_clean(lead_nul)) &&
                   (!d_net_line_find(none,
                                     0u,
                                     &line))        &&
                   (!d_net_line_find(lead_lf,
                                     3u,
                                     &line)) ),
                 TEST,
                 "values kept; first bytes count; nothing from nothing",
                 _counter) && result;
    d_net_line_buffer_init(&buffer,
                           sizeof(storage));
    result = d_tests_net_assert(
                 ( (d_tests_codec_line_put(&buffer,
                                           storage,
                                           "\na\n"))                   &&
                   (d_net_line_buffer_find(&buffer,
                                           storage,
                                           &line) == D_NET_LINE_FOUND) &&
                   (d_tests_codec_line_is(&line,
                                          "",
                                          1u,
                                          false)) ),
                 TEST,
                 "an empty line, first after init",
                 _counter) && result;
    d_net_line_buffer_consume(&buffer,
                              line.size);
    (void)d_net_line_buffer_find(&buffer,
                                 storage,
                                 &line);
    d_net_line_buffer_consume(&buffer,
                              line.size);
    result = d_tests_net_assert(
                 ( (d_tests_codec_line_put(&buffer,
                                           storage,
                                           "\n"))                      &&
                   (d_net_line_buffer_find(&buffer,
                                           storage,
                                           &line) == D_NET_LINE_FOUND) &&
                   (d_tests_codec_line_is(&line,
                                          "",
                                          1u,
                                          false))                      &&
                   (d_net_line_buffer_find(&buffer,
                                           NULL,
                                           &line) == D_NET_LINE_FULL)  &&
                   (d_net_line_buffer_find(&buffer,
                                           storage,
                                           NULL) == D_NET_LINE_FULL) ),
                 TEST,
                 "an empty line, first after emptying; parts missing",
                 _counter) && result;

    return result;
}

/*
d_tests_sa_codec_line_held
  Tests the following:
  - after a line is consumed, the bytes held are what followed it, in place
  - room moves nothing that has not reached the storage's end
  - no storage, or nowhere to put the count, gives no room and no count
  - a consume past what is held, after a line, empties the buffer
*/
bool
d_tests_sa_codec_line_held(
    struct d_test_counter* _counter
)
{
    const char* const        TEST       = "line buffer held";
    char                     storage[8] = { 0 };
    struct d_net_line_buffer buffer     = { 0u, 0u, 0u, 0u };
    struct d_net_line        line       = { { NULL, 0u }, 0u, false };
    struct d_pack_text       held       = { NULL,
                                            0u };
    size_t                   room       = 99u;
    bool                     result     = true;

    d_net_line_buffer_init(&buffer,
                           sizeof(storage));
    (void)d_tests_codec_line_put(&buffer,
                                 storage,
                                 "ab\ncd");
    (void)d_net_line_buffer_find(&buffer,
                                 storage,
                                 &line);
    d_net_line_buffer_consume(&buffer,
                              line.size);
    held   = d_net_line_buffer_held(&buffer,
                                    storage);
    result = d_tests_net_assert(
                 ( (held.data == storage + 3)                     &&
                   (held.length == 2u)                            &&
                   (d_net_line_buffer_room(&buffer,
                                           NULL,
                                           &room) == NULL)        &&
                   (d_net_line_buffer_room(&buffer,
                                           storage,
                                           NULL) == NULL)         &&
                   (room == 99u)                                  &&
                   (d_net_line_buffer_room(&buffer,
                                           storage,
                                           &room) == storage + 5) &&
                   (room == 3u)                                   &&
                   (buffer.start == 3u) ),
                 TEST,
                 "what follows a line, in place; room moves nothing",
                 _counter) && result;
    d_net_line_buffer_consume(&buffer,
                              100u);
    result = d_tests_net_assert(
                 ( (d_net_line_buffer_pending(&buffer) == 0u) &&
                   (buffer.start == 0u)                       &&
                   (buffer.end == 0u) ),
                 TEST,
                 "a consume past what is held empties the buffer",
                 _counter) && result;

    return result;
}

/*
d_tests_sa_codec_line_spill
  Tests the following:
  - a spill after a line was consumed takes what follows it, in place
  - a spill of nothing, or of a lone CR, takes nothing
  - a search after a spill that kept a CR, more bytes read first, finds the
    CR LF it was waiting for
*/
bool
d_tests_sa_codec_line_spill(
    struct d_test_counter* _counter
)
{
    const char* const        TEST       = "line spills";
    char                     storage[8] = { 0 };
    struct d_net_line_buffer buffer     = { 0u, 0u, 0u, 0u };
    struct d_net_line        line       = { { NULL, 0u }, 0u, false };
    struct d_pack_text       taken      = { NULL,
                                            0u };
    bool                     result     = true;

    d_net_line_buffer_init(&buffer,
                           sizeof(storage));
    (void)d_tests_codec_line_put(&buffer,
                                 storage,
                                 "ab\ncd");
    (void)d_net_line_buffer_find(&buffer,
                                 storage,
                                 &line);
    d_net_line_buffer_consume(&buffer,
                              line.size);
    (void)d_net_line_buffer_find(&buffer,
                                 storage,
                                 &line);
    taken  = d_net_line_buffer_spill(&buffer,
                                     storage);
    result = d_tests_net_assert(
                 ( (taken.data == storage + 3)                        &&
                   (taken.length == 2u)                               &&
                   (d_net_line_buffer_pending(&buffer) == 0u)         &&
                   (d_net_line_buffer_spill(&buffer,
                                            storage).length == 0u)    &&
                   (d_tests_codec_line_put(&buffer,
                                           storage,
                                           "\r"))                     &&
                   (d_net_line_buffer_find(&buffer,
                                           storage,
                                           &line) == D_NET_LINE_MORE) &&
                   (d_net_line_buffer_spill(&buffer,
                                            storage).length == 0u)    &&
                   (d_net_line_buffer_pending(&buffer) == 1u) ),
                 TEST,
                 "what follows a line; nothing, and a lone CR, stay",
                 _counter) && result;
    d_net_line_buffer_clear(&buffer);
    (void)d_tests_codec_line_put(&buffer,
                                 storage,
                                 "abc\r");
    (void)d_net_line_buffer_find(&buffer,
                                 storage,
                                 &line);
    taken  = d_net_line_buffer_spill(&buffer,
                                     storage);
    result = d_tests_net_assert(
                 ( (taken.length == 3u)                                &&
                   (d_tests_codec_line_put(&buffer,
                                           storage,
                                           "\n"))                      &&
                   (d_net_line_buffer_find(&buffer,
                                           storage,
                                           &line) == D_NET_LINE_FOUND) &&
                   (d_tests_codec_line_is(&line,
                                          "",
                                          2u,
                                          true)) ),
                 TEST,
                 "a kept CR meets its LF read after the spill",
                 _counter) && result;

    return result;
}

/*
d_tests_codec_line_random
  The next number of a fixed sequence, 0 to `_bound` - 1.
*/
static size_t
d_tests_codec_line_random(
    unsigned long* _state,
    size_t         _bound
)
{
    *_state = (*_state * 1103515245ul) + 12345ul;

    return (size_t)((*_state >> 8) % (unsigned long)_bound);
}

/*
d_tests_codec_line_generate
  Fills `_stream` with lines of 0 to 20 bytes, any byte but LF among them,
each ended by CR LF or a bare LF. A line ended by a bare LF never ends in
CR, since that CR would read as the terminator's.
*/
static void
d_tests_codec_line_generate(
    struct d_tests_codec_line_stream* _stream,
    unsigned long*                    _state
)
{
    static const char BYTES[] = "ab \t\r\0:Z";

    _stream->size  = 0u;
    _stream->count = 0u;

    // lines while there is room for the longest
    while ( (_stream->count < D_TESTS_CODEC_LINE_COUNT) &&
            (_stream->size + 22u <= D_TESTS_CODEC_LINE_STREAM) )
    {
        const size_t length = d_tests_codec_line_random(_state,
                                                        21u);
        const bool   crlf   = (d_tests_codec_line_random(_state,
                                                         2u) == 0u);

        _stream->starts[_stream->count]  = _stream->size;
        _stream->lengths[_stream->count] = length;
        _stream->crlf[_stream->count]    = crlf;

        // the line's bytes; a CR that would end a bare-LF line is an 'r'
        for (size_t i = 0u; i < length; i++)
        {
            const char c    = BYTES[d_tests_codec_line_random(_state,
                                                              8u)];
            const bool last = (i + 1u == length);

            _stream->bytes[_stream->size++] = ( (c == '\r') &&
                                                (!crlf)     &&
                                                (last) ) ? 'r'
                                                         : c;
        }

        // the terminator
        if (crlf)
        {
            _stream->bytes[_stream->size++] = '\r';
        }

        _stream->bytes[_stream->size++] = '\n';
        _stream->count++;
    }

    return;
}

// d_tests_codec_line_reader
//   struct: a stream being read back: the buffer and its storage, the line
// being assembled, and how far the stream has been fed and read.
struct d_tests_codec_line_reader
{
    struct d_net_line_buffer                buffer;
    char                                    storage[64];
    char                                    line[64];
    size_t                                  length;
    size_t                                  index;
    size_t                                  fed;
    const struct d_tests_codec_line_stream* stream;
    unsigned long*                          state;
};

/*
d_tests_codec_line_append
  Adds a piece to the line being assembled.
*/
static void
d_tests_codec_line_append(
    struct d_tests_codec_line_reader* _reader,
    struct d_pack_text                _piece
)
{
    memcpy(_reader->line + _reader->length,
           _piece.data,
           _piece.length);
    _reader->length += _piece.length;

    return;
}

/*
d_tests_codec_line_take
  Finishes the line being assembled with `_found`, and reports whether it is
the next line the stream was made of.
*/
static bool
d_tests_codec_line_take(
    struct d_tests_codec_line_reader* _reader,
    const struct d_net_line*          _found
)
{
    const struct d_tests_codec_line_stream* stream = _reader->stream;
    const size_t                            index  = _reader->index;

    d_tests_codec_line_append(_reader,
                              _found->text);
    d_net_line_buffer_consume(&_reader->buffer,
                              _found->size);
    _reader->index++;

    const size_t length = _reader->length;

    _reader->length = 0u;

    return ( (index < stream->count)               &&
             (stream->lengths[index] == length)    &&
             (stream->crlf[index] == _found->crlf) &&
             (memcmp(stream->bytes + stream->starts[index],
                     _reader->line,
                     length) == 0) );
}

/*
d_tests_codec_line_feed
  Reads the stream's next piece into the buffer's room: 1 byte to all the
room, at random, and no further than the stream goes.
*/
static void
d_tests_codec_line_feed(
    struct d_tests_codec_line_reader* _reader
)
{
    size_t       room = 0u;
    char* const  free = d_net_line_buffer_room(&_reader->buffer,
                                               _reader->storage,
                                               &room);
    const size_t left = _reader->stream->size - _reader->fed;
    const size_t want = 1u + d_tests_codec_line_random(_reader->state,
                                                       room);
    const size_t take = (want < left) ? want
                                      : left;

    memcpy(free,
           _reader->stream->bytes + _reader->fed,
           take);
    d_net_line_buffer_commit(&_reader->buffer,
                             take);
    _reader->fed += take;

    return;
}

/*
d_tests_codec_line_read
  Reads `_stream` through a buffer of `_capacity` bytes, assembling each line
from its spills and its end, and reports whether every line came back as it
was made, with nothing left over.
*/
static bool
d_tests_codec_line_read(
    const struct d_tests_codec_line_stream* _stream,
    size_t                                  _capacity,
    unsigned long*                          _state
)
{
    static struct d_tests_codec_line_reader reader = { .length = 0u };
    struct d_net_line                       found  = { .size = 0u };
    bool                                    all    = true;

    reader.length = 0u;
    reader.index  = 0u;
    reader.fed    = 0u;
    reader.stream = _stream;
    reader.state  = _state;
    d_net_line_buffer_init(&reader.buffer,
                           _capacity);

    // until every line is back: a whole line, a long one's spill, or a read
    while ( (all) &&
            (reader.index < _stream->count) )
    {
        const enum d_net_line_found what =
            d_net_line_buffer_find(&reader.buffer,
                                   reader.storage,
                                   &found);

        if (what == D_NET_LINE_FOUND)
        {
            all = d_tests_codec_line_take(&reader,
                                          &found);
        }
        else if (what == D_NET_LINE_FULL)
        {
            d_tests_codec_line_append(&reader,
                                      d_net_line_buffer_spill(&reader.buffer,
                                                              reader.storage));
        }
        else
        {
            d_tests_codec_line_feed(&reader);
        }
    }

    return ( (all) &&
             (d_net_line_buffer_pending(&reader.buffer) == 0u) );
}

/*
d_tests_sa_codec_line_stream
  Tests the following:
  - generated streams of lines, CR LF and bare LF mixed and every byte but
    LF inside them, come back line for line through buffers of 2, 3, 5, 8,
    21 and 64 bytes, read in pieces of random sizes; a line longer than the
    buffer comes back through spills
*/
bool
d_tests_sa_codec_line_stream(
    struct d_test_counter* _counter
)
{
    static const size_t                     CAPACITIES[] = { 2u, 3u, 5u,
                                                             8u, 21u, 64u };
    static struct d_tests_codec_line_stream stream       = { .size = 0u };
    const char* const                       TEST         = "lines in pieces";
    const size_t                            count        =
        sizeof(CAPACITIES) / sizeof(CAPACITIES[0]);
    unsigned long                           state        = 20261012ul;
    bool                                    all          = true;

    // forty streams, each through every capacity
    for (size_t round = 0u; round < 40u; round++)
    {
        d_tests_codec_line_generate(&stream,
                                    &state);

        for (size_t i = 0u; i < count; i++)
        {
            all = d_tests_codec_line_read(&stream,
                                          CAPACITIES[i],
                                          &state) && all;
        }
    }

    return d_tests_net_assert(all,
                              TEST,
                              "every line back as it was made",
                              _counter);
}
