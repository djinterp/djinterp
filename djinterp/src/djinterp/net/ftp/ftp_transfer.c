/*******************************************************************************
* djinterp [net]                                                  ftp_transfer.c
*
* Implementation of the transfer handling declared in ftp_transfer.h.
*   TYPE composition and parsing, the STRU and MODE codes, and the CR LF
* conversion state machines for both directions.
*
*
* path:      /src/djinterp/net/ftp/ftp_transfer.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/
#include "../../../../inc/djinterp/net/ftp/ftp_transfer.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // uint64_t
// djinterp
#include "../../../../inc/djinterp/net/ftp/ftp_common.h"  // d_ftp_span
#include "./ftp_internal.h"                               // shared helpers


//==============================================================================
// 3.  TRANSFER PARAMETERS
//==============================================================================

/*
d_ftp_type_format
  Composed in scratch storage sized for "L 255", so only the final copy into
`_out` can fail.
*/
enum d_ftp_error
d_ftp_type_format(
    const struct d_ftp_type* _type,
    struct d_ftp_buffer*     _out
)
{
    // parameter validation
    if ( (!_type) ||
         (!d_ftp_internal_buffer_ok(_out)) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    char                text[D_FTP_TYPE_ARGUMENT_SIZE] = { 0 };
    struct d_ftp_buffer local = { text, sizeof(text), 0u };

    switch (_type->data_type)
    {
        case D_FTP_TYPE_ASCII:
        case D_FTP_TYPE_EBCDIC:
            d_ftp_internal_append_char(&local,
                                       (char)_type->data_type);

            // a format control follows only when one is named
            if (_type->format != D_FTP_FORMAT_NONE)
            {
                // RFC 959 defines exactly three
                if ( (_type->format != D_FTP_FORMAT_NON_PRINT) &&
                     (_type->format != D_FTP_FORMAT_TELNET)    &&
                     (_type->format != D_FTP_FORMAT_CARRIAGE_CONTROL) )
                {
                    return D_FTP_ERROR_INVALID_ARGUMENT;
                }

                d_ftp_internal_append_char(&local,
                                           ' ');
                d_ftp_internal_append_char(&local,
                                           (char)_type->format);
            }

            break;

        case D_FTP_TYPE_IMAGE:
            d_ftp_internal_append_char(&local,
                                       'I');

            break;

        case D_FTP_TYPE_LOCAL:
            // the logical byte size is required and fits in a byte
            if ( (_type->byte_size == 0u) ||
                 (_type->byte_size > 255u) )
            {
                return D_FTP_ERROR_INVALID_ARGUMENT;
            }

            d_ftp_internal_append_text(&local,
                                       "L ");
            d_ftp_internal_append_uint(&local,
                                       _type->byte_size,
                                       0u);

            break;

        default:
            return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    return d_ftp_internal_commit(_out,
                                 &local);
}

/*
d_ftp_type_parse
  An unknown type code is UNSUPPORTED rather than MALFORMED, because RFC 959
answers it with 504 (parameter not implemented), not 501.
*/
enum d_ftp_error
d_ftp_type_parse(
    const char*        _text,
    size_t             _length,
    struct d_ftp_type* _out
)
{
    // parameter validation
    if ( (!_text) ||
         (!_out) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    const struct d_ftp_span text = d_ftp_internal_trim(_text,
                                                       _length);

    // an argument is at least its type code
    if (text.length == 0u)
    {
        return D_FTP_ERROR_MALFORMED;
    }

    const char        code = d_ftp_internal_to_lower(text.data[0]);
    struct d_ftp_type type = { D_FTP_TYPE_IMAGE, D_FTP_FORMAT_NONE, 8u };

    // the local type: "L", one space, and a byte size of 1 to 255
    if (code == 'l')
    {
        uint64_t size = 0;

        if ( (text.length < 3u) ||
             (text.data[1] != ' ') )
        {
            return D_FTP_ERROR_MALFORMED;
        }

        const bool parsed = d_ftp_internal_parse_uint(text.data + 2u,
                                                      text.length - 2u,
                                                      255u,
                                                      &size);

        if ( (!parsed) ||
             (size == 0u) )
        {
            return D_FTP_ERROR_MALFORMED;
        }

        type.data_type = D_FTP_TYPE_LOCAL;
        type.byte_size = (unsigned)size;
        *_out          = type;

        return D_FTP_OK;
    }

    // the image type stands alone
    if (code == 'i')
    {
        if (text.length != 1u)
        {
            return D_FTP_ERROR_MALFORMED;
        }

        *_out = type;

        return D_FTP_OK;
    }

    // beyond A, E, I, and L lie types this module does not know
    if ( (code != 'a') &&
         (code != 'e') )
    {
        return D_FTP_ERROR_UNSUPPORTED;
    }

    type.data_type = (code == 'a') ? D_FTP_TYPE_ASCII : D_FTP_TYPE_EBCDIC;
    type.format    = D_FTP_FORMAT_NON_PRINT;

    // an optional format control, after one space
    if (text.length > 1u)
    {
        if ( (text.length != 3u) ||
             (text.data[1] != ' ') )
        {
            return D_FTP_ERROR_MALFORMED;
        }

        switch (d_ftp_internal_to_lower(text.data[2]))
        {
            case 'n':
                type.format = D_FTP_FORMAT_NON_PRINT;

                break;

            case 't':
                type.format = D_FTP_FORMAT_TELNET;

                break;

            case 'c':
                type.format = D_FTP_FORMAT_CARRIAGE_CONTROL;

                break;

            default:
                return D_FTP_ERROR_MALFORMED;
        }
    }

    *_out = type;

    return D_FTP_OK;
}

/*
d_ftp_structure_from_code
  A case-folded switch; the output is written only on success.
*/
bool
d_ftp_structure_from_code(
    char                  _code,
    enum d_ftp_structure* _out
)
{
    // parameter validation
    if (!_out)
    {
        return false;
    }

    switch (d_ftp_internal_to_lower(_code))
    {
        case 'f':
            *_out = D_FTP_STRUCTURE_FILE;

            return true;

        case 'r':
            *_out = D_FTP_STRUCTURE_RECORD;

            return true;

        case 'p':
            *_out = D_FTP_STRUCTURE_PAGE;

            return true;

        default:
            break;
    }

    return false;
}

/*
d_ftp_mode_from_code
  As d_ftp_structure_from_code().
*/
bool
d_ftp_mode_from_code(
    char                      _code,
    enum d_ftp_transfer_mode* _out
)
{
    // parameter validation
    if (!_out)
    {
        return false;
    }

    switch (d_ftp_internal_to_lower(_code))
    {
        case 's':
            *_out = D_FTP_MODE_STREAM;

            return true;

        case 'b':
            *_out = D_FTP_MODE_BLOCK;

            return true;

        case 'c':
            *_out = D_FTP_MODE_COMPRESSED;

            return true;

        case 'z':
            *_out = D_FTP_MODE_DEFLATE;

            return true;

        default:
            break;
    }

    return false;
}

//==============================================================================
// 4.  ASCII TRANSFERS
//==============================================================================

/*
d_ftp_ascii_init
  No CR is pending at the start of a transfer.
*/
void
d_ftp_ascii_init(
    struct d_ftp_ascii_state* _state
)
{
    // parameter validation
    if (!_state)
    {
        return;
    }

    _state->pending_cr = false;

    return;
}

/*
d_ftp_ascii_to_network
  Remembers whether the last byte was CR, so an existing CR LF -- even one
split across chunks -- is passed through rather than doubled.
*/
enum d_ftp_error
d_ftp_ascii_to_network(
    struct d_ftp_ascii_state* _state,
    const char*               _data,
    size_t                    _length,
    size_t*                   _out_used,
    struct d_ftp_buffer*      _out
)
{
    // parameter validation
    if ( (!_state)                         ||
         (!_out_used)                      ||
         (!d_ftp_internal_buffer_ok(_out)) ||
         ( (!_data) &&
           (_length > 0u) ) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    size_t index = 0;

    // until the input or the room runs out
    while (index < _length)
    {
        const char   c      = _data[index];
        const bool   expand = ( (c == '\n') &&
                                (!_state->pending_cr) );
        const size_t width  = (expand) ? 2u : 1u;

        // a unit that does not fit waits for the next call
        if (width > d_ftp_internal_room(_out))
        {
            break;
        }

        // a lone LF becomes CR LF
        if (expand)
        {
            d_ftp_internal_append(_out,
                                  D_FTP_EOL,
                                  2u);
        }
        else
        {
            d_ftp_internal_append_char(_out,
                                       c);
        }

        _state->pending_cr = (c == '\r');
        index++;
    }

    *_out_used = index;

    return D_FTP_OK;
}

/*
d_ftp_ascii_from_network
  A CR is held until the next byte shows what it was: with LF a newline, with
NUL a bare CR (the Telnet encoding), and with anything else a CR to keep. A
held CR survives across calls in the state.
*/
enum d_ftp_error
d_ftp_ascii_from_network(
    struct d_ftp_ascii_state* _state,
    const char*               _data,
    size_t                    _length,
    size_t*                   _out_used,
    struct d_ftp_buffer*      _out
)
{
    // parameter validation
    if ( (!_state)                         ||
         (!_out_used)                      ||
         (!d_ftp_internal_buffer_ok(_out)) ||
         ( (!_data) &&
           (_length > 0u) ) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    size_t index = 0;

    // until the input or the room runs out
    while (index < _length)
    {
        const char c = _data[index];

        // everything below writes at least one byte
        if (d_ftp_internal_room(_out) == 0u)
        {
            break;
        }

        // a held CR is resolved by the byte after it
        if (_state->pending_cr)
        {
            _state->pending_cr = false;

            // CR LF is a newline, CR NUL a bare CR
            if ( (c == '\n') ||
                 (c == '\0') )
            {
                d_ftp_internal_append_char(_out,
                                           (c == '\n') ? '\n' : '\r');
                index++;

                continue;
            }

            // any other byte keeps the CR, then is itself examined
            d_ftp_internal_append_char(_out,
                                       '\r');

            continue;
        }

        // a CR waits to see what follows it
        if (c == '\r')
        {
            _state->pending_cr = true;
        }
        else
        {
            d_ftp_internal_append_char(_out,
                                       c);
        }

        index++;
    }

    *_out_used = index;

    return D_FTP_OK;
}

/*
d_ftp_ascii_finish
  A transfer that ends on CR keeps that CR.
*/
enum d_ftp_error
d_ftp_ascii_finish(
    struct d_ftp_ascii_state* _state,
    struct d_ftp_buffer*      _out
)
{
    // parameter validation
    if ( (!_state) ||
         (!d_ftp_internal_buffer_ok(_out)) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    // nothing held back
    if (!_state->pending_cr)
    {
        return D_FTP_OK;
    }

    // the held CR, if it fits
    if (!d_ftp_internal_append_char(_out,
                                    '\r'))
    {
        return D_FTP_ERROR_BUFFER_TOO_SMALL;
    }

    _state->pending_cr = false;

    return D_FTP_OK;
}
