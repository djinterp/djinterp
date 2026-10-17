/*******************************************************************************
* djinterp [parse]                                           arg_process_tests.c
*
* The suite of the processing that ends the argument parser's parse,
* declared in arg_process_tests.h.
*   Each section is a case table (arg_tests_support.h): each case makes its
* options and commands as CLI11's App("d", "prog") would, on a run's parser
* of its own, parses its arguments -- with the environment variables it
* names set for the parse -- and is written as the processing suite's CLI11
* reference writes it: a line of its tag and arguments, what its callbacks
* noted, and the parse as the runs report one, the section's cases compared
* whole. A callback's note names its command or option by the label the run
* reports it under. The cases of the processing probes keep their tags
* (R01-R40, S01-S37).
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_process_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.10.15
*******************************************************************************/
#include "./arg_process_tests.h"  // corresponding header
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
// djinterp
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text, d_pack_sink
#include "../../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // the knobs
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // commands, their settings
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // statuses, sinks, priorities
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // options, their settings
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // failing from a callback
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator.h"  // d_arg_validator_positive_number
#include "../../../c/tests_section.h"  // d_tests_section
#include "./arg_tests_support.h"  // case tables, notes, labels


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// REQUIRED
//   constant: what the cases of d_tests_arg_process_required come to, as CLI11
// processes them.
static const char REQUIRED[] =
    "R01 []\n"
    "ERR RequiredError 106: --x is required\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt x n=0 res=[]\n"
    "R13 []\n"
    "ERR RequiredError 106: sub is required\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "R14 []\n"
    "ERR RequiredError 106: [Option Group: G] is required\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd G n=1 miss=[] subs=[] order=[]\n"
    "opt g n=0 res=[]\n"
    "S06 []\n"
    "ERR RequiredError 106: --x is required\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt x n=0 res=[]\n"
    "S14 [a]\n"
    "ERR ArgumentMismatch 114: pos: At least 2 required but received 1\n"
    "cmd app n=1 miss=[] subs=[] order=[pos]\n"
    "opt pos n=1 res=[a]\n"
    "S15 []\n"
    "ERR RequiredError 106: pos is required\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt pos n=0 res=[]\n"
    "S30 [sub deep]\n"
    "ERR RequiredError 106: --o is required\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[] subs=[deep] order=[]\n"
    "cmd deep n=1 miss=[] subs=[] order=[]\n"
    "opt o n=0 res=[]\n"
    "S34 [--x 1]\n"
    "ERR ArgumentMismatch 114: --x: At least 2 required but received 1\n"
    "cmd app n=1 miss=[] subs=[] order=[--x]\n"
    "opt x n=1 res=[1]\n";

// RELATIONS
//   constant: what the cases of d_tests_arg_process_relations come to, as CLI11
// processes them.
static const char RELATIONS[] =
    "R02 [--a]\n"
    "ERR RequiresError 107: --a requires --b\n"
    "cmd app n=1 miss=[] subs=[] order=[--a]\n"
    "opt a n=1 res=[true]\n"
    "opt b n=0 res=[]\n"
    "R03 [--d --c]\n"
    "ERR ExcludesError 108: --c excludes --d\n"
    "cmd app n=1 miss=[] subs=[] order=[--d|--c]\n"
    "opt c n=1 res=[true]\n"
    "opt d n=1 res=[true]\n"
    "R04 [--top sub]\n"
    "ERR ExcludesError 108: sub excludes --top\n"
    "cmd app n=1 miss=[] subs=[sub] order=[--top]\n"
    "cmd sub n=1 miss=[] subs=[] order=[]\n"
    "opt top n=1 res=[true]\n"
    "R05 [--top]\n"
    "cmd app n=1 miss=[] subs=[] order=[--top]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt top n=1 res=[true]\n"
    "R06 [sub1]\n"
    "ERR RequiresError 107: sub1 requires sub2\n"
    "cmd app n=1 miss=[] subs=[sub1] order=[]\n"
    "cmd sub1 n=1 miss=[] subs=[] order=[]\n"
    "cmd sub2 n=0 miss=[] subs=[] order=[]\n"
    "R36 [s1 s2]\n"
    "ERR ExcludesError 108: s1 excludes s2\n"
    "cmd app n=1 miss=[] subs=[s1|s2] order=[]\n"
    "cmd s1 n=1 miss=[] subs=[] order=[]\n"
    "cmd s2 n=1 miss=[] subs=[] order=[]\n"
    "S07 [sub]\n"
    "ERR RequiresError 107: sub requires --o\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[] subs=[] order=[]\n"
    "opt o n=0 res=[]\n"
    "S09 [--g --o]\n"
    "ERR ExcludesError 108: [Option Group: G] excludes --o\n"
    "cmd app n=1 miss=[] subs=[] order=[--o]\n"
    "cmd G n=1 miss=[] subs=[] order=[--g]\n"
    "opt g n=1 res=[true]\n"
    "opt o n=1 res=[true]\n"
    "S29 [--a]\n"
    "ERR RequiresError 107: --a requires --b\n"
    "cmd app n=1 miss=[] subs=[] order=[--a]\n"
    "opt a n=1 res=[true]\n"
    "opt b n=0 res=[]\n"
    "S33 [--g --h1]\n"
    "ERR ExcludesError 108: [Option Group: G] excludes [Option Group: H]\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd G n=1 miss=[] subs=[] order=[--g]\n"
    "cmd H n=1 miss=[] subs=[] order=[--h1]\n"
    "opt g n=1 res=[true]\n"
    "opt h1 n=1 res=[true]\n"
    "R42 []\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt o n=0 res=[]\n"
    "R46 [--top t s]\n"
    "ERR ExcludesError 108: s excludes t\n"
    "cmd app n=1 miss=[] subs=[t|s] order=[--top]\n"
    "cmd s n=1 miss=[] subs=[] order=[]\n"
    "cmd t n=1 miss=[] subs=[] order=[]\n"
    "opt top n=1 res=[true]\n"
    "R47 [--o]\n"
    "cmd app n=1 miss=[] subs=[] order=[--o]\n"
    "cmd G n=1 miss=[] subs=[] order=[]\n"
    "opt g n=0 res=[]\n"
    "opt o n=1 res=[true]\n"
    "R48 []\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd G n=1 miss=[] subs=[] order=[]\n"
    "opt g n=0 res=[]\n"
    "opt o n=0 res=[]\n";

// SUBCOMMAND_COUNTS
//   constant: what the cases of d_tests_arg_process_subcommand_counts come to,
// as CLI11 processes them.
static const char SUBCOMMAND_COUNTS[] =
    "R07 []\n"
    "ERR RequiredError 106: A subcommand is required\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd s n=0 miss=[] subs=[] order=[]\n"
    "R08 [s]\n"
    "ERR RequiredError 106: Requires at least 2 subcommands\n"
    "cmd app n=1 miss=[] subs=[s] order=[]\n"
    "cmd s n=1 miss=[] subs=[] order=[]\n"
    "cmd t n=0 miss=[] subs=[] order=[]\n"
    "S21 [sub]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: sub\n"
    "cmd app n=1 miss=[sub] subs=[] order=[]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "S22 [sub]\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd sub n=1 miss=[] subs=[] order=[]\n"
    "S31 [sub]\n"
    "ERR RequiredError 106: A subcommand is required\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[] subs=[] order=[]\n"
    "cmd deep n=0 miss=[] subs=[] order=[]\n"
    "R43 [s]\n"
    "cmd app n=1 miss=[] subs=[s] order=[]\n"
    "cmd s n=1 miss=[] subs=[] order=[]\n";

// OPTION_COUNTS
//   constant: what the cases of d_tests_arg_process_option_counts come to, as
// CLI11 processes them.
static const char OPTION_COUNTS[] =
    "R09a []\n"
    "ERR RequiredError 106: Exactly 1 option from [--a,--b,-c] is required\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt a n=0 res=[]\n"
    "opt b n=0 res=[]\n"
    "opt c n=0 res=[]\n"
    "R09b [--a --b]\n"
    "ERR RequiredError 106: Exactly 1 option from [--a,--b,-c] is required but "
    "2 were given\n"
    "cmd app n=1 miss=[] subs=[] order=[--a|--b]\n"
    "opt a n=1 res=[true]\n"
    "opt b n=1 res=[true]\n"
    "opt c n=0 res=[]\n"
    "R09c []\n"
    "ERR RequiredError 106: At least 1 option from [--a,--b,-c] is required\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt a n=0 res=[]\n"
    "opt b n=0 res=[]\n"
    "opt c n=0 res=[]\n"
    "R09d [--a]\n"
    "ERR RequiredError 106: Requires at least 2 options used but only 1 were "
    "given from [--a,--b,-c]\n"
    "cmd app n=1 miss=[] subs=[] order=[--a]\n"
    "opt a n=1 res=[true]\n"
    "opt b n=0 res=[]\n"
    "opt c n=0 res=[]\n"
    "R09e [--a --b]\n"
    "ERR RequiredError 106: Requires at most 1 options be given from "
    "[--a,--b,-c]\n"
    "cmd app n=1 miss=[] subs=[] order=[--a|--b]\n"
    "opt a n=1 res=[true]\n"
    "opt b n=1 res=[true]\n"
    "opt c n=0 res=[]\n"
    "R09f [--a --b -c]\n"
    "ERR RequiredError 106: Requires at most 2 options be used but 3 were "
    "given from [--a,--b,-c]\n"
    "cmd app n=1 miss=[] subs=[] order=[--a|--b|-c]\n"
    "opt a n=1 res=[true]\n"
    "opt b n=1 res=[true]\n"
    "opt c n=1 res=[true]\n"
    "R10 []\n"
    "ERR RequiredError 106: Exactly 1 option from [--a,--f{5},,[Option Group: "
    "G]] is required\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd G n=1 miss=[] subs=[] order=[]\n"
    "opt a n=0 res=[]\n"
    "opt f n=0 res=[]\n"
    "opt g n=0 res=[]\n"
    "R11 [--g --a]\n"
    "ERR RequiredError 106: Exactly 1 option from [--a,[Option Group: G]] is "
    "required but 2 were given\n"
    "cmd app n=1 miss=[] subs=[] order=[--a]\n"
    "cmd G n=1 miss=[] subs=[] order=[--g]\n"
    "opt a n=1 res=[true]\n"
    "opt g n=1 res=[true]\n"
    "R12 [--a]\n"
    "cmd app n=1 miss=[] subs=[] order=[--a]\n"
    "cmd G n=1 miss=[] subs=[] order=[]\n"
    "opt a n=1 res=[true]\n"
    "opt g n=0 res=[]\n"
    "S32 [--a --b]\n"
    "cmd app n=1 miss=[] subs=[] order=[--a|--b]\n"
    "opt a n=1 res=[true]\n"
    "opt b n=1 res=[true]\n"
    "R44 [--a -c]\n"
    "cmd app n=1 miss=[] subs=[] order=[--a|-c]\n"
    "opt a n=1 res=[true]\n"
    "opt b n=0 res=[]\n"
    "opt c n=1 res=[true]\n"
    "R49 [--a]\n"
    "cmd app n=1 miss=[] subs=[] order=[--a]\n"
    "cmd G n=1 miss=[] subs=[] order=[]\n"
    "opt a n=1 res=[true]\n"
    "opt g n=0 res=[]\n";

// GROUP_COUNTS
//   constant: what the cases of d_tests_arg_process_group_counts come to, as
// CLI11 processes them.
static const char GROUP_COUNTS[] =
    "S08a []\n"
    "ERR RequiredError 106: Exactly 1 option from [--g1,--g2] is required\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd G n=1 miss=[] subs=[] order=[]\n"
    "opt g1 n=0 res=[]\n"
    "opt g2 n=0 res=[]\n"
    "S08b [--g1 --g2]\n"
    "ERR RequiredError 106: Exactly 1 option from [--g1,--g2] is required but "
    "2 were given\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd G n=1 miss=[] subs=[] order=[--g1|--g2]\n"
    "opt g1 n=1 res=[true]\n"
    "opt g2 n=1 res=[true]\n"
    "S08c [--g2]\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd G n=1 miss=[] subs=[] order=[--g2]\n"
    "opt g1 n=0 res=[]\n"
    "opt g2 n=1 res=[true]\n";

// HELP
//   constant: what the cases of d_tests_arg_process_help come to, as CLI11
// processes them.
static const char HELP[] =
    "R16 [-h zz]\n"
    "ERR CallForHelp 0: This should be caught in your main function, see "
    "examples\n"
    "cmd app n=1 miss=[zz] subs=[] order=[--help]\n"
    "opt x n=0 res=[]\n"
    "R17 [--help-all -h]\n"
    "ERR CallForAllHelp 0: This should be caught in your main function, see "
    "examples\n"
    "cmd app n=1 miss=[] subs=[] order=[--help-all|--help]\n"
    "R18 [sub -h]\n"
    "ERR CallForHelp 0: This should be caught in your main function, see "
    "examples\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[] subs=[] order=[--help]\n"
    "R19 [--version]\n"
    "ERR CallForVersion 0: 1.2.3\n"
    "cmd app n=1 miss=[] subs=[] order=[--version]\n"
    "S01 [sub -h]\n"
    "ERR CallForHelp 0: This should be caught in your main function, see "
    "examples\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[] subs=[] order=[--help]\n"
    "opt x n=0 res=[]\n"
    "S02 [--version -h]\n"
    "ERR CallForHelp 0: This should be caught in your main function, see "
    "examples\n"
    "cmd app n=1 miss=[] subs=[] order=[--version|--help]\n"
    "S03 [-h --version]\n"
    "ERR CallForHelp 0: This should be caught in your main function, see "
    "examples\n"
    "cmd app n=1 miss=[] subs=[] order=[--help|--version]\n"
    "S04 [sub --version]\n"
    "ERR CallForVersion 0: 2.0\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[] subs=[] order=[--version]\n"
    "S05 [-V]\n"
    "ERR CallForVersion 0: fn-1\n"
    "cmd app n=1 miss=[] subs=[] order=[--version]\n"
    "S27 [sub --help-all]\n"
    "ERR CallForAllHelp 0: This should be caught in your main function, see "
    "examples\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[] subs=[] order=[--help-all]\n"
    "S28 []\n"
    "ERR CallForHelp 0: This should be caught in your main function, see "
    "examples\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n";

// CALLBACKS
//   constant: what the cases of d_tests_arg_process_callbacks come to, as CLI11
// processes them.
static const char CALLBACKS[] =
    "R20 [--x 1 sub --y 2]\n"
    "y 2\n"
    "complete sub\n"
    "x 1\n"
    "final sub\n"
    "final app\n"
    "cmd app n=1 miss=[] subs=[sub] order=[--x]\n"
    "cmd sub n=1 miss=[] subs=[] order=[--y]\n"
    "opt x n=1 res=[1]\n"
    "opt y n=1 res=[2]\n"
    "R37 [--g 1]\n"
    "g 1\n"
    "complete G\n"
    "final G\n"
    "final G\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd G n=1 miss=[] subs=[] order=[--g]\n"
    "opt g n=1 res=[1]\n"
    "R38 []\n"
    "d 7\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt d n=0 res=[]\n"
    "R39 [--x 1 sub --y 2]\n"
    "y 2\n"
    "final sub\n"
    "x 1\n"
    "final app\n"
    "cmd app n=1 miss=[] subs=[sub] order=[--x]\n"
    "cmd sub n=1 miss=[] subs=[] order=[--y]\n"
    "opt x n=1 res=[1]\n"
    "opt y n=1 res=[2]\n"
    "R40 [sub --z]\n"
    "ERR RequiredError 106: --y is required\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[--z] subs=[] order=[]\n"
    "opt y n=0 res=[]\n"
    "opt z n=0 res=[]\n"
    "S17 [--t 1 --t 2]\n"
    "t 1\n"
    "t 2\n"
    "cmd app n=1 miss=[] subs=[] order=[--t|--t]\n"
    "opt t n=1 res=[2]\n"
    "S18 [sub]\n"
    "final sub\n"
    "final app\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[] subs=[] order=[]\n"
    "S19 [sub]\n"
    "complete app\n"
    "final sub\n"
    "final app\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[] subs=[] order=[]\n"
    "S20 [a sub b c]\n"
    "pre app 4\n"
    "pre sub 2\n"
    "cmd app n=1 miss=[a] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[b|c] subs=[] order=[]\n"
    "S35 [sub deep]\n"
    "complete deep\n"
    "final deep\n"
    "final sub\n"
    "final app\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[] subs=[deep] order=[]\n"
    "cmd deep n=1 miss=[] subs=[] order=[]\n"
    "S36 [sub]\n"
    "pre app\n"
    "pre sub\n"
    "final sub\n"
    "final app\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[] subs=[] order=[]\n"
    "R45 [--f]\n"
    "cmd app n=1 miss=[] subs=[] order=[--f]\n"
    "cmd G n=1 miss=[] subs=[] order=[]\n"
    "opt g n=0 res=[]\n"
    "opt f n=1 res=[true]\n";

// FAILURES
//   constant: what the cases of d_tests_arg_process_failures come to, as CLI11
// processes them.
static const char FAILURES[] =
    "R21 []\n"
    "ERR ValidationError 105: bad\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "R22 []\n"
    "ERR Success 0: Successfully completed, should be caught and quit\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "R23 []\n"
    "ERR RuntimeError 3: Runtime error\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "R24 [--x 1]\n"
    "ERR ConversionError 104: Could not convert: --x = 1\n"
    "cmd app n=1 miss=[] subs=[] order=[--x]\n"
    "opt x n=1 res=[1]\n"
    "S16 [--x 1]\n"
    "ERR ValidationError 105: nope\n"
    "cmd app n=1 miss=[] subs=[] order=[--x]\n"
    "opt x n=1 res=[1]\n"
    "S37 [--x 1]\n"
    "ERR ValidationError 105: --x: nope\n"
    "cmd app n=1 miss=[] subs=[] order=[--x]\n"
    "opt x n=1 res=[1]\n";

// PRIORITIES
//   constant: what the cases of d_tests_arg_process_priorities come to, as
// CLI11 processes them.
static const char PRIORITIES[] =
    "R25 [--a 1 --b 2 --c 3]\n"
    "b 2\n"
    "c 3\n"
    "a 1\n"
    "cmd app n=1 miss=[] subs=[] order=[--a|--b|--c]\n"
    "opt a n=1 res=[1]\n"
    "opt b n=1 res=[2]\n"
    "opt c n=1 res=[3]\n";

// EXTRAS
//   constant: what the cases of d_tests_arg_process_extras come to, as CLI11
// processes them.
static const char EXTRAS[] =
    "R15 [--o 1 --o 2]\n"
    "ERR ArgumentMismatch 114: --o: At most 1 required but received 2\n"
    "cmd app n=1 miss=[] subs=[] order=[--o|--o]\n"
    "opt o n=2 res=[1|2]\n"
    "R33 [sub x]\n"
    "ERR ExtrasError 109: sub: The following argument was not expected: x\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[x] subs=[] order=[]\n"
    "R34 [x -- a]\n"
    "ERR ExtrasError 109: prog: The following arguments were not expected: x "
    "-- a\n"
    "cmd app n=1 miss=[x|--|a] subs=[] order=[]\n"
    "R35 [-- a b]\n"
    "cmd app n=1 miss=[--|a|b] subs=[] order=[]\n"
    "S10 [x sub y]\n"
    "cmd app n=1 miss=[x] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[y] subs=[] order=[]\n"
    "S11 [sub x]\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[x] subs=[] order=[]\n"
    "S12 [x]\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd G n=1 miss=[x] subs=[] order=[]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt g n=0 res=[]\n"
    "S13 [--g x]\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd G n=1 miss=[x] subs=[] order=[--g]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt g n=1 res=[true]\n";

#if (D_INTERNAL_ARG_PARSER_ENVIRONMENT == 1)

// ENVIRONMENT
//   constant: what the cases of d_tests_arg_process_environment come to, as
// CLI11 processes them.
static const char ENVIRONMENT[] =
    "R26 []\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt e n=1 res=[5]\n"
    "opt f n=0 res=[]\n"
    "S23a []\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt y n=0 res=[]\n"
    "S23b [sub]\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[] subs=[] order=[]\n"
    "opt y n=1 res=[9]\n"
    "S24 []\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd G n=1 miss=[] subs=[] order=[]\n"
    "opt g n=1 res=[4]\n"
    "S25 [--a]\n"
    "ERR ExcludesError 108: --a excludes --b\n"
    "cmd app n=1 miss=[] subs=[] order=[--a]\n"
    "opt a n=1 res=[true]\n"
    "opt b n=1 res=[1]\n"
    "S26 []\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt x n=1 res=[1]\n"
    "R41 [--e 1]\n"
    "cmd app n=1 miss=[] subs=[] order=[--e]\n"
    "opt e n=1 res=[1]\n";

#endif  // D_INTERNAL_ARG_PARSER_ENVIRONMENT

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// CONFIGURATION
//   constant: what the cases of d_tests_arg_process_configuration come to, as
// CLI11 processes them.
static const char CONFIGURATION[] =
    "R27 [--config /nonexistent/probe.ini]\n"
    "ERR FileError 103: /nonexistent/probe.ini was not readable (missing?)\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[/nonexistent/probe.ini]\n"
    "R28 []\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt config n=0 res=[]\n"
    "R29 []\n"
    "ERR FileError 103: /nonexistent/default.ini was not readable (missing?)\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[/nonexistent/default.ini]\n"
    "R30 []\n"
    "ERR RequiredError 106: --config is required\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt config n=0 res=[]\n"
    "R31 [--config /nonexistent/probe.ini]\n"
    "ERR RequiredError 106: --x is required\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[/nonexistent/probe.ini]\n"
    "opt x n=0 res=[]\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if ( (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1) &&                             \
      (D_INTERNAL_ARG_PARSER_ENVIRONMENT == 1) )

// CONFIGURATION_ENVIRONMENT
//   constant: what the cases of d_tests_arg_process_configuration_environment
// come to, as CLI11 processes them.
static const char CONFIGURATION_ENVIRONMENT[] =
    "R32 []\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[/nonexistent/env.ini]\n"
    "R50 []\n"
    "ERR FileError 103: /nonexistent/env.ini was not readable (missing?)\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[/nonexistent/env.ini]\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES, D_INTERNAL_ARG_PARSER_ENVIRONMENT

//==============================================================================
// HELPERS
//==============================================================================

/**
 * @brief A final callback that notes `final <label>`, the label the one
 *        the run reports the command under.
 *
 * @param[in] _command  the command.
 * @param[in] _context  the case's run.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_process_final(
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
 * @brief A parse-complete callback that notes `complete <label>`.
 *
 * @param[in] _command  the command.
 * @param[in] _context  the case's run.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_process_complete(
    struct d_arg_command* _command,
    void*                 _context
)
{
    d_tests_arg_note(_context,
                     "complete %s",
                     d_tests_arg_command_label(_context,
                                               _command));

    return D_ARG_OK;
}

/**
 * @brief A pre-callback that notes `pre <label>`, as the reference's App
 *        overriding CLI11's pre_callback prints it.
 *
 * @param[in] _command  the command.
 * @param[in] _context  the case's run.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_process_pre(
    struct d_arg_command* _command,
    void*                 _context
)
{
    d_tests_arg_note(_context,
                     "pre %s",
                     d_tests_arg_command_label(_context,
                                               _command));

    return D_ARG_OK;
}

/**
 * @brief A pre-parse callback that notes `pre <label> <arguments left>`.
 *
 * @param[in] _command    the command.
 * @param[in] _remaining  the arguments left.
 * @param[in] _context    the case's run.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_process_preparse(
    struct d_arg_command* _command,
    size_t                _remaining,
    void*                 _context
)
{
    d_tests_arg_note(_context,
                     "pre %s %lu",
                     d_tests_arg_command_label(_context,
                                               _command),
                     (unsigned long)_remaining);

    return D_ARG_OK;
}

/**
 * @brief An option's callback that notes `<label> <first value>`, as the
 *        reference's callbacks print the std::string CLI11 converts the
 *        values to.
 *
 * @param[in] _option   the option.
 * @param[in] _results  its values.
 * @param[in] _count    how many.
 * @param[in] _context  the case's run.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_process_value(
    struct d_arg_option*      _option,
    const struct d_pack_text* _results,
    size_t                    _count,
    void*                     _context
)
{
    const struct d_pack_text first = (_count > 0u)
                                     ? _results[0]
                                     : d_pack_text_from_cstr("");

    d_tests_arg_note(_context,
                     "%s %.*s",
                     d_tests_arg_option_label(_context,
                                              _option),
                     (int)first.length,
                     first.data);

    return D_ARG_OK;
}

/**
 * @brief A final callback failing as CLI11's ValidationError("bad") thrown
 *        from one.
 *
 * @param[in] _command  the command.
 * @param[in] _context  unused.
 * @return D_ARG_ERROR_VALIDATION.
 */
static int
d_tests_arg_process_bad(
    struct d_arg_command* _command,
    void*                 _context
)
{
    (void)_context;

    return d_arg_command_fail(_command,
                              D_ARG_ERROR_VALIDATION,
                              "bad");
}

/**
 * @brief A final callback ending the parse with success, as CLI11's
 *        Success thrown from one, described by the status's own text.
 *
 * @param[in] _command  the command; unused.
 * @param[in] _context  unused.
 * @return D_ARG_SUCCESS.
 */
static int
d_tests_arg_process_success(
    struct d_arg_command* _command,
    void*                 _context
)
{
    (void)_command;
    (void)_context;

    return D_ARG_SUCCESS;
}

/**
 * @brief A final callback failing as CLI11's RuntimeError(3) thrown from
 *        one.
 *
 * @param[in] _command  the command.
 * @param[in] _context  unused.
 * @return D_ARG_RUNTIME_ERROR.
 */
static int
d_tests_arg_process_runtime(
    struct d_arg_command* _command,
    void*                 _context
)
{
    (void)_context;

    return d_arg_command_fail_runtime(_command,
                                      3);
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
d_tests_arg_process_ask_help(
    struct d_arg_command* _command,
    void*                 _context
)
{
    (void)_command;
    (void)_context;

    return D_ARG_CALL_FOR_HELP;
}

/**
 * @brief An option's callback refusing its values, as a CLI11 callback
 *        returning false.
 *
 * @param[in] _option   the option; unused.
 * @param[in] _results  its values; unused.
 * @param[in] _count    how many; unused.
 * @param[in] _context  unused.
 * @return D_ARG_ERROR_CONVERSION.
 */
static int
d_tests_arg_process_refuse(
    struct d_arg_option*      _option,
    const struct d_pack_text* _results,
    size_t                    _count,
    void*                     _context
)
{
    (void)_option;
    (void)_results;
    (void)_count;
    (void)_context;

    return D_ARG_ERROR_CONVERSION;
}

/**
 * @brief An option's callback failing as CLI11's ValidationError("nope")
 *        thrown from one.
 *
 * @param[in] _option   the option.
 * @param[in] _results  its values; unused.
 * @param[in] _count    how many; unused.
 * @param[in] _context  unused.
 * @return D_ARG_ERROR_VALIDATION.
 */
static int
d_tests_arg_process_nope(
    struct d_arg_option*      _option,
    const struct d_pack_text* _results,
    size_t                    _count,
    void*                     _context
)
{
    (void)_results;
    (void)_count;
    (void)_context;

    return d_arg_option_fail(_option,
                             D_ARG_ERROR_VALIDATION,
                             "nope");
}

/**
 * @brief A function run on each value, failing as CLI11's
 *        ValidationError("nope") thrown from one.
 *
 * @param[in] _option   the option.
 * @param[in] _value    the value; unused.
 * @param[in] _context  unused.
 * @return D_ARG_ERROR_VALIDATION.
 */
static int
d_tests_arg_process_each_nope(
    const struct d_arg_option* _option,
    struct d_pack_text         _value,
    void*                      _context
)
{
    (void)_value;
    (void)_context;

    return d_arg_option_fail(_option,
                             D_ARG_ERROR_VALIDATION,
                             "nope");
}

/**
 * @brief Writes a version, as the reference's function returning "fn-1".
 *
 * @param[in] _command  the command; unused.
 * @param[in] _sink     where the version is written.
 * @param[in] _context  unused.
 * @return the sink's status.
 */
static int
d_tests_arg_process_version(
    const struct d_arg_command* _command,
    struct d_pack_sink          _sink,
    void*                       _context
)
{
    (void)_command;
    (void)_context;

    return d_arg_sink_cstr(_sink,
                           "fn-1");
}

//==============================================================================
// 1.  REQUIRED
//==============================================================================

/**
 * @brief R01: a required option left out.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const x = d_tests_arg_make_option(_run,
                                                           "x",
                                                           _app,
                                                           "--x");

    d_arg_option_set_required(x,
                              true);

    return;
}

/**
 * @brief R13: a required subcommand left out.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r13(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_required(sub,
                               true);

    return;
}

/**
 * @brief R14: a required option group left out.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r14(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const g = d_tests_arg_make_group(_run,
                                                           _app,
                                                           "G");

    (void)d_tests_arg_make_flag(_run,
                                "g",
                                g,
                                "--g");
    d_arg_command_set_required(g,
                               true);

    return;
}

/**
 * @brief S06: a required option with default text, left out.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s06(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const x = d_tests_arg_make_option(_run,
                                                           "x",
                                                           _app,
                                                           "--x");

    d_arg_option_set_required(x,
                              true);
    (void)d_arg_option_set_default_text(x,
                                        "5");

    return;
}

/**
 * @brief S14: a required positional of two values, given one.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s14(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const pos = d_tests_arg_make_option(_run,
                                                             "pos",
                                                             _app,
                                                             "pos");

    d_arg_option_set_expected(pos,
                              2);
    d_arg_option_set_required(pos,
                              true);

    return;
}

/**
 * @brief S15: a required positional left out.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s15(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const pos = d_tests_arg_make_option(_run,
                                                             "pos",
                                                             _app,
                                                             "pos");

    d_arg_option_set_required(pos,
                              true);

    return;
}

/**
 * @brief S30: a subcommand's required option, a deeper subcommand used.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s30(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");
    struct d_arg_option* const  o   = d_tests_arg_make_option(_run,
                                                              "o",
                                                              sub,
                                                              "--o");

    d_arg_option_set_required(o,
                              true);
    (void)d_tests_arg_make_subcommand(_run,
                                      sub,
                                      "deep");

    return;
}

/**
 * @brief S34: a required option of two or three values, given one.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s34(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const x = d_tests_arg_make_option(_run,
                                                           "x",
                                                           _app,
                                                           "--x");

    d_arg_option_set_expected_range(x,
                                    2,
                                    3);
    d_arg_option_set_required(x,
                              true);

    return;
}

// REQUIRED_CASES
//   constant: the cases of d_tests_arg_process_required.
static const struct d_tests_arg_case REQUIRED_CASES[] =
{
    { "R01",
      d_tests_arg_process_r01,
      { NULL },
      NULL,
      NULL },
    { "R13",
      d_tests_arg_process_r13,
      { NULL },
      NULL,
      NULL },
    { "R14",
      d_tests_arg_process_r14,
      { NULL },
      NULL,
      NULL },
    { "S06",
      d_tests_arg_process_s06,
      { NULL },
      NULL,
      NULL },
    { "S14",
      d_tests_arg_process_s14,
      { "a" },
      NULL,
      NULL },
    { "S15",
      d_tests_arg_process_s15,
      { NULL },
      NULL,
      NULL },
    { "S30",
      d_tests_arg_process_s30,
      { "sub", "deep" },
      NULL,
      NULL },
    { "S34",
      d_tests_arg_process_s34,
      { "--x", "1" },
      NULL,
      NULL }
};

// REQUIRED_COUNT
//   constant: how many cases REQUIRED_CASES holds.
static const size_t REQUIRED_COUNT =
    sizeof(REQUIRED_CASES) / sizeof(REQUIRED_CASES[0]);

/*
d_tests_arg_process_required
  What a parse refuses for want of something required.
  Tests the following:
  - a required option, subcommand and option group left out
  - a required option with default text, which does not count
  - a required positional given too few values, and none
  - a subcommand's required option when a deeper subcommand is used
  - a required option given fewer values than it takes
*/
bool
d_tests_arg_process_required(void)
{
    return d_tests_arg_cases("REQUIRED",
                             REQUIRED_CASES,
                             REQUIRED_COUNT,
                             REQUIRED);
}

//==============================================================================
// 2.  RELATIONS
//==============================================================================

/**
 * @brief R02: an option needing another, given alone.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const a = d_tests_arg_make_flag(_run,
                                                         "a",
                                                         _app,
                                                         "--a");
    struct d_arg_option* const b = d_tests_arg_make_flag(_run,
                                                         "b",
                                                         _app,
                                                         "--b");

    (void)d_arg_option_needs(a,
                             b);

    return;
}

/**
 * @brief R03: an option excluding another, given after it.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r03(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const c = d_tests_arg_make_flag(_run,
                                                         "c",
                                                         _app,
                                                         "--c");
    struct d_arg_option* const d = d_tests_arg_make_flag(_run,
                                                         "d",
                                                         _app,
                                                         "--d");

    (void)d_arg_option_excludes(c,
                                d);

    return;
}

/**
 * @brief R04: a subcommand excluding an option, used with it.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r04(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const  top = d_tests_arg_make_flag(_run,
                                                            "top",
                                                            _app,
                                                            "--top");
    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    (void)d_arg_command_excludes_option(sub,
                                        top);

    return;
}

/**
 * @brief R05: a subcommand excluding an option, the option given alone.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r05(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const  top = d_tests_arg_make_flag(_run,
                                                            "top",
                                                            _app,
                                                            "--top");
    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    (void)d_arg_command_excludes_option(sub,
                                        top);

    return;
}

/**
 * @brief R06: a subcommand needing another, used alone.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r06(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const sub1 = d_tests_arg_make_subcommand(_run,
                                                                   _app,
                                                                   "sub1");
    struct d_arg_command* const sub2 = d_tests_arg_make_subcommand(_run,
                                                                   _app,
                                                                   "sub2");

    (void)d_arg_command_needs_command(sub1,
                                      sub2);

    return;
}

/**
 * @brief R36: subcommands excluding each other, both used.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r36(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const s1 = d_tests_arg_make_subcommand(_run,
                                                                 _app,
                                                                 "s1");
    struct d_arg_command* const s2 = d_tests_arg_make_subcommand(_run,
                                                                 _app,
                                                                 "s2");

    (void)d_arg_command_excludes_command(s1,
                                         s2);

    return;
}

/**
 * @brief S07: a subcommand needing an option, used alone.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s07(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const  o   = d_tests_arg_make_option(_run,
                                                              "o",
                                                              _app,
                                                              "--o");
    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    (void)d_arg_command_needs_option(sub,
                                     o);

    return;
}

/**
 * @brief S09: an option group excluding an option, both used.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s09(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const g = d_tests_arg_make_group(_run,
                                                           _app,
                                                           "G");

    (void)d_tests_arg_make_flag(_run,
                                "g",
                                g,
                                "--g");

    struct d_arg_option* const o = d_tests_arg_make_flag(_run,
                                                         "o",
                                                         _app,
                                                         "--o");

    (void)d_arg_command_excludes_option(g,
                                        o);

    return;
}

/**
 * @brief S29: an option needing another by name, given alone.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s29(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const a = d_tests_arg_make_flag(_run,
                                                         "a",
                                                         _app,
                                                         "--a");

    (void)d_tests_arg_make_flag(_run,
                                "b",
                                _app,
                                "--b");
    (void)d_arg_option_needs_name(a,
                                  "--b");

    return;
}

/**
 * @brief S33: option groups excluding each other, both used.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s33(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const g = d_tests_arg_make_group(_run,
                                                           _app,
                                                           "G");

    (void)d_tests_arg_make_flag(_run,
                                "g",
                                g,
                                "--g");

    struct d_arg_command* const h = d_tests_arg_make_group(_run,
                                                           _app,
                                                           "H");

    (void)d_tests_arg_make_flag(_run,
                                "h1",
                                h,
                                "--h1");
    (void)d_arg_command_excludes_command(g,
                                         h);

    return;
}

/**
 * @brief R42: a subcommand needing an option, not used.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r42(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const  o   = d_tests_arg_make_option(_run,
                                                              "o",
                                                              _app,
                                                              "--o");
    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    (void)d_arg_command_needs_option(sub,
                                     o);

    return;
}

/**
 * @brief R46: a subcommand excluding an option and a subcommand, all
 *        used: the subcommand named.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r46(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const  top = d_tests_arg_make_flag(_run,
                                                            "top",
                                                            _app,
                                                            "--top");
    struct d_arg_command* const s   = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "s");
    struct d_arg_command* const t   = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "t");

    (void)d_arg_command_excludes_option(s,
                                        top);
    (void)d_arg_command_excludes_command(s,
                                         t);

    return;
}

/**
 * @brief R47: an option group excluding an option, the group not used.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r47(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const g = d_tests_arg_make_group(_run,
                                                           _app,
                                                           "G");

    (void)d_tests_arg_make_flag(_run,
                                "g",
                                g,
                                "--g");

    struct d_arg_option* const o = d_tests_arg_make_flag(_run,
                                                         "o",
                                                         _app,
                                                         "--o");

    (void)d_arg_command_excludes_option(g,
                                        o);

    return;
}

/**
 * @brief R48: an option group needing an option, the group not used.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r48(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const g = d_tests_arg_make_group(_run,
                                                           _app,
                                                           "G");

    (void)d_tests_arg_make_flag(_run,
                                "g",
                                g,
                                "--g");

    struct d_arg_option* const o = d_tests_arg_make_flag(_run,
                                                         "o",
                                                         _app,
                                                         "--o");

    (void)d_arg_command_needs_option(g,
                                     o);

    return;
}

// RELATION_CASES
//   constant: the cases of d_tests_arg_process_relations.
static const struct d_tests_arg_case RELATION_CASES[] =
{
    { "R02",
      d_tests_arg_process_r02,
      { "--a" },
      NULL,
      NULL },
    { "R03",
      d_tests_arg_process_r03,
      { "--d", "--c" },
      NULL,
      NULL },
    { "R04",
      d_tests_arg_process_r04,
      { "--top", "sub" },
      NULL,
      NULL },
    { "R05",
      d_tests_arg_process_r05,
      { "--top" },
      NULL,
      NULL },
    { "R06",
      d_tests_arg_process_r06,
      { "sub1" },
      NULL,
      NULL },
    { "R36",
      d_tests_arg_process_r36,
      { "s1", "s2" },
      NULL,
      NULL },
    { "S07",
      d_tests_arg_process_s07,
      { "sub" },
      NULL,
      NULL },
    { "S09",
      d_tests_arg_process_s09,
      { "--g", "--o" },
      NULL,
      NULL },
    { "S29",
      d_tests_arg_process_s29,
      { "--a" },
      NULL,
      NULL },
    { "S33",
      d_tests_arg_process_s33,
      { "--g", "--h1" },
      NULL,
      NULL },
    { "R42",
      d_tests_arg_process_r42,
      { NULL },
      NULL,
      NULL },
    { "R46",
      d_tests_arg_process_r46,
      { "--top", "t", "s" },
      NULL,
      NULL },
    { "R47",
      d_tests_arg_process_r47,
      { "--o" },
      NULL,
      NULL },
    { "R48",
      d_tests_arg_process_r48,
      { NULL },
      NULL,
      NULL }
};

// RELATION_COUNT
//   constant: how many cases RELATION_CASES holds.
static const size_t RELATION_COUNT =
    sizeof(RELATION_CASES) / sizeof(RELATION_CASES[0]);

/*
d_tests_arg_process_relations
  Needs and excludes between options, subcommands and groups.
  Tests the following:
  - an option needing another, and one excluding another
  - a subcommand excluding an option, used with it and not
  - a subcommand needing a subcommand, excluding one, and needing an option
  - an option group excluding an option and another group
  - an option needing another named by its name
  - a subcommand not used, needing what is not there, and an option group not
    used, needing or excluding an option
  - a subcommand excluded by an option and a subcommand, named by the subcommand
*/
bool
d_tests_arg_process_relations(void)
{
    return d_tests_arg_cases("RELATIONS",
                             RELATION_CASES,
                             RELATION_COUNT,
                             RELATIONS);
}

//==============================================================================
// 3.  SUBCOMMAND COUNTS
//==============================================================================

/**
 * @brief R07: a command requiring a subcommand, given none.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r07(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_require_subcommand_count(_app,
                                           1);
    (void)d_tests_arg_make_subcommand(_run,
                                      _app,
                                      "s");

    return;
}

/**
 * @brief R08: a command requiring two subcommands, given one.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r08(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_require_subcommand_count(_app,
                                           2);
    (void)d_tests_arg_make_subcommand(_run,
                                      _app,
                                      "s");
    (void)d_tests_arg_make_subcommand(_run,
                                      _app,
                                      "t");

    return;
}

/**
 * @brief S21: a disabled subcommand named.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s21(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_disabled(sub,
                               true);

    return;
}

/**
 * @brief S22: a silent subcommand used.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s22(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_silent(sub,
                             true);

    return;
}

/**
 * @brief S31: a subcommand requiring one of its own, given none.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s31(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_require_subcommand_count(sub,
                                           1);
    (void)d_tests_arg_make_subcommand(_run,
                                      sub,
                                      "deep");

    return;
}

/**
 * @brief R43: a command requiring a subcommand, given one.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r43(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_require_subcommand_count(_app,
                                           1);
    (void)d_tests_arg_make_subcommand(_run,
                                      _app,
                                      "s");

    return;
}

// SUBCOMMAND_COUNT_CASES
//   constant: the cases of d_tests_arg_process_subcommand_counts.
static const struct d_tests_arg_case SUBCOMMAND_COUNT_CASES[] =
{
    { "R07",
      d_tests_arg_process_r07,
      { NULL },
      NULL,
      NULL },
    { "R08",
      d_tests_arg_process_r08,
      { "s" },
      NULL,
      NULL },
    { "S21",
      d_tests_arg_process_s21,
      { "sub" },
      NULL,
      NULL },
    { "S22",
      d_tests_arg_process_s22,
      { "sub" },
      NULL,
      NULL },
    { "S31",
      d_tests_arg_process_s31,
      { "sub" },
      NULL,
      NULL },
    { "R43",
      d_tests_arg_process_r43,
      { "s" },
      NULL,
      NULL }
};

// SUBCOMMAND_COUNT_COUNT
//   constant: how many cases SUBCOMMAND_COUNT_CASES holds.
static const size_t SUBCOMMAND_COUNT_COUNT =
    sizeof(SUBCOMMAND_COUNT_CASES) / sizeof(SUBCOMMAND_COUNT_CASES[0]);

/*
d_tests_arg_process_subcommand_counts
  How many subcommands a command requires, and those not counted.
  Tests the following:
  - a command requiring one subcommand, and two, given fewer, and one given it
  - a disabled subcommand, and a silent one
  - a subcommand requiring a subcommand of its own
*/
bool
d_tests_arg_process_subcommand_counts(void)
{
    return d_tests_arg_cases("SUBCOMMAND_COUNTS",
                             SUBCOMMAND_COUNT_CASES,
                             SUBCOMMAND_COUNT_COUNT,
                             SUBCOMMAND_COUNTS);
}

//==============================================================================
// 4.  OPTION COUNTS
//==============================================================================

/**
 * @brief R09a: a command requiring one option, given none.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r09a(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_require_option_count(_app,
                                       1);
    (void)d_tests_arg_make_flag(_run,
                                "a",
                                _app,
                                "--a");
    (void)d_tests_arg_make_flag(_run,
                                "b",
                                _app,
                                "--b");
    (void)d_tests_arg_make_flag(_run,
                                "c",
                                _app,
                                "-c");

    return;
}

/**
 * @brief R09b: a command requiring one option, given two.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r09b(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_require_option_count(_app,
                                       1);
    (void)d_tests_arg_make_flag(_run,
                                "a",
                                _app,
                                "--a");
    (void)d_tests_arg_make_flag(_run,
                                "b",
                                _app,
                                "--b");
    (void)d_tests_arg_make_flag(_run,
                                "c",
                                _app,
                                "-c");

    return;
}

/**
 * @brief R09c: a command requiring at least one option, given none.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r09c(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_require_option(_app,
                                 1u,
                                 0u);
    (void)d_tests_arg_make_flag(_run,
                                "a",
                                _app,
                                "--a");
    (void)d_tests_arg_make_flag(_run,
                                "b",
                                _app,
                                "--b");
    (void)d_tests_arg_make_flag(_run,
                                "c",
                                _app,
                                "-c");

    return;
}

/**
 * @brief R09d: a command requiring two or three options, given one.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r09d(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_require_option(_app,
                                 2u,
                                 3u);
    (void)d_tests_arg_make_flag(_run,
                                "a",
                                _app,
                                "--a");
    (void)d_tests_arg_make_flag(_run,
                                "b",
                                _app,
                                "--b");
    (void)d_tests_arg_make_flag(_run,
                                "c",
                                _app,
                                "-c");

    return;
}

/**
 * @brief R09e: a command allowing one option at most, given two.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r09e(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_require_option(_app,
                                 0u,
                                 1u);
    (void)d_tests_arg_make_flag(_run,
                                "a",
                                _app,
                                "--a");
    (void)d_tests_arg_make_flag(_run,
                                "b",
                                _app,
                                "--b");
    (void)d_tests_arg_make_flag(_run,
                                "c",
                                _app,
                                "-c");

    return;
}

/**
 * @brief R09f: a command allowing two options at most, given three.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r09f(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_require_option(_app,
                                 0u,
                                 2u);
    (void)d_tests_arg_make_flag(_run,
                                "a",
                                _app,
                                "--a");
    (void)d_tests_arg_make_flag(_run,
                                "b",
                                _app,
                                "--b");
    (void)d_tests_arg_make_flag(_run,
                                "c",
                                _app,
                                "-c");

    return;
}

/**
 * @brief R10: a command requiring an option, given none, with a help flag,
 *        a flag with a default value and a group's flag.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r10(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_flag(_run,
                                "a",
                                _app,
                                "--a");
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                _app,
                                "--f{5}");
    (void)d_arg_command_set_help_flag(_app,
                                      "-h,--help",
                                      NULL);

    struct d_arg_command* const g = d_tests_arg_make_group(_run,
                                                           _app,
                                                           "G");

    (void)d_tests_arg_make_flag(_run,
                                "g",
                                g,
                                "--g");
    d_arg_command_require_option_count(_app,
                                       1);

    return;
}

/**
 * @brief R11: a command requiring one option, given its own and its
 *        group's.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r11(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_flag(_run,
                                "a",
                                _app,
                                "--a");

    struct d_arg_command* const g = d_tests_arg_make_group(_run,
                                                           _app,
                                                           "G");

    (void)d_tests_arg_make_flag(_run,
                                "g",
                                g,
                                "--g");
    d_arg_command_require_option_count(_app,
                                       1);

    return;
}

/**
 * @brief R12: a command requiring one option, given it, its group's
 *        required option left out.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r12(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_flag(_run,
                                "a",
                                _app,
                                "--a");

    struct d_arg_command* const g  = d_tests_arg_make_group(_run,
                                                            _app,
                                                            "G");
    struct d_arg_option* const  gg = d_tests_arg_make_flag(_run,
                                                           "g",
                                                           g,
                                                           "--g");

    d_arg_option_set_required(gg,
                              true);
    d_arg_command_require_option_count(_app,
                                       1);

    return;
}

/**
 * @brief S32: a command requiring at least one option and at most none.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s32(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_flag(_run,
                                "a",
                                _app,
                                "--a");
    (void)d_tests_arg_make_flag(_run,
                                "b",
                                _app,
                                "--b");
    d_arg_command_require_option(_app,
                                 1u,
                                 0u);

    return;
}

/**
 * @brief R44: a command allowing two options at most, given two.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r44(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_require_option(_app,
                                 0u,
                                 2u);
    (void)d_tests_arg_make_flag(_run,
                                "a",
                                _app,
                                "--a");
    (void)d_tests_arg_make_flag(_run,
                                "b",
                                _app,
                                "--b");
    (void)d_tests_arg_make_flag(_run,
                                "c",
                                _app,
                                "-c");

    return;
}

/**
 * @brief R49: a command requiring at least one option, given one, its
 *        unused group's required option passed over.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r49(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_flag(_run,
                                "a",
                                _app,
                                "--a");

    struct d_arg_command* const g  = d_tests_arg_make_group(_run,
                                                            _app,
                                                            "G");
    struct d_arg_option* const  gg = d_tests_arg_make_flag(_run,
                                                           "g",
                                                           g,
                                                           "--g");

    d_arg_option_set_required(gg,
                              true);
    d_arg_command_require_option(_app,
                                 1u,
                                 0u);

    return;
}

// OPTION_COUNT_CASES
//   constant: the cases of d_tests_arg_process_option_counts.
static const struct d_tests_arg_case OPTION_COUNT_CASES[] =
{
    { "R09a",
      d_tests_arg_process_r09a,
      { NULL },
      NULL,
      NULL },
    { "R09b",
      d_tests_arg_process_r09b,
      { "--a", "--b" },
      NULL,
      NULL },
    { "R09c",
      d_tests_arg_process_r09c,
      { NULL },
      NULL,
      NULL },
    { "R09d",
      d_tests_arg_process_r09d,
      { "--a" },
      NULL,
      NULL },
    { "R09e",
      d_tests_arg_process_r09e,
      { "--a", "--b" },
      NULL,
      NULL },
    { "R09f",
      d_tests_arg_process_r09f,
      { "--a", "--b", "-c" },
      NULL,
      NULL },
    { "R10",
      d_tests_arg_process_r10,
      { NULL },
      NULL,
      NULL },
    { "R11",
      d_tests_arg_process_r11,
      { "--g", "--a" },
      NULL,
      NULL },
    { "R12",
      d_tests_arg_process_r12,
      { "--a" },
      NULL,
      NULL },
    { "S32",
      d_tests_arg_process_s32,
      { "--a", "--b" },
      NULL,
      NULL },
    { "R44",
      d_tests_arg_process_r44,
      { "--a", "-c" },
      NULL,
      NULL },
    { "R49",
      d_tests_arg_process_r49,
      { "--a" },
      NULL,
      NULL }
};

// OPTION_COUNT_COUNT
//   constant: how many cases OPTION_COUNT_CASES holds.
static const size_t OPTION_COUNT_COUNT =
    sizeof(OPTION_COUNT_CASES) / sizeof(OPTION_COUNT_CASES[0]);

/*
d_tests_arg_process_option_counts
  How many options a command requires, at least and at most.
  Tests the following:
  - a command requiring one option, at least one, two to three, and at most one
    or two
  - the help flag and a flag with a default value not counted
  - an option group's options counted for its command, and a required one in the
    group
  - a maximum of none, which is no maximum, and a maximum met
  - an unused option group passed over once the requirement is met
*/
bool
d_tests_arg_process_option_counts(void)
{
    return d_tests_arg_cases("OPTION_COUNTS",
                             OPTION_COUNT_CASES,
                             OPTION_COUNT_COUNT,
                             OPTION_COUNTS);
}

//==============================================================================
// 5.  GROUP COUNTS
//==============================================================================

/**
 * @brief S08a: a group requiring one option, given none.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s08a(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const g = d_tests_arg_make_group(_run,
                                                           _app,
                                                           "G");

    (void)d_tests_arg_make_flag(_run,
                                "g1",
                                g,
                                "--g1");
    (void)d_tests_arg_make_flag(_run,
                                "g2",
                                g,
                                "--g2");
    d_arg_command_require_option_count(g,
                                       1);

    return;
}

/**
 * @brief S08b: a group requiring one option, given two.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s08b(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const g = d_tests_arg_make_group(_run,
                                                           _app,
                                                           "G");

    (void)d_tests_arg_make_flag(_run,
                                "g1",
                                g,
                                "--g1");
    (void)d_tests_arg_make_flag(_run,
                                "g2",
                                g,
                                "--g2");
    d_arg_command_require_option_count(g,
                                       1);

    return;
}

/**
 * @brief S08c: a group requiring one option, given one.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s08c(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const g = d_tests_arg_make_group(_run,
                                                           _app,
                                                           "G");

    (void)d_tests_arg_make_flag(_run,
                                "g1",
                                g,
                                "--g1");
    (void)d_tests_arg_make_flag(_run,
                                "g2",
                                g,
                                "--g2");
    d_arg_command_require_option_count(g,
                                       1);

    return;
}

// GROUP_COUNT_CASES
//   constant: the cases of d_tests_arg_process_group_counts.
static const struct d_tests_arg_case GROUP_COUNT_CASES[] =
{
    { "S08a",
      d_tests_arg_process_s08a,
      { NULL },
      NULL,
      NULL },
    { "S08b",
      d_tests_arg_process_s08b,
      { "--g1", "--g2" },
      NULL,
      NULL },
    { "S08c",
      d_tests_arg_process_s08c,
      { "--g2" },
      NULL,
      NULL }
};

// GROUP_COUNT_COUNT
//   constant: how many cases GROUP_COUNT_CASES holds.
static const size_t GROUP_COUNT_COUNT =
    sizeof(GROUP_COUNT_CASES) / sizeof(GROUP_COUNT_CASES[0]);

/*
d_tests_arg_process_group_counts
  An option group requiring one of its options.
  Tests the following:
  - the group given none, both, and one
*/
bool
d_tests_arg_process_group_counts(void)
{
    return d_tests_arg_cases("GROUP_COUNTS",
                             GROUP_COUNT_CASES,
                             GROUP_COUNT_COUNT,
                             GROUP_COUNTS);
}

//==============================================================================
// 6.  HELP
//==============================================================================

/**
 * @brief R16: help asked for, a required option left out and an extra
 *        given.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r16(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const x = d_tests_arg_make_option(_run,
                                                           "x",
                                                           _app,
                                                           "--x");

    d_arg_option_set_required(x,
                              true);

    return;
}

/**
 * @brief R17: help for everything, then help.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r17(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_set_help_all_flag(_app,
                                          "--help-all",
                                          NULL);

    return;
}

/**
 * @brief R18: help in a subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r18(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_subcommand(_run,
                                      _app,
                                      "sub");

    return;
}

/**
 * @brief R19: the version.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r19(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_set_version_flag(_app,
                                         "--version",
                                         "1.2.3",
                                         NULL);

    return;
}

/**
 * @brief S01: help in a subcommand, its parent's required option left out.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const x = d_tests_arg_make_option(_run,
                                                           "x",
                                                           _app,
                                                           "--x");

    d_arg_option_set_required(x,
                              true);
    (void)d_tests_arg_make_subcommand(_run,
                                      _app,
                                      "sub");

    return;
}

/**
 * @brief S02: the version, then help.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_set_version_flag(_app,
                                         "--version",
                                         "1.2.3",
                                         NULL);

    return;
}

/**
 * @brief S03: help, then the version.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s03(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_set_version_flag(_app,
                                         "--version",
                                         "1.2.3",
                                         NULL);

    return;
}

/**
 * @brief S04: a subcommand's version.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s04(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    (void)d_arg_command_set_version_flag(sub,
                                         "--version",
                                         "2.0",
                                         NULL);

    return;
}

/**
 * @brief S05: a version a function writes.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s05(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    (void)d_arg_command_set_version_fn(_app,
                                       "-V,--version",
                                       d_tests_arg_process_version,
                                       NULL,
                                       NULL,
                                       NULL);

    return;
}

/**
 * @brief S27: help for everything in a subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s27(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_help_all_flag(_app,
                                          "--help-all",
                                          NULL);
    (void)d_tests_arg_make_subcommand(_run,
                                      _app,
                                      "sub");

    return;
}

/**
 * @brief S28: a final callback asking for help.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s28(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    d_arg_command_set_final_callback(_app,
                                     d_tests_arg_process_ask_help,
                                     NULL,
                                     NULL);

    return;
}

// HELP_CASES
//   constant: the cases of d_tests_arg_process_help.
static const struct d_tests_arg_case HELP_CASES[] =
{
    { "R16",
      d_tests_arg_process_r16,
      { "-h", "zz" },
      NULL,
      NULL },
    { "R17",
      d_tests_arg_process_r17,
      { "--help-all", "-h" },
      NULL,
      NULL },
    { "R18",
      d_tests_arg_process_r18,
      { "sub", "-h" },
      NULL,
      NULL },
    { "R19",
      d_tests_arg_process_r19,
      { "--version" },
      NULL,
      NULL },
    { "S01",
      d_tests_arg_process_s01,
      { "sub", "-h" },
      NULL,
      NULL },
    { "S02",
      d_tests_arg_process_s02,
      { "--version", "-h" },
      NULL,
      NULL },
    { "S03",
      d_tests_arg_process_s03,
      { "-h", "--version" },
      NULL,
      NULL },
    { "S04",
      d_tests_arg_process_s04,
      { "sub", "--version" },
      NULL,
      NULL },
    { "S05",
      d_tests_arg_process_s05,
      { "-V" },
      NULL,
      NULL },
    { "S27",
      d_tests_arg_process_s27,
      { "sub", "--help-all" },
      NULL,
      NULL },
    { "S28",
      d_tests_arg_process_s28,
      { NULL },
      NULL,
      NULL }
};

// HELP_COUNT
//   constant: how many cases HELP_CASES holds.
static const size_t HELP_COUNT = sizeof(HELP_CASES) / sizeof(HELP_CASES[0]);

/*
d_tests_arg_process_help
  Help and version requests ending a parse.
  Tests the following:
  - help before a missing required option and an extra argument
  - help for everything, and help in a subcommand
  - versions, a subcommand's, one a function writes, and either flag before the
    other
  - a final callback asking for help
*/
bool
d_tests_arg_process_help(void)
{
    return d_tests_arg_cases("HELP",
                             HELP_CASES,
                             HELP_COUNT,
                             HELP);
}

//==============================================================================
// 7.  CALLBACKS
//==============================================================================

/**
 * @brief R20: options' callbacks, then a subcommand's parse-complete and
 *        final callbacks, and its parent's final one.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r20(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_final_callback(_app,
                                     d_tests_arg_process_final,
                                     _run,
                                     NULL);

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_final_callback(sub,
                                     d_tests_arg_process_final,
                                     _run,
                                     NULL);
    d_arg_command_set_parse_complete_callback(sub,
                                              d_tests_arg_process_complete,
                                              _run,
                                              NULL);

    struct d_arg_option* const x =
        d_arg_command_add_option_callback(_app,
                                          "--x",
                                          d_tests_arg_process_value,
                                          _run,
                                          NULL,
                                          NULL);

    (void)d_tests_arg_watch_option(_run,
                                   "x",
                                   x);

    struct d_arg_option* const y =
        d_arg_command_add_option_callback(sub,
                                          "--y",
                                          d_tests_arg_process_value,
                                          _run,
                                          NULL,
                                          NULL);

    (void)d_tests_arg_watch_option(_run,
                                   "y",
                                   y);

    return;
}

/**
 * @brief R37: an option group's option, parse-complete and final
 *        callbacks.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r37(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const g  = d_tests_arg_make_group(_run,
                                                            _app,
                                                            "G");
    struct d_arg_option* const  go =
        d_arg_command_add_option_callback(g,
                                          "--g",
                                          d_tests_arg_process_value,
                                          _run,
                                          NULL,
                                          NULL);

    (void)d_tests_arg_watch_option(_run,
                                   "g",
                                   go);
    d_arg_command_set_parse_complete_callback(g,
                                              d_tests_arg_process_complete,
                                              _run,
                                              NULL);
    d_arg_command_set_final_callback(g,
                                     d_tests_arg_process_final,
                                     _run,
                                     NULL);

    return;
}

/**
 * @brief R38: a callback forced for an option given only default text.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r38(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const d =
        d_arg_command_add_option_callback(_app,
                                          "--d",
                                          d_tests_arg_process_value,
                                          _run,
                                          NULL,
                                          NULL);

    (void)d_tests_arg_watch_option(_run,
                                   "d",
                                   d);
    (void)d_arg_option_set_default_text(d,
                                        "7");
    d_arg_option_set_force_callback(d,
                                    true);

    return;
}

/**
 * @brief R39: an immediate subcommand's final callback, before its
 *        parent's option's.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r39(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_final_callback(_app,
                                     d_tests_arg_process_final,
                                     _run,
                                     NULL);

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_final_callback(sub,
                                     d_tests_arg_process_final,
                                     _run,
                                     NULL);
    d_arg_command_set_immediate_callback(sub,
                                         true);

    struct d_arg_option* const x =
        d_arg_command_add_option_callback(_app,
                                          "--x",
                                          d_tests_arg_process_value,
                                          _run,
                                          NULL,
                                          NULL);

    (void)d_tests_arg_watch_option(_run,
                                   "x",
                                   x);

    struct d_arg_option* const y =
        d_arg_command_add_option_callback(sub,
                                          "--y",
                                          d_tests_arg_process_value,
                                          _run,
                                          NULL,
                                          NULL);

    (void)d_tests_arg_watch_option(_run,
                                   "y",
                                   y);

    return;
}

/**
 * @brief R40: a subcommand's parse-complete callback, its required option
 *        left out and its parent's flag refused.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r40(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");
    struct d_arg_option* const  y   = d_tests_arg_make_option(_run,
                                                              "y",
                                                              sub,
                                                              "--y");

    d_arg_option_set_required(y,
                              true);
    d_arg_command_set_parse_complete_callback(sub,
                                              d_tests_arg_process_complete,
                                              _run,
                                              NULL);
    (void)d_tests_arg_make_flag(_run,
                                "z",
                                _app,
                                "--z");

    return;
}

/**
 * @brief S17: an option's callback run as each use is parsed, keeping the
 *        last value.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s17(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const t =
        d_arg_command_add_option_callback(_app,
                                          "--t",
                                          d_tests_arg_process_value,
                                          _run,
                                          NULL,
                                          NULL);

    (void)d_tests_arg_watch_option(_run,
                                   "t",
                                   t);
    d_arg_option_set_trigger_on_parse(t,
                                      true);
    d_arg_option_set_multi_option_policy(t,
                                         (int)D_ARG_MULTI_TAKE_LAST);

    return;
}

/**
 * @brief S18: an immediate root's final callback after its subcommand's.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s18(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_immediate_callback(_app,
                                         true);
    d_arg_command_set_final_callback(_app,
                                     d_tests_arg_process_final,
                                     _run,
                                     NULL);

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_final_callback(sub,
                                     d_tests_arg_process_final,
                                     _run,
                                     NULL);

    return;
}

/**
 * @brief S19: the root's parse-complete callback, then the final ones.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s19(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_parse_complete_callback(_app,
                                              d_tests_arg_process_complete,
                                              _run,
                                              NULL);
    d_arg_command_set_final_callback(_app,
                                     d_tests_arg_process_final,
                                     _run,
                                     NULL);

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_final_callback(sub,
                                     d_tests_arg_process_final,
                                     _run,
                                     NULL);

    return;
}

/**
 * @brief S20: pre-parse callbacks, given the arguments left.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s20(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_preparse_callback(_app,
                                        d_tests_arg_process_preparse,
                                        _run,
                                        NULL);
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_preparse_callback(sub,
                                        d_tests_arg_process_preparse,
                                        _run,
                                        NULL);

    return;
}

/**
 * @brief S35: final callbacks at three levels, and the deepest
 *        parse-complete one.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s35(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_final_callback(_app,
                                     d_tests_arg_process_final,
                                     _run,
                                     NULL);

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_final_callback(sub,
                                     d_tests_arg_process_final,
                                     _run,
                                     NULL);

    struct d_arg_command* const deep = d_tests_arg_make_subcommand(_run,
                                                                   sub,
                                                                   "deep");

    d_arg_command_set_final_callback(deep,
                                     d_tests_arg_process_final,
                                     _run,
                                     NULL);
    d_arg_command_set_parse_complete_callback(deep,
                                              d_tests_arg_process_complete,
                                              _run,
                                              NULL);

    return;
}

/**
 * @brief S36: pre-callbacks, which CLI11 has an App override, before the
 *        final callbacks.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s36(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_pre_callback(_app,
                                   d_tests_arg_process_pre,
                                   _run,
                                   NULL);

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_pre_callback(sub,
                                   d_tests_arg_process_pre,
                                   _run,
                                   NULL);
    d_arg_command_set_final_callback(sub,
                                     d_tests_arg_process_final,
                                     _run,
                                     NULL);
    d_arg_command_set_final_callback(_app,
                                     d_tests_arg_process_final,
                                     _run,
                                     NULL);

    return;
}

/**
 * @brief R45: an option group's parse-complete and final callbacks, the
 *        group not used.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r45(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const g  = d_tests_arg_make_group(_run,
                                                            _app,
                                                            "G");
    struct d_arg_option* const  go =
        d_arg_command_add_option_callback(g,
                                          "--g",
                                          d_tests_arg_process_value,
                                          _run,
                                          NULL,
                                          NULL);

    (void)d_tests_arg_watch_option(_run,
                                   "g",
                                   go);
    d_arg_command_set_parse_complete_callback(g,
                                              d_tests_arg_process_complete,
                                              _run,
                                              NULL);
    d_arg_command_set_final_callback(g,
                                     d_tests_arg_process_final,
                                     _run,
                                     NULL);
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                _app,
                                "--f");

    return;
}

// CALLBACK_CASES
//   constant: the cases of d_tests_arg_process_callbacks.
static const struct d_tests_arg_case CALLBACK_CASES[] =
{
    { "R20",
      d_tests_arg_process_r20,
      { "--x", "1", "sub", "--y", "2" },
      NULL,
      NULL },
    { "R37",
      d_tests_arg_process_r37,
      { "--g", "1" },
      NULL,
      NULL },
    { "R38",
      d_tests_arg_process_r38,
      { NULL },
      NULL,
      NULL },
    { "R39",
      d_tests_arg_process_r39,
      { "--x", "1", "sub", "--y", "2" },
      NULL,
      NULL },
    { "R40",
      d_tests_arg_process_r40,
      { "sub", "--z" },
      NULL,
      NULL },
    { "S17",
      d_tests_arg_process_s17,
      { "--t", "1", "--t", "2" },
      NULL,
      NULL },
    { "S18",
      d_tests_arg_process_s18,
      { "sub" },
      NULL,
      NULL },
    { "S19",
      d_tests_arg_process_s19,
      { "sub" },
      NULL,
      NULL },
    { "S20",
      d_tests_arg_process_s20,
      { "a", "sub", "b", "c" },
      NULL,
      NULL },
    { "S35",
      d_tests_arg_process_s35,
      { "sub", "deep" },
      NULL,
      NULL },
    { "S36",
      d_tests_arg_process_s36,
      { "sub" },
      NULL,
      NULL },
    { "R45",
      d_tests_arg_process_r45,
      { "--f" },
      NULL,
      NULL }
};

// CALLBACK_COUNT
//   constant: how many cases CALLBACK_CASES holds.
static const size_t CALLBACK_COUNT =
    sizeof(CALLBACK_CASES) / sizeof(CALLBACK_CASES[0]);

/*
d_tests_arg_process_callbacks
  The order in which processing runs callbacks.
  Tests the following:
  - options', parse-complete and final callbacks of a command and its
    subcommand, and of an option group
  - a callback forced for an option given only default text
  - an immediate subcommand, and an immediate root
  - a parse-complete callback before a refused parent's flag
  - an option triggering on each use, pre-parse callbacks, three levels of final
    callbacks, and pre-callbacks
  - an option group's callbacks, the group not used
*/
bool
d_tests_arg_process_callbacks(void)
{
    return d_tests_arg_cases("CALLBACKS",
                             CALLBACK_CASES,
                             CALLBACK_COUNT,
                             CALLBACKS);
}

//==============================================================================
// 8.  FAILURES
//==============================================================================

/**
 * @brief R21: a final callback failing with a validation error.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r21(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    d_arg_command_set_final_callback(_app,
                                     d_tests_arg_process_bad,
                                     NULL,
                                     NULL);

    return;
}

/**
 * @brief R22: a final callback ending with success.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r22(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    d_arg_command_set_final_callback(_app,
                                     d_tests_arg_process_success,
                                     NULL,
                                     NULL);

    return;
}

/**
 * @brief R23: a final callback ending with a runtime error.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r23(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    d_arg_command_set_final_callback(_app,
                                     d_tests_arg_process_runtime,
                                     NULL,
                                     NULL);

    return;
}

/**
 * @brief R24: an option's callback refusing its values.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r24(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const x =
        d_arg_command_add_option_callback(_app,
                                          "--x",
                                          d_tests_arg_process_refuse,
                                          NULL,
                                          NULL,
                                          NULL);

    (void)d_tests_arg_watch_option(_run,
                                   "x",
                                   x);

    return;
}

/**
 * @brief S16: an option's callback failing with a message.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s16(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const x =
        d_arg_command_add_option_callback(_app,
                                          "--x",
                                          d_tests_arg_process_nope,
                                          NULL,
                                          NULL,
                                          NULL);

    (void)d_tests_arg_watch_option(_run,
                                   "x",
                                   x);

    return;
}

/**
 * @brief S37: a function run on each value, failing.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s37(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const x = d_tests_arg_make_option(_run,
                                                           "x",
                                                           _app,
                                                           "--x");

    (void)d_arg_option_each(x,
                            d_tests_arg_process_each_nope,
                            NULL,
                            NULL);

    return;
}

// FAILURE_CASES
//   constant: the cases of d_tests_arg_process_failures.
static const struct d_tests_arg_case FAILURE_CASES[] =
{
    { "R21",
      d_tests_arg_process_r21,
      { NULL },
      NULL,
      NULL },
    { "R22",
      d_tests_arg_process_r22,
      { NULL },
      NULL,
      NULL },
    { "R23",
      d_tests_arg_process_r23,
      { NULL },
      NULL,
      NULL },
    { "R24",
      d_tests_arg_process_r24,
      { "--x", "1" },
      NULL,
      NULL },
    { "S16",
      d_tests_arg_process_s16,
      { "--x", "1" },
      NULL,
      NULL },
    { "S37",
      d_tests_arg_process_s37,
      { "--x", "1" },
      NULL,
      NULL }
};

// FAILURE_COUNT
//   constant: how many cases FAILURE_CASES holds.
static const size_t FAILURE_COUNT =
    sizeof(FAILURE_CASES) / sizeof(FAILURE_CASES[0]);

/*
d_tests_arg_process_failures
  Callbacks that end a parse.
  Tests the following:
  - a final callback failing with an error, ending with success, and with a
    runtime error's exit code
  - an option's callback refusing its values, and failing with a message
  - a function run on each value failing
*/
bool
d_tests_arg_process_failures(void)
{
    return d_tests_arg_cases("FAILURES",
                             FAILURE_CASES,
                             FAILURE_COUNT,
                             FAILURES);
}

//==============================================================================
// 9.  PRIORITIES
//==============================================================================

/**
 * @brief R25: options' callbacks by priority: last, first, and normal.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r25(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const a =
        d_arg_command_add_option_callback(_app,
                                          "--a",
                                          d_tests_arg_process_value,
                                          _run,
                                          NULL,
                                          NULL);

    (void)d_tests_arg_watch_option(_run,
                                   "a",
                                   a);
    d_arg_option_set_callback_priority(a,
                                       (int)D_ARG_PRIORITY_LAST);

    struct d_arg_option* const b =
        d_arg_command_add_option_callback(_app,
                                          "--b",
                                          d_tests_arg_process_value,
                                          _run,
                                          NULL,
                                          NULL);

    (void)d_tests_arg_watch_option(_run,
                                   "b",
                                   b);
    d_arg_option_set_callback_priority(b,
                                       (int)D_ARG_PRIORITY_FIRST);

    struct d_arg_option* const c =
        d_arg_command_add_option_callback(_app,
                                          "--c",
                                          d_tests_arg_process_value,
                                          _run,
                                          NULL,
                                          NULL);

    (void)d_tests_arg_watch_option(_run,
                                   "c",
                                   c);

    return;
}

// PRIORITY_CASES
//   constant: the cases of d_tests_arg_process_priorities.
static const struct d_tests_arg_case PRIORITY_CASES[] =
{
    { "R25",
      d_tests_arg_process_r25,
      { "--a", "1", "--b", "2", "--c", "3" },
      NULL,
      NULL }
};

// PRIORITY_COUNT
//   constant: how many cases PRIORITY_CASES holds.
static const size_t PRIORITY_COUNT =
    sizeof(PRIORITY_CASES) / sizeof(PRIORITY_CASES[0]);

/*
d_tests_arg_process_priorities
  The order of options' callbacks by priority.
  Tests the following:
  - an option asking to run last, one first, and one in between
*/
bool
d_tests_arg_process_priorities(void)
{
    return d_tests_arg_cases("PRIORITIES",
                             PRIORITY_CASES,
                             PRIORITY_COUNT,
                             PRIORITIES);
}

//==============================================================================
// 10. EXTRAS
//==============================================================================

/**
 * @brief R15: an option of one value given twice.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r15(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_option(_run,
                                  "o",
                                  _app,
                                  "--o");

    return;
}

/**
 * @brief R33: an extra after a subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r33(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_subcommand(_run,
                                      _app,
                                      "sub");

    return;
}

/**
 * @brief R34: a prefix command passing on the rest at `--`, after a
 *        positional.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r34(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    d_arg_command_set_prefix_command(_app,
                                     D_ARG_PREFIX_SEPARATOR_ONLY);

    return;
}

/**
 * @brief R35: the same, `--` first.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r35(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_run;

    d_arg_command_set_prefix_command(_app,
                                     D_ARG_PREFIX_SEPARATOR_ONLY);

    return;
}

/**
 * @brief S10: extras kept by the command around a subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s10(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);
    (void)d_tests_arg_make_subcommand(_run,
                                      _app,
                                      "sub");

    return;
}

/**
 * @brief S11: extras kept by a subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s11(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_allow_extras(sub,
                                   D_ARG_EXTRAS_CAPTURE);

    return;
}

/**
 * @brief S12: an extra kept by an option group.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s12(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const g = d_tests_arg_make_group(_run,
                                                           _app,
                                                           "G");

    (void)d_tests_arg_make_flag(_run,
                                "g",
                                g,
                                "--g");
    d_arg_command_set_allow_extras(g,
                                   D_ARG_EXTRAS_CAPTURE);
    (void)d_tests_arg_make_subcommand(_run,
                                      _app,
                                      "sub");

    return;
}

/**
 * @brief S13: an extra kept by an option group after its flag.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s13(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const g = d_tests_arg_make_group(_run,
                                                           _app,
                                                           "G");

    (void)d_tests_arg_make_flag(_run,
                                "g",
                                g,
                                "--g");
    d_arg_command_set_allow_extras(g,
                                   D_ARG_EXTRAS_CAPTURE);
    (void)d_tests_arg_make_subcommand(_run,
                                      _app,
                                      "sub");

    return;
}

// EXTRAS_CASES
//   constant: the cases of d_tests_arg_process_extras.
static const struct d_tests_arg_case EXTRAS_CASES[] =
{
    { "R15",
      d_tests_arg_process_r15,
      { "--o", "1", "--o", "2" },
      NULL,
      NULL },
    { "R33",
      d_tests_arg_process_r33,
      { "sub", "x" },
      NULL,
      NULL },
    { "R34",
      d_tests_arg_process_r34,
      { "x", "--", "a" },
      NULL,
      NULL },
    { "R35",
      d_tests_arg_process_r35,
      { "--", "a", "b" },
      NULL,
      NULL },
    { "S10",
      d_tests_arg_process_s10,
      { "x", "sub", "y" },
      NULL,
      NULL },
    { "S11",
      d_tests_arg_process_s11,
      { "sub", "x" },
      NULL,
      NULL },
    { "S12",
      d_tests_arg_process_s12,
      { "x" },
      NULL,
      NULL },
    { "S13",
      d_tests_arg_process_s13,
      { "--g", "x" },
      NULL,
      NULL }
};

// EXTRAS_COUNT
//   constant: how many cases EXTRAS_CASES holds.
static const size_t EXTRAS_COUNT =
    sizeof(EXTRAS_CASES) / sizeof(EXTRAS_CASES[0]);

/*
d_tests_arg_process_extras
  Arguments left over when processing ends.
  Tests the following:
  - an option given twice, and an extra after a subcommand
  - a prefix command passing on the rest at `--` alone
  - extras kept by the command, a subcommand, and an option group
*/
bool
d_tests_arg_process_extras(void)
{
    return d_tests_arg_cases("EXTRAS",
                             EXTRAS_CASES,
                             EXTRAS_COUNT,
                             EXTRAS);
}

//==============================================================================
// 11. ENVIRONMENT
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_ENVIRONMENT == 1)

/**
 * @brief R26: an option read from the environment, and one whose variable
 *        fails its validator.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r26(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const e = d_tests_arg_make_option(_run,
                                                           "e",
                                                           _app,
                                                           "--e");

    (void)d_arg_option_set_env_name(e,
                                    "DJ_ARG_E");

    struct d_arg_option* const f = d_tests_arg_make_option(_run,
                                                           "f",
                                                           _app,
                                                           "--f");

    (void)d_arg_option_set_env_name(f,
                                    "DJ_ARG_F");
    (void)d_arg_option_check(f,
                             d_arg_validator_positive_number(),
                             NULL);

    return;
}

/**
 * @brief S23a: a subcommand's option with a variable, the subcommand not
 *        used.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s23a(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");
    struct d_arg_option* const  y   = d_tests_arg_make_option(_run,
                                                              "y",
                                                              sub,
                                                              "--y");

    (void)d_arg_option_set_env_name(y,
                                    "DJ_ARG_Y");

    return;
}

/**
 * @brief S23b: the same, the subcommand used.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s23b(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");
    struct d_arg_option* const  y   = d_tests_arg_make_option(_run,
                                                              "y",
                                                              sub,
                                                              "--y");

    (void)d_arg_option_set_env_name(y,
                                    "DJ_ARG_Y");

    return;
}

/**
 * @brief S24: an option group's option with a variable.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s24(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const g  = d_tests_arg_make_group(_run,
                                                            _app,
                                                            "G");
    struct d_arg_option* const  go = d_tests_arg_make_option(_run,
                                                             "g",
                                                             g,
                                                             "--g");

    (void)d_arg_option_set_env_name(go,
                                    "DJ_ARG_G");

    return;
}

/**
 * @brief S25: an option excluding another that its variable gives.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s25(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const a = d_tests_arg_make_flag(_run,
                                                         "a",
                                                         _app,
                                                         "--a");
    struct d_arg_option* const b = d_tests_arg_make_flag(_run,
                                                         "b",
                                                         _app,
                                                         "--b");

    (void)d_arg_option_set_env_name(b,
                                    "DJ_ARG_B");
    (void)d_arg_option_excludes(a,
                                b);

    return;
}

/**
 * @brief S26: a required option its variable gives.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_s26(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const x = d_tests_arg_make_option(_run,
                                                           "x",
                                                           _app,
                                                           "--x");

    d_arg_option_set_required(x,
                              true);
    (void)d_arg_option_set_env_name(x,
                                    "DJ_ARG_X");

    return;
}

/**
 * @brief R41: an option given, its variable set too.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r41(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const e = d_tests_arg_make_option(_run,
                                                           "e",
                                                           _app,
                                                           "--e");

    (void)d_arg_option_set_env_name(e,
                                    "DJ_ARG_E");

    return;
}

// R26_VARIABLES
//   constant: the environment variables R26 parses with.
static const struct d_tests_arg_env_var R26_VARIABLES[] =
{
    { "DJ_ARG_E", "5" },
    { "DJ_ARG_F", "abc" },
    { NULL,       NULL }
};

// S23A_VARIABLES
//   constant: the environment variables S23a parses with.
static const struct d_tests_arg_env_var S23A_VARIABLES[] =
{
    { "DJ_ARG_Y", "9" },
    { NULL,       NULL }
};

// S23B_VARIABLES
//   constant: the environment variables S23b parses with.
static const struct d_tests_arg_env_var S23B_VARIABLES[] =
{
    { "DJ_ARG_Y", "9" },
    { NULL,       NULL }
};

// S24_VARIABLES
//   constant: the environment variables S24 parses with.
static const struct d_tests_arg_env_var S24_VARIABLES[] =
{
    { "DJ_ARG_G", "4" },
    { NULL,       NULL }
};

// S25_VARIABLES
//   constant: the environment variables S25 parses with.
static const struct d_tests_arg_env_var S25_VARIABLES[] =
{
    { "DJ_ARG_B", "1" },
    { NULL,       NULL }
};

// S26_VARIABLES
//   constant: the environment variables S26 parses with.
static const struct d_tests_arg_env_var S26_VARIABLES[] =
{
    { "DJ_ARG_X", "1" },
    { NULL,       NULL }
};

// R41_VARIABLES
//   constant: the environment variables R41 parses with.
static const struct d_tests_arg_env_var R41_VARIABLES[] =
{
    { "DJ_ARG_E", "5" },
    { NULL,       NULL }
};

// ENVIRONMENT_CASES
//   constant: the cases of d_tests_arg_process_environment.
static const struct d_tests_arg_case ENVIRONMENT_CASES[] =
{
    { "R26",
      d_tests_arg_process_r26,
      { NULL },
      R26_VARIABLES,
      NULL },
    { "S23a",
      d_tests_arg_process_s23a,
      { NULL },
      S23A_VARIABLES,
      NULL },
    { "S23b",
      d_tests_arg_process_s23b,
      { "sub" },
      S23B_VARIABLES,
      NULL },
    { "S24",
      d_tests_arg_process_s24,
      { NULL },
      S24_VARIABLES,
      NULL },
    { "S25",
      d_tests_arg_process_s25,
      { "--a" },
      S25_VARIABLES,
      NULL },
    { "S26",
      d_tests_arg_process_s26,
      { NULL },
      S26_VARIABLES,
      NULL },
    { "R41",
      d_tests_arg_process_r41,
      { "--e", "1" },
      R41_VARIABLES,
      NULL }
};

// ENVIRONMENT_COUNT
//   constant: how many cases ENVIRONMENT_CASES holds.
static const size_t ENVIRONMENT_COUNT =
    sizeof(ENVIRONMENT_CASES) / sizeof(ENVIRONMENT_CASES[0]);

/*
d_tests_arg_process_environment
  Values read from environment variables.
  Tests the following:
  - an option read from the environment, and one whose variable fails its
    validator
  - a subcommand's option read only when the subcommand is used
  - an option group's option read from the environment
  - a variable counting as the option for excludes, and for required
  - an option given, whose variable is then not read
*/
bool
d_tests_arg_process_environment(void)
{
    return d_tests_arg_cases("ENVIRONMENT",
                             ENVIRONMENT_CASES,
                             ENVIRONMENT_COUNT,
                             ENVIRONMENT);
}

#endif  // D_INTERNAL_ARG_PARSER_ENVIRONMENT

//==============================================================================
// 12. CONFIGURATION
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief R27: a configuration file named that is not there.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r27(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const config = d_arg_command_set_config(_app,
                                                                 "--config",
                                                                 NULL,
                                                                 NULL,
                                                                 false);

    (void)d_tests_arg_watch_option(_run,
                                   "config",
                                   config);

    return;
}

/**
 * @brief R28: a default configuration file that is not there.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r28(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const config =
        d_arg_command_set_config(_app,
                                 "--config",
                                 "/nonexistent/default.ini",
                                 NULL,
                                 false);

    (void)d_tests_arg_watch_option(_run,
                                   "config",
                                   config);

    return;
}

/**
 * @brief R29: a required default configuration file that is not there.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r29(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const config =
        d_arg_command_set_config(_app,
                                 "--config",
                                 "/nonexistent/default.ini",
                                 "Read an ini file",
                                 true);

    (void)d_tests_arg_watch_option(_run,
                                   "config",
                                   config);

    return;
}

/**
 * @brief R30: a required configuration file with no default.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r30(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const config =
        d_arg_command_set_config(_app,
                                 "--config",
                                 "",
                                 "Read an ini file",
                                 true);

    (void)d_tests_arg_watch_option(_run,
                                   "config",
                                   config);

    return;
}

/**
 * @brief R31: a configuration file that is not there, a required option
 *        left out.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r31(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const config = d_arg_command_set_config(_app,
                                                                 "--config",
                                                                 NULL,
                                                                 NULL,
                                                                 false);

    (void)d_tests_arg_watch_option(_run,
                                   "config",
                                   config);

    struct d_arg_option* const x = d_tests_arg_make_option(_run,
                                                           "x",
                                                           _app,
                                                           "--x");

    d_arg_option_set_required(x,
                              true);

    return;
}

// CONFIGURATION_CASES
//   constant: the cases of d_tests_arg_process_configuration.
static const struct d_tests_arg_case CONFIGURATION_CASES[] =
{
    { "R27",
      d_tests_arg_process_r27,
      { "--config", "/nonexistent/probe.ini" },
      NULL,
      NULL },
    { "R28",
      d_tests_arg_process_r28,
      { NULL },
      NULL,
      NULL },
    { "R29",
      d_tests_arg_process_r29,
      { NULL },
      NULL,
      NULL },
    { "R30",
      d_tests_arg_process_r30,
      { NULL },
      NULL,
      NULL },
    { "R31",
      d_tests_arg_process_r31,
      { "--config", "/nonexistent/probe.ini" },
      NULL,
      NULL }
};

// CONFIGURATION_COUNT
//   constant: how many cases CONFIGURATION_CASES holds.
static const size_t CONFIGURATION_COUNT =
    sizeof(CONFIGURATION_CASES) / sizeof(CONFIGURATION_CASES[0]);

/*
d_tests_arg_process_configuration
  Configuration files that are not there.
  Tests the following:
  - a file named on the command line, and a default file, that cannot be read
  - a required default file, and a required file with no default
  - a file that cannot be read before a required option
*/
bool
d_tests_arg_process_configuration(void)
{
    return d_tests_arg_cases("CONFIGURATION",
                             CONFIGURATION_CASES,
                             CONFIGURATION_COUNT,
                             CONFIGURATION);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 13. CONFIGURATION FROM THE ENVIRONMENT
//==============================================================================

#if ( (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1) &&                             \
      (D_INTERNAL_ARG_PARSER_ENVIRONMENT == 1) )

/**
 * @brief R32: a configuration file a variable names, which is not there.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r32(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const config = d_arg_command_set_config(_app,
                                                                 "--config",
                                                                 NULL,
                                                                 NULL,
                                                                 false);

    (void)d_tests_arg_watch_option(_run,
                                   "config",
                                   config);
    (void)d_arg_option_set_env_name(config,
                                    "DJ_ARG_CFG");

    return;
}

/**
 * @brief R50: a required configuration file a variable names, which is not
 *        there.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_process_r50(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const config =
        d_arg_command_set_config(_app,
                                 "--config",
                                 "",
                                 "Read an ini file",
                                 true);

    (void)d_tests_arg_watch_option(_run,
                                   "config",
                                   config);
    (void)d_arg_option_set_env_name(config,
                                    "DJ_ARG_CFG");

    return;
}

// R32_VARIABLES
//   constant: the environment variables R32 parses with.
static const struct d_tests_arg_env_var R32_VARIABLES[] =
{
    { "DJ_ARG_CFG", "/nonexistent/env.ini" },
    { NULL,         NULL }
};

// R50_VARIABLES
//   constant: the environment variables R50 parses with.
static const struct d_tests_arg_env_var R50_VARIABLES[] =
{
    { "DJ_ARG_CFG", "/nonexistent/env.ini" },
    { NULL,         NULL }
};

// CONFIG_ENV_CASES
//   constant: the cases of d_tests_arg_process_configuration_environment.
static const struct d_tests_arg_case CONFIG_ENV_CASES[] =
{
    { "R32",
      d_tests_arg_process_r32,
      { NULL },
      R32_VARIABLES,
      NULL },
    { "R50",
      d_tests_arg_process_r50,
      { NULL },
      R50_VARIABLES,
      NULL }
};

// CONFIG_ENV_COUNT
//   constant: how many cases CONFIG_ENV_CASES holds.
static const size_t CONFIG_ENV_COUNT =
    sizeof(CONFIG_ENV_CASES) / sizeof(CONFIG_ENV_CASES[0]);

/*
d_tests_arg_process_configuration_environment
  A configuration file named by an environment variable.
  Tests the following:
  - the file a variable names, which is not there, needed or not
*/
bool
d_tests_arg_process_configuration_environment(void)
{
    return d_tests_arg_cases("CONFIGURATION_ENVIRONMENT",
                             CONFIG_ENV_CASES,
                             CONFIG_ENV_COUNT,
                             CONFIGURATION_ENVIRONMENT);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES, D_INTERNAL_ARG_PARSER_ENVIRONMENT

//==============================================================================
// 14. THE SUITE
//==============================================================================

// d_tests_arg_process
//   constant: the suite, in the header's order.
const struct d_tests_section d_tests_arg_process[] =
{
    { "arg_process_required",
      d_tests_arg_process_required },
    { "arg_process_relations",
      d_tests_arg_process_relations },
    { "arg_process_subcommand_counts",
      d_tests_arg_process_subcommand_counts },
    { "arg_process_option_counts",
      d_tests_arg_process_option_counts },
    { "arg_process_group_counts",
      d_tests_arg_process_group_counts },
    { "arg_process_help",
      d_tests_arg_process_help },
    { "arg_process_callbacks",
      d_tests_arg_process_callbacks },
    { "arg_process_failures",
      d_tests_arg_process_failures },
    { "arg_process_priorities",
      d_tests_arg_process_priorities },
    { "arg_process_extras",
      d_tests_arg_process_extras },
#if (D_INTERNAL_ARG_PARSER_ENVIRONMENT == 1)
    { "arg_process_environment",
      d_tests_arg_process_environment },
#endif  // D_INTERNAL_ARG_PARSER_ENVIRONMENT
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_process_configuration",
      d_tests_arg_process_configuration },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if ( (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1) &&                             \
      (D_INTERNAL_ARG_PARSER_ENVIRONMENT == 1) )
    { "arg_process_configuration_environment",
      d_tests_arg_process_configuration_environment },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES, D_INTERNAL_ARG_PARSER_ENVIRONMENT
};

// d_tests_arg_process_count
//   constant: how many sections the table above holds.
const size_t d_tests_arg_process_count =
    sizeof(d_tests_arg_process) / sizeof(d_tests_arg_process[0]);
