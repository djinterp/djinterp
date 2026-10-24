/*******************************************************************************
* bitwisedom [bitwisedom]                                           cli_read.cpp
*
* The commands that read a sheet: gen, check, show, decode, eval, convert and
* examples.
*   Each is a request -- the operands and options the parse filled in -- and
* a function that does the work, the request owned by the work, so that it
* lives as long as the program does. A command whose answer has structure
* gives it as text a person reads, or with `--format=json` as JSON a script
* reads (GRAMMAR.md, R19); the JSON's keys only ever grow.
*   A command that answers a question says no with exit status 1: gen and
* check for a sheet with errors, decode for a value that sets bits no symbol
* names, eval for an expression that does not evaluate.
*   An option counts as given when it is on the command line at all: `--date=`
* is a date that is not one, never today's.
*
* path:      /src/djinterp/tools/bitwisedom/cli/cli_read.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.10.22
*******************************************************************************/
#include "./cli_internal.hpp"  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <algorithm>  // std::max
#include <cctype>     // std::isdigit
#include <cstddef>    // std::size_t
#include <memory>     // std::make_shared
#include <string>     // std::string
#include <utility>    // std::move
#include <vector>     // std::vector
// bitwisedom
#include <djinterp/tools/bitwisedom/bits.hpp>      // format_hex, runs_text
#include <djinterp/tools/bitwisedom/codegen.hpp>   // generate
#include <djinterp/tools/bitwisedom/examples.hpp>  // example_list
#include <djinterp/tools/bitwisedom/exchange.hpp>  // write_bitfield
#include <djinterp/tools/bitwisedom/expr.hpp>      // parse_expression
#include <djinterp/tools/bitwisedom/json.hpp>      // json_value, write_json
#include <djinterp/tools/bitwisedom/resolve.hpp>   // resolve, wide_constant
#include <djinterp/tools/bitwisedom/selftest.hpp>  // self_test


namespace bitwisedom
{
namespace cmd
{

namespace
{

// LANGUAGES / FORMS / STYLES / WORDS / SHAPES
//   constants: the words of gen's and convert's choices, each in its
// enumeration's order.
const std::vector<std::string> LANGUAGES = { "c", "cpp" };
const std::vector<std::string> FORMS     = { "macros", "constants" };
const std::vector<std::string> STYLES    = { "djinterp", "plain" };
const std::vector<std::string> WORDS     = { "8", "16", "32", "64" };
const std::vector<std::string> SHAPES    = { "sheet", "bitfield", "export" };

// INDENT
//   constant: what a row of decode's or eval's table starts with, under the
// line it belongs to.
const char* const INDENT = "    ";

// hex_of
//   bits as the command line writes a value: 0x and upper-case digits, as
// many as the sheet's width needs.
std::string
hex_of(
    const wide& _bits,
    int         _width
)
{
    return "0x" + format_hex(_bits,
                             (std::max(_width, 1) + 3) / 4,
                             true);
}

// binary_of
//   bits as 0b and binary digits, highest first, `_` every four from the
// lowest -- a form the command line reads back.
std::string
binary_of(
    const wide& _bits
)
{
    const int   highest = std::max(_bits.highest(), 0);
    std::string out     = "0b";

    for (int i = highest; i >= 0; --i)
    {
        out += _bits.test(i) ? '1' : '0';
        out += ( (i > 0) && ((i % 4) == 0) ) ? "_" : "";
    }

    return out;
}

// kind_word
//   a row's kind as the command line names it.
const char*
kind_word(
    const resolved_row& _row
)
{
    if (_row.kind == row_kind::group)
    {
        return "group";
    }

    switch (_row.def)
    {
        case def_kind::bit:         return "bit";
        case def_kind::range:       return "range";
        case def_kind::field_value: return "value";
        case def_kind::literal:     return "literal";
        case def_kind::expression:  return "expression";
        case def_kind::macro:       return "macro";
    }

    return "symbol";
}

// date_valid
//   whether a date is a banner's: YYYY.MM.DD.
bool
date_valid(
    const std::string& _date
)
{
    bool valid = (_date.size() == 10u);

    // four digits, a point, two, a point, two
    for (std::size_t i = 0; ( valid && (i < _date.size()) ); ++i)
    {
        const bool point = ( (i == 4u) || (i == 7u) );

        valid = point ? (_date[i] == '.')
                      : (std::isdigit(static_cast<unsigned char>(_date[i])) !=
                         0);
    }

    return valid;
}

// date_given
//   the banner's date: --date's, checked, or today's; a usage error, said,
// for one that is not a date.
int
date_given(
    kit::program&      _program,
    kit::command       _command,
    const std::string& _date,
    std::string&       _out
)
{
    if (!given(_command, "--date"))
    {
        _out = today();

        return D_CLI_EXIT_OK;
    }

    _out = _date;

    return date_valid(_date)
           ? D_CLI_EXIT_OK
           : _program.usage_error(_command,
                                  "--date: '" + _date + "' is not a date "
                                  "written YYYY.MM.DD");
}

// file_name_of
//   a path's last part, as a header names itself.
std::string
file_name_of(
    const std::string& _path
)
{
    const std::size_t slash = _path.find_last_of("/\\");

    return (slash == std::string::npos) ? _path : _path.substr(slash + 1u);
}

// write_or_say
//   a file written whole, or the error of not writing it; true when written.
bool
write_or_say(
    kit::program&      _program,
    const std::string& _path,
    const std::string& _text,
    int&               _status
)
{
    const int failed = kit::write_file(_path,
                                       _text);

    _status = (failed == 0) ? _status
                            : _program.file_error("write",
                                                  _path,
                                                  failed);

    return (failed == 0);
}

// json_or_text
//   whether --format asked for JSON; false, with `_status` the usage error
// said, for a word it does not take.
bool
json_or_text(
    kit::program&      _program,
    kit::command       _command,
    const std::string& _format,
    bool&              _json
)
{
    bool ok = true;

    _json = json_wanted(_program, _command, _format, ok);

    return ok;
}


//------------------------------------------------------------------------------
// gen's settings
//------------------------------------------------------------------------------

/*
word_given
  The place of an option's word among its words; -1 when the option was not
given; and for a word it does not take, -1 with `_status` the usage error
said. Nothing is looked at once `_status` says a mistake was found.
*/
int
word_given(
    kit::program&                   _program,
    kit::command                    _command,
    const char*                     _option,
    const std::string&              _word,
    const std::vector<std::string>& _words,
    int&                            _status
)
{
    if ( (_status != D_CLI_EXIT_OK) ||
         (!given(_command, _option)) )
    {
        return -1;
    }

    const int index = choice_of(_word, _words);

    _status = (index >= 0)
              ? _status
              : bad_word(_program, _command, _option, _word, _words);

    return index;
}

/*
apply_settings
  The command line's generator settings over the sheet's, each word checked
before any is applied; then a C++ level means a C++ header, as it always
has.
*/
int
apply_settings(
    kit::program&      _program,
    kit::command       _command,
    const gen_request& _request,
    gen_settings&      _gen
)
{
    const std::vector<std::string> levels = level_words();
    int                            status = D_CLI_EXIT_OK;
    const int lang  = word_given(_program, _command, "--lang", _request.lang,
                                 LANGUAGES, status);
    const int form  = word_given(_program, _command, "--form", _request.form,
                                 FORMS, status);
    const int style = word_given(_program, _command, "--style",
                                 _request.style, STYLES, status);
    const int level = word_given(_program, _command, "--level",
                                 _request.level, levels, status);
    const int word  = word_given(_program, _command, "--word", _request.word,
                                 WORDS, status);

    if (status != D_CLI_EXIT_OK)
    {
        return status;
    }

    _gen.lang  = (lang < 0)  ? _gen.lang
               : (lang == 1) ? language::cpp
                             : language::c;
    _gen.form  = (form < 0) ? _gen.form : static_cast<gen_form>(form);
    _gen.style = (style < 0) ? _gen.style : static_cast<gen_style>(style);
    _gen.level = (level < 0) ? _gen.level : static_cast<standard>(level);
    _gen.word  = (word < 0) ? _gen.word : (8 << word);

    // a C++ level means a C++ header
    _gen.lang  = is_cpp(_gen.level) ? language::cpp : _gen.lang;

    return D_CLI_EXIT_OK;
}


//------------------------------------------------------------------------------
// check
//------------------------------------------------------------------------------

// check_request
//   struct: what `check` was asked.
struct check_request
{
    std::vector<std::string> sheets;
    std::string              format = "text";
};

// counts
//   struct: a resolution's issues, counted by severity.
struct counts
{
    int errors   = 0;
    int warnings = 0;
    int notes    = 0;
};

// count_issues
//   a resolution's issues, counted.
counts
count_issues(
    const resolution& _resolution
)
{
    counts made;

    for (const issue& each : _resolution.issues)
    {
        made.errors   += (each.level == severity::error) ? 1 : 0;
        made.warnings += (each.level == severity::warning) ? 1 : 0;
        made.notes    += (each.level == severity::note) ? 1 : 0;
    }

    return made;
}

// issue_object
//   an issue as JSON.
json_value
issue_object(
    const resolution& _resolution,
    const issue&      _issue
)
{
    const resolved_row* where = _resolution.find(_issue.row);
    json_value          made  = json_value::object();

    made.set("severity",
             json_value::string((_issue.level == severity::error)
                                ? "error"
                                : (_issue.level == severity::warning)
                                  ? "warning"
                                  : "note"));
    made.set("row",
             json_value::string((where != nullptr) ? shown_name(*where)
                                                   : std::string()));
    made.set("message",
             json_value::string(_issue.message));

    return made;
}

// sheet_object
//   a sheet's issues as check's JSON gives them.
json_value
sheet_object(
    const std::string& _operand,
    const resolution&  _resolution,
    const counts&      _found
)
{
    json_value made   = json_value::object();
    json_value issues = json_value::array();

    for (const issue& one : _resolution.issues)
    {
        issues.push(issue_object(_resolution, one));
    }

    made.set("sheet", json_value::string(_operand));
    made.set("errors", json_value::number(_found.errors));
    made.set("warnings", json_value::number(_found.warnings));
    made.set("notes", json_value::number(_found.notes));
    made.set("issues", std::move(issues));

    return made;
}

/*
check_one
  One sheet: its issues as diagnostics and its line on the output, or its
object added to the JSON; trouble for one that cannot be read, no for one
with errors.
*/
int
check_one(
    kit::program&      _program,
    const std::string& _operand,
    bool               _json,
    json_value&        _all
)
{
    source read;

    if (!read_source(_program, _operand, read))
    {
        return D_CLI_EXIT_TROUBLE;
    }

    const resolution resolved = resolve(read.doc);
    const counts     found    = count_issues(resolved);

    if (_json)
    {
        _all.push(sheet_object(_operand, resolved, found));
    }
    else
    {
        (void)report_issues(_program, _operand, resolved);
        emit(_program,
             _operand + ": " +
             ( resolved.issues.empty()
               ? std::string("no issues")
               : plural(found.errors, "error") + ", " +
                 plural(found.warnings, "warning") + ", " +
                 plural(found.notes, "note") ) + "\n");
    }

    return (found.errors > 0) ? D_CLI_EXIT_NO : D_CLI_EXIT_OK;
}

/*
run_check
  Every sheet, one that cannot be read said and passed over: trouble, the
worst of the answers, comes first, then no.
*/
int
run_check(
    kit::program&        _program,
    kit::command         _command,
    const check_request& _request
)
{
    bool       json   = false;
    int        status = D_CLI_EXIT_OK;
    json_value all    = json_value::array();

    if (!json_or_text(_program, _command, _request.format, json))
    {
        return D_CLI_EXIT_TROUBLE;
    }

    for (const std::string& each : _request.sheets)
    {
        status = std::max(status, check_one(_program, each, json, all));
    }

    if (json)
    {
        emit(_program, write_json(all));
    }

    return status;
}


//------------------------------------------------------------------------------
// show
//------------------------------------------------------------------------------

// show_request
//   struct: what `show` was asked.
struct show_request
{
    std::string sheet;
    std::string format = "text";
};

// value_text
//   the integer C knows a row by, as the command line writes one: in hex at
// the sheet's width; a wide sheet's as wide_constant gives it; "-" for none.
std::string
value_text(
    const sheet&        _sheet,
    const resolved_row& _row
)
{
    // nothing to say
    if ( (_row.kind != row_kind::symbol) ||
         (!_row.error.empty())           ||
         (!_row.has_value) )
    {
        return "-";
    }

    // a narrow sheet: the value is its bits
    if (!is_wide(_sheet.width))
    {
        return hex_of(_row.bits, _sheet.width);
    }

    const c_constant known = wide_constant(_row);

    return (!known.ok)      ? std::string("-")
         : (known.negative) ? format_signed(static_cast<i64>(known.value))
                            : ("0x" + format_hex(known.value, 1, true));
}

// bits_text
//   the bits a row sets or lives in, as a person reads them.
std::string
bits_text(
    const resolved_row& _row
)
{
    // a group, a macro, or a symbol in error
    if ( (_row.kind != row_kind::symbol)  ||
         (_row.def == def_kind::macro)    ||
         (!_row.error.empty()) )
    {
        return "-";
    }

    // a bit, a range, a value: where it lives
    return (_row.span_lsb >= 0)
           ? runs_text(wide::run(_row.span_lsb, _row.span_width), 4)
           : runs_text(_row.bits, 4);
}

// row_object
//   a row as show's JSON gives it.
json_value
row_object(
    const sheet&        _sheet,
    const resolved_row& _row
)
{
    json_value made = json_value::object();

    made.set("name", json_value::string(shown_name(_row)));
    made.set("local", json_value::string(_row.local));
    made.set("kind", json_value::string(kind_word(_row)));
    made.set("depth", json_value::number(_row.depth));
    made.set("lsb", json_value::number(_row.span_lsb));
    made.set("width", json_value::number(_row.span_width));
    made.set("bits",
             json_value::string(_row.has_value
                                ? hex_of(_row.bits, _sheet.width)
                                : std::string()));
    made.set("value", json_value::string(value_text(_sheet, _row)));
    made.set("error", json_value::string(_row.error));

    return made;
}

/*
run_show
  The rows in the sheet's own order, each indented under its group: its
name, kind, bits and value, and its error where it has one.
*/
int
run_show(
    kit::program&       _program,
    kit::command        _command,
    const show_request& _request
)
{
    bool   json = false;
    source read;

    if ( (!json_or_text(_program, _command, _request.format, json)) ||
         (!read_source(_program, _request.sheet, read)) )
    {
        return D_CLI_EXIT_TROUBLE;
    }

    const resolution                      resolved = resolve(read.doc);
    json_value                            rows     = json_value::array();
    std::vector<std::vector<std::string>> cells;

    for (const resolved_row& each : resolved.rows)
    {
        rows.push(row_object(read.doc, each));
        cells.push_back({ std::string(static_cast<std::size_t>(each.depth) *
                                      2u, ' ') + shown_name(each),
                          kind_word(each),
                          bits_text(each),
                          value_text(read.doc, each) +
                          (each.error.empty() ? std::string()
                                              : "  error: " + each.error) });
    }

    emit(_program, json ? write_json(rows) : table(cells));

    return D_CLI_EXIT_OK;
}


//------------------------------------------------------------------------------
// decode
//------------------------------------------------------------------------------

// decode_request
//   struct: what `decode` was asked.
struct decode_request
{
    std::string              sheet;
    std::vector<std::string> values;
    std::string              format = "text";
};

// decoded
//   struct: what a value holds, as it is found: the table's lines; the
// JSON's bits set, fields and symbols equal to the whole; the bits a bit
// or a range names; and what no symbol names, the answer's no.
struct decoded
{
    std::vector<std::vector<std::string>> lines;
    json_value                            bits   = json_value::array();
    json_value                            fields = json_value::array();
    json_value                            equals = json_value::array();
    wide                                  named;
    wide                                  unnamed;
};

/*
values_named
  The names a range's values have where the field holds what it holds.
*/
std::vector<std::string>
values_named(
    const resolution&   _resolution,
    const resolved_row& _range,
    const wide&         _field
)
{
    std::vector<std::string> names;

    for (const resolved_row& value : _resolution.rows)
    {
        if ( (value.kind == row_kind::symbol)       &&
             (value.def == def_kind::field_value)   &&
             (value.field == _range.id)             &&
             (value.error.empty())                  &&
             (value.bits.slice(_range.span_lsb,
                               _range.span_width) == _field) )
        {
            names.push_back(value.full);
        }
    }

    return names;
}

/*
decode_range
  A range's field in the value -- a line and an object -- unless it is zero
and zero has no name, which says nothing.
*/
void
decode_range(
    const resolution&   _resolution,
    const resolved_row& _range,
    const wide&         _value,
    decoded&            _made
)
{
    const wide field = _value.slice(_range.span_lsb, _range.span_width);
    const std::vector<std::string> names = values_named(_resolution, _range,
                                                        field);

    _made.named |= wide::run(_range.span_lsb, _range.span_width);

    if ( (!field.any()) && names.empty() )
    {
        return;
    }

    json_value  range = json_value::object();
    json_value  said  = json_value::array();
    std::string text  = hex_of(field, _range.span_width);

    for (const std::string& name : names)
    {
        text += " " + name;
        said.push(json_value::string(name));
    }

    _made.lines.push_back({ _range.full,
                            runs_text(wide::run(_range.span_lsb,
                                                _range.span_width), 4),
                            text });
    range.set("name", json_value::string(_range.full));
    range.set("value", json_value::string(hex_of(field, _range.span_width)));
    range.set("names", std::move(said));
    _made.fields.push(std::move(range));

    return;
}

/*
decode_row
  One symbol that resolved: a bit set, a range's field, or a literal or an
expression the whole value is.
*/
void
decode_row(
    const resolution&   _resolution,
    const resolved_row& _row,
    const wide&         _value,
    decoded&            _made
)
{
    if ( (_row.def == def_kind::bit) &&
         (_row.span_lsb >= 0) )
    {
        _made.named.set(_row.span_lsb);

        if (_value.test(_row.span_lsb))
        {
            _made.lines.push_back({ _row.full,
                                    "bit " + std::to_string(_row.span_lsb),
                                    "set" });
            _made.bits.push(json_value::string(_row.full));
        }
    }
    else if ( (_row.def == def_kind::range) &&
              (_row.span_lsb >= 0)          &&
              (_row.span_width > 0) )
    {
        decode_range(_resolution, _row, _value, _made);
    }
    else if ( ( (_row.def == def_kind::literal) ||
                (_row.def == def_kind::expression) ) &&
              (_row.has_value)                         &&
              (_row.bits == _value) )
    {
        _made.equals.push(json_value::string(_row.full));
    }

    return;
}

/*
decode_value
  Each symbol that resolved, decoded; then the symbols equal to the whole,
and the bits no bit or range covers.
*/
decoded
decode_value(
    const resolution& _resolution,
    const wide&       _value
)
{
    decoded made;

    for (const resolved_row& each : _resolution.rows)
    {
        if ( (each.kind == row_kind::symbol) &&
             (each.error.empty()) )
        {
            decode_row(_resolution, each, _value, made);
        }
    }

    made.unnamed = _value;

    for (int at = _value.next(0); at >= 0; at = _value.next(at + 1))
    {
        made.unnamed.set(at, (!made.named.test(at)));
    }

    for (const json_value& each : made.equals.items)
    {
        made.lines.push_back({ "=", each.text, "" });
    }

    if (made.unnamed.any())
    {
        made.lines.push_back({ "(no symbol)", runs_text(made.unnamed, 8),
                               "" });
    }

    return made;
}

/*
decoded_object
  What decode_value found, as decode's JSON gives it.
*/
json_value
decoded_object(
    const decoded& _found,
    const wide&    _value,
    int            _width
)
{
    json_value made = json_value::object();

    made.set("value", json_value::string(hex_of(_value, _width)));
    made.set("bits", _found.bits);
    made.set("fields", _found.fields);
    made.set("equals", _found.equals);
    made.set("unnamed",
             json_value::string(_found.unnamed.any()
                                ? hex_of(_found.unnamed, _width)
                                : std::string()));

    return made;
}

/*
read_values
  Every VALUE read before anything is decoded, so a mistake in the last is
found before any output.
*/
bool
read_values(
    kit::program&                   _program,
    kit::command                    _command,
    const std::vector<std::string>& _texts,
    std::vector<wide>&              _values
)
{
    for (const std::string& each : _texts)
    {
        wide        value;
        bool        negative = false;
        std::string error;

        if (!read_value(each, value, negative, error))
        {
            (void)_program.usage_error(_command, error);

            return false;
        }

        _values.push_back(value);
    }

    return true;
}

/*
run_decode
  Each value decoded: its line and its table, each table row indented under
it, or all of them as JSON; the answer is no when any sets a bit no symbol
names.
*/
int
run_decode(
    kit::program&         _program,
    kit::command          _command,
    const decode_request& _request
)
{
    bool              json = false;
    source            read;
    std::vector<wide> values;

    if ( (!json_or_text(_program, _command, _request.format, json))    ||
         (!read_source(_program, _request.sheet, read))                 ||
         (!read_values(_program, _command, _request.values, values)) )
    {
        return D_CLI_EXIT_TROUBLE;
    }

    const resolution resolved = resolve(read.doc);
    json_value       all      = json_value::array();
    int              status   = D_CLI_EXIT_OK;

    for (const wide& each : values)
    {
        decoded found = decode_value(resolved, each);

        status = found.unnamed.any() ? D_CLI_EXIT_NO : status;
        all.push(decoded_object(found, each, read.doc.width));

        for (std::vector<std::string>& line : found.lines)
        {
            line.front() = INDENT + line.front();
        }

        // text as it is found, so it comes in step with any diagnostic
        if (!json)
        {
            emit(_program, hex_of(each, read.doc.width) + "\n" +
                           table(found.lines));
        }
    }

    emit(_program, json ? write_json(all) : std::string());

    return status;
}


//------------------------------------------------------------------------------
// eval
//------------------------------------------------------------------------------

// eval_request
//   struct: what `eval` was asked.
struct eval_request
{
    std::string              sheet;
    std::vector<std::string> expressions;
    std::string              format = "text";
};

/*
symbol_bits
  A name in an expression: a macro name, or a symbol's own name where one
symbol has it -- as row_named takes them, but quietly, the evaluator saying
what went wrong.
*/
bool
symbol_bits(
    const resolution&  _resolution,
    const std::string& _name,
    wide&              _value,
    std::string&       _error
)
{
    const auto          exact = _resolution.by_full.find(_name);
    const resolved_row* found = (exact != _resolution.by_full.end())
                                ? _resolution.find(exact->second)
                                : nullptr;
    int                 count = 0;

    for (const resolved_row& each : _resolution.rows)
    {
        if ( (found == nullptr)                 &&
             (each.kind == row_kind::symbol)    &&
             (each.local == _name) )
        {
            ++count;
            found = (count == 1) ? &each : nullptr;
        }
    }

    _error = (count > 1)
             ? "'" + _name + "' names " + std::to_string(count) +
               " symbols; use the macro name"
           : (found == nullptr)
             ? "unknown name '" + _name + "'"
           : ( (!found->error.empty()) || (!found->has_value) )
             ? found->full + " has no value"
             : std::string();
    _value = _error.empty() ? found->bits : _value;

    return _error.empty();
}

/*
eval_one
  One expression parsed and evaluated over the sheet's symbols, within its
width: its object for the JSON, and its text -- hex, decimal while it fits
64 bits, binary, and the bits it sets -- or the error of one that does not
evaluate, said, and false.
*/
bool
eval_one(
    kit::program&      _program,
    const std::string& _text,
    const wide_lookup& _lookup,
    int                _width,
    json_value&        _object,
    std::string&       _shown
)
{
    const wide_evaluation found = evaluate_wide(parse_expression(_text),
                                                _lookup, _width);

    // not a value: the answer is no
    if (!found.ok)
    {
        _program.report(kit::severity::error,
                        "'" + _text + "': " + found.error);

        return false;
    }

    if (found.overshift)
    {
        _program.report(kit::severity::warning,
                        "'" + _text + "': a shift past the sheet's width "
                        "gave zero");
    }

    const bool        narrow  = found.value.fits(64);
    const std::string decimal = narrow ? format_dec(found.value.low())
                                       : std::string("-");

    _object = json_value::object();
    _object.set("expression", json_value::string(_text));
    _object.set("hex", json_value::string(hex_of(found.value, _width)));
    _object.set("decimal", json_value::string(narrow ? decimal
                                                     : std::string()));
    _object.set("binary", json_value::string(binary_of(found.value)));
    _object.set("bits", json_value::string(runs_text(found.value, 8)));
    _shown = _text + "\n" +
             table({ { INDENT + std::string("hex"),
                       hex_of(found.value, _width) },
                     { INDENT + std::string("decimal"), decimal },
                     { INDENT + std::string("binary"),
                       binary_of(found.value) },
                     { INDENT + std::string("bits"),
                       runs_text(found.value, 8) } });

    return true;
}

/*
run_eval
  Each expression evaluated in turn, its text written as it is found, or
all of them as JSON; the answer is no when any does not evaluate.
*/
int
run_eval(
    kit::program&       _program,
    kit::command        _command,
    const eval_request& _request
)
{
    bool   json = false;
    source read;

    if ( (!json_or_text(_program, _command, _request.format, json)) ||
         (!read_source(_program, _request.sheet, read)) )
    {
        return D_CLI_EXIT_TROUBLE;
    }

    const resolution  resolved = resolve(read.doc);
    const wide_lookup lookup   = [&resolved](const std::string& _name,
                                             wide&              _value,
                                             std::string&       _error)
    {
        return symbol_bits(resolved, _name, _value, _error);
    };
    json_value        all      = json_value::array();
    int               status   = D_CLI_EXIT_OK;

    for (const std::string& each : _request.expressions)
    {
        json_value  object;
        std::string shown;

        if (!eval_one(_program, each, lookup, read.doc.width, object, shown))
        {
            status = D_CLI_EXIT_NO;

            continue;
        }

        all.push(std::move(object));
        emit(_program, json ? std::string() : shown);
    }

    emit(_program, json ? write_json(all) : std::string());

    return status;
}


//------------------------------------------------------------------------------
// convert
//------------------------------------------------------------------------------

// convert_request
//   struct: what `convert` was asked.
struct convert_request
{
    std::string sheet;
    std::string to  = "sheet";
    std::string out = "-";
    std::string date;
};

/*
run_convert
  The sheet in another shape, whole, to a file or the output.
*/
int
run_convert(
    kit::program&          _program,
    kit::command           _command,
    const convert_request& _request
)
{
    const int   shape  = choice_of(_request.to, SHAPES);
    std::string date;
    source      read;
    int         status = D_CLI_EXIT_OK;

    if (shape < 0)
    {
        return bad_word(_program, _command, "--to", _request.to, SHAPES);
    }

    if ( (date_given(_program, _command, _request.date, date) !=
          D_CLI_EXIT_OK) ||
         (!read_source(_program, _request.sheet, read)) )
    {
        return D_CLI_EXIT_TROUBLE;
    }

    (void)write_or_say(_program,
                       _request.out,
                       sheet_text(read.doc,
                                  static_cast<json_shape>(shape),
                                  date),
                       status);

    return status;
}


//------------------------------------------------------------------------------
// examples
//------------------------------------------------------------------------------

// examples_request
//   struct: what `examples` was asked.
struct examples_request
{
    std::string format = "text";
};

/*
examples_object
  Every example, as examples' JSON gives them.
*/
json_value
examples_object(
    const std::vector<example_info>& _all
)
{
    json_value list = json_value::array();

    for (const example_info& each : _all)
    {
        json_value made = json_value::object();

        made.set("name", json_value::string(each.name));
        made.set("title", json_value::string(each.title));
        made.set("shelf", json_value::string(each.shelf));
        made.set("width", json_value::number(each.width));
        list.push(std::move(made));
    }

    return list;
}

/*
examples_text
  The examples by shelf, each shelf as the File menu lists it, in the order
the first of each comes: a heading, then a line for each, its width
right-aligned -- one table, so every shelf's columns line up.
*/
std::string
examples_text(
    const std::vector<example_info>& _all
)
{
    std::vector<std::string>              shelves;
    std::vector<std::vector<std::string>> cells;

    for (const example_info& each : _all)
    {
        if (choice_of(each.shelf, shelves) < 0)
        {
            shelves.push_back(each.shelf);
        }
    }

    for (const std::string& shelf : shelves)
    {
        cells.push_back({ shelf + ":" });

        for (const example_info& each : _all)
        {
            const std::string bits = std::to_string(each.width);

            if (each.shelf == shelf)
            {
                cells.push_back({ "  " + each.name,
                                  std::string((bits.size() < 5u)
                                              ? (5u - bits.size())
                                              : 0u, ' ') + bits + " bits",
                                  each.title });
            }
        }
    }

    return table(cells);
}

/*
run_examples
  The built-in examples, as text or as JSON.
*/
int
run_examples(
    kit::program&           _program,
    kit::command            _command,
    const examples_request& _request
)
{
    bool json = false;

    if (!json_or_text(_program, _command, _request.format, json))
    {
        return D_CLI_EXIT_TROUBLE;
    }

    emit(_program, json ? write_json(examples_object(example_list()))
                        : examples_text(example_list()));

    return D_CLI_EXIT_OK;
}


//------------------------------------------------------------------------------
// gen
//------------------------------------------------------------------------------

/*
gen_files
  The files beside the header -- the bitfield and the export -- each asked
for; false, the error said and `_status` set, at the first not written.
*/
bool
gen_files(
    kit::program&      _program,
    kit::command       _command,
    const gen_request& _request,
    const source&      _read,
    const resolution&  _resolved,
    const generated&   _header,
    int&               _status
)
{
    return ( (!given(_command, "--bitfield")) ||
             write_or_say(_program, _request.bitfield,
                          write_bitfield(_read.doc), _status) ) &&
           ( (!given(_command, "--export")) ||
             write_or_say(_program, _request.everything,
                          write_export(_read.doc, _resolved, _header),
                          _status) );
}

/*
gen_write
  The header, to its file or the output -- the output only when no JSON
file was asked for alone -- then the self-test, which includes the header
by its file's name.
*/
int
gen_write(
    kit::program&      _program,
    kit::command       _command,
    const gen_request& _request,
    const source&      _read,
    const resolution&  _resolved,
    const generated&   _header,
    int                _status
)
{
    const bool to_file    = given(_command, "--out");
    const bool json_alone = ( given(_command, "--bitfield") ||
                              given(_command, "--export") );
    int        status     = _status;

    if ( (!to_file) &&
         (!json_alone) )
    {
        emit(_program, _header.text);
    }

    if ( (to_file) &&
         (!write_or_say(_program, _request.out, _header.text, status)) )
    {
        return status;
    }

    if (given(_command, "--self-test"))
    {
        (void)write_or_say(_program, _request.self_test,
                           self_test(_read.doc, _resolved,
                                     _read.doc.gen.file_name),
                           status);
    }

    return status;
}

}  // anonymous namespace


/*
add_gen_options
  The options bitwisedom-gen took, each bound to the request, and described
as help and the grammar say them.
*/
void
add_gen_options(
    kit::command _command,
    gen_request& _request
)
{
    _command.add_option("-o,--out", _request.out,
                        "write the header to FILE; standard output by "
                        "default")
            .type_name("FILE");
    _command.add_option("--self-test", _request.self_test,
                        "write the header's self-test to FILE: it compiles "
                        "only if every macro has the sheet's value")
            .type_name("FILE");
    _command.add_option("--bitfield", _request.bitfield,
                        "write the sheet as a bitfield to FILE as well")
            .type_name("FILE");
    _command.add_option("--export", _request.everything,
                        "write everything about the sheet to FILE as well")
            .type_name("FILE");
    _command.add_option("--lang", _request.lang,
                        "the header's language: c or cpp")
            .type_name("LANG")
            .default_str("the sheet's");
    _command.add_option("--form", _request.form, "macros, or C++ constants")
            .type_name("FORM")
            .default_str("the sheet's");
    _command.add_option("--style", _request.style,
                        "the header's style: djinterp or plain")
            .type_name("STYLE")
            .default_str("the sheet's");
    _command.add_option("--level", _request.level,
                        "the language level: " + word_list(level_words()))
            .type_name("LEVEL")
            .default_str("the sheet's");
    _command.add_option("--word", _request.word,
                        "a wide sheet's array words: 8, 16, 32 or 64 bits")
            .type_name("BITS")
            .default_str("the sheet's");
    _command.add_option("--date", _request.date,
                        "the banner's date, so a build writes the same bytes "
                        "every day")
            .type_name("YYYY.MM.DD")
            .default_str("today's");
    _command.add_flag("--check", _request.check,
                      "report the sheet's issues; write nothing");

    return;
}

/*
run_gen
  bitwisedom-gen's work, step for step: the date, the sheet, the settings
over it, its issues; then the files, the header named for the file it goes
to.
*/
int
run_gen(
    kit::program&      _program,
    kit::command       _command,
    const gen_request& _request
)
{
    const std::string operand = given(_command, "--example")
                                ? ("example:" + _request.example)
                                : _request.sheet;
    std::string       date;
    source            read;

    if ( (date_given(_program, _command, _request.date, date) !=
          D_CLI_EXIT_OK)                                          ||
         (!read_source(_program, operand, read))                  ||
         (apply_settings(_program, _command, _request, read.doc.gen) !=
          D_CLI_EXIT_OK) )
    {
        return D_CLI_EXIT_TROUBLE;
    }

    const resolution resolved = resolve(read.doc);
    int              status   = (report_issues(_program, operand,
                                               resolved) > 0)
                                ? D_CLI_EXIT_NO
                                : D_CLI_EXIT_OK;

    // the issues alone
    if (_request.check)
    {
        return status;
    }

    // the header is named for the file it is written to
    if ( given(_command, "--out") &&
         (!kit::is_standard(_request.out)) )
    {
        read.doc.gen.file_name = file_name_of(_request.out);
    }

    const generated header = generate(read.doc, resolved, date);

    report_lines(_program, static_cast<int>(kit::severity::note), operand,
                 header.notes);

    return gen_files(_program, _command, _request, read, resolved, header,
                     status)
           ? gen_write(_program, _command, _request, read, resolved, header,
                       status)
           : status;
}


namespace
{

/*
define_gen_check
  gen and check.
*/
void
define_gen_check(
    kit::program& _program
)
{
    const auto   gens = std::make_shared<gen_request>();
    kit::command gen  = _program.add_command(
        "gen",
        "write a sheet's header, in C or C++",
        [gens](kit::program& _p, kit::command _c)
        {
            return run_gen(_p, _c, *gens);
        });

    gen.group(READING);
    gen.add_option("SHEET", gens->sheet,
                   "the sheet: a sheet file, a bitfield or an export; "
                   "example:NAME for a built-in example; - for standard "
                   "input")
       .required();
    add_gen_options(gen, *gens);

    const auto   checks = std::make_shared<check_request>();
    kit::command check  = _program.add_command(
        "check",
        "report every issue of every sheet; write nothing else",
        [checks](kit::program& _p, kit::command _c)
        {
            return run_check(_p, _c, *checks);
        });

    check.group(READING);
    check.add_option("SHEET", checks->sheets, "the sheets, as gen reads one")
         .required();
    add_format(check, checks->format);

    return;
}

/*
define_show_decode
  show and decode.
*/
void
define_show_decode(
    kit::program& _program
)
{
    const auto   shows = std::make_shared<show_request>();
    kit::command show  = _program.add_command(
        "show",
        "list every row as it resolves: name, kind, bits and value",
        [shows](kit::program& _p, kit::command _c)
        {
            return run_show(_p, _c, *shows);
        });

    show.group(READING);
    show.add_option("SHEET", shows->sheet, "the sheet, as gen reads one")
        .required();
    add_format(show, shows->format);

    const auto   decodes = std::make_shared<decode_request>();
    kit::command decode  = _program.add_command(
        "decode",
        "say what values hold: each bit set, each field's value and its name",
        [decodes](kit::program& _p, kit::command _c)
        {
            return run_decode(_p, _c, *decodes);
        });

    decode.group(READING);
    decode.add_option("SHEET", decodes->sheet, "the sheet, as gen reads one")
          .required();
    decode.add_option("VALUE", decodes->values,
                      "a value: 42, 0x2A, 0o52 or 0b101010, `_` between "
                      "digits")
          .required();
    add_format(decode, decodes->format);

    return;
}

/*
define_eval_convert
  eval and convert.
*/
void
define_eval_convert(
    kit::program& _program
)
{
    const auto   evals = std::make_shared<eval_request>();
    kit::command eval  = _program.add_command(
        "eval",
        "evaluate C expressions over a sheet's symbols, every way",
        [evals](kit::program& _p, kit::command _c)
        {
            return run_eval(_p, _c, *evals);
        });

    eval.group(READING);
    eval.add_option("SHEET", evals->sheet, "the sheet, as gen reads one")
        .required();
    eval.add_option("EXPRESSION", evals->expressions,
                    "an expression, quoted as the shell needs it")
        .required();
    add_format(eval, evals->format);

    const auto   converts = std::make_shared<convert_request>();
    kit::command convert  = _program.add_command(
        "convert",
        "write a sheet in another shape: a sheet file, a bitfield or an "
        "export",
        [converts](kit::program& _p, kit::command _c)
        {
            return run_convert(_p, _c, *converts);
        });

    convert.group(READING);
    convert.add_option("SHEET", converts->sheet,
                       "the sheet, as gen reads one")
           .required();
    convert.add_option("--to", converts->to,
                       "the shape: sheet, bitfield or export")
           .type_name("SHAPE")
           .default_str("sheet");
    convert.add_option("-o,--out", converts->out, "write it to FILE")
           .type_name("FILE")
           .default_str("standard output");
    convert.add_option("--date", converts->date, "an export's header's date")
           .type_name("YYYY.MM.DD")
           .default_str("today's");

    return;
}

/*
define_examples
  examples.
*/
void
define_examples(
    kit::program& _program
)
{
    const auto   request = std::make_shared<examples_request>();
    kit::command made    = _program.add_command(
        "examples",
        "list the built-in examples, by shelf",
        [request](kit::program& _p, kit::command _c)
        {
            return run_examples(_p, _c, *request);
        });

    made.group(READING);
    add_format(made, request->format);

    return;
}

}  // anonymous namespace


/*
add_read_commands
  Each command with its request, owned by its work, so it lives as long as
the program and every option bound to it.
*/
void
add_read_commands(
    kit::program& _program
)
{
    define_gen_check(_program);
    define_show_decode(_program);
    define_eval_convert(_program);
    define_examples(_program);

    return;
}

}  // namespace cmd
}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
