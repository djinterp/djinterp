/*******************************************************************************
* djinterp [c][tools]                                                 cli_help.h
*
* A program's description as JSON, for the tools that are built from it:
* its man pages, its wiki pages, and the checks a page's prose is held to
* (docs/djinterp/tools/cli/HELP_JSON.html). `PROGRAM help --format=json`
* writes it, and `PROGRAM help COMMAND --format=json` the part of it from a
* command down.
*   The description names its shape first, "djinterp.help/1.0", and then
* holds what the program's parser and its registry hold: the program's name
* and version; its commands, each with its options, its arguments and its
* subcommands; every setting the program reads, by full name, with its
* kind, default, knob, choices and range, and the environment variables it
* is read from; the variables the kit reads besides; the prefixes those
* variables begin with; and what each exit status means. A program adds
* members of its own - its djins, the names it keeps, a page's sections --
* through a hook, after the kit's.
*   The JSON is written through a small writer of the kit's, which a hook
* writes its members with: pretty, two spaces an indent, each member and
* element on a line of its own, as `describe --format=json` writes; text
* escaped as JSON asks, and a byte that is not UTF-8 written as U+FFFD.
*
* HOW TO USE
* ----------
*   A program on the kit has `help --format=json` already. A program with
* members of its own sets a hook with d_cli_program_set_help_json before
* its run, and writes them in it with 1's calls: d_cli_json_member, then a
* value or a container. A program without the `help` command, or one that
* wants the text elsewhere, calls d_cli_help_json itself.
*
* OPTIONS
* -------
*   `help --format=FORMAT`: `text`, the default, or `json`.
*
* CONFIGURATION
* -------------
*   D_CLI_JSON_DEPTH is the deepest the writer nests. The settings are
* described where the option core is built (RE_STD_HAS_INT64); without it,
* the description's settings are an empty list.
*
*
* path:      /inc/djinterp/tools/cli/c/cli_help.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.22
*                                                            revised: 2026.11.22
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE WRITER
    ----------
    1.  The record
         1.  D_CLI_JSON_DEPTH
         2.  dj_cli_json
    2.  Writing
         1.  d_cli_json_init
         2.  d_cli_json_object / d_cli_json_array / d_cli_json_end
         3.  d_cli_json_member
         4.  d_cli_json_string / d_cli_json_text
         5.  d_cli_json_integer / d_cli_json_real / d_cli_json_bool /
             d_cli_json_null
         6.  d_cli_json_finish
2.  THE DESCRIPTION
    ---------------
    1.  Its shape
         1.  D_CLI_HELP_SHAPE
    2.  Writing it
         1.  d_cli_help_json
         2.  d_cli_program_set_help_json
*/

#ifndef DJINTERP_TOOLS_CLI_C_CLI_HELP_H
#define DJINTERP_TOOLS_CLI_C_CLI_HELP_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../c/djinterp.h"                                // framework root
#include "../../../c/modules/parse/cli/arg_parser/arg_common.h"  // dj_arg_buffer
#include "./cli_program.h"                                      // dj_cli_program
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // int64_t


D_EXTERN_C_BEGIN

//==============================================================================
// 1.  THE WRITER
//==============================================================================


// 1.1    The record
//------------------------------------------------------------------------------
// 1.1.1
// D_CLI_JSON_DEPTH
//   constant: the deepest a writer nests objects and arrays; a container
// opened past it fails the writer (D_ARG_ERROR_BAD_ARGUMENT).
#ifndef D_CLI_JSON_DEPTH
    #define D_CLI_JSON_DEPTH 32
#endif

// 1.1.2
// dj_cli_json
//   struct: a JSON writer over a buffer. `out` is where the text goes;
// `status` D_ARG_OK until a write fails, after which every call does
// nothing; `depth` how many containers are open; `open[k]` the k-th's
// bracket, '{' or '[', and `count[k]` how many values it holds so far --
// `count[0]` the values written outside any, at most one; `named` whether a
// member's name was just written, so that its value follows it on its line.
//   Public-payload layout: read any field; change one only through the calls
// below.
struct dj_cli_json
{
    struct dj_arg_buffer* out;
    int                   status;
    size_t                depth;
    char                  open[D_CLI_JSON_DEPTH + 1];
    size_t                count[D_CLI_JSON_DEPTH + 1];
    bool                  named;
};


// 1.2    Writing
//------------------------------------------------------------------------------
//   Every call takes a writer that has failed, or NULL, and does nothing. A
// value is a member's - after d_cli_json_member, in an object - or an
// element of an array, or the whole text, at depth 0.

// 1.2.1
// d_cli_json_init
//   setup: a writer over `_out`, which it appends to; `_out` NULL fails it.
D_CLI_API void d_cli_json_init(
    struct dj_cli_json*   _json,
    struct dj_arg_buffer* _out
);

// 1.2.2
// d_cli_json_object / d_cli_json_array / d_cli_json_end
//   writing: opens an object or an array as the next value; closes the
// innermost one, an empty one as `{}` or `[]` on its line.
D_CLI_API void d_cli_json_object(
    struct dj_cli_json* _json
);
D_CLI_API void d_cli_json_array(
    struct dj_cli_json* _json
);
D_CLI_API void d_cli_json_end(
    struct dj_cli_json* _json
);

// 1.2.3
// d_cli_json_member
//   writing: a member's name, in an object, its value to follow; NULL is
// written as "".
D_CLI_API void d_cli_json_member(
    struct dj_cli_json* _json,
    const char*         _name
);

// 1.2.4
// d_cli_json_string / d_cli_json_text
//   writing: text as a string, escaped, or `null` for NULL; the second of
// `_size` bytes, which need not end in a NUL.
D_CLI_API void d_cli_json_string(
    struct dj_cli_json* _json,
    const char*         _text
);
D_CLI_API void d_cli_json_text(
    struct dj_cli_json* _json,
    const char*         _text,
    size_t              _size
);

// 1.2.5
// d_cli_json_integer / d_cli_json_real / d_cli_json_bool / d_cli_json_null
//   writing: a number, a real one as the shortest text that reads back as
// it (`null` for a value JSON has no number for: an infinity, a NaN); a
// truth value; `null`.
D_CLI_API void d_cli_json_integer(
    struct dj_cli_json* _json,
    int64_t             _value
);
D_CLI_API void d_cli_json_real(
    struct dj_cli_json* _json,
    double              _value
);
D_CLI_API void d_cli_json_bool(
    struct dj_cli_json* _json,
    bool                _value
);
D_CLI_API void d_cli_json_null(
    struct dj_cli_json* _json
);

// 1.2.6
// d_cli_json_finish
//   writing: ends the text with a newline, once every container is closed.
// Returns the writer's status: D_ARG_OK; the buffer's failure; or
// D_ARG_ERROR_BAD_ARGUMENT for containers left open, too deep a nesting, a
// value where a member's name was due, or NULL.
D_CLI_API int d_cli_json_finish(
    struct dj_cli_json* _json
);


//==============================================================================
// 2.  THE DESCRIPTION
//==============================================================================


// 2.1    Its shape
//------------------------------------------------------------------------------
// 2.1.1
// D_CLI_HELP_SHAPE
//   constant: the description's shape, its first member's value. MAJOR.MINOR:
// a member added is a minor step, which a reader passes over; a member
// removed, or its meaning changed, is a major one.
#define D_CLI_HELP_SHAPE "djinterp.help/1.0"


// 2.2    Writing it
//------------------------------------------------------------------------------
// 2.2.1
// d_cli_help_json
/**
 * @brief Writes a program's description, as JSON, into a buffer.
 *
 * @note The members, in order: `format`, `program`, `version`, `command`
 *       (the command given, its options, arguments and subcommands, down),
 *       `settings`, `variables`, `prefixes`, `exit_statuses`; then the
 *       program's own, from its hook (d_cli_program_set_help_json). The
 *       options and commands help hides are left out, as help leaves them.
 *       HELP_JSON.html gives each member.
 *
 * @param[in]     _program  the program.
 * @param[in]     _command  the command to describe from; NULL for the
 *                          program's root.
 * @param[in,out] _out      the buffer it is appended to.
 * @return D_ARG_OK; the buffer's failure; the hook's, when it is not
 *         D_ARG_OK; D_ARG_ERROR_BAD_ARGUMENT for a NULL program or buffer.
 */
D_CLI_API int d_cli_help_json(
    const struct dj_cli_program* _program,
    const struct dj_arg_command* _command,
    struct dj_arg_buffer*        _out
);

// 2.2.2
// d_cli_program_set_help_json
/**
 * @brief Sets the hook that writes a program's own members of its
 *        description, after the kit's, before the object closes.
 *
 * @note The hook writes each member with d_cli_json_member and its value,
 *       and returns D_ARG_OK; any other status ends the description with
 *       it. HELP_JSON.html names the members a reader knows: `djins`,
 *       `reserved` and `sections`.
 *
 * @param[in,out] _program  the program.
 * @param[in]     _hook     the hook; NULL for none, as at first.
 * @param[in]     _context  given to the hook; the kit neither reads nor
 *                          frees it.
 */
D_CLI_API void d_cli_program_set_help_json(
    struct dj_cli_program* _program,
    dj_cli_help_json_fn    _hook,
    void*                  _context
);

D_EXTERN_C_END


#endif  // DJINTERP_TOOLS_CLI_C_CLI_HELP_H
