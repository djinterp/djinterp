/*******************************************************************************
* djinterp [parse]                                                cli_io_tests.h
*
* The suite of the command-line kit's streams and files (cli_io.c): which
* names are the standard streams; the standard sinks; whether a stream is a
* terminal, and how wide; whether text written to it should be coloured, as
* `--color`, NO_COLOR and TERM decide; reading a file whole, standard input
* among them, its bytes as they are; writing a file whole or not at all --
* a new one, one replaced with its permissions kept, one a symbolic link
* leads to, a device and a FIFO written to, a directory refused, standard
* output -- and nothing left behind; and every call given nothing.
*   What the system has decides what is tested: symbolic links, FIFOs,
* devices and permission bits where the system is POSIX; the standard
* streams' redirection everywhere.
*
*
* path:      /tests/djinterp/parse/cli/c/cli_io_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.23
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_C_CLI_IO_TESTS_H
#define DJINTERP_PARSE_CLI_C_CLI_IO_TESTS_H 1

// std
#include <stdbool.h>                   // bool
#include <stddef.h>                    // size_t
// djinterp
#include "../../c/tests_section.h"     // d_tests_section


// 1.  STREAMS
bool d_tests_cli_io_streams(void);

// 2.  COLOUR
bool d_tests_cli_io_colour(void);

// 3.  COLUMNS
bool d_tests_cli_io_columns(void);

// 4.  READING
bool d_tests_cli_io_reading(void);

// 5.  WRITING
bool d_tests_cli_io_writing(void);

// 6.  WHAT NO FILE STANDS IN FOR
bool d_tests_cli_io_special(void);

// 7.  STANDARD STREAMS
bool d_tests_cli_io_standard(void);

// 8.  NULLS
bool d_tests_cli_io_nulls(void);

// the suite's sections, in the order they run
extern const struct d_tests_section d_tests_cli_io[];
extern const size_t                 d_tests_cli_io_count;


#endif  // DJINTERP_PARSE_CLI_C_CLI_IO_TESTS_H
