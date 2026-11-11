/*******************************************************************************
* djinterp [tools]                                               cli_operand.hpp
*
* The command-line kit's C++ face: operands that name something other than
* a file (GRAMMAR.md, R13).
*   The C call of cli_operand.h for C++, the name given in a std::string: a
* command reads an operand through it, and is told the name after the
* prefix -- `uart_cr` for `example:uart_cr` -- or that the operand is a
* file's name; where a file has the operand's name too, the run says so,
* once, and how to reach it.
*
*
* path:      /inc/djinterp/tools/cli/cli_operand.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.25
*                                                            revised: 2026.11.11
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  NAMED OPERANDS
    --------------
    1.  Reading one
         1.  named_operand
2.  DEFINITIONS
    -----------
    1.  Named operands
*/

#ifndef DJINTERP_TOOLS_CLI_OPERAND_HPP
#define DJINTERP_TOOLS_CLI_OPERAND_HPP 1

// std
#include <string>  // std::string
// djinterp
#include "../../cpp/djinterp.hpp"                         // framework root
#include "../../cpp/modules/parse/cli/arg_parser/arg_parser.hpp"  // DCString, status
#include "./c/cli_operand.h"                          // the C call
#include "./cli_program.hpp"                          // program


namespace djinterp
{
namespace cli
{

//==============================================================================
// 1.  NAMED OPERANDS
//==============================================================================


// 1.1    Reading one
//------------------------------------------------------------------------------
// 1.1.1
// named_operand
/**
 * @brief Reads an operand as a named operand of a prefix, warning where a
 *        file has its name too; d_cli_named_operand.
 *
 * @param[in]  _program  the program, which says the warning.
 * @param[in]  _word     the operand; a std::string converts.
 * @param[in]  _prefix   the prefix, without its colon: `example`.
 * @param[in]  _what     what a named operand of the prefix is, as the
 *                       warning says it: "the built-in example".
 * @param[out] _name     receives the name, replacing what it held; emptied
 *                       when the operand is a file's name, as a name is
 *                       never empty.
 * @return the C call's status: ok, or the want of memory, said; std::bad_alloc,
 *         as std::string throws it, when the name cannot be held.
 */
cli::status named_operand(
    const program& _program,
    DCString       _word,
    DCString       _prefix,
    DCString       _what,
    std::string&   _name
);


//==============================================================================
// 2.  DEFINITIONS
//==============================================================================


// 2.1    Named operands
//------------------------------------------------------------------------------
// named_operand
//   The C call on the program's record; the name copied from the word.
inline
cli::status named_operand(
    const program& _program,
    DCString       _word,
    DCString       _prefix,
    DCString       _what,
    std::string&   _name
)
{
    const char* name   = D_NULLPTR;
    const int   status = d_cli_named_operand(&_program.kit(),
                                             _word.get(),
                                             _prefix.get(),
                                             _what.get(),
                                             &name);

    _name.assign((name != D_NULLPTR) ? name : "");

    return cli::status(status);
}

}  // namespace cli
}  // namespace djinterp


#endif  // DJINTERP_TOOLS_CLI_OPERAND_HPP
