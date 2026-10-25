/*******************************************************************************
* djinterp [parse]                                         arg_tests_support.hpp
*
* What the argument parser's C++ suites share: a parse to test through the
* C++ face, and what it came to, written out as text.
*   It is the C suites' support (c/arg_tests_support.h) made over the face:
* a section begins a run, defines options and subcommands on the run's
* parser as a program would, names the commands and options it wants
* reported, parses, and compares the run's transcript with the one expected,
* in the C suites' form:
*
*     ERR <status> <exit code>: <message>       when the parse failed
*     cmd <label> n=<uses> miss=[...] subs=[...] order=[...]
*     opt <label> n=<values> res=[...]
*
* one `cmd` line for each command named, in the order named, then one `opt`
* line for each option, a list's items separated by `|`, each argument,
* value and message escaped as escape() writes it. A callback adds lines of
* its own as it runs, which come before the report. Everything a run
* reports it reads through the face -- uses, values, names, the order of a
* parse -- so that a transcript checks the face's readers as well as the
* parse.
*   The interface is written so that it can be had over CLI11 too: the
* programs that wrote the expected transcripts give CLI11's App the same
* case functions, through a run of the same shape, so that where a section
* is CLI11's, its definitions are the suite's own text, and its transcript
* is CLI11's (START-HERE, section 8).
*   A run's parser writes help, versions and failure messages into the run's
* strings rather than to the standard streams, so that a section can read
* them; and a run frees everything it holds when it ends, so that a section
* built with AddressSanitizer fails on a leak.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/arg_tests_support.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.19
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  RECORDS
    -------
    1.  Limits
         1.  CASE_ARGS
    2.  Case tables
         1.  file_entry
         2.  variable_entry
         3.  run
         4.  define_fn / step_fn
         5.  case_entry
2.  SECTIONS
    --------
    1.  Comparing
         1.  text_holds
    2.  Running tables
         1.  cases_hold
         2.  steps_hold
3.  TEXT
    ----
    1.  Writing
         1.  escape / render
         2.  number
4.  VALIDATORS
    ----------
    1.  Transcripts
         1.  transcribe
5.  FILES
    -----
    1.  The scratch directory
         1.  scratch_directory
    2.  The environment
         1.  set_variable
*/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_ARG_TESTS_SUPPORT_HPP
#define DJINTERP_PARSE_CLI_ARG_PARSER_ARG_TESTS_SUPPORT_HPP 1

// std
#include <cstddef>    // std::size_t
#include <istream>    // std::istream
#include <string>     // std::string, std::wstring
#include <utility>    // std::pair
#include <vector>     // std::vector
// djinterp
#include "../../../../../inc/djinterp/djinterp.hpp"  // framework root
#include "../../../../../inc/djinterp/parse/cli/arg_parser/arg_parser.hpp"  // the face
#include "./c/arg_tests_files.h"  // d_tests_arg_files


namespace djinterp
{
namespace parse
{
namespace cli
{
namespace testing
{

//==============================================================================
// 1.  RECORDS
//==============================================================================


// 1.1    Limits
//------------------------------------------------------------------------------
// 1.1.1
// CASE_ARGS
//   constant: the room for the arguments of one case of a case table, which
// end at the first NULL or fill the room.
const std::size_t CASE_ARGS = 20u;

// 1.2    Case tables
//------------------------------------------------------------------------------
// 1.2.1
// file_entry
//   struct: a file a case parses with: its name, relative to the scratch
// directory the case works in, and the text it holds. A list of them ends
// at a NULL name.
struct file_entry
{
    const char* name;
    const char* content;
};

// 1.2.2
// variable_entry
//   struct: an environment variable a case parses with: its name and its
// value, which is not empty, since Windows cannot hold an empty one. A list
// of them ends at a NULL name.
struct variable_entry
{
    const char* name;
    const char* value;
};

// 1.2.3
// run
//   class: one test parse: the parser, made as CLI11's App("d", "prog") with
// suggestions off and its output and error output going to the run's
// strings; the commands and options reported, in the order named, the root
// first under "app"; and the transcript. `failed` is set when a definition
// a run makes or watches did not come to an option or a command, so that
// the section fails rather than reports less.
//   The parser points into the run, so a run is never copied: a section
// declares one and passes it by reference.
class run
{
public:
    run();
    ~run();

    // the parser, and the handle on its root
    cli::arg_parser& app() D_NOEXCEPT;
    cli::command     root() D_NOEXCEPT;

    // what is reported: an option or a command named under a label, each
    // returned so that a definition can be wrapped in it; watch_all also
    // makes the option keep every value it is given, as CLI11's
    // MultiOptionPolicy::TakeAll, as the checks against CLI11 made the
    // options they gave several values; the make calls define an option, a
    // flag, a subcommand or an option group with no description and watch
    // it -- a subcommand and a group under their names
    cli::option  watch(const char*        _label,
                       const cli::option& _option);
    cli::option  watch_all(const char*        _label,
                           const cli::option& _option);
    cli::command watch(const char*         _label,
                       const cli::command& _command);
    cli::option  make_option(const char*         _label,
                             const cli::command& _command,
                             const char*         _names);
    cli::option  make_all(const char*         _label,
                          const cli::command& _command,
                          const char*         _names);
    cli::option  make_flag(const char*         _label,
                           const cli::command& _command,
                           const char*         _names);
    cli::command make_subcommand(const cli::command& _parent,
                                 const char*         _name);
    cli::command make_group(const cli::command& _parent,
                            const char*         _name);

    // the label a command or an option is reported under -- what a
    // callback's note names it by -- or "?" for one the run does not report
    const char*  label(const cli::option& _option) const D_NOEXCEPT;
    const char*  label(const cli::command& _command) const D_NOEXCEPT;

    // a line of the transcript, for callbacks; the newline is added
    void         note(const std::string& _line);

    /**
     * @brief Parses arguments with the run's parser, as main's after the
     *        program's name, and adds the ERR line, when there is one, and
     *        the report to the transcript.
     *
     * @param[in] _args   the arguments; may be NULL when `_count` is 0.
     * @param[in] _count  how many.
     */
    void         parse(const char* const* _args,
                       std::size_t        _count);
    /**
     * @brief Parses as parse() does, adding nothing to the transcript, for a
     *        case that writes what the parse came to in its own way.
     *
     * @param[in] _args   the arguments; may be NULL when `_count` is 0.
     * @param[in] _count  how many.
     * @return whether the parse came to CLI11's success: nothing thrown.
     */
    bool         parse_quiet(const char* const* _args,
                             std::size_t        _count);
    /**
     * @brief Parses a command line held in one string, as CLI11's
     *        parse(commandline, program_name_included) does, and adds the
     *        ERR line, when there is one, and the report.
     *
     * @param[in] _line                   the line.
     * @param[in] _program_name_included  whether the line starts with the
     *                                    program's name.
     */
    void         parse_line(const std::string& _line,
                            bool               _program_name_included);
    /**
     * @brief Parses arguments given last first, as CLI11's
     *        parse(std::vector<std::string>&) does, and adds the ERR line,
     *        when there is one, and the report; the vector is left as the
     *        parse leaves it.
     *
     * @param[in,out] _args  the arguments, the last one first.
     */
    void         parse_vector(std::vector<std::string>& _args);
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    /**
     * @brief Parses arguments given last first through the overload that
     *        consumes them, CLI11's parse(std::vector<std::string>&&), and
     *        adds the ERR line, when there is one, and the report.
     *
     * @param[in,out] _args  the arguments, the last one first; moved from.
     */
    void         parse_moved(std::vector<std::string>& _args);
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    /**
     * @brief Parses configuration text from a stream, as CLI11's
     *        parse_from_stream does, and adds the ERR line, when there is
     *        one, and the report.
     *
     * @param[in,out] _input  the stream, read to its end.
     */
    void         parse_stream(std::istream& _input);
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)
    /**
     * @brief Parses wide arguments, as wmain's after the program's name,
     *        CLI11's parse(argc, const wchar_t* const* argv), and adds the
     *        ERR line, when there is one, and the report.
     *
     * @param[in] _args   the arguments; may be NULL when `_count` is 0.
     * @param[in] _count  how many.
     */
    void         parse_wide(const wchar_t* const* _args,
                            std::size_t           _count);
    /**
     * @brief Parses a wide command line, CLI11's parse(std::wstring,
     *        program_name_included), and adds the ERR line, when there is
     *        one, and the report.
     *
     * @param[in] _line                   the line.
     * @param[in] _program_name_included  whether the line starts with the
     *                                    program's name.
     */
    void         parse_wide_line(const std::wstring& _line,
                                 bool                _program_name_included);
#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV
    /**
     * @brief Parses arguments as parse() does, then again as main does with
     *        D_ARG_PARSE -- CLI11's CLI11_PARSE -- and writes what main
     *        comes to: a line `main` and the code it returns, -1 when the
     *        parse goes on, then what that parse wrote to the output and to
     *        the error output, each on a line after `out` or `err`,
     *        escaped.
     *
     * @param[in] _args   the arguments; may be NULL when `_count` is 0.
     * @param[in] _count  how many.
     */
    void         parse_main(const char* const* _args,
                            std::size_t        _count);
    /**
     * @brief Parses arguments once, as main does with D_ARG_PARSE --
     *        CLI11's CLI11_PARSE -- and writes what main comes to, as
     *        parse_main() writes it: a ported program's own parse, which
     *        runs its callbacks once.
     *
     * @param[in] _args   the arguments; may be NULL when `_count` is 0.
     * @param[in] _count  how many.
     * @return the code main returns; -1 when the parse goes on.
     */
    int          parse_as_main(const char* const* _args,
                               std::size_t        _count);

    // the ERR line of the last parse, when it did not succeed, and the
    // report, apart
    void         record();
    void         report();

    /**
     * @brief Writes the definition error the parser keeps, when there is
     *        one, as a line `THROWN`, its name, `:` and its message,
     *        escaped, and clears it: what CLI11 throws where the definition
     *        is made, written as a step's last line.
     */
    void         refused();

    /**
     * @brief Ends as main would after the last parse, as CLI11's exit()
     *        does, and writes what came of it: a line `exit` and the code,
     *        then what was written to the output and to the error output,
     *        when there was any, each on a line after `out` or `err`,
     *        escaped.
     */
    void         exit();

    // reading
    const std::string& text() const D_NOEXCEPT;
    const std::string& out() const D_NOEXCEPT;
    const std::string& err() const D_NOEXCEPT;
    bool               failed() const D_NOEXCEPT;
    void               fail() D_NOEXCEPT;

private:
    D_DELETED_FN(run(const run& _other))
    D_DELETED_FN(run& operator=(const run& _other))

    void report_command(const std::string&  _label,
                        const cli::command& _command);
    void report_option(const std::string& _label,
                       const cli::option& _option);
    void set_arguments(const char* const* _args,
                       std::size_t        _count);
    int  end_as_main(std::size_t _count);

    std::string                                        m_out;
    std::string                                        m_err;
    cli::arg_parser                                    m_app;
    std::vector<std::pair<std::string, cli::command> > m_commands;
    std::vector<std::pair<std::string, cli::option> >  m_options;
    std::vector<const char*>                           m_argv;
#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)
    std::vector<const wchar_t*>                        m_wargv;
#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV
    std::string                                        m_text;
    cli::status                                        m_status;
    bool                                               m_failed;
};

// 1.2.4
// define_fn / step_fn
//   types: a case's definitions, made on the run's parser, naming what it
// reports with the run's watch and make calls; and a step, which makes its
// definitions, parses and writes its own lines.
typedef void (*define_fn)(run& _run);
typedef void (*step_fn)(run& _run);

// 1.2.5
// case_entry
//   struct: one case of a case table: its tag; the function that makes its
// definitions; the arguments it parses, which end at the first NULL; the
// environment variables set while it parses, NULL for none; and the files
// it parses with, NULL for none, made in a scratch directory the case works
// in.
struct case_entry
{
    const char*           tag;
    define_fn             define;
    const char*           args[CASE_ARGS];
    const variable_entry* variables;
    const file_entry*     files;
};


//==============================================================================
// 2.  SECTIONS
//==============================================================================


// 2.1    Comparing
//------------------------------------------------------------------------------
// 2.1.1
// text_holds
/**
 * @brief Compares a transcript with the one expected, and shows both, line
 *        by line, when they differ.
 *
 * @param[in] _name      what the transcript is, for a printed mismatch.
 * @param[in] _written   the transcript written.
 * @param[in] _expected  the transcript expected; NULL reads as empty.
 * @return whether the two are the same, byte for byte.
 */
bool text_holds(const char*        _name,
                const std::string& _written,
                const char*        _expected);

// 2.2    Running tables
//------------------------------------------------------------------------------
// 2.2.1
// cases_hold
/**
 * @brief Runs each case of a table on a run of its own, after a line
 *        naming it -- `<tag> [<arguments>]` -- and compares the whole with
 *        the transcript expected.
 *
 * @param[in] _name      the section's transcript's name, for a mismatch.
 * @param[in] _cases     the cases.
 * @param[in] _count     how many.
 * @param[in] _expected  the transcript expected.
 * @return whether every case ran and the transcript was as expected.
 */
bool cases_hold(const char*       _name,
                const case_entry* _cases,
                std::size_t       _count,
                const char*       _expected);

// 2.2.2
// steps_hold
/**
 * @brief Runs each step of a table on a run of its own and compares their
 *        transcripts, one after another, with the transcript expected.
 *
 * @param[in] _name      the section's transcript's name, for a mismatch.
 * @param[in] _steps     the steps.
 * @param[in] _count     how many.
 * @param[in] _expected  the transcript expected.
 * @return whether every step ran and the transcript was as expected.
 */
bool steps_hold(const char*    _name,
                const step_fn* _steps,
                std::size_t    _count,
                const char*    _expected);


//==============================================================================
// 3.  TEXT
//==============================================================================


// 3.1    Writing
//------------------------------------------------------------------------------
// 3.1.1
// escape / render
//   functions: a text as a transcript line holds it -- a tab as `\t`, a
// newline as `\n`, any other control byte as `\x` and two hexadecimal
// digits, and, by escape() but not by render(), a backslash as `\\` --
// so that a line stays one line and a backslash is told from an escape.
// std::bad_alloc when the text cannot be held.
std::string escape(const cli::text& _text);
std::string render(const cli::text& _text);

// 3.1.2
// number
//   functions: a number as decimal text, as a transcript writes counts.
// std::bad_alloc when the text cannot be held.
std::string number(long _value);
std::string number(unsigned long _value);


//==============================================================================
// 4.  VALIDATORS
//==============================================================================


// 4.1    Transcripts
//------------------------------------------------------------------------------
// 4.1.1
// transcribe
/**
 * @brief Appends a validator's transcript over values to a run's: its
 *        description's line, `desc` and the description in angle
 *        brackets, then a line for each value.
 *
 * @note A value's line is the value in brackets, then `-> pass` and the
 *       value it became, `-> fail` and why, or, where the validator stopped
 *       with an error of its own -- CLI11's validators throw there --
 *       `-> status` and the message, each in angle brackets. Integers are
 *       read under CLI11's rule for leading zeros.
 *
 * @param[in,out] _run        the run whose transcript is appended to.
 * @param[in]     _validator  the validator.
 * @param[in]     _values     the values.
 * @param[in]     _count      how many.
 */
void transcribe(run&                  _run,
                const cli::validator& _validator,
                const char* const*    _values,
                std::size_t           _count);


//==============================================================================
// 5.  FILES
//==============================================================================


// 5.1    The scratch directory
//------------------------------------------------------------------------------
// 5.1.1
// scratch_directory
//   class: the C suites' scratch directory (c/arg_tests_files.h), entered by
// its constructor and left -- everything in it removed -- by its
// destructor, with the files a case lists made in it. `ok()` is whether
// everything asked of it was done.
class scratch_directory
{
public:
    explicit scratch_directory(const file_entry* _files = D_NULLPTR);
    ~scratch_directory();

    bool make(const char* _name,
              const char* _content);
    bool make_folder(const char* _name);
    bool ok() const D_NOEXCEPT;

private:
    D_DELETED_FN(scratch_directory(const scratch_directory& _other))
    D_DELETED_FN(scratch_directory& operator=(
        const scratch_directory& _other))

    ::d_tests_arg_files m_files;
    bool                m_entered;
};

// 5.2    The environment
//------------------------------------------------------------------------------
// 5.2.1
// set_variable
/**
 * @brief Sets, or removes, an environment variable of the process.
 *
 * @param[in] _name   the variable.
 * @param[in] _value  its value; NULL removes it.
 * @return whether it was done.
 */
bool set_variable(const char* _name,
                  const char* _value);

}  // namespace testing
}  // namespace cli
}  // namespace parse
}  // namespace djinterp


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_ARG_TESTS_SUPPORT_HPP
