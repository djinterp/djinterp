/*******************************************************************************
* djinterp [parse]                                                      cli_io.h
*
* The command-line kit's streams, terminals and files.
*   A program's result goes to standard output and everything said about it
* to standard error (GRAMMAR.md, R17), and both through the sinks here: on a
* Windows console, text is written through the console's wide interface so
* that it reads correctly whatever the code page; elsewhere, and redirected,
* it is UTF-8 bytes, its lines ended by LF on every system (R18) -- the
* standard streams are put in binary mode where the C runtime would
* translate them.
*   A stream is asked whether it is a terminal, how wide it is, and whether
* it should be coloured: `--color`'s mode, then NO_COLOR, TERM=dumb and the
* terminal itself, which on Windows is asked to take ANSI sequences first.
*   Files are named in UTF-8 on every system and opened through djinterp's
* filesys, which converts a name for Windows' wide calls; `-` is the
* standard stream the call reads or writes (R11). A file is read whole, and
* written whole or not at all (R20): into a new file beside it, flushed to
* the disk, then renamed over it, keeping the old file's permissions and
* writing through a symbolic link to where it points. What no file can stand
* in for -- a device such as /dev/null, a pipe, a socket -- is written to.
*
*
* path:      /inc/djinterp/parse/cli/c/cli_io.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.10.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  STREAMS
    -------
    1.  Identifiers
         1.  d_cli_stream
    2.  Colour modes
         1.  d_cli_colour
    3.  The standard sinks
         1.  d_cli_sink_output / d_cli_sink_error
    4.  Terminals
         1.  d_cli_is_terminal
         2.  d_cli_terminal_columns
         3.  d_cli_colour_wanted
2.  FILES
    -----
    1.  Names
         1.  d_cli_is_standard
    2.  Reading
         1.  d_cli_read_file
    3.  Writing
         1.  d_cli_write_file
    4.  Memory
         1.  d_cli_free
*/

#ifndef DJINTERP_PARSE_CLI_C_CLI_IO_H
#define DJINTERP_PARSE_CLI_C_CLI_IO_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../c/djinterp.h"                    // framework root
#include "../../../c/util/sink_common.h"            // d_pack_sink
#include "../../../config/parse/cli/cfg_cli.h"      // D_INTERNAL_CLI_*


//==============================================================================
// 1.  STREAMS
//==============================================================================


// 1.1    Identifiers
//------------------------------------------------------------------------------
// 1.1.1
// d_cli_stream
//   enum: the three standard streams, numbered as their descriptors are.
enum d_cli_stream
{
    D_CLI_STREAM_INPUT  = 0,
    D_CLI_STREAM_OUTPUT = 1,
    D_CLI_STREAM_ERROR  = 2
};

// 1.2    Colour modes
//------------------------------------------------------------------------------
// 1.2.1
// d_cli_colour
//   enum: `--color`'s three words (GRAMMAR.md, section 6).
enum d_cli_colour
{
    D_CLI_COLOUR_AUTO   = 0,
    D_CLI_COLOUR_ALWAYS = 1,
    D_CLI_COLOUR_NEVER  = 2
};


D_EXTERN_C_BEGIN

// 1.3    The standard sinks
//------------------------------------------------------------------------------
// 1.3.1
// d_cli_sink_output / d_cli_sink_error
//   sinks: standard output and standard error, as the kit writes them -- a
// Windows console through WriteConsoleW, anything else as bytes -- each
// accepting every byte or reporting the short write a refusal is. The first
// use puts the stream in binary mode where the C runtime translates line
// ends. Neither fails to be made.
struct d_pack_sink  d_cli_sink_output(void);
struct d_pack_sink  d_cli_sink_error(void);

// 1.4    Terminals
//------------------------------------------------------------------------------
// 1.4.1
// d_cli_is_terminal
//   query: whether a standard stream, a d_cli_stream value, is a terminal --
// a Windows console, or what isatty says elsewhere; false for any other
// value, and everywhere D_ENV_CLI_CONSOLE is NONE. Does not fail.
bool                d_cli_is_terminal(int _stream);

// 1.4.2
// d_cli_terminal_columns
//   query: how many columns the terminal standard output is shown on has,
// when standard output is one and it can be asked; COLUMNS, when it holds a
// number from 1 to 10000, otherwise; and 0 when neither says. Does not fail.
size_t              d_cli_terminal_columns(void);

/**
 * @brief Decides whether text written to a standard stream should carry
 *        colour.
 *
 * @note `D_CLI_COLOUR_ALWAYS` is yes and `D_CLI_COLOUR_NEVER` no, whatever
 *       else holds. `D_CLI_COLOUR_AUTO` is yes only when the stream is a
 *       terminal, TERM is not `dumb`, and NO_COLOR is unset or empty
 *       (no-color.org) -- and, on Windows, the console takes ANSI sequences
 *       once asked to, which this call asks. Without D_INTERNAL_CLI_COLOUR
 *       the answer is always no.
 *
 * @param[in] _stream  a d_cli_stream value.
 * @param[in] _mode    a d_cli_colour value; any other is read as
 *                     `D_CLI_COLOUR_AUTO`.
 * @return `true` if the stream should be coloured.
 */
bool                d_cli_colour_wanted(int _stream,
                                        int _mode);


//==============================================================================
// 2.  FILES
//==============================================================================


// 2.1    Names
//------------------------------------------------------------------------------
// 2.1.1
// d_cli_is_standard
//   query: whether a name is `-`, a standard stream (GRAMMAR.md, R11); false
// for NULL. Does not fail.
bool                d_cli_is_standard(const char* _path);

// 2.2    Reading
//------------------------------------------------------------------------------
// 2.2.1
// d_cli_read_file
/**
 * @brief Reads a file, or standard input, whole.
 *
 * @note The text is the file's bytes, unconverted, followed by a NUL that
 *       `*_length` does not count, so that it may be used as a C string when
 *       it holds no NUL of its own. Standard input is read to its end.
 *
 * @param[in]  _path    the file's name in UTF-8, or `-` for standard input.
 * @param[out] _data    receives the bytes, to be freed with d_cli_free; NULL
 *                      on failure.
 * @param[out] _length  receives how many bytes were read; 0 on failure.
 * @return 0 when read, or an errno value: EINVAL for a NULL argument, ENOMEM
 *         when the bytes do not fit in memory, EISDIR for a directory, and
 *         whatever opening or reading the file set.
 */
int                 d_cli_read_file(const char* _path,
                                    char**      _data,
                                    size_t*     _length);

// 2.3    Writing
//------------------------------------------------------------------------------
// 2.3.1
// d_cli_write_file
/**
 * @brief Writes a file whole or not at all, or writes standard output.
 *
 * @note A file is written into a new one beside it, named after it and
 *       `.~` and six characters, flushed to the disk and renamed over it,
 *       so that a reader sees the old file or the new one and never a part
 *       of either; a failure removes the new file and leaves the old one as
 *       it was. A file that already existed keeps its permissions; a new
 *       one has those filesys gives a file it writes
 *       (D_CFG_FILE_WRITE_CREATE_MODE), under the umask. A name that is a
 *       symbolic link is written where the link points, and the link stays;
 *       a link that leads nowhere is replaced by the file, and one that
 *       cannot be followed for another reason fails the write with it.
 *       A device, a pipe or a socket -- `-o /dev/null`, `-o /dev/stdout` --
 *       is opened and written to, as nothing can be renamed over it; that
 *       write is not whole-or-nothing, as none can be.
 *
 * @param[in] _path    the file's name in UTF-8, or `-` for standard output,
 *                     which is written through d_cli_sink_output.
 * @param[in] _data    the bytes; may be NULL when `_length` is 0.
 * @param[in] _length  how many bytes.
 * @return 0 when written, or an errno value: EINVAL for a NULL path or NULL
 *         data with a length, EISDIR for a directory, EIO when standard
 *         output refused the bytes, and whatever following a link, opening,
 *         creating, writing, flushing or renaming the file set -- ENOMEM
 *         among them.
 */
int                 d_cli_write_file(const char* _path,
                                     const void* _data,
                                     size_t      _length);

// 2.4    Memory
//------------------------------------------------------------------------------
// 2.4.1
// d_cli_free
//   lifetime: frees what d_cli_read_file and d_cli_executable_path return,
// with the allocator that made it -- which, across a plugin's boundary on
// Windows, is not necessarily the caller's. Ignores NULL.
void                d_cli_free(void* _memory);

D_EXTERN_C_END


#endif  // DJINTERP_PARSE_CLI_C_CLI_IO_H
