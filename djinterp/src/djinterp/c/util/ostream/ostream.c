/*******************************************************************************
* djinterp [c]                                                         ostream.c
*
*   The stream's slow path, lifecycle and state, the integer writers, the
* bridge back to a sink, and the destinations that need nothing but memory: a
* fixed array or a count, and the fan-out. Nothing here allocates.
*
*   ERRNO IS THE DESTINATION'S, AND ONLY ON FAILURE. errno is cleared before
* each call into a destination, so a failure the destination did not explain
* records 0 rather than a stale value, and it is put back afterwards: a caller
* sees errno change only when a call failed. This happens once per drain, never
* on the inline path.
*
*
* path:      /src/djinterp/c/util/ostream/ostream.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.07
*                                                            revised: 2026.11.05
*******************************************************************************/
#include "../../../../../inc/djinterp/c/util/ostream/ostream.h"
// std
#include <errno.h>   // errno, EINVAL, ENOSPC
#include <limits.h>  // CHAR_BIT
#include <stddef.h>  // offsetof, size_t
#include <string.h>  // memcpy, memchr, memset
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/c/util/sink_common.h"
                                                     // struct dj_pack_sink
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t, uint64_t


#if !defined(INT64_MAX)
    #error "ostream.c needs a 64-bit integer type; every C99 build has one"
#endif  // INT64_MAX


// D_INTERNAL_OSTREAM_CLOSED
//   constant: flags bit: d_ostream_close has run.
#define D_INTERNAL_OSTREAM_CLOSED  0x4000

// D_INTERNAL_OSTREAM_FLAGS
//   constant: the flag bits a caller may pass to d_ostream_init.
#define D_INTERNAL_OSTREAM_FLAGS                                               \
    (D_OSTREAM_OWNS_CONTEXT | D_OSTREAM_LINE_BUFFERED | D_OSTREAM_BINARY)

// D_INTERNAL_OSTREAM_HAS_SLOT
//   macro: whether a table of `_ops->size` bytes reaches the whole of `_slot`.
// The only way to ask that of a member by name, which is why it is a macro.
#define D_INTERNAL_OSTREAM_HAS_SLOT(_ops, _slot)                               \
    ( (_ops)->size >= ( offsetof(struct dj_ostream_ops, _slot) +               \
                        sizeof((_ops)->_slot) ) )

// D_INTERNAL_OSTREAM_FILL_BLOCK
//   constant: the bytes d_ostream_fill writes per call.
#define D_INTERNAL_OSTREAM_FILL_BLOCK  64

// D_INTERNAL_OSTREAM_DIGITS
//   constant: room for a 64-bit integer in decimal: twenty digits and a sign.
#define D_INTERNAL_OSTREAM_DIGITS  21

D_STATIC_ASSERT(sizeof(long) <= sizeof(int64_t),
                "d_ostream_write_long converts through int64_t");
D_STATIC_ASSERT(sizeof(size_t) <= sizeof(uint64_t),
                "d_ostream_write_size converts through uint64_t");
D_STATIC_ASSERT((sizeof(size_t) * CHAR_BIT) % 2 == 0,
                "d_ostream_written64 shifts by half of size_t's width twice");

// DIGIT_PAIRS
//   constant: "00" to "99", so two digits are converted per division.
static const char DIGIT_PAIRS[201] =
    "00010203040506070809"
    "10111213141516171819"
    "20212223242526272829"
    "30313233343536373839"
    "40414243444546474849"
    "50515253545556575859"
    "60616263646566676869"
    "70717273747576777879"
    "80818283848586878889"
    "90919293949596979899";

// HEX_DIGITS
//   constant: the lower-case hexadecimal digits.
static const char HEX_DIGITS[17] = "0123456789abcdef";

// d_internal_ostream_none
//   the storage every empty window points at, so the window's pointer
// arithmetic is always within one object. Nothing is ever written to it: an
// empty window has no room, and the inline path writes only into room.
static unsigned char d_internal_ostream_none[1];


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

/**
 * @brief Whether a stream's destination is a storage destination.
 *
 * @param[in] _os  the stream; initialised.
 * @return 1 or 0.
 *
 * d_ostream_init refuses a table too small to hold grow, so the slot can be
 * read directly from any table a stream keeps.
 */
static int32_t
d_internal_ostream_is_storage(
    const struct dj_ostream* _os
)
{
    return ( (_os->ops != NULL) &&
             (_os->ops->grow != NULL) ) ? 1 : 0;
}

/**
 * @brief Whether d_ostream_init accepted the stream: it has a destination.
 *
 * @param[in] _os  the stream.
 * @return 1 or 0.
 */
static int32_t
d_internal_ostream_is_made(
    const struct dj_ostream* _os
)
{
    return ( (_os->sink.write != NULL) ||
             (d_internal_ostream_is_storage(_os) != 0) ) ? 1 : 0;
}

/**
 * @brief Sets the inline path's limit from the window and the flags.
 *
 * @param[in,out] _os  the stream.
 *
 * A line-buffered transport keeps its window collapsed at cur, so every write
 * reaches the slow path, which looks for the newline; anything else may write
 * up to the window's limit.
 */
static void
d_internal_ostream_settle_end(
    struct dj_ostream* _os
)
{
    _os->end = ((_os->flags & D_OSTREAM_LINE_BUFFERED) != 0)
                   ? _os->cur
                   : _os->limit;

    return;
}

/**
 * @brief Records a failure, unless one is already recorded.
 *
 * @param[in,out] _os      the stream.
 * @param[in]     _status  the enum dj_ostream_status value to record.
 * @param[in]     _error   the errno value to record with it; 0 when unknown.
 * @return 0 always, so a caller can return it.
 *
 * The first failure is the one a caller needs; later ones are usually its
 * consequences. The window collapses, so the inline path sends every later
 * write here, where it is refused.
 */
static int32_t
d_internal_ostream_mark(
    struct dj_ostream* _os,
    int32_t            _status,
    int32_t            _error
)
{
    if (_os->status == D_OSTREAM_OK)
    {
        _os->status     = _status;
        _os->last_errno = _error;
    }

    _os->end = _os->cur;

    return 0;
}

/**
 * @brief Whether a stream may be written: made, not failed, not closed.
 *
 * @param[in,out] _os  the stream; may be null.
 * @return 1 or 0.
 *
 * A write to a closed stream is itself recorded, as D_OSTREAM_CLOSED.
 */
static int32_t
d_internal_ostream_ready(
    struct dj_ostream* _os
)
{
    if (_os == NULL)
    {
        return 0;
    }

    if ((_os->flags & D_INTERNAL_OSTREAM_CLOSED) != 0)
    {
        return d_internal_ostream_mark(_os, D_OSTREAM_CLOSED, 0);
    }

    return (_os->status == D_OSTREAM_OK) ? 1 : 0;
}

/**
 * @brief Counts bytes as having left the window.
 *
 * @param[in,out] _os     the stream.
 * @param[in]     _count  how many.
 *
 * The count wraps like the size_t it is, and the wraps are counted too, for
 * d_ostream_written64.
 */
static void
d_internal_ostream_account(
    struct dj_ostream* _os,
    size_t             _count
)
{
    const size_t before = _os->delivered;

    _os->delivered = before + _count;

    if (_os->delivered < before)
    {
        ++_os->wraps;
    }

    return;
}

/**
 * @brief Clears errno before a call into a destination.
 *
 * @return the caller's errno, for d_internal_ostream_settle to put back.
 */
static int
d_internal_ostream_errno_clear(void)
{
    const int saved = errno;

    errno = 0;

    return saved;
}

/**
 * @brief Turns a destination's answer into the stream's.
 *
 * @param[in,out] _os     the stream.
 * @param[in]     _ok     what the destination returned, as 1 or 0.
 * @param[in]     _saved  the errno d_internal_ostream_errno_clear returned.
 * @return _ok.
 *
 * On success errno is put back; on failure the destination's errno is
 * recorded and left in place.
 */
static int32_t
d_internal_ostream_settle(
    struct dj_ostream* _os,
    int32_t            _ok,
    int                _saved
)
{
    const int reason = errno;

    if (_ok != 0)
    {
        errno = _saved;

        return 1;
    }

    errno = (reason != 0) ? reason : _saved;

    return d_internal_ostream_mark(_os, D_OSTREAM_FAILED, (int32_t)reason);
}

/**
 * @brief Hands bytes to a transport's sink.
 *
 * @param[in,out] _os    the stream; a transport.
 * @param[in]     _data  the bytes.
 * @param[in]     _size  how many; not 0.
 * @return 1 or 0.
 *
 * Anything short of all of them is a failure: a producer that cannot rewind
 * has no meaningful way to continue from one.
 */
static int32_t
d_internal_ostream_deliver(
    struct dj_ostream* _os,
    const void*        _data,
    size_t             _size
)
{
    const int    saved = d_internal_ostream_errno_clear();
    const size_t taken = _os->sink.write(_os->sink.context, _data, _size);

    return d_internal_ostream_settle(_os, (taken == _size) ? 1 : 0, saved);
}

/**
 * @brief Delivers what a transport's window holds.
 *
 * @param[in,out] _os  the stream; a transport.
 * @return 1 or 0.
 *
 * The window is emptied either way: if the sink refused, the stream has
 * failed and the bytes have nowhere to go.
 */
static int32_t
d_internal_ostream_drain(
    struct dj_ostream* _os
)
{
    const size_t pending = (size_t)(_os->cur - _os->base);

    if (pending == 0u)
    {
        return 1;
    }

    _os->cur = _os->base;
    d_internal_ostream_account(_os, pending);
    d_internal_ostream_settle_end(_os);

    return d_internal_ostream_deliver(_os, _os->base, pending);
}

/**
 * @brief Commits a storage destination's window and asks for room.
 *
 * @param[in,out] _os    the stream; a storage destination.
 * @param[in]     _need  the room wanted; 0 asks only for the commit.
 * @return 1 or 0.
 *
 * The window's bytes are counted before the destination sees them, and base
 * is brought up to cur afterwards whatever the destination did, so a byte is
 * never counted twice -- not even when the grow failed and left the window
 * where it was.
 */
static int32_t
d_internal_ostream_grow(
    struct dj_ostream* _os,
    size_t             _need
)
{
    d_internal_ostream_account(_os, (size_t)(_os->cur - _os->base));

    const int     saved = d_internal_ostream_errno_clear();
    const int32_t ok    = _os->ops->grow(_os, _need);

    _os->base = _os->cur;

    return d_internal_ostream_settle(_os, (ok != 0) ? 1 : 0, saved);
}

/**
 * @brief Empties the window by the destination's means: a transport
 *        delivers it, a storage destination commits it.
 *
 * @param[in,out] _os  the stream.
 * @return 1 or 0.
 */
static int32_t
d_internal_ostream_push(
    struct dj_ostream* _os
)
{
    return (d_internal_ostream_is_storage(_os) != 0)
               ? d_internal_ostream_grow(_os, 0u)
               : d_internal_ostream_drain(_os);
}

/**
 * @brief Writes into a storage destination, growing it as needed.
 *
 * @param[in,out] _os    the stream; a storage destination.
 * @param[in]     _data  the bytes.
 * @param[in]     _size  how many; not 0.
 * @return 1 or 0.
 *
 * A destination that can grow makes room for all of it at once; one that
 * cannot -- a fixed array -- gives what it has, and is asked again.
 */
static int32_t
d_internal_ostream_store(
    struct dj_ostream* _os,
    const void*        _data,
    size_t             _size
)
{
    const unsigned char* at   = (const unsigned char*)_data;
    size_t               left = _size;

    while (left > 0u)
    {
        size_t room = (size_t)(_os->end - _os->cur);

        // not enough room: commit, and ask for all of what is left
        if (room < left)
        {
            if (d_internal_ostream_grow(_os, left) == 0)
            {
                return 0;
            }

            room = (size_t)(_os->end - _os->cur);

            if (room == 0u)
            {
                return d_internal_ostream_mark(_os, D_OSTREAM_FAILED, ENOSPC);
            }
        }

        const size_t chunk = (room < left) ? room : left;

        memcpy(_os->cur, at, chunk);
        _os->cur += chunk;
        at       += chunk;
        left     -= chunk;
    }

    return 1;
}

/**
 * @brief Writes through a transport's buffer.
 *
 * @param[in,out] _os    the stream; a transport.
 * @param[in]     _data  the bytes.
 * @param[in]     _size  how many; not 0.
 * @return 1 or 0.
 *
 * What fits beside the buffer's bytes is copied. What does not drains the
 * buffer first, and is then copied into the empty buffer, or sent straight
 * on when it is at least the buffer's size -- unbuffered, always -- so order
 * is kept and nothing is copied twice. A line-buffered stream drains after a
 * write that holds a newline.
 */
static int32_t
d_internal_ostream_transport(
    struct dj_ostream* _os,
    const void*        _data,
    size_t             _size
)
{
    if (_size > (size_t)(_os->limit - _os->cur))
    {
        if (d_internal_ostream_drain(_os) == 0)
        {
            return 0;
        }

        // too large to buffer: straight on, uncopied
        if (_size >= _os->capacity)
        {
            d_internal_ostream_account(_os, _size);

            return d_internal_ostream_deliver(_os, _data, _size);
        }
    }

    memcpy(_os->cur, _data, _size);
    _os->cur += _size;

    if ( ((_os->flags & D_OSTREAM_LINE_BUFFERED) != 0) &&
         (memchr(_data, '\n', _size) != NULL) )
    {
        return d_internal_ostream_drain(_os);
    }

    return 1;
}

/**
 * @brief The number of decimal digits in a 32-bit value.
 *
 * @param[in] _value  the value.
 * @return 1 to 10.
 *
 * A comparison tree: at most four comparisons, and no multiplication, for
 * the values almost every write sees.
 */
static size_t
d_internal_ostream_decimal_digits32(
    uint32_t _value
)
{
    if (_value < 100000u)
    {
        if (_value < 100u)
        {
            return (_value < 10u) ? 1u : 2u;
        }

        if (_value < 1000u)
        {
            return 3u;
        }

        return (_value < 10000u) ? 4u : 5u;
    }

    if (_value < 10000000u)
    {
        return (_value < 1000000u) ? 6u : 7u;
    }

    if (_value < 100000000u)
    {
        return 8u;
    }

    return (_value < 1000000000u) ? 9u : 10u;
}

/**
 * @brief The number of decimal digits in a value.
 *
 * @param[in] _value  the value.
 * @return 1 to 20.
 *
 * Above ten digits, ten at a time come off by one division, which a 32-bit
 * target does in a library call -- but only for values that large.
 */
static size_t
d_internal_ostream_decimal_digits(
    uint64_t _value
)
{
    size_t   digits = 0u;
    uint64_t rest   = _value;

    // ten digits off at a time while more than ten remain
    while (rest >= 10000000000ull)
    {
        rest   /= 10000000000ull;
        digits += 10u;
    }

    // below 10^10 but above 2^32 - 1, which itself has ten digits: ten
    return digits + ( (rest > (uint64_t)UINT32_MAX)
                          ? 10u
                          : d_internal_ostream_decimal_digits32(
                                (uint32_t)rest) );
}

/**
 * @brief Writes a value's decimal digits backwards, ending at _end.
 *
 * @param[out] _end    one past where the last digit goes.
 * @param[in]  _value  the value.
 *
 * Two digits per division. Above 32 bits the division is 64-bit; below it,
 * 32-bit, which a 32-bit target does in hardware rather than in a library
 * call -- and almost every value written is below it.
 */
static void
d_internal_ostream_decimal_fill(
    char*    _end,
    uint64_t _value
)
{
    char*    at   = _end;
    uint64_t wide = _value;

    while (wide > (uint64_t)UINT32_MAX)
    {
        const uint64_t quotient = wide / 100u;
        const size_t   pair     = (size_t)(wide - (quotient * 100u));

        at -= 2;
        memcpy(at, DIGIT_PAIRS + (2u * pair), 2u);
        wide = quotient;
    }

    uint32_t narrow = (uint32_t)wide;

    while (narrow >= 100u)
    {
        const uint32_t quotient = narrow / 100u;
        const size_t   pair     = (size_t)(narrow - (quotient * 100u));

        at -= 2;
        memcpy(at, DIGIT_PAIRS + (2u * pair), 2u);
        narrow = quotient;
    }

    if (narrow >= 10u)
    {
        at -= 2;
        memcpy(at, DIGIT_PAIRS + (2u * (size_t)narrow), 2u);
    }
    else
    {
        --at;
        *at = (char)('0' + (int)narrow);
    }

    return;
}

/**
 * @brief Writes a magnitude in decimal, with a leading minus when asked.
 *
 * @param[in,out] _os         the stream.
 * @param[in]     _magnitude  the absolute value.
 * @param[in]     _negative   non-zero for a minus sign.
 * @return as d_ostream_write().
 *
 * In place when the window has room for it; otherwise through a local array.
 */
static int32_t
d_internal_ostream_write_decimal(
    struct dj_ostream* _os,
    uint64_t           _magnitude,
    int32_t            _negative
)
{
    const size_t sign   = (_negative != 0) ? 1u : 0u;
    const size_t length = sign + d_internal_ostream_decimal_digits(_magnitude);
    char* const  room   = d_ostream_reserve(_os, length);

    // in place
    if (room != NULL)
    {
        room[0] = '-';
        d_internal_ostream_decimal_fill(room + length, _magnitude);

        return d_ostream_commit(_os, length);
    }

    char local[D_INTERNAL_OSTREAM_DIGITS];

    local[0] = '-';
    d_internal_ostream_decimal_fill(local + length, _magnitude);

    return d_ostream_write(_os, local, length);
}


//==============================================================================
// 2.  LIFECYCLE
//==============================================================================

/*
d_ostream_init
  Every member is set before anything is checked, so a refused stream is a
well-defined one: no destination, an empty window, D_OSTREAM_INVALID, and
nothing owned. That is what lets d_ostream_close and the state functions take
any stream d_ostream_init has seen. The table's size is checked before any
slot is read.
*/
int32_t
d_ostream_init(
    struct dj_ostream*           _os,
    struct dj_pack_sink          _sink,
    const struct dj_ostream_ops* _ops,
    int32_t                      _flags
)
{
    if (_os == NULL)
    {
        return 0;
    }

    d_ostream_window_clear(_os);
    _os->sink       = _sink;
    _os->ops        = _ops;
    _os->buffer     = NULL;
    _os->capacity   = 0u;
    _os->delivered  = 0u;
    _os->wraps      = 0u;
    _os->status     = D_OSTREAM_OK;
    _os->last_errno = 0;
    _os->flags      = _flags & D_INTERNAL_OSTREAM_FLAGS;
    _os->reserved   = 0;

    const int32_t versioned = ( (_ops == NULL) ||
                                (D_INTERNAL_OSTREAM_HAS_SLOT(_ops, query)) )
                                  ? 1 : 0;
    const int32_t storage   = ( (versioned != 0) &&
                                (_ops != NULL) &&
                                (_ops->grow != NULL) ) ? 1 : 0;

    // exactly one of a write and a grow, and a close slot for what is owned
    if ( (versioned == 0) ||
         ((_sink.write != NULL) == (storage != 0)) ||
         ( ((_os->flags & D_OSTREAM_OWNS_CONTEXT) != 0) &&
           ( (_ops == NULL) ||
             (_ops->close == NULL) ) ) )
    {
        _os->sink.write   = NULL;
        _os->sink.context = NULL;
        _os->ops          = NULL;
        _os->flags        = 0;

        return d_internal_ostream_mark(_os, D_OSTREAM_INVALID, EINVAL);
    }

    // a storage destination's first window comes from its grow
    if (storage != 0)
    {
        _os->flags &= ~D_OSTREAM_LINE_BUFFERED;

        if (d_internal_ostream_grow(_os, 0u) == 0)
        {
            _os->sink.context = NULL;
            _os->ops          = NULL;
            _os->flags        = 0;
            d_ostream_window_clear(_os);

            return 0;
        }
    }

    return 1;
}

/*
d_ostream_set_buffer
  The old buffer is drained before the new one is taken, so no byte changes
places twice and order is kept.
*/
int32_t
d_ostream_set_buffer(
    struct dj_ostream* _os,
    void*              _buffer,
    size_t             _capacity
)
{
    if ( (d_internal_ostream_ready(_os) == 0) ||
         (d_internal_ostream_is_storage(_os) != 0) ||
         (d_internal_ostream_drain(_os) == 0) )
    {
        return 0;
    }

    if ( (_buffer == NULL) ||
         (_capacity == 0u) )
    {
        _os->buffer   = NULL;
        _os->capacity = 0u;
        d_ostream_window_clear(_os);

        return 1;
    }

    _os->buffer   = (unsigned char*)_buffer;
    _os->capacity = _capacity;
    d_ostream_window_set(_os, _buffer, _capacity);

    return 1;
}

/*
d_ostream_flush
  The window is emptied by the destination's own means, then the flush slot
runs. A destination with no flush slot holds nothing back.
*/
int32_t
d_ostream_flush(
    struct dj_ostream* _os
)
{
    if ( (d_internal_ostream_ready(_os) == 0) ||
         (d_internal_ostream_push(_os) == 0) )
    {
        return 0;
    }

    if ( (_os->ops != NULL) &&
         (_os->ops->flush != NULL) )
    {
        const int     saved = d_internal_ostream_errno_clear();
        const int32_t ok    = _os->ops->flush(_os->sink.context);

        return d_internal_ostream_settle(_os, (ok != 0) ? 1 : 0, saved);
    }

    return 1;
}

/*
d_ostream_close
  The close slot finishes and releases in one call; a destination with no
close slot is finished by its flush slot, if it has one. An owned context is
released even after a failure, because a failed stream still owns it. The
window is emptied for good, so every later write reaches the slow path and is
refused there.
*/
int32_t
d_ostream_close(
    struct dj_ostream* _os
)
{
    if (_os == NULL)
    {
        return 0;
    }

    if ( ((_os->flags & D_INTERNAL_OSTREAM_CLOSED) == 0) &&
         (d_internal_ostream_is_made(_os) != 0) )
    {
        const int32_t release = ((_os->flags & D_OSTREAM_OWNS_CONTEXT) != 0)
                                    ? 1 : 0;

        if (_os->status == D_OSTREAM_OK)
        {
            (void)d_internal_ostream_push(_os);
        }

        if ( (_os->ops != NULL) &&
             (_os->ops->close != NULL) )
        {
            const int     saved = d_internal_ostream_errno_clear();
            const int32_t ok    = _os->ops->close(_os->sink.context, release);

            (void)d_internal_ostream_settle(_os, (ok != 0) ? 1 : 0, saved);
        }
        else if ( (_os->status == D_OSTREAM_OK) &&
                  (_os->ops != NULL) &&
                  (_os->ops->flush != NULL) )
        {
            const int     saved = d_internal_ostream_errno_clear();
            const int32_t ok    = _os->ops->flush(_os->sink.context);

            (void)d_internal_ostream_settle(_os, (ok != 0) ? 1 : 0, saved);
        }
    }

    // the count is final: nothing pending is left in an emptied window
    d_internal_ostream_account(_os, (size_t)(_os->cur - _os->base));
    d_ostream_window_clear(_os);
    _os->flags |= D_INTERNAL_OSTREAM_CLOSED;

    return (_os->status == D_OSTREAM_OK) ? 1 : 0;
}


//==============================================================================
// 3.  OUTPUT
//==============================================================================

/*
d_ostream_write_slow
  Everything the inline path declines: an empty write (the readiness check),
a failed or closed stream, null data, and a write the window cannot take. The
window's limit is set again on the way out, since draining or growing moved
the window.
*/
int32_t
d_ostream_write_slow(
    struct dj_ostream* _os,
    const void*        _data,
    size_t             _size
)
{
    if (d_internal_ostream_ready(_os) == 0)
    {
        return 0;
    }

    if (_size == 0u)
    {
        return 1;
    }

    if (_data == NULL)
    {
        return d_internal_ostream_mark(_os, D_OSTREAM_INVALID, EINVAL);
    }

    const int32_t ok = (d_internal_ostream_is_storage(_os) != 0)
                           ? d_internal_ostream_store(_os, _data, _size)
                           : d_internal_ostream_transport(_os, _data, _size);

    if (ok != 0)
    {
        d_internal_ostream_settle_end(_os);
    }

    return ok;
}

/*
d_ostream_reserve_slow
  A storage destination is asked to grow; a transport's buffer is drained to
make room, when the buffer is large enough to hold the request at all. A
line-buffered transport's room is past its collapsed end, which is how
d_ostream_commit knows to look for a newline.
*/
char*
d_ostream_reserve_slow(
    struct dj_ostream* _os,
    size_t             _need
)
{
    if (d_internal_ostream_ready(_os) == 0)
    {
        return NULL;
    }

    const size_t need = (_need == 0u) ? 1u : _need;

    if (d_internal_ostream_is_storage(_os) != 0)
    {
        if ( ((size_t)(_os->end - _os->cur) < need) &&
             (d_internal_ostream_grow(_os, need) == 0) )
        {
            return NULL;
        }

        return ((size_t)(_os->end - _os->cur) >= need) ? (char*)_os->cur
                                                       : NULL;
    }

    if ( ((size_t)(_os->limit - _os->cur) < need) &&
         (_os->capacity >= need) &&
         (d_internal_ostream_drain(_os) == 0) )
    {
        return NULL;
    }

    return ((size_t)(_os->limit - _os->cur) >= need) ? (char*)_os->cur : NULL;
}

/*
d_ostream_commit_slow
  Reached when a commit moved cur past end, which only a collapsed window
allows: the bytes just committed are [end, cur), and on a line-buffered
stream a newline among them drains it.
*/
int32_t
d_ostream_commit_slow(
    struct dj_ostream* _os
)
{
    if (_os == NULL)
    {
        return 0;
    }

    // a failed stream lends no room, so nothing here was written through it
    if (_os->status != D_OSTREAM_OK)
    {
        _os->end = _os->cur;

        return 0;
    }

    const size_t added = (size_t)(_os->cur - _os->end);

    if ( ((_os->flags & D_OSTREAM_LINE_BUFFERED) != 0) &&
         (memchr(_os->end, '\n', added) != NULL) &&
         (d_internal_ostream_drain(_os) == 0) )
    {
        return 0;
    }

    d_internal_ostream_settle_end(_os);

    return 1;
}

/*
d_ostream_cursor_write_slow
  The cursor's writes go back into the stream first, so the slow path sees
the stream as it is. A line-buffered stream's batch then lies past its
collapsed end, and is checked for a newline here, as d_ostream_cursor_give
would check it, before the write that did not fit; the batch's check is
never skipped, only done early. The cursor handed back is taken afresh,
since draining or growing moved the window.
*/
struct dj_ostream_cursor
d_ostream_cursor_write_slow(
    struct dj_ostream*       _os,
    struct dj_ostream_cursor _cursor,
    const void*              _data,
    size_t                   _size
)
{
    if (_os == NULL)
    {
        return _cursor;
    }

    _os->cur = _cursor.cur;

    if ( (_os->cur <= _os->end) ||
         (d_ostream_commit_slow(_os) != 0) )
    {
        (void)d_ostream_write_slow(_os, _data, _size);
    }

    return d_ostream_cursor_take(_os);
}

/*
d_ostream_write_char_slow
  The character is copied into a local here, on the slow path, so the inline
path can pass it in a register.
*/
int32_t
d_ostream_write_char_slow(
    struct dj_ostream* _os,
    int                _c
)
{
    const unsigned char c = (unsigned char)_c;

    return d_ostream_write_slow(_os, &c, 1u);
}

/*
d_ostream_cursor_put_slow
  As d_ostream_write_char_slow, for a cursor.
*/
struct dj_ostream_cursor
d_ostream_cursor_put_slow(
    struct dj_ostream*       _os,
    struct dj_ostream_cursor _cursor,
    int                      _c
)
{
    const unsigned char c = (unsigned char)_c;

    return d_ostream_cursor_write_slow(_os, _cursor, &c, 1u);
}

/*
d_ostream_fill
  Through the inline path, a block at a time, so a long run costs one memset
and a memcpy per block.
*/
int32_t
d_ostream_fill(
    struct dj_ostream* _os,
    char               _c,
    size_t             _count
)
{
    if (_os == NULL)
    {
        return 0;
    }

    unsigned char block[D_INTERNAL_OSTREAM_FILL_BLOCK];
    size_t        left = _count;

    memset(block,
           (unsigned char)_c,
           (left < sizeof(block)) ? left : sizeof(block));

    while (left > 0u)
    {
        const size_t chunk = (left < sizeof(block)) ? left : sizeof(block);

        if (d_ostream_write(_os, block, chunk) == 0)
        {
            return 0;
        }

        left -= chunk;
    }

    return d_ostream_write_slow(_os, NULL, 0u);
}

/*
d_ostream_fail
  Any status a producer may not record is recorded as FAILED, so the stream
is marked whatever the caller passed.
*/
int32_t
d_ostream_fail(
    struct dj_ostream* _os,
    int32_t            _status,
    int32_t            _error
)
{
    if (_os == NULL)
    {
        return 0;
    }

    const int32_t status = ( (_status == D_OSTREAM_INVALID) ||
                             (_status == D_OSTREAM_UNSUPPORTED) )
                               ? _status
                               : D_OSTREAM_FAILED;

    return d_internal_ostream_mark(_os, status, _error);
}

/*
d_ostream_write_int
  Negation happens in uint64_t, so INT_MIN is written correctly.
*/
int32_t
d_ostream_write_int(
    struct dj_ostream* _os,
    int                _value
)
{
    return d_ostream_write_int64(_os, (int64_t)_value);
}

/*
d_ostream_write_uint
  Widened to uint64_t, which every C build has.
*/
int32_t
d_ostream_write_uint(
    struct dj_ostream* _os,
    unsigned int       _value
)
{
    return d_internal_ostream_write_decimal(_os, (uint64_t)_value, 0);
}

/*
d_ostream_write_long
  Through int64_t, which a static assertion holds to be at least as wide.
*/
int32_t
d_ostream_write_long(
    struct dj_ostream* _os,
    long               _value
)
{
    return d_ostream_write_int64(_os, (int64_t)_value);
}

/*
d_ostream_write_ulong
  Widened to uint64_t.
*/
int32_t
d_ostream_write_ulong(
    struct dj_ostream* _os,
    unsigned long      _value
)
{
    return d_internal_ostream_write_decimal(_os, (uint64_t)_value, 0);
}

/*
d_ostream_write_size
  Not through unsigned long, which is narrower than size_t on 64-bit Windows.
*/
int32_t
d_ostream_write_size(
    struct dj_ostream* _os,
    size_t             _value
)
{
    return d_internal_ostream_write_decimal(_os, (uint64_t)_value, 0);
}

/*
d_ostream_write_hex
  The digits are produced backwards into a local array as wide as the widest
size_t, then written once. A width outside 1 to that many digits asks for no
padding.
*/
int32_t
d_ostream_write_hex(
    struct dj_ostream* _os,
    size_t             _value,
    int32_t            _digits
)
{
    char         text[2u * sizeof(size_t)];
    const size_t most  = sizeof(text);
    size_t       at    = most;
    size_t       value = _value;

    do
    {
        --at;
        text[at] = HEX_DIGITS[value & 0xFu];
        value  >>= 4;
    } while (value != 0u);

    // pad to the width asked for
    if ( (_digits > 0) &&
         ((size_t)_digits <= most) )
    {
        while ((most - at) < (size_t)_digits)
        {
            --at;
            text[at] = '0';
        }
    }

    return d_ostream_write(_os, text + at, most - at);
}

/*
d_ostream_write_int64
  Negation happens in uint64_t, so INT64_MIN is written correctly.
*/
int32_t
d_ostream_write_int64(
    struct dj_ostream* _os,
    int64_t            _value
)
{
    const uint64_t magnitude = (_value < 0)
                                   ? ((uint64_t)0 - (uint64_t)_value)
                                   : (uint64_t)_value;

    return d_internal_ostream_write_decimal(_os,
                                            magnitude,
                                            (_value < 0) ? 1 : 0);
}

/*
d_ostream_write_uint64
  The general case the narrower writers widen to.
*/
int32_t
d_ostream_write_uint64(
    struct dj_ostream* _os,
    uint64_t           _value
)
{
    return d_internal_ostream_write_decimal(_os, _value, 0);
}


//==============================================================================
// 4.  STATE
//==============================================================================

/*
d_ostream_status
  Read from the member; a null stream reads as never made.
*/
int32_t
d_ostream_status(
    const struct dj_ostream* _os
)
{
    return (_os != NULL) ? _os->status : (int32_t)D_OSTREAM_INVALID;
}

/*
d_ostream_last_errno
  Read from the member.
*/
int32_t
d_ostream_last_errno(
    const struct dj_ostream* _os
)
{
    return (_os != NULL) ? _os->last_errno : 0;
}

/*
d_ostream_is_ok
  Made, healthy and open: what the slow path's readiness check asks, without
recording anything.
*/
int32_t
d_ostream_is_ok(
    const struct dj_ostream* _os
)
{
    return ( (_os != NULL) &&
             (d_internal_ostream_is_made(_os) != 0) &&
             (_os->status == D_OSTREAM_OK) &&
             ((_os->flags & D_INTERNAL_OSTREAM_CLOSED) == 0) ) ? 1 : 0;
}

/*
d_ostream_written
  What left the window, plus what is in it.
*/
size_t
d_ostream_written(
    const struct dj_ostream* _os
)
{
    return (_os != NULL)
               ? (_os->delivered + (size_t)(_os->cur - _os->base))
               : 0u;
}

/*
d_ostream_written64
  The wraps are shifted in by half of size_t's width twice, which is a whole
width where size_t is 32 bits and shifts them out where it is 64 -- where
they never happen -- without a shift as wide as the type, which C leaves
undefined.
*/
uint64_t
d_ostream_written64(
    const struct dj_ostream* _os
)
{
    if (_os == NULL)
    {
        return 0u;
    }

    const unsigned half  = (unsigned)((sizeof(size_t) * CHAR_BIT) / 2u);
    const uint64_t wraps = ((uint64_t)_os->wraps << half) << half;

    return wraps +
           (uint64_t)_os->delivered +
           (uint64_t)(_os->cur - _os->base);
}

/**
 * @brief Sets every member of a description to 0, the safe answer.
 *
 * @param[out] _out  the description.
 */
static void
d_internal_ostream_info_clear(
    struct dj_ostream_info* _out
)
{
    _out->is_terminal    = 0;
    _out->has_vt         = 0;
    _out->is_binary_safe = 0;
    _out->reserved       = 0;

    return;
}

/*
d_ostream_query
  A closed stream is not asked: its context may already have been released.
*/
int32_t
d_ostream_query(
    const struct dj_ostream* _os,
    struct dj_ostream_info*  _out
)
{
    if (_out == NULL)
    {
        return 0;
    }

    d_internal_ostream_info_clear(_out);

    if ( (_os == NULL) ||
         (d_internal_ostream_is_made(_os) == 0) ||
         ((_os->flags & D_INTERNAL_OSTREAM_CLOSED) != 0) ||
         (_os->ops == NULL) ||
         (_os->ops->query == NULL) )
    {
        return 0;
    }

    if (_os->ops->query(_os->sink.context, _out) == 0)
    {
        d_internal_ostream_info_clear(_out);

        return 0;
    }

    return 1;
}


//==============================================================================
// 5.  DESTINATIONS
//==============================================================================

/**
 * @brief The lowered sink's write: one d_ostream_write, all or nothing.
 *
 * @param[in] _context  a struct dj_ostream.
 * @param[in] _data     the bytes.
 * @param[in] _size     how many.
 * @return _size, or 0 when the stream refused.
 */
static size_t
d_internal_ostream_sink_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    return (d_ostream_write((struct dj_ostream*)_context, _data, _size) != 0)
               ? _size
               : 0u;
}

/*
d_ostream_as_sink
  The sink holds the stream's address and nothing else.
*/
struct dj_pack_sink
d_ostream_as_sink(
    struct dj_ostream* _os
)
{
    struct dj_pack_sink sink;

    sink.write   = &d_internal_ostream_sink_write;
    sink.context = _os;

    return sink;
}

/**
 * @brief The bytes of a fixed destination's array that hold output.
 *
 * @param[in] _fixed  the state.
 * @return its capacity, less the terminator's byte when it keeps one.
 */
static size_t
d_internal_ostream_fixed_usable(
    const struct dj_ostream_fixed* _fixed
)
{
    if ( (_fixed->data == NULL) ||
         (_fixed->capacity == 0u) )
    {
        return 0u;
    }

    return (_fixed->terminate != 0) ? (_fixed->capacity - 1u)
                                    : _fixed->capacity;
}

/**
 * @brief A fixed destination's grow: commits into the array while it has
 *        room, then spills into the scratch, where bytes are only counted.
 *
 * @param[in,out] _os    the stream.
 * @param[in]     _need  the room wanted; 0 asks only for the commit.
 * @return 1; a fixed destination never fails.
 *
 * While the window is the array's, base sits at the stored length, so a
 * commit adds [base, cur) to it and writes the terminator. Room left in the
 * array is given back as it is, even when it is less than _need: the stream
 * fills it and asks again, and only an array with no room left spills -- so
 * the stored prefix is as long as it can be. The stream has already counted
 * the scratch's bytes, so spilling discards them.
 */
static int32_t
d_internal_ostream_fixed_grow(
    struct dj_ostream* _os,
    size_t             _need
)
{
    struct dj_ostream_fixed* const fixed   =
        (struct dj_ostream_fixed*)_os->sink.context;
    const size_t                  usable  =
        d_internal_ostream_fixed_usable(fixed);
    const size_t                  pending = (size_t)(_os->cur - _os->base);

    // the array's window: commit, and keep storing while there is room
    if (fixed->spilling == 0)
    {
        fixed->length += pending;

        if ( (fixed->terminate != 0) &&
             (fixed->capacity > 0u) )
        {
            fixed->data[fixed->length] = '\0';
        }

        if ( (_need == 0u) ||
             (fixed->length < usable) )
        {
            d_ostream_window_set(_os,
                                 (usable > 0u) ? (fixed->data + fixed->length)
                                               : NULL,
                                 usable - fixed->length);

            return 1;
        }

        fixed->spilling = 1;
    }
    else if (pending != 0u)
    {
        fixed->truncated = 1;
    }

    // room was wanted that the array does not have: what follows is counted
    if (_need != 0u)
    {
        fixed->truncated = 1;
    }

    d_ostream_window_set(_os, fixed->scratch, sizeof(fixed->scratch));

    return 1;
}

/**
 * @brief A fixed destination's description: bytes arrive unaltered.
 *
 * @param[in]  _context  the state.
 * @param[out] _out      the description; arrives zeroed.
 * @return 1.
 */
static int32_t
d_internal_ostream_fixed_query(
    void*                   _context,
    struct dj_ostream_info* _out
)
{
    (void)_context;

    _out->is_binary_safe = 1;

    return 1;
}

// d_internal_ostream_fixed_ops
//   table: the fixed destination's operations.
static const struct dj_ostream_ops d_internal_ostream_fixed_ops =
{
    sizeof(struct dj_ostream_ops),
    &d_internal_ostream_fixed_grow,
    NULL,
    NULL,
    &d_internal_ostream_fixed_query
};

/*
d_ostream_init_fixed
  The state is filled in before d_ostream_init runs, because its first grow
reads it to place the window. A null state reaches d_ostream_init as neither
a write nor a table, which refuses it.
*/
int32_t
d_ostream_init_fixed(
    struct dj_ostream*       _os,
    struct dj_ostream_fixed* _fixed,
    void*                    _data,
    size_t                   _capacity,
    int32_t                  _terminate
)
{
    struct dj_pack_sink sink;

    sink.write   = NULL;
    sink.context = _fixed;

    if (_fixed == NULL)
    {
        return d_ostream_init(_os, sink, NULL, 0);
    }

    _fixed->data      = (unsigned char*)_data;
    _fixed->capacity  = (_data != NULL) ? _capacity : 0u;
    _fixed->length    = 0u;
    _fixed->terminate = (_terminate != 0) ? 1 : 0;
    _fixed->truncated = 0;
    _fixed->spilling  = 0;
    _fixed->reserved  = 0;

    if ( (_fixed->terminate != 0) &&
         (_fixed->capacity > 0u) )
    {
        _fixed->data[0] = '\0';
    }

    return d_ostream_init(_os, sink, &d_internal_ostream_fixed_ops, 0);
}

/*
d_ostream_fixed_length
  Read from the member, which each commit updates.
*/
size_t
d_ostream_fixed_length(
    const struct dj_ostream_fixed* _fixed
)
{
    return (_fixed != NULL) ? _fixed->length : 0u;
}

/*
d_ostream_fixed_truncated
  Read from the member, which a spill sets.
*/
int32_t
d_ostream_fixed_truncated(
    const struct dj_ostream_fixed* _fixed
)
{
    return (_fixed != NULL) ? _fixed->truncated : 0;
}

/**
 * @brief Whether a fan-out succeeded, given how many of its targets did.
 *
 * @param[in] _fanout   the fan-out.
 * @param[in] _healthy  how many targets succeeded.
 * @return 1 or 0.
 */
static int32_t
d_internal_ostream_fanout_verdict(
    const struct dj_ostream_fanout* _fanout,
    size_t                          _healthy
)
{
    if (_fanout->policy == D_OSTREAM_FANOUT_ANY)
    {
        return (_healthy != 0u) ? 1 : 0;
    }

    return (_healthy == _fanout->count) ? 1 : 0;
}

/**
 * @brief The fan-out's write: every target that can still be written gets
 *        the bytes, through its own inline path.
 *
 * @param[in] _context  a struct dj_ostream_fanout.
 * @param[in] _data     the bytes.
 * @param[in] _size     how many.
 * @return _size, or 0 when the policy says the fan-out failed.
 *
 * Whatever the policy, one dead target never starves the healthy ones; the
 * policy decides only what the fan-out itself reports.
 */
static size_t
d_internal_ostream_fanout_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    const struct dj_ostream_fanout* fanout  =
        (const struct dj_ostream_fanout*)_context;
    size_t                         healthy = 0u;

    for (size_t i = 0u; i < fanout->count; ++i)
    {
        if (d_ostream_write(fanout->targets[i], _data, _size) != 0)
        {
            ++healthy;
        }
    }

    return (d_internal_ostream_fanout_verdict(fanout, healthy) != 0)
               ? _size
               : 0u;
}

/**
 * @brief Flushes every target.
 *
 * @param[in] _context  a struct dj_ostream_fanout.
 * @return 1 or 0, by the fan-out's policy.
 *
 * Closing a fan-out comes here too: the targets are borrowed, so they are
 * finished but never closed.
 */
static int32_t
d_internal_ostream_fanout_flush(
    void* _context
)
{
    const struct dj_ostream_fanout* fanout  =
        (const struct dj_ostream_fanout*)_context;
    size_t                         healthy = 0u;

    for (size_t i = 0u; i < fanout->count; ++i)
    {
        if (d_ostream_flush(fanout->targets[i]) != 0)
        {
            ++healthy;
        }
    }

    return d_internal_ostream_fanout_verdict(fanout, healthy);
}

/**
 * @brief What every target can do.
 *
 * @param[in]  _context  a struct dj_ostream_fanout.
 * @param[out] _out      the description; arrives zeroed.
 * @return 1.
 *
 * A capability holds only when it holds for all of them: escape sequences
 * meant for a terminal would litter a log file.
 */
static int32_t
d_internal_ostream_fanout_query(
    void*                   _context,
    struct dj_ostream_info* _out
)
{
    const struct dj_ostream_fanout* fanout =
        (const struct dj_ostream_fanout*)_context;

    _out->is_terminal    = 1;
    _out->has_vt         = 1;
    _out->is_binary_safe = 1;

    for (size_t i = 0u; i < fanout->count; ++i)
    {
        struct dj_ostream_info each;

        (void)d_ostream_query(fanout->targets[i], &each);

        _out->is_terminal    = _out->is_terminal & each.is_terminal;
        _out->has_vt         = _out->has_vt & each.has_vt;
        _out->is_binary_safe = _out->is_binary_safe & each.is_binary_safe;
    }

    return 1;
}

// d_internal_ostream_fanout_ops
//   table: the fan-out's operations. No close slot: the targets are borrowed,
// so d_ostream_close finishes the fan-out through its flush slot.
static const struct dj_ostream_ops d_internal_ostream_fanout_ops =
{
    sizeof(struct dj_ostream_ops),
    NULL,
    &d_internal_ostream_fanout_flush,
    NULL,
    &d_internal_ostream_fanout_query
};

/**
 * @brief Whether a fan-out can be written.
 *
 * @param[in] _os      the stream being made.
 * @param[in] _fanout  the fan-out.
 * @return 1 when it has targets, none null, none the stream being made, and
 *         a policy this file knows; 0 otherwise.
 */
static int32_t
d_internal_ostream_fanout_is_valid(
    const struct dj_ostream*        _os,
    const struct dj_ostream_fanout* _fanout
)
{
    if ( (_fanout == NULL) ||
         (_fanout->targets == NULL) ||
         (_fanout->count == 0u) ||
         ( (_fanout->policy != D_OSTREAM_FANOUT_ALL) &&
           (_fanout->policy != D_OSTREAM_FANOUT_ANY) ) )
    {
        return 0;
    }

    for (size_t i = 0u; i < _fanout->count; ++i)
    {
        if ( (_fanout->targets[i] == NULL) ||
             (_fanout->targets[i] == _os) )
        {
            return 0;
        }
    }

    return 1;
}

/*
d_ostream_init_fanout
  An invalid fan-out reaches d_ostream_init as a null write, which refuses it.
*/
int32_t
d_ostream_init_fanout(
    struct dj_ostream*        _os,
    struct dj_ostream_fanout* _fanout
)
{
    struct dj_pack_sink sink;

    sink.write   = NULL;
    sink.context = NULL;

    if (d_internal_ostream_fanout_is_valid(_os, _fanout) != 0)
    {
        sink.write   = &d_internal_ostream_fanout_write;
        sink.context = _fanout;
    }

    return d_ostream_init(_os, sink, &d_internal_ostream_fanout_ops, 0);
}

/*
d_ostream_window_set
  An empty span is the empty window, so a destination never has to tell the
two apart. The limit for the inline path follows from the flags.
*/
void
d_ostream_window_set(
    struct dj_ostream* _os,
    void*              _begin,
    size_t             _size
)
{
    if ( (_begin == NULL) ||
         (_size == 0u) )
    {
        d_ostream_window_clear(_os);

        return;
    }

    _os->base  = (unsigned char*)_begin;
    _os->cur   = _os->base;
    _os->limit = _os->base + _size;
    d_internal_ostream_settle_end(_os);

    return;
}

/*
d_ostream_window_clear
  Every pointer at the one byte no window ever writes, so the inline path's
subtraction stays within one object and finds no room.
*/
void
d_ostream_window_clear(
    struct dj_ostream* _os
)
{
    _os->base  = d_internal_ostream_none;
    _os->cur   = d_internal_ostream_none;
    _os->end   = d_internal_ostream_none;
    _os->limit = d_internal_ostream_none;

    return;
}
