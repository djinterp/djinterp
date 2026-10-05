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
*                                                            revised: 2026.09.28
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
d_ftp_internal_type_append_control
  File-local: appends an A or E type's format control, when one is named:
one space and the control's letter, of the three RFC 959 defines.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_type_append_control(
    const struct d_ftp_type* _type,
    struct d_ftp_buffer*     _local
)
{
    // a format control follows only when one is named
    if (_type->format == D_FTP_FORMAT_NONE)
    {
        return D_FTP_OK;
    }

    // RFC 959 defines exactly three
    if ( (_type->format != D_FTP_FORMAT_NON_PRINT) &&
         (_type->format != D_FTP_FORMAT_TELNET)    &&
         (_type->format != D_FTP_FORMAT_CARRIAGE_CONTROL) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    d_ftp_internal_append_char(_local,
                               ' ');
    d_ftp_internal_append_char(_local,
                               (char)_type->format);

    return D_FTP_OK;
}

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
    enum d_ftp_error    error = D_FTP_OK;

    switch (_type->data_type)
    {
        case D_FTP_TYPE_ASCII:
        case D_FTP_TYPE_EBCDIC:
            d_ftp_internal_append_char(&local,
                                       (char)_type->data_type);
            error = d_ftp_internal_type_append_control(_type,
                                                       &local);
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

    return (error != D_FTP_OK) ? error
                               : d_ftp_internal_commit(_out,
                                                       &local);
}

/*
d_ftp_internal_type_local
  File-local: reads the local type's argument: "L", one space, and a byte
size of 1 to 255.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_type_local(
    struct d_ftp_span  _text,
    struct d_ftp_type* _out
)
{
    uint64_t size = 0u;

    // one space, then a byte size of 1 to 255
    if ( (_text.length < 3u)                           ||
         (_text.data[1] != ' ')                        ||
         (!d_ftp_internal_parse_uint(_text.data + 2u,
                                     _text.length - 2u,
                                     255u,
                                     &size))           ||
         (size == 0u) )
    {
        return D_FTP_ERROR_MALFORMED;
    }

    _out->data_type = D_FTP_TYPE_LOCAL;
    _out->byte_size = (unsigned)size;

    return D_FTP_OK;
}

/*
d_ftp_internal_type_control
  File-local: reads the format control after an A or E type: one space,
then N, T, or C, in either case.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_type_control(
    struct d_ftp_span           _text,
    enum d_ftp_format_control*  _out
)
{
    // exactly one space and one letter
    if ( (_text.length != 3u) ||
         (_text.data[1] != ' ') )
    {
        return D_FTP_ERROR_MALFORMED;
    }

    switch (d_ftp_internal_to_lower(_text.data[2]))
    {
        case 'n':
            *_out = D_FTP_FORMAT_NON_PRINT;

            return D_FTP_OK;

        case 't':
            *_out = D_FTP_FORMAT_TELNET;

            return D_FTP_OK;

        case 'c':
            *_out = D_FTP_FORMAT_CARRIAGE_CONTROL;

            return D_FTP_OK;

        default:
            break;
    }

    return D_FTP_ERROR_MALFORMED;
}

/*
d_ftp_type_parse
  An unknown type code is UNSUPPORTED rather than MALFORMED, because RFC 959
answers it with 504 (parameter not implemented), not 501. `_out` changes
only on success.
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

    const char        code  = d_ftp_internal_to_lower(text.data[0]);
    struct d_ftp_type type  = { D_FTP_TYPE_IMAGE, D_FTP_FORMAT_NONE, 8u };
    enum d_ftp_error  error = D_FTP_ERROR_UNSUPPORTED;

    // L and a byte size; I alone; or A or E, with an optional control
    if (code == 'l')
    {
        error = d_ftp_internal_type_local(text,
                                          &type);
    }
    else if (code == 'i')
    {
        error = (text.length == 1u) ? D_FTP_OK : D_FTP_ERROR_MALFORMED;
    }
    else if ( (code == 'a') ||
              (code == 'e') )
    {
        type.data_type = (code == 'a') ? D_FTP_TYPE_ASCII : D_FTP_TYPE_EBCDIC;
        type.format    = D_FTP_FORMAT_NON_PRINT;
        error          = (text.length > 1u)
                         ? d_ftp_internal_type_control(text,
                                                       &type.format)
                         : D_FTP_OK;
    }

    // a type this module knows, well formed
    if (error == D_FTP_OK)
    {
        *_out = type;
    }

    return error;
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
d_ftp_internal_ascii_resolve
  File-local: resolves a held CR by the byte after it, writing one byte: CR
LF is a newline and CR NUL a bare CR, both consuming the byte; any other
byte keeps the CR and is itself examined next. Returns whether the byte
was consumed.
*/
D_STATIC bool
d_ftp_internal_ascii_resolve(
    struct d_ftp_ascii_state* _state,
    char                      _byte,
    struct d_ftp_buffer*      _out
)
{
    _state->pending_cr = false;

    // CR LF is a newline, CR NUL a bare CR
    if ( (_byte == '\n') ||
         (_byte == '\0') )
    {
        d_ftp_internal_append_char(_out,
                                   (_byte == '\n') ? '\n' : '\r');

        return true;
    }

    d_ftp_internal_append_char(_out,
                               '\r');

    return false;
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

    size_t index = 0u;

    // until the input or the room runs out: each step writes a byte
    while ( (index < _length) &&
            (d_ftp_internal_room(_out) > 0u) )
    {
        const char c = _data[index];

        // a held CR is resolved by the byte after it
        if (_state->pending_cr)
        {
            index += (d_ftp_internal_ascii_resolve(_state,
                                                   c,
                                                   _out)) ? 1u : 0u;

            continue;
        }

        // a CR waits to see what follows it; anything else is kept
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
