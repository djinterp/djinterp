/*******************************************************************************
* djinterp [parse]                                            arg_completion.hpp
*
* The argument parser's C++ face: shell completion.
*   The C library's completion scripts (arg_completion.h) for a command
* handle: the shells as an enum, their names, and the script for bash, zsh,
* fish or PowerShell, written to a sink or made a std::string. The option
* that asks for a script is the parser's (arg_parser::set_completion_flag).
*   Everything here is compiled only with D_INTERNAL_ARG_PARSER_COMPLETION;
* without it the header declares nothing.
*
*
* path:      /inc/djinterp/parse/cli/arg_parser/arg_completion.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.17
*                                                            revised: 2026.10.17
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SHELLS
    ------
    1.  Identifiers
         1.  shell
    2.  Names
         1.  shell_from_name / shell_name
2.  SCRIPTS
    -------
    1.  Writing
         1.  completion_script
3.  DEFINITIONS
    -----------
    1.  Shells
    2.  Scripts
*/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_ARG_COMPLETION_HPP
#define DJINTERP_PARSE_CLI_ARG_PARSER_ARG_COMPLETION_HPP 1

// std
#include <string>     // std::string
// djinterp
#include "../../../djinterp.hpp"          // framework root
#include "../../../c/util/sink_common.h"  // d_pack_sink
#include "../../../config/parse/cli/arg_parser/cfg_arg_parser.h"  // knobs
#include "./arg_command.hpp"              // command, raise_out_of_memory
#include "./arg_common.hpp"               // status, text, string_sink
#include "./c/arg_completion.h"           // d_arg_completion_script

#if (D_INTERNAL_ARG_PARSER_COMPLETION == 1)


namespace djinterp
{
namespace parse
{
namespace cli
{

//==============================================================================
// 1.  SHELLS
//==============================================================================


// 1.1    Identifiers
//------------------------------------------------------------------------------
// 1.1.1
// shell
//   enum: the d_arg_shell values: the shells a script can be written for.
struct shell
{
    enum value
    {
        bash       = D_ARG_SHELL_BASH,
        zsh        = D_ARG_SHELL_ZSH,
        fish       = D_ARG_SHELL_FISH,
        powershell = D_ARG_SHELL_POWERSHELL
    };
};

// 1.2    Names
//------------------------------------------------------------------------------
// 1.2.1
// shell_from_name / shell_name
//   functions: "bash", "zsh", "fish", "powershell" or "pwsh", in any ASCII
// case, as a shell -- true, the shell written to `_shell` -- or false,
// `_shell` unchanged; and a shell's name as shell_from_name reads it.
bool        shell_from_name(const text&   _name,
                            shell::value& _shell) D_NOEXCEPT;
const char* shell_name(shell::value _shell) D_NOEXCEPT;


//==============================================================================
// 2.  SCRIPTS
//==============================================================================


// 2.1    Writing
//------------------------------------------------------------------------------
// 2.1.1
// completion_script
/**
 * @brief Writes a completion script for a command and its subcommands;
 *        d_arg_completion_script.
 *
 * @param[in] _command  the command, usually a parser's root; a parser
 *                      converts.
 * @param[in] _shell    the shell.
 * @param[in] _sink     where the script goes.
 * @param[in] _program  the name the shell runs the program by; empty for
 *                      the command's, less any directory and ".exe".
 * @return status_code::ok; file_error when the sink refused the text;
 *         out_of_memory when it could not be built; or another status, as
 *         d_arg_completion_script's.
 */
status      completion_script(const command& _command,
                              shell::value   _shell,
                              ::d_pack_sink  _sink,
                              const text&    _program = text());

// the script made a std::string: empty when it could not be written, and
// std::bad_alloc when memory ran out
std::string completion_script(const command& _command,
                              shell::value   _shell,
                              const text&    _program = text());


//==============================================================================
// 3.  DEFINITIONS
//==============================================================================


// 3.1    Shells
//------------------------------------------------------------------------------
// shell_from_name / shell_name
//   The C lookups, -1 read as no shell.
inline bool
shell_from_name(
    const text&   _name,
    shell::value& _shell
) D_NOEXCEPT
{
    const int found = d_arg_shell_from_name(_name);

    // a name no shell has
    if (found < 0)
    {
        return false;
    }

    _shell = static_cast<shell::value>(found);

    return true;
}

inline const char*
shell_name(
    shell::value _shell
) D_NOEXCEPT
{
    return d_arg_shell_name(static_cast<int>(_shell));
}

// 3.2    Scripts
//------------------------------------------------------------------------------
// completion_script
//   The C call, to the sink given or to a std::string's.
inline status
completion_script(
    const command& _command,
    shell::value   _shell,
    ::d_pack_sink  _sink,
    const text&    _program
)
{
    return status(d_arg_completion_script(_command.get(),
                                          static_cast<int>(_shell),
                                          _program,
                                          _sink));
}

inline std::string
completion_script(
    const command& _command,
    shell::value   _shell,
    const text&    _program
)
{
    std::string  written;
    const status result = completion_script(_command,
                                            _shell,
                                            string_sink(written),
                                            _program);

    internal::raise_out_of_memory(result.code());

    return result.ok() ? written : std::string();
}

}  // namespace cli
}  // namespace parse
}  // namespace djinterp


#endif  // D_INTERNAL_ARG_PARSER_COMPLETION

#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_ARG_COMPLETION_HPP
