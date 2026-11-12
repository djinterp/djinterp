/*******************************************************************************
* djinterp [tools]                                                 cli_program.c
*
* A command-line program as the kit runs it, declared in cli_program.h, and
* the text helpers of cli_internal.h's section 2.
*   The kit adds its standard options to the root unbound, and reads them after
* every parse -- a failed one included, so that the message a failure writes
* is coloured as `--color` asked when it was given before the mistake. The
* parse's own work is the argument parser's; the kit's is what comes before it
* -- the true command line on Windows, `/?` as `--help`, the run's settings
* started and the command line's taken off it (cli_settings.h) -- and after:
* the exit status the grammar gives each ending, and the failure message in
* the GNU shape, written by a failure function of the kit's that the parser
* calls. After a parse that went well, at `-v`, where the run goes on to a
* command's work, an option that keeps one value and was given values that
* differ is said, with the one it took (GRAMMAR R8).
*   Every diagnostic is composed in a buffer and written to the error output
* in one write, so that two programs writing to one terminal do not
* interleave within a line.
*   `--version` at `-v` or more says the build after the version's line,
* composed whole before any of it is written: the program's file and a
* module's, where each command that is a module would load it from, which
* cli_plugin.c says, how the program was built as it was told, its own
* lines, and how the kit was built -- this unit describes itself.
*
*
* path:      /src/djinterp/tools/cli/c/cli_program.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.11.13
*******************************************************************************/
#include "../../../../../inc/djinterp/tools/cli/c/cli_program.h"  // corresponding header
// std
#include <errno.h>    // errno, ENOMEM
#include <stdarg.h>   // va_list, va_start, va_end, va_copy
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdio.h>    // snprintf, vsnprintf
#include <stdlib.h>   // the hooks' malloc, realloc and free
#include <string.h>   // memchr, memcpy, strchr, strcmp, strlen, strncmp,
                      // strpbrk, strrchr
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/c/util/sink_common.h"  // dj_pack_sink
#include "../../../../../inc/djinterp/config/tools/cli/cfg_cli.h"  // D_INTERNAL_CLI_*
#include "../../../../../inc/djinterp/env/tools/cli/env_cli.h"  // D_ENV_CLI_HELP_SLASH
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_command.h"  // commands
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_completion.h"  // scripts
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_formatter.h"  // help
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_option.h"  // options
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_parser.h"  // the parser
#include "../../../../../inc/djinterp/tools/cli/c/cli_format.h"  // d_cli_formatter_use
#include "../../../../../inc/djinterp/tools/cli/c/cli_io.h"  // sinks, colour
#include "../../../../../inc/djinterp/tools/cli/c/cli_settings.h"  // settings
#include "./cli_internal.h"  // copies, the system
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// 1.  TEXT
//==============================================================================

/*
d_cli_internal_copy
  strlen and memcpy, through the kit's allocator.
*/
char* d_cli_internal_copy(
    const char* _text
)
{
    // nothing to copy
    if (!_text)
    {
        return NULL;
    }

    const size_t length = strlen(_text);
    char* const  copy   = (char*)D_INTERNAL_CLI_MALLOC(length + 1u);

    // no room
    if (!copy)
    {
        return NULL;
    }

    memcpy(copy,
           _text,
           length + 1u);

    return copy;
}

/*
d_cli_internal_lower_first
  A capital and a small letter: a word that begins a sentence, which the
GNU shape writes in lower case. A capital before another capital, a digit,
a quote or a hyphen is a name, and stays.
*/
bool d_cli_internal_lower_first(
    const char* _text,
    size_t      _length
)
{
    return ( (_text != NULL)    &&
             (_length >= 2u)    &&
             (_text[0] >= 'A')  &&
             (_text[0] <= 'Z')  &&
             (_text[1] >= 'a')  &&
             (_text[1] <= 'z') );
}

/**
 * @brief Appends printf-formatted text from a va_list to a buffer.
 *
 * @param[in,out] _out        the buffer.
 * @param[in]     _format     the format.
 * @param[in]     _arguments  its arguments.
 * @return D_ARG_OK, the buffer's status, or D_ARG_ERROR_BAD_ARGUMENT when
 *         the format cannot be expanded.
 */
static
int d_cli_internal_vappend(
    struct dj_arg_buffer* _out,
    const char*           _format,
    va_list               _arguments
)
{
    va_list measured;

    va_copy(measured,
            _arguments);

    const int length = vsnprintf(NULL,
                                 0u,
                                 _format,
                                 measured);

    va_end(measured);

    // a format that does not expand
    if (length < 0)
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    char* const text = (char*)D_INTERNAL_CLI_MALLOC((size_t)length + 1u);

    // no room
    if (!text)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    (void)vsnprintf(text,
                    (size_t)length + 1u,
                    _format,
                    _arguments);

    const int status = d_arg_buffer_append(_out,
                                           text,
                                           (size_t)length);

    D_INTERNAL_CLI_FREE(text);

    return status;
}

/**
 * @brief Appends a span to a buffer with its first letter lowered when it
 *        begins a sentence (d_cli_internal_lower_first).
 *
 * @param[in,out] _out     the buffer.
 * @param[in]     _text    the text.
 * @param[in]     _length  its length.
 * @return D_ARG_OK, or the buffer's status.
 */
static
int d_cli_internal_append_lowered(
    struct dj_arg_buffer* _out,
    const char*           _text,
    size_t                _length
)
{
    // the first letter, lowered
    if (d_cli_internal_lower_first(_text,
                                   _length))
    {
        const int status = d_arg_buffer_append_char(_out,
                                                    (char)(_text[0] -
                                                           'A' + 'a'));

        return (status == D_ARG_OK)
               ? d_arg_buffer_append(_out,
                                     _text + 1,
                                     _length - 1u)
               : status;
    }

    return d_arg_buffer_append(_out,
                               _text,
                               _length);
}


//==============================================================================
// 2.  DIAGNOSTICS
//==============================================================================

// D_INTERNAL_CLI_BOLD / _RED / _MAGENTA / _CYAN / _RESET
//   constants: the ANSI sequences GCC colours a diagnostic with.
#define D_INTERNAL_CLI_BOLD    "\033[1m"
#define D_INTERNAL_CLI_RED     "\033[1;31m"
#define D_INTERNAL_CLI_MAGENTA "\033[1;35m"
#define D_INTERNAL_CLI_CYAN    "\033[1;36m"
#define D_INTERNAL_CLI_RESET   "\033[0m"

/**
 * @brief A severity's word, as a program says it, and the shade GCC colours
 *        it with.
 *
 * @param[in]  _program   the program: one with D_CLI_PROGRAM_PLAIN_ERRORS
 *                        says an error without its word.
 * @param[in]  _severity  a dj_cli_severity value.
 * @param[out] _shade     receives the shade.
 * @return `warning`, `note`, or, for any other value, `error` -- or NULL,
 *         for an error said without its word.
 */
static
const char* d_cli_internal_severity(
    const struct dj_cli_program* _program,
    int                          _severity,
    const char**                 _shade
)
{
    // a warning
    if (_severity == (int)D_CLI_SEVERITY_WARNING)
    {
        *_shade = D_INTERNAL_CLI_MAGENTA;

        return "warning";
    }

    // a note
    if (_severity == (int)D_CLI_SEVERITY_NOTE)
    {
        *_shade = D_INTERNAL_CLI_CYAN;

        return "note";
    }

    *_shade = D_INTERNAL_CLI_RED;

    return ((_program->flags & D_CLI_PROGRAM_PLAIN_ERRORS) != 0u)
           ? NULL
           : "error";
}

/**
 * @brief Appends a diagnostic's locus and severity: `PROGRAM: error: ` or
 *        `FILE:LINE:COLUMN: warning: `, coloured as GCC colours them; the
 *        locus alone, `PROGRAM: `, for an error said without its word.
 *
 * @param[in]     _program   the program.
 * @param[in]     _severity  a dj_cli_severity value.
 * @param[in]     _file      the file, or NULL for the program's name.
 * @param[in]     _line      the line, or 0 or less for none.
 * @param[in]     _column    the column, or 0 or less for none.
 * @param[in,out] _out       the buffer.
 * @return D_ARG_OK, or the buffer's status.
 */
static
int d_cli_internal_locus(
    const struct dj_cli_program* _program,
    int                          _severity,
    const char*                  _file,
    long                         _line,
    long                         _column,
    struct dj_arg_buffer*        _out
)
{
    const bool        colour = _program->colour_error;
    const char* const colon  = (colour) ? ":" D_INTERNAL_CLI_RESET " "
                                        : ": ";
    const char*       shade  = NULL;
    const char* const word   = d_cli_internal_severity(_program,
                                                       _severity,
                                                       &shade);
    int               status = (colour)
                               ? d_arg_buffer_append_cstr(_out,
                                                          D_INTERNAL_CLI_BOLD)
                               : D_ARG_OK;

    // the place, or the program
    status = (status == D_ARG_OK)
             ? d_arg_buffer_append_cstr(_out,
                                        (_file != NULL) ? _file
                                                        : _program->name)
             : status;
    status = ( (status == D_ARG_OK) &&
               (_file != NULL)      &&
               (_line > 0L) )
             ? ( (_column > 0L)
                 ? d_arg_buffer_printf(_out,
                                       ":%ld:%ld",
                                       _line,
                                       _column)
                 : d_arg_buffer_printf(_out,
                                       ":%ld",
                                       _line) )
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_buffer_append_cstr(_out,
                                        colon)
             : status;

    // the severity, where it is said
    if (!word)
    {
        return status;
    }

    status = ( (status == D_ARG_OK) &&
               (colour) )
             ? d_arg_buffer_append_cstr(_out,
                                        shade)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_buffer_append_cstr(_out,
                                        word)
             : status;

    return (status == D_ARG_OK)
           ? d_arg_buffer_append_cstr(_out,
                                      colon)
           : status;
}

/**
 * @brief Whether a diagnostic of a severity is written at the program's
 *        verbosity: an error always; a warning or a note unless `-q`.
 *
 * @param[in] _program   the program.
 * @param[in] _severity  a dj_cli_severity value.
 * @return `true` if it is written.
 */
static
bool d_cli_internal_shown(
    const struct dj_cli_program* _program,
    int                          _severity
)
{
    return ( (_severity == (int)D_CLI_SEVERITY_ERROR) ||
             (_program->verbosity >= 0) );
}

/**
 * @brief Writes a buffer whole to the program's error output; a refusal is
 *        dropped, as a diagnostic that cannot be written is.
 *
 * @param[in] _program  the program.
 * @param[in] _out      the buffer.
 */
static
void d_cli_internal_emit(
    const struct dj_cli_program* _program,
    const struct dj_arg_buffer*  _out
)
{
    // nothing composed, or nowhere to write
    if ( (_out->failed != 0)            ||
         (_out->length == 0u)           ||
         (_program->err.write == NULL) )
    {
        return;
    }

    (void)_program->err.write(_program->err.context,
                              _out->data,
                              _out->length);

    return;
}

// D_INTERNAL_CLI_SPARE
//   constant: the room a diagnostic is composed in when no memory can be had
// for it: a few lines, cut there.
#define D_INTERNAL_CLI_SPARE 640

// dj_cli_internal_shape
//   struct: a diagnostic composed in a room of its own: its severity; its
// place in a file -- `file` NULL for the program's name; the command whose
// place it names, NULL for none; whether the message's first letter is
// lowered, as the parser's messages are; and whether the pointer to the
// command's help follows -- never after a want of memory, which no help
// mends.
struct dj_cli_internal_shape
{
    int                         severity;
    const char*                 file;
    long                        line;
    long                        column;
    const struct dj_arg_command* command;
    bool                        lower;
    bool                        pointer;
};

/**
 * @brief Appends to a room of its own, cut at the room's end; asks for no
 *        memory.
 *
 * @param[in,out] _room    the room.
 * @param[in]     _size    its size.
 * @param[in,out] _used    how much of it is used, NUL not counted.
 * @param[in]     _format  printf's format.
 * @param[in]     ...      its arguments.
 */
static
void d_cli_internal_room_add(
    char*       _room,
    size_t      _size,
    size_t*     _used,
    const char* _format,
    ...
) D_FORMAT_PRINTF(4, 5);

static
void d_cli_internal_room_add(
    char*       _room,
    size_t      _size,
    size_t*     _used,
    const char* _format,
    ...
)
{
    va_list arguments;

    // a room already full
    if (*_used + 1u >= _size)
    {
        return;
    }

    va_start(arguments,
             _format);
    (void)vsnprintf(_room + *_used,
                    _size - *_used,
                    _format,
                    arguments);
    va_end(arguments);
    *_used += strlen(_room + *_used);

    return;
}

/**
 * @brief Appends a command's full name to a room, as d_cli_command_path
 *        writes it, asking for no memory.
 *
 * @param[in]     _command  the command.
 * @param[in,out] _room     the room.
 * @param[in]     _size     its size.
 * @param[in,out] _used     how much of it is used.
 */
static
void d_cli_internal_room_path(
    const struct dj_arg_command* _command,
    char*                        _room,
    size_t                       _size,
    size_t*                      _used
)
{
    const struct dj_arg_command* chain[32];
    size_t                      depth = 0u;

    // up to the root, option groups left out
    for (const struct dj_arg_command* at = _command;
         (at != NULL) && (depth < (sizeof(chain) / sizeof(chain[0])));
         at = at->parent)
    {
        if (!d_arg_command_is_option_group(at))
        {
            chain[depth++] = at;
        }
    }

    // root first
    while (depth > 0u)
    {
        --depth;
        d_cli_internal_room_add(_room,
                                _size,
                                _used,
                                "%s%s",
                                d_arg_command_name(chain[depth]),
                                (depth > 0u) ? " " : "");
    }

    return;
}

/**
 * @brief Appends a command's place to a room -- its full name less the
 *        program's own, and a colon -- as d_cli_internal_place appends it,
 *        asking for no memory.
 *
 * @param[in]     _program  the program.
 * @param[in]     _command  the command; NULL for no place.
 * @param[in,out] _room     the room.
 * @param[in]     _size     its size.
 * @param[in,out] _used     how much of it is used.
 */
static
void d_cli_internal_room_place(
    const struct dj_cli_program* _program,
    const struct dj_arg_command* _command,
    char*                        _room,
    size_t                       _size,
    size_t*                      _used
)
{
    char         path[D_INTERNAL_CLI_SPARE];
    size_t       length = 0u;
    const size_t own    = strlen(_program->name);
    const char*  rest   = path;

    path[0] = '\0';

    if (_command != NULL)
    {
        d_cli_internal_room_path(_command,
                                 path,
                                 sizeof(path),
                                 &length);
    }

    // the program's own name, and the space after it
    if ( (strncmp(path,
                  _program->name,
                  own) == 0)            &&
         ( (path[own] == ' ') ||
           (path[own] == '\0') ) )
    {
        rest += (path[own] == ' ') ? (own + 1u) : own;
    }

    if (rest[0] != '\0')
    {
        d_cli_internal_room_add(_room,
                                _size,
                                _used,
                                "%s: ",
                                rest);
    }

    return;
}

/**
 * @brief Appends a diagnostic's locus and severity to a room, as
 *        d_cli_internal_locus appends them, coloured as it colours them:
 *        `FILE:LINE:COLUMN: warning: `, or `PROGRAM: error: `; the locus
 *        alone for an error said without its word.
 *
 * @param[in]     _program  the program.
 * @param[in]     _shape    the diagnostic's shape.
 * @param[in,out] _room     the room.
 * @param[in]     _size     its size.
 * @param[in,out] _used     how much of it is used.
 */
static
void d_cli_internal_room_locus(
    const struct dj_cli_program*        _program,
    const struct dj_cli_internal_shape* _shape,
    char*                               _room,
    size_t                              _size,
    size_t*                             _used
)
{
    const bool        colour = _program->colour_error;
    const char* const colon  = (colour) ? ":" D_INTERNAL_CLI_RESET " "
                                        : ": ";
    const char*       shade  = NULL;
    const char* const word   = d_cli_internal_severity(_program,
                                                       _shape->severity,
                                                       &shade);

    d_cli_internal_room_add(_room,
                            _size,
                            _used,
                            "%s%s",
                            (colour) ? D_INTERNAL_CLI_BOLD : "",
                            (_shape->file != NULL) ? _shape->file
                                                   : _program->name);

    // the line and column, in a file
    if ( (_shape->file != NULL) &&
         (_shape->line > 0L) )
    {
        d_cli_internal_room_add(_room,
                                _size,
                                _used,
                                ":%ld",
                                _shape->line);

        if (_shape->column > 0L)
        {
            d_cli_internal_room_add(_room,
                                    _size,
                                    _used,
                                    ":%ld",
                                    _shape->column);
        }
    }

    d_cli_internal_room_add(_room,
                            _size,
                            _used,
                            "%s%s%s%s",
                            colon,
                            ( (colour) && (word != NULL) ) ? shade : "",
                            (word != NULL) ? word : "",
                            (word != NULL) ? colon : "");

    return;
}

/**
 * @brief Appends `Try 'PATH --help' for more information.` and a newline to
 *        a room, for a command; nothing for none.
 *
 * @param[in]     _command  the command whose help to point to, or NULL.
 * @param[in,out] _room     the room.
 * @param[in]     _size     its size.
 * @param[in,out] _used     how much of it is used.
 */
static
void d_cli_internal_room_try(
    const struct dj_arg_command* _command,
    char*                        _room,
    size_t                       _size,
    size_t*                      _used
)
{
    if (_command != NULL)
    {
        d_cli_internal_room_add(_room,
                                _size,
                                _used,
                                "Try '");
        d_cli_internal_room_path(_command,
                                 _room,
                                 _size,
                                 _used);
        d_cli_internal_room_add(_room,
                                _size,
                                _used,
                                " --help' for more information.\n");
    }

    return;
}

/**
 * @brief Writes a diagnostic composed in a room of its own, asking for no
 *        memory: in the GNU shape and colour -- the locus, the severity,
 *        the command's place and the message; then, for a command, the
 *        pointer to its help -- cut at the room's end, a newline kept last.
 *        What a report falls back on when the whole one could not be
 *        composed, so that the want of memory shortens a diagnostic, never
 *        loses it.
 *
 * @param[in] _program    the program.
 * @param[in] _shape      the diagnostic's shape.
 * @param[in] _sink       where it goes.
 * @param[in] _format     the message's format.
 * @param[in] _arguments  its arguments.
 * @return D_ARG_OK, or D_ARG_ERROR_FILE when the sink took less than all.
 */
static
int d_cli_internal_spare(
    const struct dj_cli_program*        _program,
    const struct dj_cli_internal_shape* _shape,
    struct dj_pack_sink                 _sink,
    const char*                         _format,
    va_list                             _arguments
)
{
    char   room[D_INTERNAL_CLI_SPARE];
    size_t used  = 0u;
    size_t start = 0u;

    room[0] = '\0';
    d_cli_internal_room_locus(_program,
                              _shape,
                              room,
                              sizeof(room),
                              &used);
    d_cli_internal_room_place(_program,
                              _shape->command,
                              room,
                              sizeof(room),
                              &used);
    start = used;

    // the message, its first letter lowered as the parser's are
    if (used + 1u < sizeof(room))
    {
        (void)vsnprintf(room + used,
                        sizeof(room) - used,
                        _format,
                        _arguments);
        used += strlen(room + used);
    }

    if ( (_shape->lower) &&
         (d_cli_internal_lower_first(room + start,
                                     used - start)) )
    {
        room[start] = (char)(room[start] - 'A' + 'a');
    }

    d_cli_internal_room_add(room,
                            sizeof(room),
                            &used,
                            "\n");
    d_cli_internal_room_try((_shape->pointer) ? _shape->command : NULL,
                            room,
                            sizeof(room),
                            &used);
    room[used - 1u] = '\n';

    return ( (_sink.write != NULL) &&
             (_sink.write(_sink.context,
                          room,
                          used) != used) )
           ? D_ARG_ERROR_FILE
           : D_ARG_OK;
}

/**
 * @brief d_cli_internal_spare with its arguments given.
 *
 * @param[in] _program  the program.
 * @param[in] _shape    the diagnostic's shape.
 * @param[in] _sink     where it goes.
 * @param[in] _format   the message's format.
 * @param[in] ...       its arguments.
 * @return as d_cli_internal_spare.
 */
static
int d_cli_internal_spare_say(
    const struct dj_cli_program*        _program,
    const struct dj_cli_internal_shape* _shape,
    struct dj_pack_sink                 _sink,
    const char*                         _format,
    ...
) D_FORMAT_PRINTF(4, 5);

static
int d_cli_internal_spare_say(
    const struct dj_cli_program*        _program,
    const struct dj_cli_internal_shape* _shape,
    struct dj_pack_sink                 _sink,
    const char*                         _format,
    ...
)
{
    va_list arguments;

    va_start(arguments,
             _format);

    const int status = d_cli_internal_spare(_program,
                                            _shape,
                                            _sink,
                                            _format,
                                            arguments);

    va_end(arguments);

    return status;
}

/*
d_cli_vreport
  The locus, the message, a newline; composed whole, written once -- or,
where there was no memory to compose it, composed in a room of
its own.
*/
void d_cli_vreport(
    const struct dj_cli_program* _program,
    int                          _severity,
    const char*                  _file,
    long                         _line,
    long                         _column,
    const char*                  _format,
    va_list                      _arguments
)
{
    // nothing to say, or nothing to say it with
    if ( (!_program) ||
         (!_format)  ||
         (!d_cli_internal_shown(_program,
                                _severity)) )
    {
        return;
    }

    const struct dj_cli_internal_shape shape  = { _severity,
                                                 _file,
                                                 _line,
                                                 _column,
                                                 NULL,
                                                 false,
                                                 false };
    struct dj_arg_buffer               out    = D_ARG_BUFFER_INIT;
    int                               status = d_cli_internal_locus(
                                                   _program,
                                                   _severity,
                                                   _file,
                                                   _line,
                                                   _column,
                                                   &out);
    va_list                           spare;

    va_copy(spare,
            _arguments);
    status = (status == D_ARG_OK)
             ? d_cli_internal_vappend(&out,
                                      _format,
                                      _arguments)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_buffer_append_char(&out,
                                        '\n')
             : status;

    // written whole, or in a room of its own
    if (status == D_ARG_OK)
    {
        d_cli_internal_emit(_program,
                            &out);
    }
    else
    {
        (void)d_cli_internal_spare(_program,
                                   &shape,
                                   _program->err,
                                   _format,
                                   spare);
    }

    va_end(spare);
    d_arg_buffer_release(&out);

    return;
}

/*
d_cli_report
  d_cli_vreport, at no place.
*/
void d_cli_report(
    const struct dj_cli_program* _program,
    int                          _severity,
    const char*                  _format,
    ...
)
{
    va_list arguments;

    va_start(arguments,
             _format);
    d_cli_vreport(_program,
                  _severity,
                  NULL,
                  0L,
                  0L,
                  _format,
                  arguments);
    va_end(arguments);

    return;
}

/*
d_cli_report_at
  d_cli_vreport, at a place.
*/
void d_cli_report_at(
    const struct dj_cli_program* _program,
    int                          _severity,
    const char*                  _file,
    long                         _line,
    long                         _column,
    const char*                  _format,
    ...
)
{
    va_list arguments;

    va_start(arguments,
             _format);
    d_cli_vreport(_program,
                  _severity,
                  _file,
                  _line,
                  _column,
                  _format,
                  arguments);
    va_end(arguments);

    return;
}

/*
d_cli_note
  A note, when the verbosity reaches the level.
*/
void d_cli_note(
    const struct dj_cli_program* _program,
    int                          _level,
    const char*                  _format,
    ...
)
{
    // too quiet for it
    if ( (!_program) ||
         (_program->verbosity < _level) )
    {
        return;
    }

    va_list arguments;

    va_start(arguments,
             _format);
    d_cli_vreport(_program,
                  (int)D_CLI_SEVERITY_NOTE,
                  NULL,
                  0L,
                  0L,
                  _format,
                  arguments);
    va_end(arguments);

    return;
}

/*
d_cli_command_path
  The names from the root down, collected upward then written in order.
Option groups -- nameless subcommands -- add nothing.
*/
int d_cli_command_path(
    const struct dj_arg_command* _command,
    struct dj_arg_buffer*        _out
)
{
    // parameter validation
    if ( (!_command) ||
         (!_out) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const struct dj_arg_command* chain[32];
    size_t                      depth = 0u;

    // up to the root, or as deep as the grammar allows and then some
    for (const struct dj_arg_command* at = _command;
         (at != NULL) && (depth < (sizeof(chain) / sizeof(chain[0])));
         at = at->parent)
    {
        if (!d_arg_command_is_option_group(at))
        {
            chain[depth++] = at;
        }
    }

    int status = D_ARG_OK;

    // root first
    while ( (status == D_ARG_OK) &&
            (depth > 0u) )
    {
        --depth;
        status = d_arg_buffer_append_cstr(_out,
                                          d_arg_command_name(chain[depth]));

        if ( (status == D_ARG_OK) &&
             (depth > 0u) )
        {
            status = d_arg_buffer_append_char(_out,
                                              ' ');
        }
    }

    return status;
}

/**
 * @brief Appends `Try 'PATH --help' for more information.` and a newline.
 *
 * @param[in]     _command  the command whose help to point to.
 * @param[in,out] _out      the buffer.
 * @return D_ARG_OK, or the buffer's status.
 */
static
int d_cli_internal_try(
    const struct dj_arg_command* _command,
    struct dj_arg_buffer*        _out
)
{
    int status = d_arg_buffer_append_cstr(_out,
                                          "Try '");

    status = (status == D_ARG_OK)
             ? d_cli_command_path(_command,
                                  _out)
             : status;

    return (status == D_ARG_OK)
           ? d_arg_buffer_append_cstr(_out,
                                      " --help' for more information.\n")
           : status;
}

/**
 * @brief Appends a command's place -- its full name less the program's own,
 *        which the locus gives -- and a colon: `gen: `, `example list: `, and
 *        `gui: ` for a module's root, whose parser is named `bitwisedom gui`;
 *        nothing for the program's root.
 *
 * @param[in]     _program  the program.
 * @param[in]     _command  the command.
 * @param[in,out] _out      the buffer.
 * @return D_ARG_OK, or the buffer's status.
 */
static
int d_cli_internal_place(
    const struct dj_cli_program* _program,
    const struct dj_arg_command* _command,
    struct dj_arg_buffer*        _out
)
{
    // no command to name
    if (!_command)
    {
        return D_ARG_OK;
    }

    struct dj_arg_buffer path   = D_ARG_BUFFER_INIT;
    int                 status = d_cli_command_path(_command,
                                                    &path);
    const char*         rest   = (path.data != NULL) ? path.data : "";
    const size_t        own    = strlen(_program->name);

    // the program's own name, and the space after it
    if ( (strncmp(rest,
                  _program->name,
                  own) == 0)            &&
         ( (rest[own] == ' ') ||
           (rest[own] == '\0') ) )
    {
        rest += (rest[own] == ' ') ? (own + 1u) : own;
    }

    // what is left names the place
    if ( (status == D_ARG_OK) &&
         (rest[0] != '\0') )
    {
        status = d_arg_buffer_append_cstr(_out,
                                          rest);
        status = (status == D_ARG_OK)
                 ? d_arg_buffer_append_cstr(_out,
                                            ": ")
                 : status;
    }

    d_arg_buffer_release(&path);

    return status;
}

/**
 * @brief d_cli_usage_error's work, its arguments in a va_list: the locus,
 *        the command's place, the message, then the pointer to help --
 *        composed whole and written once, or composed in a room of its own
 *        where there was no memory to compose it.
 *
 * @param[in] _program    the program.
 * @param[in] _command    the command.
 * @param[in] _format     the message's format.
 * @param[in] _arguments  its arguments.
 */
static
void d_cli_internal_vusage(
    const struct dj_cli_program* _program,
    const struct dj_arg_command* _command,
    const char*                  _format,
    va_list                      _arguments
)
{
    const struct dj_cli_internal_shape shape  = { (int)D_CLI_SEVERITY_ERROR,
                                                 NULL,
                                                 0L,
                                                 0L,
                                                 _command,
                                                 false,
                                                 true };
    struct dj_arg_buffer               out    = D_ARG_BUFFER_INIT;
    int                               status = d_cli_internal_locus(
                                                   _program,
                                                   (int)D_CLI_SEVERITY_ERROR,
                                                   NULL,
                                                   0L,
                                                   0L,
                                                   &out);
    va_list                           spare;

    va_copy(spare,
            _arguments);
    status = (status == D_ARG_OK)
             ? d_cli_internal_place(_program,
                                    _command,
                                    &out)
             : status;
    status = (status == D_ARG_OK)
             ? d_cli_internal_vappend(&out,
                                      _format,
                                      _arguments)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_buffer_append_char(&out,
                                        '\n')
             : status;
    status = (status == D_ARG_OK)
             ? d_cli_internal_try(_command,
                                  &out)
             : status;

    // written whole, or in a room of its own
    if (status == D_ARG_OK)
    {
        d_cli_internal_emit(_program,
                            &out);
    }
    else
    {
        (void)d_cli_internal_spare(_program,
                                   &shape,
                                   _program->err,
                                   _format,
                                   spare);
    }

    va_end(spare);
    d_arg_buffer_release(&out);

    return;
}

/*
d_cli_usage_error
  The locus, the command's place, the message; then the pointer to help.
*/
int d_cli_usage_error(
    const struct dj_cli_program* _program,
    const struct dj_arg_command* _command,
    const char*                  _format,
    ...
)
{
    va_list arguments;

    // nothing to say it with
    if ( (!_program) ||
         (!_format) )
    {
        return D_CLI_EXIT_TROUBLE;
    }

    va_start(arguments,
             _format);
    d_cli_internal_vusage(_program,
                          (_command != NULL) ? _command
                                             : &_program->parser->command,
                          _format,
                          arguments);
    va_end(arguments);

    return D_CLI_EXIT_TROUBLE;
}

/*
d_cli_file_error
  `cannot DOING 'PATH': REASON`, the standard streams named for what they
are; the want of memory, `out of memory`, whatever the file (GRAMMAR.md,
section 8).
*/
int d_cli_file_error(
    const struct dj_cli_program* _program,
    const char*                  _doing,
    const char*                  _path,
    int                          _errnum
)
{
    const char* const doing = (_doing != NULL) ? _doing : "open";

    // no room: said as every want of memory is
    if (_errnum == ENOMEM)
    {
        d_cli_report(_program,
                     (int)D_CLI_SEVERITY_ERROR,
                     "%s",
                     "out of memory");

        return D_CLI_EXIT_TROUBLE;
    }

    // a standard stream is named, not quoted
    if (d_cli_is_standard(_path))
    {
        const bool reads = ( (strcmp(doing,
                                     "read") == 0) ||
                             (strcmp(doing,
                                     "open") == 0) );

        d_cli_report(_program,
                     (int)D_CLI_SEVERITY_ERROR,
                     "cannot %s %s: %s",
                     doing,
                     (reads) ? "standard input" : "standard output",
                     d_cli_internal_error_text(_errnum));

        return D_CLI_EXIT_TROUBLE;
    }

    d_cli_report(_program,
                 (int)D_CLI_SEVERITY_ERROR,
                 "cannot %s '%s': %s",
                 doing,
                 (_path != NULL) ? _path : "",
                 d_cli_internal_error_text(_errnum));

    return D_CLI_EXIT_TROUBLE;
}


//==============================================================================
// 3.  THE PARSER'S FAILURES
//==============================================================================

// D_INTERNAL_CLI_NOT_EXPECTED_ONE / _MANY
//   constants: how the parser begins a message about arguments no command
// took -- CLI11's ExtrasError words.
#define D_INTERNAL_CLI_NOT_EXPECTED_ONE                                        \
    "The following argument was not expected: "
#define D_INTERNAL_CLI_NOT_EXPECTED_MANY                                       \
    "The following arguments were not expected: "

/**
 * @brief The command a parse reached: from the root of a command's tree, down
 *        the subcommands each parsed, the last of each, to one that parsed
 *        none -- where a user's mistake is, whichever command the parser
 *        reports it from: with fallthrough, an option a subcommand did not
 *        know is reported from the root.
 *
 * @param[in] _command  any command of the tree.
 * @return the command reached.
 */
static
const struct dj_arg_command* d_cli_internal_deepest(
    const struct dj_arg_command* _command
)
{
    const struct dj_arg_command* at = _command;

    while (at->parent != NULL)
    {
        at = at->parent;
    }

    for (;;)
    {
        const size_t count = d_arg_command_parsed_subcommand_count(at);
        const struct dj_arg_command* const next =
            (count > 0u)
            ? d_arg_command_parsed_subcommand_at(at,
                                                 count - 1u)
            : NULL;

        // a command that went no deeper
        if ( (!next) ||
             (d_arg_command_is_option_group(next)) )
        {
            return at;
        }

        at = next;
    }
}

/**
 * @brief Whether a command lists subcommands a user may name: one that is
 *        neither an option group, nor hidden, nor disabled.
 *
 * @param[in] _command  the command.
 * @return `true` if it has one.
 */
static
bool d_cli_internal_has_commands(
    const struct dj_arg_command* _command
)
{
    for (size_t index = 0u; index < _command->subcommands.count; ++index)
    {
        const struct dj_arg_command* const each =
            _command->subcommands.items[index];

        // a named subcommand, shown and enabled
        if ( (!d_arg_command_is_option_group(each))                     &&
             (each->group == NULL || each->group[0] != '\0')            &&
             (!d_arg_command_has_flag(each,
                                      D_ARG_COMMAND_DISABLED)) )
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief Drops the root's name and `: ` from the front of a line of a
 *        parser's message: the program's own locus gives it.
 *
 * @param[in]     _command  any command of the tree.
 * @param[in,out] _text     the line, moved past what is dropped.
 * @param[in,out] _length   its length, less what is dropped.
 */
static
void d_cli_internal_drop_root(
    const struct dj_arg_command* _command,
    const char**                 _text,
    size_t*                      _length
)
{
    const struct dj_arg_command* root = _command;

    while (root->parent != NULL)
    {
        root = root->parent;
    }

    const char* const name   = d_arg_command_name(root);
    const size_t      length = strlen(name);

    // `ROOT: `, and something after it
    if ( (*_length > length + 2u)             &&
         (strncmp(*_text,
                  name,
                  length) == 0)               &&
         ((*_text)[length] == ':')            &&
         ((*_text)[length + 1u] == ' ') )
    {
        *_text   += length + 2u;
        *_length -= length + 2u;
    }

    return;
}

/**
 * @brief The word a line of a parser's message says was not expected first,
 *        when it stands where a command was due: the command parsed none,
 *        has some a user may name, and the word is no option. The words
 *        after it are that word's arguments, and say nothing more.
 *
 * @param[in]  _command  the command the error arose in.
 * @param[in]  _text     the line.
 * @param[in]  _length   its length.
 * @param[out] _word     receives the word's length: the rest of the line
 *                       when it names one argument, up to the first space
 *                       when it names several.
 * @return the word; NULL for any other line.
 */
static
const char* d_cli_internal_stray_word(
    const struct dj_arg_command* _command,
    const char*                  _text,
    size_t                       _length,
    size_t*                      _word
)
{
    const size_t      one   = sizeof(D_INTERNAL_CLI_NOT_EXPECTED_ONE) - 1u;
    const size_t      many  = sizeof(D_INTERNAL_CLI_NOT_EXPECTED_MANY) - 1u;
    const bool        alone = ( (_length > one) &&
                                (strncmp(_text,
                                         D_INTERNAL_CLI_NOT_EXPECTED_ONE,
                                         one) == 0) );
    const bool        first = ( (_length > many) &&
                                (strncmp(_text,
                                         D_INTERNAL_CLI_NOT_EXPECTED_MANY,
                                         many) == 0) );
    const char* const word  = _text + ((alone) ? one : many);
    const char* const space = (first)
                              ? (const char*)memchr(word,
                                                    ' ',
                                                    _length - many)
                              : NULL;

    *_word = (space != NULL)
             ? (size_t)(space - word)
             : (_length - (size_t)(word - _text));

    return ( ( (alone) || (first) )                                  &&
             (word[0] != '-')                                        &&
             (d_arg_command_parsed_subcommand_count(_command) == 0u) &&
             (d_cli_internal_has_commands(_command)) )
           ? word
           : NULL;
}

/**
 * @brief Appends `PLACE: 'WORD' is not a command`.
 *
 * @param[in]     _program  the program.
 * @param[in]     _command  the command the word was given to.
 * @param[in]     _word     the word.
 * @param[in]     _length   its length.
 * @param[in,out] _out      the buffer.
 * @return D_ARG_OK, or the buffer's status.
 */
static
int d_cli_internal_not_a_command(
    const struct dj_cli_program* _program,
    const struct dj_arg_command* _command,
    const char*                  _word,
    size_t                       _length,
    struct dj_arg_buffer*        _out
)
{
    int status = d_cli_internal_place(_program,
                                      _command,
                                      _out);

    status = (status == D_ARG_OK)
             ? d_arg_buffer_append_char(_out,
                                        '\'')
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_buffer_append(_out,
                                   _word,
                                   _length)
             : status;

    return (status == D_ARG_OK)
           ? d_arg_buffer_append_cstr(_out,
                                      "' is not a command")
           : status;
}

/**
 * @brief Writes the first line of a parser's message in the GNU shape: the
 *        root's own name dropped from its front, the command's place put
 *        there where the message does not name it, and an unknown word
 *        where a command was due said to be no command.
 *
 * @param[in]     _program  the program.
 * @param[in]     _command  the command the error arose in.
 * @param[in]     _text     the line.
 * @param[in]     _length   its length.
 * @param[in,out] _out      the buffer.
 * @return D_ARG_OK, or the buffer's status.
 */
static
int d_cli_internal_first_line(
    const struct dj_cli_program* _program,
    const struct dj_arg_command* _command,
    const char*                  _text,
    size_t                       _length,
    struct dj_arg_buffer*        _out
)
{
    d_cli_internal_drop_root(_command,
                             &_text,
                             &_length);

    size_t            word_length = 0u;
    const char* const word        = d_cli_internal_stray_word(_command,
                                                              _text,
                                                              _length,
                                                              &word_length);

    // a word where a command was due: no such command
    if (word != NULL)
    {
        return d_cli_internal_not_a_command(_program,
                                            _command,
                                            word,
                                            word_length,
                                            _out);
    }

    // the command's place, unless the message begins with its name
    const char* const own        = d_arg_command_name(_command);
    const size_t      own_length = strlen(own);
    const bool        named      = ( (_command->parent != NULL)        &&
                                     (_length > own_length + 1u)       &&
                                     (strncmp(_text,
                                              own,
                                              own_length) == 0)        &&
                                     (_text[own_length] == ':') );
    int               status     = (named)
                                   ? D_ARG_OK
                                   : d_cli_internal_place(_program,
                                                          _command,
                                                          _out);

    // a command's own name before the colon keeps its case
    if ( (status == D_ARG_OK) &&
         (named) )
    {
        status = d_arg_buffer_append(_out,
                                     _text,
                                     own_length + 2u);
        _text   += own_length + 2u;
        _length -= own_length + 2u;
    }

    return (status == D_ARG_OK)
           ? d_cli_internal_append_lowered(_out,
                                           _text,
                                           _length)
           : status;
}

/**
 * @brief Appends one line of a parser's message as a diagnostic: the first
 *        an error, in the GNU shape; each further one -- a suggestion -- a
 *        note.
 *
 * @param[in]     _program  the program.
 * @param[in]     _command  the command the error arose in.
 * @param[in]     _text     the line.
 * @param[in]     _length   its length.
 * @param[in]     _first    whether it is the message's first.
 * @param[in,out] _out      the buffer.
 * @return D_ARG_OK, or the buffer's status.
 */
static
int d_cli_internal_message_line(
    const struct dj_cli_program* _program,
    const struct dj_arg_command* _command,
    const char*                  _text,
    size_t                       _length,
    bool                         _first,
    struct dj_arg_buffer*        _out
)
{
    int status = d_cli_internal_locus(_program,
                                      (_first)
                                      ? (int)D_CLI_SEVERITY_ERROR
                                      : (int)D_CLI_SEVERITY_NOTE,
                                      NULL,
                                      0L,
                                      0L,
                                      _out);

    status = (status != D_ARG_OK)
             ? status
             : (_first)
               ? d_cli_internal_first_line(_program,
                                           _command,
                                           _text,
                                           _length,
                                           _out)
               : d_cli_internal_append_lowered(_out,
                                               _text,
                                               _length);

    return (status == D_ARG_OK)
           ? d_arg_buffer_append_char(_out,
                                      '\n')
           : status;
}

/**
 * @brief Writes a buffer to a sink in one write.
 *
 * @param[in] _sink  the sink; one with no write function takes nothing.
 * @param[in] _out   the buffer.
 * @return D_ARG_OK, or D_ARG_ERROR_FILE when the sink took less than all.
 */
static
int d_cli_internal_to_sink(
    struct dj_pack_sink         _sink,
    const struct dj_arg_buffer* _out
)
{
    return ( (_sink.write != NULL) &&
             (_sink.write(_sink.context,
                          _out->data,
                          _out->length) != _out->length) )
           ? D_ARG_ERROR_FILE
           : D_ARG_OK;
}

/**
 * @brief The kit's failure message where there was no memory to compose it:
 *        the parser's first line as the error, its first letter lowered,
 *        then -- unless the error is itself a want of memory -- the pointer
 *        to the command's help, composed in a room of its own.
 *
 * @param[in] _program  the program.
 * @param[in] _command  the command the error arose in.
 * @param[in] _error    the parser's error.
 * @param[in] _sink     where the message goes.
 * @return D_ARG_OK, or D_ARG_ERROR_FILE when the sink took less than all.
 */
static
int d_cli_internal_spare_failure(
    const struct dj_cli_program* _program,
    const struct dj_arg_command* _command,
    const struct dj_arg_error*   _error,
    struct dj_pack_sink          _sink
)
{
    const char* const                 said    = d_arg_error_message(_error);
    const bool                        pointer =
        ( (_error == NULL) ||
          (_error->status != (int)D_ARG_ERROR_OUT_OF_MEMORY) );
    const struct dj_cli_internal_shape shape   = { (int)D_CLI_SEVERITY_ERROR,
                                                  NULL,
                                                  0L,
                                                  0L,
                                                  _command,
                                                  true,
                                                  pointer };
    const char* const                 text    = (said != NULL) ? said : "";
    const char* const                 end     = strchr(text,
                                                       '\n');

    return d_cli_internal_spare_say(_program,
                                    &shape,
                                    _sink,
                                    "%.*s",
                                    (end != NULL) ? (int)(end - text)
                                                  : (int)strlen(text),
                                    text);
}

/**
 * @brief The kit's failure message: the parser's message, its first line an
 *        error and every further line -- a suggestion -- a note, then the
 *        pointer to the command's help, but after a want of memory, which
 *        no help mends. A dj_arg_failure_fn.
 *
 * @param[in] _command  the command the error is reported from.
 * @param[in] _error    the error.
 * @param[in] _sink     where the message goes.
 * @param[in] _context  the program.
 * @return D_ARG_OK, or the status that stopped it.
 */
static
int d_cli_internal_failure(
    const struct dj_arg_command* _command,
    const struct dj_arg_error*   _error,
    struct dj_pack_sink          _sink,
    void*                        _context
)
{
    const struct dj_cli_program* const program =
        (const struct dj_cli_program*)_context;
    const struct dj_arg_command* const command =
        d_cli_internal_deepest(_command);
    const char*                       text    = d_arg_error_message(_error);
    struct dj_arg_buffer               out     = D_ARG_BUFFER_INIT;
    int                               status  = D_ARG_OK;
    bool                              first   = true;

    // each line of the message; a blank one says nothing
    while ( (status == D_ARG_OK) &&
            (text != NULL)       &&
            (*text != '\0') )
    {
        const char* const end    = strchr(text,
                                          '\n');
        const size_t      length = (end != NULL)
                                   ? (size_t)(end - text)
                                   : strlen(text);

        if (length > 0u)
        {
            status = d_cli_internal_message_line(program,
                                                 command,
                                                 text,
                                                 length,
                                                 first,
                                                 &out);
            first  = false;
        }

        text = (end != NULL) ? (end + 1) : NULL;
    }

    // the pointer to help, unless no help mends it, and one write
    status = ( (status == D_ARG_OK) &&
               ( (_error == NULL) ||
                 (_error->status != (int)D_ARG_ERROR_OUT_OF_MEMORY) ) )
             ? d_cli_internal_try(command,
                                  &out)
             : status;
    status = (status == D_ARG_OK)
             ? d_cli_internal_to_sink(_sink,
                                      &out)
             : (status == D_ARG_ERROR_OUT_OF_MEMORY)
             ? d_cli_internal_spare_failure(program,
                                            command,
                                            _error,
                                            _sink)
             : status;

    d_arg_buffer_release(&out);

    return status;
}


//==============================================================================
// 4.  THE STANDARD COMMANDS
//==============================================================================

// dj_cli_internal_watch
//   struct: a sink watched: the sink, and errno as the first write that took
// less than all left it -- 0 when none did, or when the sink set none.
struct dj_cli_internal_watch
{
    struct dj_pack_sink sink;
    int                errnum;
};

/**
 * @brief A watched sink's write: the sink's own, errno kept when it took
 *        less than all. A dj_pack_sink_write_fn.
 *
 * @param[in,out] _context  the watch.
 * @param[in]     _data     the bytes.
 * @param[in]     _size     how many.
 * @return how many the sink took; all, for a sink with no write function,
 *         which takes nothing and fails at nothing.
 */
static
size_t d_cli_internal_watch_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    struct dj_cli_internal_watch* const watch =
        (struct dj_cli_internal_watch*)_context;

    if (watch->sink.write == NULL)
    {
        return _size;
    }

    errno = 0;

    const size_t taken = watch->sink.write(watch->sink.context,
                                           _data,
                                           _size);

    // the first refusal's reason, as the system gave it
    if ( (taken < _size) &&
         (watch->errnum == 0) )
    {
        watch->errnum = errno;
    }

    return taken;
}

/**
 * @brief A program's output, watched.
 *
 * @param[in]  _program  the program.
 * @param[out] _watch    the watch, emptied.
 * @return the sink to write to.
 */
static
struct dj_pack_sink d_cli_internal_watched(
    const struct dj_cli_program*  _program,
    struct dj_cli_internal_watch* _watch
)
{
    const struct dj_pack_sink sink = { d_cli_internal_watch_write,
                                      _watch };

    _watch->sink   = _program->out;
    _watch->errnum = 0;

    return sink;
}

/**
 * @brief Says why what a program was to write to its output was not
 *        written: the output's refusal -- with the system's reason when it
 *        gave one -- or, for anything else that stopped it, the want of
 *        memory.
 *
 * @param[in] _program  the program.
 * @param[in] _status   what stopped it: D_ARG_ERROR_FILE for the output.
 * @param[in] _errnum   the reason the output gave; 0 for none.
 * @return D_CLI_EXIT_TROUBLE.
 */
static
int d_cli_internal_unwritten(
    const struct dj_cli_program* _program,
    int                          _status,
    int                          _errnum
)
{
    // the output's refusal, and its reason
    if ( (_status == (int)D_ARG_ERROR_FILE) &&
         (_errnum != 0) )
    {
        return d_cli_file_error(_program,
                                "write",
                                "-",
                                _errnum);
    }

    d_cli_report(_program,
                 (int)D_CLI_SEVERITY_ERROR,
                 "%s",
                 (_status == (int)D_ARG_ERROR_FILE)
                 ? "cannot write standard output"
                 : "out of memory");

    return D_CLI_EXIT_TROUBLE;
}

/**
 * @brief Writes a command's help -- its own, never that of a subcommand the
 *        last parse used -- to a sink.
 *
 * @param[in] _command  the command.
 * @param[in] _sink     where it goes.
 * @return D_ARG_OK, or the status that stopped it.
 */
static
int d_cli_internal_help_of(
    const struct dj_arg_command* _command,
    struct dj_pack_sink          _sink
)
{
    struct dj_arg_buffer name   = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer out    = D_ARG_BUFFER_INIT;
    int                 status = d_cli_command_path(_command,
                                                    &name);

    status = (status == D_ARG_OK)
             ? d_arg_format_help(d_arg_command_formatter(_command),
                                 _command,
                                 d_arg_buffer_text(&name),
                                 (int)D_ARG_HELP_NORMAL,
                                 &out)
             : status;

    status = (status == D_ARG_OK)
             ? d_cli_internal_to_sink(_sink,
                                      &out)
             : status;

    d_arg_buffer_release(&name);
    d_arg_buffer_release(&out);

    return status;
}

/**
 * @brief The `help` command's work: the help of the command its words name,
 *        or the program's. A dj_cli_command_fn.
 *
 * @param[in] _program  the program.
 * @param[in] _command  the `help` command.
 * @param[in] _context  unused.
 * @return D_CLI_EXIT_OK, or D_CLI_EXIT_TROUBLE for a word that names no
 *         command or help that cannot be written.
 */
static
int d_cli_internal_help_command(
    struct dj_cli_program* _program,
    struct dj_arg_command* _command,
    void*                  _context
)
{
    struct dj_arg_option* const words  = d_arg_command_option(_command,
                                                             "COMMAND");
    size_t                     count  = 0u;
    const struct dj_pack_text*  values = d_arg_option_values(words,
                                                            &count);
    struct dj_arg_command*      target = &_program->parser->command;

    (void)_context;

    // down the tree, a word at a time
    for (size_t index = 0u; index < count; ++index)
    {
        struct dj_arg_command* const next =
            d_arg_command_subcommand(target,
                                     values[index].data);

        // a word that names no command here
        if ( (!next) ||
             (d_arg_command_is_option_group(next)) )
        {
            return d_cli_usage_error(_program,
                                     _command,
                                     "'%s' is not a command",
                                     values[index].data);
        }

        target = next;
    }

    struct dj_cli_internal_watch watch;
    const int                   written = d_cli_internal_help_of(
                                              target,
                                              d_cli_internal_watched(_program,
                                                                     &watch));

    return (written == D_ARG_OK)
           ? D_CLI_EXIT_OK
           : d_cli_internal_unwritten(_program,
                                      written,
                                      watch.errnum);
}

#if (D_INTERNAL_ARG_PARSER_COMPLETION == 1)

/**
 * @brief The `completion` command's work: the script for the shell named.
 *        A dj_cli_command_fn.
 *
 * @param[in] _program  the program.
 * @param[in] _command  the `completion` command.
 * @param[in] _context  unused.
 * @return D_CLI_EXIT_OK, or D_CLI_EXIT_TROUBLE for a shell the parser knows
 *         no script for, or a script that cannot be written.
 */
static
int d_cli_internal_completion_command(
    struct dj_cli_program* _program,
    struct dj_arg_command* _command,
    void*                  _context
)
{
    struct dj_arg_option* const shell  = d_arg_command_option(_command,
                                                             "SHELL");
    size_t                     count  = 0u;
    const struct dj_pack_text*  values = d_arg_option_values(shell,
                                                            &count);
    const int                  which  = (count > 0u)
                                        ? d_arg_shell_from_name(values[0])
                                        : -1;

    (void)_context;

    // a shell there is no script for
    if (which < 0)
    {
        return d_cli_usage_error(_program,
                                 _command,
                                 "'%s' is not a shell: bash, zsh, fish or "
                                 "powershell",
                                 (count > 0u) ? values[0].data : "");
    }

    struct dj_cli_internal_watch watch;
    const int                   written = d_arg_completion_script(
                                              &_program->parser->command,
                                              which,
                                              d_pack_text_from_cstr(
                                                  _program->name),
                                              d_cli_internal_watched(_program,
                                                                     &watch));

    return (written == D_ARG_OK)
           ? D_CLI_EXIT_OK
           : d_cli_internal_unwritten(_program,
                                      written,
                                      watch.errnum);
}

#endif  // D_INTERNAL_ARG_PARSER_COMPLETION

/**
 * @brief Adds `help` and `completion` to a program with commands.
 *
 * @param[in,out] _program  the program.
 * @return D_ARG_OK, or the first definition's failure.
 */
static
int d_cli_internal_standard_commands(
    struct dj_cli_program* _program
)
{
    _program->help_command =
        d_cli_program_add_command(_program,
                                  NULL,
                                  "help",
                                  "display help for a command",
                                  d_cli_internal_help_command,
                                  NULL,
                                  NULL);

    struct dj_arg_option* const words =
        (_program->help_command != NULL)
        ? d_arg_command_add_option(_program->help_command,
                                   "COMMAND",
                                   "the command, and its subcommand")
        : NULL;

    // the help command and its words
    if (!words)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    d_arg_option_set_expected_range(words,
                                    0,
                                    2);

#if (D_INTERNAL_ARG_PARSER_COMPLETION == 1)
    _program->completion_command =
        d_cli_program_add_command(_program,
                                  NULL,
                                  "completion",
                                  "print a completion script for a shell",
                                  d_cli_internal_completion_command,
                                  NULL,
                                  NULL);

    struct dj_arg_option* const shell =
        (_program->completion_command != NULL)
        ? d_arg_command_add_option(_program->completion_command,
                                   "SHELL",
                                   "bash, zsh, fish or powershell")
        : NULL;

    if (!shell)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    d_arg_option_set_required(shell,
                              true);
#endif  // D_INTERNAL_ARG_PARSER_COMPLETION

    return D_ARG_OK;
}


//==============================================================================
// 5.  LIFETIME
//==============================================================================

/**
 * @brief Why a definition of the kit's own came back NULL: what the parser
 *        recorded -- a name the program had already used, say -- or, with
 *        nothing recorded, the want of memory.
 *
 * @param[in] _program  the program.
 * @return the parser's definition error's status, or
 *         D_ARG_ERROR_OUT_OF_MEMORY.
 */
static
int d_cli_internal_refused(
    const struct dj_cli_program* _program
)
{
    const struct dj_arg_error* const error =
        d_arg_parser_definition_error(_program->parser);

    return ( (error != NULL) &&
             (error->status != (int)D_ARG_OK) )
           ? error->status
           : D_ARG_ERROR_OUT_OF_MEMORY;
}

/**
 * @brief Sets the root's version flag: `--version`, which prints `PROGRAM
 *        VERSION` as GNU programs print it; none for a program without a
 *        version.
 *
 * @param[in,out] _program  the program, its name and version set.
 * @return D_ARG_OK; D_ARG_ERROR_OUT_OF_MEMORY when its text could not be
 *         made; or what the parser recorded when the flag could not be.
 */
static
int d_cli_internal_version_flag(
    struct dj_cli_program* _program
)
{
    struct dj_arg_command* const root = &_program->parser->command;

    // no version, and no flag
    if ( (_program->version == NULL) ||
         (_program->version[0] == '\0') )
    {
        (void)d_arg_command_set_version_flag(root,
                                             NULL,
                                             NULL,
                                             NULL);

        return D_ARG_OK;
    }

    struct dj_arg_buffer              text   = D_ARG_BUFFER_INIT;
    const int                        status = d_arg_buffer_printf(
                                                  &text,
                                                  "%s %s",
                                                  _program->name,
                                                  _program->version);
    const struct dj_arg_option* const flag   =
        (status == D_ARG_OK)
        ? d_arg_command_set_version_flag(root,
                                         "--version",
                                         text.data,
                                         "output version information and "
                                         "exit")
        : NULL;

    d_arg_buffer_release(&text);

    return (flag != NULL)
           ? D_ARG_OK
           : (status != D_ARG_OK)
           ? status
           : d_cli_internal_refused(_program);
}

/**
 * @brief Adds `--color` to the root, and the `--colour` help does not show,
 *        each taking WHEN.
 *
 * @param[in,out] _program  the program.
 * @return D_ARG_OK, or why the options could not be made.
 */
static
int d_cli_internal_colour_flags(
    struct dj_cli_program* _program
)
{
    struct dj_arg_command* const root = &_program->parser->command;

    _program->colour_option = d_arg_command_add_flag(
                                  root,
                                  "--color{always}",
                                  "colour diagnostics: auto (the default), "
                                  "always or never");
    _program->colour_alias  = d_arg_command_add_flag(
                                  root,
                                  "--colour{always}",
                                  "--color");

    // each made, or why not
    if ( (!_program->colour_option) ||
         (!_program->colour_alias) )
    {
        return d_cli_internal_refused(_program);
    }

    // `--colour` is taken and not shown, as GNU grep takes it
    int status = d_arg_option_set_group(_program->colour_alias,
                                        "");

    status = (status == D_ARG_OK)
             ? d_arg_option_set_type_name(_program->colour_alias,
                                          "WHEN")
             : status;

    return (status == D_ARG_OK)
           ? d_arg_option_set_type_name(_program->colour_option,
                                        "WHEN")
           : status;
}

/**
 * @brief Adds the standard options to the root: help, version, `-q`, `-v`
 *        and `--color`.
 *
 * @param[in,out] _program  the program, its name and version set.
 * @return D_ARG_OK, or the first definition's failure.
 */
static
int d_cli_internal_standard_options(
    struct dj_cli_program* _program
)
{
    struct dj_arg_command* const root = &_program->parser->command;

    // help, in the GNU words
    if (d_arg_command_set_help_flag(root,
                                    "-h,--help",
                                    "display this help and exit") == NULL)
    {
        return d_cli_internal_refused(_program);
    }

    // the version
    const int version = d_cli_internal_version_flag(_program);

    if (version != D_ARG_OK)
    {
        return version;
    }

    _program->quiet   = d_arg_command_add_flag(root,
                                               "-q,--quiet",
                                               "say nothing but errors");
    _program->verbose = d_arg_command_add_flag(root,
                                               "-v,--verbose",
                                               "say more; twice, more again");

    // each made, or why not
    if ( (!_program->quiet) ||
         (!_program->verbose) )
    {
        return d_cli_internal_refused(_program);
    }

    // -v counts: every use is kept
    d_arg_option_set_multi_option_policy(_program->verbose,
                                         (int)D_ARG_MULTI_TAKE_ALL);

    return d_cli_internal_colour_flags(_program);
}

/**
 * @brief Sets a program's parser to read a command line as the grammar
 *        does, and to write and fail through the program.
 *
 * @param[in,out] _program  the program, its fields set.
 * @param[in,out] _parser   its parser.
 * @param[in]     _flags    D_CLI_PROGRAM_* flags.
 */
static
void d_cli_internal_configure(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser,
    uint32_t               _flags
)
{
    struct dj_arg_command* const root = &_parser->command;

    // the grammar's settings, before any definition they should reach
    d_arg_option_defaults_set_multi_option_policy(
        d_arg_command_option_defaults(root),
        (int)D_ARG_MULTI_TAKE_LAST);
    d_arg_command_set_windows_style(root,
                                    false);

    // `--name=` is an empty value, as getopt_long reads it (R7); a prefix
    // of a long name is the name only where the program asks (R5)
    d_arg_parser_set_empty_values(_parser,
                                  true);
    d_arg_parser_set_abbreviations(_parser,
                                   ((_flags &
                                     D_CLI_PROGRAM_ABBREVIATIONS) != 0u));

    // one command a run (R1): at most one subcommand, which each subcommand
    // inherits, so a second command's name is an operand nobody took
    d_arg_command_require_subcommand(root,
                                     0u,
                                     1u);
#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)
    d_arg_parser_set_response_files(_parser,
                                    ((_flags &
                                      D_CLI_PROGRAM_RESPONSE_FILES) != 0u),
                                    (int)D_ARG_QUOTING_NATIVE_RESPONSE);
#endif

    // the kit's help, unless the program keeps the parser's own; the kit's
    // failures and streams
    if ((_flags & D_CLI_PROGRAM_PARSER_HELP) == 0u)
    {
        d_cli_formatter_use(&_parser->formatter);
        _parser->formatter.context = _program;
    }

    // the parser's own layout, asked for or the only one built: `@FILE` on
    // every page where response files are read (ruling cli-taken 84)
    if ( ((_flags & D_CLI_PROGRAM_RESPONSE_FILES) != 0u) &&
         (_parser->formatter.vtable == NULL) )
    {
        d_cli_internal_parser_layout(&_parser->formatter);
        _parser->formatter.context = _program;
    }

    d_arg_command_set_failure_message(root,
                                      d_cli_internal_failure,
                                      _program,
                                      NULL);
    d_arg_parser_set_output(_parser,
                            _program->out,
                            _program->err);
    _program->colour_output = d_cli_colour_wanted((int)D_CLI_STREAM_OUTPUT,
                                                  _program->colour);
    _program->colour_error  = d_cli_colour_wanted((int)D_CLI_STREAM_ERROR,
                                                  _program->colour);

    return;
}

/*
d_cli_program_init
  The copies, the parser's settings, then the standard options, each
failure releasing what was made before it. The standard commands are
d_cli_program_parse's to add.
*/
int d_cli_program_init(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser,
    const char*            _name,
    const char*            _version,
    uint32_t               _flags
)
{
    // parameter validation
    if ( (!_program) ||
         (!_parser)  ||
         (!_name) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    memset(_program,
           0,
           sizeof(*_program));

    _program->parser  = _parser;
    _program->flags   = _flags;
    _program->colour  = (int)D_CLI_COLOUR_AUTO;
    _program->out     = d_cli_sink_output();
    _program->err     = d_cli_sink_error();
    _program->name    = d_cli_internal_copy(_name);
    _program->version = d_cli_internal_copy((_version != NULL)
                                            ? _version
                                            : "");

    // the copies; a failure kept past the release, with the flags that say
    // how a run says it
    if ( (!_program->name) ||
         (!_program->version) )
    {
        d_cli_program_release(_program);
        _program->flags  = _flags;
        _program->status = D_ARG_ERROR_OUT_OF_MEMORY;

        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    d_cli_internal_configure(_program,
                             _parser,
                             _flags);

    // the standard commands wait for the first parse, to come last
    const int status = d_cli_internal_standard_options(_program);

    // a failure kept past the release, for a run to say, with the flags
    // that say how
    if (status != D_ARG_OK)
    {
        d_cli_program_release(_program);
        _program->flags = _flags;
    }

    _program->status = status;

    return status;
}

/*
d_cli_program_release
  The copies, the kept arguments and build lines, and the contexts handed
over.
*/
void d_cli_program_release(
    struct dj_cli_program* _program
)
{
    // nothing held
    if (!_program)
    {
        return;
    }

    for (size_t index = 0u; index < _program->handler_count; ++index)
    {
        const struct dj_cli_handler* const each = &_program->handlers[index];

        if (each->release != NULL)
        {
            each->release(each->context);
        }
    }

    D_INTERNAL_CLI_FREE(_program->handlers);
    D_INTERNAL_CLI_FREE((void*)_program->arguments);
    D_INTERNAL_CLI_FREE(_program->name);
    D_INTERNAL_CLI_FREE(_program->version);
    D_INTERNAL_CLI_FREE(_program->build_lines);
    D_INTERNAL_CLI_FREE(_program->plugin_path);

    struct dj_arg_parser* const parser = _program->parser;

    memset(_program,
           0,
           sizeof(*_program));
    _program->parser = parser;

    return;
}


//==============================================================================
// 6.  COMMANDS
//==============================================================================

/**
 * @brief Keeps a command's work: replaces what the command had, or adds it.
 *
 * @param[in,out] _program  the program.
 * @param[in]     _command  the command.
 * @param[in]     _run      the work.
 * @param[in]     _context  its context.
 * @param[in]     _release  what frees the context.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY, the context released.
 */
static
int d_cli_internal_keep(
    struct dj_cli_program* _program,
    struct dj_arg_command* _command,
    dj_cli_command_fn      _run,
    void*                  _context,
    dj_arg_release_fn      _release
)
{
    // the command's work already kept: replaced
    for (size_t index = 0u; index < _program->handler_count; ++index)
    {
        struct dj_cli_handler* const each = &_program->handlers[index];

        if (each->command == _command)
        {
            if (each->release != NULL)
            {
                each->release(each->context);
            }

            each->run     = _run;
            each->context = _context;
            each->release = _release;

            return D_ARG_OK;
        }
    }

    // room for one more
    if (_program->handler_count == _program->handler_capacity)
    {
        const size_t room  = (_program->handler_capacity == 0u)
                             ? 8u
                             : (_program->handler_capacity * 2u);
        void* const  grown = D_INTERNAL_CLI_REALLOC(
                                 _program->handlers,
                                 room * sizeof(struct dj_cli_handler));

        // no room: the program is not made whole, which a run will say
        if (!grown)
        {
            if (_release != NULL)
            {
                _release(_context);
            }

            _program->status = D_ARG_ERROR_OUT_OF_MEMORY;

            return D_ARG_ERROR_OUT_OF_MEMORY;
        }

        _program->handlers         = (struct dj_cli_handler*)grown;
        _program->handler_capacity = room;
    }

    struct dj_cli_handler* const made =
        &_program->handlers[_program->handler_count++];

    made->command = _command;
    made->run     = _run;
    made->context = _context;
    made->release = _release;

    return D_ARG_OK;
}

/*
d_cli_program_add_command
  The subcommand, which inherits the root's settings as the parser gives
them, then fallthrough, so the root's options are taken after its name.
*/
struct dj_arg_command* d_cli_program_add_command(
    struct dj_cli_program* _program,
    struct dj_arg_command* _parent,
    const char*            _name,
    const char*            _description,
    dj_cli_command_fn      _run,
    void*                  _context,
    dj_arg_release_fn      _release
)
{
    // parameter validation
    if (!_program)
    {
        if (_release != NULL)
        {
            _release(_context);
        }

        return NULL;
    }

    struct dj_arg_command* const parent  = (_parent != NULL)
                                          ? _parent
                                          : &_program->parser->command;
    struct dj_arg_command* const command =
        d_arg_command_add_subcommand(parent,
                                     _name,
                                     _description);

    // the parser has recorded why
    if (!command)
    {
        if (_release != NULL)
        {
            _release(_context);
        }

        return NULL;
    }

    // the root's options after its name (R2), and one command a run (R1)
    d_arg_command_set_fallthrough(command,
                                  true);
    d_arg_command_set_subcommand_fallthrough(command,
                                             false);

    // the work, kept
    if ( (_run != NULL) &&
         (d_cli_internal_keep(_program,
                              command,
                              _run,
                              _context,
                              _release) != D_ARG_OK) )
    {
        (void)d_arg_command_remove_subcommand(parent,
                                              command);

        return NULL;
    }

    return command;
}

/*
d_cli_program_set_main
  The root's work, kept as any command's is.
*/
int d_cli_program_set_main(
    struct dj_cli_program* _program,
    dj_cli_command_fn      _run,
    void*                  _context,
    dj_arg_release_fn      _release
)
{
    // parameter validation
    if (!_program)
    {
        if (_release != NULL)
        {
            _release(_context);
        }

        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return d_cli_internal_keep(_program,
                               &_program->parser->command,
                               _run,
                               _context,
                               _release);
}


//==============================================================================
// 7.  RUNNING
//==============================================================================

/**
 * @brief The colour option the last parse took last -- `--color` or the
 *        `--colour` help does not show, whichever came later in the root's
 *        parse order -- and the value it took.
 *
 * @param[in]  _program  the program, its parse done.
 * @param[out] _value    receives the value, when one was given.
 * @return the option given last, or NULL when neither was given.
 */
static
const struct dj_arg_option* d_cli_internal_colour_given(
    const struct dj_cli_program* _program,
    struct dj_pack_text*         _value
)
{
    const struct dj_arg_option_list* const order =
        &_program->parser->command.parse_order;

    // from the last argument back
    for (size_t index = order->count; index > 0u; --index)
    {
        const struct dj_arg_option* const each = order->items[index - 1u];

        if ( (each != _program->colour_option) &&
             (each != _program->colour_alias) )
        {
            continue;
        }

        size_t                          count  = 0u;
        const struct dj_pack_text* const values = d_arg_option_values(each,
                                                                     &count);

        if (count > 0u)
        {
            *_value = values[count - 1u];

            return each;
        }
    }

    return NULL;
}

/**
 * @brief Reads the standard options' values after a parse, and decides each
 *        stream's colour.
 *
 * @param[in,out] _program  the program.
 */
static
void d_cli_internal_read_standard(
    struct dj_cli_program* _program
)
{
    // -q silences; otherwise each -v adds to where the program started
    _program->verbosity = (d_arg_option_count(_program->quiet) > 0u)
                          ? -1
                          : (_program->verbosity_base +
                             (int)d_arg_option_count(_program->verbose));

    struct dj_pack_text last = d_pack_text_from_cstr("");

    // --color's last word; one that is none of the three changes nothing
    if (d_cli_internal_colour_given(_program,
                                    &last) != NULL)
    {
        if (d_pack_text_equal(last,
                              d_pack_text_from_cstr("always")))
        {
            _program->colour = (int)D_CLI_COLOUR_ALWAYS;
        }
        else if (d_pack_text_equal(last,
                                   d_pack_text_from_cstr("never")))
        {
            _program->colour = (int)D_CLI_COLOUR_NEVER;
        }
        else if (d_pack_text_equal(last,
                                   d_pack_text_from_cstr("auto")))
        {
            _program->colour = (int)D_CLI_COLOUR_AUTO;
        }
    }

    _program->colour_output = d_cli_colour_wanted((int)D_CLI_STREAM_OUTPUT,
                                                  _program->colour);
    _program->colour_error  = d_cli_colour_wanted((int)D_CLI_STREAM_ERROR,
                                                  _program->colour);

    return;
}

/**
 * @brief Whether `--color`'s last value is one of its three words; a usage
 *        error otherwise, which it reports.
 *
 * @param[in] _program  the program, its parse done.
 * @return `true` if the value is one of the three, or none was given.
 */
static
bool d_cli_internal_colour_valid(
    const struct dj_cli_program* _program
)
{
    struct dj_pack_text               last  = d_pack_text_from_cstr("");
    const struct dj_arg_option* const given = d_cli_internal_colour_given(
                                                 _program,
                                                 &last);

    // nothing given, or a word it knows
    if ( (!given) ||
         (d_pack_text_equal(last,
                            d_pack_text_from_cstr("always"))) ||
         (d_pack_text_equal(last,
                            d_pack_text_from_cstr("never")))  ||
         (d_pack_text_equal(last,
                            d_pack_text_from_cstr("auto"))) )
    {
        return true;
    }

    (void)d_cli_usage_error(_program,
                            NULL,
                            "--%s: '%.*s' is not auto, always or never",
                            (given == _program->colour_alias)
                            ? "colour"
                            : "color",
                            (int)last.length,
                            last.data);

    return false;
}

/**
 * @brief Finds a command's work.
 *
 * @param[in] _program  the program.
 * @param[in] _command  the command.
 * @return the work, or NULL.
 */
static
const struct dj_cli_handler* d_cli_internal_handler(
    const struct dj_cli_program* _program,
    const struct dj_arg_command* _command
)
{
    for (size_t index = 0u; index < _program->handler_count; ++index)
    {
        if (_program->handlers[index].command == _command)
        {
            return &_program->handlers[index];
        }
    }

    return NULL;
}

/**
 * @brief How many times the last parse gave an option: its entries in the
 *        parse order of the command that holds it -- an option group, for an
 *        option of one -- which lists an option once for each argument it
 *        takes, its delimiter's splits not counted.
 *
 * @param[in] _option  the option, or NULL.
 * @return the count; 0 for NULL.
 */
static
size_t d_cli_internal_times_given(
    const struct dj_arg_option* _option
)
{
    // no option, or none a command holds
    if ( (!_option) ||
         (!_option->parent) )
    {
        return 0u;
    }

    const struct dj_arg_option_list* const order =
        &_option->parent->parse_order;
    size_t                                times = 0u;

    for (size_t index = 0u; index < order->count; ++index)
    {
        if (order->items[index] == _option)
        {
            ++times;
        }
    }

    return times;
}

/**
 * @brief Whether any value an option was given differs from the one taken.
 *
 * @param[in] _option  the option, or NULL.
 * @param[in] _last    the value taken.
 * @return `true` if one does.
 */
static
bool d_cli_internal_differs(
    const struct dj_arg_option* _option,
    struct dj_pack_text         _last
)
{
    size_t                          count  = 0u;
    const struct dj_pack_text* const values = d_arg_option_results(_option,
                                                                  &count);

    for (size_t index = 0u; index < count; ++index)
    {
        if (!d_pack_text_equal(values[index],
                               _last))
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief Whether an option keeps one value, its last, from a command line
 *        that gives it more than once: named, not an operand; the last
 *        taken by its policy; and at most one value each time it is given,
 *        so a flag and a `--x`/`--no-x` pair, and not a list.
 *
 * @param[in] _option  the option.
 * @return `true` if it does.
 */
static
bool d_cli_internal_keeps_last(
    const struct dj_arg_option* _option
)
{
    return ( (d_arg_option_is_nonpositional(_option))                   &&
             (_option->multi_option_policy ==
                  (int)D_ARG_MULTI_TAKE_LAST)                           &&
             (d_arg_option_items_expected_max(_option) <= 1)            &&
             ( (_option->long_names.count > 0u) ||
               (_option->short_names.count > 0u) ) );
}

/**
 * @brief Says that an option was given more than once with values that
 *        differ: `PROGRAM: warning: PLACE: --NAME was given twice; the last,
 *        'VALUE', is taken`, "3 times" past two, the long name if it has
 *        one. The place is made first in a room on the stack, so that the
 *        want of memory shortens the warning, and never loses it.
 *
 * @param[in] _program  the program.
 * @param[in] _option   the option: its command gives the place, and its
 *                      first long name, else its first short, the name.
 * @param[in] _times    how many times it was given; two or more.
 * @param[in] _last     the value taken.
 */
static
void d_cli_internal_say_repeat(
    const struct dj_cli_program* _program,
    const struct dj_arg_option*  _option,
    size_t                       _times,
    struct dj_pack_text          _last
)
{
    char       place[D_INTERNAL_CLI_SPARE];
    char       times[32];
    size_t     used      = 0u;
    const bool long_name = (_option->long_names.count > 0u);

    place[0] = '\0';
    d_cli_internal_room_place(_program,
                              _option->parent,
                              place,
                              sizeof(place),
                              &used);
    (void)snprintf(times,
                   sizeof(times),
                   "%lu times",
                   (unsigned long)_times);
    d_cli_report(_program,
                 (int)D_CLI_SEVERITY_WARNING,
                 "%s%s%s was given %s; the last, '%.*s', is taken",
                 place,
                 (long_name) ? "--" : "-",
                 (long_name) ? _option->long_names.items[0]
                             : _option->short_names.items[0],
                 (_times == 2u) ? "twice" : times,
                 (int)_last.length,
                 (_last.data != NULL) ? _last.data : "");

    return;
}

/**
 * @brief Says an option given more than once with values that differ, where
 *        it keeps one value; `--colour` with `--color`, as one option named
 *        `--color`, its value the one d_cli_internal_colour_given finds.
 *
 * @param[in] _program  the program.
 * @param[in] _option   the option.
 */
static
void d_cli_internal_repeat_of(
    const struct dj_cli_program* _program,
    const struct dj_arg_option*  _option
)
{
    const bool         colour = (_option == _program->colour_option);
    struct dj_pack_text last   = d_pack_text_from_cstr("");
    size_t             times  = 0u;
    bool               differ = false;

    // `--colour` is said with `--color`
    if (_option == _program->colour_alias)
    {
        return;
    }

    if (colour)
    {
        times  = d_cli_internal_times_given(_program->colour_option) +
                 d_cli_internal_times_given(_program->colour_alias);
        differ = ( (d_cli_internal_colour_given(_program,
                                                &last) != NULL) &&
                   ( (d_cli_internal_differs(_program->colour_option,
                                             last)) ||
                     (d_cli_internal_differs(_program->colour_alias,
                                             last)) ) );
    }
    else if (d_cli_internal_keeps_last(_option))
    {
        size_t                          count  = 0u;
        const struct dj_pack_text* const values = d_arg_option_results(
                                                     _option,
                                                     &count);

        times  = d_cli_internal_times_given(_option);
        last   = (count > 0u) ? values[count - 1u] : last;
        differ = d_cli_internal_differs(_option,
                                        last);
    }

    if ( (times >= 2u) &&
         (differ) )
    {
        d_cli_internal_say_repeat(_program,
                                  _option,
                                  times,
                                  last);
    }

    return;
}

/**
 * @brief Says each of a command's options given more than once with values
 *        that differ, in the order they were defined: its own, then those
 *        of its option groups, whose options are its own.
 *
 * @param[in] _program  the program.
 * @param[in] _command  the command.
 */
static
void d_cli_internal_repeats_in(
    const struct dj_cli_program* _program,
    const struct dj_arg_command* _command
)
{
    size_t index = 0u;

    for (index = 0u; index < _command->options.count; ++index)
    {
        d_cli_internal_repeat_of(_program,
                                 _command->options.items[index]);
    }

    for (index = 0u; index < _command->subcommands.count; ++index)
    {
        const struct dj_arg_command* const group =
            _command->subcommands.items[index];

        if (d_arg_command_is_option_group(group))
        {
            d_cli_internal_repeats_in(_program,
                                      group);
        }
    }

    return;
}

/**
 * @brief The command the last parse went on to from a command: the last of
 *        its parsed subcommands that is not an option group.
 *
 * @param[in] _command  the command.
 * @return that command, or NULL where the parse went no deeper.
 */
static
const struct dj_arg_command* d_cli_internal_next_reached(
    const struct dj_arg_command* _command
)
{
    for (size_t index = d_arg_command_parsed_subcommand_count(_command);
         index > 0u;
         --index)
    {
        const struct dj_arg_command* const each =
            d_arg_command_parsed_subcommand_at(_command,
                                               index - 1u);

        if (!d_arg_command_is_option_group(each))
        {
            return each;
        }
    }

    return NULL;
}

/**
 * @brief At `-v` or more, after a parse that went well and reached a command
 *        with work, says each option that keeps one value and was given it
 *        more than once, differently (R8): in each command the run reached,
 *        from the root down.
 *
 * @param[in] _program  the program, its standard options read.
 */
static
void d_cli_internal_repeated(
    const struct dj_cli_program* _program
)
{
    const struct dj_cli_handler* const work = d_cli_internal_handler(
                                                 _program,
                                                 d_cli_program_command(
                                                     _program));

    // said only at -v or more, and where the run goes on to work; a run
    // that reached a command with none ends in its usage error
    if ( (_program->verbosity < 1) ||
         (!work)                   ||
         (!work->run) )
    {
        return;
    }

    for (const struct dj_arg_command* at = &_program->parser->command;
         at != NULL;
         at = d_cli_internal_next_reached(at))
    {
        d_cli_internal_repeats_in(_program,
                                  at);
    }

    return;
}

#if (D_ENV_CLI_HELP_SLASH == 1)

/**
 * @brief Whether `/?` is among the arguments before any `--`.
 *
 * @param[in] _argc  the argument count.
 * @param[in] _argv  the arguments.
 * @return `true` if it is.
 */
static
bool d_cli_internal_asks_slash(
    int                _argc,
    const char* const* _argv
)
{
    for (int index = 1; (index < _argc) && (_argv[index] != NULL); ++index)
    {
        // the options' end, which comes first
        if (strcmp(_argv[index],
                   "--") == 0)
        {
            return false;
        }

        if (strcmp(_argv[index],
                   "/?") == 0)
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief A copy of the arguments, which the program keeps, with each `/?`
 *        before `--` read as `--help`.
 *
 * @param[in,out] _program  the program.
 * @param[in]     _argc     the argument count.
 * @param[in]     _argv     the arguments.
 * @return the copy; `_argv` itself when there was no room for one.
 */
static
const char* const* d_cli_internal_slash_as_help(
    struct dj_cli_program* _program,
    int                    _argc,
    const char* const*     _argv
)
{
    const char** const copy = (const char**)D_INTERNAL_CLI_REALLOC(
                                  (void*)_program->arguments,
                                  ((size_t)_argc + 1u) *
                                      sizeof(const char*));
    bool               ended = false;

    // no room, which the caller says
    if (!copy)
    {
        return NULL;
    }

    for (int index = 0; index < _argc; ++index)
    {
        ended       = (ended) ||
                      (strcmp(_argv[index],
                              "--") == 0);
        copy[index] = ( (!ended) &&
                        (index > 0) &&
                        (strcmp(_argv[index],
                                "/?") == 0) )
                      ? "--help"
                      : _argv[index];
    }

    copy[_argc]              = NULL;
    _program->arguments      = copy;
    _program->argument_count = (size_t)_argc;

    return copy;
}

#endif  // D_ENV_CLI_HELP_SLASH

/**
 * @brief The arguments to parse: main's own taken again from the true
 *        command line on Windows, and, where `/?` asks for help, a copy of
 *        them with it read as `--help` before any `--`.
 *
 * @note Only main's own arguments are taken again: a module is given the
 *       arguments after its command, already UTF-8, and the process's
 *       command line is its program's, not its own.
 *
 * @param[in,out] _program  the program, which keeps a copy it makes.
 * @param[in]     _argc     the argument count.
 * @param[in]     _argv     the arguments.
 * @return the arguments to parse: `_argv` itself when nothing changed; NULL
 *         when `/?` was to be read and there was no room for the copy.
 */
static
const char* const* d_cli_internal_arguments(
    struct dj_cli_program* _program,
    int                    _argc,
    const char* const*     _argv
)
{
    const char* const* argv = _argv;

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)
    // main's, as the runtime made them in the ANSI code page: again in UTF-8
    if (d_cli_internal_main_arguments(_argc,
                                      _argv))
    {
        argv = (const char* const*)d_arg_parser_ensure_utf8(_program->parser,
                                                            (char**)_argv);
    }
#endif

#if (D_ENV_CLI_HELP_SLASH == 1)
    return (d_cli_internal_asks_slash(_argc,
                                      argv))
           ? d_cli_internal_slash_as_help(_program,
                                          _argc,
                                          argv)
           : argv;
#else
    (void)_program;
    (void)_argc;

    return argv;
#endif
}

/**
 * @brief The command a parse took after `--`, where the program's command
 *        was due: the parser, finding no operand of the root's to take the
 *        word, parses the subcommand it names without listing it among those
 *        the root used -- a command line the grammar has no reading for,
 *        since a command comes before `--` (R3).
 *
 * @param[in] _program  the program, its parse done.
 * @return the subcommand so taken, or NULL.
 */
static
const struct dj_arg_command* d_cli_internal_command_after_end(
    const struct dj_cli_program* _program
)
{
    const struct dj_arg_command* const root = &_program->parser->command;

    // the root listed the command it used: the usual way
    if (d_arg_command_parsed_subcommand_count(root) > 0u)
    {
        return NULL;
    }

    for (size_t index = 0u; index < root->subcommands.count; ++index)
    {
        const struct dj_arg_command* const each = root->subcommands.items[index];

        // used, unlisted, and not by its own wish
        if ( (!d_arg_command_is_option_group(each))           &&
             (!d_arg_command_has_flag(each,
                                      D_ARG_COMMAND_SILENT))   &&
             (d_arg_command_count(each) > 0u) )
        {
            return each;
        }
    }

    return NULL;
}

/**
 * @brief Makes every named option of a command, and of every command and
 *        option group under it, take one value each time it is given (R8):
 *        a list is the option given again, never the arguments after it.
 *
 * @note CLI11 lets an option bound to a vector go on taking arguments that
 *       are no options; a fixed count, `expected(2)`, still takes its two.
 *       An operand is not changed: it takes what is left, as it should.
 *
 * @param[in,out] _command  the command.
 */
static
void d_cli_internal_one_value_each(
    struct dj_arg_command* _command
)
{
    size_t i = 0u;

    for (i = 0u; i < _command->options.count; ++i)
    {
        struct dj_arg_option* const option = _command->options.items[i];

        // an operand takes what is left
        if (d_arg_option_is_nonpositional(option))
        {
            d_arg_option_set_allow_extra_args(option,
                                              false);
        }
    }

    for (i = 0u; i < _command->subcommands.count; ++i)
    {
        d_cli_internal_one_value_each(_command->subcommands.items[i]);
    }

    return;
}

/**
 * @brief Readies a program for a parse: the standard commands added at the
 *        first, after the program's own, and every option made to take one
 *        value each time.
 *
 * @param[in,out] _program  the program.
 * @return D_ARG_OK, or D_ARG_ERROR_OUT_OF_MEMORY, said, when the standard
 *         commands could not be added.
 */
static
int d_cli_internal_ready(
    struct dj_cli_program* _program
)
{
    // the standard commands, after the program's own, at the first parse
    if ( ((_program->flags & D_CLI_PROGRAM_COMMANDS) != 0u) &&
         (_program->help_command == NULL)                   &&
         (d_cli_internal_standard_commands(_program) != D_ARG_OK) )
    {
        d_cli_report(_program,
                     (int)D_CLI_SEVERITY_ERROR,
                     "out of memory");

        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    d_cli_internal_one_value_each(&_program->parser->command);

    return D_ARG_OK;
}

/**
 * @brief Whether a parse's status asked for something written and nothing
 *        else: help, a version or a completion script.
 *
 * @param[in] _status  the status.
 * @return `true` if it did.
 */
static
bool d_cli_internal_asked(
    int _status
)
{
    return ( (_status == (int)D_ARG_CALL_FOR_HELP)       ||
             (_status == (int)D_ARG_CALL_FOR_ALL_HELP)   ||
             (_status == (int)D_ARG_CALL_FOR_VERSION)    ||
             (_status == (int)D_ARG_CALL_FOR_COMPLETION) );
}

/**
 * @brief Whether a parse took a command after `--`, which the grammar has no
 *        reading for (R3) and so ends the run with a usage error, said here,
 *        whatever else the parse found -- unless it found help, a version
 *        or a script asked for.
 *
 * @param[in] _program  the program, its parse done.
 * @param[in] _status   the parse's status.
 * @return `true` if it did.
 */
static
bool d_cli_internal_misplaced(
    const struct dj_cli_program* _program,
    int                          _status
)
{
    const struct dj_arg_command* const command =
        (d_cli_internal_asked(_status))
        ? NULL
        : d_cli_internal_command_after_end(_program);

    if (command != NULL)
    {
        (void)d_cli_usage_error(_program,
                                NULL,
                                "'%s' follows '--': a command comes before it",
                                d_arg_command_name(command));
    }

    return (command != NULL);
}

/**
 * @brief Appends a compiler as `--version --verbose` names one: its name and
 *        its version, `GCC 13.3.0`, the version left out where unknown and
 *        the name `unknown` where it is.
 *
 * @param[in]     _build  the build.
 * @param[in,out] _out    the buffer it is appended to, its failure kept.
 */
static
void d_cli_internal_compiler_said(
    const struct dj_cli_build* _build,
    struct dj_arg_buffer*      _out
)
{
    (void)d_arg_buffer_append_cstr(_out,
                                   (_build->compiler != NULL)
                                   ? _build->compiler
                                   : "unknown");

    // the version, where it is known
    if (_build->major >= 0)
    {
        (void)d_arg_buffer_printf(_out,
                                  " %d.%d.%d",
                                  _build->major,
                                  _build->minor,
                                  _build->patch);
    }

    return;
}

/**
 * @brief Appends how a program was built, a line for each part known:
 *        `compiler`, `language`, `system` -- the system and the machine --
 *        and `build`; nothing for a program never told.
 *
 * @param[in]     _build  the program's build.
 * @param[in,out] _out    the buffer they are appended to, its failure kept.
 */
static
void d_cli_internal_build_said(
    const struct dj_cli_build* _build,
    struct dj_arg_buffer*      _out
)
{
    if (_build->compiler != NULL)
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       "compiler: ");
        d_cli_internal_compiler_said(_build,
                                     _out);
        (void)d_arg_buffer_append_char(_out,
                                       '\n');
    }

    if (_build->language != NULL)
    {
        (void)d_arg_buffer_printf(_out,
                                  "language: %s\n",
                                  _build->language);
    }

    // the system and the machine, or whichever is known
    if ( (_build->system != NULL) ||
         (_build->machine != NULL) )
    {
        (void)d_arg_buffer_printf(_out,
                                  "system: %s%s%s\n",
                                  (_build->system != NULL)
                                  ? _build->system
                                  : "",
                                  ( (_build->system != NULL) &&
                                    (_build->machine != NULL) )
                                  ? ", "
                                  : "",
                                  (_build->machine != NULL)
                                  ? _build->machine
                                  : "");
    }

    if (_build->type != NULL)
    {
        (void)d_arg_buffer_printf(_out,
                                  "build: %s\n",
                                  _build->type);
    }

    return;
}

/**
 * @brief Appends how the kit itself was built, as one line: `kit: C99,
 *        GCC 13.3.0, Release`, each part where it is known.
 *
 * @param[in,out] _out  the buffer it is appended to, its failure kept.
 */
static
void d_cli_internal_kit_said(
    struct dj_arg_buffer* _out
)
{
    const struct dj_cli_build* const kit = d_cli_kit_build();

    (void)d_arg_buffer_append_cstr(_out,
                                   "kit: ");

    if (kit->language != NULL)
    {
        (void)d_arg_buffer_printf(_out,
                                  "%s, ",
                                  kit->language);
    }

    d_cli_internal_compiler_said(kit,
                                 _out);

    if (kit->type != NULL)
    {
        (void)d_arg_buffer_printf(_out,
                                  ", %s",
                                  kit->type);
    }

    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

/**
 * @brief Appends the program's file and, in a module's program, the
 *        module's: `program` and `module`, each where it is known -- a
 *        module's as its host gave them, a program's as the system names
 *        its file.
 *
 * @param[in]     _program  the program, its parse done.
 * @param[out]    _owned    receives the program's file as it was named
 *                          here, to be freed with d_cli_free; NULL in a
 *                          module's program, or where it was not named.
 * @param[in,out] _out      the buffer they are appended to.
 * @return D_ARG_OK; D_ARG_ERROR_OUT_OF_MEMORY when there was no room to
 *         name the program's file.
 */
static
int d_cli_internal_files_said(
    const struct dj_cli_program* _program,
    char**                       _owned,
    struct dj_arg_buffer*        _out
)
{
    const struct dj_cli_plugin_host* const host = _program->host;

    *_owned = NULL;

    // a module's: its host's file and its own, as the host gave them
    if (host != NULL)
    {
        if (host->executable != NULL)
        {
            (void)d_arg_buffer_printf(_out,
                                      "program: %s\n",
                                      host->executable);
        }

        if (host->module != NULL)
        {
            (void)d_arg_buffer_printf(_out,
                                      "module: %s\n",
                                      host->module);
        }

        return D_ARG_OK;
    }

    // a program's own file, where the system can name it
    const int failed = d_cli_executable_path(_program->parser->program,
                                             _owned);

    if (failed == ENOMEM)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    if (failed == 0)
    {
        (void)d_arg_buffer_printf(_out,
                                  "program: %s\n",
                                  *_owned);
    }

    return D_ARG_OK;
}

/**
 * @brief Composes what `--version --verbose` says after the version's line
 *        (GRAMMAR.md, section 6): the files first -- the program's, a
 *        module's own, and where each command that is a module would load
 *        it from -- then how the program was built, its own lines, and how
 *        the kit was built.
 *
 * @param[in]     _program  the program, its parse done.
 * @param[in,out] _out      the buffer the lines are appended to.
 * @return D_ARG_OK when composed whole; D_ARG_ERROR_OUT_OF_MEMORY when not.
 */
static
int d_cli_internal_build_text(
    const struct dj_cli_program* _program,
    struct dj_arg_buffer*        _out
)
{
    char* owned  = NULL;
    int   status = d_cli_internal_files_said(_program,
                                             &owned,
                                             _out);

    status = (status == D_ARG_OK)
             ? d_cli_internal_modules_said(_program,
                                           (_program->host != NULL)
                                           ? _program->host->executable
                                           : owned,
                                           _out)
             : status;

    if (status == D_ARG_OK)
    {
        d_cli_internal_build_said(&_program->build,
                                  _out);
        (void)d_arg_buffer_append_cstr(_out,
                                       (_program->build_lines != NULL)
                                       ? _program->build_lines
                                       : "");
        d_cli_internal_kit_said(_out);
    }

    d_cli_free(owned);

    return (status != D_ARG_OK)
           ? status
           : (_out->failed)
             ? D_ARG_ERROR_OUT_OF_MEMORY
             : D_ARG_OK;
}

/**
 * @brief Writes the version: its line and, at `-v` or more, the build after
 *        it, composed whole before any of it is written.
 *
 * @param[in] _program  the program, its parse done.
 * @param[in] _line     the version's line, `PROGRAM VERSION`.
 * @param[in] _out      where it goes.
 * @return D_ARG_OK when written whole; D_ARG_ERROR_FILE when the output
 *         took less than all of it; D_ARG_ERROR_OUT_OF_MEMORY when the build
 *         could not be composed, nothing written.
 */
static
int d_cli_internal_version(
    const struct dj_cli_program* _program,
    const char*                  _line,
    struct dj_pack_sink          _out
)
{
    struct dj_arg_buffer build  = D_ARG_BUFFER_INIT;
    const size_t        length = strlen(_line);
    int                 status = D_ARG_OK;

    // the build, only when asked for
    if (_program->verbosity >= 1)
    {
        status = d_cli_internal_build_text(_program,
                                           &build);
    }

    if (status == D_ARG_OK)
    {
        status = ( (_out.write(_out.context,
                               _line,
                               length) == length)        &&
                   (_out.write(_out.context,
                               "\n",
                               1u) == 1u)                &&
                   ( (build.length == 0u) ||
                     (_out.write(_out.context,
                                 build.data,
                                 build.length) == build.length) ) )
                 ? D_ARG_OK
                 : D_ARG_ERROR_FILE;
    }

    d_arg_buffer_release(&build);

    return status;
}

/**
 * @brief Writes what a parse was asked for to the program's output, as the
 *        parser's exit writes it -- the help of the last command the parse
 *        used, plain or expanded; a version and a newline; a completion
 *        script -- and says whether it was written whole.
 *
 * @param[in]  _program  the program, its parse ended by the request.
 * @param[in]  _status   the parse's status, one d_cli_internal_asked holds.
 * @param[out] _watch    receives the output's reason, when it refused.
 * @return D_ARG_OK when written whole; D_ARG_ERROR_FILE when the output
 *         took less than all of it; D_ARG_ERROR_OUT_OF_MEMORY, or the
 *         status of what else stopped the help being built, nothing
 *         written.
 */
static
int d_cli_internal_answer(
    const struct dj_cli_program*  _program,
    int                           _status,
    struct dj_cli_internal_watch* _watch
)
{
    const struct dj_pack_sink out = d_cli_internal_watched(_program,
                                                          _watch);

    // help, built whole before any of it is written
    if ( (_status == (int)D_ARG_CALL_FOR_HELP) ||
         (_status == (int)D_ARG_CALL_FOR_ALL_HELP) )
    {
        return d_arg_help(&_program->parser->command,
                          d_pack_text_from_cstr(""),
                          (_status == (int)D_ARG_CALL_FOR_HELP)
                          ? (int)D_ARG_HELP_NORMAL
                          : (int)D_ARG_HELP_ALL,
                          out);
    }

    const char* const said   = d_arg_error_message(&_program->parser->error);
    const char* const text   = (said != NULL) ? said : "";
    const size_t      length = strlen(text);

    // a version ends its line, and may have the build after it
    if (_status == (int)D_ARG_CALL_FOR_VERSION)
    {
        return d_cli_internal_version(_program,
                                      text,
                                      out);
    }

    // a script, as the parse made it
    return (out.write(out.context,
                      text,
                      length) == length)
           ? D_ARG_OK
           : D_ARG_ERROR_FILE;
}

/**
 * @brief Ends a parse that called for an end: what it was asked for
 *        written, or why it was not said and the run trouble; a failure
 *        written as the parser writes one, through the kit's failure
 *        message, and the run trouble; a callback's success, done.
 *
 * @param[in] _program  the program.
 * @param[in] _status   the parse's status, not D_ARG_OK.
 * @return the status the run ends with.
 */
static
int d_cli_internal_ending(
    const struct dj_cli_program* _program,
    int                          _status
)
{
    struct dj_cli_internal_watch watch;

    // a failure, or a callback's success: as the parser ends it
    if (!d_cli_internal_asked(_status))
    {
        (void)d_arg_parser_exit(_program->parser,
                                _status);

        return (_status == (int)D_ARG_SUCCESS) ? D_CLI_EXIT_OK
                                               : D_CLI_EXIT_TROUBLE;
    }

    const int written = d_cli_internal_answer(_program,
                                              _status,
                                              &watch);

    return (written == D_ARG_OK)
           ? D_CLI_EXIT_OK
           : d_cli_internal_unwritten(_program,
                                      written,
                                      watch.errnum);
}

/**
 * @brief The name a program that could not be made goes by: its parser's,
 *        or, where the parser has none, the last part of `argv[0]`, less
 *        any `.exe`, as the grammar names a program.
 *
 * @param[in]  _program  the program.
 * @param[in]  _argc     the argument count.
 * @param[in]  _argv     the arguments.
 * @param[out] _length   receives the name's length.
 * @return the name, not NUL-terminated at `*_length`; "" for none.
 */
static
const char* d_cli_internal_unmade_name(
    const struct dj_cli_program* _program,
    int                          _argc,
    const char* const*           _argv,
    size_t*                      _length
)
{
    const char* name = d_arg_command_name(&_program->parser->command);

    // the file's, where the parser has no name
    if ( ( (!name) ||
           (name[0] == '\0') ) &&
         (_argc > 0)          &&
         (_argv != NULL)      &&
         (_argv[0] != NULL) )
    {
        const char* const slash = strrchr(_argv[0],
                                          '/');
        const char* const back  = strrchr(_argv[0],
                                          '\\');
        const char* const last  = ( (back != NULL) &&
                                    ( (slash == NULL) ||
                                      (back > slash) ) ) ? back : slash;

        name = (last != NULL) ? (last + 1) : _argv[0];
    }

    name     = (name != NULL) ? name : "";
    *_length = strlen(name);

    // less any .exe
    if ( (*_length > 4u) &&
         (strcmp(name + *_length - 4u,
                 ".exe") == 0) )
    {
        *_length -= 4u;
    }

    return name;
}

/**
 * @brief Says why a program could not be made -- the want of memory, or the
 *        definition its making stopped at -- to the error output it was
 *        given, or to standard error; composed without asking for memory,
 *        as there may be none.
 *
 * @param[in] _program  a program d_cli_program_init failed to make, its
 *                      parser kept.
 * @param[in] _argc     the argument count.
 * @param[in] _argv     the arguments.
 */
static
void d_cli_internal_unmade(
    const struct dj_cli_program* _program,
    int                          _argc,
    const char* const*           _argv
)
{
    char                            room[D_INTERNAL_CLI_SPARE];
    size_t                          used    = 0u;
    size_t                          length  = 0u;
    const char* const               name    = d_cli_internal_unmade_name(
                                                  _program,
                                                  _argc,
                                                  _argv,
                                                  &length);
    const struct dj_arg_error* const defined =
        d_arg_parser_definition_error(_program->parser);
    const char* const               message = (defined != NULL)
                                              ? d_arg_error_message(defined)
                                              : NULL;
    const char* const               why     =
        (_program->status == (int)D_ARG_ERROR_OUT_OF_MEMORY)
        ? "out of memory"
        : ( (message != NULL) &&
            (message[0] != '\0') )
        ? message
        : "the program could not be made";
    const struct dj_pack_sink        err     = (_program->err.write != NULL)
                                              ? _program->err
                                              : d_cli_sink_error();

    // the program, and the word an error is said with, unless the program
    // says its errors without it
    room[0] = '\0';
    d_cli_internal_room_add(room,
                            sizeof(room),
                            &used,
                            "%.*s%s%s",
                            (int)length,
                            name,
                            (length > 0u) ? ": " : "",
                            ((_program->flags &
                              D_CLI_PROGRAM_PLAIN_ERRORS) != 0u)
                            ? ""
                            : "error: ");

    // the reason, its first letter lowered as the parser's are
    const size_t start = used;

    d_cli_internal_room_add(room,
                            sizeof(room),
                            &used,
                            "%s\n",
                            why);

    if (d_cli_internal_lower_first(room + start,
                                   used - start))
    {
        room[start] = (char)(room[start] - 'A' + 'a');
    }

    room[used - 1u] = '\n';
    (void)err.write(err.context,
                    room,
                    used);

    return;
}

/**
 * @brief Whether a program cannot be run at all: none, nowhere to say how
 *        its run ends, or one whose making failed -- which is said.
 *
 * @param[in]  _program  the program.
 * @param[in]  _argc     the argument count.
 * @param[in]  _argv     the arguments.
 * @param[out] _exit     receives D_CLI_EXIT_TROUBLE when the run ends here;
 *                       may be NULL.
 * @return `true` when the run ends here.
 */
static
bool d_cli_internal_unrunnable(
    const struct dj_cli_program* _program,
    int                          _argc,
    const char* const*           _argv,
    int*                         _exit
)
{
    // a program made whole, and somewhere to say how its run ends
    if ( (_program != NULL)                 &&
         (_exit != NULL)                    &&
         (_program->name != NULL)           &&
         (_program->status == (int)D_ARG_OK) )
    {
        return false;
    }

    if (_exit != NULL)
    {
        *_exit = D_CLI_EXIT_TROUBLE;
    }

    // a program whose making failed, said
    if ( (_program != NULL)                 &&
         (_program->parser != NULL)         &&
         (_program->status != (int)D_ARG_OK) )
    {
        d_cli_internal_unmade(_program,
                              _argc,
                              _argv);
    }

    return true;
}

/**
 * @brief Readies a program and its arguments for a parse: the standard
 *        commands at the first, main's arguments taken again on Windows,
 *        and `/?` read as `--help` where it is.
 *
 * @param[in,out] _program  the program.
 * @param[in]     _argc     the argument count.
 * @param[in]     _argv     the arguments.
 * @return the arguments to parse; NULL, said, when there was no memory for
 *         what the parse needs.
 */
static
const char* const* d_cli_internal_prepare(
    struct dj_cli_program* _program,
    int                    _argc,
    const char* const*     _argv
)
{
    // the standard commands, said when they could not be added
    if (d_cli_internal_ready(_program) != D_ARG_OK)
    {
        return NULL;
    }

    const char* const* const argv = d_cli_internal_arguments(_program,
                                                             _argc,
                                                             _argv);

    // a change due, and no room for it
    if (!argv)
    {
        d_cli_report(_program,
                     (int)D_CLI_SEVERITY_ERROR,
                     "out of memory");
    }

    return argv;
}

/**
 * @brief The run's settings: started at a program's first parse, the run
 *        then being its program's -- a module's program is told its run's
 *        settings by the program that loaded it -- and taken off the
 *        arguments, which the program keeps without them.
 *
 * @param[in,out] _program  the program.
 * @param[in,out] _argc     the argument count; receives the count kept.
 * @param[in,out] _argv     the arguments; receives those kept.
 * @return D_CLI_EXIT_OK; or D_CLI_EXIT_TROUBLE, said, when the settings
 *         could not be read for want of memory, or the command line named
 *         one badly.
 */
static
int d_cli_internal_settings(
    struct dj_cli_program* _program,
    int*                   _argc,
    const char* const**    _argv
)
{
    const char** kept;
    int          count  = 0;
    int          status = (_program->host == NULL)
                          ? d_cli_settings_start(_program)
                          : D_CLI_EXIT_OK;

    if (status != D_CLI_EXIT_OK)
    {
        return status;
    }

    kept = (const char**)D_INTERNAL_CLI_MALLOC(((size_t)*_argc + 1u) *
                                               sizeof(const char*));

    if (!kept)
    {
        d_cli_report(_program,
                     (int)D_CLI_SEVERITY_ERROR,
                     "out of memory");

        return D_CLI_EXIT_TROUBLE;
    }

    status = d_cli_settings_take(_program,
                                 *_argc,
                                 *_argv,
                                 kept,
                                 &count);

    // kept in place of the copy the arguments may have been read from
    D_INTERNAL_CLI_FREE((void*)_program->arguments);
    _program->arguments      = kept;
    _program->argument_count = (size_t)count;
    *_argc                   = count;
    *_argv                   = kept;

    return status;
}

/*
d_cli_program_parse
  The program readied, the arguments, the parse, the standard options'
values; then the ending the status calls for -- help, a version and a
script to the output, whole or the want of it said; a failure through the
kit's failure message to the error output -- and the grammar's exit status
for it. A parse that went well ends nothing; at -v, where the run goes on
to work, it says first each option that kept the last of values that differ.
*/
bool d_cli_program_parse(
    struct dj_cli_program* _program,
    int                    _argc,
    const char* const*     _argv,
    int*                   _exit
)
{
    // nothing to run, or a program whose making failed, said
    if (d_cli_internal_unrunnable(_program,
                                  _argc,
                                  _argv,
                                  _exit))
    {
        return true;
    }

    const char* const* argv = d_cli_internal_prepare(_program,
                                                     _argc,
                                                     _argv);
    int                argc = _argc;

    // no memory to parse with, or a setting named badly, said
    if ( (!argv) ||
         (d_cli_internal_settings(_program,
                                  &argc,
                                  &argv) != D_CLI_EXIT_OK) )
    {
        *_exit = D_CLI_EXIT_TROUBLE;

        return true;
    }

    const int status = d_arg_parser_parse(_program->parser,
                                          argc,
                                          (char* const*)argv);

    d_cli_internal_read_standard(_program);

    // a command after `--`; or a parse that went well, the program going
    // on unless --color was given a word it does not know
    if ( (d_cli_internal_misplaced(_program,
                                   status)) ||
         ( (status == (int)D_ARG_OK) &&
           (!d_cli_internal_colour_valid(_program)) ) )
    {
        *_exit = D_CLI_EXIT_TROUBLE;

        return true;
    }

    // the run goes on; at -v, after an option that took a second value
    // differently from its first was said
    if (status == (int)D_ARG_OK)
    {
        d_cli_internal_repeated(_program);

        return false;
    }

    *_exit = d_cli_internal_ending(_program,
                                   status);

    return true;
}

/*
d_cli_program_command
  Down the subcommands each command parsed, the last of each, to one that
parsed none.
*/
struct dj_arg_command* d_cli_program_command(
    const struct dj_cli_program* _program
)
{
    // no program
    if (!_program)
    {
        return NULL;
    }

    struct dj_arg_command* at = &_program->parser->command;

    for (;;)
    {
        const size_t count = d_arg_command_parsed_subcommand_count(at);

        // a command that went no deeper
        if (count == 0u)
        {
            return at;
        }

        struct dj_arg_command* const next =
            d_arg_command_parsed_subcommand_at(at,
                                               count - 1u);

        if (!next)
        {
            return at;
        }

        at = next;
    }
}

/**
 * @brief Appends `Commands: ` and the names of a command's subcommands a
 *        user may name, and a newline; nothing for a command with none.
 *
 * @param[in]     _command  the command.
 * @param[in,out] _out      the buffer.
 * @return D_ARG_OK, or the buffer's status.
 */
static
int d_cli_internal_list_commands(
    const struct dj_arg_command* _command,
    struct dj_arg_buffer*        _out
)
{
    int  status = D_ARG_OK;
    bool listed = false;

    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _command->subcommands.count);
         ++index)
    {
        const struct dj_arg_command* const each =
            _command->subcommands.items[index];

        // an option group, or a command not shown
        if ( (d_arg_command_is_option_group(each)) ||
             ( (each->group != NULL) &&
               (each->group[0] == '\0') ) )
        {
            continue;
        }

        status = d_arg_buffer_append_cstr(_out,
                                          (listed) ? ", " : "Commands: ");
        status = (status == D_ARG_OK)
                 ? d_arg_buffer_append_cstr(_out,
                                            d_arg_command_name(each))
                 : status;
        listed = true;
    }

    return ( (status == D_ARG_OK) &&
             (listed) )
           ? d_arg_buffer_append_char(_out,
                                      '\n')
           : status;
}

/**
 * @brief Appends a command's usage line, as its formatter writes it, to a
 *        message: the blank lines and spaces around it left out, and
 *        `Usage: ` before it where the formatter's line does not begin so,
 *        as the argument parser's own layout writes it alone between blank
 *        lines (D_CLI_PROGRAM_PARSER_HELP); then a newline.
 *
 * @param[in]     _command  the command.
 * @param[in]     _name     its full name, as the line begins.
 * @param[in,out] _out      the buffer.
 * @return D_ARG_OK, or the formatter's or the buffer's status.
 */
static
int d_cli_internal_usage_line(
    const struct dj_arg_command* _command,
    struct dj_pack_text          _name,
    struct dj_arg_buffer*        _out
)
{
    struct dj_arg_buffer usage  = D_ARG_BUFFER_INIT;
    int                 status = d_arg_format_usage(
                                     d_arg_command_formatter(_command),
                                     _command,
                                     _name,
                                     &usage);
    const char*         text   = (usage.data != NULL) ? usage.data : "";
    size_t              length = (usage.data != NULL) ? usage.length : 0u;

    // the blank lines and spaces around it
    while ( (length > 0u) &&
            ( (text[0] == '\n') ||
              (text[0] == ' ') ) )
    {
        ++text;
        --length;
    }

    while ( (length > 0u) &&
            ( (text[length - 1u] == '\n') ||
              (text[length - 1u] == ' ') ) )
    {
        --length;
    }

    // the label, where the formatter gave none; the line; its end
    status = ( (status == D_ARG_OK) &&
               (length > 0u)        &&
               (strncmp(text,
                        "Usage:",
                        6u) != 0) )
             ? d_arg_buffer_append_cstr(_out,
                                        "Usage: ")
             : status;
    status = ( (status == D_ARG_OK) &&
               (length > 0u) )
             ? d_arg_buffer_append(_out,
                                   text,
                                   length)
             : status;
    status = ( (status == D_ARG_OK) &&
               (length > 0u) )
             ? d_arg_buffer_append_char(_out,
                                        '\n')
             : status;

    d_arg_buffer_release(&usage);

    return status;
}

/**
 * @brief Composes the usage error of a command reached with no work: the
 *        error, its usage line and the commands it has, then the pointer to
 *        its help.
 *
 * @param[in]     _program  the program.
 * @param[in]     _command  the command.
 * @param[in,out] _out      the buffer.
 * @return D_ARG_OK, or the status that stopped it.
 */
static
int d_cli_internal_no_work_text(
    const struct dj_cli_program* _program,
    const struct dj_arg_command* _command,
    struct dj_arg_buffer*        _out
)
{
    struct dj_arg_buffer name   = D_ARG_BUFFER_INIT;
    int                 status = d_cli_command_path(_command,
                                                    &name);

    status = (status == D_ARG_OK)
             ? d_cli_internal_locus(_program,
                                    (int)D_CLI_SEVERITY_ERROR,
                                    NULL,
                                    0L,
                                    0L,
                                    _out)
             : status;
    status = (status == D_ARG_OK)
             ? d_cli_internal_place(_program,
                                    _command,
                                    _out)
             : status;
    status = (status == D_ARG_OK)
             ? d_arg_buffer_append_cstr(_out,
                                        "a command is required\n")
             : status;
    status = (status == D_ARG_OK)
             ? d_cli_internal_usage_line(_command,
                                         d_arg_buffer_text(&name),
                                         _out)
             : status;

    // the commands, by name, and the pointer to help
    status = (status == D_ARG_OK)
             ? d_cli_internal_list_commands(_command,
                                            _out)
             : status;
    status = (status == D_ARG_OK)
             ? d_cli_internal_try(_command,
                                  _out)
             : status;

    d_arg_buffer_release(&name);

    return status;
}

/**
 * @brief The usage error of a command reached with no work: its usage line
 *        and the commands it has, then the pointer to its help -- or, where
 *        there was no memory to compose it, the error and the pointer alone,
 *        composed in a room of their own.
 *
 * @param[in] _program  the program.
 * @param[in] _command  the command.
 * @return D_CLI_EXIT_TROUBLE.
 */
static
int d_cli_internal_no_work(
    const struct dj_cli_program* _program,
    const struct dj_arg_command* _command
)
{
    const struct dj_cli_internal_shape shape = { (int)D_CLI_SEVERITY_ERROR,
                                                NULL,
                                                0L,
                                                0L,
                                                _command,
                                                false,
                                                true };
    struct dj_arg_buffer               out   = D_ARG_BUFFER_INIT;

    // written whole, or in a room of its own
    if (d_cli_internal_no_work_text(_program,
                                    _command,
                                    &out) == D_ARG_OK)
    {
        d_cli_internal_emit(_program,
                            &out);
    }
    else
    {
        (void)d_cli_internal_spare_say(_program,
                                       &shape,
                                       _program->err,
                                       "%s",
                                       "a command is required");
    }

    d_arg_buffer_release(&out);

    return D_CLI_EXIT_TROUBLE;
}

/*
d_cli_program_dispatch
  The deepest command's work, or the usage error of reaching none.
*/
int d_cli_program_dispatch(
    struct dj_cli_program* _program
)
{
    // no program, or one whose making failed
    if ( (!_program) ||
         (!_program->name) )
    {
        return D_CLI_EXIT_TROUBLE;
    }

    struct dj_arg_command* const       command = d_cli_program_command(
                                                    _program);
    const struct dj_cli_handler* const work    = d_cli_internal_handler(
                                                    _program,
                                                    command);

    // the command's work, which says how it went
    if ( (work != NULL) &&
         (work->run != NULL) )
    {
        return work->run(_program,
                         command,
                         work->context);
    }

    return d_cli_internal_no_work(_program,
                                  command);
}

/*
d_cli_program_run
  Parse, then dispatch, unless the parse ended the run.
*/
int d_cli_program_run(
    struct dj_cli_program* _program,
    int                    _argc,
    const char* const*     _argv
)
{
    int status = D_CLI_EXIT_TROUBLE;

    // the parse ended it
    if (d_cli_program_parse(_program,
                            _argc,
                            _argv,
                            &status))
    {
        return status;
    }

    return d_cli_program_dispatch(_program);
}


//==============================================================================
// 8.  SETTINGS AND QUERIES
//==============================================================================

/*
d_cli_program_set_output
  The program's sinks and the parser's together.
*/
void d_cli_program_set_output(
    struct dj_cli_program* _program,
    struct dj_pack_sink    _out,
    struct dj_pack_sink    _err
)
{
    // no program
    if (!_program)
    {
        return;
    }

    _program->out = _out;
    _program->err = _err;
    d_arg_parser_set_output(_program->parser,
                            _out,
                            _err);

    return;
}

/*
d_cli_program_verbosity
  The field.
*/
int d_cli_program_verbosity(
    const struct dj_cli_program* _program
)
{
    return (_program != NULL) ? _program->verbosity : 0;
}

/*
d_cli_program_colour
  The decision the last parse made for the stream.
*/
bool d_cli_program_colour(
    const struct dj_cli_program* _program,
    int                          _stream
)
{
    // no program
    if (!_program)
    {
        return false;
    }

    return (_stream == (int)D_CLI_STREAM_OUTPUT)
           ? _program->colour_output
           : (_stream == (int)D_CLI_STREAM_ERROR)
             ? _program->colour_error
             : false;
}

/*
d_cli_program_set_build
  The record copied whole; its strings stay the caller's literals.
*/
void d_cli_program_set_build(
    struct dj_cli_program*     _program,
    const struct dj_cli_build* _build
)
{
    // nothing to set, or nothing to set it on
    if ( (!_program) ||
         (!_build) )
    {
        return;
    }

    _program->build = *_build;

    return;
}

/*
d_cli_program_add_build
  The lines kept so far and the new one composed in a buffer, then copied
through the kit's allocator to take their place, as everything the program
frees is the kit's; an empty key or value, either holding a line break, or
a key holding the colon that would end it early, is refused before
anything is touched.
*/
int d_cli_program_add_build(
    struct dj_cli_program* _program,
    const char*            _key,
    const char*            _value
)
{
    // parameter validation
    if ( (!_program)                       ||
         (!_key)                           ||
         (!_value)                         ||
         (_key[0] == '\0')                 ||
         (_value[0] == '\0')               ||
         (strpbrk(_key, ":\r\n") != NULL)  ||
         (strpbrk(_value, "\r\n") != NULL) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    struct dj_arg_buffer lines = D_ARG_BUFFER_INIT;

    (void)d_arg_buffer_append_cstr(&lines,
                                   (_program->build_lines != NULL)
                                   ? _program->build_lines
                                   : "");
    (void)d_arg_buffer_printf(&lines,
                              "%s: %s\n",
                              _key,
                              _value);

    char* const kept = (!lines.failed)
                       ? d_cli_internal_copy(lines.data)
                       : NULL;

    d_arg_buffer_release(&lines);

    // no room: the lines kept stay, and the program is short of one
    if (!kept)
    {
        _program->status = D_ARG_ERROR_OUT_OF_MEMORY;

        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    D_INTERNAL_CLI_FREE(_program->build_lines);
    _program->build_lines = kept;

    return D_ARG_OK;
}

// D_INTERNAL_CLI_KIT_BUILD
//   constant: how the kit was built, described where its C is compiled.
static const struct dj_cli_build D_INTERNAL_CLI_KIT_BUILD = D_CLI_BUILD_INIT;

/*
d_cli_kit_build
  The record this unit describes itself with.
*/
const struct dj_cli_build* d_cli_kit_build(void)
{
    return &D_INTERNAL_CLI_KIT_BUILD;
}
