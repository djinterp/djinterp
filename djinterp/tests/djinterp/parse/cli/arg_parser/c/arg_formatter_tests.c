/*******************************************************************************
* djinterp [parse]                                         arg_formatter_tests.c
*
* The suite of help, declared in arg_formatter_tests.h.
*   Each section of help's is a table of steps (arg_tests_support.h): each
* case makes a root as the help probe's App does -- its description and
* name set on a run's parser of its own -- with its options, subcommands and
* groups, the variables options are bound to and any formatter of its own
* kept by the run, and writes each help as the reference writes it:
* `=== <tag>`, the help with each line end written `$`, so that trailing
* spaces show, and `|end`. The cases keep the help probe's tags
* (H01-H33), and a case that writes several helps is named by the first.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_formatter_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.10.14
*******************************************************************************/
#include "./arg_formatter_tests.h"  // corresponding header
// std
#include <math.h>     // NAN
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <stdio.h>    // printf
#include <stdlib.h>   // getenv
#include <string.h>   // memchr, memcmp, strcmp, strlen, strstr
// djinterp
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text, d_pack_sink
#include "../../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // the knobs
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // commands, their settings
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // statuses, buffers, help modes
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_formatter.h"  // the formatter, its steps, help
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // options, their settings
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // the parser, failure messages
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_type.h"  // d_arg_type_builtin
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator.h"  // CLI11's Range, PositiveNumber
#include "../../../c/tests_section.h"  // d_tests_section
#include "./arg_tests_support.h"  // runs, steps, kept storage
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uintmax_t


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// BASICS
//   constant: what the cases of d_tests_arg_formatter_basics come to, as the
// reference writes them with CLI11.
static const char BASICS[] =
    "=== H01\n"
    "My prog$\n"
    "$\n"
    "$\n"
    "Usage: [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "|end\n"
    "=== H02\n"
    "My prog$\n"
    "$\n"
    "$\n"
    "program [OPTIONS] [something]$\n"
    "$\n"
    "$\n"
    "POSITIONALS:$\n"
    "  something TEXT              My option here$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "  -q,     --quick             Quickly$\n"
    "|end\n"
    "=== H03\n"
    "Types$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS] file [pair(2x)] [rest...]$\n"
    "$\n"
    "$\n"
    "POSITIONALS:$\n"
    "  file TEXT REQUIRED          The file$\n"
    "  pair INT x 2                Two of them$\n"
    "  rest TEXT ...               The rest$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "  -i,     --int INT           An integer$\n"
    "  -d,     --double FLOAT [2.5]  $\n"
    "                              A double$\n"
    "          --str TEXT REQUIRED A string$\n"
    "  -v,     --vals INT ...      Values$\n"
    "          --two INT x 2       Two values$\n"
    "          --some INT x 2      Two to four$\n"
    "|end\n"
    "=== H04\n"
    "Flags$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --flag, --no-flag{false} $\n"
    "                              A flag with a negation$\n"
    "  -a{5},  --all{7}            Flag with values$\n"
    "  -O{false}, --opt2, --no_opt2{false} $\n"
    "                              Something else$\n"
    "  -v,     --verbose [0]       Count$\n"
    "          --env-flag (Env:MY_FLAG) $\n"
    "                              env$\n"
    "|end\n"
    "=== H04b\n"
    "Flags$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --flag, --no-flag   A flag with a negation$\n"
    "  -a,     --all               Flag with values$\n"
    "  -O,     --opt2, --no_opt2   Something else$\n"
    "  -v,     --verbose [0]       Count$\n"
    "          --env-flag (Env:MY_FLAG) $\n"
    "                              env$\n"
    "|end\n"
    "=== H05\n"
    "Links$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "  -b,     --bravo Excludes: --delta $\n"
    "                              Bravo$\n"
    "  -c                          Charlie$\n"
    "          --zulu Excludes: --delta $\n"
    "                              Zulu$\n"
    "  -a,     --alpha TEXT (Env:ALPHA_ENV) Needs: --bravo --zulu -c $\n"
    "                              Alpha$\n"
    "          --delta Excludes: --bravo --zulu $\n"
    "                              Delta$\n"
    "|end\n";

// OPTION_TEXT
//   constant: what the cases of d_tests_arg_formatter_option_text come to, as
// the reference writes them with CLI11.
static const char OPTION_TEXT[] =
    "=== H06\n"
    "Text$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --opt (ARG)         Something$\n"
    "          --file FILE         File$\n"
    "          --range INT:INT in [1 - 10] $\n"
    "                              In range$\n"
    "          --r2 INT:INT in [1 - 10]:POSITIVE $\n"
    "                              Two checks$\n"
    "          --list TEXT,... ... Delimited$\n"
    "          --one TEXT;...      Delimited single$\n"
    "          --dflt TEXT [hello]  $\n"
    "                              With default$\n"
    "          --req TEXT [x y]  REQUIRED $\n"
    "                              Required with default$\n"
    "          --bare              No type at all$\n"
    "|end\n"
    "=== H06b\n"
    "Text$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --opt (ARG)         Something$\n"
    "          --file FILE         File$\n"
    "          --range INT:INT in [1 - 10] $\n"
    "                              In range$\n"
    "          --r2 INT:INT in [1 - 10]:POSITIVE $\n"
    "                              Two checks$\n"
    "          --list TEXT,... ... Delimited$\n"
    "          --one TEXT;...      Delimited single$\n"
    "          --dflt TEXT         With default$\n"
    "          --req TEXT REQUIRED Required with default$\n"
    "          --bare              No type at all$\n"
    "|end\n"
    "=== H06c\n"
    "Text$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --opt (ARG)         Something$\n"
    "          --file              File$\n"
    "          --range             In range$\n"
    "          --r2                Two checks$\n"
    "          --list ...          Delimited$\n"
    "          --one               Delimited single$\n"
    "          --dflt              With default$\n"
    "          --req REQUIRED      Required with default$\n"
    "          --bare              No type at all$\n"
    "|end\n"
    "=== H07\n"
    "Groups$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS] [pos] [bothpos]$\n"
    "$\n"
    "$\n"
    "POSITIONALS:$\n"
    "  pos TEXT                    Positional$\n"
    "  bothpos TEXT                Both kinds$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "  -b TEXT                     Both kinds$\n"
    "$\n"
    "Group A:$\n"
    "          --first             First$\n"
    "          --third             Third$\n"
    "$\n"
    "Group B:$\n"
    "          --second            Second$\n"
    "|end\n"
    "=== H26\n"
    "Sizes$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS] vpos... [wpos(3x)] [upos(1x)]$\n"
    "$\n"
    "$\n"
    "POSITIONALS:$\n"
    "  vpos INT ... REQUIRED       Vector positional$\n"
    "  wpos INT x 3                Three of them$\n"
    "  upos INT                    Up to three$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --pair [INT,INT]    A pair$\n"
    "          --pairs [INT,INT] ... $\n"
    "                              Pairs$\n"
    "|end\n"
    "=== H34\n"
    "Flag text$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --must REQUIRED     A required flag$\n"
    "          --dflag [x]         A flag with default text$\n"
    "          --zero              Of no size$\n"
    "|end\n";

// LABELS
//   constant: what the cases of d_tests_arg_formatter_labels come to, as the
// reference writes them with CLI11.
static const char LABELS[] =
    "=== H08\n"
    "Labels$\n"
    "$\n"
    "$\n"
    "prog [Opts] [pos] [COMMANDS]$\n"
    "$\n"
    "$\n"
    "ARGS:$\n"
    "  pos STRING                  Pos$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --name STRING (MUST) (Environment:NAME) $\n"
    "                              Name$\n"
    "          --r SMALL           range$\n"
    "  -x Conflicts: -w            X$\n"
    "  -y Requires: -x             Y$\n"
    "  -w Conflicts: -x            W$\n"
    "$\n"
    "SUBCOMMANDS:$\n"
    "  sub (MUST)                  A sub$\n"
    "|end\n"
    "=== H08b\n"
    "Labels$\n"
    "$\n"
    "$\n"
    "Run: [Opts] [pos] [COMMANDS]$\n"
    "$\n"
    "$\n"
    "ARGS:$\n"
    "  pos STRING                  Pos$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --name STRING (MUST) (Environment:NAME) $\n"
    "                              Name$\n"
    "          --r SMALL           range$\n"
    "  -x Conflicts: -w            X$\n"
    "  -y Requires: -x             Y$\n"
    "  -w Conflicts: -x            W$\n"
    "$\n"
    "SUBCOMMANDS:$\n"
    "  sub (MUST)                  A sub$\n"
    "|end\n";

// WIDTHS
//   constant: what the cases of d_tests_arg_formatter_widths come to, as the
// reference writes them with CLI11.
static const char WIDTHS[] =
    "=== H09\n"
    "Widths$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS] [a-long-positional-name] [q] [SUBCOMMANDS]$\n"
    "$\n"
    "$\n"
    "POSITIONALS:$\n"
    "  a-long-positional-name TEXT$\n"
    "                    positional description that is$\n"
    "                    long enough to wrap around$\n"
    "  q TEXT            a positional with a short name$\n"
    "                    and a description long enough$\n"
    "                    to wrap$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h, --help        Print this help message and$\n"
    "                    exit$\n"
    "  -a, --alpha       This is a long description$\n"
    "                    that will wrap across several$\n"
    "                    lines of the right column$\n"
    "      --a-very-long-option-name $\n"
    "                    Short desc$\n"
    "      --a-very-long-option-name-2 $\n"
    "                    This is a long description$\n"
    "                    that will wrap across several$\n"
    "                    lines$\n"
    "      --exactly-eighteen1 $\n"
    "                    Exactly at the edge$\n"
    "      --exactly-seventeen $\n"
    "                    One short of it$\n"
    "$\n"
    "SUBCOMMANDS:$\n"
    "  a-very-long-subcommand$\n"
    "                    subcommand description that is$\n"
    "                    long enough to wrap around the$\n"
    "                    column$\n"
    "  exactly-eighteen  edge$\n"
    "  s                 a short subcommand with a long$\n"
    "                    description that wraps across$\n"
    "                    lines$\n"
    "|end\n"
    "=== H25\n"
    "Tiny$\n"
    "widths$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS] [p] [SUBCOMMANDS]$\n"
    "$\n"
    "$\n"
    "POSITIONALS:$\n"
    "  p TEXT$\n"
    "     pos$\n"
    "     words$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h, --help $\n"
    "     Print$\n"
    "     this$\n"
    "     help$\n"
    "     message$\n"
    "     and$\n"
    "     exit$\n"
    "  -a, --alpha $\n"
    "     words$\n"
    "     in$\n"
    "     a$\n"
    "     column$\n"
    "$\n"
    "SUBCOMMANDS:$\n"
    "  s  sub$\n"
    "     words$\n"
    "|end\n"
    "=== H25b\n"
    "Zero$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h, --help $\n"
    "Print this help message and exit$\n"
    "  -a, --alpha $\n"
    "words$\n"
    "|end\n"
    "=== H29\n"
    "Aliased group$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "[Option Group: Aliased]$\n"
    "  desc$\n"
    "  $\n"
    "                       aliases: al1, al2\\x0d              x$\n"
    "$\n"
    "  OPTIONS:$\n"
    "            --af                flag$\n"
    "|end\n"
    "=== H29b\n"
    "Aliased group$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h, --help $\n"
    "    Print this help message and exit$\n"
    "[Option Group: Aliased]$\n"
    "  desc$\n"
    "  $\n"
    "     aliases: al1, al2\\x0d              x$\n"
    "$\n"
    "  OPTIONS:$\n"
    "   --af $\n"
    "      flag$\n"
    "|end\n";

// ALIGNMENT
//   constant: what the cases of d_tests_arg_formatter_alignment come to, as the
// reference writes them with CLI11.
static const char ALIGNMENT[] =
    "=== H10a\n"
    "Ratio$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h, --help                  Print this help message and exit$\n"
    "      --opt INT               Something$\n"
    "  -a INT                      Short only$\n"
    "  -e, -f, -g, -k, --long-one  Many shorts$\n"
    "  -x, -y, --ex, --why         Two of each$\n"
    "|end\n"
    "=== H10b\n"
    "Ratio$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h, --help                  Print this help message and exit$\n"
    "--opt INT                     Something$\n"
    "  -a INT                      Short only$\n"
    "  -e, -f, -g, -k, --long-one  Many shorts$\n"
    "  -x, -y, --ex, --why         Two of each$\n"
    "|end\n"
    "=== H10c\n"
    "Ratio$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,          --help         Print this help message and exit$\n"
    "               --opt INT      Something$\n"
    "  -a INT                      Short only$\n"
    "  -e, -f, -g, -k, --long-one  Many shorts$\n"
    "  -x, -y,      --ex, --why    Two of each$\n"
    "|end\n"
    "=== H10d\n"
    "Ratio$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,          --help         Print this help message and exit$\n"
    "               --opt INT      Something$\n"
    "  -a INT                      Short only$\n"
    "  -e, -f, -g, -k, --long-one  Many shorts$\n"
    "  -x, -y,      --ex, --why    Two of each$\n"
    "|end\n"
    "=== H10e\n"
    "Ratio$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,  --help                 Print this help message and exit$\n"
    "       --opt INT              Something$\n"
    "  -a INT                      Short only$\n"
    "  -e, -f, -g, -k, --long-one  Many shorts$\n"
    "  -x, -y, --ex, --why         Two of each$\n"
    "|end\n"
    "=== H10f\n"
    "Ratio$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,  --help                 Print this help message and exit$\n"
    "       --opt INT              Something$\n"
    "  -a INT                      Short only$\n"
    "  -e, -f, -g, -k, --long-one  Many shorts$\n"
    "  -x, -y, --ex, --why         Two of each$\n"
    "|end\n"
    "=== H10g\n"
    "Ratio$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,                         --help $\n"
    "                              Print this help message and exit$\n"
    "                              --opt INT $\n"
    "                              Something$\n"
    "  -a INT                      Short only$\n"
    "  -e, -f, -g, -k,             --long-one $\n"
    "                              Many shorts$\n"
    "  -x, -y,                     --ex, --why $\n"
    "                              Two of each$\n"
    "|end\n"
    "=== H10h\n"
    "Ratio$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,                --help   Print this help message and exit$\n"
    "                     --opt INT $\n"
    "                              Something$\n"
    "  -a INT                      Short only$\n"
    "  -e, -f, -g, -k,    --long-one $\n"
    "                              Many shorts$\n"
    "  -x, -y,            --ex, --why $\n"
    "                              Two of each$\n"
    "|end\n";

// PARAGRAPHS
//   constant: what the cases of d_tests_arg_formatter_paragraphs come to, as
// the reference writes them with CLI11.
static const char PARAGRAPHS[] =
    "=== H11a\n"
    "This is a test of the description$\n"
    "systemsssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss$"
    "\n"
    "to Pr e s e r v e SPA C ES$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --option            MyFlag$\n"
    "$\n"
    "this is a test of the footer$\n"
    "systemsssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss$\n"
    "to Pr e s e r v e SPA C ES$\n"
    "|end\n"
    "=== H11b\n"
    "prog$\n"
    "This is a test of the description$\n"
    "systemsssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss$"
    "\n"
    "to Pr e s e r v e SPA C ES$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "          --option            MyFlag$\n"
    "$\n"
    "this is a test of the footer$\n"
    "systemsssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss$\n"
    "to Pr e s e r v e SPA C ES$\n"
    "|end\n"
    "=== H11c\n"
    "This is       a test of the description "
    "systemsssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss "
    "to  Pr e  s  e  r  v  e  SPA C ES$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --option            MyFlag$\n"
    "$\n"
    "this is a test of the footer$\n"
    "systemsssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss$\n"
    "to Pr e s e r v e SPA C ES$\n"
    "|end\n"
    "=== H11d\n"
    "prog$\n"
    "This is       a test of the description "
    "systemsssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss "
    "to  Pr e  s  e  r  v  e  SPA C ES$\n"
    "$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "          --option            MyFlag$\n"
    "$\n"
    "this is a test of the footer$\n"
    "systemsssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss$\n"
    "to Pr e s e r v e SPA C ES$\n"
    "|end\n"
    "=== H11e\n"
    "This is       a test of the description "
    "systemsssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss "
    "to  Pr e  s  e  r  v  e  SPA C ES$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --option            MyFlag$\n"
    "$\n"
    "this is       a test of the footer "
    "systemsssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss to  "
    "Pr e  s  e  r  v  e  SPA C ES$\n"
    "|end\n"
    "=== H11f\n"
    "prog$\n"
    "This is       a test of the description "
    "systemsssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss "
    "to  Pr e  s  e  r  v  e  SPA C ES$\n"
    "$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "          --option            MyFlag$\n"
    "$\n"
    "this is       a test of the footer "
    "systemsssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss to  "
    "Pr e  s  e  r  v  e  SPA C ES$\n"
    "|end\n"
    "=== H12\n"
    "Line one$\n"
    "Line two$\n"
    "$\n"
    "Line four with spaces$\n"
    "$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --multi             first line$\n"
    "                              second line that is long enough to need "
    "wrapping in the right$\n"
    "                              column$\n"
    "                              $\n"
    "                              fourth$\n"
    "$\n"
    "          --tabs              leading tab and double spaces$\n"
    "          --spaces            $\n"
    "          --newline           $\n"
    "$\n"
    "$\n"
    "Foot one$\n"
    "$\n"
    "Foot three$\n"
    "$\n"
    "|end\n"
    "=== H24\n"
    "Caf\\xc3\\xa9 \\xe2\\x80\\x94 na\\xc3\\xafve r\\xc3\\xa9sum\\xc3\\xa9 "
    "text that goes on and on to see where it wraps when$\n"
    "bytes are counted$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --caf\\xc3\\xa9             "
    "\\xc3\\xa9\\xc3\\xa9\\xc3\\xa9\\xc3\\xa9\\xc3\\xa9\\xc3\\xa9\\xc3\\xa9\\xc"
    "3\\xa9\\xc3\\xa9\\xc3\\xa9 "
    "\\xc3\\xa9\\xc3\\xa9\\xc3\\xa9\\xc3\\xa9\\xc3\\xa9\\xc3\\xa9\\xc3\\xa9\\xc"
    "3\\xa9\\xc3\\xa9\\xc3\\xa9 "
    "\\xc3\\xa9\\xc3\\xa9\\xc3\\xa9\\xc3\\xa9\\xc3\\xa9\\xc3\\xa9\\xc3\\xa9\\xc"
    "3\\xa9\\xc3\\xa9\\xc3\\xa9$\n"
    "                              "
    "\\xc3\\xa9\\xc3\\xa9\\xc3\\xa9\\xc3\\xa9\\xc3\\xa9\\xc3\\xa9\\xc3\\xa9\\xc"
    "3\\xa9\\xc3\\xa9\\xc3\\xa9$\n"
    "|end\n";

// REQUIREMENTS
//   constant: what the cases of d_tests_arg_formatter_requirements come to, as
// the reference writes them with CLI11.
static const char REQUIREMENTS[] =
    "=== H13a\n"
    "Req REQUIRED$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "|end\n"
    "=== H13b\n"
    "Req$\n"
    "[Exactly 2 of the following options are required]$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "|end\n"
    "=== H13c\n"
    "Req$\n"
    "[Between 1 and 3 of the following options are required]$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "|end\n"
    "=== H13d\n"
    "Req$\n"
    "[At least 2 of the following options are required]$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "|end\n"
    "=== H13e\n"
    "Req$\n"
    "[At most 3 of the following options are allowed]$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "|end\n"
    "=== H13f\n"
    "Req$\n"
    "[At most 2 of the following options are allowed]$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "|end\n"
    "=== H13g\n"
    "REQUIRED$\n"
    "[Exactly 1 of the following options are required]$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "|end\n"
    "=== H13h\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "|end\n"
    "=== H14b\n"
    "Subs$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS] SUBCOMMAND$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "$\n"
    "SUBCOMMANDS:$\n"
    "  one                         One$\n"
    "|end\n"
    "=== H14c\n"
    "Subs$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS] [SUBCOMMAND]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "$\n"
    "SUBCOMMANDS:$\n"
    "  one                         One$\n"
    "|end\n"
    "=== H14d\n"
    "Subs$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS] SUBCOMMANDS$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "$\n"
    "SUBCOMMANDS:$\n"
    "  one                         One$\n"
    "|end\n";

// SUBCOMMANDS
//   constant: what the cases of d_tests_arg_formatter_subcommands come to, as
// the reference writes them with CLI11.
static const char SUBCOMMANDS[] =
    "=== H14\n"
    "Subs$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS] [SUBCOMMANDS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "$\n"
    "SUBCOMMANDS:$\n"
    "  start, st, begin            Start the thing$\n"
    "  stop REQUIRED               Stop the thing$\n"
    "  off                         Disabled$\n"
    "  nodesc                      $\n"
    "$\n"
    "Admin:$\n"
    "  admin1                      Admin one$\n"
    "  admin2                      Admin two$\n"
    "$\n"
    "+Plus:$\n"
    "  plus                        Plus group$\n"
    "|end\n"
    "=== H14e\n"
    "Subs$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "$\n"
    "SUBCOMMANDS:$\n"
    "  one                         One$\n"
    "|end\n"
    "=== H14f\n"
    "Subs$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS] [SUBCOMMANDS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "|end\n"
    "=== H17\n"
    "My prog$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS] [SUBCOMMANDS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "[Option Group: SUBCOMMANDS]$\n"
    "  This subcommand$\n"
    "  $\n"
    "$\n"
    "  OPTIONS:$\n"
    "    -h,     --help              Print this help message and exit$\n"
    "            --insub             MyFlag$\n"
    "[Option Group: group1]$\n"
    "  Grouped$\n"
    "  $\n"
    "$\n"
    "  OPTIONS:$\n"
    "    -h,     --help              Print this help message and exit$\n"
    "            --insub3            flag$\n"
    "$\n"
    "group1:$\n"
    "  sub2                        subcommand2$\n"
    "|end\n"
    "=== H27a\n"
    "Mixed$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --help-all          all$\n"
    "[Option Group: G]$\n"
    "  group g$\n"
    "  $\n"
    "$\n"
    "  OPTIONS:$\n"
    "            --help-all          all$\n"
    "            --gf                group flag$\n"
    "$\n"
    "  SUBCOMMANDS:$\n"
    "    gs                          a sub, later moved into the group$\n"
    "|end\n"
    "=== H27b\n"
    "Mixed$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --help-all          all$\n"
    "[Option Group: G]$\n"
    "  group g$\n"
    "  $\n"
    "$\n"
    "  OPTIONS:$\n"
    "            --help-all          all$\n"
    "            --gf                group flag$\n"
    "$\n"
    "  SUBCOMMANDS:$\n"
    "  gs$\n"
    "  a sub, later moved into the group$\n"
    "$\n"
    "$\n"
    "  OPTIONS:$\n"
    "            --gsf               f$\n"
    "$\n"
    "|end\n"
    "=== H28a\n"
    "Req subs$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS] [SUBCOMMANDS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "$\n"
    "SUBCOMMANDS:$\n"
    "alpha, a$\n"
    "Alpha sub REQUIRED$\n"
    "[Exactly 1 of the following options are required]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "          --x                 x$\n"
    "$\n"
    "beta$\n"
    "$\n"
    "OPTIONS:$\n"
    "          --y                 y$\n"
    "$\n"
    "|end\n"
    "=== H28b\n"
    "Req subs$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS] [SUBCOMMANDS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "$\n"
    "SUBCOMMANDS:$\n"
    "  alpha, a REQUIRED           Alpha sub$\n"
    "  beta                        $\n"
    "|end\n";

// EXPANDED
//   constant: what the cases of d_tests_arg_formatter_expanded come to, as the
// reference writes them with CLI11.
static const char EXPANDED[] =
    "=== H15a\n"
    "All$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS] [SUBCOMMANDS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --help-all          Expand all help$\n"
    "          --top               Top flag$\n"
    "$\n"
    "SUBCOMMANDS:$\n"
    "one$\n"
    "First$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "          --in-one            Flag in one$\n"
    "$\n"
    "two, 2$\n"
    "Second$\n"
    "$\n"
    "$\n"
    "POSITIONALS:$\n"
    "  target TEXT REQUIRED        Target$\n"
    "$\n"
    "SUBCOMMANDS:$\n"
    "  deep                        Deep$\n"
    "$\n"
    "Two footer$\n"
    "$\n"
    "three$\n"
    "Third$\n"
    "$\n"
    "$\n"
    "$\n"
    "Root footer$\n"
    "|end\n"
    "=== H15b\n"
    "All$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS] [SUBCOMMANDS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --help-all          Expand all help$\n"
    "          --top               Top flag$\n"
    "$\n"
    "SUBCOMMANDS:$\n"
    "  one                         First$\n"
    "  two, 2                      Second$\n"
    "  three                       Third$\n"
    "$\n"
    "Root footer$\n"
    "|end\n"
    "=== H15c\n"
    "prog$\n"
    "All$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "          --top               Top flag$\n"
    "$\n"
    "SUBCOMMANDS:$\n"
    "  one                         First$\n"
    "  two, 2                      Second$\n"
    "  three                       Third$\n"
    "$\n"
    "Root footer$\n"
    "|end\n"
    "=== H15d\n"
    "two, 2$\n"
    "Second$\n"
    "$\n"
    "$\n"
    "POSITIONALS:$\n"
    "  target TEXT REQUIRED        Target$\n"
    "$\n"
    "SUBCOMMANDS:$\n"
    "  deep                        Deep$\n"
    "$\n"
    "Two footer$\n"
    "|end\n"
    "=== H15e\n"
    "Second$\n"
    "$\n"
    "$\n"
    "two [OPTIONS] target [SUBCOMMANDS]$\n"
    "$\n"
    "$\n"
    "POSITIONALS:$\n"
    "  target TEXT REQUIRED        Target$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --help-all          Expand all help$\n"
    "$\n"
    "SUBCOMMANDS:$\n"
    "deep$\n"
    "Deep$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "          --depth INT         How deep$\n"
    "$\n"
    "$\n"
    "Two footer$\n"
    "|end\n"
    "=== H15f\n"
    "Second$\n"
    "$\n"
    "$\n"
    "before two [OPTIONS] target [SUBCOMMANDS]$\n"
    "$\n"
    "$\n"
    "POSITIONALS:$\n"
    "  target TEXT REQUIRED        Target$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --help-all          Expand all help$\n"
    "$\n"
    "SUBCOMMANDS:$\n"
    "  deep                        Deep$\n"
    "$\n"
    "Two footer$\n"
    "|end\n"
    "=== H15g\n"
    "Deep$\n"
    "$\n"
    "$\n"
    "deep [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --help-all          Expand all help$\n"
    "          --depth INT         How deep$\n"
    "$\n"
    "Two footer$\n"
    "|end\n";

// OPTION_GROUPS
//   constant: what the cases of d_tests_arg_formatter_option_groups come to, as
// the reference writes them with CLI11.
static const char OPTION_GROUPS[] =
    "=== H16a\n"
    "OptGroups$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS] [mpos]$\n"
    "$\n"
    "$\n"
    "POSITIONALS:$\n"
    "  mpos TEXT                   Merged positional$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --help-all          Expand all help$\n"
    "          --top               Top$\n"
    "          --merged            Merged flag$\n"
    "[Option Group: Group1]$\n"
    "  The first group$\n"
    "  $\n"
    "$\n"
    "  POSITIONALS:$\n"
    "    gpos TEXT                   Group positional$\n"
    "$\n"
    "  OPTIONS:$\n"
    "            --help-all          Expand all help$\n"
    "            --g1a               In group 1$\n"
    "[Option Group: Required group]$\n"
    "  REQUIRED$\n"
    "  [Exactly 1 of the following options are required]$\n"
    "  $\n"
    "$\n"
    "  OPTIONS:$\n"
    "            --help-all          Expand all help$\n"
    "            --r1                $\n"
    "            --r2                $\n"
    "[Option Group: Nested]$\n"
    "  Outer group$\n"
    "  $\n"
    "$\n"
    "  OPTIONS:$\n"
    "            --help-all          Expand all help$\n"
    "            --outer             Outer flag$\n"
    "  [Option Group: Inner]$\n"
    "    Inner group with a description long enough to wrap around the eighty "
    "column$\n"
    "    paragraph width$\n"
    "    $\n"
    "$\n"
    "    OPTIONS:$\n"
    "              --help-all          Expand all help$\n"
    "              --inner             Inner flag with a description that is "
    "long enough to wrap in the$\n"
    "                                  right column of the help$\n"
    "$\n"
    "  Nested:$\n"
    "    gsub                        A subcommand in a group$\n"
    "|end\n"
    "=== H16b\n"
    "OptGroups$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS] [mpos]$\n"
    "$\n"
    "$\n"
    "POSITIONALS:$\n"
    "  mpos TEXT                   Merged positional$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --help-all          Expand all help$\n"
    "          --top               Top$\n"
    "          --merged            Merged flag$\n"
    "[Option Group: Group1]$\n"
    "  The first group$\n"
    "  $\n"
    "$\n"
    "  POSITIONALS:$\n"
    "    gpos TEXT                   Group positional$\n"
    "$\n"
    "  OPTIONS:$\n"
    "            --help-all          Expand all help$\n"
    "            --g1a               In group 1$\n"
    "[Option Group: Required group]$\n"
    "  REQUIRED$\n"
    "  [Exactly 1 of the following options are required]$\n"
    "  $\n"
    "$\n"
    "  OPTIONS:$\n"
    "            --help-all          Expand all help$\n"
    "            --r1                $\n"
    "            --r2                $\n"
    "[Option Group: Nested]$\n"
    "  Outer group$\n"
    "  $\n"
    "$\n"
    "  OPTIONS:$\n"
    "            --help-all          Expand all help$\n"
    "            --outer             Outer flag$\n"
    "  [Option Group: Inner]$\n"
    "    Inner group with a description long enough to wrap around the eighty "
    "column$\n"
    "    paragraph width$\n"
    "    $\n"
    "$\n"
    "    OPTIONS:$\n"
    "              --help-all          Expand all help$\n"
    "              --inner             Inner flag with a description that is "
    "long enough to wrap in the$\n"
    "                                  right column of the help$\n"
    "$\n"
    "  Nested:$\n"
    "  gsub$\n"
    "  A subcommand in a group$\n"
    "$\n"
    "$\n"
    "  OPTIONS:$\n"
    "            --gsubflag          flag$\n"
    "$\n"
    "|end\n"
    "=== H16c\n"
    "Group1:The first group$\n"
    "$\n"
    "$\n"
    "Usage: [OPTIONS] [gpos]$\n"
    "$\n"
    "$\n"
    "POSITIONALS:$\n"
    "  gpos TEXT                   Group positional$\n"
    "$\n"
    "OPTIONS:$\n"
    "          --help-all          Expand all help$\n"
    "          --g1a               In group 1$\n"
    "|end\n"
    "=== H16d\n"
    "[Option Group: Required group]$\n"
    "  REQUIRED$\n"
    "  [Exactly 1 of the following options are required]$\n"
    "  $\n"
    "$\n"
    "  OPTIONS:$\n"
    "            --r1                $\n"
    "            --r2                $\n"
    "|end\n";

// PARSED
//   constant: what the cases of d_tests_arg_formatter_parsed come to, as the
// reference writes them with CLI11.
static const char PARSED[] =
    "=== H18a\n"
    "The sub$\n"
    "$\n"
    "$\n"
    "prog sub [OPTIONS] [SUBCOMMANDS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --sf                Sub flag$\n"
    "$\n"
    "SUBCOMMANDS:$\n"
    "  deeper                      Deeper$\n"
    "|end\n"
    "=== H18b\n"
    "The sub$\n"
    "$\n"
    "$\n"
    "prog sub [OPTIONS] [SUBCOMMANDS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --sf                Sub flag$\n"
    "$\n"
    "SUBCOMMANDS:$\n"
    "deeper$\n"
    "Deeper$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "          --df                Deeper flag$\n"
    "$\n"
    "|end\n"
    "=== H18c\n"
    "Deeper$\n"
    "$\n"
    "$\n"
    "prog sub deeper [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --df                Deeper flag$\n"
    "|end\n"
    "=== H18d\n"
    "Deeper$\n"
    "$\n"
    "$\n"
    "pre prog sub deeper [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --df                Deeper flag$\n"
    "|end\n"
    "=== H18e\n"
    "Deeper$\n"
    "$\n"
    "$\n"
    "prog sub deeper [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --df                Deeper flag$\n"
    "|end\n"
    "=== H18f\n"
    "Deeper$\n"
    "$\n"
    "$\n"
    "prog sub deeper [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --df                Deeper flag$\n"
    "|end\n"
    "=== H18g\n"
    "ERROR: ExtrasError: sub: The following argument was not expected: "
    "--nope$\n"
    "The sub$\n"
    "$\n"
    "$\n"
    "prog sub [OPTIONS] [SUBCOMMANDS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --sf                Sub flag$\n"
    "$\n"
    "SUBCOMMANDS:$\n"
    "  deeper                      Deeper$\n"
    "|end\n";

// FALLTHROUGH
//   constant: what the cases of d_tests_arg_formatter_fallthrough come to, as
// the reference writes them with CLI11.
static const char FALLTHROUGH[] =
    "=== H19\n"
    "Sub$\n"
    "$\n"
    "$\n"
    "sub [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --shared            Sub's shared$\n"
    "          --parent-flag       Parent's$\n"
    "|end\n"
    "=== H33\n"
    "Sub$\n"
    "$\n"
    "$\n"
    "sub [OPTIONS] [ppos]$\n"
    "$\n"
    "$\n"
    "POSITIONALS:$\n"
    "  ppos TEXT                   parent positional$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --shared            Parent's shared$\n"
    "$\n"
    "Other:$\n"
    "          --shared            Sub's shared$\n"
    "          --pf                parent other$\n"
    "|end\n";

// USAGE_AND_FOOTER
//   constant: what the cases of d_tests_arg_formatter_usage_and_footer come to,
// as the reference writes them with CLI11.
static const char USAGE_AND_FOOTER[] =
    "=== H20a\n"
    "Use$\n"
    "$\n"
    "use: just use it$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "|end\n"
    "=== H20b\n"
    "Use$\n"
    "$\n"
    "use: from a function$\n"
    "$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "|end\n"
    "=== H20c\n"
    "Use$\n"
    "$\n"
    "use: first$\n"
    "and then$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "|end\n"
    "=== H20d\n"
    "Use$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "$\n"
    "footer from a function$\n"
    "$\n"
    "|end\n"
    "=== H20e\n"
    "Use$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "$\n"
    "first$\n"
    "second$\n"
    "|end\n"
    "=== H20f\n"
    "Use$\n"
    "$\n"
    "$\n"
    "$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "|end\n";

// SPECIAL_OPTIONS
//   constant: what the cases of d_tests_arg_formatter_special_options come to,
// as the reference writes them with CLI11.
static const char SPECIAL_OPTIONS[] =
    "=== H21\n"
    "Ver$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "  -V,     --version           Display program version information and "
    "exit$\n"
    "          --config [default.ini]  $\n"
    "                              Read an ini file$\n"
    "|end\n"
    "=== H21b\n"
    "Ver$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "  -c,     --cfg REQUIRED      Read a file$\n"
    "|end\n"
    "=== H22a\n"
    "Bare$\n"
    "$\n"
    "$\n"
    "prog$\n"
    "$\n"
    "|end\n"
    "=== H22b\n"
    "Bare$\n"
    "$\n"
    "$\n"
    "prog [p]$\n"
    "$\n"
    "$\n"
    "POSITIONALS:$\n"
    "  p TEXT                      pos$\n"
    "|end\n"
    "=== H22c\n"
    "Bare$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "|end\n"
    "=== H23\n"
    "Old$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --something TEXT:DEPRECATED $\n"
    "                              My option here DEPRECATED: please use "
    "'--something_else' instead$\n"
    "          --something_else TEXT $\n"
    "                              My option here$\n"
    "          --old RETIRED [RETIRED]  $\n"
    "                              option has been retired and has no effect$\n"
    "          --gone RETIRED [RETIRED]  $\n"
    "                              option has been retired and has no effect$\n"
    "|end\n";

// REPLACED
//   constant: what the cases of d_tests_arg_formatter_replaced come to, as the
// reference writes them with CLI11.
static const char REPLACED[] =
    "=== H30a\n"
    "Custom$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS] [P:pos] [SUBCOMMANDS]$\n"
    "$\n"
    "$\n"
    "== POSITIONALS (positional) ==$\n"
    "  P:pos TEXT                  <A positional>$\n"
    "$\n"
    "== OPTIONS ==$\n"
    "  -h,     --help              <Print this help message and exit>$\n"
    "          --help-all          <all>$\n"
    "  -n,     --num INT           <A number>$\n"
    "[Option Group: G]$\n"
    "  group$\n"
    "  $\n"
    "$\n"
    "  == OPTIONS ==$\n"
    "            --help-all          <all>$\n"
    "            --gf                <group flag>$\n"
    "FOOT()$\n"
    "SUBCOMMANDS:$\n"
    "  * sub$\n"
    "FOOT(prog)|end\n"
    "=== H30b\n"
    "Custom$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS] [P:pos] [SUBCOMMANDS]$\n"
    "$\n"
    "$\n"
    "== POSITIONALS (positional) ==$\n"
    "  P:pos TEXT                  <A positional>$\n"
    "$\n"
    "== OPTIONS ==$\n"
    "  -h,     --help              <Print this help message and exit>$\n"
    "          --help-all          <all>$\n"
    "  -n,     --num INT           <A number>$\n"
    "[Option Group: G]$\n"
    "  group$\n"
    "  $\n"
    "$\n"
    "  == OPTIONS ==$\n"
    "            --help-all          <all>$\n"
    "            --gf                <group flag>$\n"
    "FOOT()$\n"
    "SUBCOMMANDS:$\n"
    "sub$\n"
    "A sub$\n"
    "$\n"
    "$\n"
    "== OPTIONS ==$\n"
    "          --sf                <sub flag>$\n"
    "FOOT(sub)$\n"
    "FOOT(prog)|end\n"
    "=== H31a\n"
    "HELP for [prog] of prog mode 0$\n"
    "|end\n"
    "=== H31b\n"
    "HELP for [pre prog] of prog mode 1$\n"
    "|end\n"
    "=== H31c\n"
    "HELP for [prog sub] of sub mode 0$\n"
    "|end\n"
    "=== H32a\n"
    "PerSub$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS] [SUBCOMMANDS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this help message and exit$\n"
    "          --help-all          all$\n"
    "          --rf                root flag$\n"
    "$\n"
    "SUBCOMMANDS:$\n"
    "sub$\n"
    "A sub$\n"
    "$\n"
    "$\n"
    "== OPTIONS ==$\n"
    "          --sf                <sub flag>$\n"
    "FOOT(sub)$\n"
    "|end\n"
    "=== H32b\n"
    "A sub$\n"
    "$\n"
    "$\n"
    "sub [OPTIONS]$\n"
    "$\n"
    "$\n"
    "== OPTIONS ==$\n"
    "  -h,     --help              <Print this help message and exit>$\n"
    "          --help-all          <all>$\n"
    "          --sf                <sub flag>$\n"
    "FOOT(sub)|end\n";

// LINE_WIDTH
//   constant: what the cases of d_tests_arg_formatter_line_width come to,
// the module's own: CLI11 has no line width. At 40 the right column
// narrows to what the line leaves it, and a long name keeps its line
// whole; at 20, narrower than the left column, a word goes to a line.
static const char LINE_WIDTH[] =
    "=== L01\n"
    "alpha beta gamma delta epsilon zeta eta$\n"
    "theta$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print this$\n"
    "                              help$\n"
    "                              message$\n"
    "                              and exit$\n"
    "  -a,     --alpha             one two$\n"
    "                              three four$\n"
    "                              five six$\n"
    "                              seven$\n"
    "                              eight nine$\n"
    "                              ten$\n"
    "          --a-very-long-option-name $\n"
    "                              short$\n"
    "|end\n"
    "=== L02\n"
    "alpha beta gamma$\n"
    "delta epsilon zeta$\n"
    "eta theta$\n"
    "$\n"
    "$\n"
    "prog [OPTIONS]$\n"
    "$\n"
    "$\n"
    "OPTIONS:$\n"
    "  -h,     --help              Print$\n"
    "                              this$\n"
    "                              help$\n"
    "                              message$\n"
    "                              and$\n"
    "                              exit$\n"
    "  -a,     --alpha             one$\n"
    "                              two$\n"
    "                              three$\n"
    "                              four$\n"
    "                              five$\n"
    "                              six$\n"
    "                              seven$\n"
    "                              eight$\n"
    "                              nine$\n"
    "                              ten$\n"
    "          --a-very-long-option-name $\n"
    "                              short$\n"
    "|end\n";

// d_tests_arg_formatter_variable
//   union: room for a variable of any built-in type an option is bound
// to; nothing is parsed into one.
union d_tests_arg_formatter_variable
{
    long double real;
    uintmax_t   whole;
    const char* text;
    bool        flag;
};

// d_tests_arg_formatter_columns
//   struct: a value of COLUMNS, and the width d_arg_terminal_width reads
// from it.
struct d_tests_arg_formatter_columns
{
    const char* value;
    size_t      width;
};

#if (D_INTERNAL_ARG_PARSER_TERMINAL_WIDTH == 1)

// COLUMNS_CASES
//   constant: values of COLUMNS, each with the width read from it: digits
// alone from 1 to 10000, and 0 for anything else.
static const struct d_tests_arg_formatter_columns COLUMNS_CASES[] =
{
    { "1",     1u     },
    { "007",   7u     },
    { "10000", 10000u },
    { "0",     0u     },
    { "10001", 0u     },
    { "-5",    0u     },
    { "abc",   0u     },
    { "80x",   0u     }
};

// COLUMNS_COUNT
//   constant: how many values COLUMNS_CASES holds.
static const size_t COLUMNS_COUNT =
    sizeof(COLUMNS_CASES) / sizeof(COLUMNS_CASES[0]);

#endif  // D_INTERNAL_ARG_PARSER_TERMINAL_WIDTH

//==============================================================================
// HELPERS
//==============================================================================

/**
 * @brief Adds a text as the help probe writes one: `=== <tag>`, the text
 *        with each line end written `$` and a newline -- so that trailing
 *        spaces show -- each byte outside printable ASCII, and the
 *        backslash, as a hexadecimal escape of two digits, then `|end`.
 *
 * @param[in,out] _run   the run.
 * @param[in]     _tag   the case's tag.
 * @param[in]     _text  the text.
 */
static void
d_tests_arg_formatter_put(
    struct d_tests_arg_run* _run,
    const char*             _tag,
    struct d_pack_text      _text
)
{
    (void)d_arg_buffer_printf(&_run->transcript,
                              "=== %s\n",
                              _tag);

    // each byte, as itself or as its escape
    for (size_t index = 0u; index < _text.length; ++index)
    {
        const unsigned char byte = (unsigned char)_text.data[index];

        // a line end, a byte shown as itself, or an escape
        if (byte == '\n')
        {
            (void)d_arg_buffer_append_cstr(&_run->transcript,
                                           "$\n");
        }
        else if ( (byte >= 0x20)   &&
                  (byte < 0x7f)    &&
                  (byte != '\\') )
        {
            (void)d_arg_buffer_append_char(&_run->transcript,
                                           (char)byte);
        }
        else
        {
            (void)d_arg_buffer_printf(&_run->transcript,
                                      "\\x%02x",
                                      (unsigned int)byte);
        }
    }

    (void)d_arg_buffer_append_cstr(&_run->transcript,
                                   "|end\n");

    return;
}

/**
 * @brief Adds a command's help, as CLI11's help(prev, mode) writes it, in
 *        the help probe's form; `=== <tag>` and `STATUS <status>` for help
 *        that could not be made.
 *
 * @param[in,out] _run      the run.
 * @param[in]     _tag      the case's tag.
 * @param[in]     _command  the command.
 * @param[in]     _prefix   the words before the command's name.
 * @param[in]     _mode     a d_arg_help_mode value.
 */
static void
d_tests_arg_formatter_help(
    struct d_tests_arg_run*     _run,
    const char*                 _tag,
    const struct d_arg_command* _command,
    const char*                 _prefix,
    int                         _mode
)
{
    struct d_arg_buffer help   = D_ARG_BUFFER_INIT;
    const int           status = d_arg_help(_command,
                                            d_pack_text_from_cstr(_prefix),
                                            _mode,
                                            d_arg_buffer_sink(&help));

    // the help, or what stopped it
    if (status == D_ARG_OK)
    {
        d_tests_arg_formatter_put(_run,
                                  _tag,
                                  d_arg_buffer_text(&help));
    }
    else
    {
        (void)d_arg_buffer_printf(&_run->transcript,
                                  "=== %s\nSTATUS %d\n|end\n",
                                  _tag,
                                  status);
    }

    d_arg_buffer_release(&help);

    return;
}

/**
 * @brief Makes an option bound to a variable of a built-in type, kept by
 *        the run, as the help probe binds options to variables of its own.
 *
 * @param[in,out] _run          the run.
 * @param[in,out] _command      the command the option is made on.
 * @param[in]     _names        the option's names.
 * @param[in]     _builtin      the variable's d_arg_builtin type.
 * @param[in]     _description  the option's description.
 * @return the option; NULL when it was not made, which fails the run.
 */
static struct d_arg_option*
d_tests_arg_formatter_bound(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _command,
    const char*             _names,
    int                     _builtin,
    const char*             _description
)
{
    void* const variable = d_tests_arg_keep(
                               _run,
                               sizeof(union d_tests_arg_formatter_variable));

    // a variable that could not be kept has failed the run
    if (!variable)
    {
        return NULL;
    }

    struct d_arg_option* const option =
        d_arg_command_add_option_bound(_command,
                                       _names,
                                       variable,
                                       d_arg_type_builtin(_builtin),
                                       _description);

    _run->failed = ( (_run->failed) ||
                     (option == NULL) );

    return option;
}

/**
 * @brief Makes a flag bound to a variable of a built-in type, kept by the
 *        run, as the help probe binds flags to variables of its own.
 *
 * @param[in,out] _run          the run.
 * @param[in,out] _command      the command the flag is made on.
 * @param[in]     _names        the flag's names.
 * @param[in]     _builtin      the variable's d_arg_builtin type.
 * @param[in]     _description  the flag's description.
 * @return the flag; NULL when it was not made, which fails the run.
 */
static struct d_arg_option*
d_tests_arg_formatter_bound_flag(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _command,
    const char*             _names,
    int                     _builtin,
    const char*             _description
)
{
    void* const variable = d_tests_arg_keep(
                               _run,
                               sizeof(union d_tests_arg_formatter_variable));

    // a variable that could not be kept has failed the run
    if (!variable)
    {
        return NULL;
    }

    struct d_arg_option* const flag =
        d_arg_command_add_flag_bound(_command,
                                     _names,
                                     variable,
                                     d_arg_type_builtin(_builtin),
                                     _description);

    _run->failed = ( (_run->failed) ||
                     (flag == NULL) );

    return flag;
}

/**
 * @brief Makes an option of a type's name that takes any number of values,
 *        as CLI11's add_option over a std::vector shows one in help.
 *
 * @param[in,out] _command      the command the option is made on.
 * @param[in]     _names        the option's names.
 * @param[in]     _type_name    the name of the values' type.
 * @param[in]     _description  the option's description.
 * @return the option; NULL when it was not made.
 */
static struct d_arg_option*
d_tests_arg_formatter_vector(
    struct d_arg_command* _command,
    const char*           _names,
    const char*           _type_name,
    const char*           _description
)
{
    struct d_arg_option* const option = d_arg_command_add_option(_command,
                                                                 _names,
                                                                 _description);

    (void)d_arg_option_set_type_name(option,
                                     _type_name);
    d_arg_option_set_expected(option,
                              D_ARG_EXPECTED_UNLIMITED);

    return option;
}

/**
 * @brief Writes the text it is given as a usage or a footer, as the help
 *        probe's functions return theirs.
 *
 * @param[in] _command  the command; unused.
 * @param[in] _sink     where the text goes.
 * @param[in] _context  the text.
 * @return the sink's status.
 */
static int
d_tests_arg_formatter_text(
    const struct d_arg_command* _command,
    struct d_pack_sink          _sink,
    void*                       _context
)
{
    (void)_command;

    return d_arg_sink_cstr(_sink,
                           (const char*)_context);
}

/**
 * @brief Parses a command line held in one string, adding nothing to the
 *        transcript, as the help probe parses before asking for help.
 *
 * @param[in,out] _run   the run.
 * @param[in]     _line  the line, without the program's name.
 * @return the parse's status.
 */
static int
d_tests_arg_formatter_parse(
    struct d_tests_arg_run* _run,
    const char*             _line
)
{
    return d_arg_parser_parse_line(&_run->parser,
                                   d_pack_text_from_cstr(_line),
                                   false);
}

/**
 * @brief Ends the parse with a status into the run's buffers, as CLI11's
 *        exit(error, out, err), and adds what it wrote to one of them in
 *        the help probe's form; then empties both.
 *
 * @param[in,out] _run           the run.
 * @param[in]     _tag           the case's tag.
 * @param[in]     _status        the status ended with.
 * @param[in]     _error_output  whether the error output is added rather
 *                               than the output.
 */
static void
d_tests_arg_formatter_ended(
    struct d_tests_arg_run* _run,
    const char*             _tag,
    int                     _status,
    bool                    _error_output
)
{
    (void)d_arg_parser_exit_to(&_run->parser,
                               _status,
                               d_arg_buffer_sink(&_run->out),
                               d_arg_buffer_sink(&_run->err));
    d_tests_arg_formatter_put(_run,
                              _tag,
                              d_arg_buffer_text((_error_output)
                                                ? &_run->err
                                                : &_run->out));
    d_arg_buffer_clear(&_run->out);
    d_arg_buffer_clear(&_run->err);

    return;
}

/**
 * @brief Parses a line and ends the parse with its status, adding what the
 *        failure message wrote, as the help probe's last parse.
 *
 * @param[in,out] _run   the run.
 * @param[in]     _tag   the case's tag.
 * @param[in]     _line  the line, without the program's name.
 */
static void
d_tests_arg_formatter_failed(
    struct d_tests_arg_run* _run,
    const char*             _tag,
    const char*             _line
)
{
    const int status = d_tests_arg_formatter_parse(_run,
                                                   _line);

    d_tests_arg_formatter_ended(_run,
                                _tag,
                                status,
                                true);

    return;
}

/**
 * @brief Makes the root the help probe's `ratio` makes -- two options, and
 *        flags of many short and long names -- under a long-name alignment
 *        ratio, and adds its help.
 *
 * @param[in,out] _run    the run.
 * @param[in,out] _app    the run's root.
 * @param[in]     _tag    the case's tag.
 * @param[in]     _ratio  the ratio.
 */
static void
d_tests_arg_formatter_ratio(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app,
    const char*             _tag,
    float                   _ratio
)
{
    (void)d_arg_command_set_description(_app,
                                        "Ratio");
    d_arg_formatter_set_long_option_alignment_ratio(&_run->parser.formatter,
                                                    _ratio);
    (void)d_tests_arg_formatter_bound(_run,
                                      _app,
                                      "--opt",
                                      D_ARG_BUILTIN_INT,
                                      "Something");
    (void)d_tests_arg_formatter_bound(_run,
                                      _app,
                                      "-a",
                                      D_ARG_BUILTIN_INT,
                                      "Short only");
    (void)d_arg_command_add_flag(_app,
                                 "-e,-f,-g,-k,--long-one",
                                 "Many shorts");
    (void)d_arg_command_add_flag(_app,
                                 "-x,-y,--ex,--why",
                                 "Two of each");
    d_tests_arg_formatter_help(_run,
                               _tag,
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief The help probe's make_option_desc: the description in angle
 *        brackets.
 *
 * @param[in]  _formatter  the formatter.
 * @param[in]  _option     the option.
 * @param[out] _out        what the step appends to.
 * @return D_ARG_OK, or the status that stopped the step.
 */
static int
d_tests_arg_formatter_desc(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_option*    _option,
    struct d_arg_buffer*          _out
)
{
    int status = d_arg_buffer_append_char(_out,
                                          '<');

    status = (status == D_ARG_OK)
             ? d_arg_format_option_desc_default(_formatter,
                                                _option,
                                                _out)
             : status;

    return (status == D_ARG_OK)
           ? d_arg_buffer_append_char(_out,
                                      '>')
           : status;
}

/**
 * @brief The help probe's make_option_name: a positional's name after
 *        `P:`.
 *
 * @param[in]  _formatter   the formatter.
 * @param[in]  _option      the option.
 * @param[in]  _positional  whether the name is a positional's.
 * @param[out] _out         what the step appends to.
 * @return D_ARG_OK, or the status that stopped the step.
 */
static int
d_tests_arg_formatter_name(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_option*    _option,
    bool                          _positional,
    struct d_arg_buffer*          _out
)
{
    const int status = (_positional)
                       ? d_arg_buffer_append_cstr(_out,
                                                  "P:")
                       : D_ARG_OK;

    return (status == D_ARG_OK)
           ? d_arg_format_option_name_default(_formatter,
                                              _option,
                                              _positional,
                                              _out)
           : status;
}

/**
 * @brief The help probe's make_footer: `FOOT(<command's name>)`.
 *
 * @param[in]  _formatter  the formatter; unused.
 * @param[in]  _command    the command.
 * @param[out] _out        what the step appends to.
 * @return D_ARG_OK, or the buffer's status.
 */
static int
d_tests_arg_formatter_footer(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    struct d_arg_buffer*          _out
)
{
    (void)_formatter;

    return d_arg_buffer_printf(_out,
                               "FOOT(%s)",
                               d_arg_command_name(_command));
}

/**
 * @brief The help probe's make_group: `== <group> ==` -- with
 *        ` (positional)` after a positionals' group -- then each option.
 *
 * @param[in]  _formatter   the formatter.
 * @param[in]  _group       the group's name.
 * @param[in]  _positional  whether the group's options are positionals.
 * @param[in]  _options     the group's options.
 * @param[out] _out         what the step appends to.
 * @return D_ARG_OK, or the status that stopped the step.
 */
static int
d_tests_arg_formatter_group(
    const struct d_arg_formatter*   _formatter,
    struct d_pack_text              _group,
    bool                            _positional,
    const struct d_arg_option_list* _options,
    struct d_arg_buffer*            _out
)
{
    int status = d_arg_buffer_printf(_out,
                                     "\n== %.*s%s ==\n",
                                     (int)_group.length,
                                     _group.data,
                                     (_positional) ? " (positional)" : "");

    // each option, while each is written
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _options->count);
         ++index)
    {
        status = d_arg_format_option(_formatter,
                                     _options->items[index],
                                     _positional,
                                     _out);
    }

    return status;
}

/**
 * @brief The help probe's make_subcommand: `  * <name>`.
 *
 * @param[in]  _formatter  the formatter; unused.
 * @param[in]  _command    the subcommand.
 * @param[out] _out        what the step appends to.
 * @return D_ARG_OK, or the buffer's status.
 */
static int
d_tests_arg_formatter_subcommand(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    struct d_arg_buffer*          _out
)
{
    (void)_formatter;

    return d_arg_buffer_printf(_out,
                               "  * %s\n",
                               d_arg_command_name(_command));
}

/**
 * @brief The help probe's formatter_fn: one line naming the help asked
 *        for, in place of the whole of it.
 *
 * @param[in]  _formatter  the formatter; unused.
 * @param[in]  _command    the command.
 * @param[in]  _name       the name help shows.
 * @param[in]  _mode       the d_arg_help_mode value.
 * @param[out] _out        what the step appends to.
 * @return D_ARG_OK, or the buffer's status.
 */
static int
d_tests_arg_formatter_whole(
    const struct d_arg_formatter* _formatter,
    const struct d_arg_command*   _command,
    struct d_pack_text            _name,
    int                           _mode,
    struct d_arg_buffer*          _out
)
{
    (void)_formatter;

    return d_arg_buffer_printf(_out,
                               "HELP for [%.*s] of %s mode %d\n",
                               (int)_name.length,
                               _name.data,
                               d_arg_command_name(_command),
                               _mode);
}

/**
 * @brief A formatter with a step table, in storage the run keeps, so that
 *        it outlives the commands that borrow it.
 *
 * @param[in,out] _run     the run.
 * @param[in]     _vtable  the formatter's steps.
 * @return the formatter, as d_arg_formatter_init makes it but for its
 *         steps; NULL when it could not be kept, which fails the run.
 */
static struct d_arg_formatter*
d_tests_arg_formatter_kept(
    struct d_tests_arg_run*              _run,
    const struct d_arg_formatter_vtable* _vtable
)
{
    struct d_arg_formatter* const formatter =
        d_tests_arg_keep(_run,
                         sizeof(struct d_arg_formatter));

    // a formatter that could not be kept has failed the run
    if (!formatter)
    {
        return NULL;
    }

    d_arg_formatter_init(formatter);
    formatter->vtable = _vtable;

    return formatter;
}

/**
 * @brief The help probe's MyFormatter, kept by the run: CLI11's Formatter
 *        with five steps replaced -- groups, subcommands, the footer, an
 *        option's name and its description.
 *
 * @param[in,out] _run  the run.
 * @return the formatter; NULL when it could not be kept, which fails the
 *         run.
 */
static struct d_arg_formatter*
d_tests_arg_formatter_custom(
    struct d_tests_arg_run* _run
)
{
    // MY_FORMATTER
    //   constant: the steps MyFormatter replaces, the rest CLI11's.
    static const struct d_arg_formatter_vtable MY_FORMATTER =
    {
        .make_group       = d_tests_arg_formatter_group,
        .make_subcommand  = d_tests_arg_formatter_subcommand,
        .make_footer      = d_tests_arg_formatter_footer,
        .make_option_name = d_tests_arg_formatter_name,
        .make_option_desc = d_tests_arg_formatter_desc
    };

    return d_tests_arg_formatter_kept(_run,
                                      &MY_FORMATTER);
}

/**
 * @brief The help probe's formatter_fn, kept by the run: the whole of help
 *        replaced by one line.
 *
 * @param[in,out] _run  the run.
 * @return the formatter; NULL when it could not be kept, which fails the
 *         run.
 */
static struct d_arg_formatter*
d_tests_arg_formatter_lambda(
    struct d_tests_arg_run* _run
)
{
    // WHOLE_HELP
    //   constant: the one step replaced, the whole of help.
    static const struct d_arg_formatter_vtable WHOLE_HELP =
    {
        .make_help = d_tests_arg_formatter_whole
    };

    return d_tests_arg_formatter_kept(_run,
                                      &WHOLE_HELP);
}

/**
 * @brief Makes a command's help into a buffer, emptied first.
 *
 * @param[in]  _command  the command.
 * @param[in]  _mode     a d_arg_help_mode value.
 * @param[out] _out      the buffer.
 * @return d_arg_help's status.
 */
static int
d_tests_arg_formatter_make(
    const struct d_arg_command* _command,
    int                         _mode,
    struct d_arg_buffer*        _out
)
{
    d_arg_buffer_clear(_out);

    return d_arg_help(_command,
                      d_pack_text_from_cstr(""),
                      _mode,
                      d_arg_buffer_sink(_out));
}

/**
 * @brief The length of a text's longest line but for a line that is one
 *        word behind its indent: a word longer than the room it has, which
 *        no width can break.
 *
 * @param[in] _text  the text.
 * @return the length, 0 for a text of no such line.
 */
static size_t
d_tests_arg_formatter_longest(
    struct d_pack_text _text
)
{
    size_t longest = 0u;
    size_t start   = 0u;

    // each line, at its end
    for (size_t index = 0u; index <= _text.length; ++index)
    {
        // inside a line
        if ( (index < _text.length)       &&
             (_text.data[index] != '\n') )
        {
            continue;
        }

        size_t at = start;

        // past the indent
        while ( (at < index)              &&
                (_text.data[at] == ' ') )
        {
            ++at;
        }

        const bool words = (memchr(_text.data + at,
                                   ' ',
                                   index - at) != NULL);

        longest = ( (words)                   &&
                    ((index - start) > longest) )
                  ? (index - start)
                  : longest;
        start   = index + 1u;
    }

    return longest;
}

/**
 * @brief Makes the option group of the root the C-only sections measure:
 *        a flag, a group inside it and a subcommand, each described at
 *        length.
 *
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_build_group(
    struct d_arg_command* _app
)
{
    struct d_arg_command* const group =
        d_arg_command_add_option_group(_app,
                                       "Group",
                                       "a group whose description goes on "
                                       "for quite a while, longer than the "
                                       "line");

    (void)d_arg_command_add_flag(group,
                                 "--in-group",
                                 "a flag in the group whose description "
                                 "goes on for quite a while");

    struct d_arg_command* const inner =
        d_arg_command_add_option_group(group,
                                       "Inner",
                                       "an inner group whose description goes "
                                       "on and on and on and on");

    (void)d_arg_command_add_flag(inner,
                                 "--inner",
                                 "a flag in the inner group whose "
                                 "description goes on and on");

    struct d_arg_command* const named =
        d_arg_command_add_subcommand(group,
                                     "named",
                                     "a named subcommand in the group, "
                                     "described at some length to wrap");

    (void)d_arg_command_add_flag(named,
                                 "--nf",
                                 "a flag of the named subcommand in the "
                                 "group, described at length");

    return;
}

/**
 * @brief Makes the root the C-only sections measure: a description, a
 *        footer and help text long enough to wrap, a positional, an option
 *        group holding a group and a subcommand, and a subcommand, as the
 *        help probe's C checks make it.
 *
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_build(
    struct d_arg_command* _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "alpha beta gamma delta epsilon zeta "
                                        "eta theta iota kappa lambda mu nu xi "
                                        "omicron pi rho sigma tau");
    (void)d_arg_command_set_footer(_app,
                                   "one two three four five six seven eight "
                                   "nine ten eleven twelve thirteen fourteen "
                                   "fifteen sixteen");
    (void)d_arg_command_set_help_all_flag(_app,
                                          "--help-all",
                                          "Expand all help, with a "
                                          "description long enough to wrap");
    (void)d_arg_command_add_flag(_app,
                                 "-a,--alpha",
                                 "one two three four five six seven eight "
                                 "nine ten eleven twelve thirteen");
    (void)d_arg_command_add_option(_app,
                                   "pos",
                                   "a positional with a description that "
                                   "goes on for quite a while");
    d_tests_arg_formatter_build_group(_app);

    struct d_arg_command* const sub =
        d_arg_command_add_subcommand(_app,
                                     "sub",
                                     "a subcommand whose description goes on "
                                     "for quite a while, past the line");

    (void)d_arg_command_add_flag(sub,
                                 "--sf",
                                 "a flag of the subcommand whose description "
                                 "goes on for a while");

    return;
}

/**
 * @brief A usage function that fails, as no CLI11 usage function can.
 *
 * @param[in] _command  the command; unused.
 * @param[in] _sink     where the usage would go; unused.
 * @param[in] _context  unused.
 * @return D_ARG_ERROR_CONVERSION.
 */
static int
d_tests_arg_formatter_unusable(
    const struct d_arg_command* _command,
    struct d_pack_sink          _sink,
    void*                       _context
)
{
    (void)_command;
    (void)_sink;
    (void)_context;

    return D_ARG_ERROR_CONVERSION;
}

/**
 * @brief A sink's write that takes nothing, as a full disk or a closed pipe
 *        would.
 *
 * @param[in] _context  unused.
 * @param[in] _data     unused.
 * @param[in] _size     unused.
 * @return 0.
 */
static size_t
d_tests_arg_formatter_refuse(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    (void)_context;
    (void)_data;
    (void)_size;

    return 0u;
}

/**
 * @brief Says what did not hold.
 *
 * @param[in] _held  whether it held.
 * @param[in] _what  what it is, printed when it did not.
 * @return `_held`.
 */
static bool
d_tests_arg_formatter_held(
    bool        _held,
    const char* _what
)
{
    // what did not hold, said
    if (!_held)
    {
        printf("    %s\n",
               _what);
    }

    return _held;
}

/**
 * @brief Whether a span holds a text.
 *
 * @param[in] _span  the span.
 * @param[in] _text  the text.
 * @return whether the two are the same.
 */
static bool
d_tests_arg_formatter_is(
    struct d_pack_text _span,
    const char*        _text
)
{
    const size_t length = strlen(_text);

    return ( (_span.length == length) &&
             ( (length == 0u) ||
               (memcmp(_span.data,
                       _text,
                       length) == 0) ) );
}

/**
 * @brief Whether a formatter labels a word with a text.
 *
 * @param[in] _formatter  the formatter; NULL for none.
 * @param[in] _key        the word.
 * @param[in] _text       the label expected.
 * @return whether d_arg_formatter_label gives the text.
 */
static bool
d_tests_arg_formatter_labelled(
    const struct d_arg_formatter* _formatter,
    const char*                   _key,
    const char*                   _text
)
{
    return d_tests_arg_formatter_is(
               d_arg_formatter_label(_formatter,
                                     d_pack_text_from_cstr(_key)),
               _text);
}

//==============================================================================
// 1.  BASICS
//==============================================================================

/**
 * @brief H01: a root without a name.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "My prog");
    (void)d_arg_command_set_name(_app,
                                 NULL);
    d_tests_arg_formatter_help(_run,
                               "H01",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H02: a named root, a flag and an optional positional.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "My prog");
    (void)d_arg_command_set_name(_app,
                                 "program");
    (void)d_arg_command_add_flag(_app,
                                 "-q,--quick",
                                 "Quickly");
    (void)d_tests_arg_formatter_bound(_run,
                                      _app,
                                      "something",
                                      D_ARG_BUILTIN_STRING,
                                      "My option here");
    d_tests_arg_formatter_help(_run,
                               "H02",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H03: type names, a default, value counts, required options.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h03(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Types");
    (void)d_tests_arg_formatter_bound(_run,
                                      _app,
                                      "-i,--int",
                                      D_ARG_BUILTIN_INT,
                                      "An integer");

    struct d_arg_option* const real =
        d_tests_arg_formatter_bound(_run,
                                    _app,
                                    "-d,--double",
                                    D_ARG_BUILTIN_DOUBLE,
                                    "A double");

    (void)d_arg_option_set_default_text(real,
                                        "2.5");

    struct d_arg_option* const word =
        d_tests_arg_formatter_bound(_run,
                                    _app,
                                    "--str",
                                    D_ARG_BUILTIN_STRING,
                                    "A string");

    d_arg_option_set_required(word,
                              true);
    (void)d_tests_arg_formatter_vector(_app,
                                       "-v,--vals",
                                       "INT",
                                       "Values");

    struct d_arg_option* const two = d_tests_arg_formatter_vector(_app,
                                                                  "--two",
                                                                  "INT",
                                                                  "Two values");

    d_arg_option_set_expected(two,
                              2);

    struct d_arg_option* const some =
        d_tests_arg_formatter_vector(_app,
                                     "--some",
                                     "INT",
                                     "Two to four");

    d_arg_option_set_expected_range(some,
                                    2,
                                    4);

    struct d_arg_option* const file =
        d_tests_arg_formatter_bound(_run,
                                    _app,
                                    "file",
                                    D_ARG_BUILTIN_STRING,
                                    "The file");

    d_arg_option_set_required(file,
                              true);

    struct d_arg_option* const pair =
        d_tests_arg_formatter_vector(_app,
                                     "pair",
                                     "INT",
                                     "Two of them");

    d_arg_option_set_expected(pair,
                              2);
    (void)d_tests_arg_formatter_vector(_app,
                                       "rest",
                                       "TEXT",
                                       "The rest");
    d_tests_arg_formatter_help(_run,
                               "H03",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H04: flags with values and negations, then, as H04b, without their
 *        values.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h04(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Flags");
    (void)d_arg_command_add_flag(_app,
                                 "--flag,!--no-flag",
                                 "A flag with a negation");
    (void)d_arg_command_add_flag(_app,
                                 "-a{5},--all{7}",
                                 "Flag with values");
    (void)d_tests_arg_formatter_bound_flag(_run,
                                           _app,
                                           "!-O,--opt2,--no_opt2{false}",
                                           D_ARG_BUILTIN_BOOL,
                                           "Something else");
    (void)d_tests_arg_formatter_bound_flag(_run,
                                           _app,
                                           "-v,--verbose",
                                           D_ARG_BUILTIN_INT,
                                           "Count");

    struct d_arg_option* const variable = d_arg_command_add_flag(_app,
                                                                 "--env-flag",
                                                                 "env");

    (void)d_arg_option_set_env_name(variable,
                                    "MY_FLAG");
    d_tests_arg_formatter_help(_run,
                               "H04",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);
    d_arg_formatter_set_flag(&_run->parser.formatter,
                             D_ARG_FORMAT_DEFAULT_FLAG_VALUES,
                             false);
    d_tests_arg_formatter_help(_run,
                               "H04b",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H05: needs, excludes and an environment variable.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h05(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Links");

    struct d_arg_option* const bravo   = d_arg_command_add_flag(_app,
                                                                "-b,--bravo",
                                                                "Bravo");
    struct d_arg_option* const charlie = d_arg_command_add_flag(_app,
                                                                "-c",
                                                                "Charlie");
    struct d_arg_option* const zulu    = d_arg_command_add_flag(_app,
                                                                "--zulu",
                                                                "Zulu");
    struct d_arg_option* const alpha   =
        d_tests_arg_formatter_bound(_run,
                                    _app,
                                    "-a,--alpha",
                                    D_ARG_BUILTIN_STRING,
                                    "Alpha");

    (void)d_arg_option_set_env_name(alpha,
                                    "ALPHA_ENV");
    (void)d_arg_option_needs(alpha,
                             zulu);
    (void)d_arg_option_needs(alpha,
                             bravo);
    (void)d_arg_option_needs(alpha,
                             charlie);

    struct d_arg_option* const delta = d_arg_command_add_flag(_app,
                                                              "--delta",
                                                              "Delta");

    (void)d_arg_option_excludes(delta,
                                bravo);
    (void)d_arg_option_excludes(delta,
                                zulu);
    d_tests_arg_formatter_help(_run,
                               "H05",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

// BASIC_STEPS
//   constant: the cases of d_tests_arg_formatter_basics.
static const d_tests_arg_define_fn BASIC_STEPS[] =
{
    d_tests_arg_formatter_h01,
    d_tests_arg_formatter_h02,
    d_tests_arg_formatter_h03,
    d_tests_arg_formatter_h04,
    d_tests_arg_formatter_h05
};

// BASIC_COUNT
//   constant: how many cases BASIC_STEPS holds.
static const size_t BASIC_COUNT = sizeof(BASIC_STEPS) / sizeof(BASIC_STEPS[0]);

/*
d_tests_arg_formatter_basics
  The help of a root as CLI11's default Formatter writes it.
  Tests the following:
  - a root without a name, and one with a name, a flag and a positional
  - type names, defaults, value counts, required options and positionals
  - flags with values and negations, a bound flag, an environment variable, and
    the flags' values left out
  - needs, excludes and an environment variable
*/
bool
d_tests_arg_formatter_basics(void)
{
    return d_tests_arg_steps("BASICS",
                             BASIC_STEPS,
                             BASIC_COUNT,
                             BASICS);
}

//==============================================================================
// 2.  OPTION TEXT
//==============================================================================

/**
 * @brief H06: option text, type names, validators, delimiters and defaults,
 *        then, as H06b, the defaults left out and, as H06c, the type
 *        names.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h06(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Text");

    struct d_arg_option* const opt =
        d_tests_arg_formatter_bound(_run,
                                    _app,
                                    "--opt",
                                    D_ARG_BUILTIN_INT,
                                    "Something");

    (void)d_arg_option_set_option_text(opt,
                                       "(ARG)");

    struct d_arg_option* const file =
        d_tests_arg_formatter_bound(_run,
                                    _app,
                                    "--file",
                                    D_ARG_BUILTIN_STRING,
                                    "File");

    (void)d_arg_option_set_type_name(file,
                                     "FILE");

    struct d_arg_option* const   range    =
        d_tests_arg_formatter_bound(_run,
                                    _app,
                                    "--range",
                                    D_ARG_BUILTIN_INT,
                                    "In range");
    const struct d_arg_validator in_range = d_arg_validator_range_long(1,
                                                                       10);

    (void)d_arg_option_check(range,
                             in_range,
                             NULL);

    struct d_arg_option* const   both     =
        d_tests_arg_formatter_bound(_run,
                                    _app,
                                    "--r2",
                                    D_ARG_BUILTIN_INT,
                                    "Two checks");
    const struct d_arg_validator again    = d_arg_validator_range_long(1,
                                                                       10);
    const struct d_arg_validator positive = d_arg_validator_positive_number();

    (void)d_arg_option_check(both,
                             again,
                             NULL);
    (void)d_arg_option_check(both,
                             positive,
                             NULL);

    struct d_arg_option* const list = d_tests_arg_formatter_vector(_app,
                                                                   "--list",
                                                                   "TEXT",
                                                                   "Delimited");

    d_arg_option_set_delimiter(list,
                               ',');

    struct d_arg_option* const one =
        d_tests_arg_formatter_bound(_run,
                                    _app,
                                    "--one",
                                    D_ARG_BUILTIN_STRING,
                                    "Delimited single");

    d_arg_option_set_delimiter(one,
                               ';');

    struct d_arg_option* const given =
        d_tests_arg_formatter_bound(_run,
                                    _app,
                                    "--dflt",
                                    D_ARG_BUILTIN_STRING,
                                    "With default");

    (void)d_arg_option_set_default_text(given,
                                        "hello");

    struct d_arg_option* const needed =
        d_tests_arg_formatter_bound(_run,
                                    _app,
                                    "--req",
                                    D_ARG_BUILTIN_STRING,
                                    "Required with default");

    (void)d_arg_option_set_default_text(needed,
                                        "x y");
    d_arg_option_set_required(needed,
                              true);
    (void)d_arg_command_add_option(_app,
                                   "--bare",
                                   "No type at all");
    d_tests_arg_formatter_help(_run,
                               "H06",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);
    d_arg_formatter_set_flag(&_run->parser.formatter,
                             D_ARG_FORMAT_OPTION_DEFAULTS,
                             false);
    d_tests_arg_formatter_help(_run,
                               "H06b",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);
    d_arg_formatter_set_flag(&_run->parser.formatter,
                             D_ARG_FORMAT_OPTION_TYPE_NAMES,
                             false);
    d_tests_arg_formatter_help(_run,
                               "H06c",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H07: groups, hidden options and positionals.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h07(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Groups");

    struct d_arg_option* const first = d_arg_command_add_flag(_app,
                                                              "--first",
                                                              "First");

    (void)d_arg_option_set_group(first,
                                 "Group A");

    struct d_arg_option* const second = d_arg_command_add_flag(_app,
                                                               "--second",
                                                               "Second");

    (void)d_arg_option_set_group(second,
                                 "Group B");

    struct d_arg_option* const third = d_arg_command_add_flag(_app,
                                                              "--third",
                                                              "Third");

    (void)d_arg_option_set_group(third,
                                 "Group A");

    struct d_arg_option* const hidden = d_arg_command_add_flag(_app,
                                                               "--hidden",
                                                               "Hidden");

    (void)d_arg_option_set_group(hidden,
                                 "");

    struct d_arg_option* const pos =
        d_tests_arg_formatter_bound(_run,
                                    _app,
                                    "pos",
                                    D_ARG_BUILTIN_STRING,
                                    "Positional");

    (void)d_arg_option_set_group(pos,
                                 "Group B");

    struct d_arg_option* const hpos =
        d_tests_arg_formatter_bound(_run,
                                    _app,
                                    "hpos",
                                    D_ARG_BUILTIN_STRING,
                                    "Hidden positional");

    (void)d_arg_option_set_group(hpos,
                                 "");
    (void)d_tests_arg_formatter_bound(_run,
                                      _app,
                                      "-b,bothpos",
                                      D_ARG_BUILTIN_STRING,
                                      "Both kinds");
    d_tests_arg_formatter_help(_run,
                               "H07",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H26: a type of two values, and positionals of several.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h26(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Sizes");

    struct d_arg_option* const pair = d_arg_command_add_option(_app,
                                                               "--pair",
                                                               "A pair");

    (void)d_arg_option_set_type_name(pair,
                                     "[INT,INT]");
    d_arg_option_set_type_size(pair,
                               2);

    struct d_arg_option* const pairs = d_tests_arg_formatter_vector(_app,
                                                                    "--pairs",
                                                                    "[INT,INT]",
                                                                    "Pairs");

    d_arg_option_set_type_size(pairs,
                               2);

    struct d_arg_option* const vpos =
        d_tests_arg_formatter_vector(_app,
                                     "vpos",
                                     "INT",
                                     "Vector positional");

    d_arg_option_set_required(vpos,
                              true);

    struct d_arg_option* const wpos =
        d_tests_arg_formatter_vector(_app,
                                     "wpos",
                                     "INT",
                                     "Three of them");

    d_arg_option_set_expected(wpos,
                              3);

    struct d_arg_option* const upos =
        d_tests_arg_formatter_vector(_app,
                                     "upos",
                                     "INT",
                                     "Up to three");

    d_arg_option_set_expected_range(upos,
                                    1,
                                    3);
    d_tests_arg_formatter_help(_run,
                               "H26",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H34: flags required and given default text, which show both, and an
 *        option of no type size, which shows neither its type nor its
 *        default.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h34(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Flag text");

    struct d_arg_option* const must = d_arg_command_add_flag(_app,
                                                             "--must",
                                                             "A required flag");

    d_arg_option_set_required(must,
                              true);

    struct d_arg_option* const given =
        d_arg_command_add_flag(_app,
                               "--dflag",
                               "A flag with default text");

    (void)d_arg_option_set_default_text(given,
                                        "x");

    struct d_arg_option* const zero =
        d_tests_arg_formatter_bound(_run,
                                    _app,
                                    "--zero",
                                    D_ARG_BUILTIN_STRING,
                                    "Of no size");

    d_arg_option_set_type_size(zero,
                               0);
    (void)d_arg_option_set_default_text(zero,
                                        "x");
    d_tests_arg_formatter_help(_run,
                               "H34",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

// OPTION_TEXT_STEPS
//   constant: the cases of d_tests_arg_formatter_option_text.
static const d_tests_arg_define_fn OPTION_TEXT_STEPS[] =
{
    d_tests_arg_formatter_h06,
    d_tests_arg_formatter_h07,
    d_tests_arg_formatter_h26,
    d_tests_arg_formatter_h34
};

// OPTION_TEXT_COUNT
//   constant: how many cases OPTION_TEXT_STEPS holds.
static const size_t OPTION_TEXT_COUNT =
    sizeof(OPTION_TEXT_STEPS) / sizeof(OPTION_TEXT_STEPS[0]);

/*
d_tests_arg_formatter_option_text
  What help shows of an option beside its names.
  Tests the following:
  - option text and type names given, validators' descriptions, delimiters,
    default text, an option of no type; defaults, then type names, left out
  - groups, hidden options and positionals, and an option that is both
  - a type of two values, and positionals of several
  - flags, which show REQUIRED and a default, and an option of no type size,
    which shows neither its type nor its default
*/
bool
d_tests_arg_formatter_option_text(void)
{
    return d_tests_arg_steps("OPTION_TEXT",
                             OPTION_TEXT_STEPS,
                             OPTION_TEXT_COUNT,
                             OPTION_TEXT);
}

//==============================================================================
// 3.  LABELS
//==============================================================================

/**
 * @brief H08: every label set, then, as H08b, the usage word on a root
 *        without a name.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h08(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Labels");
    (void)d_arg_formatter_set_label(&_run->parser.formatter,
                                    "OPTIONS",
                                    "Opts");
    (void)d_arg_formatter_set_label(&_run->parser.formatter,
                                    "REQUIRED",
                                    "(MUST)");
    (void)d_arg_formatter_set_label(&_run->parser.formatter,
                                    "TEXT",
                                    "STRING");
    (void)d_arg_formatter_set_label(&_run->parser.formatter,
                                    "POSITIONALS",
                                    "ARGS");
    (void)d_arg_formatter_set_label(&_run->parser.formatter,
                                    "SUBCOMMANDS",
                                    "COMMANDS");
    (void)d_arg_formatter_set_label(&_run->parser.formatter,
                                    "Env",
                                    "Environment");
    (void)d_arg_formatter_set_label(&_run->parser.formatter,
                                    "Needs",
                                    "Requires");
    (void)d_arg_formatter_set_label(&_run->parser.formatter,
                                    "Excludes",
                                    "Conflicts");
    (void)d_arg_formatter_set_label(&_run->parser.formatter,
                                    "INT:INT in [1 - 10]",
                                    "SMALL");

    struct d_arg_option* const named =
        d_tests_arg_formatter_bound(_run,
                                    _app,
                                    "--name",
                                    D_ARG_BUILTIN_STRING,
                                    "Name");

    d_arg_option_set_required(named,
                              true);
    (void)d_arg_option_set_env_name(named,
                                    "NAME");
    (void)d_tests_arg_formatter_bound(_run,
                                      _app,
                                      "pos",
                                      D_ARG_BUILTIN_STRING,
                                      "Pos");

    struct d_arg_option* const   range    =
        d_tests_arg_formatter_bound(_run,
                                    _app,
                                    "--r",
                                    D_ARG_BUILTIN_INT,
                                    "range");
    const struct d_arg_validator in_range = d_arg_validator_range_long(1,
                                                                       10);

    (void)d_arg_option_check(range,
                             in_range,
                             NULL);

    struct d_arg_option* const x = d_arg_command_add_flag(_app,
                                                          "-x",
                                                          "X");
    struct d_arg_option* const y = d_arg_command_add_flag(_app,
                                                          "-y",
                                                          "Y");

    (void)d_arg_option_needs(y,
                             x);

    struct d_arg_option* const w = d_arg_command_add_flag(_app,
                                                          "-w",
                                                          "W");

    (void)d_arg_option_excludes(w,
                                x);

    struct d_arg_command* const sub = d_arg_command_add_subcommand(_app,
                                                                   "sub",
                                                                   "A sub");

    d_arg_command_set_required(sub,
                               true);
    d_tests_arg_formatter_help(_run,
                               "H08",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);
    (void)d_arg_formatter_set_label(&_run->parser.formatter,
                                    "Usage",
                                    "Run");
    (void)d_arg_command_set_name(_app,
                                 "");
    d_tests_arg_formatter_help(_run,
                               "H08b",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

// LABEL_STEPS
//   constant: the cases of d_tests_arg_formatter_labels.
static const d_tests_arg_define_fn LABEL_STEPS[] =
{
    d_tests_arg_formatter_h08
};

// LABEL_COUNT
//   constant: how many cases LABEL_STEPS holds.
static const size_t LABEL_COUNT = sizeof(LABEL_STEPS) / sizeof(LABEL_STEPS[0]);

/*
d_tests_arg_formatter_labels
  Help's fixed words and type names translated; CLI11's label().
  Tests the following:
  - every word help shows translated, and a type name with its validator's
  - the usage word, after the root loses its name
*/
bool
d_tests_arg_formatter_labels(void)
{
    return d_tests_arg_steps("LABELS",
                             LABEL_STEPS,
                             LABEL_COUNT,
                             LABELS);
}

//==============================================================================
// 4.  WIDTHS
//==============================================================================

/**
 * @brief H09: narrow columns and long names.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h09(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Widths");
    d_arg_formatter_set_column_width(&_run->parser.formatter,
                                     20u);
    d_arg_formatter_set_right_column_width(&_run->parser.formatter,
                                           30u);
    (void)d_arg_command_add_flag(_app,
                                 "-a,--alpha",
                                 "This is a long description that will wrap "
                                 "across several lines of the right column");
    (void)d_arg_command_add_flag(_app,
                                 "--a-very-long-option-name",
                                 "Short desc");
    (void)d_arg_command_add_flag(
        _app,
        "--a-very-long-option-name-2",
        "This is a long description that will wrap across several lines");
    (void)d_arg_command_add_flag(_app,
                                 "--exactly-eighteen1",
                                 "Exactly at the edge");
    (void)d_arg_command_add_flag(_app,
                                 "--exactly-seventeen",
                                 "One short of it");
    (void)d_tests_arg_formatter_bound(
        _run,
        _app,
        "a-long-positional-name",
        D_ARG_BUILTIN_STRING,
        "positional description that is long enough to wrap around");
    (void)d_tests_arg_formatter_bound(
        _run,
        _app,
        "q",
        D_ARG_BUILTIN_STRING,
        "a positional with a short name and a description long enough to wrap");
    (void)d_arg_command_add_subcommand(
        _app,
        "a-very-long-subcommand",
        "subcommand description that is long enough to wrap around the column");
    (void)d_arg_command_add_subcommand(_app,
                                       "exactly-eighteen",
                                       "edge");
    (void)d_arg_command_add_subcommand(
        _app,
        "s",
        "a short subcommand with a long description that wraps across lines");
    d_tests_arg_formatter_help(_run,
                               "H09",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H25: columns too narrow for their words.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h25(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Tiny widths");
    d_arg_formatter_set_column_width(&_run->parser.formatter,
                                     5u);
    d_arg_formatter_set_right_column_width(&_run->parser.formatter,
                                           1u);
    d_arg_formatter_set_description_paragraph_width(&_run->parser.formatter,
                                                    0u);
    (void)d_arg_command_add_flag(_app,
                                 "-a,--alpha",
                                 "words in a column");
    (void)d_arg_command_add_subcommand(_app,
                                       "s",
                                       "sub words");
    (void)d_tests_arg_formatter_bound(_run,
                                      _app,
                                      "p",
                                      D_ARG_BUILTIN_STRING,
                                      "pos words");
    d_tests_arg_formatter_help(_run,
                               "H25",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H25b: a left column of no width.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h25b(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Zero");
    d_arg_formatter_set_column_width(&_run->parser.formatter,
                                     0u);
    (void)d_arg_command_add_flag(_app,
                                 "-a,--alpha",
                                 "words");
    d_tests_arg_formatter_help(_run,
                               "H25b",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H29: an option group's aliases, then, as H29b, in a narrow column.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h29(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Aliased group");

    struct d_arg_command* const aliased =
        d_arg_command_add_option_group(_app,
                                       "Aliased",
                                       "desc");

    (void)d_arg_command_add_alias(aliased,
                                  "al1");
    (void)d_arg_command_add_alias(aliased,
                                  "al2\rx");
    (void)d_arg_command_add_flag(aliased,
                                 "--af",
                                 "flag");
    d_tests_arg_formatter_help(_run,
                               "H29",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);
    d_arg_formatter_set_column_width(&_run->parser.formatter,
                                     4u);
    d_tests_arg_formatter_help(_run,
                               "H29b",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

// WIDTH_STEPS
//   constant: the cases of d_tests_arg_formatter_widths.
static const d_tests_arg_define_fn WIDTH_STEPS[] =
{
    d_tests_arg_formatter_h09,
    d_tests_arg_formatter_h25,
    d_tests_arg_formatter_h25b,
    d_tests_arg_formatter_h29
};

// WIDTH_COUNT
//   constant: how many cases WIDTH_STEPS holds.
static const size_t WIDTH_COUNT = sizeof(WIDTH_STEPS) / sizeof(WIDTH_STEPS[0]);

/*
d_tests_arg_formatter_widths
  The left and right columns' widths: CLI11's column_width and
right_column_width.
  Tests the following:
  - narrow columns: wrapped descriptions, names that reach the edge or pass it,
    subcommands and positionals
  - columns too narrow for anything, and none at all
  - an option group's aliases, at the default width and a narrow one
*/
bool
d_tests_arg_formatter_widths(void)
{
    return d_tests_arg_steps("WIDTHS",
                             WIDTH_STEPS,
                             WIDTH_COUNT,
                             WIDTHS);
}

//==============================================================================
// 5.  ALIGNMENT
//==============================================================================

/**
 * @brief H10a: a ratio of 6.0f / 30.0f.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h10a(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_tests_arg_formatter_ratio(_run,
                                _app,
                                "H10a",
                                6.0f / 30.0f);

    return;
}

/**
 * @brief H10b: a ratio of 0.0f.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h10b(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_tests_arg_formatter_ratio(_run,
                                _app,
                                "H10b",
                                0.0f);

    return;
}

/**
 * @brief H10c: a ratio of 0.5f.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h10c(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_tests_arg_formatter_ratio(_run,
                                _app,
                                "H10c",
                                0.5f);

    return;
}

/**
 * @brief H10d: a ratio of 2.0f.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h10d(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_tests_arg_formatter_ratio(_run,
                                _app,
                                "H10d",
                                2.0f);

    return;
}

/**
 * @brief H10e: a ratio of -0.25f.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h10e(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_tests_arg_formatter_ratio(_run,
                                _app,
                                "H10e",
                                -0.25f);

    return;
}

/**
 * @brief H10f: a ratio of -4.0f.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h10f(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_tests_arg_formatter_ratio(_run,
                                _app,
                                "H10f",
                                -4.0f);

    return;
}

/**
 * @brief H10g: a ratio of 1.0f.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h10g(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_tests_arg_formatter_ratio(_run,
                                _app,
                                "H10g",
                                1.0f);

    return;
}

/**
 * @brief H10h: a ratio of 0.7f.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h10h(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_tests_arg_formatter_ratio(_run,
                                _app,
                                "H10h",
                                0.7f);

    return;
}

// ALIGNMENT_STEPS
//   constant: the cases of d_tests_arg_formatter_alignment.
static const d_tests_arg_define_fn ALIGNMENT_STEPS[] =
{
    d_tests_arg_formatter_h10a,
    d_tests_arg_formatter_h10b,
    d_tests_arg_formatter_h10c,
    d_tests_arg_formatter_h10d,
    d_tests_arg_formatter_h10e,
    d_tests_arg_formatter_h10f,
    d_tests_arg_formatter_h10g,
    d_tests_arg_formatter_h10h
};

// ALIGNMENT_COUNT
//   constant: how many cases ALIGNMENT_STEPS holds.
static const size_t ALIGNMENT_COUNT =
    sizeof(ALIGNMENT_STEPS) / sizeof(ALIGNMENT_STEPS[0]);

/*
d_tests_arg_formatter_alignment
  Where long names start in the left column; CLI11's
long_option_alignment_ratio, given as a fraction, above 1, negative, and at its
ends.
  Tests the following:
  - ratios of 6/30, 0, 1/2, 2, -1/4, -4, 1 and 0.7, over options and flags of
    many short and long names
*/
bool
d_tests_arg_formatter_alignment(void)
{
    return d_tests_arg_steps("ALIGNMENT",
                             ALIGNMENT_STEPS,
                             ALIGNMENT_COUNT,
                             ALIGNMENT);
}

//==============================================================================
// 6.  PARAGRAPHS
//==============================================================================

/**
 * @brief H11a: paragraphs of 50, as plain help and, as H11b, a subcommand's;
 *        unformatted descriptions as H11c and H11d, unformatted footers
 *        as H11e and H11f.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h11a(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "This is       a test of the "
                                        "description systemssssssssssssssssss"
                                        "ssssssssssssssssssssssssssssssssssss"
                                        "sssssssssssss to  Pr e  s  e  r  v  "
                                        "e  SPA C ES");
    (void)d_arg_command_set_footer(_app,
                                   "this is       a test of the footer system"
                                   "sssssssssssssssssssssssssssssssssssssssss"
                                   "ssssssssssssssssssss to  Pr e  s  e  r  "
                                   "v  e  SPA C ES");
    (void)d_arg_command_add_flag(_app,
                                 "--option",
                                 "MyFlag");
    d_arg_formatter_set_description_paragraph_width(&_run->parser.formatter,
                                                    50u);
    d_arg_formatter_set_footer_paragraph_width(&_run->parser.formatter,
                                               50u);
    d_tests_arg_formatter_help(_run,
                               "H11a",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);
    d_tests_arg_formatter_help(_run,
                               "H11b",
                               _app,
                               "",
                               D_ARG_HELP_SUB);
    d_arg_formatter_set_flag(&_run->parser.formatter,
                             D_ARG_FORMAT_DESCRIPTION_PARAGRAPHS,
                             false);
    d_tests_arg_formatter_help(_run,
                               "H11c",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);
    d_tests_arg_formatter_help(_run,
                               "H11d",
                               _app,
                               "",
                               D_ARG_HELP_SUB);
    d_arg_formatter_set_flag(&_run->parser.formatter,
                             D_ARG_FORMAT_FOOTER_PARAGRAPHS,
                             false);
    d_tests_arg_formatter_help(_run,
                               "H11e",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);
    d_tests_arg_formatter_help(_run,
                               "H11f",
                               _app,
                               "",
                               D_ARG_HELP_SUB);

    return;
}

/**
 * @brief H12: line breaks and tabs in text.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h12(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(
        _app,
        "Line one\nLine two\n\nLine four  with\tspaces\n");
    (void)d_arg_command_set_footer(_app,
                                   "Foot one\n\nFoot three\n");
    (void)d_arg_command_add_flag(_app,
                                 "--multi",
                                 "first line\nsecond line that is long "
                                 "enough to need wrapping in the right "
                                 "column\n\nfourth\n");
    (void)d_arg_command_add_flag(_app,
                                 "--tabs",
                                 "\tleading tab and  double  spaces\t");
    (void)d_arg_command_add_flag(_app,
                                 "--spaces",
                                 "   ");
    (void)d_arg_command_add_flag(_app,
                                 "--newline",
                                 "\n");
    d_tests_arg_formatter_help(_run,
                               "H12",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H24: UTF-8 text, counted in bytes.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h24(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Caf\xc3\xa9 \xe2\x80\x94 "
                                        "na\xc3\xafve r\xc3\xa9sum\xc3\xa9 "
                                        "text that goes on and on to see "
                                        "where it wraps when bytes are "
                                        "counted");
    (void)d_arg_command_add_flag(_app,
                                 "--caf\xc3\xa9",
                                 "\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9"
                                 "\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9 "
                                 "\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9"
                                 "\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9 "
                                 "\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9"
                                 "\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9 "
                                 "\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9"
                                 "\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9");
    d_tests_arg_formatter_help(_run,
                               "H24",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

// PARAGRAPH_STEPS
//   constant: the cases of d_tests_arg_formatter_paragraphs.
static const d_tests_arg_define_fn PARAGRAPH_STEPS[] =
{
    d_tests_arg_formatter_h11a,
    d_tests_arg_formatter_h12,
    d_tests_arg_formatter_h24
};

// PARAGRAPH_COUNT
//   constant: how many cases PARAGRAPH_STEPS holds.
static const size_t PARAGRAPH_COUNT =
    sizeof(PARAGRAPH_STEPS) / sizeof(PARAGRAPH_STEPS[0]);

/*
d_tests_arg_formatter_paragraphs
  The description and the footer as paragraphs, and text that holds line breaks,
tabs and UTF-8.
  Tests the following:
  - the description and the footer wrapped as paragraphs, and not, in plain help
    and a subcommand's
  - line breaks and tabs in the description, the footer and an option's
    description
  - UTF-8 text wrapped by its bytes, as CLI11 counts it
*/
bool
d_tests_arg_formatter_paragraphs(void)
{
    return d_tests_arg_steps("PARAGRAPHS",
                             PARAGRAPH_STEPS,
                             PARAGRAPH_COUNT,
                             PARAGRAPHS);
}

//==============================================================================
// 7.  REQUIREMENTS
//==============================================================================

/**
 * @brief H13a: a required root.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h13a(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Req");
    d_arg_command_set_required(_app,
                               true);
    d_tests_arg_formatter_help(_run,
                               "H13a",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H13b: two options required.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h13b(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Req");
    d_arg_command_require_option_count(_app,
                                       2);
    d_tests_arg_formatter_help(_run,
                               "H13b",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H13c: from 1 to 3 options required.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h13c(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Req");
    d_arg_command_require_option(_app,
                                 1u,
                                 3u);
    d_tests_arg_formatter_help(_run,
                               "H13c",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H13d: from 2 to 0 options required.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h13d(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Req");
    d_arg_command_require_option(_app,
                                 2u,
                                 0u);
    d_tests_arg_formatter_help(_run,
                               "H13d",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H13e: from 0 to 3 options required.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h13e(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Req");
    d_arg_command_require_option(_app,
                                 0u,
                                 3u);
    d_tests_arg_formatter_help(_run,
                               "H13e",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H13f: a negative count of options required.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h13f(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Req");
    d_arg_command_require_option_count(_app,
                                       -2);
    d_tests_arg_formatter_help(_run,
                               "H13f",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H13g: a required root of one option, without a description.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h13g(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "");
    d_arg_command_require_option_count(_app,
                                       1);
    d_arg_command_set_required(_app,
                               true);
    d_tests_arg_formatter_help(_run,
                               "H13g",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H13h: a root without a description.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h13h(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "");
    d_tests_arg_formatter_help(_run,
                               "H13h",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H14b: one subcommand required.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h14b(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Subs");
    (void)d_arg_command_add_subcommand(_app,
                                       "one",
                                       "One");
    d_arg_command_require_subcommand_count(_app,
                                           1);
    d_tests_arg_formatter_help(_run,
                               "H14b",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H14c: from 0 to 1 subcommands required.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h14c(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Subs");
    (void)d_arg_command_add_subcommand(_app,
                                       "one",
                                       "One");
    d_arg_command_require_subcommand(_app,
                                     0u,
                                     1u);
    d_tests_arg_formatter_help(_run,
                               "H14c",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H14d: from 2 to 0 subcommands required.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h14d(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Subs");
    (void)d_arg_command_add_subcommand(_app,
                                       "one",
                                       "One");
    d_arg_command_require_subcommand(_app,
                                     2u,
                                     0u);
    d_tests_arg_formatter_help(_run,
                               "H14d",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

// REQUIREMENT_STEPS
//   constant: the cases of d_tests_arg_formatter_requirements.
static const d_tests_arg_define_fn REQUIREMENT_STEPS[] =
{
    d_tests_arg_formatter_h13a,
    d_tests_arg_formatter_h13b,
    d_tests_arg_formatter_h13c,
    d_tests_arg_formatter_h13d,
    d_tests_arg_formatter_h13e,
    d_tests_arg_formatter_h13f,
    d_tests_arg_formatter_h13g,
    d_tests_arg_formatter_h13h,
    d_tests_arg_formatter_h14b,
    d_tests_arg_formatter_h14c,
    d_tests_arg_formatter_h14d
};

// REQUIREMENT_COUNT
//   constant: how many cases REQUIREMENT_STEPS holds.
static const size_t REQUIREMENT_COUNT =
    sizeof(REQUIREMENT_STEPS) / sizeof(REQUIREMENT_STEPS[0]);

/*
d_tests_arg_formatter_requirements
  How the usage line shows what a root requires of its options and subcommands.
  Tests the following:
  - a required root; option counts exact, ranged, open and negative; a root
    without a description
  - subcommand counts exact, ranged and open
*/
bool
d_tests_arg_formatter_requirements(void)
{
    return d_tests_arg_steps("REQUIREMENTS",
                             REQUIREMENT_STEPS,
                             REQUIREMENT_COUNT,
                             REQUIREMENTS);
}

//==============================================================================
// 8.  SUBCOMMANDS
//==============================================================================

/**
 * @brief H14: subcommands of every kind.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h14(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Subs");

    struct d_arg_command* const start =
        d_arg_command_add_subcommand(_app,
                                     "start",
                                     "Start the thing");

    (void)d_arg_command_add_alias(start,
                                  "st");
    (void)d_arg_command_add_alias(start,
                                  "begin");

    struct d_arg_command* const stop =
        d_arg_command_add_subcommand(_app,
                                     "stop",
                                     "Stop the thing");

    d_arg_command_set_required(stop,
                               true);

    struct d_arg_command* const admin1 =
        d_arg_command_add_subcommand(_app,
                                     "admin1",
                                     "Admin one");

    (void)d_arg_command_set_group(admin1,
                                  "Admin");

    struct d_arg_command* const admin2 =
        d_arg_command_add_subcommand(_app,
                                     "admin2",
                                     "Admin two");

    (void)d_arg_command_set_group(admin2,
                                  "admin");

    struct d_arg_command* const hidden = d_arg_command_add_subcommand(_app,
                                                                      "hidden",
                                                                      "Hidden");

    (void)d_arg_command_set_group(hidden,
                                  "");

    struct d_arg_command* const off = d_arg_command_add_subcommand(_app,
                                                                   "off",
                                                                   "Disabled");

    d_arg_command_set_disabled(off,
                               true);
    (void)d_arg_command_add_subcommand(_app,
                                       "nodesc",
                                       NULL);

    struct d_arg_command* const plus =
        d_arg_command_add_subcommand(_app,
                                     "plus",
                                     "Plus group");

    (void)d_arg_command_set_group(plus,
                                  "+Plus");
    d_tests_arg_formatter_help(_run,
                               "H14",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H14e: a lone subcommand, disabled.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h14e(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Subs");

    struct d_arg_command* const one = d_arg_command_add_subcommand(_app,
                                                                   "one",
                                                                   "One");

    d_arg_command_set_disabled(one,
                               true);
    d_tests_arg_formatter_help(_run,
                               "H14e",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H14f: a lone subcommand, hidden.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h14f(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Subs");

    struct d_arg_command* const one = d_arg_command_add_subcommand(_app,
                                                                   "one",
                                                                   "One");

    (void)d_arg_command_set_group(one,
                                  "");
    d_tests_arg_formatter_help(_run,
                               "H14f",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H17: nameless subcommands.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h17(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "My prog");

    struct d_arg_command* const sub =
        d_arg_command_add_subcommand(_app,
                                     "",
                                     "This subcommand");

    (void)d_arg_command_add_flag(sub,
                                 "--insub",
                                 "MyFlag");

    struct d_arg_command* const sub2 =
        d_arg_command_add_subcommand(_app,
                                     "sub2",
                                     "subcommand2");

    (void)d_tests_arg_formatter_bound(_run,
                                      sub2,
                                      "pos",
                                      D_ARG_BUILTIN_INT,
                                      "positional");

    struct d_arg_command* const sub3 = d_arg_command_add_subcommand(_app,
                                                                    "",
                                                                    "Grouped");

    (void)d_arg_command_add_flag(sub3,
                                 "--insub3",
                                 "flag");
    (void)d_arg_command_set_group(sub3,
                                  "group1");
    (void)d_arg_command_set_group(sub2,
                                  "group1");
    d_tests_arg_formatter_help(_run,
                               "H17",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H27a: a subcommand moved into an option group, in plain help and,
 *        as H27b, expanded.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h27a(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Mixed");
    (void)d_arg_command_set_help_all_flag(_app,
                                          "--help-all",
                                          "all");

    struct d_arg_command* const g = d_arg_command_add_option_group(_app,
                                                                   "G",
                                                                   "group g");

    (void)d_arg_command_add_flag(g,
                                 "--gf",
                                 "group flag");

    struct d_arg_command* const gs =
        d_arg_command_add_subcommand(_app,
                                     "gs",
                                     "a sub, later moved into the group");

    (void)d_arg_command_add_flag(gs,
                                 "--gsf",
                                 "f");
    (void)d_arg_command_group_add_subcommand(g,
                                             gs);
    d_tests_arg_formatter_help(_run,
                               "H27a",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);
    d_tests_arg_formatter_help(_run,
                               "H27b",
                               _app,
                               "",
                               D_ARG_HELP_ALL);

    return;
}

/**
 * @brief H28a: requirements and aliases in expanded help, then, as H28b,
 *        plain.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h28a(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Req subs");

    struct d_arg_command* const alpha =
        d_arg_command_add_subcommand(_app,
                                     "alpha",
                                     "Alpha sub");

    d_arg_command_set_required(alpha,
                               true);
    (void)d_arg_command_add_alias(alpha,
                                  "a");
    d_arg_command_require_option_count(alpha,
                                       1);
    (void)d_arg_command_add_flag(alpha,
                                 "--x",
                                 "x");

    struct d_arg_command* const beta = d_arg_command_add_subcommand(_app,
                                                                    "beta",
                                                                    "");

    (void)d_arg_command_add_flag(beta,
                                 "--y",
                                 "y");
    d_tests_arg_formatter_help(_run,
                               "H28a",
                               _app,
                               "",
                               D_ARG_HELP_ALL);
    d_tests_arg_formatter_help(_run,
                               "H28b",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

// SUBCOMMAND_STEPS
//   constant: the cases of d_tests_arg_formatter_subcommands.
static const d_tests_arg_define_fn SUBCOMMAND_STEPS[] =
{
    d_tests_arg_formatter_h14,
    d_tests_arg_formatter_h14e,
    d_tests_arg_formatter_h14f,
    d_tests_arg_formatter_h17,
    d_tests_arg_formatter_h27a,
    d_tests_arg_formatter_h28a
};

// SUBCOMMAND_COUNT
//   constant: how many cases SUBCOMMAND_STEPS holds.
static const size_t SUBCOMMAND_COUNT =
    sizeof(SUBCOMMAND_STEPS) / sizeof(SUBCOMMAND_STEPS[0]);

/*
d_tests_arg_formatter_subcommands
  Subcommands in help: their aliases, groups, requirements and descriptions.
  Tests the following:
  - aliases, a required subcommand, groups that differ in case, a hidden, a
    disabled and an undescribed one, and a group merged
  - a lone subcommand disabled, and hidden
  - nameless subcommands, and one in a group with a named one
  - a subcommand moved into an option group, in plain and expanded help
  - requirements and aliases in expanded help
*/
bool
d_tests_arg_formatter_subcommands(void)
{
    return d_tests_arg_steps("SUBCOMMANDS",
                             SUBCOMMAND_STEPS,
                             SUBCOMMAND_COUNT,
                             SUBCOMMANDS);
}

//==============================================================================
// 9.  EXPANDED
//==============================================================================

/**
 * @brief H15a: help expanded, plain and as a subcommand's, of the root, of a
 *        subcommand and of a deeper one (H15a-H15g).
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h15a(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "All");
    (void)d_arg_command_set_footer(_app,
                                   "Root footer");
    (void)d_arg_command_set_help_all_flag(_app,
                                          "--help-all",
                                          "Expand all help");

    struct d_arg_command* const one = d_arg_command_add_subcommand(_app,
                                                                   "one",
                                                                   "First");

    (void)d_arg_command_add_flag(one,
                                 "--in-one",
                                 "Flag in one");

    struct d_arg_command* const two = d_arg_command_add_subcommand(_app,
                                                                   "two",
                                                                   "Second");

    (void)d_arg_command_set_footer(two,
                                   "Two footer");

    struct d_arg_command* const deep = d_arg_command_add_subcommand(two,
                                                                    "deep",
                                                                    "Deep");

    (void)d_tests_arg_formatter_bound(_run,
                                      deep,
                                      "--depth",
                                      D_ARG_BUILTIN_INT,
                                      "How deep");

    struct d_arg_option* const target =
        d_tests_arg_formatter_bound(_run,
                                    two,
                                    "target",
                                    D_ARG_BUILTIN_STRING,
                                    "Target");

    d_arg_option_set_required(target,
                              true);
    (void)d_arg_command_add_alias(two,
                                  "2");

    struct d_arg_command* const three = d_arg_command_add_subcommand(_app,
                                                                     "three",
                                                                     "Third");

    (void)d_arg_command_set_footer(three,
                                   "");
    (void)d_arg_command_add_flag(_app,
                                 "--top",
                                 "Top flag");
    d_tests_arg_formatter_help(_run,
                               "H15a",
                               _app,
                               "",
                               D_ARG_HELP_ALL);
    d_tests_arg_formatter_help(_run,
                               "H15b",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);
    d_tests_arg_formatter_help(_run,
                               "H15c",
                               _app,
                               "",
                               D_ARG_HELP_SUB);
    d_tests_arg_formatter_help(_run,
                               "H15d",
                               two,
                               "",
                               D_ARG_HELP_SUB);
    d_tests_arg_formatter_help(_run,
                               "H15e",
                               two,
                               "",
                               D_ARG_HELP_ALL);
    d_tests_arg_formatter_help(_run,
                               "H15f",
                               two,
                               "before",
                               D_ARG_HELP_NORMAL);
    d_tests_arg_formatter_help(_run,
                               "H15g",
                               deep,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

// EXPANDED_STEPS
//   constant: the cases of d_tests_arg_formatter_expanded.
static const d_tests_arg_define_fn EXPANDED_STEPS[] =
{
    d_tests_arg_formatter_h15a
};

// EXPANDED_COUNT
//   constant: how many cases EXPANDED_STEPS holds.
static const size_t EXPANDED_COUNT =
    sizeof(EXPANDED_STEPS) / sizeof(EXPANDED_STEPS[0]);

/*
d_tests_arg_formatter_expanded
  Help of everything, and of a subcommand: CLI11's AppFormatMode::All and
AppFormatMode::Sub.
  Tests the following:
  - the root's help expanded, plain and as a subcommand's
  - a subcommand's, as a subcommand's and expanded, after words of the caller's,
    and a deeper one's
  - footers, an alias and a required positional on the way
*/
bool
d_tests_arg_formatter_expanded(void)
{
    return d_tests_arg_steps("EXPANDED",
                             EXPANDED_STEPS,
                             EXPANDED_COUNT,
                             EXPANDED);
}

//==============================================================================
// 10. OPTION GROUPS
//==============================================================================

/**
 * @brief H16a: option groups, in plain and expanded help, and of a group
 *        (H16a-H16d).
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h16a(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "OptGroups");
    (void)d_arg_command_set_help_all_flag(_app,
                                          "--help-all",
                                          "Expand all help");

    struct d_arg_command* const g1 =
        d_arg_command_add_option_group(_app,
                                       "Group1",
                                       "The first group");

    (void)d_arg_command_add_flag(g1,
                                 "--g1a",
                                 "In group 1");
    (void)d_tests_arg_formatter_bound(_run,
                                      g1,
                                      "gpos",
                                      D_ARG_BUILTIN_STRING,
                                      "Group positional");

    struct d_arg_command* const g2 =
        d_arg_command_add_option_group(_app,
                                       "+Merged",
                                       "Merged group");

    (void)d_arg_command_add_flag(g2,
                                 "--merged",
                                 "Merged flag");
    (void)d_tests_arg_formatter_bound(_run,
                                      g2,
                                      "mpos",
                                      D_ARG_BUILTIN_STRING,
                                      "Merged positional");

    struct d_arg_command* const g3 =
        d_arg_command_add_option_group(_app,
                                       "",
                                       "hidden group");

    (void)d_arg_command_add_flag(g3,
                                 "--hid",
                                 "Hidden");

    struct d_arg_command* const g4 =
        d_arg_command_add_option_group(_app,
                                       "Required group",
                                       NULL);

    (void)d_arg_command_add_flag(g4,
                                 "--r1",
                                 NULL);
    (void)d_arg_command_add_flag(g4,
                                 "--r2",
                                 NULL);
    d_arg_command_require_option_count(g4,
                                       1);
    d_arg_command_set_required(g4,
                               true);

    struct d_arg_command* const g5    =
        d_arg_command_add_option_group(_app,
                                       "Nested",
                                       "Outer group");
    struct d_arg_command* const inner =
        d_arg_command_add_option_group(g5,
                                       "Inner",
                                       "Inner group with a description long "
                                       "enough to wrap around the eighty "
                                       "column paragraph width");

    (void)d_arg_command_add_flag(inner,
                                 "--inner",
                                 "Inner flag with a description that is long "
                                 "enough to wrap in the right column of the "
                                 "help");
    (void)d_arg_command_add_flag(g5,
                                 "--outer",
                                 "Outer flag");

    struct d_arg_command* const gsub =
        d_arg_command_add_subcommand(g5,
                                     "gsub",
                                     "A subcommand in a group");

    (void)d_arg_command_add_flag(gsub,
                                 "--gsubflag",
                                 "flag");
    (void)d_arg_command_add_flag(_app,
                                 "--top",
                                 "Top");
    d_tests_arg_formatter_help(_run,
                               "H16a",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);
    d_tests_arg_formatter_help(_run,
                               "H16b",
                               _app,
                               "",
                               D_ARG_HELP_ALL);
    d_tests_arg_formatter_help(_run,
                               "H16c",
                               g1,
                               "",
                               D_ARG_HELP_NORMAL);
    d_tests_arg_formatter_help(_run,
                               "H16d",
                               g4,
                               "",
                               D_ARG_HELP_SUB);

    return;
}

// OPTION_GROUP_STEPS
//   constant: the cases of d_tests_arg_formatter_option_groups.
static const d_tests_arg_define_fn OPTION_GROUP_STEPS[] =
{
    d_tests_arg_formatter_h16a
};

// OPTION_GROUP_COUNT
//   constant: how many cases OPTION_GROUP_STEPS holds.
static const size_t OPTION_GROUP_COUNT =
    sizeof(OPTION_GROUP_STEPS) / sizeof(OPTION_GROUP_STEPS[0]);

/*
d_tests_arg_formatter_option_groups
  Option groups in help: named, merged, hidden, required and nested, with a
subcommand among them.
  Tests the following:
  - groups in plain and expanded help
  - a group's own help, and a required group's as a subcommand's
*/
bool
d_tests_arg_formatter_option_groups(void)
{
    return d_tests_arg_steps("OPTION_GROUPS",
                             OPTION_GROUP_STEPS,
                             OPTION_GROUP_COUNT,
                             OPTION_GROUPS);
}

//==============================================================================
// 11. PARSED
//==============================================================================

/**
 * @brief H18a: help after parses that used a subcommand and a deeper one
 *        (H18a-H18g).
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h18a(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Parsed");

    struct d_arg_command* const sub = d_arg_command_add_subcommand(_app,
                                                                   "sub",
                                                                   "The sub");

    (void)d_arg_command_add_flag(sub,
                                 "--sf",
                                 "Sub flag");

    struct d_arg_command* const deeper = d_arg_command_add_subcommand(sub,
                                                                      "deeper",
                                                                      "Deeper");

    (void)d_arg_command_add_flag(deeper,
                                 "--df",
                                 "Deeper flag");
    (void)d_arg_command_set_help_all_flag(_app,
                                          "--help-all",
                                          NULL);
    (void)d_tests_arg_formatter_parse(_run,
                                      "sub --help");
    d_tests_arg_formatter_help(_run,
                               "H18a",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);
    d_tests_arg_formatter_help(_run,
                               "H18b",
                               _app,
                               "",
                               D_ARG_HELP_ALL);
    (void)d_tests_arg_formatter_parse(_run,
                                      "sub deeper --help");
    d_tests_arg_formatter_help(_run,
                               "H18c",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);
    d_tests_arg_formatter_help(_run,
                               "H18d",
                               _app,
                               "pre",
                               D_ARG_HELP_NORMAL);
    d_tests_arg_formatter_ended(_run,
                                "H18e",
                                D_ARG_CALL_FOR_HELP,
                                false);
    (void)d_tests_arg_formatter_parse(_run,
                                      "sub deeper --help-all");
    d_tests_arg_formatter_ended(_run,
                                "H18f",
                                D_ARG_CALL_FOR_ALL_HELP,
                                false);
    d_arg_command_set_failure_message(_app,
                                      d_arg_failure_help,
                                      NULL,
                                      NULL);
    d_tests_arg_formatter_failed(_run,
                                 "H18g",
                                 "sub --nope");

    return;
}

// PARSED_STEPS
//   constant: the cases of d_tests_arg_formatter_parsed.
static const d_tests_arg_define_fn PARSED_STEPS[] =
{
    d_tests_arg_formatter_h18a
};

// PARSED_COUNT
//   constant: how many cases PARSED_STEPS holds.
static const size_t PARSED_COUNT =
    sizeof(PARSED_STEPS) / sizeof(PARSED_STEPS[0]);

/*
d_tests_arg_formatter_parsed
  Help after a parse: the help of the last subcommand the parse used, and the
help an exit writes.
  Tests the following:
  - a subcommand's help after a parse that used it, plain and expanded
  - a deeper one's, and after words of the caller's
  - help and help of everything from exit()
  - the help failure message after an option not found
*/
bool
d_tests_arg_formatter_parsed(void)
{
    return d_tests_arg_steps("PARSED",
                             PARSED_STEPS,
                             PARSED_COUNT,
                             PARSED);
}

//==============================================================================
// 12. FALLTHROUGH
//==============================================================================

/**
 * @brief H19: the parent's options in a falling-through subcommand's help.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h19(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Fall");
    (void)d_arg_command_add_flag(_app,
                                 "--parent-flag",
                                 "Parent's");
    (void)d_arg_command_add_flag(_app,
                                 "--shared",
                                 "Parent's shared");

    struct d_arg_option* const other = d_arg_command_add_flag(_app,
                                                              "--other",
                                                              "Other group");

    (void)d_arg_option_set_group(other,
                                 "Other");

    struct d_arg_command* const sub = d_arg_command_add_subcommand(_app,
                                                                   "sub",
                                                                   "Sub");

    d_arg_command_set_fallthrough(sub,
                                  true);
    (void)d_arg_command_add_flag(sub,
                                 "--shared",
                                 "Sub's shared");
    d_tests_arg_formatter_help(_run,
                               "H19",
                               sub,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H33: the parent's groups and positionals, beside the subcommand's.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h33(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Fall groups");
    (void)d_arg_command_add_flag(_app,
                                 "--shared",
                                 "Parent's shared");

    struct d_arg_option* const other = d_arg_command_add_flag(_app,
                                                              "--pf",
                                                              "parent other");

    (void)d_arg_option_set_group(other,
                                 "Other");
    (void)d_tests_arg_formatter_bound(_run,
                                      _app,
                                      "ppos",
                                      D_ARG_BUILTIN_STRING,
                                      "parent positional");

    struct d_arg_command* const sub = d_arg_command_add_subcommand(_app,
                                                                   "sub",
                                                                   "Sub");

    d_arg_command_set_fallthrough(sub,
                                  true);

    struct d_arg_option* const shared = d_arg_command_add_flag(sub,
                                                               "--shared",
                                                               "Sub's shared");

    (void)d_arg_option_set_group(shared,
                                 "Other");

    struct d_arg_option* const spos =
        d_tests_arg_formatter_bound(_run,
                                    sub,
                                    "spos",
                                    D_ARG_BUILTIN_STRING,
                                    "sub positional");

    (void)d_arg_option_set_group(spos,
                                 "");
    d_tests_arg_formatter_help(_run,
                               "H33",
                               sub,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

// FALLTHROUGH_STEPS
//   constant: the cases of d_tests_arg_formatter_fallthrough.
static const d_tests_arg_define_fn FALLTHROUGH_STEPS[] =
{
    d_tests_arg_formatter_h19,
    d_tests_arg_formatter_h33
};

// FALLTHROUGH_COUNT
//   constant: how many cases FALLTHROUGH_STEPS holds.
static const size_t FALLTHROUGH_COUNT =
    sizeof(FALLTHROUGH_STEPS) / sizeof(FALLTHROUGH_STEPS[0]);

/*
d_tests_arg_formatter_fallthrough
  A subcommand that falls through to its parent shows the parent's options
beside its own.
  Tests the following:
  - the parent's options, but those the subcommand has by name
  - the parent's groups and positionals beside the subcommand's own
*/
bool
d_tests_arg_formatter_fallthrough(void)
{
    return d_tests_arg_steps("FALLTHROUGH",
                             FALLTHROUGH_STEPS,
                             FALLTHROUGH_COUNT,
                             FALLTHROUGH);
}

//==============================================================================
// 13. USAGE AND FOOTER
//==============================================================================

/**
 * @brief H20a: a usage given.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h20a(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Use");
    (void)d_arg_command_set_usage(_app,
                                  "use: just use it");
    d_tests_arg_formatter_help(_run,
                               "H20a",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H20b: a usage written by a function.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h20b(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Use");
    d_arg_command_set_usage_fn(_app,
                               d_tests_arg_formatter_text,
                               (void*)"use: from a function",
                               NULL);
    d_tests_arg_formatter_help(_run,
                               "H20b",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H20c: a usage written by a function, and given after it.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h20c(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Use");
    d_arg_command_set_usage_fn(_app,
                               d_tests_arg_formatter_text,
                               (void*)"use: first",
                               NULL);
    (void)d_arg_command_set_usage(_app,
                                  "and then");
    d_tests_arg_formatter_help(_run,
                               "H20c",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H20d: a footer written by a function.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h20d(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Use");
    d_arg_command_set_footer_fn(_app,
                                d_tests_arg_formatter_text,
                                (void*)"footer from a function",
                                NULL);
    d_tests_arg_formatter_help(_run,
                               "H20d",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H20e: a footer written by a function, and given after it.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h20e(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Use");
    d_arg_command_set_footer_fn(_app,
                                d_tests_arg_formatter_text,
                                (void*)"first",
                                NULL);
    (void)d_arg_command_set_footer(_app,
                                   " second");
    d_tests_arg_formatter_help(_run,
                               "H20e",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H20f: a usage function that writes nothing.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h20f(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Use");
    d_arg_command_set_usage_fn(_app,
                               d_tests_arg_formatter_text,
                               (void*)"",
                               NULL);
    d_tests_arg_formatter_help(_run,
                               "H20f",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

// USAGE_STEPS
//   constant: the cases of d_tests_arg_formatter_usage_and_footer.
static const d_tests_arg_define_fn USAGE_STEPS[] =
{
    d_tests_arg_formatter_h20a,
    d_tests_arg_formatter_h20b,
    d_tests_arg_formatter_h20c,
    d_tests_arg_formatter_h20d,
    d_tests_arg_formatter_h20e,
    d_tests_arg_formatter_h20f
};

// USAGE_COUNT
//   constant: how many cases USAGE_STEPS holds.
static const size_t USAGE_COUNT = sizeof(USAGE_STEPS) / sizeof(USAGE_STEPS[0]);

/*
d_tests_arg_formatter_usage_and_footer
  Usage and footer text set, and written by a function; CLI11's usage() and
footer().
  Tests the following:
  - a usage given, written by a function, and both
  - a footer written by a function, and with text after it
  - a usage function that writes nothing
*/
bool
d_tests_arg_formatter_usage_and_footer(void)
{
    return d_tests_arg_steps("USAGE_AND_FOOTER",
                             USAGE_STEPS,
                             USAGE_COUNT,
                             USAGE_AND_FOOTER);
}

//==============================================================================
// 14. SPECIAL OPTIONS
//==============================================================================

/**
 * @brief H21: a version flag and a configuration option.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h21(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Ver");
    (void)d_arg_command_set_version_flag(_app,
                                         "-V,--version",
                                         "1.2.3",
                                         NULL);
    (void)d_arg_command_set_config(_app,
                                   "--config",
                                   "default.ini",
                                   "Read an ini file",
                                   false);
    d_tests_arg_formatter_help(_run,
                               "H21",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H21b: a required configuration option.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h21b(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Ver");
    (void)d_arg_command_set_config(_app,
                                   "-c,--cfg",
                                   "",
                                   "Read a file",
                                   true);
    d_tests_arg_formatter_help(_run,
                               "H21b",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H22a: no help flag.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h22a(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Bare");
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);
    d_tests_arg_formatter_help(_run,
                               "H22a",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H22b: no help flag, a positional.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h22b(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Bare");
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);
    (void)d_tests_arg_formatter_bound(_run,
                                      _app,
                                      "p",
                                      D_ARG_BUILTIN_STRING,
                                      "pos");
    d_tests_arg_formatter_help(_run,
                               "H22b",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H22c: no help flag, a hidden flag.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h22c(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Bare");
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);

    struct d_arg_option* const hidden = d_arg_command_add_flag(_app,
                                                               "--h",
                                                               "hidden");

    (void)d_arg_option_set_group(hidden,
                                 "");
    d_tests_arg_formatter_help(_run,
                               "H22c",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H23: deprecated and retired options.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h23(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Old");

    struct d_arg_option* const something =
        d_tests_arg_formatter_bound(_run,
                                    _app,
                                    "--something",
                                    D_ARG_BUILTIN_STRING,
                                    "My option here");

    (void)d_tests_arg_formatter_bound(_run,
                                      _app,
                                      "--something_else",
                                      D_ARG_BUILTIN_STRING,
                                      "My option here");
    (void)d_arg_command_deprecate_option(something,
                                         "--something_else");

    struct d_arg_option* const old =
        d_tests_arg_formatter_bound(_run,
                                    _app,
                                    "--old",
                                    D_ARG_BUILTIN_STRING,
                                    "Old option");

    (void)d_arg_command_retire_option(_app,
                                      old);
    (void)d_arg_command_retire_option_name(_app,
                                           "--gone");
    d_tests_arg_formatter_help(_run,
                               "H23",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

// SPECIAL_STEPS
//   constant: the cases of d_tests_arg_formatter_special_options.
static const d_tests_arg_define_fn SPECIAL_STEPS[] =
{
    d_tests_arg_formatter_h21,
    d_tests_arg_formatter_h21b,
    d_tests_arg_formatter_h22a,
    d_tests_arg_formatter_h22b,
    d_tests_arg_formatter_h22c,
    d_tests_arg_formatter_h23
};

// SPECIAL_COUNT
//   constant: how many cases SPECIAL_STEPS holds.
static const size_t SPECIAL_COUNT =
    sizeof(SPECIAL_STEPS) / sizeof(SPECIAL_STEPS[0]);

/*
d_tests_arg_formatter_special_options
  The version and configuration options, the help flag removed, and deprecated
and retired options.
  Tests the following:
  - a version flag and a configuration option, plain and required
  - no help flag: alone, with a positional, with a hidden flag
  - deprecated and retired options
*/
bool
d_tests_arg_formatter_special_options(void)
{
    return d_tests_arg_steps("SPECIAL_OPTIONS",
                             SPECIAL_STEPS,
                             SPECIAL_COUNT,
                             SPECIAL_OPTIONS);
}

//==============================================================================
// 15. REPLACED STEPS
//==============================================================================

/**
 * @brief H30a: five steps replaced, in plain help and, as H30b, expanded.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h30a(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Custom");

    struct d_arg_formatter* const custom = d_tests_arg_formatter_custom(_run);

    d_arg_command_set_formatter(_app,
                                custom);
    (void)d_arg_command_set_footer(_app,
                                   "ignored");
    (void)d_arg_command_set_help_all_flag(_app,
                                          "--help-all",
                                          "all");
    (void)d_tests_arg_formatter_bound(_run,
                                      _app,
                                      "-n,--num",
                                      D_ARG_BUILTIN_INT,
                                      "A number");
    (void)d_tests_arg_formatter_bound(_run,
                                      _app,
                                      "pos",
                                      D_ARG_BUILTIN_STRING,
                                      "A positional");

    struct d_arg_command* const sub = d_arg_command_add_subcommand(_app,
                                                                   "sub",
                                                                   "A sub");

    (void)d_arg_command_add_flag(sub,
                                 "--sf",
                                 "sub flag");

    struct d_arg_command* const g = d_arg_command_add_option_group(_app,
                                                                   "G",
                                                                   "group");

    (void)d_arg_command_add_flag(g,
                                 "--gf",
                                 "group flag");
    d_tests_arg_formatter_help(_run,
                               "H30a",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);
    d_tests_arg_formatter_help(_run,
                               "H30b",
                               _app,
                               "",
                               D_ARG_HELP_ALL);

    return;
}

/**
 * @brief H31a: the whole of help replaced, with words before the name as H31b
 *        and after a parse as H31c.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h31a(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "Lambda");

    struct d_arg_formatter* const whole = d_tests_arg_formatter_lambda(_run);

    d_arg_command_set_formatter(_app,
                                whole);
    (void)d_arg_command_add_subcommand(_app,
                                       "sub",
                                       "A sub");
    d_tests_arg_formatter_help(_run,
                               "H31a",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);
    d_tests_arg_formatter_help(_run,
                               "H31b",
                               _app,
                               "pre",
                               D_ARG_HELP_ALL);
    (void)d_tests_arg_formatter_parse(_run,
                                      "sub");
    d_tests_arg_formatter_help(_run,
                               "H31c",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief H32a: a subcommand's own formatter, in expanded help and, as H32b,
 *        its own.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_h32a(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "PerSub");
    (void)d_arg_command_set_help_all_flag(_app,
                                          "--help-all",
                                          "all");

    struct d_arg_command* const sub = d_arg_command_add_subcommand(_app,
                                                                   "sub",
                                                                   "A sub");

    (void)d_arg_command_add_flag(sub,
                                 "--sf",
                                 "sub flag");

    struct d_arg_formatter* const custom = d_tests_arg_formatter_custom(_run);

    d_arg_command_set_formatter(sub,
                                custom);
    (void)d_arg_command_add_flag(_app,
                                 "--rf",
                                 "root flag");
    d_tests_arg_formatter_help(_run,
                               "H32a",
                               _app,
                               "",
                               D_ARG_HELP_ALL);
    d_tests_arg_formatter_help(_run,
                               "H32b",
                               sub,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

// REPLACED_STEPS
//   constant: the cases of d_tests_arg_formatter_replaced.
static const d_tests_arg_define_fn REPLACED_STEPS[] =
{
    d_tests_arg_formatter_h30a,
    d_tests_arg_formatter_h31a,
    d_tests_arg_formatter_h32a
};

// REPLACED_COUNT
//   constant: how many cases REPLACED_STEPS holds.
static const size_t REPLACED_COUNT =
    sizeof(REPLACED_STEPS) / sizeof(REPLACED_STEPS[0]);

/*
d_tests_arg_formatter_replaced
  Help built by steps of the caller's: CLI11's Formatter with steps overridden,
a function in place of the whole, and a subcommand's own formatter.
  Tests the following:
  - groups, subcommands, the footer, an option's name and its description
    replaced, in plain and expanded help
  - the whole of help replaced, with words of the caller's, and after a parse
  - a subcommand's own formatter, in its parent's expanded help and its own
*/
bool
d_tests_arg_formatter_replaced(void)
{
    return d_tests_arg_steps("REPLACED",
                             REPLACED_STEPS,
                             REPLACED_COUNT,
                             REPLACED);
}

//==============================================================================
// 16. LINE WIDTH
//==============================================================================

/**
 * @brief L01: help 40 wide.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_l01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(
        _app,
        "alpha beta gamma delta epsilon zeta eta theta");
    (void)d_arg_command_add_flag(
        _app,
        "-a,--alpha",
        "one two three four five six seven eight nine ten");
    (void)d_arg_command_add_flag(_app,
                                 "--a-very-long-option-name",
                                 "short");
    d_arg_formatter_set_line_width(&_run->parser.formatter,
                                   40u);
    d_tests_arg_formatter_help(_run,
                               "L01",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

/**
 * @brief L02: help 20 wide.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_formatter_l02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(
        _app,
        "alpha beta gamma delta epsilon zeta eta theta");
    (void)d_arg_command_add_flag(
        _app,
        "-a,--alpha",
        "one two three four five six seven eight nine ten");
    (void)d_arg_command_add_flag(_app,
                                 "--a-very-long-option-name",
                                 "short");
    d_arg_formatter_set_line_width(&_run->parser.formatter,
                                   20u);
    d_tests_arg_formatter_help(_run,
                               "L02",
                               _app,
                               "",
                               D_ARG_HELP_NORMAL);

    return;
}

// LINE_WIDTH_STEPS
//   constant: the cases of d_tests_arg_formatter_line_width.
static const d_tests_arg_define_fn LINE_WIDTH_STEPS[] =
{
    d_tests_arg_formatter_l01,
    d_tests_arg_formatter_l02
};

// LINE_WIDTH_COUNT
//   constant: how many cases LINE_WIDTH_STEPS holds.
static const size_t LINE_WIDTH_COUNT =
    sizeof(LINE_WIDTH_STEPS) / sizeof(LINE_WIDTH_STEPS[0]);

/*
d_tests_arg_formatter_line_width
  The line width, beyond CLI11: the right column and the paragraphs narrowed so
that help keeps within it.
  Tests the following:
  - help 40 wide, a name that cannot fit kept whole on its line
  - help 20 wide, narrower than the left column: a word to a line
*/
bool
d_tests_arg_formatter_line_width(void)
{
    return d_tests_arg_steps("LINE_WIDTH",
                             LINE_WIDTH_STEPS,
                             LINE_WIDTH_COUNT,
                             LINE_WIDTH);
}

//==============================================================================
// 17. FITTING
//==============================================================================

/**
 * @brief Makes a command's help, plain and expanded, at each line width
 *        from 40 to 79, and says where a line passes the width, but for a
 *        word longer than its room.
 *
 * @param[in,out] _formatter  the command's formatter, its line width left
 *                            at 79.
 * @param[in]     _command    the command.
 * @return whether every help was made, each line within its width.
 */
static bool
d_tests_arg_formatter_each_width(
    struct d_arg_formatter*     _formatter,
    const struct d_arg_command* _command
)
{
    struct d_arg_buffer help = D_ARG_BUFFER_INIT;
    bool                held = true;

    // each width, in plain and in expanded help
    for (size_t width = 40u; width < 80u; ++width)
    {
        d_arg_formatter_set_line_width(_formatter,
                                       width);

        // each mode
        for (int mode = 0; mode <= (int)D_ARG_HELP_ALL; ++mode)
        {
            const int    status  = d_tests_arg_formatter_make(_command,
                                                              mode,
                                                              &help);
            const size_t longest = d_tests_arg_formatter_longest(
                                       d_arg_buffer_text(&help));

            // help not made, or a line past the width
            if ( (status != D_ARG_OK) ||
                 (longest > width) )
            {
                printf("    width %lu, mode %d: status %d, a line of %lu\n",
                       (unsigned long)width,
                       mode,
                       status,
                       (unsigned long)longest);
                held = false;
            }
        }
    }

    d_arg_buffer_release(&help);

    return held;
}

/*
d_tests_arg_formatter_fitting
  The line width, beyond CLI11: help kept within it, the right column and
the paragraphs narrowed, at every width.
  Tests the following:
  - a width wider than every line changes nothing
  - each width from 40 to 79 holds every line of plain and expanded help,
    nested option groups included, but a word longer than its room
  - help leaves the formatter's own widths as they were
*/
bool
d_tests_arg_formatter_fitting(void)
{
    struct d_tests_arg_run      run    = { 0 };
    struct d_arg_command* const app    = d_tests_arg_begin(&run);
    struct d_arg_buffer         plain  = D_ARG_BUFFER_INIT;
    struct d_arg_buffer         fitted = D_ARG_BUFFER_INIT;

    // the root, when it was made
    if (app != NULL)
    {
        d_tests_arg_formatter_build(app);
    }

    const size_t right     = run.parser.formatter.right_column_width;
    const size_t paragraph = run.parser.formatter.description_paragraph_width;

    const int plain_status = d_tests_arg_formatter_make(app,
                                                        D_ARG_HELP_ALL,
                                                        &plain);

    d_arg_formatter_set_line_width(&run.parser.formatter,
                                   500u);

    const int  wide_status = d_tests_arg_formatter_make(app,
                                                        D_ARG_HELP_ALL,
                                                        &fitted);
    const bool same        = d_tests_arg_same("help 500 wide",
                                              d_arg_buffer_text(&fitted),
                                              plain.data);
    const bool wraps       = (d_tests_arg_formatter_longest(
                                  d_arg_buffer_text(&plain)) > 60u);
    const bool widths      = d_tests_arg_formatter_each_width(
                                 &run.parser.formatter,
                                 app);
    const bool untouched   =
        ( (run.parser.formatter.right_column_width == right)              &&
          (run.parser.formatter.description_paragraph_width == paragraph) );

    d_arg_buffer_release(&plain);
    d_arg_buffer_release(&fitted);

    const bool held = ( (app != NULL)                                       &&
                        (plain_status == D_ARG_OK)                          &&
                        (wide_status == D_ARG_OK)                           &&
                        (same)                                              &&
                        (wraps)                                             &&
                        (widths)                                            &&
                        (d_tests_arg_formatter_held(untouched,
                                                    "help changed the "
                                                    "formatter's widths"))  &&
                        (!run.failed) );

    d_tests_arg_end(&run);

    return held;
}

//==============================================================================
// 18. TERMINAL
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_TERMINAL_WIDTH == 1)

/**
 * @brief Compares a command's help with D_ARG_FORMAT_TERMINAL_WIDTH on with
 *        the same help with the switch off and the line width the switch
 *        stands for: one less than d_arg_terminal_width, when that is
 *        narrower than the formatter's own; then turns the switch off.
 *
 * @param[in]     _what       what is compared, for a printed mismatch.
 * @param[in,out] _formatter  the command's formatter.
 * @param[in]     _command    the command.
 * @param[in]     _own        the formatter's own line width; 0 for none.
 * @return whether the two were made, and the same.
 */
static bool
d_tests_arg_formatter_as_terminal(
    const char*                 _what,
    struct d_arg_formatter*     _formatter,
    const struct d_arg_command* _command,
    size_t                      _own
)
{
    struct d_arg_buffer measured = D_ARG_BUFFER_INIT;
    struct d_arg_buffer expected = D_ARG_BUFFER_INIT;
    const size_t        terminal = d_arg_terminal_width();
    const size_t        limit    = (terminal > 0u) ? (terminal - 1u) : 0u;
    const size_t        width    = ( (_own == 0u)    ||
                                     ( (limit > 0u) &&
                                       (limit < _own) ) )
                                   ? limit
                                   : _own;

    d_arg_formatter_set_flag(_formatter,
                             D_ARG_FORMAT_TERMINAL_WIDTH,
                             true);
    d_arg_formatter_set_line_width(_formatter,
                                   _own);

    const int measured_status = d_tests_arg_formatter_make(_command,
                                                           D_ARG_HELP_ALL,
                                                           &measured);

    d_arg_formatter_set_flag(_formatter,
                             D_ARG_FORMAT_TERMINAL_WIDTH,
                             false);
    d_arg_formatter_set_line_width(_formatter,
                                   width);

    const int  expected_status = d_tests_arg_formatter_make(_command,
                                                            D_ARG_HELP_ALL,
                                                            &expected);
    const bool same            = d_tests_arg_same(_what,
                                                  d_arg_buffer_text(&measured),
                                                  expected.data);

    d_arg_formatter_set_line_width(_formatter,
                                   0u);
    d_arg_buffer_release(&measured);
    d_arg_buffer_release(&expected);

    return ( (measured_status == D_ARG_OK) &&
             (expected_status == D_ARG_OK) &&
             (same) );
}

/**
 * @brief The terminal switch on the C checks' root, as COLUMNS changes:
 *        unset, then 60 -- alone and under a narrower width of the
 *        formatter's own -- and the switch off, which COLUMNS cannot reach.
 *
 * @param[in,out] _run  the run, its root built.
 * @return whether every help was made and as expected.
 */
static bool
d_tests_arg_formatter_switching(
    struct d_tests_arg_run* _run
)
{
    struct d_arg_formatter* const     formatter = &_run->parser.formatter;
    const struct d_arg_command* const app       = &_run->parser.command;
    struct d_arg_buffer               plain     = D_ARG_BUFFER_INIT;
    struct d_arg_buffer               off       = D_ARG_BUFFER_INIT;
    const bool                        unset     = d_tests_arg_setenv("COLUMNS",
                                                                     NULL);
    const int                         made      =
        d_tests_arg_formatter_make(app,
                                   D_ARG_HELP_ALL,
                                   &plain);
    const bool                        bare      =
        d_tests_arg_formatter_as_terminal("help, COLUMNS unset",
                                          formatter,
                                          app,
                                          0u);
    const bool                        sixty     = d_tests_arg_setenv("COLUMNS",
                                                                     "60");
    const bool                        measured  =
        d_tests_arg_formatter_as_terminal("help, COLUMNS=60",
                                          formatter,
                                          app,
                                          0u);
    const bool                        narrower  =
        d_tests_arg_formatter_as_terminal("help, COLUMNS=60, 45 wide",
                                          formatter,
                                          app,
                                          45u);
    const int                         remade    =
        d_tests_arg_formatter_make(app,
                                   D_ARG_HELP_ALL,
                                   &off);
    const bool                        ignored   =
        d_tests_arg_same("help, the switch off",
                         d_arg_buffer_text(&off),
                         plain.data);

    d_arg_buffer_release(&plain);
    d_arg_buffer_release(&off);

    return ( (unset)              &&
             (made == D_ARG_OK)   &&
             (bare)               &&
             (sixty)              &&
             (measured)           &&
             (narrower)           &&
             (remade == D_ARG_OK) &&
             (ignored) );
}

/**
 * @brief Checks d_arg_terminal_width's reading of COLUMNS where standard
 *        output is not a terminal -- known by the width following COLUMNS
 *        through two values -- and says so where it is.
 *
 * @return whether every value was read as it should be.
 */
static bool
d_tests_arg_formatter_columns(void)
{
    const bool first  = ( (d_tests_arg_setenv("COLUMNS",
                                              "60")) &&
                          (d_arg_terminal_width() == 60u) );
    const bool second = ( (d_tests_arg_setenv("COLUMNS",
                                              "77")) &&
                          (d_arg_terminal_width() == 77u) );
    bool       held   = true;

    // standard output a terminal, measured rather than read
    if ( (!first) ||
         (!second) )
    {
        printf("    (standard output is a terminal: COLUMNS not read)\n");

        return true;
    }

    // each value, and the width read from it
    for (size_t index = 0u; index < COLUMNS_COUNT; ++index)
    {
        const bool   set   = d_tests_arg_setenv("COLUMNS",
                                                COLUMNS_CASES[index].value);
        const size_t width = d_arg_terminal_width();

        // a value not read as it should be
        if ( (!set) ||
             (width != COLUMNS_CASES[index].width) )
        {
            printf("    COLUMNS=%s: %lu\n",
                   COLUMNS_CASES[index].value,
                   (unsigned long)width);
            held = false;
        }
    }

    return held;
}

/*
d_tests_arg_formatter_terminal
  The terminal's width, beyond CLI11: D_ARG_FORMAT_TERMINAL_WIDTH takes the
line width as one less than the terminal's when that is narrower, and the
terminal's width is COLUMNS where standard output is not a terminal. Each
help is compared with the help of the line width it stands for, so that the
section holds where standard output is a terminal too; COLUMNS is restored
at the end.
  Tests the following:
  - the switch with COLUMNS unset, and set
  - a narrower line width of the formatter's own winning
  - the switch off, COLUMNS changing nothing
  - where standard output is not a terminal, COLUMNS read when it is digits
    alone from 1 to 10000, and ignored otherwise
*/
bool
d_tests_arg_formatter_terminal(void)
{
    struct d_tests_arg_run      run      = { 0 };
    struct d_arg_command* const app      = d_tests_arg_begin(&run);
    const char* const           original = getenv("COLUMNS");
    const bool                  had      = (original != NULL);
    struct d_arg_buffer         saved    = D_ARG_BUFFER_INIT;

    (void)d_arg_buffer_append_cstr(&saved,
                                   (had) ? original : "");

    // the root, when it was made
    if (app != NULL)
    {
        d_tests_arg_formatter_build(app);
    }

    const bool switching = ( (app != NULL) &&
                             (d_tests_arg_formatter_switching(&run)) );
    const bool columns   = d_tests_arg_formatter_columns();
    const bool restored  = d_tests_arg_setenv("COLUMNS",
                                              (had) ? saved.data : NULL);
    const bool held      = ( (switching)   &&
                             (columns)     &&
                             (restored)    &&
                             (!run.failed) &&
                             (saved.failed == 0) );

    d_arg_buffer_release(&saved);
    d_tests_arg_end(&run);

    return held;
}

#endif  // D_INTERNAL_ARG_PARSER_TERMINAL_WIDTH

//==============================================================================
// 19. REFUSALS
//==============================================================================

/**
 * @brief Says which of a set of calls did not refuse with
 *        D_ARG_ERROR_BAD_ARGUMENT.
 *
 * @param[in] _what      what the calls are, for a printed status.
 * @param[in] _statuses  the calls' statuses.
 * @param[in] _count     how many.
 * @return whether every call refused so.
 */
static bool
d_tests_arg_formatter_refused(
    const char* _what,
    const int*  _statuses,
    size_t      _count
)
{
    bool held = true;

    // each call's status
    for (size_t index = 0u; index < _count; ++index)
    {
        // a call that did not refuse
        if (_statuses[index] != (int)D_ARG_ERROR_BAD_ARGUMENT)
        {
            printf("    %s %lu: status %d\n",
                   _what,
                   (unsigned long)(index + 1u),
                   _statuses[index]);
            held = false;
        }
    }

    return held;
}

/**
 * @brief Calls help without a command, and with modes that are not
 *        d_arg_help_mode values.
 *
 * @param[in,out] _run  the run, whose output help would be written to.
 * @return whether every call refused with D_ARG_ERROR_BAD_ARGUMENT.
 */
static bool
d_tests_arg_formatter_help_refusals(
    struct d_tests_arg_run* _run
)
{
    const struct d_arg_command* const app        = &_run->parser.command;
    const struct d_pack_text          empty      = d_pack_text_from_cstr("");
    const struct d_pack_sink          sink       =
        d_arg_buffer_sink(&_run->out);
    const int                         refusals[] =
    {
        d_arg_help(NULL,
                   empty,
                   (int)D_ARG_HELP_NORMAL,
                   sink),
        d_arg_help(app,
                   empty,
                   3,
                   sink),
        d_arg_help(app,
                   empty,
                   -1,
                   sink)
    };

    const size_t count = sizeof(refusals) / sizeof(refusals[0]);

    return d_tests_arg_formatter_refused("help's refusal",
                                         refusals,
                                         count);
}

/**
 * @brief Calls help's steps without a formatter, an option, a list or a
 *        buffer, and with a mode that is not a d_arg_help_mode value.
 *
 * @param[in,out] _run  the run, whose output the steps would write to.
 * @return whether every call refused with D_ARG_ERROR_BAD_ARGUMENT.
 */
static bool
d_tests_arg_formatter_step_refusals(
    struct d_tests_arg_run* _run
)
{
    const struct d_arg_command* const   app        = &_run->parser.command;
    const struct d_arg_formatter* const formatter  = &_run->parser.formatter;
    const struct d_pack_text            empty      = d_pack_text_from_cstr("");
    const int                           refusals[] =
    {
        d_arg_format_help(formatter,
                          app,
                          empty,
                          9,
                          &_run->out),
        d_arg_format_groups(formatter,
                            app,
                            9,
                            &_run->out),
        d_arg_format_help_default(NULL,
                                  app,
                                  empty,
                                  (int)D_ARG_HELP_NORMAL,
                                  &_run->out),
        d_arg_format_option(formatter,
                            NULL,
                            false,
                            &_run->out),
        d_arg_format_group(formatter,
                           d_pack_text_from_cstr("x"),
                           false,
                           NULL,
                           &_run->out),
        d_arg_format_usage(formatter,
                           app,
                           empty,
                           NULL)
    };

    const size_t count = sizeof(refusals) / sizeof(refusals[0]);

    return d_tests_arg_formatter_refused("a step's refusal",
                                         refusals,
                                         count);
}

/*
d_tests_arg_formatter_refusals
  What help refuses, which no CLI11 call can be given, and help that cannot
be written.
  Tests the following:
  - d_arg_help without a command, or with a mode of 3 or -1
  - the steps without a formatter, an option, a list or a buffer, or with a
    mode of 9
  - each refusal D_ARG_ERROR_BAD_ARGUMENT, nothing written
  - a usage function that fails: its status, nothing written
  - a sink that refuses the text: D_ARG_ERROR_FILE
*/
bool
d_tests_arg_formatter_refusals(void)
{
    struct d_tests_arg_run      run   = { 0 };
    struct d_arg_command* const app   = d_tests_arg_begin(&run);
    const struct d_pack_text    empty = d_pack_text_from_cstr("");
    const bool                  bad   =
        ( (app != NULL)                                    &&
          (d_tests_arg_formatter_help_refusals(&run))      &&
          (d_tests_arg_formatter_step_refusals(&run))      &&
          (d_tests_arg_same("output",
                            d_arg_buffer_text(&run.out),
                            NULL)) );

    d_arg_command_set_usage_fn(app,
                               d_tests_arg_formatter_unusable,
                               NULL,
                               NULL);

    const int  unusable = d_arg_help(app,
                                     empty,
                                     (int)D_ARG_HELP_NORMAL,
                                     d_arg_buffer_sink(&run.out));
    const bool quiet    = d_tests_arg_same("output",
                                           d_arg_buffer_text(&run.out),
                                           NULL);

    d_arg_command_set_usage_fn(app,
                               NULL,
                               NULL,
                               NULL);

    const struct d_pack_sink refusing = { d_tests_arg_formatter_refuse,
                                          NULL };
    const int                refused  = d_arg_help(app,
                                                   empty,
                                                   (int)D_ARG_HELP_NORMAL,
                                                   refusing);
    const bool               held     =
        ( (bad)                                &&
          (unusable == D_ARG_ERROR_CONVERSION) &&
          (quiet)                              &&
          (refused == D_ARG_ERROR_FILE)        &&
          (!run.failed) );

    d_tests_arg_end(&run);

    return held;
}

//==============================================================================
// 20. SETTINGS
//==============================================================================

/**
 * @brief A formatter's labels: set, replaced, looked up and refused.
 *
 * @param[in,out] _formatter  an initialized formatter without labels.
 * @return whether every check held.
 */
static bool
d_tests_arg_formatter_labelling(
    struct d_arg_formatter* _formatter
)
{
    const int  first    = d_arg_formatter_set_label(_formatter,
                                                    "REQUIRED",
                                                    "(req)");
    const int  replaced = d_arg_formatter_set_label(_formatter,
                                                    "REQUIRED",
                                                    "(must)");
    const int  no_key   = d_arg_formatter_set_label(_formatter,
                                                    NULL,
                                                    "x");
    const int  no_value = d_arg_formatter_set_label(_formatter,
                                                    "x",
                                                    NULL);
    const bool set      = d_tests_arg_formatter_held(
                              ( (first == D_ARG_OK)                    &&
                                (replaced == D_ARG_OK)                 &&
                                (_formatter->label_keys.count == 1u)   &&
                                (_formatter->label_values.count == 1u) ),
                              "a label set, then replaced, held once");
    const bool found    = d_tests_arg_formatter_held(
                              ( (d_tests_arg_formatter_labelled(_formatter,
                                                                "REQUIRED",
                                                                "(must)")) &&
                                (d_tests_arg_formatter_labelled(_formatter,
                                                                "OTHER",
                                                                "OTHER"))  &&
                                (d_tests_arg_formatter_labelled(NULL,
                                                                "X",
                                                                "X")) ),
                              "a label, a word without one, no formatter");
    const bool refused  = d_tests_arg_formatter_held(
                              ( (no_key == D_ARG_ERROR_BAD_ARGUMENT)   &&
                                (no_value == D_ARG_ERROR_BAD_ARGUMENT) ),
                              "a label without a word or a text");

    return ( (set)   &&
             (found) &&
             (refused) );
}

/**
 * @brief A formatter's settings and labels copied into another, which then
 *        holds copies of its own; a copy into itself and into nothing; and
 *        the copy released back to the defaults.
 *
 * @param[in,out] _source  a formatter labelled by
 *                         d_tests_arg_formatter_labelling; its column width
 *                         is set to 12.
 * @return whether every check held.
 */
static bool
d_tests_arg_formatter_copies(
    struct d_arg_formatter* _source
)
{
    struct d_arg_formatter copy  = { 0 };
    struct d_arg_formatter fresh = { 0 };

    d_arg_formatter_init(&copy);
    d_arg_formatter_init(&fresh);
    d_arg_formatter_set_column_width(_source,
                                     12u);

    const int  labelled = d_arg_formatter_set_label(&copy,
                                                    "A",
                                                    "B");
    const int  copied   = d_arg_formatter_copy(&copy,
                                               _source);
    const bool one      = ( (copy.label_keys.count == 1u)   &&
                            (copy.label_values.count == 1u) );
    const bool own      = d_tests_arg_formatter_held(
                              ( (labelled == D_ARG_OK)          &&
                                (copied == D_ARG_OK)            &&
                                (copy.column_width == 12u)      &&
                                (one)                           &&
                                (copy.label_keys.items !=
                                 _source->label_keys.items)     &&
                                (strcmp(copy.label_values.items[0],
                                        "(must)") == 0) ),
                              "a copy of the settings and the labels");
    const int  onto     = d_arg_formatter_copy(&copy,
                                               &copy);
    const int  nowhere  = d_arg_formatter_copy(NULL,
                                               &copy);
    const bool itself   = d_tests_arg_formatter_held(
                              ( (onto == D_ARG_OK)              &&
                                (copy.label_keys.count == 1u)   &&
                                (nowhere == D_ARG_ERROR_BAD_ARGUMENT) ),
                              "a copy into itself, and into nothing");

    d_arg_formatter_release(&copy);

    const bool released = d_tests_arg_formatter_held(
                              ( (copy.label_keys.count == 0u)             &&
                                (copy.column_width == fresh.column_width) ),
                              "a copy released back to the defaults");

    return ( (own)    &&
             (itself) &&
             (released) );
}

/**
 * @brief The long-name alignment ratio as the setter stores it: one above 1
 *        inverted, a negative one by its magnitude, and one that is not a
 *        number the default.
 *
 * @param[in,out] _formatter  the formatter.
 * @return whether every ratio was stored so.
 */
static bool
d_tests_arg_formatter_ratios(
    struct d_arg_formatter* _formatter
)
{
    struct d_arg_formatter fresh = { 0 };

    d_arg_formatter_init(&fresh);
    d_arg_formatter_set_long_option_alignment_ratio(_formatter,
                                                    4.0f);

    const bool inverted = (_formatter->long_option_alignment_ratio == 0.25f);

    d_arg_formatter_set_long_option_alignment_ratio(_formatter,
                                                    -0.5f);

    const bool magnitude = (_formatter->long_option_alignment_ratio == 0.5f);

    d_arg_formatter_set_long_option_alignment_ratio(_formatter,
                                                    NAN);

    const bool defaulted = (_formatter->long_option_alignment_ratio ==
                            fresh.long_option_alignment_ratio);

    return d_tests_arg_formatter_held(( (inverted)  &&
                                        (magnitude) &&
                                        (defaulted) ),
                                      "ratios of 4, -0.5 and not a number");
}

/**
 * @brief Which formatter a command's help uses: its own, its nearest
 *        ancestor's, its parser's, and none for no command.
 *
 * @param[in,out] _run        the run.
 * @param[in,out] _formatter  a formatter of the caller's, borrowed while
 *                            this runs.
 * @return whether every check held.
 */
static bool
d_tests_arg_formatter_resolution(
    struct d_tests_arg_run* _run,
    struct d_arg_formatter* _formatter
)
{
    struct d_arg_command* const         app     = &_run->parser.command;
    const struct d_arg_formatter* const parser  = &_run->parser.formatter;
    struct d_arg_command* const         sub     =
        d_arg_command_add_subcommand(app,
                                     "sub",
                                     NULL);
    struct d_arg_command* const         deep    =
        d_arg_command_add_subcommand(sub,
                                     "deep",
                                     NULL);
    const bool                          parsers =
        (d_arg_command_formatter(deep) == parser);

    d_arg_command_set_formatter(sub,
                                _formatter);

    const bool ancestors = ( (d_arg_command_formatter(deep) == _formatter) &&
                             (d_arg_command_formatter(sub) == _formatter)  &&
                             (d_arg_command_formatter(app) == parser) );
    const bool nothing   = (d_arg_command_formatter(NULL) == NULL);

    d_arg_command_set_formatter(sub,
                                NULL);

    return ( (d_tests_arg_formatter_held(parsers,
                                         "a subcommand's, its parser's")) &&
             (d_tests_arg_formatter_held(ancestors,
                                         "its own, an ancestor's, and "
                                         "never a child's"))              &&
             (d_tests_arg_formatter_held(nothing,
                                         "no command's")) );
}

/**
 * @brief A ratio written to a formatter's field rather than set, read by
 *        help as the setter would have stored it.
 *
 * @param[in,out] _run        the run.
 * @param[in,out] _formatter  a formatter of the caller's, borrowed while
 *                            this runs.
 * @return whether help placed the long names as the stored ratio would.
 */
static bool
d_tests_arg_formatter_written(
    struct d_tests_arg_run* _run,
    struct d_arg_formatter* _formatter
)
{
    struct d_arg_command* const app  = &_run->parser.command;
    struct d_arg_buffer         help = D_ARG_BUFFER_INIT;

    _formatter->long_option_alignment_ratio = 5.0f;
    d_arg_command_set_formatter(app,
                                _formatter);
    (void)d_arg_command_add_flag(app,
                                 "-z,--zed",
                                 "z");

    const int  made    = d_tests_arg_formatter_make(app,
                                                    D_ARG_HELP_NORMAL,
                                                    &help);
    const bool written = ( (made == D_ARG_OK)                  &&
                           (help.data != NULL)                 &&
                           (strstr(help.data,
                                   "  -z, --zed ") != NULL) );

    d_arg_command_set_formatter(app,
                                NULL);
    d_arg_buffer_release(&help);

    return d_tests_arg_formatter_held(written,
                                      "a ratio of 5 written to the field");
}

/*
d_tests_arg_formatter_settings
  A formatter's own settings, beyond what help shows of them.
  Tests the following:
  - labels set, replaced, looked up and refused
  - a formatter copied, into another, into itself and into nothing, and
    released
  - the alignment ratio as stored: inverted, by its magnitude, defaulted
  - the formatter a command's help uses
  - a ratio written to the field, read as the setter would store it
*/
bool
d_tests_arg_formatter_settings(void)
{
    struct d_tests_arg_run      run       = { 0 };
    struct d_arg_command* const app       = d_tests_arg_begin(&run);
    struct d_arg_formatter      formatter = { 0 };

    d_arg_formatter_init(&formatter);

    const bool labels     = d_tests_arg_formatter_labelling(&formatter);
    const bool copies     = d_tests_arg_formatter_copies(&formatter);
    const bool ratios     = d_tests_arg_formatter_ratios(&formatter);
    const bool resolution = ( (app != NULL)                              &&
                              (d_tests_arg_formatter_resolution(&run,
                                                                &formatter)) &&
                              (d_tests_arg_formatter_written(&run,
                                                             &formatter)) );
    const bool held       = ( (labels)     &&
                              (copies)     &&
                              (ratios)     &&
                              (resolution) &&
                              (!run.failed) );

    d_tests_arg_end(&run);
    d_arg_formatter_release(&formatter);

    return held;
}

//==============================================================================
// 21. THE SUITE
//==============================================================================

// d_tests_arg_formatter
//   constant: the suite, in the header's order.
const struct d_tests_section d_tests_arg_formatter[] =
{
    { "arg_formatter_basics",
      d_tests_arg_formatter_basics },
    { "arg_formatter_option_text",
      d_tests_arg_formatter_option_text },
    { "arg_formatter_labels",
      d_tests_arg_formatter_labels },
    { "arg_formatter_widths",
      d_tests_arg_formatter_widths },
    { "arg_formatter_alignment",
      d_tests_arg_formatter_alignment },
    { "arg_formatter_paragraphs",
      d_tests_arg_formatter_paragraphs },
    { "arg_formatter_requirements",
      d_tests_arg_formatter_requirements },
    { "arg_formatter_subcommands",
      d_tests_arg_formatter_subcommands },
    { "arg_formatter_expanded",
      d_tests_arg_formatter_expanded },
    { "arg_formatter_option_groups",
      d_tests_arg_formatter_option_groups },
    { "arg_formatter_parsed",
      d_tests_arg_formatter_parsed },
    { "arg_formatter_fallthrough",
      d_tests_arg_formatter_fallthrough },
    { "arg_formatter_usage_and_footer",
      d_tests_arg_formatter_usage_and_footer },
    { "arg_formatter_special_options",
      d_tests_arg_formatter_special_options },
    { "arg_formatter_replaced",
      d_tests_arg_formatter_replaced },
    { "arg_formatter_line_width",
      d_tests_arg_formatter_line_width },
    { "arg_formatter_fitting",
      d_tests_arg_formatter_fitting },
#if (D_INTERNAL_ARG_PARSER_TERMINAL_WIDTH == 1)
    { "arg_formatter_terminal",
      d_tests_arg_formatter_terminal },
#endif  // D_INTERNAL_ARG_PARSER_TERMINAL_WIDTH
    { "arg_formatter_refusals",
      d_tests_arg_formatter_refusals },
    { "arg_formatter_settings",
      d_tests_arg_formatter_settings },
};

// d_tests_arg_formatter_count
//   constant: how many sections the table above holds.
const size_t d_tests_arg_formatter_count =
    sizeof(d_tests_arg_formatter) / sizeof(d_tests_arg_formatter[0]);
