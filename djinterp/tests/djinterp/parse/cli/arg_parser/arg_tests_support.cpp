/*******************************************************************************
* djinterp [parse]                                         arg_tests_support.cpp
*
* What the argument parser's C++ suites share: the run, its report, the
* section runners and the text they write.
*   Everything a run reports is read through the C++ face -- a command's
* uses, the arguments it left, the subcommands it parsed and the options in
* the order met; an option's values -- so that the transcripts the suites
* compare check the face's readers as they check the parse.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/arg_tests_support.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.19
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./arg_tests_support.hpp"  // corresponding header
// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf
#include <cstdlib>  // setenv, unsetenv, _putenv_s
#include <istream>  // std::istream
#include <sstream>  // std::ostringstream
#include <string>   // std::string, std::wstring
#include <utility>  // std::make_pair, std::move
#include <vector>   // std::vector
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
// FILE-LOCAL DEFINITIONS
//==============================================================================

namespace
{

/**
 * @brief Appends a text to a string, escaped as a transcript holds it.
 *
 * @param[in,out] _out        the string, appended to.
 * @param[in]     _text       the text.
 * @param[in]     _backslash  whether a backslash is doubled.
 *
 * A tab and a newline become their escapes, any other control byte `\\x`
 * and two lower-case hexadecimal digits, as the C suites' writer makes
 * them, so that a transcript of either kind reads the same.
 */
void
append_escaped(
    std::string&     _out,
    const cli::text& _text,
    bool             _backslash
)
{
    static const char DIGITS[] = "0123456789abcdef";

    // each byte
    for (cli::text::const_iterator at = _text.begin();
         at != _text.end();
         ++at)
    {
        const unsigned char byte = static_cast<unsigned char>(*at);

        // a control character as its escape, a backslash doubled when
        // asked, anything else as itself
        if (byte == '\t')
        {
            _out += "\\t";
        }
        else if (byte == '\n')
        {
            _out += "\\n";
        }
        else if ( (byte < 0x20u) ||
                  (byte == 0x7fu) )
        {
            _out += "\\x";
            _out += DIGITS[byte >> 4u];
            _out += DIGITS[byte & 0x0fu];
        }
        else if ( (_backslash) &&
                  (byte == '\\') )
        {
            _out += "\\\\";
        }
        else
        {
            _out += static_cast<char>(byte);
        }
    }

    return;
}

/**
 * @brief Prints a text under a heading, each line indented and marked, so
 *        that an empty line and a line's trailing spaces can be seen.
 *
 * @param[in] _heading  the heading.
 * @param[in] _text     the text.
 */
void
show(
    const char*        _heading,
    const std::string& _text
)
{
    std::string::size_type start = 0u;

    std::printf("    %s\n",
                _heading);

    // each line, the last whether or not a newline ends it
    while (start < _text.size())
    {
        std::string::size_type end = _text.find('\n',
                                                start);

        // the last line, without its newline
        if (end == std::string::npos)
        {
            end = _text.size();
        }

        std::printf("      |%s|\n",
                    _text.substr(start,
                                 end - start).c_str());
        start = end + 1u;
    }

    return;
}

/**
 * @brief How many arguments a case gives: those before its first NULL, or
 *        all CASE_ARGS of them.
 *
 * @param[in] _case  the case.
 * @return the count.
 */
std::size_t
case_count(
    const case_entry& _case
)
{
    std::size_t count = 0u;

    // the arguments up to the first NULL
    while ( (count < CASE_ARGS) &&
            (_case.args[count] != D_NULLPTR) )
    {
        ++count;
    }

    return count;
}

/**
 * @brief Sets, or restores, the environment variables a case parses with.
 *
 * @param[in] _variables  the variables, ending at a NULL name; NULL for
 *                        none.
 * @param[in] _set        whether to set them, or to remove them.
 * @return whether each was done.
 */
bool
case_variables(
    const variable_entry* _variables,
    bool                  _set
)
{
    bool done = true;

    // each variable of the list
    for (const variable_entry* at = _variables;
         ( (at != D_NULLPTR) &&
           (at->name != D_NULLPTR) );
         ++at)
    {
        done = set_variable(at->name,
                            (_set) ? at->value : D_NULLPTR) && done;
    }

    return done;
}

/**
 * @brief Runs one case of a table on a run of its own and appends what came
 *        of it.
 *
 * @param[in,out] _out   the section's transcript, appended to.
 * @param[in]     _case  the case.
 * @return whether the case ran: its files and variables made, and its
 *         definitions and watches all came to something.
 */
bool
case_run(
    std::string&      _out,
    const case_entry& _case
)
{
    scratch_directory files(_case.files);
    run               test;
    const std::size_t count = case_count(_case);

    _case.define(test);
    _out += _case.tag;
    _out += " [";

    // each argument, after a space when one comes before it
    for (std::size_t index = 0u; index < count; ++index)
    {
        _out += (index > 0u) ? " " : "";
        append_escaped(_out,
                       cli::text(_case.args[index]),
                       true);
    }

    _out += "]\n";

    const bool set = case_variables(_case.variables,
                                    true);

    test.parse(_case.args,
               count);

    const bool unset = case_variables(_case.variables,
                                      false);
    const bool held  = ( (files.ok())     &&
                         (set)            &&
                         (unset)          &&
                         (!test.failed()) );

    _out += test.text();

    // a case that did not run says so
    if (!held)
    {
        std::printf("    %s: the case failed: a definition, a watch, a "
                    "variable or a file\n",
                    _case.tag);
    }

    return held;
}

/**
 * @brief What main does with D_ARG_PARSE: parses, and ends unless the parse
 *        goes on.
 *
 * @param[in,out] _app   the parser.
 * @param[in]     _argc  the argument count.
 * @param[in]     _argv  the arguments, the program's name first.
 * @return the code main would return; -1 when the parse goes on.
 */
int
main_like(
    cli::arg_parser&   _app,
    int                _argc,
    const char* const* _argv
)
{
    D_ARG_PARSE(_app,
                _argc,
                _argv);

    return -1;
}

}  // namespace

//==============================================================================
// 1.  THE RUN
//==============================================================================

/*
run::run
  The strings come before the parser, so that the sinks it writes through
outlive it; the root is reported first, under "app".
*/
run::run()
    : m_out(),
      m_err(),
      m_app("d",
            "prog"),
      m_commands(),
      m_options(),
      m_argv(),
#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)
      m_wargv(),
#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV
      m_text(),
      m_status(),
      m_failed(false)
{
    m_app.output(cli::string_sink(m_out),
                 cli::string_sink(m_err));
#if (D_INTERNAL_ARG_PARSER_SUGGESTIONS == 1)
    m_app.suggestions(false);
#endif
    (void)watch("app",
                cli::command(m_app));
}

/*
run::~run
  The parser goes before the strings it wrote to, as members go in the
reverse of their order.
*/
run::~run()
{}

/*
run::app / run::root
  The parser, and the handle a parser converts to.
*/
cli::arg_parser&
run::app() D_NOEXCEPT
{
    return m_app;
}

cli::command
run::root() D_NOEXCEPT
{
    return cli::command(m_app);
}

/*
run::watch
  A handle on nothing -- a definition that failed -- fails the run, and is
not reported.
*/
cli::option
run::watch(
    const char*        _label,
    const cli::option& _option
)
{
    // a definition that failed
    if (!_option.valid())
    {
        m_failed = true;

        return _option;
    }

    m_options.push_back(std::make_pair(std::string(_label),
                                       _option));

    return _option;
}

cli::option
run::watch_all(
    const char*        _label,
    const cli::option& _option
)
{
    cli::option watched = watch(_label,
                                _option);

    watched.take_all();

    return watched;
}

cli::command
run::watch(
    const char*         _label,
    const cli::command& _command
)
{
    // a definition that failed
    if (!_command.valid())
    {
        m_failed = true;

        return _command;
    }

    m_commands.push_back(std::make_pair(std::string(_label),
                                        _command));

    return _command;
}

/*
run::make_option and kin
  CLI11's add_option, add_flag, add_subcommand and add_option_group on the
command given, through the handle, then watched.
*/
cli::option
run::make_option(
    const char*         _label,
    const cli::command& _command,
    const char*         _names
)
{
    cli::command parent = _command;

    return watch(_label,
                 parent.add_option(_names));
}

cli::option
run::make_all(
    const char*         _label,
    const cli::command& _command,
    const char*         _names
)
{
    cli::command parent = _command;

    return watch_all(_label,
                     parent.add_option(_names));
}

cli::option
run::make_flag(
    const char*         _label,
    const cli::command& _command,
    const char*         _names
)
{
    cli::command parent = _command;

    return watch(_label,
                 parent.add_flag(_names));
}

cli::command
run::make_subcommand(
    const cli::command& _parent,
    const char*         _name
)
{
    cli::command parent = _parent;

    return watch(_name,
                 parent.add_subcommand(_name));
}

cli::command
run::make_group(
    const cli::command& _parent,
    const char*         _name
)
{
    cli::command parent = _parent;

    return watch(_name,
                 parent.add_option_group(_name));
}

/*
run::label
  The first label a handle on the same record was watched under.
*/
const char*
run::label(
    const cli::option& _option
) const D_NOEXCEPT
{
    // each option watched
    for (std::size_t index = 0u; index < m_options.size(); ++index)
    {
        // the same option
        if (m_options[index].second == _option)
        {
            return m_options[index].first.c_str();
        }
    }

    return "?";
}

const char*
run::label(
    const cli::command& _command
) const D_NOEXCEPT
{
    // each command watched
    for (std::size_t index = 0u; index < m_commands.size(); ++index)
    {
        // the same command
        if (m_commands[index].second == _command)
        {
            return m_commands[index].first.c_str();
        }
    }

    return "?";
}

/*
run::note
  The line and its newline.
*/
void
run::note(
    const std::string& _line
)
{
    m_text += _line;
    m_text += '\n';

    return;
}

/*
run::parse / run::parse_quiet
  The arguments after "prog", in a vector the run keeps until it ends,
since a parse's values borrow main's arguments; the parse through
parse(argc, argv), as main gives it.
*/
void
run::parse(
    const char* const* _args,
    std::size_t        _count
)
{
    (void)parse_quiet(_args,
                      _count);
    record();
    report();

    return;
}

bool
run::parse_quiet(
    const char* const* _args,
    std::size_t        _count
)
{
    set_arguments(_args,
                  _count);
    m_status = m_app.parse(static_cast<int>(_count + 1u),
                           &m_argv[0]);

    return m_status.ok();
}

/**
 * @brief Makes main's arguments: "prog", the arguments given, and the
 *        NULL that ends them, in the vector the run keeps.
 *
 * @param[in] _args   the arguments; may be NULL when `_count` is 0.
 * @param[in] _count  how many.
 */
void
run::set_arguments(
    const char* const* _args,
    std::size_t        _count
)
{
    m_argv.clear();
    m_argv.push_back("prog");

    // each argument after the program's name
    for (std::size_t index = 0u; index < _count; ++index)
    {
        m_argv.push_back(_args[index]);
    }

    m_argv.push_back(D_NULLPTR);

    return;
}

/*
run::parse_line and kin
  Each of the parser's other parse forms, its status kept for record(), then
the ERR line and the report, as parse() writes them.
*/
void
run::parse_line(
    const std::string& _line,
    bool               _program_name_included
)
{
    m_status = m_app.parse(cli::text(_line),
                           _program_name_included);
    record();
    report();

    return;
}

void
run::parse_vector(
    std::vector<std::string>& _args
)
{
    m_status = m_app.parse(_args);
    record();
    report();

    return;
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

void
run::parse_moved(
    std::vector<std::string>& _args
)
{
    m_status = m_app.parse(std::move(_args));
    record();
    report();

    return;
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

void
run::parse_stream(
    std::istream& _input
)
{
    m_status = m_app.parse_from_stream(_input);
    record();
    report();

    return;
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

void
run::parse_wide(
    const wchar_t* const* _args,
    std::size_t           _count
)
{
    m_wargv.clear();
    m_wargv.push_back(L"prog");

    // each argument after the program's name
    for (std::size_t index = 0u; index < _count; ++index)
    {
        m_wargv.push_back(_args[index]);
    }

    m_wargv.push_back(D_NULLPTR);
    m_status = m_app.parse(static_cast<int>(_count + 1u),
                           &m_wargv[0]);
    record();
    report();

    return;
}

void
run::parse_wide_line(
    const std::wstring& _line,
    bool                _program_name_included
)
{
    m_status = m_app.parse(_line,
                           _program_name_included);
    record();
    report();

    return;
}

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

/*
run::parse_main / run::parse_as_main
  The parse that writes the ERR line and the report, then the same
arguments again through D_ARG_PARSE; or that parse alone, for a program
ported, whose callbacks a second parse would run twice.
*/
void
run::parse_main(
    const char* const* _args,
    std::size_t        _count
)
{
    parse(_args,
          _count);
    (void)end_as_main(_count);

    return;
}

int
run::parse_as_main(
    const char* const* _args,
    std::size_t        _count
)
{
    set_arguments(_args,
                  _count);

    return end_as_main(_count);
}

/**
 * @brief Parses the arguments set through D_ARG_PARSE, whose ending --
 *        help, a version, a failure message -- goes to the run's strings,
 *        and writes the code and what the parse added to them.
 *
 * @param[in] _count  how many arguments follow the program's name.
 * @return the code main returns; -1 when the parse goes on.
 */
int
run::end_as_main(
    std::size_t _count
)
{
    const std::size_t out_before = m_out.size();
    const std::size_t err_before = m_err.size();
    const int         code       = main_like(m_app,
                                             static_cast<int>(_count + 1u),
                                             &m_argv[0]);
    std::string       line       = "main ";

    line += number(static_cast<long>(code));
    note(line);

    // what went to the output
    if (m_out.size() > out_before)
    {
        line = "out ";
        append_escaped(line,
                       cli::text(m_out.substr(out_before)),
                       true);
        note(line);
    }

    // what went to the error output
    if (m_err.size() > err_before)
    {
        line = "err ";
        append_escaped(line,
                       cli::text(m_err.substr(err_before)),
                       true);
        note(line);
    }

    return code;
}

/*
run::record
  The status's name, the exit code and the message, read from the parser's
error view, escaped; nothing after a parse that came to status_code::ok.
*/
void
run::record()
{
    // a parse that went on has no line
    if (m_status.ok())
    {
        return;
    }

    const cli::error error = m_app.get_error();
    std::string      line  = "ERR ";

    line += error.get_name();
    line += ' ';
    line += number(static_cast<long>(error.get_exit_code()));
    line += ": ";
    append_escaped(line,
                   cli::text(error.what()),
                   true);
    note(line);

    return;
}

/*
run::report
  Each command watched, then each option, in the order named.
*/
void
run::report()
{
    // each command
    for (std::size_t index = 0u; index < m_commands.size(); ++index)
    {
        report_command(m_commands[index].first,
                       m_commands[index].second);
    }

    // each option
    for (std::size_t index = 0u; index < m_options.size(); ++index)
    {
        report_option(m_options[index].first,
                      m_options[index].second);
    }

    return;
}

/*
run::report_command
  CLI11's count(), remaining(), get_subcommands() by their display names
and parse_order() by the options' names, read through the handle.
*/
void
run::report_command(
    const std::string&  _label,
    const cli::command& _command
)
{
    const std::vector<std::string> missing = _command.remaining();
    const cli::command_list        subs    = _command.get_subcommands();
    const cli::option_list         order   = _command.parse_order();
    std::string                    line    = "cmd ";

    line += _label;
    line += " n=";
    line += number(static_cast<unsigned long>(_command.count()));
    line += " miss=[";

    // the arguments not used
    for (std::size_t index = 0u; index < missing.size(); ++index)
    {
        line += (index > 0u) ? "|" : "";
        append_escaped(line,
                       cli::text(missing[index]),
                       true);
    }

    line += "] subs=[";

    // the subcommands parsed, groups by their display names
    for (std::size_t index = 0u; index < subs.size(); ++index)
    {
        line += (index > 0u) ? "|" : "";
        line += subs[index].get_display_name();
    }

    line += "] order=[";

    // the options in the order met
    for (std::size_t index = 0u; index < order.size(); ++index)
    {
        line += (index > 0u) ? "|" : "";
        line += order[index].get_name();
    }

    line += "]";
    note(line);

    return;
}

/*
run::report_option
  CLI11's count() and results(), read through the handle.
*/
void
run::report_option(
    const std::string& _label,
    const cli::option& _option
)
{
    const cli::text_list results = _option.results();
    std::string          line    = "opt ";

    line += _label;
    line += " n=";
    line += number(static_cast<unsigned long>(_option.count()));
    line += " res=[";

    // each value as given
    for (std::size_t index = 0u; index < results.size(); ++index)
    {
        line += (index > 0u) ? "|" : "";
        append_escaped(line,
                       results[index],
                       true);
    }

    line += "]";
    note(line);

    return;
}

/*
run::refused
  The definition error, when there is one, named and described, then
cleared.
*/
void
run::refused()
{
    const cli::error refusal = m_app.get_definition_error();

    // a step whose definitions all held
    if (refusal.get_status().ok())
    {
        return;
    }

    std::string line = "THROWN ";

    line += refusal.get_name();
    line += ": ";
    append_escaped(line,
                   cli::text(refusal.what()),
                   true);
    note(line);
    m_app.clear_definition_error();

    return;
}

/*
run::exit
  arg_parser::exit with the last parse's status, into streams of its own,
so that the run's strings keep only what the parse itself wrote.
*/
void
run::exit()
{
    std::ostringstream out;
    std::ostringstream err;
    const int          code = m_app.exit(m_status,
                                         out,
                                         err);
    std::string        line = "exit ";

    line += number(static_cast<long>(code));
    note(line);

    // what went to the output
    if (!out.str().empty())
    {
        line = "out ";
        append_escaped(line,
                       cli::text(out.str()),
                       true);
        note(line);
    }

    // what went to the error output
    if (!err.str().empty())
    {
        line = "err ";
        append_escaped(line,
                       cli::text(err.str()),
                       true);
        note(line);
    }

    return;
}

/*
run::text and kin
  The run's strings and its verdict.
*/
const std::string&
run::text() const D_NOEXCEPT
{
    return m_text;
}

const std::string&
run::out() const D_NOEXCEPT
{
    return m_out;
}

const std::string&
run::err() const D_NOEXCEPT
{
    return m_err;
}

bool
run::failed() const D_NOEXCEPT
{
    return m_failed;
}

void
run::fail() D_NOEXCEPT
{
    m_failed = true;

    return;
}

//==============================================================================
// 2.  SECTIONS
//==============================================================================

/*
text_holds
  A mismatch prints both texts, each line between bars.
*/
bool
text_holds(
    const char*        _name,
    const std::string& _written,
    const char*        _expected
)
{
    const std::string expected = (_expected != D_NULLPTR)
                                 ? std::string(_expected)
                                 : std::string();
    const bool        same     = (expected == _written);

    // a mismatch is shown
    if (!same)
    {
        std::printf("    %s differs\n",
                    (_name != D_NULLPTR) ? _name : "text");
        show("expected:",
             expected);
        show("written:",
             _written);
    }

    return same;
}

/*
cases_hold
  Each case on a run of its own, the whole compared once.
*/
bool
cases_hold(
    const char*       _name,
    const case_entry* _cases,
    std::size_t       _count,
    const char*       _expected
)
{
    std::string written;
    bool        ran = true;

    // each case
    for (std::size_t index = 0u; index < _count; ++index)
    {
        ran = case_run(written,
                       _cases[index]) && ran;
    }

    const bool same = text_holds(_name,
                                 written,
                                 _expected);

    return ( (same) &&
             (ran) );
}

/*
steps_hold
  Each step on a run of its own, their transcripts one after another.
*/
bool
steps_hold(
    const char*    _name,
    const step_fn* _steps,
    std::size_t    _count,
    const char*    _expected
)
{
    std::string written;
    bool        ran = true;

    // each step
    for (std::size_t index = 0u; index < _count; ++index)
    {
        run test;

        _steps[index](test);
        test.refused();
        written += test.text();

        // a step that did not run says so
        if (test.failed())
        {
            std::printf("    step %lu failed: a definition or a watch\n",
                        static_cast<unsigned long>(index + 1u));
            ran = false;
        }
    }

    const bool same = text_holds(_name,
                                 written,
                                 _expected);

    return ( (same) &&
             (ran) );
}

//==============================================================================
// 3.  TEXT
//==============================================================================

/*
escape / render
  append_escaped into a string of their own.
*/
std::string
escape(
    const cli::text& _text
)
{
    std::string out;

    append_escaped(out,
                   _text,
                   true);

    return out;
}

std::string
render(
    const cli::text& _text
)
{
    std::string out;

    append_escaped(out,
                   _text,
                   false);

    return out;
}

/*
number
  A stream in the classic locale, which every program starts in.
*/
std::string
number(
    long _value
)
{
    std::ostringstream out;

    out << _value;

    return out.str();
}

std::string
number(
    unsigned long _value
)
{
    std::ostringstream out;

    out << _value;

    return out.str();
}

//==============================================================================
// 4.  VALIDATORS
//==============================================================================

/*
transcribe
  The description, then each value: the run goes through the C call, which
gives the run's status beside the reason -- what tells a value refused from
a validator stopped by an error of its own, where CLI11's throws -- and lets
the value be read under CLI11's rule for leading zeros, as the C suites'
transcripts are; operator() is held to its own contract in the validator
suite.
*/
void
transcribe(
    run&                  _run,
    const cli::validator& _validator,
    const char* const*    _values,
    std::size_t           _count
)
{
    std::string line = "desc <";

    line += render(cli::text(_validator.get_description()));
    line += ">";
    _run.note(line);

    // each value, under CLI11's rule for leading zeros
    for (std::size_t index = 0u; index < _count; ++index)
    {
        ::d_arg_validation validation = ::d_arg_validation();

        d_arg_validation_init(&validation,
                              cli::text(_values[index]));
        validation.integer_rule = D_ARG_INTEGER_C;

        const int        status = d_arg_validator_run(&_validator,
                                                      &validation);
        const bool       passed = ( (status == D_ARG_OK) &&
                                    (d_arg_validation_passed(&validation)) );
        const cli::text  value  = validation.value;
        const cli::text  reason = d_arg_buffer_text(&validation.message);

        line = "[";
        line += render(cli::text(_values[index]));
        line += "] -> ";
        line += (passed)
                ? "pass <"
                : ( (status == D_ARG_OK)
                    ? "fail <"
                    : "status <" );
        line += render((passed) ? value : reason);
        line += ">";
        _run.note(line);
        d_arg_validation_release(&validation);
    }

    return;
}

//==============================================================================
// 5.  FILES
//==============================================================================

/*
scratch_directory::scratch_directory
  Entered only when there are files to make, so that a case without any
leaves the working directory alone; each file made in turn.
*/
scratch_directory::scratch_directory(
    const file_entry* _files
)
    : m_files(),
      m_entered(false)
{
    // a case with files works in a directory of its own
    if (_files != D_NULLPTR)
    {
        m_entered = d_tests_arg_files_enter(&m_files);
        m_files.failed = !m_entered;
    }

    // each file of the list
    for (const file_entry* at = _files;
         ( (m_entered)               &&
           (at != D_NULLPTR)         &&
           (at->name != D_NULLPTR) );
         ++at)
    {
        (void)make(at->name,
                   at->content);
    }
}

/*
scratch_directory::~scratch_directory
  Left, with everything made in it, when it was entered.
*/
scratch_directory::~scratch_directory()
{
    // leave what was entered
    if (m_entered)
    {
        d_tests_arg_files_leave(&m_files);
    }
}

/*
scratch_directory::make / make_folder / ok
  The C calls, which record a failure in `failed`.
*/
bool
scratch_directory::make(
    const char* _name,
    const char* _content
)
{
    return ( (m_entered) &&
             (d_tests_arg_files_file(&m_files,
                                     _name,
                                     _content)) );
}

bool
scratch_directory::make_folder(
    const char* _name
)
{
    return ( (m_entered) &&
             (d_tests_arg_files_folder(&m_files,
                                       _name)) );
}

bool
scratch_directory::ok() const D_NOEXCEPT
{
    return !m_files.failed;
}

/*
set_variable
  POSIX's setenv and unsetenv, or the Microsoft runtime's _putenv_s, where
an empty value removes the variable.
*/
bool
set_variable(
    const char* _name,
    const char* _value
)
{
#if (D_ENV_ARG_PARSER_STAT == D_ENV_ARG_PARSER_STAT_WINDOWS)
    return (_putenv_s(_name,
                      (_value != D_NULLPTR) ? _value : "") == 0);
#else
    return (_value != D_NULLPTR)
           ? (setenv(_name,
                     _value,
                     1) == 0)
           : (unsetenv(_name) == 0);
#endif
}

}  // namespace testing
}  // namespace cli
}  // namespace parse
}  // namespace djinterp
