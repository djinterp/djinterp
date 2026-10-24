/*******************************************************************************
* djinterp [parse]                                                  cli_plugin.h
*
* A program's commands that live in modules of their own: its window, loaded
* only when asked for (GRAMMAR.md, section 7).
*   THE INTERFACE. A program and a module it loads meet at two structures
* and one function, all C: the module exports `d_cli_plugin_entry`, which
* returns its d_cli_plugin -- the ABI it was built to, the program and version
* it belongs to, its name and summary, and the function that runs it -- and
* the program calls that function with a d_cli_plugin_host -- itself, as the
* module should present it -- and the arguments after the command's name.
* Nothing else crosses: each side has its own heap, its own C runtime where
* the platform gives each module one, and its own copy of whatever library
* both link, so a module built with another toolchain's runtime still runs,
* and the program exports nothing. D_CLI_PLUGIN defines the entry point.
*   FINDING ONE. The module of command NAME of program PROGRAM is the file
* `PROGRAM-NAME` with the platform's suffix (D_ENV_CLI_MODULE_SUFFIX), looked
* for in the directories PROGRAM_PLUGIN_PATH lists -- the program's name
* upper-cased, each `-` an `_` -- then in the program's own directory, then in
* `../lib/PROGRAM/` from it. A program whose file carries a tag after its
* name -- `bitwisedom-linux64`, as a build's bin/ directory names it, so that
* every platform's builds can share one -- looks in each first for the
* module with the same tag, `bitwisedom-gui-linux64.so`.
*   LOADING ONE. A module whose ABI is another, or that belongs to another
* program, or to another version of this one, is refused, by name and with
* both versions. A module loaded stays loaded until the process ends: what
* it started -- a window system's or a GPU driver's threads -- may outlive
* its run.
*   ITS PROGRAM. A module reads its arguments with its own parser and the
* same kit: d_cli_program_init_plugin names its program as the host's and
* takes the host's verbosity and colour, so that `bitwisedom -v gui` is
* verbose in the window too, and `bitwisedom gui --help` is the window's help.
*
*
* path:      /inc/djinterp/parse/cli/c/cli_plugin.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.10.22
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE INTERFACE
    -------------
    1.  Versions and names
         1.  D_CLI_PLUGIN_ABI
         2.  D_CLI_PLUGIN_ENTRY_NAME
    2.  Records
         1.  d_cli_plugin_host
         2.  d_cli_plugin_run_fn
         3.  d_cli_plugin
         4.  d_cli_plugin_entry_fn
    3.  The entry point
         1.  D_CLI_PLUGIN
2.  THE PROGRAM'S FILE
    ------------------
    1.  d_cli_executable_path
3.  MODULES
    -------
    1.  Records
         1.  d_cli_module
         2.  D_CLI_MODULE_INIT
    2.  Finding
         1.  d_cli_plugin_find
    3.  Loading
         1.  d_cli_module_load
         2.  d_cli_module_unload
4.  IN A PROGRAM
    ------------
    1.  The host's side
         1.  d_cli_program_add_plugin
    2.  The module's side
         1.  d_cli_program_init_plugin
*/

#ifndef DJINTERP_PARSE_CLI_C_CLI_PLUGIN_H
#define DJINTERP_PARSE_CLI_C_CLI_PLUGIN_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../../c/djinterp.h"                    // framework root
#include "../../../config/parse/cli/cfg_cli.h"      // D_INTERNAL_CLI_PLUGINS
#include "../../../env/c/env_vendor_attributes.h"   // D_EXPORT
#include "../arg_parser/c/arg_parser.h"             // d_arg_parser,
                                                    // d_arg_command,
                                                    // d_arg_buffer
#include "./cli_program.h"                          // d_cli_program
// re_std
#include "../../../../re_std/cstdint/dstdint.h"     // uint32_t


//==============================================================================
// 1.  THE INTERFACE
//==============================================================================


// 1.1    Versions and names
//------------------------------------------------------------------------------
// 1.1.1
// D_CLI_PLUGIN_ABI
//   constant: the version of the two records below. A record only grows, its
// `size` saying how far, and a change to a field that is there is a new ABI.
#define D_CLI_PLUGIN_ABI        1u

// 1.1.2
// D_CLI_PLUGIN_ENTRY_NAME
//   constant: the name a module exports its entry point by.
#define D_CLI_PLUGIN_ENTRY_NAME "d_cli_plugin_entry"

// 1.2    Records
//------------------------------------------------------------------------------
// 1.2.1
// d_cli_plugin_host
//   struct: the program that runs a module, as the module should present
// it: its name and version, as diagnostics and `--version` say them; the
// words its usage lines begin with, `bitwisedom gui`; its own file and the
// module's, or NULL where unknown; and the verbosity and the d_cli_colour
// mode the program's own options gave. Every string outlives the run.
struct d_cli_plugin_host
{
    uint32_t    abi;
    uint32_t    size;
    const char* program;
    const char* version;
    const char* command;
    const char* executable;
    const char* module;
    int         verbosity;
    int         colour;
};

// 1.2.2
// d_cli_plugin_run_fn
//   type: a module's work: given the host and the arguments after the
// command's name -- `_argv[0]` the host's `command`, `_argv[_argc]` NULL --
// it returns the status the program exits with (GRAMMAR.md, section 8).
typedef int (*d_cli_plugin_run_fn)(const struct d_cli_plugin_host* _host,
                                   int                             _argc,
                                   const char* const*              _argv);

// 1.2.3
// d_cli_plugin
//   struct: what a module says of itself: the ABI it was built to and the
// record's size; the program and the version it belongs to; the command it
// is, `gui`, and a line saying what it does; and its work.
struct d_cli_plugin
{
    uint32_t            abi;
    uint32_t            size;
    const char*         program;
    const char*         version;
    const char*         name;
    const char*         summary;
    d_cli_plugin_run_fn run;
};

// 1.2.4
// d_cli_plugin_entry_fn
//   type: a module's entry point, D_CLI_PLUGIN_ENTRY_NAME: its d_cli_plugin,
// which lives as long as the module.
typedef const struct d_cli_plugin* (*d_cli_plugin_entry_fn)(void);

// 1.3    The entry point
//------------------------------------------------------------------------------
// 1.3.1
// D_CLI_PLUGIN
//   macro: defines a module's entry point, exported with C linkage, in
// exactly one of its units: the program and version it belongs to, its
// command's name and summary, all string literals, and its
// d_cli_plugin_run_fn.
#define D_CLI_PLUGIN(program,                                                  \
                     version,                                                  \
                     name,                                                     \
                     summary,                                                  \
                     run)                                                      \
    D_EXTERN_C D_EXPORT const struct d_cli_plugin*                             \
        d_cli_plugin_entry(void);                                              \
                                                                               \
    D_EXTERN_C D_EXPORT const struct d_cli_plugin*                             \
    d_cli_plugin_entry(void)                                                   \
    {                                                                          \
        static const struct d_cli_plugin plugin =                              \
        {                                                                      \
            D_CLI_PLUGIN_ABI,                                                  \
            sizeof(struct d_cli_plugin),                                       \
            (program),                                                         \
            (version),                                                         \
            (name),                                                            \
            (summary),                                                         \
            (run)                                                              \
        };                                                                     \
                                                                               \
        return &plugin;                                                        \
    }


D_EXTERN_C_BEGIN

//==============================================================================
// 2.  THE PROGRAM'S FILE
//==============================================================================


// 2.1.1
// d_cli_executable_path
/**
 * @brief Finds the file the running program was started from.
 *
 * @note Asked of the system as D_ENV_CLI_EXECUTABLE says -- the process file
 *       system's link, Windows' module name, macOS's dyld, FreeBSD's sysctl,
 *       Solaris' getexecname -- and, where the system cannot say, worked out
 *       from `_argv0`: made absolute where it names a directory, and looked
 *       for along PATH where it does not.
 *
 * @param[in]  _argv0  the program's `argv[0]`, for the fallback; may be
 *                     NULL.
 * @param[out] _path   receives the absolute path in UTF-8, to be freed with
 *                     d_cli_free; NULL on failure.
 * @return 0 when found, or an errno value: EINVAL for a NULL `_path`,
 *         ENOENT when no way of asking found it, ENOMEM.
 */
int         d_cli_executable_path(const char* _argv0,
                                  char**      _path);


//==============================================================================
// 3.  MODULES
//==============================================================================


// 3.1    Records
//------------------------------------------------------------------------------
// 3.1.1
// d_cli_module
//   struct: a module loaded, or none: the system's handle, the module's
// d_cli_plugin, and the file it was loaded from, owned.
struct d_cli_module
{
    void*                      handle;
    const struct d_cli_plugin* plugin;
    char*                      path;
};

// 3.1.2
// D_CLI_MODULE_INIT
//   constant: the initializer of a d_cli_module that holds no module.
#define D_CLI_MODULE_INIT { NULL, NULL, NULL }

// 3.2    Finding
//------------------------------------------------------------------------------
// 3.2.1
// d_cli_plugin_find
/**
 * @brief Finds the module of a program's command.
 *
 * @note In each directory, the module named with the tag the program's
 *       file carries after the program's name -- `bitwisedom-linux64`'s
 *       `-linux64`, less `.exe` on Windows -- is taken before the one named
 *       without it.
 *
 * @param[in]     _program     the program's name, `bitwisedom`.
 * @param[in]     _name        the command's, `gui`.
 * @param[in]     _executable  the program's own file, as
 *                             d_cli_executable_path finds it; NULL leaves
 *                             only PROGRAM_PLUGIN_PATH to search.
 * @param[in,out] _found       the module's path is appended to it when
 *                             found.
 * @param[in,out] _searched    each directory searched is appended to it, a
 *                             line each, so a refusal can say where it
 *                             looked; may be NULL.
 * @return D_ARG_OK when found; D_ARG_ERROR_FILE when no directory held it;
 *         D_ARG_ERROR_OUT_OF_MEMORY; D_ARG_ERROR_BAD_ARGUMENT for a NULL
 *         name or buffer.
 */
int         d_cli_plugin_find(const char*          _program,
                              const char*          _name,
                              const char*          _executable,
                              struct d_arg_buffer* _found,
                              struct d_arg_buffer* _searched);

// 3.3    Loading
//------------------------------------------------------------------------------
// 3.3.1
// d_cli_module_load
/**
 * @brief Loads a module, and checks it belongs to the program loading it.
 *
 * @note The module is opened with every symbol resolved at once, and its
 *       own symbols kept to itself; its entry point is called, and its
 *       record checked: its ABI is D_CLI_PLUGIN_ABI and its size at least
 *       the record's, and its program and version are those given. A module
 *       refused is unloaded again.
 *
 * @param[out]    _module   receives the module; none on failure.
 * @param[in]     _path     the module's file, in UTF-8.
 * @param[in]     _program  the program it must belong to.
 * @param[in]     _version  the version it must belong to; NULL accepts any.
 * @param[in,out] _why      on failure, why, as a message's words, is
 *                          appended to it; may be NULL.
 * @return D_ARG_OK when loaded; D_ARG_ERROR_FILE when it could not be
 *         opened or has no entry point; D_ARG_ERROR_INVALID when its record
 *         is refused; D_ARG_ERROR_UNSUPPORTED where the kit has no plugin
 *         loader; D_ARG_ERROR_OUT_OF_MEMORY; D_ARG_ERROR_BAD_ARGUMENT for a
 *         NULL module, path or program.
 */
int         d_cli_module_load(struct d_cli_module* _module,
                              const char*          _path,
                              const char*          _program,
                              const char*          _version,
                              struct d_arg_buffer* _why);

// 3.3.2
// d_cli_module_unload
/**
 * @brief Unloads a module.
 *
 * @warning Whatever the module started that outlives its run -- threads, a
 *          handler registered with the C runtime -- then runs code that is
 *          gone. A program does not unload a window's module; a test of the
 *          loader does.
 *
 * @param[in,out] _module  the module; none, or NULL, is ignored.
 * @post   `_module` is none; the module's record and code are invalid.
 */
void        d_cli_module_unload(struct d_cli_module* _module);


//==============================================================================
// 4.  IN A PROGRAM
//==============================================================================


// 4.1    The host's side
//------------------------------------------------------------------------------
// 4.1.1
// d_cli_program_add_plugin
/**
 * @brief Adds a command whose work is a module's: `bitwisedom gui`.
 *
 * @note The command takes every argument after its name as it is -- its own
 *       `--help` included -- and its work finds the module
 *       (d_cli_plugin_find), loads it, and runs it with a d_cli_plugin_host
 *       made from the program, returning the module's status. A module not
 *       found, or refused, is reported -- where it looked, or why -- and is
 *       D_CLI_EXIT_TROUBLE; so is the command where the kit has no plugin
 *       loader, which says so.
 *
 * @param[in,out] _program      the program.
 * @param[in,out] _parent       the command to add it under; NULL for the
 *                              root.
 * @param[in]     _name         the command's name, and the second half of
 *                              the module's: `gui`.
 * @param[in]     _description  what it does, as the command list shows it.
 * @return the command; NULL on failure, as d_cli_program_add_command.
 */
struct d_arg_command*
            d_cli_program_add_plugin(struct d_cli_program* _program,
                                     struct d_arg_command* _parent,
                                     const char*           _name,
                                     const char*           _description);

// 4.2    The module's side
//------------------------------------------------------------------------------
// 4.2.1
// d_cli_program_init_plugin
/**
 * @brief Makes a module's program: d_cli_program_init, named and versioned
 *        as the host, starting from its verbosity, and colouring as its
 *        `--color` asked unless the module's own arguments ask otherwise.
 *
 * @note The parser should be made with the host's `command` as its name,
 *       so that the module's usage lines read `Usage: bitwisedom gui ...`.
 *
 * @param[out]    _program  the program to make.
 * @param[in,out] _parser   the parser, made and empty.
 * @param[in]     _host     the host the module was run by.
 * @param[in]     _flags    D_CLI_PROGRAM_* flags.
 * @return as d_cli_program_init; D_ARG_ERROR_BAD_ARGUMENT for a NULL host
 *         too.
 */
int         d_cli_program_init_plugin(struct d_cli_program*           _program,
                                      struct d_arg_parser*            _parser,
                                      const struct d_cli_plugin_host* _host,
                                      uint32_t                        _flags);

D_EXTERN_C_END


#endif  // DJINTERP_PARSE_CLI_C_CLI_PLUGIN_H
