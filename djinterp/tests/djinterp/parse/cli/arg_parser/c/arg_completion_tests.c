/*******************************************************************************
* djinterp [parse]                                        arg_completion_tests.c
*
* The suite of shell completion, declared in arg_completion_tests.h.
*   Its sections are rows -- names, numbers, bytes, lines and descriptions,
* each written as a line -- and steps, each making a command tree on a run's
* parser of its own (arg_tests_support.h) and writing the script each shell
* is given for it, or what a parse with the completion option came to. A
* script's engines -- the fixed text every script for a shell holds -- are
* held to completion/eng/, the source the library's arrays are written from,
* and written as one line each, `[engine <name>]`, so that a transcript
* holds what the tree makes.
*   The transcripts are tools/gen/completion_ref.py's, which works them out
* from completion/eng/ and the rules arg_completion.h states, apart from
* the C that follows them; each is held in as many constants as a C99
* string literal's 4095 characters need.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_completion_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.17
*                                                            revised: 2026.10.19
*******************************************************************************/
#include "./arg_completion_tests.h"  // corresponding header
// std
#include <limits.h>   // INT_MAX, INT_MIN, UCHAR_MAX
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <stdlib.h>   // free
#include <string.h>   // memcmp
// djinterp
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text, d_pack_sink
#include "../../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // the knobs
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // commands, their settings
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // buffers, statuses, modes
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_completion.h"  // the calls tested
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // options, their settings
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // the parser, its exit
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_type.h"  // built-in types, type flags
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator.h"  // validators, their kinds
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator_extra.h"  // sets and mappings
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator_path.h"  // path validators
#include "../../../../../../src/djinterp/parse/cli/arg_parser/c/arg_internal.h"  // identifiers, lines, descriptions
#include "../../../c/tests_section.h"  // d_tests_section
#include "./arg_tests_support.h"  // runs, steps, escaping
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


#if (D_INTERNAL_ARG_PARSER_COMPLETION == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// SHELL_FROM_NAME
//   constant: what d_tests_arg_completion_shell_from_name writes, as
// completion_ref.py works it out from the rules arg_completion.h states.
static const char SHELL_FROM_NAME[] =
    "[bash] 0\n"
    "[zsh] 1\n"
    "[fish] 2\n"
    "[powershell] 3\n"
    "[pwsh] 3\n"
    "[BASH] 0\n"
    "[Zsh] 1\n"
    "[fIsH] 2\n"
    "[PowerShell] 3\n"
    "[PWSH] 3\n"
    "[pWsH] 3\n"
    "[empty] -1\n"
    "[no data] -1\n"
    "[no data, a length] -1\n"
    "[bas] -1\n"
    "[bashx] -1\n"
    "[zs] -1\n"
    "[pwsh ] -1\n"
    "[ bash] -1\n"
    "[ba sh] -1\n"
    "[bash, cut] 0\n"
    "[zsh, cut short] -1\n"
    "[bash NUL] -1\n"
    "[NUL bash] -1\n"
    "[powershel] -1\n"
    "[powershells] -1\n"
    "[power] -1\n"
    "[sh] -1\n"
    "[csh] -1\n"
    "[tcsh] -1\n"
    "[ksh] -1\n"
    "[cmd] -1\n"
    "[nushell] -1\n"
    "[elvish] -1\n"
    "[BASH, dotless] -1\n"
    "[ZSH, long s] -1\n"
    "[FISH, fullwidth] -1\n"
    "[BaSh\\t] -1\n"
    "[\\x80ash] -1\n"
    "[b@sh] -1\n"
    "[bAsh] 0\n"
    "[fisH] 2\n"
    "[zsH] 1\n";

// SHELL_NAME
//   constant: what d_tests_arg_completion_shell_name writes, as
// completion_ref.py works it out from the rules arg_completion.h states.
static const char SHELL_NAME[] =
    "0 [bash]\n"
    "1 [zsh]\n"
    "2 [fish]\n"
    "3 [powershell]\n"
    "4 []\n"
    "5 []\n"
    "-1 []\n"
    "-2 []\n"
    "INT_MAX []\n"
    "INT_MIN []\n"
    "100 []\n";

// IDENTIFIERS
//   constant: what d_tests_arg_completion_identifiers writes, as
// completion_ref.py works it out from the rules arg_completion.h states.
static const char IDENTIFIERS[] =
    "00 _00 OK\n"
    "01 _01 OK\n"
    "02 _02 OK\n"
    "03 _03 OK\n"
    "04 _04 OK\n"
    "05 _05 OK\n"
    "06 _06 OK\n"
    "07 _07 OK\n"
    "08 _08 OK\n"
    "09 _09 OK\n"
    "0a _0a OK\n"
    "0b _0b OK\n"
    "0c _0c OK\n"
    "0d _0d OK\n"
    "0e _0e OK\n"
    "0f _0f OK\n"
    "10 _10 OK\n"
    "11 _11 OK\n"
    "12 _12 OK\n"
    "13 _13 OK\n"
    "14 _14 OK\n"
    "15 _15 OK\n"
    "16 _16 OK\n"
    "17 _17 OK\n"
    "18 _18 OK\n"
    "19 _19 OK\n"
    "1a _1a OK\n"
    "1b _1b OK\n"
    "1c _1c OK\n"
    "1d _1d OK\n"
    "1e _1e OK\n"
    "1f _1f OK\n"
    "20 _20 OK\n"
    "21 _21 OK\n"
    "22 _22 OK\n"
    "23 _23 OK\n"
    "24 _24 OK\n"
    "25 _25 OK\n"
    "26 _26 OK\n"
    "27 _27 OK\n"
    "28 _28 OK\n"
    "29 _29 OK\n"
    "2a _2a OK\n"
    "2b _2b OK\n"
    "2c _2c OK\n"
    "2d _2d OK\n"
    "2e _2e OK\n"
    "2f _2f OK\n"
    "30 0 OK\n"
    "31 1 OK\n"
    "32 2 OK\n"
    "33 3 OK\n"
    "34 4 OK\n"
    "35 5 OK\n"
    "36 6 OK\n"
    "37 7 OK\n"
    "38 8 OK\n"
    "39 9 OK\n"
    "3a _3a OK\n"
    "3b _3b OK\n"
    "3c _3c OK\n"
    "3d _3d OK\n"
    "3e _3e OK\n"
    "3f _3f OK\n"
    "40 _40 OK\n"
    "41 A OK\n"
    "42 B OK\n"
    "43 C OK\n"
    "44 D OK\n"
    "45 E OK\n"
    "46 F OK\n"
    "47 G OK\n"
    "48 H OK\n"
    "49 I OK\n"
    "4a J OK\n"
    "4b K OK\n"
    "4c L OK\n"
    "4d M OK\n"
    "4e N OK\n"
    "4f O OK\n"
    "50 P OK\n"
    "51 Q OK\n"
    "52 R OK\n"
    "53 S OK\n"
    "54 T OK\n"
    "55 U OK\n"
    "56 V OK\n"
    "57 W OK\n"
    "58 X OK\n"
    "59 Y OK\n"
    "5a Z OK\n"
    "5b _5b OK\n"
    "5c _5c OK\n"
    "5d _5d OK\n"
    "5e _5e OK\n"
    "5f __ OK\n"
    "60 _60 OK\n"
    "61 a OK\n"
    "62 b OK\n"
    "63 c OK\n"
    "64 d OK\n"
    "65 e OK\n"
    "66 f OK\n"
    "67 g OK\n"
    "68 h OK\n"
    "69 i OK\n"
    "6a j OK\n"
    "6b k OK\n"
    "6c l OK\n"
    "6d m OK\n"
    "6e n OK\n"
    "6f o OK\n"
    "70 p OK\n"
    "71 q OK\n"
    "72 r OK\n"
    "73 s OK\n"
    "74 t OK\n"
    "75 u OK\n"
    "76 v OK\n"
    "77 w OK\n"
    "78 x OK\n"
    "79 y OK\n"
    "7a z OK\n"
    "7b _7b OK\n"
    "7c _7c OK\n"
    "7d _7d OK\n"
    "7e _7e OK\n"
    "7f _7f OK\n"
    "80 _80 OK\n"
    "81 _81 OK\n"
    "82 _82 OK\n"
    "83 _83 OK\n"
    "84 _84 OK\n"
    "85 _85 OK\n"
    "86 _86 OK\n"
    "87 _87 OK\n"
    "88 _88 OK\n"
    "89 _89 OK\n"
    "8a _8a OK\n"
    "8b _8b OK\n"
    "8c _8c OK\n"
    "8d _8d OK\n"
    "8e _8e OK\n"
    "8f _8f OK\n"
    "90 _90 OK\n"
    "91 _91 OK\n"
    "92 _92 OK\n"
    "93 _93 OK\n"
    "94 _94 OK\n"
    "95 _95 OK\n"
    "96 _96 OK\n"
    "97 _97 OK\n"
    "98 _98 OK\n"
    "99 _99 OK\n"
    "9a _9a OK\n"
    "9b _9b OK\n"
    "9c _9c OK\n"
    "9d _9d OK\n"
    "9e _9e OK\n"
    "9f _9f OK\n"
    "a0 _a0 OK\n"
    "a1 _a1 OK\n"
    "a2 _a2 OK\n"
    "a3 _a3 OK\n"
    "a4 _a4 OK\n"
    "a5 _a5 OK\n"
    "a6 _a6 OK\n"
    "a7 _a7 OK\n"
    "a8 _a8 OK\n"
    "a9 _a9 OK\n"
    "aa _aa OK\n"
    "ab _ab OK\n"
    "ac _ac OK\n"
    "ad _ad OK\n"
    "ae _ae OK\n"
    "af _af OK\n"
    "b0 _b0 OK\n"
    "b1 _b1 OK\n"
    "b2 _b2 OK\n"
    "b3 _b3 OK\n"
    "b4 _b4 OK\n"
    "b5 _b5 OK\n"
    "b6 _b6 OK\n"
    "b7 _b7 OK\n"
    "b8 _b8 OK\n"
    "b9 _b9 OK\n"
    "ba _ba OK\n"
    "bb _bb OK\n"
    "bc _bc OK\n"
    "bd _bd OK\n"
    "be _be OK\n"
    "bf _bf OK\n"
    "c0 _c0 OK\n"
    "c1 _c1 OK\n"
    "c2 _c2 OK\n"
    "c3 _c3 OK\n"
    "c4 _c4 OK\n"
    "c5 _c5 OK\n"
    "c6 _c6 OK\n"
    "c7 _c7 OK\n"
    "c8 _c8 OK\n"
    "c9 _c9 OK\n"
    "ca _ca OK\n"
    "cb _cb OK\n"
    "cc _cc OK\n"
    "cd _cd OK\n"
    "ce _ce OK\n"
    "cf _cf OK\n"
    "d0 _d0 OK\n"
    "d1 _d1 OK\n"
    "d2 _d2 OK\n"
    "d3 _d3 OK\n"
    "d4 _d4 OK\n"
    "d5 _d5 OK\n"
    "d6 _d6 OK\n"
    "d7 _d7 OK\n"
    "d8 _d8 OK\n"
    "d9 _d9 OK\n"
    "da _da OK\n"
    "db _db OK\n"
    "dc _dc OK\n"
    "dd _dd OK\n"
    "de _de OK\n"
    "df _df OK\n"
    "e0 _e0 OK\n"
    "e1 _e1 OK\n"
    "e2 _e2 OK\n"
    "e3 _e3 OK\n"
    "e4 _e4 OK\n"
    "e5 _e5 OK\n"
    "e6 _e6 OK\n"
    "e7 _e7 OK\n"
    "e8 _e8 OK\n"
    "e9 _e9 OK\n"
    "ea _ea OK\n"
    "eb _eb OK\n"
    "ec _ec OK\n"
    "ed _ed OK\n"
    "ee _ee OK\n"
    "ef _ef OK\n"
    "f0 _f0 OK\n"
    "f1 _f1 OK\n"
    "f2 _f2 OK\n"
    "f3 _f3 OK\n"
    "f4 _f4 OK\n"
    "f5 _f5 OK\n"
    "f6 _f6 OK\n"
    "f7 _f7 OK\n"
    "f8 _f8 OK\n"
    "f9 _f9 OK\n"
    "fa _fa OK\n"
    "fb _fb OK\n"
    "fc _fc OK\n"
    "fd _fd OK\n"
    "fe _fe OK\n"
    "ff _ff OK\n"
    "[prog] prog OK\n"
    "[my_tool] my__tool OK\n"
    "[my-tool] my_2dtool OK\n"
    "[a.b.c] a_2eb_2ec OK\n"
    "[7z] 7z OK\n"
    "[__init__] ____init____ OK\n"
    "[two words] two_20words OK\n"
    "[UTF-8] caf_c3_a9 OK\n"
    "[astral] _f0_9f_98_80 OK\n"
    "[stray] _ff_fe OK\n"
    "[quote] it_27s OK\n"
    "[slash] a_2fb_5cc OK\n"
    "[empty]  OK\n"
    "[NUL] a_00b OK\n"
    "[tab] a_09b OK\n"
    "[mixed] Ab__9_2dx_2eY OK\n"
    "[digits] 0123456789 OK\n"
    "[letters] AZaz OK\n"
    "[edges] _40_5b_60_7b_2f_3a OK\n";

// SCRIPT_LINES
//   constant: what d_tests_arg_completion_script_lines writes, as
// completion_ref.py works it out from the rules arg_completion.h states.
static const char SCRIPT_LINES[] =
    "== none OK\n"
    "plain\n"
    "\n"
    "== one OK\n"
    "__djarg_prog_first() {\n"
    "== two OK\n"
    "_djarg_p x _djarg_p\n"
    "== three marks OK\n"
    "X@\n"
    "== four marks OK\n"
    "XX\n"
    "== five marks OK\n"
    "XX@\n"
    "== a lone mark OK\n"
    "@\n"
    "a@b\n"
    "@ @\n"
    "== ends OK\n"
    "idstart\n"
    "endid\n"
    "id\n"
    "== empty ident OK\n"
    "ab\n"
    "\n"
    "\n"
    "== long ident OK\n"
    "<my__tool_2dname_c3_a9>\n"
    "== ident with marks OK\n"
    "@@\n"
    "== several lines OK\n"
    "first q\n"
    "\n"
    "second q and q\n"
    "third\n"
    "== no lines OK\n";

// LINE_TEXT
//   constant: what d_tests_arg_completion_line_text writes, as
// completion_ref.py works it out from the rules arg_completion.h states.
static const char LINE_TEXT[] =
    "[NULL] []\n"
    "[empty] []\n"
    "[plain] [Do the thing]\n"
    "[two lines] [First line]\n"
    "[carriage return] [First]\n"
    "[CRLF] [First]\n"
    "[leading line break] []\n"
    "[padded] [padded]\n"
    "[tabs] [in tabs]\n"
    "[every space] [text]\n"
    "[padded first line] [one]\n"
    "[only spaces] []\n"
    "[only a line break] []\n"
    "[inner spaces] [a  b\x09" "c]\n"
    "[trailing break] [end]\n"
    "[vertical tab] [a\x0b" "b]\n"
    "[form feed] [a\x0c" "b]\n"
    "[control] [a\x01" "b]\n"
    "[UTF-8] [caf\xc3\xa9 \xe2\x86\x92 ok]\n"
    "[NBSP] [\xc2\xa0kept\xc2\xa0]\n";

// REFUSALS
//   constant: what d_tests_arg_completion_refusals writes, as completion_ref.py
// works it out from the rules arg_completion.h states.
static const char REFUSALS[] =
    "no command: BadArgument wrote=0\n"
    "shell -1: BadArgument wrote=0\n"
    "shell COUNT: BadArgument wrote=0\n"
    "shell INT_MAX: BadArgument wrote=0\n"
    "shell INT_MIN: BadArgument wrote=0\n"
    "program without data: BadArgument wrote=0\n"
    "program without data, one byte long: BadArgument wrote=0\n"
    "program without data or length: OK wrote=12819\n"
    "root unnamed: BadArgument wrote=0\n"
    "root named /: BadArgument wrote=0\n"
    "root named dir\\: BadArgument wrote=0\n"
    "root unnamed, a program given: OK wrote=16378\n"
    "the sink refuses: FileError wrote=0\n"
    "a listing fails: Unsupported wrote=0\n"
    "a listing fails after a value: Unsupported wrote=0\n"
    "a listing fails in fish: Unsupported wrote=0\n"
    "a listing fails in PowerShell: Unsupported wrote=0\n"
    "a listing fails in a subcommand: Unsupported wrote=0\n"
    "a listing fails, the sink refusing: Unsupported wrote=0\n"
    "an inactive listing that would fail: OK wrote=12875\n"
    "a hidden listing that would fail: OK wrote=12819\n"
    "a hidden positional's listing: OK wrote=12869\n"
    "a flag's listing: OK wrote=12875\n"
    "a listing for the second value: OK wrote=12875\n"
    "a listing in a hidden subcommand: OK wrote=12819\n"
    "a listing in a disabled group: OK wrote=12819\n"
    "a subcommand: OK wrote=12793\n"
    "a subcommand named by a path: OK wrote=12573\n"
    "the completion option called back with no value: OK recorded=OK\n";

// PROGRAM_NAMES
//   constant: what d_tests_arg_completion_program_names writes, as
// completion_ref.py works it out from the rules arg_completion.h states, in
// parts.
static const char PROGRAM_NAMES[] =
    "== given bash OK\n"
    "# bash completion for tool, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_tool_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_opts() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_tool 'tool'\n"
    "== given zsh OK\n"
    "#compdef tool\n"
    "# zsh completion for tool, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_tool_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_opts() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_tool' ]]; then\n"
    "    compdef _djarg_tool 'tool'\n"
    "    _djarg_tool \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_tool 'tool'\n"
    "else\n"
    "    print -ru2 -- 'tool' 'completion needs compinit first'\n"
    "fi\n"
    "== given fish OK\n"
    "# fish completion for tool, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_tool_cmds \\\n"
    "    '0 s 0 1 0 1 0'\n"
    "set -g -- __djarg_tool_sk\n"
    "set -g -- __djarg_tool_sn\n"
    "set -g -- __djarg_tool_st\n"
    "set -g -- __djarg_tool_sd\n"
    "set -g -- __djarg_tool_ok\n"
    "set -g -- __djarg_tool_on\n"
    "set -g -- __djarg_tool_ov\n"
    "set -g -- __djarg_tool_od\n"
    "set -g -- __djarg_tool_info\n"
    "set -g -- __djarg_tool_pk\n"
    "set -g -- __djarg_tool_pv\n"
    "set -g -- __djarg_tool_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'tool' -e\n"
    "complete -c 'tool' -f -a '(__djarg_tool_complete)'\n"
    "== given powershell OK\n"
    "# PowerShell completion for tool, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 0'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "    )\n"
    "    $oc = @(\n"
    "    )\n"
    "    $ov = @(\n"
    "    )\n"
    "    $od = @(\n"
    "    )\n"
    "    $info = @(\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'tool', "
    "'tool.exe') -ScriptBlock $complete\n"
    "}\n"
    "== from the root bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== from the root zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n";

// PROGRAM_NAMES_2
//   constant: the transcript of d_tests_arg_completion_program_names,
// continued.
static const char PROGRAM_NAMES_2[] =
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== from the root fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 0 1 0'\n"
    "set -g -- __djarg_prog_sk\n"
    "set -g -- __djarg_prog_sn\n"
    "set -g -- __djarg_prog_st\n"
    "set -g -- __djarg_prog_sd\n"
    "set -g -- __djarg_prog_ok\n"
    "set -g -- __djarg_prog_on\n"
    "set -g -- __djarg_prog_ov\n"
    "set -g -- __djarg_prog_od\n"
    "set -g -- __djarg_prog_info\n"
    "set -g -- __djarg_prog_pk\n"
    "set -g -- __djarg_prog_pv\n"
    "set -g -- __djarg_prog_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== from the root powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 0'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "    )\n"
    "    $oc = @(\n"
    "    )\n"
    "    $ov = @(\n"
    "    )\n"
    "    $od = @(\n"
    "    )\n"
    "    $info = @(\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n"
    "== root path bash OK\n"
    "# bash completion for tool, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_tool_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_opts() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_tool 'tool'\n"
    "== root path zsh OK\n"
    "#compdef tool\n"
    "# zsh completion for tool, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_tool_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_opts() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_tool' ]]; then\n"
    "    compdef _djarg_tool 'tool'\n"
    "    _djarg_tool \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_tool 'tool'\n"
    "else\n"
    "    print -ru2 -- 'tool' 'completion needs compinit first'\n"
    "fi\n"
    "== root path fish OK\n"
    "# fish completion for tool, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_tool_cmds \\\n"
    "    '0 s 0 1 0 1 0'\n";

// PROGRAM_NAMES_3
//   constant: the transcript of d_tests_arg_completion_program_names,
// continued.
static const char PROGRAM_NAMES_3[] =
    "set -g -- __djarg_tool_sk\n"
    "set -g -- __djarg_tool_sn\n"
    "set -g -- __djarg_tool_st\n"
    "set -g -- __djarg_tool_sd\n"
    "set -g -- __djarg_tool_ok\n"
    "set -g -- __djarg_tool_on\n"
    "set -g -- __djarg_tool_ov\n"
    "set -g -- __djarg_tool_od\n"
    "set -g -- __djarg_tool_info\n"
    "set -g -- __djarg_tool_pk\n"
    "set -g -- __djarg_tool_pv\n"
    "set -g -- __djarg_tool_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'tool' -e\n"
    "complete -c 'tool' -f -a '(__djarg_tool_complete)'\n"
    "== root path powershell OK\n"
    "# PowerShell completion for tool, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 0'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "    )\n"
    "    $oc = @(\n"
    "    )\n"
    "    $ov = @(\n"
    "    )\n"
    "    $od = @(\n"
    "    )\n"
    "    $info = @(\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'tool', "
    "'tool.exe') -ScriptBlock $complete\n"
    "}\n"
    "== root backslash bash OK\n"
    "# bash completion for Tool, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_Tool_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_Tool_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_Tool_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_Tool_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_Tool_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_Tool_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_Tool_opts() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_Tool_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_Tool 'Tool'\n"
    "== root backslash zsh OK\n"
    "#compdef Tool\n"
    "# zsh completion for Tool, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_Tool_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_Tool_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_Tool_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_Tool_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_Tool_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_Tool_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_Tool_opts() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_Tool_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_Tool' ]]; then\n"
    "    compdef _djarg_Tool 'Tool'\n"
    "    _djarg_Tool \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_Tool 'Tool'\n"
    "else\n"
    "    print -ru2 -- 'Tool' 'completion needs compinit first'\n"
    "fi\n"
    "== root backslash fish OK\n"
    "# fish completion for Tool, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_Tool_cmds \\\n"
    "    '0 s 0 1 0 1 0'\n"
    "set -g -- __djarg_Tool_sk\n"
    "set -g -- __djarg_Tool_sn\n"
    "set -g -- __djarg_Tool_st\n"
    "set -g -- __djarg_Tool_sd\n"
    "set -g -- __djarg_Tool_ok\n"
    "set -g -- __djarg_Tool_on\n"
    "set -g -- __djarg_Tool_ov\n"
    "set -g -- __djarg_Tool_od\n"
    "set -g -- __djarg_Tool_info\n"
    "set -g -- __djarg_Tool_pk\n"
    "set -g -- __djarg_Tool_pv\n"
    "set -g -- __djarg_Tool_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'Tool' -e\n"
    "complete -c 'Tool' -f -a '(__djarg_Tool_complete)'\n"
    "== root backslash powershell OK\n"
    "# PowerShell completion for Tool, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 0'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "    )\n"
    "    $oc = @(\n"
    "    )\n"
    "    $ov = @(\n"
    "    )\n"
    "    $od = @(\n"
    "    )\n"
    "    $info = @(\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'Tool', "
    "'Tool.exe') -ScriptBlock $complete\n"
    "}\n"
    "== root .Exe twice bash OK\n";

// PROGRAM_NAMES_4
//   constant: the transcript of d_tests_arg_completion_program_names,
// continued.
static const char PROGRAM_NAMES_4[] =
    "# bash completion for tool.Exe, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_tool_2eExe_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eExe_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eExe_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eExe_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eExe_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eExe_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eExe_opts() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eExe_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_tool_2eExe 'tool.Exe'\n"
    "== root .Exe twice zsh OK\n"
    "#compdef tool.Exe\n"
    "# zsh completion for tool.Exe, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_tool_2eExe_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eExe_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eExe_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eExe_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eExe_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eExe_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eExe_opts() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eExe_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_tool.Exe' ]]; then\n"
    "    compdef _djarg_tool_2eExe 'tool.Exe'\n"
    "    _djarg_tool_2eExe \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_tool_2eExe 'tool.Exe'\n"
    "else\n"
    "    print -ru2 -- 'tool.Exe' 'completion needs compinit first'\n"
    "fi\n"
    "== root .Exe twice fish OK\n"
    "# fish completion for tool.Exe, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_tool_2eExe_cmds \\\n"
    "    '0 s 0 1 0 1 0'\n"
    "set -g -- __djarg_tool_2eExe_sk\n"
    "set -g -- __djarg_tool_2eExe_sn\n"
    "set -g -- __djarg_tool_2eExe_st\n"
    "set -g -- __djarg_tool_2eExe_sd\n"
    "set -g -- __djarg_tool_2eExe_ok\n"
    "set -g -- __djarg_tool_2eExe_on\n"
    "set -g -- __djarg_tool_2eExe_ov\n"
    "set -g -- __djarg_tool_2eExe_od\n"
    "set -g -- __djarg_tool_2eExe_info\n"
    "set -g -- __djarg_tool_2eExe_pk\n"
    "set -g -- __djarg_tool_2eExe_pv\n"
    "set -g -- __djarg_tool_2eExe_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'tool.Exe' -e\n"
    "complete -c 'tool.Exe' -f -a '(__djarg_tool_2eExe_complete)'\n"
    "== root .Exe twice powershell OK\n"
    "# PowerShell completion for tool.Exe, generated by djinterp's "
    "arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 0'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "    )\n"
    "    $oc = @(\n"
    "    )\n"
    "    $ov = @(\n"
    "    )\n"
    "    $od = @(\n"
    "    )\n"
    "    $info = @(\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'tool.Exe') "
    "-ScriptBlock $complete\n"
    "}\n"
    "== root .exe alone bash OK\n"
    "# bash completion for .exe, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg__2eexe_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2eexe_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2eexe_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2eexe_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2eexe_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2eexe_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2eexe_opts() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2eexe_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n";

// PROGRAM_NAMES_5
//   constant: the transcript of d_tests_arg_completion_program_names,
// continued.
static const char PROGRAM_NAMES_5[] =
    "complete -F _djarg__2eexe '.exe'\n"
    "== root .exe alone zsh OK\n"
    "#compdef .exe\n"
    "# zsh completion for .exe, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg__2eexe_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2eexe_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2eexe_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2eexe_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2eexe_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2eexe_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2eexe_opts() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2eexe_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_.exe' ]]; then\n"
    "    compdef _djarg__2eexe '.exe'\n"
    "    _djarg__2eexe \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg__2eexe '.exe'\n"
    "else\n"
    "    print -ru2 -- '.exe' 'completion needs compinit first'\n"
    "fi\n"
    "== root .exe alone fish OK\n"
    "# fish completion for .exe, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg__2eexe_cmds \\\n"
    "    '0 s 0 1 0 1 0'\n"
    "set -g -- __djarg__2eexe_sk\n"
    "set -g -- __djarg__2eexe_sn\n"
    "set -g -- __djarg__2eexe_st\n"
    "set -g -- __djarg__2eexe_sd\n"
    "set -g -- __djarg__2eexe_ok\n"
    "set -g -- __djarg__2eexe_on\n"
    "set -g -- __djarg__2eexe_ov\n"
    "set -g -- __djarg__2eexe_od\n"
    "set -g -- __djarg__2eexe_info\n"
    "set -g -- __djarg__2eexe_pk\n"
    "set -g -- __djarg__2eexe_pv\n"
    "set -g -- __djarg__2eexe_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c '.exe' -e\n"
    "complete -c '.exe' -f -a '(__djarg__2eexe_complete)'\n"
    "== root .exe alone powershell OK\n"
    "# PowerShell completion for .exe, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 0'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "    )\n"
    "    $oc = @(\n"
    "    )\n"
    "    $ov = @(\n"
    "    )\n"
    "    $od = @(\n"
    "    )\n"
    "    $info = @(\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( '.exe') "
    "-ScriptBlock $complete\n"
    "}\n"
    "== root mixed separators bash OK\n"
    "# bash completion for d, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_d_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_d_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_d_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_d_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_d_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_d_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_d_opts() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_d_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_d 'd'\n"
    "== root mixed separators zsh OK\n"
    "#compdef d\n"
    "# zsh completion for d, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_d_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_d_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_d_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_d_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_d_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_d_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_d_opts() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_d_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_d' ]]; then\n"
    "    compdef _djarg_d 'd'\n"
    "    _djarg_d \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_d 'd'\n"
    "else\n";

// PROGRAM_NAMES_6
//   constant: the transcript of d_tests_arg_completion_program_names,
// continued.
static const char PROGRAM_NAMES_6[] =
    "    print -ru2 -- 'd' 'completion needs compinit first'\n"
    "fi\n"
    "== root mixed separators fish OK\n"
    "# fish completion for d, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_d_cmds \\\n"
    "    '0 s 0 1 0 1 0'\n"
    "set -g -- __djarg_d_sk\n"
    "set -g -- __djarg_d_sn\n"
    "set -g -- __djarg_d_st\n"
    "set -g -- __djarg_d_sd\n"
    "set -g -- __djarg_d_ok\n"
    "set -g -- __djarg_d_on\n"
    "set -g -- __djarg_d_ov\n"
    "set -g -- __djarg_d_od\n"
    "set -g -- __djarg_d_info\n"
    "set -g -- __djarg_d_pk\n"
    "set -g -- __djarg_d_pv\n"
    "set -g -- __djarg_d_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'd' -e\n"
    "complete -c 'd' -f -a '(__djarg_d_complete)'\n"
    "== root mixed separators powershell OK\n"
    "# PowerShell completion for d, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 0'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "    )\n"
    "    $oc = @(\n"
    "    )\n"
    "    $ov = @(\n"
    "    )\n"
    "    $od = @(\n"
    "    )\n"
    "    $info = @(\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'd', 'd.exe') "
    "-ScriptBlock $complete\n"
    "}\n"
    "== given .exe bash OK\n"
    "# bash completion for tool.exe, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_tool_2eexe_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eexe_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eexe_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eexe_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eexe_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eexe_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eexe_opts() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eexe_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_tool_2eexe 'tool.exe'\n"
    "== given .exe zsh OK\n"
    "#compdef tool.exe\n"
    "# zsh completion for tool.exe, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_tool_2eexe_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eexe_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eexe_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eexe_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eexe_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eexe_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eexe_opts() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_2eexe_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_tool.exe' ]]; then\n"
    "    compdef _djarg_tool_2eexe 'tool.exe'\n"
    "    _djarg_tool_2eexe \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_tool_2eexe 'tool.exe'\n"
    "else\n"
    "    print -ru2 -- 'tool.exe' 'completion needs compinit first'\n"
    "fi\n"
    "== given .exe fish OK\n"
    "# fish completion for tool.exe, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_tool_2eexe_cmds \\\n"
    "    '0 s 0 1 0 1 0'\n"
    "set -g -- __djarg_tool_2eexe_sk\n"
    "set -g -- __djarg_tool_2eexe_sn\n"
    "set -g -- __djarg_tool_2eexe_st\n"
    "set -g -- __djarg_tool_2eexe_sd\n"
    "set -g -- __djarg_tool_2eexe_ok\n"
    "set -g -- __djarg_tool_2eexe_on\n"
    "set -g -- __djarg_tool_2eexe_ov\n"
    "set -g -- __djarg_tool_2eexe_od\n"
    "set -g -- __djarg_tool_2eexe_info\n"
    "set -g -- __djarg_tool_2eexe_pk\n"
    "set -g -- __djarg_tool_2eexe_pv\n"
    "set -g -- __djarg_tool_2eexe_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'tool.exe' -e\n"
    "complete -c 'tool.exe' -f -a '(__djarg_tool_2eexe_complete)'\n"
    "== given .exe powershell OK\n"
    "# PowerShell completion for tool.exe, generated by djinterp's "
    "arg_parser.\n";

// PROGRAM_NAMES_7
//   constant: the transcript of d_tests_arg_completion_program_names,
// continued.
static const char PROGRAM_NAMES_7[] =
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 0'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "    )\n"
    "    $oc = @(\n"
    "    )\n"
    "    $ov = @(\n"
    "    )\n"
    "    $od = @(\n"
    "    )\n"
    "    $info = @(\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'tool.exe') "
    "-ScriptBlock $complete\n"
    "}\n"
    "== leading dash bash OK\n"
    "# bash completion for -x, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg__2dx_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2dx_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2dx_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2dx_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2dx_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2dx_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2dx_opts() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2dx_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg__2dx '-x'\n"
    "== leading dash zsh OK\n"
    "# zsh completion for -x, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg__2dx_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2dx_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2dx_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2dx_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2dx_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2dx_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2dx_opts() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__2dx_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_-x' ]]; then\n"
    "    compdef _djarg__2dx '-x'\n"
    "    _djarg__2dx \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg__2dx '-x'\n"
    "else\n"
    "    print -ru2 -- '-x' 'completion needs compinit first'\n"
    "fi\n"
    "== leading dash fish OK\n"
    "# fish completion for -x, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg__2dx_cmds \\\n"
    "    '0 s 0 1 0 1 0'\n"
    "set -g -- __djarg__2dx_sk\n"
    "set -g -- __djarg__2dx_sn\n"
    "set -g -- __djarg__2dx_st\n"
    "set -g -- __djarg__2dx_sd\n"
    "set -g -- __djarg__2dx_ok\n"
    "set -g -- __djarg__2dx_on\n"
    "set -g -- __djarg__2dx_ov\n"
    "set -g -- __djarg__2dx_od\n"
    "set -g -- __djarg__2dx_info\n"
    "set -g -- __djarg__2dx_pk\n"
    "set -g -- __djarg__2dx_pv\n"
    "set -g -- __djarg__2dx_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c '-x' -e\n"
    "complete -c '-x' -f -a '(__djarg__2dx_complete)'\n"
    "== leading dash powershell OK\n"
    "# PowerShell completion for -x, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 0'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "    )\n"
    "    $oc = @(\n"
    "    )\n"
    "    $ov = @(\n"
    "    )\n"
    "    $od = @(\n"
    "    )\n"
    "    $info = @(\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( '-x', '-x.exe') "
    "-ScriptBlock $complete\n"
    "}\n"
    "== space bash OK\n"
    "# bash completion for two words, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_two_20words_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_two_20words_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_two_20words_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_two_20words_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_two_20words_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_two_20words_subs() {\n"
    "    case $1 in\n";

// PROGRAM_NAMES_8
//   constant: the transcript of d_tests_arg_completion_program_names,
// continued.
static const char PROGRAM_NAMES_8[] =
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_two_20words_opts() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_two_20words_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_two_20words 'two words'\n"
    "== space zsh OK\n"
    "# zsh completion for two words, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_two_20words_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_two_20words_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_two_20words_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_two_20words_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_two_20words_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_two_20words_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_two_20words_opts() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_two_20words_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_two words' ]]; then\n"
    "    compdef _djarg_two_20words 'two words'\n"
    "    _djarg_two_20words \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_two_20words 'two words'\n"
    "else\n"
    "    print -ru2 -- 'two words' 'completion needs compinit first'\n"
    "fi\n"
    "== space fish OK\n"
    "# fish completion for two words, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_two_20words_cmds \\\n"
    "    '0 s 0 1 0 1 0'\n"
    "set -g -- __djarg_two_20words_sk\n"
    "set -g -- __djarg_two_20words_sn\n"
    "set -g -- __djarg_two_20words_st\n"
    "set -g -- __djarg_two_20words_sd\n"
    "set -g -- __djarg_two_20words_ok\n"
    "set -g -- __djarg_two_20words_on\n"
    "set -g -- __djarg_two_20words_ov\n"
    "set -g -- __djarg_two_20words_od\n"
    "set -g -- __djarg_two_20words_info\n"
    "set -g -- __djarg_two_20words_pk\n"
    "set -g -- __djarg_two_20words_pv\n"
    "set -g -- __djarg_two_20words_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'two words' -e\n"
    "complete -c 'two words' -f -a '(__djarg_two_20words_complete)'\n"
    "== space powershell OK\n"
    "# PowerShell completion for two words, generated by djinterp's "
    "arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 0'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "    )\n"
    "    $oc = @(\n"
    "    )\n"
    "    $ov = @(\n"
    "    )\n"
    "    $od = @(\n"
    "    )\n"
    "    $info = @(\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'two words', 'two "
    "words.exe') -ScriptBlock $complete\n"
    "}\n"
    "== leading space bash OK\n"
    "# bash completion for  x, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg__20x_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__20x_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__20x_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__20x_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__20x_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__20x_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__20x_opts() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__20x_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg__20x ' x'\n"
    "== leading space zsh OK\n"
    "# zsh completion for  x, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg__20x_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__20x_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__20x_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__20x_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__20x_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__20x_subs() {\n"
    "    case $1 in\n";

// PROGRAM_NAMES_9
//   constant: the transcript of d_tests_arg_completion_program_names,
// continued.
static const char PROGRAM_NAMES_9[] =
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__20x_opts() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__20x_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_ x' ]]; then\n"
    "    compdef _djarg__20x ' x'\n"
    "    _djarg__20x \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg__20x ' x'\n"
    "else\n"
    "    print -ru2 -- ' x' 'completion needs compinit first'\n"
    "fi\n"
    "== leading space fish OK\n"
    "# fish completion for  x, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg__20x_cmds \\\n"
    "    '0 s 0 1 0 1 0'\n"
    "set -g -- __djarg__20x_sk\n"
    "set -g -- __djarg__20x_sn\n"
    "set -g -- __djarg__20x_st\n"
    "set -g -- __djarg__20x_sd\n"
    "set -g -- __djarg__20x_ok\n"
    "set -g -- __djarg__20x_on\n"
    "set -g -- __djarg__20x_ov\n"
    "set -g -- __djarg__20x_od\n"
    "set -g -- __djarg__20x_info\n"
    "set -g -- __djarg__20x_pk\n"
    "set -g -- __djarg__20x_pv\n"
    "set -g -- __djarg__20x_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c ' x' -e\n"
    "complete -c ' x' -f -a '(__djarg__20x_complete)'\n"
    "== leading space powershell OK\n"
    "# PowerShell completion for  x, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 0'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "    )\n"
    "    $oc = @(\n"
    "    )\n"
    "    $ov = @(\n"
    "    )\n"
    "    $od = @(\n"
    "    )\n"
    "    $info = @(\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( ' x', ' x.exe') "
    "-ScriptBlock $complete\n"
    "}\n"
    "== tab bash OK\n"
    "# bash completion for a b, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_a_09b_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_09b_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_09b_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_09b_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_09b_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_09b_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_09b_opts() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_09b_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_a_09b 'a\x09" "b'\n"
    "== tab zsh OK\n"
    "# zsh completion for a b, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_a_09b_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_09b_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_09b_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_09b_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_09b_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_09b_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_09b_opts() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_09b_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_a\x09" "b' ]]; then\n"
    "    compdef _djarg_a_09b 'a\x09" "b'\n"
    "    _djarg_a_09b \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_a_09b 'a\x09" "b'\n"
    "else\n"
    "    print -ru2 -- 'a b' 'completion needs compinit first'\n"
    "fi\n"
    "== tab fish OK\n"
    "# fish completion for a b, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_a_09b_cmds \\\n"
    "    '0 s 0 1 0 1 0'\n"
    "set -g -- __djarg_a_09b_sk\n"
    "set -g -- __djarg_a_09b_sn\n"
    "set -g -- __djarg_a_09b_st\n"
    "set -g -- __djarg_a_09b_sd\n"
    "set -g -- __djarg_a_09b_ok\n"
    "set -g -- __djarg_a_09b_on\n"
    "set -g -- __djarg_a_09b_ov\n"
    "set -g -- __djarg_a_09b_od\n"
    "set -g -- __djarg_a_09b_info\n"
    "set -g -- __djarg_a_09b_pk\n"
    "set -g -- __djarg_a_09b_pv\n"
    "set -g -- __djarg_a_09b_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'a\x09" "b' -e\n";

// PROGRAM_NAMES_10
//   constant: the transcript of d_tests_arg_completion_program_names,
// continued.
static const char PROGRAM_NAMES_10[] =
    "complete -c 'a\x09" "b' -f -a '(__djarg_a_09b_complete)'\n"
    "== tab powershell OK\n"
    "# PowerShell completion for a?b, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 0'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "    )\n"
    "    $oc = @(\n"
    "    )\n"
    "    $ov = @(\n"
    "    )\n"
    "    $od = @(\n"
    "    )\n"
    "    $info = @(\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( ('' + 'a' + "
    "[char]0x0009 + 'b'), ('' + 'a' + [char]0x0009 + 'b.exe')) -ScriptBlock "
    "$complete\n"
    "}\n"
    "== quote bash OK\n"
    "# bash completion for it'\\''s, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_it_27s_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_it_27s_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_it_27s_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_it_27s_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_it_27s_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_it_27s_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_it_27s_opts() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_it_27s_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_it_27s 'it'\\''s'\n"
    "== quote zsh OK\n"
    "#compdef it's\n"
    "# zsh completion for it'\\''s, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_it_27s_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_it_27s_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_it_27s_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_it_27s_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_it_27s_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_it_27s_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_it_27s_opts() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_it_27s_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_it'\\''s' ]]; then\n"
    "    compdef _djarg_it_27s 'it'\\''s'\n"
    "    _djarg_it_27s \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_it_27s 'it'\\''s'\n"
    "else\n"
    "    print -ru2 -- 'it'\\''s' 'completion needs compinit first'\n"
    "fi\n"
    "== quote fish OK\n"
    "# fish completion for it's, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_it_27s_cmds \\\n"
    "    '0 s 0 1 0 1 0'\n"
    "set -g -- __djarg_it_27s_sk\n"
    "set -g -- __djarg_it_27s_sn\n"
    "set -g -- __djarg_it_27s_st\n"
    "set -g -- __djarg_it_27s_sd\n"
    "set -g -- __djarg_it_27s_ok\n"
    "set -g -- __djarg_it_27s_on\n"
    "set -g -- __djarg_it_27s_ov\n"
    "set -g -- __djarg_it_27s_od\n"
    "set -g -- __djarg_it_27s_info\n"
    "set -g -- __djarg_it_27s_pk\n"
    "set -g -- __djarg_it_27s_pv\n"
    "set -g -- __djarg_it_27s_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'it\\'s' -e\n"
    "complete -c 'it\\'s' -f -a '(__djarg_it_27s_complete)'\n"
    "== quote powershell OK\n"
    "# PowerShell completion for it's, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 0'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "    )\n"
    "    $oc = @(\n"
    "    )\n"
    "    $ov = @(\n"
    "    )\n"
    "    $od = @(\n"
    "    )\n"
    "    $info = @(\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'it''s', "
    "'it''s.exe') -ScriptBlock $complete\n"
    "}\n"
    "== backslash bash OK\n"
    "# bash completion for a\\b, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_a_5cb_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_5cb_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n";

// PROGRAM_NAMES_11
//   constant: the transcript of d_tests_arg_completion_program_names,
// continued.
static const char PROGRAM_NAMES_11[] =
    "    esac\n"
    "}\n"
    "_djarg_a_5cb_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_5cb_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_5cb_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_5cb_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_5cb_opts() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_5cb_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_a_5cb 'a\\b'\n"
    "== backslash zsh OK\n"
    "#compdef a\\b\n"
    "# zsh completion for a\\b, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_a_5cb_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_5cb_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_5cb_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_5cb_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_5cb_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_5cb_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_5cb_opts() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_5cb_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_a\\b' ]]; then\n"
    "    compdef _djarg_a_5cb 'a\\b'\n"
    "    _djarg_a_5cb \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_a_5cb 'a\\b'\n"
    "else\n"
    "    print -ru2 -- 'a\\b' 'completion needs compinit first'\n"
    "fi\n"
    "== backslash fish OK\n"
    "# fish completion for a\\b, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_a_5cb_cmds \\\n"
    "    '0 s 0 1 0 1 0'\n"
    "set -g -- __djarg_a_5cb_sk\n"
    "set -g -- __djarg_a_5cb_sn\n"
    "set -g -- __djarg_a_5cb_st\n"
    "set -g -- __djarg_a_5cb_sd\n"
    "set -g -- __djarg_a_5cb_ok\n"
    "set -g -- __djarg_a_5cb_on\n"
    "set -g -- __djarg_a_5cb_ov\n"
    "set -g -- __djarg_a_5cb_od\n"
    "set -g -- __djarg_a_5cb_info\n"
    "set -g -- __djarg_a_5cb_pk\n"
    "set -g -- __djarg_a_5cb_pv\n"
    "set -g -- __djarg_a_5cb_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'a\\\\b' -e\n"
    "complete -c 'a\\\\b' -f -a '(__djarg_a_5cb_complete)'\n"
    "== backslash powershell OK\n"
    "# PowerShell completion for a\\b, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 0'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "    )\n"
    "    $oc = @(\n"
    "    )\n"
    "    $ov = @(\n"
    "    )\n"
    "    $od = @(\n"
    "    )\n"
    "    $info = @(\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'a\\b', "
    "'a\\b.exe') -ScriptBlock $complete\n"
    "}\n"
    "== UTF-8 bash OK\n"
    "# bash completion for caf\xc3\xa9, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_caf_c3_a9_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_caf_c3_a9_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_caf_c3_a9_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_caf_c3_a9_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_caf_c3_a9_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_caf_c3_a9_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_caf_c3_a9_opts() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_caf_c3_a9_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_caf_c3_a9 'caf\xc3\xa9'\n"
    "== UTF-8 zsh OK\n"
    "#compdef caf\xc3\xa9\n"
    "# zsh completion for caf\xc3\xa9, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_caf_c3_a9_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_caf_c3_a9_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_caf_c3_a9_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_caf_c3_a9_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n";

// PROGRAM_NAMES_12
//   constant: the transcript of d_tests_arg_completion_program_names,
// continued.
static const char PROGRAM_NAMES_12[] =
    "    esac\n"
    "}\n"
    "_djarg_caf_c3_a9_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_caf_c3_a9_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_caf_c3_a9_opts() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_caf_c3_a9_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_caf\xc3\xa9' ]]; then\n"
    "    compdef _djarg_caf_c3_a9 'caf\xc3\xa9'\n"
    "    _djarg_caf_c3_a9 \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_caf_c3_a9 'caf\xc3\xa9'\n"
    "else\n"
    "    print -ru2 -- 'caf\xc3\xa9' 'completion needs compinit first'\n"
    "fi\n"
    "== UTF-8 fish OK\n"
    "# fish completion for caf\xc3\xa9, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_caf_c3_a9_cmds \\\n"
    "    '0 s 0 1 0 1 0'\n"
    "set -g -- __djarg_caf_c3_a9_sk\n"
    "set -g -- __djarg_caf_c3_a9_sn\n"
    "set -g -- __djarg_caf_c3_a9_st\n"
    "set -g -- __djarg_caf_c3_a9_sd\n"
    "set -g -- __djarg_caf_c3_a9_ok\n"
    "set -g -- __djarg_caf_c3_a9_on\n"
    "set -g -- __djarg_caf_c3_a9_ov\n"
    "set -g -- __djarg_caf_c3_a9_od\n"
    "set -g -- __djarg_caf_c3_a9_info\n"
    "set -g -- __djarg_caf_c3_a9_pk\n"
    "set -g -- __djarg_caf_c3_a9_pv\n"
    "set -g -- __djarg_caf_c3_a9_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'caf\xc3\xa9' -e\n"
    "complete -c 'caf\xc3\xa9' -f -a '(__djarg_caf_c3_a9_complete)'\n"
    "== UTF-8 powershell OK\n"
    "# PowerShell completion for caf??, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 0'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "    )\n"
    "    $oc = @(\n"
    "    )\n"
    "    $ov = @(\n"
    "    )\n"
    "    $od = @(\n"
    "    )\n"
    "    $info = @(\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( ('' + 'caf' + "
    "[char]0x00E9), ('' + 'caf' + [char]0x00E9 + '.exe')) -ScriptBlock "
    "$complete\n"
    "}\n"
    "== astral bash OK\n"
    "# bash completion for \xf0\x9f\x98\x80, generated by djinterp's "
    "arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg__f0_9f_98_80_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__f0_9f_98_80_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__f0_9f_98_80_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__f0_9f_98_80_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__f0_9f_98_80_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__f0_9f_98_80_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__f0_9f_98_80_opts() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__f0_9f_98_80_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg__f0_9f_98_80 '\xf0\x9f\x98\x80'\n"
    "== astral zsh OK\n"
    "#compdef \xf0\x9f\x98\x80\n"
    "# zsh completion for \xf0\x9f\x98\x80, generated by djinterp's "
    "arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg__f0_9f_98_80_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__f0_9f_98_80_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__f0_9f_98_80_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__f0_9f_98_80_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__f0_9f_98_80_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__f0_9f_98_80_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__f0_9f_98_80_opts() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg__f0_9f_98_80_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_\xf0\x9f\x98\x80' ]]; then\n"
    "    compdef _djarg__f0_9f_98_80 '\xf0\x9f\x98\x80'\n"
    "    _djarg__f0_9f_98_80 \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg__f0_9f_98_80 '\xf0\x9f\x98\x80'\n"
    "else\n"
    "    print -ru2 -- '\xf0\x9f\x98\x80' 'completion needs compinit first'\n"
    "fi\n"
    "== astral fish OK\n"
    "# fish completion for \xf0\x9f\x98\x80, generated by djinterp's "
    "arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n";

// PROGRAM_NAMES_13
//   constant: the transcript of d_tests_arg_completion_program_names,
// continued.
static const char PROGRAM_NAMES_13[] =
    "set -g -- __djarg__f0_9f_98_80_cmds \\\n"
    "    '0 s 0 1 0 1 0'\n"
    "set -g -- __djarg__f0_9f_98_80_sk\n"
    "set -g -- __djarg__f0_9f_98_80_sn\n"
    "set -g -- __djarg__f0_9f_98_80_st\n"
    "set -g -- __djarg__f0_9f_98_80_sd\n"
    "set -g -- __djarg__f0_9f_98_80_ok\n"
    "set -g -- __djarg__f0_9f_98_80_on\n"
    "set -g -- __djarg__f0_9f_98_80_ov\n"
    "set -g -- __djarg__f0_9f_98_80_od\n"
    "set -g -- __djarg__f0_9f_98_80_info\n"
    "set -g -- __djarg__f0_9f_98_80_pk\n"
    "set -g -- __djarg__f0_9f_98_80_pv\n"
    "set -g -- __djarg__f0_9f_98_80_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c '\xf0\x9f\x98\x80' -e\n"
    "complete -c '\xf0\x9f\x98\x80' -f -a '(__djarg__f0_9f_98_80_complete)'\n"
    "== astral powershell OK\n"
    "# PowerShell completion for ????, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 0'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "    )\n"
    "    $oc = @(\n"
    "    )\n"
    "    $ov = @(\n"
    "    )\n"
    "    $od = @(\n"
    "    )\n"
    "    $info = @(\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( ('' + [char]0xD83D "
    "+ [char]0xDE00), ('' + [char]0xD83D + [char]0xDE00 + '.exe')) "
    "-ScriptBlock $complete\n"
    "}\n"
    "== stray byte bash OK\n"
    "# bash completion for x\xffy, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_x_ffy_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_x_ffy_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_x_ffy_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_x_ffy_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_x_ffy_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_x_ffy_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_x_ffy_opts() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_x_ffy_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_x_ffy 'x\xffy'\n"
    "== stray byte zsh OK\n"
    "#compdef x\xffy\n"
    "# zsh completion for x\xffy, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_x_ffy_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_x_ffy_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_x_ffy_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_x_ffy_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_x_ffy_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_x_ffy_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_x_ffy_opts() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_x_ffy_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_x\xffy' ]]; then\n"
    "    compdef _djarg_x_ffy 'x\xffy'\n"
    "    _djarg_x_ffy \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_x_ffy 'x\xffy'\n"
    "else\n"
    "    print -ru2 -- 'x\xffy' 'completion needs compinit first'\n"
    "fi\n"
    "== stray byte fish OK\n"
    "# fish completion for x\xffy, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_x_ffy_cmds \\\n"
    "    '0 s 0 1 0 1 0'\n"
    "set -g -- __djarg_x_ffy_sk\n"
    "set -g -- __djarg_x_ffy_sn\n"
    "set -g -- __djarg_x_ffy_st\n"
    "set -g -- __djarg_x_ffy_sd\n"
    "set -g -- __djarg_x_ffy_ok\n"
    "set -g -- __djarg_x_ffy_on\n"
    "set -g -- __djarg_x_ffy_ov\n"
    "set -g -- __djarg_x_ffy_od\n"
    "set -g -- __djarg_x_ffy_info\n"
    "set -g -- __djarg_x_ffy_pk\n"
    "set -g -- __djarg_x_ffy_pv\n"
    "set -g -- __djarg_x_ffy_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'x\xffy' -e\n"
    "complete -c 'x\xffy' -f -a '(__djarg_x_ffy_complete)'\n"
    "== stray byte powershell OK\n"
    "# PowerShell completion for x?y, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 0'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "    )\n"
    "    $oc = @(\n"
    "    )\n"
    "    $ov = @(\n"
    "    )\n"
    "    $od = @(\n"
    "    )\n"
    "    $info = @(\n"
    "    )\n";

// PROGRAM_NAMES_14
//   constant: the transcript of d_tests_arg_completion_program_names,
// continued.
static const char PROGRAM_NAMES_14[] =
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( ('' + 'x' + "
    "[char]0xFFFD + 'y'), ('' + 'x' + [char]0xFFFD + 'y.exe')) -ScriptBlock "
    "$complete\n"
    "}\n"
    "== DEL bash OK\n"
    "# bash completion for a b, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_a_7fb_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_7fb_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_7fb_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_7fb_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_7fb_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_7fb_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_7fb_opts() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_7fb_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_a_7fb 'a\x7f" "b'\n"
    "== DEL zsh OK\n"
    "#compdef a\x7f" "b\n"
    "# zsh completion for a b, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_a_7fb_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_7fb_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_7fb_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_7fb_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_7fb_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_7fb_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_7fb_opts() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_a_7fb_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_a\x7f" "b' ]]; then\n"
    "    compdef _djarg_a_7fb 'a\x7f" "b'\n"
    "    _djarg_a_7fb \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_a_7fb 'a\x7f" "b'\n"
    "else\n"
    "    print -ru2 -- 'a b' 'completion needs compinit first'\n"
    "fi\n"
    "== DEL fish OK\n"
    "# fish completion for a b, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_a_7fb_cmds \\\n"
    "    '0 s 0 1 0 1 0'\n"
    "set -g -- __djarg_a_7fb_sk\n"
    "set -g -- __djarg_a_7fb_sn\n"
    "set -g -- __djarg_a_7fb_st\n"
    "set -g -- __djarg_a_7fb_sd\n"
    "set -g -- __djarg_a_7fb_ok\n"
    "set -g -- __djarg_a_7fb_on\n"
    "set -g -- __djarg_a_7fb_ov\n"
    "set -g -- __djarg_a_7fb_od\n"
    "set -g -- __djarg_a_7fb_info\n"
    "set -g -- __djarg_a_7fb_pk\n"
    "set -g -- __djarg_a_7fb_pv\n"
    "set -g -- __djarg_a_7fb_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'a\x7f" "b' -e\n"
    "complete -c 'a\x7f" "b' -f -a '(__djarg_a_7fb_complete)'\n"
    "== DEL powershell OK\n"
    "# PowerShell completion for a?b, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 0'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "    )\n"
    "    $oc = @(\n"
    "    )\n"
    "    $ov = @(\n"
    "    )\n"
    "    $od = @(\n"
    "    )\n"
    "    $info = @(\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( ('' + 'a' + "
    "[char]0x007F + 'b'), ('' + 'a' + [char]0x007F + 'b.exe')) -ScriptBlock "
    "$complete\n"
    "}\n"
    "== short .exe bash OK\n"
    "# bash completion for ab, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_ab_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_ab_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_ab_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_ab_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_ab_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_ab_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_ab_opts() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_ab_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n";

// PROGRAM_NAMES_15
//   constant: the transcript of d_tests_arg_completion_program_names,
// continued.
static const char PROGRAM_NAMES_15[] =
    "complete -F _djarg_ab 'ab'\n"
    "== short .exe zsh OK\n"
    "#compdef ab\n"
    "# zsh completion for ab, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_ab_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_ab_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_ab_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_ab_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_ab_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_ab_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_ab_opts() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_ab_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_ab' ]]; then\n"
    "    compdef _djarg_ab 'ab'\n"
    "    _djarg_ab \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_ab 'ab'\n"
    "else\n"
    "    print -ru2 -- 'ab' 'completion needs compinit first'\n"
    "fi\n"
    "== short .exe fish OK\n"
    "# fish completion for ab, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_ab_cmds \\\n"
    "    '0 s 0 1 0 1 0'\n"
    "set -g -- __djarg_ab_sk\n"
    "set -g -- __djarg_ab_sn\n"
    "set -g -- __djarg_ab_st\n"
    "set -g -- __djarg_ab_sd\n"
    "set -g -- __djarg_ab_ok\n"
    "set -g -- __djarg_ab_on\n"
    "set -g -- __djarg_ab_ov\n"
    "set -g -- __djarg_ab_od\n"
    "set -g -- __djarg_ab_info\n"
    "set -g -- __djarg_ab_pk\n"
    "set -g -- __djarg_ab_pv\n"
    "set -g -- __djarg_ab_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'ab' -e\n"
    "complete -c 'ab' -f -a '(__djarg_ab_complete)'\n"
    "== short .exe powershell OK\n"
    "# PowerShell completion for ab, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 0'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "    )\n"
    "    $oc = @(\n"
    "    )\n"
    "    $ov = @(\n"
    "    )\n"
    "    $od = @(\n"
    "    )\n"
    "    $info = @(\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'ab', 'ab.exe') "
    "-ScriptBlock $complete\n"
    "}\n"
    "== EXE bash OK\n"
    "# bash completion for TOOL.EXE, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_TOOL_2eEXE_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_TOOL_2eEXE_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_TOOL_2eEXE_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_TOOL_2eEXE_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_TOOL_2eEXE_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_TOOL_2eEXE_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_TOOL_2eEXE_opts() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_TOOL_2eEXE_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_TOOL_2eEXE 'TOOL.EXE'\n"
    "== EXE zsh OK\n"
    "#compdef TOOL.EXE\n"
    "# zsh completion for TOOL.EXE, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_TOOL_2eEXE_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_TOOL_2eEXE_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_TOOL_2eEXE_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_TOOL_2eEXE_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_TOOL_2eEXE_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_TOOL_2eEXE_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_TOOL_2eEXE_opts() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_TOOL_2eEXE_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_TOOL.EXE' ]]; then\n"
    "    compdef _djarg_TOOL_2eEXE 'TOOL.EXE'\n"
    "    _djarg_TOOL_2eEXE \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n";

// PROGRAM_NAMES_16
//   constant: the transcript of d_tests_arg_completion_program_names,
// continued.
static const char PROGRAM_NAMES_16[] =
    "    compdef _djarg_TOOL_2eEXE 'TOOL.EXE'\n"
    "else\n"
    "    print -ru2 -- 'TOOL.EXE' 'completion needs compinit first'\n"
    "fi\n"
    "== EXE fish OK\n"
    "# fish completion for TOOL.EXE, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_TOOL_2eEXE_cmds \\\n"
    "    '0 s 0 1 0 1 0'\n"
    "set -g -- __djarg_TOOL_2eEXE_sk\n"
    "set -g -- __djarg_TOOL_2eEXE_sn\n"
    "set -g -- __djarg_TOOL_2eEXE_st\n"
    "set -g -- __djarg_TOOL_2eEXE_sd\n"
    "set -g -- __djarg_TOOL_2eEXE_ok\n"
    "set -g -- __djarg_TOOL_2eEXE_on\n"
    "set -g -- __djarg_TOOL_2eEXE_ov\n"
    "set -g -- __djarg_TOOL_2eEXE_od\n"
    "set -g -- __djarg_TOOL_2eEXE_info\n"
    "set -g -- __djarg_TOOL_2eEXE_pk\n"
    "set -g -- __djarg_TOOL_2eEXE_pv\n"
    "set -g -- __djarg_TOOL_2eEXE_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'TOOL.EXE' -e\n"
    "complete -c 'TOOL.EXE' -f -a '(__djarg_TOOL_2eEXE_complete)'\n"
    "== EXE powershell OK\n"
    "# PowerShell completion for TOOL.EXE, generated by djinterp's "
    "arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 0'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "    )\n"
    "    $oc = @(\n"
    "    )\n"
    "    $ov = @(\n"
    "    )\n"
    "    $od = @(\n"
    "    )\n"
    "    $info = @(\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'TOOL.EXE') "
    "-ScriptBlock $complete\n"
    "}\n";

// TREE_ROOT
//   constant: what d_tests_arg_completion_tree_root writes, as
// completion_ref.py works it out from the rules arg_completion.h states, in
// parts.
static const char TREE_ROOT[] =
    "== alone bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== alone zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== alone fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 0 1 2'\n"
    "set -g -- __djarg_prog_sk\n"
    "set -g -- __djarg_prog_sn\n"
    "set -g -- __djarg_prog_st\n"
    "set -g -- __djarg_prog_sd\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0'\n"
    "set -g -- __djarg_prog_pk\n"
    "set -g -- __djarg_prog_pv\n"
    "set -g -- __djarg_prog_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== alone powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 2'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n"
    "== no help bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n";

// TREE_ROOT_2
//   constant: the transcript of d_tests_arg_completion_tree_root, continued.
static const char TREE_ROOT_2[] =
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== no help zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== no help fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 0 1 0'\n"
    "set -g -- __djarg_prog_sk\n"
    "set -g -- __djarg_prog_sn\n"
    "set -g -- __djarg_prog_st\n"
    "set -g -- __djarg_prog_sd\n"
    "set -g -- __djarg_prog_ok\n"
    "set -g -- __djarg_prog_on\n"
    "set -g -- __djarg_prog_ov\n"
    "set -g -- __djarg_prog_od\n"
    "set -g -- __djarg_prog_info\n"
    "set -g -- __djarg_prog_pk\n"
    "set -g -- __djarg_prog_pv\n"
    "set -g -- __djarg_prog_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== no help powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 0'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "    )\n"
    "    $oc = @(\n"
    "    )\n"
    "    $ov = @(\n"
    "    )\n"
    "    $od = @(\n"
    "    )\n"
    "    $info = @(\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n"
    "== help renamed bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-?') r=0 ;;\n"
    "        '0:--usage') r=0 ;;\n"
    "        '0:--HELP') r=0 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-?' '--usage' '--HELP' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== help renamed zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-?') r=0 ;;\n"
    "        '0:--usage') r=0 ;;\n"
    "        '0:--HELP') r=0 ;;\n"
    "        *) r= ;;\n"
    "    esac\n";

// TREE_ROOT_3
//   constant: the transcript of d_tests_arg_completion_tree_root, continued.
static const char TREE_ROOT_3[] =
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-?' '--usage' '--HELP' ) d=( 'Show it' 'Show it' 'Show "
    "it' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== help renamed fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 0 1 3'\n"
    "set -g -- __djarg_prog_sk\n"
    "set -g -- __djarg_prog_sn\n"
    "set -g -- __djarg_prog_st\n"
    "set -g -- __djarg_prog_sd\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-?' \\\n"
    "    '0:--usage' \\\n"
    "    '0:--HELP'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-?' \\\n"
    "    '--usage' \\\n"
    "    '--HELP'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    0\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Show it' \\\n"
    "    'Show it' \\\n"
    "    'Show it'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0'\n"
    "set -g -- __djarg_prog_pk\n"
    "set -g -- __djarg_prog_pv\n"
    "set -g -- __djarg_prog_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== help renamed powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 3'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "        '-?'\n"
    "        '--usage'\n"
    "        '--HELP'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $od = @(\n"
    "        'Show it'\n"
    "        'Show it'\n"
    "        'Show it'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n"
    "== help-all bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--help-all') r=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--help-all' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== help-all zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--help-all') r=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n";

// TREE_ROOT_4
//   constant: the transcript of d_tests_arg_completion_tree_root, continued.
static const char TREE_ROOT_4[] =
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--help-all' ) d=( 'Print this help message "
    "and exit' 'Print this help message and exit' 'Expand all help' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== help-all fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 0 1 3'\n"
    "set -g -- __djarg_prog_sk\n"
    "set -g -- __djarg_prog_sn\n"
    "set -g -- __djarg_prog_st\n"
    "set -g -- __djarg_prog_sd\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '0:--help-all'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '--help-all'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    1\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Expand all help'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0'\n"
    "set -g -- __djarg_prog_pk\n"
    "set -g -- __djarg_prog_pv\n"
    "set -g -- __djarg_prog_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== help-all powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 3'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '--help-all'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Expand all help'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n"
    "== described bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:-x') r=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '-x' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== described zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:-x') r=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n";

// TREE_ROOT_5
//   constant: the transcript of d_tests_arg_completion_tree_root, continued.
static const char TREE_ROOT_5[] =
    "        0) c=( '-h' '--help' '-x' ) d=( 'Print this help message and "
    "exit' 'Print this help message and exit' 'One' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== described fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 0 1 3'\n"
    "set -g -- __djarg_prog_sk\n"
    "set -g -- __djarg_prog_sn\n"
    "set -g -- __djarg_prog_st\n"
    "set -g -- __djarg_prog_sd\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '0:-x'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-x'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    1\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'One'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0'\n"
    "set -g -- __djarg_prog_pk\n"
    "set -g -- __djarg_prog_pv\n"
    "set -g -- __djarg_prog_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== described powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 3'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-x'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'One'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n";

// TREE_OPTIONS
//   constant: what d_tests_arg_completion_tree_options writes, as
// completion_ref.py works it out from the rules arg_completion.h states, in
// parts.
static const char TREE_OPTIONS[] =
    "== names bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:-v') r=1 ;;\n"
    "        '0:--verbose') r=1 ;;\n"
    "        '0:-q') r=2 ;;\n"
    "        '0:--dry-run') r=3 ;;\n"
    "        '0:-a') r=4 ;;\n"
    "        '0:-b') r=4 ;;\n"
    "        '0:--alpha') r=4 ;;\n"
    "        '0:--beta') r=4 ;;\n"
    "        '0:-c') r=5 ;;\n"
    "        '0:--beta2') r=5 ;;\n"
    "        '0:-z') r=6 ;;\n"
    "        '0:-y') r=6 ;;\n"
    "        '0:--zed') r=6 ;;\n"
    "        '0:--why') r=6 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=0 x=0 k=- ;;\n"
    "        2) n=0 x=0 k=- ;;\n"
    "        3) n=0 x=0 k=- ;;\n"
    "        4) n=1 x=1 k=f ;;\n"
    "        5) n=1 x=1 k=f ;;\n"
    "        6) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '-v' '--verbose' '-q' '--dry-run' '-a' '-b' "
    "'--alpha' '--beta' '-c' '--beta2' '-z' '-y' '--zed' '--why' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== names zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:-v') r=1 ;;\n"
    "        '0:--verbose') r=1 ;;\n"
    "        '0:-q') r=2 ;;\n"
    "        '0:--dry-run') r=3 ;;\n"
    "        '0:-a') r=4 ;;\n"
    "        '0:-b') r=4 ;;\n"
    "        '0:--alpha') r=4 ;;\n"
    "        '0:--beta') r=4 ;;\n"
    "        '0:-c') r=5 ;;\n"
    "        '0:--beta2') r=5 ;;\n"
    "        '0:-z') r=6 ;;\n"
    "        '0:-y') r=6 ;;\n"
    "        '0:--zed') r=6 ;;\n"
    "        '0:--why') r=6 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=0 x=0 k=- ;;\n"
    "        2) n=0 x=0 k=- ;;\n"
    "        3) n=0 x=0 k=- ;;\n"
    "        4) n=1 x=1 k=f ;;\n"
    "        5) n=1 x=1 k=f ;;\n"
    "        6) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '-v' '--verbose' '-q' '--dry-run' '-a' '-b' "
    "'--alpha' '--beta' '-c' '--beta2' '-z' '-y' '--zed' '--why' ) d=( 'Print "
    "this help message and exit' 'Print this help message and exit' 'Say more' "
    "'Say more' 'Say less' 'Pretend' 'Two of each' 'Two of each' 'Two of each' "
    "'Two of each' 'Long before short' 'Long before short' '' '' '' '' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== names fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 0 1 16'\n"
    "set -g -- __djarg_prog_sk\n"
    "set -g -- __djarg_prog_sn\n"
    "set -g -- __djarg_prog_st\n"
    "set -g -- __djarg_prog_sd\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '0:-v' \\\n"
    "    '0:--verbose' \\\n"
    "    '0:-q' \\\n"
    "    '0:--dry-run' \\\n"
    "    '0:-a' \\\n"
    "    '0:-b' \\\n"
    "    '0:--alpha' \\\n"
    "    '0:--beta' \\\n"
    "    '0:-c' \\\n";

// TREE_OPTIONS_2
//   constant: the transcript of d_tests_arg_completion_tree_options, continued.
static const char TREE_OPTIONS_2[] =
    "    '0:--beta2' \\\n"
    "    '0:-z' \\\n"
    "    '0:-y' \\\n"
    "    '0:--zed' \\\n"
    "    '0:--why'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-v' \\\n"
    "    '--verbose' \\\n"
    "    '-q' \\\n"
    "    '--dry-run' \\\n"
    "    '-a' \\\n"
    "    '-b' \\\n"
    "    '--alpha' \\\n"
    "    '--beta' \\\n"
    "    '-c' \\\n"
    "    '--beta2' \\\n"
    "    '-z' \\\n"
    "    '-y' \\\n"
    "    '--zed' \\\n"
    "    '--why'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    1 \\\n"
    "    1 \\\n"
    "    2 \\\n"
    "    3 \\\n"
    "    4 \\\n"
    "    4 \\\n"
    "    4 \\\n"
    "    4 \\\n"
    "    5 \\\n"
    "    5 \\\n"
    "    6 \\\n"
    "    6 \\\n"
    "    6 \\\n"
    "    6\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Say more' \\\n"
    "    'Say more' \\\n"
    "    'Say less' \\\n"
    "    'Pretend' \\\n"
    "    'Two of each' \\\n"
    "    'Two of each' \\\n"
    "    'Two of each' \\\n"
    "    'Two of each' \\\n"
    "    'Long before short' \\\n"
    "    'Long before short' \\\n"
    "    '' \\\n"
    "    '' \\\n"
    "    '' \\\n"
    "    ''\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '1 1 f 1 0' \\\n"
    "    '1 1 f 1 0' \\\n"
    "    '0 0 - 1 0'\n"
    "set -g -- __djarg_prog_pk\n"
    "set -g -- __djarg_prog_pv\n"
    "set -g -- __djarg_prog_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== names powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 16'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-v'\n"
    "        '--verbose'\n"
    "        '-q'\n"
    "        '--dry-run'\n"
    "        '-a'\n"
    "        '-b'\n"
    "        '--alpha'\n"
    "        '--beta'\n"
    "        '-c'\n"
    "        '--beta2'\n"
    "        '-z'\n"
    "        '-y'\n"
    "        '--zed'\n"
    "        '--why'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        1\n"
    "        2\n"
    "        3\n"
    "        4\n"
    "        4\n"
    "        4\n"
    "        4\n"
    "        5\n"
    "        5\n"
    "        6\n"
    "        6\n"
    "        6\n"
    "        6\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Say more'\n"
    "        'Say more'\n"
    "        'Say less'\n"
    "        'Pretend'\n"
    "        'Two of each'\n"
    "        'Two of each'\n"
    "        'Two of each'\n"
    "        'Two of each'\n"
    "        'Long before short'\n"
    "        'Long before short'\n"
    "        ''\n"
    "        ''\n"
    "        ''\n"
    "        ''\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '1 1 f 0 0'\n"
    "        '1 1 f 0 0'\n"
    "        '0 0 - 0 0'\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n"
    "== counts bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:-n') r=1 ;;\n"
    "        '0:--count') r=1 ;;\n"
    "        '0:--pair') r=2 ;;\n"
    "        '0:--range') r=3 ;;\n"
    "        '0:--reversed') r=4 ;;\n"
    "        '0:--many') r=5 ;;\n"
    "        '0:--two-plus') r=6 ;;\n"
    "        '0:--unlimited') r=7 ;;\n"
    "        '0:--extra') r=8 ;;\n"
    "        '0:--zero') r=9 ;;\n"
    "        '0:--triple') r=10 ;;\n"
    "        '0:--sized') r=11 ;;\n"
    "        '0:--sized-twice') r=12 ;;\n"
    "        '0:--any-size') r=13 ;;\n"
    "        '0:--any-items') r=14 ;;\n"
    "        '0:--flag') r=15 ;;\n"
    "        '0:--big') r=16 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=f ;;\n"
    "        2) n=1 x=2 k=f ;;\n"
    "        3) n=1 x=3 k=f ;;\n"
    "        4) n=1 x=3 k=f ;;\n"
    "        5) n=1 x=-1 k=f ;;\n"
    "        6) n=1 x=-1 k=f ;;\n"
    "        7) n=1 x=-1 k=f ;;\n"
    "        8) n=1 x=-1 k=f ;;\n"
    "        9) n=0 x=0 k=- ;;\n"
    "        10) n=3 x=3 k=f ;;\n";

// TREE_OPTIONS_3
//   constant: the transcript of d_tests_arg_completion_tree_options, continued.
static const char TREE_OPTIONS_3[] =
    "        11) n=1 x=2 k=f ;;\n"
    "        12) n=1 x=4 k=f ;;\n"
    "        13) n=1 x=1 k=f ;;\n"
    "        14) n=1 x=-1 k=f ;;\n"
    "        15) n=0 x=0 k=- ;;\n"
    "        16) n=1 x=1 k=f ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '-n' '--count' '--pair' '--range' "
    "'--reversed' '--many' '--two-plus' '--unlimited' '--extra' '--zero' "
    "'--triple' '--sized' '--sized-twice' '--any-size' '--any-items' '--flag' "
    "'--big' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== counts zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:-n') r=1 ;;\n"
    "        '0:--count') r=1 ;;\n"
    "        '0:--pair') r=2 ;;\n"
    "        '0:--range') r=3 ;;\n"
    "        '0:--reversed') r=4 ;;\n"
    "        '0:--many') r=5 ;;\n"
    "        '0:--two-plus') r=6 ;;\n"
    "        '0:--unlimited') r=7 ;;\n"
    "        '0:--extra') r=8 ;;\n"
    "        '0:--zero') r=9 ;;\n"
    "        '0:--triple') r=10 ;;\n"
    "        '0:--sized') r=11 ;;\n"
    "        '0:--sized-twice') r=12 ;;\n"
    "        '0:--any-size') r=13 ;;\n"
    "        '0:--any-items') r=14 ;;\n"
    "        '0:--flag') r=15 ;;\n"
    "        '0:--big') r=16 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=f ;;\n"
    "        2) n=1 x=2 k=f ;;\n"
    "        3) n=1 x=3 k=f ;;\n"
    "        4) n=1 x=3 k=f ;;\n"
    "        5) n=1 x=-1 k=f ;;\n"
    "        6) n=1 x=-1 k=f ;;\n"
    "        7) n=1 x=-1 k=f ;;\n"
    "        8) n=1 x=-1 k=f ;;\n"
    "        9) n=0 x=0 k=- ;;\n"
    "        10) n=3 x=3 k=f ;;\n"
    "        11) n=1 x=2 k=f ;;\n"
    "        12) n=1 x=4 k=f ;;\n"
    "        13) n=1 x=1 k=f ;;\n"
    "        14) n=1 x=-1 k=f ;;\n"
    "        15) n=0 x=0 k=- ;;\n"
    "        16) n=1 x=1 k=f ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '-n' '--count' '--pair' '--range' "
    "'--reversed' '--many' '--two-plus' '--unlimited' '--extra' '--zero' "
    "'--triple' '--sized' '--sized-twice' '--any-size' '--any-items' '--flag' "
    "'--big' ) d=( 'Print this help message and exit' 'Print this help message "
    "and exit' 'One value' 'One value' 'Two values' 'One to three' 'Three to "
    "one' 'At least one' 'At least two' 'Any number' 'Extra arguments' 'Takes "
    "none' 'Three to an item' 'One or two to an item' 'Two items of one or "
    "two' 'Items of any size' 'Items of any length' 'A flag' 'A count past "
    "reason' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== counts fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 0 1 19'\n"
    "set -g -- __djarg_prog_sk\n"
    "set -g -- __djarg_prog_sn\n"
    "set -g -- __djarg_prog_st\n"
    "set -g -- __djarg_prog_sd\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '0:-n' \\\n"
    "    '0:--count' \\\n"
    "    '0:--pair' \\\n"
    "    '0:--range' \\\n"
    "    '0:--reversed' \\\n"
    "    '0:--many' \\\n"
    "    '0:--two-plus' \\\n"
    "    '0:--unlimited' \\\n"
    "    '0:--extra' \\\n"
    "    '0:--zero' \\\n"
    "    '0:--triple' \\\n"
    "    '0:--sized' \\\n"
    "    '0:--sized-twice' \\\n"
    "    '0:--any-size' \\\n"
    "    '0:--any-items' \\\n";

// TREE_OPTIONS_4
//   constant: the transcript of d_tests_arg_completion_tree_options, continued.
static const char TREE_OPTIONS_4[] =
    "    '0:--flag' \\\n"
    "    '0:--big'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-n' \\\n"
    "    '--count' \\\n"
    "    '--pair' \\\n"
    "    '--range' \\\n"
    "    '--reversed' \\\n"
    "    '--many' \\\n"
    "    '--two-plus' \\\n"
    "    '--unlimited' \\\n"
    "    '--extra' \\\n"
    "    '--zero' \\\n"
    "    '--triple' \\\n"
    "    '--sized' \\\n"
    "    '--sized-twice' \\\n"
    "    '--any-size' \\\n"
    "    '--any-items' \\\n"
    "    '--flag' \\\n"
    "    '--big'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    1 \\\n"
    "    1 \\\n"
    "    2 \\\n"
    "    3 \\\n"
    "    4 \\\n"
    "    5 \\\n"
    "    6 \\\n"
    "    7 \\\n"
    "    8 \\\n"
    "    9 \\\n"
    "    10 \\\n"
    "    11 \\\n"
    "    12 \\\n"
    "    13 \\\n"
    "    14 \\\n"
    "    15 \\\n"
    "    16\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'One value' \\\n"
    "    'One value' \\\n"
    "    'Two values' \\\n"
    "    'One to three' \\\n"
    "    'Three to one' \\\n"
    "    'At least one' \\\n"
    "    'At least two' \\\n"
    "    'Any number' \\\n"
    "    'Extra arguments' \\\n"
    "    'Takes none' \\\n"
    "    'Three to an item' \\\n"
    "    'One or two to an item' \\\n"
    "    'Two items of one or two' \\\n"
    "    'Items of any size' \\\n"
    "    'Items of any length' \\\n"
    "    'A flag' \\\n"
    "    'A count past reason'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '1 1 f 1 0' \\\n"
    "    '1 2 f 1 0' \\\n"
    "    '1 3 f 1 0' \\\n"
    "    '1 3 f 1 0' \\\n"
    "    '1 -1 f 1 0' \\\n"
    "    '1 -1 f 1 0' \\\n"
    "    '1 -1 f 1 0' \\\n"
    "    '1 -1 f 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '3 3 f 1 0' \\\n"
    "    '1 2 f 1 0' \\\n"
    "    '1 4 f 1 0' \\\n"
    "    '1 1 f 1 0' \\\n"
    "    '1 -1 f 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '1 1 f 1 0'\n"
    "set -g -- __djarg_prog_pk\n"
    "set -g -- __djarg_prog_pv\n"
    "set -g -- __djarg_prog_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== counts powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 19'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-n'\n"
    "        '--count'\n"
    "        '--pair'\n"
    "        '--range'\n"
    "        '--reversed'\n"
    "        '--many'\n"
    "        '--two-plus'\n"
    "        '--unlimited'\n"
    "        '--extra'\n"
    "        '--zero'\n"
    "        '--triple'\n"
    "        '--sized'\n"
    "        '--sized-twice'\n"
    "        '--any-size'\n"
    "        '--any-items'\n"
    "        '--flag'\n"
    "        '--big'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        1\n"
    "        2\n"
    "        3\n"
    "        4\n"
    "        5\n"
    "        6\n"
    "        7\n"
    "        8\n"
    "        9\n"
    "        10\n"
    "        11\n"
    "        12\n"
    "        13\n"
    "        14\n"
    "        15\n"
    "        16\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'One value'\n"
    "        'One value'\n"
    "        'Two values'\n"
    "        'One to three'\n"
    "        'Three to one'\n"
    "        'At least one'\n"
    "        'At least two'\n"
    "        'Any number'\n"
    "        'Extra arguments'\n"
    "        'Takes none'\n"
    "        'Three to an item'\n"
    "        'One or two to an item'\n"
    "        'Two items of one or two'\n"
    "        'Items of any size'\n"
    "        'Items of any length'\n"
    "        'A flag'\n"
    "        'A count past reason'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '1 1 f 0 0'\n"
    "        '1 2 f 0 0'\n"
    "        '1 3 f 0 0'\n"
    "        '1 3 f 0 0'\n"
    "        '1 -1 f 0 0'\n"
    "        '1 -1 f 0 0'\n"
    "        '1 -1 f 0 0'\n"
    "        '1 -1 f 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '3 3 f 0 0'\n"
    "        '1 2 f 0 0'\n"
    "        '1 4 f 0 0'\n"
    "        '1 1 f 0 0'\n"
    "        '1 -1 f 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '1 1 f 0 0'\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n"
    "== odd names bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n";

// TREE_OPTIONS_5
//   constant: the transcript of d_tests_arg_completion_tree_options, continued.
static const char TREE_OPTIONS_5[] =
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--it'\\''s') r=1 ;;\n"
    "        '0:--back\\slash') r=2 ;;\n"
    "        '0:--dollar$') r=3 ;;\n"
    "        '0:--tick`') r=4 ;;\n"
    "        '0:--quote\"d') r=5 ;;\n"
    "        '0:--caf\xc3\xa9') r=6 ;;\n"
    "        '0:--a\x09" "b') r=7 ;;\n"
    "        '0:--del\x7f') r=8 ;;\n"
    "        '0:--\xf0\x9f\x98\x80') r=9 ;;\n"
    "        '0:--semi;colon') r=10 ;;\n"
    "        '0:--star*') r=11 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=0 x=0 k=- ;;\n"
    "        2) n=0 x=0 k=- ;;\n"
    "        3) n=0 x=0 k=- ;;\n"
    "        4) n=0 x=0 k=- ;;\n"
    "        5) n=0 x=0 k=- ;;\n"
    "        6) n=0 x=0 k=- ;;\n"
    "        7) n=0 x=0 k=- ;;\n"
    "        8) n=0 x=0 k=- ;;\n"
    "        9) n=0 x=0 k=- ;;\n"
    "        10) n=0 x=0 k=- ;;\n"
    "        11) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--it'\\''s' '--back\\slash' '--dollar$' "
    "'--tick`' '--quote\"d' '--caf\xc3\xa9' '--a\x09" "b' '--del\x7f' "
    "'--\xf0\x9f\x98\x80' '--semi;colon' '--star*' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== odd names zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--it'\\''s') r=1 ;;\n"
    "        '0:--back\\slash') r=2 ;;\n"
    "        '0:--dollar$') r=3 ;;\n"
    "        '0:--tick`') r=4 ;;\n"
    "        '0:--quote\"d') r=5 ;;\n"
    "        '0:--caf\xc3\xa9') r=6 ;;\n"
    "        '0:--a\x09" "b') r=7 ;;\n"
    "        '0:--del\x7f') r=8 ;;\n"
    "        '0:--\xf0\x9f\x98\x80') r=9 ;;\n"
    "        '0:--semi;colon') r=10 ;;\n"
    "        '0:--star*') r=11 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=0 x=0 k=- ;;\n"
    "        2) n=0 x=0 k=- ;;\n"
    "        3) n=0 x=0 k=- ;;\n"
    "        4) n=0 x=0 k=- ;;\n"
    "        5) n=0 x=0 k=- ;;\n"
    "        6) n=0 x=0 k=- ;;\n"
    "        7) n=0 x=0 k=- ;;\n"
    "        8) n=0 x=0 k=- ;;\n"
    "        9) n=0 x=0 k=- ;;\n"
    "        10) n=0 x=0 k=- ;;\n"
    "        11) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--it'\\''s' '--back\\slash' '--dollar$' "
    "'--tick`' '--quote\"d' '--caf\xc3\xa9' '--a\x09" "b' '--del\x7f' "
    "'--\xf0\x9f\x98\x80' '--semi;colon' '--star*' ) d=( 'Print this help "
    "message and exit' 'Print this help message and exit' 'Quote'\\''s here' "
    "'A backslash' 'A dollar' 'A back quote' 'A double quote' 'Caf\xc3\xa9' 'A "
    "tab' 'A delete' 'Astral' 'A semicolon' 'A star' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== odd names fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 0 1 13'\n"
    "set -g -- __djarg_prog_sk\n"
    "set -g -- __djarg_prog_sn\n"
    "set -g -- __djarg_prog_st\n"
    "set -g -- __djarg_prog_sd\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '0:--it\\'s' \\\n"
    "    '0:--back\\\\slash' \\\n"
    "    '0:--dollar$' \\\n"
    "    '0:--tick`' \\\n"
    "    '0:--quote\"d' \\\n"
    "    '0:--caf\xc3\xa9' \\\n"
    "    '0:--a\x09" "b' \\\n"
    "    '0:--del\x7f' \\\n"
    "    '0:--\xf0\x9f\x98\x80' \\\n"
    "    '0:--semi;colon' \\\n"
    "    '0:--star*'\n"
    "set -g -- __djarg_prog_on \\\n";

// TREE_OPTIONS_6
//   constant: the transcript of d_tests_arg_completion_tree_options, continued.
static const char TREE_OPTIONS_6[] =
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '--it\\'s' \\\n"
    "    '--back\\\\slash' \\\n"
    "    '--dollar$' \\\n"
    "    '--tick`' \\\n"
    "    '--quote\"d' \\\n"
    "    '--caf\xc3\xa9' \\\n"
    "    '--a\x09" "b' \\\n"
    "    '--del\x7f' \\\n"
    "    '--\xf0\x9f\x98\x80' \\\n"
    "    '--semi;colon' \\\n"
    "    '--star*'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    1 \\\n"
    "    2 \\\n"
    "    3 \\\n"
    "    4 \\\n"
    "    5 \\\n"
    "    6 \\\n"
    "    7 \\\n"
    "    8 \\\n"
    "    9 \\\n"
    "    10 \\\n"
    "    11\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Quote\\'s here' \\\n"
    "    'A backslash' \\\n"
    "    'A dollar' \\\n"
    "    'A back quote' \\\n"
    "    'A double quote' \\\n"
    "    'Caf\xc3\xa9' \\\n"
    "    'A tab' \\\n"
    "    'A delete' \\\n"
    "    'Astral' \\\n"
    "    'A semicolon' \\\n"
    "    'A star'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0'\n"
    "set -g -- __djarg_prog_pk\n"
    "set -g -- __djarg_prog_pv\n"
    "set -g -- __djarg_prog_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== odd names powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 13'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '--it''s'\n"
    "        '--back\\slash'\n"
    "        '--dollar$'\n"
    "        '--tick`'\n"
    "        '--quote\"d'\n"
    "        ('' + '--caf' + [char]0x00E9)\n"
    "        ('' + '--a' + [char]0x0009 + 'b')\n"
    "        ('' + '--del' + [char]0x007F)\n"
    "        ('' + '--' + [char]0xD83D + [char]0xDE00)\n"
    "        '--semi;colon'\n"
    "        '--star*'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        2\n"
    "        3\n"
    "        4\n"
    "        5\n"
    "        6\n"
    "        7\n"
    "        8\n"
    "        9\n"
    "        10\n"
    "        11\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Quote''s here'\n"
    "        'A backslash'\n"
    "        'A dollar'\n"
    "        'A back quote'\n"
    "        'A double quote'\n"
    "        ('' + 'Caf' + [char]0x00E9)\n"
    "        'A tab'\n"
    "        'A delete'\n"
    "        'Astral'\n"
    "        'A semicolon'\n"
    "        'A star'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n";

// TREE_VALUES
//   constant: what d_tests_arg_completion_tree_values writes, as
// completion_ref.py works it out from the rules arg_completion.h states, in
// parts.
static const char TREE_VALUES[] =
    "== choices bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--color') r=1 ;;\n"
    "        '0:--shade') r=2 ;;\n"
    "        '0:--mixed') r=3 ;;\n"
    "        '0:--off') r=4 ;;\n"
    "        '0:--later') r=5 ;;\n"
    "        '0:--first') r=6 ;;\n"
    "        '0:--empty') r=7 ;;\n"
    "        '0:--same') r=8 ;;\n"
    "        '0:--shared') r=9 ;;\n"
    "        '0:--flag') r=10 ;;\n"
    "        '0:--case') r=11 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=c ;;\n"
    "        2) n=1 x=1 k=c ;;\n"
    "        3) n=1 x=1 k=c ;;\n"
    "        4) n=1 x=1 k=f ;;\n"
    "        5) n=1 x=1 k=f ;;\n"
    "        6) n=1 x=1 k=c ;;\n"
    "        7) n=1 x=1 k=f ;;\n"
    "        8) n=1 x=1 k=c ;;\n"
    "        9) n=1 x=1 k=c ;;\n"
    "        10) n=0 x=0 k=- ;;\n"
    "        11) n=1 x=1 k=c ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--color' '--shade' '--mixed' '--off' "
    "'--later' '--first' '--empty' '--same' '--shared' '--flag' '--case' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        1) c=( 'red' 'green' 'blue' ) ;;\n"
    "        2) c=( 'dark' 'light' 'pale' ) ;;\n"
    "        3) c=( 'b' 'a' ) ;;\n"
    "        6) c=( 'z' ) ;;\n"
    "        8) c=( 'one' 'two' ) ;;\n"
    "        9) c=( 'red' ) ;;\n"
    "        11) c=( 'A' 'a' '' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== choices zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--color') r=1 ;;\n"
    "        '0:--shade') r=2 ;;\n"
    "        '0:--mixed') r=3 ;;\n"
    "        '0:--off') r=4 ;;\n"
    "        '0:--later') r=5 ;;\n"
    "        '0:--first') r=6 ;;\n"
    "        '0:--empty') r=7 ;;\n"
    "        '0:--same') r=8 ;;\n"
    "        '0:--shared') r=9 ;;\n"
    "        '0:--flag') r=10 ;;\n"
    "        '0:--case') r=11 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=c ;;\n"
    "        2) n=1 x=1 k=c ;;\n"
    "        3) n=1 x=1 k=c ;;\n"
    "        4) n=1 x=1 k=f ;;\n"
    "        5) n=1 x=1 k=f ;;\n"
    "        6) n=1 x=1 k=c ;;\n"
    "        7) n=1 x=1 k=f ;;\n"
    "        8) n=1 x=1 k=c ;;\n"
    "        9) n=1 x=1 k=c ;;\n"
    "        10) n=0 x=0 k=- ;;\n"
    "        11) n=1 x=1 k=c ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--color' '--shade' '--mixed' '--off' "
    "'--later' '--first' '--empty' '--same' '--shared' '--flag' '--case' ) d=( "
    "'Print this help message and exit' 'Print this help message and exit' "
    "'Pick one' 'Two lists' 'Transformed first' 'An inactive list' 'A list for "
    "the second value' 'A list for the first value' 'An empty list' 'The same "
    "twice' 'A value another lists' 'A flag with a list' 'Values told apart by "
    "case' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        1) c=( 'red' 'green' 'blue' ) ;;\n"
    "        2) c=( 'dark' 'light' 'pale' ) ;;\n"
    "        3) c=( 'b' 'a' ) ;;\n"
    "        6) c=( 'z' ) ;;\n"
    "        8) c=( 'one' 'two' ) ;;\n"
    "        9) c=( 'red' ) ;;\n"
    "        11) c=( 'A' 'a' '' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n";

// TREE_VALUES_2
//   constant: the transcript of d_tests_arg_completion_tree_values, continued.
static const char TREE_VALUES_2[] =
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== choices fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 0 1 13'\n"
    "set -g -- __djarg_prog_sk\n"
    "set -g -- __djarg_prog_sn\n"
    "set -g -- __djarg_prog_st\n"
    "set -g -- __djarg_prog_sd\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '0:--color' \\\n"
    "    '0:--shade' \\\n"
    "    '0:--mixed' \\\n"
    "    '0:--off' \\\n"
    "    '0:--later' \\\n"
    "    '0:--first' \\\n"
    "    '0:--empty' \\\n"
    "    '0:--same' \\\n"
    "    '0:--shared' \\\n"
    "    '0:--flag' \\\n"
    "    '0:--case'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '--color' \\\n"
    "    '--shade' \\\n"
    "    '--mixed' \\\n"
    "    '--off' \\\n"
    "    '--later' \\\n"
    "    '--first' \\\n"
    "    '--empty' \\\n"
    "    '--same' \\\n"
    "    '--shared' \\\n"
    "    '--flag' \\\n"
    "    '--case'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    1 \\\n"
    "    2 \\\n"
    "    3 \\\n"
    "    4 \\\n"
    "    5 \\\n"
    "    6 \\\n"
    "    7 \\\n"
    "    8 \\\n"
    "    9 \\\n"
    "    10 \\\n"
    "    11\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Pick one' \\\n"
    "    'Two lists' \\\n"
    "    'Transformed first' \\\n"
    "    'An inactive list' \\\n"
    "    'A list for the second value' \\\n"
    "    'A list for the first value' \\\n"
    "    'An empty list' \\\n"
    "    'The same twice' \\\n"
    "    'A value another lists' \\\n"
    "    'A flag with a list' \\\n"
    "    'Values told apart by case'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '1 1 c 1 3' \\\n"
    "    '1 1 c 4 3' \\\n"
    "    '1 1 c 7 2' \\\n"
    "    '1 1 f 9 0' \\\n"
    "    '1 1 f 9 0' \\\n"
    "    '1 1 c 9 1' \\\n"
    "    '1 1 f 10 0' \\\n"
    "    '1 1 c 10 2' \\\n"
    "    '1 1 c 12 1' \\\n"
    "    '0 0 - 13 0' \\\n"
    "    '1 1 c 13 3'\n"
    "set -g -- __djarg_prog_pk\n"
    "set -g -- __djarg_prog_pv\n"
    "set -g -- __djarg_prog_vv \\\n"
    "    'red' \\\n"
    "    'green' \\\n"
    "    'blue' \\\n"
    "    'dark' \\\n"
    "    'light' \\\n"
    "    'pale' \\\n"
    "    'b' \\\n"
    "    'a' \\\n"
    "    'z' \\\n"
    "    'one' \\\n"
    "    'two' \\\n"
    "    'red' \\\n"
    "    'A' \\\n"
    "    'a' \\\n"
    "    ''\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== choices powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 13'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '--color'\n"
    "        '--shade'\n"
    "        '--mixed'\n"
    "        '--off'\n"
    "        '--later'\n"
    "        '--first'\n"
    "        '--empty'\n"
    "        '--same'\n"
    "        '--shared'\n"
    "        '--flag'\n"
    "        '--case'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        2\n"
    "        3\n"
    "        4\n"
    "        5\n"
    "        6\n"
    "        7\n"
    "        8\n"
    "        9\n"
    "        10\n"
    "        11\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Pick one'\n"
    "        'Two lists'\n"
    "        'Transformed first'\n"
    "        'An inactive list'\n"
    "        'A list for the second value'\n"
    "        'A list for the first value'\n"
    "        'An empty list'\n"
    "        'The same twice'\n"
    "        'A value another lists'\n"
    "        'A flag with a list'\n"
    "        'Values told apart by case'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '1 1 c 0 3'\n"
    "        '1 1 c 3 3'\n"
    "        '1 1 c 6 2'\n"
    "        '1 1 f 8 0'\n"
    "        '1 1 f 8 0'\n"
    "        '1 1 c 8 1'\n"
    "        '1 1 f 9 0'\n"
    "        '1 1 c 9 2'\n"
    "        '1 1 c 11 1'\n"
    "        '0 0 - 12 0'\n"
    "        '1 1 c 12 3'\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "        'red'\n"
    "        'green'\n"
    "        'blue'\n"
    "        'dark'\n"
    "        'light'\n"
    "        'pale'\n"
    "        'b'\n"
    "        'a'\n"
    "        'z'\n"
    "        'one'\n"
    "        'two'\n"
    "        'red'\n"
    "        'A'\n"
    "        'a'\n"
    "        ''\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n";

// TREE_VALUES_3
//   constant: the transcript of d_tests_arg_completion_tree_values, continued.
static const char TREE_VALUES_3[] =
    "== kinds bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--plain') r=1 ;;\n"
    "        '0:--file') r=2 ;;\n"
    "        '0:--dir') r=3 ;;\n"
    "        '0:--num') r=4 ;;\n"
    "        '0:--path-number') r=5 ;;\n"
    "        '0:--path-dir') r=6 ;;\n"
    "        '0:--range') r=7 ;;\n"
    "        '0:--listed-dir') r=8 ;;\n"
    "        '0:--inactive-number') r=9 ;;\n"
    "        '0:--second-dir') r=10 ;;\n"
    "        '0:--dir-first') r=11 ;;\n"
    "        '0:--every') r=12 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=f ;;\n"
    "        2) n=1 x=1 k=f ;;\n"
    "        3) n=1 x=1 k=d ;;\n"
    "        4) n=1 x=1 k=n ;;\n"
    "        5) n=1 x=1 k=n ;;\n"
    "        6) n=1 x=1 k=d ;;\n"
    "        7) n=1 x=1 k=n ;;\n"
    "        8) n=1 x=1 k=c ;;\n"
    "        9) n=1 x=1 k=f ;;\n"
    "        10) n=1 x=1 k=f ;;\n"
    "        11) n=1 x=1 k=d ;;\n"
    "        12) n=1 x=1 k=d ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--plain' '--file' '--dir' '--num' "
    "'--path-number' '--path-dir' '--range' '--listed-dir' '--inactive-number' "
    "'--second-dir' '--dir-first' '--every' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        8) c=( 'here' 'there' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== kinds zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--plain') r=1 ;;\n"
    "        '0:--file') r=2 ;;\n"
    "        '0:--dir') r=3 ;;\n"
    "        '0:--num') r=4 ;;\n"
    "        '0:--path-number') r=5 ;;\n"
    "        '0:--path-dir') r=6 ;;\n"
    "        '0:--range') r=7 ;;\n"
    "        '0:--listed-dir') r=8 ;;\n"
    "        '0:--inactive-number') r=9 ;;\n"
    "        '0:--second-dir') r=10 ;;\n"
    "        '0:--dir-first') r=11 ;;\n"
    "        '0:--every') r=12 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=f ;;\n"
    "        2) n=1 x=1 k=f ;;\n"
    "        3) n=1 x=1 k=d ;;\n"
    "        4) n=1 x=1 k=n ;;\n"
    "        5) n=1 x=1 k=n ;;\n"
    "        6) n=1 x=1 k=d ;;\n"
    "        7) n=1 x=1 k=n ;;\n"
    "        8) n=1 x=1 k=c ;;\n"
    "        9) n=1 x=1 k=f ;;\n"
    "        10) n=1 x=1 k=f ;;\n"
    "        11) n=1 x=1 k=d ;;\n"
    "        12) n=1 x=1 k=d ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--plain' '--file' '--dir' '--num' "
    "'--path-number' '--path-dir' '--range' '--listed-dir' '--inactive-number' "
    "'--second-dir' '--dir-first' '--every' ) d=( 'Print this help message and "
    "exit' 'Print this help message and exit' 'No validator' 'A path' 'A "
    "directory' 'A number' 'A path, then a number' 'A path, then a directory' "
    "'A range' 'A list and a directory' 'An inactive number' 'A directory for "
    "the second' 'A directory for the first' 'Every value a directory' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        8) c=( 'here' 'there' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n";

// TREE_VALUES_4
//   constant: the transcript of d_tests_arg_completion_tree_values, continued.
static const char TREE_VALUES_4[] =
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== kinds fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 0 1 14'\n"
    "set -g -- __djarg_prog_sk\n"
    "set -g -- __djarg_prog_sn\n"
    "set -g -- __djarg_prog_st\n"
    "set -g -- __djarg_prog_sd\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '0:--plain' \\\n"
    "    '0:--file' \\\n"
    "    '0:--dir' \\\n"
    "    '0:--num' \\\n"
    "    '0:--path-number' \\\n"
    "    '0:--path-dir' \\\n"
    "    '0:--range' \\\n"
    "    '0:--listed-dir' \\\n"
    "    '0:--inactive-number' \\\n"
    "    '0:--second-dir' \\\n"
    "    '0:--dir-first' \\\n"
    "    '0:--every'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '--plain' \\\n"
    "    '--file' \\\n"
    "    '--dir' \\\n"
    "    '--num' \\\n"
    "    '--path-number' \\\n"
    "    '--path-dir' \\\n"
    "    '--range' \\\n"
    "    '--listed-dir' \\\n"
    "    '--inactive-number' \\\n"
    "    '--second-dir' \\\n"
    "    '--dir-first' \\\n"
    "    '--every'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    1 \\\n"
    "    2 \\\n"
    "    3 \\\n"
    "    4 \\\n"
    "    5 \\\n"
    "    6 \\\n"
    "    7 \\\n"
    "    8 \\\n"
    "    9 \\\n"
    "    10 \\\n"
    "    11 \\\n"
    "    12\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'No validator' \\\n"
    "    'A path' \\\n"
    "    'A directory' \\\n"
    "    'A number' \\\n"
    "    'A path, then a number' \\\n"
    "    'A path, then a directory' \\\n"
    "    'A range' \\\n"
    "    'A list and a directory' \\\n"
    "    'An inactive number' \\\n"
    "    'A directory for the second' \\\n"
    "    'A directory for the first' \\\n"
    "    'Every value a directory'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '1 1 f 1 0' \\\n"
    "    '1 1 f 1 0' \\\n"
    "    '1 1 d 1 0' \\\n"
    "    '1 1 n 1 0' \\\n"
    "    '1 1 n 1 0' \\\n"
    "    '1 1 d 1 0' \\\n"
    "    '1 1 n 1 0' \\\n"
    "    '1 1 c 1 2' \\\n"
    "    '1 1 f 3 0' \\\n"
    "    '1 1 f 3 0' \\\n"
    "    '1 1 d 3 0' \\\n"
    "    '1 1 d 3 0'\n"
    "set -g -- __djarg_prog_pk\n"
    "set -g -- __djarg_prog_pv\n"
    "set -g -- __djarg_prog_vv \\\n"
    "    'here' \\\n"
    "    'there'\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== kinds powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 14'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '--plain'\n"
    "        '--file'\n"
    "        '--dir'\n"
    "        '--num'\n"
    "        '--path-number'\n"
    "        '--path-dir'\n"
    "        '--range'\n"
    "        '--listed-dir'\n"
    "        '--inactive-number'\n"
    "        '--second-dir'\n"
    "        '--dir-first'\n"
    "        '--every'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        2\n"
    "        3\n"
    "        4\n"
    "        5\n"
    "        6\n"
    "        7\n"
    "        8\n"
    "        9\n"
    "        10\n"
    "        11\n"
    "        12\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'No validator'\n"
    "        'A path'\n"
    "        'A directory'\n"
    "        'A number'\n"
    "        'A path, then a number'\n"
    "        'A path, then a directory'\n"
    "        'A range'\n"
    "        'A list and a directory'\n"
    "        'An inactive number'\n"
    "        'A directory for the second'\n"
    "        'A directory for the first'\n"
    "        'Every value a directory'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '1 1 f 0 0'\n"
    "        '1 1 f 0 0'\n"
    "        '1 1 d 0 0'\n"
    "        '1 1 n 0 0'\n"
    "        '1 1 n 0 0'\n"
    "        '1 1 d 0 0'\n"
    "        '1 1 n 0 0'\n"
    "        '1 1 c 0 2'\n"
    "        '1 1 f 2 0'\n"
    "        '1 1 f 2 0'\n"
    "        '1 1 d 2 0'\n"
    "        '1 1 d 2 0'\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "        'here'\n"
    "        'there'\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n"
    "== composites bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n";

// TREE_VALUES_5
//   constant: the transcript of d_tests_arg_completion_tree_values, continued.
static const char TREE_VALUES_5[] =
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--and') r=1 ;;\n"
    "        '0:--and-second') r=2 ;;\n"
    "        '0:--and-empty') r=3 ;;\n"
    "        '0:--and-kinds') r=4 ;;\n"
    "        '0:--and-number') r=5 ;;\n"
    "        '0:--or') r=6 ;;\n"
    "        '0:--or-dir') r=7 ;;\n"
    "        '0:--or-numbers') r=8 ;;\n"
    "        '0:--or-paths') r=9 ;;\n"
    "        '0:--or-dirs') r=10 ;;\n"
    "        '0:--or-second') r=11 ;;\n"
    "        '0:--not') r=12 ;;\n"
    "        '0:--not-number') r=13 ;;\n"
    "        '0:--and-inactive') r=14 ;;\n"
    "        '0:--or-inactive') r=15 ;;\n"
    "        '0:--and-later') r=16 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=c ;;\n"
    "        2) n=1 x=1 k=c ;;\n"
    "        3) n=1 x=1 k=f ;;\n"
    "        4) n=1 x=1 k=d ;;\n"
    "        5) n=1 x=1 k=n ;;\n"
    "        6) n=1 x=1 k=c ;;\n"
    "        7) n=1 x=1 k=d ;;\n"
    "        8) n=1 x=1 k=n ;;\n"
    "        9) n=1 x=1 k=f ;;\n"
    "        10) n=1 x=1 k=d ;;\n"
    "        11) n=1 x=1 k=c ;;\n"
    "        12) n=1 x=1 k=f ;;\n"
    "        13) n=1 x=1 k=f ;;\n"
    "        14) n=1 x=1 k=f ;;\n"
    "        15) n=1 x=1 k=f ;;\n"
    "        16) n=1 x=1 k=f ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--and' '--and-second' '--and-empty' "
    "'--and-kinds' '--and-number' '--or' '--or-dir' '--or-numbers' "
    "'--or-paths' '--or-dirs' '--or-second' '--not' '--not-number' "
    "'--and-inactive' '--or-inactive' '--and-later' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        1) c=( 'one' 'two' ) ;;\n"
    "        2) c=( 'x' ) ;;\n"
    "        6) c=( 'p' 'q' ) ;;\n"
    "        11) c=( 'n' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== composites zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--and') r=1 ;;\n"
    "        '0:--and-second') r=2 ;;\n"
    "        '0:--and-empty') r=3 ;;\n"
    "        '0:--and-kinds') r=4 ;;\n"
    "        '0:--and-number') r=5 ;;\n"
    "        '0:--or') r=6 ;;\n"
    "        '0:--or-dir') r=7 ;;\n"
    "        '0:--or-numbers') r=8 ;;\n"
    "        '0:--or-paths') r=9 ;;\n"
    "        '0:--or-dirs') r=10 ;;\n"
    "        '0:--or-second') r=11 ;;\n"
    "        '0:--not') r=12 ;;\n"
    "        '0:--not-number') r=13 ;;\n"
    "        '0:--and-inactive') r=14 ;;\n"
    "        '0:--or-inactive') r=15 ;;\n"
    "        '0:--and-later') r=16 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=c ;;\n"
    "        2) n=1 x=1 k=c ;;\n"
    "        3) n=1 x=1 k=f ;;\n"
    "        4) n=1 x=1 k=d ;;\n"
    "        5) n=1 x=1 k=n ;;\n"
    "        6) n=1 x=1 k=c ;;\n"
    "        7) n=1 x=1 k=d ;;\n"
    "        8) n=1 x=1 k=n ;;\n"
    "        9) n=1 x=1 k=f ;;\n"
    "        10) n=1 x=1 k=d ;;\n"
    "        11) n=1 x=1 k=c ;;\n"
    "        12) n=1 x=1 k=f ;;\n"
    "        13) n=1 x=1 k=f ;;\n"
    "        14) n=1 x=1 k=f ;;\n"
    "        15) n=1 x=1 k=f ;;\n"
    "        16) n=1 x=1 k=f ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n";

// TREE_VALUES_6
//   constant: the transcript of d_tests_arg_completion_tree_values, continued.
static const char TREE_VALUES_6[] =
    "        0) c=( '-h' '--help' '--and' '--and-second' '--and-empty' "
    "'--and-kinds' '--and-number' '--or' '--or-dir' '--or-numbers' "
    "'--or-paths' '--or-dirs' '--or-second' '--not' '--not-number' "
    "'--and-inactive' '--or-inactive' '--and-later' ) d=( 'Print this help "
    "message and exit' 'Print this help message and exit' 'A list and a "
    "number' 'A directory and a list' 'An empty list first' 'A path and a "
    "directory' 'A path and a number' 'Two lists' 'A number or a directory' "
    "'Two numbers' 'A file or a directory' 'Two directories' 'A list second' "
    "'Not a list' 'Not a number' 'An inactive operand' 'An inactive operand' "
    "'A composite for the second' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        1) c=( 'one' 'two' ) ;;\n"
    "        2) c=( 'x' ) ;;\n"
    "        6) c=( 'p' 'q' ) ;;\n"
    "        11) c=( 'n' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== composites fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 0 1 18'\n"
    "set -g -- __djarg_prog_sk\n"
    "set -g -- __djarg_prog_sn\n"
    "set -g -- __djarg_prog_st\n"
    "set -g -- __djarg_prog_sd\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '0:--and' \\\n"
    "    '0:--and-second' \\\n"
    "    '0:--and-empty' \\\n"
    "    '0:--and-kinds' \\\n"
    "    '0:--and-number' \\\n"
    "    '0:--or' \\\n"
    "    '0:--or-dir' \\\n"
    "    '0:--or-numbers' \\\n"
    "    '0:--or-paths' \\\n"
    "    '0:--or-dirs' \\\n"
    "    '0:--or-second' \\\n"
    "    '0:--not' \\\n"
    "    '0:--not-number' \\\n"
    "    '0:--and-inactive' \\\n"
    "    '0:--or-inactive' \\\n"
    "    '0:--and-later'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '--and' \\\n"
    "    '--and-second' \\\n"
    "    '--and-empty' \\\n"
    "    '--and-kinds' \\\n"
    "    '--and-number' \\\n"
    "    '--or' \\\n"
    "    '--or-dir' \\\n"
    "    '--or-numbers' \\\n"
    "    '--or-paths' \\\n"
    "    '--or-dirs' \\\n"
    "    '--or-second' \\\n"
    "    '--not' \\\n"
    "    '--not-number' \\\n"
    "    '--and-inactive' \\\n"
    "    '--or-inactive' \\\n"
    "    '--and-later'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    1 \\\n"
    "    2 \\\n"
    "    3 \\\n"
    "    4 \\\n"
    "    5 \\\n"
    "    6 \\\n"
    "    7 \\\n"
    "    8 \\\n"
    "    9 \\\n"
    "    10 \\\n"
    "    11 \\\n"
    "    12 \\\n"
    "    13 \\\n"
    "    14 \\\n"
    "    15 \\\n"
    "    16\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'A list and a number' \\\n"
    "    'A directory and a list' \\\n"
    "    'An empty list first' \\\n"
    "    'A path and a directory' \\\n"
    "    'A path and a number' \\\n"
    "    'Two lists' \\\n"
    "    'A number or a directory' \\\n"
    "    'Two numbers' \\\n"
    "    'A file or a directory' \\\n"
    "    'Two directories' \\\n"
    "    'A list second' \\\n"
    "    'Not a list' \\\n"
    "    'Not a number' \\\n"
    "    'An inactive operand' \\\n"
    "    'An inactive operand' \\\n"
    "    'A composite for the second'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '1 1 c 1 2' \\\n"
    "    '1 1 c 3 1' \\\n"
    "    '1 1 f 4 0' \\\n"
    "    '1 1 d 4 0' \\\n"
    "    '1 1 n 4 0' \\\n"
    "    '1 1 c 4 2' \\\n"
    "    '1 1 d 6 0' \\\n"
    "    '1 1 n 6 0' \\\n"
    "    '1 1 f 6 0' \\\n"
    "    '1 1 d 6 0' \\\n"
    "    '1 1 c 6 1' \\\n"
    "    '1 1 f 7 0' \\\n"
    "    '1 1 f 7 0' \\\n"
    "    '1 1 f 7 0' \\\n"
    "    '1 1 f 7 0' \\\n"
    "    '1 1 f 7 0'\n"
    "set -g -- __djarg_prog_pk\n"
    "set -g -- __djarg_prog_pv\n"
    "set -g -- __djarg_prog_vv \\\n"
    "    'one' \\\n"
    "    'two' \\\n"
    "    'x' \\\n"
    "    'p' \\\n"
    "    'q' \\\n"
    "    'n'\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== composites powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 18'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '--and'\n"
    "        '--and-second'\n"
    "        '--and-empty'\n"
    "        '--and-kinds'\n"
    "        '--and-number'\n"
    "        '--or'\n"
    "        '--or-dir'\n"
    "        '--or-numbers'\n"
    "        '--or-paths'\n"
    "        '--or-dirs'\n";

// TREE_VALUES_7
//   constant: the transcript of d_tests_arg_completion_tree_values, continued.
static const char TREE_VALUES_7[] =
    "        '--or-second'\n"
    "        '--not'\n"
    "        '--not-number'\n"
    "        '--and-inactive'\n"
    "        '--or-inactive'\n"
    "        '--and-later'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        2\n"
    "        3\n"
    "        4\n"
    "        5\n"
    "        6\n"
    "        7\n"
    "        8\n"
    "        9\n"
    "        10\n"
    "        11\n"
    "        12\n"
    "        13\n"
    "        14\n"
    "        15\n"
    "        16\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'A list and a number'\n"
    "        'A directory and a list'\n"
    "        'An empty list first'\n"
    "        'A path and a directory'\n"
    "        'A path and a number'\n"
    "        'Two lists'\n"
    "        'A number or a directory'\n"
    "        'Two numbers'\n"
    "        'A file or a directory'\n"
    "        'Two directories'\n"
    "        'A list second'\n"
    "        'Not a list'\n"
    "        'Not a number'\n"
    "        'An inactive operand'\n"
    "        'An inactive operand'\n"
    "        'A composite for the second'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '1 1 c 0 2'\n"
    "        '1 1 c 2 1'\n"
    "        '1 1 f 3 0'\n"
    "        '1 1 d 3 0'\n"
    "        '1 1 n 3 0'\n"
    "        '1 1 c 3 2'\n"
    "        '1 1 d 5 0'\n"
    "        '1 1 n 5 0'\n"
    "        '1 1 f 5 0'\n"
    "        '1 1 d 5 0'\n"
    "        '1 1 c 5 1'\n"
    "        '1 1 f 6 0'\n"
    "        '1 1 f 6 0'\n"
    "        '1 1 f 6 0'\n"
    "        '1 1 f 6 0'\n"
    "        '1 1 f 6 0'\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "        'one'\n"
    "        'two'\n"
    "        'x'\n"
    "        'p'\n"
    "        'q'\n"
    "        'n'\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n"
    "== types bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--int') r=1 ;;\n"
    "        '0:--unsigned') r=2 ;;\n"
    "        '0:--double') r=3 ;;\n"
    "        '0:--bool') r=4 ;;\n"
    "        '0:--char') r=5 ;;\n"
    "        '0:--string') r=6 ;;\n"
    "        '0:--text') r=7 ;;\n"
    "        '0:--int-choice') r=8 ;;\n"
    "        '0:--int-path') r=9 ;;\n"
    "        '0:--string-number') r=10 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=n ;;\n"
    "        2) n=1 x=1 k=n ;;\n"
    "        3) n=1 x=1 k=n ;;\n"
    "        4) n=1 x=1 k=n ;;\n"
    "        5) n=1 x=1 k=n ;;\n"
    "        6) n=1 x=1 k=f ;;\n"
    "        7) n=1 x=1 k=f ;;\n"
    "        8) n=1 x=1 k=c ;;\n"
    "        9) n=1 x=1 k=f ;;\n"
    "        10) n=1 x=1 k=n ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--int' '--unsigned' '--double' '--bool' "
    "'--char' '--string' '--text' '--int-choice' '--int-path' "
    "'--string-number' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        8) c=( '1' '2' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== types zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--int') r=1 ;;\n"
    "        '0:--unsigned') r=2 ;;\n"
    "        '0:--double') r=3 ;;\n"
    "        '0:--bool') r=4 ;;\n"
    "        '0:--char') r=5 ;;\n"
    "        '0:--string') r=6 ;;\n"
    "        '0:--text') r=7 ;;\n"
    "        '0:--int-choice') r=8 ;;\n"
    "        '0:--int-path') r=9 ;;\n";

// TREE_VALUES_8
//   constant: the transcript of d_tests_arg_completion_tree_values, continued.
static const char TREE_VALUES_8[] =
    "        '0:--string-number') r=10 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=n ;;\n"
    "        2) n=1 x=1 k=n ;;\n"
    "        3) n=1 x=1 k=n ;;\n"
    "        4) n=1 x=1 k=n ;;\n"
    "        5) n=1 x=1 k=n ;;\n"
    "        6) n=1 x=1 k=f ;;\n"
    "        7) n=1 x=1 k=f ;;\n"
    "        8) n=1 x=1 k=c ;;\n"
    "        9) n=1 x=1 k=f ;;\n"
    "        10) n=1 x=1 k=n ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--int' '--unsigned' '--double' '--bool' "
    "'--char' '--string' '--text' '--int-choice' '--int-path' "
    "'--string-number' ) d=( 'Print this help message and exit' 'Print this "
    "help message and exit' 'An int' 'An unsigned' 'A double' 'A truth value' "
    "'A character' 'A string' 'A text' 'An int from a list' 'An int a path' 'A "
    "string a number' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        8) c=( '1' '2' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== types fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 0 1 12'\n"
    "set -g -- __djarg_prog_sk\n"
    "set -g -- __djarg_prog_sn\n"
    "set -g -- __djarg_prog_st\n"
    "set -g -- __djarg_prog_sd\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '0:--int' \\\n"
    "    '0:--unsigned' \\\n"
    "    '0:--double' \\\n"
    "    '0:--bool' \\\n"
    "    '0:--char' \\\n"
    "    '0:--string' \\\n"
    "    '0:--text' \\\n"
    "    '0:--int-choice' \\\n"
    "    '0:--int-path' \\\n"
    "    '0:--string-number'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '--int' \\\n"
    "    '--unsigned' \\\n"
    "    '--double' \\\n"
    "    '--bool' \\\n"
    "    '--char' \\\n"
    "    '--string' \\\n"
    "    '--text' \\\n"
    "    '--int-choice' \\\n"
    "    '--int-path' \\\n"
    "    '--string-number'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    1 \\\n"
    "    2 \\\n"
    "    3 \\\n"
    "    4 \\\n"
    "    5 \\\n"
    "    6 \\\n"
    "    7 \\\n"
    "    8 \\\n"
    "    9 \\\n"
    "    10\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'An int' \\\n"
    "    'An unsigned' \\\n"
    "    'A double' \\\n"
    "    'A truth value' \\\n"
    "    'A character' \\\n"
    "    'A string' \\\n"
    "    'A text' \\\n"
    "    'An int from a list' \\\n"
    "    'An int a path' \\\n"
    "    'A string a number'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '1 1 n 1 0' \\\n"
    "    '1 1 n 1 0' \\\n"
    "    '1 1 n 1 0' \\\n"
    "    '1 1 n 1 0' \\\n"
    "    '1 1 n 1 0' \\\n"
    "    '1 1 f 1 0' \\\n"
    "    '1 1 f 1 0' \\\n"
    "    '1 1 c 1 2' \\\n"
    "    '1 1 f 3 0' \\\n"
    "    '1 1 n 3 0'\n"
    "set -g -- __djarg_prog_pk\n"
    "set -g -- __djarg_prog_pv\n"
    "set -g -- __djarg_prog_vv \\\n"
    "    '1' \\\n"
    "    '2'\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== types powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 12'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '--int'\n"
    "        '--unsigned'\n"
    "        '--double'\n"
    "        '--bool'\n"
    "        '--char'\n"
    "        '--string'\n"
    "        '--text'\n"
    "        '--int-choice'\n"
    "        '--int-path'\n"
    "        '--string-number'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        2\n"
    "        3\n"
    "        4\n"
    "        5\n"
    "        6\n"
    "        7\n"
    "        8\n"
    "        9\n"
    "        10\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'An int'\n"
    "        'An unsigned'\n"
    "        'A double'\n"
    "        'A truth value'\n";

// TREE_VALUES_9
//   constant: the transcript of d_tests_arg_completion_tree_values, continued.
static const char TREE_VALUES_9[] =
    "        'A character'\n"
    "        'A string'\n"
    "        'A text'\n"
    "        'An int from a list'\n"
    "        'An int a path'\n"
    "        'A string a number'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '1 1 n 0 0'\n"
    "        '1 1 n 0 0'\n"
    "        '1 1 n 0 0'\n"
    "        '1 1 n 0 0'\n"
    "        '1 1 n 0 0'\n"
    "        '1 1 f 0 0'\n"
    "        '1 1 f 0 0'\n"
    "        '1 1 c 0 2'\n"
    "        '1 1 f 2 0'\n"
    "        '1 1 n 2 0'\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "        '1'\n"
    "        '2'\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n"
    "== own types bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--enum') r=1 ;;\n"
    "        '0:--ints') r=2 ;;\n"
    "        '0:--strings') r=3 ;;\n"
    "        '0:--untyped') r=4 ;;\n"
    "        '0:--signed') r=5 ;;\n"
    "        '0:--floats') r=6 ;;\n"
    "        '0:--enum-choice') r=7 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=n ;;\n"
    "        2) n=1 x=1 k=n ;;\n"
    "        3) n=1 x=1 k=f ;;\n"
    "        4) n=1 x=1 k=f ;;\n"
    "        5) n=1 x=1 k=f ;;\n"
    "        6) n=1 x=1 k=n ;;\n"
    "        7) n=1 x=1 k=c ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--enum' '--ints' '--strings' '--untyped' "
    "'--signed' '--floats' '--enum-choice' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        7) c=( 'low' 'high' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== own types zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--enum') r=1 ;;\n"
    "        '0:--ints') r=2 ;;\n"
    "        '0:--strings') r=3 ;;\n"
    "        '0:--untyped') r=4 ;;\n"
    "        '0:--signed') r=5 ;;\n"
    "        '0:--floats') r=6 ;;\n"
    "        '0:--enum-choice') r=7 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=n ;;\n"
    "        2) n=1 x=1 k=n ;;\n"
    "        3) n=1 x=1 k=f ;;\n"
    "        4) n=1 x=1 k=f ;;\n"
    "        5) n=1 x=1 k=f ;;\n"
    "        6) n=1 x=1 k=n ;;\n"
    "        7) n=1 x=1 k=c ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--enum' '--ints' '--strings' '--untyped' "
    "'--signed' '--floats' '--enum-choice' ) d=( 'Print this help message and "
    "exit' 'Print this help message and exit' 'An enumeration' 'Integers' "
    "'Strings' 'No flags' 'Signed alone' 'Floating point numbers' 'An "
    "enumeration listed' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        7) c=( 'low' 'high' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== own types fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n";

// TREE_VALUES_10
//   constant: the transcript of d_tests_arg_completion_tree_values, continued.
static const char TREE_VALUES_10[] =
    "    '0 s 0 1 0 1 9'\n"
    "set -g -- __djarg_prog_sk\n"
    "set -g -- __djarg_prog_sn\n"
    "set -g -- __djarg_prog_st\n"
    "set -g -- __djarg_prog_sd\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '0:--enum' \\\n"
    "    '0:--ints' \\\n"
    "    '0:--strings' \\\n"
    "    '0:--untyped' \\\n"
    "    '0:--signed' \\\n"
    "    '0:--floats' \\\n"
    "    '0:--enum-choice'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '--enum' \\\n"
    "    '--ints' \\\n"
    "    '--strings' \\\n"
    "    '--untyped' \\\n"
    "    '--signed' \\\n"
    "    '--floats' \\\n"
    "    '--enum-choice'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    1 \\\n"
    "    2 \\\n"
    "    3 \\\n"
    "    4 \\\n"
    "    5 \\\n"
    "    6 \\\n"
    "    7\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'An enumeration' \\\n"
    "    'Integers' \\\n"
    "    'Strings' \\\n"
    "    'No flags' \\\n"
    "    'Signed alone' \\\n"
    "    'Floating point numbers' \\\n"
    "    'An enumeration listed'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '1 1 n 1 0' \\\n"
    "    '1 1 n 1 0' \\\n"
    "    '1 1 f 1 0' \\\n"
    "    '1 1 f 1 0' \\\n"
    "    '1 1 f 1 0' \\\n"
    "    '1 1 n 1 0' \\\n"
    "    '1 1 c 1 2'\n"
    "set -g -- __djarg_prog_pk\n"
    "set -g -- __djarg_prog_pv\n"
    "set -g -- __djarg_prog_vv \\\n"
    "    'low' \\\n"
    "    'high'\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== own types powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 9'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '--enum'\n"
    "        '--ints'\n"
    "        '--strings'\n"
    "        '--untyped'\n"
    "        '--signed'\n"
    "        '--floats'\n"
    "        '--enum-choice'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        2\n"
    "        3\n"
    "        4\n"
    "        5\n"
    "        6\n"
    "        7\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'An enumeration'\n"
    "        'Integers'\n"
    "        'Strings'\n"
    "        'No flags'\n"
    "        'Signed alone'\n"
    "        'Floating point numbers'\n"
    "        'An enumeration listed'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '1 1 n 0 0'\n"
    "        '1 1 n 0 0'\n"
    "        '1 1 f 0 0'\n"
    "        '1 1 f 0 0'\n"
    "        '1 1 f 0 0'\n"
    "        '1 1 n 0 0'\n"
    "        '1 1 c 0 2'\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "        'low'\n"
    "        'high'\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n"
    "== hidden bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:-s') r=3 ;;\n"
    "        '0:--regrouped') r=5 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=n ;;\n"
    "        2) n=1 x=1 k=n ;;\n"
    "        3) n=1 x=1 k=c ;;\n"
    "        4) n=1 x=1 k=d ;;\n"
    "        5) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        0:0) r=1 y=1 ;;\n"
    "        0:1) r=2 y=1 ;;\n"
    "        0:2) r=3 y=1 ;;\n"
    "        0:3) r=4 y=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '-s' '--regrouped' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        3) c=( 's1' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== hidden zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n";

// TREE_VALUES_11
//   constant: the transcript of d_tests_arg_completion_tree_values, continued.
static const char TREE_VALUES_11[] =
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:-s') r=3 ;;\n"
    "        '0:--regrouped') r=5 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=n ;;\n"
    "        2) n=1 x=1 k=n ;;\n"
    "        3) n=1 x=1 k=c ;;\n"
    "        4) n=1 x=1 k=d ;;\n"
    "        5) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        0:0) r=1 y=1 ;;\n"
    "        0:1) r=2 y=1 ;;\n"
    "        0:2) r=3 y=1 ;;\n"
    "        0:3) r=4 y=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '-s' '--regrouped' ) d=( 'Print this help "
    "message and exit' 'Print this help message and exit' 'Named and "
    "positional, shown' 'In another group' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        3) c=( 's1' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== hidden fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 0 1 4'\n"
    "set -g -- __djarg_prog_sk\n"
    "set -g -- __djarg_prog_sn\n"
    "set -g -- __djarg_prog_st\n"
    "set -g -- __djarg_prog_sd\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '0:-s' \\\n"
    "    '0:--regrouped'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-s' \\\n"
    "    '--regrouped'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    3 \\\n"
    "    5\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Named and positional, shown' \\\n"
    "    'In another group'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '1 1 n 1 0' \\\n"
    "    '1 1 n 1 0' \\\n"
    "    '1 1 c 1 1' \\\n"
    "    '1 1 d 2 0' \\\n"
    "    '0 0 - 2 0'\n"
    "set -g -- __djarg_prog_pk \\\n"
    "    '0:0' \\\n"
    "    '0:1' \\\n"
    "    '0:2' \\\n"
    "    '0:3'\n"
    "set -g -- __djarg_prog_pv \\\n"
    "    '1 1' \\\n"
    "    '2 1' \\\n"
    "    '3 1' \\\n"
    "    '4 1'\n"
    "set -g -- __djarg_prog_vv \\\n"
    "    's1'\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== hidden powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 4'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-s'\n"
    "        '--regrouped'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        3\n"
    "        5\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Named and positional, shown'\n"
    "        'In another group'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '1 1 n 0 0'\n"
    "        '1 1 n 0 0'\n"
    "        '1 1 c 0 1'\n"
    "        '1 1 d 1 0'\n"
    "        '0 0 - 1 0'\n"
    "    )\n"
    "    $ps = @(\n"
    "        '0:0'\n"
    "        '0:1'\n"
    "        '0:2'\n"
    "        '0:3'\n"
    "    )\n"
    "    $pv = @(\n"
    "        '1 1'\n"
    "        '2 1'\n"
    "        '3 1'\n"
    "        '4 1'\n"
    "    )\n"
    "    $vv = @(\n"
    "        's1'\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n";

// TREE_COMMANDS
//   constant: what d_tests_arg_completion_tree_commands writes, as
// completion_ref.py works it out from the rules arg_completion.h states, in
// parts.
static const char TREE_COMMANDS[] =
    "== subcommands bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        1) p=0 f=s m=0 ;;\n"
    "        2) p=0 f=s m=0 ;;\n"
    "        3) p=1 f=s m=0 ;;\n"
    "        4) p=1 f=s m=0 ;;\n"
    "        5) p=2 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        '0:alpha') r=1 ;;\n"
    "        '0:al') r=1 ;;\n"
    "        '0:a-lias') r=1 ;;\n"
    "        '0:beta') r=2 ;;\n"
    "        '1:one') r=3 ;;\n"
    "        '1:two') r=4 ;;\n"
    "        '2:three') r=5 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '1:-h') r=1 ;;\n"
    "        '1:--help') r=1 ;;\n"
    "        '2:-h') r=2 ;;\n"
    "        '2:--help') r=2 ;;\n"
    "        '3:-h') r=3 ;;\n"
    "        '3:--help') r=3 ;;\n"
    "        '3:--deep') r=4 ;;\n"
    "        '4:-h') r=5 ;;\n"
    "        '4:--help') r=5 ;;\n"
    "        '5:-h') r=6 ;;\n"
    "        '5:--help') r=6 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=0 x=0 k=- ;;\n"
    "        2) n=0 x=0 k=- ;;\n"
    "        3) n=0 x=0 k=- ;;\n"
    "        4) n=0 x=0 k=- ;;\n"
    "        5) n=0 x=0 k=- ;;\n"
    "        6) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        0) c=( 'alpha' 'al' 'a-lias' 'beta' ) ;;\n"
    "        1) c=( 'one' 'two' ) ;;\n"
    "        2) c=( 'three' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' ) ;;\n"
    "        1) c=( '-h' '--help' ) ;;\n"
    "        2) c=( '-h' '--help' ) ;;\n"
    "        3) c=( '-h' '--help' '--deep' ) ;;\n"
    "        4) c=( '-h' '--help' ) ;;\n"
    "        5) c=( '-h' '--help' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== subcommands zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        1) p=0 f=s m=0 ;;\n"
    "        2) p=0 f=s m=0 ;;\n"
    "        3) p=1 f=s m=0 ;;\n"
    "        4) p=1 f=s m=0 ;;\n"
    "        5) p=2 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        '0:alpha') r=1 ;;\n"
    "        '0:al') r=1 ;;\n"
    "        '0:a-lias') r=1 ;;\n"
    "        '0:beta') r=2 ;;\n"
    "        '1:one') r=3 ;;\n"
    "        '1:two') r=4 ;;\n"
    "        '2:three') r=5 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '1:-h') r=1 ;;\n"
    "        '1:--help') r=1 ;;\n"
    "        '2:-h') r=2 ;;\n"
    "        '2:--help') r=2 ;;\n"
    "        '3:-h') r=3 ;;\n"
    "        '3:--help') r=3 ;;\n"
    "        '3:--deep') r=4 ;;\n"
    "        '4:-h') r=5 ;;\n"
    "        '4:--help') r=5 ;;\n"
    "        '5:-h') r=6 ;;\n"
    "        '5:--help') r=6 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=0 x=0 k=- ;;\n"
    "        2) n=0 x=0 k=- ;;\n"
    "        3) n=0 x=0 k=- ;;\n"
    "        4) n=0 x=0 k=- ;;\n"
    "        5) n=0 x=0 k=- ;;\n"
    "        6) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        0) c=( 'alpha' 'al' 'a-lias' 'beta' ) d=( 'First' 'First' 'First' "
    "'padded' ) ;;\n"
    "        1) c=( 'one' 'two' ) d=( '' 'Two' ) ;;\n"
    "        2) c=( 'three' ) d=( 'Three' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        1) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        2) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n";

// TREE_COMMANDS_2
//   constant: the transcript of d_tests_arg_completion_tree_commands,
// continued.
static const char TREE_COMMANDS_2[] =
    "        3) c=( '-h' '--help' '--deep' ) d=( 'Print this help message and "
    "exit' 'Print this help message and exit' 'Deep' ) ;;\n"
    "        4) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        5) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== subcommands fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 4 1 2' \\\n"
    "    '0 s 0 5 2 3 2' \\\n"
    "    '0 s 0 7 1 5 2' \\\n"
    "    '1 s 0 8 0 7 3' \\\n"
    "    '1 s 0 8 0 10 2' \\\n"
    "    '2 s 0 8 0 12 2'\n"
    "set -g -- __djarg_prog_sk \\\n"
    "    '0:alpha' \\\n"
    "    '0:al' \\\n"
    "    '0:a-lias' \\\n"
    "    '0:beta' \\\n"
    "    '1:one' \\\n"
    "    '1:two' \\\n"
    "    '2:three'\n"
    "set -g -- __djarg_prog_sn \\\n"
    "    'alpha' \\\n"
    "    'al' \\\n"
    "    'a-lias' \\\n"
    "    'beta' \\\n"
    "    'one' \\\n"
    "    'two' \\\n"
    "    'three'\n"
    "set -g -- __djarg_prog_st \\\n"
    "    1 \\\n"
    "    1 \\\n"
    "    1 \\\n"
    "    2 \\\n"
    "    3 \\\n"
    "    4 \\\n"
    "    5\n"
    "set -g -- __djarg_prog_sd \\\n"
    "    'First' \\\n"
    "    'First' \\\n"
    "    'First' \\\n"
    "    'padded' \\\n"
    "    '' \\\n"
    "    'Two' \\\n"
    "    'Three'\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '1:-h' \\\n"
    "    '1:--help' \\\n"
    "    '2:-h' \\\n"
    "    '2:--help' \\\n"
    "    '3:-h' \\\n"
    "    '3:--help' \\\n"
    "    '3:--deep' \\\n"
    "    '4:-h' \\\n"
    "    '4:--help' \\\n"
    "    '5:-h' \\\n"
    "    '5:--help'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '--deep' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    1 \\\n"
    "    1 \\\n"
    "    2 \\\n"
    "    2 \\\n"
    "    3 \\\n"
    "    3 \\\n"
    "    4 \\\n"
    "    5 \\\n"
    "    5 \\\n"
    "    6 \\\n"
    "    6\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Deep' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0'\n"
    "set -g -- __djarg_prog_pk\n"
    "set -g -- __djarg_prog_pv\n"
    "set -g -- __djarg_prog_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== subcommands powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 4 0 2'\n"
    "        '0 s 0 4 2 2 2'\n"
    "        '0 s 0 6 1 4 2'\n"
    "        '1 s 0 7 0 6 3'\n"
    "        '1 s 0 7 0 9 2'\n"
    "        '2 s 0 7 0 11 2'\n"
    "    )\n"
    "    $sn = @(\n"
    "        'alpha'\n"
    "        'al'\n"
    "        'a-lias'\n"
    "        'beta'\n"
    "        'one'\n"
    "        'two'\n"
    "        'three'\n"
    "    )\n"
    "    $sc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        1\n"
    "        2\n"
    "    )\n"
    "    $st = @(\n"
    "        1\n"
    "        1\n"
    "        1\n"
    "        2\n"
    "        3\n"
    "        4\n"
    "        5\n"
    "    )\n"
    "    $sd = @(\n"
    "        'First'\n"
    "        'First'\n"
    "        'First'\n"
    "        'padded'\n"
    "        ''\n"
    "        'Two'\n"
    "        'Three'\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '--deep'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        1\n"
    "        2\n"
    "        2\n"
    "        3\n"
    "        3\n"
    "        3\n"
    "        4\n"
    "        4\n"
    "        5\n";

// TREE_COMMANDS_3
//   constant: the transcript of d_tests_arg_completion_tree_commands,
// continued.
static const char TREE_COMMANDS_3[] =
    "        5\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        1\n"
    "        2\n"
    "        2\n"
    "        3\n"
    "        3\n"
    "        4\n"
    "        5\n"
    "        5\n"
    "        6\n"
    "        6\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Deep'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n"
    "== disabled bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        1) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        '0:on') r=1 ;;\n"
    "        '0:yes') r=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '1:-h') r=1 ;;\n"
    "        '1:--help') r=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        0) c=( 'on' 'yes' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' ) ;;\n"
    "        1) c=( '-h' '--help' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== disabled zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        1) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        '0:on') r=1 ;;\n"
    "        '0:yes') r=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '1:-h') r=1 ;;\n"
    "        '1:--help') r=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        0) c=( 'on' 'yes' ) d=( 'On' 'On' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        1) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== disabled fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 2 1 2' \\\n"
    "    '0 s 0 3 0 3 2'\n"
    "set -g -- __djarg_prog_sk \\\n"
    "    '0:on' \\\n"
    "    '0:yes'\n"
    "set -g -- __djarg_prog_sn \\\n"
    "    'on' \\\n";

// TREE_COMMANDS_4
//   constant: the transcript of d_tests_arg_completion_tree_commands,
// continued.
static const char TREE_COMMANDS_4[] =
    "    'yes'\n"
    "set -g -- __djarg_prog_st \\\n"
    "    1 \\\n"
    "    1\n"
    "set -g -- __djarg_prog_sd \\\n"
    "    'On' \\\n"
    "    'On'\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '1:-h' \\\n"
    "    '1:--help'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    1 \\\n"
    "    1\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0'\n"
    "set -g -- __djarg_prog_pk\n"
    "set -g -- __djarg_prog_pv\n"
    "set -g -- __djarg_prog_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== disabled powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 2 0 2'\n"
    "        '0 s 0 2 0 2 2'\n"
    "    )\n"
    "    $sn = @(\n"
    "        'on'\n"
    "        'yes'\n"
    "    )\n"
    "    $sc = @(\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $st = @(\n"
    "        1\n"
    "        1\n"
    "    )\n"
    "    $sd = @(\n"
    "        'On'\n"
    "        'On'\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        1\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        1\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n"
    "== groups bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        1) p=0 f=s m=0 ;;\n"
    "        2) p=0 f=s m=0 ;;\n"
    "        3) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        '0:gamma') r=1 ;;\n"
    "        '0:delta') r=2 ;;\n"
    "        '0:alpha') r=3 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--help-all') r=1 ;;\n"
    "        '0:--late') r=2 ;;\n"
    "        '0:--gflag') r=4 ;;\n"
    "        '0:--nflag') r=6 ;;\n"
    "        '0:--pflag') r=7 ;;\n"
    "        '1:--help-all') r=8 ;;\n"
    "        '2:--help-all') r=9 ;;\n"
    "        '3:-h') r=10 ;;\n"
    "        '3:--help') r=10 ;;\n"
    "        '3:--help-all') r=11 ;;\n"
    "        '3:--inner') r=13 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=0 x=0 k=- ;;\n"
    "        2) n=0 x=0 k=- ;;\n"
    "        3) n=0 x=0 k=- ;;\n"
    "        4) n=0 x=0 k=- ;;\n"
    "        5) n=0 x=0 k=- ;;\n"
    "        6) n=0 x=0 k=- ;;\n"
    "        7) n=0 x=0 k=- ;;\n"
    "        8) n=0 x=0 k=- ;;\n"
    "        9) n=0 x=0 k=- ;;\n"
    "        10) n=0 x=0 k=- ;;\n"
    "        11) n=0 x=0 k=- ;;\n"
    "        12) n=0 x=0 k=- ;;\n"
    "        13) n=0 x=0 k=- ;;\n"
    "        14) n=1 x=1 k=f ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        3:0) r=14 y=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        0) c=( 'gamma' 'delta' 'alpha' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--help-all' '--late' '--gflag' '--nflag' "
    "'--pflag' ) ;;\n"
    "        1) c=( '--help-all' ) ;;\n"
    "        2) c=( '--help-all' ) ;;\n"
    "        3) c=( '-h' '--help' '--help-all' '--inner' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== groups zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        1) p=0 f=s m=0 ;;\n";

// TREE_COMMANDS_5
//   constant: the transcript of d_tests_arg_completion_tree_commands,
// continued.
static const char TREE_COMMANDS_5[] =
    "        2) p=0 f=s m=0 ;;\n"
    "        3) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        '0:gamma') r=1 ;;\n"
    "        '0:delta') r=2 ;;\n"
    "        '0:alpha') r=3 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--help-all') r=1 ;;\n"
    "        '0:--late') r=2 ;;\n"
    "        '0:--gflag') r=4 ;;\n"
    "        '0:--nflag') r=6 ;;\n"
    "        '0:--pflag') r=7 ;;\n"
    "        '1:--help-all') r=8 ;;\n"
    "        '2:--help-all') r=9 ;;\n"
    "        '3:-h') r=10 ;;\n"
    "        '3:--help') r=10 ;;\n"
    "        '3:--help-all') r=11 ;;\n"
    "        '3:--inner') r=13 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=0 x=0 k=- ;;\n"
    "        2) n=0 x=0 k=- ;;\n"
    "        3) n=0 x=0 k=- ;;\n"
    "        4) n=0 x=0 k=- ;;\n"
    "        5) n=0 x=0 k=- ;;\n"
    "        6) n=0 x=0 k=- ;;\n"
    "        7) n=0 x=0 k=- ;;\n"
    "        8) n=0 x=0 k=- ;;\n"
    "        9) n=0 x=0 k=- ;;\n"
    "        10) n=0 x=0 k=- ;;\n"
    "        11) n=0 x=0 k=- ;;\n"
    "        12) n=0 x=0 k=- ;;\n"
    "        13) n=0 x=0 k=- ;;\n"
    "        14) n=1 x=1 k=f ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        3:0) r=14 y=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        0) c=( 'gamma' 'delta' 'alpha' ) d=( 'In a group' 'Nested deeper' "
    "'After the group' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--help-all' '--late' '--gflag' '--nflag' "
    "'--pflag' ) d=( 'Print this help message and exit' 'Print this help "
    "message and exit' 'Expand all help' 'A root flag after the groups' 'A "
    "group flag' 'A nested flag' 'A plus flag' ) ;;\n"
    "        1) c=( '--help-all' ) d=( 'Expand all help' ) ;;\n"
    "        2) c=( '--help-all' ) d=( 'Expand all help' ) ;;\n"
    "        3) c=( '-h' '--help' '--help-all' '--inner' ) d=( 'Print this "
    "help message and exit' 'Print this help message and exit' 'Expand all "
    "help' 'Inner flag' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== groups fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 3 1 7' \\\n"
    "    '0 s 0 4 0 8 1' \\\n"
    "    '0 s 0 4 0 9 1' \\\n"
    "    '0 s 0 4 0 10 4'\n"
    "set -g -- __djarg_prog_sk \\\n"
    "    '0:gamma' \\\n"
    "    '0:delta' \\\n"
    "    '0:alpha'\n"
    "set -g -- __djarg_prog_sn \\\n"
    "    'gamma' \\\n"
    "    'delta' \\\n"
    "    'alpha'\n"
    "set -g -- __djarg_prog_st \\\n"
    "    1 \\\n"
    "    2 \\\n"
    "    3\n"
    "set -g -- __djarg_prog_sd \\\n"
    "    'In a group' \\\n"
    "    'Nested deeper' \\\n"
    "    'After the group'\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '0:--help-all' \\\n"
    "    '0:--late' \\\n"
    "    '0:--gflag' \\\n"
    "    '0:--nflag' \\\n"
    "    '0:--pflag' \\\n"
    "    '1:--help-all' \\\n"
    "    '2:--help-all' \\\n"
    "    '3:-h' \\\n"
    "    '3:--help' \\\n"
    "    '3:--help-all' \\\n"
    "    '3:--inner'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '--help-all' \\\n"
    "    '--late' \\\n"
    "    '--gflag' \\\n"
    "    '--nflag' \\\n"
    "    '--pflag' \\\n"
    "    '--help-all' \\\n"
    "    '--help-all' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '--help-all' \\\n"
    "    '--inner'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    1 \\\n"
    "    2 \\\n"
    "    4 \\\n"
    "    6 \\\n"
    "    7 \\\n"
    "    8 \\\n"
    "    9 \\\n"
    "    10 \\\n"
    "    10 \\\n"
    "    11 \\\n"
    "    13\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Expand all help' \\\n"
    "    'A root flag after the groups' \\\n"
    "    'A group flag' \\\n"
    "    'A nested flag' \\\n"
    "    'A plus flag' \\\n"
    "    'Expand all help' \\\n"
    "    'Expand all help' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Expand all help' \\\n"
    "    'Inner flag'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n";

// TREE_COMMANDS_6
//   constant: the transcript of d_tests_arg_completion_tree_commands,
// continued.
static const char TREE_COMMANDS_6[] =
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '1 1 f 1 0'\n"
    "set -g -- __djarg_prog_pk \\\n"
    "    '3:0'\n"
    "set -g -- __djarg_prog_pv \\\n"
    "    '14 1'\n"
    "set -g -- __djarg_prog_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== groups powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 3 0 7'\n"
    "        '0 s 0 3 0 7 1'\n"
    "        '0 s 0 3 0 8 1'\n"
    "        '0 s 0 3 0 9 4'\n"
    "    )\n"
    "    $sn = @(\n"
    "        'gamma'\n"
    "        'delta'\n"
    "        'alpha'\n"
    "    )\n"
    "    $sc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $st = @(\n"
    "        1\n"
    "        2\n"
    "        3\n"
    "    )\n"
    "    $sd = @(\n"
    "        'In a group'\n"
    "        'Nested deeper'\n"
    "        'After the group'\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '--help-all'\n"
    "        '--late'\n"
    "        '--gflag'\n"
    "        '--nflag'\n"
    "        '--pflag'\n"
    "        '--help-all'\n"
    "        '--help-all'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '--help-all'\n"
    "        '--inner'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        2\n"
    "        3\n"
    "        3\n"
    "        3\n"
    "        3\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        2\n"
    "        4\n"
    "        6\n"
    "        7\n"
    "        8\n"
    "        9\n"
    "        10\n"
    "        10\n"
    "        11\n"
    "        13\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Expand all help'\n"
    "        'A root flag after the groups'\n"
    "        'A group flag'\n"
    "        'A nested flag'\n"
    "        'A plus flag'\n"
    "        'Expand all help'\n"
    "        'Expand all help'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Expand all help'\n"
    "        'Inner flag'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '1 1 f 0 0'\n"
    "    )\n"
    "    $ps = @(\n"
    "        '3:0'\n"
    "    )\n"
    "    $pv = @(\n"
    "        '14 1'\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n"
    "== names listed once bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        1) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        '0:sub') r=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--help-all') r=1 ;;\n"
    "        '0:-Y') r=2 ;;\n"
    "        '0:--yes') r=2 ;;\n"
    "        '0:-y') r=3 ;;\n"
    "        '0:--only-here') r=5 ;;\n"
    "        '0:--elsewhere') r=7 ;;\n"
    "        '1:-h') r=8 ;;\n"
    "        '1:--help') r=8 ;;\n"
    "        '1:--help-all') r=9 ;;\n"
    "        '1:-z') r=10 ;;\n"
    "        '1:--zed') r=10 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=0 x=0 k=- ;;\n"
    "        2) n=1 x=1 k=f ;;\n"
    "        3) n=0 x=0 k=- ;;\n"
    "        4) n=0 x=0 k=- ;;\n"
    "        5) n=0 x=0 k=- ;;\n"
    "        6) n=0 x=0 k=- ;;\n"
    "        7) n=0 x=0 k=- ;;\n"
    "        8) n=0 x=0 k=- ;;\n"
    "        9) n=0 x=0 k=- ;;\n"
    "        10) n=0 x=0 k=- ;;\n"
    "        11) n=0 x=0 k=- ;;\n"
    "        12) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        0) c=( 'sub' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--help-all' '-Y' '--yes' '-y' '--only-here' "
    "'--elsewhere' ) ;;\n";

// TREE_COMMANDS_7
//   constant: the transcript of d_tests_arg_completion_tree_commands,
// continued.
static const char TREE_COMMANDS_7[] =
    "        1) c=( '-h' '--help' '--help-all' '-z' '--zed' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== names listed once zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        1) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        '0:sub') r=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--help-all') r=1 ;;\n"
    "        '0:-Y') r=2 ;;\n"
    "        '0:--yes') r=2 ;;\n"
    "        '0:-y') r=3 ;;\n"
    "        '0:--only-here') r=5 ;;\n"
    "        '0:--elsewhere') r=7 ;;\n"
    "        '1:-h') r=8 ;;\n"
    "        '1:--help') r=8 ;;\n"
    "        '1:--help-all') r=9 ;;\n"
    "        '1:-z') r=10 ;;\n"
    "        '1:--zed') r=10 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=0 x=0 k=- ;;\n"
    "        2) n=1 x=1 k=f ;;\n"
    "        3) n=0 x=0 k=- ;;\n"
    "        4) n=0 x=0 k=- ;;\n"
    "        5) n=0 x=0 k=- ;;\n"
    "        6) n=0 x=0 k=- ;;\n"
    "        7) n=0 x=0 k=- ;;\n"
    "        8) n=0 x=0 k=- ;;\n"
    "        9) n=0 x=0 k=- ;;\n"
    "        10) n=0 x=0 k=- ;;\n"
    "        11) n=0 x=0 k=- ;;\n"
    "        12) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        0) c=( 'sub' ) d=( 'A subcommand' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--help-all' '-Y' '--yes' '-y' '--only-here' "
    "'--elsewhere' ) d=( 'Print this help message and exit' 'Print this help "
    "message and exit' 'Expand all help' 'The command'\\''s, found first' 'The "
    "command'\\''s, found first' 'The command'\\''s too' 'The group'\\''s "
    "flag' 'Its own' ) ;;\n"
    "        1) c=( '-h' '--help' '--help-all' '-z' '--zed' ) d=( 'Print this "
    "help message and exit' 'Print this help message and exit' 'Expand all "
    "help' 'The subcommand'\\''s, found first' 'The subcommand'\\''s, found "
    "first' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== names listed once fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 1 1 8' \\\n"
    "    '0 s 0 2 0 9 5'\n"
    "set -g -- __djarg_prog_sk \\\n"
    "    '0:sub'\n"
    "set -g -- __djarg_prog_sn \\\n"
    "    'sub'\n"
    "set -g -- __djarg_prog_st \\\n"
    "    1\n"
    "set -g -- __djarg_prog_sd \\\n"
    "    'A subcommand'\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '0:--help-all' \\\n"
    "    '0:-Y' \\\n"
    "    '0:--yes' \\\n"
    "    '0:-y' \\\n"
    "    '0:--only-here' \\\n"
    "    '0:--elsewhere' \\\n"
    "    '1:-h' \\\n"
    "    '1:--help' \\\n"
    "    '1:--help-all' \\\n"
    "    '1:-z' \\\n"
    "    '1:--zed'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '--help-all' \\\n"
    "    '-Y' \\\n"
    "    '--yes' \\\n"
    "    '-y' \\\n"
    "    '--only-here' \\\n"
    "    '--elsewhere' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '--help-all' \\\n"
    "    '-z' \\\n"
    "    '--zed'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    1 \\\n"
    "    2 \\\n"
    "    2 \\\n"
    "    3 \\\n"
    "    5 \\\n"
    "    7 \\\n"
    "    8 \\\n"
    "    8 \\\n"
    "    9 \\\n"
    "    10 \\\n"
    "    10\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Expand all help' \\\n"
    "    'The command\\'s, found first' \\\n"
    "    'The command\\'s, found first' \\\n"
    "    'The command\\'s too' \\\n"
    "    'The group\\'s flag' \\\n"
    "    'Its own' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Expand all help' \\\n"
    "    'The subcommand\\'s, found first' \\\n";

// TREE_COMMANDS_8
//   constant: the transcript of d_tests_arg_completion_tree_commands,
// continued.
static const char TREE_COMMANDS_8[] =
    "    'The subcommand\\'s, found first'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '1 1 f 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0'\n"
    "set -g -- __djarg_prog_pk\n"
    "set -g -- __djarg_prog_pv\n"
    "set -g -- __djarg_prog_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== names listed once powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 1 0 8'\n"
    "        '0 s 0 1 0 8 5'\n"
    "    )\n"
    "    $sn = @(\n"
    "        'sub'\n"
    "    )\n"
    "    $sc = @(\n"
    "        0\n"
    "    )\n"
    "    $st = @(\n"
    "        1\n"
    "    )\n"
    "    $sd = @(\n"
    "        'A subcommand'\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '--help-all'\n"
    "        '-Y'\n"
    "        '--yes'\n"
    "        '-y'\n"
    "        '--only-here'\n"
    "        '--elsewhere'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '--help-all'\n"
    "        '-z'\n"
    "        '--zed'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        1\n"
    "        1\n"
    "        1\n"
    "        1\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        2\n"
    "        2\n"
    "        3\n"
    "        5\n"
    "        7\n"
    "        8\n"
    "        8\n"
    "        9\n"
    "        10\n"
    "        10\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Expand all help'\n"
    "        'The command''s, found first'\n"
    "        'The command''s, found first'\n"
    "        'The command''s too'\n"
    "        'The group''s flag'\n"
    "        'Its own'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Expand all help'\n"
    "        'The subcommand''s, found first'\n"
    "        'The subcommand''s, found first'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '1 1 f 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n"
    "== order bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        1) p=0 f=s m=0 ;;\n"
    "        2) p=0 f=s m=0 ;;\n"
    "        3) p=0 f=s m=0 ;;\n"
    "        4) p=1 f=s m=0 ;;\n"
    "        5) p=1 f=s m=0 ;;\n"
    "        6) p=2 f=s m=0 ;;\n"
    "        7) p=3 f=s m=0 ;;\n"
    "        8) p=5 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        '0:a') r=1 ;;\n"
    "        '0:b') r=2 ;;\n"
    "        '0:c') r=3 ;;\n"
    "        '1:a1') r=4 ;;\n"
    "        '1:a2') r=5 ;;\n"
    "        '2:b1') r=6 ;;\n"
    "        '3:c1') r=7 ;;\n"
    "        '5:a21') r=8 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '1:-h') r=1 ;;\n"
    "        '1:--help') r=1 ;;\n"
    "        '1:-a') r=2 ;;\n"
    "        '2:-h') r=3 ;;\n"
    "        '2:--help') r=3 ;;\n"
    "        '3:-h') r=4 ;;\n"
    "        '3:--help') r=4 ;;\n"
    "        '3:-c') r=5 ;;\n"
    "        '4:-h') r=6 ;;\n"
    "        '4:--help') r=6 ;;\n"
    "        '5:-h') r=7 ;;\n"
    "        '5:--help') r=7 ;;\n"
    "        '6:-h') r=8 ;;\n"
    "        '6:--help') r=8 ;;\n"
    "        '7:-h') r=9 ;;\n"
    "        '7:--help') r=9 ;;\n"
    "        '8:-h') r=10 ;;\n"
    "        '8:--help') r=10 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=0 x=0 k=- ;;\n"
    "        2) n=0 x=0 k=- ;;\n"
    "        3) n=0 x=0 k=- ;;\n"
    "        4) n=0 x=0 k=- ;;\n"
    "        5) n=0 x=0 k=- ;;\n"
    "        6) n=0 x=0 k=- ;;\n"
    "        7) n=0 x=0 k=- ;;\n"
    "        8) n=0 x=0 k=- ;;\n"
    "        9) n=0 x=0 k=- ;;\n"
    "        10) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n";

// TREE_COMMANDS_9
//   constant: the transcript of d_tests_arg_completion_tree_commands,
// continued.
static const char TREE_COMMANDS_9[] =
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        0) c=( 'a' 'b' 'c' ) ;;\n"
    "        1) c=( 'a1' 'a2' ) ;;\n"
    "        2) c=( 'b1' ) ;;\n"
    "        3) c=( 'c1' ) ;;\n"
    "        5) c=( 'a21' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' ) ;;\n"
    "        1) c=( '-h' '--help' '-a' ) ;;\n"
    "        2) c=( '-h' '--help' ) ;;\n"
    "        3) c=( '-h' '--help' '-c' ) ;;\n"
    "        4) c=( '-h' '--help' ) ;;\n"
    "        5) c=( '-h' '--help' ) ;;\n"
    "        6) c=( '-h' '--help' ) ;;\n"
    "        7) c=( '-h' '--help' ) ;;\n"
    "        8) c=( '-h' '--help' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== order zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        1) p=0 f=s m=0 ;;\n"
    "        2) p=0 f=s m=0 ;;\n"
    "        3) p=0 f=s m=0 ;;\n"
    "        4) p=1 f=s m=0 ;;\n"
    "        5) p=1 f=s m=0 ;;\n"
    "        6) p=2 f=s m=0 ;;\n"
    "        7) p=3 f=s m=0 ;;\n"
    "        8) p=5 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        '0:a') r=1 ;;\n"
    "        '0:b') r=2 ;;\n"
    "        '0:c') r=3 ;;\n"
    "        '1:a1') r=4 ;;\n"
    "        '1:a2') r=5 ;;\n"
    "        '2:b1') r=6 ;;\n"
    "        '3:c1') r=7 ;;\n"
    "        '5:a21') r=8 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '1:-h') r=1 ;;\n"
    "        '1:--help') r=1 ;;\n"
    "        '1:-a') r=2 ;;\n"
    "        '2:-h') r=3 ;;\n"
    "        '2:--help') r=3 ;;\n"
    "        '3:-h') r=4 ;;\n"
    "        '3:--help') r=4 ;;\n"
    "        '3:-c') r=5 ;;\n"
    "        '4:-h') r=6 ;;\n"
    "        '4:--help') r=6 ;;\n"
    "        '5:-h') r=7 ;;\n"
    "        '5:--help') r=7 ;;\n"
    "        '6:-h') r=8 ;;\n"
    "        '6:--help') r=8 ;;\n"
    "        '7:-h') r=9 ;;\n"
    "        '7:--help') r=9 ;;\n"
    "        '8:-h') r=10 ;;\n"
    "        '8:--help') r=10 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=0 x=0 k=- ;;\n"
    "        2) n=0 x=0 k=- ;;\n"
    "        3) n=0 x=0 k=- ;;\n"
    "        4) n=0 x=0 k=- ;;\n"
    "        5) n=0 x=0 k=- ;;\n"
    "        6) n=0 x=0 k=- ;;\n"
    "        7) n=0 x=0 k=- ;;\n"
    "        8) n=0 x=0 k=- ;;\n"
    "        9) n=0 x=0 k=- ;;\n"
    "        10) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        0) c=( 'a' 'b' 'c' ) d=( '' '' '' ) ;;\n"
    "        1) c=( 'a1' 'a2' ) d=( '' '' ) ;;\n"
    "        2) c=( 'b1' ) d=( '' ) ;;\n"
    "        3) c=( 'c1' ) d=( '' ) ;;\n"
    "        5) c=( 'a21' ) d=( '' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        1) c=( '-h' '--help' '-a' ) d=( 'Print this help message and "
    "exit' 'Print this help message and exit' '' ) ;;\n"
    "        2) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        3) c=( '-h' '--help' '-c' ) d=( 'Print this help message and "
    "exit' 'Print this help message and exit' '' ) ;;\n"
    "        4) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        5) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        6) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        7) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        8) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n";

// TREE_COMMANDS_10
//   constant: the transcript of d_tests_arg_completion_tree_commands,
// continued.
static const char TREE_COMMANDS_10[] =
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== order fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 3 1 2' \\\n"
    "    '0 s 0 4 2 3 3' \\\n"
    "    '0 s 0 6 1 6 2' \\\n"
    "    '0 s 0 7 1 8 3' \\\n"
    "    '1 s 0 8 0 11 2' \\\n"
    "    '1 s 0 8 1 13 2' \\\n"
    "    '2 s 0 9 0 15 2' \\\n"
    "    '3 s 0 9 0 17 2' \\\n"
    "    '5 s 0 9 0 19 2'\n"
    "set -g -- __djarg_prog_sk \\\n"
    "    '0:a' \\\n"
    "    '0:b' \\\n"
    "    '0:c' \\\n"
    "    '1:a1' \\\n"
    "    '1:a2' \\\n"
    "    '2:b1' \\\n"
    "    '3:c1' \\\n"
    "    '5:a21'\n"
    "set -g -- __djarg_prog_sn \\\n"
    "    'a' \\\n"
    "    'b' \\\n"
    "    'c' \\\n"
    "    'a1' \\\n"
    "    'a2' \\\n"
    "    'b1' \\\n"
    "    'c1' \\\n"
    "    'a21'\n"
    "set -g -- __djarg_prog_st \\\n"
    "    1 \\\n"
    "    2 \\\n"
    "    3 \\\n"
    "    4 \\\n"
    "    5 \\\n"
    "    6 \\\n"
    "    7 \\\n"
    "    8\n"
    "set -g -- __djarg_prog_sd \\\n"
    "    '' \\\n"
    "    '' \\\n"
    "    '' \\\n"
    "    '' \\\n"
    "    '' \\\n"
    "    '' \\\n"
    "    '' \\\n"
    "    ''\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '1:-h' \\\n"
    "    '1:--help' \\\n"
    "    '1:-a' \\\n"
    "    '2:-h' \\\n"
    "    '2:--help' \\\n"
    "    '3:-h' \\\n"
    "    '3:--help' \\\n"
    "    '3:-c' \\\n"
    "    '4:-h' \\\n"
    "    '4:--help' \\\n"
    "    '5:-h' \\\n"
    "    '5:--help' \\\n"
    "    '6:-h' \\\n"
    "    '6:--help' \\\n"
    "    '7:-h' \\\n"
    "    '7:--help' \\\n"
    "    '8:-h' \\\n"
    "    '8:--help'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-a' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-c' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    1 \\\n"
    "    1 \\\n"
    "    2 \\\n"
    "    3 \\\n"
    "    3 \\\n"
    "    4 \\\n"
    "    4 \\\n"
    "    5 \\\n"
    "    6 \\\n"
    "    6 \\\n"
    "    7 \\\n"
    "    7 \\\n"
    "    8 \\\n"
    "    8 \\\n"
    "    9 \\\n"
    "    9 \\\n"
    "    10 \\\n"
    "    10\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    '' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    '' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0'\n"
    "set -g -- __djarg_prog_pk\n"
    "set -g -- __djarg_prog_pv\n"
    "set -g -- __djarg_prog_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== order powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 3 0 2'\n"
    "        '0 s 0 3 2 2 3'\n"
    "        '0 s 0 5 1 5 2'\n"
    "        '0 s 0 6 1 7 3'\n"
    "        '1 s 0 7 0 10 2'\n"
    "        '1 s 0 7 1 12 2'\n"
    "        '2 s 0 8 0 14 2'\n"
    "        '3 s 0 8 0 16 2'\n"
    "        '5 s 0 8 0 18 2'\n"
    "    )\n"
    "    $sn = @(\n"
    "        'a'\n"
    "        'b'\n"
    "        'c'\n"
    "        'a1'\n"
    "        'a2'\n"
    "        'b1'\n"
    "        'c1'\n"
    "        'a21'\n"
    "    )\n"
    "    $sc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        1\n"
    "        2\n"
    "        3\n"
    "        5\n"
    "    )\n"
    "    $st = @(\n"
    "        1\n"
    "        2\n"
    "        3\n"
    "        4\n"
    "        5\n"
    "        6\n"
    "        7\n"
    "        8\n"
    "    )\n"
    "    $sd = @(\n"
    "        ''\n"
    "        ''\n"
    "        ''\n"
    "        ''\n"
    "        ''\n"
    "        ''\n"
    "        ''\n"
    "        ''\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-a'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-c'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n";

// TREE_COMMANDS_11
//   constant: the transcript of d_tests_arg_completion_tree_commands,
// continued.
static const char TREE_COMMANDS_11[] =
    "        '-h'\n"
    "        '--help'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        1\n"
    "        1\n"
    "        2\n"
    "        2\n"
    "        3\n"
    "        3\n"
    "        3\n"
    "        4\n"
    "        4\n"
    "        5\n"
    "        5\n"
    "        6\n"
    "        6\n"
    "        7\n"
    "        7\n"
    "        8\n"
    "        8\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        1\n"
    "        2\n"
    "        3\n"
    "        3\n"
    "        4\n"
    "        4\n"
    "        5\n"
    "        6\n"
    "        6\n"
    "        7\n"
    "        7\n"
    "        8\n"
    "        8\n"
    "        9\n"
    "        9\n"
    "        10\n"
    "        10\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        ''\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        ''\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n";

// TREE_SWITCHES
//   constant: what d_tests_arg_completion_tree_switches writes, as
// completion_ref.py works it out from the rules arg_completion.h states, in
// parts.
static const char TREE_SWITCHES[] =
    "== each bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        1) p=0 f=sf m=0 ;;\n"
    "        2) p=0 f=- m=0 ;;\n"
    "        3) p=0 f=sw m=0 ;;\n"
    "        4) p=0 f=se m=0 ;;\n"
    "        5) p=0 f=se m=0 ;;\n"
    "        6) p=0 f=se m=0 ;;\n"
    "        7) p=0 f=se m=0 ;;\n"
    "        8) p=0 f=s m=0 ;;\n"
    "        9) p=0 f=se m=0 ;;\n"
    "        10) p=0 f=se m=0 ;;\n"
    "        11) p=0 f=se m=0 ;;\n"
    "        12) p=0 f=s m=2 ;;\n"
    "        13) p=0 f=- m=0 ;;\n"
    "        14) p=0 f=sfwe m=7 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        '0:fall') r=1 ;;\n"
    "        '0:nosub') r=2 ;;\n"
    "        '0:win') r=3 ;;\n"
    "        '0:capture') r=4 ;;\n"
    "        '0:ignore') r=5 ;;\n"
    "        '0:single') r=6 ;;\n"
    "        '0:multiple') r=7 ;;\n"
    "        '0:immediately') r=8 ;;\n"
    "        '0:prefix') r=9 ;;\n"
    "        '0:separator') r=10 ;;\n"
    "        '0:positional') r=11 ;;\n"
    "        '0:two') r=12 ;;\n"
    "        '0:none') r=13 ;;\n"
    "        '0:every') r=14 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '1:-h') r=1 ;;\n"
    "        '1:--help') r=1 ;;\n"
    "        '2:-h') r=2 ;;\n"
    "        '2:--help') r=2 ;;\n"
    "        '3:-h') r=3 ;;\n"
    "        '3:--help') r=3 ;;\n"
    "        '4:-h') r=4 ;;\n"
    "        '4:--help') r=4 ;;\n"
    "        '5:-h') r=5 ;;\n"
    "        '5:--help') r=5 ;;\n"
    "        '6:-h') r=6 ;;\n"
    "        '6:--help') r=6 ;;\n"
    "        '7:-h') r=7 ;;\n"
    "        '7:--help') r=7 ;;\n"
    "        '8:-h') r=8 ;;\n"
    "        '8:--help') r=8 ;;\n"
    "        '9:-h') r=9 ;;\n"
    "        '9:--help') r=9 ;;\n"
    "        '10:-h') r=10 ;;\n"
    "        '10:--help') r=10 ;;\n"
    "        '11:-h') r=11 ;;\n"
    "        '11:--help') r=11 ;;\n"
    "        '12:-h') r=12 ;;\n"
    "        '12:--help') r=12 ;;\n"
    "        '13:-h') r=13 ;;\n"
    "        '13:--help') r=13 ;;\n"
    "        '14:-h') r=14 ;;\n"
    "        '14:--help') r=14 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=0 x=0 k=- ;;\n"
    "        2) n=0 x=0 k=- ;;\n"
    "        3) n=0 x=0 k=- ;;\n"
    "        4) n=0 x=0 k=- ;;\n"
    "        5) n=0 x=0 k=- ;;\n"
    "        6) n=0 x=0 k=- ;;\n"
    "        7) n=0 x=0 k=- ;;\n"
    "        8) n=0 x=0 k=- ;;\n"
    "        9) n=0 x=0 k=- ;;\n"
    "        10) n=0 x=0 k=- ;;\n"
    "        11) n=0 x=0 k=- ;;\n"
    "        12) n=0 x=0 k=- ;;\n"
    "        13) n=0 x=0 k=- ;;\n"
    "        14) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        0) c=( 'fall' 'nosub' 'win' 'capture' 'ignore' 'single' "
    "'multiple' 'immediately' 'prefix' 'separator' 'positional' 'two' 'none' "
    "'every' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' ) ;;\n"
    "        1) c=( '-h' '--help' ) ;;\n"
    "        2) c=( '-h' '--help' ) ;;\n"
    "        3) c=( '-h' '--help' ) ;;\n"
    "        4) c=( '-h' '--help' ) ;;\n"
    "        5) c=( '-h' '--help' ) ;;\n"
    "        6) c=( '-h' '--help' ) ;;\n"
    "        7) c=( '-h' '--help' ) ;;\n"
    "        8) c=( '-h' '--help' ) ;;\n"
    "        9) c=( '-h' '--help' ) ;;\n"
    "        10) c=( '-h' '--help' ) ;;\n"
    "        11) c=( '-h' '--help' ) ;;\n"
    "        12) c=( '-h' '--help' ) ;;\n"
    "        13) c=( '-h' '--help' ) ;;\n"
    "        14) c=( '-h' '--help' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== each zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        1) p=0 f=sf m=0 ;;\n"
    "        2) p=0 f=- m=0 ;;\n"
    "        3) p=0 f=sw m=0 ;;\n"
    "        4) p=0 f=se m=0 ;;\n"
    "        5) p=0 f=se m=0 ;;\n"
    "        6) p=0 f=se m=0 ;;\n"
    "        7) p=0 f=se m=0 ;;\n"
    "        8) p=0 f=s m=0 ;;\n"
    "        9) p=0 f=se m=0 ;;\n"
    "        10) p=0 f=se m=0 ;;\n"
    "        11) p=0 f=se m=0 ;;\n"
    "        12) p=0 f=s m=2 ;;\n"
    "        13) p=0 f=- m=0 ;;\n"
    "        14) p=0 f=sfwe m=7 ;;\n";

// TREE_SWITCHES_2
//   constant: the transcript of d_tests_arg_completion_tree_switches,
// continued.
static const char TREE_SWITCHES_2[] =
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        '0:fall') r=1 ;;\n"
    "        '0:nosub') r=2 ;;\n"
    "        '0:win') r=3 ;;\n"
    "        '0:capture') r=4 ;;\n"
    "        '0:ignore') r=5 ;;\n"
    "        '0:single') r=6 ;;\n"
    "        '0:multiple') r=7 ;;\n"
    "        '0:immediately') r=8 ;;\n"
    "        '0:prefix') r=9 ;;\n"
    "        '0:separator') r=10 ;;\n"
    "        '0:positional') r=11 ;;\n"
    "        '0:two') r=12 ;;\n"
    "        '0:none') r=13 ;;\n"
    "        '0:every') r=14 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '1:-h') r=1 ;;\n"
    "        '1:--help') r=1 ;;\n"
    "        '2:-h') r=2 ;;\n"
    "        '2:--help') r=2 ;;\n"
    "        '3:-h') r=3 ;;\n"
    "        '3:--help') r=3 ;;\n"
    "        '4:-h') r=4 ;;\n"
    "        '4:--help') r=4 ;;\n"
    "        '5:-h') r=5 ;;\n"
    "        '5:--help') r=5 ;;\n"
    "        '6:-h') r=6 ;;\n"
    "        '6:--help') r=6 ;;\n"
    "        '7:-h') r=7 ;;\n"
    "        '7:--help') r=7 ;;\n"
    "        '8:-h') r=8 ;;\n"
    "        '8:--help') r=8 ;;\n"
    "        '9:-h') r=9 ;;\n"
    "        '9:--help') r=9 ;;\n"
    "        '10:-h') r=10 ;;\n"
    "        '10:--help') r=10 ;;\n"
    "        '11:-h') r=11 ;;\n"
    "        '11:--help') r=11 ;;\n"
    "        '12:-h') r=12 ;;\n"
    "        '12:--help') r=12 ;;\n"
    "        '13:-h') r=13 ;;\n"
    "        '13:--help') r=13 ;;\n"
    "        '14:-h') r=14 ;;\n"
    "        '14:--help') r=14 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=0 x=0 k=- ;;\n"
    "        2) n=0 x=0 k=- ;;\n"
    "        3) n=0 x=0 k=- ;;\n"
    "        4) n=0 x=0 k=- ;;\n"
    "        5) n=0 x=0 k=- ;;\n"
    "        6) n=0 x=0 k=- ;;\n"
    "        7) n=0 x=0 k=- ;;\n"
    "        8) n=0 x=0 k=- ;;\n"
    "        9) n=0 x=0 k=- ;;\n"
    "        10) n=0 x=0 k=- ;;\n"
    "        11) n=0 x=0 k=- ;;\n"
    "        12) n=0 x=0 k=- ;;\n"
    "        13) n=0 x=0 k=- ;;\n"
    "        14) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        0) c=( 'fall' 'nosub' 'win' 'capture' 'ignore' 'single' "
    "'multiple' 'immediately' 'prefix' 'separator' 'positional' 'two' 'none' "
    "'every' ) d=( 'Fallthrough' 'No subcommand fallthrough' 'Windows style' "
    "'Extras captured' 'Extras ignored' 'Extras as one' 'Extras as many' "
    "'Extras an error at once' 'A prefix command' 'A prefix at the separator' "
    "'A prefix at a positional' 'At most two subcommands' 'Nothing at all' "
    "'Every switch' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        1) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        2) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        3) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        4) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        5) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        6) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        7) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        8) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        9) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        10) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        11) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        12) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        13) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n";

// TREE_SWITCHES_3
//   constant: the transcript of d_tests_arg_completion_tree_switches,
// continued.
static const char TREE_SWITCHES_3[] =
    "        14) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== each fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 14 1 2' \\\n"
    "    '0 sf 0 15 0 3 2' \\\n"
    "    '0 - 0 15 0 5 2' \\\n"
    "    '0 sw 0 15 0 7 2' \\\n"
    "    '0 se 0 15 0 9 2' \\\n"
    "    '0 se 0 15 0 11 2' \\\n"
    "    '0 se 0 15 0 13 2' \\\n"
    "    '0 se 0 15 0 15 2' \\\n"
    "    '0 s 0 15 0 17 2' \\\n"
    "    '0 se 0 15 0 19 2' \\\n"
    "    '0 se 0 15 0 21 2' \\\n"
    "    '0 se 0 15 0 23 2' \\\n"
    "    '0 s 2 15 0 25 2' \\\n"
    "    '0 - 0 15 0 27 2' \\\n"
    "    '0 sfwe 7 15 0 29 2'\n"
    "set -g -- __djarg_prog_sk \\\n"
    "    '0:fall' \\\n"
    "    '0:nosub' \\\n"
    "    '0:win' \\\n"
    "    '0:capture' \\\n"
    "    '0:ignore' \\\n"
    "    '0:single' \\\n"
    "    '0:multiple' \\\n"
    "    '0:immediately' \\\n"
    "    '0:prefix' \\\n"
    "    '0:separator' \\\n"
    "    '0:positional' \\\n"
    "    '0:two' \\\n"
    "    '0:none' \\\n"
    "    '0:every'\n"
    "set -g -- __djarg_prog_sn \\\n"
    "    'fall' \\\n"
    "    'nosub' \\\n"
    "    'win' \\\n"
    "    'capture' \\\n"
    "    'ignore' \\\n"
    "    'single' \\\n"
    "    'multiple' \\\n"
    "    'immediately' \\\n"
    "    'prefix' \\\n"
    "    'separator' \\\n"
    "    'positional' \\\n"
    "    'two' \\\n"
    "    'none' \\\n"
    "    'every'\n"
    "set -g -- __djarg_prog_st \\\n"
    "    1 \\\n"
    "    2 \\\n"
    "    3 \\\n"
    "    4 \\\n"
    "    5 \\\n"
    "    6 \\\n"
    "    7 \\\n"
    "    8 \\\n"
    "    9 \\\n"
    "    10 \\\n"
    "    11 \\\n"
    "    12 \\\n"
    "    13 \\\n"
    "    14\n"
    "set -g -- __djarg_prog_sd \\\n"
    "    'Fallthrough' \\\n"
    "    'No subcommand fallthrough' \\\n"
    "    'Windows style' \\\n"
    "    'Extras captured' \\\n"
    "    'Extras ignored' \\\n"
    "    'Extras as one' \\\n"
    "    'Extras as many' \\\n"
    "    'Extras an error at once' \\\n"
    "    'A prefix command' \\\n"
    "    'A prefix at the separator' \\\n"
    "    'A prefix at a positional' \\\n"
    "    'At most two subcommands' \\\n"
    "    'Nothing at all' \\\n"
    "    'Every switch'\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '1:-h' \\\n"
    "    '1:--help' \\\n"
    "    '2:-h' \\\n"
    "    '2:--help' \\\n"
    "    '3:-h' \\\n"
    "    '3:--help' \\\n"
    "    '4:-h' \\\n"
    "    '4:--help' \\\n"
    "    '5:-h' \\\n"
    "    '5:--help' \\\n"
    "    '6:-h' \\\n"
    "    '6:--help' \\\n"
    "    '7:-h' \\\n"
    "    '7:--help' \\\n"
    "    '8:-h' \\\n"
    "    '8:--help' \\\n"
    "    '9:-h' \\\n"
    "    '9:--help' \\\n"
    "    '10:-h' \\\n"
    "    '10:--help' \\\n"
    "    '11:-h' \\\n"
    "    '11:--help' \\\n"
    "    '12:-h' \\\n"
    "    '12:--help' \\\n"
    "    '13:-h' \\\n"
    "    '13:--help' \\\n"
    "    '14:-h' \\\n"
    "    '14:--help'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    1 \\\n"
    "    1 \\\n"
    "    2 \\\n"
    "    2 \\\n"
    "    3 \\\n"
    "    3 \\\n"
    "    4 \\\n"
    "    4 \\\n"
    "    5 \\\n"
    "    5 \\\n"
    "    6 \\\n"
    "    6 \\\n"
    "    7 \\\n"
    "    7 \\\n"
    "    8 \\\n"
    "    8 \\\n"
    "    9 \\\n"
    "    9 \\\n"
    "    10 \\\n"
    "    10 \\\n"
    "    11 \\\n"
    "    11 \\\n"
    "    12 \\\n"
    "    12 \\\n"
    "    13 \\\n"
    "    13 \\\n"
    "    14 \\\n"
    "    14\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n";

// TREE_SWITCHES_4
//   constant: the transcript of d_tests_arg_completion_tree_switches,
// continued.
static const char TREE_SWITCHES_4[] =
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0'\n"
    "set -g -- __djarg_prog_pk\n"
    "set -g -- __djarg_prog_pv\n"
    "set -g -- __djarg_prog_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== each powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 14 0 2'\n"
    "        '0 sf 0 14 0 2 2'\n"
    "        '0 - 0 14 0 4 2'\n"
    "        '0 sw 0 14 0 6 2'\n"
    "        '0 se 0 14 0 8 2'\n"
    "        '0 se 0 14 0 10 2'\n"
    "        '0 se 0 14 0 12 2'\n"
    "        '0 se 0 14 0 14 2'\n"
    "        '0 s 0 14 0 16 2'\n"
    "        '0 se 0 14 0 18 2'\n"
    "        '0 se 0 14 0 20 2'\n"
    "        '0 se 0 14 0 22 2'\n"
    "        '0 s 2 14 0 24 2'\n"
    "        '0 - 0 14 0 26 2'\n"
    "        '0 sfwe 7 14 0 28 2'\n"
    "    )\n"
    "    $sn = @(\n"
    "        'fall'\n"
    "        'nosub'\n"
    "        'win'\n"
    "        'capture'\n"
    "        'ignore'\n"
    "        'single'\n"
    "        'multiple'\n"
    "        'immediately'\n"
    "        'prefix'\n"
    "        'separator'\n"
    "        'positional'\n"
    "        'two'\n"
    "        'none'\n"
    "        'every'\n"
    "    )\n"
    "    $sc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $st = @(\n"
    "        1\n"
    "        2\n"
    "        3\n"
    "        4\n"
    "        5\n"
    "        6\n"
    "        7\n"
    "        8\n"
    "        9\n"
    "        10\n"
    "        11\n"
    "        12\n"
    "        13\n"
    "        14\n"
    "    )\n"
    "    $sd = @(\n"
    "        'Fallthrough'\n"
    "        'No subcommand fallthrough'\n"
    "        'Windows style'\n"
    "        'Extras captured'\n"
    "        'Extras ignored'\n"
    "        'Extras as one'\n"
    "        'Extras as many'\n"
    "        'Extras an error at once'\n"
    "        'A prefix command'\n"
    "        'A prefix at the separator'\n"
    "        'A prefix at a positional'\n"
    "        'At most two subcommands'\n"
    "        'Nothing at all'\n"
    "        'Every switch'\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        1\n"
    "        2\n"
    "        2\n"
    "        3\n"
    "        3\n"
    "        4\n"
    "        4\n"
    "        5\n"
    "        5\n"
    "        6\n"
    "        6\n"
    "        7\n"
    "        7\n"
    "        8\n"
    "        8\n"
    "        9\n"
    "        9\n"
    "        10\n"
    "        10\n"
    "        11\n"
    "        11\n"
    "        12\n"
    "        12\n"
    "        13\n"
    "        13\n"
    "        14\n"
    "        14\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        1\n"
    "        2\n"
    "        2\n"
    "        3\n"
    "        3\n"
    "        4\n"
    "        4\n"
    "        5\n"
    "        5\n"
    "        6\n"
    "        6\n"
    "        7\n"
    "        7\n"
    "        8\n"
    "        8\n"
    "        9\n"
    "        9\n"
    "        10\n"
    "        10\n"
    "        11\n"
    "        11\n"
    "        12\n"
    "        12\n"
    "        13\n"
    "        13\n"
    "        14\n"
    "        14\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n";

// TREE_SWITCHES_5
//   constant: the transcript of d_tests_arg_completion_tree_switches,
// continued.
static const char TREE_SWITCHES_5[] =
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n"
    "== inherited bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=fwe m=3 ;;\n"
    "        1) p=0 f=sfwe m=3 ;;\n"
    "        2) p=0 f=swe m=3 ;;\n"
    "        3) p=0 f=sfwe m=3 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        '0:x') r=1 ;;\n"
    "        '0:y') r=2 ;;\n"
    "        '0:z') r=3 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '1:-h') r=1 ;;\n"
    "        '1:--help') r=1 ;;\n"
    "        '3:-h') r=2 ;;\n"
    "        '3:--help') r=2 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=0 x=0 k=- ;;\n"
    "        2) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        0) c=( 'x' 'y' 'z' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' ) ;;\n"
    "        1) c=( '-h' '--help' ) ;;\n"
    "        3) c=( '-h' '--help' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== inherited zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=fwe m=3 ;;\n"
    "        1) p=0 f=sfwe m=3 ;;\n"
    "        2) p=0 f=swe m=3 ;;\n"
    "        3) p=0 f=sfwe m=3 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        '0:x') r=1 ;;\n"
    "        '0:y') r=2 ;;\n"
    "        '0:z') r=3 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '1:-h') r=1 ;;\n"
    "        '1:--help') r=1 ;;\n"
    "        '3:-h') r=2 ;;\n"
    "        '3:--help') r=2 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=0 x=0 k=- ;;\n"
    "        2) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        0) c=( 'x' 'y' 'z' ) d=( 'Inherits' 'Under the group' 'Inherits "
    "the prefix' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n";

// TREE_SWITCHES_6
//   constant: the transcript of d_tests_arg_completion_tree_switches,
// continued.
static const char TREE_SWITCHES_6[] =
    "        0) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        1) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        3) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== inherited fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 fwe 3 1 3 1 2' \\\n"
    "    '0 sfwe 3 4 0 3 2' \\\n"
    "    '0 swe 3 4 0 5 0' \\\n"
    "    '0 sfwe 3 4 0 5 2'\n"
    "set -g -- __djarg_prog_sk \\\n"
    "    '0:x' \\\n"
    "    '0:y' \\\n"
    "    '0:z'\n"
    "set -g -- __djarg_prog_sn \\\n"
    "    'x' \\\n"
    "    'y' \\\n"
    "    'z'\n"
    "set -g -- __djarg_prog_st \\\n"
    "    1 \\\n"
    "    2 \\\n"
    "    3\n"
    "set -g -- __djarg_prog_sd \\\n"
    "    'Inherits' \\\n"
    "    'Under the group' \\\n"
    "    'Inherits the prefix'\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '1:-h' \\\n"
    "    '1:--help' \\\n"
    "    '3:-h' \\\n"
    "    '3:--help'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    1 \\\n"
    "    1 \\\n"
    "    2 \\\n"
    "    2\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0'\n"
    "set -g -- __djarg_prog_pk\n"
    "set -g -- __djarg_prog_pv\n"
    "set -g -- __djarg_prog_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== inherited powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 fwe 3 0 3 0 2'\n"
    "        '0 sfwe 3 3 0 2 2'\n"
    "        '0 swe 3 3 0 4 0'\n"
    "        '0 sfwe 3 3 0 4 2'\n"
    "    )\n"
    "    $sn = @(\n"
    "        'x'\n"
    "        'y'\n"
    "        'z'\n"
    "    )\n"
    "    $sc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $st = @(\n"
    "        1\n"
    "        2\n"
    "        3\n"
    "    )\n"
    "    $sd = @(\n"
    "        'Inherits'\n"
    "        'Under the group'\n"
    "        'Inherits the prefix'\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        1\n"
    "        3\n"
    "        3\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        1\n"
    "        2\n"
    "        2\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n";

// TREE_POSITIONALS
//   constant: what d_tests_arg_completion_tree_positionals writes, as
// completion_ref.py works it out from the rules arg_completion.h states, in
// parts.
static const char TREE_POSITIONALS[] =
    "== first of all bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-v') r=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=1 x=1 k=c ;;\n"
    "        1) n=0 x=0 k=- ;;\n"
    "        2) n=1 x=1 k=f ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        0:0) r=0 y=1 ;;\n"
    "        0:1) r=2 y=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-v' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        0) c=( 'one' 'two' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== first of all zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-v') r=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=1 x=1 k=c ;;\n"
    "        1) n=0 x=0 k=- ;;\n"
    "        2) n=1 x=1 k=f ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        0:0) r=0 y=1 ;;\n"
    "        0:1) r=2 y=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-v' ) d=( '' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        0) c=( 'one' 'two' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== first of all fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 0 1 1'\n"
    "set -g -- __djarg_prog_sk\n"
    "set -g -- __djarg_prog_sn\n"
    "set -g -- __djarg_prog_st\n"
    "set -g -- __djarg_prog_sd\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-v'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-v'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    1\n"
    "set -g -- __djarg_prog_od \\\n"
    "    ''\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '1 1 c 1 2' \\\n"
    "    '0 0 - 3 0' \\\n"
    "    '1 1 f 3 0'\n"
    "set -g -- __djarg_prog_pk \\\n"
    "    '0:0' \\\n"
    "    '0:1'\n"
    "set -g -- __djarg_prog_pv \\\n"
    "    '0 1' \\\n"
    "    '2 1'\n"
    "set -g -- __djarg_prog_vv \\\n"
    "    'one' \\\n"
    "    'two'\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== first of all powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 1'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "        '-v'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "    )\n"
    "    $ov = @(\n"
    "        1\n"
    "    )\n"
    "    $od = @(\n"
    "        ''\n"
    "    )\n"
    "    $info = @(\n"
    "        '1 1 c 0 2'\n"
    "        '0 0 - 2 0'\n"
    "        '1 1 f 2 0'\n"
    "    )\n"
    "    $ps = @(\n"
    "        '0:0'\n"
    "        '0:1'\n"
    "    )\n"
    "    $pv = @(\n"
    "        '0 1'\n"
    "        '2 1'\n"
    "    )\n"
    "    $vv = @(\n"
    "        'one'\n"
    "        'two'\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n"
    "== slots bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        1) p=0 f=s m=0 ;;\n";

// TREE_POSITIONALS_2
//   constant: the transcript of d_tests_arg_completion_tree_positionals,
// continued.
static const char TREE_POSITIONALS_2[] =
    "        2) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        '0:run') r=1 ;;\n"
    "        '0:none') r=2 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:-v') r=3 ;;\n"
    "        '1:-h') r=5 ;;\n"
    "        '1:--help') r=5 ;;\n"
    "        '2:-h') r=9 ;;\n"
    "        '2:--help') r=9 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=f ;;\n"
    "        2) n=1 x=2 k=f ;;\n"
    "        3) n=0 x=0 k=- ;;\n"
    "        4) n=1 x=-1 k=f ;;\n"
    "        5) n=0 x=0 k=- ;;\n"
    "        6) n=1 x=1 k=f ;;\n"
    "        7) n=2 x=2 k=f ;;\n"
    "        8) n=1 x=1 k=f ;;\n"
    "        9) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        0:0) r=1 y=1 ;;\n"
    "        0:1) r=2 y=2 ;;\n"
    "        0:2) r=4 y=-1 ;;\n"
    "        1:0) r=6 y=1 ;;\n"
    "        1:1) r=7 y=2 ;;\n"
    "        1:2) r=8 y=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        0) c=( 'run' 'none' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '-v' ) ;;\n"
    "        1) c=( '-h' '--help' ) ;;\n"
    "        2) c=( '-h' '--help' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== slots zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        1) p=0 f=s m=0 ;;\n"
    "        2) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        '0:run') r=1 ;;\n"
    "        '0:none') r=2 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:-v') r=3 ;;\n"
    "        '1:-h') r=5 ;;\n"
    "        '1:--help') r=5 ;;\n"
    "        '2:-h') r=9 ;;\n"
    "        '2:--help') r=9 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=f ;;\n"
    "        2) n=1 x=2 k=f ;;\n"
    "        3) n=0 x=0 k=- ;;\n"
    "        4) n=1 x=-1 k=f ;;\n"
    "        5) n=0 x=0 k=- ;;\n"
    "        6) n=1 x=1 k=f ;;\n"
    "        7) n=2 x=2 k=f ;;\n"
    "        8) n=1 x=1 k=f ;;\n"
    "        9) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        0:0) r=1 y=1 ;;\n"
    "        0:1) r=2 y=2 ;;\n"
    "        0:2) r=4 y=-1 ;;\n"
    "        1:0) r=6 y=1 ;;\n"
    "        1:1) r=7 y=2 ;;\n"
    "        1:2) r=8 y=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        0) c=( 'run' 'none' ) d=( 'Run' 'No positionals' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '-v' ) d=( 'Print this help message and "
    "exit' 'Print this help message and exit' '' ) ;;\n"
    "        1) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        2) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== slots fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 2 1 3' \\\n"
    "    '0 s 0 3 0 4 2' \\\n"
    "    '0 s 0 3 0 6 2'\n"
    "set -g -- __djarg_prog_sk \\\n"
    "    '0:run' \\\n"
    "    '0:none'\n"
    "set -g -- __djarg_prog_sn \\\n"
    "    'run' \\\n"
    "    'none'\n"
    "set -g -- __djarg_prog_st \\\n"
    "    1 \\\n"
    "    2\n"
    "set -g -- __djarg_prog_sd \\\n"
    "    'Run' \\\n"
    "    'No positionals'\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '0:-v' \\\n";

// TREE_POSITIONALS_3
//   constant: the transcript of d_tests_arg_completion_tree_positionals,
// continued.
static const char TREE_POSITIONALS_3[] =
    "    '1:-h' \\\n"
    "    '1:--help' \\\n"
    "    '2:-h' \\\n"
    "    '2:--help'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-v' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    3 \\\n"
    "    5 \\\n"
    "    5 \\\n"
    "    9 \\\n"
    "    9\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    '' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '1 1 f 1 0' \\\n"
    "    '1 2 f 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '1 -1 f 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '1 1 f 1 0' \\\n"
    "    '2 2 f 1 0' \\\n"
    "    '1 1 f 1 0' \\\n"
    "    '0 0 - 1 0'\n"
    "set -g -- __djarg_prog_pk \\\n"
    "    '0:0' \\\n"
    "    '0:1' \\\n"
    "    '0:2' \\\n"
    "    '1:0' \\\n"
    "    '1:1' \\\n"
    "    '1:2'\n"
    "set -g -- __djarg_prog_pv \\\n"
    "    '1 1' \\\n"
    "    '2 2' \\\n"
    "    '4 -1' \\\n"
    "    '6 1' \\\n"
    "    '7 2' \\\n"
    "    '8 1'\n"
    "set -g -- __djarg_prog_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== slots powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 2 0 3'\n"
    "        '0 s 0 2 0 3 2'\n"
    "        '0 s 0 2 0 5 2'\n"
    "    )\n"
    "    $sn = @(\n"
    "        'run'\n"
    "        'none'\n"
    "    )\n"
    "    $sc = @(\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $st = @(\n"
    "        1\n"
    "        2\n"
    "    )\n"
    "    $sd = @(\n"
    "        'Run'\n"
    "        'No positionals'\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-v'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        1\n"
    "        2\n"
    "        2\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        3\n"
    "        5\n"
    "        5\n"
    "        9\n"
    "        9\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        ''\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '1 1 f 0 0'\n"
    "        '1 2 f 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '1 -1 f 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '1 1 f 0 0'\n"
    "        '2 2 f 0 0'\n"
    "        '1 1 f 0 0'\n"
    "        '0 0 - 0 0'\n"
    "    )\n"
    "    $ps = @(\n"
    "        '0:0'\n"
    "        '0:1'\n"
    "        '0:2'\n"
    "        '1:0'\n"
    "        '1:1'\n"
    "        '1:2'\n"
    "    )\n"
    "    $pv = @(\n"
    "        '1 1'\n"
    "        '2 2'\n"
    "        '4 -1'\n"
    "        '6 1'\n"
    "        '7 2'\n"
    "        '8 1'\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n"
    "== hidden bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=f ;;\n"
    "        2) n=1 x=1 k=n ;;\n"
    "        3) n=1 x=1 k=c ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        0:0) r=1 y=1 ;;\n"
    "        0:1) r=2 y=1 ;;\n"
    "        0:2) r=3 y=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        3) c=( 'x' 'y' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== hidden zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n";

// TREE_POSITIONALS_4
//   constant: the transcript of d_tests_arg_completion_tree_positionals,
// continued.
static const char TREE_POSITIONALS_4[] =
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=f ;;\n"
    "        2) n=1 x=1 k=n ;;\n"
    "        3) n=1 x=1 k=c ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        0:0) r=1 y=1 ;;\n"
    "        0:1) r=2 y=1 ;;\n"
    "        0:2) r=3 y=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        3) c=( 'x' 'y' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== hidden fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 0 1 2'\n"
    "set -g -- __djarg_prog_sk\n"
    "set -g -- __djarg_prog_sn\n"
    "set -g -- __djarg_prog_st\n"
    "set -g -- __djarg_prog_sd\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '1 1 f 1 0' \\\n"
    "    '1 1 n 1 0' \\\n"
    "    '1 1 c 1 2'\n"
    "set -g -- __djarg_prog_pk \\\n"
    "    '0:0' \\\n"
    "    '0:1' \\\n"
    "    '0:2'\n"
    "set -g -- __djarg_prog_pv \\\n"
    "    '1 1' \\\n"
    "    '2 1' \\\n"
    "    '3 1'\n"
    "set -g -- __djarg_prog_vv \\\n"
    "    'x' \\\n"
    "    'y'\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== hidden powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 2'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '1 1 f 0 0'\n"
    "        '1 1 n 0 0'\n"
    "        '1 1 c 0 2'\n"
    "    )\n"
    "    $ps = @(\n"
    "        '0:0'\n"
    "        '0:1'\n"
    "        '0:2'\n"
    "    )\n"
    "    $pv = @(\n"
    "        '1 1'\n"
    "        '2 1'\n"
    "        '3 1'\n"
    "    )\n"
    "    $vv = @(\n"
    "        'x'\n"
    "        'y'\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n"
    "== extra bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=-1 k=f ;;\n"
    "        2) n=1 x=1 k=f ;;\n"
    "        3) n=1 x=1 k=f ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        0:0) r=1 y=-1 ;;\n"
    "        0:1) r=2 y=536870911 ;;\n"
    "        0:2) r=3 y=-1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== extra zsh OK\n"
    "#compdef prog\n";

// TREE_POSITIONALS_5
//   constant: the transcript of d_tests_arg_completion_tree_positionals,
// continued.
static const char TREE_POSITIONALS_5[] =
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=-1 k=f ;;\n"
    "        2) n=1 x=1 k=f ;;\n"
    "        3) n=1 x=1 k=f ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        0:0) r=1 y=-1 ;;\n"
    "        0:1) r=2 y=536870911 ;;\n"
    "        0:2) r=3 y=-1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== extra fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 0 1 2'\n"
    "set -g -- __djarg_prog_sk\n"
    "set -g -- __djarg_prog_sn\n"
    "set -g -- __djarg_prog_st\n"
    "set -g -- __djarg_prog_sd\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '1 -1 f 1 0' \\\n"
    "    '1 1 f 1 0' \\\n"
    "    '1 1 f 1 0'\n"
    "set -g -- __djarg_prog_pk \\\n"
    "    '0:0' \\\n"
    "    '0:1' \\\n"
    "    '0:2'\n"
    "set -g -- __djarg_prog_pv \\\n"
    "    '1 -1' \\\n"
    "    '2 536870911' \\\n"
    "    '3 -1'\n"
    "set -g -- __djarg_prog_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== extra powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 2'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '1 -1 f 0 0'\n"
    "        '1 1 f 0 0'\n"
    "        '1 1 f 0 0'\n"
    "    )\n"
    "    $ps = @(\n"
    "        '0:0'\n"
    "        '0:1'\n"
    "        '0:2'\n"
    "    )\n"
    "    $pv = @(\n"
    "        '1 -1'\n"
    "        '2 536870911'\n"
    "        '3 -1'\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n";

// TREE_QUOTING
//   constant: what d_tests_arg_completion_tree_quoting writes, as
// completion_ref.py works it out from the rules arg_completion.h states, in
// parts.
static const char TREE_QUOTING[] =
    "== choices bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--pick') r=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=c ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--pick' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        1) c=( 'it'\\''s' 'back\\slash' 'dollar$x' 'tick`x' 'quote\"d' "
    "'two words' 'tab\x09here' 'line\n"
    "break' 'cr\x0dhere' 'del\x7fhere' 'caf\xc3\xa9' '\xe2\x86\x92' "
    "'\xf0\x9f\x98\x80' 'semi;colon' 'star*' '-dash' '' '\x01' ''\\'''\\''' "
    "'\\' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== choices zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--pick') r=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=c ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--pick' ) d=( 'Print this help message and "
    "exit' 'Print this help message and exit' 'Text to quote' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        1) c=( 'it'\\''s' 'back\\slash' 'dollar$x' 'tick`x' 'quote\"d' "
    "'two words' 'tab\x09here' 'line\n"
    "break' 'cr\x0dhere' 'del\x7fhere' 'caf\xc3\xa9' '\xe2\x86\x92' "
    "'\xf0\x9f\x98\x80' 'semi;colon' 'star*' '-dash' '' '\x01' ''\\'''\\''' "
    "'\\' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== choices fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 0 1 3'\n"
    "set -g -- __djarg_prog_sk\n"
    "set -g -- __djarg_prog_sn\n"
    "set -g -- __djarg_prog_st\n"
    "set -g -- __djarg_prog_sd\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '0:--pick'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '--pick'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    1\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Text to quote'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '1 1 c 1 20'\n"
    "set -g -- __djarg_prog_pk\n"
    "set -g -- __djarg_prog_pv\n"
    "set -g -- __djarg_prog_vv \\\n"
    "    'it\\'s' \\\n"
    "    'back\\\\slash' \\\n"
    "    'dollar$x' \\\n"
    "    'tick`x' \\\n"
    "    'quote\"d' \\\n"
    "    'two words' \\\n"
    "    'tab\x09here' \\\n"
    "    'line\n"
    "break' \\\n"
    "    'cr\x0dhere' \\\n"
    "    'del\x7fhere' \\\n"
    "    'caf\xc3\xa9' \\\n"
    "    '\xe2\x86\x92' \\\n"
    "    '\xf0\x9f\x98\x80' \\\n"
    "    'semi;colon' \\\n"
    "    'star*' \\\n"
    "    '-dash' \\\n"
    "    '' \\\n"
    "    '\x01' \\\n"
    "    '\\'\\'' \\\n"
    "    '\\\\'\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== choices powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n";

// TREE_QUOTING_2
//   constant: the transcript of d_tests_arg_completion_tree_quoting, continued.
static const char TREE_QUOTING_2[] =
    "        '0 s 0 0 0 0 3'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '--pick'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Text to quote'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '1 1 c 0 20'\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "        'it''s'\n"
    "        'back\\slash'\n"
    "        'dollar$x'\n"
    "        'tick`x'\n"
    "        'quote\"d'\n"
    "        'two words'\n"
    "        ('' + 'tab' + [char]0x0009 + 'here')\n"
    "        ('' + 'line' + [char]0x000A + 'break')\n"
    "        ('' + 'cr' + [char]0x000D + 'here')\n"
    "        ('' + 'del' + [char]0x007F + 'here')\n"
    "        ('' + 'caf' + [char]0x00E9)\n"
    "        ('' + [char]0x2192)\n"
    "        ('' + [char]0xD83D + [char]0xDE00)\n"
    "        'semi;colon'\n"
    "        'star*'\n"
    "        '-dash'\n"
    "        ''\n"
    "        ('' + [char]0x0001)\n"
    "        ''''''\n"
    "        '\\'\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n"
    "== bytes bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--pick') r=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=c ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--pick' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        1) c=( '\xff' '\xc3' '\xc3(' '\xe2\x82' '\xe2\x82(' '\xc0\x80' "
    "'\xed\xa0\x80' '\xf4\x90\x80\x80' '\xf0\x8f\xbf\xbf' '\xe0\x9f\xbf' "
    "'\xf5\x80' '\xf5\x80\x80\x80' '\xf4\x80\x80\x80' '\xef\xbf\xbf' "
    "'\xf4\x8f\xbf\xbf' '\xc2\x80' '\xdf\xbf' '\xe0\xa0\x80' "
    "'\xf0\x90\x80\x80' 'a\x80" "b' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== bytes zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--pick') r=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=c ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--pick' ) d=( 'Print this help message and "
    "exit' 'Print this help message and exit' 'Bytes past ASCII' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        1) c=( '\xff' '\xc3' '\xc3(' '\xe2\x82' '\xe2\x82(' '\xc0\x80' "
    "'\xed\xa0\x80' '\xf4\x90\x80\x80' '\xf0\x8f\xbf\xbf' '\xe0\x9f\xbf' "
    "'\xf5\x80' '\xf5\x80\x80\x80' '\xf4\x80\x80\x80' '\xef\xbf\xbf' "
    "'\xf4\x8f\xbf\xbf' '\xc2\x80' '\xdf\xbf' '\xe0\xa0\x80' "
    "'\xf0\x90\x80\x80' 'a\x80" "b' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== bytes fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 0 1 3'\n"
    "set -g -- __djarg_prog_sk\n"
    "set -g -- __djarg_prog_sn\n"
    "set -g -- __djarg_prog_st\n"
    "set -g -- __djarg_prog_sd\n";

// TREE_QUOTING_3
//   constant: the transcript of d_tests_arg_completion_tree_quoting, continued.
static const char TREE_QUOTING_3[] =
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '0:--pick'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '--pick'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    1\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Bytes past ASCII'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '1 1 c 1 20'\n"
    "set -g -- __djarg_prog_pk\n"
    "set -g -- __djarg_prog_pv\n"
    "set -g -- __djarg_prog_vv \\\n"
    "    '\xff' \\\n"
    "    '\xc3' \\\n"
    "    '\xc3(' \\\n"
    "    '\xe2\x82' \\\n"
    "    '\xe2\x82(' \\\n"
    "    '\xc0\x80' \\\n"
    "    '\xed\xa0\x80' \\\n"
    "    '\xf4\x90\x80\x80' \\\n"
    "    '\xf0\x8f\xbf\xbf' \\\n"
    "    '\xe0\x9f\xbf' \\\n"
    "    '\xf5\x80' \\\n"
    "    '\xf5\x80\x80\x80' \\\n"
    "    '\xf4\x80\x80\x80' \\\n"
    "    '\xef\xbf\xbf' \\\n"
    "    '\xf4\x8f\xbf\xbf' \\\n"
    "    '\xc2\x80' \\\n"
    "    '\xdf\xbf' \\\n"
    "    '\xe0\xa0\x80' \\\n"
    "    '\xf0\x90\x80\x80' \\\n"
    "    'a\x80" "b'\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== bytes powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 3'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '--pick'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Bytes past ASCII'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '1 1 c 0 20'\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "        ('' + [char]0xFFFD)\n"
    "        ('' + [char]0xFFFD)\n"
    "        ('' + [char]0xFFFD + '(')\n"
    "        ('' + [char]0xFFFD + [char]0xFFFD)\n"
    "        ('' + [char]0xFFFD + [char]0xFFFD + '(')\n"
    "        ('' + [char]0xFFFD + [char]0xFFFD)\n"
    "        ('' + [char]0xFFFD + [char]0xFFFD + [char]0xFFFD)\n"
    "        ('' + [char]0xFFFD + [char]0xFFFD + [char]0xFFFD + [char]0xFFFD)\n"
    "        ('' + [char]0xFFFD + [char]0xFFFD + [char]0xFFFD + [char]0xFFFD)\n"
    "        ('' + [char]0xFFFD + [char]0xFFFD + [char]0xFFFD)\n"
    "        ('' + [char]0xFFFD + [char]0xFFFD)\n"
    "        ('' + [char]0xFFFD + [char]0xFFFD + [char]0xFFFD + [char]0xFFFD)\n"
    "        ('' + [char]0xDBC0 + [char]0xDC00)\n"
    "        ('' + [char]0xFFFF)\n"
    "        ('' + [char]0xDBFF + [char]0xDFFF)\n"
    "        ('' + [char]0x0080)\n"
    "        ('' + [char]0x07FF)\n"
    "        ('' + [char]0x0800)\n"
    "        ('' + [char]0xD800 + [char]0xDC00)\n"
    "        ('' + 'a' + [char]0xFFFD + 'b')\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n"
    "== descriptions bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        1) p=0 f=s m=0 ;;\n"
    "        2) p=0 f=s m=0 ;;\n"
    "        3) p=0 f=s m=0 ;;\n"
    "        4) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        '0:it'\\''s') r=1 ;;\n"
    "        '0:caf\xc3\xa9') r=2 ;;\n"
    "        '0:k\xc3\xb8') r=2 ;;\n"
    "        '0:a\x09" "b') r=3 ;;\n"
    "        '0:x\x09y') r=3 ;;\n"
    "        '0:del\x7f') r=4 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--a') r=1 ;;\n"
    "        '0:--b') r=2 ;;\n"
    "        '0:--c') r=3 ;;\n"
    "        '0:--d') r=4 ;;\n"
    "        '0:--e') r=5 ;;\n"
    "        '0:--f') r=6 ;;\n"
    "        '1:-h') r=7 ;;\n"
    "        '1:--help') r=7 ;;\n"
    "        '2:-h') r=8 ;;\n"
    "        '2:--help') r=8 ;;\n"
    "        '3:-h') r=9 ;;\n"
    "        '3:--help') r=9 ;;\n"
    "        '4:-h') r=10 ;;\n"
    "        '4:--help') r=10 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=0 x=0 k=- ;;\n"
    "        2) n=0 x=0 k=- ;;\n"
    "        3) n=0 x=0 k=- ;;\n"
    "        4) n=0 x=0 k=- ;;\n"
    "        5) n=0 x=0 k=- ;;\n"
    "        6) n=0 x=0 k=- ;;\n"
    "        7) n=0 x=0 k=- ;;\n"
    "        8) n=0 x=0 k=- ;;\n"
    "        9) n=0 x=0 k=- ;;\n"
    "        10) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n";

// TREE_QUOTING_4
//   constant: the transcript of d_tests_arg_completion_tree_quoting, continued.
static const char TREE_QUOTING_4[] =
    "        0) c=( 'it'\\''s' 'caf\xc3\xa9' 'k\xc3\xb8' 'a\x09" "b' 'x\x09y' "
    "'del\x7f' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--a' '--b' '--c' '--d' '--e' '--f' ) ;;\n"
    "        1) c=( '-h' '--help' ) ;;\n"
    "        2) c=( '-h' '--help' ) ;;\n"
    "        3) c=( '-h' '--help' ) ;;\n"
    "        4) c=( '-h' '--help' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== descriptions zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        1) p=0 f=s m=0 ;;\n"
    "        2) p=0 f=s m=0 ;;\n"
    "        3) p=0 f=s m=0 ;;\n"
    "        4) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        '0:it'\\''s') r=1 ;;\n"
    "        '0:caf\xc3\xa9') r=2 ;;\n"
    "        '0:k\xc3\xb8') r=2 ;;\n"
    "        '0:a\x09" "b') r=3 ;;\n"
    "        '0:x\x09y') r=3 ;;\n"
    "        '0:del\x7f') r=4 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--a') r=1 ;;\n"
    "        '0:--b') r=2 ;;\n"
    "        '0:--c') r=3 ;;\n"
    "        '0:--d') r=4 ;;\n"
    "        '0:--e') r=5 ;;\n"
    "        '0:--f') r=6 ;;\n"
    "        '1:-h') r=7 ;;\n"
    "        '1:--help') r=7 ;;\n"
    "        '2:-h') r=8 ;;\n"
    "        '2:--help') r=8 ;;\n"
    "        '3:-h') r=9 ;;\n"
    "        '3:--help') r=9 ;;\n"
    "        '4:-h') r=10 ;;\n"
    "        '4:--help') r=10 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=0 x=0 k=- ;;\n"
    "        2) n=0 x=0 k=- ;;\n"
    "        3) n=0 x=0 k=- ;;\n"
    "        4) n=0 x=0 k=- ;;\n"
    "        5) n=0 x=0 k=- ;;\n"
    "        6) n=0 x=0 k=- ;;\n"
    "        7) n=0 x=0 k=- ;;\n"
    "        8) n=0 x=0 k=- ;;\n"
    "        9) n=0 x=0 k=- ;;\n"
    "        10) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        0) c=( 'it'\\''s' 'caf\xc3\xa9' 'k\xc3\xb8' 'a\x09" "b' 'x\x09y' "
    "'del\x7f' ) d=( 'Quote'\\''s here' 'Caf\xc3\xa9 \xe2\x86\x92' "
    "'Caf\xc3\xa9 \xe2\x86\x92' 'A tab in a name' 'A tab in a name' 'A delete "
    "in a name' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--a' '--b' '--c' '--d' '--e' '--f' ) d=( "
    "'Print this help message and exit' 'Print this help message and exit' "
    "'it'\\''s' 'tab here' 'bell , del , escape ' 'caf\xc3\xa9 \xe2\x86\x92 "
    "\xf0\x9f\x98\x80' 'back\\slash \"quoted\" $x `y`' 'stray \xff byte' ) ;;\n"
    "        1) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        2) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        3) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        4) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== descriptions fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 6 1 8' \\\n"
    "    '0 s 0 7 0 9 2' \\\n"
    "    '0 s 0 7 0 11 2' \\\n"
    "    '0 s 0 7 0 13 2' \\\n"
    "    '0 s 0 7 0 15 2'\n"
    "set -g -- __djarg_prog_sk \\\n"
    "    '0:it\\'s' \\\n"
    "    '0:caf\xc3\xa9' \\\n"
    "    '0:k\xc3\xb8' \\\n"
    "    '0:a\x09" "b' \\\n"
    "    '0:x\x09y' \\\n"
    "    '0:del\x7f'\n"
    "set -g -- __djarg_prog_sn \\\n"
    "    'it\\'s' \\\n"
    "    'caf\xc3\xa9' \\\n"
    "    'k\xc3\xb8' \\\n"
    "    'a\x09" "b' \\\n"
    "    'x\x09y' \\\n"
    "    'del\x7f'\n"
    "set -g -- __djarg_prog_st \\\n"
    "    1 \\\n"
    "    2 \\\n"
    "    2 \\\n"
    "    3 \\\n"
    "    3 \\\n"
    "    4\n"
    "set -g -- __djarg_prog_sd \\\n"
    "    'Quote\\'s here' \\\n"
    "    'Caf\xc3\xa9 \xe2\x86\x92' \\\n";

// TREE_QUOTING_5
//   constant: the transcript of d_tests_arg_completion_tree_quoting, continued.
static const char TREE_QUOTING_5[] =
    "    'Caf\xc3\xa9 \xe2\x86\x92' \\\n"
    "    'A tab in a name' \\\n"
    "    'A tab in a name' \\\n"
    "    'A delete in a name'\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '0:--a' \\\n"
    "    '0:--b' \\\n"
    "    '0:--c' \\\n"
    "    '0:--d' \\\n"
    "    '0:--e' \\\n"
    "    '0:--f' \\\n"
    "    '1:-h' \\\n"
    "    '1:--help' \\\n"
    "    '2:-h' \\\n"
    "    '2:--help' \\\n"
    "    '3:-h' \\\n"
    "    '3:--help' \\\n"
    "    '4:-h' \\\n"
    "    '4:--help'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '--a' \\\n"
    "    '--b' \\\n"
    "    '--c' \\\n"
    "    '--d' \\\n"
    "    '--e' \\\n"
    "    '--f' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-h' \\\n"
    "    '--help'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    1 \\\n"
    "    2 \\\n"
    "    3 \\\n"
    "    4 \\\n"
    "    5 \\\n"
    "    6 \\\n"
    "    7 \\\n"
    "    7 \\\n"
    "    8 \\\n"
    "    8 \\\n"
    "    9 \\\n"
    "    9 \\\n"
    "    10 \\\n"
    "    10\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'it\\'s' \\\n"
    "    'tab here' \\\n"
    "    'bell , del , escape ' \\\n"
    "    'caf\xc3\xa9 \xe2\x86\x92 \xf0\x9f\x98\x80' \\\n"
    "    'back\\\\slash \"quoted\" $x `y`' \\\n"
    "    'stray \xff byte' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '0 0 - 1 0'\n"
    "set -g -- __djarg_prog_pk\n"
    "set -g -- __djarg_prog_pv\n"
    "set -g -- __djarg_prog_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== descriptions powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 6 0 8'\n"
    "        '0 s 0 6 0 8 2'\n"
    "        '0 s 0 6 0 10 2'\n"
    "        '0 s 0 6 0 12 2'\n"
    "        '0 s 0 6 0 14 2'\n"
    "    )\n"
    "    $sn = @(\n"
    "        'it''s'\n"
    "        ('' + 'caf' + [char]0x00E9)\n"
    "        ('' + 'k' + [char]0x00F8)\n"
    "        ('' + 'a' + [char]0x0009 + 'b')\n"
    "        ('' + 'x' + [char]0x0009 + 'y')\n"
    "        ('' + 'del' + [char]0x007F)\n"
    "    )\n"
    "    $sc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $st = @(\n"
    "        1\n"
    "        2\n"
    "        2\n"
    "        3\n"
    "        3\n"
    "        4\n"
    "    )\n"
    "    $sd = @(\n"
    "        'Quote''s here'\n"
    "        ('' + 'Caf' + [char]0x00E9 + ' ' + [char]0x2192)\n"
    "        ('' + 'Caf' + [char]0x00E9 + ' ' + [char]0x2192)\n"
    "        'A tab in a name'\n"
    "        'A tab in a name'\n"
    "        'A delete in a name'\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '--a'\n"
    "        '--b'\n"
    "        '--c'\n"
    "        '--d'\n"
    "        '--e'\n"
    "        '--f'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-h'\n"
    "        '--help'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        1\n"
    "        2\n"
    "        2\n"
    "        3\n"
    "        3\n"
    "        4\n"
    "        4\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        2\n"
    "        3\n"
    "        4\n"
    "        5\n"
    "        6\n"
    "        7\n"
    "        7\n"
    "        8\n"
    "        8\n"
    "        9\n"
    "        9\n"
    "        10\n"
    "        10\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'it''s'\n"
    "        'tab here'\n"
    "        'bell , del , escape '\n"
    "        ('' + 'caf' + [char]0x00E9 + ' ' + [char]0x2192 + ' ' + "
    "[char]0xD83D + [char]0xDE00)\n"
    "        'back\\slash \"quoted\" $x `y`'\n"
    "        ('' + 'stray ' + [char]0xFFFD + ' byte')\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n";

// TREE_QUOTING_6
//   constant: the transcript of d_tests_arg_completion_tree_quoting, continued.
static const char TREE_QUOTING_6[] =
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "        '0 0 - 0 0'\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n";

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

// TREE_MEMBER
//   constant: what d_tests_arg_completion_tree_member writes, as
// completion_ref.py works it out from the rules arg_completion.h states, in
// parts.
static const char TREE_MEMBER[] =
    "== sets bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--member') r=1 ;;\n"
    "        '0:--mapped') r=2 ;;\n"
    "        '0:--checked') r=3 ;;\n"
    "        '0:--both') r=4 ;;\n"
    "        '0:--number') r=5 ;;\n"
    "        '0:--quoted') r=6 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=c ;;\n"
    "        2) n=1 x=1 k=c ;;\n"
    "        3) n=1 x=1 k=c ;;\n"
    "        4) n=1 x=1 k=c ;;\n"
    "        5) n=1 x=1 k=c ;;\n"
    "        6) n=1 x=1 k=c ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--member' '--mapped' '--checked' '--both' "
    "'--number' '--quoted' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        1) c=( 'one' 'two' 'three' ) ;;\n"
    "        2) c=( 'low' 'high' ) ;;\n"
    "        3) c=( 'on' 'off' 'yes' 'no' ) ;;\n"
    "        4) c=( 'x' 'y' 'z' ) ;;\n"
    "        5) c=( '1' '2' ) ;;\n"
    "        6) c=( 'it'\\''s' 'a b' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== sets zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--member') r=1 ;;\n"
    "        '0:--mapped') r=2 ;;\n"
    "        '0:--checked') r=3 ;;\n"
    "        '0:--both') r=4 ;;\n"
    "        '0:--number') r=5 ;;\n"
    "        '0:--quoted') r=6 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=c ;;\n"
    "        2) n=1 x=1 k=c ;;\n"
    "        3) n=1 x=1 k=c ;;\n"
    "        4) n=1 x=1 k=c ;;\n"
    "        5) n=1 x=1 k=c ;;\n"
    "        6) n=1 x=1 k=c ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--member' '--mapped' '--checked' '--both' "
    "'--number' '--quoted' ) d=( 'Print this help message and exit' 'Print "
    "this help message and exit' 'A set' 'A mapping' 'A checked mapping' 'A "
    "set and a list' 'A set of numbers' 'A set to quote' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        1) c=( 'one' 'two' 'three' ) ;;\n"
    "        2) c=( 'low' 'high' ) ;;\n"
    "        3) c=( 'on' 'off' 'yes' 'no' ) ;;\n"
    "        4) c=( 'x' 'y' 'z' ) ;;\n"
    "        5) c=( '1' '2' ) ;;\n"
    "        6) c=( 'it'\\''s' 'a b' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== sets fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 0 1 8'\n"
    "set -g -- __djarg_prog_sk\n"
    "set -g -- __djarg_prog_sn\n"
    "set -g -- __djarg_prog_st\n"
    "set -g -- __djarg_prog_sd\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '0:--member' \\\n"
    "    '0:--mapped' \\\n"
    "    '0:--checked' \\\n"
    "    '0:--both' \\\n"
    "    '0:--number' \\\n"
    "    '0:--quoted'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '--member' \\\n"
    "    '--mapped' \\\n"
    "    '--checked' \\\n";

// TREE_MEMBER_2
//   constant: the transcript of d_tests_arg_completion_tree_member, continued.
static const char TREE_MEMBER_2[] =
    "    '--both' \\\n"
    "    '--number' \\\n"
    "    '--quoted'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    1 \\\n"
    "    2 \\\n"
    "    3 \\\n"
    "    4 \\\n"
    "    5 \\\n"
    "    6\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'A set' \\\n"
    "    'A mapping' \\\n"
    "    'A checked mapping' \\\n"
    "    'A set and a list' \\\n"
    "    'A set of numbers' \\\n"
    "    'A set to quote'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '1 1 c 1 3' \\\n"
    "    '1 1 c 4 2' \\\n"
    "    '1 1 c 6 4' \\\n"
    "    '1 1 c 10 3' \\\n"
    "    '1 1 c 13 2' \\\n"
    "    '1 1 c 15 2'\n"
    "set -g -- __djarg_prog_pk\n"
    "set -g -- __djarg_prog_pv\n"
    "set -g -- __djarg_prog_vv \\\n"
    "    'one' \\\n"
    "    'two' \\\n"
    "    'three' \\\n"
    "    'low' \\\n"
    "    'high' \\\n"
    "    'on' \\\n"
    "    'off' \\\n"
    "    'yes' \\\n"
    "    'no' \\\n"
    "    'x' \\\n"
    "    'y' \\\n"
    "    'z' \\\n"
    "    '1' \\\n"
    "    '2' \\\n"
    "    'it\\'s' \\\n"
    "    'a b'\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== sets powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 8'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '--member'\n"
    "        '--mapped'\n"
    "        '--checked'\n"
    "        '--both'\n"
    "        '--number'\n"
    "        '--quoted'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        2\n"
    "        3\n"
    "        4\n"
    "        5\n"
    "        6\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'A set'\n"
    "        'A mapping'\n"
    "        'A checked mapping'\n"
    "        'A set and a list'\n"
    "        'A set of numbers'\n"
    "        'A set to quote'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '1 1 c 0 3'\n"
    "        '1 1 c 3 2'\n"
    "        '1 1 c 5 4'\n"
    "        '1 1 c 9 3'\n"
    "        '1 1 c 12 2'\n"
    "        '1 1 c 14 2'\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "        'one'\n"
    "        'two'\n"
    "        'three'\n"
    "        'low'\n"
    "        'high'\n"
    "        'on'\n"
    "        'off'\n"
    "        'yes'\n"
    "        'no'\n"
    "        'x'\n"
    "        'y'\n"
    "        'z'\n"
    "        '1'\n"
    "        '2'\n"
    "        'it''s'\n"
    "        'a b'\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n";

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)

// TREE_PATH
//   constant: what d_tests_arg_completion_tree_path writes, as
// completion_ref.py works it out from the rules arg_completion.h states, in
// parts.
static const char TREE_PATH[] =
    "== paths bash OK\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--file') r=1 ;;\n"
    "        '0:--dir') r=2 ;;\n"
    "        '0:--path') r=3 ;;\n"
    "        '0:--new') r=4 ;;\n"
    "        '0:--dir-or-file') r=5 ;;\n"
    "        '0:--dir-and-number') r=6 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=f ;;\n"
    "        2) n=1 x=1 k=d ;;\n"
    "        3) n=1 x=1 k=f ;;\n"
    "        4) n=1 x=1 k=f ;;\n"
    "        5) n=1 x=1 k=f ;;\n"
    "        6) n=1 x=1 k=n ;;\n"
    "        7) n=1 x=1 k=d ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        0:0) r=7 y=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--file' '--dir' '--path' '--new' "
    "'--dir-or-file' '--dir-and-number' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== paths zsh OK\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--file') r=1 ;;\n"
    "        '0:--dir') r=2 ;;\n"
    "        '0:--path') r=3 ;;\n"
    "        '0:--new') r=4 ;;\n"
    "        '0:--dir-or-file') r=5 ;;\n"
    "        '0:--dir-and-number') r=6 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=f ;;\n"
    "        2) n=1 x=1 k=d ;;\n"
    "        3) n=1 x=1 k=f ;;\n"
    "        4) n=1 x=1 k=f ;;\n"
    "        5) n=1 x=1 k=f ;;\n"
    "        6) n=1 x=1 k=n ;;\n"
    "        7) n=1 x=1 k=d ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        0:0) r=7 y=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--file' '--dir' '--path' '--new' "
    "'--dir-or-file' '--dir-and-number' ) d=( 'Print this help message and "
    "exit' 'Print this help message and exit' 'An existing file' 'An existing "
    "directory' 'An existing path' 'A path not there yet' 'Either' 'Both' ) "
    ";;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== paths fish OK\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 0 1 8'\n"
    "set -g -- __djarg_prog_sk\n"
    "set -g -- __djarg_prog_sn\n"
    "set -g -- __djarg_prog_st\n"
    "set -g -- __djarg_prog_sd\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '0:--file' \\\n"
    "    '0:--dir' \\\n"
    "    '0:--path' \\\n"
    "    '0:--new' \\\n"
    "    '0:--dir-or-file' \\\n"
    "    '0:--dir-and-number'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '--file' \\\n"
    "    '--dir' \\\n"
    "    '--path' \\\n"
    "    '--new' \\\n"
    "    '--dir-or-file' \\\n"
    "    '--dir-and-number'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    1 \\\n"
    "    2 \\\n"
    "    3 \\\n"
    "    4 \\\n"
    "    5 \\\n"
    "    6\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'An existing file' \\\n"
    "    'An existing directory' \\\n";

// TREE_PATH_2
//   constant: the transcript of d_tests_arg_completion_tree_path, continued.
static const char TREE_PATH_2[] =
    "    'An existing path' \\\n"
    "    'A path not there yet' \\\n"
    "    'Either' \\\n"
    "    'Both'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '1 1 f 1 0' \\\n"
    "    '1 1 d 1 0' \\\n"
    "    '1 1 f 1 0' \\\n"
    "    '1 1 f 1 0' \\\n"
    "    '1 1 f 1 0' \\\n"
    "    '1 1 n 1 0' \\\n"
    "    '1 1 d 1 0'\n"
    "set -g -- __djarg_prog_pk \\\n"
    "    '0:0'\n"
    "set -g -- __djarg_prog_pv \\\n"
    "    '7 1'\n"
    "set -g -- __djarg_prog_vv\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== paths powershell OK\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 8'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '--file'\n"
    "        '--dir'\n"
    "        '--path'\n"
    "        '--new'\n"
    "        '--dir-or-file'\n"
    "        '--dir-and-number'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        2\n"
    "        3\n"
    "        4\n"
    "        5\n"
    "        6\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'An existing file'\n"
    "        'An existing directory'\n"
    "        'An existing path'\n"
    "        'A path not there yet'\n"
    "        'Either'\n"
    "        'Both'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '1 1 f 0 0'\n"
    "        '1 1 d 0 0'\n"
    "        '1 1 f 0 0'\n"
    "        '1 1 f 0 0'\n"
    "        '1 1 f 0 0'\n"
    "        '1 1 n 0 0'\n"
    "        '1 1 d 0 0'\n"
    "    )\n"
    "    $ps = @(\n"
    "        '0:0'\n"
    "    )\n"
    "    $pv = @(\n"
    "        '7 1'\n"
    "    )\n"
    "    $vv = @(\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n";

#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS

// FLAG
//   constant: what d_tests_arg_completion_flag writes, as completion_ref.py
// works it out from the rules arg_completion.h states, in parts.
static const char FLAG[] =
    "== bash - [--completion bash]\n"
    "status=CallForCompletion code=0\n"
    "out:\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--completion') r=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=c ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--completion' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        1) c=( 'bash' 'zsh' 'fish' 'powershell' 'pwsh' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== zsh - [--completion zsh]\n"
    "status=CallForCompletion code=0\n"
    "out:\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--completion') r=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=c ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--completion' ) d=( 'Print this help "
    "message and exit' 'Print this help message and exit' 'Print a completion "
    "script for a shell (bash, zsh, fish or powershell) and exit' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        1) c=( 'bash' 'zsh' 'fish' 'powershell' 'pwsh' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== fish - [--completion fish]\n"
    "status=CallForCompletion code=0\n"
    "out:\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 0 1 3'\n"
    "set -g -- __djarg_prog_sk\n"
    "set -g -- __djarg_prog_sn\n"
    "set -g -- __djarg_prog_st\n"
    "set -g -- __djarg_prog_sd\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '0:--completion'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '--completion'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    1\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print a completion script for a shell (bash, zsh, fish or "
    "powershell) and exit'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '1 1 c 1 5'\n"
    "set -g -- __djarg_prog_pk\n"
    "set -g -- __djarg_prog_pv\n"
    "set -g -- __djarg_prog_vv \\\n"
    "    'bash' \\\n"
    "    'zsh' \\\n"
    "    'fish' \\\n"
    "    'powershell' \\\n"
    "    'pwsh'\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== powershell - [--completion powershell]\n"
    "status=CallForCompletion code=0\n"
    "out:\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 3'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n";

// FLAG_2
//   constant: the transcript of d_tests_arg_completion_flag, continued.
static const char FLAG_2[] =
    "        '--completion'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print a completion script for a shell (bash, zsh, fish or "
    "powershell) and exit'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '1 1 c 0 5'\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "        'bash'\n"
    "        'zsh'\n"
    "        'fish'\n"
    "        'powershell'\n"
    "        'pwsh'\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n"
    "== pwsh - [--completion pwsh]\n"
    "status=CallForCompletion code=0\n"
    "out:\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 3'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '--completion'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print a completion script for a shell (bash, zsh, fish or "
    "powershell) and exit'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '1 1 c 0 5'\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "        'bash'\n"
    "        'zsh'\n"
    "        'fish'\n"
    "        'powershell'\n"
    "        'pwsh'\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n"
    "== any case - [--completion BaSh]\n"
    "status=CallForCompletion code=0\n"
    "out:\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--completion') r=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=c ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--completion' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        1) c=( 'bash' 'zsh' 'fish' 'powershell' 'pwsh' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== any case - [--completion PWSH]\n"
    "status=CallForCompletion code=0\n"
    "out:\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 3'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '--completion'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Print a completion script for a shell (bash, zsh, fish or "
    "powershell) and exit'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '1 1 c 0 5'\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "        'bash'\n"
    "        'zsh'\n"
    "        'fish'\n"
    "        'powershell'\n"
    "        'pwsh'\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n"
    "== after = - [--completion=zsh]\n"
    "status=CallForCompletion code=0\n"
    "out:\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n";

// FLAG_3
//   constant: the transcript of d_tests_arg_completion_flag, continued.
static const char FLAG_3[] =
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--completion') r=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=c ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--completion' ) d=( 'Print this help "
    "message and exit' 'Print this help message and exit' 'Print a completion "
    "script for a shell (bash, zsh, fish or powershell) and exit' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        1) c=( 'bash' 'zsh' 'fish' 'powershell' 'pwsh' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== the program from argv[0] /opt/bin/Tool.EXE [--completion fish]\n"
    "status=CallForCompletion code=0\n"
    "out:\n"
    "# fish completion for Tool, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_Tool_cmds \\\n"
    "    '0 s 0 1 0 1 3'\n"
    "set -g -- __djarg_Tool_sk\n"
    "set -g -- __djarg_Tool_sn\n"
    "set -g -- __djarg_Tool_st\n"
    "set -g -- __djarg_Tool_sd\n"
    "set -g -- __djarg_Tool_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '0:--completion'\n"
    "set -g -- __djarg_Tool_on \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '--completion'\n"
    "set -g -- __djarg_Tool_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    1\n"
    "set -g -- __djarg_Tool_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print a completion script for a shell (bash, zsh, fish or "
    "powershell) and exit'\n"
    "set -g -- __djarg_Tool_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '1 1 c 1 5'\n"
    "set -g -- __djarg_Tool_pk\n"
    "set -g -- __djarg_Tool_pv\n"
    "set -g -- __djarg_Tool_vv \\\n"
    "    'bash' \\\n"
    "    'zsh' \\\n"
    "    'fish' \\\n"
    "    'powershell' \\\n"
    "    'pwsh'\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'Tool' -e\n"
    "complete -c 'Tool' -f -a '(__djarg_Tool_complete)'\n"
    "== the program from argv[0] tool [--completion bash]\n"
    "status=CallForCompletion code=0\n"
    "out:\n"
    "# bash completion for tool, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_tool_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--completion') r=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=c ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--completion' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_tool_vals() {\n"
    "    case $1 in\n"
    "        1) c=( 'bash' 'zsh' 'fish' 'powershell' 'pwsh' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_tool 'tool'\n"
    "== the program from argv[0] C:\\\\bin\\\\x.exe [--completion powershell]\n"
    "status=CallForCompletion code=0\n"
    "out:\n"
    "# PowerShell completion for x, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 3'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '--completion'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n";

// FLAG_4
//   constant: the transcript of d_tests_arg_completion_flag, continued.
static const char FLAG_4[] =
    "        'Print this help message and exit'\n"
    "        'Print a completion script for a shell (bash, zsh, fish or "
    "powershell) and exit'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '1 1 c 0 5'\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "        'bash'\n"
    "        'zsh'\n"
    "        'fish'\n"
    "        'powershell'\n"
    "        'pwsh'\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'x', 'x.exe') "
    "-ScriptBlock $complete\n"
    "}\n"
    "== the program after a parse without one first [--completion bash]\n"
    "status=CallForCompletion code=0\n"
    "out:\n"
    "# bash completion for first, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_first_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_first_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_first_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--completion') r=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_first_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=c ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_first_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_first_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_first_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--completion' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_first_vals() {\n"
    "    case $1 in\n"
    "        1) c=( 'bash' 'zsh' 'fish' 'powershell' 'pwsh' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_first 'first'\n"
    "== the program after a parse without one - [--completion bash]\n"
    "status=CallForCompletion code=0\n"
    "out:\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--completion') r=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=c ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--completion' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        1) c=( 'bash' 'zsh' 'fish' 'powershell' 'pwsh' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== a shell that is none - [--completion tcsh]\n"
    "status=ValidationError code=105\n"
    "err: [--completion: tcsh not in {bash,zsh,fish,powershell,pwsh}\\nRun "
    "with --help for more information.\\n]\n"
    "== a shell that is none - [--completion ]\n"
    "status=ValidationError code=105\n"
    "err: [--completion:  not in {bash,zsh,fish,powershell,pwsh}\\nRun with "
    "--help for more information.\\n]\n"
    "== no shell - [--completion]\n"
    "status=ArgumentMismatch code=114\n"
    "err: [--completion: 1 required SHELL missing\\nRun with --help for more "
    "information.\\n]\n"
    "== no shell - [--completion=]\n"
    "status=ArgumentMismatch code=114\n"
    "err: [--completion: 1 required SHELL missing\\nRun with --help for more "
    "information.\\n]\n"
    "== two shells - [--completion bash --completion zsh]\n"
    "status=ArgumentMismatch code=114\n"
    "err: [--completion: At most 1 required but received 2\\nRun with --help "
    "for more information.\\n]\n"
    "== help wins - [--completion bash --help]\n"
    "status=CallForHelp code=0\n"
    "out: [d]\n"
    "== help wins - [-h --completion zsh]\n"
    "status=CallForHelp code=0\n"
    "out: [d]\n"
    "== version wins - [--completion fish -V]\n"
    "status=CallForVersion code=0\n"
    "out: [1.2]\n"
    "== version wins - [-V --completion fish]\n"
    "status=CallForVersion code=0\n"
    "out: [1.2]\n"
    "== before the requirements - [-a -x -y --completion bash]\n"
    "status=CallForCompletion code=0\n"
    "out:\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n";

// FLAG_5
//   constant: the transcript of d_tests_arg_completion_flag, continued.
static const char FLAG_5[] =
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        1) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        '0:sub') r=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--completion') r=1 ;;\n"
    "        '0:--need') r=2 ;;\n"
    "        '0:-a') r=4 ;;\n"
    "        '0:-b') r=5 ;;\n"
    "        '0:-x') r=6 ;;\n"
    "        '0:-y') r=7 ;;\n"
    "        '1:-h') r=8 ;;\n"
    "        '1:--help') r=8 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=c ;;\n"
    "        2) n=1 x=1 k=f ;;\n"
    "        3) n=1 x=1 k=f ;;\n"
    "        4) n=0 x=0 k=- ;;\n"
    "        5) n=0 x=0 k=- ;;\n"
    "        6) n=0 x=0 k=- ;;\n"
    "        7) n=0 x=0 k=- ;;\n"
    "        8) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        0:0) r=3 y=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        0) c=( 'sub' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--completion' '--need' '-a' '-b' '-x' '-y' "
    ") ;;\n"
    "        1) c=( '-h' '--help' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        1) c=( 'bash' 'zsh' 'fish' 'powershell' 'pwsh' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== before the requirements - [-a -x -y]\n"
    "status=RequiredError code=106\n"
    "err: [--need is required\\nRun with --help for more information.\\n]\n"
    "== before other callbacks - [--noted v --completion zsh]\n"
    "status=CallForCompletion code=0\n"
    "out:\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--completion') r=1 ;;\n"
    "        '0:--noted') r=2 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=c ;;\n"
    "        2) n=1 x=1 k=f ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--completion' '--noted' ) d=( 'Print this "
    "help message and exit' 'Print this help message and exit' 'Print a "
    "completion script for a shell (bash, zsh, fish or powershell) and exit' "
    "'' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        1) c=( 'bash' 'zsh' 'fish' 'powershell' 'pwsh' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== before other callbacks - [--noted v]\n"
    "noted [v]\n"
    "status=OK code=0\n"
    "== from a subcommand falling through - [sub --completion bash]\n"
    "status=CallForCompletion code=0\n"
    "out:\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        1) p=0 f=sf m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        '0:sub') r=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--completion') r=1 ;;\n"
    "        '1:-h') r=2 ;;\n"
    "        '1:--help') r=2 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=c ;;\n"
    "        2) n=0 x=0 k=- ;;\n";

// FLAG_6
//   constant: the transcript of d_tests_arg_completion_flag, continued.
static const char FLAG_6[] =
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        0) c=( 'sub' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--completion' ) ;;\n"
    "        1) c=( '-h' '--help' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        1) c=( 'bash' 'zsh' 'fish' 'powershell' 'pwsh' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== from a subcommand not falling through - [sub --completion bash]\n"
    "status=ExtrasError code=109\n"
    "err: [sub: The following arguments were not expected: --completion "
    "bash\\nRun with --help for more information.\\n]\n"
    "== renamed - [-c fish]\n"
    "status=CallForCompletion code=0\n"
    "out:\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 0 1 4'\n"
    "set -g -- __djarg_prog_sk\n"
    "set -g -- __djarg_prog_sn\n"
    "set -g -- __djarg_prog_st\n"
    "set -g -- __djarg_prog_sd\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '0:-c' \\\n"
    "    '0:--complete'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '-c' \\\n"
    "    '--complete'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    1 \\\n"
    "    1\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Write a script' \\\n"
    "    'Write a script'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '1 1 c 1 5'\n"
    "set -g -- __djarg_prog_pk\n"
    "set -g -- __djarg_prog_pv\n"
    "set -g -- __djarg_prog_vv \\\n"
    "    'bash' \\\n"
    "    'zsh' \\\n"
    "    'fish' \\\n"
    "    'powershell' \\\n"
    "    'pwsh'\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== renamed - [--complete=powershell]\n"
    "status=CallForCompletion code=0\n"
    "out:\n"
    "# PowerShell completion for prog, generated by djinterp's arg_parser.\n"
    "# Run it in PowerShell -- from the profile, to keep it.\n"
    "\n"
    "& {\n"
    "    $cmds = @(\n"
    "        '0 s 0 0 0 0 4'\n"
    "    )\n"
    "    $sn = @(\n"
    "    )\n"
    "    $sc = @(\n"
    "    )\n"
    "    $st = @(\n"
    "    )\n"
    "    $sd = @(\n"
    "    )\n"
    "    $on = @(\n"
    "        '-h'\n"
    "        '--help'\n"
    "        '-c'\n"
    "        '--complete'\n"
    "    )\n"
    "    $oc = @(\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "        0\n"
    "    )\n"
    "    $ov = @(\n"
    "        0\n"
    "        0\n"
    "        1\n"
    "        1\n"
    "    )\n"
    "    $od = @(\n"
    "        'Print this help message and exit'\n"
    "        'Print this help message and exit'\n"
    "        'Write a script'\n"
    "        'Write a script'\n"
    "    )\n"
    "    $info = @(\n"
    "        '0 0 - 0 0'\n"
    "        '1 1 c 0 5'\n"
    "    )\n"
    "    $ps = @(\n"
    "    )\n"
    "    $pv = @(\n"
    "    )\n"
    "    $vv = @(\n"
    "        'bash'\n"
    "        'zsh'\n"
    "        'fish'\n"
    "        'powershell'\n"
    "        'pwsh'\n"
    "    )\n"
    "[engine pwsh]\n"
    "    Register-ArgumentCompleter -Native -CommandName @( 'prog', "
    "'prog.exe') -ScriptBlock $complete\n"
    "}\n"
    "== replaced - [--script zsh]\n"
    "status=CallForCompletion code=0\n"
    "out:\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--script') r=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=c ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--script' ) d=( 'Print this help message "
    "and exit' 'Print this help message and exit' 'Print a completion script "
    "for a shell (bash, zsh, fish or powershell) and exit' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        1) c=( 'bash' 'zsh' 'fish' 'powershell' 'pwsh' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n";

// FLAG_7
//   constant: the transcript of d_tests_arg_completion_flag, continued.
static const char FLAG_7[] =
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n"
    "== replaced - [--completion zsh]\n"
    "status=ExtrasError code=109\n"
    "err: [prog: The following arguments were not expected: --completion "
    "zsh\\nRun with --help for more information.\\n]\n"
    "== removed - [--completion bash]\n"
    "status=ExtrasError code=109\n"
    "err: [prog: The following arguments were not expected: --completion "
    "bash\\nRun with --help for more information.\\n]\n"
    "== removed by an empty name - [--completion bash]\n"
    "status=ExtrasError code=109\n"
    "err: [prog: The following arguments were not expected: --completion "
    "bash\\nRun with --help for more information.\\n]\n"
    "== moved into a group, then replaced - [--again bash]\n"
    "status=CallForCompletion code=0\n"
    "out:\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--again') r=1 ;;\n"
    "        '0:--kept') r=2 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=c ;;\n"
    "        2) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--again' '--kept' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        1) c=( 'bash' 'zsh' 'fish' 'powershell' 'pwsh' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== moved into a group - [--completion fish]\n"
    "status=CallForCompletion code=0\n"
    "out:\n"
    "# fish completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in fish, or save it as <program>.fish in a directory of\n"
    "# $fish_complete_path.\n"
    "\n"
    "set -g -- __djarg_prog_cmds \\\n"
    "    '0 s 0 1 0 1 3'\n"
    "set -g -- __djarg_prog_sk\n"
    "set -g -- __djarg_prog_sn\n"
    "set -g -- __djarg_prog_st\n"
    "set -g -- __djarg_prog_sd\n"
    "set -g -- __djarg_prog_ok \\\n"
    "    '0:-h' \\\n"
    "    '0:--help' \\\n"
    "    '0:--completion'\n"
    "set -g -- __djarg_prog_on \\\n"
    "    '-h' \\\n"
    "    '--help' \\\n"
    "    '--completion'\n"
    "set -g -- __djarg_prog_ov \\\n"
    "    0 \\\n"
    "    0 \\\n"
    "    1\n"
    "set -g -- __djarg_prog_od \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print this help message and exit' \\\n"
    "    'Print a completion script for a shell (bash, zsh, fish or "
    "powershell) and exit'\n"
    "set -g -- __djarg_prog_info \\\n"
    "    '0 0 - 1 0' \\\n"
    "    '1 1 c 1 5'\n"
    "set -g -- __djarg_prog_pk\n"
    "set -g -- __djarg_prog_pv\n"
    "set -g -- __djarg_prog_vv \\\n"
    "    'bash' \\\n"
    "    'zsh' \\\n"
    "    'fish' \\\n"
    "    'powershell' \\\n"
    "    'pwsh'\n"
    "\n"
    "[engine fish]\n"
    "\n"
    "complete -c 'prog' -e\n"
    "complete -c 'prog' -f -a '(__djarg_prog_complete)'\n"
    "== names refused - []\n"
    "status=BadNameString code=101\n"
    "err: [Long names strings require 2 dashes -bad name\\nRun with --help for "
    "more information.\\n]\n"
    "== among other options - [--color red --completion zsh]\n"
    "status=CallForCompletion code=0\n"
    "out:\n"
    "#compdef prog\n"
    "# zsh completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it after compinit, or save it as _<program> in a directory of "
    "$fpath.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        1) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        '0:run') r=1 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--color') r=1 ;;\n"
    "        '0:--completion') r=2 ;;\n"
    "        '1:-h') r=3 ;;\n"
    "        '1:--help') r=3 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=c ;;\n"
    "        2) n=1 x=1 k=c ;;\n";

// FLAG_8
//   constant: the transcript of d_tests_arg_completion_flag, continued.
static const char FLAG_8[] =
    "        3) n=0 x=0 k=- ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        0) c=( 'run' ) d=( 'Run it' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--color' '--completion' ) d=( 'Print this "
    "help message and exit' 'Print this help message and exit' 'A color' "
    "'Print a completion script for a shell (bash, zsh, fish or powershell) "
    "and exit' ) ;;\n"
    "        1) c=( '-h' '--help' ) d=( 'Print this help message and exit' "
    "'Print this help message and exit' ) ;;\n"
    "        *) c=() d=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        1) c=( 'red' 'blue' ) ;;\n"
    "        2) c=( 'bash' 'zsh' 'fish' 'powershell' 'pwsh' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine zsh]\n"
    "if [[ ${funcstack[1]-} == '_prog' ]]; then\n"
    "    compdef _djarg_prog 'prog'\n"
    "    _djarg_prog \"$@\"\n"
    "elif (( ${+functions[compdef]} )); then\n"
    "    compdef _djarg_prog 'prog'\n"
    "else\n"
    "    print -ru2 -- 'prog' 'completion needs compinit first'\n"
    "fi\n";

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// FLAG_CONFIG
//   constant: what d_tests_arg_completion_flag_config writes, as
// completion_ref.py works it out from the rules arg_completion.h states.
static const char FLAG_CONFIG[] =
    "== a configuration file not read - [--completion bash]\n"
    "status=CallForCompletion code=0\n"
    "out:\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--completion') r=1 ;;\n"
    "        '0:--config') r=2 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=c ;;\n"
    "        2) n=1 x=1 k=f ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--completion' '--config' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        1) c=( 'bash' 'zsh' 'fish' 'powershell' 'pwsh' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "== a configuration file not read - []\n"
    "status=FileError code=103\n"
    "err: [missing.ini was not readable (missing?)\\nRun with --help for more "
    "information.\\n]\n"
    "== not configurable config [completion=bash\\n]\n"
    "status=ConfigError code=110\n"
    "err: [completion: This option is not allowed in a configuration "
    "file\\nRun with --help for more information.\\n]\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_ENVIRONMENT == 1)

// FLAG_ENVIRONMENT
//   constant: what d_tests_arg_completion_flag_environment writes, as
// completion_ref.py works it out from the rules arg_completion.h states.
static const char FLAG_ENVIRONMENT[] =
    "== the environment not read - [--completion bash]\n"
    "status=CallForCompletion code=0\n"
    "out:\n"
    "# bash completion for prog, generated by djinterp's arg_parser.\n"
    "# Source it in bash, or save it among bash-completion's completions.\n"
    "\n"
    "_djarg_prog_cmd() {\n"
    "    case $1 in\n"
    "        0) p=0 f=s m=0 ;;\n"
    "        *) p=0 f=- m=0 ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_sub() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opt() {\n"
    "    case $1:$2 in\n"
    "        '0:-h') r=0 ;;\n"
    "        '0:--help') r=0 ;;\n"
    "        '0:--completion') r=1 ;;\n"
    "        '0:--level') r=2 ;;\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_info() {\n"
    "    case $1 in\n"
    "        0) n=0 x=0 k=- ;;\n"
    "        1) n=1 x=1 k=c ;;\n"
    "        2) n=1 x=1 k=f ;;\n"
    "        *) n=0 x=0 k=- ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_pos() {\n"
    "    case $1:$2 in\n"
    "        *) r= ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_subs() {\n"
    "    case $1 in\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_opts() {\n"
    "    case $1 in\n"
    "        0) c=( '-h' '--help' '--completion' '--level' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "_djarg_prog_vals() {\n"
    "    case $1 in\n"
    "        1) c=( 'bash' 'zsh' 'fish' 'powershell' 'pwsh' ) ;;\n"
    "        *) c=() ;;\n"
    "    esac\n"
    "}\n"
    "[engine walk]\n"
    "[engine bash]\n"
    "complete -F _djarg_prog 'prog'\n"
    "given l 0\n"
    "== the environment not read - []\n"
    "status=OK code=0\n"
    "given l 1\n";

#endif  // D_INTERNAL_ARG_PARSER_ENVIRONMENT

// ENGINE_WALK
//   constant: the walk over the words bash and zsh share, as
// completion/eng/walk.sh holds it -- the source the library's array is written
// from -- each `@@` standing for the program's identifier.
static const char* const ENGINE_WALK[] =
{
    "__djarg_@@_first() {",
    "    # whether `$1` may begin a name",
    "    case $1 in",
    "        ''|-|'!'|[[:space:]]|[[:cntrl:]]) return 1 ;;",
    "    esac",
    "    return 0",
    "}",
    "__djarg_@@_find() {",
    "    # o: the option named `$1` at the command or, falling",
    "    # through, above it",
    "    local L=$cmd p f m r",
    "    o=",
    "    while :; do",
    "        _djarg_@@_opt \"$L\" \"$1\"",
    "        if [[ -n $r ]]; then",
    "            o=$r",
    "            return",
    "        fi",
    "        _djarg_@@_cmd \"$L\"",
    "        if [[ $f == *f* ]] && (( L != 0 )); then",
    "            L=$p",
    "        else",
    "            return",
    "        fi",
    "    done",
    "}",
    "__djarg_@@_target() {",
    "    # t: the subcommand `$1` names, from the command up, and l:",
    "    # the command it is under",
    "    local L=$cmd p f m r",
    "    t= l=",
    "    while :; do",
    "        _djarg_@@_cmd \"$L\"",
    "        if (( m == 0 )) || (( ${used[$L]:-0} < m )) ||",
    "           [[ $f != *s* ]]; then",
    "            _djarg_@@_sub \"$L\" \"$1\"",
    "            if [[ -n $r ]]; then",
    "                t=$r l=$L",
    "                return",
    "            fi",
    "        fi",
    "        if [[ $f == *s* ]] && (( L != 0 )); then",
    "            L=$p",
    "        else",
    "            return",
    "        fi",
    "    done",
    "}",
    "__djarg_@@_class() {",
    "    # r: what `$1` is to the command -- mark, sub, long, short,",
    "    # win, term or none",
    "    local w=$1",
    "    r=none",
    "    if [[ $w == -- ]]; then",
    "        r=mark",
    "        return",
    "    fi",
    "    __djarg_@@_target \"$w\"",
    "    if [[ -n $t ]]; then",
    "        r=sub",
    "        return",
    "    fi",
    "    case $w in",
    "        --?*)",
    "            if __djarg_@@_first \"${w:2:1}\"; then",
    "                r=long",
    "            fi",
    "            return",
    "            ;;",
    "        -?*)",
    "            if ! __djarg_@@_first \"${w:1:1}\"; then",
    "                return",
    "            fi",
    "            case ${w:1:2} in",
    "                [0-9]*|.[0-9])",
    "                    _djarg_@@_opt \"$cmd\" \"-${w:1:1}\"",
    "                    if [[ -z $r ]]; then",
    "                        r=none",
    "                        return",
    "                    fi",
    "                    ;;",
    "            esac",
    "            r=short",
    "            return",
    "            ;;",
    "    esac",
    "    if [[ $cf == *w* ]] && [[ $w == /?* ]] &&",
    "       __djarg_@@_first \"${w:1:1}\"; then",
    "        r=win",
    "    elif [[ $w == ++ ]] && (( cmd != 0 )); then",
    "        r=term",
    "    fi",
    "}",
    "__djarg_@@_take() {",
    "    # o: the positional that takes a word, from the command up,",
    "    # counted when `$1` is 1; extra: the last command's flags",
    "    local L=$cmd s r y p f m",
    "    o=",
    "    while :; do",
    "        s=0",
    "        while :; do",
    "            _djarg_@@_pos \"$L\" \"$s\"",
    "            if [[ -z $r ]]; then",
    "                break",
    "            fi",
    "            if (( y < 0 )) || (( ${counts[$r]:-0} < y )); then",
    "                o=$r",
    "                if (( $1 )); then",
    "                    counts[$r]=$(( ${counts[$r]:-0} + 1 ))",
    "                fi",
    "                return",
    "            fi",
    "            s=$(( s + 1 ))",
    "        done",
    "        _djarg_@@_cmd \"$L\"",
    "        if [[ $f == *f* ]] && (( L != 0 )); then",
    "            L=$p",
    "        else",
    "            extra=$f",
    "            return",
    "        fi",
    "    done",
    "}",
    "__djarg_@@_given() {",
    "    # the option found, met with the value `$1` -- empty for",
    "    # none: what it still needs, and what more it may take",
    "    local n x k got=0",
    "    if [[ -z $o ]]; then",
    "        return",
    "    fi",
    "    _djarg_@@_info \"$o\"",
    "    if (( x == 0 )); then",
    "        return",
    "    fi",
    "    if [[ -n $1 ]]; then",
    "        got=1",
    "    fi",
    "    need=$(( n - got ))",
    "    if (( need < 0 )); then",
    "        need=0",
    "    fi",
    "    if (( x < 0 )); then",
    "        room=-1",
    "    elif (( got > n )); then",
    "        room=$(( x - got ))",
    "    else",
    "        room=$(( x - n ))",
    "    fi",
    "    if (( need > 0 )) || (( room != 0 )); then",
    "        opt=$o",
    "    fi",
    "}",
    "__djarg_@@_step() {",
    "    # one word the parse meets",
    "    local w=$1 r t l o name rest n x k",
    "    __djarg_@@_class \"$w\"",
    "    case $r in",
    "        mark)",
    "            only=1",
    "            ;;",
    "        sub)",
    "            used[$l]=$(( ${used[$l]:-0} + 1 ))",
    "            cmd=$t",
    "            _djarg_@@_cmd \"$cmd\"",
    "            cf=$f",
    "            ;;",
    "        term)",
    "            _djarg_@@_cmd \"$cmd\"",
    "            cmd=$p",
    "            _djarg_@@_cmd \"$cmd\"",
    "            cf=$f",
    "            ;;",
    "        long)",
    "            name=${w%%=*}",
    "            rest=${w#\"$name\"}",
    "            __djarg_@@_find \"$name\"",
    "            __djarg_@@_given \"${rest:1}\"",
    "            ;;",
    "        win)",
    "            name=${w%%:*}",
    "            rest=${w#\"$name\"}",
    "            __djarg_@@_find \"--${name:1}\"",
    "            if [[ -z $o ]]; then",
    "                __djarg_@@_find \"-${name:1}\"",
    "            fi",
    "            __djarg_@@_given \"${rest:1}\"",
    "            ;;",
    "        short)",
    "            rest=${w:1}",
    "            while [[ -n $rest ]]; do",
    "                name=-${rest:0:1}",
    "                rest=${rest:1}",
    "                __djarg_@@_find \"$name\"",
    "                if [[ -z $o ]]; then",
    "                    break",
    "                fi",
    "                _djarg_@@_info \"$o\"",
    "                if (( x != 0 )); then",
    "                    __djarg_@@_given \"$rest\"",
    "                    break",
    "                fi",
    "            done",
    "            ;;",
    "        *)",
    "            __djarg_@@_take 1",
    "            ;;",
    "    esac",
    "}"
};

// ENGINE_BASH
//   constant: bash's own functions, as completion/eng/bash.sh holds it -- the
// source the library's array is written from -- each `@@` standing for the
// program's identifier.
static const char* const ENGINE_BASH[] =
{
    "__djarg_@@_words() {",
    "    # words and cword: the line to the cursor in words, rejoining",
    "    # what COMP_WORDBREAKS split; tail: the part readline replaces",
    "    local line=${COMP_LINE:0:COMP_POINT} i=0 w lead",
    "    words=() cword=-1 tail=",
    "    while (( i <= COMP_CWORD )); do",
    "        w=${COMP_WORDS[i]}",
    "        lead=${line%%[![:space:]]*}",
    "        line=${line#\"$lead\"}",
    "        if (( i == COMP_CWORD )); then",
    "            w=$line",
    "        fi",
    "        line=${line#\"$w\"}",
    "        if [[ -n $lead ]] || (( cword < 0 )); then",
    "            cword=$(( cword + 1 ))",
    "            words[cword]=$w",
    "        else",
    "            words[cword]=${words[cword]}$w",
    "        fi",
    "        i=$(( i + 1 ))",
    "    done",
    "    tail=$w",
    "}",
    "__djarg_@@_unq() {",
    "    # r: `$1` with its quotes and escapes removed",
    "    local s=$1 out= ch q=",
    "    case $s in",
    "        *[\\\\\\'\\\"]*) ;;",
    "        *) r=$s; return ;;",
    "    esac",
    "    while [[ -n $s ]]; do",
    "        ch=${s:0:1} s=${s:1}",
    "        if [[ $q == \"'\" ]]; then",
    "            if [[ $ch == \"'\" ]]; then",
    "                q=",
    "            else",
    "                out=$out$ch",
    "            fi",
    "        elif [[ $q == '\"' ]]; then",
    "            if [[ $ch == '\"' ]]; then",
    "                q=",
    "            elif [[ $ch == '\\' ]]; then",
    "                case ${s:0:1} in",
    "                    '\"'|'\\'|'$'|'`') out=$out${s:0:1} s=${s:1} ;;",
    "                    *) out=$out$ch ;;",
    "                esac",
    "            else",
    "                out=$out$ch",
    "            fi",
    "        else",
    "            case $ch in",
    "                \"'\"|'\"') q=$ch ;;",
    "                '\\') out=$out${s:0:1} s=${s:1} ;;",
    "                *) out=$out$ch ;;",
    "            esac",
    "        fi",
    "    done",
    "    r=$out",
    "}",
    "__djarg_@@_kind() {",
    "    # the values of kind `$1` -- c, the choices in c; f, files; d,",
    "    # directories -- that start with `$3`, after `$2`",
    "    local v",
    "    case $1 in",
    "        c)",
    "            if (( ${#c[@]} > 0 )); then",
    "                for v in \"${c[@]}\"; do",
    "                    if [[ $v == \"$3\"* ]]; then",
    "                        reply+=( \"$2$v\" )",
    "                    fi",
    "                done",
    "            fi",
    "            ;;",
    "        f|d)",
    "            files=1",
    "            while IFS= read -r v; do",
    "                reply+=( \"$2$v\" )",
    "            done < <(compgen -\"$1\" -- \"$3\")",
    "            ;;",
    "    esac",
    "}",
    "__djarg_@@_values() {",
    "    # the values option `$1` takes that start with `$3`, after `$2`",
    "    local n x k c=()",
    "    _djarg_@@_info \"$1\"",
    "    if [[ $k == c ]]; then",
    "        _djarg_@@_vals \"$1\"",
    "    fi",
    "    __djarg_@@_kind \"$k\" \"$2\" \"$3\"",
    "}",
    "__djarg_@@_names() {",
    "    # the names list `$1` gives that start with `$2`, each once:",
    "    # at the command and, for options falling through, above it",
    "    local L=$cmd c v p f m seen==",
    "    while :; do",
    "        c=()",
    "        \"$1\" \"$L\"",
    "        if (( ${#c[@]} > 0 )); then",
    "            for v in \"${c[@]}\"; do",
    "                if [[ $v == \"$2\"* ]] && [[ $seen != *\"=$v=\"* ]]; then",
    "                    seen=$seen$v=",
    "                    reply+=( \"$v\" )",
    "                fi",
    "            done",
    "        fi",
    "        _djarg_@@_cmd \"$L\"",
    "        if [[ $1 == *_opts ]] && [[ $f == *f* ]] && (( L != 0 ))",
    "        then",
    "            L=$p",
    "        else",
    "            return",
    "        fi",
    "    done",
    "}",
    "__djarg_@@_reply() {",
    "    # COMPREPLY: the reply as readline replaces the word -- less",
    "    # what precedes the last word break, and escaped unless quoted",
    "    # or completed as file names",
    "    local s=$tail ch pre v q",
    "    while [[ -n $s ]]; do",
    "        ch=${s:0:1} s=${s:1}",
    "        case $COMP_WORDBREAKS in",
    "            *\"$ch\"*) ;;",
    "            *) break ;;",
    "        esac",
    "        if [[ -z $s ]]; then",
    "            tail=",
    "        fi",
    "    done",
    "    __djarg_@@_unq \"${words[cword]%\"$tail\"}\"",
    "    pre=$r",
    "    COMPREPLY=()",
    "    if (( ${#reply[@]} > 0 )); then",
    "        for v in \"${reply[@]}\"; do",
    "            if [[ $v != \"$pre\"* ]]; then",
    "                continue",
    "            fi",
    "            v=${v#\"$pre\"}",
    "            case $tail in",
    "                \\'*|\\\"*) ;;",
    "                *)",
    "                    if (( ! files )); then",
    "                        printf -v q '%q' \"$v\"",
    "                        v=$q",
    "                    fi",
    "                    ;;",
    "            esac",
    "            COMPREPLY+=( \"$v\" )",
    "        done",
    "    fi",
    "    if (( files )) && type compopt >/dev/null 2>&1; then",
    "        compopt -o filenames",
    "    fi",
    "}",
    "__djarg_@@_offer() {",
    "    # reply: what may come at the cursor, `$cur`",
    "    if (( need > 0 )); then",
    "        __djarg_@@_values \"$opt\" \"\" \"$cur\"",
    "        return",
    "    fi",
    "    if (( only )); then",
    "        __djarg_@@_take 0",
    "        if [[ -n $o ]]; then",
    "            __djarg_@@_values \"$o\" \"\" \"$cur\"",
    "        fi",
    "        return",
    "    fi",
    "    if [[ -n $opt ]]; then",
    "        __djarg_@@_values \"$opt\" \"\" \"$cur\"",
    "    fi",
    "    case $cur in",
    "        --*=*)",
    "            __djarg_@@_find \"${cur%%=*}\"",
    "            if [[ -n $o ]]; then",
    "                __djarg_@@_values \"$o\" \"${cur%%=*}=\" \"${cur#*=}\"",
    "            fi",
    "            ;;",
    "        -*)",
    "            __djarg_@@_names _djarg_@@_opts \"$cur\"",
    "            ;;",
    "        *)",
    "            __djarg_@@_take 0",
    "            if [[ -n $o ]]; then",
    "                __djarg_@@_values \"$o\" \"\" \"$cur\"",
    "            elif [[ $extra == *e* ]]; then",
    "                __djarg_@@_kind f \"\" \"$cur\"",
    "            fi",
    "            _djarg_@@_cmd \"$cmd\"",
    "            c=()",
    "            if (( m == 0 )) || (( ${used[$cmd]:-0} < m )); then",
    "                _djarg_@@_subs \"$cmd\"",
    "            fi",
    "            if (( ${#c[@]} > 0 )); then",
    "                __djarg_@@_names _djarg_@@_subs \"$cur\"",
    "            elif [[ -z $o ]] && [[ $extra != *e* ]]; then",
    "                __djarg_@@_names _djarg_@@_opts \"$cur\"",
    "            fi",
    "            ;;",
    "    esac",
    "}",
    "_djarg_@@() {",
    "    local words cword tail cur r t l o p f m n x k cf w i",
    "    local cmd=0 opt= need=0 room=0 only=0 extra= files=0 nocase=0",
    "    local -a used counts reply c",
    "    if shopt -q nocasematch; then",
    "        nocase=1",
    "        shopt -u nocasematch",
    "    fi",
    "    __djarg_@@_words",
    "    _djarg_@@_cmd 0",
    "    cf=$f",
    "    i=1",
    "    while (( i < cword )); do",
    "        __djarg_@@_unq \"${words[i]}\"",
    "        w=$r",
    "        i=$(( i + 1 ))",
    "        if (( need > 0 )); then",
    "            need=$(( need - 1 ))",
    "            if (( need == 0 )) && (( room == 0 )); then",
    "                opt=",
    "            fi",
    "            continue",
    "        fi",
    "        if (( only )); then",
    "            __djarg_@@_take 1",
    "            continue",
    "        fi",
    "        if [[ -n $opt ]]; then",
    "            if [[ $w == -- ]]; then",
    "                opt=",
    "                continue",
    "            fi",
    "            __djarg_@@_class \"$w\"",
    "            if [[ $r == none ]] && (( room != 0 )); then",
    "                if (( room > 0 )); then",
    "                    room=$(( room - 1 ))",
    "                fi",
    "                if (( room == 0 )); then",
    "                    opt=",
    "                fi",
    "                continue",
    "            fi",
    "            opt=",
    "        fi",
    "        __djarg_@@_step \"$w\"",
    "    done",
    "    __djarg_@@_unq \"${words[cword]}\"",
    "    cur=$r",
    "    reply=()",
    "    __djarg_@@_offer",
    "    __djarg_@@_reply",
    "    if (( nocase )); then",
    "        shopt -s nocasematch",
    "    fi",
    "}"
};

// ENGINE_ZSH
//   constant: zsh's own functions, as completion/eng/zsh.sh holds it -- the
// source the library's array is written from -- each `@@` standing for the
// program's identifier.
static const char* const ENGINE_ZSH[] =
{
    "__djarg_@@_values() {",
    "    # offers the values option `$1` takes",
    "    local n x k c=()",
    "    _djarg_@@_info \"$1\"",
    "    case $k in",
    "        c)",
    "            _djarg_@@_vals \"$1\"",
    "            compadd -a c",
    "            ;;",
    "        f)",
    "            _files",
    "            ;;",
    "        d)",
    "            _files -/",
    "            ;;",
    "    esac",
    "}",
    "__djarg_@@_names() {",
    "    # offers the names list `$1` gives, each once with its",
    "    # description, as `$2` and `$3`: at the command and, for",
    "    # options falling through, above it",
    "    local L=$cmd c d p f m i",
    "    local -a specs",
    "    local -A seen",
    "    while :; do",
    "        c=() d=()",
    "        \"$1\" \"$L\"",
    "        i=1",
    "        while (( i <= ${#c[@]} )); do",
    "            if (( ! ${+seen[${c[i]}]} )); then",
    "                seen[${c[i]}]=1",
    "                specs+=( \"${c[i]//:/\\\\:}:${d[i]}\" )",
    "            fi",
    "            i=$(( i + 1 ))",
    "        done",
    "        _djarg_@@_cmd \"$L\"",
    "        if [[ $1 == *_opts ]] && [[ $f == *f* ]] && (( L != 0 ))",
    "        then",
    "            L=$p",
    "        else",
    "            break",
    "        fi",
    "    done",
    "    _describe -t \"$2\" \"$3\" specs",
    "}",
    "__djarg_@@_offer() {",
    "    # offers what may come at the cursor, `$cur`",
    "    if (( need > 0 )); then",
    "        __djarg_@@_values \"$opt\"",
    "        return",
    "    fi",
    "    if (( only )); then",
    "        __djarg_@@_take 0",
    "        if [[ -n $o ]]; then",
    "            __djarg_@@_values \"$o\"",
    "        fi",
    "        return",
    "    fi",
    "    if [[ -n $opt ]]; then",
    "        __djarg_@@_values \"$opt\"",
    "    fi",
    "    case $cur in",
    "        --*=*)",
    "            __djarg_@@_find \"${cur%%=*}\"",
    "            if [[ -n $o ]]; then",
    "                compset -P 1 '*='",
    "                __djarg_@@_values \"$o\"",
    "            fi",
    "            ;;",
    "        -*)",
    "            __djarg_@@_names _djarg_@@_opts options option",
    "            ;;",
    "        *)",
    "            __djarg_@@_take 0",
    "            if [[ -n $o ]]; then",
    "                __djarg_@@_values \"$o\"",
    "            elif [[ $extra == *e* ]]; then",
    "                _files",
    "            fi",
    "            _djarg_@@_cmd \"$cmd\"",
    "            c=()",
    "            if (( m == 0 )) || (( ${used[$cmd]:-0} < m )); then",
    "                _djarg_@@_subs \"$cmd\"",
    "            fi",
    "            if (( ${#c[@]} > 0 )); then",
    "                __djarg_@@_names _djarg_@@_subs commands command",
    "            elif [[ -z $o ]] && [[ $extra != *e* ]]; then",
    "                __djarg_@@_names _djarg_@@_opts options option",
    "            fi",
    "            ;;",
    "    esac",
    "}",
    "_djarg_@@() {",
    "    local cur r t l o p f m n x k cf w i d",
    "    local cmd=0 opt= need=0 room=0 only=0 extra=",
    "    local -a c",
    "    local -A used counts",
    "    _djarg_@@_cmd 0",
    "    cf=$f",
    "    i=2",
    "    while (( i < CURRENT )); do",
    "        w=${(Q)words[i]}",
    "        i=$(( i + 1 ))",
    "        if (( need > 0 )); then",
    "            need=$(( need - 1 ))",
    "            if (( need == 0 )) && (( room == 0 )); then",
    "                opt=",
    "            fi",
    "            continue",
    "        fi",
    "        if (( only )); then",
    "            __djarg_@@_take 1",
    "            continue",
    "        fi",
    "        if [[ -n $opt ]]; then",
    "            if [[ $w == -- ]]; then",
    "                opt=",
    "                continue",
    "            fi",
    "            __djarg_@@_class \"$w\"",
    "            if [[ $r == none ]] && (( room != 0 )); then",
    "                if (( room > 0 )); then",
    "                    room=$(( room - 1 ))",
    "                fi",
    "                if (( room == 0 )); then",
    "                    opt=",
    "                fi",
    "                continue",
    "            fi",
    "            opt=",
    "        fi",
    "        __djarg_@@_step \"$w\"",
    "    done",
    "    cur=${(Q)PREFIX}",
    "    __djarg_@@_offer",
    "}"
};

// ENGINE_FISH
//   constant: fish's functions, as completion/eng/fish.fish holds it -- the
// source the library's array is written from -- each `@@` standing for the
// program's identifier.
static const char* const ENGINE_FISH[] =
{
    "function __djarg_@@_row",
    "    # the fields of row `$argv[2]` of table `$argv[1]`",
    "    set -l table __djarg_@@_$argv[1]",
    "    string split ' ' -- $$table[1][(math $argv[2] + 1)]",
    "end",
    "",
    "function __djarg_@@_first",
    "    # whether `$argv[1]` may begin a name",
    "    string match -qr -- '^[^\\-!\\s\\x00-\\x1f\\x7f]' \"$argv[1]\"",
    "end",
    "",
    "function __djarg_@@_unq",
    "    # the token `$argv[1]` with its quotes and escapes removed",
    "    set -l s \"$argv[1]\"",
    "    if string match -qr -- '^\\'[^\\']*$|^\"[^\"]*$' \"$s\"",
    "        string sub -s 2 -- \"$s\"",
    "    else if not string unescape -- \"$s\"",
    "        printf '%s\\n' \"$s\"",
    "    end",
    "end",
    "",
    "function __djarg_@@_find -S",
    "    # o: the option named `$argv[1]` at the command or, falling",
    "    # through, above it",
    "    set -l L $cmd",
    "    set o",
    "    while true",
    "        set -l i (contains -i -- \"$L:$argv[1]\" $__djarg_@@_ok)",
    "        if test -n \"$i\"",
    "            set o $__djarg_@@_ov[$i]",
    "            return",
    "        end",
    "        set -l c (__djarg_@@_row cmds $L)",
    "        if string match -q -- '*f*' $c[2]; and test $L -ne 0",
    "            set L $c[1]",
    "        else",
    "            return",
    "        end",
    "    end",
    "end",
    "",
    "function __djarg_@@_target -S",
    "    # t: the subcommand `$argv[1]` names, from the command up, and",
    "    # l: the command it is under",
    "    set -l L $cmd",
    "    set t",
    "    set l",
    "    while true",
    "        set -l c (__djarg_@@_row cmds $L)",
    "        set -l u $used[(math $L + 1)]",
    "        if test -z \"$u\"",
    "            set u 0",
    "        end",
    "        if test $c[3] -eq 0; or test $u -lt $c[3]",
    "            or not string match -q -- '*s*' $c[2]",
    "            set -l i (contains -i -- \"$L:$argv[1]\" $__djarg_@@_sk)",
    "            if test -n \"$i\"",
    "                set t $__djarg_@@_st[$i]",
    "                set l $L",
    "                return",
    "            end",
    "        end",
    "        if string match -q -- '*s*' $c[2]; and test $L -ne 0",
    "            set L $c[1]",
    "        else",
    "            return",
    "        end",
    "    end",
    "end",
    "",
    "function __djarg_@@_class -S",
    "    # r: what `$argv[1]` is to the command -- mark, sub, long,",
    "    # short, win, term or none",
    "    set -l w \"$argv[1]\"",
    "    set r none",
    "    if test \"$w\" = --",
    "        set r mark",
    "        return",
    "    end",
    "    __djarg_@@_target \"$w\"",
    "    if test -n \"$t\"",
    "        set r sub",
    "        return",
    "    end",
    "    if string match -q -- '--?*' \"$w\"",
    "        if __djarg_@@_first (string sub -s 3 -l 1 -- \"$w\")",
    "            set r long",
    "        end",
    "        return",
    "    end",
    "    if string match -q -- '-?*' \"$w\"",
    "        set -l first (string sub -s 2 -l 1 -- \"$w\")",
    "        if not __djarg_@@_first \"$first\"",
    "            return",
    "        end",
    "        if string match -qr -- '^-([0-9]|\\.[0-9])' \"$w\"",
    "            and not contains -- \"$cmd:-$first\" $__djarg_@@_ok",
    "            return",
    "        end",
    "        set r short",
    "        return",
    "    end",
    "    if string match -q -- '*w*' $cf; and string match -q -- '/?*' \"$w\"",
    "        and __djarg_@@_first (string sub -s 2 -l 1 -- \"$w\")",
    "        set r win",
    "    else if test \"$w\" = ++; and test $cmd -ne 0",
    "        set r term",
    "    end",
    "end",
    "",
    "function __djarg_@@_take -S",
    "    # o: the positional that takes a word, from the command up,",
    "    # counted when `$argv[1]` is 1; extra: the last command's flags",
    "    set -l L $cmd",
    "    set o",
    "    while true",
    "        set -l s 0",
    "        while true",
    "            set -l i (contains -i -- \"$L:$s\" $__djarg_@@_pk)",
    "            if test -z \"$i\"",
    "                break",
    "            end",
    "            set -l e (string split ' ' -- $__djarg_@@_pv[$i])",
    "            set -l n $counts[(math $e[1] + 1)]",
    "            if test -z \"$n\"",
    "                set n 0",
    "            end",
    "            if test $e[2] -lt 0; or test $n -lt $e[2]",
    "                set o $e[1]",
    "                if test $argv[1] -eq 1",
    "                    set counts[(math $e[1] + 1)] (math $n + 1)",
    "                end",
    "                return",
    "            end",
    "            set s (math $s + 1)",
    "        end",
    "        set -l c (__djarg_@@_row cmds $L)",
    "        if string match -q -- '*f*' $c[2]; and test $L -ne 0",
    "            set L $c[1]",
    "        else",
    "            set extra $c[2]",
    "            return",
    "        end",
    "    end",
    "end",
    "",
    "function __djarg_@@_given -S",
    "    # the option found, met with the value `$argv[1]` -- empty for",
    "    # none: what it still needs, and what more it may take",
    "    if test -z \"$o\"",
    "        return",
    "    end",
    "    set -l e (__djarg_@@_row info $o)",
    "    if test $e[2] -eq 0",
    "        return",
    "    end",
    "    set -l got 0",
    "    if test -n \"$argv[1]\"",
    "        set got 1",
    "    end",
    "    set need (math $e[1] - $got)",
    "    if test $need -lt 0",
    "        set need 0",
    "    end",
    "    if test $e[2] -lt 0",
    "        set room -1",
    "    else if test $got -gt $e[1]",
    "        set room (math $e[2] - $got)",
    "    else",
    "        set room (math $e[2] - $e[1])",
    "    end",
    "    if test $need -gt 0; or test $room -ne 0",
    "        set opt $o",
    "    end",
    "end",
    "",
    "function __djarg_@@_step -S",
    "    # one word the parse meets",
    "    set -l w \"$argv[1]\"",
    "    set -l r",
    "    set -l t",
    "    set -l l",
    "    set -l o",
    "    __djarg_@@_class \"$w\"",
    "    switch $r",
    "        case mark",
    "            set only 1",
    "        case sub",
    "            set -l u $used[(math $l + 1)]",
    "            if test -z \"$u\"",
    "                set u 0",
    "            end",
    "            set used[(math $l + 1)] (math $u + 1)",
    "            set cmd $t",
    "            set cf (__djarg_@@_row cmds $cmd)[2]",
    "        case term",
    "            set cmd (__djarg_@@_row cmds $cmd)[1]",
    "            set cf (__djarg_@@_row cmds $cmd)[2]",
    "        case long win",
    "            set -l at =",
    "            if test $r = win",
    "                set at :",
    "            end",
    "            set -l name (string split -m 1 -- $at \"$w\")",
    "            set -l value \"$name[2]\"",
    "            if test $r = long",
    "                __djarg_@@_find \"$name[1]\"",
    "            else",
    "                set name (string sub -s 2 -- \"$name[1]\")",
    "                __djarg_@@_find \"--$name\"",
    "                if test -z \"$o\"",
    "                    __djarg_@@_find \"-$name\"",
    "                end",
    "            end",
    "            __djarg_@@_given \"$value\"",
    "        case short",
    "            set -l rest (string sub -s 2 -- \"$w\")",
    "            while test -n \"$rest\"",
    "                set -l name -(string sub -l 1 -- \"$rest\")",
    "                set rest (string sub -s 2 -- \"$rest\")",
    "                __djarg_@@_find \"$name\"",
    "                if test -z \"$o\"",
    "                    break",
    "                end",
    "                if test (__djarg_@@_row info $o)[2] -ne 0",
    "                    __djarg_@@_given \"$rest\"",
    "                    break",
    "                end",
    "            end",
    "        case '*'",
    "            __djarg_@@_take 1",
    "    end",
    "end",
    "",
    "function __djarg_@@_kind",
    "    # prints the values of kind `$argv[1]` -- c, choices",
    "    # `$argv[4]` on; f, files; d, directories -- after `$argv[2]`,",
    "    # for the text `$argv[3]`",
    "    switch $argv[1]",
    "        case c",
    "            for v in $argv[4..-1]",
    "                printf '%s%s\\n' \"$argv[2]\" \"$v\"",
    "            end",
    "        case f",
    "            set -l w (string escape -- \"$argv[3]\")",
    "            for v in (__fish_complete_path \"$w\")",
    "                printf '%s%s\\n' \"$argv[2]\" \"$v\"",
    "            end",
    "        case d",
    "            set -l w (string escape -- \"$argv[3]\")",
    "            for v in (__fish_complete_directories \"$w\" '')",
    "                printf '%s%s\\n' \"$argv[2]\" \"$v\"",
    "            end",
    "    end",
    "end",
    "",
    "function __djarg_@@_values",
    "    # prints the values option `$argv[1]` takes, after `$argv[2]`,",
    "    # for the text `$argv[3]`",
    "    set -l e (__djarg_@@_row info $argv[1])",
    "    set -l v",
    "    if test $e[3] = c; and test $e[5] -gt 0",
    "        set v $__djarg_@@_vv[$e[4]..(math $e[4] + $e[5] - 1)]",
    "    end",
    "    __djarg_@@_kind $e[3] \"$argv[2]\" \"$argv[3]\" $v",
    "end",
    "",
    "function __djarg_@@_names -S",
    "    # prints the names of kind `$argv[1]` -- s, subcommands; o,",
    "    # options -- each once with its description: at the command",
    "    # and, for options falling through, above it",
    "    set -l L $cmd",
    "    set -l seen",
    "    while true",
    "        set -l c (__djarg_@@_row cmds $L)",
    "        set -l i $c[4]",
    "        set -l z (math $c[4] + $c[5])",
    "        set -l names __djarg_@@_sn",
    "        set -l texts __djarg_@@_sd",
    "        if test $argv[1] = o",
    "            set i $c[6]",
    "            set z (math $c[6] + $c[7])",
    "            set names __djarg_@@_on",
    "            set texts __djarg_@@_od",
    "        end",
    "        while test $i -lt $z",
    "            set -l name $$names[1][$i]",
    "            if not contains -- \"$name\" $seen",
    "                set -a seen \"$name\"",
    "                printf '%s\\t%s\\n' \"$name\" $$texts[1][$i]",
    "            end",
    "            set i (math $i + 1)",
    "        end",
    "        if test $argv[1] = o; and string match -q -- '*f*' $c[2]",
    "            and test $L -ne 0",
    "            set L $c[1]",
    "        else",
    "            return",
    "        end",
    "    end",
    "end",
    "",
    "function __djarg_@@_offer -S",
    "    # prints what may come at the cursor, `$cur`",
    "    if test $need -gt 0",
    "        __djarg_@@_values $opt '' \"$cur\"",
    "        return",
    "    end",
    "    if test $only -eq 1",
    "        __djarg_@@_take 0",
    "        if test -n \"$o\"",
    "            __djarg_@@_values $o '' \"$cur\"",
    "        end",
    "        return",
    "    end",
    "    if test -n \"$opt\"",
    "        __djarg_@@_values $opt '' \"$cur\"",
    "    end",
    "    if string match -q -- '--*=*' \"$cur\"",
    "        set -l name (string split -m 1 -- = \"$cur\")",
    "        __djarg_@@_find \"$name[1]\"",
    "        if test -n \"$o\"",
    "            __djarg_@@_values $o \"$name[1]=\" \"$name[2]\"",
    "        end",
    "    else if string match -q -- '-*' \"$cur\"",
    "        __djarg_@@_names o",
    "    else",
    "        __djarg_@@_take 0",
    "        if test -n \"$o\"",
    "            __djarg_@@_values $o '' \"$cur\"",
    "        else if string match -q -- '*e*' \"$extra\"",
    "            __djarg_@@_kind f '' \"$cur\"",
    "        end",
    "        set -l c (__djarg_@@_row cmds $cmd)",
    "        set -l u $used[(math $cmd + 1)]",
    "        if test -z \"$u\"",
    "            set u 0",
    "        end",
    "        if test $c[5] -gt 0",
    "            and begin",
    "                test $c[3] -eq 0; or test $u -lt $c[3]",
    "            end",
    "            __djarg_@@_names s",
    "        else if test -z \"$o\"",
    "            and not string match -q -- '*e*' \"$extra\"",
    "            __djarg_@@_names o",
    "        end",
    "    end",
    "end",
    "",
    "function __djarg_@@_complete",
    "    # prints the completions for the command line",
    "    set -l tokens (commandline -opc)",
    "    set -l cur (__djarg_@@_unq (commandline -ct))",
    "    set -l cmd 0",
    "    set -l cf (__djarg_@@_row cmds 0)[2]",
    "    set -l opt",
    "    set -l need 0",
    "    set -l room 0",
    "    set -l only 0",
    "    set -l extra",
    "    set -l used",
    "    set -l counts",
    "    set -l r",
    "    set -l t",
    "    set -l l",
    "    set -l o",
    "    for w in $tokens[2..-1]",
    "        if test $need -gt 0",
    "            set need (math $need - 1)",
    "            if test $need -eq 0; and test $room -eq 0",
    "                set opt",
    "            end",
    "            continue",
    "        end",
    "        if test $only -eq 1",
    "            __djarg_@@_take 1",
    "            continue",
    "        end",
    "        if test -n \"$opt\"",
    "            if test \"$w\" = --",
    "                set opt",
    "                continue",
    "            end",
    "            __djarg_@@_class \"$w\"",
    "            if test $r = none; and test $room -ne 0",
    "                if test $room -gt 0",
    "                    set room (math $room - 1)",
    "                end",
    "                if test $room -eq 0",
    "                    set opt",
    "                end",
    "                continue",
    "            end",
    "            set opt",
    "        end",
    "        __djarg_@@_step \"$w\"",
    "    end",
    "    __djarg_@@_offer",
    "end"
};

// ENGINE_PWSH
//   constant: the PowerShell block's own part, as completion/eng/pwsh.ps1 holds
// it -- the source the library's array is written from -- each `@@` standing
// for the program's identifier.
static const char* const ENGINE_PWSH[] =
{
    "    $ordinal = [System.StringComparer]::Ordinal",
    "    $sk = [Collections.Generic.Dictionary[string,int]]::new(",
    "        $ordinal)",
    "    for ($i = 0; $i -lt $sn.Count; $i++) {",
    "        $sk[[string]$sc[$i] + ':' + $sn[$i]] = $st[$i]",
    "    }",
    "    $ok = [Collections.Generic.Dictionary[string,int]]::new(",
    "        $ordinal)",
    "    for ($i = 0; $i -lt $on.Count; $i++) {",
    "        $ok[[string]$oc[$i] + ':' + $on[$i]] = $ov[$i]",
    "    }",
    "    $pk = [Collections.Generic.Dictionary[string,string]]::new(",
    "        $ordinal)",
    "    for ($i = 0; $i -lt $ps.Count; $i++) {",
    "        $pk[$ps[$i]] = $pv[$i]",
    "    }",
    "    $complete = {",
    "        param($wordToComplete, $commandAst, $cursorPosition)",
    "        $s = @{ cmd = 0; cf = ''; opt = -1; need = 0; room = 0",
    "                only = $false; extra = ''; used = @{}; counts = @{} }",
    "        $ord = [System.StringComparison]::Ordinal",
    "        $any = [System.StringComparison]::OrdinalIgnoreCase",
    "        $completers = [Management.Automation.CompletionCompleters]",
    "        $result = [Management.Automation.CompletionResult]",
    "        $constant = ('System.Management.Automation.Language.' +",
    "                     'StringConstantExpressionAst') -as [type]",
    "        function Cmd([int]$L) {",
    "            # the command's row",
    "            $e = $cmds[$L].Split(' ')",
    "            @{ p = [int]$e[0]; f = $e[1]; m = [int]$e[2]",
    "               sa = [int]$e[3]; sc = [int]$e[4]",
    "               oa = [int]$e[5]; oc = [int]$e[6] }",
    "        }",
    "        function Info([int]$o) {",
    "            # the option's row",
    "            $e = $info[$o].Split(' ')",
    "            @{ n = [int]$e[0]; x = [int]$e[1]; k = $e[2]",
    "               va = [int]$e[3]; vc = [int]$e[4] }",
    "        }",
    "        function First([string]$c) {",
    "            # whether the character may begin a name",
    "            ($c.Length -gt 0) -and",
    "                ($c -cnotmatch '^[-!\\s\\x00-\\x1f\\x7f]')",
    "        }",
    "        function Used([int]$L) {",
    "            # the subcommands the command was given",
    "            if ($s.used.ContainsKey($L)) {",
    "                return $s.used[$L]",
    "            }",
    "            return 0",
    "        }",
    "        function Find([string]$name) {",
    "            # the option named at the command or, falling",
    "            # through, above it; -1 for none",
    "            $L = $s.cmd",
    "            while ($true) {",
    "                $v = 0",
    "                if ($ok.TryGetValue([string]$L + ':' + $name,",
    "                                    [ref]$v)) {",
    "                    return $v",
    "                }",
    "                $c = Cmd $L",
    "                if ($c.f.Contains('f') -and ($L -ne 0)) {",
    "                    $L = $c.p",
    "                }",
    "                else {",
    "                    return -1",
    "                }",
    "            }",
    "        }",
    "        function Target([string]$w) {",
    "            # the subcommand the word names, from the command up,",
    "            # and the command it is under; $null for none",
    "            $L = $s.cmd",
    "            while ($true) {",
    "                $c = Cmd $L",
    "                if (($c.m -eq 0) -or ((Used $L) -lt $c.m) -or",
    "                    (-not $c.f.Contains('s'))) {",
    "                    $t = 0",
    "                    if ($sk.TryGetValue([string]$L + ':' + $w,",
    "                                        [ref]$t)) {",
    "                        return @($t, $L)",
    "                    }",
    "                }",
    "                if ($c.f.Contains('s') -and ($L -ne 0)) {",
    "                    $L = $c.p",
    "                }",
    "                else {",
    "                    return $null",
    "                }",
    "            }",
    "        }",
    "        function Class([string]$w) {",
    "            # what the word is to the command",
    "            if ($w -ceq '--') {",
    "                return 'mark'",
    "            }",
    "            if ($null -ne (Target $w)) {",
    "                return 'sub'",
    "            }",
    "            if ($w.StartsWith('--', $ord)) {",
    "                if (($w.Length -gt 2) -and (First $w.Substring(2, 1))) {",
    "                    return 'long'",
    "                }",
    "                return 'none'",
    "            }",
    "            if ($w.StartsWith('-', $ord) -and ($w.Length -gt 1)) {",
    "                $first = $w.Substring(1, 1)",
    "                if (-not (First $first)) {",
    "                    return 'none'",
    "                }",
    "                if (($w -cmatch '^-([0-9]|\\.[0-9])') -and",
    "                    (-not $ok.ContainsKey([string]$s.cmd + ':-' +",
    "                                          $first))) {",
    "                    return 'none'",
    "                }",
    "                return 'short'",
    "            }",
    "            if ($s.cf.Contains('w') -and $w.StartsWith('/', $ord) -and",
    "                ($w.Length -gt 1) -and (First $w.Substring(1, 1))) {",
    "                return 'win'",
    "            }",
    "            if (($w -ceq '++') -and ($s.cmd -ne 0)) {",
    "                return 'term'",
    "            }",
    "            return 'none'",
    "        }",
    "        function Take([bool]$count) {",
    "            # the positional that takes a word, from the command",
    "            # up, counted when asked; -1 for none",
    "            $L = $s.cmd",
    "            while ($true) {",
    "                $slot = 0",
    "                while ($true) {",
    "                    $e = $null",
    "                    if (-not $pk.TryGetValue([string]$L + ':' + $slot,",
    "                                             [ref]$e)) {",
    "                        break",
    "                    }",
    "                    $e = $e.Split(' ')",
    "                    $o = [int]$e[0]",
    "                    $c = 0",
    "                    if ($s.counts.ContainsKey($o)) {",
    "                        $c = $s.counts[$o]",
    "                    }",
    "                    if (([int]$e[1] -lt 0) -or ($c -lt [int]$e[1])) {",
    "                        if ($count) {",
    "                            $s.counts[$o] = $c + 1",
    "                        }",
    "                        return $o",
    "                    }",
    "                    $slot++",
    "                }",
    "                $c = Cmd $L",
    "                if ($c.f.Contains('f') -and ($L -ne 0)) {",
    "                    $L = $c.p",
    "                }",
    "                else {",
    "                    $s.extra = $c.f",
    "                    return -1",
    "                }",
    "            }",
    "        }",
    "        function Given([int]$o, [string]$value) {",
    "            # the option met with a value -- empty for none: what",
    "            # it still needs, and what more it may take",
    "            if ($o -lt 0) {",
    "                return",
    "            }",
    "            $i = Info $o",
    "            if ($i.x -eq 0) {",
    "                return",
    "            }",
    "            $got = 0",
    "            if ($value.Length -gt 0) {",
    "                $got = 1",
    "            }",
    "            $s.need = [Math]::Max(0, $i.n - $got)",
    "            if ($i.x -lt 0) {",
    "                $s.room = -1",
    "            }",
    "            elseif ($got -gt $i.n) {",
    "                $s.room = $i.x - $got",
    "            }",
    "            else {",
    "                $s.room = $i.x - $i.n",
    "            }",
    "            if (($s.need -gt 0) -or ($s.room -ne 0)) {",
    "                $s.opt = $o",
    "            }",
    "        }",
    "        function Named([string]$w, [string]$at) {",
    "            # a long or Windows-style name met, with its value",
    "            $i = $w.IndexOf($at)",
    "            $name = $w",
    "            $value = ''",
    "            if ($i -ge 0) {",
    "                $name = $w.Substring(0, $i)",
    "                $value = $w.Substring($i + 1)",
    "            }",
    "            if ($at -ceq '=') {",
    "                $o = Find $name",
    "            }",
    "            else {",
    "                $o = Find ('--' + $name.Substring(1))",
    "                if ($o -lt 0) {",
    "                    $o = Find ('-' + $name.Substring(1))",
    "                }",
    "            }",
    "            Given $o $value",
    "        }",
    "        function Step([string]$w) {",
    "            # one word the parse meets",
    "            switch (Class $w) {",
    "                'mark' {",
    "                    $s.only = $true",
    "                }",
    "                'sub' {",
    "                    $t = Target $w",
    "                    $s.used[$t[1]] = (Used $t[1]) + 1",
    "                    $s.cmd = $t[0]",
    "                    $s.cf = (Cmd $s.cmd).f",
    "                }",
    "                'term' {",
    "                    $s.cmd = (Cmd $s.cmd).p",
    "                    $s.cf = (Cmd $s.cmd).f",
    "                }",
    "                'long' {",
    "                    Named $w '='",
    "                }",
    "                'win' {",
    "                    Named $w ':'",
    "                }",
    "                'short' {",
    "                    $rest = $w.Substring(1)",
    "                    while ($rest.Length -gt 0) {",
    "                        $o = Find ('-' + $rest.Substring(0, 1))",
    "                        $rest = $rest.Substring(1)",
    "                        if ($o -lt 0) {",
    "                            break",
    "                        }",
    "                        if ((Info $o).x -ne 0) {",
    "                            Given $o $rest",
    "                            break",
    "                        }",
    "                    }",
    "                }",
    "                default {",
    "                    $null = Take $true",
    "                }",
    "            }",
    "        }",
    "        function Result([string]$text, [string]$pre, [string]$type,",
    "                        [string]$tip) {",
    "            # a completion of the text, quoted where it must be",
    "            $q = $text",
    "            if (($q.Length -eq 0) -or",
    "                ($q -match '[\\s''\"`$(){}<>;,&|@#\\u2018-\\u201b]')) {",
    "                $quotes = '[''\\u2018-\\u201b]'",
    "                $q = \"'\" + ($q -replace $quotes, '$0$0') + \"'\"",
    "            }",
    "            if ([string]::IsNullOrEmpty($tip)) {",
    "                $tip = $text",
    "            }",
    "            $list = $text",
    "            if ($list.Length -eq 0) {",
    "                $list = \"''\"",
    "            }",
    "            $result::new($pre + $q, $list, $type, $tip)",
    "        }",
    "        function Kind([string]$k, [int]$va, [int]$vc, [string]$pre,",
    "                      [string]$word) {",
    "            # the values of a kind -- c, choices; f, files; d,",
    "            # directories -- that start with the word, after $pre",
    "            if ($k -ceq 'c') {",
    "                for ($i = $va; $i -lt $va + $vc; $i++) {",
    "                    if ($vv[$i].StartsWith($word, $any)) {",
    "                        Result $vv[$i] $pre 'ParameterValue' ''",
    "                    }",
    "                }",
    "            }",
    "            elseif (($k -ceq 'f') -or ($k -ceq 'd')) {",
    "                $all = $completers::CompleteFilename($word)",
    "                foreach ($r in $all) {",
    "                    if (($k -ceq 'f') -or",
    "                        ($r.ResultType -eq 'ProviderContainer')) {",
    "                        $result::new($pre + $r.CompletionText,",
    "                                     $r.ListItemText, $r.ResultType,",
    "                                     $r.ToolTip)",
    "                    }",
    "                }",
    "            }",
    "        }",
    "        function Values([int]$o, [string]$pre, [string]$word) {",
    "            # the values the option takes",
    "            $i = Info $o",
    "            Kind $i.k $i.va $i.vc $pre $word",
    "        }",
    "        function Names([string]$which, [string]$word) {",
    "            # the names -- s, subcommands; o, options -- that start",
    "            # with the word, each once with its description: at the",
    "            # command and, for options falling through, above it",
    "            $L = $s.cmd",
    "            $seen = @{}",
    "            while ($true) {",
    "                $c = Cmd $L",
    "                $a = $c.sa",
    "                $z = $c.sa + $c.sc",
    "                $names = $sn",
    "                $texts = $sd",
    "                $type = 'Command'",
    "                if ($which -ceq 'o') {",
    "                    $a = $c.oa",
    "                    $z = $c.oa + $c.oc",
    "                    $names = $on",
    "                    $texts = $od",
    "                    $type = 'ParameterName'",
    "                }",
    "                for ($i = $a; $i -lt $z; $i++) {",
    "                    $name = $names[$i]",
    "                    if ((-not $seen.ContainsKey($name)) -and",
    "                        $name.StartsWith($word, $any)) {",
    "                        $seen[$name] = 1",
    "                        Result $name '' $type $texts[$i]",
    "                    }",
    "                }",
    "                if (($which -ceq 'o') -and $c.f.Contains('f') -and",
    "                    ($L -ne 0)) {",
    "                    $L = $c.p",
    "                }",
    "                else {",
    "                    return",
    "                }",
    "            }",
    "        }",
    "        function Offer([string]$cur) {",
    "            # what may come at the cursor",
    "            if ($s.need -gt 0) {",
    "                Values $s.opt '' $cur",
    "                return",
    "            }",
    "            if ($s.only) {",
    "                $o = Take $false",
    "                if ($o -ge 0) {",
    "                    Values $o '' $cur",
    "                }",
    "                return",
    "            }",
    "            if ($s.opt -ge 0) {",
    "                Values $s.opt '' $cur",
    "            }",
    "            if ($cur -cmatch '^--[^=]*=') {",
    "                $i = $cur.IndexOf('=')",
    "                $o = Find $cur.Substring(0, $i)",
    "                if ($o -ge 0) {",
    "                    $pre = $cur.Substring(0, $i + 1)",
    "                    Values $o $pre $cur.Substring($i + 1)",
    "                }",
    "            }",
    "            elseif ($cur.StartsWith('-', $ord)) {",
    "                Names 'o' $cur",
    "            }",
    "            else {",
    "                $o = Take $false",
    "                if ($o -ge 0) {",
    "                    Values $o '' $cur",
    "                }",
    "                elseif ($s.extra.Contains('e')) {",
    "                    Kind 'f' 0 0 '' $cur",
    "                }",
    "                $c = Cmd $s.cmd",
    "                if (($c.sc -gt 0) -and",
    "                    (($c.m -eq 0) -or ((Used $s.cmd) -lt $c.m))) {",
    "                    Names 's' $cur",
    "                }",
    "                elseif (($o -lt 0) -and (-not $s.extra.Contains('e'))) {",
    "                    Names 'o' $cur",
    "                }",
    "            }",
    "        }",
    "        function Main {",
    "            # the words to the cursor walked, then what may come",
    "            $words = @()",
    "            $elements = $commandAst.CommandElements",
    "            for ($i = 1; $i -lt $elements.Count; $i++) {",
    "                $e = $elements[$i]",
    "                if ($e.Extent.EndOffset -ge $cursorPosition) {",
    "                    break",
    "                }",
    "                if ($e -is $constant) {",
    "                    $words += , $e.Value",
    "                }",
    "                else {",
    "                    $words += , $e.Extent.Text",
    "                }",
    "            }",
    "            $s.cf = (Cmd 0).f",
    "            foreach ($w in $words) {",
    "                if ($s.need -gt 0) {",
    "                    $s.need--",
    "                    if (($s.need -eq 0) -and ($s.room -eq 0)) {",
    "                        $s.opt = -1",
    "                    }",
    "                    continue",
    "                }",
    "                if ($s.only) {",
    "                    $null = Take $true",
    "                    continue",
    "                }",
    "                if ($s.opt -ge 0) {",
    "                    if ($w -ceq '--') {",
    "                        $s.opt = -1",
    "                        continue",
    "                    }",
    "                    if (((Class $w) -ceq 'none') -and ($s.room -ne 0)) {",
    "                        if ($s.room -gt 0) {",
    "                            $s.room--",
    "                        }",
    "                        if ($s.room -eq 0) {",
    "                            $s.opt = -1",
    "                        }",
    "                        continue",
    "                    }",
    "                    $s.opt = -1",
    "                }",
    "                Step $w",
    "            }",
    "            $cur = $wordToComplete",
    "            if ($cur -cmatch \"^'(.*)'$\") {",
    "                $cur = $Matches[1] -replace \"''\", \"'\"",
    "            }",
    "            elseif ($cur -cmatch '^\"(.*)\"$') {",
    "                $cur = $Matches[1]",
    "            }",
    "            elseif ($cur -cmatch '^[''\"]') {",
    "                $cur = $cur.Substring(1)",
    "            }",
    "            Offer $cur",
    "        }",
    "        $results = @(Main)",
    "        if (($results.Count -eq 0) -and",
    "            ($wordToComplete.Length -gt 0)) {",
    "            # the word as it stands, so that PowerShell does not offer",
    "            # files where nothing fits",
    "            $w = $wordToComplete",
    "            $result::new($w, $w, 'ParameterValue', $w)",
    "        }",
    "        else {",
    "            $results",
    "        }",
    "    }.GetNewClosure()"
};

// ENGINE_WALK_COUNT and kin
//   constants: how many lines each engine holds, as constant expressions
// for ENGINES.
enum
{
    ENGINE_WALK_COUNT = (int)(sizeof(ENGINE_WALK) / sizeof(ENGINE_WALK[0])),
    ENGINE_BASH_COUNT = (int)(sizeof(ENGINE_BASH) / sizeof(ENGINE_BASH[0])),
    ENGINE_ZSH_COUNT  = (int)(sizeof(ENGINE_ZSH) / sizeof(ENGINE_ZSH[0])),
    ENGINE_FISH_COUNT = (int)(sizeof(ENGINE_FISH) / sizeof(ENGINE_FISH[0])),
    ENGINE_PWSH_COUNT = (int)(sizeof(ENGINE_PWSH) / sizeof(ENGINE_PWSH[0]))
};

// LINES_1
//   constant: lines whose marks a row replaces.
static const char* const LINES_1[] =
{
    "plain",
    ""
};

// LINES_2
//   constant: lines whose marks a row replaces.
static const char* const LINES_2[] =
{
    "__djarg_@@_first() {"
};

// LINES_3
//   constant: lines whose marks a row replaces.
static const char* const LINES_3[] =
{
    "_djarg_@@ x _djarg_@@"
};

// LINES_4
//   constant: lines whose marks a row replaces.
static const char* const LINES_4[] =
{
    "@@@"
};

// LINES_5
//   constant: lines whose marks a row replaces.
static const char* const LINES_5[] =
{
    "@@@@"
};

// LINES_6
//   constant: lines whose marks a row replaces.
static const char* const LINES_6[] =
{
    "@@@@@"
};

// LINES_7
//   constant: lines whose marks a row replaces.
static const char* const LINES_7[] =
{
    "@",
    "a@b",
    "@ @"
};

// LINES_8
//   constant: lines whose marks a row replaces.
static const char* const LINES_8[] =
{
    "@@start",
    "end@@",
    "@@"
};

// LINES_9
//   constant: lines whose marks a row replaces.
static const char* const LINES_9[] =
{
    "a@@b",
    "@@",
    "@@@@"
};

// LINES_10
//   constant: lines whose marks a row replaces.
static const char* const LINES_10[] =
{
    "<@@>"
};

// LINES_11
//   constant: lines whose marks a row replaces.
static const char* const LINES_11[] =
{
    "@@"
};

// LINES_12
//   constant: lines whose marks a row replaces.
static const char* const LINES_12[] =
{
    "first @@",
    "",
    "second @@ and @@",
    "third"
};

// CHOICES_1
//   constant: the values a listing validator lists.
static const char* const CHOICES_1[] =
{
    "red",
    "green",
    "blue",
    NULL
};

// CHOICES_2
//   constant: the values a listing validator lists.
static const char* const CHOICES_2[] =
{
    "dark",
    "light",
    NULL
};

// CHOICES_3
//   constant: the values a listing validator lists.
static const char* const CHOICES_3[] =
{
    "light",
    "pale",
    "dark",
    NULL
};

// CHOICES_4
//   constant: the values a listing validator lists.
static const char* const CHOICES_4[] =
{
    "a",
    NULL
};

// CHOICES_5
//   constant: the values a listing validator lists.
static const char* const CHOICES_5[] =
{
    "b",
    NULL
};

// CHOICES_6
//   constant: the values a listing validator lists.
static const char* const CHOICES_6[] =
{
    "x",
    NULL
};

// CHOICES_7
//   constant: the values a listing validator lists.
static const char* const CHOICES_7[] =
{
    "y",
    NULL
};

// CHOICES_8
//   constant: the values a listing validator lists.
static const char* const CHOICES_8[] =
{
    "z",
    NULL
};

// CHOICES_9
//   constant: the values a listing validator lists.
static const char* const CHOICES_9[] =
{
    NULL
};

// CHOICES_10
//   constant: the values a listing validator lists.
static const char* const CHOICES_10[] =
{
    "one",
    "one",
    "two",
    "one",
    NULL
};

// CHOICES_11
//   constant: the values a listing validator lists.
static const char* const CHOICES_11[] =
{
    "red",
    NULL
};

// CHOICES_12
//   constant: the values a listing validator lists.
static const char* const CHOICES_12[] =
{
    "never",
    NULL
};

// CHOICES_13
//   constant: the values a listing validator lists.
static const char* const CHOICES_13[] =
{
    "A",
    "a",
    "",
    NULL
};

// CHOICES_14
//   constant: the values a listing validator lists.
static const char* const CHOICES_14[] =
{
    "here",
    "there",
    NULL
};

// CHOICES_15
//   constant: the values a listing validator lists.
static const char* const CHOICES_15[] =
{
    "one",
    "two",
    NULL
};

// CHOICES_16
//   constant: the values a listing validator lists.
static const char* const CHOICES_16[] =
{
    "q",
    NULL
};

// CHOICES_17
//   constant: the values a listing validator lists.
static const char* const CHOICES_17[] =
{
    "p",
    NULL
};

// CHOICES_18
//   constant: the values a listing validator lists.
static const char* const CHOICES_18[] =
{
    "q",
    "p",
    NULL
};

// CHOICES_19
//   constant: the values a listing validator lists.
static const char* const CHOICES_19[] =
{
    "n",
    NULL
};

// CHOICES_20
//   constant: the values a listing validator lists.
static const char* const CHOICES_20[] =
{
    "in",
    NULL
};

// CHOICES_21
//   constant: the values a listing validator lists.
static const char* const CHOICES_21[] =
{
    "out",
    NULL
};

// CHOICES_22
//   constant: the values a listing validator lists.
static const char* const CHOICES_22[] =
{
    "l",
    NULL
};

// CHOICES_23
//   constant: the values a listing validator lists.
static const char* const CHOICES_23[] =
{
    "m",
    NULL
};

// CHOICES_24
//   constant: the values a listing validator lists.
static const char* const CHOICES_24[] =
{
    "1",
    "2",
    NULL
};

// CHOICES_25
//   constant: the values a listing validator lists.
static const char* const CHOICES_25[] =
{
    "low",
    "high",
    NULL
};

// CHOICES_26
//   constant: the values a listing validator lists.
static const char* const CHOICES_26[] =
{
    "never",
    "listed",
    NULL
};

// CHOICES_27
//   constant: the values a listing validator lists.
static const char* const CHOICES_27[] =
{
    "not",
    "listed",
    NULL
};

// CHOICES_28
//   constant: the values a listing validator lists.
static const char* const CHOICES_28[] =
{
    "s1",
    NULL
};

// CHOICES_29
//   constant: the values a listing validator lists.
static const char* const CHOICES_29[] =
{
    "x",
    "y",
    NULL
};

// CHOICES_30
//   constant: the values a listing validator lists.
static const char* const CHOICES_30[] =
{
    "it's",
    "back\\slash",
    "dollar$x",
    "tick`x",
    "quote\"d",
    "two words",
    "tab\there",
    "line\nbreak",
    "cr\rhere",
    "del\x7fhere",
    "caf\xc3\xa9",
    "\xe2\x86\x92",
    "\xf0\x9f\x98\x80",
    "semi;colon",
    "star*",
    "-dash",
    "",
    "\x01",
    "''",
    "\\",
    NULL
};

// CHOICES_31
//   constant: the values a listing validator lists.
static const char* const CHOICES_31[] =
{
    "\xff",
    "\xc3",
    "\xc3(",
    "\xe2\x82",
    "\xe2\x82(",
    "\xc0\x80",
    "\xed\xa0\x80",
    "\xf4\x90\x80\x80",
    "\xf0\x8f\xbf\xbf",
    "\xe0\x9f\xbf",
    "\xf5\x80",
    "\xf5\x80\x80\x80",
    "\xf4\x80\x80\x80",
    "\xef\xbf\xbf",
    "\xf4\x8f\xbf\xbf",
    "\xc2\x80",
    "\xdf\xbf",
    "\xe0\xa0\x80",
    "\xf0\x90\x80\x80",
    "a\x80" "b",
    NULL
};

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
// MEMBERS_1
//   constant: the members of a set.
static const char* const MEMBERS_1[] =
{
    "one",
    "two",
    "three"
};
#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
// MAPPING_1
//   constant: the pairs of a mapping.
static const struct d_arg_mapping MAPPING_1[] =
{
    { "low", "1" },
    { "high", "9" }
};
#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
// MAPPING_2
//   constant: the pairs of a mapping.
static const struct d_arg_mapping MAPPING_2[] =
{
    { "on", "yes" },
    { "off", "no" }
};
#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
// MEMBERS_2
//   constant: the members of a set.
static const char* const MEMBERS_2[] =
{
    "x",
    "y"
};
#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
// CHOICES_32
//   constant: the values a listing validator lists.
static const char* const CHOICES_32[] =
{
    "y",
    "z",
    NULL
};
#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
// MEMBERS_3
//   constant: the members of a set.
static const char* const MEMBERS_3[] =
{
    "1",
    "2"
};
#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
// MEMBERS_4
//   constant: the members of a set.
static const char* const MEMBERS_4[] =
{
    "it's",
    "a b"
};
#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

// CHOICES_33
//   constant: the values a listing validator lists.
static const char* const CHOICES_33[] =
{
    "red",
    "blue",
    NULL
};

//==============================================================================
// HELPERS
//==============================================================================

// d_tests_arg_completion_span
//   struct: a row of bytes: the row's tag, its bytes -- NULL for a span
// without data -- and how many.
struct d_tests_arg_completion_span
{
    const char* tag;
    const char* data;
    size_t      length;
};

// d_tests_arg_completion_number
//   struct: a row of a number: the row's tag, as the transcript writes it,
// and the number.
struct d_tests_arg_completion_number
{
    const char* tag;
    int         value;
};

// d_tests_arg_completion_lines
//   struct: a row of lines: its tag, the lines -- NULL for none -- and how
// many, and the identifier their marks stand for.
struct d_tests_arg_completion_lines
{
    const char*        tag;
    const char* const* lines;
    size_t             count;
    const char*        ident;
};

// d_tests_arg_completion_text
//   struct: a row of text: its tag and the text, NULL for none.
struct d_tests_arg_completion_text
{
    const char* tag;
    const char* text;
};

// d_tests_arg_completion_engine
//   struct: an engine as the suite holds it: its name, as the transcripts
// write it, and its lines.
struct d_tests_arg_completion_engine
{
    const char*        name;
    const char* const* lines;
    size_t             count;
};

// ENGINES
//   constant: the engines, in the order a script may hold them: the walk
// bash and zsh share, then each shell's own.
static const struct d_tests_arg_completion_engine ENGINES[] =
{
    { "walk", ENGINE_WALK, ENGINE_WALK_COUNT },
    { "bash", ENGINE_BASH, ENGINE_BASH_COUNT },
    { "zsh",  ENGINE_ZSH,  ENGINE_ZSH_COUNT  },
    { "fish", ENGINE_FISH, ENGINE_FISH_COUNT },
    { "pwsh", ENGINE_PWSH, ENGINE_PWSH_COUNT }
};

// SHELL_LABELS
//   constant: each d_arg_shell's name as the transcripts write it, apart
// from d_arg_shell_name, whose own section holds it.
static const char* const SHELL_LABELS[D_ARG_SHELL_COUNT] =
{
    "bash",
    "zsh",
    "fish",
    "powershell"
};

/**
 * @brief Joins a transcript's parts into one text.
 *
 * @param[in] _parts  the parts, a NULL after the last.
 * @return the text, which the caller frees; NULL when it could not be had.
 */
static char*
d_tests_arg_completion_join(
    const char* const* _parts
)
{
    struct d_arg_buffer joined = D_ARG_BUFFER_INIT;

    // each part, after the ones before it
    for (size_t index = 0u; _parts[index] != NULL; ++index)
    {
        (void)d_arg_buffer_append_cstr(&joined,
                                       _parts[index]);
    }

    // an empty transcript is still a text
    (void)d_arg_buffer_append_cstr(&joined,
                                   "");

    char* const text = (joined.failed == 0)
                       ? d_arg_buffer_detach(&joined)
                       : NULL;

    d_arg_buffer_release(&joined);

    return text;
}

/**
 * @brief Appends an engine's text as a script holds it: each line with its
 *        `@@` marks the identifier, and a newline after it.
 *
 * @param[in,out] _out     the text.
 * @param[in]     _engine  the engine.
 * @param[in]     _ident   the identifier.
 *
 * Worked out here, apart from the library's own replacement, so that a
 * script's engines are held to their source, completion/eng/.
 */
static void
d_tests_arg_completion_expand(
    struct d_arg_buffer*                        _out,
    const struct d_tests_arg_completion_engine* _engine,
    const char*                                 _ident
)
{
    // each line
    for (size_t index = 0u; index < _engine->count; ++index)
    {
        const char* const line = _engine->lines[index];
        size_t            at   = 0u;

        // each byte, a mark the identifier
        while (line[at] != '\0')
        {
            const bool mark = ( (line[at] == '@') &&
                                (line[at + 1u] == '@') );

            if (mark)
            {
                (void)d_arg_buffer_append_cstr(_out,
                                               _ident);
            }
            else
            {
                (void)d_arg_buffer_append_char(_out,
                                               line[at]);
            }

            at += (mark) ? 2u : 1u;
        }

        (void)d_arg_buffer_append_char(_out,
                                       '\n');
    }

    return;
}

/**
 * @brief Finds text within text, from a place on.
 *
 * @param[in]  _text    the text searched.
 * @param[in]  _from    where the search begins.
 * @param[in]  _needle  the text sought, not empty.
 * @param[out] _at      receives where it was found.
 * @return whether it was found.
 */
static bool
d_tests_arg_completion_find(
    struct d_pack_text _text,
    size_t             _from,
    struct d_pack_text _needle,
    size_t*            _at
)
{
    // each place it could begin
    for (size_t at = _from;
         (_needle.length > 0u) && (at + _needle.length <= _text.length);
         ++at)
    {
        if (memcmp(_text.data + at,
                   _needle.data,
                   _needle.length) == 0)
        {
            *_at = at;

            return true;
        }
    }

    return false;
}

/**
 * @brief Appends a script with each engine it holds whole as one line,
 *        `[engine <name>]`.
 *
 * @param[in,out] _out     the transcript.
 * @param[in]     _script  the script.
 * @param[in]     _ident   the identifier the engines were written with.
 *
 * The engines are sought in the order a script holds them, each after the
 * one before; one not found whole is left as the script has it, so that the
 * transcript shows what the script holds in its place.
 */
static void
d_tests_arg_completion_elide(
    struct d_arg_buffer* _out,
    struct d_pack_text   _script,
    const char*          _ident
)
{
    const size_t engines = sizeof(ENGINES) / sizeof(ENGINES[0]);
    size_t       done    = 0u;

    // each engine
    for (size_t index = 0u; index < engines; ++index)
    {
        struct d_arg_buffer text  = D_ARG_BUFFER_INIT;
        size_t              found = 0u;

        d_tests_arg_completion_expand(&text,
                                      &ENGINES[index],
                                      _ident);

        // found whole, from where the last one ended
        if (d_tests_arg_completion_find(_script,
                                        done,
                                        d_arg_buffer_text(&text),
                                        &found))
        {
            (void)d_arg_buffer_append(_out,
                                      _script.data + done,
                                      found - done);
            (void)d_arg_buffer_printf(_out,
                                      "[engine %s]\n",
                                      ENGINES[index].name);
            done = found + text.length;
        }

        _out->failed |= text.failed;
        d_arg_buffer_release(&text);
    }

    (void)d_arg_buffer_append(_out,
                              _script.data + done,
                              _script.length - done);

    return;
}

/**
 * @brief Adds a command's script in each shell to the transcript, each
 *        after a line `== <tag> <shell> <status>`, its engines elided.
 *
 * @param[in,out] _run      the run.
 * @param[in]     _command  the command.
 * @param[in]     _tag      the case's tag.
 * @param[in]     _program  the program, as the script is asked for it.
 * @param[in]     _ident    the identifier its engines are written with.
 */
static void
d_tests_arg_completion_scripts(
    struct d_tests_arg_run*     _run,
    const struct d_arg_command* _command,
    const char*                 _tag,
    const char*                 _program,
    const char*                 _ident
)
{
    // each shell
    for (int shell = 0; shell < (int)D_ARG_SHELL_COUNT; ++shell)
    {
        struct d_arg_buffer script = D_ARG_BUFFER_INIT;
        const int           status =
            d_arg_completion_script(_command,
                                    shell,
                                    d_pack_text_from_cstr(_program),
                                    d_arg_buffer_sink(&script));

        (void)d_arg_buffer_printf(&_run->transcript,
                                  "== %s %s %s\n",
                                  _tag,
                                  SHELL_LABELS[shell],
                                  d_arg_status_name(status));
        d_tests_arg_completion_elide(&_run->transcript,
                                     d_arg_buffer_text(&script),
                                     _ident);
        _run->failed |= (script.failed != 0);
        d_arg_buffer_release(&script);
    }

    return;
}

/**
 * @brief Lists the values a listing validator was made with.
 *
 * @param[in] _validator  the validator, whose context is the values.
 * @param[in] _each       called once for each value.
 * @param[in] _context    `_each`'s context.
 * @return D_ARG_OK, or the first other status `_each` returned.
 */
static int
d_tests_arg_completion_list(
    const struct d_arg_validator* _validator,
    d_arg_choice_fn               _each,
    void*                         _context
)
{
    const char* const* const values = (const char* const*)_validator->data;
    int                      status = D_ARG_OK;

    // each value, until a listing stops
    for (size_t index = 0u;
         (status == D_ARG_OK) && (values[index] != NULL);
         ++index)
    {
        status = _each(d_pack_text_from_cstr(values[index]),
                       _context);
    }

    return status;
}

/**
 * @brief Makes a validator that passes every value and lists some for
 *        completion.
 *
 * @param[in] _values  the values, a NULL after the last, which must
 *                     outlive the validator.
 * @return the validator.
 *
 * The values are the validator's context, which it only reads: a context
 * is a pointer to what may change, so the values' address goes through an
 * integer to become one, as a cast that drops `const` is refused.
 */
static struct d_arg_validator
d_tests_arg_completion_listing(
    const char* const* _values
)
{
    struct d_arg_validator validator =
        d_arg_validator_custom(NULL,
                               (void*)(uintptr_t)_values,
                               NULL);

    d_arg_validator_set_choices(&validator,
                                d_tests_arg_completion_list);

    return validator;
}

/**
 * @brief Makes a validator that passes every value and completes as a kind
 *        of value.
 *
 * @param[in] _kind  D_ARG_VALIDATOR_PATH, _DIRECTORY or _NUMBER.
 * @return the validator.
 */
static struct d_arg_validator
d_tests_arg_completion_kind(
    uint32_t _kind
)
{
    struct d_arg_validator validator = d_arg_validator_custom(NULL,
                                                              NULL,
                                                              NULL);

    d_arg_validator_set_value_kind(&validator,
                                   _kind);

    return validator;
}

/**
 * @brief A listing that fails at once.
 *
 * @param[in] _validator  the validator.
 * @param[in] _each       called for no value.
 * @param[in] _context    `_each`'s context.
 * @return D_ARG_ERROR_UNSUPPORTED.
 */
static int
d_tests_arg_completion_fail_now(
    const struct d_arg_validator* _validator,
    d_arg_choice_fn               _each,
    void*                         _context
)
{
    (void)_validator;
    (void)_each;
    (void)_context;

    return D_ARG_ERROR_UNSUPPORTED;
}

/**
 * @brief A listing that fails after listing one value.
 *
 * @param[in] _validator  the validator.
 * @param[in] _each       called for the one value.
 * @param[in] _context    `_each`'s context.
 * @return the status `_each` returned, or D_ARG_ERROR_UNSUPPORTED.
 */
static int
d_tests_arg_completion_fail_later(
    const struct d_arg_validator* _validator,
    d_arg_choice_fn               _each,
    void*                         _context
)
{
    const int status = _each(d_pack_text_from_cstr("v0"),
                             _context);

    (void)_validator;

    return (status == D_ARG_OK)
           ? D_ARG_ERROR_UNSUPPORTED
           : status;
}

/**
 * @brief Makes a validator whose listing fails.
 *
 * @param[in] _listed  how many values it lists before it fails: 0 or 1.
 * @return the validator.
 */
static struct d_arg_validator
d_tests_arg_completion_failing(
    size_t _listed
)
{
    struct d_arg_validator validator = d_arg_validator_custom(NULL,
                                                              NULL,
                                                              NULL);

    d_arg_validator_set_choices(&validator,
                                (_listed > 0u)
                                ? d_tests_arg_completion_fail_later
                                : d_tests_arg_completion_fail_now);

    return validator;
}

/**
 * @brief Makes an option bound to a variable of a built-in type, the
 *        variable kept by the run.
 *
 * @param[in,out] _run          the run.
 * @param[in,out] _command      the command.
 * @param[in]     _names        the option's names.
 * @param[in]     _description  its description.
 * @param[in]     _builtin      a d_arg_builtin value.
 * @return the option; NULL, failing the run, when it could not be made.
 */
static struct d_arg_option*
d_tests_arg_completion_bound(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _command,
    const char*             _names,
    const char*             _description,
    int                     _builtin
)
{
    const struct d_arg_type* const type     = d_arg_type_builtin(_builtin);
    void* const                    variable = (type != NULL)
                                              ? d_tests_arg_keep(_run,
                                                                 type->size)
                                              : NULL;

    // nothing to bind without a type and a variable
    if (!variable)
    {
        _run->failed = true;

        return NULL;
    }

    return d_tests_arg_watch_option(
               _run,
               _names,
               d_arg_command_add_option_bound(_command,
                                              _names,
                                              variable,
                                              type,
                                              _description));
}

/**
 * @brief Makes an option bound to a type of the suite's own: the built-in
 *        string's, with other flags, the type and the variable kept by the
 *        run.
 *
 * @param[in,out] _run          the run.
 * @param[in,out] _command      the command.
 * @param[in]     _names        the option's names.
 * @param[in]     _description  its description.
 * @param[in]     _flags        the type's D_ARG_TYPE_FLAG_* flags.
 * @return the option; NULL, failing the run, when it could not be made.
 */
static struct d_arg_option*
d_tests_arg_completion_typed(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _command,
    const char*             _names,
    const char*             _description,
    uint32_t                _flags
)
{
    const struct d_arg_type* const string =
        d_arg_type_builtin((int)D_ARG_BUILTIN_STRING);
    struct d_arg_type* const       type     =
        (struct d_arg_type*)d_tests_arg_keep(_run,
                                             sizeof(struct d_arg_type));
    void* const                    variable =
        (string != NULL)
        ? d_tests_arg_keep(_run,
                           string->size)
        : NULL;

    // nothing to bind without the types and a variable
    if ( (!string)   ||
         (!type)     ||
         (!variable) )
    {
        _run->failed = true;

        return NULL;
    }

    *type       = *string;
    type->flags = _flags;

    return d_tests_arg_watch_option(
               _run,
               _names,
               d_arg_command_add_option_bound(_command,
                                              _names,
                                              variable,
                                              type,
                                              _description));
}

/**
 * @brief An option's callback that notes the first value it was given.
 *
 * @param[in] _option   the option.
 * @param[in] _results  its values.
 * @param[in] _count    how many.
 * @param[in] _context  the run.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_completion_noted(
    struct d_arg_option*      _option,
    const struct d_pack_text* _results,
    size_t                    _count,
    void*                     _context
)
{
    (void)_option;

    d_tests_arg_note((struct d_tests_arg_run*)_context,
                     "noted [%.*s]",
                     (_count > 0u) ? (int)_results[0].length : 0,
                     (_count > 0u) ? _results[0].data : "");

    return D_ARG_OK;
}

/**
 * @brief A sink's write function that takes nothing.
 *
 * @param[in] _context  unused.
 * @param[in] _data     unused.
 * @param[in] _size     unused.
 * @return 0.
 */
static size_t
d_tests_arg_completion_refuse(
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
 * @brief A span over bytes, exactly as given: the framework's
 *        d_pack_text_from_span reads data that is NULL as an empty span,
 *        whatever the length, where a caller's own struct can hold a length
 *        beside NULL data -- which is what the refusals and the rows of a
 *        span without data give the calls.
 *
 * @param[in] _data    the bytes; may be NULL.
 * @param[in] _length  how many, whatever `_data` is.
 * @return the span.
 */
static struct d_pack_text
d_tests_arg_completion_exact(
    const char* _data,
    size_t      _length
)
{
    const struct d_pack_text text = { _data, _length };

    return text;
}

/**
 * @brief Asks for a script and adds the line it came to: `<tag>: <status>
 *        wrote=<bytes>`, the bytes the sink took.
 *
 * @param[in,out] _run       the run.
 * @param[in]     _tag       the step's tag.
 * @param[in]     _command   the command; may be NULL.
 * @param[in]     _shell     the shell, any number.
 * @param[in]     _program   the program, as the script is asked for it.
 * @param[in]     _refusing  whether the sink takes nothing.
 */
static void
d_tests_arg_completion_try(
    struct d_tests_arg_run*     _run,
    const char*                 _tag,
    const struct d_arg_command* _command,
    int                         _shell,
    struct d_pack_text          _program,
    bool                        _refusing
)
{
    struct d_arg_buffer script = D_ARG_BUFFER_INIT;
    struct d_pack_sink  sink   = d_arg_buffer_sink(&script);

    // a sink that takes nothing, when asked for
    if (_refusing)
    {
        sink.write = d_tests_arg_completion_refuse;
    }

    const int status = d_arg_completion_script(_command,
                                               _shell,
                                               _program,
                                               sink);

    (void)d_arg_buffer_printf(&_run->transcript,
                              "%s: %s wrote=%zu\n",
                              _tag,
                              d_arg_status_name(status),
                              script.length);
    d_arg_buffer_release(&script);

    return;
}

/**
 * @brief Calls an option's callback with no values, as no parse does, and
 *        adds the line it came to: `<tag>: <status> recorded=<status>`, the
 *        second what the parser then reports.
 *
 * @param[in,out] _run     the run.
 * @param[in]     _tag     the step's tag.
 * @param[in,out] _option  the option.
 */
static void
d_tests_arg_completion_called_back(
    struct d_tests_arg_run* _run,
    const char*             _tag,
    struct d_arg_option*    _option
)
{
    const int status = _option->callback(_option,
                                         NULL,
                                         0u,
                                         _option->callback_context);

    (void)d_arg_buffer_printf(&_run->transcript,
                              "%s: %s recorded=%s\n",
                              _tag,
                              d_arg_status_name(status),
                              d_arg_status_name(
                                  d_arg_parser_error(&_run->parser)->status));

    return;
}

/**
 * @brief Adds the lines a parse came to, after the parser's exit: its
 *        status and exit code, then what the exit wrote -- a completion
 *        script whole, its engines elided, anything else by its first line
 *        -- and the error output; then empties the output for the next.
 *
 * @param[in,out] _run     the run.
 * @param[in]     _status  the parse's status.
 * @param[in]     _ident   the identifier a script's engines are written with.
 */
static void
d_tests_arg_completion_ended(
    struct d_tests_arg_run* _run,
    int                     _status,
    const char*             _ident
)
{
    const int                code = d_arg_parser_exit(&_run->parser,
                                                      _status);
    const struct d_pack_text out  = d_arg_buffer_text(&_run->out);
    size_t                   line = 0u;

    (void)d_arg_buffer_printf(&_run->transcript,
                              "status=%s code=%d\n",
                              d_arg_status_name(_status),
                              code);

    // the first line of what was written
    while ( (line < out.length) &&
            (out.data[line] != '\n') )
    {
        ++line;
    }

    // a script whole, or anything else by its first line
    if (_status == (int)D_ARG_CALL_FOR_COMPLETION)
    {
        (void)d_arg_buffer_append_cstr(&_run->transcript,
                                       "out:\n");
        d_tests_arg_completion_elide(&_run->transcript,
                                     out,
                                     _ident);
    }
    else if (out.length > 0u)
    {
        (void)d_arg_buffer_append_cstr(&_run->transcript,
                                       "out: [");
        d_tests_arg_escape(&_run->transcript,
                           d_pack_text_from_span(out.data,
                                                 line));
        (void)d_arg_buffer_append_cstr(&_run->transcript,
                                       "]\n");
    }

    // the error output, whole
    if (_run->err.length > 0u)
    {
        (void)d_arg_buffer_append_cstr(&_run->transcript,
                                       "err: [");
        d_tests_arg_escape(&_run->transcript,
                           d_arg_buffer_text(&_run->err));
        (void)d_arg_buffer_append_cstr(&_run->transcript,
                                       "]\n");
    }

    d_arg_buffer_clear(&_run->out);
    d_arg_buffer_clear(&_run->err);

    return;
}

/**
 * @brief Parses words as main's arguments, after a program's name, or alone
 *        -- as d_arg_parser_parse_args takes them -- and adds what the parse
 *        came to: `== <tag> <program or -> [<words>]`, what its callbacks
 *        noted, then the lines d_tests_arg_completion_ended adds.
 *
 * @param[in,out] _run      the run.
 * @param[in]     _tag      the step's tag.
 * @param[in]     _program  `argv[0]`; NULL for none.
 * @param[in]     _words    the words, split at each space.
 * @param[in]     _ident    the identifier a script's engines are written
 *                          with.
 */
static void
d_tests_arg_completion_parse(
    struct d_tests_arg_run* _run,
    const char*             _tag,
    const char*             _program,
    const char*             _words,
    const char*             _ident
)
{
    int                argc  = 0;
    char* const* const argv  = d_tests_arg_argv(_run,
                                                _program,
                                                _words,
                                                &argc);
    const int          first = (_program != NULL) ? 1 : 0;

    (void)d_arg_buffer_printf(&_run->transcript,
                              "== %s ",
                              _tag);

    // the program, or none
    if (_program != NULL)
    {
        d_tests_arg_escape(&_run->transcript,
                           d_pack_text_from_cstr(_program));
    }
    else
    {
        (void)d_arg_buffer_append_char(&_run->transcript,
                                       '-');
    }

    (void)d_arg_buffer_append_cstr(&_run->transcript,
                                   " [");

    // each word, after a space when one comes before it
    for (int index = first; (argv != NULL) && (index < argc); ++index)
    {
        (void)d_arg_buffer_append_cstr(&_run->transcript,
                                       (index > first) ? " " : "");
        d_tests_arg_escape(&_run->transcript,
                           d_pack_text_from_cstr(argv[index]));
    }

    (void)d_arg_buffer_append_cstr(&_run->transcript,
                                   "]\n");

    const int status =
        (argv == NULL)
        ? (int)D_ARG_ERROR_OUT_OF_MEMORY
        : (_program != NULL)
          ? d_arg_parser_parse(&_run->parser,
                               argc,
                               argv)
          : d_tests_arg_parse_quiet(_run,
                                    (const char* const*)argv,
                                    (size_t)argc);

    d_tests_arg_completion_ended(_run,
                                 status,
                                 _ident);

    return;
}

#if (D_INTERNAL_ARG_PARSER_ENVIRONMENT == 1)

/**
 * @brief Adds how many values an option was given: `given <label> <count>`.
 *
 * @param[in,out] _run     the run.
 * @param[in]     _label   what the option is called in the transcript.
 * @param[in]     _option  the option.
 */
static void
d_tests_arg_completion_given(
    struct d_tests_arg_run*    _run,
    const char*                _label,
    const struct d_arg_option* _option
)
{
    (void)d_arg_buffer_printf(&_run->transcript,
                              "given %s %zu\n",
                              _label,
                              d_arg_option_count(_option));

    return;
}

#endif  // D_INTERNAL_ARG_PARSER_ENVIRONMENT

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief Parses configuration text and adds what the parse came to: `==
 *        <tag> config [<text>]`, then the lines d_tests_arg_completion_ended
 *        adds.
 *
 * @param[in,out] _run   the run.
 * @param[in]     _tag   the step's tag.
 * @param[in]     _text  the configuration text.
 */
static void
d_tests_arg_completion_parse_config(
    struct d_tests_arg_run* _run,
    const char*             _tag,
    const char*             _text
)
{
    (void)d_arg_buffer_printf(&_run->transcript,
                              "== %s config [",
                              _tag);
    d_tests_arg_escape(&_run->transcript,
                       d_pack_text_from_cstr(_text));
    (void)d_arg_buffer_append_cstr(&_run->transcript,
                                   "]\n");

    const int status = d_arg_parser_parse_config(
                           &_run->parser,
                           d_pack_text_from_cstr(_text));

    d_tests_arg_completion_ended(_run,
                                 status,
                                 "");

    return;
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

/**
 * @brief Runs a table of steps and compares what they wrote with the
 *        transcript expected, given in parts.
 *
 * @param[in] _name      the transcript, for a printed mismatch.
 * @param[in] _steps     the steps.
 * @param[in] _count     how many.
 * @param[in] _expected  the transcript's parts, a NULL after the last.
 * @return whether every step ran and the transcript was as expected.
 */
static bool
d_tests_arg_completion_steps(
    const char*                  _name,
    const d_tests_arg_define_fn* _steps,
    size_t                       _count,
    const char* const*           _expected
)
{
    char* const expected = d_tests_arg_completion_join(_expected);
    const bool  held     = ( (expected != NULL) &&
                             (d_tests_arg_steps(_name,
                                                _steps,
                                                _count,
                                                expected)) );

    free(expected);

    return held;
}

/**
 * @brief Compares what rows came to with the transcript expected, given in
 *        parts, then releases what they came to.
 *
 * @param[in]     _name      the transcript, for a printed mismatch.
 * @param[in,out] _written   what the rows came to.
 * @param[in]     _expected  the transcript's parts, a NULL after the last.
 * @return whether what they came to was written whole and as expected.
 */
static bool
d_tests_arg_completion_compare(
    const char*          _name,
    struct d_arg_buffer* _written,
    const char* const*   _expected
)
{
    char* const expected = d_tests_arg_completion_join(_expected);
    const bool  held     = ( (expected != NULL)          &&
                             (_written->failed == 0)     &&
                             (d_tests_arg_same(_name,
                                               d_arg_buffer_text(_written),
                                               expected)) );

    free(expected);
    d_arg_buffer_release(_written);

    return held;
}

/**
 * @brief Looks each row's span up as a shell's name: `[<tag>] <shell>`.
 *
 * @param[in] _name      the transcript, for a printed mismatch.
 * @param[in] _rows      the rows.
 * @param[in] _count     how many.
 * @param[in] _expected  the transcript's parts, a NULL after the last.
 * @return whether the transcript was as expected.
 */
static bool
d_tests_arg_completion_shells(
    const char*                               _name,
    const struct d_tests_arg_completion_span* _rows,
    size_t                                    _count,
    const char* const*                        _expected
)
{
    struct d_arg_buffer written = D_ARG_BUFFER_INIT;

    // each row
    for (size_t index = 0u; index < _count; ++index)
    {
        (void)d_arg_buffer_printf(
            &written,
            "[%s] %d\n",
            _rows[index].tag,
            d_arg_shell_from_name(
                d_tests_arg_completion_exact(_rows[index].data,
                                             _rows[index].length)));
    }

    return d_tests_arg_completion_compare(_name,
                                          &written,
                                          _expected);
}

/**
 * @brief Names each row's number as a shell: `<tag> [<name>]`.
 *
 * @param[in] _name      the transcript, for a printed mismatch.
 * @param[in] _rows      the rows.
 * @param[in] _count     how many.
 * @param[in] _expected  the transcript's parts, a NULL after the last.
 * @return whether the transcript was as expected.
 */
static bool
d_tests_arg_completion_numbers(
    const char*                                 _name,
    const struct d_tests_arg_completion_number* _rows,
    size_t                                      _count,
    const char* const*                          _expected
)
{
    struct d_arg_buffer written = D_ARG_BUFFER_INIT;

    // each row
    for (size_t index = 0u; index < _count; ++index)
    {
        (void)d_arg_buffer_printf(&written,
                                  "%s [%s]\n",
                                  _rows[index].tag,
                                  d_arg_shell_name(_rows[index].value));
    }

    return d_tests_arg_completion_compare(_name,
                                          &written,
                                          _expected);
}

/**
 * @brief Writes every byte, then each row's bytes, as an identifier: `<two
 *        hexadecimal digits> <identifier> <status>` for a byte, `[<tag>]
 *        <identifier> <status>` for a row.
 *
 * @param[in] _name      the transcript, for a printed mismatch.
 * @param[in] _rows      the rows.
 * @param[in] _count     how many.
 * @param[in] _expected  the transcript's parts, a NULL after the last.
 * @return whether the transcript was as expected.
 */
static bool
d_tests_arg_completion_idents(
    const char*                               _name,
    const struct d_tests_arg_completion_span* _rows,
    size_t                                    _count,
    const char* const*                        _expected
)
{
    struct d_arg_buffer written = D_ARG_BUFFER_INIT;

    // each byte alone
    for (unsigned int value = 0u; value <= (unsigned int)UCHAR_MAX; ++value)
    {
        const char byte = (char)(unsigned char)value;

        (void)d_arg_buffer_printf(&written,
                                  "%02x ",
                                  value);

        const int status = d_arg_internal_complete_ident(
                               d_pack_text_from_span(&byte,
                                                     1u),
                               &written);

        (void)d_arg_buffer_printf(&written,
                                  " %s\n",
                                  d_arg_status_name(status));
    }

    // each row
    for (size_t index = 0u; index < _count; ++index)
    {
        (void)d_arg_buffer_printf(&written,
                                  "[%s] ",
                                  _rows[index].tag);

        const int status = d_arg_internal_complete_ident(
                               d_pack_text_from_span(_rows[index].data,
                                                     _rows[index].length),
                               &written);

        (void)d_arg_buffer_printf(&written,
                                  " %s\n",
                                  d_arg_status_name(status));
    }

    return d_tests_arg_completion_compare(_name,
                                          &written,
                                          _expected);
}

/**
 * @brief Writes each row's lines with their marks replaced, after a line
 *        `== <tag> <status>`.
 *
 * @param[in] _name      the transcript, for a printed mismatch.
 * @param[in] _rows      the rows.
 * @param[in] _count     how many.
 * @param[in] _expected  the transcript's parts, a NULL after the last.
 * @return whether the transcript was as expected.
 */
static bool
d_tests_arg_completion_marks(
    const char*                                _name,
    const struct d_tests_arg_completion_lines* _rows,
    size_t                                     _count,
    const char* const*                         _expected
)
{
    struct d_arg_buffer written = D_ARG_BUFFER_INIT;

    // each row
    for (size_t index = 0u; index < _count; ++index)
    {
        struct d_arg_buffer lines  = D_ARG_BUFFER_INIT;
        const int           status = d_arg_internal_complete_lines(
                                         _rows[index].lines,
                                         _rows[index].count,
                                         d_pack_text_from_cstr(
                                             _rows[index].ident),
                                         &lines);

        (void)d_arg_buffer_printf(&written,
                                  "== %s %s\n",
                                  _rows[index].tag,
                                  d_arg_status_name(status));
        (void)d_arg_buffer_append_text(&written,
                                       d_arg_buffer_text(&lines));
        written.failed |= lines.failed;
        d_arg_buffer_release(&lines);
    }

    return d_tests_arg_completion_compare(_name,
                                          &written,
                                          _expected);
}

/**
 * @brief Writes each row's text as the line a script shows beside a name:
 *        `[<tag>] [<line>]`.
 *
 * @param[in] _name      the transcript, for a printed mismatch.
 * @param[in] _rows      the rows.
 * @param[in] _count     how many.
 * @param[in] _expected  the transcript's parts, a NULL after the last.
 * @return whether the transcript was as expected.
 */
static bool
d_tests_arg_completion_line_texts(
    const char*                               _name,
    const struct d_tests_arg_completion_text* _rows,
    size_t                                    _count,
    const char* const*                        _expected
)
{
    struct d_arg_buffer written = D_ARG_BUFFER_INIT;

    // each row
    for (size_t index = 0u; index < _count; ++index)
    {
        (void)d_arg_buffer_printf(&written,
                                  "[%s] [",
                                  _rows[index].tag);
        (void)d_arg_buffer_append_text(
                  &written,
                  d_arg_internal_complete_line_text(_rows[index].text));
        (void)d_arg_buffer_append_cstr(&written,
                                       "]\n");
    }

    return d_tests_arg_completion_compare(_name,
                                          &written,
                                          _expected);
}

//==============================================================================
// 1.  SHELL NAMES
//==============================================================================

// SHELL_FROM_NAME_ROWS
//   constant: the cases of d_tests_arg_completion_shell_from_name.
static const struct d_tests_arg_completion_span SHELL_FROM_NAME_ROWS[] =
{
    { "bash", "bash", 4u },
    { "zsh", "zsh", 3u },
    { "fish", "fish", 4u },
    { "powershell", "powershell", 10u },
    { "pwsh", "pwsh", 4u },
    { "BASH", "BASH", 4u },
    { "Zsh", "Zsh", 3u },
    { "fIsH", "fIsH", 4u },
    { "PowerShell", "PowerShell", 10u },
    { "PWSH", "PWSH", 4u },
    { "pWsH", "pWsH", 4u },
    { "empty", "", 0u },
    { "no data", NULL, 0u },
    { "no data, a length", NULL, 3u },
    { "bas", "bas", 3u },
    { "bashx", "bashx", 5u },
    { "zs", "zs", 2u },
    { "pwsh ", "pwsh ", 5u },
    { " bash", " bash", 5u },
    { "ba sh", "ba sh", 5u },
    { "bash, cut", "bashful", 4u },
    { "zsh, cut short", "zsh", 2u },
    { "bash NUL", "bash\x00", 5u },
    { "NUL bash", "\x00" "bash", 5u },
    { "powershel", "powershel", 9u },
    { "powershells", "powershells", 11u },
    { "power", "power", 5u },
    { "sh", "sh", 2u },
    { "csh", "csh", 3u },
    { "tcsh", "tcsh", 4u },
    { "ksh", "ksh", 3u },
    { "cmd", "cmd", 3u },
    { "nushell", "nushell", 7u },
    { "elvish", "elvish", 6u },
    { "BASH, dotless", "BAS\xc4\xb0", 5u },
    { "ZSH, long s", "z\xc5\xbfh", 4u },
    { "FISH, fullwidth", "\xef\xbd\x86ish", 6u },
    { "BaSh\\t", "BaSh\t", 5u },
    { "\\x80ash", "\x80" "ash", 4u },
    { "b@sh", "b@sh", 4u },
    { "bAsh", "bAsh", 4u },
    { "fisH", "fisH", 4u },
    { "zsH", "zsH", 3u }
};

// SHELL_FROM_NAME_COUNT
//   constant: how many cases SHELL_FROM_NAME_ROWS holds.
static const size_t SHELL_FROM_NAME_COUNT =
    sizeof(SHELL_FROM_NAME_ROWS) / sizeof(SHELL_FROM_NAME_ROWS[0]);

/*
d_tests_arg_completion_shell_from_name
  Every name d_arg_shell_from_name reads, in any case, and spans that name no
shell.
  Tests the following:
  - each shell by its name, in any ASCII case; "pwsh" as PowerShell
  - a span without data, empty, cut short, run on, padded or holding a NUL
  - names that are near, or other shells'
  - letters past ASCII that fold to a name's in Unicode: none
*/
bool
d_tests_arg_completion_shell_from_name(void)
{
    // expected
    //   the transcript, in the parts a C99 string literal can hold.
    const char* const expected[] =
    {
        SHELL_FROM_NAME,
        NULL
    };

    return d_tests_arg_completion_shells("SHELL_FROM_NAME",
                                         SHELL_FROM_NAME_ROWS,
                                         SHELL_FROM_NAME_COUNT,
                                         expected);
}

//==============================================================================
// 2.  SHELL NUMBERS
//==============================================================================

// SHELL_NAME_ROWS
//   constant: the cases of d_tests_arg_completion_shell_name.
static const struct d_tests_arg_completion_number SHELL_NAME_ROWS[] =
{
    { "0", 0 },
    { "1", 1 },
    { "2", 2 },
    { "3", 3 },
    { "4", 4 },
    { "5", 5 },
    { "-1", -1 },
    { "-2", -2 },
    { "INT_MAX", INT_MAX },
    { "INT_MIN", INT_MIN },
    { "100", 100 }
};

// SHELL_NAME_COUNT
//   constant: how many cases SHELL_NAME_ROWS holds.
static const size_t SHELL_NAME_COUNT =
    sizeof(SHELL_NAME_ROWS) / sizeof(SHELL_NAME_ROWS[0]);

/*
d_tests_arg_completion_shell_name
  What d_arg_shell_name gives each shell, and numbers that are none.
  Tests the following:
  - each d_arg_shell value's name, PowerShell's "powershell"
  - D_ARG_SHELL_COUNT, numbers past it and below 0: ""
*/
bool
d_tests_arg_completion_shell_name(void)
{
    // expected
    //   the transcript, in the parts a C99 string literal can hold.
    const char* const expected[] =
    {
        SHELL_NAME,
        NULL
    };

    return d_tests_arg_completion_numbers("SHELL_NAME",
                                          SHELL_NAME_ROWS,
                                          SHELL_NAME_COUNT,
                                          expected);
}

//==============================================================================
// 3.  IDENTIFIERS
//==============================================================================

// IDENTIFIERS_ROWS
//   constant: the cases of d_tests_arg_completion_identifiers.
static const struct d_tests_arg_completion_span IDENTIFIERS_ROWS[] =
{
    { "prog", "prog", 4u },
    { "my_tool", "my_tool", 7u },
    { "my-tool", "my-tool", 7u },
    { "a.b.c", "a.b.c", 5u },
    { "7z", "7z", 2u },
    { "__init__", "__init__", 8u },
    { "two words", "two words", 9u },
    { "UTF-8", "caf\xc3\xa9", 5u },
    { "astral", "\xf0\x9f\x98\x80", 4u },
    { "stray", "\xff\xfe", 2u },
    { "quote", "it's", 4u },
    { "slash", "a/b\\c", 5u },
    { "empty", "", 0u },
    { "NUL", "a\x00" "b", 3u },
    { "tab", "a\tb", 3u },
    { "mixed", "Ab_9-x.Y", 8u },
    { "digits", "0123456789", 10u },
    { "letters", "AZaz", 4u },
    { "edges", "@[`{/:", 6u }
};

// IDENTIFIERS_COUNT
//   constant: how many cases IDENTIFIERS_ROWS holds.
static const size_t IDENTIFIERS_COUNT =
    sizeof(IDENTIFIERS_ROWS) / sizeof(IDENTIFIERS_ROWS[0]);

/*
d_tests_arg_completion_identifiers
  The program's name as the identifier a script's functions are named by: every
byte alone, then whole names.
  Tests the following:
  - ASCII letters and digits as they are, `_` doubled
  - every other byte as `_` and two lower-case hexadecimal digits
  - names of several bytes, a NUL among them, and the empty name
*/
bool
d_tests_arg_completion_identifiers(void)
{
    // expected
    //   the transcript, in the parts a C99 string literal can hold.
    const char* const expected[] =
    {
        IDENTIFIERS,
        NULL
    };

    return d_tests_arg_completion_idents("IDENTIFIERS",
                                         IDENTIFIERS_ROWS,
                                         IDENTIFIERS_COUNT,
                                         expected);
}

//==============================================================================
// 4.  LINES
//==============================================================================

// SCRIPT_LINES_ROWS
//   constant: the cases of d_tests_arg_completion_script_lines.
static const struct d_tests_arg_completion_lines SCRIPT_LINES_ROWS[] =
{
    { "none", LINES_1, 2u, "prog" },
    { "one", LINES_2, 1u, "prog" },
    { "two", LINES_3, 1u, "p" },
    { "three marks", LINES_4, 1u, "X" },
    { "four marks", LINES_5, 1u, "X" },
    { "five marks", LINES_6, 1u, "X" },
    { "a lone mark", LINES_7, 3u, "X" },
    { "ends", LINES_8, 3u, "id" },
    { "empty ident", LINES_9, 3u, "" },
    { "long ident", LINES_10, 1u, "my__tool_2dname_c3_a9" },
    { "ident with marks", LINES_11, 1u, "@@" },
    { "several lines", LINES_12, 4u, "q" },
    { "no lines", NULL, 0u, "prog" }
};

// SCRIPT_LINES_COUNT
//   constant: how many cases SCRIPT_LINES_ROWS holds.
static const size_t SCRIPT_LINES_COUNT =
    sizeof(SCRIPT_LINES_ROWS) / sizeof(SCRIPT_LINES_ROWS[0]);

/*
d_tests_arg_completion_script_lines
  Lines of a script, each `@@` in them the identifier and a newline after each.
  Tests the following:
  - no mark, one, two, and runs of three to five
  - a lone `@`
  - marks at either end, an empty identifier, an identifier of marks
  - several lines, and none
*/
bool
d_tests_arg_completion_script_lines(void)
{
    // expected
    //   the transcript, in the parts a C99 string literal can hold.
    const char* const expected[] =
    {
        SCRIPT_LINES,
        NULL
    };

    return d_tests_arg_completion_marks("SCRIPT_LINES",
                                        SCRIPT_LINES_ROWS,
                                        SCRIPT_LINES_COUNT,
                                        expected);
}

//==============================================================================
// 5.  DESCRIPTIONS
//==============================================================================

// LINE_TEXT_ROWS
//   constant: the cases of d_tests_arg_completion_line_text.
static const struct d_tests_arg_completion_text LINE_TEXT_ROWS[] =
{
    { "NULL", NULL },
    { "empty", "" },
    { "plain", "Do the thing" },
    { "two lines", "First line\nsecond line" },
    { "carriage return", "First\rsecond" },
    { "CRLF", "First\r\nsecond" },
    { "leading line break", "\nafter" },
    { "padded", "  padded  " },
    { "tabs", "\tin tabs\t" },
    { "every space", " \t\x0b\x0c text \t\x0b\x0c" },
    { "padded first line", "   one   \n   two   " },
    { "only spaces", "   " },
    { "only a line break", "\n" },
    { "inner spaces", "a  b\tc" },
    { "trailing break", "end\n" },
    { "vertical tab", "a\x0b" "b" },
    { "form feed", "a\x0c" "b" },
    { "control", "a\x01" "b" },
    { "UTF-8", "caf\xc3\xa9 \xe2\x86\x92 ok" },
    { "NBSP", "\xc2\xa0kept\xc2\xa0" }
};

// LINE_TEXT_COUNT
//   constant: how many cases LINE_TEXT_ROWS holds.
static const size_t LINE_TEXT_COUNT =
    sizeof(LINE_TEXT_ROWS) / sizeof(LINE_TEXT_ROWS[0]);

/*
d_tests_arg_completion_line_text
  The first line of a description, trimmed, as a script shows it beside a name.
  Tests the following:
  - NULL and empty text
  - to the first line feed or carriage return
  - every ASCII space trimmed from both ends, and nothing else
  - text past ASCII kept
*/
bool
d_tests_arg_completion_line_text(void)
{
    // expected
    //   the transcript, in the parts a C99 string literal can hold.
    const char* const expected[] =
    {
        LINE_TEXT,
        NULL
    };

    return d_tests_arg_completion_line_texts("LINE_TEXT",
                                             LINE_TEXT_ROWS,
                                             LINE_TEXT_COUNT,
                                             expected);
}

//==============================================================================
// 6.  REFUSALS
//==============================================================================

/**
 * @brief refusal_01: no command
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)_app;

    d_tests_arg_completion_try(_run,
                               "no command",
                               NULL,
                               0,
                               d_pack_text_from_cstr("p"),
                               false);

    return;
}

/**
 * @brief refusal_02: shell -1
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_tests_arg_completion_try(_run,
                               "shell -1",
                               _app,
                               -1,
                               d_pack_text_from_cstr("p"),
                               false);

    return;
}

/**
 * @brief refusal_03: shell COUNT
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_03(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_tests_arg_completion_try(_run,
                               "shell COUNT",
                               _app,
                               (int)D_ARG_SHELL_COUNT,
                               d_pack_text_from_cstr("p"),
                               false);

    return;
}

/**
 * @brief refusal_04: shell INT_MAX
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_04(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_tests_arg_completion_try(_run,
                               "shell INT_MAX",
                               _app,
                               INT_MAX,
                               d_pack_text_from_cstr("p"),
                               false);

    return;
}

/**
 * @brief refusal_05: shell INT_MIN
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_05(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_tests_arg_completion_try(_run,
                               "shell INT_MIN",
                               _app,
                               INT_MIN,
                               d_pack_text_from_cstr("p"),
                               false);

    return;
}

/**
 * @brief refusal_06: program without data
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_06(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_tests_arg_completion_try(_run,
                               "program without data",
                               _app,
                               0,
                               d_tests_arg_completion_exact(NULL,
                                                            3u),
                               false);

    return;
}

/**
 * @brief refusal_07: program without data, one byte long
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_07(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_tests_arg_completion_try(_run,
                               "program without data, one byte long",
                               _app,
                               0,
                               d_tests_arg_completion_exact(NULL,
                                                            1u),
                               false);

    return;
}

/**
 * @brief refusal_08: program without data or length
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_08(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    d_tests_arg_completion_try(_run,
                               "program without data or length",
                               _app,
                               0,
                               d_tests_arg_completion_exact(NULL,
                                                            0u),
                               false);

    return;
}

/**
 * @brief refusal_09: root unnamed
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_09(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_name(_app,
                                 "");
    d_tests_arg_completion_try(_run,
                               "root unnamed",
                               _app,
                               0,
                               d_pack_text_from_cstr(""),
                               false);

    return;
}

/**
 * @brief refusal_10: root named /
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_10(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_name(_app,
                                 "/");
    d_tests_arg_completion_try(_run,
                               "root named /",
                               _app,
                               1,
                               d_pack_text_from_cstr(""),
                               false);

    return;
}

/**
 * @brief refusal_11: root named dir\
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_11(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_name(_app,
                                 "dir\\");
    d_tests_arg_completion_try(_run,
                               "root named dir\\",
                               _app,
                               2,
                               d_pack_text_from_cstr(""),
                               false);

    return;
}

/**
 * @brief refusal_12: root unnamed, a program given
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_12(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_set_name(_app,
                                 "");
    d_tests_arg_completion_try(_run,
                               "root unnamed, a program given",
                               _app,
                               3,
                               d_pack_text_from_cstr("x"),
                               false);

    return;
}

/**
 * @brief refusal_13: the sink refuses
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_13(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    d_tests_arg_completion_try(_run,
                               "the sink refuses",
                               _app,
                               0,
                               d_pack_text_from_cstr(""),
                               true);

    return;
}

/**
 * @brief refusal_14: a listing fails
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_14(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_option* const   o  = d_arg_command_add_option(_app,
                                                               "--x",
                                                               NULL);
    const struct d_arg_validator v1 = d_tests_arg_completion_failing(0u);

    (void)d_arg_option_check(o,
                             v1,
                             NULL);
    d_tests_arg_completion_try(_run,
                               "a listing fails",
                               _app,
                               0,
                               d_pack_text_from_cstr(""),
                               false);

    return;
}

/**
 * @brief refusal_15: a listing fails after a value
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_15(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_option* const   o  = d_arg_command_add_option(_app,
                                                               "--x",
                                                               NULL);
    const struct d_arg_validator v1 = d_tests_arg_completion_failing(1u);

    (void)d_arg_option_check(o,
                             v1,
                             NULL);
    d_tests_arg_completion_try(_run,
                               "a listing fails after a value",
                               _app,
                               1,
                               d_pack_text_from_cstr(""),
                               false);

    return;
}

/**
 * @brief refusal_16: a listing fails in fish
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_16(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_option* const   o  = d_arg_command_add_option(_app,
                                                               "--x",
                                                               NULL);
    const struct d_arg_validator v1 = d_tests_arg_completion_failing(0u);

    (void)d_arg_option_check(o,
                             v1,
                             NULL);
    d_tests_arg_completion_try(_run,
                               "a listing fails in fish",
                               _app,
                               2,
                               d_pack_text_from_cstr(""),
                               false);

    return;
}

/**
 * @brief refusal_17: a listing fails in PowerShell
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_17(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_option* const   o  = d_arg_command_add_option(_app,
                                                               "x",
                                                               NULL);
    const struct d_arg_validator v1 = d_tests_arg_completion_failing(1u);

    (void)d_arg_option_check(o,
                             v1,
                             NULL);
    d_tests_arg_completion_try(_run,
                               "a listing fails in PowerShell",
                               _app,
                               3,
                               d_pack_text_from_cstr(""),
                               false);

    return;
}

/**
 * @brief refusal_18: a listing fails in a subcommand
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_18(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_command* const  s  = d_arg_command_add_subcommand(_app,
                                                                   "s",
                                                                   NULL);
    struct d_arg_option* const   o  = d_arg_command_add_option(s,
                                                               "--x",
                                                               NULL);
    const struct d_arg_validator v1 = d_tests_arg_completion_failing(1u);

    (void)d_arg_option_check(o,
                             v1,
                             NULL);
    d_tests_arg_completion_try(_run,
                               "a listing fails in a subcommand",
                               _app,
                               0,
                               d_pack_text_from_cstr(""),
                               false);

    return;
}

/**
 * @brief refusal_19: a listing fails, the sink refusing
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_19(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_option* const   o  = d_arg_command_add_option(_app,
                                                               "--x",
                                                               NULL);
    const struct d_arg_validator v1 = d_tests_arg_completion_failing(0u);

    (void)d_arg_option_check(o,
                             v1,
                             NULL);
    d_tests_arg_completion_try(_run,
                               "a listing fails, the sink refusing",
                               _app,
                               0,
                               d_pack_text_from_cstr(""),
                               true);

    return;
}

/**
 * @brief refusal_20: an inactive listing that would fail
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_20(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_option* const o  = d_arg_command_add_option(_app,
                                                             "--x",
                                                             NULL);
    struct d_arg_validator     v1 = d_tests_arg_completion_failing(0u);

    d_arg_validator_set_active(&v1,
                               false);
    (void)d_arg_option_check(o,
                             v1,
                             NULL);
    d_tests_arg_completion_try(_run,
                               "an inactive listing that would fail",
                               _app,
                               0,
                               d_pack_text_from_cstr(""),
                               false);

    return;
}

/**
 * @brief refusal_21: a hidden listing that would fail
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_21(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_option* const   o  = d_arg_command_add_option(_app,
                                                               "--x",
                                                               NULL);
    const struct d_arg_validator v1 = d_tests_arg_completion_failing(0u);

    (void)d_arg_option_check(o,
                             v1,
                             NULL);
    (void)d_arg_option_set_group(o,
                                 "");
    d_tests_arg_completion_try(_run,
                               "a hidden listing that would fail",
                               _app,
                               0,
                               d_pack_text_from_cstr(""),
                               false);

    return;
}

/**
 * @brief refusal_22: a hidden positional's listing
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_22(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_option* const   o  = d_arg_command_add_option(_app,
                                                               "x",
                                                               NULL);
    const struct d_arg_validator v1 = d_tests_arg_completion_failing(0u);

    (void)d_arg_option_check(o,
                             v1,
                             NULL);
    (void)d_arg_option_set_group(o,
                                 "");
    d_tests_arg_completion_try(_run,
                               "a hidden positional's listing",
                               _app,
                               0,
                               d_pack_text_from_cstr(""),
                               false);

    return;
}

/**
 * @brief refusal_23: a flag's listing
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_23(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_option* const   o  = d_arg_command_add_flag(_app,
                                                             "--x",
                                                             NULL);
    const struct d_arg_validator v1 = d_tests_arg_completion_failing(0u);

    (void)d_arg_option_check(o,
                             v1,
                             NULL);
    d_tests_arg_completion_try(_run,
                               "a flag's listing",
                               _app,
                               0,
                               d_pack_text_from_cstr(""),
                               false);

    return;
}

/**
 * @brief refusal_24: a listing for the second value
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_24(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_option* const o  = d_arg_command_add_option(_app,
                                                             "--x",
                                                             NULL);
    struct d_arg_validator     v1 = d_tests_arg_completion_failing(0u);

    d_arg_validator_set_application_index(&v1,
                                          1);
    (void)d_arg_option_check(o,
                             v1,
                             NULL);
    d_tests_arg_completion_try(_run,
                               "a listing for the second value",
                               _app,
                               0,
                               d_pack_text_from_cstr(""),
                               false);

    return;
}

/**
 * @brief refusal_25: a listing in a hidden subcommand
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_25(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_command* const s = d_arg_command_add_subcommand(_app,
                                                                 "s",
                                                                 NULL);

    (void)d_arg_command_set_group(s,
                                  "");

    struct d_arg_option* const   o  = d_arg_command_add_option(s,
                                                               "--x",
                                                               NULL);
    const struct d_arg_validator v1 = d_tests_arg_completion_failing(0u);

    (void)d_arg_option_check(o,
                             v1,
                             NULL);
    d_tests_arg_completion_try(_run,
                               "a listing in a hidden subcommand",
                               _app,
                               0,
                               d_pack_text_from_cstr(""),
                               false);

    return;
}

/**
 * @brief refusal_26: a listing in a disabled group
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_26(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_command* const  g  = d_arg_command_add_option_group(_app,
                                                                     "G",
                                                                     NULL);
    struct d_arg_option* const   o  = d_arg_command_add_option(g,
                                                               "--x",
                                                               NULL);
    const struct d_arg_validator v1 = d_tests_arg_completion_failing(0u);

    (void)d_arg_option_check(o,
                             v1,
                             NULL);
    d_arg_command_set_disabled(g,
                               true);
    d_tests_arg_completion_try(_run,
                               "a listing in a disabled group",
                               _app,
                               0,
                               d_pack_text_from_cstr(""),
                               false);

    return;
}

/**
 * @brief refusal_27: a subcommand
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_27(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_command* const s = d_arg_command_add_subcommand(_app,
                                                                 "sub",
                                                                 NULL);

    (void)d_arg_command_add_flag(s,
                                 "--y",
                                 NULL);
    d_tests_arg_completion_try(_run,
                               "a subcommand",
                               s,
                               0,
                               d_pack_text_from_cstr(""),
                               false);

    return;
}

/**
 * @brief refusal_28: a subcommand named by a path
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_28(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_command* const s = d_arg_command_add_subcommand(_app,
                                                                 "x.exe",
                                                                 NULL);

    d_tests_arg_completion_try(_run,
                               "a subcommand named by a path",
                               s,
                               0,
                               d_pack_text_from_cstr(""),
                               false);

    return;
}

/**
 * @brief refusal_29: the completion option called back with no value
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_refusal_29(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_option* const c =
        d_arg_parser_set_completion_flag(&_run->parser,
                                         "--completion",
                                         NULL);

    d_tests_arg_completion_called_back(
        _run,
        "the completion option called back with no value",
        c);

    return;
}

// REFUSALS_STEPS
//   constant: the cases of d_tests_arg_completion_refusals.
static const d_tests_arg_define_fn REFUSALS_STEPS[] =
{
    d_tests_arg_completion_refusal_01,
    d_tests_arg_completion_refusal_02,
    d_tests_arg_completion_refusal_03,
    d_tests_arg_completion_refusal_04,
    d_tests_arg_completion_refusal_05,
    d_tests_arg_completion_refusal_06,
    d_tests_arg_completion_refusal_07,
    d_tests_arg_completion_refusal_08,
    d_tests_arg_completion_refusal_09,
    d_tests_arg_completion_refusal_10,
    d_tests_arg_completion_refusal_11,
    d_tests_arg_completion_refusal_12,
    d_tests_arg_completion_refusal_13,
    d_tests_arg_completion_refusal_14,
    d_tests_arg_completion_refusal_15,
    d_tests_arg_completion_refusal_16,
    d_tests_arg_completion_refusal_17,
    d_tests_arg_completion_refusal_18,
    d_tests_arg_completion_refusal_19,
    d_tests_arg_completion_refusal_20,
    d_tests_arg_completion_refusal_21,
    d_tests_arg_completion_refusal_22,
    d_tests_arg_completion_refusal_23,
    d_tests_arg_completion_refusal_24,
    d_tests_arg_completion_refusal_25,
    d_tests_arg_completion_refusal_26,
    d_tests_arg_completion_refusal_27,
    d_tests_arg_completion_refusal_28,
    d_tests_arg_completion_refusal_29
};

// REFUSALS_COUNT
//   constant: how many cases REFUSALS_STEPS holds.
static const size_t REFUSALS_COUNT =
    sizeof(REFUSALS_STEPS) / sizeof(REFUSALS_STEPS[0]);

/*
d_tests_arg_completion_refusals
  What d_arg_completion_script refuses, and the status it returns when a listing
or the sink fails; the C call's alone.
  Tests the following:
  - no command, a shell that is none, a program span without data
  - no name to complete: a root named nothing, or by a path that ends in a
    separator
  - a sink that takes nothing: FileError, and nothing written
  - a listing that fails, at once or after a value, in every shell and in a
    subcommand: its status, and nothing written
  - a listing never asked for: inactive, hidden, a flag's, for a later value,
    under a hidden subcommand or a disabled group
  - a subcommand as the command, its own name the program
  - the completion option's callback given no value: nothing done, nothing
    recorded
*/
bool
d_tests_arg_completion_refusals(void)
{
    // expected
    //   the transcript, in the parts a C99 string literal can hold.
    const char* const expected[] =
    {
        REFUSALS,
        NULL
    };

    return d_tests_arg_completion_steps("REFUSALS",
                                        REFUSALS_STEPS,
                                        REFUSALS_COUNT,
                                        expected);
}

//==============================================================================
// 7.  PROGRAM NAMES
//==============================================================================

/**
 * @brief program_01: given
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_program_01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "given",
                                   "tool",
                                   "tool");

    return;
}

/**
 * @brief program_02: from the root
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_program_02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "from the root",
                                   "",
                                   "prog");

    return;
}

/**
 * @brief program_03: root path
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_program_03(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);
    (void)d_arg_command_set_name(_app,
                                 "/usr/local/bin/tool");
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "root path",
                                   "",
                                   "tool");

    return;
}

/**
 * @brief program_04: root backslash
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_program_04(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);
    (void)d_arg_command_set_name(_app,
                                 "C:\\Tools\\Tool.EXE");
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "root backslash",
                                   "",
                                   "Tool");

    return;
}

/**
 * @brief program_05: root .Exe twice
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_program_05(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);
    (void)d_arg_command_set_name(_app,
                                 "tool.Exe.exe");
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "root .Exe twice",
                                   "",
                                   "tool_2eExe");

    return;
}

/**
 * @brief program_06: root .exe alone
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_program_06(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);
    (void)d_arg_command_set_name(_app,
                                 ".exe");
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "root .exe alone",
                                   "",
                                   "_2eexe");

    return;
}

/**
 * @brief program_07: root mixed separators
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_program_07(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);
    (void)d_arg_command_set_name(_app,
                                 "a\\b/c\\d");
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "root mixed separators",
                                   "",
                                   "d");

    return;
}

/**
 * @brief program_08: given .exe
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_program_08(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "given .exe",
                                   "tool.exe",
                                   "tool_2eexe");

    return;
}

/**
 * @brief program_09: leading dash
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_program_09(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "leading dash",
                                   "-x",
                                   "_2dx");

    return;
}

/**
 * @brief program_10: space
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_program_10(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "space",
                                   "two words",
                                   "two_20words");

    return;
}

/**
 * @brief program_11: leading space
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_program_11(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "leading space",
                                   " x",
                                   "_20x");

    return;
}

/**
 * @brief program_12: tab
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_program_12(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "tab",
                                   "a\tb",
                                   "a_09b");

    return;
}

/**
 * @brief program_13: quote
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_program_13(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "quote",
                                   "it's",
                                   "it_27s");

    return;
}

/**
 * @brief program_14: backslash
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_program_14(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "backslash",
                                   "a\\b",
                                   "a_5cb");

    return;
}

/**
 * @brief program_15: UTF-8
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_program_15(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "UTF-8",
                                   "caf\xc3\xa9",
                                   "caf_c3_a9");

    return;
}

/**
 * @brief program_16: astral
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_program_16(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "astral",
                                   "\xf0\x9f\x98\x80",
                                   "_f0_9f_98_80");

    return;
}

/**
 * @brief program_17: stray byte
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_program_17(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "stray byte",
                                   "x\xffy",
                                   "x_ffy");

    return;
}

/**
 * @brief program_18: DEL
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_program_18(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "DEL",
                                   "a\x7f" "b",
                                   "a_7fb");

    return;
}

/**
 * @brief program_19: short .exe
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_program_19(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "short .exe",
                                   "ab",
                                   "ab");

    return;
}

/**
 * @brief program_20: EXE
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_program_20(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "EXE",
                                   "TOOL.EXE",
                                   "TOOL_2eEXE");

    return;
}

// PROGRAM_NAMES_STEPS
//   constant: the cases of d_tests_arg_completion_program_names.
static const d_tests_arg_define_fn PROGRAM_NAMES_STEPS[] =
{
    d_tests_arg_completion_program_01,
    d_tests_arg_completion_program_02,
    d_tests_arg_completion_program_03,
    d_tests_arg_completion_program_04,
    d_tests_arg_completion_program_05,
    d_tests_arg_completion_program_06,
    d_tests_arg_completion_program_07,
    d_tests_arg_completion_program_08,
    d_tests_arg_completion_program_09,
    d_tests_arg_completion_program_10,
    d_tests_arg_completion_program_11,
    d_tests_arg_completion_program_12,
    d_tests_arg_completion_program_13,
    d_tests_arg_completion_program_14,
    d_tests_arg_completion_program_15,
    d_tests_arg_completion_program_16,
    d_tests_arg_completion_program_17,
    d_tests_arg_completion_program_18,
    d_tests_arg_completion_program_19,
    d_tests_arg_completion_program_20
};

// PROGRAM_NAMES_COUNT
//   constant: how many cases PROGRAM_NAMES_STEPS holds.
static const size_t PROGRAM_NAMES_COUNT =
    sizeof(PROGRAM_NAMES_STEPS) / sizeof(PROGRAM_NAMES_STEPS[0]);

/*
d_tests_arg_completion_program_names
  Scripts for the root alone, for programs given and named by the root: where
each shell writes the name, and how it quotes it.
  Tests the following:
  - the program given, or the root's name after its last `/` or `\` less ".exe"
    in any case -- unless nothing else is left
  - the identifier the functions are named by
  - the header's comment, each control character a space -- or, in PowerShell's,
    each byte past printable ASCII a `?`
  - zsh's `#compdef` only for a name that may follow it
  - PowerShell's registration with ".exe" added unless the name ends so
  - the quoting of each shell, for quotes, backslashes and text past ASCII
*/
bool
d_tests_arg_completion_program_names(void)
{
    // expected
    //   the transcript, in the parts a C99 string literal can hold.
    const char* const expected[] =
    {
        PROGRAM_NAMES,
        PROGRAM_NAMES_2,
        PROGRAM_NAMES_3,
        PROGRAM_NAMES_4,
        PROGRAM_NAMES_5,
        PROGRAM_NAMES_6,
        PROGRAM_NAMES_7,
        PROGRAM_NAMES_8,
        PROGRAM_NAMES_9,
        PROGRAM_NAMES_10,
        PROGRAM_NAMES_11,
        PROGRAM_NAMES_12,
        PROGRAM_NAMES_13,
        PROGRAM_NAMES_14,
        PROGRAM_NAMES_15,
        PROGRAM_NAMES_16,
        NULL
    };

    return d_tests_arg_completion_steps("PROGRAM_NAMES",
                                        PROGRAM_NAMES_STEPS,
                                        PROGRAM_NAMES_COUNT,
                                        expected);
}

//==============================================================================
// 8.  THE ROOT ALONE
//==============================================================================

/**
 * @brief root_01: alone
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_root_01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "alone",
                                   "",
                                   "prog");

    return;
}

/**
 * @brief root_02: no help
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_root_02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "no help",
                                   "",
                                   "prog");

    return;
}

/**
 * @brief root_03: help renamed
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_root_03(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_set_help_flag(_app,
                                      "-?,--usage,--HELP",
                                      "Show it");
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "help renamed",
                                   "",
                                   "prog");

    return;
}

/**
 * @brief root_04: help-all
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_root_04(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_set_help_all_flag(_app,
                                          "--help-all",
                                          "Expand all help");
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "help-all",
                                   "",
                                   "prog");

    return;
}

/**
 * @brief root_05: described
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_root_05(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_add_flag(_app,
                                 "-x",
                                 "One\nTwo");
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "described",
                                   "",
                                   "prog");

    return;
}

// TREE_ROOT_STEPS
//   constant: the cases of d_tests_arg_completion_tree_root.
static const d_tests_arg_define_fn TREE_ROOT_STEPS[] =
{
    d_tests_arg_completion_root_01,
    d_tests_arg_completion_root_02,
    d_tests_arg_completion_root_03,
    d_tests_arg_completion_root_04,
    d_tests_arg_completion_root_05
};

// TREE_ROOT_COUNT
//   constant: how many cases TREE_ROOT_STEPS holds.
static const size_t TREE_ROOT_COUNT =
    sizeof(TREE_ROOT_STEPS) / sizeof(TREE_ROOT_STEPS[0]);

/*
d_tests_arg_completion_tree_root
  The root alone, with its help flags as they are made, renamed and removed.
  Tests the following:
  - the data every script holds for one command and its help flag
  - a help flag renamed, removed, and a help-all flag beside it
  - a description cut to its first line
*/
bool
d_tests_arg_completion_tree_root(void)
{
    // expected
    //   the transcript, in the parts a C99 string literal can hold.
    const char* const expected[] =
    {
        TREE_ROOT,
        TREE_ROOT_2,
        TREE_ROOT_3,
        TREE_ROOT_4,
        TREE_ROOT_5,
        NULL
    };

    return d_tests_arg_completion_steps("TREE_ROOT",
                                        TREE_ROOT_STEPS,
                                        TREE_ROOT_COUNT,
                                        expected);
}

//==============================================================================
// 9.  OPTIONS
//==============================================================================

/**
 * @brief options_01: names
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_options_01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_add_flag(_app,
                                 "-v,--verbose",
                                 "Say more");
    (void)d_arg_command_add_flag(_app,
                                 "-q",
                                 "Say less");
    (void)d_arg_command_add_flag(_app,
                                 "--dry-run",
                                 "Pretend");
    (void)d_arg_command_add_option(_app,
                                   "-a,-b,--alpha,--beta",
                                   "Two of each");
    (void)d_arg_command_add_option(_app,
                                   "--beta2,-c",
                                   "Long before short");
    (void)d_arg_command_add_flag(_app,
                                 "-z,--zed,-y,--why",
                                 NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "names",
                                   "",
                                   "prog");

    return;
}

/**
 * @brief options_02: counts
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_options_02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_add_option(_app,
                                   "-n,--count",
                                   "One value");

    struct d_arg_option* const two = d_arg_command_add_option(_app,
                                                              "--pair",
                                                              "Two values");

    d_arg_option_set_expected(two,
                              2);

    struct d_arg_option* const rng = d_arg_command_add_option(_app,
                                                              "--range",
                                                              "One to three");

    d_arg_option_set_expected_range(rng,
                                    1,
                                    3);

    struct d_arg_option* const rev = d_arg_command_add_option(_app,
                                                              "--reversed",
                                                              "Three to one");

    d_arg_option_set_expected_range(rev,
                                    3,
                                    1);

    struct d_arg_option* const many = d_arg_command_add_option(_app,
                                                               "--many",
                                                               "At least one");

    d_arg_option_set_expected(many,
                              -1);

    struct d_arg_option* const two_plus =
        d_arg_command_add_option(_app,
                                 "--two-plus",
                                 "At least two");

    d_arg_option_set_expected(two_plus,
                              -2);

    struct d_arg_option* const un = d_arg_command_add_option(_app,
                                                             "--unlimited",
                                                             "Any number");

    d_arg_option_set_expected(un,
                              (int)D_ARG_EXPECTED_UNLIMITED);

    struct d_arg_option* const ex = d_arg_command_add_option(_app,
                                                             "--extra",
                                                             "Extra arguments");

    d_arg_option_set_allow_extra_args(ex,
                                      true);

    struct d_arg_option* const zero = d_arg_command_add_option(_app,
                                                               "--zero",
                                                               "Takes none");

    d_arg_option_set_expected(zero,
                              0);

    struct d_arg_option* const ts =
        d_arg_command_add_option(_app,
                                 "--triple",
                                 "Three to an item");

    d_arg_option_set_type_size(ts,
                               3);

    struct d_arg_option* const tsr =
        d_arg_command_add_option(_app,
                                 "--sized",
                                 "One or two to an item");

    d_arg_option_set_type_size_range(tsr,
                                     1,
                                     2);

    struct d_arg_option* const tsx =
        d_arg_command_add_option(_app,
                                 "--sized-twice",
                                 "Two items of one or two");

    d_arg_option_set_type_size_range(tsx,
                                     1,
                                     2);
    d_arg_option_set_expected(tsx,
                              2);

    struct d_arg_option* const tsn =
        d_arg_command_add_option(_app,
                                 "--any-size",
                                 "Items of any size");

    d_arg_option_set_type_size(tsn,
                               -1);

    struct d_arg_option* const tsu =
        d_arg_command_add_option(_app,
                                 "--any-items",
                                 "Items of any length");

    d_arg_option_set_type_size(tsu,
                               (int)D_ARG_EXPECTED_UNLIMITED);
    (void)d_arg_command_add_flag(_app,
                                 "--flag",
                                 "A flag");

    struct d_arg_option* const big =
        d_arg_command_add_option(_app,
                                 "--big",
                                 "A count past reason");

    d_arg_option_set_expected_range(big,
                                    1,
                                    (int)D_ARG_EXPECTED_UNLIMITED - 1);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "counts",
                                   "",
                                   "prog");

    return;
}

/**
 * @brief options_03: odd names
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_options_03(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_add_flag(_app,
                                 "--it's",
                                 "Quote's here");
    (void)d_arg_command_add_flag(_app,
                                 "--back\\slash",
                                 "A backslash");
    (void)d_arg_command_add_flag(_app,
                                 "--dollar$",
                                 "A dollar");
    (void)d_arg_command_add_flag(_app,
                                 "--tick`",
                                 "A back quote");
    (void)d_arg_command_add_flag(_app,
                                 "--quote\"d",
                                 "A double quote");
    (void)d_arg_command_add_flag(_app,
                                 "--caf\xc3\xa9",
                                 "Caf\xc3\xa9");
    (void)d_arg_command_add_flag(_app,
                                 "--a\tb",
                                 "A tab");
    (void)d_arg_command_add_flag(_app,
                                 "--del\x7f",
                                 "A delete");
    (void)d_arg_command_add_flag(_app,
                                 "--\xf0\x9f\x98\x80",
                                 "Astral");
    (void)d_arg_command_add_flag(_app,
                                 "--semi;colon",
                                 "A semicolon");
    (void)d_arg_command_add_flag(_app,
                                 "--star*",
                                 "A star");
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "odd names",
                                   "",
                                   "prog");

    return;
}

// TREE_OPTIONS_STEPS
//   constant: the cases of d_tests_arg_completion_tree_options.
static const d_tests_arg_define_fn TREE_OPTIONS_STEPS[] =
{
    d_tests_arg_completion_options_01,
    d_tests_arg_completion_options_02,
    d_tests_arg_completion_options_03
};

// TREE_OPTIONS_COUNT
//   constant: how many cases TREE_OPTIONS_STEPS holds.
static const size_t TREE_OPTIONS_COUNT =
    sizeof(TREE_OPTIONS_STEPS) / sizeof(TREE_OPTIONS_STEPS[0]);

/*
d_tests_arg_completion_tree_options
  Options' names and the values a name takes.
  Tests the following:
  - short names before long ones, in the order given
  - the fewest values a name takes, as CLI11 takes them, and the most, unlimited
    for extra arguments or a count past reason
  - a flag, and an option made flag-like, take none
  - names holding what a shell must quote, and text past ASCII
*/
bool
d_tests_arg_completion_tree_options(void)
{
    // expected
    //   the transcript, in the parts a C99 string literal can hold.
    const char* const expected[] =
    {
        TREE_OPTIONS,
        TREE_OPTIONS_2,
        TREE_OPTIONS_3,
        TREE_OPTIONS_4,
        TREE_OPTIONS_5,
        TREE_OPTIONS_6,
        NULL
    };

    return d_tests_arg_completion_steps("TREE_OPTIONS",
                                        TREE_OPTIONS_STEPS,
                                        TREE_OPTIONS_COUNT,
                                        expected);
}

//==============================================================================
// 10. VALUES
//==============================================================================

/**
 * @brief values_01: choices
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_values_01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_option* const   c1 = d_arg_command_add_option(_app,
                                                               "--color",
                                                               "Pick one");
    const struct d_arg_validator v1 = d_tests_arg_completion_listing(CHOICES_1);

    (void)d_arg_option_check(c1,
                             v1,
                             NULL);

    struct d_arg_option* const   c2 = d_arg_command_add_option(_app,
                                                               "--shade",
                                                               "Two lists");
    const struct d_arg_validator v2 = d_tests_arg_completion_listing(CHOICES_2);

    (void)d_arg_option_check(c2,
                             v2,
                             NULL);

    const struct d_arg_validator v3 = d_tests_arg_completion_listing(CHOICES_3);

    (void)d_arg_option_check(c2,
                             v3,
                             NULL);

    struct d_arg_option* const   c3 =
        d_arg_command_add_option(_app,
                                 "--mixed",
                                 "Transformed first");
    const struct d_arg_validator v4 = d_tests_arg_completion_listing(CHOICES_4);

    (void)d_arg_option_check(c3,
                             v4,
                             NULL);

    const struct d_arg_validator v5 = d_tests_arg_completion_listing(CHOICES_5);

    (void)d_arg_option_transform(c3,
                                 v5,
                                 NULL,
                                 NULL);

    struct d_arg_option* const c4 =
        d_arg_command_add_option(_app,
                                 "--off",
                                 "An inactive list");
    struct d_arg_validator     v6 = d_tests_arg_completion_listing(CHOICES_6);

    d_arg_validator_set_active(&v6,
                               false);
    (void)d_arg_option_check(c4,
                             v6,
                             NULL);

    struct d_arg_option* const c5 =
        d_arg_command_add_option(_app,
                                 "--later",
                                 "A list for the second value");
    struct d_arg_validator     v7 = d_tests_arg_completion_listing(CHOICES_7);

    d_arg_validator_set_application_index(&v7,
                                          1);
    (void)d_arg_option_check(c5,
                             v7,
                             NULL);

    struct d_arg_option* const c6 =
        d_arg_command_add_option(_app,
                                 "--first",
                                 "A list for the first value");
    struct d_arg_validator     v8 = d_tests_arg_completion_listing(CHOICES_8);

    d_arg_validator_set_application_index(&v8,
                                          0);
    (void)d_arg_option_check(c6,
                             v8,
                             NULL);

    struct d_arg_option* const   c7 = d_arg_command_add_option(_app,
                                                               "--empty",
                                                               "An empty list");
    const struct d_arg_validator v9 = d_tests_arg_completion_listing(CHOICES_9);

    (void)d_arg_option_check(c7,
                             v9,
                             NULL);

    struct d_arg_option* const   c8  =
        d_arg_command_add_option(_app,
                                 "--same",
                                 "The same twice");
    const struct d_arg_validator v10 =
        d_tests_arg_completion_listing(CHOICES_10);

    (void)d_arg_option_check(c8,
                             v10,
                             NULL);

    struct d_arg_option* const   c9  =
        d_arg_command_add_option(_app,
                                 "--shared",
                                 "A value another lists");
    const struct d_arg_validator v11 =
        d_tests_arg_completion_listing(CHOICES_11);

    (void)d_arg_option_check(c9,
                             v11,
                             NULL);

    struct d_arg_option* const   f1  =
        d_arg_command_add_flag(_app,
                               "--flag",
                               "A flag with a list");
    const struct d_arg_validator v12 =
        d_tests_arg_completion_listing(CHOICES_12);

    (void)d_arg_option_check(f1,
                             v12,
                             NULL);

    struct d_arg_option* const   e1  =
        d_arg_command_add_option(_app,
                                 "--case",
                                 "Values told apart by case");
    const struct d_arg_validator v13 =
        d_tests_arg_completion_listing(CHOICES_13);

    (void)d_arg_option_check(e1,
                             v13,
                             NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "choices",
                                   "",
                                   "prog");

    return;
}

/**
 * @brief values_02: kinds
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_values_02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_add_option(_app,
                                   "--plain",
                                   "No validator");

    struct d_arg_option* const   p1 = d_arg_command_add_option(_app,
                                                               "--file",
                                                               "A path");
    const struct d_arg_validator v1 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_PATH);

    (void)d_arg_option_check(p1,
                             v1,
                             NULL);

    struct d_arg_option* const   p2 = d_arg_command_add_option(_app,
                                                               "--dir",
                                                               "A directory");
    const struct d_arg_validator v2 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_DIRECTORY);

    (void)d_arg_option_check(p2,
                             v2,
                             NULL);

    struct d_arg_option* const   p3 = d_arg_command_add_option(_app,
                                                               "--num",
                                                               "A number");
    const struct d_arg_validator v3 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_NUMBER);

    (void)d_arg_option_check(p3,
                             v3,
                             NULL);

    struct d_arg_option* const   p4 =
        d_arg_command_add_option(_app,
                                 "--path-number",
                                 "A path, then a number");
    const struct d_arg_validator v4 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_PATH);

    (void)d_arg_option_check(p4,
                             v4,
                             NULL);

    const struct d_arg_validator v5 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_NUMBER);

    (void)d_arg_option_check(p4,
                             v5,
                             NULL);

    struct d_arg_option* const   p5 =
        d_arg_command_add_option(_app,
                                 "--path-dir",
                                 "A path, then a directory");
    const struct d_arg_validator v6 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_PATH);

    (void)d_arg_option_check(p5,
                             v6,
                             NULL);

    const struct d_arg_validator v7 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_DIRECTORY);

    (void)d_arg_option_check(p5,
                             v7,
                             NULL);

    struct d_arg_option* const   p6 = d_arg_command_add_option(_app,
                                                               "--range",
                                                               "A range");
    const struct d_arg_validator v8 = d_arg_validator_range_long(1L,
                                                                 9L);

    (void)d_arg_option_check(p6,
                             v8,
                             NULL);

    struct d_arg_option* const   p7 =
        d_arg_command_add_option(_app,
                                 "--listed-dir",
                                 "A list and a directory");
    const struct d_arg_validator v9 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_DIRECTORY);

    (void)d_arg_option_check(p7,
                             v9,
                             NULL);

    const struct d_arg_validator v10 =
        d_tests_arg_completion_listing(CHOICES_14);

    (void)d_arg_option_check(p7,
                             v10,
                             NULL);

    struct d_arg_option* const p8  =
        d_arg_command_add_option(_app,
                                 "--inactive-number",
                                 "An inactive number");
    struct d_arg_validator     v11 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_NUMBER);

    d_arg_validator_set_active(&v11,
                               false);
    (void)d_arg_option_check(p8,
                             v11,
                             NULL);

    struct d_arg_option* const p9  =
        d_arg_command_add_option(_app,
                                 "--second-dir",
                                 "A directory for the second");
    struct d_arg_validator     v12 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_DIRECTORY);

    d_arg_validator_set_application_index(&v12,
                                          1);
    (void)d_arg_option_check(p9,
                             v12,
                             NULL);

    struct d_arg_option* const p10 =
        d_arg_command_add_option(_app,
                                 "--dir-first",
                                 "A directory for the first");
    struct d_arg_validator     v13 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_DIRECTORY);

    d_arg_validator_set_application_index(&v13,
                                          0);
    (void)d_arg_option_check(p10,
                             v13,
                             NULL);

    struct d_arg_option* const p11 =
        d_arg_command_add_option(_app,
                                 "--every",
                                 "Every value a directory");
    struct d_arg_validator     v14 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_DIRECTORY);

    d_arg_validator_set_application_index(&v14,
                                          -1);
    (void)d_arg_option_check(p11,
                             v14,
                             NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "kinds",
                                   "",
                                   "prog");

    return;
}

/**
 * @brief values_03: composites
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_values_03(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_option* const   a1 =
        d_arg_command_add_option(_app,
                                 "--and",
                                 "A list and a number");
    const struct d_arg_validator v1 =
        d_tests_arg_completion_listing(CHOICES_15);
    const struct d_arg_validator v2 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_NUMBER);
    const struct d_arg_validator v3 = d_arg_validator_and(v1,
                                                          v2);

    (void)d_arg_option_check(a1,
                             v3,
                             NULL);

    struct d_arg_option* const   a2 =
        d_arg_command_add_option(_app,
                                 "--and-second",
                                 "A directory and a list");
    const struct d_arg_validator v4 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_DIRECTORY);
    const struct d_arg_validator v5 = d_tests_arg_completion_listing(CHOICES_6);
    const struct d_arg_validator v6 = d_arg_validator_and(v4,
                                                          v5);

    (void)d_arg_option_check(a2,
                             v6,
                             NULL);

    struct d_arg_option* const   a3 =
        d_arg_command_add_option(_app,
                                 "--and-empty",
                                 "An empty list first");
    const struct d_arg_validator v7 = d_tests_arg_completion_listing(CHOICES_9);
    const struct d_arg_validator v8 =
        d_tests_arg_completion_listing(CHOICES_16);
    const struct d_arg_validator v9 = d_arg_validator_and(v7,
                                                          v8);

    (void)d_arg_option_check(a3,
                             v9,
                             NULL);

    struct d_arg_option* const   a4  =
        d_arg_command_add_option(_app,
                                 "--and-kinds",
                                 "A path and a directory");
    const struct d_arg_validator v10 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_PATH);
    const struct d_arg_validator v11 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_DIRECTORY);
    const struct d_arg_validator v12 = d_arg_validator_and(v10,
                                                           v11);

    (void)d_arg_option_check(a4,
                             v12,
                             NULL);

    struct d_arg_option* const   a5  =
        d_arg_command_add_option(_app,
                                 "--and-number",
                                 "A path and a number");
    const struct d_arg_validator v13 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_PATH);
    const struct d_arg_validator v14 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_NUMBER);
    const struct d_arg_validator v15 = d_arg_validator_and(v13,
                                                           v14);

    (void)d_arg_option_check(a5,
                             v15,
                             NULL);

    struct d_arg_option* const   o1  = d_arg_command_add_option(_app,
                                                                "--or",
                                                                "Two lists");
    const struct d_arg_validator v16 =
        d_tests_arg_completion_listing(CHOICES_17);
    const struct d_arg_validator v17 =
        d_tests_arg_completion_listing(CHOICES_18);
    const struct d_arg_validator v18 = d_arg_validator_or(v16,
                                                          v17);

    (void)d_arg_option_check(o1,
                             v18,
                             NULL);

    struct d_arg_option* const   o2  =
        d_arg_command_add_option(_app,
                                 "--or-dir",
                                 "A number or a directory");
    const struct d_arg_validator v19 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_NUMBER);
    const struct d_arg_validator v20 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_DIRECTORY);
    const struct d_arg_validator v21 = d_arg_validator_or(v19,
                                                          v20);

    (void)d_arg_option_check(o2,
                             v21,
                             NULL);

    struct d_arg_option* const   o3  = d_arg_command_add_option(_app,
                                                                "--or-numbers",
                                                                "Two numbers");
    const struct d_arg_validator v22 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_NUMBER);
    const struct d_arg_validator v23 = d_arg_validator_range_long(0L,
                                                                  1L);
    const struct d_arg_validator v24 = d_arg_validator_or(v22,
                                                          v23);

    (void)d_arg_option_check(o3,
                             v24,
                             NULL);

    struct d_arg_option* const   o4  =
        d_arg_command_add_option(_app,
                                 "--or-paths",
                                 "A file or a directory");
    const struct d_arg_validator v25 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_PATH);
    const struct d_arg_validator v26 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_DIRECTORY);
    const struct d_arg_validator v27 = d_arg_validator_or(v25,
                                                          v26);

    (void)d_arg_option_check(o4,
                             v27,
                             NULL);

    struct d_arg_option* const   o5  =
        d_arg_command_add_option(_app,
                                 "--or-dirs",
                                 "Two directories");
    const struct d_arg_validator v28 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_DIRECTORY);
    const struct d_arg_validator v29 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_DIRECTORY);
    const struct d_arg_validator v30 = d_arg_validator_or(v28,
                                                          v29);

    (void)d_arg_option_check(o5,
                             v30,
                             NULL);

    struct d_arg_option* const   o6  =
        d_arg_command_add_option(_app,
                                 "--or-second",
                                 "A list second");
    const struct d_arg_validator v31 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_NUMBER);
    const struct d_arg_validator v32 =
        d_tests_arg_completion_listing(CHOICES_19);
    const struct d_arg_validator v33 = d_arg_validator_or(v31,
                                                          v32);

    (void)d_arg_option_check(o6,
                             v33,
                             NULL);

    struct d_arg_option* const   n1  = d_arg_command_add_option(_app,
                                                                "--not",
                                                                "Not a list");
    const struct d_arg_validator v34 =
        d_tests_arg_completion_listing(CHOICES_4);
    const struct d_arg_validator v35 = d_arg_validator_not(v34);

    (void)d_arg_option_check(n1,
                             v35,
                             NULL);

    struct d_arg_option* const   n2  = d_arg_command_add_option(_app,
                                                                "--not-number",
                                                                "Not a number");
    const struct d_arg_validator v36 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_NUMBER);
    const struct d_arg_validator v37 = d_arg_validator_not(v36);

    (void)d_arg_option_check(n2,
                             v37,
                             NULL);

    struct d_arg_option* const i1  =
        d_arg_command_add_option(_app,
                                 "--and-inactive",
                                 "An inactive operand");
    struct d_arg_validator     v38 = d_tests_arg_completion_listing(CHOICES_20);

    d_arg_validator_set_active(&v38,
                               false);

    const struct d_arg_validator v39 =
        d_tests_arg_completion_listing(CHOICES_21);
    const struct d_arg_validator v40 = d_arg_validator_and(v38,
                                                           v39);

    (void)d_arg_option_check(i1,
                             v40,
                             NULL);

    struct d_arg_option* const i2  =
        d_arg_command_add_option(_app,
                                 "--or-inactive",
                                 "An inactive operand");
    struct d_arg_validator     v41 = d_tests_arg_completion_listing(CHOICES_20);

    d_arg_validator_set_active(&v41,
                               false);

    const struct d_arg_validator v42 =
        d_tests_arg_completion_listing(CHOICES_21);
    const struct d_arg_validator v43 = d_arg_validator_or(v41,
                                                          v42);

    (void)d_arg_option_check(i2,
                             v43,
                             NULL);

    struct d_arg_option* const i3  =
        d_arg_command_add_option(_app,
                                 "--and-later",
                                 "A composite for the second");
    struct d_arg_validator     v44 = d_tests_arg_completion_listing(CHOICES_22);

    d_arg_validator_set_application_index(&v44,
                                          1);

    const struct d_arg_validator v45 =
        d_tests_arg_completion_listing(CHOICES_23);
    const struct d_arg_validator v46 = d_arg_validator_and(v44,
                                                           v45);

    (void)d_arg_option_check(i3,
                             v46,
                             NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "composites",
                                   "",
                                   "prog");

    return;
}

/**
 * @brief values_04: types
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_values_04(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_tests_arg_completion_bound(_run,
                                       _app,
                                       "--int",
                                       "An int",
                                       (int)D_ARG_BUILTIN_INT);
    (void)d_tests_arg_completion_bound(_run,
                                       _app,
                                       "--unsigned",
                                       "An unsigned",
                                       (int)D_ARG_BUILTIN_UNSIGNED_LONG);
    (void)d_tests_arg_completion_bound(_run,
                                       _app,
                                       "--double",
                                       "A double",
                                       (int)D_ARG_BUILTIN_DOUBLE);
    (void)d_tests_arg_completion_bound(_run,
                                       _app,
                                       "--bool",
                                       "A truth value",
                                       (int)D_ARG_BUILTIN_BOOL);
    (void)d_tests_arg_completion_bound(_run,
                                       _app,
                                       "--char",
                                       "A character",
                                       (int)D_ARG_BUILTIN_CHAR);
    (void)d_tests_arg_completion_bound(_run,
                                       _app,
                                       "--string",
                                       "A string",
                                       (int)D_ARG_BUILTIN_STRING);
    (void)d_tests_arg_completion_bound(_run,
                                       _app,
                                       "--text",
                                       "A text",
                                       (int)D_ARG_BUILTIN_TEXT);

    struct d_arg_option* const   b1 =
        d_tests_arg_completion_bound(_run,
                                     _app,
                                     "--int-choice",
                                     "An int from a list",
                                     (int)D_ARG_BUILTIN_INT);
    const struct d_arg_validator v1 =
        d_tests_arg_completion_listing(CHOICES_24);

    (void)d_arg_option_check(b1,
                             v1,
                             NULL);

    struct d_arg_option* const   b2 =
        d_tests_arg_completion_bound(_run,
                                     _app,
                                     "--int-path",
                                     "An int a path",
                                     (int)D_ARG_BUILTIN_INT);
    const struct d_arg_validator v2 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_PATH);

    (void)d_arg_option_check(b2,
                             v2,
                             NULL);

    struct d_arg_option* const   b3 =
        d_tests_arg_completion_bound(_run,
                                     _app,
                                     "--string-number",
                                     "A string a number",
                                     (int)D_ARG_BUILTIN_STRING);
    const struct d_arg_validator v3 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_NUMBER);

    (void)d_arg_option_check(b3,
                             v3,
                             NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "types",
                                   "",
                                   "prog");

    return;
}

/**
 * @brief values_05: own types
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_values_05(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_tests_arg_completion_typed(_run,
                                       _app,
                                       "--enum",
                                       "An enumeration",
                                       D_ARG_TYPE_FLAG_ENUM);
    (void)d_tests_arg_completion_typed(
        _run,
        _app,
        "--ints",
        "Integers",
        D_ARG_TYPE_FLAG_CONTAINER | D_ARG_TYPE_FLAG_INTEGER);
    (void)d_tests_arg_completion_typed(
        _run,
        _app,
        "--strings",
        "Strings",
        D_ARG_TYPE_FLAG_CONTAINER | D_ARG_TYPE_FLAG_STRING);
    (void)d_tests_arg_completion_typed(_run,
                                       _app,
                                       "--untyped",
                                       "No flags",
                                       0u);
    (void)d_tests_arg_completion_typed(_run,
                                       _app,
                                       "--signed",
                                       "Signed alone",
                                       D_ARG_TYPE_FLAG_SIGNED);
    (void)d_tests_arg_completion_typed(
        _run,
        _app,
        "--floats",
        "Floating point numbers",
        D_ARG_TYPE_FLAG_CONTAINER | D_ARG_TYPE_FLAG_FLOAT);

    struct d_arg_option* const   t1 =
        d_tests_arg_completion_typed(_run,
                                     _app,
                                     "--enum-choice",
                                     "An enumeration listed",
                                     D_ARG_TYPE_FLAG_ENUM);
    const struct d_arg_validator v1 =
        d_tests_arg_completion_listing(CHOICES_25);

    (void)d_arg_option_check(t1,
                             v1,
                             NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "own types",
                                   "",
                                   "prog");

    return;
}

/**
 * @brief values_06: hidden
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_values_06(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_option* const   h1 = d_arg_command_add_option(_app,
                                                               "--secret",
                                                               "Hidden");
    const struct d_arg_validator v1 =
        d_tests_arg_completion_listing(CHOICES_26);

    (void)d_arg_option_check(h1,
                             v1,
                             NULL);
    (void)d_arg_option_set_group(h1,
                                 "");

    struct d_arg_option* const   h2 =
        d_arg_command_add_option(_app,
                                 "hidden-pos",
                                 "A hidden positional");
    const struct d_arg_validator v2 =
        d_tests_arg_completion_listing(CHOICES_27);

    (void)d_arg_option_check(h2,
                             v2,
                             NULL);
    (void)d_arg_option_set_group(h2,
                                 "");

    struct d_arg_option* const h3 =
        d_arg_command_add_option(_app,
                                 "-o,out",
                                 "Named and positional, hidden");

    (void)d_arg_option_set_group(h3,
                                 "");

    struct d_arg_option* const   h4 =
        d_arg_command_add_option(_app,
                                 "-s,shown",
                                 "Named and positional, shown");
    const struct d_arg_validator v3 =
        d_tests_arg_completion_listing(CHOICES_28);

    (void)d_arg_option_check(h4,
                             v3,
                             NULL);

    struct d_arg_option* const   h5 =
        d_arg_command_add_option(_app,
                                 "last",
                                 "Shown positional");
    const struct d_arg_validator v4 =
        d_tests_arg_completion_kind(D_ARG_VALIDATOR_DIRECTORY);

    (void)d_arg_option_check(h5,
                             v4,
                             NULL);

    struct d_arg_option* const h6 = d_arg_command_add_flag(_app,
                                                           "--hidden-flag",
                                                           "A hidden flag");

    (void)d_arg_option_set_group(h6,
                                 "");

    struct d_arg_option* const h7 = d_arg_command_add_flag(_app,
                                                           "--regrouped",
                                                           "In another group");

    (void)d_arg_option_set_group(h7,
                                 "Other");
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "hidden",
                                   "",
                                   "prog");

    return;
}

// TREE_VALUES_STEPS
//   constant: the cases of d_tests_arg_completion_tree_values.
static const d_tests_arg_define_fn TREE_VALUES_STEPS[] =
{
    d_tests_arg_completion_values_01,
    d_tests_arg_completion_values_02,
    d_tests_arg_completion_values_03,
    d_tests_arg_completion_values_04,
    d_tests_arg_completion_values_05,
    d_tests_arg_completion_values_06
};

// TREE_VALUES_COUNT
//   constant: how many cases TREE_VALUES_STEPS holds.
static const size_t TREE_VALUES_COUNT =
    sizeof(TREE_VALUES_STEPS) / sizeof(TREE_VALUES_STEPS[0]);

/*
d_tests_arg_completion_tree_values
  How values complete: as the choices validators list, files, directories, or
nothing.
  Tests the following:
  - the choices of each validator that applies to the first value, once each,
    transformations first
  - an inactive validator, or one for a later value, ignored
  - a number before a path, a directory before a file, a list before either
  - composites listing and completing as their operands do
  - bound types: numbers, truth values, characters and enumerations complete as
    nothing, strings as files
  - a hidden option left out, a hidden positional kept, completing as nothing
*/
bool
d_tests_arg_completion_tree_values(void)
{
    // expected
    //   the transcript, in the parts a C99 string literal can hold.
    const char* const expected[] =
    {
        TREE_VALUES,
        TREE_VALUES_2,
        TREE_VALUES_3,
        TREE_VALUES_4,
        TREE_VALUES_5,
        TREE_VALUES_6,
        TREE_VALUES_7,
        TREE_VALUES_8,
        TREE_VALUES_9,
        TREE_VALUES_10,
        TREE_VALUES_11,
        NULL
    };

    return d_tests_arg_completion_steps("TREE_VALUES",
                                        TREE_VALUES_STEPS,
                                        TREE_VALUES_COUNT,
                                        expected);
}

//==============================================================================
// 11. COMMANDS
//==============================================================================

/**
 * @brief commands_01: subcommands
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_commands_01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_command* const a =
        d_arg_command_add_subcommand(_app,
                                     "alpha",
                                     "First\nsecond line");

    (void)d_arg_command_add_alias(a,
                                  "al");
    (void)d_arg_command_add_alias(a,
                                  "a-lias");

    struct d_arg_command* const b = d_arg_command_add_subcommand(_app,
                                                                 "beta",
                                                                 "  padded  ");
    struct d_arg_command* const d = d_arg_command_add_subcommand(_app,
                                                                 "disabled",
                                                                 "Disabled");

    d_arg_command_set_disabled(d,
                               true);

    struct d_arg_command* const h = d_arg_command_add_subcommand(_app,
                                                                 "hidden",
                                                                 "Hidden");

    (void)d_arg_command_set_group(h,
                                  "");

    struct d_arg_command* const a1 = d_arg_command_add_subcommand(a,
                                                                  "one",
                                                                  NULL);

    (void)d_arg_command_add_subcommand(a,
                                       "two",
                                       "Two");
    (void)d_arg_command_add_subcommand(b,
                                       "three",
                                       "Three");
    (void)d_arg_command_add_flag(a1,
                                 "--deep",
                                 "Deep");
    (void)d_arg_command_add_subcommand(h,
                                       "under-hidden",
                                       NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "subcommands",
                                   "",
                                   "prog");

    return;
}

/**
 * @brief commands_02: disabled
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_commands_02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_command* const d = d_arg_command_add_subcommand(_app,
                                                                 "off",
                                                                 "Off");

    d_arg_command_set_disabled(d,
                               true);
    (void)d_arg_command_add_subcommand(d,
                                       "below",
                                       NULL);

    struct d_arg_command* const e = d_arg_command_add_subcommand(_app,
                                                                 "on",
                                                                 "On");

    (void)d_arg_command_add_alias(e,
                                  "yes");
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "disabled",
                                   "",
                                   "prog");

    return;
}

/**
 * @brief commands_03: groups
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_commands_03(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_set_help_all_flag(_app,
                                          "--help-all",
                                          "Expand all help");

    struct d_arg_command* const g = d_arg_command_add_option_group(_app,
                                                                   "Group",
                                                                   "A group");

    (void)d_arg_command_add_subcommand(g,
                                       "gamma",
                                       "In a group");
    (void)d_arg_command_add_flag(g,
                                 "--gflag",
                                 "A group flag");

    struct d_arg_command* const ng =
        d_arg_command_add_option_group(g,
                                       "Nested",
                                       "A group in a group");

    (void)d_arg_command_add_subcommand(ng,
                                       "delta",
                                       "Nested deeper");
    (void)d_arg_command_add_flag(ng,
                                 "--nflag",
                                 "A nested flag");

    struct d_arg_command* const a  =
        d_arg_command_add_subcommand(_app,
                                     "alpha",
                                     "After the group");
    struct d_arg_command* const hg =
        d_arg_command_add_option_group(_app,
                                       "",
                                       "A hidden group");

    (void)d_arg_command_add_subcommand(hg,
                                       "hidden-sub",
                                       NULL);
    (void)d_arg_command_add_flag(hg,
                                 "--hidden-gflag",
                                 NULL);

    struct d_arg_command* const pg =
        d_arg_command_add_option_group(_app,
                                       "+Plus",
                                       "A plus group");

    (void)d_arg_command_add_flag(pg,
                                 "--pflag",
                                 "A plus flag");

    struct d_arg_command* const dg =
        d_arg_command_add_option_group(_app,
                                       "Disabled",
                                       "A disabled group");

    (void)d_arg_command_add_flag(dg,
                                 "--dflag",
                                 NULL);
    (void)d_arg_command_add_subcommand(dg,
                                       "dsub",
                                       NULL);
    d_arg_command_set_disabled(dg,
                               true);

    struct d_arg_command* const ag =
        d_arg_command_add_option_group(a,
                                       "Inner",
                                       "A subcommand's group");

    (void)d_arg_command_add_flag(ag,
                                 "--inner",
                                 "Inner flag");
    (void)d_arg_command_add_option(ag,
                                   "inner-pos",
                                   "Inner positional");
    (void)d_arg_command_add_flag(_app,
                                 "--late",
                                 "A root flag after the groups");
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "groups",
                                   "",
                                   "prog");

    return;
}

/**
 * @brief commands_04: names listed once
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_commands_04(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_set_help_all_flag(_app,
                                          "--help-all",
                                          "Expand all help");

    struct d_arg_command* const g = d_arg_command_add_option_group(_app,
                                                                   "G",
                                                                   "A group");

    (void)d_arg_command_add_flag(g,
                                 "-y,--yes,--only-here",
                                 "The group's flag");
    (void)d_arg_command_add_option(_app,
                                   "--yes,-Y",
                                   "The command's, found first");
    (void)d_arg_command_add_flag(_app,
                                 "-y",
                                 "The command's too");

    struct d_arg_command* const g2 =
        d_arg_command_add_option_group(_app,
                                       "G2",
                                       "A second group");

    (void)d_arg_command_add_flag(g2,
                                 "--elsewhere",
                                 "Its own");

    struct d_arg_command* const s  =
        d_arg_command_add_subcommand(_app,
                                     "sub",
                                     "A subcommand");
    struct d_arg_command* const sg =
        d_arg_command_add_option_group(s,
                                       "SG",
                                       "The subcommand's group");

    (void)d_arg_command_add_flag(sg,
                                 "-z",
                                 "In the subcommand's group");
    (void)d_arg_command_add_flag(s,
                                 "-z,--zed",
                                 "The subcommand's, found first");
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "names listed once",
                                   "",
                                   "prog");

    return;
}

/**
 * @brief commands_05: order
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_commands_05(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_command* const a = d_arg_command_add_subcommand(_app,
                                                                 "a",
                                                                 NULL);
    struct d_arg_command* const b = d_arg_command_add_subcommand(_app,
                                                                 "b",
                                                                 NULL);
    struct d_arg_command* const c = d_arg_command_add_subcommand(_app,
                                                                 "c",
                                                                 NULL);

    (void)d_arg_command_add_subcommand(a,
                                       "a1",
                                       NULL);

    struct d_arg_command* const a2 = d_arg_command_add_subcommand(a,
                                                                  "a2",
                                                                  NULL);

    (void)d_arg_command_add_subcommand(b,
                                       "b1",
                                       NULL);
    (void)d_arg_command_add_subcommand(c,
                                       "c1",
                                       NULL);
    (void)d_arg_command_add_subcommand(a2,
                                       "a21",
                                       NULL);
    (void)d_arg_command_add_flag(c,
                                 "-c",
                                 NULL);
    (void)d_arg_command_add_flag(a,
                                 "-a",
                                 NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "order",
                                   "",
                                   "prog");

    return;
}

// TREE_COMMANDS_STEPS
//   constant: the cases of d_tests_arg_completion_tree_commands.
static const d_tests_arg_define_fn TREE_COMMANDS_STEPS[] =
{
    d_tests_arg_completion_commands_01,
    d_tests_arg_completion_commands_02,
    d_tests_arg_completion_commands_03,
    d_tests_arg_completion_commands_04,
    d_tests_arg_completion_commands_05
};

// TREE_COMMANDS_COUNT
//   constant: how many cases TREE_COMMANDS_STEPS holds.
static const size_t TREE_COMMANDS_COUNT =
    sizeof(TREE_COMMANDS_STEPS) / sizeof(TREE_COMMANDS_STEPS[0]);

/*
d_tests_arg_completion_tree_commands
  Subcommands, their names and aliases, and what option groups give the command
they are in.
  Tests the following:
  - subcommands and aliases with the first line of their description
  - a disabled or hidden subcommand left out with all it holds
  - an option group's options and subcommands as its command's, nested groups
    too; a hidden or disabled group left out
  - commands in breadth-first order
*/
bool
d_tests_arg_completion_tree_commands(void)
{
    // expected
    //   the transcript, in the parts a C99 string literal can hold.
    const char* const expected[] =
    {
        TREE_COMMANDS,
        TREE_COMMANDS_2,
        TREE_COMMANDS_3,
        TREE_COMMANDS_4,
        TREE_COMMANDS_5,
        TREE_COMMANDS_6,
        TREE_COMMANDS_7,
        TREE_COMMANDS_8,
        TREE_COMMANDS_9,
        TREE_COMMANDS_10,
        TREE_COMMANDS_11,
        NULL
    };

    return d_tests_arg_completion_steps("TREE_COMMANDS",
                                        TREE_COMMANDS_STEPS,
                                        TREE_COMMANDS_COUNT,
                                        expected);
}

//==============================================================================
// 12. SWITCHES
//==============================================================================

/**
 * @brief switches_01: each
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_switches_01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_command* const s1 =
        d_arg_command_add_subcommand(_app,
                                     "fall",
                                     "Fallthrough");

    d_arg_command_set_fallthrough(s1,
                                  true);

    struct d_arg_command* const s2 =
        d_arg_command_add_subcommand(_app,
                                     "nosub",
                                     "No subcommand fallthrough");

    d_arg_command_set_subcommand_fallthrough(s2,
                                             false);

    struct d_arg_command* const s3 =
        d_arg_command_add_subcommand(_app,
                                     "win",
                                     "Windows style");

    d_arg_command_set_windows_style(s3,
                                    true);

    struct d_arg_command* const s4 =
        d_arg_command_add_subcommand(_app,
                                     "capture",
                                     "Extras captured");

    d_arg_command_set_allow_extras(s4,
                                   (int)D_ARG_EXTRAS_CAPTURE);

    struct d_arg_command* const s5 =
        d_arg_command_add_subcommand(_app,
                                     "ignore",
                                     "Extras ignored");

    d_arg_command_set_allow_extras(s5,
                                   (int)D_ARG_EXTRAS_IGNORE);

    struct d_arg_command* const s6 =
        d_arg_command_add_subcommand(_app,
                                     "single",
                                     "Extras as one");

    d_arg_command_set_allow_extras(s6,
                                   (int)D_ARG_EXTRAS_ASSUME_SINGLE);

    struct d_arg_command* const s7 =
        d_arg_command_add_subcommand(_app,
                                     "multiple",
                                     "Extras as many");

    d_arg_command_set_allow_extras(s7,
                                   (int)D_ARG_EXTRAS_ASSUME_MULTIPLE);

    struct d_arg_command* const s8 =
        d_arg_command_add_subcommand(_app,
                                     "immediately",
                                     "Extras an error at once");

    d_arg_command_set_allow_extras(s8,
                                   (int)D_ARG_EXTRAS_ERROR_IMMEDIATELY);

    struct d_arg_command* const s9 =
        d_arg_command_add_subcommand(_app,
                                     "prefix",
                                     "A prefix command");

    d_arg_command_set_prefix_command(s9,
                                     (int)D_ARG_PREFIX_ON);

    struct d_arg_command* const s10 =
        d_arg_command_add_subcommand(_app,
                                     "separator",
                                     "A prefix at the separator");

    d_arg_command_set_prefix_command(s10,
                                     (int)D_ARG_PREFIX_SEPARATOR_ONLY);

    struct d_arg_command* const s11 =
        d_arg_command_add_subcommand(_app,
                                     "positional",
                                     "A prefix at a positional");

    d_arg_command_set_prefix_command(s11,
                                     (int)D_ARG_PREFIX_POSITIONAL_ONLY);

    struct d_arg_command* const s12 =
        d_arg_command_add_subcommand(_app,
                                     "two",
                                     "At most two subcommands");

    d_arg_command_require_subcommand(s12,
                                     0u,
                                     2u);

    struct d_arg_command* const s13 =
        d_arg_command_add_subcommand(_app,
                                     "none",
                                     "Nothing at all");

    d_arg_command_set_subcommand_fallthrough(s13,
                                             false);

    struct d_arg_command* const s14 =
        d_arg_command_add_subcommand(_app,
                                     "every",
                                     "Every switch");

    d_arg_command_set_fallthrough(s14,
                                  true);
    d_arg_command_set_windows_style(s14,
                                    true);
    d_arg_command_set_allow_extras(s14,
                                   (int)D_ARG_EXTRAS_CAPTURE);
    d_arg_command_require_subcommand(s14,
                                     0u,
                                     7u);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "each",
                                   "",
                                   "prog");

    return;
}

/**
 * @brief switches_02: inherited
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_switches_02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    d_arg_command_set_fallthrough(_app,
                                  true);
    d_arg_command_set_windows_style(_app,
                                    true);
    d_arg_command_set_allow_extras(_app,
                                   (int)D_ARG_EXTRAS_CAPTURE);
    d_arg_command_require_subcommand(_app,
                                     0u,
                                     3u);
    d_arg_command_set_subcommand_fallthrough(_app,
                                             false);
    (void)d_arg_command_add_subcommand(_app,
                                       "x",
                                       "Inherits");

    struct d_arg_command* const g = d_arg_command_add_option_group(_app,
                                                                   "G",
                                                                   "A group");

    (void)d_arg_command_add_subcommand(g,
                                       "y",
                                       "Under the group");
    d_arg_command_set_allow_extras(_app,
                                   (int)D_ARG_EXTRAS_ERROR);
    d_arg_command_set_prefix_command(_app,
                                     (int)D_ARG_PREFIX_ON);
    (void)d_arg_command_add_subcommand(_app,
                                       "z",
                                       "Inherits the prefix");
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "inherited",
                                   "",
                                   "prog");

    return;
}

// TREE_SWITCHES_STEPS
//   constant: the cases of d_tests_arg_completion_tree_switches.
static const d_tests_arg_define_fn TREE_SWITCHES_STEPS[] =
{
    d_tests_arg_completion_switches_01,
    d_tests_arg_completion_switches_02
};

// TREE_SWITCHES_COUNT
//   constant: how many cases TREE_SWITCHES_STEPS holds.
static const size_t TREE_SWITCHES_COUNT =
    sizeof(TREE_SWITCHES_STEPS) / sizeof(TREE_SWITCHES_STEPS[0]);

/*
d_tests_arg_completion_tree_switches
  Each command's switches and the most subcommands it takes, set and inherited.
  Tests the following:
  - subcommand fallthrough, fallthrough, Windows-style options and extra
    arguments kept, as `s`, `f`, `w` and `e`, or `-`
  - every extras mode and prefix command mode
  - what a subcommand inherits, and an option group passes on
*/
bool
d_tests_arg_completion_tree_switches(void)
{
    // expected
    //   the transcript, in the parts a C99 string literal can hold.
    const char* const expected[] =
    {
        TREE_SWITCHES,
        TREE_SWITCHES_2,
        TREE_SWITCHES_3,
        TREE_SWITCHES_4,
        TREE_SWITCHES_5,
        TREE_SWITCHES_6,
        NULL
    };

    return d_tests_arg_completion_steps("TREE_SWITCHES",
                                        TREE_SWITCHES_STEPS,
                                        TREE_SWITCHES_COUNT,
                                        expected);
}

//==============================================================================
// 13. POSITIONALS
//==============================================================================

/**
 * @brief positionals_01: first of all
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_positionals_01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);

    struct d_arg_option* const   p  =
        d_arg_command_add_option(_app,
                                 "first",
                                 "The first option of all");
    const struct d_arg_validator v1 =
        d_tests_arg_completion_listing(CHOICES_15);

    (void)d_arg_option_check(p,
                             v1,
                             NULL);
    (void)d_arg_command_add_flag(_app,
                                 "-v",
                                 NULL);
    (void)d_arg_command_add_option(_app,
                                   "second",
                                   "The positional after it");
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "first of all",
                                   "",
                                   "prog");

    return;
}

/**
 * @brief positionals_02: slots
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_positionals_02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_add_option(_app,
                                   "first",
                                   "One");

    struct d_arg_option* const p2 = d_arg_command_add_option(_app,
                                                             "second",
                                                             "Two");

    d_arg_option_set_expected(p2,
                              2);
    (void)d_arg_command_add_flag(_app,
                                 "-v",
                                 NULL);

    struct d_arg_option* const p3 = d_arg_command_add_option(_app,
                                                             "rest",
                                                             "Any number");

    d_arg_option_set_expected(p3,
                              -1);

    struct d_arg_command* const s = d_arg_command_add_subcommand(_app,
                                                                 "run",
                                                                 "Run");

    (void)d_arg_command_add_option(s,
                                   "target",
                                   "Under run");

    struct d_arg_option* const p5 = d_arg_command_add_option(s,
                                                             "more",
                                                             "Under run too");

    d_arg_option_set_type_size(p5,
                               2);

    struct d_arg_command* const g = d_arg_command_add_option_group(s,
                                                                   "G",
                                                                   NULL);

    (void)d_arg_command_add_option(g,
                                   "grouped",
                                   "In run's group");
    (void)d_arg_command_add_subcommand(_app,
                                       "none",
                                       "No positionals");
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "slots",
                                   "",
                                   "prog");

    return;
}

/**
 * @brief positionals_03: hidden
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_positionals_03(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_add_option(_app,
                                   "one",
                                   NULL);

    struct d_arg_option* const p2 = d_arg_command_add_option(_app,
                                                             "two",
                                                             NULL);

    (void)d_arg_option_set_group(p2,
                                 "");

    struct d_arg_option* const   p3 = d_arg_command_add_option(_app,
                                                               "three",
                                                               NULL);
    const struct d_arg_validator v1 =
        d_tests_arg_completion_listing(CHOICES_29);

    (void)d_arg_option_check(p3,
                             v1,
                             NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "hidden",
                                   "",
                                   "prog");

    return;
}

/**
 * @brief positionals_04: extra
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_positionals_04(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_option* const p1 = d_arg_command_add_option(_app,
                                                             "files",
                                                             "Files");

    d_arg_option_set_allow_extra_args(p1,
                                      true);

    struct d_arg_option* const p2 = d_arg_command_add_option(_app,
                                                             "big",
                                                             "Past reason");

    d_arg_option_set_expected_range(p2,
                                    1,
                                    (int)D_ARG_EXPECTED_UNLIMITED - 1);

    struct d_arg_option* const p3 =
        d_arg_command_add_option(_app,
                                 "vec",
                                 "Up to any number");

    d_arg_option_set_expected_range(p3,
                                    1,
                                    (int)D_ARG_EXPECTED_UNLIMITED);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "extra",
                                   "",
                                   "prog");

    return;
}

// TREE_POSITIONALS_STEPS
//   constant: the cases of d_tests_arg_completion_tree_positionals.
static const d_tests_arg_define_fn TREE_POSITIONALS_STEPS[] =
{
    d_tests_arg_completion_positionals_01,
    d_tests_arg_completion_positionals_02,
    d_tests_arg_completion_positionals_03,
    d_tests_arg_completion_positionals_04
};

// TREE_POSITIONALS_COUNT
//   constant: how many cases TREE_POSITIONALS_STEPS holds.
static const size_t TREE_POSITIONALS_COUNT =
    sizeof(TREE_POSITIONALS_STEPS) / sizeof(TREE_POSITIONALS_STEPS[0]);

/*
d_tests_arg_completion_tree_positionals
  Positionals, in the order they take words, under each command.
  Tests the following:
  - each command's positionals counted from 0, a group's among its command's
  - the most values each takes, unlimited for extra arguments
  - a hidden positional counted
*/
bool
d_tests_arg_completion_tree_positionals(void)
{
    // expected
    //   the transcript, in the parts a C99 string literal can hold.
    const char* const expected[] =
    {
        TREE_POSITIONALS,
        TREE_POSITIONALS_2,
        TREE_POSITIONALS_3,
        TREE_POSITIONALS_4,
        TREE_POSITIONALS_5,
        NULL
    };

    return d_tests_arg_completion_steps("TREE_POSITIONALS",
                                        TREE_POSITIONALS_STEPS,
                                        TREE_POSITIONALS_COUNT,
                                        expected);
}

//==============================================================================
// 14. QUOTING
//==============================================================================

/**
 * @brief quoting_01: choices
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_quoting_01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_option* const   q  = d_arg_command_add_option(_app,
                                                               "--pick",
                                                               "Text to quote");
    const struct d_arg_validator v1 =
        d_tests_arg_completion_listing(CHOICES_30);

    (void)d_arg_option_check(q,
                             v1,
                             NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "choices",
                                   "",
                                   "prog");

    return;
}

/**
 * @brief quoting_02: bytes
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_quoting_02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_option* const   q  =
        d_arg_command_add_option(_app,
                                 "--pick",
                                 "Bytes past ASCII");
    const struct d_arg_validator v1 =
        d_tests_arg_completion_listing(CHOICES_31);

    (void)d_arg_option_check(q,
                             v1,
                             NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "bytes",
                                   "",
                                   "prog");

    return;
}

/**
 * @brief quoting_03: descriptions
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_quoting_03(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_command_add_flag(_app,
                                 "--a",
                                 "it's");
    (void)d_arg_command_add_flag(_app,
                                 "--b",
                                 "tab\there");
    (void)d_arg_command_add_flag(_app,
                                 "--c",
                                 "bell\x07, del\x7f, escape\x1b");
    (void)d_arg_command_add_flag(_app,
                                 "--d",
                                 "caf\xc3\xa9 \xe2\x86\x92 \xf0\x9f\x98\x80");
    (void)d_arg_command_add_flag(_app,
                                 "--e",
                                 "back\\slash \"quoted\" $x `y`");
    (void)d_arg_command_add_flag(_app,
                                 "--f",
                                 "stray \xff byte");
    (void)d_arg_command_add_subcommand(_app,
                                       "it's",
                                       "Quote's\there");

    struct d_arg_command* const s =
        d_arg_command_add_subcommand(_app,
                                     "caf\xc3\xa9",
                                     "Caf\xc3\xa9 \xe2\x86\x92");

    (void)d_arg_command_add_alias(s,
                                  "k\xc3\xb8");

    struct d_arg_command* const t =
        d_arg_command_add_subcommand(_app,
                                     "a\tb",
                                     "A tab in a name");

    (void)d_arg_command_add_alias(t,
                                  "x\ty");
    (void)d_arg_command_add_subcommand(_app,
                                       "del\x7f",
                                       "A delete in a name");
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "descriptions",
                                   "",
                                   "prog");

    return;
}

// TREE_QUOTING_STEPS
//   constant: the cases of d_tests_arg_completion_tree_quoting.
static const d_tests_arg_define_fn TREE_QUOTING_STEPS[] =
{
    d_tests_arg_completion_quoting_01,
    d_tests_arg_completion_quoting_02,
    d_tests_arg_completion_quoting_03
};

// TREE_QUOTING_COUNT
//   constant: how many cases TREE_QUOTING_STEPS holds.
static const size_t TREE_QUOTING_COUNT =
    sizeof(TREE_QUOTING_STEPS) / sizeof(TREE_QUOTING_STEPS[0]);

/*
d_tests_arg_completion_tree_quoting
  Choices and descriptions holding what each shell must quote, and bytes past
ASCII.
  Tests the following:
  - quotes, backslashes, controls and text past ASCII in every shell
  - PowerShell's [char] values for every character outside printable ASCII, a
    surrogate pair past the basic plane
  - a byte that starts no valid sequence as U+FFFD
*/
bool
d_tests_arg_completion_tree_quoting(void)
{
    // expected
    //   the transcript, in the parts a C99 string literal can hold.
    const char* const expected[] =
    {
        TREE_QUOTING,
        TREE_QUOTING_2,
        TREE_QUOTING_3,
        TREE_QUOTING_4,
        TREE_QUOTING_5,
        TREE_QUOTING_6,
        NULL
    };

    return d_tests_arg_completion_steps("TREE_QUOTING",
                                        TREE_QUOTING_STEPS,
                                        TREE_QUOTING_COUNT,
                                        expected);
}

//==============================================================================
// 15. SETS AND MAPPINGS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

/**
 * @brief member_01: sets
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_member_01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_option* const   m1 = d_arg_command_add_option(_app,
                                                               "--member",
                                                               "A set");
    const struct d_arg_validator v1 = d_arg_validator_is_member(NULL,
                                                                MEMBERS_1,
                                                                3u,
                                                                NULL);

    (void)d_arg_option_check(m1,
                             v1,
                             NULL);

    struct d_arg_option* const   m2 = d_arg_command_add_option(_app,
                                                               "--mapped",
                                                               "A mapping");
    const struct d_arg_validator v2 = d_arg_validator_transformer(NULL,
                                                                  MAPPING_1,
                                                                  2u,
                                                                  NULL);

    (void)d_arg_option_transform(m2,
                                 v2,
                                 NULL,
                                 NULL);

    struct d_arg_option* const   m3 =
        d_arg_command_add_option(_app,
                                 "--checked",
                                 "A checked mapping");
    const struct d_arg_validator v3 =
        d_arg_validator_checked_transformer(NULL,
                                            MAPPING_2,
                                            2u,
                                            NULL);

    (void)d_arg_option_transform(m3,
                                 v3,
                                 NULL,
                                 NULL);

    struct d_arg_option* const   m4 =
        d_arg_command_add_option(_app,
                                 "--both",
                                 "A set and a list");
    const struct d_arg_validator v4 = d_arg_validator_is_member(NULL,
                                                                MEMBERS_2,
                                                                2u,
                                                                NULL);

    (void)d_arg_option_check(m4,
                             v4,
                             NULL);

    const struct d_arg_validator v5 =
        d_tests_arg_completion_listing(CHOICES_32);

    (void)d_arg_option_check(m4,
                             v5,
                             NULL);

    struct d_arg_option* const   m5 =
        d_tests_arg_completion_bound(_run,
                                     _app,
                                     "--number",
                                     "A set of numbers",
                                     (int)D_ARG_BUILTIN_INT);
    const struct d_arg_validator v6 = d_arg_validator_is_member(NULL,
                                                                MEMBERS_3,
                                                                2u,
                                                                NULL);

    (void)d_arg_option_check(m5,
                             v6,
                             NULL);

    struct d_arg_option* const   m6 =
        d_arg_command_add_option(_app,
                                 "--quoted",
                                 "A set to quote");
    const struct d_arg_validator v7 = d_arg_validator_is_member(NULL,
                                                                MEMBERS_4,
                                                                2u,
                                                                NULL);

    (void)d_arg_option_check(m6,
                             v7,
                             NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "sets",
                                   "",
                                   "prog");

    return;
}

// TREE_MEMBER_STEPS
//   constant: the cases of d_tests_arg_completion_tree_member.
static const d_tests_arg_define_fn TREE_MEMBER_STEPS[] =
{
    d_tests_arg_completion_member_01
};

// TREE_MEMBER_COUNT
//   constant: how many cases TREE_MEMBER_STEPS holds.
static const size_t TREE_MEMBER_COUNT =
    sizeof(TREE_MEMBER_STEPS) / sizeof(TREE_MEMBER_STEPS[0]);

/*
d_tests_arg_completion_tree_member
  CLI11's IsMember, Transformer and CheckedTransformer as completion lists them.
  Tests the following:
  - a set's members, a transformer's keys
  - a checked transformer's keys, then the values not listed already
  - beside a listing of the suite's own
*/
bool
d_tests_arg_completion_tree_member(void)
{
    // expected
    //   the transcript, in the parts a C99 string literal can hold.
    const char* const expected[] =
    {
        TREE_MEMBER,
        TREE_MEMBER_2,
        NULL
    };

    return d_tests_arg_completion_steps("TREE_MEMBER",
                                        TREE_MEMBER_STEPS,
                                        TREE_MEMBER_COUNT,
                                        expected);
}

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

//==============================================================================
// 16. PATHS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)

/**
 * @brief path_01: paths
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_path_01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_option* const   p1 =
        d_arg_command_add_option(_app,
                                 "--file",
                                 "An existing file");
    const struct d_arg_validator v1 = d_arg_validator_existing_file();

    (void)d_arg_option_check(p1,
                             v1,
                             NULL);

    struct d_arg_option* const   p2 =
        d_arg_command_add_option(_app,
                                 "--dir",
                                 "An existing directory");
    const struct d_arg_validator v2 = d_arg_validator_existing_directory();

    (void)d_arg_option_check(p2,
                             v2,
                             NULL);

    struct d_arg_option* const   p3 =
        d_arg_command_add_option(_app,
                                 "--path",
                                 "An existing path");
    const struct d_arg_validator v3 = d_arg_validator_existing_path();

    (void)d_arg_option_check(p3,
                             v3,
                             NULL);

    struct d_arg_option* const   p4 =
        d_arg_command_add_option(_app,
                                 "--new",
                                 "A path not there yet");
    const struct d_arg_validator v4 = d_arg_validator_nonexistent_path();

    (void)d_arg_option_check(p4,
                             v4,
                             NULL);

    struct d_arg_option* const   p5 = d_arg_command_add_option(_app,
                                                               "--dir-or-file",
                                                               "Either");
    const struct d_arg_validator v5 = d_arg_validator_existing_directory();
    const struct d_arg_validator v6 = d_arg_validator_existing_file();
    const struct d_arg_validator v7 = d_arg_validator_or(v5,
                                                         v6);

    (void)d_arg_option_check(p5,
                             v7,
                             NULL);

    struct d_arg_option* const   p6  =
        d_arg_command_add_option(_app,
                                 "--dir-and-number",
                                 "Both");
    const struct d_arg_validator v8  = d_arg_validator_existing_directory();
    const struct d_arg_validator v9  = d_arg_validator_range_long(0L,
                                                                  9L);
    const struct d_arg_validator v10 = d_arg_validator_and(v8,
                                                           v9);

    (void)d_arg_option_check(p6,
                             v10,
                             NULL);

    struct d_arg_option* const   p7  =
        d_arg_command_add_option(_app,
                                 "where",
                                 "A directory positional");
    const struct d_arg_validator v11 = d_arg_validator_existing_directory();

    (void)d_arg_option_check(p7,
                             v11,
                             NULL);
    d_tests_arg_completion_scripts(_run,
                                   _app,
                                   "paths",
                                   "",
                                   "prog");

    return;
}

// TREE_PATH_STEPS
//   constant: the cases of d_tests_arg_completion_tree_path.
static const d_tests_arg_define_fn TREE_PATH_STEPS[] =
{
    d_tests_arg_completion_path_01
};

// TREE_PATH_COUNT
//   constant: how many cases TREE_PATH_STEPS holds.
static const size_t TREE_PATH_COUNT =
    sizeof(TREE_PATH_STEPS) / sizeof(TREE_PATH_STEPS[0]);

/*
d_tests_arg_completion_tree_path
  CLI11's path validators as completion offers their values.
  Tests the following:
  - files for an existing file, path or a path not there yet
  - directories for an existing directory
  - composites over them
*/
bool
d_tests_arg_completion_tree_path(void)
{
    // expected
    //   the transcript, in the parts a C99 string literal can hold.
    const char* const expected[] =
    {
        TREE_PATH,
        TREE_PATH_2,
        NULL
    };

    return d_tests_arg_completion_steps("TREE_PATH",
                                        TREE_PATH_STEPS,
                                        TREE_PATH_COUNT,
                                        expected);
}

#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS

//==============================================================================
// 17. THE COMPLETION FLAG
//==============================================================================

/**
 * @brief flag_01: bash
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "--completion",
                                           NULL);
    d_tests_arg_completion_parse(_run,
                                 "bash",
                                 NULL,
                                 "--completion bash",
                                 "prog");

    return;
}

/**
 * @brief flag_02: zsh
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "--completion",
                                           NULL);
    d_tests_arg_completion_parse(_run,
                                 "zsh",
                                 NULL,
                                 "--completion zsh",
                                 "prog");

    return;
}

/**
 * @brief flag_03: fish
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_03(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "--completion",
                                           NULL);
    d_tests_arg_completion_parse(_run,
                                 "fish",
                                 NULL,
                                 "--completion fish",
                                 "prog");

    return;
}

/**
 * @brief flag_04: powershell
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_04(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "--completion",
                                           NULL);
    d_tests_arg_completion_parse(_run,
                                 "powershell",
                                 NULL,
                                 "--completion powershell",
                                 "prog");

    return;
}

/**
 * @brief flag_05: pwsh
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_05(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "--completion",
                                           NULL);
    d_tests_arg_completion_parse(_run,
                                 "pwsh",
                                 NULL,
                                 "--completion pwsh",
                                 "prog");

    return;
}

/**
 * @brief flag_06: any case
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_06(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "--completion",
                                           NULL);
    d_tests_arg_completion_parse(_run,
                                 "any case",
                                 NULL,
                                 "--completion BaSh",
                                 "prog");
    d_tests_arg_completion_parse(_run,
                                 "any case",
                                 NULL,
                                 "--completion PWSH",
                                 "prog");

    return;
}

/**
 * @brief flag_07: after =
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_07(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "--completion",
                                           NULL);
    d_tests_arg_completion_parse(_run,
                                 "after =",
                                 NULL,
                                 "--completion=zsh",
                                 "prog");

    return;
}

/**
 * @brief flag_08: the program from argv[0]
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_08(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "--completion",
                                           NULL);
    d_tests_arg_completion_parse(_run,
                                 "the program from argv[0]",
                                 "/opt/bin/Tool.EXE",
                                 "--completion fish",
                                 "Tool");
    d_tests_arg_completion_parse(_run,
                                 "the program from argv[0]",
                                 "tool",
                                 "--completion bash",
                                 "tool");
    d_tests_arg_completion_parse(_run,
                                 "the program from argv[0]",
                                 "C:\\bin\\x.exe",
                                 "--completion powershell",
                                 "x");

    return;
}

/**
 * @brief flag_09: the program after a parse without one
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_09(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "--completion",
                                           NULL);
    d_tests_arg_completion_parse(_run,
                                 "the program after a parse without one",
                                 "first",
                                 "--completion bash",
                                 "first");
    d_tests_arg_completion_parse(_run,
                                 "the program after a parse without one",
                                 NULL,
                                 "--completion bash",
                                 "prog");

    return;
}

/**
 * @brief flag_10: a shell that is none
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_10(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "--completion",
                                           NULL);
    d_tests_arg_completion_parse(_run,
                                 "a shell that is none",
                                 NULL,
                                 "--completion tcsh",
                                 "");
    d_tests_arg_completion_parse(_run,
                                 "a shell that is none",
                                 NULL,
                                 "--completion ",
                                 "");

    return;
}

/**
 * @brief flag_11: no shell
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_11(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "--completion",
                                           NULL);
    d_tests_arg_completion_parse(_run,
                                 "no shell",
                                 NULL,
                                 "--completion",
                                 "");
    d_tests_arg_completion_parse(_run,
                                 "no shell",
                                 NULL,
                                 "--completion=",
                                 "");

    return;
}

/**
 * @brief flag_12: two shells
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_12(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "--completion",
                                           NULL);
    d_tests_arg_completion_parse(_run,
                                 "two shells",
                                 NULL,
                                 "--completion bash --completion zsh",
                                 "");

    return;
}

/**
 * @brief flag_13: help wins
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_13(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "--completion",
                                           NULL);
    d_tests_arg_completion_parse(_run,
                                 "help wins",
                                 NULL,
                                 "--completion bash --help",
                                 "");
    d_tests_arg_completion_parse(_run,
                                 "help wins",
                                 NULL,
                                 "-h --completion zsh",
                                 "");

    return;
}

/**
 * @brief flag_14: version wins
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_14(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "--completion",
                                           NULL);
    (void)d_arg_command_set_version_flag(_app,
                                         "-V",
                                         "1.2",
                                         NULL);
    d_tests_arg_completion_parse(_run,
                                 "version wins",
                                 NULL,
                                 "--completion fish -V",
                                 "");
    d_tests_arg_completion_parse(_run,
                                 "version wins",
                                 NULL,
                                 "-V --completion fish",
                                 "");

    return;
}

/**
 * @brief flag_15: before the requirements
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_15(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "--completion",
                                           NULL);

    struct d_arg_option* const need = d_arg_command_add_option(_app,
                                                               "--need",
                                                               "Required");

    d_arg_option_set_required(need,
                              true);

    struct d_arg_option* const p =
        d_arg_command_add_option(_app,
                                 "pos",
                                 "A required positional");

    d_arg_option_set_required(p,
                              true);

    struct d_arg_option* const a = d_arg_command_add_flag(_app,
                                                          "-a",
                                                          NULL);
    struct d_arg_option* const b = d_arg_command_add_flag(_app,
                                                          "-b",
                                                          NULL);

    (void)d_arg_option_needs(a,
                             b);

    struct d_arg_option* const x = d_arg_command_add_flag(_app,
                                                          "-x",
                                                          NULL);
    struct d_arg_option* const y = d_arg_command_add_flag(_app,
                                                          "-y",
                                                          NULL);

    (void)d_arg_option_excludes(x,
                                y);
    d_arg_command_require_subcommand(_app,
                                     1u,
                                     0u);
    (void)d_arg_command_add_subcommand(_app,
                                       "sub",
                                       NULL);
    d_tests_arg_completion_parse(_run,
                                 "before the requirements",
                                 NULL,
                                 "-a -x -y --completion bash",
                                 "prog");
    d_tests_arg_completion_parse(_run,
                                 "before the requirements",
                                 NULL,
                                 "-a -x -y",
                                 "");

    return;
}

/**
 * @brief flag_16: before other callbacks
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_16(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "--completion",
                                           NULL);
    (void)d_arg_command_add_option_callback(_app,
                                            "--noted",
                                            d_tests_arg_completion_noted,
                                            _run,
                                            NULL,
                                            NULL);
    d_tests_arg_completion_parse(_run,
                                 "before other callbacks",
                                 NULL,
                                 "--noted v --completion zsh",
                                 "prog");
    d_tests_arg_completion_parse(_run,
                                 "before other callbacks",
                                 NULL,
                                 "--noted v",
                                 "");

    return;
}

/**
 * @brief flag_17: from a subcommand falling through
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_17(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "--completion",
                                           NULL);

    struct d_arg_command* const s =
        d_arg_command_add_subcommand(_app,
                                     "sub",
                                     "A subcommand");

    d_arg_command_set_fallthrough(s,
                                  true);
    d_tests_arg_completion_parse(_run,
                                 "from a subcommand falling through",
                                 NULL,
                                 "sub --completion bash",
                                 "prog");

    return;
}

/**
 * @brief flag_18: from a subcommand not falling through
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_18(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "--completion",
                                           NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "sub",
                                       "A subcommand");
    d_tests_arg_completion_parse(_run,
                                 "from a subcommand not falling through",
                                 NULL,
                                 "sub --completion bash",
                                 "");

    return;
}

/**
 * @brief flag_19: renamed
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_19(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "-c,--complete",
                                           "Write a script");
    d_tests_arg_completion_parse(_run,
                                 "renamed",
                                 NULL,
                                 "-c fish",
                                 "prog");
    d_tests_arg_completion_parse(_run,
                                 "renamed",
                                 NULL,
                                 "--complete=powershell",
                                 "prog");

    return;
}

/**
 * @brief flag_20: replaced
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_20(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "--completion",
                                           NULL);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "--script",
                                           NULL);
    d_tests_arg_completion_parse(_run,
                                 "replaced",
                                 NULL,
                                 "--script zsh",
                                 "prog");
    d_tests_arg_completion_parse(_run,
                                 "replaced",
                                 NULL,
                                 "--completion zsh",
                                 "");

    return;
}

/**
 * @brief flag_21: removed
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_21(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "--completion",
                                           NULL);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           NULL,
                                           NULL);
    d_tests_arg_completion_parse(_run,
                                 "removed",
                                 NULL,
                                 "--completion bash",
                                 "");

    return;
}

/**
 * @brief flag_22: removed by an empty name
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_22(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "--completion",
                                           NULL);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "",
                                           NULL);
    d_tests_arg_completion_parse(_run,
                                 "removed by an empty name",
                                 NULL,
                                 "--completion bash",
                                 "");

    return;
}

/**
 * @brief flag_23: moved into a group, then replaced
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_23(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_option* const  c =
        d_arg_parser_set_completion_flag(&_run->parser,
                                         "--completion",
                                         NULL);
    struct d_arg_command* const g = d_arg_command_add_option_group(_app,
                                                                   "G",
                                                                   NULL);

    (void)d_arg_command_add_flag(g,
                                 "--kept",
                                 "Kept");
    (void)d_arg_command_group_add_option(g,
                                         c);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "--again",
                                           "Again");
    d_tests_arg_completion_parse(_run,
                                 "moved into a group, then replaced",
                                 NULL,
                                 "--again bash",
                                 "prog");

    return;
}

/**
 * @brief flag_24: moved into a group
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_24(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_option* const  c =
        d_arg_parser_set_completion_flag(&_run->parser,
                                         "--completion",
                                         NULL);
    struct d_arg_command* const g = d_arg_command_add_option_group(_app,
                                                                   "G",
                                                                   NULL);

    (void)d_arg_command_group_add_option(g,
                                         c);
    d_tests_arg_completion_parse(_run,
                                 "moved into a group",
                                 NULL,
                                 "--completion fish",
                                 "prog");

    return;
}

/**
 * @brief flag_25: names refused
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_25(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "-bad name",
                                           NULL);
    d_tests_arg_completion_parse(_run,
                                 "names refused",
                                 NULL,
                                 "",
                                 "");

    return;
}

/**
 * @brief flag_26: among other options
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_26(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);

    struct d_arg_option* const   c  = d_arg_command_add_option(_app,
                                                               "--color",
                                                               "A color");
    const struct d_arg_validator v1 =
        d_tests_arg_completion_listing(CHOICES_33);

    (void)d_arg_option_check(c,
                             v1,
                             NULL);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "--completion",
                                           NULL);
    (void)d_arg_command_add_subcommand(_app,
                                       "run",
                                       "Run it");
    d_tests_arg_completion_parse(_run,
                                 "among other options",
                                 NULL,
                                 "--color red --completion zsh",
                                 "prog");

    return;
}

// FLAG_STEPS
//   constant: the cases of d_tests_arg_completion_flag.
static const d_tests_arg_define_fn FLAG_STEPS[] =
{
    d_tests_arg_completion_flag_01,
    d_tests_arg_completion_flag_02,
    d_tests_arg_completion_flag_03,
    d_tests_arg_completion_flag_04,
    d_tests_arg_completion_flag_05,
    d_tests_arg_completion_flag_06,
    d_tests_arg_completion_flag_07,
    d_tests_arg_completion_flag_08,
    d_tests_arg_completion_flag_09,
    d_tests_arg_completion_flag_10,
    d_tests_arg_completion_flag_11,
    d_tests_arg_completion_flag_12,
    d_tests_arg_completion_flag_13,
    d_tests_arg_completion_flag_14,
    d_tests_arg_completion_flag_15,
    d_tests_arg_completion_flag_16,
    d_tests_arg_completion_flag_17,
    d_tests_arg_completion_flag_18,
    d_tests_arg_completion_flag_19,
    d_tests_arg_completion_flag_20,
    d_tests_arg_completion_flag_21,
    d_tests_arg_completion_flag_22,
    d_tests_arg_completion_flag_23,
    d_tests_arg_completion_flag_24,
    d_tests_arg_completion_flag_25,
    d_tests_arg_completion_flag_26
};

// FLAG_COUNT
//   constant: how many cases FLAG_STEPS holds.
static const size_t FLAG_COUNT = sizeof(FLAG_STEPS) / sizeof(FLAG_STEPS[0]);

/*
d_tests_arg_completion_flag
  The option d_arg_parser_set_completion_flag makes, in a parse ended by
d_arg_parser_exit.
  Tests the following:
  - each shell by name, in any case, and after `=`
  - the script for `argv[0]`, its last component less ".exe", or the root's name
    after a parse given none
  - a shell that is none, no shell and two: CLI11's messages
  - a help or version flag winning; the requirements and other callbacks never
    reached
  - from a subcommand that falls through, and one that does not
  - the option renamed, replaced, removed, moved into a group, and refused
*/
bool
d_tests_arg_completion_flag(void)
{
    // expected
    //   the transcript, in the parts a C99 string literal can hold.
    const char* const expected[] =
    {
        FLAG,
        FLAG_2,
        FLAG_3,
        FLAG_4,
        FLAG_5,
        FLAG_6,
        FLAG_7,
        FLAG_8,
        NULL
    };

    return d_tests_arg_completion_steps("FLAG",
                                        FLAG_STEPS,
                                        FLAG_COUNT,
                                        expected);
}

//==============================================================================
// 18. THE FLAG AND CONFIGURATION FILES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief flag_config_01: a configuration file not read
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_config_01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "--completion",
                                           NULL);
    (void)d_arg_command_set_config(_app,
                                   "--config",
                                   "missing.ini",
                                   NULL,
                                   true);
    d_tests_arg_completion_parse(_run,
                                 "a configuration file not read",
                                 NULL,
                                 "--completion bash",
                                 "prog");
    d_tests_arg_completion_parse(_run,
                                 "a configuration file not read",
                                 NULL,
                                 "",
                                 "");

    return;
}

/**
 * @brief flag_config_02: not configurable
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_config_02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "--completion",
                                           NULL);
    d_tests_arg_completion_parse_config(_run,
                                        "not configurable",
                                        "completion=bash\n");

    return;
}

// FLAG_CONFIG_STEPS
//   constant: the cases of d_tests_arg_completion_flag_config.
static const d_tests_arg_define_fn FLAG_CONFIG_STEPS[] =
{
    d_tests_arg_completion_flag_config_01,
    d_tests_arg_completion_flag_config_02
};

// FLAG_CONFIG_COUNT
//   constant: how many cases FLAG_CONFIG_STEPS holds.
static const size_t FLAG_CONFIG_COUNT =
    sizeof(FLAG_CONFIG_STEPS) / sizeof(FLAG_CONFIG_STEPS[0]);

/*
d_tests_arg_completion_flag_config
  The completion option beside configuration files.
  Tests the following:
  - a required configuration file never read
  - the option not configurable
*/
bool
d_tests_arg_completion_flag_config(void)
{
    // expected
    //   the transcript, in the parts a C99 string literal can hold.
    const char* const expected[] =
    {
        FLAG_CONFIG,
        NULL
    };

    return d_tests_arg_completion_steps("FLAG_CONFIG",
                                        FLAG_CONFIG_STEPS,
                                        FLAG_CONFIG_COUNT,
                                        expected);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 19. THE FLAG AND THE ENVIRONMENT
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_ENVIRONMENT == 1)

/**
 * @brief flag_environment_01: the environment not read
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_completion_flag_environment_01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    d_arg_command_set_windows_style(_app,
                                    false);
    (void)d_arg_parser_set_completion_flag(&_run->parser,
                                           "--completion",
                                           NULL);

    struct d_arg_option* const l = d_arg_command_add_option(_app,
                                                            "--level",
                                                            "A level");

    (void)d_arg_option_set_env_name(l,
                                    "DJ_TESTS_ARG_COMPLETION_LEVEL");
    (void)d_tests_arg_setenv("DJ_TESTS_ARG_COMPLETION_LEVEL",
                             "5");
    d_tests_arg_completion_parse(_run,
                                 "the environment not read",
                                 NULL,
                                 "--completion bash",
                                 "prog");
    d_tests_arg_completion_given(_run,
                                 "l",
                                 l);
    d_tests_arg_completion_parse(_run,
                                 "the environment not read",
                                 NULL,
                                 "",
                                 "");
    d_tests_arg_completion_given(_run,
                                 "l",
                                 l);
    (void)d_tests_arg_setenv("DJ_TESTS_ARG_COMPLETION_LEVEL",
                             NULL);

    return;
}

// FLAG_ENVIRONMENT_STEPS
//   constant: the cases of d_tests_arg_completion_flag_environment.
static const d_tests_arg_define_fn FLAG_ENVIRONMENT_STEPS[] =
{
    d_tests_arg_completion_flag_environment_01
};

// FLAG_ENVIRONMENT_COUNT
//   constant: how many cases FLAG_ENVIRONMENT_STEPS holds.
static const size_t FLAG_ENVIRONMENT_COUNT =
    sizeof(FLAG_ENVIRONMENT_STEPS) / sizeof(FLAG_ENVIRONMENT_STEPS[0]);

/*
d_tests_arg_completion_flag_environment
  The completion option beside an option read from the environment.
  Tests the following:
  - the environment never read
*/
bool
d_tests_arg_completion_flag_environment(void)
{
    // expected
    //   the transcript, in the parts a C99 string literal can hold.
    const char* const expected[] =
    {
        FLAG_ENVIRONMENT,
        NULL
    };

    return d_tests_arg_completion_steps("FLAG_ENVIRONMENT",
                                        FLAG_ENVIRONMENT_STEPS,
                                        FLAG_ENVIRONMENT_COUNT,
                                        expected);
}

#endif  // D_INTERNAL_ARG_PARSER_ENVIRONMENT

//==============================================================================
// 20. THE SUITE
//==============================================================================

// d_tests_arg_completion
//   constant: the suite, in the header's order.
const struct d_tests_section d_tests_arg_completion[] =
{
    { "arg_completion_shell_from_name",
      d_tests_arg_completion_shell_from_name },
    { "arg_completion_shell_name",
      d_tests_arg_completion_shell_name },
    { "arg_completion_identifiers",
      d_tests_arg_completion_identifiers },
    { "arg_completion_script_lines",
      d_tests_arg_completion_script_lines },
    { "arg_completion_line_text",
      d_tests_arg_completion_line_text },
    { "arg_completion_refusals",
      d_tests_arg_completion_refusals },
    { "arg_completion_program_names",
      d_tests_arg_completion_program_names },
    { "arg_completion_tree_root",
      d_tests_arg_completion_tree_root },
    { "arg_completion_tree_options",
      d_tests_arg_completion_tree_options },
    { "arg_completion_tree_values",
      d_tests_arg_completion_tree_values },
    { "arg_completion_tree_commands",
      d_tests_arg_completion_tree_commands },
    { "arg_completion_tree_switches",
      d_tests_arg_completion_tree_switches },
    { "arg_completion_tree_positionals",
      d_tests_arg_completion_tree_positionals },
    { "arg_completion_tree_quoting",
      d_tests_arg_completion_tree_quoting },
#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
    { "arg_completion_tree_member",
      d_tests_arg_completion_tree_member },
#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS
#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)
    { "arg_completion_tree_path",
      d_tests_arg_completion_tree_path },
#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS
    { "arg_completion_flag",
      d_tests_arg_completion_flag },
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_completion_flag_config",
      d_tests_arg_completion_flag_config },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_ENVIRONMENT == 1)
    { "arg_completion_flag_environment",
      d_tests_arg_completion_flag_environment },
#endif  // D_INTERNAL_ARG_PARSER_ENVIRONMENT
};

// d_tests_arg_completion_count
//   constant: how many sections the table above holds.
const size_t d_tests_arg_completion_count =
    sizeof(d_tests_arg_completion) / sizeof(d_tests_arg_completion[0]);

#endif  // D_INTERNAL_ARG_PARSER_COMPLETION
