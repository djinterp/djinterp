/*******************************************************************************
* djinterp [tools]                                              cli_edit_tests.c
*
* The suite of the command-line kit's commands that change a file, declared
* in cli_edit_tests.h.
*   Two fixtures make the programs the case tables run, alike but for their
* rule: BACK (A) and OUTPUT (B), `prog 1.2`, each with two commands. `add
* FILE LINE` reads the file -- or standard input, for `-` -- and writes it
* with the line after its last, where the edit sends it; in a dry run it
* says what it would add, and at `-v` what it added. `where FILE` writes
* nothing but where the result would go: `back`, `out` or `output`, then
* the file's name -- the operand's, or `-o`'s -- and `(dry run)` after `-n`.
* Each command keeps its edit record as its context, freed with the
* program.
*   The writing section runs in a scratch directory: before each case,
* `notes.txt` holds two lines and `copy.txt` is not there; after it, each
* holds what the case says, or is still not there.
*
*
* path:      /tests/djinterp/tools/cli/c/cli_edit_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.25
*                                                            revised: 2026.10.25
*******************************************************************************/
#include "./cli_edit_tests.h"  // corresponding header
// std
#include <errno.h>    // ENOMEM
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdio.h>    // remove
#include <stdlib.h>   // malloc, free
#include <string.h>   // memcpy, strlen
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_command.h"  // commands
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_common.h"  // statuses
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_option.h"  // options
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_parser.h"  // the parser
#include "../../../../../inc/djinterp/tools/cli/c/cli_edit.h"  // the edit
#include "../../../../../inc/djinterp/tools/cli/c/cli_io.h"  // d_cli_read_file
#include "../../../../../inc/djinterp/tools/cli/c/cli_program.h"  // the program
#include "../../../c/modules/parse/cli/arg_parser/arg_tests_files.h"  // scratch directories
#include "../../../c/modules/parse/tests_section.h"  // dj_tests_section
#include "./cli_tests_support.h"  // cases, checks, files
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// D_TESTS_CLI_EDIT_COUNT
//   macro: how many entries a table has.
#define D_TESTS_CLI_EDIT_COUNT(_table)  (sizeof(_table) / sizeof((_table)[0]))

// D_TESTS_CLI_EDIT_NOTES / D_TESTS_CLI_EDIT_ADDED
//   constants: what `notes.txt` holds before a case, and with `three`
// added.
#define D_TESTS_CLI_EDIT_NOTES  "one\ntwo\n"
#define D_TESTS_CLI_EDIT_ADDED  "one\ntwo\nthree\n"

/**
 * @brief The words a place is printed as by `where`.
 *
 * @param[in] _place  a dj_cli_edit_place value.
 * @return the words.
 */
static
const char* d_tests_cli_edit_place(
    int _place
)
{
    return (_place == (int)D_CLI_EDIT_OUTPUT) ? "output"
         : (_place == (int)D_CLI_EDIT_OUT)    ? "out"
                                              : "back";
}

/**
 * @brief `where`'s work: where the result would go, and the file's name.
 *
 * @param[in] _program  the program.
 * @param[in] _command  `where`.
 * @param[in] _context  its edit record.
 * @return D_CLI_EXIT_OK.
 */
static
int d_tests_cli_edit_work_where(
    struct dj_cli_program* _program,
    struct dj_arg_command* _command,
    void*                  _context
)
{
    const struct dj_cli_edit* const edit  = (const struct dj_cli_edit*)_context;
    const int                      place = d_cli_edit_target(_program,
                                                             edit);
    char                           name[64];

    (void)d_tests_cli_values(_command,
                             (place == (int)D_CLI_EDIT_OUT) ? "--out"
                                                            : "FILE",
                             name,
                             sizeof(name));
    d_tests_cli_print(_program,
                      "%s %s%s\n",
                      d_tests_cli_edit_place(place),
                      name,
                      (d_cli_edit_dry_run(edit)) ? " (dry run)" : "");

    return D_CLI_EXIT_OK;
}

/**
 * @brief The file read, with a line after its last, in a block of its own.
 *
 * @param[in]  _data    the file's bytes.
 * @param[in]  _length  how many.
 * @param[in]  _line    the line.
 * @param[out] _size    receives the block's length.
 * @return the block, to be freed with free; NULL when there was no room.
 */
static
char* d_tests_cli_edit_added(
    const char* _data,
    size_t      _length,
    const char* _line,
    size_t*     _size
)
{
    const size_t line  = strlen(_line);
    char* const  added = (char*)malloc(_length + line + 1u);

    if (!added)
    {
        return NULL;
    }

    if (_length != 0u)
    {
        memcpy(added,
               _data,
               _length);
    }

    memcpy(added + _length,
           _line,
           line);
    added[_length + line] = '\n';
    *_size                = _length + line + 1u;

    return added;
}

/**
 * @brief What `add` says once its result is written, or not: in a dry run,
 *        what it would add; at `-v`, what it added.
 *
 * @param[in] _program  the program.
 * @param[in] _edit     its edit record.
 * @param[in] _line     the line.
 * @param[in] _status   the write's status.
 */
static
void d_tests_cli_edit_said(
    const struct dj_cli_program* _program,
    const struct dj_cli_edit*    _edit,
    const char*                  _line,
    int                          _status
)
{
    if (d_cli_edit_dry_run(_edit))
    {
        d_cli_report(_program,
                     (int)D_CLI_SEVERITY_NOTE,
                     "would add '%s' (a dry run: nothing was written)",
                     _line);
    }
    else if (_status == D_CLI_EXIT_OK)
    {
        d_cli_note(_program,
                   1,
                   "added '%s'",
                   _line);
    }

    return;
}

/**
 * @brief `add`'s work: the file read, the line added, the result written
 *        where the edit sends it, and what was done said.
 *
 * @param[in] _program  the program.
 * @param[in] _command  `add`.
 * @param[in] _context  its edit record.
 * @return the write's status; D_CLI_EXIT_TROUBLE, said, when the file could
 *         not be read or the result made.
 */
static
int d_tests_cli_edit_work_add(
    struct dj_cli_program* _program,
    struct dj_arg_command* _command,
    void*                  _context
)
{
    const struct dj_cli_edit* const edit   = (const struct dj_cli_edit*)_context;
    char                           file[64];
    char                           line[64];
    char*                          data   = NULL;
    size_t                         length = 0u;
    size_t                         size   = 0u;

    (void)d_tests_cli_values(_command, "LINE", line, sizeof(line));

    const int failed = d_cli_read_file(d_tests_cli_values(_command,
                                                          "FILE",
                                                          file,
                                                          sizeof(file)),
                                       &data,
                                       &length);

    if (failed != 0)
    {
        return d_cli_file_error(_program, "read", file, failed);
    }

    char* const added  = d_tests_cli_edit_added(data, length, line, &size);
    const int   status = (added != NULL)
                         ? d_cli_edit_write(_program, edit, added, size)
                         : d_cli_file_error(_program, "read", file, ENOMEM);

    d_cli_free(data);
    free(added);

    if (added != NULL)
    {
        d_tests_cli_edit_said(_program, edit, line, status);
    }

    return status;
}

/**
 * @brief Adds a command that changes a file: its edit record, as its
 *        context; `FILE`; `LINE`, for `add`; and the options for where the
 *        result goes.
 *
 * @param[in,out] _program      the program.
 * @param[in]     _name         the command's name.
 * @param[in]     _description  what it does.
 * @param[in]     _run          its work.
 * @param[in]     _line         whether it takes `LINE`.
 * @return whether everything was made.
 */
static
bool d_tests_cli_edit_command(
    struct dj_cli_program* _program,
    const char*            _name,
    const char*            _description,
    dj_cli_command_fn      _run,
    bool                   _line
)
{
    static const struct dj_cli_edit none = D_CLI_EDIT_INIT;
    struct dj_cli_edit* const       edit = malloc(sizeof(*edit));

    if (!edit)
    {
        return false;
    }

    *edit = none;

    // the command keeps the record, and frees it, made or not
    struct dj_arg_command* const command = d_cli_program_add_command(
                                              _program,
                                              NULL,
                                              _name,
                                              _description,
                                              _run,
                                              edit,
                                              free);
    struct dj_arg_option*        line    = NULL;

    if ( (!command) ||
         (!d_cli_edit_add_operand(_program,
                                  command,
                                  edit,
                                  "FILE",
                                  "the file")) )
    {
        return false;
    }

    // the line, read after the file
    if (_line)
    {
        line = d_arg_command_add_option(command,
                                        "LINE",
                                        "the line to add");

        if (!line)
        {
            return false;
        }

        d_arg_option_set_required(line,
                                  true);
    }

    return (d_cli_edit_add_options(_program,
                                   command,
                                   edit,
                                   "the file") == D_ARG_OK);
}

/**
 * @brief Makes a program of the two commands, by a rule.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser.
 * @param[in]     _flags    the program's flags, D_CLI_PROGRAM_COMMANDS
 *                          joined.
 * @return whether everything was made.
 */
static
bool d_tests_cli_edit_make_with(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser,
    uint32_t               _flags
)
{
    return (d_cli_program_init(_program,
                               _parser,
                               "prog",
                               "1.2",
                               _flags | D_CLI_PROGRAM_COMMANDS) ==
            D_ARG_OK)                                                      &&
           (d_tests_cli_edit_command(_program,
                                     "add",
                                     "add a line to a file",
                                     d_tests_cli_edit_work_add,
                                     true))                                &&
           (d_tests_cli_edit_command(_program,
                                     "where",
                                     "say where the file would be written",
                                     d_tests_cli_edit_work_where,
                                     false));
}

/*
d_tests_cli_edit_make_back / d_tests_cli_edit_make_output
  The fixtures: A, the default; B.
*/
static
bool d_tests_cli_edit_make_back(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
)
{
    return d_tests_cli_edit_make_with(_program,
                                      _parser,
                                      0u);
}

static
bool d_tests_cli_edit_make_output(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
)
{
    return d_tests_cli_edit_make_with(_program,
                                      _parser,
                                      D_CLI_PROGRAM_EDITS_TO_OUTPUT);
}


//==============================================================================
// 1.  HELP
//==============================================================================

// HELP_BACK / HELP_OUTPUT
//   constants: `add`'s help, by each rule; the operand left out, and an
// option the rule does not have, or two it has that exclude each other.
static const struct dj_tests_cli_case HELP_BACK[] =
{
    { "H01", "prog add --help", 0,
      "Usage: prog add [OPTION]... FILE LINE\n"
      "Add a line to a file.\n"
      "\n"
      "Arguments:\n"
      "  FILE                the file, written back where it was read; - for "
      "standard\n"
      "                      input, written to standard output\n"
      "  LINE                the line to add\n"
      "\n"
      "Options:\n"
      "  -o, --out=FILE      write the file to FILE, - for standard output, "
      "and leave\n"
      "                      the one read as it was\n"
      "  -n, --dry-run       write nothing; say what would change\n"
      "  -h, --help          display this help and exit\n"
      "\n"
      "Global options:\n"
      "  -q, --quiet         say nothing but errors\n"
      "  -v, --verbose       say more; twice, more again\n"
      "      --color[=WHEN]  colour diagnostics: auto (the default), always or "
      "never\n",
      "" },
    { "H02", "prog add", 2, "",
      "prog: error: add: FILE is required\n"
      "Try 'prog add --help' for more information.\n" },
    { "H03", "prog add notes.txt three -i", 2, "",
      "prog: error: add: the following argument was not expected: -i\n"
      "Try 'prog add --help' for more information.\n" }
};
static const struct dj_tests_cli_case HELP_OUTPUT[] =
{
    { "H04", "prog add --help", 0,
      "Usage: prog add [OPTION]... FILE LINE\n"
      "Add a line to a file.\n"
      "\n"
      "Arguments:\n"
      "  FILE                the file, written to standard output, or with -i "
      "back\n"
      "                      where it was read; - for standard input\n"
      "  LINE                the line to add\n"
      "\n"
      "Options:\n"
      "  -i, --in-place      write the file back where it was read\n"
      "  -o, --out=FILE      write the file to FILE, and leave the one read as "
      "it was\n"
      "  -n, --dry-run       write nothing; say what would change\n"
      "  -h, --help          display this help and exit\n"
      "\n"
      "Global options:\n"
      "  -q, --quiet         say nothing but errors\n"
      "  -v, --verbose       say more; twice, more again\n"
      "      --color[=WHEN]  colour diagnostics: auto (the default), always or "
      "never\n",
      "" },
    { "H05", "prog add notes.txt three -i -o copy.txt", 2, "",
      "prog: error: add: --in-place excludes --out\n"
      "Try 'prog add --help' for more information.\n" },
    { "H06", "prog add notes.txt three --out copy.txt --in-place", 2, "",
      "prog: error: add: --in-place excludes --out\n"
      "Try 'prog add --help' for more information.\n" }
};

/*
d_tests_cli_edit_help
  HELP_BACK on BACK; HELP_OUTPUT on OUTPUT.
*/
bool d_tests_cli_edit_help(void)
{
    const bool held = d_tests_cli_cases_run(HELP_BACK,
                                            D_TESTS_CLI_EDIT_COUNT(HELP_BACK),
                                            d_tests_cli_edit_make_back);

    return d_tests_cli_cases_run(HELP_OUTPUT,
                                 D_TESTS_CLI_EDIT_COUNT(HELP_OUTPUT),
                                 d_tests_cli_edit_make_output) && held;
}


//==============================================================================
// 2.  WHERE IT GOES
//==============================================================================

// WHERE_BACK
//   constant: A's places: the file read; standard output for standard
// input; `-o`'s file, or standard output for `-o -`, whatever was read, the
// last `-o` taken; a dry run, which changes nothing of where, and one
// turned off.
static const struct dj_tests_cli_case WHERE_BACK[] =
{
    { "P01", "prog where notes.txt", 0, "back notes.txt\n", "" },
    { "P02", "prog where -", 0, "output -\n", "" },
    { "P03", "prog where notes.txt -o copy.txt", 0, "out copy.txt\n", "" },
    { "P04", "prog where notes.txt -o -", 0, "output notes.txt\n", "" },
    { "P05", "prog where - --out=copy.txt", 0, "out copy.txt\n", "" },
    { "P06", "prog where notes.txt -o a.txt -o copy.txt", 0,
      "out copy.txt\n", "" },
    { "P07", "prog where notes.txt -n", 0, "back notes.txt (dry run)\n", "" },
    { "P08", "prog where notes.txt -n -o copy.txt", 0,
      "out copy.txt (dry run)\n", "" },
    { "P09", "prog where notes.txt --dry-run=false", 0,
      "back notes.txt\n", "" },
    { "P10", "prog where -- -", 0, "output -\n", "" }
};

// WHERE_OUTPUT
//   constant: B's: standard output; the file read with `-i`, or standard
// output for standard input; `-o`'s file; `-i` turned off; a dry run.
static const struct dj_tests_cli_case WHERE_OUTPUT[] =
{
    { "P11", "prog where notes.txt", 0, "output notes.txt\n", "" },
    { "P12", "prog where notes.txt -i", 0, "back notes.txt\n", "" },
    { "P13", "prog where - --in-place", 0, "output -\n", "" },
    { "P14", "prog where notes.txt -o copy.txt", 0, "out copy.txt\n", "" },
    { "P15", "prog where notes.txt -o -", 0, "output notes.txt\n", "" },
    { "P16", "prog where notes.txt --in-place=false", 0,
      "output notes.txt\n", "" },
    { "P17", "prog where notes.txt -i -n", 0, "back notes.txt (dry run)\n", "" }
};

/*
d_tests_cli_edit_where
  WHERE_BACK on BACK; WHERE_OUTPUT on OUTPUT.
*/
bool d_tests_cli_edit_where(void)
{
    const bool held = d_tests_cli_cases_run(WHERE_BACK,
                                            D_TESTS_CLI_EDIT_COUNT(WHERE_BACK),
                                            d_tests_cli_edit_make_back);

    return d_tests_cli_cases_run(WHERE_OUTPUT,
                                 D_TESTS_CLI_EDIT_COUNT(WHERE_OUTPUT),
                                 d_tests_cli_edit_make_output) && held;
}


//==============================================================================
// 3.  WRITING
//==============================================================================

// dj_tests_cli_edit_write
//   struct: a write's case: the case, its program's fixture, whether
// standard input reads `notes.txt`, and what `notes.txt` and `copy.txt`
// hold after it -- NULL for `copy.txt` not there.
struct dj_tests_cli_edit_write
{
    struct dj_tests_cli_case test;
    dj_tests_cli_make_fn     make;
    bool                    input;
    const char*             notes;
    const char*             copy;
};

// WRITES
//   constant: A's writes -- back; to `-o`'s file; to standard output; a dry
// run; standard input to standard output; at `-v`; a file that cannot be
// written, for a directory not there and for a directory -- then B's --
// standard output; back with `-i`; to `-o`'s file; a dry run.
static const struct dj_tests_cli_edit_write WRITES[] =
{
    { { "W01", "prog add notes.txt three", 0, "", "" },
      d_tests_cli_edit_make_back, false, D_TESTS_CLI_EDIT_ADDED, NULL },
    { { "W02", "prog add notes.txt three -o copy.txt", 0, "", "" },
      d_tests_cli_edit_make_back, false, D_TESTS_CLI_EDIT_NOTES,
      D_TESTS_CLI_EDIT_ADDED },
    { { "W03", "prog add notes.txt three -o -", 0, "one\ntwo\nthree\n", "" },
      d_tests_cli_edit_make_back, false, D_TESTS_CLI_EDIT_NOTES, NULL },
    { { "W04", "prog add notes.txt three -n", 0, "",
        "prog: note: would add 'three' (a dry run: nothing was written)\n" },
      d_tests_cli_edit_make_back, false, D_TESTS_CLI_EDIT_NOTES, NULL },
    { { "W05", "prog add - three", 0, "one\ntwo\nthree\n", "" },
      d_tests_cli_edit_make_back, true, D_TESTS_CLI_EDIT_NOTES, NULL },
    { { "W06", "prog -v add notes.txt three", 0, "",
        "prog: note: added 'three'\n" },
      d_tests_cli_edit_make_back, false, D_TESTS_CLI_EDIT_ADDED, NULL },
    { { "W07", "prog add notes.txt three -o none/copy.txt", 2, "",
        "prog: error: cannot write 'none/copy.txt': No such file or "
        "directory\n" },
      d_tests_cli_edit_make_back, false, D_TESTS_CLI_EDIT_NOTES, NULL },
    { { "W08", "prog add notes.txt three -o folder", 2, "",
        "prog: error: cannot write 'folder': Is a directory\n" },
      d_tests_cli_edit_make_back, false, D_TESTS_CLI_EDIT_NOTES, NULL },
    { { "W09", "prog add notes.txt three", 0, "one\ntwo\nthree\n", "" },
      d_tests_cli_edit_make_output, false, D_TESTS_CLI_EDIT_NOTES, NULL },
    { { "W10", "prog add notes.txt three -i", 0, "", "" },
      d_tests_cli_edit_make_output, false, D_TESTS_CLI_EDIT_ADDED, NULL },
    { { "W11", "prog add notes.txt three -o copy.txt", 0, "", "" },
      d_tests_cli_edit_make_output, false, D_TESTS_CLI_EDIT_NOTES,
      D_TESTS_CLI_EDIT_ADDED },
    { { "W12", "prog add notes.txt three -i -n", 0, "",
        "prog: note: would add 'three' (a dry run: nothing was written)\n" },
      d_tests_cli_edit_make_output, false, D_TESTS_CLI_EDIT_NOTES, NULL }
};

/**
 * @brief Whether a file holds what a case says it should, or, for NULL, is
 *        not there; a difference said under the case's tag.
 *
 * @param[in] _tag   the case's tag.
 * @param[in] _name  the file's name.
 * @param[in] _want  what it should hold; NULL for no file.
 * @return whether it does.
 */
static
bool d_tests_cli_edit_left(
    const char* _tag,
    const char* _name,
    const char* _want
)
{
    size_t      length = 0u;
    char* const held   = d_tests_cli_file_read(_name,
                                               &length);
    const bool  same   = (_want != NULL)
                         ? d_tests_cli_same(_tag,
                                            _name,
                                            _want,
                                            held,
                                            length)
                         : d_tests_cli_check((held == NULL),
                                             _tag,
                                             "a file written that should "
                                             "not have been");

    free(held);

    return same;
}

/**
 * @brief Runs a write's case: the files as they are before every case,
 *        standard input in place if the case reads it, the case, then the
 *        files as it left them.
 *
 * @param[in] _write  the case.
 * @return whether it came to what it says.
 */
static
bool d_tests_cli_edit_write_run(
    const struct dj_tests_cli_edit_write* _write
)
{
    (void)remove("copy.txt");

    bool held = d_tests_cli_check(
                    d_tests_cli_file_write("notes.txt",
                                           D_TESTS_CLI_EDIT_NOTES,
                                           strlen(D_TESTS_CLI_EDIT_NOTES)),
                    _write->test.tag,
                    "notes.txt could not be written");
    const int saved = (_write->input)
                      ? d_tests_cli_redirect(0,
                                             "notes.txt")
                      : -1;

    held = (held) &&
           (d_tests_cli_case_run(&_write->test,
                                 _write->make));
    d_tests_cli_restore(0,
                        saved);

    held = d_tests_cli_edit_left(_write->test.tag,
                                 "notes.txt",
                                 _write->notes) && held;

    return d_tests_cli_edit_left(_write->test.tag,
                                 "copy.txt",
                                 _write->copy) && held;
}

/*
d_tests_cli_edit_writing
  WRITES in a scratch directory holding `folder`, a directory.
*/
bool d_tests_cli_edit_writing(void)
{
    struct dj_tests_arg_files files;
    bool                     held = d_tests_arg_files_enter(&files);

    held = (held) &&
           (d_tests_arg_files_folder(&files,
                                     "folder"));

    if (!d_tests_cli_check(held,
                           "W01",
                           "the scratch directory could not be made"))
    {
        d_tests_arg_files_leave(&files);

        return false;
    }

    for (size_t index = 0u; index < D_TESTS_CLI_EDIT_COUNT(WRITES); ++index)
    {
        held = d_tests_cli_edit_write_run(&WRITES[index]) && held;
    }

    (void)remove("notes.txt");
    (void)remove("copy.txt");
    d_tests_arg_files_leave(&files);

    return held;
}


//==============================================================================
// 4.  NULLS
//==============================================================================

/**
 * @brief N01: the definitions given nothing, each in turn: nothing added.
 *
 * @param[in,out] _program  a program made.
 * @param[in,out] _root     its root.
 * @return whether every check held.
 */
static
bool d_tests_cli_edit_nulls_defined(
    struct dj_cli_program* _program,
    struct dj_arg_command* _root
)
{
    struct dj_cli_edit edit = D_CLI_EDIT_INIT;

    return d_tests_cli_check(
               (!d_cli_edit_add_operand(NULL, _root, &edit, "F", "x"))      &&
               (!d_cli_edit_add_operand(_program, NULL, &edit, "F", "x"))   &&
               (!d_cli_edit_add_operand(_program, _root, NULL, "F", "x"))   &&
               (!d_cli_edit_add_operand(_program, _root, &edit, NULL, "x")) &&
               (!d_cli_edit_add_operand(_program, _root, &edit, "F", NULL)) &&
               (d_cli_edit_add_options(NULL, _root, &edit, "x") ==
                D_ARG_ERROR_BAD_ARGUMENT)                                   &&
               (d_cli_edit_add_options(_program, NULL, &edit, "x") ==
                D_ARG_ERROR_BAD_ARGUMENT)                                   &&
               (d_cli_edit_add_options(_program, _root, NULL, "x") ==
                D_ARG_ERROR_BAD_ARGUMENT)                                   &&
               (d_cli_edit_add_options(_program, _root, &edit, NULL) ==
                D_ARG_ERROR_BAD_ARGUMENT)                                   &&
               (edit.operand == NULL)                                       &&
               (edit.out == NULL)                                           &&
               (edit.dry_run == NULL)                                       &&
               (edit.in_place == NULL),
               "N01",
               "definitions given nothing add nothing");
}

/*
d_tests_cli_edit_nulls
  Every call given nothing: nothing added (N01); the place back, no dry
run, and the write refused, unsaid (N02).
*/
bool d_tests_cli_edit_nulls(void)
{
    struct dj_cli_program    program;
    struct dj_arg_parser     parser;
    struct dj_arg_buffer     out  = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer     err  = D_ARG_BUFFER_INIT;
    const struct dj_cli_edit edit = D_CLI_EDIT_INIT;
    bool                    held = d_tests_cli_start(&program,
                                                     &parser,
                                                     d_tests_cli_edit_make_back,
                                                     &out,
                                                     &err);

    if (!d_tests_cli_check(held,
                           "N01",
                           "the program could not be made"))
    {
        return false;
    }

    held = d_tests_cli_edit_nulls_defined(&program,
                                          &parser.command);
    held = d_tests_cli_check(
               (d_cli_edit_target(NULL, &edit) == (int)D_CLI_EDIT_BACK)    &&
               (d_cli_edit_target(&program, NULL) == (int)D_CLI_EDIT_BACK) &&
               (d_cli_edit_target(&program, &edit) ==
                (int)D_CLI_EDIT_BACK)                                      &&
               (!d_cli_edit_dry_run(NULL))                                 &&
               (!d_cli_edit_dry_run(&edit))                                &&
               (d_cli_edit_write(NULL, &edit, "x", 1u) ==
                D_CLI_EXIT_TROUBLE)                                        &&
               (d_cli_edit_write(&program, NULL, "x", 1u) ==
                D_CLI_EXIT_TROUBLE)                                        &&
               (d_cli_edit_write(&program, &edit, NULL, 1u) ==
                D_CLI_EXIT_TROUBLE)                                        &&
               (out.length == 0u)                                          &&
               (err.length == 0u),
               "N02",
               "queries and the write given nothing") && held;

    d_tests_cli_finish(&program,
                       &parser);
    d_arg_buffer_release(&out);
    d_arg_buffer_release(&err);

    return held;
}


//==============================================================================
// THE SUITE
//==============================================================================

// d_tests_cli_edit
//   constant: the suite's sections, in the order they run.
const struct dj_tests_section d_tests_cli_edit[] =
{
    { "cli_edit_help",    d_tests_cli_edit_help    },
    { "cli_edit_where",   d_tests_cli_edit_where   },
    { "cli_edit_writing", d_tests_cli_edit_writing },
    { "cli_edit_nulls",   d_tests_cli_edit_nulls   }
};

// d_tests_cli_edit_count
//   constant: how many sections d_tests_cli_edit has.
const size_t d_tests_cli_edit_count =
    sizeof(d_tests_cli_edit) / sizeof(d_tests_cli_edit[0]);
