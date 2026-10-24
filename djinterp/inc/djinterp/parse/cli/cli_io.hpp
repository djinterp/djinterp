/*******************************************************************************
* djinterp [parse]                                                    cli_io.hpp
*
* The command-line kit's C++ face: streams, terminals and files.
*   The C calls of cli_io.h for C++: the standard streams as sinks, whether a
* stream is a terminal and how wide, whether it takes colour, and a file read
* whole or written whole -- `-` the standard stream either way -- with
* std::string for the bytes. A file call returns 0 or an errno value, as the
* C call does, for d_cli_file_error or program::file_error to report.
*
*
* path:      /inc/djinterp/parse/cli/cli_io.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.10.22
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VOCABULARY
    ----------
    1.  Streams and colour
         1.  stream
         2.  colour
2.  STREAMS AND TERMINALS
    ---------------------
    1.  Sinks
         1.  output_sink / error_sink
    2.  Terminals
         1.  is_terminal
         2.  terminal_columns
         3.  colour_wanted
3.  FILES
    -----
    1.  Names
         1.  is_standard
    2.  Reading and writing
         1.  read_file
         2.  write_file
4.  DEFINITIONS
    -----------
    1.  Streams and terminals
    2.  Files
*/

#ifndef DJINTERP_PARSE_CLI_CLI_IO_HPP
#define DJINTERP_PARSE_CLI_CLI_IO_HPP 1

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string
// djinterp
#include "../../djinterp.hpp"                // framework root
#include "../../c/util/sink_common.h"        // d_pack_sink
#include "./arg_parser/arg_common.hpp"       // c_string, text
#include "./c/cli_io.h"                      // the C calls


namespace djinterp
{
namespace parse
{
namespace cli
{

//==============================================================================
// 1.  VOCABULARY
//==============================================================================


// 1.1    Streams and colour
//------------------------------------------------------------------------------
// 1.1.1
// stream
//   enum: the d_cli_stream values: the standard streams by what they hold.
struct stream
{
    enum value
    {
        input  = D_CLI_STREAM_INPUT,
        output = D_CLI_STREAM_OUTPUT,
        error  = D_CLI_STREAM_ERROR
    };
};

// 1.1.2
// colour
//   enum: the d_cli_colour values, `--color`'s three words; `auto` being a
// keyword, its word is automatic.
struct colour
{
    enum value
    {
        automatic = D_CLI_COLOUR_AUTO,
        always    = D_CLI_COLOUR_ALWAYS,
        never     = D_CLI_COLOUR_NEVER
    };
};


//==============================================================================
// 2.  STREAMS AND TERMINALS
//==============================================================================


// 2.1    Sinks
//------------------------------------------------------------------------------
// 2.1.1
// output_sink / error_sink
//   functions: d_cli_sink_output and d_cli_sink_error: the standard output
// and standard error as sinks, written through the console's wide interface
// on a Windows console. Neither allocates or fails.
::d_pack_sink output_sink() D_NOEXCEPT;
::d_pack_sink error_sink() D_NOEXCEPT;

// 2.2    Terminals
//------------------------------------------------------------------------------
// 2.2.1
// is_terminal
//   query: d_cli_is_terminal: whether a standard stream is a terminal.
bool          is_terminal(stream::value _stream) D_NOEXCEPT;

// 2.2.2
// terminal_columns
//   query: d_cli_terminal_columns: the width of the terminal standard output
// is, in columns; `COLUMNS` where it is none; 0 where neither says.
std::size_t   terminal_columns() D_NOEXCEPT;

// 2.2.3
// colour_wanted
//   query: d_cli_colour_wanted: whether a stream takes colour under a mode --
// always, never, or, automatically, a terminal that is not `TERM=dumb` with
// `NO_COLOR` unset or empty, and one the console takes ANSI sequences for.
bool          colour_wanted(stream::value _stream,
                            colour::value _mode) D_NOEXCEPT;


//==============================================================================
// 3.  FILES
//==============================================================================


// 3.1    Names
//------------------------------------------------------------------------------
// 3.1.1
// is_standard
//   predicate: d_cli_is_standard: whether a file operand is `-`, a standard
// stream (R11).
bool          is_standard(c_string _path) D_NOEXCEPT;

// 3.2    Reading and writing
//------------------------------------------------------------------------------
// 3.2.1
// read_file
/**
 * @brief Reads a file whole, or standard input for `-`; d_cli_read_file.
 *
 * @param[in]  _path  the file's name in UTF-8, or `-`.
 * @param[out] _data  receives the bytes, replacing what it held, when they
 *                    are read; left as it was otherwise.
 * @return 0 when read, or the C call's errno value; std::bad_alloc, as
 *         std::string throws it, when the string cannot hold the bytes.
 */
int           read_file(c_string     _path,
                        std::string& _data);

// 3.2.2
// write_file
/**
 * @brief Writes a file whole or not at all, or standard output for `-`;
 *        d_cli_write_file.
 *
 * @param[in] _path  the file's name in UTF-8, or `-`.
 * @param[in] _data  the bytes; a std::string or a C string converts.
 * @return 0 when written, or the C call's errno value; the file is then as
 *         it was.
 */
int           write_file(c_string    _path,
                         const text& _data) D_NOEXCEPT;


//==============================================================================
// 4.  DEFINITIONS
//==============================================================================


// 4.1    Streams and terminals
//------------------------------------------------------------------------------
// output_sink / error_sink / is_terminal / terminal_columns / colour_wanted
//   The C calls, the enumerations as the ints they take.
inline ::d_pack_sink
output_sink() D_NOEXCEPT
{
    return d_cli_sink_output();
}

inline ::d_pack_sink
error_sink() D_NOEXCEPT
{
    return d_cli_sink_error();
}

inline bool
is_terminal(
    stream::value _stream
) D_NOEXCEPT
{
    return d_cli_is_terminal(static_cast<int>(_stream));
}

inline std::size_t
terminal_columns() D_NOEXCEPT
{
    return d_cli_terminal_columns();
}

inline bool
colour_wanted(
    stream::value _stream,
    colour::value _mode
) D_NOEXCEPT
{
    return d_cli_colour_wanted(static_cast<int>(_stream),
                               static_cast<int>(_mode));
}

// 4.2    Files
//------------------------------------------------------------------------------
// is_standard / read_file / write_file
//   The C calls; the bytes read are copied into the string, then freed,
// before the string is touched, so that a std::bad_alloc leaks nothing.
inline bool
is_standard(
    c_string _path
) D_NOEXCEPT
{
    return d_cli_is_standard(_path.get());
}

inline int
read_file(
    c_string     _path,
    std::string& _data
)
{
    char*       bytes  = D_NULLPTR;
    std::size_t length = 0u;
    const int   result = d_cli_read_file(_path.get(),
                                         &bytes,
                                         &length);

    // nothing read
    if (result != 0)
    {
        return result;
    }

    // the bytes, owned by a string before the C buffer is freed
    struct owner
    {
        char* m_bytes;

        ~owner()
        {
            d_cli_free(m_bytes);
        }
    } const held = { bytes };

    std::string(held.m_bytes,
                length).swap(_data);

    return 0;
}

inline int
write_file(
    c_string    _path,
    const text& _data
) D_NOEXCEPT
{
    return d_cli_write_file(_path.get(),
                            _data.data(),
                            _data.size());
}

}  // namespace cli
}  // namespace parse
}  // namespace djinterp


#endif  // DJINTERP_PARSE_CLI_CLI_IO_HPP
