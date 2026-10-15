/*******************************************************************************
* bitwisedom [bitwisedom]                                               json.cpp
*
* A small JSON value, reader and writer.
*   The reader is recursive descent with a depth limit, so a hostile file
* cannot exhaust the stack. The writer indents by four spaces and puts an
* array of scalars on one line, and any other array or object on one line
* where its line has room -- a bitfield's row is `{ "name": "EN", "bit": 0 }`
* -- which keeps a sheet's settings readable, and a bitfield of a thousand
* rows a thousand lines long.
*
* path:      /src/djinterp/tools/bitwisedom/json.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.11
*******************************************************************************/
#include <djinterp/tools/bitwisedom/json.hpp>  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstdio>   // std::snprintf
#include <cstdlib>  // std::strtod
#include <utility>  // std::move


namespace bitwisedom
{

namespace
{

// MAX_NESTING
//   constant: how deeply a file may nest arrays and objects.
const int MAX_NESTING = 128;

// reader
//   class: one parse.
class reader
{
public:
    explicit reader(
        std::string_view _text
    )
        : m_text(_text)
    {}

    bool run(
        json_value&  _value,
        std::string& _error
    )
    {
        skip();

        if (!value(_value, 0))
        {
            _error = where() + ": " + m_error;

            return false;
        }

        skip();

        if (m_at != m_text.size())
        {
            _error = where() + ": unexpected text after the value";

            return false;
        }

        return true;
    }

private:
    std::string_view m_text;
    std::size_t      m_at = 0;
    std::string      m_error;

    std::string where() const
    {
        std::size_t line   = 1;
        std::size_t column = 1;

        for (std::size_t i = 0; ( (i < m_at) && (i < m_text.size()) ); ++i)
        {
            if (m_text[i] == '\n')
            {
                ++line;
                column = 1;
            }
            else
            {
                ++column;
            }
        }

        return "line " + format_dec(line) + ", column " + format_dec(column);
    }

    bool fail(const std::string& _message)
    {
        m_error = _message;

        return false;
    }

    void skip()
    {
        while ( (m_at < m_text.size()) &&
                ( (m_text[m_at] == ' ')  ||
                  (m_text[m_at] == '\t') ||
                  (m_text[m_at] == '\n') ||
                  (m_text[m_at] == '\r') ) )
        {
            ++m_at;
        }

        return;
    }

    bool literal_word(const char* _word)
    {
        const std::string_view word(_word);

        if (m_text.substr(m_at, word.size()) != word)
        {
            return fail("expected '" + std::string(word) + "'");
        }

        m_at += word.size();

        return true;
    }

    bool value(
        json_value& _out,
        int         _depth
    )
    {
        if (_depth > MAX_NESTING)
        {
            return fail("the file nests too deeply");
        }

        if (m_at >= m_text.size())
        {
            return fail("the file ends where a value should be");
        }

        const char first = m_text[m_at];

        switch (first)
        {
            case '{': return object(_out, _depth);
            case '[': return array(_out, _depth);
            case '"':
                _out.type = json_type::string;
                return string(_out.text);
            case 't':
                _out = json_value::boolean(true);
                return literal_word("true");
            case 'f':
                _out = json_value::boolean(false);
                return literal_word("false");
            case 'n':
                _out = json_value{};
                return literal_word("null");
            default:
                return number(_out);
        }
    }

    bool number(json_value& _out)
    {
        const std::size_t start = m_at;

        while ( (m_at < m_text.size()) &&
                ( ( (m_text[m_at] >= '0') && (m_text[m_at] <= '9') ) ||
                  (m_text[m_at] == '-') ||
                  (m_text[m_at] == '+') ||
                  (m_text[m_at] == '.') ||
                  (m_text[m_at] == 'e') ||
                  (m_text[m_at] == 'E') ) )
        {
            ++m_at;
        }

        if (start == m_at)
        {
            return fail(std::string("unexpected character '") +
                        m_text[m_at] + "'");
        }

        _out.type = json_type::number;
        _out.text = std::string(m_text.substr(start, m_at - start));

        return true;
    }

    static void append_utf8(
        std::string&     _out,
        re_std::uint32_t _code
    )
    {
        if (_code < 0x80u)
        {
            _out += static_cast<char>(_code);
        }
        else if (_code < 0x800u)
        {
            _out += static_cast<char>(0xC0u | (_code >> 6));
            _out += static_cast<char>(0x80u | (_code & 0x3Fu));
        }
        else
        {
            _out += static_cast<char>(0xE0u | (_code >> 12));
            _out += static_cast<char>(0x80u | ((_code >> 6) & 0x3Fu));
            _out += static_cast<char>(0x80u | (_code & 0x3Fu));
        }

        return;
    }

    bool string(std::string& _out)
    {
        ++m_at;  // the opening quote
        _out.clear();

        while (m_at < m_text.size())
        {
            const char each = m_text[m_at++];

            if (each == '"')
            {
                return true;
            }

            if (each != '\\')
            {
                _out += each;

                continue;
            }

            if (m_at >= m_text.size())
            {
                break;
            }

            const char escaped = m_text[m_at++];

            switch (escaped)
            {
                case '"':  _out += '"';  break;
                case '\\': _out += '\\'; break;
                case '/':  _out += '/';  break;
                case 'b':  _out += '\b'; break;
                case 'f':  _out += '\f'; break;
                case 'n':  _out += '\n'; break;
                case 'r':  _out += '\r'; break;
                case 't':  _out += '\t'; break;
                case 'u':
                {
                    // four hexadecimal digits: one code unit
                    if (m_at + 4 > m_text.size())
                    {
                        return fail("a \\u escape needs four digits");
                    }

                    const parsed_integer code = parse_integer(
                        "0x" + std::string(m_text.substr(m_at, 4)));

                    if (!code.ok)
                    {
                        return fail("a bad \\u escape");
                    }

                    append_utf8(_out,
                                static_cast<re_std::uint32_t>(code.value));
                    m_at += 4;
                    break;
                }
                default:
                    return fail(std::string("unknown escape '\\") +
                                escaped + "'");
            }
        }

        return fail("a string is not closed");
    }

    bool array(
        json_value& _out,
        int         _depth
    )
    {
        _out = json_value::array();
        ++m_at;  // [
        skip();

        if ( (m_at < m_text.size()) &&
             (m_text[m_at] == ']') )
        {
            ++m_at;

            return true;
        }

        while (true)
        {
            json_value item;

            skip();

            if (!value(item, _depth + 1))
            {
                return false;
            }

            _out.items.push_back(std::move(item));
            skip();

            if (m_at >= m_text.size())
            {
                return fail("an array is not closed");
            }

            if (m_text[m_at] == ',')
            {
                ++m_at;

                continue;
            }

            if (m_text[m_at] == ']')
            {
                ++m_at;

                return true;
            }

            return fail("expected ',' or ']'");
        }
    }

    bool object(
        json_value& _out,
        int         _depth
    )
    {
        _out = json_value::object();
        ++m_at;  // {
        skip();

        if ( (m_at < m_text.size()) &&
             (m_text[m_at] == '}') )
        {
            ++m_at;

            return true;
        }

        while (true)
        {
            skip();

            if ( (m_at >= m_text.size()) ||
                 (m_text[m_at] != '"') )
            {
                return fail("expected a member name");
            }

            std::string key;

            if (!string(key))
            {
                return false;
            }

            skip();

            if ( (m_at >= m_text.size()) ||
                 (m_text[m_at] != ':') )
            {
                return fail("expected ':'");
            }

            ++m_at;
            skip();

            json_value member;

            if (!value(member, _depth + 1))
            {
                return false;
            }

            _out.members.push_back(json_member{ std::move(key),
                                                std::move(member) });
            skip();

            if (m_at >= m_text.size())
            {
                return fail("an object is not closed");
            }

            if (m_text[m_at] == ',')
            {
                ++m_at;

                continue;
            }

            if (m_text[m_at] == '}')
            {
                ++m_at;

                return true;
            }

            return fail("expected ',' or '}'");
        }
    }
};

// quoted
//   a string as a JSON string literal.
std::string
quoted(
    const std::string& _text
)
{
    std::string out = "\"";

    for (char each : _text)
    {
        switch (each)
        {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (static_cast<unsigned char>(each) < 0x20u)
                {
                    out += "\\u00" +
                           format_hex(static_cast<unsigned char>(each),
                                      2,
                                      false);
                }
                else
                {
                    out += each;
                }

                break;
        }
    }

    return out + "\"";
}

// LINE
//   constant: the columns a line of the writer's fills, past which an array
// or an object is laid out an item or a member to a line. JSON is not code:
// a bitfield's row reads best on its line, note and all, and a hundred
// columns hold most.
const std::size_t LINE = 100;

// scalar
//   whether a value prints on one line.
bool
scalar(
    const json_value& _value
)
{
    return ( (_value.type != json_type::array) &&
             (_value.type != json_type::object) );
}

// flat_array
//   whether a value is an array of scalars, which prints on one line
// however long it is.
bool
flat_array(
    const json_value& _value
)
{
    if (_value.type != json_type::array)
    {
        return false;
    }

    for (const json_value& item : _value.items)
    {
        if (!scalar(item))
        {
            return false;
        }
    }

    return true;
}

// one_line
//   appends a value, written on one line, to _out; false, and _out left
// part-written, once _out is longer than _room -- which stops it early in
// a value a thousand lines long.
bool
one_line(
    const json_value& _value,
    std::size_t       _room,
    std::string&      _out
)
{
    switch (_value.type)
    {
        case json_type::null:    _out += "null";                       break;
        case json_type::boolean: _out += _value.flag ? "true" : "false"; break;
        case json_type::number:  _out += _value.text;                  break;
        case json_type::string:  _out += quoted(_value.text);          break;

        case json_type::array:
        {
            _out += "[";

            for (std::size_t i = 0; i < _value.items.size(); ++i)
            {
                _out += (i == 0) ? "" : ", ";

                if (!one_line(_value.items[i], _room, _out))
                {
                    return false;
                }
            }

            _out += "]";

            break;
        }

        case json_type::object:
        {
            if (_value.members.empty())
            {
                _out += "{}";

                break;
            }

            _out += "{ ";

            for (std::size_t i = 0; i < _value.members.size(); ++i)
            {
                _out += (i == 0) ? "" : ", ";
                _out += quoted(_value.members[i].key) + ": ";

                if (!one_line(_value.members[i].value, _room, _out))
                {
                    return false;
                }
            }

            _out += " }";

            break;
        }
    }

    return (_out.size() <= _room);
}

// write_value
//   _value, at _indent, starting in _column: a scalar, or an array of
// scalars, on one line; any other array or object on one line where the
// line has room for it, with the comma that may follow; and where not, an
// item or a member to a line.
void
write_value(
    const json_value& _value,
    int               _indent,
    std::size_t       _column,
    std::string&      _out
)
{
    const std::string pad(static_cast<std::size_t>(_indent) * 4u, ' ');
    const std::string inner(static_cast<std::size_t>(_indent + 1) * 4u, ' ');

    if ( (scalar(_value)) ||
         (flat_array(_value)) )
    {
        (void)one_line(_value, static_cast<std::size_t>(-1), _out);

        return;
    }

    // the line, and the comma that may follow it
    std::string line;

    if ( (_column + 1u < LINE) &&
         (one_line(_value, LINE - _column - 1u, line)) )
    {
        _out += line;

        return;
    }

    if (_value.type == json_type::array)
    {
        _out += "[\n";

        for (std::size_t i = 0; i < _value.items.size(); ++i)
        {
            _out += inner;
            write_value(_value.items[i], _indent + 1, inner.size(), _out);
            _out += (i + 1 < _value.items.size()) ? ",\n" : "\n";
        }

        _out += pad + "]";

        return;
    }

    _out += "{\n";

    for (std::size_t i = 0; i < _value.members.size(); ++i)
    {
        const std::string key = quoted(_value.members[i].key) + ": ";

        _out += inner + key;
        write_value(_value.members[i].value,
                    _indent + 1,
                    inner.size() + key.size(),
                    _out);
        _out += (i + 1 < _value.members.size()) ? ",\n" : "\n";
    }

    _out += pad + "}";

    return;
}

}  // anonymous namespace


json_value
json_value::boolean(
    bool _flag
)
{
    json_value made;
    made.type = json_type::boolean;
    made.flag = _flag;

    return made;
}

json_value
json_value::number(
    i64 _number
)
{
    json_value made;
    made.type = json_type::number;
    made.text = format_signed(_number);

    return made;
}

json_value
json_value::real(
    double _number
)
{
    char buffer[64];

    std::snprintf(buffer, sizeof(buffer), "%.6g", _number);

    json_value made;
    made.type = json_type::number;
    made.text = buffer;

    return made;
}

json_value
json_value::string(
    const std::string& _text
)
{
    json_value made;
    made.type = json_type::string;
    made.text = _text;

    return made;
}

json_value
json_value::array()
{
    json_value made;
    made.type = json_type::array;

    return made;
}

json_value
json_value::object()
{
    json_value made;
    made.type = json_type::object;

    return made;
}

const json_value*
json_value::find(
    const std::string& _key
) const
{
    if (type != json_type::object)
    {
        return nullptr;
    }

    for (const json_member& each : members)
    {
        if (each.key == _key)
        {
            return &each.value;
        }
    }

    return nullptr;
}

void
json_value::set(
    const std::string& _key,
    json_value         _value
)
{
    type = json_type::object;

    for (json_member& each : members)
    {
        if (each.key == _key)
        {
            each.value = std::move(_value);

            return;
        }
    }

    members.push_back(json_member{ _key, std::move(_value) });

    return;
}

void
json_value::push(
    json_value _value
)
{
    type = json_type::array;
    items.push_back(std::move(_value));

    return;
}

bool
json_value::as_bool(
    bool _fallback
) const
{
    return (type == json_type::boolean) ? flag
                                        : _fallback;
}

i64
json_value::as_int(
    i64 _fallback
) const
{
    if (type != json_type::number)
    {
        return _fallback;
    }

    const parsed_integer parsed = parse_integer(text);

    return parsed.ok ? static_cast<i64>(parsed.value)
                     : _fallback;
}

double
json_value::as_real(
    double _fallback
) const
{
    if (type != json_type::number)
    {
        return _fallback;
    }

    char*        end    = nullptr;
    const double parsed = std::strtod(text.c_str(), &end);

    return (end != text.c_str()) ? parsed
                                 : _fallback;
}

std::string
json_value::as_string(
    const std::string& _fallback
) const
{
    return (type == json_type::string) ? text
                                       : _fallback;
}

bool
parse_json(
    std::string_view _text,
    json_value&      _value,
    std::string&     _error
)
{
    reader parse(_text);

    return parse.run(_value, _error);
}

std::string
write_json(
    const json_value& _value
)
{
    std::string out;

    write_value(_value, 0, 0, out);
    out += '\n';

    return out;
}


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
