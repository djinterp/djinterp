/*******************************************************************************
* bitwisedom [bitwisedom]                                       cli_internal.hpp
*
* What the command line's sources share: sheets read and written back, rows
* and values as a command line names them, and output.
*   A SHEET operand is a path, `-` for standard input, or `example:NAME` for
* a built-in example (GRAMMAR.md, R13); read, it is any of the three JSON
* shapes exchange.hpp reads, and remembers which, so that a changed sheet is
* written back in the shape it came in. A changed sheet goes back where it
* came from, whole or not at all; `-o FILE` sends it elsewhere, standard
* input's goes to standard output, and an example, which has nowhere to go
* back to, must be given `-o`.
*   A change that would give the sheet an error it does not have -- an
* expression naming a row that is gone, a value with no range -- is refused,
* and nothing written, unless `-f` says to write it anyway: the window can
* undo a change, and a command line cannot.
*   Every diagnostic about a sheet is placed at it -- `uart.json: error:
* UART_CR_EN: ...` -- and one about a place in its text at that place --
* `uart.json:3:7: error: ...` -- as compilers place theirs.
*
* path:      /src/djinterp/tools/bitwisedom/cli/cli_internal.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.10.22
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SHEETS IN AND OUT
    -----------------
    1.  Reading one
         1.  source
         2.  read_source
    2.  Where it goes
         1.  destination
         2.  add_destination
         3.  add_sheet
    3.  Writing it
         1.  sheet_text
         2.  new_errors
         3.  store
2.  ROWS AND VALUES
    ---------------
    1.  Rows
         1.  row_named
         2.  shown_name
    2.  Numbers
         1.  read_value / read_number
    3.  Words
         1.  choice_of / word_list / nearest
         2.  bad_word
         3.  level_words
    4.  Options
         1.  given
3.  OUTPUT
    ------
    1.  Text
         1.  emit / say
         2.  table
         3.  plural
    2.  Formats
         1.  add_format / json_wanted
    3.  Diagnostics
         1.  report_issues
         2.  report_new_issues
         3.  report_lines
    4.  Dates
         1.  today
4.  COMMANDS
    --------
    1.  Headings
         1.  READING / CHANGING
    2.  gen
         1.  gen_request
         2.  add_gen_options
         3.  run_gen
    3.  The commands
         1.  add_read_commands / add_edit_commands / add_row_commands
*/

#ifndef BITWISEDOM_CLI_CLI_INTERNAL_HPP
#define BITWISEDOM_CLI_CLI_INTERNAL_HPP 1

// FLOOR: below C++17 this file is empty, rather than an error.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <string>  // std::string
#include <vector>  // std::vector
// djinterp
#include "djinterp/djinterp.hpp"            // framework root
#include "djinterp/parse/cli/cli.hpp"       // the command-line kit
// bitwisedom
#include "djinterp/tools/bitwisedom/exchange.hpp"  // json_shape
#include "djinterp/tools/bitwisedom/resolve.hpp"   // resolution
#include "djinterp/tools/bitwisedom/sheet.hpp"     // sheet, wide


namespace bitwisedom
{
namespace cmd
{

// kit
//   namespace: djinterp's command-line kit, its C++ face.
namespace kit = ::djinterp::parse::cli;


//==============================================================================
// 1.  SHEETS IN AND OUT
//==============================================================================


// 1.1    Reading one
//------------------------------------------------------------------------------
// 1.1.1
// source
//   struct: a SHEET operand, read: the operand as given, the sheet, the shape
// its JSON came in, and whether it is a built-in example.
struct source
{
    std::string operand;
    sheet       doc;
    json_shape  shape   = json_shape::sheet;
    bool        example = false;
};

// 1.1.2
// read_source
/**
 * @brief Reads a SHEET operand.
 *
 * @param[in]  _program  the program, which says what went wrong.
 * @param[in]  _operand  the operand: a path, `-`, or `example:NAME`.
 * @param[out] _source   the sheet, when it was read.
 * @return `true` if it was read; `false`, having said why at the operand.
 */
D_NODISCARD bool  read_source(kit::program&      _program,
                              const std::string& _operand,
                              source&            _source);

// 1.2    Where it goes
//------------------------------------------------------------------------------
// 1.2.1
// destination
//   struct: where a changed sheet goes: `-o FILE`, empty for back where it
// came from; `-n`, nowhere; and `-f`, there even with errors the change
// gave it.
struct destination
{
    std::string out;
    bool        dry_run = false;
    bool        force   = false;
};

// 1.2.2
// add_destination
//   definition: a changing command's `-o, --out=FILE`, `-n, --dry-run` and
// `-f, --force`.
void  add_destination(kit::command _command,
                      destination& _destination);

// 1.2.3
// add_sheet
//   definition: a changing command's SHEET operand, required: the sheet
// written back where it was read.
void  add_sheet(kit::command _command,
                std::string& _sheet);

// 1.3    Writing it
//------------------------------------------------------------------------------
// 1.3.1
// sheet_text
//   function: a sheet as the text of a shape: a sheet file, a bitfield, or
// an export, whose header is written with `_date`.
D_NODISCARD std::string  sheet_text(const sheet&       _sheet,
                                    json_shape         _shape,
                                    const std::string& _date);

// 1.3.2
// new_errors
//   function: how many errors a change gave a sheet: for each row, those it
// has beyond what it had -- so an error reworded, its bits moved, is not
// one, and one a removed row took with it is not counted against another.
D_NODISCARD int  new_errors(const resolution& _before,
                            const resolution& _after);

// 1.3.3
// store
/**
 * @brief Writes a changed sheet where it goes, and says what changed.
 *
 * @note The issues the change brought are said, always. A change that gave
 *       the sheet errors (new_errors) is refused, nothing written, unless
 *       `-f` was given -- in a dry run too, which says what a real run would.
 *       The change is said as a note at `-v`, or, with `-n`, always, the
 *       file left as it was.
 *
 * @param[in] _program      the program.
 * @param[in] _command      the command, for a usage error.
 * @param[in] _source       the sheet as it was read, now changed.
 * @param[in] _destination  where it goes.
 * @param[in] _change       what changed, as a note says it.
 * @param[in] _before       the sheet's resolution before the change.
 * @return D_CLI_EXIT_OK; D_CLI_EXIT_TROUBLE, said, when it was refused or
 *         could not be written.
 */
D_NODISCARD int  store(kit::program&      _program,
                       kit::command       _command,
                       const source&      _source,
                       const destination& _destination,
                       const std::string& _change,
                       const resolution&  _before);


//==============================================================================
// 2.  ROWS AND VALUES
//==============================================================================


// 2.1    Rows
//------------------------------------------------------------------------------
// 2.1.1
// row_named
/**
 * @brief Finds the row a ROW operand names: a symbol's macro name, a row's
 *        own name where one row has it, or a group's name.
 *
 * @param[in] _program     the program.
 * @param[in] _command     the command, for the usage error.
 * @param[in] _resolution  the sheet's resolution.
 * @param[in] _name        the name.
 * @param[in] _option      what gave the name -- `--in`, `ROW` -- for the
 *                         error.
 * @return the row's id; 0, the error said, when none or several have it.
 */
D_NODISCARD int  row_named(kit::program&      _program,
                           kit::command       _command,
                           const resolution&  _resolution,
                           const std::string& _name,
                           const std::string& _option);

// 2.1.2
// shown_name
//   function: a row as a person names it: a symbol by its macro name, a
// group by its own.
D_NODISCARD std::string  shown_name(const resolved_row& _row);

// 2.2    Numbers
//------------------------------------------------------------------------------
// 2.2.1
// read_value / read_number
//   functions: an integer as the grammar writes one (GRAMMAR.md, section 4)
// -- decimal unless 0x, 0o or 0b says otherwise, a leading zero not octal,
// `_` between digits, `-` before a negative one -- of any width, a negative
// one as its 64-bit two's complement; or held in 64 bits, signed. False,
// with why for read_value, for anything else.
D_NODISCARD bool  read_value(const std::string& _text,
                             wide&              _value,
                             bool&              _negative,
                             std::string&       _error);
D_NODISCARD bool  read_number(const std::string& _text,
                              i64&               _value);

// 2.3    Words
//------------------------------------------------------------------------------
// 2.3.1
// choice_of / word_list / nearest
//   functions: a word's place among a choice's words, -1 for none; the
// words as a list says them, "c or cpp"; and the one of them a mistyped
// word most likely meant, "" when none is near enough to say.
D_NODISCARD int          choice_of(const std::string&              _word,
                                   const std::vector<std::string>& _words);
D_NODISCARD std::string  word_list(const std::vector<std::string>& _words);
D_NODISCARD std::string  nearest(const std::string&              _word,
                                 const std::vector<std::string>& _words);

// 2.3.2
// bad_word
//   diagnostic: the usage error of an option given a word it does not take
// -- the words it does, and the one meant, where one is near enough -- and
// D_CLI_EXIT_TROUBLE, for the work to return.
D_NODISCARD int  bad_word(kit::program&                   _program,
                          kit::command                    _command,
                          const std::string&              _option,
                          const std::string&              _word,
                          const std::vector<std::string>& _words);

// 2.3.3
// level_words
//   function: the language levels' words, as the sheet file writes them:
// c99 ... cpp23, in standard's order.
D_NODISCARD std::vector<std::string>  level_words();

// 2.4    Options
//------------------------------------------------------------------------------
// 2.4.1
// given
//   function: whether an option was given in the last parse, by any of its
// names: `--emit` for `--emit` and `--no-emit` alike.
D_NODISCARD bool  given(kit::command       _command,
                        const std::string& _option);


//==============================================================================
// 3.  OUTPUT
//==============================================================================


// 3.1    Text
//------------------------------------------------------------------------------
// 3.1.1
// emit / say
//   output: text to the program's output, its results' stream; and to its
// errors' stream, where what is said about the work goes (GRAMMAR.md, R17).
void  emit(kit::program&      _program,
           const std::string& _text);
void  say(kit::program&      _program,
          const std::string& _text);

// 3.1.2
// table
//   function: rows of columns as text a person reads: each column as wide as
// its widest cell, two spaces between, no blanks at a line's end.
D_NODISCARD std::string  table(
                             const std::vector<std::vector<std::string>>&
                                 _rows);

// 3.1.3
// plural
//   function: a count and its noun, "1 error", "2 errors".
D_NODISCARD std::string  plural(long        _count,
                                const char* _noun);

// 3.2    Formats
//------------------------------------------------------------------------------
// 3.2.1
// add_format / json_wanted
//   definition: `--format=text|json`, the text a person reads or the JSON
// a script reads; and whether it asked for JSON -- false, the usage error
// said and `_ok` false, for a word it does not take.
void              add_format(kit::command _command,
                             std::string& _format);
D_NODISCARD bool  json_wanted(kit::program&      _program,
                              kit::command       _command,
                              const std::string& _format,
                              bool&              _ok);

// 3.3    Diagnostics
//------------------------------------------------------------------------------
// 3.3.1
// report_issues
//   diagnostics: a sheet's issues, each at the sheet and named for its row:
// `uart.json: warning: UART_CR_EN: ...`. Returns how many are errors.
int   report_issues(kit::program&      _program,
                    const std::string& _operand,
                    const resolution&  _resolution);

// 3.3.2
// report_new_issues
//   diagnostics: the issues a change brought -- those the sheet has after it
// and did not before, as report_issues says them -- so that a row removed
// that an expression named is heard of when it happens.
void  report_new_issues(kit::program&      _program,
                        const std::string& _operand,
                        const resolution&  _before,
                        const resolution&  _after);

// 3.3.3
// report_lines
//   diagnostics: lines of the model's -- a read's notes, an import's -- each
// placed where it says it is: `line 3: ...` at line 3 of the file, `line 3,
// column 7: ...` there.
void  report_lines(kit::program&                   _program,
                   int                             _severity,
                   const std::string&              _file,
                   const std::vector<std::string>& _lines);

// 3.4    Dates
//------------------------------------------------------------------------------
// 3.4.1
// today
//   function: the local date as a banner writes it, YYYY.MM.DD.
D_NODISCARD std::string  today();


//==============================================================================
// 4.  COMMANDS
//==============================================================================


// 4.1    Headings
//------------------------------------------------------------------------------
// 4.1.1
// READING / CHANGING
//   constants: the headings help lists the commands under.
inline constexpr const char* READING  = "Reading a sheet";
inline constexpr const char* CHANGING = "Making and changing a sheet";

// 4.2    gen
//------------------------------------------------------------------------------
// 4.2.1
// gen_request
//   struct: what `gen` was asked: the sheet, the files to write, the
// generator's settings over the sheet's, the banner's date, and whether
// only to check. `example` is bitwisedom-gen's `--example NAME`.
struct gen_request
{
    std::string sheet;
    std::string example;
    std::string out;
    std::string self_test;
    std::string bitfield;
    std::string everything;
    std::string lang;
    std::string form;
    std::string style;
    std::string level;
    std::string word;
    std::string date;
    bool        check = false;
};

// 4.2.2
// add_gen_options
//   definitions: gen's options, as bitwisedom-gen always took them.
void  add_gen_options(kit::command _command,
                      gen_request& _request);

// 4.2.3
// run_gen
/**
 * @brief Does gen's work.
 *
 * @param[in] _program  the program.
 * @param[in] _command  the command, for its usage errors.
 * @param[in] _request  what was asked.
 * @return D_CLI_EXIT_OK; D_CLI_EXIT_NO when the sheet has errors, the
 *         header written without them; D_CLI_EXIT_TROUBLE.
 */
D_NODISCARD int  run_gen(kit::program&      _program,
                         kit::command       _command,
                         const gen_request& _request);

// 4.3    The commands
//------------------------------------------------------------------------------
// 4.3.1
// add_read_commands / add_edit_commands / add_row_commands
//   definitions: the commands that read a sheet; those that make one or a
// row -- new, import, add, change; and those that rearrange its rows or
// change the rest of it -- duplicate, remove, rename, move, sort, set, get
// and op -- each with its work, in that order.
void  add_read_commands(kit::program& _program);
void  add_edit_commands(kit::program& _program);
void  add_row_commands(kit::program& _program);


}  // namespace cmd
}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // BITWISEDOM_CLI_CLI_INTERNAL_HPP
