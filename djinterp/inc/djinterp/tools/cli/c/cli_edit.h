/*******************************************************************************
* djinterp [tools]                                                    cli_edit.h
*
* The command-line kit's commands that change a file: where the result goes
* (ruling 12).
*   By default (A), a command writes the file back where it read it, whole
* or not at all (R20); `-o FILE` writes it elsewhere and leaves the one
* read as it was; `-n` writes nothing; a file read from standard input is
* written to standard output -- as `git config`, `cargo add` and `npm pkg
* set` treat the files they edit. A program made with
* D_CLI_PROGRAM_EDITS_TO_OUTPUT (B) writes standard output, as sed does,
* unless `-i` writes the file back where it was read, or `-o FILE` writes
* it elsewhere; `-i` and `-o` exclude each other.
*   A command declares its operand and its options in two calls, each where
* its other definitions put it -- the operand among its operands, so that
* the order they are read in is the command's; the options among its
* options -- and, once parsed, asks where the result goes and writes it
* there. The words of the operand's and the options' help name what the
* command changes, as the command gives it: "the sheet".
*
*
* path:      /inc/djinterp/tools/cli/c/cli_edit.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.25
*                                                            revised: 2026.10.25
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  RECORDS
    -------
    1.  A command that changes a file
         1.  dj_cli_edit
         2.  D_CLI_EDIT_INIT
    2.  Where the result goes
         1.  dj_cli_edit_place
2.  DEFINING
    --------
    1.  The operand and the options
         1.  d_cli_edit_add_operand
         2.  d_cli_edit_add_options
3.  WRITING
    -------
    1.  Where it goes
         1.  d_cli_edit_target
         2.  d_cli_edit_dry_run
    2.  Writing it
         1.  d_cli_edit_write
*/

#ifndef DJINTERP_TOOLS_CLI_C_CLI_EDIT_H
#define DJINTERP_TOOLS_CLI_C_CLI_EDIT_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../c/djinterp.h"                           // framework root
#include "../../../c/modules/parse/cli/arg_parser/arg_command.h"   // dj_arg_command
#include "../../../c/modules/parse/cli/arg_parser/arg_option.h"    // dj_arg_option
#include "./cli_program.h"                                 // dj_cli_program


//==============================================================================
// 1.  RECORDS
//==============================================================================


// 1.1    A command that changes a file
//------------------------------------------------------------------------------
// 1.1.1
// dj_cli_edit
//   struct: what a command that changes a file was given, as the parser
// holds it: its operand, the file read; `-o, --out=FILE`; `-n, --dry-run`;
// and, for a program that writes standard output by default (B),
// `-i, --in-place`, NULL otherwise. Each is NULL until its call adds it.
//   Public-payload layout: read any field; set them only through the calls
// below.
struct dj_cli_edit
{
    struct dj_arg_option* operand;
    struct dj_arg_option* out;
    struct dj_arg_option* dry_run;
    struct dj_arg_option* in_place;
};

// 1.1.2
// D_CLI_EDIT_INIT
//   macro: a dj_cli_edit with nothing added.
#define D_CLI_EDIT_INIT  { NULL, NULL, NULL, NULL }

// 1.2    Where the result goes
//------------------------------------------------------------------------------
// 1.2.1
// dj_cli_edit_place
//   enum: where a command that changes a file writes its result: back over
// the file read; to the file `-o` names; or to standard output -- `-o -`,
// a file read from standard input, or, for B, the default.
enum dj_cli_edit_place
{
    D_CLI_EDIT_BACK   = 0,
    D_CLI_EDIT_OUT    = 1,
    D_CLI_EDIT_OUTPUT = 2
};


D_EXTERN_C_BEGIN

//==============================================================================
// 2.  DEFINING
//==============================================================================


// 2.1    The operand and the options
//------------------------------------------------------------------------------
// 2.1.1
// d_cli_edit_add_operand
/**
 * @brief Adds a command's operand, the file it changes, required, and keeps
 *        it in the record.
 *
 * @note Its help says where the result goes, after `_what`: A's "WHAT,
 *       written back where it was read; - for standard input, written to
 *       standard output"; B's "WHAT, written to standard output, or with -i
 *       back where it was read; - for standard input". Call it where the
 *       command's other operands are added, in the order they are read.
 *
 * @param[in,out] _program  the program, whose flags choose the rule; the
 *                          want of memory is kept in its `status`, for its
 *                          run to say.
 * @param[in,out] _command  the command.
 * @param[in,out] _edit     the record, which keeps the operand.
 * @param[in]     _name     the operand's name: `SHEET`.
 * @param[in]     _what     what the command changes, as its help names it:
 *                          "the sheet".
 * @return the operand; NULL when it could not be added, the parser keeping
 *         why, or for a NULL argument.
 */
struct dj_arg_option* d_cli_edit_add_operand(
    struct dj_cli_program* _program,
    struct dj_arg_command* _command,
    struct dj_cli_edit*    _edit,
    const char*            _name,
    const char*            _what
);

// 2.1.2
// d_cli_edit_add_options
/**
 * @brief Adds a command's options for where its result goes, and keeps them
 *        in the record.
 *
 * @note A: `-o, --out=FILE` -- "write WHAT to FILE, - for standard output,
 *       and leave the one read as it was" -- and `-n, --dry-run` -- "write
 *       nothing; say what would change". B: `-i, --in-place` -- "write WHAT
 *       back where it was read" -- then `-o, --out=FILE` -- "write WHAT to
 *       FILE, and leave the one read as it was" -- excluding each other, and
 *       `-n, --dry-run`. Call it where the command's other options are
 *       added.
 *
 * @param[in,out] _program  the program, whose flags choose the rule; the
 *                          want of memory is kept in its `status`.
 * @param[in,out] _command  the command.
 * @param[in,out] _edit     the record, which keeps the options.
 * @param[in]     _what     what the command changes, as its help names it.
 * @return D_ARG_OK; or the first failure, the parser keeping a definition's
 *         and the program the want of memory; D_ARG_ERROR_BAD_ARGUMENT for
 *         a NULL argument.
 */
int d_cli_edit_add_options(
    struct dj_cli_program* _program,
    struct dj_arg_command* _command,
    struct dj_cli_edit*    _edit,
    const char*            _what
);


//==============================================================================
// 3.  WRITING
//==============================================================================


// 3.1    Where it goes
//------------------------------------------------------------------------------
// 3.1.1
// d_cli_edit_target
//   lookup: after a parse, where the result is written, `-n` aside -- so a
// command can refuse a place it has nothing to write back to, a dry run or
// not: `-o`'s file, or standard output for `-o -`; else, for B without
// `-i`, standard output; else back where it was read, or standard output
// for a file read from standard input. D_CLI_EDIT_BACK for a NULL argument.
int d_cli_edit_target(
    const struct dj_cli_program* _program,
    const struct dj_cli_edit*    _edit
);

// 3.1.2
// d_cli_edit_dry_run
//   lookup: after a parse, whether `-n` was given; `false` for NULL.
bool d_cli_edit_dry_run(
    const struct dj_cli_edit* _edit
);

// 3.2    Writing it
//------------------------------------------------------------------------------
// 3.2.1
// d_cli_edit_write
/**
 * @brief Writes a command's result where it goes: nothing after `-n`; else
 *        a file whole or not at all (d_cli_write_file), or standard output
 *        through the program's own output.
 *
 * @param[in] _program  the program, which says a failure.
 * @param[in] _edit     the record, parsed.
 * @param[in] _data     the bytes; may be NULL when `_length` is 0.
 * @param[in] _length   how many bytes.
 * @return D_CLI_EXIT_OK when written, or nothing was to be; else
 *         D_CLI_EXIT_TROUBLE, said as d_cli_file_error says it -- `cannot
 *         write 'FILE': REASON`, `cannot write standard output: REASON` --
 *         or as `out of memory`, the file as it was; D_CLI_EXIT_TROUBLE,
 *         unsaid, for a NULL program or record, or NULL data with a length.
 */
int d_cli_edit_write(
    const struct dj_cli_program* _program,
    const struct dj_cli_edit*    _edit,
    const void*                  _data,
    size_t                       _length
);

D_EXTERN_C_END


#endif  // DJINTERP_TOOLS_CLI_C_CLI_EDIT_H
