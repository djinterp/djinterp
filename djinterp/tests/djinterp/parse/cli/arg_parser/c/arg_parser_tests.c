/*******************************************************************************
* djinterp [parse]                                            arg_parser_tests.c
*
* The suite of the argument parser itself, declared in arg_parser_tests.h.
*   Each section but the last is a table of steps (arg_tests_support.h):
* each case makes its options and commands as CLI11's App("d", "prog")
* would, on a run's parser of its own -- App("d") naming the root as the
* names' cases need -- and writes what it does as the parser suite's CLI11
* reference writes it: a line naming what it parses, then what the parse
* came to -- the line an exit came to, or the report a run gives -- the
* section's cases compared whole. Main's arguments are built in storage the
* run keeps, so that what a parse borrows outlives it. The cases of the exit
* probe keep their tags (E01-E11), as do the parse probe's parses of a
* vector (Q13, Q14).
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_parser_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./arg_parser_tests.h"  // corresponding header
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <stdio.h>    // printf
#include <string.h>   // memset, strcmp, strlen
// djinterp
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text, d_pack_sink
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // commands, their settings
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // statuses, errors, buffers, sinks
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // options, their settings
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // the parser, its endings
#include "../../../c/tests_section.h"  // d_tests_section
#include "./arg_tests_support.h"  // runs, steps, notes, labels
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// EXIT
//   constant: what the cases of d_tests_arg_parser_exits come to, as the
// reference writes them with CLI11.
static const char EXIT[] =
    "E01 code=105 out=[] err=[--x: bad value\\nRun with --help for more "
    "information.\\n]\n"
    "E02 code=105 out=[] err=[--x: bad value\\nRun with --help or --help-all "
    "for more information.\\n]\n"
    "E03 code=106 out=[] err=[--y is required\\nRun with --help-all for more "
    "information.\\n]\n"
    "E04 code=104 out=[] err=[The value --z is not an allowed value for "
    "abc\\n]\n"
    "E05 code=3 out=[] err=[]\n"
    "E06 code=0 out=[] err=[]\n"
    "E07 code=0 out=[1.2.3\\n] err=[]\n"
    "E08 code=109 out=[] err=[ERROR: ExtrasError: The following arguments were "
    "not expected: a b\\nd\\n\\n\\nprog [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,     "
    "--help              Print this help message and exit\\n]\n"
    "E09 code=113 out=[] err=[prog|OptionNotFound|--w not found\\n]\n"
    "E10 code=107 out=[] err=[--a requires --b\\nRun with --help for more "
    "information.\\n]\n"
    "E11 code=1 out=[] err=[]\n"
    "E12 code=0 out=[d\\n\\n\\nprog [OPTIONS] "
    "[SUBCOMMANDS]\\n\\n\\nOPTIONS:\\n  -h,     --help              Print this "
    "help message and exit\\n          --f                 a "
    "flag\\n\\nSUBCOMMANDS:\\n  s                           sd\\n] err=[]\n"
    "E13 code=0 out=[d\\n\\n\\nprog [OPTIONS] "
    "[SUBCOMMANDS]\\n\\n\\nOPTIONS:\\n  -h,     --help              Print this "
    "help message and exit\\n          --help-all          all\\n          --f "
    "                a flag\\n\\nSUBCOMMANDS:\\ns\\nsd\\n\\n\\nOPTIONS:\\n     "
    "     --g                 g flag\\n\\n] err=[]\n"
    "E14 code=105 out=[] err=[--x: bad value\\nRun with -? for more "
    "information.\\n]\n"
    "E15 code=105 out=[] err=[--x: bad value\\nRun with --usage or -A for more "
    "information.\\n]\n";

// COMMAND_EXIT
//   constant: what the cases of d_tests_arg_parser_command_exits come to, as
// the reference writes them with CLI11.
static const char COMMAND_EXIT[] =
    "F01 code=113 out=[] err=[s|OptionNotFound|--w not found\\n]\n"
    "F02 code=113 out=[] err=[s|OptionNotFound|--w not found\\n]\n"
    "F04 code=106 out=[] err=[--y is required\\n]\n"
    "F05 code=0 out=[sd\\n\\n\\ns [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,     "
    "--help              Print this help message and exit\\n] err=[]\n"
    "F06 code=0 out=[td\\n\\n\\ns t [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,     "
    "--help              Print this help message and exit\\n] err=[]\n"
    "F07 code=0 out=[td\\n\\n\\nprog s t [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,    "
    " --help              Print this help message and exit\\n] err=[]\n"
    "F08 code=106 out=[] err=[--y is required\\n]\n"
    "F09 code=105 out=[] err=[--x: bad value\\nRun with --help or --help-all "
    "for more information.\\n]\n"
    "F10 code=109 out=[] err=[ERROR: ExtrasError: The following argument was "
    "not expected: a\\nsd\\n\\n\\ns [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,     "
    "--help              Print this help message and exit\\n]\n"
    "F11 code=109 out=[] err=[ERROR: ExtrasError: The following argument was "
    "not expected: a\\nsd\\n\\n\\nprog s [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,    "
    " --help              Print this help message and exit\\n]\n"
    "F12 code=0 out=[G:gd\\n\\n\\nUsage: [OPTIONS]\\n\\n\\nOPTIONS:\\n         "
    " --gf                a flag\\n] err=[]\n";

// OWN_SINKS
//   constant: what the cases of d_tests_arg_parser_own_sinks come to, as the
// reference writes them with CLI11.
static const char OWN_SINKS[] =
    "G01 code=105 out=[] err=[--x: bad value\\nRun with --help for more "
    "information.\\n]\n"
    "G02 code=0 out=[d\\n\\n\\nprog [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,     "
    "--help              Print this help message and exit\\n          --f      "
    "           a flag\\n] err=[]\n"
    "G03 code=0 out=[1.2.3\\n] err=[]\n"
    "G04 code=0 out=[sd\\n\\n\\ns [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,     "
    "--help              Print this help message and exit\\n] err=[]\n"
    "G05 code=106 out=[] err=[s|RequiredError|--y is required\\n]\n"
    "G06 code=4 out=[] err=[]\n";

// DEPARTED_FAILURE
//   constant: what the cases of d_tests_arg_parser_failure_lookup come to,
// each error written with the failure message of the nearest command that
// has one, looked up when the error is reported, as d_arg_command_exit
// looks it up. CLI11 copies a parent's failure message into a subcommand
// when the subcommand is added, and a null one writes nothing: its F03
// writes the simple message the subcommand had when it was added, `--w not
// found` and the line naming --help, and its F13 and F14 write nothing
// with the same codes. CLI11's failure message returns its text, so none
// fails, where F15's does and gives way to the simple message (ISSUES 63).
static const char DEPARTED_FAILURE[] =
    "F03 code=113 out=[] err=[s|OptionNotFound|--w not found\\n]\n"
    "F13 code=106 out=[] err=[--y is required\\nRun with --help for more "
    "information.\\n]\n"
    "F14 code=106 out=[] err=[s|RequiredError|--y is required\\n]\n"
    "F15 code=106 out=[] err=[--y is required\\nRun with --help for more "
    "information.\\n]\n";

// ENDINGS
//   constant: what the cases of d_tests_arg_parser_endings come to, as the
// reference writes them with CLI11.
static const char ENDINGS[] =
    "K01 [--help]\n"
    "code=0 out=[d\\n\\n\\nprog [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,     --help  "
    "            Print this help message and exit\\n          --f              "
    "   a flag\\n] err=[]\n"
    "K02 [s --help]\n"
    "code=0 out=[sd\\n\\n\\nprog s [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,     "
    "--help              Print this help message and exit\\n          --g      "
    "           g flag\\n] err=[]\n"
    "K03 [--help-all]\n"
    "code=0 out=[d\\n\\n\\nprog [OPTIONS] [SUBCOMMANDS]\\n\\n\\nOPTIONS:\\n  "
    "-h,     --help              Print this help message and exit\\n          "
    "--help-all          all\\n\\nSUBCOMMANDS:\\ns\\nsd\\n\\n\\nOPTIONS:\\n    "
    "      --g                 g flag\\n\\n] err=[]\n"
    "K04 [--version]\n"
    "code=0 out=[1.2.3\\n] err=[]\n"
    "K05 [a b]\n"
    "code=109 out=[] err=[prog: The following arguments were not expected: a "
    "b\\nRun with --help for more information.\\n]\n"
    "K06 []\n"
    "code=106 out=[] err=[--x is required\\nRun with --help for more "
    "information.\\n]\n"
    "K07 [--x]\n"
    "code=114 out=[] err=[--x: 1 required  missing\\nRun with --help for more "
    "information.\\n]\n"
    "K08 [--f]\n"
    "code=3 out=[] err=[]\n"
    "K09 [--f]\n"
    "code=0 out=[] err=[]\n"
    "K10 [--f]\n"
    "code=0 out=[d\\n\\n\\nprog [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,     --help  "
    "            Print this help message and exit\\n          --f              "
    "   \\n] err=[]\n"
    "K11 [--f]\n"
    "code=0 out=[2.0\\n] err=[]\n"
    "K12 [s]\n"
    "code=0 out=[sd\\n\\n\\nprog s [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,     "
    "--help              Print this help message and exit\\n] err=[]\n"
    "K13 [s a]\n"
    "code=109 out=[] err=[ERROR: ExtrasError: s: The following argument was "
    "not expected: a\\nsd\\n\\n\\nprog s [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,    "
    " --help              Print this help message and exit\\n]\n"
    "K14 [s]\n"
    "code=105 out=[] err=[prog|ValidationError|v: w\\n]\n"
    "K15 [--f]\n"
    "code=0 out=[] err=[]\n";

// MACRO
//   constant: what the cases of d_tests_arg_parser_macro come to, as the
// reference writes them with CLI11.
static const char MACRO[] =
    "M01 [--f]\n"
    "code=42 out=[] err=[]\n"
    "M02 [--help]\n"
    "code=0 out=[d\\n\\n\\nprog [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,     --help  "
    "            Print this help message and exit\\n] err=[]\n"
    "M03 [a]\n"
    "code=109 out=[] err=[prog: The following argument was not expected: "
    "a\\nRun with --help for more information.\\n]\n"
    "M04 [--f]\n"
    "code=5 out=[] err=[]\n"
    "M05 [--version]\n"
    "code=0 out=[1.2.3\\n] err=[]\n";

// ARGUMENTS
//   constant: what the cases of d_tests_arg_parser_arguments come to, as the
// reference writes them with CLI11.
static const char ARGUMENTS[] =
    "Q13 [--o 1 x --f --o=2]\n"
    "cmd app n=1 miss=[x] subs=[] order=[--o|--f|--o]\n"
    "opt f n=1 res=[true]\n"
    "opt o n=2 res=[1|2]\n"
    "Q14 [--o 1 x --f --o=2]\n"
    "cmd app n=1 miss=[x] subs=[] order=[--o|--f|--o]\n"
    "opt f n=1 res=[true]\n"
    "opt o n=2 res=[1|2]\n";

// LINES
//   constant: what the cases of d_tests_arg_parser_lines come to, as the
// reference writes them with CLI11.
static const char LINES[] =
    "L01 [-v -o out f1 f2]\n"
    "name [prog]\n"
    "cmd app n=1 miss=[] subs=[] order=[--verbose|--out|files|files]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt v n=1 res=[true]\n"
    "opt o n=1 res=[out]\n"
    "opt files n=2 res=[f1|f2]\n"
    "opt name n=0 res=[]\n"
    "opt x n=0 res=[]\n"
    "L02 [--out=\"a b\" f]\n"
    "name [prog]\n"
    "cmd app n=1 miss=[] subs=[] order=[--out|files]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt v n=0 res=[]\n"
    "opt o n=1 res=[a b]\n"
    "opt files n=1 res=[f]\n"
    "opt name n=0 res=[]\n"
    "opt x n=0 res=[]\n"
    "L03 [sub --x=\"q r\" f]\n"
    "ERR ExtrasError 109: sub: The following argument was not expected: f\n"
    "name [prog]\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[f] subs=[] order=[--x]\n"
    "opt v n=0 res=[]\n"
    "opt o n=0 res=[]\n"
    "opt files n=0 res=[]\n"
    "opt name n=0 res=[]\n"
    "opt x n=1 res=[q r]\n"
    "L04 [\"bad \\\\q\"]\n"
    "ERR ParseError 111: unrecognized escape sequence \\\\q in bad \\\\q\n"
    "name [prog]\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt v n=0 res=[]\n"
    "opt o n=0 res=[]\n"
    "opt files n=0 res=[]\n"
    "opt name n=0 res=[]\n"
    "opt x n=0 res=[]\n"
    "L05 [/name:\"v w\" f]\n"
    "name [prog]\n"
    "cmd app n=1 miss=[] subs=[] order=[--name|files]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt v n=0 res=[]\n"
    "opt o n=0 res=[]\n"
    "opt files n=1 res=[f]\n"
    "opt name n=1 res=[v w]\n"
    "opt x n=0 res=[]\n"
    "L06 named [\"quoted name\" -v f]\n"
    "name [quoted name]\n"
    "cmd app n=1 miss=[] subs=[] order=[--verbose|files]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt v n=1 res=[true]\n"
    "opt o n=0 res=[]\n"
    "opt files n=1 res=[f]\n"
    "opt name n=0 res=[]\n"
    "opt x n=0 res=[]\n"
    "L07 named [other -v]\n"
    "name [prog]\n"
    "cmd app n=1 miss=[] subs=[] order=[--verbose]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt v n=1 res=[true]\n"
    "opt o n=0 res=[]\n"
    "opt files n=0 res=[]\n"
    "opt name n=0 res=[]\n"
    "opt x n=0 res=[]\n"
    "L08 named []\n"
    "name []\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt v n=0 res=[]\n"
    "opt o n=0 res=[]\n"
    "opt files n=0 res=[]\n"
    "opt name n=0 res=[]\n"
    "opt x n=0 res=[]\n"
    "L09 [--out \"a\\\\0b\" f]\n"
    "name [prog]\n"
    "cmd app n=1 miss=[] subs=[] order=[--out|files]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt v n=0 res=[]\n"
    "opt o n=1 res=[a\\x00b]\n"
    "opt files n=1 res=[f]\n"
    "opt name n=0 res=[]\n"
    "opt x n=0 res=[]\n"
    "L10 [--name=\"x\\\\u0000y\" -v]\n"
    "name [prog]\n"
    "cmd app n=1 miss=[] subs=[] order=[--name|--verbose]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt v n=1 res=[true]\n"
    "opt o n=0 res=[]\n"
    "opt files n=0 res=[]\n"
    "opt name n=1 res=[x\\x00y]\n"
    "opt x n=0 res=[]\n"
    "L11 [sub --x \"p\\\\0q\"]\n"
    "name [prog]\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[] subs=[] order=[--x]\n"
    "opt v n=0 res=[]\n"
    "opt o n=0 res=[]\n"
    "opt files n=0 res=[]\n"
    "opt name n=0 res=[]\n"
    "opt x n=1 res=[p\\x00q]\n"
    "L12 [-o a\\x00b f\\x00g --name=c\\x00d]\n"
    "name [prog]\n"
    "cmd app n=1 miss=[] subs=[] order=[--out|files|--name]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt v n=0 res=[]\n"
    "opt o n=1 res=[a\\x00b]\n"
    "opt files n=1 res=[f\\x00g]\n"
    "opt name n=1 res=[c\\x00d]\n"
    "opt x n=0 res=[]\n"
    "L13 [--verbose\\x00 -v\\x00 -oa\\x00b]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--verbose\n"
    "name [prog]\n"
    "cmd app n=1 miss=[--verbose\\x00] subs=[] order=[--verbose|files|--out]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt v n=1 res=[true]\n"
    "opt o n=1 res=[a\\x00b]\n"
    "opt files n=1 res=[-\\x00]\n"
    "opt name n=0 res=[]\n"
    "opt x n=0 res=[]\n"
    "L14 [/name:v\\x00w \"f\\x00\"]\n"
    "name [prog]\n"
    "cmd app n=1 miss=[] subs=[] order=[--name|files]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt v n=0 res=[]\n"
    "opt o n=0 res=[]\n"
    "opt files n=1 res=[f\\x00]\n"
    "opt name n=1 res=[v\\x00w]\n"
    "opt x n=0 res=[]\n";

// NAMES
//   constant: what the cases of d_tests_arg_parser_names come to, as the
// reference writes them with CLI11.
static const char NAMES[] =
    "N01 argv [prog1]\n"
    "name [prog1]\n"
    "N01 argv [prog2 --f]\n"
    "name [prog2]\n"
    "N02 argv [prog1]\n"
    "name [given]\n"
    "N03 args []\n"
    "name []\n"
    "N03 argv [p3]\n"
    "name [p3]\n"
    "N03 args []\n"
    "name [p3]\n"
    "N04 argv [p4]\n"
    "name [p4]\n"
    "N04 argv [p5]\n"
    "name [fixed]\n"
    "N05 argv [p6]\n"
    "name [p6]\n"
    "N05 argv [p7]\n"
    "name [p7]\n"
    "N06 named line [\"my prog\" -f]\n"
    "name [my prog]\n"
    "N06 named line [other]\n"
    "name [other]\n"
    "N07 args []\n"
    "final app\n"
    "name []\n"
    "N08 argv [tool --help]\n"
    "code=0 out=[d\\n\\n\\ntool [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,     --help  "
    "            Print this help message and exit\\n] err=[]\n";

// STATE
//   constant: what the cases of d_tests_arg_parser_state come to, as the
// reference writes them with CLI11.
static const char STATE[] =
    "T01 [--x 1]\n"
    "cmd app n=1 miss=[] subs=[] order=[--x]\n"
    "opt x n=1 res=[1]\n"
    "T01 clear\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "opt x n=0 res=[]\n"
    "T01 [--x 2]\n"
    "cmd app n=1 miss=[] subs=[] order=[--x]\n"
    "opt x n=1 res=[2]\n"
    "T02 [s --y 1]\n"
    "cmd app n=1 miss=[] subs=[s] order=[]\n"
    "cmd s n=1 miss=[] subs=[] order=[--y]\n"
    "opt y n=1 res=[1]\n"
    "T02 []\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd s n=0 miss=[] subs=[] order=[]\n"
    "opt y n=0 res=[]\n"
    "T03 [a --f]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: a\n"
    "cmd app n=1 miss=[a] subs=[] order=[--f]\n"
    "opt f n=1 res=[true]\n"
    "T03 [--f]\n"
    "cmd app n=1 miss=[] subs=[] order=[--f]\n"
    "opt f n=1 res=[true]\n";

// STATUSES
//   constant: what the cases of d_tests_arg_parser_statuses come to, the
// module's own: CLI11's exit() is given an error, never a status, so
// these have no CLI11 counterpart. A status is described by the error
// the parser recorded when it is that error's status, and otherwise by
// its own text and exit code.
static const char STATUSES[] =
    "X01 code=109 out=[] err=[Extras error\\nRun with --help for more "
    "information.\\n]\n"
    "X02 code=106 out=[] err=[Required error\\nRun with --help for more "
    "information.\\n]\n"
    "X03 code=0 out=[] err=[]\n"
    "X04 code=1 out=[] err=[]\n";

//==============================================================================
// HELPERS
//==============================================================================

/**
 * @brief Adds the line an exit came to, as the reference's `ended` writes
 *        it -- `<tag> code=<exit code> out=[<output>] err=[<error output>]`,
 *        or the same without the tag for an empty one -- each control
 *        character written as its escape; then empties the run's output and
 *        error output for the next.
 *
 * @param[in,out] _run   the run, whose buffers the exit wrote to.
 * @param[in]     _tag   the case's tag; empty for none.
 * @param[in]     _code  the code the exit returned.
 */
static void
d_tests_arg_parser_exited(
    struct d_tests_arg_run* _run,
    const char*             _tag,
    int                     _code
)
{
    (void)d_arg_buffer_printf(&_run->transcript,
                              "%s%scode=%d out=[",
                              _tag,
                              (_tag[0] != '\0') ? " " : "",
                              _code);
    d_tests_arg_render(&_run->transcript,
                       d_arg_buffer_text(&_run->out));
    (void)d_arg_buffer_append_cstr(&_run->transcript,
                                   "] err=[");
    d_tests_arg_render(&_run->transcript,
                       d_arg_buffer_text(&_run->err));
    (void)d_arg_buffer_append_cstr(&_run->transcript,
                                   "]\n");
    d_arg_buffer_clear(&_run->out);
    d_arg_buffer_clear(&_run->err);

    return;
}

/**
 * @brief Ends a parse with a status from the root, into the run's buffers,
 *        as CLI11's exit(error, out, err); then adds the line it came to.
 *
 * @param[in,out] _run     the run.
 * @param[in]     _tag     the case's tag.
 * @param[in]     _status  the status, described by the parser's error when
 *                         it is that error's.
 */
static void
d_tests_arg_parser_end(
    struct d_tests_arg_run* _run,
    const char*             _tag,
    int                     _status
)
{
    const int code = d_arg_parser_exit_to(&_run->parser,
                                          _status,
                                          d_arg_buffer_sink(&_run->out),
                                          d_arg_buffer_sink(&_run->err));

    d_tests_arg_parser_exited(_run,
                              _tag,
                              code);

    return;
}

/**
 * @brief Ends a parse from a command with an error made as CLI11 makes one,
 *        into the run's buffers, as CLI11's exit(error, out, err) on that
 *        command; then adds the line it came to.
 *
 * @param[in,out] _run      the run.
 * @param[in]     _tag      the case's tag.
 * @param[in]     _command  the command the error is reported from.
 * @param[in]     _status   the error's status.
 * @param[in]     _message  its message; NULL for the status's own.
 */
static void
d_tests_arg_parser_end_error(
    struct d_tests_arg_run*     _run,
    const char*                 _tag,
    const struct d_arg_command* _command,
    int                         _status,
    const char*                 _message
)
{
    struct d_arg_error error = D_ARG_ERROR_INIT;

    (void)d_arg_error_set(&error,
                          _status,
                          _command,
                          NULL,
                          _message);

    const int code = d_arg_command_exit_to(_command,
                                           &error,
                                           d_arg_buffer_sink(&_run->out),
                                           d_arg_buffer_sink(&_run->err));

    d_tests_arg_parser_exited(_run,
                              _tag,
                              code);
    d_arg_error_release(&error);

    return;
}

/**
 * @brief Ends a parse with a status from the root, into the parser's own
 *        output and error output, as CLI11's exit(error) into std::cout and
 *        std::cerr; then adds the line it came to.
 *
 * @param[in,out] _run     the run, whose buffers are its parser's own.
 * @param[in]     _tag     the case's tag.
 * @param[in]     _status  the status.
 */
static void
d_tests_arg_parser_own(
    struct d_tests_arg_run* _run,
    const char*             _tag,
    int                     _status
)
{
    const int code = d_arg_parser_exit(&_run->parser,
                                       _status);

    d_tests_arg_parser_exited(_run,
                              _tag,
                              code);

    return;
}

/**
 * @brief Ends a parse from a command with an error made as CLI11 makes one,
 *        into the parser's own output and error output, as CLI11's
 *        exit(error) on that command; then adds the line it came to.
 *
 * @param[in,out] _run      the run, whose buffers are its parser's own.
 * @param[in]     _tag      the case's tag.
 * @param[in]     _command  the command the error is reported from.
 * @param[in]     _status   the error's status.
 * @param[in]     _message  its message; NULL for the status's own.
 */
static void
d_tests_arg_parser_own_error(
    struct d_tests_arg_run*     _run,
    const char*                 _tag,
    const struct d_arg_command* _command,
    int                         _status,
    const char*                 _message
)
{
    struct d_arg_error error = D_ARG_ERROR_INIT;

    (void)d_arg_error_set(&error,
                          _status,
                          _command,
                          NULL,
                          _message);

    const int code = d_arg_command_exit(_command,
                                        &error);

    d_tests_arg_parser_exited(_run,
                              _tag,
                              code);
    d_arg_error_release(&error);

    return;
}

/**
 * @brief The failure message of the reference's `custom`:
 *        `<command's name>|<error's name>|<message>` and a newline.
 *
 * @param[in] _command  the command the error is reported from.
 * @param[in] _error    the error.
 * @param[in] _sink     where the message is written.
 * @param[in] _context  unused.
 * @return the sink's status.
 */
static int
d_tests_arg_parser_custom(
    const struct d_arg_command* _command,
    const struct d_arg_error*   _error,
    struct d_pack_sink          _sink,
    void*                       _context
)
{
    (void)_context;

    return d_arg_sink_printf(_sink,
                             "%s|%s|%s\n",
                             d_arg_command_name(_command),
                             d_arg_status_name(_error->status),
                             d_arg_error_message(_error));
}

/**
 * @brief A failure message that fails, as a function of the caller's that
 *        throws comes to through the face: it writes nothing and reports a
 *        conversion error.
 *
 * @param[in] _command  the command the error is reported from; unused.
 * @param[in] _error    the error; unused.
 * @param[in] _sink     where the message would be written; unused.
 * @param[in] _context  unused.
 * @return D_ARG_ERROR_CONVERSION.
 */
static int
d_tests_arg_parser_failing(
    const struct d_arg_command* _command,
    const struct d_arg_error*   _error,
    struct d_pack_sink          _sink,
    void*                       _context
)
{
    (void)_command;
    (void)_error;
    (void)_sink;
    (void)_context;

    return D_ARG_ERROR_CONVERSION;
}

/**
 * @brief A flag's callback ending the parse as CLI11's RuntimeError(3)
 *        thrown from one.
 *
 * @param[in] _option   the flag.
 * @param[in] _count    its count; unused.
 * @param[in] _context  unused.
 * @return D_ARG_RUNTIME_ERROR.
 */
static int
d_tests_arg_parser_runtime_three(
    struct d_arg_option* _option,
    long                 _count,
    void*                _context
)
{
    (void)_count;
    (void)_context;

    return d_arg_command_fail_runtime(_option->parent,
                                      3);
}

/**
 * @brief A flag's callback ending the parse as CLI11's RuntimeError(5)
 *        thrown from one.
 *
 * @param[in] _option   the flag.
 * @param[in] _count    its count; unused.
 * @param[in] _context  unused.
 * @return D_ARG_RUNTIME_ERROR.
 */
static int
d_tests_arg_parser_runtime_five(
    struct d_arg_option* _option,
    long                 _count,
    void*                _context
)
{
    (void)_count;
    (void)_context;

    return d_arg_command_fail_runtime(_option->parent,
                                      5);
}

/**
 * @brief A flag's callback ending the parse with success, as CLI11's
 *        Success thrown from one.
 *
 * @param[in] _option   the flag; unused.
 * @param[in] _count    its count; unused.
 * @param[in] _context  unused.
 * @return D_ARG_SUCCESS.
 */
static int
d_tests_arg_parser_success(
    struct d_arg_option* _option,
    long                 _count,
    void*                _context
)
{
    (void)_option;
    (void)_count;
    (void)_context;

    return D_ARG_SUCCESS;
}

/**
 * @brief A flag's callback asking for help, as CLI11's CallForHelp thrown
 *        from one.
 *
 * @param[in] _option   the flag; unused.
 * @param[in] _count    its count; unused.
 * @param[in] _context  unused.
 * @return D_ARG_CALL_FOR_HELP.
 */
static int
d_tests_arg_parser_flag_help(
    struct d_arg_option* _option,
    long                 _count,
    void*                _context
)
{
    (void)_option;
    (void)_count;
    (void)_context;

    return D_ARG_CALL_FOR_HELP;
}

/**
 * @brief A flag's callback asking for a version, as CLI11's
 *        CallForVersion("2.0", 0) thrown from one.
 *
 * @param[in] _option   the flag.
 * @param[in] _count    its count; unused.
 * @param[in] _context  unused.
 * @return D_ARG_CALL_FOR_VERSION.
 */
static int
d_tests_arg_parser_version_two(
    struct d_arg_option* _option,
    long                 _count,
    void*                _context
)
{
    (void)_count;
    (void)_context;

    return d_arg_option_fail(_option,
                             D_ARG_CALL_FOR_VERSION,
                             "2.0");
}

/**
 * @brief A final callback asking for help, as CLI11's CallForHelp thrown
 *        from one.
 *
 * @param[in] _command  the command; unused.
 * @param[in] _context  unused.
 * @return D_ARG_CALL_FOR_HELP.
 */
static int
d_tests_arg_parser_command_help(
    struct d_arg_command* _command,
    void*                 _context
)
{
    (void)_command;
    (void)_context;

    return D_ARG_CALL_FOR_HELP;
}

/**
 * @brief A final callback failing as CLI11's ValidationError("v", "w")
 *        thrown from one.
 *
 * @param[in] _command  the command.
 * @param[in] _context  unused.
 * @return D_ARG_ERROR_VALIDATION.
 */
static int
d_tests_arg_parser_invalid(
    struct d_arg_command* _command,
    void*                 _context
)
{
    (void)_context;

    return d_arg_command_fail(_command,
                              D_ARG_ERROR_VALIDATION,
                              "v: w");
}

/**
 * @brief A final callback that notes `final <label>`, the label the one the
 *        run reports the command under.
 *
 * @param[in] _command  the command.
 * @param[in] _context  the case's run.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_parser_final(
    struct d_arg_command* _command,
    void*                 _context
)
{
    d_tests_arg_note(_context,
                     "final %s",
                     d_tests_arg_command_label(_context,
                                               _command));

    return D_ARG_OK;
}

/**
 * @brief Adds the line naming what a step parses, `<tag> <word> [<text>]`,
 *        or `<tag> [<text>]` for an empty word: the text a first word, when
 *        there is one, then the words after it.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _tag    the case's tag.
 * @param[in]     _word   what the step parses -- "argv", "args" -- or empty
 *                        for nothing.
 * @param[in]     _first  the first word, the program's name; NULL for none.
 * @param[in]     _words  the words after it, separated by spaces.
 */
static void
d_tests_arg_parser_heading(
    struct d_tests_arg_run* _run,
    const char*             _tag,
    const char*             _word,
    const char*             _first,
    const char*             _words
)
{
    const bool spaced = ( (_first != NULL)    &&
                          (_words[0] != '\0') );

    (void)d_arg_buffer_printf(&_run->transcript,
                              "%s %s%s[%s%s%s]\n",
                              _tag,
                              _word,
                              (_word[0] != '\0') ? " " : "",
                              (_first != NULL) ? _first : "",
                              (spaced) ? " " : "",
                              _words);

    return;
}

/**
 * @brief Parses main's arguments, built in storage the run keeps, adding
 *        nothing to the transcript.
 *
 * @param[in,out] _run      the run.
 * @param[in]     _program  the program's name, `argv[0]`.
 * @param[in]     _words    the arguments after it, separated by spaces.
 * @return the parse's status; D_ARG_ERROR_BAD_ARGUMENT, the run failed, for
 *         arguments that could not be built.
 */
static int
d_tests_arg_parser_parse_argv(
    struct d_tests_arg_run* _run,
    const char*             _program,
    const char*             _words
)
{
    int                argc = 0;
    char* const* const argv = d_tests_arg_argv(_run,
                                               _program,
                                               _words,
                                               &argc);

    return (argv != NULL)
           ? d_arg_parser_parse(&_run->parser,
                                argc,
                                argv)
           : (int)D_ARG_ERROR_BAD_ARGUMENT;
}

/**
 * @brief Parses main's arguments as the reference's parse_main: the parse's
 *        ERR line when it fails, and nothing more.
 *
 * @param[in,out] _run      the run.
 * @param[in]     _program  the program's name, `argv[0]`.
 * @param[in]     _words    the arguments after it, separated by spaces.
 */
static void
d_tests_arg_parser_parse_main(
    struct d_tests_arg_run* _run,
    const char*             _program,
    const char*             _words
)
{
    const int status = d_tests_arg_parser_parse_argv(_run,
                                                     _program,
                                                     _words);

    d_tests_arg_record_error(_run,
                             status);

    return;
}

/**
 * @brief Parses main's arguments and ends the parse as the reference ends
 *        one that threw: d_arg_parser_exit_to into the run's buffers when
 *        the parse did not go on, as CLI11's exit(error, out, err) after a
 *        caught ParseError; then the line it came to, its code 0 and nothing
 *        written when the parse went on.
 *
 * @param[in,out] _run      the run.
 * @param[in]     _program  the program's name, `argv[0]`.
 * @param[in]     _words    the arguments after it, separated by spaces.
 */
static void
d_tests_arg_parser_parse_exit(
    struct d_tests_arg_run* _run,
    const char*             _program,
    const char*             _words
)
{
    const int status = d_tests_arg_parser_parse_argv(_run,
                                                     _program,
                                                     _words);
    const int code   = (status != (int)D_ARG_OK)
                       ? d_arg_parser_exit_to(&_run->parser,
                                              status,
                                              d_arg_buffer_sink(&_run->out),
                                              d_arg_buffer_sink(&_run->err))
                       : 0;

    d_tests_arg_parser_exited(_run,
                              "",
                              code);

    return;
}

/**
 * @brief Adds `name [<the root's name>]`.
 *
 * @param[in,out] _run  the run.
 */
static void
d_tests_arg_parser_name(
    struct d_tests_arg_run* _run
)
{
    d_tests_arg_note(_run,
                     "name [%s]",
                     d_arg_command_name(&_run->parser.command));

    return;
}

/**
 * @brief Parses main's arguments as the names' cases do: the line naming
 *        them, `argv` and the program's name first; the parse's ERR line
 *        when it fails; and the root's name.
 *
 * @param[in,out] _run      the run.
 * @param[in]     _tag      the case's tag.
 * @param[in]     _program  the program's name, `argv[0]`.
 * @param[in]     _words    the arguments after it, separated by spaces.
 */
static void
d_tests_arg_parser_named_main(
    struct d_tests_arg_run* _run,
    const char*             _tag,
    const char*             _program,
    const char*             _words
)
{
    d_tests_arg_parser_heading(_run,
                               _tag,
                               "argv",
                               _program,
                               _words);
    d_tests_arg_parser_parse_main(_run,
                                  _program,
                                  _words);
    d_tests_arg_parser_name(_run);

    return;
}

/**
 * @brief Parses no arguments, as CLI11's parse of an empty vector, as the
 *        names' cases do: the line naming them, the parse's ERR line when it
 *        fails, and the root's name.
 *
 * @param[in,out] _run  the run.
 * @param[in]     _tag  the case's tag.
 */
static void
d_tests_arg_parser_named_vector(
    struct d_tests_arg_run* _run,
    const char*             _tag
)
{
    d_tests_arg_parser_heading(_run,
                               _tag,
                               "args",
                               NULL,
                               "");

    const int status = d_arg_parser_parse_args(&_run->parser,
                                               NULL,
                                               0u,
                                               0u);

    d_tests_arg_record_error(_run,
                             status);
    d_tests_arg_parser_name(_run);

    return;
}

/**
 * @brief Parses main's arguments and ends the parse, as the names' last case
 *        does: the line naming them, `argv` and the program's name first,
 *        then the line the ending came to.
 *
 * @param[in,out] _run      the run.
 * @param[in]     _tag      the case's tag.
 * @param[in]     _program  the program's name, `argv[0]`.
 * @param[in]     _words    the arguments after it, separated by spaces.
 */
static void
d_tests_arg_parser_named_ending(
    struct d_tests_arg_run* _run,
    const char*             _tag,
    const char*             _program,
    const char*             _words
)
{
    d_tests_arg_parser_heading(_run,
                               _tag,
                               "argv",
                               _program,
                               _words);
    d_tests_arg_parser_parse_exit(_run,
                                  _program,
                                  _words);

    return;
}

/**
 * @brief Parses a command line held in one string, as the lines' and the
 *        names' cases do: the line naming it -- `<tag> [<line>]`, or `<tag>
 *        <word> [<line>]`, the line escaped -- the parse's ERR line when it
 *        fails, and the root's name.
 *
 * @param[in,out] _run     the run.
 * @param[in]     _tag     the case's tag.
 * @param[in]     _word    what the naming line calls the parse -- "named",
 *                         "named line" -- for a line starting with the
 *                         program's name, parsed as one; empty for a line
 *                         without it.
 * @param[in]     _line    the command line.
 * @param[in]     _length  how many bytes it holds, NUL bytes among them.
 */
static void
d_tests_arg_parser_line(
    struct d_tests_arg_run* _run,
    const char*             _tag,
    const char*             _word,
    const char*             _line,
    size_t                  _length
)
{
    const struct d_pack_text line = d_pack_text_from_span(_line,
                                                          _length);

    (void)d_arg_buffer_printf(&_run->transcript,
                              "%s %s%s[",
                              _tag,
                              _word,
                              (_word[0] != '\0') ? " " : "");
    d_tests_arg_escape(&_run->transcript,
                       line);
    (void)d_arg_buffer_append_cstr(&_run->transcript,
                                   "]\n");

    const int status = d_arg_parser_parse_line(&_run->parser,
                                               line,
                                               (_word[0] != '\0'));

    d_tests_arg_record_error(_run,
                             status);
    d_tests_arg_parser_name(_run);

    return;
}

/**
 * @brief Makes the options and the subcommand each of the lines' cases is
 *        parsed with, as the reference's TEXT_APP makes them.
 *
 * @param[in,out] _run      the case's run.
 * @param[in,out] _app      the run's root.
 * @param[in]     _windows  whether the root takes Windows-style options.
 */
static void
d_tests_arg_parser_text_app(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app,
    bool                    _windows
)
{
    d_arg_command_set_windows_style(_app,
                                    _windows);
    (void)d_tests_arg_make_flag(_run,
                                "v",
                                _app,
                                "-v,--verbose");
    (void)d_tests_arg_make_option(_run,
                                  "o",
                                  _app,
                                  "-o,--out");

    struct d_arg_option* const files = d_tests_arg_make_all(_run,
                                                            "files",
                                                            _app,
                                                            "files");

    d_arg_option_set_expected(files,
                              -1);
    (void)d_tests_arg_make_option(_run,
                                  "name",
                                  _app,
                                  "--name");

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");

    return;
}

/**
 * @brief Parses main's arguments, "prog" first, and ends the parse, as the
 *        endings' cases do: the line naming them, then the line the ending
 *        came to.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _tag    the case's tag.
 * @param[in]     _words  the arguments, separated by spaces.
 */
static void
d_tests_arg_parser_ending(
    struct d_tests_arg_run* _run,
    const char*             _tag,
    const char*             _words
)
{
    d_tests_arg_parser_heading(_run,
                               _tag,
                               "",
                               NULL,
                               _words);
    d_tests_arg_parser_parse_exit(_run,
                                  "prog",
                                  _words);

    return;
}

/**
 * @brief A function written as main is, taking its arguments as
 *        d_tests_arg_argv builds them: D_ARG_PARSE, then 42 when the parse
 *        goes on.
 *
 * @param[in,out] _parser  the parser.
 * @param[in]     _argc    the argument count.
 * @param[in]     _argv    the arguments.
 * @return what D_ARG_PARSE returns when the parse ends the program; 42
 *         otherwise.
 */
static int
d_tests_arg_parser_program(
    struct d_arg_parser* _parser,
    int                  _argc,
    char* const*         _argv
)
{
    D_ARG_PARSE(_parser,
                _argc,
                _argv);

    return 42;
}

/**
 * @brief Parses main's arguments, "prog" first, with D_ARG_PARSE in a
 *        function written as main is, as the macro's cases do: the line
 *        naming them, then the line the function's return came to, with what
 *        D_ARG_PARSE wrote into the parser's own buffers.
 *
 * @param[in,out] _run    the run, whose buffers are its parser's own.
 * @param[in]     _tag    the case's tag.
 * @param[in]     _words  the arguments, separated by spaces.
 */
static void
d_tests_arg_parser_macro_ending(
    struct d_tests_arg_run* _run,
    const char*             _tag,
    const char*             _words
)
{
    d_tests_arg_parser_heading(_run,
                               _tag,
                               "",
                               NULL,
                               _words);

    int                argc = 0;
    char* const* const argv = d_tests_arg_argv(_run,
                                               "prog",
                                               _words,
                                               &argc);

    // arguments that could not be built have failed the run
    if (!argv)
    {
        return;
    }

    const int code = d_tests_arg_parser_program(&_run->parser,
                                                argc,
                                                argv);

    d_tests_arg_parser_exited(_run,
                              "",
                              code);

    return;
}

/**
 * @brief Parses main's arguments, "prog" first, as the state's cases do: a
 *        line naming them, then the report.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _tag    the case's tag.
 * @param[in]     _words  the arguments, separated by spaces.
 */
static void
d_tests_arg_parser_step(
    struct d_tests_arg_run* _run,
    const char*             _tag,
    const char*             _words
)
{
    d_tests_arg_parser_heading(_run,
                               _tag,
                               "",
                               NULL,
                               _words);
    d_tests_arg_parser_parse_main(_run,
                                  "prog",
                                  _words);
    d_tests_arg_report(_run);

    return;
}

// d_tests_arg_parser_alias
//   struct: what a visit of a vector parse's extras gathers: the arguments
// the parse was given, how many, and how many extras are those arguments
// themselves -- starting where one does -- and how many are not.
struct d_tests_arg_parser_alias
{
    char* const* argv;
    size_t       count;
    size_t       borrowed;
    size_t       copied;
};

/**
 * @brief Counts an extra of a vector parse as one of the arguments given,
 *        or as a copy of one.
 *
 * @param[in]     _extra    the extra.
 * @param[in,out] _context  the d_tests_arg_parser_alias gathering them.
 * @return D_ARG_OK, to visit every one.
 */
static int
d_tests_arg_parser_alias_visit(
    const struct d_arg_extra* _extra,
    void*                     _context
)
{
    struct d_tests_arg_parser_alias* const alias = _context;
    bool                                   found = false;

    // one of the arguments when it starts where one does
    for (size_t index = 0u;
         (!found) && (index < alias->count);
         ++index)
    {
        found = (_extra->text.data == alias->argv[index]);
    }

    alias->borrowed += (found) ? 1u : 0u;
    alias->copied   += (found) ? 0u : 1u;

    return D_ARG_OK;
}

/**
 * @brief Holds a vector parse to what its flags asked of its arguments: the
 *        run failed unless its extras are the arguments themselves,
 *        borrowed, or, under D_ARG_ARGS_COPY, copies of them -- which no
 *        transcript shows, a copy reading as its argument does; and, copied,
 *        the storage the arguments were built in overwritten, which the
 *        report then shows the parse did not borrow.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _argv   the arguments the parse was given.
 * @param[in]     _count  how many.
 * @param[in]     _flags  the parse's flags.
 */
static void
d_tests_arg_parser_vector_kept(
    struct d_tests_arg_run* _run,
    char* const*            _argv,
    size_t                  _count,
    uint32_t                _flags
)
{
    struct d_tests_arg_parser_alias alias  = { _argv,
                                               _count,
                                               0u,
                                               0u };
    const int                       status = d_arg_command_each_remaining(
                                                 &_run->parser.command,
                                                 false,
                                                 d_tests_arg_parser_alias_visit,
                                                 &alias);
    const bool                      copy   =
        ((_flags & D_ARG_ARGS_COPY) != 0u);

    // each extra borrowed, or each copied, as asked
    if ( (status != D_ARG_OK)      ||
         ( (copy) &&
           (alias.borrowed > 0u) ) ||
         ( (!copy) &&
           (alias.copied > 0u) ) )
    {
        _run->failed = true;
    }

    // each argument a copying parse was given overwritten
    for (size_t index = 0u;
         (copy) && (index < _count);
         ++index)
    {
        memset(_argv[index],
               '#',
               strlen(_argv[index]));
    }

    return;
}

/**
 * @brief Parses a vector of arguments as the arguments' cases do: a line
 *        naming them; their spans given in reverse for D_ARG_ARGS_REVERSED,
 *        as CLI11's vector holds them; what the flags asked of them held
 *        (d_tests_arg_parser_vector_kept); and the report.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _tag    the case's tag.
 * @param[in]     _words  the arguments, first to last, separated by spaces.
 * @param[in]     _flags  the parse's flags.
 */
static void
d_tests_arg_parser_vector(
    struct d_tests_arg_run* _run,
    const char*             _tag,
    const char*             _words,
    uint32_t                _flags
)
{
    d_tests_arg_parser_heading(_run,
                               _tag,
                               "",
                               NULL,
                               _words);

    int                argc = 0;
    char* const* const argv = d_tests_arg_argv(_run,
                                               NULL,
                                               _words,
                                               &argc);

    // arguments that could not be built have failed the run
    if (!argv)
    {
        return;
    }

    struct d_pack_text texts[D_TESTS_ARG_ARGS_MAX] = { { NULL, 0u } };
    const size_t       count    = (size_t)argc;
    const bool         reversed = ((_flags & D_ARG_ARGS_REVERSED) != 0u);

    // each argument's span, the last first when reversed
    for (size_t index = 0u; index < count; ++index)
    {
        texts[(reversed) ? (count - 1u - index) : index] =
            d_pack_text_from_cstr(argv[index]);
    }

    const int status = d_arg_parser_parse_args(&_run->parser,
                                               texts,
                                               count,
                                               _flags);

    d_tests_arg_parser_vector_kept(_run,
                                   argv,
                                   count,
                                   _flags);
    d_tests_arg_record(_run,
                       status);

    return;
}

//==============================================================================
// 1.  EXITS
//==============================================================================

/**
 * @brief E01: a validation error, the root with its help flag.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_e01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    const int status = d_arg_command_fail(_app,
                                          D_ARG_ERROR_VALIDATION,
                                          "--x: bad value");

    d_tests_arg_parser_end(_run,
                           "E01",
                           status);

    return;
}

/**
 * @brief E02: a validation error, the root with both help flags.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_e02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_help_all_flag(_app,
                                          "--help-all",
                                          "all");

    const int status = d_arg_command_fail(_app,
                                          D_ARG_ERROR_VALIDATION,
                                          "--x: bad value");

    d_tests_arg_parser_end(_run,
                           "E02",
                           status);

    return;
}

/**
 * @brief E03: a required error, the root with its help-all flag alone.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_e03(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_help_all_flag(_app,
                                          "-a,--help-all",
                                          "all");
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);

    const int status = d_arg_command_fail(_app,
                                          D_ARG_ERROR_REQUIRED,
                                          "--y is required");

    d_tests_arg_parser_end(_run,
                           "E03",
                           status);

    return;
}

/**
 * @brief E04: a conversion error, the root without a help flag.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_e04(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);

    const int status =
        d_arg_command_fail(_app,
                           D_ARG_ERROR_CONVERSION,
                           "The value --z is not an allowed value for abc");

    d_tests_arg_parser_end(_run,
                           "E04",
                           status);

    return;
}

/**
 * @brief E05: a runtime error of code 3.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_e05(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    const int status = d_arg_command_fail_runtime(_app,
                                                  3);

    d_tests_arg_parser_end(_run,
                           "E05",
                           status);

    return;
}

/**
 * @brief E06: a success.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_e06(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    const int status = d_arg_command_fail(_app,
                                          D_ARG_SUCCESS,
                                          NULL);

    d_tests_arg_parser_end(_run,
                           "E06",
                           status);

    return;
}

/**
 * @brief E07: a version asked for.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_e07(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    const int status = d_arg_command_fail(_app,
                                          D_ARG_CALL_FOR_VERSION,
                                          "1.2.3");

    d_tests_arg_parser_end(_run,
                           "E07",
                           status);

    return;
}

/**
 * @brief E08: an extras error under the help failure message.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_e08(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_failure_message(_app,
                                      d_arg_failure_help,
                                      NULL,
                                      NULL);

    const int status =
        d_arg_command_fail(_app,
                           D_ARG_ERROR_EXTRAS,
                           "The following arguments were not expected: a b");

    d_tests_arg_parser_end(_run,
                           "E08",
                           status);

    return;
}

/**
 * @brief E09: an error under a failure message of the root's own.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_e09(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_failure_message(_app,
                                      d_tests_arg_parser_custom,
                                      NULL,
                                      NULL);

    const int status = d_arg_command_fail(_app,
                                          D_ARG_ERROR_OPTION_NOT_FOUND,
                                          "--w not found");

    d_tests_arg_parser_end(_run,
                           "E09",
                           status);

    return;
}

/**
 * @brief E10: a requires error, the root with a subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_e10(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_add_subcommand(_app,
                                       "s",
                                       NULL);

    const int status = d_arg_command_fail(_app,
                                          D_ARG_ERROR_REQUIRES,
                                          "--a requires --b");

    d_tests_arg_parser_end(_run,
                           "E10",
                           status);

    return;
}

/**
 * @brief E11: a runtime error of no code of its own.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_e11(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    const int status = d_arg_command_fail(_app,
                                          D_ARG_RUNTIME_ERROR,
                                          NULL);

    d_tests_arg_parser_end(_run,
                           "E11",
                           status);

    return;
}

/**
 * @brief E12: help asked for, the root with a flag and a subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_e12(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_add_flag(_app,
                                 "--f",
                                 "a flag");
    (void)d_arg_command_add_subcommand(_app,
                                       "s",
                                       "sd");
    d_tests_arg_parser_end(_run,
                           "E12",
                           D_ARG_CALL_FOR_HELP);

    return;
}

/**
 * @brief E13: help of everything asked for.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_e13(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_help_all_flag(_app,
                                          "--help-all",
                                          "all");
    (void)d_arg_command_add_flag(_app,
                                 "--f",
                                 "a flag");

    struct d_arg_command* const s = d_arg_command_add_subcommand(_app,
                                                                 "s",
                                                                 "sd");

    (void)d_arg_command_add_flag(s,
                                 "--g",
                                 "g flag");
    d_tests_arg_parser_end(_run,
                           "E13",
                           D_ARG_CALL_FOR_ALL_HELP);

    return;
}

/**
 * @brief E14: a validation error, the help flag of another name.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_e14(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_help_flag(_app,
                                      "-?",
                                      "q");

    const int status = d_arg_command_fail(_app,
                                          D_ARG_ERROR_VALIDATION,
                                          "--x: bad value");

    d_tests_arg_parser_end(_run,
                           "E14",
                           status);

    return;
}

/**
 * @brief E15: a validation error, both help flags of other names.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_e15(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_help_flag(_app,
                                      "-?,--usage",
                                      "q");
    (void)d_arg_command_set_help_all_flag(_app,
                                          "-A",
                                          "all");

    const int status = d_arg_command_fail(_app,
                                          D_ARG_ERROR_VALIDATION,
                                          "--x: bad value");

    d_tests_arg_parser_end(_run,
                           "E15",
                           status);

    return;
}

// EXIT_STEPS
//   constant: the cases of d_tests_arg_parser_exits.
static const d_tests_arg_define_fn EXIT_STEPS[] =
{
    d_tests_arg_parser_e01,
    d_tests_arg_parser_e02,
    d_tests_arg_parser_e03,
    d_tests_arg_parser_e04,
    d_tests_arg_parser_e05,
    d_tests_arg_parser_e06,
    d_tests_arg_parser_e07,
    d_tests_arg_parser_e08,
    d_tests_arg_parser_e09,
    d_tests_arg_parser_e10,
    d_tests_arg_parser_e11,
    d_tests_arg_parser_e12,
    d_tests_arg_parser_e13,
    d_tests_arg_parser_e14,
    d_tests_arg_parser_e15
};

// EXIT_COUNT
//   constant: how many cases EXIT_STEPS holds.
static const size_t EXIT_COUNT = sizeof(EXIT_STEPS) / sizeof(EXIT_STEPS[0]);

/*
d_tests_arg_parser_exits
  Ending a parse from the root, as CLI11's exit(error, out, err) ends one: the
code, and what is written where.
  Tests the following:
  - an error's message, and the line naming the help flags the root has: --help,
    both help flags, the help-all flag alone, none, and flags of other names
  - a runtime error's code, a success's and a version's, the version written
  - the help failure message and a failure message of the root's own
  - help and help of everything, with an option and a subcommand
*/
bool
d_tests_arg_parser_exits(void)
{
    return d_tests_arg_steps("EXIT",
                             EXIT_STEPS,
                             EXIT_COUNT,
                             EXIT);
}

//==============================================================================
// 2.  COMMAND EXITS
//==============================================================================

/**
 * @brief F01: a subcommand's own failure message.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_f01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const s = d_arg_command_add_subcommand(_app,
                                                                 "s",
                                                                 NULL);

    d_arg_command_set_failure_message(s,
                                      d_tests_arg_parser_custom,
                                      NULL,
                                      NULL);
    d_tests_arg_parser_end_error(_run,
                                 "F01",
                                 s,
                                 D_ARG_ERROR_OPTION_NOT_FOUND,
                                 "--w not found");

    return;
}

/**
 * @brief F02: a failure message the subcommand takes from its parent.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_f02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_failure_message(_app,
                                      d_tests_arg_parser_custom,
                                      NULL,
                                      NULL);

    struct d_arg_command* const s = d_arg_command_add_subcommand(_app,
                                                                 "s",
                                                                 NULL);

    d_tests_arg_parser_end_error(_run,
                                 "F02",
                                 s,
                                 D_ARG_ERROR_OPTION_NOT_FOUND,
                                 "--w not found");

    return;
}

/**
 * @brief F04: a required error from a subcommand without a help flag.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_f04(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const s = d_arg_command_add_subcommand(_app,
                                                                 "s",
                                                                 NULL);

    (void)d_arg_command_set_help_flag(s,
                                      NULL,
                                      NULL);
    d_tests_arg_parser_end_error(_run,
                                 "F04",
                                 s,
                                 D_ARG_ERROR_REQUIRED,
                                 "--y is required");

    return;
}

/**
 * @brief F05: a subcommand's help.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_f05(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const s = d_arg_command_add_subcommand(_app,
                                                                 "s",
                                                                 "sd");

    d_tests_arg_parser_end_error(_run,
                                 "F05",
                                 s,
                                 D_ARG_CALL_FOR_HELP,
                                 NULL);

    return;
}

/**
 * @brief F06: help from a subcommand after a parse that used a deeper one,
 *        then, as F07, from the root.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_f06(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const s = d_arg_command_add_subcommand(_app,
                                                                 "s",
                                                                 "sd");

    (void)d_arg_command_add_subcommand(s,
                                       "t",
                                       "td");
    d_tests_arg_parser_parse_main(_run,
                                  "prog",
                                  "s t");
    d_tests_arg_parser_end_error(_run,
                                 "F06",
                                 s,
                                 D_ARG_CALL_FOR_HELP,
                                 NULL);
    d_tests_arg_parser_end_error(_run,
                                 "F07",
                                 _app,
                                 D_ARG_CALL_FOR_HELP,
                                 NULL);

    return;
}

/**
 * @brief F08: a required error from an option group.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_f08(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const s = d_arg_command_add_subcommand(_app,
                                                                 "s",
                                                                 NULL);
    struct d_arg_command* const g = d_arg_command_add_option_group(s,
                                                                   "G",
                                                                   NULL);

    d_tests_arg_parser_end_error(_run,
                                 "F08",
                                 g,
                                 D_ARG_ERROR_REQUIRED,
                                 "--y is required");

    return;
}

/**
 * @brief F09: a validation error from a subcommand whose parent has both
 *        help flags.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_f09(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_help_all_flag(_app,
                                          "--help-all",
                                          "all");

    struct d_arg_command* const s = d_arg_command_add_subcommand(_app,
                                                                 "s",
                                                                 NULL);

    d_tests_arg_parser_end_error(_run,
                                 "F09",
                                 s,
                                 D_ARG_ERROR_VALIDATION,
                                 "--x: bad value");

    return;
}

/**
 * @brief F10: an extras error under the help failure message, from a
 *        subcommand after a parse that used it, then, as F11, from the
 *        root.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_f10(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_failure_message(_app,
                                      d_arg_failure_help,
                                      NULL,
                                      NULL);

    struct d_arg_command* const s = d_arg_command_add_subcommand(_app,
                                                                 "s",
                                                                 "sd");

    d_tests_arg_parser_parse_main(_run,
                                  "prog",
                                  "s");
    d_tests_arg_parser_end_error(_run,
                                 "F10",
                                 s,
                                 D_ARG_ERROR_EXTRAS,
                                 "The following argument was not expected: a");
    d_tests_arg_parser_end_error(_run,
                                 "F11",
                                 _app,
                                 D_ARG_ERROR_EXTRAS,
                                 "The following argument was not expected: a");

    return;
}

/**
 * @brief F12: an option group's help.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_f12(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const g = d_arg_command_add_option_group(_app,
                                                                   "G",
                                                                   "gd");

    (void)d_arg_command_add_flag(g,
                                 "--gf",
                                 "a flag");
    d_tests_arg_parser_end_error(_run,
                                 "F12",
                                 g,
                                 D_ARG_CALL_FOR_HELP,
                                 NULL);

    return;
}

// COMMAND_EXIT_STEPS
//   constant: the cases of d_tests_arg_parser_command_exits.
static const d_tests_arg_define_fn COMMAND_EXIT_STEPS[] =
{
    d_tests_arg_parser_f01,
    d_tests_arg_parser_f02,
    d_tests_arg_parser_f04,
    d_tests_arg_parser_f05,
    d_tests_arg_parser_f06,
    d_tests_arg_parser_f08,
    d_tests_arg_parser_f09,
    d_tests_arg_parser_f10,
    d_tests_arg_parser_f12
};

// COMMAND_EXIT_COUNT
//   constant: how many cases COMMAND_EXIT_STEPS holds.
static const size_t COMMAND_EXIT_COUNT =
    sizeof(COMMAND_EXIT_STEPS) / sizeof(COMMAND_EXIT_STEPS[0]);

/*
d_tests_arg_parser_command_exits
  Ending a parse from a subcommand or an option group, as CLI11's exit(error,
out, err) called on one: d_arg_command_exit_to.
  Tests the following:
  - a failure message of the subcommand's own, and one it takes from its parent
  - a subcommand without a help flag, and an option group, which has none
  - a subcommand's help, before a parse and after one that used a deeper
    subcommand, from it and from the root
  - the line naming the help flags a subcommand takes from its parent
  - the help failure message from a subcommand and the root after a parse that
    used the subcommand
  - an option group's help
*/
bool
d_tests_arg_parser_command_exits(void)
{
    return d_tests_arg_steps("COMMAND_EXIT",
                             COMMAND_EXIT_STEPS,
                             COMMAND_EXIT_COUNT,
                             COMMAND_EXIT);
}

//==============================================================================
// 3.  OWN SINKS
//==============================================================================

/**
 * @brief G01: a validation error from the root.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_g01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    const int status = d_arg_command_fail(_app,
                                          D_ARG_ERROR_VALIDATION,
                                          "--x: bad value");

    d_tests_arg_parser_own(_run,
                           "G01",
                           status);

    return;
}

/**
 * @brief G02: help asked of the root.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_g02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_add_flag(_app,
                                 "--f",
                                 "a flag");
    d_tests_arg_parser_own(_run,
                           "G02",
                           D_ARG_CALL_FOR_HELP);

    return;
}

/**
 * @brief G03: a version asked for.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_g03(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    const int status = d_arg_command_fail(_app,
                                          D_ARG_CALL_FOR_VERSION,
                                          "1.2.3");

    d_tests_arg_parser_own(_run,
                           "G03",
                           status);

    return;
}

/**
 * @brief G04: help asked of a subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_g04(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const s = d_arg_command_add_subcommand(_app,
                                                                 "s",
                                                                 "sd");

    d_tests_arg_parser_own_error(_run,
                                 "G04",
                                 s,
                                 D_ARG_CALL_FOR_HELP,
                                 NULL);

    return;
}

/**
 * @brief G05: a required error from a subcommand, under its parent's
 *        failure message.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_g05(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_failure_message(_app,
                                      d_tests_arg_parser_custom,
                                      NULL,
                                      NULL);

    struct d_arg_command* const s = d_arg_command_add_subcommand(_app,
                                                                 "s",
                                                                 NULL);

    d_tests_arg_parser_own_error(_run,
                                 "G05",
                                 s,
                                 D_ARG_ERROR_REQUIRED,
                                 "--y is required");

    return;
}

/**
 * @brief G06: a runtime error of code 4 recorded from a subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_g06(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const s = d_arg_command_add_subcommand(_app,
                                                                 "s",
                                                                 NULL);

    (void)d_arg_command_fail_runtime(s,
                                     4);

    const int code = d_arg_command_exit(s,
                                        d_arg_parser_error(&_run->parser));

    d_tests_arg_parser_exited(_run,
                              "G06",
                              code);

    return;
}

// OWN_SINK_STEPS
//   constant: the cases of d_tests_arg_parser_own_sinks.
static const d_tests_arg_define_fn OWN_SINK_STEPS[] =
{
    d_tests_arg_parser_g01,
    d_tests_arg_parser_g02,
    d_tests_arg_parser_g03,
    d_tests_arg_parser_g04,
    d_tests_arg_parser_g05,
    d_tests_arg_parser_g06
};

// OWN_SINK_COUNT
//   constant: how many cases OWN_SINK_STEPS holds.
static const size_t OWN_SINK_COUNT =
    sizeof(OWN_SINK_STEPS) / sizeof(OWN_SINK_STEPS[0]);

/*
d_tests_arg_parser_own_sinks
  Ending a parse into the parser's own output and error output, as CLI11's
exit(error) into std::cout and std::cerr: d_arg_parser_exit and
d_arg_command_exit.
  Tests the following:
  - an error's failure message to the error output
  - help and a version to the output, from the root and a subcommand
  - a failure message a subcommand takes from its parent
  - the code of a runtime error the parser recorded from a subcommand
*/
bool
d_tests_arg_parser_own_sinks(void)
{
    return d_tests_arg_steps("OWN_SINKS",
                             OWN_SINK_STEPS,
                             OWN_SINK_COUNT,
                             OWN_SINKS);
}

//==============================================================================
// 4.  FAILURE LOOKUP
//==============================================================================

/**
 * @brief F03: a failure message set on the parent after the subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_f03(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const s = d_arg_command_add_subcommand(_app,
                                                                 "s",
                                                                 NULL);

    d_arg_command_set_failure_message(_app,
                                      d_tests_arg_parser_custom,
                                      NULL,
                                      NULL);
    d_tests_arg_parser_end_error(_run,
                                 "F03",
                                 s,
                                 D_ARG_ERROR_OPTION_NOT_FOUND,
                                 "--w not found");

    return;
}

/**
 * @brief F13: no failure message on the root.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_f13(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_failure_message(_app,
                                      NULL,
                                      NULL,
                                      NULL);
    d_tests_arg_parser_end_error(_run,
                                 "F13",
                                 _app,
                                 D_ARG_ERROR_REQUIRED,
                                 "--y is required");

    return;
}

/**
 * @brief F14: no failure message on a subcommand whose parent has one.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_f14(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_failure_message(_app,
                                      d_tests_arg_parser_custom,
                                      NULL,
                                      NULL);

    struct d_arg_command* const s = d_arg_command_add_subcommand(_app,
                                                                 "s",
                                                                 NULL);

    d_arg_command_set_failure_message(s,
                                      NULL,
                                      NULL,
                                      NULL);
    d_tests_arg_parser_end_error(_run,
                                 "F14",
                                 s,
                                 D_ARG_ERROR_REQUIRED,
                                 "--y is required");

    return;
}

/**
 * @brief F15: a failure message that fails, which gives way to the simple
 *        message.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_f15(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_failure_message(_app,
                                      d_tests_arg_parser_failing,
                                      NULL,
                                      NULL);
    d_tests_arg_parser_end_error(_run,
                                 "F15",
                                 _app,
                                 D_ARG_ERROR_REQUIRED,
                                 "--y is required");

    return;
}

// FAILURE_LOOKUP_STEPS
//   constant: the cases of d_tests_arg_parser_failure_lookup.
static const d_tests_arg_define_fn FAILURE_LOOKUP_STEPS[] =
{
    d_tests_arg_parser_f03,
    d_tests_arg_parser_f13,
    d_tests_arg_parser_f14,
    d_tests_arg_parser_f15
};

// FAILURE_LOOKUP_COUNT
//   constant: how many cases FAILURE_LOOKUP_STEPS holds.
static const size_t FAILURE_LOOKUP_COUNT =
    sizeof(FAILURE_LOOKUP_STEPS) / sizeof(FAILURE_LOOKUP_STEPS[0]);

/*
d_tests_arg_parser_failure_lookup
  Which failure message an error is written with: the nearest command's, looked
up when the error is reported.
  Tests the following:
  - a failure message set on the parent after the subcommand was made
  - none set on the root, which is d_arg_failure_simple
  - none set on a subcommand, which is its parent's
  - one that fails, which gives way to d_arg_failure_simple
*/
bool
d_tests_arg_parser_failure_lookup(void)
{
    return d_tests_arg_steps("DEPARTED_FAILURE",
                             FAILURE_LOOKUP_STEPS,
                             FAILURE_LOOKUP_COUNT,
                             DEPARTED_FAILURE);
}

//==============================================================================
// 5.  ENDINGS
//==============================================================================

/**
 * @brief K01: --help.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_k01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_add_flag(_app,
                                 "--f",
                                 "a flag");
    d_tests_arg_parser_ending(_run,
                              "K01",
                              "--help");

    return;
}

/**
 * @brief K02: --help after a subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_k02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const s = d_arg_command_add_subcommand(_app,
                                                                 "s",
                                                                 "sd");

    (void)d_arg_command_add_flag(s,
                                 "--g",
                                 "g flag");
    d_tests_arg_parser_ending(_run,
                              "K02",
                              "s --help");

    return;
}

/**
 * @brief K03: --help-all, the root with a subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_k03(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_help_all_flag(_app,
                                          "--help-all",
                                          "all");

    struct d_arg_command* const s = d_arg_command_add_subcommand(_app,
                                                                 "s",
                                                                 "sd");

    (void)d_arg_command_add_flag(s,
                                 "--g",
                                 "g flag");
    d_tests_arg_parser_ending(_run,
                              "K03",
                              "--help-all");

    return;
}

/**
 * @brief K04: --version.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_k04(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_version_flag(_app,
                                         "--version",
                                         "1.2.3",
                                         NULL);
    d_tests_arg_parser_ending(_run,
                              "K04",
                              "--version");

    return;
}

/**
 * @brief K05: arguments left over.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_k05(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_app;

    d_tests_arg_parser_ending(_run,
                              "K05",
                              "a b");

    return;
}

/**
 * @brief K06: a required option left out.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_k06(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const x = d_arg_command_add_option(_app,
                                                            "--x",
                                                            NULL);

    d_arg_option_set_required(x,
                              true);
    d_tests_arg_parser_ending(_run,
                              "K06",
                              "");

    return;
}

/**
 * @brief K07: an option without its value.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_k07(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_add_option(_app,
                                   "--x",
                                   NULL);
    d_tests_arg_parser_ending(_run,
                              "K07",
                              "--x");

    return;
}

/**
 * @brief K08: a flag's callback failing with a runtime error of code 3.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_k08(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_add_flag_callback(_app,
                                          "--f",
                                          d_tests_arg_parser_runtime_three,
                                          NULL,
                                          NULL,
                                          NULL);
    d_tests_arg_parser_ending(_run,
                              "K08",
                              "--f");

    return;
}

/**
 * @brief K09: a flag's callback ending the parse with success.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_k09(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_add_flag_callback(_app,
                                          "--f",
                                          d_tests_arg_parser_success,
                                          NULL,
                                          NULL,
                                          NULL);
    d_tests_arg_parser_ending(_run,
                              "K09",
                              "--f");

    return;
}

/**
 * @brief K10: a flag's callback asking for help.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_k10(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_add_flag_callback(_app,
                                          "--f",
                                          d_tests_arg_parser_flag_help,
                                          NULL,
                                          NULL,
                                          NULL);
    d_tests_arg_parser_ending(_run,
                              "K10",
                              "--f");

    return;
}

/**
 * @brief K11: a flag's callback asking for a version.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_k11(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_add_flag_callback(_app,
                                          "--f",
                                          d_tests_arg_parser_version_two,
                                          NULL,
                                          NULL,
                                          NULL);
    d_tests_arg_parser_ending(_run,
                              "K11",
                              "--f");

    return;
}

/**
 * @brief K12: a subcommand's final callback asking for help.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_k12(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const s = d_arg_command_add_subcommand(_app,
                                                                 "s",
                                                                 "sd");

    d_arg_command_set_final_callback(s,
                                     d_tests_arg_parser_command_help,
                                     NULL,
                                     NULL);
    d_tests_arg_parser_ending(_run,
                              "K12",
                              "s");

    return;
}

/**
 * @brief K13: a subcommand's argument left over under the help failure
 *        message.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_k13(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_failure_message(_app,
                                      d_arg_failure_help,
                                      NULL,
                                      NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "s",
                                       "sd");
    d_tests_arg_parser_ending(_run,
                              "K13",
                              "s a");

    return;
}

/**
 * @brief K14: a subcommand's final callback failing, under the root's
 *        failure message.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_k14(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_failure_message(_app,
                                      d_tests_arg_parser_custom,
                                      NULL,
                                      NULL);

    struct d_arg_command* const s = d_arg_command_add_subcommand(_app,
                                                                 "s",
                                                                 NULL);

    d_arg_command_set_final_callback(s,
                                     d_tests_arg_parser_invalid,
                                     NULL,
                                     NULL);
    d_tests_arg_parser_ending(_run,
                              "K14",
                              "s");

    return;
}

/**
 * @brief K15: a parse that goes on.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_k15(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_add_flag(_app,
                                 "--f",
                                 NULL);
    d_tests_arg_parser_ending(_run,
                              "K15",
                              "--f");

    return;
}

// ENDING_STEPS
//   constant: the cases of d_tests_arg_parser_endings.
static const d_tests_arg_define_fn ENDING_STEPS[] =
{
    d_tests_arg_parser_k01,
    d_tests_arg_parser_k02,
    d_tests_arg_parser_k03,
    d_tests_arg_parser_k04,
    d_tests_arg_parser_k05,
    d_tests_arg_parser_k06,
    d_tests_arg_parser_k07,
    d_tests_arg_parser_k08,
    d_tests_arg_parser_k09,
    d_tests_arg_parser_k10,
    d_tests_arg_parser_k11,
    d_tests_arg_parser_k12,
    d_tests_arg_parser_k13,
    d_tests_arg_parser_k14,
    d_tests_arg_parser_k15
};

// ENDING_COUNT
//   constant: how many cases ENDING_STEPS holds.
static const size_t ENDING_COUNT =
    sizeof(ENDING_STEPS) / sizeof(ENDING_STEPS[0]);

/*
d_tests_arg_parser_endings
  A parse of main's arguments ended with d_arg_parser_exit_to, as CLI11's
exit(error, out, err) after a caught ParseError.
  Tests the following:
  - help, a subcommand's help, help of everything and a version asked for on the
    command line
  - arguments left over, a required option left out, and an option without its
    value
  - a flag's callback ending the parse with a runtime error, a success, a
    request for help and one for a version
  - a subcommand's final callback asking for help, and failing under its
    parent's failure message
  - a subcommand's arguments left over under the help failure message
  - a parse that goes on
*/
bool
d_tests_arg_parser_endings(void)
{
    return d_tests_arg_steps("ENDINGS",
                             ENDING_STEPS,
                             ENDING_COUNT,
                             ENDINGS);
}

//==============================================================================
// 6.  MACRO
//==============================================================================

/**
 * @brief M01: a parse that goes on.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_m01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_add_flag(_app,
                                 "--f",
                                 NULL);
    d_tests_arg_parser_macro_ending(_run,
                                    "M01",
                                    "--f");

    return;
}

/**
 * @brief M02: --help.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_m02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_app;

    d_tests_arg_parser_macro_ending(_run,
                                    "M02",
                                    "--help");

    return;
}

/**
 * @brief M03: an argument left over.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_m03(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_app;

    d_tests_arg_parser_macro_ending(_run,
                                    "M03",
                                    "a");

    return;
}

/**
 * @brief M04: a flag's callback failing with a runtime error of code 5.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_m04(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_add_flag_callback(_app,
                                          "--f",
                                          d_tests_arg_parser_runtime_five,
                                          NULL,
                                          NULL,
                                          NULL);
    d_tests_arg_parser_macro_ending(_run,
                                    "M04",
                                    "--f");

    return;
}

/**
 * @brief M05: --version.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_m05(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_version_flag(_app,
                                         "--version",
                                         "1.2.3",
                                         NULL);
    d_tests_arg_parser_macro_ending(_run,
                                    "M05",
                                    "--version");

    return;
}

// MACRO_STEPS
//   constant: the cases of d_tests_arg_parser_macro.
static const d_tests_arg_define_fn MACRO_STEPS[] =
{
    d_tests_arg_parser_m01,
    d_tests_arg_parser_m02,
    d_tests_arg_parser_m03,
    d_tests_arg_parser_m04,
    d_tests_arg_parser_m05
};

// MACRO_COUNT
//   constant: how many cases MACRO_STEPS holds.
static const size_t MACRO_COUNT = sizeof(MACRO_STEPS) / sizeof(MACRO_STEPS[0]);

/*
d_tests_arg_parser_macro
  D_ARG_PARSE in a function written as main is, as CLI11_PARSE: the function
going on, or returning the code of the parse's ending, which the macro writes to
the parser's own output and error output.
  Tests the following:
  - a parse that goes on, the function returning its own code
  - help asked for, arguments left over, a runtime error of a code of its own,
    and a version asked for
*/
bool
d_tests_arg_parser_macro(void)
{
    return d_tests_arg_steps("MACRO",
                             MACRO_STEPS,
                             MACRO_COUNT,
                             MACRO);
}

//==============================================================================
// 7.  ARGUMENTS
//==============================================================================

/**
 * @brief Q13: the arguments reversed, borrowed.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_q13(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                _app,
                                "--f");
    (void)d_tests_arg_make_all(_run,
                               "o",
                               _app,
                               "--o");
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);
    d_tests_arg_parser_vector(_run,
                              "Q13",
                              "--o 1 x --f --o=2",
                              D_ARG_ARGS_REVERSED);

    return;
}

/**
 * @brief Q14: the arguments reversed, copied.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_q14(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                _app,
                                "--f");
    (void)d_tests_arg_make_all(_run,
                               "o",
                               _app,
                               "--o");
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);
    d_tests_arg_parser_vector(_run,
                              "Q14",
                              "--o 1 x --f --o=2",
                              D_ARG_ARGS_REVERSED | D_ARG_ARGS_COPY);

    return;
}

// ARGUMENT_STEPS
//   constant: the cases of d_tests_arg_parser_arguments.
static const d_tests_arg_define_fn ARGUMENT_STEPS[] =
{
    d_tests_arg_parser_q13,
    d_tests_arg_parser_q14
};

// ARGUMENT_COUNT
//   constant: how many cases ARGUMENT_STEPS holds.
static const size_t ARGUMENT_COUNT =
    sizeof(ARGUMENT_STEPS) / sizeof(ARGUMENT_STEPS[0]);

/*
d_tests_arg_parser_arguments
  A vector of arguments given last first, as CLI11's
parse(std::vector<std::string>) takes them: d_arg_parser_parse_args with
D_ARG_ARGS_REVERSED, borrowing them or copying them.
  Tests the following:
  - options, a flag and an argument left over, read in the order given
  - values borrowed from the arguments
  - values copied, which outlive the arguments being overwritten
*/
bool
d_tests_arg_parser_arguments(void)
{
    return d_tests_arg_steps("ARGUMENTS",
                             ARGUMENT_STEPS,
                             ARGUMENT_COUNT,
                             ARGUMENTS);
}

//==============================================================================
// 8.  LINES
//==============================================================================

/**
 * @brief L01: flags, an option and positionals.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_l01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_tests_arg_parser_text_app(_run,
                                _app,
                                false);
    d_tests_arg_parser_line(_run,
                            "L01",
                            "",
                            "-v -o out f1 f2",
                            15u);
    d_tests_arg_report(_run);

    return;
}

/**
 * @brief L02: an `=` value in quotes.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_l02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_tests_arg_parser_text_app(_run,
                                _app,
                                false);
    d_tests_arg_parser_line(_run,
                            "L02",
                            "",
                            "--out=\"a b\" f",
                            13u);
    d_tests_arg_report(_run);

    return;
}

/**
 * @brief L03: a subcommand's option, its value in quotes.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_l03(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_tests_arg_parser_text_app(_run,
                                _app,
                                false);
    d_tests_arg_parser_line(_run,
                            "L03",
                            "",
                            "sub --x=\"q r\" f",
                            15u);
    d_tests_arg_report(_run);

    return;
}

/**
 * @brief L04: a malformed escape.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_l04(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_tests_arg_parser_text_app(_run,
                                _app,
                                false);
    d_tests_arg_parser_line(_run,
                            "L04",
                            "",
                            "\"bad \\q\"",
                            8u);
    d_tests_arg_report(_run);

    return;
}

/**
 * @brief L05: a Windows-style option, its value in quotes.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_l05(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_tests_arg_parser_text_app(_run,
                                _app,
                                true);
    d_tests_arg_parser_line(_run,
                            "L05",
                            "",
                            "/name:\"v w\" f",
                            13u);
    d_tests_arg_report(_run);

    return;
}

/**
 * @brief L06: a program's name in quotes, naming a root without one.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_l06(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_name(_app,
                                 NULL);
    d_tests_arg_parser_text_app(_run,
                                _app,
                                false);
    d_tests_arg_parser_line(_run,
                            "L06",
                            "named",
                            "\"quoted name\" -v f",
                            18u);
    d_tests_arg_report(_run);

    return;
}

/**
 * @brief L07: a program's name, the root keeping its own.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_l07(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_tests_arg_parser_text_app(_run,
                                _app,
                                false);
    d_tests_arg_parser_line(_run,
                            "L07",
                            "named",
                            "other -v",
                            8u);
    d_tests_arg_report(_run);

    return;
}

/**
 * @brief L08: an empty line with a program's name.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_l08(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_name(_app,
                                 NULL);
    d_tests_arg_parser_text_app(_run,
                                _app,
                                false);
    d_tests_arg_parser_line(_run,
                            "L08",
                            "named",
                            "",
                            0u);
    d_tests_arg_report(_run);

    return;
}

/**
 * @brief L09: a NUL byte from an escape in double quotes, kept in a value.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_l09(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_tests_arg_parser_text_app(_run,
                                _app,
                                false);
    d_tests_arg_parser_line(_run,
                            "L09",
                            "",
                            "--out \"a\\0b\" f",
                            14u);
    d_tests_arg_report(_run);

    return;
}

/**
 * @brief L10: a NUL byte from a Unicode escape in an `=` value in quotes.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_l10(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_tests_arg_parser_text_app(_run,
                                _app,
                                false);
    d_tests_arg_parser_line(_run,
                            "L10",
                            "",
                            "--name=\"x\\u0000y\" -v",
                            20u);
    d_tests_arg_report(_run);

    return;
}

/**
 * @brief L11: a NUL byte from an escape in a subcommand's option's value.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_l11(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_tests_arg_parser_text_app(_run,
                                _app,
                                false);
    d_tests_arg_parser_line(_run,
                            "L11",
                            "",
                            "sub --x \"p\\0q\"",
                            14u);
    d_tests_arg_report(_run);

    return;
}

/**
 * @brief L12: NUL bytes as they are, in values and a positional, each word
 *        kept whole.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_l12(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_tests_arg_parser_text_app(_run,
                                _app,
                                false);
    d_tests_arg_parser_line(_run,
                            "L12",
                            "",
                            "-o a\x00" "b f\x00g --name=c\x00" "d",
                            21u);
    d_tests_arg_report(_run);

    return;
}

/**
 * @brief L13: NUL bytes beside the names of options: a long option's name left
 *        over, the rest after a short flag read as a positional, and a short
 *        option's value attached.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_l13(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_tests_arg_parser_text_app(_run,
                                _app,
                                false);
    d_tests_arg_parser_line(_run,
                            "L13",
                            "",
                            "--verbose\x00 -v\x00 -oa\x00" "b",
                            20u);
    d_tests_arg_report(_run);

    return;
}

/**
 * @brief L14: NUL bytes in a Windows-style option's value and in quotes.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_l14(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_tests_arg_parser_text_app(_run,
                                _app,
                                true);
    d_tests_arg_parser_line(_run,
                            "L14",
                            "",
                            "/name:v\x00w \"f\x00\"",
                            14u);
    d_tests_arg_report(_run);

    return;
}

// LINE_STEPS
//   constant: the cases of d_tests_arg_parser_lines.
static const d_tests_arg_define_fn LINE_STEPS[] =
{
    d_tests_arg_parser_l01,
    d_tests_arg_parser_l02,
    d_tests_arg_parser_l03,
    d_tests_arg_parser_l04,
    d_tests_arg_parser_l05,
    d_tests_arg_parser_l06,
    d_tests_arg_parser_l07,
    d_tests_arg_parser_l08,
    d_tests_arg_parser_l09,
    d_tests_arg_parser_l10,
    d_tests_arg_parser_l11,
    d_tests_arg_parser_l12,
    d_tests_arg_parser_l13,
    d_tests_arg_parser_l14
};

// LINE_COUNT
//   constant: how many cases LINE_STEPS holds.
static const size_t LINE_COUNT = sizeof(LINE_STEPS) / sizeof(LINE_STEPS[0]);

/*
d_tests_arg_parser_lines
  A command line held in one string: CLI11's parse(string,
program_name_included).
  Tests the following:
  - the line split into words, quoted values kept whole, `=` values and a
    subcommand's option among them
  - a malformed escape refused as CLI11's ParseError
  - Windows-style options with a quoted value after `:`
  - a program's name in quotes naming a root without one; a root with a name
    keeping it; and an empty line
  - NUL bytes, from escapes in double quotes and as they are, kept whole in
    values, positionals and the names of options
*/
bool
d_tests_arg_parser_lines(void)
{
    return d_tests_arg_steps("LINES",
                             LINE_STEPS,
                             LINE_COUNT,
                             LINES);
}

//==============================================================================
// 9.  NAMES
//==============================================================================

/**
 * @brief N01: a root without a name, parsed twice.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_n01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_name(_app,
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--f",
                                 NULL);
    d_tests_arg_parser_named_main(_run,
                                  "N01",
                                  "prog1",
                                  "");
    d_tests_arg_parser_named_main(_run,
                                  "N01",
                                  "prog2",
                                  "--f");

    return;
}

/**
 * @brief N02: a root with a name.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_n02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_name(_app,
                                 "given");
    d_tests_arg_parser_named_main(_run,
                                  "N02",
                                  "prog1",
                                  "");

    return;
}

/**
 * @brief N03: parses without a program's name, before and after one with it.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_n03(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_name(_app,
                                 NULL);
    d_tests_arg_parser_named_vector(_run,
                                    "N03");
    d_tests_arg_parser_named_main(_run,
                                  "N03",
                                  "p3",
                                  "");
    d_tests_arg_parser_named_vector(_run,
                                    "N03");

    return;
}

/**
 * @brief N04: a name set between parses.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_n04(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_name(_app,
                                 NULL);
    d_tests_arg_parser_named_main(_run,
                                  "N04",
                                  "p4",
                                  "");
    (void)d_arg_command_set_name(_app,
                                 "fixed");
    d_tests_arg_parser_named_main(_run,
                                  "N04",
                                  "p5",
                                  "");

    return;
}

/**
 * @brief N05: an empty name set between parses.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_n05(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_name(_app,
                                 NULL);
    d_tests_arg_parser_named_main(_run,
                                  "N05",
                                  "p6",
                                  "");
    (void)d_arg_command_set_name(_app,
                                 "");
    d_tests_arg_parser_named_main(_run,
                                  "N05",
                                  "p7",
                                  "");

    return;
}

/**
 * @brief N06: lines with a program's name, in quotes and not.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_n06(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_name(_app,
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "-f",
                                 NULL);
    d_tests_arg_parser_line(_run,
                            "N06",
                            "named line",
                            "\"my prog\" -f",
                            12u);
    d_tests_arg_parser_line(_run,
                            "N06",
                            "named line",
                            "other",
                            5u);

    return;
}

/**
 * @brief N07: a final callback on a parse of nothing.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_n07(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_name(_app,
                                 NULL);
    d_arg_command_set_final_callback(_app,
                                     d_tests_arg_parser_final,
                                     _run,
                                     NULL);
    d_tests_arg_parser_named_vector(_run,
                                    "N07");

    return;
}

/**
 * @brief N08: help, the root named by the program's name.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_n08(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_name(_app,
                                 NULL);
    d_tests_arg_parser_named_ending(_run,
                                    "N08",
                                    "tool",
                                    "--help");

    return;
}

// NAME_STEPS
//   constant: the cases of d_tests_arg_parser_names.
static const d_tests_arg_define_fn NAME_STEPS[] =
{
    d_tests_arg_parser_n01,
    d_tests_arg_parser_n02,
    d_tests_arg_parser_n03,
    d_tests_arg_parser_n04,
    d_tests_arg_parser_n05,
    d_tests_arg_parser_n06,
    d_tests_arg_parser_n07,
    d_tests_arg_parser_n08
};

// NAME_COUNT
//   constant: how many cases NAME_STEPS holds.
static const size_t NAME_COUNT = sizeof(NAME_STEPS) / sizeof(NAME_STEPS[0]);

/*
d_tests_arg_parser_names
  The root's name from the program's name, as CLI11's App("d") takes it: from
argv[0], and from a line's first word.
  Tests the following:
  - a root without a name named by each parse, and one with a name keeping it
  - a parse without a program's name leaving the name as it was
  - a name set between parses kept, and an empty one set taking the next
  - a line's program's name, in quotes and not
  - a final callback on a parse of nothing
  - help naming the program the root took its name from
*/
bool
d_tests_arg_parser_names(void)
{
    return d_tests_arg_steps("NAMES",
                             NAME_STEPS,
                             NAME_COUNT,
                             NAMES);
}

//==============================================================================
// 10. STATE
//==============================================================================

/**
 * @brief T01: an option parsed, cleared, and parsed again.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_t01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  _app,
                                  "--x");
    d_tests_arg_parser_step(_run,
                            "T01",
                            "--x 1");
    d_tests_arg_note(_run,
                     "T01 clear");
    d_arg_parser_clear(&_run->parser);
    d_tests_arg_report(_run);
    d_tests_arg_parser_step(_run,
                            "T01",
                            "--x 2");

    return;
}

/**
 * @brief T02: a subcommand used, then not.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_t02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const s = d_tests_arg_make_subcommand(_run,
                                                                _app,
                                                                "s");

    (void)d_tests_arg_make_option(_run,
                                  "y",
                                  s,
                                  "--y");
    d_tests_arg_parser_step(_run,
                            "T02",
                            "s --y 1");
    d_tests_arg_parser_step(_run,
                            "T02",
                            "");

    return;
}

/**
 * @brief T03: an argument left over, then not.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_t03(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                _app,
                                "--f");
    d_tests_arg_parser_step(_run,
                            "T03",
                            "a --f");
    d_tests_arg_parser_step(_run,
                            "T03",
                            "--f");

    return;
}

// STATE_STEPS
//   constant: the cases of d_tests_arg_parser_state.
static const d_tests_arg_define_fn STATE_STEPS[] =
{
    d_tests_arg_parser_t01,
    d_tests_arg_parser_t02,
    d_tests_arg_parser_t03
};

// STATE_COUNT
//   constant: how many cases STATE_STEPS holds.
static const size_t STATE_COUNT = sizeof(STATE_STEPS) / sizeof(STATE_STEPS[0]);

/*
d_tests_arg_parser_state
  What one parse leaves for the next: nothing, each parse forgetting the last,
as d_arg_parser_clear does.
  Tests the following:
  - an option's value forgotten by d_arg_parser_clear and by the next parse
  - a subcommand used by one parse and not the next
  - an argument left over by one parse and not the next
*/
bool
d_tests_arg_parser_state(void)
{
    return d_tests_arg_steps("STATE",
                             STATE_STEPS,
                             STATE_COUNT,
                             STATE);
}

//==============================================================================
// 11. STATUSES
//==============================================================================

/**
 * @brief X01: an error status, none recorded.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_x01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_app;

    d_tests_arg_parser_end(_run,
                           "X01",
                           D_ARG_ERROR_EXTRAS);

    return;
}

/**
 * @brief X02: an error status other than the one recorded.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_x02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_fail(_app,
                             D_ARG_ERROR_VALIDATION,
                             "--x: bad value");
    d_tests_arg_parser_end(_run,
                           "X02",
                           D_ARG_ERROR_REQUIRED);

    return;
}

/**
 * @brief X03: D_ARG_OK.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_x03(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_app;

    d_tests_arg_parser_end(_run,
                           "X03",
                           D_ARG_OK);

    return;
}

/**
 * @brief X04: a runtime error, none recorded.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parser_x04(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_app;

    d_tests_arg_parser_end(_run,
                           "X04",
                           D_ARG_RUNTIME_ERROR);

    return;
}

// STATUS_STEPS
//   constant: the cases of d_tests_arg_parser_statuses.
static const d_tests_arg_define_fn STATUS_STEPS[] =
{
    d_tests_arg_parser_x01,
    d_tests_arg_parser_x02,
    d_tests_arg_parser_x03,
    d_tests_arg_parser_x04
};

// STATUS_COUNT
//   constant: how many cases STATUS_STEPS holds.
static const size_t STATUS_COUNT =
    sizeof(STATUS_STEPS) / sizeof(STATUS_STEPS[0]);

/*
d_tests_arg_parser_statuses
  Ending a parse with a status rather than an error, which CLI11's exit(), given
an error, has no counterpart of: the status described by the parser's error only
when it is that error's.
  Tests the following:
  - an error status with no error recorded, described by its own text and exit
    code
  - an error status other than the one recorded, described by its own
  - D_ARG_OK, and a runtime error with no code of its own, writing nothing
*/
bool
d_tests_arg_parser_statuses(void)
{
    return d_tests_arg_steps("STATUSES",
                             STATUS_STEPS,
                             STATUS_COUNT,
                             STATUSES);
}

//==============================================================================
// 12. NULLS
//==============================================================================

/**
 * @brief The endings given nothing to end: d_arg_parser_exit and
 *        d_arg_parser_exit_to without a parser, d_arg_command_exit and
 *        d_arg_command_exit_to without a command or an error -- each
 *        returning CLI11's BaseClass code and writing nothing.
 *
 * @return whether each came to that.
 */
static bool
d_tests_arg_parser_nulls_ended(void)
{
    struct d_tests_arg_run      run     = { 0 };
    struct d_arg_command* const app     = d_tests_arg_begin(&run);
    struct d_arg_error          error   = D_ARG_ERROR_INIT;
    const struct d_pack_sink    out     = d_arg_buffer_sink(&run.out);
    const struct d_pack_sink    err     = d_arg_buffer_sink(&run.err);
    const int                   codes[] =
    {
        d_arg_parser_exit(NULL,
                          (int)D_ARG_ERROR_EXTRAS),
        d_arg_parser_exit_to(NULL,
                             (int)D_ARG_ERROR_EXTRAS,
                             out,
                             err),
        d_arg_command_exit(NULL,
                           &error),
        d_arg_command_exit(app,
                           NULL),
        d_arg_command_exit_to(NULL,
                              &error,
                              out,
                              err),
        d_arg_command_exit_to(app,
                              NULL,
                              out,
                              err)
    };
    bool                        held    = !run.failed;

    // each call's code
    for (size_t index = 0u; index < (sizeof(codes) / sizeof(codes[0])); ++index)
    {
        // a code other than BaseClass's
        if (codes[index] != (int)D_ARG_EXIT_BASE_CLASS)
        {
            printf("    call %lu returned %d\n",
                   (unsigned long)index,
                   codes[index]);
            held = false;
        }
    }

    const bool quiet = ( (d_tests_arg_same("output",
                                           d_arg_buffer_text(&run.out),
                                           NULL)) &&
                         (d_tests_arg_same("error output",
                                           d_arg_buffer_text(&run.err),
                                           NULL)) );

    d_tests_arg_end(&run);

    return ( (held) &&
             (quiet) );
}

/**
 * @brief Whether each of a set of calls was refused, naming each that was
 *        not.
 *
 * @param[in] _codes  what the calls returned.
 * @param[in] _count  how many.
 * @param[in] _what   what the calls are, as each one's name begins.
 * @return whether every call returned D_ARG_ERROR_BAD_ARGUMENT.
 */
static bool
d_tests_arg_parser_nulls_check(
    const int*  _codes,
    size_t      _count,
    const char* _what
)
{
    bool held = true;

    // each call's status
    for (size_t index = 0u; index < _count; ++index)
    {
        // a status other than the refusal
        if (_codes[index] != (int)D_ARG_ERROR_BAD_ARGUMENT)
        {
            printf("    %s %lu returned %d\n",
                   _what,
                   (unsigned long)index,
                   _codes[index]);
            held = false;
        }
    }

    return held;
}

/**
 * @brief main's arguments given nothing to parse, refused:
 *        d_arg_parser_parse without a parser, with arguments that are not
 *        there, or with a NULL argument, first or later.
 *
 * @param[in,out] _run  the run, whose parser each call but the first is
 *                      given.
 * @return whether every call was refused.
 */
static bool
d_tests_arg_parser_nulls_argv(
    struct d_tests_arg_run* _run
)
{
    // PROGRAM
    //   constant: the program's name.
    static char PROGRAM[] = "prog";
    // GAP
    //   constant: arguments with a NULL where the second should be.
    static char* const GAP[] = { PROGRAM,
                                 NULL };
    // HEAD
    //   constant: arguments with a NULL where the program's name should be.
    static char* const HEAD[] = { NULL,
                                  PROGRAM };

    const int codes[] =
    {
        d_arg_parser_parse(NULL,
                           1,
                           GAP),
        d_arg_parser_parse(&_run->parser,
                           1,
                           NULL),
        d_arg_parser_parse(&_run->parser,
                           2,
                           GAP),
        d_arg_parser_parse(&_run->parser,
                           2,
                           HEAD)
    };

    return d_tests_arg_parser_nulls_check(codes,
                                          sizeof(codes) / sizeof(codes[0]),
                                          "d_arg_parser_parse refusal");
}

/**
 * @brief Arguments as spans, and a line, given nothing to parse, refused:
 *        d_arg_parser_parse_args without a parser, with arguments that are
 *        not there, or with one that is not there though it has a length;
 *        d_arg_parser_parse_line without a parser, or with a line that is
 *        not there though it has a length.
 *
 * @param[in,out] _run  the run, whose parser each call but the first of
 *                      each kind is given.
 * @return whether every call was refused.
 */
static bool
d_tests_arg_parser_nulls_spans(
    struct d_tests_arg_run* _run
)
{
    // LOST
    //   constant: arguments, the first not there though it has a length.
    static const struct d_pack_text LOST[] = { { NULL, 1u },
                                               { "x", 1u } };

    const struct d_pack_text gone    = { NULL, 1u };
    const int                codes[] =
    {
        d_arg_parser_parse_args(NULL,
                                NULL,
                                0u,
                                0u),
        d_arg_parser_parse_args(&_run->parser,
                                NULL,
                                1u,
                                0u),
        d_arg_parser_parse_args(&_run->parser,
                                LOST,
                                2u,
                                0u),
        d_arg_parser_parse_line(NULL,
                                d_pack_text_from_cstr("x"),
                                false),
        d_arg_parser_parse_line(&_run->parser,
                                gone,
                                false)
    };

    return d_tests_arg_parser_nulls_check(codes,
                                          sizeof(codes) / sizeof(codes[0]),
                                          "span parse refusal");
}

/**
 * @brief Adds an extra's bytes to the count a visit gathers.
 *
 * @param[in]     _extra    the extra.
 * @param[in,out] _context  the count, a `size_t`.
 * @return D_ARG_OK, to visit every one.
 */
static int
d_tests_arg_parser_nulls_bytes(
    const struct d_arg_extra* _extra,
    void*                     _context
)
{
    size_t* const bytes = _context;

    *bytes += _extra->text.length;

    return D_ARG_OK;
}

/**
 * @brief An argument that is not there with no length, parsed as the empty
 *        argument: one extra, of no bytes.
 *
 * @param[in,out] _run  the run, its root capturing extras.
 * @return whether it came to that.
 */
static bool
d_tests_arg_parser_nulls_empty(
    struct d_tests_arg_run* _run
)
{
    // EMPTY
    //   constant: an argument that is not there, and has no length.
    static const struct d_pack_text EMPTY[] = { { NULL, 0u } };

    size_t    bytes  = 0u;
    const int status = d_arg_parser_parse_args(&_run->parser,
                                               EMPTY,
                                               1u,
                                               0u);
    const int each   = d_arg_command_each_remaining(
                           &_run->parser.command,
                           false,
                           d_tests_arg_parser_nulls_bytes,
                           &bytes);

    return ( (status == D_ARG_OK)                        &&
             (d_arg_command_remaining_size(&_run->parser.command,
                                           false) == 1u) &&
             (each == D_ARG_OK)                          &&
             (bytes == 0u) );
}

/**
 * @brief The parses given nothing to parse: each refusal made before the
 *        parser is touched, the program's name of the parse before kept;
 *        then no arguments, and a line that is not there with no length,
 *        parsed as nothing there -- no argument, which an extra would fail
 *        -- and an argument that is not there with no length as the empty
 *        argument, captured as an extra of no bytes.
 *
 * @return whether each came to that.
 */
static bool
d_tests_arg_parser_nulls_parsed(void)
{
    // PROGRAM
    //   constant: the program's name.
    static char PROGRAM[] = "prog";
    // ALONE
    //   constant: the program's name, and no argument after it.
    static char* const ALONE[] = { PROGRAM };

    struct d_tests_arg_run      run     = { 0 };
    struct d_arg_command* const app     = d_tests_arg_begin(&run);
    const struct d_pack_text    none    = { NULL, 0u };
    const int                   alone   = d_arg_parser_parse(&run.parser,
                                                             1,
                                                             ALONE);
    const bool                  refused =
        ( (d_tests_arg_parser_nulls_argv(&run))   &&
          (d_tests_arg_parser_nulls_spans(&run)) );
    const bool                  kept    =
        ( (run.parser.program != NULL)   &&
          (strcmp(run.parser.program,
                  "prog") == 0) );
    const bool                  nothing =
        ( (d_arg_parser_parse(&run.parser,
                              0,
                              NULL) == D_ARG_OK)       &&
          (d_arg_parser_parse_args(&run.parser,
                                   NULL,
                                   0u,
                                   0u) == D_ARG_OK)    &&
          (d_arg_parser_parse_line(&run.parser,
                                   none,
                                   false) == D_ARG_OK) &&
          (d_arg_parser_parse_line(&run.parser,
                                   none,
                                   true) == D_ARG_OK) );

    d_arg_command_set_allow_extras(app,
                                   (int)D_ARG_EXTRAS_CAPTURE);

    const bool empty = d_tests_arg_parser_nulls_empty(&run);

    d_tests_arg_end(&run);

    return ( (app != NULL)       &&
             (alone == D_ARG_OK) &&
             (refused)           &&
             (kept)              &&
             (nothing)           &&
             (empty) );
}

/*
d_tests_arg_parser_nulls
  The calls given nothing, which no CLI11 call can be given: the endings
given nothing to end, and the parses nothing to parse.
  Tests the following:
  - d_arg_parser_exit and d_arg_parser_exit_to without a parser
  - d_arg_command_exit and d_arg_command_exit_to without a command, and
    without an error
  - each returns CLI11's BaseClass code and writes nothing
  - d_arg_parser_parse, d_arg_parser_parse_args and d_arg_parser_parse_line
    refused without a parser, arguments or a line, or with an argument that
    is not there, first or later, the parse before untouched
  - no arguments, and a line not there of no length, parsed as nothing
    there; an argument not there of no length as the empty argument
*/
bool
d_tests_arg_parser_nulls(void)
{
    const bool ended  = d_tests_arg_parser_nulls_ended();
    const bool parsed = d_tests_arg_parser_nulls_parsed();

    return ( (ended) &&
             (parsed) );
}

//==============================================================================
// 13. THE SUITE
//==============================================================================

// d_tests_arg_parser
//   constant: the suite, in the header's order.
const struct d_tests_section d_tests_arg_parser[] =
{
    { "arg_parser_exits",
      d_tests_arg_parser_exits },
    { "arg_parser_command_exits",
      d_tests_arg_parser_command_exits },
    { "arg_parser_own_sinks",
      d_tests_arg_parser_own_sinks },
    { "arg_parser_failure_lookup",
      d_tests_arg_parser_failure_lookup },
    { "arg_parser_endings",
      d_tests_arg_parser_endings },
    { "arg_parser_macro",
      d_tests_arg_parser_macro },
    { "arg_parser_arguments",
      d_tests_arg_parser_arguments },
    { "arg_parser_lines",
      d_tests_arg_parser_lines },
    { "arg_parser_names",
      d_tests_arg_parser_names },
    { "arg_parser_state",
      d_tests_arg_parser_state },
    { "arg_parser_statuses",
      d_tests_arg_parser_statuses },
    { "arg_parser_nulls",
      d_tests_arg_parser_nulls }
};

// d_tests_arg_parser_count
//   constant: how many sections the table above holds.
const size_t d_tests_arg_parser_count =
    sizeof(d_tests_arg_parser) / sizeof(d_tests_arg_parser[0]);
