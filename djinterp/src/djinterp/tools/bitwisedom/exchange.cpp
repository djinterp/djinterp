/*******************************************************************************
* bitwisedom [bitwisedom]                                           exchange.cpp
*
* A bitfield as JSON for other programs, and from them.
*   Reading a bitfield is two passes. The first makes every row, in order,
* as the file gives them -- a group's rows inside it, a range's values after
* it -- and keeps, for each value that names its range, the name. The
* second resolves the sheet, so that every row has its macro name, and
* finds each such name as the expressions find theirs.
*   Writing one leaves out what the defaults say, so a file written is a
* file a person could have written: a bit is {"name": "EN", "bit": 0}.
*
* path:      /src/djinterp/tools/bitwisedom/exchange.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.11
*******************************************************************************/
#include <djinterp/tools/bitwisedom/exchange.hpp>  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <algorithm>         // std::min, std::max
#include <cstddef>           // std::size_t
#include <initializer_list>  // std::initializer_list
#include <utility>           // std::move
// bitwisedom
#include <djinterp/tools/bitwisedom/json.hpp>     // json_value, parse_json
#include <djinterp/tools/bitwisedom/persist.hpp>  // sheet_object, read_enum


namespace bitwisedom
{

namespace
{

// ------------------------------------------------------------------ helpers

// has
//   whether an object has a member.
bool
has(
    const json_value&  _object,
    const std::string& _key
)
{
    return (_object.find(_key) != nullptr);
}

// text_of
//   a member that is a string, or the fallback.
std::string
text_of(
    const json_value&  _object,
    const std::string& _key,
    const std::string& _fallback
)
{
    const json_value* found = _object.find(_key);

    return ( (found != nullptr) &&
             (found->type == json_type::string) ) ? found->text
                                                  : _fallback;
}

// bits_text
//   a span as a bitfield writes it: lowest..highest.
std::string
bits_text(
    int _lsb,
    int _width
)
{
    return format_dec(static_cast<u64>(_lsb)) + ".." +
           format_dec(static_cast<u64>(_lsb + _width - 1));
}

// integer_value
//   a number as JSON: past 2^53, which a reader in doubles would round, as
// a string of its digits, which the readers here take as well.
json_value
integer_value(
    i64 _number
)
{
    const i64 exact = (i64(1) << 53);

    if ( (_number > exact) ||
         (_number < -exact) )
    {
        return json_value::string(format_signed(_number));
    }

    return json_value::number(_number);
}

// level_word
//   a language level as the command line names it: c99, cpp17.
std::string
level_word(
    standard _level
)
{
    std::string name = standard_name(_level);

    for (char& each : name)
    {
        each = (each == '+') ? 'p'
                             : static_cast<char>(each | 0x20);
    }

    return name;
}

// ---------------------------------------------------------------- reading

/*
bitfield_reader
  One bitfield being read into a sheet. Every method that can fail sets the
error, with the path of what it was reading, and returns false; reading
stops there.
*/
class bitfield_reader
{
public:
    bitfield_reader(
        const json_value& _in,
        json_read&        _out
    )
        : m_in(_in),
          m_out(_out)
    {
    }

    bool run()
    {
        const json_value* width = m_in.find("width");
        i64               bits  = 0;

        if ( (width == nullptr) ||
             (!integer(*width, bits)) ||
             (bits < 1) ||
             (bits > MAX_WIDTH) )
        {
            return fail("width",
                        "a bitfield's width is a number of bits, 1 to " +
                            format_dec(static_cast<u64>(MAX_WIDTH)));
        }

        m_sheet = make_sheet(text_of(m_in, "title", "untitled"),
                             static_cast<int>(bits));

        unknown_keys(m_in, std::string(),
                     { "format", "version", "title", "width", "prefix",
                       "suffix", "trim_both", "trim_left", "trim_right",
                       "numbering", "msb_left", "byte_order",
                       "probe", "environment", "definitions", "rows",
                       "segments",
                       "generator", "note" });

        m_sheet.prefix      = text_of(m_in, "prefix", "");
        m_sheet.suffix      = text_of(m_in, "suffix", "");
        m_sheet.trim_both   = text_of(m_in, "trim_both", "");
        m_sheet.trim_left   = text_of(m_in, "trim_left", "");
        m_sheet.trim_right  = text_of(m_in, "trim_right", "");
        m_sheet.environment = text_of(m_in, "environment", "");
        if (const json_value* files = m_in.find("definitions"))
        {
            if (files->type == json_type::array)
            {
                for (const json_value& entry : files->items)
                {
                    if (entry.type != json_type::object) continue;
                    definition_source source;
                    source.path = text_of(entry, "path", "");
                    source.text = text_of(entry, "text", "");
                    source.show_other = false;
                    m_sheet.definitions.push_back(std::move(source));
                }
            }
        }

        if ( (!word(m_in, "numbering", std::string(), m_sheet.bits))  ||
             (!word(m_in, "byte_order", std::string(), m_sheet.bytes)) ||
             (!flag(m_in, "msb_left", std::string(), m_sheet.msb_left)) )
        {
            return false;
        }

        if (has(m_in, "probe"))
        {
            bool negative = false;

            if (!number(*m_in.find("probe"), "probe", m_sheet.probe,
                        negative))
            {
                return false;
            }
        }

        read_generator(m_in.find("generator"), m_sheet.gen);

        const json_value* rows = m_in.find("rows");

        if ( (rows != nullptr) &&
             (!read_rows(*rows, 0, "rows")) )
        {
            return false;
        }

        if ( (!read_segments()) ||
             (!place_values()) )
        {
            return false;
        }

        m_out.result = std::move(m_sheet);
        m_out.ok     = true;

        return true;
    }

private:
    // pending
    //   struct: a value whose range is named, to be found once every row
    // is made.
    struct pending
    {
        int         id = 0;
        std::string field;
        std::string where;
    };

    const json_value&    m_in;
    json_read&           m_out;
    sheet                m_sheet;
    std::vector<pending> m_pending;

    // fail
    //   the error, with the path it was found at.
    bool fail(
        const std::string& _where,
        const std::string& _what
    )
    {
        m_out.error = _where.empty() ? _what
                                     : _where + ": " + _what;

        return false;
    }

    // unknown_keys
    //   a note for each member a thing does not have.
    void unknown_keys(
        const json_value&                  _object,
        const std::string&                 _where,
        std::initializer_list<const char*> _known
    )
    {
        for (const json_member& each : _object.members)
        {
            bool known = false;

            for (const char* name : _known)
            {
                known = ( (known) || (each.key == name) );
            }

            if (!known)
            {
                m_out.notes.push_back(
                    (_where.empty() ? std::string() : _where + ": ") + "\"" +
                    each.key + "\" is not a key " +
                    (_where.empty() ? "a bitfield" : "it") +
                    " has, and was passed over");
            }
        }

        return;
    }

    // integer
    //   a JSON number, or a string C would read as an integer.
    static bool integer(
        const json_value& _value,
        i64&              _out
    )
    {
        if (_value.type == json_type::number)
        {
            // an integer, not 1.5 or 1e3
            if (_value.text.find_first_of(".eE") != std::string::npos)
            {
                return false;
            }

            const parsed_integer parsed = parse_integer(_value.text);

            _out = static_cast<i64>(parsed.value);

            return parsed.ok;
        }

        if (_value.type == json_type::string)
        {
            const parsed_integer parsed = parse_integer(_value.text);

            _out = static_cast<i64>(parsed.value);

            return parsed.ok;
        }

        return false;
    }

    // number
    //   a value of any width: a JSON integer, or a string in any base;
    // negative is set for a negative one.
    bool number(
        const json_value&  _value,
        const std::string& _where,
        wide&              _out,
        bool&              _negative
    )
    {
        std::string text = _value.text;

        if ( (_value.type != json_type::number) &&
             (_value.type != json_type::string) )
        {
            return fail(_where, "a number, or a string such as \"0x1F\"");
        }

        if ( (_value.type == json_type::number) &&
             (text.find_first_of(".eE") != std::string::npos) )
        {
            return fail(_where, text + " is not an integer");
        }

        const parsed_wide parsed = parse_wide(text);

        if (!parsed.ok)
        {
            return fail(_where, "\"" + text + "\" is not a number: " +
                                    parsed.error);
        }

        _out      = parsed.value;
        _negative = parsed.negative;

        return true;
    }

    // small
    //   a member that is a non-negative count or position, if there.
    bool small(
        const json_value&  _object,
        const char*        _key,
        const std::string& _where,
        int&               _out
    )
    {
        const json_value* found = _object.find(_key);
        i64               value = 0;

        if (found == nullptr)
        {
            return true;
        }

        if ( (!integer(*found, value)) ||
             (value < 0) ||
             (value > MAX_WIDTH) )
        {
            return fail(_where, std::string("\"") + _key + "\" is a "
                        "number from 0 to " +
                        format_dec(static_cast<u64>(MAX_WIDTH)));
        }

        _out = static_cast<int>(value);

        return true;
    }

    // flag
    //   a member that is true or false, if there.
    bool flag(
        const json_value&  _object,
        const char*        _key,
        const std::string& _where,
        bool&              _out
    )
    {
        const json_value* found = _object.find(_key);

        if (found == nullptr)
        {
            return true;
        }

        if (found->type != json_type::boolean)
        {
            return fail(_where, std::string("\"") + _key +
                                    "\" is true or false");
        }

        _out = found->flag;

        return true;
    }

    // word
    //   a member that is one of an enumeration's words, if there.
    template<typename Enum>
    bool word(
        const json_value&  _object,
        const char*        _key,
        const std::string& _where,
        Enum&              _out
    )
    {
        const json_value* found = _object.find(_key);

        if (found == nullptr)
        {
            return true;
        }

        if ( (found->type != json_type::string) ||
             (!read_enum(found->text, _out)) )
        {
            return fail(_where, std::string("\"") + _key + "\" is not one "
                        "of its words: \"" + found->text + "\"");
        }

        return true;
    }

    // span
    //   "bits": "4..6", "4", or [4, 6] -- either way round -- as a lowest
    // bit and a width.
    bool span(
        const json_value&  _value,
        const std::string& _where,
        int&               _lsb,
        int&               _width
    )
    {
        i64 first = 0;
        i64 last  = 0;

        if ( (_value.type == json_type::array) &&
             (_value.items.size() == 2) &&
             (integer(_value.items[0], first)) &&
             (integer(_value.items[1], last)) )
        {
            // [low, high], or [high, low]
        }
        else if (_value.type == json_type::string)
        {
            const std::size_t dots = _value.text.find("..");
            const std::string low  = _value.text.substr(0, dots);
            const std::string high = (dots == std::string::npos)
                                         ? low
                                         : _value.text.substr(dots + 2);

            if ( (!integer(json_value::string(low), first)) ||
                 (!integer(json_value::string(high), last)) )
            {
                return fail(_where, "\"bits\" is \"lowest..highest\", "
                            "such as \"4..6\": not \"" + _value.text + "\"");
            }
        }
        else
        {
            return fail(_where, "\"bits\" is \"lowest..highest\", such as "
                        "\"4..6\", or [4, 6]");
        }

        if ( (first < 0) ||
             (last < 0) ||
             (first >= MAX_WIDTH) ||
             (last >= MAX_WIDTH) )
        {
            return fail(_where, "\"bits\" are from 0 to " +
                        format_dec(static_cast<u64>(MAX_WIDTH - 1)));
        }

        _lsb   = static_cast<int>(std::min(first, last));
        _width = static_cast<int>(std::max(first, last) -
                                  std::min(first, last) + 1);

        return true;
    }

    // common
    //   the keys every row may have.
    bool common(
        const json_value&  _object,
        const std::string& _where,
        row&               _row
    )
    {
        _row.note       = text_of(_object, "note", "");
        _row.short_name = text_of(_object, "short", "");
        _row.prefix     = text_of(_object, "prefix", "");
        _row.suffix     = text_of(_object, "suffix", "");

        const std::string colour = text_of(_object, "colour", "");

        if ( (!colour.empty()) &&
             (!parse_rgba(colour, _row.colour)) )
        {
            return fail(_where, "\"colour\" is \"#RRGGBB\" or "
                        "\"#RRGGBBAA\": not \"" + colour + "\"");
        }

        return ( (flag(_object, "transparent", _where,
                        _row.transparent)) &&
                 (flag(_object, "emit", _where, _row.emit))         &&
                 (word(_object, "radix", _where, _row.fmt))         &&
                 (word(_object, "int_suffix", _where, _row.suf))    &&
                 (small(_object, "digits", _where, _row.digits)) );
    }

    // read_rows
    //   an array of rows, into a group.
    bool read_rows(
        const json_value&  _rows,
        int                _parent,
        const std::string& _where
    )
    {
        if (_rows.type != json_type::array)
        {
            return fail(_where, "\"rows\" is an array of rows");
        }

        for (std::size_t i = 0; i < _rows.items.size(); ++i)
        {
            const std::string where = _where + "[" + format_dec(i) + "]";

            if (!read_row(_rows.items[i], _parent, where))
            {
                return false;
            }
        }

        return true;
    }

    bool read_row(
        const json_value&  _entry,
        int                _parent,
        const std::string& _where
    )
    {
        if (_entry.type != json_type::object)
        {
            return fail(_where, "a row is an object: {\"name\": ...} or "
                        "{\"group\": ...}");
        }

        if (has(_entry, "group"))
        {
            return read_group(_entry, _parent, _where);
        }

        return read_symbol(_entry, _parent, _where);
    }

    bool read_group(
        const json_value&  _entry,
        int                _parent,
        const std::string& _where
    )
    {
        row made;
        made.kind = row_kind::group;
        made.name = text_of(_entry, "group", "");

        const std::string where = _where + " (" + made.name + ")";

        unknown_keys(_entry, where,
                     { "group", "note", "short", "prefix", "suffix",
                       "colour", "emit", "radix", "int_suffix", "digits",
                       "exclusive", "isolate", "name_children",
                       "collapsed", "rows", "transparent" });

        if ( (!common(_entry, where, made))                          ||
             (!flag(_entry, "exclusive", where, made.exclusive))      ||
             (!flag(_entry, "isolate", where, made.isolate))          ||
             (!flag(_entry, "name_children", where, made.name_children)) ||
             (!flag(_entry, "collapsed", where, made.collapsed)) )
        {
            return false;
        }

        const int         id   = insert_row(m_sheet, _parent, -1, made);
        const json_value* rows = _entry.find("rows");

        return ( (rows == nullptr) ||
                 (read_rows(*rows, id, _where + ".rows")) );
    }

    bool read_symbol(
        const json_value&  _entry,
        int                _parent,
        const std::string& _where
    )
    {
        row made;
        made.kind = row_kind::symbol;
        made.name = text_of(_entry, "name", "");

        if (made.name.empty())
        {
            return fail(_where, "a symbol has a \"name\"; a group, "
                        "\"group\"");
        }

        const std::string where = _where + " (" + made.name + ")";

        unknown_keys(_entry, where,
                     { "name", "note", "short", "prefix", "suffix",
                       "colour", "emit", "radix", "int_suffix", "digits",
                       "bit", "bits", "lsb", "width", "values", "field",
                       "number", "encoding", "bias", "value", "expr",
                       "body", "params", "callable", "rows",
                       "exclusive", "isolate", "name_children",
                       "collapsed", "transparent" });

        if (!common(_entry, where, made))
        {
            return false;
        }

        // one definition: a value's span is its own, not a range's
        const bool        number  = has(_entry, "number");
        const bool        spanned = ( (has(_entry, "bits")) ||
                                      (has(_entry, "lsb")) );
        std::vector<std::string> kinds;

        for (const char* key : { "bit", "value", "expr", "body" })
        {
            if (has(_entry, key))
            {
                kinds.emplace_back(key);
            }
        }

        if (number)
        {
            kinds.emplace_back("number");
        }
        else if (spanned)
        {
            kinds.emplace_back(has(_entry, "bits") ? "bits" : "lsb");
        }

        if (kinds.empty())
        {
            return fail(where, "no definition: give it \"bit\", \"bits\", "
                        "\"number\" (a value in a range), \"value\" (a "
                        "literal), \"expr\" or \"body\" (a macro)");
        }

        if (kinds.size() > 1)
        {
            return fail(where, "two definitions, \"" + kinds[0] +
                               "\" and \"" + kinds[1] + "\": a row has one");
        }

        if ( (has(_entry, "values")) &&
             ( (number) || (!spanned) ) )
        {
            return fail(where, "\"values\" belong to a range: a row with "
                        "\"bits\", or \"lsb\" and \"width\"");
        }

        const std::string& kind = kinds.front();

        if (kind == "bit")
        {
            made.def = def_kind::bit;

            if (!small(_entry, "bit", where, made.bit))
            {
                return false;
            }
        }
        else if ( (kind == "bits") ||
                  (kind == "lsb") ||
                  (kind == "number") )
        {
            made.def = number ? def_kind::field_value : def_kind::range;

            if (!read_span(_entry, where, number, made))
            {
                return false;
            }
        }
        else if (kind == "value")
        {
            made.def = def_kind::literal;

            if (!number_literal(_entry, where, made))
            {
                return false;
            }
        }
        else if (kind == "expr")
        {
            made.def  = def_kind::expression;
            made.expr = text_of(_entry, "expr", "");
        }
        else
        {
            made.def    = def_kind::macro;
            made.body   = text_of(_entry, "body", "");
            made.params = text_of(_entry, "params", "");

            if (!flag(_entry, "callable", where, made.callable))
            {
                return false;
            }
        }

        if ((made.def == def_kind::range) &&
            ((!flag(_entry, "name_children", where, made.name_children)) ||
             (!flag(_entry, "exclusive", where, made.exclusive)) ||
             (!flag(_entry, "isolate", where, made.isolate)) ||
             (!flag(_entry, "collapsed", where, made.collapsed))))
        {
            return false;
        }

        const int id = insert_row(m_sheet, _parent, -1, made);
        if ((has(_entry, "rows")) && (made.def != def_kind::range))
            return fail(where, "nested rows require a range or group");

        if ( (made.def == def_kind::field_value) &&
             (has(_entry, "field")) )
        {
            m_pending.push_back({ id, text_of(_entry, "field", ""), where });
        }

        const json_value* values = _entry.find("values");

        if ((values != nullptr) &&
            (!read_values(*values, _parent, id, where + ".values")))
            return false;
        const json_value* children = _entry.find("rows");
        return ((children == nullptr) ||
                read_rows(*children, id, where + ".rows"));
    }

    // read_span
    //   a range's bits, or a value's: its range by name, or its own bits;
    // and a value's number, encoding and bias.
    bool read_span(
        const json_value&  _entry,
        const std::string& _where,
        bool               _value,
        row&               _row
    )
    {
        if ( (_value) &&
             (has(_entry, "field")) &&
             ( (has(_entry, "bits")) || (has(_entry, "lsb")) ) )
        {
            return fail(_where, "a value names its range (\"field\") or "
                        "gives its own bits, not both");
        }

        if ( (_value) &&
             (!has(_entry, "field")) &&
             (!has(_entry, "bits")) &&
             (!has(_entry, "lsb")) )
        {
            return fail(_where, "a value names its range (\"field\") or "
                        "gives its own bits (\"bits\")");
        }

        if (has(_entry, "bits"))
        {
            if ( (has(_entry, "lsb")) ||
                 (has(_entry, "width")) )
            {
                return fail(_where, "\"bits\", or \"lsb\" and \"width\": "
                            "not both");
            }

            if (!span(*_entry.find("bits"), _where, _row.lsb, _row.width))
            {
                return false;
            }
        }
        else if (has(_entry, "lsb"))
        {
            _row.width = 1;

            if ( (!small(_entry, "lsb", _where, _row.lsb)) ||
                 (!small(_entry, "width", _where, _row.width)) )
            {
                return false;
            }
        }

        if (!_value)
        {
            return true;
        }

        return read_number(_entry, _where, _row);
    }

    // read_number
    //   a value's number, its encoding and its bias.
    bool read_number(
        const json_value&  _entry,
        const std::string& _where,
        row&               _row
    )
    {
        i64 value = 0;

        if (!integer(*_entry.find("number"), value))
        {
            return fail(_where, "\"number\" is an integer, or a string "
                        "such as \"-0x10\"");
        }

        _row.number = value;

        if (!word(_entry, "encoding", _where, _row.enc))
        {
            return false;
        }

        const json_value* bias = _entry.find("bias");

        if ( (bias != nullptr) &&
             (!integer(*bias, _row.bias)) )
        {
            return fail(_where, "\"bias\" is an integer");
        }

        return true;
    }

    // number_literal
    //   a literal's value: a number, which may be negative, of any width.
    bool number_literal(
        const json_value&  _entry,
        const std::string& _where,
        row&               _row
    )
    {
        bool negative = false;

        if (!number(*_entry.find("value"), _where, _row.literal, negative))
        {
            return false;
        }

        _row.negative = negative;

        return true;
    }

    // read_values
    //   a range's named values, each a row after it: a number, or an
    // object with its number and the rest.
    bool read_values(
        const json_value&  _values,
        int                _parent,
        int                _field,
        const std::string& _where
    )
    {
        if (_values.type != json_type::object)
        {
            return fail(_where, "\"values\" is an object: {\"NAME\": 2}");
        }

        for (const json_member& each : _values.members)
        {
            const std::string where = _where + "." + each.key;
            row               made;

            made.kind  = row_kind::symbol;
            made.def   = def_kind::field_value;
            made.name  = each.key;
            made.field = _field;

            if (each.value.type == json_type::object)
            {
                unknown_keys(each.value, where,
                             { "number", "encoding", "bias", "note",
                               "short", "prefix", "suffix", "colour",
                               "emit", "radix", "int_suffix", "digits",
                               "transparent" });

                if (!has(each.value, "number"))
                {
                    return fail(where, "a value's object has its "
                                "\"number\"");
                }

                if ( (!common(each.value, where, made)) ||
                     (!read_number(each.value, where, made)) )
                {
                    return false;
                }
            }
            else if (!integer(each.value, made.number))
            {
                return fail(where, "a value is a number, or an object "
                            "with its \"number\"");
            }

            insert_row(m_sheet, _parent, -1, made);
        }

        return true;
    }

    bool read_segments()
    {
        const json_value* segments = m_in.find("segments");

        if (segments == nullptr)
        {
            return true;
        }

        if (segments->type != json_type::array)
        {
            return fail("segments", "an array of {\"name\", \"bits\"}");
        }

        for (std::size_t i = 0; i < segments->items.size(); ++i)
        {
            const json_value& each  = segments->items[i];
            const std::string where = "segments[" + format_dec(i) + "]";
            column_segment    made;
            int               lsb   = 0;
            int               width = 1;

            if ( (each.type != json_type::object) ||
                 (!has(each, "bits")) )
            {
                return fail(where, "a segment is {\"name\": ..., "
                            "\"bits\": \"lowest..highest\"}");
            }

            unknown_keys(each, where, { "name", "bits", "collapsed",
                                        "colour" });

            if ( (!span(*each.find("bits"), where, lsb, width)) ||
                 (!flag(each, "collapsed", where, made.collapsed)) )
            {
                return false;
            }

            const std::string colour = text_of(each, "colour", "");

            if ( (!colour.empty()) &&
                 (!parse_rgba(colour, made.colour)) )
            {
                return fail(where, "\"colour\" is \"#RRGGBBAA\"");
            }

            made.name = text_of(each, "name", "");
            made.lsb  = lsb;
            made.msb  = lsb + width - 1;
            m_sheet.segments.push_back(std::move(made));
        }

        // segments given are the sheet's own, not made every n bits
        m_sheet.grid.auto_segments = 0;

        return true;
    }

    // place_values
    //   the second pass: each value that names its range finds it, as an
    // expression finds a name.
    bool place_values()
    {
        if (m_pending.empty())
        {
            return true;
        }

        const resolution resolved = resolve(m_sheet);

        for (const pending& each : m_pending)
        {
            std::string error;
            const int   found = find_name(resolved, each.field, each.id,
                                          error);
            const row*  field = find_row(m_sheet, found);

            if ( (found == 0) ||
                 (field == nullptr) )
            {
                return fail(each.where, "its \"field\", " + each.field +
                            ", names no row: " + error);
            }

            if ( (field->kind != row_kind::symbol) ||
                 ( (field->def != def_kind::range) &&
                   (field->def != def_kind::bit) ) )
            {
                return fail(each.where, "its \"field\", " + each.field +
                            ", is not a range");
            }

            find_row(m_sheet, each.id)->field = found;
        }

        return true;
    }
};

// ---------------------------------------------------------------- writing

// differences
//   the members of an object that a default one does not have the same.
json_value
differences(
    const json_value& _object,
    const json_value& _default
)
{
    json_value out = json_value::object();

    for (const json_member& each : _object.members)
    {
        const json_value* was = _default.find(each.key);

        if ( (was == nullptr) ||
             (write_json(*was) != write_json(each.value)) )
        {
            out.set(each.key, each.value);
        }
    }

    return out;
}

/*
bitfield_writer
  A sheet as a bitfield: each row in as few keys as say what it is. A range
takes the values that follow it in its group as its "values"; a value
anywhere else names its range by its macro name.
*/
class bitfield_writer
{
public:
    explicit bitfield_writer(
        const sheet& _sheet
    )
        : m_sheet(_sheet),
          m_res(resolve(_sheet))
    {
    }

    std::string run()
    {
        json_value out = json_value::object();
        sheet      fresh = make_sheet(m_sheet.title, m_sheet.width);

        out.set("format", json_value::string(BITFIELD_FORMAT));
        out.set("version", json_value::number(BITFIELD_VERSION));
        out.set("title", json_value::string(m_sheet.title));
        out.set("width", json_value::number(m_sheet.width));

        maybe(out, "prefix", m_sheet.prefix);
        maybe(out, "suffix", m_sheet.suffix);
        maybe(out, "trim_both", m_sheet.trim_both);
        maybe(out, "trim_left", m_sheet.trim_left);
        maybe(out, "trim_right", m_sheet.trim_right);

        if (m_sheet.bits != fresh.bits)
        {
            out.set("numbering", json_value::string(enum_word(m_sheet.bits)));
        }

        if (m_sheet.msb_left != fresh.msb_left)
        {
            out.set("msb_left", json_value::boolean(m_sheet.msb_left));
        }

        if (m_sheet.bytes != fresh.bytes)
        {
            out.set("byte_order",
                    json_value::string(enum_word(m_sheet.bytes)));
        }

        if (m_sheet.probe.any())
        {
            out.set("probe", json_value::string(
                                 "0x" + format_hex(m_sheet.probe, 0, true)));
        }

        maybe(out, "environment", m_sheet.environment);
        if (!m_sheet.definitions.empty())
        {
            json_value files = json_value::array();
            for (const definition_source& source : m_sheet.definitions)
            {
                json_value entry = json_value::object();
                entry.set("path", json_value::string(source.path));
                entry.set("text", json_value::string(source.text));
                files.items.push_back(std::move(entry));
            }
            out.set("definitions", std::move(files));
        }
        out.set("rows", rows(m_sheet.root));

        if (!m_sheet.segments.empty())
        {
            json_value segments = json_value::array();

            for (const column_segment& each : m_sheet.segments)
            {
                json_value made = json_value::object();

                made.set("name", json_value::string(each.name));
                made.set("bits", json_value::string(
                                     bits_text(each.lsb,
                                               each.msb - each.lsb + 1)));

                if (each.collapsed)
                {
                    made.set("collapsed", json_value::boolean(true));
                }

                if (each.colour != 0)
                {
                    made.set("colour",
                             json_value::string(rgba_text(each.colour)));
                }

                segments.push(std::move(made));
            }

            out.set("segments", std::move(segments));
        }

        // the generator's settings a new sheet does not have
        const json_value generator =
            differences(generator_object(m_sheet.gen),
                        generator_object(fresh.gen));

        if (!generator.members.empty())
        {
            out.set("generator", generator);
        }

        return write_json(out);
    }

private:
    const sheet&     m_sheet;
    const resolution m_res;

    // maybe
    //   a string member, when it is not empty.
    static void maybe(
        json_value&        _object,
        const char*        _key,
        const std::string& _text
    )
    {
        if (!_text.empty())
        {
            _object.set(_key, json_value::string(_text));
        }

        return;
    }

    // common
    //   the keys every row may have, where they are not the defaults.
    static void common(
        json_value& _object,
        const row&  _row
    )
    {
        maybe(_object, "note", _row.note);
        maybe(_object, "short", _row.short_name);
        maybe(_object, "prefix", _row.prefix);
        maybe(_object, "suffix", _row.suffix);

        if (_row.colour != 0)
        {
            _object.set("colour", json_value::string(rgba_text(_row.colour)));
        }
        if (_row.transparent)
        {
            _object.set("transparent", json_value::boolean(true));
        }

        if (!_row.emit)
        {
            _object.set("emit", json_value::boolean(false));
        }

        if (_row.fmt != radix::inherit)
        {
            _object.set("radix", json_value::string(enum_word(_row.fmt)));
        }

        if (_row.suf != int_suffix::inherit)
        {
            _object.set("int_suffix", json_value::string(enum_word(_row.suf)));
        }

        if (_row.digits != 0)
        {
            _object.set("digits", json_value::number(_row.digits));
        }

        return;
    }

    // value_entry
    //   a value as its range's "values" hold it: its number alone, where
    // nothing else is said of it.
    static json_value value_entry(
        const row& _row
    )
    {
        json_value out = json_value::object();

        out.set("number", integer_value(_row.number));

        if (_row.enc != encoding::unsigned_binary)
        {
            out.set("encoding", json_value::string(enum_word(_row.enc)));
        }

        if (_row.bias != 0)
        {
            out.set("bias", json_value::number(_row.bias));
        }

        common(out, _row);

        return (out.members.size() == 1) ? integer_value(_row.number)
                                         : out;
    }

    json_value rows(
        const row& _group
    )
    {
        json_value  out      = json_value::array();
        const auto& children = _group.children;

        for (std::size_t i = 0; i < children.size(); ++i)
        {
            const row& each = children[i];

            if (each.kind == row_kind::group)
            {
                json_value made = json_value::object();

                made.set("group", json_value::string(each.name));
                common(made, each);

                if (!each.exclusive)
                {
                    made.set("exclusive", json_value::boolean(false));
                }

                if (each.name_children)
                {
                    made.set("name_children", json_value::boolean(true));
                }

                if (each.isolate)
                {
                    made.set("isolate", json_value::boolean(true));
                }

                if (each.collapsed)
                {
                    made.set("collapsed", json_value::boolean(true));
                }

                made.set("rows", rows(each));
                out.push(std::move(made));

                continue;
            }

            json_value made = json_value::object();

            made.set("name", json_value::string(each.name));

            switch (each.def)
            {
                case def_kind::bit:
                    made.set("bit", json_value::number(each.bit));
                    break;

                case def_kind::range:
                    made.set("bits", json_value::string(
                                         bits_text(each.lsb, each.width)));
                    break;

                case def_kind::field_value:
                {
                    const resolved_row* field = m_res.find(each.field);

                    if (field != nullptr)
                    {
                        made.set("field", json_value::string(field->full));
                    }
                    else
                    {
                        made.set("bits", json_value::string(
                                             bits_text(each.lsb,
                                                       each.width)));
                    }

                    made.set("number", integer_value(each.number));

                    if (each.enc != encoding::unsigned_binary)
                    {
                        made.set("encoding",
                                 json_value::string(enum_word(each.enc)));
                    }

                    if (each.bias != 0)
                    {
                        made.set("bias", json_value::number(each.bias));
                    }

                    break;
                }

                case def_kind::literal:
                    made.set("value",
                             each.negative
                                 ? integer_value(static_cast<i64>(
                                       each.literal.low()))
                                 : json_value::string(
                                       "0x" + format_hex(each.literal, 0,
                                                         true)));
                    break;

                case def_kind::expression:
                    made.set("expr", json_value::string(each.expr));
                    break;

                case def_kind::macro:
                    maybe(made, "params", each.params);
                    made.set("body", json_value::string(each.body));

                    if (each.callable)
                    {
                        made.set("callable", json_value::boolean(true));
                    }

                    break;
            }

            common(made, each);

            // A range can also own rows, nested to arbitrary depth.
            if (can_contain(each))
            {
                if (each.name_children)
                    made.set("name_children", json_value::boolean(true));
                if (!each.exclusive)
                    made.set("exclusive", json_value::boolean(false));
                if (each.isolate)
                    made.set("isolate", json_value::boolean(true));
                if (each.collapsed)
                    made.set("collapsed", json_value::boolean(true));
                if (!each.children.empty())
                    made.set("rows", rows(each));
            }

            // a range takes the values that follow it as its own
            if (each.def == def_kind::range)
            {
                json_value values = json_value::object();

                while ( (i + 1 < children.size()) &&
                        (children[i + 1].kind == row_kind::symbol) &&
                        (children[i + 1].def == def_kind::field_value) &&
                        (children[i + 1].field == each.id) &&
                        (values.find(children[i + 1].name) == nullptr) )
                {
                    ++i;
                    values.set(children[i].name, value_entry(children[i]));
                }

                if (!values.members.empty())
                {
                    made.set("values", std::move(values));
                }
            }

            out.push(std::move(made));
        }

        return out;
    }
};

// ---------------------------------------------------------------- export

/*
symbol_object
  One row as it resolves: what it is, where it sits, its bits and value,
the integer C knows it by, and what is wrong with it.
*/
json_value
symbol_object(
    const sheet&                    _sheet,
    const resolved_row&             _row,
    const row&                      _source,
    const std::vector<std::string>& _path
)
{
    json_value out  = json_value::object();
    json_value path = json_value::array();

    for (const std::string& each : _path)
    {
        path.push(json_value::string(each));
    }

    out.set("id", json_value::number(_row.id));

    if (_row.kind == row_kind::group)
    {
        out.set("kind", json_value::string("group"));
        out.set("name", json_value::string(_source.name));
        out.set("path", std::move(path));

        if (!_source.note.empty())
        {
            out.set("note", json_value::string(_source.note));
        }

        out.set("bits", json_value::string(runs_text(_row.bits, 1000000)));

        return out;
    }

    const char* const kinds[] = { "bit", "range", "value", "literal",
                                  "expression", "macro" };

    out.set("kind", json_value::string(kinds[static_cast<int>(_row.def)]));
    out.set("name", json_value::string(_source.name));
    out.set("macro", json_value::string(_row.full));
    out.set("path", std::move(path));

    if (!_source.note.empty())
    {
        out.set("note", json_value::string(_source.note));
    }

    if (!_source.emit)
    {
        out.set("emit", json_value::boolean(false));
    }

    if (_row.span_lsb >= 0)
    {
        out.set("lsb", json_value::number(_row.span_lsb));
        out.set("msb", json_value::number(_row.span_lsb + _row.span_width -
                                          1));
        out.set("width", json_value::number(_row.span_width));
    }

    if (_row.has_value)
    {
        out.set("bits", json_value::string(runs_text(_row.bits, 1000000)));
        out.set("value", json_value::string(
                             "0x" + format_hex(_row.bits, 0, true)));

        // the integer C knows it by
        if (is_wide(_sheet.width))
        {
            const c_constant known = wide_constant(_row);

            if (known.ok)
            {
                out.set("integer", json_value::string(
                                       known.negative
                                           ? format_signed(static_cast<i64>(
                                                 known.value))
                                           : format_dec(known.value)));
            }
        }
        else
        {
            out.set("integer", json_value::string(
                                   _row.negative
                                       ? format_signed(sign_extend(
                                             _row.value, _sheet.width))
                                       : format_dec(_row.value)));
        }
    }

    switch (_row.def)
    {
        case def_kind::field_value:
            out.set("number", integer_value(_row.number));
            out.set("encoding", json_value::string(enum_word(_source.enc)));

            if (_source.bias != 0)
            {
                out.set("bias", json_value::number(_source.bias));
            }

            break;

        case def_kind::literal:
            if (_source.negative)
            {
                out.set("negative", json_value::boolean(true));
            }

            break;

        case def_kind::expression:
            out.set("expr", json_value::string(_source.expr));
            break;

        case def_kind::macro:
            if (!_source.params.empty())
            {
                out.set("params", json_value::string(_source.params));
            }

            out.set("body", json_value::string(_source.body));

            if (!_row.ctype.empty())
            {
                out.set("ctype", json_value::string(_row.ctype));
            }

            if (!_row.expansion.empty())
            {
                out.set("expansion", json_value::string(_row.expansion));
            }

            break;

        default:
            break;
    }

    if (!_row.error.empty())
    {
        out.set("error", json_value::string(_row.error));
    }

    return out;
}

}  // anonymous namespace


json_read
read_json_sheet(
    std::string_view _text
)
{
    json_read  out;
    json_value in;

    if (!parse_json(_text, in, out.error))
    {
        return out;
    }

    if (in.type != json_type::object)
    {
        out.error = "the JSON is not an object: a sheet, a bitfield or an "
                    "export is one";

        return out;
    }

    const std::string format  = text_of(in, "format", "");
    const json_value* version = in.find("version");
    const i64         newest  = (format == EXPORT_FORMAT) ? EXPORT_VERSION
                                                          : BITFIELD_VERSION;

    if (format == SHEET_FORMAT)
    {
        out.shape = json_shape::sheet;
        out.ok    = sheet_from_object(in, out.result, out.error);

        return out;
    }

    if ( (!format.empty()) &&
         (format != EXPORT_FORMAT) &&
         (format != BITFIELD_FORMAT) )
    {
        out.error = "\"" + format + "\" is not a format this reads: a "
                    "sheet, a bitfield or an export";

        return out;
    }

    if ( (version != nullptr) &&
         (version->as_int(0) > newest) )
    {
        out.error = format + " version " +
                    format_signed(version->as_int(0)) + ": this build reads "
                    "up to version " + format_signed(newest);

        return out;
    }

    if (format == EXPORT_FORMAT)
    {
        const json_value* held = in.find("sheet");

        out.shape = json_shape::everything;

        if (held == nullptr)
        {
            out.error = "an export with no \"sheet\"";

            return out;
        }

        out.ok = sheet_from_object(*held, out.result, out.error);

        return out;
    }

    out.shape = json_shape::bitfield;

    bitfield_reader reader(in, out);

    reader.run();

    return out;
}

std::string
write_bitfield(
    const sheet& _sheet
)
{
    bitfield_writer writer(_sheet);

    return writer.run();
}

std::string
write_export(
    const sheet&      _sheet,
    const resolution& _resolution,
    const generated&  _header
)
{
    json_value out    = json_value::object();
    json_value layout = json_value::object();

    out.set("format", json_value::string(EXPORT_FORMAT));
    out.set("version", json_value::number(EXPORT_VERSION));
    out.set("title", json_value::string(_sheet.title));

    // the layout: how the bits are counted, and a wide sheet's words
    layout.set("width", json_value::number(_sheet.width));
    layout.set("numbering", json_value::string(enum_word(_sheet.bits)));
    layout.set("msb_left", json_value::boolean(_sheet.msb_left));
    layout.set("byte_order", json_value::string(enum_word(_sheet.bytes)));

    if (is_wide(_sheet.width))
    {
        json_value words = json_value::object();
        const int  word  = word_bits(_sheet);

        words.set("bits", json_value::number(word));
        words.set("count", json_value::number((_sheet.width + word - 1) /
                                              word));
        words.set("macros", json_value::string(bitset_prefix(_sheet)));
        layout.set("words", std::move(words));
    }

    out.set("layout", std::move(layout));

    // every row, in the tree's order, with the groups it is in
    json_value               symbols = json_value::array();
    std::vector<std::string> path;

    walk(_sheet,
         [&](const row& _row, const row&, int _depth)
         {
             const resolved_row* resolved = _resolution.find(_row.id);

             path.resize(static_cast<std::size_t>(_depth));

             if (resolved != nullptr)
             {
                 symbols.push(symbol_object(_sheet, *resolved, _row, path));
             }

             if (can_contain(_row))
             {
                 path.push_back(_row.name);
             }
         });

    out.set("symbols", std::move(symbols));

    json_value issues = json_value::array();

    for (const issue& each : _resolution.issues)
    {
        const resolved_row* where = _resolution.find(each.row);
        json_value          made  = json_value::object();

        made.set("level", json_value::string(
                              (each.level == severity::error)   ? "error"
                            : (each.level == severity::warning) ? "warning"
                                                                : "note"));
        made.set("symbol", (where != nullptr)
                               ? json_value::string(where->full)
                               : json_value());
        made.set("message", json_value::string(each.message));
        issues.push(std::move(made));
    }

    out.set("issues", std::move(issues));

    json_value header = json_value::object();
    json_value notes  = json_value::array();

    for (const std::string& each : _header.notes)
    {
        notes.push(json_value::string(each));
    }

    header.set("file_name", json_value::string(_sheet.gen.file_name));
    header.set("level", json_value::string(level_word(_sheet.gen.level)));
    header.set("form", json_value::string(
                           (_sheet.gen.form == gen_form::constants)
                               ? "constants" : "macros"));
    header.set("macros", json_value::number(_header.macros));
    header.set("skipped", json_value::number(_header.skipped));
    header.set("notes", std::move(notes));
    header.set("text", json_value::string(_header.text));
    out.set("header", std::move(header));

    out.set("sheet", sheet_object(_sheet));

    return write_json(out);
}


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
