/*******************************************************************************
* bitwisedom [bitwisedom]                                            codegen.cpp
*
* The C/C++ header generator.
*   The header is built as a list of lines. A first walk over the tree
* numbers the groups the way the style guide numbers a header's structure --
* top-level groups are sections, their groups subsections, theirs items --
* and that numbering serves both the table of contents and the section
* banners, so the two cannot disagree. A second walk writes each group's
* symbols: a brief, the macro, and the macros its templates make for it.
*   A macro is laid out as the style guide lays one out: on one line when
* it fits; otherwise its head alone and its body below, indented, every
* line but the last ending in a backslash at the 80th column. A body
* written over several lines keeps them, and their indentation; a head too
* long for its line spreads its parameters one to a line, aligned after its
* parenthesis.
*
* path:      /src/djinterp/tools/bitwisedom/codegen.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.11
*******************************************************************************/
#include <djinterp/tools/bitwisedom/codegen.hpp>  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <algorithm>  // std::find, std::max, std::min
#include <cctype>     // std::isalnum, std::isdigit, std::toupper
#include <cstddef>    // std::size_t
#include <utility>    // std::move
// bitwisedom
#include <djinterp/tools/bitwisedom/expr.hpp>  // parse_expression, print_c


namespace bitwisedom
{

namespace
{

// LINE
//   constant: the line length the style guide holds a header to.
const std::size_t LINE = 80;

// RULE_EQUALS / RULE_DASHES
//   constants: a section's and a subsection's delimiter lines.
const std::string RULE_EQUALS = "//" + std::string(78, '=');
const std::string RULE_DASHES = "//" + std::string(78, '-');

// heading
//   struct: one numbered group -- its number, its depth and its title.
struct heading
{
    int         id    = 0;
    int         depth = 0;
    std::string number;
    std::string title;
    std::string note;
};

// writer
//   class: one header being written.
class writer
{
public:
    writer(
        const sheet&            _sheet,
        const resolution&       _resolution,
        const std::string&      _date,
        generated&              _out,
        const std::vector<int>* _scope = nullptr
    )
        : m_sheet(_sheet),
          m_res(_resolution),
          m_gen(_sheet.gen),
          m_date(_date),
          m_out(_out),
          m_scope(_scope)
    {
        m_djinterp  = (m_gen.style == gen_style::djinterp);
        m_cpp       = is_cpp(m_gen.level);
        m_constants = ( (m_gen.form == gen_form::constants) &&
                        (m_cpp) );
        m_wide      = is_wide(m_sheet.width);
        m_word      = word_bits(m_sheet);
        m_bitset    = bitset_prefix(m_sheet);

        // constants are a C++ form; C keeps its macros
        if ( (m_gen.form == gen_form::constants) &&
             (!m_cpp) )
        {
            note("constants are a C++ form; a C header gets macros");
        }

        // C++98 has no 64-bit type of its own
        if ( (m_constants)                        &&
             (m_gen.level == standard::cpp98)     &&
             (m_sheet.width > 32)                 &&
             (!m_wide) )
        {
            note("C++98 has no long long; a constant wider than 32 bits is "
                 "a long, which holds it only where long has 64 bits "
                 "(LP64: not Windows, nor a 32-bit target)");
        }
    }

    void run()
    {
        const bool whole = (m_scope == nullptr);

        number_groups(m_sheet.root, 0, std::string());

        if ( (whole) &&
             (m_gen.banner) )
        {
            write_banner();
        }

        if ( (whole)               &&
             (m_djinterp)          &&
             (m_gen.sections)      &&
             (!m_headings.empty()) )
        {
            write_contents();
        }

        const std::string guard = m_gen.guard_name.empty()
                                      ? derive_guard(m_gen.file_name)
                                      : m_gen.guard_name;

        if ( (whole) &&
             (m_gen.guard) )
        {
            if (m_gen.pragma_once)
            {
                line("#pragma once");
            }
            else
            {
                line("#ifndef " + guard);
                line("#define " + guard + " 1");
            }

            blank();
        }

        if (whole)
        {
            write_preamble();
        }

        write_group(m_sheet.root, 0, whole);

        if ( (whole) &&
             (m_constants) &&
             (!m_gen.name_space.empty()) )
        {
            blank();
            line("}  // namespace " + m_gen.name_space);
        }

        // the file ends two empty lines after its last definition
        while ( (!m_lines.empty()) &&
                (m_lines.back().empty()) )
        {
            m_lines.pop_back();
        }

        if ( (whole) &&
             (m_gen.guard) &&
             (!m_gen.pragma_once) )
        {
            blank();
            blank();
            line("#endif  // " + guard);
        }

        for (const std::string& each : m_lines)
        {
            m_out.text += each;
            m_out.text += '\n';
        }

        return;
    }

private:
    const sheet&             m_sheet;
    const resolution&        m_res;
    const gen_settings&      m_gen;
    std::string              m_date;
    generated&               m_out;
    const std::vector<int>*  m_scope = nullptr;
    bool                     m_djinterp  = true;
    bool                     m_cpp       = false;
    bool                     m_constants = false;
    bool                     m_wide      = false;   // wider than an integer
    int                      m_word      = 32;      // its arrays' words
    std::string              m_bitset;              // its array macros'
                                                    // prefix
    std::vector<heading>     m_headings;
    std::vector<std::string> m_lines;
    std::vector<std::string> m_noted;

    // ---------------------------------------------------------- basics

    void line(const std::string& _text)
    {
        m_lines.push_back(_text);

        return;
    }

    // continued
    //   a line with its continuation backslash in the last column.
    static std::string continued(const std::string& _text)
    {
        const std::size_t pad = (_text.size() + 2 <= LINE)
                                    ? (LINE - 1 - _text.size())
                                    : 1;

        return _text + std::string(pad, ' ') + "\\";
    }

    // split_body
    //   a macro body cut, where it is too long for one line, at the places
    // a reader would break it: just after an operator between two spaces,
    // or after a comma, inside the body's parentheses or braces. Each piece
    // runs as far as it can; of the places that keep it within the room,
    // the one nearest the outside is taken, so "(A | B | C)" becomes "(A |
    // B |" and " C)", and an initializer breaks between its words.
    static std::vector<std::string> split_body(
        const std::string& _body,
        std::size_t        _room
    )
    {
        std::vector<std::string> pieces;

        if (_body.size() <= _room)
        {
            pieces.push_back(_body);

            return pieces;
        }

        // the places a piece may end: just past "op " or ", ", by depth
        std::vector<std::size_t> cuts;
        std::vector<int>         depths;
        int                      depth = 0;

        for (std::size_t i = 0; i < _body.size(); ++i)
        {
            const char each = _body[i];

            depth += ( (each == '(') || (each == '{') ) ? 1 : 0;
            depth -= ( (each == ')') || (each == '}') ) ? 1 : 0;

            const bool op    = ( (each == ' ')                  &&
                                 (i + 2 < _body.size())         &&
                                 (_body[i + 2] == ' ')          &&
                                 (std::string("|&^+*").find(_body[i + 1]) !=
                                  std::string::npos) );
            const bool comma = ( (each == ',')                  &&
                                 (i + 1 < _body.size())         &&
                                 (_body[i + 1] == ' ') );

            if ( (depth >= 1) &&
                 ( (op) || (comma) ) )
            {
                cuts.push_back(op ? (i + 2) : (i + 1));
                depths.push_back(depth);
            }
        }

        std::size_t start = 0;

        while (_body.size() - start > _room)
        {
            // of the cuts within the room, the shallowest, then the last
            std::size_t best       = 0;
            int         best_depth = 0;

            for (std::size_t c = 0; c < cuts.size(); ++c)
            {
                if ( (cuts[c] <= start) ||
                     (cuts[c] - start > _room) )
                {
                    continue;
                }

                if ( (best == 0) ||
                     (depths[c] <= best_depth) )
                {
                    best       = cuts[c];
                    best_depth = depths[c];
                }
            }

            // no cut within the room: the first past it, if there is one
            if (best == 0)
            {
                for (std::size_t cut : cuts)
                {
                    if (cut > start)
                    {
                        best = cut;

                        break;
                    }
                }
            }

            if (best == 0)
            {
                break;
            }

            pieces.push_back(_body.substr(start, best - start));
            start = best + 1;
        }

        pieces.push_back(_body.substr(start));

        return pieces;
    }

    // definition
    //   "#define head  body" on one line if it fits; otherwise the head
    // alone, and the body below it, indented, over as many lines as it
    // needs, every line but the last ending in a backslash.
    void definition(
        const std::string& _head,
        const std::string& _body,
        std::size_t        _column
    )
    {
        definition(std::vector<std::string>{ _head }, _body, _column);

        return;
    }

    // definition
    //   the same, for a head already laid out over several lines.
    void definition(
        const std::vector<std::string>& _head,
        const std::string&              _body,
        std::size_t                     _column
    )
    {
        const bool lines = (_body.find('\n') != std::string::npos);

        if ( (_head.size() == 1) &&
             (!lines) )
        {
            const std::string single = _body.empty()
                                           ? _head.front()
                                           : padded(_head.front(), _column) +
                                                 _body;

            if (single.size() <= LINE)
            {
                line(single);

                return;
            }

            // not aligned, but on one line
            if (_head.front().size() + 1 + _body.size() <= LINE)
            {
                line(_head.front() + " " + _body);

                return;
            }
        }

        // the head, then the body below it
        for (std::size_t i = 0; i < _head.size(); ++i)
        {
            if ( (i + 1 == _head.size()) &&
                 (_body.empty()) )
            {
                line(_head[i]);

                return;
            }

            line(continued(_head[i]));
        }

        std::vector<std::string> pieces;

        if (lines)
        {
            std::size_t at = 0;

            while (at <= _body.size())
            {
                const std::size_t end  = _body.find('\n', at);
                const std::size_t stop = (end == std::string::npos)
                                             ? _body.size()
                                             : end;
                std::string       one  = _body.substr(at, stop - at);

                while ( (!one.empty()) &&
                        ( (one.back() == ' ') || (one.back() == '\t') ) )
                {
                    one.pop_back();
                }

                pieces.push_back("    " + one);

                if (end == std::string::npos)
                {
                    break;
                }

                at = end + 1;
            }

            // an empty last line would end the macro on a backslash
            while ( (pieces.size() > 1) &&
                    (pieces.back() == "    ") )
            {
                pieces.pop_back();
            }
        }
        else
        {
            // an initializer's words align after its brace, an expression's
            // after its parenthesis
            const bool                     braced =
                (_body.compare(0, 2, "{ ") == 0);
            const std::string              rest   = braced ? "      "
                                                           : "     ";
            const std::vector<std::string> cut    =
                split_body(_body, braced ? (LINE - 8) : (LINE - 6));

            for (std::size_t i = 0; i < cut.size(); ++i)
            {
                pieces.push_back(((i == 0) ? "    " : rest) + cut[i]);
            }
        }

        for (std::size_t i = 0; i < pieces.size(); ++i)
        {
            if (i + 1 < pieces.size())
            {
                line(continued(pieces[i]));
            }
            else
            {
                line(pieces[i]);
            }
        }

        return;
    }

    // macro_head
    //   "#define NAME(a, b)" on one line, or -- when that and its backslash
    // would pass the 80th column -- its parameters one to a line, aligned
    // after the parenthesis, as the style guide spreads them.
    static std::vector<std::string> macro_head(
        const std::string& _name,
        const std::string& _params
    )
    {
        const std::string whole = "#define " + _name + "(" + _params + ")";

        if (whole.size() + 2 <= LINE)
        {
            return { whole };
        }

        std::vector<std::string> params;
        std::size_t              at = 0;

        while (at <= _params.size())
        {
            const std::size_t comma = _params.find(',', at);
            const std::size_t end   = (comma == std::string::npos)
                                          ? _params.size()
                                          : comma;
            std::string       one   = _params.substr(at, end - at);
            const std::size_t first = one.find_first_not_of(' ');

            one = (first == std::string::npos) ? std::string()
                                               : one.substr(first);

            while ( (!one.empty()) &&
                    (one.back() == ' ') )
            {
                one.pop_back();
            }

            params.push_back(one);

            if (comma == std::string::npos)
            {
                break;
            }

            at = comma + 1;
        }

        const std::string        open   = "#define " + _name + "(";
        const std::string        indent(open.size(), ' ');
        std::vector<std::string> out;

        for (std::size_t i = 0; i < params.size(); ++i)
        {
            out.push_back(((i == 0) ? open : indent) + params[i] +
                          ((i + 1 < params.size()) ? "," : ")"));
        }

        return out;
    }

    // padded
    //   _head, then spaces to _column -- or one space when it is past it.
    std::string padded(
        const std::string& _head,
        std::size_t        _column
    ) const
    {
        const std::size_t pad =
            ( (m_gen.align) &&
              (_head.size() + 1 < _column) ) ? (_column - _head.size())
                                             : 1;

        return _head + std::string(pad, ' ');
    }

    void blank()
    {
        m_lines.emplace_back();

        return;
    }

    // set_apart
    //   without briefs, a definition over several lines stands between
    // empty lines, as one stands between briefs: _first is its first line.
    void set_apart(
        std::size_t _first
    )
    {
        if ( (_first > 0) &&
             (!m_lines[_first - 1].empty()) &&
             (m_lines[_first - 1].compare(0, 4, "//==") != 0) &&
             (m_lines[_first - 1].compare(0, 4, "//--") != 0) )
        {
            m_lines.insert(m_lines.begin() +
                               static_cast<std::ptrdiff_t>(_first),
                           std::string());
        }

        blank();

        return;
    }

    void note(const std::string& _text)
    {
        // each note once
        for (const std::string& each : m_noted)
        {
            if (each == _text)
            {
                return;
            }
        }

        m_noted.push_back(_text);
        m_out.notes.push_back(_text);

        return;
    }

    // wrap
    //   words laid out over lines of at most LINE characters: the first
    // line starts with _first, the rest with _rest, each followed by a
    // space before its first word.
    void wrap(
        const std::string& _text,
        const std::string& _first,
        const std::string& _rest
    )
    {
        std::string current = _first;
        bool        fresh   = true;
        std::size_t at      = 0;

        while (at < _text.size())
        {
            // the next word, and the blanks before it
            while ( (at < _text.size()) &&
                    (_text[at] == ' ') )
            {
                ++at;
            }

            const std::size_t start = at;

            while ( (at < _text.size()) &&
                    (_text[at] != ' ') )
            {
                ++at;
            }

            if (start == at)
            {
                break;
            }

            const std::string word      = _text.substr(start, at - start);
            const std::string candidate = current + " " + word;

            // a word that would run past the line starts the next one
            if ( (!fresh) &&
                 (candidate.size() > LINE) )
            {
                line(current);
                current = _rest + " " + word;
            }
            else
            {
                current = candidate;
            }

            fresh = false;
        }

        line(current);

        return;
    }

    // brief
    //   the style guide's two-line comment over a definition.
    void brief(
        const std::string& _name,
        const std::string& _category,
        const std::string& _text
    )
    {
        if (!m_gen.comments)
        {
            return;
        }

        if (m_djinterp)
        {
            line("// " + _name);
            wrap(_text, "//   " + _category + ":", "//");
        }
        else if (!_text.empty())
        {
            wrap(_text, "//", "//");
        }

        return;
    }

    // ----------------------------------------------------------- literals

    radix level_radix(radix _wanted)
    {
        if ( (_wanted == radix::bin) &&
             (!has_binary_literals(m_gen.level)) )
        {
            note(std::string("binary literals need C23 or C++14; ") +
                 standard_name(m_gen.level) + " gets hexadecimal");

            return radix::hex;
        }

        return _wanted;
    }

    int_suffix level_suffix(int_suffix _wanted)
    {
        if ( (_wanted == int_suffix::ull) &&
             (!has_long_long(m_gen.level)) )
        {
            note(std::string(standard_name(m_gen.level)) +
                 " has no unsigned long long; ull is written as ul" +
                 ((m_sheet.width > 32)
                      ? ", which holds 64 bits only where long does (LP64: "
                        "not Windows, nor a 32-bit target)"
                      : ""));

            return int_suffix::ul;
        }

        return _wanted;
    }

    std::string literal(
        u64                 _value,
        const resolved_row& _row,
        bool                _pad,
        int                 _digits = 0
    )
    {
        return format_literal(_value,
                              m_sheet.width,
                              level_radix(_row.fmt),
                              level_suffix(_row.suf),
                              m_sheet,
                              _pad,
                              _digits);
    }

    // shifted
    //   (mask << lsb), the shift radix's spelling of a run.
    std::string shifted(
        u64                 _mask,
        int                 _lsb,
        const resolved_row& _row
    )
    {
        // a mask of one digit reads as one: (1u << 3), (7u << 4)
        const std::string mask = format_literal(_mask,
                                                m_sheet.width,
                                                (_mask < 10u) ? radix::dec
                                                              : radix::hex,
                                                level_suffix(_row.suf),
                                                m_sheet,
                                                false);

        return "(" + mask + " << " + format_dec(static_cast<u64>(_lsb)) +
               ")";
    }

    // ---------------------------------------------------- a wide sheet's C

    // word_type
    //   the C or C++ type of one of a wide sheet's words, as a constant
    // holds it.
    std::string word_type() const
    {
        if ( (m_cpp) &&
             (m_gen.level >= standard::cpp11) )
        {
            return "std::uint" + format_dec(static_cast<u64>(m_word)) +
                   "_t";
        }

        switch (m_word)
        {
            case 8:  return "unsigned char";
            case 16: return "unsigned short";
            case 64: return has_long_long(m_gen.level) ? "unsigned long long"
                                                       : "unsigned long";
            default: return "unsigned int";
        }
    }

    // word_cast
    //   the cast a word is written back through: the carrier narrowed to
    // the word, which says the narrowing is meant.
    std::string word_cast() const
    {
        switch (m_word)
        {
            case 8:  return "(unsigned char)";
            case 16: return "(unsigned short)";
            case 32: return "(unsigned int)";
            default: return std::string();
        }
    }

    // carrier
    //   the type a field is moved in, across words: the widest unsigned
    // type the level has.
    std::string carrier() const
    {
        return has_long_long(m_gen.level) ? "unsigned long long"
                                          : "unsigned long";
    }

    // mask_literal
    //   _bits as a literal as wide as _width bits needs: unsigned to 32
    // bits, unsigned long long past them.
    std::string mask_literal(
        u64 _bits,
        int _width
    )
    {
        return format_literal(_bits,
                              _width,
                              radix::hex,
                              (_width <= 32) ? int_suffix::u
                                             : level_suffix(int_suffix::ull),
                              m_sheet,
                              true,
                              0);
    }

    // words_of
    //   how many words a wide sheet's arrays have.
    int words_of() const
    {
        return (m_sheet.width + m_word - 1) / m_word;
    }

    // position
    //   a bit's number, as a wide sheet names a place: plainly, or in the
    // shift radix as its word and its place in the word.
    std::string position(
        int                 _bit,
        const resolved_row& _row
    ) const
    {
        if (_row.fmt == radix::shift)
        {
            return "(" + format_dec(static_cast<u64>(_bit / m_word)) + " * " +
                   format_dec(static_cast<u64>(m_word)) + " + " +
                   format_dec(static_cast<u64>(_bit % m_word)) + ")";
        }

        return format_dec(static_cast<u64>(_bit));
    }

    // initializer
    //   a set of bits as the words of an array, lowest first.
    std::string initializer(const wide& _bits)
    {
        const int   words = words_of();
        std::string out   = "{ ";

        for (int k = 0; k < words; ++k)
        {
            out += mask_literal(_bits.field(k * m_word, m_word), m_word);
            out += (k + 1 < words) ? ", " : " }";
        }

        return out;
    }

    // is_array
    //   whether a symbol of a wide sheet is a set of bits, written as an
    // array's initializer, rather than a number.
    bool is_array(
        const resolved_row& _row,
        const row&          _source
    ) const
    {
        return ( (m_wide) &&
                 ( (_source.def == def_kind::expression) ||
                   ( (_source.def == def_kind::literal) &&
                     (!_source.negative) &&
                     (_row.bits.highest() >= INTEGER_WIDTH) ) ) );
    }

    /*
    field_get
      The expression that reads _width bits from _lsb out of the array
    _array: within one word, the word shifted down and masked; across
    words, each word widened to the carrier, moved to its place in the
    field, and the whole masked.
    */
    std::string field_get(
        int                _lsb,
        int                _width,
        const std::string& _array
    )
    {
        const int         first = _lsb / m_word;
        const int         last  = (_lsb + _width - 1) / m_word;
        const std::string mask  = mask_literal(width_mask(_width), _width);

        // word
        //   one word of the array.
        auto word = [&_array](int _index)
        {
            return "(" + _array + ")[" +
                   format_dec(static_cast<u64>(_index)) + "]";
        };

        if (first == last)
        {
            const int shift = _lsb % m_word;

            return "(" +
                   ( (shift == 0) ? word(first)
                                  : "(" + word(first) + " >> " +
                                        format_dec(static_cast<u64>(shift)) +
                                        ")" ) +
                   " & " + mask + ")";
        }

        std::string parts;

        for (int k = first; k <= last; ++k)
        {
            const int         offset = k * m_word - _lsb;
            const std::string part   = "((" + carrier() + ")" + word(k) + ")";

            parts += (parts.empty() ? "" : " | ");
            parts += (offset < 0)  ? "(" + part + " >> " +
                                         format_dec(static_cast<u64>(
                                             -offset)) + ")"
                   : (offset == 0) ? part
                                   : "(" + part + " << " +
                                         format_dec(static_cast<u64>(
                                             offset)) + ")";
        }

        return "((" + parts + ") & " + mask + ")";
    }

    /*
    field_put
      The expression that writes _value into _width bits from _lsb of the
    array _array: each word the field touches has its part of the field
    cleared and the value's bits for it put in, the words written one after
    another with commas.
    */
    std::string field_put(
        int                _lsb,
        int                _width,
        const std::string& _array,
        const std::string& _value
    )
    {
        const int   first = _lsb / m_word;
        const int   last  = (_lsb + _width - 1) / m_word;
        std::string out;

        for (int k = first; k <= last; ++k)
        {
            const int         low   = std::max(_lsb, k * m_word) - k * m_word;
            const int         high  = std::min(_lsb + _width,
                                               (k + 1) * m_word) -
                                      k * m_word;
            const std::string mask  = mask_literal(range_mask(low,
                                                              high - low),
                                                   m_word);
            const int         move  = _lsb - k * m_word;
            const std::string value = "((" + carrier() + ")(" + _value + "))";
            const std::string moved =
                (move > 0)  ? "(" + value + " << " +
                                  format_dec(static_cast<u64>(move)) + ")"
              : (move < 0)  ? "(" + value + " >> " +
                                  format_dec(static_cast<u64>(-move)) + ")"
                            : value;
            const std::string word  = "(" + _array + ")[" +
                                      format_dec(static_cast<u64>(k)) + "]";

            out += (out.empty() ? "" : ", ");
            out += word + " = " + word_cast() + "((" + word + " & ~" + mask +
                   ") | (" + moved + " & " + mask + "))";
        }

        return "(" + out + ")";
    }

    // full_name_of
    //   the macro name a name in an expression or a body stands for.
    std::string full_name_of(
        const std::string& _name,
        int                _from
    )
    {
        std::string error;
        const int   id = find_name(m_res, _name, _from, error);

        if (id == 0)
        {
            note("'" + _name + "' names no symbol; written as it is");

            return _name;
        }

        return m_res.find(id)->full;
    }

    std::string expression_text(
        const resolved_row& _row,
        const row&          _source
    )
    {
        const expression parsed = parse_expression(_source.expr);

        return print_c(
            parsed,
            [this, &_row](const std::string& _name)
            {
                return full_name_of(_name, _row.id);
            },
            [this, &_row](u64 _value)
            {
                return literal(_value, _row, false);
            },
            [this, &_row](const std::string& _name,
                          u64&               _value,
                          std::string&       _error)
            {
                const int id = find_name(m_res, _name, _row.id, _error);

                if (id == 0)
                {
                    return false;
                }

                _value = m_res.find(id)->value;

                return true;
            });
    }

    std::string value_text(
        const resolved_row& _row,
        const row&          _source
    )
    {
        const bool shift = (_row.fmt == radix::shift);

        // a wide sheet's symbols are places and numbers, and sets of bits
        if (m_wide)
        {
            switch (_source.def)
            {
                case def_kind::bit:
                case def_kind::range:
                    return position(_row.span_lsb, _row);

                // the bits its field holds, as the field's _GET reads them
                case def_kind::field_value:
                    return format_literal(_row.bits.field(_row.span_lsb,
                                                          _row.span_width),
                                          _row.span_width,
                                          level_radix(_row.fmt),
                                          level_suffix(_row.suf),
                                          m_sheet,
                                          m_gen.pad);

                case def_kind::literal:
                    if (_source.negative)
                    {
                        return "(" + format_signed(static_cast<i64>(
                                         _source.literal.low())) + ")";
                    }

                    return is_array(_row, _source)
                               ? initializer(_row.bits)
                               : literal(_row.value, _row, false,
                                         _source.digits);

                case def_kind::expression:
                    return initializer(_row.bits);

                case def_kind::macro:
                    break;
            }

            return std::string();
        }

        switch (_source.def)
        {
            case def_kind::bit:
                return shift ? shifted(1u, _row.span_lsb, _row)
                             : literal(_row.value, _row, m_gen.pad);

            case def_kind::range:
                return shift ? shifted(width_mask(_row.span_width),
                                       _row.span_lsb,
                                       _row)
                             : literal(_row.value, _row, m_gen.pad);

            case def_kind::field_value:
                return shift ? shifted(_row.value >> _row.span_lsb,
                                       _row.span_lsb,
                                       _row)
                             : literal(_row.value, _row, m_gen.pad);

            case def_kind::literal:
            {
                const i64 number = sign_extend(_row.value, m_sheet.width);
                int       lsb    = 0;
                int       width  = 0;

                // a negative literal is written as the number it is
                if ( (_source.negative) &&
                     (number < 0) )
                {
                    return "(" + format_signed(number) + ")";
                }

                if ( (shift) &&
                     (is_run(_row.value, lsb, width)) )
                {
                    return shifted(width_mask(width), lsb, _row);
                }

                return literal(_row.value, _row, m_gen.pad, _source.digits);
            }

            case def_kind::expression:
                return expression_text(_row, _source);

            case def_kind::macro:
                break;
        }

        return std::string();
    }

    // ---------------------------------------------------------- templates

    // applies
    //   whether a template is written beside a symbol: one that is on, made
    // for the symbol's kind, and for the sheet's -- a wide sheet's for a
    // sheet wider than an integer, the others for the rest.
    bool applies(
        const macro_template& _made,
        const row&            _source
    ) const
    {
        return ( (_made.enabled)          &&
                 (_made.wide == m_wide)   &&
                 ((_made.applies & applies_bit(_source.def)) != 0) );
    }

    // uses
    //   whether a template's name, parameters or body has a placeholder.
    static bool uses(
        const macro_template& _made,
        const std::string&    _key
    )
    {
        return ( (_made.name.find(_key) != std::string::npos)   ||
                 (_made.params.find(_key) != std::string::npos) ||
                 (_made.body.find(_key) != std::string::npos) );
    }

    std::string substitute(
        const std::string&  _text,
        const resolved_row& _row,
        const row&          _source
    )
    {
        std::string mask = _row.full;

        // a value's mask is its field's name, or its bits when it has none
        // -- in a wide sheet, its bits in the word it begins in
        if (_source.def == def_kind::field_value)
        {
            const resolved_row* field = m_res.find(_row.field);
            const int           shift = (m_wide && (_row.span_lsb >= 0))
                                            ? (_row.span_lsb % m_word)
                                            : 0;

            mask = (field != nullptr)
                       ? field->full
                   : m_wide
                       ? mask_literal(range_mask(shift,
                                                 std::min(_row.span_width,
                                                          m_word - shift)),
                                      m_word)
                       : literal(range_mask(_row.span_lsb, _row.span_width),
                                 _row,
                                 false);
        }

        std::vector<std::pair<const char*, std::string>> table = {
            { "{NAME}",   _row.full },
            { "{LOCAL}",  _row.local },
            { "{VALUE}",  value_text(_row, _source) },
            { "{MASK}",   mask },
            { "{SHIFT}",  format_signed(_row.span_lsb) },
            { "{WIDTH}",  format_signed(_row.span_width) },
            { "{BIT}",    format_signed(_row.span_lsb) },
            { "{PREFIX}", _row.prefix },
            { "{SUFFIX}", _row.suffix } };

        // a wide sheet's: the word a span begins in and where in it, its
        // bits there, the array macros, and a field read and written
        if ( (m_wide) &&
             (_row.span_lsb >= 0) )
        {
            const int         lsb   = _row.span_lsb;
            const int         shift = lsb % m_word;
            const int         here  = std::min(_row.span_width,
                                               m_word - shift);
            const resolved_row* field = m_res.find(_row.field);

            table.push_back({ "{FIELD}", (field != nullptr) ? field->full
                                                            : _row.full });
            table.push_back({ "{WORD}",
                              format_dec(static_cast<u64>(lsb / m_word)) });
            table.push_back({ "{WORD_SHIFT}",
                              format_dec(static_cast<u64>(shift)) });
            table.push_back({ "{WORD_MASK}",
                              mask_literal(range_mask(shift, here),
                                           m_word) });
            table.push_back({ "{WORD_BITS}",
                              format_dec(static_cast<u64>(m_word)) });

            if (_row.span_width <= INTEGER_WIDTH)
            {
                table.push_back({ "{GET}",
                                  field_get(lsb, _row.span_width, "a") });
                table.push_back({ "{PUT}",
                                  field_put(lsb, _row.span_width, "a",
                                            "v") });
            }
        }

        if (m_wide)
        {
            table.push_back({ "{BITSET}", m_bitset });
        }

        std::string out = _text;

        for (const std::pair<const char*, std::string>& each : table)
        {
            const std::string key = each.first;

            if (out.find(key) == std::string::npos)
            {
                continue;
            }

            for (std::size_t at = out.find(key);
                 at != std::string::npos;
                 at = out.find(key, at + each.second.size()))
            {
                out.replace(at, key.size(), each.second);
            }
        }

        return out;
    }

    // body_text
    //   a macro row's body with {name} placeholders as macro names.
    std::string body_text(
        const resolved_row& _row,
        const row&          _source
    )
    {
        std::vector<std::string> unknown;
        const std::string        out = macro_body(m_res, _source, &unknown);

        for (const std::string& each : unknown)
        {
            note("{" + each + "} in " + _row.full + " names no symbol; "
                 "written as it is");
        }

        return out;
    }

    // ---------------------------------------------------------- definitions

    // type_of
    //   the C++ type of a constant _bits wide: the fixed-width type that
    // holds it -- or, in C++98, which has no long long, a long, as wide as
    // it gets.
    std::string type_of(
        int  _bits,
        bool _signed
    ) const
    {
        const int bits = (_bits <= 8)  ? 8
                       : (_bits <= 16) ? 16
                       : (_bits <= 32) ? 32
                                       : 64;

        if (m_gen.level == standard::cpp98)
        {
            return _signed ? "long" : "unsigned long";
        }

        return std::string("std::") + (_signed ? "int" : "uint") +
               format_dec(static_cast<u64>(bits)) + "_t";
    }

    std::string type_for(bool _signed) const
    {
        return type_of(m_sheet.width, _signed);
    }

    // constant_type
    //   the C++ type of a symbol's constant: a narrow sheet's are as wide as
    // the sheet; a wide sheet's places are ints, its values as wide as
    // their fields, and its arrays arrays of its words.
    std::string constant_type(
        const resolved_row& _row,
        const row&          _source
    ) const
    {
        const bool negative = ( (_source.def == def_kind::literal) &&
                                (_source.negative) );

        if (!m_wide)
        {
            return type_for(negative);
        }

        switch (_source.def)
        {
            case def_kind::bit:
            case def_kind::range:
                return "int";

            case def_kind::field_value:
                return type_of(_row.span_width, false);

            case def_kind::literal:
                if (is_array(_row, _source))
                {
                    return word_type();
                }

                return negative ? type_of(INTEGER_WIDTH, true)
                                : type_of(std::max(1,
                                                   _row.bits.highest() + 1),
                                          false);

            case def_kind::expression:
                return word_type();

            case def_kind::macro:
                break;
        }

        return type_for(false);
    }

    // define
    //   one object-like definition: a macro, or in the C++ constants form a
    // constant of _type -- an array of the sheet's words when _array says
    // so.
    void define(
        const std::string& _name,
        const std::string& _value,
        const std::string& _type,
        std::size_t        _column,
        bool               _array = false
    )
    {
        if (m_constants)
        {
            const char* qualifier =
                (m_gen.level >= standard::cpp17) ? "inline constexpr "
              : (m_gen.level >= standard::cpp11) ? "constexpr "
                                                 : "const ";

            const std::string declarator = _array ? _name + "[" + m_bitset +
                                                        "WORDS]"
                                                  : _name;
            const std::string head  = std::string(qualifier) + _type + " " +
                                      declarator + " =";
            const std::string whole = head + " " + _value + ";";

            // a long constant breaks after its equals sign
            if (whole.size() <= LINE)
            {
                line(whole);
            }
            else if (!_array)
            {
                line(head);
                line("    " + _value + ";");
            }
            else
            {
                // and a long initializer between its words, aligned
                const std::vector<std::string> cut =
                    split_body(_value + ";", LINE - 6);

                line(head);

                for (std::size_t i = 0; i < cut.size(); ++i)
                {
                    line(((i == 0) ? "    " : "      ") + cut[i]);
                }
            }

            return;
        }

        definition("#define " + _name, _value, _column);

        return;
    }

    // function_macro
    //   one function-like macro.
    void function_macro(
        const std::string& _name,
        const std::string& _params,
        const std::string& _body,
        std::size_t        _column
    )
    {
        definition(macro_head(_name, _params), _body, _column);

        return;
    }

    std::string describe(
        const resolved_row& _row,
        const row&          _source
    )
    {
        if (!_source.note.empty())
        {
            std::string text = _source.note;

            if (text.back() != '.')
            {
                text += '.';
            }

            return text;
        }

        const std::string span =
            (_row.span_width == 1)
                ? "bit " + format_signed(_row.span_lsb)
                : "bits " + format_signed(_row.span_lsb) + ".." +
                      format_signed(_row.span_lsb + _row.span_width - 1);

        // a wide sheet's symbols are places, numbers and arrays
        if (m_wide)
        {
            switch (_source.def)
            {
                case def_kind::bit:
                    return span + ", by its number.";

                case def_kind::range:
                    return span + ", by its lowest bit's number.";

                case def_kind::literal:
                    if (is_array(_row, _source))
                    {
                        return runs_text(_row.bits, 4) + ", as an array's "
                               "initializer.";
                    }

                    break;

                case def_kind::expression:
                    return _source.expr + ", as an array's initializer.";

                default:
                    break;
            }
        }

        switch (_source.def)
        {
            case def_kind::bit:
                return span + ".";

            case def_kind::range:
                return span + ", as a mask.";

            case def_kind::field_value:
            {
                const resolved_row* field = m_res.find(_row.field);
                const std::string   owner =
                    (field != nullptr) ? field->local : std::string("field");

                return owner + " = " + format_signed(_row.number) + " (" +
                       encoding_name(_source.enc) + ", " + span + ")" +
                       (m_wide ? ", as its field holds it." : ".");
            }

            case def_kind::literal:
                return "the value " +
                       (_source.negative
                            ? format_signed(sign_extend(_row.value,
                                                        m_sheet.width))
                            : format_dec(_row.value)) + ".";

            case def_kind::expression:
                return _source.expr + ".";

            case def_kind::macro:
                break;
        }

        return std::string();
    }

    // column_for
    //   the column a group's values align to: past its longest name.
    std::size_t column_for(const row& _group) const
    {
        std::size_t longest = 0;

        for (const row& child : _group.children)
        {
            const resolved_row* each = m_res.find(child.id);

            if ( (each == nullptr) ||
                 (child.kind != row_kind::symbol) )
            {
                continue;
            }

            // a macro's head is its name and its parameters; one whose
            // body takes lines of its own aligns with nothing
            if (child.def == def_kind::macro)
            {
                if (child.body.find('\n') == std::string::npos)
                {
                    longest = std::max(longest,
                                       each->full.size() +
                                           (function_like(child)
                                                ? child.params.size() + 2
                                                : 0));
                }

                continue;
            }

            longest = std::max(longest, each->full.size());

            // a range's templates are longer than its name
            for (const macro_template& made : m_gen.templates)
            {
                if (applies(made, child))
                {
                    longest = std::max(longest,
                                       each->full.size() + made.name.size() +
                                           made.params.size());
                }
            }
        }

        return std::min<std::size_t>(std::string("#define ").size() +
                                         longest + 2,
                                     48);
    }

    void write_symbol(
        const row&  _source,
        std::size_t _column
    )
    {
        const resolved_row* found = m_res.find(_source.id);

        if (found == nullptr)
        {
            return;
        }

        const resolved_row& each = *found;

        // a symbol in error stays out, and the notes say why
        if (!each.error.empty())
        {
            ++m_out.skipped;
            note("left out " + each.full + ": " + each.error);

            return;
        }

        if (_source.def == def_kind::macro)
        {
            brief(each.full, "macro", describe(each, _source));

            const std::size_t before = m_lines.size();

            if (!function_like(_source))
            {
                definition("#define " + each.full,
                           body_text(each, _source),
                           _column);
            }
            else
            {
                function_macro(each.full,
                               _source.params,
                               body_text(each, _source),
                               _column);
            }

            ++m_out.macros;

            if (m_gen.comments)
            {
                blank();
            }
            else if (m_lines.size() > before + 1)
            {
                set_apart(before);
            }

            return;
        }

        brief(each.full, "constant", describe(each, _source));
        define(each.full,
               value_text(each, _source),
               constant_type(each, _source),
               _column,
               is_array(each, _source));
        ++m_out.macros;

        for (const macro_template& made : m_gen.templates)
        {
            if (!applies(made, _source))
            {
                continue;
            }

            // no integer carries a range wider than one
            if ( (each.span_width > INTEGER_WIDTH) &&
                 ( (uses(made, "{GET}")) || (uses(made, "{PUT}")) ) )
            {
                note(substitute(made.name, each, _source) + " is left out: " +
                     each.full + " spans " +
                     format_dec(static_cast<u64>(each.span_width)) +
                     " bits, more than any integer holds");

                continue;
            }

            const std::string name = substitute(made.name, each, _source);
            const std::string body = substitute(made.body, each, _source);

            if ( (m_djinterp) &&
                 (m_gen.comments) )
            {
                blank();
            }

            brief(name,
                  made.params.empty() ? "constant" : "macro",
                  substitute(made.note, each, _source) + ".");

            if (made.params.empty())
            {
                // a template is a macro even beside C++ constants: it may
                // name the symbol it is written for
                definition("#define " + name, body, _column);
            }
            else
            {
                function_macro(name, made.params, body, _column);
            }

            ++m_out.macros;
        }

        // without briefs, a group's definitions sit together
        if (m_gen.comments)
        {
            blank();
        }

        return;
    }

    // -------------------------------------------------------- structure

    // number_groups
    //   the first walk: every group that will be written, numbered.
    void number_groups(
        const row&         _group,
        int                _depth,
        const std::string& _prefix
    )
    {
        int ordinal = 0;

        // symbols straight under the sheet open a section of their own
        if ( (_depth == 0)              &&
             (has_symbols(_group))      &&
             (has_groups(_group)) )
        {
            ++ordinal;

            heading made;
            made.id     = 0;
            made.depth  = 0;
            made.number = "1";
            made.title  = upper(m_sheet.title.empty() ? "symbols"
                                                      : m_sheet.title);
            m_headings.push_back(made);
        }

        for (const row& child : _group.children)
        {
            if ( (child.kind != row_kind::group) ||
                 (!child.emit) )
            {
                continue;
            }

            ++ordinal;

            heading made;
            made.id     = child.id;
            made.depth  = _depth;
            made.number = _prefix + format_dec(static_cast<u64>(ordinal));
            made.title  = (_depth == 0) ? upper(display(child))
                                        : display(child);
            made.note   = child.note;
            m_headings.push_back(made);

            // the guide numbers three levels; deeper groups are comments
            if (_depth < 2)
            {
                number_groups(child, _depth + 1, made.number + ".");
            }
        }

        return;
    }

    bool has_symbols(const row& _group) const
    {
        for (const row& child : _group.children)
        {
            if (child.kind == row_kind::symbol)
            {
                return true;
            }
        }

        return false;
    }

    bool has_groups(const row& _group) const
    {
        for (const row& child : _group.children)
        {
            if (child.kind == row_kind::group)
            {
                return true;
            }
        }

        return false;
    }

    static std::string upper(const std::string& _text)
    {
        std::string out = _text;

        for (char& each : out)
        {
            each = static_cast<char>(
                std::toupper(static_cast<unsigned char>(each)));
        }

        return out;
    }

    static std::string display(const row& _group)
    {
        return _group.name.empty() ? std::string("group")
                                   : _group.name;
    }

    const heading* heading_for(int _id) const
    {
        for (const heading& each : m_headings)
        {
            if (each.id == _id)
            {
                return &each;
            }
        }

        return nullptr;
    }

    void write_heading(const heading& _heading)
    {
        if (_heading.depth == 0)
        {
            // two empty lines before a section, however the last ended
            if (!m_lines.empty())
            {
                while ( (m_lines.size() >= 2) &&
                        (m_lines.back().empty()) &&
                        (m_lines[m_lines.size() - 2].empty()) )
                {
                    m_lines.pop_back();
                }

                while ( (m_lines.size() < 2) ||
                        (!m_lines.back().empty()) ||
                        (!m_lines[m_lines.size() - 2].empty()) )
                {
                    blank();
                }
            }

            line(RULE_EQUALS);
            line("// " + _heading.number + ".  " + _heading.title);
            line(RULE_EQUALS);

            if (!_heading.note.empty())
            {
                wrap(_heading.note, "//", "//");
            }

            blank();
            blank();

            return;
        }

        if (_heading.depth == 1)
        {
            std::string number = _heading.number;
            number.resize(std::max<std::size_t>(number.size() + 1, 7), ' ');

            line("// " + number + _heading.title);
            line(RULE_DASHES);

            if (!_heading.note.empty())
            {
                wrap(_heading.note, "//", "//");
            }

            return;
        }

        line("// " + _heading.number);
        line("// " + _heading.title);

        if (!_heading.note.empty())
        {
            wrap(_heading.note, "//", "//");
        }

        return;
    }

    // chosen
    //   whether a row is one of those asked for, or the whole sheet is.
    bool chosen(
        int _id
    ) const
    {
        return ( (m_scope == nullptr) ||
                 (std::find(m_scope->begin(), m_scope->end(), _id) !=
                  m_scope->end()) );
    }

    // holds_chosen
    //   whether anything in a group is to be written.
    bool holds_chosen(
        const row& _group,
        bool       _inside
    ) const
    {
        for (const row& child : _group.children)
        {
            if (!child.emit)
            {
                continue;
            }

            const bool in = ( (_inside) || (chosen(child.id)) );

            if ( (child.kind == row_kind::symbol) &&
                 (in) )
            {
                return true;
            }

            if ( (can_contain(child)) &&
                 (holds_chosen(child, in)) )
            {
                return true;
            }
        }

        return false;
    }

    // write_group
    //   the second walk: a group's symbols, then its groups. _inside says
    // the group is one of those asked for, and so is all of it.
    void write_group(
        const row& _group,
        int        _depth,
        bool       _inside
    )
    {
        const bool        numbered = ( (m_djinterp)          &&
                                       (m_gen.sections)      &&
                                       (m_scope == nullptr) );
        const std::size_t column   = column_for(_group);

        // the sheet's own symbols, under their section when there is one
        if ( (_depth == 0)          &&
             (numbered)             &&
             (has_symbols(_group))  &&
             (has_groups(_group)) )
        {
            write_heading(m_headings.front());
        }

        for (const row& child : _group.children)
        {
            if ( (child.kind == row_kind::symbol) &&
                 (child.emit) &&
                 ( (_inside) || (chosen(child.id)) ) )
            {
                write_symbol(child, column);
            }
        }

        for (const row& child : _group.children)
        {
            const bool in = ( (_inside) || (chosen(child.id)) );

            if ( (!can_contain(child)) ||
                 (!child.emit) ||
                 (!holds_chosen(child, in)) )
            {
                continue;
            }

            // A range already has its #define emitted above; recurse
            // into nested ranges/symbols without generating a group heading.
            if (child.kind == row_kind::symbol)
            {
                write_group(child, _depth + 1, in);
                continue;
            }

            const heading* found = heading_for(child.id);

            if ( (numbered) &&
                 (found != nullptr) )
            {
                write_heading(*found);
            }
            else
            {
                if ( (!m_lines.empty()) &&
                     (!m_lines.back().empty()) )
                {
                    blank();
                }

                line("// " + display(child));

                if ( (!child.note.empty()) &&
                     (!m_djinterp) )
                {
                    wrap(child.note, "//", "//");
                }

                // a group's heading stands apart from its first brief
                if (m_gen.comments)
                {
                    blank();
                }
            }

            write_group(child, _depth + 1, in);
        }

        return;
    }

    // ----------------------------------------------------------- framing

    void write_banner()
    {
        if (!m_djinterp)
        {
            line("/* " + m_gen.file_name + " -- generated by bitwis(e)dom from "
                 "the sheet \"" + m_sheet.title + "\". Do not edit. */");
            blank();

            return;
        }

        const std::string tag   = "* bitwisedom [generated]";
        const std::string name  = m_gen.file_name;
        const std::size_t room  = (tag.size() + 1 + name.size() <= LINE)
                                      ? LINE - tag.size() - name.size()
                                      : 1;
        int               count = 0;

        for (const resolved_row& each : m_res.rows)
        {
            count += (each.kind == row_kind::symbol) ? 1 : 0;
        }

        const std::string created = "created: " + m_date;
        const std::string author  = "* author(s): " + m_gen.author;
        const std::size_t gap     =
            (author.size() + created.size() < LINE)
                ? LINE - author.size() - created.size()
                : 1;

        line("/" + std::string(79, '*'));
        line(tag + std::string(room, ' ') + name);
        line("*");
        wrap(m_sheet.title + ": " + format_dec(static_cast<u64>(count)) +
                 " symbols in " +
                 format_dec(static_cast<u64>(m_sheet.width)) +
                 " bits, written by bitwis(e)dom.",
             "*",
             "*");
        line("*   Generated: edit the sheet, not this file. Writing it again "
             "replaces");
        line("* every line.");
        line("*");
        line("* path:      /" + name);
        line("* link(s):   TBA");
        line(author + std::string(gap, ' ') + created);
        line("*" + std::string(60, ' ') + "revised: " + m_date);
        line(std::string(79, '*') + "/");
        blank();

        return;
    }

    void write_contents()
    {
        line("/*");
        line("TABLE OF CONTENTS");
        line("=================");

        for (const heading& each : m_headings)
        {
            const std::size_t dot     = each.number.rfind('.');
            const std::string ordinal =
                (dot == std::string::npos) ? each.number
                                           : each.number.substr(dot + 1);
            const std::string label   =
                (ordinal.size() == 1) ? ordinal + ".  " : ordinal + ". ";

            if (each.depth == 0)
            {
                line(label + each.title);
                line(std::string(label.size(), ' ') +
                     std::string(each.title.size(), '-'));
            }
            else if (each.depth == 1)
            {
                line("    " + label + each.title);
            }
            else
            {
                line("         " + label + each.title);
            }
        }

        line("*/");
        blank();

        return;
    }

    void write_preamble()
    {
        if ( (m_constants) &&
             (m_gen.level >= standard::cpp11) )
        {
            if (m_djinterp)
            {
                line("// std");
            }

            line("#include <cstdint>  // the fixed-width integer types");
            blank();
        }

        if (m_wide)
        {
            write_arrays();
        }

        if ( (m_constants) &&
             (!m_gen.name_space.empty()) )
        {
            line("namespace " + m_gen.name_space);
            line("{");
            blank();
        }

        return;
    }

    /*
    write_arrays
      A wide sheet's array macros, ahead of its symbols: how many bits a
    word holds and how many words hold the sheet, and a bit's word, its mask
    there, and its test, set and clear, by its number. A word narrower than
    an int is written back through a cast, which says the narrowing is
    meant.
    */
    void write_arrays()
    {
        const std::string p     = m_bitset;
        const std::string bits  = format_dec(static_cast<u64>(m_word));
        const std::string words = format_dec(static_cast<u64>(words_of()));
        const std::string one   =
            format_literal(1u,
                           m_word,
                           radix::dec,
                           (m_word > 32) ? level_suffix(int_suffix::ull)
                                         : int_suffix::u,
                           m_sheet,
                           false);
        const std::string word  = "(a)[" + p + "WORD(n)]";
        const std::string cast  = (m_word < 32) ? word_cast() : std::string();
        const std::string names[] = { p + "WORD_BITS", p + "WORDS",
                                      p + "WORD", p + "MASK", p + "TEST",
                                      p + "SET", p + "CLEAR" };

        // a symbol of the same name would be defined twice
        for (const resolved_row& each : m_res.rows)
        {
            for (const std::string& name : names)
            {
                if ( (each.kind == row_kind::symbol) &&
                     (each.full == name) )
                {
                    note(name + " is both a symbol and one of the array "
                         "macros; a prefix for the sheet, or another name, "
                         "keeps them apart");
                }
            }
        }

        line("// " + m_sheet.title + "'s arrays");
        wrap("The sheet's " + format_dec(static_cast<u64>(m_sheet.width)) +
                 " bits are held in an array of " + words + " words of " +
                 bits + " bits, lowest first: bit n is bit n % " + bits +
                 " of word n / " + bits + ".",
             m_djinterp ? "//  " : "//",
             "//");

        if (m_gen.comments)
        {
            blank();
        }

        const std::size_t column =
            std::string("#define ").size() + p.size() + 13;

        // define_one
        //   one of the array macros, with its brief.
        auto define_one = [&](const std::string& _name,
                              const std::string& _params,
                              const std::string& _body,
                              const std::string& _text)
        {
            brief(_name, _params.empty() ? "constant" : "macro", _text);

            if (_params.empty())
            {
                definition("#define " + _name, _body, column);
            }
            else
            {
                function_macro(_name, _params, _body, column);
            }

            ++m_out.macros;

            if (m_gen.comments)
            {
                blank();
            }

            return;
        };

        define_one(p + "WORD_BITS", "", bits, "how many bits a word holds.");
        define_one(p + "WORDS", "", words,
                   "how many words hold the sheet's bits.");
        define_one(p + "WORD", "n", "((n) / " + p + "WORD_BITS)",
                   "the word bit n is in.");
        define_one(p + "MASK", "n",
                   "(" + one + " << ((n) % " + p + "WORD_BITS))",
                   "bit n's mask in its word.");
        define_one(p + "TEST", "a, n",
                   "((" + word + " & " + p + "MASK(n)) != 0)",
                   "whether bit n is set in the array a.");
        define_one(p + "SET", "a, n",
                   cast.empty()
                       ? "(" + word + " |= " + p + "MASK(n))"
                       : "(" + word + " = " + cast + "(" + word + " | " + p +
                             "MASK(n)))",
                   "sets bit n in the array a.");
        define_one(p + "CLEAR", "a, n",
                   cast.empty()
                       ? "(" + word + " &= ~" + p + "MASK(n))"
                       : "(" + word + " = " + cast + "(" + word + " & ~" +
                             p + "MASK(n)))",
                   "clears bit n in the array a.");

        if (!m_gen.comments)
        {
            blank();
        }

        return;
    }
};

}  // anonymous namespace


const char*
standard_name(
    standard _level
)
{
    switch (_level)
    {
        case standard::c99:   return "C99";
        case standard::c11:   return "C11";
        case standard::c17:   return "C17";
        case standard::c23:   return "C23";
        case standard::cpp98: return "C++98";
        case standard::cpp11: return "C++11";
        case standard::cpp14: return "C++14";
        case standard::cpp17: return "C++17";
        case standard::cpp20: return "C++20";
        case standard::cpp23: return "C++23";
    }

    return "C99";
}

bool
is_cpp(
    standard _level
)
{
    return (_level >= standard::cpp98);
}

bool
has_binary_literals(
    standard _level
)
{
    return ( (_level == standard::c23) ||
             (_level >= standard::cpp14) );
}

bool
has_long_long(
    standard _level
)
{
    return (_level != standard::cpp98);
}

std::string
format_literal(
    u64          _value,
    int          _width,
    radix        _radix,
    int_suffix   _suffix,
    const sheet& _sheet,
    bool         _pad,
    int          _digits
)
{
    std::string suffix;

    switch (_suffix)
    {
        case int_suffix::u:   suffix = "u";   break;
        case int_suffix::ul:  suffix = "ul";  break;
        case int_suffix::ull: suffix = "ull"; break;
        default:                              break;
    }

    if (_sheet.gen.upper_suffix)
    {
        for (char& each : suffix)
        {
            each = static_cast<char>(
                std::toupper(static_cast<unsigned char>(each)));
        }
    }

    switch (_radix)
    {
        case radix::dec:
            return format_dec(_value) + suffix;

        case radix::oct:
        {
            std::string digits = (_value == 0) ? std::string()
                                               : format_oct(_value);

            while (static_cast<int>(digits.size()) < _digits)
            {
                digits.insert(digits.begin(), '0');
            }

            return "0" + digits + suffix;
        }

        case radix::bin:
        {
            const int width = std::max(_pad ? _width
                                            : std::max(1,
                                                       highest_set(_value) + 1),
                                       std::min(_digits, 64));

            return "0b" + format_bin(_value, width, 0, '\0') + suffix;
        }

        default:
            return "0x" +
                   format_hex(_value,
                              std::max(_pad ? (_width + 3) / 4 : 0, _digits),
                              _sheet.gen.upper_hex) +
                   suffix;
    }
}

std::string
derive_guard(
    const std::string& _file_name
)
{
    std::string guard;

    // letters and digits upper-cased, every other run one underscore
    for (char each : _file_name)
    {
        if (std::isalnum(static_cast<unsigned char>(each)))
        {
            guard += static_cast<char>(
                std::toupper(static_cast<unsigned char>(each)));
        }
        else if ( (!guard.empty()) &&
                  (guard.back() != '_') )
        {
            guard += '_';
        }
    }

    while ( (!guard.empty()) &&
            (guard.back() == '_') )
    {
        guard.pop_back();
    }

    if ( (guard.empty()) ||
         (std::isdigit(static_cast<unsigned char>(guard[0]))) )
    {
        guard = "BITWISEDOM_" + guard;
    }

    return guard;
}

std::string
bitset_prefix(
    const sheet& _sheet
)
{
    std::string prefix = _sheet.prefix.empty() ? derive_guard(_sheet.title)
                                               : _sheet.prefix;

    if ( (!prefix.empty()) &&
         (prefix.back() != '_') )
    {
        prefix += '_';
    }

    return prefix;
}

generated
generate(
    const sheet&       _sheet,
    const resolution&  _resolution,
    const std::string& _date
)
{
    generated out;
    writer    pass(_sheet, _resolution, _date, out);

    pass.run();

    return out;
}

generated
generate_code(
    const sheet&            _sheet,
    const resolution&       _resolution,
    const std::vector<int>& _rows
)
{
    generated out;
    writer    pass(_sheet, _resolution, std::string(), out, &_rows);

    pass.run();

    return out;
}


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
