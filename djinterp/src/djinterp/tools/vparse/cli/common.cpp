/*******************************************************************************
* vparse [cli]                                                        common.cpp
*
* Definitions for common.hpp: the steps the subcommands share.
*   A file is read whole, in binary, so a grammar or an input is run byte for
* byte; `-` is the standard input, read to its end, once. A failure is said
* through the kit's program the run is, so its shape, its colour and the
* exit status it ends with are the kit's whoever failed.
*
* path:      /src/djinterp/tools/vparse/cli/common.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.17
*                                                            revised: 2026.10.27
*******************************************************************************/
#include "./common.hpp"  // corresponding header
#if D_ENV_LANG_IS_CPP17_OR_HIGHER  // the floor the header has

// std
#include <algorithm>  // std::max, std::min
#include <cctype>     // std::tolower
#include <cerrno>     // errno
#include <cstddef>    // std::size_t
#include <cstdio>     // std::FILE, std::fread, std::ferror
#include <cstring>    // std::strerror
#include <string>     // std::string, std::to_string
#include <utility>    // std::move
#include <vector>     // std::vector
// djinterp
#include "djinterp/c/modules/parse/cli/arg_parser/arg_parser.h"  // option values,
                                                         // sinks
#include "djinterp/tools/cli/c/cli_program.h"  // d_cli_report,
                                               // d_cli_usage_error,
                                               // d_cli_file_error
// vparse
#include "./report.hpp"  // clipped
#include "../../../../../inc/djinterp/tools/vparse/store.hpp"  // files by
                                                               // name


namespace vparse
{
namespace cli
{

namespace
{

namespace wb = ::vparse::workbench;

// TEXT_NAME_MOST
//   constant: the most bytes of an input given as text that its name shows.
const std::size_t TEXT_NAME_MOST = 40u;

// JSON_FORMAT
//   constant: --format's value for JSON; any other is text.
const char* const JSON_FORMAT = "json";

// NAMES_MOST
//   constant: the most rule names a sentence names; JSON names them all.
const std::size_t NAMES_MOST = 10u;

/**
 * @brief Reads a stream to its end.
 *
 * @param[in]  _stream  the stream; NULL when there is none.
 * @param[out] _bytes   receives what was read; unchanged on failure.
 * @param[out] _error   receives why it could not be read, on failure: the
 *                      system's words where it gave a reason.
 * @return whether the stream was read to its end.
 */
bool read_stream(
    std::FILE*   _stream,
    std::string& _bytes,
    std::string& _error
)
{
    // a context with no standard input
    if (_stream == nullptr)
    {
        _error = "the run was given none";

        return false;
    }

    std::string bytes;
    char        buffer[4096] = {};
    std::size_t got          = sizeof(buffer);
    int         reason       = 0;

    // whole blocks until a short one, which is the end or an error
    while (got == sizeof(buffer))
    {
        errno  = 0;
        got    = std::fread(buffer,
                            1u,
                            sizeof(buffer),
                            _stream);
        reason = errno;
        bytes.append(buffer,
                     got);
    }

    // a short block from an error, not from the end
    if (std::ferror(_stream) != 0)
    {
        _error = (reason != 0) ? std::strerror(reason)
                               : "the stream reported an error";

        return false;
    }

    _bytes = std::move(bytes);

    return true;
}

/**
 * @brief Names a file for a reader: as given, but the standard input by
 *        what it is.
 *
 * @param[in] _name  the name as given.
 * @return the name to show.
 */
std::string shown_name(
    const std::string& _name
)
{
    return (_name == STANDARD_INPUT) ? std::string("standard input")
                                     : _name;
}

/**
 * @brief Writes a sentence as a diagnostic is written: its first letter in
 *        lower case, and without its full stop.
 *
 * @param[in] _sentence  the sentence, as the window shows it: "The grammar
 *                       could not be read: ...".
 * @return the diagnostic's words: "the grammar could not be read: ...".
 */
std::string as_diagnostic(
    const std::string& _sentence
)
{
    std::string words = _sentence;

    // the full stop, where there is one
    if ( (!words.empty()) &&
         (words.back() == '.') )
    {
        words.pop_back();
    }

    // a capital that only began the sentence
    if (!words.empty())
    {
        words[0] = static_cast<char>(
            std::tolower(static_cast<unsigned char>(words[0])));
    }

    return words;
}

}  // anonymous namespace


/*
value_of
  The parser converts, so a value is read as the option's own type
reads it, and a default shown in help is the value an option not given has.
*/
std::string value_of(
    struct dj_arg_option* _option
)
{
    // nothing to read from an option that was not made
    if (_option == nullptr)
    {
        return std::string();
    }

    const char* text   = nullptr;
    const int   status =
        d_arg_option_as(_option,
                        d_arg_type_builtin(D_ARG_BUILTIN_STRING),
                        &text);

    return ( (status == D_ARG_OK) &&
             (text != nullptr) ) ? std::string(text)
                                 : std::string();
}

/*
values_of
  The values as the parser kept them, a value an argument; a default
stands in only where nothing was given, as it does for value_of().
*/
std::vector<std::string> values_of(
    struct dj_arg_option* _option
)
{
    std::vector<std::string> out;
    std::size_t              count = 0u;

    const struct dj_pack_text* const values =
        d_arg_option_values(_option,
                            &count);

    // the values given, in order
    for (std::size_t index = 0u; index < count; index++)
    {
        out.emplace_back(values[index].data,
                         values[index].length);
    }

    const std::string fallback = d_arg_option_default_text(_option);

    // nothing given: the default, where there is one
    if ( (out.empty()) &&
         (!fallback.empty()) )
    {
        out.push_back(fallback);
    }

    return out;
}

/*
given
  Whether the parse saw the option, which a default does not make so.
*/
bool given(
    const struct dj_arg_option* _option
)
{
    return d_arg_option_is_given(_option);
}

/*
step_limit_of
  The validator has held the value to its range before any handler
runs, so this converts and does not judge.
*/
long step_limit_of(
    const run_options& _options
)
{
    long limit = wb::run_request().step_limit;

    // the validator has seen the value already; this only converts it
    if (_options.step_limit != nullptr)
    {
        (void)d_arg_option_as(_options.step_limit,
                              d_arg_type_builtin(D_ARG_BUILTIN_LONG),
                              &limit);
    }

    return std::max(1L,
                    limit);
}

/*
wants_json
  Any format but JSON is text: the validator allows those two alone.
*/
bool wants_json(
    const output_options& _output
)
{
    return (value_of(_output.format) == JSON_FORMAT);
}

/*
frontend_of
  The order the help states: --ebnf, then --frontend, then the name.
*/
std::string frontend_of(
    const source_options& _source
)
{
    // --ebnf is --frontend ebnf; the parser refuses the two together
    if (given(_source.ebnf))
    {
        return "ebnf";
    }

    // said outright
    if (given(_source.frontend))
    {
        return value_of(_source.frontend);
    }

    return wb::frontend_for_file(value_of(_source.grammar));
}

/*
memo_of
  The values are folded in the order given: a name after names joins them,
and anything else -- a word, the first name, or nothing -- starts the
setting again, so that a word appended to a command line overrides what was
before it (GRAMMAR.md, R8). The model reads the setting -- "none", "all",
"auto", or rule names separated by commas -- and refuses a name no rule
has; a value of nothing is no memo, as it reads one.
*/
std::string memo_of(
    struct dj_arg_option* _memoize
)
{
    std::string setting;
    bool        named = false;

    // each value, in order
    for (const std::string& value : values_of(_memoize))
    {
        const bool word = ( (value == "none") ||
                            (value == "all")  ||
                            (value == "auto") ||
                            (value.empty()) );

        setting = ( (named) &&
                    (!word) ) ? setting + "," + value
                              : value;
        named   = !word;
    }

    return setting.empty() ? std::string("none")
                           : setting;
}

/*
number_of
  The parser converts, as it does for step_limit_of; every number option has
a default, so an option not given reads as the default help shows.
*/
long number_of(
    struct dj_arg_option* _option,
    long                  _fallback
)
{
    long number = _fallback;

    // the validator has seen the value already; this only converts it
    if (_option != nullptr)
    {
        (void)d_arg_option_as(_option,
                              d_arg_type_builtin(D_ARG_BUILTIN_LONG),
                              &number);
    }

    return number;
}

/*
quiet
  The kit reads -q, wherever it was given, as a verbosity below 0.
*/
bool quiet(
    const context& _context
)
{
    return (d_cli_program_verbosity(_context.program) < 0);
}

/*
usage_error
  The kit's own report, so that the line and the pointer to help are the
ones the parser's failures take.
*/
int usage_error(
    const struct dj_arg_command* _command,
    const std::string&           _message,
    const context&               _context
)
{
    (void)d_cli_usage_error(_context.program,
                            _command,
                            "%s",
                            _message.c_str());

    return exit_status::REFUSED;
}

/*
fail
  An error, said; the status is the kit's trouble.
*/
int fail(
    const context&     _context,
    const std::string& _message
)
{
    d_cli_report(_context.program,
                 static_cast<int>(D_CLI_SEVERITY_ERROR),
                 "%s",
                 _message.c_str());

    return exit_status::REFUSED;
}

/*
emit
  One write of the whole output: a failure part-way through is the
writing's, said with the system's reason where it set one, as the kit says
a write to standard output it could not make.
*/
int emit(
    const context&     _context,
    const std::string& _text,
    int                _status
)
{
    // nothing to write: quiet, or a run with nothing to say
    if (_text.empty())
    {
        return _status;
    }

    errno = 0;

    // a closed pipe or a full disk is a failure of its own
    if (d_arg_sink_write(_context.out,
                         _text.data(),
                         _text.size()) != D_ARG_OK)
    {
        const int reason = errno;

        // `-`, which the kit names standard output for a write
        return (reason != 0) ? d_cli_file_error(_context.program,
                                                "write",
                                                "-",
                                                reason)
                             : fail(_context,
                                    "cannot write standard output");
    }

    return _status;
}

/*
refuse
  The run's headline, which says which stage refused the grammar and why,
as the window says it, worded as a diagnostic; where the reader stopped at a
place, the place comes first, and is not said again.
*/
int refuse(
    const context&        _context,
    const std::string&    _name,
    const wb::run_record& _run
)
{
    const wb::refusal said = wb::refusal_of(_run);

    // a place in the grammar: first, as an editor reads it
    if (said.line > 0)
    {
        d_cli_report_at(_context.program,
                        static_cast<int>(D_CLI_SEVERITY_ERROR),
                        _name.c_str(),
                        static_cast<long>(said.line),
                        static_cast<long>(said.column),
                        "%s",
                        as_diagnostic(said.sentence).c_str());

        return exit_status::REFUSED;
    }

    return fail(_context,
                _name + ": " + as_diagnostic(said.sentence));
}

/*
read_source
  Text is its own name, clipped; a file is read whole, in binary, and `-`
is the standard input read to its end.
*/
bool read_source(
    const std::string& _name,
    bool               _as_text,
    const context&     _context,
    source&            _read,
    std::string&       _error
)
{
    _read.argument = _name;

    // the argument is the text
    if (_as_text)
    {
        _read.name  = "'" + clipped(wb::escape(_name),
                                    TEXT_NAME_MOST) + "'";
        _read.bytes = _name;

        return true;
    }

    std::string reason;
    const bool  read =
        (_name == STANDARD_INPUT)
            ? read_stream(_context.in,
                          _read.bytes,
                          reason)
            : wb::read_whole_file(wb::path_from_utf8(_name),
                                  _read.bytes,
                                  &reason);

    // said as the kit says a file it cannot read
    if (!read)
    {
        _error = (_name == STANDARD_INPUT)
                     ? "cannot read standard input: " + reason
                     : "cannot read '" + _name + "': " + reason;

        return false;
    }

    _read.name = shown_name(_name);

    return true;
}

/*
standard_inputs
  Counted before anything is read, so that `-` named twice is refused
without reading the stream once.
*/
std::size_t standard_inputs(
    const std::vector<std::string>& _names,
    bool                            _as_text
)
{
    std::size_t count = 0u;

    // text names no file, the standard input included
    if (_as_text)
    {
        return count;
    }

    // each `-` among the names
    for (const std::string& name : _names)
    {
        count += (name == STANDARD_INPUT) ? 1u : 0u;
    }

    return count;
}

/*
read_grammar
  The grammar is a file the command line names, so a failure to read it
is that file's.
*/
std::string read_grammar(
    const source_options& _source,
    const context&        _context,
    source&               _grammar
)
{
    std::string error;

    // a grammar that cannot be read is a file the command line names
    if (!read_source(value_of(_source.grammar),
                     false,
                     _context,
                     _grammar,
                     error))
    {
        return error;
    }

    return std::string();
}

/*
read_inputs
  In order, stopping at the first that cannot be read: a later input is
not read for a run that will not happen.
*/
std::string read_inputs(
    const std::vector<std::string>& _names,
    bool                            _as_text,
    const context&                  _context,
    std::vector<source>&            _inputs
)
{
    // every input, in order
    for (const std::string& name : _names)
    {
        source      input;
        std::string error;

        // the first input that cannot be read ends the reading
        if (!read_source(name,
                         _as_text,
                         _context,
                         input,
                         error))
        {
            return error;
        }

        _inputs.push_back(std::move(input));
    }

    return std::string();
}

/*
request_for
  The origin names what the command line gave, as a saved run's says
which folder it came from.
*/
wb::run_request request_for(
    const std::string& _frontend,
    const source&      _grammar,
    const source&      _input,
    bool               _trace,
    long               _limit
)
{
    wb::run_request request;

    request.frontend   = _frontend;
    request.grammar    = _grammar.bytes;
    request.input      = _input.bytes;
    request.trace      = _trace;
    request.step_limit = _limit;
    request.origin     = "command line: " + _grammar.name +
                         (_input.name.empty() ? std::string()
                                              : " over " + _input.name);

    return request;
}

/*
accepted
  Read, judged by the session, and built: the three a run needs before it
runs.
*/
bool accepted(
    const wb::run_record& _run
)
{
    return ( (_run.read_ok) &&
             (_run.built) );
}

/*
status_of
  The grammar decides first: a refused grammar matches nothing.
*/
int status_of(
    const wb::run_record& _run
)
{
    // the grammar decides first
    if (!accepted(_run))
    {
        return exit_status::REFUSED;
    }

    return _run.recognition.matched ? exit_status::PASSED
                                    : exit_status::NO_MATCH;
}

/*
joined
  A long grammar's names are counted past NAMES_MOST, so that a sentence
stays a sentence.
*/
std::string joined(
    const std::vector<std::string>& _names
)
{
    const std::size_t shown = std::min(_names.size(),
                                       NAMES_MOST);
    std::string       out;

    // the names shown, separated by commas
    for (std::size_t index = 0u; index < shown; index++)
    {
        out += (index == 0u) ? "" : ", ";
        out += _names[index];
    }

    // a long grammar's names are counted, not listed
    if (shown < _names.size())
    {
        out += " and " + std::to_string(_names.size() - shown) + " more";
    }

    return out;
}

}  // namespace cli
}  // namespace vparse

#endif  // floor, for now
