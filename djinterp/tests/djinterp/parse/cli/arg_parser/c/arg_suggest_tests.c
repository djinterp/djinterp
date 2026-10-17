/*******************************************************************************
* djinterp [parse]                                           arg_suggest_tests.c
*
* The suite of the parser's suggestions, declared in arg_suggest_tests.h.
*   Each section is a case table (arg_tests_support.h) its runner runs
* twice: each case makes its options and commands as CLI11's
* App("d", "prog") would, on a run's parser of its own, turns the
* parser's suggestions on, or leaves them off, parses its arguments, and is
* written as the reference writes it -- a line of its tag and arguments,
* then the parse as the runs report one -- each pass compared whole with
* its transcript. The cases come from tools/gen/suggest_rows.py, which
* writes the reference's too; those of the suggestion probe
* (probes/suggest/mine_suggest.c) keep its tags, S01-S29, and those after
* them begin at S30.
*   The distance is the module's own call (arg_internal.h), which only the
* band of the table within its limit computes; its section holds it to
* the whole table over every pair of short words and many longer ones,
* within every limit, as no parse, whose limit the knob fixes, could.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_suggest_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "./arg_suggest_tests.h"  // corresponding header
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <stdio.h>    // printf, snprintf
#include <string.h>   // memcpy, memmove
// djinterp
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // the knobs
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // commands, their settings
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, modes
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // options, their settings
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // the parser, its suggestions
#include "../../../../../../src/djinterp/parse/cli/arg_parser/c/arg_internal.h"  // the edit distance
#include "../../../c/tests_section.h"  // d_tests_section
#include "./arg_tests_support.h"  // runs, case tables, escaping


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// OPTIONS
//   constant: what the cases of d_tests_arg_suggest_options come to with the
// parser's suggestions: CLI11's message, and the lines the module's rules add
// (ISSUES 21), as the reference works them out.
static const char OPTIONS[] =
    "S01 [--verbse]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--verbse\\nDid you mean '--verbose'?\n"
    "cmd app n=1 miss=[--verbse] subs=[] order=[]\n"
    "S02 [--verbse=3]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--verbse=3\\nDid you mean '--verbose'?\n"
    "cmd app n=1 miss=[--verbse=3] subs=[] order=[]\n"
    "S04 [--dryrun]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--dryrun\\nDid you mean '--dry-run'?\n"
    "cmd app n=1 miss=[--dryrun] subs=[] order=[]\n"
    "S05 [--colr]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--colr\\nDid you mean '--color'?\n"
    "cmd app n=1 miss=[--colr] subs=[] order=[]\n"
    "S06 [--colou]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--colou\n"
    "cmd app n=1 miss=[--colou] subs=[] order=[]\n"
    "S07 [--secrt]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--secrt\n"
    "cmd app n=1 miss=[--secrt] subs=[] order=[]\n"
    "S08 [--NAEM]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--NAEM\\nDid you mean '--Name'?\n"
    "cmd app n=1 miss=[--NAEM] subs=[] order=[]\n"
    "S09 [--longnam]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--longnam\\nDid you mean '--long_name'?\n"
    "cmd app n=1 miss=[--longnam] subs=[] order=[]\n"
    "S10 [--extar]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--extar\\nDid you mean '--extra'?\n"
    "cmd app n=1 miss=[--extar] subs=[] order=[]\n"
    "S19 [--v]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--v\\nDid you mean '-v'?\n"
    "cmd app n=1 miss=[--v] subs=[] order=[]\n"
    "S20 [-V]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: -V\n"
    "cmd app n=1 miss=[-V] subs=[] order=[]\n"
    "S30 [--hlep]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--hlep\\nDid you mean '--help'?\n"
    "cmd app n=1 miss=[--hlep] subs=[] order=[]\n"
    "S31 [--Verbose]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--Verbose\\nDid you mean '--verbose'?\n"
    "cmd app n=1 miss=[--Verbose] subs=[] order=[]\n"
    "S32 [--outptu=x]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--outptu=x\\nDid you mean '--output'?\n"
    "cmd app n=1 miss=[--outptu=x] subs=[] order=[]\n"
    "S33 [--no-lod]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--no-lod\\nDid you mean '--no-loud'?\n"
    "cmd app n=1 miss=[--no-lod] subs=[] order=[]\n"
    "S34 [--extar]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--extar\n"
    "cmd app n=1 miss=[--extar] subs=[] order=[]\n"
    "S35 [--deper]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--deper\\nDid you mean '--deeper'?\n"
    "cmd app n=1 miss=[--deper] subs=[] order=[]\n"
    "S25 [/verbse]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "/verbse\\nDid you mean '/verbose'?\n"
    "cmd app n=1 miss=[/verbse] subs=[] order=[]\n"
    "S26 [/outptu:x]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "/outptu:x\\nDid you mean '/output'?\n"
    "cmd app n=1 miss=[/outptu:x] subs=[] order=[]\n"
    "S78 [--outptu:x]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--outptu:x\n"
    "cmd app n=1 miss=[--outptu:x] subs=[] order=[]\n"
    "S62 [--abcd]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--abcd\n"
    "cmd app n=1 miss=[--abcd] subs=[] order=[]\n"
    "S63 [--abce]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--abce\\nDid you mean '--a_b_c_d'?\n"
    "cmd app n=1 miss=[--abce] subs=[] order=[]\n"
    "S64 [--ABCE]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--ABCE\n"
    "cmd app n=1 miss=[--ABCE] subs=[] order=[]\n"
    "S65 [--ABCD]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--ABCD\n"
    "cmd app n=1 miss=[--ABCD] subs=[] order=[]\n";

// OPTIONS_PLAIN
//   constant: what the cases of d_tests_arg_suggest_options come to without
// suggestions, as CLI11 parses them.
static const char OPTIONS_PLAIN[] =
    "S01 [--verbse]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--verbse\n"
    "cmd app n=1 miss=[--verbse] subs=[] order=[]\n"
    "S02 [--verbse=3]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--verbse=3\n"
    "cmd app n=1 miss=[--verbse=3] subs=[] order=[]\n"
    "S04 [--dryrun]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--dryrun\n"
    "cmd app n=1 miss=[--dryrun] subs=[] order=[]\n"
    "S05 [--colr]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--colr\n"
    "cmd app n=1 miss=[--colr] subs=[] order=[]\n"
    "S06 [--colou]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--colou\n"
    "cmd app n=1 miss=[--colou] subs=[] order=[]\n"
    "S07 [--secrt]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--secrt\n"
    "cmd app n=1 miss=[--secrt] subs=[] order=[]\n"
    "S08 [--NAEM]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--NAEM\n"
    "cmd app n=1 miss=[--NAEM] subs=[] order=[]\n"
    "S09 [--longnam]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--longnam\n"
    "cmd app n=1 miss=[--longnam] subs=[] order=[]\n"
    "S10 [--extar]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--extar\n"
    "cmd app n=1 miss=[--extar] subs=[] order=[]\n"
    "S19 [--v]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: --v\n"
    "cmd app n=1 miss=[--v] subs=[] order=[]\n"
    "S20 [-V]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: -V\n"
    "cmd app n=1 miss=[-V] subs=[] order=[]\n"
    "S30 [--hlep]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--hlep\n"
    "cmd app n=1 miss=[--hlep] subs=[] order=[]\n"
    "S31 [--Verbose]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--Verbose\n"
    "cmd app n=1 miss=[--Verbose] subs=[] order=[]\n"
    "S32 [--outptu=x]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--outptu=x\n"
    "cmd app n=1 miss=[--outptu=x] subs=[] order=[]\n"
    "S33 [--no-lod]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--no-lod\n"
    "cmd app n=1 miss=[--no-lod] subs=[] order=[]\n"
    "S34 [--extar]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--extar\n"
    "cmd app n=1 miss=[--extar] subs=[] order=[]\n"
    "S35 [--deper]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--deper\n"
    "cmd app n=1 miss=[--deper] subs=[] order=[]\n"
    "S25 [/verbse]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "/verbse\n"
    "cmd app n=1 miss=[/verbse] subs=[] order=[]\n"
    "S26 [/outptu:x]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "/outptu:x\n"
    "cmd app n=1 miss=[/outptu:x] subs=[] order=[]\n"
    "S78 [--outptu:x]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--outptu:x\n"
    "cmd app n=1 miss=[--outptu:x] subs=[] order=[]\n"
    "S62 [--abcd]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--abcd\n"
    "cmd app n=1 miss=[--abcd] subs=[] order=[]\n"
    "S63 [--abce]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--abce\n"
    "cmd app n=1 miss=[--abce] subs=[] order=[]\n"
    "S64 [--ABCE]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--ABCE\n"
    "cmd app n=1 miss=[--ABCE] subs=[] order=[]\n"
    "S65 [--ABCD]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--ABCD\n"
    "cmd app n=1 miss=[--ABCD] subs=[] order=[]\n";

// COMMANDS
//   constant: what the cases of d_tests_arg_suggest_commands come to with the
// parser's suggestions: CLI11's message, and the lines the module's rules add
// (ISSUES 21), as the reference works them out.
static const char COMMANDS[] =
    "S03 [strat]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "strat\\nDid you mean 'start'?\n"
    "cmd app n=1 miss=[strat] subs=[] order=[]\n"
    "S13 [begn]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "begn\\nDid you mean 'begin'?\n"
    "cmd app n=1 miss=[begn] subs=[] order=[]\n"
    "S23 [stauts]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "stauts\\nDid you mean 'status'?\n"
    "cmd app n=1 miss=[stauts] subs=[] order=[]\n"
    "S36 [STRAT]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "STRAT\\nDid you mean 'start'?\n"
    "cmd app n=1 miss=[STRAT] subs=[] order=[]\n"
    "S37 [insbu]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "insbu\\nDid you mean 'insub'?\n"
    "cmd app n=1 miss=[insbu] subs=[] order=[]\n"
    "S11 [internl]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "internl\n"
    "cmd app n=1 miss=[internl] subs=[] order=[]\n"
    "S12 [ofline]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "ofline\n"
    "cmd app n=1 miss=[ofline] subs=[] order=[]\n"
    "S22 [u]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: u\n"
    "cmd app n=1 miss=[u] subs=[] order=[]\n"
    "S18 [-5]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: -5\n"
    "cmd app n=1 miss=[-5] subs=[] order=[]\n"
    "S38 [-- strat]\n"
    "ERR ExtrasError 109: prog: The following arguments were not expected: -- "
    "strat\\nDid you mean 'start' for 'strat'?\n"
    "cmd app n=1 miss=[--|strat] subs=[] order=[]\n"
    "S29 [lone innr]\n"
    "ERR ExtrasError 109: lone: The following argument was not expected: "
    "innr\\nDid you mean 'inner'?\n"
    "cmd app n=1 miss=[] subs=[lone] order=[]\n"
    "S39 [lone strat]\n"
    "ERR ExtrasError 109: lone: The following argument was not expected: "
    "strat\\nDid you mean 'start'?\n"
    "cmd app n=1 miss=[] subs=[lone] order=[]\n"
    "S40 [lone strat]\n"
    "ERR ExtrasError 109: lone: The following argument was not expected: "
    "strat\n"
    "cmd app n=1 miss=[] subs=[lone] order=[]\n"
    "S70 [a b strat]\n"
    "ERR ExtrasError 109: b: The following argument was not expected: "
    "strat\\nDid you mean 'start'?\n"
    "cmd app n=1 miss=[] subs=[a] order=[]\n"
    "S75 [bx]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "bx\\nDid you mean 'b'?\n"
    "cmd app n=1 miss=[bx] subs=[] order=[]\n"
    "S77 [r]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "r\\nDid you mean 'rm'?\n"
    "cmd app n=1 miss=[r] subs=[] order=[]\n"
    "S76 [-1]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: -1\n"
    "cmd app n=1 miss=[-1] subs=[] order=[]\n";

// COMMANDS_PLAIN
//   constant: what the cases of d_tests_arg_suggest_commands come to without
// suggestions, as CLI11 parses them.
static const char COMMANDS_PLAIN[] =
    "S03 [strat]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "strat\n"
    "cmd app n=1 miss=[strat] subs=[] order=[]\n"
    "S13 [begn]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: begn\n"
    "cmd app n=1 miss=[begn] subs=[] order=[]\n"
    "S23 [stauts]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "stauts\n"
    "cmd app n=1 miss=[stauts] subs=[] order=[]\n"
    "S36 [STRAT]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "STRAT\n"
    "cmd app n=1 miss=[STRAT] subs=[] order=[]\n"
    "S37 [insbu]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "insbu\n"
    "cmd app n=1 miss=[insbu] subs=[] order=[]\n"
    "S11 [internl]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "internl\n"
    "cmd app n=1 miss=[internl] subs=[] order=[]\n"
    "S12 [ofline]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "ofline\n"
    "cmd app n=1 miss=[ofline] subs=[] order=[]\n"
    "S22 [u]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: u\n"
    "cmd app n=1 miss=[u] subs=[] order=[]\n"
    "S18 [-5]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: -5\n"
    "cmd app n=1 miss=[-5] subs=[] order=[]\n"
    "S38 [-- strat]\n"
    "ERR ExtrasError 109: prog: The following arguments were not expected: -- "
    "strat\n"
    "cmd app n=1 miss=[--|strat] subs=[] order=[]\n"
    "S29 [lone innr]\n"
    "ERR ExtrasError 109: lone: The following argument was not expected: innr\n"
    "cmd app n=1 miss=[] subs=[lone] order=[]\n"
    "S39 [lone strat]\n"
    "ERR ExtrasError 109: lone: The following argument was not expected: "
    "strat\n"
    "cmd app n=1 miss=[] subs=[lone] order=[]\n"
    "S40 [lone strat]\n"
    "ERR ExtrasError 109: lone: The following argument was not expected: "
    "strat\n"
    "cmd app n=1 miss=[] subs=[lone] order=[]\n"
    "S70 [a b strat]\n"
    "ERR ExtrasError 109: b: The following argument was not expected: strat\n"
    "cmd app n=1 miss=[] subs=[a] order=[]\n"
    "S75 [bx]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: bx\n"
    "cmd app n=1 miss=[bx] subs=[] order=[]\n"
    "S77 [r]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: r\n"
    "cmd app n=1 miss=[r] subs=[] order=[]\n"
    "S76 [-1]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: -1\n"
    "cmd app n=1 miss=[-1] subs=[] order=[]\n";

// DISTANCES
//   constant: what the cases of d_tests_arg_suggest_distances come to with the
// parser's suggestions: CLI11's message, and the lines the module's rules add
// (ISSUES 21), as the reference works them out.
static const char DISTANCES[] =
    "S50 [--vebrose]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--vebrose\\nDid you mean '--verbose'?\n"
    "cmd app n=1 miss=[--vebrose] subs=[] order=[]\n"
    "S51 [--verbosee]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--verbosee\\nDid you mean '--verbose'?\n"
    "cmd app n=1 miss=[--verbosee] subs=[] order=[]\n"
    "S52 [--vrbse]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--vrbse\\nDid you mean '--verbose'?\n"
    "cmd app n=1 miss=[--vrbse] subs=[] order=[]\n"
    "S53 [--vrbs]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--vrbs\n"
    "cmd app n=1 miss=[--vrbs] subs=[] order=[]\n"
    "S54 [--ac]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--ac\\nDid you mean '--ab'?\n"
    "cmd app n=1 miss=[--ac] subs=[] order=[]\n"
    "S55 [--xy]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: --xy\n"
    "cmd app n=1 miss=[--xy] subs=[] order=[]\n"
    "S56 [--abxy]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--abxy\\nDid you mean '--abcd'?\n"
    "cmd app n=1 miss=[--abxy] subs=[] order=[]\n"
    "S61 [--axz]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--axz\n"
    "cmd app n=1 miss=[--axz] subs=[] order=[]\n"
    "S57 [--abx]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--abx\n"
    "cmd app n=1 miss=[--abx] subs=[] order=[]\n"
    "S58 [remote --forse]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--forse\\nDid you mean '--force'?\n"
    "cmd app n=1 miss=[--forse] subs=[remote] order=[]\n";

// DISTANCES_PLAIN
//   constant: what the cases of d_tests_arg_suggest_distances come to without
// suggestions, as CLI11 parses them.
static const char DISTANCES_PLAIN[] =
    "S50 [--vebrose]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--vebrose\n"
    "cmd app n=1 miss=[--vebrose] subs=[] order=[]\n"
    "S51 [--verbosee]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--verbosee\n"
    "cmd app n=1 miss=[--verbosee] subs=[] order=[]\n"
    "S52 [--vrbse]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--vrbse\n"
    "cmd app n=1 miss=[--vrbse] subs=[] order=[]\n"
    "S53 [--vrbs]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--vrbs\n"
    "cmd app n=1 miss=[--vrbs] subs=[] order=[]\n"
    "S54 [--ac]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: --ac\n"
    "cmd app n=1 miss=[--ac] subs=[] order=[]\n"
    "S55 [--xy]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: --xy\n"
    "cmd app n=1 miss=[--xy] subs=[] order=[]\n"
    "S56 [--abxy]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--abxy\n"
    "cmd app n=1 miss=[--abxy] subs=[] order=[]\n"
    "S61 [--axz]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--axz\n"
    "cmd app n=1 miss=[--axz] subs=[] order=[]\n"
    "S57 [--abx]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--abx\n"
    "cmd app n=1 miss=[--abx] subs=[] order=[]\n"
    "S58 [remote --forse]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--forse\n"
    "cmd app n=1 miss=[--forse] subs=[remote] order=[]\n";

// REACH
//   constant: what the cases of d_tests_arg_suggest_reach come to with the
// parser's suggestions: CLI11's message, and the lines the module's rules add
// (ISSUES 21), as the reference works them out.
static const char REACH[] =
    "S14 [remote --forse]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--forse\\nDid you mean '--force'?\n"
    "cmd app n=1 miss=[--forse] subs=[remote] order=[]\n"
    "S15 [remote --verbse]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--verbse\\nDid you mean '--verbose'?\n"
    "cmd app n=1 miss=[--verbse] subs=[remote] order=[]\n"
    "S59 [a b --zzy]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--zzy\\nDid you mean '--zzz'?\n"
    "cmd app n=1 miss=[--zzy] subs=[a] order=[]\n"
    "S28 [lone --quite]\n"
    "ERR ExtrasError 109: lone: The following argument was not expected: "
    "--quite\\nDid you mean '--quiet'?\n"
    "cmd app n=1 miss=[] subs=[lone] order=[]\n"
    "S68 [lone --verbse]\n"
    "ERR ExtrasError 109: lone: The following argument was not expected: "
    "--verbse\n"
    "cmd app n=1 miss=[] subs=[lone] order=[]\n"
    "S71 [remote forse]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "forse\n"
    "cmd app n=1 miss=[forse] subs=[remote] order=[]\n"
    "S72 [remote /forse]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "/forse\\nDid you mean '/force'?\n"
    "cmd app n=1 miss=[/forse] subs=[remote] order=[]\n"
    "S74 [remote ad]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "ad\\nDid you mean 'add'?\n"
    "cmd app n=1 miss=[ad] subs=[remote] order=[]\n"
    "S73 [-xb]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: -xb\n"
    "cmd app n=1 miss=[-xb] subs=[] order=[]\n"
    "S60 [--in s]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: --in\n"
    "cmd app n=1 miss=[--in] subs=[s] order=[]\n"
    "S16 [--verbse --outptu x]\n"
    "ERR ExtrasError 109: prog: The following arguments were not expected: "
    "--verbse --outptu x\\nDid you mean '--verbose' for '--verbse'?\\nDid you "
    "mean '--output' for '--outptu'?\n"
    "cmd app n=1 miss=[--verbse|--outptu|x] subs=[] order=[]\n"
    "S41 [strat stpo]\n"
    "ERR ExtrasError 109: prog: The following arguments were not expected: "
    "strat stpo\\nDid you mean 'start' for 'strat'?\\nDid you mean 'stop' for "
    "'stpo'?\n"
    "cmd app n=1 miss=[strat|stpo] subs=[] order=[]\n"
    "S24 [--colour=red --verbse]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--verbse\\nDid you mean '--verbose'?\n"
    "cmd app n=1 miss=[--verbse] subs=[] order=[--colour]\n"
    "S21 [--verbse]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--verbse\\nDid you mean '--verbose'?\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "S42 [lone --quite]\n"
    "ERR ExtrasError 109: lone: The following argument was not expected: "
    "--quite\\nDid you mean '--quiet'?\n"
    "cmd app n=1 miss=[] subs=[lone] order=[]\n"
    "S27 [f1 --verbse]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--verbse\\nDid you mean '--verbose'?\n"
    "cmd app n=1 miss=[] subs=[] order=[file]\n"
    "S43 [--verbse strat]\n"
    "cmd app n=1 miss=[--verbse|strat] subs=[] order=[]\n";

// REACH_PLAIN
//   constant: what the cases of d_tests_arg_suggest_reach come to without
// suggestions, as CLI11 parses them.
static const char REACH_PLAIN[] =
    "S14 [remote --forse]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--forse\n"
    "cmd app n=1 miss=[--forse] subs=[remote] order=[]\n"
    "S15 [remote --verbse]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--verbse\n"
    "cmd app n=1 miss=[--verbse] subs=[remote] order=[]\n"
    "S59 [a b --zzy]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--zzy\n"
    "cmd app n=1 miss=[--zzy] subs=[a] order=[]\n"
    "S28 [lone --quite]\n"
    "ERR ExtrasError 109: lone: The following argument was not expected: "
    "--quite\n"
    "cmd app n=1 miss=[] subs=[lone] order=[]\n"
    "S68 [lone --verbse]\n"
    "ERR ExtrasError 109: lone: The following argument was not expected: "
    "--verbse\n"
    "cmd app n=1 miss=[] subs=[lone] order=[]\n"
    "S71 [remote forse]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "forse\n"
    "cmd app n=1 miss=[forse] subs=[remote] order=[]\n"
    "S72 [remote /forse]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "/forse\n"
    "cmd app n=1 miss=[/forse] subs=[remote] order=[]\n"
    "S74 [remote ad]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: ad\n"
    "cmd app n=1 miss=[ad] subs=[remote] order=[]\n"
    "S73 [-xb]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: -xb\n"
    "cmd app n=1 miss=[-xb] subs=[] order=[]\n"
    "S60 [--in s]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: --in\n"
    "cmd app n=1 miss=[--in] subs=[s] order=[]\n"
    "S16 [--verbse --outptu x]\n"
    "ERR ExtrasError 109: prog: The following arguments were not expected: "
    "--verbse --outptu x\n"
    "cmd app n=1 miss=[--verbse|--outptu|x] subs=[] order=[]\n"
    "S41 [strat stpo]\n"
    "ERR ExtrasError 109: prog: The following arguments were not expected: "
    "strat stpo\n"
    "cmd app n=1 miss=[strat|stpo] subs=[] order=[]\n"
    "S24 [--colour=red --verbse]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--verbse\n"
    "cmd app n=1 miss=[--verbse] subs=[] order=[--colour]\n"
    "S21 [--verbse]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--verbse\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "S42 [lone --quite]\n"
    "ERR ExtrasError 109: lone: The following argument was not expected: "
    "--quite\n"
    "cmd app n=1 miss=[] subs=[lone] order=[]\n"
    "S27 [f1 --verbse]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--verbse\n"
    "cmd app n=1 miss=[] subs=[] order=[file]\n"
    "S43 [--verbse strat]\n"
    "cmd app n=1 miss=[--verbse|strat] subs=[] order=[]\n";

//==============================================================================
// HELPERS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_SUGGESTIONS == 1)

/**
 * @brief Turns a parser's suggestions on or off.
 *
 * @param[in,out] _parser  the parser.
 * @param[in]     _on      whether it is to make them.
 */
static void
d_tests_arg_suggest_switch(
    struct d_arg_parser* _parser,
    bool                 _on
)
{
    d_arg_parser_set_suggestions(_parser,
                                 _on);

    return;
}

#else

/**
 * @brief Where suggestions are not built, a parser makes none either way.
 *
 * @param[in,out] _parser  the parser.
 * @param[in]     _on      whether it would be to make them.
 */
static void
d_tests_arg_suggest_switch(
    struct d_arg_parser* _parser,
    bool                 _on
)
{
    (void)_parser;
    (void)_on;

    return;
}

#endif  // D_INTERNAL_ARG_PARSER_SUGGESTIONS

/**
 * @brief Appends a case's line, as the reference writes it: `<tag>
 *        [<arguments>]`, the arguments escaped and parted by spaces.
 *
 * @param[in,out] _out    the transcript.
 * @param[in]     _case   the case.
 * @param[in]     _count  how many arguments it has.
 */
static void
d_tests_arg_suggest_line(
    struct d_arg_buffer*           _out,
    const struct d_tests_arg_case* _case,
    size_t                         _count
)
{
    (void)d_arg_buffer_printf(_out,
                              "%s [",
                              _case->tag);

    // each argument, after a space when one comes before it
    for (size_t index = 0u; index < _count; ++index)
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       (index > 0u) ? " " : "");
        d_tests_arg_escape(_out,
                           d_pack_text_from_cstr(_case->args[index]));
    }

    (void)d_arg_buffer_append_cstr(_out,
                                   "]\n");

    return;
}

/**
 * @brief Runs one case on a run of its own: its definitions, its line, then
 *        the parse -- the parser's suggestions on or off -- as the run
 *        reports it.
 *
 * @param[in,out] _out   the pass's transcript.
 * @param[in]     _case  the case.
 * @param[in]     _on    whether the parser makes suggestions.
 * @return whether the run was made, its definitions and watches kept.
 */
static bool
d_tests_arg_suggest_case(
    struct d_arg_buffer*           _out,
    const struct d_tests_arg_case* _case,
    bool                           _on
)
{
    struct d_tests_arg_run      run   = { 0 };
    struct d_arg_command* const app   = d_tests_arg_begin(&run);
    size_t                      count = 0u;

    // the arguments, up to the first that is not there
    while ( (count < (size_t)D_TESTS_ARG_CASE_ARGS) &&
            (_case->args[count] != NULL) )
    {
        count++;
    }

    // the definitions, on a root that was made
    if (app != NULL)
    {
        _case->define(&run,
                      app);
    }

    d_tests_arg_suggest_switch(&run.parser,
                               _on);
    d_tests_arg_suggest_line(_out,
                             _case,
                             count);
    (void)d_tests_arg_parse(&run,
                            _case->args,
                            count);
    (void)d_arg_buffer_append_text(_out,
                                   d_arg_buffer_text(&run.transcript));

    const bool held = ( (app != NULL)                  &&
                        (!run.failed)                  &&
                        (run.transcript.failed == 0) );

    d_tests_arg_end(&run);

    return held;
}

/**
 * @brief Runs a section's cases twice -- with the parser's suggestions, then
 *        without them -- and compares each pass with its transcript.
 *
 * @note Every case runs in each pass, whatever the ones before it came to,
 *       so that a mismatch shows the whole transcript.
 *
 * @param[in] _name      the section's transcript, for a printed mismatch.
 * @param[in] _cases     the cases.
 * @param[in] _count     how many.
 * @param[in] _expected  the two passes' transcripts, in their order.
 * @return whether every case ran and both passes wrote what was expected.
 */
static bool
d_tests_arg_suggest_cases(
    const char*                    _name,
    const struct d_tests_arg_case* _cases,
    size_t                         _count,
    const char* const*             _expected
)
{
    bool held = true;

    // each pass: with suggestions, then without
    for (size_t pass = 0u; pass < 2u; ++pass)
    {
        struct d_arg_buffer written  = D_ARG_BUFFER_INIT;
        char                what[64] = { 0 };

        // each case, on a run of its own
        for (size_t index = 0u; index < _count; ++index)
        {
            held = d_tests_arg_suggest_case(&written,
                                            &_cases[index],
                                            (pass == 0u)) && held;
        }

        (void)snprintf(what,
                       sizeof what,
                       "%s%s",
                       _name,
                       (pass == 0u) ? "" : "_PLAIN");
        held = ( (d_tests_arg_same(what,
                                   d_arg_buffer_text(&written),
                                   _expected[pass])) &&
                 (written.failed == 0) ) && held;
        d_arg_buffer_release(&written);
    }

    return held;
}

//==============================================================================
// 1.  OPTIONS
//==============================================================================

/**
 * @brief S01: a long name, a letter left out.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S02: a long name with a value after `=`.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S04: a long name, its dash left out.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s04(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S05: an option of two names, the nearer one named.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s05(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S06: an option of two names, each as near.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s06(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S07: a hidden option.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s07(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S08: an option that ignores case.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s08(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S09: an option that ignores underscores.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s09(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S10: an option group's option.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s10(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S19: a short name given as a long one.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s19(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S20: a short name of another case.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s20(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S30: the help flag.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s30(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S31: a long name of another case, the option comparing case.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s31(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S32: an option taking a value, its value after `=`.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s32(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S33: a negated flag's name.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s33(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--loud,!--no-loud",
                                 NULL);

    return;
}

/**
 * @brief S34: a disabled option group's option.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s34(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);
    d_arg_command_set_disabled(extras,
                               true);

    return;
}

/**
 * @brief S35: an option in a group within a group.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s35(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    struct d_arg_command* const inner = d_arg_command_add_option_group(extras,
                                                                       "Inner",
                                                                       NULL);

    (void)d_arg_command_add_flag(inner,
                                 "--deeper",
                                 NULL);

    return;
}

/**
 * @brief S25: a Windows-style option.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s25(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);
    d_arg_command_set_windows_style(_app,
                                    true);

    return;
}

/**
 * @brief S26: a Windows-style option, its value after `:`.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s26(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);
    d_arg_command_set_windows_style(_app,
                                    true);

    return;
}

/**
 * @brief S78: a long name with `:` in it, which only a Windows-style option
 *        ends its name at.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s78(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S62: underscores compared as any other character, the option not
 *        ignoring them.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s62(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "--a_b_c_d",
                                 NULL);

    return;
}

/**
 * @brief S63: underscores dropped by an option ignoring them.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s63(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    struct d_arg_option* const u = d_arg_command_add_flag(_app,
                                                          "--a_b_c_d",
                                                          NULL);

    (void)d_arg_option_set_ignore_underscore(u,
                                             true);

    return;
}

/**
 * @brief S64: underscores compared by an option ignoring case alone.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s64(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    struct d_arg_option* const u = d_arg_command_add_flag(_app,
                                                          "--a_b_c_d",
                                                          NULL);

    (void)d_arg_option_set_ignore_case(u,
                                       true);

    return;
}

/**
 * @brief S65: case compared by an option ignoring underscores alone.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s65(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    struct d_arg_option* const u = d_arg_command_add_flag(_app,
                                                          "--a_b_c_d",
                                                          NULL);

    (void)d_arg_option_set_ignore_underscore(u,
                                             true);

    return;
}

// OPTION_CASES
//   constant: the cases of d_tests_arg_suggest_options.
static const struct d_tests_arg_case OPTION_CASES[] =
{
    { "S01",
      d_tests_arg_suggest_s01,
      { "--verbse" },
      NULL,
      NULL },
    { "S02",
      d_tests_arg_suggest_s02,
      { "--verbse=3" },
      NULL,
      NULL },
    { "S04",
      d_tests_arg_suggest_s04,
      { "--dryrun" },
      NULL,
      NULL },
    { "S05",
      d_tests_arg_suggest_s05,
      { "--colr" },
      NULL,
      NULL },
    { "S06",
      d_tests_arg_suggest_s06,
      { "--colou" },
      NULL,
      NULL },
    { "S07",
      d_tests_arg_suggest_s07,
      { "--secrt" },
      NULL,
      NULL },
    { "S08",
      d_tests_arg_suggest_s08,
      { "--NAEM" },
      NULL,
      NULL },
    { "S09",
      d_tests_arg_suggest_s09,
      { "--longnam" },
      NULL,
      NULL },
    { "S10",
      d_tests_arg_suggest_s10,
      { "--extar" },
      NULL,
      NULL },
    { "S19",
      d_tests_arg_suggest_s19,
      { "--v" },
      NULL,
      NULL },
    { "S20",
      d_tests_arg_suggest_s20,
      { "-V" },
      NULL,
      NULL },
    { "S30",
      d_tests_arg_suggest_s30,
      { "--hlep" },
      NULL,
      NULL },
    { "S31",
      d_tests_arg_suggest_s31,
      { "--Verbose" },
      NULL,
      NULL },
    { "S32",
      d_tests_arg_suggest_s32,
      { "--outptu=x" },
      NULL,
      NULL },
    { "S33",
      d_tests_arg_suggest_s33,
      { "--no-lod" },
      NULL,
      NULL },
    { "S34",
      d_tests_arg_suggest_s34,
      { "--extar" },
      NULL,
      NULL },
    { "S35",
      d_tests_arg_suggest_s35,
      { "--deper" },
      NULL,
      NULL },
    { "S25",
      d_tests_arg_suggest_s25,
      { "/verbse" },
      NULL,
      NULL },
    { "S26",
      d_tests_arg_suggest_s26,
      { "/outptu:x" },
      NULL,
      NULL },
    { "S78",
      d_tests_arg_suggest_s78,
      { "--outptu:x" },
      NULL,
      NULL },
    { "S62",
      d_tests_arg_suggest_s62,
      { "--abcd" },
      NULL,
      NULL },
    { "S63",
      d_tests_arg_suggest_s63,
      { "--abce" },
      NULL,
      NULL },
    { "S64",
      d_tests_arg_suggest_s64,
      { "--ABCE" },
      NULL,
      NULL },
    { "S65",
      d_tests_arg_suggest_s65,
      { "--ABCD" },
      NULL,
      NULL }
};

// OPTION_COUNT
//   constant: how many cases OPTION_CASES holds.
static const size_t OPTION_COUNT =
    sizeof(OPTION_CASES) / sizeof(OPTION_CASES[0]);

/*
d_tests_arg_suggest_options
  An option's name mistyped: in each form an option is given, and compared as
the option compares names.
  Tests the following:
  - a long name, with a value after `=` and without; a short name given as a
    long one; a short name of another case, which is too short to resemble
    anything
  - two names of one option, the nearer named, and as near, neither
  - a name of another case, an option ignoring case, and one ignoring
    underscores
  - the help flag, a negated flag's name, an option group's option, a nested
    group's, and none of a disabled group or a hidden option
  - Windows-style options, after their slash, a value after `:` dropped, which a
    long name keeps
  - underscores compared as any character, unless the option ignores them, and
    case unless it ignores case, whichever it ignores
*/
bool
d_tests_arg_suggest_options(void)
{
    // expected
    //   the transcripts of the two passes: with the parser's suggestions --
    // CLI11's own message where they are not built -- and without them.
    const char* const expected[2] =
    {
        (D_INTERNAL_ARG_PARSER_SUGGESTIONS == 1) ? OPTIONS : OPTIONS_PLAIN,
        OPTIONS_PLAIN
    };

    return d_tests_arg_suggest_cases("OPTIONS",
                                     OPTION_CASES,
                                     OPTION_COUNT,
                                     expected);
}

//==============================================================================
// 2.  SUBCOMMANDS
//==============================================================================

/**
 * @brief S03: a subcommand's name, two letters swapped.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s03(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S13: an alias.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s13(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S23: a name, two letters swapped, among names alike.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s23(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S36: a subcommand that ignores case.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s36(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);
    (void)d_arg_command_set_ignore_case(start,
                                        true);

    return;
}

/**
 * @brief S37: an option group's subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s37(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);
    (void)d_arg_command_add_subcommand(extras,
                                       "insub",
                                       NULL);

    return;
}

/**
 * @brief S11: a hidden subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s11(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S12: a disabled subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s12(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S22: a name of one letter.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s22(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S18: a negative number.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s18(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S38: a name after `--`.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s38(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S29: a nested subcommand's name, after its parent.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s29(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    struct d_arg_command* const lone = d_arg_command_add_subcommand(_app,
                                                                    "lone",
                                                                    NULL);

    (void)d_arg_command_add_flag(lone,
                                 "--quiet",
                                 NULL);
    (void)d_arg_command_add_subcommand(lone,
                                       "inner",
                                       NULL);

    return;
}

/**
 * @brief S39: the parent's subcommand, from a subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s39(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    struct d_arg_command* const lone = d_arg_command_add_subcommand(_app,
                                                                    "lone",
                                                                    NULL);

    (void)d_arg_command_add_flag(lone,
                                 "--quiet",
                                 NULL);
    (void)d_arg_command_add_subcommand(lone,
                                       "inner",
                                       NULL);

    return;
}

/**
 * @brief S40: the parent's subcommand, from a subcommand keeping its own.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s40(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    struct d_arg_command* const lone = d_arg_command_add_subcommand(_app,
                                                                    "lone",
                                                                    NULL);

    (void)d_arg_command_add_flag(lone,
                                 "--quiet",
                                 NULL);
    (void)d_arg_command_add_subcommand(lone,
                                       "inner",
                                       NULL);
    d_arg_command_set_subcommand_fallthrough(lone,
                                             false);

    return;
}

/**
 * @brief S70: a subcommand two levels up, through each one's subcommand
 *        fallthrough.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s70(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    struct d_arg_command* const a = d_arg_command_add_subcommand(_app,
                                                                 "a",
                                                                 NULL);

    (void)d_arg_command_add_subcommand(a,
                                       "b",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "start",
                                       NULL);

    return;
}

/**
 * @brief S75: a subcommand of one letter, for an argument of two.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s75(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_subcommand(_app,
                                       "b",
                                       NULL);

    return;
}

/**
 * @brief S77: an argument of one letter, for a subcommand of two.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s77(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_subcommand(_app,
                                       "rm",
                                       NULL);

    return;
}

/**
 * @brief S76: a negative number, near a subcommand's name.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s76(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_subcommand(_app,
                                       "v1",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "v2",
                                       NULL);

    return;
}

// COMMAND_CASES
//   constant: the cases of d_tests_arg_suggest_commands.
static const struct d_tests_arg_case COMMAND_CASES[] =
{
    { "S03",
      d_tests_arg_suggest_s03,
      { "strat" },
      NULL,
      NULL },
    { "S13",
      d_tests_arg_suggest_s13,
      { "begn" },
      NULL,
      NULL },
    { "S23",
      d_tests_arg_suggest_s23,
      { "stauts" },
      NULL,
      NULL },
    { "S36",
      d_tests_arg_suggest_s36,
      { "STRAT" },
      NULL,
      NULL },
    { "S37",
      d_tests_arg_suggest_s37,
      { "insbu" },
      NULL,
      NULL },
    { "S11",
      d_tests_arg_suggest_s11,
      { "internl" },
      NULL,
      NULL },
    { "S12",
      d_tests_arg_suggest_s12,
      { "ofline" },
      NULL,
      NULL },
    { "S22",
      d_tests_arg_suggest_s22,
      { "u" },
      NULL,
      NULL },
    { "S18",
      d_tests_arg_suggest_s18,
      { "-5" },
      NULL,
      NULL },
    { "S38",
      d_tests_arg_suggest_s38,
      { "--", "strat" },
      NULL,
      NULL },
    { "S29",
      d_tests_arg_suggest_s29,
      { "lone", "innr" },
      NULL,
      NULL },
    { "S39",
      d_tests_arg_suggest_s39,
      { "lone", "strat" },
      NULL,
      NULL },
    { "S40",
      d_tests_arg_suggest_s40,
      { "lone", "strat" },
      NULL,
      NULL },
    { "S70",
      d_tests_arg_suggest_s70,
      { "a", "b", "strat" },
      NULL,
      NULL },
    { "S75",
      d_tests_arg_suggest_s75,
      { "bx" },
      NULL,
      NULL },
    { "S77",
      d_tests_arg_suggest_s77,
      { "r" },
      NULL,
      NULL },
    { "S76",
      d_tests_arg_suggest_s76,
      { "-1" },
      NULL,
      NULL }
};

// COMMAND_COUNT
//   constant: how many cases COMMAND_CASES holds.
static const size_t COMMAND_COUNT =
    sizeof(COMMAND_CASES) / sizeof(COMMAND_CASES[0]);

/*
d_tests_arg_suggest_commands
  A subcommand's name mistyped: its own and its aliases, compared as the
subcommand compares names, and those never offered.
  Tests the following:
  - a name, an alias, a subcommand that ignores case, one in an option group,
    one of a letter, and one of two for a letter
  - none of a hidden or a disabled subcommand, nor for a name too short to
    resemble one, a negative number, even one near a name, or `--` itself,
    though a name after it is named
  - a nested subcommand's name after its parent, and, from a subcommand, its
    parent's subcommands unless it keeps its own, and theirs in turn
*/
bool
d_tests_arg_suggest_commands(void)
{
    // expected
    //   the transcripts of the two passes: with the parser's suggestions --
    // CLI11's own message where they are not built -- and without them.
    const char* const expected[2] =
    {
        (D_INTERNAL_ARG_PARSER_SUGGESTIONS == 1) ? COMMANDS : COMMANDS_PLAIN,
        COMMANDS_PLAIN
    };

    return d_tests_arg_suggest_cases("COMMANDS",
                                     COMMAND_CASES,
                                     COMMAND_COUNT,
                                     expected);
}

//==============================================================================
// 3.  DISTANCES
//==============================================================================

/**
 * @brief S50: two letters swapped.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s50(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "--verbose",
                                 NULL);

    return;
}

/**
 * @brief S51: a letter more.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s51(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "--verbose",
                                 NULL);

    return;
}

/**
 * @brief S52: two letters fewer.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s52(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "--verbose",
                                 NULL);

    return;
}

/**
 * @brief S53: three letters fewer.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s53(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "--verbose",
                                 NULL);

    return;
}

/**
 * @brief S54: one edit in a name of two letters.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s54(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "--ab",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--cd",
                                 NULL);

    return;
}

/**
 * @brief S55: two edits in a name of two letters.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s55(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "--ab",
                                 NULL);

    return;
}

/**
 * @brief S56: two edits in a name of four letters.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s56(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "--abcd",
                                 NULL);

    return;
}

/**
 * @brief S61: two edits in a name of three letters, more than half of it.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s61(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "--abc",
                                 NULL);

    return;
}

/**
 * @brief S57: two names one edit away.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s57(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "--abc",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--abd",
                                 NULL);

    return;
}

/**
 * @brief S58: one name, in a command and in a subcommand it reaches.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s58(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "--force",
                                 NULL);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

// DISTANCE_CASES
//   constant: the cases of d_tests_arg_suggest_distances.
static const struct d_tests_arg_case DISTANCE_CASES[] =
{
    { "S50",
      d_tests_arg_suggest_s50,
      { "--vebrose" },
      NULL,
      NULL },
    { "S51",
      d_tests_arg_suggest_s51,
      { "--verbosee" },
      NULL,
      NULL },
    { "S52",
      d_tests_arg_suggest_s52,
      { "--vrbse" },
      NULL,
      NULL },
    { "S53",
      d_tests_arg_suggest_s53,
      { "--vrbs" },
      NULL,
      NULL },
    { "S54",
      d_tests_arg_suggest_s54,
      { "--ac" },
      NULL,
      NULL },
    { "S55",
      d_tests_arg_suggest_s55,
      { "--xy" },
      NULL,
      NULL },
    { "S56",
      d_tests_arg_suggest_s56,
      { "--abxy" },
      NULL,
      NULL },
    { "S61",
      d_tests_arg_suggest_s61,
      { "--axz" },
      NULL,
      NULL },
    { "S57",
      d_tests_arg_suggest_s57,
      { "--abx" },
      NULL,
      NULL },
    { "S58",
      d_tests_arg_suggest_s58,
      { "remote", "--forse" },
      NULL,
      NULL }
};

// DISTANCE_COUNT
//   constant: how many cases DISTANCE_CASES holds.
static const size_t DISTANCE_COUNT =
    sizeof(DISTANCE_CASES) / sizeof(DISTANCE_CASES[0]);

/*
d_tests_arg_suggest_distances
  How near a name must be: within two edits -- a swap of neighbours one -- and
half the longer name, nearer than any other.
  Tests the following:
  - a swap of neighbours, a letter more, two letters fewer, and three edits, too
    many
  - one edit in two letters, two in two, two in three, two in four
  - two names as near, neither named; one name as near by two ways, named
*/
bool
d_tests_arg_suggest_distances(void)
{
    // expected
    //   the transcripts of the two passes: with the parser's suggestions --
    // CLI11's own message where they are not built -- and without them.
    const char* const expected[2] =
    {
        (D_INTERNAL_ARG_PARSER_SUGGESTIONS == 1) ? DISTANCES : DISTANCES_PLAIN,
        DISTANCES_PLAIN
    };

    return d_tests_arg_suggest_cases("DISTANCES",
                                     DISTANCE_CASES,
                                     DISTANCE_COUNT,
                                     expected);
}

//==============================================================================
// 4.  WHERE A NAME IS LOOKED FOR
//==============================================================================

/**
 * @brief S14: a subcommand's option, after the subcommand passes it up.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s14(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S15: the root's option, after a subcommand passes it up.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s15(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S59: a flag of a subcommand two deep.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s59(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    struct d_arg_command* const a = d_arg_command_add_subcommand(_app,
                                                                 "a",
                                                                 NULL);

    d_arg_command_set_fallthrough(a,
                                  true);

    struct d_arg_command* const b = d_arg_command_add_subcommand(a,
                                                                 "b",
                                                                 NULL);

    d_arg_command_set_fallthrough(b,
                                  true);
    (void)d_arg_command_add_flag(b,
                                 "--zzz",
                                 NULL);

    return;
}

/**
 * @brief S28: a subcommand that does not fall through.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s28(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    struct d_arg_command* const lone = d_arg_command_add_subcommand(_app,
                                                                    "lone",
                                                                    NULL);

    (void)d_arg_command_add_flag(lone,
                                 "--quiet",
                                 NULL);
    (void)d_arg_command_add_subcommand(lone,
                                       "inner",
                                       NULL);

    return;
}

/**
 * @brief S68: a subcommand that does not fall through, its parent's option
 *        not offered.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s68(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    struct d_arg_command* const lone = d_arg_command_add_subcommand(_app,
                                                                    "lone",
                                                                    NULL);

    (void)d_arg_command_add_flag(lone,
                                 "--quiet",
                                 NULL);
    (void)d_arg_command_add_subcommand(lone,
                                       "inner",
                                       NULL);

    return;
}

/**
 * @brief S71: a positional after a subcommand that passes it up, not offered
 *        the subcommand's option.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s71(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S72: a Windows-style option of a subcommand the parse went on to.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s72(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);
    d_arg_command_set_windows_style(_app,
                                    true);
    d_arg_command_set_windows_style(remote,
                                    true);

    return;
}

/**
 * @brief S74: a subcommand of a subcommand the parse went on to.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s74(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);
    (void)d_arg_command_add_subcommand(remote,
                                       "add",
                                       NULL);

    return;
}

/**
 * @brief S73: a short argument's name, all after its dash, as near two names.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s73(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-b",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--ab",
                                 NULL);

    return;
}

/**
 * @brief S60: an option given before its subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s60(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    struct d_arg_command* const s = d_arg_command_add_subcommand(_app,
                                                                 "s",
                                                                 NULL);

    (void)d_arg_command_add_flag(s,
                                 "--in",
                                 NULL);

    return;
}

/**
 * @brief S16: two arguments, each named.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s16(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S41: two subcommands' names, each named.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s41(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S24: a flag given a value, then a mistyped one.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s24(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    return;
}

/**
 * @brief S21: an argument the root refuses at once.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s21(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);
    d_arg_command_set_allow_extras(_app,
                                   (int)D_ARG_EXTRAS_ERROR_IMMEDIATELY);

    return;
}

/**
 * @brief S42: an argument a subcommand refuses at once.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s42(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);

    struct d_arg_command* const lone = d_arg_command_add_subcommand(_app,
                                                                    "lone",
                                                                    NULL);

    (void)d_arg_command_add_flag(lone,
                                 "--quiet",
                                 NULL);
    (void)d_arg_command_add_subcommand(lone,
                                       "inner",
                                       NULL);
    d_arg_command_set_allow_extras(lone,
                                   (int)D_ARG_EXTRAS_ERROR_IMMEDIATELY);

    return;
}

/**
 * @brief S27: the rest after a positional, with positionals at the end.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s27(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "file",
                                   NULL);
    d_arg_command_set_positionals_at_end(_app,
                                         true);

    return;
}

/**
 * @brief S43: arguments the root captures.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_suggest_s43(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "-o,--output",
                                   NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 NULL);
    (void)d_arg_command_add_flag(_app,
                                 "--colour,--color",
                                 NULL);

    struct d_arg_option* const secret = d_arg_command_add_flag(_app,
                                                               "--secret",
                                                               NULL);

    (void)d_arg_option_set_group(secret,
                                 "");

    struct d_arg_option* const name = d_arg_command_add_flag(_app,
                                                             "--Name",
                                                             NULL);

    (void)d_arg_option_set_ignore_case(name,
                                       true);

    struct d_arg_option* const lname = d_arg_command_add_flag(_app,
                                                              "--long_name",
                                                              NULL);

    (void)d_arg_option_set_ignore_underscore(lname,
                                             true);

    struct d_arg_command* const extras =
        d_arg_command_add_option_group(_app,
                                       "Extras",
                                       NULL);

    (void)d_arg_command_add_flag(extras,
                                 "--extra",
                                 NULL);

    struct d_arg_command* const start = d_arg_command_add_subcommand(_app,
                                                                     "start",
                                                                     NULL);

    (void)d_arg_command_add_alias(start,
                                  "begin");
    (void)d_arg_command_add_subcommand(_app,
                                       "stop",
                                       NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "status",
                                       NULL);

    struct d_arg_command* const internal =
        d_arg_command_add_subcommand(_app,
                                     "internal",
                                     NULL);

    (void)d_arg_command_set_group(internal,
                                  "");

    struct d_arg_command* const offline =
        d_arg_command_add_subcommand(_app,
                                     "offline",
                                     NULL);

    d_arg_command_set_disabled(offline,
                               true);

    struct d_arg_command* const remote = d_arg_command_add_subcommand(_app,
                                                                      "remote",
                                                                      NULL);

    d_arg_command_set_fallthrough(remote,
                                  true);
    (void)d_arg_command_add_flag(remote,
                                 "--force",
                                 NULL);
    d_arg_command_set_allow_extras(_app,
                                   (int)D_ARG_EXTRAS_CAPTURE);

    return;
}

// REACH_CASES
//   constant: the cases of d_tests_arg_suggest_reach.
static const struct d_tests_arg_case REACH_CASES[] =
{
    { "S14",
      d_tests_arg_suggest_s14,
      { "remote", "--forse" },
      NULL,
      NULL },
    { "S15",
      d_tests_arg_suggest_s15,
      { "remote", "--verbse" },
      NULL,
      NULL },
    { "S59",
      d_tests_arg_suggest_s59,
      { "a", "b", "--zzy" },
      NULL,
      NULL },
    { "S28",
      d_tests_arg_suggest_s28,
      { "lone", "--quite" },
      NULL,
      NULL },
    { "S68",
      d_tests_arg_suggest_s68,
      { "lone", "--verbse" },
      NULL,
      NULL },
    { "S71",
      d_tests_arg_suggest_s71,
      { "remote", "forse" },
      NULL,
      NULL },
    { "S72",
      d_tests_arg_suggest_s72,
      { "remote", "/forse" },
      NULL,
      NULL },
    { "S74",
      d_tests_arg_suggest_s74,
      { "remote", "ad" },
      NULL,
      NULL },
    { "S73",
      d_tests_arg_suggest_s73,
      { "-xb" },
      NULL,
      NULL },
    { "S60",
      d_tests_arg_suggest_s60,
      { "--in", "s" },
      NULL,
      NULL },
    { "S16",
      d_tests_arg_suggest_s16,
      { "--verbse", "--outptu", "x" },
      NULL,
      NULL },
    { "S41",
      d_tests_arg_suggest_s41,
      { "strat", "stpo" },
      NULL,
      NULL },
    { "S24",
      d_tests_arg_suggest_s24,
      { "--colour=red", "--verbse" },
      NULL,
      NULL },
    { "S21",
      d_tests_arg_suggest_s21,
      { "--verbse" },
      NULL,
      NULL },
    { "S42",
      d_tests_arg_suggest_s42,
      { "lone", "--quite" },
      NULL,
      NULL },
    { "S27",
      d_tests_arg_suggest_s27,
      { "f1", "--verbse" },
      NULL,
      NULL },
    { "S43",
      d_tests_arg_suggest_s43,
      { "--verbse", "strat" },
      NULL,
      NULL }
};

// REACH_COUNT
//   constant: how many cases REACH_CASES holds.
static const size_t REACH_COUNT = sizeof(REACH_CASES) / sizeof(REACH_CASES[0]);

/*
d_tests_arg_suggest_reach
  The command that reports the arguments, and the names it gathers: its own, the
parents it falls through to, the subcommands the parse went on to; several
arguments; the ends of a parse other than its last.
  Tests the following:
  - an argument a subcommand passes up, named from the root and from the
    subcommand, and from subcommands two deep; a Windows-style one; a
    subcommand's name; and no option for a positional
  - a subcommand that keeps what it does not take, and its parent's options not
    offered
  - an option given before its subcommand, which is not suggested to itself
  - several arguments, each named for which it is, and a value of an option
    given; a short argument's name, all after its dash
  - an argument at once, from the root and from a subcommand; the rest after a
    positional, with positionals at the end; and nothing where extras are
    captured
*/
bool
d_tests_arg_suggest_reach(void)
{
    // expected
    //   the transcripts of the two passes: with the parser's suggestions --
    // CLI11's own message where they are not built -- and without them.
    const char* const expected[2] =
    {
        (D_INTERNAL_ARG_PARSER_SUGGESTIONS == 1) ? REACH : REACH_PLAIN,
        REACH_PLAIN
    };

    return d_tests_arg_suggest_cases("REACH",
                                     REACH_CASES,
                                     REACH_COUNT,
                                     expected);
}

//==============================================================================
// 5.  THE EDIT DISTANCE
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_SUGGESTIONS == 1)

// DISTANCE_LONGEST / DISTANCE_SHOWN
//   constant: the longest text the distance's checks make -- a word of the
// long pairs, grown by its edits -- and the most mismatches the section
// prints, of all it counts.
enum
{
    DISTANCE_LONGEST = 24,
    DISTANCE_SHOWN   = 8
};

// DISTANCE_LIMITS
//   constant: the limits each pair is measured within: none to past the
// reach of 8 the band is computed for, and the largest a `size_t` holds,
// which is taken as 8.
static const size_t DISTANCE_LIMITS[] = { 0u,
                                          1u,
                                          2u,
                                          3u,
                                          4u,
                                          5u,
                                          6u,
                                          7u,
                                          8u,
                                          9u,
                                          (size_t)-1 };

// DISTANCE_LIMIT_COUNT
//   constant: how many limits DISTANCE_LIMITS holds.
static const size_t DISTANCE_LIMIT_COUNT =
    sizeof(DISTANCE_LIMITS) / sizeof(DISTANCE_LIMITS[0]);

/**
 * @brief The smaller of two counts.
 *
 * @param[in] _a  one.
 * @param[in] _b  the other.
 * @return the smaller.
 */
static size_t
d_tests_arg_suggest_smaller(
    size_t _a,
    size_t _b
)
{
    return (_a < _b) ? _a : _b;
}

/**
 * @brief The optimal string alignment distance between two texts, by the
 *        whole table -- Levenshtein's, with a swap of neighbours one edit --
 *        the reference d_arg_internal_edit_distance's band is held to.
 *
 * @param[in] _a  the first text, at most DISTANCE_LONGEST bytes.
 * @param[in] _b  the second, at most as long.
 * @return the distance.
 */
static size_t
d_tests_arg_suggest_osa(
    struct d_pack_text _a,
    struct d_pack_text _b
)
{
    size_t table[DISTANCE_LONGEST + 1][DISTANCE_LONGEST + 1] = { { 0u } };

    // as many deletions, and as many insertions, as letters
    for (size_t i = 0u; i <= _a.length; ++i)
    {
        table[i][0] = i;
    }

    for (size_t j = 0u; j <= _b.length; ++j)
    {
        table[0][j] = j;
    }

    // each cell from those before it
    for (size_t i = 1u; i <= _a.length; ++i)
    {
        for (size_t j = 1u; j <= _b.length; ++j)
        {
            const size_t replace = table[i - 1u][j - 1u] +
                                   ((_a.data[i - 1u] != _b.data[j - 1u])
                                    ? 1u
                                    : 0u);
            size_t       best    = d_tests_arg_suggest_smaller(
                                       replace,
                                       d_tests_arg_suggest_smaller(
                                           table[i - 1u][j] + 1u,
                                           table[i][j - 1u] + 1u));

            // two neighbours swapped
            if ( (i > 1u)                             &&
                 (j > 1u)                             &&
                 (_a.data[i - 1u] == _b.data[j - 2u]) &&
                 (_a.data[i - 2u] == _b.data[j - 1u]) )
            {
                best = d_tests_arg_suggest_smaller(best,
                                                   table[i - 2u][j - 2u] + 1u);
            }

            table[i][j] = best;
        }
    }

    return table[_a.length][_b.length];
}

/**
 * @brief Measures one pair within each of DISTANCE_LIMITS, as the band and
 *        as the whole table, which must agree: the distance, or the limit --
 *        at most 8 -- plus one when it is further.
 *
 * @param[in]     _a       the first text.
 * @param[in]     _b       the second.
 * @param[in,out] _misses  the mismatches so far; the first few printed.
 */
static void
d_tests_arg_suggest_pair(
    struct d_pack_text _a,
    struct d_pack_text _b,
    size_t*            _misses
)
{
    const size_t whole = d_tests_arg_suggest_osa(_a,
                                                 _b);

    // each limit
    for (size_t at = 0u; at < DISTANCE_LIMIT_COUNT; ++at)
    {
        const size_t limit = DISTANCE_LIMITS[at];
        const size_t want  = d_tests_arg_suggest_smaller(
                                 whole,
                                 d_tests_arg_suggest_smaller(limit,
                                                             8u) + 1u);
        const size_t got   = d_arg_internal_edit_distance(_a,
                                                          _b,
                                                          limit);

        // a mismatch, counted, and shown while few
        if (got != want)
        {
            if (*_misses < (size_t)DISTANCE_SHOWN)
            {
                printf("    distance of [%.*s] and [%.*s] within %lu: %lu, "
                       "by the whole table %lu\n",
                       (int)_a.length,
                       _a.data,
                       (int)_b.length,
                       _b.data,
                       (unsigned long)limit,
                       (unsigned long)got,
                       (unsigned long)want);
            }

            (*_misses)++;
        }
    }

    return;
}

/**
 * @brief Writes the word of an index: the words over `a`, `b` and `c` in
 *        order of length, then of their letters -- the empty word, the
 *        three of one letter, the nine of two, and so on.
 *
 * @param[in]  _index  the word's index.
 * @param[out] _out    receives its letters.
 * @return its length.
 */
static size_t
d_tests_arg_suggest_word(
    size_t _index,
    char*  _out
)
{
    size_t length = 0u;
    size_t first  = 0u;
    size_t count  = 1u;

    // the length whose words the index falls among
    while (_index >= (first + count))
    {
        first += count;
        count *= 3u;
        length++;
    }

    size_t rest = _index - first;

    // its letters, the last first
    for (size_t at = length; at > 0u; --at)
    {
        _out[at - 1u] = (char)('a' + (int)(rest % 3u));
        rest         /= 3u;
    }

    return length;
}

/**
 * @brief Every pair of words over `a`, `b` and `c` of up to four letters,
 *        measured within each limit.
 *
 * @param[in,out] _misses  the mismatches so far.
 */
static void
d_tests_arg_suggest_words(
    size_t* _misses
)
{
    char left[4]  = { 0 };
    char right[4] = { 0 };

    // each first word, then each second: the 121 of up to four letters
    for (size_t i = 0u; i < 121u; ++i)
    {
        const size_t a = d_tests_arg_suggest_word(i,
                                                  left);

        for (size_t j = 0u; j < 121u; ++j)
        {
            const size_t b = d_tests_arg_suggest_word(j,
                                                      right);

            d_tests_arg_suggest_pair(d_pack_text_from_span(left,
                                                           a),
                                     d_pack_text_from_span(right,
                                                           b),
                                     _misses);
        }
    }

    return;
}

/**
 * @brief The next number of a linear congruential sequence, of 15 bits.
 *
 * @param[in,out] _state  the sequence's state.
 * @return the number.
 */
static size_t
d_tests_arg_suggest_next(
    unsigned long* _state
)
{
    *_state = ((*_state * 1103515245ul) + 12345ul) & 0x7FFFFFFFul;

    return (size_t)((*_state >> 16u) & 0x7FFFul);
}

/**
 * @brief One edit of a word, at a place the sequence picks: a letter
 *        replaced, put in, taken out, or swapped with the next -- each within
 *        DISTANCE_LONGEST letters, and none where the word is too short for
 *        it.
 *
 * @param[in,out] _word    the word.
 * @param[in,out] _length  its length.
 * @param[in,out] _state   the sequence's state.
 */
static void
d_tests_arg_suggest_edit(
    char*          _word,
    size_t*        _length,
    unsigned long* _state
)
{
    const size_t kind   = d_tests_arg_suggest_next(_state) % 4u;
    const size_t letter = d_tests_arg_suggest_next(_state) % 3u;
    const size_t at     = d_tests_arg_suggest_next(_state) %
                          (*_length + 1u);

    // a letter replaced
    if ( (kind == 0u) &&
         (at < *_length) )
    {
        _word[at] = (char)('a' + (int)letter);

        return;
    }

    // a letter put in
    if ( (kind == 1u) &&
         (*_length < (size_t)DISTANCE_LONGEST) )
    {
        memmove(_word + at + 1u,
                _word + at,
                *_length - at);
        _word[at] = (char)('a' + (int)letter);
        (*_length)++;

        return;
    }

    // a letter taken out
    if ( (kind == 2u) &&
         (at < *_length) )
    {
        memmove(_word + at,
                _word + at + 1u,
                *_length - at - 1u);
        (*_length)--;

        return;
    }

    // a letter swapped with the next
    if ( (kind == 3u) &&
         ((at + 1u) < *_length) )
    {
        const char kept = _word[at];

        _word[at]      = _word[at + 1u];
        _word[at + 1u] = kept;
    }

    return;
}

/**
 * @brief Pairs of longer words, each a word over `a`, `b` and `c` of up to
 *        fourteen letters and that word after up to ten edits -- distances
 *        on both sides of every reach, and lengths far apart -- measured
 *        within each limit.
 *
 * @param[in,out] _misses  the mismatches so far.
 */
static void
d_tests_arg_suggest_long(
    size_t* _misses
)
{
    unsigned long state = 2026ul;

    // each pair, from the sequence
    for (size_t pair = 0u; pair < 3000u; ++pair)
    {
        char         first[DISTANCE_LONGEST]  = { 0 };
        char         second[DISTANCE_LONGEST] = { 0 };
        const size_t length = d_tests_arg_suggest_next(&state) % 15u;
        const size_t edits  = d_tests_arg_suggest_next(&state) % 11u;
        size_t       grown  = length;

        // the word, then its copy, edited
        for (size_t at = 0u; at < length; ++at)
        {
            const size_t letter = d_tests_arg_suggest_next(&state) % 3u;

            first[at] = (char)('a' + (int)letter);
        }

        memcpy(second,
               first,
               length);

        for (size_t edit = 0u; edit < edits; ++edit)
        {
            d_tests_arg_suggest_edit(second,
                                     &grown,
                                     &state);
        }

        d_tests_arg_suggest_pair(d_pack_text_from_span(first,
                                                       length),
                                 d_pack_text_from_span(second,
                                                       grown),
                                 _misses);
    }

    return;
}

/*
d_tests_arg_suggest_edit_distance
  The distance suggestions are measured by (d_arg_internal_edit_distance),
which only the band of the table within the limit computes, held to the
whole table: every pair of words over three letters of up to four letters,
and pairs of longer words some edits apart, each within limits from none
to past the reach of 8.
  Tests the following:
  - the distance, swaps of neighbours one edit, wherever it is within the
    limit
  - the limit plus one wherever the distance is further, lengths further
    apart than the limit among them
  - a limit past 8 taken as 8
*/
bool
d_tests_arg_suggest_edit_distance(void)
{
    size_t misses = 0u;

    d_tests_arg_suggest_words(&misses);
    d_tests_arg_suggest_long(&misses);

    // how many, when more than were shown
    if (misses > (size_t)DISTANCE_SHOWN)
    {
        printf("    and %lu more mismatches\n",
               (unsigned long)(misses - (size_t)DISTANCE_SHOWN));
    }

    return (misses == 0u);
}

#endif  // D_INTERNAL_ARG_PARSER_SUGGESTIONS

//==============================================================================
// 6.  THE SUITE
//==============================================================================

// d_tests_arg_suggest
//   constant: the suite, in the header's order.
const struct d_tests_section d_tests_arg_suggest[] =
{
    { "arg_suggest_options",
      d_tests_arg_suggest_options },
    { "arg_suggest_commands",
      d_tests_arg_suggest_commands },
    { "arg_suggest_distances",
      d_tests_arg_suggest_distances },
    { "arg_suggest_reach",
      d_tests_arg_suggest_reach },
#if (D_INTERNAL_ARG_PARSER_SUGGESTIONS == 1)
    { "arg_suggest_edit_distance",
      d_tests_arg_suggest_edit_distance },
#endif  // D_INTERNAL_ARG_PARSER_SUGGESTIONS
};

// d_tests_arg_suggest_count
//   constant: how many sections the table above holds.
const size_t d_tests_arg_suggest_count =
    sizeof(d_tests_arg_suggest) / sizeof(d_tests_arg_suggest[0]);
