/*******************************************************************************
* djinterp [tools]                                                cli_settings.h
*
* The command-line kit's settings, on the knob-and-option pattern
* (docs/djinterp/option/knob_and_option.md): a knob decides a setting's
* default when the program is built, and a run may set it again, from the
* user's settings file, the environment or the command line, each from its
* own source, so that the order they arrive in cannot change the outcome.
*   The kit's own settings are its djin's, `cli`, in the program's registry
* (option_env_registry.h): `cli.plugin_strict_version`, and, where
* D_CFG_CLI_PLUGIN_VARIABLE is 1, `cli.plugin_search` and the program's
* `plugin_path` on the registry's root. Where it is 0, the environment is
* not to choose what code a program loads, so neither of those two is
* declared.
*   At a program's first parse the kit:
*     - registers its djin, after every djin a module registered before the
*       program ran;
*     - binds the environment's settings under `DJINTERP_`, then under the
*       program's own name -- upper-cased, each `-` an `_` -- so that
*       `BITWISEDOM_CLI_PLUGIN_SEARCH` wins over
*       `DJINTERP_CLI_PLUGIN_SEARCH`;
*     - reads the user's settings file, a statement a line, as
*       `cli.plugin_strict_version = false`: a statement that fails is said
*       by its line, as a warning, and the rest are read on;
*     - takes the command line's settings off it before the parser sees
*       them: `--NAME=VALUE`, `--NAME` for a bool's true and `--no-NAME`
*       for its false, NAME a full name, which always holds a '.'; and
*       `--set NAME=VALUE` for any name. Never past `--`, and never the
*       value an option before it takes, as `--level --x.y` gives `--level`
*       the word `--x.y`.
*   A setting the command line names badly is a usage error, exit 2: a
* name that names no setting, with the nearest that does; a value the
* setting does not take; one this build does not have.
*   The settings are the program's, as its registry is: read and written
* by one thread at a time, before the program starts its threads.
*
*
* path:      /inc/djinterp/tools/cli/c/cli_settings.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE KIT'S SETTINGS
    ------------------
    1.  The djin
         1.  d_cli_settings_djin
    2.  A run's settings
         1.  d_cli_settings_start
         2.  d_cli_settings_take
         3.  d_cli_settings_file
         4.  d_cli_settings_why
    3.  What the kit reads
         1.  d_cli_settings_plugin_search
         2.  d_cli_settings_plugin_path
         3.  d_cli_settings_plugin_strict_version
    4.  Testing
         1.  d_cli_settings_reset
*/

#ifndef DJINTERP_TOOLS_CLI_C_CLI_SETTINGS_H
#define DJINTERP_TOOLS_CLI_C_CLI_SETTINGS_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../c/djinterp.h"  // framework root
#include "./cli_program.h"        // dj_cli_program
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // int32_t


struct dj_option_env;

D_EXTERN_C_BEGIN

//==============================================================================
// 1.  THE KIT'S SETTINGS
//==============================================================================


// 1.1    The djin
//------------------------------------------------------------------------------
// 1.1.1
// d_cli_settings_djin
//   query: the kit's djin, `cli`, its variables declared; registered in the
// program's registry by the first d_cli_settings_start. NULL where the
// option core is not built, as without a 64-bit integer.
struct dj_option_env* d_cli_settings_djin(void);


// 1.2    A run's settings
//------------------------------------------------------------------------------
// 1.2.1
// d_cli_settings_start
/**
 * @brief Readies the run's settings, once a process: the kit's djin and
 *        the program's own variables in the program's registry, the
 *        environment's settings under `DJINTERP_` and the program's own
 *        prefix, and the user's settings file.
 *
 * @note d_cli_program_parse calls it before the first parse. A statement
 *       of the file that fails is said, `FILE:LINE: warning: MESSAGE`, and
 *       the file is read on from the next. A file that is not there is no
 *       failure.
 *
 * @param[in] _program  the program, by whose name the environment is read
 *                      and to whose error output a failure is said.
 * @return D_CLI_EXIT_OK; or D_CLI_EXIT_TROUBLE, said, when there was no
 *         memory to read the file with.
 */
int d_cli_settings_start(
    const struct dj_cli_program* _program
);

// 1.2.2
// d_cli_settings_take
/**
 * @brief Takes the settings off a command line, from COMMAND_LINE, and
 *        keeps every other argument, in order, for the parser.
 *
 * @param[in]  _program     the program, whose parser says which options
 *                          take the argument after them.
 * @param[in]  _argc        the argument count.
 * @param[in]  _argv        the arguments, `_argv[0]` the program's path.
 * @param[out] _kept        receives the arguments kept, `_kept[0]` the
 *                          program's path; room for `_argc` and a NULL.
 * @param[out] _kept_count  receives how many were kept.
 * @return D_CLI_EXIT_OK; or D_CLI_EXIT_TROUBLE, said as a usage error, when
 *         the command line names a setting badly.
 */
int d_cli_settings_take(
    const struct dj_cli_program* _program,
    int                          _argc,
    const char* const*           _argv,
    const char**                 _kept,
    int*                         _kept_count
);

// 1.2.3
// d_cli_settings_file
/**
 * @brief The user's settings file: `$XDG_CONFIG_HOME/djinterp/settings`,
 *        else `$HOME/.config/djinterp/settings`; on Windows
 *        `%APPDATA%\djinterp\settings`.
 *
 * @param[out] _buffer    receives the path and a NUL, when it fits.
 * @param[in]  _capacity  the buffer's size.
 * @return the path's length; 0 when the system names no home to put it in.
 *         A length not less than `_capacity` did not fit.
 */
size_t d_cli_settings_file(
    char*  _buffer,
    size_t _capacity
);

// 1.2.4
// d_cli_settings_why
/**
 * @brief Says what a binding's failure means, as the kit says it of a
 *        command line or a file: `no setting is named 'NAME'`, with `; did
 *        you mean 'NEAR'?` where a setting's name is near; `'VALUE' is not
 *        a value 'NAME' takes`; `'NAME' cannot be 'VALUE' in this build`.
 *
 * @param[in]  _status    the failure's status (dj_option_status).
 * @param[in]  _name      the setting's full name; need not end in a NUL.
 * @param[in]  _size      its length.
 * @param[in]  _value     the value given, or NULL for none.
 * @param[out] _buffer    receives the text, cut short to fit.
 * @param[in]  _capacity  the buffer's size.
 */
void d_cli_settings_why(
    int32_t     _status,
    const char* _name,
    size_t      _size,
    const char* _value,
    char*       _buffer,
    size_t      _capacity
);


// 1.3    What the kit reads
//------------------------------------------------------------------------------
// 1.3.1
// d_cli_settings_plugin_search
//   query: the directories a module is looked for in after the run's own
// and `plugin_path`'s: `cli.plugin_search`, or D_CFG_CLI_PLUGIN_SEARCH
// before the settings are started or where the variable is not declared.
const char* d_cli_settings_plugin_search(void);

// 1.3.2
// d_cli_settings_plugin_path
//   query: the program's `plugin_path`, read from PROGRAM_PLUGIN_PATH, the
// file or the command line; NULL when unset, or where
// D_CFG_CLI_PLUGIN_VARIABLE is 0.
const char* d_cli_settings_plugin_path(void);

// 1.3.3
// d_cli_settings_plugin_strict_version
/**
 * @brief Whether a module of another version of the program is refused.
 *
 * @param[in] _program_says  what the program set, a dj_cli_plugin_versions
 *                           (d_cli_program_set_plugin_versions): REFUSE,
 *                           WARN, or DEFAULT for nothing.
 * @return the setting from the file, the environment or the command line,
 *         where one was given, since each stands above the program's own
 *         say; else the program's; else D_CFG_CLI_PLUGIN_STRICT_VERSION.
 */
bool d_cli_settings_plugin_strict_version(
    int _program_says
);


// 1.4    Testing
//------------------------------------------------------------------------------
#if defined(D_TESTING)
#if (D_TESTING == 1)

// 1.4.1
// d_cli_settings_reset
//   testing: lets go of the run's settings -- the kit's djin taken out of
// the program's registry, its bindings and the root's gone -- so that the
// next d_cli_settings_start starts them again, as a new run would.
void d_cli_settings_reset(void);

#endif  // D_TESTING
#endif  // defined(D_TESTING)

D_EXTERN_C_END


#endif  // DJINTERP_TOOLS_CLI_C_CLI_SETTINGS_H
