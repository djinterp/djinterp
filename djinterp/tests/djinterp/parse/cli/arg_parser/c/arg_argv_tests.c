/*******************************************************************************
* djinterp [parse]                                              arg_argv_tests.c
*
* The suite of arguments from text, declared in arg_argv_tests.h.
*   Each section but the last of each part is a table of rows -- lines,
* words, files, the arguments of a parse -- each run and written as the
* reference writes it, the whole compared at once. A text is split, quoted
* or searched from storage of exactly its length, so that a read past
* either end is AddressSanitizer's to see; files are made in a scratch
* directory. The rows keep the tags the reference gives them: P, W, C and
* G a set of rules' lines, F a switch ignored, O the module's own, Q a
* word, N a program's name, R a file read, E a parse, U wide text, B UTF-8
* made wide.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_argv_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./arg_argv_tests.h"  // corresponding header
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL, wchar_t
#include <stdio.h>    // printf, snprintf
#include <stdlib.h>   // malloc, free; __argc, __argv on Windows
#include <string.h>   // memcmp, memcpy, strcmp, strlen
#include <wchar.h>    // WCHAR_MAX
// djinterp
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // the knobs
#include "../../../../../../inc/djinterp/env/parse/cli/arg_parser/env_arg_parser.h"  // D_ENV_ARG_PARSER_COMMAND_LINE
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_argv.h"  // splitting, quoting, wide text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // extras
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // statuses, buffers, lists
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // the parser, response files
#include "../../../c/tests_section.h"  // d_tests_section
#include "./arg_tests_files.h"  // scratch directories
#include "./arg_tests_support.h"  // runs, reports, escaping
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// SPLIT_POSIX
//   constant: what the cases of d_tests_arg_argv_split_posix come to, as the
// references give them.
static const char SPLIT_POSIX[] =
    "P01 posix 0 []: 0\n"
    "P02 posix 0 [ ]: 0\n"
    "P03 posix 0 [a]: 1 [a]\n"
    "P04 posix 0 [ a  b ]: 2 [a] [b]\n"
    "P05 posix 0 ['a b']: 1 [a b]\n"
    "P06 posix 0 [\"a b\"]: 1 [a b]\n"
    "P07 posix 0 [a'b c'd]: 1 [ab cd]\n"
    "P08 posix 0 [a\"b c\"d]: 1 [ab cd]\n"
    "P09 posix 0 ['']: 1 []\n"
    "P10 posix 0 [\"\"]: 1 []\n"
    "P11 posix 0 ['' '']: 2 [] []\n"
    "P12 posix 0 [a\\\\ b]: 1 [a b]\n"
    "P13 posix 0 [\\\\\\\\]: 1 [\\\\]\n"
    "P14 posix 0 [\\\\\"]: 1 [\"]\n"
    "P15 posix 0 [\\\\']: 1 [']\n"
    "P16 posix 0 [\"\\\\\"\"]: 1 [\"]\n"
    "P17 posix 0 [\"\\\\\\\\\"]: 1 [\\\\]\n"
    "P18 posix 0 [\"\\\\$\"]: 1 [$]\n"
    "P19 posix 0 [\"\\\\`\"]: 1 [`]\n"
    "P20 posix 0 [\"\\\\a\"]: 1 [\\\\a]\n"
    "P21 posix 0 [\"a\\\\\\nb\"]: 1 [ab]\n"
    "P22 posix 0 [a\\\\\\nb]: 1 [ab]\n"
    "P23 posix 0 [\"a\\nb\"]: 1 [a\\nb]\n"
    "P24 posix 0 ['a\\nb']: 1 [a\\nb]\n"
    "P25 posix 0 ['a\\\\b']: 1 [a\\\\b]\n"
    "P26 posix 0 [\"\\\\'\"]: 1 [\\\\']\n"
    "P27 posix 0 [x\\\\\\n]: 1 [x]\n"
    "P28 posix 0 [']: ERR InvalidError\n"
    "P29 posix 0 [\"]: ERR InvalidError\n"
    "P30 posix 0 [a\\\\]: 1 [a\\\\]\n"
    "P31 posix 0 [\"a]: ERR InvalidError\n"
    "P32 posix 0 ['a]: ERR InvalidError\n"
    "P33 posix 0 [a\\tb]: 2 [a] [b]\n"
    "P34 posix 0 [a\\x0bb]: 1 [a\\x0bb]\n"
    "P35 posix 0 [a\\x0cb]: 1 [a\\x0cb]\n"
    "P36 posix 0 [a\\x0db]: 1 [a\\x0db]\n"
    "P37 posix 0 [\\\\$x]: 1 [$x]\n"
    "P38 posix 0 [\\\\`x]: 1 [`x]\n"
    "P39 posix 0 [\"$\"x]: 1 [$x]\n"
    "P40 posix 0 ['$']: 1 [$]\n"
    "P41 posix 0 [a#b]: 1 [a#b]\n"
    "P42 posix 0 [a\\nb]: 2 [a] [b]\n"
    "P43 posix 0 [a\\n\\nb]: 2 [a] [b]\n"
    "P44 posix 0 [\\n]: 0\n"
    "P45 posix 0 [a \\n b]: 2 [a] [b]\n"
    "P46 posix 0 [a\\n\"b c\"\\nd]: 3 [a] [b c] [d]\n"
    "P47 posix 0 ['a\\nb]: ERR InvalidError\n"
    "P48 posix 0 [a\\\\ \\nb]: 2 [a ] [b]\n"
    "P49 posix 0 [\"\\\\\"]: ERR InvalidError\n";

// SPLIT_WINDOWS
//   constant: what the cases of d_tests_arg_argv_split_windows come to, as the
// references give them.
static const char SPLIT_WINDOWS[] =
    "W01 windows 0 [\"a b c\" d e]: 3 [a b c] [d] [e]\n"
    "W02 windows 0 [\"ab\\\\\"c\" \"\\\\\\\\\" d]: 3 [ab\"c] [\\\\] [d]\n"
    "W03 windows 0 [a\\\\\\\\\\\\b d\"e f\"g h]: 3 [a\\\\\\\\\\\\b] [de fg] "
    "[h]\n"
    "W04 windows 0 [a\\\\\\\\\\\\\"b c d]: 3 [a\\\\\"b] [c] [d]\n"
    "W05 windows 0 [a\\\\\\\\\\\\\\\\\"b c\" d e]: 3 [a\\\\\\\\b c] [d] [e]\n"
    "W06 windows 0 [a\"b\"\" c d]: 1 [ab\" c d]\n"
    "W07 windows 0 []: 0\n"
    "W08 windows 0 [ ]: 0\n"
    "W09 windows 0 [\"]: 1 []\n"
    "W10 windows 0 [\"\"]: 1 []\n"
    "W11 windows 0 [\"\" \"\"]: 2 [] []\n"
    "W12 windows 0 [a\\\\]: 1 [a\\\\]\n"
    "W13 windows 0 [\\\\\"]: 1 [\"]\n"
    "W14 windows 0 [\\\\\\\\\"]: 1 [\\\\]\n"
    "W15 windows 0 [\"a\\\\\\\\\"]: 1 [a\\\\]\n"
    "W16 windows 0 [\"a\\\\\\\\\\\\\"b\"]: 1 [a\\\\\"b]\n"
    "W17 windows 0 [a\"\"b]: 1 [ab]\n"
    "W18 windows 0 [\"a\"\"b\"]: 1 [a\"b]\n"
    "W19 windows 0 [\"a\"\"\"b\"]: 1 [a\"b]\n"
    "W20 windows 0 [a\\nb]: 1 [a\\nb]\n"
    "W21 windows 0 [\"a\\nb\"]: 1 [a\\nb]\n"
    "W22 windows 0 [a\\x0db c]: 2 [a\\x0db] [c]\n"
    "W23 windows 0 [\\t a \\t]: 1 [a]\n"
    "W24 windows 0 [\\\\\\\\\\\\\\\\]: 1 [\\\\\\\\\\\\\\\\]\n"
    "W25 windows 0 [a\\\\b\\\\\\\\c]: 1 [a\\\\b\\\\\\\\c]\n"
    "W26 windows 0 [\"\\\\\\\\\\\\\\\\\"]: 1 [\\\\\\\\]\n"
    "W27 windows 0 [\"\"\"\"]: 1 [\"]\n"
    "W28 windows 0 [\"\"\"a\"\"\"]: 1 [\"a\"]\n"
    "W29 windows 0 [a \"\" b]: 3 [a] [] [b]\n"
    "W30 windows 0 [a\\x0bb\\x0cc]: 1 [a\\x0bb\\x0cc]\n"
    "W31 windows 0 [\\x0d\\n a \\x0d\\n]: 3 [\\x0d\\n] [a] [\\x0d\\n]\n";

// SPLIT_WINDOWS_BREAKS
//   constant: what the cases of d_tests_arg_argv_split_windows_breaks come to,
// as the references give them.
static const char SPLIT_WINDOWS_BREAKS[] =
    "W01 windows 4 [\"a b c\" d e]: 3 [a b c] [d] [e]\n"
    "W02 windows 4 [\"ab\\\\\"c\" \"\\\\\\\\\" d]: 3 [ab\"c] [\\\\] [d]\n"
    "W03 windows 4 [a\\\\\\\\\\\\b d\"e f\"g h]: 3 [a\\\\\\\\\\\\b] [de fg] "
    "[h]\n"
    "W04 windows 4 [a\\\\\\\\\\\\\"b c d]: 3 [a\\\\\"b] [c] [d]\n"
    "W05 windows 4 [a\\\\\\\\\\\\\\\\\"b c\" d e]: 3 [a\\\\\\\\b c] [d] [e]\n"
    "W06 windows 4 [a\"b\"\" c d]: 1 [ab\" c d]\n"
    "W07 windows 4 []: 0\n"
    "W08 windows 4 [ ]: 0\n"
    "W09 windows 4 [\"]: 1 []\n"
    "W10 windows 4 [\"\"]: 1 []\n"
    "W11 windows 4 [\"\" \"\"]: 2 [] []\n"
    "W12 windows 4 [a\\\\]: 1 [a\\\\]\n"
    "W13 windows 4 [\\\\\"]: 1 [\"]\n"
    "W14 windows 4 [\\\\\\\\\"]: 1 [\\\\]\n"
    "W15 windows 4 [\"a\\\\\\\\\"]: 1 [a\\\\]\n"
    "W16 windows 4 [\"a\\\\\\\\\\\\\"b\"]: 1 [a\\\\\"b]\n"
    "W17 windows 4 [a\"\"b]: 1 [ab]\n"
    "W18 windows 4 [\"a\"\"b\"]: 1 [a\"b]\n"
    "W19 windows 4 [\"a\"\"\"b\"]: 1 [a\"b]\n"
    "W20 windows 4 [a\\nb]: 2 [a] [b]\n"
    "W21 windows 4 [\"a\\nb\"]: 1 [a\\nb]\n"
    "W22 windows 4 [a\\x0db c]: 3 [a] [b] [c]\n"
    "W23 windows 4 [\\t a \\t]: 1 [a]\n"
    "W24 windows 4 [\\\\\\\\\\\\\\\\]: 1 [\\\\\\\\\\\\\\\\]\n"
    "W25 windows 4 [a\\\\b\\\\\\\\c]: 1 [a\\\\b\\\\\\\\c]\n"
    "W26 windows 4 [\"\\\\\\\\\\\\\\\\\"]: 1 [\\\\\\\\]\n"
    "W27 windows 4 [\"\"\"\"]: 1 [\"]\n"
    "W28 windows 4 [\"\"\"a\"\"\"]: 1 [\"a\"]\n"
    "W29 windows 4 [a \"\" b]: 3 [a] [] [b]\n"
    "W30 windows 4 [a\\x0bb\\x0cc]: 1 [a\\x0bb\\x0cc]\n"
    "W31 windows 4 [\\x0d\\n a \\x0d\\n]: 1 [a]\n";

// SPLIT_WINDOWS_NAMES
//   constant: what the cases of d_tests_arg_argv_split_windows_names come to,
// as the references give them.
static const char SPLIT_WINDOWS_NAMES[] =
    "W01 windows 1 [\"a b c\" d e]: 3 [a b c] [d] [e]\n"
    "W02 windows 1 [\"ab\\\\\"c\" \"\\\\\\\\\" d]: 1 [ab\\\\c \\\\\\\\ d]\n"
    "W03 windows 1 [a\\\\\\\\\\\\b d\"e f\"g h]: 3 [a\\\\\\\\\\\\b] [de fg] "
    "[h]\n"
    "W04 windows 1 [a\\\\\\\\\\\\\"b c d]: 1 [a\\\\\\\\\\\\b c d]\n"
    "W05 windows 1 [a\\\\\\\\\\\\\\\\\"b c\" d e]: 3 [a\\\\\\\\\\\\\\\\b c] "
    "[d] [e]\n"
    "W06 windows 1 [a\"b\"\" c d]: 1 [ab c d]\n"
    "W07 windows 1 []: 1 []\n"
    "W08 windows 1 [ ]: 1 []\n"
    "W09 windows 1 [\"]: 1 []\n"
    "W10 windows 1 [\"\"]: 1 []\n"
    "W11 windows 1 [\"\" \"\"]: 2 [] []\n"
    "W12 windows 1 [a\\\\]: 1 [a\\\\]\n"
    "W13 windows 1 [\\\\\"]: 1 [\\\\]\n"
    "W14 windows 1 [\\\\\\\\\"]: 1 [\\\\\\\\]\n"
    "W15 windows 1 [\"a\\\\\\\\\"]: 1 [a\\\\\\\\]\n"
    "W16 windows 1 [\"a\\\\\\\\\\\\\"b\"]: 1 [a\\\\\\\\\\\\b]\n"
    "W17 windows 1 [a\"\"b]: 1 [ab]\n"
    "W18 windows 1 [\"a\"\"b\"]: 1 [ab]\n"
    "W19 windows 1 [\"a\"\"\"b\"]: 1 [ab]\n"
    "W20 windows 1 [a\\nb]: 1 [a\\nb]\n"
    "W21 windows 1 [\"a\\nb\"]: 1 [a\\nb]\n"
    "W22 windows 1 [a\\x0db c]: 2 [a\\x0db] [c]\n"
    "W23 windows 1 [\\t a \\t]: 2 [] [a]\n"
    "W24 windows 1 [\\\\\\\\\\\\\\\\]: 1 [\\\\\\\\\\\\\\\\]\n"
    "W25 windows 1 [a\\\\b\\\\\\\\c]: 1 [a\\\\b\\\\\\\\c]\n"
    "W26 windows 1 [\"\\\\\\\\\\\\\\\\\"]: 1 [\\\\\\\\\\\\\\\\]\n"
    "W27 windows 1 [\"\"\"\"]: 1 []\n"
    "W28 windows 1 [\"\"\"a\"\"\"]: 1 [a]\n"
    "W29 windows 1 [a \"\" b]: 3 [a] [] [b]\n"
    "W30 windows 1 [a\\x0bb\\x0cc]: 1 [a\\x0bb\\x0cc]\n"
    "W31 windows 1 [\\x0d\\n a \\x0d\\n]: 3 [\\x0d\\n] [a] [\\x0d\\n]\n"
    "W32 windows 1 [\"C:\\\\Program Files\\\\x.exe\" a b]: 3 [C:\\\\Program "
    "Files\\\\x.exe] [a] [b]\n"
    "W33 windows 1 [C:\\\\x\\\\y.exe \"a b\"]: 2 [C:\\\\x\\\\y.exe] [a b]\n"
    "W34 windows 1 [\"a\"b\" c\" d]: 2 [ab c] [d]\n"
    "W35 windows 1 [\"a\"\"b\" c]: 2 [ab] [c]\n"
    "W36 windows 1 [ a b]: 3 [] [a] [b]\n"
    "W37 windows 1 [a\\\\\"b c]: 1 [a\\\\b c]\n"
    "W38 windows 1 [\"a\\\\\" b]: 2 [a\\\\] [b]\n"
    "W39 windows 1 [\\\\\\\\server\\\\x.exe a]: 2 [\\\\\\\\server\\\\x.exe] "
    "[a]\n"
    "W40 windows 1 [a\\nb c]: 2 [a\\nb] [c]\n"
    "W41 windows 1 [\"a\\nb\" c]: 2 [a\\nb] [c]\n"
    "W42 windows 1 [prog\\x0d\\nx]: 1 [prog\\x0d\\nx]\n"
    "W43 windows 1 [a\\tb]: 2 [a] [b]\n";

// SPLIT_WINDOWS_NAMES_BREAKS
//   constant: what the cases of d_tests_arg_argv_split_windows_names_breaks
// come to, as the references give them.
static const char SPLIT_WINDOWS_NAMES_BREAKS[] =
    "W01 windows 5 [\"a b c\" d e]: 3 [a b c] [d] [e]\n"
    "W02 windows 5 [\"ab\\\\\"c\" \"\\\\\\\\\" d]: 1 [ab\\\\c \\\\\\\\ d]\n"
    "W03 windows 5 [a\\\\\\\\\\\\b d\"e f\"g h]: 3 [a\\\\\\\\\\\\b] [de fg] "
    "[h]\n"
    "W04 windows 5 [a\\\\\\\\\\\\\"b c d]: 1 [a\\\\\\\\\\\\b c d]\n"
    "W05 windows 5 [a\\\\\\\\\\\\\\\\\"b c\" d e]: 3 [a\\\\\\\\\\\\\\\\b c] "
    "[d] [e]\n"
    "W06 windows 5 [a\"b\"\" c d]: 1 [ab c d]\n"
    "W07 windows 5 []: 1 []\n"
    "W08 windows 5 [ ]: 1 []\n"
    "W09 windows 5 [\"]: 1 []\n"
    "W10 windows 5 [\"\"]: 1 []\n"
    "W11 windows 5 [\"\" \"\"]: 2 [] []\n"
    "W12 windows 5 [a\\\\]: 1 [a\\\\]\n"
    "W13 windows 5 [\\\\\"]: 1 [\\\\]\n"
    "W14 windows 5 [\\\\\\\\\"]: 1 [\\\\\\\\]\n"
    "W15 windows 5 [\"a\\\\\\\\\"]: 1 [a\\\\\\\\]\n"
    "W16 windows 5 [\"a\\\\\\\\\\\\\"b\"]: 1 [a\\\\\\\\\\\\b]\n"
    "W17 windows 5 [a\"\"b]: 1 [ab]\n"
    "W18 windows 5 [\"a\"\"b\"]: 1 [ab]\n"
    "W19 windows 5 [\"a\"\"\"b\"]: 1 [ab]\n"
    "W20 windows 5 [a\\nb]: 2 [a] [b]\n"
    "W21 windows 5 [\"a\\nb\"]: 1 [a\\nb]\n"
    "W22 windows 5 [a\\x0db c]: 3 [a] [b] [c]\n"
    "W23 windows 5 [\\t a \\t]: 2 [] [a]\n"
    "W24 windows 5 [\\\\\\\\\\\\\\\\]: 1 [\\\\\\\\\\\\\\\\]\n"
    "W25 windows 5 [a\\\\b\\\\\\\\c]: 1 [a\\\\b\\\\\\\\c]\n"
    "W26 windows 5 [\"\\\\\\\\\\\\\\\\\"]: 1 [\\\\\\\\\\\\\\\\]\n"
    "W27 windows 5 [\"\"\"\"]: 1 []\n"
    "W28 windows 5 [\"\"\"a\"\"\"]: 1 [a]\n"
    "W29 windows 5 [a \"\" b]: 3 [a] [] [b]\n"
    "W30 windows 5 [a\\x0bb\\x0cc]: 1 [a\\x0bb\\x0cc]\n"
    "W31 windows 5 [\\x0d\\n a \\x0d\\n]: 2 [] [a]\n"
    "W32 windows 5 [\"C:\\\\Program Files\\\\x.exe\" a b]: 3 [C:\\\\Program "
    "Files\\\\x.exe] [a] [b]\n"
    "W33 windows 5 [C:\\\\x\\\\y.exe \"a b\"]: 2 [C:\\\\x\\\\y.exe] [a b]\n"
    "W34 windows 5 [\"a\"b\" c\" d]: 2 [ab c] [d]\n"
    "W35 windows 5 [\"a\"\"b\" c]: 2 [ab] [c]\n"
    "W36 windows 5 [ a b]: 3 [] [a] [b]\n"
    "W37 windows 5 [a\\\\\"b c]: 1 [a\\\\b c]\n"
    "W38 windows 5 [\"a\\\\\" b]: 2 [a\\\\] [b]\n"
    "W39 windows 5 [\\\\\\\\server\\\\x.exe a]: 2 [\\\\\\\\server\\\\x.exe] "
    "[a]\n"
    "W40 windows 5 [a\\nb c]: 3 [a] [b] [c]\n"
    "W41 windows 5 [\"a\\nb\" c]: 2 [a\\nb] [c]\n"
    "W42 windows 5 [prog\\x0d\\nx]: 2 [prog] [x]\n"
    "W43 windows 5 [a\\tb]: 2 [a] [b]\n";

// SPLIT_CLI11
//   constant: what the cases of d_tests_arg_argv_split_cli11 come to, as the
// references give them.
static const char SPLIT_CLI11[] =
    "C01 cli11 0 []: 0\n"
    "C02 cli11 0 [ a b ]: 2 [a] [b]\n"
    "C03 cli11 0 [--name=\"a b\" c]: 3 [--name] [a b] [c]\n"
    "C04 cli11 0 [--name='a b']: 2 [--name] [a b]\n"
    "C05 cli11 0 [--name=`a b`]: 2 [--name] [a b]\n"
    "C06 cli11 0 [-n=\"a b\"]: 2 [-n] [a b]\n"
    "C07 cli11 0 [x=\"a b\"]: 2 [x=\"a] [b\"]\n"
    "C08 cli11 0 [/name:\"a b\" c]: 3 [/name:\"a] [b\"] [c]\n"
    "C09 cli11 0 [\"a b\" c]: 2 [a b] [c]\n"
    "C10 cli11 0 [\"a \\\\\"b\\\\\" c\"]: 1 [a \"b\" c]\n"
    "C11 cli11 0 [\"a\\\\nb\"]: 1 [a\\nb]\n"
    "C12 cli11 0 [\"a\\\\tb\\\\\\\\\"]: 1 [a\\tb\\\\]\n"
    "C13 cli11 0 [\"\\\\u0041\"]: 1 [A]\n"
    "C14 cli11 0 [\"\\\\U0001F600\"]: 1 [\xf0\x9f\x98\x80]\n"
    "C15 cli11 0 [\"\\\\q\"]: ERR InvalidError: unrecognized escape sequence "
    "\\\\q in \\\\q\n"
    "C16 cli11 0 [\"\\\\u12\"]: ERR InvalidError: unicode sequence must have 4 "
    "hex codes \\\\u12\n"
    "C17 cli11 0 [\"\\\\uD800\"]: ERR InvalidError: [0xD800, 0xDFFF] are not "
    "valid code points.\n"
    "C18 cli11 0 [\"\\\\U00110000\"]: ERR InvalidError: values above 0x10FFFF "
    "are not valid code points.\n"
    "C19 cli11 0 [\"\\\\x41\"]: ERR InvalidError: unrecognized escape sequence "
    "\\\\x in \\\\x41\n"
    "C20 cli11 0 ['a\\\\nb']: 1 [a\\\\nb]\n"
    "C21 cli11 0 [`a b`]: 1 [a b]\n"
    "C22 cli11 0 [[a b] c]: 2 [[a b]] [c]\n"
    "C23 cli11 0 [(a (b) c) d]: 2 [(a (b) c)] [d]\n"
    "C24 cli11 0 [<a b>]: 1 [<a b>]\n"
    "C25 cli11 0 [{a {b} c}]: 1 [{a {b} c}]\n"
    "C26 cli11 0 [\"a\"b]: 2 [a] [b]\n"
    "C27 cli11 0 ['a'b]: 2 [a] [b]\n"
    "C28 cli11 0 [\"a\"'b']: 2 [a] [b']\n"
    "C29 cli11 0 [\"unclosed]: 1 [\"unclosed]\n"
    "C30 cli11 0 ['unclosed]: 1 ['unclosed]\n"
    "C31 cli11 0 [[unclosed]: 1 [[unclosed]\n"
    "C32 cli11 0 [\"]: 1 [\"]\n"
    "C33 cli11 0 [']: 1 [']\n"
    "C34 cli11 0 [a \"\" b]: 3 [a] [] [b]\n"
    "C35 cli11 0 [a '' b]: 3 [a] [] [b]\n"
    "C36 cli11 0 [-a=\"b c]: 2 [-a] [\"b c]\n"
    "C37 cli11 0 [=\"x\"]: 1 [=\"x\"]\n"
    "C38 cli11 0 [--a= \"b\"]: 2 [--a=] [b]\n"
    "C39 cli11 0 [--a=\"b\" --c=\"d e\"]: 4 [--a] [b] [--c] [d e]\n"
    "C40 cli11 0 [a=b]: 1 [a=b]\n"
    "C41 cli11 0 [-a=b]: 1 [-a=b]\n"
    "C42 cli11 0 [\"--a=\\\\\"b\\\\\"\"]: 1 [--a=\"b\"]\n"
    "C43 cli11 0 [--opt=[a,b]]: 1 [--opt=[a,b]]\n"
    "C44 cli11 0 [a\\nb\\tc]: 3 [a] [b] [c]\n"
    "C45 cli11 0 [\\x0b a \\x0c]: 1 [a]\n"
    "C46 cli11 0 [\"a\\\\]: 1 [\"a\\\\]\n"
    "C47 cli11 0 [\"\\\\u00e9\\\\b\\\\f\\\\r\"]: 1 [\xc3\xa9\\x08\\x0c\\x0d]\n"
    "C48 cli11 0 [[a \"]\" b] c]: 2 [[a \"]\" b]] [c]\n"
    "C49 cli11 0 [(a [b) c] d]: 1 [(a [b) c] d]\n"
    "C50 cli11 0 [[a ']' b] c]: 2 [[a ']' b]] [c]\n"
    "C51 cli11 0 [[a `]` b] c]: 2 [[a `]` b]] [c]\n"
    "C52 cli11 0 [[] x]: 2 [[]] [x]\n"
    "C53 cli11 0 [[[a] b] c]: 2 [[[a] b]] [c]\n"
    "C54 cli11 0 [`a [b` c]: 2 [a [b] [c]\n"
    "C55 cli11 0 [\"a\\\\\"]: ERR InvalidError: invalid escape sequence a\\\\\n"
    "C56 cli11 0 [\"\\\\\"]: ERR InvalidError: invalid escape sequence \\\\\n"
    "C57 cli11 0 [\"a\"\"]: 1 [a]\n"
    "C58 cli11 0 ['a'']: 1 [a]\n"
    "C59 cli11 0 [-a x=\"b c\"]: 3 [-a] [x=\"b] [c\"]\n"
    "C60 cli11 0 [-=\"b\"]: 2 [-] [b]\n"
    "C61 cli11 0 [/:\"b\"]: 1 [/:\"b\"]\n"
    "C62 cli11 0 [-a=\"]: 2 [-a] [\"]\n"
    "C63 cli11 0 [=\"x\" -]: 2 [=\"x\"] [-]\n"
    "C64 cli11 0 [:\"x\" /]: 2 [:\"x\"] [/]\n"
    "C65 cli11 0 [-a :\"b\"]: 2 [-a] [:\"b\"]\n";

// SPLIT_CLI11_WINDOWS
//   constant: what the cases of d_tests_arg_argv_split_cli11_windows come to,
// as the references give them.
static const char SPLIT_CLI11_WINDOWS[] =
    "C01 cli11 2 []: 0\n"
    "C02 cli11 2 [ a b ]: 2 [a] [b]\n"
    "C03 cli11 2 [--name=\"a b\" c]: 3 [--name] [a b] [c]\n"
    "C04 cli11 2 [--name='a b']: 2 [--name] [a b]\n"
    "C05 cli11 2 [--name=`a b`]: 2 [--name] [a b]\n"
    "C06 cli11 2 [-n=\"a b\"]: 2 [-n] [a b]\n"
    "C07 cli11 2 [x=\"a b\"]: 2 [x=\"a] [b\"]\n"
    "C08 cli11 2 [/name:\"a b\" c]: 3 [/name] [a b] [c]\n"
    "C09 cli11 2 [\"a b\" c]: 2 [a b] [c]\n"
    "C10 cli11 2 [\"a \\\\\"b\\\\\" c\"]: 1 [a \"b\" c]\n"
    "C11 cli11 2 [\"a\\\\nb\"]: 1 [a\\nb]\n"
    "C12 cli11 2 [\"a\\\\tb\\\\\\\\\"]: 1 [a\\tb\\\\]\n"
    "C13 cli11 2 [\"\\\\u0041\"]: 1 [A]\n"
    "C14 cli11 2 [\"\\\\U0001F600\"]: 1 [\xf0\x9f\x98\x80]\n"
    "C15 cli11 2 [\"\\\\q\"]: ERR InvalidError: unrecognized escape sequence "
    "\\\\q in \\\\q\n"
    "C16 cli11 2 [\"\\\\u12\"]: ERR InvalidError: unicode sequence must have 4 "
    "hex codes \\\\u12\n"
    "C17 cli11 2 [\"\\\\uD800\"]: ERR InvalidError: [0xD800, 0xDFFF] are not "
    "valid code points.\n"
    "C18 cli11 2 [\"\\\\U00110000\"]: ERR InvalidError: values above 0x10FFFF "
    "are not valid code points.\n"
    "C19 cli11 2 [\"\\\\x41\"]: ERR InvalidError: unrecognized escape sequence "
    "\\\\x in \\\\x41\n"
    "C20 cli11 2 ['a\\\\nb']: 1 [a\\\\nb]\n"
    "C21 cli11 2 [`a b`]: 1 [a b]\n"
    "C22 cli11 2 [[a b] c]: 2 [[a b]] [c]\n"
    "C23 cli11 2 [(a (b) c) d]: 2 [(a (b) c)] [d]\n"
    "C24 cli11 2 [<a b>]: 1 [<a b>]\n"
    "C25 cli11 2 [{a {b} c}]: 1 [{a {b} c}]\n"
    "C26 cli11 2 [\"a\"b]: 2 [a] [b]\n"
    "C27 cli11 2 ['a'b]: 2 [a] [b]\n"
    "C28 cli11 2 [\"a\"'b']: 2 [a] [b']\n"
    "C29 cli11 2 [\"unclosed]: 1 [\"unclosed]\n"
    "C30 cli11 2 ['unclosed]: 1 ['unclosed]\n"
    "C31 cli11 2 [[unclosed]: 1 [[unclosed]\n"
    "C32 cli11 2 [\"]: 1 [\"]\n"
    "C33 cli11 2 [']: 1 [']\n"
    "C34 cli11 2 [a \"\" b]: 3 [a] [] [b]\n"
    "C35 cli11 2 [a '' b]: 3 [a] [] [b]\n"
    "C36 cli11 2 [-a=\"b c]: 2 [-a] [\"b c]\n"
    "C37 cli11 2 [=\"x\"]: 1 [=\"x\"]\n"
    "C38 cli11 2 [--a= \"b\"]: 2 [--a=] [b]\n"
    "C39 cli11 2 [--a=\"b\" --c=\"d e\"]: 4 [--a] [b] [--c] [d e]\n"
    "C40 cli11 2 [a=b]: 1 [a=b]\n"
    "C41 cli11 2 [-a=b]: 1 [-a=b]\n"
    "C42 cli11 2 [\"--a=\\\\\"b\\\\\"\"]: 1 [--a=\"b\"]\n"
    "C43 cli11 2 [--opt=[a,b]]: 1 [--opt=[a,b]]\n"
    "C44 cli11 2 [a\\nb\\tc]: 3 [a] [b] [c]\n"
    "C45 cli11 2 [\\x0b a \\x0c]: 1 [a]\n"
    "C46 cli11 2 [\"a\\\\]: 1 [\"a\\\\]\n"
    "C47 cli11 2 [\"\\\\u00e9\\\\b\\\\f\\\\r\"]: 1 [\xc3\xa9\\x08\\x0c\\x0d]\n"
    "C48 cli11 2 [[a \"]\" b] c]: 2 [[a \"]\" b]] [c]\n"
    "C49 cli11 2 [(a [b) c] d]: 1 [(a [b) c] d]\n"
    "C50 cli11 2 [[a ']' b] c]: 2 [[a ']' b]] [c]\n"
    "C51 cli11 2 [[a `]` b] c]: 2 [[a `]` b]] [c]\n"
    "C52 cli11 2 [[] x]: 2 [[]] [x]\n"
    "C53 cli11 2 [[[a] b] c]: 2 [[[a] b]] [c]\n"
    "C54 cli11 2 [`a [b` c]: 2 [a [b] [c]\n"
    "C55 cli11 2 [\"a\\\\\"]: ERR InvalidError: invalid escape sequence a\\\\\n"
    "C56 cli11 2 [\"\\\\\"]: ERR InvalidError: invalid escape sequence \\\\\n"
    "C57 cli11 2 [\"a\"\"]: 1 [a]\n"
    "C58 cli11 2 ['a'']: 1 [a]\n"
    "C59 cli11 2 [-a x=\"b c\"]: 3 [-a] [x=\"b] [c\"]\n"
    "C60 cli11 2 [-=\"b\"]: 2 [-] [b]\n"
    "C61 cli11 2 [/:\"b\"]: 2 [/] [b]\n"
    "C62 cli11 2 [-a=\"]: 2 [-a] [\"]\n"
    "C63 cli11 2 [=\"x\" -]: 2 [=\"x\"] [-]\n"
    "C64 cli11 2 [:\"x\" /]: 2 [:\"x\"] [/]\n"
    "C65 cli11 2 [-a :\"b\"]: 2 [-a] [:\"b\"]\n";

// SPLIT_GNU
//   constant: what the cases of d_tests_arg_argv_split_gnu come to, as the
// references give them.
static const char SPLIT_GNU[] =
    "G01 gnu 0 []: 0\n"
    "G02 gnu 0 [ ]: 0\n"
    "G03 gnu 0 [ \\n\\t ]: 0\n"
    "G04 gnu 0 [a]: 1 [a]\n"
    "G05 gnu 0 [ a  b ]: 2 [a] [b]\n"
    "G06 gnu 0 ['a b']: 1 [a b]\n"
    "G07 gnu 0 [\"a b\"]: 1 [a b]\n"
    "G08 gnu 0 [a'b c'd]: 1 [ab cd]\n"
    "G09 gnu 0 ['']: 1 []\n"
    "G10 gnu 0 [\"\"]: 1 []\n"
    "G11 gnu 0 [a\\\\ b]: 1 [a b]\n"
    "G12 gnu 0 [\\\\\\\\]: 1 [\\\\]\n"
    "G13 gnu 0 ['a\\\\'b']: 1 [a'b]\n"
    "G14 gnu 0 [\"a\\\\\"b\"]: 1 [a\"b]\n"
    "G15 gnu 0 ['unclosed b]: 1 [unclosed b]\n"
    "G16 gnu 0 [\"unclosed b]: 1 [unclosed b]\n"
    "G17 gnu 0 [a\\\\]: 1 [a]\n"
    "G18 gnu 0 [\\\\]: 1 []\n"
    "G19 gnu 0 ['a\\nb']: 1 [a\\nb]\n"
    "G20 gnu 0 [a\\nb]: 2 [a] [b]\n"
    "G21 gnu 0 [\\\\\\n]: 1 [\\n]\n"
    "G22 gnu 0 ['\"']: 1 [\"]\n"
    "G23 gnu 0 [\"'\"]: 1 [']\n"
    "G24 gnu 0 [a\\x0bb\\x0cc\\x0dd]: 4 [a] [b] [c] [d]\n"
    "G25 gnu 0 ['a'\"b\"c]: 1 [abc]\n"
    "G26 gnu 0 [\"\\\\]: 1 []\n"
    "G27 gnu 0 [a''b]: 1 [ab]\n";

// SPLIT_FLAGS
//   constant: what the cases of d_tests_arg_argv_split_flags come to, as the
// references give them.
static const char SPLIT_FLAGS[] =
    "F01 posix 7 [ a  b ]: 2 [a] [b]\n"
    "F02 posix 1 [\"a b\" c]: 2 [a b] [c]\n"
    "F03 posix 4 [a\\nb c]: 3 [a] [b] [c]\n"
    "F04 windows 2 [\"a b\" c]: 2 [a b] [c]\n"
    "F05 windows 2 [a:\"b c\" d]: 2 [a:b c] [d]\n"
    "F06 windows 6 [a\\nb]: 2 [a] [b]\n"
    "F07 cli11 5 [--name=\"a b\" c]: 3 [--name] [a b] [c]\n"
    "F08 cli11 5 [/name:\"a b\" c]: 3 [/name:\"a] [b\"] [c]\n"
    "F09 cli11 7 [/name:\"a b\" c]: 3 [/name] [a b] [c]\n"
    "F10 cli11 1 [\"my prog\" a]: 2 [my prog] [a]\n"
    "F11 gnu 7 [ a  b ]: 2 [a] [b]\n"
    "F12 gnu 1 ['a b' c]: 2 [a b] [c]\n"
    "F13 gnu 2 [a:\"b c\"]: 1 [a:b c]\n";

// SPLIT_OWN
//   constant: what the lines of d_tests_arg_argv_split_own come to, the
// module's own: no reference reads them so. A shell would read the
// comments, operators and expansions; a NUL byte ends the text any of
// them is given, and a word CLI11 gives holding one -- its escape `\0`
// among them -- reads, from the list, as ending there.
static const char SPLIT_OWN[] =
    "O01 posix 0 [#a b]: 2 [#a] [b]\n"
    "O02 posix 0 [a #b]: 2 [a] [#b]\n"
    "O03 posix 0 [a;b|c&d]: 1 [a;b|c&d]\n"
    "O04 posix 0 [<a >b 2>&1]: 3 [<a] [>b] [2>&1]\n"
    "O05 posix 0 [(a) {b,c}]: 2 [(a)] [{b,c}]\n"
    "O06 posix 0 [$x ${y} $(z) `w` $((1))]: 5 [$x] [${y}] [$(z)] [`w`] "
    "[$((1))]\n"
    "O07 posix 0 [\"$x\" $'a\\\\tb']: 2 [$x] [$a\\\\tb]\n"
    "O08 posix 0 [~ ~/a a~]: 3 [~] [~/a] [a~]\n"
    "O09 posix 0 [*.c a? [ab]]: 3 [*.c] [a?] [[ab]]\n"
    "O10 posix 0 [a\\x00b c]: 2 [a] [c]\n"
    "O11 posix 0 ['\\x00' \"x\\x00\"]: 2 [] [x]\n"
    "O12 windows 0 [a\\x00b c]: 2 [a] [c]\n"
    "O13 windows 1 [\"\\x00a\" b]: 2 [] [b]\n"
    "O14 cli11 0 [a\\x00b c]: 2 [a] [c]\n"
    "O15 cli11 0 [\"\\x00\" x]: 2 [] [x]\n"
    "O16 cli11 0 [\"a\\\\0b\" c]: 2 [a] [c]\n"
    "O17 cli11 2 [\"\\\\u0000\" x]: 2 [] [x]\n"
    "O18 cli11 0 [\\x00x y]: 2 [] [y]\n"
    "O19 cli11 0 [\"\\\\u0000a\" \"b\\x00\"]: 2 [] [b]\n"
    "O20 gnu 0 [a\\x00b c]: 2 [a] [c]\n"
    "O21 gnu 0 [\\x00]: 1 []\n"
    "O22 gnu 0 [\\x00 a]: 2 [] [a]\n"
    "O23 gnu 0 ['a\\x00b' c]: 2 [a] [c]\n"
    "O24 windows 4 [\"a\\x00\\n\" b]: 2 [a] [b]\n";

// QUOTE_POSIX
//   constant: what the words of d_tests_arg_argv_quote_posix are written as,
// the module's own: CLI11 has no inverse of its split. The reference splits
// each back by its rules' references (QUOTE_CHECK), as the section does by the
// module's.
static const char QUOTE_POSIX[] =
    "Q01 posix []: ['']\n"
    "Q02 posix [a]: [a]\n"
    "Q03 posix [a b]: ['a b']\n"
    "Q04 posix [it's]: ['it'\\\\''s']\n"
    "Q05 posix [say \"hi\"]: ['say \"hi\"']\n"
    "Q06 posix [a\\\\b]: ['a\\\\b']\n"
    "Q07 posix [a\\\\]: ['a\\\\']\n"
    "Q08 posix [\\\\]: ['\\\\']\n"
    "Q09 posix [\\\\\\\\\"]: ['\\\\\\\\\"']\n"
    "Q10 posix [tab\\there]: ['tab\\there']\n"
    "Q11 posix [new\\nline]: ['new\\nline']\n"
    "Q12 posix [--opt=val]: [--opt=val]\n"
    "Q13 posix [-a=\"b\"]: ['-a=\"b\"']\n"
    "Q14 posix [-a='b']: ['-a='\\\\''b'\\\\''']\n"
    "Q15 posix [/a:`b`]: ['/a:`b`']\n"
    "Q16 posix [x=]: [x=]\n"
    "Q17 posix [x:]: [x:]\n"
    "Q18 posix [\"]: ['\"']\n"
    "Q19 posix [']: [''\\\\''']\n"
    "Q20 posix [`]: ['`']\n"
    "Q21 posix [[a]]: ['[a]']\n"
    "Q22 posix [(a)]: ['(a)']\n"
    "Q23 posix [{a}]: ['{a}']\n"
    "Q24 posix [<a>]: ['<a>']\n"
    "Q25 posix [a=']: ['a='\\\\''']\n"
    "Q26 posix [C:\\\\Program Files\\\\]: ['C:\\\\Program Files\\\\']\n"
    "Q27 posix [na\xc3\xafve]: ['na\xc3\xafve']\n"
    "Q28 posix [$HOME]: ['$HOME']\n"
    "Q29 posix [*]: ['*']\n"
    "Q30 posix [#]: ['#']\n"
    "Q31 posix [a\\x0db]: ['a\\x0db']\n"
    "Q32 posix [\\x0b]: ['\\x0b']\n"
    "Q33 posix [\\x0c]: ['\\x0c']\n"
    "Q34 posix [=]: [=]\n"
    "Q35 posix [:]: [:]\n"
    "Q36 posix [-a=]: [-a=]\n"
    "Q37 posix [\"a\"]: ['\"a\"']\n"
    "Q38 posix [a\"b\"]: ['a\"b\"']\n"
    "Q39 posix [\\x7f]: ['\\x7f']\n"
    "Q40 posix [@file]: [@file]\n"
    "Q41 posix [a=b\"c]: ['a=b\"c']\n"
    "Q42 posix [a:\"b]: ['a:\"b']\n"
    "Q43 posix [\\\\\"]: ['\\\\\"']\n"
    "Q44 posix [cr\\x0d\\nlf]: ['cr\\x0d\\nlf']\n"
    "Q45 posix [\\x08]: ['\\x08']\n"
    "Q46 posix [a b=]: ['a b=']\n"
    "Q47 posix [a b:]: ['a b:']\n"
    "Q48 posix [-x\\t=]: ['-x\\t=']\n";

// QUOTE_WINDOWS
//   constant: what the words of d_tests_arg_argv_quote_windows are written as,
// the module's own: CLI11 has no inverse of its split. The reference splits
// each back by its rules' references (QUOTE_CHECK), as the section does by the
// module's.
static const char QUOTE_WINDOWS[] =
    "Q01 windows []: [\"\"]\n"
    "Q02 windows [a]: [a]\n"
    "Q03 windows [a b]: [\"a b\"]\n"
    "Q04 windows [it's]: [it's]\n"
    "Q05 windows [say \"hi\"]: [\"say \\\\\"hi\\\\\"\"]\n"
    "Q06 windows [a\\\\b]: [a\\\\b]\n"
    "Q07 windows [a\\\\]: [a\\\\]\n"
    "Q08 windows [\\\\]: [\\\\]\n"
    "Q09 windows [\\\\\\\\\"]: [\"\\\\\\\\\\\\\\\\\\\\\"\"]\n"
    "Q10 windows [tab\\there]: [\"tab\\there\"]\n"
    "Q11 windows [new\\nline]: [\"new\\nline\"]\n"
    "Q12 windows [--opt=val]: [--opt=val]\n"
    "Q13 windows [-a=\"b\"]: [\"-a=\\\\\"b\\\\\"\"]\n"
    "Q14 windows [-a='b']: [-a='b']\n"
    "Q15 windows [/a:`b`]: [/a:`b`]\n"
    "Q16 windows [x=]: [x=]\n"
    "Q17 windows [x:]: [x:]\n"
    "Q18 windows [\"]: [\"\\\\\"\"]\n"
    "Q19 windows [']: [']\n"
    "Q20 windows [`]: [`]\n"
    "Q21 windows [[a]]: [[a]]\n"
    "Q22 windows [(a)]: [(a)]\n"
    "Q23 windows [{a}]: [{a}]\n"
    "Q24 windows [<a>]: [<a>]\n"
    "Q25 windows [a=']: [a=']\n"
    "Q26 windows [C:\\\\Program Files\\\\]: [\"C:\\\\Program Files\\\\\\\\\"]\n"
    "Q27 windows [na\xc3\xafve]: [na\xc3\xafve]\n"
    "Q28 windows [$HOME]: [$HOME]\n"
    "Q29 windows [*]: [*]\n"
    "Q30 windows [#]: [#]\n"
    "Q31 windows [a\\x0db]: [\"a\\x0db\"]\n"
    "Q32 windows [\\x0b]: [\\x0b]\n"
    "Q33 windows [\\x0c]: [\\x0c]\n"
    "Q34 windows [=]: [=]\n"
    "Q35 windows [:]: [:]\n"
    "Q36 windows [-a=]: [-a=]\n"
    "Q37 windows [\"a\"]: [\"\\\\\"a\\\\\"\"]\n"
    "Q38 windows [a\"b\"]: [\"a\\\\\"b\\\\\"\"]\n"
    "Q39 windows [\\x7f]: [\\x7f]\n"
    "Q40 windows [@file]: [@file]\n"
    "Q41 windows [a=b\"c]: [\"a=b\\\\\"c\"]\n"
    "Q42 windows [a:\"b]: [\"a:\\\\\"b\"]\n"
    "Q43 windows [\\\\\"]: [\"\\\\\\\\\\\\\"\"]\n"
    "Q44 windows [cr\\x0d\\nlf]: [\"cr\\x0d\\nlf\"]\n"
    "Q45 windows [\\x08]: [\\x08]\n"
    "Q46 windows [a b=]: [\"a b=\"]\n"
    "Q47 windows [a b:]: [\"a b:\"]\n"
    "Q48 windows [-x\\t=]: [\"-x\\t=\"]\n";

// QUOTE_CLI11
//   constant: what the words of d_tests_arg_argv_quote_cli11 are written as,
// the module's own: CLI11 has no inverse of its split. The reference splits
// each back by its rules' references (QUOTE_CHECK), as the section does by the
// module's.
static const char QUOTE_CLI11[] =
    "Q01 cli11 []: [\"\"]\n"
    "Q02 cli11 [a]: [a]\n"
    "Q03 cli11 [a b]: [\"a b\"]\n"
    "Q04 cli11 [it's]: [it's]\n"
    "Q05 cli11 [say \"hi\"]: [\"say \\\\\"hi\\\\\"\"]\n"
    "Q06 cli11 [a\\\\b]: [a\\\\b]\n"
    "Q07 cli11 [a\\\\]: [a\\\\]\n"
    "Q08 cli11 [\\\\]: [\\\\]\n"
    "Q09 cli11 [\\\\\\\\\"]: [\\\\\\\\\"]\n"
    "Q10 cli11 [tab\\there]: [\"tab\\\\there\"]\n"
    "Q11 cli11 [new\\nline]: [\"new\\\\nline\"]\n"
    "Q12 cli11 [--opt=val]: [--opt=val]\n"
    "Q13 cli11 [-a=\"b\"]: [\"-a\\\\u003d\\\\\"b\\\\\"\"]\n"
    "Q14 cli11 [-a='b']: [\"-a\\\\u003d'b'\"]\n"
    "Q15 cli11 [/a:`b`]: [\"/a\\\\u003a`b`\"]\n"
    "Q16 cli11 [x=]: [x=]\n"
    "Q17 cli11 [x:]: [x:]\n"
    "Q18 cli11 [\"]: [\"\\\\\"\"]\n"
    "Q19 cli11 [']: [\"'\"]\n"
    "Q20 cli11 [`]: [\"`\"]\n"
    "Q21 cli11 [[a]]: [\"[a]\"]\n"
    "Q22 cli11 [(a)]: [\"(a)\"]\n"
    "Q23 cli11 [{a}]: [\"{a}\"]\n"
    "Q24 cli11 [<a>]: [\"<a>\"]\n"
    "Q25 cli11 [a=']: [\"a\\\\u003d'\"]\n"
    "Q26 cli11 [C:\\\\Program Files\\\\]: [\"C:\\\\\\\\Program "
    "Files\\\\\\\\\"]\n"
    "Q27 cli11 [na\xc3\xafve]: [na\xc3\xafve]\n"
    "Q28 cli11 [$HOME]: [$HOME]\n"
    "Q29 cli11 [*]: [*]\n"
    "Q30 cli11 [#]: [#]\n"
    "Q31 cli11 [a\\x0db]: [\"a\\\\rb\"]\n"
    "Q32 cli11 [\\x0b]: [\"\\x0b\"]\n"
    "Q33 cli11 [\\x0c]: [\"\\\\f\"]\n"
    "Q34 cli11 [=]: [=]\n"
    "Q35 cli11 [:]: [:]\n"
    "Q36 cli11 [-a=]: [-a=]\n"
    "Q37 cli11 [\"a\"]: [\"\\\\\"a\\\\\"\"]\n"
    "Q38 cli11 [a\"b\"]: [a\"b\"]\n"
    "Q39 cli11 [\\x7f]: [\\x7f]\n"
    "Q40 cli11 [@file]: [@file]\n"
    "Q41 cli11 [a=b\"c]: [a=b\"c]\n"
    "Q42 cli11 [a:\"b]: [\"a\\\\u003a\\\\\"b\"]\n"
    "Q43 cli11 [\\\\\"]: [\\\\\"]\n"
    "Q44 cli11 [cr\\x0d\\nlf]: [\"cr\\\\r\\\\nlf\"]\n"
    "Q45 cli11 [\\x08]: [\\x08]\n"
    "Q46 cli11 [a b=]: [\"a b\\\\u003d\"]\n"
    "Q47 cli11 [a b:]: [\"a b\\\\u003a\"]\n"
    "Q48 cli11 [-x\\t=]: [\"-x\\\\t\\\\u003d\"]\n";

// QUOTE_GNU
//   constant: what the words of d_tests_arg_argv_quote_gnu are written as, the
// module's own: CLI11 has no inverse of its split. The reference splits each
// back by its rules' references (QUOTE_CHECK), as the section does by the
// module's.
static const char QUOTE_GNU[] =
    "Q01 gnu []: ['']\n"
    "Q02 gnu [a]: [a]\n"
    "Q03 gnu [a b]: [a\\\\ b]\n"
    "Q04 gnu [it's]: [it\\\\'s]\n"
    "Q05 gnu [say \"hi\"]: [say\\\\ \\\\\"hi\\\\\"]\n"
    "Q06 gnu [a\\\\b]: [a\\\\\\\\b]\n"
    "Q07 gnu [a\\\\]: [a\\\\\\\\]\n"
    "Q08 gnu [\\\\]: [\\\\\\\\]\n"
    "Q09 gnu [\\\\\\\\\"]: [\\\\\\\\\\\\\\\\\\\\\"]\n"
    "Q10 gnu [tab\\there]: [tab\\\\\\there]\n"
    "Q11 gnu [new\\nline]: [new\\\\\\nline]\n"
    "Q12 gnu [--opt=val]: [--opt=val]\n"
    "Q13 gnu [-a=\"b\"]: [-a=\\\\\"b\\\\\"]\n"
    "Q14 gnu [-a='b']: [-a=\\\\'b\\\\']\n"
    "Q15 gnu [/a:`b`]: [/a:`b`]\n"
    "Q16 gnu [x=]: [x=]\n"
    "Q17 gnu [x:]: [x:]\n"
    "Q18 gnu [\"]: [\\\\\"]\n"
    "Q19 gnu [']: [\\\\']\n"
    "Q20 gnu [`]: [`]\n"
    "Q21 gnu [[a]]: [[a]]\n"
    "Q22 gnu [(a)]: [(a)]\n"
    "Q23 gnu [{a}]: [{a}]\n"
    "Q24 gnu [<a>]: [<a>]\n"
    "Q25 gnu [a=']: [a=\\\\']\n"
    "Q26 gnu [C:\\\\Program Files\\\\]: [C:\\\\\\\\Program\\\\ Files\\\\\\\\]\n"
    "Q27 gnu [na\xc3\xafve]: [na\xc3\xafve]\n"
    "Q28 gnu [$HOME]: [$HOME]\n"
    "Q29 gnu [*]: [*]\n"
    "Q30 gnu [#]: [#]\n"
    "Q31 gnu [a\\x0db]: [a\\\\\\x0db]\n"
    "Q32 gnu [\\x0b]: [\\\\\\x0b]\n"
    "Q33 gnu [\\x0c]: [\\\\\\x0c]\n"
    "Q34 gnu [=]: [=]\n"
    "Q35 gnu [:]: [:]\n"
    "Q36 gnu [-a=]: [-a=]\n"
    "Q37 gnu [\"a\"]: [\\\\\"a\\\\\"]\n"
    "Q38 gnu [a\"b\"]: [a\\\\\"b\\\\\"]\n"
    "Q39 gnu [\\x7f]: [\\x7f]\n"
    "Q40 gnu [@file]: [@file]\n"
    "Q41 gnu [a=b\"c]: [a=b\\\\\"c]\n"
    "Q42 gnu [a:\"b]: [a:\\\\\"b]\n"
    "Q43 gnu [\\\\\"]: [\\\\\\\\\\\\\"]\n"
    "Q44 gnu [cr\\x0d\\nlf]: [cr\\\\\\x0d\\\\\\nlf]\n"
    "Q45 gnu [\\x08]: [\\x08]\n"
    "Q46 gnu [a b=]: [a\\\\ b=]\n"
    "Q47 gnu [a b:]: [a\\\\ b:]\n"
    "Q48 gnu [-x\\t=]: [-x\\\\\\t=]\n";

// PROGRAM_NAMES
//   constant: what the cases of d_tests_arg_argv_program_names come to, as the
// references give them.
static const char PROGRAM_NAMES[] =
    "N01 []: [] []\n"
    "N02 [ ]: [] []\n"
    "N03 [prog]: [prog] []\n"
    "N04 [pd/prog]: [pd/prog] []\n"
    "N05 [pd/prog a b]: [pd/prog] [a b]\n"
    "N06 [pd/my prog a]: [pd/my prog] [a]\n"
    "N07 [  pd/my prog  a  ]: [pd/my prog] [a]\n"
    "N08 [pd/a b c d]: [pd/a] [b c d]\n"
    "N09 [pd/dir with space/tool x]: [pd/dir with space/tool] [x]\n"
    "N10 [pd/only dir/x]: [pd/only] [dir/x]\n"
    "N11 [\"pd/my prog\" a]: [pd/my prog] [a]\n"
    "N12 [\"my prog\" a]: [my prog] [a]\n"
    "N13 [\"my \\\\\"prog\\\\\"\" a]: [my \"prog\"] [a]\n"
    "N14 ['quoted name' rest]: [quoted name] [rest]\n"
    "N15 [`back` x]: [back] [x]\n"
    "N16 [\"unclosed name a]: [\"unclosed] [name a]\n"
    "N17 [\"\"]: [\"\"] []\n"
    "N18 [\"\" a]: [\"\"] [a]\n"
    "N19 [\"a\"x b]: [a] [b]\n"
    "N20 [plain]: [plain] []\n"
    "N21 [plain  rest  ]: [plain] [rest]\n"
    "N22 [\\tplain\\trest]: [plain\\trest] []\n"
    "N23 [pd/a  b]: [pd/a] [b]\n"
    "N24 [pd/x y]: [pd/x y] []\n"
    "N25 [\"pd/x y\"z rest]: [pd/x y] [rest]\n"
    "N26 [\"a\\\\\"b\\\\\"c\" d]: [a\"b\"c] [d]\n"
    "N27 ['it\\\\'s' x]: [it's] [x]\n"
    "N28 [pd/only dir]: [pd/only] [dir]\n"
    "N29 [pd/my]: [pd/my] []\n"
    "N30 [pd/dir with space]: [pd/dir] [with space]\n"
    "N31 [\"pd/only dir\" a]: [pd/only dir] [a]\n"
    "N32 [a \"b c\"]: [a] [\"b c\"]\n"
    "N33 [pd/my prog]: [pd/my prog] []\n"
    "N34 [pd/a b c]: [pd/a] [b c]\n"
    "N35 [\"pd/prog\"]: [pd/prog] []\n"
    "N36 ['pd/my prog' x]: [pd/my prog] [x]\n"
    "N37 [\"a\\\\\" b]: [\"a\\\\\"] [b]\n"
    "N38 [\"\\\\\"]: [\"\\\\\"] []\n"
    "N39 [a\\tb c]: [a\\tb] [c]\n"
    "N40 [q r s]: [q] [r s]\n"
    "N41 [pd/s p  q]: [pd/s p] [q]\n"
    "N42 [\"a\\\\b\\\\\"c\" d]: [a\\\\b\"c] [d]\n"
    "N43 [\"a\\\\\"\"]: [a\"] []\n";

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// RESPONSE_READ
//   constant: what the cases of d_tests_arg_argv_response_read come to, as the
// references give them.
static const char RESPONSE_READ[] =
    "R01 posix simple.rsp: 3 [a] [b] [c]\n"
    "R02 posix gnuq.rsp: ERR InvalidError\n"
    "R03 posix crlf.rsp: 3 [one\\x0d] [two] [three\\x0d]\n"
    "R04 posix ws.rsp: 1 [\\x0d]\n"
    "R05 posix empty.rsp: 0\n"
    "R06 posix quote_open.rsp: ERR InvalidError\n"
    "R07 posix nested1.rsp: 3 [x] [@simple.rsp] [y]\n"
    "R08 posix bom8.rsp: 2 [bom8] [x]\n"
    "R09 posix bom16be.rsp: 3 [be] [y] [\xf0\x9f\x98\x80]\n"
    "R10 posix bomonly16le.rsp: 0\n"
    "R11 windows simple.rsp: 3 [a] [b] [c]\n"
    "R12 windows winq.rsp: 6 [a b] [c\"d] [e\\\\\\\\f] [g\"h] [i\\\\\"j] [k]\n"
    "R13 windows crlf.rsp: 3 [one] [two] [three]\n"
    "R14 windows ws.rsp: 0\n"
    "R15 windows empty.rsp: 0\n"
    "R16 windows bom8.rsp: 2 [bom8] [x]\n"
    "R17 windows bom16le.rsp: 3 [le] [w x] [\xd0\xb6]\n"
    "R18 windows bom16be.rsp: 3 [be] [y] [\xf0\x9f\x98\x80]\n"
    "R19 windows latin1.rsp: 2 [caf\xe9] [na\xefve]\n"
    "R20 windows bad16le.rsp: 3 [\xef\xbf\xbd] [a] [\xef\xbf\xbd\xef\xbf\xbd]\n"
    "R21 windows bad16be.rsp: 2 [\xef\xbf\xbd\xef\xbf\xbd] [z]\n"
    "R22 windows odd16le.rsp: 1 [ab\xef\xbf\xbd]\n"
    "R23 windows pairs16le.rsp: 4 [\xf0\x90\x80\x80] [\xf4\x8f\xbf\xbf] "
    "[\xed\x9f\xbf] [\xee\x80\x80]\n"
    "R24 windows bomonly16le.rsp: 0\n"
    "R25 windows bomonly16be.rsp: 0\n"
    "R26 windows bomonly8.rsp: 0\n"
    "R27 cli11 simple.rsp: 3 [a] [b] [c]\n"
    "R28 cli11 gnuq.rsp: 8 [a b] [c d] [e\\\\] [f] [g\\\\\\\\h] [it\\\\] [s'] "
    "[x\"y]\n"
    "R29 cli11 winq.rsp: 7 [a b] [c\\\\\"d] [e\\\\\\\\f] [g] [h\"] "
    "[i\\\\\\\\\\\\\"j] [k]\n"
    "R30 cli11 crlf.rsp: 3 [one] [two] [three]\n"
    "R31 cli11 ws.rsp: 0\n"
    "R32 cli11 empty.rsp: 0\n"
    "R33 cli11 bom16le.rsp: 3 [le] [w x] [\xd0\xb6]\n"
    "R34 gnu simple.rsp: 3 [a] [b] [c]\n"
    "R35 gnu gnuq.rsp: 6 [a b] [c d] [e f] [g\\\\h] [it's] [x\"y]\n"
    "R36 gnu winq.rsp: 6 [a b] [c\"d] [e\\\\f] [gh] [i\\\\\"j] [k]\n"
    "R37 gnu crlf.rsp: 3 [one] [two] [three]\n"
    "R38 gnu ws.rsp: 0\n"
    "R39 gnu empty.rsp: 0\n"
    "R40 gnu latin1.rsp: 2 [caf\xe9] [na\xefve]\n"
    "R41 gnu quote_open.rsp: 2 [a] [unclosed]\n"
    "R42 gnu nested1.rsp: 3 [x] [@simple.rsp] [y]\n"
    "R43 gnu bom8.rsp: 2 [bom8] [x]\n"
    "R44 gnu bom16le.rsp: 3 [le] [w x] [\xd0\xb6]\n"
    "R45 gnu bom8nl.rsp: 2 [a] [b]\n"
    "R46 gnu bomonly8.rsp: 0\n"
    "R47 posix nulbyte.rsp: 5 [a] [c] [@simple.rsp] [] [e\\\\0f]\n"
    "R48 windows nulbyte.rsp: 5 [a] [c] [@simple.rsp] [] [e\\\\0f]\n"
    "R49 cli11 nulbyte.rsp: 5 [a] [c] [@simple.rsp] [] [e]\n"
    "R50 gnu nulbyte.rsp: 5 [a] [c] [@simple.rsp] [] [e0f]\n";

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// RESPONSE_EXPAND_GNU
//   constant: what the cases of d_tests_arg_argv_response_expand_gnu come to,
// as the references give them.
static const char RESPONSE_EXPAND_GNU[] =
    "E01 gnu [@simple.rsp @gnuq.rsp]\n"
    "cmd app n=1 miss=[a|b|c|a b|c d|e f|g\\\\h|it's|x\"y] subs=[] order=[]\n"
    "E02 gnu [x @simple.rsp y @gnuq.rsp]\n"
    "cmd app n=1 miss=[x|a|b|c|y|a b|c d|e f|g\\\\h|it's|x\"y] subs=[] "
    "order=[]\n"
    "E03 gnu [@nested2.rsp @gnuq.rsp]\n"
    "cmd app n=1 miss=[x|a|b|c|y|z|a|b|c|a b|c d|e f|g\\\\h|it's|x\"y] subs=[] "
    "order=[]\n"
    "E04 gnu [@empty.rsp a @gnuq.rsp]\n"
    "cmd app n=1 miss=[a|a b|c d|e f|g\\\\h|it's|x\"y] subs=[] order=[]\n"
    "E05 gnu [@ws.rsp @gnuq.rsp]\n"
    "cmd app n=1 miss=[a b|c d|e f|g\\\\h|it's|x\"y] subs=[] order=[]\n"
    "E06 gnu [@crlf.rsp @gnuq.rsp]\n"
    "cmd app n=1 miss=[one|two|three|a b|c d|e f|g\\\\h|it's|x\"y] subs=[] "
    "order=[]\n"
    "E07 gnu [@missing.rsp b @gnuq.rsp]\n"
    "cmd app n=1 miss=[@missing.rsp|b|a b|c d|e f|g\\\\h|it's|x\"y] subs=[] "
    "order=[]\n"
    "E08 gnu [@ c @gnuq.rsp]\n"
    "cmd app n=1 miss=[@|c|a b|c d|e f|g\\\\h|it's|x\"y] subs=[] order=[]\n"
    "E09 gnu [@@simple.rsp @gnuq.rsp]\n"
    "cmd app n=1 miss=[@@simple.rsp|a b|c d|e f|g\\\\h|it's|x\"y] subs=[] "
    "order=[]\n"
    "E10 gnu [-f @simple.rsp --opt=@x @gnuq.rsp]\n"
    "cmd app n=1 miss=[-f|a|b|c|--opt=@x|a b|c d|e f|g\\\\h|it's|x\"y] subs=[] "
    "order=[]\n"
    "E11 gnu [@latin1.rsp @gnuq.rsp]\n"
    "cmd app n=1 miss=[caf\xe9|na\xefve|a b|c d|e f|g\\\\h|it's|x\"y] subs=[] "
    "order=[]\n"
    "E12 gnu [@simple.rsp @simple.rsp @gnuq.rsp]\n"
    "cmd app n=1 miss=[a|b|c|a|b|c|a b|c d|e f|g\\\\h|it's|x\"y] subs=[] "
    "order=[]\n"
    "E13 gnu [plain @gnuq.rsp]\n"
    "cmd app n=1 miss=[plain|a b|c d|e f|g\\\\h|it's|x\"y] subs=[] order=[]\n"
    "E14 gnu [@gnuq.rsp]\n"
    "cmd app n=1 miss=[a b|c d|e f|g\\\\h|it's|x\"y] subs=[] order=[]\n"
    "E15 gnu [@deep5.rsp @gnuq.rsp]\n"
    "cmd app n=1 miss=[bottom|d10|d9|d8|d7|d6|d5|a b|c d|e f|g\\\\h|it's|x\"y] "
    "subs=[] order=[]\n"
    "E16 gnu [@r @gnuq.rsp]\n"
    "cmd app n=1 miss=[one|char|a b|c d|e f|g\\\\h|it's|x\"y] subs=[] "
    "order=[]\n";

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// RESPONSE_EXPAND_WINDOWS
//   constant: what the cases of d_tests_arg_argv_response_expand_windows come
// to, as the references give them.
static const char RESPONSE_EXPAND_WINDOWS[] =
    "E01 windows [@simple.rsp @winq.rsp]\n"
    "cmd app n=1 miss=[a|b|c|a b|c\"d|e\\\\\\\\f|g\"h|i\\\\\"j|k] subs=[] "
    "order=[]\n"
    "E02 windows [x @simple.rsp y @winq.rsp]\n"
    "cmd app n=1 miss=[x|a|b|c|y|a b|c\"d|e\\\\\\\\f|g\"h|i\\\\\"j|k] subs=[] "
    "order=[]\n"
    "E03 windows [@nested2.rsp @winq.rsp]\n"
    "cmd app n=1 miss=[x|a|b|c|y|z|a|b|c|a b|c\"d|e\\\\\\\\f|g\"h|i\\\\\"j|k] "
    "subs=[] order=[]\n"
    "E04 windows [@empty.rsp a @winq.rsp]\n"
    "cmd app n=1 miss=[a|a b|c\"d|e\\\\\\\\f|g\"h|i\\\\\"j|k] subs=[] "
    "order=[]\n"
    "E05 windows [@ws.rsp @winq.rsp]\n"
    "cmd app n=1 miss=[a b|c\"d|e\\\\\\\\f|g\"h|i\\\\\"j|k] subs=[] order=[]\n"
    "E06 windows [@crlf.rsp @winq.rsp]\n"
    "cmd app n=1 miss=[one|two|three|a b|c\"d|e\\\\\\\\f|g\"h|i\\\\\"j|k] "
    "subs=[] order=[]\n"
    "E07 windows [@missing.rsp b @winq.rsp]\n"
    "cmd app n=1 miss=[@missing.rsp|b|a b|c\"d|e\\\\\\\\f|g\"h|i\\\\\"j|k] "
    "subs=[] order=[]\n"
    "E08 windows [@@simple.rsp @winq.rsp]\n"
    "cmd app n=1 miss=[@@simple.rsp|a b|c\"d|e\\\\\\\\f|g\"h|i\\\\\"j|k] "
    "subs=[] order=[]\n"
    "E09 windows [-f @simple.rsp --opt=@x @winq.rsp]\n"
    "cmd app n=1 miss=[-f|a|b|c|--opt=@x|a b|c\"d|e\\\\\\\\f|g\"h|i\\\\\"j|k] "
    "subs=[] order=[]\n"
    "E10 windows [@latin1.rsp @winq.rsp]\n"
    "cmd app n=1 miss=[caf\xe9|na\xefve|a b|c\"d|e\\\\\\\\f|g\"h|i\\\\\"j|k] "
    "subs=[] order=[]\n"
    "E11 windows [@simple.rsp @simple.rsp @winq.rsp]\n"
    "cmd app n=1 miss=[a|b|c|a|b|c|a b|c\"d|e\\\\\\\\f|g\"h|i\\\\\"j|k] "
    "subs=[] order=[]\n"
    "E12 windows [plain @winq.rsp]\n"
    "cmd app n=1 miss=[plain|a b|c\"d|e\\\\\\\\f|g\"h|i\\\\\"j|k] subs=[] "
    "order=[]\n"
    "E13 windows [@winq.rsp]\n"
    "cmd app n=1 miss=[a b|c\"d|e\\\\\\\\f|g\"h|i\\\\\"j|k] subs=[] order=[]\n"
    "E14 windows [@deep5.rsp @winq.rsp]\n"
    "cmd app n=1 miss=[bottom|d10|d9|d8|d7|d6|d5|a "
    "b|c\"d|e\\\\\\\\f|g\"h|i\\\\\"j|k] subs=[] order=[]\n"
    "E15 windows [@r @winq.rsp]\n"
    "cmd app n=1 miss=[one|char|a b|c\"d|e\\\\\\\\f|g\"h|i\\\\\"j|k] subs=[] "
    "order=[]\n"
    "E16 windows [@bom8.rsp]\n"
    "cmd app n=1 miss=[bom8|x] subs=[] order=[]\n"
    "E17 windows [@bom16le.rsp z]\n"
    "cmd app n=1 miss=[le|w x|\xd0\xb6|z] subs=[] order=[]\n"
    "E18 windows [@bom16be.rsp]\n"
    "cmd app n=1 miss=[be|y|\xf0\x9f\x98\x80] subs=[] order=[]\n";

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// RESPONSE_EXPAND_OWN
//   constant: what the cases of d_tests_arg_argv_response_expand_own come
// to, the module's own, which neither expandargv nor LLVM has: the
// arguments each is to give CLI11, or its failure. Where its files are
// read whole, the words are the reference's split of their text.
static const char RESPONSE_EXPAND_OWN[] =
    "E01 gnu [@dash.rsp @simple.rsp]\n"
    "cmd app n=1 miss=[p|--|@simple.rsp|q|@simple.rsp] subs=[] order=[]\n"
    "E02 gnu [-- @simple.rsp]\n"
    "cmd app n=1 miss=[--|@simple.rsp] subs=[] order=[]\n"
    "E03 gnu [a @simple.rsp -- @simple.rsp]\n"
    "cmd app n=1 miss=[a|a|b|c|--|@simple.rsp] subs=[] order=[]\n"
    "E04 gnu [@deep1.rsp]\n"
    "ERR FileError 103: deep9.rsp: response files nest more than 8 deep\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "E05 gnu [@deep4.rsp]\n"
    "cmd app n=1 miss=[bottom|d10|d9|d8|d7|d6|d5|d4] subs=[] order=[]\n"
    "E06 gnu [@deep3.rsp]\n"
    "ERR FileError 103: deep11.rsp: response files nest more than 8 deep\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "E07 gnu [@self.rsp]\n"
    "ERR FileError 103: self.rsp: response files nest more than 8 deep\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "E08 gnu [@selfat.rsp]\n"
    "ERR FileError 103: selfat.rsp: response files nest more than 8 deep\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "E09 gnu [@adir x]\n"
    "cmd app n=1 miss=[@adir|x] subs=[] order=[]\n"
    "E10 posix [@quote_open.rsp]\n"
    "ERR InvalidError 111: quote_open.rsp: a single quote is not closed\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "E11 posix [a @simple.rsp @crlf.rsp]\n"
    "cmd app n=1 miss=[a|a|b|c|one\\x0d|two|three\\x0d] subs=[] order=[]\n"
    "E12 cli11 [@gnuq.rsp z @winq.rsp]\n"
    "cmd app n=1 miss=[a b|c d|e\\\\|f|g\\\\\\\\h|it\\\\|s'|x\"y|z|a "
    "b|c\\\\\"d|e\\\\\\\\f|g|h\"|i\\\\\\\\\\\\\"j|k] subs=[] order=[]\n"
    "E13 cli11 [@badesc.rsp]\n"
    "ERR InvalidError 111: badesc.rsp: unrecognized escape sequence \\\\q in "
    "a\\\\qb\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "E14 windows [@bad16le.rsp @odd16le.rsp]\n"
    "cmd app n=1 miss=[\xef\xbf\xbd|a|\xef\xbf\xbd\xef\xbf\xbd|ab\xef\xbf\xbd] "
    "subs=[] order=[]\n"
    "E15 gnu [@bom8.rsp @bom16be.rsp]\n"
    "cmd app n=1 miss=[bom8|x|be|y|\xf0\x9f\x98\x80] subs=[] order=[]\n"
    "E16 gnu [@nulbyte.rsp z]\n"
    "cmd app n=1 miss=[a\\x00b|c\\x00d|@simple.rsp\\x00x|\\x00|e0f|z] subs=[] "
    "order=[]\n"
    "E17 posix [@nulbyte.rsp]\n"
    "cmd app n=1 miss=[a\\x00b|c\\x00d|@simple.rsp\\x00x|\\x00|e\\\\0f] "
    "subs=[] order=[]\n"
    "E18 windows [@nulbyte.rsp]\n"
    "cmd app n=1 miss=[a\\x00b|c\\x00d|@simple.rsp\\x00x|\\x00|e\\\\0f] "
    "subs=[] order=[]\n"
    "E19 cli11 [z @nulbyte.rsp]\n"
    "cmd app n=1 miss=[z|a\\x00b|c\\x00d|@simple.rsp\\x00x|\\x00|e\\x00f] "
    "subs=[] order=[]\n";

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// RESPONSE_EXPAND_ENTRIES
//   constant: what the cases of d_tests_arg_argv_response_expand_entries
// come to: the arguments each is to give CLI11 -- libiberty's expandargv's
// or LLVM's expansion of its words, or, by the module's own rules, by
// hand.
static const char RESPONSE_EXPAND_ENTRIES[] =
    "A01 args gnu [x @simple.rsp y]\n"
    "cmd app n=1 miss=[x|a|b|c|y] subs=[] order=[]\n"
    "A02 args windows [@winq.rsp z]\n"
    "cmd app n=1 miss=[a b|c\"d|e\\\\\\\\f|g\"h|i\\\\\"j|k|z] subs=[] "
    "order=[]\n"
    "A03 reversed gnu [q @nested2.rsp]\n"
    "cmd app n=1 miss=[x|a|b|c|y|z|a|b|c|q] subs=[] order=[]\n"
    "A04 line gnu [prog x \"@simple.rsp\" y]\n"
    "cmd app n=1 miss=[x|a|b|c|y] subs=[] order=[]\n"
    "A05 line cli11 [prog @simple.rsp -- @simple.rsp]\n"
    "cmd app n=1 miss=[a|b|c|--|@simple.rsp] subs=[] order=[]\n"
    "A06 args gnu [@simple.rsp\\x00x @simple.rsp]\n"
    "cmd app n=1 miss=[@simple.rsp\\x00x|a|b|c] subs=[] order=[]\n"
    "A07 line gnu [prog \"@simple.rsp\\\\0x\" z]\n"
    "cmd app n=1 miss=[@simple.rsp\\x00x|z] subs=[] order=[]\n";

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

// d_tests_arg_argv_split
//   struct: a line a splitting section splits: its tag; its rules, a
// d_arg_quoting value; the D_ARG_SPLIT_* switches; and the text and its
// length, which may hold NUL bytes.
struct d_tests_arg_argv_split
{
    const char* tag;
    int         quoting;
    uint32_t    flags;
    const char* text;
    size_t      length;
};

// d_tests_arg_argv_quote
//   struct: a word a quoting section writes: its tag, its rules, and the
// word and its length.
struct d_tests_arg_argv_quote
{
    const char* tag;
    int         quoting;
    const char* word;
    size_t      length;
};

// d_tests_arg_argv_place
//   struct: a file, or a folder, made for a program's name to be found
// among: its name, relative to the scratch directory, and which it is.
struct d_tests_arg_argv_place
{
    const char* name;
    bool        folder;
};

// PROGRAM_PLACES
//   constant: the folders and files d_tests_arg_argv_program_names finds names
// among, made in its scratch directory in this order, each folder before what
// it holds.
static const struct d_tests_arg_argv_place PROGRAM_PLACES[] =
{
    { "pd", true },
    { "pd/dir with space", true },
    { "pd/only dir", true },
    { "pd/prog", false },
    { "pd/my prog", false },
    { "pd/a", false },
    { "pd/a b c", false },
    { "pd/dir with space/tool", false },
    { "pd/x y", false },
    { "q", false },
    { "q r", false },
    { "pd/s p ", false }
};

// PLACE_COUNT
//   constant: how many places PROGRAM_PLACES holds.
static const size_t PLACE_COUNT =
    sizeof(PROGRAM_PLACES) / sizeof(PROGRAM_PLACES[0]);

// d_tests_arg_argv_program
//   struct: a command line a program's name is found at the start of: its
// tag and the line.
struct d_tests_arg_argv_program
{
    const char* tag;
    const char* line;
};

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// USE_READ / USE_EXPAND / USE_OWN / USE_REFUSALS
//   constant: the sections a response file is made for, the bits of its
// `uses`: a scratch directory holds no more than D_TESTS_ARG_FILES_MAX, so
// each section makes the files it reads -- the reading section those its
// rows name; GCC's and Windows' expanding sections those their arguments
// name and those name in turn, and the module's own expanding section
// those its arguments name and so on; the refusals' the three they read.
enum
{
    USE_READ     = 1,
    USE_EXPAND   = 2,
    USE_OWN      = 4,
    USE_REFUSALS = 8
};

// d_tests_arg_argv_response
//   struct: a response file made in a scratch directory: its name; the
// bytes it holds and how many, NULL bytes for a folder; and the sections it
// is made for, USE_* bits.
struct d_tests_arg_argv_response
{
    const char*  name;
    const char*  data;
    size_t       size;
    unsigned int uses;
};

// RESPONSE_FILES
//   constant: the response files the response sections read, and the sections
// each is made for; `adir` is a folder.
static const struct d_tests_arg_argv_response RESPONSE_FILES[] =
{
    { "simple.rsp",
      "a b c", 5u,
      USE_READ | USE_EXPAND | USE_OWN | USE_REFUSALS },
    { "gnuq.rsp",
      "\"a b\" 'c d' e\\ f g\\\\h 'it\\'s' \"x\\\"y\"\n", 37u,
      USE_READ | USE_EXPAND | USE_OWN },
    { "winq.rsp",
      "\"a b\" c\\\"d e\\\\f \"g\"\"h\" i\\\\\\\"j\r\nk\n", 33u,
      USE_READ | USE_EXPAND | USE_OWN },
    { "nested1.rsp", "x @simple.rsp y", 15u, USE_READ | USE_EXPAND },
    { "nested2.rsp", "@nested1.rsp z\n@simple.rsp", 26u, USE_EXPAND },
    { "empty.rsp", "", 0u, USE_READ | USE_EXPAND },
    { "ws.rsp", "  \n\t \r\n", 7u, USE_READ | USE_EXPAND },
    { "crlf.rsp",
      "one\r\ntwo three\r\n", 16u,
      USE_READ | USE_EXPAND | USE_OWN },
    { "bom8.rsp",
      "\xef\xbb\xbf" "bom8 x", 9u,
      USE_READ | USE_EXPAND | USE_OWN },
    { "bom16le.rsp",
      "\xff\xfel\x00" "e\x00 \x00\"\x00w\x00 \x00x\x00\"\x00 \x00" "6\x04", 22u,
      USE_READ | USE_EXPAND },
    { "bom16be.rsp",
      "\xfe\xff\x00" "b\x00" "e\x00 \x00y\x00 \xd8=\xde\x00", 16u,
      USE_READ | USE_EXPAND | USE_OWN },
    { "latin1.rsp", "caf\xe9 na\xefve", 10u, USE_READ | USE_EXPAND },
    { "dash.rsp", "p -- @simple.rsp q", 18u, USE_OWN },
    { "self.rsp", "@self.rsp", 9u, USE_OWN },
    { "deep1.rsp", "@deep2.rsp d1", 13u, USE_OWN },
    { "deep2.rsp", "@deep3.rsp d2", 13u, USE_OWN },
    { "deep3.rsp", "@deep4.rsp d3", 13u, USE_OWN },
    { "deep4.rsp", "@deep5.rsp d4", 13u, USE_OWN },
    { "deep5.rsp", "@deep6.rsp d5", 13u, USE_EXPAND | USE_OWN },
    { "deep6.rsp", "@deep7.rsp d6", 13u, USE_EXPAND | USE_OWN },
    { "deep7.rsp", "@deep8.rsp d7", 13u, USE_EXPAND | USE_OWN },
    { "deep8.rsp", "@deep9.rsp d8", 13u, USE_EXPAND | USE_OWN },
    { "deep9.rsp", "@deep10.rsp d9", 14u, USE_EXPAND | USE_OWN },
    { "deep10.rsp", "@deep11.rsp d10", 15u, USE_EXPAND | USE_OWN },
    { "deep11.rsp", "bottom", 6u, USE_EXPAND | USE_OWN },
    { "quote_open.rsp", "a 'unclosed", 11u, USE_READ | USE_OWN | USE_REFUSALS },
    { "bad16le.rsp",
      "\xff\xfe=\xd8 \x00" "a\x00 \x00\x00\xde=\xd8", 14u,
      USE_READ | USE_OWN },
    { "bad16be.rsp", "\xfe\xff\xde\x00\xd8=\x00 \x00z", 10u, USE_READ },
    { "odd16le.rsp", "\xff\xfe" "a\x00" "b\x00" "c", 7u, USE_READ | USE_OWN },
    { "pairs16le.rsp",
      "\xff\xfe\x00\xd8\x00\xdc \x00\xff\xdb\xff\xdf \x00\xff\xd7 "
      "\x00\x00\xe0", 20u,
      USE_READ },
    { "badesc.rsp", "\"a\\qb\" c", 8u, USE_OWN },
    { "bomonly16le.rsp", "\xff\xfe", 2u, USE_READ },
    { "bomonly16be.rsp", "\xfe\xff", 2u, USE_READ },
    { "bomonly8.rsp", "\xef\xbb\xbf", 3u, USE_READ },
    { "bom8nl.rsp", "\xef\xbb\xbf" "a b\n", 7u, USE_READ },
    { "selfat.rsp", "@ @selfat.rsp", 13u, USE_OWN },
    { "r", "one char", 8u, USE_EXPAND },
    { "nulbyte.rsp",
      "a\x00" "b \"c\x00" "d\" @simple.rsp\x00x \x00 \"e\\0f\"", 32u,
      USE_READ | USE_OWN },
    { "adir", NULL, 0u, USE_OWN | USE_REFUSALS }
};

// RESPONSE_COUNT
//   constant: how many files RESPONSE_FILES holds.
static const size_t RESPONSE_COUNT =
    sizeof(RESPONSE_FILES) / sizeof(RESPONSE_FILES[0]);

// d_tests_arg_argv_read
//   struct: a response file read: its tag, the rules it is read by, and its
// name.
struct d_tests_arg_argv_read
{
    const char* tag;
    int         quoting;
    const char* file;
};

// EXPAND_ARGS_MAX
//   constant: the room for the arguments of one parse that names response
// files, which end at the first NULL.
enum
{
    EXPAND_ARGS_MAX = 5
};

// d_tests_arg_argv_expand
//   struct: a parse that names response files: its tag, the rules its
// files are read by, and its arguments.
struct d_tests_arg_argv_expand
{
    const char* tag;
    int         quoting;
    const char* args[EXPAND_ARGS_MAX];
};

// ENTRY_WORDS_MAX
//   constant: the room for the words of one parse given them otherwise
// than main's `argv`.
enum
{
    ENTRY_WORDS_MAX = 3
};

// ENTRY_ARGS / ENTRY_REVERSED / ENTRY_LINE
//   constant: how a parse of ENTRY_ROWS is given its words:
// d_arg_parser_parse_args, in order and borrowed, or last first and
// copied; or d_arg_parser_parse_line, the line starting with the
// program's name.
enum
{
    ENTRY_ARGS     = 0,
    ENTRY_REVERSED = 1,
    ENTRY_LINE     = 2
};

// d_tests_arg_argv_entry
//   struct: a parse that names response files, given its words otherwise
// than main's `argv`: its tag; how it is given them, an ENTRY_* value;
// the rules its files are read by; and its words -- the line, for
// ENTRY_LINE -- each a span that may hold NUL bytes, and how many.
struct d_tests_arg_argv_entry
{
    const char*        tag;
    int                entry;
    int                quoting;
    struct d_pack_text words[ENTRY_WORDS_MAX];
    size_t             count;
};

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

// WIDE_UNITS_MAX
//   constant: the room for the units of one row of wide text.
enum
{
    WIDE_UNITS_MAX = 4
};

// d_tests_arg_argv_wide
//   struct: wide text converted: its tag, and its units -- UTF-16's or code
// points, each a `wchar_t` as it would hold the value -- and how many.
struct d_tests_arg_argv_wide
{
    const char* tag;
    uint32_t    units[WIDE_UNITS_MAX];
    size_t      length;
};

// WIDE16_ROWS
//   constant: the UTF-16 units d_tests_arg_argv_wide_text converts where
// `wchar_t` has 16 bits.
static const struct d_tests_arg_argv_wide WIDE16_ROWS[] =
{
    { "U01", { 0u }, 0u },
    { "U02", { 0x61u }, 1u },
    { "U03", { 0x7Fu, 0x80u }, 2u },
    { "U04", { 0x7FFu, 0x800u }, 2u },
    { "U05", { 0xFFFFu }, 1u },
    { "U06", { 0xD83Du, 0xDE00u }, 2u },
    { "U07", { 0xD83Du }, 1u },
    { "U08", { 0xDE00u }, 1u },
    { "U09", { 0xDE00u, 0xD83Du }, 2u },
    { "U10", { 0xD83Du, 0x61u }, 2u },
    { "U11", { 0x61u, 0xDBFFu }, 2u },
    { "U12", { 0xDBFFu, 0xDFFFu }, 2u },
    { "U13", { 0xD800u, 0xD800u, 0xDC00u }, 3u },
    { "U14", { 0x436u, 0x20u, 0xD83Du, 0xDE00u }, 4u },
    { "U15", { 0xFFFDu }, 1u },
    { "U16", { 0xFEFFu, 0x61u }, 2u },
    { "U17", { 0x0u, 0x61u }, 2u }
};

// WIDE16_COUNT
//   constant: how many rows WIDE16_ROWS holds.
static const size_t WIDE16_COUNT = sizeof(WIDE16_ROWS) / sizeof(WIDE16_ROWS[0]);

// WIDE_TEXT_UTF16
//   constant: what the units of WIDE16_ROWS come to: Python's UTF-16 codec's
// text, each unpaired surrogate U+FFFD, as Windows converts one.
static const char WIDE_TEXT_UTF16[] =
    "U01 []: []\n"
    "U02 [61]: [a]\n"
    "U03 [7f 80]: [\\x7f\xc2\x80]\n"
    "U04 [7ff 800]: [\xdf\xbf\xe0\xa0\x80]\n"
    "U05 [ffff]: [\xef\xbf\xbf]\n"
    "U06 [d83d de00]: [\xf0\x9f\x98\x80]\n"
    "U07 [d83d]: [\xef\xbf\xbd]\n"
    "U08 [de00]: [\xef\xbf\xbd]\n"
    "U09 [de00 d83d]: [\xef\xbf\xbd\xef\xbf\xbd]\n"
    "U10 [d83d 61]: [\xef\xbf\xbd" "a]\n"
    "U11 [61 dbff]: [a\xef\xbf\xbd]\n"
    "U12 [dbff dfff]: [\xf4\x8f\xbf\xbf]\n"
    "U13 [d800 d800 dc00]: [\xef\xbf\xbd\xf0\x90\x80\x80]\n"
    "U14 [436 20 d83d de00]: [\xd0\xb6 \xf0\x9f\x98\x80]\n"
    "U15 [fffd]: [\xef\xbf\xbd]\n"
    "U16 [feff 61]: [\xef\xbb\xbf" "a]\n"
    "U17 [0 61]: [\\x00a]\n";

// WIDE32_ROWS
//   constant: the code points d_tests_arg_argv_wide_text converts where
// `wchar_t` has more.
static const struct d_tests_arg_argv_wide WIDE32_ROWS[] =
{
    { "U01", { 0u }, 0u },
    { "U02", { 0x61u }, 1u },
    { "U03", { 0x7Fu, 0x80u }, 2u },
    { "U04", { 0x7FFu, 0x800u }, 2u },
    { "U05", { 0xFFFFu, 0x10000u }, 2u },
    { "U06", { 0x1F600u }, 1u },
    { "U07", { 0x10FFFFu }, 1u },
    { "U08", { 0x110000u }, 1u },
    { "U09", { 0xD800u }, 1u },
    { "U10", { 0xDFFFu }, 1u },
    { "U11", { 0xD7FFu, 0xE000u }, 2u },
    { "U12", { 0x7FFFFFFFu }, 1u },
    { "U13", { 0xFFFFFFFFu }, 1u },
    { "U14", { 0x436u, 0x20u, 0x1F600u }, 3u },
    { "U15", { 0x0u, 0x61u }, 2u }
};

// WIDE32_COUNT
//   constant: how many rows WIDE32_ROWS holds.
static const size_t WIDE32_COUNT = sizeof(WIDE32_ROWS) / sizeof(WIDE32_ROWS[0]);

// WIDE_TEXT_UTF32
//   constant: what the code points of WIDE32_ROWS come to: Python's UTF-8 of
// each, a surrogate or a value past U+10FFFF -- a negative `wchar_t` among them
// -- U+FFFD.
static const char WIDE_TEXT_UTF32[] =
    "U01 []: []\n"
    "U02 [61]: [a]\n"
    "U03 [7f 80]: [\\x7f\xc2\x80]\n"
    "U04 [7ff 800]: [\xdf\xbf\xe0\xa0\x80]\n"
    "U05 [ffff 10000]: [\xef\xbf\xbf\xf0\x90\x80\x80]\n"
    "U06 [1f600]: [\xf0\x9f\x98\x80]\n"
    "U07 [10ffff]: [\xf4\x8f\xbf\xbf]\n"
    "U08 [110000]: [\xef\xbf\xbd]\n"
    "U09 [d800]: [\xef\xbf\xbd]\n"
    "U10 [dfff]: [\xef\xbf\xbd]\n"
    "U11 [d7ff e000]: [\xed\x9f\xbf\xee\x80\x80]\n"
    "U12 [7fffffff]: [\xef\xbf\xbd]\n"
    "U13 [ffffffff]: [\xef\xbf\xbd]\n"
    "U14 [436 20 1f600]: [\xd0\xb6 \xf0\x9f\x98\x80]\n"
    "U15 [0 61]: [\\x00a]\n";

// d_tests_arg_argv_utf8
//   struct: UTF-8 converted to wide text: its tag, and its bytes and how
// many.
struct d_tests_arg_argv_utf8
{
    const char* tag;
    const char* text;
    size_t      length;
};

// WIDE_BACK_ROWS
//   constant: the UTF-8 d_tests_arg_argv_wide_back converts.
static const struct d_tests_arg_argv_utf8 WIDE_BACK_ROWS[] =
{
    { "B01", "", 0u },
    { "B02", "a", 1u },
    { "B03", "\x7f\xc2\x80", 3u },
    { "B04", "\xdf\xbf\xe0\xa0\x80", 5u },
    { "B05", "\xef\xbf\xbf\xf0\x90\x80\x80", 7u },
    { "B06", "\xf0\x9f\x98\x80", 4u },
    { "B07", "\xf4\x8f\xbf\xbf", 4u },
    { "B08", "\xed\x9f\xbf\xee\x80\x80", 6u },
    { "B09", "\xef\xbf\xbd", 3u },
    { "B10", "\xef\xbb\xbf" "a", 4u },
    { "B11", "\x00" "a", 2u },
    { "B12", "\xd0\xb6 \xf0\x9f\x98\x80", 7u },
    { "B13", "\x80", 1u },
    { "B14", "\xbf", 1u },
    { "B15", "\xc0\xaf", 2u },
    { "B16", "\xc1\xbf", 2u },
    { "B17", "\xe0\x9f\xbf", 3u },
    { "B18", "\xf0\x8f\xbf\xbf", 4u },
    { "B19", "\xed\xa0\x80", 3u },
    { "B20", "\xed\xbf\xbf", 3u },
    { "B21", "\xf4\x90\x80\x80", 4u },
    { "B22", "\xf5\x80\x80\x80", 4u },
    { "B23", "\xfe", 1u },
    { "B24", "\xff", 1u },
    { "B25", "\xe2\x82", 2u },
    { "B26", "a\xf0\x9f\x98", 4u },
    { "B27", "\xc3(", 2u },
    { "B28", "\xe0\xa0\xc0", 3u },
    { "B29", "a\x80" "b", 3u }
};

// WIDE_BACK_COUNT
//   constant: how many rows WIDE_BACK_ROWS holds.
static const size_t WIDE_BACK_COUNT =
    sizeof(WIDE_BACK_ROWS) / sizeof(WIDE_BACK_ROWS[0]);

// WIDE_BACK_UTF16
//   constant: what the rows of WIDE_BACK_ROWS come to where `wchar_t` has 16
// bits: Python's strict UTF-8 codec's text as UTF-16 units, or `refused`.
static const char WIDE_BACK_UTF16[] =
    "B01 []: []\n"
    "B02 [61]: [61]\n"
    "B03 [7f c2 80]: [7f 80]\n"
    "B04 [df bf e0 a0 80]: [7ff 800]\n"
    "B05 [ef bf bf f0 90 80 80]: [ffff d800 dc00]\n"
    "B06 [f0 9f 98 80]: [d83d de00]\n"
    "B07 [f4 8f bf bf]: [dbff dfff]\n"
    "B08 [ed 9f bf ee 80 80]: [d7ff e000]\n"
    "B09 [ef bf bd]: [fffd]\n"
    "B10 [ef bb bf 61]: [feff 61]\n"
    "B11 [00 61]: [0 61]\n"
    "B12 [d0 b6 20 f0 9f 98 80]: [436 20 d83d de00]\n"
    "B13 [80]: refused\n"
    "B14 [bf]: refused\n"
    "B15 [c0 af]: refused\n"
    "B16 [c1 bf]: refused\n"
    "B17 [e0 9f bf]: refused\n"
    "B18 [f0 8f bf bf]: refused\n"
    "B19 [ed a0 80]: refused\n"
    "B20 [ed bf bf]: refused\n"
    "B21 [f4 90 80 80]: refused\n"
    "B22 [f5 80 80 80]: refused\n"
    "B23 [fe]: refused\n"
    "B24 [ff]: refused\n"
    "B25 [e2 82]: refused\n"
    "B26 [61 f0 9f 98]: refused\n"
    "B27 [c3 28]: refused\n"
    "B28 [e0 a0 c0]: refused\n"
    "B29 [61 80 62]: refused\n";

// WIDE_BACK_UTF32
//   constant: what the rows of WIDE_BACK_ROWS come to where `wchar_t` has more:
// Python's strict UTF-8 codec's code points, or `refused`.
static const char WIDE_BACK_UTF32[] =
    "B01 []: []\n"
    "B02 [61]: [61]\n"
    "B03 [7f c2 80]: [7f 80]\n"
    "B04 [df bf e0 a0 80]: [7ff 800]\n"
    "B05 [ef bf bf f0 90 80 80]: [ffff 10000]\n"
    "B06 [f0 9f 98 80]: [1f600]\n"
    "B07 [f4 8f bf bf]: [10ffff]\n"
    "B08 [ed 9f bf ee 80 80]: [d7ff e000]\n"
    "B09 [ef bf bd]: [fffd]\n"
    "B10 [ef bb bf 61]: [feff 61]\n"
    "B11 [00 61]: [0 61]\n"
    "B12 [d0 b6 20 f0 9f 98 80]: [436 20 1f600]\n"
    "B13 [80]: refused\n"
    "B14 [bf]: refused\n"
    "B15 [c0 af]: refused\n"
    "B16 [c1 bf]: refused\n"
    "B17 [e0 9f bf]: refused\n"
    "B18 [f0 8f bf bf]: refused\n"
    "B19 [ed a0 80]: refused\n"
    "B20 [ed bf bf]: refused\n"
    "B21 [f4 90 80 80]: refused\n"
    "B22 [f5 80 80 80]: refused\n"
    "B23 [fe]: refused\n"
    "B24 [ff]: refused\n"
    "B25 [e2 82]: refused\n"
    "B26 [61 f0 9f 98]: refused\n"
    "B27 [c3 28]: refused\n"
    "B28 [e0 a0 c0]: refused\n"
    "B29 [61 80 62]: refused\n";

// WIDE_PARSE
//   constant: what the parses of d_tests_arg_argv_wide_parse come to, as
// CLI11's wide parses give them.
static const char WIDE_PARSE[] =
    "V01 [--name \xc3\xa9t\xc3\xa9 x y \xf0\x9f\x98\x80]\n"
    "cmd app n=1 miss=[x y|\xf0\x9f\x98\x80] subs=[] order=[--name]\n"
    "opt name n=1 res=[\xc3\xa9t\xc3\xa9]\n"
    "V02 [prog --name \"\xc3\xa9 t\xc3\xa9\" x \xf0\x9f\x98\x80]\n"
    "cmd app n=1 miss=[x|\xf0\x9f\x98\x80] subs=[] order=[--name]\n"
    "opt name n=1 res=[\xc3\xa9 t\xc3\xa9]\n"
    "V03 [prog --name \"\\\\q\" x]\n"
    "ERR ParseError 111: unrecognized escape sequence \\\\q in \\\\q\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "opt name n=0 res=[]\n"
    "V04 [prog --name a\\x00b \"c\\\\0d\"]\n"
    "cmd app n=1 miss=[c\\x00d] subs=[] order=[--name]\n"
    "opt name n=1 res=[a\\x00b]\n";

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

//==============================================================================
// HELPERS
//==============================================================================

/**
 * @brief Prints what a check was about when it did not hold.
 *
 * @param[in] _held  whether it held.
 * @param[in] _what  what it was about.
 * @return `_held`.
 */
static bool
d_tests_arg_argv_held(
    bool        _held,
    const char* _what
)
{
    // a check that did not hold says what it was
    if (!_held)
    {
        printf("    did not hold: %s\n",
               _what);
    }

    return _held;
}

/**
 * @brief The name the transcripts give a set of rules.
 *
 * @param[in] _quoting  a d_arg_quoting value.
 * @return `posix`, `windows`, `cli11` or `gnu`; `?` for any other value.
 */
static const char*
d_tests_arg_argv_rules(
    int _quoting
)
{
    const char* name = "?";

    switch (_quoting)
    {
        case D_ARG_QUOTING_POSIX:
            name = "posix";
            break;

        case D_ARG_QUOTING_WINDOWS:
            name = "windows";
            break;

        case D_ARG_QUOTING_CLI11:
            name = "cli11";
            break;

        case D_ARG_QUOTING_GNU:
            name = "gnu";
            break;

        default:
            break;
    }

    return name;
}

/**
 * @brief A copy of text in storage of exactly its length -- none for no
 *        text -- so that a read past either end of it is AddressSanitizer's
 *        to see.
 *
 * @param[in]  _text  the text.
 * @param[out] _had   receives whether the storage was had; true for no text.
 * @return the copy, freed by the caller; NULL for no text, or when the
 *         storage could not be had.
 */
static char*
d_tests_arg_argv_exact(
    struct d_pack_text _text,
    bool*              _had
)
{
    char* const exact = (_text.length > 0u)
                        ? (char*)malloc(_text.length)
                        : NULL;

    *_had = ( (_text.length == 0u) ||
              (exact != NULL) );

    // the text's bytes, with nothing after them
    if (exact != NULL)
    {
        memcpy(exact,
               _text.data,
               _text.length);
    }

    return exact;
}

/**
 * @brief Appends the words a list holds from `_from` on: how many, then
 *        each in brackets, escaped.
 *
 * @param[in,out] _out    the transcript.
 * @param[in]     _list   the list.
 * @param[in]     _from   the first word written.
 */
static void
d_tests_arg_argv_words(
    struct d_arg_buffer*            _out,
    const struct d_arg_string_list* _list,
    size_t                          _from
)
{
    (void)d_arg_buffer_printf(_out,
                              "%lu",
                              (unsigned long)(_list->count - _from));

    // each word
    for (size_t index = _from; index < _list->count; ++index)
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       " [");
        d_tests_arg_escape(_out,
                           d_pack_text_from_cstr(_list->items[index]));
        (void)d_arg_buffer_append_char(_out,
                                       ']');
    }

    return;
}

/**
 * @brief Appends what a split came to, and a newline: its words, or `ERR
 *        <status>` and, when it is described, `: <description>`.
 *
 * @param[in,out] _out        the transcript.
 * @param[in]     _described  whether a failure is described: but by
 *                            POSIX's rules, whose shells describe none.
 * @param[in]     _status     what the split came to.
 * @param[in]     _words      the words it gave.
 * @param[in]     _error      the description of a failure.
 */
static void
d_tests_arg_argv_outcome(
    struct d_arg_buffer*            _out,
    bool                            _described,
    int                             _status,
    const struct d_arg_string_list* _words,
    const struct d_arg_buffer*      _error
)
{
    // the words, or the failure
    if (_status == D_ARG_OK)
    {
        d_tests_arg_argv_words(_out,
                               _words,
                               0u);
    }
    else
    {
        (void)d_arg_buffer_printf(_out,
                                  "ERR %s",
                                  d_arg_status_name(_status));

        // what describes it, where the transcript has it
        if (_described)
        {
            (void)d_arg_buffer_append_cstr(_out,
                                           ": ");
            d_tests_arg_escape(_out,
                               d_arg_buffer_text(_error));
        }
    }

    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

/**
 * @brief Splits a row's line, from storage of exactly its length, and
 *        appends its line: `<tag> <rules> <switches> [<text>]: ` and what it
 *        came to.
 *
 * @param[in,out] _out  the transcript.
 * @param[in]     _row  the row.
 * @return whether the storage was had and the words and description kept.
 */
static bool
d_tests_arg_argv_split_row(
    struct d_arg_buffer*                 _out,
    const struct d_tests_arg_argv_split* _row
)
{
    const struct d_pack_text text   = d_pack_text_from_span(_row->text,
                                                            _row->length);
    bool                     had    = false;
    char* const              exact  = d_tests_arg_argv_exact(text,
                                                             &had);
    struct d_arg_string_list words  = D_ARG_LIST_INIT;
    struct d_arg_buffer      error  = D_ARG_BUFFER_INIT;
    const int                status = d_arg_split_line(
                                          d_pack_text_from_span(exact,
                                                                text.length),
                                          _row->quoting,
                                          _row->flags,
                                          &words,
                                          &error);

    (void)d_arg_buffer_printf(_out,
                              "%s %s %lu [",
                              _row->tag,
                              d_tests_arg_argv_rules(_row->quoting),
                              (unsigned long)_row->flags);
    d_tests_arg_escape(_out,
                       text);
    (void)d_arg_buffer_append_cstr(_out,
                                   "]: ");
    d_tests_arg_argv_outcome(_out,
                             (_row->quoting != (int)D_ARG_QUOTING_POSIX),
                             status,
                             &words,
                             &error);

    const bool kept = (error.failed == 0);

    d_arg_string_list_release(&words);
    d_arg_buffer_release(&error);
    free(exact);

    return d_tests_arg_argv_held(( (had) &&
                                   (kept) ),
                                 _row->tag);
}

/**
 * @brief Splits each line of a table and compares what they came to with
 *        the transcript expected.
 *
 * @param[in] _name      what the transcript is, for a printed mismatch.
 * @param[in] _rows      the lines.
 * @param[in] _count     how many.
 * @param[in] _expected  the transcript expected.
 * @return whether every line was split and the transcript was as expected.
 */
static bool
d_tests_arg_argv_splits(
    const char*                          _name,
    const struct d_tests_arg_argv_split* _rows,
    size_t                               _count,
    const char*                          _expected
)
{
    struct d_arg_buffer written = D_ARG_BUFFER_INIT;
    bool                held    = true;

    // each line
    for (size_t index = 0u; index < _count; ++index)
    {
        held = d_tests_arg_argv_split_row(&written,
                                          &_rows[index]) && held;
    }

    const bool same = d_tests_arg_same(_name,
                                       d_arg_buffer_text(&written),
                                       _expected);
    const bool kept = (written.failed == 0);

    d_arg_buffer_release(&written);

    return ( (same) &&
             (held) &&
             (kept) );
}

/**
 * @brief Splits a quoted word back by its rules with the switches given,
 *        from storage of exactly its length, and appends a line saying what
 *        it split into when that is not the word alone.
 *
 * @param[in,out] _out     the transcript.
 * @param[in]     _row     the word's row.
 * @param[in]     _quoted  the word as quoted.
 * @param[in]     _flags   the D_ARG_SPLIT_* switches split with.
 * @return whether the word came back alone, the storage was had and the
 *         description kept.
 */
static bool
d_tests_arg_argv_splits_back(
    struct d_arg_buffer*                 _out,
    const struct d_tests_arg_argv_quote* _row,
    struct d_pack_text                   _quoted,
    uint32_t                             _flags
)
{
    bool                     had    = false;
    char* const              exact  = d_tests_arg_argv_exact(_quoted,
                                                             &had);
    struct d_arg_string_list words  = D_ARG_LIST_INIT;
    struct d_arg_buffer      error  = D_ARG_BUFFER_INIT;
    const int                status = d_arg_split_line(
                                          d_pack_text_from_span(exact,
                                                                _quoted.length),
                                          _row->quoting,
                                          _flags,
                                          &words,
                                          &error);
    const bool               back   =
        ( (status == D_ARG_OK)                               &&
          (words.count == 1u)                                &&
          (strlen(words.items[0]) == _row->length)           &&
          (memcmp(words.items[0],
                  _row->word,
                  _row->length) == 0) );

    // a word that did not come back alone says what it came to
    if (!back)
    {
        (void)d_arg_buffer_printf(_out,
                                  "%s splits back by %lu to ",
                                  _row->tag,
                                  (unsigned long)_flags);
        d_tests_arg_argv_outcome(_out,
                                 true,
                                 status,
                                 &words,
                                 &error);
    }

    const bool kept = (error.failed == 0);

    d_arg_string_list_release(&words);
    d_arg_buffer_release(&error);
    free(exact);

    return ( (back) &&
             (had)  &&
             (kept) );
}

/**
 * @brief Quotes a row's word, from storage of exactly its length, appends
 *        its line, `<tag> <rules> [<word>]: [<quoted>]`, and splits it back:
 *        by its rules alone, and by the C runtime's with line breaks too and
 *        CLI11's in Windows' style too.
 *
 * @param[in,out] _out  the transcript.
 * @param[in]     _row  the row.
 * @return whether the word was quoted and came back alone in each reading.
 */
static bool
d_tests_arg_argv_quote_row(
    struct d_arg_buffer*                 _out,
    const struct d_tests_arg_argv_quote* _row
)
{
    const struct d_pack_text word   = d_pack_text_from_span(_row->word,
                                                            _row->length);
    bool                     had    = false;
    char* const              exact  = d_tests_arg_argv_exact(word,
                                                             &had);
    struct d_arg_buffer      quoted = D_ARG_BUFFER_INIT;
    const int                status = d_arg_quote_word(
                                          d_pack_text_from_span(exact,
                                                                word.length),
                                          _row->quoting,
                                          &quoted);
    const uint32_t           second =
        (_row->quoting == (int)D_ARG_QUOTING_WINDOWS)
        ? D_ARG_SPLIT_LINE_BREAKS
        : ( (_row->quoting == (int)D_ARG_QUOTING_CLI11)
            ? D_ARG_SPLIT_WINDOWS_STYLE
            : 0u );

    (void)d_arg_buffer_printf(_out,
                              "%s %s [",
                              _row->tag,
                              d_tests_arg_argv_rules(_row->quoting));
    d_tests_arg_escape(_out,
                       word);
    (void)d_arg_buffer_append_cstr(_out,
                                   "]: [");
    d_tests_arg_escape(_out,
                       d_arg_buffer_text(&quoted));
    (void)d_arg_buffer_append_cstr(_out,
                                   "]\n");

    const bool back = ( (d_tests_arg_argv_splits_back(_out,
                                                      _row,
                                                      d_arg_buffer_text(
                                                          &quoted),
                                                      0u))     &&
                        (d_tests_arg_argv_splits_back(_out,
                                                      _row,
                                                      d_arg_buffer_text(
                                                          &quoted),
                                                      second)) );

    d_arg_buffer_release(&quoted);
    free(exact);

    return d_tests_arg_argv_held(( (status == D_ARG_OK) &&
                                   (had)                &&
                                   (back) ),
                                 _row->tag);
}

/**
 * @brief Quotes each word of a table, splitting each back, and compares
 *        what they came to with the transcript expected.
 *
 * @param[in] _name      what the transcript is, for a printed mismatch.
 * @param[in] _rows      the words.
 * @param[in] _count     how many.
 * @param[in] _expected  the transcript expected.
 * @return whether every word was quoted and came back, and the transcript
 *         was as expected.
 */
static bool
d_tests_arg_argv_quotes(
    const char*                          _name,
    const struct d_tests_arg_argv_quote* _rows,
    size_t                               _count,
    const char*                          _expected
)
{
    struct d_arg_buffer written = D_ARG_BUFFER_INIT;
    bool                held    = true;

    // each word
    for (size_t index = 0u; index < _count; ++index)
    {
        held = d_tests_arg_argv_quote_row(&written,
                                          &_rows[index]) && held;
    }

    const bool same = d_tests_arg_same(_name,
                                       d_arg_buffer_text(&written),
                                       _expected);
    const bool kept = (written.failed == 0);

    d_arg_buffer_release(&written);

    return ( (same) &&
             (held) &&
             (kept) );
}

/**
 * @brief Finds the program's name at the start of a row's line, from
 *        storage of exactly its length, and appends its line: `<tag>
 *        [<line>]: [<name>] [<rest>]`.
 *
 * @param[in,out] _out  the transcript.
 * @param[in]     _row  the row.
 * @return whether the name was found, the rest a span of the line, and the
 *         storage had.
 */
static bool
d_tests_arg_argv_program_row(
    struct d_arg_buffer*                   _out,
    const struct d_tests_arg_argv_program* _row
)
{
    const struct d_pack_text line   = d_pack_text_from_cstr(_row->line);
    bool                     had    = false;
    char* const              exact  = d_tests_arg_argv_exact(line,
                                                             &had);
    struct d_arg_buffer      name   = D_ARG_BUFFER_INIT;
    struct d_pack_text       rest   = { NULL, 0u };
    const int                status = d_arg_split_program_name(
                                          d_pack_text_from_span(exact,
                                                                line.length),
                                          &name,
                                          &rest);
    const bool               within =
        ( (rest.length == 0u)                            ||
          ( (rest.data >= exact)                     &&
            ((rest.data + rest.length) <= (exact + line.length)) ) );

    (void)d_arg_buffer_printf(_out,
                              "%s [",
                              _row->tag);
    d_tests_arg_escape(_out,
                       line);
    (void)d_arg_buffer_append_cstr(_out,
                                   "]: [");
    d_tests_arg_escape(_out,
                       d_arg_buffer_text(&name));
    (void)d_arg_buffer_append_cstr(_out,
                                   "] [");
    d_tests_arg_escape(_out,
                       rest);
    (void)d_arg_buffer_append_cstr(_out,
                                   "]\n");

    d_arg_buffer_release(&name);
    free(exact);

    return d_tests_arg_argv_held(( (status == D_ARG_OK) &&
                                   (within)             &&
                                   (had) ),
                                 _row->tag);
}

/**
 * @brief Makes the folders and files of PROGRAM_PLACES in a scratch
 *        directory.
 *
 * @param[in,out] _scratch  the scratch directory, entered.
 * @return whether every one was made.
 */
static bool
d_tests_arg_argv_places(
    struct d_tests_arg_files* _scratch
)
{
    bool made = true;

    // each place: a folder, or a file holding a byte
    for (size_t index = 0u; index < PLACE_COUNT; ++index)
    {
        made = ( (PROGRAM_PLACES[index].folder)
                 ? d_tests_arg_files_folder(_scratch,
                                            PROGRAM_PLACES[index].name)
                 : d_tests_arg_files_file(_scratch,
                                          PROGRAM_PLACES[index].name,
                                          "#") ) && made;
    }

    return made;
}

/**
 * @brief Finds the program's name in each line of a table, in a scratch
 *        directory holding PROGRAM_PLACES, and compares what they came to
 *        with the transcript expected.
 *
 * @param[in] _name      what the transcript is, for a printed mismatch.
 * @param[in] _rows      the lines.
 * @param[in] _count     how many.
 * @param[in] _expected  the transcript expected.
 * @return whether the places were made, every name found, and the
 *         transcript was as expected.
 */
static bool
d_tests_arg_argv_programs(
    const char*                            _name,
    const struct d_tests_arg_argv_program* _rows,
    size_t                                 _count,
    const char*                            _expected
)
{
    struct d_tests_arg_files scratch = { 0 };
    struct d_arg_buffer      written = D_ARG_BUFFER_INIT;
    const bool               entered = d_tests_arg_files_enter(&scratch);
    bool                     held    = ( (entered) &&
                                         (d_tests_arg_argv_places(&scratch)) );

    // each line, among the places made
    for (size_t index = 0u;
         (entered) && (index < _count);
         ++index)
    {
        held = d_tests_arg_argv_program_row(&written,
                                            &_rows[index]) && held;
    }

    d_tests_arg_files_leave(&scratch);

    const bool same = d_tests_arg_same(_name,
                                       d_arg_buffer_text(&written),
                                       _expected);
    const bool kept = (written.failed == 0);

    d_arg_buffer_release(&written);

    return ( (same) &&
             (held) &&
             (kept) );
}

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

/**
 * @brief Makes the response files of RESPONSE_FILES a section uses in a
 *        scratch directory.
 *
 * @param[in,out] _scratch  the scratch directory, entered.
 * @param[in]     _use      the section's USE_* bit.
 * @return whether every one was made.
 */
static bool
d_tests_arg_argv_responses(
    struct d_tests_arg_files* _scratch,
    unsigned int              _use
)
{
    bool made = true;

    // each file, or folder, the section uses
    for (size_t index = 0u; index < RESPONSE_COUNT; ++index)
    {
        const struct d_tests_arg_argv_response* const file =
            &RESPONSE_FILES[index];

        // a file, or a folder, made; another section's left alone
        if ((file->uses & _use) != 0u)
        {
            made = ( (file->data != NULL)
                     ? d_tests_arg_files_write(_scratch,
                                               file->name,
                                               file->data,
                                               file->size)
                     : d_tests_arg_files_folder(_scratch,
                                                file->name) ) && made;
        }
    }

    return made;
}

/**
 * @brief Reads a row's file by its rules and appends its line: `<tag>
 *        <rules> <file>: ` and what it came to.
 *
 * @param[in,out] _out  the transcript.
 * @param[in]     _row  the row.
 * @return whether the words and description were kept.
 */
static bool
d_tests_arg_argv_read_row(
    struct d_arg_buffer*                _out,
    const struct d_tests_arg_argv_read* _row
)
{
    struct d_arg_string_list words  = D_ARG_LIST_INIT;
    struct d_arg_buffer      error  = D_ARG_BUFFER_INIT;
    const int                status = d_arg_read_response_file(_row->file,
                                                               _row->quoting,
                                                               &words,
                                                               &error);

    (void)d_arg_buffer_printf(_out,
                              "%s %s %s: ",
                              _row->tag,
                              d_tests_arg_argv_rules(_row->quoting),
                              _row->file);
    d_tests_arg_argv_outcome(_out,
                             (_row->quoting != (int)D_ARG_QUOTING_POSIX),
                             status,
                             &words,
                             &error);

    const bool kept = (error.failed == 0);

    d_arg_string_list_release(&words);
    d_arg_buffer_release(&error);

    return d_tests_arg_argv_held(kept,
                                 _row->tag);
}

/**
 * @brief Reads each file of a table, in a scratch directory holding
 *        RESPONSE_FILES, and compares what they came to with the transcript
 *        expected.
 *
 * @param[in] _name      what the transcript is, for a printed mismatch.
 * @param[in] _rows      the files.
 * @param[in] _count     how many.
 * @param[in] _expected  the transcript expected.
 * @return whether the files were made and read, and the transcript was as
 *         expected.
 */
static bool
d_tests_arg_argv_reads(
    const char*                         _name,
    const struct d_tests_arg_argv_read* _rows,
    size_t                              _count,
    const char*                         _expected
)
{
    struct d_tests_arg_files scratch = { 0 };
    struct d_arg_buffer      written = D_ARG_BUFFER_INIT;
    const bool               entered = d_tests_arg_files_enter(&scratch);
    bool                     held    =
        ( (entered) &&
          (d_tests_arg_argv_responses(&scratch,
                                      USE_READ)) );

    // each file, among the files made
    for (size_t index = 0u;
         (entered) && (index < _count);
         ++index)
    {
        held = d_tests_arg_argv_read_row(&written,
                                         &_rows[index]) && held;
    }

    d_tests_arg_files_leave(&scratch);

    const bool same = d_tests_arg_same(_name,
                                       d_arg_buffer_text(&written),
                                       _expected);
    const bool kept = (written.failed == 0);

    d_arg_buffer_release(&written);

    return ( (same) &&
             (held) &&
             (kept) );
}

/**
 * @brief Appends a row's line, `<tag> <rules> [<arguments>]`, the arguments
 *        escaped and parted by spaces.
 *
 * @param[in,out] _out    the transcript.
 * @param[in]     _row    the row.
 * @param[in]     _count  how many arguments it has.
 */
static void
d_tests_arg_argv_expand_line(
    struct d_arg_buffer*                  _out,
    const struct d_tests_arg_argv_expand* _row,
    size_t                                _count
)
{
    (void)d_arg_buffer_printf(_out,
                              "%s %s [",
                              _row->tag,
                              d_tests_arg_argv_rules(_row->quoting));

    // each argument, after a space when one comes before it
    for (size_t index = 0u; index < _count; ++index)
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       (index > 0u) ? " " : "");
        d_tests_arg_escape(_out,
                           d_pack_text_from_cstr(_row->args[index]));
    }

    (void)d_arg_buffer_append_cstr(_out,
                                   "]\n");

    return;
}

/**
 * @brief Parses a row's arguments as main's, after the program's name
 *        "prog", built in storage the run keeps -- the way a program's
 *        response files reach the parser.
 *
 * @note The arguments are parted by spaces and built as d_tests_arg_argv
 *       builds them, so none holds a space or is empty (argv_rows.py sees
 *       to it).
 *
 * @param[in,out] _run    the row's run.
 * @param[in]     _row    the row.
 * @param[in]     _count  how many arguments it has.
 * @return the parse's status; D_ARG_ERROR_BAD_ARGUMENT, the run failed, for
 *         arguments that could not be built.
 */
static int
d_tests_arg_argv_expand_parse(
    struct d_tests_arg_run*               _run,
    const struct d_tests_arg_argv_expand* _row,
    size_t                                _count
)
{
    struct d_arg_buffer words = D_ARG_BUFFER_INIT;
    int                 argc  = 0;

    // the arguments, parted by spaces
    for (size_t index = 0u; index < _count; ++index)
    {
        (void)d_arg_buffer_append_cstr(&words,
                                       (index > 0u) ? " " : "");
        (void)d_arg_buffer_append_cstr(&words,
                                       _row->args[index]);
    }

    char* const* const argv  = d_tests_arg_argv(_run,
                                                "prog",
                                                (words.data != NULL)
                                                ? words.data
                                                : "",
                                                &argc);
    const bool         built = ( (argv != NULL)       &&
                                 (words.failed == 0) );

    d_arg_buffer_release(&words);

    // arguments that could not be built fail the run
    if (!built)
    {
        _run->failed = true;

        return D_ARG_ERROR_BAD_ARGUMENT;
    }

    return d_arg_parser_parse(&_run->parser,
                              argc,
                              argv);
}

/**
 * @brief Parses a row's arguments as main's, with response files read by
 *        its rules, on a run of its own taking any argument as an extra, as
 *        the reference's CLI11 App does, and appends its line and the
 *        parse's report.
 *
 * @param[in,out] _out  the transcript.
 * @param[in]     _row  the row.
 * @return whether the run was made and its report kept.
 */
static bool
d_tests_arg_argv_expand_row(
    struct d_arg_buffer*                  _out,
    const struct d_tests_arg_argv_expand* _row
)
{
    struct d_tests_arg_run      run   = { 0 };
    struct d_arg_command* const app   = d_tests_arg_begin(&run);
    size_t                      count = 0u;

    // the arguments, to the first NULL
    while ( (count < (size_t)EXPAND_ARGS_MAX) &&
            (_row->args[count] != NULL) )
    {
        count++;
    }

    d_tests_arg_argv_expand_line(_out,
                                 _row,
                                 count);
    d_arg_command_set_allow_extras(app,
                                   (int)D_ARG_EXTRAS_CAPTURE);
    d_arg_parser_set_response_files(&run.parser,
                                    true,
                                    _row->quoting);
    d_tests_arg_record(&run,
                       d_tests_arg_argv_expand_parse(&run,
                                                     _row,
                                                     count));
    (void)d_arg_buffer_append_text(_out,
                                   d_arg_buffer_text(&run.transcript));

    const bool held = ( (app != NULL)                &&
                        (!run.failed)                &&
                        (run.transcript.failed == 0) );

    d_tests_arg_end(&run);

    return d_tests_arg_argv_held(held,
                                 _row->tag);
}

/**
 * @brief Parses the arguments of each row of a table, in a scratch directory
 *        holding the files of RESPONSE_FILES a section uses, and compares
 *        what they came to with the transcript expected.
 *
 * @param[in] _name      what the transcript is, for a printed mismatch.
 * @param[in] _rows      the parses.
 * @param[in] _count     how many.
 * @param[in] _expected  the transcript expected.
 * @param[in] _use       the section's USE_* bit.
 * @return whether the files were made, every parse ran, and the transcript
 *         was as expected.
 */
static bool
d_tests_arg_argv_expand_rows(
    const char*                           _name,
    const struct d_tests_arg_argv_expand* _rows,
    size_t                                _count,
    const char*                           _expected,
    unsigned int                          _use
)
{
    struct d_tests_arg_files scratch = { 0 };
    struct d_arg_buffer      written = D_ARG_BUFFER_INIT;
    const bool               entered = d_tests_arg_files_enter(&scratch);
    bool                     held    =
        ( (entered) &&
          (d_tests_arg_argv_responses(&scratch,
                                      _use)) );

    // each parse, among the files made
    for (size_t index = 0u;
         (entered) && (index < _count);
         ++index)
    {
        held = d_tests_arg_argv_expand_row(&written,
                                           &_rows[index]) && held;
    }

    d_tests_arg_files_leave(&scratch);

    const bool same = d_tests_arg_same(_name,
                                       d_arg_buffer_text(&written),
                                       _expected);
    const bool kept = (written.failed == 0);

    d_arg_buffer_release(&written);

    return ( (same) &&
             (held) &&
             (kept) );
}

/**
 * @brief Parses the arguments of each row of GCC's or Windows' table, among
 *        the files those sections use, as d_tests_arg_argv_expand_rows does.
 *
 * @param[in] _name      what the transcript is, for a printed mismatch.
 * @param[in] _rows      the parses.
 * @param[in] _count     how many.
 * @param[in] _expected  the transcript expected.
 * @return whether the files were made, every parse ran, and the transcript
 *         was as expected.
 */
static bool
d_tests_arg_argv_expands(
    const char*                           _name,
    const struct d_tests_arg_argv_expand* _rows,
    size_t                                _count,
    const char*                           _expected
)
{
    return d_tests_arg_argv_expand_rows(_name,
                                        _rows,
                                        _count,
                                        _expected,
                                        USE_EXPAND);
}

/**
 * @brief Parses the arguments of each row of the module's own table, among
 *        the files that section uses, as d_tests_arg_argv_expand_rows does.
 *
 * @param[in] _name      what the transcript is, for a printed mismatch.
 * @param[in] _rows      the parses.
 * @param[in] _count     how many.
 * @param[in] _expected  the transcript expected.
 * @return whether the files were made, every parse ran, and the transcript
 *         was as expected.
 */
static bool
d_tests_arg_argv_own_expands(
    const char*                           _name,
    const struct d_tests_arg_argv_expand* _rows,
    size_t                                _count,
    const char*                           _expected
)
{
    return d_tests_arg_argv_expand_rows(_name,
                                        _rows,
                                        _count,
                                        _expected,
                                        USE_OWN);
}

/**
 * @brief Appends the line of a parse given its words otherwise than main's,
 *        `<tag> <how> <rules> [<words>]`: how -- "args", "reversed",
 *        "line" -- then the words escaped, parted by spaces.
 *
 * @param[in,out] _out  the transcript.
 * @param[in]     _row  the parse.
 */
static void
d_tests_arg_argv_entry_line(
    struct d_arg_buffer*                 _out,
    const struct d_tests_arg_argv_entry* _row
)
{
    // HOW
    //   constant: each ENTRY_* value's name, in its order.
    static const char* const HOW[] = { "args",
                                       "reversed",
                                       "line" };

    (void)d_arg_buffer_printf(_out,
                              "%s %s %s [",
                              _row->tag,
                              HOW[_row->entry],
                              d_tests_arg_argv_rules(_row->quoting));

    // each word, after a space when one comes before it
    for (size_t index = 0u; index < _row->count; ++index)
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       (index > 0u) ? " " : "");
        d_tests_arg_escape(_out,
                           _row->words[index]);
    }

    (void)d_arg_buffer_append_cstr(_out,
                                   "]\n");

    return;
}

/**
 * @brief Parses a row's words as it is given them: a line with the
 *        program's name, or the arguments of a vector, in order and
 *        borrowed or last first and copied.
 *
 * @param[in,out] _parser  the run's parser.
 * @param[in]     _row     the parse.
 * @return the parse's status.
 */
static int
d_tests_arg_argv_entry_parse(
    struct d_arg_parser*                 _parser,
    const struct d_tests_arg_argv_entry* _row
)
{
    // a line, starting with the program's name
    if (_row->entry == ENTRY_LINE)
    {
        return d_arg_parser_parse_line(_parser,
                                       _row->words[0],
                                       true);
    }

    return d_arg_parser_parse_args(_parser,
                                   _row->words,
                                   _row->count,
                                   (_row->entry == ENTRY_REVERSED)
                                   ? (D_ARG_ARGS_REVERSED | D_ARG_ARGS_COPY)
                                   : 0u);
}

/**
 * @brief Parses a row's words with response files read by its rules, on a
 *        run of its own taking any argument as an extra, as the reference's
 *        CLI11 App does, and appends its line and the parse's report.
 *
 * @param[in,out] _out  the transcript.
 * @param[in]     _row  the parse.
 * @return whether the run was made and its report kept.
 */
static bool
d_tests_arg_argv_entry_row(
    struct d_arg_buffer*                 _out,
    const struct d_tests_arg_argv_entry* _row
)
{
    struct d_tests_arg_run      run = { 0 };
    struct d_arg_command* const app = d_tests_arg_begin(&run);

    d_tests_arg_argv_entry_line(_out,
                                _row);
    d_arg_command_set_allow_extras(app,
                                   (int)D_ARG_EXTRAS_CAPTURE);
    d_arg_parser_set_response_files(&run.parser,
                                    true,
                                    _row->quoting);
    d_tests_arg_record(&run,
                       d_tests_arg_argv_entry_parse(&run.parser,
                                                    _row));
    (void)d_arg_buffer_append_text(_out,
                                   d_arg_buffer_text(&run.transcript));

    const bool held = ( (app != NULL)                &&
                        (!run.failed)                &&
                        (run.transcript.failed == 0) );

    d_tests_arg_end(&run);

    return d_tests_arg_argv_held(held,
                                 _row->tag);
}

/**
 * @brief Parses each row of a table given its words otherwise than main's,
 *        in a scratch directory holding the files GCC's and Windows'
 *        expanding sections use, and compares what they came to with the
 *        transcript expected.
 *
 * @param[in] _name      what the transcript is, for a printed mismatch.
 * @param[in] _rows      the parses.
 * @param[in] _count     how many.
 * @param[in] _expected  the transcript expected.
 * @return whether the files were made, every parse ran, and the transcript
 *         was as expected.
 */
static bool
d_tests_arg_argv_entries(
    const char*                          _name,
    const struct d_tests_arg_argv_entry* _rows,
    size_t                               _count,
    const char*                          _expected
)
{
    struct d_tests_arg_files scratch = { 0 };
    struct d_arg_buffer      written = D_ARG_BUFFER_INIT;
    const bool               entered = d_tests_arg_files_enter(&scratch);
    bool                     held    =
        ( (entered) &&
          (d_tests_arg_argv_responses(&scratch,
                                      USE_EXPAND)) );

    // each parse, among the files made
    for (size_t index = 0u;
         (entered) && (index < _count);
         ++index)
    {
        held = d_tests_arg_argv_entry_row(&written,
                                          &_rows[index]) && held;
    }

    d_tests_arg_files_leave(&scratch);

    const bool same = d_tests_arg_same(_name,
                                       d_arg_buffer_text(&written),
                                       _expected);
    const bool kept = (written.failed == 0);

    d_arg_buffer_release(&written);

    return ( (same) &&
             (held) &&
             (kept) );
}

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

//==============================================================================
// 1.  SPLITTING: POSIX
//==============================================================================

// SPLIT_POSIX_ROWS
//   constant: the lines of d_tests_arg_argv_split_posix.
static const struct d_tests_arg_argv_split SPLIT_POSIX_ROWS[] =
{
    { "P01", D_ARG_QUOTING_POSIX, 0u, "", 0u },
    { "P02", D_ARG_QUOTING_POSIX, 0u, " ", 1u },
    { "P03", D_ARG_QUOTING_POSIX, 0u, "a", 1u },
    { "P04", D_ARG_QUOTING_POSIX, 0u, " a  b ", 6u },
    { "P05", D_ARG_QUOTING_POSIX, 0u, "'a b'", 5u },
    { "P06", D_ARG_QUOTING_POSIX, 0u, "\"a b\"", 5u },
    { "P07", D_ARG_QUOTING_POSIX, 0u, "a'b c'd", 7u },
    { "P08", D_ARG_QUOTING_POSIX, 0u, "a\"b c\"d", 7u },
    { "P09", D_ARG_QUOTING_POSIX, 0u, "''", 2u },
    { "P10", D_ARG_QUOTING_POSIX, 0u, "\"\"", 2u },
    { "P11", D_ARG_QUOTING_POSIX, 0u, "'' ''", 5u },
    { "P12", D_ARG_QUOTING_POSIX, 0u, "a\\ b", 4u },
    { "P13", D_ARG_QUOTING_POSIX, 0u, "\\\\", 2u },
    { "P14", D_ARG_QUOTING_POSIX, 0u, "\\\"", 2u },
    { "P15", D_ARG_QUOTING_POSIX, 0u, "\\'", 2u },
    { "P16", D_ARG_QUOTING_POSIX, 0u, "\"\\\"\"", 4u },
    { "P17", D_ARG_QUOTING_POSIX, 0u, "\"\\\\\"", 4u },
    { "P18", D_ARG_QUOTING_POSIX, 0u, "\"\\$\"", 4u },
    { "P19", D_ARG_QUOTING_POSIX, 0u, "\"\\`\"", 4u },
    { "P20", D_ARG_QUOTING_POSIX, 0u, "\"\\a\"", 4u },
    { "P21", D_ARG_QUOTING_POSIX, 0u, "\"a\\\nb\"", 6u },
    { "P22", D_ARG_QUOTING_POSIX, 0u, "a\\\nb", 4u },
    { "P23", D_ARG_QUOTING_POSIX, 0u, "\"a\nb\"", 5u },
    { "P24", D_ARG_QUOTING_POSIX, 0u, "'a\nb'", 5u },
    { "P25", D_ARG_QUOTING_POSIX, 0u, "'a\\b'", 5u },
    { "P26", D_ARG_QUOTING_POSIX, 0u, "\"\\'\"", 4u },
    { "P27", D_ARG_QUOTING_POSIX, 0u, "x\\\n", 3u },
    { "P28", D_ARG_QUOTING_POSIX, 0u, "'", 1u },
    { "P29", D_ARG_QUOTING_POSIX, 0u, "\"", 1u },
    { "P30", D_ARG_QUOTING_POSIX, 0u, "a\\", 2u },
    { "P31", D_ARG_QUOTING_POSIX, 0u, "\"a", 2u },
    { "P32", D_ARG_QUOTING_POSIX, 0u, "'a", 2u },
    { "P33", D_ARG_QUOTING_POSIX, 0u, "a\tb", 3u },
    { "P34", D_ARG_QUOTING_POSIX, 0u, "a\x0b" "b", 3u },
    { "P35", D_ARG_QUOTING_POSIX, 0u, "a\x0c" "b", 3u },
    { "P36", D_ARG_QUOTING_POSIX, 0u, "a\rb", 3u },
    { "P37", D_ARG_QUOTING_POSIX, 0u, "\\$x", 3u },
    { "P38", D_ARG_QUOTING_POSIX, 0u, "\\`x", 3u },
    { "P39", D_ARG_QUOTING_POSIX, 0u, "\"$\"x", 4u },
    { "P40", D_ARG_QUOTING_POSIX, 0u, "'$'", 3u },
    { "P41", D_ARG_QUOTING_POSIX, 0u, "a#b", 3u },
    { "P42", D_ARG_QUOTING_POSIX, 0u, "a\nb", 3u },
    { "P43", D_ARG_QUOTING_POSIX, 0u, "a\n\nb", 4u },
    { "P44", D_ARG_QUOTING_POSIX, 0u, "\n", 1u },
    { "P45", D_ARG_QUOTING_POSIX, 0u, "a \n b", 5u },
    { "P46", D_ARG_QUOTING_POSIX, 0u, "a\n\"b c\"\nd", 9u },
    { "P47", D_ARG_QUOTING_POSIX, 0u, "'a\nb", 4u },
    { "P48", D_ARG_QUOTING_POSIX, 0u, "a\\ \nb", 5u },
    { "P49", D_ARG_QUOTING_POSIX, 0u, "\"\\\"", 3u }
};

// SPLIT_POSIX_COUNT
//   constant: how many cases SPLIT_POSIX_ROWS holds.
static const size_t SPLIT_POSIX_COUNT =
    sizeof(SPLIT_POSIX_ROWS) / sizeof(SPLIT_POSIX_ROWS[0]);

/*
d_tests_arg_argv_split_posix
  A POSIX shell's words, without expansions, comments or operators
(d_arg_split_line, D_ARG_QUOTING_POSIX).
  Tests the following:
  - blanks -- spaces, tabs and line feeds -- separating, other whitespace
    ordinary
  - single quotes, double quotes and the escapes they honour, a backslash
    outside them
  - a backslash before a line break, joining the lines, and one last, kept
  - quotes never closed, refused
*/
bool
d_tests_arg_argv_split_posix(void)
{
    return d_tests_arg_argv_splits("SPLIT_POSIX",
                                   SPLIT_POSIX_ROWS,
                                   SPLIT_POSIX_COUNT,
                                   SPLIT_POSIX);
}

//==============================================================================
// 2.  SPLITTING: WINDOWS
//==============================================================================

// SPLIT_WINDOWS_ROWS
//   constant: the lines of d_tests_arg_argv_split_windows.
static const struct d_tests_arg_argv_split SPLIT_WINDOWS_ROWS[] =
{
    { "W01", D_ARG_QUOTING_WINDOWS, 0u, "\"a b c\" d e", 11u },
    { "W02", D_ARG_QUOTING_WINDOWS, 0u, "\"ab\\\"c\" \"\\\\\" d", 14u },
    { "W03", D_ARG_QUOTING_WINDOWS, 0u, "a\\\\\\b d\"e f\"g h", 15u },
    { "W04", D_ARG_QUOTING_WINDOWS, 0u, "a\\\\\\\"b c d", 10u },
    { "W05", D_ARG_QUOTING_WINDOWS, 0u, "a\\\\\\\\\"b c\" d e", 14u },
    { "W06", D_ARG_QUOTING_WINDOWS, 0u, "a\"b\"\" c d", 9u },
    { "W07", D_ARG_QUOTING_WINDOWS, 0u, "", 0u },
    { "W08", D_ARG_QUOTING_WINDOWS, 0u, " ", 1u },
    { "W09", D_ARG_QUOTING_WINDOWS, 0u, "\"", 1u },
    { "W10", D_ARG_QUOTING_WINDOWS, 0u, "\"\"", 2u },
    { "W11", D_ARG_QUOTING_WINDOWS, 0u, "\"\" \"\"", 5u },
    { "W12", D_ARG_QUOTING_WINDOWS, 0u, "a\\", 2u },
    { "W13", D_ARG_QUOTING_WINDOWS, 0u, "\\\"", 2u },
    { "W14", D_ARG_QUOTING_WINDOWS, 0u, "\\\\\"", 3u },
    { "W15", D_ARG_QUOTING_WINDOWS, 0u, "\"a\\\\\"", 5u },
    { "W16", D_ARG_QUOTING_WINDOWS, 0u, "\"a\\\\\\\"b\"", 8u },
    { "W17", D_ARG_QUOTING_WINDOWS, 0u, "a\"\"b", 4u },
    { "W18", D_ARG_QUOTING_WINDOWS, 0u, "\"a\"\"b\"", 6u },
    { "W19", D_ARG_QUOTING_WINDOWS, 0u, "\"a\"\"\"b\"", 7u },
    { "W20", D_ARG_QUOTING_WINDOWS, 0u, "a\nb", 3u },
    { "W21", D_ARG_QUOTING_WINDOWS, 0u, "\"a\nb\"", 5u },
    { "W22", D_ARG_QUOTING_WINDOWS, 0u, "a\rb c", 5u },
    { "W23", D_ARG_QUOTING_WINDOWS, 0u, "\t a \t", 5u },
    { "W24", D_ARG_QUOTING_WINDOWS, 0u, "\\\\\\\\", 4u },
    { "W25", D_ARG_QUOTING_WINDOWS, 0u, "a\\b\\\\c", 6u },
    { "W26", D_ARG_QUOTING_WINDOWS, 0u, "\"\\\\\\\\\"", 6u },
    { "W27", D_ARG_QUOTING_WINDOWS, 0u, "\"\"\"\"", 4u },
    { "W28", D_ARG_QUOTING_WINDOWS, 0u, "\"\"\"a\"\"\"", 7u },
    { "W29", D_ARG_QUOTING_WINDOWS, 0u, "a \"\" b", 6u },
    { "W30", D_ARG_QUOTING_WINDOWS, 0u, "a\x0b" "b\x0c" "c", 5u },
    { "W31", D_ARG_QUOTING_WINDOWS, 0u, "\r\n a \r\n", 7u }
};

// SPLIT_WINDOWS_COUNT
//   constant: how many cases SPLIT_WINDOWS_ROWS holds.
static const size_t SPLIT_WINDOWS_COUNT =
    sizeof(SPLIT_WINDOWS_ROWS) / sizeof(SPLIT_WINDOWS_ROWS[0]);

/*
d_tests_arg_argv_split_windows
  The C runtime's arguments (D_ARG_QUOTING_WINDOWS): the Universal C Runtime's
parse_command_line.
  Tests the following:
  - runs of backslashes before a quote and elsewhere
  - quotes opening and closing, `""` within quotes, an unclosed quote running to
    the end
  - spaces and tabs separating, line breaks ordinary
*/
bool
d_tests_arg_argv_split_windows(void)
{
    return d_tests_arg_argv_splits("SPLIT_WINDOWS",
                                   SPLIT_WINDOWS_ROWS,
                                   SPLIT_WINDOWS_COUNT,
                                   SPLIT_WINDOWS);
}

//==============================================================================
// 3.  SPLITTING: WINDOWS, LINE BREAKS
//==============================================================================

// SPLIT_WINDOWS_BREAKS_ROWS
//   constant: the lines of d_tests_arg_argv_split_windows_breaks.
static const struct d_tests_arg_argv_split SPLIT_WINDOWS_BREAKS_ROWS[] =
{
    { "W01", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS,
      "\"a b c\" d e", 11u },
    { "W02", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS,
      "\"ab\\\"c\" \"\\\\\" d", 14u },
    { "W03", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS,
      "a\\\\\\b d\"e f\"g h", 15u },
    { "W04", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS,
      "a\\\\\\\"b c d", 10u },
    { "W05", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS,
      "a\\\\\\\\\"b c\" d e", 14u },
    { "W06", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS,
      "a\"b\"\" c d", 9u },
    { "W07", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS, "", 0u },
    { "W08", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS, " ", 1u },
    { "W09", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS, "\"", 1u },
    { "W10", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS, "\"\"", 2u },
    { "W11", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS, "\"\" \"\"", 5u },
    { "W12", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS, "a\\", 2u },
    { "W13", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS, "\\\"", 2u },
    { "W14", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS, "\\\\\"", 3u },
    { "W15", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS, "\"a\\\\\"", 5u },
    { "W16", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS,
      "\"a\\\\\\\"b\"", 8u },
    { "W17", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS, "a\"\"b", 4u },
    { "W18", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS, "\"a\"\"b\"", 6u },
    { "W19", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS,
      "\"a\"\"\"b\"", 7u },
    { "W20", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS, "a\nb", 3u },
    { "W21", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS, "\"a\nb\"", 5u },
    { "W22", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS, "a\rb c", 5u },
    { "W23", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS, "\t a \t", 5u },
    { "W24", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS, "\\\\\\\\", 4u },
    { "W25", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS, "a\\b\\\\c", 6u },
    { "W26", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS,
      "\"\\\\\\\\\"", 6u },
    { "W27", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS, "\"\"\"\"", 4u },
    { "W28", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS,
      "\"\"\"a\"\"\"", 7u },
    { "W29", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS, "a \"\" b", 6u },
    { "W30", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS,
      "a\x0b" "b\x0c" "c", 5u },
    { "W31", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS,
      "\r\n a \r\n", 7u }
};

// SPLIT_WINDOWS_BREAKS_COUNT
//   constant: how many cases SPLIT_WINDOWS_BREAKS_ROWS holds.
static const size_t SPLIT_WINDOWS_BREAKS_COUNT =
    sizeof(SPLIT_WINDOWS_BREAKS_ROWS) / sizeof(SPLIT_WINDOWS_BREAKS_ROWS[0]);

/*
d_tests_arg_argv_split_windows_breaks
  The C runtime's arguments with line breaks as separators
(D_ARG_SPLIT_LINE_BREAKS), as LLVM reads a response file too.
  Tests the following:
  - the lines of split_windows, line feeds and carriage returns separating
*/
bool
d_tests_arg_argv_split_windows_breaks(void)
{
    return d_tests_arg_argv_splits("SPLIT_WINDOWS_BREAKS",
                                   SPLIT_WINDOWS_BREAKS_ROWS,
                                   SPLIT_WINDOWS_BREAKS_COUNT,
                                   SPLIT_WINDOWS_BREAKS);
}

//==============================================================================
// 4.  SPLITTING: WINDOWS, PROGRAM NAMES
//==============================================================================

// SPLIT_WINDOWS_NAMES_ROWS
//   constant: the lines of d_tests_arg_argv_split_windows_names.
static const struct d_tests_arg_argv_split SPLIT_WINDOWS_NAMES_ROWS[] =
{
    { "W01", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME,
      "\"a b c\" d e", 11u },
    { "W02", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME,
      "\"ab\\\"c\" \"\\\\\" d", 14u },
    { "W03", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME,
      "a\\\\\\b d\"e f\"g h", 15u },
    { "W04", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME,
      "a\\\\\\\"b c d", 10u },
    { "W05", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME,
      "a\\\\\\\\\"b c\" d e", 14u },
    { "W06", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME,
      "a\"b\"\" c d", 9u },
    { "W07", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME, "", 0u },
    { "W08", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME, " ", 1u },
    { "W09", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME, "\"", 1u },
    { "W10", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME, "\"\"", 2u },
    { "W11", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME, "\"\" \"\"", 5u },
    { "W12", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME, "a\\", 2u },
    { "W13", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME, "\\\"", 2u },
    { "W14", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME, "\\\\\"", 3u },
    { "W15", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME, "\"a\\\\\"", 5u },
    { "W16", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME,
      "\"a\\\\\\\"b\"", 8u },
    { "W17", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME, "a\"\"b", 4u },
    { "W18", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME,
      "\"a\"\"b\"", 6u },
    { "W19", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME,
      "\"a\"\"\"b\"", 7u },
    { "W20", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME, "a\nb", 3u },
    { "W21", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME, "\"a\nb\"", 5u },
    { "W22", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME, "a\rb c", 5u },
    { "W23", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME, "\t a \t", 5u },
    { "W24", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME, "\\\\\\\\", 4u },
    { "W25", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME, "a\\b\\\\c", 6u },
    { "W26", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME,
      "\"\\\\\\\\\"", 6u },
    { "W27", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME, "\"\"\"\"", 4u },
    { "W28", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME,
      "\"\"\"a\"\"\"", 7u },
    { "W29", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME, "a \"\" b", 6u },
    { "W30", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME,
      "a\x0b" "b\x0c" "c", 5u },
    { "W31", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME,
      "\r\n a \r\n", 7u },
    { "W32", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME,
      "\"C:\\Program Files\\x.exe\" a b", 28u },
    { "W33", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME,
      "C:\\x\\y.exe \"a b\"", 16u },
    { "W34", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME,
      "\"a\"b\" c\" d", 10u },
    { "W35", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME,
      "\"a\"\"b\" c", 8u },
    { "W36", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME, " a b", 4u },
    { "W37", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME, "a\\\"b c", 6u },
    { "W38", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME, "\"a\\\" b", 6u },
    { "W39", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME,
      "\\\\server\\x.exe a", 16u },
    { "W40", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME, "a\nb c", 5u },
    { "W41", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME,
      "\"a\nb\" c", 7u },
    { "W42", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME, "prog\r\nx", 7u },
    { "W43", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME, "a\tb", 3u }
};

// SPLIT_WINDOWS_NAMES_COUNT
//   constant: how many cases SPLIT_WINDOWS_NAMES_ROWS holds.
static const size_t SPLIT_WINDOWS_NAMES_COUNT =
    sizeof(SPLIT_WINDOWS_NAMES_ROWS) / sizeof(SPLIT_WINDOWS_NAMES_ROWS[0]);

/*
d_tests_arg_argv_split_windows_names
  The C runtime's arguments after a program's name (D_ARG_SPLIT_PROGRAM_NAME),
which its rule of its own reads.
  Tests the following:
  - the name's quotes, which open and close, and its backslashes, which are
    ordinary
  - a name always given, empty or not
  - paths, quoted and not, and the arguments after them
*/
bool
d_tests_arg_argv_split_windows_names(void)
{
    return d_tests_arg_argv_splits("SPLIT_WINDOWS_NAMES",
                                   SPLIT_WINDOWS_NAMES_ROWS,
                                   SPLIT_WINDOWS_NAMES_COUNT,
                                   SPLIT_WINDOWS_NAMES);
}

//==============================================================================
// 5.  SPLITTING: WINDOWS, PROGRAM NAMES AND LINE BREAKS
//==============================================================================

// SPLIT_WINDOWS_NAMES_BREAKS_ROWS
//   constant: the lines of d_tests_arg_argv_split_windows_names_breaks.
static const struct d_tests_arg_argv_split SPLIT_WINDOWS_NAMES_BREAKS_ROWS[] =
{
    { "W01", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "\"a b c\" d e", 11u },
    { "W02", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "\"ab\\\"c\" \"\\\\\" d", 14u },
    { "W03", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "a\\\\\\b d\"e f\"g h", 15u },
    { "W04", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "a\\\\\\\"b c d", 10u },
    { "W05", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "a\\\\\\\\\"b c\" d e", 14u },
    { "W06", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "a\"b\"\" c d", 9u },
    { "W07", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "", 0u },
    { "W08", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      " ", 1u },
    { "W09", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "\"", 1u },
    { "W10", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "\"\"", 2u },
    { "W11", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "\"\" \"\"", 5u },
    { "W12", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "a\\", 2u },
    { "W13", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "\\\"", 2u },
    { "W14", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "\\\\\"", 3u },
    { "W15", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "\"a\\\\\"", 5u },
    { "W16", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "\"a\\\\\\\"b\"", 8u },
    { "W17", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "a\"\"b", 4u },
    { "W18", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "\"a\"\"b\"", 6u },
    { "W19", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "\"a\"\"\"b\"", 7u },
    { "W20", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "a\nb", 3u },
    { "W21", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "\"a\nb\"", 5u },
    { "W22", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "a\rb c", 5u },
    { "W23", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "\t a \t", 5u },
    { "W24", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "\\\\\\\\", 4u },
    { "W25", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "a\\b\\\\c", 6u },
    { "W26", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "\"\\\\\\\\\"", 6u },
    { "W27", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "\"\"\"\"", 4u },
    { "W28", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "\"\"\"a\"\"\"", 7u },
    { "W29", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "a \"\" b", 6u },
    { "W30", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "a\x0b" "b\x0c" "c", 5u },
    { "W31", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "\r\n a \r\n", 7u },
    { "W32", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "\"C:\\Program Files\\x.exe\" a b", 28u },
    { "W33", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "C:\\x\\y.exe \"a b\"", 16u },
    { "W34", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "\"a\"b\" c\" d", 10u },
    { "W35", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "\"a\"\"b\" c", 8u },
    { "W36", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      " a b", 4u },
    { "W37", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "a\\\"b c", 6u },
    { "W38", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "\"a\\\" b", 6u },
    { "W39", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "\\\\server\\x.exe a", 16u },
    { "W40", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "a\nb c", 5u },
    { "W41", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "\"a\nb\" c", 7u },
    { "W42", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "prog\r\nx", 7u },
    { "W43", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "a\tb", 3u }
};

// SPLIT_WINDOWS_NAMES_BREAKS_COUNT
//   constant: how many cases SPLIT_WINDOWS_NAMES_BREAKS_ROWS holds.
static const size_t SPLIT_WINDOWS_NAMES_BREAKS_COUNT =
    sizeof(SPLIT_WINDOWS_NAMES_BREAKS_ROWS) /
    sizeof(SPLIT_WINDOWS_NAMES_BREAKS_ROWS[0]);

/*
d_tests_arg_argv_split_windows_names_breaks
  A program's name and the arguments after it, with line breaks as separators.
  Tests the following:
  - the lines of split_windows_names, line breaks ending the name too
*/
bool
d_tests_arg_argv_split_windows_names_breaks(void)
{
    return d_tests_arg_argv_splits("SPLIT_WINDOWS_NAMES_BREAKS",
                                   SPLIT_WINDOWS_NAMES_BREAKS_ROWS,
                                   SPLIT_WINDOWS_NAMES_BREAKS_COUNT,
                                   SPLIT_WINDOWS_NAMES_BREAKS);
}

//==============================================================================
// 6.  SPLITTING: CLI11
//==============================================================================

// SPLIT_CLI11_ROWS
//   constant: the lines of d_tests_arg_argv_split_cli11.
static const struct d_tests_arg_argv_split SPLIT_CLI11_ROWS[] =
{
    { "C01", D_ARG_QUOTING_CLI11, 0u, "", 0u },
    { "C02", D_ARG_QUOTING_CLI11, 0u, " a b ", 5u },
    { "C03", D_ARG_QUOTING_CLI11, 0u, "--name=\"a b\" c", 14u },
    { "C04", D_ARG_QUOTING_CLI11, 0u, "--name='a b'", 12u },
    { "C05", D_ARG_QUOTING_CLI11, 0u, "--name=`a b`", 12u },
    { "C06", D_ARG_QUOTING_CLI11, 0u, "-n=\"a b\"", 8u },
    { "C07", D_ARG_QUOTING_CLI11, 0u, "x=\"a b\"", 7u },
    { "C08", D_ARG_QUOTING_CLI11, 0u, "/name:\"a b\" c", 13u },
    { "C09", D_ARG_QUOTING_CLI11, 0u, "\"a b\" c", 7u },
    { "C10", D_ARG_QUOTING_CLI11, 0u, "\"a \\\"b\\\" c\"", 11u },
    { "C11", D_ARG_QUOTING_CLI11, 0u, "\"a\\nb\"", 6u },
    { "C12", D_ARG_QUOTING_CLI11, 0u, "\"a\\tb\\\\\"", 8u },
    { "C13", D_ARG_QUOTING_CLI11, 0u, "\"\\u0041\"", 8u },
    { "C14", D_ARG_QUOTING_CLI11, 0u, "\"\\U0001F600\"", 12u },
    { "C15", D_ARG_QUOTING_CLI11, 0u, "\"\\q\"", 4u },
    { "C16", D_ARG_QUOTING_CLI11, 0u, "\"\\u12\"", 6u },
    { "C17", D_ARG_QUOTING_CLI11, 0u, "\"\\uD800\"", 8u },
    { "C18", D_ARG_QUOTING_CLI11, 0u, "\"\\U00110000\"", 12u },
    { "C19", D_ARG_QUOTING_CLI11, 0u, "\"\\x41\"", 6u },
    { "C20", D_ARG_QUOTING_CLI11, 0u, "'a\\nb'", 6u },
    { "C21", D_ARG_QUOTING_CLI11, 0u, "`a b`", 5u },
    { "C22", D_ARG_QUOTING_CLI11, 0u, "[a b] c", 7u },
    { "C23", D_ARG_QUOTING_CLI11, 0u, "(a (b) c) d", 11u },
    { "C24", D_ARG_QUOTING_CLI11, 0u, "<a b>", 5u },
    { "C25", D_ARG_QUOTING_CLI11, 0u, "{a {b} c}", 9u },
    { "C26", D_ARG_QUOTING_CLI11, 0u, "\"a\"b", 4u },
    { "C27", D_ARG_QUOTING_CLI11, 0u, "'a'b", 4u },
    { "C28", D_ARG_QUOTING_CLI11, 0u, "\"a\"'b'", 6u },
    { "C29", D_ARG_QUOTING_CLI11, 0u, "\"unclosed", 9u },
    { "C30", D_ARG_QUOTING_CLI11, 0u, "'unclosed", 9u },
    { "C31", D_ARG_QUOTING_CLI11, 0u, "[unclosed", 9u },
    { "C32", D_ARG_QUOTING_CLI11, 0u, "\"", 1u },
    { "C33", D_ARG_QUOTING_CLI11, 0u, "'", 1u },
    { "C34", D_ARG_QUOTING_CLI11, 0u, "a \"\" b", 6u },
    { "C35", D_ARG_QUOTING_CLI11, 0u, "a '' b", 6u },
    { "C36", D_ARG_QUOTING_CLI11, 0u, "-a=\"b c", 7u },
    { "C37", D_ARG_QUOTING_CLI11, 0u, "=\"x\"", 4u },
    { "C38", D_ARG_QUOTING_CLI11, 0u, "--a= \"b\"", 8u },
    { "C39", D_ARG_QUOTING_CLI11, 0u, "--a=\"b\" --c=\"d e\"", 17u },
    { "C40", D_ARG_QUOTING_CLI11, 0u, "a=b", 3u },
    { "C41", D_ARG_QUOTING_CLI11, 0u, "-a=b", 4u },
    { "C42", D_ARG_QUOTING_CLI11, 0u, "\"--a=\\\"b\\\"\"", 11u },
    { "C43", D_ARG_QUOTING_CLI11, 0u, "--opt=[a,b]", 11u },
    { "C44", D_ARG_QUOTING_CLI11, 0u, "a\nb\tc", 5u },
    { "C45", D_ARG_QUOTING_CLI11, 0u, "\x0b a \x0c", 5u },
    { "C46", D_ARG_QUOTING_CLI11, 0u, "\"a\\", 3u },
    { "C47", D_ARG_QUOTING_CLI11, 0u, "\"\\u00e9\\b\\f\\r\"", 14u },
    { "C48", D_ARG_QUOTING_CLI11, 0u, "[a \"]\" b] c", 11u },
    { "C49", D_ARG_QUOTING_CLI11, 0u, "(a [b) c] d", 11u },
    { "C50", D_ARG_QUOTING_CLI11, 0u, "[a ']' b] c", 11u },
    { "C51", D_ARG_QUOTING_CLI11, 0u, "[a `]` b] c", 11u },
    { "C52", D_ARG_QUOTING_CLI11, 0u, "[] x", 4u },
    { "C53", D_ARG_QUOTING_CLI11, 0u, "[[a] b] c", 9u },
    { "C54", D_ARG_QUOTING_CLI11, 0u, "`a [b` c", 8u },
    { "C55", D_ARG_QUOTING_CLI11, 0u, "\"a\\\"", 4u },
    { "C56", D_ARG_QUOTING_CLI11, 0u, "\"\\\"", 3u },
    { "C57", D_ARG_QUOTING_CLI11, 0u, "\"a\"\"", 4u },
    { "C58", D_ARG_QUOTING_CLI11, 0u, "'a''", 4u },
    { "C59", D_ARG_QUOTING_CLI11, 0u, "-a x=\"b c\"", 10u },
    { "C60", D_ARG_QUOTING_CLI11, 0u, "-=\"b\"", 5u },
    { "C61", D_ARG_QUOTING_CLI11, 0u, "/:\"b\"", 5u },
    { "C62", D_ARG_QUOTING_CLI11, 0u, "-a=\"", 4u },
    { "C63", D_ARG_QUOTING_CLI11, 0u, "=\"x\" -", 6u },
    { "C64", D_ARG_QUOTING_CLI11, 0u, ":\"x\" /", 6u },
    { "C65", D_ARG_QUOTING_CLI11, 0u, "-a :\"b\"", 7u }
};

// SPLIT_CLI11_COUNT
//   constant: how many cases SPLIT_CLI11_ROWS holds.
static const size_t SPLIT_CLI11_COUNT =
    sizeof(SPLIT_CLI11_ROWS) / sizeof(SPLIT_CLI11_ROWS[0]);

/*
d_tests_arg_argv_split_cli11
  CLI11's split of a command line (D_ARG_QUOTING_CLI11), as its parse of a
string splits one.
  Tests the following:
  - `=` before a quote after a `-name`, which ends a word
  - words opening with a quote or a bracket, running to their match
  - outer quotes removed, and a double-quoted word's escapes, a bad one refused
    with CLI11's message
*/
bool
d_tests_arg_argv_split_cli11(void)
{
    return d_tests_arg_argv_splits("SPLIT_CLI11",
                                   SPLIT_CLI11_ROWS,
                                   SPLIT_CLI11_COUNT,
                                   SPLIT_CLI11);
}

//==============================================================================
// 7.  SPLITTING: CLI11, WINDOWS STYLE
//==============================================================================

// SPLIT_CLI11_WINDOWS_ROWS
//   constant: the lines of d_tests_arg_argv_split_cli11_windows.
static const struct d_tests_arg_argv_split SPLIT_CLI11_WINDOWS_ROWS[] =
{
    { "C01", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "", 0u },
    { "C02", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, " a b ", 5u },
    { "C03", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE,
      "--name=\"a b\" c", 14u },
    { "C04", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE,
      "--name='a b'", 12u },
    { "C05", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE,
      "--name=`a b`", 12u },
    { "C06", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "-n=\"a b\"", 8u },
    { "C07", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "x=\"a b\"", 7u },
    { "C08", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE,
      "/name:\"a b\" c", 13u },
    { "C09", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "\"a b\" c", 7u },
    { "C10", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE,
      "\"a \\\"b\\\" c\"", 11u },
    { "C11", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "\"a\\nb\"", 6u },
    { "C12", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE,
      "\"a\\tb\\\\\"", 8u },
    { "C13", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE,
      "\"\\u0041\"", 8u },
    { "C14", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE,
      "\"\\U0001F600\"", 12u },
    { "C15", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "\"\\q\"", 4u },
    { "C16", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "\"\\u12\"", 6u },
    { "C17", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE,
      "\"\\uD800\"", 8u },
    { "C18", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE,
      "\"\\U00110000\"", 12u },
    { "C19", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "\"\\x41\"", 6u },
    { "C20", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "'a\\nb'", 6u },
    { "C21", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "`a b`", 5u },
    { "C22", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "[a b] c", 7u },
    { "C23", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE,
      "(a (b) c) d", 11u },
    { "C24", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "<a b>", 5u },
    { "C25", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "{a {b} c}", 9u },
    { "C26", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "\"a\"b", 4u },
    { "C27", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "'a'b", 4u },
    { "C28", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "\"a\"'b'", 6u },
    { "C29", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "\"unclosed", 9u },
    { "C30", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "'unclosed", 9u },
    { "C31", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "[unclosed", 9u },
    { "C32", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "\"", 1u },
    { "C33", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "'", 1u },
    { "C34", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "a \"\" b", 6u },
    { "C35", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "a '' b", 6u },
    { "C36", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "-a=\"b c", 7u },
    { "C37", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "=\"x\"", 4u },
    { "C38", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "--a= \"b\"", 8u },
    { "C39", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE,
      "--a=\"b\" --c=\"d e\"", 17u },
    { "C40", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "a=b", 3u },
    { "C41", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "-a=b", 4u },
    { "C42", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE,
      "\"--a=\\\"b\\\"\"", 11u },
    { "C43", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE,
      "--opt=[a,b]", 11u },
    { "C44", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "a\nb\tc", 5u },
    { "C45", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE,
      "\x0b a \x0c", 5u },
    { "C46", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "\"a\\", 3u },
    { "C47", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE,
      "\"\\u00e9\\b\\f\\r\"", 14u },
    { "C48", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE,
      "[a \"]\" b] c", 11u },
    { "C49", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE,
      "(a [b) c] d", 11u },
    { "C50", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE,
      "[a ']' b] c", 11u },
    { "C51", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE,
      "[a `]` b] c", 11u },
    { "C52", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "[] x", 4u },
    { "C53", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "[[a] b] c", 9u },
    { "C54", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "`a [b` c", 8u },
    { "C55", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "\"a\\\"", 4u },
    { "C56", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "\"\\\"", 3u },
    { "C57", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "\"a\"\"", 4u },
    { "C58", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "'a''", 4u },
    { "C59", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE,
      "-a x=\"b c\"", 10u },
    { "C60", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "-=\"b\"", 5u },
    { "C61", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "/:\"b\"", 5u },
    { "C62", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "-a=\"", 4u },
    { "C63", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "=\"x\" -", 6u },
    { "C64", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, ":\"x\" /", 6u },
    { "C65", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE, "-a :\"b\"", 7u }
};

// SPLIT_CLI11_WINDOWS_COUNT
//   constant: how many cases SPLIT_CLI11_WINDOWS_ROWS holds.
static const size_t SPLIT_CLI11_WINDOWS_COUNT =
    sizeof(SPLIT_CLI11_WINDOWS_ROWS) / sizeof(SPLIT_CLI11_WINDOWS_ROWS[0]);

/*
d_tests_arg_argv_split_cli11_windows
  CLI11's split for a command taking Windows-style options
(D_ARG_SPLIT_WINDOWS_STYLE).
  Tests the following:
  - the lines of split_cli11, `:` before a quote after a `/name` ending a word
    too
*/
bool
d_tests_arg_argv_split_cli11_windows(void)
{
    return d_tests_arg_argv_splits("SPLIT_CLI11_WINDOWS",
                                   SPLIT_CLI11_WINDOWS_ROWS,
                                   SPLIT_CLI11_WINDOWS_COUNT,
                                   SPLIT_CLI11_WINDOWS);
}

//==============================================================================
// 8.  SPLITTING: GNU
//==============================================================================

// SPLIT_GNU_ROWS
//   constant: the lines of d_tests_arg_argv_split_gnu.
static const struct d_tests_arg_argv_split SPLIT_GNU_ROWS[] =
{
    { "G01", D_ARG_QUOTING_GNU, 0u, "", 0u },
    { "G02", D_ARG_QUOTING_GNU, 0u, " ", 1u },
    { "G03", D_ARG_QUOTING_GNU, 0u, " \n\t ", 4u },
    { "G04", D_ARG_QUOTING_GNU, 0u, "a", 1u },
    { "G05", D_ARG_QUOTING_GNU, 0u, " a  b ", 6u },
    { "G06", D_ARG_QUOTING_GNU, 0u, "'a b'", 5u },
    { "G07", D_ARG_QUOTING_GNU, 0u, "\"a b\"", 5u },
    { "G08", D_ARG_QUOTING_GNU, 0u, "a'b c'd", 7u },
    { "G09", D_ARG_QUOTING_GNU, 0u, "''", 2u },
    { "G10", D_ARG_QUOTING_GNU, 0u, "\"\"", 2u },
    { "G11", D_ARG_QUOTING_GNU, 0u, "a\\ b", 4u },
    { "G12", D_ARG_QUOTING_GNU, 0u, "\\\\", 2u },
    { "G13", D_ARG_QUOTING_GNU, 0u, "'a\\'b'", 6u },
    { "G14", D_ARG_QUOTING_GNU, 0u, "\"a\\\"b\"", 6u },
    { "G15", D_ARG_QUOTING_GNU, 0u, "'unclosed b", 11u },
    { "G16", D_ARG_QUOTING_GNU, 0u, "\"unclosed b", 11u },
    { "G17", D_ARG_QUOTING_GNU, 0u, "a\\", 2u },
    { "G18", D_ARG_QUOTING_GNU, 0u, "\\", 1u },
    { "G19", D_ARG_QUOTING_GNU, 0u, "'a\nb'", 5u },
    { "G20", D_ARG_QUOTING_GNU, 0u, "a\nb", 3u },
    { "G21", D_ARG_QUOTING_GNU, 0u, "\\\n", 2u },
    { "G22", D_ARG_QUOTING_GNU, 0u, "'\"'", 3u },
    { "G23", D_ARG_QUOTING_GNU, 0u, "\"'\"", 3u },
    { "G24", D_ARG_QUOTING_GNU, 0u, "a\x0b" "b\x0c" "c\rd", 7u },
    { "G25", D_ARG_QUOTING_GNU, 0u, "'a'\"b\"c", 7u },
    { "G26", D_ARG_QUOTING_GNU, 0u, "\"\\", 2u },
    { "G27", D_ARG_QUOTING_GNU, 0u, "a''b", 4u }
};

// SPLIT_GNU_COUNT
//   constant: how many cases SPLIT_GNU_ROWS holds.
static const size_t SPLIT_GNU_COUNT =
    sizeof(SPLIT_GNU_ROWS) / sizeof(SPLIT_GNU_ROWS[0]);

/*
d_tests_arg_argv_split_gnu
  GCC's arguments of a response file (D_ARG_QUOTING_GNU): libiberty's buildargv.
  Tests the following:
  - quotes of either kind keeping all to their match, an unclosed one running to
    the end
  - a backslash taking the byte after it, inside quotes as outside
  - any whitespace separating; text of whitespace alone holding no words
*/
bool
d_tests_arg_argv_split_gnu(void)
{
    return d_tests_arg_argv_splits("SPLIT_GNU",
                                   SPLIT_GNU_ROWS,
                                   SPLIT_GNU_COUNT,
                                   SPLIT_GNU);
}

//==============================================================================
// 9.  SPLITTING: SWITCHES IGNORED
//==============================================================================

// SPLIT_FLAGS_ROWS
//   constant: the lines of d_tests_arg_argv_split_flags.
static const struct d_tests_arg_argv_split SPLIT_FLAGS_ROWS[] =
{
    { "F01", D_ARG_QUOTING_POSIX,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_WINDOWS_STYLE |
      D_ARG_SPLIT_LINE_BREAKS,
      " a  b ", 6u },
    { "F02", D_ARG_QUOTING_POSIX, D_ARG_SPLIT_PROGRAM_NAME, "\"a b\" c", 7u },
    { "F03", D_ARG_QUOTING_POSIX, D_ARG_SPLIT_LINE_BREAKS, "a\nb c", 5u },
    { "F04", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_WINDOWS_STYLE,
      "\"a b\" c", 7u },
    { "F05", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_WINDOWS_STYLE,
      "a:\"b c\" d", 9u },
    { "F06", D_ARG_QUOTING_WINDOWS,
      D_ARG_SPLIT_WINDOWS_STYLE | D_ARG_SPLIT_LINE_BREAKS,
      "a\nb", 3u },
    { "F07", D_ARG_QUOTING_CLI11,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "--name=\"a b\" c", 14u },
    { "F08", D_ARG_QUOTING_CLI11,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_LINE_BREAKS,
      "/name:\"a b\" c", 13u },
    { "F09", D_ARG_QUOTING_CLI11,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_WINDOWS_STYLE |
      D_ARG_SPLIT_LINE_BREAKS,
      "/name:\"a b\" c", 13u },
    { "F10", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_PROGRAM_NAME,
      "\"my prog\" a", 11u },
    { "F11", D_ARG_QUOTING_GNU,
      D_ARG_SPLIT_PROGRAM_NAME | D_ARG_SPLIT_WINDOWS_STYLE |
      D_ARG_SPLIT_LINE_BREAKS,
      " a  b ", 6u },
    { "F12", D_ARG_QUOTING_GNU, D_ARG_SPLIT_PROGRAM_NAME, "'a b' c", 7u },
    { "F13", D_ARG_QUOTING_GNU, D_ARG_SPLIT_WINDOWS_STYLE, "a:\"b c\"", 7u }
};

// SPLIT_FLAGS_COUNT
//   constant: how many cases SPLIT_FLAGS_ROWS holds.
static const size_t SPLIT_FLAGS_COUNT =
    sizeof(SPLIT_FLAGS_ROWS) / sizeof(SPLIT_FLAGS_ROWS[0]);

/*
d_tests_arg_argv_split_flags
  The switches a set of rules ignores, given anyway.
  Tests the following:
  - the program's name and line breaks to POSIX's, CLI11's and GNU's rules
  - Windows-style options to the C runtime's and GNU's rules
*/
bool
d_tests_arg_argv_split_flags(void)
{
    return d_tests_arg_argv_splits("SPLIT_FLAGS",
                                   SPLIT_FLAGS_ROWS,
                                   SPLIT_FLAGS_COUNT,
                                   SPLIT_FLAGS);
}

//==============================================================================
// 10. SPLITTING: THE MODULE'S OWN
//==============================================================================

// SPLIT_OWN_ROWS
//   constant: the lines of d_tests_arg_argv_split_own.
static const struct d_tests_arg_argv_split SPLIT_OWN_ROWS[] =
{
    { "O01", D_ARG_QUOTING_POSIX, 0u, "#a b", 4u },
    { "O02", D_ARG_QUOTING_POSIX, 0u, "a #b", 4u },
    { "O03", D_ARG_QUOTING_POSIX, 0u, "a;b|c&d", 7u },
    { "O04", D_ARG_QUOTING_POSIX, 0u, "<a >b 2>&1", 10u },
    { "O05", D_ARG_QUOTING_POSIX, 0u, "(a) {b,c}", 9u },
    { "O06", D_ARG_QUOTING_POSIX, 0u, "$x ${y} $(z) `w` $((1))", 23u },
    { "O07", D_ARG_QUOTING_POSIX, 0u, "\"$x\" $'a\\tb'", 12u },
    { "O08", D_ARG_QUOTING_POSIX, 0u, "~ ~/a a~", 8u },
    { "O09", D_ARG_QUOTING_POSIX, 0u, "*.c a? [ab]", 11u },
    { "O10", D_ARG_QUOTING_POSIX, 0u, "a\x00" "b c", 5u },
    { "O11", D_ARG_QUOTING_POSIX, 0u, "'\x00' \"x\x00\"", 8u },
    { "O12", D_ARG_QUOTING_WINDOWS, 0u, "a\x00" "b c", 5u },
    { "O13", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_PROGRAM_NAME,
      "\"\x00" "a\" b", 6u },
    { "O14", D_ARG_QUOTING_CLI11, 0u, "a\x00" "b c", 5u },
    { "O15", D_ARG_QUOTING_CLI11, 0u, "\"\x00\" x", 5u },
    { "O16", D_ARG_QUOTING_CLI11, 0u, "\"a\\0b\" c", 8u },
    { "O17", D_ARG_QUOTING_CLI11, D_ARG_SPLIT_WINDOWS_STYLE,
      "\"\\u0000\" x", 10u },
    { "O18", D_ARG_QUOTING_CLI11, 0u, "\x00x y", 4u },
    { "O19", D_ARG_QUOTING_CLI11, 0u, "\"\\u0000a\" \"b\x00\"", 14u },
    { "O20", D_ARG_QUOTING_GNU, 0u, "a\x00" "b c", 5u },
    { "O21", D_ARG_QUOTING_GNU, 0u, "\x00", 1u },
    { "O22", D_ARG_QUOTING_GNU, 0u, "\x00 a", 3u },
    { "O23", D_ARG_QUOTING_GNU, 0u, "'a\x00" "b' c", 7u },
    { "O24", D_ARG_QUOTING_WINDOWS, D_ARG_SPLIT_LINE_BREAKS,
      "\"a\x00\n\" b", 7u }
};

// SPLIT_OWN_COUNT
//   constant: how many cases SPLIT_OWN_ROWS holds.
static const size_t SPLIT_OWN_COUNT =
    sizeof(SPLIT_OWN_ROWS) / sizeof(SPLIT_OWN_ROWS[0]);

/*
d_tests_arg_argv_split_own
  Lines the module reads by rules of its own, which no reference reads so.
  Tests the following:
  - a shell's comments, operators, expansions and patterns, ordinary text
  - a NUL byte, an ordinary character, the word holding it reading as ending
    there
*/
bool
d_tests_arg_argv_split_own(void)
{
    return d_tests_arg_argv_splits("SPLIT_OWN",
                                   SPLIT_OWN_ROWS,
                                   SPLIT_OWN_COUNT,
                                   SPLIT_OWN);
}

//==============================================================================
// 11. SPLITTING: CALLS REFUSED AND KEPT
//==============================================================================

/**
 * @brief Splitting refused without a list, with text that is not there, or
 *        by rules that are none of the four.
 *
 * @return whether every call was refused, leaving the list and the
 *         description as they were.
 */
static bool
d_tests_arg_argv_split_refused(void)
{
    struct d_arg_string_list words = D_ARG_LIST_INIT;
    struct d_arg_buffer      error = D_ARG_BUFFER_INIT;
    const struct d_pack_text line  = d_pack_text_from_cstr("a b");
    const struct d_pack_text none  = { NULL, 1u };
    const bool               held  =
        ( (d_arg_split_line(line,
                            (int)D_ARG_QUOTING_POSIX,
                            0u,
                            NULL,
                            &error) == D_ARG_ERROR_BAD_ARGUMENT)  &&
          (d_arg_split_line(none,
                            (int)D_ARG_QUOTING_GNU,
                            0u,
                            &words,
                            &error) == D_ARG_ERROR_BAD_ARGUMENT)  &&
          (d_arg_split_line(line,
                            -1,
                            0u,
                            &words,
                            &error) == D_ARG_ERROR_BAD_ARGUMENT)  &&
          (d_arg_split_line(line,
                            (int)D_ARG_QUOTING_GNU + 1,
                            0u,
                            &words,
                            &error) == D_ARG_ERROR_BAD_ARGUMENT)  &&
          (words.count == 0u)                                     &&
          (error.length == 0u) );

    return d_tests_arg_argv_held(held,
                                 "splitting without a list or text, or by "
                                 "rules that are none");
}

/**
 * @brief Text that is not there, with no length, split as an empty line by
 *        each set of rules: no words, but the program's name the C runtime
 *        always gives.
 *
 * @return whether each split came to that.
 */
static bool
d_tests_arg_argv_split_nothing(void)
{
    // RULES
    //   constant: the four sets of rules.
    static const int RULES[] = { (int)D_ARG_QUOTING_POSIX,
                                 (int)D_ARG_QUOTING_WINDOWS,
                                 (int)D_ARG_QUOTING_CLI11,
                                 (int)D_ARG_QUOTING_GNU };
    // RULE_COUNT
    //   constant: how many sets RULES holds.
    static const size_t RULE_COUNT = sizeof(RULES) / sizeof(RULES[0]);

    struct d_arg_string_list words = D_ARG_LIST_INIT;
    const struct d_pack_text none  = { NULL, 0u };
    bool                     held  = true;

    // each set of rules, nothing given
    for (size_t index = 0u; index < RULE_COUNT; ++index)
    {
        held = (d_arg_split_line(none,
                                 RULES[index],
                                 0u,
                                 &words,
                                 NULL) == D_ARG_OK) && held;
    }

    const bool empty = (words.count == 0u);
    const int  named = d_arg_split_line(none,
                                        (int)D_ARG_QUOTING_WINDOWS,
                                        D_ARG_SPLIT_PROGRAM_NAME,
                                        &words,
                                        NULL);

    held = ( (held)                         &&
             (empty)                        &&
             (named == D_ARG_OK)            &&
             (words.count == 1u)            &&
             (words.items[0][0] == '\0') );
    d_arg_string_list_release(&words);

    return d_tests_arg_argv_held(held,
                                 "nothing split by each set of rules, and "
                                 "the C runtime's program name");
}

/**
 * @brief Words appended after those a list holds, and a split that fails
 *        leaving the list as it was -- by POSIX's rules and by CLI11's,
 *        which have given words before they fail -- without a buffer for the
 *        description.
 *
 * @return whether the list held what it should after each.
 */
static bool
d_tests_arg_argv_split_kept(void)
{
    struct d_arg_string_list words  = D_ARG_LIST_INIT;
    const int                first  = d_arg_string_list_append(
                                          &words,
                                          d_pack_text_from_cstr("x"));
    const int                gnu    = d_arg_split_line(
                                          d_pack_text_from_cstr("a b"),
                                          (int)D_ARG_QUOTING_GNU,
                                          0u,
                                          &words,
                                          NULL);
    const int                posix  = d_arg_split_line(
                                          d_pack_text_from_cstr("c 'd"),
                                          (int)D_ARG_QUOTING_POSIX,
                                          0u,
                                          &words,
                                          NULL);
    const int                cli11  = d_arg_split_line(
                                          d_pack_text_from_cstr("e \"\\q\""),
                                          (int)D_ARG_QUOTING_CLI11,
                                          0u,
                                          &words,
                                          NULL);
    const bool               held   =
        ( (first == D_ARG_OK)                    &&
          (gnu == D_ARG_OK)                      &&
          (posix == D_ARG_ERROR_INVALID)         &&
          (cli11 == D_ARG_ERROR_INVALID)         &&
          (words.count == 3u)                    &&
          (strcmp(words.items[0],
                  "x") == 0)                     &&
          (strcmp(words.items[1],
                  "a") == 0)                     &&
          (strcmp(words.items[2],
                  "b") == 0) );

    d_arg_string_list_release(&words);

    return d_tests_arg_argv_held(held,
                                 "words appended, and a failure leaving the "
                                 "list as it was");
}

/**
 * @brief Each failure's description appended after what the buffer holds:
 *        POSIX's two, then CLI11's for an escape it refuses; a split that
 *        does not fail adds nothing.
 *
 * @return whether the buffer held the descriptions, in order.
 */
static bool
d_tests_arg_argv_split_described(void)
{
    struct d_arg_string_list words  = D_ARG_LIST_INIT;
    struct d_arg_buffer      error  = D_ARG_BUFFER_INIT;
    const int                before = d_arg_buffer_append_cstr(&error,
                                                               "<");
    const int                single = d_arg_split_line(
                                          d_pack_text_from_cstr("'a"),
                                          (int)D_ARG_QUOTING_POSIX,
                                          0u,
                                          &words,
                                          &error);
    const int                twice  = d_arg_split_line(
                                          d_pack_text_from_cstr("\"a"),
                                          (int)D_ARG_QUOTING_POSIX,
                                          0u,
                                          &words,
                                          &error);
    const int                fine   = d_arg_split_line(
                                          d_pack_text_from_cstr("a"),
                                          (int)D_ARG_QUOTING_CLI11,
                                          0u,
                                          &words,
                                          &error);
    const int                escape = d_arg_split_line(
                                          d_pack_text_from_cstr("\"\\q\""),
                                          (int)D_ARG_QUOTING_CLI11,
                                          0u,
                                          &words,
                                          &error);
    const bool               held   =
        ( (before == D_ARG_OK)                                         &&
          (single == D_ARG_ERROR_INVALID)                              &&
          (twice == D_ARG_ERROR_INVALID)                               &&
          (fine == D_ARG_OK)                                           &&
          (escape == D_ARG_ERROR_INVALID)                              &&
          (words.count == 1u)                                          &&
          (d_tests_arg_same("the descriptions of three failures",
                            d_arg_buffer_text(&error),
                            "<a single quote is not closed"
                            "a double quote is not closed"
                            "unrecognized escape sequence \\q in \\q")) );

    d_arg_string_list_release(&words);
    d_arg_buffer_release(&error);

    return d_tests_arg_argv_held(held,
                                 "each failure's description appended");
}

/**
 * @brief The platform's rules, D_ARG_QUOTING_NATIVE and
 *        D_ARG_QUOTING_NATIVE_RESPONSE: the first POSIX's or the C runtime's,
 *        the second the C runtime's where the first is and GNU's elsewhere;
 *        each taken as rules.
 *
 * @return whether the two are as documented and split by.
 */
static bool
d_tests_arg_argv_split_native(void)
{
    const bool               windows  =
        ((int)D_ARG_QUOTING_NATIVE == (int)D_ARG_QUOTING_WINDOWS);
    const int                response = (windows)
                                        ? (int)D_ARG_QUOTING_WINDOWS
                                        : (int)D_ARG_QUOTING_GNU;
    struct d_arg_string_list words    = D_ARG_LIST_INIT;
    const int                line     = d_arg_split_line(
                                            d_pack_text_from_cstr("a"),
                                            (int)D_ARG_QUOTING_NATIVE,
                                            0u,
                                            &words,
                                            NULL);
    const int                file     = d_arg_split_line(
                                            d_pack_text_from_cstr("b"),
                                            (int)D_ARG_QUOTING_NATIVE_RESPONSE,
                                            D_ARG_SPLIT_LINE_BREAKS,
                                            &words,
                                            NULL);
    const bool               held     =
        ( ( (windows) ||
            ((int)D_ARG_QUOTING_NATIVE == (int)D_ARG_QUOTING_POSIX) )  &&
          ((int)D_ARG_QUOTING_NATIVE_RESPONSE == response)             &&
          (line == D_ARG_OK)                                           &&
          (file == D_ARG_OK)                                           &&
          (words.count == 2u) );

    d_arg_string_list_release(&words);

    return d_tests_arg_argv_held(held,
                                 "the platform's rules, for lines and for "
                                 "response files");
}

/*
d_tests_arg_argv_split_refusals
  The calls of d_arg_split_line besides its rules: what it refuses, what it
keeps, and the platform's rules.
  Tests the following:
  - splitting refused without a list, with text that is not there, or by
    rules that are none of the four
  - text that is not there, with no length, an empty line to each set of
    rules, the C runtime's program name given all the same
  - words appended after a list's; a failure leaving the list as it was
  - each failure's description appended after the buffer's text; POSIX's
    two descriptions, CLI11's for an escape
  - the platform's rules for lines and for response files
*/
bool
d_tests_arg_argv_split_refusals(void)
{
    const bool refused   = d_tests_arg_argv_split_refused();
    const bool nothing   = d_tests_arg_argv_split_nothing();
    const bool kept      = d_tests_arg_argv_split_kept();
    const bool described = d_tests_arg_argv_split_described();
    const bool native    = d_tests_arg_argv_split_native();

    return ( (refused)   &&
             (nothing)   &&
             (kept)      &&
             (described) &&
             (native) );
}

//==============================================================================
// 12. QUOTING: POSIX
//==============================================================================

// QUOTE_POSIX_ROWS
//   constant: the words of d_tests_arg_argv_quote_posix.
static const struct d_tests_arg_argv_quote QUOTE_POSIX_ROWS[] =
{
    { "Q01", D_ARG_QUOTING_POSIX, "", 0u },
    { "Q02", D_ARG_QUOTING_POSIX, "a", 1u },
    { "Q03", D_ARG_QUOTING_POSIX, "a b", 3u },
    { "Q04", D_ARG_QUOTING_POSIX, "it's", 4u },
    { "Q05", D_ARG_QUOTING_POSIX, "say \"hi\"", 8u },
    { "Q06", D_ARG_QUOTING_POSIX, "a\\b", 3u },
    { "Q07", D_ARG_QUOTING_POSIX, "a\\", 2u },
    { "Q08", D_ARG_QUOTING_POSIX, "\\", 1u },
    { "Q09", D_ARG_QUOTING_POSIX, "\\\\\"", 3u },
    { "Q10", D_ARG_QUOTING_POSIX, "tab\there", 8u },
    { "Q11", D_ARG_QUOTING_POSIX, "new\nline", 8u },
    { "Q12", D_ARG_QUOTING_POSIX, "--opt=val", 9u },
    { "Q13", D_ARG_QUOTING_POSIX, "-a=\"b\"", 6u },
    { "Q14", D_ARG_QUOTING_POSIX, "-a='b'", 6u },
    { "Q15", D_ARG_QUOTING_POSIX, "/a:`b`", 6u },
    { "Q16", D_ARG_QUOTING_POSIX, "x=", 2u },
    { "Q17", D_ARG_QUOTING_POSIX, "x:", 2u },
    { "Q18", D_ARG_QUOTING_POSIX, "\"", 1u },
    { "Q19", D_ARG_QUOTING_POSIX, "'", 1u },
    { "Q20", D_ARG_QUOTING_POSIX, "`", 1u },
    { "Q21", D_ARG_QUOTING_POSIX, "[a]", 3u },
    { "Q22", D_ARG_QUOTING_POSIX, "(a)", 3u },
    { "Q23", D_ARG_QUOTING_POSIX, "{a}", 3u },
    { "Q24", D_ARG_QUOTING_POSIX, "<a>", 3u },
    { "Q25", D_ARG_QUOTING_POSIX, "a='", 3u },
    { "Q26", D_ARG_QUOTING_POSIX, "C:\\Program Files\\", 17u },
    { "Q27", D_ARG_QUOTING_POSIX, "na\xc3\xafve", 6u },
    { "Q28", D_ARG_QUOTING_POSIX, "$HOME", 5u },
    { "Q29", D_ARG_QUOTING_POSIX, "*", 1u },
    { "Q30", D_ARG_QUOTING_POSIX, "#", 1u },
    { "Q31", D_ARG_QUOTING_POSIX, "a\rb", 3u },
    { "Q32", D_ARG_QUOTING_POSIX, "\x0b", 1u },
    { "Q33", D_ARG_QUOTING_POSIX, "\x0c", 1u },
    { "Q34", D_ARG_QUOTING_POSIX, "=", 1u },
    { "Q35", D_ARG_QUOTING_POSIX, ":", 1u },
    { "Q36", D_ARG_QUOTING_POSIX, "-a=", 3u },
    { "Q37", D_ARG_QUOTING_POSIX, "\"a\"", 3u },
    { "Q38", D_ARG_QUOTING_POSIX, "a\"b\"", 4u },
    { "Q39", D_ARG_QUOTING_POSIX, "\x7f", 1u },
    { "Q40", D_ARG_QUOTING_POSIX, "@file", 5u },
    { "Q41", D_ARG_QUOTING_POSIX, "a=b\"c", 5u },
    { "Q42", D_ARG_QUOTING_POSIX, "a:\"b", 4u },
    { "Q43", D_ARG_QUOTING_POSIX, "\\\"", 2u },
    { "Q44", D_ARG_QUOTING_POSIX, "cr\r\nlf", 6u },
    { "Q45", D_ARG_QUOTING_POSIX, "\x08", 1u },
    { "Q46", D_ARG_QUOTING_POSIX, "a b=", 4u },
    { "Q47", D_ARG_QUOTING_POSIX, "a b:", 4u },
    { "Q48", D_ARG_QUOTING_POSIX, "-x\t=", 4u }
};

// QUOTE_POSIX_COUNT
//   constant: how many cases QUOTE_POSIX_ROWS holds.
static const size_t QUOTE_POSIX_COUNT =
    sizeof(QUOTE_POSIX_ROWS) / sizeof(QUOTE_POSIX_ROWS[0]);

/*
d_tests_arg_argv_quote_posix
  Words written so that POSIX's rules give them back (d_arg_quote_word).
  Tests the following:
  - single quotes, each `'` written `'\''`; a word of letters, digits and
    `@%+=:,./-_` alone as it is
  - each word split back by the same rules into the word alone
*/
bool
d_tests_arg_argv_quote_posix(void)
{
    return d_tests_arg_argv_quotes("QUOTE_POSIX",
                                   QUOTE_POSIX_ROWS,
                                   QUOTE_POSIX_COUNT,
                                   QUOTE_POSIX);
}

//==============================================================================
// 13. QUOTING: WINDOWS
//==============================================================================

// QUOTE_WINDOWS_ROWS
//   constant: the words of d_tests_arg_argv_quote_windows.
static const struct d_tests_arg_argv_quote QUOTE_WINDOWS_ROWS[] =
{
    { "Q01", D_ARG_QUOTING_WINDOWS, "", 0u },
    { "Q02", D_ARG_QUOTING_WINDOWS, "a", 1u },
    { "Q03", D_ARG_QUOTING_WINDOWS, "a b", 3u },
    { "Q04", D_ARG_QUOTING_WINDOWS, "it's", 4u },
    { "Q05", D_ARG_QUOTING_WINDOWS, "say \"hi\"", 8u },
    { "Q06", D_ARG_QUOTING_WINDOWS, "a\\b", 3u },
    { "Q07", D_ARG_QUOTING_WINDOWS, "a\\", 2u },
    { "Q08", D_ARG_QUOTING_WINDOWS, "\\", 1u },
    { "Q09", D_ARG_QUOTING_WINDOWS, "\\\\\"", 3u },
    { "Q10", D_ARG_QUOTING_WINDOWS, "tab\there", 8u },
    { "Q11", D_ARG_QUOTING_WINDOWS, "new\nline", 8u },
    { "Q12", D_ARG_QUOTING_WINDOWS, "--opt=val", 9u },
    { "Q13", D_ARG_QUOTING_WINDOWS, "-a=\"b\"", 6u },
    { "Q14", D_ARG_QUOTING_WINDOWS, "-a='b'", 6u },
    { "Q15", D_ARG_QUOTING_WINDOWS, "/a:`b`", 6u },
    { "Q16", D_ARG_QUOTING_WINDOWS, "x=", 2u },
    { "Q17", D_ARG_QUOTING_WINDOWS, "x:", 2u },
    { "Q18", D_ARG_QUOTING_WINDOWS, "\"", 1u },
    { "Q19", D_ARG_QUOTING_WINDOWS, "'", 1u },
    { "Q20", D_ARG_QUOTING_WINDOWS, "`", 1u },
    { "Q21", D_ARG_QUOTING_WINDOWS, "[a]", 3u },
    { "Q22", D_ARG_QUOTING_WINDOWS, "(a)", 3u },
    { "Q23", D_ARG_QUOTING_WINDOWS, "{a}", 3u },
    { "Q24", D_ARG_QUOTING_WINDOWS, "<a>", 3u },
    { "Q25", D_ARG_QUOTING_WINDOWS, "a='", 3u },
    { "Q26", D_ARG_QUOTING_WINDOWS, "C:\\Program Files\\", 17u },
    { "Q27", D_ARG_QUOTING_WINDOWS, "na\xc3\xafve", 6u },
    { "Q28", D_ARG_QUOTING_WINDOWS, "$HOME", 5u },
    { "Q29", D_ARG_QUOTING_WINDOWS, "*", 1u },
    { "Q30", D_ARG_QUOTING_WINDOWS, "#", 1u },
    { "Q31", D_ARG_QUOTING_WINDOWS, "a\rb", 3u },
    { "Q32", D_ARG_QUOTING_WINDOWS, "\x0b", 1u },
    { "Q33", D_ARG_QUOTING_WINDOWS, "\x0c", 1u },
    { "Q34", D_ARG_QUOTING_WINDOWS, "=", 1u },
    { "Q35", D_ARG_QUOTING_WINDOWS, ":", 1u },
    { "Q36", D_ARG_QUOTING_WINDOWS, "-a=", 3u },
    { "Q37", D_ARG_QUOTING_WINDOWS, "\"a\"", 3u },
    { "Q38", D_ARG_QUOTING_WINDOWS, "a\"b\"", 4u },
    { "Q39", D_ARG_QUOTING_WINDOWS, "\x7f", 1u },
    { "Q40", D_ARG_QUOTING_WINDOWS, "@file", 5u },
    { "Q41", D_ARG_QUOTING_WINDOWS, "a=b\"c", 5u },
    { "Q42", D_ARG_QUOTING_WINDOWS, "a:\"b", 4u },
    { "Q43", D_ARG_QUOTING_WINDOWS, "\\\"", 2u },
    { "Q44", D_ARG_QUOTING_WINDOWS, "cr\r\nlf", 6u },
    { "Q45", D_ARG_QUOTING_WINDOWS, "\x08", 1u },
    { "Q46", D_ARG_QUOTING_WINDOWS, "a b=", 4u },
    { "Q47", D_ARG_QUOTING_WINDOWS, "a b:", 4u },
    { "Q48", D_ARG_QUOTING_WINDOWS, "-x\t=", 4u }
};

// QUOTE_WINDOWS_COUNT
//   constant: how many cases QUOTE_WINDOWS_ROWS holds.
static const size_t QUOTE_WINDOWS_COUNT =
    sizeof(QUOTE_WINDOWS_ROWS) / sizeof(QUOTE_WINDOWS_ROWS[0]);

/*
d_tests_arg_argv_quote_windows
  Words written so that the C runtime's rules give them back (d_arg_quote_word).
  Tests the following:
  - double quotes, backslashes doubled before a `"` and at the end, each `"`
    escaped; a word without a separator or a `"` as it is
  - each word split back by the same rules, with and without line breaks into
    the word alone
*/
bool
d_tests_arg_argv_quote_windows(void)
{
    return d_tests_arg_argv_quotes("QUOTE_WINDOWS",
                                   QUOTE_WINDOWS_ROWS,
                                   QUOTE_WINDOWS_COUNT,
                                   QUOTE_WINDOWS);
}

//==============================================================================
// 14. QUOTING: CLI11
//==============================================================================

// QUOTE_CLI11_ROWS
//   constant: the words of d_tests_arg_argv_quote_cli11.
static const struct d_tests_arg_argv_quote QUOTE_CLI11_ROWS[] =
{
    { "Q01", D_ARG_QUOTING_CLI11, "", 0u },
    { "Q02", D_ARG_QUOTING_CLI11, "a", 1u },
    { "Q03", D_ARG_QUOTING_CLI11, "a b", 3u },
    { "Q04", D_ARG_QUOTING_CLI11, "it's", 4u },
    { "Q05", D_ARG_QUOTING_CLI11, "say \"hi\"", 8u },
    { "Q06", D_ARG_QUOTING_CLI11, "a\\b", 3u },
    { "Q07", D_ARG_QUOTING_CLI11, "a\\", 2u },
    { "Q08", D_ARG_QUOTING_CLI11, "\\", 1u },
    { "Q09", D_ARG_QUOTING_CLI11, "\\\\\"", 3u },
    { "Q10", D_ARG_QUOTING_CLI11, "tab\there", 8u },
    { "Q11", D_ARG_QUOTING_CLI11, "new\nline", 8u },
    { "Q12", D_ARG_QUOTING_CLI11, "--opt=val", 9u },
    { "Q13", D_ARG_QUOTING_CLI11, "-a=\"b\"", 6u },
    { "Q14", D_ARG_QUOTING_CLI11, "-a='b'", 6u },
    { "Q15", D_ARG_QUOTING_CLI11, "/a:`b`", 6u },
    { "Q16", D_ARG_QUOTING_CLI11, "x=", 2u },
    { "Q17", D_ARG_QUOTING_CLI11, "x:", 2u },
    { "Q18", D_ARG_QUOTING_CLI11, "\"", 1u },
    { "Q19", D_ARG_QUOTING_CLI11, "'", 1u },
    { "Q20", D_ARG_QUOTING_CLI11, "`", 1u },
    { "Q21", D_ARG_QUOTING_CLI11, "[a]", 3u },
    { "Q22", D_ARG_QUOTING_CLI11, "(a)", 3u },
    { "Q23", D_ARG_QUOTING_CLI11, "{a}", 3u },
    { "Q24", D_ARG_QUOTING_CLI11, "<a>", 3u },
    { "Q25", D_ARG_QUOTING_CLI11, "a='", 3u },
    { "Q26", D_ARG_QUOTING_CLI11, "C:\\Program Files\\", 17u },
    { "Q27", D_ARG_QUOTING_CLI11, "na\xc3\xafve", 6u },
    { "Q28", D_ARG_QUOTING_CLI11, "$HOME", 5u },
    { "Q29", D_ARG_QUOTING_CLI11, "*", 1u },
    { "Q30", D_ARG_QUOTING_CLI11, "#", 1u },
    { "Q31", D_ARG_QUOTING_CLI11, "a\rb", 3u },
    { "Q32", D_ARG_QUOTING_CLI11, "\x0b", 1u },
    { "Q33", D_ARG_QUOTING_CLI11, "\x0c", 1u },
    { "Q34", D_ARG_QUOTING_CLI11, "=", 1u },
    { "Q35", D_ARG_QUOTING_CLI11, ":", 1u },
    { "Q36", D_ARG_QUOTING_CLI11, "-a=", 3u },
    { "Q37", D_ARG_QUOTING_CLI11, "\"a\"", 3u },
    { "Q38", D_ARG_QUOTING_CLI11, "a\"b\"", 4u },
    { "Q39", D_ARG_QUOTING_CLI11, "\x7f", 1u },
    { "Q40", D_ARG_QUOTING_CLI11, "@file", 5u },
    { "Q41", D_ARG_QUOTING_CLI11, "a=b\"c", 5u },
    { "Q42", D_ARG_QUOTING_CLI11, "a:\"b", 4u },
    { "Q43", D_ARG_QUOTING_CLI11, "\\\"", 2u },
    { "Q44", D_ARG_QUOTING_CLI11, "cr\r\nlf", 6u },
    { "Q45", D_ARG_QUOTING_CLI11, "\x08", 1u },
    { "Q46", D_ARG_QUOTING_CLI11, "a b=", 4u },
    { "Q47", D_ARG_QUOTING_CLI11, "a b:", 4u },
    { "Q48", D_ARG_QUOTING_CLI11, "-x\t=", 4u }
};

// QUOTE_CLI11_COUNT
//   constant: how many cases QUOTE_CLI11_ROWS holds.
static const size_t QUOTE_CLI11_COUNT =
    sizeof(QUOTE_CLI11_ROWS) / sizeof(QUOTE_CLI11_ROWS[0]);

/*
d_tests_arg_argv_quote_cli11
  Words written so that CLI11's rules give them back (d_arg_quote_word).
  Tests the following:
  - double quotes with CLI11's escapes, a `=` or `:` before a quote as a `\u`
    escape; a word CLI11 would not change as it is
  - each word split back by the same rules, in both styles into the word alone
*/
bool
d_tests_arg_argv_quote_cli11(void)
{
    return d_tests_arg_argv_quotes("QUOTE_CLI11",
                                   QUOTE_CLI11_ROWS,
                                   QUOTE_CLI11_COUNT,
                                   QUOTE_CLI11);
}

//==============================================================================
// 15. QUOTING: GNU
//==============================================================================

// QUOTE_GNU_ROWS
//   constant: the words of d_tests_arg_argv_quote_gnu.
static const struct d_tests_arg_argv_quote QUOTE_GNU_ROWS[] =
{
    { "Q01", D_ARG_QUOTING_GNU, "", 0u },
    { "Q02", D_ARG_QUOTING_GNU, "a", 1u },
    { "Q03", D_ARG_QUOTING_GNU, "a b", 3u },
    { "Q04", D_ARG_QUOTING_GNU, "it's", 4u },
    { "Q05", D_ARG_QUOTING_GNU, "say \"hi\"", 8u },
    { "Q06", D_ARG_QUOTING_GNU, "a\\b", 3u },
    { "Q07", D_ARG_QUOTING_GNU, "a\\", 2u },
    { "Q08", D_ARG_QUOTING_GNU, "\\", 1u },
    { "Q09", D_ARG_QUOTING_GNU, "\\\\\"", 3u },
    { "Q10", D_ARG_QUOTING_GNU, "tab\there", 8u },
    { "Q11", D_ARG_QUOTING_GNU, "new\nline", 8u },
    { "Q12", D_ARG_QUOTING_GNU, "--opt=val", 9u },
    { "Q13", D_ARG_QUOTING_GNU, "-a=\"b\"", 6u },
    { "Q14", D_ARG_QUOTING_GNU, "-a='b'", 6u },
    { "Q15", D_ARG_QUOTING_GNU, "/a:`b`", 6u },
    { "Q16", D_ARG_QUOTING_GNU, "x=", 2u },
    { "Q17", D_ARG_QUOTING_GNU, "x:", 2u },
    { "Q18", D_ARG_QUOTING_GNU, "\"", 1u },
    { "Q19", D_ARG_QUOTING_GNU, "'", 1u },
    { "Q20", D_ARG_QUOTING_GNU, "`", 1u },
    { "Q21", D_ARG_QUOTING_GNU, "[a]", 3u },
    { "Q22", D_ARG_QUOTING_GNU, "(a)", 3u },
    { "Q23", D_ARG_QUOTING_GNU, "{a}", 3u },
    { "Q24", D_ARG_QUOTING_GNU, "<a>", 3u },
    { "Q25", D_ARG_QUOTING_GNU, "a='", 3u },
    { "Q26", D_ARG_QUOTING_GNU, "C:\\Program Files\\", 17u },
    { "Q27", D_ARG_QUOTING_GNU, "na\xc3\xafve", 6u },
    { "Q28", D_ARG_QUOTING_GNU, "$HOME", 5u },
    { "Q29", D_ARG_QUOTING_GNU, "*", 1u },
    { "Q30", D_ARG_QUOTING_GNU, "#", 1u },
    { "Q31", D_ARG_QUOTING_GNU, "a\rb", 3u },
    { "Q32", D_ARG_QUOTING_GNU, "\x0b", 1u },
    { "Q33", D_ARG_QUOTING_GNU, "\x0c", 1u },
    { "Q34", D_ARG_QUOTING_GNU, "=", 1u },
    { "Q35", D_ARG_QUOTING_GNU, ":", 1u },
    { "Q36", D_ARG_QUOTING_GNU, "-a=", 3u },
    { "Q37", D_ARG_QUOTING_GNU, "\"a\"", 3u },
    { "Q38", D_ARG_QUOTING_GNU, "a\"b\"", 4u },
    { "Q39", D_ARG_QUOTING_GNU, "\x7f", 1u },
    { "Q40", D_ARG_QUOTING_GNU, "@file", 5u },
    { "Q41", D_ARG_QUOTING_GNU, "a=b\"c", 5u },
    { "Q42", D_ARG_QUOTING_GNU, "a:\"b", 4u },
    { "Q43", D_ARG_QUOTING_GNU, "\\\"", 2u },
    { "Q44", D_ARG_QUOTING_GNU, "cr\r\nlf", 6u },
    { "Q45", D_ARG_QUOTING_GNU, "\x08", 1u },
    { "Q46", D_ARG_QUOTING_GNU, "a b=", 4u },
    { "Q47", D_ARG_QUOTING_GNU, "a b:", 4u },
    { "Q48", D_ARG_QUOTING_GNU, "-x\t=", 4u }
};

// QUOTE_GNU_COUNT
//   constant: how many cases QUOTE_GNU_ROWS holds.
static const size_t QUOTE_GNU_COUNT =
    sizeof(QUOTE_GNU_ROWS) / sizeof(QUOTE_GNU_ROWS[0]);

/*
d_tests_arg_argv_quote_gnu
  Words written so that GNU's rules give them back (d_arg_quote_word).
  Tests the following:
  - each special byte after a backslash; an empty word as two single quotes
  - each word split back by the same rules into the word alone
*/
bool
d_tests_arg_argv_quote_gnu(void)
{
    return d_tests_arg_argv_quotes("QUOTE_GNU",
                                   QUOTE_GNU_ROWS,
                                   QUOTE_GNU_COUNT,
                                   QUOTE_GNU);
}

//==============================================================================
// 16. QUOTING: CALLS REFUSED AND KEPT
//==============================================================================

/**
 * @brief Quoting refused without a buffer, with a word that is not there, or
 *        by rules that are none of the four.
 *
 * @return whether every call was refused, leaving the buffer as it was.
 */
static bool
d_tests_arg_argv_quote_refused(void)
{
    struct d_arg_buffer      out  = D_ARG_BUFFER_INIT;
    const struct d_pack_text word = d_pack_text_from_cstr("a b");
    const struct d_pack_text none = { NULL, 1u };
    const bool               held =
        ( (d_arg_quote_word(word,
                            (int)D_ARG_QUOTING_POSIX,
                            NULL) == D_ARG_ERROR_BAD_ARGUMENT)  &&
          (d_arg_quote_word(none,
                            (int)D_ARG_QUOTING_GNU,
                            &out) == D_ARG_ERROR_BAD_ARGUMENT)  &&
          (d_arg_quote_word(word,
                            -1,
                            &out) == D_ARG_ERROR_BAD_ARGUMENT)  &&
          (d_arg_quote_word(word,
                            (int)D_ARG_QUOTING_GNU + 1,
                            &out) == D_ARG_ERROR_BAD_ARGUMENT)  &&
          (out.length == 0u) );

    return d_tests_arg_argv_held(held,
                                 "quoting without a buffer or a word, or by "
                                 "rules that are none");
}

/**
 * @brief Quotes a word by a set of rules, appending, and compares what the
 *        buffer then holds, byte for byte.
 *
 * @param[in] _word      the word.
 * @param[in] _quoting   the rules.
 * @param[in] _expected  what the buffer is to hold after `<`.
 * @param[in] _length    how many bytes that is.
 * @return whether the word was quoted to what was expected.
 */
static bool
d_tests_arg_argv_quote_bytes(
    struct d_pack_text _word,
    int                _quoting,
    const char*        _expected,
    size_t             _length
)
{
    struct d_arg_buffer out    = D_ARG_BUFFER_INIT;
    const int           before = d_arg_buffer_append_char(&out,
                                                          '<');
    const int           status = d_arg_quote_word(_word,
                                                  _quoting,
                                                  &out);
    const bool          held   = ( (before == D_ARG_OK)             &&
                                   (status == D_ARG_OK)             &&
                                   (out.length == (_length + 1u))   &&
                                   (memcmp(out.data + 1,
                                           _expected,
                                           _length) == 0) );

    d_arg_buffer_release(&out);

    return held;
}

/**
 * @brief A word that is not there, with no length, quoted as the empty word
 *        by each set of rules; and a word holding a NUL byte, which POSIX's
 *        quote, the others' leave, and CLI11's keep as it is within quotes
 *        it needs for another reason -- each appended after the buffer's
 *        text.
 *
 * @return whether each came to its bytes.
 */
static bool
d_tests_arg_argv_quote_kept(void)
{
    const struct d_pack_text none = { NULL, 0u };
    const struct d_pack_text nul  = d_pack_text_from_span("a\0b",
                                                          3u);
    const struct d_pack_text both = d_pack_text_from_span("a b\0",
                                                          4u);
    const bool               held =
        ( (d_tests_arg_argv_quote_bytes(none,
                                        (int)D_ARG_QUOTING_POSIX,
                                        "''",
                                        2u))                          &&
          (d_tests_arg_argv_quote_bytes(none,
                                        (int)D_ARG_QUOTING_WINDOWS,
                                        "\"\"",
                                        2u))                          &&
          (d_tests_arg_argv_quote_bytes(none,
                                        (int)D_ARG_QUOTING_CLI11,
                                        "\"\"",
                                        2u))                          &&
          (d_tests_arg_argv_quote_bytes(none,
                                        (int)D_ARG_QUOTING_GNU,
                                        "''",
                                        2u))                          &&
          (d_tests_arg_argv_quote_bytes(nul,
                                        (int)D_ARG_QUOTING_POSIX,
                                        "'a\0b'",
                                        5u))                          &&
          (d_tests_arg_argv_quote_bytes(nul,
                                        (int)D_ARG_QUOTING_WINDOWS,
                                        "a\0b",
                                        3u))                          &&
          (d_tests_arg_argv_quote_bytes(nul,
                                        (int)D_ARG_QUOTING_CLI11,
                                        "a\0b",
                                        3u))                          &&
          (d_tests_arg_argv_quote_bytes(nul,
                                        (int)D_ARG_QUOTING_GNU,
                                        "a\0b",
                                        3u))                          &&
          (d_tests_arg_argv_quote_bytes(both,
                                        (int)D_ARG_QUOTING_CLI11,
                                        "\"a b\0\"",
                                        6u)) );

    return d_tests_arg_argv_held(held,
                                 "the empty word, and NUL bytes, appended");
}

/*
d_tests_arg_argv_quote_refusals
  The calls of d_arg_quote_word besides its rules: what it refuses and what
it keeps.
  Tests the following:
  - quoting refused without a buffer, with a word that is not there, or by
    rules that are none of the four
  - a word that is not there, with no length, quoted as the empty word
  - a NUL byte, which POSIX's rules quote and the others leave
  - what is written appended after the buffer's text
*/
bool
d_tests_arg_argv_quote_refusals(void)
{
    const bool refused = d_tests_arg_argv_quote_refused();
    const bool kept    = d_tests_arg_argv_quote_kept();

    return ( (refused) &&
             (kept) );
}

//==============================================================================
// 17. PROGRAM NAMES
//==============================================================================

// PROGRAM_ROWS
//   constant: the lines of d_tests_arg_argv_program_names.
static const struct d_tests_arg_argv_program PROGRAM_ROWS[] =
{
    { "N01", "" },
    { "N02", " " },
    { "N03", "prog" },
    { "N04", "pd/prog" },
    { "N05", "pd/prog a b" },
    { "N06", "pd/my prog a" },
    { "N07", "  pd/my prog  a  " },
    { "N08", "pd/a b c d" },
    { "N09", "pd/dir with space/tool x" },
    { "N10", "pd/only dir/x" },
    { "N11", "\"pd/my prog\" a" },
    { "N12", "\"my prog\" a" },
    { "N13", "\"my \\\"prog\\\"\" a" },
    { "N14", "'quoted name' rest" },
    { "N15", "`back` x" },
    { "N16", "\"unclosed name a" },
    { "N17", "\"\"" },
    { "N18", "\"\" a" },
    { "N19", "\"a\"x b" },
    { "N20", "plain" },
    { "N21", "plain  rest  " },
    { "N22", "\tplain\trest" },
    { "N23", "pd/a  b" },
    { "N24", "pd/x y" },
    { "N25", "\"pd/x y\"z rest" },
    { "N26", "\"a\\\"b\\\"c\" d" },
    { "N27", "'it\\'s' x" },
    { "N28", "pd/only dir" },
    { "N29", "pd/my" },
    { "N30", "pd/dir with space" },
    { "N31", "\"pd/only dir\" a" },
    { "N32", "a \"b c\"" },
    { "N33", "pd/my prog" },
    { "N34", "pd/a b c" },
    { "N35", "\"pd/prog\"" },
    { "N36", "'pd/my prog' x" },
    { "N37", "\"a\\\" b" },
    { "N38", "\"\\\"" },
    { "N39", "a\tb c" },
    { "N40", "q r s" },
    { "N41", "pd/s p  q" },
    { "N42", "\"a\\b\\\"c\" d" },
    { "N43", "\"a\\\"\"" }
};

// PROGRAM_COUNT
//   constant: how many cases PROGRAM_ROWS holds.
static const size_t PROGRAM_COUNT =
    sizeof(PROGRAM_ROWS) / sizeof(PROGRAM_ROWS[0]);

/*
d_tests_arg_argv_program_names
  A program's name at the start of a command line (d_arg_split_program_name),
found among files made in a scratch directory.
  Tests the following:
  - the shortest run of words naming a file, so that an unquoted path holding
    spaces is found whole
  - directories passed over
  - a quoted name when no run names a file, its escaped quotes unescaped, and
    the first word otherwise
  - the rest after the byte that follows the name
*/
bool
d_tests_arg_argv_program_names(void)
{
    return d_tests_arg_argv_programs("PROGRAM_NAMES",
                                     PROGRAM_ROWS,
                                     PROGRAM_COUNT,
                                     PROGRAM_NAMES);
}

//==============================================================================
// 18. PROGRAM NAMES: CALLS REFUSED AND KEPT
//==============================================================================

/**
 * @brief Finding a program's name refused without a buffer for it, without
 *        a span for the rest, or with a line that is not there.
 *
 * @return whether every call was refused, leaving the name and the rest as
 *         they were.
 */
static bool
d_tests_arg_argv_program_refused(void)
{
    struct d_arg_buffer      name = D_ARG_BUFFER_INIT;
    const struct d_pack_text line = d_pack_text_from_cstr("prog a");
    const struct d_pack_text none = { NULL, 1u };
    const struct d_pack_text kept = d_pack_text_from_cstr("kept");
    struct d_pack_text       rest = kept;
    const bool               held =
        ( (d_arg_split_program_name(line,
                                    NULL,
                                    &rest) == D_ARG_ERROR_BAD_ARGUMENT)  &&
          (d_arg_split_program_name(line,
                                    &name,
                                    NULL) == D_ARG_ERROR_BAD_ARGUMENT)   &&
          (d_arg_split_program_name(none,
                                    &name,
                                    &rest) == D_ARG_ERROR_BAD_ARGUMENT)  &&
          (name.length == 0u)                                            &&
          (rest.data == kept.data)                                       &&
          (rest.length == kept.length) );

    return d_tests_arg_argv_held(held,
                                 "a program's name without a buffer, a "
                                 "span or a line");
}

/**
 * @brief A line that is not there, with no length, and one of spaces alone:
 *        an empty name and no rest; and a name appended after the buffer's
 *        text, the rest a span of the line.
 *
 * @return whether each came to that.
 */
static bool
d_tests_arg_argv_program_kept(void)
{
    // LINE
    //   constant: a line whose first word names no file.
    static const char LINE[] = "prog  a b ";

    struct d_arg_buffer      name   = D_ARG_BUFFER_INIT;
    struct d_pack_text       rest   = d_pack_text_from_cstr("kept");
    const struct d_pack_text none   = { NULL, 0u };
    const int                absent = d_arg_split_program_name(none,
                                                               &name,
                                                               &rest);
    const bool               empty  = ( (absent == D_ARG_OK) &&
                                        (name.length == 0u)  &&
                                        (rest.length == 0u) );
    const int                spaces = d_arg_split_program_name(
                                          d_pack_text_from_cstr("   "),
                                          &name,
                                          &rest);
    const bool               blank  = ( (spaces == D_ARG_OK) &&
                                        (name.length == 0u)  &&
                                        (rest.length == 0u) );
    const int                before = d_arg_buffer_append_char(&name,
                                                               '<');
    const int                found  = d_arg_split_program_name(
                                          d_pack_text_from_cstr(LINE),
                                          &name,
                                          &rest);
    const bool               held   =
        ( (empty)                                        &&
          (blank)                                        &&
          (before == D_ARG_OK)                           &&
          (found == D_ARG_OK)                            &&
          (d_tests_arg_same("a name appended",
                            d_arg_buffer_text(&name),
                            "<prog"))                    &&
          (rest.data == (LINE + 6))                      &&
          (rest.length == 3u) );

    d_arg_buffer_release(&name);

    return d_tests_arg_argv_held(held,
                                 "an empty line, a blank one, and a name "
                                 "appended");
}

/*
d_tests_arg_argv_program_refusals
  The calls of d_arg_split_program_name besides its search: what it refuses
and what it keeps.
  Tests the following:
  - finding a name refused without a buffer, a span for the rest, or a line
  - a line that is not there, and one of spaces alone, an empty name with no
    rest
  - the name appended after the buffer's text, the rest a span of the line,
    trimmed
*/
bool
d_tests_arg_argv_program_refusals(void)
{
    const bool refused = d_tests_arg_argv_program_refused();
    const bool kept    = d_tests_arg_argv_program_kept();

    return ( (refused) &&
             (kept) );
}

//==============================================================================
// 19. RESPONSE FILES: READING
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// READ_ROWS
//   constant: the files of d_tests_arg_argv_response_read, and the rules each
// is read by.
static const struct d_tests_arg_argv_read READ_ROWS[] =
{
    { "R01", D_ARG_QUOTING_POSIX, "simple.rsp" },
    { "R02", D_ARG_QUOTING_POSIX, "gnuq.rsp" },
    { "R03", D_ARG_QUOTING_POSIX, "crlf.rsp" },
    { "R04", D_ARG_QUOTING_POSIX, "ws.rsp" },
    { "R05", D_ARG_QUOTING_POSIX, "empty.rsp" },
    { "R06", D_ARG_QUOTING_POSIX, "quote_open.rsp" },
    { "R07", D_ARG_QUOTING_POSIX, "nested1.rsp" },
    { "R08", D_ARG_QUOTING_POSIX, "bom8.rsp" },
    { "R09", D_ARG_QUOTING_POSIX, "bom16be.rsp" },
    { "R10", D_ARG_QUOTING_POSIX, "bomonly16le.rsp" },
    { "R11", D_ARG_QUOTING_WINDOWS, "simple.rsp" },
    { "R12", D_ARG_QUOTING_WINDOWS, "winq.rsp" },
    { "R13", D_ARG_QUOTING_WINDOWS, "crlf.rsp" },
    { "R14", D_ARG_QUOTING_WINDOWS, "ws.rsp" },
    { "R15", D_ARG_QUOTING_WINDOWS, "empty.rsp" },
    { "R16", D_ARG_QUOTING_WINDOWS, "bom8.rsp" },
    { "R17", D_ARG_QUOTING_WINDOWS, "bom16le.rsp" },
    { "R18", D_ARG_QUOTING_WINDOWS, "bom16be.rsp" },
    { "R19", D_ARG_QUOTING_WINDOWS, "latin1.rsp" },
    { "R20", D_ARG_QUOTING_WINDOWS, "bad16le.rsp" },
    { "R21", D_ARG_QUOTING_WINDOWS, "bad16be.rsp" },
    { "R22", D_ARG_QUOTING_WINDOWS, "odd16le.rsp" },
    { "R23", D_ARG_QUOTING_WINDOWS, "pairs16le.rsp" },
    { "R24", D_ARG_QUOTING_WINDOWS, "bomonly16le.rsp" },
    { "R25", D_ARG_QUOTING_WINDOWS, "bomonly16be.rsp" },
    { "R26", D_ARG_QUOTING_WINDOWS, "bomonly8.rsp" },
    { "R27", D_ARG_QUOTING_CLI11, "simple.rsp" },
    { "R28", D_ARG_QUOTING_CLI11, "gnuq.rsp" },
    { "R29", D_ARG_QUOTING_CLI11, "winq.rsp" },
    { "R30", D_ARG_QUOTING_CLI11, "crlf.rsp" },
    { "R31", D_ARG_QUOTING_CLI11, "ws.rsp" },
    { "R32", D_ARG_QUOTING_CLI11, "empty.rsp" },
    { "R33", D_ARG_QUOTING_CLI11, "bom16le.rsp" },
    { "R34", D_ARG_QUOTING_GNU, "simple.rsp" },
    { "R35", D_ARG_QUOTING_GNU, "gnuq.rsp" },
    { "R36", D_ARG_QUOTING_GNU, "winq.rsp" },
    { "R37", D_ARG_QUOTING_GNU, "crlf.rsp" },
    { "R38", D_ARG_QUOTING_GNU, "ws.rsp" },
    { "R39", D_ARG_QUOTING_GNU, "empty.rsp" },
    { "R40", D_ARG_QUOTING_GNU, "latin1.rsp" },
    { "R41", D_ARG_QUOTING_GNU, "quote_open.rsp" },
    { "R42", D_ARG_QUOTING_GNU, "nested1.rsp" },
    { "R43", D_ARG_QUOTING_GNU, "bom8.rsp" },
    { "R44", D_ARG_QUOTING_GNU, "bom16le.rsp" },
    { "R45", D_ARG_QUOTING_GNU, "bom8nl.rsp" },
    { "R46", D_ARG_QUOTING_GNU, "bomonly8.rsp" },
    { "R47", D_ARG_QUOTING_POSIX, "nulbyte.rsp" },
    { "R48", D_ARG_QUOTING_WINDOWS, "nulbyte.rsp" },
    { "R49", D_ARG_QUOTING_CLI11, "nulbyte.rsp" },
    { "R50", D_ARG_QUOTING_GNU, "nulbyte.rsp" }
};

// READ_COUNT
//   constant: how many cases READ_ROWS holds.
static const size_t READ_COUNT = sizeof(READ_ROWS) / sizeof(READ_ROWS[0]);

/*
d_tests_arg_argv_response_read
  Response files read whole and split by each set of rules
(d_arg_read_response_file).
  Tests the following:
  - line breaks separating words, as each set of rules reads them
  - byte-order marks honoured: UTF-8's skipped, UTF-16 converted either way
    round, a broken one repaired with U+FFFD
  - files that break their rules, refused
  - a NUL byte an ordinary byte, each word in the list read to its first
*/
bool
d_tests_arg_argv_response_read(void)
{
    return d_tests_arg_argv_reads("RESPONSE_READ",
                                  READ_ROWS,
                                  READ_COUNT,
                                  RESPONSE_READ);
}

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

//==============================================================================
// 20. RESPONSE FILES: GCC'S
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// EXPAND_GNU_ROWS
//   constant: the arguments of d_tests_arg_argv_response_expand_gnu.
static const struct d_tests_arg_argv_expand EXPAND_GNU_ROWS[] =
{
    { "E01", D_ARG_QUOTING_GNU,
      { "@simple.rsp", "@gnuq.rsp" } },
    { "E02", D_ARG_QUOTING_GNU,
      { "x", "@simple.rsp", "y", "@gnuq.rsp" } },
    { "E03", D_ARG_QUOTING_GNU,
      { "@nested2.rsp", "@gnuq.rsp" } },
    { "E04", D_ARG_QUOTING_GNU,
      { "@empty.rsp", "a", "@gnuq.rsp" } },
    { "E05", D_ARG_QUOTING_GNU,
      { "@ws.rsp", "@gnuq.rsp" } },
    { "E06", D_ARG_QUOTING_GNU,
      { "@crlf.rsp", "@gnuq.rsp" } },
    { "E07", D_ARG_QUOTING_GNU,
      { "@missing.rsp", "b", "@gnuq.rsp" } },
    { "E08", D_ARG_QUOTING_GNU,
      { "@", "c", "@gnuq.rsp" } },
    { "E09", D_ARG_QUOTING_GNU,
      { "@@simple.rsp", "@gnuq.rsp" } },
    { "E10", D_ARG_QUOTING_GNU,
      { "-f", "@simple.rsp", "--opt=@x", "@gnuq.rsp" } },
    { "E11", D_ARG_QUOTING_GNU,
      { "@latin1.rsp", "@gnuq.rsp" } },
    { "E12", D_ARG_QUOTING_GNU,
      { "@simple.rsp", "@simple.rsp", "@gnuq.rsp" } },
    { "E13", D_ARG_QUOTING_GNU,
      { "plain", "@gnuq.rsp" } },
    { "E14", D_ARG_QUOTING_GNU,
      { "@gnuq.rsp" } },
    { "E15", D_ARG_QUOTING_GNU,
      { "@deep5.rsp", "@gnuq.rsp" } },
    { "E16", D_ARG_QUOTING_GNU,
      { "@r", "@gnuq.rsp" } }
};

// EXPAND_GNU_COUNT
//   constant: how many cases EXPAND_GNU_ROWS holds.
static const size_t EXPAND_GNU_COUNT =
    sizeof(EXPAND_GNU_ROWS) / sizeof(EXPAND_GNU_ROWS[0]);

/*
d_tests_arg_argv_response_expand_gnu
  Main's arguments naming response files (d_arg_parser_parse,
d_arg_parser_set_response_files), read by GNU's rules, as libiberty's expandargv
expands them.
  Tests the following:
  - each `@file` replaced by its file's words, in place, nested files in turn
  - a file that cannot be read, a bare `@` and `@@file` kept as arguments
*/
bool
d_tests_arg_argv_response_expand_gnu(void)
{
    return d_tests_arg_argv_expands("RESPONSE_EXPAND_GNU",
                                    EXPAND_GNU_ROWS,
                                    EXPAND_GNU_COUNT,
                                    RESPONSE_EXPAND_GNU);
}

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

//==============================================================================
// 21. RESPONSE FILES: WINDOWS'
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// EXPAND_WINDOWS_ROWS
//   constant: the arguments of d_tests_arg_argv_response_expand_windows.
static const struct d_tests_arg_argv_expand EXPAND_WINDOWS_ROWS[] =
{
    { "E01", D_ARG_QUOTING_WINDOWS,
      { "@simple.rsp", "@winq.rsp" } },
    { "E02", D_ARG_QUOTING_WINDOWS,
      { "x", "@simple.rsp", "y", "@winq.rsp" } },
    { "E03", D_ARG_QUOTING_WINDOWS,
      { "@nested2.rsp", "@winq.rsp" } },
    { "E04", D_ARG_QUOTING_WINDOWS,
      { "@empty.rsp", "a", "@winq.rsp" } },
    { "E05", D_ARG_QUOTING_WINDOWS,
      { "@ws.rsp", "@winq.rsp" } },
    { "E06", D_ARG_QUOTING_WINDOWS,
      { "@crlf.rsp", "@winq.rsp" } },
    { "E07", D_ARG_QUOTING_WINDOWS,
      { "@missing.rsp", "b", "@winq.rsp" } },
    { "E08", D_ARG_QUOTING_WINDOWS,
      { "@@simple.rsp", "@winq.rsp" } },
    { "E09", D_ARG_QUOTING_WINDOWS,
      { "-f", "@simple.rsp", "--opt=@x", "@winq.rsp" } },
    { "E10", D_ARG_QUOTING_WINDOWS,
      { "@latin1.rsp", "@winq.rsp" } },
    { "E11", D_ARG_QUOTING_WINDOWS,
      { "@simple.rsp", "@simple.rsp", "@winq.rsp" } },
    { "E12", D_ARG_QUOTING_WINDOWS,
      { "plain", "@winq.rsp" } },
    { "E13", D_ARG_QUOTING_WINDOWS,
      { "@winq.rsp" } },
    { "E14", D_ARG_QUOTING_WINDOWS,
      { "@deep5.rsp", "@winq.rsp" } },
    { "E15", D_ARG_QUOTING_WINDOWS,
      { "@r", "@winq.rsp" } },
    { "E16", D_ARG_QUOTING_WINDOWS,
      { "@bom8.rsp" } },
    { "E17", D_ARG_QUOTING_WINDOWS,
      { "@bom16le.rsp", "z" } },
    { "E18", D_ARG_QUOTING_WINDOWS,
      { "@bom16be.rsp" } }
};

// EXPAND_WINDOWS_COUNT
//   constant: how many cases EXPAND_WINDOWS_ROWS holds.
static const size_t EXPAND_WINDOWS_COUNT =
    sizeof(EXPAND_WINDOWS_ROWS) / sizeof(EXPAND_WINDOWS_ROWS[0]);

/*
d_tests_arg_argv_response_expand_windows
  Main's arguments naming response files, read by the C runtime's rules, as
LLVM's ExpansionContext expands them for clang-cl.
  Tests the following:
  - each `@file` replaced by its file's words, line breaks separating
  - byte-order marks honoured
*/
bool
d_tests_arg_argv_response_expand_windows(void)
{
    return d_tests_arg_argv_expands("RESPONSE_EXPAND_WINDOWS",
                                    EXPAND_WINDOWS_ROWS,
                                    EXPAND_WINDOWS_COUNT,
                                    RESPONSE_EXPAND_WINDOWS);
}

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

//==============================================================================
// 22. RESPONSE FILES: THE MODULE'S OWN
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// EXPAND_OWN_ROWS
//   constant: the arguments of d_tests_arg_argv_response_expand_own.
static const struct d_tests_arg_argv_expand EXPAND_OWN_ROWS[] =
{
    { "E01", D_ARG_QUOTING_GNU,
      { "@dash.rsp", "@simple.rsp" } },
    { "E02", D_ARG_QUOTING_GNU,
      { "--", "@simple.rsp" } },
    { "E03", D_ARG_QUOTING_GNU,
      { "a", "@simple.rsp", "--", "@simple.rsp" } },
    { "E04", D_ARG_QUOTING_GNU,
      { "@deep1.rsp" } },
    { "E05", D_ARG_QUOTING_GNU,
      { "@deep4.rsp" } },
    { "E06", D_ARG_QUOTING_GNU,
      { "@deep3.rsp" } },
    { "E07", D_ARG_QUOTING_GNU,
      { "@self.rsp" } },
    { "E08", D_ARG_QUOTING_GNU,
      { "@selfat.rsp" } },
    { "E09", D_ARG_QUOTING_GNU,
      { "@adir", "x" } },
    { "E10", D_ARG_QUOTING_POSIX,
      { "@quote_open.rsp" } },
    { "E11", D_ARG_QUOTING_POSIX,
      { "a", "@simple.rsp", "@crlf.rsp" } },
    { "E12", D_ARG_QUOTING_CLI11,
      { "@gnuq.rsp", "z", "@winq.rsp" } },
    { "E13", D_ARG_QUOTING_CLI11,
      { "@badesc.rsp" } },
    { "E14", D_ARG_QUOTING_WINDOWS,
      { "@bad16le.rsp", "@odd16le.rsp" } },
    { "E15", D_ARG_QUOTING_GNU,
      { "@bom8.rsp", "@bom16be.rsp" } },
    { "E16", D_ARG_QUOTING_GNU,
      { "@nulbyte.rsp", "z" } },
    { "E17", D_ARG_QUOTING_POSIX,
      { "@nulbyte.rsp" } },
    { "E18", D_ARG_QUOTING_WINDOWS,
      { "@nulbyte.rsp" } },
    { "E19", D_ARG_QUOTING_CLI11,
      { "z", "@nulbyte.rsp" } }
};

// EXPAND_OWN_COUNT
//   constant: how many cases EXPAND_OWN_ROWS holds.
static const size_t EXPAND_OWN_COUNT =
    sizeof(EXPAND_OWN_ROWS) / sizeof(EXPAND_OWN_ROWS[0]);

/*
d_tests_arg_argv_response_expand_own
  Rules of the module's own for response files among main's arguments.
  Tests the following:
  - `--` ending expansion
  - files nested deeper than D_INTERNAL_ARG_PARSER_RESPONSE_DEPTH, a file naming
    itself among them, refused as a FileError
  - a folder kept as an argument
  - files read by POSIX's and CLI11's rules, and refused where they break them
  - byte-order marks honoured by GNU's rules too, and broken UTF-16 repaired
  - a file's words keeping the NUL bytes they hold, by each set of rules, and a
    word holding one naming no file
*/
bool
d_tests_arg_argv_response_expand_own(void)
{
    return d_tests_arg_argv_own_expands("RESPONSE_EXPAND_OWN",
                                        EXPAND_OWN_ROWS,
                                        EXPAND_OWN_COUNT,
                                        RESPONSE_EXPAND_OWN);
}

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

//==============================================================================
// 23. RESPONSE FILES: THE OTHER PARSES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// ENTRY_ROWS
//   constant: the parses of d_tests_arg_argv_response_expand_entries.
static const struct d_tests_arg_argv_entry ENTRY_ROWS[] =
{
    { "A01", ENTRY_ARGS, D_ARG_QUOTING_GNU,
      { { "x", 1u },
        { "@simple.rsp", 11u },
        { "y", 1u } },
      3u },
    { "A02", ENTRY_ARGS, D_ARG_QUOTING_WINDOWS,
      { { "@winq.rsp", 9u },
        { "z", 1u } },
      2u },
    { "A03", ENTRY_REVERSED, D_ARG_QUOTING_GNU,
      { { "q", 1u },
        { "@nested2.rsp", 12u } },
      2u },
    { "A04", ENTRY_LINE, D_ARG_QUOTING_GNU,
      { { "prog x \"@simple.rsp\" y", 22u } },
      1u },
    { "A05", ENTRY_LINE, D_ARG_QUOTING_CLI11,
      { { "prog @simple.rsp -- @simple.rsp", 31u } },
      1u },
    { "A06", ENTRY_ARGS, D_ARG_QUOTING_GNU,
      { { "@simple.rsp\x00x", 13u },
        { "@simple.rsp", 11u } },
      2u },
    { "A07", ENTRY_LINE, D_ARG_QUOTING_GNU,
      { { "prog \"@simple.rsp\\0x\" z", 23u } },
      1u }
};

// ENTRY_COUNT
//   constant: how many cases ENTRY_ROWS holds.
static const size_t ENTRY_COUNT = sizeof(ENTRY_ROWS) / sizeof(ENTRY_ROWS[0]);

/*
d_tests_arg_argv_response_expand_entries
  Arguments naming response files given to the parses that take their words
otherwise than main's: d_arg_parser_parse_args and d_arg_parser_parse_line.
  Tests the following:
  - a vector's `@file` arguments replaced, given in order and borrowed, or last
    first and copied
  - a line's, once it is split by CLI11's rules, `--` ending expansion there too
  - an argument holding a NUL byte, as given or from an escape in a line, naming
    no file
*/
bool
d_tests_arg_argv_response_expand_entries(void)
{
    return d_tests_arg_argv_entries("RESPONSE_EXPAND_ENTRIES",
                                    ENTRY_ROWS,
                                    ENTRY_COUNT,
                                    RESPONSE_EXPAND_ENTRIES);
}

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

//==============================================================================
// 24. RESPONSE FILES: CALLS REFUSED AND KEPT
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

/**
 * @brief Reading a response file refused without a path, without a list, or
 *        by rules that are none of the four -- before the file is looked
 *        for, so that one not there is not described.
 *
 * @return whether every call was refused, leaving the list and the
 *         description as they were.
 */
static bool
d_tests_arg_argv_response_refused(void)
{
    struct d_arg_string_list words = D_ARG_LIST_INIT;
    struct d_arg_buffer      error = D_ARG_BUFFER_INIT;
    const bool               held  =
        ( (d_arg_read_response_file(NULL,
                                    (int)D_ARG_QUOTING_GNU,
                                    &words,
                                    &error) == D_ARG_ERROR_BAD_ARGUMENT)  &&
          (d_arg_read_response_file("missing.rsp",
                                    (int)D_ARG_QUOTING_GNU,
                                    NULL,
                                    &error) == D_ARG_ERROR_BAD_ARGUMENT)  &&
          (d_arg_read_response_file("missing.rsp",
                                    -1,
                                    &words,
                                    &error) == D_ARG_ERROR_BAD_ARGUMENT)  &&
          (d_arg_read_response_file("missing.rsp",
                                    (int)D_ARG_QUOTING_GNU + 1,
                                    &words,
                                    &error) == D_ARG_ERROR_BAD_ARGUMENT)  &&
          (words.count == 0u)                                             &&
          (error.length == 0u) );

    return d_tests_arg_argv_held(held,
                                 "reading without a path or a list, or by "
                                 "rules that are none");
}

/**
 * @brief Files that cannot be read -- one not there, a folder -- refused as
 *        CLI11 describes a missing file, each description appended; a file
 *        breaking its rules refused with its rules' description, leaving the
 *        list as it was; and a file's words appended after the list's.
 *
 * @return whether each came to that.
 */
static bool
d_tests_arg_argv_response_kept(void)
{
    struct d_arg_string_list words   = D_ARG_LIST_INIT;
    struct d_arg_buffer      error   = D_ARG_BUFFER_INIT;
    const int                first   = d_arg_string_list_append(
                                           &words,
                                           d_pack_text_from_cstr("x"));
    const int                missing = d_arg_read_response_file(
                                           "missing.rsp",
                                           (int)D_ARG_QUOTING_GNU,
                                           &words,
                                           &error);
    const int                folder  = d_arg_read_response_file(
                                           "adir",
                                           (int)D_ARG_QUOTING_WINDOWS,
                                           &words,
                                           &error);
    const int                silent  = d_arg_read_response_file(
                                           "missing.rsp",
                                           (int)D_ARG_QUOTING_GNU,
                                           &words,
                                           NULL);
    const int                broken  = d_arg_read_response_file(
                                           "quote_open.rsp",
                                           (int)D_ARG_QUOTING_POSIX,
                                           &words,
                                           &error);
    const int                read    = d_arg_read_response_file(
                                           "simple.rsp",
                                           (int)D_ARG_QUOTING_CLI11,
                                           &words,
                                           &error);
    const bool               held    =
        ( (first == D_ARG_OK)                                         &&
          (missing == D_ARG_ERROR_FILE)                               &&
          (folder == D_ARG_ERROR_FILE)                                &&
          (silent == D_ARG_ERROR_FILE)                                &&
          (broken == D_ARG_ERROR_INVALID)                             &&
          (read == D_ARG_OK)                                          &&
          (words.count == 4u)                                         &&
          (strcmp(words.items[0],
                  "x") == 0)                                          &&
          (strcmp(words.items[3],
                  "c") == 0)                                          &&
          (d_tests_arg_same("the descriptions of three failures",
                            d_arg_buffer_text(&error),
                            "missing.rsp was not readable (missing?)"
                            "adir was not readable (missing?)"
                            "a single quote is not closed")) );

    d_arg_string_list_release(&words);
    d_arg_buffer_release(&error);

    return d_tests_arg_argv_held(held,
                                 "files that cannot be read or break their "
                                 "rules, and words appended");
}

/**
 * @brief Reads `long.rsp` by a set of rules: its numbered words, `w0000`
 *        on, every one in order.
 *
 * @param[in] _quoting  the rules.
 * @param[in] _count    how many words the file holds.
 * @return whether the file was read to each of its words.
 */
static bool
d_tests_arg_argv_response_whole(
    int    _quoting,
    size_t _count
)
{
    struct d_arg_string_list words  = D_ARG_LIST_INIT;
    const int                status = d_arg_read_response_file("long.rsp",
                                                               _quoting,
                                                               &words,
                                                               NULL);
    bool                     held   = ( (status == D_ARG_OK)     &&
                                        (words.count == _count) );

    // each word, by its number
    for (size_t index = 0u; (held) && (index < _count); ++index)
    {
        char expected[16] = { 0 };

        (void)snprintf(expected,
                       sizeof expected,
                       "w%04lu",
                       (unsigned long)index);
        held = (strcmp(words.items[index],
                       expected) == 0);
    }

    d_arg_string_list_release(&words);

    return held;
}

/**
 * @brief A file several times longer than the module reads at a time --
 *        LONG_WORDS numbered words, `w0000 w0001 ...` -- read whole by each
 *        set of rules: every word in order, those across a read's end too.
 *
 * @param[in,out] _scratch  the scratch directory, which the file is made in.
 * @return whether the file was made, and each set of rules read all of it.
 */
static bool
d_tests_arg_argv_response_long(
    struct d_tests_arg_files* _scratch
)
{
    // LONG_WORDS
    //   constant: how many words the file holds, six bytes each with the
    // space after it: 18,000 bytes, past several of the module's reads of
    // 4,096, a word across the end of each.
    enum
    {
        LONG_WORDS = 3000
    };
    // RULES
    //   constant: the four sets of rules the file is read by.
    static const int RULES[] = { (int)D_ARG_QUOTING_POSIX,
                                 (int)D_ARG_QUOTING_WINDOWS,
                                 (int)D_ARG_QUOTING_CLI11,
                                 (int)D_ARG_QUOTING_GNU };

    struct d_arg_buffer text = D_ARG_BUFFER_INIT;

    // the words, a space after each
    for (unsigned long index = 0u; index < (unsigned long)LONG_WORDS; ++index)
    {
        (void)d_arg_buffer_printf(&text,
                                  "w%04lu ",
                                  index);
    }

    bool held = ( (text.failed == 0)                              &&
                  (d_tests_arg_files_write(_scratch,
                                           "long.rsp",
                                           text.data,
                                           text.length)) );

    d_arg_buffer_release(&text);

    // each set of rules
    for (size_t index = 0u; index < (sizeof(RULES) / sizeof(RULES[0])); ++index)
    {
        held = ( (held) &&
                 (d_tests_arg_argv_response_whole(RULES[index],
                                                  (size_t)LONG_WORDS)) );
    }

    return d_tests_arg_argv_held(held,
                                 "a file longer than a read, read whole");
}

/**
 * @brief Whether a parser reads response files, and by the rules given.
 *
 * @param[in] _parser   the parser.
 * @param[in] _enabled  whether it is to read them.
 * @param[in] _quoting  the rules it is to read them by.
 * @return whether the parser's switch and rules are those.
 */
static bool
d_tests_arg_argv_response_state(
    const struct d_arg_parser* _parser,
    bool                       _enabled,
    int                        _quoting
)
{
    const bool enabled =
        ((_parser->flags & D_ARG_PARSER_RESPONSE_FILES) != 0u);

    return ( (enabled == _enabled) &&
             (_parser->response_quoting == _quoting) );
}

/**
 * @brief The parser's switch for response files: off at first, with the
 *        platform's rules; turned on with rules of the caller's, which rules
 *        that are none leave as they were; and turned off, an `\@file`
 *        then an argument like any other -- and nothing set on no parser.
 *
 * @return whether the parser held each setting, and parsed as it said.
 */
static bool
d_tests_arg_argv_response_switch(void)
{
    // ARGS
    //   constant: the arguments parsed with response files off.
    static const char* const ARGS[] = { "@simple.rsp" };

    struct d_tests_arg_run      run   = { 0 };
    struct d_arg_command* const app   = d_tests_arg_begin(&run);
    const bool                  first = d_tests_arg_argv_response_state(
                                            &run.parser,
                                            false,
                                            (int)D_ARG_QUOTING_NATIVE_RESPONSE);

    d_arg_parser_set_response_files(&run.parser,
                                    true,
                                    (int)D_ARG_QUOTING_CLI11);

    const bool on = d_tests_arg_argv_response_state(&run.parser,
                                                    true,
                                                    (int)D_ARG_QUOTING_CLI11);

    d_arg_parser_set_response_files(&run.parser,
                                    true,
                                    (int)D_ARG_QUOTING_GNU + 1);

    const bool unknown = d_tests_arg_argv_response_state(
                             &run.parser,
                             true,
                             (int)D_ARG_QUOTING_CLI11);

    d_arg_parser_set_response_files(&run.parser,
                                    false,
                                    (int)D_ARG_QUOTING_POSIX);
    d_arg_parser_set_response_files(NULL,
                                    true,
                                    (int)D_ARG_QUOTING_GNU);
    d_arg_command_set_allow_extras(app,
                                   (int)D_ARG_EXTRAS_CAPTURE);

    const bool off  = d_tests_arg_argv_response_state(&run.parser,
                                                      false,
                                                      (int)D_ARG_QUOTING_POSIX);
    const bool kept = d_tests_arg_expect(&run,
                                         ARGS,
                                         1u,
                                         "cmd app n=1 miss=[@simple.rsp] "
                                         "subs=[] order=[]\n");

    return d_tests_arg_argv_held(( (first)   &&
                                   (on)      &&
                                   (unknown) &&
                                   (off)     &&
                                   (kept) ),
                                 "the parser's switch for response files");
}

/*
d_tests_arg_argv_response_refusals
  The calls of d_arg_read_response_file besides its rules, and the parser's
switch: what they refuse and what they keep.
  Tests the following:
  - reading refused without a path or a list, or by rules that are none
  - a file not there, and a folder, refused as CLI11's missing file, each
    description appended, without a buffer for it too
  - a file breaking its rules refused, the list as it was; words appended
  - a file several times longer than one read, read whole by each set of
    rules
  - the parser's switch: off at first with the platform's rules, rules that
    are none ignored, an `@file` an argument like any other when off
*/
bool
d_tests_arg_argv_response_refusals(void)
{
    struct d_tests_arg_files scratch = { 0 };
    const bool               entered = d_tests_arg_files_enter(&scratch);
    const bool               made    = ( (entered) &&
                                         (d_tests_arg_argv_responses(
                                              &scratch,
                                              USE_REFUSALS)) );
    const bool               refused = d_tests_arg_argv_response_refused();
    const bool               kept    = ( (made) &&
                                         (d_tests_arg_argv_response_kept()) );
    const bool               whole   = ( (entered) &&
                                         (d_tests_arg_argv_response_long(
                                              &scratch)) );
    const bool               toggled = ( (made) &&
                                         (d_tests_arg_argv_response_switch()) );

    d_tests_arg_files_leave(&scratch);

    return ( (refused) &&
             (kept)    &&
             (whole)   &&
             (toggled) );
}

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

//==============================================================================
// 25. WIDE TEXT
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

/**
 * @brief Converts a row's wide text, from storage of exactly its length,
 *        and appends its line: `<tag> [<units, hexadecimal>]: [<UTF-8>]`.
 *
 * @param[in,out] _out  the transcript.
 * @param[in]     _row  the row.
 * @return whether the storage was had and the text converted.
 */
static bool
d_tests_arg_argv_wide_row(
    struct d_arg_buffer*                _out,
    const struct d_tests_arg_argv_wide* _row
)
{
    wchar_t* const exact = (_row->length > 0u)
                           ? (wchar_t*)malloc(_row->length *
                                              sizeof(wchar_t))
                           : NULL;

    (void)d_arg_buffer_printf(_out,
                              "%s [",
                              _row->tag);

    // each unit written in hexadecimal
    for (size_t index = 0u; index < _row->length; ++index)
    {
        // stored as a `wchar_t` holds it, where there is storage
        if (exact != NULL)
        {
            exact[index] = (wchar_t)_row->units[index];
        }

        (void)d_arg_buffer_printf(_out,
                                  (index > 0u) ? " %lx" : "%lx",
                                  (unsigned long)_row->units[index]);
    }

    const bool          had    = ( (_row->length == 0u) ||
                                   (exact != NULL) );
    struct d_arg_buffer text   = D_ARG_BUFFER_INIT;
    const int           status = (had)
                                 ? d_arg_utf8_from_wide(exact,
                                                        _row->length,
                                                        &text)
                                 : D_ARG_ERROR_OUT_OF_MEMORY;

    (void)d_arg_buffer_append_cstr(_out,
                                   "]: [");
    d_tests_arg_escape(_out,
                       d_arg_buffer_text(&text));
    (void)d_arg_buffer_append_cstr(_out,
                                   "]\n");
    d_arg_buffer_release(&text);
    free(exact);

    return d_tests_arg_argv_held((status == D_ARG_OK),
                                 _row->tag);
}

/**
 * @brief Conversion refused without a buffer, or with text that is not
 *        there; text that is not there, with no length, converted to
 *        nothing; and the UTF-8 appended after the buffer's text.
 *
 * @return whether each came to that.
 */
static bool
d_tests_arg_argv_wide_kept(void)
{
    // TEXT
    //   constant: wide text, `ab`.
    static const wchar_t TEXT[] = { L'a', L'b' };

    struct d_arg_buffer out    = D_ARG_BUFFER_INIT;
    const int           before = d_arg_buffer_append_char(&out,
                                                          '<');
    const bool          held   =
        ( (before == D_ARG_OK)                                        &&
          (d_arg_utf8_from_wide(TEXT,
                                2u,
                                NULL) == D_ARG_ERROR_BAD_ARGUMENT)    &&
          (d_arg_utf8_from_wide(NULL,
                                1u,
                                &out) == D_ARG_ERROR_BAD_ARGUMENT)    &&
          (d_arg_utf8_from_wide(NULL,
                                0u,
                                &out) == D_ARG_OK)                    &&
          (d_arg_utf8_from_wide(TEXT,
                                2u,
                                &out) == D_ARG_OK)                    &&
          (d_tests_arg_same("wide text appended",
                            d_arg_buffer_text(&out),
                            "<ab")) );

    d_arg_buffer_release(&out);

    return d_tests_arg_argv_held(held,
                                 "converting without a buffer or text, and "
                                 "appended");
}

/*
d_tests_arg_argv_wide_text
  Wide text converted to UTF-8 (d_arg_utf8_from_wide): UTF-16 units where
`wchar_t` has 16 bits, code points where it has more -- the table for the
build's `wchar_t` -- as Python's codecs convert them.
  Tests the following:
  - text of one, two, three and four bytes in UTF-8, and U+FFFD itself
  - surrogate pairs; unpaired surrogates, first, last, and in a row, U+FFFD
  - code points that are surrogates, past U+10FFFF or negative, U+FFFD
  - a byte-order mark kept, a NUL kept
  - conversion refused without a buffer or text; nothing for no text;
    appended after the buffer's text
*/
bool
d_tests_arg_argv_wide_text(void)
{
    const bool                                sixteen = (WCHAR_MAX <= 0xFFFF);
    const struct d_tests_arg_argv_wide* const rows    = (sixteen)
                                                        ? WIDE16_ROWS
                                                        : WIDE32_ROWS;
    const size_t                              count   = (sixteen)
                                                        ? WIDE16_COUNT
                                                        : WIDE32_COUNT;
    struct d_arg_buffer                       written = D_ARG_BUFFER_INIT;
    bool                                      held    = true;

    // each row of the build's table
    for (size_t index = 0u; index < count; ++index)
    {
        held = d_tests_arg_argv_wide_row(&written,
                                         &rows[index]) && held;
    }

    const char* const name     = (sixteen)
                                 ? "WIDE_TEXT_UTF16"
                                 : "WIDE_TEXT_UTF32";
    const char* const expected = (sixteen)
                                 ? WIDE_TEXT_UTF16
                                 : WIDE_TEXT_UTF32;
    const bool        same     = d_tests_arg_same(name,
                                                  d_arg_buffer_text(&written),
                                                  expected);
    const bool        kept     = ( (written.failed == 0) &&
                                   (d_tests_arg_argv_wide_kept()) );

    d_arg_buffer_release(&written);

    return ( (held) &&
             (same) &&
             (kept) );
}

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

//==============================================================================
// 26. WIDE TEXT FROM UTF-8
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

/**
 * @brief Appends units as the transcripts write them: in brackets, each in
 *        hexadecimal, parted by spaces.
 *
 * @param[in,out] _out    the transcript.
 * @param[in]     _units  the units; may be NULL when `_count` is 0.
 * @param[in]     _count  how many.
 */
static void
d_tests_arg_argv_units(
    struct d_arg_buffer* _out,
    const wchar_t*       _units,
    size_t               _count
)
{
    (void)d_arg_buffer_append_char(_out,
                                   '[');

    // each unit, after a space when one comes before it
    for (size_t index = 0u; index < _count; ++index)
    {
        (void)d_arg_buffer_printf(_out,
                                  (index > 0u) ? " %lx" : "%lx",
                                  (unsigned long)(uint32_t)_units[index]);
    }

    (void)d_arg_buffer_append_char(_out,
                                   ']');

    return;
}

/**
 * @brief Converts UTF-8 where the conversion must write nothing, into room
 *        for `_capacity` units -- and one past them -- filled with a mark.
 *
 * @param[in] _text      the UTF-8, in storage of exactly its length.
 * @param[in] _length    its length.
 * @param[in] _capacity  the room the conversion is offered.
 * @param[in] _status    the status it must return.
 * @param[in] _needed    the count it must leave: what it reports, or
 *                       `(size_t)-1` where it must report nothing.
 * @return whether the room was had, the status and the count were as
 *         expected, and every unit kept its mark.
 */
static bool
d_tests_arg_argv_untouched(
    const char* _text,
    size_t      _length,
    size_t      _capacity,
    int         _status,
    size_t      _needed
)
{
    // MARK
    //   constant: what the room is filled with before the conversion.
    static const wchar_t MARK = (wchar_t)0x5A5A;

    wchar_t* const room   = (wchar_t*)malloc((_capacity + 1u) *
                                             sizeof(wchar_t));
    size_t         needed = (size_t)-1;
    bool           marked = (room != NULL);

    // the room marked throughout, the unit past it too
    for (size_t index = 0u;
         (room != NULL) && (index <= _capacity);
         ++index)
    {
        room[index] = MARK;
    }

    const int status = (room != NULL)
                       ? d_arg_wide_from_utf8(_text,
                                              _length,
                                              room,
                                              _capacity,
                                              &needed)
                       : D_ARG_ERROR_OUT_OF_MEMORY;

    // each unit still marked
    for (size_t index = 0u;
         (room != NULL) && (index <= _capacity);
         ++index)
    {
        marked = (room[index] == MARK) && marked;
    }

    free(room);

    return ( (status == _status) &&
             (needed == _needed) &&
             (marked) );
}

/**
 * @brief Converts UTF-8 a measure accepted into storage of exactly the
 *        units it takes, appends the units, and converts them back.
 *
 * @param[in,out] _out     the transcript.
 * @param[in]     _text    the UTF-8, in storage of exactly its length.
 * @param[in]     _length  its length.
 * @param[in]     _needed  the units the measure reported.
 * @return whether the storage was had, the conversion reported the measure's
 *         count, the units converted back to the bytes, and a unit too few
 *         wrote nothing.
 */
static bool
d_tests_arg_argv_utf8_units(
    struct d_arg_buffer* _out,
    const char*          _text,
    size_t               _length,
    size_t               _needed
)
{
    wchar_t* const      units   = (_needed > 0u)
                                  ? (wchar_t*)malloc(_needed *
                                                     sizeof(wchar_t))
                                  : NULL;
    const bool          had     = ( (_needed == 0u) ||
                                    (units != NULL) );
    size_t              written = 0u;
    const int           status  = (had)
                                  ? d_arg_wide_from_utf8(_text,
                                                         _length,
                                                         units,
                                                         _needed,
                                                         &written)
                                  : D_ARG_ERROR_OUT_OF_MEMORY;
    struct d_arg_buffer back    = D_ARG_BUFFER_INIT;

    d_tests_arg_argv_units(_out,
                           units,
                           (status == D_ARG_OK) ? written : 0u);

    // the units, as UTF-8 again, are the bytes they came from
    const bool returned =
        ( (status == D_ARG_OK)                             &&
          (written == _needed)                             &&
          (d_arg_utf8_from_wide(units,
                                written,
                                &back) == D_ARG_OK)        &&
          (back.length == _length)                         &&
          ( (_length == 0u) ||
            (memcmp(back.data,
                    _text,
                    _length) == 0) ) );
    const bool short_kept = ( (_needed == 0u) ||
                              (d_tests_arg_argv_untouched(_text,
                                                          _length,
                                                          _needed - 1u,
                                                          D_ARG_OK,
                                                          _needed)) );

    d_arg_buffer_release(&back);
    free(units);

    return ( (returned) &&
             (short_kept) );
}

/**
 * @brief Appends `refused` for UTF-8 a measure refused, and converts it
 *        again, given room.
 *
 * @param[in,out] _out       the transcript.
 * @param[in]     _text      the UTF-8, in storage of exactly its length.
 * @param[in]     _length    its length.
 * @param[in]     _measured  the status the measure returned.
 * @return whether the measure refused the text as not UTF-8, and the
 *         conversion refused it again without a unit or the count written.
 */
static bool
d_tests_arg_argv_utf8_refused(
    struct d_arg_buffer* _out,
    const char*          _text,
    size_t               _length,
    int                  _measured
)
{
    (void)d_arg_buffer_append_cstr(_out,
                                   "refused");

    return ( (_measured == D_ARG_ERROR_CONVERSION) &&
             (d_tests_arg_argv_untouched(_text,
                                         _length,
                                         _length + 1u,
                                         D_ARG_ERROR_CONVERSION,
                                         (size_t)-1)) );
}

/**
 * @brief Measures a row's UTF-8, from storage of exactly its length, then
 *        converts it, and appends its line: `<tag> [<bytes, hexadecimal>]:
 *        [<units, hexadecimal>]`, or `: refused`.
 *
 * @param[in,out] _out  the transcript.
 * @param[in]     _row  the row.
 * @return whether the storage was had, text measured was converted as
 *         d_tests_arg_argv_utf8_units checks, and text refused was refused
 *         again, given room, without a unit or the count written.
 */
static bool
d_tests_arg_argv_utf8_row(
    struct d_arg_buffer*                _out,
    const struct d_tests_arg_argv_utf8* _row
)
{
    const unsigned char* const bytes    =
        (const unsigned char*)_row->text;
    bool                       had      = false;
    char* const                exact    = d_tests_arg_argv_exact(
                                              d_pack_text_from_span(
                                                  _row->text,
                                                  _row->length),
                                              &had);
    size_t                     needed   = 0u;
    const int                  measured = d_arg_wide_from_utf8(exact,
                                                               _row->length,
                                                               NULL,
                                                               0u,
                                                               &needed);

    (void)d_arg_buffer_printf(_out,
                              "%s [",
                              _row->tag);

    // each byte in hexadecimal
    for (size_t index = 0u; index < _row->length; ++index)
    {
        (void)d_arg_buffer_printf(_out,
                                  (index > 0u) ? " %02x" : "%02x",
                                  (unsigned int)bytes[index]);
    }

    (void)d_arg_buffer_append_cstr(_out,
                                   "]: ");

    // text measured converted, text refused refused again
    const bool held = (measured == D_ARG_OK)
                      ? d_tests_arg_argv_utf8_units(_out,
                                                    exact,
                                                    _row->length,
                                                    needed)
                      : d_tests_arg_argv_utf8_refused(_out,
                                                      exact,
                                                      _row->length,
                                                      measured);

    (void)d_arg_buffer_append_char(_out,
                                   '\n');
    free(exact);

    return d_tests_arg_argv_held(( (had) &&
                                   (held) ),
                                 _row->tag);
}

/**
 * @brief Conversion refused without a count, with text that is not there,
 *        or with room that is not there, each writing nothing; and text
 *        that is not there, with no length, measured as nothing.
 *
 * @return whether each came to that.
 */
static bool
d_tests_arg_argv_wide_back_kept(void)
{
    wchar_t    room[2] = { L'x', L'x' };
    size_t     needed  = 7u;
    const bool held    =
        ( (d_arg_wide_from_utf8("ab",
                                2u,
                                room,
                                2u,
                                NULL) == D_ARG_ERROR_BAD_ARGUMENT)     &&
          (d_arg_wide_from_utf8(NULL,
                                1u,
                                room,
                                2u,
                                &needed) == D_ARG_ERROR_BAD_ARGUMENT)  &&
          (d_arg_wide_from_utf8("ab",
                                2u,
                                NULL,
                                1u,
                                &needed) == D_ARG_ERROR_BAD_ARGUMENT)  &&
          (needed == 7u)                                               &&
          (room[0] == L'x')                                            &&
          (room[1] == L'x')                                            &&
          (d_arg_wide_from_utf8(NULL,
                                0u,
                                NULL,
                                0u,
                                &needed) == D_ARG_OK)                  &&
          (needed == 0u) );

    return d_tests_arg_argv_held(held,
                                 "converting without a count, text or room, "
                                 "and converting no text");
}

/*
d_tests_arg_argv_wide_back
  UTF-8 converted to wide text (d_arg_wide_from_utf8): UTF-16 units where
`wchar_t` has 16 bits, code points where it has more -- the transcript for
the build's `wchar_t` -- as Python's strict UTF-8 codec converts it.
  Tests the following:
  - text of one, two, three and four bytes, at the edges of each length and
    beside the surrogates; U+FFFD itself; a byte-order mark and a NUL kept
  - surrogate pairs, where `wchar_t` has 16 bits
  - refused: stray continuation bytes, overlong forms, surrogates, values
    past U+10FFFF, bytes that lead nothing, sequences cut short or broken
  - measured, then converted into storage of exactly the units, which
    convert back to the bytes; a unit too few, or a refusal, writes nothing
  - conversion refused without a count, text or room; no text measured as
    nothing
*/
bool
d_tests_arg_argv_wide_back(void)
{
    const bool          sixteen = (WCHAR_MAX <= 0xFFFF);
    struct d_arg_buffer written = D_ARG_BUFFER_INIT;
    bool                held    = true;

    // each row
    for (size_t index = 0u; index < WIDE_BACK_COUNT; ++index)
    {
        held = d_tests_arg_argv_utf8_row(&written,
                                         &WIDE_BACK_ROWS[index]) && held;
    }

    const char* const name     = (sixteen)
                                 ? "WIDE_BACK_UTF16"
                                 : "WIDE_BACK_UTF32";
    const char* const expected = (sixteen)
                                 ? WIDE_BACK_UTF16
                                 : WIDE_BACK_UTF32;
    const bool        same     = d_tests_arg_same(name,
                                                  d_arg_buffer_text(&written),
                                                  expected);
    const bool        kept     = ( (written.failed == 0) &&
                                   (d_tests_arg_argv_wide_back_kept()) );

    d_arg_buffer_release(&written);

    return ( (held) &&
             (same) &&
             (kept) );
}

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

//==============================================================================
// 27. WIDE ARGUMENTS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

/**
 * @brief Begins a run as the reference's App for its wide parses: an option
 *        `--name`, reported as `name`, and any other argument taken as an
 *        extra.
 *
 * @param[out] _run  the run.
 * @return the root; NULL when the run could not be made, which fails it.
 */
static struct d_arg_command*
d_tests_arg_argv_wide_begin(
    struct d_tests_arg_run* _run
)
{
    struct d_arg_command* const app = d_tests_arg_begin(_run);

    (void)d_tests_arg_make_option(_run,
                                  "name",
                                  app,
                                  "--name");
    d_arg_command_set_allow_extras(app,
                                   (int)D_ARG_EXTRAS_CAPTURE);

    return app;
}

/**
 * @brief Appends a wide parse's run: what it wrote -- its ERR line, when it
 *        failed, and its report -- after its line.
 *
 * @param[in,out] _out     the transcript.
 * @param[in]     _line    the parse's line, `<tag> [<arguments>]`.
 * @param[in,out] _run     the run, ended on return.
 * @param[in]     _status  what the parse came to.
 * @return whether the run was made and its report kept.
 */
static bool
d_tests_arg_argv_wide_record(
    struct d_arg_buffer*    _out,
    const char*             _line,
    struct d_tests_arg_run* _run,
    int                     _status
)
{
    d_tests_arg_record(_run,
                       _status);
    (void)d_arg_buffer_append_cstr(_out,
                                   _line);
    (void)d_arg_buffer_append_text(_out,
                                   d_arg_buffer_text(&_run->transcript));

    const bool held = ( (!_run->failed)                &&
                        (_run->transcript.failed == 0) );

    d_tests_arg_end(_run);

    return held;
}

/**
 * @brief wmain's arguments parsed (d_arg_parser_parse_wide) as the
 *        reference's V01: written as it writes them, and the program's name
 *        kept as the parser's.
 *
 * @param[in,out] _out  the transcript.
 * @return whether the parse ran and kept the program's name.
 */
static bool
d_tests_arg_argv_wide_args(
    struct d_arg_buffer* _out
)
{
    // ARGS
    //   constant: the arguments, the program's name first.
    static wchar_t* const ARGS[] = { L"prog",
                                     L"--name",
                                     L"\u00e9t\u00e9",
                                     L"x y",
                                     L"\U0001F600" };
    // LINE
    //   constant: the parse's line, as the reference writes it.
    static const char LINE[] = "V01 [--name \xc3\xa9t\xc3\xa9 x y "
                               "\xf0\x9f\x98\x80]\n";

    struct d_tests_arg_run      run    = { 0 };
    struct d_arg_command* const app    = d_tests_arg_argv_wide_begin(&run);
    const int                   status = d_arg_parser_parse_wide(&run.parser,
                                                                 5,
                                                                 ARGS);
    const bool                  named  =
        ( (run.parser.program != NULL)   &&
          (strcmp(run.parser.program,
                  "prog") == 0) );
    const bool                  held   = d_tests_arg_argv_wide_record(_out,
                                                                      LINE,
                                                                      &run,
                                                                      status);

    return d_tests_arg_argv_held(( (app != NULL) &&
                                   (named)       &&
                                   (held) ),
                                 "V01");
}

/**
 * @brief A wide line parsed with its program's name
 *        (d_arg_parser_parse_line_wide), written as the reference writes it:
 *        the line as UTF-8 -- the module's own conversion of it, which the
 *        reference's text then checks too -- and the parse's run.
 *
 * @param[in,out] _out     the transcript.
 * @param[in]     _tag     the reference's tag for it.
 * @param[in]     _line    the line.
 * @param[in]     _length  how many units it holds, U+0000 among them.
 * @return whether the parse ran and the line was converted.
 */
static bool
d_tests_arg_argv_wide_line(
    struct d_arg_buffer* _out,
    const char*          _tag,
    const wchar_t*       _line,
    size_t               _length
)
{
    struct d_tests_arg_run      run    = { 0 };
    struct d_arg_command* const app    = d_tests_arg_argv_wide_begin(&run);
    const int                   status = d_arg_parser_parse_line_wide(
                                             &run.parser,
                                             _line,
                                             _length,
                                             true);
    struct d_arg_buffer         utf8   = D_ARG_BUFFER_INIT;
    const int                   made   = d_arg_utf8_from_wide(_line,
                                                              _length,
                                                              &utf8);
    struct d_arg_buffer         line   = D_ARG_BUFFER_INIT;

    (void)d_arg_buffer_printf(&line,
                              "%s [",
                              _tag);
    d_tests_arg_escape(&line,
                       d_arg_buffer_text(&utf8));
    (void)d_arg_buffer_append_cstr(&line,
                                   "]\n");

    const bool held = d_tests_arg_argv_wide_record(_out,
                                                   (line.data != NULL)
                                                   ? line.data
                                                   : "",
                                                   &run,
                                                   status);
    const bool kept = ( (made == D_ARG_OK) &&
                        (line.failed == 0) );

    d_arg_buffer_release(&line);
    d_arg_buffer_release(&utf8);

    return d_tests_arg_argv_held(( (app != NULL) &&
                                   (held)        &&
                                   (kept) ),
                                 _tag);
}

/**
 * @brief The wide lines parsed as the reference's V02 to V04: one past
 *        ASCII and past UTF-16's first range, an escape CLI11 refuses, and
 *        NUL bytes -- U+0000, and one from an escape in double quotes --
 *        each word holding them kept whole.
 *
 * @param[in,out] _out  the transcript.
 * @return whether every parse ran.
 */
static bool
d_tests_arg_argv_wide_lines(
    struct d_arg_buffer* _out
)
{
    // V02 / V03 / V04
    //   constant: the lines.
    static const wchar_t V02[] = L"prog --name \"\u00e9 t\u00e9\" x "
                                 L"\U0001F600";
    static const wchar_t V03[] = L"prog --name \"\\q\" x";
    static const wchar_t V04[] = L"prog --name a\0b \"c\\0d\"";

    const bool wide    = d_tests_arg_argv_wide_line(_out,
                                                    "V02",
                                                    V02,
                                                    (sizeof V02 /
                                                     sizeof V02[0]) - 1u);
    const bool escaped = d_tests_arg_argv_wide_line(_out,
                                                    "V03",
                                                    V03,
                                                    (sizeof V03 /
                                                     sizeof V03[0]) - 1u);
    const bool nul     = d_tests_arg_argv_wide_line(_out,
                                                    "V04",
                                                    V04,
                                                    (sizeof V04 /
                                                     sizeof V04[0]) - 1u);

    return ( (wide)    &&
             (escaped) &&
             (nul) );
}

/**
 * @brief An unpaired surrogate among wmain's arguments: the argument parsed
 *        holds U+FFFD for it, the parse reporting what its UTF-8 gives.
 *
 * @return whether the two parses reported the same.
 */
static bool
d_tests_arg_argv_wide_repaired(void)
{
    // BROKEN
    //   constant: an argument whose first unit is an unpaired surrogate.
    static wchar_t BROKEN[] = { (wchar_t)0xD800, L'x', L'\0' };
    // WIDE
    //   constant: the wide arguments, the program's name first.
    static wchar_t* const WIDE[] = { L"prog",
                                     L"--name",
                                     BROKEN };
    // ARGS
    //   constant: the same arguments as UTF-8, after the program's name.
    static const char* const ARGS[] = { "--name",
                                        "\xef\xbf\xbdx" };

    struct d_tests_arg_run      wide  = { 0 };
    struct d_tests_arg_run      utf8  = { 0 };
    struct d_arg_command* const first = d_tests_arg_argv_wide_begin(&wide);
    struct d_arg_command* const again = d_tests_arg_argv_wide_begin(&utf8);

    d_tests_arg_record(&wide,
                       d_arg_parser_parse_wide(&wide.parser,
                                               3,
                                               WIDE));
    (void)d_tests_arg_parse(&utf8,
                            ARGS,
                            2u);

    const bool held = ( (first != NULL)                                   &&
                        (again != NULL)                                   &&
                        (d_tests_arg_same("an unpaired surrogate, parsed",
                                          d_arg_buffer_text(
                                              &wide.transcript),
                                          utf8.transcript.data)) );

    d_tests_arg_end(&wide);
    d_tests_arg_end(&utf8);

    return d_tests_arg_argv_held(held,
                                 "an unpaired surrogate among the "
                                 "arguments");
}

/**
 * @brief Wide parses refused: d_arg_parser_parse_wide without a parser,
 *        with arguments that are not there, or with a NULL argument, first
 *        or later; d_arg_parser_parse_line_wide without a parser, or with a
 *        line that is not there.
 *
 * @param[in,out] _run  the run, whose parser each call but the first of
 *                      each kind is given.
 * @return whether every call was refused.
 */
static bool
d_tests_arg_argv_wide_refusals(
    struct d_tests_arg_run* _run
)
{
    // GAP
    //   constant: arguments with a NULL where the second should be.
    static wchar_t* const GAP[] = { L"prog",
                                    NULL };
    // HEAD
    //   constant: arguments with a NULL where the program's name should be.
    static wchar_t* const HEAD[] = { NULL,
                                     L"x" };

    const int codes[] =
    {
        d_arg_parser_parse_wide(NULL,
                                1,
                                GAP),
        d_arg_parser_parse_wide(&_run->parser,
                                1,
                                NULL),
        d_arg_parser_parse_wide(&_run->parser,
                                2,
                                GAP),
        d_arg_parser_parse_wide(&_run->parser,
                                2,
                                HEAD),
        d_arg_parser_parse_line_wide(NULL,
                                     L"a",
                                     1u,
                                     false),
        d_arg_parser_parse_line_wide(&_run->parser,
                                     NULL,
                                     1u,
                                     false)
    };
    bool      held    = true;

    // each call's status
    for (size_t index = 0u; index < (sizeof(codes) / sizeof(codes[0])); ++index)
    {
        // a status other than the refusal
        if (codes[index] != (int)D_ARG_ERROR_BAD_ARGUMENT)
        {
            printf("    refusal %lu returned %d\n",
                   (unsigned long)index,
                   codes[index]);
            held = false;
        }
    }

    return held;
}

/**
 * @brief Wide parses refused, each before the parser is touched -- the
 *        program's name of the parse before, the program's name alone,
 *        kept -- and none given, and an empty line, parsed.
 *
 * @return whether every call came to that.
 */
static bool
d_tests_arg_argv_wide_refused(void)
{
    // ALONE
    //   constant: the program's name, and no argument after it.
    static wchar_t* const ALONE[] = { L"prog" };

    struct d_tests_arg_run      run      = { 0 };
    struct d_arg_command* const app      = d_tests_arg_begin(&run);
    const int                   alone    = d_arg_parser_parse_wide(
                                               &run.parser,
                                               1,
                                               ALONE);
    const bool                  refused  = d_tests_arg_argv_wide_refusals(
                                               &run);
    const bool                  kept     =
        ( (run.parser.program != NULL)   &&
          (strcmp(run.parser.program,
                  "prog") == 0) );
    const int                   none     = d_arg_parser_parse_wide(
                                               &run.parser,
                                               0,
                                               NULL);
    const bool                  nameless = (run.parser.program == NULL);
    const int                   empty    = d_arg_parser_parse_line_wide(
                                               &run.parser,
                                               NULL,
                                               0u,
                                               true);
    const bool                  held     = ( (app != NULL)       &&
                                             (alone == D_ARG_OK) &&
                                             (refused)           &&
                                             (kept)              &&
                                             (none == D_ARG_OK)  &&
                                             (nameless)          &&
                                             (empty == D_ARG_OK) );

    d_tests_arg_end(&run);

    return d_tests_arg_argv_held(held,
                                 "wide parses refused, and none given");
}

#if (D_ENV_ARG_PARSER_COMMAND_LINE == D_ENV_ARG_PARSER_COMMAND_LINE_WINDOWS)

/**
 * @brief Whether a text is ASCII throughout, which the ANSI code page and
 *        UTF-8 spell alike.
 *
 * @param[in] _text  the text.
 * @return whether every byte is below 0x80.
 */
static bool
d_tests_arg_argv_wide_ascii(
    const char* _text
)
{
    // each byte
    for (const char* at = _text; *at != '\0'; ++at)
    {
        if ((unsigned char)*at > 0x7Fu)
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Whether recovered arguments are the C runtime's own: as many as
 *        `__argc`, and each `__argv`'s wherever that is ASCII -- elsewhere
 *        the two spell it in different encodings.
 *
 * @param[in] _got    the recovered arguments.
 * @param[in] _count  how many.
 * @return whether they are.
 */
static bool
d_tests_arg_argv_wide_runtime(
    char* const* _got,
    size_t       _count
)
{
    bool held = ( (__argc >= 0)                &&
                  ((size_t)__argc == _count) );

    // each argument the runtime holds in ASCII
    for (size_t index = 0u; (held) && (index < _count); ++index)
    {
        held = ( (!d_tests_arg_argv_wide_ascii(__argv[index])) ||
                 (strcmp(_got[index],
                         __argv[index]) == 0) );
    }

    return held;
}

#else

/**
 * @brief Where `argv` is never recovered, there is nothing to compare: the
 *        runtime's arguments are `argv` itself.
 *
 * @param[in] _got    the recovered arguments.
 * @param[in] _count  how many.
 * @return true.
 */
static bool
d_tests_arg_argv_wide_runtime(
    char* const* _got,
    size_t       _count
)
{
    (void)_got;
    (void)_count;

    return true;
}

#endif  // D_ENV_ARG_PARSER_COMMAND_LINE

/**
 * @brief The calls of d_arg_parser_ensure_utf8 that leave `argv` as it is
 *        everywhere: no parser, with arguments and without; an `argv` of no
 *        arguments, read no further than its NULL; and no `argv` at all.
 *
 * @param[in,out] _run   the run, whose parser is given.
 * @param[in]     _full  an `argv` of more arguments than the process has.
 * @return whether each call returned the `argv` it was given.
 */
static bool
d_tests_arg_argv_wide_untouched(
    struct d_tests_arg_run* _run,
    char**                  _full
)
{
    // NONE
    //   constant: an `argv` of no arguments, its NULL alone.
    static char* NONE[] = { NULL };

    return ( (d_arg_parser_ensure_utf8(NULL,
                                       _full) == _full)        &&
             (d_arg_parser_ensure_utf8(NULL,
                                       NONE) == NONE)          &&
             (d_arg_parser_ensure_utf8(&_run->parser,
                                       NONE) == NONE)          &&
             (d_arg_parser_ensure_utf8(&_run->parser,
                                       NULL) == NULL) );
}

/**
 * @brief main's arguments recovered in UTF-8 (d_arg_parser_ensure_utf8):
 *        tried with each count of arguments up to ENSURE_MAX, exactly one --
 *        the process's own -- is recovered where the system keeps the
 *        process's wide command line, as copies the parser keeps, ending in
 *        a NULL within their storage, each the runtime's own argument; for
 *        every other count, and everywhere else, `argv` itself, as the
 *        calls d_tests_arg_argv_wide_untouched makes give it.
 *
 * @return whether each came to that.
 */
static bool
d_tests_arg_argv_wide_ensured(void)
{
    // ENSURE_MAX
    //   constant: the most arguments tried, past the suite's own.
    enum
    {
        ENSURE_MAX = 64
    };
    // EMPTY
    //   constant: the text of every argument tried.
    static char EMPTY[] = "";

    char*                  fake[ENSURE_MAX + 2] = { NULL };
    struct d_tests_arg_run run                  = { 0 };
    const bool             begun                =
        (d_tests_arg_begin(&run) != NULL);
    const bool             windows              =
        (D_ENV_ARG_PARSER_COMMAND_LINE ==
         D_ENV_ARG_PARSER_COMMAND_LINE_WINDOWS);
    size_t                 recovered            = 0u;
    bool                   held                 = true;

    // each count of arguments, every argument empty, a NULL after the last
    for (size_t count = 0u; count <= (size_t)ENSURE_MAX; ++count)
    {
        char** const got = d_arg_parser_ensure_utf8(&run.parser,
                                                    fake);

        // a recovery: as many copies as arguments, then NULL in their room
        if (got != fake)
        {
            recovered++;
            held = ( (got[count] == NULL)                       &&
                     (run.parser.utf8_argv.count == count)      &&
                     (run.parser.utf8_argv.capacity > count)    &&
                     (d_tests_arg_argv_wide_runtime(got,
                                                    count)) ) && held;
        }

        fake[count] = EMPTY;
    }

    held = ( (held)                                  &&
             (begun)                                 &&
             (recovered == ((windows) ? 1u : 0u))    &&
             (d_tests_arg_argv_wide_untouched(&run,
                                              fake)) );
    d_tests_arg_end(&run);

    return d_tests_arg_argv_held(held,
                                 "main's arguments recovered in UTF-8");
}

/*
d_tests_arg_argv_wide_parse
  Wide arguments parsed (d_arg_parser_parse_wide,
d_arg_parser_parse_line_wide) as CLI11's wide parses give them, and main's
arguments recovered in UTF-8 (d_arg_parser_ensure_utf8).
  Tests the following:
  - wmain's arguments, converted and parsed, the program's name kept
  - wide lines with the program's name, converted and parsed; an escape
    CLI11 refuses, its ParseError; NUL bytes, U+0000 and from an escape,
    each word holding them kept whole
  - an unpaired surrogate in an argument, U+FFFD, as its UTF-8 parses
  - wide parses refused without a parser, arguments, a line, or with a NULL
    argument, first or later, the parse before untouched; the program's
    name alone, none given and an empty line parsed
  - main's arguments recovered where the system keeps the wide command line,
    for the process's own count alone, the runtime's own arguments, a NULL
    in their storage after them; `argv` itself elsewhere, read no further
    than its NULL
*/
bool
d_tests_arg_argv_wide_parse(void)
{
    struct d_arg_buffer written  = D_ARG_BUFFER_INIT;
    const bool          args     = d_tests_arg_argv_wide_args(&written);
    const bool          lines    = d_tests_arg_argv_wide_lines(&written);
    const bool          same     = d_tests_arg_same("WIDE_PARSE",
                                                    d_arg_buffer_text(
                                                        &written),
                                                    WIDE_PARSE);
    const bool          kept     = (written.failed == 0);
    const bool          repaired = d_tests_arg_argv_wide_repaired();
    const bool          refused  = d_tests_arg_argv_wide_refused();
    const bool          ensured  = d_tests_arg_argv_wide_ensured();

    d_arg_buffer_release(&written);

    return ( (args)     &&
             (lines)    &&
             (same)     &&
             (kept)     &&
             (repaired) &&
             (refused)  &&
             (ensured) );
}

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

//==============================================================================
// 28. THE SUITE
//==============================================================================

// d_tests_arg_text
//   constant: the suite, in the header's order.
const struct d_tests_section d_tests_arg_text[] =
{
    { "arg_argv_split_posix",
      d_tests_arg_argv_split_posix },
    { "arg_argv_split_windows",
      d_tests_arg_argv_split_windows },
    { "arg_argv_split_windows_breaks",
      d_tests_arg_argv_split_windows_breaks },
    { "arg_argv_split_windows_names",
      d_tests_arg_argv_split_windows_names },
    { "arg_argv_split_windows_names_breaks",
      d_tests_arg_argv_split_windows_names_breaks },
    { "arg_argv_split_cli11",
      d_tests_arg_argv_split_cli11 },
    { "arg_argv_split_cli11_windows",
      d_tests_arg_argv_split_cli11_windows },
    { "arg_argv_split_gnu",
      d_tests_arg_argv_split_gnu },
    { "arg_argv_split_flags",
      d_tests_arg_argv_split_flags },
    { "arg_argv_split_own",
      d_tests_arg_argv_split_own },
    { "arg_argv_split_refusals",
      d_tests_arg_argv_split_refusals },
    { "arg_argv_quote_posix",
      d_tests_arg_argv_quote_posix },
    { "arg_argv_quote_windows",
      d_tests_arg_argv_quote_windows },
    { "arg_argv_quote_cli11",
      d_tests_arg_argv_quote_cli11 },
    { "arg_argv_quote_gnu",
      d_tests_arg_argv_quote_gnu },
    { "arg_argv_quote_refusals",
      d_tests_arg_argv_quote_refusals },
    { "arg_argv_program_names",
      d_tests_arg_argv_program_names },
    { "arg_argv_program_refusals",
      d_tests_arg_argv_program_refusals },
#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)
    { "arg_argv_response_read",
      d_tests_arg_argv_response_read },
#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES
#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)
    { "arg_argv_response_expand_gnu",
      d_tests_arg_argv_response_expand_gnu },
#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES
#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)
    { "arg_argv_response_expand_windows",
      d_tests_arg_argv_response_expand_windows },
#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES
#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)
    { "arg_argv_response_expand_own",
      d_tests_arg_argv_response_expand_own },
#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES
#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)
    { "arg_argv_response_expand_entries",
      d_tests_arg_argv_response_expand_entries },
#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES
#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)
    { "arg_argv_response_refusals",
      d_tests_arg_argv_response_refusals },
#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES
#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)
    { "arg_argv_wide_text",
      d_tests_arg_argv_wide_text },
#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV
#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)
    { "arg_argv_wide_back",
      d_tests_arg_argv_wide_back },
#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV
#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)
    { "arg_argv_wide_parse",
      d_tests_arg_argv_wide_parse },
#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV
};

// d_tests_arg_text_count
//   constant: how many sections the table above holds.
const size_t d_tests_arg_text_count =
    sizeof(d_tests_arg_text) / sizeof(d_tests_arg_text[0]);
