/*******************************************************************************
* djinterp [tools]                                                    cli_edit.c
*
* The command-line kit's commands that change a file, declared in cli_edit.h.
*   Defining is the parser's work, with help composed from what the command
* changes; a failure is kept where a run will say it -- the parser's own
* record, or, for a want of memory nothing recorded, the program's status.
* Where the result goes is read from the values the parse left; a file is
* written by d_cli_write_file, whole or not at all, and standard output
* through the program's own output, so that a program that sends its output
* elsewhere sends the result with it.
*
*
* path:      /src/djinterp/tools/cli/c/cli_edit.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.25
*                                                            revised: 2026.10.25
*******************************************************************************/
#include "../../../../../inc/djinterp/tools/cli/c/cli_edit.h"  // corresponding header
// std
#include <errno.h>    // EIO
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdlib.h>   // the hooks' malloc and free
#include <string.h>   // memcpy, strlen
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/c/util/sink_common.h"  // dj_pack_text, dj_pack_sink
#include "../../../../../inc/djinterp/config/tools/cli/cfg_cli.h"  // D_INTERNAL_CLI_*
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_command.h"  // adding options
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_option.h"  // options
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_parser.h"  // d_arg_parser_definition_error
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_type.h"  // d_arg_parse_bool
#include "../../../../../inc/djinterp/tools/cli/c/cli_io.h"  // d_cli_write_file
#include "../../../../../inc/djinterp/tools/cli/c/cli_program.h"  // dj_cli_program, d_cli_file_error


//==============================================================================
// 1.  DEFINING
//==============================================================================

// 1.1    The words
//------------------------------------------------------------------------------

/**
 * @brief An option of the edit's, as its help says it: its names; the
 *        words before and after what the command changes; and the name of
 *        its value, NULL for a flag. With no words after, the words before
 *        are all of them.
 */
struct dj_cli_internal_edit_option
{
    const char* names;
    const char* before;
    const char* after;
    const char* type_name;
};

// A's operand, written back; B's, written to standard output
static const char* const d_cli_internal_edit_back =
    ", written back where it was read; - for standard input, written to "
    "standard output";
static const char* const d_cli_internal_edit_output =
    ", written to standard output, or with -i back where it was read; - for "
    "standard input";

// A's `-o`; B's `-o`, `-i`; and `-n`, which names nothing
static const struct dj_cli_internal_edit_option d_cli_internal_edit_out_a =
{
    "-o,--out",
    "write ",
    " to FILE, - for standard output, and leave the one read as it was",
    "FILE"
};
static const struct dj_cli_internal_edit_option d_cli_internal_edit_out_b =
{
    "-o,--out",
    "write ",
    " to FILE, and leave the one read as it was",
    "FILE"
};
static const struct dj_cli_internal_edit_option d_cli_internal_edit_in_place =
{
    "-i,--in-place",
    "write ",
    " back where it was read",
    NULL
};
static const struct dj_cli_internal_edit_option d_cli_internal_edit_dry =
{
    "-n,--dry-run",
    "write nothing; say what would change",
    NULL,
    NULL
};

/**
 * @brief Composes an option's help: the words before, what the command
 *        changes, and the words after.
 *
 * @param[in] _before  the words before.
 * @param[in] _what    what the command changes.
 * @param[in] _after   the words after; NULL when `_before` is all of them.
 * @return the help, to be freed with D_INTERNAL_CLI_FREE; NULL when there
 *         was no room.
 */
static
char* d_cli_internal_edit_words(
    const char* _before,
    const char* _what,
    const char* _after
)
{
    const size_t before = strlen(_before);
    const size_t what   = (_after != NULL) ? strlen(_what) : 0u;
    const size_t after  = (_after != NULL) ? strlen(_after) : 0u;
    char* const  words  = (char*)D_INTERNAL_CLI_MALLOC(before + what +
                                                       after + 1u);

    if (!words)
    {
        return NULL;
    }

    memcpy(words,
           _before,
           before);
    memcpy(words + before,
           _what,
           what);
    memcpy(words + before + what,
           (_after != NULL) ? _after : "",
           after);
    words[before + what + after] = '\0';

    return words;
}

// 1.2    Failures
//------------------------------------------------------------------------------

/**
 * @brief Keeps why a definition failed where a run says it: a failure the
 *        parser recorded is said by every parse; a want of memory nothing
 *        recorded, by the program, whose status keeps it.
 *
 * @param[in,out] _program  the program.
 * @return the parser's definition error's status, or
 *         D_ARG_ERROR_OUT_OF_MEMORY.
 */
static
int d_cli_internal_edit_refused(
    struct dj_cli_program* _program
)
{
    const struct dj_arg_error* const error =
        d_arg_parser_definition_error(_program->parser);

    // the parser's record, which every parse says
    if ( (error != NULL) &&
         (error->status != (int)D_ARG_OK) )
    {
        return error->status;
    }

    _program->status = D_ARG_ERROR_OUT_OF_MEMORY;

    return D_ARG_ERROR_OUT_OF_MEMORY;
}

// 1.3    An option
//------------------------------------------------------------------------------

/**
 * @brief Adds one of the edit's options to a command.
 *
 * @param[in,out] _program  the program, which keeps a want of memory.
 * @param[in,out] _command  the command.
 * @param[in]     _option   the option, as its help says it.
 * @param[in]     _what     what the command changes.
 * @param[out]    _made     receives the option once added; untouched when
 *                          it was not.
 * @return D_ARG_OK, or why not, kept as d_cli_internal_edit_refused keeps
 *         it.
 */
static
int d_cli_internal_edit_add(
    struct dj_cli_program*                    _program,
    struct dj_arg_command*                    _command,
    const struct dj_cli_internal_edit_option* _option,
    const char*                               _what,
    struct dj_arg_option**                    _made
)
{
    char* const          words = d_cli_internal_edit_words(_option->before,
                                                           _what,
                                                           _option->after);
    struct dj_arg_option* made  = NULL;

    // a value's option, or a flag
    if (words != NULL)
    {
        made = (_option->type_name != NULL)
               ? d_arg_command_add_option(_command,
                                          _option->names,
                                          words)
               : d_arg_command_add_flag(_command,
                                        _option->names,
                                        words);
    }

    D_INTERNAL_CLI_FREE(words);

    if (!made)
    {
        return d_cli_internal_edit_refused(_program);
    }

    *_made = made;

    // the value's name, as help shows it
    if ( (_option->type_name != NULL) &&
         (d_arg_option_set_type_name(made,
                                     _option->type_name) != D_ARG_OK) )
    {
        return d_cli_internal_edit_refused(_program);
    }

    return D_ARG_OK;
}

// 1.4    The operand and the options
//------------------------------------------------------------------------------

/*
d_cli_edit_add_operand
  The operand, required, its help saying where the result goes by the
program's rule.
*/
struct dj_arg_option* d_cli_edit_add_operand(
    struct dj_cli_program* _program,
    struct dj_arg_command* _command,
    struct dj_cli_edit*    _edit,
    const char*            _name,
    const char*            _what
)
{
    // parameter validation
    if ( (!_program) ||
         (!_command) ||
         (!_edit)    ||
         (!_name)    ||
         (!_what) )
    {
        return NULL;
    }

    const bool  to_output = ( (_program->flags &
                               D_CLI_PROGRAM_EDITS_TO_OUTPUT) != 0u );
    char* const words     = d_cli_internal_edit_words(
                                _what,
                                "",
                                (to_output)
                                ? d_cli_internal_edit_output
                                : d_cli_internal_edit_back);
    struct dj_arg_option* const operand =
        (words != NULL)
        ? d_arg_command_add_option(_command,
                                   _name,
                                   words)
        : NULL;

    D_INTERNAL_CLI_FREE(words);

    if (!operand)
    {
        (void)d_cli_internal_edit_refused(_program);

        return NULL;
    }

    d_arg_option_set_required(operand,
                              true);
    _edit->operand = operand;

    return operand;
}

/*
d_cli_edit_add_options
  For B, `-i` first, the switch the rule turns on; `-o`, excluding it; and
`-n`.
*/
int d_cli_edit_add_options(
    struct dj_cli_program* _program,
    struct dj_arg_command* _command,
    struct dj_cli_edit*    _edit,
    const char*            _what
)
{
    // parameter validation
    if ( (!_program) ||
         (!_command) ||
         (!_edit)    ||
         (!_what) )
    {
        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    const bool to_output = ( (_program->flags &
                              D_CLI_PROGRAM_EDITS_TO_OUTPUT) != 0u );
    int        status    = (to_output)
                           ? d_cli_internal_edit_add(
                                 _program,
                                 _command,
                                 &d_cli_internal_edit_in_place,
                                 _what,
                                 &_edit->in_place)
                           : D_ARG_OK;

    if (status == D_ARG_OK)
    {
        status = d_cli_internal_edit_add(_program,
                                         _command,
                                         (to_output)
                                         ? &d_cli_internal_edit_out_b
                                         : &d_cli_internal_edit_out_a,
                                         _what,
                                         &_edit->out);
    }

    // B's two places, one or the other
    if ( (status == D_ARG_OK) &&
         (to_output)          &&
         (d_arg_option_excludes(_edit->out,
                                _edit->in_place) != D_ARG_OK) )
    {
        status = d_cli_internal_edit_refused(_program);
    }

    return (status == D_ARG_OK)
           ? d_cli_internal_edit_add(_program,
                                     _command,
                                     &d_cli_internal_edit_dry,
                                     _what,
                                     &_edit->dry_run)
           : status;
}


//==============================================================================
// 2.  WRITING
//==============================================================================

// 2.1    What the parse left
//------------------------------------------------------------------------------

/**
 * @brief An option's value, the last its policy kept.
 *
 * @param[in]  _option  the option; NULL reads as one not given.
 * @param[out] _value   receives the value when there is one.
 * @return whether there is one.
 */
static
bool d_cli_internal_edit_value(
    const struct dj_arg_option* _option,
    struct dj_pack_text*        _value
)
{
    size_t                          count  = 0u;
    const struct dj_pack_text* const values = d_arg_option_values(_option,
                                                                 &count);

    if (count == 0u)
    {
        return false;
    }

    *_value = values[count - 1u];

    return true;
}

/**
 * @brief Whether a flag is on: given, and its last value true --
 *        `--dry-run=false` is off, as a flag bound to a `bool` reads it.
 *
 * @param[in] _flag  the flag; NULL reads as one not given.
 * @return whether it is on.
 */
static
bool d_cli_internal_edit_on(
    const struct dj_arg_option* _flag
)
{
    struct dj_pack_text value = D_PACK_TEXT_NONE;
    bool               on    = false;

    return (d_cli_internal_edit_value(_flag,
                                      &value)) &&
           (d_arg_parse_bool(value,
                             _flag->integer_rule,
                             &on) == D_ARG_OK) &&
           (on);
}

/**
 * @brief Whether a name is `-`, a standard stream (R11).
 *
 * @param[in] _name  the name.
 * @return whether it is.
 */
static
bool d_cli_internal_edit_standard(
    struct dj_pack_text _name
)
{
    return ( (_name.length == 1u) &&
             (_name.data[0] == '-') );
}

// 2.2    Where it goes
//------------------------------------------------------------------------------

/*
d_cli_edit_target
  `-o` first, whatever the rule; then B's standard output, unless `-i`; then
the file read, or standard output for standard input.
*/
int d_cli_edit_target(
    const struct dj_cli_program* _program,
    const struct dj_cli_edit*    _edit
)
{
    struct dj_pack_text name = D_PACK_TEXT_NONE;

    // parameter validation
    if ( (!_program) ||
         (!_edit) )
    {
        return (int)D_CLI_EDIT_BACK;
    }

    // -o: a file, or standard output
    if (d_cli_internal_edit_value(_edit->out,
                                  &name))
    {
        return (d_cli_internal_edit_standard(name))
               ? (int)D_CLI_EDIT_OUTPUT
               : (int)D_CLI_EDIT_OUT;
    }

    // B, without -i
    if ( ( (_program->flags & D_CLI_PROGRAM_EDITS_TO_OUTPUT) != 0u ) &&
         (!d_cli_internal_edit_on(_edit->in_place)) )
    {
        return (int)D_CLI_EDIT_OUTPUT;
    }

    return ( (d_cli_internal_edit_value(_edit->operand,
                                        &name)) &&
             (d_cli_internal_edit_standard(name)) )
           ? (int)D_CLI_EDIT_OUTPUT
           : (int)D_CLI_EDIT_BACK;
}

/*
d_cli_edit_dry_run
  `-n`, read as a flag bound to a `bool` reads it.
*/
bool d_cli_edit_dry_run(
    const struct dj_cli_edit* _edit
)
{
    return (_edit != NULL) &&
           (d_cli_internal_edit_on(_edit->dry_run));
}

// 2.3    Writing it
//------------------------------------------------------------------------------

/**
 * @brief Writes the result to standard output, through the program's own
 *        output, whole.
 *
 * @param[in] _program  the program.
 * @param[in] _data     the bytes.
 * @param[in] _length   how many.
 * @return D_CLI_EXIT_OK; or D_CLI_EXIT_TROUBLE, said as `cannot write
 *         standard output: REASON`, when the output took fewer.
 */
static
int d_cli_internal_edit_output_write(
    const struct dj_cli_program* _program,
    const void*                  _data,
    size_t                       _length
)
{
    const struct dj_pack_sink out = _program->out;

    // nothing, or all of it
    if ( (_length == 0u) ||
         ( (out.write != NULL) &&
           (out.write(out.context,
                      _data,
                      _length) == _length) ) )
    {
        return D_CLI_EXIT_OK;
    }

    return d_cli_file_error(_program,
                            "write",
                            "-",
                            EIO);
}

/**
 * @brief Writes the result to a file, whole or not at all, its name copied
 *        to end it, as the parser's values need not be ended.
 *
 * @param[in] _program  the program.
 * @param[in] _name     the file's name, as the parse left it.
 * @param[in] _data     the bytes.
 * @param[in] _length   how many.
 * @return D_CLI_EXIT_OK; or D_CLI_EXIT_TROUBLE, said as d_cli_file_error
 *         says it, or as `out of memory` where the name could not be
 *         copied.
 */
static
int d_cli_internal_edit_file_write(
    const struct dj_cli_program* _program,
    struct dj_pack_text          _name,
    const void*                  _data,
    size_t                       _length
)
{
    char* const path = (char*)D_INTERNAL_CLI_MALLOC(_name.length + 1u);

    if (!path)
    {
        d_cli_report(_program,
                     (int)D_CLI_SEVERITY_ERROR,
                     "%s",
                     "out of memory");

        return D_CLI_EXIT_TROUBLE;
    }

    if (_name.length != 0u)
    {
        memcpy(path,
               _name.data,
               _name.length);
    }

    path[_name.length] = '\0';

    const int failed = d_cli_write_file(path,
                                        _data,
                                        _length);
    const int status = (failed != 0)
                       ? d_cli_file_error(_program,
                                          "write",
                                          path,
                                          failed)
                       : D_CLI_EXIT_OK;

    D_INTERNAL_CLI_FREE(path);

    return status;
}

/*
d_cli_edit_write
  Nothing after `-n`; else standard output, or the file -- `-o`'s, or the
one read.
*/
int d_cli_edit_write(
    const struct dj_cli_program* _program,
    const struct dj_cli_edit*    _edit,
    const void*                  _data,
    size_t                       _length
)
{
    struct dj_pack_text name = D_PACK_TEXT_NONE;

    // parameter validation
    if ( (!_program) ||
         (!_edit)    ||
         ( (!_data) &&
           (_length != 0u) ) )
    {
        return D_CLI_EXIT_TROUBLE;
    }

    // nothing to write
    if (d_cli_edit_dry_run(_edit))
    {
        return D_CLI_EXIT_OK;
    }

    const int place = d_cli_edit_target(_program,
                                        _edit);

    if (place == (int)D_CLI_EDIT_OUTPUT)
    {
        return d_cli_internal_edit_output_write(_program,
                                                _data,
                                                _length);
    }

    (void)d_cli_internal_edit_value((place == (int)D_CLI_EDIT_OUT)
                                    ? _edit->out
                                    : _edit->operand,
                                    &name);

    return d_cli_internal_edit_file_write(_program,
                                          name,
                                          _data,
                                          _length);
}
