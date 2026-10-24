/*******************************************************************************
* djinterp [parse]                                           cli_tests_support.h
*
* What the command-line kit's C suites share: a program made fresh for each
* case, what it writes captured, and what a case came to compared with what
* it should have come to, every difference said.
*   A case is a command line and what running it must come to: the status
* the program exits with, and its standard output and standard error,
* whole. A fixture makes the program a section's cases run: the kit's
* program over a parser made for the case, and the program's own commands
* and options. The command line is split at spaces, a pair of single quotes
* keeping what is between them together as a shell keeps it, so that a table
* of cases reads as the lines a user types; its first word is the program's
* `argv[0]`.
*   Every program a case makes writes into the case's buffers, never to the
* standard streams, and wraps its help to 80 columns, so that what a case
* comes to is the same text wherever the suite runs: a terminal or not,
* however wide. The runner sets NO_COLOR before the first section, so that
* `--color`'s `auto` colours nothing even on a terminal; a case that wants
* colour asks for it with `--color=always`.
*   A check says what failed, under the case's tag, and returns whether it
* held, so that a section can make every check and look once.
*
*
* path:      /tests/djinterp/parse/cli/c/cli_tests_support.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  RECORDS
    -------
    1.  Limits
         1.  D_TESTS_CLI_WORDS_MAX
         2.  D_TESTS_CLI_LINE_MAX
         3.  D_TESTS_CLI_WIDTH
    2.  Command lines
         1.  d_tests_cli_words
    3.  Cases
         1.  d_tests_cli_make_fn
         2.  d_tests_cli_case
2.  OPERATIONS
    ----------
    1.  Command lines
         1.  d_tests_cli_split
    2.  Programs
         1.  d_tests_cli_start
         2.  d_tests_cli_finish
    3.  Cases
         1.  d_tests_cli_case_run
         2.  d_tests_cli_cases_run
    4.  Checks
         1.  d_tests_cli_same
         2.  d_tests_cli_check
    5.  The environment
         1.  d_tests_cli_setenv
    6.  A command's work
         1.  d_tests_cli_print
         2.  d_tests_cli_values
    7.  Files
         1.  d_tests_cli_file_read
         2.  d_tests_cli_file_write
         3.  d_tests_cli_file_holds
    8.  The standard streams
         1.  d_tests_cli_redirect
         2.  d_tests_cli_restore
*/

#ifndef DJINTERP_PARSE_CLI_C_CLI_TESTS_SUPPORT_H
#define DJINTERP_PARSE_CLI_C_CLI_TESTS_SUPPORT_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/env/c/env_vendor_attributes.h"  // D_FORMAT_PRINTF
#include "../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // d_arg_command
#include "../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // d_arg_buffer
#include "../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // d_arg_parser
#include "../../../../../inc/djinterp/parse/cli/c/cli_program.h"  // d_cli_program


//==============================================================================
// 1.  RECORDS
//==============================================================================


// 1.1    Limits
//------------------------------------------------------------------------------
// 1.1.1
// D_TESTS_CLI_WORDS_MAX
//   constant: the most words one command line splits into; a case with more
// is a case to shorten.
#define D_TESTS_CLI_WORDS_MAX 32

// 1.1.2
// D_TESTS_CLI_LINE_MAX
//   constant: the longest command line a case gives, in bytes.
#define D_TESTS_CLI_LINE_MAX  1024

// 1.1.3
// D_TESTS_CLI_WIDTH
//   constant: the columns every program a case makes wraps its help to.
#define D_TESTS_CLI_WIDTH     80

// 1.2    Command lines
//------------------------------------------------------------------------------
// 1.2.1
// d_tests_cli_words
//   struct: a command line split into words: their text, each word ended by
// a NUL; each word's start, a NULL after the last, as main's `argv` is; and
// how many there are.
struct d_tests_cli_words
{
    char        text[D_TESTS_CLI_LINE_MAX];
    const char* items[D_TESTS_CLI_WORDS_MAX + 1];
    int         count;
};

// 1.3    Cases
//------------------------------------------------------------------------------
// 1.3.1
// d_tests_cli_make_fn
//   type: a fixture: makes a section's program over a parser that
// d_arg_parser_init has made -- d_cli_program_init, then the program's own
// definitions -- and returns whether everything was made.
typedef bool (*d_tests_cli_make_fn)(struct d_cli_program* _program,
                                    struct d_arg_parser*  _parser);

// 1.3.2
// d_tests_cli_case
//   struct: one case: its tag, as a failure names it; its command line; and
// what running it must come to -- the status d_cli_program_run returns,
// and the whole of what it writes to standard output and standard error.
struct d_tests_cli_case
{
    const char* tag;
    const char* line;
    int         status;
    const char* out;
    const char* err;
};


D_EXTERN_C_BEGIN

//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    Command lines
//------------------------------------------------------------------------------
// 2.1.1
// d_tests_cli_split
/**
 * @brief Splits a command line into words: at runs of spaces, a pair of
 *        single quotes keeping what is between them -- spaces, and nothing
 *        at all -- as one word, or part of one.
 *
 * @param[in]  _line   the command line.
 * @param[out] _words  receives the words.
 * @return whether it split: false for a line too long, of too many words,
 *         or with a quote left open.
 */
bool d_tests_cli_split(const char*               _line,
                       struct d_tests_cli_words* _words);

// 2.2    Programs
//------------------------------------------------------------------------------
// 2.2.1
// d_tests_cli_start
/**
 * @brief Makes a program for a case: the parser, named `prog` and described
 *        "a probe of the kit", the fixture's program over it, writing into
 *        the buffers given and wrapping its help to D_TESTS_CLI_WIDTH,
 *        unless the fixture chose a width of its own.
 *
 * @param[out]    _program  the program.
 * @param[out]    _parser   its parser.
 * @param[in]     _make     the fixture.
 * @param[in,out] _out      the buffer standard output goes to.
 * @param[in,out] _err      the buffer standard error goes to.
 * @return whether the program was made; on failure nothing is held.
 */
bool d_tests_cli_start(struct d_cli_program* _program,
                       struct d_arg_parser*  _parser,
                       d_tests_cli_make_fn   _make,
                       struct d_arg_buffer*  _out,
                       struct d_arg_buffer*  _err);

// 2.2.2
// d_tests_cli_finish
//   lifetime: releases a program d_tests_cli_start made, then its parser.
void d_tests_cli_finish(struct d_cli_program* _program,
                        struct d_arg_parser*  _parser);

// 2.3    Cases
//------------------------------------------------------------------------------
// 2.3.1
// d_tests_cli_case_run
/**
 * @brief Runs one case on a program of its own -- d_cli_program_run over
 *        its command line -- and compares what it came to.
 *
 * @param[in] _case  the case.
 * @param[in] _make  the fixture.
 * @return whether the status, the output and the error output were each
 *         what the case says; each that was not is said, under the tag.
 */
bool d_tests_cli_case_run(const struct d_tests_cli_case* _case,
                          d_tests_cli_make_fn            _make);

// 2.3.2
// d_tests_cli_cases_run
//   check: d_tests_cli_case_run for each case of a table, every one run
// whatever came before; whether all held.
bool d_tests_cli_cases_run(const struct d_tests_cli_case* _cases,
                           size_t                         _count,
                           d_tests_cli_make_fn            _make);

// 2.4    Checks
//------------------------------------------------------------------------------
// 2.4.1
// d_tests_cli_same
/**
 * @brief Compares text with what it should be.
 *
 * @note A difference is said as the line it starts in, from each side,
 *       control characters escaped -- `\n`, `\033` -- after the tag and the
 *       name of what differs.
 *
 * @param[in] _tag       the case's tag.
 * @param[in] _what      what the text is: `out`, `err`, a file's name.
 * @param[in] _expected  what it should be, a C string.
 * @param[in] _actual    what it is; may be NULL for nothing.
 * @param[in] _length    its length.
 * @return whether they are the same.
 */
bool d_tests_cli_same(const char* _tag,
                      const char* _what,
                      const char* _expected,
                      const char* _actual,
                      size_t      _length);

// 2.4.2
// d_tests_cli_check
//   check: says `[tag] what` as a failure when `_held` is false; returns
// `_held`.
bool d_tests_cli_check(bool        _held,
                       const char* _tag,
                       const char* _what);

// 2.5    The environment
//------------------------------------------------------------------------------
// 2.5.1
// d_tests_cli_setenv
//   setting: sets an environment variable of the process -- the C runtime's
// and, on Windows, the wide one the kit reads -- or unsets it for a NULL
// value; whether it was done.
bool d_tests_cli_setenv(const char* _name,
                        const char* _value);

// 2.6    A command's work
//------------------------------------------------------------------------------
// 2.6.1
// d_tests_cli_print
//   writing: printf's format and arguments, written to a program's output
// as a command's work writes its result; text past 512 bytes is cut.
void d_tests_cli_print(const struct d_cli_program* _program,
                       const char*                 _format,
                       ...) D_FORMAT_PRINTF(2, 3);

// 2.6.2
// d_tests_cli_values
/**
 * @brief Writes what an option holds after a parse, as a case reads it: its
 *        values joined by `,`, an empty one as `''`; `-` when it was not
 *        given; `?` when the command has no option of that name.
 *
 * @param[in]  _command  the command to look the option up from, as
 *                       d_arg_command_option looks: its own, then its
 *                       parents' it takes.
 * @param[in]  _name     the option's name: `--out`, `SHEET`.
 * @param[out] _text     receives the text, cut to fit.
 * @param[in]  _size     the room in `_text`.
 * @return `_text`.
 */
const char* d_tests_cli_values(const struct d_arg_command* _command,
                               const char*                 _name,
                               char*                       _text,
                               size_t                      _size);

// 2.7    Files
//------------------------------------------------------------------------------
// 2.7.1
// d_tests_cli_file_read
/**
 * @brief Reads a file whole, as the C runtime reads it in binary mode.
 *
 * @param[in]  _name    the file's name.
 * @param[out] _length  receives its length; 0 on failure.
 * @return the bytes, to be freed with free; NULL when it could not be read.
 */
char*       d_tests_cli_file_read(const char* _name,
                                  size_t*     _length);

// 2.7.2
// d_tests_cli_file_write
//   making: writes bytes to a file, made anew, as the C runtime writes in
// binary mode; whether every byte was written.
bool        d_tests_cli_file_write(const char* _name,
                                   const char* _bytes,
                                   size_t      _length);

// 2.7.3
// d_tests_cli_file_holds
//   check: whether a file holds exactly the bytes given.
bool        d_tests_cli_file_holds(const char* _name,
                                   const char* _bytes,
                                   size_t      _length);

// 2.8    The standard streams
//------------------------------------------------------------------------------
// 2.8.1
// d_tests_cli_redirect
/**
 * @brief Puts a file in a standard stream's place for a while: the
 *        stream's descriptor saved, the file opened in binary mode -- for
 *        reading in standard input's place, written anew in standard
 *        output's or standard error's -- and its descriptor put where the
 *        stream's was.
 *
 * @param[in] _stream  the stream: 0, 1 or 2.
 * @param[in] _name    the file's name.
 * @return the stream's own descriptor, saved for d_tests_cli_restore; -1
 *         when nothing changed.
 */
int         d_tests_cli_redirect(int         _stream,
                                 const char* _name);

// 2.8.2
// d_tests_cli_restore
//   setting: puts back a standard stream's own descriptor, as
// d_tests_cli_redirect saved it; nothing for -1.
void        d_tests_cli_restore(int _stream,
                                int _saved);

D_EXTERN_C_END


#endif  // DJINTERP_PARSE_CLI_C_CLI_TESTS_SUPPORT_H
