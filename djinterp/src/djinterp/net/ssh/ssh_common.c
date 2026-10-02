/*******************************************************************************
* djinterp [net]                                                    ssh_common.c
*
*   Definitions for ssh_common.h, and the byte, text, and status helpers
* every SSH source file shares through ssh_internal.h.
*
*
* path:      /src/djinterp/net/ssh/ssh_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/
#include "../../../../inc/djinterp/net/ssh/ssh_common.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // uint32_t, uint64_t
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"            // framework root
#include "../../../../inc/djinterp/net/ssh/ssh_internal.h"  // d_ssh_internal_*


/*
d_ssh_internal_lower
  Internal: ASCII lowercase, independent of the locale, as host matching
must be.
*/
char
d_ssh_internal_lower(
    char _c
)
{
    // only the 26 ASCII capitals change
    if ( (_c >= 'A') &&
         (_c <= 'Z') )
    {
        return (char)(_c - 'A' + 'a');
    }

    return _c;
}

/*
d_ssh_internal_load32
  Internal: reads a big-endian uint32.
*/
uint32_t
d_ssh_internal_load32(
    const unsigned char* _bytes
)
{
    return ( ((uint32_t)_bytes[0] << 24) |
             ((uint32_t)_bytes[1] << 16) |
             ((uint32_t)_bytes[2] << 8)  |
             ((uint32_t)_bytes[3]) );
}

/*
d_ssh_internal_store32
  Internal: writes a big-endian uint32.
*/
void
d_ssh_internal_store32(
    unsigned char* _bytes,
    uint32_t       _value
)
{
    _bytes[0] = (unsigned char)(_value >> 24);
    _bytes[1] = (unsigned char)(_value >> 16);
    _bytes[2] = (unsigned char)(_value >> 8);
    _bytes[3] = (unsigned char)(_value);

    return;
}

/*
d_ssh_internal_store64
  Internal: writes a big-endian uint64.
*/
void
d_ssh_internal_store64(
    unsigned char* _bytes,
    uint64_t       _value
)
{
    d_ssh_internal_store32(_bytes, (uint32_t)(_value >> 32));
    d_ssh_internal_store32(_bytes + 4, (uint32_t)(_value & 0xFFFFFFFFu));

    return;
}

/*
d_ssh_internal_name_list_valid
  Internal: whether `_length` bytes form an RFC 4251 name-list: empty, or
non-empty names of printable US-ASCII separated by single commas.
*/
bool
d_ssh_internal_name_list_valid(
    const char* _list,
    size_t      _length
)
{
    // the empty list names nothing, validly
    if (_length == 0)
    {
        return true;
    }

    // a comma at either end would bound an empty name
    if ( (_list[0] == ',') ||
         (_list[_length - 1] == ',') )
    {
        return false;
    }

    // printable US-ASCII only, and never two commas together
    for (size_t i = 0; i < _length; i++)
    {
        const unsigned char c = (unsigned char)_list[i];

        if ( (c < 0x21)          ||
             (c > 0x7E)          ||
             ( (c == ',') &&
               (_list[i + 1] == ',') ) )
        {
            return false;
        }
    }

    return true;
}

/*
d_ssh_internal_is_fatal
  Internal: whether a status leaves a session unusable. Only these ever
change a session's state to D_SSH_STATE_FAILED.
*/
bool
d_ssh_internal_is_fatal(
    enum d_ssh_status _status
)
{
    switch (_status)
    {
        case D_SSH_ERR_MEMORY:
        case D_SSH_ERR_IO:
        case D_SSH_ERR_TIMEOUT:
        case D_SSH_ERR_PROTOCOL:
        case D_SSH_ERR_HANDSHAKE:
        case D_SSH_ERR_HOST_UNKNOWN:
        case D_SSH_ERR_HOST_CHANGED:
        case D_SSH_ERR_HOST_REVOKED:
        case D_SSH_ERR_KNOWN_HOSTS:
            return true;

        default:
            return false;
    }
}

/*
d_ssh_status_string
  One name per status.
*/
const char*
d_ssh_status_string(
    enum d_ssh_status _status
)
{
    switch (_status)
    {
        case D_SSH_OK:
            return "ok";
        case D_SSH_ERR_ARGUMENT:
            return "invalid argument";
        case D_SSH_ERR_STATE:
            return "invalid state";
        case D_SSH_ERR_MEMORY:
            return "out of memory";
        case D_SSH_ERR_IO:
            return "I/O error";
        case D_SSH_ERR_CLOSED:
            return "stream ended";
        case D_SSH_ERR_TIMEOUT:
            return "timed out";
        case D_SSH_ERR_PROTOCOL:
            return "protocol violation";
        case D_SSH_ERR_FORMAT:
            return "malformed data";
        case D_SSH_ERR_NO_BACKEND:
            return "no SSH engine";
        case D_SSH_ERR_UNSUPPORTED:
            return "unsupported";
        case D_SSH_ERR_HANDSHAKE:
            return "handshake failed";
        case D_SSH_ERR_HOST_UNKNOWN:
            return "unknown host key";
        case D_SSH_ERR_HOST_CHANGED:
            return "host key changed";
        case D_SSH_ERR_HOST_REVOKED:
            return "host key revoked";
        case D_SSH_ERR_KNOWN_HOSTS:
            return "known_hosts failure";
        case D_SSH_ERR_AUTH_DENIED:
            return "authentication denied";
        case D_SSH_ERR_NO_AUTH_METHOD:
            return "no usable authentication method";
        case D_SSH_ERR_KEY:
            return "unusable private key";
        case D_SSH_ERR_CHANNEL:
            return "channel request refused";
    }

    return "unknown";
}
