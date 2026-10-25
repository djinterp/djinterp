/*******************************************************************************
* djinterp [env]                                                env_arg_parser.h
*
* Environment detection for the command-line argument parser.
*   What the platform decides for a command line: whether `/option` is the
* native option syntax, which quoting rules its shells and tools use, which
* character separates path components, whether the system's own text is
* wide, where the true command line can be read from, how environment
* variables are read and files opened without a deprecation diagnostic, how
* a file's status is asked for, how the terminal's width is asked for, how
* the C runtime turns a `long double` into text and back, and, under C++,
* whether exceptions are enabled -- the fact the C++ face needs to keep
* every call from C into C++ from throwing.
*   It includes no system header. Each fact is derived from env.h's OS and
* compiler classification -- and, on Windows, env_windows.h's toolchain
* classification -- is #ifndef-guarded so a build may define it first, and
* is read only by cfg_arg_parser.h and the module's sources and C++
* headers.
*
*
* path:      /inc/djinterp/env/parse/cli/arg_parser/env_arg_parser.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.10
*                                                            revised: 2026.10.17
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  PLATFORM CONVENTIONS
    --------------------
    1.  Option syntax
         1.  D_ENV_ARG_PARSER_WINDOWS_STYLE_NATIVE
    2.  Quoting
         1.  Quoting identifiers
              1.  D_ENV_ARG_PARSER_QUOTING_POSIX
              2.  D_ENV_ARG_PARSER_QUOTING_WINDOWS
         2.  D_ENV_ARG_PARSER_QUOTING
    3.  Paths
         1.  D_ENV_ARG_PARSER_PATH_SEPARATOR
         2.  D_ENV_ARG_PARSER_PATH_SEPARATORS
         3.  D_ENV_ARG_PARSER_WIDE_NATIVE
2.  PROCESS INTERFACE
    -----------------
    1.  Command line
         1.  Command-line identifiers
              1.  D_ENV_ARG_PARSER_COMMAND_LINE_ARGV
              2.  D_ENV_ARG_PARSER_COMMAND_LINE_WINDOWS
         2.  D_ENV_ARG_PARSER_COMMAND_LINE
    2.  Environment variables
         1.  Environment identifiers
              1.  D_ENV_ARG_PARSER_GETENV_STANDARD
              2.  D_ENV_ARG_PARSER_GETENV_DUPENV
         2.  D_ENV_ARG_PARSER_GETENV
    3.  Terminal
         1.  Terminal identifiers
              1.  D_ENV_ARG_PARSER_TERMINAL_NONE
              2.  D_ENV_ARG_PARSER_TERMINAL_POSIX
              3.  D_ENV_ARG_PARSER_TERMINAL_WINDOWS
         2.  D_ENV_ARG_PARSER_TERMINAL
    4.  Files
         1.  File-status identifiers
              1.  D_ENV_ARG_PARSER_STAT_NONE
              2.  D_ENV_ARG_PARSER_STAT_POSIX
              3.  D_ENV_ARG_PARSER_STAT_WINDOWS
         2.  D_ENV_ARG_PARSER_STAT
         3.  File-opening identifiers
              1.  D_ENV_ARG_PARSER_FOPEN_STANDARD
              2.  D_ENV_ARG_PARSER_FOPEN_SECURE
         4.  D_ENV_ARG_PARSER_FOPEN
3.  C RUNTIME
    ---------
    1.  Long double text
         1.  Long-double identifiers
              1.  D_ENV_ARG_PARSER_LONG_DOUBLE_STANDARD
              2.  D_ENV_ARG_PARSER_LONG_DOUBLE_MINGW
         2.  D_ENV_ARG_PARSER_LONG_DOUBLE
4.  LANGUAGE
    --------
    1.  C++
         1.  D_ENV_ARG_PARSER_CPP_EXCEPTIONS
*/

#ifndef DJINTERP_ENV_PARSE_CLI_ARG_PARSER_ENV_ARG_PARSER_H
#define DJINTERP_ENV_PARSE_CLI_ARG_PARSER_ENV_ARG_PARSER_H 1

// djinterp
#include "../../../env.h"  // D_ENV_OS_ID, D_ENV_IS_OS_*, D_ENV_LANG_USING_CPP,
                           // D_ENV_COMPILER_MSVC_FAMILY, D_ENV_C_HAS_UNISTD_H,
                           // D_ENV_C_HAS_SYS_STAT_H
#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
#include "../../../os/env_windows.h"  // D_ENV_WIN_IS_MINGW
#endif


//==============================================================================
// 1.  PLATFORM CONVENTIONS
//==============================================================================
// Conventions a command line follows on the build's platform. Each one only
// chooses a default: the parser's run-time settings can still choose either
// convention on any platform.


// 1.1    Option syntax
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_ARG_PARSER_WINDOWS_STYLE_NATIVE
//   detection: 1 where `/name` and `/name:value` are the platform's own option
// syntax -- Windows -- and 0 elsewhere. cfg_arg_parser.h makes it the default
// of a parser's allow-Windows-style setting, as CLI11 does with `_WIN32`.
#ifndef D_ENV_ARG_PARSER_WINDOWS_STYLE_NATIVE
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_ARG_PARSER_WINDOWS_STYLE_NATIVE 1
    #else
        #define D_ENV_ARG_PARSER_WINDOWS_STYLE_NATIVE 0
    #endif
#endif  // D_ENV_ARG_PARSER_WINDOWS_STYLE_NATIVE

// 1.2    Quoting
//------------------------------------------------------------------------------
// 1.2.1
// Quoting identifiers
//   constant: the two families of rules by which a line of text becomes a list
// of arguments, for D_ENV_ARG_PARSER_QUOTING.

// 1.2.1.1
// D_ENV_ARG_PARSER_QUOTING_POSIX
//   constant: the POSIX shell's rules, as GCC reads a response file: single
// quotes are literal, double quotes and backslashes escape.
#define D_ENV_ARG_PARSER_QUOTING_POSIX   0

// 1.2.1.2
// D_ENV_ARG_PARSER_QUOTING_WINDOWS
//   constant: the Microsoft C runtime's rules, as cl reads a response file and
// as the runtime builds argv: only double quotes group, and backslashes escape
// only before a double quote.
#define D_ENV_ARG_PARSER_QUOTING_WINDOWS 1

// 1.2.2
// D_ENV_ARG_PARSER_QUOTING
//   detection: the platform's quoting rules: the Windows runtime's on Windows,
// the POSIX shell's elsewhere. The default for reading response files.
#ifndef D_ENV_ARG_PARSER_QUOTING
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_ARG_PARSER_QUOTING D_ENV_ARG_PARSER_QUOTING_WINDOWS
    #else
        #define D_ENV_ARG_PARSER_QUOTING D_ENV_ARG_PARSER_QUOTING_POSIX
    #endif
#endif  // D_ENV_ARG_PARSER_QUOTING

// 1.3    Paths
//------------------------------------------------------------------------------
// 1.3.1
// D_ENV_ARG_PARSER_PATH_SEPARATOR
//   detection: the character that joins a directory and a file name, as the
// file-on-default-path validator joins them: `\` on Windows and MS-DOS, `/`
// elsewhere.
#ifndef D_ENV_ARG_PARSER_PATH_SEPARATOR
    #if ( (D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)) ||                                \
          (D_ENV_IS_OS_MSDOS(D_ENV_OS_ID)) )
        #define D_ENV_ARG_PARSER_PATH_SEPARATOR '\\'
    #else
        #define D_ENV_ARG_PARSER_PATH_SEPARATOR '/'
    #endif
#endif  // D_ENV_ARG_PARSER_PATH_SEPARATOR

// 1.3.2
// D_ENV_ARG_PARSER_PATH_SEPARATORS
//   detection: every character that ends a directory name in a path, as a
// string: both slashes on Windows and MS-DOS, where either is accepted, and
// `/` elsewhere.
#ifndef D_ENV_ARG_PARSER_PATH_SEPARATORS
    #if ( (D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)) ||                                \
          (D_ENV_IS_OS_MSDOS(D_ENV_OS_ID)) )
        #define D_ENV_ARG_PARSER_PATH_SEPARATORS "\\/"
    #else
        #define D_ENV_ARG_PARSER_PATH_SEPARATORS "/"
    #endif
#endif  // D_ENV_ARG_PARSER_PATH_SEPARATORS

// 1.3.3
// D_ENV_ARG_PARSER_WIDE_NATIVE
//   detection: 1 where the system's own text is wide -- Windows, whose calls
// take UTF-16, as its std::filesystem::path holds it -- and 0 elsewhere. The
// C++ face then makes a value of a type made from both a std::string and a
// std::wstring from the wide one, its UTF-8 converted exactly, as CLI11
// makes it under MSVC.
#ifndef D_ENV_ARG_PARSER_WIDE_NATIVE
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_ARG_PARSER_WIDE_NATIVE 1
    #else
        #define D_ENV_ARG_PARSER_WIDE_NATIVE 0
    #endif
#endif  // D_ENV_ARG_PARSER_WIDE_NATIVE


//==============================================================================
// 2.  PROCESS INTERFACE
//==============================================================================
// How the running process reaches its command line, its environment, the
// terminal it writes to, and its files. Each selection names an
// implementation the module's sources contain; where none applies, the module
// still builds, and the feature reports that it is unavailable.


// 2.1    Command line
//------------------------------------------------------------------------------
// 2.1.1
// Command-line identifiers
//   constant: where the arguments as the user typed them can be read from,
// for D_ENV_ARG_PARSER_COMMAND_LINE.

// 2.1.1.1
// D_ENV_ARG_PARSER_COMMAND_LINE_ARGV
//   constant: only from `argv`, which is already the user's text: on POSIX
// systems the shell passes bytes through unchanged, and they are UTF-8 by
// convention.
#define D_ENV_ARG_PARSER_COMMAND_LINE_ARGV    0

// 2.1.1.2
// D_ENV_ARG_PARSER_COMMAND_LINE_WINDOWS
//   constant: from GetCommandLineW, the UTF-16 command line Windows keeps for
// every process; the narrow `argv` holds it converted to the active code page,
// which loses whatever that page cannot represent.
#define D_ENV_ARG_PARSER_COMMAND_LINE_WINDOWS 1

// 2.1.2
// D_ENV_ARG_PARSER_COMMAND_LINE
//   detection: Windows' wide command line on Windows, `argv` alone elsewhere.
// The UTF-8 recovery of `argv` (CLI11's ensure_utf8) does work only under the
// first; under the second it returns `argv` unchanged.
#ifndef D_ENV_ARG_PARSER_COMMAND_LINE
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_ARG_PARSER_COMMAND_LINE                                  \
            D_ENV_ARG_PARSER_COMMAND_LINE_WINDOWS
    #else
        #define D_ENV_ARG_PARSER_COMMAND_LINE                                  \
            D_ENV_ARG_PARSER_COMMAND_LINE_ARGV
    #endif
#endif  // D_ENV_ARG_PARSER_COMMAND_LINE

// 2.2    Environment variables
//------------------------------------------------------------------------------
// 2.2.1
// Environment identifiers
//   constant: how a variable's value is read, for D_ENV_ARG_PARSER_GETENV.

// 2.2.1.1
// D_ENV_ARG_PARSER_GETENV_STANDARD
//   constant: through ISO C's getenv.
#define D_ENV_ARG_PARSER_GETENV_STANDARD 0

// 2.2.1.2
// D_ENV_ARG_PARSER_GETENV_DUPENV
//   constant: through the Microsoft runtime's _dupenv_s, which copies the value
// into storage the caller frees. That runtime marks getenv deprecated, and
// the guides require a build without warnings.
#define D_ENV_ARG_PARSER_GETENV_DUPENV   1

// 2.2.2
// D_ENV_ARG_PARSER_GETENV
//   detection: _dupenv_s under the Microsoft toolchain's runtime (cl,
// clang-cl), getenv elsewhere, MinGW included.
#ifndef D_ENV_ARG_PARSER_GETENV
    #if D_ENV_COMPILER_MSVC_FAMILY
        #define D_ENV_ARG_PARSER_GETENV D_ENV_ARG_PARSER_GETENV_DUPENV
    #else
        #define D_ENV_ARG_PARSER_GETENV D_ENV_ARG_PARSER_GETENV_STANDARD
    #endif
#endif  // D_ENV_ARG_PARSER_GETENV

// 2.3    Terminal
//------------------------------------------------------------------------------
// 2.3.1
// Terminal identifiers
//   constant: how the width of the terminal standard output is shown on can
// be asked, for D_ENV_ARG_PARSER_TERMINAL.

// 2.3.1.1
// D_ENV_ARG_PARSER_TERMINAL_NONE
//   constant: it cannot; only the COLUMNS variable is consulted.
#define D_ENV_ARG_PARSER_TERMINAL_NONE    0

// 2.3.1.2
// D_ENV_ARG_PARSER_TERMINAL_POSIX
//   constant: through isatty and the TIOCGWINSZ request to ioctl, which every
// POSIX-like system here provides although POSIX does not specify it.
#define D_ENV_ARG_PARSER_TERMINAL_POSIX   1

// 2.3.1.3
// D_ENV_ARG_PARSER_TERMINAL_WINDOWS
//   constant: through GetConsoleScreenBufferInfo on the standard output's
// console handle.
#define D_ENV_ARG_PARSER_TERMINAL_WINDOWS 2

// 2.3.2
// D_ENV_ARG_PARSER_TERMINAL
//   detection: the console API on Windows, the POSIX interface where env.h
// found <unistd.h>, and neither elsewhere.
#ifndef D_ENV_ARG_PARSER_TERMINAL
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_ARG_PARSER_TERMINAL D_ENV_ARG_PARSER_TERMINAL_WINDOWS
    #elif D_ENV_C_HAS_UNISTD_H
        #define D_ENV_ARG_PARSER_TERMINAL D_ENV_ARG_PARSER_TERMINAL_POSIX
    #else
        #define D_ENV_ARG_PARSER_TERMINAL D_ENV_ARG_PARSER_TERMINAL_NONE
    #endif
#endif  // D_ENV_ARG_PARSER_TERMINAL

// 2.4    Files
//------------------------------------------------------------------------------
// 2.4.1
// File-status identifiers
//   constant: how whether a path exists, what it is, how large it is and who
// may read, write or execute it is asked, for D_ENV_ARG_PARSER_STAT.

// 2.4.1.1
// D_ENV_ARG_PARSER_STAT_NONE
//   constant: it cannot be; a path is a file when it can be opened for
// reading, and nothing else is known of it.
#define D_ENV_ARG_PARSER_STAT_NONE    0

// 2.4.1.2
// D_ENV_ARG_PARSER_STAT_POSIX
//   constant: through stat from <sys/stat.h>, which follows symbolic links.
#define D_ENV_ARG_PARSER_STAT_POSIX   1

// 2.4.1.3
// D_ENV_ARG_PARSER_STAT_WINDOWS
//   constant: through the Microsoft runtime's _wstat64, a UTF-8 path
// converted to UTF-16 as CLI11 widens one, and _stat64 for a path that is not
// UTF-8, which the runtime reads in the ANSI code page.
#define D_ENV_ARG_PARSER_STAT_WINDOWS 2

// 2.4.2
// D_ENV_ARG_PARSER_STAT
//   detection: the wide runtime on Windows, stat where env_c_lib.h found
// <sys/stat.h>, and neither elsewhere.
#ifndef D_ENV_ARG_PARSER_STAT
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_ARG_PARSER_STAT D_ENV_ARG_PARSER_STAT_WINDOWS
    #elif D_ENV_C_HAS_SYS_STAT_H
        #define D_ENV_ARG_PARSER_STAT D_ENV_ARG_PARSER_STAT_POSIX
    #else
        #define D_ENV_ARG_PARSER_STAT D_ENV_ARG_PARSER_STAT_NONE
    #endif
#endif  // D_ENV_ARG_PARSER_STAT

// 2.4.3
// File-opening identifiers
//   constant: how a file is opened for reading, for D_ENV_ARG_PARSER_FOPEN.

// 2.4.3.1
// D_ENV_ARG_PARSER_FOPEN_STANDARD
//   constant: through ISO C's fopen, and, on Windows, the runtime's _wfopen
// for a UTF-8 path.
#define D_ENV_ARG_PARSER_FOPEN_STANDARD 0

// 2.4.3.2
// D_ENV_ARG_PARSER_FOPEN_SECURE
//   constant: through the Microsoft runtime's fopen_s and _wfopen_s. That
// runtime marks fopen and _wfopen deprecated, and the guides require a build
// without warnings.
#define D_ENV_ARG_PARSER_FOPEN_SECURE   1

// 2.4.4
// D_ENV_ARG_PARSER_FOPEN
//   detection: the secure functions under the Microsoft toolchain's runtime
// (cl, clang-cl), the standard ones elsewhere, MinGW included -- as
// D_ENV_ARG_PARSER_GETENV chooses.
#ifndef D_ENV_ARG_PARSER_FOPEN
    #if D_ENV_COMPILER_MSVC_FAMILY
        #define D_ENV_ARG_PARSER_FOPEN D_ENV_ARG_PARSER_FOPEN_SECURE
    #else
        #define D_ENV_ARG_PARSER_FOPEN D_ENV_ARG_PARSER_FOPEN_STANDARD
    #endif
#endif  // D_ENV_ARG_PARSER_FOPEN


//==============================================================================
// 3.  C RUNTIME
//==============================================================================
// Where the C runtime the build links falls short of what ISO C promises, in
// the few places the module depends on.


// 3.1    Long double text
//------------------------------------------------------------------------------
// 3.1.1
// Long-double identifiers
//   constant: how a `long double` is written as text, for
// D_ENV_ARG_PARSER_LONG_DOUBLE.

// 3.1.1.1
// D_ENV_ARG_PARSER_LONG_DOUBLE_STANDARD
//   constant: through snprintf's `%Lg`, as ISO C specifies.
#define D_ENV_ARG_PARSER_LONG_DOUBLE_STANDARD 0

// 3.1.1.2
// D_ENV_ARG_PARSER_LONG_DOUBLE_MINGW
//   constant: through MinGW-w64's own __mingw_snprintf. MinGW's `long double`
// is the 80-bit x87 type, but the Microsoft runtime it links -- msvcrt or the
// UCRT -- formats `%Lg` as a 64-bit double and so prints garbage; MinGW-w64's
// own formatter, which its strtold already is, reads the full type.
#define D_ENV_ARG_PARSER_LONG_DOUBLE_MINGW    1

// 3.1.2
// D_ENV_ARG_PARSER_LONG_DOUBLE
//   detection: MinGW-w64's formatter under MinGW, the standard one elsewhere,
// including the Microsoft toolchain, whose `long double` is a double.
#ifndef D_ENV_ARG_PARSER_LONG_DOUBLE
    #if ( (D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)) &&                                \
          (D_ENV_WIN_IS_MINGW) )
        #define D_ENV_ARG_PARSER_LONG_DOUBLE                                   \
            D_ENV_ARG_PARSER_LONG_DOUBLE_MINGW
    #else
        #define D_ENV_ARG_PARSER_LONG_DOUBLE                                   \
            D_ENV_ARG_PARSER_LONG_DOUBLE_STANDARD
    #endif
#endif  // D_ENV_ARG_PARSER_LONG_DOUBLE


//==============================================================================
// 4.  LANGUAGE
//==============================================================================
// Facts about the language the including unit is compiled as.


// 4.1    C++
//------------------------------------------------------------------------------
// 4.1.1
// D_ENV_ARG_PARSER_CPP_EXCEPTIONS
//   detection: 1 in a C++ build with exceptions enabled, 0 in C and in a C++
// build without them (-fno-exceptions, /EHs-). The C++ face catches
// everything at each entry from the C core only when it is 1, since a try
// block does not compile without exceptions, and an exception crossing a C
// frame is undefined.
#ifndef D_ENV_ARG_PARSER_CPP_EXCEPTIONS
    #if ( (D_ENV_LANG_USING_CPP)                &&                             \
          ( (defined(__cpp_exceptions)) ||                                     \
            (defined(__EXCEPTIONS))     ||                                     \
            (defined(_CPPUNWIND)) ) )
        #define D_ENV_ARG_PARSER_CPP_EXCEPTIONS 1
    #else
        #define D_ENV_ARG_PARSER_CPP_EXCEPTIONS 0
    #endif
#endif  // D_ENV_ARG_PARSER_CPP_EXCEPTIONS


#endif  // DJINTERP_ENV_PARSE_CLI_ARG_PARSER_ENV_ARG_PARSER_H
