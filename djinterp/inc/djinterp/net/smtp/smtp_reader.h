/*******************************************************************************
* djinterp [net]                                                   smtp_reader.h
*
* djinterp SMTP line reader.
*   SMTP is line-oriented in both directions, and both engines read through
* this one reader: it buffers what a net.h connection delivers and hands out
* one line at a time, framed by net's line buffer. The connection is any
* d_net_connection -- a TCP connection from tcp.h, the view of a TLS session
* of the SSL kernel over one, a caller's own, or net_memory.h's pipe in a
* test -- so neither engine knows what carries its lines.
*   Writing needs nothing of SMTP's own: the engines write through
* d_net_write_all and judge what it reports with d_smtp_error_from_net.
*
*
* path:      /inc/djinterp/net/smtp/smtp_reader.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.10.13
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  LINE READER
    -----------
    1.  Reader type
         1.  D_SMTP_READER_BUFFER
         2.  d_smtp_reader
    2.  Reader operations
*/

#ifndef DJINTERP_NET_SMTP_SMTP_READER_H
#define DJINTERP_NET_SMTP_SMTP_READER_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../c/djinterp.h"     // framework root
#include "../codec/codec_line.h"  // d_net_line_buffer
#include "../net_common.h"        // d_net_connection, d_net_error
#include "./smtp_common.h"        // d_smtp_error


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  LINE READER
//==============================================================================
// A reader buffers connection reads and hands out one CRLF-terminated line
// at a time. A read that delivers bytes succeeds, whatever error came with
// them, as net.h allows: the bytes are kept, and a lasting failure is the
// next read's to report.


// 1.1    Reader type
//------------------------------------------------------------------------------
// 1.1.1
// D_SMTP_READER_BUFFER
//   constant: the bytes the reader holds, and so the most it requests from
// the connection per read. A longer line still arrives whole: the reader
// gathers it into the caller's line a buffer at a time.
#define D_SMTP_READER_BUFFER 4096u

// 1.1.2
// d_smtp_reader
//   struct: a connection and the bytes read from it but not yet consumed.
// The connection is borrowed, and may be changed between lines -- as an
// engine does when TLS takes over the stream -- once the buffer is empty.
// `net_error` is what the connection reported with the last failure it
// caused, D_NET_OK until one did: the cause behind D_SMTP_ERROR_TRANSPORT,
// and behind a timeout or an end the connection's own error brought about.
struct d_smtp_reader
{
    struct d_net_connection* connection;                    // borrowed
    enum d_net_error         net_error;                     // its cause
    struct d_net_line_buffer lines;                         // buffer's lines
    bool                     crlf;                          // last line: CRLF
    char                     buffer[D_SMTP_READER_BUFFER];
};

// 1.2    Reader operations
//------------------------------------------------------------------------------
// d_smtp_reader_init -- binds a reader to `_connection`, which it borrows,
// emptied and with no cause; a NULL reader is ignored
void              d_smtp_reader_init(struct d_smtp_reader*    _reader,
                                     struct d_net_connection* _connection);

/**
 * @brief Reads one line, without its terminator.
 *
 * @note CRLF ends a line; a bare LF is also accepted, as RFC 5321 section
 *       2.3.8 permits a receiver to. A bare CR inside a line is kept.
 *
 * @param[in,out] _reader    the reader.
 * @param[out]    _line      receives the line, NUL-terminated.
 * @param[in]     _capacity  size of `_line`, NUL included.
 * @param[out]    _length    receives the length stored.
 * @post `_reader->crlf` records whether the line ended with CRLF rather than
 *       a bare LF -- what a server needs to recognize end of data strictly.
 * @return D_SMTP_OK; D_SMTP_ERROR_INVALID_ARGUMENT for a NULL argument or a
 *         reader without a connection; D_SMTP_ERROR_LINE_TOO_LONG when the
 *         line did not fit -- the whole line has still been consumed and
 *         `_line` holds its beginning, so the stream stays in sync;
 *         D_SMTP_ERROR_CLOSED at the end of the stream; or what the
 *         connection reported, by d_smtp_error_from_net, `_line` then
 *         holding what arrived of the line.
 */
enum d_smtp_error d_smtp_reader_line(struct d_smtp_reader* _reader,
                                     char*                 _line,
                                     size_t                _capacity,
                                     size_t*               _length);

// buffered-byte control -- O(1), never fail. `pending` counts bytes read but
// not consumed; `discard` drops them, which a STARTTLS upgrade requires.
size_t            d_smtp_reader_pending(const struct d_smtp_reader* _reader);
void              d_smtp_reader_discard(struct d_smtp_reader* _reader);


D_EXTERN_C_END


#endif  // DJINTERP_NET_SMTP_SMTP_READER_H
