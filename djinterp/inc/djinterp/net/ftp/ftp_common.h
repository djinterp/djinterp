/*******************************************************************************
* djinterp [net]                                                    ftp_common.h
*
* Shared vocabulary of the FTP protocol foundation.
*   Well-known ports, the CR LF terminator, the buffer type every module
* writes through, and the error enumeration; lines are split by net's
* codec_line.h. Nothing under net/ftp/ performs I/O or allocates: parsers
* return spans (d_pack_text) into caller memory, and writers fill caller
* buffers, leaving them unchanged on failure.
*
*
* path:      /inc/djinterp/net/ftp/ftp_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONSTANTS
    ---------
    1.  Well-known ports
         1.  D_FTP_PORT_CONTROL
         2.  D_FTP_PORT_DATA
         3.  D_FTP_PORT_IMPLICIT_TLS
         4.  D_FTP_PORT_IMPLICIT_TLS_DATA
    2.  Wire syntax
         1.  D_FTP_EOL
2.  TYPES
    -----
    1.  Text
         1.  d_ftp_buffer
    2.  Errors
         1.  d_ftp_error
3.  TEXT AND ERRORS
    ---------------
    1.  Buffers
    2.  Errors
*/

#ifndef DJINTERP_NET_FTP_FTP_COMMON_H
#define DJINTERP_NET_FTP_FTP_COMMON_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../c/djinterp.h"          // framework root
#include "../../c/util/sink_common.h"  // d_pack_text
#include "../codec/codec_line.h"       // d_net_line_next, for listings
#include "../net_common.h"             // d_net_error


// Linkage: declared inside D_EXTERN_C_BEGIN / D_EXTERN_C_END so that C++
// translation units link against the C definitions in ftp_common.c.
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  CONSTANTS
//==============================================================================
// Numbers and syntax the protocol fixes.


// 1.1    Well-known ports
//------------------------------------------------------------------------------
// 1.1.1
// D_FTP_PORT_CONTROL
//   constant: the control-connection port, 21 (RFC 959). Explicit FTPS uses
// it too, upgrading the plain connection with AUTH TLS.
#define D_FTP_PORT_CONTROL           21

// 1.1.2
// D_FTP_PORT_DATA
//   constant: the default data port, 20, from which an active-mode server
// connects.
#define D_FTP_PORT_DATA              20

// 1.1.3
// D_FTP_PORT_IMPLICIT_TLS
//   constant: the implicit-FTPS control port, 990, where TLS begins with the
// first byte instead of after AUTH TLS.
#define D_FTP_PORT_IMPLICIT_TLS      990

// 1.1.4
// D_FTP_PORT_IMPLICIT_TLS_DATA
//   constant: the implicit-FTPS default data port, 989.
#define D_FTP_PORT_IMPLICIT_TLS_DATA 989

// 1.2    Wire syntax
//------------------------------------------------------------------------------
// 1.2.1
// D_FTP_EOL
//   constant: the terminator of every command and reply line, CR LF, per the
// Telnet NVT convention the control connection follows.
#define D_FTP_EOL      "\r\n"


//==============================================================================
// 2.  TYPES
//==============================================================================
// Plain data, with no hidden state and nothing to destroy: callers declare
// these on the stack or embed them, and zero-initialization is always a valid
// starting point.


// 2.1    Text
//------------------------------------------------------------------------------
// Parsers hand back the parts of what they read as d_pack_text spans
// (c/util/sink_common.h): borrowed, unterminated, `data` NULL only when
// `length` is 0, and valid exactly as long as the caller's memory they point
// into.

// 2.1.1
// d_ftp_buffer
//   struct: caller-owned output storage. Writers append at `length`, never
// past `capacity`, and keep the text NUL-terminated, so `capacity` must exceed
// the longest text by one. A writer that cannot fit its whole output writes
// nothing and reports D_FTP_ERROR_BUFFER_TOO_SMALL. Prepare one with
// d_ftp_buffer_init().
struct d_ftp_buffer
{
    char*  data;      // storage; data[length] is always '\0'
    size_t capacity;  // size of `data` in bytes
    size_t length;    // bytes written, terminator excluded
};

// 2.2    Errors
//------------------------------------------------------------------------------
// 2.2.1
// d_ftp_error
//   enum: the library-neutral outcome of an FTP operation. This module reports
// the caller-side and parsing values itself; backends map native failures
// (errno, CURLcode, WinINet errors) onto the transport values, and negative
// replies onto the protocol values with d_ftp_error_from_reply().
enum d_ftp_error
{
    D_FTP_OK = 0,                      // success

    // caller side and parsing
    D_FTP_ERROR_INVALID_ARGUMENT,      // a NULL or out-of-range input
    D_FTP_ERROR_BUFFER_TOO_SMALL,      // output did not fit; nothing written
    D_FTP_ERROR_MALFORMED,             // input does not follow its grammar
    D_FTP_ERROR_UNSUPPORTED,           // well-formed, but not supported
    D_FTP_ERROR_OUT_OF_MEMORY,         // a backend allocation failed

    // transport
    D_FTP_ERROR_RESOLVE,               // the host name did not resolve
    D_FTP_ERROR_CONNECT,               // no connection could be made
    D_FTP_ERROR_TIMED_OUT,             // an operation outlived its deadline
    D_FTP_ERROR_CLOSED,                // the peer closed a connection early
    D_FTP_ERROR_TLS,                   // TLS negotiation or verification

    // protocol, mapped from negative replies
    D_FTP_ERROR_SERVICE_UNAVAILABLE,   // 421
    D_FTP_ERROR_DATA_CONNECTION,       // 425
    D_FTP_ERROR_TRANSFER_ABORTED,      // 426
    D_FTP_ERROR_FILE_UNAVAILABLE,      // 450, 550
    D_FTP_ERROR_LOCAL_ERROR,           // 451
    D_FTP_ERROR_INSUFFICIENT_STORAGE,  // 452, 552
    D_FTP_ERROR_COMMAND_UNRECOGNIZED,  // 500
    D_FTP_ERROR_SYNTAX,                // 501
    D_FTP_ERROR_NOT_IMPLEMENTED,       // 502, 504
    D_FTP_ERROR_BAD_SEQUENCE,          // 503
    D_FTP_ERROR_PROTOCOL_UNSUPPORTED,  // 522
    D_FTP_ERROR_LOGIN_DENIED,          // 530
    D_FTP_ERROR_ACCOUNT_REQUIRED,      // 532
    D_FTP_ERROR_PAGE_TYPE_UNKNOWN,     // 551
    D_FTP_ERROR_NAME_NOT_ALLOWED,      // 553
    D_FTP_ERROR_SECURITY,              // 431, 533 through 537
    D_FTP_ERROR_REJECTED,              // any other negative reply
    D_FTP_ERROR_UNEXPECTED_REPLY,      // a reply the exchange did not allow

    D_FTP_ERROR_UNKNOWN                // anything else
};


//==============================================================================
// 3.  TEXT AND ERRORS
//==============================================================================


// 3.1    Buffers
//------------------------------------------------------------------------------
/**
 * @brief Prepares an output buffer over caller storage, empty and terminated.
 *
 * @param[out] _buffer   the buffer to prepare.
 * @param[in]  _storage  its storage; NULL gives a buffer every writer
 *                       rejects.
 * @param[in]  _capacity the size of `_storage` in bytes.
 */
void d_ftp_buffer_init(struct d_ftp_buffer* _buffer,
                       char*                _storage,
                       size_t               _capacity);

// 3.2    Errors
//------------------------------------------------------------------------------
/**
 * @brief Describes an error in a short, static, English phrase.
 *
 * @param[in] _error the error.
 * @return the description; never NULL.
 */
const char* d_ftp_error_string(enum d_ftp_error _error);
/**
 * @brief Maps an error onto d_net_error, by the rules net.h gives with it.
 *
 * @param[in] _error the error.
 * @return the d_net_error it maps to; D_NET_ERROR_UNKNOWN for a value
 *         outside the enum.
 */
enum d_net_error d_ftp_error_to_net(enum d_ftp_error _error);
/**
 * @brief Gives the error a connection's d_net_error is here, by net.h's
 *        rules the other way.
 *
 * D_NET_OK is D_FTP_OK; INVALID_ARGUMENT, OUT_OF_MEMORY and TIMED_OUT are
 * their namesakes; a stream that ended or was lost -- CLOSED,
 * CONNECTION_RESET, CONNECTION_ABORTED -- is CLOSED; an address that names
 * no host -- ADDRESS_INVALID, HOST_UNREACHABLE -- is RESOLVE; a connection
 * refused -- CONNECTION_REFUSED, NETWORK_DOWN, ACCESS_DENIED -- is CONNECT;
 * PROTOCOL is MALFORMED; anything else, an unknown value included, is
 * UNKNOWN. d_ftp_error_to_net of a transport error comes back to it.
 *
 * @param[in] _error the connection's error.
 * @return the FTP error it is.
 */
enum d_ftp_error d_ftp_error_from_net(enum d_net_error _error);


D_EXTERN_C_END


#endif  // DJINTERP_NET_FTP_FTP_COMMON_H
