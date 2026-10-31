/*******************************************************************************
* djinterp [tools]                                           cli_operand_tests.c
*
* The suite of the command-line kit's operands that name something other
* than a file, declared in cli_operand_tests.h.
*   NAMED is the program the cases run: `prog 1.2`, with `show WORD`, whose
* work reads WORD as a named operand of the prefix `example`, a built-in
* example's, and prints `named NAME` or `file WORD`. The cases run in a
* scratch directory holding `notes.txt` and, where a file's name may hold a
* colon, a file named `example:x`, a directory named `example:dir`, a link
* named `example:link` to `notes.txt`, and one named `example:gone` that
* leads nowhere.
*
*
* path:      /tests/djinterp/tools/cli/c/cli_operand_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.25
*                                                            revised: 2026.10.25
*******************************************************************************/
#include "./cli_operand_tests.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_command.h"  // commands
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_common.h"  // statuses
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_option.h"  // options
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_parser.h"  // the parser
#include "../../../../../inc/djinterp/tools/cli/c/cli_operand.h"  // named operands
#include "../../../../../inc/djinterp/tools/cli/c/cli_program.h"  // the program
#include "../../../c/modules/parse/cli/arg_parser/arg_tests_files.h"  // scratch directories
#include "../../../c/modules/parse/tests_section.h"  // dj_tests_section
#include "./cli_tests_support.h"  // cases, checks


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// D_TESTS_CLI_OPERAND_COUNT
//   macro: how many entries a table has.
#define D_TESTS_CLI_OPERAND_COUNT(_table)                                      \
    (sizeof(_table) / sizeof((_table)[0]))

/**
 * @brief `show`'s work: WORD read as a named operand of `example`, and what
 *        it is printed.
 *
 * @param[in] _program  the program.
 * @param[in] _command  `show`.
 * @param[in] _context  unused.
 * @return D_CLI_EXIT_OK; D_CLI_EXIT_TROUBLE when the read said a failure.
 */
static
int d_tests_cli_operand_work_show(
    struct dj_cli_program* _program,
    struct dj_arg_command* _command,
    void*                  _context
)
{
    char        word[64];
    const char* name   = NULL;
    const int   status = d_cli_named_operand(_program,
                                             d_tests_cli_values(_command,
                                                                "WORD",
                                                                word,
                                                                sizeof(word)),
                                             "example",
                                             "the built-in example",
                                             &name);

    (void)_context;

    if (status != D_ARG_OK)
    {
        return D_CLI_EXIT_TROUBLE;
    }

    d_tests_cli_print(_program,
                      "%s %s\n",
                      (name != NULL) ? "named" : "file",
                      (name != NULL) ? name : word);

    return D_CLI_EXIT_OK;
}

/**
 * @brief NAMED: `prog 1.2`, with `show WORD`.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser.
 * @return whether it was made.
 */
static
bool d_tests_cli_operand_make(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
)
{
    if (d_cli_program_init(_program,
                           _parser,
                           "prog",
                           "1.2",
                           D_CLI_PROGRAM_COMMANDS) != D_ARG_OK)
    {
        return false;
    }

    struct dj_arg_command* const show = d_cli_program_add_command(
                                           _program,
                                           NULL,
                                           "show",
                                           "say what a word names",
                                           d_tests_cli_operand_work_show,
                                           NULL,
                                           NULL);
    struct dj_arg_option* const  word = (show != NULL)
                                       ? d_arg_command_add_option(
                                             show,
                                             "WORD",
                                             "a file, or example:NAME")
                                       : NULL;

    if (word != NULL)
    {
        d_arg_option_set_required(word,
                                  true);
    }

    return (word != NULL);
}


//==============================================================================
// 1.  NAMED OPERANDS
//==============================================================================

// NAMES
//   constant: a named operand, where no file has its name; then words that
// are files' names: another file's, the prefix with no name, the prefix
// spelled otherwise, the word written as a path, a prefix a letter short and
// one a letter long.
static const struct dj_tests_cli_case NAMES[] =
{
    { "O01", "prog show example:uart_cr", 0, "named uart_cr\n", "" },
    { "O02", "prog show notes.txt",       0, "file notes.txt\n", "" },
    { "O03", "prog show example:",        0, "file example:\n", "" },
    { "O04", "prog show Example:x",       0, "file Example:x\n", "" },
    { "O05", "prog show ./example:x",     0, "file ./example:x\n", "" },
    { "O06", "prog show exampl:x",        0, "file exampl:x\n", "" },
    { "O07", "prog show examples:x",      0, "file examples:x\n", "" }
};

#if (D_TESTS_ARG_FILES_POSIX == 1)

// FILES
//   constant: a named operand that is a file's name too: said, and still the
// example; not said under `-q`; a directory, not said; a link to a file,
// said; a link that leads nowhere, not said.
static const struct dj_tests_cli_case FILES[] =
{
    { "O08", "prog show example:x", 0, "named x\n",
      "prog: warning: 'example:x' is the built-in example; the file is "
      "'./example:x'\n" },
    { "O09", "prog -q show example:x",  0, "named x\n", "" },
    { "O10", "prog show example:dir",   0, "named dir\n", "" },
    { "O11", "prog show example:link", 0, "named link\n",
      "prog: warning: 'example:link' is the built-in example; the file is "
      "'./example:link'\n" },
    { "O12", "prog show example:gone",  0, "named gone\n", "" }
};

#endif  // D_TESTS_ARG_FILES_POSIX

/**
 * @brief Makes what the cases look at in the scratch directory: `notes.txt`
 *        and, where a name may hold a colon, the file, the directory and
 *        the two links.
 *
 * @param[in,out] _files  the scratch directory.
 * @return whether everything was made.
 */
static
bool d_tests_cli_operand_files(
    struct dj_tests_arg_files* _files
)
{
    bool made = d_tests_arg_files_file(_files,
                                       "notes.txt",
                                       "one\n");

#if (D_TESTS_ARG_FILES_POSIX == 1)
    made = (made)                                        &&
           (d_tests_arg_files_file(_files,
                                   "example:x",
                                   "a file\n"))         &&
           (d_tests_arg_files_folder(_files,
                                     "example:dir"))     &&
           (d_tests_arg_files_link(_files,
                                   "example:link",
                                   "notes.txt"))         &&
           (d_tests_arg_files_link(_files,
                                   "example:gone",
                                   "nowhere.txt"));
#endif

    return made;
}

/*
d_tests_cli_operand_named
  NAMES, and where a name may hold a colon FILES, on NAMED, in a scratch
directory.
*/
bool d_tests_cli_operand_named(void)
{
    struct dj_tests_arg_files files;
    bool                     held = d_tests_arg_files_enter(&files);

    held = (held) &&
           (d_tests_cli_operand_files(&files));

    if (!d_tests_cli_check(held,
                           "O01",
                           "the scratch directory could not be made"))
    {
        d_tests_arg_files_leave(&files);

        return false;
    }

    held = d_tests_cli_cases_run(NAMES,
                                 D_TESTS_CLI_OPERAND_COUNT(NAMES),
                                 d_tests_cli_operand_make);
#if (D_TESTS_ARG_FILES_POSIX == 1)
    held = d_tests_cli_cases_run(FILES,
                                 D_TESTS_CLI_OPERAND_COUNT(FILES),
                                 d_tests_cli_operand_make) && held;
#endif

    d_tests_arg_files_leave(&files);

    return held;
}


//==============================================================================
// 2.  NULLS
//==============================================================================

/**
 * @brief Whether a call is refused, unsaid, with no name given.
 *
 * @param[in] _program  the program, or NULL.
 * @param[in] _word     the operand, or NULL.
 * @param[in] _prefix   the prefix, or NULL.
 * @param[in] _what     what it names, or NULL.
 * @return whether it was refused so.
 */
static
bool d_tests_cli_operand_refused(
    const struct dj_cli_program* _program,
    const char*                  _word,
    const char*                  _prefix,
    const char*                  _what
)
{
    const char* name = "untouched";

    return (d_cli_named_operand(_program,
                                _word,
                                _prefix,
                                _what,
                                &name) == D_ARG_ERROR_BAD_ARGUMENT) &&
           (name == NULL);
}

/*
d_tests_cli_operand_nulls
  Each argument given nothing; then prefixes R13 does not allow: a letter,
the empty prefix, a capital, a digit -- and one it does, `ex`.
*/
bool d_tests_cli_operand_nulls(void)
{
    struct dj_cli_program program;
    struct dj_arg_parser  parser;
    struct dj_arg_buffer  out  = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer  err  = D_ARG_BUFFER_INIT;
    const char*          name = NULL;
    bool                 held = d_tests_cli_start(&program,
                                                  &parser,
                                                  d_tests_cli_operand_make,
                                                  &out,
                                                  &err);

    if (!d_tests_cli_check(held,
                           "N01",
                           "the program could not be made"))
    {
        return false;
    }

    held = d_tests_cli_check(
               (d_tests_cli_operand_refused(NULL, "ex:a", "ex", "x"))     &&
               (d_tests_cli_operand_refused(&program, NULL, "ex", "x"))   &&
               (d_tests_cli_operand_refused(&program, "ex:a", NULL, "x")) &&
               (d_tests_cli_operand_refused(&program, "ex:a", "ex", NULL)) &&
               (d_cli_named_operand(&program, "ex:a", "ex", "x", NULL) ==
                D_ARG_ERROR_BAD_ARGUMENT),
               "N01",
               "each argument given nothing");
    held = d_tests_cli_check(
               (d_tests_cli_operand_refused(&program, "c:a", "c", "x"))   &&
               (d_tests_cli_operand_refused(&program, ":a", "", "x"))     &&
               (d_tests_cli_operand_refused(&program, "Ex:a", "Ex", "x")) &&
               (d_tests_cli_operand_refused(&program, "e1:a", "e1", "x")) &&
               (d_cli_named_operand(&program, "ex:a", "ex", "x", &name) ==
                D_ARG_OK)                                                 &&
               (name != NULL)                                             &&
               (name[0] == 'a')                                           &&
               (out.length == 0u)                                         &&
               (err.length == 0u),
               "N02",
               "prefixes R13 does not allow, and one it does") && held;

    d_tests_cli_finish(&program,
                       &parser);
    d_arg_buffer_release(&out);
    d_arg_buffer_release(&err);

    return held;
}


//==============================================================================
// THE SUITE
//==============================================================================

// d_tests_cli_operand
//   constant: the suite's sections, in the order they run.
const struct dj_tests_section d_tests_cli_operand[] =
{
    { "cli_operand_named", d_tests_cli_operand_named },
    { "cli_operand_nulls", d_tests_cli_operand_nulls }
};

// d_tests_cli_operand_count
//   constant: how many sections d_tests_cli_operand has.
const size_t d_tests_cli_operand_count =
    sizeof(d_tests_cli_operand) / sizeof(d_tests_cli_operand[0]);
