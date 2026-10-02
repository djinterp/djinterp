/*******************************************************************************
* djinterp [net]                                                      ftp_path.c
*
* Implementation of the pathname handling declared in ftp_path.h.
*   Resolution clamps ".." at the root; that clamp is the whole security
* property a server relies on.
*
*
* path:      /src/djinterp/net/ftp/ftp_path.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/
#include "../../../../inc/djinterp/net/ftp/ftp_path.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <string.h>   // memchr, strlen
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"          // framework root
#include "../../../../inc/djinterp/net/ftp/ftp_common.h"  // d_ftp_error
#include "./ftp_internal.h"                               // shared helpers


//==============================================================================
// 1.  PATHS
//==============================================================================

/*
d_ftp_pathname_quote
  The quotes are counted first so the exact length is known before writing.
*/
enum d_ftp_error
d_ftp_pathname_quote(
    const char*          _path,
    struct d_ftp_buffer* _out
)
{
    // parameter validation
    if ( (!_path) ||
         (!d_ftp_internal_buffer_ok(_out)) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    const size_t length = strlen(_path);

    // a line break cannot be represented inside a reply line
    if (d_ftp_internal_has_line_break(_path,
                                      length))
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    size_t quotes = 0;

    // every embedded quote will be doubled
    for (size_t index = 0; index < length; index++)
    {
        if (_path[index] == '"')
        {
            quotes++;
        }
    }

    // the whole quoted name, or nothing
    if ((length + quotes + 2u) > d_ftp_internal_room(_out))
    {
        return D_FTP_ERROR_BUFFER_TOO_SMALL;
    }

    d_ftp_internal_append_char(_out,
                               '"');

    // copy, doubling quotes
    for (size_t index = 0; index < length; index++)
    {
        d_ftp_internal_append_char(_out,
                                   _path[index]);

        if (_path[index] == '"')
        {
            d_ftp_internal_append_char(_out,
                                       '"');
        }
    }

    d_ftp_internal_append_char(_out,
                               '"');

    return D_FTP_OK;
}

/*
d_ftp_pathname_unquote
  Starts at the first '"' and ends at the first lone one; a doubled quote
inside is one literal quote.
*/
enum d_ftp_error
d_ftp_pathname_unquote(
    const char*          _text,
    size_t               _length,
    struct d_ftp_buffer* _out
)
{
    // parameter validation
    if ( (!_text) ||
         (!d_ftp_internal_buffer_ok(_out)) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    const char* const open = memchr(_text,
                                    '"',
                                    _length);

    // no quoted name at all
    if (!open)
    {
        return D_FTP_ERROR_MALFORMED;
    }

    const size_t mark     = _out->length;
    size_t       position = (size_t)(open - _text) + 1u;

    // copy until the lone quote that closes the name
    while (position < _length)
    {
        const char c        = _text[position];
        const bool doubled  = ( (c == '"')                       &&
                                ((position + 1u) < _length)      &&
                                (_text[position + 1u] == '"') );

        // a lone quote closes the name
        if ( (c == '"') &&
             (!doubled) )
        {
            return D_FTP_OK;
        }

        // copy one byte; a doubled quote is one literal quote
        if (!d_ftp_internal_append_char(_out,
                                        c))
        {
            d_ftp_internal_rollback(_out,
                                    mark);

            return D_FTP_ERROR_BUFFER_TOO_SMALL;
        }

        position += (doubled) ? 2u : 1u;
    }

    d_ftp_internal_rollback(_out,
                            mark);

    return D_FTP_ERROR_MALFORMED;
}

/*
d_ftp_internal_path_push
  File-local: applies one segment to a resolution in progress. Empty and "."
segments do nothing, ".." drops the last segment but never the root, and any
other segment is appended after a '/'. `_root` is the offset just past the
leading '/'.
*/
D_STATIC bool
d_ftp_internal_path_push(
    struct d_ftp_buffer* _out,
    size_t               _root,
    const char*          _segment,
    size_t               _length
)
{
    // empty and "." segments name the current directory
    if ( (_length == 0u) ||
         ( (_length == 1u) &&
           (_segment[0] == '.') ) )
    {
        return true;
    }

    // ".." climbs one level, never above the root
    if ( (_length == 2u)       &&
         (_segment[0] == '.')  &&
         (_segment[1] == '.') )
    {
        size_t cut = _out->length;

        // back to the start of the last segment
        while ( (cut > _root) &&
                (_out->data[cut - 1u] != '/') )
        {
            cut--;
        }

        // and over its separator, unless that separator is the root
        if (cut > _root)
        {
            cut--;
        }

        d_ftp_internal_rollback(_out,
                                cut);

        return true;
    }

    const size_t separator = (_out->length > _root) ? 1u : 0u;

    // the separator and the segment together
    if ((separator + _length) > d_ftp_internal_room(_out))
    {
        return false;
    }

    // segments are joined by '/', but the root already ends in one
    if (separator > 0u)
    {
        d_ftp_internal_append_char(_out,
                                   '/');
    }

    d_ftp_internal_append(_out,
                          _segment,
                          _length);

    return true;
}

/*
d_ftp_internal_path_walk
  File-local: pushes every '/'-separated segment of a NUL-terminated path.
*/
D_STATIC bool
d_ftp_internal_path_walk(
    struct d_ftp_buffer* _out,
    size_t               _root,
    const char*          _path
)
{
    const char* segment = _path;

    // one segment per pass, until the terminator
    for (;;)
    {
        size_t length = 0;

        // the segment runs to the next '/' or the end
        while ( (segment[length] != '\0') &&
                (segment[length] != '/') )
        {
            length++;
        }

        // a segment that does not fit fails the walk
        if (!d_ftp_internal_path_push(_out,
                                      _root,
                                      segment,
                                      length))
        {
            return false;
        }

        // the path ended with this segment
        if (segment[length] == '\0')
        {
            return true;
        }

        segment += length + 1u;
    }
}

/*
d_ftp_path_resolve
  The base is walked first, then the path on top of it; an absolute path
skips the base. Clamping ".." at the root is the whole security property.
*/
enum d_ftp_error
d_ftp_path_resolve(
    const char*          _base,
    const char*          _path,
    struct d_ftp_buffer* _out
)
{
    // parameter validation
    if ( (!_path) ||
         (!d_ftp_internal_buffer_ok(_out)) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    const char* const base     = ( (_base) &&
                                   (_path[0] != '/') ) ? _base : "";
    const bool        bad_base = d_ftp_internal_has_line_break(base,
                                                               strlen(base));
    const bool        bad_path = d_ftp_internal_has_line_break(_path,
                                                               strlen(_path));

    // CR and LF would reach a command line
    if ( (bad_base) ||
         (bad_path) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    const size_t mark = _out->length;

    // every result is absolute
    if (!d_ftp_internal_append_char(_out,
                                    '/'))
    {
        return D_FTP_ERROR_BUFFER_TOO_SMALL;
    }

    const size_t root = _out->length;

    // the working directory, then the path relative to it
    if ( (!d_ftp_internal_path_walk(_out,
                                    root,
                                    base)) ||
         (!d_ftp_internal_path_walk(_out,
                                    root,
                                    _path)) )
    {
        d_ftp_internal_rollback(_out,
                                mark);

        return D_FTP_ERROR_BUFFER_TOO_SMALL;
    }

    return D_FTP_OK;
}
