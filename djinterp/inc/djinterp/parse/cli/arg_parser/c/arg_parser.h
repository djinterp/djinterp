/*******************************************************************************
* djinterp [parse]                                                  arg_parser.h
*
* The argument parser: the root of a command tree, and the parse itself.
*   A `struct d_arg_parser` is what a program makes first: CLI11's top-level
* App -- the root command every option and subcommand hangs from -- with what
* belongs to a parse rather than to any one command: the error reports, the
* formatter and configuration format every command without its own uses,
* the sinks help and failures are written to, and the storage for the
* arguments a parse makes. Its first member is its root command, so
* `&parser->command` is the root wherever a command is wanted, and the C++
* face derives from it without adding to it.
*   A parse returns a d_arg_status. D_ARG_OK means the program goes on; any
* other status goes to d_arg_parser_exit, which writes what it calls for --
* help, a version, a completion script, a failure message -- and returns the
* code to end the process with. D_ARG_PARSE does both for main, as
* CLI11_PARSE does.
*   This is also the C face's umbrella header: including it includes every
* header of the parser.
*
*
* path:      /inc/djinterp/parse/cli/arg_parser/c/arg_parser.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.12
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VOCABULARY
    ----------
    1.  Parser flags
         1.  D_ARG_PARSER_RESPONSE_FILES
         2.  D_ARG_PARSER_SUGGESTIONS
    2.  Argument flags
         1.  D_ARG_ARGS_REVERSED
         2.  D_ARG_ARGS_COPY
2.  RECORDS
    -------
    1.  The parser
         1.  d_arg_parser
3.  LIFETIME
    --------
    1.  Making and releasing
         1.  d_arg_parser_init
         2.  d_arg_parser_release
    2.  Parse state
         1.  d_arg_parser_clear
4.  PARSING
    -------
    1.  Arguments
         1.  d_arg_parser_parse
         2.  d_arg_parser_parse_args
         3.  d_arg_parser_parse_line
    2.  Wide arguments
         1.  d_arg_parser_parse_wide
         2.  d_arg_parser_parse_line_wide
         3.  d_arg_parser_ensure_utf8
    3.  Configuration text
         1.  d_arg_parser_parse_config
5.  ENDING
    ------
    1.  Exit
         1.  d_arg_parser_exit
         2.  d_arg_parser_exit_to
         3.  d_arg_parser_parse_or_exit
         4.  D_ARG_PARSE
    2.  Failure messages
         1.  d_arg_failure_simple
         2.  d_arg_failure_help
    3.  Failing from a callback
         1.  d_arg_command_fail / d_arg_option_fail
         2.  d_arg_command_fail_runtime
6.  SETTINGS
    --------
    1.  Output
         1.  d_arg_parser_set_output
    2.  Extensions
         1.  d_arg_parser_set_response_files
         2.  d_arg_parser_set_suggestions
7.  ERRORS
    ------
    1.  The last parse
         1.  d_arg_parser_error
    2.  Definitions
         1.  d_arg_parser_definition_error
         2.  d_arg_parser_clear_definition_error
*/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_PARSER_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_PARSER_H 1

// std
#include <stddef.h>                          // size_t, wchar_t
// djinterp
#include "../../../../c/djinterp.h"          // framework root
#include "../../../../c/util/sink_common.h"  // d_pack_text, d_pack_sink
#include "./arg_common.h"                    // statuses, lists, errors
#include "./arg_type.h"                      // umbrella: types
#include "./arg_validator.h"                 // umbrella: validators
#include "./arg_validator_extra.h"           // umbrella: more validators
#include "./arg_validator_path.h"            // umbrella: path validators
#include "./arg_option.h"                    // umbrella: options
#include "./arg_command.h"                   // d_arg_command
#include "./arg_formatter.h"                 // d_arg_formatter
#include "./arg_config.h"                    // d_arg_config_format
#include "./arg_argv.h"                      // umbrella: splitting lines
#include "./arg_completion.h"                // umbrella: completion
// re_std
#include "../../../../../re_std/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// 1.  VOCABULARY
//==============================================================================


// 1.1    Parser flags
//------------------------------------------------------------------------------
//   constant: the switches of d_arg_parser's `flags`, set through the calls
// of section 6.

// 1.1.1
// D_ARG_PARSER_RESPONSE_FILES
//   constant: `@file` arguments stand for the arguments their files hold; see
// d_arg_parser_set_response_files.
#define D_ARG_PARSER_RESPONSE_FILES 0x00000001u

// 1.1.2
// D_ARG_PARSER_SUGGESTIONS
//   constant: an unexpected argument is reported with the name it most
// likely meant; see d_arg_parser_set_suggestions. Set by default, and read
// only with D_INTERNAL_ARG_PARSER_SUGGESTIONS.
#define D_ARG_PARSER_SUGGESTIONS    0x00000002u

// 1.2    Argument flags
//------------------------------------------------------------------------------
//   constant: how d_arg_parser_parse_args receives its arguments.

// 1.2.1
// D_ARG_ARGS_REVERSED
//   constant: the arguments are given last first, as CLI11's vector holds
// them.
#define D_ARG_ARGS_REVERSED         0x00000001u

// 1.2.2
// D_ARG_ARGS_COPY
//   constant: the parser copies the arguments, so they need not outlive the
// values the parse gives, nor end in a NUL.
#define D_ARG_ARGS_COPY             0x00000002u


//==============================================================================
// 2.  RECORDS
//==============================================================================


// 2.1    The parser
//------------------------------------------------------------------------------
// 2.1.1
// d_arg_parser
//   struct: a parser. `command` is the root, first so that a parser's address
// is its root's. `error` is what the last parse ended with, and `definition`
// the first definition that failed since the parser was made or the error
// cleared. `formatter` and `config_format` serve every command that has none
// of its own; `out` receives help, versions, completion scripts and
// deprecation warnings, and `err` failure messages. The rest is storage:
// `pending` holds the arguments still to parse, the next one last; `pool`
// the strings the last parse made -- a split line's words, a response
// file's, converted wide arguments; `utf8_argv` the arguments
// d_arg_parser_ensure_utf8 recovered, NULL-terminated. `completion` is the
// option that asks for a completion script, if any; `response_quoting` a
// d_arg_quoting value; `flags` the D_ARG_PARSER_* switches.
//   Public-payload layout: read any field; change one only through the calls
// below.
struct d_arg_parser
{
    struct d_arg_command       command;
    struct d_arg_error         error;
    struct d_arg_error         definition;
    struct d_arg_formatter     formatter;
    struct d_arg_config_format config_format;
    struct d_pack_sink         out;
    struct d_pack_sink         err;
    struct d_arg_text_list     pending;
    struct d_arg_string_list   pool;
    struct d_arg_string_list   utf8_argv;
    struct d_arg_option*       completion;
    int                        response_quoting;
    uint32_t                   flags;
};


D_EXTERN_C_BEGIN

//==============================================================================
// 3.  LIFETIME
//==============================================================================


// 3.1    Making and releasing
//------------------------------------------------------------------------------
// 3.1.1
// d_arg_parser_init
/**
 * @brief Makes a parser and its root command; CLI11's App(description,
 *        name).
 *
 * @note The root has CLI11's `-h,--help` flag, "Print this help message and
 *       exit", and starts from the option defaults, help layout and TOML
 *       configuration format CLI11's App starts from. Output goes to
 *       standard output and standard error. Windows-style options and the
 *       integer rule follow D_INTERNAL_ARG_PARSER_WINDOWS_STYLE and
 *       D_INTERNAL_ARG_PARSER_INTEGER_RULE; suggestions are on and response
 *       files off.
 *
 * @param[out] _parser       the parser; whatever it held is ignored.
 * @param[in]  _description  the program's description, copied; NULL for
 *                           none.
 * @param[in]  _name         the program's name, copied; NULL or "" to take
 *                           it from `argv[0]` at each parse.
 * @post   the parser can be released, and parsed, even when this fails: the
 *         failure is its definition error, which every parse reports.
 * @retval D_ARG_OK                   made.
 * @retval D_ARG_ERROR_OUT_OF_MEMORY  a copy could not be made.
 * @retval D_ARG_ERROR_BAD_ARGUMENT   `_parser` is NULL.
 */
int     d_arg_parser_init(struct d_arg_parser* _parser,
                          const char*          _description,
                          const char*          _name);

// 3.1.2
// d_arg_parser_release
/**
 * @brief Frees everything a parser owns.
 *
 * @param[in,out] _parser  the parser; may be NULL.
 * @post   every command, option, value and error the parser held is
 *         invalid, as are the arguments d_arg_parser_ensure_utf8 recovered;
 *         the parser can be made again with d_arg_parser_init.
 */
void    d_arg_parser_release(struct d_arg_parser* _parser);

// 3.2    Parse state
//------------------------------------------------------------------------------
// 3.2.1
// d_arg_parser_clear
/**
 * @brief Forgets the last parse; CLI11's clear() on the root.
 *
 * @note Every definition stays, a definition error included. Every parse
 *       clears first, so this only frees a parse's storage early.
 *
 * @param[in,out] _parser  the parser; may be NULL.
 * @post   every command is unparsed and every option holds no value; the
 *         values, the remaining arguments and the error the last parse gave
 *         are invalid.
 */
void    d_arg_parser_clear(struct d_arg_parser* _parser);


//==============================================================================
// 4.  PARSING
//==============================================================================
// Each parse first forgets the last one, then reports the parser's
// definition error, if it has one, without parsing. Otherwise it checks the
// definitions CLI11 checks only when parsing, parses, and processes:
// configuration files, environment variables, the callbacks in priority
// order with the help flags among them, the requirements, the arguments
// left over. D_ARG_OK means every callback ran and the program goes on.
// Any other status -- a completed request (D_ARG_CALL_FOR_HELP,
// D_ARG_CALL_FOR_ALL_HELP, D_ARG_CALL_FOR_VERSION,
// D_ARG_CALL_FOR_COMPLETION, D_ARG_SUCCESS) or an error -- is described by
// d_arg_parser_error and ended by d_arg_parser_exit.


// 4.1    Arguments
//------------------------------------------------------------------------------
// 4.1.1
// d_arg_parser_parse
/**
 * @brief Parses main's arguments; CLI11's parse(argc, argv).
 *
 * @note A root without a name -- or named from `argv[0]` before -- takes
 *       `argv[0]` as its name. A value that is a whole argument, or its
 *       tail after `=` or a short name, borrows `argv`'s storage.
 *
 * @param[in,out] _parser  the parser.
 * @param[in]     _argc    the argument count; 0 or less for none.
 * @param[in]     _argv    the arguments, never modified; `_argv[0]` is the
 *                         program's name. Typed as POSIX's getopt types it,
 *                         so main's `argv` converts without a cast.
 * @pre    `_argv` outlives the values the parse gives.
 * @return D_ARG_OK, a completed request or an error, as described above;
 *         D_ARG_ERROR_BAD_ARGUMENT for a NULL parser, a NULL `_argv` with a
 *         positive `_argc`, or a NULL argument before `_argv[_argc]`.
 */
int     d_arg_parser_parse(struct d_arg_parser* _parser,
                           int                  _argc,
                           char* const*         _argv);

// 4.1.2
// d_arg_parser_parse_args
/**
 * @brief Parses arguments with no program name before them; CLI11's
 *        parse(std::vector<std::string>).
 *
 * @param[in,out] _parser  the parser.
 * @param[in]     _args    the arguments; may be NULL when `_count` is 0.
 * @param[in]     _count   how many.
 * @param[in]     _flags   D_ARG_ARGS_REVERSED when `_args` holds the last
 *                         argument first, as CLI11's vector does;
 *                         D_ARG_ARGS_COPY to have the parser copy them.
 * @pre    without D_ARG_ARGS_COPY, each argument is followed by a NUL at
 *         `data[length]` and outlives the values the parse gives.
 * @return a status, as d_arg_parser_parse's; D_ARG_ERROR_OUT_OF_MEMORY when
 *         the arguments could not be copied or held.
 */
int     d_arg_parser_parse_args(struct d_arg_parser*      _parser,
                                const struct d_pack_text* _args,
                                size_t                    _count,
                                uint32_t                  _flags);

// 4.1.3
// d_arg_parser_parse_line
/**
 * @brief Parses a command line held in one string; CLI11's parse(string,
 *        program_name_included).
 *
 * @note The line is split as d_arg_split_line splits it with
 *       D_ARG_QUOTING_CLI11 -- reading `:` as `=` when the root takes
 *       Windows-style options -- into words the parser keeps. With a program
 *       name, the name is what d_arg_split_program_name finds, and it names
 *       a root without one.
 *
 * @param[in,out] _parser             the parser.
 * @param[in]     _line               the command line.
 * @param[in]     _with_program_name  whether the line starts with the
 *                                    program's name.
 * @return a status, as d_arg_parser_parse's; D_ARG_ERROR_INVALID for a
 *         malformed escape, as CLI11's ParseError(InvalidError).
 */
int     d_arg_parser_parse_line(struct d_arg_parser* _parser,
                                struct d_pack_text   _line,
                                bool                 _with_program_name);

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

// 4.2    Wide arguments
//------------------------------------------------------------------------------
// 4.2.1
// d_arg_parser_parse_wide
/**
 * @brief Parses wmain's arguments; CLI11's parse(argc, const wchar_t*
 *        const* argv).
 *
 * @note Each argument is converted to UTF-8, as d_arg_utf8_from_wide
 *       converts, into words the parser keeps until the next parse; then as
 *       d_arg_parser_parse.
 *
 * @param[in,out] _parser  the parser.
 * @param[in]     _argc    the argument count; 0 or less for none.
 * @param[in]     _argv    the arguments, never modified; `_argv[0]` is the
 *                         program's name.
 * @return a status, as d_arg_parser_parse's; D_ARG_ERROR_OUT_OF_MEMORY when
 *         the arguments could not be converted.
 */
int     d_arg_parser_parse_wide(struct d_arg_parser* _parser,
                                int                  _argc,
                                wchar_t* const*      _argv);

// 4.2.2
// d_arg_parser_parse_line_wide
/**
 * @brief Parses a wide command line; CLI11's parse(std::wstring,
 *        program_name_included).
 *
 * @param[in,out] _parser             the parser.
 * @param[in]     _line               the command line; may be NULL when
 *                                    `_length` is 0.
 * @param[in]     _length             its length in `wchar_t` units.
 * @param[in]     _with_program_name  whether the line starts with the
 *                                    program's name.
 * @return a status, as d_arg_parser_parse_line's; D_ARG_ERROR_OUT_OF_MEMORY
 *         when the line could not be converted.
 */
int     d_arg_parser_parse_line_wide(struct d_arg_parser* _parser,
                                     const wchar_t*       _line,
                                     size_t               _length,
                                     bool                 _with_program_name);

// 4.2.3
// d_arg_parser_ensure_utf8
/**
 * @brief Recovers main's arguments in UTF-8; CLI11's ensure_utf8.
 *
 * @note On Windows `argv` holds the arguments in the ANSI code page, which
 *       cannot hold every character. This splits the process's wide command
 *       line by the C runtime's rules into UTF-8 copies the parser keeps,
 *       and returns them when they are as many as `argv` holds -- they are
 *       not when the runtime expanded wildcards, and `argv` is returned.
 *       Elsewhere `argv` is UTF-8 already and is returned as it is.
 *
 * @param[in,out] _parser  the parser, which keeps the copies.
 * @param[in]     _argv    main's arguments, ending in NULL.
 * @return the arguments to parse: the copies, NULL-terminated and valid
 *         until the next call or the parser's release, or `_argv`. When the
 *         copies could not be made, `_argv`, after recording
 *         D_ARG_ERROR_OUT_OF_MEMORY as the parser's definition error.
 */
char**  d_arg_parser_ensure_utf8(struct d_arg_parser* _parser,
                                 char**               _argv);

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 4.3    Configuration text
//------------------------------------------------------------------------------
// 4.3.1
// d_arg_parser_parse_config
/**
 * @brief Parses configuration text as if a configuration file held it;
 *        CLI11's parse_from_stream.
 *
 * @note The text is read with the root's configuration format. Its items
 *       give values to the options they name that have none -- so it adds
 *       to a parse of the command line rather than replacing it -- and the
 *       processing that ends a parse follows. Without a parse before it,
 *       the definitions are checked first, as a parse checks them.
 *
 * @param[in,out] _parser  the parser.
 * @param[in]     _text    the configuration text.
 * @return a status, as d_arg_parser_parse's; D_ARG_ERROR_INVALID for a
 *         malformed escape in the text, described in CLI11's words;
 *         D_ARG_ERROR_CONFIG for an item no option claims, when
 *         configuration extras are errors.
 */
int     d_arg_parser_parse_config(struct d_arg_parser* _parser,
                                  struct d_pack_text   _text);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES


//==============================================================================
// 5.  ENDING
//==============================================================================


// 5.1    Exit
//------------------------------------------------------------------------------
// 5.1.1
// d_arg_parser_exit
/**
 * @brief Ends a parse the way its status calls for; CLI11's exit().
 *
 * @note D_ARG_CALL_FOR_HELP and D_ARG_CALL_FOR_ALL_HELP write the help of
 *       the last subcommand the parse used; D_ARG_CALL_FOR_VERSION the
 *       version the parse read from the command whose version flag was
 *       given -- the error's message -- and a newline; and
 *       D_ARG_CALL_FOR_COMPLETION the completion script the parse made, the
 *       message too; each to the parser's output. An error writes the root's
 *       failure message -- its own, or d_arg_failure_simple -- to the
 *       parser's error output. D_ARG_OK, D_ARG_SUCCESS and
 *       D_ARG_RUNTIME_ERROR write nothing. When `_status` is the status of
 *       the parser's error, that error's message and exit code are used; any
 *       other status is reported with its own.
 *
 * @param[in] _parser  the parser.
 * @param[in] _status  the status to end with, usually the last parse's.
 * @return the code to end the process with: CLI11's for the status, or the
 *         one a runtime error chose; D_ARG_EXIT_BASE_CLASS for a NULL
 *         parser.
 */
int     d_arg_parser_exit(const struct d_arg_parser* _parser,
                          int                        _status);

// 5.1.2
// d_arg_parser_exit_to
//   ending: d_arg_parser_exit writing to the sinks given rather than the
// parser's, as CLI11's exit(error, out, err).
int     d_arg_parser_exit_to(const struct d_arg_parser* _parser,
                             int                        _status,
                             struct d_pack_sink         _out,
                             struct d_pack_sink         _err);

// 5.1.3
// d_arg_parser_parse_or_exit
/**
 * @brief Parses main's arguments and, unless the parse came to D_ARG_OK,
 *        ends it with d_arg_parser_exit; the body of D_ARG_PARSE.
 *
 * @note In C++ the arguments are `const char* const*`, as CLI11's parse
 *       takes them, so an array of string literals passes too.
 *
 * @param[in,out] _parser     the parser.
 * @param[in]     _argc       the argument count.
 * @param[in]     _argv       the arguments.
 * @param[out]    _exit_code  receives d_arg_parser_exit's code when the
 *                            program is to end.
 * @return `true` when the program is to end with `*_exit_code`; `false`
 *         when it goes on.
 */
#if !defined(__cplusplus)

D_INLINE bool
d_arg_parser_parse_or_exit(
    struct d_arg_parser* _parser,
    int                  _argc,
    char* const*         _argv,
    int*                 _exit_code
)
{
    const int status = d_arg_parser_parse(_parser,
                                          _argc,
                                          _argv);

    // the program goes on
    if (status == (int)D_ARG_OK)
    {
        return false;
    }

    *_exit_code = d_arg_parser_exit(_parser,
                                    status);

    return true;
}

#else

D_INLINE bool
d_arg_parser_parse_or_exit(
    struct d_arg_parser* _parser,
    int                  _argc,
    const char* const*   _argv,
    int*                 _exit_code
)
{
    // the parse never writes through _argv, so dropping const is sound
    const int status = d_arg_parser_parse(_parser,
                                          _argc,
                                          const_cast<char* const*>(_argv));

    // the program goes on
    if (status == static_cast<int>(D_ARG_OK))
    {
        return false;
    }

    *_exit_code = d_arg_parser_exit(_parser,
                                    status);

    return true;
}

#endif  // __cplusplus

// 5.1.4
// D_ARG_PARSE
//   macro: CLI11_PARSE, for C and C++: parses main's arguments with a parser
// and, unless the parse came to D_ARG_OK, returns d_arg_parser_exit's code
// from the function it is used in. A macro because returning from its
// caller is its purpose, which no function can do; the work is
// d_arg_parser_parse_or_exit's, and each argument is evaluated once.
#define D_ARG_PARSE(parser,                                                    \
                    argc,                                                      \
                    argv)                                                      \
    do                                                                         \
    {                                                                          \
        int d_internal_arg_exit_code = 0;                                      \
                                                                               \
        if (d_arg_parser_parse_or_exit((parser),                               \
                                       (argc),                                 \
                                       (argv),                                 \
                                       &d_internal_arg_exit_code))             \
        {                                                                      \
            return d_internal_arg_exit_code;                                   \
        }                                                                      \
    } while (0)

// 5.2    Failure messages
//------------------------------------------------------------------------------
//   d_arg_failure_fn values for d_arg_command_set_failure_message: CLI11's
// two FailureMessage functions. Each returns D_ARG_OK, or D_ARG_ERROR_FILE
// when the sink refused the text.

// 5.2.1
// d_arg_failure_simple
//   failure message: CLI11's FailureMessage::simple, the default: the error's
// message and a newline, then "Run with <help> for more information." when
// the command has a help flag -- "<help> or <help-all>" when it has both.
int     d_arg_failure_simple(const struct d_arg_command* _command,
                             const struct d_arg_error*   _error,
                             struct d_pack_sink          _sink,
                             void*                       _context);

// 5.2.2
// d_arg_failure_help
//   failure message: CLI11's FailureMessage::help: "ERROR: <name>: <message>"
// and a newline, then the command's help.
int     d_arg_failure_help(const struct d_arg_command* _command,
                           const struct d_arg_error*   _error,
                           struct d_pack_sink          _sink,
                           void*                       _context);

// 5.3    Failing from a callback
//------------------------------------------------------------------------------
// 5.3.1
// d_arg_command_fail / d_arg_option_fail
/**
 * @brief Records why a callback ends the parse, for the parse to report;
 *        what throwing one of CLI11's errors from a callback does.
 *
 * @note The callback then returns what this returns. option_fail is the
 *       same for an option's callback, and names the option in the error.
 *
 * @param[in,out] _command  the command whose callback failed.
 * @param[in]     _status   the d_arg_status to end with: D_ARG_SUCCESS,
 *                          D_ARG_RUNTIME_ERROR or an error.
 * @param[in]     _format   the message's format string; NULL for the
 *                          status's own text.
 * @return `_status`; D_ARG_ERROR_BAD_ARGUMENT for a NULL command.
 */
int     d_arg_command_fail(const struct d_arg_command* _command,
                           int                         _status,
                           const char*                 _format,
                           ...) D_FORMAT_PRINTF(3, 4);
int     d_arg_option_fail(const struct d_arg_option* _option,
                          int                        _status,
                          const char*                _format,
                          ...) D_FORMAT_PRINTF(3, 4);

// 5.3.2
// d_arg_command_fail_runtime
//   failure: CLI11's RuntimeError(exit_code): records D_ARG_RUNTIME_ERROR,
// with no message and the code the process is to end with, and returns
// D_ARG_RUNTIME_ERROR; D_ARG_ERROR_BAD_ARGUMENT for a NULL command.
int     d_arg_command_fail_runtime(const struct d_arg_command* _command,
                                   int                         _exit_code);


//==============================================================================
// 6.  SETTINGS
//==============================================================================


// 6.1    Output
//------------------------------------------------------------------------------
// 6.1.1
// d_arg_parser_set_output
//   setting: where help, versions, completion scripts and deprecation
// warnings are written (`_out`, standard output at first) and where failure
// messages are (`_err`, standard error at first), as CLI11's exit() takes
// its streams. Each sink must outlive its use. Does not fail.
void    d_arg_parser_set_output(struct d_arg_parser* _parser,
                                struct d_pack_sink   _out,
                                struct d_pack_sink   _err);

// 6.2    Extensions
//------------------------------------------------------------------------------
#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// 6.2.1
// d_arg_parser_set_response_files
/**
 * @brief Makes an `@file` argument stand for the arguments its file holds,
 *        as GCC's and MSVC's do.
 *
 * @note Before parsing, an argument starting with `@` -- unless it follows
 *       `--` -- is replaced by the words of its file, split as
 *       d_arg_read_response_file splits them; those starting with `@` are
 *       replaced in turn, up to D_INTERNAL_ARG_PARSER_RESPONSE_DEPTH files
 *       deep, below which the parse fails with D_ARG_ERROR_FILE. A file that
 *       cannot be read leaves its argument as it is, as GCC does. Off at
 *       first.
 *
 * @param[in,out] _parser   the parser.
 * @param[in]     _enabled  whether `@file` is replaced.
 * @param[in]     _quoting  a d_arg_quoting value, the rules a file is split
 *                          by; D_ARG_QUOTING_NATIVE_RESPONSE for the
 *                          platform's.
 */
void    d_arg_parser_set_response_files(struct d_arg_parser* _parser,
                                        bool                 _enabled,
                                        int                  _quoting);

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

#if (D_INTERNAL_ARG_PARSER_SUGGESTIONS == 1)

// 6.2.2
// d_arg_parser_set_suggestions
//   setting: whether the message for arguments the parse did not expect
// adds a line for each one with the option or subcommand name it most
// likely meant -- "Did you mean '--verbose'?", or, among several, "Did you
// mean '--verbose' for '--verbse'?" -- the one name nearest by optimal string
// alignment distance (Damerau-Levenshtein's, a swap of neighbours one edit),
// within D_INTERNAL_ARG_PARSER_SUGGESTION_DISTANCE edits and no more than
// half the longer name, among those the command, its option groups, the
// parents it falls through to and the subcommands the parse used would
// have taken; hidden and disabled ones are never named. On at first; off,
// the message is CLI11's. Does not fail.
void    d_arg_parser_set_suggestions(struct d_arg_parser* _parser,
                                     bool                 _enabled);

#endif  // D_INTERNAL_ARG_PARSER_SUGGESTIONS


//==============================================================================
// 7.  ERRORS
//==============================================================================


// 7.1    The last parse
//------------------------------------------------------------------------------
// 7.1.1
// d_arg_parser_error
//   lookup: what the last parse ended with -- D_ARG_OK for one that did not
// fail -- valid until the next parse, clear or release; NULL for a NULL
// parser.
const struct d_arg_error*
        d_arg_parser_error(const struct d_arg_parser* _parser);

// 7.2    Definitions
//------------------------------------------------------------------------------
//   A definition that fails -- a bad name, a name taken, a copy that could not
// be made -- returns its failure and records it as the parser's definition
// error, unless one is recorded already: the first is kept. Every parse
// reports it, without parsing, until it is cleared, as CLI11 throws it where
// the definition is made.

// 7.2.1
// d_arg_parser_definition_error
//   lookup: the definition error; D_ARG_OK when there is none. NULL for a
// NULL parser.
const struct d_arg_error*
        d_arg_parser_definition_error(const struct d_arg_parser* _parser);

// 7.2.2
// d_arg_parser_clear_definition_error
//   operation: forgets the definition error, for a caller that has dealt with
// the failed definition -- as catching CLI11's exception there does -- so
// that parsing goes ahead. Ignores NULL.
void    d_arg_parser_clear_definition_error(struct d_arg_parser* _parser);

D_EXTERN_C_END


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_PARSER_H
