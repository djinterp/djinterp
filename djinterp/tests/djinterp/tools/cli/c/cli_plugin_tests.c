/*******************************************************************************
* djinterp [tools]                                            cli_plugin_tests.c
*
* The suite of the command-line kit's plugins, declared in
* cli_plugin_tests.h.
*   Finding is tested in a scratch directory of empty files named as modules
* are, by paths relative to it, so that what was searched is the same text
* everywhere. Loading and running take the probes from the modules'
* directory, by its path; a message that names a module's file is compared
* with that path in it. HOST is the program that runs them: `prog 1.2`, a
* plugin command for each probe, and `missing`, whose module was never
* built. KIT writes to the process's standard streams, which a case
* redirects to files for the run. What `--version --verbose` says is
* composed where it names a file -- the runner's, a module's -- or the kit's
* own build.
*
*
* path:      /tests/djinterp/tools/cli/c/cli_plugin_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.27
*******************************************************************************/
#include "./cli_plugin_tests.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdio.h>    // printf, snprintf, remove
#include <stdlib.h>   // free
#include <string.h>   // memset, strcmp, strlen, strncmp, strrchr, strstr
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/c/filesys/file_dir.h"  // d_path_resolve
#include "../../../../../inc/djinterp/c/filesys/file_stat.h"  // d_file_is_regular
#include "../../../../../inc/djinterp/env/tools/cli/env_cli.h"  // D_ENV_CLI_*
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_common.h"  // buffers
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_parser.h"  // the parser
#include "../../../../../inc/djinterp/tools/cli/c/cli_io.h"  // d_cli_free
#include "../../../../../inc/djinterp/tools/cli/c/cli_plugin.h"  // the plugin interface
#include "../../../../../inc/djinterp/tools/cli/c/cli_program.h"  // the program
#include "../../../c/modules/parse/cli/arg_parser/arg_tests_files.h"  // scratch directories
#include "../../../c/modules/parse/tests_section.h"  // dj_tests_section
#include "./cli_tests_support.h"  // checks, the environment, the streams
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// D_TESTS_CLI_PATH_MAX
//   constant: the longest path a case builds.
#define D_TESTS_CLI_PATH_MAX  1024

// D_TESTS_CLI_SEPARATOR
//   constant: what separates the directories of a list, as a string.
#if (D_ENV_CLI_PATH_LIST_SEPARATOR == ';')
    #define D_TESTS_CLI_SEPARATOR  ";"
#else
    #define D_TESTS_CLI_SEPARATOR  ":"
#endif

// D_TESTS_CLI_SLASH
//   constant: what the kit joins a directory and a name with, as a string:
// D_ENV_CLI_PATH_SEPARATOR.
#if (D_ENV_CLI_PATH_SEPARATOR == '\\')
    #define D_TESTS_CLI_SLASH  "\\"
#else
    #define D_TESTS_CLI_SLASH  "/"
#endif

// D_TESTS_CLI_MODULE
//   macro: a module's file name: `NAME` and the platform's suffix.
#define D_TESTS_CLI_MODULE(_name)  _name D_ENV_CLI_MODULE_SUFFIX

// d_tests_cli_modules
//   variable: the directory the probe modules are in, as the runner said.
static char d_tests_cli_modules[D_TESTS_CLI_PATH_MAX];

/*
d_tests_cli_plugin_modules
  The directory given, made absolute while the working directory is still
the one it was given from -- the sections move into scratch directories,
and under Wine a path such as /tmp/x names the working directory's drive;
or the runner's own, its file's name cut off.
*/
void d_tests_cli_plugin_modules(
    const char* _directory
)
{
    char* executable = NULL;

    // the directory the runner was given
    if (_directory != NULL)
    {
        char* const whole = d_path_resolve(_directory,
                                           NULL,
                                           0u);

        (void)snprintf(d_tests_cli_modules,
                       sizeof(d_tests_cli_modules),
                       "%s",
                       (whole != NULL) ? whole : _directory);
        d_cli_free(whole);

        return;
    }

    // the runner's own
    if (d_cli_executable_path(NULL,
                              &executable) == 0)
    {
        char* const slash = strrchr(executable,
                                    '/');
        char* const back  = strrchr(executable,
                                    '\\');
        char* const last  = ( (back != NULL) &&
                              ( (slash == NULL) ||
                                (back > slash) ) ) ? back : slash;

        if (last != NULL)
        {
            *last = '\0';
        }

        (void)snprintf(d_tests_cli_modules,
                       sizeof(d_tests_cli_modules),
                       "%s",
                       executable);
    }

    d_cli_free(executable);

    return;
}

/**
 * @brief The path of a probe module, as the kit joins it: the modules'
 *        directory, the separator, `prog-KIND` and the platform's suffix.
 *
 * @param[in]  _kind  the probe's kind: `probe`, `other`.
 * @param[out] _path  receives the path.
 * @param[in]  _size  the room in `_path`.
 * @return `_path`.
 */
static
const char* d_tests_cli_probe_path(
    const char* _kind,
    char*       _path,
    size_t      _size
)
{
    (void)snprintf(_path,
                   _size,
                   "%s" D_TESTS_CLI_SLASH "prog-%s%s",
                   d_tests_cli_modules,
                   _kind,
                   D_ENV_CLI_MODULE_SUFFIX);

    return _path;
}

/**
 * @brief Whether a buffer holds the text given, whole.
 *
 * @param[in] _buffer  the buffer.
 * @param[in] _text    the text.
 * @return `true` if it does.
 */
static
bool d_tests_cli_holds(
    const struct dj_arg_buffer* _buffer,
    const char*                 _text
)
{
    const size_t length = strlen(_text);

    return ( (_buffer->length == length) &&
             ( (length == 0u) ||
               (memcmp(_buffer->data,
                       _text,
                       length) == 0) ) );
}


//==============================================================================
// 1.  THE PROGRAM'S FILE
//==============================================================================

/*
d_tests_cli_plugin_executable
  The runner's file: there, a regular file, named by an absolute path;
EINVAL when there is nowhere to put it.
*/
bool d_tests_cli_plugin_executable(void)
{
    char*      path   = NULL;
    const int  status = d_cli_executable_path("ignored",
                                              &path);
    const bool whole  = ( (status == 0)    &&
                          (path != NULL)   &&
                          ( (path[0] == '/') ||
                            ( (path[0] != '\0') &&
                              (path[1] == ':') ) ||
                            (path[0] == '\\') ) );
    bool       held   = d_tests_cli_check( (whole) &&
                                           (d_file_is_regular(path) != 0),
                                           "X01",
                                           "the runner's own file, by an "
                                           "absolute path");

    d_cli_free(path);
    held = d_tests_cli_check(d_cli_executable_path(NULL,
                                                   NULL) != 0,
                             "X02",
                             "nowhere to put the path") && held;

    return held;
}


//==============================================================================
// 2.  FINDING
//==============================================================================

/**
 * @brief Finds a module of a program's command and compares where it was
 *        found and where it was looked for.
 *
 * @param[in] _tag         the case's tag.
 * @param[in] _program     the program.
 * @param[in] _list        PROGRAM_PLUGIN_PATH's value, or NULL to unset it.
 * @param[in] _executable  the program's file, or NULL.
 * @param[in] _found       where it must be found; NULL for nowhere.
 * @param[in] _searched    the directories it must have looked in, a line
 *                         each.
 * @return whether it held.
 */
static
bool d_tests_cli_find_case(
    const char* _tag,
    const char* _program,
    const char* _list,
    const char* _executable,
    const char* _found,
    const char* _searched
)
{
    struct dj_arg_buffer found    = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer searched = D_ARG_BUFFER_INIT;
    char                variable[64];

    (void)snprintf(variable,
                   sizeof(variable),
                   "%s",
                   (strcmp(_program,
                           "prog") == 0)
                   ? "PROG_PLUGIN_PATH"
                   : "MY_TOOL_PLUGIN_PATH");
    (void)d_tests_cli_setenv(variable,
                             _list);

    const int  status = d_cli_plugin_find(_program,
                                          "gui",
                                          _executable,
                                          &found,
                                          &searched);
    const bool held   = ( (_found != NULL)
                          ? ( (status == D_ARG_OK) &&
                              (d_tests_cli_holds(&found,
                                                 _found)) )
                          : ( (status == D_ARG_ERROR_FILE) &&
                              (found.length == 0u) ) );

    (void)d_tests_cli_setenv(variable,
                             NULL);

    const bool same = d_tests_cli_same(_tag,
                                       "searched",
                                       _searched,
                                       searched.data,
                                       searched.length);

    d_arg_buffer_release(&found);
    d_arg_buffer_release(&searched);

    return d_tests_cli_check(held,
                             _tag,
                             "where the module was found") && same;
}

/**
 * @brief Makes the scratch directory's modules: plain and tagged, in the
 *        list's directories, beside a program and in ../lib/prog, a
 *        directory named as a module, and another program's.
 *
 * @param[in,out] _files  the scratch directory.
 * @return whether everything was made.
 */
static
bool d_tests_cli_find_files(
    struct dj_tests_arg_files* _files
)
{
    static const char* const folders[] =
    {
        "one", "two", "three", "empty", "bin", "lib", "lib/prog",
        "four", "three/" D_TESTS_CLI_MODULE("prog-gui")
    };
    static const char* const modules[] =
    {
        "one/" D_TESTS_CLI_MODULE("prog-gui-linux64"),
        "one/" D_TESTS_CLI_MODULE("prog-gui-win64"),
        "two/" D_TESTS_CLI_MODULE("prog-gui"),
        "lib/prog/" D_TESTS_CLI_MODULE("prog-gui"),
        "bin/" D_TESTS_CLI_MODULE("prog-tool"),
        "four/" D_TESTS_CLI_MODULE("my-tool-gui")
    };
    bool made = true;

    for (size_t index = 0u;
         index < (sizeof(folders) / sizeof(folders[0]));
         ++index)
    {
        made = d_tests_arg_files_folder(_files,
                                        folders[index]) && made;
    }

    for (size_t index = 0u;
         index < (sizeof(modules) / sizeof(modules[0]));
         ++index)
    {
        made = d_tests_arg_files_file(_files,
                                      modules[index],
                                      "not a module") && made;
    }

    return made;
}

/**
 * @brief The cases of a list: its order, its empty entries, a directory
 *        ending in its separator, a name that is a directory, and the
 *        variable a program named with `-` reads.
 *
 * @return whether every case held.
 */
static
bool d_tests_cli_find_lists(void)
{
    bool held = d_tests_cli_find_case(
                    "F01",
                    "prog",
                    "one" D_TESTS_CLI_SEPARATOR "two",
                    NULL,
                    "two" D_TESTS_CLI_SLASH D_TESTS_CLI_MODULE("prog-gui"),
                    "one\ntwo\n");

    held = d_tests_cli_find_case("F02",
                                 "prog",
                                 D_TESTS_CLI_SEPARATOR
                                 D_TESTS_CLI_SEPARATOR "two"
                                 D_TESTS_CLI_SEPARATOR,
                                 NULL,
                                 "two" D_TESTS_CLI_SLASH
                                 D_TESTS_CLI_MODULE("prog-gui"),
                                 "two\n") && held;
    held = d_tests_cli_find_case("F03",
                                 "prog",
                                 "two/",
                                 NULL,
                                 "two/" D_TESTS_CLI_MODULE("prog-gui"),
                                 "two/\n") && held;
    held = d_tests_cli_find_case("F04",
                                 "prog",
                                 "three" D_TESTS_CLI_SEPARATOR "empty",
                                 NULL,
                                 NULL,
                                 "three\nempty\n") && held;
    held = d_tests_cli_find_case("F05",
                                 "my-tool",
                                 "four",
                                 NULL,
                                 "four" D_TESTS_CLI_SLASH
                                 D_TESTS_CLI_MODULE("my-tool-gui"),
                                 "four\n") && held;

    return held;
}

/**
 * @brief The cases of the program's own file: its tag, its directory and
 *        ../lib/PROGRAM from it, a file named without a directory, `.exe`,
 *        which only Windows leaves out of the tag, and a program at the
 *        root, `/` or `C:\`, whose directory keeps its separator.
 *
 * @return whether every case held.
 */
static
bool d_tests_cli_find_beside(void)
{
    bool held = d_tests_cli_find_case(
                    "F06",
                    "prog",
                    "one" D_TESTS_CLI_SEPARATOR "two",
                    "bin/prog-linux64",
                    "one" D_TESTS_CLI_SLASH
                    D_TESTS_CLI_MODULE("prog-gui-linux64"),
                    "one\n");

    held = d_tests_cli_find_case("F07",
                                 "prog",
                                 NULL,
                                 "bin/prog",
                                 "bin" D_TESTS_CLI_SLASH ".."
                                 D_TESTS_CLI_SLASH "lib" D_TESTS_CLI_SLASH
                                 "prog" D_TESTS_CLI_SLASH
                                 D_TESTS_CLI_MODULE("prog-gui"),
                                 "bin\nbin" D_TESTS_CLI_SLASH ".."
                                 D_TESTS_CLI_SLASH "lib" D_TESTS_CLI_SLASH
                                 "prog\n") && held;
    held = d_tests_cli_find_case("F08",
                                 "prog",
                                 NULL,
                                 "prog",
                                 NULL,
                                 "") && held;
#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    held = d_tests_cli_find_case("F09",
                                 "prog",
                                 "one",
                                 "bin\\prog-win64.EXE",
                                 "one" D_TESTS_CLI_SLASH
                                 D_TESTS_CLI_MODULE("prog-gui-win64"),
                                 "one\n") && held;
    held = d_tests_cli_find_case("F10",
                                 "prog",
                                 NULL,
                                 "C:\\prog.exe",
                                 NULL,
                                 "C:\\\nC:\\..\\lib\\prog\n") && held;
#else
    held = d_tests_cli_find_case("F09",
                                 "prog",
                                 "one",
                                 "prog-win64.exe",
                                 NULL,
                                 "one\n") && held;
    held = d_tests_cli_find_case("F10",
                                 "prog",
                                 NULL,
                                 "/prog",
                                 NULL,
                                 "/\n/../lib/prog\n") && held;
#endif

    return held;
}

/*
d_tests_cli_plugin_finding
  The scratch directory's modules, found by the list, then by the program's
own file.
*/
bool d_tests_cli_plugin_finding(void)
{
    struct dj_tests_arg_files files;
    bool                     held = d_tests_arg_files_enter(&files);

    // a scratch directory of modules
    if (!d_tests_cli_check( (held) &&
                            (d_tests_cli_find_files(&files)),
                            "F01",
                            "the modules to find could not be made"))
    {
        d_tests_arg_files_leave(&files);

        return false;
    }

    held = d_tests_cli_find_lists();
    held = d_tests_cli_find_beside() && held;
    d_tests_arg_files_leave(&files);

    return held;
}


//==============================================================================
// 3.  LOADING
//==============================================================================

/**
 * @brief Loads a probe and checks what came of it: the module and its
 *        record when it was taken; why, and no module, when it was not.
 *
 * @param[in] _tag      the case's tag.
 * @param[in] _path     the module's file.
 * @param[in] _version  the version it must belong to, or NULL for any.
 * @param[in] _status   the status the load must give.
 * @param[in] _why      why it must say it was refused; NULL for whatever
 *                      the system said, as long as it said something.
 * @return whether it held.
 */
static
bool d_tests_cli_load_case(
    const char* _tag,
    const char* _path,
    const char* _version,
    int         _status,
    const char* _why
)
{
    struct dj_cli_module module = D_CLI_MODULE_INIT;
    struct dj_arg_buffer why    = D_ARG_BUFFER_INIT;
    const int           status = d_cli_module_load(&module,
                                                   _path,
                                                   "prog",
                                                   _version,
                                                   &why);
    bool                held   = (status == _status);

    // taken: the record, and the path kept
    if (status == D_ARG_OK)
    {
        held = ( (held)                                 &&
                 (module.handle != NULL)                &&
                 (module.plugin != NULL)                &&
                 (module.plugin->run != NULL)           &&
                 (strcmp(module.plugin->program,
                         "prog") == 0)                  &&
                 (strcmp(module.path,
                         _path) == 0)                   &&
                 (why.length == 0u) );
        d_cli_module_unload(&module);
        held = ( (held)                    &&
                 (module.handle == NULL)   &&
                 (module.plugin == NULL)   &&
                 (module.path == NULL) );
    }
    else
    {
        held = ( (held)                                  &&
                 (module.handle == NULL)                 &&
                 (module.path == NULL)                   &&
                 ( (_why != NULL)
                   ? d_tests_cli_holds(&why,
                                       _why)
                   : (why.length > 0u) ) );
    }

    if (!held)
    {
        printf("  [%s] status %d, why: %.*s\n",
               _tag,
               status,
               (int)why.length,
               (why.data != NULL) ? why.data : "");
    }

    d_arg_buffer_release(&why);

    return d_tests_cli_check(held,
                             _tag,
                             "the module loaded, or refused and why");
}

/**
 * @brief The probes the host must refuse, each with its reason.
 *
 * @return whether every case held.
 */
static
bool d_tests_cli_load_refused(void)
{
    char path[D_TESTS_CLI_PATH_MAX];
    bool held = d_tests_cli_load_case(
                    "L03",
                    d_tests_cli_probe_path("other",
                                           path,
                                           sizeof(path)),
                    "1.2",
                    D_ARG_ERROR_INVALID,
                    "it belongs to other, not prog");

    held = d_tests_cli_load_case("L04",
                                 d_tests_cli_probe_path("old",
                                                        path,
                                                        sizeof(path)),
                                 "1.2",
                                 D_ARG_ERROR_INVALID,
                                 "it belongs to prog 0.9, and this is prog "
                                 "1.2") && held;
    held = d_tests_cli_load_case("L05",
                                 d_tests_cli_probe_path("abi",
                                                        path,
                                                        sizeof(path)),
                                 "1.2",
                                 D_ARG_ERROR_INVALID,
                                 "it was built for plugin interface 99, and "
                                 "this program takes 1") && held;
    held = d_tests_cli_load_case("L06",
                                 d_tests_cli_probe_path("short",
                                                        path,
                                                        sizeof(path)),
                                 "1.2",
                                 D_ARG_ERROR_INVALID,
                                 "its record is too short for its "
                                 "interface") && held;
    held = d_tests_cli_load_case("L07",
                                 d_tests_cli_probe_path("idle",
                                                        path,
                                                        sizeof(path)),
                                 "1.2",
                                 D_ARG_ERROR_INVALID,
                                 "it has no work to run") && held;
    held = d_tests_cli_load_case("L08",
                                 d_tests_cli_probe_path("null",
                                                        path,
                                                        sizeof(path)),
                                 "1.2",
                                 D_ARG_ERROR_INVALID,
                                 "its entry point returned no record") &&
           held;
    held = d_tests_cli_load_case("L09",
                                 d_tests_cli_probe_path("none",
                                                        path,
                                                        sizeof(path)),
                                 "1.2",
                                 D_ARG_ERROR_FILE,
                                 "it has no entry point "
                                 D_CLI_PLUGIN_ENTRY_NAME) && held;

    return held;
}

/*
d_tests_cli_plugin_loading
  PROBE taken, at its version and at any; the refused probes; then a file
that is no module and one that is not there, each with the system's words.
*/
bool d_tests_cli_plugin_loading(void)
{
    struct dj_tests_arg_files files;
    char                     path[D_TESTS_CLI_PATH_MAX];
    bool                     held = d_tests_cli_load_case(
                                        "L01",
                                        d_tests_cli_probe_path(
                                            "probe",
                                            path,
                                            sizeof(path)),
                                        "1.2",
                                        D_ARG_OK,
                                        NULL);

    held = d_tests_cli_load_case("L02",
                                 d_tests_cli_probe_path("old",
                                                        path,
                                                        sizeof(path)),
                                 NULL,
                                 D_ARG_OK,
                                 NULL) && held;
    held = d_tests_cli_load_refused() && held;

    // a scratch directory for the file that is no module
    if (!d_tests_cli_check(d_tests_arg_files_enter(&files),
                           "L10",
                           "the scratch directory could not be made"))
    {
        return false;
    }

    held = ( (d_tests_arg_files_file(&files,
                                     D_TESTS_CLI_MODULE("prog-text"),
                                     "text, and no module")) &&
             (d_tests_cli_load_case("L10",
                                    D_TESTS_CLI_MODULE("./prog-text"),
                                    NULL,
                                    D_ARG_ERROR_FILE,
                                    NULL)) ) && held;
    held = d_tests_cli_load_case("L11",
                                 D_TESTS_CLI_MODULE("./prog-missing"),
                                 NULL,
                                 D_ARG_ERROR_FILE,
                                 NULL) && held;
    d_tests_arg_files_leave(&files);

    return held;
}


//==============================================================================
// 4.  RUNNING
//==============================================================================

/**
 * @brief A program with a plugin command for each probe, and one for a
 *        module never built, made with the program flags given.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser.
 * @param[in]     _flags    D_CLI_PROGRAM_* flags, D_CLI_PROGRAM_COMMANDS
 *                          among them.
 * @return whether it was made.
 */
static
bool d_tests_cli_make_host_with(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser,
    uint32_t               _flags
)
{
    static const char* const names[] =
    {
        "probe", "other", "old", "abi", "short", "idle", "null", "none",
        "kit", "missing"
    };
    bool made = (d_cli_program_init(_program,
                                    _parser,
                                    "prog",
                                    "1.2",
                                    _flags) == D_ARG_OK);

    for (size_t index = 0u;
         (made) && (index < (sizeof(names) / sizeof(names[0])));
         ++index)
    {
        made = (d_cli_program_add_plugin(_program,
                                         NULL,
                                         names[index],
                                         "a probe") != NULL);
    }

    return made;
}

/**
 * @brief HOST: a program with a plugin command for each probe, and one for
 *        a module never built.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser.
 * @return whether it was made.
 */
static
bool d_tests_cli_make_host(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
)
{
    return d_tests_cli_make_host_with(_program,
                                      _parser,
                                      D_CLI_PROGRAM_COMMANDS);
}

/**
 * @brief STYLED: HOST saying its errors without their word, keeping the
 *        argument parser's help, taking a prefix of a long name for the
 *        name, and writing a file it changes to standard output -- the
 *        flags a host gives its modules.
 *
 * @param[out]    _program  the program.
 * @param[in,out] _parser   its parser.
 * @return whether it was made.
 */
static
bool d_tests_cli_make_styled(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
)
{
    return d_tests_cli_make_host_with(_program,
                                      _parser,
                                      D_CLI_PROGRAM_COMMANDS      |
                                      D_CLI_PROGRAM_PLAIN_ERRORS  |
                                      D_CLI_PROGRAM_PARSER_HELP   |
                                      D_CLI_PROGRAM_ABBREVIATIONS |
                                      D_CLI_PROGRAM_EDITS_TO_OUTPUT);
}

/**
 * @brief Runs a host on a command line, PROG_PLUGIN_PATH the modules'
 *        directory, and gives back its status and what it wrote.
 *
 * @param[in]     _line    the command line.
 * @param[in]     _make    the host's fixture: HOST, or STYLED.
 * @param[out]    _status  receives the status.
 * @param[in,out] _out     receives the host's output.
 * @param[in,out] _err     receives the host's error output.
 * @return whether the host was made and run.
 */
static
bool d_tests_cli_host_run(
    const char*           _line,
    dj_tests_cli_make_fn  _make,
    int*                  _status,
    struct dj_arg_buffer* _out,
    struct dj_arg_buffer* _err
)
{
    struct dj_tests_cli_words words;
    struct dj_cli_program     program;
    struct dj_arg_parser      parser;

    // the words, and the host
    if ( (!d_tests_cli_split(_line,
                             &words)) ||
         (!d_tests_cli_start(&program,
                             &parser,
                             _make,
                             _out,
                             _err)) )
    {
        return false;
    }

    (void)d_tests_cli_setenv("PROG_PLUGIN_PATH",
                             d_tests_cli_modules);
    *_status = d_cli_program_run(&program,
                                 words.count,
                                 words.items);
    (void)d_tests_cli_setenv("PROG_PLUGIN_PATH",
                             NULL);
    d_tests_cli_finish(&program,
                       &parser);

    return true;
}

/**
 * @brief Runs PROBE through HOST and compares the status, the report the
 *        probe wrote, and the host's own output.
 *
 * @param[in] _tag     the case's tag.
 * @param[in] _make    the host's fixture: HOST, or STYLED.
 * @param[in] _line    the command line.
 * @param[in] _status  the status the run must give.
 * @param[in] _report  what the report must hold.
 * @return whether it held.
 */
static
bool d_tests_cli_probe_case(
    const char*          _tag,
    dj_tests_cli_make_fn _make,
    const char*          _line,
    int                  _status,
    const char*          _report
)
{
    struct dj_arg_buffer out    = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer err    = D_ARG_BUFFER_INIT;
    int                 status = -1;
    size_t              length = 0u;
    const bool          ran    = d_tests_cli_host_run(_line,
                                                      _make,
                                                      &status,
                                                      &out,
                                                      &err);
    char* const         report = d_tests_cli_file_read("report.txt",
                                                       &length);
    bool                held   = d_tests_cli_check(
                                     (ran)                    &&
                                     (status == _status)      &&
                                     (out.length == 0u)       &&
                                     (err.length == 0u),
                                     _tag,
                                     "the probe's status, and nothing "
                                     "from the host");

    held = d_tests_cli_same(_tag,
                            "report",
                            _report,
                            report,
                            length) && held;

    free(report);
    (void)remove("report.txt");
    d_arg_buffer_release(&out);
    d_arg_buffer_release(&err);

    return held;
}

/**
 * @brief Runs a probe the host cannot run, and compares the status and the
 *        host's error output's first line.
 *
 * @param[in] _tag    the case's tag.
 * @param[in] _line   the command line.
 * @param[in] _first  the error output's first line.
 * @param[in] _notes  how many notes must follow it.
 * @return whether it held.
 */
static
bool d_tests_cli_refused_case(
    const char* _tag,
    const char* _line,
    const char* _first,
    size_t      _notes
)
{
    struct dj_arg_buffer out    = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer err    = D_ARG_BUFFER_INIT;
    int                 status = -1;
    const bool          ran    = d_tests_cli_host_run(_line,
                                                      d_tests_cli_make_host,
                                                      &status,
                                                      &out,
                                                      &err);
    const size_t        length = strlen(_first);
    size_t              notes  = 0u;

    for (const char* at = (err.data != NULL) ? err.data : "";
         (at = strstr(at,
                      "\nprog: note: looked in ")) != NULL;
         ++at)
    {
        ++notes;
    }

    const bool held = ( (ran)                                  &&
                        (status == D_CLI_EXIT_TROUBLE)         &&
                        (out.length == 0u)                     &&
                        (err.length >= length)                 &&
                        (strncmp(err.data,
                                 _first,
                                 length) == 0)                 &&
                        (notes == _notes) );

    if (!held)
    {
        printf("  [%s] status %d, err: %.*s\n",
               _tag,
               status,
               (int)err.length,
               (err.data != NULL) ? err.data : "");
    }

    d_arg_buffer_release(&out);
    d_arg_buffer_release(&err);

    return d_tests_cli_check(held,
                             _tag,
                             "the module refused, and why");
}

// PROBE_PLAIN
//   constant: what PROBE reports run as `prog probe report.txt 3 a --b`.
static const char PROBE_PLAIN[] =
    "abi=1 size=whole program=prog version=1.2 command=prog probe\n"
    "module=" D_TESTS_CLI_MODULE("prog-probe") " executable=given "
    "verbosity=0 colour=0 flags=0\n"
    "argc=5 argv=[prog probe] [report.txt] [3] [a] [--b]\n"
    "ended=NULL\n";

// PROBE_LOUD
//   constant: what it reports run as `prog -vv --color=always probe
// report.txt`.
static const char PROBE_LOUD[] =
    "abi=1 size=whole program=prog version=1.2 command=prog probe\n"
    "module=" D_TESTS_CLI_MODULE("prog-probe") " executable=given "
    "verbosity=2 colour=1 flags=0\n"
    "argc=2 argv=[prog probe] [report.txt]\n"
    "ended=NULL\n";

// PROBE_STYLED
//   constant: what it reports run through STYLED as `prog probe
// report.txt`: the host's flags that shape what is read and written.
static const char PROBE_STYLED[] =
    "abi=1 size=whole program=prog version=1.2 command=prog probe\n"
    "module=" D_TESTS_CLI_MODULE("prog-probe") " executable=given "
    "verbosity=0 colour=0 flags=60\n"
    "argc=2 argv=[prog probe] [report.txt]\n"
    "ended=NULL\n";

// PROBE_OWN
//   constant: what it reports run as `prog probe report.txt 0 --help -v
// -- x`: every argument after the command's name is the module's.
static const char PROBE_OWN[] =
    "abi=1 size=whole program=prog version=1.2 command=prog probe\n"
    "module=" D_TESTS_CLI_MODULE("prog-probe") " executable=given "
    "verbosity=0 colour=0 flags=0\n"
    "argc=7 argv=[prog probe] [report.txt] [0] [--help] [-v] [--] [x]\n"
    "ended=NULL\n";

/*
d_tests_cli_plugin_running
  PROBE run three ways; each refused probe through the host; a command
whose module was never built; and PROBE through STYLED, given the flags
that shape what is read and written.
*/
bool d_tests_cli_plugin_running(void)
{
    struct dj_tests_arg_files files;
    char                     first[D_TESTS_CLI_PATH_MAX + 128u];
    char                     path[D_TESTS_CLI_PATH_MAX];
    bool                     held = d_tests_arg_files_enter(&files);

    // a scratch directory for the probe's reports
    if (!d_tests_cli_check(held,
                           "U01",
                           "the scratch directory could not be made"))
    {
        return false;
    }

    held = d_tests_cli_probe_case("U01",
                                  d_tests_cli_make_host,
                                  "prog probe report.txt 3 a --b",
                                  3,
                                  PROBE_PLAIN);
    held = d_tests_cli_probe_case("U02",
                                  d_tests_cli_make_host,
                                  "prog -vv --color=always probe "
                                  "report.txt",
                                  0,
                                  PROBE_LOUD) && held;
    held = d_tests_cli_probe_case("U03",
                                  d_tests_cli_make_host,
                                  "prog probe report.txt 0 --help -v -- x",
                                  0,
                                  PROBE_OWN) && held;
    (void)snprintf(first,
                   sizeof(first),
                   "prog: error: cannot load '%s': it belongs to other, "
                   "not prog\n",
                   d_tests_cli_probe_path("other",
                                          path,
                                          sizeof(path)));
    held = d_tests_cli_refused_case("U04",
                                    "prog other",
                                    first,
                                    0u) && held;
    held = d_tests_cli_refused_case("U05",
                                    "prog missing",
                                    "prog: error: 'missing' is not "
                                    "installed: "
                                    D_TESTS_CLI_MODULE("prog-missing")
                                    " was not found\n"
                                    "prog: note: looked in ",
                                    3u) && held;
    held = d_tests_cli_probe_case("U06",
                                  d_tests_cli_make_styled,
                                  "prog probe report.txt",
                                  0,
                                  PROBE_STYLED) && held;
    d_tests_arg_files_leave(&files);

    return held;
}


//==============================================================================
// 5.  A PROGRAM OF ITS OWN
//==============================================================================

/**
 * @brief Runs KIT through a host, its standard streams redirected to files,
 *        and compares the status and what it wrote to each.
 *
 * @param[in] _tag     the case's tag.
 * @param[in] _make    the host's fixture: HOST, or STYLED.
 * @param[in] _line    the command line.
 * @param[in] _status  the status the run must give.
 * @param[in] _out     what KIT must write to standard output.
 * @param[in] _err     what it must write to standard error.
 * @return whether it held.
 */
static
bool d_tests_cli_kit_case(
    const char*          _tag,
    dj_tests_cli_make_fn _make,
    const char*          _line,
    int                  _status,
    const char*          _out,
    const char*          _err
)
{
    struct dj_arg_buffer out     = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer err     = D_ARG_BUFFER_INIT;
    int                 status  = -1;
    size_t              written = 0u;
    size_t              said    = 0u;
    const int           saved_1 = d_tests_cli_redirect(1,
                                                       "kit.out");
    const int           saved_2 = d_tests_cli_redirect(2,
                                                       "kit.err");
    const bool          ran     = d_tests_cli_host_run(_line,
                                                       _make,
                                                       &status,
                                                       &out,
                                                       &err);

    d_tests_cli_restore(2,
                        saved_2);
    d_tests_cli_restore(1,
                        saved_1);

    char* const stdout_text = d_tests_cli_file_read("kit.out",
                                                    &written);
    char* const stderr_text = d_tests_cli_file_read("kit.err",
                                                    &said);
    bool        held        = d_tests_cli_check(
                                  (saved_1 >= 0)          &&
                                  (saved_2 >= 0)          &&
                                  (ran)                   &&
                                  (status == _status)     &&
                                  (out.length == 0u)      &&
                                  (err.length == 0u),
                                  _tag,
                                  "the module's status, and nothing from "
                                  "the host");

    held = d_tests_cli_same(_tag,
                            "out",
                            _out,
                            stdout_text,
                            written) && held;
    held = d_tests_cli_same(_tag,
                            "err",
                            _err,
                            stderr_text,
                            said) && held;

    free(stdout_text);
    free(stderr_text);
    (void)remove("kit.out");
    (void)remove("kit.err");
    d_arg_buffer_release(&out);
    d_arg_buffer_release(&err);

    return held;
}

// KIT_HELP
//   constant: KIT's help, its usage under the host's command.
static const char KIT_HELP[] =
    "Usage: prog kit [OPTION]... NAME\n"
    "Say hello from a module.\n"
    "\n"
    "Arguments:\n"
    "  NAME                who to say hello to\n"
    "\n"
    "Options:\n"
    "  -q, --quiet         say nothing but errors\n"
    "  -v, --verbose       say more; twice, more again\n"
    "      --color[=WHEN]  colour diagnostics: auto (the default), always "
    "or never\n"
    "      --shout         say it in capitals\n"
    "  -h, --help          display this help and exit\n"
    "      --version       output version information and exit\n"
    "\n"
    "Exit status:\n"
    "  0  if it did what was asked, and the answer, if one was asked, is "
    "yes\n"
    "  1  if it did, and the answer is no\n"
    "  2  if there was trouble: the command line, a file, or the "
    "program\n";

// KIT_HELP_PARSER
//   constant: KIT's help through STYLED: the argument parser's own layout,
// as its host keeps.
static const char KIT_HELP_PARSER[] =
    "say hello from a module\n"
    "\n"
    "\n"
    "prog kit [OPTIONS] NAME\n"
    "\n"
    "\n"
    "POSITIONALS:\n"
    "  NAME REQUIRED               who to say hello to\n"
    "\n"
    "OPTIONS:\n"
    "  -h,     --help              display this help and exit\n"
    "          --version           output version information and exit\n"
    "  -q,     --quiet             say nothing but errors\n"
    "  -v,     --verbose           say more; twice, more again\n"
    "          --color{always} WHEN \n"
    "                              colour diagnostics: auto (the default), "
    "always or never\n"
    "          --shout             say it in capitals\n";

// KIT_BUILD
//   constant: what KIT says of its build, and its own line.
static const char KIT_BUILD[] =
    "compiler: Probe CC 4.5.6\n"
    "language: C99\n"
    "system: Probe OS, probe-64\n"
    "build: Release\n"
    "probe: kit\n";

/**
 * @brief Runs KIT's `--version --verbose` and compares what it says: the
 *        version's line; the host's file, as the system names it, and the
 *        module's, as the host found it; KIT's build; and its kit's.
 *
 * @param[in] _tag   the case's tag.
 * @param[in] _line  the command line.
 * @return whether it held.
 */
static
bool d_tests_cli_kit_version(
    const char* _tag,
    const char* _line
)
{
    struct dj_arg_buffer expected = D_ARG_BUFFER_INIT;
    char                path[D_TESTS_CLI_PATH_MAX];
    char                kit[256];
    char*               exe      = NULL;
    const bool          named    = (d_cli_executable_path("prog",
                                                          &exe) == 0);

    d_tests_cli_kit_line(kit,
                         sizeof(kit));
    (void)d_arg_buffer_printf(&expected,
                              "prog 1.2\n%s%s%smodule: %s\n%s%s",
                              (named) ? "program: " : "",
                              (named) ? exe : "",
                              (named) ? "\n" : "",
                              d_tests_cli_probe_path("kit",
                                                     path,
                                                     sizeof(path)),
                              KIT_BUILD,
                              kit);
    d_cli_free(exe);

    const bool held = d_tests_cli_kit_case(_tag,
                                           d_tests_cli_make_host,
                                           _line,
                                           0,
                                           (expected.data != NULL)
                                           ? expected.data
                                           : "",
                                           "");

    d_arg_buffer_release(&expected);

    return held;
}

/*
d_tests_cli_plugin_kit
  KIT's work, its note at the host's `-v`, its help, its failures in the
host's colour, and its version, plain and with its build -- `-v` its own,
or the host's; and through STYLED, a failure without its word, help in the
parser's layout and a prefix taken for a long name, as the host has them.
*/
bool d_tests_cli_plugin_kit(void)
{
    struct dj_tests_arg_files files;
    bool                     held = d_tests_arg_files_enter(&files);

    // a scratch directory for the streams' files
    if (!d_tests_cli_check(held,
                           "K01",
                           "the scratch directory could not be made"))
    {
        return false;
    }

    held = d_tests_cli_kit_case("K01",
                                d_tests_cli_make_host,
                                "prog kit --shout world",
                                0,
                                "HELLO world\n",
                                "");
    held = d_tests_cli_kit_case("K02",
                                d_tests_cli_make_host,
                                "prog -v kit world",
                                0,
                                "hello world\n",
                                "prog: note: said hello\n") && held;
    held = d_tests_cli_kit_case("K03",
                                d_tests_cli_make_host,
                                "prog kit --help",
                                0,
                                KIT_HELP,
                                "") && held;
    held = d_tests_cli_kit_case("K04",
                                d_tests_cli_make_host,
                                "prog --color=always kit --frob world",
                                2,
                                "",
                                "\033[1mprog:\033[0m \033[1;31merror:\033[0m "
                                "kit: the following argument was not "
                                "expected: --frob\n"
                                "Try 'prog kit --help' for more "
                                "information.\n") && held;
    held = d_tests_cli_kit_case("K05",
                                d_tests_cli_make_host,
                                "prog -q kit",
                                2,
                                "",
                                "prog: error: kit: NAME is required\n"
                                "Try 'prog kit --help' for more "
                                "information.\n") && held;
    held = d_tests_cli_kit_case("K06",
                                d_tests_cli_make_host,
                                "prog kit --version",
                                0,
                                "prog 1.2\n",
                                "") && held;
    held = d_tests_cli_kit_version("K07",
                                   "prog kit --version -v") && held;
    held = d_tests_cli_kit_version("K08",
                                   "prog -v kit --version") && held;
    held = d_tests_cli_kit_case("K09",
                                d_tests_cli_make_styled,
                                "prog kit --frob world",
                                2,
                                "",
                                "prog: kit: the following argument was not "
                                "expected: --frob\n"
                                "Try 'prog kit --help' for more "
                                "information.\n") && held;
    held = d_tests_cli_kit_case("K10",
                                d_tests_cli_make_styled,
                                "prog kit --help",
                                0,
                                KIT_HELP_PARSER,
                                "") && held;
    held = d_tests_cli_kit_case("K11",
                                d_tests_cli_make_styled,
                                "prog kit --sho world",
                                0,
                                "HELLO world\n",
                                "") && held;
    d_tests_arg_files_leave(&files);

    return held;
}


//==============================================================================
// 6.  THE VERSION
//==============================================================================

/**
 * @brief What HOST's `--version --verbose` must say: the version's line;
 *        the runner's file, as the system names it; where each command's
 *        module would be loaded from -- each probe's from the modules'
 *        directory, MISSING's not installed; or, where the kit has no
 *        loader, none -- and the kit's line.
 *
 * @param[in,out] _expected  receives the text.
 */
static
void d_tests_cli_host_version(
    struct dj_arg_buffer* _expected
)
{
    static const char* const names[] =
    {
        "probe", "other", "old", "abi", "short", "idle", "null", "none",
        "kit"
    };
    char                     path[D_TESTS_CLI_PATH_MAX];
    char                     kit[256];
    char*                    exe   = NULL;
    const bool               named = (d_cli_executable_path("prog",
                                                            &exe) == 0);

    (void)d_arg_buffer_printf(_expected,
                              "prog 1.2\n%s%s%s",
                              (named) ? "program: " : "",
                              (named) ? exe : "",
                              (named) ? "\n" : "");
    d_cli_free(exe);

    for (size_t index = 0u;
         index < (sizeof(names) / sizeof(names[0]));
         ++index)
    {
        (void)d_arg_buffer_printf(_expected,
                                  "%s module: %s\n",
                                  names[index],
                                  (D_INTERNAL_CLI_PLUGINS == 1)
                                  ? d_tests_cli_probe_path(names[index],
                                                           path,
                                                           sizeof(path))
                                  : "built without plugins");
    }

    d_tests_cli_kit_line(kit,
                         sizeof(kit));
    (void)d_arg_buffer_printf(_expected,
                              "missing module: %s\n%s",
                              (D_INTERNAL_CLI_PLUGINS == 1)
                              ? "not installed"
                              : "built without plugins",
                              kit);

    return;
}

/*
d_tests_cli_plugin_version
  HOST's `--version`, the line alone; and its `--version --verbose`, with
the modules' directory along PROG_PLUGIN_PATH.
*/
bool d_tests_cli_plugin_version(void)
{
    struct dj_arg_buffer expected = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer out      = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer err      = D_ARG_BUFFER_INIT;
    int                 status   = -1;
    bool                held     = d_tests_cli_host_run("prog --version",
                                                        d_tests_cli_make_host,
                                                        &status,
                                                        &out,
                                                        &err);

    held = d_tests_cli_check( (held)                          &&
                              (status == D_CLI_EXIT_OK)       &&
                              (d_tests_cli_holds(&out,
                                                 "prog 1.2\n")) &&
                              (err.length == 0u),
                              "P01",
                              "the version's line alone");
    d_arg_buffer_release(&out);
    d_arg_buffer_release(&err);
    d_tests_cli_host_version(&expected);
    held = d_tests_cli_check( (d_tests_cli_host_run("prog --version -v",
                                                    d_tests_cli_make_host,
                                                    &status,
                                                    &out,
                                                    &err))    &&
                              (status == D_CLI_EXIT_OK)       &&
                              (err.length == 0u),
                              "P02",
                              "the build, and nothing on the error "
                              "output") && held;
    held = d_tests_cli_same("P02",
                            "out",
                            (expected.data != NULL) ? expected.data : "",
                            out.data,
                            out.length) && held;
    d_arg_buffer_release(&expected);
    d_arg_buffer_release(&out);
    d_arg_buffer_release(&err);

    return held;
}


//==============================================================================
// 7.  THE MODULE'S SIDE
//==============================================================================

/**
 * @brief Makes a module's program from a host, parses a command line with
 *        it, and gives back the verbosity and colour it came to.
 *
 * @param[in]  _host       the host.
 * @param[in]  _line       the command line; NULL to parse nothing.
 * @param[out] _verbosity  receives the program's verbosity.
 * @param[out] _colour     receives its colour mode.
 * @return d_cli_program_init_plugin's status.
 */
static
int d_tests_cli_module_with(
    const struct dj_cli_plugin_host* _host,
    const char*                      _line,
    int*                             _verbosity,
    int*                             _colour
)
{
    struct dj_tests_cli_words words;
    struct dj_cli_program     program;
    struct dj_arg_parser      parser;
    int                      exit_status = 0;

    memset(&program,
           0,
           sizeof(program));

    // a parser, as the module makes one
    if (d_arg_parser_init(&parser,
                          "a module",
                          "prog gui") != D_ARG_OK)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    const int status = d_cli_program_init_plugin(&program,
                                                 &parser,
                                                 _host,
                                                 0u);

    if ( (status == D_ARG_OK) &&
         (_line != NULL)      &&
         (d_tests_cli_split(_line,
                            &words)) )
    {
        (void)d_cli_program_parse(&program,
                                  words.count,
                                  words.items,
                                  &exit_status);
    }

    *_verbosity = program.verbosity;
    *_colour    = program.colour;
    d_cli_program_release(&program);
    d_arg_parser_release(&parser);

    return status;
}

/**
 * @brief The hosts a module's program refuses: of another interface, with
 *        a record too short, with no program.
 *
 * @param[in] _host  a host that is one, as a module is given it.
 * @return whether every check held.
 */
static
bool d_tests_cli_module_refused(
    const struct dj_cli_plugin_host* _host
)
{
    struct dj_cli_plugin_host host      = *_host;
    int                      verbosity = 0;
    int                      colour    = 0;
    bool                     held      = true;

    host.abi = D_CLI_PLUGIN_ABI + 1u;
    held     = d_tests_cli_check(d_tests_cli_module_with(
                                     &host,
                                     NULL,
                                     &verbosity,
                                     &colour) == D_ARG_ERROR_BAD_ARGUMENT,
                                 "S02",
                                 "a host of another interface") && held;

    host      = *_host;
    host.size = 4u;
    held      = d_tests_cli_check(d_tests_cli_module_with(
                                      &host,
                                      NULL,
                                      &verbosity,
                                      &colour) == D_ARG_ERROR_BAD_ARGUMENT,
                                  "S03",
                                  "a host's record too short") && held;

    host         = *_host;
    host.program = NULL;

    return d_tests_cli_check(d_tests_cli_module_with(
                                 &host,
                                 NULL,
                                 &verbosity,
                                 &colour) == D_ARG_ERROR_BAD_ARGUMENT,
                             "S04",
                             "a host with no program") && held;
}

/**
 * @brief Makes a module's program from a host, as a module makes one, and
 *        gives back the flags it was made with.
 *
 * @param[in]  _host   the host.
 * @param[in]  _own    the module's own D_CLI_PROGRAM_* flags.
 * @param[out] _flags  receives the program's flags.
 * @return what d_cli_program_init_plugin returned.
 */
static
int d_tests_cli_module_flags(
    const struct dj_cli_plugin_host* _host,
    uint32_t                         _own,
    uint32_t*                        _flags
)
{
    struct dj_cli_program program;
    struct dj_arg_parser  parser;

    memset(&program,
           0,
           sizeof(program));

    // a parser, as the module makes one
    if (d_arg_parser_init(&parser,
                          "a module",
                          "prog gui") != D_ARG_OK)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    const int status = d_cli_program_init_plugin(&program,
                                                 &parser,
                                                 _host,
                                                 _own);

    *_flags = program.flags;
    d_cli_program_release(&program);
    d_arg_parser_release(&parser);

    return status;
}

/**
 * @brief The host's flags a module's program takes: those that shape what
 *        is read and written, joined to its own, and no others; none from a
 *        record of the first size, which has none, and which is still
 *        accepted; and a record shorter than that refused.
 *
 * @param[in] _host  a whole host.
 * @return whether every check held.
 */
static
bool d_tests_cli_module_styles(
    const struct dj_cli_plugin_host* _host
)
{
    struct dj_cli_plugin_host host  = *_host;
    uint32_t                 flags = 0u;

    host.flags = D_CLI_PROGRAM_COMMANDS       |
                 D_CLI_PROGRAM_RESPONSE_FILES |
                 D_CLI_PROGRAM_PARSER_HELP    |
                 D_CLI_PROGRAM_PLAIN_ERRORS   |
                 D_CLI_PROGRAM_ABBREVIATIONS  |
                 D_CLI_PROGRAM_EDITS_TO_OUTPUT;

    bool held = d_tests_cli_check(
                    (d_tests_cli_module_flags(&host,
                                              0u,
                                              &flags) == D_ARG_OK) &&
                    (flags == (D_CLI_PROGRAM_PARSER_HELP   |
                               D_CLI_PROGRAM_PLAIN_ERRORS  |
                               D_CLI_PROGRAM_ABBREVIATIONS |
                               D_CLI_PROGRAM_EDITS_TO_OUTPUT)),
                    "S05",
                    "a host's flags that shape what is read and written, "
                    "and no others");

    host.flags = D_CLI_PROGRAM_PLAIN_ERRORS;
    held       = d_tests_cli_check(
                     (d_tests_cli_module_flags(&host,
                                               D_CLI_PROGRAM_COMMANDS,
                                               &flags) == D_ARG_OK) &&
                     (flags == (D_CLI_PROGRAM_COMMANDS |
                                D_CLI_PROGRAM_PLAIN_ERRORS)),
                     "S06",
                     "the host's joined to the module's own") && held;

    host.size = (uint32_t)D_CLI_PLUGIN_HOST_FIRST_SIZE;
    held      = d_tests_cli_check(
                    (d_tests_cli_module_flags(&host,
                                              0u,
                                              &flags) == D_ARG_OK) &&
                    (flags == 0u),
                    "S07",
                    "a record of the first size: taken, its flags not "
                    "read") && held;

    host.size = (uint32_t)D_CLI_PLUGIN_HOST_FIRST_SIZE - 1u;

    return d_tests_cli_check(d_tests_cli_module_flags(
                                 &host,
                                 0u,
                                 &flags) == D_ARG_ERROR_BAD_ARGUMENT,
                             "S08",
                             "a record shorter than the first") && held;
}

/*
d_tests_cli_plugin_module_side
  The host's verbosity is where `-v` counts from, and `-q` still silences;
its colour holds until the module's own `--color`; a host that is not one
is refused; and the host's flags that shape what is written are the
module's too, where its record has them.
*/
bool d_tests_cli_plugin_module_side(void)
{
    struct dj_cli_plugin_host host;
    int                      verbosity = 0;
    int                      colour    = 0;
    bool                     held      = true;

    memset(&host,
           0,
           sizeof(host));
    host.abi       = D_CLI_PLUGIN_ABI;
    host.size      = (uint32_t)sizeof(host);
    host.program   = "prog";
    host.version   = "1.2";
    host.command   = "prog gui";
    host.verbosity = 2;
    host.colour    = (int)D_CLI_COLOUR_ALWAYS;

    held = d_tests_cli_check(
               (d_tests_cli_module_with(&host,
                                        "prog-gui",
                                        &verbosity,
                                        &colour) == D_ARG_OK)    &&
               (verbosity == 2)                                  &&
               (colour == (int)D_CLI_COLOUR_ALWAYS)              &&
               (d_tests_cli_module_with(&host,
                                        "prog-gui -v",
                                        &verbosity,
                                        &colour) == D_ARG_OK)    &&
               (verbosity == 3)                                  &&
               (d_tests_cli_module_with(&host,
                                        "prog-gui -q --color=never",
                                        &verbosity,
                                        &colour) == D_ARG_OK)    &&
               (verbosity == -1)                                 &&
               (colour == (int)D_CLI_COLOUR_NEVER),
               "S01",
               "the host's verbosity and colour, and the module's own") &&
           held;

    held = d_tests_cli_module_refused(&host) && held;

    return d_tests_cli_module_styles(&host) && held;
}


//==============================================================================
// 8.  NULLS
//==============================================================================

/*
d_tests_cli_plugin_nulls
  A status, NULL or nothing for every pointer the calls need and were not
given.
*/
bool d_tests_cli_plugin_nulls(void)
{
    struct dj_cli_module      module = D_CLI_MODULE_INIT;
    struct dj_arg_buffer      found  = D_ARG_BUFFER_INIT;
    struct dj_cli_plugin_host host;
    struct dj_arg_parser      parser;

    memset(&host,
           0,
           sizeof(host));
    host.abi     = D_CLI_PLUGIN_ABI;
    host.size    = (uint32_t)sizeof(host);
    host.program = "prog";
    d_cli_module_unload(NULL);
    d_cli_module_unload(&module);

    return d_tests_cli_check(
               (d_cli_plugin_find(NULL,
                                  "gui",
                                  NULL,
                                  &found,
                                  NULL) == D_ARG_ERROR_BAD_ARGUMENT)    &&
               (d_cli_plugin_find("prog",
                                  NULL,
                                  NULL,
                                  &found,
                                  NULL) == D_ARG_ERROR_BAD_ARGUMENT)    &&
               (d_cli_plugin_find("prog",
                                  "gui",
                                  NULL,
                                  NULL,
                                  NULL) == D_ARG_ERROR_BAD_ARGUMENT)    &&
               (d_cli_module_load(NULL,
                                  "x",
                                  "prog",
                                  NULL,
                                  NULL) == D_ARG_ERROR_BAD_ARGUMENT)    &&
               (d_cli_module_load(&module,
                                  NULL,
                                  "prog",
                                  NULL,
                                  NULL) == D_ARG_ERROR_BAD_ARGUMENT)    &&
               (d_cli_module_load(&module,
                                  "x",
                                  NULL,
                                  NULL,
                                  NULL) == D_ARG_ERROR_BAD_ARGUMENT)    &&
               (d_cli_program_add_plugin(NULL,
                                         NULL,
                                         "gui",
                                         "x") == NULL)                  &&
               (d_cli_program_init_plugin(NULL,
                                          &parser,
                                          &host,
                                          0u) == D_ARG_ERROR_BAD_ARGUMENT) &&
               (d_cli_program_init_plugin(NULL,
                                          &parser,
                                          NULL,
                                          0u) == D_ARG_ERROR_BAD_ARGUMENT),
               "N01",
               "a status or NULL for what the calls were not given");
}


//==============================================================================
// THE SUITE
//==============================================================================

// d_tests_cli_plugin
//   constant: the suite's sections, in the order they run; loading and
// running modules where the kit has a loader.
const struct dj_tests_section d_tests_cli_plugin[] =
{
    { "cli_plugin_executable",  d_tests_cli_plugin_executable  },
    { "cli_plugin_finding",     d_tests_cli_plugin_finding     },
#if (D_INTERNAL_CLI_PLUGINS == 1)
    { "cli_plugin_loading",     d_tests_cli_plugin_loading     },
    { "cli_plugin_running",     d_tests_cli_plugin_running     },
    { "cli_plugin_kit",         d_tests_cli_plugin_kit         },
#endif
    { "cli_plugin_version",     d_tests_cli_plugin_version     },
    { "cli_plugin_module_side", d_tests_cli_plugin_module_side },
    { "cli_plugin_nulls",       d_tests_cli_plugin_nulls       }
};

// d_tests_cli_plugin_count
//   constant: how many sections d_tests_cli_plugin has.
const size_t d_tests_cli_plugin_count =
    sizeof(d_tests_cli_plugin) / sizeof(d_tests_cli_plugin[0]);
