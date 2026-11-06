/*******************************************************************************
* djinterp [tools]                                               cli_program.hpp
*
* The command-line kit's C++ face: the program.
*   `cli::program` is a dj_cli_program over the parser it is. It derives from
* cli::DArgParser, so every definition CLI11's App makes -- options, flags,
* option groups, settings -- is made on it as on any parser, and `&app` is
* still the dj_arg_parser* the C calls take; it holds the kit's record,
* made by its constructor once the parser is, so that the standard options
* and the rule that an option given twice takes its last value come before
* the program's own definitions, and released by its destructor before the
* parser is.
*   COMMANDS. A command's work is any function object -- a function, a
* functor, a lambda -- called as `work(program, command)` with the program
* and the command the parse reached, and returning the status the program
* exits with, an int or an exit_status. The program keeps a copy of it, and
* frees it with itself. The work runs behind the C boundary's guard: an
* exception it lets out is reported as the program's error -- its what(),
* or "out of memory" -- and the run is trouble.
*   RUNNING. run() is the whole of main: the parse, its ending where it
* asked for one, and the work of the command it reached. parse_or_exit()
* and dispatch() are its halves, for a program with something to do
* between them. DArgParser's own parse() parses without the kit: no `/?`,
* no standard commands, the standard options unread -- a program parses
* through these.
*   DIAGNOSTICS. report(), report_at(), note(), usage_error() and
* file_error() are the C reports with a message already composed; the GNU
* shape and the colour are theirs.
*   THE VERSION. Each constructor tells the kit how the program was built
* (D_CLI_BUILD_INIT), written where the constructor is compiled -- in the
* program's own code, the constructors being inline -- so that `--version
* --verbose` says it with nothing asked of the program; add_build() adds
* lines of its own.
*
*
* path:      /inc/djinterp/tools/cli/cli_program.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.11.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VOCABULARY
    ----------
    1.  Exit statuses and severities
         1.  exit_status
         2.  severity
    2.  Program flags
         1.  program_flags
         2.  plugin_versions
2.  THE PROGRAM
    -----------
    1.  The class
         1.  program
3.  INTERNALS
    ---------
    1.  A command's work
         1.  DWorkHolder
         2.  DWorkCall
         3.  DWorkFailure
         4.  work_function
4.  DEFINITIONS
    -----------
    1.  Making and releasing
    2.  Commands
    3.  Running
    4.  Settings and queries
    5.  Diagnostics
    6.  Internals
*/

#ifndef DJINTERP_TOOLS_CLI_CLI_PROGRAM_HPP
#define DJINTERP_TOOLS_CLI_CLI_PROGRAM_HPP 1

// std
#include <cstddef>    // std::size_t
#include <exception>  // std::exception
#include <ostream>    // std::ostream
#include <string>     // std::string
// djinterp
#include "../../cpp/djinterp.hpp"                         // framework root
#include "../../c/util/sink_common.h"                 // dj_pack_sink
#include "../../cpp/modules/parse/cli/arg_parser/arg_parser.hpp"  // DArgParser, command,
                                                      // DCString, text, guard
#include "./c/cli_plugin.h"                           // dj_cli_plugin_host,
                                                      // the plugin calls
#include "./c/cli_program.h"                          // dj_cli_program,
                                                      // D_CLI_BUILD_INIT
#include "./cli_io.hpp"                               // stream
// re_std
#include "../../../re_std/cpp/cstdint/cstdint.hpp"        // re_std::uint32_t


namespace djinterp
{
namespace cli
{

// the argument parser's names, which a program makes its definitions
// with, carried by the kit's namespace, so that `kit::command` and
// `kit::program` are spelled alike
using namespace ::djinterp::parse::cli;

//==============================================================================
// 1.  VOCABULARY
//==============================================================================


// 1.1    Exit statuses and severities
//------------------------------------------------------------------------------
// 1.1.1
// exit_status
//   enum: the grammar's exit statuses (GRAMMAR.md, section 8): done, done
// and the answer is no, and trouble.
struct exit_status
{
    enum value
    {
        ok      = D_CLI_EXIT_OK,
        no      = D_CLI_EXIT_NO,
        trouble = D_CLI_EXIT_TROUBLE
    };
};

// 1.1.2
// severity
//   enum: the dj_cli_severity values: what a diagnostic names itself.
struct severity
{
    enum value
    {
        error   = D_CLI_SEVERITY_ERROR,
        warning = D_CLI_SEVERITY_WARNING,
        note    = D_CLI_SEVERITY_NOTE
    };
};

// 1.2    Program flags
//------------------------------------------------------------------------------
// 1.2.1
// program_flags
//   enum: the D_CLI_PROGRAM_* flags, combined with `|`: a program with
// commands; one that reads response files; one whose help keeps the
// argument parser's own layout; one that says an error without its word;
// one that takes a unique prefix of a long name for the name; and one whose
// commands that change a file write standard output unless given `-i`.
// djinterp's own programs set none of the last four.
struct program_flags
{
    enum value
    {
        commands        = D_CLI_PROGRAM_COMMANDS,
        response_files  = D_CLI_PROGRAM_RESPONSE_FILES,
        parser_help     = D_CLI_PROGRAM_PARSER_HELP,
        plain_errors    = D_CLI_PROGRAM_PLAIN_ERRORS,
        abbreviations   = D_CLI_PROGRAM_ABBREVIATIONS,
        edits_to_output = D_CLI_PROGRAM_EDITS_TO_OUTPUT
    };
};

// 1.2.2
// plugin_versions
//   enum: the dj_cli_plugin_versions values: what a program does with a
// module of another version of it -- the build's choice, refuse it, or load
// it with a warning.
struct plugin_versions
{
    enum value
    {
        build_default = D_CLI_PLUGIN_VERSIONS_DEFAULT,
        refuse        = D_CLI_PLUGIN_VERSIONS_REFUSE,
        warn          = D_CLI_PLUGIN_VERSIONS_WARN
    };
};


//==============================================================================
// 2.  THE PROGRAM
//==============================================================================


// 2.1    The class
//------------------------------------------------------------------------------
// 2.1.1
// program
//   class: a parser with the kit's conventions, and the commands' work.
// Every member that names a C call does what the call's contract says
// (cli_program.h, cli_plugin.h); kit() is the record those calls take. Like
// the parser it is, a program is neither copied nor moved.
class program
    : public DArgParser
{
public:
    // making and releasing

    /**
     * @brief Makes a program; d_arg_parser_init, then d_cli_program_init.
     *
     * @param[in] _name         the program's own name, as diagnostics begin
     *                          and usage lines show it.
     * @param[in] _version      its version, as `--version` prints it after
     *                          the name; empty for none.
     * @param[in] _description  what it does, as its help begins.
     * @param[in] _flags        program_flags values.
     * @post   made() says whether the kit's record was made; a program that
     *         was not ends every run with exit_status::trouble. The build is
     *         the one the constructor was compiled in.
     */
    program(
        DCString         _name,
        DCString         _version,
        DCString         _description = DCString(),
        re_std::uint32_t _flags = 0u
    ) D_NOEXCEPT;
    /**
     * @brief Makes a module's program from the host that runs it;
     *        d_arg_parser_init under the host's command, then
     *        d_cli_program_init_plugin.
     *
     * @param[in] _host         the host, as the module's run function was
     *                          given it.
     * @param[in] _description  what the module does, as its help begins.
     * @param[in] _flags        program_flags values.
     * @post   made() says whether the kit's record was made. The build is
     *         the module's, as the constructor was compiled in it.
     */
    explicit
    program(
        const ::dj_cli_plugin_host& _host,
        DCString                    _description = DCString(),
        re_std::uint32_t            _flags = 0u
    ) D_NOEXCEPT;
    ~program();

    // whether the program was made whole: the kit's record, its standard
    // options, and every command's work kept; one that was not ends every
    // run in trouble, and says why
    bool made() const D_NOEXCEPT;

    // commands: d_cli_program_add_command with the work a copy of the
    // function object given, or with none -- a command that only groups
    // subcommands; under the root, or under a command given. The command,
    // or a NULL handle when it could not be made. Making the copy throws
    // what its copy constructor or `new` throws.
    template<typename Function>
    cli::command add_command(
        DCString _name,
        DCString _description,
        Function _work
    );
    template<typename Function>
    cli::command add_command(
        cli::command _parent,
        DCString     _name,
        DCString     _description,
        Function     _work
    );
    cli::command add_command(
        DCString _name,
        DCString _description
    );
    cli::command add_command(
        cli::command _parent,
        DCString     _name,
        DCString     _description
    );

    // the root's own work, for a program of one command or a module:
    // d_cli_program_set_main; a failure -- memory alone -- leaves the root
    // with none
    template<typename Function>
    program& set_main(
        Function _work
    );

    // a command whose work is a module's: d_cli_program_add_plugin
    cli::command add_plugin(
        DCString _name,
        DCString _description
    );
    cli::command add_plugin(
        cli::command _parent,
        DCString     _name,
        DCString     _description
    );

    // where a run looks for modules first, and what it does with one of
    // another version: d_cli_program_set_plugin_path, which copies the list
    // -- empty for none, a want of memory leaving the setting as it was --
    // and d_cli_program_set_plugin_versions; each overrides the build's
    // knob for the program's runs
    status plugin_path(
        DCString _path
    ) D_NOEXCEPT;
    program& plugin_versions(
        cli::plugin_versions::value _versions
    ) D_NOEXCEPT;

    // running: d_cli_program_run, d_cli_program_parse and
    // d_cli_program_dispatch; reached() is the command the last parse
    // reached, the root when it reached none

    /**
     * @brief Runs the program on main's arguments: the parse, its ending, or
     *        the work of the command it reached; d_cli_program_run.
     *
     * @param[in] _argc  the argument count.
     * @param[in] _argv  the arguments, `_argv[0]` the program's path;
     *                   `char**` converts.
     * @return the status main returns.
     */
    int run(
        int                _argc,
        const char* const* _argv
    );
    /**
     * @brief Parses main's arguments, and ends a parse that called for an
     *        end; d_cli_program_parse.
     *
     * @param[in]  _argc         the argument count.
     * @param[in]  _argv         the arguments.
     * @param[out] _exit_status  receives the status to exit with when the
     *                           parse ended the run; untouched otherwise.
     * @return `true` when the program is to exit with `_exit_status`;
     *         `false` when it goes on.
     */
    bool parse_or_exit(
        int                _argc,
        const char* const* _argv,
        int&               _exit_status
    );
    int dispatch();
    cli::command reached() const D_NOEXCEPT;

    // output: where results and diagnostics go, the parser's help and
    // failure messages with them -- d_cli_program_set_output -- as sinks or
    // as streams, which must outlive their use; DArgParser's output(),
    // which would leave the kit's own, is hidden
    program& output(
        ::dj_pack_sink _out,
        ::dj_pack_sink _err
    ) D_NOEXCEPT;
    program& output(
        std::ostream& _out,
        std::ostream& _err
    ) D_NOEXCEPT;

    // the standard options' values, as the last parse left them:
    // d_cli_program_verbosity, d_cli_program_colour
    int verbosity() const D_NOEXCEPT;
    bool coloured(
        stream::value _stream
    ) const D_NOEXCEPT;

    /**
     * @brief A command's full name, as a user types it: `bitwisedom gen`;
     *        d_cli_command_path.
     *
     * @param[in] _command  the command; a NULL handle for the root.
     * @return the name; std::bad_alloc, as std::string throws it, when it
     *         cannot be made.
     */
    std::string path_of(
        cli::command _command
    ) const;

    // the build: a line of the program's own, `KEY: VALUE`, after how the
    // program was built in what `--version --verbose` says --
    // d_cli_program_add_build, a want of memory kept as made() says
    status add_build(
        DCString _key,
        DCString _value
    ) D_NOEXCEPT;

    // diagnostics: d_cli_report, d_cli_report_at and d_cli_note with the
    // message given whole -- lower case, no full stop; d_cli_usage_error,
    // for a command or the root, and d_cli_file_error, which return
    // exit_status::trouble for the work to return. None fails.
    void report(
        severity::value _severity,
        const text&     _message
    ) const D_NOEXCEPT;
    void report_at(
        severity::value _severity,
        DCString        _file,
        long            _line,
        long            _column,
        const text&     _message
    ) const D_NOEXCEPT;
    void note(
        int         _level,
        const text& _message
    ) const D_NOEXCEPT;
    int usage_error(
        cli::command _command,
        const text&  _message
    ) const D_NOEXCEPT;
    int usage_error(
        const text& _message
    ) const D_NOEXCEPT;
    int file_error(
        DCString _doing,
        DCString _path,
        int      _errnum
    ) const D_NOEXCEPT;

    // the kit's record, for the C calls
    ::dj_cli_program& kit() D_NOEXCEPT;
    const ::dj_cli_program& kit() const D_NOEXCEPT;

private:
    D_DELETED_FN(program(const program& _other))
    D_DELETED_FN(program& operator=(const program& _other))

    ::dj_cli_program m_program;
};


//==============================================================================
// 3.  INTERNALS
//==============================================================================


// 3.1    A command's work
//------------------------------------------------------------------------------
NS_INTERNAL

    // the parser's own helpers, which the kit's work and names run through:
    // its guard against exceptions, their words, a context's release, and
    // a buffer that becomes a std::string
    using ::djinterp::parse::cli::internal::exception_words;
    using ::djinterp::parse::cli::internal::guard;
    using ::djinterp::parse::cli::internal::release_context;
    using ::djinterp::parse::cli::internal::scratch;

    // 3.1.1
    // DWorkHolder
    //   class: a command's work as the program keeps it: the program, and a
    //   copy of the function object.
    template<typename Function>
    class DWorkHolder
    {
    public:
        DWorkHolder(
            program&        _program,
            const Function& _function
        );

        int run(
            ::dj_arg_command* _command
        );

    private:
        program* m_program;
        Function m_function;
    };

    // 3.1.2
    // DWorkCall
    //   class: the work work_function guards: the holder's function called
    //   with the program and the command.
    template<typename Function>
    class DWorkCall
    {
    public:
        typedef int result_type;

        DWorkCall(
            void*             _context,
            ::dj_arg_command* _command
        ) D_NOEXCEPT;

        int operator()() const;

    private:
        DWorkHolder<Function>* m_holder;
        ::dj_arg_command*       m_command;
    };

    // 3.1.3
    // DWorkFailure
    //   class: guard's handler for a command's work: what was thrown, said
    //   as the program's error, and trouble.
    class DWorkFailure
    {
    public:
        typedef int result_type;

        explicit
        DWorkFailure(
            const ::dj_cli_program* _program
        ) D_NOEXCEPT;

        int operator()(
            const std::exception* _exception,
            bool                  _out_of_memory
        ) const;

    private:
        const ::dj_cli_program* m_program;
    };

    // 3.1.4
    // work_function
    //   function: a command's work as dj_cli_command_fn: DWorkCall under the
    //   guard.
    template<typename Function>
    int work_function(
        ::dj_cli_program* _program,
        ::dj_arg_command* _command,
        void*             _context
    );

NS_END  // internal


//==============================================================================
// 4.  DEFINITIONS
//==============================================================================


// 4.1    Making and releasing
//------------------------------------------------------------------------------
// program::program / ~program
//   The parser first, as DArgParser makes it; then the kit's record over
// it, before any definition of the program's, and the build the
// constructor is compiled in. The record goes before the parser does,
// freeing the copies of the commands' work.
inline
program::program(
    DCString         _name,
    DCString         _version,
    DCString         _description,
    re_std::uint32_t _flags
) D_NOEXCEPT
    : DArgParser(_description,
                 _name),
      m_program()
{
    const ::dj_cli_build build = D_CLI_BUILD_INIT;

    (void)d_cli_program_init(&m_program,
                             this,
                             _name.get(),
                             _version.get(),
                             _flags);
    d_cli_program_set_build(&m_program,
                            &build);
}

inline
program::program(
    const ::dj_cli_plugin_host& _host,
    DCString                   _description,
    re_std::uint32_t           _flags
) D_NOEXCEPT
    : DArgParser(_description,
                 DCString(_host.command)),
      m_program()
{
    const ::dj_cli_build build = D_CLI_BUILD_INIT;

    (void)d_cli_program_init_plugin(&m_program,
                                    this,
                                    &_host,
                                    _flags);
    d_cli_program_set_build(&m_program,
                            &build);
}

inline
program::~program()
{
    d_cli_program_release(&m_program);
}

// program::made
//   A record made has its name, and a status that says nothing failed since;
// a failed one was released to nothing but why.
inline
bool program::made() const D_NOEXCEPT
{
    return ( (m_program.name != D_NULLPTR) &&
             (m_program.status == static_cast<int>(D_ARG_OK)) );
}

// 4.2    Commands
//------------------------------------------------------------------------------
// program::add_command / set_main / add_plugin
//   The C calls; the work a holder made with `new`, which the program frees
// with `delete` through release_context, or at once when the command could
// not be made.
template<typename Function>
inline
cli::command program::add_command(
    DCString _name,
    DCString _description,
    Function _work
)
{
    return add_command(cli::command(),
                       _name,
                       _description,
                       _work);
}

template<typename Function>
inline
cli::command program::add_command(
    cli::command _parent,
    DCString     _name,
    DCString     _description,
    Function     _work
)
{
    return cli::command(d_cli_program_add_command(
                            &m_program,
                            _parent.get(),
                            _name.get(),
                            _description.get(),
                            &internal::work_function<Function>,
                            new internal::DWorkHolder<Function>(*this,
                                                                _work),
                            &internal::release_context<
                                internal::DWorkHolder<Function> >));
}

inline
cli::command program::add_command(
    DCString _name,
    DCString _description
)
{
    return add_command(cli::command(),
                       _name,
                       _description);
}

inline
cli::command program::add_command(
    cli::command _parent,
    DCString     _name,
    DCString     _description
)
{
    return cli::command(d_cli_program_add_command(&m_program,
                                                  _parent.get(),
                                                  _name.get(),
                                                  _description.get(),
                                                  D_NULLPTR,
                                                  D_NULLPTR,
                                                  D_NULLPTR));
}

template<typename Function>
inline
program& program::set_main(
    Function _work
)
{
    (void)d_cli_program_set_main(
              &m_program,
              &internal::work_function<Function>,
              new internal::DWorkHolder<Function>(*this,
                                                  _work),
              &internal::release_context<internal::DWorkHolder<Function> >);

    return *this;
}

inline
cli::command program::add_plugin(
    DCString _name,
    DCString _description
)
{
    return add_plugin(cli::command(),
                      _name,
                      _description);
}

inline
cli::command program::add_plugin(
    cli::command _parent,
    DCString     _name,
    DCString     _description
)
{
    return cli::command(d_cli_program_add_plugin(&m_program,
                                                 _parent.get(),
                                                 _name.get(),
                                                 _description.get()));
}

// 4.3    Running
//------------------------------------------------------------------------------
// program::run / parse_or_exit / dispatch / reached
//   The C calls.
inline
int program::run(
    int                _argc,
    const char* const* _argv
)
{
    return d_cli_program_run(&m_program,
                             _argc,
                             _argv);
}

inline
bool program::parse_or_exit(
    int                _argc,
    const char* const* _argv,
    int&               _exit_status
)
{
    return d_cli_program_parse(&m_program,
                               _argc,
                               _argv,
                               &_exit_status);
}

inline
int program::dispatch()
{
    return d_cli_program_dispatch(&m_program);
}

inline
cli::command program::reached() const D_NOEXCEPT
{
    return cli::command(d_cli_program_command(&m_program));
}

// 4.4    Settings and queries
//------------------------------------------------------------------------------
// program::output / verbosity / coloured / path_of / add_build / kit
//   The C calls; the streams' sinks the face's stream_sink.
inline
program& program::output(
    ::dj_pack_sink _out,
    ::dj_pack_sink _err
) D_NOEXCEPT
{
    d_cli_program_set_output(&m_program,
                             _out,
                             _err);

    return *this;
}

inline
program& program::output(
    std::ostream& _out,
    std::ostream& _err
) D_NOEXCEPT
{
    return output(stream_sink(_out),
                  stream_sink(_err));
}

inline
int program::verbosity() const D_NOEXCEPT
{
    return d_cli_program_verbosity(&m_program);
}

inline
bool program::coloured(
    stream::value _stream
) const D_NOEXCEPT
{
    return d_cli_program_colour(&m_program,
                                static_cast<int>(_stream));
}

inline
std::string program::path_of(
    cli::command _command
) const
{
    internal::scratch name;

    (void)d_cli_command_path(
              (_command.get() != D_NULLPTR)
              ? _command.get()
              : &m_program.parser->command,
              name.get());

    return name.str();
}

inline
status program::add_build(
    DCString _key,
    DCString _value
) D_NOEXCEPT
{
    return status(d_cli_program_add_build(&m_program,
                                          _key.get(),
                                          _value.get()));
}

// program::plugin_path / plugin_versions
//   The C calls.
inline
status program::plugin_path(
    DCString _path
) D_NOEXCEPT
{
    return status(d_cli_program_set_plugin_path(&m_program,
                                                _path.get()));
}

inline
program& program::plugin_versions(
    cli::plugin_versions::value _versions
) D_NOEXCEPT
{
    d_cli_program_set_plugin_versions(&m_program,
                                      static_cast<int>(_versions));

    return *this;
}

inline
::dj_cli_program& program::kit() D_NOEXCEPT
{
    return m_program;
}

inline
const ::dj_cli_program& program::kit() const D_NOEXCEPT
{
    return m_program;
}

// 4.5    Diagnostics
//------------------------------------------------------------------------------
// program::report / report_at / note / usage_error / file_error
//   The C reports with the message as `%.*s`, so that a message holding `%`
// is written as it is, and one that holds a NUL is cut there.
inline
void program::report(
    severity::value _severity,
    const text&     _message
) const D_NOEXCEPT
{
    d_cli_report(&m_program,
                 static_cast<int>(_severity),
                 "%.*s",
                 static_cast<int>(_message.size()),
                 _message.data());

    return;
}

inline
void program::report_at(
    severity::value _severity,
    DCString        _file,
    long            _line,
    long            _column,
    const text&     _message
) const D_NOEXCEPT
{
    d_cli_report_at(&m_program,
                    static_cast<int>(_severity),
                    _file.get(),
                    _line,
                    _column,
                    "%.*s",
                    static_cast<int>(_message.size()),
                    _message.data());

    return;
}

inline
void program::note(
    int         _level,
    const text& _message
) const D_NOEXCEPT
{
    d_cli_note(&m_program,
               _level,
               "%.*s",
               static_cast<int>(_message.size()),
               _message.data());

    return;
}

inline
int program::usage_error(
    cli::command _command,
    const text&  _message
) const D_NOEXCEPT
{
    return d_cli_usage_error(&m_program,
                             _command.get(),
                             "%.*s",
                             static_cast<int>(_message.size()),
                             _message.data());
}

inline
int program::usage_error(
    const text& _message
) const D_NOEXCEPT
{
    return usage_error(cli::command(),
                       _message);
}

inline
int program::file_error(
    DCString _doing,
    DCString _path,
    int      _errnum
) const D_NOEXCEPT
{
    return d_cli_file_error(&m_program,
                            _doing.get(),
                            _path.get(),
                            _errnum);
}

// 4.6    Internals
//------------------------------------------------------------------------------
// internal::DWorkHolder / DWorkCall / DWorkFailure / work_function
//   The work is called through the holder, which the C program hands back
// as the context; what it returns -- an int, or anything that converts to
// one, an exit_status among them -- is the status. A throw is caught by the
// guard where C++ exceptions are on, and said as the program's error.
template<typename Function>
inline
internal::DWorkHolder<Function>::DWorkHolder(
    program&        _program,
    const Function& _function
)
    : m_program(&_program),
      m_function(_function)
{}

template<typename Function>
inline
int internal::DWorkHolder<Function>::run(
    ::dj_arg_command* _command
)
{
    return static_cast<int>(m_function(*m_program,
                                       cli::command(_command)));
}

template<typename Function>
inline
internal::DWorkCall<Function>::DWorkCall(
    void*            _context,
    ::dj_arg_command* _command
) D_NOEXCEPT
    : m_holder(static_cast<DWorkHolder<Function>*>(_context)),
      m_command(_command)
{}

template<typename Function>
inline
int internal::DWorkCall<Function>::operator()() const
{
    return m_holder->run(m_command);
}

inline
internal::DWorkFailure::DWorkFailure(
    const ::dj_cli_program* _program
) D_NOEXCEPT
    : m_program(_program)
{}

inline
int internal::DWorkFailure::operator()(
    const std::exception* _exception,
    bool                  _out_of_memory
) const
{
    const char* const words = internal::exception_words(_exception);

    d_cli_report(m_program,
                 static_cast<int>(D_CLI_SEVERITY_ERROR),
                 "%s",
                 (_out_of_memory)
                 ? "out of memory"
                 : (words != D_NULLPTR)
                   ? words
                   : "the command ended with an exception of no standard "
                     "type");

    return D_CLI_EXIT_TROUBLE;
}

template<typename Function>
inline
int internal::work_function(
    ::dj_cli_program* _program,
    ::dj_arg_command* _command,
    void*             _context
)
{
    return internal::guard(internal::DWorkCall<Function>(_context,
                                                         _command),
                           internal::DWorkFailure(_program));
}

}  // namespace cli
}  // namespace djinterp


#endif  // DJINTERP_TOOLS_CLI_CLI_PROGRAM_HPP
