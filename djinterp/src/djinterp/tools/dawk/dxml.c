/*******************************************************************************
* djinterp [tools]                                                        dxml.c
*
* Definitions for the declarations in dxml.h.
*   A tag is found by its `<` and ended by the first `>` outside a quoted
* attribute value, which is the one rule that keeps `>` inside a value from
* ending the tag early. Everything between two tags is one text token, white
* space included: a reader that does not want it skips it.
*
* path:      /src/djinterp/tools/dawk/dxml.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dxml.h"  // corresponding header
// std
#include <string.h>  // memcmp, memset, strlen


/*
d_internal_is_space
  XML's white space: space, tab, carriage return and line feed.
*/
static bool
d_internal_is_space(
    char _byte
)
{
    return ( (_byte == ' ')  ||
             (_byte == '\t') ||
             (_byte == '\r') ||
             (_byte == '\n') );
}


/*
d_internal_find
  Finds the first occurrence of `_needle` at or after `_from`.
*/
static size_t
d_internal_find(
    const struct d_xml* _xml,
    size_t              _from,
    const char*         _needle
)
{
    const size_t span = strlen(_needle);

    for (size_t at = _from; (at + span) <= _xml->length; ++at)
    {
        // the first match is the one wanted
        if (memcmp(&_xml->text[at], _needle, span) == 0)
        {
            return at;
        }
    }

    return _xml->length;
}


/*
d_internal_fail
  Marks the scan as failed at the cursor, for good.
*/
static bool
d_internal_fail(
    struct d_xml*       _xml,
    struct d_xml_token* _out
)
{
    _out->kind  = D_XML_ERROR;
    _out->start = _xml->at;
    _out->end   = _xml->at;
    _xml->at    = _xml->length + 1u;

    return false;
}


/*
d_internal_markup
  Reads a comment, a processing instruction, a declaration or a CDATA
section: the markup that begins `<!` or `<?`.
*/
static bool
d_internal_markup(
    struct d_xml*       _xml,
    struct d_xml_token* _out
)
{
    const char* const at = &_xml->text[_xml->at];
    const size_t      left = _xml->length - _xml->at;

    const char* close  = ">";
    size_t      skip   = 0;
    bool        cdata  = false;

    // each kind of markup ends with its own delimiter
    if ( (left >= 4u) &&
         (memcmp(at, "<!--", 4u) == 0) )
    {
        close = "-->";
        skip  = 4u;
    }
    else if ( (left >= 9u) &&
              (memcmp(at, "<![CDATA[", 9u) == 0) )
    {
        close = "]]>";
        skip  = 9u;
        cdata = true;
    }
    else if (at[1] == '?')
    {
        close = "?>";
        skip  = 2u;
    }

    const size_t end = d_internal_find(_xml, _xml->at + skip, close);

    // markup that never closes is not well formed
    if (end >= _xml->length)
    {
        return d_internal_fail(_xml, _out);
    }

    _out->kind  = cdata ? D_XML_TEXT : D_XML_OTHER;
    _out->start = _xml->at;
    _out->end   = end + strlen(close);
    _out->cdata = cdata;

    // a CDATA section's contents are its text, taken as they stand
    if (cdata)
    {
        _out->text        = &_xml->text[_xml->at + skip];
        _out->text_length = end - (_xml->at + skip);
    }

    _xml->at = _out->end;

    return true;
}


/*
d_internal_tag
  Reads a start, end or empty-element tag.
*/
static bool
d_internal_tag(
    struct d_xml*       _xml,
    struct d_xml_token* _out
)
{
    const bool closing = ( ((_xml->at + 1u) < _xml->length) &&
                           (_xml->text[_xml->at + 1u] == '/') );
    size_t     at      = _xml->at + (closing ? 2u : 1u);
    const size_t name  = at;

    // the name runs to white space, `/` or `>`
    while ( (at < _xml->length)                      &&
            (!d_internal_is_space(_xml->text[at]))   &&
            (_xml->text[at] != '/')                  &&
            (_xml->text[at] != '>') )
    {
        at++;
    }

    // a tag with no name is not well formed
    if (at == name)
    {
        return d_internal_fail(_xml, _out);
    }

    const size_t attributes = at;
    char         quote      = '\0';

    // the tag ends at the first `>` outside a quoted value
    while (at < _xml->length)
    {
        const char byte = _xml->text[at];

        // a quote opens a value, and the same quote closes it
        if (quote != '\0')
        {
            quote = (byte == quote) ? '\0' : quote;
        }
        else if ( (byte == '"') ||
                  (byte == '\'') )
        {
            quote = byte;
        }
        else if (byte == '>')
        {
            break;
        }

        at++;
    }

    // a tag that never closes is not well formed
    if (at >= _xml->length)
    {
        return d_internal_fail(_xml, _out);
    }

    const bool empty = ( (!closing) &&
                         (at > attributes) &&
                         (_xml->text[at - 1u] == '/') );

    _out->kind              = closing ? D_XML_CLOSE
                            : empty   ? D_XML_EMPTY
                            :           D_XML_START;
    _out->start             = _xml->at;
    _out->end               = at + 1u;
    _out->name              = &_xml->text[name];
    _out->name_length       = attributes - name;
    _out->attributes        = &_xml->text[attributes];
    _out->attributes_length = (at - attributes) - (empty ? 1u : 0u);

    _xml->at = at + 1u;

    return true;
}


/*
d_xml_begin
  The buffer is borrowed, and must stay as it is while it is scanned.
*/
void
d_xml_begin(
    struct d_xml* _xml,
    const char*   _text,
    size_t        _length
)
{
    _xml->text   = _text;
    _xml->length = (_text) ? _length : 0u;
    _xml->at     = 0;

    return;
}


/*
d_xml_next
  A scan that failed stays failed: the cursor is put past the end, where
every call reports D_XML_ERROR rather than D_XML_END.
*/
bool
d_xml_next(
    struct d_xml*       _xml,
    struct d_xml_token* _out
)
{
    memset(_out, 0, sizeof(*_out));

    // a failed scan keeps saying so
    if (_xml->at > _xml->length)
    {
        _out->kind = D_XML_ERROR;

        return false;
    }

    // the end of the buffer ends the scan
    if (_xml->at == _xml->length)
    {
        _out->kind  = D_XML_END;
        _out->start = _xml->length;
        _out->end   = _xml->length;

        return false;
    }

    // text runs to the next `<`
    if (_xml->text[_xml->at] != '<')
    {
        const size_t end = d_internal_find(_xml, _xml->at, "<");

        _out->kind        = D_XML_TEXT;
        _out->start       = _xml->at;
        _out->end         = end;
        _out->text        = &_xml->text[_xml->at];
        _out->text_length = end - _xml->at;
        _xml->at          = end;

        return true;
    }

    // a `<` at the very end opens nothing
    if ((_xml->at + 1u) >= _xml->length)
    {
        return d_internal_fail(_xml, _out);
    }

    const char next = _xml->text[_xml->at + 1u];

    return ( (next == '!') ||
             (next == '?') ) ? d_internal_markup(_xml, _out)
                             : d_internal_tag(_xml, _out);
}


/*
d_xml_is
  The local name is what follows the last `:` of the name, or all of it.
*/
bool
d_xml_is(
    const struct d_xml_token* _token,
    const char*               _local_name
)
{
    // only a tag has a name
    if ( (!_token)                                  ||
         (!_local_name)                             ||
         ( (_token->kind != D_XML_START) &&
           (_token->kind != D_XML_EMPTY) &&
           (_token->kind != D_XML_CLOSE) ) )
    {
        return false;
    }

    size_t local = 0;

    // the prefix ends at the last colon
    for (size_t at = 0; at < _token->name_length; ++at)
    {
        local = (_token->name[at] == ':') ? (at + 1u) : local;
    }

    const size_t span = strlen(_local_name);

    return ( ((_token->name_length - local) == span) &&
             (memcmp(&_token->name[local], _local_name, span) == 0) );
}


/*
d_xml_attribute
  Walks the attributes as name, `=`, quoted value; an attribute without a
quoted value ends the walk, since the rest cannot be read reliably.
*/
bool
d_xml_attribute(
    const struct d_xml_token* _token,
    const char*               _name,
    const char**              _out_value,
    size_t*                   _out_length
)
{
    // only a start or empty-element tag carries attributes
    if ( (!_token)                                  ||
         (!_name)                                   ||
         ( (_token->kind != D_XML_START) &&
           (_token->kind != D_XML_EMPTY) ) )
    {
        return false;
    }

    const char* const text   = _token->attributes;
    const size_t      length = _token->attributes_length;
    const size_t      span   = strlen(_name);
    size_t            at     = 0;

    while (at < length)
    {
        // white space separates attributes
        while ( (at < length) &&
                (d_internal_is_space(text[at])) )
        {
            at++;
        }

        const size_t name = at;

        // the name runs to `=` or white space
        while ( (at < length)                       &&
                (text[at] != '=')                   &&
                (!d_internal_is_space(text[at])) )
        {
            at++;
        }

        const size_t name_end = at;

        // white space may stand on either side of the `=`
        while ( (at < length) &&
                (d_internal_is_space(text[at])) )
        {
            at++;
        }

        // an attribute without `=` and a quoted value ends the walk
        if ( (at >= length)                         ||
             (text[at] != '=') )
        {
            return false;
        }

        at++;

        while ( (at < length) &&
                (d_internal_is_space(text[at])) )
        {
            at++;
        }

        // the value must be quoted
        if ( (at >= length)                          ||
             ( (text[at] != '"') && (text[at] != '\'') ) )
        {
            return false;
        }

        const char   quote = text[at++];
        const size_t value = at;

        // the value runs to the matching quote
        while ( (at < length) &&
                (text[at] != quote) )
        {
            at++;
        }

        // a value that is never closed ends the walk
        if (at >= length)
        {
            return false;
        }

        // the attribute wanted is reported where it is
        if ( ((name_end - name) == span) &&
             (memcmp(&text[name], _name, span) == 0) )
        {
            *_out_value  = &text[value];
            *_out_length = at - value;

            return true;
        }

        at++;
    }

    return false;
}


/*
d_internal_utf8
  Writes a code point as UTF-8, returning the bytes written; a value no
character has is written as nothing.
*/
static size_t
d_internal_utf8(
    unsigned long _code,
    char*         _out
)
{
    // one byte for ASCII
    if (_code < 0x80ul)
    {
        _out[0] = (char)_code;

        return 1u;
    }

    // two for the rest of the first 2048
    if (_code < 0x800ul)
    {
        _out[0] = (char)(0xC0ul | (_code >> 6));
        _out[1] = (char)(0x80ul | (_code & 0x3Ful));

        return 2u;
    }

    // three for the basic plane, surrogates excluded
    if (_code < 0x10000ul)
    {
        // a surrogate is no character
        if ( (_code >= 0xD800ul) &&
             (_code <= 0xDFFFul) )
        {
            return 0u;
        }

        _out[0] = (char)(0xE0ul | (_code >> 12));
        _out[1] = (char)(0x80ul | ((_code >> 6) & 0x3Ful));
        _out[2] = (char)(0x80ul | (_code & 0x3Ful));

        return 3u;
    }

    // four for the other planes, up to the last
    if (_code <= 0x10FFFFul)
    {
        _out[0] = (char)(0xF0ul | (_code >> 18));
        _out[1] = (char)(0x80ul | ((_code >> 12) & 0x3Ful));
        _out[2] = (char)(0x80ul | ((_code >> 6) & 0x3Ful));
        _out[3] = (char)(0x80ul | (_code & 0x3Ful));

        return 4u;
    }

    return 0u;
}


/*
d_internal_reference
  Decodes the reference at `_raw[0]`, a `&`, into `_out`.  Returns how many
raw bytes it took, or 0 when it is no reference this decoder knows.
*/
static size_t
d_internal_reference(
    const char* _raw,
    size_t      _length,
    char*       _out,
    size_t*     _out_written
)
{
    // d_internal_entity
    //   struct: a predefined entity, as written, and the byte it stands for.
    struct d_internal_entity
    {
        const char* name;
        char        byte;
    };

    static const struct d_internal_entity named[] =
    {
        { "&amp;", '&' }, { "&lt;", '<' }, { "&gt;", '>' },
        { "&quot;", '"' }, { "&apos;", '\'' }
    };

    for (size_t at = 0; at < (sizeof(named) / sizeof(named[0])); ++at)
    {
        const size_t span = strlen(named[at].name);

        // a predefined entity is its one byte
        if ( (span <= _length) &&
             (memcmp(_raw, named[at].name, span) == 0) )
        {
            _out[0]      = named[at].byte;
            *_out_written = 1u;

            return span;
        }
    }

    // anything else must be a character reference
    if ( (_length < 4u) ||
         (_raw[1] != '#') )
    {
        return 0u;
    }

    const bool    hex   = ( (_raw[2] == 'x') || (_raw[2] == 'X') );
    size_t        at    = hex ? 3u : 2u;
    unsigned long code  = 0;
    const size_t  first = at;

    // the digits run to the `;`, and a value past Unicode stops counting
    while ( (at < _length)  &&
            (_raw[at] != ';') &&
            (code <= 0x10FFFFul) )
    {
        const char digit = _raw[at];
        int        value = -1;

        // a digit of the reference's base, or the reference is no reference
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

        if (value < 0)
        {
            return 0u;
        }

        code = (code * (hex ? 16ul : 10ul)) + (unsigned long)value;
        at++;
    }

    // a reference must have digits and end with `;`
    if ( (at == first)        ||
         (at >= _length)      ||
         (_raw[at] != ';') )
    {
        return 0u;
    }

    *_out_written = d_internal_utf8(code, _out);

    // a reference to no character -- a surrogate, a value past Unicode -- is
    // no reference, and is copied as it stands
    if (*_out_written == 0u)
    {
        return 0u;
    }

    return at + 1u;
}


/*
d_xml_decode
  A reference is at least four bytes and its character at most four, so the
output never runs ahead of the input.
*/
size_t
d_xml_decode(
    const char* _raw,
    size_t      _length,
    char*       _out
)
{
    size_t used = 0;
    size_t at   = 0;

    while (at < _length)
    {
        size_t written = 0;
        size_t taken   = 0;

        // a reference is decoded; anything else is copied
        if (_raw[at] == '&')
        {
            taken = d_internal_reference(&_raw[at],
                                         _length - at,
                                         &_out[used],
                                         &written);
        }

        // a byte that begins no known reference is itself
        if (taken == 0)
        {
            _out[used++] = _raw[at++];
            continue;
        }

        used += written;
        at   += taken;
    }

    return used;
}
