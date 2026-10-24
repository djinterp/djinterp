/*******************************************************************************
* bitwisedom [bitwisedom]                                           cli_edit.cpp
*
* The commands that make a sheet or a row: new, import, add and change.
*   A changing command reads its sheet, changes it in memory, and writes it
* back where it came from, in the shape it came in, whole or not at all
* (GRAMMAR.md, R20): the change is made by the model's own edits -- those
* the window makes -- so a row the window would refuse is refused here in
* the same words. Every operand is checked before anything is written, so
* a command that fails leaves the file as it was.
*   A row is defined as a bitfield's row is, by options: one definition --
* a bit, a range, a value in a range, a literal, an expression, a macro, or
* for add a group -- and anything else a row may say. add takes exactly
* one definition; change takes one to replace the row's, or only the parts
* of its own -- a range's width, a value's number, a macro's parameters --
* and keeps whatever it is not given. An option counts as given when it is
* on the command line at all, so `--note=` clears a note.
*
* path:      /src/djinterp/tools/bitwisedom/cli/cli_edit.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.10.22
*******************************************************************************/
#include "./cli_internal.hpp"  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>  // std::size_t
#include <memory>   // std::make_shared
#include <string>   // std::string
#include <utility>  // std::move
#include <vector>   // std::vector
// bitwisedom
#include <djinterp/tools/bitwisedom/bits.hpp>     // runs_text, default_bias
#include <djinterp/tools/bitwisedom/cimport.hpp>  // import_code, merge_rows
#include <djinterp/tools/bitwisedom/edit.hpp>     // try_insert, try_change
#include <djinterp/tools/bitwisedom/persist.hpp>  // read_enum, enum_word


namespace bitwisedom
{
namespace cmd
{

namespace
{

// MODELS / NEW_SHAPES
//   constants: the data models' words, and the shapes a new sheet is
// written in.
const std::vector<std::string> MODELS     = { "lp64", "llp64", "ilp32" };
const std::vector<std::string> NEW_SHAPES = { "sheet", "bitfield" };

// NEXT
//   constant: what --bit and --lsb are, given alone: the next bit free.
const char* const NEXT = "next";

// DEFINITIONS
//   constant: the heading help lists the definitions' options under.
const char* const DEFINITIONS = "Definitions";

// KINDS
//   constant: the options that choose each definition, in def_kind's order,
// then a group's: one or two names each, the second "" where there is none.
const char* const KINDS[][2] = {
    { "--bit",   ""            },
    { "--bits",  "--lsb"       },
    { "--field", ""            },
    { "--value", ""            },
    { "--expr",  "--expr-file" },
    { "--body",  "--body-file" },
    { "--group", ""            } };

// KIND_COUNT / GROUP
//   constants: how many definitions the options choose among, and a group's
// place among them, after def_kind's.
const int KIND_COUNT = 7;
const int GROUP      = 6;

// part
//   struct: an option that gives a part of one definition -- the width of a
// range, the number of a value -- and the definition it is a part of.
struct part
{
    const char* option;
    def_kind    kind;
};

// PARTS
//   constant: every part's option.
const part PARTS[] = {
    { "--width",    def_kind::range       },
    { "--number",   def_kind::field_value },
    { "--encoding", def_kind::field_value },
    { "--bias",     def_kind::field_value },
    { "--params",   def_kind::macro       },
    { "--callable", def_kind::macro       } };

// ROW_OPTIONS
//   constant: the options of what a row says besides its definition.
const char* const ROW_OPTIONS[] = {
    "--note", "--short", "--prefix", "--suffix", "--colour", "--emit",
    "--isolate", "--name-children", "--exclusive", "--radix", "--int-suffix",
    "--digits" };

// KIND_WORDS / KIND_OPTION
//   constants: each definition as a person names it, and the option that
// chooses it, in def_kind's order.
const char* const KIND_WORDS[]  = { "a bit", "a range", "a value",
                                    "a literal", "an expression", "a macro" };
const char* const KIND_OPTION[] = { "--bit", "--lsb", "--field", "--value",
                                    "--expr", "--body" };

// ONE_DEFINITION
//   constant: the definitions, as a usage error lists them.
const char* const ONE_DEFINITION =
    "--bit, --bits or --lsb, --field, --value, --expr, --body";


//------------------------------------------------------------------------------
// words
//------------------------------------------------------------------------------

// encoding_words / radix_words / suffix_words
//   the words a sheet file writes each encoding, radix and literal suffix
// as, in their enumerations' order.
std::vector<std::string>
encoding_words()
{
    std::vector<std::string> out;

    for (int i = 0; i < ENCODING_COUNT; ++i)
    {
        out.push_back(enum_word(static_cast<encoding>(i)));
    }

    return out;
}

std::vector<std::string>
radix_words()
{
    std::vector<std::string> out;

    for (int i = static_cast<int>(radix::inherit);
         i <= static_cast<int>(radix::shift);
         ++i)
    {
        out.push_back(enum_word(static_cast<radix>(i)));
    }

    return out;
}

std::vector<std::string>
suffix_words()
{
    std::vector<std::string> out;

    for (int i = static_cast<int>(int_suffix::inherit);
         i <= static_cast<int>(int_suffix::ull);
         ++i)
    {
        out.push_back(enum_word(static_cast<int_suffix>(i)));
    }

    return out;
}


//------------------------------------------------------------------------------
// new
//------------------------------------------------------------------------------

// new_request
//   struct: what `new` was asked.
struct new_request
{
    std::string title  = "untitled";
    std::string width  = "32";
    std::string prefix;
    std::string suffix;
    std::string to     = "sheet";
    std::string out    = "-";
};

/*
run_new
  File > New's sheet -- a full-width range named root, and in it one bit --
under the title, width and affixes given.
*/
int
run_new(
    kit::program&      _program,
    kit::command       _command,
    const new_request& _request
)
{
    i64       width = 0;
    const int shape = choice_of(_request.to, NEW_SHAPES);

    if ( (!read_number(_request.width, width)) ||
         (width < 1)                           ||
         (width > MAX_WIDTH) )
    {
        return _program.usage_error(_command,
                                    "--width: '" + _request.width + "' is "
                                    "not a width from 1 to " +
                                    std::to_string(MAX_WIDTH) + " bits");
    }

    if (shape < 0)
    {
        return bad_word(_program, _command, "--to", _request.to, NEW_SHAPES);
    }

    sheet made = make_document(_request.title,
                               static_cast<int>(width));

    made.prefix = _request.prefix;
    made.suffix = _request.suffix;

    const int failed = kit::write_file(_request.out,
                                       sheet_text(made,
                                                  static_cast<json_shape>(
                                                      shape),
                                                  today()));

    return (failed == 0) ? D_CLI_EXIT_OK
                         : _program.file_error("write",
                                               _request.out,
                                               failed);
}


//------------------------------------------------------------------------------
// import
//------------------------------------------------------------------------------

// import_request
//   struct: what `import` was asked.
struct import_request
{
    std::vector<std::string> files;
    std::string              level = "c99";
    std::string              model;
    std::string              title;
    std::string              file_name;
    std::string              to    = "sheet";
    std::string              out   = "-";
};

/*
import_settings
  The level, the data model and the shape, each word checked, as the
importer's options and the shape's index.
*/
int
import_settings(
    kit::program&         _program,
    kit::command          _command,
    const import_request& _request,
    import_options&       _options,
    int&                  _shape
)
{
    const std::vector<std::string> levels = level_words();
    const int                      level  = choice_of(_request.level,
                                                      levels);
    const int                      model  = given(_command, "--model")
                                            ? choice_of(_request.model,
                                                        MODELS)
                                            : static_cast<int>(
                                                  host_data_model());

    _shape = choice_of(_request.to, NEW_SHAPES);

    if (level < 0)
    {
        return bad_word(_program, _command, "--level", _request.level,
                        levels);
    }

    if (model < 0)
    {
        return bad_word(_program, _command, "--model", _request.model,
                        MODELS);
    }

    if (_shape < 0)
    {
        return bad_word(_program, _command, "--to", _request.to, NEW_SHAPES);
    }

    _options.level     = static_cast<standard>(level);
    _options.model     = static_cast<data_model>(model);
    _options.title     = _request.title;
    _options.file_name = _request.file_name;

    return D_CLI_EXIT_OK;
}

/*
import_one
  One file read and imported, every note at its line; the first makes the
sheet, each after it merged in. A file with no #define in it is trouble, as
nothing came of it.
*/
int
import_one(
    kit::program&         _program,
    const std::string&    _file,
    const import_options& _options,
    bool                  _first,
    sheet&                _made
)
{
    std::string text;
    const int   failed = kit::read_file(_file, text);

    if (failed != 0)
    {
        return _program.file_error("read", _file, failed);
    }

    imported read = import_code(text, _options);

    report_lines(_program, static_cast<int>(kit::severity::note), _file,
                 read.notes);

    // nothing came of it
    if (!read.ok)
    {
        _program.report_at(kit::severity::error, _file, 0L, 0L,
                           "there is no #define in it");

        return D_CLI_EXIT_TROUBLE;
    }

    _program.note(1, _file + ": " + plural(read.definitions, "definition") +
                     " read");

    if (_first)
    {
        _made = std::move(read.result);

        return D_CLI_EXIT_OK;
    }

    const merged into = merge_rows(_made, 0, read.result);

    report_lines(_program, static_cast<int>(kit::severity::note), _file,
                 into.notes);

    return D_CLI_EXIT_OK;
}

/*
run_import
  Every file imported in turn, then the sheet written whole.
*/
int
run_import(
    kit::program&         _program,
    kit::command          _command,
    const import_request& _request
)
{
    import_options options;
    int            shape  = 0;
    int            status = import_settings(_program, _command, _request,
                                            options, shape);
    sheet          made;

    for (std::size_t i = 0;
         ( (status == D_CLI_EXIT_OK) && (i < _request.files.size()) );
         ++i)
    {
        status = import_one(_program, _request.files[i], options, (i == 0u),
                            made);
    }

    if (status != D_CLI_EXIT_OK)
    {
        return status;
    }

    const int failed = kit::write_file(_request.out,
                                       sheet_text(made,
                                                  static_cast<json_shape>(
                                                      shape),
                                                  today()));

    return (failed == 0) ? D_CLI_EXIT_OK
                         : _program.file_error("write",
                                               _request.out,
                                               failed);
}


//------------------------------------------------------------------------------
// what a row is given
//------------------------------------------------------------------------------

// definition_options
//   struct: a row's definition as add and change take it: the options that
// choose one, and the parts that go with them.
struct definition_options
{
    std::string bit;
    std::string bits;
    std::string lsb;
    std::string width;
    std::string field;
    std::string number;
    std::string encoding;
    std::string bias;
    std::string value;
    std::string expr;
    std::string expr_file;
    std::string body;
    std::string body_file;
    std::string params;
    bool        callable = false;
    bool        group    = false;
};

// row_options
//   struct: what any row may say besides its definition: its texts, its
// colour, how its literal is written, and its switches -- each read only
// when it was given.
struct row_options
{
    std::string note;
    std::string short_name;
    std::string prefix;
    std::string suffix;
    std::string colour;
    std::string radix_word;
    std::string suffix_word;
    std::string digits;
    bool        emit          = true;
    bool        isolate       = false;
    bool        name_children = false;
    bool        exclusive     = true;
};

/*
chosen_kind
  Which definition the options choose: its index in KINDS; -1 for none, and
KIND_COUNT for more than one.
*/
int
chosen_kind(
    kit::command _command
)
{
    int found = -1;
    int count = 0;

    for (int i = 0; i < KIND_COUNT; ++i)
    {
        const bool named = ( given(_command, KINDS[i][0]) ||
                             ( (KINDS[i][1][0] != '\0') &&
                               given(_command, KINDS[i][1]) ) );

        found  = named ? i : found;
        count += named ? 1 : 0;
    }

    return (count > 1) ? KIND_COUNT : found;
}

/*
parts_fit
  Every part given belongs to the definition the row will have; a usage
error for the first that does not, which says where it goes -- or, when no
definition was given, what the row is.
*/
int
parts_fit(
    kit::program&      _program,
    kit::command       _command,
    def_kind           _kind,
    bool               _chosen,
    const std::string& _name
)
{
    const int kind = static_cast<int>(_kind);

    for (const part& each : PARTS)
    {
        if ( (!given(_command, each.option)) ||
             (each.kind == _kind) )
        {
            continue;
        }

        const int owner = static_cast<int>(each.kind);

        return _program.usage_error(
                   _command,
                   _chosen
                       ? std::string(each.option) + " goes with " +
                         KIND_OPTION[owner]
                       : std::string(each.option) + " is " +
                         KIND_WORDS[owner] + "'s, and " + _name + " is " +
                         KIND_WORDS[kind]);
    }

    // a range's own bits give its width
    if ( (_kind == def_kind::range) &&
         given(_command, "--bits") &&
         given(_command, "--width") )
    {
        return _program.usage_error(_command,
                                    "--width goes with --lsb: --bits gives "
                                    "its own");
    }

    return D_CLI_EXIT_OK;
}

/*
text_or_file
  An option's text, or a file's, read whole -- `-` standard input -- so
that text the shells would take apart need never be quoted.
*/
bool
text_or_file(
    kit::program&      _program,
    const std::string& _text,
    const std::string& _file,
    std::string&       _out,
    int&               _status
)
{
    // the text itself
    if (_file.empty())
    {
        _out = _text;

        return true;
    }

    std::string read;
    const int   failed = kit::read_file(_file, read);

    if (failed != 0)
    {
        _status = _program.file_error("read", _file, failed);

        return false;
    }

    // one trailing newline is the file's, not the text's
    if ( (!read.empty()) &&
         (read.back() == '\n') )
    {
        read.pop_back();
        read.erase( ( (!read.empty()) && (read.back() == '\r') )
                        ? read.size() - 1u
                        : read.size() );
    }

    _out = read;

    return true;
}

/*
read_range
  A RANGE as the grammar writes one: `4..6`, `6..4`, or one bit, `5`.
*/
bool
read_range(
    const std::string& _text,
    int&               _lsb,
    int&               _width
)
{
    const std::size_t dots = _text.find("..");
    i64               low  = 0;
    i64               high = 0;
    const bool        read = (dots == std::string::npos)
                             ? read_number(_text, low)
                             : ( read_number(_text.substr(0, dots), low) &&
                                 read_number(_text.substr(dots + 2u),
                                             high) );

    high = (dots == std::string::npos) ? low : high;

    // either way round
    if (low > high)
    {
        const i64 swap = low;

        low  = high;
        high = swap;
    }

    _lsb   = static_cast<int>(low);
    _width = static_cast<int>(high - low + 1);

    return ( read            &&
             (low >= 0)      &&
             (high < MAX_WIDTH) );
}


//------------------------------------------------------------------------------
// definitions
//------------------------------------------------------------------------------

/*
define_bit
  --bit, alone the next bit free in the row's group.
*/
int
define_bit(
    kit::program&             _program,
    kit::command              _command,
    const definition_options& _options,
    const sheet&              _sheet,
    int                       _parent,
    row&                      _row
)
{
    i64 at = 0;

    if ( (_options.bit != NEXT) &&
         ( (!read_number(_options.bit, at)) ||
           (at < 0)                         ||
           (at >= _sheet.width) ) )
    {
        return _program.usage_error(_command,
                                    "--bit: '" + _options.bit + "' is not a "
                                    "bit of the sheet's " +
                                    std::to_string(_sheet.width));
    }

    _row.def = def_kind::bit;
    _row.bit = (_options.bit == NEXT) ? place_bit(_sheet, _parent)
                                      : static_cast<int>(at);

    return D_CLI_EXIT_OK;
}

/*
range_width
  --width, a range's part: one bit or more, inside the sheet from its lowest.
*/
int
range_width(
    kit::program&             _program,
    kit::command              _command,
    const definition_options& _options,
    const sheet&              _sheet,
    row&                      _row
)
{
    i64 width = _row.width;

    if ( given(_command, "--width") &&
         ( (!read_number(_options.width, width)) ||
           (width < 1) ) )
    {
        return _program.usage_error(_command,
                                    "--width: '" + _options.width + "' is "
                                    "not a width of one bit or more");
    }

    _row.width = static_cast<int>(width);

    if (_row.lsb + _row.width > _sheet.width)
    {
        return _program.usage_error(_command,
                                    "bits " + std::to_string(_row.lsb) +
                                    ".." +
                                    std::to_string(_row.lsb + _row.width -
                                                   1) +
                                    " run past the sheet's " +
                                    std::to_string(_sheet.width));
    }

    return D_CLI_EXIT_OK;
}

/*
define_range
  --bits, or --lsb -- alone the next bit free -- and its width: --width's,
or the row's own where it was a range already, or one bit.
*/
int
define_range(
    kit::program&             _program,
    kit::command              _command,
    const definition_options& _options,
    const sheet&              _sheet,
    int                       _parent,
    row&                      _row
)
{
    const bool was = (_row.def == def_kind::range);
    i64        at  = 0;

    _row.def = def_kind::range;

    // its bits
    if (given(_command, "--bits"))
    {
        if (!read_range(_options.bits, _row.lsb, _row.width))
        {
            return _program.usage_error(_command,
                                        "--bits: '" + _options.bits + "' is "
                                        "not a range of bits, such as 4..7");
        }

        return range_width(_program, _command, _options, _sheet, _row);
    }

    if ( (_options.lsb != NEXT) &&
         ( (!read_number(_options.lsb, at)) ||
           (at < 0) ) )
    {
        return _program.usage_error(_command,
                                    "--lsb: '" + _options.lsb + "' is not a "
                                    "bit");
    }

    _row.lsb   = (_options.lsb == NEXT) ? place_bit(_sheet, _parent)
                                        : static_cast<int>(at);
    _row.width = was ? _row.width : 1;

    return range_width(_program, _command, _options, _sheet, _row);
}

/*
value_parts
  --number, --encoding and --bias, a value's parts: each given read, the
rest kept; offset binary with no bias takes its field's, as the window
gives it.
*/
int
value_parts(
    kit::program&             _program,
    kit::command              _command,
    const definition_options& _options,
    const resolution&         _resolution,
    row&                      _row
)
{
    if ( given(_command, "--number") &&
         (!read_number(_options.number, _row.number)) )
    {
        return _program.usage_error(_command,
                                    "--number: '" + _options.number + "' is "
                                    "not a number");
    }

    if ( given(_command, "--encoding") &&
         (!read_enum(_options.encoding, _row.enc)) )
    {
        return bad_word(_program, _command, "--encoding", _options.encoding,
                        encoding_words());
    }

    if ( given(_command, "--bias") &&
         (!read_number(_options.bias, _row.bias)) )
    {
        return _program.usage_error(_command,
                                    "--bias: '" + _options.bias + "' is not "
                                    "a number");
    }

    const resolved_row* range = _resolution.find(_row.field);

    // offset binary's bias, where none was given
    if ( (_row.enc == encoding::offset_binary) &&
         (_row.bias == 0)                      &&
         (!given(_command, "--bias"))          &&
         (range != nullptr) )
    {
        _row.bias = default_bias(range->span_width);
    }

    return D_CLI_EXIT_OK;
}

/*
define_value
  --field, a range and not the row itself; then its parts, a new value's
starting from 0, unsigned.
*/
int
define_value(
    kit::program&             _program,
    kit::command              _command,
    const definition_options& _options,
    const resolution&         _resolution,
    row&                      _row
)
{
    const int field = row_named(_program, _command, _resolution,
                                _options.field, "--field");

    if (field == 0)
    {
        return D_CLI_EXIT_TROUBLE;
    }

    const resolved_row* range = _resolution.find(field);

    if ( (range == nullptr)                  ||
         (range->kind != row_kind::symbol)   ||
         (range->def != def_kind::range)     ||
         (field == _row.id) )
    {
        return _program.usage_error(_command,
                                    "--field: " + _options.field + " is not "
                                    "a range" +
                                    ((field == _row.id) ? " of its own"
                                                        : ""));
    }

    // a value anew: its number and encoding start over
    if (_row.def != def_kind::field_value)
    {
        _row.number = 0;
        _row.enc    = encoding::unsigned_binary;
        _row.bias   = 0;
    }

    _row.def   = def_kind::field_value;
    _row.field = field;

    return value_parts(_program, _command, _options, _resolution, _row);
}

/*
macro_parts
  --params and --callable, a macro's parts: each given set, the rest kept.
*/
void
macro_parts(
    kit::command              _command,
    const definition_options& _options,
    row&                      _row
)
{
    _row.params   = given(_command, "--params") ? _options.params
                                                : _row.params;
    _row.callable = given(_command, "--callable") ? _options.callable
                                                  : _row.callable;

    return;
}

/*
define_text
  A literal, an expression or a macro: what each is written as -- the
literal read as the grammar writes an integer, the text from the option or
its file -- and a macro's parts, a new macro's starting with none.
*/
int
define_text(
    kit::program&             _program,
    kit::command              _command,
    const definition_options& _options,
    def_kind                  _kind,
    row&                      _row
)
{
    std::string error;
    int         status = D_CLI_EXIT_OK;
    const bool  was    = (_row.def == _kind);

    _row.def = _kind;

    switch (_kind)
    {
        case def_kind::literal:
            return read_value(_options.value, _row.literal, _row.negative,
                              error)
                   ? D_CLI_EXIT_OK
                   : _program.usage_error(_command, "--value: " + error);

        case def_kind::expression:
            return text_or_file(_program, _options.expr, _options.expr_file,
                                _row.expr, status)
                   ? D_CLI_EXIT_OK
                   : status;

        default:
            break;
    }

    if (!text_or_file(_program, _options.body, _options.body_file, _row.body,
                      status))
    {
        return status;
    }

    _row.params   = was ? _row.params : std::string();
    _row.callable = was ? _row.callable : false;
    macro_parts(_command, _options, _row);

    return D_CLI_EXIT_OK;
}

/*
define
  The definition the options chose, with its parts; -- none chosen -- the
parts alone, of the definition the row has.
*/
int
define(
    kit::program&             _program,
    kit::command              _command,
    const definition_options& _options,
    const sheet&              _sheet,
    const resolution&         _resolution,
    int                       _parent,
    int                       _kind,
    row&                      _row
)
{
    switch (_kind)
    {
        case 0:
            return define_bit(_program, _command, _options, _sheet, _parent,
                              _row);
        case 1:
            return define_range(_program, _command, _options, _sheet,
                                _parent, _row);
        case 2:
            return define_value(_program, _command, _options, _resolution,
                                _row);
        case 3:
        case 4:
        case 5:
            return define_text(_program, _command, _options,
                               static_cast<def_kind>(_kind), _row);
        default:
            break;
    }

    // the parts of the row's own definition
    switch (_row.def)
    {
        case def_kind::range:
            return range_width(_program, _command, _options, _sheet, _row);
        case def_kind::field_value:
            return value_parts(_program, _command, _options, _resolution,
                               _row);
        case def_kind::macro:
            macro_parts(_command, _options, _row);
            break;
        default:
            break;
    }

    return D_CLI_EXIT_OK;
}


//------------------------------------------------------------------------------
// what else a row says
//------------------------------------------------------------------------------

/*
row_colour
  --colour: #RRGGBB, #RRGGBBAA; auto, a colour of the scheme's; or none,
no fill.
*/
int
row_colour(
    kit::program&      _program,
    kit::command       _command,
    const std::string& _colour,
    row&               _row
)
{
    if ( (_colour == "auto") ||
         (_colour == "none") )
    {
        _row.colour      = 0;
        _row.transparent = (_colour == "none");

        return D_CLI_EXIT_OK;
    }

    rgba read = 0;

    if ( (_colour.empty())     ||
         (_colour[0] != '#')   ||
         (!parse_rgba(_colour, read)) )
    {
        return _program.usage_error(_command,
                                    "--colour: '" + _colour + "' is not "
                                    "#RRGGBB, #RRGGBBAA, auto or none");
    }

    _row.colour      = read;
    _row.transparent = false;

    return D_CLI_EXIT_OK;
}

/*
row_switches
  --emit, and a group's or a range's own: --isolate, --name-children and
--exclusive, refused for a row that holds no rows.
*/
int
row_switches(
    kit::program&      _program,
    kit::command       _command,
    const row_options& _options,
    row&               _row
)
{
    _row.emit = given(_command, "--emit") ? _options.emit : _row.emit;

    for (const char* const each : { "--isolate", "--name-children",
                                    "--exclusive" })
    {
        if ( given(_command, each) &&
             (!can_contain(_row)) )
        {
            return _program.usage_error(_command,
                                        std::string(each) + " is a group's "
                                        "or a range's, and " + _row.name +
                                        " holds no rows");
        }
    }

    _row.isolate       = given(_command, "--isolate") ? _options.isolate
                                                      : _row.isolate;
    _row.name_children = given(_command, "--name-children")
                         ? _options.name_children
                         : _row.name_children;
    _row.exclusive     = given(_command, "--exclusive") ? _options.exclusive
                                                        : _row.exclusive;

    return D_CLI_EXIT_OK;
}

/*
row_form
  --radix, --int-suffix and --digits: how a symbol's value is written,
refused for a group, which has none.
*/
int
row_form(
    kit::program&      _program,
    kit::command       _command,
    const row_options& _options,
    row&               _row
)
{
    i64 digits = _row.digits;

    for (const char* const each : { "--radix", "--int-suffix", "--digits" })
    {
        if ( given(_command, each) &&
             (_row.kind == row_kind::group) )
        {
            return _program.usage_error(_command,
                                        std::string(each) + ": " +
                                        _row.name + " is a group, which has "
                                        "no value to write");
        }
    }

    if ( given(_command, "--radix") &&
         (!read_enum(_options.radix_word, _row.fmt)) )
    {
        return bad_word(_program, _command, "--radix", _options.radix_word,
                        radix_words());
    }

    if ( given(_command, "--int-suffix") &&
         (!read_enum(_options.suffix_word, _row.suf)) )
    {
        return bad_word(_program, _command, "--int-suffix",
                        _options.suffix_word, suffix_words());
    }

    if ( given(_command, "--digits") &&
         ( (!read_number(_options.digits, digits)) ||
           (digits < 0)                            ||
           (digits > 64) ) )
    {
        return _program.usage_error(_command,
                                    "--digits: '" + _options.digits + "' is "
                                    "not a count of digits from 0 to 64");
    }

    _row.digits = static_cast<int>(digits);

    return D_CLI_EXIT_OK;
}

/*
apply_row_options
  Every option given of what a row says besides its definition; the rest
left as the row has them.
*/
int
apply_row_options(
    kit::program&      _program,
    kit::command       _command,
    const row_options& _options,
    row&               _row
)
{
    _row.note       = given(_command, "--note") ? _options.note : _row.note;
    _row.short_name = given(_command, "--short") ? _options.short_name
                                                 : _row.short_name;
    _row.prefix     = given(_command, "--prefix") ? _options.prefix
                                                  : _row.prefix;
    _row.suffix     = given(_command, "--suffix") ? _options.suffix
                                                  : _row.suffix;

    const int status = given(_command, "--colour")
                       ? row_colour(_program, _command, _options.colour,
                                    _row)
                       : D_CLI_EXIT_OK;

    if (status != D_CLI_EXIT_OK)
    {
        return status;
    }

    const int switched = row_switches(_program, _command, _options, _row);

    return (switched != D_CLI_EXIT_OK)
           ? switched
           : row_form(_program, _command, _options, _row);
}


//------------------------------------------------------------------------------
// add
//------------------------------------------------------------------------------

// add_request
//   struct: what `add` was asked: the sheet, the row's name, its one
// definition, where it goes, and the rest of what a row may say.
struct add_request
{
    std::string        sheet;
    std::string        name;
    definition_options def;
    std::string        in;
    row_options        rest;
    destination        where;
};

/*
add_parent
  --in: a group or a range, by name; the sheet's own rows, 0, without it.
*/
int
add_parent(
    kit::program&      _program,
    kit::command       _command,
    const std::string& _in,
    const source&      _source,
    const resolution&  _resolution,
    int&               _parent
)
{
    _parent = 0;

    if (!given(_command, "--in"))
    {
        return D_CLI_EXIT_OK;
    }

    _parent = row_named(_program, _command, _resolution, _in, "--in");

    const row* holder = (_parent != 0) ? find_row(_source.doc, _parent)
                                       : nullptr;

    if (_parent == 0)
    {
        return D_CLI_EXIT_TROUBLE;
    }

    return ( (holder != nullptr) && can_contain(*holder) )
           ? D_CLI_EXIT_OK
           : _program.usage_error(_command,
                                  "--in: " + _in + " holds no rows: name a "
                                  "group or a range");
}

/*
keep_trial
  A row tried in a copy: refused as the window refuses it, or the copy kept
and stored, the row said with the bits it took.
*/
int
keep_trial(
    kit::program&      _program,
    kit::command       _command,
    source&            _source,
    const destination& _where,
    trial&             _tried,
    const std::string& _done,
    const resolution&  _before
)
{
    // refused as the window refuses it
    if (!_tried.refused.empty())
    {
        _program.report(kit::severity::error, _tried.refused);

        return D_CLI_EXIT_TROUBLE;
    }

    const resolved_row* now   = _tried.named.find(_tried.added);
    const row*          made  = find_row(_tried.result, _tried.added);
    const bool          span  = ( (now != nullptr)     &&
                                  (made != nullptr)    &&
                                  (now->span_lsb >= 0) &&
                                  (made->def != def_kind::field_value) );
    const std::string   where = span ? ", at " +
                                       runs_text(wide::run(now->span_lsb,
                                                           now->span_width),
                                                 4)
                                     : std::string();
    const std::string   name  = (now != nullptr) ? shown_name(*now)
                                                 : std::string();

    _source.doc = std::move(_tried.result);

    return store(_program, _command, _source, _where,
                 name + " " + _done + where, _before);
}

/*
run_add
  One definition, a place to put it, the rest of what it says, and the row
tried in a copy of the sheet.
*/
int
run_add(
    kit::program&      _program,
    kit::command       _command,
    const add_request& _request
)
{
    const int kind = chosen_kind(_command);

    if ( (kind < 0) ||
         (kind == KIND_COUNT) )
    {
        return _program.usage_error(_command,
                                    std::string("give one definition: ") +
                                    ONE_DEFINITION + ", or --group");
    }

    source read;

    if (!read_source(_program, _request.sheet, read))
    {
        return D_CLI_EXIT_TROUBLE;
    }

    const resolution before = resolve(read.doc);
    int              parent = 0;
    int              status = add_parent(_program, _command, _request.in,
                                         read, before, parent);
    row              made;

    // a group has no parts: none fits it as none fits a bit
    made.name          = _request.name;
    made.kind          = (kind == GROUP) ? row_kind::group : row_kind::symbol;
    made.name_children = (kind == 1);
    status             = (status == D_CLI_EXIT_OK)
                         ? parts_fit(_program, _command,
                                     (kind == GROUP)
                                         ? def_kind::bit
                                         : static_cast<def_kind>(kind),
                                     true, _request.name)
                         : status;
    status             = ( (status == D_CLI_EXIT_OK) && (kind != GROUP) )
                         ? define(_program, _command, _request.def, read.doc,
                                  before, parent, kind, made)
                         : status;
    status             = (status == D_CLI_EXIT_OK)
                         ? apply_row_options(_program, _command,
                                             _request.rest, made)
                         : status;

    if (status != D_CLI_EXIT_OK)
    {
        return status;
    }

    trial tried = try_insert(read.doc, parent, -1, made);

    return keep_trial(_program, _command, read, _request.where, tried,
                      "added", before);
}


//------------------------------------------------------------------------------
// change
//------------------------------------------------------------------------------

// change_request
//   struct: what `change` was asked: the sheet, the row, a definition or
// its parts, and the rest of what a row may say.
struct change_request
{
    std::string        sheet;
    std::string        row;
    definition_options def;
    row_options        rest;
    destination        where;
};

/*
anything_given
  Whether the options give a change at all: a definition, a part of one, or
anything else a row says.
*/
bool
anything_given(
    kit::command _command,
    int          _kind
)
{
    bool found = (_kind >= 0);

    for (const part& each : PARTS)
    {
        found = found || given(_command, each.option);
    }

    for (const char* const each : ROW_OPTIONS)
    {
        found = found || given(_command, each);
    }

    return found;
}

/*
change_definition
  A group has none to change; a symbol takes the definition given, or the
parts of its own.
*/
int
change_definition(
    kit::program&         _program,
    kit::command          _command,
    const change_request& _request,
    source&               _source,
    const resolution&     _before,
    int                   _kind,
    row&                  _row
)
{
    const row* const parent = parent_of(_source.doc, _row.id);
    bool             parts  = false;

    for (const part& each : PARTS)
    {
        parts = parts || given(_command, each.option);
    }

    if (_row.kind == row_kind::group)
    {
        return ( (_kind < 0) && (!parts) )
               ? D_CLI_EXIT_OK
               : _program.usage_error(_command,
                                      _row.name + " is a group: it has no "
                                      "definition");
    }

    const int status = parts_fit(_program, _command,
                                 (_kind >= 0) ? static_cast<def_kind>(_kind)
                                              : _row.def,
                                 (_kind >= 0), _request.row);

    return (status != D_CLI_EXIT_OK)
           ? status
           : define(_program, _command, _request.def, _source.doc, _before,
                    (parent != nullptr) ? parent->id : 0, _kind, _row);
}

/*
run_change
  The row found, its fields changed in a copy of it, and the copy tried in
place of the row -- refused as the window refuses a name it cannot take.
*/
int
run_change(
    kit::program&         _program,
    kit::command          _command,
    const change_request& _request
)
{
    const int kind = chosen_kind(_command);

    if ( (kind == KIND_COUNT) ||
         (!anything_given(_command, kind)) )
    {
        return _program.usage_error(_command,
                                    (kind == KIND_COUNT)
                                        ? std::string("give one definition "
                                                      "at most: ") +
                                          ONE_DEFINITION
                                        : std::string("say what to change: "
                                                      "a definition, a part "
                                                      "of one, --note, "
                                                      "--colour ..."));
    }

    source read;

    if (!read_source(_program, _request.sheet, read))
    {
        return D_CLI_EXIT_TROUBLE;
    }

    const resolution before = resolve(read.doc);
    const int        id     = row_named(_program, _command, before,
                                        _request.row, std::string());

    if (id == 0)
    {
        return D_CLI_EXIT_TROUBLE;
    }

    row changed = *find_row(read.doc, id);
    int status  = change_definition(_program, _command, _request, read,
                                    before, kind, changed);

    status = (status == D_CLI_EXIT_OK)
             ? apply_row_options(_program, _command, _request.rest, changed)
             : status;

    if (status != D_CLI_EXIT_OK)
    {
        return status;
    }

    trial tried = try_change(read.doc, id, changed);

    return keep_trial(_program, _command, read, _request.where, tried,
                      "changed", before);
}


//------------------------------------------------------------------------------
// the commands' options
//------------------------------------------------------------------------------

/*
add_place_options
  A bit's and a range's options; a change keeps what a range already is.
*/
void
add_place_options(
    kit::command        _command,
    definition_options& _options,
    bool                _adding
)
{
    _command.add_flag("--bit{next}", _options.bit,
                      "a bit: bit N, or the next free")
            .type_name("N")
            .group(DEFINITIONS);
    _command.add_option("--bits", _options.bits,
                        "a range: 4..7, 7..4, or one bit")
            .type_name("RANGE")
            .group(DEFINITIONS);
    _command.add_flag("--lsb{next}", _options.lsb,
                      "a range from bit N, or the next free")
            .type_name("N")
            .group(DEFINITIONS);
    _command.add_option("--width", _options.width,
                        _adding ? "the range's width, with --lsb"
                                : "a range's width, with --lsb or alone; "
                                  "a range made with --lsb keeps its own")
            .type_name("N")
            .default_str(_adding ? "1" : "")
            .group(DEFINITIONS);

    return;
}

/*
add_value_options
  A value's options; a change keeps what a value already is.
*/
void
add_value_options(
    kit::command        _command,
    definition_options& _options,
    bool                _adding
)
{
    _command.add_option("--field", _options.field,
                        "a value in the range ROW")
            .type_name("ROW")
            .group(DEFINITIONS);
    _command.add_option("--number", _options.number,
                        _adding ? "the value's number"
                                : "a value's number, with --field or alone")
            .type_name("N")
            .default_str(_adding ? "0" : "")
            .group(DEFINITIONS);
    _command.add_option("--encoding", _options.encoding,
                        "how the number is held: " +
                        word_list(encoding_words()))
            .type_name("ENCODING")
            .default_str(_adding ? "unsigned" : "")
            .group(DEFINITIONS);
    _command.add_option("--bias", _options.bias,
                        "offset binary's bias")
            .type_name("N")
            .default_str("half the field's range")
            .group(DEFINITIONS);

    return;
}

/*
add_text_options
  A literal's, an expression's and a macro's options; add's group.
*/
void
add_text_options(
    kit::command        _command,
    definition_options& _options,
    bool                _adding
)
{
    _command.add_option("--value", _options.value, "a literal")
            .type_name("N")
            .group(DEFINITIONS);
    _command.add_option("--expr", _options.expr,
                        "an expression over the sheet's symbols")
            .type_name("TEXT")
            .group(DEFINITIONS);
    _command.add_option("--expr-file", _options.expr_file,
                        "the expression, read from FILE")
            .type_name("FILE")
            .group(DEFINITIONS);
    _command.add_option("--body", _options.body, "a macro's body")
            .type_name("TEXT")
            .group(DEFINITIONS);
    _command.add_option("--body-file", _options.body_file,
                        "the body, read from FILE")
            .type_name("FILE")
            .group(DEFINITIONS);
    _command.add_option("--params", _options.params,
                        "the macro's parameters: \"x, v\"")
            .type_name("TEXT")
            .group(DEFINITIONS);
    _command.add_flag("--callable,!--no-callable", _options.callable,
                      "a function-like macro, even with no parameters; or "
                      "not")
            .group(DEFINITIONS);

    if (_adding)
    {
        _command.add_flag("--group", _options.group, "a group")
                .group(DEFINITIONS);
    }

    return;
}

/*
add_row_options
  What a row says besides its definition: its texts, colour and switches,
and how its literal is written.
*/
void
add_row_options(
    kit::command _command,
    row_options& _options
)
{
    _command.add_option("--note", _options.note, "its brief, in the header")
            .type_name("TEXT");
    _command.add_option("--short", _options.short_name,
                        "the abbreviation the grid may show")
            .type_name("TEXT");
    _command.add_option("--prefix", _options.prefix,
                        "what its name, and its rows', begin with")
            .type_name("TEXT");
    _command.add_option("--suffix", _options.suffix,
                        "what its name, and its rows', end with")
            .type_name("TEXT");
    _command.add_option("--colour", _options.colour,
                        "its colour: #RRGGBB, #RRGGBBAA, auto or none")
            .type_name("COLOUR");
    _command.add_flag("--emit,!--no-emit", _options.emit,
                      "put it in the header, or keep it out");
    _command.add_flag("--isolate,!--no-isolate", _options.isolate,
                      "a group's or a range's: ignore the affixes of the "
                      "groups around it, or not");
    _command.add_flag("--name-children,!--no-name-children",
                      _options.name_children,
                      "a group's or a range's: begin its rows' names with "
                      "its own, or not");
    _command.add_flag("--exclusive,!--no-exclusive", _options.exclusive,
                      "a group's or a range's: warn when its rows' bits "
                      "overlap, or not");
    _command.add_option("--radix", _options.radix_word,
                        "how its literal is written: " +
                        word_list(radix_words()))
            .type_name("RADIX");
    _command.add_option("--int-suffix", _options.suffix_word,
                        "its literal's suffix: " + word_list(suffix_words()))
            .type_name("SUFFIX");
    _command.add_option("--digits", _options.digits,
                        "the fewest digits its literal is written with")
            .type_name("N");

    return;
}


//------------------------------------------------------------------------------
// the commands
//------------------------------------------------------------------------------

/*
define_new
  new: a sheet as File > New makes one.
*/
void
define_new(
    kit::program& _program
)
{
    const auto   request = std::make_shared<new_request>();
    kit::command made    = _program.add_command(
        "new",
        "write a new sheet, as File > New makes one",
        [request](kit::program& _p, kit::command _c)
        {
            return run_new(_p, _c, *request);
        });

    made.group(CHANGING);
    made.add_option("--title", request->title, "its title")
        .type_name("TEXT")
        .default_str("untitled");
    made.add_option("--width", request->width, "its width, in bits")
        .type_name("N")
        .default_str("32");
    made.add_option("--prefix", request->prefix,
                    "what every macro name begins with")
        .type_name("TEXT");
    made.add_option("--suffix", request->suffix,
                    "what every macro name ends with")
        .type_name("TEXT");
    made.add_option("--to", request->to, "its shape: sheet or bitfield")
        .type_name("SHAPE")
        .default_str("sheet");
    made.add_option("-o,--out", request->out, "write it to FILE")
        .type_name("FILE")
        .default_str("standard output");

    return;
}

/*
define_import
  import: a sheet from C headers.
*/
void
define_import(
    kit::program& _program
)
{
    const auto   request = std::make_shared<import_request>();
    kit::command made    = _program.add_command(
        "import",
        "make a sheet from C: each #define a row, read as the preprocessor "
        "reads it",
        [request](kit::program& _p, kit::command _c)
        {
            return run_import(_p, _c, *request);
        });

    made.group(CHANGING);
    made.add_option("FILE", request->files,
                    "the headers, - for standard input; the first makes the "
                    "sheet, and each after it is merged in")
        .required();
    made.add_option("--level", request->level,
                    "the language level to read them at")
        .type_name("LEVEL")
        .default_str("c99");
    made.add_option("--model", request->model,
                    "the data model: " + word_list(MODELS))
        .type_name("MODEL")
        .default_str("this machine's");
    made.add_option("--title", request->title, "the sheet's title")
        .type_name("TEXT")
        .default_str("imported");
    made.add_option("--file-name", request->file_name,
                    "the header the sheet writes")
        .type_name("NAME")
        .default_str("TITLE.h");
    made.add_option("--to", request->to, "its shape: sheet or bitfield")
        .type_name("SHAPE")
        .default_str("sheet");
    made.add_option("-o,--out", request->out, "write it to FILE")
        .type_name("FILE")
        .default_str("standard output");

    return;
}

/*
define_add
  add: a row by a bitfield's keys.
*/
void
define_add(
    kit::program& _program
)
{
    const auto   request = std::make_shared<add_request>();
    kit::command made    = _program.add_command(
        "add",
        "add a row, defined as a bitfield's row is",
        [request](kit::program& _p, kit::command _c)
        {
            return run_add(_p, _c, *request);
        });

    made.group(CHANGING);
    add_sheet(made, request->sheet);
    made.add_option("NAME", request->name,
                    "its own name, without the affixes around it")
        .required();
    add_place_options(made, request->def, true);
    add_value_options(made, request->def, true);
    add_text_options(made, request->def, true);
    made.add_option("--in", request->in,
                    "the group or range it goes in, last")
        .type_name("ROW")
        .default_str("the sheet's own rows");
    add_row_options(made, request->rest);
    add_destination(made, request->where);

    return;
}

/*
define_change
  change: a row's definition, or anything else it says.
*/
void
define_change(
    kit::program& _program
)
{
    const auto   request = std::make_shared<change_request>();
    kit::command made    = _program.add_command(
        "change",
        "change a row: its definition, or anything else the inspector shows",
        [request](kit::program& _p, kit::command _c)
        {
            return run_change(_p, _c, *request);
        });

    made.group(CHANGING);
    add_sheet(made, request->sheet);
    made.add_option("ROW", request->row,
                    "the row: its macro name, its own name where one row "
                    "has it, or a group's name")
        .required();
    add_place_options(made, request->def, false);
    add_value_options(made, request->def, false);
    add_text_options(made, request->def, false);
    add_row_options(made, request->rest);
    add_destination(made, request->where);

    return;
}

}  // anonymous namespace


/*
add_edit_commands
  new, import, add and change, in the order the grammar lists them.
*/
void
add_edit_commands(
    kit::program& _program
)
{
    define_new(_program);
    define_import(_program);
    define_add(_program);
    define_change(_program);

    return;
}

}  // namespace cmd
}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
