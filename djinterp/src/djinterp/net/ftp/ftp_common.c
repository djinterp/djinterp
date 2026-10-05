/*******************************************************************************
* djinterp [net]                                                    ftp_common.c
*
* Implementation of the shared vocabulary declared in ftp_common.h.
*   Buffer preparation, line splitting, and error descriptions.
*
*
* path:      /src/djinterp/net/ftp/ftp_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../../inc/djinterp/net/ftp/ftp_common.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <string.h>   // memchr


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// ERROR_MESSAGES
//   constant: d_ftp_error_string's text for each error, one row per
// enumerator in the enumeration's order; the assertion after it fails the
// build when an error is added or a row removed.
static const char* const ERROR_MESSAGES[] =
{
    "success",                         // D_FTP_OK
    "invalid argument",                // D_FTP_ERROR_INVALID_ARGUMENT
    "buffer too small",                // D_FTP_ERROR_BUFFER_TOO_SMALL
    "malformed input",                 // D_FTP_ERROR_MALFORMED
    "not supported",                   // D_FTP_ERROR_UNSUPPORTED
    "out of memory",                   // D_FTP_ERROR_OUT_OF_MEMORY
    "host name did not resolve",       // D_FTP_ERROR_RESOLVE
    "connection failed",               // D_FTP_ERROR_CONNECT
    "timed out",                       // D_FTP_ERROR_TIMEOUT
    "connection closed by peer",       // D_FTP_ERROR_CONNECTION_CLOSED
    "TLS failure",                     // D_FTP_ERROR_TLS
    "service not available",           // D_FTP_ERROR_SERVICE_UNAVAILABLE
    "cannot open data connection",     // D_FTP_ERROR_DATA_CONNECTION
    "transfer aborted",                // D_FTP_ERROR_TRANSFER_ABORTED
    "file unavailable",                // D_FTP_ERROR_FILE_UNAVAILABLE
    "server-side processing error",    // D_FTP_ERROR_LOCAL_ERROR
    "insufficient storage",            // D_FTP_ERROR_INSUFFICIENT_STORAGE
    "command not recognized",          // D_FTP_ERROR_COMMAND_UNRECOGNIZED
    "syntax error in arguments",       // D_FTP_ERROR_SYNTAX
    "command not implemented",         // D_FTP_ERROR_NOT_IMPLEMENTED
    "bad sequence of commands",        // D_FTP_ERROR_BAD_SEQUENCE
    "network protocol not supported",  // D_FTP_ERROR_PROTOCOL_UNSUPPORTED
    "not logged in",                   // D_FTP_ERROR_LOGIN_DENIED
    "account required",                // D_FTP_ERROR_ACCOUNT_REQUIRED
    "page type unknown",               // D_FTP_ERROR_PAGE_TYPE_UNKNOWN
    "file name not allowed",           // D_FTP_ERROR_NAME_NOT_ALLOWED
    "security exchange failed",        // D_FTP_ERROR_SECURITY
    "request rejected",                // D_FTP_ERROR_REJECTED
    "unexpected reply",                // D_FTP_ERROR_UNEXPECTED_REPLY
    "unknown error"                    // D_FTP_ERROR_UNKNOWN
};

D_STATIC_ASSERT( (sizeof(ERROR_MESSAGES) / sizeof(ERROR_MESSAGES[0])) ==
                 ((size_t)D_FTP_ERROR_UNKNOWN + 1u),
                 "every d_ftp_error needs a message" );

//==============================================================================
// 3.  TEXT AND ERRORS
//==============================================================================

/*
d_ftp_buffer_init
  A NULL `_storage` yields zero capacity, which every writer's validation
rejects, so a failed allocation upstream cannot become a wild write here.
*/
void
d_ftp_buffer_init(
    struct d_ftp_buffer* _buffer,
    char*                _storage,
    size_t               _capacity
)
{
    // parameter validation
    if (!_buffer)
    {
        return;
    }

    _buffer->data     = _storage;
    _buffer->capacity = (_storage) ? _capacity : 0u;
    _buffer->length   = 0u;

    // an empty buffer still holds a valid empty string
    if (_buffer->capacity > 0u)
    {
        _buffer->data[0] = '\0';
    }

    return;
}

/*
d_ftp_span_next_line
  One memchr() finds the LF; a CR before it is trimmed from the line rather
than searched for, so CR LF and bare-LF text split identically.
*/
bool
d_ftp_span_next_line(
    struct d_ftp_span* _cursor,
    struct d_ftp_span* _out_line
)
{
    // parameter validation; an exhausted cursor has no lines left
    if ( (!_cursor)               ||
         (!_out_line)             ||
         (!_cursor->data)         ||
         (_cursor->length == 0u) )
    {
        return false;
    }

    const char* const newline  = memchr(_cursor->data,
                                        '\n',
                                        _cursor->length);
    const size_t      length   = (newline)
                                 ? (size_t)(newline - _cursor->data)
                                 : _cursor->length;
    const size_t      consumed = (newline) ? (length + 1u) : length;

    _out_line->data   = _cursor->data;
    _out_line->length = length;

    // a CR before the LF belongs to the terminator
    if ( (length > 0u) &&
         (_out_line->data[length - 1u] == '\r') )
    {
        _out_line->length--;
    }

    _cursor->data   += consumed;
    _cursor->length -= consumed;

    return true;
}

/*
d_ftp_error_string
  A lookup in ERROR_MESSAGES, whose assertion keeps it complete as a switch
over every enumerator would under -Wswitch; values out of range fall back
to the unknown error's text.
*/
const char*
d_ftp_error_string(
    enum d_ftp_error _error
)
{
    const size_t index = (size_t)_error;

    // an out-of-range value is unknown by definition
    if (index > (size_t)D_FTP_ERROR_UNKNOWN)
    {
        return ERROR_MESSAGES[D_FTP_ERROR_UNKNOWN];
    }

    return ERROR_MESSAGES[index];
}
