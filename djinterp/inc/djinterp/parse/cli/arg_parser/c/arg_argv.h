/*******************************************************************************
* djinterp [parse]                                                    arg_argv.h
*
* Arguments from text: splitting command lines and response files, quoting
* words to survive a split, finding a program's name, and converting wide
* arguments to UTF-8.
*   d_arg_split_line splits a line by one of four sets of rules, the
* d_arg_quoting values: a POSIX shell's, without expansions; the Microsoft C
* runtime's, by which a Windows program's `argv` is made; CLI11's, which its
* parse of a string uses; and GCC's for response files. The parser uses it
* for d_arg_parser_parse_line, for response files and for recovering UTF-8
* arguments on Windows; it is public so that a program can split text the
* same way -- options held in an environment variable, say -- and
* d_arg_quote_word, its inverse, can write words that split back unchanged.
*
*
* path:      /inc/djinterp/parse/cli/arg_parser/c/arg_argv.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VOCABULARY
    ----------
    1.  Split flags
         1.  D_ARG_SPLIT_PROGRAM_NAME
         2.  D_ARG_SPLIT_WINDOWS_STYLE
         3.  D_ARG_SPLIT_LINE_BREAKS
2.  SPLITTING
    ---------
    1.  Lines
         1.  d_arg_split_line
         2.  d_arg_quote_word
    2.  Program names
         1.  d_arg_split_program_name
    3.  Response files
         1.  d_arg_read_response_file
3.  ENCODING
    --------
    1.  Wide text
         1.  d_arg_utf8_from_wide
*/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_ARGV_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_ARGV_H 1

// std
#include <stddef.h>                          // size_t, wchar_t
// djinterp
#include "../../../../c/djinterp.h"          // framework root
#include "../../../../c/util/sink_common.h"  // d_pack_text
#include "./arg_common.h"                    // d_arg_buffer,
                                             // d_arg_string_list, statuses
// re_std
#include "../../../../../re_std/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// 1.  VOCABULARY
//==============================================================================


// 1.1    Split flags
//------------------------------------------------------------------------------
//   constant: the switches d_arg_split_line takes.

// 1.1.1
// D_ARG_SPLIT_PROGRAM_NAME
//   constant: the line starts with a program's name, which the C runtime's
// rules read by a rule of their own -- quotes open and close, and nothing is
// escaped -- and always give as the first word, empty or not, as the
// runtime always gives `argv[0]`. The other rules read it as they read every
// word.
#define D_ARG_SPLIT_PROGRAM_NAME  0x00000001u

// 1.1.2
// D_ARG_SPLIT_WINDOWS_STYLE
//   constant: for CLI11's rules, `:` before a quote, after a `/name`, ends a
// word as `=` after a `-name` does, as CLI11 splits for a command that takes
// Windows-style options.
#define D_ARG_SPLIT_WINDOWS_STYLE 0x00000002u

// 1.1.3
// D_ARG_SPLIT_LINE_BREAKS
//   constant: for the C runtime's rules, a line break -- `\n` or `\r` --
// outside quotes separates words as a space does, as MSVC's tools and
// clang-cl read a response file; the runtime itself reads a command line,
// which holds none, with spaces and tabs alone. The other rules always
// separate words at line breaks.
#define D_ARG_SPLIT_LINE_BREAKS   0x00000004u


//==============================================================================
// 2.  SPLITTING
//==============================================================================


D_EXTERN_C_BEGIN

// 2.1    Lines
//------------------------------------------------------------------------------
// 2.1.1
// d_arg_split_line
/**
 * @brief Splits a line of text into arguments.
 *
 * @note By `_quoting`. D_ARG_QUOTING_POSIX: a POSIX shell's words, without
 *       expansions, comments or operators -- `;`, `|`, `&`, `<`, `>` and
 *       parentheses are ordinary -- spaces, tabs and line feeds separating,
 *       as in the shells, other whitespace being ordinary; a backslash keeps
 *       the next character, removes a newline, and, last, is itself, as
 *       dash and bash read one; single quotes keep all up to the next;
 *       double quotes keep all but a backslash before `$`, a backquote,
 *       `"`, a backslash or a newline. D_ARG_QUOTING_WINDOWS:
 *       the C runtime's -- spaces and tabs separate, and line breaks with
 *       D_ARG_SPLIT_LINE_BREAKS; 2n backslashes before a `"` give n and
 *       open or close a quote, 2n+1 give n and a `"`; other backslashes are
 *       themselves; `""` within a quote gives `"`; an unclosed quote runs
 *       to the end.
 *       D_ARG_QUOTING_CLI11: CLI11's -- whitespace separates, a word that
 *       starts with a quote or a bracket runs to its match, `=` before a
 *       quote after a `-name` ends a word, and each word then loses its
 *       outer quotes, and a double-quoted one its escapes.
 *       D_ARG_QUOTING_GNU: GCC's for response files, libiberty's
 *       buildargv -- whitespace separates, single and double quotes keep all
 *       up to their match or the end, and a backslash keeps the next
 *       character inside them as outside; text of whitespace alone holds no
 *       words, as GCC reads a response file.
 *       NATIVE and NATIVE_RESPONSE are the platform's choices.
 *
 * @param[in]     _line     the text; a NUL byte in it is an ordinary
 *                          character, though a word holding one -- or one
 *                          that CLI11's escapes `\\0` and `\\u0000` give --
 *                          reads, from the list, as ending there, where
 *                          d_arg_parser_parse_line keeps it whole.
 * @param[in]     _quoting  a d_arg_quoting value.
 * @param[in]     _flags    D_ARG_SPLIT_* switches; 0 for none.
 * @param[in,out] _out      receives the words, appended.
 * @param[in,out] _error    receives a failure's description, appended; may
 *                          be NULL.
 * @post   on failure `_out` is as it was.
 * @retval D_ARG_OK                   split.
 * @retval D_ARG_ERROR_INVALID        the text breaks the rules: a quote
 *                                    never closed for POSIX's, an escape
 *                                    CLI11 refuses for CLI11's. The other
 *                                    rules take any text.
 * @retval D_ARG_ERROR_OUT_OF_MEMORY  a word could not be stored.
 * @retval D_ARG_ERROR_BAD_ARGUMENT   `_out` is NULL, `_line` is NULL with a
 *                                    nonzero length, or `_quoting` is not a
 *                                    d_arg_quoting value.
 */
int     d_arg_split_line(struct d_pack_text        _line,
                         int                       _quoting,
                         uint32_t                  _flags,
                         struct d_arg_string_list* _out,
                         struct d_arg_buffer*      _error);

// 2.1.2
// d_arg_quote_word
/**
 * @brief Writes a word so that d_arg_split_line, by the same rules, gives it
 *        back as one argument.
 *
 * @note A word the rules would not change is written as it is -- for
 *       POSIX's, only a word of ASCII letters, digits and `@%+=:,./-_`,
 *       which no shell reads specially, as Python's shlex.quote judges, so
 *       that a real shell gives the word back too. Otherwise it is quoted:
 *       in single quotes for POSIX, each `'` written `'\''`; in
 *       double quotes for Windows, backslashes doubled before a `"` and at
 *       the end, each `"` escaped; in double quotes for CLI11, with CLI11's
 *       escapes, and a `=` or `:` that a quote would follow written
 *       `\\u003d` or `\\u003a`, so that CLI11 cannot split the word there;
 *       for GNU, each special character after a backslash. An empty word is
 *       an empty pair of quotes.
 *
 * @param[in]     _word     the word.
 * @param[in]     _quoting  a d_arg_quoting value.
 * @param[in,out] _out      receives the written word, appended.
 * @return D_ARG_OK, or the buffer's status; D_ARG_ERROR_BAD_ARGUMENT for a
 *         NULL buffer, NULL word data with a nonzero length, or a
 *         `_quoting` that is not a d_arg_quoting value.
 */
int     d_arg_quote_word(struct d_pack_text   _word,
                         int                  _quoting,
                         struct d_arg_buffer* _out);

// 2.2    Program names
//------------------------------------------------------------------------------
// 2.2.1
// d_arg_split_program_name
/**
 * @brief Finds a program's name at the start of a command line; CLI11's
 *        split_program_name.
 *
 * @note Surrounding whitespace is ignored. The name is the shortest run of
 *       the line's first words, with the spaces between them, that names
 *       something existing that is not a directory, so an unquoted path
 *       holding spaces is found whole. When no run does, a line starting
 *       with a quote gives what the quote encloses, its escaped quotes
 *       unescaped, and any other line its first word. One byte after the
 *       name is dropped -- the space after it, or, after a quoted name, the
 *       byte after the closing quote, whatever it is, as CLI11 drops it.
 *
 * @param[in]     _line  the command line.
 * @param[in,out] _name  receives the name, appended.
 * @param[out]    _rest  receives the rest of the line after the name, less
 *                       its leading whitespace: a span within `_line`.
 * @return D_ARG_OK, or the buffer's status; D_ARG_ERROR_BAD_ARGUMENT for a
 *         NULL argument, or NULL line data with a nonzero length.
 */
int     d_arg_split_program_name(struct d_pack_text   _line,
                                 struct d_arg_buffer* _name,
                                 struct d_pack_text*  _rest);

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// 2.3    Response files
//------------------------------------------------------------------------------
// 2.3.1
// d_arg_read_response_file
/**
 * @brief Reads the arguments a response file holds.
 *
 * @note The file is read whole -- its path taken as UTF-8 on Windows, or in
 *       the ANSI code page when it is not valid UTF-8 -- and split as
 *       d_arg_split_line splits it with D_ARG_SPLIT_LINE_BREAKS, line
 *       breaks parting words as the rules read them: by POSIX's, a carriage
 *       return is an ordinary byte, as a shell reads one, so that a file with
 *       Windows' line ends gives words ending in one. A byte-order mark is
 *       honoured, whatever the rules: UTF-8's is skipped, and a file starting
 *       with UTF-16's, little- or big-endian, is read as UTF-16, as MSVC
 *       reads one, an unpaired surrogate -- or an odd byte left at the end --
 *       becoming U+FFFD. A NUL byte is an ordinary character, as
 *       d_arg_split_line reads one: a word holding one reads, from the
 *       list, as ending there, where a parse reading the file keeps it whole
 *       (d_arg_parser_set_response_files).
 *
 * @param[in]     _path     the file's path.
 * @param[in]     _quoting  a d_arg_quoting value.
 * @param[in,out] _out      receives the arguments, appended.
 * @param[in,out] _error    receives a failure's description, appended; may
 *                          be NULL.
 * @post   on failure `_out` is as it was.
 * @return D_ARG_OK; D_ARG_ERROR_FILE when the file could not be read, "<path>
 *         was not readable (missing?)"; or a status of d_arg_split_line.
 */
int     d_arg_read_response_file(const char*               _path,
                                 int                       _quoting,
                                 struct d_arg_string_list* _out,
                                 struct d_arg_buffer*      _error);

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

D_EXTERN_C_END


#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

//==============================================================================
// 3.  ENCODING
//==============================================================================


D_EXTERN_C_BEGIN

// 3.1    Wide text
//------------------------------------------------------------------------------
// 3.1.1
// d_arg_utf8_from_wide
/**
 * @brief Converts wide text to UTF-8.
 *
 * @note `wchar_t` is read as UTF-16 where it has 16 bits, as on Windows, and
 *       as UTF-32 where it has more; an unpaired surrogate, or a value past
 *       U+10FFFF, becomes U+FFFD, as Windows converts one.
 *
 * @param[in]     _text    the text; may be NULL when `_length` is 0.
 * @param[in]     _length  its length, in `wchar_t` units.
 * @param[in,out] _out     receives the UTF-8, appended.
 * @return D_ARG_OK, or the buffer's status; D_ARG_ERROR_BAD_ARGUMENT for a
 *         NULL buffer, or NULL text with a nonzero length.
 */
int     d_arg_utf8_from_wide(const wchar_t*       _text,
                             size_t               _length,
                             struct d_arg_buffer* _out);

D_EXTERN_C_END

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_ARGV_H
