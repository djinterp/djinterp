/*******************************************************************************
* djinterp [net]                                                    codec_line.h
*
* Line framing for net's line-oriented protocols: POP3, SMTP, the STARTTLS
* negotiations, FTP's replies and listings.
*   It supplies
*     - lines found in a text, and a text split into its lines          [1]
*     - checks for the bytes that would end or cut a line               [2]
*     - a line buffer that does no I/O of its own                       [3]
*   A line ends at LF, and a CR just before the LF belongs to the
* terminator. What else a line may hold, and whether a bare LF may end one,
* is each protocol's rule: the framing reports what it found -- the line,
* and whether CR LF ended it -- and the caller judges. The buffer reads
* nothing itself: a module asks it for a line, and when none is whole, asks
* where to read the next bytes to and reads them through its own transport.
*
*
* path:      /inc/djinterp/net/codec/codec_line.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.12
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  LINES
    -----
    1.  The line
         1.  d_net_line
    2.  Finding and splitting
2.  LINE SAFETY
    -----------
    1.  Checks
3.  LINE BUFFER
    -----------
    1.  Types
         1.  d_net_line_buffer
         2.  d_net_line_found
    2.  Lines
    3.  Bytes
*/

#ifndef DJINTERP_NET_CODEC_CODEC_LINE_H
#define DJINTERP_NET_CODEC_CODEC_LINE_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../c/djinterp.h"          // framework root
#include "../../c/util/sink_common.h"  // d_pack_text


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  LINES
//==============================================================================


// 1.1    The line
//------------------------------------------------------------------------------
// 1.1.1
// d_net_line
//   struct: a line found at the start of a text: the line without its
// terminator, the bytes it took with the terminator, and whether that
// terminator was CR LF rather than LF alone.
struct d_net_line
{
    struct d_pack_text text;  // the line; a CR before its LF is not in it
    size_t             size;  // bytes from its start through its LF
    bool               crlf;  // CR LF ended it, not a bare LF
};

// 1.2    Finding and splitting
//------------------------------------------------------------------------------
/**
 * @brief Finds the line at the start of `_text`, when its LF has arrived.
 *
 * @note Only the bytes from `_from` on are searched for the LF, so a caller
 *       whose text grows a little at a time need not search any byte twice;
 *       the CR before the LF is looked for wherever it is.
 *
 * @param[in]  _text  the text, the line at its start; its `data` may be
 *                    `NULL` when its `length` is 0.
 * @param[in]  _from  the offset the search starts at: bytes before it are
 *                    known to hold no LF.
 * @param[out] _out   receives the line.
 * @return `true` when an LF ends a line; `false`, leaving `_out` alone, when
 *         none has arrived, `_from` is past the text, or an argument is
 *         missing.
 */
bool               d_net_line_find(struct d_pack_text _text,
                                   size_t             _from,
                                   struct d_net_line* _out);
/**
 * @brief Takes the next line of a whole text, and moves past it.
 *
 * @note The text's last line needs no LF. A CR that ends a line belongs to
 *       its terminator, the last line's included, so CR LF and bare-LF text
 *       split alike; a trailing LF ends a line and starts none.
 *
 * @param[in,out] _cursor    the text not yet split; it moves past the line
 *                           and its terminator.
 * @param[out]    _out_line  receives the line, without its terminator.
 * @return `true` with a line; `false` when the cursor is empty, or an
 *         argument is missing.
 */
bool               d_net_line_next(struct d_pack_text* _cursor,
                                   struct d_pack_text* _out_line);
/**
 * @brief The text without the CR and LF bytes that end it, however many.
 *
 * @note Blanks before them stay: in a listing they may belong to a name.
 *
 * @param[in] _text  the text; one that is `NULL` comes back as it went.
 * @return the part of `_text` before its trailing CR and LF bytes.
 */
struct d_pack_text d_net_line_chomp(struct d_pack_text _text);


//==============================================================================
// 2.  LINE SAFETY
//==============================================================================
// Text that is to become part of a line -- a command's argument, a reply's
// text -- must hold no byte that ends one, or a peer reads the rest as a
// line of its own: the command injection every line protocol has to refuse.
// A NUL cuts the text short wherever it meets a C string.


// 2.1    Checks
//------------------------------------------------------------------------------
// predicates -- never fail; a span that is NULL but claims characters holds
// a break and is not clean:
//   has_break  whether the text holds a CR or an LF
//   is_clean   whether it holds no CR, no LF, and no NUL
bool               d_net_line_has_break(struct d_pack_text _text);
bool               d_net_line_is_clean(struct d_pack_text _text);


//==============================================================================
// 3.  LINE BUFFER
//==============================================================================
// The bytes a module has read from a stream and not yet consumed, kept in
// storage the module owns, and the lines among them. The buffer holds offsets
// only -- the storage is passed to every call that reads or writes it -- so a
// structure holding both may be copied or moved.
//   A module asks for a line; when none is whole it asks for the room to read
// into, reads through its own transport, and commits the count read. What a
// line too long for the storage becomes is the module's choice: an error, a
// line skipped, or a line delivered a piece at a time.


// 3.1    Types
//------------------------------------------------------------------------------
// 3.1.1
// d_net_line_buffer
//   struct: the state of a line buffer over `capacity` bytes of storage:
// the unconsumed bytes are those from `start` to `end`, and the first
// `scanned` of them are known to hold no LF.
struct d_net_line_buffer
{
    size_t capacity;  // the storage's size
    size_t start;     // the first byte not consumed
    size_t end;       // one past the last byte held
    size_t scanned;   // bytes from `start` searched for an LF and not found
};

// 3.1.2
// d_net_line_found
//   enum: what the bytes held make: a whole line, or not yet one with room
// for more, or not yet one with the storage full.
enum d_net_line_found
{
    D_NET_LINE_FOUND = 0,  // a line, which the caller consumes
    D_NET_LINE_MORE  = 1,  // no LF yet, and room to read more into
    D_NET_LINE_FULL  = 2   // no LF yet, and the storage is full of the line
};

// 3.2    Lines
//------------------------------------------------------------------------------
/**
 * @brief Readies a buffer over `_capacity` bytes of storage, empty.
 *
 * @param[out] _buffer    the buffer.
 * @param[in]  _capacity  the storage's size; at least 2, so that a line's
 *                        CR LF fits.
 */
void
d_net_line_buffer_init(struct d_net_line_buffer* _buffer,
                       size_t                    _capacity);
/**
 * @brief Finds the first line among the bytes held.
 *
 * @param[in,out] _buffer   the buffer; it remembers how far it searched.
 * @param[in]     _storage  its storage.
 * @param[out]    _out      receives the line, which borrows `_storage`; it
 *                          is not consumed until d_net_line_buffer_consume()
 *                          is given its `size`.
 * @return D_NET_LINE_FOUND with a line; D_NET_LINE_MORE or D_NET_LINE_FULL,
 *         leaving `_out` alone, when no LF has arrived.
 */
enum d_net_line_found
d_net_line_buffer_find(struct d_net_line_buffer* _buffer,
                       const char*               _storage,
                       struct d_net_line*        _out);
/**
 * @brief Takes what is held of a line not yet whole, all but a CR that ends
 *        it, which may be the first half of the line's CR LF.
 *
 * @param[in,out] _buffer   the buffer; the bytes returned are consumed.
 * @param[in]     _storage  its storage.
 * @return the bytes taken, borrowing `_storage` until the next
 *         d_net_line_buffer_room().
 */
struct d_pack_text
d_net_line_buffer_spill(struct d_net_line_buffer* _buffer,
                        const char*               _storage);

// 3.3    Bytes
//------------------------------------------------------------------------------
/**
 * @brief The free bytes the next read may fill.
 *
 * @param[in,out] _buffer   the buffer.
 * @param[in,out] _storage  its storage.
 * @param[out]    _room     receives the count of free bytes; 0 only when
 *                          the storage is full.
 * @post the bytes held may have moved to the front of the storage, when they
 *       had reached its end: any line or text borrowed from it before the
 *       call is invalid.
 * @return where the free bytes begin, in `_storage`.
 */
char*
d_net_line_buffer_room(struct d_net_line_buffer* _buffer,
                       char*                     _storage,
                       size_t*                   _room);
// the bytes held -- O(1), never fail. `commit` adds the `_count` bytes a read
// put in the room, no more than it gave; `held` is the bytes not consumed,
// borrowing the storage, and `pending` their count; `consume` drops the
// first `_count` of them, no more than are held; `clear` drops them all.
void
d_net_line_buffer_commit(struct d_net_line_buffer* _buffer,
                         size_t                    _count);
struct d_pack_text
d_net_line_buffer_held(const struct d_net_line_buffer* _buffer,
                       const char*                     _storage);
size_t
d_net_line_buffer_pending(const struct d_net_line_buffer* _buffer);
void
d_net_line_buffer_consume(struct d_net_line_buffer* _buffer,
                          size_t                    _count);
void
d_net_line_buffer_clear(struct d_net_line_buffer* _buffer);


D_EXTERN_C_END


#endif  // DJINTERP_NET_CODEC_CODEC_LINE_H
