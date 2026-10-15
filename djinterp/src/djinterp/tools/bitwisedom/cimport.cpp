/*******************************************************************************
* bitwisedom [bitwisedom]                                            cimport.cpp
*
* C and C++ code read into a sheet.
*   One pass over the tokens, comments included. Directives are read as
* lines, through the same conditionals the define environment follows;
* comments are read as lines too, and classified: a rule, a section's
* heading, a numbered item's marker, or text. Text directly above a
* definition is its note. Groups are made as rules and headings are met,
* and those left empty are taken out at the end, when the untitled ones
* are named.
*   A body keeps its layout: each of its lines is its tokens with the
* spacing between them, and is indented as far past the body's first token
* as it was in the file, tabs counted to the next multiple of four.
*
* path:      /src/djinterp/tools/bitwisedom/cimport.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.09
*                                                            revised: 2026.10.10
*******************************************************************************/
#include <djinterp/tools/bitwisedom/cimport.hpp>  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <algorithm>  // std::max, std::min, std::find
#include <cctype>     // std::isdigit, std::isspace
#include <cstddef>    // std::size_t
#include <map>        // std::map
#include <set>        // std::set
#include <utility>    // std::move
// bitwisedom
#include <djinterp/tools/bitwisedom/cenv.hpp>     // c_env, c_branches
#include <djinterp/tools/bitwisedom/clex.hpp>     // lex_c, decode_literal
#include <djinterp/tools/bitwisedom/codegen.hpp>  // format_literal, derive_guard
#include <djinterp/tools/bitwisedom/resolve.hpp>  // resolve


namespace bitwisedom
{

namespace
{

// TAB
//   constant: the tab stops a file's indentation is read with.
const int TAB = 4;

// trim
//   a text without the blanks around it.
std::string
trim(
    const std::string& _text
)
{
    const std::size_t first = _text.find_first_not_of(" \t\r\n");

    if (first == std::string::npos)
    {
        return std::string();
    }

    const std::size_t last = _text.find_last_not_of(" \t\r\n");

    return _text.substr(first, last - first + 1);
}

// visual_column
//   where a byte stands in its line, from 0: tabs to the next multiple of
// TAB, and a UTF-8 character one column whatever its length.
int
visual_column(
    std::string_view _text,
    std::size_t      _offset
)
{
    std::size_t start = 0;

    if (_offset > 0)
    {
        const std::size_t newline = _text.rfind('\n', _offset - 1);

        start = (newline == std::string_view::npos) ? 0 : newline + 1;
    }

    int column = 0;

    for (std::size_t i = start; ( (i < _offset) && (i < _text.size()) ); ++i)
    {
        const unsigned char each = static_cast<unsigned char>(_text[i]);

        if (each == '\t')
        {
            column = (column / TAB + 1) * TAB;
        }
        else if ((each & 0xC0u) != 0x80u)
        {
            ++column;
        }
    }

    return column;
}

// comment_lines
//   a comment's lines, without its marks: `//`, `/*`, `*/`, and the `*` a
// block comment's lines begin with.
std::vector<std::string>
comment_lines(
    const c_token& _comment
)
{
    std::vector<std::string> out;
    std::string              text = _comment.text;

    if (text.compare(0, 2, "//") == 0)
    {
        out.push_back(text.substr(2));

        return out;
    }

    if (text.compare(0, 2, "/*") == 0)
    {
        text = text.substr(2);
    }

    if ( (text.size() >= 2) &&
         (text.compare(text.size() - 2, 2, "*/") == 0) )
    {
        text = text.substr(0, text.size() - 2);
    }

    std::size_t at = 0;

    while (at <= text.size())
    {
        const std::size_t end = text.find('\n', at);
        std::string       one = text.substr(at, (end == std::string::npos)
                                                    ? std::string::npos
                                                    : end - at);
        const std::size_t first = one.find_first_not_of(" \t");

        // a block comment's lines often begin " * "
        if ( (first != std::string::npos) &&
             (one[first] == '*') )
        {
            one = one.substr(first + 1);
        }

        out.push_back(one);

        if (end == std::string::npos)
        {
            break;
        }

        at = end + 1;
    }

    return out;
}

// line_kind
//   enum: what one comment line is.
enum class line_kind
{
    text,
    rule_equals,     // //====...
    rule_dashes,     // //----...
    heading,         // 1.  TITLE
    subheading,      // 1.1    Title
    item             // 1.1.1, alone
};

// numbered
//   a line's leading section number -- "1.", "1.1", "1.1.1" -- its parts
// counted, and what follows it.
int
numbered(
    const std::string& _line,
    std::string&       _rest,
    bool&              _dot
)
{
    std::size_t at    = 0;
    int         parts = 0;

    _dot = false;

    while (at < _line.size())
    {
        const std::size_t start = at;

        while ( (at < _line.size()) &&
                (std::isdigit(static_cast<unsigned char>(_line[at]))) )
        {
            ++at;
        }

        if (at == start)
        {
            break;
        }

        ++parts;

        if ( (at < _line.size()) &&
             (_line[at] == '.') )
        {
            ++at;
            _dot = true;

            continue;
        }

        _dot = false;

        break;
    }

    _rest = _line.substr(at);

    return parts;
}

line_kind
classify_line(
    const std::string& _line,
    std::string&       _title
)
{
    const std::string text = trim(_line);

    _title.clear();

    if ( (text.size() >= 8) &&
         (text.find_first_not_of('=') == std::string::npos) )
    {
        return line_kind::rule_equals;
    }

    if ( (text.size() >= 8) &&
         (text.find_first_not_of('-') == std::string::npos) )
    {
        return line_kind::rule_dashes;
    }

    std::string rest;
    bool        dot   = false;
    const int   parts = numbered(text, rest, dot);

    if (parts == 0)
    {
        return line_kind::text;
    }

    const std::string after = trim(rest);

    // 1.  TITLE -- one part and its dot, two spaces, a title
    if ( (parts == 1) &&
         (dot) &&
         (rest.size() >= 2) &&
         (rest.compare(0, 1, " ") == 0) &&
         (!after.empty()) )
    {
        _title = after;

        return line_kind::heading;
    }

    // 1.1    Title -- two parts, no dot, at least two spaces
    if ( (parts == 2) &&
         (!dot) &&
         (rest.compare(0, 2, "  ") == 0) &&
         (!after.empty()) )
    {
        _title = after;

        return line_kind::subheading;
    }

    if ( (parts >= 3) &&
         (after.empty()) )
    {
        return line_kind::item;
    }

    return line_kind::text;
}

// brief_text
//   a comment block above a definition, as a note. A djinterp brief --
// the name, then "  category: text" -- is read as one; numbered markers and
// doxygen's tags are dropped.
std::string
brief_text(
    const std::vector<std::string>& _lines,
    const std::string&              _name
)
{
    std::string out;

    for (std::size_t i = 0; i < _lines.size(); ++i)
    {
        std::string line  = trim(_lines[i]);
        std::string title;

        if ( (line.empty()) ||
             (classify_line(line, title) == line_kind::item) )
        {
            continue;
        }

        // the brief's first line is the name it is about
        if ( (i == 0) &&
             ( (line == _name) ||
               (line.compare(0, _name.size(), _name) == 0) ) &&
             (_lines.size() > 1) )
        {
            continue;
        }

        // "category: text"
        const std::size_t colon = line.find(':');

        if ( (out.empty()) &&
             (colon != std::string::npos) &&
             (colon < 12) &&
             (line.find(' ') == std::string::npos ||
              line.find(' ') > colon) )
        {
            line = trim(line.substr(colon + 1));
        }

        if (line.compare(0, 6, "@brief") == 0)
        {
            line = trim(line.substr(6));
        }
        else if ( (!line.empty()) &&
                  (line[0] == '@') )
        {
            continue;
        }

        if (line.empty())
        {
            continue;
        }

        out += (out.empty() ? "" : " ") + line;
    }

    // the generator adds the full stop a brief ends with
    while ( (!out.empty()) &&
            (out.back() == '.') )
    {
        out.pop_back();
    }

    return out;
}

// squeezed
//   a text without its blanks, to compare spellings by.
std::string
squeezed(
    const std::string& _text
)
{
    std::string out;

    for (char each : _text)
    {
        if (!std::isspace(static_cast<unsigned char>(each)))
        {
            out += each;
        }
    }

    return out;
}

// importer
//   class: one import in progress.
class importer
{
public:
    importer(
        std::string_view      _text,
        const import_options& _options,
        imported&             _out
    )
        : m_text(_text),
          m_options(_options),
          m_out(_out),
          m_env(settings_of(_options)),
          m_branches(m_env)
    {}

    void run()
    {
        const std::string title = m_options.title.empty()
                                      ? std::string("imported")
                                      : m_options.title;

        m_out.result = make_sheet(title, 64);

        sheet& made = m_out.result;
        made.gen.level     = m_options.level;
        made.gen.lang      = cpp_level(m_options.level) ? language::cpp
                                                         : language::c;
        made.gen.model     = m_options.model;
        made.gen.file_name = m_options.file_name.empty()
                                 ? title + ".h"
                                 : m_options.file_name;
        made.gen.pad       = false;
        made.gen.banner    = false;
        made.gen.guard     = false;

        // the templates would write macros the code never had
        for (macro_template& each : made.gen.templates)
        {
            each.enabled = false;
        }

        const c_lexed scanned = lex_c(m_text, m_options.level, true);

        for (const std::string& error : scanned.errors)
        {
            m_out.notes.push_back("line " + error);
        }

        m_tokens = &scanned.tokens;

        read_case();
        read_guard();
        read_banner();
        walk();
        finish();

        return;
    }

private:
    std::string_view            m_text;
    const import_options&       m_options;
    imported&                   m_out;
    c_env                       m_env;
    c_branches                  m_branches;
    const std::vector<c_token>* m_tokens = nullptr;

    // groups: the current section, its current subsection and item
    int                         m_top  = 0;
    int                         m_sub  = 0;
    int                         m_item = 0;
    std::string                 m_item_title;
    bool                        m_item_next = false;
    std::vector<int>            m_untitled;

    // comments: the block above what comes next, and the section state
    enum class reading
    {
        nothing,
        opened,      // a rule of =, which may open a heading
        titled,      // its heading, which a rule of = closes
        closed,      // the heading closed: its description may follow
        subtitled    // a subsection's heading, which a rule of - closes
    };

    std::vector<std::string>    m_block;
    int                         m_block_end = -10;
    reading                     m_reading   = reading::nothing;
    int                         m_last_line = -10;

    // definitions
    std::map<std::string, int>  m_rows;
    std::string                 m_guard;
    bool                        m_guard_read = false;
    int                         m_unread     = 0;
    std::set<int>               m_code;
    bool                        m_noted      = false;

    static c_settings settings_of(
        const import_options& _options
    )
    {
        c_settings made;
        made.level = _options.level;
        made.model = _options.model;

        return made;
    }

    void note(
        int                _line,
        const std::string& _text
    )
    {
        m_out.notes.push_back("line " + std::to_string(_line) + ": " + _text);

        return;
    }

    // read_case
    //   the case of the file's hex digits and suffixes, from the first
    // literal that shows each: the generator writes one case for a sheet.
    void read_case()
    {
        bool hex_seen    = false;
        bool suffix_seen = false;

        for (const c_token& each : *m_tokens)
        {
            if (each.kind != c_class::number)
            {
                continue;
            }

            const c_literal literal = decode_literal(each,
                                                     m_options.level,
                                                     m_options.model);

            if ( (!literal.ok) ||
                 (!literal.integer) )
            {
                continue;
            }

            if ( (!hex_seen) &&
                 ( (literal.upper_hex) || (literal.lower_hex) ) )
            {
                hex_seen = true;
                m_out.result.gen.upper_hex = literal.upper_hex;
            }

            if ( (!suffix_seen) &&
                 ( (literal.is_unsigned) || (literal.longs > 0) ) )
            {
                suffix_seen = true;
                m_out.result.gen.upper_suffix = literal.upper_suffix;
            }
        }

        return;
    }

    /*
    read_guard
      An include guard: the first directive #ifndef G, the second #define
    G with nothing or 1 for a body, and the last #endif, with nothing but
    comments after it.
    */
    void read_guard()
    {
        const auto lines = directive_lines(*m_tokens);

        if (lines.size() < 3)
        {
            return;
        }

        std::vector<c_token> first;
        std::vector<c_token> second;
        std::vector<c_token> last;

        for (const c_token& each : lines.front().second)
        {
            if (each.kind != c_class::comment)
            {
                first.push_back(each);
            }
        }

        for (const c_token& each : lines[1].second)
        {
            if (each.kind != c_class::comment)
            {
                second.push_back(each);
            }
        }

        for (const c_token& each : lines.back().second)
        {
            if (each.kind != c_class::comment)
            {
                last.push_back(each);
            }
        }

        const bool opens  = ( (first.size() == 2) &&
                              (first[0].text == "ifndef") );
        const bool names  = ( (second.size() >= 2)      &&
                              (second.size() <= 3)      &&
                              (second[0].text == "define") &&
                              (opens) &&
                              (second[1].text == first[1].text) &&
                              ( (second.size() == 2) ||
                                (second[2].text == "1") ) );
        const bool closes = ( (!last.empty()) &&
                              (last[0].text == "endif") );

        if ( (!names) ||
             (!closes) )
        {
            return;
        }

        // nothing but comments after the #endif
        const int end_line = lines.back().first;

        for (const c_token& each : *m_tokens)
        {
            if ( (each.line > end_line) &&
                 (each.kind != c_class::comment) )
            {
                return;
            }
        }

        m_guard = first[1].text;

        sheet& made = m_out.result;
        made.gen.guard      = true;
        made.gen.guard_name = (m_guard == derive_guard(made.gen.file_name))
                                  ? std::string()
                                  : m_guard;

        return;
    }

    // read_banner
    //   a djinterp banner -- a block comment of stars before anything
    // else -- and the author it names.
    void read_banner()
    {
        if ( (m_tokens->empty()) ||
             (m_tokens->front().kind != c_class::comment) ||
             (m_tokens->front().text.compare(0, 22,
                                             "/*********************") != 0) )
        {
            return;
        }

        sheet& made = m_out.result;
        made.gen.banner = true;

        for (const std::string& line : comment_lines(m_tokens->front()))
        {
            const std::size_t at = line.find("author(s):");

            if (at == std::string::npos)
            {
                continue;
            }

            std::string author  = line.substr(at + 10);
            const std::size_t created = author.find("created:");

            if (created != std::string::npos)
            {
                author = author.substr(0, created);
            }

            author = trim(author);

            if (!author.empty())
            {
                made.gen.author = author;
            }
        }

        return;
    }

    // ------------------------------------------------------------ the walk

    void walk()
    {
        const std::vector<c_token>& tokens = *m_tokens;
        bool                        banner = m_out.result.gen.banner;

        for (std::size_t i = 0; i < tokens.size(); )
        {
            const c_token& each = tokens[i];

            if ( (each.is("#")) &&
                 (each.has(C_LINE_START)) &&
                 (each.has(C_IN_DIRECTIVE)) )
            {
                std::vector<c_token> line;
                std::size_t          j = i + 1;

                for (; j < tokens.size(); ++j)
                {
                    if ( (!tokens[j].has(C_IN_DIRECTIVE)) ||
                         ( (tokens[j].is("#")) &&
                           (tokens[j].has(C_LINE_START)) ) )
                    {
                        break;
                    }

                    line.push_back(tokens[j]);
                }

                directive(each.line, line);
                i = j;

                continue;
            }

            if (each.kind == c_class::comment)
            {
                // the banner is the generator's to write
                if ( (!banner) &&
                     (m_branches.live()) )
                {
                    comment(each);
                }

                banner = false;
                ++i;

                continue;
            }

            if (m_branches.live())
            {
                m_code.insert(each.line);
                m_block.clear();
                m_reading = reading::nothing;
            }

            banner = false;
            ++i;
        }

        if (m_branches.depth() > 0)
        {
            m_out.notes.push_back(format_dec(static_cast<u64>(
                                      m_branches.depth())) +
                                  " #if without its #endif at the end");
        }

        return;
    }

    // ------------------------------------------------------------ comments

    void comment(
        const c_token& _comment
    )
    {
        const std::vector<std::string> lines = comment_lines(_comment);

        // a table of contents is the generator's to write
        for (const std::string& line : lines)
        {
            if (trim(line) == "TABLE OF CONTENTS")
            {
                return;
            }
        }

        // a blank line ends a block, and a section's description
        if (_comment.line > m_last_line + 1)
        {
            m_block.clear();

            if (m_reading == reading::closed)
            {
                m_reading = reading::nothing;
            }
        }

        for (std::size_t k = 0; k < lines.size(); ++k)
        {
            std::string     title;
            const line_kind kind = classify_line(lines[k], title);

            switch (kind)
            {
                case line_kind::rule_equals:
                    if (m_reading == reading::titled)
                    {
                        m_reading = reading::closed;
                    }
                    else
                    {
                        open_section(std::string());
                        m_reading = reading::opened;
                    }

                    m_block.clear();
                    break;

                case line_kind::rule_dashes:
                    m_reading = (m_reading == reading::subtitled)
                                    ? reading::closed
                                    : reading::nothing;
                    m_block.clear();
                    break;

                case line_kind::heading:
                    if (m_reading == reading::opened)
                    {
                        name_section(title);
                        m_reading = reading::titled;
                    }
                    else
                    {
                        m_block.push_back(lines[k]);
                    }

                    break;

                case line_kind::subheading:
                    open_subsection(title);
                    m_reading = reading::subtitled;
                    m_block.clear();
                    break;

                case line_kind::item:
                    // its title is the next line
                    m_item_next = true;
                    m_block.clear();
                    break;

                case line_kind::text:
                    if ( (m_item_next) &&
                         (!trim(lines[k]).empty()) )
                    {
                        m_item_title = trim(lines[k]);
                        m_item_next  = false;
                        m_reading    = reading::nothing;
                    }
                    else if ( (m_reading == reading::closed) &&
                         (!trim(lines[k]).empty()) )
                    {
                        describe(trim(lines[k]));
                    }
                    else if ( (m_reading == reading::opened) ||
                              (m_reading == reading::titled) )
                    {
                        // a rule's companion line that is not a heading
                        m_reading = reading::nothing;
                        m_block.push_back(lines[k]);
                    }
                    else
                    {
                        m_reading = reading::nothing;
                        m_block.push_back(lines[k]);
                    }

                    break;
            }
        }

        m_last_line = _comment.end_line;
        m_block_end = _comment.end_line;

        // a line comment ends where it starts
        if (_comment.text.compare(0, 2, "//") == 0)
        {
            m_last_line = _comment.line;
            m_block_end = _comment.line;
        }

        return;
    }

    void open_section(
        const std::string& _title
    )
    {
        row group;
        group.kind      = row_kind::group;
        group.name      = _title;
        group.exclusive = false;

        m_top  = insert_row(m_out.result, 0, -1, group);
        m_sub  = 0;
        m_item = 0;
        m_item_title.clear();

        if (_title.empty())
        {
            m_untitled.push_back(m_top);
        }

        return;
    }

    void name_section(
        const std::string& _title
    )
    {
        row* group = find_row(m_out.result, m_top);

        if (group == nullptr)
        {
            open_section(_title);

            return;
        }

        group->name = _title;
        m_untitled.erase(std::remove(m_untitled.begin(),
                                     m_untitled.end(),
                                     m_top),
                         m_untitled.end());

        return;
    }

    void open_subsection(
        const std::string& _title
    )
    {
        row group;
        group.kind      = row_kind::group;
        group.name      = _title;
        group.exclusive = false;

        m_sub  = insert_row(m_out.result, m_top, -1, group);
        m_item = 0;
        m_item_title.clear();

        return;
    }

    // open_item
    //   a numbered item that names a run of definitions, not one of them:
    // a group inside the subsection.
    void open_item(
        const std::string& _title
    )
    {
        row group;
        group.kind      = row_kind::group;
        group.name      = _title;
        group.exclusive = false;

        m_item = insert_row(m_out.result,
                            (m_sub != 0) ? m_sub : m_top,
                            -1,
                            group);

        return;
    }

    void describe(
        const std::string& _text
    )
    {
        const int at    = (m_item != 0) ? m_item
                        : (m_sub != 0)  ? m_sub
                                        : m_top;
        row*      group = find_row(m_out.result, at);

        if (group != nullptr)
        {
            group->note += (group->note.empty() ? "" : " ") + _text;
        }

        return;
    }

    // ---------------------------------------------------------- directives

    void directive(
        int                         _number,
        const std::vector<c_token>& _line
    )
    {
        std::vector<c_token> words;

        for (const c_token& each : _line)
        {
            if (each.kind != c_class::comment)
            {
                words.push_back(each);
            }
        }

        m_reading = reading::nothing;

        if (words.empty())
        {
            m_block.clear();

            return;
        }

        if (m_branches.feed(words, _number, m_out.notes))
        {
            m_block.clear();

            return;
        }

        const std::string& kind = words[0].text;

        if (!m_branches.live())
        {
            m_unread += (kind == "define") ? 1 : 0;
            m_block.clear();

            return;
        }

        if (kind == "define")
        {
            define(_number, _line, words);
        }
        else if (kind == "undef")
        {
            if (words.size() > 1)
            {
                (void)m_env.undefine(words[1].text);

                if (m_rows.count(words[1].text) != 0)
                {
                    note(_number, words[1].text + " is #undef'd here; its "
                                  "row stays");
                }
            }
        }
        else if (kind == "include")
        {
            include(_number, words);
        }
        else if ( (kind == "pragma") &&
                  (words.size() > 1) &&
                  (words[1].text == "once") )
        {
            m_out.result.gen.guard       = true;
            m_out.result.gen.pragma_once = true;
        }
        else if ( (kind == "error") ||
                  (kind == "warning") )
        {
            note(_number, "#" + kind + " in a branch taken: " +
                              spelled(std::vector<c_token>(words.begin() + 1,
                                                           words.end())));
        }

        m_block.clear();

        return;
    }

    void include(
        int                         _number,
        const std::vector<c_token>& _words
    )
    {
        static const char* const known[] = {
            "<stdint.h>", "<cstdint>", "<limits.h>", "<climits>",
            "<stdbool.h>", "<stddef.h>", "<cstddef>", "<inttypes.h>",
            "<cinttypes>" };

        const std::string target =
            spelled(std::vector<c_token>(_words.begin() + 1, _words.end()));

        for (const char* each : known)
        {
            if (squeezed(target) == each)
            {
                return;
            }
        }

        note(_number, "#include " + target + " is not followed: what it "
                      "defines is not known here, and can go in the sheet's "
                      "environment");

        return;
    }

    /*
    layout
      A body's text with its layout: each line its tokens and the spacing
    between them, indented as far past the body's first token as it was.
    A comment inside the body is left out.
    */
    std::string layout(
        const std::vector<c_token>& _words,
        std::size_t                 _from
    ) const
    {
        if (_from >= _words.size())
        {
            return std::string();
        }

        const int   base    = visual_column(m_text, _words[_from].offset);
        std::string out;
        int         line    = -1;
        std::size_t end     = 0;

        for (std::size_t i = _from; i < _words.size(); ++i)
        {
            const c_token& each = _words[i];

            if (each.line != line)
            {
                if (line != -1)
                {
                    out += '\n';
                }

                const int column = visual_column(m_text, each.offset);

                out += std::string(static_cast<std::size_t>(
                                       std::max(0, column - base)),
                                   ' ');
                line = each.line;
            }
            else
            {
                const std::string_view gap =
                    m_text.substr(end, each.offset - end);
                const bool plain =
                    (gap.find_first_not_of(" \t") == std::string_view::npos);

                if (plain)
                {
                    const int width = visual_column(m_text, each.offset) -
                                      visual_column(m_text, end);

                    out += std::string(static_cast<std::size_t>(
                                           std::max(1, width)),
                                       ' ');
                }
                else
                {
                    out += ' ';
                }

                // tokens that touched in the file still touch
                if ( (plain) &&
                     (gap.empty()) )
                {
                    out.pop_back();
                }
            }

            out += each.text;
            end  = each.offset + each.length;
        }

        return out;
    }

    // fits_written
    //   whether the generator writes a row's value back as _written was.
    bool fits_written(
        const std::string& _written,
        const std::string& _generated
    ) const
    {
        return (squeezed(_written) == squeezed(_generated));
    }

    /*
    classify
      A definition as the row it can be: a literal, bit or range row when
    the generator would write it back as it was written, a macro row
    otherwise.
    */
    row classify(
        const c_macro&              _macro,
        const std::vector<c_token>& _words,
        std::size_t                 _from,
        const std::string&          _body
    ) const
    {
        row made;
        made.def      = def_kind::macro;
        made.body     = _body;
        made.params   = _macro.function_like ? params_text(_macro)
                                             : std::string();
        made.callable = ( (_macro.function_like) &&
                          (made.params.empty()) );

        if ( (_macro.function_like) ||
             (_from >= _words.size()) )
        {
            return made;
        }

        std::vector<c_token> body(_words.begin() +
                                      static_cast<std::ptrdiff_t>(_from),
                                  _words.end());
        const sheet& target = m_out.result;

        // literal  |  ( - literal )  |  ( literal << literal )
        if ( (body.size() == 1) &&
             (body[0].kind == c_class::number) )
        {
            const c_literal literal = decode_literal(body[0],
                                                     m_options.level,
                                                     m_options.model);
            int_suffix      suffix  = int_suffix::none;
            radix           base    = radix::hex;

            if ( (!integer_literal(literal, suffix, base)) )
            {
                return made;
            }

            const int digits = (base == radix::dec) ? 0 : literal.digits;

            if (!fits_written(body[0].text,
                              format_literal(literal.value, 64, base, suffix,
                                             target, false, digits)))
            {
                return made;
            }

            made.def     = def_kind::literal;
            made.literal = literal.value;
            made.fmt     = base;
            made.suf     = suffix;
            made.digits  = digits;

            return made;
        }

        if ( (body.size() == 4)                  &&
             (body[0].is("("))                   &&
             (body[1].is("-"))                   &&
             (body[2].kind == c_class::number)   &&
             (body[3].is(")")) )
        {
            const c_literal literal = decode_literal(body[2],
                                                     m_options.level,
                                                     m_options.model);
            int_suffix      suffix  = int_suffix::none;
            radix           base    = radix::dec;

            if ( (!integer_literal(literal, suffix, base)) ||
                 (base != radix::dec)                     ||
                 (suffix != int_suffix::none)             ||
                 (literal.value == 0)                     ||
                 (literal.value > (u64(1) << 63)) )
            {
                return made;
            }

            made.def      = def_kind::literal;
            made.literal  = u64(0) - literal.value;
            made.negative = true;
            made.fmt      = radix::dec;
            made.suf      = int_suffix::none;

            return made;
        }

        if ( (body.size() == 5)                  &&
             (body[0].is("("))                   &&
             (body[1].kind == c_class::number)   &&
             (body[2].is("<<"))                  &&
             (body[3].kind == c_class::number)   &&
             (body[4].is(")")) )
        {
            const c_literal mask  = decode_literal(body[1],
                                                   m_options.level,
                                                   m_options.model);
            const c_literal shift = decode_literal(body[3],
                                                   m_options.level,
                                                   m_options.model);
            int_suffix      suffix = int_suffix::none;
            radix           base   = radix::hex;
            int             lsb    = 0;
            int             width  = 0;

            if ( (!integer_literal(mask, suffix, base))    ||
                 (!shift.ok)                               ||
                 (!shift.integer)                          ||
                 (shift.base != 10)                        ||
                 (shift.value >= 64)                       ||
                 (!is_run(mask.value, lsb, width))         ||
                 (lsb != 0)                                ||
                 (static_cast<int>(shift.value) + width > 64) )
            {
                return made;
            }

            const std::string written =
                "(" + format_literal(mask.value, 64,
                                     (mask.value < 10u) ? radix::dec
                                                        : radix::hex,
                                     suffix, target, false) +
                " << " + format_dec(shift.value) + ")";

            if (!fits_written(_body, written))
            {
                return made;
            }

            made.fmt = radix::shift;
            made.suf = suffix;

            if (width == 1)
            {
                made.def = def_kind::bit;
                made.bit = static_cast<int>(shift.value);
            }
            else
            {
                made.def   = def_kind::range;
                made.lsb   = static_cast<int>(shift.value);
                made.width = width;
            }
        }

        return made;
    }

    // integer_literal
    //   whether a literal is an integer a row can hold: its suffix one of
    // none, u, ul and ull, its radix one the generator writes at this
    // level.
    bool integer_literal(
        const c_literal& _literal,
        int_suffix&      _suffix,
        radix&           _radix
    ) const
    {
        if ( (!_literal.ok)          ||
             (!_literal.integer)     ||
             (_literal.overflow)     ||
             (_literal.user_suffix)  ||
             (_literal.size)         ||
             (_literal.bit_precise)  ||
             ( (_literal.longs > 0) && (!_literal.is_unsigned) ) )
        {
            return false;
        }

        _suffix = !_literal.is_unsigned    ? int_suffix::none
                : (_literal.longs == 2)    ? int_suffix::ull
                : (_literal.longs == 1)    ? int_suffix::ul
                                           : int_suffix::u;

        switch (_literal.base)
        {
            case 16: _radix = radix::hex; break;
            case 10: _radix = radix::dec; break;
            case 8:  _radix = radix::oct; break;
            case 2:  _radix = radix::bin; break;
            default: return false;
        }

        return ( (_radix != radix::bin) ||
                 (has_binary_literals(m_options.level)) );
    }

    void define(
        int                         _number,
        const std::vector<c_token>& _line,
        const std::vector<c_token>& _words
    )
    {
        c_macro     macro;
        std::size_t from = 0;
        std::string error;

        if (!read_define(_words, macro, from, error))
        {
            note(_number, error);

            return;
        }

        // the include guard's own definition is the generator's
        if ( (!m_guard.empty())     &&
             (!m_guard_read)        &&
             (macro.name == m_guard) )
        {
            m_guard_read = true;
            (void)m_env.define(macro, error);

            return;
        }

        if (!m_env.define(macro, error))
        {
            note(_number, error);
        }

        // an item titled with another name than this one's is a group
        if (!m_item_title.empty())
        {
            if (m_item_title != macro.name)
            {
                open_item(m_item_title);
            }

            m_item_title.clear();
        }

        // its note: the block just above, or a comment on its own lines
        std::string text;

        if ( (!m_block.empty()) &&
             (m_block_end == _number - 1) )
        {
            text = brief_text(m_block, macro.name);
        }

        if (text.empty())
        {
            std::vector<std::string> trailing;

            for (const c_token& each : _line)
            {
                if (each.kind != c_class::comment)
                {
                    continue;
                }

                for (const std::string& one : comment_lines(each))
                {
                    trailing.push_back(one);
                }
            }

            text = brief_text(trailing, macro.name);
        }

        m_block.clear();

        row made = classify(macro, _words, from, layout(_words, from));

        made.name = macro.name;
        made.note = text;
        m_noted   = ( (m_noted) || (!text.empty()) );

        place(_number, std::move(made));

        return;
    }

    void place(
        int   _number,
        row&& _made
    )
    {
        ++m_out.definitions;

        const auto existing = m_rows.find(_made.name);

        if (existing != m_rows.end())
        {
            row* earlier = find_row(m_out.result, existing->second);

            if (earlier != nullptr)
            {
                if ( (earlier->body != _made.body)       ||
                     (earlier->params != _made.params)   ||
                     (earlier->def != _made.def)         ||
                     (earlier->literal != _made.literal) )
                {
                    note(_number, _made.name + " is defined again; the "
                                  "later definition is kept");
                }

                const std::string kept = earlier->note;
                const int         id   = earlier->id;

                *earlier = std::move(_made);
                earlier->id = id;

                if (earlier->note.empty())
                {
                    earlier->note = kept;
                }
            }

            return;
        }

        const int parent = (m_item != 0) ? m_item
                         : (m_sub != 0)  ? m_sub
                                         : m_top;
        const int id     = insert_row(m_out.result, parent, -1, _made);

        m_rows[_made.name] = id;

        return;
    }

    // ------------------------------------------------------------- the end

    void finish()
    {
        sheet& made = m_out.result;

        name_untitled();
        drop_empty(made.root);

        int groups = 0;

        walk_rows(made.root, groups);
        m_out.groups = groups;

        made.gen.comments = m_noted;
        made.gen.sections = (groups > 0);

        if (!m_code.empty())
        {
            m_out.skipped = static_cast<int>(m_code.size());
            m_out.notes.push_back(
                format_dec(m_code.size()) + " line(s) of declarations or "
                "code, from line " + std::to_string(*m_code.begin()) +
                ", are not definitions, and were left out");
        }

        if (m_unread > 0)
        {
            m_out.notes.push_back(format_dec(static_cast<u64>(m_unread)) +
                                  " #define(s) in branches not taken were "
                                  "left out");
        }

        // the width: 32 bits, unless a value needs more
        const resolution resolved = resolve(made);
        int              highest  = 0;

        for (const resolved_row& each : resolved.rows)
        {
            if ( (each.kind == row_kind::symbol) &&
                 (each.has_value) &&
                 (!each.negative) )
            {
                highest = std::max(highest, highest_set(each.value) + 1);
            }
        }

        made.width = (highest > 32) ? 64 : 32;
        reset_segments(made);

        m_out.ok = (m_out.definitions > 0);

        return;
    }

    void reset_segments(
        sheet& _sheet
    ) const
    {
        _sheet.segments.clear();

        return;
    }

    void walk_rows(
        const row& _group,
        int&       _groups
    )
    {
        for (const row& child : _group.children)
        {
            if (child.kind == row_kind::group)
            {
                ++_groups;
                walk_rows(child, _groups);

                continue;
            }

            switch (child.def)
            {
                case def_kind::literal:
                    ++m_out.literals;
                    break;

                case def_kind::bit:
                case def_kind::range:
                    ++m_out.allocations;
                    break;

                default:
                    ++m_out.macros;
                    break;
            }
        }

        return;
    }

    // drop_empty
    //   groups with no symbol in them, taken out.
    static bool drop_empty(
        row& _group
    )
    {
        bool any = false;

        for (std::size_t i = _group.children.size(); i-- > 0; )
        {
            row& child = _group.children[i];

            if (child.kind == row_kind::symbol)
            {
                any = true;

                continue;
            }

            if (drop_empty(child))
            {
                any = true;
            }
            else
            {
                _group.children.erase(_group.children.begin() +
                                      static_cast<std::ptrdiff_t>(i));
            }
        }

        return any;
    }

    static void names_in(
        const row&                _group,
        std::vector<std::string>& _names
    )
    {
        for (const row& child : _group.children)
        {
            if (child.kind == row_kind::group)
            {
                names_in(child, _names);
            }
            else
            {
                _names.push_back(child.name);
            }
        }

        return;
    }

    static std::vector<std::string> words_of(
        const std::string& _name
    )
    {
        std::vector<std::string> out;
        std::string              current;

        for (char each : _name)
        {
            if (each == '_')
            {
                if (!current.empty())
                {
                    out.push_back(current);
                }

                current.clear();

                continue;
            }

            current += each;
        }

        if (!current.empty())
        {
            out.push_back(current);
        }

        return out;
    }

    /*
    name_untitled
      A group no heading named takes the words its names begin with, past
    those every name in the file begins with -- INTERVAL for
    D_INTERVAL_A_MEETS_B and its kin -- or, when they share none, the words
    most of them begin with, or else its place.
    */
    void name_untitled()
    {
        std::vector<std::string> all;

        names_in(m_out.result.root, all);

        // the words every name begins with
        std::size_t common = all.empty() ? 0 : words_of(all.front()).size();

        for (const std::string& name : all)
        {
            const std::vector<std::string> words = words_of(name);
            const std::vector<std::string> first = words_of(all.front());
            std::size_t                    same  = 0;

            while ( (same < common)         &&
                    (same < words.size())   &&
                    (same < first.size())   &&
                    (words[same] == first[same]) )
            {
                ++same;
            }

            common = same;
        }

        int ordinal = 0;

        for (const row& child : m_out.result.root.children)
        {
            if (child.kind != row_kind::group)
            {
                continue;
            }

            ++ordinal;

            if (std::find(m_untitled.begin(),
                          m_untitled.end(),
                          child.id) == m_untitled.end())
            {
                continue;
            }

            std::vector<std::string> names;
            names_in(child, names);

            const std::string title = shared_words(names, common);
            row*              group = find_row(m_out.result, child.id);

            if (group != nullptr)
            {
                group->name = title.empty()
                                  ? "section " + std::to_string(ordinal)
                                  : title;
            }
        }

        return;
    }

    // shared_words
    //   the longest run of words, past the first _skip, that more than
    // half of the names begin with.
    static std::string shared_words(
        const std::vector<std::string>& _names,
        std::size_t                     _skip
    )
    {
        if (_names.empty())
        {
            return std::string();
        }

        std::map<std::string, int> counts;

        for (const std::string& name : _names)
        {
            const std::vector<std::string> words = words_of(name);
            std::string                    run;

            // every prefix of the name's words, past the skipped ones
            for (std::size_t i = _skip;
                 ( (i < words.size()) &&
                   // a name's last word is its own, not its group's,
                   // unless the group is that name alone
                   ( (i + 1 < words.size()) || (_names.size() == 1) ) );
                 ++i)
            {
                run += (run.empty() ? "" : "_") + words[i];
                ++counts[run];
            }
        }

        const int   all  = static_cast<int>(_names.size());
        std::string every;
        std::string most;

        for (const auto& each : counts)
        {
            if ( (each.second == all) &&
                 (each.first.size() > every.size()) )
            {
                every = each.first;
            }

            if ( (each.second * 2 > all) &&
                 (each.first.size() > most.size()) )
            {
                most = each.first;
            }
        }

        return every.empty() ? most : every;
    }
};

// affixes_inside
//   the prefix and suffix the names directly inside a group take.
void
affixes_inside(
    const sheet&      _sheet,
    const resolution& _resolution,
    int               _group,
    std::string&      _prefix,
    std::string&      _suffix
)
{
    const row*          group    = find_row(_sheet, _group);
    const resolved_row* resolved = _resolution.find(_group);

    if ( (_group == 0)         ||
         (group == nullptr)    ||
         (resolved == nullptr) )
    {
        _prefix = _sheet.prefix;
        _suffix = _sheet.suffix;

        return;
    }

    if (group->isolate)
    {
        _prefix = _sheet.prefix + group->prefix;
        _suffix = group->suffix + _sheet.suffix;

        return;
    }

    _prefix = resolved->prefix;
    _suffix = resolved->suffix;

    return;
}

// take_definition
//   one row's definition, given to another: everything that says what the
// symbol is, not where it is or what it is called.
void
take_definition(
    row&       _row,
    const row& _from
)
{
    _row.def      = _from.def;
    _row.bit      = _from.bit;
    _row.lsb      = _from.lsb;
    _row.width    = _from.width;
    _row.literal  = _from.literal;
    _row.negative = _from.negative;
    _row.digits   = _from.digits;
    _row.expr     = _from.expr;
    _row.params   = _from.params;
    _row.body     = _from.body;
    _row.callable = _from.callable;
    _row.fmt      = _from.fmt;
    _row.suf      = _from.suf;

    if (!_from.note.empty())
    {
        _row.note = _from.note;
    }

    return;
}

void
merge_into(
    sheet&            _into,
    const resolution& _before,
    int               _parent,
    int               _index,
    const row&        _from,
    merged&           _out
)
{
    for (const row& child : _from.children)
    {
        if (child.kind == row_kind::group)
        {
            // a group of the same name takes the rows; another is made
            int        target = 0;
            const row* parent = find_row(_into, _parent);

            if (parent != nullptr)
            {
                for (const row& each : parent->children)
                {
                    if ( (each.kind == row_kind::group) &&
                         (each.name == child.name) )
                    {
                        target = each.id;
                    }
                }
            }

            if (target == 0)
            {
                row made = child;
                made.children.clear();
                target = insert_row(_into, _parent, _index, made);

                if (_index >= 0)
                {
                    ++_index;
                }
            }

            merge_into(_into, _before, target, -1, child, _out);

            continue;
        }

        const auto existing = _before.by_full.find(child.name);

        if (existing != _before.by_full.end())
        {
            row* earlier = find_row(_into, existing->second);

            if (earlier != nullptr)
            {
                take_definition(*earlier, child);
                ++_out.updated;
                _out.ids.push_back(earlier->id);

                continue;
            }
        }

        std::string prefix;
        std::string suffix;

        affixes_inside(_into, _before, _parent, prefix, suffix);

        row made = child;

        // the name made local, so the macro name stays what it was
        if ( (made.name.size() > prefix.size() + suffix.size()) &&
             (made.name.compare(0, prefix.size(), prefix) == 0) &&
             (made.name.compare(made.name.size() - suffix.size(),
                                suffix.size(),
                                suffix) == 0) )
        {
            made.name = made.name.substr(prefix.size(),
                                         made.name.size() - prefix.size() -
                                             suffix.size());
        }
        else if ( (!prefix.empty()) ||
                  (!suffix.empty()) )
        {
            _out.notes.push_back(child.name + " does not have the affixes "
                                 "of where it goes; its macro name becomes " +
                                 prefix + child.name + suffix);
        }

        const int id = insert_row(_into, _parent, _index, made);

        if (_index >= 0)
        {
            ++_index;
        }

        ++_out.added;
        _out.ids.push_back(id);
    }

    return;
}

}  // anonymous namespace


imported
import_code(
    std::string_view      _text,
    const import_options& _options
)
{
    imported out;
    importer pass(_text, _options, out);

    pass.run();

    return out;
}

bool
looks_like_sheet(
    std::string_view _text
)
{
    std::size_t at = 0;

    // a byte-order mark first
    if ( (_text.size() >= 3)                       &&
         (static_cast<unsigned char>(_text[0]) == 0xEFu) &&
         (static_cast<unsigned char>(_text[1]) == 0xBBu) &&
         (static_cast<unsigned char>(_text[2]) == 0xBFu) )
    {
        at = 3;
    }

    while ( (at < _text.size()) &&
            (std::isspace(static_cast<unsigned char>(_text[at]))) )
    {
        ++at;
    }

    return ( (at < _text.size()) &&
             (_text[at] == '{') );
}

merged
merge_rows(
    sheet&       _into,
    int          _parent,
    const sheet& _from
)
{
    merged           out;
    const resolution before = resolve(_into);
    const row*       target = find_row(_into, _parent);
    int              parent = _parent;
    int              index  = -1;

    // a symbol: beside it, in its group
    if ( (target != nullptr) &&
         (target->kind == row_kind::symbol) )
    {
        const row* holder = parent_of(_into, _parent);

        parent = (holder != nullptr) ? holder->id : 0;

        if (holder != nullptr)
        {
            for (std::size_t i = 0; i < holder->children.size(); ++i)
            {
                if (holder->children[i].id == _parent)
                {
                    index = static_cast<int>(i) + 1;
                }
            }
        }
    }
    else if (target == nullptr)
    {
        parent = 0;
    }

    merge_into(_into, before, parent, index, _from.root, out);

    return out;
}


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
