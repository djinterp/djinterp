/*******************************************************************************
* vparse [cli]                                                        report.cpp
*
* Definitions for report.hpp.
*   The JSON here is written, never read, so it is made as text: a value is
* JSON text from the moment it is made, and a document is its members' texts
* joined. Pretty printing is a matter of where the line breaks go, and each
* value that spans lines is made knowing the indent it will sit at.
*
* path:      /src/djinterp/tools/vparse/cli/report.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.26
*******************************************************************************/
#include "./report.hpp"  // corresponding header
#if D_ENV_LANG_IS_CPP17_OR_HIGHER  // the floor the header has

// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::snprintf
#include <string>   // std::string, std::to_string
#include <utility>  // std::move, std::pair
#include <vector>   // std::vector
// vparse
#include "../../../../../inc/djinterp/tools/vparse/store.hpp"  // listing_text


namespace vparse
{
namespace cli
{

namespace
{

namespace wb = ::vparse::workbench;

// HEX_DIGITS
//   constant: the digits an escape writes a byte's value in: upper case, as
// the model's listings write `\xHH` (workbench::escape).
const char* const HEX_DIGITS = "0123456789ABCDEF";

/**
 * @brief Measures the UTF-8 sequence that starts at a position of a text.
 *
 * @param[in] _text  the text.
 * @param[in] _at    the position of the sequence's first byte, below the
 *                   text's length.
 * @return the sequence's length, 2 to 4, when the bytes there are one
 *         well-formed UTF-8 sequence of a character above U+007F; 0 when
 *         they are not.
 *
 * Well-formed is Unicode's table 3-7: no overlong form, no surrogate, nothing
 * above U+10FFFF. The second byte's range depends on the first, which is the
 * part a check of the lead byte and the continuation bits alone gets wrong.
 */
std::size_t utf8_sequence(
    const std::string& _text,
    std::size_t        _at
)
{
    const unsigned char lead = static_cast<unsigned char>(_text[_at]);

    // the lead byte decides the length and the second byte's range
    std::size_t   length = 0u;
    unsigned char low    = 0x80u;
    unsigned char high   = 0xBFu;

    // the sequence's length: C0, C1 and F5 to FF lead none
    if ( (lead >= 0xC2u) &&
         (lead <= 0xDFu) )
    {
        length = 2u;
    }
    else if ( (lead >= 0xE0u) &&
              (lead <= 0xEFu) )
    {
        length = 3u;
        low    = (lead == 0xE0u) ? 0xA0u : 0x80u;
        high   = (lead == 0xEDu) ? 0x9Fu : 0xBFu;
    }
    else if ( (lead >= 0xF0u) &&
              (lead <= 0xF4u) )
    {
        length = 4u;
        low    = (lead == 0xF0u) ? 0x90u : 0x80u;
        high   = (lead == 0xF4u) ? 0x8Fu : 0xBFu;
    }

    // a lead byte that starts no sequence, or a sequence the text cuts short
    if ( (length == 0u)                       ||
         ((_text.size() - _at) < length) )
    {
        return 0u;
    }

    // each byte after the lead is in its range
    for (std::size_t index = 1u; index < length; index++)
    {
        const unsigned char next  =
            static_cast<unsigned char>(_text[_at + index]);
        const unsigned char least = (index == 1u) ? low : 0x80u;
        const unsigned char most  = (index == 1u) ? high : 0xBFu;

        // out of range: an overlong form, a surrogate, or past U+10FFFF
        if ( (next < least) ||
             (next > most) )
        {
            return 0u;
        }
    }

    return length;
}

/**
 * @brief Writes one byte as the text `\xHH`, escaped for JSON.
 *
 * @param[in]     _byte  the byte.
 * @param[in,out] _out   receives `\\xHH`, appended.
 */
void append_byte_escape(
    unsigned char _byte,
    std::string&  _out
)
{
    _out += "\\\\x";
    _out += HEX_DIGITS[(_byte >> 4u) & 0x0Fu];
    _out += HEX_DIGITS[_byte & 0x0Fu];

    return;
}

/**
 * @brief Writes one byte below U+0080 as JSON writes it inside a string.
 *
 * @param[in]     _byte  the byte.
 * @param[in,out] _out   receives the byte, or its escape, appended.
 */
void append_ascii(
    unsigned char _byte,
    std::string&  _out
)
{
    switch (_byte)
    {
        case '"':  _out += "\\\""; break;
        case '\\': _out += "\\\\"; break;
        case '\n': _out += "\\n";  break;
        case '\r': _out += "\\r";  break;
        case '\t': _out += "\\t";  break;
        case '\b': _out += "\\b";  break;
        case '\f': _out += "\\f";  break;
        default:
        {
            // the other control characters have no short escape
            if (_byte < 0x20u)
            {
                _out += "\\u00";
                _out += HEX_DIGITS[(_byte >> 4u) & 0x0Fu];
                _out += HEX_DIGITS[_byte & 0x0Fu];
            }
            else
            {
                _out += static_cast<char>(_byte);
            }

            break;
        }
    }

    return;
}

/**
 * @brief Makes the run of spaces a line `_count` columns in starts with.
 *
 * @param[in] _count  how many spaces.
 * @return the spaces.
 */
std::string pad(
    std::size_t _count
)
{
    return std::string(_count,
                       ' ');
}

/**
 * @brief Joins JSON values into an object's or an array's text.
 *
 * @param[in] _lines   each member or element, as it is to be written.
 * @param[in] _open    `{` or `[`.
 * @param[in] _close   `}` or `]`.
 * @param[in] _indent  the column the closing bracket is written at.
 * @return the text, one value a line `_indent + 2` columns in; the two
 *         brackets alone when there are no values.
 */
std::string bracketed(
    const std::vector<std::string>& _lines,
    char                            _open,
    char                            _close,
    std::size_t                     _indent
)
{
    std::string out(1u,
                    _open);

    // an empty object or array closes on the line it opens
    if (_lines.empty())
    {
        out += _close;

        return out;
    }

    out += "\n";

    // a value a line, each but the last with its comma
    for (std::size_t index = 0u; index < _lines.size(); index++)
    {
        out += pad(_indent + 2u);
        out += _lines[index];
        out += ((index + 1u) < _lines.size()) ? ",\n" : "\n";
    }

    out += pad(_indent);
    out += _close;

    return out;
}

}  // anonymous namespace

/*
json_quote
  Byte by byte: below U+0080 through JSON's escapes, a well-formed UTF-8
sequence copied whole, and anything else -- a stray continuation byte, a
truncated sequence, an overlong form -- as the visible `\xHH`.
*/
std::string json_quote(
    const std::string& _text
)
{
    std::string out = "\"";

    out.reserve(_text.size() + 2u);

    // a character at a time: a byte, or a UTF-8 sequence whole
    for (std::size_t at = 0u; at < _text.size(); )
    {
        const unsigned char byte = static_cast<unsigned char>(_text[at]);

        // ASCII, escaped where JSON needs it
        if (byte < 0x80u)
        {
            append_ascii(byte,
                         out);
            at++;

            continue;
        }

        const std::size_t length = utf8_sequence(_text,
                                                  at);

        // a character beyond ASCII, copied as its bytes
        if (length != 0u)
        {
            out.append(_text,
                       at,
                       length);
            at += length;

            continue;
        }

        append_byte_escape(byte,
                           out);
        at++;
    }

    out += "\"";

    return out;
}

/*
json_strings
  One line, the elements separated by a comma and a space, so that a row of a
listing reads as the row it is.
*/
std::string json_strings(
    const std::vector<std::string>& _texts
)
{
    std::string out = "[";

    // each string quoted, separated by commas
    for (std::size_t index = 0u; index < _texts.size(); index++)
    {
        out += (index == 0u) ? "" : ", ";
        out += json_quote(_texts[index]);
    }

    out += "]";

    return out;
}

/*
json_object::add
  The value is kept as given: it is already JSON.
*/
json_object& json_object::add(
    const std::string& _key,
    const std::string& _json
)
{
    m_members.push_back(std::make_pair(_key,
                                       _json));

    return *this;
}

/*
json_object::add_all
  The other's members are JSON already, and are kept as they are, in their
order.
*/
json_object& json_object::add_all(
    const json_object& _other
)
{
    m_members.insert(m_members.end(),
                     _other.m_members.begin(),
                     _other.m_members.end());

    return *this;
}

/*
json_object::add_string
  Quoted here, so that a caller cannot forget to.
*/
json_object& json_object::add_string(
    const std::string& _key,
    const std::string& _text
)
{
    return add(_key,
               json_quote(_text));
}

/*
json_object::add_bool
  JSON's two literals.
*/
json_object& json_object::add_bool(
    const std::string& _key,
    bool               _value
)
{
    return add(_key,
               _value ? "true" : "false");
}

/*
json_object::add_count
  Decimal, as std::to_string writes it: a count needs no exponent and no sign.
*/
json_object& json_object::add_count(
    const std::string& _key,
    std::size_t        _count
)
{
    return add(_key,
               std::to_string(_count));
}

/*
json_object::text
  Each member as `"key": value`; a value spanning lines brings its own
indentation, since it was made for this indent.
*/
std::string json_object::text(
    std::size_t _indent
) const
{
    std::vector<std::string> lines;

    lines.reserve(m_members.size());

    // a member a line, its key quoted
    for (const std::pair<std::string, std::string>& member : m_members)
    {
        lines.push_back(json_quote(member.first) + ": " + member.second);
    }

    return bracketed(lines,
                     '{',
                     '}',
                     _indent);
}

/*
json_object::line
  As text() lays the members out, on one line.
*/
std::string json_object::line() const
{
    std::string out = "{";

    // every member on the one line, separated by commas
    for (std::size_t index = 0u; index < m_members.size(); index++)
    {
        out += (index == 0u) ? "" : ", ";
        out += json_quote(m_members[index].first);
        out += ": ";
        out += m_members[index].second;
    }

    out += "}";

    return out;
}

/*
json_array::add
  The element is kept as given: it is already JSON.
*/
json_array& json_array::add(
    const std::string& _json
)
{
    m_elements.push_back(_json);

    return *this;
}

/*
json_array::text
  As an object's text, with elements in place of members.
*/
std::string json_array::text(
    std::size_t _indent
) const
{
    return bracketed(m_elements,
                     '[',
                     ']',
                     _indent);
}

/*
json_array::empty
  Whether nothing was added.
*/
bool json_array::empty() const
{
    return m_elements.empty();
}

/*
json_listing
  The marks and keys a listing also carries are the window's -- a row's tint,
what two runs are compared by -- and stay out: a program reading a part
wants its cells.
*/
std::string json_listing(
    const workbench::listing& _listing,
    const std::string&        _title,
    std::size_t               _indent
)
{
    json_array rows;

    // a row a line, each an array of its cells
    for (const std::vector<std::string>& row : _listing.rows)
    {
        rows.add(json_strings(row));
    }

    json_object part;

    part.add_string("title",
                    _title)
        .add("columns",
             json_strings(_listing.columns))
        .add("rows",
             rows.text(_indent + 2u))
        .add_string("note",
                    _listing.note);

    return part.text(_indent);
}

/*
parts_text
  One part prints exactly as a saved run's part file holds it, which is what
the workbench's --print printed; a title is added only where there is more
than one part to tell apart.
*/
std::string parts_text(
    const workbench::run_record&    _run,
    const std::vector<std::string>& _parts
)
{
    // one part: the listing alone
    if (_parts.size() == 1u)
    {
        return wb::listing_text(wb::part_listing(_run,
                                                 _parts[0]));
    }

    std::string out;

    // each part under its title, a blank line between two
    for (std::size_t index = 0u; index < _parts.size(); index++)
    {
        const std::string title = wb::part_title(_parts[index]);

        out += (index == 0u) ? "" : "\n";
        out += title;
        out += "\n";
        out += std::string(title.size(),
                           '-');
        out += "\n";
        out += wb::listing_text(wb::part_listing(_run,
                                                 _parts[index]));
    }

    return out;
}

/*
clipped
  Counted in bytes, and cut where a character begins: backing off past UTF-8
continuation bytes keeps a cut from splitting a character, which would leave
a byte no terminal can show.
*/
std::string clipped(
    const std::string& _text,
    std::size_t        _most
)
{
    // short enough to keep whole
    if (_text.size() <= _most)
    {
        return _text;
    }

    std::size_t cut = (_most > 3u) ? (_most - 3u) : 0u;

    // back off to the first byte of a character
    while ( (cut > 0u)                                                  &&
            ((static_cast<unsigned char>(_text[cut]) & 0xC0u) == 0x80u) )
    {
        cut--;
    }

    return _text.substr(0u,
                        cut) + "...";
}

}  // namespace cli
}  // namespace vparse

#endif  // floor, for now
