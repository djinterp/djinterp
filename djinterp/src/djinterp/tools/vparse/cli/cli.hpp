/*******************************************************************************
* vparse [cli]                                                           cli.hpp
*
* vparse's command line: the program `vparse`, as a function.
*   The command line is djinterp's command-line kit (tools/cli) over its
* argument parser, and each command is one use of the run the workbench's
* window shows:
*
*     check     read a grammar and judge it, as the session judges it
*     compile   build a grammar's program and write it out
*     match     run a grammar over inputs, and say whether each one matched
*     run       run a grammar over an input, and print the parts of the run
*     replay    run a saved run again, and print its parts
*     saved     list the runs saved under a folder
*     bench     run a grammar over many inputs, counting and timing each run
*     corpus    make inputs from a grammar
*     compare   run two grammars over the same inputs
*
* and the kit gives it what every djinterp program has (the kit's
* GRAMMAR.md, section 6): `-h`, `--version`, `-q`, `-v`, `--color`, and the
* `help` and `completion` commands; help in the kit's layout; and failures in
* the GNU shape, `vparse: error: MESSAGE`, with the kit's exit statuses.
*   Every command drives the model the window drives (run.hpp, store.hpp),
* so a part printed here is the part the window shows, row for row, and a run
* saved here is one the window loads. Nothing here needs uxoxo, Dear ImGui or
* a display.
*   run() is the program with its streams passed in, so that a suite runs the
* command line in-process and reads what it wrote; main() is a few lines
* around it (main.cpp).
*
*   LEVELS. The command line's floor is the model's, C++17: below it this
* file is empty rather than an error.
*
* path:      /src/djinterp/tools/vparse/cli/cli.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VOCABULARY
    ----------
    1.  exit_status
    2.  context
2.  THE PROGRAM
    -----------
    1.  run
    2.  version
*/

#ifndef VPARSE_CLI_CLI_HPP
#define VPARSE_CLI_CLI_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error, as
// the model it drives is.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstdio>  // std::FILE
// djinterp
#include "djinterp/c/util/sink_common.h"  // dj_pack_sink


// dj_cli_program
//   struct: the kit's program (tools/cli/c/cli_program.h), which a context
// points to and its users include.
struct dj_cli_program;


namespace vparse
{
namespace cli
{


//==============================================================================
// 1.  VOCABULARY
//==============================================================================


// 1.1    exit_status
//------------------------------------------------------------------------------
// 1.1.1
// exit_status
//   struct: the statuses a run of the command line ends with, as constants:
// the kit's three (GRAMMAR.md, section 8), as vparse's commands answer.
// NO_MATCH and NOT_ACCEPTED are the kit's no, each the answer of the
// commands it names. REFUSED is the kit's trouble, D_CLI_EXIT_TROUBLE: a
// grammar refused, for every command but check, whose answer it is, as a
// command line the parser refuses is, or a file that cannot be read or
// written; every one of them is said on the error stream. Help, a version
// and a completion script end with PASSED.
struct exit_status
{
    enum value
    {
        PASSED       = 0,  // the grammar was accepted, and every input
                           // matched
        NO_MATCH     = 1,  // an input did not match, or two programs
                           // differed on one
        NOT_ACCEPTED = 1,  // check's grammar was refused
        REFUSED      = 2   // the grammar was refused, or any other trouble
    };
};

// 1.2    context
//------------------------------------------------------------------------------
// 1.2.1
// context
//   struct: what a run of the command line reads and writes besides the files
// it is given: the stream `-` reads, the sink results go to, and the sink
// diagnostics go to. `program` is the kit's program the run is, which says
// every diagnostic and knows whether `-q` was given: run() sets it in the
// context its commands are handed, and a caller's is not read.
struct context
{
    std::FILE*                  in      = nullptr;
    struct dj_pack_sink          out     = { nullptr, nullptr };
    struct dj_pack_sink          err     = { nullptr, nullptr };
    const struct dj_cli_program* program = nullptr;
};


//==============================================================================
// 2.  THE PROGRAM
//==============================================================================


// 2.1    run
//------------------------------------------------------------------------------
// 2.1.1
// run
/**
 * @brief Runs the vparse command line: parses the arguments, does what they
 *        ask, and says how it went.
 *
 * @note Help, a version, a completion script and every failure of the
 *       command line are written as the kit writes them: help and results to
 *       `_context.out`, diagnostics to `_context.err`. A command's output is
 *       written at once, when it is complete. On Windows main's own
 *       arguments are read again from the process's command line, in UTF-8,
 *       as the kit reads them; arguments a suite makes up are parsed as
 *       given.
 *
 * @param[in] _argc     the argument count, the program's name included.
 * @param[in] _argv     the arguments; `_argv[0]` is the program's name.
 * @param[in] _context  what the run reads and writes; its `program` is not
 *                      read.
 * @return the exit status, an exit_status value; std::bad_alloc, when the
 *         command line cannot be made -- the work of a command, once it
 *         runs, is guarded, a want of memory said and REFUSED.
 */
int run(
    int                _argc,
    const char* const* _argv,
    const context&     _context
);

// 2.2    version
//------------------------------------------------------------------------------
// 2.2.1
// version
//   function: what `--version` prints, "vparse <major>.<minor>.<patch>",
// the build's version, on its first line; never NULL.
const char* version();

}  // namespace cli
}  // namespace vparse

#endif  // floor, for now

#endif  // VPARSE_CLI_CLI_HPP
