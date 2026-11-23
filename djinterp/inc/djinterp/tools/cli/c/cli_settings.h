/*******************************************************************************
* djinterp [c][tools]                                             cli_settings.h
*
* The command-line kit's settings, on the knob-and-option pattern
* (docs/djinterp/option/knob_and_option.md): a knob decides a setting's
* default when the program is built, and a run may set it again, from the
* user's settings file, the environment or the command line, each from its
* own source, so that the order they arrive in cannot change the outcome.
*   The kit's own settings are its djin's, `cli`, in the program's registry
* (option_env_registry.h): `cli.plugin_strict_version`, `cli.plugin_search`
* and the program's `plugin_path` on the registry's root. Where
* D_CFG_CLI_PLUGIN_VARIABLE is 0, the caller is not to choose what code a
* program loads, so those two refuse every source the caller controls - the
* user's file, the environment, the command line - and only the program
* and an override bind them (D_OPTION_ENV_REFUSES_CALLER).
*   At a program's first parse the kit:
*     - registers its djin, after every djin a module registered before the
*       program ran;
*     - binds the environment's settings under `DJINTERP_`, then under the
*       program's own name - upper-cased, each `-` an `_` - so that
*       `BITWISEDOM_CLI_PLUGIN_SEARCH` wins over
*       `DJINTERP_CLI_PLUGIN_SEARCH`;
*     - reads the user's settings file, a statement a line, as
*       `cli.plugin_strict_version = false`: a statement that fails is said
*       by its line, as a warning, and the rest are read on;
*     - sets its hook on the program's parser, which offers it the
*       command line's settings where the parse reads them as options:
*       `--NAME=VALUE`, `--NAME` for a bool's true and `--no-NAME` for its
*       false, NAME a full name, which always holds a '.'; and `--set
*       NAME=VALUE` for any name. A response file's are offered as if
*       typed; never one past `--`, nor the value an option before it
*       takes, as `--level --x.y` gives `--level` the word `--x.y`, nor one
*       a plugin command's module is to read.
*   A setting the command line names badly is a usage error, exit 2: a
* name that names no setting, with the nearest that does; a djin not
* registered here, where none is near; a value the setting does not take;
* one this build does not have.
*   The settings are the program's, as its registry is: read and written
* by one thread at a time, before the program starts its threads.
*
* HOW TO USE
* ----------
*   A program built on the kit calls nothing here: d_cli_program_run starts
* the settings at its first parse. A program reads the kit's settings
* through 1.3's queries, and names the user's file with d_cli_settings_file.
* A program of its own djins registers them before the run, so that their
* settings are read from the same three sources. A program that sets a
* setting hook of its own on its parser before the run keeps it, and can
* hand an argument on to the kit's, d_cli_settings_hook.
*
* OPTIONS
* -------
*   `cli.plugin_strict_version` (bool), `cli.plugin_search` (text), the
* root's `plugin_path` (text), and what the user's file is said for:
* `cli.unknown_settings` (none, typos, all or error; typos by default) and
* `cli.shared_refusals` (none, verbose, warn or error; verbose by default).
* Each is set from the user's file
* ($XDG_CONFIG_HOME/djinterp/settings), the environment (`DJINTERP_` and the
* program's own prefix) and the command line (`--NAME=VALUE`, `--NAME`,
* `--no-NAME`, `--set NAME=VALUE`), in that order of standing, the command
* line's highest.
*
* CONFIGURATION
* -------------
*   Each setting's default is its knob in cfg_cli.h:
* D_CFG_CLI_PLUGIN_STRICT_VERSION, D_CFG_CLI_PLUGIN_SEARCH,
* D_CFG_CLI_UNKNOWN_SETTINGS and D_CFG_CLI_SHARED_REFUSALS.
* D_CFG_CLI_PLUGIN_VARIABLE 0 makes the two plugin settings refuse every
* source the caller controls.
*
*
* path:      /inc/djinterp/tools/cli/c/cli_settings.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.21
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
         2.  d_cli_settings_hook
         3.  d_cli_settings_refused
         4.  d_cli_settings_file
         5.  d_cli_settings_why
         6.  d_cli_settings_report
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
D_CLI_API struct dj_option_env* d_cli_settings_djin(void);


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
D_CLI_API int d_cli_settings_start(
    const struct dj_cli_program* _program
);

// 1.2.2
// d_cli_settings_hook
/**
 * @brief The kit's setting hook (dj_arg_setting_fn): takes a setting the
 *        parse offers it - `--NAME=VALUE`, `--NAME`, `--no-NAME`, or
 *        `--set` and the NAME=VALUE after it - into the registry, from
 *        COMMAND_LINE.
 *
 * @note d_cli_program_parse sets it on a program's parser before each
 *       parse (d_arg_parser_set_setting_hook), unless the program has set
 *       a hook of its own, which can call this one.
 * @note Of the long options the parse does not know, only one shaped as a
 *       setting - `--set`, or a name holding a '.' after `no-` and before
 *       `=` - is the kit's; for any other it returns 0, so that the parse
 *       goes on to its abbreviations and suggestions, `--out` becoming
 *       `--output`. No option of a program's holds a '.' (R5).
 * @note A setting it refuses ends the parse (D_ARG_ERROR_SETTING), its
 *       words kept for d_cli_settings_refused.
 *
 * @param[in]     _ctx     the registry to take into; NULL for the
 *                         program's (d_option_env_registry).
 * @param[in]     _argc    the parser's argument count.
 * @param[in]     _argv    the parser's arguments, `_argv[0]` the program's
 *                         name.
 * @param[in,out] _at      the offered argument's index; moved onto the
 *                         last argument taken.
 * @param[out]    _status  receives the registry's reason for a refusal
 *                         (dj_option_status); may be NULL.
 * @return how many arguments it took, the offered one first; 0 for an
 *         argument not the kit's; negative for a refusal.
 */
D_CLI_API int d_cli_settings_hook(
    void*        _ctx,
    int          _argc,
    char* const* _argv,
    int*         _at,
    int32_t*     _status
);

// 1.2.3
// d_cli_settings_refused
/**
 * @brief Says the setting the kit's hook refused, when the program's last
 *        parse ended with that refusal: a usage error that names the
 *        option as typed and what was wrong with it, from the command the
 *        parse failed in.
 *
 * @note d_cli_program_parse calls it for a parse that ended in
 *       D_ARG_ERROR_SETTING; a refusal a program's own hook made is said
 *       as the parser's failure message says it. What it held is let go of
 *       either way.
 *
 * @param[in] _program  the program, for its parser's error and its
 *                      reports.
 * @return D_CLI_EXIT_TROUBLE, said, for the kit's refusal; D_CLI_EXIT_OK,
 *         saying nothing, when the parse did not end with one.
 */
D_CLI_API int d_cli_settings_refused(
    const struct dj_cli_program* _program
);

// 1.2.4
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
D_CLI_API size_t d_cli_settings_file(
    char*  _buffer,
    size_t _capacity
);

// 1.2.5
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
D_CLI_API void d_cli_settings_why(
    int32_t     _status,
    const char* _name,
    size_t      _size,
    const char* _value,
    char*       _buffer,
    size_t      _capacity
);


// 1.2.6
// d_cli_settings_report
/**
 * @brief Says what the run's start held: each statement of the user's file
 *        and each value of the environment a setting could not take, once
 *        the parse has ended, so that -q, -v and the command line's own
 *        settings govern them. `cli.unknown_settings` decides what is said
 *        of a name no setting here has - nothing for a djin the program
 *        does not register, by default - and `cli.shared_refusals` of a
 *        setting that refuses the user's file or `DJINTERP_`; a refusal
 *        under the program's own prefix, and a value a setting does not
 *        take, are warnings; an environment variable set empty is taken as
 *        one not set, and is not said. Each is said once.
 *
 * @param[in] _program  the program, for its reports and its verbosity.
 * @return D_CLI_EXIT_OK; D_CLI_EXIT_TROUBLE when a setting made one an
 *         error.
 */
D_CLI_API int d_cli_settings_report(
    const struct dj_cli_program* _program
);

// 1.3    What the kit reads
//------------------------------------------------------------------------------
// 1.3.1
// d_cli_settings_plugin_search
//   query: the directories a module is looked for in after the run's own
// and `plugin_path`'s: `cli.plugin_search`, or D_CFG_CLI_PLUGIN_SEARCH
// before the settings are started or where the variable is not declared.
D_CLI_API const char* d_cli_settings_plugin_search(void);

// 1.3.2
// d_cli_settings_plugin_path
//   query: the program's `plugin_path`, read from PROGRAM_PLUGIN_PATH, the
// file or the command line; where D_CFG_CLI_PLUGIN_VARIABLE is 0, from the
// program and an override alone. NULL when unset.
D_CLI_API const char* d_cli_settings_plugin_path(void);

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
D_CLI_API bool d_cli_settings_plugin_strict_version(
    int _program_says
);


// 1.4    Testing
//------------------------------------------------------------------------------
#if defined(D_TESTING)
#if (D_TESTING == 1)

// 1.4.1
// d_cli_settings_reset
//   testing: lets go of the run's settings - the kit's djin taken out of
// the program's registry, its bindings and the root's gone - so that the
// next d_cli_settings_start starts them again, as a new run would.
D_CLI_API void d_cli_settings_reset(void);

#endif  // D_TESTING
#endif  // defined(D_TESTING)

D_EXTERN_C_END


#endif  // DJINTERP_TOOLS_CLI_C_CLI_SETTINGS_H
