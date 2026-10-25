/*******************************************************************************
* djinterp [parse]                                                arg_parser.hpp
*
* The argument parser's C++ face: the parser.
*   `cli::arg_parser` is CLI11's App at the root of a command tree. It derives
* from struct d_arg_parser, through command_interface (arg_command.hpp),
* which gives it CLI11's App interface over its root command, and adds
* nothing to it, so that `&app` is the d_arg_parser* every C call takes, and
* a parser that C and C++ build together is one parser. A parser converts to
* the cli::command handle on its root, so that it can be given wherever a
* command is.
*   A parse returns a cli::status where CLI11's throws, and exit() ends it as
* CLI11's exit() ends one: help, a version, a completion script or a
* failure message written, and the code to end the process with returned.
* D_ARG_PARSE(app, argc, argv) does both for main, as CLI11_PARSE does,
* given the parser or its address. A definition that fails -- a name that
* is bad or taken -- is kept as the parser's definition error, which the
* next parse reports, as CLI11 throws it where the definition is made.
*   A parser holds pointers into itself -- each command to its parser and
* its parent -- so it is neither copied nor moved.
*   This is also the C++ face's umbrella header: including it includes every
* header of the face, and so the C library's.
*
*
* path:      /inc/djinterp/parse/cli/arg_parser/arg_parser.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.17
*                                                            revised: 2026.10.17
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE PARSER
    ----------
    1.  The class
         1.  arg_parser
    2.  Layout
         1.  The parser adds no member
2.  MAIN
    ----
    1.  D_ARG_PARSE
         1.  d_arg_parser_parse_or_exit
3.  INTERNALS
    ---------
    1.  Leftovers
         1.  passthrough
4.  DEFINITIONS
    -----------
    1.  arg_parser
    2.  D_ARG_PARSE
    3.  Internals
*/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_ARG_PARSER_HPP
#define DJINTERP_PARSE_CLI_ARG_PARSER_ARG_PARSER_HPP 1

// std
#include <cstddef>    // std::size_t
#include <istream>    // std::istream
#include <iterator>   // std::istreambuf_iterator
#include <ostream>    // std::ostream
#include <string>     // std::string, std::wstring
#include <vector>     // std::vector
// djinterp
#include "../../../djinterp.hpp"                // framework root
#include "../../../c/util/sink_common.h"        // d_pack_text, d_pack_sink
#include "../../../config/parse/cli/arg_parser/cfg_arg_parser.h"  // knobs
#include "../../../env/env.h"                   // language levels
#include "./arg_argv.hpp"                       // umbrella: arguments
                                                // from text
#include "./arg_command.hpp"                    // command_interface,
                                                // umbrella: commands
#include "./arg_common.hpp"                     // umbrella: status, text,
                                                // c_string, error, sinks
#include "./arg_completion.hpp"                 // umbrella: completion
#include "./arg_config.hpp"                     // umbrella: configuration
                                                // formats
#include "./arg_formatter.hpp"                  // umbrella: formatters
#include "./arg_option.hpp"                     // umbrella: options
#include "./arg_type.hpp"                       // umbrella: types
#include "./arg_validator.hpp"                  // umbrella: validators
#include "./c/arg_parser.h"                     // d_arg_parser, the C
                                                // library


namespace djinterp
{
namespace parse
{
namespace cli
{

//==============================================================================
// 1.  THE PARSER
//==============================================================================


// 1.1    The class
//------------------------------------------------------------------------------
// 1.1.1
// arg_parser
//   class: CLI11's App at the root: a d_arg_parser made by its constructor and
// released by its destructor, with CLI11's App interface from
// command_interface, whose settings return the parser, and the parse, the
// ending and the parser's own settings as members. Every member that names a
// C call does what the call's contract says (arg_parser.h), and the C
// struct's fields may be read as its layout allows; a member function hides
// the field of its name, and the fields `command` and `error` hide the
// face's types of those names, which the class's members spell `cli::`.
class arg_parser
    : public command_interface<arg_parser, ::d_arg_parser>
{
public:
    // making and releasing: CLI11's App(description, name); a failure to
    // copy either is the definition error, which every parse reports
    explicit arg_parser(c_string _description = c_string(),
                        c_string _name        = c_string()) D_NOEXCEPT;
    ~arg_parser();

    // parsing: every parse first forgets the last one, then reports the
    // definition error, if any, without parsing; a status that is not ok()
    // goes to exit()

    /**
     * @brief Parses main's arguments; CLI11's parse(argc, argv).
     *
     * @note Values that are whole arguments, or their tails, borrow
     *       `_argv`'s storage.
     *
     * @param[in] _argc  the argument count; 0 or less for none.
     * @param[in] _argv  the arguments, `_argv[0]` the program's name, never
     *                   modified; `char**` converts.
     * @pre    `_argv` outlives the values the parse gives.
     * @return the parse's status, as d_arg_parser_parse's.
     */
    D_NODISCARD
    status      parse(int                _argc,
                      const char* const* _argv);
    /**
     * @brief Parses a command line held in one string; CLI11's
     *        parse(commandline, program_name_included).
     *
     * @param[in] _line                   the line, which may hold NUL bytes;
     *                                    a C string or a std::string
     *                                    converts.
     * @param[in] _program_name_included  whether the line starts with the
     *                                    program's name.
     * @return the parse's status, as d_arg_parser_parse_line's.
     */
    D_NODISCARD
    status      parse(const text& _line,
                      bool        _program_name_included = false);
    /**
     * @brief Parses arguments given last first; CLI11's
     *        parse(std::vector<std::string>&).
     *
     * @note The parser copies the arguments, so the vector may change
     *       afterwards. After a parse that came to status_code::ok, the
     *       vector holds the arguments the root did not use, last first, as
     *       CLI11's remaining_for_passthrough gives them; after any other,
     *       it is as it was given.
     *
     * @param[in,out] _args  the arguments, the last one first.
     * @return the parse's status, as d_arg_parser_parse_args's; std::bad_alloc
     *         is thrown, as std::vector and std::string throw it, when the
     *         vector of spans or the leftovers cannot be made.
     */
    D_NODISCARD
    status      parse(std::vector<std::string>& _args);

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    /**
     * @brief Parses arguments given last first, which the call consumes;
     *        CLI11's parse(std::vector<std::string>&&).
     *
     * @param[in] _args  the arguments, the last one first; copied.
     * @return the parse's status, as the vector's overload's.
     */
    D_NODISCARD
    status      parse(std::vector<std::string>&& _args);
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)
    /**
     * @brief Parses wmain's arguments; CLI11's parse(argc, const wchar_t*
     *        const* argv).
     *
     * @param[in] _argc  the argument count; 0 or less for none.
     * @param[in] _argv  the arguments, `_argv[0]` the program's name.
     * @return the parse's status, as d_arg_parser_parse_wide's.
     */
    D_NODISCARD
    status      parse(int                   _argc,
                      const wchar_t* const* _argv);
    /**
     * @brief Parses a wide command line; CLI11's parse(std::wstring,
     *        program_name_included).
     *
     * @param[in] _line                   the line.
     * @param[in] _program_name_included  whether the line starts with the
     *                                    program's name.
     * @return the parse's status, as d_arg_parser_parse_line_wide's.
     */
    D_NODISCARD
    status      parse(const std::wstring& _line,
                      bool                _program_name_included = false);
    /**
     * @brief Recovers main's arguments in UTF-8; CLI11's ensure_utf8.
     *
     * @param[in] _argv  main's arguments, ending in NULL.
     * @return the arguments to parse, as d_arg_parser_ensure_utf8 returns
     *         them: the parser's UTF-8 copies, or `_argv`.
     */
    D_NODISCARD
    char**      ensure_utf8(char** _argv);
#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    /**
     * @brief Parses configuration text from a stream as if a configuration
     *        file held it; CLI11's parse_from_stream.
     *
     * @note The stream is read to its end first; its items give values to
     *       the options that have none, and the processing that ends a parse
     *       follows.
     *
     * @param[in,out] _input  the stream.
     * @return the parse's status, as d_arg_parser_parse_config's;
     *         std::bad_alloc, or the stream's own exception, is thrown when
     *         the text cannot be read into a std::string.
     */
    D_NODISCARD
    status      parse_from_stream(std::istream& _input);
    /**
     * @brief Parses configuration text as if a configuration file held it;
     *        d_arg_parser_parse_config.
     *
     * @param[in] _text  the text.
     * @return the parse's status, as d_arg_parser_parse_config's.
     */
    D_NODISCARD
    status      parse_config(const text& _text);
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

    // forgetting the last parse early: CLI11's clear(), on the whole
    // parser; every parse clears first, so this only frees the parse's
    // storage sooner
    void        clear() D_NOEXCEPT;

    // ending: CLI11's exit(), writing what the status calls for -- help, a
    // version, a completion script, the failure message -- to the parser's
    // output and error output, or to the streams given, and returning the
    // code to end the process with. A status that is the last parse's is
    // reported with that parse's message and code, any other with its own;
    // an error view as d_arg_command_exit reports it from the root.
    int         exit(const status& _status) const;
    int         exit(const status& _status,
                     std::ostream& _out) const;
    int         exit(const status& _status,
                     std::ostream& _out,
                     std::ostream& _err) const;
    int         exit(const cli::error& _error) const;
    int         exit(const cli::error& _error,
                     std::ostream&     _out) const;
    int         exit(const cli::error& _error,
                     std::ostream&     _out,
                     std::ostream&     _err) const;

    // errors: views of the last parse's report and of the definition error,
    // valid until the next parse, clear or release; status_code::ok when
    // there is none. Clearing the definition error lets parsing go ahead,
    // as catching CLI11's exception where the definition was made does.
    cli::error  get_error() const D_NOEXCEPT;
    cli::error  get_definition_error() const D_NOEXCEPT;
    arg_parser& clear_definition_error() D_NOEXCEPT;

    // output: where help, versions, completion scripts and deprecation
    // warnings go, and where failure messages go -- standard output and
    // standard error at first -- as sinks, or as streams; either must
    // outlive its use
    arg_parser& output(::d_pack_sink _out,
                       ::d_pack_sink _err) D_NOEXCEPT;
    arg_parser& output(std::ostream& _out,
                       std::ostream& _err) D_NOEXCEPT;

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)
    // response files: whether an `@file` argument stands for the arguments
    // its file holds, split by the rules given -- the platform's for
    // response files when none are -- off at first. See
    // d_arg_parser_set_response_files.
    arg_parser&    response_files(bool _enabled = true) D_NOEXCEPT;
    arg_parser&    response_files(bool           _enabled,
                                  quoting::value _quoting) D_NOEXCEPT;
    bool           get_response_files() const D_NOEXCEPT;
    quoting::value get_response_quoting() const D_NOEXCEPT;
#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

#if (D_INTERNAL_ARG_PARSER_SUGGESTIONS == 1)
    // suggestions: whether an unexpected argument is reported with the name
    // it most likely meant; on at first. See d_arg_parser_set_suggestions.
    arg_parser& suggestions(bool _enabled = true) D_NOEXCEPT;
    bool        get_suggestions() const D_NOEXCEPT;
#endif  // D_INTERNAL_ARG_PARSER_SUGGESTIONS

#if (D_INTERNAL_ARG_PARSER_COMPLETION == 1)
    // completion: the option that asks for a completion script -- `prog
    // --completion bash` ends the parse with call_for_completion, and exit()
    // writes the script -- replacing the one before; no names removes it,
    // and returns a NULL handle, as a failure does. See
    // d_arg_parser_set_completion_flag.
    cli::option set_completion_flag(c_string _names       = c_string(),
                                    c_string _description = c_string());
    cli::option get_completion_ptr() const D_NOEXCEPT;
#endif  // D_INTERNAL_ARG_PARSER_COMPLETION

private:
    D_DELETED_FN(arg_parser(const arg_parser& _other))
    D_DELETED_FN(arg_parser& operator=(const arg_parser& _other))
};

// 1.2    Layout
//------------------------------------------------------------------------------
// 1.2.1
// The parser adds no member
//   assertion: an arg_parser is its d_arg_parser and nothing more, so the C++
// face costs no storage and `&app` is the C parser itself.
D_STATIC_ASSERT( (sizeof(arg_parser) == sizeof(::d_arg_parser)),
                 "cli::arg_parser adds no member to struct d_arg_parser" );


//==============================================================================
// 2.  MAIN
//==============================================================================


// 2.1    D_ARG_PARSE
//------------------------------------------------------------------------------
// 2.1.1
// d_arg_parser_parse_or_exit
/**
 * @brief Parses main's arguments with a parser and, unless the parse came to
 *        status_code::ok, ends it; what D_ARG_PARSE calls when it is given
 *        the parser rather than its address.
 *
 * @note Named as the C function D_ARG_PARSE calls, and found by
 *       argument-dependent lookup when the macro's first argument is an
 *       arg_parser, so that `D_ARG_PARSE(app, argc, argv)` reads as
 *       CLI11_PARSE(app, argc, argv) does; given `&app`, the macro calls the
 *       C function, to the same effect. A wide overload serves wmain.
 *
 * @param[in,out] _parser     the parser.
 * @param[in]     _argc       the argument count.
 * @param[in]     _argv       the arguments.
 * @param[out]    _exit_code  receives exit()'s code when the program is to
 *                            end.
 * @return `true` when the program is to end with `*_exit_code`; `false`
 *         when it goes on.
 */
bool d_arg_parser_parse_or_exit(arg_parser&        _parser,
                                int                _argc,
                                const char* const* _argv,
                                int*               _exit_code);

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)
/**
 * @brief The wide form of d_arg_parser_parse_or_exit, for wmain.
 *
 * @param[in,out] _parser     the parser.
 * @param[in]     _argc       the argument count.
 * @param[in]     _argv       the wide arguments.
 * @param[out]    _exit_code  receives exit()'s code when the program is to
 *                            end.
 * @return `true` when the program is to end with `*_exit_code`; `false`
 *         when it goes on.
 */
bool d_arg_parser_parse_or_exit(arg_parser&           _parser,
                                int                   _argc,
                                const wchar_t* const* _argv,
                                int*                  _exit_code);
#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV


//==============================================================================
// 3.  INTERNALS
//==============================================================================


// 3.1    Leftovers
//------------------------------------------------------------------------------
NS_INTERNAL

    // 3.1.1
    // passthrough
    /**
     * @brief Replaces a vector of arguments with the ones a command did not
     *        use, last first; CLI11's remaining_for_passthrough(false).
     *
     * @param[in]  _command  the command.
     * @param[out] _args     receives the arguments, replacing what it held
     *                       only once they are all made.
     * @post   std::bad_alloc, as std::vector and std::string throw it, leaves
     *         `_args` as it was.
     */
    void passthrough(const ::d_arg_command&    _command,
                     std::vector<std::string>& _args);

NS_END  // internal


//==============================================================================
// 4.  DEFINITIONS
//==============================================================================


// 4.1    arg_parser
//------------------------------------------------------------------------------
// arg_parser::arg_parser / ~arg_parser
//   d_arg_parser_init over the base, which is plain data and so already
// there to be made; d_arg_parser_release. init records its own failure as
// the definition error, so the parser is always one that can be parsed and
// released.
inline
arg_parser::arg_parser(
    c_string _description,
    c_string _name
) D_NOEXCEPT
    : command_interface<arg_parser, ::d_arg_parser>()
{
    (void)d_arg_parser_init(this,
                            _description.get(),
                            _name.get());
}

inline
arg_parser::~arg_parser()
{
    d_arg_parser_release(this);
}

// arg_parser::parse (argc, argv)
//   The C parse never writes through the arguments, so dropping the const
// CLI11's signature carries is sound.
inline status
arg_parser::parse(
    int                _argc,
    const char* const* _argv
)
{
    return status(d_arg_parser_parse(this,
                                     _argc,
                                     const_cast<char* const*>(_argv)));
}

// arg_parser::parse (line)
//   The text is the span the C call takes.
inline status
arg_parser::parse(
    const text& _line,
    bool        _program_name_included
)
{
    return status(d_arg_parser_parse_line(this,
                                          _line,
                                          _program_name_included));
}

// arg_parser::parse (vector)
//   A span for each string, in the vector's order, which the C call reads
// as last first; copied by the parser, since the vector is then replaced
// with the leftovers, whose strings the values could not borrow.
inline status
arg_parser::parse(
    std::vector<std::string>& _args
)
{
    std::vector< ::d_pack_text> words;

    words.reserve(_args.size());

    // each argument as the span of its string, terminator after it
    for (std::size_t index = 0u;
         index < _args.size();
         ++index)
    {
        words.push_back(text(_args[index]));
    }

    const status result(
        d_arg_parser_parse_args(this,
                                words.empty() ? D_NULLPTR : &words[0],
                                words.size(),
                                (D_ARG_ARGS_REVERSED | D_ARG_ARGS_COPY)));

    // a parse that went on leaves what the root did not use
    if (result.ok())
    {
        internal::passthrough(this->d_arg_parser::command,
                              _args);
    }

    return result;
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// arg_parser::parse (vector, consumed)
//   The lvalue overload, on the vector the caller gave away.
inline status
arg_parser::parse(
    std::vector<std::string>&& _args
)
{
    return parse(_args);
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

// arg_parser::parse (wide argc, argv) / parse (wide line) / ensure_utf8
//   The C calls, the const dropped as for the narrow arguments.
inline status
arg_parser::parse(
    int                   _argc,
    const wchar_t* const* _argv
)
{
    return status(d_arg_parser_parse_wide(this,
                                          _argc,
                                          const_cast<wchar_t* const*>(_argv)));
}

inline status
arg_parser::parse(
    const std::wstring& _line,
    bool                _program_name_included
)
{
    return status(d_arg_parser_parse_line_wide(this,
                                               _line.data(),
                                               _line.size(),
                                               _program_name_included));
}

inline char**
arg_parser::ensure_utf8(
    char** _argv
)
{
    return d_arg_parser_ensure_utf8(this,
                                    _argv);
}

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// arg_parser::parse_from_stream / parse_config
//   The stream read whole into a string, then the text parsed as the C call
// parses it.
inline status
arg_parser::parse_from_stream(
    std::istream& _input
)
{
    const std::string content((std::istreambuf_iterator<char>(_input)),
                              std::istreambuf_iterator<char>());

    return parse_config(content);
}

inline status
arg_parser::parse_config(
    const text& _text
)
{
    return status(d_arg_parser_parse_config(this,
                                            _text));
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

// arg_parser::clear
//   d_arg_parser_clear.
inline void
arg_parser::clear() D_NOEXCEPT
{
    d_arg_parser_clear(this);

    return;
}

// arg_parser::exit
//   d_arg_parser_exit, to the parser's sinks or to the streams given -- the
// error stream the parser's own when only the output is given, as CLI11's
// exit(e, out) keeps std::cerr; an error view through d_arg_command_exit
// and d_arg_command_exit_to, from the root.
inline int
arg_parser::exit(
    const status& _status
) const
{
    return d_arg_parser_exit(this,
                             _status.code());
}

inline int
arg_parser::exit(
    const status& _status,
    std::ostream& _out
) const
{
    return d_arg_parser_exit_to(this,
                                _status.code(),
                                stream_sink(_out),
                                this->d_arg_parser::err);
}

inline int
arg_parser::exit(
    const status& _status,
    std::ostream& _out,
    std::ostream& _err
) const
{
    return d_arg_parser_exit_to(this,
                                _status.code(),
                                stream_sink(_out),
                                stream_sink(_err));
}

inline int
arg_parser::exit(
    const cli::error& _error
) const
{
    return d_arg_command_exit(&this->d_arg_parser::command,
                              &_error.get());
}

inline int
arg_parser::exit(
    const cli::error& _error,
    std::ostream&     _out
) const
{
    return d_arg_command_exit_to(&this->d_arg_parser::command,
                                 &_error.get(),
                                 stream_sink(_out),
                                 this->d_arg_parser::err);
}

inline int
arg_parser::exit(
    const cli::error& _error,
    std::ostream&     _out,
    std::ostream&     _err
) const
{
    return d_arg_command_exit_to(&this->d_arg_parser::command,
                                 &_error.get(),
                                 stream_sink(_out),
                                 stream_sink(_err));
}

// arg_parser::get_error / get_definition_error / clear_definition_error
//   The C lookups, which never return NULL for a parser.
inline cli::error
arg_parser::get_error() const D_NOEXCEPT
{
    return cli::error(*d_arg_parser_error(this));
}

inline cli::error
arg_parser::get_definition_error() const D_NOEXCEPT
{
    return cli::error(*d_arg_parser_definition_error(this));
}

inline arg_parser&
arg_parser::clear_definition_error() D_NOEXCEPT
{
    d_arg_parser_clear_definition_error(this);

    return *this;
}

// arg_parser::output
//   d_arg_parser_set_output, with the streams' sinks for the second form.
inline arg_parser&
arg_parser::output(
    ::d_pack_sink _out,
    ::d_pack_sink _err
) D_NOEXCEPT
{
    d_arg_parser_set_output(this,
                            _out,
                            _err);

    return *this;
}

inline arg_parser&
arg_parser::output(
    std::ostream& _out,
    std::ostream& _err
) D_NOEXCEPT
{
    return output(stream_sink(_out),
                  stream_sink(_err));
}

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// arg_parser::response_files / get_response_files / get_response_quoting
//   d_arg_parser_set_response_files, with the platform's rules for response
// files when none are given; the switch and the rules read back from the
// parser's fields.
inline arg_parser&
arg_parser::response_files(
    bool _enabled
) D_NOEXCEPT
{
    return response_files(_enabled,
                          quoting::native_response);
}

inline arg_parser&
arg_parser::response_files(
    bool           _enabled,
    quoting::value _quoting
) D_NOEXCEPT
{
    d_arg_parser_set_response_files(this,
                                    _enabled,
                                    static_cast<int>(_quoting));

    return *this;
}

inline bool
arg_parser::get_response_files() const D_NOEXCEPT
{
    return ((this->d_arg_parser::flags & D_ARG_PARSER_RESPONSE_FILES) != 0u);
}

inline quoting::value
arg_parser::get_response_quoting() const D_NOEXCEPT
{
    return static_cast<quoting::value>(this->d_arg_parser::response_quoting);
}

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

#if (D_INTERNAL_ARG_PARSER_SUGGESTIONS == 1)

// arg_parser::suggestions / get_suggestions
//   d_arg_parser_set_suggestions; the switch read back.
inline arg_parser&
arg_parser::suggestions(
    bool _enabled
) D_NOEXCEPT
{
    d_arg_parser_set_suggestions(this,
                                 _enabled);

    return *this;
}

inline bool
arg_parser::get_suggestions() const D_NOEXCEPT
{
    return ((this->d_arg_parser::flags & D_ARG_PARSER_SUGGESTIONS) != 0u);
}

#endif  // D_INTERNAL_ARG_PARSER_SUGGESTIONS

#if (D_INTERNAL_ARG_PARSER_COMPLETION == 1)

// arg_parser::set_completion_flag / get_completion_ptr
//   d_arg_parser_set_completion_flag; the option the parser keeps.
inline cli::option
arg_parser::set_completion_flag(
    c_string _names,
    c_string _description
)
{
    return cli::option(d_arg_parser_set_completion_flag(this,
                                                        _names.get(),
                                                        _description.get()));
}

inline cli::option
arg_parser::get_completion_ptr() const D_NOEXCEPT
{
    return cli::option(this->d_arg_parser::completion);
}

#endif  // D_INTERNAL_ARG_PARSER_COMPLETION

// 4.2    D_ARG_PARSE
//------------------------------------------------------------------------------
// d_arg_parser_parse_or_exit
//   The parse, then exit() unless it went on: the C function's body, over
// the parser object.
inline bool
d_arg_parser_parse_or_exit(
    arg_parser&        _parser,
    int                _argc,
    const char* const* _argv,
    int*               _exit_code
)
{
    const status result = _parser.parse(_argc,
                                        _argv);

    // the program goes on
    if (result.ok())
    {
        return false;
    }

    *_exit_code = _parser.exit(result);

    return true;
}

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

inline bool
d_arg_parser_parse_or_exit(
    arg_parser&           _parser,
    int                   _argc,
    const wchar_t* const* _argv,
    int*                  _exit_code
)
{
    const status result = _parser.parse(_argc,
                                        _argv);

    // the program goes on
    if (result.ok())
    {
        return false;
    }

    *_exit_code = _parser.exit(result);

    return true;
}

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

// 4.3    Internals
//------------------------------------------------------------------------------
// internal::passthrough
//   The command's own extras, read from its list last to first, into a new
// vector that replaces the old only when whole.
inline void
internal::passthrough(
    const ::d_arg_command&    _command,
    std::vector<std::string>& _args
)
{
    std::vector<std::string> remaining;

    remaining.reserve(_command.missing.count);

    // the last unused argument first, as CLI11 passes them on
    for (std::size_t index = _command.missing.count;
         index > 0u;
         --index)
    {
        remaining.push_back(text(_command.missing.items[index - 1u].text));
    }

    _args.swap(remaining);

    return;
}

}  // namespace cli
}  // namespace parse
}  // namespace djinterp


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_ARG_PARSER_HPP
