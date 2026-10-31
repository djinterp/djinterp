/*******************************************************************************
* vparse [cli]                                                        common.hpp
*
* What the subcommands share: reading their options, reading what they are
* given, the request a run is made from, and ending, with an output or a
* failure, as the kit ends a command.
*   The handlers are in commands.cpp and measures.cpp; these are the steps
* more than one of them takes, so that an option is read, a file is named, a
* failure is worded and an output is written the same way, whichever
* subcommand does it.
*   LEVELS. The command line's floor is the model's, C++17, so this file's
* is: below it the file is empty rather than an error.
*
*
* path:      /src/djinterp/tools/vparse/cli/common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.17
*                                                            revised: 2026.10.27
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  WHAT A SUBCOMMAND READS
    -----------------------
    1.  Names
         1.  STANDARD_INPUT
    2.  Sources
         1.  source
         2.  sources
2.  OPTIONS
    -------
    1.  Values
         1.  value_of
         2.  values_of
         3.  given
    2.  Settings
         1.  step_limit_of
         2.  wants_json
         3.  frontend_of
         4.  memo_of
         5.  number_of
3.  ENDINGS
    -------
    1.  quiet
    2.  usage_error
    3.  fail
    4.  emit
    5.  refuse
4.  READING
    -------
    1.  Files and text
         1.  read_source
         2.  standard_inputs
    2.  A subcommand's sources
         1.  read_grammar
         2.  read_inputs
         3.  read_all
5.  RUNS
    ----
    1.  request_for
    2.  accepted
    3.  status_of
6.  WORDS
    -----
    1.  joined
*/

#ifndef VPARSE_CLI_COMMON_HPP
#define VPARSE_CLI_COMMON_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error, as
// the model it drives is.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string
#include <utility>  // std::move
#include <vector>   // std::vector
// djinterp
#include "djinterp/c/modules/parse/cli/arg_parser/arg_parser.h"  // dj_arg_option,
                                                         // dj_arg_command
// vparse
#include "./cli.hpp"       // context, exit_status
#include "./commands.hpp"  // the option records
#include "../../../../../inc/djinterp/tools/vparse/run.hpp"  // run_request,
                                                             // run_record


namespace vparse
{
namespace cli
{


//==============================================================================
// 1.  WHAT A SUBCOMMAND READS
//==============================================================================


// 1.1    Names
//------------------------------------------------------------------------------
// 1.1.1
// STANDARD_INPUT
//   constant: the name that stands for the standard input, as a file name.
const char* const STANDARD_INPUT = "-";

// 1.2    Sources
//------------------------------------------------------------------------------
// 1.2.1
// source
//   struct: one text a run reads: the name a reader knows it by, the
// argument that named it -- a file name, `-`, or the text itself -- and its
// bytes.
struct source
{
    std::string name;
    std::string argument;
    std::string bytes;
};

// 1.2.2
// sources
//   struct: what a subcommand that runs reads before it runs: its grammar,
// and its inputs, in order.
struct sources
{
    source              grammar;
    std::vector<source> inputs;
};


//==============================================================================
// 2.  OPTIONS
//==============================================================================


// 2.1    Values
//------------------------------------------------------------------------------
// 2.1.1
// value_of
/**
 * @brief Reads an option's value as text: the value given, else the
 *        option's default.
 *
 * @param[in] _option  the option; NULL reads as an option not given and
 *                     without a default.
 * @return the value; empty when there is neither.
 */
std::string value_of(
    struct dj_arg_option* _option
);

// 2.1.2
// values_of
/**
 * @brief Reads every value an option was given, in order.
 *
 * @param[in] _option  the option; may be NULL.
 * @return the values; the option's default alone when none was given and it
 *         has one, and none otherwise.
 */
std::vector<std::string> values_of(
    struct dj_arg_option* _option
);

// 2.1.3
// given
/**
 * @brief Determines whether an option was given.
 *
 * @param[in] _option  the option; NULL reads as not given.
 * @return whether the parse gave it a value.
 */
bool given(
    const struct dj_arg_option* _option
);

// 2.2    Settings
//------------------------------------------------------------------------------
// 2.2.1
// step_limit_of
/**
 * @brief Reads the step limit a subcommand runs under.
 *
 * @param[in] _options  the subcommand's run options.
 * @return --step-limit's value, which its validator holds to 1 or more, or
 *         a fresh request's limit when it could not be read.
 */
long step_limit_of(
    const run_options& _options
);

// 2.2.2
// wants_json
/**
 * @brief Determines whether a subcommand prints JSON.
 *
 * @param[in] _output  the subcommand's output options.
 * @return whether --format is json.
 */
bool wants_json(
    const output_options& _output
);

// 2.2.3
// frontend_of
/**
 * @brief Works out which frontend reads a subcommand's grammar.
 *
 * @param[in] _source  the subcommand's source options.
 * @return "ebnf" for --ebnf; --frontend's value when it was given; and,
 *         when neither was, "ebnf" for a grammar whose name, as the command
 *         line gave it, ends in ".ebnf", in any case, and "notation" for
 *         any other.
 */
std::string frontend_of(
    const source_options& _source
);

// 2.2.4
// memo_of
/**
 * @brief Reads which rules a subcommand's program calls through the memo.
 *
 * @param[in] _memoize  the subcommand's --memoize option, which collects
 *                      its values; NULL for one that has none.
 * @return the setting the values given make, in order -- a name after
 *         names joined to them with a comma, anything else replacing what
 *         came before it -- or the default; "none" for an option not made,
 *         or a last value of nothing.
 */
std::string memo_of(
    struct dj_arg_option* _memoize
);

// 2.2.5
// number_of
/**
 * @brief Reads a whole number an option was given, or its default.
 *
 * @param[in] _option    the option; NULL for one not made.
 * @param[in] _fallback  the number for an option not made, or not read.
 * @return the value, which the option's validator has held to its range,
 *         or `_fallback`.
 */
long number_of(
    struct dj_arg_option* _option,
    long                  _fallback
);


//==============================================================================
// 3.  ENDINGS
//==============================================================================
//   A failure is said as the kit says one -- `vparse: error: MESSAGE`, on the
// error stream, through the context's program -- and is trouble, REFUSED;
// an output is written whole, once nothing is left to fail but the writing.
// A message starts in lower case and has no full stop (GRAMMAR.md, section
// 8).


// 3.1    quiet
//------------------------------------------------------------------------------
// 3.1.1
// quiet
/**
 * @brief Determines whether `-q` was given, anywhere on the command line:
 *        the kit's, which keeps only errors on the error stream, and leaves
 *        out of what a subcommand prints what its exit status says.
 *
 * @param[in] _context  the run, whose program read `-q`.
 * @return whether the program's verbosity is below 0.
 */
bool quiet(
    const context& _context
);

// 3.2    usage_error
//------------------------------------------------------------------------------
// 3.2.1
// usage_error
/**
 * @brief Ends a subcommand with a usage error its handler found, said as
 *        the kit says the parser's: `vparse: error: COMMAND: MESSAGE`, and
 *        a pointer to the subcommand's help.
 *
 * @param[in] _command  the subcommand.
 * @param[in] _message  what is wrong with the command line.
 * @param[in] _context  the run, whose program says it.
 * @return exit_status::REFUSED, the kit's trouble.
 */
int usage_error(
    const struct dj_arg_command* _command,
    const std::string&           _message,
    const context&               _context
);

// 3.3    fail
//------------------------------------------------------------------------------
// 3.3.1
// fail
/**
 * @brief Ends a subcommand with trouble: `vparse: error: MESSAGE`.
 *
 * @param[in] _context  the run, whose program says it.
 * @param[in] _message  what went wrong: `cannot read 'NAME': REASON`, say.
 * @return exit_status::REFUSED, the kit's trouble.
 */
int fail(
    const context&     _context,
    const std::string& _message
);

// 3.4    emit
//------------------------------------------------------------------------------
// 3.4.1
// emit
/**
 * @brief Writes a subcommand's output, whole, and ends the subcommand.
 *
 * @param[in] _context  where the output goes.
 * @param[in] _text     the output; nothing is written when it is empty.
 * @param[in] _status   the exit status the subcommand came to.
 * @return `_status`; REFUSED when the output could not be written, said as
 *         the kit says it: `cannot write standard output: REASON`.
 */
int emit(
    const context&     _context,
    const std::string& _text,
    int                _status
);

// 3.5    refuse
//------------------------------------------------------------------------------
// 3.5.1
// refuse
/**
 * @brief Says why a grammar was refused, as an error, and ends the
 *        subcommand with REFUSED: `NAME:LINE:COLUMN: error: SENTENCE`
 *        where its reader stopped at a place, as a compiler puts the place
 *        first (GRAMMAR.md, section 8), and `vparse: error: NAME: SENTENCE`
 *        where there is none -- the sentence the run's headline, the place
 *        taken out of it, as a diagnostic is written.
 *
 * @param[in] _context  the run of the command line, whose program says it.
 * @param[in] _name     the grammar's name, as the command line gave it.
 * @param[in] _run      a run of the grammar that refused it.
 * @return exit_status::REFUSED.
 */
int refuse(
    const context&               _context,
    const std::string&           _name,
    const workbench::run_record& _run
);


//==============================================================================
// 4.  READING
//==============================================================================


// 4.1    Files and text
//------------------------------------------------------------------------------
// 4.1.1
// read_source
/**
 * @brief Reads what a command line names: a file, `-` for the standard
 *        input, or, given as text, the argument itself.
 *
 * @param[in]  _name     the argument.
 * @param[in]  _as_text  whether the argument is the text itself.
 * @param[in]  _context  the standard input `-` reads.
 * @param[out] _read     receives the name to show and the bytes.
 * @param[out] _error    receives why it could not be read, on failure, as
 *                       the kit says it: "cannot read 'NAME': REASON", or
 *                       "cannot read standard input: REASON" for `-`.
 * @return whether there is something to run.
 */
bool read_source(
    const std::string& _name,
    bool               _as_text,
    const context&     _context,
    source&            _read,
    std::string&       _error
);

// 4.1.2
// standard_inputs
/**
 * @brief Counts the arguments that name the standard input.
 *
 * @param[in] _names    the arguments that name files.
 * @param[in] _as_text  whether they are text instead, and name nothing.
 * @return how many of them are `-`.
 */
std::size_t standard_inputs(
    const std::vector<std::string>& _names,
    bool                            _as_text
);

// 4.2    A subcommand's sources
//------------------------------------------------------------------------------
// 4.2.1
// read_grammar
/**
 * @brief Reads a subcommand's grammar.
 *
 * @param[in]  _source   its source options.
 * @param[in]  _context  the standard input `-` reads.
 * @param[out] _grammar  receives the grammar's name and bytes.
 * @return an empty string when the grammar was read; why it was not
 *         otherwise, as the failure is written.
 */
std::string read_grammar(
    const source_options& _source,
    const context&        _context,
    source&               _grammar
);

// 4.2.2
// read_inputs
/**
 * @brief Reads every input a subcommand names.
 *
 * @param[in]  _names    the arguments naming the inputs.
 * @param[in]  _as_text  whether each argument is the input itself.
 * @param[in]  _context  the standard input `-` reads.
 * @param[out] _inputs   receives each input's name and bytes, in order.
 * @return an empty string when every input was read; why the first that
 *         could not be was not otherwise, as the failure is written.
 */
std::string read_inputs(
    const std::vector<std::string>& _names,
    bool                            _as_text,
    const context&                  _context,
    std::vector<source>&            _inputs
);

// 4.2.3
// read_all
/**
 * @brief Reads everything a subcommand that runs is given: its grammar, and
 *        then its inputs, the standard input at most once.
 *
 * @tparam Command  the subcommand's record: match_command or run_command.
 * @param[in]  _command  the subcommand's record.
 * @param[in]  _context  the standard input `-` reads, and where a failure is
 *                       written.
 * @param[out] _read     receives the grammar and the inputs.
 * @return exit_status::PASSED when everything was read; otherwise REFUSED,
 *         the failure said: a usage error for `-` named twice, and the file
 *         that could not be read.
 *
 * Everything is read before anything runs, so that a misnamed input fails
 * the command line before any output is made; and `-` named twice is refused
 * before anything is read, since a stream read to its end has nothing left
 * for a second reader.
 */
template<typename Command>
int read_all(
    const Command& _command,
    const context& _context,
    sources&       _read
)
{
    const bool                     as_text  = given(_command.text);
    const std::vector<std::string> names    = values_of(_command.inputs);
    const bool                     piped    =
        (value_of(_command.source.grammar) == STANDARD_INPUT);
    const std::size_t              standard =
        standard_inputs(names,
                        as_text) + (piped ? 1u : 0u);

    // '-' named for more than one of them
    if (standard > 1u)
    {
        return usage_error(_command.command,
                           "standard input can be read once: - may stand "
                           "for the grammar or for one input",
                           _context);
    }

    std::string unread = read_grammar(_command.source,
                                      _context,
                                      _read.grammar);

    // the inputs, once the grammar is read
    if (unread.empty())
    {
        unread = read_inputs(names,
                             as_text,
                             _context,
                             _read.inputs);
    }

    // a file that cannot be read
    if (!unread.empty())
    {
        return fail(_context,
                    unread);
    }

    return exit_status::PASSED;
}


//==============================================================================
// 5.  RUNS
//==============================================================================


// 5.1    request_for
//------------------------------------------------------------------------------
// 5.1.1
// request_for
/**
 * @brief Makes the request a subcommand runs.
 *
 * @param[in] _frontend  "notation" or "ebnf".
 * @param[in] _grammar   the grammar.
 * @param[in] _input     the input, and its name; an empty name for none.
 * @param[in] _trace     whether to record the trace.
 * @param[in] _limit     the step limit.
 * @return the request, its origin naming what the command line gave it.
 */
workbench::run_request request_for(
    const std::string& _frontend,
    const source&      _grammar,
    const source&      _input,
    bool               _trace,
    long               _limit
);

// 5.2    accepted
//------------------------------------------------------------------------------
// 5.2.1
// accepted
/**
 * @brief Determines whether a run's grammar was read and built.
 *
 * @param[in] _run  the run.
 * @return whether the session accepted the grammar and the program exists.
 */
bool accepted(
    const workbench::run_record& _run
);

// 5.3    status_of
//------------------------------------------------------------------------------
// 5.3.1
// status_of
/**
 * @brief Gives a run's exit status.
 *
 * @param[in] _run  the run.
 * @return REFUSED for a grammar not read or not built, PASSED for an input
 *         that matched, NO_MATCH otherwise.
 */
int status_of(
    const workbench::run_record& _run
);


//==============================================================================
// 6.  WORDS
//==============================================================================


// 6.1    joined
//------------------------------------------------------------------------------
// 6.1.1
// joined
/**
 * @brief Joins rule names for a sentence.
 *
 * @param[in] _names  the names.
 * @return the first NAMES_MOST of them, separated by commas, and how many
 *         more there are, when there are more.
 */
std::string joined(
    const std::vector<std::string>& _names
);

}  // namespace cli
}  // namespace vparse

#endif  // floor, for now

#endif  // VPARSE_CLI_COMMON_HPP
