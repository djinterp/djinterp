/*******************************************************************************
* djinterp [net]                                                    ftp_common.c
*
* Implementation of the shared vocabulary declared in ftp_common.h.
*   Buffer preparation and error descriptions.
*
*
* path:      /src/djinterp/net/ftp/ftp_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "../../../../inc/djinterp/net/ftp/ftp_common.h"  // corresponding header
// std
#include <stddef.h>  // size_t


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
    "timed out",                       // D_FTP_ERROR_TIMED_OUT
    "connection closed by peer",       // D_FTP_ERROR_CLOSED
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

/*
d_ftp_error_to_net
  One case per enumerator and no default, so the compiler reports a missing
case; the groups follow net.h's rules. A value outside the enum is UNKNOWN.
*/
enum d_net_error
d_ftp_error_to_net(
    enum d_ftp_error _error
)
{
    switch (_error)
    {
        case D_FTP_OK:
            return D_NET_OK;
        case D_FTP_ERROR_INVALID_ARGUMENT:
        case D_FTP_ERROR_BUFFER_TOO_SMALL:
            return D_NET_ERROR_INVALID_ARGUMENT;
        case D_FTP_ERROR_MALFORMED:
        case D_FTP_ERROR_TLS:
        case D_FTP_ERROR_COMMAND_UNRECOGNIZED:
        case D_FTP_ERROR_SYNTAX:
        case D_FTP_ERROR_BAD_SEQUENCE:
        case D_FTP_ERROR_UNEXPECTED_REPLY:
            return D_NET_ERROR_PROTOCOL;
        case D_FTP_ERROR_RESOLVE:
            return D_NET_ERROR_HOST_UNREACHABLE;
        case D_FTP_ERROR_CONNECT:
        case D_FTP_ERROR_DATA_CONNECTION:
            return D_NET_ERROR_CONNECTION_REFUSED;
        case D_FTP_ERROR_CLOSED:
        case D_FTP_ERROR_SERVICE_UNAVAILABLE:
        case D_FTP_ERROR_TRANSFER_ABORTED:
            return D_NET_ERROR_CLOSED;
        case D_FTP_ERROR_TIMED_OUT:
            return D_NET_ERROR_TIMED_OUT;
        case D_FTP_ERROR_OUT_OF_MEMORY:
            return D_NET_ERROR_OUT_OF_MEMORY;
        case D_FTP_ERROR_LOGIN_DENIED:
        case D_FTP_ERROR_ACCOUNT_REQUIRED:
        case D_FTP_ERROR_SECURITY:
            return D_NET_ERROR_ACCESS_DENIED;
        case D_FTP_ERROR_UNSUPPORTED:
        case D_FTP_ERROR_FILE_UNAVAILABLE:
        case D_FTP_ERROR_LOCAL_ERROR:
        case D_FTP_ERROR_INSUFFICIENT_STORAGE:
        case D_FTP_ERROR_NOT_IMPLEMENTED:
        case D_FTP_ERROR_PROTOCOL_UNSUPPORTED:
        case D_FTP_ERROR_PAGE_TYPE_UNKNOWN:
        case D_FTP_ERROR_NAME_NOT_ALLOWED:
        case D_FTP_ERROR_REJECTED:
        case D_FTP_ERROR_UNKNOWN:
            return D_NET_ERROR_UNKNOWN;
    }

    return D_NET_ERROR_UNKNOWN;
}

/*
d_ftp_error_from_net
  A switch over the errors net.h names, each group as its contract lists
it; the default catches the rest, and any value outside the enum, as
UNKNOWN.
*/
enum d_ftp_error
d_ftp_error_from_net(
    enum d_net_error _error
)
{
    switch (_error)
    {
        case D_NET_OK:
            return D_FTP_OK;

        case D_NET_ERROR_INVALID_ARGUMENT:
            return D_FTP_ERROR_INVALID_ARGUMENT;

        case D_NET_ERROR_OUT_OF_MEMORY:
            return D_FTP_ERROR_OUT_OF_MEMORY;

        case D_NET_ERROR_TIMED_OUT:
            return D_FTP_ERROR_TIMED_OUT;

        case D_NET_ERROR_CLOSED:
        case D_NET_ERROR_CONNECTION_RESET:
        case D_NET_ERROR_CONNECTION_ABORTED:
            return D_FTP_ERROR_CLOSED;

        case D_NET_ERROR_ADDRESS_INVALID:
        case D_NET_ERROR_HOST_UNREACHABLE:
            return D_FTP_ERROR_RESOLVE;

        case D_NET_ERROR_CONNECTION_REFUSED:
        case D_NET_ERROR_NETWORK_DOWN:
        case D_NET_ERROR_ACCESS_DENIED:
            return D_FTP_ERROR_CONNECT;

        case D_NET_ERROR_PROTOCOL:
            return D_FTP_ERROR_MALFORMED;

        default:
            break;
    }

    return D_FTP_ERROR_UNKNOWN;
}
