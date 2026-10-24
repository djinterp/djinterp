/*******************************************************************************
* djinterp [parse]                                             cli_tests_probe.c
*
* The modules the plugin suite loads: one source, built once for each kind
* D_TESTS_CLI_PROBE names, each into a module `prog-KIND` beside the others.
*   PROBE belongs to `prog 1.2`, made with D_CLI_PLUGIN: its run writes what
* the host gave it -- the host's record, the module's and the program's file
* names, and every argument -- to the file its first argument names, and
* returns the number its second argument holds. OTHER belongs to another
* program, OLD to another version, ABI to another interface, SHORT has a
* record too short for its interface, IDLE has no work, NULL's entry point
* returns no record, and NONE has no entry point at all: each is a module
* the host must refuse, and say why. KIT is a module as a program's window
* is one: its own copy of the kit, a program of its own made from the host
* (d_cli_program_init_plugin), its own help and its own failures, written to
* the process's standard streams.
*   Only the entry point is exported: the build gives a module hidden
* visibility, and D_CLI_PLUGIN exports the one symbol.
*
*     D_TESTS_CLI_PROBE    which kind: one of D_TESTS_CLI_PROBE_*
*
*
* path:      /tests/djinterp/parse/cli/c/probe/cli_tests_probe.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.23
*******************************************************************************/
// std
#include <stdio.h>    // fopen, fprintf, fclose
#include <stdlib.h>   // atoi
#include <string.h>   // strrchr
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/env/c/env_vendor_attributes.h"  // D_EXPORT
#include "../../../../../../inc/djinterp/parse/cli/c/cli_plugin.h"  // the plugin interface


// D_TESTS_CLI_PROBE_*
//   constants: the kinds of module this source builds.
#define D_TESTS_CLI_PROBE_PROBE  1
#define D_TESTS_CLI_PROBE_OTHER  2
#define D_TESTS_CLI_PROBE_OLD    3
#define D_TESTS_CLI_PROBE_ABI    4
#define D_TESTS_CLI_PROBE_SHORT  5
#define D_TESTS_CLI_PROBE_IDLE   6
#define D_TESTS_CLI_PROBE_NULL   7
#define D_TESTS_CLI_PROBE_NONE   8
#define D_TESTS_CLI_PROBE_KIT    9

#ifndef D_TESTS_CLI_PROBE
    #error "cli_tests_probe: D_TESTS_CLI_PROBE names no kind of probe"
#endif


#if (D_TESTS_CLI_PROBE == D_TESTS_CLI_PROBE_KIT)

// std
#include <stdbool.h>  // bool
// djinterp
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // commands
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // options
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // the parser
#include "../../../../../../inc/djinterp/parse/cli/c/cli_program.h"  // the program

/**
 * @brief KIT's work: says hello to the name given -- in capitals after
 *        `--shout` -- and notes that it did at `-v`.
 *
 * @param[in] _program  the module's program.
 * @param[in] _command  its root.
 * @param[in] _context  unused.
 * @return D_CLI_EXIT_OK.
 */
static int
d_tests_cli_probe_hello(
    struct d_cli_program* _program,
    struct d_arg_command* _command,
    void*                 _context
)
{
    size_t                          count = 0u;
    const struct d_pack_text* const name  = d_arg_option_values(
                                                d_arg_command_option(
                                                    _command,
                                                    "NAME"),
                                                &count);
    const bool                      shout = (d_arg_option_count(
                                                 d_arg_command_option(
                                                     _command,
                                                     "--shout")) > 0u);
    char                            text[128];
    const int                       length =
        snprintf(text,
                 sizeof(text),
                 "%s %.*s\n",
                 (shout) ? "HELLO" : "hello",
                 (count > 0u) ? (int)name[0].length : 0,
                 (count > 0u) ? name[0].data : "");

    (void)_context;

    if ( (length > 0) &&
         ((size_t)length < sizeof(text)) )
    {
        (void)_program->out.write(_program->out.context,
                                  text,
                                  (size_t)length);
    }

    d_cli_note(_program,
               1,
               "said hello");

    return D_CLI_EXIT_OK;
}

/**
 * @brief KIT's definitions: NAME, required, and `--shout`.
 *
 * @param[in,out] _program  the module's program.
 * @param[in,out] _root     its root.
 * @return whether they were made.
 */
static bool
d_tests_cli_probe_define(
    struct d_cli_program* _program,
    struct d_arg_command* _root
)
{
    struct d_arg_option* const name = d_arg_command_add_option(
                                          _root,
                                          "NAME",
                                          "who to say hello to");

    if (name != NULL)
    {
        d_arg_option_set_required(name,
                                  true);
    }

    return ( (name != NULL)                                      &&
             (d_arg_command_add_flag(_root,
                                     "--shout",
                                     "say it in capitals") != NULL) &&
             (d_cli_program_set_main(_program,
                                     d_tests_cli_probe_hello,
                                     NULL,
                                     NULL) == D_ARG_OK) );
}

/**
 * @brief KIT's run: a program of the module's own, made from the host, run
 *        on the arguments the host's command left.
 *
 * @param[in] _host  the host.
 * @param[in] _argc  the argument count.
 * @param[in] _argv  the arguments, the host's command first.
 * @return the program's status.
 */
static int
d_tests_cli_probe_kit(
    const struct d_cli_plugin_host* _host,
    int                             _argc,
    const char* const*              _argv
)
{
    struct d_arg_parser  parser;
    struct d_cli_program program;
    int                  status = D_CLI_EXIT_TROUBLE;

    // a parser named as the host's command
    if (d_arg_parser_init(&parser,
                          "say hello from a module",
                          _host->command) != D_ARG_OK)
    {
        return status;
    }

    if ( (d_cli_program_init_plugin(&program,
                                    &parser,
                                    _host,
                                    0u) == D_ARG_OK)          &&
         (d_tests_cli_probe_define(&program,
                                   &parser.command)) )
    {
        status = d_cli_program_run(&program,
                                   _argc,
                                   _argv);
    }

    d_cli_program_release(&program);
    d_arg_parser_release(&parser);

    return status;
}

D_CLI_PLUGIN("prog",
             "1.2",
             "kit",
             "say hello, through the kit",
             d_tests_cli_probe_kit)

#elif (D_TESTS_CLI_PROBE == D_TESTS_CLI_PROBE_NONE)

// d_tests_cli_probe_nothing
//   function: what NONE exports in place of an entry point.
D_EXTERN_C D_EXPORT int d_tests_cli_probe_nothing(void);

D_EXTERN_C D_EXPORT int
d_tests_cli_probe_nothing(void)
{
    return 0;
}

#else

#if (D_TESTS_CLI_PROBE != D_TESTS_CLI_PROBE_IDLE)

/**
 * @brief A file name without its directories.
 *
 * @param[in] _path  the path; may be NULL.
 * @return its last part, or "(none)" for NULL.
 */
static const char*
d_tests_cli_probe_base(
    const char* _path
)
{
    const char* slash = NULL;

    // nothing to name
    if (!_path)
    {
        return "(none)";
    }

    slash = strrchr(_path,
                    '/');
#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    {
        const char* const back = strrchr(_path,
                                         '\\');

        slash = ( (back != NULL) &&
                  ( (slash == NULL) ||
                    (back > slash) ) )
                ? back
                : slash;
    }
#endif

    return (slash != NULL) ? (slash + 1) : _path;
}

/**
 * @brief PROBE's run: what the host gave it, written to the file its first
 *        argument names.
 *
 * @param[in] _host  the host.
 * @param[in] _argc  the argument count.
 * @param[in] _argv  the arguments, the host's command first.
 * @return the number the second argument holds; 0 without one; 2 when
 *         there is no file to write.
 */
static int
d_tests_cli_probe_run(
    const struct d_cli_plugin_host* _host,
    int                             _argc,
    const char* const*              _argv
)
{
    FILE* const report = (_argc > 1) ? fopen(_argv[1],
                                             "wb")
                                     : NULL;

    // nowhere to write
    if (!report)
    {
        return 2;
    }

    (void)fprintf(report,
                  "abi=%lu size=%s program=%s version=%s command=%s\n"
                  "module=%s executable=%s verbosity=%d colour=%d\n"
                  "argc=%d argv=",
                  (unsigned long)_host->abi,
                  (_host->size >= sizeof(*_host)) ? "whole" : "short",
                  _host->program,
                  _host->version,
                  _host->command,
                  d_tests_cli_probe_base(_host->module),
                  (_host->executable != NULL) ? "given" : "(none)",
                  _host->verbosity,
                  _host->colour,
                  _argc);

    for (int index = 0; index < _argc; ++index)
    {
        (void)fprintf(report,
                      "%s[%s]",
                      (index > 0) ? " " : "",
                      _argv[index]);
    }

    (void)fprintf(report,
                  "\nended=%s\n",
                  (_argv[_argc] == NULL) ? "NULL" : "not NULL");
    (void)fclose(report);

    return (_argc > 2) ? atoi(_argv[2]) : 0;
}

#endif  // D_TESTS_CLI_PROBE_IDLE

#if (D_TESTS_CLI_PROBE == D_TESTS_CLI_PROBE_PROBE)

D_CLI_PLUGIN("prog",
             "1.2",
             "probe",
             "write what the host gave it",
             d_tests_cli_probe_run)

#elif (D_TESTS_CLI_PROBE == D_TESTS_CLI_PROBE_OTHER)

D_CLI_PLUGIN("other",
             "1.2",
             "other",
             "belong to another program",
             d_tests_cli_probe_run)

#elif (D_TESTS_CLI_PROBE == D_TESTS_CLI_PROBE_OLD)

D_CLI_PLUGIN("prog",
             "0.9",
             "old",
             "belong to another version",
             d_tests_cli_probe_run)

#else

// D_TESTS_CLI_PROBE_ABI_OF / _SIZE / _RUN
//   constants: what the hand-made record of ABI, SHORT, IDLE and NULL says.
#if (D_TESTS_CLI_PROBE == D_TESTS_CLI_PROBE_ABI)
    #define D_TESTS_CLI_PROBE_ABI_OF  99u
#else
    #define D_TESTS_CLI_PROBE_ABI_OF  D_CLI_PLUGIN_ABI
#endif

#if (D_TESTS_CLI_PROBE == D_TESTS_CLI_PROBE_SHORT)
    #define D_TESTS_CLI_PROBE_SIZE    8u
#else
    #define D_TESTS_CLI_PROBE_SIZE    sizeof(struct d_cli_plugin)
#endif

#if (D_TESTS_CLI_PROBE == D_TESTS_CLI_PROBE_IDLE)
    #define D_TESTS_CLI_PROBE_RUN     NULL
#else
    #define D_TESTS_CLI_PROBE_RUN     d_tests_cli_probe_run
#endif

D_EXTERN_C D_EXPORT const struct d_cli_plugin* d_cli_plugin_entry(void);

/*
d_cli_plugin_entry
  The record as D_CLI_PLUGIN makes it, but for the field the kind gets
wrong; NULL's returns none.
*/
D_EXTERN_C D_EXPORT const struct d_cli_plugin*
d_cli_plugin_entry(void)
{
    static const struct d_cli_plugin plugin =
    {
        D_TESTS_CLI_PROBE_ABI_OF,
        D_TESTS_CLI_PROBE_SIZE,
        "prog",
        "1.2",
        "probe",
        "a probe the host refuses",
        D_TESTS_CLI_PROBE_RUN
    };

#if (D_TESTS_CLI_PROBE == D_TESTS_CLI_PROBE_NULL)
    (void)plugin;

    return NULL;
#else
    return &plugin;
#endif
}

#endif  // D_TESTS_CLI_PROBE

#endif  // D_TESTS_CLI_PROBE_KIT, D_TESTS_CLI_PROBE_NONE
