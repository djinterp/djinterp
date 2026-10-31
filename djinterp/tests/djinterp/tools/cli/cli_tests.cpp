/*******************************************************************************
* djinterp [tools]                                                 cli_tests.cpp
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
* path:      /tests/djinterp/tools/cli/cli_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.27
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
#include "../../../../inc/djinterp/cpp/djinterp.hpp"  // framework root
#include "../../../../inc/djinterp/c/filesys/file_dir.h"  // d_path_resolve
#include "../../../../inc/djinterp/env/tools/cli/env_cli.h"  // D_ENV_CLI_*
#include "../../../../inc/djinterp/tools/cli/cli.hpp"  // the kit's C++ face
#include "../../c/modules/parse/cli/arg_parser/arg_tests_files.h"  // scratch directories
#include "./c/cli_tests_support.h"  // checks, the environment, files
// re_std
#include "../../../../inc/re_std/cpp/cstdint/cstdint.hpp"  // re_std::uint32_t


namespace
{

namespace kit = djinterp::cli;


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

// DShowWork
//   class: `show`'s work, a functor: counts the run, writes what it was
// given to the program's output, and returns the status it was made with.
class DShowWork
{
public:
    DShowWork(state* _state,
              int    _status)
        : m_state(_state),
          m_status(_status)
    {}

    int operator()(
        kit::program& _program,
        kit::command  _command
    ) const
    {
        const std::string line = "show " + m_state->sheet +
                                 ((m_state->all) ? " --all" : "") + " at " +
                                 _program.path_of(_command) + "\n";
        const ::dj_pack_sink out = _program.kit().out;

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
 *        is a DShowWork, and `group`, with no work, holding `one`.
 *
 * @param[in,out] _program  the program, made.
 * @param[in,out] _state    what the work sees.
 * @param[in]     _status   what the work returns.
 * @return whether everything was made.
 */
bool make_commands(
    kit::program& _program,
    state&        _state,
    int           _status
)
{
    kit::command show  = _program.add_command("show",
                                              "list what a sheet holds",
                                              DShowWork(&_state,
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
                                DShowWork(&_state,
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
int run_line(
    kit::program& _program,
    const char*   _line,
    std::string&  _out,
    std::string&  _err
)
{
    dj_tests_cli_words words;

    // a line the case should not hold
    if (!d_tests_cli_split(_line,
                           &words))
    {
        return -1;
    }

    _program.output(kit::string_sink(_out),
                    kit::string_sink(_err));
    d_arg_formatter_set_line_width(
        &static_cast< ::dj_arg_parser&>(_program).formatter,
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
bool same(
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
bool starts(
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
void d_tests_cli_cpp_modules(
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
bool d_tests_cli_cpp_program()
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
bool d_tests_cli_cpp_halves()
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
        &static_cast< ::dj_arg_parser&>(program).formatter,
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

namespace
{

/**
 * @brief A program made with the flags that shape what it reads and
 *        writes: an error said without its word, at the root and at a
 *        place in a file, a warning with its; and help in the argument
 *        parser's own layout, asked for by a prefix of `--help`.
 *
 * @return whether it held.
 */
bool styled_reports()
{
    kit::program program("prog",
                         "1.2",
                         "a probe of the kit's face",
                         kit::program_flags::commands     |
                         kit::program_flags::plain_errors |
                         kit::program_flags::parser_help  |
                         kit::program_flags::abbreviations);
    state        seen   = { std::string(), false, 0 };
    std::string  out;
    std::string  err;
    const bool   made   = make_commands(program,
                                        seen,
                                        D_CLI_EXIT_OK);
    const int    status = run_line(program,
                                   "prog frob",
                                   out,
                                   err);

    program.report_at(kit::severity::error,
                      "a.txt",
                      2L,
                      5L,
                      "held");
    program.report(kit::severity::warning,
                   "kept");

    bool held = d_tests_cli_check( (made) &&
                                   (status == D_CLI_EXIT_TROUBLE) &&
                                   (out.empty()),
                                   "R02",
                                   "a failure without its word") &&
                same("R02",
                     "err",
                     "prog: 'frob' is not a command\n"
                     "Try 'prog --help' for more information.\n"
                     "a.txt:2:5: held\n"
                     "prog: warning: kept\n",
                     err);

    out.clear();
    err.clear();

    // the parser's layout begins with the description, as CLI11's does;
    // asked for by a prefix of `--help`
    held = d_tests_cli_check( (run_line(program,
                                        "prog --hel",
                                        out,
                                        err) == D_CLI_EXIT_OK) &&
                              (out.compare(0u,
                                           29u,
                                           "a probe of the kit's face\n\n\n"
                                           "p") == 0)             &&
                              (err.empty()),
                              "R03",
                              "help in the parser's own layout, asked for "
                              "by a prefix") && held;

    return held;
}

}  // namespace

/*
d_tests_cli_cpp_reports
  The reports with a message given whole: `%` written as it is, a NUL
ending it; the ending reports' trouble; and a program made to say its
errors without their word, to keep the parser's help, and to take a
prefix for a long name.
*/
bool d_tests_cli_cpp_reports()
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
                err) &&
           styled_reports();
}


//==============================================================================
// 2.  EXCEPTIONS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CPP_CATCH == 1)

namespace
{

// DThrownWork
//   class: a command's work that throws: a std::runtime_error with words,
// std::bad_alloc, or an exception of no standard type.
class DThrownWork
{
public:
    explicit DThrownWork(int _kind)
        : m_kind(_kind)
    {}

    int operator()(
        kit::program& _program,
        kit::command  _command
    ) const
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
bool thrown_case(
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
                              DThrownWork(_kind));

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
bool d_tests_cli_cpp_exceptions()
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
bool d_tests_cli_cpp_lambdas()
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
bool d_tests_cli_cpp_module_side()
{
    ::dj_cli_plugin_host host;
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

namespace
{

/**
 * @brief A named operand read through the face: the name in a string, an
 *        operand that is a file's name emptying it, and -- where a name may
 *        hold a colon -- the warning carried for a file of the operand's
 *        name, made here.
 *
 * @param[in,out] _files  the scratch directory the case runs in.
 * @return whether every check held.
 */
bool named_operands(
    dj_tests_arg_files& _files
)
{
    kit::program program("prog",
                         "1.2");
    std::string  out;
    std::string  err;
    std::string  name("untouched");

    program.output(kit::string_sink(out),
                   kit::string_sink(err));

    bool held = d_tests_cli_check(
                    (kit::named_operand(program,
                                        "example:uart_cr",
                                        "example",
                                        "the built-in example",
                                        name).ok())          &&
                    (name == "uart_cr")                      &&
                    (kit::named_operand(program,
                                        std::string("bytes.bin"),
                                        "example",
                                        "the built-in example",
                                        name).ok())          &&
                    (name.empty())                           &&
                    (err.empty()),
                    "F03",
                    "a named operand, and a file's name");

#if (D_TESTS_ARG_FILES_POSIX == 1)
    held = d_tests_cli_check(
               (d_tests_arg_files_file(&_files,
                                       "example:x",
                                       "a file\n"))          &&
               (kit::named_operand(program,
                                   "example:x",
                                   "example",
                                   "the built-in example",
                                   name).ok())               &&
               (name == "x"),
               "F04",
               "a named operand that is a file's name too") && held;
    held = same("F04",
                "err",
                "prog: warning: 'example:x' is the built-in example; the "
                "file is './example:x'\n",
                err) && held;
#else
    (void)_files;
#endif

    return held && (out.empty());
}

}  // namespace


/*
d_tests_cli_cpp_files
  The enumerations are the C values; `-` alone is standard; a file written
and read back with std::string, its bytes as they are; a file not there
leaves the string as it was; and a named operand, read through the face.
*/
bool d_tests_cli_cpp_files()
{
    dj_tests_arg_files files;
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
    held = named_operands(files) && held;

    (void)std::remove("bytes.bin");
    d_tests_arg_files_leave(&files);

    return held;
}


//==============================================================================
// 6.  EDITS
//==============================================================================

namespace
{

// DEditWork
//   class: `add`'s work, a functor: the file the edit's operand was bound
// to, read, with `three` after its last line, written where the edit sends
// it; and where that was, kept for the case.
class DEditWork
{
public:
    DEditWork(const kit::edit*         _edit,
              const std::string*       _file,
              kit::edit_place::value*  _place)
        : m_edit(_edit),
          m_file(_file),
          m_place(_place)
    {}

    int operator()(
        kit::program& _program,
        kit::command  _command
    ) const
    {
        std::string text;
        const int   failed = kit::read_file(*m_file,
                                            text);

        (void)_command;
        *m_place = m_edit->target(_program);

        return (failed != 0) ? _program.file_error("read",
                                                   *m_file,
                                                   failed)
                             : m_edit->write(_program,
                                             text + "three\n");
    }

private:
    const kit::edit*        m_edit;
    const std::string*      m_file;
    kit::edit_place::value* m_place;
};

/**
 * @brief Gives `add` its operand, bound to a string, and its options.
 *
 * @param[in,out] _program  the program.
 * @param[in]     _add      `add`.
 * @param[in,out] _edit     its edit.
 * @param[out]    _file     the string the operand is read into.
 * @return whether each was made.
 */
bool edit_made(
    kit::program& _program,
    kit::command  _add,
    kit::edit&    _edit,
    std::string&  _file
)
{
    return (_add.valid())                         &&
           (_edit.add_operand(_program,
                              _add,
                              "FILE",
                              "the file",
                              _file).valid())     &&
           (_edit.add_options(_program,
                              _add,
                              "the file").ok());
}

/**
 * @brief Runs `prog add` over `notes.txt`, holding `one`, by a rule, and
 *        compares what it came to: where the edit sent the result, what the
 *        program wrote, and what the file holds after.
 *
 * @param[in] _tag    the case's tag.
 * @param[in] _flags  the program's flags, beside program_flags::commands.
 * @param[in] _line   the command line.
 * @param[in] _place  where the result should go.
 * @param[in] _out    what the program should write to its output.
 * @param[in] _notes  what `notes.txt` should hold after.
 * @return whether everything was as the case says.
 */
bool edit_case(
    const char*            _tag,
    re_std::uint32_t       _flags,
    const char*            _line,
    kit::edit_place::value _place,
    const char*            _out,
    const char*            _notes
)
{
    kit::edit              edit;
    std::string            file;
    std::string            out;
    std::string            err;
    kit::edit_place::value place = kit::edit_place::back;
    kit::program           program("prog",
                                   "1.2",
                                   "",
                                   kit::program_flags::commands | _flags);
    kit::command           add   = program.add_command(
                                       "add",
                                       "add a line to a file",
                                       DEditWork(&edit,
                                                 &file,
                                                 &place));
    bool                   held  = d_tests_cli_check(
                                       (edit_made(program, add, edit, file)) &&
                                       (d_tests_cli_file_write("notes.txt",
                                                               "one\n",
                                                               4u)),
                                       _tag,
                                       "the program could not be made");

    held = (held) &&
           (d_tests_cli_check( (run_line(program, _line, out, err) ==
                                D_CLI_EXIT_OK)                         &&
                               (file == "notes.txt")                   &&
                               (place == _place),
                               _tag,
                               "the run, the operand or the place"));

    return (held)                       &&
           (same(_tag, "out", _out, out)) &&
           (same(_tag, "err", "", err))   &&
           (d_tests_cli_check(d_tests_cli_file_holds("notes.txt",
                                                     _notes,
                                                     std::strlen(_notes)),
                              _tag,
                              "notes.txt holds what it should"));
}

}  // namespace


/*
d_tests_cli_cpp_edits
  The places are the C values and a new edit holds nothing; then, in a
scratch directory, A writes the file back and B writes standard output,
and writes it back with `-i`; and a dry run, which A answers with nothing
written.
*/
bool d_tests_cli_cpp_edits()
{
    dj_tests_arg_files files;
    const kit::edit   none;
    bool              held = d_tests_cli_check(
                                 (static_cast<int>(kit::edit_place::back) ==
                                  static_cast<int>(D_CLI_EDIT_BACK))     &&
                                 (static_cast<int>(kit::edit_place::out) ==
                                  static_cast<int>(D_CLI_EDIT_OUT))      &&
                                 (static_cast<int>(kit::edit_place::output) ==
                                  static_cast<int>(D_CLI_EDIT_OUTPUT))   &&
                                 (static_cast<re_std::uint32_t>(
                                      kit::program_flags::edits_to_output) ==
                                  D_CLI_PROGRAM_EDITS_TO_OUTPUT)         &&
                                 (none.record().operand == D_NULLPTR)    &&
                                 (none.record().out == D_NULLPTR)        &&
                                 (none.record().dry_run == D_NULLPTR)    &&
                                 (none.record().in_place == D_NULLPTR)   &&
                                 (!none.dry_run()),
                                 "D01",
                                 "the vocabulary, and an edit made empty");

    // a scratch directory for the file
    if (!d_tests_cli_check(d_tests_arg_files_enter(&files),
                           "D02",
                           "the scratch directory could not be made"))
    {
        return false;
    }

    held = edit_case("D02",
                     0u,
                     "prog add notes.txt",
                     kit::edit_place::back,
                     "",
                     "one\nthree\n") && held;
    held = edit_case("D03",
                     kit::program_flags::edits_to_output,
                     "prog add notes.txt",
                     kit::edit_place::output,
                     "one\nthree\n",
                     "one\n") && held;
    held = edit_case("D04",
                     kit::program_flags::edits_to_output,
                     "prog add notes.txt -i",
                     kit::edit_place::back,
                     "",
                     "one\nthree\n") && held;
    held = edit_case("D05",
                     0u,
                     "prog add notes.txt -n",
                     kit::edit_place::back,
                     "",
                     "one\n") && held;

    (void)std::remove("notes.txt");
    d_tests_arg_files_leave(&files);

    return held;
}


//==============================================================================
// 7.  PLUGINS
//==============================================================================

/*
d_tests_cli_cpp_plugins
  The runner's file; a module found along the list; PROBE loaded and held,
then OTHER refused with why and nothing held; and the module unloaded by
the destructor.
*/
bool d_tests_cli_cpp_plugins()
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
                              (found == MODULES +
                                        D_ENV_CLI_PATH_SEPARATOR +
                                        "prog-probe" + suffix)        &&
                              (searched == MODULES + "\n"),
                              "G02",
                              "a module found along the list") && held;
    (void)d_tests_cli_setenv("PROG_PLUGIN_PATH",
                             D_NULLPTR);

#if (D_INTERNAL_CLI_PLUGINS == 1)
    kit::DPluginModule module;

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
// 8.  THE VERSION
//==============================================================================

namespace
{

/**
 * @brief Whether two of a build's strings are the same: both NULL, or the
 *        same text.
 *
 * @param[in] _one    one.
 * @param[in] _other  the other.
 * @return `true` if they are.
 */
bool same_part(
    const char* _one,
    const char* _other
)
{
    return ( (_one == D_NULLPTR) ||
             (_other == D_NULLPTR) )
           ? (_one == _other)
           : (std::strcmp(_one,
                          _other) == 0);
}

/**
 * @brief Whether a program carries the build this unit describes: every
 *        part the same, its language a C++ level.
 *
 * @param[in] _program  the program.
 * @return `true` if it does.
 */
bool built_here(
    const kit::program& _program
)
{
    static const ::dj_cli_build here  = D_CLI_BUILD_INIT;
    const ::dj_cli_build&       build = _program.kit().build;

    return ( (same_part(build.compiler,
                        here.compiler))                 &&
             (build.major == here.major)                &&
             (build.minor == here.minor)                &&
             (build.patch == here.patch)                &&
             (same_part(build.language,
                        here.language))                 &&
             (build.language != D_NULLPTR)              &&
             (std::strncmp(build.language,
                           "C++",
                           3u) == 0)                    &&
             (same_part(build.system,
                        here.system))                   &&
             (same_part(build.machine,
                        here.machine))                  &&
             (same_part(build.type,
                        here.type)) );
}

}  // namespace


/*
d_tests_cli_cpp_version
  A program and a module's program each carry the build their constructor
was compiled in, with nothing asked of them; add_build's line is said
before the kit's, a key it refuses refused with the C call's status; and a
module's program names its host's file and its own.
*/
bool d_tests_cli_cpp_version()
{
    kit::program        program("prog",
                                "1.2",
                                "a probe of the kit's face");
    ::dj_cli_plugin_host host;
    std::string         out;
    std::string         err;

    std::memset(&host,
                0,
                sizeof(host));
    host.abi        = D_CLI_PLUGIN_ABI;
    host.size       = static_cast<re_std::uint32_t>(sizeof(host));
    host.program    = "prog";
    host.version    = "1.2";
    host.command    = "prog gui";
    host.executable = "/x/prog";
    host.module     = "/x/prog-gui.so";

    bool held = d_tests_cli_check(
                    (program.made())                                      &&
                    (built_here(program))                                 &&
                    (program.add_build("window",
                                       "none").ok())                      &&
                    (program.add_build("a:b",
                                       "v").code() ==
                     static_cast<int>(D_ARG_ERROR_BAD_ARGUMENT))          &&
                    (run_line(program,
                              "prog --version -v",
                              out,
                              err) == D_CLI_EXIT_OK)                      &&
                    (starts(out,
                            "prog 1.2\n"))                                &&
                    (out.find("\nwindow: none\nkit: ") != std::string::npos) &&
                    (err.empty()),
                    "B01",
                    "a program's build, and a line of its own");
    kit::program module(host,
                        "a module's program");

    out.clear();
    err.clear();
    held = d_tests_cli_check( (module.made())                         &&
                              (built_here(module))                    &&
                              (module.kit().host == &host)            &&
                              (run_line(module,
                                        "prog-gui --version -v",
                                        out,
                                        err) == D_CLI_EXIT_OK)        &&
                              (starts(out,
                                      "prog 1.2\nprogram: /x/prog\n"
                                      "module: /x/prog-gui.so\n"))     &&
                              (err.empty()),
                              "B02",
                              "a module's program: its host's file, its "
                              "own, and its build") && held;

    return held;
}


//==============================================================================
// THE SUITE
//==============================================================================

// d_tests_cli_cpp
//   constant: the suite's sections, in the order they run.
const dj_tests_section d_tests_cli_cpp[] =
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
    { "cli_cpp_edits",       d_tests_cli_cpp_edits       },
    { "cli_cpp_plugins",     d_tests_cli_cpp_plugins     },
    { "cli_cpp_version",     d_tests_cli_cpp_version     }
};

// d_tests_cli_cpp_count
//   constant: how many sections d_tests_cli_cpp has.
const std::size_t d_tests_cli_cpp_count =
    sizeof(d_tests_cli_cpp) / sizeof(d_tests_cli_cpp[0]);
