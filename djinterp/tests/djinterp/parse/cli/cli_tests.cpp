/*******************************************************************************
* djinterp [parse]                                                 cli_tests.cpp
*
* The command-line kit's C++ suite, declared in cli_tests.hpp.
*   Each section makes its programs as a C++ program makes them -- a
* kit::program, its commands added with their work -- and checks what they
* came to through the C suites' support (cli_tests_support.h), compiled as
* C: what a program writes goes to strings through string_sink, or to
* streams, and a difference is said under the case's tag. The words of every
* message are the C suites' to hold; a case here compares them only where
* the face composes or carries them.
*
*
* path:      /tests/djinterp/parse/cli/cli_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.23
*******************************************************************************/
#include "./cli_tests.hpp"  // corresponding header
// std
#include <cerrno>     // ENOENT
#include <cstddef>    // std::size_t
#include <cstdio>     // std::remove
#include <cstring>    // std::strcmp, std::memset
#include <new>        // std::bad_alloc
#include <sstream>    // std::ostringstream
#include <stdexcept>  // std::runtime_error
#include <string>     // std::string
// djinterp
#include "../../../../inc/djinterp/djinterp.hpp"  // framework root
#include "../../../../inc/djinterp/c/filesys/file_dir.h"  // d_path_resolve
#include "../../../../inc/djinterp/env/parse/cli/env_cli.h"  // D_ENV_CLI_*
#include "../../../../inc/djinterp/parse/cli/cli.hpp"  // the kit's C++ face
#include "./arg_parser/c/arg_tests_files.h"  // scratch directories
#include "./c/cli_tests_support.h"  // checks, the environment, files
// re_std
#include "../../../../inc/re_std/cstdint/cstdint.hpp"  // re_std::uint32_t


namespace
{

namespace kit = djinterp::parse::cli;


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// MODULES
//   variable: the directory the probe modules are in, as the runner said.
std::string MODULES;

// state
//   struct: what a command's work saw, for the case to read: the sheet it
// was given, whether `--all` was, and how many times it ran.
struct state
{
    std::string sheet;
    bool        all;
    int         runs;
};

// show_work
//   class: `show`'s work, a functor: counts the run, writes what it was
// given to the program's output, and returns the status it was made with.
class show_work
{
public:
    show_work(state* _state,
              int    _status)
        : m_state(_state),
          m_status(_status)
    {}

    int operator()(kit::program& _program,
                   kit::command  _command) const
    {
        const std::string line = "show " + m_state->sheet +
                                 ((m_state->all) ? " --all" : "") + " at " +
                                 _program.path_of(_command) + "\n";
        const ::d_pack_sink out = _program.kit().out;

        ++m_state->runs;
        (void)out.write(out.context,
                        line.data(),
                        line.size());

        return m_status;
    }

private:
    state* m_state;
    int    m_status;
};

/**
 * @brief Makes COMMANDS: `prog 1.2`, with `show SHEET [--all]`, whose work
 *        is a show_work, and `group`, with no work, holding `one`.
 *
 * @param[in,out] _program  the program, made.
 * @param[in,out] _state    what the work sees.
 * @param[in]     _status   what the work returns.
 * @return whether everything was made.
 */
bool
make_commands(
    kit::program& _program,
    state&        _state,
    int           _status
)
{
    kit::command show  = _program.add_command("show",
                                              "list what a sheet holds",
                                              show_work(&_state,
                                                        _status));
    kit::command group = _program.add_command("group",
                                              "hold commands of its own");

    // made
    if ( (!show.valid()) ||
         (!group.valid()) )
    {
        return false;
    }

    show.add_option("SHEET",
                    _state.sheet,
                    "the sheet to list")
        .required();
    show.add_flag("--all",
                  _state.all,
                  "every row");

    return _program.add_command(group,
                                "one",
                                "the group's first command",
                                show_work(&_state,
                                          D_CLI_EXIT_OK)).valid();
}

/**
 * @brief Runs a program on a command line given as words, its output and
 *        error output in strings.
 *
 * @param[in,out] _program  the program.
 * @param[in]     _line     the command line.
 * @param[out]    _out      receives what it wrote to its output.
 * @param[out]    _err      receives what it wrote to its error output.
 * @return the status it exits with; -1 when the line does not split.
 */
int
run_line(
    kit::program& _program,
    const char*   _line,
    std::string&  _out,
    std::string&  _err
)
{
    d_tests_cli_words words;

    // a line the case should not hold
    if (!d_tests_cli_split(_line,
                           &words))
    {
        return -1;
    }

    _program.output(kit::string_sink(_out),
                    kit::string_sink(_err));
    d_arg_formatter_set_line_width(
        &static_cast< ::d_arg_parser&>(_program).formatter,
        80u);

    return _program.run(words.count,
                        words.items);
}

/**
 * @brief Compares a string with what it should be.
 *
 * @param[in] _tag       the case's tag.
 * @param[in] _what      what the string is.
 * @param[in] _expected  what it should be.
 * @param[in] _actual    what it is.
 * @return whether they are the same.
 */
bool
same(
    const char*        _tag,
    const char*        _what,
    const char*        _expected,
    const std::string& _actual
)
{
    return d_tests_cli_same(_tag,
                            _what,
                            _expected,
                            _actual.data(),
                            _actual.size());
}

/**
 * @brief Whether a string begins with another.
 *
 * @param[in] _text    the string.
 * @param[in] _prefix  what it must begin with.
 * @return `true` if it does.
 */
bool
starts(
    const std::string& _text,
    const std::string& _prefix
)
{
    return (_text.compare(0u,
                          _prefix.size(),
                          _prefix) == 0);
}

}  // namespace


/*
d_tests_cli_cpp_modules
  The directory given, made absolute while the working directory is still
the one it was given from, as the C suite makes it; or the runner's own,
its file's name cut off.
*/
void
d_tests_cli_cpp_modules(
    const char* _directory
)
{
    std::string executable;

    // the directory the runner was given
    if (_directory != D_NULLPTR)
    {
        char* const whole = d_path_resolve(_directory,
                                           D_NULLPTR,
                                           0u);

        MODULES = (whole != D_NULLPTR) ? whole : _directory;
        d_cli_free(whole);

        return;
    }

    // the runner's own
    if (kit::executable_path(D_NULLPTR,
                             executable) == 0)
    {
        MODULES = executable.substr(0u,
                                    executable.find_last_of("/\\"));
    }

    return;
}


//==============================================================================
// 1.  THE PROGRAM
//==============================================================================

/*
d_tests_cli_cpp_program
  A program made, its commands' work run and its status passed on; the
command the run reached and its path; output to streams; and the root's
help, the kit's.
*/
bool
d_tests_cli_cpp_program()
{
    kit::program program("prog",
                         "1.2",
                         "a probe of the kit's face",
                         kit::program_flags::commands);
    state        seen    = { std::string(), false, 0 };
    std::string  out;
    std::string  err;
    bool         held    = d_tests_cli_check(
                               (program.made())                       &&
                               (std::strcmp(program.kit().name,
                                            "prog") == 0)              &&
                               (make_commands(program,
                                              seen,
                                              D_CLI_EXIT_NO)),
                               "P01",
                               "the program, and its commands");
    const int    status  = run_line(program,
                                    "prog show s.json --all",
                                    out,
                                    err);

    held = d_tests_cli_check( (status == D_CLI_EXIT_NO)               &&
                              (seen.runs == 1)                        &&
                              (seen.sheet == "s.json")                &&
                              (seen.all)                              &&
                              (program.path_of(program.reached()) ==
                               "prog show")                           &&
                              (program.path_of(kit::command()) ==
                               "prog"),
                              "P02",
                              "the work run, its status passed on") && held;
    held = same("P02",
                "out",
                "show s.json --all at prog show\n",
                out) && held;
    held = same("P02",
                "err",
                "",
                err) && held;

    return held;
}

/*
d_tests_cli_cpp_halves
  parse_or_exit and dispatch, apart: a parse that goes on leaves the status
alone, and dispatch runs the work; one that asks for help ends the run
with 0 and the help on the output, a stream here.
*/
bool
d_tests_cli_cpp_halves()
{
    static const char* const help[] = { "prog", "--help", D_NULLPTR };
    static const char* const work[] = { "prog", "group", "one", D_NULLPTR };
    kit::program             program("prog",
                                     "1.2",
                                     "a probe of the kit's face",
                                     kit::program_flags::commands);
    state                    seen = { std::string(), false, 0 };
    std::ostringstream       out;
    std::ostringstream       err;
    int                      exit_status = -1;
    bool                     held = make_commands(program,
                                                  seen,
                                                  D_CLI_EXIT_OK);

    program.output(out,
                   err);
    d_arg_formatter_set_line_width(
        &static_cast< ::d_arg_parser&>(program).formatter,
        80u);
    held = d_tests_cli_check( (held)                                  &&
                              (!program.parse_or_exit(3,
                                                      work,
                                                      exit_status))   &&
                              (exit_status == -1)                     &&
                              (program.dispatch() == D_CLI_EXIT_OK)   &&
                              (seen.runs == 1)                        &&
                              (program.path_of(program.reached()) ==
                               "prog group one"),
                              "H01",
                              "a parse that goes on, then the work");
    out.str("");
    held = d_tests_cli_check( (program.parse_or_exit(2,
                                                     help,
                                                     exit_status))    &&
                              (exit_status == D_CLI_EXIT_OK)          &&
                              (starts(out.str(),
                                      "Usage: prog [OPTION]... COMMAND "
                                      "[ARGUMENT]...\n"))             &&
                              (err.str().empty()),
                              "H02",
                              "help asked: the run ends, help on the "
                              "output") && held;

    return held;
}

/*
d_tests_cli_cpp_reports
  The reports with a message given whole: `%` written as it is, a NUL
ending it; the ending reports' trouble.
*/
bool
d_tests_cli_cpp_reports()
{
    kit::program program("prog",
                         "1.2",
                         "a probe of the kit's face",
                         kit::program_flags::commands);
    state        seen = { std::string(), false, 0 };
    std::string  out;
    std::string  err;
    const bool   made = make_commands(program,
                                      seen,
                                      D_CLI_EXIT_OK);

    program.output(kit::string_sink(out),
                   kit::string_sink(err));
    program.report(kit::severity::warning,
                   "100% %s %d");
    program.report_at(kit::severity::error,
                      "a.txt",
                      2L,
                      5L,
                      std::string("held\0cut", 8u));
    program.note(0,
                 "a note at the default level");
    program.note(1,
                 "a note at -v, not written");

    const bool ended = ( (program.usage_error("no %s") ==
                          D_CLI_EXIT_TROUBLE)                         &&
                         (program.usage_error(program.add_command("x",
                                                                  "x"),
                                              "--x: bad") ==
                          D_CLI_EXIT_TROUBLE)                         &&
                         (program.file_error("read",
                                             "-",
                                             ENOENT) ==
                          D_CLI_EXIT_TROUBLE) );

    return d_tests_cli_check( (made) &&
                              (ended) &&
                              (out.empty()),
                              "R01",
                              "the reports, and the ending reports' "
                              "trouble") &&
           same("R01",
                "err",
                "prog: warning: 100% %s %d\n"
                "a.txt:2:5: error: held\n"
                "prog: note: a note at the default level\n"
                "prog: error: no %s\n"
                "Try 'prog --help' for more information.\n"
                "prog: error: x: --x: bad\n"
                "Try 'prog x --help' for more information.\n"
                "prog: error: cannot read standard input: No such file or "
                "directory\n",
                err);
}


//==============================================================================
// 2.  EXCEPTIONS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CPP_CATCH == 1)

namespace
{

// thrown_work
//   class: a command's work that throws: a std::runtime_error with words,
// std::bad_alloc, or an exception of no standard type.
class thrown_work
{
public:
    explicit thrown_work(int _kind)
        : m_kind(_kind)
    {}

    int operator()(kit::program& _program,
                   kit::command  _command) const
    {
        (void)_program;
        (void)_command;

        if (m_kind == 0)
        {
            throw std::runtime_error("the sheet broke");
        }

        if (m_kind == 1)
        {
            throw std::bad_alloc();
        }

        throw 42;
    }

private:
    int m_kind;
};

/**
 * @brief Runs a command whose work throws, and compares what the program
 *        said and the status it ended with.
 *
 * @param[in] _tag   the case's tag.
 * @param[in] _kind  what the work throws.
 * @param[in] _said  what the program must say.
 * @return whether it held.
 */
bool
thrown_case(
    const char* _tag,
    int         _kind,
    const char* _said
)
{
    kit::program program("prog",
                         "1.2",
                         "a probe of the kit's face",
                         kit::program_flags::commands);
    std::string  out;
    std::string  err;

    (void)program.add_command("throw",
                              "throw something",
                              thrown_work(_kind));

    const int status = run_line(program,
                                "prog throw",
                                out,
                                err);

    return d_tests_cli_check( (status == D_CLI_EXIT_TROUBLE) &&
                              (out.empty()),
                              _tag,
                              "an exception the work let out is trouble") &&
           same(_tag,
                "err",
                _said,
                err);
}

}  // namespace

/*
d_tests_cli_cpp_exceptions
  Each kind of exception a command's work lets out, said as the program's
error.
*/
bool
d_tests_cli_cpp_exceptions()
{
    bool held = thrown_case("T01",
                            0,
                            "prog: error: the sheet broke\n");

    held = thrown_case("T02",
                       1,
                       "prog: error: out of memory\n") && held;
    held = thrown_case("T03",
                       2,
                       "prog: error: the command ended with an exception "
                       "of no standard type\n") && held;

    return held;
}

#endif  // D_INTERNAL_ARG_PARSER_CPP_CATCH


//==============================================================================
// 3.  LAMBDAS
//==============================================================================

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/*
d_tests_cli_cpp_lambdas
  A command's work and the root's as lambdas, their captures kept with the
program and freed with it.
*/
bool
d_tests_cli_cpp_lambdas()
{
    std::string out;
    std::string err;
    int         ran   = 0;
    std::string named;
    int         status = -1;

    {
        kit::program program("prog",
                             "1.2",
                             "a probe of the kit's face");

        program.add_option("NAME",
                           named,
                           "a name");
        program.set_main(
            [&ran](kit::program& _program, kit::command)
            {
                ++ran;

                return (_program.verbosity() > 0) ? D_CLI_EXIT_NO
                                                  : D_CLI_EXIT_OK;
            });
        status = run_line(program,
                          "prog -v somebody",
                          out,
                          err);
    }

    return d_tests_cli_check( (status == D_CLI_EXIT_NO) &&
                              (ran == 1)                 &&
                              (named == "somebody")      &&
                              (out.empty())              &&
                              (err.empty()),
                              "L01",
                              "the root's work, a lambda");
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


//==============================================================================
// 4.  A MODULE'S PROGRAM
//==============================================================================

/*
d_tests_cli_cpp_module_side
  A program made from its host: named and versioned as the host, its usage
under the host's command, its verbosity counting from the host's and its
colour the host's until its own `--color`.
*/
bool
d_tests_cli_cpp_module_side()
{
    ::d_cli_plugin_host host;
    std::string         out;
    std::string         err;

    std::memset(&host,
                0,
                sizeof(host));
    host.abi       = D_CLI_PLUGIN_ABI;
    host.size      = static_cast<re_std::uint32_t>(sizeof(host));
    host.program   = "prog";
    host.version   = "1.2";
    host.command   = "prog gui";
    host.verbosity = 1;
    host.colour    = D_CLI_COLOUR_NEVER;

    kit::program module(host,
                        "a module's program");
    const bool   made   = ( (module.made())                          &&
                            (std::strcmp(module.kit().version,
                                         "1.2") == 0)                 &&
                            (module.verbosity() == 1) );
    const int    status = run_line(module,
                                   "prog-gui -v --help",
                                   out,
                                   err);

    return d_tests_cli_check( (made)                                  &&
                              (status == D_CLI_EXIT_OK)               &&
                              (module.verbosity() == 2)               &&
                              (!module.coloured(kit::stream::error))   &&
                              (starts(out,
                                      "Usage: prog gui [OPTION]...\n"
                                      "A module's program.\n"))       &&
                              (err.empty()),
                              "M01",
                              "a module's program, from its host");
}


//==============================================================================
// 5.  FILES
//==============================================================================

/*
d_tests_cli_cpp_files
  The enumerations are the C values; `-` alone is standard; a file written
and read back with std::string, its bytes as they are; a file not there
leaves the string as it was.
*/
bool
d_tests_cli_cpp_files()
{
    d_tests_arg_files files;
    const std::string bytes("a\0b\r\n\xff", 6u);
    std::string       read("untouched");
    bool              held = d_tests_cli_check(
                                 (static_cast<int>(kit::stream::error) ==
                                  static_cast<int>(D_CLI_STREAM_ERROR)) &&
                                 (static_cast<int>(kit::colour::automatic) ==
                                  static_cast<int>(D_CLI_COLOUR_AUTO)) &&
                                 (kit::is_standard("-"))              &&
                                 (!kit::is_standard("--"))            &&
                                 (kit::output_sink().write != D_NULLPTR) &&
                                 (kit::error_sink().write != D_NULLPTR)  &&
                                 (!kit::colour_wanted(kit::stream::output,
                                                      kit::colour::never)),
                                 "F01",
                                 "the vocabulary, and the streams");

    // a scratch directory for the files
    if (!d_tests_cli_check(d_tests_arg_files_enter(&files),
                           "F02",
                           "the scratch directory could not be made"))
    {
        return false;
    }

    held = d_tests_cli_check( (kit::read_file("missing.txt",
                                              read) == ENOENT)        &&
                              (read == "untouched")                   &&
                              (kit::write_file("bytes.bin",
                                               bytes) == 0)           &&
                              (kit::read_file("bytes.bin",
                                              read) == 0)             &&
                              (read == bytes),
                              "F02",
                              "a file written and read back") && held;

    (void)std::remove("bytes.bin");
    d_tests_arg_files_leave(&files);

    return held;
}


//==============================================================================
// 6.  PLUGINS
//==============================================================================

/*
d_tests_cli_cpp_plugins
  The runner's file; a module found along the list; PROBE loaded and held,
then OTHER refused with why and nothing held; and the module unloaded by
the destructor.
*/
bool
d_tests_cli_cpp_plugins()
{
    const std::string suffix(D_ENV_CLI_MODULE_SUFFIX);
    std::string       executable;
    std::string       found;
    std::string       searched;
    std::string       why;
    bool              held = d_tests_cli_check(
                                 (kit::executable_path(D_NULLPTR,
                                                       executable) == 0) &&
                                 (!executable.empty()),
                                 "G01",
                                 "the runner's own file");

    (void)d_tests_cli_setenv("PROG_PLUGIN_PATH",
                             MODULES.c_str());
    held = d_tests_cli_check( (kit::find_plugin("prog",
                                                "probe",
                                                "",
                                                found,
                                                searched).ok())       &&
                              (found == MODULES + "/prog-probe" + suffix) &&
                              (searched == MODULES + "\n"),
                              "G02",
                              "a module found along the list") && held;
    (void)d_tests_cli_setenv("PROG_PLUGIN_PATH",
                             D_NULLPTR);

#if (D_INTERNAL_CLI_PLUGINS == 1)
    kit::plugin_module module;

    held = d_tests_cli_check( (module.load(found,
                                           "prog",
                                           "1.2",
                                           why).ok())                 &&
                              (module.loaded())                       &&
                              (std::strcmp(module.plugin()->name,
                                           "probe") == 0)             &&
                              (found == module.path())                &&
                              (why.empty()),
                              "G03",
                              "PROBE loaded and held") && held;
    held = d_tests_cli_check( (!module.load(MODULES + "/prog-other" + suffix,
                                            "prog",
                                            "1.2",
                                            why).ok())                &&
                              (!module.loaded())                      &&
                              (std::string(module.path()).empty())    &&
                              (why == "it belongs to other, not prog"),
                              "G04",
                              "OTHER refused, and why") && held;
#endif

    return held;
}


//==============================================================================
// THE SUITE
//==============================================================================

// d_tests_cli_cpp
//   constant: the suite's sections, in the order they run.
const d_tests_section d_tests_cli_cpp[] =
{
    { "cli_cpp_program",     d_tests_cli_cpp_program     },
    { "cli_cpp_halves",      d_tests_cli_cpp_halves      },
    { "cli_cpp_reports",     d_tests_cli_cpp_reports     },
#if (D_INTERNAL_ARG_PARSER_CPP_CATCH == 1)
    { "cli_cpp_exceptions",  d_tests_cli_cpp_exceptions  },
#endif
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    { "cli_cpp_lambdas",     d_tests_cli_cpp_lambdas     },
#endif
    { "cli_cpp_module_side", d_tests_cli_cpp_module_side },
    { "cli_cpp_files",       d_tests_cli_cpp_files       },
    { "cli_cpp_plugins",     d_tests_cli_cpp_plugins     }
};

// d_tests_cli_cpp_count
//   constant: how many sections d_tests_cli_cpp has.
const std::size_t d_tests_cli_cpp_count =
    sizeof(d_tests_cli_cpp) / sizeof(d_tests_cli_cpp[0]);
