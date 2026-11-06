/*******************************************************************************
* djinterp [config]                                                    cfg_cli.h
*
* Configuration of the command-line kit.
*   Which optional parts of the kit are compiled -- the plugin loader, colour,
* its own layout of help, and wide writes to a Windows console -- the widths
* help is laid out to, and where a program looks for its modules and which it
* takes, resolved into D_INTERNAL_CLI_*.
*   Every feature knob is a BRANCH knob: it removes code, never a struct
* member, so a structure has one layout whatever the knobs say, and a C unit
* and a C++ one built with different settings still agree on it. A knob
* turned off leaves the kit's behaviour defined: without plugins a plugin
* command says the program was built without them and exits 2; without
* colour no stream is coloured; without the kit's layout help is the
* argument parser's own.
*   The plugin knobs of 1.4 are DEFAULTS: each sets what a program does
* when nothing at run time says otherwise, and the program's own setting --
* d_cli_program_set_plugin_path and d_cli_program_set_plugin_versions,
* which the run-time options are read into -- overrides it per run.
*   targets:  the C headers in tools/cli/c/, their sources, and the C++ faces
*             beside them -> D_INTERNAL_CLI_*
*   requires: cfg_common.h; env_cli.h, for what the platform can do
*
*
* path:      /inc/djinterp/config/tools/cli/cfg_cli.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.11.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Aggregate
         1.  D_CFG_CLI_ALL
    2.  Features
         1.  D_CFG_CLI_PLUGINS
         2.  D_CFG_CLI_COLOUR
         3.  D_CFG_CLI_FORMATTER
         4.  D_CFG_CLI_CONSOLE_WIDE
    3.  Sizes
         1.  D_CFG_CLI_HELP_WIDTH
         2.  D_CFG_CLI_HELP_WIDTH_MAX
         3.  D_CFG_CLI_HELP_COLUMN_MAX
    4.  Plugins
         1.  D_CFG_CLI_PLUGIN_SEARCH
         2.  D_CFG_CLI_PLUGIN_VARIABLE
         3.  D_CFG_CLI_PLUGIN_STRICT_VERSION
    5.  Validation
2.  RESOLVED VALUES
    ---------------
    1.  Features
         1.  D_INTERNAL_CLI_PLUGINS
         2.  D_INTERNAL_CLI_COLOUR
         3.  D_INTERNAL_CLI_FORMATTER
         4.  D_INTERNAL_CLI_CONSOLE_WIDE
    2.  Sizes
         1.  D_INTERNAL_CLI_HELP_WIDTH
         2.  D_INTERNAL_CLI_HELP_WIDTH_MAX
         3.  D_INTERNAL_CLI_HELP_COLUMN_MAX
    3.  Plugins
         1.  D_INTERNAL_CLI_PLUGIN_SEARCH / _SEARCH_DEFAULT
         2.  D_INTERNAL_CLI_PLUGIN_VARIABLE
         3.  D_INTERNAL_CLI_PLUGIN_STRICT_VERSION
    4.  Allocation
         1.  D_INTERNAL_CLI_MALLOC / D_INTERNAL_CLI_REALLOC /
             D_INTERNAL_CLI_FREE
*/

#ifndef DJINTERP_CONFIG_TOOLS_CLI_CFG_CLI_H
#define DJINTERP_CONFIG_TOOLS_CLI_CFG_CLI_H 1

// djinterp
#include "../../cfg_common.h"                // D_CFG_NORM, D_CFG_IS_BOOL
#include "../../c/filesys/cfg_file_common.h"  // D_CFG_FILE_MALLOC,
                                              // D_CFG_FILE_REALLOC,
                                              // D_CFG_FILE_FREE
#include "../../../env/tools/cli/env_cli.h"  // D_ENV_CLI_*


//==============================================================================
// 1.  KNOBS
//==============================================================================
// Each is #ifndef-guarded, so a value defined earlier -- by -D, a prefix
// header, or cfg_custom.h -- wins. Feature knobs fall back to the aggregate
// before their own default.


// 1.1    Aggregate
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_CLI_ALL
//   knob: optional aggregate for the feature knobs of 1.2. NOT defaulted -- its
// absence is meaningful. Define it to 0 or 1, and every feature knob with no
// value of its own falls back to it: 0 builds the smallest kit, the grammar's
// conventions over the argument parser and nothing more.

// 1.2    Features
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_CLI_PLUGINS
//   knob: 1 compiles the plugin loader -- finding a program's module, loading
// it, checking it belongs to the program, and running it -- where the
// platform has loadable modules (D_ENV_CLI_MODULES); 0 removes it, and a
// plugin command then says the program was built without plugins. Default 1.
#ifndef D_CFG_CLI_PLUGINS
    #if defined(D_CFG_CLI_ALL)
        #define D_CFG_CLI_PLUGINS        D_CFG_CLI_ALL
    #else
        #define D_CFG_CLI_PLUGINS        1
    #endif
#endif  // D_CFG_CLI_PLUGINS

// 1.2.2
// D_CFG_CLI_COLOUR
//   knob: 1 colours diagnostics on a terminal, as `--color` and NO_COLOR
// allow; 0 never colours, and `--color` is still accepted. Default 1.
#ifndef D_CFG_CLI_COLOUR
    #if defined(D_CFG_CLI_ALL)
        #define D_CFG_CLI_COLOUR         D_CFG_CLI_ALL
    #else
        #define D_CFG_CLI_COLOUR         1
    #endif
#endif  // D_CFG_CLI_COLOUR

// 1.2.3
// D_CFG_CLI_FORMATTER
//   knob: 1 lays help out as GNU programs do -- `Usage:` first, then the
// description, `Arguments:`, `Commands:` and `Options:`, each option as
// `-o, --out=FILE` -- through a formatter of the kit's; 0 leaves the argument
// parser's own layout, CLI11's. Default 1.
#ifndef D_CFG_CLI_FORMATTER
    #if defined(D_CFG_CLI_ALL)
        #define D_CFG_CLI_FORMATTER      D_CFG_CLI_ALL
    #else
        #define D_CFG_CLI_FORMATTER      1
    #endif
#endif  // D_CFG_CLI_FORMATTER

// 1.2.4
// D_CFG_CLI_CONSOLE_WIDE
//   knob: 1 writes text to a Windows console through WriteConsoleW, so that it
// reads correctly whatever the console's code page; 0 writes UTF-8 bytes to
// it, as to a file. Nothing elsewhere. Default 1.
#ifndef D_CFG_CLI_CONSOLE_WIDE
    #if defined(D_CFG_CLI_ALL)
        #define D_CFG_CLI_CONSOLE_WIDE   D_CFG_CLI_ALL
    #else
        #define D_CFG_CLI_CONSOLE_WIDE   1
    #endif
#endif  // D_CFG_CLI_CONSOLE_WIDE

// 1.3    Sizes
//------------------------------------------------------------------------------
// 1.3.1
// D_CFG_CLI_HELP_WIDTH
//   knob: the columns help is wrapped to when standard output is not a
// terminal, so that what a script captures does not depend on the window it
// ran in. Default 80.
#ifndef D_CFG_CLI_HELP_WIDTH
    #define D_CFG_CLI_HELP_WIDTH         80
#endif  // D_CFG_CLI_HELP_WIDTH

// 1.3.2
// D_CFG_CLI_HELP_WIDTH_MAX
//   knob: the most columns help is wrapped to on a terminal, however wide:
// a line past it is hard to read. Default 100.
#ifndef D_CFG_CLI_HELP_WIDTH_MAX
    #define D_CFG_CLI_HELP_WIDTH_MAX     100
#endif  // D_CFG_CLI_HELP_WIDTH_MAX

// 1.3.3
// D_CFG_CLI_HELP_COLUMN_MAX
//   knob: the furthest right a description in help may start; an option
// whose names and placeholder reach past it has its description on the next
// line. Default 30.
#ifndef D_CFG_CLI_HELP_COLUMN_MAX
    #define D_CFG_CLI_HELP_COLUMN_MAX    30
#endif  // D_CFG_CLI_HELP_COLUMN_MAX

// 1.4    Plugins
//------------------------------------------------------------------------------
// 1.4.1
// D_CFG_CLI_PLUGIN_SEARCH
//   knob: the directories a program looks for a command's module in after
// the ones it is told of at run time (d_cli_program_set_plugin_path) and
// PROGRAM_PLUGIN_PATH (1.4.2): a string literal, its directories separated
// as PATH separates them -- ':', or ';' on Windows -- each searched in turn,
// the first that holds the module winning. In a directory, `$ORIGIN` (or
// `${ORIGIN}`) stands for the directory of the program's own file, as an ELF
// run path writes it, and `$PROGRAM` (or `${PROGRAM}`) for the program's
// name; a directory that uses `$ORIGIN` is passed over where the program's
// file cannot be found, and on Windows each `/` is written `\`. Default:
// the program's directory, then `../lib/PROGRAM` from it -- a build tree, a
// Windows folder and a POSIX installation, with nothing to configure
// (ruling cli-taken 14). `"$ORIGIN"` alone looks beside the program only;
// an installation whose modules live elsewhere names that directory, as
// `"/usr/lib64/$PROGRAM"`.
#ifndef D_CFG_CLI_PLUGIN_SEARCH
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_CFG_CLI_PLUGIN_SEARCH  "$ORIGIN;$ORIGIN/../lib/$PROGRAM"
    #else
        #define D_CFG_CLI_PLUGIN_SEARCH  "$ORIGIN:$ORIGIN/../lib/$PROGRAM"
    #endif

    #define D_INTERNAL_CLI_PLUGIN_SEARCH_DEFAULT 1
#else
    #define D_INTERNAL_CLI_PLUGIN_SEARCH_DEFAULT 0
#endif  // D_CFG_CLI_PLUGIN_SEARCH

// 1.4.2
// D_CFG_CLI_PLUGIN_VARIABLE
//   knob: 1 reads PROGRAM_PLUGIN_PATH -- the program's name upper-cased,
// each `-` an `_` -- and looks in its directories before the build's
// (1.4.1); 0 never reads it, for a program whose environment is not to be
// trusted with what code it loads, as one run with privileges it does not
// share with its caller. Default 1.
#ifndef D_CFG_CLI_PLUGIN_VARIABLE
    #define D_CFG_CLI_PLUGIN_VARIABLE    1
#endif  // D_CFG_CLI_PLUGIN_VARIABLE

// 1.4.3
// D_CFG_CLI_PLUGIN_STRICT_VERSION
//   knob: 1 refuses a module that belongs to another version of the program,
// naming both versions, and the command exits 2 (ruling cli-taken 15): the
// module runs its own copy of the core, so a sheet a newer window saves may
// be one an older program cannot read; 0 loads it, with a warning that names
// both. A module built to another plugin ABI is refused either way. A
// program's own setting overrides it per run
// (d_cli_program_set_plugin_versions). Default 1.
#ifndef D_CFG_CLI_PLUGIN_STRICT_VERSION
    #define D_CFG_CLI_PLUGIN_STRICT_VERSION 1
#endif  // D_CFG_CLI_PLUGIN_STRICT_VERSION

// 1.5    Validation
//------------------------------------------------------------------------------
#if !D_CFG_IS_BOOL(D_CFG_CLI_PLUGINS)
    #error "cfg_cli: D_CFG_CLI_PLUGINS must be 0 or 1"
#endif

#if !D_CFG_IS_BOOL(D_CFG_CLI_COLOUR)
    #error "cfg_cli: D_CFG_CLI_COLOUR must be 0 or 1"
#endif

#if !D_CFG_IS_BOOL(D_CFG_CLI_FORMATTER)
    #error "cfg_cli: D_CFG_CLI_FORMATTER must be 0 or 1"
#endif

#if !D_CFG_IS_BOOL(D_CFG_CLI_CONSOLE_WIDE)
    #error "cfg_cli: D_CFG_CLI_CONSOLE_WIDE must be 0 or 1"
#endif

#if !D_CFG_IS_BOOL(D_CFG_CLI_PLUGIN_VARIABLE)
    #error "cfg_cli: D_CFG_CLI_PLUGIN_VARIABLE must be 0 or 1"
#endif

#if !D_CFG_IS_BOOL(D_CFG_CLI_PLUGIN_STRICT_VERSION)
    #error "cfg_cli: D_CFG_CLI_PLUGIN_STRICT_VERSION must be 0 or 1"
#endif

#if ( (D_CFG_NORM(D_CFG_CLI_HELP_WIDTH) < 40) ||                              \
      (D_CFG_NORM(D_CFG_CLI_HELP_WIDTH) > 1000) )
    #error "cfg_cli: D_CFG_CLI_HELP_WIDTH must lie in [40, 1000]"
#endif

#if ( (D_CFG_NORM(D_CFG_CLI_HELP_WIDTH_MAX) < 40) ||                          \
      (D_CFG_NORM(D_CFG_CLI_HELP_WIDTH_MAX) > 1000) )
    #error "cfg_cli: D_CFG_CLI_HELP_WIDTH_MAX must lie in [40, 1000]"
#endif

#if ( (D_CFG_NORM(D_CFG_CLI_HELP_COLUMN_MAX) < 10) ||                         \
      (D_CFG_NORM(D_CFG_CLI_HELP_COLUMN_MAX) >                                \
       (D_CFG_NORM(D_CFG_CLI_HELP_WIDTH) - 20)) )
    #error "cfg_cli: D_CFG_CLI_HELP_COLUMN_MAX must lie in [10, \
D_CFG_CLI_HELP_WIDTH - 20]"
#endif


//==============================================================================
// 2.  RESOLVED VALUES
//==============================================================================
// What the kit's headers and sources read. Each is 0 or 1, or a number.


// 2.1    Features
//------------------------------------------------------------------------------
// 2.1.1
// D_INTERNAL_CLI_PLUGINS
//   resolved: 1 when the plugin loader is compiled: asked for, and the
// platform has loadable modules.
#if ( (D_CFG_IS_ON(D_CFG_CLI_PLUGINS)) &&                                     \
      (D_ENV_CLI_MODULES != D_ENV_CLI_MODULES_NONE) )
    #define D_INTERNAL_CLI_PLUGINS 1
#else
    #define D_INTERNAL_CLI_PLUGINS 0
#endif

// 2.1.2
// D_INTERNAL_CLI_COLOUR
//   resolved: 1 when diagnostics may be coloured.
#define D_INTERNAL_CLI_COLOUR                                                  \
    D_CFG_NORM(D_CFG_CLI_COLOUR)

// 2.1.3
// D_INTERNAL_CLI_FORMATTER
//   resolved: 1 when help takes the kit's layout.
#define D_INTERNAL_CLI_FORMATTER                                               \
    D_CFG_NORM(D_CFG_CLI_FORMATTER)

// 2.1.4
// D_INTERNAL_CLI_CONSOLE_WIDE
//   resolved: 1 when a Windows console is written through WriteConsoleW:
// asked for, and the platform's consoles are Windows'.
#if ( (D_CFG_IS_ON(D_CFG_CLI_CONSOLE_WIDE)) &&                                \
      (D_ENV_CLI_CONSOLE == D_ENV_CLI_CONSOLE_WINDOWS) )
    #define D_INTERNAL_CLI_CONSOLE_WIDE 1
#else
    #define D_INTERNAL_CLI_CONSOLE_WIDE 0
#endif

// 2.2    Sizes
//------------------------------------------------------------------------------
// 2.2.1
// D_INTERNAL_CLI_HELP_WIDTH
//   resolved: help's width off a terminal.
#define D_INTERNAL_CLI_HELP_WIDTH                                              \
    D_CFG_NORM(D_CFG_CLI_HELP_WIDTH)

// 2.2.2
// D_INTERNAL_CLI_HELP_WIDTH_MAX
//   resolved: help's widest on a terminal.
#define D_INTERNAL_CLI_HELP_WIDTH_MAX                                          \
    D_CFG_NORM(D_CFG_CLI_HELP_WIDTH_MAX)

// 2.2.3
// D_INTERNAL_CLI_HELP_COLUMN_MAX
//   resolved: the furthest right a description starts.
#define D_INTERNAL_CLI_HELP_COLUMN_MAX                                         \
    D_CFG_NORM(D_CFG_CLI_HELP_COLUMN_MAX)

// 2.3    Plugins
//------------------------------------------------------------------------------
// 2.3.1
// D_INTERNAL_CLI_PLUGIN_SEARCH / _SEARCH_DEFAULT
//   resolved: the build's directories for a module, as D_CFG_CLI_PLUGIN_SEARCH
// writes them; and 1 when that is the default, the knob not given -- which
// the kit's suites read, the places they expect a module looked for being
// the default's. Defined with the knob, in 1.4.1.
#define D_INTERNAL_CLI_PLUGIN_SEARCH                                           \
    D_CFG_CLI_PLUGIN_SEARCH

// 2.3.2
// D_INTERNAL_CLI_PLUGIN_VARIABLE
//   resolved: 1 when PROGRAM_PLUGIN_PATH is read.
#define D_INTERNAL_CLI_PLUGIN_VARIABLE                                         \
    D_CFG_NORM(D_CFG_CLI_PLUGIN_VARIABLE)

// 2.3.3
// D_INTERNAL_CLI_PLUGIN_STRICT_VERSION
//   resolved: 1 when a module of another version is refused, unless the
// program's setting says otherwise.
#define D_INTERNAL_CLI_PLUGIN_STRICT_VERSION                                   \
    D_CFG_NORM(D_CFG_CLI_PLUGIN_STRICT_VERSION)

// 2.4    Allocation
//------------------------------------------------------------------------------
// 2.4.1
// D_INTERNAL_CLI_MALLOC / D_INTERNAL_CLI_REALLOC / D_INTERNAL_CLI_FREE
//   resolved: the kit allocates through filesys's hooks, so that a block it
// returns from a filesys read and one it made itself are freed alike, by
// d_cli_free, and a host that captures filesys's allocations captures the
// kit's.
#define D_INTERNAL_CLI_MALLOC(_size)          D_CFG_FILE_MALLOC(_size)
#define D_INTERNAL_CLI_REALLOC(_ptr, _size)   D_CFG_FILE_REALLOC(_ptr, _size)
#define D_INTERNAL_CLI_FREE(_ptr)             D_CFG_FILE_FREE(_ptr)


#endif  // DJINTERP_CONFIG_TOOLS_CLI_CFG_CLI_H
