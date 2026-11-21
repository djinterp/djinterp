/*******************************************************************************
* djinterp [c/parse]                                         arg_setting_tests.c
*
* The suite of the parser's setting hook, declared in arg_setting_tests.h.
*   Most sections are case tables (arg_tests_support.h): each case makes a
* parser as CLI11's App("d", "prog") would, sets the stand-in hook on it,
* parses its arguments -- given without a program name, so the hook's
* `argv[0]` is "" -- and is written as the hook's calls, then the parse's
* report. The rest are steps that parse main's arguments or a line, so that
* the program's name is there to see, or that end a parse as CLI11's exit()
* does, so that the messages a refusal writes are there to read.
*   A call the hook gets is written `hook <at> [<argv>] -> <return>`: the
* index of the offered argument, every argument with `|` between, and what
* the hook returned, with ` status=<n>` after a refusal; `set <name>=<value>`
* follows for a setting it bound.
*
*
* path:      /tests/djinterp/c/modules/parse/cli/arg_parser/arg_setting_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.20
*                                                            revised: 2026.11.20
*******************************************************************************/
#include "./arg_setting_tests.h"  // corresponding header
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <string.h>   // memchr, strchr, strcmp, strcspn, strlen, strncmp
// djinterp
#include "../../../../../../../inc/djinterp/c/util/sink_common.h"  // dj_pack_text
#include "../../../../../../../inc/djinterp/config/c/modules/parse/cli/arg_parser/cfg_arg_parser.h"  // the knobs
#include "../../../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_command.h"  // commands, their settings
#include "../../../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_common.h"  // buffers, statuses, modes
#include "../../../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_option.h"  // options, their settings
#include "../../../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_parser.h"  // the parser, its hook
#include "../../../../../../../inc/re_std/c/cstdint/dstdint.h"  // int32_t
#include "../../tests_section.h"  // dj_tests_section
#include "./arg_tests_support.h"  // runs, case tables, escaping


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// SETTING_KNOWN
//   constant: the one setting the stand-in hook knows, as the probe's kit
// knew the plugin search path.
static const char SETTING_KNOWN[] = "cli.plugin_search";

// SETTING_UNKNOWN / SETTING_UNVALUED / SETTING_UNPAIRED / SETTING_OTHER
//   constant: the reasons the stand-in hook gives for a refusal: a name it
// does not know, a dotted name without a value, `--set` or `--put` without
// a `NAME=VALUE` after it, and the refusal that returns another count.
#define SETTING_UNKNOWN  7
#define SETTING_UNVALUED 9
#define SETTING_UNPAIRED 8
#define SETTING_OTHER    5

// PROBE
//   constant: what the probe's cases come to, the hook's calls before each
// report.
static const char PROBE[] =
#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)
    "P01 [@rf show]\n"
    "hook 1 [|--cli.plugin_search=/rf|show] -> 1\n"
    "set cli.plugin_search=/rf\n"
    "cmd app n=1 miss=[] subs=[show] order=[]\n"
    "cmd show n=1 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES
    "P02 [--out --cli.plugin_search=/ab show]\n"
    "hook 1 [|--out|--cli.plugin_search=/ab|show] -> 0\n"
    "cmd app n=1 miss=[] subs=[show] order=[--output]\n"
    "cmd show n=1 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=1 res=[--cli.plugin_search=/ab]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "P03 [-ab --cli.plugin_search=/ns show]\n"
    "cmd app n=1 miss=[] subs=[show] order=[-ab]\n"
    "cmd show n=1 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=1 res=[--cli.plugin_search=/ns]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "P04 [pre --mod.depth=3 x]\n"
    "cmd app n=1 miss=[] subs=[pre] order=[]\n"
    "cmd show n=0 miss=[] subs=[] order=[]\n"
    "cmd pre n=1 miss=[--mod.depth=3|x] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "P05 [--output --cli.plugin_search=/ab show]\n"
    "cmd app n=1 miss=[] subs=[show] order=[--output]\n"
    "cmd show n=1 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=1 res=[--cli.plugin_search=/ab]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "P06 [sh --mode m --cli.plugin_search=/sh w]\n"
    "hook 4 [|sh|--mode|m|--cli.plugin_search=/sh|w] -> 1\n"
    "set cli.plugin_search=/sh\n"
    "cmd app n=1 miss=[] subs=[show] order=[]\n"
    "cmd show n=1 miss=[] subs=[] order=[--mode|word]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=1 res=[m]\n"
    "opt word n=1 res=[w]\n"
    "P07 [--set cli.plugin_search=/s show]\n"
    "hook 1 [|--set|cli.plugin_search=/s|show] -> 2\n"
    "set cli.plugin_search=/s\n"
    "cmd app n=1 miss=[] subs=[show] order=[]\n"
    "cmd show n=1 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "P08 [show -- --cli.plugin_search=/x]\n"
    "cmd app n=1 miss=[] subs=[show] order=[]\n"
    "cmd show n=1 miss=[--] subs=[] order=[word]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=1 res=[--cli.plugin_search=/x]\n"
    "P09 [--cli.nope=1 show]\n"
    "hook 1 [|--cli.nope=1|show] -> -1 status=7\n"
    "ERR SettingError 2: --cli.nope=1\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd show n=0 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "P10 [--set cli.plugin_search=/s show]\n"
    "hook 1 [|--set|cli.plugin_search=/s|show] -> 2\n"
    "set cli.plugin_search=/s\n"
    "cmd app n=1 miss=[] subs=[show] order=[]\n"
    "cmd show n=1 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "opt settings n=0 res=[]\n";

// OFFERED
//   constant: what the cases of d_tests_arg_setting_offered come to.
static const char OFFERED[] =
    "N01 [--LE_VEL 3 show]\n"
    "cmd app n=1 miss=[] subs=[show] order=[--level]\n"
    "cmd show n=1 miss=[] subs=[] order=[]\n"
    "cmd list n=0 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=1 res=[3]\n"
    "opt output n=0 res=[]\n"
    "opt many n=0 res=[]\n"
    "opt color n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "opt item n=0 res=[]\n"
    "N02 [--color red]\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd show n=0 miss=[] subs=[] order=[]\n"
    "cmd list n=0 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt many n=0 res=[]\n"
    "opt color n=1 res=[red]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "opt item n=0 res=[]\n"
    "N03 [show --level 3]\n"
    "cmd app n=1 miss=[] subs=[show] order=[--level]\n"
    "cmd show n=1 miss=[] subs=[] order=[]\n"
    "cmd list n=0 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=1 res=[3]\n"
    "opt output n=0 res=[]\n"
    "opt many n=0 res=[]\n"
    "opt color n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "opt item n=0 res=[]\n"
    "N04 [list --level 3]\n"
    "hook 2 [|list|--level|3] -> 0\n"
    "ERR ExtrasError 109: list: The following argument was not expected: "
    "--level\n"
    "cmd app n=1 miss=[] subs=[list] order=[]\n"
    "cmd show n=0 miss=[] subs=[] order=[]\n"
    "cmd list n=1 miss=[--level] subs=[] order=[item]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt many n=0 res=[]\n"
    "opt color n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "opt item n=1 res=[3]\n"
    "N05 [--show.mode=m]\n"
    "cmd app n=2 miss=[] subs=[show] order=[]\n"
    "cmd show n=0 miss=[] subs=[] order=[--mode]\n"
    "cmd list n=0 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt many n=0 res=[]\n"
    "opt color n=0 res=[]\n"
    "opt mode n=1 res=[m]\n"
    "opt word n=0 res=[]\n"
    "opt item n=0 res=[]\n"
    "N06 [--show.nope=1]\n"
    "hook 1 [|--show.nope=1] -> -1 status=7\n"
    "ERR SettingError 2: --show.nope=1\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd show n=0 miss=[] subs=[] order=[]\n"
    "cmd list n=0 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt many n=0 res=[]\n"
    "opt color n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "opt item n=0 res=[]\n"
    "N07 [--lev 3]\n"
    "hook 1 [|--lev|3] -> 0\n"
    "cmd app n=1 miss=[] subs=[] order=[--level]\n"
    "cmd show n=0 miss=[] subs=[] order=[]\n"
    "cmd list n=0 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=1 res=[3]\n"
    "opt output n=0 res=[]\n"
    "opt many n=0 res=[]\n"
    "opt color n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "opt item n=0 res=[]\n"
#if (D_INTERNAL_ARG_PARSER_SUGGESTIONS == 1)
    "N08 [--levl 3]\n"
    "hook 1 [|--levl|3] -> 0\n"
    "ERR ExtrasError 109: prog: The following arguments were not expected: "
    "--levl 3\\nDid you mean '--level' for '--levl'?\n"
    "cmd app n=1 miss=[--levl|3] subs=[] order=[]\n"
    "cmd show n=0 miss=[] subs=[] order=[]\n"
    "cmd list n=0 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt many n=0 res=[]\n"
    "opt color n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "opt item n=0 res=[]\n"
#endif  // D_INTERNAL_ARG_PARSER_SUGGESTIONS
    "";

// OFFERED_PLACE
//   constant: what the cases of d_tests_arg_setting_offered_place come to.
static const char OFFERED_PLACE[] =
    "W01 [-- --cli.plugin_search=/t]\n"
    "ERR ExtrasError 109: prog: The following arguments were not expected: -- "
    "--cli.plugin_search=/t\n"
    "cmd app n=1 miss=[--|--cli.plugin_search=/t] subs=[] order=[]\n"
    "cmd show n=0 miss=[] subs=[] order=[]\n"
    "cmd list n=0 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt many n=0 res=[]\n"
    "opt color n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "opt item n=0 res=[]\n"
    "W02 [show w -- --cli.plugin_search=/t]\n"
    "hook 4 [|show|w|--|--cli.plugin_search=/t] -> 1\n"
    "set cli.plugin_search=/t\n"
    "cmd app n=1 miss=[] subs=[show] order=[]\n"
    "cmd show n=1 miss=[] subs=[] order=[word]\n"
    "cmd list n=0 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt many n=0 res=[]\n"
    "opt color n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=1 res=[w]\n"
    "opt item n=0 res=[]\n"
    "W03 [pre --cli.plugin_search=/p x]\n"
    "cmd app n=1 miss=[] subs=[pre] order=[]\n"
    "cmd show n=0 miss=[] subs=[] order=[]\n"
    "cmd list n=0 miss=[] subs=[] order=[]\n"
    "cmd pre n=1 miss=[--cli.plugin_search=/p|x] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt many n=0 res=[]\n"
    "opt color n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "opt item n=0 res=[]\n"
    "W04 [show --cli.plugin_search=/k x]\n"
    "cmd app n=1 miss=[--cli.plugin_search=/k|x] subs=[show] order=[]\n"
    "cmd show n=1 miss=[] subs=[] order=[]\n"
    "cmd list n=0 miss=[] subs=[] order=[]\n"
    "W05 [list --cli.plugin_search=/k x]\n"
    "hook 2 [|list|--cli.plugin_search=/k|x] -> 1\n"
    "set cli.plugin_search=/k\n"
    "ERR ExtrasError 109: list: The following argument was not expected: x\n"
    "cmd app n=1 miss=[] subs=[list] order=[]\n"
    "cmd show n=0 miss=[] subs=[] order=[]\n"
    "cmd list n=1 miss=[x] subs=[] order=[]\n"
    "W06 [show --cli.plugin_search=/f x]\n"
    "hook 2 [|show|--cli.plugin_search=/f|x] -> 1\n"
    "set cli.plugin_search=/f\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: x\n"
    "cmd app n=1 miss=[x] subs=[show] order=[]\n"
    "cmd show n=1 miss=[] subs=[] order=[]\n"
    "W07 [--cli.plugin_search=/s1 -- --cli.plugin_search=/s2]\n"
    "hook 1 [|--cli.plugin_search=/s1|--|--cli.plugin_search=/s2] -> 1\n"
    "set cli.plugin_search=/s1\n"
    "cmd app n=1 miss=[--|--cli.plugin_search=/s2] subs=[] order=[]\n"
    "W08 [--cli.plugin_search=/p1 word --cli.plugin_search=/p2]\n"
    "hook 1 [|--cli.plugin_search=/p1|word|--cli.plugin_search=/p2] -> 1\n"
    "set cli.plugin_search=/p1\n"
    "cmd app n=1 miss=[word|--cli.plugin_search=/p2] subs=[] order=[]\n"
    "W09 [--many a --cli.plugin_search=/v b]\n"
    "hook 3 [|--many|a|--cli.plugin_search=/v|b] -> 1\n"
    "set cli.plugin_search=/v\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: b\n"
    "cmd app n=1 miss=[b] subs=[] order=[--many]\n"
    "cmd show n=0 miss=[] subs=[] order=[]\n"
    "cmd list n=0 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt many n=1 res=[a]\n"
    "opt color n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "opt item n=0 res=[]\n"
    "W10 [/cli.plugin_search:/w /level:2]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "/cli.plugin_search:/w\n"
    "cmd app n=1 miss=[/cli.plugin_search:/w] subs=[] order=[--level]\n"
    "opt level n=1 res=[2]\n"
    "W11 [-o --cli.plugin_search=/o --level=--cli.plugin_search=/e]\n"
    "cmd app n=1 miss=[] subs=[] order=[--output|--level]\n"
    "cmd show n=0 miss=[] subs=[] order=[]\n"
    "cmd list n=0 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=1 res=[--cli.plugin_search=/e]\n"
    "opt output n=1 res=[--cli.plugin_search=/o]\n"
    "opt many n=0 res=[]\n"
    "opt color n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "opt item n=0 res=[]\n";

// SEES
//   constant: what the cases of d_tests_arg_setting_sees come to.
static const char SEES[] =
#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)
    "S01 [@a.rsp show --cli.plugin_search=/c]\n"
    "hook 3 [|--level|2|--cli.plugin_search=/b|show|--cli.plugin_search=/c] -> "
    "1\n"
    "set cli.plugin_search=/b\n"
    "hook 5 [|--level|2|--cli.plugin_search=/b|show|--cli.plugin_search=/c] -> "
    "1\n"
    "set cli.plugin_search=/c\n"
    "cmd app n=1 miss=[] subs=[show] order=[--level]\n"
    "cmd show n=1 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=1 res=[2]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES
    "S02 [show --mode m --cli.plugin_search=/x w]\n"
    "hook 4 [|show|--mode|m|--cli.plugin_search=/x|w] -> 1\n"
    "set cli.plugin_search=/x\n"
    "cmd app n=1 miss=[] subs=[show] order=[]\n"
    "cmd show n=1 miss=[] subs=[] order=[--mode|word]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=1 res=[m]\n"
    "opt word n=1 res=[w]\n"
    "S03 [show --cli.plugin_search=/1 --cli.plugin_search=/2]\n"
    "hook 2 [|show|--cli.plugin_search=/1|--cli.plugin_search=/2] -> 1\n"
    "set cli.plugin_search=/1\n"
    "hook 3 [|show|--cli.plugin_search=/1|--cli.plugin_search=/2] -> 1\n"
    "set cli.plugin_search=/2\n"
    "cmd app n=1 miss=[] subs=[show] order=[]\n"
    "cmd show n=1 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n";

// SEES_STEPS
//   constant: what the steps of d_tests_arg_setting_sees write.
static const char SEES_STEPS[] =
    "S04 main [probe show --cli.plugin_search=/m]\n"
    "hook 2 [probe|show|--cli.plugin_search=/m] -> 1\n"
    "set cli.plugin_search=/m\n"
    "cmd app n=1 miss=[] subs=[show] order=[]\n"
    "cmd show n=1 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "S05 line named [probe show --cli.plugin_search=/l]\n"
    "hook 2 [probe|show|--cli.plugin_search=/l] -> 1\n"
    "set cli.plugin_search=/l\n"
    "cmd app n=1 miss=[] subs=[show] order=[]\n"
    "cmd show n=1 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "S05 line [show --cli.plugin_search=/n]\n"
    "hook 2 [|show|--cli.plugin_search=/n] -> 1\n"
    "set cli.plugin_search=/n\n"
    "cmd app n=1 miss=[] subs=[show] order=[]\n"
    "cmd show n=1 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "S06 main [probe show --level 2]\n"
    "ERR ExtrasError 109: show: The following argument was not expected: "
    "--level\n"
    "cmd app n=1 miss=[] subs=[show] order=[]\n"
    "cmd show n=1 miss=[--level] subs=[] order=[word]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=1 res=[2]\n"
    "S06 no hook kept=0 status=0\n"
    "S06 main [probe show --level 2]\n"
    "hook 2 [probe|show|--level|2] -> 0\n"
    "ERR ExtrasError 109: show: The following argument was not expected: "
    "--level\n"
    "cmd app n=1 miss=[] subs=[show] order=[]\n"
    "cmd show n=1 miss=[--level] subs=[] order=[word]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=1 res=[2]\n"
    "S06 hook kept=4 status=0\n"
    "S06 cleared kept=0 status=0\n";

// TAKES
//   constant: what the cases of d_tests_arg_setting_takes come to.
static const char TAKES[] =
    "T01 [--set cli.plugin_search=/a show]\n"
    "hook 1 [|--set|cli.plugin_search=/a|show] -> 2\n"
    "set cli.plugin_search=/a\n"
    "cmd app n=1 miss=[] subs=[show] order=[]\n"
    "cmd show n=1 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "T02 [--put cli.plugin_search=/b show]\n"
    "hook 1 [|--put|cli.plugin_search=/b|show] -> 1\n"
    "set cli.plugin_search=/b\n"
    "cmd app n=1 miss=[] subs=[show] order=[]\n"
    "cmd show n=1 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "T03 [--back.x=1 show]\n"
    "hook 1 [|--back.x=1|show] -> 1\n"
    "set back.x=1\n"
    "cmd app n=1 miss=[] subs=[show] order=[]\n"
    "cmd show n=1 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "T04 [show --set cli.plugin_search=/z]\n"
    "hook 2 [|show|--set|cli.plugin_search=/z] -> 2\n"
    "set cli.plugin_search=/z\n"
    "cmd app n=1 miss=[] subs=[show] order=[]\n"
    "cmd show n=1 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "T05 [show --greedy.x]\n"
    "hook 2 [|show|--greedy.x] -> 99\n"
    "ERR HorribleError 112: The setting hook took more arguments than were "
    "left: 99 of 1\n"
    "cmd app n=1 miss=[] subs=[show] order=[]\n"
    "cmd show n=1 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "T06 [show --set]\n"
    "hook 2 [|show|--set] -> -1 status=8\n"
    "ERR SettingError 2: --set\n"
    "cmd app n=1 miss=[] subs=[show] order=[]\n"
    "cmd show n=1 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "T07 [--neg.x show]\n"
    "hook 1 [|--neg.x|show] -> -5 status=5\n"
    "ERR SettingError 2: --neg.x\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd show n=0 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "T08 [--cli.plugin_search show]\n"
    "hook 1 [|--cli.plugin_search|show] -> -1 status=9\n"
    "ERR SettingError 2: --cli.plugin_search\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd show n=0 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n";

// REFUSALS
//   constant: what the steps of d_tests_arg_setting_refusals write.
static const char REFUSALS[] =
    "R01 main [probe --cli.nope=1 show]\n"
    "hook 1 [probe|--cli.nope=1|show] -> -1 status=7\n"
    "ERR SettingError 2: --cli.nope=1\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd show n=0 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "R01 kept=3 status=7\n"
    "R01 code=2 out=[] err=[Setting refused: --cli.nope=1\\nRun with --help "
    "for more information.\\n]\n"
    "R02 main [probe show --neg.x]\n"
    "hook 2 [probe|show|--neg.x] -> -5 status=5\n"
    "ERR SettingError 2: --neg.x\n"
    "cmd app n=1 miss=[] subs=[show] order=[]\n"
    "cmd show n=1 miss=[] subs=[] order=[]\n"
    "R02 kept=3 status=5\n"
    "R02 code=2 out=[] err=[ERROR: SettingError: --neg.x\\n\\nprog show "
    "[OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,     --help              Print this "
    "help message and exit\\n]\n"
    "R03 main [probe --cli.nope=1 show]\n"
    "hook 1 [probe|--cli.nope=1|show] -> -1 status=7\n"
    "ERR SettingError 2: --cli.nope=1\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd show n=0 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "R03 refused kept=3 status=7\n"
    "R03 main [probe --cli.plugin_search=/ok show]\n"
    "hook 1 [probe|--cli.plugin_search=/ok|show] -> 1\n"
    "set cli.plugin_search=/ok\n"
    "cmd app n=1 miss=[] subs=[show] order=[]\n"
    "cmd show n=1 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "R03 parsed kept=3 status=0\n"
    "R03 main [probe show --set nope=1]\n"
    "hook 2 [probe|show|--set|nope=1] -> -1 status=7\n"
    "ERR SettingError 2: --set\n"
    "cmd app n=1 miss=[] subs=[show] order=[]\n"
    "cmd show n=1 miss=[] subs=[] order=[]\n"
    "cmd pre n=0 miss=[] subs=[] order=[]\n"
    "opt level n=0 res=[]\n"
    "opt output n=0 res=[]\n"
    "opt ab n=0 res=[]\n"
    "opt mode n=0 res=[]\n"
    "opt word n=0 res=[]\n"
    "R03 refused kept=4 status=7\n"
    "R03 cleared kept=0 status=0\n";


//==============================================================================
// THE STAND-IN HOOK
//==============================================================================

/**
 * @brief Whether a setting's text -- what follows `--`, or `--set`'s
 *        argument -- binds: `NAME=VALUE` with NAME the setting known.
 *
 * @param[in]  _text    the text.
 * @param[out] _status  receives the reason, where it does not bind.
 * @return `true` when it binds.
 */
static
bool d_tests_arg_setting_binds(
    const char* _text,
    int32_t*    _status
)
{
    const char* const equals = strchr(_text,
                                      '=');

    // a name without a value
    if (!equals)
    {
        *_status = SETTING_UNVALUED;

        return false;
    }

    // the setting known, and no other
    if ( ((size_t)(equals - _text) != strlen(SETTING_KNOWN)) ||
         (strncmp(_text,
                  SETTING_KNOWN,
                  strlen(SETTING_KNOWN)) != 0) )
    {
        *_status = SETTING_UNKNOWN;

        return false;
    }

    return true;
}

/**
 * @brief Writes a call the hook got: where the offered argument is, the
 *        arguments, and what it returned.
 *
 * @param[in,out] _run     the run.
 * @param[in]     _argc    the argument count it was given.
 * @param[in]     _argv    the arguments.
 * @param[in]     _at      the offered argument's index, as given.
 * @param[in]     _return  what the hook returns.
 * @param[in]     _status  its reason, for a refusal.
 */
static
void d_tests_arg_setting_saw(
    struct dj_tests_arg_run* _run,
    int                      _argc,
    char* const*             _argv,
    int                      _at,
    int                      _return,
    int32_t                  _status
)
{
    (void)d_arg_buffer_printf(&_run->transcript,
                              "hook %d [",
                              _at);

    // each argument, as the parser gave it
    for (int index = 0; index < _argc; ++index)
    {
        if (index > 0)
        {
            (void)d_arg_buffer_append_char(&_run->transcript,
                                           '|');
        }

        const struct dj_pack_text each = { _argv[index],
                                           strlen(_argv[index]) };

        d_tests_arg_escape(&_run->transcript,
                           each);
    }

    (void)d_arg_buffer_printf(&_run->transcript,
                              "]%s -> %d",
                              (_argv[_argc] != NULL) ? " unended" : "",
                              _return);

    // the reason for a refusal
    if (_return < 0)
    {
        (void)d_arg_buffer_printf(&_run->transcript,
                                  " status=%ld",
                                  (long)_status);
    }

    (void)d_arg_buffer_append_char(&_run->transcript,
                                   '\n');

    return;
}

/**
 * @brief The stand-in for the settings kit's hook.
 *
 * @param[in]     _ctx     the run.
 * @param[in]     _argc    the parser's argument count.
 * @param[in]     _argv    the parser's arguments.
 * @param[in,out] _at      the offered argument's index; moved for `--put`
 *                         and `--back.NAME=VALUE`.
 * @param[out]    _status  the reason for a refusal.
 * @return how many arguments it took, 0, or a negative count.
 *
 * `--set NAME=VALUE` takes two, counted; `--put NAME=VALUE` takes two by
 * moving `*_at` onto the second and returning 1, as the option registry's
 * take_option does; `--NAME=VALUE`, NAME dotted, takes one. A dotted name
 * it does not know, or one without a value, is refused, as is a `--set` or
 * `--put` without a `NAME=VALUE`. For the cases of what it takes:
 * `--greedy.NAME` claims 99, `--back.NAME=VALUE` moves `*_at` before itself
 * and returns 1, and `--neg.NAME` refuses with -5. Anything else is not its
 * own.
 */
static
int d_tests_arg_setting_kit(
    void*        _ctx,
    int          _argc,
    char* const* _argv,
    int*         _at,
    int32_t*     _status
)
{
    struct dj_tests_arg_run* const run    = (struct dj_tests_arg_run*)_ctx;
    const int                     at     = *_at;
    const char* const             given  = _argv[at];
    const char*                   bound  = NULL;
    int                           result = 0;

    // two arguments, counted or pointed at
    if ( (strcmp(given, "--set") == 0) ||
         (strcmp(given, "--put") == 0) )
    {
        const bool paired = ( (at + 1 < _argc) &&
                              (strchr(_argv[at + 1], '=') != NULL) );

        if (!paired)
        {
            *_status = SETTING_UNPAIRED;
            result   = -1;
        }
        else if (!d_tests_arg_setting_binds(_argv[at + 1],
                                            _status))
        {
            result = -1;
        }
        else
        {
            bound  = _argv[at + 1];
            result = (given[2] == 's') ? 2 : 1;
            *_at   = (given[2] == 's') ? at : at + 1;
        }
    }
    // more than there is
    else if (strncmp(given, "--greedy.", 9u) == 0)
    {
        result = 99;
    }
    // pointed before itself
    else if (strncmp(given, "--back.", 7u) == 0)
    {
        bound  = given + 2;
        result = 1;
        *_at   = 0;
    }
    // another negative count
    else if (strncmp(given, "--neg.", 6u) == 0)
    {
        *_status = SETTING_OTHER;
        result   = -5;
    }
    // a dotted setting
    else if ( (strncmp(given, "--", 2u) == 0) &&
              (memchr(given + 2,
                      '.',
                      strcspn(given + 2, "=")) != NULL) )
    {
        if (d_tests_arg_setting_binds(given + 2,
                                      _status))
        {
            bound  = given + 2;
            result = 1;
        }
        else
        {
            result = -1;
        }
    }

    d_tests_arg_setting_saw(run,
                            _argc,
                            _argv,
                            at,
                            result,
                            *_status);

    // the setting bound
    if (bound != NULL)
    {
        d_tests_arg_note(run,
                         "set %s",
                         bound);
    }

    return result;
}


//==============================================================================
// DEFINITIONS
//==============================================================================

/**
 * @brief The probe's setup: response files, abbreviations, names folded in
 *        case and underscore, non-standard short names and subcommand
 *        prefixes on; `--level`, `-o,--output` and `-ab` at the root,
 *        which needs a subcommand; `show`, with `--mode` and a word; and
 *        `pre`, a prefix command; the stand-in hook set.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static
void d_tests_arg_setting_probe_app(
    struct dj_tests_arg_run* _run,
    struct dj_arg_command*   _app
)
{
    struct dj_arg_parser* const parser = &_run->parser;

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)
    d_arg_parser_set_response_files(parser,
                                    true,
                                    (int)D_ARG_QUOTING_GNU);
#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

    d_arg_parser_set_abbreviations(parser,
                                   true);
    d_arg_parser_set_setting_hook(parser,
                                  d_tests_arg_setting_kit,
                                  _run);
    d_arg_command_set_non_standard_names(_app,
                                         true);
    (void)d_arg_command_set_ignore_case(_app,
                                        true);
    (void)d_arg_command_set_ignore_underscore(_app,
                                              true);
    d_arg_command_set_prefix_matching(_app,
                                      true);
    d_arg_command_require_subcommand(_app,
                                     1u,
                                     0u);
    (void)d_tests_arg_make_option(_run,
                                  "level",
                                  _app,
                                  "--level");
    (void)d_tests_arg_make_option(_run,
                                  "output",
                                  _app,
                                  "-o,--output");
    (void)d_tests_arg_make_option(_run,
                                  "ab",
                                  _app,
                                  "-ab");

    struct dj_arg_command* const show = d_tests_arg_make_subcommand(_run,
                                                                   _app,
                                                                   "show");

    (void)d_tests_arg_make_option(_run,
                                  "mode",
                                  show,
                                  "--mode");
    (void)d_tests_arg_make_option(_run,
                                  "word",
                                  show,
                                  "word");

    struct dj_arg_command* const pre = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "pre");

    d_arg_command_set_prefix_command(pre,
                                     (int)D_ARG_PREFIX_ON);

    return;
}

/**
 * @brief The probe's setup with a root `--settings FILE`, which `--set`
 *        abbreviates.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static
void d_tests_arg_setting_probe_settings(
    struct dj_tests_arg_run* _run,
    struct dj_arg_command*   _app
)
{
    d_tests_arg_setting_probe_app(_run,
                                  _app);
    (void)d_tests_arg_make_option(_run,
                                  "settings",
                                  _app,
                                  "--settings");

    return;
}

/**
 * @brief Where the search reaches: `--level`, which folds case and
 *        underscores, `-o,--output` and `--many`, taking one value or
 *        more, at the root, and `--color` in its option group `grp`;
 *        `show`, which falls through, with `--mode` and a word; `list`,
 *        which does not, with a word; `pre`, a prefix command;
 *        abbreviations on, and the stand-in hook set.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static
void d_tests_arg_setting_reach_app(
    struct dj_tests_arg_run* _run,
    struct dj_arg_command*   _app
)
{
    d_arg_parser_set_abbreviations(&_run->parser,
                                   true);
    d_arg_parser_set_setting_hook(&_run->parser,
                                  d_tests_arg_setting_kit,
                                  _run);

    struct dj_arg_option* const level = d_tests_arg_make_option(_run,
                                                               "level",
                                                               _app,
                                                               "--level");

    if (level != NULL)
    {
        (void)d_arg_option_set_ignore_case(level,
                                           true);
        (void)d_arg_option_set_ignore_underscore(level,
                                                 true);
    }

    (void)d_tests_arg_make_option(_run,
                                  "output",
                                  _app,
                                  "-o,--output");

    struct dj_arg_option* const many = d_tests_arg_make_all(_run,
                                                           "many",
                                                           _app,
                                                           "--many");

    if (many != NULL)
    {
        d_arg_option_set_expected_range(many,
                                        1,
                                        -1);
    }

    struct dj_arg_command* const group = d_arg_command_add_option_group(_app,
                                                                       "grp",
                                                                       NULL);

    (void)d_tests_arg_make_option(_run,
                                  "color",
                                  group,
                                  "--color");

    struct dj_arg_command* const show = d_tests_arg_make_subcommand(_run,
                                                                   _app,
                                                                   "show");

    d_arg_command_set_fallthrough(show,
                                  true);
    (void)d_tests_arg_make_option(_run,
                                  "mode",
                                  show,
                                  "--mode");
    (void)d_tests_arg_make_option(_run,
                                  "word",
                                  show,
                                  "word");

    struct dj_arg_command* const list = d_tests_arg_make_subcommand(_run,
                                                                   _app,
                                                                   "list");

    (void)d_tests_arg_make_option(_run,
                                  "item",
                                  list,
                                  "item");

    struct dj_arg_command* const pre = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "pre");

    d_arg_command_set_prefix_command(pre,
                                     (int)D_ARG_PREFIX_ON);

    return;
}

/**
 * @brief A root that is itself a prefix command, with `show`, which falls
 *        through to it, and `list`, which does not, neither a prefix
 *        command, as each would be by inheriting the root's mode; the
 *        stand-in hook set.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static
void d_tests_arg_setting_keeper_app(
    struct dj_tests_arg_run* _run,
    struct dj_arg_command*   _app
)
{
    d_arg_parser_set_setting_hook(&_run->parser,
                                  d_tests_arg_setting_kit,
                                  _run);
    d_arg_command_set_prefix_command(_app,
                                     (int)D_ARG_PREFIX_ON);

    struct dj_arg_command* const show = d_tests_arg_make_subcommand(_run,
                                                                   _app,
                                                                   "show");

    d_arg_command_set_fallthrough(show,
                                  true);
    d_arg_command_set_prefix_command(show,
                                     (int)D_ARG_PREFIX_OFF);

    struct dj_arg_command* const list = d_tests_arg_make_subcommand(_run,
                                                                   _app,
                                                                   "list");

    d_arg_command_set_prefix_command(list,
                                     (int)D_ARG_PREFIX_OFF);

    return;
}

/**
 * @brief A root that keeps nothing, with `show`, a prefix command that
 *        falls through to it, so that an option it does not know is the
 *        root's to report, not its own to keep; the stand-in hook set.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static
void d_tests_arg_setting_through_app(
    struct dj_tests_arg_run* _run,
    struct dj_arg_command*   _app
)
{
    d_arg_parser_set_setting_hook(&_run->parser,
                                  d_tests_arg_setting_kit,
                                  _run);

    struct dj_arg_command* const show = d_tests_arg_make_subcommand(_run,
                                                                   _app,
                                                                   "show");

    d_arg_command_set_prefix_command(show,
                                     (int)D_ARG_PREFIX_ON);
    d_arg_command_set_fallthrough(show,
                                  true);

    return;
}

/**
 * @brief A root that stops only at `--` (D_ARG_PREFIX_SEPARATOR_ONLY), with
 *        the stand-in hook set.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static
void d_tests_arg_setting_separator_app(
    struct dj_tests_arg_run* _run,
    struct dj_arg_command*   _app
)
{
    d_arg_parser_set_setting_hook(&_run->parser,
                                  d_tests_arg_setting_kit,
                                  _run);
    d_arg_command_set_prefix_command(_app,
                                     (int)D_ARG_PREFIX_SEPARATOR_ONLY);

    return;
}

/**
 * @brief A root that stops at its first positional
 *        (D_ARG_PREFIX_POSITIONAL_ONLY), with the stand-in hook set.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static
void d_tests_arg_setting_positional_app(
    struct dj_tests_arg_run* _run,
    struct dj_arg_command*   _app
)
{
    d_arg_parser_set_setting_hook(&_run->parser,
                                  d_tests_arg_setting_kit,
                                  _run);
    d_arg_command_set_prefix_command(_app,
                                     (int)D_ARG_PREFIX_POSITIONAL_ONLY);

    return;
}

/**
 * @brief A root that takes Windows-style options, with `/level` and the
 *        stand-in hook set.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static
void d_tests_arg_setting_windows_app(
    struct dj_tests_arg_run* _run,
    struct dj_arg_command*   _app
)
{
    d_arg_parser_set_setting_hook(&_run->parser,
                                  d_tests_arg_setting_kit,
                                  _run);
    d_arg_command_set_windows_style(_app,
                                    true);
    (void)d_tests_arg_make_option(_run,
                                  "level",
                                  _app,
                                  "--level");

    return;
}

#if (D_INTERNAL_ARG_PARSER_SUGGESTIONS == 1)

/**
 * @brief The reach's setup with the parser's suggestions on, so that an
 *        argument the hook leaves is reported as it would be without one.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static
void d_tests_arg_setting_suggest_app(
    struct dj_tests_arg_run* _run,
    struct dj_arg_command*   _app
)
{
    d_tests_arg_setting_reach_app(_run,
                                  _app);
    d_arg_parser_set_suggestions(&_run->parser,
                                 true);

    return;
}

#endif  // D_INTERNAL_ARG_PARSER_SUGGESTIONS

/**
 * @brief The probe's options and commands without a hook, for the steps
 *        that set their own.
 *
 * @param[in,out] _run  the step's run.
 * @param[in,out] _app  the run's root.
 */
static
void d_tests_arg_setting_plain_app(
    struct dj_tests_arg_run* _run,
    struct dj_arg_command*   _app
)
{
    d_tests_arg_setting_probe_app(_run,
                                  _app);
    d_arg_parser_set_setting_hook(&_run->parser,
                                  NULL,
                                  NULL);

    return;
}


//==============================================================================
// STEPS' HELPERS
//==============================================================================

/**
 * @brief Parses main's arguments, built in storage the run keeps, and adds
 *        the report: a line naming the step, the hook's calls, then the
 *        parse's lines.
 *
 * @param[in,out] _run      the run.
 * @param[in]     _tag      the step's tag.
 * @param[in]     _program  the program's name, `argv[0]`.
 * @param[in]     _words    the arguments after it, separated by spaces.
 * @return the parse's status.
 */
static
int d_tests_arg_setting_main(
    struct dj_tests_arg_run* _run,
    const char*              _tag,
    const char*              _program,
    const char*              _words
)
{
    int                argc = 0;
    char* const* const argv = d_tests_arg_argv(_run,
                                               _program,
                                               _words,
                                               &argc);

    d_tests_arg_note(_run,
                     "%s main [%s %s]",
                     _tag,
                     _program,
                     _words);

    const int status = (argv != NULL)
                       ? d_arg_parser_parse(&_run->parser,
                                            argc,
                                            argv)
                       : (int)D_ARG_ERROR_BAD_ARGUMENT;

    d_tests_arg_record(_run,
                       status);

    return status;
}

/**
 * @brief Parses a line, with or without the program's name first, and adds
 *        the report: a line naming the step, the hook's calls, then the
 *        parse's lines.
 *
 * @param[in,out] _run      the run.
 * @param[in]     _tag      the step's tag.
 * @param[in]     _line     the line.
 * @param[in]     _program  whether the line starts with the program's name.
 * @return the parse's status.
 */
static
int d_tests_arg_setting_line(
    struct dj_tests_arg_run* _run,
    const char*              _tag,
    const char*              _line,
    bool                     _program
)
{
    struct dj_pack_text line = { _line, strlen(_line) };

    d_tests_arg_note(_run,
                     "%s line%s [%s]",
                     _tag,
                     _program ? " named" : "",
                     _line);

    const int status = d_arg_parser_parse_line(&_run->parser,
                                               line,
                                               _program);

    d_tests_arg_record(_run,
                       status);

    return status;
}

/**
 * @brief Adds what the parser keeps for the hook: how many arguments, and
 *        the reason the last refusal gave.
 *
 * @param[in,out] _run  the run.
 * @param[in]     _tag  the step's tag.
 */
static
void d_tests_arg_setting_kept(
    struct dj_tests_arg_run* _run,
    const char*              _tag
)
{
    d_tests_arg_note(_run,
                     "%s kept=%lu status=%ld",
                     _tag,
                     (unsigned long)_run->parser.setting_args.count,
                     (long)_run->parser.setting_status);

    return;
}

/**
 * @brief Ends the run's parse as CLI11's exit(error, out, err) on the root,
 *        into the run's buffers, and adds what it came to: the code, and
 *        what was written to each.
 *
 * @param[in,out] _run     the run.
 * @param[in]     _tag     the step's tag.
 * @param[in]     _status  the parse's status.
 */
static
void d_tests_arg_setting_exit(
    struct dj_tests_arg_run* _run,
    const char*              _tag,
    int                      _status
)
{
    const int code = d_arg_parser_exit_to(&_run->parser,
                                          _status,
                                          d_arg_buffer_sink(&_run->out),
                                          d_arg_buffer_sink(&_run->err));

    (void)d_arg_buffer_printf(&_run->transcript,
                              "%s code=%d out=[",
                              _tag,
                              code);
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


//==============================================================================
// 1.  THE PROBE'S CASES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// P01_FILES
//   constant: the response file P01 parses with.
static const struct dj_tests_arg_file P01_FILES[] =
{
    { "rf", "--cli.plugin_search=/rf\n" },
    { NULL, NULL }
};

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

// PROBE_CASES
//   constant: the cases of d_tests_arg_setting_probe: P01 to P04 the gaps
// in the kit's own walk, P05 to P09 what it got right, P10 the hook before
// an abbreviation.
static const struct dj_tests_arg_case PROBE_CASES[] =
{
#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)
    { "P01",
      d_tests_arg_setting_probe_app,
      { "@rf", "show" },
      NULL,
      P01_FILES },
#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES
    { "P02",
      d_tests_arg_setting_probe_app,
      { "--out", "--cli.plugin_search=/ab", "show" },
      NULL,
      NULL },
    { "P03",
      d_tests_arg_setting_probe_app,
      { "-ab", "--cli.plugin_search=/ns", "show" },
      NULL,
      NULL },
    { "P04",
      d_tests_arg_setting_probe_app,
      { "pre", "--mod.depth=3", "x" },
      NULL,
      NULL },
    { "P05",
      d_tests_arg_setting_probe_app,
      { "--output", "--cli.plugin_search=/ab", "show" },
      NULL,
      NULL },
    { "P06",
      d_tests_arg_setting_probe_app,
      { "sh", "--mode", "m", "--cli.plugin_search=/sh", "w" },
      NULL,
      NULL },
    { "P07",
      d_tests_arg_setting_probe_app,
      { "--set", "cli.plugin_search=/s", "show" },
      NULL,
      NULL },
    { "P08",
      d_tests_arg_setting_probe_app,
      { "show", "--", "--cli.plugin_search=/x" },
      NULL,
      NULL },
    { "P09",
      d_tests_arg_setting_probe_app,
      { "--cli.nope=1", "show" },
      NULL,
      NULL },
    { "P10",
      d_tests_arg_setting_probe_settings,
      { "--set", "cli.plugin_search=/s", "show" },
      NULL,
      NULL }
};

// PROBE_COUNT
//   constant: how many cases PROBE_CASES holds.
static const size_t PROBE_COUNT =
    sizeof(PROBE_CASES) / sizeof(PROBE_CASES[0]);

/*
d_tests_arg_setting_probe
  The probe's ten cases, on its setup.
  Tests the following:
  - a setting in a response file, offered as if typed
  - an abbreviated option offered first and declined, then given its value;
    a value of a multi-letter short option, never offered
  - a prefix command's arguments, never offered
  - a known option's value; a setting after a subcommand's prefix, with its
    absolute index; `--set` taking two; a positional after `--`
  - a refusal: SettingError, exit code 2, the argument as typed
  - `--set` offered before it abbreviates `--settings`
*/
bool d_tests_arg_setting_probe(void)
{
    return d_tests_arg_cases("PROBE",
                             PROBE_CASES,
                             PROBE_COUNT,
                             PROBE);
}

//==============================================================================
// 2.  WHEN IT IS OFFERED
//==============================================================================

// OFFERED_CASES
//   constant: the cases of d_tests_arg_setting_offered.
static const struct dj_tests_arg_case OFFERED_CASES[] =
{
    { "N01",
      d_tests_arg_setting_reach_app,
      { "--LE_VEL", "3", "show" },
      NULL,
      NULL },
    { "N02",
      d_tests_arg_setting_reach_app,
      { "--color", "red" },
      NULL,
      NULL },
    { "N03",
      d_tests_arg_setting_reach_app,
      { "show", "--level", "3" },
      NULL,
      NULL },
    { "N04",
      d_tests_arg_setting_reach_app,
      { "list", "--level", "3" },
      NULL,
      NULL },
    { "N05",
      d_tests_arg_setting_reach_app,
      { "--show.mode=m" },
      NULL,
      NULL },
    { "N06",
      d_tests_arg_setting_reach_app,
      { "--show.nope=1" },
      NULL,
      NULL },
    { "N07",
      d_tests_arg_setting_reach_app,
      { "--lev", "3" },
      NULL,
      NULL },
#if (D_INTERNAL_ARG_PARSER_SUGGESTIONS == 1)
    { "N08",
      d_tests_arg_setting_suggest_app,
      { "--levl", "3" },
      NULL,
      NULL }
#endif  // D_INTERNAL_ARG_PARSER_SUGGESTIONS
};

// OFFERED_COUNT
//   constant: how many cases OFFERED_CASES holds.
static const size_t OFFERED_COUNT =
    sizeof(OFFERED_CASES) / sizeof(OFFERED_CASES[0]);

/*
d_tests_arg_setting_offered
  Which long options the hook is offered, by the names the search reaches.
  Tests the following:
  - never an option the search reaches by name: the command's own, folded
    in case and underscores; its option group's; its fallthrough parent's
  - an option the search does not reach, from a subcommand that does not
    fall through; offered, declined, and reported as before
  - never one CLI11's dot notation gives a subcommand's option; one it does
    not give, offered
  - an abbreviation offered before it is expanded
  - a suggestion for an option declined, as without a hook
*/
bool d_tests_arg_setting_offered(void)
{
    return d_tests_arg_cases("OFFERED",
                             OFFERED_CASES,
                             OFFERED_COUNT,
                             OFFERED);
}

// OFFERED_PLACE_CASES
//   constant: the cases of d_tests_arg_setting_offered_place.
static const struct dj_tests_arg_case OFFERED_PLACE_CASES[] =
{
    { "W01",
      d_tests_arg_setting_reach_app,
      { "--", "--cli.plugin_search=/t" },
      NULL,
      NULL },
    { "W02",
      d_tests_arg_setting_reach_app,
      { "show", "w", "--", "--cli.plugin_search=/t" },
      NULL,
      NULL },
    { "W03",
      d_tests_arg_setting_reach_app,
      { "pre", "--cli.plugin_search=/p", "x" },
      NULL,
      NULL },
    { "W04",
      d_tests_arg_setting_keeper_app,
      { "show", "--cli.plugin_search=/k", "x" },
      NULL,
      NULL },
    { "W05",
      d_tests_arg_setting_keeper_app,
      { "list", "--cli.plugin_search=/k", "x" },
      NULL,
      NULL },
    { "W06",
      d_tests_arg_setting_through_app,
      { "show", "--cli.plugin_search=/f", "x" },
      NULL,
      NULL },
    { "W07",
      d_tests_arg_setting_separator_app,
      { "--cli.plugin_search=/s1", "--", "--cli.plugin_search=/s2" },
      NULL,
      NULL },
    { "W08",
      d_tests_arg_setting_positional_app,
      { "--cli.plugin_search=/p1", "word", "--cli.plugin_search=/p2" },
      NULL,
      NULL },
    { "W09",
      d_tests_arg_setting_reach_app,
      { "--many", "a", "--cli.plugin_search=/v", "b" },
      NULL,
      NULL },
    { "W10",
      d_tests_arg_setting_windows_app,
      { "/cli.plugin_search:/w", "/level:2" },
      NULL,
      NULL },
    { "W11",
      d_tests_arg_setting_reach_app,
      { "-o", "--cli.plugin_search=/o", "--level=--cli.plugin_search=/e" },
      NULL,
      NULL }
};

// OFFERED_PLACE_COUNT
//   constant: how many cases OFFERED_PLACE_CASES holds.
static const size_t OFFERED_PLACE_COUNT =
    sizeof(OFFERED_PLACE_CASES) / sizeof(OFFERED_PLACE_CASES[0]);

/*
d_tests_arg_setting_offered_place
  Which long options the hook is offered, by where they stand.
  Tests the following:
  - never after a `--` that ends the options; after a `--` that ends a
    subcommand whose positionals are full, offered to the parent's parse
  - never one a prefix command keeps, nor one that falls through to one; one
    that ends in a command that keeps nothing, offered, a prefix command
    that falls through to one included
  - a command that stops only at `--`, and one that stops at its first
    positional: offered before, never after
  - an argument that ends an option's values, offered; a value, never
  - a Windows-style option, never
*/
bool d_tests_arg_setting_offered_place(void)
{
    return d_tests_arg_cases("OFFERED_PLACE",
                             OFFERED_PLACE_CASES,
                             OFFERED_PLACE_COUNT,
                             OFFERED_PLACE);
}

//==============================================================================
// 3.  WHAT IT SEES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// S01_FILES
//   constant: the response files S01 parses with, one inside the other.
static const struct dj_tests_arg_file S01_FILES[] =
{
    { "a.rsp", "--level 2 @b.rsp\n" },
    { "b.rsp", "--cli.plugin_search=/b\n" },
    { NULL,    NULL }
};

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

// SEES_CASES
//   constant: the cases of d_tests_arg_setting_sees.
static const struct dj_tests_arg_case SEES_CASES[] =
{
#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)
    { "S01",
      d_tests_arg_setting_probe_app,
      { "@a.rsp", "show", "--cli.plugin_search=/c" },
      NULL,
      S01_FILES },
#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES
    { "S02",
      d_tests_arg_setting_probe_app,
      { "show", "--mode", "m", "--cli.plugin_search=/x", "w" },
      NULL,
      NULL },
    { "S03",
      d_tests_arg_setting_probe_app,
      { "show", "--cli.plugin_search=/1", "--cli.plugin_search=/2" },
      NULL,
      NULL }
};

// SEES_COUNT
//   constant: how many cases SEES_CASES holds.
static const size_t SEES_COUNT =
    sizeof(SEES_CASES) / sizeof(SEES_CASES[0]);

/**
 * @brief S04: main's arguments, the program's name `argv[0]`.
 *
 * @param[in,out] _run  the step's run.
 * @param[in,out] _app  the run's root.
 */
static
void d_tests_arg_setting_s04(
    struct dj_tests_arg_run* _run,
    struct dj_arg_command*   _app
)
{
    d_tests_arg_setting_probe_app(_run,
                                  _app);
    (void)d_tests_arg_setting_main(_run,
                                   "S04",
                                   "probe",
                                   "show --cli.plugin_search=/m");

    return;
}

/**
 * @brief S05: a line with the program's name first, and one without.
 *
 * @param[in,out] _run  the step's run.
 * @param[in,out] _app  the run's root.
 */
static
void d_tests_arg_setting_s05(
    struct dj_tests_arg_run* _run,
    struct dj_arg_command*   _app
)
{
    d_tests_arg_setting_probe_app(_run,
                                  _app);
    (void)d_tests_arg_setting_line(_run,
                                   "S05",
                                   "probe show --cli.plugin_search=/l",
                                   true);
    (void)d_tests_arg_setting_line(_run,
                                   "S05",
                                   "show --cli.plugin_search=/n",
                                   false);

    return;
}

/**
 * @brief S06: what the parser keeps for the hook: the arguments with a hook
 *        set, none without, and none once cleared.
 *
 * @param[in,out] _run  the step's run.
 * @param[in,out] _app  the run's root.
 */
static
void d_tests_arg_setting_s06(
    struct dj_tests_arg_run* _run,
    struct dj_arg_command*   _app
)
{
    d_tests_arg_setting_plain_app(_run,
                                  _app);
    (void)d_tests_arg_setting_main(_run,
                                   "S06",
                                   "probe",
                                   "show --level 2");
    d_tests_arg_setting_kept(_run,
                             "S06 no hook");
    d_arg_parser_set_setting_hook(&_run->parser,
                                  d_tests_arg_setting_kit,
                                  _run);
    (void)d_tests_arg_setting_main(_run,
                                   "S06",
                                   "probe",
                                   "show --level 2");
    d_tests_arg_setting_kept(_run,
                             "S06 hook");
    d_arg_parser_clear(&_run->parser);
    d_tests_arg_setting_kept(_run,
                             "S06 cleared");

    return;
}

// SEES_STEP_TABLE
//   constant: the steps of d_tests_arg_setting_sees.
static const dj_tests_arg_define_fn SEES_STEP_TABLE[] =
{
    d_tests_arg_setting_s04,
    d_tests_arg_setting_s05,
    d_tests_arg_setting_s06
};

// SEES_STEP_COUNT
//   constant: how many steps SEES_STEP_TABLE holds.
static const size_t SEES_STEP_COUNT =
    sizeof(SEES_STEP_TABLE) / sizeof(SEES_STEP_TABLE[0]);

/*
d_tests_arg_setting_sees
  The arguments the hook sees, and where the offered one sits.
  Tests the following:
  - response files inside one another, expanded in place
  - a subcommand's option, at its index in the whole list
  - two settings in a row, each at its own index
  - the program's name first: main's `argv[0]`, a line's first word, and ""
    for a line without one; NULL after the last
  - the arguments kept only while a hook is set, and forgotten on clear
*/
bool d_tests_arg_setting_sees(void)
{
    bool held = d_tests_arg_cases("SEES",
                                  SEES_CASES,
                                  SEES_COUNT,
                                  SEES);

    held = d_tests_arg_steps("SEES_STEPS",
                             SEES_STEP_TABLE,
                             SEES_STEP_COUNT,
                             SEES_STEPS) && held;

    return held;
}

//==============================================================================
// 4.  WHAT IT TAKES
//==============================================================================

// TAKES_CASES
//   constant: the cases of d_tests_arg_setting_takes.
static const struct dj_tests_arg_case TAKES_CASES[] =
{
    { "T01",
      d_tests_arg_setting_probe_app,
      { "--set", "cli.plugin_search=/a", "show" },
      NULL,
      NULL },
    { "T02",
      d_tests_arg_setting_probe_app,
      { "--put", "cli.plugin_search=/b", "show" },
      NULL,
      NULL },
    { "T03",
      d_tests_arg_setting_probe_app,
      { "--back.x=1", "show" },
      NULL,
      NULL },
    { "T04",
      d_tests_arg_setting_probe_app,
      { "show", "--set", "cli.plugin_search=/z" },
      NULL,
      NULL },
    { "T05",
      d_tests_arg_setting_probe_app,
      { "show", "--greedy.x" },
      NULL,
      NULL },
    { "T06",
      d_tests_arg_setting_probe_app,
      { "show", "--set" },
      NULL,
      NULL },
    { "T07",
      d_tests_arg_setting_probe_app,
      { "--neg.x", "show" },
      NULL,
      NULL },
    { "T08",
      d_tests_arg_setting_probe_app,
      { "--cli.plugin_search", "show" },
      NULL,
      NULL }
};

// TAKES_COUNT
//   constant: how many cases TAKES_CASES holds.
static const size_t TAKES_COUNT =
    sizeof(TAKES_CASES) / sizeof(TAKES_CASES[0]);

/*
d_tests_arg_setting_takes
  What the hook takes, by its count and by where it leaves `*_at`.
  Tests the following:
  - two arguments counted; two pointed at, the count 1; `*_at` moved before
    the offered one, the count taken
  - the last arguments taken, the parse ending after them
  - more taken than there are: HorribleError
  - refusals: a `--set` without its pair, another negative count, a dotted
    name without a value
*/
bool d_tests_arg_setting_takes(void)
{
    return d_tests_arg_cases("TAKES",
                             TAKES_CASES,
                             TAKES_COUNT,
                             TAKES);
}

//==============================================================================
// 5.  REFUSALS
//==============================================================================

/**
 * @brief R01: a refusal at the root, ended as CLI11's exit() ends a parse:
 *         the status, the reason kept, the message, exit code 2, and
 *         d_arg_failure_simple's text.
 *
 * @param[in,out] _run  the step's run.
 * @param[in,out] _app  the run's root.
 */
static
void d_tests_arg_setting_r01(
    struct dj_tests_arg_run* _run,
    struct dj_arg_command*   _app
)
{
    d_tests_arg_setting_probe_app(_run,
                                  _app);

    const int status = d_tests_arg_setting_main(_run,
                                                "R01",
                                                "probe",
                                                "--cli.nope=1 show");

    d_tests_arg_setting_kept(_run,
                             "R01");
    d_tests_arg_setting_exit(_run,
                             "R01",
                             status);

    return;
}

/**
 * @brief R02: a refusal in a subcommand, with d_arg_failure_help: the
 *         error's name and the argument, then the subcommand's help.
 *
 * @param[in,out] _run  the step's run.
 * @param[in,out] _app  the run's root.
 */
static
void d_tests_arg_setting_r02(
    struct dj_tests_arg_run* _run,
    struct dj_arg_command*   _app
)
{
    d_arg_parser_set_setting_hook(&_run->parser,
                                  d_tests_arg_setting_kit,
                                  _run);
    d_arg_command_set_failure_message(_app,
                                      d_arg_failure_help,
                                      NULL,
                                      NULL);
    (void)d_tests_arg_make_subcommand(_run,
                                      _app,
                                      "show");

    const int status = d_tests_arg_setting_main(_run,
                                                "R02",
                                                "probe",
                                                "show --neg.x");

    d_tests_arg_setting_kept(_run,
                             "R02");
    d_tests_arg_setting_exit(_run,
                             "R02",
                             status);

    return;
}

/**
 * @brief R03: the reason forgotten by the next parse, and by a clear.
 *
 * @param[in,out] _run  the step's run.
 * @param[in,out] _app  the run's root.
 */
static
void d_tests_arg_setting_r03(
    struct dj_tests_arg_run* _run,
    struct dj_arg_command*   _app
)
{
    d_tests_arg_setting_probe_app(_run,
                                  _app);
    (void)d_tests_arg_setting_main(_run,
                                   "R03",
                                   "probe",
                                   "--cli.nope=1 show");
    d_tests_arg_setting_kept(_run,
                             "R03 refused");
    (void)d_tests_arg_setting_main(_run,
                                   "R03",
                                   "probe",
                                   "--cli.plugin_search=/ok show");
    d_tests_arg_setting_kept(_run,
                             "R03 parsed");
    (void)d_tests_arg_setting_main(_run,
                                   "R03",
                                   "probe",
                                   "show --set nope=1");
    d_tests_arg_setting_kept(_run,
                             "R03 refused");
    d_arg_parser_clear(&_run->parser);
    d_tests_arg_setting_kept(_run,
                             "R03 cleared");

    return;
}

// REFUSAL_STEPS
//   constant: the steps of d_tests_arg_setting_refusals.
static const dj_tests_arg_define_fn REFUSAL_STEPS[] =
{
    d_tests_arg_setting_r01,
    d_tests_arg_setting_r02,
    d_tests_arg_setting_r03
};

// REFUSAL_COUNT
//   constant: how many steps REFUSAL_STEPS holds.
static const size_t REFUSAL_COUNT =
    sizeof(REFUSAL_STEPS) / sizeof(REFUSAL_STEPS[0]);

/*
d_tests_arg_setting_refusals
  A refusal, as the parse reports it and the program ends.
  Tests the following:
  - SettingError, exit code 2, the argument as typed as the message, the
    hook's reason kept
  - d_arg_failure_simple's "Setting refused: " before the argument;
    d_arg_failure_help's error name, and the subcommand's help
  - the reason forgotten by the next parse and by a clear
*/
bool d_tests_arg_setting_refusals(void)
{
    return d_tests_arg_steps("REFUSALS",
                             REFUSAL_STEPS,
                             REFUSAL_COUNT,
                             REFUSALS);
}

//==============================================================================
// 6.  THE SUITE
//==============================================================================

// d_tests_arg_setting
//   constant: the suite, in the header's order.
const struct dj_tests_section d_tests_arg_setting[] =
{
    { "arg_setting_probe",
      d_tests_arg_setting_probe },
    { "arg_setting_offered",
      d_tests_arg_setting_offered },
    { "arg_setting_offered_place",
      d_tests_arg_setting_offered_place },
    { "arg_setting_sees",
      d_tests_arg_setting_sees },
    { "arg_setting_takes",
      d_tests_arg_setting_takes },
    { "arg_setting_refusals",
      d_tests_arg_setting_refusals }
};

// d_tests_arg_setting_count
//   constant: how many sections the table above holds.
const size_t d_tests_arg_setting_count =
    sizeof(d_tests_arg_setting) / sizeof(d_tests_arg_setting[0]);
