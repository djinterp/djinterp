/*******************************************************************************
* djinterp [tools]                                                 cli_operand.h
*
* The command-line kit's operands that name something other than a file
* (GRAMMAR.md, R13).
*   An operand that names something else says so with a prefix of two
* lowercase letters or more and a colon -- `example:uart_cr`, the built-in
* example named `uart_cr` -- as git, docker and scp name what is not a
* file; a Windows drive, one letter, is never a prefix. A command reads
* each of its operands through d_cli_named_operand, which gives it the name
* after the prefix, or tells it the operand is a file's name.
*   A file may still have such a name, on a system whose names may hold a
* colon: the word then names the other thing, and the run says so, once,
* with the spelling that reaches the file (ruling 11) --
*
*     PROGRAM: warning: 'example:x' is the built-in example; the file is
*     './example:x'
*
* -- so that the file is never passed over in silence, and the thing named
* stays reachable by its name.
*
*
* path:      /inc/djinterp/tools/cli/c/cli_operand.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.25
*                                                            revised: 2026.10.25
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  NAMED OPERANDS
    --------------
    1.  Reading one
         1.  d_cli_named_operand
*/

#ifndef DJINTERP_TOOLS_CLI_C_CLI_OPERAND_H
#define DJINTERP_TOOLS_CLI_C_CLI_OPERAND_H 1

// djinterp
#include "../../../c/djinterp.h"  // framework root
#include "./cli_program.h"        // dj_cli_program


D_EXTERN_C_BEGIN

//==============================================================================
// 1.  NAMED OPERANDS
//==============================================================================


// 1.1    Reading one
//------------------------------------------------------------------------------
// 1.1.1
// d_cli_named_operand
/**
 * @brief Reads an operand as a named operand of a prefix (R13): the name
 *        after `PREFIX:`, or none, the operand being a file's name; and,
 *        where the operand is a named operand that is also the name of
 *        something there other than a directory, a warning --
 *        `PROGRAM: warning: 'WORD' is WHAT; the file is './WORD'`.
 *
 * @note The prefix matches as it is spelled, and the name is at least one
 *       byte: `example:` alone, `Example:x` and `./example:x` are files'
 *       names. A directory of the name is not warned of: no command reads
 *       one where it reads a named operand's thing. `-q` drops the warning,
 *       as every warning.
 *
 * @param[in]  _program  the program, which says the warning.
 * @param[in]  _word     the operand.
 * @param[in]  _prefix   the prefix, without its colon: `example`; two
 *                       lowercase ASCII letters or more, as R13 has it.
 * @param[in]  _what     what a named operand of the prefix is, as the
 *                       warning says it: "the built-in example".
 * @param[out] _name     receives the name, a pointer into `_word`, valid
 *                       while it is; NULL when the operand is a file's
 *                       name.
 * @return D_ARG_OK; D_ARG_ERROR_OUT_OF_MEMORY, said as `out of memory`,
 *         when there was no room to ask whether the file is there --
 *         `*_name` set all the same; D_ARG_ERROR_BAD_ARGUMENT for a NULL
 *         argument or a prefix R13 does not allow, unsaid, `*_name` NULL
 *         where it can be written.
 */
int d_cli_named_operand(
    const struct dj_cli_program* _program,
    const char*                  _word,
    const char*                  _prefix,
    const char*                  _what,
    const char**                 _name
);

D_EXTERN_C_END


#endif  // DJINTERP_TOOLS_CLI_C_CLI_OPERAND_H
