/*******************************************************************************
* djinterp [tools]                                          cli_settings_tests.c
*
* The suite of the command-line kit's settings, declared in
* cli_settings_tests.h.
*   SHOWN is the program the cases run: `prog 1.2`, with `--level LEVEL` and
* `-o, --output FILE` on its root, and `show [WORD]...` with `--mode MODE`,
* whose work prints what the kit reads -- `cli.plugin_strict_version`,
* `cli.plugin_search` (`default` for the knob's), `plugin_path` (`-` for
* none) -- and the values the parser gave the options and the operands.
* Every case's program lets go of the run's settings first, so that each
* starts them as a run of its own would.
*
*
* path:      /tests/djinterp/tools/cli/c/cli_settings_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/
#include "./cli_settings_tests.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdio.h>    // remove, snprintf
#include <stdlib.h>   // getenv
#include <string.h>   // strcmp, strlen
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/c/filesys/file_dir.h"  // d_dir_*
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_command.h"  // commands
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_option.h"  // options
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_parser.h"  // the parser
#include "../../../../../inc/djinterp/config/tools/cli/cfg_cli.h"  // the knobs
#include "../../../../../inc/djinterp/tools/cli/c/cli_plugin.h"  // dj_cli_plugin_versions
#include "../../../../../inc/djinterp/tools/cli/c/cli_program.h"  // the program
#include "../../../../../inc/djinterp/tools/cli/c/cli_settings.h"  // settings
#include "../../../c/modules/parse/tests_section.h"  // dj_tests_section
#include "./cli_tests_support.h"  // cases, checks
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // RE_STD_HAS_INT64

// D_TESTS_CLI_SETTINGS_ON
//   macro: 1 where the suite has its sections: the option core built, and
// the kit built for testing, which lets go of a run's settings
// (d_cli_settings_reset).
#if defined(D_TESTING)
    #if (D_TESTING == 1) && RE_STD_HAS_INT64
        #define D_TESTS_CLI_SETTINGS_ON 1
    #endif
#endif
#ifndef D_TESTS_CLI_SETTINGS_ON
    #define D_TESTS_CLI_SETTINGS_ON 0
#endif

#if D_TESTS_CLI_SETTINGS_ON


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// D_TESTS_CLI_SETTINGS_COUNT
//   macro: how many entries a table has.
#define D_TESTS_CLI_SETTINGS_COUNT(_table)                                     \
    (sizeof(_table) / sizeof((_table)[0]))

// D_TESTS_CLI_SETTINGS_ROOM
//   macro: the room for a path, or a line of what is said.
#define D_TESTS_CLI_SETTINGS_ROOM 512u

/**
 * @brief `show`'s work: what the kit reads, and what the parser gave.
 *
 * @param[in] _program  the program.
 * @param[in] _command  `show`.
 * @param[in] _context  unused.
 * @return D_CLI_EXIT_OK.
 */
static
int d_tests_cli_settings_work_show(
    struct dj_cli_program* _program,
    struct dj_arg_command* _command,
    void*                  _context
)
{
    const char* const search = d_cli_settings_plugin_search();
    const char* const path   = d_cli_settings_plugin_path();
    char              level[64];
    char              output[64];
    char              mode[64];
    char              words[128];

    (void)_context;

    d_tests_cli_print(
        _program,
        "strict=%d search=%s path=%s level=%s output=%s mode=%s words=%s\n",
        d_cli_settings_plugin_strict_version(_program->plugin_versions)
            ? 1 : 0,
        (strcmp(search, D_CFG_CLI_PLUGIN_SEARCH) == 0) ? "default" : search,
        (path != NULL) ? path : "-",
        d_tests_cli_values(_command, "--level", level, sizeof(level)),
        d_tests_cli_values(_command, "--output", output, sizeof(output)),
        d_tests_cli_values(_command, "--mode", mode, sizeof(mode)),
        d_tests_cli_values(_command, "WORD", words, sizeof(words)));

    return D_CLI_EXIT_OK;
}

/**
 * @brief SHOWN: `prog 1.2`, with `--level` and `-o, --output` on its root,
 *        and `show [WORD]...` with `--mode`; the run's settings let go of
 *        first.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser.
 * @return whether it was made.
 */
static
bool d_tests_cli_settings_make(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
)
{
    d_cli_settings_reset();

    if (d_cli_program_init(_program,
                           _parser,
                           "prog",
                           "1.2",
                           D_CLI_PROGRAM_COMMANDS) != D_ARG_OK)
    {
        return false;
    }

    struct dj_arg_command* const root = &_parser->command;
    struct dj_arg_command* const show = d_cli_program_add_command(
                                           _program,
                                           NULL,
                                           "show",
                                           "say what the kit reads",
                                           d_tests_cli_settings_work_show,
                                           NULL,
                                           NULL);

    return ( (show != NULL) &&
             (d_arg_command_add_option(root, "--level",
                                       "a level") != NULL) &&
             (d_arg_command_add_option(root, "-o,--output",
                                       "a file") != NULL) &&
             (d_arg_command_add_option(show, "--mode",
                                       "a mode") != NULL) &&
             (d_arg_command_add_option(show, "WORD",
                                       "words") != NULL) );
}

/**
 * @brief Puts the user's settings in a scratch directory of the cases'
 *        own, `cli_settings_home_` and the C level under the current one --
 *        each level's suite runs beside the others' -- holding a file of
 *        `_text`, or none for NULL.
 *
 * @param[out] _path      receives the file's path.
 * @param[in]  _capacity  its room.
 * @param[in]  _text      the file's text, or NULL.
 * @return whether it was done.
 */
static
bool d_tests_cli_settings_home(
    char*       _path,
    size_t      _capacity,
    const char* _text
)
{
    char   home[D_TESTS_CLI_SETTINGS_ROOM];
    char   folder[D_TESTS_CLI_SETTINGS_ROOM + 16u];
    size_t length;

    if (d_dir_get_current(home, sizeof(home) - 32u) == NULL)
    {
        return false;
    }

    (void)snprintf(home + strlen(home), 32u, "/cli_settings_home_%ld",
                   (long)__STDC_VERSION__);
    (void)snprintf(folder, sizeof(folder), "%s/djinterp", home);

    if ( (!d_tests_cli_setenv("XDG_CONFIG_HOME", home)) ||
         (!d_tests_cli_setenv("APPDATA", home)) ||
         (d_dir_create_parents(folder, 0755u) != 0) )
    {
        return false;
    }

    length = d_cli_settings_file(_path, _capacity);

    if ( (length == 0u) ||
         (length >= _capacity) )
    {
        return false;
    }

    (void)remove(_path);

    return ( (_text == NULL) ||
             (d_tests_cli_file_write(_path, _text, strlen(_text))) );
}

/**
 * @brief Unsets the environment's settings the cases set.
 */
static
void d_tests_cli_settings_unset(void)
{
    static const char* const NAMES[] =
    {
        "DJINTERP_CLI_PLUGIN_STRICT_VERSION",
        "PROG_CLI_PLUGIN_STRICT_VERSION",
        "DJINTERP_CLI_PLUGIN_SEARCH",
        "PROG_CLI_PLUGIN_SEARCH",
        "PROG_PLUGIN_PATH"
    };

    for (size_t i = 0u; i < D_TESTS_CLI_SETTINGS_COUNT(NAMES); ++i)
    {
        (void)d_tests_cli_setenv(NAMES[i], NULL);
    }

    return;
}


//==============================================================================
// 1.  TAKEN
//==============================================================================

// TAKEN
//   constant: settings taken in each form, before the command and after it,
// and arguments never taken: an option's value, and everything past `--`.
static const struct dj_tests_cli_case TAKEN[] =
{
    { "T01", "prog show a", 0,
      "strict=1 search=default path=- level=- output=- mode=- words=a\n",
      "" },
    { "T02", "prog --cli.plugin_strict_version=false show a", 0,
      "strict=0 search=default path=- level=- output=- mode=- words=a\n",
      "" },
    { "T03", "prog --no-cli.plugin_strict_version show a", 0,
      "strict=0 search=default path=- level=- output=- mode=- words=a\n",
      "" },
    { "T04", "prog --set cli.plugin_strict_version=false show", 0,
      "strict=0 search=default path=- level=- output=- mode=- words=-\n",
      "" },
    { "T05", "prog show a --cli.plugin_strict_version=0", 0,
      "strict=0 search=default path=- level=- output=- mode=- words=a\n",
      "" },
    { "T06", "prog show --no-cli.plugin_strict_version "
             "--cli.plugin_strict_version", 0,
      "strict=1 search=default path=- level=- output=- mode=- words=-\n",
      "" },
    { "T07", "prog --level --cli.plugin_strict_version=false show", 0,
      "strict=1 search=default path=- "
      "level=--cli.plugin_strict_version=false output=- mode=- words=-\n",
      "" },
    { "T08", "prog -o --no-cli.plugin_strict_version show", 0,
      "strict=1 search=default path=- level=- "
      "output=--no-cli.plugin_strict_version mode=- words=-\n",
      "" },
    { "T09", "prog show --mode --set x", 0,
      "strict=1 search=default path=- level=- output=- mode=--set "
      "words=x\n",
      "" },
    { "T10", "prog show -- --cli.plugin_strict_version=false", 0,
      "strict=1 search=default path=- level=- output=- mode=- "
      "words=--cli.plugin_strict_version=false\n",
      "" },
    { "T11", "prog --level=--x.y show", 0,
      "strict=1 search=default path=- level=--x.y output=- mode=- "
      "words=-\n",
      "" },
#if (D_INTERNAL_CLI_PLUGIN_VARIABLE == 1)
    { "T12", "prog --cli.plugin_search=/x show", 0,
      "strict=1 search=/x path=- level=- output=- mode=- words=-\n",
      "" },
    { "T13", "prog --set plugin_path=/p --set 'cli.plugin_search=/a b' "
             "show", 0,
      "strict=1 search=/a b path=/p level=- output=- mode=- words=-\n",
      "" },
#endif
};

/*
d_tests_cli_settings_taken
  Tests the following:
  - `--NAME=VALUE`, `--no-NAME`, `--set NAME=VALUE` taken, before the
    command and after it, after its operand
  - of two given, the last binds, as R8 has an option take the last
  - an option's value is kept for it, whatever it looks like: `--level`'s,
    `-o`'s, a subcommand's `--mode`'s, `--level=--x.y`'s
  - nothing past `--` is taken
  - `cli.plugin_search` and `plugin_path` given, and read by the kit
*/
bool d_tests_cli_settings_taken(void)
{
    char path[D_TESTS_CLI_SETTINGS_ROOM];

    d_tests_cli_settings_unset();

    return ( (d_tests_cli_check(d_tests_cli_settings_home(path, sizeof(path),
                                                          NULL),
                                "T00",
                                "a scratch home for the user's settings")) &&
             (d_tests_cli_cases_run(TAKEN,
                                    D_TESTS_CLI_SETTINGS_COUNT(TAKEN),
                                    d_tests_cli_settings_make)) );
}


//==============================================================================
// 2.  REFUSED
//==============================================================================

// REFUSED
//   constant: settings the command line names badly, each a usage error
// that names the option as typed.
static const struct dj_tests_cli_case REFUSED[] =
{
    { "R01", "prog --a.b=1 show", 2, "",
      "prog: error: --a.b: no setting is named 'a.b'\n"
      "Try 'prog --help' for more information.\n" },
    { "R02", "prog --cli.plugin_strict_versio=1 show", 2, "",
      "prog: error: --cli.plugin_strict_versio: no setting is named "
      "'cli.plugin_strict_versio'; did you mean "
      "'cli.plugin_strict_version'?\n"
      "Try 'prog --help' for more information.\n" },
    { "R03", "prog --cli.plugin_strict_version=maybe show", 2, "",
      "prog: error: --cli.plugin_strict_version: 'maybe' is not a value "
      "'cli.plugin_strict_version' takes\n"
      "Try 'prog --help' for more information.\n" },
    { "R04", "prog --set", 2, "",
      "prog: error: --set: NAME=VALUE is required\n"
      "Try 'prog --help' for more information.\n" },
    { "R05", "prog --set cli show", 2, "",
      "prog: error: --set: NAME=VALUE is required\n"
      "Try 'prog --help' for more information.\n" },
    { "R06", "prog show --x.y=1", 2, "",
      "prog: error: show: --x.y: no setting is named 'x.y'\n"
      "Try 'prog show --help' for more information.\n" },
    { "R07", "prog --set nosuch=1 show", 2, "",
      "prog: error: --set: no setting is named 'nosuch'\n"
      "Try 'prog --help' for more information.\n" }
};

/*
d_tests_cli_settings_refused
  Tests the following:
  - a name that names no setting, with the nearest that does where one is
    near, from the root and from a subcommand
  - a value the setting does not take
  - `--set` with no NAME=VALUE after it, and with a word that is not one
  - each exits 2 and writes nothing to standard output
*/
bool d_tests_cli_settings_refused(void)
{
    char path[D_TESTS_CLI_SETTINGS_ROOM];

    d_tests_cli_settings_unset();

    return ( (d_tests_cli_check(d_tests_cli_settings_home(path, sizeof(path),
                                                          NULL),
                                "R00",
                                "a scratch home for the user's settings")) &&
             (d_tests_cli_cases_run(REFUSED,
                                    D_TESTS_CLI_SETTINGS_COUNT(REFUSED),
                                    d_tests_cli_settings_make)) );
}


//==============================================================================
// 3.  THE USER'S FILE
//==============================================================================

// FILE_TEXT
//   constant: a user's file: a comment, a setting, a name that names none,
// and two statements on one line, the second misspelt.
static const char FILE_TEXT[] =
    "# the user's settings\n"
    "cli.plugin_strict_version = false\n"
    "cli.nosuch = 1\n"
    "cli.plugin_strict_version = maybe ; cli.plugin_strict_versoin = 1\n";

/*
d_tests_cli_settings_file
  Tests the following:
  - the file's settings bind, from FILE
  - a statement that fails is said by its file and line, as a warning, and
    the file is read on: a name that names none; a value the setting does
    not take; one misspelt after another on its line, the nearest named
  - the command line stands above the file
  - a file that is not there says nothing
  - where the file is: XDG_CONFIG_HOME's, and the home's .config when
    XDG_CONFIG_HOME is not a full path
*/
bool d_tests_cli_settings_file(void)
{
    char                     path[D_TESTS_CLI_SETTINGS_ROOM];
    char                     said[D_TESTS_CLI_SETTINGS_ROOM * 4u];
    struct dj_tests_cli_case cases[3];
    bool                     held;

    d_tests_cli_settings_unset();
    held = d_tests_cli_check(d_tests_cli_settings_home(path, sizeof(path),
                                                       FILE_TEXT),
                             "F00",
                             "a scratch home holding the user's settings");

    (void)snprintf(said, sizeof(said),
                   "%s:3: warning: no setting is named 'cli.nosuch'\n"
                   "%s:4: warning: 'maybe' is not a value "
                   "'cli.plugin_strict_version' takes\n"
                   "%s:4: warning: no setting is named "
                   "'cli.plugin_strict_versoin'; did you mean "
                   "'cli.plugin_strict_version'?\n",
                   path, path, path);

    cases[0].tag    = "F01";
    cases[0].line   = "prog show";
    cases[0].status = 0;
    cases[0].out    = "strict=0 search=default path=- level=- output=- "
                      "mode=- words=-\n";
    cases[0].err    = said;
    cases[1]        = cases[0];
    cases[1].tag    = "F02";
    cases[1].line   = "prog --cli.plugin_strict_version=1 show";
    cases[1].out    = "strict=1 search=default path=- level=- output=- "
                      "mode=- words=-\n";
    held = (d_tests_cli_cases_run(cases, 2u, d_tests_cli_settings_make)) &&
           (held);

    // no file: nothing said
    held = (d_tests_cli_settings_home(path, sizeof(path), NULL)) && (held);
    cases[2]     = cases[0];
    cases[2].tag = "F03";
    cases[2].out = "strict=1 search=default path=- level=- output=- "
                   "mode=- words=-\n";
    cases[2].err = "";
    held = (d_tests_cli_cases_run(cases + 2, 1u,
                                  d_tests_cli_settings_make)) && (held);

#if !D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    {
        static const char  WANT[] = "/home/someone/.config/djinterp/settings";
        const char* const  home   = getenv("HOME");
        char               kept[D_TESTS_CLI_SETTINGS_ROOM];

        (void)snprintf(kept, sizeof(kept), "%s", (home != NULL) ? home : "");

        // not a full path: the home's
        (void)d_tests_cli_setenv("XDG_CONFIG_HOME", "relative");
        (void)d_tests_cli_setenv("HOME", "/home/someone");
        held = d_tests_cli_check(
                   (d_cli_settings_file(path, sizeof(path)) ==
                    sizeof(WANT) - 1u) &&
                   (strcmp(path, WANT) == 0),
                   "F04",
                   "the home's .config, XDG_CONFIG_HOME not a full path") &&
               (held);
        (void)d_tests_cli_setenv("HOME", (home != NULL) ? kept : NULL);
        held = (d_tests_cli_settings_home(path, sizeof(path), NULL)) &&
               (held);
    }
#endif

    return held;
}


//==============================================================================
// 4.  THE ENVIRONMENT
//==============================================================================

// ENVIRONMENT
//   constant: a case for each state of the environment, set before it.
static const struct
{
    const char*              shared;   // DJINTERP_CLI_PLUGIN_STRICT_VERSION
    const char*              own;      // PROG_CLI_PLUGIN_STRICT_VERSION
    struct dj_tests_cli_case run;
} ENVIRONMENT[] =
{
    { "0", NULL,
      { "E01", "prog show", 0,
        "strict=0 search=default path=- level=- output=- mode=- "
        "words=-\n", "" } },
    { "0", "1",
      { "E02", "prog show", 0,
        "strict=1 search=default path=- level=- output=- mode=- "
        "words=-\n", "" } },
    { NULL, "false",
      { "E03", "prog --cli.plugin_strict_version=true show", 0,
        "strict=1 search=default path=- level=- output=- mode=- "
        "words=-\n", "" } }
};

/*
d_tests_cli_settings_environment
  Tests the following:
  - DJINTERP_ binds a setting of the kit's djin
  - the program's own prefix binds it too, and wins over DJINTERP_
  - the command line stands above the environment
  - PROG_PLUGIN_PATH binds the program's plugin_path
*/
bool d_tests_cli_settings_environment(void)
{
    char path[D_TESTS_CLI_SETTINGS_ROOM];
    bool held = d_tests_cli_settings_home(path, sizeof(path), NULL);

    for (size_t i = 0u; i < D_TESTS_CLI_SETTINGS_COUNT(ENVIRONMENT); ++i)
    {
        d_tests_cli_settings_unset();
        (void)d_tests_cli_setenv("DJINTERP_CLI_PLUGIN_STRICT_VERSION",
                                 ENVIRONMENT[i].shared);
        (void)d_tests_cli_setenv("PROG_CLI_PLUGIN_STRICT_VERSION",
                                 ENVIRONMENT[i].own);
        held = (d_tests_cli_case_run(&ENVIRONMENT[i].run,
                                     d_tests_cli_settings_make)) && (held);
    }

#if (D_INTERNAL_CLI_PLUGIN_VARIABLE == 1)
    {
        static const struct dj_tests_cli_case PATH =
        {
            "E04", "prog show", 0,
            "strict=1 search=/s path=/env level=- output=- mode=- "
            "words=-\n", ""
        };

        d_tests_cli_settings_unset();
        (void)d_tests_cli_setenv("PROG_PLUGIN_PATH", "/env");
        (void)d_tests_cli_setenv("DJINTERP_CLI_PLUGIN_SEARCH", "/s");
        held = (d_tests_cli_case_run(&PATH,
                                     d_tests_cli_settings_make)) && (held);
    }
#endif

    d_tests_cli_settings_unset();

    return held;
}


//==============================================================================
// 5.  WHAT THE KIT READS
//==============================================================================

/*
d_tests_cli_settings_reads
  Tests the following:
  - before the settings start, each reads its knob, or nothing
  - the program's own say decides where no source above it gave one
  - the environment, above the program, decides over it
*/
bool d_tests_cli_settings_reads(void)
{
    struct dj_cli_program program;
    struct dj_arg_parser  parser;
    struct dj_arg_buffer  out = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer  err = D_ARG_BUFFER_INIT;
    static const char*    SHOW[] = { "prog", "show", NULL };
    char                  path[D_TESTS_CLI_SETTINGS_ROOM];
    bool                  held;

    d_tests_cli_settings_unset();
    d_cli_settings_reset();

    held = d_tests_cli_check(
               (strcmp(d_cli_settings_plugin_search(),
                       D_CFG_CLI_PLUGIN_SEARCH) == 0) &&
               (d_cli_settings_plugin_path() == NULL) &&
               (d_cli_settings_plugin_strict_version(
                    (int)D_CLI_PLUGIN_VERSIONS_WARN) == false) &&
               (d_cli_settings_plugin_strict_version(
                    (int)D_CLI_PLUGIN_VERSIONS_REFUSE) == true) &&
               (d_cli_settings_plugin_strict_version(
                    (int)D_CLI_PLUGIN_VERSIONS_DEFAULT) ==
                (D_INTERNAL_CLI_PLUGIN_STRICT_VERSION == 1)),
               "K01",
               "before the settings start: the knobs, and the program's");

    // the environment above the program's own say
    (void)d_tests_cli_setenv("PROG_CLI_PLUGIN_STRICT_VERSION", "true");
    held = (d_tests_cli_settings_home(path, sizeof(path), NULL)) &&
           (d_tests_cli_start(&program, &parser, d_tests_cli_settings_make,
                              &out, &err)) &&
           (held);
    held = d_tests_cli_check(
               (d_cli_program_run(&program, 2, SHOW) == D_CLI_EXIT_OK) &&
               (d_cli_settings_plugin_strict_version(
                    (int)D_CLI_PLUGIN_VERSIONS_WARN) == true),
               "K02",
               "the environment stands above the program's own say") &&
           (held);

    d_tests_cli_finish(&program, &parser);
    d_arg_buffer_release(&out);
    d_arg_buffer_release(&err);
    d_tests_cli_settings_unset();
    d_cli_settings_reset();

    return held;
}


//==============================================================================
// THE SUITE
//==============================================================================

// d_tests_cli_settings
//   constant: the suite's sections, in the order they run.
const struct dj_tests_section d_tests_cli_settings[] =
{
    { "cli_settings_taken",       d_tests_cli_settings_taken       },
    { "cli_settings_refused",     d_tests_cli_settings_refused     },
    { "cli_settings_file",        d_tests_cli_settings_file        },
    { "cli_settings_environment", d_tests_cli_settings_environment },
    { "cli_settings_reads",       d_tests_cli_settings_reads       }
};

// d_tests_cli_settings_count
//   constant: how many sections d_tests_cli_settings has.
const size_t d_tests_cli_settings_count =
    sizeof(d_tests_cli_settings) / sizeof(d_tests_cli_settings[0]);

#else  // !D_TESTS_CLI_SETTINGS_ON

// without the option core the kit keeps no settings, and outside testing a
// run's cannot be let go of: the suite has no sections; a table holds one
// entry, never run
const struct dj_tests_section d_tests_cli_settings[1] = { { NULL, NULL } };
const size_t                 d_tests_cli_settings_count = 0u;

#endif  // D_TESTS_CLI_SETTINGS_ON
