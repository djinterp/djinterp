/*******************************************************************************
* djinterp [net]                                                    codec_line.c
*
* Definitions for codec_line.h: lines found in a text and a text split into
* lines, the checks for line breaks, and the line buffer.
*
*
* path:      /src/djinterp/net/codec/codec_line.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.12
*******************************************************************************/
#include "../../../../inc/djinterp/net/codec/codec_line.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <string.h>   // memchr, memmove
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"          // framework root
#include "../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text


/*
d_internal_net_line_text_ok
  A span can be read when it has storage or is empty; a NULL span that claims
characters is a caller's mistake.
*/
D_STATIC bool
d_internal_net_line_text_ok(
    struct d_pack_text _text
)
{
    return ( (_text.data != NULL) ||
             (_text.length == 0u) );
}

/*
d_net_line_find
  memchr finds the LF; the CR is looked for only then, at the byte before
it, which may lie before `_from`: a CR that arrived in an earlier read than
its LF still makes the terminator CR LF.
*/
bool
d_net_line_find(
    struct d_pack_text _text,
    size_t             _from,
    struct d_net_line* _out
)
{
    // parameter validation, and bytes left to search
    if ( (_out == NULL)                        ||
         (!d_internal_net_line_text_ok(_text)) ||
         (_from >= _text.length) )
    {
        return false;
    }

    const char* const lf = memchr(_text.data + _from,
                                  '\n',
                                  _text.length - _from);

    // no LF has arrived
    if (lf == NULL)
    {
        return false;
    }

    const size_t length = (size_t)(lf - _text.data);
    const bool   crlf   = ( (length > 0u) &&
                            (_text.data[length - 1u] == '\r') );

    _out->text.data   = _text.data;
    _out->text.length = (crlf) ? (length - 1u)
                               : length;
    _out->size        = length + 1u;
    _out->crlf        = crlf;

    return true;
}

/*
d_net_line_next
  A line ended by its LF is d_net_line_find()'s; the last line, which needs
none, loses a CR that ends it all the same, so that "a\r" and "a\r\n" split
alike.
*/
bool
d_net_line_next(
    struct d_pack_text* _cursor,
    struct d_pack_text* _out_line
)
{
    // parameter validation; an exhausted cursor has no lines left
    if ( (_cursor == NULL)       ||
         (_out_line == NULL)     ||
         (_cursor->data == NULL) ||
         (_cursor->length == 0u) )
    {
        return false;
    }

    struct d_net_line line = { { NULL, 0u }, 0u, false };

    // the last line runs to the end of the text, a CR ending it dropped
    if (!d_net_line_find(*_cursor,
                         0u,
                         &line))
    {
        const bool cr = (_cursor->data[_cursor->length - 1u] == '\r');

        line.text.data   = _cursor->data;
        line.text.length = (cr) ? (_cursor->length - 1u)
                                : _cursor->length;
        line.size        = _cursor->length;
    }

    *_out_line       = line.text;
    _cursor->data   += line.size;
    _cursor->length -= line.size;

    return true;
}

/*
d_net_line_chomp
  Walks back over CR and LF bytes alone; a blank stops the walk.
*/
struct d_pack_text
d_net_line_chomp(
    struct d_pack_text _text
)
{
    struct d_pack_text result = _text;

    // a NULL span comes back as it went
    if (result.data == NULL)
    {
        return result;
    }

    // the line-ending bytes at the end, however many
    while ( (result.length > 0u) &&
            ( (result.data[result.length - 1u] == '\r') ||
              (result.data[result.length - 1u] == '\n') ) )
    {
        result.length--;
    }

    return result;
}

/*
d_net_line_has_break
  One pass over the text. A span that cannot be read is answered as though
it held a break, so a caller refusing breaks refuses it too.
*/
bool
d_net_line_has_break(
    struct d_pack_text _text
)
{
    // a span that cannot be read
    if (!d_internal_net_line_text_ok(_text))
    {
        return true;
    }

    // any CR or LF
    for (size_t i = 0u; i < _text.length; i++)
    {
        if ( (_text.data[i] == '\r') ||
             (_text.data[i] == '\n') )
        {
            return true;
        }
    }

    return false;
}

/*
d_net_line_is_clean
  One pass over the text, as d_net_line_has_break() makes, NUL included.
*/
bool
d_net_line_is_clean(
    struct d_pack_text _text
)
{
    // a span that cannot be read is not known to be clean
    if (!d_internal_net_line_text_ok(_text))
    {
        return false;
    }

    // no CR, LF, or NUL
    for (size_t i = 0u; i < _text.length; i++)
    {
        if ( (_text.data[i] == '\r') ||
             (_text.data[i] == '\n') ||
             (_text.data[i] == '\0') )
        {
            return false;
        }
    }

    return true;
}

/*
d_net_line_buffer_init
  Every offset 0: nothing held, nothing searched.
*/
void
d_net_line_buffer_init(
    struct d_net_line_buffer* _buffer,
    size_t                    _capacity
)
{
    // nothing to ready
    if (_buffer == NULL)
    {
        return;
    }

    _buffer->capacity = _capacity;
    _buffer->start    = 0u;
    _buffer->end      = 0u;
    _buffer->scanned  = 0u;

    return;
}

/*
d_net_line_buffer_find
  The search starts where the last one stopped, so however many reads a line
takes to arrive, each byte is searched once. A missing argument reports a
full buffer, which no caller reads more into.
*/
enum d_net_line_found
d_net_line_buffer_find(
    struct d_net_line_buffer* _buffer,
    const char*               _storage,
    struct d_net_line*        _out
)
{
    // parameter validation
    if ( (_buffer == NULL)  ||
         (_storage == NULL) ||
         (_out == NULL) )
    {
        return D_NET_LINE_FULL;
    }

    const size_t             held = _buffer->end - _buffer->start;
    const struct d_pack_text text = { _storage + _buffer->start,
                                      held };

    // a line, from the bytes not searched before
    if (d_net_line_find(text,
                        _buffer->scanned,
                        _out))
    {
        return D_NET_LINE_FOUND;
    }

    _buffer->scanned = held;

    return (held == _buffer->capacity) ? D_NET_LINE_FULL
                                       : D_NET_LINE_MORE;
}

/*
d_net_line_buffer_spill
  A CR at the end of what is held stays, since the next byte may be its LF:
taking it would make the line's CR LF a CR in the text and a bare LF. With
a capacity of at least 2, a full buffer always has something to give.
*/
struct d_pack_text
d_net_line_buffer_spill(
    struct d_net_line_buffer* _buffer,
    const char*               _storage
)
{
    struct d_pack_text taken = { _storage,
                                 0u };

    // parameter validation
    if ( (_buffer == NULL) ||
         (_storage == NULL) )
    {
        return taken;
    }

    size_t count = _buffer->end - _buffer->start;

    // a CR at the end may be the first half of a CR LF
    if ( (count > 0u) &&
         (_storage[_buffer->end - 1u] == '\r') )
    {
        count--;
    }

    taken.data   = _storage + _buffer->start;
    taken.length = count;

    d_net_line_buffer_consume(_buffer,
                              count);

    return taken;
}

/*
d_net_line_buffer_room
  The bytes held move to the front only when they have reached the end of the
storage, the one time it is needed; `scanned` counts from `start`, so it
still holds after the move.
*/
char*
d_net_line_buffer_room(
    struct d_net_line_buffer* _buffer,
    char*                     _storage,
    size_t*                   _room
)
{
    // parameter validation
    if ( (_buffer == NULL)  ||
         (_storage == NULL) ||
         (_room == NULL) )
    {
        return NULL;
    }

    // make room at the end, when the bytes held have reached it
    if ( (_buffer->end == _buffer->capacity) &&
         (_buffer->start > 0u) )
    {
        const size_t held = _buffer->end - _buffer->start;

        memmove(_storage,
                _storage + _buffer->start,
                held);
        _buffer->start = 0u;
        _buffer->end   = held;
    }

    *_room = _buffer->capacity - _buffer->end;

    return _storage + _buffer->end;
}

/*
d_net_line_buffer_commit
  A count past the room is held to it: a read cannot have put more there.
*/
void
d_net_line_buffer_commit(
    struct d_net_line_buffer* _buffer,
    size_t                    _count
)
{
    // nothing to add to
    if (_buffer == NULL)
    {
        return;
    }

    const size_t room = _buffer->capacity - _buffer->end;

    _buffer->end += (_count < room) ? _count
                                    : room;

    return;
}

/*
d_net_line_buffer_held
  The bytes from `start` to `end`.
*/
struct d_pack_text
d_net_line_buffer_held(
    const struct d_net_line_buffer* _buffer,
    const char*                     _storage
)
{
    struct d_pack_text held = { _storage,
                                0u };

    // nothing held without a buffer and its storage
    if ( (_buffer == NULL) ||
         (_storage == NULL) )
    {
        return held;
    }

    held.data   = _storage + _buffer->start;
    held.length = _buffer->end - _buffer->start;

    return held;
}

/*
d_net_line_buffer_pending
  The count d_net_line_buffer_held() gives.
*/
size_t
d_net_line_buffer_pending(
    const struct d_net_line_buffer* _buffer
)
{
    return (_buffer != NULL) ? (_buffer->end - _buffer->start)
                             : 0u;
}

/*
d_net_line_buffer_consume
  `scanned` shrinks with the bytes consumed, never past 0; a buffer emptied
starts again at the front of its storage.
*/
void
d_net_line_buffer_consume(
    struct d_net_line_buffer* _buffer,
    size_t                    _count
)
{
    // nothing to consume from
    if (_buffer == NULL)
    {
        return;
    }

    const size_t held  = _buffer->end - _buffer->start;
    const size_t count = (_count < held) ? _count
                                         : held;

    _buffer->start   += count;
    _buffer->scanned  = (_buffer->scanned > count) ? (_buffer->scanned - count)
                                                   : 0u;

    // empty: back to the front
    if (_buffer->start == _buffer->end)
    {
        d_net_line_buffer_clear(_buffer);
    }

    return;
}

/*
d_net_line_buffer_clear
  Every offset back to 0; the capacity stays.
*/
void
d_net_line_buffer_clear(
    struct d_net_line_buffer* _buffer
)
{
    // nothing to clear
    if (_buffer == NULL)
    {
        return;
    }

    _buffer->start   = 0u;
    _buffer->end     = 0u;
    _buffer->scanned = 0u;

    return;
}
