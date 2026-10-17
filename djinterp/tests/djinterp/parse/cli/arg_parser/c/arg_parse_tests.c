/*******************************************************************************
* djinterp [parse]                                             arg_parse_tests.c
*
* The suite of the argument parser's parse, declared in arg_parse_tests.h.
*   Each section is a case table (arg_tests_support.h): each case makes its
* options and commands as CLI11's App("d", "prog") would, on a run's parser
* of its own, parses its arguments, and is written as the parse suite's
* CLI11 reference writes it -- a line of its tag and arguments, what its
* callbacks noted, and the parse as the runs report one -- the section's
* cases compared whole. The cases of the parse probes keep their tags
* (P01-P38, Q01-Q18, V01-V06); those after them begin at P39. Q13 and Q14,
* the probes' parses of a vector, are the parser suite's.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_parse_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.10.15
*******************************************************************************/
#include "./arg_parse_tests.h"  // corresponding header
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
// djinterp
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // commands, their settings
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // statuses, modes, policies
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // options, their settings
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator.h"  // d_arg_validator_positive_number
#include "../../../c/tests_section.h"  // d_tests_section
#include "./arg_tests_support.h"  // case tables, notes


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// TOKENS
//   constant: what the cases of d_tests_arg_parsing_tokens come to, as CLI11
// parses them.
static const char TOKENS[] =
    "P01 [--flag -o val a b -- -c --flag]\n"
    "cmd app n=1 miss=[--] subs=[] order=[--flag|--opt|pos|pos|pos|pos]\n"
    "opt flag n=1 res=[true]\n"
    "opt opt n=1 res=[val]\n"
    "opt pos n=4 res=[a|b|-c|--flag]\n"
    "P02 [-ab -cval -bac 7 x]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: x\n"
    "cmd app n=1 miss=[x] subs=[] order=[-a|-b|-c|-b|-a|-c]\n"
    "opt a n=2 res=[true|true]\n"
    "opt b n=2 res=[true|true]\n"
    "opt c n=2 res=[val|7]\n"
    "P17 [--opt]\n"
    "ERR ArgumentMismatch 114: --opt: 1 required  missing\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt opt n=0 res=[]\n"
    "P19 [-5 -.5 -x -.x]\n"
    "cmd app n=1 miss=[-x|-.x] subs=[] order=[n|n]\n"
    "opt n n=2 res=[-5|-.5]\n"
    "P26 [--flag=false --no-flag -vvv --no-flag=0 --flag]\n"
    "cmd app n=1 miss=[] subs=[] order=[--flag|--flag|-v|-v|-v|--flag|--flag]\n"
    "opt flag n=4 res=[false|false|true|true]\n"
    "opt v n=3 res=[true|true|true]\n"
    "P36 [-ao v -oa]\n"
    "cmd app n=1 miss=[] subs=[] order=[-a|-o|-o]\n"
    "opt o n=2 res=[v|a]\n"
    "opt a n=1 res=[true]\n"
    "Q12 [-a-b -a=5 -aa]\n"
    "cmd app n=1 miss=[--b|-=5] subs=[] order=[-a|-a|-a|-a]\n"
    "opt a n=4 res=[true|true|true|true]\n"
    "Q15 [--flag=true --flag=2]\n"
    "ERR ArgumentMismatch 114: flag was given a disallowed flag override\n"
    "cmd app n=1 miss=[] subs=[] order=[--flag]\n"
    "opt flag n=1 res=[true]\n"
    "P39 [--opt --flag]\n"
    "cmd app n=1 miss=[] subs=[] order=[--opt]\n"
    "opt opt n=1 res=[--flag]\n"
    "opt flag n=0 res=[]\n"
    "P40 [-o=v --long= -o  --long x=y]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: \n"
    "cmd app n=1 miss=[] subs=[] order=[-o|--long|--long]\n"
    "opt o n=1 res=[=v]\n"
    "opt long n=2 res=[-o|x=y]\n"
    "P41 [- -- -- --f -]\n"
    "cmd app n=1 miss=[--] subs=[] order=[pos|pos|pos|pos]\n"
    "opt pos n=4 res=[-|--|--f|-]\n"
    "opt f n=0 res=[]\n"
    "P42 [-1 -2 -12 -.1]\n"
    "cmd app n=1 miss=[] subs=[] order=[-1|n|-1|n|n]\n"
    "opt one n=2 res=[true|true]\n"
    "opt n n=3 res=[-2|-2|-.1]\n";

// EXTRAS
//   constant: what the cases of d_tests_arg_parsing_extras come to, as CLI11
// parses them.
static const char EXTRAS[] =
    "P08 [--zzz 1 2 --flag 3 -q 4]\n"
    "cmd app n=1 miss=[--zzz|1|2|3|-q|4] subs=[] order=[--flag]\n"
    "opt flag n=1 res=[true]\n"
    "P09 [--zzz 1 2 --flag -q]\n"
    "cmd app n=1 miss=[--zzz|1|2|-q] subs=[] order=[--flag]\n"
    "opt flag n=1 res=[true]\n"
    "P18 [--f a b]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: a\n"
    "cmd app n=1 miss=[] subs=[] order=[--f]\n"
    "opt f n=1 res=[true]\n"
    "Q10 [x --zz --f -q]\n"
    "cmd app n=1 miss=[] subs=[] order=[--f]\n"
    "cmd s n=0 miss=[] subs=[] order=[]\n"
    "opt f n=1 res=[true]\n"
    "Q03 [++ --f ++]\n"
    "cmd app n=1 miss=[++|++] subs=[] order=[--f]\n"
    "opt f n=1 res=[true]\n"
    "P33 [s x t y s z]\n"
    "cmd app n=1 miss=[] subs=[s|t] order=[]\n"
    "cmd s n=2 miss=[x|z] subs=[] order=[]\n"
    "cmd t n=1 miss=[y] subs=[] order=[]\n"
    "P43 [s --in --top]\n"
    "ERR ExtrasError 109: s: The following argument was not expected: --top\n"
    "cmd app n=1 miss=[] subs=[s] order=[]\n"
    "cmd s n=1 miss=[--top] subs=[] order=[--in]\n"
    "opt top n=0 res=[]\n"
    "opt in n=1 res=[true]\n"
    "P44 [s a --in --zz b]\n"
    "cmd app n=1 miss=[] subs=[s] order=[]\n"
    "cmd s n=1 miss=[a|--zz|b] subs=[] order=[--in]\n"
    "opt in n=1 res=[true]\n"
    "P45 [--zz 1 s --in]\n"
    "cmd app n=1 miss=[--zz|1] subs=[s] order=[]\n"
    "cmd s n=1 miss=[] subs=[] order=[--in]\n"
    "opt in n=1 res=[true]\n"
    "P68 [--zz 1 rest]\n"
    "cmd app n=1 miss=[--zz|1|rest] subs=[] order=[]\n"
    "cmd H n=1 miss=[] subs=[] order=[]\n"
    "cmd s n=0 miss=[] subs=[] order=[]\n"
    "P71 [--zz 1 2 3]\n"
    "cmd app n=1 miss=[--zz|1] subs=[] order=[pos|pos]\n"
    "opt pos n=2 res=[2|3]\n";

// STYLES
//   constant: what the cases of d_tests_arg_parsing_styles come to, as CLI11
// parses them.
static const char STYLES[] =
    "P10 [/opt:val /opt val2 /o:3 /f /unk]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: /unk\n"
    "cmd app n=1 miss=[/unk] subs=[] order=[--opt|--opt|--opt|-f]\n"
    "opt opt n=3 res=[val|val2|3]\n"
    "opt f n=1 res=[true]\n"
    "P11 [-ab=5 -ab 6 -c]\n"
    "cmd app n=1 miss=[] subs=[] order=[-ab|-ab|-c]\n"
    "opt ab n=2 res=[5|6]\n"
    "opt c n=1 res=[true]\n"
    "P25 [/sub.x:5 /sub.y:6 --sub.y=7]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "/sub.x:5\n"
    "cmd app n=3 miss=[/sub.x:5] subs=[sub|sub] order=[]\n"
    "cmd sub n=0 miss=[] subs=[] order=[-y|-y]\n"
    "opt x n=0 res=[]\n"
    "opt y n=2 res=[6|7]\n"
    "P46 [/opt:a:b / /opt: //x /opt=c]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "/opt=c\n"
    "cmd app n=1 miss=[/opt=c] subs=[] order=[--opt|pos|--opt]\n"
    "opt opt n=2 res=[a:b|//x]\n"
    "opt pos n=1 res=[/]\n"
    "P47 [-abc -xy -abcd -yx -abc=2]\n"
    "cmd app n=1 miss=[-abcd] subs=[] order=[-abc|-x|-y|-y|-x|-abc]\n"
    "opt abc n=2 res=[true|2]\n"
    "opt x n=2 res=[true|true]\n"
    "opt y n=2 res=[true|true]\n";

// SUBCOMMANDS
//   constant: what the cases of d_tests_arg_parsing_subcommands come to, as
// CLI11 parses them.
static const char SUBCOMMANDS[] =
    "P03 [sub1 --x 1 ++ --flag sub1 --x 2]\n"
    "cmd app n=1 miss=[] subs=[sub1] order=[--flag]\n"
    "cmd sub1 n=2 miss=[] subs=[] order=[--x|--x]\n"
    "opt flag n=1 res=[true]\n"
    "opt x n=2 res=[1|2]\n"
    "P06 [sub --flag a --own 1]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: a\n"
    "cmd app n=1 miss=[a] subs=[sub] order=[--flag]\n"
    "cmd sub n=1 miss=[] subs=[] order=[--own]\n"
    "opt flag n=1 res=[true]\n"
    "opt own n=1 res=[1]\n"
    "P21 [sub --x 1 sub --x 2]\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=2 miss=[] subs=[] order=[--x]\n"
    "opt x n=1 res=[2]\n"
    "P23 [a b]\n"
    "cmd app n=1 miss=[] subs=[a] order=[]\n"
    "cmd a n=1 miss=[b] subs=[] order=[]\n"
    "cmd b n=0 miss=[] subs=[] order=[]\n"
    "Q02 [sub other]\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[other] subs=[] order=[]\n"
    "cmd other n=0 miss=[] subs=[] order=[]\n"
    "Q04 [su o]\n"
    "cmd app n=1 miss=[] subs=[sub|other] order=[]\n"
    "cmd sub n=1 miss=[] subs=[] order=[]\n"
    "cmd other n=1 miss=[] subs=[] order=[]\n"
    "Q05 [SUB dis quiet]\n"
    "ERR ExtrasError 109: Sub: The following argument was not expected: dis\n"
    "cmd app n=1 miss=[] subs=[Sub] order=[]\n"
    "cmd Sub n=1 miss=[dis] subs=[] order=[]\n"
    "cmd dis n=0 miss=[] subs=[] order=[]\n"
    "cmd quiet n=1 miss=[] subs=[] order=[]\n"
    "Q06 [a x a y]\n"
    "cmd app n=1 miss=[x|y] subs=[a] order=[]\n"
    "cmd a n=2 miss=[] subs=[] order=[]\n"
    "cmd b n=0 miss=[] subs=[] order=[]\n"
    "Q07 [a x a y]\n"
    "cmd app n=1 miss=[x|y] subs=[a] order=[]\n"
    "cmd a n=2 miss=[] subs=[] order=[]\n"
    "cmd b n=0 miss=[] subs=[] order=[]\n"
    "Q11 [s -- v w]\n"
    "cmd app n=1 miss=[] subs=[s] order=[]\n"
    "cmd s n=1 miss=[--|w] subs=[] order=[p]\n"
    "opt p n=1 res=[v]\n"
    "Q17 [s t --deep --top ++ ++ --top]\n"
    "cmd app n=1 miss=[] subs=[s] order=[--top|--top]\n"
    "cmd s n=1 miss=[] subs=[t] order=[]\n"
    "cmd t n=1 miss=[] subs=[] order=[--deep]\n"
    "opt deep n=1 res=[true]\n"
    "opt top n=2 res=[true|true]\n"
    "P48 [s2 --x 1 third sub]\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=3 miss=[] subs=[] order=[--x]\n"
    "opt x n=1 res=[1]\n"
    "P49 [su sup s]\n"
    "cmd app n=1 miss=[su] subs=[super|sub] order=[]\n"
    "cmd sub n=1 miss=[] subs=[] order=[]\n"
    "cmd super n=1 miss=[] subs=[] order=[]\n"
    "P50 [x sub y -- sub]\n"
    "cmd app n=1 miss=[] subs=[sub] order=[pos|pos]\n"
    "cmd sub n=1 miss=[y] subs=[] order=[]\n"
    "opt pos n=2 res=[x|sub]\n";

// DOTS
//   constant: what the cases of d_tests_arg_parsing_dots come to, as CLI11
// parses them.
static const char DOTS[] =
    "P04 [--sub1.x=5 --sub1.v 6 sub1.sub2 --y 7]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--sub1.x=5\n"
    "cmd app n=2 miss=[--sub1.x=5] subs=[sub1|sub1] order=[]\n"
    "cmd sub1 n=1 miss=[] subs=[sub2] order=[-v]\n"
    "cmd sub2 n=1 miss=[] subs=[] order=[--y]\n"
    "opt x n=0 res=[]\n"
    "opt v n=1 res=[6]\n"
    "opt y n=1 res=[7]\n"
    "P38 [--sub.a --sub.b --sub. --.x]\n"
    "cmd app n=1 miss=[--sub.a|--sub.b|--sub.|--.x] subs=[] order=[]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt a n=0 res=[]\n"
    "Q18 [--s.v=1 --t.x t]\n"
    "cmd app n=1 miss=[--s.v=1|--t.x] subs=[t] order=[]\n"
    "cmd s n=0 miss=[] subs=[] order=[]\n"
    "cmd t n=1 miss=[] subs=[] order=[]\n"
    "opt v n=0 res=[]\n"
    "P51 [--sub.y 1 --sub.y=2 --sub.f --sub.f=3 --sub.y]\n"
    "ERR ArgumentMismatch 114: -y: 1 required  missing\n"
    "cmd app n=3 miss=[--sub.f|--sub.f=3] subs=[sub|sub] order=[]\n"
    "cmd sub n=0 miss=[] subs=[] order=[-y|-y]\n"
    "opt y n=2 res=[1|2]\n"
    "opt f n=0 res=[]\n"
    "P52 [--sub.xx a b c --sub.x d --sub.xx=e f]\n"
    "cmd app n=3 miss=[c|--sub.x|d] subs=[sub|sub] order=[]\n"
    "cmd sub n=0 miss=[] subs=[] order=[--xx|--xx|--xx|--xx]\n"
    "opt xx n=4 res=[a|b|e|f]\n"
    "opt x n=0 res=[]\n"
    "P53 [--SUB.aa --t.bb --t.bb t.x --sub.aa]\n"
    "cmd app n=5 miss=[t.x] subs=[Sub|t|t|Sub] order=[]\n"
    "cmd Sub n=0 miss=[] subs=[] order=[--aa|--aa]\n"
    "cmd t n=0 miss=[] subs=[] order=[--bb|--bb]\n"
    "opt aa n=2 res=[true|true]\n"
    "opt bb n=2 res=[true|true]\n"
    "P69 [--sub.=x --sub.y= z --sub.aa= --sub..aa --sub.-y]\n"
    "cmd app n=3 miss=[--sub.=x|--sub..aa|--sub.-y] subs=[sub|sub] order=[]\n"
    "cmd sub n=0 miss=[] subs=[] order=[-y|--aa]\n"
    "opt y n=1 res=[z]\n"
    "opt aa n=1 res=[true]\n"
    "P70 [--s.vv=1 --t.ww=2 --s.vv=3]\n"
    "cmd app n=3 miss=[--t.ww=2] subs=[s|s] order=[]\n"
    "cmd s n=0 miss=[] subs=[] order=[--vv|--vv]\n"
    "cmd t n=0 miss=[] subs=[] order=[]\n"
    "opt vv n=2 res=[1|3]\n"
    "opt ww n=0 res=[]\n";

// GROUPS
//   constant: what the cases of d_tests_arg_parsing_groups come to, as CLI11
// parses them.
static const char GROUPS[] =
    "P05 [--g 3 rest --zz --f]\n"
    "cmd app n=1 miss=[] subs=[] order=[--f]\n"
    "cmd G n=1 miss=[] subs=[] order=[--g]\n"
    "cmd H n=1 miss=[rest|--zz] subs=[] order=[]\n"
    "opt g n=1 res=[3]\n"
    "opt f n=1 res=[true]\n"
    "P24 [gs --q]\n"
    "cmd app n=1 miss=[] subs=[gs] order=[]\n"
    "cmd G n=1 miss=[] subs=[gs] order=[]\n"
    "cmd gs n=1 miss=[] subs=[] order=[--q]\n"
    "opt q n=1 res=[true]\n"
    "P37 [sub --gopt 1 q]\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[] subs=[] order=[]\n"
    "cmd SG n=1 miss=[] subs=[] order=[--gopt|gpos]\n"
    "opt gopt n=1 res=[1]\n"
    "opt gpos n=1 res=[q]\n"
    "Q16 [--ho 1 q r]\n"
    "cmd app n=1 miss=[r] subs=[] order=[]\n"
    "cmd G n=1 miss=[] subs=[] order=[]\n"
    "cmd H n=1 miss=[] subs=[] order=[--ho|hp]\n"
    "opt hp n=1 res=[q]\n"
    "opt ho n=1 res=[1]\n"
    "P54 [a b c]\n"
    "cmd app n=1 miss=[] subs=[] order=[first|last]\n"
    "cmd G n=1 miss=[] subs=[] order=[gp]\n"
    "opt first n=1 res=[a]\n"
    "opt gp n=1 res=[c]\n"
    "opt last n=1 res=[b]\n"
    "P55 [--gf s --sf --gf]\n"
    "ERR ExtrasError 109: s: The following argument was not expected: --gf\n"
    "cmd app n=1 miss=[] subs=[s] order=[]\n"
    "cmd G n=1 miss=[] subs=[] order=[--gf]\n"
    "cmd s n=1 miss=[--gf] subs=[] order=[--sf]\n"
    "opt gf n=1 res=[true]\n"
    "opt sf n=1 res=[true]\n";

// PREFIX_COMMANDS
//   constant: what the cases of d_tests_arg_parsing_prefix_commands come to, as
// CLI11 parses them.
static const char PREFIX_COMMANDS[] =
    "P07 [--flag x --unknown y -- z]\n"
    "cmd app n=1 miss=[x|--unknown|y|--|z] subs=[] order=[--flag]\n"
    "opt flag n=1 res=[true]\n"
    "P32 [s --f u -- --f]\n"
    "ERR ExtrasError 109: s: The following arguments were not expected: u -- "
    "--f\n"
    "cmd app n=1 miss=[] subs=[s] order=[]\n"
    "cmd s n=1 miss=[u|--|--f] subs=[] order=[--f]\n"
    "opt f n=1 res=[true]\n"
    "Q01 [--flag --unk p --x --flag]\n"
    "cmd app n=1 miss=[--unk|p|--x|--flag] subs=[] order=[--flag]\n"
    "opt flag n=1 res=[true]\n"
    "P56 [--top s --f rest --f --top]\n"
    "cmd app n=1 miss=[] subs=[s] order=[--top]\n"
    "cmd s n=1 miss=[rest|--f|--top] subs=[] order=[--f]\n"
    "opt f n=1 res=[true]\n"
    "opt top n=1 res=[true]\n"
    "P57 [a b --flag]\n"
    "cmd app n=1 miss=[b|--flag] subs=[] order=[p]\n"
    "opt p n=1 res=[a]\n"
    "opt flag n=0 res=[]\n"
    "P58 [-- sub --flag]\n"
    "cmd app n=1 miss=[--|sub|--flag] subs=[] order=[]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt flag n=0 res=[]\n"
    "P72 [--unknown --flag x]\n"
    "cmd app n=1 miss=[--unknown|--flag|x] subs=[] order=[]\n"
    "opt flag n=0 res=[]\n";

// VALUES
//   constant: what the cases of d_tests_arg_parsing_values come to, as CLI11
// parses them.
static const char VALUES[] =
    "P13 [--v a b --v c --f]\n"
    "cmd app n=1 miss=[] subs=[] order=[--v|--v|--v|--f]\n"
    "opt v n=4 res=[a|b||c]\n"
    "opt f n=1 res=[true]\n"
    "P27 [--o --f --o x --o]\n"
    "cmd app n=1 miss=[] subs=[] order=[--o|--f|--o|--o]\n"
    "opt o n=3 res=[|x|]\n"
    "opt f n=1 res=[true]\n"
    "P28 [--pair 1]\n"
    "ERR ArgumentMismatch 114: --pair: 2 required  missing\n"
    "cmd app n=1 miss=[] subs=[] order=[--pair]\n"
    "opt pair n=1 res=[1]\n"
    "P29 [--pair 1 2 --pair 3 --f]\n"
    "cmd app n=1 miss=[] subs=[] order=[--pair|--pair|--pair|--f]\n"
    "opt pair n=4 res=[1|2|3|]\n"
    "opt f n=1 res=[true]\n"
    "P31 [--o 1 2 3 4]\n"
    "cmd app n=1 miss=[] subs=[] order=[--o|--o|a|b]\n"
    "opt a n=1 res=[3]\n"
    "opt o n=2 res=[1|2]\n"
    "opt b n=1 res=[4]\n"
    "P34 [--v 1 2 x 3]\n"
    "cmd app n=1 miss=[x|3] subs=[] order=[--v|--v]\n"
    "opt v n=2 res=[1|2]\n"
    "P35 [--v 1 2 3]\n"
    "cmd app n=1 miss=[] subs=[] order=[--v|--v|p]\n"
    "opt v n=2 res=[1|2]\n"
    "opt p n=1 res=[3]\n"
    "P59 [--two a b c --two d]\n"
    "cmd app n=1 miss=[] subs=[] order=[--two|--two|pos|--two]\n"
    "opt two n=3 res=[a|b|d]\n"
    "opt pos n=1 res=[c]\n"
    "P60 [--v 1 --f 2 --v 3 sub 4]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: 2\n"
    "cmd app n=1 miss=[2] subs=[sub] order=[--v|--f|--v]\n"
    "cmd sub n=1 miss=[] subs=[] order=[p]\n"
    "opt v n=2 res=[1|3]\n"
    "opt f n=1 res=[true]\n"
    "opt p n=1 res=[4]\n"
    "P61 [--r 1 2 3 4 --r 5 --f --r 6 -x]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: -x\n"
    "cmd app n=1 miss=[-x] subs=[] order=[--r|--r|--r|pos|--r|--f|--r]\n"
    "opt r n=5 res=[1|2|3|5|6]\n"
    "opt f n=1 res=[true]\n"
    "opt pos n=1 res=[4]\n"
    "P73 [--a 1 2 --b 3 4 --c 5 6 7]\n"
    "cmd app n=1 miss=[] subs=[] order=[--a|pos|--b|--b|--c|--c|pos]\n"
    "opt a n=1 res=[1]\n"
    "opt b n=2 res=[3|4]\n"
    "opt c n=2 res=[5|6]\n"
    "opt pos n=2 res=[2|7]\n"
    "P74 [--v 1 -- 2 --one 3 -- 4]\n"
    "cmd app n=1 miss=[--] subs=[] order=[--v|pos|--one|pos]\n"
    "opt v n=1 res=[1]\n"
    "opt one n=1 res=[3]\n"
    "opt pos n=2 res=[2|4]\n";

// POSITIONALS
//   constant: what the cases of d_tests_arg_parsing_positionals come to, as
// CLI11 parses them.
static const char POSITIONALS[] =
    "P14 [x]\n"
    "cmd app n=1 miss=[] subs=[] order=[b]\n"
    "opt a n=0 res=[]\n"
    "opt b n=1 res=[x]\n"
    "P15 [x y z w]\n"
    "ERR ExtrasError 109: prog: The following arguments were not expected: z "
    "w\n"
    "cmd app n=1 miss=[] subs=[] order=[a|b]\n"
    "opt a n=1 res=[x]\n"
    "opt b n=1 res=[y]\n"
    "P16 [abc 12]\n"
    "cmd app n=1 miss=[] subs=[] order=[word|num]\n"
    "opt num n=1 res=[12]\n"
    "opt word n=1 res=[abc]\n"
    "P30 [x]\n"
    "cmd app n=1 miss=[] subs=[] order=[a]\n"
    "opt a n=1 res=[x]\n"
    "opt b n=0 res=[]\n"
    "Q08 [x --f]\n"
    "cmd app n=1 miss=[] subs=[] order=[pos|--f]\n"
    "opt pos n=1 res=[x]\n"
    "opt f n=1 res=[true]\n"
    "P62 [a b c]\n"
    "cmd app n=1 miss=[] subs=[] order=[pair|pair|last]\n"
    "opt pair n=2 res=[a|b]\n"
    "opt last n=1 res=[c]\n"
    "P63 [a]\n"
    "cmd app n=1 miss=[] subs=[] order=[pair]\n"
    "opt pair n=1 res=[a]\n"
    "P64 [w]\n"
    "ERR RequiredError 106: n is required\n"
    "cmd app n=1 miss=[] subs=[] order=[a]\n"
    "opt a n=1 res=[w]\n"
    "opt n n=0 res=[]\n"
    "P65 [x y]\n"
    "cmd app n=1 miss=[] subs=[] order=[b|b]\n"
    "opt a n=0 res=[]\n"
    "opt b n=2 res=[x|y]\n";

// CHECKS
//   constant: what the cases of d_tests_arg_parsing_checks come to, as CLI11
// parses them.
static const char CHECKS[] =
    "V01 [x]\n"
    "ERR InvalidError 111: prog: Too many positional arguments with unlimited "
    "expected args\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt a n=0 res=[]\n"
    "opt b n=0 res=[]\n"
    "V02 [x y]\n"
    "cmd app n=1 miss=[] subs=[] order=[a|a]\n"
    "opt a n=2 res=[x|y]\n"
    "opt b n=0 res=[]\n"
    "V03 [--f]\n"
    "ERR InvalidError 111: Required min options greater than number of "
    "available options\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt f n=0 res=[]\n"
    "V04 [--f]\n"
    "ERR InvalidError 111: Required min options greater than required max "
    "options\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt f n=0 res=[]\n"
    "opt g n=0 res=[]\n"
    "V05 [sub]\n"
    "ERR InvalidError 111: sub: Too many positional arguments with unlimited "
    "expected args\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt a n=0 res=[]\n"
    "opt b n=0 res=[]\n"
    "V06 [--f --gf]\n"
    "cmd app n=1 miss=[] subs=[] order=[--f]\n"
    "cmd G n=1 miss=[] subs=[] order=[--gf]\n"
    "opt gf n=1 res=[true]\n"
    "opt f n=1 res=[true]\n";

// CALLBACKS
//   constant: what the cases of d_tests_arg_parsing_callbacks come to, as CLI11
// parses them.
static const char CALLBACKS[] =
    "P12 [--t 1 --t 2]\n"
    "T:1\n"
    "T:2\n"
    "cmd app n=1 miss=[] subs=[] order=[--t|--t]\n"
    "opt t n=1 res=[2]\n"
    "P20 [sub b c]\n"
    "PRE app 3\n"
    "PRE sub 2\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[] subs=[] order=[p|p]\n"
    "opt p n=2 res=[b|c]\n"
    "Q09 [a , b,c]\n"
    "T:a\n"
    "T:a\n"
    "cmd app n=1 miss=[] subs=[] order=[p|p|p]\n"
    "opt p n=3 res=[a|b|c]\n"
    "P66 [x --sub.aa y]\n"
    "PRE app 3\n"
    "cmd app n=2 miss=[x|y] subs=[sub] order=[]\n"
    "cmd sub n=0 miss=[] subs=[] order=[--aa]\n"
    "opt aa n=1 res=[true]\n"
    "P67 [sub t x sub]\n"
    "DONE sub\n"
    "DONE sub\n"
    "DONE app\n"
    "FINAL sub\n"
    "FINAL t\n"
    "FINAL app\n"
    "cmd app n=1 miss=[] subs=[sub|t] order=[]\n"
    "cmd sub n=2 miss=[] subs=[] order=[]\n"
    "cmd t n=1 miss=[x] subs=[] order=[]\n";

// MANY
//   constant: the fewest values CLI11 takes as many -- a sixteenth of
// the unlimited count -- at which an option that takes no extra
// arguments takes only one item's values at each use.
enum
{
    MANY = D_ARG_EXPECTED_UNLIMITED / 16
};

//==============================================================================
// HELPERS
//==============================================================================

/**
 * @brief An option's callback that notes its first value, `T:<value>`, as
 *        the reference's callbacks print the std::string CLI11 converts the
 *        values to.
 *
 * @param[in] _option   the option; unused.
 * @param[in] _results  its values.
 * @param[in] _count    how many.
 * @param[in] _context  the case's run.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_parsing_first(
    struct d_arg_option*      _option,
    const struct d_pack_text* _results,
    size_t                    _count,
    void*                     _context
)
{
    const struct d_pack_text first = (_count > 0u)
                                     ? _results[0]
                                     : d_pack_text_from_cstr("");

    (void)_option;
    d_tests_arg_note(_context,
                     "T:%.*s",
                     (int)first.length,
                     first.data);

    return D_ARG_OK;
}

/**
 * @brief A pre-parse callback that notes `PRE <label> <arguments left>`,
 *        the label the one the run reports the command under.
 *
 * @param[in] _command    the command.
 * @param[in] _remaining  the arguments left.
 * @param[in] _context    the case's run.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_parsing_preparse(
    struct d_arg_command* _command,
    size_t                _remaining,
    void*                 _context
)
{
    d_tests_arg_note(_context,
                     "PRE %s %lu",
                     d_tests_arg_command_label(_context,
                                               _command),
                     (unsigned long)_remaining);

    return D_ARG_OK;
}

/**
 * @brief A parse-complete callback that notes `DONE <label>`.
 *
 * @param[in] _command  the command.
 * @param[in] _context  the case's run.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_parsing_done(
    struct d_arg_command* _command,
    void*                 _context
)
{
    d_tests_arg_note(_context,
                     "DONE %s",
                     d_tests_arg_command_label(_context,
                                               _command));

    return D_ARG_OK;
}

/**
 * @brief A final callback that notes `FINAL <label>`.
 *
 * @param[in] _command  the command.
 * @param[in] _context  the case's run.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_parsing_final(
    struct d_arg_command* _command,
    void*                 _context
)
{
    d_tests_arg_note(_context,
                     "FINAL %s",
                     d_tests_arg_command_label(_context,
                                               _command));

    return D_ARG_OK;
}

/**
 * @brief A parse-complete callback that does nothing, as the reference's
 *        empty lambda.
 *
 * @param[in] _command  the command; unused.
 * @param[in] _context  unused.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_parsing_nothing(
    struct d_arg_command* _command,
    void*                 _context
)
{
    (void)_command;
    (void)_context;

    return D_ARG_OK;
}

//==============================================================================
// 1.  TOKENS
//==============================================================================

/**
 * @brief P01: a flag and an option, then a positional taking every value
 *        and `--` ending the options.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_flag(_run,
                                "flag",
                                _app,
                                "--flag,-f");
    (void)d_tests_arg_make_option(_run,
                                  "opt",
                                  _app,
                                  "--opt,-o");

    struct d_arg_option* const pos = d_tests_arg_make_all(_run,
                                                          "pos",
                                                          _app,
                                                          "pos");

    d_arg_option_set_expected(pos,
                              -1);

    return;
}

/**
 * @brief P02: clusters of short flags, a short option's attached value, and
 *        an option at the end of a cluster taking the next argument.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_flag(_run,
                                "a",
                                _app,
                                "-a");
    (void)d_tests_arg_make_flag(_run,
                                "b",
                                _app,
                                "-b");
    (void)d_tests_arg_make_all(_run,
                               "c",
                               _app,
                               "-c");

    return;
}

/**
 * @brief P17: an option given no value at the end of the line.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p17(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_option(_run,
                                  "opt",
                                  _app,
                                  "--opt");

    return;
}

/**
 * @brief P19: negative numbers, and what only looks like one, among
 *        positionals.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p19(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const n = d_tests_arg_make_all(_run,
                                                        "n",
                                                        _app,
                                                        "n");

    d_arg_option_set_expected(n,
                              -1);
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);

    return;
}

/**
 * @brief P26: flag values, a negated name, and a run of one short flag.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p26(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const flag =
        d_tests_arg_make_flag(_run,
                              "flag",
                              _app,
                              "--flag,!--no-flag");

    d_arg_option_set_multi_option_policy(flag,
                                         (int)D_ARG_MULTI_TAKE_ALL);

    struct d_arg_option* const v = d_tests_arg_make_flag(_run,
                                                         "v",
                                                         _app,
                                                         "-v");

    d_arg_option_set_multi_option_policy(v,
                                         (int)D_ARG_MULTI_TAKE_ALL);

    return;
}

/**
 * @brief P36: an option inside a cluster of short names, and a cluster
 *        after an option, which becomes its value.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p36(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_all(_run,
                               "o",
                               _app,
                               "-o");
    (void)d_tests_arg_make_flag(_run,
                                "a",
                                _app,
                                "-a");
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);

    return;
}

/**
 * @brief Q12: what follows a short flag in its argument: a dash, `=`, the
 *        flag again.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_q12(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_flag(_run,
                                "a",
                                _app,
                                "-a");
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);

    return;
}

/**
 * @brief Q15: a flag that refuses a value other than its own.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_q15(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const flag = d_tests_arg_make_flag(_run,
                                                            "flag",
                                                            _app,
                                                            "--flag");

    d_arg_option_set_disable_flag_override(flag,
                                           true);
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);

    return;
}

/**
 * @brief P39: an option whose value looks like another option.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p39(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_option(_run,
                                  "opt",
                                  _app,
                                  "--opt");
    (void)d_tests_arg_make_flag(_run,
                                "flag",
                                _app,
                                "--flag");

    return;
}

/**
 * @brief P40: `=` after a short name, an empty value after `=` and given
 *        apart, and `=` inside a value.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p40(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_all(_run,
                               "o",
                               _app,
                               "-o");
    (void)d_tests_arg_make_all(_run,
                               "long",
                               _app,
                               "--long");

    return;
}

/**
 * @brief P41: a lone `-`, and `--` given twice.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p41(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const pos = d_tests_arg_make_all(_run,
                                                          "pos",
                                                          _app,
                                                          "pos");

    d_arg_option_set_expected(pos,
                              -1);
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                _app,
                                "--f");

    return;
}

/**
 * @brief P42: a short flag named by a digit, beside negative numbers.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p42(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_flag(_run,
                                "one",
                                _app,
                                "-1");

    struct d_arg_option* const n = d_tests_arg_make_all(_run,
                                                        "n",
                                                        _app,
                                                        "n");

    d_arg_option_set_expected(n,
                              -1);

    return;
}

// TOKEN_CASES
//   constant: the cases of d_tests_arg_parsing_tokens.
static const struct d_tests_arg_case TOKEN_CASES[] =
{
    { "P01",
      d_tests_arg_parsing_p01,
      { "--flag", "-o", "val", "a", "b", "--", "-c", "--flag" },
      NULL,
      NULL },
    { "P02",
      d_tests_arg_parsing_p02,
      { "-ab", "-cval", "-bac", "7", "x" },
      NULL,
      NULL },
    { "P17",
      d_tests_arg_parsing_p17,
      { "--opt" },
      NULL,
      NULL },
    { "P19",
      d_tests_arg_parsing_p19,
      { "-5", "-.5", "-x", "-.x" },
      NULL,
      NULL },
    { "P26",
      d_tests_arg_parsing_p26,
      { "--flag=false", "--no-flag", "-vvv", "--no-flag=0", "--flag" },
      NULL,
      NULL },
    { "P36",
      d_tests_arg_parsing_p36,
      { "-ao", "v", "-oa" },
      NULL,
      NULL },
    { "Q12",
      d_tests_arg_parsing_q12,
      { "-a-b", "-a=5", "-aa" },
      NULL,
      NULL },
    { "Q15",
      d_tests_arg_parsing_q15,
      { "--flag=true", "--flag=2" },
      NULL,
      NULL },
    { "P39",
      d_tests_arg_parsing_p39,
      { "--opt", "--flag" },
      NULL,
      NULL },
    { "P40",
      d_tests_arg_parsing_p40,
      { "-o=v", "--long=", "-o", "", "--long", "x=y" },
      NULL,
      NULL },
    { "P41",
      d_tests_arg_parsing_p41,
      { "-", "--", "--", "--f", "-" },
      NULL,
      NULL },
    { "P42",
      d_tests_arg_parsing_p42,
      { "-1", "-2", "-12", "-.1" },
      NULL,
      NULL }
};

// TOKEN_COUNT
//   constant: how many cases TOKEN_CASES holds.
static const size_t TOKEN_COUNT = sizeof(TOKEN_CASES) / sizeof(TOKEN_CASES[0]);

/*
d_tests_arg_parsing_tokens
  How arguments are told apart and cut.
  Tests the following:
  - long and short names, clusters of short flags, a short option's attached
    value, and `--` ending the options
  - an option without its value at the end of the line, and one whose value
    looks like an option
  - negative numbers taken as positionals, but for a short name that is a digit
  - flag values, negated names, and a refused override
  - `=` after a short name, an empty value, and a lone `-`
*/
bool
d_tests_arg_parsing_tokens(void)
{
    return d_tests_arg_cases("TOKENS",
                             TOKEN_CASES,
                             TOKEN_COUNT,
                             TOKENS);
}

//==============================================================================
// 2.  EXTRAS
//==============================================================================

/**
 * @brief P08: extras assumed to take every value after them.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p08(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_ASSUME_MULTIPLE);
    (void)d_tests_arg_make_flag(_run,
                                "flag",
                                _app,
                                "--flag");

    return;
}

/**
 * @brief P09: extras assumed to take one value.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p09(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_ASSUME_SINGLE);
    (void)d_tests_arg_make_flag(_run,
                                "flag",
                                _app,
                                "--flag");

    return;
}

/**
 * @brief P18: extras refused as soon as one is met.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p18(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_ERROR_IMMEDIATELY);
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                _app,
                                "--f");

    return;
}

/**
 * @brief Q10: extras ignored.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_q10(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_IGNORE);
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                _app,
                                "--f");
    (void)d_tests_arg_make_subcommand(_run,
                                      _app,
                                      "s");

    return;
}

/**
 * @brief Q03: `++` where there is no subcommand to leave.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_q03(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                _app,
                                "--f");

    return;
}

/**
 * @brief P33: extras kept by the subcommand each came after, a subcommand
 *        met again.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p33(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);

    struct d_arg_command* const s = d_tests_arg_make_subcommand(_run,
                                                                _app,
                                                                "s");

    d_arg_command_set_allow_extras(s,
                                   D_ARG_EXTRAS_CAPTURE);

    struct d_arg_command* const t = d_tests_arg_make_subcommand(_run,
                                                                _app,
                                                                "t");

    d_arg_command_set_allow_extras(t,
                                   D_ARG_EXTRAS_CAPTURE);

    return;
}

/**
 * @brief P43: a parent's option after a subcommand that does not fall
 *        through to it.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p43(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_flag(_run,
                                "top",
                                _app,
                                "--top");

    struct d_arg_command* const s = d_tests_arg_make_subcommand(_run,
                                                                _app,
                                                                "s");

    (void)d_tests_arg_make_flag(_run,
                                "in",
                                s,
                                "--in");

    return;
}

/**
 * @brief P44: extras kept by a subcommand around its own flag.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p44(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const s = d_tests_arg_make_subcommand(_run,
                                                                _app,
                                                                "s");

    d_arg_command_set_allow_extras(s,
                                   D_ARG_EXTRAS_CAPTURE);
    (void)d_tests_arg_make_flag(_run,
                                "in",
                                s,
                                "--in");

    return;
}

/**
 * @brief P45: extras assumed to take every value, stopping at a
 *        subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p45(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_ASSUME_MULTIPLE);

    struct d_arg_command* const s = d_tests_arg_make_subcommand(_run,
                                                                _app,
                                                                "s");

    (void)d_tests_arg_make_flag(_run,
                                "in",
                                s,
                                "--in");

    return;
}

/**
 * @brief P68: extras assumed to take one value, kept by their command
 *        rather than by its option group that keeps extras.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p68(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_ASSUME_SINGLE);

    struct d_arg_command* const h = d_tests_arg_make_group(_run,
                                                           _app,
                                                           "H");

    d_arg_command_set_allow_extras(h,
                                   D_ARG_EXTRAS_CAPTURE);
    (void)d_tests_arg_make_subcommand(_run,
                                      _app,
                                      "s");

    return;
}

/**
 * @brief P71: an extra assumed to take one value, before a positional.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p71(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_ASSUME_SINGLE);

    struct d_arg_option* const pos = d_tests_arg_make_all(_run,
                                                          "pos",
                                                          _app,
                                                          "pos");

    d_arg_option_set_expected(pos,
                              -1);

    return;
}

// EXTRAS_CASES
//   constant: the cases of d_tests_arg_parsing_extras.
static const struct d_tests_arg_case EXTRAS_CASES[] =
{
    { "P08",
      d_tests_arg_parsing_p08,
      { "--zzz", "1", "2", "--flag", "3", "-q", "4" },
      NULL,
      NULL },
    { "P09",
      d_tests_arg_parsing_p09,
      { "--zzz", "1", "2", "--flag", "-q" },
      NULL,
      NULL },
    { "P18",
      d_tests_arg_parsing_p18,
      { "--f", "a", "b" },
      NULL,
      NULL },
    { "Q10",
      d_tests_arg_parsing_q10,
      { "x", "--zz", "--f", "-q" },
      NULL,
      NULL },
    { "Q03",
      d_tests_arg_parsing_q03,
      { "++", "--f", "++" },
      NULL,
      NULL },
    { "P33",
      d_tests_arg_parsing_p33,
      { "s", "x", "t", "y", "s", "z" },
      NULL,
      NULL },
    { "P43",
      d_tests_arg_parsing_p43,
      { "s", "--in", "--top" },
      NULL,
      NULL },
    { "P44",
      d_tests_arg_parsing_p44,
      { "s", "a", "--in", "--zz", "b" },
      NULL,
      NULL },
    { "P45",
      d_tests_arg_parsing_p45,
      { "--zz", "1", "s", "--in" },
      NULL,
      NULL },
    { "P68",
      d_tests_arg_parsing_p68,
      { "--zz", "1", "rest" },
      NULL,
      NULL },
    { "P71",
      d_tests_arg_parsing_p71,
      { "--zz", "1", "2", "3" },
      NULL,
      NULL }
};

// EXTRAS_COUNT
//   constant: how many cases EXTRAS_CASES holds.
static const size_t EXTRAS_COUNT =
    sizeof(EXTRAS_CASES) / sizeof(EXTRAS_CASES[0]);

/*
d_tests_arg_parsing_extras
  What a command does with arguments it does not expect.
  Tests the following:
  - each extras mode: kept, kept with the values after an unknown option,
    refused at once, ignored
  - `++` outside a subcommand
  - extras kept by the subcommand they came after, and a parent's option refused
    there
  - extras a command keeps itself rather than leave to an option group that
    keeps them
*/
bool
d_tests_arg_parsing_extras(void)
{
    return d_tests_arg_cases("EXTRAS",
                             EXTRAS_CASES,
                             EXTRAS_COUNT,
                             EXTRAS);
}

//==============================================================================
// 3.  STYLES
//==============================================================================

/**
 * @brief P10: Windows-style options: values after `:` and apart, a short
 *        name, a flag, and an unknown one.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p10(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    true);
    (void)d_tests_arg_make_all(_run,
                               "opt",
                               _app,
                               "--opt,-o");
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                _app,
                                "-f");

    return;
}

/**
 * @brief P11: a short name of two characters.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p11(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_non_standard_names(_app,
                                         true);
    (void)d_tests_arg_make_all(_run,
                               "ab",
                               _app,
                               "-ab");
    (void)d_tests_arg_make_flag(_run,
                                "c",
                                _app,
                                "-c");

    return;
}

/**
 * @brief P25: dot notation in the Windows style, and in the usual one.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p25(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    true);

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    (void)d_tests_arg_make_all(_run,
                               "x",
                               sub,
                               "--x");
    (void)d_tests_arg_make_all(_run,
                               "y",
                               sub,
                               "-y");

    return;
}

/**
 * @brief P46: a value holding `:`, a lone `/`, an empty value, and `=`
 *        where the style has `:`.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p46(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    true);
    (void)d_tests_arg_make_all(_run,
                               "opt",
                               _app,
                               "--opt");

    struct d_arg_option* const pos = d_tests_arg_make_all(_run,
                                                          "pos",
                                                          _app,
                                                          "pos");

    d_arg_option_set_expected(pos,
                              -1);

    return;
}

/**
 * @brief P47: a short name of three characters, given whole and longer,
 *        beside a cluster of one-character flags.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p47(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_non_standard_names(_app,
                                         true);
    (void)d_tests_arg_make_flag(_run,
                                "abc",
                                _app,
                                "-abc");
    (void)d_tests_arg_make_flag(_run,
                                "x",
                                _app,
                                "-x");
    (void)d_tests_arg_make_flag(_run,
                                "y",
                                _app,
                                "-y");
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);

    return;
}

// STYLE_CASES
//   constant: the cases of d_tests_arg_parsing_styles.
static const struct d_tests_arg_case STYLE_CASES[] =
{
    { "P10",
      d_tests_arg_parsing_p10,
      { "/opt:val", "/opt", "val2", "/o:3", "/f", "/unk" },
      NULL,
      NULL },
    { "P11",
      d_tests_arg_parsing_p11,
      { "-ab=5", "-ab", "6", "-c" },
      NULL,
      NULL },
    { "P25",
      d_tests_arg_parsing_p25,
      { "/sub.x:5", "/sub.y:6", "--sub.y=7" },
      NULL,
      NULL },
    { "P46",
      d_tests_arg_parsing_p46,
      { "/opt:a:b", "/", "/opt:", "//x", "/opt=c" },
      NULL,
      NULL },
    { "P47",
      d_tests_arg_parsing_p47,
      { "-abc", "-xy", "-abcd", "-yx", "-abc=2" },
      NULL,
      NULL }
};

// STYLE_COUNT
//   constant: how many cases STYLE_CASES holds.
static const size_t STYLE_COUNT = sizeof(STYLE_CASES) / sizeof(STYLE_CASES[0]);

/*
d_tests_arg_parsing_styles
  Windows-style options and names CLI11 calls non-standard.
  Tests the following:
  - `/name:value`, `/name value` and `/flag`, an unknown one, and dot notation
    in that style
  - a value holding `:`, an empty one, a lone `/`, and `=` where `:` belongs
  - a short name of several characters, given whole, among clusters, and with a
    value
*/
bool
d_tests_arg_parsing_styles(void)
{
    return d_tests_arg_cases("STYLES",
                             STYLE_CASES,
                             STYLE_COUNT,
                             STYLES);
}

//==============================================================================
// 4.  SUBCOMMANDS
//==============================================================================

/**
 * @brief P03: `++` leaving a subcommand, which is then met again.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p03(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_flag(_run,
                                "flag",
                                _app,
                                "--flag");

    struct d_arg_command* const sub1 = d_tests_arg_make_subcommand(_run,
                                                                   _app,
                                                                   "sub1");

    (void)d_tests_arg_make_all(_run,
                               "x",
                               sub1,
                               "--x");

    return;
}

/**
 * @brief P06: a subcommand falling through to its parent's flag.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p06(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_flag(_run,
                                "flag",
                                _app,
                                "--flag");

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_fallthrough(sub,
                                  true);
    (void)d_tests_arg_make_option(_run,
                                  "own",
                                  sub,
                                  "--own");

    return;
}

/**
 * @brief P21: a subcommand with an immediate callback, met twice.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p21(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_immediate_callback(sub,
                                         true);
    (void)d_tests_arg_make_all(_run,
                               "x",
                               sub,
                               "--x");

    return;
}

/**
 * @brief P23: a command taking one subcommand at most, given two.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p23(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_require_subcommand(_app,
                                     0u,
                                     1u);

    struct d_arg_command* const a = d_tests_arg_make_subcommand(_run,
                                                                _app,
                                                                "a");

    (void)d_tests_arg_make_subcommand(_run,
                                      _app,
                                      "b");
    d_arg_command_set_allow_extras(a,
                                   D_ARG_EXTRAS_CAPTURE);

    return;
}

/**
 * @brief Q02: a subcommand that does not let its parent's subcommands
 *        through.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_q02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_subcommand_fallthrough(sub,
                                             false);
    (void)d_tests_arg_make_subcommand(_run,
                                      _app,
                                      "other");
    d_arg_command_set_allow_extras(sub,
                                   D_ARG_EXTRAS_CAPTURE);

    return;
}

/**
 * @brief Q04: subcommands found by a prefix of their names.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_q04(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_prefix_matching(_app,
                                      true);
    (void)d_tests_arg_make_subcommand(_run,
                                      _app,
                                      "sub");
    (void)d_tests_arg_make_subcommand(_run,
                                      _app,
                                      "other");
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);

    return;
}

/**
 * @brief Q05: a subcommand ignoring case, a disabled one, and a silent one.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_q05(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const upper = d_tests_arg_make_subcommand(_run,
                                                                    _app,
                                                                    "Sub");

    d_arg_command_set_ignore_case(upper,
                                  true);

    struct d_arg_command* const dis = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "dis");

    d_arg_command_set_disabled(dis,
                               true);

    struct d_arg_command* const quiet = d_tests_arg_make_subcommand(_run,
                                                                    _app,
                                                                    "quiet");

    d_arg_command_set_silent(quiet,
                             true);
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);

    return;
}

/**
 * @brief Q06: a subcommand falling through, with a parse-complete
 *        callback, met twice.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_q06(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);

    struct d_arg_command* const a = d_tests_arg_make_subcommand(_run,
                                                                _app,
                                                                "a");

    d_arg_command_set_fallthrough(a,
                                  true);
    d_arg_command_set_parse_complete_callback(a,
                                              d_tests_arg_parsing_nothing,
                                              NULL,
                                              NULL);
    (void)d_tests_arg_make_subcommand(_run,
                                      _app,
                                      "b");

    return;
}

/**
 * @brief Q07: a subcommand falling through, met twice.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_q07(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);

    struct d_arg_command* const a = d_tests_arg_make_subcommand(_run,
                                                                _app,
                                                                "a");

    d_arg_command_set_fallthrough(a,
                                  true);
    (void)d_tests_arg_make_subcommand(_run,
                                      _app,
                                      "b");

    return;
}

/**
 * @brief Q11: a subcommand's required positional after `--`.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_q11(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const s = d_tests_arg_make_subcommand(_run,
                                                                _app,
                                                                "s");
    struct d_arg_option* const  p = d_tests_arg_make_option(_run,
                                                            "p",
                                                            s,
                                                            "p");

    d_arg_option_set_required(p,
                              true);
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);
    d_arg_command_set_allow_extras(s,
                                   D_ARG_EXTRAS_CAPTURE);

    return;
}

/**
 * @brief Q17: `++` from two levels down, both falling through.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_q17(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const s = d_tests_arg_make_subcommand(_run,
                                                                _app,
                                                                "s");
    struct d_arg_command* const t = d_tests_arg_make_subcommand(_run,
                                                                s,
                                                                "t");

    (void)d_tests_arg_make_flag(_run,
                                "deep",
                                t,
                                "--deep");
    (void)d_tests_arg_make_flag(_run,
                                "top",
                                _app,
                                "--top");
    d_arg_command_set_fallthrough(t,
                                  true);
    d_arg_command_set_fallthrough(s,
                                  true);

    return;
}

/**
 * @brief P48: a subcommand entered by its aliases and its name.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p48(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    (void)d_arg_command_add_alias(sub,
                                  "s2");
    (void)d_arg_command_add_alias(sub,
                                  "third");
    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);

    return;
}

/**
 * @brief P49: a prefix two subcommands share, then one that a used
 *        subcommand no longer shares.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p49(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_prefix_matching(_app,
                                      true);
    (void)d_tests_arg_make_subcommand(_run,
                                      _app,
                                      "sub");
    (void)d_tests_arg_make_subcommand(_run,
                                      _app,
                                      "super");
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);

    return;
}

/**
 * @brief P50: a subcommand's name after positionals, and after `--`.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p50(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const pos = d_tests_arg_make_all(_run,
                                                          "pos",
                                                          _app,
                                                          "pos");

    d_arg_option_set_expected(pos,
                              -1);

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_allow_extras(sub,
                                   D_ARG_EXTRAS_CAPTURE);

    return;
}

// SUBCOMMAND_CASES
//   constant: the cases of d_tests_arg_parsing_subcommands.
static const struct d_tests_arg_case SUBCOMMAND_CASES[] =
{
    { "P03",
      d_tests_arg_parsing_p03,
      { "sub1", "--x", "1", "++", "--flag", "sub1", "--x", "2" },
      NULL,
      NULL },
    { "P06",
      d_tests_arg_parsing_p06,
      { "sub", "--flag", "a", "--own", "1" },
      NULL,
      NULL },
    { "P21",
      d_tests_arg_parsing_p21,
      { "sub", "--x", "1", "sub", "--x", "2" },
      NULL,
      NULL },
    { "P23",
      d_tests_arg_parsing_p23,
      { "a", "b" },
      NULL,
      NULL },
    { "Q02",
      d_tests_arg_parsing_q02,
      { "sub", "other" },
      NULL,
      NULL },
    { "Q04",
      d_tests_arg_parsing_q04,
      { "su", "o" },
      NULL,
      NULL },
    { "Q05",
      d_tests_arg_parsing_q05,
      { "SUB", "dis", "quiet" },
      NULL,
      NULL },
    { "Q06",
      d_tests_arg_parsing_q06,
      { "a", "x", "a", "y" },
      NULL,
      NULL },
    { "Q07",
      d_tests_arg_parsing_q07,
      { "a", "x", "a", "y" },
      NULL,
      NULL },
    { "Q11",
      d_tests_arg_parsing_q11,
      { "s", "--", "v", "w" },
      NULL,
      NULL },
    { "Q17",
      d_tests_arg_parsing_q17,
      { "s", "t", "--deep", "--top", "++", "++", "--top" },
      NULL,
      NULL },
    { "P48",
      d_tests_arg_parsing_p48,
      { "s2", "--x", "1", "third", "sub" },
      NULL,
      NULL },
    { "P49",
      d_tests_arg_parsing_p49,
      { "su", "sup", "s" },
      NULL,
      NULL },
    { "P50",
      d_tests_arg_parsing_p50,
      { "x", "sub", "y", "--", "sub" },
      NULL,
      NULL }
};

// SUBCOMMAND_COUNT
//   constant: how many cases SUBCOMMAND_CASES holds.
static const size_t SUBCOMMAND_COUNT =
    sizeof(SUBCOMMAND_CASES) / sizeof(SUBCOMMAND_CASES[0]);

/*
d_tests_arg_parsing_subcommands
  How subcommands are entered, left and found.
  Tests the following:
  - `++` leaving a subcommand, and a subcommand met again
  - falling through to a parent, or not, from one level and from two
  - immediate callbacks, and the most subcommands a command takes
  - prefixes, aliases, case, and disabled and silent subcommands
  - a subcommand's name among positionals, and after `--`
*/
bool
d_tests_arg_parsing_subcommands(void)
{
    return d_tests_arg_cases("SUBCOMMANDS",
                             SUBCOMMAND_CASES,
                             SUBCOMMAND_COUNT,
                             SUBCOMMANDS);
}

//==============================================================================
// 5.  DOTS
//==============================================================================

/**
 * @brief P04: options named through a subcommand, and a path of two
 *        subcommands.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p04(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const sub1 = d_tests_arg_make_subcommand(_run,
                                                                   _app,
                                                                   "sub1");

    (void)d_tests_arg_make_all(_run,
                               "x",
                               sub1,
                               "--x");
    (void)d_tests_arg_make_all(_run,
                               "v",
                               sub1,
                               "-v");

    struct d_arg_command* const sub2 = d_tests_arg_make_subcommand(_run,
                                                                   sub1,
                                                                   "sub2");

    (void)d_tests_arg_make_option(_run,
                                  "y",
                                  sub2,
                                  "--y");

    return;
}

/**
 * @brief P38: a long name of one character through a subcommand, an
 *        unknown one, and paths with nothing after or before the dot.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p38(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    (void)d_tests_arg_make_flag(_run,
                                "a",
                                sub,
                                "--a");

    return;
}

/**
 * @brief Q18: dot notation where the command takes one subcommand at most.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_q18(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_require_subcommand(_app,
                                     0u,
                                     1u);

    struct d_arg_command* const s = d_tests_arg_make_subcommand(_run,
                                                                _app,
                                                                "s");

    (void)d_tests_arg_make_option(_run,
                                  "v",
                                  s,
                                  "--v");
    (void)d_tests_arg_make_subcommand(_run,
                                      _app,
                                      "t");
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);

    return;
}

/**
 * @brief P51: a short name through a subcommand, its value attached and
 *        apart, a long name of one character, and a missing value.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p51(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    (void)d_tests_arg_make_all(_run,
                               "y",
                               sub,
                               "-y");
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                sub,
                                "--f");
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);

    return;
}

/**
 * @brief P52: an option taking two values through a subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p52(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");
    struct d_arg_option* const  xx  = d_tests_arg_make_all(_run,
                                                           "xx",
                                                           sub,
                                                           "--xx");

    d_arg_option_set_expected(xx,
                              2);
    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);

    return;
}

/**
 * @brief P53: dot notation through a subcommand ignoring case, and a
 *        subcommand path that names no subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p53(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const upper = d_tests_arg_make_subcommand(_run,
                                                                    _app,
                                                                    "Sub");

    d_arg_command_set_ignore_case(upper,
                                  true);
    (void)d_tests_arg_make_flag(_run,
                                "aa",
                                upper,
                                "--aa");

    struct d_arg_command* const t = d_tests_arg_make_subcommand(_run,
                                                                _app,
                                                                "t");

    (void)d_tests_arg_make_flag(_run,
                                "bb",
                                t,
                                "--bb");
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);

    return;
}

/**
 * @brief P69: dot notation with nothing between the dot and `=`, an empty
 *        value, a dot after the dot, and a dash.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p69(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    (void)d_tests_arg_make_all(_run,
                               "y",
                               sub,
                               "-y");
    (void)d_tests_arg_make_flag(_run,
                                "aa",
                                sub,
                                "--aa");
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);

    return;
}

/**
 * @brief P70: dot notation once the command has its most subcommands: the
 *        one it has, and another.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p70(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_require_subcommand(_app,
                                     0u,
                                     1u);

    struct d_arg_command* const s = d_tests_arg_make_subcommand(_run,
                                                                _app,
                                                                "s");

    (void)d_tests_arg_make_all(_run,
                               "vv",
                               s,
                               "--vv");

    struct d_arg_command* const t = d_tests_arg_make_subcommand(_run,
                                                                _app,
                                                                "t");

    (void)d_tests_arg_make_option(_run,
                                  "ww",
                                  t,
                                  "--ww");
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);

    return;
}

// DOT_CASES
//   constant: the cases of d_tests_arg_parsing_dots.
static const struct d_tests_arg_case DOT_CASES[] =
{
    { "P04",
      d_tests_arg_parsing_p04,
      { "--sub1.x=5", "--sub1.v", "6", "sub1.sub2", "--y", "7" },
      NULL,
      NULL },
    { "P38",
      d_tests_arg_parsing_p38,
      { "--sub.a", "--sub.b", "--sub.", "--.x" },
      NULL,
      NULL },
    { "Q18",
      d_tests_arg_parsing_q18,
      { "--s.v=1", "--t.x", "t" },
      NULL,
      NULL },
    { "P51",
      d_tests_arg_parsing_p51,
      { "--sub.y", "1", "--sub.y=2", "--sub.f", "--sub.f=3", "--sub.y" },
      NULL,
      NULL },
    { "P52",
      d_tests_arg_parsing_p52,
      { "--sub.xx", "a", "b", "c", "--sub.x", "d", "--sub.xx=e", "f" },
      NULL,
      NULL },
    { "P53",
      d_tests_arg_parsing_p53,
      { "--SUB.aa", "--t.bb", "--t.bb", "t.x", "--sub.aa" },
      NULL,
      NULL },
    { "P69",
      d_tests_arg_parsing_p69,
      { "--sub.=x", "--sub.y=", "z", "--sub.aa=", "--sub..aa", "--sub.-y" },
      NULL,
      NULL },
    { "P70",
      d_tests_arg_parsing_p70,
      { "--s.vv=1", "--t.ww=2", "--s.vv=3" },
      NULL,
      NULL }
};

// DOT_COUNT
//   constant: how many cases DOT_CASES holds.
static const size_t DOT_COUNT = sizeof(DOT_CASES) / sizeof(DOT_CASES[0]);

/*
d_tests_arg_parsing_dots
  Dot notation: a subcommand's option named through it.
  Tests the following:
  - `--sub.name`, with values attached and apart, and a path of subcommands
  - names of one character, read as short names, and unknown ones
  - an option taking two values, a subcommand that ignores case, and the most
    subcommands a command takes
  - a dot with nothing after it, an empty value, and names no option form takes
*/
bool
d_tests_arg_parsing_dots(void)
{
    return d_tests_arg_cases("DOTS",
                             DOT_CASES,
                             DOT_COUNT,
                             DOTS);
}

//==============================================================================
// 6.  GROUPS
//==============================================================================

/**
 * @brief P05: an option in a group, and a group keeping extras.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p05(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const g = d_tests_arg_make_group(_run,
                                                           _app,
                                                           "G");

    (void)d_tests_arg_make_option(_run,
                                  "g",
                                  g,
                                  "--g");

    struct d_arg_command* const h = d_tests_arg_make_group(_run,
                                                           _app,
                                                           "H");

    d_arg_command_set_allow_extras(h,
                                   D_ARG_EXTRAS_CAPTURE);
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                _app,
                                "--f");

    return;
}

/**
 * @brief P24: a subcommand inside a group.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p24(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const g  = d_tests_arg_make_group(_run,
                                                            _app,
                                                            "G");
    struct d_arg_command* const gs = d_tests_arg_make_subcommand(_run,
                                                                 g,
                                                                 "gs");

    (void)d_tests_arg_make_flag(_run,
                                "q",
                                gs,
                                "--q");

    return;
}

/**
 * @brief P37: a group inside a subcommand, with a positional.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p37(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");
    struct d_arg_command* const sg  = d_tests_arg_make_group(_run,
                                                             sub,
                                                             "SG");

    (void)d_tests_arg_make_option(_run,
                                  "gopt",
                                  sg,
                                  "--gopt");
    (void)d_tests_arg_make_option(_run,
                                  "gpos",
                                  sg,
                                  "gpos");

    return;
}

/**
 * @brief Q16: a group inside a group.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_q16(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const g = d_tests_arg_make_group(_run,
                                                           _app,
                                                           "G");
    struct d_arg_command* const h = d_tests_arg_make_group(_run,
                                                           g,
                                                           "H");

    (void)d_tests_arg_make_option(_run,
                                  "hp",
                                  h,
                                  "hp");
    (void)d_tests_arg_make_option(_run,
                                  "ho",
                                  h,
                                  "--ho");
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);

    return;
}

/**
 * @brief P54: a group's positional between two of its command's.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p54(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_option(_run,
                                  "first",
                                  _app,
                                  "first");

    struct d_arg_command* const g = d_tests_arg_make_group(_run,
                                                           _app,
                                                           "G");

    (void)d_tests_arg_make_option(_run,
                                  "gp",
                                  g,
                                  "gp");
    (void)d_tests_arg_make_option(_run,
                                  "last",
                                  _app,
                                  "last");

    return;
}

/**
 * @brief P55: a group's flag before a subcommand and after it.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p55(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const g = d_tests_arg_make_group(_run,
                                                           _app,
                                                           "G");

    (void)d_tests_arg_make_flag(_run,
                                "gf",
                                g,
                                "--gf");
    d_arg_command_set_allow_extras(g,
                                   D_ARG_EXTRAS_CAPTURE);

    struct d_arg_command* const s = d_tests_arg_make_subcommand(_run,
                                                                _app,
                                                                "s");

    (void)d_tests_arg_make_flag(_run,
                                "sf",
                                s,
                                "--sf");

    return;
}

// GROUP_CASES
//   constant: the cases of d_tests_arg_parsing_groups.
static const struct d_tests_arg_case GROUP_CASES[] =
{
    { "P05",
      d_tests_arg_parsing_p05,
      { "--g", "3", "rest", "--zz", "--f" },
      NULL,
      NULL },
    { "P24",
      d_tests_arg_parsing_p24,
      { "gs", "--q" },
      NULL,
      NULL },
    { "P37",
      d_tests_arg_parsing_p37,
      { "sub", "--gopt", "1", "q" },
      NULL,
      NULL },
    { "Q16",
      d_tests_arg_parsing_q16,
      { "--ho", "1", "q", "r" },
      NULL,
      NULL },
    { "P54",
      d_tests_arg_parsing_p54,
      { "a", "b", "c" },
      NULL,
      NULL },
    { "P55",
      d_tests_arg_parsing_p55,
      { "--gf", "s", "--sf", "--gf" },
      NULL,
      NULL }
};

// GROUP_COUNT
//   constant: how many cases GROUP_CASES holds.
static const size_t GROUP_COUNT = sizeof(GROUP_CASES) / sizeof(GROUP_CASES[0]);

/*
d_tests_arg_parsing_groups
  Option groups in a parse.
  Tests the following:
  - a group's options and positionals, and a group keeping extras
  - a subcommand in a group, a group in a subcommand, and a group in a group
  - positionals shared between a command and its group
  - a group's flag after a subcommand
*/
bool
d_tests_arg_parsing_groups(void)
{
    return d_tests_arg_cases("GROUPS",
                             GROUP_CASES,
                             GROUP_COUNT,
                             GROUPS);
}

//==============================================================================
// 7.  PREFIX COMMANDS
//==============================================================================

/**
 * @brief P07: a prefix command meeting an argument it does not know.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p07(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_prefix_command(_app,
                                     D_ARG_PREFIX_ON);
    (void)d_tests_arg_make_flag(_run,
                                "flag",
                                _app,
                                "--flag");

    return;
}

/**
 * @brief P32: a subcommand passing on the rest at `--` alone.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p32(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const s = d_tests_arg_make_subcommand(_run,
                                                                _app,
                                                                "s");

    d_arg_command_set_prefix_command(s,
                                     D_ARG_PREFIX_SEPARATOR_ONLY);
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                s,
                                "--f");

    return;
}

/**
 * @brief Q01: a prefix command passing on the rest at a positional alone.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_q01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_prefix_command(_app,
                                     D_ARG_PREFIX_POSITIONAL_ONLY);
    (void)d_tests_arg_make_flag(_run,
                                "flag",
                                _app,
                                "--flag");

    return;
}

/**
 * @brief P56: a subcommand that is a prefix command, after its parent's
 *        flag.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p56(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const s = d_tests_arg_make_subcommand(_run,
                                                                _app,
                                                                "s");

    d_arg_command_set_prefix_command(s,
                                     D_ARG_PREFIX_ON);
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                s,
                                "--f");
    (void)d_tests_arg_make_flag(_run,
                                "top",
                                _app,
                                "--top");

    return;
}

/**
 * @brief P57: a prefix command with a positional, which takes the first
 *        argument.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p57(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_prefix_command(_app,
                                     D_ARG_PREFIX_ON);
    (void)d_tests_arg_make_option(_run,
                                  "p",
                                  _app,
                                  "p");
    (void)d_tests_arg_make_flag(_run,
                                "flag",
                                _app,
                                "--flag");

    return;
}

/**
 * @brief P58: a prefix command given `--` first.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p58(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_prefix_command(_app,
                                     D_ARG_PREFIX_ON);
    (void)d_tests_arg_make_flag(_run,
                                "flag",
                                _app,
                                "--flag");
    (void)d_tests_arg_make_subcommand(_run,
                                      _app,
                                      "sub");

    return;
}

/**
 * @brief P72: a prefix command meeting an unknown option first.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p72(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_prefix_command(_app,
                                     D_ARG_PREFIX_ON);
    (void)d_tests_arg_make_flag(_run,
                                "flag",
                                _app,
                                "--flag");

    return;
}

// PREFIX_CASES
//   constant: the cases of d_tests_arg_parsing_prefix_commands.
static const struct d_tests_arg_case PREFIX_CASES[] =
{
    { "P07",
      d_tests_arg_parsing_p07,
      { "--flag", "x", "--unknown", "y", "--", "z" },
      NULL,
      NULL },
    { "P32",
      d_tests_arg_parsing_p32,
      { "s", "--f", "u", "--", "--f" },
      NULL,
      NULL },
    { "Q01",
      d_tests_arg_parsing_q01,
      { "--flag", "--unk", "p", "--x", "--flag" },
      NULL,
      NULL },
    { "P56",
      d_tests_arg_parsing_p56,
      { "--top", "s", "--f", "rest", "--f", "--top" },
      NULL,
      NULL },
    { "P57",
      d_tests_arg_parsing_p57,
      { "a", "b", "--flag" },
      NULL,
      NULL },
    { "P58",
      d_tests_arg_parsing_p58,
      { "--", "sub", "--flag" },
      NULL,
      NULL },
    { "P72",
      d_tests_arg_parsing_p72,
      { "--unknown", "--flag", "x" },
      NULL,
      NULL }
};

// PREFIX_COUNT
//   constant: how many cases PREFIX_CASES holds.
static const size_t PREFIX_COUNT =
    sizeof(PREFIX_CASES) / sizeof(PREFIX_CASES[0]);

/*
d_tests_arg_parsing_prefix_commands
  Commands that pass on the rest of the line.
  Tests the following:
  - a prefix command ending its parse at the first argument it does not know, a
    positional or an option
  - one ending it at `--` alone, and one at a positional alone
  - a subcommand that is a prefix command
  - a positional before the rest is passed on, and `--` first
*/
bool
d_tests_arg_parsing_prefix_commands(void)
{
    return d_tests_arg_cases("PREFIX_COMMANDS",
                             PREFIX_CASES,
                             PREFIX_COUNT,
                             PREFIX_COMMANDS);
}

//==============================================================================
// 8.  VALUES
//==============================================================================

/**
 * @brief P13: an option taking every value, with a separator between its
 *        uses.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p13(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const v = d_tests_arg_make_option(_run,
                                                           "v",
                                                           _app,
                                                           "--v");

    d_arg_option_set_expected(v,
                              -1);
    d_arg_option_set_inject_separator(v,
                                      true);
    d_arg_option_set_multi_option_policy(v,
                                         (int)D_ARG_MULTI_TAKE_ALL);
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                _app,
                                "--f");

    return;
}

/**
 * @brief P27: an option whose value may be left out.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p27(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const o = d_tests_arg_make_all(_run,
                                                        "o",
                                                        _app,
                                                        "--o");

    d_arg_option_set_expected_range(o,
                                    0,
                                    1);
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                _app,
                                "--f");

    return;
}

/**
 * @brief P28: a pair given one value.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p28(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const pair = d_tests_arg_make_option(_run,
                                                              "pair",
                                                              _app,
                                                              "--pair");

    d_arg_option_set_type_size(pair,
                               2);

    return;
}

/**
 * @brief P29: an option of one or two values, used twice.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p29(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const pair = d_tests_arg_make_option(_run,
                                                              "pair",
                                                              _app,
                                                              "--pair");

    d_arg_option_set_type_size_range(pair,
                                     1,
                                     2);
    d_arg_option_set_expected(pair,
                              1);
    d_arg_option_set_multi_option_policy(pair,
                                         (int)D_ARG_MULTI_TAKE_ALL);
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                _app,
                                "--f");

    return;
}

/**
 * @brief P31: an option taking every value it can between two required
 *        positionals.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p31(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const a = d_tests_arg_make_option(_run,
                                                           "a",
                                                           _app,
                                                           "a");

    d_arg_option_set_required(a,
                              true);

    struct d_arg_option* const o = d_tests_arg_make_all(_run,
                                                        "o",
                                                        _app,
                                                        "--o");

    d_arg_option_set_expected(o,
                              -1);
    d_arg_option_set_allow_extra_args(o,
                                      true);

    struct d_arg_option* const b = d_tests_arg_make_option(_run,
                                                           "b",
                                                           _app,
                                                           "b");

    d_arg_option_set_required(b,
                              true);

    return;
}

/**
 * @brief P34: an option taking every value, which stop at the first that
 *        fails its validator.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p34(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const v = d_tests_arg_make_all(_run,
                                                        "v",
                                                        _app,
                                                        "--v");

    d_arg_option_set_expected(v,
                              -1);
    d_arg_command_set_validate_optional_arguments(_app,
                                                  true);
    (void)d_arg_option_check(v,
                             d_arg_validator_positive_number(),
                             NULL);
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);

    return;
}

/**
 * @brief P35: an option taking every value before a required positional.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p35(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const v = d_tests_arg_make_all(_run,
                                                        "v",
                                                        _app,
                                                        "--v");

    d_arg_option_set_expected(v,
                              -1);

    struct d_arg_option* const p = d_tests_arg_make_option(_run,
                                                           "p",
                                                           _app,
                                                           "p");

    d_arg_option_set_required(p,
                              true);
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);

    return;
}

/**
 * @brief P59: an option taking two values, used twice, beside a positional.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p59(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const two = d_tests_arg_make_all(_run,
                                                          "two",
                                                          _app,
                                                          "--two");

    d_arg_option_set_expected(two,
                              2);

    struct d_arg_option* const pos = d_tests_arg_make_all(_run,
                                                          "pos",
                                                          _app,
                                                          "pos");

    d_arg_option_set_expected(pos,
                              -1);

    return;
}

/**
 * @brief P60: an option taking every value, stopping at a flag and at a
 *        subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p60(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const v = d_tests_arg_make_all(_run,
                                                        "v",
                                                        _app,
                                                        "--v");

    d_arg_option_set_expected(v,
                              -1);
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                _app,
                                "--f");

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");
    struct d_arg_option* const  p   = d_tests_arg_make_all(_run,
                                                           "p",
                                                           sub,
                                                           "p");

    d_arg_option_set_expected(p,
                              -1);

    return;
}

/**
 * @brief P61: an option taking one to three values, given more, fewer and
 *        none before an unknown option.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p61(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const r = d_tests_arg_make_all(_run,
                                                        "r",
                                                        _app,
                                                        "--r");

    d_arg_option_set_expected_range(r,
                                    1,
                                    3);
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                _app,
                                "--f");

    struct d_arg_option* const pos = d_tests_arg_make_all(_run,
                                                          "pos",
                                                          _app,
                                                          "pos");

    d_arg_option_set_expected(pos,
                              -1);

    return;
}

/**
 * @brief P73: options of one value or more on either side of the count
 *        CLI11 takes as many, and one of two or more.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p73(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const a = d_tests_arg_make_all(_run,
                                                        "a",
                                                        _app,
                                                        "--a");

    d_arg_option_set_expected_range(a,
                                    1,
                                    MANY);

    struct d_arg_option* const b = d_tests_arg_make_all(_run,
                                                        "b",
                                                        _app,
                                                        "--b");

    d_arg_option_set_expected_range(b,
                                    1,
                                    MANY - 1);

    struct d_arg_option* const c = d_tests_arg_make_all(_run,
                                                        "c",
                                                        _app,
                                                        "--c");

    d_arg_option_set_expected_range(c,
                                    2,
                                    MANY);

    struct d_arg_option* const pos = d_tests_arg_make_all(_run,
                                                          "pos",
                                                          _app,
                                                          "pos");

    d_arg_option_set_expected(pos,
                              -1);

    return;
}

/**
 * @brief P74: `--` ending an option's values, and after an option of one
 *        value.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p74(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const v = d_tests_arg_make_all(_run,
                                                        "v",
                                                        _app,
                                                        "--v");

    d_arg_option_set_expected(v,
                              -1);
    (void)d_tests_arg_make_all(_run,
                               "one",
                               _app,
                               "--one");

    struct d_arg_option* const pos = d_tests_arg_make_all(_run,
                                                          "pos",
                                                          _app,
                                                          "pos");

    d_arg_option_set_expected(pos,
                              -1);

    return;
}

// VALUE_CASES
//   constant: the cases of d_tests_arg_parsing_values.
static const struct d_tests_arg_case VALUE_CASES[] =
{
    { "P13",
      d_tests_arg_parsing_p13,
      { "--v", "a", "b", "--v", "c", "--f" },
      NULL,
      NULL },
    { "P27",
      d_tests_arg_parsing_p27,
      { "--o", "--f", "--o", "x", "--o" },
      NULL,
      NULL },
    { "P28",
      d_tests_arg_parsing_p28,
      { "--pair", "1" },
      NULL,
      NULL },
    { "P29",
      d_tests_arg_parsing_p29,
      { "--pair", "1", "2", "--pair", "3", "--f" },
      NULL,
      NULL },
    { "P31",
      d_tests_arg_parsing_p31,
      { "--o", "1", "2", "3", "4" },
      NULL,
      NULL },
    { "P34",
      d_tests_arg_parsing_p34,
      { "--v", "1", "2", "x", "3" },
      NULL,
      NULL },
    { "P35",
      d_tests_arg_parsing_p35,
      { "--v", "1", "2", "3" },
      NULL,
      NULL },
    { "P59",
      d_tests_arg_parsing_p59,
      { "--two", "a", "b", "c", "--two", "d" },
      NULL,
      NULL },
    { "P60",
      d_tests_arg_parsing_p60,
      { "--v", "1", "--f", "2", "--v", "3", "sub", "4" },
      NULL,
      NULL },
    { "P61",
      d_tests_arg_parsing_p61,
      { "--r", "1", "2", "3", "4", "--r", "5", "--f", "--r", "6", "-x" },
      NULL,
      NULL },
    { "P73",
      d_tests_arg_parsing_p73,
      { "--a", "1", "2", "--b", "3", "4", "--c", "5", "6", "7" },
      NULL,
      NULL },
    { "P74",
      d_tests_arg_parsing_p74,
      { "--v", "1", "--", "2", "--one", "3", "--", "4" },
      NULL,
      NULL }
};

// VALUE_COUNT
//   constant: how many cases VALUE_CASES holds.
static const size_t VALUE_COUNT = sizeof(VALUE_CASES) / sizeof(VALUE_CASES[0]);

/*
d_tests_arg_parsing_values
  How many values an option takes, and where they stop.
  Tests the following:
  - a separator between an option's uses, and optional values
  - pairs, too few for one, and sizes between one and two
  - an option taking every value, beside required positionals, validated, and
    stopping at an option or a subcommand
  - an option taking two values, and one taking one to three
  - the count at which an option without extra arguments takes one item at each
    use
  - `--` after an option's values, eaten where it ends them
*/
bool
d_tests_arg_parsing_values(void)
{
    return d_tests_arg_cases("VALUES",
                             VALUE_CASES,
                             VALUE_COUNT,
                             VALUES);
}

//==============================================================================
// 9.  POSITIONALS
//==============================================================================

/**
 * @brief P14: an optional and a required positional at the end, given one
 *        argument.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p14(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_positionals_at_end(_app,
                                         true);
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "a");

    struct d_arg_option* const b = d_tests_arg_make_option(_run,
                                                           "b",
                                                           _app,
                                                           "b");

    d_arg_option_set_required(b,
                              true);

    return;
}

/**
 * @brief P15: the same, given more arguments than they take.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p15(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_positionals_at_end(_app,
                                         true);
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "a");

    struct d_arg_option* const b = d_tests_arg_make_option(_run,
                                                           "b",
                                                           _app,
                                                           "b");

    d_arg_option_set_required(b,
                              true);

    return;
}

/**
 * @brief P16: a positional whose validator sends an argument to the next.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p16(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_validate_positionals(_app,
                                           true);

    struct d_arg_option* const num = d_tests_arg_make_option(_run,
                                                             "num",
                                                             _app,
                                                             "num");

    (void)d_arg_option_check(num,
                             d_arg_validator_positive_number(),
                             NULL);
    (void)d_tests_arg_make_option(_run,
                                  "word",
                                  _app,
                                  "word");

    return;
}

/**
 * @brief P30: two positionals taking every value.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p30(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const a = d_tests_arg_make_option(_run,
                                                           "a",
                                                           _app,
                                                           "a");

    d_arg_option_set_expected(a,
                              -1);

    struct d_arg_option* const b = d_tests_arg_make_option(_run,
                                                           "b",
                                                           _app,
                                                           "b");

    d_arg_option_set_expected(b,
                              -1);

    return;
}

/**
 * @brief Q08: a positional that may be left out, before a flag.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_q08(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const pos = d_tests_arg_make_all(_run,
                                                          "pos",
                                                          _app,
                                                          "pos");

    d_arg_option_set_expected_range(pos,
                                    0,
                                    1);
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                _app,
                                "--f");

    return;
}

/**
 * @brief P62: a positional pair, then another positional.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p62(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const pair = d_tests_arg_make_all(_run,
                                                           "pair",
                                                           _app,
                                                           "pair");

    d_arg_option_set_expected(pair,
                              2);
    (void)d_tests_arg_make_option(_run,
                                  "last",
                                  _app,
                                  "last");

    return;
}

/**
 * @brief P63: a positional pair given one argument.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p63(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const pair = d_tests_arg_make_all(_run,
                                                           "pair",
                                                           _app,
                                                           "pair");

    d_arg_option_set_expected(pair,
                              2);

    return;
}

/**
 * @brief P64: a required positional at the end whose validator refuses the
 *        one argument, which goes to the optional one.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p64(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_positionals_at_end(_app,
                                         true);
    d_arg_command_set_validate_positionals(_app,
                                           true);
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "a");

    struct d_arg_option* const n = d_tests_arg_make_option(_run,
                                                           "n",
                                                           _app,
                                                           "n");

    d_arg_option_set_required(n,
                              true);
    (void)d_arg_option_check(n,
                             d_arg_validator_positive_number(),
                             NULL);
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);

    return;
}

/**
 * @brief P65: a required pair at the end, given just two arguments.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p65(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_positionals_at_end(_app,
                                         true);
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "a");

    struct d_arg_option* const b = d_tests_arg_make_all(_run,
                                                        "b",
                                                        _app,
                                                        "b");

    d_arg_option_set_expected(b,
                              2);
    d_arg_option_set_required(b,
                              true);

    return;
}

// POSITIONAL_CASES
//   constant: the cases of d_tests_arg_parsing_positionals.
static const struct d_tests_arg_case POSITIONAL_CASES[] =
{
    { "P14",
      d_tests_arg_parsing_p14,
      { "x" },
      NULL,
      NULL },
    { "P15",
      d_tests_arg_parsing_p15,
      { "x", "y", "z", "w" },
      NULL,
      NULL },
    { "P16",
      d_tests_arg_parsing_p16,
      { "abc", "12" },
      NULL,
      NULL },
    { "P30",
      d_tests_arg_parsing_p30,
      { "x" },
      NULL,
      NULL },
    { "Q08",
      d_tests_arg_parsing_q08,
      { "x", "--f" },
      NULL,
      NULL },
    { "P62",
      d_tests_arg_parsing_p62,
      { "a", "b", "c" },
      NULL,
      NULL },
    { "P63",
      d_tests_arg_parsing_p63,
      { "a" },
      NULL,
      NULL },
    { "P64",
      d_tests_arg_parsing_p64,
      { "w" },
      NULL,
      NULL },
    { "P65",
      d_tests_arg_parsing_p65,
      { "x", "y" },
      NULL,
      NULL }
};

// POSITIONAL_COUNT
//   constant: how many cases POSITIONAL_CASES holds.
static const size_t POSITIONAL_COUNT =
    sizeof(POSITIONAL_CASES) / sizeof(POSITIONAL_CASES[0]);

/*
d_tests_arg_parsing_positionals
  Which positional takes an argument.
  Tests the following:
  - required positionals filled first, when they are at the end
  - a positional's validator passing an argument on to the next
  - two positionals taking every value, an optional one, and pairs
*/
bool
d_tests_arg_parsing_positionals(void)
{
    return d_tests_arg_cases("POSITIONALS",
                             POSITIONAL_CASES,
                             POSITIONAL_COUNT,
                             POSITIONALS);
}

//==============================================================================
// 10. CHECKS
//==============================================================================

/**
 * @brief V01: two optional positionals taking every value.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_v01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const a = d_tests_arg_make_option(_run,
                                                           "a",
                                                           _app,
                                                           "a");

    d_arg_option_set_expected_range(a,
                                    1,
                                    D_ARG_EXPECTED_UNLIMITED);

    struct d_arg_option* const b = d_tests_arg_make_option(_run,
                                                           "b",
                                                           _app,
                                                           "b");

    d_arg_option_set_expected_range(b,
                                    1,
                                    D_ARG_EXPECTED_UNLIMITED);

    return;
}

/**
 * @brief V02: the same, the first required.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_v02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const a = d_tests_arg_make_option(_run,
                                                           "a",
                                                           _app,
                                                           "a");

    d_arg_option_set_expected_range(a,
                                    1,
                                    D_ARG_EXPECTED_UNLIMITED);
    d_arg_option_set_required(a,
                              true);

    struct d_arg_option* const b = d_tests_arg_make_all(_run,
                                                        "b",
                                                        _app,
                                                        "b");

    d_arg_option_set_expected_range(b,
                                    1,
                                    D_ARG_EXPECTED_UNLIMITED);

    return;
}

/**
 * @brief V03: a command requiring more options than it has.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_v03(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_require_option_count(_app,
                                       3);
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                _app,
                                "--f");

    return;
}

/**
 * @brief V04: a command requiring more options than its most.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_v04(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_require_option(_app,
                                 2u,
                                 1u);
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                _app,
                                "--f");
    (void)d_tests_arg_make_flag(_run,
                                "g",
                                _app,
                                "--g");

    return;
}

/**
 * @brief V05: two optional positionals taking every value, in a
 *        subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_v05(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");
    struct d_arg_option* const  a   = d_tests_arg_make_option(_run,
                                                              "a",
                                                              sub,
                                                              "a");

    d_arg_option_set_expected_range(a,
                                    1,
                                    D_ARG_EXPECTED_UNLIMITED);

    struct d_arg_option* const b = d_tests_arg_make_option(_run,
                                                           "b",
                                                           sub,
                                                           "b");

    d_arg_option_set_expected_range(b,
                                    1,
                                    D_ARG_EXPECTED_UNLIMITED);

    return;
}

/**
 * @brief V06: an option group's option counted for its command.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_v06(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const g = d_tests_arg_make_group(_run,
                                                           _app,
                                                           "G");

    (void)d_tests_arg_make_flag(_run,
                                "gf",
                                g,
                                "--gf");
    d_arg_command_require_option_count(_app,
                                       2);
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                _app,
                                "--f");

    return;
}

// CHECK_CASES
//   constant: the cases of d_tests_arg_parsing_checks.
static const struct d_tests_arg_case CHECK_CASES[] =
{
    { "V01",
      d_tests_arg_parsing_v01,
      { "x" },
      NULL,
      NULL },
    { "V02",
      d_tests_arg_parsing_v02,
      { "x", "y" },
      NULL,
      NULL },
    { "V03",
      d_tests_arg_parsing_v03,
      { "--f" },
      NULL,
      NULL },
    { "V04",
      d_tests_arg_parsing_v04,
      { "--f" },
      NULL,
      NULL },
    { "V05",
      d_tests_arg_parsing_v05,
      { "sub" },
      NULL,
      NULL },
    { "V06",
      d_tests_arg_parsing_v06,
      { "--f", "--gf" },
      NULL,
      NULL }
};

// CHECK_COUNT
//   constant: how many cases CHECK_CASES holds.
static const size_t CHECK_COUNT = sizeof(CHECK_CASES) / sizeof(CHECK_CASES[0]);

/*
d_tests_arg_parsing_checks
  The definitions checked when a parse begins.
  Tests the following:
  - positionals taking every value, as many as can share the arguments
  - the options a command requires, against those it has and its most
*/
bool
d_tests_arg_parsing_checks(void)
{
    return d_tests_arg_cases("CHECKS",
                             CHECK_CASES,
                             CHECK_COUNT,
                             CHECKS);
}

//==============================================================================
// 11. CALLBACKS
//==============================================================================

/**
 * @brief P12: an option whose callback runs as each use is parsed.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p12(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const t =
        d_arg_command_add_option_callback(_app,
                                          "--t",
                                          d_tests_arg_parsing_first,
                                          _run,
                                          NULL,
                                          NULL);

    (void)d_tests_arg_watch_option(_run,
                                   "t",
                                   t);
    d_arg_option_set_trigger_on_parse(t,
                                      true);

    return;
}

/**
 * @brief P20: pre-parse callbacks on a command and its subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p20(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_preparse_callback(_app,
                                        d_tests_arg_parsing_preparse,
                                        _run,
                                        NULL);

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_preparse_callback(sub,
                                        d_tests_arg_parsing_preparse,
                                        _run,
                                        NULL);

    struct d_arg_option* const p = d_tests_arg_make_all(_run,
                                                        "p",
                                                        sub,
                                                        "p");

    d_arg_option_set_expected(p,
                              -1);

    return;
}

/**
 * @brief Q09: a positional whose callback runs as each use is parsed, its
 *        values split at commas.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_q09(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const p =
        d_arg_command_add_option_callback(_app,
                                          "p",
                                          d_tests_arg_parsing_first,
                                          _run,
                                          NULL,
                                          NULL);

    (void)d_tests_arg_watch_all(_run,
                                "p",
                                p);
    d_arg_option_set_expected(p,
                              -1);
    d_arg_option_set_delimiter(p,
                               ',');
    d_arg_option_set_trigger_on_parse(p,
                                      true);

    return;
}

/**
 * @brief P66: pre-parse callbacks, a subcommand used through dot notation.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p66(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_preparse_callback(_app,
                                        d_tests_arg_parsing_preparse,
                                        _run,
                                        NULL);

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_preparse_callback(sub,
                                        d_tests_arg_parsing_preparse,
                                        _run,
                                        NULL);
    (void)d_tests_arg_make_flag(_run,
                                "aa",
                                sub,
                                "--aa");
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);

    return;
}

/**
 * @brief P67: parse-complete and final callbacks on a command and a
 *        subcommand met twice, and an immediate one.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_parsing_p67(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_allow_extras(_app,
                                   D_ARG_EXTRAS_CAPTURE);

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_parse_complete_callback(sub,
                                              d_tests_arg_parsing_done,
                                              _run,
                                              NULL);
    d_arg_command_set_final_callback(sub,
                                     d_tests_arg_parsing_final,
                                     _run,
                                     NULL);
    d_arg_command_set_parse_complete_callback(_app,
                                              d_tests_arg_parsing_done,
                                              _run,
                                              NULL);
    d_arg_command_set_final_callback(_app,
                                     d_tests_arg_parsing_final,
                                     _run,
                                     NULL);

    struct d_arg_command* const t = d_tests_arg_make_subcommand(_run,
                                                                _app,
                                                                "t");

    d_arg_command_set_immediate_callback(t,
                                         true);
    d_arg_command_set_final_callback(t,
                                     d_tests_arg_parsing_final,
                                     _run,
                                     NULL);

    return;
}

// CALLBACK_CASES
//   constant: the cases of d_tests_arg_parsing_callbacks.
static const struct d_tests_arg_case CALLBACK_CASES[] =
{
    { "P12",
      d_tests_arg_parsing_p12,
      { "--t", "1", "--t", "2" },
      NULL,
      NULL },
    { "P20",
      d_tests_arg_parsing_p20,
      { "sub", "b", "c" },
      NULL,
      NULL },
    { "Q09",
      d_tests_arg_parsing_q09,
      { "a", ",", "b,c" },
      NULL,
      NULL },
    { "P66",
      d_tests_arg_parsing_p66,
      { "x", "--sub.aa", "y" },
      NULL,
      NULL },
    { "P67",
      d_tests_arg_parsing_p67,
      { "sub", "t", "x", "sub" },
      NULL,
      NULL }
};

// CALLBACK_COUNT
//   constant: how many cases CALLBACK_CASES holds.
static const size_t CALLBACK_COUNT =
    sizeof(CALLBACK_CASES) / sizeof(CALLBACK_CASES[0]);

/*
d_tests_arg_parsing_callbacks
  Callbacks the parse itself runs.
  Tests the following:
  - an option's callback run as each use is parsed, with a delimiter
  - pre-parse callbacks, given the arguments left, and through dot notation
  - parse-complete and final callbacks, and an immediate one, in their order
*/
bool
d_tests_arg_parsing_callbacks(void)
{
    return d_tests_arg_cases("CALLBACKS",
                             CALLBACK_CASES,
                             CALLBACK_COUNT,
                             CALLBACKS);
}

//==============================================================================
// 12. THE SUITE
//==============================================================================

// d_tests_arg_parsing
//   constant: the suite, in the header's order.
const struct d_tests_section d_tests_arg_parsing[] =
{
    { "arg_parsing_tokens",
      d_tests_arg_parsing_tokens },
    { "arg_parsing_extras",
      d_tests_arg_parsing_extras },
    { "arg_parsing_styles",
      d_tests_arg_parsing_styles },
    { "arg_parsing_subcommands",
      d_tests_arg_parsing_subcommands },
    { "arg_parsing_dots",
      d_tests_arg_parsing_dots },
    { "arg_parsing_groups",
      d_tests_arg_parsing_groups },
    { "arg_parsing_prefix_commands",
      d_tests_arg_parsing_prefix_commands },
    { "arg_parsing_values",
      d_tests_arg_parsing_values },
    { "arg_parsing_positionals",
      d_tests_arg_parsing_positionals },
    { "arg_parsing_checks",
      d_tests_arg_parsing_checks },
    { "arg_parsing_callbacks",
      d_tests_arg_parsing_callbacks }
};

// d_tests_arg_parsing_count
//   constant: how many sections the table above holds.
const size_t d_tests_arg_parsing_count =
    sizeof(d_tests_arg_parsing) / sizeof(d_tests_arg_parsing[0]);
