/*******************************************************************************
* djinterp [tools]                                                       dmsvs.c
*
* Reads Visual Studio's files into records: see dmsvs.h.
*   An XML file is read with dxml's scanner into an element tree, each
* element linked to its parent, its first child and its next sibling, with
* a stack of the elements still open. A solution's projects are then read
* off the tree, or, for a .sln, off its lines: Project(...) = ... and
* EndProject, the dependencies a project section lists, the configurations
* and the folder nesting the global sections give, each record with the
* span of text it stands in.
*   And three things a writer needs: GUIDs made from names, by SHA-1;
* relative paths; and escaped text.
*
* path:      /src/djinterp/tools/dawk/ext/msvs/dmsvs.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "../../../../../../inc/djinterp/tools/dawk/ext/msvs/dmsvs.h"  // corresponding header
// std
#include <stdint.h>  // uint32_t, uint8_t
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, realloc, free
#include <string.h>  // memcpy, memset, memcmp, strlen, strchr
// djinterp
#include "../../../../../../inc/djinterp/tools/dawk/dxml.h"  // d_xml_next, d_xml_decode


// D_INTERNAL_FOLDER_TYPE
//   constant: the project type a .sln gives a solution folder.
#define D_INTERNAL_FOLDER_TYPE "2150e333-8fdc-42a3-9474-1a3956d46de8"


// d_internal_reader
//   struct: what one reading holds while it runs: the records it grows, the
//   line it has counted to, and where to say a failure is.
struct d_internal_reader
{
    struct d_msvs* msvs;
    size_t         element_capacity;
    size_t         project_capacity;
    size_t         folder_capacity;
    size_t         section_capacity;
    size_t         dependency_capacity;
    size_t         configuration_capacity;
    size_t         counted;
    size_t         line;
    size_t         line_start;
    char*          problem;
    size_t         capacity;
    bool           failed;
};


//==============================================================================
// 1.  HELPERS
//==============================================================================


/*
d_internal_fail
  Records where reading stopped and why; the first failure is the one kept.
*/
static bool
d_internal_fail(
    struct d_internal_reader* _reader,
    size_t                    _line,
    size_t                    _column,
    const char*               _why
)
{
    // a later failure follows from the first, and would only mislead
    if (!_reader->failed)
    {
        _reader->failed             = true;
        _reader->msvs->error_line   = _line;
        _reader->msvs->error_column = _column;

        // the reason goes where there is room for it
        if ( (_reader->problem) &&
             (_reader->capacity > 0u) )
        {
            (void)snprintf(_reader->problem, _reader->capacity,
                           "line %lu, column %lu: %s",
                           (unsigned long)_line, (unsigned long)_column,
                           _why);
        }
    }

    return false;
}


/*
d_internal_line_at
  Counts lines up to `_at`, which never goes back, and gives the line it is
on; `_column`, when given, receives its column.
*/
static size_t
d_internal_line_at(
    struct d_internal_reader* _reader,
    size_t                    _at,
    size_t*                   _column
)
{
    const char* const text = _reader->msvs->text;

    // each byte up to the offset, a line ending counting a line
    while (_reader->counted < _at)
    {
        // a line ending moves the count on
        if (text[_reader->counted] == '\n')
        {
            // the first ending says how the file ends its lines
            if (_reader->line == 1u)
            {
                _reader->msvs->crlf = ( (_reader->counted > 0u) &&
                                        (text[_reader->counted - 1u]
                                         == '\r') );
            }

            _reader->line++;
            _reader->line_start = _reader->counted + 1u;
        }

        _reader->counted++;
    }

    // a column, when one is asked for
    if (_column)
    {
        *_column = (_at - _reader->line_start) + 1u;
    }

    return _reader->line;
}


/*
d_internal_grow
  Makes room for one more record in an array of `_size`-byte records.
*/
static bool
d_internal_grow(
    void**  _array,
    size_t* _capacity,
    size_t  _count,
    size_t  _size
)
{
    // there is room already
    if (_count < *_capacity)
    {
        return true;
    }

    const size_t wanted = (*_capacity == 0u) ? 16u : (*_capacity * 2u);
    void* const  grown  = realloc(*_array, wanted * _size);

    // the records so far stay where they were when memory runs out
    if (!grown)
    {
        return false;
    }

    *_array    = grown;
    *_capacity = wanted;

    return true;
}


/*
d_internal_copy
  A string of its own, of `_length` bytes from `_text`.
*/
static char*
d_internal_copy(
    const char* _text,
    size_t      _length
)
{
    char* const copy = malloc(_length + 1u);

    // a copy that cannot be made is NULL
    if (copy)
    {
        if (_length > 0u)
        {
            memcpy(copy, _text, _length);
        }

        copy[_length] = '\0';
    }

    return copy;
}


/*
d_internal_lower_guid
  A GUID as the records keep one: lower case, without braces.
*/
static char*
d_internal_lower_guid(
    const char* _text,
    size_t      _length
)
{
    const char* from   = _text;
    size_t      length = _length;

    // braces are not part of it
    if ( (length >= 2u)      &&
         (from[0] == '{')    &&
         (from[length - 1u] == '}') )
    {
        from   += 1;
        length -= 2u;
    }

    char* const guid = d_internal_copy(from, length);

    // each letter lowered, the rest as it is
    for (size_t at = 0; (guid) && (at < length); ++at)
    {
        guid[at] = ( (guid[at] >= 'A') && (guid[at] <= 'Z') )
                 ? (char)(guid[at] - 'A' + 'a') : guid[at];
    }

    return guid;
}


/*
d_internal_same_text
  ASCII case folding, which is all a GUID or a configuration needs.
*/
static bool
d_internal_same_text(
    const char* _left,
    size_t      _left_length,
    const char* _right,
    size_t      _right_length
)
{
    // texts of other lengths are other texts
    if (_left_length != _right_length)
    {
        return false;
    }

    // each byte, its letters folded
    for (size_t at = 0; at < _left_length; ++at)
    {
        char left  = _left[at];
        char right = _right[at];

        left  = ( (left >= 'A') && (left <= 'Z') )
              ? (char)(left - 'A' + 'a') : left;
        right = ( (right >= 'A') && (right <= 'Z') )
              ? (char)(right - 'A' + 'a') : right;

        // one letter apart is apart
        if (left != right)
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_ends_with
  Whether a name ends in a suffix, in any case.
*/
static bool
d_internal_ends_with(
    const char* _name,
    const char* _suffix
)
{
    const size_t name   = strlen(_name);
    const size_t suffix = strlen(_suffix);

    return ( (name > suffix) &&
             (d_internal_same_text(&_name[name - suffix], suffix, _suffix,
                                   suffix)) );
}


//==============================================================================
// 2.  KINDS
//==============================================================================


/*
d_msvs_kind_of
  By the name's ending; .vcxproj.filters before .vcxproj's own kind.
*/
enum d_msvs_kind
d_msvs_kind_of(
    const char* _path
)
{
    // d_internal_ending
    //   struct: a name's ending, and the kind it marks.
    struct d_internal_ending
    {
        const char*      suffix;
        enum d_msvs_kind kind;
    };

    static const struct d_internal_ending endings[] =
    {
        { ".vcxproj.filters", D_MSVS_FILTERS  },
        { ".vcxproj.user",    D_MSVS_USER     },
        { ".slnx",            D_MSVS_SOLUTION },
        { ".sln",             D_MSVS_SOLUTION },
        { ".vcxproj",         D_MSVS_PROJECT  },
        { ".vcxitems",        D_MSVS_PROJECT  },
        { ".csproj",          D_MSVS_PROJECT  },
        { ".vbproj",          D_MSVS_PROJECT  },
        { ".fsproj",          D_MSVS_PROJECT  },
        { ".props",           D_MSVS_PROPS    },
        { ".targets",         D_MSVS_PROPS    }
    };

    // a name must be given to be named
    if (!_path)
    {
        return D_MSVS_OTHER;
    }

    // the endings, the longest of a kind first
    for (size_t at = 0; at < (sizeof(endings) / sizeof(endings[0])); ++at)
    {
        // the first ending that matches is the kind
        if (d_internal_ends_with(_path, endings[at].suffix))
        {
            return endings[at].kind;
        }
    }

    return D_MSVS_OTHER;
}


//==============================================================================
// 3.  XML
//==============================================================================


/*
d_internal_is_name_start, d_internal_is_name_char
  The bytes an XML name may begin with, and hold: ASCII's letters, `_` and
`:`, digits, `.` and `-` after the first, and any byte of a character past
ASCII.
*/
static bool
d_internal_is_name_start(
    unsigned char _byte
)
{
    return ( ( (_byte >= 'A') && (_byte <= 'Z') ) ||
             ( (_byte >= 'a') && (_byte <= 'z') ) ||
             (_byte == '_')                       ||
             (_byte == ':')                       ||
             (_byte >= 0x80u) );
}


static bool
d_internal_is_name_char(
    unsigned char _byte
)
{
    return ( (d_internal_is_name_start(_byte))    ||
             ( (_byte >= '0') && (_byte <= '9') ) ||
             (_byte == '.')                       ||
             (_byte == '-') );
}


/*
d_internal_name_sound
  Whether a span is an XML name.
*/
static bool
d_internal_name_sound(
    const char* _name,
    size_t      _length
)
{
    // a name has a first byte, of its own kind
    if ( (_length == 0u) ||
         (!d_internal_is_name_start((unsigned char)_name[0])) )
    {
        return false;
    }

    // each byte after the first
    for (size_t at = 1; at < _length; ++at)
    {
        // and every other of a name's
        if (!d_internal_is_name_char((unsigned char)_name[at]))
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_reference_length
  The length of the reference at `_text`, its `&` and `;` included: a
predefined entity, or a character reference to a character XML allows; 0
when no such reference begins there.
*/
static size_t
d_internal_reference_length(
    const char* _text,
    size_t      _length
)
{
    static const char* const named[] =
    {
        "&amp;", "&lt;", "&gt;", "&quot;", "&apos;"
    };

    // the five references XML predefines
    for (size_t at = 0; at < (sizeof(named) / sizeof(named[0])); ++at)
    {
        const size_t span = strlen(named[at]);

        // a predefined entity, whole
        if ( (_length >= span) &&
             (memcmp(_text, named[at], span) == 0) )
        {
            return span;
        }
    }

    // anything else must be a character reference
    if ( (_length < 4u) ||
         (_text[1] != '#') )
    {
        return 0u;
    }

    const bool    hex   = (_text[2] == 'x');
    size_t        at    = hex ? 3u : 2u;
    const size_t  first = at;
    unsigned long code  = 0;

    // the reference's digits, up to its `;`
    while ( (at < _length) &&
            (_text[at] != ';') &&
            (code <= 0x10FFFFul) )
    {
        const char digit = _text[at];
        int        value = -1;

        // a digit of the reference's base
        if ( (digit >= '0') && (digit <= '9') )
        {
            value = digit - '0';
        }
        else if ( (hex) && (digit >= 'a') && (digit <= 'f') )
        {
            value = digit - 'a' + 10;
        }
        else if ( (hex) && (digit >= 'A') && (digit <= 'F') )
        {
            value = digit - 'A' + 10;
        }

        // a byte that is no digit ends no reference
        if (value < 0)
        {
            return 0u;
        }

        code = (code * (hex ? 16ul : 10ul)) + (unsigned long)value;
        at++;
    }

    // digits, a `;`, and a character XML allows: no control but tab and
    // the line endings, no surrogate, nothing past Unicode
    if ( (at == first)        ||
         (at >= _length)      ||
         (_text[at] != ';')   ||
         ( (code < 0x20ul) &&
           (code != 0x09ul) && (code != 0x0Aul) && (code != 0x0Dul) ) ||
         ( (code >= 0xD800ul) && (code <= 0xDFFFul) )                 ||
         (code == 0xFFFEul)   ||
         (code == 0xFFFFul)   ||
         (code > 0x10FFFFul) )
    {
        return 0u;
    }

    return at + 1u;
}


/*
d_internal_text_problem
  What is wrong with character data, if anything: an `&` that begins no
reference, or `]]>`, which only a CDATA section may end with.  `_at`
receives where.
*/
static const char*
d_internal_text_problem(
    const char* _text,
    size_t      _length,
    size_t*     _at
)
{
    // each byte the text holds
    for (size_t at = 0; at < _length; ++at)
    {
        // an `&` must begin a reference
        if ( (_text[at] == '&') &&
             (d_internal_reference_length(&_text[at], _length - at) == 0u) )
        {
            *_at = at;

            return "an & that begins no reference";
        }

        // and `]]>` ends nothing outside CDATA
        if ( ((at + 2u) < _length) &&
             (memcmp(&_text[at], "]]>", 3u) == 0) )
        {
            *_at = at;

            return "]]> outside a CDATA section";
        }
    }

    return NULL;
}


/*
d_internal_attributes_problem
  What is wrong with an element's attributes, if anything: each must be a
name, `=` and a quoted value, once each, the value holding no `<` and no
`&` that begins no reference.  `_at` receives where.
*/
static const char*
d_internal_attributes_problem(
    const char* _text,
    size_t      _length,
    size_t*     _at
)
{
    size_t names[64][2];
    size_t count = 0;
    size_t at    = 0;

    // one attribute at a time
    while (at < _length)
    {
        // space separates attributes
        if ( (_text[at] == ' ')  || (_text[at] == '\t') ||
             (_text[at] == '\r') || (_text[at] == '\n') )
        {
            at++;
            continue;
        }

        const size_t name = at;

        // a name runs to `=` or space
        while ( (at < _length)     &&
                (_text[at] != '=') &&
                (_text[at] != ' ') && (_text[at] != '\t') &&
                (_text[at] != '\r') && (_text[at] != '\n') )
        {
            at++;
        }

        *_at = name;

        // and is a name
        if (!d_internal_name_sound(&_text[name], at - name))
        {
            return "an attribute's name is not an XML name";
        }

        // each name before this one
        for (size_t before = 0; before < count; ++before)
        {
            // which no attribute before it had
            if ( (names[before][1] == (at - name)) &&
                 (memcmp(&_text[names[before][0]], &_text[name],
                         at - name) == 0) )
            {
                return "an attribute given twice";
            }
        }

        // the names are kept for the next to be held to
        if (count < 64u)
        {
            names[count][0] = name;
            names[count][1] = at - name;
            count++;
        }

        // past the space before `=`
        while ( (at < _length) &&
                ( (_text[at] == ' ') || (_text[at] == '\t') ||
                  (_text[at] == '\r') || (_text[at] == '\n') ) )
        {
            at++;
        }

        // then `=`
        if ( (at >= _length) ||
             (_text[at] != '=') )
        {
            return "an attribute is not name=\"value\"";
        }

        at++;

        // past the space after it
        while ( (at < _length) &&
                ( (_text[at] == ' ') || (_text[at] == '\t') ||
                  (_text[at] == '\r') || (_text[at] == '\n') ) )
        {
            at++;
        }

        // then a value in quotes of either kind
        if ( (at >= _length) ||
             ( (_text[at] != '"') && (_text[at] != '\'') ) )
        {
            return "an attribute is not name=\"value\"";
        }

        const char quote = _text[at++];

        // the value, up to its closing quote
        while ( (at < _length) &&
                (_text[at] != quote) )
        {
            *_at = at;

            // which holds no `<`, and no `&` but a reference's
            if (_text[at] == '<')
            {
                return "a < in an attribute's value";
            }

            if ( (_text[at] == '&') &&
                 (d_internal_reference_length(&_text[at], _length - at)
                  == 0u) )
            {
                return "an & that begins no reference";
            }

            at++;
        }

        // which close
        if (at >= _length)
        {
            *_at = name;

            return "an attribute is not name=\"value\"";
        }

        at++;

        // and the next attribute stands apart from it
        if ( (at < _length) &&
             (_text[at] != ' ') && (_text[at] != '\t') &&
             (_text[at] != '\r') && (_text[at] != '\n') )
        {
            *_at = at;

            return "attributes not set apart by space";
        }
    }

    return NULL;
}


/*
d_internal_pseudo
  Reads one of a declaration's pseudo-attributes at `*_at`: space, the name
given, `=`, and a quoted value, setting the value's span.  Gives false,
leaving `*_at` as it was, when that name does not come next.
*/
static bool
d_internal_pseudo(
    const char* _text,
    size_t      _length,
    size_t*     _at,
    const char* _name,
    size_t*     _value,
    size_t*     _value_length
)
{
    size_t       at   = *_at;
    const size_t span = strlen(_name);

    // space must come first
    if ( (at >= _length) ||
         ( (_text[at] != ' ') && (_text[at] != '\t') &&
           (_text[at] != '\r') && (_text[at] != '\n') ) )
    {
        return false;
    }

    // past the space before the pseudo-attribute's name
    while ( (at < _length) &&
            ( (_text[at] == ' ') || (_text[at] == '\t') ||
              (_text[at] == '\r') || (_text[at] == '\n') ) )
    {
        at++;
    }

    // then the name
    if ( ((at + span) > _length) ||
         (memcmp(&_text[at], _name, span) != 0) )
    {
        return false;
    }

    at += span;

    // past the space before `=`
    while ( (at < _length) &&
            ( (_text[at] == ' ') || (_text[at] == '\t') ||
              (_text[at] == '\r') || (_text[at] == '\n') ) )
    {
        at++;
    }

    // then `=`
    if ( (at >= _length) ||
         (_text[at] != '=') )
    {
        return false;
    }

    at++;

    // past the space after it
    while ( (at < _length) &&
            ( (_text[at] == ' ') || (_text[at] == '\t') ||
              (_text[at] == '\r') || (_text[at] == '\n') ) )
    {
        at++;
    }

    // then a quoted value
    if ( (at >= _length) ||
         ( (_text[at] != '"') && (_text[at] != '\'') ) )
    {
        return false;
    }

    const char   quote = _text[at++];
    const size_t value = at;

    // the value, up to its closing quote
    while ( (at < _length) &&
            (_text[at] != quote) )
    {
        at++;
    }

    // a value that never closes is no value
    if (at >= _length)
    {
        return false;
    }

    *_value        = value;
    *_value_length = at - value;
    *_at           = at + 1u;

    return true;
}


/*
d_internal_declaration_problem
  What is wrong with the XML declaration, if anything.  It stands first in
the file, is named in lower case, and gives, in this order, its version,
1.0 or another 1.x, an encoding if any -- which must be UTF-8, the one
encoding this reader reads -- and standalone, yes or no, if any.
*/
static const char*
d_internal_declaration_problem(
    const char* _text,
    size_t      _length,
    size_t      _offset,
    size_t      _first,
    size_t*     _at
)
{
    size_t at     = 5u;
    size_t value  = 0;
    size_t span   = 0;
    size_t inner  = (_length >= 2u) ? (_length - 2u) : 0u;

    *_at = 0u;

    // first in the file, and in lower case
    if (_offset != _first)
    {
        return "an XML declaration that is not first in the file";
    }

    // the declaration begins `<?xml`
    if (memcmp(_text, "<?xml", 5u) != 0)
    {
        return "an XML declaration that is not well formed";
    }

    // the version, 1. and digits
    if (!d_internal_pseudo(_text, inner, &at, "version", &value, &span))
    {
        *_at = at;

        return "an XML declaration that is not well formed";
    }

    // the version is 1. and at least one digit
    if ( (span < 3u) ||
         (_text[value] != '1') ||
         (_text[value + 1u] != '.') )
    {
        *_at = value;

        return "an XML declaration that is not well formed";
    }

    // each byte after the point
    for (size_t digit = value + 2u; digit < (value + span); ++digit)
    {
        // nothing but digits after the point
        if ( (_text[digit] < '0') ||
             (_text[digit] > '9') )
        {
            *_at = digit;

            return "an XML declaration that is not well formed";
        }
    }

    // the encoding, if given: UTF-8, in any case
    if (d_internal_pseudo(_text, inner, &at, "encoding", &value, &span))
    {
        // the encoding UTF-8, in any case
        if ( (span != 5u) ||
             ( (_text[value] != 'u') && (_text[value] != 'U') ) ||
             ( (_text[value + 1u] != 't') && (_text[value + 1u] != 'T') ) ||
             ( (_text[value + 2u] != 'f') && (_text[value + 2u] != 'F') ) ||
             (_text[value + 3u] != '-') ||
             (_text[value + 4u] != '8') )
        {
            *_at = value;

            return "an encoding other than UTF-8";
        }
    }

    // standalone, if given: yes or no
    if (d_internal_pseudo(_text, inner, &at, "standalone", &value, &span))
    {
        // standalone yes or no
        if ( !( ( (span == 3u) && (memcmp(&_text[value], "yes", 3u) == 0) ) ||
                ( (span == 2u) && (memcmp(&_text[value], "no", 2u) == 0) ) ) )
        {
            *_at = value;

            return "an XML declaration that is not well formed";
        }
    }

    // and nothing more but space
    while ( (at < inner) &&
            ( (_text[at] == ' ') || (_text[at] == '\t') ||
              (_text[at] == '\r') || (_text[at] == '\n') ) )
    {
        at++;
    }

    // nothing left over
    if (at != inner)
    {
        *_at = at;

        return "an XML declaration that is not well formed";
    }

    return NULL;
}


/*
d_internal_markup_problem
  What is wrong with a comment, an instruction or a declaration, if
anything: a comment holds no `--`, an instruction is named by a name, and
the XML declaration is sound.  `_at` receives where.
*/
static const char*
d_internal_markup_problem(
    const char* _text,
    size_t      _length,
    size_t      _offset,
    size_t      _first,
    size_t*     _at
)
{
    *_at = 0u;

    // a comment holds no `--`, nor ends `--->`
    if ( (_length >= 7u) &&
         (memcmp(_text, "<!--", 4u) == 0) )
    {
        // each `--` within the comment
        for (size_t at = 4u; (at + 3u) < _length; ++at)
        {
            if ( (_text[at] == '-') &&
                 (_text[at + 1u] == '-') )
            {
                *_at = at;

                return "a comment holding --";
            }
        }

        return NULL;
    }

    // an instruction is named by its target, a name
    if ( (_length >= 4u) &&
         (_text[0] == '<') &&
         (_text[1] == '?') )
    {
        size_t target = 2u;

        // the target runs while it is a name
        while ( (target < _length) &&
                (d_internal_is_name_char((unsigned char)_text[target])) )
        {
            target++;
        }

        // a name, then space or the instruction's end
        if ( (!d_internal_name_sound(&_text[2], target - 2u)) ||
             ( (_text[target] != ' ') && (_text[target] != '\t') &&
               (_text[target] != '\r') && (_text[target] != '\n') &&
               ( ((target + 2u) != _length) ||
                 (_text[target] != '?') ) ) )
        {
            *_at = 2u;

            return "an instruction's target is not an XML name";
        }

        // one named xml, in any case, is the declaration
        if ( ((target - 2u) == 3u) &&
             ( (_text[2] == 'x') || (_text[2] == 'X') ) &&
             ( (_text[3] == 'm') || (_text[3] == 'M') ) &&
             ( (_text[4] == 'l') || (_text[4] == 'L') ) )
        {
            return d_internal_declaration_problem(_text, _length, _offset,
                                                  _first, _at);
        }
    }

    return NULL;
}


/*
d_internal_document_problem
  What is wrong with the text as a whole, if anything: a control character
XML allows nowhere, or bytes that are not UTF-8.  `_at` receives where.
*/
static const char*
d_internal_document_problem(
    const char* _text,
    size_t      _length,
    size_t      _from,
    size_t*     _at
)
{
    size_t at = _from;

    // each byte, or each character a lead byte begins
    while (at < _length)
    {
        const unsigned char byte = (unsigned char)_text[at];

        *_at = at;

        // a control character but tab and the line endings
        if ( (byte < 0x20u) &&
             (byte != '\t') && (byte != '\n') && (byte != '\r') )
        {
            return "a control character XML does not allow";
        }

        // ASCII is itself
        if (byte < 0x80u)
        {
            at++;
            continue;
        }

        // a lead byte, and the continuation bytes it calls for
        const size_t follow = ( (byte >= 0xC2u) && (byte <= 0xDFu) ) ? 1u
                            : ( (byte >= 0xE0u) && (byte <= 0xEFu) ) ? 2u
                            : ( (byte >= 0xF0u) && (byte <= 0xF4u) ) ? 3u
                            :                                          0u;
        bool sound = (follow > 0u) && ((at + follow) < (_length + 1u));

        // each continuation byte the lead byte calls for
        for (size_t next = 1; (sound) && (next <= follow); ++next)
        {
            sound = ( (at + next) < _length ) &&
                    ( ((unsigned char)_text[at + next] & 0xC0u) == 0x80u );
        }

        // a character cut short, or overlong, is not UTF-8
        if (!sound)
        {
            return "bytes that are not UTF-8";
        }

        at += follow + 1u;
    }

    return NULL;
}


/*
d_internal_new_element
  Adds an element for a start or empty-element tag, linked beneath the
element open, if any.
*/
static size_t
d_internal_new_element(
    struct d_internal_reader* _reader,
    const struct d_xml_token* _token,
    size_t                    _parent,
    size_t*                   _last_child,
    size_t                    _depth
)
{
    struct d_msvs* const msvs = _reader->msvs;

    // the element is recorded with room for it
    if (!d_internal_grow((void**)&msvs->elements, &_reader->element_capacity,
                         msvs->element_count, sizeof(*msvs->elements)))
    {
        return D_MSVS_NONE;
    }

    const size_t                 which   = msvs->element_count++;
    struct d_msvs_element* const element = &msvs->elements[which];
    size_t                       column  = 0;

    memset(element, 0, sizeof(*element));
    element->start             = _token->start;
    element->name              = (size_t)(_token->name - msvs->text);
    element->name_length       = _token->name_length;
    element->attributes        = (size_t)(_token->attributes - msvs->text);
    element->attributes_length = _token->attributes_length;
    element->line              = d_internal_line_at(_reader, _token->start,
                                                    &column);
    element->column            = column;
    element->depth             = _depth;
    element->parent            = _parent;
    element->first_child       = D_MSVS_NONE;
    element->next_sibling      = D_MSVS_NONE;

    // the new element is its parent's last child
    if (_parent != D_MSVS_NONE)
    {
        struct d_msvs_element* const parent = &msvs->elements[_parent];

        // the first child, or one after the last
        if (parent->first_child == D_MSVS_NONE)
        {
            parent->first_child = which;
        }
        else
        {
            msvs->elements[_last_child[_parent]].next_sibling = which;
        }

        _last_child[_parent] = which;
        parent->child_count++;
    }

    return which;
}


/*
d_internal_blank
  Whether a span holds only space.
*/
static bool
d_internal_blank(
    const char* _text,
    size_t      _length
)
{
    // each byte, space or not
    for (size_t at = 0; at < _length; ++at)
    {
        // anything else is text
        if ( (_text[at] != ' ')  &&
             (_text[at] != '\t') &&
             (_text[at] != '\r') &&
             (_text[at] != '\n') )
        {
            return false;
        }
    }

    return true;
}


/*
d_internal_read_xml
  The element tree: start tags open elements and end tags close them, each
end tag the one its open element needs; one root, and nothing but space,
comments and declarations outside it.
*/
static bool
d_internal_read_xml(
    struct d_internal_reader* _reader,
    size_t                    _from
)
{
    struct d_msvs* const msvs  = _reader->msvs;
    size_t*              open  = NULL;
    size_t               open_capacity = 0;
    size_t*              last  = NULL;
    size_t               last_capacity = 0;
    size_t               depth = 0;
    size_t               roots = 0;
    struct d_xml         xml;
    struct d_xml_token   token;
    bool                 ok    = true;

    memset(&token, 0, sizeof(token));

    size_t            where   = 0;
    const char* const whole   = d_internal_document_problem(msvs->text,
                                                            msvs->length,
                                                            _from, &where);

    // the text as a whole must be text XML allows
    if (whole)
    {
        size_t       column = 0;
        const size_t line   = d_internal_line_at(_reader, where, &column);

        return d_internal_fail(_reader, line, column, whole);
    }

    d_xml_begin(&xml, msvs->text, msvs->length);
    xml.at = _from;

    // every token the scanner gives, until one fails
    while ( (ok) &&
            (d_xml_next(&xml, &token)) )
    {
        size_t column = 0;

        // the last-child links grow with the elements
        if ( (msvs->element_count + 1u) > last_capacity )
        {
            const size_t wanted = (last_capacity == 0u)
                                ? 64u : (last_capacity * 2u);
            size_t* const grown = realloc(last, wanted * sizeof(size_t));

            if (!grown)
            {
                ok = d_internal_fail(_reader, 0u, 0u, "out of memory");
                break;
            }

            last          = grown;
            last_capacity = wanted;
        }

        // text outside the root must be space; inside, it is content
        if (token.kind == D_XML_TEXT)
        {
            const char* const problem =
                (token.cdata) ? NULL
                              : d_internal_text_problem(token.text,
                                                        token.text_length,
                                                        &where);

            // content must be sound
            if (problem)
            {
                const size_t line = d_internal_line_at(
                                        _reader,
                                        (size_t)(token.text - msvs->text)
                                        + where, &column);

                ok = d_internal_fail(_reader, line, column, problem);
                break;
            }

            if ( (depth == 0u) &&
                 (!d_internal_blank(token.text, token.text_length)) )
            {
                size_t stray = token.start;

                // the failure is where the text begins, past its space
                while ( (msvs->text[stray] == ' ')  ||
                        (msvs->text[stray] == '\t') ||
                        (msvs->text[stray] == '\r') ||
                        (msvs->text[stray] == '\n') )
                {
                    stray++;
                }

                const size_t line = d_internal_line_at(_reader, stray,
                                                       &column);

                ok = d_internal_fail(_reader, line, column,
                                     "text stands outside the root "
                                     "element");
            }

            continue;
        }

        // comments, instructions and declarations are nothing here, but
        // must be sound
        if (token.kind == D_XML_OTHER)
        {
            const char* const problem =
                d_internal_markup_problem(&msvs->text[token.start],
                                          token.end - token.start,
                                          token.start, _from, &where);

            if (problem)
            {
                const size_t line = d_internal_line_at(_reader,
                                                       token.start + where,
                                                       &column);

                ok = d_internal_fail(_reader, line, column, problem);
                break;
            }

            continue;
        }

        // a tag's name must be a name
        if (!d_internal_name_sound(token.name, token.name_length))
        {
            const size_t line = d_internal_line_at(_reader, token.start,
                                                   &column);

            ok = d_internal_fail(_reader, line, column,
                                 "a tag's name is not an XML name");
            break;
        }

        // an element's attributes must be sound
        if ( (token.kind == D_XML_START) ||
             (token.kind == D_XML_EMPTY) )
        {
            const char* const problem =
                d_internal_attributes_problem(token.attributes,
                                              token.attributes_length,
                                              &where);

            if (problem)
            {
                const size_t line = d_internal_line_at(
                                        _reader,
                                        (size_t)(token.attributes
                                                 - msvs->text) + where,
                                        &column);

                ok = d_internal_fail(_reader, line, column, problem);
                break;
            }
        }

        // an end tag holds its name alone
        if ( (token.kind == D_XML_CLOSE) &&
             (!d_internal_blank(token.attributes, token.attributes_length)) )
        {
            const size_t line = d_internal_line_at(_reader, token.start,
                                                   &column);

            ok = d_internal_fail(_reader, line, column,
                                 "an end tag holding more than its name");
            break;
        }

        // a start tag or an empty one at the top is a root, and only one
        if ( ( (token.kind == D_XML_START) ||
               (token.kind == D_XML_EMPTY) ) &&
             (depth == 0u) &&
             (++roots > 1u) )
        {
            const size_t line = d_internal_line_at(_reader, token.start,
                                                   &column);

            ok = d_internal_fail(_reader, line, column,
                                 "a second root element");
            break;
        }

        // a start tag or an empty one opens an element
        if ( (token.kind == D_XML_START) ||
             (token.kind == D_XML_EMPTY) )
        {
            const size_t parent = (depth > 0u) ? open[depth - 1u]
                                               : D_MSVS_NONE;
            const size_t which  = d_internal_new_element(_reader, &token,
                                                         parent, last,
                                                         depth);

            if (which == D_MSVS_NONE)
            {
                ok = d_internal_fail(_reader, 0u, 0u, "out of memory");
                break;
            }

            struct d_msvs_element* const element = &msvs->elements[which];

            // an empty element ends where it starts
            if (token.kind == D_XML_EMPTY)
            {
                element->empty         = true;
                element->content_start = token.end;
                element->content_end   = token.end;
                element->end           = token.end;
                element->end_line      = d_internal_line_at(_reader,
                                                            token.end - 1u,
                                                            NULL);
            }
            else if (!d_internal_grow((void**)&open, &open_capacity, depth,
                                      sizeof(size_t)))
            {
                ok = d_internal_fail(_reader, 0u, 0u, "out of memory");
                break;
            }
            else
            {
                element->content_start = token.end;
                open[depth++]          = which;
            }

            continue;
        }

        // an end tag closes the element open, which must be its own
        if (token.kind == D_XML_CLOSE)
        {
            const size_t line = d_internal_line_at(_reader, token.start,
                                                   &column);

            // an end tag with nothing open is one too many
            if (depth == 0u)
            {
                ok = d_internal_fail(_reader, line, column,
                                     "an end tag with no element open");
                break;
            }

            struct d_msvs_element* const element =
                &msvs->elements[open[depth - 1u]];

            // and one for an element of another name ends the wrong one
            if ( (element->name_length != token.name_length) ||
                 (memcmp(&msvs->text[element->name], token.name,
                         token.name_length) != 0) )
            {
                char why[160];

                (void)snprintf(why, sizeof(why),
                               "</%.*s> ends <%.*s>",
                               (int)((token.name_length < 60u)
                                     ? token.name_length : 60u),
                               token.name,
                               (int)((element->name_length < 60u)
                                     ? element->name_length : 60u),
                               &msvs->text[element->name]);
                ok = d_internal_fail(_reader, line, column, why);
                break;
            }

            element->content_end = token.start;
            element->end         = token.end;
            element->end_line    = d_internal_line_at(_reader,
                                                      token.end - 1u, NULL);
            depth--;
        }
    }

    // where the scanner stopped, when it stopped on bad markup
    if ( (ok) &&
         (token.kind == D_XML_ERROR) )
    {
        size_t       column = 0;
        const size_t line   = d_internal_line_at(_reader,
                                                 (token.start < msvs->length)
                                                 ? token.start
                                                 : msvs->length, &column);

        ok = d_internal_fail(_reader, line, column,
                             "markup that is not well formed");
    }

    // every element closes, and there is one
    if ( (ok) &&
         (depth > 0u) )
    {
        const struct d_msvs_element* const element =
            &msvs->elements[open[depth - 1u]];

        ok = d_internal_fail(_reader, element->line, element->column,
                             "an element is never closed");
    }

    // and there is one
    if ( (ok) &&
         (roots == 0u) )
    {
        size_t       column = 0;
        const size_t line   = d_internal_line_at(_reader, msvs->length,
                                                 &column);

        ok = d_internal_fail(_reader, line, column, "no root element");
    }

    free(open);
    free(last);

    return ok;
}


//==============================================================================
// 4.  SOLUTIONS
//==============================================================================


/*
d_internal_add_text
  Adds a string to one of the solution's lists.
*/
static bool
d_internal_add_text(
    char***     _list,
    size_t*     _count,
    size_t*     _capacity,
    const char* _text,
    size_t      _length
)
{
    // the string is kept with room for it
    if (!d_internal_grow((void**)_list, _capacity, *_count, sizeof(char*)))
    {
        return false;
    }

    char* const copy = d_internal_copy(_text, _length);

    if (!copy)
    {
        return false;
    }

    (*_list)[(*_count)++] = copy;

    return true;
}


/*
d_internal_add_project
  Adds a project, owning the strings it is given; NULL ones become "".
*/
static struct d_msvs_project*
d_internal_add_project(
    struct d_internal_reader* _reader,
    char*                     _name,
    char*                     _path,
    char*                     _type,
    char*                     _id,
    char*                     _folder,
    size_t                    _line
)
{
    struct d_msvs* const msvs = _reader->msvs;

    // the project needs room, and every string it holds
    if ( (!_name) || (!_path) || (!_type) || (!_id) || (!_folder) ||
         (!d_internal_grow((void**)&msvs->projects,
                           &_reader->project_capacity, msvs->project_count,
                           sizeof(*msvs->projects))) )
    {
        free(_name);
        free(_path);
        free(_type);
        free(_id);
        free(_folder);

        return NULL;
    }

    struct d_msvs_project* const project =
        &msvs->projects[msvs->project_count++];

    project->name             = _name;
    project->path             = _path;
    project->type             = _type;
    project->id               = _id;
    project->folder           = _folder;
    project->first_dependency = msvs->dependency_count;
    project->dependency_count = 0;
    project->line             = _line;
    project->start            = 0;
    project->end              = 0;
    project->element          = D_MSVS_NONE;
    project->depends          = D_MSVS_NONE;
    project->depends_close    = D_MSVS_NONE;

    return project;
}


/*
d_internal_add_folder
  Adds a folder, owning the strings it is given; NULL ones fail.
*/
static struct d_msvs_folder*
d_internal_add_folder(
    struct d_internal_reader* _reader,
    char*                     _path,
    char*                     _id,
    size_t                    _line
)
{
    struct d_msvs* const msvs = _reader->msvs;

    // the folder needs room, and both its strings
    if ( (!_path) || (!_id) ||
         (!d_internal_grow((void**)&msvs->folders, &_reader->folder_capacity,
                           msvs->folder_count, sizeof(*msvs->folders))) )
    {
        free(_path);
        free(_id);

        return NULL;
    }

    struct d_msvs_folder* const folder = &msvs->folders[msvs->folder_count++];

    folder->path    = _path;
    folder->id      = _id;
    folder->line    = _line;
    folder->start   = 0;
    folder->end     = 0;
    folder->element = D_MSVS_NONE;

    return folder;
}


/*
d_internal_stem
  A path's file name without its extension.
*/
static char*
d_internal_stem(
    const char* _path
)
{
    const char* name = _path;

    // the name after the last separator of either kind
    for (const char* at = _path; *at != '\0'; ++at)
    {
        // the name follows the last separator of either kind
        if ( (*at == '/') ||
             (*at == '\\') )
        {
            name = at + 1;
        }
    }

    const char* const dot = strrchr(name, '.');

    return d_internal_copy(name, (dot && (dot > name))
                                 ? (size_t)(dot - name) : strlen(name));
}


/*
d_internal_decoded_attribute
  An attribute's value, decoded, as a string of its own; "" when absent.
*/
static char*
d_internal_decoded_attribute(
    const struct d_msvs*         _msvs,
    const struct d_msvs_element* _element,
    const char*                  _name
)
{
    size_t start  = 0;
    size_t length = 0;

    // an attribute that is not there is ""
    if (!d_msvs_attribute(_msvs, _element, _name, &start, &length))
    {
        return d_internal_copy("", 0u);
    }

    char* const value = malloc(length + 1u);

    if (value)
    {
        (void)d_msvs_value(_msvs, start, length, value);
    }

    return value;
}


/*
d_internal_read_slnx
  The projects of a .slnx: every <Project> under the <Solution> or one of
its <Folder>s, its <BuildDependency>s, and the configurations: every
<BuildType> of <Configurations> with every <Platform>, Debug and Release
when only platforms are named.
*/
static bool
d_internal_read_slnx(
    struct d_internal_reader* _reader
)
{
    struct d_msvs* const msvs  = _reader->msvs;
    char**               types = NULL;
    size_t               type_count = 0;
    size_t               type_capacity = 0;
    char**               platforms = NULL;
    size_t               platform_count = 0;
    size_t               platform_capacity = 0;
    bool                 ok    = true;

    // every element, in document order
    for (size_t at = 0; (ok) && (at < msvs->element_count); ++at)
    {
        const struct d_msvs_element* const element = &msvs->elements[at];
        const struct d_msvs_element* const parent  =
            (element->parent != D_MSVS_NONE)
            ? &msvs->elements[element->parent] : NULL;

        // the build types and platforms the solution names
        if ( (parent) &&
             (d_msvs_is(msvs, parent, "Configurations")) &&
             ( (d_msvs_is(msvs, element, "BuildType")) ||
               (d_msvs_is(msvs, element, "Platform")) ) )
        {
            char* const name = d_internal_decoded_attribute(msvs, element,
                                                            "Name");
            const bool  type = d_msvs_is(msvs, element, "BuildType");

            ok = (name != NULL) &&
                 (d_internal_add_text(type ? &types : &platforms,
                                      type ? &type_count : &platform_count,
                                      type ? &type_capacity
                                           : &platform_capacity,
                                      name, strlen(name)));
            free(name);
            continue;
        }

        // a folder, by the path it names
        if ( (parent) &&
             (d_msvs_is(msvs, parent, "Solution")) &&
             (d_msvs_is(msvs, element, "Folder")) )
        {
            char* const id    = d_internal_decoded_attribute(msvs, element,
                                                             "Id");
            char* const ident = (id) ? d_internal_lower_guid(id, strlen(id))
                                     : NULL;
            struct d_msvs_folder* const folder =
                d_internal_add_folder(_reader,
                                      d_internal_decoded_attribute(msvs,
                                                                   element,
                                                                   "Name"),
                                      ident, element->line);

            free(id);
            ok = (folder != NULL);

            if (ok)
            {
                folder->start   = element->start;
                folder->end     = element->end;
                folder->element = at;
            }

            continue;
        }

        // a project, within a folder or not
        if (!d_msvs_is(msvs, element, "Project"))
        {
            continue;
        }

        char* const path    = d_internal_decoded_attribute(msvs, element,
                                                           "Path");
        char* const display = d_internal_decoded_attribute(msvs, element,
                                                           "DisplayName");
        char* const type    = d_internal_decoded_attribute(msvs, element,
                                                           "Type");
        char* const id      = d_internal_decoded_attribute(msvs, element,
                                                           "Id");
        char* const folder  = ( (parent) &&
                                (d_msvs_is(msvs, parent, "Folder")) )
                            ? d_internal_decoded_attribute(msvs, parent,
                                                           "Name")
                            : d_internal_copy("", 0u);
        char* const name    = ( (display) && (display[0] != '\0') )
                            ? d_internal_copy(display, strlen(display))
                            : ( (path) ? d_internal_stem(path) : NULL );
        char* const guid    = (type) ? d_internal_lower_guid(type,
                                                             strlen(type))
                                     : NULL;
        char* const ident   = (id) ? d_internal_lower_guid(id, strlen(id))
                                   : NULL;

        free(display);
        free(type);
        free(id);

        struct d_msvs_project* const project =
            d_internal_add_project(_reader, name, path, guid, ident, folder,
                                   element->line);

        ok = (project != NULL);

        // its record is its element
        if (ok)
        {
            project->start   = element->start;
            project->end     = element->end;
            project->element = at;
        }

        // its dependencies, each a project's path
        for (size_t child = element->first_child;
             (ok) && (child != D_MSVS_NONE);
             child = msvs->elements[child].next_sibling)
        {
            // only <BuildDependency> names a dependency
            if (!d_msvs_is(msvs, &msvs->elements[child], "BuildDependency"))
            {
                continue;
            }

            char* const depends = d_internal_decoded_attribute(
                                      msvs, &msvs->elements[child],
                                      "Project");

            ok = (depends != NULL) &&
                 (d_internal_add_text(&msvs->dependencies,
                                      &msvs->dependency_count,
                                      &_reader->dependency_capacity,
                                      depends, strlen(depends)));
            free(depends);

            if (ok)
            {
                msvs->projects[msvs->project_count - 1u].dependency_count++;
            }
        }
    }

    // platforms named with no build type build Visual Studio's two
    if ( (ok) &&
         (type_count == 0u) &&
         (platform_count > 0u) )
    {
        ok = (d_internal_add_text(&types, &type_count, &type_capacity,
                                  "Debug", 5u)) &&
             (d_internal_add_text(&types, &type_count, &type_capacity,
                                  "Release", 7u));
    }

    // each build type on each platform, or alone when none is named
    for (size_t type = 0; (ok) && (type < type_count); ++type)
    {
        // each platform, or none when none is named
        for (size_t platform = 0;
             (ok) && (platform < ((platform_count > 0u) ? platform_count
                                                         : 1u));
             ++platform)
        {
            char         both[256];
            const size_t length =
                (size_t)snprintf(both, sizeof(both), "%s%s%s", types[type],
                                 (platform_count > 0u) ? "|" : "",
                                 (platform_count > 0u) ? platforms[platform]
                                                       : "");

            ok = d_internal_add_text(&msvs->configurations,
                                     &msvs->configuration_count,
                                     &_reader->configuration_capacity, both,
                                     (length < sizeof(both))
                                     ? length : (sizeof(both) - 1u));
        }
    }

    // the names are freed, both lists
    for (size_t at = 0; at < type_count; ++at)
    {
        free(types[at]);
    }

    for (size_t at = 0; at < platform_count; ++at)
    {
        free(platforms[at]);
    }

    free(types);
    free(platforms);

    // memory, not the file, is all that can fail here
    if (!ok)
    {
        return d_internal_fail(_reader, 0u, 0u, "out of memory");
    }

    return true;
}


// d_internal_sln_folder
//   struct: a .sln solution folder, or a nesting of one thing in another;
//   a folder's `record` is its record among the folders.
struct d_internal_sln_folder
{
    char*  id;
    char*  name;
    char*  parent;
    size_t record;
};


/*
d_internal_quoted
  The next "..." on a line at or after `*_at`, moving past it.
*/
static bool
d_internal_quoted(
    const char* _line,
    size_t      _length,
    size_t*     _at,
    size_t*     _start,
    size_t*     _span
)
{
    size_t at = *_at;

    // the opening quote
    while ( (at < _length) &&
            (_line[at] != '"') )
    {
        at++;
    }

    // a quote must open it
    if (at >= _length)
    {
        return false;
    }

    const size_t start = ++at;

    // and the closing one
    while ( (at < _length) &&
            (_line[at] != '"') )
    {
        at++;
    }

    // and another close it
    if (at >= _length)
    {
        return false;
    }

    *_start = start;
    *_span  = at - start;
    *_at    = at + 1u;

    return true;
}


/*
d_internal_trimmed
  A line without its leading and trailing space.
*/
static void
d_internal_trimmed(
    const char*  _line,
    size_t       _length,
    const char** _out,
    size_t*      _out_length
)
{
    size_t from = 0;
    size_t to   = _length;

    // past the space it begins with
    while ( (from < to) &&
            ( (_line[from] == ' ') || (_line[from] == '\t') ) )
    {
        from++;
    }

    // back over the space it ends with
    while ( (to > from) &&
            ( (_line[to - 1u] == ' ')  || (_line[to - 1u] == '\t') ||
              (_line[to - 1u] == '\r') ) )
    {
        to--;
    }

    *_out        = &_line[from];
    *_out_length = to - from;

    return;
}


/*
d_internal_starts
  Whether a trimmed line begins with a word.
*/
static bool
d_internal_starts(
    const char* _line,
    size_t      _length,
    const char* _word
)
{
    const size_t span = strlen(_word);

    return ( (_length >= span) &&
             (memcmp(_line, _word, span) == 0) );
}


/*
d_internal_is
  Whether a trimmed line is a word and nothing more.
*/
static bool
d_internal_is(
    const char* _line,
    size_t      _length,
    const char* _word
)
{
    return ( (_length == strlen(_word)) &&
             (d_internal_starts(_line, _length, _word)) );
}


/*
d_internal_nesting
  The folders holding a thing, outward from it, as "/outer/inner/": "/" for
a thing nested in none.
*/
static void
d_internal_nesting(
    const struct d_internal_sln_folder* _folders,
    size_t                              _folder_count,
    const struct d_internal_sln_folder* _nests,
    size_t                              _nest_count,
    const char*                         _id,
    char*                               _out,
    size_t                              _capacity
)
{
    const char* id    = _id;
    size_t      steps = 0;

    (void)snprintf(_out, _capacity, "/");

    // a nesting that loops is cut short
    while (steps++ < 64u)
    {
        const char* parent = NULL;

        // what holds it, by its id
        for (size_t nest = 0; nest < _nest_count; ++nest)
        {
            if (strcmp(_nests[nest].id, id) == 0)
            {
                parent = _nests[nest].parent;
                break;
            }
        }

        // the outermost folder ends the walk
        if (!parent)
        {
            break;
        }

        const char* name = NULL;

        // the folder that is, by its id
        for (size_t folder = 0; folder < _folder_count; ++folder)
        {
            if (strcmp(_folders[folder].id, parent) == 0)
            {
                name = _folders[folder].name;
                break;
            }
        }

        // a folder not listed ends the nesting
        if (!name)
        {
            break;
        }

        // the folder found goes before those already found
        char         rest[512];
        const size_t written = (size_t)snprintf(rest, sizeof(rest), "/%s%s",
                                                name, _out);

        // the path is kept while it fits
        if (written < _capacity)
        {
            memcpy(_out, rest, written + 1u);
        }

        id = parent;
    }

    return;
}


/*
d_internal_add_section
  Records a global section opening on a line, by the name in its
parentheses.
*/
static bool
d_internal_add_section(
    struct d_internal_reader* _reader,
    const char*               _text,
    size_t                    _length,
    size_t                    _start,
    size_t                    _content,
    size_t                    _line
)
{
    struct d_msvs* const msvs  = _reader->msvs;
    const char* const    open  = memchr(_text, '(', _length);
    const char* const    close = (open)
                                 ? memchr(open, ')',
                                          _length - (size_t)(open - _text))
                                 : NULL;

    // the section needs room
    if (!d_internal_grow((void**)&msvs->sections, &_reader->section_capacity,
                         msvs->section_count, sizeof(*msvs->sections)))
    {
        return false;
    }

    char* const name = ( (open) && (close) )
                     ? d_internal_copy(open + 1,
                                       (size_t)(close - open) - 1u)
                     : d_internal_copy("", 0u);

    // a name that cannot be copied is memory run out
    if (!name)
    {
        return false;
    }

    struct d_msvs_section* const section =
        &msvs->sections[msvs->section_count++];

    section->name    = name;
    section->line    = _line;
    section->start   = _start;
    section->content = _content;
    section->close   = D_MSVS_NONE;
    section->end     = D_MSVS_NONE;

    return true;
}


/*
d_internal_read_sln
  The projects of a .sln, a line at a time: Project("{type}") = "name",
"path", "{id}" up to EndProject, with the dependencies its
ProjectSection(ProjectDependencies) lists; then Global, and its sections:
SolutionConfigurationPlatforms for the configurations, NestedProjects for
the folders.  A solution folder is a project of a type of its own, and is
read as a folder.  Each record keeps the bytes its lines span.
*/
static bool
d_internal_read_sln(
    struct d_internal_reader* _reader,
    size_t                    _from
)
{
    struct d_msvs* const          msvs     = _reader->msvs;
    struct d_internal_sln_folder* folders  = NULL;
    size_t                        folder_count    = 0;
    size_t                        folder_capacity = 0;
    struct d_internal_sln_folder* nests    = NULL;
    size_t                        nest_count      = 0;
    size_t                        nest_capacity   = 0;
    char**                        ids      = NULL;   // dependencies, by id
    size_t                        id_capacity     = 0;
    size_t                        at       = _from;
    size_t                        line     = 1;
    enum { D_TOP, D_PROJECT, D_DEPENDS, D_CONFIGS, D_NESTED, D_SECTION }
                                  where    = D_TOP;
    enum { D_NOTHING, D_A_PROJECT, D_A_FOLDER }
                                  opened   = D_NOTHING;
    bool                          header   = false;
    bool                          global   = false;
    bool                          ok       = true;
    size_t                        open_line = 0;

    // one line at a time
    while ( (ok) &&
            (at < msvs->length) )
    {
        const char* const raw   = &msvs->text[at];
        const char* const ends  = memchr(raw, '\n', msvs->length - at);
        const size_t      span  = (ends) ? (size_t)(ends - raw)
                                         : (msvs->length - at);
        const size_t      next  = (ends) ? (at + span + 1u) : msvs->length;
        const char*       text  = NULL;
        size_t            length = 0;

        d_internal_trimmed(raw, span, &text, &length);

        // the first line ending says how the file ends its lines
        if ( (line == 1u) &&
             (ends) )
        {
            msvs->crlf = ( (span > 0u) && (raw[span - 1u] == '\r') );
        }

        // blank lines and comments are nothing
        if ( (length == 0u) ||
             (text[0] == '#') )
        {
            at = next;
            line++;
            continue;
        }

        // the first line of any other kind names the format
        if (!header)
        {
            // the header, which every .sln begins with
            if (!d_internal_starts(text, length,
                                   "Microsoft Visual Studio Solution File"))
            {
                ok = d_internal_fail(_reader, line, 1u,
                                     "not a Visual Studio solution");
                break;
            }

            header = true;
        }
        else if ( (where == D_TOP) &&
                  (!global) &&
                  (d_internal_starts(text, length, "Project(")) )
        {
            size_t cursor = 0;
            size_t parts[4][2];
            bool   whole  = true;

            // the type, the name, the path and the id, each quoted
            for (size_t part = 0; (whole) && (part < 4u); ++part)
            {
                whole = d_internal_quoted(text, length, &cursor,
                                          &parts[part][0], &parts[part][1]);
            }

            // four quoted parts, or the line is not a project's
            if (!whole)
            {
                ok = d_internal_fail(_reader, line, 1u,
                                     "a Project line is not "
                                     "Project(\"{type}\") = \"name\", "
                                     "\"path\", \"{id}\"");
                break;
            }

            char* const type = d_internal_lower_guid(&text[parts[0][0]],
                                                     parts[0][1]);
            char* const id   = d_internal_lower_guid(&text[parts[3][0]],
                                                     parts[3][1]);

            // a solution folder is a folder, and nothing more
            if ( (type) &&
                 (strcmp(type, D_INTERNAL_FOLDER_TYPE) == 0) )
            {
                char* const own = (id) ? d_internal_copy(id, strlen(id))
                                       : NULL;
                struct d_msvs_folder* const folder =
                    d_internal_add_folder(_reader, d_internal_copy("", 0u),
                                          own, line);

                free(type);
                ok = (id != NULL) && (folder != NULL) &&
                     (d_internal_grow((void**)&folders, &folder_capacity,
                                      folder_count, sizeof(*folders)));

                if (ok)
                {
                    folder->start                = at;
                    folders[folder_count].id     = id;
                    folders[folder_count].name   =
                        d_internal_copy(&text[parts[1][0]], parts[1][1]);
                    folders[folder_count].parent = NULL;
                    folders[folder_count].record = msvs->folder_count - 1u;
                    ok = (folders[folder_count++].name != NULL);
                }
                else
                {
                    free(id);
                }

                opened = D_A_FOLDER;
            }
            else
            {
                struct d_msvs_project* const project =
                    d_internal_add_project(
                        _reader,
                        d_internal_copy(&text[parts[1][0]], parts[1][1]),
                        d_internal_copy(&text[parts[2][0]], parts[2][1]),
                        type, id, d_internal_copy("", 0u), line);

                ok = (project != NULL);

                if (ok)
                {
                    project->start = at;
                }

                opened = D_A_PROJECT;
            }

            where     = D_PROJECT;
            open_line = line;
        }
        else if ( (where == D_PROJECT) &&
                  (opened == D_A_PROJECT) &&
                  (d_internal_starts(text, length,
                                     "ProjectSection(ProjectDependencies)")) )
        {
            msvs->projects[msvs->project_count - 1u].depends = at;
            where = D_DEPENDS;
        }
        else if ( (where == D_PROJECT) &&
                  (d_internal_starts(text, length, "ProjectSection(")) )
        {
            where = D_SECTION;
        }
        else if ( ( (where == D_DEPENDS) || (where == D_SECTION) ) &&
                  (!global) &&
                  (d_internal_starts(text, length, "EndProjectSection")) )
        {
            // the dependencies end where their section does
            if (where == D_DEPENDS)
            {
                msvs->projects[msvs->project_count - 1u].depends_close = at;
            }

            where = D_PROJECT;
        }
        else if (where == D_DEPENDS)
        {
            const char* const equals = memchr(text, '=', length);
            const char*       left   = NULL;
            size_t            span_left = 0;

            // a line of the section that is no dependency is passed over
            if (!equals)
            {
                at = next;
                line++;
                continue;
            }

            d_internal_trimmed(text, (size_t)(equals - text), &left,
                               &span_left);

            // a dependency of the project open, by its id
            char* const id = d_internal_lower_guid(left, span_left);

            ok = (id != NULL) &&
                 (d_internal_grow((void**)&ids, &id_capacity,
                                  msvs->dependency_count, sizeof(char*))) &&
                 (d_internal_add_text(&msvs->dependencies,
                                      &msvs->dependency_count,
                                      &_reader->dependency_capacity, "", 0u));

            if (ok)
            {
                ids[msvs->dependency_count - 1u] = id;
                msvs->projects[msvs->project_count - 1u].dependency_count++;
            }
            else
            {
                free(id);
            }
        }
        else if ( (where == D_PROJECT) &&
                  (d_internal_is(text, length, "EndProject")) )
        {
            // the record ends past its last line
            if (opened == D_A_PROJECT)
            {
                msvs->projects[msvs->project_count - 1u].end = next;
            }
            else
            {
                msvs->folders[msvs->folder_count - 1u].end = next;
            }

            where  = D_TOP;
            opened = D_NOTHING;
        }
        else if ( (where == D_TOP) &&
                  (!global) &&
                  (d_internal_is(text, length, "Global")) )
        {
            msvs->global_start = at;
            global             = true;
            open_line          = line;
        }
        else if ( (where == D_TOP) &&
                  (global) &&
                  (d_internal_is(text, length, "EndGlobal")) )
        {
            msvs->global_close = at;
            msvs->global_end   = next;
            global             = false;
        }
        else if ( (where == D_TOP) &&
                  (d_internal_starts(text, length, "GlobalSection(")) )
        {
            where = (d_internal_starts(text, length,
                                       "GlobalSection("
                                       "SolutionConfigurationPlatforms)"))
                  ? D_CONFIGS
                  : (d_internal_starts(text, length,
                                       "GlobalSection(NestedProjects)"))
                  ? D_NESTED
                  : D_SECTION;
            open_line = line;
            ok        = d_internal_add_section(_reader, text, length, at,
                                               next, line);
        }
        else if ( ( (where == D_CONFIGS) || (where == D_NESTED) ||
                    (where == D_SECTION) ) &&
                  (d_internal_starts(text, length, "EndGlobalSection")) )
        {
            struct d_msvs_section* const section =
                &msvs->sections[msvs->section_count - 1u];

            section->close = at;
            section->end   = next;
            where          = D_TOP;
        }
        else if ( ( (where == D_CONFIGS) || (where == D_NESTED) ) &&
                  (memchr(text, '=', length) != NULL) )
        {
            const char* const equals = memchr(text, '=', length);
            const char*       left   = NULL;
            size_t            span_left = 0;
            const char*       right  = NULL;
            size_t            span_right = 0;

            d_internal_trimmed(text, (size_t)(equals - text), &left,
                               &span_left);
            d_internal_trimmed(equals + 1,
                               length - (size_t)(equals - text) - 1u,
                               &right, &span_right);

            // a configuration, or a thing nested in a folder
            if (where == D_CONFIGS)
            {
                ok = d_internal_add_text(&msvs->configurations,
                                         &msvs->configuration_count,
                                         &_reader->configuration_capacity,
                                         left, span_left);
            }
            else
            {
                ok = d_internal_grow((void**)&nests, &nest_capacity,
                                     nest_count, sizeof(*nests));

                if (ok)
                {
                    nests[nest_count].id     =
                        d_internal_lower_guid(left, span_left);
                    nests[nest_count].parent =
                        d_internal_lower_guid(right, span_right);
                    nests[nest_count].name   = NULL;
                    ok = (nests[nest_count].id != NULL) &&
                         (nests[nest_count].parent != NULL);
                    nest_count++;
                }
            }
        }

        at = next;
        line++;
    }

    // the format is named, and every project, section and Global ends
    if ( (ok) &&
         (!header) )
    {
        ok = d_internal_fail(_reader, 1u, 1u,
                             "not a Visual Studio solution");
    }

    if ( (ok) &&
         ( (where != D_TOP) || (global) ) )
    {
        ok = d_internal_fail(_reader, open_line, 1u,
                             (where != D_TOP)
                             ? "a project or section is never ended"
                             : "Global is never ended");
    }

    // dependencies by id become dependencies by path, where the id is known
    for (size_t which = 0; (ok) && (which < msvs->dependency_count); ++which)
    {
        const char* path = ids[which];

        for (size_t project = 0; project < msvs->project_count; ++project)
        {
            if (strcmp(msvs->projects[project].id, ids[which]) == 0)
            {
                path = msvs->projects[project].path;
                break;
            }
        }

        char* const copy = d_internal_copy(path, strlen(path));

        ok = (copy != NULL);

        if (ok)
        {
            free(msvs->dependencies[which]);
            msvs->dependencies[which] = copy;
        }
    }

    // each project's folder: the names of the folders holding it, outward
    for (size_t project = 0; (ok) && (project < msvs->project_count);
         ++project)
    {
        char path[512];

        d_internal_nesting(folders, folder_count, nests, nest_count,
                           msvs->projects[project].id, path, sizeof(path));

        // a project at the top has no folder
        if (strcmp(path, "/") == 0)
        {
            continue;
        }

        char* const folder = d_internal_copy(path, strlen(path));

        ok = (folder != NULL);

        if (ok)
        {
            free(msvs->projects[project].folder);
            msvs->projects[project].folder = folder;
        }
    }

    // and each folder's path: the folders holding it, then its own name
    for (size_t which = 0; (ok) && (which < folder_count); ++which)
    {
        char         path[512];
        char         own[600];
        const size_t record = folders[which].record;

        d_internal_nesting(folders, folder_count, nests, nest_count,
                           folders[which].id, path, sizeof(path));

        const size_t written = (size_t)snprintf(own, sizeof(own), "%s%s/",
                                                path, folders[which].name);
        char* const  copy    = d_internal_copy(own, (written < sizeof(own))
                                                    ? written
                                                    : (sizeof(own) - 1u));

        ok = (copy != NULL);

        if (ok)
        {
            free(msvs->folders[record].path);
            msvs->folders[record].path = copy;
        }
    }

    for (size_t which = 0; which < folder_count; ++which)
    {
        free(folders[which].id);
        free(folders[which].name);
    }

    for (size_t which = 0; which < nest_count; ++which)
    {
        free(nests[which].id);
        free(nests[which].parent);
    }

    for (size_t which = 0; (ids) && (which < msvs->dependency_count);
         ++which)
    {
        free(ids[which]);
    }

    free(folders);
    free(nests);
    free(ids);

    // a failure not yet said is a failure of memory
    if ( (!ok) &&
         (!_reader->failed) )
    {
        return d_internal_fail(_reader, 0u, 0u, "out of memory");
    }

    return ok;
}


//==============================================================================
// 5.  READING
//==============================================================================


/*
d_msvs_read
  XML or .sln by the text's first byte that is not space, after any
byte-order mark.
*/
bool
d_msvs_read(
    struct d_msvs*   _out,
    const char*      _text,
    size_t           _length,
    enum d_msvs_kind _kind,
    char*            _problem,
    size_t           _capacity
)
{
    // parameter validation first
    if (!_out)
    {
        return false;
    }

    memset(_out, 0, sizeof(*_out));
    _out->text         = (_text) ? _text : "";
    _out->length       = (_text) ? _length : 0u;
    _out->kind         = _kind;
    _out->global_start = D_MSVS_NONE;
    _out->global_close = D_MSVS_NONE;
    _out->global_end   = D_MSVS_NONE;

    if ( (_problem) &&
         (_capacity > 0u) )
    {
        _problem[0] = '\0';
    }

    struct d_internal_reader reader;

    memset(&reader, 0, sizeof(reader));
    reader.msvs     = _out;
    reader.line     = 1u;
    reader.problem  = _problem;
    reader.capacity = _capacity;

    size_t from = 0;

    // a byte-order mark is no part of the file's content
    if ( (_out->length >= 3u)                        &&
         ((unsigned char)_out->text[0] == 0xEFu)     &&
         ((unsigned char)_out->text[1] == 0xBBu)     &&
         ((unsigned char)_out->text[2] == 0xBFu) )
    {
        _out->bom         = true;
        from              = 3u;
        reader.counted    = 3u;
        reader.line_start = 3u;
    }

    size_t first = from;

    while ( (first < _out->length) &&
            ( (_out->text[first] == ' ')  || (_out->text[first] == '\t') ||
              (_out->text[first] == '\r') || (_out->text[first] == '\n') ) )
    {
        first++;
    }

    _out->xml = ( (first < _out->length) &&
                  (_out->text[first] == '<') );

    // a .sln is text; everything else Visual Studio writes is XML
    if (!_out->xml)
    {
        if ( (_kind != D_MSVS_OTHER) &&
             (_kind != D_MSVS_SOLUTION) )
        {
            return d_internal_fail(&reader, 1u, 1u,
                                   "an MSBuild file must be XML");
        }

        _out->kind = D_MSVS_SOLUTION;

        return d_internal_read_sln(&reader, from);
    }

    if (!d_internal_read_xml(&reader, from))
    {
        return false;
    }

    const struct d_msvs_element* const root = &_out->elements[0];
    const bool solution = d_msvs_is(_out, root, "Solution");
    const bool project  = d_msvs_is(_out, root, "Project");

    // the root says what the file is, and must agree with its name
    if ( ( (_kind == D_MSVS_SOLUTION) && (!solution) ) ||
         ( (_kind != D_MSVS_SOLUTION) && (_kind != D_MSVS_OTHER) &&
           (!project) ) )
    {
        return d_internal_fail(&reader, root->line, root->column,
                               (_kind == D_MSVS_SOLUTION)
                               ? "a solution's root must be <Solution>"
                               : "an MSBuild file's root must be "
                                 "<Project>");
    }

    if (_out->kind == D_MSVS_OTHER)
    {
        _out->kind = (solution) ? D_MSVS_SOLUTION
                   : (project)  ? D_MSVS_PROPS
                   :              D_MSVS_OTHER;
    }

    // a solution's projects are read off its tree
    if (_out->kind == D_MSVS_SOLUTION)
    {
        return d_internal_read_slnx(&reader);
    }

    return true;
}


/*
d_msvs_release
  Frees what the reading made; the text was never the reader's.
*/
void
d_msvs_release(
    struct d_msvs* _msvs
)
{
    // releasing nothing is allowed
    if (!_msvs)
    {
        return;
    }

    for (size_t at = 0; at < _msvs->project_count; ++at)
    {
        free(_msvs->projects[at].name);
        free(_msvs->projects[at].path);
        free(_msvs->projects[at].type);
        free(_msvs->projects[at].id);
        free(_msvs->projects[at].folder);
    }

    for (size_t at = 0; at < _msvs->folder_count; ++at)
    {
        free(_msvs->folders[at].path);
        free(_msvs->folders[at].id);
    }

    for (size_t at = 0; at < _msvs->section_count; ++at)
    {
        free(_msvs->sections[at].name);
    }

    for (size_t at = 0; at < _msvs->dependency_count; ++at)
    {
        free(_msvs->dependencies[at]);
    }

    for (size_t at = 0; at < _msvs->configuration_count; ++at)
    {
        free(_msvs->configurations[at]);
    }

    free(_msvs->elements);
    free(_msvs->projects);
    free(_msvs->folders);
    free(_msvs->sections);
    free(_msvs->dependencies);
    free(_msvs->configurations);
    memset(_msvs, 0, sizeof(*_msvs));

    return;
}


//==============================================================================
// 6.  ELEMENTS
//==============================================================================


/*
d_msvs_is
  Exact, as XML names compare.
*/
bool
d_msvs_is(
    const struct d_msvs*         _msvs,
    const struct d_msvs_element* _element,
    const char*                  _name
)
{
    // an element must be named to be compared
    if ( (!_msvs)    ||
         (!_element) ||
         (!_name) )
    {
        return false;
    }

    const size_t span = strlen(_name);

    return ( (_element->name_length == span) &&
             (memcmp(&_msvs->text[_element->name], _name, span) == 0) );
}


/*
d_msvs_attribute
  Walks the attributes as name, `=`, quoted value, which reading checked.
*/
bool
d_msvs_attribute(
    const struct d_msvs*         _msvs,
    const struct d_msvs_element* _element,
    const char*                  _name,
    size_t*                      _out_start,
    size_t*                      _out_length
)
{
    // an element must be given, and the name sought
    if ( (!_msvs)    ||
         (!_element) ||
         (!_name) )
    {
        return false;
    }

    struct d_xml_token token;
    const char*        value  = NULL;
    size_t             length = 0;

    memset(&token, 0, sizeof(token));
    token.kind              = D_XML_START;
    token.attributes        = &_msvs->text[_element->attributes];
    token.attributes_length = _element->attributes_length;

    // the attribute, where it is
    if (!d_xml_attribute(&token, _name, &value, &length))
    {
        return false;
    }

    *_out_start  = (size_t)(value - _msvs->text);
    *_out_length = length;

    return true;
}


/*
d_internal_decode
  Decodes raw XML text as an XML processor must: every line ending, CR LF or
a lone CR, is LF; in an attribute's value, each tab and line ending is then
a space; and a reference is the character it names, which no normalizing
touches.  CDATA has its line endings normalized and nothing else.
*/
static size_t
d_internal_decode(
    const char* _raw,
    size_t      _length,
    bool        _attribute,
    bool        _cdata,
    char*       _out
)
{
    size_t used = 0;
    size_t at   = 0;

    while (at < _length)
    {
        const char byte = _raw[at];

        // a line ending, of either kind, is one LF, or a space in a value
        if ( (byte == '\r') ||
             (byte == '\n') )
        {
            _out[used++] = (_attribute) ? ' ' : '\n';
            at += ( (byte == '\r')           &&
                    ((at + 1u) < _length)    &&
                    (_raw[at + 1u] == '\n') ) ? 2u : 1u;
            continue;
        }

        // a tab in a value is a space
        if ( (byte == '\t') &&
             (_attribute) )
        {
            _out[used++] = ' ';
            at++;
            continue;
        }

        // a reference is decoded alone, so what it names is never normalized
        if ( (byte == '&') &&
             (!_cdata) )
        {
            size_t end = at + 1u;

            while ( (end < _length)        &&
                    ((end - at) < 16u)    &&
                    (_raw[end] != ';')    &&
                    (_raw[end] != '&') )
            {
                end++;
            }

            if ( (end < _length) &&
                 (_raw[end] == ';') )
            {
                used += d_xml_decode(&_raw[at], (end - at) + 1u,
                                     &_out[used]);
                at    = end + 1u;
                continue;
            }
        }

        _out[used++] = byte;
        at++;
    }

    return used;
}


/*
d_msvs_value
  An attribute's value, decoded as XML decodes one.
*/
size_t
d_msvs_value(
    const struct d_msvs* _msvs,
    size_t               _start,
    size_t               _length,
    char*                _out
)
{
    // a value needs a file and a place to go
    if ( (!_msvs) ||
         (!_out) )
    {
        return 0u;
    }

    const size_t length = d_internal_decode(&_msvs->text[_start], _length,
                                            true, false, _out);

    _out[length] = '\0';

    return length;
}


/*
d_msvs_text
  The text of an element holding no element: its character data decoded,
its CDATA as it stands but for its line endings, its comments left out.
*/
size_t
d_msvs_text(
    const struct d_msvs*         _msvs,
    const struct d_msvs_element* _element,
    char*                        _out
)
{
    // text needs a file, an element and a place to go
    if ( (!_msvs)    ||
         (!_element) ||
         (!_out) )
    {
        return 0u;
    }

    struct d_xml       xml;
    struct d_xml_token token;
    size_t             used = 0;

    d_xml_begin(&xml, &_msvs->text[_element->content_start],
                _element->content_end - _element->content_start);

    while ( (_element->child_count == 0u) &&
            (d_xml_next(&xml, &token)) )
    {
        // text is decoded, and CDATA taken as it stands
        if (token.kind != D_XML_TEXT)
        {
            continue;
        }

        used += d_internal_decode(token.text, token.text_length, false,
                                  token.cdata, &_out[used]);
    }

    _out[used] = '\0';

    return used;
}


/*
d_msvs_child
  The first child so named.
*/
size_t
d_msvs_child(
    const struct d_msvs* _msvs,
    size_t               _element,
    const char*          _name
)
{
    // an element must be there to have children
    if ( (!_msvs) ||
         (_element >= _msvs->element_count) )
    {
        return D_MSVS_NONE;
    }

    for (size_t child = _msvs->elements[_element].first_child;
         child != D_MSVS_NONE;
         child = _msvs->elements[child].next_sibling)
    {
        // the first so named is the one
        if (d_msvs_is(_msvs, &_msvs->elements[child], _name))
        {
            return child;
        }
    }

    return D_MSVS_NONE;
}


/*
d_msvs_is_name
  XML's Name production, as the reader holds a tag's name to it.
*/
bool
d_msvs_is_name(
    const char* _text,
    size_t      _length
)
{
    return ( (_text) &&
             (d_internal_name_sound(_text, _length)) );
}


//==============================================================================
// 7.  CONDITIONS
//==============================================================================


/*
d_internal_quote_at
  Reads a single-quoted string at `*_at`, past any space, moving past it.
*/
static bool
d_internal_quote_at(
    const char* _text,
    size_t*     _at,
    size_t*     _start,
    size_t*     _length
)
{
    size_t at = *_at;

    while ( (_text[at] == ' ') ||
            (_text[at] == '\t') )
    {
        at++;
    }

    // a quote opens it
    if (_text[at] != '\'')
    {
        return false;
    }

    const size_t start = ++at;

    while ( (_text[at] != '\0') &&
            (_text[at] != '\'') )
    {
        at++;
    }

    // and closes it
    if (_text[at] != '\'')
    {
        return false;
    }

    *_start  = start;
    *_length = at - start;
    *_at     = at + 1u;

    return true;
}


/*
d_msvs_condition_of
  'A'=='B', one side the configuration's properties and the other plain.
*/
enum d_msvs_condition
d_msvs_condition_of(
    const char* _condition,
    char*       _out_tested,
    size_t      _capacity
)
{
    // no condition always holds
    if (!_condition)
    {
        return D_MSVS_ALWAYS;
    }

    size_t at = 0;

    while ( (_condition[at] == ' ')  || (_condition[at] == '\t') ||
            (_condition[at] == '\r') || (_condition[at] == '\n') )
    {
        at++;
    }

    // a condition of space holds as none does
    if (_condition[at] == '\0')
    {
        return D_MSVS_ALWAYS;
    }

    size_t left_start  = 0;
    size_t left_length = 0;
    size_t right_start = 0;
    size_t right_length = 0;

    // 'left' == 'right', and nothing else
    if (!d_internal_quote_at(_condition, &at, &left_start, &left_length))
    {
        return D_MSVS_UNKNOWN;
    }

    while ( (_condition[at] == ' ') ||
            (_condition[at] == '\t') )
    {
        at++;
    }

    if ( (_condition[at] != '=') ||
         (_condition[at + 1u] != '=') )
    {
        return D_MSVS_UNKNOWN;
    }

    at += 2u;

    if (!d_internal_quote_at(_condition, &at, &right_start, &right_length))
    {
        return D_MSVS_UNKNOWN;
    }

    while ( (_condition[at] == ' ')  || (_condition[at] == '\t') ||
            (_condition[at] == '\r') || (_condition[at] == '\n') )
    {
        at++;
    }

    if (_condition[at] != '\0')
    {
        return D_MSVS_UNKNOWN;
    }

    // d_internal_side
    //   struct: what one side of the test may be, and what it tests.
    struct d_internal_side
    {
        const char* text;
        const char* before;
        const char* after;
    };

    static const struct d_internal_side sides[] =
    {
        { "$(Configuration)|$(Platform)", "",   ""   },
        { "$(Configuration)",             "",   "|*" },
        { "$(Platform)",                  "*|", ""   }
    };

    for (int swap = 0; swap < 2; ++swap)
    {
        const char* const   property = &_condition[(swap) ? right_start
                                                          : left_start];
        const size_t        property_length = (swap) ? right_length
                                                     : left_length;
        const char* const   value    = &_condition[(swap) ? left_start
                                                          : right_start];
        const size_t        value_length = (swap) ? left_length
                                                  : right_length;

        for (size_t which = 0; which < (sizeof(sides) / sizeof(sides[0]));
             ++which)
        {
            // one side the properties, the other no property at all
            if ( (!d_internal_same_text(property, property_length,
                                        sides[which].text,
                                        strlen(sides[which].text))) ||
                 (memchr(value, '$', value_length) != NULL) )
            {
                continue;
            }

            if ( (_out_tested) &&
                 (_capacity > 0u) )
            {
                (void)snprintf(_out_tested, _capacity, "%s%.*s%s",
                               sides[which].before, (int)value_length,
                               value, sides[which].after);
            }

            return D_MSVS_CONFIGURATION;
        }
    }

    return D_MSVS_UNKNOWN;
}


/*
d_msvs_config_matches
  Each half compared without case, `*` matching any; a configuration named
without a platform is any platform's.
*/
bool
d_msvs_config_matches(
    const char* _tested,
    const char* _config
)
{
    // both must be given
    if ( (!_tested) ||
         (!_config) )
    {
        return false;
    }

    const char* const tested_bar = strchr(_tested, '|');
    const char* const config_bar = strchr(_config, '|');
    const size_t      tested_configuration = (tested_bar)
                                           ? (size_t)(tested_bar - _tested)
                                           : strlen(_tested);
    const size_t      config_configuration = (config_bar)
                                           ? (size_t)(config_bar - _config)
                                           : strlen(_config);
    const char* const tested_platform = (tested_bar) ? (tested_bar + 1)
                                                     : "*";
    const char* const config_platform = (config_bar) ? (config_bar + 1)
                                                     : "*";

    const bool configuration =
        ( (tested_configuration == 1u) && (_tested[0] == '*') )   ||
        ( (config_configuration == 1u) && (_config[0] == '*') )   ||
        (d_internal_same_text(_tested, tested_configuration, _config,
                              config_configuration));
    const bool platform =
        (strcmp(tested_platform, "*") == 0)                       ||
        (strcmp(config_platform, "*") == 0)                       ||
        (d_internal_same_text(tested_platform, strlen(tested_platform),
                              config_platform, strlen(config_platform)));

    return ( (configuration) &&
             (platform) );
}


/*
d_msvs_compare_paths
  Byte by byte, each folded to lower case and `/` to `\`.
*/
int
d_msvs_compare_paths(
    const char* _left,
    const char* _right
)
{
    size_t at = 0;

    for (;;)
    {
        int left  = (unsigned char)_left[at];
        int right = (unsigned char)_right[at];

        left  = (left == '/') ? '\\' : left;
        right = (right == '/') ? '\\' : right;
        left  = ( (left >= 'A') && (left <= 'Z') ) ? (left - 'A' + 'a')
                                                   : left;
        right = ( (right >= 'A') && (right <= 'Z') ) ? (right - 'A' + 'a')
                                                     : right;

        // the first difference, or the end of both, decides
        if ( (left != right) ||
             (left == 0) )
        {
            return left - right;
        }

        at++;
    }
}


//==============================================================================
// 8.  WRITING
//==============================================================================


// d_internal_sha1
//   struct: SHA-1 as it runs: the state, the block being filled, and how
//   many bytes it has taken.
struct d_internal_sha1
{
    uint32_t state[5];
    uint8_t  block[64];
    size_t   filled;
    uint64_t total;
};


/*
d_internal_rotate
  A 32-bit word rotated left.
*/
static uint32_t
d_internal_rotate(
    uint32_t _word,
    unsigned _by
)
{
    return (uint32_t)((_word << _by) | (_word >> (32u - _by)));
}


/*
d_internal_sha1_block
  Takes one 64-byte block into the state: FIPS 180-4, 6.1.2.
*/
static void
d_internal_sha1_block(
    struct d_internal_sha1* _sha1
)
{
    uint32_t words[80];

    for (size_t at = 0; at < 16u; ++at)
    {
        words[at] = ((uint32_t)_sha1->block[(at * 4u)]      << 24) |
                    ((uint32_t)_sha1->block[(at * 4u) + 1u] << 16) |
                    ((uint32_t)_sha1->block[(at * 4u) + 2u] <<  8) |
                    ((uint32_t)_sha1->block[(at * 4u) + 3u]);
    }

    for (size_t at = 16u; at < 80u; ++at)
    {
        words[at] = d_internal_rotate(words[at - 3u] ^ words[at - 8u] ^
                                      words[at - 14u] ^ words[at - 16u], 1u);
    }

    uint32_t a = _sha1->state[0];
    uint32_t b = _sha1->state[1];
    uint32_t c = _sha1->state[2];
    uint32_t d = _sha1->state[3];
    uint32_t e = _sha1->state[4];

    for (size_t at = 0; at < 80u; ++at)
    {
        uint32_t mixed = 0;
        uint32_t added = 0;

        // each of the four rounds mixes the words its own way
        if (at < 20u)
        {
            mixed = (b & c) | ((~b) & d);
            added = 0x5A827999u;
        }
        else if (at < 40u)
        {
            mixed = b ^ c ^ d;
            added = 0x6ED9EBA1u;
        }
        else if (at < 60u)
        {
            mixed = (b & c) | (b & d) | (c & d);
            added = 0x8F1BBCDCu;
        }
        else
        {
            mixed = b ^ c ^ d;
            added = 0xCA62C1D6u;
        }

        const uint32_t next = d_internal_rotate(a, 5u) + mixed + e + added +
                              words[at];

        e = d;
        d = c;
        c = d_internal_rotate(b, 30u);
        b = a;
        a = next;
    }

    _sha1->state[0] += a;
    _sha1->state[1] += b;
    _sha1->state[2] += c;
    _sha1->state[3] += d;
    _sha1->state[4] += e;
    _sha1->filled    = 0;

    return;
}


/*
d_internal_sha1_take
  Takes bytes into the hash.
*/
static void
d_internal_sha1_take(
    struct d_internal_sha1* _sha1,
    const uint8_t*          _bytes,
    size_t                  _length
)
{
    for (size_t at = 0; at < _length; ++at)
    {
        _sha1->block[_sha1->filled++] = _bytes[at];

        // a full block is taken in
        if (_sha1->filled == sizeof(_sha1->block))
        {
            d_internal_sha1_block(_sha1);
        }
    }

    _sha1->total += _length;

    return;
}


/*
d_internal_sha1_finish
  Pads the message with its length and gives the 20-byte digest.
*/
static void
d_internal_sha1_finish(
    struct d_internal_sha1* _sha1,
    uint8_t*                _digest
)
{
    const uint64_t bits = _sha1->total * 8u;

    _sha1->block[_sha1->filled++] = 0x80u;

    // the length needs the last eight bytes of a block
    if (_sha1->filled > 56u)
    {
        while (_sha1->filled < sizeof(_sha1->block))
        {
            _sha1->block[_sha1->filled++] = 0u;
        }

        d_internal_sha1_block(_sha1);
    }

    while (_sha1->filled < 56u)
    {
        _sha1->block[_sha1->filled++] = 0u;
    }

    for (size_t at = 0; at < 8u; ++at)
    {
        _sha1->block[56u + at] = (uint8_t)(bits >> (56u - (at * 8u)));
    }

    d_internal_sha1_block(_sha1);

    for (size_t at = 0; at < 5u; ++at)
    {
        _digest[(at * 4u)]      = (uint8_t)(_sha1->state[at] >> 24);
        _digest[(at * 4u) + 1u] = (uint8_t)(_sha1->state[at] >> 16);
        _digest[(at * 4u) + 2u] = (uint8_t)(_sha1->state[at] >>  8);
        _digest[(at * 4u) + 3u] = (uint8_t)(_sha1->state[at]);
    }

    return;
}


/*
d_msvs_guid
  RFC 9562's version 5: SHA-1 over the namespace and the name, its first
sixteen bytes marked with the version and the variant.
*/
void
d_msvs_guid(
    const char* _seed,
    size_t      _length,
    char*       _out
)
{
    // djinterp's namespace: 0e0b6c4d-6a2f-5d8e-9b1c-7f3a2d6e4c58
    static const uint8_t space[16] =
    {
        0x0eu, 0x0bu, 0x6cu, 0x4du, 0x6au, 0x2fu, 0x5du, 0x8eu,
        0x9bu, 0x1cu, 0x7fu, 0x3au, 0x2du, 0x6eu, 0x4cu, 0x58u
    };
    static const char digits[] = "0123456789abcdef";
    struct d_internal_sha1 sha1;
    uint8_t                digest[20];

    memset(&sha1, 0, sizeof(sha1));
    sha1.state[0] = 0x67452301u;
    sha1.state[1] = 0xEFCDAB89u;
    sha1.state[2] = 0x98BADCFEu;
    sha1.state[3] = 0x10325476u;
    sha1.state[4] = 0xC3D2E1F0u;

    d_internal_sha1_take(&sha1, space, sizeof(space));
    d_internal_sha1_take(&sha1, (const uint8_t*)((_seed) ? _seed : ""),
                         (_seed) ? _length : 0u);
    d_internal_sha1_finish(&sha1, digest);

    digest[6] = (uint8_t)((digest[6] & 0x0Fu) | 0x50u);
    digest[8] = (uint8_t)((digest[8] & 0x3Fu) | 0x80u);

    size_t used = 0;

    for (size_t at = 0; at < 16u; ++at)
    {
        // the groups are 8, 4, 4, 4 and 12 digits
        if ( (at == 4u) || (at == 6u) || (at == 8u) || (at == 10u) )
        {
            _out[used++] = '-';
        }

        _out[used++] = digits[digest[at] >> 4];
        _out[used++] = digits[digest[at] & 0x0Fu];
    }

    _out[used] = '\0';

    return;
}


// d_internal_parts
//   struct: a path's parts, each a span of the path, with `.` dropped and
//   `..` taken back where it can be.
struct d_internal_parts
{
    const char* text;
    size_t*     starts;
    size_t*     lengths;
    size_t      count;
};


/*
d_internal_split
  Splits a path at `/` and `\`.
*/
static bool
d_internal_split(
    const char*              _path,
    struct d_internal_parts* _out
)
{
    const size_t length = strlen(_path);

    _out->text    = _path;
    _out->count   = 0;
    _out->starts  = malloc((length + 1u) * sizeof(size_t));
    _out->lengths = malloc((length + 1u) * sizeof(size_t));

    // the parts need room of their own
    if ( (!_out->starts) ||
         (!_out->lengths) )
    {
        free(_out->starts);
        free(_out->lengths);

        return false;
    }

    size_t from = 0;

    while (from <= length)
    {
        size_t to = from;

        while ( (to < length) &&
                (_path[to] != '/') &&
                (_path[to] != '\\') )
        {
            to++;
        }

        const size_t span    = to - from;
        const bool   dot     = (span == 1u) && (_path[from] == '.');
        const bool   up      = (span == 2u) && (_path[from] == '.') &&
                               (_path[from + 1u] == '.');
        const bool   back    = (up) && (_out->count > 0u) &&
                               !( (_out->lengths[_out->count - 1u] == 2u) &&
                                  (memcmp(&_path[_out->starts[_out->count
                                                              - 1u]],
                                          "..", 2u) == 0) );

        // an empty part or `.` is nothing; `..` takes back the part before
        if (back)
        {
            _out->count--;
        }
        else if ( (span > 0u) &&
                  (!dot) )
        {
            _out->starts[_out->count]  = from;
            _out->lengths[_out->count] = span;
            _out->count++;
        }

        from = to + 1u;
    }

    return true;
}


/*
d_msvs_relative
  Climbs from the directory to where it and the path part, then down the
rest of the path.
*/
char*
d_msvs_relative(
    const char* _directory,
    const char* _path,
    char        _separator
)
{
    struct d_internal_parts from;
    struct d_internal_parts to;

    // both must be given
    if ( (!_directory) ||
         (!_path) )
    {
        return NULL;
    }

    if (!d_internal_split(_directory, &from))
    {
        return NULL;
    }

    if (!d_internal_split(_path, &to))
    {
        free(from.starts);
        free(from.lengths);

        return NULL;
    }

    size_t common = 0;

    while ( (common < from.count) &&
            (common < to.count) &&
            (d_internal_same_text(&from.text[from.starts[common]],
                                  from.lengths[common],
                                  &to.text[to.starts[common]],
                                  to.lengths[common])) )
    {
        common++;
    }

    // a `..` of the directory past where they part cannot be climbed back
    bool sound = true;

    for (size_t at = common; at < from.count; ++at)
    {
        sound = sound &&
                !( (from.lengths[at] == 2u) &&
                   (memcmp(&from.text[from.starts[at]], "..", 2u) == 0) );
    }

    size_t length = 2u;

    for (size_t at = common; at < from.count; ++at)
    {
        length += 3u;
    }

    for (size_t at = common; at < to.count; ++at)
    {
        length += to.lengths[at] + 1u;
    }

    char* const out = (sound) ? malloc(length) : NULL;
    size_t      used = 0;

    for (size_t at = common; (out) && (at < from.count); ++at)
    {
        // each part of the directory past the common ones is climbed
        if (used > 0u)
        {
            out[used++] = _separator;
        }

        out[used++] = '.';
        out[used++] = '.';
    }

    for (size_t at = common; (out) && (at < to.count); ++at)
    {
        if (used > 0u)
        {
            out[used++] = _separator;
        }

        memcpy(&out[used], &to.text[to.starts[at]], to.lengths[at]);
        used += to.lengths[at];
    }

    // the directory itself is "."
    if ( (out) &&
         (used == 0u) )
    {
        out[used++] = '.';
    }

    if (out)
    {
        out[used] = '\0';
    }

    free(from.starts);
    free(from.lengths);
    free(to.starts);
    free(to.lengths);

    return out;
}


/*
d_msvs_escape
  The references XML predefines, for what would not read back as itself:
markup's two, the quote around a value, and character references for what
line-ending and value normalization would change.
*/
size_t
d_msvs_escape(
    const char* _text,
    size_t      _length,
    char        _quote,
    char*       _out
)
{
    // d_internal_reference
    //   struct: a byte, what stands for it, and whether only a value needs it.
    struct d_internal_reference
    {
        char        byte;
        const char* reference;
        bool        value_only;
    };

    static const struct d_internal_reference references[] =
    {
        { '&',  "&amp;",  false },
        { '<',  "&lt;",   false },
        { '>',  "&gt;",   false },
        { '\r', "&#13;",  false },
        { '\t', "&#9;",   true  },
        { '\n', "&#10;",  true  }
    };
    size_t used = 0;

    for (size_t at = 0; at < _length; ++at)
    {
        const char* reference = NULL;

        for (size_t which = 0;
             which < (sizeof(references) / sizeof(references[0])); ++which)
        {
            // a byte is escaped where it would not read back as itself
            if ( (references[which].byte == _text[at]) &&
                 ( (_quote != '\0') || (!references[which].value_only) ) )
            {
                reference = references[which].reference;
                break;
            }
        }

        // and the quote around a value, where it would end it
        if ( (_quote != '\0') &&
             (_text[at] == _quote) )
        {
            reference = (_quote == '"') ? "&quot;" : "&apos;";
        }

        // a byte with nothing to stand for it is itself
        if (!reference)
        {
            _out[used++] = _text[at];
            continue;
        }

        const size_t span = strlen(reference);

        memcpy(&_out[used], reference, span);
        used += span;
    }

    _out[used] = '\0';

    return used;
}
