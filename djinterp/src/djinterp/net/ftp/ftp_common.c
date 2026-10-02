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
*                                                            revised: 2026.09.26
*******************************************************************************/
#include "../../../../inc/djinterp/net/ftp/ftp_common.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <string.h>   // memchr


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
  A switch over every enumerator, so -Wswitch flags a new error that lacks a
description; the fallback after it covers out-of-range values.
*/
const char*
d_ftp_error_string(
    enum d_ftp_error _error
)
{
    switch (_error)
    {
        case D_FTP_OK:
            return "success";
        case D_FTP_ERROR_INVALID_ARGUMENT:
            return "invalid argument";
        case D_FTP_ERROR_BUFFER_TOO_SMALL:
            return "buffer too small";
        case D_FTP_ERROR_MALFORMED:
            return "malformed input";
        case D_FTP_ERROR_UNSUPPORTED:
            return "not supported";
        case D_FTP_ERROR_OUT_OF_MEMORY:
            return "out of memory";
        case D_FTP_ERROR_RESOLVE:
            return "host name did not resolve";
        case D_FTP_ERROR_CONNECT:
            return "connection failed";
        case D_FTP_ERROR_TIMEOUT:
            return "timed out";
        case D_FTP_ERROR_CONNECTION_CLOSED:
            return "connection closed by peer";
        case D_FTP_ERROR_TLS:
            return "TLS failure";
        case D_FTP_ERROR_SERVICE_UNAVAILABLE:
            return "service not available";
        case D_FTP_ERROR_DATA_CONNECTION:
            return "cannot open data connection";
        case D_FTP_ERROR_TRANSFER_ABORTED:
            return "transfer aborted";
        case D_FTP_ERROR_FILE_UNAVAILABLE:
            return "file unavailable";
        case D_FTP_ERROR_LOCAL_ERROR:
            return "server-side processing error";
        case D_FTP_ERROR_INSUFFICIENT_STORAGE:
            return "insufficient storage";
        case D_FTP_ERROR_COMMAND_UNRECOGNIZED:
            return "command not recognized";
        case D_FTP_ERROR_SYNTAX:
            return "syntax error in arguments";
        case D_FTP_ERROR_NOT_IMPLEMENTED:
            return "command not implemented";
        case D_FTP_ERROR_BAD_SEQUENCE:
            return "bad sequence of commands";
        case D_FTP_ERROR_PROTOCOL_UNSUPPORTED:
            return "network protocol not supported";
        case D_FTP_ERROR_LOGIN_DENIED:
            return "not logged in";
        case D_FTP_ERROR_ACCOUNT_REQUIRED:
            return "account required";
        case D_FTP_ERROR_PAGE_TYPE_UNKNOWN:
            return "page type unknown";
        case D_FTP_ERROR_NAME_NOT_ALLOWED:
            return "file name not allowed";
        case D_FTP_ERROR_SECURITY:
            return "security exchange failed";
        case D_FTP_ERROR_REJECTED:
            return "request rejected";
        case D_FTP_ERROR_UNEXPECTED_REPLY:
            return "unexpected reply";
        case D_FTP_ERROR_UNKNOWN:
            return "unknown error";
    }

    return "unknown error";
}
