/*******************************************************************************
* djinterp [c][tools]                                       cli_settings_tests.c
*
* The suite of the command-line kit's settings, declared in
* cli_settings_tests.h.
*   SHOWN is the program the cases run: `prog 1.2`, with `--level LEVEL` and
* `-o, --output FILE` on its root, and `show [WORD]...` with `--mode MODE`,
* whose work prints what the kit reads - `cli.plugin_strict_version`,
* `cli.plugin_search` (`default` for the knob's), `plugin_path` (`-` for
* none) - and the values the parser gave the options and the operands.
* Every case's program lets go of the run's settings first, so that each
* starts them as a run of its own would. HOOKED is SHOWN as the parser's
* hook is held to: response files and abbreviations read, a multi-letter
* short option `-ab` on its root and `pre`, a plugin command.
*
*
* path:      /tests/djinterp/tools/cli/c/cli_settings_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.27
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
#include "../../../../../inc/djinterp/c/option/option_env_registry.h"  // a djin
#include "../../../../../inc/djinterp/config/tools/cli/cfg_cli.h"  // the knobs
#include "../../../../../inc/djinterp/tools/cli/c/cli_plugin.h"  // dj_cli_plugin_versions
#include "../../../../../inc/djinterp/tools/cli/c/cli_program.h"  // the program
#include "../../../../../inc/djinterp/tools/cli/c/cli_settings.h"  // settings
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // specs
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
 *        own, `cli_settings_home_` and the C level under the current one,
 *        since each level's suite runs beside the others', holding a file
 *        of `_text`, or none for NULL.
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
        "PROG_PLUGIN_PATH",
        "DJINTERP_PROBE_SECRET",
        "PROG_PROBE_SECRET",
        "DJINTERP_CLI_SETTINGS_SECTIONS",
        "PROG_CLI_SETTINGS_SECTIONS",
        "DJINTERP_CLI_UNKNOWN_DJIN"
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

    return ( (D_TESTS_CLI_CHECK(d_tests_cli_settings_home(path, sizeof(path),
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
      "prog: error: --a.b: no djin is named 'a'\n"
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
      "prog: error: show: --x.y: no djin is named 'x'\n"
      "Try 'prog show --help' for more information.\n" },
    { "R07", "prog --set nosuch=1 show", 2, "",
      "prog: error: --set: no setting is named 'nosuch'\n"
      "Try 'prog --help' for more information.\n" },
#if (D_INTERNAL_CLI_PLUGIN_VARIABLE == 1)
    { "R08", "prog --cli.plugin_search show", 2, "",
      "prog: error: --cli.plugin_search: 'cli.plugin_search' is not a bool: "
      "give it a value, as --cli.plugin_search=VALUE\n"
      "Try 'prog --help' for more information.\n" },
    { "R09", "prog show --no-cli.plugin_search", 2, "",
      "prog: error: show: --no-cli.plugin_search: 'cli.plugin_search' is "
      "not a bool: give it a value, as --cli.plugin_search=VALUE\n"
      "Try 'prog show --help' for more information.\n" },
    { "R10", "prog --cli.plugin_strict_version --cli.plugin_search show", 2,
      "",
      "prog: error: --cli.plugin_search: 'cli.plugin_search' is not a bool: "
      "give it a value, as --cli.plugin_search=VALUE\n"
      "Try 'prog --help' for more information.\n" },
#endif
};

/*
d_tests_cli_settings_refused
  Tests the following:
  - a name that names no setting, with the nearest that does where one is
    near, from the root and from a subcommand
  - a value the setting does not take
  - `--set` with no NAME=VALUE after it, and with a word that is not one
  - a bare `--NAME` or `--no-NAME` on a setting that is not a bool, before
    the command and after it, and after a bare bool that is taken
  - each exits 2 and writes nothing to standard output
*/
bool d_tests_cli_settings_refused(void)
{
    char path[D_TESTS_CLI_SETTINGS_ROOM];

    d_tests_cli_settings_unset();

    return ( (D_TESTS_CLI_CHECK(d_tests_cli_settings_home(path, sizeof(path),
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
    held = D_TESTS_CLI_CHECK(d_tests_cli_settings_home(path, sizeof(path),
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
        held = D_TESTS_CLI_CHECK(
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
// 4a. THE SHARED FILE
//==============================================================================

// dj_tests_cli_settings_probe_storage, the probe's frame
//   a djin of the cases' own, `probe`, with one bool that refuses the user's
// file and the environment, as a setting that decides what code runs does.
D_OPTION_ENV_DECLARE(dj_tests_cli_settings_probe_storage, 4u, 2u, 256u);

static const struct dj_option_env_var D_TESTS_CLI_SETTINGS_SECRET[] =
{
    D_OPTION_ENV_ROW_REFUSING(D_OPTION_ENV_KIND_BOOL, "secret", NULL, 0u,
                              NULL, 0u, NULL, 0, 0, 1, 0.0, NULL, NULL,
                              "a bool the shared sources cannot set",
                              D_OPTION_ENV_REFUSES(D_OPTION_ENV_SOURCE_FILE) |
                              D_OPTION_ENV_REFUSES(
                                  D_OPTION_ENV_SOURCE_ENVIRONMENT))
};

static struct dj_option_env                        d_tests_cli_settings_probe;
static struct dj_tests_cli_settings_probe_storage d_tests_cli_settings_room;

/*
d_tests_cli_settings_make_probe
  The cases' program, with the probe's djin registered before it runs.
*/
static bool
d_tests_cli_settings_make_probe(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
)
{
    struct dj_option_env_registry* const registry = d_option_env_registry();

    if (d_option_env_registry_find(registry, "probe", 5u) != NULL)
    {
        (void)d_option_env_registry_remove(registry,
                                           &d_tests_cli_settings_probe);
    }

    return ( (d_tests_cli_settings_make(_program, _parser)) &&
             (D_OPTION_ENV_VIEW(&d_tests_cli_settings_probe,
                                d_tests_cli_settings_room).is_ok) &&
             (d_option_env_set_name(&d_tests_cli_settings_probe,
                                    "probe").is_ok) &&
             (d_option_env_declare_all(&d_tests_cli_settings_probe,
                                       D_TESTS_CLI_SETTINGS_SECRET,
                                       1u).is_ok) &&
             (d_option_env_registry_add(registry,
                                        &d_tests_cli_settings_probe).is_ok) );
}

// SHARED_TEXT
//   constant: a shared file's lines: a djin's not registered here, a near
// miss, a known djin's unknown setting, and a setting that refuses the file.
static const char SHARED_TEXT[] =
    "nosuch.depth = 3\n"
    "cli.plugin_serch = /x\n"
    "cli.nosuch = 1\n"
    "probe.secret = true\n";

// D_TESTS_CLI_SETTINGS_SHOW
//   constant: what `show` says with nothing bound.
#define D_TESTS_CLI_SETTINGS_SHOW                                              \
    "strict=1 search=default path=- level=- output=- mode=- words=-\n"

/*
d_tests_cli_settings_said
  The text of the shared file's notes, chosen by mask: bit 0 the foreign
line, 1 the near miss, 2 the known djin's, 3 the refusal; and the words
each is said with.
*/
static void
d_tests_cli_settings_said(
    char*       _buffer,
    size_t      _capacity,
    const char* _path,
    unsigned    _mask,
    const char* _unknown,
    const char* _refused
)
{
    size_t used = 0u;

    _buffer[0] = '\0';
    used += (_mask & 1u) ? (size_t)snprintf(_buffer + used, _capacity - used,
        "%s:1: %s: no djin is named 'nosuch'\n", _path, _unknown)
        : 0u;
    used += (_mask & 2u) ? (size_t)snprintf(_buffer + used, _capacity - used,
        "%s:2: %s: no setting is named 'cli.plugin_serch'; did you mean "
        "'cli.plugin_search'?\n", _path, _unknown) : 0u;
    used += (_mask & 4u) ? (size_t)snprintf(_buffer + used, _capacity - used,
        "%s:3: %s: no setting is named 'cli.nosuch'\n", _path, _unknown)
        : 0u;
    (void)((_mask & 8u) ? snprintf(_buffer + used, _capacity - used,
        "%s:4: %s: 'probe.secret' is not taken from the user's settings "
        "file\n", _path, _refused) : 0);

    return;
}

/*
d_tests_cli_settings_shared_environment
  The environment's cases of d_tests_cli_settings_shared, with no file: each
variable set alone, then unset again.
*/
static bool
d_tests_cli_settings_shared_environment(
    bool _held
)
{
    static const struct
    {
        const char*              name;
        const char*              value;
        struct dj_tests_cli_case run;
    } CASES[] =
    {
        { "DJINTERP_PROBE_SECRET", "true",
          { "V01", "prog --cli.shared_refusals=warn show", 0,
            D_TESTS_CLI_SETTINGS_SHOW,
            "prog: warning: DJINTERP_PROBE_SECRET: 'probe.secret' is not "
            "taken from the environment\n" } },
        { "DJINTERP_PROBE_SECRET", "true",
          { "V02", "prog show", 0, D_TESTS_CLI_SETTINGS_SHOW, "" } },
        { "PROG_PROBE_SECRET", "true",
          { "V03", "prog show", 0, D_TESTS_CLI_SETTINGS_SHOW,
            "prog: warning: PROG_PROBE_SECRET: 'probe.secret' is not taken "
            "from the environment\n" } },
        { "DJINTERP_CLI_PLUGIN_STRICT_VERSION", "maybe",
          { "V04", "prog show", 0, D_TESTS_CLI_SETTINGS_SHOW,
            "prog: warning: DJINTERP_CLI_PLUGIN_STRICT_VERSION: 'maybe' is "
            "not a value 'cli.plugin_strict_version' takes\n" } },
        { "DJINTERP_CLI_PLUGIN_STRICT_VERSION", "",
          { "V05", "prog show", 0, D_TESTS_CLI_SETTINGS_SHOW, "" } }
    };
    char path[D_TESTS_CLI_SETTINGS_ROOM];
    bool held = (d_tests_cli_settings_home(path, sizeof(path), NULL)) &&
                (_held);

    for (size_t i = 0u; i < sizeof(CASES) / sizeof(CASES[0]); ++i)
    {
        d_tests_cli_settings_unset();
        (void)d_tests_cli_setenv(CASES[i].name, CASES[i].value);
        held = (d_tests_cli_case_run(&CASES[i].run,
                                     d_tests_cli_settings_make_probe)) &&
               (held);
    }

    d_tests_cli_settings_unset();

    return held;
}

/*
d_tests_cli_settings_shared
  Tests the following:
  - by default a djin not registered here is quiet, and a near miss and a
    known djin's unknown setting warn (cli.unknown_settings typos)
  - all warns of every unknown line, none of none, error makes them errors
  - a setting that refuses the shared file is a note only at -v by default
    (cli.shared_refusals verbose), and a warning or an error as set
  - the command line's setting governs what the file's start held
  - from the environment: a refusal under DJINTERP_ is shared, one under
    the program's own prefix a warning; a value not taken, a warning; a
    variable set empty, nothing
*/
bool d_tests_cli_settings_shared(void)
{
    struct dj_tests_cli_case cases[7];
    char                     said[7][D_TESTS_CLI_SETTINGS_ROOM * 4u];
    char                     path[D_TESTS_CLI_SETTINGS_ROOM];
    static const char* const lines[7] =
    {
        "prog show",
        "prog --cli.unknown_settings=all show",
        "prog --cli.unknown_settings=none show",
        "prog --cli.unknown_settings=error show",
        "prog -v show",
        "prog --cli.shared_refusals=warn show",
        "prog --cli.shared_refusals=error show"
    };
    static const unsigned masks[7] = { 6u, 7u, 0u, 7u, 14u, 14u, 14u };
    static const char* const tags[7] =
        { "S01", "S02", "S03", "S04", "S05", "S06", "S07" };
    bool held;

    d_tests_cli_settings_unset();
    held = D_TESTS_CLI_CHECK(d_tests_cli_settings_home(path, sizeof(path),
                                                       SHARED_TEXT),
                             "S00",
                             "a scratch home holding the shared file");

    for (size_t i = 0u; i < 7u; ++i)
    {
        d_tests_cli_settings_said(said[i], sizeof(said[i]), path, masks[i],
                                  (i == 3u) ? "error" : "warning",
                                  (i == 4u) ? "note"
                                  : (i == 6u) ? "error" : "warning");
        cases[i].tag    = tags[i];
        cases[i].line   = lines[i];
        cases[i].status = ( (i == 3u) || (i == 6u) ) ? 2 : 0;
        cases[i].out    = ( (i == 3u) || (i == 6u) )
                          ? "" : D_TESTS_CLI_SETTINGS_SHOW;
        cases[i].err    = said[i];
    }

    held = (d_tests_cli_cases_run(cases, 7u,
                                  d_tests_cli_settings_make_probe)) && (held);

    return d_tests_cli_settings_shared_environment(held);
}

//==============================================================================
// 4b. THE FILE'S SECTIONS
//==============================================================================

// D_TESTS_CLI_SETTINGS_SEARCHED
//   constant: `show`'s search after SECTIONS_TEXT: the last line's, where
// the build takes `cli.plugin_search` from the file.
#if (D_INTERNAL_CLI_PLUGIN_VARIABLE == 1)
    #define D_TESTS_CLI_SETTINGS_SEARCHED "/every"
#else
    #define D_TESTS_CLI_SETTINGS_SEARCHED "default"
#endif

// SECTIONS_TEXT
//   constant: a user's file in sections: every program's line, another
// program's, the program's own under its name in capitals, a header the kit
// cannot read and a line under it, then every program's lines again.
static const char SECTIONS_TEXT[] =
    "cli.plugin_search = /shared\n"
    "[other]\n"
    "cli.plugin_strict_version = maybe\n"
    "[PROG]  # the cases' program\n"
    "cli.plugin_strict_version = false\n"
    "[bad header\n"
    "cli.nosuch = 1\n"
    "[*]\n"
    "cli.plugin_search = /every\n";

/*
d_tests_cli_settings_headers
  The section 1.4 queries, on text of their own: a header after a ';' and
a comment, one inside a statement passed over, `[*]`, a malformed one, the
text's end; whose a section is.
*/
static bool
d_tests_cli_settings_headers(void)
{
    static const char TEXT[]  = "a = 1 ; [x] # c\nb = \"[y]\"\n[*]\n";
    static const char WRONG[] = "[a b]\n";
    struct dj_cli_settings_header header;
    bool                          held;

    held = D_TESTS_CLI_CHECK(
               (d_cli_settings_next_header(TEXT, 0u, &header)) &&
               (header.at == 8u) && (header.end == 15u) &&
               (header.size == 1u) && (header.name[0] == 'x') &&
               (d_cli_settings_next_header(TEXT, header.end, &header)) &&
               (header.at == 26u) && (header.size == 1u) &&
               (header.name[0] == '*') &&
               (!d_cli_settings_next_header(TEXT, header.end, &header)) &&
               (header.at == sizeof(TEXT) - 1u) && (header.size == 0u),
               "N05",
               "each header where a statement could begin, and none past");
    held = D_TESTS_CLI_CHECK(
               (d_cli_settings_next_header(WRONG, 0u, &header)) &&
               (header.at == 0u) && (header.size == 0u) &&
               (!d_cli_settings_next_header(NULL, 0u, &header)),
               "N06",
               "a malformed header found, its size 0; none in no text") &&
           (held);
    held = D_TESTS_CLI_CHECK(
               (d_cli_settings_section_reads("PROG", 4u, "prog")) &&
               (d_cli_settings_section_reads("*", 1u, NULL)) &&
               (!d_cli_settings_section_reads("prog", 4u, NULL)) &&
               (!d_cli_settings_section_reads("pro", 3u, "prog")) &&
               (!d_cli_settings_section_reads("other", 5u, "prog")),
               "N07",
               "a section the program's by name, case aside, or `*`") &&
           (held);

    d_cli_settings_reset();

    return D_TESTS_CLI_CHECK(
               d_cli_settings_sectioned() ==
               (D_INTERNAL_CLI_SETTINGS_SECTIONS == 1),
               "N08",
               "before the settings start, the knob says whether") &&
           (held);
}

/*
d_tests_cli_settings_sections
  Tests the following:
  - in sections, whatever the knob: the program's own section read,
    another program's passed over, a header's name matched whatever its
    letters' case
  - a header the kit cannot read said by its line, the lines under it no
    program's; `[*]` every program's again
  - `cli.settings_sections` false, from either prefix: the file read as one
    part, each header said as such
  - the setting refused from the command line
  - the queries a program that writes the file finds the headers with
*/
bool d_tests_cli_settings_sections(void)
{
    struct dj_tests_cli_case cases[4];
    char                     said[2][D_TESTS_CLI_SETTINGS_ROOM * 8u];
    char                     path[D_TESTS_CLI_SETTINGS_ROOM];
    static const char* const prefix[2] =
        { "DJINTERP_CLI_SETTINGS_SECTIONS", "PROG_CLI_SETTINGS_SECTIONS" };
    bool                     held;

    d_tests_cli_settings_unset();
    held = D_TESTS_CLI_CHECK(d_tests_cli_settings_home(path, sizeof(path),
                                                       SECTIONS_TEXT),
                             "N00",
                             "a scratch home holding a file in sections");

    (void)snprintf(said[0], sizeof(said[0]),
                   "%s:6: warning: '[bad header' is not a section's header: "
                   "[PROGRAM] or [*]\n", path);
    (void)snprintf(said[1], sizeof(said[1]),
                   "%s:2: warning: '[other]' is a section's header, and this "
                   "run reads the file without sections "
                   "(cli.settings_sections)\n"
                   "%s:3: warning: 'maybe' is not a value "
                   "'cli.plugin_strict_version' takes\n"
                   "%s:4: warning: '[PROG]' is a section's header, and this "
                   "run reads the file without sections "
                   "(cli.settings_sections)\n"
                   "%s:6: warning: '[bad header' is not a statement: NAME = "
                   "VALUE\n"
                   "%s:7: warning: no setting is named 'cli.nosuch'\n"
                   "%s:8: warning: '[*]' is a section's header, and this run "
                   "reads the file without sections (cli.settings_sections)\n",
                   path, path, path, path, path, path);

    cases[0].tag    = "N01";
    cases[0].line   = "prog show";
    cases[0].status = 0;
    cases[0].out    = "strict=0 search=" D_TESTS_CLI_SETTINGS_SEARCHED
                      " path=- level=- output=- mode=- words=-\n";
    cases[0].err    = said[0];
    cases[1]        = cases[0];
    cases[1].tag    = "N02";
    cases[1].err    = said[1];
    cases[2]        = cases[1];
    cases[2].tag    = "N03";
    cases[3].tag    = "N04";
    cases[3].line   = "prog --cli.settings_sections=false show";
    cases[3].status = 2;
    cases[3].out    = "";
    cases[3].err    = "prog: error: --cli.settings_sections: "
                      "'cli.settings_sections' is not taken from the command "
                      "line\nTry 'prog --help' for more information.\n";

    // in sections whatever the knob, the environment being the one source
    (void)d_tests_cli_setenv(prefix[0], "true");
    held = (d_tests_cli_case_run(&cases[0],
                                 d_tests_cli_settings_make)) && (held);
    (void)d_tests_cli_setenv(prefix[0], NULL);

    for (size_t i = 0u; i < 2u; ++i)
    {
        (void)d_tests_cli_setenv(prefix[i], "false");
        held = (d_tests_cli_case_run(&cases[1u + i],
                                     d_tests_cli_settings_make)) && (held);
        (void)d_tests_cli_setenv(prefix[i], NULL);
    }

    held = (d_tests_cli_case_run(&cases[3],
                                 d_tests_cli_settings_make)) && (held);
    held = (d_tests_cli_settings_home(path, sizeof(path), NULL)) && (held);

    return d_tests_cli_settings_headers() && (held);
}


//==============================================================================
// 4c. AN UNKNOWN DJIN
//==============================================================================

// d_tests_cli_settings_net
//   constant: the root's one scope in the unknown djin's cases, a name no
// djin has.
static const char* const d_tests_cli_settings_net[1] = { "net" };

/*
d_tests_cli_settings_make_net
  The cases' program, the root frame given the scope `net`, which no djin
is named: a name whose first part the registry finds, but no frame.
*/
static bool
d_tests_cli_settings_make_net(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
)
{
    return ( (d_tests_cli_settings_make(_program, _parser)) &&
             (d_option_env_set_scopes(
                  d_option_env_registry_frame(d_option_env_registry()),
                  d_tests_cli_settings_net, 1u).is_ok) );
}

// UNKNOWN_TEXT
//   constant: a file naming the root's scope, and a djin nothing has.
static const char UNKNOWN_TEXT[] =
    "net.depth = 3\n"
    "nosuch.depth = 3\n";

/*
d_tests_cli_settings_unknown_djin
  Tests the following:
  - reason: a name whose first part is a root scope is a setting no one
    has, said as a near miss is; one whose first part nothing has is a
    djin not registered here, quiet
  - lookup: both first parts name no frame, so both are djins not
    registered here, quiet, and said as such at cli.unknown_settings all
  - from the environment, and on the command line, each way
  - d_cli_settings_foreign tells another program's name as the run's way
    says: the root's scope a name to know by reason, another's by lookup;
    a djin nothing has another's either way; a name without a `.`, or the
    kit's own, never another's
*/
bool d_tests_cli_settings_unknown_djin(void)
{
    struct dj_tests_cli_case cases[6];
    char                     said[2][D_TESTS_CLI_SETTINGS_ROOM * 4u];
    char                     path[D_TESTS_CLI_SETTINGS_ROOM];
    struct dj_option_env*    root;
    const char* const*       scopes;
    uint32_t                 scope_count;
    bool                     held;

    d_tests_cli_settings_unset();
    root        = d_option_env_registry_frame(d_option_env_registry());
    scopes      = root->scopes;
    scope_count = root->scope_count;
    held        = D_TESTS_CLI_CHECK(
                      d_tests_cli_settings_home(path, sizeof(path),
                                                UNKNOWN_TEXT),
                      "U00", "a scratch home naming an unknown djin");

    (void)snprintf(said[0], sizeof(said[0]),
                   "%s:1: warning: no setting is named 'net.depth'\n", path);
    (void)snprintf(said[1], sizeof(said[1]),
                   "%s:1: warning: no djin is named 'net'\n"
                   "%s:2: warning: no djin is named 'nosuch'\n", path, path);

    cases[0].tag    = "U01";
    cases[0].line   = "prog --cli.unknown_djin=reason show";
    cases[0].status = 0;
    cases[0].out    = D_TESTS_CLI_SETTINGS_SHOW;
    cases[0].err    = said[0];
    cases[1]        = cases[0];
    cases[1].tag    = "U02";
    cases[1].line   = "prog --cli.unknown_djin=lookup show";
    cases[1].err    = "";
    cases[2]        = cases[1];
    cases[2].tag    = "U03";
    cases[2].line   = "prog --cli.unknown_djin=lookup "
                      "--cli.unknown_settings=all show";
    cases[2].err    = said[1];
    cases[3]        = cases[2];
    cases[3].tag    = "U04";
    cases[3].line   = "prog --cli.unknown_settings=all show";
    held = (d_tests_cli_cases_run(cases, 1u,
                                  d_tests_cli_settings_make_net)) && (held);
    held = D_TESTS_CLI_CHECK(
               (!d_cli_settings_foreign("net.depth", 9u, "3")) &&
               (d_cli_settings_foreign("nosuch.depth", 12u, "3")) &&
               (!d_cli_settings_foreign("nodot", 5u, "3")) &&
               (!d_cli_settings_foreign("cli.plugin_search", 17u, "x")),
               "U07", "another program's name, by reason") && (held);
    held = (d_tests_cli_cases_run(cases + 1, 2u,
                                  d_tests_cli_settings_make_net)) && (held);
    held = D_TESTS_CLI_CHECK(
               (d_cli_settings_foreign("net.depth", 9u, "3")) &&
               (d_cli_settings_foreign("nosuch.depth", 12u, "3")) &&
               (!d_cli_settings_foreign("nodot", 5u, "3")) &&
               (!d_cli_settings_foreign("cli.plugin_search", 17u, "x")),
               "U08", "another program's name, by lookup") && (held);

    // the environment's way, under the command line's words
    (void)d_tests_cli_setenv("DJINTERP_CLI_UNKNOWN_DJIN", "lookup");
    held = (d_tests_cli_case_run(&cases[3],
                                 d_tests_cli_settings_make_net)) && (held);
    (void)d_tests_cli_setenv("DJINTERP_CLI_UNKNOWN_DJIN", NULL);

    // on the command line, a usage error either way
    held = (d_tests_cli_settings_home(path, sizeof(path), NULL)) && (held);
    cases[4].tag    = "U05";
    cases[4].line   = "prog --cli.unknown_djin=reason --net.depth=3 show";
    cases[4].status = 2;
    cases[4].out    = "";
    cases[4].err    = "prog: error: --net.depth: no setting is named "
                      "'net.depth'\nTry 'prog --help' for more "
                      "information.\n";
    cases[5]        = cases[4];
    cases[5].tag    = "U06";
    cases[5].line   = "prog --cli.unknown_djin=lookup --net.depth=3 show";
    cases[5].err    = "prog: error: --net.depth: no djin is named 'net'\n"
                      "Try 'prog --help' for more information.\n";
    held = (d_tests_cli_cases_run(cases + 4, 2u,
                                  d_tests_cli_settings_make_net)) && (held);

    (void)d_option_env_set_scopes(root, scopes, scope_count);

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

    held = D_TESTS_CLI_CHECK(
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
    held = D_TESTS_CLI_CHECK(
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
// 5a. A PRIVILEGED RUN
//==============================================================================

#if (D_INTERNAL_CLI_PLUGIN_VARIABLE == 1) &&                                   \
    (D_INTERNAL_CLI_PRIVILEGED_OPTION == 1)
/*
d_tests_cli_settings_make_trust
  The cases' program, which trusts its caller in a privileged run
(cli_plugin.h, D_CLI_PLUGIN_PRIVILEGED_TRUST).
*/
static bool
d_tests_cli_settings_make_trust(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
)
{
    return ( (d_tests_cli_settings_make(_program, _parser)) &&
             (d_cli_program_set_plugin_privileged(
                  _program, (int)D_CLI_PLUGIN_PRIVILEGED_TRUST)) );
}
#endif

/*
d_tests_cli_settings_privileged_runs
  The privileged run's cases of d_tests_cli_settings_privileged, the run
taken as privileged and the plugin settings set from the environment.
*/
static bool
d_tests_cli_settings_privileged_runs(void)
{
    static const struct dj_tests_cli_case CLOSED =
    {
        "P01", "prog show", 0, D_TESTS_CLI_SETTINGS_SHOW,
        "prog: warning: PROG_PLUGIN_PATH: 'plugin_path' is not taken from "
        "the environment\n"
    };
    static const struct dj_tests_cli_case TRUSTED =
    {
        "P02", "prog show", 0,
        "strict=1 search=/s path=/env level=- output=- mode=- words=-\n",
        "prog: warning: running with privileges its caller lacks, and "
        "taking the plugin settings from the caller, as the program "
        "says\n"
    };
    static const struct dj_tests_cli_case REFUSED =
    {
        "P03", "prog --no-cli.privileged_refuse show", 2, "",
        "prog: error: --no-cli.privileged_refuse: 'cli.privileged_refuse' "
        "is not taken from the command line\nTry 'prog --help' for more "
        "information.\n"
    };
    static const struct dj_tests_cli_case BUILT =
    {
        "P01", "prog show", 0,
        "strict=1 search=/s path=/env level=- output=- mode=- words=-\n",
        "prog: warning: running with privileges its caller lacks, and "
        "taking the plugin settings from the caller, as the build says "
        "(D_CFG_CLI_PRIVILEGED_REFUSE)\n"
    };
    bool held = true;

    d_tests_cli_settings_unset();
    (void)d_tests_cli_setenv("PROG_PLUGIN_PATH", "/env");
    (void)d_tests_cli_setenv("DJINTERP_CLI_PLUGIN_SEARCH", "/s");

    // refused by default; trusted where the build says so
#if (D_INTERNAL_CLI_PLUGIN_VARIABLE == 1) &&                                   \
    (D_INTERNAL_CLI_PRIVILEGED_REFUSE == 1)
    held = (d_tests_cli_case_run(&CLOSED,
                                 d_tests_cli_settings_make)) && (held);
    (void)BUILT;
#elif (D_INTERNAL_CLI_PLUGIN_VARIABLE == 1)
    held = (d_tests_cli_case_run(&BUILT,
                                 d_tests_cli_settings_make)) && (held);
    (void)CLOSED;
#else
    (void)CLOSED;
    (void)BUILT;
#endif

#if (D_INTERNAL_CLI_PLUGIN_VARIABLE == 1) &&                                   \
    (D_INTERNAL_CLI_PRIVILEGED_OPTION == 1)
    held = (d_tests_cli_case_run(&TRUSTED,
                                 d_tests_cli_settings_make_trust)) && (held);
    held = (d_tests_cli_case_run(&REFUSED,
                                 d_tests_cli_settings_make)) && (held);
#else
    (void)TRUSTED;
    (void)REFUSED;
#endif

    d_tests_cli_settings_unset();

    return held;
}

/*
d_tests_cli_settings_privileged
  Tests the following:
  - a privileged run refuses the plugin settings from the caller's sources,
    by default (D_CFG_CLI_PRIVILEGED_REFUSE), a refusal under the
    program's own prefix said; trusts the caller where the build says so,
    and says that
  - a program that trusts its caller is taken at its word, and that said
  - `cli.privileged_refuse` is refused from the command line then; taken
    in a run that is not privileged
  - the program's say comes too late once the djin is made
*/
bool d_tests_cli_settings_privileged(void)
{
    static const struct dj_tests_cli_case PLAIN =
    {
        "P04", "prog --no-cli.privileged_refuse show", 0,
        D_TESTS_CLI_SETTINGS_SHOW, ""
    };
    char path[D_TESTS_CLI_SETTINGS_ROOM];
    bool held = d_tests_cli_settings_home(path, sizeof(path), NULL);

    d_cli_settings_pretend_privileged(1);
    held = (d_tests_cli_settings_privileged_runs()) && (held);

    // once the djin is made, the say is too late, and the run's own
    d_cli_settings_reset();
    held = D_TESTS_CLI_CHECK(
               (d_cli_settings_djin() != NULL) &&
               (!d_cli_settings_privileged(
                    (int)D_CLI_PLUGIN_PRIVILEGED_TRUST)) &&
               (d_cli_settings_closed() ==
                ( (D_INTERNAL_CLI_PLUGIN_VARIABLE == 0) ||
                  (D_INTERNAL_CLI_PRIVILEGED_REFUSE == 1) )),
               "P05",
               "the program's say too late once the djin is made") &&
           (held);

    d_cli_settings_pretend_privileged(0);

#if (D_INTERNAL_CLI_PRIVILEGED_OPTION == 1)
    held = (d_tests_cli_case_run(&PLAIN,
                                 d_tests_cli_settings_make)) && (held);
#else
    (void)PLAIN;
#endif

    d_cli_settings_pretend_privileged(-1);
    d_cli_settings_reset();

    return held;
}


//==============================================================================
// 6.  THE PARSER'S HOOK
//==============================================================================

/**
 * @brief HOOKED's `show`: what the kit reads, and what the parser gave
 *        `--output`, `-ab` and the operands.
 *
 * @param[in] _program  the program.
 * @param[in] _command  `show`.
 * @param[in] _context  unused.
 * @return D_CLI_EXIT_OK.
 */
static
int d_tests_cli_settings_work_hooked(
    struct dj_cli_program* _program,
    struct dj_arg_command* _command,
    void*                  _context
)
{
    char output[64];
    char ab[64];
    char words[128];

    (void)_context;

    d_tests_cli_print(
        _program,
        "strict=%d output=%s ab=%s words=%s\n",
        d_cli_settings_plugin_strict_version(_program->plugin_versions)
            ? 1 : 0,
        d_tests_cli_values(_command, "--output", output, sizeof(output)),
        d_tests_cli_values(_command, "-ab", ab, sizeof(ab)),
        d_tests_cli_values(_command, "WORD", words, sizeof(words)));

    return D_CLI_EXIT_OK;
}

/**
 * @brief HOOKED: `prog 1.2`, reading response files and abbreviations, with
 *        `-o, --output` and a multi-letter short option `-ab` on its root,
 *        `show [WORD]...`, and `pre`, a plugin command: the four places the
 *        kit's own walk read otherwise than the parser before the hook.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser.
 * @return whether it was made.
 */
static
bool d_tests_cli_settings_make_hooked(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
)
{
    d_cli_settings_reset();

    if (d_cli_program_init(_program,
                           _parser,
                           "prog",
                           "1.2",
                           D_CLI_PROGRAM_COMMANDS |
                           D_CLI_PROGRAM_RESPONSE_FILES |
                           D_CLI_PROGRAM_ABBREVIATIONS) != D_ARG_OK)
    {
        return false;
    }

    struct dj_arg_command* const root = &_parser->command;
    struct dj_arg_command* const show = d_cli_program_add_command(
                                           _program,
                                           NULL,
                                           "show",
                                           "say what the kit reads",
                                           d_tests_cli_settings_work_hooked,
                                           NULL,
                                           NULL);

    root->flags |= D_ARG_COMMAND_NON_STANDARD_NAMES;

    return ( (show != NULL) &&
             (d_cli_program_add_plugin(_program, NULL, "pre",
                                       "a plugin command") != NULL) &&
             (d_arg_command_add_option(root, "-o,--output",
                                       "a file") != NULL) &&
             (d_arg_command_add_option(root, "-ab",
                                       "a multi-letter short option") !=
              NULL) &&
             (d_arg_command_add_option(show, "WORD",
                                       "words") != NULL) );
}

// HOOKED
//   constant: the cases the parser's hook reads as the parser does, where
// the kit's own walk read them otherwise: an abbreviated option's value, a
// multi-letter short option's, and a setting after either.
static const struct dj_tests_cli_case HOOKED[] =
{
    { "H02", "prog --out --cli.plugin_strict_version=false show", 0,
      "strict=1 output=--cli.plugin_strict_version=false ab=- words=-\n",
      "" },
    { "H03", "prog -ab --no-cli.plugin_strict_version show", 0,
      "strict=1 output=- ab=--no-cli.plugin_strict_version words=-\n",
      "" },
    { "H04", "prog --out=x --cli.plugin_strict_version=false show", 0,
      "strict=0 output=x ab=- words=-\n",
      "" },
    { "H05", "prog -ab y show w --set cli.plugin_strict_version=0", 0,
      "strict=0 output=- ab=y words=w\n",
      "" },
    { "H06", "prog --outp=x --a.b=1 show", 2, "",
      "prog: error: --a.b: no djin is named 'a'\n"
      "Try 'prog --help' for more information.\n" }
};

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

/**
 * @brief A response file's setting, as if typed: `prog @FILE show`, the
 *        file in the current directory, named by the C level, as each
 *        level's suite runs beside the others'.
 *
 * @return whether it held.
 */
static
bool d_tests_cli_settings_hook_response(void)
{
    char                    folder[D_TESTS_CLI_SETTINGS_ROOM];
    char                    path[D_TESTS_CLI_SETTINGS_ROOM + 32u];
    char                    line[D_TESTS_CLI_SETTINGS_ROOM + 64u];
    struct dj_tests_cli_case each =
    {
        "H01", NULL, 0,
        "strict=0 output=- ab=- words=a\n",
        ""
    };
    bool                    held;

    held = (d_dir_get_current(folder, sizeof(folder)) != NULL);

    if (held)
    {
        static const char TEXT[] = "--cli.plugin_strict_version=false\n";

        (void)snprintf(path, sizeof(path), "%s/cli_settings_rf_%ld", folder,
                       (long)__STDC_VERSION__);
        (void)snprintf(line, sizeof(line), "prog '@%s' show a", path);
        held = d_tests_cli_file_write(path, TEXT, sizeof(TEXT) - 1u);
    }

    if (!D_TESTS_CLI_CHECK(held, "H01", "a response file to read"))
    {
        return false;
    }

    each.line = line;
    held      = d_tests_cli_case_run(&each, d_tests_cli_settings_make_hooked);
    (void)remove(path);

    return held;
}

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

/**
 * @brief A visit of a command's unused arguments: each appended to the text
 *        `_context` holds, after a space, as far as it has room.
 */
static
int d_tests_cli_settings_kept(
    const struct dj_arg_extra* _extra,
    void*                      _context
)
{
    char* const  text = (char*)_context;
    const size_t used = strlen(text);

    (void)snprintf(text + used, D_TESTS_CLI_SETTINGS_ROOM - used, "%s%.*s",
                   (used > 0u) ? " " : "", (int)_extra->text.length,
                   _extra->text.data);

    return D_ARG_OK;
}

/**
 * @brief A setting after a plugin command's name is its module's, never
 *        taken by the program that loads it: `prog pre --x.y=1
 *        --cli.plugin_strict_version=false` parses, binds nothing, and
 *        leaves both to `pre`.
 *
 * @return whether it held.
 */
static
bool d_tests_cli_settings_hook_plugin(void)
{
    static const char* const LINE[] =
    {
        "prog", "pre", "--x.y=1", "--cli.plugin_strict_version=false", NULL
    };
    struct dj_cli_program     program;
    struct dj_arg_parser      parser;
    struct dj_arg_buffer      out  = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer      err  = D_ARG_BUFFER_INIT;
    int                       exit = -1;
    bool                      held;

    held = d_tests_cli_start(&program, &parser,
                             d_tests_cli_settings_make_hooked, &out, &err);

    if (held)
    {
        struct dj_arg_command* const pre =
            d_arg_command_subcommand(&parser.command, "pre");
        char                         kept[D_TESTS_CLI_SETTINGS_ROOM] = "";

        held = (!d_cli_program_parse(&program, 4, LINE, &exit)) &&
               (pre != NULL) &&
               (d_arg_command_each_remaining(pre, false,
                                             d_tests_cli_settings_kept,
                                             kept) == D_ARG_OK) &&
               (strcmp(kept, "--x.y=1 --cli.plugin_strict_version=false") ==
                0) &&
               (d_cli_settings_plugin_strict_version(
                    (int)D_CLI_PLUGIN_VERSIONS_DEFAULT) ==
                (D_INTERNAL_CLI_PLUGIN_STRICT_VERSION == 1)) &&
               (err.length == 0u);
    }

    d_tests_cli_finish(&program, &parser);
    d_arg_buffer_release(&out);
    d_arg_buffer_release(&err);

    return D_TESTS_CLI_CHECK(held, "H07",
                             "a plugin command keeps the settings after "
                             "its name, the host binding none");
}

// d_tests_cli_settings_counted
//   variable: how many arguments the program's own hook was offered.
static int d_tests_cli_settings_counted;

/**
 * @brief A program's own hook, which counts what it is offered and hands
 *        each argument on to the kit's.
 */
static
int d_tests_cli_settings_own_hook(
    void*        _ctx,
    int          _argc,
    char* const* _argv,
    int*         _at,
    int32_t*     _status
)
{
    ++d_tests_cli_settings_counted;

    return d_cli_settings_hook(_ctx, _argc, _argv, _at, _status);
}

/**
 * @brief A program that sets a hook of its own keeps it: it is offered
 *        each long option the parse does not know, and the kit's takes
 *        what it hands on.
 *
 * @return whether it held.
 */
static
bool d_tests_cli_settings_hook_own(void)
{
    static const char* const LINE[] =
    {
        "prog", "--cli.plugin_strict_version=false", "show", "--a.b=1", NULL
    };
    struct dj_cli_program     program;
    struct dj_arg_parser      parser;
    struct dj_arg_buffer      out  = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer      err  = D_ARG_BUFFER_INIT;
    int                       exit = -1;
    bool                      held;

    d_tests_cli_settings_counted = 0;
    held = d_tests_cli_start(&program, &parser,
                             d_tests_cli_settings_make_hooked, &out, &err);

    if (held)
    {
        d_arg_parser_set_setting_hook(&parser, d_tests_cli_settings_own_hook,
                                      NULL);
        held = (d_cli_program_parse(&program, 4, LINE, &exit)) &&
               (exit == D_CLI_EXIT_TROUBLE) &&
               (d_tests_cli_settings_counted == 2) &&
               (parser.setting_hook == d_tests_cli_settings_own_hook) &&
               (d_cli_settings_plugin_strict_version(
                    (int)D_CLI_PLUGIN_VERSIONS_DEFAULT) == false);
    }

    d_tests_cli_finish(&program, &parser);
    held = (d_tests_cli_same("H08", "err",
                             "prog: error: show: --a.b: no djin is named "
                             "'a'\n"
                             "Try 'prog show --help' for more "
                             "information.\n",
                             err.data, err.length)) &&
           (held);
    d_arg_buffer_release(&out);
    d_arg_buffer_release(&err);

    return D_TESTS_CLI_CHECK(held, "H08",
                             "a program's own hook kept, the kit's called "
                             "from it");
}

/*
d_tests_cli_settings_hooked
  Tests the following:
  - a response file's setting is taken, as if typed
  - an abbreviated option's value, and a multi-letter short option's, are
    kept for it, whatever they look like; a setting after either is taken
  - a name the parse would abbreviate is not offered as a setting, and a
    setting it names badly is refused by the option as typed
  - a setting after a plugin command's name is left to the command
  - a program's own hook is kept, and the kit's takes what it hands on
*/
bool d_tests_cli_settings_hooked(void)
{
    char path[D_TESTS_CLI_SETTINGS_ROOM];
    bool held;

    d_tests_cli_settings_unset();

    held = D_TESTS_CLI_CHECK(d_tests_cli_settings_home(path, sizeof(path),
                                                       NULL),
                             "H00",
                             "a scratch home for the user's settings");
#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)
    // a response file's setting, where the parser reads response files
    held = (held) &&
           (d_tests_cli_settings_hook_response());
#endif
    held = (d_tests_cli_cases_run(HOOKED,
                                  D_TESTS_CLI_SETTINGS_COUNT(HOOKED),
                                  d_tests_cli_settings_make_hooked)) &&
           (held);
    held = (d_tests_cli_settings_hook_plugin()) &&
           (held);
    held = (d_tests_cli_settings_hook_own()) &&
           (held);

    return held;
}


//==============================================================================
// THE SUITE
//==============================================================================

// d_tests_cli_settings
//   constant: the suite's sections, in the order they run.
const struct dj_test_case_spec d_tests_cli_settings[] =
{
    { "cli_settings_taken",
      "settings taken from the command line, either side",
      d_tests_cli_settings_taken },
    { "cli_settings_refused",
      "unknown names, bad values and bad --set refused",
      d_tests_cli_settings_refused },
    { "cli_settings_file",
      "the user's file binds; its failures said by line",
      d_tests_cli_settings_file },
    { "cli_settings_environment",
      "the environment binds; the command line wins",
      d_tests_cli_settings_environment },
    { "cli_settings_shared",
      "what the shared file and environment are said for",
      d_tests_cli_settings_shared },
    { "cli_settings_sections",
      "the user's file in sections, and read as one part",
      d_tests_cli_settings_sections },
    { "cli_settings_unknown_djin",
      "an unknown djin told by reason and by lookup",
      d_tests_cli_settings_unknown_djin },
    { "cli_settings_reads",
      "each setting's knob, the program's say, the sources",
      d_tests_cli_settings_reads },
    { "cli_settings_privileged",
      "a privileged run: the caller's sources refused, or trusted",
      d_tests_cli_settings_privileged },
    { "cli_settings_hooked",
      "settings as the parser reads them, through its hook",
      d_tests_cli_settings_hooked }
};

// d_tests_cli_settings_block
//   constant: d_tests_cli_settings as a block of the C face's spec form.
const struct dj_test_block_spec d_tests_cli_settings_block =
{
    "settings",
    "the settings the kit keeps (cli_settings.c)",
    d_tests_cli_settings,
    sizeof(d_tests_cli_settings) / sizeof(d_tests_cli_settings[0])
};


#else  // !D_TESTS_CLI_SETTINGS_ON

// without the option core the kit keeps no settings, and outside testing a
// run's cannot be let go of: the suite has no sections; a table holds one
// entry, never run
const struct dj_test_case_spec d_tests_cli_settings[1] =
    { { NULL, NULL, NULL } };

// d_tests_cli_settings_block
//   constant: d_tests_cli_settings as a block of the C face's spec form:
// empty.
const struct dj_test_block_spec d_tests_cli_settings_block =
{
    "settings",
    "the settings the kit keeps (cli_settings.c): none here",
    d_tests_cli_settings,
    0u
};

#endif  // D_TESTS_CLI_SETTINGS_ON
