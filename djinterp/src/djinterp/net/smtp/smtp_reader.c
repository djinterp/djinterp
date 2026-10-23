/*******************************************************************************
* djinterp [net]                                                   smtp_reader.c
*
*   Definitions for smtp_reader.h: the SMTP line reader over a net.h
* connection, framed by net's line buffer.
*
*
* path:      /src/djinterp/net/smtp/smtp_reader.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "../../../../inc/djinterp/net/smtp/smtp_reader.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <string.h>   // memcpy
// djinterp
#include "../../../../inc/djinterp/net/codec/codec_line.h"  // d_net_line_*
#include "../../../../inc/djinterp/net/net_common.h"        // connections
#include "../../../../inc/djinterp/net/smtp/smtp_common.h"  // errors


/*
d_smtp_reader_init
  Forgets all an earlier session left the reader: what it held, the last
line's terminator, and the cause it kept. Over no connection it reads
nothing: d_smtp_reader_line refuses it.
*/
void
d_smtp_reader_init(
    struct d_smtp_reader*    _reader,
    struct d_net_connection* _connection
)
{
    // parameter validation first
    if (!_reader)
    {
        return;
    }

    _reader->connection = _connection;
    _reader->net_error  = D_NET_OK;
    _reader->crlf       = false;
    d_net_line_buffer_init(&_reader->lines,
                           sizeof(_reader->buffer));

    return;
}

// d_internal_smtp_line
//   struct: a line being assembled by the reader.
struct d_internal_smtp_line
{
    char*  data;
    size_t capacity;
    size_t used;
    bool   overflow;
};

/*
d_internal_smtp_keep
  Stores a piece of a line, keeping room for the NUL; what does not fit is
counted as overflow and dropped.
*/
static void
d_internal_smtp_keep(
    struct d_internal_smtp_line* _line,
    struct d_pack_text           _piece
)
{
    const size_t room  = _line->capacity - 1 - _line->used;
    const size_t taken = (_piece.length < room) ? _piece.length
                                                : room;

    // the part that fits
    if (taken > 0)
    {
        memcpy(_line->data + _line->used,
               _piece.data,
               taken);
        _line->used += taken;
    }

    _line->overflow = ( (_line->overflow) ||
                        (_piece.length > room) );

    return;
}

/*
d_internal_smtp_give_up
  Ends a read cut short by the connection: the part of the line held, all
but a CR that may have been the first half of its CR LF, goes into the line,
and the reader is left empty.
*/
static void
d_internal_smtp_give_up(
    struct d_smtp_reader*        _reader,
    struct d_internal_smtp_line* _line
)
{
    d_internal_smtp_keep(_line,
                         d_net_line_buffer_spill(&_reader->lines,
                                                 _reader->buffer));
    d_net_line_buffer_clear(&_reader->lines);

    return;
}

/*
d_internal_smtp_pull
  Reads once into the line buffer's room. Bytes that arrive are kept,
whatever came with them; a read of nothing is the end of the stream when it
reports nothing else, and the connection's failure, whose cause is kept,
when it does. net.h holds the connection to its contract: a read claiming
more than it had room for arrives here as nothing with UNKNOWN.
*/
static enum d_smtp_error
d_internal_smtp_pull(
    struct d_smtp_reader* _reader
)
{
    size_t                       room   = 0;
    char* const                  into   = d_net_line_buffer_room(
                                              &_reader->lines,
                                              _reader->buffer,
                                              &room);
    const struct d_net_io_result result =
        d_net_connection_read(_reader->connection,
                              into,
                              room);

    // bytes arrived, so the read succeeded
    if (result.count > 0)
    {
        d_net_line_buffer_commit(&_reader->lines,
                                 result.count);

        return D_SMTP_OK;
    }

    // nothing, and nothing wrong: the end of the stream
    if (result.error == D_NET_OK)
    {
        return D_SMTP_ERROR_CLOSED;
    }

    _reader->net_error = result.error;

    return d_smtp_error_from_net(result.error);
}

/*
d_internal_smtp_close
  Ends a line at the LF the line buffer found: its last piece kept, its bytes
consumed, the line terminated, and its terminator recorded.
*/
static enum d_smtp_error
d_internal_smtp_close(
    struct d_smtp_reader*        _reader,
    struct d_internal_smtp_line* _line,
    const struct d_net_line*     _found,
    size_t*                      _length
)
{
    d_internal_smtp_keep(_line,
                         _found->text);
    d_net_line_buffer_consume(&_reader->lines,
                              _found->size);
    _line->data[_line->used] = '\0';
    *_length                 = _line->used;
    _reader->crlf            = _found->crlf;

    return (_line->overflow) ? D_SMTP_ERROR_LINE_TOO_LONG : D_SMTP_OK;
}

/*
d_smtp_reader_line
  The line buffer finds the line; one longer than the buffer arrives in
spills, each all but a CR that may begin the line's CR LF, so a line that
exactly fills `_line` is never reported as overflowing because of its own
terminator. A read that fails, or finds the end of the stream, gives up what
was held of the line, which is terminated and reported.
*/
enum d_smtp_error
d_smtp_reader_line(
    struct d_smtp_reader* _reader,
    char*                 _line,
    size_t                _capacity,
    size_t*               _length
)
{
    // parameter validation first
    if ( (!_reader)             ||
         (!_reader->connection) ||
         (!_line)               ||
         (_capacity == 0)       ||
         (!_length) )
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    struct d_internal_smtp_line line = { .data     = _line,
                                         .capacity = _capacity,
                                         .used     = 0,
                                         .overflow = false };

    for (;;)
    {
        struct d_net_line           found = { { NULL, 0 }, 0, false };
        const enum d_net_line_found what  =
            d_net_line_buffer_find(&_reader->lines,
                                   _reader->buffer,
                                   &found);

        // the line's end
        if (what == D_NET_LINE_FOUND)
        {
            return d_internal_smtp_close(_reader,
                                         &line,
                                         &found,
                                         _length);
        }

        // a piece of a line longer than the buffer
        if (what == D_NET_LINE_FULL)
        {
            d_internal_smtp_keep(&line,
                                 d_net_line_buffer_spill(&_reader->lines,
                                                         _reader->buffer));

            continue;
        }

        const enum d_smtp_error result = d_internal_smtp_pull(_reader);

        // the connection failed, or its stream ended
        if (result != D_SMTP_OK)
        {
            d_internal_smtp_give_up(_reader,
                                    &line);
            _line[line.used] = '\0';
            *_length         = line.used;

            return result;
        }
    }
}

/*
d_smtp_reader_pending
  What was read but not yet consumed; nothing, for no reader.
*/
size_t
d_smtp_reader_pending(
    const struct d_smtp_reader* _reader
)
{
    return (_reader) ? d_net_line_buffer_pending(&_reader->lines)
                     : 0;
}

/*
d_smtp_reader_discard
  Drops what was read but not consumed -- what a peer pipelined before
STARTTLS, say -- and nothing else.
*/
void
d_smtp_reader_discard(
    struct d_smtp_reader* _reader
)
{
    // parameter validation first
    if (!_reader)
    {
        return;
    }

    d_net_line_buffer_clear(&_reader->lines);

    return;
}
