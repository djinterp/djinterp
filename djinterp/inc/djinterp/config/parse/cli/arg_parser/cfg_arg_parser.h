/*******************************************************************************
* djinterp [config]                                             cfg_arg_parser.h
*
* Configuration of the command-line argument parser.
*   Which optional parts of the parser are compiled -- configuration files,
* environment variables, the extra and the path validators, response files,
* suggestions, terminal-width help, completion scripts, UTF-8 argument
* recovery -- the defaults of two run-time settings, and the sizes help and
* suggestions start from, resolved into D_INTERNAL_ARG_PARSER_*.
*   Every feature knob is a BRANCH knob: it removes code, never a struct
* member, so a structure has one layout whatever the knobs say and C and C++
* units built with different settings still agree on it. A knob turned off
* removes its functions; the parser then reports a request for the feature,
* such as a configuration file, as an error rather than ignoring it.
*   targets:  the C headers in parse/cli/arg_parser/c/, their sources, and the
*             C++ faces beside them -> D_INTERNAL_ARG_PARSER_*
*   requires: cfg_common.h; env_arg_parser.h, for the platform defaults
*
*
* path:      /inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.17
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Aggregate
         1.  D_CFG_ARG_PARSER_ALL
    2.  Features
         1.  D_CFG_ARG_PARSER_CONFIG_FILES
         2.  D_CFG_ARG_PARSER_ENVIRONMENT
         3.  D_CFG_ARG_PARSER_EXTRA_VALIDATORS
         4.  D_CFG_ARG_PARSER_PATH_VALIDATORS
         5.  D_CFG_ARG_PARSER_RESPONSE_FILES
         6.  D_CFG_ARG_PARSER_SUGGESTIONS
         7.  D_CFG_ARG_PARSER_TERMINAL_WIDTH
         8.  D_CFG_ARG_PARSER_COMPLETION
         9.  D_CFG_ARG_PARSER_WIDE_ARGV
    3.  Run-time defaults
         1.  D_CFG_ARG_PARSER_WINDOWS_STYLE
         2.  D_CFG_ARG_PARSER_INTEGER_RULE
    4.  Sizes
         1.  D_CFG_ARG_PARSER_COLUMN_WIDTH
         2.  D_CFG_ARG_PARSER_RIGHT_COLUMN_WIDTH
         3.  D_CFG_ARG_PARSER_PARAGRAPH_WIDTH
         4.  D_CFG_ARG_PARSER_SUGGESTION_DISTANCE
         5.  D_CFG_ARG_PARSER_RESPONSE_DEPTH
    5.  Validation
2.  RESOLVED VALUES
    ---------------
    1.  Features
         1.  D_INTERNAL_ARG_PARSER_CONFIG_FILES
         2.  D_INTERNAL_ARG_PARSER_ENVIRONMENT
         3.  D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS
         4.  D_INTERNAL_ARG_PARSER_PATH_VALIDATORS
         5.  D_INTERNAL_ARG_PARSER_RESPONSE_FILES
         6.  D_INTERNAL_ARG_PARSER_SUGGESTIONS
         7.  D_INTERNAL_ARG_PARSER_TERMINAL_WIDTH
         8.  D_INTERNAL_ARG_PARSER_COMPLETION
         9.  D_INTERNAL_ARG_PARSER_WIDE_ARGV
    2.  Run-time defaults
         1.  D_INTERNAL_ARG_PARSER_WINDOWS_STYLE
         2.  D_INTERNAL_ARG_PARSER_INTEGER_RULE
    3.  Sizes
         1.  D_INTERNAL_ARG_PARSER_COLUMN_WIDTH
         2.  D_INTERNAL_ARG_PARSER_RIGHT_COLUMN_WIDTH
         3.  D_INTERNAL_ARG_PARSER_PARAGRAPH_WIDTH
         4.  D_INTERNAL_ARG_PARSER_SUGGESTION_DISTANCE
         5.  D_INTERNAL_ARG_PARSER_RESPONSE_DEPTH
    4.  The C++ boundary
         1.  D_INTERNAL_ARG_PARSER_CPP_CATCH
*/

#ifndef DJINTERP_CONFIG_PARSE_CLI_ARG_PARSER_CFG_ARG_PARSER_H
#define DJINTERP_CONFIG_PARSE_CLI_ARG_PARSER_CFG_ARG_PARSER_H 1

// djinterp
#include "../../../cfg_common.h"  // D_CFG_NORM, D_CFG_IS_BOOL,
                                  // D_CFG_IS_INT_LITERAL
#include "../../../../env/parse/cli/arg_parser/env_arg_parser.h"  // D_ENV_ARG_*


//==============================================================================
// 1.  KNOBS
//==============================================================================
// Each is #ifndef-guarded, so a value defined earlier -- by -D, a prefix
// header, or cfg_custom.h -- wins. Feature knobs fall back to the aggregate
// before their own default.


// 1.1    Aggregate
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_ARG_PARSER_ALL
//   knob: optional aggregate for the feature knobs of 1.2. NOT defaulted -- its
// absence is meaningful. Define it to 0 or 1, and every feature knob with no
// value of its own falls back to it: 0 builds the smallest parser, CLI11's
// core with none of its optional parts.

// 1.2    Features
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_ARG_PARSER_CONFIG_FILES
//   knob: 1 compiles configuration files -- TOML and INI read through a
// command's config option and written by d_arg_command_config_to_sink; 0
// removes them, and a config option then reports a configuration error.
// Default 1.
#ifndef D_CFG_ARG_PARSER_CONFIG_FILES
    #if defined(D_CFG_ARG_PARSER_ALL)
        #define D_CFG_ARG_PARSER_CONFIG_FILES      D_CFG_ARG_PARSER_ALL
    #else
        #define D_CFG_ARG_PARSER_CONFIG_FILES      1
    #endif
#endif  // D_CFG_ARG_PARSER_CONFIG_FILES

// 1.2.2
// D_CFG_ARG_PARSER_ENVIRONMENT
//   knob: 1 lets an option take its value from an environment variable when the
// command line gives none; 0 removes every read of the environment, for a
// sandbox or a platform without one. Default 1.
#ifndef D_CFG_ARG_PARSER_ENVIRONMENT
    #if defined(D_CFG_ARG_PARSER_ALL)
        #define D_CFG_ARG_PARSER_ENVIRONMENT       D_CFG_ARG_PARSER_ALL
    #else
        #define D_CFG_ARG_PARSER_ENVIRONMENT       1
    #endif
#endif  // D_CFG_ARG_PARSER_ENVIRONMENT

// 1.2.3
// D_CFG_ARG_PARSER_EXTRA_VALIDATORS
//   knob: 1 compiles the validators CLI11 compiles unless
// CLI11_DISABLE_EXTRA_VALIDATORS is set -- membership, the transformers,
// numbers with units, sizes, IPv4 addresses, type checks, bounds; 0 keeps
// CLI11's default ones (ranges, number signs, escaped strings) and the path
// validators of 1.2.4. Default 1.
#ifndef D_CFG_ARG_PARSER_EXTRA_VALIDATORS
    #if defined(D_CFG_ARG_PARSER_ALL)
        #define D_CFG_ARG_PARSER_EXTRA_VALIDATORS  D_CFG_ARG_PARSER_ALL
    #else
        #define D_CFG_ARG_PARSER_EXTRA_VALIDATORS  1
    #endif
#endif  // D_CFG_ARG_PARSER_EXTRA_VALIDATORS

// 1.2.4
// D_CFG_ARG_PARSER_PATH_VALIDATORS
//   knob: 1 compiles the validators that ask the file system -- existing file,
// directory and path, nonexistent path, file on a default path, permissions,
// file size -- through the module's own system layer; 0 removes them.
// Default 1.
#ifndef D_CFG_ARG_PARSER_PATH_VALIDATORS
    #if defined(D_CFG_ARG_PARSER_ALL)
        #define D_CFG_ARG_PARSER_PATH_VALIDATORS   D_CFG_ARG_PARSER_ALL
    #else
        #define D_CFG_ARG_PARSER_PATH_VALIDATORS   1
    #endif
#endif  // D_CFG_ARG_PARSER_PATH_VALIDATORS

// 1.2.5
// D_CFG_ARG_PARSER_RESPONSE_FILES
//   knob: 1 compiles response files, `@file` arguments replaced by the
// arguments the file holds, which a parser still enables one by one at run
// time; 0 removes them, and `@file` is then an ordinary argument. Default 1.
#ifndef D_CFG_ARG_PARSER_RESPONSE_FILES
    #if defined(D_CFG_ARG_PARSER_ALL)
        #define D_CFG_ARG_PARSER_RESPONSE_FILES    D_CFG_ARG_PARSER_ALL
    #else
        #define D_CFG_ARG_PARSER_RESPONSE_FILES    1
    #endif
#endif  // D_CFG_ARG_PARSER_RESPONSE_FILES

// 1.2.6
// D_CFG_ARG_PARSER_SUGGESTIONS
//   knob: 1 compiles "did you mean" suggestions, which name the closest known
// option or subcommand in the message about an unexpected argument, on by
// default at run time; 0 removes them. Default 1.
#ifndef D_CFG_ARG_PARSER_SUGGESTIONS
    #if defined(D_CFG_ARG_PARSER_ALL)
        #define D_CFG_ARG_PARSER_SUGGESTIONS       D_CFG_ARG_PARSER_ALL
    #else
        #define D_CFG_ARG_PARSER_SUGGESTIONS       1
    #endif
#endif  // D_CFG_ARG_PARSER_SUGGESTIONS

// 1.2.7
// D_CFG_ARG_PARSER_TERMINAL_WIDTH
//   knob: 1 compiles the terminal-width query, with which a formatter that asks
// for it keeps help within the terminal, narrowing CLI11's fixed widths
// where the terminal is narrower; 0 removes it, and such a formatter keeps
// to its own widths and line width. Default 1.
#ifndef D_CFG_ARG_PARSER_TERMINAL_WIDTH
    #if defined(D_CFG_ARG_PARSER_ALL)
        #define D_CFG_ARG_PARSER_TERMINAL_WIDTH    D_CFG_ARG_PARSER_ALL
    #else
        #define D_CFG_ARG_PARSER_TERMINAL_WIDTH    1
    #endif
#endif  // D_CFG_ARG_PARSER_TERMINAL_WIDTH

// 1.2.8
// D_CFG_ARG_PARSER_COMPLETION
//   knob: 1 compiles the shell completion scripts (bash, zsh, fish,
// PowerShell) generated from a command tree; 0 removes them. Default 1.
#ifndef D_CFG_ARG_PARSER_COMPLETION
    #if defined(D_CFG_ARG_PARSER_ALL)
        #define D_CFG_ARG_PARSER_COMPLETION        D_CFG_ARG_PARSER_ALL
    #else
        #define D_CFG_ARG_PARSER_COMPLETION        1
    #endif
#endif  // D_CFG_ARG_PARSER_COMPLETION

// 1.2.9
// D_CFG_ARG_PARSER_WIDE_ARGV
//   knob: 1 compiles the UTF-8 recovery of `argv` from the platform's wide
// command line (CLI11's ensure_utf8), parsing a wide `argv` (`wmain`'s), and
// wide text converted both ways -- CLI11's narrow() and widen(), and, in the
// C++ face, values of the types made from a std::wstring; 0 removes them.
// Default 1.
#ifndef D_CFG_ARG_PARSER_WIDE_ARGV
    #if defined(D_CFG_ARG_PARSER_ALL)
        #define D_CFG_ARG_PARSER_WIDE_ARGV         D_CFG_ARG_PARSER_ALL
    #else
        #define D_CFG_ARG_PARSER_WIDE_ARGV         1
    #endif
#endif  // D_CFG_ARG_PARSER_WIDE_ARGV

// 1.3    Run-time defaults
//------------------------------------------------------------------------------
// 1.3.1
// D_CFG_ARG_PARSER_WINDOWS_STYLE
//   knob: whether a new parser accepts `/name` and `/name:value`: 0 never, 1
// always, 2 where it is the platform's syntax (D_ENV_ARG_PARSER_WINDOWS_
// STYLE_NATIVE), as CLI11 does. Every command can still change its own
// setting. Default 2.
#ifndef D_CFG_ARG_PARSER_WINDOWS_STYLE
    #define D_CFG_ARG_PARSER_WINDOWS_STYLE         2
#endif  // D_CFG_ARG_PARSER_WINDOWS_STYLE

// 1.3.2
// D_CFG_ARG_PARSER_INTEGER_RULE
//   knob: how a new parser reads an integer with leading zeros: 0 as decimal,
// so `010` is ten, with `0x`, `0o` and `0b` naming the other bases; 1 as C
// and CLI11 do, so `010` is eight and `08` is an error. Every parser and
// option can still choose its own rule (decision 7 of the arg_parser
// register). Default 0.
#ifndef D_CFG_ARG_PARSER_INTEGER_RULE
    #define D_CFG_ARG_PARSER_INTEGER_RULE          0
#endif  // D_CFG_ARG_PARSER_INTEGER_RULE

// 1.4    Sizes
//------------------------------------------------------------------------------
// 1.4.1
// D_CFG_ARG_PARSER_COLUMN_WIDTH
//   knob: the width of help's left column, the one holding names, when a
// formatter is initialized; CLI11's is 30. From 10 to 200.
#ifndef D_CFG_ARG_PARSER_COLUMN_WIDTH
    #define D_CFG_ARG_PARSER_COLUMN_WIDTH          30
#endif  // D_CFG_ARG_PARSER_COLUMN_WIDTH

// 1.4.2
// D_CFG_ARG_PARSER_RIGHT_COLUMN_WIDTH
//   knob: the width help's descriptions wrap at, right of the left column,
// when a formatter is initialized; CLI11's is 65. From 10 to 1000.
#ifndef D_CFG_ARG_PARSER_RIGHT_COLUMN_WIDTH
    #define D_CFG_ARG_PARSER_RIGHT_COLUMN_WIDTH    65
#endif  // D_CFG_ARG_PARSER_RIGHT_COLUMN_WIDTH

// 1.4.3
// D_CFG_ARG_PARSER_PARAGRAPH_WIDTH
//   knob: the width a command's description and footer wrap at when a
// formatter is initialized; CLI11's is 80. From 20 to 1000.
#ifndef D_CFG_ARG_PARSER_PARAGRAPH_WIDTH
    #define D_CFG_ARG_PARSER_PARAGRAPH_WIDTH       80
#endif  // D_CFG_ARG_PARSER_PARAGRAPH_WIDTH

// 1.4.4
// D_CFG_ARG_PARSER_SUGGESTION_DISTANCE
//   knob: the most edits -- insertions, deletions, substitutions and adjacent
// transpositions -- by which a known name may differ from an unexpected
// argument and still be suggested for it. From 1 to 8; default 2.
#ifndef D_CFG_ARG_PARSER_SUGGESTION_DISTANCE
    #define D_CFG_ARG_PARSER_SUGGESTION_DISTANCE   2
#endif  // D_CFG_ARG_PARSER_SUGGESTION_DISTANCE

// 1.4.5
// D_CFG_ARG_PARSER_RESPONSE_DEPTH
//   knob: how deeply response files may name further response files before
// the parser reports the nesting as an error, which is also what stops a file
// that names itself. From 1 to 64; default 8.
#ifndef D_CFG_ARG_PARSER_RESPONSE_DEPTH
    #define D_CFG_ARG_PARSER_RESPONSE_DEPTH        8
#endif  // D_CFG_ARG_PARSER_RESPONSE_DEPTH

// 1.5    Validation
//------------------------------------------------------------------------------
#if !D_CFG_IS_BOOL(D_CFG_ARG_PARSER_CONFIG_FILES)
    #error "cfg_arg_parser: D_CFG_ARG_PARSER_CONFIG_FILES must be 0 or 1"
#endif

#if !D_CFG_IS_BOOL(D_CFG_ARG_PARSER_ENVIRONMENT)
    #error "cfg_arg_parser: D_CFG_ARG_PARSER_ENVIRONMENT must be 0 or 1"
#endif

#if !D_CFG_IS_BOOL(D_CFG_ARG_PARSER_EXTRA_VALIDATORS)
    #error "cfg_arg_parser: D_CFG_ARG_PARSER_EXTRA_VALIDATORS must be 0 or 1"
#endif

#if !D_CFG_IS_BOOL(D_CFG_ARG_PARSER_PATH_VALIDATORS)
    #error "cfg_arg_parser: D_CFG_ARG_PARSER_PATH_VALIDATORS must be 0 or 1"
#endif

#if !D_CFG_IS_BOOL(D_CFG_ARG_PARSER_RESPONSE_FILES)
    #error "cfg_arg_parser: D_CFG_ARG_PARSER_RESPONSE_FILES must be 0 or 1"
#endif

#if !D_CFG_IS_BOOL(D_CFG_ARG_PARSER_SUGGESTIONS)
    #error "cfg_arg_parser: D_CFG_ARG_PARSER_SUGGESTIONS must be 0 or 1"
#endif

#if !D_CFG_IS_BOOL(D_CFG_ARG_PARSER_TERMINAL_WIDTH)
    #error "cfg_arg_parser: D_CFG_ARG_PARSER_TERMINAL_WIDTH must be 0 or 1"
#endif

#if !D_CFG_IS_BOOL(D_CFG_ARG_PARSER_COMPLETION)
    #error "cfg_arg_parser: D_CFG_ARG_PARSER_COMPLETION must be 0 or 1"
#endif

#if !D_CFG_IS_BOOL(D_CFG_ARG_PARSER_WIDE_ARGV)
    #error "cfg_arg_parser: D_CFG_ARG_PARSER_WIDE_ARGV must be 0 or 1"
#endif

#if ( (!D_CFG_IS_INT_LITERAL(D_CFG_ARG_PARSER_WINDOWS_STYLE)) ||              \
      (D_CFG_NORM(D_CFG_ARG_PARSER_WINDOWS_STYLE) > 2) )
    #error "cfg_arg_parser: D_CFG_ARG_PARSER_WINDOWS_STYLE must be 0, 1 or 2"
#endif

#if !D_CFG_IS_BOOL(D_CFG_ARG_PARSER_INTEGER_RULE)
    #error "cfg_arg_parser: D_CFG_ARG_PARSER_INTEGER_RULE must be 0 or 1"
#endif

#if ( (D_CFG_NORM(D_CFG_ARG_PARSER_COLUMN_WIDTH) < 10) ||                     \
      (D_CFG_NORM(D_CFG_ARG_PARSER_COLUMN_WIDTH) > 200) )
    #error "cfg_arg_parser: D_CFG_ARG_PARSER_COLUMN_WIDTH must lie in [10, 200]"
#endif

#if ( (D_CFG_NORM(D_CFG_ARG_PARSER_RIGHT_COLUMN_WIDTH) < 10) ||               \
      (D_CFG_NORM(D_CFG_ARG_PARSER_RIGHT_COLUMN_WIDTH) > 1000) )
    #error "cfg_arg_parser: D_CFG_ARG_PARSER_RIGHT_COLUMN_WIDTH must lie in \
[10, 1000]"
#endif

#if ( (D_CFG_NORM(D_CFG_ARG_PARSER_PARAGRAPH_WIDTH) < 20) ||                  \
      (D_CFG_NORM(D_CFG_ARG_PARSER_PARAGRAPH_WIDTH) > 1000) )
    #error "cfg_arg_parser: D_CFG_ARG_PARSER_PARAGRAPH_WIDTH must lie in \
[20, 1000]"
#endif

#if ( (D_CFG_NORM(D_CFG_ARG_PARSER_SUGGESTION_DISTANCE) < 1) ||               \
      (D_CFG_NORM(D_CFG_ARG_PARSER_SUGGESTION_DISTANCE) > 8) )
    #error "cfg_arg_parser: D_CFG_ARG_PARSER_SUGGESTION_DISTANCE must lie in \
[1, 8]"
#endif

#if ( (D_CFG_NORM(D_CFG_ARG_PARSER_RESPONSE_DEPTH) < 1) ||                    \
      (D_CFG_NORM(D_CFG_ARG_PARSER_RESPONSE_DEPTH) > 64) )
    #error "cfg_arg_parser: D_CFG_ARG_PARSER_RESPONSE_DEPTH must lie in [1, 64]"
#endif


//==============================================================================
// 2.  RESOLVED VALUES
//==============================================================================
// What the module's headers and sources read. Nothing outside this file
// resolves them, and nothing below is meant to be set by a user.


// 2.1    Features
//------------------------------------------------------------------------------
// 2.1.1
// D_INTERNAL_ARG_PARSER_CONFIG_FILES
//   resolved: 1 when configuration files are compiled.
#define D_INTERNAL_ARG_PARSER_CONFIG_FILES                                     \
    D_CFG_NORM(D_CFG_ARG_PARSER_CONFIG_FILES)

// 2.1.2
// D_INTERNAL_ARG_PARSER_ENVIRONMENT
//   resolved: 1 when options may read environment variables.
#define D_INTERNAL_ARG_PARSER_ENVIRONMENT                                      \
    D_CFG_NORM(D_CFG_ARG_PARSER_ENVIRONMENT)

// 2.1.3
// D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS
//   resolved: 1 when the extra validators are compiled.
#define D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS                                 \
    D_CFG_NORM(D_CFG_ARG_PARSER_EXTRA_VALIDATORS)

// 2.1.4
// D_INTERNAL_ARG_PARSER_PATH_VALIDATORS
//   resolved: 1 when the file-system validators are compiled.
#define D_INTERNAL_ARG_PARSER_PATH_VALIDATORS                                  \
    D_CFG_NORM(D_CFG_ARG_PARSER_PATH_VALIDATORS)

// 2.1.5
// D_INTERNAL_ARG_PARSER_RESPONSE_FILES
//   resolved: 1 when response files are compiled.
#define D_INTERNAL_ARG_PARSER_RESPONSE_FILES                                   \
    D_CFG_NORM(D_CFG_ARG_PARSER_RESPONSE_FILES)

// 2.1.6
// D_INTERNAL_ARG_PARSER_SUGGESTIONS
//   resolved: 1 when suggestions are compiled.
#define D_INTERNAL_ARG_PARSER_SUGGESTIONS                                      \
    D_CFG_NORM(D_CFG_ARG_PARSER_SUGGESTIONS)

// 2.1.7
// D_INTERNAL_ARG_PARSER_TERMINAL_WIDTH
//   resolved: 1 when the terminal-width query is compiled.
#define D_INTERNAL_ARG_PARSER_TERMINAL_WIDTH                                   \
    D_CFG_NORM(D_CFG_ARG_PARSER_TERMINAL_WIDTH)

// 2.1.8
// D_INTERNAL_ARG_PARSER_COMPLETION
//   resolved: 1 when completion scripts are compiled.
#define D_INTERNAL_ARG_PARSER_COMPLETION                                       \
    D_CFG_NORM(D_CFG_ARG_PARSER_COMPLETION)

// 2.1.9
// D_INTERNAL_ARG_PARSER_WIDE_ARGV
//   resolved: 1 when UTF-8 recovery and wide parsing are compiled.
#define D_INTERNAL_ARG_PARSER_WIDE_ARGV                                        \
    D_CFG_NORM(D_CFG_ARG_PARSER_WIDE_ARGV)

// 2.2    Run-time defaults
//------------------------------------------------------------------------------
// 2.2.1
// D_INTERNAL_ARG_PARSER_WINDOWS_STYLE
//   resolved: 1 when a new parser accepts Windows-style options, with 2 of the
// knob read through the platform's convention.
#if (D_CFG_NORM(D_CFG_ARG_PARSER_WINDOWS_STYLE) == 2)
    #define D_INTERNAL_ARG_PARSER_WINDOWS_STYLE                                \
        D_ENV_ARG_PARSER_WINDOWS_STYLE_NATIVE
#else
    #define D_INTERNAL_ARG_PARSER_WINDOWS_STYLE                                \
        D_CFG_NORM(D_CFG_ARG_PARSER_WINDOWS_STYLE)
#endif

// 2.2.2
// D_INTERNAL_ARG_PARSER_INTEGER_RULE
//   resolved: the integer rule a new parser starts with, as an
// enum d_arg_integer_rule value (0 decimal, 1 C).
#define D_INTERNAL_ARG_PARSER_INTEGER_RULE                                     \
    D_CFG_NORM(D_CFG_ARG_PARSER_INTEGER_RULE)

// 2.3    Sizes
//------------------------------------------------------------------------------
// 2.3.1
// D_INTERNAL_ARG_PARSER_COLUMN_WIDTH
//   resolved: the left-column width a formatter starts with.
#define D_INTERNAL_ARG_PARSER_COLUMN_WIDTH                                     \
    D_CFG_NORM(D_CFG_ARG_PARSER_COLUMN_WIDTH)

// 2.3.2
// D_INTERNAL_ARG_PARSER_RIGHT_COLUMN_WIDTH
//   resolved: the description width a formatter starts with.
#define D_INTERNAL_ARG_PARSER_RIGHT_COLUMN_WIDTH                               \
    D_CFG_NORM(D_CFG_ARG_PARSER_RIGHT_COLUMN_WIDTH)

// 2.3.3
// D_INTERNAL_ARG_PARSER_PARAGRAPH_WIDTH
//   resolved: the paragraph width a formatter starts with.
#define D_INTERNAL_ARG_PARSER_PARAGRAPH_WIDTH                                  \
    D_CFG_NORM(D_CFG_ARG_PARSER_PARAGRAPH_WIDTH)

// 2.3.4
// D_INTERNAL_ARG_PARSER_SUGGESTION_DISTANCE
//   resolved: the greatest edit distance a suggestion may have.
#define D_INTERNAL_ARG_PARSER_SUGGESTION_DISTANCE                              \
    D_CFG_NORM(D_CFG_ARG_PARSER_SUGGESTION_DISTANCE)

// 2.3.5
// D_INTERNAL_ARG_PARSER_RESPONSE_DEPTH
//   resolved: the deepest nesting of response files.
#define D_INTERNAL_ARG_PARSER_RESPONSE_DEPTH                                   \
    D_CFG_NORM(D_CFG_ARG_PARSER_RESPONSE_DEPTH)

// 2.4    The C++ boundary
//------------------------------------------------------------------------------
// 2.4.1
// D_INTERNAL_ARG_PARSER_CPP_CATCH
//   resolved: 1 when the C++ face's thunks -- every function the C core calls
// back into C++ through -- must catch whatever the user's code throws and
// return it as a status. Not a knob: the guides require it wherever
// exceptions exist, so it follows D_ENV_ARG_PARSER_CPP_EXCEPTIONS.
#define D_INTERNAL_ARG_PARSER_CPP_CATCH  D_ENV_ARG_PARSER_CPP_EXCEPTIONS


#endif  // DJINTERP_CONFIG_PARSE_CLI_ARG_PARSER_CFG_ARG_PARSER_H
