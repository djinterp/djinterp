/*******************************************************************************
* djinterp [c]                                                         ostream.h
*
*   An output stream: one destination protocol for everything in the framework
* that produces bytes -- a string, a file, the console, a place in a document,
* or several of those at once.
* Compiled by BOTH C and C++.
*
*   A STREAM IS A WINDOW AND A SLOW PATH.
*   The stream exposes a window of writable bytes, [cur, end). Writing is one
* comparison and a memcpy into the window, inlined at the call site; only when
* the window cannot take a write does the stream call out of line, and only
* then does it touch the destination. That is the shape LLVM's raw_ostream,
* fmt's buffer, the C++ put area, libucw's fastbufs and Zig's Writer all
* settled on, and it is what makes the stream's cost the cost of the memcpy.
*
*   TWO KINDS OF DESTINATION.
*     A TRANSPORT consumes bytes: a FILE*, the console, a socket, a fan-out,
*   any struct dj_pack_sink. The window is the caller's buffer
*   (d_ostream_set_buffer); a full window is drained through the sink's write
*   in one call, and a write at least as large as the buffer goes straight to
*   the sink, uncopied. Unbuffered, every write is one call to the sink.
*     A STORAGE destination owns the memory the bytes end in: a dj_string, a
*   fixed array. Its window is that memory, so a byte is copied once, into its
*   final place. Its table's grow slot commits what the window holds and makes
*   more room.
*   Every sink in the framework is already a transport, and every stream lowers
* back to a sink (d_ostream_as_sink) for the producers that take only that.
*
*   THE TABLE ANSWERS d_pdf_backend's THREE QUESTIONS, THE SAME WAY.
*     WHO OWNS THE CONTEXT -- the caller, unless the stream was initialised
*   with D_OSTREAM_OWNS_CONTEXT; then d_ostream_close asks the close slot to
*   release it. A stream told to own a context it has no slot to release is
*   refused, and a refused stream owns nothing.
*     WHAT A NULL SLOT MEANS -- the operation does not apply, never a crash.
*   For flush and close that is not a failure: a destination without them
*   holds nothing back and has nothing to finish. For query it is reported,
*   with every capability read as 0, the safe answer. A null grow makes the
*   destination a transport.
*     WHETHER THE TABLE IS VERSIONED -- yes, by a leading `size`, checked and
*   never trusted: a table smaller than its first version is refused, and a
*   slot added later is read only from a table large enough to hold it.
*   Unlike d_pdf_backend, the table carries no context: one static const
*   table serves every stream of a kind, and the context travels in the sink.
*
*   THE STATUS IS STICKY, AND FREE. The first failure is kept, and every write
* after it returns 0 without reaching the destination, so a producer can write
* many times and check once. A failed or closed stream collapses its window,
* so the inline path never tests the status: a write that does not fit was
* going to the slow path anyway, and the slow path is where the status lives.
* A producer that fails part-way through its own output says so with
* d_ostream_fail. d_ostream_close reports the first failure, as fclose
* reports a failed buffered write.
*
*   NOTHING HERE ALLOCATES, AND THERE IS NO <stdio.h>. Buffering is opt-in and
* uses the caller's storage. The FILE*, console and dj_string units are
* ostream_file.h, ostream_console.h and ostream_string.c, and a double's
* formatting and printf are text's (c/text/ostream_format.h), so a target
* without stdio keeps this header and its source.
*
*   A STREAM BELONGS TO ONE THREAD AT A TIME. Locking is one more transport
* wrapped around another.
*
*
* path:      /inc/djinterp/c/util/ostream/ostream.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.07
*                                                            revised: 2026.11.05
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Status and flags
         1.  dj_ostream_status
         2.  D_OSTREAM_OWNS_CONTEXT
         3.  D_OSTREAM_LINE_BUFFERED
         4.  D_OSTREAM_BINARY
    2.  The destination's operations
         1.  dj_ostream_info
         2.  dj_ostream_ops
    3.  The stream
         1.  dj_ostream
         2.  dj_ostream_cursor
    4.  Destinations kept here
         1.  D_OSTREAM_FIXED_SCRATCH
         2.  dj_ostream_fixed
         3.  dj_ostream_fanout_policy
         4.  dj_ostream_fanout
2.  LIFECYCLE
    ---------
    1.  Initialisation
    2.  Buffering
    3.  Flush and close
3.  OUTPUT
    ------
    1.  The slow path
    2.  Bytes and text
    3.  Writing in place
    4.  Cursors
    5.  Numbers
    6.  A producer's own failure
4.  STATE
    -----
    1.  Status
    2.  Capabilities
5.  DESTINATIONS
    ------------
    1.  Lowering to a sink
    2.  A fixed array, or a count
    3.  A dj_string
    4.  Several streams at once
    5.  For destination authors
*/

#ifndef DJINTERP_C_UTIL_OSTREAM_OSTREAM_H
#define DJINTERP_C_UTIL_OSTREAM_OSTREAM_H 1

// std
#include <stddef.h>  // size_t
#include <string.h>  // memcpy, strlen: the inline fast path
// djinterp
#include "../../djinterp.h"  // framework root
#include "../sink_common.h"  // struct dj_pack_sink, struct dj_pack_text
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"
                                                // int32_t; INT64_MAX, uint64_t

#if !defined(D_INLINE)
    #error "ostream.h needs D_INLINE; define it or set D_CFG_DEFINE_INLINE to 1"
#endif  // D_INLINE


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Status and flags
//------------------------------------------------------------------------------
// 1.1.1
// dj_ostream_status
//   enum: why a stream stopped. Only the first failure is kept.
enum dj_ostream_status
{
    D_OSTREAM_OK           = 0,
    D_OSTREAM_INVALID      = 1,  // a bad argument, or a stream never made
    D_OSTREAM_UNSUPPORTED  = 2,  // the destination cannot do what was asked
    D_OSTREAM_FAILED       = 3,  // the destination refused; see last_errno
    D_OSTREAM_CLOSED       = 4,  // written after d_ostream_close
    D_OSTREAM_STATUS_COUNT = 5
};

// 1.1.2
// D_OSTREAM_OWNS_CONTEXT
//   constant: flag: d_ostream_close asks the close slot to release the
// destination's context.
#define D_OSTREAM_OWNS_CONTEXT   0x0001

// 1.1.3
// D_OSTREAM_LINE_BUFFERED
//   constant: flag: a buffered transport is drained after every write that
// holds a newline. Its window stays collapsed, so every write takes the slow
// path, which is where the newline is looked for; a storage destination
// ignores it.
#define D_OSTREAM_LINE_BUFFERED  0x0002

// 1.1.4
// D_OSTREAM_BINARY
//   constant: flag: the bytes are binary. A destination that could translate
// them -- a console in text mode -- delivers them unaltered or refuses.
#define D_OSTREAM_BINARY         0x0004

// 1.2    The destination's operations
//------------------------------------------------------------------------------
// 1.2.1
// dj_ostream_info
//   struct: what a destination says about itself. Zero is the safe answer for
// every member, so a destination that cannot tell leaves it there.
struct dj_ostream_info
{
    int32_t is_terminal;     // an interactive console
    int32_t has_vt;          // ANSI/VT escape sequences are interpreted
    int32_t is_binary_safe;  // bytes arrive unaltered: no newline rewriting
    int32_t reserved;        // pad; 0
};

struct dj_ostream;

// 1.2.2
// dj_ostream_ops
//   struct: a destination's optional operations, one static const table per
// kind of destination. Each slot returns 1 on success and 0 on failure, with
// errno set where the destination knows why.
//
//   `size` MUST be set to sizeof(struct dj_ostream_ops) by whoever builds the
// table. It is the version field.
//
//   `grow` makes the destination a storage destination; section 5.5 gives its
// contract. The other three take the sink's context.
struct dj_ostream_ops
{
    size_t  size;

    // commit the window's bytes, then make room for _need more (5.5)
    int32_t (*grow)(struct dj_ostream* _os,
                    size_t             _need);

    // push anything held back on to the destination's own destination
    int32_t (*flush)(void* _context);

    // finish; and, when _release is non-zero, release the context
    int32_t (*close)(void*   _context,
                     int32_t _release);

    // describe the destination; *_out arrives zeroed
    int32_t (*query)(void*                   _context,
                     struct dj_ostream_info* _out);
};

// 1.3    The stream
//------------------------------------------------------------------------------
// 1.3.1
// dj_ostream
//   struct: the stream. A value the caller owns, on the stack if it likes.
// Read its state through section 4; the window's four pointers are public
// because the inline path and destination authors (5.5) use them.
//
//   Invariant: base <= cur <= end <= limit, all four in the same storage.
// [base, cur) holds bytes written and not yet delivered or committed;
// [cur, limit) is free; the inline path writes only below end, which is
// limit except where the window is collapsed (end == cur).
struct dj_ostream
{
    unsigned char*               cur;        // where the next byte goes
    unsigned char*               end;        // the inline path's limit
    unsigned char*               base;       // the first byte not yet delivered
    unsigned char*               limit;      // the end of the window's storage
    struct dj_pack_sink          sink;       // write (a transport's) + context
    const struct dj_ostream_ops* ops;        // optional; grow: storage
    unsigned char*               buffer;     // a transport's buffer; or null
    size_t                       capacity;   // its size in bytes
    size_t                       delivered;  // bytes before base, wrapping
    size_t                       wraps;      // how often delivered wrapped
    int32_t                      status;     // enum dj_ostream_status; sticky
    int32_t                      last_errno; // errno at the first failure
    int32_t                      flags;      // D_OSTREAM_*
    int32_t                      reserved;   // pad; 0
};

// 1.3.2
// dj_ostream_cursor
//   struct: a stream's window taken into a local value (3.4): where the next
// byte goes, and the limit of what may be written before the slow path.
struct dj_ostream_cursor
{
    unsigned char* cur;
    unsigned char* end;
};

// 1.4    Destinations kept here
//------------------------------------------------------------------------------
// 1.4.1
// D_OSTREAM_FIXED_SCRATCH
//   constant: the bytes a fixed destination keeps for counting what did not
// fit; each window of it is one call to its grow slot.
#define D_OSTREAM_FIXED_SCRATCH  128

// 1.4.2
// dj_ostream_fixed
//   struct: the state of a stream over a caller's array -- snprintf's
// contract: what fits is stored, the rest is counted, and d_ostream_written
// gives the length the whole output needs. With no array it only counts,
// which is the measure pass. The caller owns it; it must outlive the stream.
// Read it through d_ostream_fixed_length and d_ostream_fixed_truncated.
struct dj_ostream_fixed
{
    unsigned char* data;       // the array; may be null when capacity is 0
    size_t         capacity;   // its size, a terminator's byte included
    size_t         length;     // bytes stored, as of the last commit
    int32_t        terminate;  // non-zero: kept NUL-terminated at each commit
    int32_t        truncated;  // non-zero once a byte did not fit
    int32_t        spilling;   // non-zero once the window is the scratch
    int32_t        reserved;   // pad; 0
    unsigned char  scratch[D_OSTREAM_FIXED_SCRATCH];  // counted, not kept
};

// 1.4.3
// dj_ostream_fanout_policy
//   enum: when a fan-out stream counts as failed. Each target keeps its own
// status either way, so the caller can see which one died.
enum dj_ostream_fanout_policy
{
    D_OSTREAM_FANOUT_ALL = 0,  // when any target fails (the default)
    D_OSTREAM_FANOUT_ANY = 1   // only when every target has failed
};

// 1.4.4
// dj_ostream_fanout
//   struct: the context of a stream that copies every write to several other
// streams. The caller owns it, its array and the targets, all of which must
// outlive the stream; closing the fan-out flushes the targets and closes none.
struct dj_ostream_fanout
{
    struct dj_ostream** targets;
    size_t              count;
    int32_t             policy;    // enum dj_ostream_fanout_policy
    int32_t             reserved;  // pad; 0
};

D_STATIC_ASSERT(D_OSTREAM_OK == 0,
                "dj_ostream: a zeroed status must read as a healthy stream");
D_STATIC_ASSERT(sizeof(struct dj_ostream_info) == (4 * sizeof(int32_t)),
                "dj_ostream_info: four int32_t members and no padding");
D_STATIC_ASSERT(sizeof(unsigned char) == 1,
                "dj_ostream: the window counts bytes as unsigned char");


//==============================================================================
// 2.  LIFECYCLE
//==============================================================================


// 2.1    Initialisation
//------------------------------------------------------------------------------
/**
 * @brief Makes a stream over a destination: a transport when _sink.write is
 *        set, a storage destination when _ops->grow is.
 *
 * @note On failure the stream is left D_OSTREAM_INVALID (D_OSTREAM_FAILED when
 *       a storage destination's first grow fails) and owns nothing: a context
 *       it was asked to own is still the caller's to release. A transport
 *       starts unbuffered; give it a buffer with d_ostream_set_buffer.
 *
 * @param[out] _os     the stream.
 * @param[in]  _sink   the destination: its write, or null for a storage
 *                     destination, and its context.
 * @param[in]  _ops    the destination's operations, or null for a transport
 *                     with none.
 * @param[in]  _flags  D_OSTREAM_OWNS_CONTEXT, D_OSTREAM_LINE_BUFFERED and
 *                     D_OSTREAM_BINARY, or 0; other bits are ignored.
 * @return 1, or 0 for a null _os, a table smaller than its first version,
 *         neither or both of a write and a grow, D_OSTREAM_OWNS_CONTEXT
 *         without a close slot, or a failed first grow.
 */
int32_t d_ostream_init(struct dj_ostream*           _os,
                       struct dj_pack_sink          _sink,
                       const struct dj_ostream_ops* _ops,
                       int32_t                      _flags);

// 2.2    Buffering
//------------------------------------------------------------------------------
/**
 * @brief Gives a transport a buffer, or takes it away.
 *
 * @note Whatever the old buffer holds is delivered first. A null _buffer or a
 *       zero _capacity leaves the stream unbuffered. A storage destination
 *       has its own window and keeps it.
 *
 * @param[in,out] _os        the stream.
 * @param[in]     _buffer    storage the caller keeps alive while the stream
 *                           uses it.
 * @param[in]     _capacity  its size in bytes.
 * @return 1, or 0 if the stream cannot be written, the old buffer could not
 *         be delivered, or the destination is a storage destination.
 */
int32_t d_ostream_set_buffer(struct dj_ostream* _os,
                             void*              _buffer,
                             size_t             _capacity);

// 2.3    Flush and close
//------------------------------------------------------------------------------
/**
 * @brief Delivers or commits the window, then asks the destination to flush.
 *
 * @note After it, a storage destination is current: a dj_string's size counts
 *       every byte written, and a fixed array is terminated.
 *
 * @param[in,out] _os  the stream.
 * @return 1, or 0 if the stream had failed or the flush failed.
 */
int32_t d_ostream_flush(struct dj_ostream* _os);
/**
 * @brief Delivers or commits the window, finishes the destination and, when
 *        the stream owns it, releases it.
 *
 * @note Idempotent: a second call does nothing and reports the same result.
 *       An owned context is released even when the stream had failed.
 *
 * @param[in,out] _os  the stream.
 * @return 1 when the stream never failed, 0 otherwise.
 */
int32_t d_ostream_close(struct dj_ostream* _os);


//==============================================================================
// 3.  OUTPUT
//==============================================================================
//   The inline functions here require a stream d_ostream_init has seen; a
// null stream is a contract violation they do not test for, as putc's is.
// Every out-of-line function tests for one.


// 3.1    The slow path
//------------------------------------------------------------------------------
//   Called by the inline functions when the window cannot take a write, and
// directly by anyone who wants a write that skips the window test. A
// zero-length write touches nothing and returns whether the stream can be
// written -- the readiness check; a write to a closed stream records
// D_OSTREAM_CLOSED. The one-character forms take the character by value, so
// the inline path never has to store it to memory for the slow path's sake
// -- a store that, in a loop, costs as much as the write itself.
/**
 * @brief Writes bytes the window could not take.
 *
 * @param[in,out] _os    the stream.
 * @param[in]     _data  the bytes; may be null only when _size is 0.
 * @param[in]     _size  how many; 0 asks only whether the stream is ready.
 * @return 1 when every byte was accepted (or, for 0, the stream is ready),
 *         0 otherwise.
 */
int32_t d_ostream_write_slow(struct dj_ostream* _os,
                             const void*        _data,
                             size_t             _size);
/**
 * @brief Writes one character the window could not take.
 *
 * @param[in,out] _os  the stream.
 * @param[in]     _c   the character, as an unsigned char's value.
 * @return as d_ostream_write_slow().
 */
int32_t d_ostream_write_char_slow(struct dj_ostream* _os,
                                  int                _c);
/**
 * @brief Makes room the window did not have: grows a storage destination, or
 *        drains a transport's buffer.
 *
 * @param[in,out] _os    the stream.
 * @param[in]     _need  the bytes wanted.
 * @return the room, or null when there is none to give or the stream failed.
 */
char*   d_ostream_reserve_slow(struct dj_ostream* _os,
                               size_t             _need);
/**
 * @brief Finishes a commit past a collapsed window: a line-buffered stream
 *        is drained when the bytes committed hold a newline.
 *
 * @param[in,out] _os  the stream.
 * @return 1, or 0 when the stream has failed.
 */
int32_t d_ostream_commit_slow(struct dj_ostream* _os);
/**
 * @brief Writes bytes a cursor could not take, then takes the window again.
 *
 * @param[in,out] _os      the stream the cursor was taken from.
 * @param[in]     _cursor  the cursor, whose writes go back to the stream.
 * @param[in]     _data    the bytes.
 * @param[in]     _size    how many.
 * @return the cursor to write through next; empty when the stream failed.
 */
struct dj_ostream_cursor
        d_ostream_cursor_write_slow(struct dj_ostream*       _os,
                                    struct dj_ostream_cursor _cursor,
                                    const void*              _data,
                                    size_t                   _size);
/**
 * @brief Writes one character a cursor could not take.
 *
 * @param[in,out] _os      the stream the cursor was taken from.
 * @param[in]     _cursor  the cursor.
 * @param[in]     _c       the character, as an unsigned char's value.
 * @return as d_ostream_cursor_write_slow().
 */
struct dj_ostream_cursor
        d_ostream_cursor_put_slow(struct dj_ostream*       _os,
                                  struct dj_ostream_cursor _cursor,
                                  int                      _c);

// 3.2    Bytes and text
//------------------------------------------------------------------------------
/**
 * @brief Writes bytes: a memcpy into the window when they fit.
 *
 * @note A zero-length write touches nothing and returns whether the stream
 *       can be written.
 *
 * @param[in,out] _os    the stream.
 * @param[in]     _data  the bytes; may be null only when _size is 0.
 * @param[in]     _size  how many.
 * @return 1 when every byte was accepted, 0 otherwise.
 */
D_INLINE int32_t
d_ostream_write(
    struct dj_ostream* _os,
    const void*        _data,
    size_t             _size
)
{
    unsigned char* const cur = _os->cur;

    // one comparison: _size - 1 wraps for 0, so an empty write goes slow too
    if ((size_t)(_size - 1u) < (size_t)(_os->end - cur))
    {
        memcpy(cur, _data, _size);
        _os->cur = cur + _size;

        return 1;
    }

    return d_ostream_write_slow(_os, _data, _size);
}

/**
 * @brief Writes one character.
 *
 * @param[in,out] _os  the stream.
 * @param[in]     _c   the character.
 * @return as d_ostream_write().
 */
D_INLINE int32_t
d_ostream_write_char(
    struct dj_ostream* _os,
    char               _c
)
{
    if (_os->cur < _os->end)
    {
        *_os->cur = (unsigned char)_c;
        ++_os->cur;

        return 1;
    }

    return d_ostream_write_char_slow(_os, (int)(unsigned char)_c);
}

/**
 * @brief Writes a NUL-terminated string, without its terminator.
 *
 * @note For a literal, the length is a constant and the write a fixed-size
 *       memcpy.
 *
 * @param[in,out] _os    the stream.
 * @param[in]     _cstr  the string; null is a bad argument.
 * @return as d_ostream_write().
 */
D_INLINE int32_t
d_ostream_write_cstr(
    struct dj_ostream* _os,
    const char*        _cstr
)
{
    // a null string reaches the slow path as null data, which records it
    if (_cstr == NULL)
    {
        return d_ostream_write_slow(_os, NULL, 1u);
    }

    return d_ostream_write(_os, _cstr, strlen(_cstr));
}

/**
 * @brief Writes a text span.
 *
 * @param[in,out] _os    the stream.
 * @param[in]     _text  the span; its length is authoritative.
 * @return as d_ostream_write().
 */
D_INLINE int32_t
d_ostream_write_text(
    struct dj_ostream*  _os,
    struct dj_pack_text _text
)
{
    return d_ostream_write(_os, _text.data, _text.length);
}

/**
 * @brief Writes one byte value _count times: padding, indentation, rules.
 *
 * @param[in,out] _os     the stream.
 * @param[in]     _c      the character.
 * @param[in]     _count  how many times.
 * @return as d_ostream_write().
 */
int32_t d_ostream_fill(struct dj_ostream* _os,
                       char               _c,
                       size_t             _count);

// 3.3    Writing in place
//------------------------------------------------------------------------------
//   A producer that formats -- digits, a date, an escape -- asks for room,
// writes into it, and commits what it used, so its bytes are written once,
// in place: .NET's GetSpan and Advance, Go's AvailableBuffer.
/**
 * @brief Room for at least _need bytes, written in place.
 *
 * @note No room is not a failure: a transport whose buffer is smaller than
 *       _need, or an unbuffered one, has none to give, and a fixed array
 *       near its end may not. Format into storage of your own and
 *       d_ostream_write it then. A failed or closed stream gives none.
 *
 * @param[in,out] _os    the stream.
 * @param[in]     _need  the bytes wanted; at least 1.
 * @post   the room is valid until the next call on the stream.
 * @return the room, or null when there is none.
 */
D_INLINE char*
d_ostream_reserve(
    struct dj_ostream* _os,
    size_t             _need
)
{
    if ((size_t)(_need - 1u) < (size_t)(_os->end - _os->cur))
    {
        return (char*)_os->cur;
    }

    return d_ostream_reserve_slow(_os, _need);
}

/**
 * @brief Keeps _used bytes of the room d_ostream_reserve gave.
 *
 * @param[in,out] _os    the stream.
 * @param[in]     _used  how many were written, from the room's start; no
 *                       more than were reserved.
 * @return 1, or 0 when a line-buffered stream's drain failed.
 */
D_INLINE int32_t
d_ostream_commit(
    struct dj_ostream* _os,
    size_t             _used
)
{
    _os->cur += _used;

    // only a collapsed window ends up past its end: a line-buffered stream
    if (_os->cur > _os->end)
    {
        return d_ostream_commit_slow(_os);
    }

    return 1;
}

// 3.4    Cursors
//------------------------------------------------------------------------------
//   The window, taken out of the stream into a value the caller keeps in a
// local. A run of writes through the stream reloads and stores the stream's
// pointers at every write, because a byte stored through the window may, for
// all the compiler knows, be a byte of the stream itself; a cursor's two
// pointers are a local value whose address is never taken, so they stay in
// registers and the run costs what writing to a char array costs. The
// cursor is passed and returned by value -- two pointers, returned in
// registers on the common ABIs -- for the same reason.
//
//   Take it, write through it, give it back; touch the stream through nothing
// else in between. A line-buffered stream lends its whole buffer to a cursor
// and looks for a newline once, when the cursor is given back, so a terminal
// keeps the fast path within a line. A failed stream lends an empty cursor,
// and every write through it reaches the slow path, which refuses it; the
// stream's status says so at the end, as for any write.

/**
 * @brief Takes the stream's window as a cursor.
 *
 * @param[in] _os  the stream.
 * @return the cursor; empty when the stream cannot be written.
 */
D_INLINE struct dj_ostream_cursor
d_ostream_cursor_take(
    struct dj_ostream* _os
)
{
    struct dj_ostream_cursor cursor;

    cursor.cur = _os->cur;
    cursor.end = ( ((_os->flags & D_OSTREAM_LINE_BUFFERED) != 0) &&
                   (_os->status == D_OSTREAM_OK) )
                     ? _os->limit
                     : _os->end;

    return cursor;
}

/**
 * @brief Gives a cursor's writes back to the stream.
 *
 * @param[in,out] _os      the stream the cursor was taken from.
 * @param[in]     _cursor  the cursor.
 * @return 1, or 0 when the stream has failed.
 */
D_INLINE int32_t
d_ostream_cursor_give(
    struct dj_ostream*       _os,
    struct dj_ostream_cursor _cursor
)
{
    _os->cur = _cursor.cur;

    // a line-buffered stream's batch lies past its collapsed end
    if (_os->cur > _os->end)
    {
        return d_ostream_commit_slow(_os);
    }

    return (_os->status == D_OSTREAM_OK) ? 1 : 0;
}

/**
 * @brief Writes bytes through a cursor.
 *
 * @param[in,out] _os      the stream the cursor was taken from.
 * @param[in]     _cursor  the cursor.
 * @param[in]     _data    the bytes; may be null only when _size is 0.
 * @param[in]     _size    how many.
 * @return the cursor to write through next.
 */
D_INLINE struct dj_ostream_cursor
d_ostream_cursor_write(
    struct dj_ostream*       _os,
    struct dj_ostream_cursor _cursor,
    const void*              _data,
    size_t                   _size
)
{
    if ((size_t)(_size - 1u) < (size_t)(_cursor.end - _cursor.cur))
    {
        memcpy(_cursor.cur, _data, _size);
        _cursor.cur += _size;

        return _cursor;
    }

    return d_ostream_cursor_write_slow(_os, _cursor, _data, _size);
}

/**
 * @brief Writes one character through a cursor.
 *
 * @param[in,out] _os      the stream the cursor was taken from.
 * @param[in]     _cursor  the cursor.
 * @param[in]     _c       the character.
 * @return the cursor to write through next.
 */
D_INLINE struct dj_ostream_cursor
d_ostream_cursor_put(
    struct dj_ostream*       _os,
    struct dj_ostream_cursor _cursor,
    char                     _c
)
{
    if (_cursor.cur < _cursor.end)
    {
        *_cursor.cur = (unsigned char)_c;
        ++_cursor.cur;

        return _cursor;
    }

    return d_ostream_cursor_put_slow(_os,
                                     _cursor,
                                     (int)(unsigned char)_c);
}

// 3.5    Numbers
//------------------------------------------------------------------------------
//   Integers are converted here, two digits per division, into the window in
// place when it has room, so no locale reaches them; a double is text's
// (c/text/ostream_format.h). Each writer returns as d_ostream_write(): 1 when
// every digit was accepted, 0 when the stream refused them.
/**
 * @brief Writes an int in decimal.
 *
 * @param[in,out] _os     the stream.
 * @param[in]     _value  the value.
 * @return as d_ostream_write().
 */
int32_t d_ostream_write_int(struct dj_ostream* _os,
                            int                _value);
/**
 * @brief Writes an unsigned int in decimal.
 *
 * @param[in,out] _os     the stream.
 * @param[in]     _value  the value.
 * @return as d_ostream_write().
 */
int32_t d_ostream_write_uint(struct dj_ostream* _os,
                             unsigned int       _value);
/**
 * @brief Writes a long in decimal.
 *
 * @param[in,out] _os     the stream.
 * @param[in]     _value  the value.
 * @return as d_ostream_write().
 */
int32_t d_ostream_write_long(struct dj_ostream* _os,
                             long               _value);
/**
 * @brief Writes an unsigned long in decimal.
 *
 * @param[in,out] _os     the stream.
 * @param[in]     _value  the value.
 * @return as d_ostream_write().
 */
int32_t d_ostream_write_ulong(struct dj_ostream* _os,
                              unsigned long      _value);
/**
 * @brief Writes a size_t in decimal, whatever its width.
 *
 * @param[in,out] _os     the stream.
 * @param[in]     _value  the value.
 * @return as d_ostream_write().
 */
int32_t d_ostream_write_size(struct dj_ostream* _os,
                             size_t             _value);
/**
 * @brief Writes a size_t in lower-case hexadecimal, without a prefix.
 *
 * @param[in,out] _os      the stream.
 * @param[in]     _value   the value.
 * @param[in]     _digits  the width to pad to with zeros; 0, or more than a
 *                         size_t has, for none.
 * @return as d_ostream_write().
 */
int32_t d_ostream_write_hex(struct dj_ostream* _os,
                            size_t             _value,
                            int32_t            _digits);

// 64-bit floor: these two need a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict C++98
// on a 32-bit target -- they are absent (the owner's ruling of 2026.10.03 on
// round 3's question 1, (a)); every C build has them.
#if defined(INT64_MAX)
/**
 * @brief Writes an int64_t in decimal.
 *
 * @param[in,out] _os     the stream.
 * @param[in]     _value  the value.
 * @return as d_ostream_write().
 */
int32_t d_ostream_write_int64(struct dj_ostream* _os,
                              int64_t            _value);
/**
 * @brief Writes a uint64_t in decimal.
 *
 * @param[in,out] _os     the stream.
 * @param[in]     _value  the value.
 * @return as d_ostream_write().
 */
int32_t d_ostream_write_uint64(struct dj_ostream* _os,
                               uint64_t           _value);
#endif  // INT64_MAX

// 3.6    A producer's own failure
//------------------------------------------------------------------------------
/**
 * @brief Records a producer's own failure, so a stream that is missing output
 *        says so.
 *
 * @note Kept only when the stream has not already failed.
 *
 * @param[in,out] _os      the stream.
 * @param[in]     _status  D_OSTREAM_INVALID, D_OSTREAM_UNSUPPORTED or
 *                         D_OSTREAM_FAILED; any other value records FAILED.
 * @param[in]     _error   the errno value that explains it, or 0.
 * @return 0, so a producer can `return d_ostream_fail(...)`.
 */
int32_t d_ostream_fail(struct dj_ostream* _os,
                       int32_t            _status,
                       int32_t            _error);


//==============================================================================
// 4.  STATE
//==============================================================================


// 4.1    Status
//------------------------------------------------------------------------------
//   None of these touches the destination; all take a null stream: the first
// failure (an enum dj_ostream_status; INVALID for null), the errno recorded
// with it (0 when the destination did not say), and whether the stream can
// be written -- made, healthy and open -- as 1 or 0.
int32_t d_ostream_status(const struct dj_ostream* _os);
int32_t d_ostream_last_errno(const struct dj_ostream* _os);
int32_t d_ostream_is_ok(const struct dj_ostream* _os);
/**
 * @brief How many bytes the stream has accepted, buffered or delivered.
 *
 * @note A producer can take this as the offset of what it writes next, with
 *       no seek, so output can go down a pipe. Modulo SIZE_MAX + 1: a 32-bit
 *       program writing past 4 GiB reads d_ostream_written64.
 *
 * @param[in] _os  the stream.
 * @return the count; 0 for a null _os.
 */
size_t  d_ostream_written(const struct dj_ostream* _os);
#if defined(INT64_MAX)
// as d_ostream_written, without the wrap: every byte, on every target
uint64_t
        d_ostream_written64(const struct dj_ostream* _os);
#endif  // INT64_MAX

// 4.2    Capabilities
//------------------------------------------------------------------------------
/**
 * @brief Asks the destination to describe itself.
 *
 * @param[in]  _os   the stream.
 * @param[out] _out  receives the description; every member 0 on failure.
 * @return 1, or 0 when the destination has no query slot, the stream is
 *         closed or was never valid, or the query failed.
 */
int32_t d_ostream_query(const struct dj_ostream* _os,
                        struct dj_ostream_info*  _out);


//==============================================================================
// 5.  DESTINATIONS
//==============================================================================


// 5.1    Lowering to a sink
//------------------------------------------------------------------------------
/**
 * @brief A sink that writes through the stream, window and status included,
 *        for a producer that takes a struct dj_pack_sink.
 *
 * @param[in] _os  the stream, which must outlive the sink.
 * @return the sink.
 */
struct dj_pack_sink d_ostream_as_sink(struct dj_ostream* _os);

// 5.2    A fixed array, or a count
//------------------------------------------------------------------------------
/**
 * @brief Makes a stream over a caller's array, with snprintf's contract.
 *
 * @note What fits is stored; the rest is counted, so d_ostream_written gives
 *       the length the whole output needs and a caller can retry once with
 *       enough. A null _data or a zero _capacity only counts. With
 *       _terminate, one byte is kept for a NUL, written at every flush and
 *       at close, and at once.
 *
 * @param[out] _os         the stream.
 * @param[out] _fixed      its state, which must outlive it.
 * @param[out] _data       the array.
 * @param[in]  _capacity   its size in bytes.
 * @param[in]  _terminate  non-zero to keep the array NUL-terminated.
 * @return as d_ostream_init().
 */
int32_t d_ostream_init_fixed(struct dj_ostream*       _os,
                             struct dj_ostream_fixed* _fixed,
                             void*                    _data,
                             size_t                   _capacity,
                             int32_t                  _terminate);
// as of the last flush or close: the bytes stored, and whether any did not fit
size_t  d_ostream_fixed_length(const struct dj_ostream_fixed* _fixed);
int32_t d_ostream_fixed_truncated(const struct dj_ostream_fixed* _fixed);

// 5.3    A dj_string
//------------------------------------------------------------------------------
struct dj_string;

/**
 * @brief Makes a stream that appends to a dj_string, writing into its spare
 *        capacity in place.
 *
 * @note While the stream is open the string belongs to it: read it after
 *       d_ostream_flush, and change it only after d_ostream_close. Never
 *       write bytes that point into the string itself, as for
 *       d_string_append_buffer. Linking needs ostream_string.c and dstring.c.
 *
 * @param[out] _os      the stream.
 * @param[in]  _string  the target, which must outlive the stream.
 * @return as d_ostream_init().
 */
int32_t d_ostream_init_string(struct dj_ostream* _os,
                              struct dj_string*  _string);

// 5.4    Several streams at once
//------------------------------------------------------------------------------
/**
 * @brief Makes a stream that copies every write to each of _fanout's targets.
 *
 * @note Its capabilities are what every target has: a fan-out is a terminal
 *       only when all of its targets are. A target that is the stream itself
 *       is refused; a longer cycle is the caller's to avoid. Give it a buffer
 *       and the targets are written once per buffer, not once per write.
 *
 * @param[out] _os      the stream.
 * @param[in]  _fanout  the targets and policy, which must outlive the stream.
 * @return as d_ostream_init(); 0 as well for no targets, a null target or an
 *         unknown policy.
 */
int32_t d_ostream_init_fanout(struct dj_ostream*        _os,
                              struct dj_ostream_fanout* _fanout);

// 5.5    For destination authors
//------------------------------------------------------------------------------
//   A storage destination's grow slot is called with the window holding
// [base, cur), bytes the stream has already counted. It commits them to its
// storage, then -- when _need is not 0 -- makes room, and points the window
// at the free storage with d_ostream_window_set (or empties it with
// d_ostream_window_clear). A grow that gives less room than _need is not a
// failure: the stream writes what fits and calls grow again, and a grow that
// gives none fails the stream. _need of 0 asks only for the commit: flush and
// close send it, and d_ostream_init sends it to get the first window. Return
// 0, with errno set, when the storage cannot be had.
void    d_ostream_window_set(struct dj_ostream* _os,
                             void*              _begin,
                             size_t             _size);
void    d_ostream_window_clear(struct dj_ostream* _os);


D_EXTERN_C_END


#endif  // DJINTERP_C_UTIL_OSTREAM_OSTREAM_H
