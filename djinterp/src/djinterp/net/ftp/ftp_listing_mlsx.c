/*******************************************************************************
* djinterp [net]                                              ftp_listing_mlsx.c
*
* Implementation of the MLSx listing dialect: MLSD and MLST lines (RFC 3659).
*   Each line is a list of "name=value;" facts, a space, and the name. Unknown
* facts are ignored, as section 7.5 requires.
*
*
* path:      /src/djinterp/net/ftp/ftp_listing_mlsx.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../../inc/djinterp/net/ftp/ftp_listing.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdint.h>   // uint32_t, uint64_t, UINT64_MAX
#include <string.h>   // memchr
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"          // framework root
#include "../../../../inc/djinterp/net/ftp/ftp_common.h"  // d_ftp_span
#include "../../../../inc/djinterp/net/ftp/ftp_fact.h"    // d_ftp_time
#include "./ftp_internal.h"                               // text helpers
#include "./ftp_listing_internal.h"                       // listing helpers


//==============================================================================
// 2.  DIRECTORY LISTINGS
//==============================================================================

/*
d_ftp_internal_mlsx_split
  Splits an MLSx line into its facts and its name. The facts run
to the first space and end in ';' (RFC 3659 7.2); an MLST line carries one
extra leading space, which is skipped.
*/
bool
d_ftp_internal_mlsx_split(
    struct d_ftp_span  _line,
    struct d_ftp_span* _out_facts,
    struct d_ftp_span* _out_name
)
{
    const size_t      start = ( (_line.length > 0u) &&
                                (_line.data[0] == ' ') ) ? 1u : 0u;
    const char* const space = memchr(_line.data + start,
                                     ' ',
                                     _line.length - start);

    // facts end at the first space
    if (!space)
    {
        return false;
    }

    const size_t      length = (size_t)(space - (_line.data + start));
    const char* const equals = memchr(_line.data + start,
                                      '=',
                                      length);

    // at least one fact, and the last one terminated
    if ( (length == 0u)                                  ||
         (!equals)                                       ||
         (_line.data[start + length - 1u] != ';') )
    {
        return false;
    }

    _out_facts->data   = _line.data + start;
    _out_facts->length = length;
    _out_name->data    = space + 1;
    _out_name->length  = _line.length - start - length - 1u;

    return true;
}

/*
d_ftp_internal_parse_octal
  File-local: reads one to six octal digits, at most 07777, as mode bits.
*/
D_STATIC bool
d_ftp_internal_parse_octal(
    struct d_ftp_span _text,
    uint32_t*         _out_value
)
{
    // one to six digits
    if ( (_text.length == 0u) ||
         (_text.length > 6u) )
    {
        return false;
    }

    uint32_t value = 0;

    // octal digits only
    for (size_t index = 0; index < _text.length; index++)
    {
        const char c = _text.data[index];

        if ( (c < '0') ||
             (c > '7') )
        {
            return false;
        }

        value = (value * 8u) + (uint32_t)(c - '0');
    }

    // permission and special bits only
    if (value > 07777u)
    {
        return false;
    }

    *_out_value = value;

    return true;
}

/*
d_ftp_internal_mlsx_type
  File-local: reads the "type" fact, including the OS.unix symbolic-link forms
("OS.unix=slink:target" and "OS.unix=symlink") that servers use.
*/
D_STATIC void
d_ftp_internal_mlsx_type(
    struct d_ftp_span   _value,
    struct d_ftp_entry* _out
)
{
    static const char* const NAMES[4] = { "file", "dir", "cdir", "pdir" };
    static const enum d_ftp_entry_type TYPES[4] =
    {
        D_FTP_ENTRY_FILE,
        D_FTP_ENTRY_DIRECTORY,
        D_FTP_ENTRY_CURRENT_DIRECTORY,
        D_FTP_ENTRY_PARENT_DIRECTORY
    };

    // the four standard types
    for (size_t index = 0; index < 4u; index++)
    {
        if (d_ftp_internal_equals_nocase(_value.data,
                                         _value.length,
                                         NAMES[index]))
        {
            _out->type = TYPES[index];

            return;
        }
    }

    const bool slink   = d_ftp_internal_starts_nocase(_value.data,
                                                      _value.length,
                                                      "os.unix=slink");
    const bool symlink = d_ftp_internal_starts_nocase(_value.data,
                                                      _value.length,
                                                      "os.unix=symlink");

    // anything else is some other kind of object
    if ( (!slink) &&
         (!symlink) )
    {
        _out->type = D_FTP_ENTRY_OTHER;

        return;
    }

    const char* const colon = memchr(_value.data,
                                     ':',
                                     _value.length);

    _out->type = D_FTP_ENTRY_SYMLINK;

    // "slink:target" names the target
    if (colon)
    {
        _out->link_target.data    = colon + 1;
        _out->link_target.length  = _value.length -
                                    (size_t)(colon - _value.data) - 1u;
        _out->known              |= D_FTP_ENTRY_KNOWN_LINK_TARGET;
    }

    return;
}

/*
d_ftp_internal_mlsx_text_fact
  File-local: records a fact whose value is kept as text -- "unique",
"perm", "unix.owner", or "unix.group" -- and reports whether `_name` was
one of them.
*/
D_STATIC bool
d_ftp_internal_mlsx_text_fact(
    struct d_ftp_span   _name,
    struct d_ftp_span   _value,
    struct d_ftp_entry* _out
)
{
    struct d_ftp_span* field = NULL;
    unsigned           known = 0u;

    // which field the fact fills
    if (d_ftp_internal_equals_nocase(_name.data,
                                     _name.length,
                                     "unique"))
    {
        field = &_out->unique;
        known = D_FTP_ENTRY_KNOWN_UNIQUE;
    }
    else if (d_ftp_internal_equals_nocase(_name.data,
                                          _name.length,
                                          "perm"))
    {
        field = &_out->permissions;
        known = D_FTP_ENTRY_KNOWN_PERMISSIONS;
    }
    else if (d_ftp_internal_equals_nocase(_name.data,
                                          _name.length,
                                          "unix.owner"))
    {
        field = &_out->owner;
        known = D_FTP_ENTRY_KNOWN_OWNER;
    }
    else if (d_ftp_internal_equals_nocase(_name.data,
                                          _name.length,
                                          "unix.group"))
    {
        field = &_out->group;
        known = D_FTP_ENTRY_KNOWN_GROUP;
    }

    // not a text fact
    if (!field)
    {
        return false;
    }

    *field       = _value;
    _out->known |= known;

    return true;
}

/*
d_ftp_internal_mlsx_value_fact
  File-local: records a fact whose value is read: "size" for files and
"sizd" for directories, "modify" in UTC, and "unix.mode" in octal. A value
that does not read leaves its field unknown.
*/
D_STATIC void
d_ftp_internal_mlsx_value_fact(
    struct d_ftp_span   _name,
    struct d_ftp_span   _value,
    struct d_ftp_entry* _out
)
{
    uint64_t          number = 0u;
    uint32_t          mode   = 0u;
    struct d_ftp_time time   = { 0u, 0u, 0u, 0u, 0u, 0u, 0u };

    // a size
    if ( ( (d_ftp_internal_equals_nocase(_name.data,
                                         _name.length,
                                         "size")) ||
           (d_ftp_internal_equals_nocase(_name.data,
                                         _name.length,
                                         "sizd")) )            &&
         (d_ftp_internal_parse_uint(_value.data,
                                    _value.length,
                                    UINT64_MAX,
                                    &number)) )
    {
        _out->size   = number;
        _out->known |= D_FTP_ENTRY_KNOWN_SIZE;
    }
    else if ( (d_ftp_internal_equals_nocase(_name.data,
                                            _name.length,
                                            "modify"))       &&
              (d_ftp_time_parse(_value.data,
                                _value.length,
                                &time) == D_FTP_OK) )
    {
        _out->modified  = time;
        _out->known    |= D_FTP_ENTRY_KNOWN_MODIFIED |
                          D_FTP_ENTRY_KNOWN_TIME     |
                          D_FTP_ENTRY_KNOWN_UTC;
    }
    else if ( (d_ftp_internal_equals_nocase(_name.data,
                                            _name.length,
                                            "unix.mode"))    &&
              (d_ftp_internal_parse_octal(_value,
                                          &mode)) )
    {
        _out->mode   = mode;
        _out->known |= D_FTP_ENTRY_KNOWN_MODE;
    }

    return;
}

/*
d_ftp_internal_mlsx_fact
  File-local: records one fact. Unknown facts are ignored, as RFC 3659 7.5
requires, and a malformed value leaves its field unknown rather than
discarding the entry.
*/
D_STATIC void
d_ftp_internal_mlsx_fact(
    struct d_ftp_span   _name,
    struct d_ftp_span   _value,
    struct d_ftp_entry* _out
)
{
    // the type fact classifies the entry
    if (d_ftp_internal_equals_nocase(_name.data,
                                     _name.length,
                                     "type"))
    {
        d_ftp_internal_mlsx_type(_value,
                                 _out);

        return;
    }

    // a fact kept as text, or else one with a value to read
    if (!d_ftp_internal_mlsx_text_fact(_name,
                                       _value,
                                       _out))
    {
        d_ftp_internal_mlsx_value_fact(_name,
                                       _value,
                                       _out);
    }

    return;
}

/*
d_ftp_internal_mlsx_facts
  File-local: records each fact of a fact list: "name=value;", split at its
first '=' so values such as "OS.unix=slink:x" keep their own. ";;" leaves
an empty fact, which says nothing; a fact without '=' fails the line.
*/
D_STATIC bool
d_ftp_internal_mlsx_facts(
    struct d_ftp_span   _facts,
    struct d_ftp_entry* _out
)
{
    // fact by fact
    for (size_t position = 0u; position < _facts.length; )
    {
        const char* const       semicolon = memchr(_facts.data + position,
                                                   ';',
                                                   _facts.length - position);
        const size_t            end       =
            (semicolon) ? (size_t)(semicolon - _facts.data)
                        : _facts.length;
        const struct d_ftp_span fact      = d_ftp_internal_span_at(_facts,
                                                                   position,
                                                                   end);
        const char* const       equals    = memchr(fact.data,
                                                   '=',
                                                   fact.length);

        position = end + 1u;

        // an empty fact says nothing
        if (fact.length == 0u)
        {
            continue;
        }

        // every other fact names itself
        if (!equals)
        {
            return false;
        }

        const size_t name_length = (size_t)(equals - fact.data);

        d_ftp_internal_mlsx_fact(d_ftp_internal_span_at(fact,
                                                        0u,
                                                        name_length),
                                 d_ftp_internal_span_at(fact,
                                                        name_length + 1u,
                                                        fact.length),
                                 _out);
    }

    return true;
}

/*
d_ftp_internal_parse_mlsx
  Reads an MLSD or MLST line: the facts, then a space and the name. A line
without facts is taken as a bare name.
*/
enum d_ftp_line_result
d_ftp_internal_parse_mlsx(
    struct d_ftp_span   _line,
    struct d_ftp_entry* _out
)
{
    struct d_ftp_span facts = { NULL, 0u };
    struct d_ftp_span name  = { NULL, 0u };

    // an entry without facts is only its name
    if (!d_ftp_internal_mlsx_split(_line,
                                   &facts,
                                   &name))
    {
        const size_t start = ( (_line.length > 0u) &&
                               (_line.data[0] == ' ') ) ? 1u : 0u;

        name = d_ftp_internal_span_at(_line,
                                      start,
                                      _line.length);
    }

    // a name, which the line exists to carry, and well-formed facts
    if ( (name.length == 0u) ||
         (!d_ftp_internal_mlsx_facts(facts,
                                     _out)) )
    {
        return D_FTP_LINE_MALFORMED;
    }

    _out->name = name;

    return D_FTP_LINE_ENTRY;
}
