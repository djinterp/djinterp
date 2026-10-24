/*******************************************************************************
* bitwisedom [bitwisedom]                                         cli_common.cpp
*
* What the command line's sources share: sheets read and written back, rows
* and values as a command line names them, and output.
*   A file is read and written through the kit -- `-` the standard stream,
* a write whole or not at all -- and its text read and written by the
* model, so the command line holds no format of its own. An integer is read
* as the grammar writes one, then handed to the model's reader in the form
* that reader takes, so a leading zero is never octal here as it is in C.
*   A change is judged by what it did to the sheet's errors, row by row: one
* that gave a row an error it did not have is refused unless forced, and
* every issue it brought is said either way.
*   The window's SHEET operands are taken here too, by read_source's rules,
* so that `bitwisedom gui` refuses what the commands refuse, in their words.
*
* path:      /src/djinterp/tools/bitwisedom/cli/cli_common.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.10.23
*******************************************************************************/
#include "./cli_internal.hpp"  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <algorithm>  // std::max, std::min
#include <cctype>     // std::isdigit, std::isxdigit, std::tolower
#include <cstddef>    // std::size_t
#include <cstdio>     // std::snprintf
#include <ctime>      // std::time, std::localtime
#include <map>        // std::map
#include <utility>    // std::move
// bitwisedom
#include <djinterp/tools/bitwisedom/bits.hpp>      // parse_wide
#include <djinterp/tools/bitwisedom/cli.hpp>       // window_sheet_of
#include <djinterp/tools/bitwisedom/codegen.hpp>   // generate, standard_name
#include <djinterp/tools/bitwisedom/examples.hpp>  // example_names
#include <djinterp/tools/bitwisedom/exchange.hpp>  // read_json_sheet
#include <djinterp/tools/bitwisedom/persist.hpp>   // save_sheet
#include <djinterp/tools/bitwisedom/version.hpp>   // BITWISEDOM_PROGRAM


namespace bitwisedom
{
namespace cmd
{

namespace
{

// EXAMPLE_PREFIX
//   constant: what a named operand that is a built-in example begins with
// (GRAMMAR.md, R13).
const char* const EXAMPLE_PREFIX = "example:";

// FORMATS
//   constant: --format's words.
const std::vector<std::string> FORMATS = { "text", "json" };

// errors_by_row
//   how many errors each row has, by its id; the sheet's own under 0.
std::map<int, int>
errors_by_row(
    const resolution& _resolution
)
{
    std::map<int, int> out;

    for (const issue& each : _resolution.issues)
    {
        out[each.row] += (each.level == severity::error) ? 1 : 0;
    }

    return out;
}

// write_to
//   text to a sink, when it has a writer and there is text.
void
write_to(
    const ::d_pack_sink& _sink,
    const std::string&   _text
)
{
    if ( (_sink.write != nullptr) &&
         (!_text.empty()) )
    {
        (void)_sink.write(_sink.context,
                          _text.data(),
                          _text.size());
    }

    return;
}

// distance
//   the edits -- a letter put in, taken out or changed -- from one word to
// another, as the parser's suggestions count them.
std::size_t
distance(
    const std::string& _from,
    const std::string& _to
)
{
    std::vector<std::size_t> above(_to.size() + 1u);
    std::vector<std::size_t> here(_to.size() + 1u);

    for (std::size_t j = 0; j <= _to.size(); ++j)
    {
        above[j] = j;
    }

    for (std::size_t i = 1; i <= _from.size(); ++i)
    {
        here[0] = i;

        for (std::size_t j = 1; j <= _to.size(); ++j)
        {
            const std::size_t change = above[j - 1u] +
                                       ((_from[i - 1u] == _to[j - 1u]) ? 0u
                                                                       : 1u);

            here[j] = std::min(std::min(above[j] + 1u,
                                        here[j - 1u] + 1u),
                               change);
        }

        above.swap(here);
    }

    return above[_to.size()];
}

/*
place_of
  A line of the model's that begins with where it is -- "line 3: ...",
"line 3, column 7: ..." -- split into the place and the rest; a line that
does not keeps its whole text, at no place.
*/
std::string
place_of(
    const std::string& _line,
    long&              _row,
    long&              _column
)
{
    _row    = 0;
    _column = 0;

    const std::string head = "line ";

    // no place
    if (_line.compare(0, head.size(), head) != 0)
    {
        return _line;
    }

    std::size_t at  = head.size();
    long        row = 0;

    while ( (at < _line.size()) &&
            (std::isdigit(static_cast<unsigned char>(_line[at]))) )
    {
        row = row * 10 + (_line[at++] - '0');
    }

    long column = 0;

    // a column after the line
    if (_line.compare(at, 9, ", column ") == 0)
    {
        at += 9u;

        while ( (at < _line.size()) &&
                (std::isdigit(static_cast<unsigned char>(_line[at]))) )
        {
            column = column * 10 + (_line[at++] - '0');
        }
    }

    // the colon that ends the place
    if ( (row <= 0) ||
         (_line.compare(at, 2, ": ") != 0) )
    {
        return _line;
    }

    _row    = row;
    _column = column;

    return _line.substr(at + 2u);
}

// digit_of
//   a digit's value in a base, -1 when it is none of that base's.
int
digit_of(
    char _digit,
    int  _base
)
{
    const int lower = std::tolower(static_cast<unsigned char>(_digit));
    const int value = ( (lower >= '0') && (lower <= '9') ) ? (lower - '0')
                    : ( (lower >= 'a') && (lower <= 'z') ) ? (lower - 'a' +
                                                              10)
                                                           : 99;

    return (value < _base) ? value : -1;
}

/*
base_of
  Where an integer's digits start, past its sign and its base's prefix, and
the base, with the prefix the model's reader takes for it.
*/
std::size_t
base_of(
    const std::string& _text,
    int&               _base,
    std::string&       _prefix
)
{
    std::size_t at = ( (!_text.empty()) &&
                       ( (_text[0] == '-') ||
                         (_text[0] == '+') ) ) ? 1u : 0u;

    _base   = 10;
    _prefix = "";

    // a base's prefix
    if ( (_text.size() > at + 1u) &&
         (_text[at] == '0') )
    {
        const char mark = static_cast<char>(std::tolower(
                              static_cast<unsigned char>(_text[at + 1u])));

        _base   = (mark == 'x') ? 16 : (mark == 'o') ? 8 : (mark == 'b') ? 2
                                                                         : 10;
        _prefix = (_base == 16) ? "0x" : (_base == 8) ? "0o"
                : (_base == 2)  ? "0b" : "";
        at     += (_base != 10) ? 2u : 0u;
    }

    return at;
}

/*
digits_of
  An integer's digits from `_at`, each of the base, a `_` only between two;
false, with why, at the first that is not.
*/
bool
digits_of(
    const std::string& _text,
    std::size_t        _at,
    int                _base,
    std::string&       _digits,
    std::string&       _error
)
{
    for (std::size_t at = _at; at < _text.size(); ++at)
    {
        const char each = _text[at];
        const bool apart = ( (each == '_')                         &&
                             (!_digits.empty())                    &&
                             (at + 1u < _text.size())              &&
                             (digit_of(_text[at + 1u], _base) >= 0) );

        if ( (each != '_') &&
             (digit_of(each, _base) >= 0) )
        {
            _digits += each;

            continue;
        }

        _error = apart ? std::string()
               : (each == '_')
                 ? "'" + _text + "' has a '_' that is not between two digits"
                 : "'" + _text + "' is not a number: '" +
                   std::string(1u, each) + "' is not a " +
                   ( (_base == 16) ? "hexadecimal"
                   : (_base == 8)  ? "octal"
                   : (_base == 2)  ? "binary"
                                   : "decimal" ) + " digit";

        if (!apart)
        {
            return false;
        }
    }

    return true;
}

/*
which_rows
  The rows a name is several rows' own name of, by their macro names, the
first three, as a usage error lists them.
*/
std::string
which_rows(
    const std::vector<const resolved_row*>& _named
)
{
    std::string which;

    for (std::size_t i = 0; ( (i < _named.size()) && (i < 3u) ); ++i)
    {
        which += (i == 0u) ? "" : ", ";
        which += shown_name(*_named[i]);
    }

    return which + ((_named.size() > 3u) ? " and more" : "");
}

/*
example_place
  An example's place in the list, by name, checked against it, since
make_example gives the first for a name it does not know: one it does not
know said, with the nearest, and where the examples are listed -- by
bitwisedom-gen's own option, for its users -- and -1.
*/
int
example_place(
    kit::program&      _program,
    const std::string& _name
)
{
    const std::vector<std::string> names   = example_names();
    const std::string              program = _program.kit().name;
    const int                      place   = choice_of(_name,
                                                       names);

    if (place >= 0)
    {
        return place;
    }

    const std::string near = nearest(_name, names);

    _program.report(kit::severity::error,
                    "'" + _name + "' is not a built-in example");

    if (!near.empty())
    {
        _program.report(kit::severity::note,
                        "did you mean 'example:" + near + "'?");
    }

    _program.report(kit::severity::note,
                    (program == BITWISEDOM_PROGRAM)
                        ? "'bitwisedom examples' lists them"
                        : "'" + program + " --list-examples' lists them");

    return -1;
}

/*
read_example
  An example by name, once example_place knows it.
*/
bool
read_example(
    kit::program&      _program,
    const std::string& _name,
    source&            _source
)
{
    if (example_place(_program, _name) < 0)
    {
        return false;
    }

    _source.doc     = make_example(_name);
    _source.example = true;

    return true;
}

}  // anonymous namespace


/*
read_source
  An example by name; otherwise the file, read whole, then any of the three
shapes, its notes and its error placed in it.
*/
bool
read_source(
    kit::program&      _program,
    const std::string& _operand,
    source&            _source
)
{
    _source         = source();
    _source.operand = _operand;

    const std::string prefix = EXAMPLE_PREFIX;

    // a built-in example
    if (_operand.compare(0, prefix.size(), prefix) == 0)
    {
        return read_example(_program, _operand.substr(prefix.size()),
                            _source);
    }

    std::string text;
    const int   failed = kit::read_file(_operand,
                                        text);

    // nothing to read
    if (failed != 0)
    {
        (void)_program.file_error("read",
                                  _operand,
                                  failed);

        return false;
    }

    json_read read = read_json_sheet(text);

    report_lines(_program,
                 static_cast<int>(kit::severity::note),
                 _operand,
                 read.notes);

    // not a sheet, a bitfield or an export
    if (!read.ok)
    {
        report_lines(_program,
                     static_cast<int>(kit::severity::error),
                     _operand,
                     std::vector<std::string>(1u, read.error));

        return false;
    }

    _source.doc   = std::move(read.result);
    _source.shape = read.shape;

    return true;
}

/*
add_destination
  The two options, bound to the destination.
*/
void
add_destination(
    kit::command _command,
    destination& _destination
)
{
    _command.add_option("-o,--out",
                        _destination.out,
                        "write the sheet to FILE, - for standard output, "
                        "and leave the one read as it was")
            .type_name("FILE");
    _command.add_flag("-n,--dry-run",
                      _destination.dry_run,
                      "write nothing; say what would change");
    _command.add_flag("-f,--force",
                      _destination.force,
                      "write it even if the change gives the sheet errors");

    return;
}

/*
add_sheet
  The operand, as every changing command takes it.
*/
void
add_sheet(
    kit::command _command,
    std::string& _sheet
)
{
    _command.add_option("SHEET",
                        _sheet,
                        "the sheet, written back where it was read; - for "
                        "standard input, written to standard output")
            .required();

    return;
}

/*
sheet_text
  The model's writer for the shape; an export with its header, as the
generator writes it on the date given.
*/
std::string
sheet_text(
    const sheet&       _sheet,
    json_shape         _shape,
    const std::string& _date
)
{
    switch (_shape)
    {
        case json_shape::bitfield:
            return write_bitfield(_sheet);

        case json_shape::everything:
        {
            const resolution resolved = resolve(_sheet);

            return write_export(_sheet,
                                resolved,
                                generate(_sheet,
                                         resolved,
                                         _date));
        }

        case json_shape::sheet:
            break;
    }

    return save_sheet(_sheet);
}

/*
new_errors
  Each row's errors after, less those it had before, where it has more.
*/
int
new_errors(
    const resolution& _before,
    const resolution& _after
)
{
    const std::map<int, int> had   = errors_by_row(_before);
    int                      added = 0;

    for (const auto& each : errors_by_row(_after))
    {
        const auto old = had.find(each.first);

        added += std::max(0,
                          each.second -
                          ( (old == had.end()) ? 0 : old->second ));
    }

    return added;
}

/*
store
  Back where it came from unless told otherwise -- an example has no such
place -- written whole by the kit; refused when it gives the sheet errors
and was not forced; the change said once it is done, or, in a dry run,
instead.
*/
int
store(
    kit::program&      _program,
    kit::command       _command,
    const source&      _source,
    const destination& _destination,
    const std::string& _change,
    const resolution&  _before
)
{
    const std::string target = (!_destination.out.empty())
                               ? _destination.out
                               : _source.operand;

    // a built-in example, with nowhere to go back to
    if ( (_destination.out.empty()) &&
         (_source.example) )
    {
        return _program.usage_error(_command,
                                    _source.operand + " is built in: give "
                                    "-o FILE to write it, changed");
    }

    const resolution after = resolve(_source.doc);
    const int        worse = new_errors(_before,
                                        after);

    // what the change brought
    report_new_issues(_program,
                      _source.operand,
                      _before,
                      after);

    // errors it did not have: refused, unless forced
    if ( (worse > 0) &&
         (!_destination.force) )
    {
        _program.report(kit::severity::error,
                        "nothing was written: it would give the sheet " +
                        plural(worse, "new error") +
                        "; --force writes it anyway");

        return D_CLI_EXIT_TROUBLE;
    }

    // nothing written
    if (_destination.dry_run)
    {
        _program.report(kit::severity::note,
                        _change + " (a dry run: nothing was written)");

        return D_CLI_EXIT_OK;
    }

    const int failed = kit::write_file(target,
                                       sheet_text(_source.doc,
                                                  _source.shape,
                                                  today()));

    if (failed != 0)
    {
        return _program.file_error("write",
                                   target,
                                   failed);
    }

    _program.note(1,
                  _change);

    return D_CLI_EXIT_OK;
}

/*
row_named
  A macro name first, as the model's lookup takes one; then a row's own
name, which must be one row's -- a command line refuses to guess, where a
row's expression would take the nearest sibling's.
*/
int
row_named(
    kit::program&      _program,
    kit::command       _command,
    const resolution&  _resolution,
    const std::string& _name,
    const std::string& _option
)
{
    const auto exact = _resolution.by_full.find(_name);

    // a macro name
    if (exact != _resolution.by_full.end())
    {
        return exact->second;
    }

    std::vector<const resolved_row*> named;
    std::vector<std::string>         known;

    for (const resolved_row& each : _resolution.rows)
    {
        if (each.local == _name)
        {
            named.push_back(&each);
        }

        known.push_back(each.local);

        if (each.kind == row_kind::symbol)
        {
            known.push_back(each.full);
        }
    }

    // one row's own name
    if (named.size() == 1u)
    {
        return named.front()->id;
    }

    const std::string place = (!_option.empty()) ? (_option + ": ") : "";

    if (named.size() > 1u)
    {
        (void)_program.usage_error(_command,
                                   place + "'" + _name + "' names " +
                                   std::to_string(named.size()) +
                                   " rows -- " + which_rows(named) +
                                   " -- name one by its macro name");

        return 0;
    }

    const std::string near = nearest(_name, known);

    (void)_program.usage_error(_command,
                               place + "no row is named '" + _name + "'" +
                               (near.empty()
                                    ? std::string()
                                    : "; did you mean '" + near + "'?"));

    return 0;
}

/*
shown_name
  A symbol by the name the header defines, a group by its own.
*/
std::string
shown_name(
    const resolved_row& _row
)
{
    return (_row.kind == row_kind::symbol) ? _row.full
                                           : _row.local;
}

/*
read_value
  The grammar's integer checked here -- its sign, its base's prefix, its
digits, its separators -- then given to the model's reader in a form it
reads the same way: hexadecimal, octal and binary with their prefixes, and
decimal with its leading zeros gone, which C would read as octal.
*/
bool
read_value(
    const std::string& _text,
    wide&              _value,
    bool&              _negative,
    std::string&       _error
)
{
    int               base   = 10;
    std::string       prefix;
    std::string       digits;
    const std::size_t at     = base_of(_text, base, prefix);

    _negative = ( (!_text.empty()) &&
                  (_text[0] == '-') );

    // the base read first: an argument's order is the compiler's to choose
    if (!digits_of(_text, at, base, digits, _error))
    {
        return false;
    }

    // no digits at all
    if (digits.empty())
    {
        _error = "'" + _text + "' is not a number";

        return false;
    }

    // decimal: its leading zeros gone, so the reader does not see octal
    if (base == 10)
    {
        const std::size_t first = digits.find_first_not_of('0');

        digits = (first == std::string::npos) ? std::string("0")
                                              : digits.substr(first);
    }

    const parsed_wide read = parse_wide((_negative ? "-" : "") + prefix +
                                        digits);

    if (!read.ok)
    {
        _error = "'" + _text + "': " + read.error;

        return false;
    }

    _value = read.value;

    return true;
}

/*
read_number
  read_value, then held to what 64 bits hold as a signed number.
*/
bool
read_number(
    const std::string& _text,
    i64&               _value
)
{
    wide        value;
    bool        negative = false;
    std::string error;

    // not a number at all
    if (!read_value(_text,
                    value,
                    negative,
                    error))
    {
        return false;
    }

    // past 63 bits, unless negative, whose bits are its 64-bit pattern
    if ( (!value.fits(64)) ||
         ( (!negative) &&
           (value.test(63)) ) )
    {
        return false;
    }

    _value = static_cast<i64>(value.low());

    return true;
}

/*
choice_of / word_list / nearest
  A search; a join; and the parser's own rule for a suggestion: no more
edits than a third of the word, and at least one.
*/
int
choice_of(
    const std::string&              _word,
    const std::vector<std::string>& _words
)
{
    for (std::size_t i = 0; i < _words.size(); ++i)
    {
        if (_words[i] == _word)
        {
            return static_cast<int>(i);
        }
    }

    return -1;
}

std::string
word_list(
    const std::vector<std::string>& _words
)
{
    std::string out;

    for (std::size_t i = 0; i < _words.size(); ++i)
    {
        out += (i == 0u)                    ? ""
             : (i + 1u == _words.size())   ? " or "
                                           : ", ";
        out += _words[i];
    }

    return out;
}

std::string
nearest(
    const std::string&              _word,
    const std::vector<std::string>& _words
)
{
    const std::size_t most     = std::max<std::size_t>(1u,
                                                       _word.size() / 3u);
    std::size_t       best     = most + 1u;
    std::string       found;
    std::string       begun;
    std::size_t       begins   = 0u;

    for (const std::string& each : _words)
    {
        const std::size_t edits = distance(_word, each);

        if (edits < best)
        {
            best  = edits;
            found = each;
        }

        // a word the mistyped one begins, as an abbreviation would
        if ( (!_word.empty()) &&
             (each.compare(0, _word.size(), _word) == 0) )
        {
            begun   = each;
            begins += 1u;
        }
    }

    return (best <= most)  ? found
         : (begins == 1u)  ? begun
                           : std::string();
}

/*
bad_word
  The words the option takes, and the nearest, in one usage error.
*/
int
bad_word(
    kit::program&                   _program,
    kit::command                    _command,
    const std::string&              _option,
    const std::string&              _word,
    const std::vector<std::string>& _words
)
{
    const std::string near = nearest(_word,
                                     _words);

    return _program.usage_error(_command,
                                _option + ": '" + _word + "' is not " +
                                word_list(_words) +
                                (near.empty()
                                     ? std::string()
                                     : "; did you mean '" + near + "'?"));
}

/*
level_words
  Each standard's name in lower case, `+` as `p`: C++17 is cpp17.
*/
std::vector<std::string>
level_words()
{
    std::vector<std::string> out;

    for (int i = 0; i < STANDARD_COUNT; ++i)
    {
        std::string word;

        for (const char* at = standard_name(static_cast<standard>(i));
             *at != '\0';
             ++at)
        {
            word += (*at == '+')
                    ? 'p'
                    : static_cast<char>(std::tolower(
                          static_cast<unsigned char>(*at)));
        }

        out.push_back(word);
    }

    return out;
}

/*
given
  The parser's count of the option, by whichever name finds it.
*/
bool
given(
    kit::command       _command,
    const std::string& _option
)
{
    return (_command.count(_option) > 0u);
}

/*
table
  Each column as wide as its widest cell; the last cell of a line is left as
it is, and a line's trailing blanks go.
*/
std::string
table(
    const std::vector<std::vector<std::string>>& _rows
)
{
    std::vector<std::size_t> widths;

    for (const std::vector<std::string>& each : _rows)
    {
        widths.resize(std::max(widths.size(), each.size()), 0u);

        for (std::size_t i = 0; i < each.size(); ++i)
        {
            widths[i] = std::max(widths[i], each[i].size());
        }
    }

    std::string out;

    for (const std::vector<std::string>& each : _rows)
    {
        std::string line;

        for (std::size_t i = 0; i < each.size(); ++i)
        {
            line += each[i];

            if (i + 1u < each.size())
            {
                line += std::string(widths[i] - each[i].size() + 2u, ' ');
            }
        }

        // no blanks at the end of a line
        line.erase(line.find_last_not_of(' ') + 1u);
        out += line + "\n";
    }

    return out;
}

/*
plural
  The noun with an s for every count but one.
*/
std::string
plural(
    long        _count,
    const char* _noun
)
{
    return std::to_string(_count) + " " + _noun +
           ((_count == 1) ? "" : "s");
}

/*
emit
  The program's output sink, which is standard output unless a test gave
the program another.
*/
void
emit(
    kit::program&      _program,
    const std::string& _text
)
{
    write_to(_program.kit().out,
             _text);

    return;
}

/*
say
  The program's error sink, as emit writes its output's.
*/
void
say(
    kit::program&      _program,
    const std::string& _text
)
{
    write_to(_program.kit().err,
             _text);

    return;
}

/*
add_format / json_wanted
  One option, --format; and its word looked up, a usage error for any other.
*/
void
add_format(
    kit::command _command,
    std::string& _format
)
{
    _command.add_option("--format",
                        _format,
                        "the answer as text or as json")
            .type_name("FORMAT")
            .default_str("text");

    return;
}

bool
json_wanted(
    kit::program&      _program,
    kit::command       _command,
    const std::string& _format,
    bool&              _ok
)
{
    const int index = choice_of(_format,
                                FORMATS);

    _ok = (index >= 0);

    if (!_ok)
    {
        (void)bad_word(_program,
                       _command,
                       "--format",
                       _format,
                       FORMATS);
    }

    return (index == 1);
}

/*
report_issues
  Each issue at the sheet, named for its row -- a symbol by its macro name,
a group by its own -- with the kit's severity for the model's.
*/
int
report_issues(
    kit::program&      _program,
    const std::string& _operand,
    const resolution&  _resolution
)
{
    int errors = 0;

    for (const issue& each : _resolution.issues)
    {
        const resolved_row* where = _resolution.find(each.row);
        const std::string   named = (where == nullptr)
                                    ? std::string()
                                    : ( (where->kind == row_kind::symbol)
                                        ? where->full
                                        : where->local ) + ": ";
        const kit::severity::value level =
            (each.level == severity::error)   ? kit::severity::error
          : (each.level == severity::warning) ? kit::severity::warning
                                              : kit::severity::note;

        _program.report_at(level,
                           _operand,
                           0L,
                           0L,
                           named + each.message);
        errors += (each.level == severity::error) ? 1 : 0;
    }

    return errors;
}

/*
report_new_issues
  An issue is the same before and after when its row, its severity and its
words are; ids survive every edit but a row's removal, so a moved or
renamed row's old issues are not new.
*/
void
report_new_issues(
    kit::program&      _program,
    const std::string& _operand,
    const resolution&  _before,
    const resolution&  _after
)
{
    resolution fresh = _after;

    fresh.issues.clear();

    for (const issue& each : _after.issues)
    {
        bool seen = false;

        for (const issue& old : _before.issues)
        {
            seen = (seen) ||
                   ( (old.row == each.row)         &&
                     (old.level == each.level)     &&
                     (old.message == each.message) );
        }

        if (!seen)
        {
            fresh.issues.push_back(each);
        }
    }

    (void)report_issues(_program,
                        _operand,
                        fresh);

    return;
}

/*
report_lines
  Each line at its place in the file, where it gives one.
*/
void
report_lines(
    kit::program&                   _program,
    int                             _severity,
    const std::string&              _file,
    const std::vector<std::string>& _lines
)
{
    for (const std::string& each : _lines)
    {
        long              row    = 0;
        long              column = 0;
        const std::string rest   = place_of(each,
                                            row,
                                            column);

        _program.report_at(static_cast<kit::severity::value>(_severity),
                           _file,
                           row,
                           column,
                           rest);
    }

    return;
}

/*
today
  The local date; a build that must write the same bytes every day gives
the date itself.
*/
std::string
today()
{
    const std::time_t now   = std::time(nullptr);
    const std::tm*    local = std::localtime(&now);
    char              text[48];

    // no local time to be had
    if (local == nullptr)
    {
        return "2026.01.01";
    }

    (void)std::snprintf(text,
                        sizeof(text),
                        "%04d.%02d.%02d",
                        local->tm_year + 1900,
                        local->tm_mon + 1,
                        local->tm_mday);

    return text;
}

}  // namespace cmd


/*
window_sheet_of
  read_source's rules for what an operand is, kept here beside them: an
example by its place, checked as read_source checks one; a file read once,
to know that the window can read it, and said as read_source says one it
cannot; and `-` refused, since a window has no standard input to read.
*/
bool
window_sheet_of(
    cmd::kit::program& _program,
    const std::string& _operand,
    window_sheet&      _sheet
)
{
    const std::string prefix = cmd::EXAMPLE_PREFIX;
    std::string       text;

    _sheet = window_sheet();

    // standard input, which the window cannot read
    if (cmd::kit::is_standard(_operand))
    {
        (void)_program.usage_error("the window opens sheet files and "
                                   "examples, not standard input");

        return false;
    }

    // a built-in example, by its place
    if (_operand.compare(0, prefix.size(), prefix) == 0)
    {
        _sheet.example = cmd::example_place(_program,
                                            _operand.substr(prefix.size()));

        return (_sheet.example >= 0);
    }

    const int failed = cmd::kit::read_file(_operand,
                                           text);

    // a file the window could not read either
    if (failed != 0)
    {
        (void)_program.file_error("read",
                                  _operand,
                                  failed);

        return false;
    }

    _sheet.path = _operand;

    return true;
}

}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
