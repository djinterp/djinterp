/*******************************************************************************
* djinterp [parse]                                              arg_completion.h
*
* Shell completion: scripts that teach bash, zsh, fish and PowerShell a
* program's subcommands and options.
*   d_arg_completion_script writes a script, generated from the command tree
* as it is defined, that completes subcommand names and aliases, option
* names, and option values: the values a validator lists (an is_member's
* set, a transformer's keys -- see d_arg_validator_each_choice), directories
* or files for a path validator, nothing for a number or a boolean, and files
* otherwise. What help hides -- an option or subcommand in the empty group, a
* disabled subcommand -- the script leaves out.
*   A program offers the script through an option made by
* d_arg_parser_set_completion_flag: `prog --completion bash` ends the parse
* with D_ARG_CALL_FOR_COMPLETION, d_arg_parser_exit writes the script, and
* the shell loads it (`source <(prog --completion bash)` in bash).
*   Everything here is compiled only with D_INTERNAL_ARG_PARSER_COMPLETION;
* without it the header declares nothing.
*
*
* path:      /inc/djinterp/parse/cli/arg_parser/c/arg_completion.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.11
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SHELLS
    ------
    1.  Identifiers
         1.  d_arg_shell
    2.  Names
         1.  d_arg_shell_from_name / d_arg_shell_name
2.  SCRIPTS
    -------
    1.  Writing
         1.  d_arg_completion_script
    2.  The completion flag
         1.  d_arg_parser_set_completion_flag
*/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_COMPLETION_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_COMPLETION_H 1

// djinterp
#include "../../../../c/djinterp.h"          // framework root
#include "../../../../c/util/sink_common.h"  // d_pack_text, d_pack_sink
#include "./arg_common.h"                    // knobs, statuses, records

#if (D_INTERNAL_ARG_PARSER_COMPLETION == 1)


//==============================================================================
// 1.  SHELLS
//==============================================================================


// 1.1    Identifiers
//------------------------------------------------------------------------------
// 1.1.1
// d_arg_shell
//   enum: the shells a completion script can be written for.
enum d_arg_shell
{
    D_ARG_SHELL_BASH       = 0,
    D_ARG_SHELL_ZSH        = 1,
    D_ARG_SHELL_FISH       = 2,
    D_ARG_SHELL_POWERSHELL = 3,
    D_ARG_SHELL_COUNT      = 4
};


D_EXTERN_C_BEGIN

// 1.2    Names
//------------------------------------------------------------------------------
// 1.2.1
// d_arg_shell_from_name / d_arg_shell_name
//   lookups: from_name reads "bash", "zsh", "fish", "powershell" or "pwsh",
// without regard to ASCII case, as a d_arg_shell value, and anything else as
// -1; name gives a shell's name as from_name reads it ("powershell" for
// PowerShell), and "" for anything else. Neither allocates nor fails.
int         d_arg_shell_from_name(struct d_pack_text _name);
const char* d_arg_shell_name(int _shell);


//==============================================================================
// 2.  SCRIPTS
//==============================================================================


// 2.1    Writing
//------------------------------------------------------------------------------
// 2.1.1
// d_arg_completion_script
/**
 * @brief Writes a completion script for a command and its subcommands.
 *
 * @note The script completes the command as `_program`: the subcommands, as
 *       far down the command line as they are typed; at each, its options
 *       and those its option groups hold, and, with fallthrough, its
 *       parent's; and the values described above. Names are matched as
 *       typed, whatever the commands' ignore-case settings.
 *
 * @param[in] _command  the command, usually a parser's root.
 * @param[in] _shell    a d_arg_shell value.
 * @param[in] _program  the name the shell runs the program by; empty for
 *                      the last component of the command's name, less any
 *                      ".exe".
 * @param[in] _sink     where the script goes.
 * @return D_ARG_OK when written; D_ARG_ERROR_FILE when the sink refused the
 *         text; D_ARG_ERROR_OUT_OF_MEMORY when it could not be built; the
 *         status of a validator's listing that failed; or
 *         D_ARG_ERROR_BAD_ARGUMENT for a NULL command, an unknown shell, or
 *         no name to complete.
 */
int         d_arg_completion_script(const struct d_arg_command* _command,
                                    int                         _shell,
                                    struct d_pack_text          _program,
                                    struct d_pack_sink          _sink);

// 2.2    The completion flag
//------------------------------------------------------------------------------
// 2.2.1
// d_arg_parser_set_completion_flag
/**
 * @brief Adds to a parser's root the option that asks for a completion
 *        script.
 *
 * @note The option takes one value, a shell's name as d_arg_shell_from_name
 *       reads it, is not configurable, and is listed in help like any
 *       other. Given, it ends the parse before the requirements are checked
 *       -- as the help flag does -- with D_ARG_CALL_FOR_COMPLETION, and
 *       d_arg_parser_exit writes the script for the program as `argv[0]`
 *       names it. Replaces the option made before, if any.
 *
 * @param[in,out] _parser       the parser.
 * @param[in]     _names        the option's names, as
 *                              d_arg_command_add_option takes them; NULL or
 *                              "" removes the option.
 * @param[in]     _description  the help text, copied; NULL for "Print a
 *                              completion script for a shell and exit".
 * @return the option, owned by the root; NULL when removed, or on failure,
 *         as d_arg_command_add_option.
 */
struct d_arg_option*
            d_arg_parser_set_completion_flag(struct d_arg_parser* _parser,
                                             const char*          _names,
                                             const char*          _description);

D_EXTERN_C_END


#endif  // D_INTERNAL_ARG_PARSER_COMPLETION

#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_COMPLETION_H
