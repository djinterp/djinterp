/*******************************************************************************
* bitwisedom [bitwisedom]                                                cli.hpp
*
* bitwisedom's command line.
*   `bitwisedom COMMAND ...` reads, writes and changes sheets without the
* window, as GRAMMAR.md (parse/cli/, section 10) sets out: gen, check, show,
* decode, eval, convert and examples read a sheet; new, import, add,
* remove, rename, move, set, get and op make or change one; gui opens the
* window, a module the program loads only then. The program is made with
* djinterp's command-line kit, so its options, its help, its diagnostics
* and its exit statuses are every djinterp program's.
*   `bitwisedom-gen` is the generator's command line from before: the same
* work as `bitwisedom gen`, under the options it always took, so that build
* scripts written for it run unchanged.
*   The window, which is built apart from djinterp (build/ui), takes its
* SHEET operands through window_sheet_of, by the commands' rules, so that
* `bitwisedom gui` refuses what they refuse, in their words.
*
* path:      /inc/djinterp/tools/bitwisedom/cli.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.10.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE PROGRAMS
    ------------
    1.  run_cli
    2.  run_gen_front
2.  THE WINDOW'S SHEETS
    -------------------
    1.  window_sheet
    2.  window_sheet_of
*/

#ifndef BITWISEDOM_CLI_HPP
#define BITWISEDOM_CLI_HPP 1

// FLOOR: below C++17 this file is empty, rather than an error.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <string>  // std::string
// djinterp
#include "djinterp/djinterp.hpp"  // framework root


namespace djinterp
{
namespace parse
{
namespace cli
{

class program;

}  // namespace cli
}  // namespace parse
}  // namespace djinterp


namespace bitwisedom
{


//==============================================================================
// 1.  THE PROGRAMS
//==============================================================================


// 1.1.1
// run_cli
/**
 * @brief Runs `bitwisedom` on main's arguments.
 *
 * @param[in] _argc  the argument count.
 * @param[in] _argv  the arguments, `_argv[0]` the program's path.
 * @return the status main returns: 0 done, 1 done and the answer is no, 2
 *         trouble (GRAMMAR.md, section 8).
 */
D_NODISCARD int  run_cli(int                _argc,
                         const char* const* _argv);

// 1.1.2
// run_gen_front
/**
 * @brief Runs `bitwisedom-gen` on main's arguments: `bitwisedom gen` under
 *        the options bitwisedom-gen always took, and its `--example`,
 *        `--list-examples` and `--save-example`.
 *
 * @param[in] _argc  the argument count.
 * @param[in] _argv  the arguments, `_argv[0]` the program's path.
 * @return the status main returns, as `bitwisedom gen`'s.
 */
D_NODISCARD int  run_gen_front(int                _argc,
                               const char* const* _argv);


//==============================================================================
// 2.  THE WINDOW'S SHEETS
//==============================================================================


// 2.1.1
// window_sheet
//   struct: a SHEET operand as the window opens one: a built-in example, by
// its place in example_names(), or a file -- a sheet, a bitfield, an export,
// or code to import -- by its path.
struct window_sheet
{
    int         example = -1;
    std::string path;
};

// 2.1.2
// window_sheet_of
/**
 * @brief Takes a SHEET operand for the window, by the rules the commands
 *        read one by: `example:NAME` names a built-in example; anything
 *        else is a file, which must be readable; and `-`, standard input,
 *        is refused, since a window cannot read it.
 *
 * @param[in]  _program  the window's program, which says what is wrong
 *                       with the operand as the commands say it.
 * @param[in]  _operand  the operand.
 * @param[out] _sheet    the example or the file.
 * @return `true` if the window can open it; `false`, having said why.
 */
D_NODISCARD bool  window_sheet_of(::djinterp::parse::cli::program& _program,
                                  const std::string&               _operand,
                                  window_sheet&                    _sheet);


}  // namespace bitwisedom


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // BITWISEDOM_CLI_HPP
