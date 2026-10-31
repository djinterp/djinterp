/*******************************************************************************
* vparse [cli]                                                          main.cpp
*
* The program `vparse`: vparse's command line, on the process's own streams.
*   Everything the program does is cli::run(); main() hands it the standard
* input and the kit's standard sinks -- which write a Windows console
* through its wide interface, flush every write and so meet a full disk
* there -- and main's own arguments, and says what nothing below it can: a
* failure that escaped before a command ran, which the kit's guard then did
* not catch.
*   On Windows the standard input is read in binary mode, as every file the
* command line names is, so that a grammar or an input piped in is run byte
* for byte, its line endings included.
*
*   vparse [OPTION]... COMMAND [ARGUMENT]...
*     check     read a grammar and judge it, as the session judges it
*     compile   build a grammar's program and write it out
*     match     run a grammar over each input, and say whether it matched
*     run       run a grammar over an input, and print the parts of the run
*     replay    run a saved run again, and print its parts
*     saved     list the runs saved under a folder
*     bench     run a grammar over many inputs, counting and timing each run
*     corpus    make inputs from a grammar
*     compare   run two grammars over the same inputs
*     help, completion   the kit's
*
*   LEVELS. The command line's floor is the model's, C++17; below it this file
* is empty rather than an error.
*
* path:      /src/djinterp/tools/vparse/cli/main.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.25
*******************************************************************************/
#include "./cli.hpp"  // run, context
#if D_ENV_LANG_IS_CPP17_OR_HIGHER  // the floor the header has

// std
#include <cstdio>     // std::fprintf, stdin, stderr
#include <exception>  // std::exception
#include <new>        // std::bad_alloc
// djinterp
#include "djinterp/tools/cli/c/cli_io.h"       // d_cli_sink_output,
                                               // d_cli_sink_error
#include "djinterp/tools/cli/c/cli_program.h"  // D_CLI_EXIT_TROUBLE, and
                                               // through the parser,
                                               // D_ENV_ARG_PARSER_CPP_*
#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    // windows
    #include <fcntl.h>  // _O_BINARY
    #include <io.h>     // _setmode, _fileno
#endif  // windows


namespace
{

#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)

/**
 * @brief Puts the standard input in binary mode, as the files the command
 *        line names are read.
 *
 * Windows' C runtime opens it in text mode, which turns CR LF into LF and
 * stops at a Ctrl-Z; a grammar piped in would then not be the grammar.
 */
void read_standard_input_as_bytes()
{
    (void)_setmode(_fileno(stdin),
                   _O_BINARY);

    return;
}

#else

/**
 * @brief Leaves the standard input as it is: elsewhere it is bytes already.
 */
void read_standard_input_as_bytes()
{
    return;
}

#endif  // windows

/**
 * @brief Runs the command line on the process's streams.
 *
 * @param[in] _argc  main's argument count.
 * @param[in] _argv  main's arguments.
 * @return the exit status.
 */
int run_on_process(
    int    _argc,
    char** _argv
)
{
    vparse::cli::context streams;

    streams.in  = stdin;
    streams.out = d_cli_sink_output();
    streams.err = d_cli_sink_error();

    read_standard_input_as_bytes();

    return vparse::cli::run(_argc,
                            _argv,
                            streams);
}

#if (D_ENV_ARG_PARSER_CPP_EXCEPTIONS == 1)

/**
 * @brief Runs the command line, catching what escapes it.
 *
 * @param[in] _argc  main's argument count.
 * @param[in] _argv  main's arguments.
 * @return run_on_process's status; D_CLI_EXIT_TROUBLE, after an error in
 *         the kit's shape on the standard error, for an exception.
 *
 * Nothing below throws by design, and a command's work runs behind the
 * kit's guard; a command line that cannot be made for want of memory ends
 * here, said as the kit says every want of memory, rather than with
 * std::terminate.
 */
int guarded(
    int    _argc,
    char** _argv
)
{
    try
    {
        return run_on_process(_argc,
                              _argv);
    }
    catch (const std::bad_alloc&)
    {
        std::fprintf(stderr,
                     "vparse: error: out of memory\n");
    }
    catch (const std::exception& failure)
    {
        std::fprintf(stderr,
                     "vparse: error: %s\n",
                     failure.what());
    }

    return D_CLI_EXIT_TROUBLE;
}

#else

/**
 * @brief Runs the command line: without exceptions there is nothing to
 *        catch.
 *
 * @param[in] _argc  main's argument count.
 * @param[in] _argv  main's arguments.
 * @return run_on_process's status.
 */
int guarded(
    int    _argc,
    char** _argv
)
{
    return run_on_process(_argc,
                          _argv);
}

#endif  // D_ENV_ARG_PARSER_CPP_EXCEPTIONS

}  // anonymous namespace

/*
main
  Everything is run_on_process()'s, under guarded().
*/
int main(
    int    _argc,
    char** _argv
)
{
    return guarded(_argc,
                   _argv);
}

#endif  // floor, for now
