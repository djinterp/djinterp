/*******************************************************************************
* djinterp [parse]                                                cli_internal.h
*
* Private helpers shared by the command-line kit's C sources.
*   What the kit asks of the operating system -- terminals, the standard
* streams, the environment, the program's own file and loadable modules --
* answered in cli_system.c, the one source that includes a system header;
* then the text the other sources share: words in lower case, and the
* wrapping help and diagnostics are written with. Not installed, and not
* part of the public interface.
*   Section 1 is defined in cli_system.c, section 2 in cli_program.c.
*
*
* path:      /src/djinterp/parse/cli/c/cli_internal.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.10.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE SYSTEM
    ----------
    1.  Terminals
         1.  d_cli_internal_is_terminal
         2.  d_cli_internal_take_sequences
         3.  d_cli_internal_terminal_columns
    2.  The standard streams
         1.  d_cli_internal_binary_streams
         2.  d_cli_internal_write_stream
    3.  The environment
         1.  d_cli_internal_getenv
         2.  d_cli_internal_main_arguments
    4.  The program's file
         1.  d_cli_internal_executable
    5.  Modules
         1.  d_cli_internal_module_open
         2.  d_cli_internal_module_entry
         3.  d_cli_internal_module_close
    6.  Errors and files
         1.  d_cli_internal_error_text
         2.  d_cli_internal_mode_kind
2.  TEXT
    ----
    1.  Copies
         1.  d_cli_internal_copy
    2.  Words
         1.  d_cli_internal_lower_first
*/

#ifndef DJINTERP_PARSE_CLI_C_CLI_INTERNAL_H
#define DJINTERP_PARSE_CLI_C_CLI_INTERNAL_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // d_arg_buffer
#include "../../../../../inc/djinterp/parse/cli/c/cli_plugin.h"  // d_cli_plugin_entry_fn
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


D_EXTERN_C_BEGIN

//==============================================================================
// 1.  THE SYSTEM
//==============================================================================


// 1.1    Terminals
//------------------------------------------------------------------------------
// 1.1.1
// d_cli_internal_is_terminal
//   query: whether a standard stream, a d_cli_stream value, is a terminal.
bool        d_cli_internal_is_terminal(int _stream);

// 1.1.2
// d_cli_internal_take_sequences
//   setting: asks the terminal a standard stream is to take ANSI sequences,
// and says whether it does: a Windows console with virtual terminal
// processing turned on, which this turns on; every POSIX terminal. False
// for a stream that is no terminal.
bool        d_cli_internal_take_sequences(int _stream);

// 1.1.3
// d_cli_internal_terminal_columns
//   query: the columns of the terminal standard output is, or 0 when it is
// none or cannot say.
size_t      d_cli_internal_terminal_columns(void);

// 1.2    The standard streams
//------------------------------------------------------------------------------
// 1.2.1
// d_cli_internal_binary_streams
//   setting: puts the three standard streams in binary mode where the C
// runtime translates line ends -- Windows -- once; nothing elsewhere.
void        d_cli_internal_binary_streams(void);

// 1.2.2
// d_cli_internal_write_stream
//   writing: writes bytes to standard output or standard error -- through
// WriteConsoleW to a Windows console, as UTF-8 converted, a sequence split
// across two writes kept for the second; as bytes, flushed, to anything
// else -- and returns how many were taken: all, or fewer on failure.
size_t      d_cli_internal_write_stream(int         _stream,
                                        const void* _data,
                                        size_t      _size);

// 1.3    The environment
//------------------------------------------------------------------------------
// 1.3.1
// d_cli_internal_getenv
//   lookup: an environment variable's value in UTF-8, in a block to be freed
// with free, through `_value`: NULL when the variable is unset. Returns 0;
// or ENOMEM when there was no room for the value -- or, on Windows, for its
// name, or a value that is no UTF-16 to convert -- `_value` then NULL too,
// so that a caller can tell a variable it could not read from one not set.
int         d_cli_internal_getenv(const char* _name,
                                  char**      _value);

// 1.3.2
// d_cli_internal_main_arguments
//   query: whether arguments are the ones the C runtime gave main -- as
// many, each the same text -- which on Windows it made from the process's
// wide command line in the ANSI code page, so that they are the ones worth
// taking again from that line in UTF-8. False for any others -- a module's,
// a test's, a list a program made -- and everywhere but Windows, where
// nothing is taken again.
bool        d_cli_internal_main_arguments(int                _argc,
                                          const char* const* _argv);

// 1.4    The program's file
//------------------------------------------------------------------------------
// 1.4.1
// d_cli_internal_executable
//   lookup: d_cli_executable_path's work: 0 and the path in a block to be
// freed with free, or an errno value.
int         d_cli_internal_executable(const char* _argv0,
                                      char**      _path);

// 1.5    Modules
//------------------------------------------------------------------------------
// 1.5.1
// d_cli_internal_module_open
//   loading: opens a module into `*_module`, every symbol resolved at once
// and its own kept to itself. D_ARG_OK; D_ARG_ERROR_FILE, `*_module` NULL
// and the system's words appended to `_why`; or D_ARG_ERROR_OUT_OF_MEMORY
// where there was no room for the name the system takes -- Windows'
// UTF-16.
int         d_cli_internal_module_open(const char*          _path,
                                       void**               _module,
                                       struct d_arg_buffer* _why);

// 1.5.2
// d_cli_internal_module_entry
//   lookup: a module's D_CLI_PLUGIN_ENTRY_NAME, or NULL.
d_cli_plugin_entry_fn
            d_cli_internal_module_entry(void* _module);

// 1.5.3
// d_cli_internal_module_close
//   loading: closes a module opened above; ignores NULL.
void        d_cli_internal_module_close(void* _module);

// 1.6    Errors and files
//------------------------------------------------------------------------------
// 1.6.1
// d_cli_internal_error_text
//   lookup: the system's words for an errno value.
const char* d_cli_internal_error_text(int _errnum);

// 1.6.2
// d_cli_internal_mode_kind
//   query: what a file's mode, as d_file_mode reports it, is: 1 a regular
// file, 2 a directory, 0 anything else.
int         d_cli_internal_mode_kind(uint32_t _mode);


//==============================================================================
// 2.  TEXT
//==============================================================================


// 2.1    Copies
//------------------------------------------------------------------------------
// 2.1.1
// d_cli_internal_copy
//   allocation: a copy of a C string, to be freed with free; NULL for NULL,
// or when there is no room.
char*       d_cli_internal_copy(const char* _text);

// 2.2    Words
//------------------------------------------------------------------------------
// 2.2.1
// d_cli_internal_lower_first
//   query: whether a message's first letter should be lowered for the GNU
// shape: an ASCII capital followed by a small letter -- `The following`, not
// `SHEET` or `--level`.
bool        d_cli_internal_lower_first(const char* _text,
                                       size_t      _length);

D_EXTERN_C_END


#endif  // DJINTERP_PARSE_CLI_C_CLI_INTERNAL_H
