/*******************************************************************************
* djinterp [parse]                                              arg_completion.h
*
* Shell completion: scripts that teach bash, zsh, fish and PowerShell a
* program's subcommands and options.
*   d_arg_completion_script writes a script, generated from the command tree
* as it is defined, that completes subcommand names and aliases, option
* names, and the values of options and positionals: the values a validator
* lists (an is_member's set, a transformer's keys -- see
* d_arg_validator_each_choice), directories or files for a path validator,
* nothing for a number, a truth value or a character -- a validator that
* says so (D_ARG_VALIDATOR_NUMBER) or a bound type -- and files otherwise.
* What help hides -- an option or subcommand in the empty group, a disabled
* subcommand or option group -- the script leaves out, but for a hidden
* positional, which it counts, completing nothing for it.
*   The script holds the tree as data and walks the words before the cursor
* as the parse takes them: subcommands, with subcommand fallthrough and the
* most subcommands a command takes; options found by fallthrough; the values
* an option takes after its name, after `=`, `:` (Windows style) or a short
* name, and in a cluster of short flags; positionals in order; `--` and `++`.
* Names are matched as typed, whatever the commands' ignore-case,
* ignore-underscore and prefix-matching settings, and dotted subcommand
* names, non-standard short names and the parse's rules for positionals at
* the end are not followed. Bash runs the script from 3.2 on; zsh needs
* compinit loaded first. PowerShell offers its own file completion for an
* empty word where the script has nothing to offer, as it does for every
* native command, and the script gives back a word nothing fits as it
* stands, so that PowerShell offers no files in its place.
*   A program offers the script through an option made by
* d_arg_parser_set_completion_flag: `prog --completion bash` ends the parse
* with D_ARG_CALL_FOR_COMPLETION, d_arg_parser_exit writes the script, and
* the shell loads it (`source <(prog --completion bash)` in bash and zsh,
* `prog --completion fish | source` in fish, and `prog --completion
* powershell | Out-String | Invoke-Expression` in PowerShell).
*   Everything here is compiled only with D_INTERNAL_ARG_PARSER_COMPLETION;
* without it the header declares nothing.
*
*
* path:      /inc/djinterp/parse/cli/arg_parser/c/arg_completion.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.13
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
 * @note The script completes the command as `_program`, defining shell
 *       functions and variables named after it -- `_djarg_<name>` and
 *       `__djarg_<name>_*`, the name's bytes other than ASCII letters and
 *       digits escaped -- so that scripts for several programs live side by
 *       side; PowerShell's keeps its own in a closure. What it completes is
 *       described above. A subcommand's options include those its option
 *       groups hold, and, with fallthrough, its parent's.
 *
 * @param[in] _command  the command, usually a parser's root.
 * @param[in] _shell    a d_arg_shell value.
 * @param[in] _program  the name the shell runs the program by; empty for
 *                      the last component of the command's name -- after
 *                      its last `/` or `\` -- less any ".exe".
 * @param[in] _sink     where the script goes, written at once.
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
 *       reads it, shown as SHELL in help and offered by the script itself;
 *       another is rejected with "<value> not in
 *       {bash,zsh,fish,powershell,pwsh}". It is not configurable, and is
 *       called back first, as the help flag is: given, it ends the parse
 *       before the configuration, the environment and the requirements are
 *       read -- a help or version flag given with it winning -- with
 *       D_ARG_CALL_FOR_COMPLETION, the script the error's message, and
 *       d_arg_parser_exit writes it to the parser's output. The script is
 *       for the program as `argv[0]` names it -- its last component, less
 *       any ".exe" -- or, after a parse given no program name, as the root
 *       is named. Replaces the option made before, if any, wherever it was
 *       moved.
 *
 * @param[in,out] _parser       the parser.
 * @param[in]     _names        the option's names, as
 *                              d_arg_command_add_option takes them; NULL or
 *                              "" removes the option.
 * @param[in]     _description  the help text, copied; NULL for "Print a
 *                              completion script for a shell (bash, zsh,
 *                              fish or powershell) and exit".
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
