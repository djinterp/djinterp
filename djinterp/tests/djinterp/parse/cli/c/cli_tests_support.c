/*******************************************************************************
* djinterp [parse]                                           cli_tests_support.c
*
* What the command-line kit's C suites share, declared in
* cli_tests_support.h.
*   A difference is said on the runner's standard output, before the line
* that reports its section, so that a failed section's report follows its
* reasons. With D_TESTS_CLI_DUMP set in the environment, each case run also
* prints what it came to as the table entry that would hold it: a
* maintainer who changed the kit's words on purpose reads those, checks
* them, and puts them in the table.
*
*
* path:      /tests/djinterp/parse/cli/c/cli_tests_support.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.23
*******************************************************************************/
#include "./cli_tests_support.h"  // corresponding header
// std
#include <fcntl.h>    // O_RDONLY, O_WRONLY, O_CREAT, O_TRUNC
#include <stdarg.h>   // va_list, va_start, va_end
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdio.h>    // printf, fopen, fread, ferror, snprintf, vsnprintf
#include <stdlib.h>   // getenv, setenv, unsetenv, _putenv, realloc, free
#include <string.h>   // memcmp, memset, strlen
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // d_arg_command_option
#include "../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // d_arg_buffer
#include "../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_formatter.h"  // d_arg_formatter_set_line_width
#include "../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // d_arg_option_values
#include "../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // d_arg_parser
#include "../../../../../inc/djinterp/parse/cli/c/cli_program.h"  // d_cli_program

#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    #include <io.h>       // _dup, _dup2, _close, _open
#else
    #include <unistd.h>   // dup, dup2, close
#endif


// D_TESTS_CLI_DUP / _DUP2 / _CLOSE / _OPEN / _BINARY
//   macros: the runtime's descriptor calls, spelled as each system spells
// them, and the flag that keeps a Windows descriptor's bytes as they are.
#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    #define D_TESTS_CLI_DUP     _dup
    #define D_TESTS_CLI_DUP2    _dup2
    #define D_TESTS_CLI_CLOSE   _close
    #define D_TESTS_CLI_OPEN    _open
    #define D_TESTS_CLI_BINARY  _O_BINARY
#else
    #define D_TESTS_CLI_DUP     dup
    #define D_TESTS_CLI_DUP2    dup2
    #define D_TESTS_CLI_CLOSE   close
    #define D_TESTS_CLI_OPEN    open
    #define D_TESTS_CLI_BINARY  0
#endif


//==============================================================================
// 1.  COMMAND LINES
//==============================================================================

/**
 * @brief Appends a byte to the words' text, keeping room for the NUL that
 *        ends the word.
 *
 * @param[in,out] _words  the words.
 * @param[in,out] _used   the text's length.
 * @param[in]     _byte   the byte.
 * @return whether there was room.
 */
static bool
d_tests_cli_put(
    struct d_tests_cli_words* _words,
    size_t*                   _used,
    char                      _byte
)
{
    // room for it and for a NUL after it
    if (*_used + 2u > sizeof(_words->text))
    {
        return false;
    }

    _words->text[(*_used)++] = _byte;

    return true;
}

/*
d_tests_cli_split
  A word starts at the first byte that is not a space outside quotes, and
ends at the next such space; a quote toggles quoting and is not kept.
*/
bool
d_tests_cli_split(
    const char*               _line,
    struct d_tests_cli_words* _words
)
{
    size_t used    = 0u;
    bool   quoted  = false;
    bool   in_word = false;
    bool   room    = true;

    memset(_words,
           0,
           sizeof(*_words));

    for (const char* at = _line; (room) && (*at != '\0'); ++at)
    {
        // a space outside quotes ends the word it follows
        if ( (*at == ' ') &&
             (!quoted) )
        {
            room    = ( (!in_word) ||
                        (d_tests_cli_put(_words,
                                         &used,
                                         '\0')) );
            in_word = false;

            continue;
        }

        // a word begins, while there is room for one
        if (!in_word)
        {
            room    = (_words->count < D_TESTS_CLI_WORDS_MAX);
            in_word = true;

            if (room)
            {
                _words->items[_words->count++] = &_words->text[used];
            }
        }

        quoted = (*at == '\'') ? (!quoted) : quoted;
        room   = ( (room) &&
                   ( (*at == '\'') ||
                     (d_tests_cli_put(_words,
                                      &used,
                                      *at)) ) );
    }

    room = ( (room) &&
             ( (!in_word) ||
               (d_tests_cli_put(_words,
                                &used,
                                '\0')) ) );
    _words->items[_words->count] = NULL;

    return ( (room) &&
             (!quoted) );
}


//==============================================================================
// 2.  PROGRAMS
//==============================================================================

/*
d_tests_cli_start
  The parser, the fixture's program, then the case's buffers and width.
*/
bool
d_tests_cli_start(
    struct d_cli_program* _program,
    struct d_arg_parser*  _parser,
    d_tests_cli_make_fn   _make,
    struct d_arg_buffer*  _out,
    struct d_arg_buffer*  _err
)
{
    memset(_program,
           0,
           sizeof(*_program));

    // the parser
    if (d_arg_parser_init(_parser,
                          "a probe of the kit",
                          "prog") != D_ARG_OK)
    {
        return false;
    }

    // the fixture's program
    if (!_make(_program,
               _parser))
    {
        d_tests_cli_finish(_program,
                           _parser);

        return false;
    }

    d_cli_program_set_output(_program,
                             d_arg_buffer_sink(_out),
                             d_arg_buffer_sink(_err));

    // the width a fixture chose, or the cases' own
    if (_parser->formatter.line_width == 0u)
    {
        d_arg_formatter_set_line_width(&_parser->formatter,
                                       (size_t)D_TESTS_CLI_WIDTH);
    }

    return true;
}

/*
d_tests_cli_finish
  The program, then the parser it was over.
*/
void
d_tests_cli_finish(
    struct d_cli_program* _program,
    struct d_arg_parser*  _parser
)
{
    d_cli_program_release(_program);
    d_arg_parser_release(_parser);

    return;
}


//==============================================================================
// 3.  CASES
//==============================================================================

/**
 * @brief Prints text as a C string literal's contents: a quote, a
 *        backslash and each control character escaped.
 *
 * @param[in] _text    the text; may be NULL for none.
 * @param[in] _length  its length.
 */
static void
d_tests_cli_print_escaped(
    const char* _text,
    size_t      _length
)
{
    for (size_t index = 0u; (_text != NULL) && (index < _length); ++index)
    {
        const unsigned char each = (unsigned char)_text[index];

        if (each == '\n')
        {
            printf("\\n");
        }
        else if ( (each == '"') ||
                  (each == '\\') )
        {
            printf("\\%c",
                   (char)each);
        }
        else if (each < 0x20u)
        {
            printf("\\%03o",
                   (unsigned int)each);
        }
        else
        {
            (void)putchar((int)each);
        }
    }

    return;
}

/**
 * @brief Prints text as C string literals, one for each of its lines, each
 *        on a line of its own after an indent.
 *
 * @param[in] _text    the text; may be NULL for none.
 * @param[in] _length  its length.
 */
static void
d_tests_cli_print_literal(
    const char* _text,
    size_t      _length
)
{
    const char* at  = _text;
    const char* end = (_text != NULL) ? (_text + _length) : NULL;

    printf("      \"");

    for (; (at != NULL) && (at < end); ++at)
    {
        d_tests_cli_print_escaped(at,
                                  1u);

        if ( (*at == '\n') &&
             (at + 1 < end) )
        {
            printf("\"\n      \"");
        }
    }

    printf("\"");

    return;
}

/**
 * @brief Prints what a case came to as the table entry that would hold it,
 *        when D_TESTS_CLI_DUMP is set: the output and the error output each
 *        a string literal per line.
 *
 * @param[in] _case    the case.
 * @param[in] _status  the status it came to.
 * @param[in] _out     its output.
 * @param[in] _err     its error output.
 */
static void
d_tests_cli_dump(
    const struct d_tests_cli_case* _case,
    int                            _status,
    const struct d_arg_buffer*     _out,
    const struct d_arg_buffer*     _err
)
{
    const struct d_arg_buffer* const parts[2] = { _out, _err };

    // asked for by the environment alone
    if (getenv("D_TESTS_CLI_DUMP") == NULL)
    {
        return;
    }

    printf("    { \"%s\",\n      \"%s\",\n      %d,\n",
           _case->tag,
           _case->line,
           _status);

    for (size_t part = 0u; part < 2u; ++part)
    {
        d_tests_cli_print_literal(parts[part]->data,
                                  parts[part]->length);
        printf("%s\n",
               (part == 0u) ? "," : " },");
    }

    (void)fflush(stdout);

    return;
}

/*
d_tests_cli_case_run
  The words, a program of the case's own, the run; then the status and the
two outputs compared, the program released before the buffers it wrote
into.
*/
bool
d_tests_cli_case_run(
    const struct d_tests_cli_case* _case,
    d_tests_cli_make_fn            _make
)
{
    struct d_tests_cli_words words;
    struct d_cli_program     program;
    struct d_arg_parser      parser;
    struct d_arg_buffer      out = D_ARG_BUFFER_INIT;
    struct d_arg_buffer      err = D_ARG_BUFFER_INIT;

    // a command line the table should not hold, or no program
    if ( (!d_tests_cli_split(_case->line,
                             &words)) ||
         (!d_tests_cli_start(&program,
                             &parser,
                             _make,
                             &out,
                             &err)) )
    {
        return d_tests_cli_check(false,
                                 _case->tag,
                                 "the case's program could not be made");
    }

    const int status = d_cli_program_run(&program,
                                         words.count,
                                         words.items);
    bool      held   = (status == _case->status);

    if (!held)
    {
        printf("  [%s] status: expected %d, was %d\n",
               _case->tag,
               _case->status,
               status);
    }

    d_tests_cli_dump(_case,
                     status,
                     &out,
                     &err);
    d_tests_cli_finish(&program,
                       &parser);
    held = ( (d_tests_cli_same(_case->tag,
                               "out",
                               _case->out,
                               out.data,
                               out.length)) &&
             (held) );
    held = ( (d_tests_cli_same(_case->tag,
                               "err",
                               _case->err,
                               err.data,
                               err.length)) &&
             (held) );
    d_arg_buffer_release(&out);
    d_arg_buffer_release(&err);

    return held;
}

/*
d_tests_cli_cases_run
  Every case, whatever the one before came to.
*/
bool
d_tests_cli_cases_run(
    const struct d_tests_cli_case* _cases,
    size_t                         _count,
    d_tests_cli_make_fn            _make
)
{
    bool held = true;

    for (size_t index = 0u; index < _count; ++index)
    {
        held = ( (d_tests_cli_case_run(&_cases[index],
                                       _make)) &&
                 (held) );
    }

    return held;
}


//==============================================================================
// 4.  CHECKS
//==============================================================================

/**
 * @brief Prints the line of a text that holds an offset, escaped and
 *        quoted, after a label.
 *
 * @param[in] _label   what the line is: `expected` or `actual`.
 * @param[in] _text    the text; may be NULL for none.
 * @param[in] _length  its length.
 * @param[in] _at      the offset, where the texts first differ.
 */
static void
d_tests_cli_print_line(
    const char* _label,
    const char* _text,
    size_t      _length,
    size_t      _at
)
{
    size_t start = (_at < _length) ? _at : _length;
    size_t end   = start;

    while ( (start > 0u) &&
            (_text[start - 1u] != '\n') )
    {
        --start;
    }

    while ( (end < _length) &&
            (_text[end] != '\n') )
    {
        ++end;
    }

    printf("      %-9s\"",
           _label);
    d_tests_cli_print_escaped(_text + start,
                              (end < _length) ? (end - start + 1u)
                                              : (end - start));
    printf("\"%s\n",
           (start == _length) ? " (the text ends here)" : "");

    return;
}

/*
d_tests_cli_same
  The texts' lengths and bytes; where they differ, the line each holds there.
*/
bool
d_tests_cli_same(
    const char* _tag,
    const char* _what,
    const char* _expected,
    const char* _actual,
    size_t      _length
)
{
    const char* const actual = (_actual != NULL) ? _actual : "";
    const size_t      length = (_actual != NULL) ? _length : 0u;
    const size_t      wanted = strlen(_expected);
    size_t            at     = 0u;

    // the same text
    if ( (wanted == length) &&
         (memcmp(_expected,
                 actual,
                 length) == 0) )
    {
        return true;
    }

    while ( (at < wanted) &&
            (at < length) &&
            (_expected[at] == actual[at]) )
    {
        ++at;
    }

    printf("  [%s] %s differs at byte %lu:\n",
           _tag,
           _what,
           (unsigned long)at);
    d_tests_cli_print_line("expected",
                           _expected,
                           wanted,
                           at);
    d_tests_cli_print_line("actual",
                           actual,
                           length,
                           at);

    // the whole of it, to be checked and put where it is expected
    if (getenv("D_TESTS_CLI_DUMP") != NULL)
    {
        printf("    all of it:\n");
        d_tests_cli_print_literal(actual,
                                  length);
        printf("\n");
    }

    (void)fflush(stdout);

    return false;
}

/*
d_tests_cli_check
  The failure said, and the verdict returned.
*/
bool
d_tests_cli_check(
    bool        _held,
    const char* _tag,
    const char* _what
)
{
    if (!_held)
    {
        printf("  [%s] %s\n",
               _tag,
               _what);
        (void)fflush(stdout);
    }

    return _held;
}


//==============================================================================
// 5.  THE ENVIRONMENT
//==============================================================================

/*
d_tests_cli_setenv
  setenv and unsetenv; on Windows _putenv, which copies its text, updates
the wide environment as well, and removes a variable given nothing.
*/
bool
d_tests_cli_setenv(
    const char* _name,
    const char* _value
)
{
#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    char setting[512];

    // NAME=VALUE, or NAME= to remove it
    if (strlen(_name) + ((_value != NULL) ? strlen(_value) : 0u) + 2u >
        sizeof(setting))
    {
        return false;
    }

    (void)snprintf(setting,
                   sizeof(setting),
                   "%s=%s",
                   _name,
                   (_value != NULL) ? _value : "");

    return (_putenv(setting) == 0);
#else
    return (_value != NULL)
           ? (setenv(_name,
                     _value,
                     1) == 0)
           : (unsetenv(_name) == 0);
#endif
}


//==============================================================================
// 6.  A COMMAND'S WORK
//==============================================================================

/*
d_tests_cli_print
  vsnprintf into a line's room, then the program's output sink.
*/
void
d_tests_cli_print(
    const struct d_cli_program* _program,
    const char*                 _format,
    ...
)
{
    char    text[512];
    va_list arguments;

    va_start(arguments,
             _format);

    const int length = vsnprintf(text,
                                 sizeof(text),
                                 _format,
                                 arguments);

    va_end(arguments);

    // nothing to write, or nowhere to write it
    if ( (length <= 0) ||
         (_program->out.write == NULL) )
    {
        return;
    }

    (void)_program->out.write(_program->out.context,
                              text,
                              ((size_t)length < sizeof(text))
                              ? (size_t)length
                              : (sizeof(text) - 1u));

    return;
}

/*
d_tests_cli_values
  The option looked up; each value appended after a `,`, cut where the room
ends.
*/
const char*
d_tests_cli_values(
    const struct d_arg_command* _command,
    const char*                 _name,
    char*                       _text,
    size_t                      _size
)
{
    const struct d_arg_option* const option = d_arg_command_option(_command,
                                                                   _name);
    size_t                           count  = 0u;
    const struct d_pack_text* const  values = (option != NULL)
                                              ? d_arg_option_values(option,
                                                                    &count)
                                              : NULL;
    size_t                           used   = 0u;

    (void)snprintf(_text,
                   _size,
                   "%s",
                   (option == NULL) ? "?" : "-");

    for (size_t index = 0u; (index < count) && (used < _size); ++index)
    {
        const bool empty = (values[index].length == 0u);
        const int  wrote = snprintf(_text + used,
                                    _size - used,
                                    "%s%.*s",
                                    (index > 0u) ? "," : "",
                                    (empty) ? 2 : (int)values[index].length,
                                    (empty) ? "''" : values[index].data);

        used += (wrote > 0) ? (size_t)wrote : 0u;
    }

    return _text;
}


//==============================================================================
// 7.  FILES
//==============================================================================

/*
d_tests_cli_file_read
  fread in blocks, the buffer grown for each, until a short block; an error
reading, or no room, is no file read.
*/
char*
d_tests_cli_file_read(
    const char* _name,
    size_t*     _length
)
{
    FILE* const file  = fopen(_name,
                              "rb");
    char*       bytes = NULL;
    size_t      size  = 0u;
    bool        done  = false;

    *_length = 0u;

    // each block, until the end
    while ( (file != NULL) &&
            (!done) )
    {
        char* const grown = (char*)realloc(bytes,
                                           size + 4096u);

        if (!grown)
        {
            break;
        }

        const size_t got = fread(grown + size,
                                 1u,
                                 4096u,
                                 file);

        bytes  = grown;
        size  += got;
        done   = (got < 4096u);
    }

    // a block short of an error, or no file
    if ( (file == NULL) ||
         (!done)        ||
         (ferror(file)) )
    {
        free(bytes);

        if (file != NULL)
        {
            (void)fclose(file);
        }

        return NULL;
    }

    (void)fclose(file);

    *_length = size;

    return bytes;
}

/*
d_tests_cli_file_write
  fwrite, then fclose, which reports what the writes deferred.
*/
bool
d_tests_cli_file_write(
    const char* _name,
    const char* _bytes,
    size_t      _length
)
{
    FILE* const file = fopen(_name,
                             "wb");
    bool        done = (file != NULL);

    if (done)
    {
        done = (fwrite(_bytes,
                       1u,
                       _length,
                       file) == _length);
        done = (fclose(file) == 0) && done;
    }

    return done;
}

/*
d_tests_cli_file_holds
  The file read, its length and bytes compared.
*/
bool
d_tests_cli_file_holds(
    const char* _name,
    const char* _bytes,
    size_t      _length
)
{
    size_t     length = 0u;
    char*      held   = d_tests_cli_file_read(_name,
                                              &length);
    const bool same   = ( (held != NULL)        &&
                          (length == _length)   &&
                          ( (_length == 0u) ||
                            (memcmp(held,
                                    _bytes,
                                    _length) == 0) ) );

    free(held);

    return same;
}


//==============================================================================
// 8.  THE STANDARD STREAMS
//==============================================================================

/**
 * @brief The C stream over a standard stream's descriptor.
 *
 * @param[in] _stream  0, 1 or 2.
 * @return stdin, stdout or stderr.
 */
static FILE*
d_tests_cli_stream(
    int _stream
)
{
    return (_stream == 0) ? stdin
         : (_stream == 1) ? stdout
                          : stderr;
}

/*
d_tests_cli_redirect
  What the C streams hold is flushed first, so that none of it lands in the
file; the stream's error and end-of-file are cleared after, so that it
reads and writes the file afresh.
*/
int
d_tests_cli_redirect(
    int         _stream,
    const char* _name
)
{
    const int flags = (_stream == 0)
                      ? (O_RDONLY | D_TESTS_CLI_BINARY)
                      : (O_WRONLY | O_CREAT | O_TRUNC | D_TESTS_CLI_BINARY);

    (void)fflush(stdout);
    (void)fflush(stderr);

    const int file  = D_TESTS_CLI_OPEN(_name,
                                       flags,
                                       0644);
    const int saved = (file >= 0) ? D_TESTS_CLI_DUP(_stream) : -1;

    // the file in the stream's place
    if ( (saved >= 0) &&
         (D_TESTS_CLI_DUP2(file,
                           _stream) < 0) )
    {
        (void)D_TESTS_CLI_CLOSE(saved);
        (void)D_TESTS_CLI_CLOSE(file);

        return -1;
    }

    if (file >= 0)
    {
        (void)D_TESTS_CLI_CLOSE(file);
    }

    clearerr(d_tests_cli_stream(_stream));

    return saved;
}

/*
d_tests_cli_restore
  Flushed, put back, cleared.
*/
void
d_tests_cli_restore(
    int _stream,
    int _saved
)
{
    (void)fflush(stdout);
    (void)fflush(stderr);

    if (_saved >= 0)
    {
        (void)D_TESTS_CLI_DUP2(_saved,
                               _stream);
        (void)D_TESTS_CLI_CLOSE(_saved);
    }

    clearerr(d_tests_cli_stream(_stream));

    return;
}
