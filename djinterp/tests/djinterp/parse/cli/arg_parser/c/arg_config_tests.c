/*******************************************************************************
* djinterp [parse]                                            arg_config_tests.c
*
* The suite of configuration files, declared in arg_config_tests.h.
*   The reading sections are tables of texts, each read under the settings
* named beside it and written as the reference writes CLI11's items. The
* parse's sections are case tables (arg_tests_support.h) whose files are
* made in a scratch directory; the writing sections, tables of steps, each
* parsing and then writing the root's configuration in each mode. The
* cases keep the tags of the probes they came from: T01-T78 the reader's
* corpus (probes/config/gen_cfg.py), E01-E57 the parse probe's, W01-W22 the
* writing probe's; the tags after those, the reference's own.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_config_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./arg_config_tests.h"  // corresponding header
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <stdio.h>    // FILE, fopen, fclose, printf
#include <stdlib.h>   // malloc, free
#include <string.h>   // memchr, memcmp, memcpy, strcmp
// djinterp
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // the knobs
#include "../../../../../../inc/djinterp/env/env.h"  // D_ENV_OS_ID, D_ENV_IS_OS_WINDOWS
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // commands, their settings
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // statuses, buffers, modes
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_config.h"  // formats, items, reading, writing
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // options, their settings
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // the parser, text parsed
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator.h"  // CLI11's Range
#include "../../../c/tests_section.h"  // d_tests_section
#include "./arg_tests_files.h"  // scratch directories
#include "./arg_tests_support.h"  // runs, cases, steps, files
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// VALUES
//   constant: what the cases of d_tests_arg_config_values come to, as the
// reference writes them with CLI11.
static const char VALUES[] =
    "T01 [a=1\\nb = 2\\nc= three\\n]\n"
    "toml OK 3\n"
    "  P#0= N=a M=0 I#1=1\n"
    "  P#0= N=b M=0 I#1=2\n"
    "  P#0= N=c M=0 I#1=three\n"
    "ini OK 3\n"
    "  P#0= N=a M=0 I#1=1\n"
    "  P#0= N=b M=0 I#1=2\n"
    "  P#0= N=c M=0 I#1=three\n"
    "T02 [flag\\nab\\nabc\\nx\\n]\n"
    "toml OK 2\n"
    "  P#0= N=flag M=0 I#1=true\n"
    "  P#0= N=abc M=0 I#1=true\n"
    "ini OK 2\n"
    "  P#0= N=flag M=0 I#1=true\n"
    "  P#0= N=abc M=0 I#1=true\n"
    "T03 [list = 1, 2, 3\\nspaced = a b c\\nmixed = a, b c\\n]\n"
    "toml OK 3\n"
    "  P#0= N=list M=0 I#3=1|2|3\n"
    "  P#0= N=spaced M=0 I#3=a|b|c\n"
    "  P#0= N=mixed M=0 I#2=a|b c\n"
    "ini OK 3\n"
    "  P#0= N=list M=0 I#3=1|2|3\n"
    "  P#0= N=spaced M=0 I#3=a|b|c\n"
    "  P#0= N=mixed M=0 I#2=a|b c\n"
    "T04 [q = \"a b\"\\nl = 'c d'\\nb = `e f`\\n]\n"
    "toml OK 3\n"
    "  P#0= N=q M=0 I#1=a b\n"
    "  P#0= N=l M=0 I#1=c d\n"
    "  P#0= N=b M=0 I#1=e f\n"
    "ini OK 3\n"
    "  P#0= N=q M=0 I#1=a b\n"
    "  P#0= N=l M=0 I#1=c d\n"
    "  P#0= N=b M=0 I#1=e f\n"
    "T05 [e = \"\"\\nf = ''\\ng =\\nh = \\n]\n"
    "toml OK 4\n"
    "  P#0= N=e M=0 I#1=\n"
    "  P#0= N=f M=0 I#1=\n"
    "  P#0= N=g M=0 I#1=\n"
    "  P#0= N=h M=0 I#1=\n"
    "ini OK 4\n"
    "  P#0= N=e M=0 I#1=\n"
    "  P#0= N=f M=0 I#1=\n"
    "  P#0= N=g M=0 I#1=\n"
    "  P#0= N=h M=0 I#1=\n"
    "T06 [# comment\\n; also\\na = 1 # trailing\\nb = \"x # y\" # z\\nc = "
    "'p;q' ; r\\n]\n"
    "toml OK 3\n"
    "  P#0= N=a M=0 I#1=1\n"
    "  P#0= N=b M=0 I#1=x # y\n"
    "  P#0= N=c M=0 I#3=p;q|;|r\n"
    "ini OK 3\n"
    "  P#0= N=a M=0 I#3=1|#|trailing\n"
    "  P#0= N=b M=0 I#3=x # y|#|z\n"
    "  P#0= N=c M=0 I#1=p;q\n"
    "T07 [a = # only a comment\\nb=#\\n]\n"
    "toml OK 2\n"
    "  P#0= N=a M=0 I#1=\n"
    "  P#0= N=b M=0 I#1=\n"
    "ini OK 2\n"
    "  P#0= N=a M=0 I#4=#|only|a|comment\n"
    "  P#0= N=b M=0 I#1=#\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// SECTIONS
//   constant: what the cases of d_tests_arg_config_sections come to, as the
// reference writes them with CLI11.
static const char SECTIONS[] =
    "T08 [[sub]\\nx=1\\n[sub.deeper]\\ny=2\\n[other]\\nz=3\\n]\n"
    "toml OK 9\n"
    "  P#1=sub N=++ M=0 I#0=\n"
    "  P#1=sub N=x M=0 I#1=1\n"
    "  P#2=sub|deeper N=++ M=0 I#0=\n"
    "  P#2=sub|deeper N=y M=0 I#1=2\n"
    "  P#2=sub|deeper N=-- M=0 I#0=\n"
    "  P#1=sub N=-- M=0 I#0=\n"
    "  P#1=other N=++ M=0 I#0=\n"
    "  P#1=other N=z M=0 I#1=3\n"
    "  P#1=other N=-- M=0 I#0=\n"
    "ini OK 9\n"
    "  P#1=sub N=++ M=0 I#0=\n"
    "  P#1=sub N=x M=0 I#1=1\n"
    "  P#2=sub|deeper N=++ M=0 I#0=\n"
    "  P#2=sub|deeper N=y M=0 I#1=2\n"
    "  P#2=sub|deeper N=-- M=0 I#0=\n"
    "  P#1=sub N=-- M=0 I#0=\n"
    "  P#1=other N=++ M=0 I#0=\n"
    "  P#1=other N=z M=0 I#1=3\n"
    "  P#1=other N=-- M=0 I#0=\n"
    "T09 [[a.b.c]\\nv=1\\n[a.b.d]\\nw=2\\n[a.e]\\nu=3\\n[f]\\nt=4\\n]\n"
    "toml OK 16\n"
    "  P#1=a N=++ M=0 I#0=\n"
    "  P#2=a|b N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=v M=0 I#1=1\n"
    "  P#3=a|b|c N=-- M=0 I#0=\n"
    "  P#3=a|b|d N=++ M=0 I#0=\n"
    "  P#3=a|b|d N=w M=0 I#1=2\n"
    "  P#3=a|b|d N=-- M=0 I#0=\n"
    "  P#2=a|b N=-- M=0 I#0=\n"
    "  P#2=a|e N=++ M=0 I#0=\n"
    "  P#2=a|e N=u M=0 I#1=3\n"
    "  P#2=a|e N=-- M=0 I#0=\n"
    "  P#1=a N=-- M=0 I#0=\n"
    "  P#1=f N=++ M=0 I#0=\n"
    "  P#1=f N=t M=0 I#1=4\n"
    "  P#1=f N=-- M=0 I#0=\n"
    "ini OK 16\n"
    "  P#1=a N=++ M=0 I#0=\n"
    "  P#2=a|b N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=v M=0 I#1=1\n"
    "  P#3=a|b|c N=-- M=0 I#0=\n"
    "  P#3=a|b|d N=++ M=0 I#0=\n"
    "  P#3=a|b|d N=w M=0 I#1=2\n"
    "  P#3=a|b|d N=-- M=0 I#0=\n"
    "  P#2=a|b N=-- M=0 I#0=\n"
    "  P#2=a|e N=++ M=0 I#0=\n"
    "  P#2=a|e N=u M=0 I#1=3\n"
    "  P#2=a|e N=-- M=0 I#0=\n"
    "  P#1=a N=-- M=0 I#0=\n"
    "  P#1=f N=++ M=0 I#0=\n"
    "  P#1=f N=t M=0 I#1=4\n"
    "  P#1=f N=-- M=0 I#0=\n"
    "T10 [[a]\\n[a.b]\\n[a.b.c]\\nx=1\\n[a]\\ny=2\\n]\n"
    "toml OK 10\n"
    "  P#1=a N=++ M=0 I#0=\n"
    "  P#2=a|b N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=x M=0 I#1=1\n"
    "  P#3=a|b|c N=-- M=0 I#0=\n"
    "  P#2=a|b N=-- M=0 I#0=\n"
    "  P#1=a N=-- M=0 I#0=\n"
    "  P#1=a N=++ M=0 I#0=\n"
    "  P#1=a N=y M=0 I#1=2\n"
    "  P#1=a N=-- M=0 I#0=\n"
    "ini OK 10\n"
    "  P#1=a N=++ M=0 I#0=\n"
    "  P#2=a|b N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=x M=0 I#1=1\n"
    "  P#3=a|b|c N=-- M=0 I#0=\n"
    "  P#2=a|b N=-- M=0 I#0=\n"
    "  P#1=a N=-- M=0 I#0=\n"
    "  P#1=a N=++ M=0 I#0=\n"
    "  P#1=a N=y M=0 I#1=2\n"
    "  P#1=a N=-- M=0 I#0=\n"
    "T11 [[[arr]]\\nx=1\\n[[arr]]\\nx=2\\n]\n"
    "toml OK 6\n"
    "  P#1=arr N=++ M=0 I#0=\n"
    "  P#1=arr N=x M=0 I#1=1\n"
    "  P#1=arr N=-- M=0 I#0=\n"
    "  P#1=arr N=++ M=0 I#0=\n"
    "  P#1=arr N=x M=0 I#1=2\n"
    "  P#1=arr N=-- M=0 I#0=\n"
    "ini OK 6\n"
    "  P#1=arr N=++ M=0 I#0=\n"
    "  P#1=arr N=x M=0 I#1=1\n"
    "  P#1=arr N=-- M=0 I#0=\n"
    "  P#1=arr N=++ M=0 I#0=\n"
    "  P#1=arr N=x M=0 I#1=2\n"
    "  P#1=arr N=-- M=0 I#0=\n"
    "T12 "
    "[[default]\\na=1\\n[DEFAULT]\\nb=2\\n[sub]\\nc=3\\n[Default]\\nd=4\\n]\n"
    "toml OK 6\n"
    "  P#0= N=a M=0 I#1=1\n"
    "  P#0= N=b M=0 I#1=2\n"
    "  P#1=sub N=++ M=0 I#0=\n"
    "  P#1=sub N=c M=0 I#1=3\n"
    "  P#1=sub N=-- M=0 I#0=\n"
    "  P#0= N=d M=0 I#1=4\n"
    "ini OK 6\n"
    "  P#0= N=a M=0 I#1=1\n"
    "  P#0= N=b M=0 I#1=2\n"
    "  P#1=sub N=++ M=0 I#0=\n"
    "  P#1=sub N=c M=0 I#1=3\n"
    "  P#1=sub N=-- M=0 I#0=\n"
    "  P#0= N=d M=0 I#1=4\n"
    "T13 [[sub] # comment\\nx=1\\n[\"q.s\"] # c2\\ny=2\\n['li.t'] ; "
    "c3\\nz=3\\n]\n"
    "toml OK 8\n"
    "  P#1=sub N=++ M=0 I#0=\n"
    "  P#1=sub N=x M=0 I#1=1\n"
    "  P#1=sub N=-- M=0 I#0=\n"
    "  P#1=q.s N=++ M=0 I#0=\n"
    "  P#1=q.s N=y M=0 I#1=2\n"
    "  P#2=q.s|['li.t'] N=; c3 M=0 I#1=true\n"
    "  P#1=q.s N=z M=0 I#1=3\n"
    "  P#1=q.s N=-- M=0 I#0=\n"
    "ini OK 7\n"
    "  P#0= N=[sub] # comment M=0 I#1=true\n"
    "  P#0= N=x M=0 I#1=1\n"
    "  P#1=[\"q.s\"] N=# c2 M=0 I#1=true\n"
    "  P#0= N=y M=0 I#1=2\n"
    "  P#1=li.t N=++ M=0 I#0=\n"
    "  P#1=li.t N=z M=0 I#1=3\n"
    "  P#1=li.t N=-- M=0 I#0=\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// HEADERS
//   constant: what the cases of d_tests_arg_config_headers come to, as the
// reference writes them with CLI11.
static const char HEADERS[] =
    "T14 [[sub # not closed\\nx=1\\n]\n"
    "toml OK 2\n"
    "  P#0= N=[sub M=0 I#1=true\n"
    "  P#0= N=x M=0 I#1=1\n"
    "ini OK 2\n"
    "  P#0= N=[sub # not closed M=0 I#1=true\n"
    "  P#0= N=x M=0 I#1=1\n"
    "T15 [[ spaced ]\\nx=1\\n[a. b]\\ny=2\\n[a..b]\\nz=3\\n]\n"
    "toml OK 13\n"
    "  P#1= spaced  N=++ M=0 I#0=\n"
    "  P#1= spaced  N=x M=0 I#1=1\n"
    "  P#1= spaced  N=-- M=0 I#0=\n"
    "  P#1=a N=++ M=0 I#0=\n"
    "  P#2=a|b N=++ M=0 I#0=\n"
    "  P#2=a|b N=y M=0 I#1=2\n"
    "  P#2=a|b N=-- M=0 I#0=\n"
    "  P#2=a| N=++ M=0 I#0=\n"
    "  P#3=a||b N=++ M=0 I#0=\n"
    "  P#3=a||b N=z M=0 I#1=3\n"
    "  P#3=a||b N=-- M=0 I#0=\n"
    "  P#2=a| N=-- M=0 I#0=\n"
    "  P#1=a N=-- M=0 I#0=\n"
    "ini OK 13\n"
    "  P#1= spaced  N=++ M=0 I#0=\n"
    "  P#1= spaced  N=x M=0 I#1=1\n"
    "  P#1= spaced  N=-- M=0 I#0=\n"
    "  P#1=a N=++ M=0 I#0=\n"
    "  P#2=a|b N=++ M=0 I#0=\n"
    "  P#2=a|b N=y M=0 I#1=2\n"
    "  P#2=a|b N=-- M=0 I#0=\n"
    "  P#2=a| N=++ M=0 I#0=\n"
    "  P#3=a||b N=++ M=0 I#0=\n"
    "  P#3=a||b N=z M=0 I#1=3\n"
    "  P#3=a||b N=-- M=0 I#0=\n"
    "  P#2=a| N=-- M=0 I#0=\n"
    "  P#1=a N=-- M=0 I#0=\n"
    "T16 [[]\\n[[]]\\nx=1\\n]\n"
    "toml OK 3\n"
    "  P#1= N=++ M=0 I#0=\n"
    "  P#1= N=x M=0 I#1=1\n"
    "  P#1= N=-- M=0 I#0=\n"
    "ini OK 3\n"
    "  P#1= N=++ M=0 I#0=\n"
    "  P#1= N=x M=0 I#1=1\n"
    "  P#1= N=-- M=0 I#0=\n"
    "T17 [[\"esc\\\\tsec\"]\\nx=1\\n]\n"
    "toml OK 3\n"
    "  P#1=esc\\tsec N=++ M=0 I#0=\n"
    "  P#1=esc\\tsec N=x M=0 I#1=1\n"
    "  P#1=esc\\tsec N=-- M=0 I#0=\n"
    "ini OK 3\n"
    "  P#1=esc\\tsec N=++ M=0 I#0=\n"
    "  P#1=esc\\tsec N=x M=0 I#1=1\n"
    "  P#1=esc\\tsec N=-- M=0 I#0=\n"
    "T18 [[\"bad\\\\qsec\"]\\nx=1\\n]\n"
    "toml ERR ParseError 111: unrecognized escape sequence \\\\q in "
    "bad\\\\qsec\n"
    "ini ERR ParseError 111: unrecognized escape sequence \\\\q in "
    "bad\\\\qsec\n"
    "T19 [x=0\\n[s1]\\n[s2]\\n[s3.t]\\n]\n"
    "toml OK 9\n"
    "  P#0= N=x M=0 I#1=0\n"
    "  P#1=s1 N=++ M=0 I#0=\n"
    "  P#1=s1 N=-- M=0 I#0=\n"
    "  P#1=s2 N=++ M=0 I#0=\n"
    "  P#1=s2 N=-- M=0 I#0=\n"
    "  P#1=s3 N=++ M=0 I#0=\n"
    "  P#2=s3|t N=++ M=0 I#0=\n"
    "  P#2=s3|t N=-- M=0 I#0=\n"
    "  P#1=s3 N=-- M=0 I#0=\n"
    "ini OK 9\n"
    "  P#0= N=x M=0 I#1=0\n"
    "  P#1=s1 N=++ M=0 I#0=\n"
    "  P#1=s1 N=-- M=0 I#0=\n"
    "  P#1=s2 N=++ M=0 I#0=\n"
    "  P#1=s2 N=-- M=0 I#0=\n"
    "  P#1=s3 N=++ M=0 I#0=\n"
    "  P#2=s3|t N=++ M=0 I#0=\n"
    "  P#2=s3|t N=-- M=0 I#0=\n"
    "  P#1=s3 N=-- M=0 I#0=\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// CHOSEN
//   constant: what the cases of d_tests_arg_config_chosen come to, as the
// reference writes them with CLI11.
static const char CHOSEN[] =
    "T01 [a=1\\nb = 2\\nc= three\\n]\n"
    "toml section sub OK 0\n"
    "T08 [[sub]\\nx=1\\n[sub.deeper]\\ny=2\\n[other]\\nz=3\\n]\n"
    "toml section sub OK 8\n"
    "  P#1=sub N=++ M=0 I#0=\n"
    "  P#0= N=x M=0 I#1=1\n"
    "  P#2=sub|deeper N=++ M=0 I#0=\n"
    "  P#1=deeper N=y M=0 I#1=2\n"
    "  P#2=sub|deeper N=-- M=0 I#0=\n"
    "  P#1=sub N=-- M=0 I#0=\n"
    "  P#1=other N=++ M=0 I#0=\n"
    "  P#1=other N=-- M=0 I#0=\n"
    "T09 [[a.b.c]\\nv=1\\n[a.b.d]\\nw=2\\n[a.e]\\nu=3\\n[f]\\nt=4\\n]\n"
    "toml section a 1 OK 12\n"
    "  P#1=a N=++ M=0 I#0=\n"
    "  P#2=a|b N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=-- M=0 I#0=\n"
    "  P#3=a|b|d N=++ M=0 I#0=\n"
    "  P#3=a|b|d N=-- M=0 I#0=\n"
    "  P#2=a|b N=-- M=0 I#0=\n"
    "  P#2=a|e N=++ M=0 I#0=\n"
    "  P#2=a|e N=-- M=0 I#0=\n"
    "  P#1=a N=-- M=0 I#0=\n"
    "  P#1=f N=++ M=0 I#0=\n"
    "  P#1=f N=-- M=0 I#0=\n"
    "T10 [[a]\\n[a.b]\\n[a.b.c]\\nx=1\\n[a]\\ny=2\\n]\n"
    "toml section a 1 OK 8\n"
    "  P#1=a N=++ M=0 I#0=\n"
    "  P#2=a|b N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=-- M=0 I#0=\n"
    "  P#2=a|b N=-- M=0 I#0=\n"
    "  P#1=a N=-- M=0 I#0=\n"
    "  P#1=a N=++ M=0 I#0=\n"
    "  P#1=a N=-- M=0 I#0=\n"
    "T11 [[[arr]]\\nx=1\\n[[arr]]\\nx=2\\n]\n"
    "toml section arr 0 OK 5\n"
    "  P#1=arr N=++ M=0 I#0=\n"
    "  P#0= N=x M=0 I#1=1\n"
    "  P#1=arr N=-- M=0 I#0=\n"
    "  P#1=arr N=++ M=0 I#0=\n"
    "  P#1=arr N=-- M=0 I#0=\n"
    "toml section arr 1 OK 5\n"
    "  P#1=arr N=++ M=0 I#0=\n"
    "  P#1=arr N=-- M=0 I#0=\n"
    "  P#1=arr N=++ M=0 I#0=\n"
    "  P#0= N=x M=0 I#1=2\n"
    "  P#1=arr N=-- M=0 I#0=\n"
    "toml section sub OK 4\n"
    "  P#1=arr N=++ M=0 I#0=\n"
    "  P#1=arr N=-- M=0 I#0=\n"
    "  P#1=arr N=++ M=0 I#0=\n"
    "  P#1=arr N=-- M=0 I#0=\n"
    "T12 "
    "[[default]\\na=1\\n[DEFAULT]\\nb=2\\n[sub]\\nc=3\\n[Default]\\nd=4\\n]\n"
    "toml section sub OK 3\n"
    "  P#1=sub N=++ M=0 I#0=\n"
    "  P#0= N=c M=0 I#1=3\n"
    "  P#1=sub N=-- M=0 I#0=\n"
    "T13 [[sub] # comment\\nx=1\\n[\"q.s\"] # c2\\ny=2\\n['li.t'] ; "
    "c3\\nz=3\\n]\n"
    "toml section sub OK 5\n"
    "  P#1=sub N=++ M=0 I#0=\n"
    "  P#0= N=x M=0 I#1=1\n"
    "  P#1=sub N=-- M=0 I#0=\n"
    "  P#1=q.s N=++ M=0 I#0=\n"
    "  P#1=q.s N=-- M=0 I#0=\n"
    "T21 [[sec]\\nsub.opt = 5\\n\"quoted.name\" = 6\\n]\n"
    "toml section sub OK 2\n"
    "  P#1=sec N=++ M=0 I#0=\n"
    "  P#1=sec N=-- M=0 I#0=\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// LAYERS
//   constant: what the cases of d_tests_arg_config_layers come to, as the
// reference writes them with CLI11.
static const char LAYERS[] =
    "T08 [[sub]\\nx=1\\n[sub.deeper]\\ny=2\\n[other]\\nz=3\\n]\n"
    "toml layers 0 OK 6\n"
    "  P#1=sub N=++ M=0 I#0=\n"
    "  P#2=sub|deeper N=++ M=0 I#0=\n"
    "  P#2=sub|deeper N=-- M=0 I#0=\n"
    "  P#1=sub N=-- M=0 I#0=\n"
    "  P#1=other N=++ M=0 I#0=\n"
    "  P#1=other N=-- M=0 I#0=\n"
    "toml layers 1 OK 8\n"
    "  P#1=sub N=++ M=0 I#0=\n"
    "  P#1=sub N=x M=0 I#1=1\n"
    "  P#2=sub|deeper N=++ M=0 I#0=\n"
    "  P#2=sub|deeper N=-- M=0 I#0=\n"
    "  P#1=sub N=-- M=0 I#0=\n"
    "  P#1=other N=++ M=0 I#0=\n"
    "  P#1=other N=z M=0 I#1=3\n"
    "  P#1=other N=-- M=0 I#0=\n"
    "T09 [[a.b.c]\\nv=1\\n[a.b.d]\\nw=2\\n[a.e]\\nu=3\\n[f]\\nt=4\\n]\n"
    "toml layers 1 OK 13\n"
    "  P#1=a N=++ M=0 I#0=\n"
    "  P#2=a|b N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=-- M=0 I#0=\n"
    "  P#3=a|b|d N=++ M=0 I#0=\n"
    "  P#3=a|b|d N=-- M=0 I#0=\n"
    "  P#2=a|b N=-- M=0 I#0=\n"
    "  P#2=a|e N=++ M=0 I#0=\n"
    "  P#2=a|e N=-- M=0 I#0=\n"
    "  P#1=a N=-- M=0 I#0=\n"
    "  P#1=f N=++ M=0 I#0=\n"
    "  P#1=f N=t M=0 I#1=4\n"
    "  P#1=f N=-- M=0 I#0=\n"
    "T10 [[a]\\n[a.b]\\n[a.b.c]\\nx=1\\n[a]\\ny=2\\n]\n"
    "toml layers 1 OK 9\n"
    "  P#1=a N=++ M=0 I#0=\n"
    "  P#2=a|b N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=-- M=0 I#0=\n"
    "  P#2=a|b N=-- M=0 I#0=\n"
    "  P#1=a N=-- M=0 I#0=\n"
    "  P#1=a N=++ M=0 I#0=\n"
    "  P#1=a N=y M=0 I#1=2\n"
    "  P#1=a N=-- M=0 I#0=\n"
    "T13 [[sub] # comment\\nx=1\\n[\"q.s\"] # c2\\ny=2\\n['li.t'] ; "
    "c3\\nz=3\\n]\n"
    "toml layers 1 OK 7\n"
    "  P#1=sub N=++ M=0 I#0=\n"
    "  P#1=sub N=x M=0 I#1=1\n"
    "  P#1=sub N=-- M=0 I#0=\n"
    "  P#1=q.s N=++ M=0 I#0=\n"
    "  P#1=q.s N=y M=0 I#1=2\n"
    "  P#1=q.s N=z M=0 I#1=3\n"
    "  P#1=q.s N=-- M=0 I#0=\n"
    "ini layers 1 OK 7\n"
    "  P#0= N=[sub] # comment M=0 I#1=true\n"
    "  P#0= N=x M=0 I#1=1\n"
    "  P#1=[\"q.s\"] N=# c2 M=0 I#1=true\n"
    "  P#0= N=y M=0 I#1=2\n"
    "  P#1=li.t N=++ M=0 I#0=\n"
    "  P#1=li.t N=z M=0 I#1=3\n"
    "  P#1=li.t N=-- M=0 I#0=\n"
    "T20 [a.b = 1\\na.b.c = 2\\n\"a.b\".c = 3\\n'x.y' = 4\\n]\n"
    "toml layers 0 OK 1\n"
    "  P#0= N=x.y M=0 I#1=4\n"
    "toml layers 1 OK 3\n"
    "  P#1=a N=b M=0 I#1=1\n"
    "  P#1=a.b N=c M=0 I#1=3\n"
    "  P#0= N=x.y M=0 I#1=4\n"
    "T68 [a.\"b c\".d = 1\\n]\n"
    "toml layers 1 OK 0\n"
    "T84 [[a.b.c.d]\\nx=1\\na.b.c.d.e = 2\\n[a]\\nb.c = 3\\n]\n"
    "toml OK 13\n"
    "  P#1=a N=++ M=0 I#0=\n"
    "  P#2=a|b N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=++ M=0 I#0=\n"
    "  P#4=a|b|c|d N=++ M=0 I#0=\n"
    "  P#4=a|b|c|d N=x M=0 I#1=1\n"
    "  P#8=a|b|c|d|a|b|c|d N=e M=0 I#1=2\n"
    "  P#4=a|b|c|d N=-- M=0 I#0=\n"
    "  P#3=a|b|c N=-- M=0 I#0=\n"
    "  P#2=a|b N=-- M=0 I#0=\n"
    "  P#1=a N=-- M=0 I#0=\n"
    "  P#1=a N=++ M=0 I#0=\n"
    "  P#2=a|b N=c M=0 I#1=3\n"
    "  P#1=a N=-- M=0 I#0=\n"
    "toml layers 1 OK 10\n"
    "  P#1=a N=++ M=0 I#0=\n"
    "  P#2=a|b N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=++ M=0 I#0=\n"
    "  P#4=a|b|c|d N=++ M=0 I#0=\n"
    "  P#4=a|b|c|d N=-- M=0 I#0=\n"
    "  P#3=a|b|c N=-- M=0 I#0=\n"
    "  P#2=a|b N=-- M=0 I#0=\n"
    "  P#1=a N=-- M=0 I#0=\n"
    "  P#1=a N=++ M=0 I#0=\n"
    "  P#1=a N=-- M=0 I#0=\n"
    "toml layers 2 OK 11\n"
    "  P#1=a N=++ M=0 I#0=\n"
    "  P#2=a|b N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=++ M=0 I#0=\n"
    "  P#4=a|b|c|d N=++ M=0 I#0=\n"
    "  P#4=a|b|c|d N=-- M=0 I#0=\n"
    "  P#3=a|b|c N=-- M=0 I#0=\n"
    "  P#2=a|b N=-- M=0 I#0=\n"
    "  P#1=a N=-- M=0 I#0=\n"
    "  P#1=a N=++ M=0 I#0=\n"
    "  P#2=a|b N=c M=0 I#1=3\n"
    "  P#1=a N=-- M=0 I#0=\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// NAMES
//   constant: what the cases of d_tests_arg_config_names come to, as the
// reference writes them with CLI11.
static const char NAMES[] =
    "T20 [a.b = 1\\na.b.c = 2\\n\"a.b\".c = 3\\n'x.y' = 4\\n]\n"
    "toml OK 4\n"
    "  P#1=a N=b M=0 I#1=1\n"
    "  P#2=a|b N=c M=0 I#1=2\n"
    "  P#1=a.b N=c M=0 I#1=3\n"
    "  P#0= N=x.y M=0 I#1=4\n"
    "ini OK 4\n"
    "  P#1=a N=b M=0 I#1=1\n"
    "  P#2=a|b N=c M=0 I#1=2\n"
    "  P#1=a.b N=c M=0 I#1=3\n"
    "  P#0= N=x.y M=0 I#1=4\n"
    "T21 [[sec]\\nsub.opt = 5\\n\"quoted.name\" = 6\\n]\n"
    "toml OK 4\n"
    "  P#1=sec N=++ M=0 I#0=\n"
    "  P#2=sec|sub N=opt M=0 I#1=5\n"
    "  P#1=sec N=quoted.name M=0 I#1=6\n"
    "  P#1=sec N=-- M=0 I#0=\n"
    "ini OK 4\n"
    "  P#1=sec N=++ M=0 I#0=\n"
    "  P#2=sec|sub N=opt M=0 I#1=5\n"
    "  P#1=sec N=quoted.name M=0 I#1=6\n"
    "  P#1=sec N=-- M=0 I#0=\n"
    "T22 [\"nm\\\\u00e9\" = 1\\n\"bad\\\\x\" = 2\\n]\n"
    "toml ERR ParseError 111: unrecognized escape sequence \\\\x in bad\\\\x\n"
    "ini ERR ParseError 111: unrecognized escape sequence \\\\x in bad\\\\x\n"
    "T23 [\"bad\\\\qname\".c = 1\\n]\n"
    "toml ERR ParseError 111: unrecognized escape sequence \\\\q in "
    "bad\\\\qname\n"
    "ini ERR ParseError 111: unrecognized escape sequence \\\\q in "
    "bad\\\\qname\n"
    "T68 [a.\"b c\".d = 1\\n]\n"
    "toml OK 1\n"
    "  P#2=a|b c N=d M=0 I#1=1\n"
    "ini OK 1\n"
    "  P#2=a|b c N=d M=0 I#1=1\n"
    "T78 [\"a b\" c = 1\\n]\n"
    "toml OK 1\n"
    "  P#0= N=\"a b\" c M=0 I#1=1\n"
    "ini OK 1\n"
    "  P#0= N=\"a b\" c M=0 I#1=1\n"
    "T08 [[sub]\\nx=1\\n[sub.deeper]\\ny=2\\n[other]\\nz=3\\n]\n"
    "toml no parents OK 9\n"
    "  P#1=sub N=++ M=0 I#0=\n"
    "  P#1=sub N=x M=0 I#1=1\n"
    "  P#1=sub N=-- M=0 I#0=\n"
    "  P#1=sub.deeper N=++ M=0 I#0=\n"
    "  P#1=sub.deeper N=y M=0 I#1=2\n"
    "  P#1=sub.deeper N=-- M=0 I#0=\n"
    "  P#1=other N=++ M=0 I#0=\n"
    "  P#1=other N=z M=0 I#1=3\n"
    "  P#1=other N=-- M=0 I#0=\n"
    "T13 [[sub] # comment\\nx=1\\n[\"q.s\"] # c2\\ny=2\\n['li.t'] ; "
    "c3\\nz=3\\n]\n"
    "toml no parents OK 8\n"
    "  P#1=sub N=++ M=0 I#0=\n"
    "  P#1=sub N=x M=0 I#1=1\n"
    "  P#1=sub N=-- M=0 I#0=\n"
    "  P#1=q.s N=++ M=0 I#0=\n"
    "  P#1=q.s N=y M=0 I#1=2\n"
    "  P#1=q.s N=['li.t'] ; c3 M=0 I#1=true\n"
    "  P#1=q.s N=z M=0 I#1=3\n"
    "  P#1=q.s N=-- M=0 I#0=\n"
    "T15 [[ spaced ]\\nx=1\\n[a. b]\\ny=2\\n[a..b]\\nz=3\\n]\n"
    "toml no parents OK 9\n"
    "  P#1= spaced  N=++ M=0 I#0=\n"
    "  P#1= spaced  N=x M=0 I#1=1\n"
    "  P#1= spaced  N=-- M=0 I#0=\n"
    "  P#1=a. b N=++ M=0 I#0=\n"
    "  P#1=a. b N=y M=0 I#1=2\n"
    "  P#1=a. b N=-- M=0 I#0=\n"
    "  P#1=a..b N=++ M=0 I#0=\n"
    "  P#1=a..b N=z M=0 I#1=3\n"
    "  P#1=a..b N=-- M=0 I#0=\n"
    "T20 [a.b = 1\\na.b.c = 2\\n\"a.b\".c = 3\\n'x.y' = 4\\n]\n"
    "toml no parents OK 4\n"
    "  P#0= N=a.b M=0 I#1=1\n"
    "  P#0= N=a.b.c M=0 I#1=2\n"
    "  P#0= N=\"a.b\".c M=0 I#1=3\n"
    "  P#0= N=x.y M=0 I#1=4\n"
    "T21 [[sec]\\nsub.opt = 5\\n\"quoted.name\" = 6\\n]\n"
    "toml no parents OK 4\n"
    "  P#1=sec N=++ M=0 I#0=\n"
    "  P#1=sec N=sub.opt M=0 I#1=5\n"
    "  P#1=sec N=quoted.name M=0 I#1=6\n"
    "  P#1=sec N=-- M=0 I#0=\n"
    "T23 [\"bad\\\\qname\".c = 1\\n]\n"
    "toml no parents OK 1\n"
    "  P#0= N=\"bad\\\\qname\".c M=0 I#1=1\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// MULTILINE
//   constant: what the cases of d_tests_arg_config_multiline come to, as the
// reference writes them with CLI11.
static const char MULTILINE[] =
    "T24 [m = '''one\\ntwo\\nthree'''\\n]\n"
    "toml OK 1\n"
    "  P#0= N=m M=0 I#1=one\\ntwo\\nthree\n"
    "ini OK 1\n"
    "  P#0= N=m M=0 I#1=one\\ntwo\\nthree\n"
    "T25 [m = \"\"\"one\\\\ttab\\ntwo\"\"\"\\n]\n"
    "toml OK 1\n"
    "  P#0= N=m M=0 I#1=one\\ttab\\ntwo\n"
    "ini OK 1\n"
    "  P#0= N=m M=0 I#1=one\\ttab\\ntwo\n"
    "T26 [m = '''\\nlead\\n'''\\n]\n"
    "toml OK 1\n"
    "  P#0= N=m M=0 I#1=lead\n"
    "ini OK 1\n"
    "  P#0= N=m M=0 I#1=lead\n"
    "T27 [m = '''same line'''\\n]\n"
    "toml OK 1\n"
    "  P#0= N=m M=0 I#1=same line\n"
    "ini OK 1\n"
    "  P#0= N=m M=0 I#1=same line\n"
    "T28 [m = \"\"\"esc \\\\\"\"\" still\\nend\"\"\"\\n]\n"
    "toml OK 1\n"
    "  P#0= N=m M=0 I#1=esc \"\"\" still\\nend\n"
    "ini OK 1\n"
    "  P#0= N=m M=0 I#1=esc \"\"\" still\\nend\n"
    "T29 [m = \"\"\"join \\\\\\n   next line\"\"\"\\n]\n"
    "toml OK 1\n"
    "  P#0= N=m M=0 I#1=join next line\n"
    "ini OK 1\n"
    "  P#0= N=m M=0 I#1=join next line\n"
    "T30 [m = \"\"\"join \\\\\\n   \\\\\\n  more\\n\"\"\"\\n]\n"
    "toml OK 1\n"
    "  P#0= N=m M=0 I#1=join more\n"
    "ini OK 1\n"
    "  P#0= N=m M=0 I#1=join more\n"
    "T31 [m = '''a\\n'''  # trailing comment\\nn = 1\\n]\n"
    "toml OK 2\n"
    "  P#0= N=m M=0 I#1=a\n"
    "  P#0= N=n M=0 I#1=1\n"
    "ini OK 1\n"
    "  P#0= N=m M=0 I#1=a\\n'''  # trailing comment\\nn = 1\n"
    "T32 [m = '''x''''\\nn=2\\n]\n"
    "toml OK 2\n"
    "  P#0= N=m M=0 I#1=x'\n"
    "  P#0= N=n M=0 I#1=2\n"
    "ini OK 2\n"
    "  P#0= N=m M=0 I#1=x'\n"
    "  P#0= N=n M=0 I#1=2\n"
    "T33 [m = '''x'''''\\n]\n"
    "toml OK 1\n"
    "  P#0= N=m M=0 I#1=x''\n"
    "ini OK 1\n"
    "  P#0= N=m M=0 I#1=x''\n"
    "T34 [m = '''x''' junk\\nmore'''\\nn=3\\n]\n"
    "toml OK 2\n"
    "  P#0= N=m M=0 I#1=x''' junk\\nmore\n"
    "  P#0= N=n M=0 I#1=3\n"
    "ini OK 2\n"
    "  P#0= N=m M=0 I#1=x''' junk\\nmore\n"
    "  P#0= N=n M=0 I#1=3\n"
    "T35 [m = \"\"\"bad \\\\q escape\"\"\"\\n]\n"
    "toml ERR ParseError 111: unrecognized escape sequence \\\\q in bad \\\\q "
    "escape\n"
    "ini ERR ParseError 111: unrecognized escape sequence \\\\q in bad \\\\q "
    "escape\n"
    "T36 [m = \"\"\"never\\nclosed\\n]\n"
    "toml OK 1\n"
    "  P#0= N=m M=0 I#1=never\\nclosed\n"
    "ini OK 1\n"
    "  P#0= N=m M=0 I#1=never\\nclosed\n"
    "T37 [m = '''B\"(\\\\x41\\\\x00\\\\x42)\"'''\\n]\n"
    "toml OK 1\n"
    "  P#0= N=m M=0 I#1=A\\x00B\n"
    "ini OK 1\n"
    "  P#0= N=m M=0 I#1=A\\x00B\n"
    "T38 [   m = '''lead spaces'''\\n\\tn = '''tab'''\\n]\n"
    "toml OK 2\n"
    "  P#0= N=m M=0 I#1=lead spaces\n"
    "  P#0= N=n M=0 I#1=tab\n"
    "ini OK 2\n"
    "  P#0= N=m M=0 I#1=lead spaces\n"
    "  P#0= N=n M=0 I#1=tab\n"
    "T39 [\\x0bm = '''vt'''\\n]\n"
    "toml OK 1\n"
    "  P#0= N=m M=0 I#1=''vt\n"
    "ini OK 1\n"
    "  P#0= N=m M=0 I#1=''vt\n"
    "T40 [m = '''\\x0d\\nwin\\x0d\\n'''\\x0d\\n]\n"
    "toml OK 1\n"
    "  P#0= N=m M=0 I#1=\\x0d\\nwin\\x0d\n"
    "ini OK 1\n"
    "  P#0= N=m M=0 I#1=\\x0d\\nwin\\x0d\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// COMMENTS
//   constant: what the cases of d_tests_arg_config_comments come to, as the
// reference writes them with CLI11.
static const char COMMENTS[] =
    "T41 ['''\\ncomment\\n'''\\na=1\\n]\n"
    "toml OK 1\n"
    "  P#0= N=a M=0 I#1=1\n"
    "ini OK 1\n"
    "  P#0= N=a M=0 I#1=1\n"
    "T42 [\"\"\"one line comment\"\"\"\\nb=2\\n]\n"
    "toml OK 1\n"
    "  P#0= N=b M=0 I#1=2\n"
    "ini OK 1\n"
    "  P#0= N=b M=0 I#1=2\n"
    "T43 [\"\"\"\"\"\"\\nc=3\\n]\n"
    "toml OK 1\n"
    "  P#0= N=c M=0 I#1=3\n"
    "ini OK 1\n"
    "  P#0= N=c M=0 I#1=3\n"
    "T44 ['''\\nnever closed\\na=1\\n]\n"
    "toml OK 0\n"
    "ini OK 0\n"
    "T45 [\"\"\"x\"\"\"\\nd=4\\n]\n"
    "toml OK 1\n"
    "  P#0= N=d M=0 I#1=4\n"
    "ini OK 1\n"
    "  P#0= N=d M=0 I#1=4\n"
    "T06 [# comment\\n; also\\na = 1 # trailing\\nb = \"x # y\" # z\\nc = "
    "'p;q' ; r\\n]\n"
    "toml no comment OK 3\n"
    "  P#0= N=a M=0 I#3=1|#|trailing\n"
    "  P#0= N=b M=0 I#3=x # y|#|z\n"
    "  P#0= N=c M=0 I#3=p;q|;|r\n"
    "T07 [a = # only a comment\\nb=#\\n]\n"
    "toml no comment OK 2\n"
    "  P#0= N=a M=0 I#4=#|only|a|comment\n"
    "  P#0= N=b M=0 I#1=#\n"
    "T13 [[sub] # comment\\nx=1\\n[\"q.s\"] # c2\\ny=2\\n['li.t'] ; "
    "c3\\nz=3\\n]\n"
    "toml no comment OK 6\n"
    "  P#0= N=[sub] # comment M=0 I#1=true\n"
    "  P#0= N=x M=0 I#1=1\n"
    "  P#1=[\"q.s\"] N=# c2 M=0 I#1=true\n"
    "  P#0= N=y M=0 I#1=2\n"
    "  P#1=['li.t'] N=; c3 M=0 I#1=true\n"
    "  P#0= N=z M=0 I#1=3\n"
    "T14 [[sub # not closed\\nx=1\\n]\n"
    "toml no comment OK 2\n"
    "  P#0= N=[sub # not closed M=0 I#1=true\n"
    "  P#0= N=x M=0 I#1=1\n"
    "T31 [m = '''a\\n'''  # trailing comment\\nn = 1\\n]\n"
    "toml no comment OK 1\n"
    "  P#0= N=m M=0 I#1=a\\n'''  # trailing comment\\nn = 1\n"
    "T67 [\"a\" b = 1\\n'x' 'y' = 2\\n\"q\" # c\\n]\n"
    "toml no comment OK 3\n"
    "  P#0= N=\"a\" b M=0 I#1=1\n"
    "  P#0= N=x' 'y M=0 I#1=2\n"
    "  P#0= N=\"q\" # c M=0 I#1=true\n"
    "T06 [# comment\\n; also\\na = 1 # trailing\\nb = \"x # y\" # z\\nc = "
    "'p;q' ; r\\n]\n"
    "ini no comment OK 3\n"
    "  P#0= N=a M=0 I#3=1|#|trailing\n"
    "  P#0= N=b M=0 I#3=x # y|#|z\n"
    "  P#0= N=c M=0 I#3=p;q|;|r\n"
    "T13 [[sub] # comment\\nx=1\\n[\"q.s\"] # c2\\ny=2\\n['li.t'] ; "
    "c3\\nz=3\\n]\n"
    "ini no comment OK 6\n"
    "  P#0= N=[sub] # comment M=0 I#1=true\n"
    "  P#0= N=x M=0 I#1=1\n"
    "  P#1=[\"q.s\"] N=# c2 M=0 I#1=true\n"
    "  P#0= N=y M=0 I#1=2\n"
    "  P#1=['li.t'] N=; c3 M=0 I#1=true\n"
    "  P#0= N=z M=0 I#1=3\n"
    "T76 [a = 1 ; not ini comment\\n]\n"
    "ini no comment OK 1\n"
    "  P#0= N=a M=0 I#5=1|;|not|ini|comment\n"
    "T06 [# comment\\n; also\\na = 1 # trailing\\nb = \"x # y\" # z\\nc = "
    "'p;q' ; r\\n]\n"
    "toml comment = OK 3\n"
    "  P#0= N=a M=0 I#3=1|#|trailing\n"
    "  P#0= N=b M=0 I#1=x # y\n"
    "  P#0= N=c M=0 I#1=p;q\n"
    "ini comment = OK 3\n"
    "  P#0= N=a M=0 I#3=1|#|trailing\n"
    "  P#0= N=b M=0 I#1=x # y\n"
    "  P#0= N=c M=0 I#1=p;q\n"
    "T13 [[sub] # comment\\nx=1\\n[\"q.s\"] # c2\\ny=2\\n['li.t'] ; "
    "c3\\nz=3\\n]\n"
    "toml comment = OK 6\n"
    "  P#0= N=[sub] # comment M=0 I#1=true\n"
    "  P#0= N=x M=0 I#1=1\n"
    "  P#1=[\"q.s\"] N=# c2 M=0 I#1=true\n"
    "  P#0= N=y M=0 I#1=2\n"
    "  P#1=['li.t'] N=; c3 M=0 I#1=true\n"
    "  P#0= N=z M=0 I#1=3\n"
    "ini comment = OK 6\n"
    "  P#0= N=[sub] # comment M=0 I#1=true\n"
    "  P#0= N=x M=0 I#1=1\n"
    "  P#1=[\"q.s\"] N=# c2 M=0 I#1=true\n"
    "  P#0= N=y M=0 I#1=2\n"
    "  P#1=['li.t'] N=; c3 M=0 I#1=true\n"
    "  P#0= N=z M=0 I#1=3\n"
    "T61 [a = \"\", 2\\na = 3, 4\\n]\n"
    "toml comment = OK 1\n"
    "  P#0= N=a M=0 I#3=|3|4\n"
    "ini comment = OK 1\n"
    "  P#0= N=a M=0 I#3=|3|4\n"
    "T66 [=5\\n\"a\"=6\\n= \"x\"\\n]\n"
    "toml comment = OK 1\n"
    "  P#0= N=a M=0 I#1=6\n"
    "ini comment = OK 1\n"
    "  P#0= N=a M=0 I#1=6\n"
    "T73 [x = \"a\" \"b\"\\n]\n"
    "toml comment = OK 1\n"
    "  P#0= N=x M=0 I#1=a\n"
    "ini comment = OK 1\n"
    "  P#0= N=x M=0 I#1=a\n"
    "T07 [a = # only a comment\\nb=#\\n]\n"
    "toml comment = OK 2\n"
    "  P#0= N=a M=0 I#4=#|only|a|comment\n"
    "  P#0= N=b M=0 I#1=#\n"
    "T14 [[sub # not closed\\nx=1\\n]\n"
    "toml comment = OK 2\n"
    "  P#0= N=[sub # not closed M=0 I#1=true\n"
    "  P#0= N=x M=0 I#1=1\n"
    "T31 [m = '''a\\n'''  # trailing comment\\nn = 1\\n]\n"
    "toml comment = OK 1\n"
    "  P#0= N=m M=0 I#1=a\\n'''  # trailing comment\\nn = 1\n"
    "T67 [\"a\" b = 1\\n'x' 'y' = 2\\n\"q\" # c\\n]\n"
    "toml comment = OK 3\n"
    "  P#0= N=\"a\" b M=0 I#1=1\n"
    "  P#0= N=x' 'y M=0 I#1=2\n"
    "  P#0= N=\"q\" # c M=0 I#1=true\n"
    "T74 [x = 'a'b\\n]\n"
    "toml comment = OK 1\n"
    "  P#0= N=x M=0 I#1=a\n"
    "T75 [a=b=c\\n]\n"
    "toml comment = OK 1\n"
    "  P#0= N=a M=0 I#1=b\n"
    "T74 [x = 'a'b\\n]\n"
    "ini comment = OK 1\n"
    "  P#0= N=x M=0 I#1=a\n"
    "T75 [a=b=c\\n]\n"
    "ini comment = OK 1\n"
    "  P#0= N=a M=0 I#1=b\n"
    "T76 [a = 1 ; not ini comment\\n]\n"
    "ini comment = OK 1\n"
    "  P#0= N=a M=0 I#5=1|;|not|ini|comment\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// ARRAYS
//   constant: what the cases of d_tests_arg_config_arrays come to, as the
// reference writes them with CLI11.
static const char ARRAYS[] =
    "T46 [a = [1, 2, 3]\\nb = [\"x\", 'y']\\nc = []\\nd = [ ]\\n]\n"
    "toml OK 4\n"
    "  P#0= N=a M=0 I#3=1|2|3\n"
    "  P#0= N=b M=0 I#2=x|y\n"
    "  P#0= N=c M=0 I#0=\n"
    "  P#0= N=d M=0 I#0=\n"
    "ini OK 4\n"
    "  P#0= N=a M=0 I#3=1|2|3\n"
    "  P#0= N=b M=0 I#2=x|y\n"
    "  P#0= N=c M=0 I#0=\n"
    "  P#0= N=d M=0 I#0=\n"
    "toml ( ; ) OK 4\n"
    "  P#0= N=a M=0 I#1=[1, 2, 3]\n"
    "  P#0= N=b M=0 I#1=[\"x\", 'y']\n"
    "  P#0= N=c M=0 I#1=[]\n"
    "  P#0= N=d M=0 I#1=[ ]\n"
    "toml | , | OK 4\n"
    "  P#0= N=a M=0 I#1=[1, 2, 3]\n"
    "  P#0= N=b M=0 I#1=[\"x\", 'y']\n"
    "  P#0= N=c M=0 I#1=[]\n"
    "  P#0= N=d M=0 I#1=[ ]\n"
    "T47 [a = [1,\\n2,\\n3]\\nb = 4\\n]\n"
    "toml OK 2\n"
    "  P#0= N=a M=0 I#3=1|2|3\n"
    "  P#0= N=b M=0 I#1=4\n"
    "ini OK 2\n"
    "  P#0= N=a M=0 I#3=1|2|3\n"
    "  P#0= N=b M=0 I#1=4\n"
    "toml ( ; ) OK 2\n"
    "  P#0= N=a M=0 I#1=[1,\n"
    "  P#0= N=b M=0 I#1=4\n"
    "toml | , | OK 2\n"
    "  P#0= N=a M=0 I#1=[1,\n"
    "  P#0= N=b M=0 I#1=4\n"
    "T48 [a = [1, # one\\n  2, # two\\n  3]\\n]\n"
    "toml OK 1\n"
    "  P#0= N=a M=0 I#3=1|# one2|# two3\n"
    "ini OK 1\n"
    "  P#0= N=a M=0 I#3=1|# one2|# two3\n"
    "toml ( ; ) OK 2\n"
    "  P#0= N=a M=0 I#1=[1, # one\n"
    "  P#0= N=2, M=0 I#1=true\n"
    "toml | , | OK 2\n"
    "  P#0= N=a M=0 I#1=[1, # one\n"
    "  P#0= N=2, M=0 I#1=true\n"
    "T49 [a = [1, 2\\n]\n"
    "toml OK 1\n"
    "  P#0= N=a M=0 I#2=1|2\n"
    "ini OK 1\n"
    "  P#0= N=a M=0 I#2=1|2\n"
    "toml ( ; ) OK 1\n"
    "  P#0= N=a M=0 I#1=[1, 2\n"
    "toml | , | OK 1\n"
    "  P#0= N=a M=0 I#1=[1, 2\n"
    "T50 [a = [[1, 2], [3]]\\nb = [\"[x]\", '[y]']\\n]\n"
    "toml OK 2\n"
    "  P#0= N=a M=0 I#2=[1, 2]|[3]\n"
    "  P#0= N=b M=0 I#2=[[xx]]|[[yy]]\n"
    "ini OK 2\n"
    "  P#0= N=a M=0 I#2=[1, 2]|[3]\n"
    "  P#0= N=b M=0 I#2=[[xx]]|[[yy]]\n"
    "toml ( ; ) OK 2\n"
    "  P#0= N=a M=0 I#1=[[1, 2], [3]]\n"
    "  P#0= N=b M=0 I#1=[\"[x]\", '[y]']\n"
    "toml | , | OK 2\n"
    "  P#0= N=a M=0 I#1=[[1, 2], [3]]\n"
    "  P#0= N=b M=0 I#1=[\"[x]\", '[y]']\n"
    "T51 [a = \"[x]\"\\nb = '[y]'\\nc = [z]\\n]\n"
    "toml OK 3\n"
    "  P#0= N=a M=0 I#1=[[xx]]\n"
    "  P#0= N=b M=0 I#1=[[yy]]\n"
    "  P#0= N=c M=0 I#1=z\n"
    "ini OK 3\n"
    "  P#0= N=a M=0 I#1=[[xx]]\n"
    "  P#0= N=b M=0 I#1=[[yy]]\n"
    "  P#0= N=c M=0 I#1=z\n"
    "toml ( ; ) OK 3\n"
    "  P#0= N=a M=0 I#1=[[xx]]\n"
    "  P#0= N=b M=0 I#1=[[yy]]\n"
    "  P#0= N=c M=0 I#1=[z]\n"
    "toml | , | OK 3\n"
    "  P#0= N=a M=0 I#1=[[xx]]\n"
    "  P#0= N=b M=0 I#1=[[yy]]\n"
    "  P#0= N=c M=0 I#1=[z]\n"
    "T03 [list = 1, 2, 3\\nspaced = a b c\\nmixed = a, b c\\n]\n"
    "toml ( ; ) OK 3\n"
    "  P#0= N=list M=0 I#1=1, 2, 3\n"
    "  P#0= N=spaced M=0 I#1=a b c\n"
    "  P#0= N=mixed M=0 I#1=a, b c\n"
    "toml | , | OK 3\n"
    "  P#0= N=list M=0 I#1=1, 2, 3\n"
    "  P#0= N=spaced M=0 I#1=a b c\n"
    "  P#0= N=mixed M=0 I#1=a, b c\n"
    "ini ( ; ) OK 3\n"
    "  P#0= N=list M=0 I#1=1, 2, 3\n"
    "  P#0= N=spaced M=0 I#1=a b c\n"
    "  P#0= N=mixed M=0 I#1=a, b c\n"
    "T06 [# comment\\n; also\\na = 1 # trailing\\nb = \"x # y\" # z\\nc = "
    "'p;q' ; r\\n]\n"
    "toml ( ; ) OK 3\n"
    "  P#0= N=a M=0 I#1=1\n"
    "  P#0= N=b M=0 I#1=x # y\n"
    "  P#0= N=c M=0 I#1='p;q' ; r\n"
    "toml | , | OK 3\n"
    "  P#0= N=a M=0 I#1=1\n"
    "  P#0= N=b M=0 I#1=x # y\n"
    "  P#0= N=c M=0 I#1='p;q' ; r\n"
    "ini ( ; ) OK 3\n"
    "  P#0= N=a M=0 I#1=1 # trailing\n"
    "  P#0= N=b M=0 I#1=\"x # y\" # z\n"
    "  P#0= N=c M=0 I#1=p;q\n"
    "T07 [a = # only a comment\\nb=#\\n]\n"
    "toml ( ; ) OK 2\n"
    "  P#0= N=a M=0 I#1=\n"
    "  P#0= N=b M=0 I#1=\n"
    "toml | , | OK 2\n"
    "  P#0= N=a M=0 I#1=\n"
    "  P#0= N=b M=0 I#1=\n"
    "ini ( ; ) OK 2\n"
    "  P#0= N=a M=0 I#1=# only a comment\n"
    "  P#0= N=b M=0 I#1=#\n"
    "T59 [a = 1, 2\\na = 3, 4\\n]\n"
    "toml ( ; ) OK 1\n"
    "  P#0= N=a M=0 I#2=1, 2|3, 4\n"
    "T62 [a = 1, 2\\na = %%, 4\\n]\n"
    "toml ( ; ) OK 1\n"
    "  P#0= N=a M=0 I#2=1, 2|%%, 4\n"
    "T73 [x = \"a\" \"b\"\\n]\n"
    "toml ( ; ) OK 1\n"
    "  P#0= N=x M=0 I#1=a\" \"b\n"
    "T76 [a = 1 ; not ini comment\\n]\n"
    "toml ( ; ) OK 1\n"
    "  P#0= N=a M=0 I#1=1 ; not ini comment\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// ESCAPES
//   constant: what the cases of d_tests_arg_config_escapes come to, as the
// reference writes them with CLI11.
static const char ESCAPES[] =
    "T52 [b = B\"(\\\\x01\\\\x02\\\\xff)\"\\nc = 'B\"(\\\\x41)\"'\\n]\n"
    "toml OK 2\n"
    "  P#0= N=b M=0 I#1=\\x01\\x02\xff\n"
    "  P#0= N=c M=0 I#1=A\n"
    "ini OK 2\n"
    "  P#0= N=b M=0 I#1=\\x01\\x02\xff\n"
    "  P#0= N=c M=0 I#1=A\n"
    "toml ' \" OK 2\n"
    "  P#0= N=b M=0 I#1=\\x01\\x02\xff\n"
    "  P#0= N=c M=0 I#1=A\n"
    "T53 [s = \"tab\\\\there\"\\nt = \"nl\\\\nhere\"\\nu = "
    "\"\\\\u00e9\\\\U0001F600\"\\n]\n"
    "toml OK 3\n"
    "  P#0= N=s M=0 I#1=tab\\there\n"
    "  P#0= N=t M=0 I#1=nl\\nhere\n"
    "  P#0= N=u M=0 I#1=\xc3\xa9\xf0\x9f\x98\x80\n"
    "ini OK 3\n"
    "  P#0= N=s M=0 I#1=tab\\there\n"
    "  P#0= N=t M=0 I#1=nl\\nhere\n"
    "  P#0= N=u M=0 I#1=\xc3\xa9\xf0\x9f\x98\x80\n"
    "toml ' \" OK 3\n"
    "  P#0= N=s M=0 I#1=tab\\\\there\n"
    "  P#0= N=t M=0 I#1=nl\\\\nhere\n"
    "  P#0= N=u M=0 I#1=\\\\u00e9\\\\U0001F600\n"
    "T54 [s = \"bad \\\\q\"\\n]\n"
    "toml ERR ParseError 111: unrecognized escape sequence \\\\q in bad \\\\q\n"
    "ini ERR ParseError 111: unrecognized escape sequence \\\\q in bad \\\\q\n"
    "toml ' \" OK 1\n"
    "  P#0= N=s M=0 I#1=bad \\\\q\n"
    "T55 [s = \"\\\\ud800\"\\n]\n"
    "toml ERR ParseError 111: [0xD800, 0xDFFF] are not valid code points.\n"
    "ini ERR ParseError 111: [0xD800, 0xDFFF] are not valid code points.\n"
    "toml ' \" OK 1\n"
    "  P#0= N=s M=0 I#1=\\\\ud800\n"
    "T56 [s = \"end\\\\\\\\\"\\nt = \"\\\\\"\"\\n]\n"
    "toml OK 2\n"
    "  P#0= N=s M=0 I#1=end\\\\\n"
    "  P#0= N=t M=0 I#1=\"\n"
    "ini OK 2\n"
    "  P#0= N=s M=0 I#1=end\\\\\n"
    "  P#0= N=t M=0 I#1=\"\n"
    "toml ' \" OK 2\n"
    "  P#0= N=s M=0 I#1=end\\\\\\\\\n"
    "  P#0= N=t M=0 I#1=\\\\\"\n"
    "T57 [s = \"nul\\\\0x\"\\n]\n"
    "toml OK 1\n"
    "  P#0= N=s M=0 I#1=nul\\x00x\n"
    "ini OK 1\n"
    "  P#0= N=s M=0 I#1=nul\\x00x\n"
    "toml ' \" OK 1\n"
    "  P#0= N=s M=0 I#1=nul\\\\0x\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// DUPLICATES
//   constant: what the cases of d_tests_arg_config_duplicates come to, as the
// reference writes them with CLI11.
static const char DUPLICATES[] =
    "T11 [[[arr]]\\nx=1\\n[[arr]]\\nx=2\\n]\n"
    "toml OK 6\n"
    "  P#1=arr N=++ M=0 I#0=\n"
    "  P#1=arr N=x M=0 I#1=1\n"
    "  P#1=arr N=-- M=0 I#0=\n"
    "  P#1=arr N=++ M=0 I#0=\n"
    "  P#1=arr N=x M=0 I#1=2\n"
    "  P#1=arr N=-- M=0 I#0=\n"
    "ini OK 6\n"
    "  P#1=arr N=++ M=0 I#0=\n"
    "  P#1=arr N=x M=0 I#1=1\n"
    "  P#1=arr N=-- M=0 I#0=\n"
    "  P#1=arr N=++ M=0 I#0=\n"
    "  P#1=arr N=x M=0 I#1=2\n"
    "  P#1=arr N=-- M=0 I#0=\n"
    "toml duplicates OK 5\n"
    "  P#1=arr N=++ M=0 I#0=\n"
    "  P#1=arr N=x M=1 I#3=1|%%|2\n"
    "  P#1=arr N=-- M=0 I#0=\n"
    "  P#1=arr N=++ M=0 I#0=\n"
    "  P#1=arr N=-- M=0 I#0=\n"
    "ini duplicates OK 5\n"
    "  P#1=arr N=++ M=0 I#0=\n"
    "  P#1=arr N=x M=1 I#3=1|%%|2\n"
    "  P#1=arr N=-- M=0 I#0=\n"
    "  P#1=arr N=++ M=0 I#0=\n"
    "  P#1=arr N=-- M=0 I#0=\n"
    "T58 [a = 1\\na = 2\\n]\n"
    "toml OK 1\n"
    "  P#0= N=a M=0 I#2=1|2\n"
    "ini OK 1\n"
    "  P#0= N=a M=0 I#2=1|2\n"
    "toml duplicates OK 1\n"
    "  P#0= N=a M=1 I#3=1|%%|2\n"
    "ini duplicates OK 1\n"
    "  P#0= N=a M=1 I#3=1|%%|2\n"
    "T59 [a = 1, 2\\na = 3, 4\\n]\n"
    "toml OK 1\n"
    "  P#0= N=a M=1 I#5=1|2|%%|3|4\n"
    "ini OK 1\n"
    "  P#0= N=a M=1 I#5=1|2|%%|3|4\n"
    "toml duplicates OK 1\n"
    "  P#0= N=a M=1 I#5=1|2|%%|3|4\n"
    "ini duplicates OK 1\n"
    "  P#0= N=a M=1 I#5=1|2|%%|3|4\n"
    "T60 [a = 1, 2\\nb = 0\\na = 3, 4\\n]\n"
    "toml OK 3\n"
    "  P#0= N=a M=0 I#2=1|2\n"
    "  P#0= N=b M=0 I#1=0\n"
    "  P#0= N=a M=0 I#2=3|4\n"
    "ini OK 3\n"
    "  P#0= N=a M=0 I#2=1|2\n"
    "  P#0= N=b M=0 I#1=0\n"
    "  P#0= N=a M=0 I#2=3|4\n"
    "toml duplicates OK 2\n"
    "  P#0= N=a M=1 I#5=1|2|%%|3|4\n"
    "  P#0= N=b M=0 I#1=0\n"
    "ini duplicates OK 2\n"
    "  P#0= N=a M=1 I#5=1|2|%%|3|4\n"
    "  P#0= N=b M=0 I#1=0\n"
    "T61 [a = \"\", 2\\na = 3, 4\\n]\n"
    "toml OK 1\n"
    "  P#0= N=a M=1 I#5=|2|%%|3|4\n"
    "ini OK 1\n"
    "  P#0= N=a M=1 I#5=|2|%%|3|4\n"
    "toml duplicates OK 1\n"
    "  P#0= N=a M=1 I#5=|2|%%|3|4\n"
    "ini duplicates OK 1\n"
    "  P#0= N=a M=1 I#5=|2|%%|3|4\n"
    "T62 [a = 1, 2\\na = %%, 4\\n]\n"
    "toml OK 1\n"
    "  P#0= N=a M=0 I#4=1|2|%%|4\n"
    "ini OK 1\n"
    "  P#0= N=a M=0 I#4=1|2|%%|4\n"
    "toml duplicates OK 1\n"
    "  P#0= N=a M=0 I#4=1|2|%%|4\n"
    "ini duplicates OK 1\n"
    "  P#0= N=a M=0 I#4=1|2|%%|4\n"
    "T63 [a = []\\na = 1\\n]\n"
    "toml OK 1\n"
    "  P#0= N=a M=0 I#1=1\n"
    "ini OK 1\n"
    "  P#0= N=a M=0 I#1=1\n"
    "T64 [a = 1\\na = []\\n]\n"
    "toml OK 1\n"
    "  P#0= N=a M=0 I#1=1\n"
    "ini OK 1\n"
    "  P#0= N=a M=0 I#1=1\n"
    "T89 [a = 1\\n-- = 2\\n-- = 3\\n]\n"
    "toml OK 2\n"
    "  P#0= N=a M=0 I#1=1\n"
    "  P#0= N=-- M=0 I#2=2|3\n"
    "toml duplicates OK 2\n"
    "  P#0= N=a M=0 I#1=1\n"
    "  P#0= N=-- M=1 I#3=2|%%|3\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// ODD_LINES
//   constant: what the cases of d_tests_arg_config_odd_lines come to, as the
// reference writes them with CLI11.
static const char ODD_LINES[] =
    "T65 [-- = 1\\n++ = 2\\n[s]\\n-- = 3\\n]\n"
    "toml OK 5\n"
    "  P#0= N=-- M=0 I#1=1\n"
    "  P#0= N=++ M=0 I#1=2\n"
    "  P#1=s N=++ M=0 I#0=\n"
    "  P#1=s N=-- M=0 I#1=3\n"
    "  P#1=s N=-- M=0 I#0=\n"
    "ini OK 5\n"
    "  P#0= N=-- M=0 I#1=1\n"
    "  P#0= N=++ M=0 I#1=2\n"
    "  P#1=s N=++ M=0 I#0=\n"
    "  P#1=s N=-- M=0 I#1=3\n"
    "  P#1=s N=-- M=0 I#0=\n"
    "T66 [=5\\n\"a\"=6\\n= \"x\"\\n]\n"
    "toml OK 2\n"
    "  P#0= N=a M=0 I#1=6\n"
    "  P#0= N= M=0 I#1=x\n"
    "ini OK 2\n"
    "  P#0= N=a M=0 I#1=6\n"
    "  P#0= N= M=0 I#1=x\n"
    "T67 [\"a\" b = 1\\n'x' 'y' = 2\\n\"q\" # c\\n]\n"
    "toml OK 3\n"
    "  P#0= N=\"a\" b M=0 I#1=1\n"
    "  P#0= N=x' 'y M=0 I#1=2\n"
    "  P#0= N=q M=0 I#1=true\n"
    "ini OK 3\n"
    "  P#0= N=\"a\" b M=0 I#1=1\n"
    "  P#0= N=x' 'y M=0 I#1=2\n"
    "  P#0= N=\"q\" # c M=0 I#1=true\n"
    "T69 [a = 1\\x0d\\nb = 2\\x0d\\n[s]\\x0d\\nc = 3\\x0d\\n]\n"
    "toml OK 5\n"
    "  P#0= N=a M=0 I#1=1\n"
    "  P#0= N=b M=0 I#1=2\n"
    "  P#1=s N=++ M=0 I#0=\n"
    "  P#1=s N=c M=0 I#1=3\n"
    "  P#1=s N=-- M=0 I#0=\n"
    "ini OK 5\n"
    "  P#0= N=a M=0 I#1=1\n"
    "  P#0= N=b M=0 I#1=2\n"
    "  P#1=s N=++ M=0 I#0=\n"
    "  P#1=s N=c M=0 I#1=3\n"
    "  P#1=s N=-- M=0 I#0=\n"
    "T70 [\"unterminated = 1\\n]\n"
    "toml OK 1\n"
    "  P#0= N=\"unterminated = 1 M=0 I#1=true\n"
    "ini OK 1\n"
    "  P#0= N=\"unterminated = 1 M=0 I#1=true\n"
    "T71 ['x = 2\\n]\n"
    "toml OK 1\n"
    "  P#0= N='x = 2 M=0 I#1=true\n"
    "ini OK 1\n"
    "  P#0= N='x = 2 M=0 I#1=true\n"
    "T72 [\"a\\\\\" = 3\\n]\n"
    "toml OK 1\n"
    "  P#0= N=\"a\\\\\" = 3 M=0 I#1=true\n"
    "ini OK 1\n"
    "  P#0= N=\"a\\\\\" = 3 M=0 I#1=true\n"
    "T73 [x = \"a\" \"b\"\\n]\n"
    "toml OK 1\n"
    "  P#0= N=x M=0 I#2=a|b\n"
    "ini OK 1\n"
    "  P#0= N=x M=0 I#2=a|b\n"
    "T74 [x = 'a'b\\n]\n"
    "toml OK 1\n"
    "  P#0= N=x M=0 I#1='a'b\n"
    "ini OK 1\n"
    "  P#0= N=x M=0 I#1='a'b\n"
    "T75 [a=b=c\\n]\n"
    "toml OK 1\n"
    "  P#0= N=a M=0 I#1=b=c\n"
    "ini OK 1\n"
    "  P#0= N=a M=0 I#1=b=c\n"
    "T76 [a = 1 ; not ini comment\\n]\n"
    "toml OK 1\n"
    "  P#0= N=a M=0 I#5=1|;|not|ini|comment\n"
    "ini OK 1\n"
    "  P#0= N=a M=0 I#1=1\n"
    "T77 [a b c\\n]\n"
    "toml OK 1\n"
    "  P#0= N=a b c M=0 I#1=true\n"
    "ini OK 1\n"
    "  P#0= N=a b c M=0 I#1=true\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// DELIMITERS
//   constant: what the cases of d_tests_arg_config_delimiters come to, as the
// reference writes them with CLI11.
static const char DELIMITERS[] =
    "T88 [a: 1\\nb : 2 ; c\\n[x/y]\\nz: 3\\nw/v: 4\\n\"q/r\": 5\\nt = 6\\n]\n"
    "toml OK 8\n"
    "  P#0= N=a: 1 M=0 I#1=true\n"
    "  P#0= N=b : 2 ; c M=0 I#1=true\n"
    "  P#1=x/y N=++ M=0 I#0=\n"
    "  P#1=x/y N=z: 3 M=0 I#1=true\n"
    "  P#1=x/y N=w/v: 4 M=0 I#1=true\n"
    "  P#1=x/y N=\"q/r\": 5 M=0 I#1=true\n"
    "  P#1=x/y N=t M=0 I#1=6\n"
    "  P#1=x/y N=-- M=0 I#0=\n"
    "toml ; : / OK 10\n"
    "  P#0= N=a M=0 I#1=1\n"
    "  P#0= N=b M=0 I#1=2\n"
    "  P#1=x N=++ M=0 I#0=\n"
    "  P#2=x|y N=++ M=0 I#0=\n"
    "  P#2=x|y N=z M=0 I#1=3\n"
    "  P#3=x|y|w N=v M=0 I#1=4\n"
    "  P#2=x|y N=q/r M=0 I#1=5\n"
    "  P#2=x|y N=t = 6 M=0 I#1=true\n"
    "  P#2=x|y N=-- M=0 I#0=\n"
    "  P#1=x N=-- M=0 I#0=\n"
    "T01 [a=1\\nb = 2\\nc= three\\n]\n"
    "toml ; : / OK 3\n"
    "  P#0= N=a=1 M=0 I#1=true\n"
    "  P#0= N=b = 2 M=0 I#1=true\n"
    "  P#0= N=c= three M=0 I#1=true\n"
    "T06 [# comment\\n; also\\na = 1 # trailing\\nb = \"x # y\" # z\\nc = "
    "'p;q' ; r\\n]\n"
    "toml ; : / OK 3\n"
    "  P#0= N=a = 1 # trailing M=0 I#1=true\n"
    "  P#0= N=b = \"x # y\" # z M=0 I#1=true\n"
    "  P#0= N=c = 'p M=0 I#1=true\n"
    "T08 [[sub]\\nx=1\\n[sub.deeper]\\ny=2\\n[other]\\nz=3\\n]\n"
    "toml ; : / OK 9\n"
    "  P#1=sub N=++ M=0 I#0=\n"
    "  P#1=sub N=x=1 M=0 I#1=true\n"
    "  P#1=sub N=-- M=0 I#0=\n"
    "  P#1=sub.deeper N=++ M=0 I#0=\n"
    "  P#1=sub.deeper N=y=2 M=0 I#1=true\n"
    "  P#1=sub.deeper N=-- M=0 I#0=\n"
    "  P#1=other N=++ M=0 I#0=\n"
    "  P#1=other N=z=3 M=0 I#1=true\n"
    "  P#1=other N=-- M=0 I#0=\n"
    "T20 [a.b = 1\\na.b.c = 2\\n\"a.b\".c = 3\\n'x.y' = 4\\n]\n"
    "toml ; : / OK 4\n"
    "  P#0= N=a.b = 1 M=0 I#1=true\n"
    "  P#0= N=a.b.c = 2 M=0 I#1=true\n"
    "  P#0= N=\"a.b\".c = 3 M=0 I#1=true\n"
    "  P#0= N='x.y' = 4 M=0 I#1=true\n"
    "T65 [-- = 1\\n++ = 2\\n[s]\\n-- = 3\\n]\n"
    "toml ; : / OK 5\n"
    "  P#0= N=-- = 1 M=0 I#1=true\n"
    "  P#0= N=++ = 2 M=0 I#1=true\n"
    "  P#1=s N=++ M=0 I#0=\n"
    "  P#1=s N=-- = 3 M=0 I#1=true\n"
    "  P#1=s N=-- M=0 I#0=\n"
    "T01 [a=1\\nb = 2\\nc= three\\n]\n"
    "toml no delimiter OK 3\n"
    "  P#0= N=a=1 M=0 I#1=true\n"
    "  P#0= N=b = 2 M=0 I#1=true\n"
    "  P#0= N=c= three M=0 I#1=true\n"
    "T03 [list = 1, 2, 3\\nspaced = a b c\\nmixed = a, b c\\n]\n"
    "toml no delimiter OK 3\n"
    "  P#0= N=list = 1, 2, 3 M=0 I#1=true\n"
    "  P#0= N=spaced = a b c M=0 I#1=true\n"
    "  P#0= N=mixed = a, b c M=0 I#1=true\n"
    "T08 [[sub]\\nx=1\\n[sub.deeper]\\ny=2\\n[other]\\nz=3\\n]\n"
    "toml no delimiter OK 9\n"
    "  P#1=sub N=++ M=0 I#0=\n"
    "  P#1=sub N=x=1 M=0 I#1=true\n"
    "  P#2=sub|deeper N=++ M=0 I#0=\n"
    "  P#2=sub|deeper N=y=2 M=0 I#1=true\n"
    "  P#2=sub|deeper N=-- M=0 I#0=\n"
    "  P#1=sub N=-- M=0 I#0=\n"
    "  P#1=other N=++ M=0 I#0=\n"
    "  P#1=other N=z=3 M=0 I#1=true\n"
    "  P#1=other N=-- M=0 I#0=\n"
    "T20 [a.b = 1\\na.b.c = 2\\n\"a.b\".c = 3\\n'x.y' = 4\\n]\n"
    "toml no delimiter OK 4\n"
    "  P#1=a N=b = 1 M=0 I#1=true\n"
    "  P#2=a|b N=c = 2 M=0 I#1=true\n"
    "  P#1=a.b N=c = 3 M=0 I#1=true\n"
    "  P#1=x.y N== 4 M=0 I#1=true\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// BYTES
//   constant: what the cases of d_tests_arg_config_bytes come to, as the
// reference writes them with CLI11.
static const char BYTES[] =
    "T79 [a=x\\x00y\\nn\\x00m=1\\n\"q\\x00r\"=2\\n]\n"
    "toml OK 3\n"
    "  P#0= N=a M=0 I#1=x\\x00y\n"
    "  P#0= N=n\\x00m M=0 I#1=1\n"
    "  P#0= N=q\\x00r M=0 I#1=2\n"
    "ini OK 3\n"
    "  P#0= N=a M=0 I#1=x\\x00y\n"
    "  P#0= N=n\\x00m M=0 I#1=1\n"
    "  P#0= N=q\\x00r M=0 I#1=2\n"
    "T80 [s = \"a\\\\u0000b\"\\nb = B\"(\\\\x00\\\\x01)\"\\nm = "
    "\"\"\"c\\\\u0000d\"\"\"\\n]\n"
    "toml OK 3\n"
    "  P#0= N=s M=0 I#1=a\\x00b\n"
    "  P#0= N=b M=0 I#1=\\x00\\x01\n"
    "  P#0= N=m M=0 I#1=c\\x00d\n"
    "ini OK 3\n"
    "  P#0= N=s M=0 I#1=a\\x00b\n"
    "  P#0= N=b M=0 I#1=\\x00\\x01\n"
    "  P#0= N=m M=0 I#1=c\\x00d\n"
    "T81 [name = \"\xc3\xa9t\xc3\xa9\"\\n[s\xc3\xa9]\\nv = \xc3\xbc, "
    "\xc3\xb6\\n\xc3\xa9 = 1\\n]\n"
    "toml OK 5\n"
    "  P#0= N=name M=0 I#1=\xc3\xa9t\xc3\xa9\n"
    "  P#1=s\xc3\xa9 N=++ M=0 I#0=\n"
    "  P#1=s\xc3\xa9 N=v M=0 I#2=\xc3\xbc|\xc3\xb6\n"
    "  P#1=s\xc3\xa9 N=\xc3\xa9 M=0 I#1=1\n"
    "  P#1=s\xc3\xa9 N=-- M=0 I#0=\n"
    "ini OK 5\n"
    "  P#0= N=name M=0 I#1=\xc3\xa9t\xc3\xa9\n"
    "  P#1=s\xc3\xa9 N=++ M=0 I#0=\n"
    "  P#1=s\xc3\xa9 N=v M=0 I#2=\xc3\xbc|\xc3\xb6\n"
    "  P#1=s\xc3\xa9 N=\xc3\xa9 M=0 I#1=1\n"
    "  P#1=s\xc3\xa9 N=-- M=0 I#0=\n"
    "T82 [a=1\\nb=2]\n"
    "toml OK 2\n"
    "  P#0= N=a M=0 I#1=1\n"
    "  P#0= N=b M=0 I#1=2\n"
    "ini OK 2\n"
    "  P#0= N=a M=0 I#1=1\n"
    "  P#0= N=b M=0 I#1=2\n"
    "T83 []\n"
    "toml OK 0\n"
    "ini OK 0\n"
    "T85 [v = "
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
    "xxxx\\nw = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, "
    "18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, "
    "36, 37, 38, 39, 40]\\n]\n"
    "toml OK 2\n"
    "  P#0= N=v M=0 "
    "I#1=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxx\n"
    "  P#0= N=w M=0 "
    "I#40=1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|"
    "27|28|29|30|31|32|33|34|35|36|37|38|39|40\n"
    "ini OK 2\n"
    "  P#0= N=v M=0 "
    "I#1=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxx\n"
    "  P#0= N=w M=0 "
    "I#40=1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|"
    "27|28|29|30|31|32|33|34|35|36|37|38|39|40\n"
    "T86 "
    "[k01=1\\nk02=2\\nk03=3\\nk04=4\\nk05=5\\nk06=6\\nk07=7\\nk08=8\\nk09=9\\nk"
    "10=10\\nk11=11\\nk12=12\\nk13=13\\nk14=14\\nk15=15\\nk16=16\\nk17=17\\nk18"
    "=18\\nk19=19\\nk20=20\\nk21=21\\nk22=22\\nk23=23\\nk24=24\\n]\n"
    "toml OK 24\n"
    "  P#0= N=k01 M=0 I#1=1\n"
    "  P#0= N=k02 M=0 I#1=2\n"
    "  P#0= N=k03 M=0 I#1=3\n"
    "  P#0= N=k04 M=0 I#1=4\n"
    "  P#0= N=k05 M=0 I#1=5\n"
    "  P#0= N=k06 M=0 I#1=6\n"
    "  P#0= N=k07 M=0 I#1=7\n"
    "  P#0= N=k08 M=0 I#1=8\n"
    "  P#0= N=k09 M=0 I#1=9\n"
    "  P#0= N=k10 M=0 I#1=10\n"
    "  P#0= N=k11 M=0 I#1=11\n"
    "  P#0= N=k12 M=0 I#1=12\n"
    "  P#0= N=k13 M=0 I#1=13\n"
    "  P#0= N=k14 M=0 I#1=14\n"
    "  P#0= N=k15 M=0 I#1=15\n"
    "  P#0= N=k16 M=0 I#1=16\n"
    "  P#0= N=k17 M=0 I#1=17\n"
    "  P#0= N=k18 M=0 I#1=18\n"
    "  P#0= N=k19 M=0 I#1=19\n"
    "  P#0= N=k20 M=0 I#1=20\n"
    "  P#0= N=k21 M=0 I#1=21\n"
    "  P#0= N=k22 M=0 I#1=22\n"
    "  P#0= N=k23 M=0 I#1=23\n"
    "  P#0= N=k24 M=0 I#1=24\n"
    "ini OK 24\n"
    "  P#0= N=k01 M=0 I#1=1\n"
    "  P#0= N=k02 M=0 I#1=2\n"
    "  P#0= N=k03 M=0 I#1=3\n"
    "  P#0= N=k04 M=0 I#1=4\n"
    "  P#0= N=k05 M=0 I#1=5\n"
    "  P#0= N=k06 M=0 I#1=6\n"
    "  P#0= N=k07 M=0 I#1=7\n"
    "  P#0= N=k08 M=0 I#1=8\n"
    "  P#0= N=k09 M=0 I#1=9\n"
    "  P#0= N=k10 M=0 I#1=10\n"
    "  P#0= N=k11 M=0 I#1=11\n"
    "  P#0= N=k12 M=0 I#1=12\n"
    "  P#0= N=k13 M=0 I#1=13\n"
    "  P#0= N=k14 M=0 I#1=14\n"
    "  P#0= N=k15 M=0 I#1=15\n"
    "  P#0= N=k16 M=0 I#1=16\n"
    "  P#0= N=k17 M=0 I#1=17\n"
    "  P#0= N=k18 M=0 I#1=18\n"
    "  P#0= N=k19 M=0 I#1=19\n"
    "  P#0= N=k20 M=0 I#1=20\n"
    "  P#0= N=k21 M=0 I#1=21\n"
    "  P#0= N=k22 M=0 I#1=22\n"
    "  P#0= N=k23 M=0 I#1=23\n"
    "  P#0= N=k24 M=0 I#1=24\n"
    "T87 [a=1\\x0db=2\\x0d\\nc=3\\n]\n"
    "toml OK 2\n"
    "  P#0= N=a M=0 I#1=1\\x0db=2\n"
    "  P#0= N=c M=0 I#1=3\n"
    "ini OK 2\n"
    "  P#0= N=a M=0 I#1=1\\x0db=2\n"
    "  P#0= N=c M=0 I#1=3\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// ENTERED
//   constant: what the cases of d_tests_arg_config_entered come to, as the
// reference writes them with CLI11.
static const char ENTERED[] =
    "T92 [[sub]\\na=1\\nb.c=2\\nd=3\\n[other]\\ne=4\\n[sub]\\nf=5\\n]\n"
    "toml section sub OK 10\n"
    "  P#1=sub N=++ M=0 I#0=\n"
    "  P#0= N=a M=0 I#1=1\n"
    "  P#2=sub|b N=c M=0 I#1=2\n"
    "  P#1=sub N=d M=0 I#1=3\n"
    "  P#1=sub N=-- M=0 I#0=\n"
    "  P#1=other N=++ M=0 I#0=\n"
    "  P#1=other N=-- M=0 I#0=\n"
    "  P#1=sub N=++ M=0 I#0=\n"
    "  P#0= N=f M=0 I#1=5\n"
    "  P#1=sub N=-- M=0 I#0=\n"
    "toml section \"\" OK 11\n"
    "  P#1=sub N=++ M=0 I#0=\n"
    "  P#1=sub N=a M=0 I#1=1\n"
    "  P#2=sub|b N=c M=0 I#1=2\n"
    "  P#1=sub N=d M=0 I#1=3\n"
    "  P#1=sub N=-- M=0 I#0=\n"
    "  P#1=other N=++ M=0 I#0=\n"
    "  P#1=other N=e M=0 I#1=4\n"
    "  P#1=other N=-- M=0 I#0=\n"
    "  P#1=sub N=++ M=0 I#0=\n"
    "  P#1=sub N=f M=0 I#1=5\n"
    "  P#1=sub N=-- M=0 I#0=\n"
    "T93 [arr.x = 1\\n[default]\\narr.y = 2\\n]\n"
    "toml section arr 0 OK 1\n"
    "  P#0= N=x M=0 I#1=1\n"
    "toml section arr 1 OK 1\n"
    "  P#0= N=y M=0 I#1=2\n"
    "T94 [[sub.a.b.c]\\nx=1\\ny.z=2\\n]\n"
    "toml section sub OK 10\n"
    "  P#1=sub N=++ M=0 I#0=\n"
    "  P#2=sub|a N=++ M=0 I#0=\n"
    "  P#3=sub|a|b N=++ M=0 I#0=\n"
    "  P#4=sub|a|b|c N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=x M=0 I#1=1\n"
    "  P#5=sub|a|b|c|y N=z M=0 I#1=2\n"
    "  P#4=sub|a|b|c N=-- M=0 I#0=\n"
    "  P#3=sub|a|b N=-- M=0 I#0=\n"
    "  P#2=sub|a N=-- M=0 I#0=\n"
    "  P#1=sub N=-- M=0 I#0=\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// TRANSITIONS
//   constant: what the cases of d_tests_arg_config_transitions come to, as the
// reference writes them with CLI11.
static const char TRANSITIONS[] =
    "T95 [[a]\\nx=1\\n[a.b.c]\\ny=2\\n]\n"
    "toml OK 8\n"
    "  P#1=a N=++ M=0 I#0=\n"
    "  P#1=a N=x M=0 I#1=1\n"
    "  P#2=a|b N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=y M=0 I#1=2\n"
    "  P#3=a|b|c N=-- M=0 I#0=\n"
    "  P#2=a|b N=-- M=0 I#0=\n"
    "  P#1=a N=-- M=0 I#0=\n"
    "ini OK 8\n"
    "  P#1=a N=++ M=0 I#0=\n"
    "  P#1=a N=x M=0 I#1=1\n"
    "  P#2=a|b N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=y M=0 I#1=2\n"
    "  P#3=a|b|c N=-- M=0 I#0=\n"
    "  P#2=a|b N=-- M=0 I#0=\n"
    "  P#1=a N=-- M=0 I#0=\n"
    "T96 [[a.b.c]\\nx=1\\n[a.x.y.z]\\ny=2\\n]\n"
    "toml OK 14\n"
    "  P#1=a N=++ M=0 I#0=\n"
    "  P#2=a|b N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=x M=0 I#1=1\n"
    "  P#3=a|b|c N=-- M=0 I#0=\n"
    "  P#2=a|b N=-- M=0 I#0=\n"
    "  P#2=a|x N=++ M=0 I#0=\n"
    "  P#3=a|x|y N=++ M=0 I#0=\n"
    "  P#4=a|x|y|z N=++ M=0 I#0=\n"
    "  P#4=a|x|y|z N=y M=0 I#1=2\n"
    "  P#4=a|x|y|z N=-- M=0 I#0=\n"
    "  P#3=a|x|y N=-- M=0 I#0=\n"
    "  P#2=a|x N=-- M=0 I#0=\n"
    "  P#1=a N=-- M=0 I#0=\n"
    "ini OK 14\n"
    "  P#1=a N=++ M=0 I#0=\n"
    "  P#2=a|b N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=x M=0 I#1=1\n"
    "  P#3=a|b|c N=-- M=0 I#0=\n"
    "  P#2=a|b N=-- M=0 I#0=\n"
    "  P#2=a|x N=++ M=0 I#0=\n"
    "  P#3=a|x|y N=++ M=0 I#0=\n"
    "  P#4=a|x|y|z N=++ M=0 I#0=\n"
    "  P#4=a|x|y|z N=y M=0 I#1=2\n"
    "  P#4=a|x|y|z N=-- M=0 I#0=\n"
    "  P#3=a|x|y N=-- M=0 I#0=\n"
    "  P#2=a|x N=-- M=0 I#0=\n"
    "  P#1=a N=-- M=0 I#0=\n"
    "T97 [[a.b.c.d]\\nx=1\\n[a.b.c.d.e]\\ny=2\\n]\n"
    "toml OK 12\n"
    "  P#1=a N=++ M=0 I#0=\n"
    "  P#2=a|b N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=++ M=0 I#0=\n"
    "  P#4=a|b|c|d N=++ M=0 I#0=\n"
    "  P#4=a|b|c|d N=x M=0 I#1=1\n"
    "  P#5=a|b|c|d|e N=++ M=0 I#0=\n"
    "  P#5=a|b|c|d|e N=y M=0 I#1=2\n"
    "  P#5=a|b|c|d|e N=-- M=0 I#0=\n"
    "  P#4=a|b|c|d N=-- M=0 I#0=\n"
    "  P#3=a|b|c N=-- M=0 I#0=\n"
    "  P#2=a|b N=-- M=0 I#0=\n"
    "  P#1=a N=-- M=0 I#0=\n"
    "ini OK 12\n"
    "  P#1=a N=++ M=0 I#0=\n"
    "  P#2=a|b N=++ M=0 I#0=\n"
    "  P#3=a|b|c N=++ M=0 I#0=\n"
    "  P#4=a|b|c|d N=++ M=0 I#0=\n"
    "  P#4=a|b|c|d N=x M=0 I#1=1\n"
    "  P#5=a|b|c|d|e N=++ M=0 I#0=\n"
    "  P#5=a|b|c|d|e N=y M=0 I#1=2\n"
    "  P#5=a|b|c|d|e N=-- M=0 I#0=\n"
    "  P#4=a|b|c|d N=-- M=0 I#0=\n"
    "  P#3=a|b|c N=-- M=0 I#0=\n"
    "  P#2=a|b N=-- M=0 I#0=\n"
    "  P#1=a N=-- M=0 I#0=\n"
    "T98 [-- = 1\\n[s.t]\\nx=2\\n]\n"
    "toml OK 5\n"
    "  P#1=s N=++ M=0 I#0=\n"
    "  P#2=s|t N=++ M=0 I#0=\n"
    "  P#2=s|t N=x M=0 I#1=2\n"
    "  P#2=s|t N=-- M=0 I#0=\n"
    "  P#1=s N=-- M=0 I#0=\n"
    "ini OK 5\n"
    "  P#1=s N=++ M=0 I#0=\n"
    "  P#2=s|t N=++ M=0 I#0=\n"
    "  P#2=s|t N=x M=0 I#1=2\n"
    "  P#2=s|t N=-- M=0 I#0=\n"
    "  P#1=s N=-- M=0 I#0=\n"
    "T99 [a.b.-- = 2\\n[c]\\nx=1\\n]\n"
    "toml OK 5\n"
    "  P#2=a|b N=-- M=0 I#1=2\n"
    "  P#1=a N=-- M=0 I#1=2\n"
    "  P#1=c N=++ M=0 I#0=\n"
    "  P#1=c N=x M=0 I#1=1\n"
    "  P#1=c N=-- M=0 I#0=\n"
    "ini OK 5\n"
    "  P#2=a|b N=-- M=0 I#1=2\n"
    "  P#1=a N=-- M=0 I#1=2\n"
    "  P#1=c N=++ M=0 I#0=\n"
    "  P#1=c N=x M=0 I#1=1\n"
    "  P#1=c N=-- M=0 I#0=\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// HEADER_EDGES
//   constant: what the cases of d_tests_arg_config_header_edges come to, as the
// reference writes them with CLI11.
static const char HEADER_EDGES[] =
    "T100 [[.a]\\nb=1\\n[c]\\n.d = 2\\n]\n"
    "toml OK 8\n"
    "  P#1= N=++ M=0 I#0=\n"
    "  P#2=|a N=++ M=0 I#0=\n"
    "  P#2=|a N=b M=0 I#1=1\n"
    "  P#2=|a N=-- M=0 I#0=\n"
    "  P#1= N=-- M=0 I#0=\n"
    "  P#1=c N=++ M=0 I#0=\n"
    "  P#2=c| N=d M=0 I#1=2\n"
    "  P#1=c N=-- M=0 I#0=\n"
    "ini OK 8\n"
    "  P#1= N=++ M=0 I#0=\n"
    "  P#2=|a N=++ M=0 I#0=\n"
    "  P#2=|a N=b M=0 I#1=1\n"
    "  P#2=|a N=-- M=0 I#0=\n"
    "  P#1= N=-- M=0 I#0=\n"
    "  P#1=c N=++ M=0 I#0=\n"
    "  P#2=c| N=d M=0 I#1=2\n"
    "  P#1=c N=-- M=0 I#0=\n"
    "T101 [[[a]\\nx=1\\n[b]]\\ny=2\\n]\n"
    "toml OK 6\n"
    "  P#1=[a N=++ M=0 I#0=\n"
    "  P#1=[a N=x M=0 I#1=1\n"
    "  P#1=[a N=-- M=0 I#0=\n"
    "  P#1=b] N=++ M=0 I#0=\n"
    "  P#1=b] N=y M=0 I#1=2\n"
    "  P#1=b] N=-- M=0 I#0=\n"
    "ini OK 6\n"
    "  P#1=[a N=++ M=0 I#0=\n"
    "  P#1=[a N=x M=0 I#1=1\n"
    "  P#1=[a N=-- M=0 I#0=\n"
    "  P#1=b] N=++ M=0 I#0=\n"
    "  P#1=b] N=y M=0 I#1=2\n"
    "  P#1=b] N=-- M=0 I#0=\n"
    "T102 [[a#b]\\nx=1\\n[c] # d\\ny=2\\n[e;f]\\nz=3\\n]\n"
    "toml OK 9\n"
    "  P#1=a#b N=++ M=0 I#0=\n"
    "  P#1=a#b N=x M=0 I#1=1\n"
    "  P#1=a#b N=-- M=0 I#0=\n"
    "  P#1=c N=++ M=0 I#0=\n"
    "  P#1=c N=y M=0 I#1=2\n"
    "  P#1=c N=-- M=0 I#0=\n"
    "  P#1=e;f N=++ M=0 I#0=\n"
    "  P#1=e;f N=z M=0 I#1=3\n"
    "  P#1=e;f N=-- M=0 I#0=\n"
    "ini OK 8\n"
    "  P#1=a#b N=++ M=0 I#0=\n"
    "  P#1=a#b N=x M=0 I#1=1\n"
    "  P#1=a#b N=[c] # d M=0 I#1=true\n"
    "  P#1=a#b N=y M=0 I#1=2\n"
    "  P#1=a#b N=-- M=0 I#0=\n"
    "  P#1=e;f N=++ M=0 I#0=\n"
    "  P#1=e;f N=z M=0 I#1=3\n"
    "  P#1=e;f N=-- M=0 I#0=\n"
    "T103 [[a #c\\nx=1\\n]\n"
    "toml OK 1\n"
    "  P#0= N=x M=0 I#1=1\n"
    "ini OK 2\n"
    "  P#0= N=[a #c M=0 I#1=true\n"
    "  P#0= N=x M=0 I#1=1\n"
    "T104 [[a \"#\"\\n]\n"
    "toml OK 1\n"
    "  P#0= N=[a \" M=0 I#1=true\n"
    "ini OK 1\n"
    "  P#0= N=[a \"#\" M=0 I#1=true\n"
    "T121 [[a] [b\\nx=1\\n[c]\\ny=2\\n]\n"
    "toml comment [ OK 4\n"
    "  P#0= N=x M=0 I#1=1\n"
    "  P#1=c N=++ M=0 I#0=\n"
    "  P#1=c N=y M=0 I#1=2\n"
    "  P#1=c N=-- M=0 I#0=\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// MULTILINE_EDGES
//   constant: what the cases of d_tests_arg_config_multiline_edges come to, as
// the reference writes them with CLI11.
static const char MULTILINE_EDGES[] =
    "T105 [m = \"\"\"a\\\\\"\"\"\\nb\"\"\"\\nn = \"\"\"c\\\\\\\\\"\"\"\\no = "
    "\"\"\"d\\\\\\\\\\\\\"\"\"\\ne\"\"\"\\np = '''f\\\\'''\\nq = 1\\n]\n"
    "toml OK 5\n"
    "  P#0= N=m M=0 I#1=a\"\"\"\\nb\n"
    "  P#0= N=n M=0 I#1=c\\\\\n"
    "  P#0= N=o M=0 I#1=d\\\\\"\"\"\\ne\n"
    "  P#0= N=p M=0 I#1=f\\\\\n"
    "  P#0= N=q M=0 I#1=1\n"
    "ini OK 5\n"
    "  P#0= N=m M=0 I#1=a\"\"\"\\nb\n"
    "  P#0= N=n M=0 I#1=c\\\\\n"
    "  P#0= N=o M=0 I#1=d\\\\\"\"\"\\ne\n"
    "  P#0= N=p M=0 I#1=f\\\\\n"
    "  P#0= N=q M=0 I#1=1\n"
    "T106 [m1 = \"\"\"\\\\\\nx\"\"\"\\nm2 = \"\"\"\\n\\n\\nx\"\"\"\\nm3 = "
    "\"\"\"\\nx\"\"\"\\nm4 = \"\"\"y\\nx\"\"\"\\nm5 = \"\"\"\\n\"\"\"\\nm6 = "
    "\"\"\"\\n\\n\"\"\"\\nm7 = '''\"y\"'''\\nm8 = \"\"\"x # y\\nz\"\"\"\\n]\n"
    "toml OK 8\n"
    "  P#0= N=m1 M=0 I#1=x\n"
    "  P#0= N=m2 M=0 I#1=\\n\\nx\n"
    "  P#0= N=m3 M=0 I#1=x\n"
    "  P#0= N=m4 M=0 I#1=y\\nx\n"
    "  P#0= N=m5 M=0 I#1=\n"
    "  P#0= N=m6 M=0 I#1=\n"
    "  P#0= N=m7 M=0 I#1=\"y\"\n"
    "  P#0= N=m8 M=0 I#1=x # y\\nz\n"
    "ini OK 8\n"
    "  P#0= N=m1 M=0 I#1=x\n"
    "  P#0= N=m2 M=0 I#1=\\n\\nx\n"
    "  P#0= N=m3 M=0 I#1=x\n"
    "  P#0= N=m4 M=0 I#1=y\\nx\n"
    "  P#0= N=m5 M=0 I#1=\n"
    "  P#0= N=m6 M=0 I#1=\n"
    "  P#0= N=m7 M=0 I#1=\"y\"\n"
    "  P#0= N=m8 M=0 I#1=x # y\\nz\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// QUOTE_EDGES
//   constant: what the cases of d_tests_arg_config_quote_edges come to, as the
// reference writes them with CLI11.
static const char QUOTE_EDGES[] =
    "T107 [a = \"\\nb = '\\nc = `\\nd = x\\n]\n"
    "toml OK 4\n"
    "  P#0= N=a M=0 I#1=\"\n"
    "  P#0= N=b M=0 I#1='\n"
    "  P#0= N=c M=0 I#1=`\n"
    "  P#0= N=d M=0 I#1=x\n"
    "ini OK 4\n"
    "  P#0= N=a M=0 I#1=\"\n"
    "  P#0= N=b M=0 I#1='\n"
    "  P#0= N=c M=0 I#1=`\n"
    "  P#0= N=d M=0 I#1=x\n"
    "T108 [a.'b=c' = 1\\nab\"c=d\" = 2\\n'e' 'f=g' = 3\\n]\n"
    "toml OK 3\n"
    "  P#1=a N=b=c M=0 I#1=1\n"
    "  P#0= N=ab\"c M=0 I#3=d\"|=|2\n"
    "  P#0= N=e' 'f=g M=0 I#1=3\n"
    "ini OK 3\n"
    "  P#1=a N=b=c M=0 I#1=1\n"
    "  P#0= N=ab\"c M=0 I#3=d\"|=|2\n"
    "  P#0= N=e' 'f=g M=0 I#1=3\n"
    "T109 [\"a=b\"\\n\"c#d\"\\n]\n"
    "toml OK 2\n"
    "  P#0= N=a=b M=0 I#1=true\n"
    "  P#0= N=c#d M=0 I#1=true\n"
    "ini OK 2\n"
    "  P#0= N=a=b M=0 I#1=true\n"
    "  P#0= N=c#d M=0 I#1=true\n"
    "T110 [=55\\n= 6\\n]\n"
    "toml OK 2\n"
    "  P#0= N==55 M=0 I#1=true\n"
    "  P#0= N== 6 M=0 I#1=true\n"
    "ini OK 2\n"
    "  P#0= N==55 M=0 I#1=true\n"
    "  P#0= N== 6 M=0 I#1=true\n"
    "T111 [\"a=b\" c\\n]\n"
    "toml OK 1\n"
    "  P#0= N=\"a M=0 I#2=b\"|c\n"
    "ini OK 1\n"
    "  P#0= N=\"a M=0 I#2=b\"|c\n"
    "T112 [\"[x]\" = 1\\n'[y]' = 2\\n]\n"
    "toml OK 2\n"
    "  P#0= N=[x] M=0 I#1=1\n"
    "  P#0= N=[y] M=0 I#1=2\n"
    "ini OK 2\n"
    "  P#0= N=[x] M=0 I#1=1\n"
    "  P#0= N=[y] M=0 I#1=2\n"
    "T113 [a = \"[]\"\\nb = \"x]\"\\nc = \"[x\"\\nd = '[]'\\ne = \"\"\\n]\n"
    "toml OK 5\n"
    "  P#0= N=a M=0 I#1=[[]]\n"
    "  P#0= N=b M=0 I#1=x]\n"
    "  P#0= N=c M=0 I#1=[x\n"
    "  P#0= N=d M=0 I#1=[[]]\n"
    "  P#0= N=e M=0 I#1=\n"
    "ini OK 5\n"
    "  P#0= N=a M=0 I#1=[[]]\n"
    "  P#0= N=b M=0 I#1=x]\n"
    "  P#0= N=c M=0 I#1=[x\n"
    "  P#0= N=d M=0 I#1=[[]]\n"
    "  P#0= N=e M=0 I#1=\n"
    "T120 [a = 1\\nh =\\n]\n"
    "toml OK 2\n"
    "  P#0= N=a M=0 I#1=1\n"
    "  P#0= N=h M=0 I#1=\n"
    "ini OK 2\n"
    "  P#0= N=a M=0 I#1=1\n"
    "  P#0= N=h M=0 I#1=\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// LIST_EDGES
//   constant: what the cases of d_tests_arg_config_list_edges come to, as the
// reference writes them with CLI11.
static const char LIST_EDGES[] =
    "T114 [a = |\\nb = ||\\nc = |x|\\n]\n"
    "toml | , | OK 3\n"
    "  P#0= N=a M=0 I#0=\n"
    "  P#0= N=b M=0 I#0=\n"
    "  P#0= N=c M=0 I#1=x\n"
    "T115 [a = ,b\\nc = d,\\ne = ,\\n]\n"
    "toml OK 3\n"
    "  P#0= N=a M=0 I#2=|b\n"
    "  P#0= N=c M=0 I#1=d\n"
    "  P#0= N=e M=0 I#1=\n"
    "ini OK 3\n"
    "  P#0= N=a M=0 I#2=|b\n"
    "  P#0= N=c M=0 I#1=d\n"
    "  P#0= N=e M=0 I#1=\n"
    "T116 [a = [1 2 3]\\nb = 1 2\\nc = 1,2\\n]\n"
    "toml [ space ] OK 3\n"
    "  P#0= N=a M=0 I#3=1|2|3\n"
    "  P#0= N=b M=0 I#1=1 2\n"
    "  P#0= N=c M=0 I#1=1,2\n"
    "toml OK 3\n"
    "  P#0= N=a M=0 I#1=1 2 3\n"
    "  P#0= N=b M=0 I#2=1|2\n"
    "  P#0= N=c M=0 I#2=1|2\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// MERGES
//   constant: what the cases of d_tests_arg_config_merges come to, as the
// reference writes them with CLI11.
static const char MERGES[] =
    "T117 [a.x = 1\\nb.x = 2\\n]\n"
    "toml OK 2\n"
    "  P#1=a N=x M=0 I#1=1\n"
    "  P#1=b N=x M=0 I#1=2\n"
    "ini OK 2\n"
    "  P#1=a N=x M=0 I#1=1\n"
    "  P#1=b N=x M=0 I#1=2\n"
    "T118 [a = 1, \"\"\\na = 3, 4\\nb = 1, 2\\nb = \"\", 4\\nc = 1, %%\\nc = "
    "3, 4\\n]\n"
    "toml OK 3\n"
    "  P#0= N=a M=0 I#4=1||3|4\n"
    "  P#0= N=b M=0 I#4=1|2||4\n"
    "  P#0= N=c M=0 I#4=1|%%|3|4\n"
    "ini OK 3\n"
    "  P#0= N=a M=0 I#4=1||3|4\n"
    "  P#0= N=b M=0 I#4=1|2||4\n"
    "  P#0= N=c M=0 I#4=1|%%|3|4\n"
    "toml duplicates OK 3\n"
    "  P#0= N=a M=0 I#4=1||3|4\n"
    "  P#0= N=b M=0 I#4=1|2||4\n"
    "  P#0= N=c M=0 I#4=1|%%|3|4\n"
    "ini duplicates OK 3\n"
    "  P#0= N=a M=0 I#4=1||3|4\n"
    "  P#0= N=b M=0 I#4=1|2||4\n"
    "  P#0= N=c M=0 I#4=1|%%|3|4\n"
    "T119 [a = 1\\na = 3, 4\\nb = 1, 2\\nb = 3\\n]\n"
    "toml OK 2\n"
    "  P#0= N=a M=0 I#3=1|3|4\n"
    "  P#0= N=b M=0 I#3=1|2|3\n"
    "ini OK 2\n"
    "  P#0= N=a M=0 I#3=1|3|4\n"
    "  P#0= N=b M=0 I#3=1|2|3\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// DEPARTED_READS
//   constant: what the reads of d_tests_arg_config_departed_reads come to,
// the module's own (ISSUES 14): a value that is not there is the empty
// value, and no `%%` goes between a side with no values and the other.
// CLI11 takes the first or last element of an empty list on each of these
// texts -- undefined behaviour, which in the probes is a crash.
static const char DEPARTED_READS[] =
    "T05 [e = \"\"\\nf = ''\\ng =\\nh = \\n]\n"
    "toml comment = OK 4\n"
    "  P#0= N=e M=0 I#1=\n"
    "  P#0= N=f M=0 I#1=\n"
    "  P#0= N=g M=0 I#1=\n"
    "  P#0= N=h M=0 I#1=\n"
    "ini comment = OK 4\n"
    "  P#0= N=e M=0 I#1=\n"
    "  P#0= N=f M=0 I#1=\n"
    "  P#0= N=g M=0 I#1=\n"
    "  P#0= N=h M=0 I#1=\n"
    "T63 [a = []\\na = 1\\n]\n"
    "toml duplicates OK 1\n"
    "  P#0= N=a M=0 I#1=1\n"
    "ini duplicates OK 1\n"
    "  P#0= N=a M=0 I#1=1\n"
    "T64 [a = 1\\na = []\\n]\n"
    "toml duplicates OK 1\n"
    "  P#0= N=a M=0 I#1=1\n"
    "ini duplicates OK 1\n"
    "  P#0= N=a M=0 I#1=1\n"
    "T90 [a = 1\\na = [\\n]\n"
    "toml duplicates OK 1\n"
    "  P#0= N=a M=0 I#1=1\n"
    "T91 [[s]\\n++ = 1\\n]\n"
    "toml duplicates OK 2\n"
    "  P#1=s N=++ M=0 I#1=1\n"
    "  P#1=s N=-- M=0 I#0=\n"
    "T120 [a = 1\\nh =\\n]\n"
    "toml comment = OK 2\n"
    "  P#0= N=a M=0 I#1=1\n"
    "  P#0= N=h M=0 I#1=\n"
    "ini comment = OK 2\n"
    "  P#0= N=a M=0 I#1=1\n"
    "  P#0= N=h M=0 I#1=\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

// ITEMS
//   constant: what the cases of d_tests_arg_config_items come to, as the
// reference writes them with CLI11.
static const char ITEMS[] =
    "I01 P#0= N=a M=0 I#0=\n"
    "  fullname a\n"
    "  flag {}\n"
    "I02 P#2=s|t N=a M=0 I#1=x\n"
    "  fullname s.t.a\n"
    "  flag x\n"
    "I03 P#1=s N=b M=0 I#2=x|y\n"
    "  fullname s.b\n"
    "  flag ERR ConversionError 104\n"
    "I04 P#0= N=c M=0 I#1=\n"
    "  fullname c\n"
    "  flag \n"
    "I05 P#1= N= M=0 I#3=1|%%|2\n"
    "  fullname \n"
    "  flag ERR ConversionError 104\n"
    "I06 P#1=s.t N=n\\x00m M=0 I#1=\\x00\n"
    "  fullname s.t.n\\x00m\n"
    "  flag \\x00\n"
    "I07 P#3=a|b|c N=-- M=0 I#0=\n"
    "  fullname a.b.c.--\n"
    "  flag {}\n";

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// FILE_OPTIONS
//   constant: what the cases of d_tests_arg_config_file_options come to, as the
// reference writes them with CLI11.
static const char FILE_OPTIONS[] =
    "E01 [--config E01.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E01.cfg]\n"
    "opt a n=1 res=[1]\n"
    "opt b n=1 res=[true]\n"
    "opt v n=3 res=[1|2|3]\n"
    "E02 [--config E02.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E02.cfg]\n"
    "opt b n=1 res=[false]\n"
    "opt c n=1 res=[2]\n"
    "opt d n=1 res=[true]\n"
    "opt e n=3 res=[true|false|3]\n"
    "E03 [--config E03.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E03.cfg]\n"
    "opt x n=1 res=[5]\n"
    "opt y n=1 res=[3]\n"
    "opt z n=1 res=[4]\n"
    "E04 [--config E04.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E04.cfg]\n"
    "opt pos n=1 res=[7]\n"
    "opt opt n=1 res=[8]\n"
    "E13 [--a 5 --config E13.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--a|--config]\n"
    "opt config n=1 res=[E13.cfg]\n"
    "opt a n=1 res=[5]\n"
    "opt b n=1 res=[2]\n"
    "E14 [--config E14.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E14.cfg]\n"
    "opt v n=4 res=[1|2|3|4]\n"
    "opt w n=5 res=[1|2|%%|3|4]\n"
    "E18 [--config E18a.cfg --config E18b.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config|--config]\n"
    "opt config n=2 res=[E18a.cfg|E18b.cfg]\n"
    "opt a n=1 res=[2]\n"
    "opt b n=1 res=[3]\n"
    "opt c n=1 res=[y]\n"
    "E32 [--config E32.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E32.cfg]\n"
    "opt v n=1 res=[{}]\n"
    "opt w n=3 res=[a|%%|b]\n"
    "E34 [--config E34.cfg]\n"
    "ERR ArgumentMismatch 114: --p: At most 2 required but received 3\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E34.cfg]\n"
    "opt p n=3 res=[1|2|3]\n"
    "E36 [--config E36.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E36.cfg]\n"
    "opt a n=1 res=[line\\tone\\nline two]\n"
    "opt b n=1 res=[raw\\\\t\\n  two]\n"
    "E43 [--config E43.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E43.cfg]\n"
    "opt a n=1 res=[1]\n"
    "opt b n=0 res=[]\n"
    "E44 [--config E44.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E44.cfg]\n"
    "opt a n=0 res=[]\n"
    "opt f n=0 res=[]\n"
    "E49 [--config E49.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E49.cfg]\n"
    "opt a n=2 res=[x|y]\n"
    "opt j n=2 res=[p|q]\n"
    "E50 [--config E50.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E50.cfg]\n"
    "opt a n=1 res=[1]\n"
    "opt b n=1 res=[2 3]\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// FILE_VALUES
//   constant: what the cases of d_tests_arg_config_file_values come to, as the
// reference writes them with CLI11.
static const char FILE_VALUES[] =
    "E20 [--config E20.cfg]\n"
    "ERR ArgumentMismatch 114: d was given a disallowed flag override\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E20.cfg]\n"
    "opt b n=1 res=[true]\n"
    "opt c n=1 res=[true]\n"
    "opt d n=0 res=[]\n"
    "E21 [--config E21.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E21.cfg]\n"
    "opt b n=1 res=[true]\n"
    "E22 [--config E22.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E22.cfg]\n"
    "opt b n=3 res=[1|1|true]\n"
    "opt c n=2 res=[1|1]\n"
    "E23 [--config E23.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E23.cfg]\n"
    "opt f n=1 res=[true]\n"
    "opt g n=1 res=[false]\n"
    "opt h n=1 res=[7]\n"
    "opt k n=1 res=[false]\n"
    "E30 [--config E30.cfg]\n"
    "ERR ParseError 111: unrecognized escape sequence \\\\q in bad\\\\q\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E30.cfg]\n"
    "opt a n=0 res=[]\n"
    "E38 [--config E38.cfg]\n"
    "ERR ConversionError 104: b: too many inputs for a flag\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E38.cfg]\n"
    "opt b n=0 res=[]\n"
    "E40 [--config E40.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E40.cfg]\n"
    "opt name n=1 res=[tab\\there\xc3\xa9]\n"
    "opt bin n=1 res=[A\\x00B]\n"
    "E46 [--config E46.cfg]\n"
    "ERR ValidationError 105: --a: Value 9 not in range [1 - 5]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E46.cfg]\n"
    "opt a n=1 res=[9]\n"
    "E59 [--config E59.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E59.cfg]\n"
    "opt a n=0 res=[]\n"
    "opt b n=0 res=[]\n"
    "opt v n=1 res=[{}]\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// FILE_COMMANDS
//   constant: what the cases of d_tests_arg_config_file_commands come to, as
// the reference writes them with CLI11.
static const char FILE_COMMANDS[] =
    "E05 [--config E05.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E05.cfg]\n"
    "opt x n=1 res=[1]\n"
    "opt a n=1 res=[0]\n"
    "E06 [--config E06.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "cmd deep n=0 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E06.cfg]\n"
    "opt y n=1 res=[2]\n"
    "opt x n=1 res=[1]\n"
    "E07 [--config E07.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "cmd deep n=0 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E07.cfg]\n"
    "opt x n=1 res=[2]\n"
    "opt y n=1 res=[3]\n"
    "E19 [--config E19.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E19.cfg]\n"
    "opt x n=1 res=[1]\n"
    "opt v n=3 res=[1|2|3]\n"
    "opt a n=1 res=[1]\n"
    "E24 [--config E24.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd grp n=1 miss=[] subs=[] order=[]\n"
    "cmd plus n=1 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E24.cfg]\n"
    "opt gx n=1 res=[1]\n"
    "opt hx n=1 res=[2]\n"
    "E25 [--config E25.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E25.cfg]\n"
    "opt a n=1 res=[1]\n"
    "opt x n=1 res=[2]\n"
    "E26 [--config E26.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E26.cfg]\n"
    "opt a n=1 res=[2]\n"
    "opt b n=1 res=[3]\n"
    "opt x n=1 res=[1]\n"
    "E27 [--config E27.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E27.cfg]\n"
    "opt x n=1 res=[1]\n"
    "E33 [--config E33.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E33.cfg]\n"
    "opt x n=1 res=[1]\n"
    "E39 [--config E39.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "cmd og n=0 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E39.cfg]\n"
    "opt gx n=1 res=[4]\n"
    "E42 [--config E42.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E42.cfg]\n"
    "opt x n=0 res=[]\n"
    "opt rx n=1 res=[1]\n"
    "E48 [sub --x 9 --config E48.cfg]\n"
    "ERR ExtrasError 109: sub: The following arguments were not expected: "
    "--config E48.cfg\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[--config|E48.cfg] subs=[] order=[--x]\n"
    "opt config n=0 res=[]\n"
    "opt a n=0 res=[]\n"
    "opt x n=1 res=[9]\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// FILE_NAMES
//   constant: what the cases of d_tests_arg_config_file_names come to, as the
// reference writes them with CLI11.
static const char FILE_NAMES[] =
    "E60 [--config E60.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E60.cfg]\n"
    "opt x n=1 res=[1]\n"
    "E61 [--config E61.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E61.cfg]\n"
    "opt x n=1 res=[1]\n"
    "E62 [--config E62.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd grp n=1 miss=[] subs=[] order=[]\n"
    "cmd insub n=0 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E62.cfg]\n"
    "opt x n=1 res=[1]\n"
    "E63 [--config E63.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd nameless n=1 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E63.cfg]\n"
    "opt a n=1 res=[1]\n"
    "opt b n=1 res=[2]\n"
    "E64 [sub --config E64.cfg]\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E64.cfg]\n"
    "opt x n=0 res=[]\n"
    "opt rx n=0 res=[]\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// FILE_SUBCOMMANDS
//   constant: what the cases of d_tests_arg_config_file_subcommands come to, as
// the reference writes them with CLI11.
static const char FILE_SUBCOMMANDS[] =
    "E35 [--config E35.cfg]\n"
    "cb-a 1\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E35.cfg]\n"
    "opt a n=1 res=[1]\n"
    "opt x n=1 res=[2]\n"
    "E51 [--config E51.cfg]\n"
    "pre-sub 2\n"
    "final-sub\n"
    "cmd app n=1 miss=[] subs=[sub] order=[--config]\n"
    "cmd sub n=1 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E51.cfg]\n"
    "opt x n=1 res=[1]\n"
    "E52 [--config E52.cfg]\n"
    "final-deep\n"
    "final-deep\n"
    "cmd app n=1 miss=[] subs=[sub|sub] order=[--config]\n"
    "cmd sub n=2 miss=[] subs=[deep] order=[]\n"
    "cmd deep n=1 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E52.cfg]\n"
    "opt y n=1 res=[2]\n"
    "opt x n=1 res=[1]\n"
    "E53 [--config E53.cfg]\n"
    "complete-sub\n"
    "cmd app n=1 miss=[] subs=[sub] order=[--config]\n"
    "cmd sub n=1 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E53.cfg]\n"
    "opt x n=1 res=[1]\n"
    "opt a n=1 res=[2]\n"
    "E54 [--config E54.cfg]\n"
    "cmd app n=1 miss=[] subs=[sub|sub] order=[--config]\n"
    "cmd sub n=2 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E54.cfg]\n"
    "opt x n=1 res=[1]\n"
    "E55 [--config E55.cfg]\n"
    "cmd app n=1 miss=[] subs=[sub|other] order=[--config]\n"
    "cmd sub n=1 miss=[] subs=[] order=[]\n"
    "cmd other n=1 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E55.cfg]\n"
    "opt x n=1 res=[1]\n"
    "opt y n=1 res=[2]\n"
    "E56 [--config E56.cfg]\n"
    "final-sub\n"
    "cmd app n=1 miss=[] subs=[sub] order=[--config]\n"
    "cmd sub n=1 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E56.cfg]\n"
    "opt x n=1 res=[1]\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// FILE_EXTRAS
//   constant: what the cases of d_tests_arg_config_file_extras come to, as the
// reference writes them with CLI11.
static const char FILE_EXTRAS[] =
    "E08 [--config E08.cfg]\n"
    "ERR ConfigError 110: a: This option is not allowed in a configuration "
    "file\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E08.cfg]\n"
    "opt a n=0 res=[]\n"
    "E09 [--config E09.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E09.cfg]\n"
    "opt a n=1 res=[1]\n"
    "E10 [--config E10.cfg]\n"
    "ERR ConfigError 110: INI was not able to parse unknown\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E10.cfg]\n"
    "opt a n=1 res=[1]\n"
    "E11 [--config E11.cfg]\n"
    "ERR ExtrasError 109: prog: The following arguments were not expected: "
    "unknown 2 list 3 4\n"
    "cmd app n=1 miss=[unknown|2|list|3|4] subs=[] order=[--config]\n"
    "opt config n=1 res=[E11.cfg]\n"
    "opt a n=1 res=[1]\n"
    "E12 [--config E12.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E12.cfg]\n"
    "opt a n=0 res=[]\n"
    "E15 [--config E15.cfg]\n"
    "ERR RequiredError 106: --b is required\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E15.cfg]\n"
    "opt a n=1 res=[1]\n"
    "opt b n=0 res=[]\n"
    "E45 [--config E45.cfg]\n"
    "ERR ConfigError 110: INI was not able to parse sub.y\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E45.cfg]\n"
    "opt a n=0 res=[]\n"
    "opt x n=1 res=[1]\n"
    "E65 [--config E65.cfg]\n"
    "ERR ConfigError 110: a: This option is not allowed in a configuration "
    "file\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E65.cfg]\n"
    "opt a n=0 res=[]\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// FILE_SOURCES
//   constant: what the cases of d_tests_arg_config_file_sources come to, as the
// reference writes them with CLI11.
static const char FILE_SOURCES[] =
    "E17 []\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E17.cfg]\n"
    "opt a n=1 res=[7]\n"
    "E41 [--config E41.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E41.cfg]\n"
    "opt a n=2 res=[1|2]\n"
    "E47 [--config E47.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E47.cfg]\n"
    "opt a n=1 res=[3]\n"
    "E66 [--config E66.cfg --config E66x.cfg]\n"
    "ERR FileError 103: E66x.cfg was not readable (missing?)\n"
    "cmd app n=1 miss=[] subs=[] order=[--config|--config]\n"
    "opt config n=2 res=[E66.cfg|E66x.cfg]\n"
    "opt a n=0 res=[]\n"
    "E67 [--config E67b.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E67b.cfg]\n"
    "opt a n=0 res=[]\n"
    "opt b n=1 res=[2]\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if ( (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1) &&                             \
      (D_INTERNAL_ARG_PARSER_ENVIRONMENT == 1) )

// FILE_ENVIRONMENT
//   constant: what the cases of d_tests_arg_config_file_environment come to, as
// the reference writes them with CLI11.
static const char FILE_ENVIRONMENT[] =
    "E16 []\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E16.cfg]\n"
    "opt a n=1 res=[6]\n"
    "E68 [--config E68b.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E68b.cfg]\n"
    "opt a n=0 res=[]\n"
    "opt b n=1 res=[2]\n"
    "E58 [--config E58.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E58.cfg]\n"
    "opt a n=1 res=[1]\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES, D_INTERNAL_ARG_PARSER_ENVIRONMENT

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// FILE_FLAGS
//   constant: what the cases of d_tests_arg_config_file_flags come to, as the
// reference writes them with CLI11.
static const char FILE_FLAGS[] =
    "E74 [--config E74.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E74.cfg]\n"
    "opt o n=1 res=[]\n"
    "opt p n=1 res=[{}]\n"
    "opt q n=1 res=[d]\n"
    "opt r n=1 res=[e]\n"
    "E75 [--config E75.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E75.cfg]\n"
    "opt f n=2 res=[true|false]\n"
    "opt g n=2 res=[1|2]\n"
    "E76 [--config E76.cfg]\n"
    "ERR InvalidError 111: invalid flag argument given: Too many positional "
    "arguments with unlimited expected args\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E76.cfg]\n"
    "opt g n=1 res=[1]\n"
    "E77 [--config E77.cfg]\n"
    "ERR ConversionError 104: a: too many inputs for a flag\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E77.cfg]\n"
    "opt a n=0 res=[]\n"
    "E78 [--config E78.cfg]\n"
    "ERR ArgumentMismatch 114: b: At most 2 required but received 3\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E78.cfg]\n"
    "opt b n=0 res=[]\n"
    "E79 [--config E79.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E79.cfg]\n"
    "opt b n=2 res=[1|2]\n"
    "E80 [--config E80.cfg]\n"
    "ERR ArgumentMismatch 114: --c: At most 1 required but received 2\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E80.cfg]\n"
    "opt c n=2 res=[true|1]\n"
    "E81 [--config E81.cfg]\n"
    "ERR ArgumentMismatch 114: d: At most 2 required but received 3\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E81.cfg]\n"
    "opt d n=0 res=[]\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// FILE_LOOKUPS
//   constant: what the cases of d_tests_arg_config_file_lookups come to, as the
// reference writes them with CLI11.
static const char FILE_LOOKUPS[] =
    "E82 [--config E82.cfg]\n"
    "ERR ConfigError 110: x: This option is not allowed in a configuration "
    "file\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E82.cfg]\n"
    "opt x n=0 res=[]\n"
    "E83 [--config E83.cfg]\n"
    "ERR ConfigError 110: pos: This option is not allowed in a configuration "
    "file\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E83.cfg]\n"
    "opt pos n=0 res=[]\n"
    "E84 [--config E84.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd grp n=1 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E84.cfg]\n"
    "opt name n=0 res=[]\n"
    "opt pname n=1 res=[1]\n"
    "E85 [--config E85.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd s n=0 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E85.cfg]\n"
    "opt z n=1 res=[1]\n"
    "E86 [--config E86.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd s n=0 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E86.cfg]\n"
    "opt ra n=1 res=[5]\n"
    "opt sa n=0 res=[]\n"
    "E87 [--config E87.cfg]\n"
    "ERR ConfigError 110: s.a: This option is not allowed in a configuration "
    "file\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd s n=0 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E87.cfg]\n"
    "opt ra n=0 res=[]\n"
    "opt sa n=0 res=[]\n"
    "E100 [--config E100.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd s n=0 miss=[] subs=[] order=[]\n"
    "cmd g n=0 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E100.cfg]\n"
    "opt px n=0 res=[]\n"
    "opt lx n=0 res=[]\n"
    "opt rx n=1 res=[1]\n"
    "E101 [--config E101.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd g n=1 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E101.cfg]\n"
    "opt lx n=1 res=[1]\n"
    "opt sx n=0 res=[]\n"
    "E102 [--config E102.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E102.cfg]\n"
    "opt x n=1 res=[1]\n"
    "E103 [--config E103.cfg]\n"
    "ERR ConfigError 110: INI was not able to parse nosub.++\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E103.cfg]\n"
    "opt a n=1 res=[1]\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// FILE_MARKS
//   constant: what the cases of d_tests_arg_config_file_marks come to, as the
// reference writes them with CLI11.
static const char FILE_MARKS[] =
    "E88 [--config E88.cfg]\n"
    "cmd app n=2 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E88.cfg]\n"
    "opt a n=1 res=[2]\n"
    "E90 [--b 2 --config E90.cfg]\n"
    "cb-a 1\n"
    "cb-b 2\n"
    "cmd app n=1 miss=[] subs=[] order=[--b|--config]\n"
    "opt config n=1 res=[E90.cfg]\n"
    "opt b n=1 res=[2]\n"
    "opt a n=1 res=[1]\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// FILE_NAMED
//   constant: what the cases of d_tests_arg_config_file_named come to, as the
// reference writes them with CLI11.
static const char FILE_NAMED[] =
    "E91 [--config {}]\n"
    "ERR FileError 103: {} was not readable (missing?)\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[{}]\n"
    "opt a n=0 res=[]\n"
    "E92 [--config %% --config {}]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config|--config]\n"
    "opt config n=2 res=[%%|{}]\n"
    "opt a n=1 res=[1]\n"
    "E93 [--config {} --config %%]\n"
    "ERR FileError 103: %% was not readable (missing?)\n"
    "cmd app n=1 miss=[] subs=[] order=[--config|--config]\n"
    "opt config n=2 res=[{}|%%]\n"
    "opt a n=0 res=[]\n"
    "E94 [--config x --config {}]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config|--config]\n"
    "opt config n=2 res=[x|{}]\n"
    "opt a n=1 res=[1]\n"
    "E95 [--config ]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[]\n"
    "opt a n=0 res=[]\n"
    "E104 [--config  --config {}]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config|--config]\n"
    "opt config n=2 res=[|{}]\n"
    "opt a n=1 res=[1]\n"
    "E105 []\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[{}]\n"
    "opt a n=1 res=[1]\n"
    "E98 []\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt config n=0 res=[]\n"
    "opt a n=0 res=[]\n"
    "E99 []\n"
    "ERR ParseError 111: unrecognized escape sequence \\\\q in bad\\\\q\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E99.cfg]\n"
    "opt a n=0 res=[]\n"
    "E109 [--config {}]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[{}]\n"
    "opt a n=0 res=[]\n"
    "E110 [--config  --config {}]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config|--config]\n"
    "opt config n=2 res=[|{}]\n"
    "opt a n=1 res=[1]\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if ( (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1) &&                             \
      (D_INTERNAL_ARG_PARSER_ENVIRONMENT == 1) )

// FILE_VARIABLES
//   constant: what the cases of d_tests_arg_config_file_variables come to, as
// the reference writes them with CLI11.
static const char FILE_VARIABLES[] =
    "E89 [--config E89.cfg]\n"
    "complete-sub\n"
    "cmd app n=1 miss=[] subs=[sub] order=[--config]\n"
    "cmd sub n=1 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E89.cfg]\n"
    "opt x n=1 res=[1]\n"
    "opt y n=0 res=[]\n"
    "E96 []\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[c]\n"
    "opt a n=1 res=[3]\n"
    "E97 []\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt config n=0 res=[]\n"
    "opt a n=0 res=[]\n"
    "E106 []\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[{}]\n"
    "opt a n=1 res=[1]\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES, D_INTERNAL_ARG_PARSER_ENVIRONMENT

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// FILE_PERMISSIONS
//   constant: what the cases of d_tests_arg_config_file_permissions come to
// where the files can be read, as by root or on Windows; the reference run
// so.
static const char FILE_PERMISSIONS[] =
    "E107 [--config E107.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E107.cfg]\n"
    "opt a n=1 res=[1]\n"
    "E108 []\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt config n=1 res=[E108.cfg]\n"
    "opt a n=1 res=[2]\n";

// FILE_PERMISSIONS_DENIED
//   constant: FILE_PERMISSIONS where the files cannot be read; the
// reference run by a user who cannot read them.
static const char FILE_PERMISSIONS_DENIED[] =
    "E107 [--config E107.cfg]\n"
    "ERR FileError 103: E107.cfg was not readable (missing?)\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E107.cfg]\n"
    "opt a n=0 res=[]\n"
    "E108 []\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt config n=0 res=[]\n"
    "opt a n=0 res=[]\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// STREAMS
//   constant: what the cases of d_tests_arg_config_streams come to, as the
// reference writes them with CLI11.
static const char STREAMS[] =
    "E28 [--a 1]\n"
    "cmd app n=1 miss=[] subs=[] order=[--a]\n"
    "opt a n=1 res=[1]\n"
    "opt b n=0 res=[]\n"
    "opt c n=0 res=[]\n"
    "E28 stream [b=true\\na=2\\nc=3\\n]\n"
    "cmd app n=2 miss=[] subs=[] order=[--a]\n"
    "opt a n=1 res=[1]\n"
    "opt b n=1 res=[true]\n"
    "opt c n=1 res=[3]\n"
    "E29 stream [a=1\\n[sub]\\nx=2\\n]\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt a n=1 res=[1]\n"
    "opt x n=1 res=[2]\n"
    "E31 stream [\"bad\\\\qname\".x=1\\n]\n"
    "ERR ParseError 111: unrecognized escape sequence \\\\q in bad\\\\qname\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "opt a n=0 res=[]\n"
    "E57 [--a 1]\n"
    "cmd app n=1 miss=[] subs=[] order=[--a]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt x n=0 res=[]\n"
    "opt a n=1 res=[1]\n"
    "E57 stream [[sub]\\nx=5\\n]\n"
    "cmd app n=2 miss=[] subs=[sub] order=[--a]\n"
    "cmd sub n=1 miss=[] subs=[] order=[]\n"
    "opt x n=1 res=[5]\n"
    "opt a n=1 res=[1]\n"
    "E69 stream [v = 1 2 3\\ns = 'a b' ; c\\n]\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt v n=3 res=[1|2|3]\n"
    "opt s n=1 res=[a b]\n"
    "E70 stream [a=1\\n]\n"
    "ERR ConfigError 110: a: This option is not allowed in a configuration "
    "file\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "opt a n=0 res=[]\n"
    "E111 stream [a=1\\n]\n"
    "ERR InvalidError 111: Required min options greater than required max "
    "options\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "opt a n=0 res=[]\n"
    "E112 stream []\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd s n=0 miss=[] subs=[] order=[]\n"
    "opt x n=0 res=[]\n"
    "E113 []\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt a n=0 res=[]\n"
    "E113 stream [a=1\\n]\n"
    "ERR RequiredError 106: Requires at least 2 options used but only 1 were "
    "given from [--a]\n"
    "cmd app n=2 miss=[] subs=[] order=[]\n"
    "opt a n=1 res=[1]\n"
    "E114 stream [a=1\\n]\n"
    "pre-app 1\n"
    "complete-app\n"
    "final-app\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt a n=1 res=[1]\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// TEXT_MODE
//   constant: what the cases of d_tests_arg_config_text_mode come to, as the
// reference writes them with CLI11.
static const char TEXT_MODE[] =
    "E37 [--config E37.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E37.cfg]\n"
    "opt a n=1 res=[x\\x0d\\ny]\n"
    "opt b n=1 res=[2]\n"
    "E71 [--config E71.cfg]\n"
    "ERR ParseError 111: unrecognized escape sequence \\\\\\x0d in x "
    "\\\\\\x0d\\n  y\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E71.cfg]\n"
    "opt a n=0 res=[]\n"
    "opt b n=0 res=[]\n"
    "E72 [--config E72.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E72.cfg]\n"
    "opt a n=1 res=[1]\n"
    "opt b n=1 res=[2]\n"
    "E73 [--config E73.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E73.cfg]\n"
    "opt a n=1 res=[1]\n"
    "opt b n=0 res=[]\n";

// TEXT_MODE_WINDOWS
//   constant: TEXT_MODE where a file is read as text, as on Windows, where
// CLI11's std::ifstream and the module's fopen drop a carriage return
// before a line feed and end the text at a Ctrl-Z; the reference built for
// Windows and run under Wine.
static const char TEXT_MODE_WINDOWS[] =
    "E37 [--config E37.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E37.cfg]\n"
    "opt a n=1 res=[x\\ny]\n"
    "opt b n=1 res=[2]\n"
    "E71 [--config E71.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E71.cfg]\n"
    "opt a n=1 res=[x y]\n"
    "opt b n=1 res=[2]\n"
    "E72 [--config E72.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E72.cfg]\n"
    "opt a n=1 res=[1]\n"
    "opt b n=0 res=[]\n"
    "E73 [--config E73.cfg]\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[E73.cfg]\n"
    "opt a n=1 res=[1]\n"
    "opt b n=0 res=[]\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// WRITE_BASICS
//   constant: what the cases of d_tests_arg_config_write_basics come to, as the
// reference writes them with CLI11.
static const char WRITE_BASICS[] =
    "W01 [--a 1 --b --v 1 2 3 p]\n"
    "--- m0 d0\n"
    "a=1$\n"
    "b=true$\n"
    "v=[1, 2, 3, 'p']$\n"
    "--- m0 d1\n"
    "## d$\n"
    "# first option$\n"
    "a=1$\n"
    "b=true$\n"
    "v=[1, 2, 3, 'p']$\n"
    "--- m1 d0\n"
    "a=1$\n"
    "b=true$\n"
    "v=[1, 2, 3, 'p']$\n"
    "pos=\"\"$\n"
    "--- m1 d1\n"
    "## d$\n"
    "# first option$\n"
    "a=1$\n"
    "b=true$\n"
    "v=[1, 2, 3, 'p']$\n"
    "pos=\"\"$\n"
    "--- m2 d0\n"
    "a=1$\n"
    "b=true$\n"
    "v=[1, 2, 3, 'p']$\n"
    "pos=\"\"$\n"
    "--- m2 d1\n"
    "## d$\n"
    "# first option$\n"
    "a=1$\n"
    "b=true$\n"
    "v=[1, 2, 3, 'p']$\n"
    "pos=\"\"$\n"
    "W02 [--r x --n 6]\n"
    "--- m0 d0\n"
    "n=6$\n"
    "r='x'$\n"
    "--- m0 d1\n"
    "## d$\n"
    "n=6$\n"
    "r='x'$\n"
    "--- m1 d0\n"
    "s=\"abc\"$\n"
    "n=6$\n"
    "r='x'$\n"
    "f=false$\n"
    "c=\"\"$\n"
    "e=false$\n"
    "l=[1, 2]$\n"
    "--- m1 d1\n"
    "## d$\n"
    "s=\"abc\"$\n"
    "n=6$\n"
    "r='x'$\n"
    "f=false$\n"
    "c=\"\"$\n"
    "e=false$\n"
    "l=[1, 2]$\n"
    "--- m2 d0\n"
    "s=\"abc\"$\n"
    "n=6$\n"
    "r='x'$\n"
    "f=false$\n"
    "c=\"\"$\n"
    "e=false$\n"
    "l=[1, 2]$\n"
    "--- m2 d1\n"
    "## d$\n"
    "s=\"abc\"$\n"
    "n=6$\n"
    "r='x'$\n"
    "f=false$\n"
    "c=\"\"$\n"
    "e=false$\n"
    "l=[1, 2]$\n"
    "W10 [--f --nok --h -vvv]\n"
    "--- m0 d0\n"
    "f=5$\n"
    "k=false$\n"
    "h=7$\n"
    "verbose=true$\n"
    "--- m0 d1\n"
    "## d$\n"
    "f=5$\n"
    "k=false$\n"
    "h=7$\n"
    "verbose=true$\n"
    "--- m1 d0\n"
    "f=5$\n"
    "k=false$\n"
    "g=false$\n"
    "h=7$\n"
    "verbose=true$\n"
    "n=false$\n"
    "--- m1 d1\n"
    "## d$\n"
    "f=5$\n"
    "k=false$\n"
    "g=false$\n"
    "h=7$\n"
    "verbose=true$\n"
    "n=false$\n"
    "--- m2 d0\n"
    "f=5$\n"
    "k=false$\n"
    "g=false$\n"
    "h=7$\n"
    "verbose=true$\n"
    "n=false$\n"
    "--- m2 d1\n"
    "## d$\n"
    "f=5$\n"
    "k=false$\n"
    "g=false$\n"
    "h=7$\n"
    "verbose=true$\n"
    "n=false$\n"
    "W11 [--given 1 --h 2]\n"
    "--- m0 d0\n"
    "h=2$\n"
    "given=1$\n"
    "--- m0 d1\n"
    "## d$\n"
    "h=2$\n"
    "given=1$\n"
    "--- m1 d0\n"
    "#s=\"abc\"$\n"
    "#f=false$\n"
    "#g=\"\"$\n"
    "h=2$\n"
    "given=1$\n"
    "--- m1 d1\n"
    "## d$\n"
    "#s=\"abc\"$\n"
    "#f=false$\n"
    "#g=\"\"$\n"
    "h=2$\n"
    "given=1$\n"
    "--- m2 d0\n"
    "#s=\"abc\"$\n"
    "#f=false$\n"
    "#g=\"\"$\n"
    "h=2$\n"
    "given=1$\n"
    "--- m2 d1\n"
    "## d$\n"
    "#s=\"abc\"$\n"
    "#f=false$\n"
    "#g=\"\"$\n"
    "h=2$\n"
    "given=1$\n"
    "W14 [--v {} --w {}]\n"
    "--- m0 d0\n"
    "v=\"{}\"$\n"
    "w=\"{}\"$\n"
    "--- m0 d1\n"
    "## d$\n"
    "v=\"{}\"$\n"
    "w=\"{}\"$\n"
    "--- m1 d0\n"
    "v=\"{}\"$\n"
    "w=\"{}\"$\n"
    "--- m1 d1\n"
    "## d$\n"
    "v=\"{}\"$\n"
    "w=\"{}\"$\n"
    "--- m2 d0\n"
    "v=\"{}\"$\n"
    "w=\"{}\"$\n"
    "--- m2 d1\n"
    "## d$\n"
    "v=\"{}\"$\n"
    "w=\"{}\"$\n"
    "W15 [p1 -s 2 -t 3]\n"
    "--- m0 d0\n"
    "pos=\"p1\"$\n"
    "s=2$\n"
    "tee=3$\n"
    "--- m0 d1\n"
    "## d$\n"
    "pos=\"p1\"$\n"
    "s=2$\n"
    "tee=3$\n"
    "--- m1 d0\n"
    "pos=\"p1\"$\n"
    "only=\"\"$\n"
    "s=2$\n"
    "tee=3$\n"
    "--- m1 d1\n"
    "## d$\n"
    "pos=\"p1\"$\n"
    "only=\"\"$\n"
    "s=2$\n"
    "tee=3$\n"
    "--- m2 d0\n"
    "pos=\"p1\"$\n"
    "only=\"\"$\n"
    "s=2$\n"
    "tee=3$\n"
    "--- m2 d1\n"
    "## d$\n"
    "pos=\"p1\"$\n"
    "only=\"\"$\n"
    "s=2$\n"
    "tee=3$\n"
    "W16 [--n 3 4]\n"
    "--- m0 d0\n"
    "n=[3, 4]$\n"
    "--- m0 d1\n"
    "## d$\n"
    "n=[3, 4]$\n"
    "--- m1 d0\n"
    "n=[3, 4]$\n"
    "--- m1 d1\n"
    "## d$\n"
    "n=[3, 4]$\n"
    "--- m2 d0\n"
    "n=[3, 4]$\n"
    "--- m2 d1\n"
    "## d$\n"
    "n=[3, 4]$\n"
    "W17 [--a 2]\n"
    "--- m0 d0\n"
    "a=2$\n"
    "--- m0 d1\n"
    "## d$\n"
    "a=2$\n"
    "--- m1 d0\n"
    "a=2$\n"
    "--- m1 d1\n"
    "## d$\n"
    "a=2$\n"
    "--- m2 d0\n"
    "a=2$\n"
    "--- m2 d1\n"
    "## d$\n"
    "a=2$\n"
    "W19 []\n"
    "--- m0 d0\n"
    "ERR ArgumentMismatch 114: --t: At most 1 required but received 2\n"
    "--- m0 d1\n"
    "ERR ArgumentMismatch 114: --t: At most 1 required but received 2\n"
    "--- m1 d0\n"
    "ERR ArgumentMismatch 114: --t: At most 1 required but received 2\n"
    "--- m1 d1\n"
    "ERR ArgumentMismatch 114: --t: At most 1 required but received 2\n"
    "--- m2 d0\n"
    "ERR ArgumentMismatch 114: --t: At most 1 required but received 2\n"
    "--- m2 d1\n"
    "ERR ArgumentMismatch 114: --t: At most 1 required but received 2\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// WRITE_TEXT
//   constant: what the cases of d_tests_arg_config_write_text come to, as the
// reference writes them with CLI11.
static const char WRITE_TEXT[] =
    "W03 [--s hello world a\"b tab\\there  x ' 0x1F \xc3\xa9 line\\nbreak --o1 "
    "hello world --o2 a\"b --o3 tab\\there --o4  --o5 x --o6 ' --o7 0x1F --o8 "
    "0X1F --o9 0o17 --o10 0b101 --o11 0b102 --o12 1e5 --o13 nan --o14 true "
    "--o15 -1.5 --o16 1e --o17 .5 --o18 \xc3\xa9 --o19 line\\nbreak --o20 0x]\n"
    "--- m0 d0\n"
    "s=[\"hello world\", \"a\\\\\"b\", \"tab\\\\there\", \"\", 'x', \"'\", "
    "0x1F, 'B\"(\\\\xc3\\\\xa9)\"', \"line\\\\nbreak\"]$\n"
    "o1=\"hello world\"$\n"
    "o2=\"a\\\\\"b\"$\n"
    "o3=\"tab\\\\there\"$\n"
    "o4=\"\"$\n"
    "o5='x'$\n"
    "o6=\"'\"$\n"
    "o7=0x1F$\n"
    "o8=\"0X1F\"$\n"
    "o9=0o17$\n"
    "o10=0b101$\n"
    "o11=\"0b102\"$\n"
    "o12=1e5$\n"
    "o13=nan$\n"
    "o14=true$\n"
    "o15=-1.5$\n"
    "o16=\"1e\"$\n"
    "o17=.5$\n"
    "o18='B\"(\\\\xc3\\\\xa9)\"'$\n"
    "o19=\"line\\\\nbreak\"$\n"
    "o20=0x$\n"
    "--- m0 d1\n"
    "## d$\n"
    "s=[\"hello world\", \"a\\\\\"b\", \"tab\\\\there\", \"\", 'x', \"'\", "
    "0x1F, 'B\"(\\\\xc3\\\\xa9)\"', \"line\\\\nbreak\"]$\n"
    "o1=\"hello world\"$\n"
    "o2=\"a\\\\\"b\"$\n"
    "o3=\"tab\\\\there\"$\n"
    "o4=\"\"$\n"
    "o5='x'$\n"
    "o6=\"'\"$\n"
    "o7=0x1F$\n"
    "o8=\"0X1F\"$\n"
    "o9=0o17$\n"
    "o10=0b101$\n"
    "o11=\"0b102\"$\n"
    "o12=1e5$\n"
    "o13=nan$\n"
    "o14=true$\n"
    "o15=-1.5$\n"
    "o16=\"1e\"$\n"
    "o17=.5$\n"
    "o18='B\"(\\\\xc3\\\\xa9)\"'$\n"
    "o19=\"line\\\\nbreak\"$\n"
    "o20=0x$\n"
    "--- m1 d0\n"
    "s=[\"hello world\", \"a\\\\\"b\", \"tab\\\\there\", \"\", 'x', \"'\", "
    "0x1F, 'B\"(\\\\xc3\\\\xa9)\"', \"line\\\\nbreak\"]$\n"
    "o1=\"hello world\"$\n"
    "o2=\"a\\\\\"b\"$\n"
    "o3=\"tab\\\\there\"$\n"
    "o4=\"\"$\n"
    "o5='x'$\n"
    "o6=\"'\"$\n"
    "o7=0x1F$\n"
    "o8=\"0X1F\"$\n"
    "o9=0o17$\n"
    "o10=0b101$\n"
    "o11=\"0b102\"$\n"
    "o12=1e5$\n"
    "o13=nan$\n"
    "o14=true$\n"
    "o15=-1.5$\n"
    "o16=\"1e\"$\n"
    "o17=.5$\n"
    "o18='B\"(\\\\xc3\\\\xa9)\"'$\n"
    "o19=\"line\\\\nbreak\"$\n"
    "o20=0x$\n"
    "--- m1 d1\n"
    "## d$\n"
    "s=[\"hello world\", \"a\\\\\"b\", \"tab\\\\there\", \"\", 'x', \"'\", "
    "0x1F, 'B\"(\\\\xc3\\\\xa9)\"', \"line\\\\nbreak\"]$\n"
    "o1=\"hello world\"$\n"
    "o2=\"a\\\\\"b\"$\n"
    "o3=\"tab\\\\there\"$\n"
    "o4=\"\"$\n"
    "o5='x'$\n"
    "o6=\"'\"$\n"
    "o7=0x1F$\n"
    "o8=\"0X1F\"$\n"
    "o9=0o17$\n"
    "o10=0b101$\n"
    "o11=\"0b102\"$\n"
    "o12=1e5$\n"
    "o13=nan$\n"
    "o14=true$\n"
    "o15=-1.5$\n"
    "o16=\"1e\"$\n"
    "o17=.5$\n"
    "o18='B\"(\\\\xc3\\\\xa9)\"'$\n"
    "o19=\"line\\\\nbreak\"$\n"
    "o20=0x$\n"
    "--- m2 d0\n"
    "s=[\"hello world\", \"a\\\\\"b\", \"tab\\\\there\", \"\", 'x', \"'\", "
    "0x1F, 'B\"(\\\\xc3\\\\xa9)\"', \"line\\\\nbreak\"]$\n"
    "o1=\"hello world\"$\n"
    "o2=\"a\\\\\"b\"$\n"
    "o3=\"tab\\\\there\"$\n"
    "o4=\"\"$\n"
    "o5='x'$\n"
    "o6=\"'\"$\n"
    "o7=0x1F$\n"
    "o8=\"0X1F\"$\n"
    "o9=0o17$\n"
    "o10=0b101$\n"
    "o11=\"0b102\"$\n"
    "o12=1e5$\n"
    "o13=nan$\n"
    "o14=true$\n"
    "o15=-1.5$\n"
    "o16=\"1e\"$\n"
    "o17=.5$\n"
    "o18='B\"(\\\\xc3\\\\xa9)\"'$\n"
    "o19=\"line\\\\nbreak\"$\n"
    "o20=0x$\n"
    "--- m2 d1\n"
    "## d$\n"
    "s=[\"hello world\", \"a\\\\\"b\", \"tab\\\\there\", \"\", 'x', \"'\", "
    "0x1F, 'B\"(\\\\xc3\\\\xa9)\"', \"line\\\\nbreak\"]$\n"
    "o1=\"hello world\"$\n"
    "o2=\"a\\\\\"b\"$\n"
    "o3=\"tab\\\\there\"$\n"
    "o4=\"\"$\n"
    "o5='x'$\n"
    "o6=\"'\"$\n"
    "o7=0x1F$\n"
    "o8=\"0X1F\"$\n"
    "o9=0o17$\n"
    "o10=0b101$\n"
    "o11=\"0b102\"$\n"
    "o12=1e5$\n"
    "o13=nan$\n"
    "o14=true$\n"
    "o15=-1.5$\n"
    "o16=\"1e\"$\n"
    "o17=.5$\n"
    "o18='B\"(\\\\xc3\\\\xa9)\"'$\n"
    "o19=\"line\\\\nbreak\"$\n"
    "o20=0x$\n"
    "W05 [--a.b 1 --x#y 2 --it's 3 --qu\"ote 4 --[br] 5 --back\\\\slash 6 "
    "--semi;colon 7 --eq 8]\n"
    "--- m0 d0\n"
    "'a.b'=1$\n"
    "'x#y'=2$\n"
    "\"it's\"=3$\n"
    "'qu\"ote'=4$\n"
    "'[br]'=5$\n"
    "'back\\\\slash'=6$\n"
    "'semi;colon'=7$\n"
    "eq=8$\n"
    "--- m0 d1\n"
    "## d$\n"
    "'a.b'=1$\n"
    "'x#y'=2$\n"
    "\"it's\"=3$\n"
    "'qu\"ote'=4$\n"
    "'[br]'=5$\n"
    "'back\\\\slash'=6$\n"
    "'semi;colon'=7$\n"
    "eq=8$\n"
    "--- m1 d0\n"
    "'a.b'=1$\n"
    "'x#y'=2$\n"
    "\"it's\"=3$\n"
    "'qu\"ote'=4$\n"
    "'[br]'=5$\n"
    "'back\\\\slash'=6$\n"
    "'semi;colon'=7$\n"
    "eq=8$\n"
    "--- m1 d1\n"
    "## d$\n"
    "'a.b'=1$\n"
    "'x#y'=2$\n"
    "\"it's\"=3$\n"
    "'qu\"ote'=4$\n"
    "'[br]'=5$\n"
    "'back\\\\slash'=6$\n"
    "'semi;colon'=7$\n"
    "eq=8$\n"
    "--- m2 d0\n"
    "'a.b'=1$\n"
    "'x#y'=2$\n"
    "\"it's\"=3$\n"
    "'qu\"ote'=4$\n"
    "'[br]'=5$\n"
    "'back\\\\slash'=6$\n"
    "'semi;colon'=7$\n"
    "eq=8$\n"
    "--- m2 d1\n"
    "## d$\n"
    "'a.b'=1$\n"
    "'x#y'=2$\n"
    "\"it's\"=3$\n"
    "'qu\"ote'=4$\n"
    "'[br]'=5$\n"
    "'back\\\\slash'=6$\n"
    "'semi;colon'=7$\n"
    "eq=8$\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// WRITE_LONG
//   constant: what the cases of d_tests_arg_config_write_long come to, as the
// reference writes them with CLI11.
static const char WRITE_LONG[] =
    "W04 [--l1 "
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\naaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa --l2 "
    "aaaaaaaaaa'''aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\naaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa --l3 "
    "\\nbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\\n]\n"
    "--- m0 d0\n"
    "l1='''aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa$\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'''$\n"
    "l2='B\"(aaaaaaaaaa\\\\x27\\\\x27\\\\x27aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaa\\\\x0aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaa)\"'$\n"
    "l3='''$\n"
    "$\n"
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb$\n"
    "$\n"
    "'''$\n"
    "--- m0 d1\n"
    "## d$\n"
    "l1='''aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa$\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'''$\n"
    "l2='B\"(aaaaaaaaaa\\\\x27\\\\x27\\\\x27aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaa\\\\x0aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaa)\"'$\n"
    "l3='''$\n"
    "$\n"
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb$\n"
    "$\n"
    "'''$\n"
    "--- m1 d0\n"
    "l1='''aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa$\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'''$\n"
    "l2='B\"(aaaaaaaaaa\\\\x27\\\\x27\\\\x27aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaa\\\\x0aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaa)\"'$\n"
    "l3='''$\n"
    "$\n"
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb$\n"
    "$\n"
    "'''$\n"
    "--- m1 d1\n"
    "## d$\n"
    "l1='''aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa$\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'''$\n"
    "l2='B\"(aaaaaaaaaa\\\\x27\\\\x27\\\\x27aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaa\\\\x0aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaa)\"'$\n"
    "l3='''$\n"
    "$\n"
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb$\n"
    "$\n"
    "'''$\n"
    "--- m2 d0\n"
    "l1='''aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa$\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'''$\n"
    "l2='B\"(aaaaaaaaaa\\\\x27\\\\x27\\\\x27aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaa\\\\x0aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaa)\"'$\n"
    "l3='''$\n"
    "$\n"
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb$\n"
    "$\n"
    "'''$\n"
    "--- m2 d1\n"
    "## d$\n"
    "l1='''aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa$\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'''$\n"
    "l2='B\"(aaaaaaaaaa\\\\x27\\\\x27\\\\x27aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaa\\\\x0aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaa)\"'$\n"
    "l3='''$\n"
    "$\n"
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb$\n"
    "$\n"
    "'''$\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// WRITE_LONGER
//   constant: what the cases of d_tests_arg_config_write_longer come to, as the
// reference writes them with CLI11.
static const char WRITE_LONGER[] =
    "W26 [--l4 "
    "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
    "cccccccccccccccccccccccccccccccccccccccccccccccccccccccc --l5 "
    "dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
    "dddddddddddddddddddddddddddddddddddd\"q\" --l6 "
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\naaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa x]\n"
    "--- m0 d0\n"
    "l4=\"ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
    "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc\"$\n"
    "l5='''dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
    "dddddddddddddddddddddddddddddddddddddddddd\"q\"'''$\n"
    "l6=[\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\\\naaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\", 'x']$\n"
    "--- m0 d1\n"
    "## d$\n"
    "l4=\"ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
    "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc\"$\n"
    "l5='''dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
    "dddddddddddddddddddddddddddddddddddddddddd\"q\"'''$\n"
    "l6=[\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\\\naaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\", 'x']$\n"
    "--- m1 d0\n"
    "l4=\"ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
    "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc\"$\n"
    "l5='''dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
    "dddddddddddddddddddddddddddddddddddddddddd\"q\"'''$\n"
    "l6=[\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\\\naaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\", 'x']$\n"
    "--- m1 d1\n"
    "## d$\n"
    "l4=\"ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
    "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc\"$\n"
    "l5='''dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
    "dddddddddddddddddddddddddddddddddddddddddd\"q\"'''$\n"
    "l6=[\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\\\naaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\", 'x']$\n"
    "--- m2 d0\n"
    "l4=\"ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
    "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc\"$\n"
    "l5='''dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
    "dddddddddddddddddddddddddddddddddddddddddd\"q\"'''$\n"
    "l6=[\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\\\naaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\", 'x']$\n"
    "--- m2 d1\n"
    "## d$\n"
    "l4=\"ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
    "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc\"$\n"
    "l5='''dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
    "dddddddddddddddddddddddddddddddddddddddddd\"q\"'''$\n"
    "l6=[\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\\\naaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\", 'x']$\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// WRITE_POLICIES
//   constant: what the cases of d_tests_arg_config_write_policies come to, as
// the reference writes them with CLI11.
static const char WRITE_POLICIES[] =
    "W09 [--rev 1 2 3 --sum 1 2 3 --join a --join b --join0 a --join0 b "
    "--joinbad a, --joinbad b --first f1 --first f2 --last l1 --last l2]\n"
    "--- m0 d0\n"
    "rev=3$\n"
    "sum=6$\n"
    "join=\"a,b\"$\n"
    "join0=['a', 'b']$\n"
    "joinbad=\"a,b\"$\n"
    "first=\"f1\"$\n"
    "last=\"l2\"$\n"
    "--- m0 d1\n"
    "## d$\n"
    "rev=3$\n"
    "sum=6$\n"
    "join=\"a,b\"$\n"
    "join0=['a', 'b']$\n"
    "joinbad=\"a,b\"$\n"
    "first=\"f1\"$\n"
    "last=\"l2\"$\n"
    "--- m1 d0\n"
    "rev=3$\n"
    "sum=6$\n"
    "join=\"a,b\"$\n"
    "join0=['a', 'b']$\n"
    "joinbad=\"a,b\"$\n"
    "first=\"f1\"$\n"
    "last=\"l2\"$\n"
    "--- m1 d1\n"
    "## d$\n"
    "rev=3$\n"
    "sum=6$\n"
    "join=\"a,b\"$\n"
    "join0=['a', 'b']$\n"
    "joinbad=\"a,b\"$\n"
    "first=\"f1\"$\n"
    "last=\"l2\"$\n"
    "--- m2 d0\n"
    "rev=3$\n"
    "sum=6$\n"
    "join=\"a,b\"$\n"
    "join0=['a', 'b']$\n"
    "joinbad=\"a,b\"$\n"
    "first=\"f1\"$\n"
    "last=\"l2\"$\n"
    "--- m2 d1\n"
    "## d$\n"
    "rev=3$\n"
    "sum=6$\n"
    "join=\"a,b\"$\n"
    "join0=['a', 'b']$\n"
    "joinbad=\"a,b\"$\n"
    "first=\"f1\"$\n"
    "last=\"l2\"$\n"
    "W23 [--j a,b --j c --k  --k b --m a --m ,,b --s x y]\n"
    "--- m0 d0\n"
    "j=\"a,b,c\"$\n"
    "k=\",b\"$\n"
    "m=\"a,b\"$\n"
    "s=\"xy\"$\n"
    "--- m0 d1\n"
    "## d$\n"
    "j=\"a,b,c\"$\n"
    "k=\",b\"$\n"
    "m=\"a,b\"$\n"
    "s=\"xy\"$\n"
    "--- m1 d0\n"
    "j=\"a,b,c\"$\n"
    "k=\",b\"$\n"
    "m=\"a,b\"$\n"
    "s=\"xy\"$\n"
    "--- m1 d1\n"
    "## d$\n"
    "j=\"a,b,c\"$\n"
    "k=\",b\"$\n"
    "m=\"a,b\"$\n"
    "s=\"xy\"$\n"
    "--- m2 d0\n"
    "j=\"a,b,c\"$\n"
    "k=\",b\"$\n"
    "m=\"a,b\"$\n"
    "s=\"xy\"$\n"
    "--- m2 d1\n"
    "## d$\n"
    "j=\"a,b,c\"$\n"
    "k=\",b\"$\n"
    "m=\"a,b\"$\n"
    "s=\"xy\"$\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// WRITE_COMMANDS
//   constant: what the cases of d_tests_arg_config_write_commands come to, as
// the reference writes them with CLI11.
static const char WRITE_COMMANDS[] =
    "W06 [--a 0 sub --x 1 deep --y 2]\n"
    "--- m0 d0\n"
    "a=0$\n"
    "sub.x=1$\n"
    "sub.deep.y=2$\n"
    "--- m0 d1\n"
    "## d$\n"
    "a=0$\n"
    "## the sub$\n"
    "sub.x=1$\n"
    "## $\n"
    "sub.deep.y=2$\n"
    "--- m1 d0\n"
    "a=0$\n"
    "sub.x=1$\n"
    "sub.deep.y=2$\n"
    "idle.z=\"zz\"$\n"
    "--- m1 d1\n"
    "## d$\n"
    "a=0$\n"
    "## the sub$\n"
    "sub.x=1$\n"
    "## $\n"
    "sub.deep.y=2$\n"
    "## $\n"
    "idle.z=\"zz\"$\n"
    "--- m2 d0\n"
    "a=0$\n"
    "sub.x=1$\n"
    "sub.deep.y=2$\n"
    "--- m2 d1\n"
    "## d$\n"
    "a=0$\n"
    "## the sub$\n"
    "sub.x=1$\n"
    "## $\n"
    "sub.deep.y=2$\n"
    "W07 [--a 0 plain --p 3 sub --x 1 deep --y 2]\n"
    "--- m0 d0\n"
    "a=0$\n"
    "[sub]$\n"
    "x=1$\n"
    "[sub.deep]$\n"
    "y=2$\n"
    "plain.p=3$\n"
    "--- m0 d1\n"
    "## d$\n"
    "a=0$\n"
    "[sub]$\n"
    "## the sub$\n"
    "x=1$\n"
    "[sub.deep]$\n"
    "## the deep$\n"
    "y=2$\n"
    "## $\n"
    "plain.p=3$\n"
    "--- m1 d0\n"
    "a=0$\n"
    "[sub]$\n"
    "x=1$\n"
    "[sub.deep]$\n"
    "y=2$\n"
    "plain.p=3$\n"
    "--- m1 d1\n"
    "## d$\n"
    "a=0$\n"
    "[sub]$\n"
    "## the sub$\n"
    "x=1$\n"
    "[sub.deep]$\n"
    "## the deep$\n"
    "y=2$\n"
    "## $\n"
    "plain.p=3$\n"
    "--- m2 d0\n"
    "a=0$\n"
    "[sub]$\n"
    "x=1$\n"
    "[sub.deep]$\n"
    "y=2$\n"
    "plain.p=3$\n"
    "--- m2 d1\n"
    "## d$\n"
    "a=0$\n"
    "[sub]$\n"
    "## the sub$\n"
    "x=1$\n"
    "[sub.deep]$\n"
    "## the deep$\n"
    "y=2$\n"
    "## $\n"
    "plain.p=3$\n"
    "W18 [a]\n"
    "--- m0 d0\n"
    "[a]$\n"
    "--- m0 d1\n"
    "## d$\n"
    "[a]$\n"
    "--- m1 d0\n"
    "[a]$\n"
    "x=1$\n"
    "[b]$\n"
    "y=2$\n"
    "--- m1 d1\n"
    "## d$\n"
    "[a]$\n"
    "## $\n"
    "x=1$\n"
    "[b]$\n"
    "## $\n"
    "y=2$\n"
    "--- m2 d0\n"
    "[a]$\n"
    "x=1$\n"
    "--- m2 d1\n"
    "## d$\n"
    "[a]$\n"
    "## $\n"
    "x=1$\n"
    "W21 [insub --x 1]\n"
    "--- m0 d0\n"
    "[.insub]$\n"
    "x=1$\n"
    "--- m0 d1\n"
    "## d$\n"
    "$\n"
    "## grp Options$\n"
    "## $\n"
    "[.insub]$\n"
    "## $\n"
    "x=1$\n"
    "--- m1 d0\n"
    "[.insub]$\n"
    "x=1$\n"
    "--- m1 d1\n"
    "## d$\n"
    "$\n"
    "## grp Options$\n"
    "## $\n"
    "[.insub]$\n"
    "## $\n"
    "x=1$\n"
    "--- m2 d0\n"
    "[.insub]$\n"
    "x=1$\n"
    "--- m2 d1\n"
    "## d$\n"
    "$\n"
    "## grp Options$\n"
    "## $\n"
    "[.insub]$\n"
    "## $\n"
    "x=1$\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// WRITE_GROUPS
//   constant: what the cases of d_tests_arg_config_write_groups come to, as the
// reference writes them with CLI11.
static const char WRITE_GROUPS[] =
    "W08 [--gx 1 --hx 2 --top 3 --hid 4 --top3 5]\n"
    "--- m0 d0\n"
    "hid=4$\n"
    "top3=5$\n"
    "top=3$\n"
    "gx=1$\n"
    "hx=2$\n"
    "--- m0 d1\n"
    "## d$\n"
    "hid=4$\n"
    "top3=5$\n"
    "$\n"
    "## Other Options$\n"
    "top=3$\n"
    "$\n"
    "## grp Options$\n"
    "## grouped$\n"
    "# gx option$\n"
    "gx=1$\n"
    "## $\n"
    "hx=2$\n"
    "--- m1 d0\n"
    "hid=4$\n"
    "top3=5$\n"
    "top=3$\n"
    "top2=\"\"$\n"
    "gx=1$\n"
    "hx=2$\n"
    "--- m1 d1\n"
    "## d$\n"
    "hid=4$\n"
    "top3=5$\n"
    "$\n"
    "## Other Options$\n"
    "top=3$\n"
    "top2=\"\"$\n"
    "$\n"
    "## grp Options$\n"
    "## grouped$\n"
    "# gx option$\n"
    "gx=1$\n"
    "## $\n"
    "hx=2$\n"
    "--- m2 d0\n"
    "hid=4$\n"
    "top3=5$\n"
    "top=3$\n"
    "top2=\"\"$\n"
    "gx=1$\n"
    "hx=2$\n"
    "--- m2 d1\n"
    "## d$\n"
    "hid=4$\n"
    "top3=5$\n"
    "$\n"
    "## Other Options$\n"
    "top=3$\n"
    "top2=\"\"$\n"
    "$\n"
    "## grp Options$\n"
    "## grouped$\n"
    "# gx option$\n"
    "gx=1$\n"
    "## $\n"
    "hx=2$\n"
    "W13 [--a 1 --b 2]\n"
    "--- m0 d0\n"
    "a=1$\n"
    "b=2$\n"
    "--- m0 d1\n"
    "## line1$\n"
    "## line2\\x0d## $\n"
    "## line3$\n"
    "# a$\n"
    "# b$\n"
    "a=1$\n"
    "$\n"
    "## grp Options$\n"
    "## g$\n"
    "## desc$\n"
    "# bdesc$\n"
    "b=2$\n"
    "--- m1 d0\n"
    "a=1$\n"
    "b=2$\n"
    "--- m1 d1\n"
    "## line1$\n"
    "## line2\\x0d## $\n"
    "## line3$\n"
    "# a$\n"
    "# b$\n"
    "a=1$\n"
    "$\n"
    "## grp Options$\n"
    "## g$\n"
    "## desc$\n"
    "# bdesc$\n"
    "b=2$\n"
    "--- m2 d0\n"
    "a=1$\n"
    "b=2$\n"
    "--- m2 d1\n"
    "## line1$\n"
    "## line2\\x0d## $\n"
    "## line3$\n"
    "# a$\n"
    "# b$\n"
    "a=1$\n"
    "$\n"
    "## grp Options$\n"
    "## g$\n"
    "## desc$\n"
    "# bdesc$\n"
    "b=2$\n"
    "W22 [--a 1 --b 2]\n"
    "--- m0 d0\n"
    "a=1$\n"
    "b=2$\n"
    "--- m0 d1\n"
    "## d$\n"
    "a=1$\n"
    "$\n"
    "## SUBCOMMANDS Options$\n"
    "## nameless$\n"
    "b=2$\n"
    "--- m1 d0\n"
    "a=1$\n"
    "b=2$\n"
    "--- m1 d1\n"
    "## d$\n"
    "a=1$\n"
    "$\n"
    "## SUBCOMMANDS Options$\n"
    "## nameless$\n"
    "b=2$\n"
    "--- m2 d0\n"
    "a=1$\n"
    "b=2$\n"
    "--- m2 d1\n"
    "## d$\n"
    "a=1$\n"
    "$\n"
    "## SUBCOMMANDS Options$\n"
    "## nameless$\n"
    "b=2$\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// WRITE_FORMATS
//   constant: what the cases of d_tests_arg_config_write_formats come to, as
// the reference writes them with CLI11.
static const char WRITE_FORMATS[] =
    "W12 [--v 1 2 3 --s a b sub --x y]\n"
    "--- m0 d0\n"
    "v=1 2 3$\n"
    "s=\"a b\"$\n"
    "[sub]$\n"
    "x='y'$\n"
    "--- m0 d1\n"
    ";; d$\n"
    "v=1 2 3$\n"
    "s=\"a b\"$\n"
    "[sub]$\n"
    ";; $\n"
    "x='y'$\n"
    "--- m1 d0\n"
    "v=1 2 3$\n"
    "s=\"a b\"$\n"
    "[sub]$\n"
    "x='y'$\n"
    "--- m1 d1\n"
    ";; d$\n"
    "v=1 2 3$\n"
    "s=\"a b\"$\n"
    "[sub]$\n"
    ";; $\n"
    "x='y'$\n"
    "--- m2 d0\n"
    "v=1 2 3$\n"
    "s=\"a b\"$\n"
    "[sub]$\n"
    "x='y'$\n"
    "--- m2 d1\n"
    ";; d$\n"
    "v=1 2 3$\n"
    "s=\"a b\"$\n"
    "[sub]$\n"
    ";; $\n"
    "x='y'$\n"
    "W25 [--v 1 a b --s it's --t x!y --n!m 4 --p/q<r> 7 sub --x 5 deep --y 6]\n"
    "--- m0 d0\n"
    "v:<1; 'a b'>$\n"
    "s:'it's'$\n"
    "t:'x!y'$\n"
    "'n!m':4$\n"
    "'p/q<r>':7$\n"
    "sub/x:5$\n"
    "[sub/deep]$\n"
    "y:6$\n"
    "--- m0 d1\n"
    "!! d$\n"
    "! values$\n"
    "v:<1; 'a b'>$\n"
    "s:'it's'$\n"
    "t:'x!y'$\n"
    "'n!m':4$\n"
    "'p/q<r>':7$\n"
    "!! a sub$\n"
    "sub/x:5$\n"
    "[sub/deep]$\n"
    "!! $\n"
    "y:6$\n"
    "--- m1 d0\n"
    "v:<1; 'a b'>$\n"
    "s:'it's'$\n"
    "t:'x!y'$\n"
    "'n!m':4$\n"
    "'p/q<r>':7$\n"
    "sub/x:5$\n"
    "[sub/deep]$\n"
    "y:6$\n"
    "--- m1 d1\n"
    "!! d$\n"
    "! values$\n"
    "v:<1; 'a b'>$\n"
    "s:'it's'$\n"
    "t:'x!y'$\n"
    "'n!m':4$\n"
    "'p/q<r>':7$\n"
    "!! a sub$\n"
    "sub/x:5$\n"
    "[sub/deep]$\n"
    "!! $\n"
    "y:6$\n"
    "--- m2 d0\n"
    "v:<1; 'a b'>$\n"
    "s:'it's'$\n"
    "t:'x!y'$\n"
    "'n!m':4$\n"
    "'p/q<r>':7$\n"
    "sub/x:5$\n"
    "[sub/deep]$\n"
    "y:6$\n"
    "--- m2 d1\n"
    "!! d$\n"
    "! values$\n"
    "v:<1; 'a b'>$\n"
    "s:'it's'$\n"
    "t:'x!y'$\n"
    "'n!m':4$\n"
    "'p/q<r>':7$\n"
    "!! a sub$\n"
    "sub/x:5$\n"
    "[sub/deep]$\n"
    "!! $\n"
    "y:6$\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// WRITE_PREFIX
//   constant: what the cases of d_tests_arg_config_write_prefix come to, as the
// reference writes them with CLI11.
static const char WRITE_PREFIX[] =
    "W24 [--a 1 sub --x 2 plain --p 3]\n"
    "--- m0 d0\n"
    "pre.a=1$\n"
    "[pre.sub]$\n"
    "x=2$\n"
    "pre.plain.p=3$\n"
    "--- m0 d1\n"
    "## d$\n"
    "pre.a=1$\n"
    "[pre.sub]$\n"
    "## $\n"
    "x=2$\n"
    "## $\n"
    "pre.plain.p=3$\n"
    "--- m1 d0\n"
    "pre.a=1$\n"
    "[pre.sub]$\n"
    "x=2$\n"
    "pre.plain.p=3$\n"
    "--- m1 d1\n"
    "## d$\n"
    "pre.a=1$\n"
    "[pre.sub]$\n"
    "## $\n"
    "x=2$\n"
    "## $\n"
    "pre.plain.p=3$\n"
    "--- m2 d0\n"
    "pre.a=1$\n"
    "[pre.sub]$\n"
    "x=2$\n"
    "pre.plain.p=3$\n"
    "--- m2 d1\n"
    "## d$\n"
    "pre.a=1$\n"
    "[pre.sub]$\n"
    "## $\n"
    "x=2$\n"
    "## $\n"
    "pre.plain.p=3$\n"
    "W27 [--a 1 --p 2 --g 3 --n 4]\n"
    "--- m0 d0\n"
    "pre.a=1$\n"
    "pre.p=2$\n"
    "pre.g=3$\n"
    "pre.n=4$\n"
    "--- m0 d1\n"
    "## d$\n"
    "pre.a=1$\n"
    "## $\n"
    "pre.p=2$\n"
    "$\n"
    "## grp Options$\n"
    "## $\n"
    "pre.g=3$\n"
    "$\n"
    "## SUBCOMMANDS Options$\n"
    "## nl$\n"
    "pre.n=4$\n"
    "--- m1 d0\n"
    "pre.a=1$\n"
    "pre.p=2$\n"
    "pre.g=3$\n"
    "pre.n=4$\n"
    "--- m1 d1\n"
    "## d$\n"
    "pre.a=1$\n"
    "## $\n"
    "pre.p=2$\n"
    "$\n"
    "## grp Options$\n"
    "## $\n"
    "pre.g=3$\n"
    "$\n"
    "## SUBCOMMANDS Options$\n"
    "## nl$\n"
    "pre.n=4$\n"
    "--- m2 d0\n"
    "pre.a=1$\n"
    "pre.p=2$\n"
    "pre.g=3$\n"
    "pre.n=4$\n"
    "--- m2 d1\n"
    "## d$\n"
    "pre.a=1$\n"
    "## $\n"
    "pre.p=2$\n"
    "$\n"
    "## grp Options$\n"
    "## $\n"
    "pre.g=3$\n"
    "$\n"
    "## SUBCOMMANDS Options$\n"
    "## nl$\n"
    "pre.n=4$\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// WRITE_VALUES
//   constant: what the cases of d_tests_arg_config_write_values come to, as the
// reference writes them with CLI11.
static const char WRITE_VALUES[] =
    "W28 [--v1 ~ --v2 a~b --v3 -5 --v4 +5 --v5 1e+5 --v6 1e-5 --v7 . --v8 + "
    "--v9 e5 --v10 0x10 --v11 0x0 --v12 0o10 --v13 0o9 --v14 0xg --v15 0b2 "
    "--v16 \\x01x --v17 \" --v18 \\\\]\n"
    "--- m0 d0\n"
    "v1='~'$\n"
    "v2=\"a~b\"$\n"
    "v3=-5$\n"
    "v4=+5$\n"
    "v5=1e+5$\n"
    "v6=1e-5$\n"
    "v7='.'$\n"
    "v8='+'$\n"
    "v9=\"e5\"$\n"
    "v10=0x10$\n"
    "v11=0x0$\n"
    "v12=0o10$\n"
    "v13=\"0o9\"$\n"
    "v14=\"0xg\"$\n"
    "v15=\"0b2\"$\n"
    "v16='B\"(\\\\x01x)\"'$\n"
    "v17='\"'$\n"
    "v18='\\\\'$\n"
    "--- m0 d1\n"
    "## d$\n"
    "v1='~'$\n"
    "v2=\"a~b\"$\n"
    "v3=-5$\n"
    "v4=+5$\n"
    "v5=1e+5$\n"
    "v6=1e-5$\n"
    "v7='.'$\n"
    "v8='+'$\n"
    "v9=\"e5\"$\n"
    "v10=0x10$\n"
    "v11=0x0$\n"
    "v12=0o10$\n"
    "v13=\"0o9\"$\n"
    "v14=\"0xg\"$\n"
    "v15=\"0b2\"$\n"
    "v16='B\"(\\\\x01x)\"'$\n"
    "v17='\"'$\n"
    "v18='\\\\'$\n"
    "--- m1 d0\n"
    "v1='~'$\n"
    "v2=\"a~b\"$\n"
    "v3=-5$\n"
    "v4=+5$\n"
    "v5=1e+5$\n"
    "v6=1e-5$\n"
    "v7='.'$\n"
    "v8='+'$\n"
    "v9=\"e5\"$\n"
    "v10=0x10$\n"
    "v11=0x0$\n"
    "v12=0o10$\n"
    "v13=\"0o9\"$\n"
    "v14=\"0xg\"$\n"
    "v15=\"0b2\"$\n"
    "v16='B\"(\\\\x01x)\"'$\n"
    "v17='\"'$\n"
    "v18='\\\\'$\n"
    "--- m1 d1\n"
    "## d$\n"
    "v1='~'$\n"
    "v2=\"a~b\"$\n"
    "v3=-5$\n"
    "v4=+5$\n"
    "v5=1e+5$\n"
    "v6=1e-5$\n"
    "v7='.'$\n"
    "v8='+'$\n"
    "v9=\"e5\"$\n"
    "v10=0x10$\n"
    "v11=0x0$\n"
    "v12=0o10$\n"
    "v13=\"0o9\"$\n"
    "v14=\"0xg\"$\n"
    "v15=\"0b2\"$\n"
    "v16='B\"(\\\\x01x)\"'$\n"
    "v17='\"'$\n"
    "v18='\\\\'$\n"
    "--- m2 d0\n"
    "v1='~'$\n"
    "v2=\"a~b\"$\n"
    "v3=-5$\n"
    "v4=+5$\n"
    "v5=1e+5$\n"
    "v6=1e-5$\n"
    "v7='.'$\n"
    "v8='+'$\n"
    "v9=\"e5\"$\n"
    "v10=0x10$\n"
    "v11=0x0$\n"
    "v12=0o10$\n"
    "v13=\"0o9\"$\n"
    "v14=\"0xg\"$\n"
    "v15=\"0b2\"$\n"
    "v16='B\"(\\\\x01x)\"'$\n"
    "v17='\"'$\n"
    "v18='\\\\'$\n"
    "--- m2 d1\n"
    "## d$\n"
    "v1='~'$\n"
    "v2=\"a~b\"$\n"
    "v3=-5$\n"
    "v4=+5$\n"
    "v5=1e+5$\n"
    "v6=1e-5$\n"
    "v7='.'$\n"
    "v8='+'$\n"
    "v9=\"e5\"$\n"
    "v10=0x10$\n"
    "v11=0x0$\n"
    "v12=0o10$\n"
    "v13=\"0o9\"$\n"
    "v14=\"0xg\"$\n"
    "v15=\"0b2\"$\n"
    "v16='B\"(\\\\x01x)\"'$\n"
    "v17='\"'$\n"
    "v18='\\\\'$\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// WRITE_QUOTING
//   constant: what the cases of d_tests_arg_config_write_quoting come to, as
// the reference writes them with CLI11.
static const char WRITE_QUOTING[] =
    "W29 [--e100 "
    "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
    "eeeeeeeeeeeeeeeeeeeeeeeee\\n --q a 'x' b "
    "''y\\nqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq"
    "qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq]\n"
    "--- m0 d0\n"
    "e100=\"eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
    "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee\\\\n\"$\n"
    "q='''a 'x' b ''y$\n"
    "qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq"
    "qqqqqqqqqqqqqqqqqqqqqqqqqq'''$\n"
    "--- m0 d1\n"
    "## d$\n"
    "e100=\"eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
    "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee\\\\n\"$\n"
    "q='''a 'x' b ''y$\n"
    "qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq"
    "qqqqqqqqqqqqqqqqqqqqqqqqqq'''$\n"
    "--- m1 d0\n"
    "e100=\"eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
    "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee\\\\n\"$\n"
    "q='''a 'x' b ''y$\n"
    "qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq"
    "qqqqqqqqqqqqqqqqqqqqqqqqqq'''$\n"
    "--- m1 d1\n"
    "## d$\n"
    "e100=\"eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
    "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee\\\\n\"$\n"
    "q='''a 'x' b ''y$\n"
    "qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq"
    "qqqqqqqqqqqqqqqqqqqqqqqqqq'''$\n"
    "--- m2 d0\n"
    "e100=\"eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
    "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee\\\\n\"$\n"
    "q='''a 'x' b ''y$\n"
    "qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq"
    "qqqqqqqqqqqqqqqqqqqqqqqqqq'''$\n"
    "--- m2 d1\n"
    "## d$\n"
    "e100=\"eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
    "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee\\\\n\"$\n"
    "q='''a 'x' b ''y$\n"
    "qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq"
    "qqqqqqqqqqqqqqqqqqqqqqqqqq'''$\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// WRITE_TRIPLES
//   constant: what the cases of d_tests_arg_config_write_triples come to, as
// the reference writes them with CLI11.
static const char WRITE_TRIPLES[] =
    "W44 [--st "
    "'''sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss"
    "sssssssssssssssssssssssssssss\\n --en "
    "nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn"
    "nnnnnnnnnnnnnnnnnnnnnnnnnn\\n''' --tq "
    "'''\"ttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttt"
    "ttttttttttttttttttttttttttttttt]\n"
    "--- m0 d0\n"
    "st='B\"(\\\\x27\\\\x27\\\\x27sssssssssssssssssssssssssssssssssssssssssssss"
    "sssssssssssssssssssssssssssssssssssssssssssssssssssssss\\\\x0a)\"'$\n"
    "en='B\"(nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn"
    "nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn\\\\x0a\\\\x27\\\\x27\\\\x27)\"'$\n"
    "tq='B\"(\\\\x27\\\\x27\\\\x27\"ttttttttttttttttttttttttttttttttttttttttttt"
    "ttttttttttttttttttttttttttttttttttttttttttttttttttttttttt)\"'$\n"
    "--- m0 d1\n"
    "## d$\n"
    "st='B\"(\\\\x27\\\\x27\\\\x27sssssssssssssssssssssssssssssssssssssssssssss"
    "sssssssssssssssssssssssssssssssssssssssssssssssssssssss\\\\x0a)\"'$\n"
    "en='B\"(nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn"
    "nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn\\\\x0a\\\\x27\\\\x27\\\\x27)\"'$\n"
    "tq='B\"(\\\\x27\\\\x27\\\\x27\"ttttttttttttttttttttttttttttttttttttttttttt"
    "ttttttttttttttttttttttttttttttttttttttttttttttttttttttttt)\"'$\n"
    "--- m1 d0\n"
    "st='B\"(\\\\x27\\\\x27\\\\x27sssssssssssssssssssssssssssssssssssssssssssss"
    "sssssssssssssssssssssssssssssssssssssssssssssssssssssss\\\\x0a)\"'$\n"
    "en='B\"(nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn"
    "nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn\\\\x0a\\\\x27\\\\x27\\\\x27)\"'$\n"
    "tq='B\"(\\\\x27\\\\x27\\\\x27\"ttttttttttttttttttttttttttttttttttttttttttt"
    "ttttttttttttttttttttttttttttttttttttttttttttttttttttttttt)\"'$\n"
    "--- m1 d1\n"
    "## d$\n"
    "st='B\"(\\\\x27\\\\x27\\\\x27sssssssssssssssssssssssssssssssssssssssssssss"
    "sssssssssssssssssssssssssssssssssssssssssssssssssssssss\\\\x0a)\"'$\n"
    "en='B\"(nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn"
    "nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn\\\\x0a\\\\x27\\\\x27\\\\x27)\"'$\n"
    "tq='B\"(\\\\x27\\\\x27\\\\x27\"ttttttttttttttttttttttttttttttttttttttttttt"
    "ttttttttttttttttttttttttttttttttttttttttttttttttttttttttt)\"'$\n"
    "--- m2 d0\n"
    "st='B\"(\\\\x27\\\\x27\\\\x27sssssssssssssssssssssssssssssssssssssssssssss"
    "sssssssssssssssssssssssssssssssssssssssssssssssssssssss\\\\x0a)\"'$\n"
    "en='B\"(nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn"
    "nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn\\\\x0a\\\\x27\\\\x27\\\\x27)\"'$\n"
    "tq='B\"(\\\\x27\\\\x27\\\\x27\"ttttttttttttttttttttttttttttttttttttttttttt"
    "ttttttttttttttttttttttttttttttttttttttttttttttttttttttttt)\"'$\n"
    "--- m2 d1\n"
    "## d$\n"
    "st='B\"(\\\\x27\\\\x27\\\\x27sssssssssssssssssssssssssssssssssssssssssssss"
    "sssssssssssssssssssssssssssssssssssssssssssssssssssssss\\\\x0a)\"'$\n"
    "en='B\"(nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn"
    "nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn\\\\x0a\\\\x27\\\\x27\\\\x27)\"'$\n"
    "tq='B\"(\\\\x27\\\\x27\\\\x27\"ttttttttttttttttttttttttttttttttttttttttttt"
    "ttttttttttttttttttttttttttttttttttttttttttttttttttttttttt)\"'$\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// WRITE_NAMES
//   constant: what the cases of d_tests_arg_config_write_names come to, as the
// reference writes them with CLI11.
static const char WRITE_NAMES[] =
    "W30 [--.a 1 --`b 2 --\\\\c 3]\n"
    "--- m0 d0\n"
    "'.a'=1$\n"
    "'`b'=2$\n"
    "'\\\\c'=3$\n"
    "--- m0 d1\n"
    "## d$\n"
    "'.a'=1$\n"
    "'`b'=2$\n"
    "'\\\\c'=3$\n"
    "--- m1 d0\n"
    "'.a'=1$\n"
    "'`b'=2$\n"
    "'\\\\c'=3$\n"
    "--- m1 d1\n"
    "## d$\n"
    "'.a'=1$\n"
    "'`b'=2$\n"
    "'\\\\c'=3$\n"
    "--- m2 d0\n"
    "'.a'=1$\n"
    "'`b'=2$\n"
    "'\\\\c'=3$\n"
    "--- m2 d1\n"
    "## d$\n"
    "'.a'=1$\n"
    "'`b'=2$\n"
    "'\\\\c'=3$\n"
    "W31 [--[a] 1 --a] 2 --[b 3]\n"
    "--- m0 d0\n"
    "'[a]'=1$\n"
    "a]=2$\n"
    "[b=3$\n"
    "--- m0 d1\n"
    "## d$\n"
    "'[a]'=1$\n"
    "a]=2$\n"
    "[b=3$\n"
    "--- m1 d0\n"
    "'[a]'=1$\n"
    "a]=2$\n"
    "[b=3$\n"
    "--- m1 d1\n"
    "## d$\n"
    "'[a]'=1$\n"
    "a]=2$\n"
    "[b=3$\n"
    "--- m2 d0\n"
    "'[a]'=1$\n"
    "a]=2$\n"
    "[b=3$\n"
    "--- m2 d1\n"
    "## d$\n"
    "'[a]'=1$\n"
    "a]=2$\n"
    "[b=3$\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// WRITE_FLAGS
//   constant: what the cases of d_tests_arg_config_write_flags come to, as the
// reference writes them with CLI11.
static const char WRITE_FLAGS[] =
    "W32 [--no-f --f1 --g2 --f3 --g3 --f4 --g4 --f5 --o a --o b]\n"
    "--- m0 d0\n"
    "no-f=true$\n"
    "f1=1$\n"
    "g2=2$\n"
    "x3=[1, 2]$\n"
    "x4=[1, 2]$\n"
    "f5=true$\n"
    "o='b'$\n"
    "--- m0 d1\n"
    "## d$\n"
    "no-f=true$\n"
    "f1=1$\n"
    "g2=2$\n"
    "x3=[1, 2]$\n"
    "x4=[1, 2]$\n"
    "f5=true$\n"
    "o='b'$\n"
    "--- m1 d0\n"
    "no-f=true$\n"
    "f1=1$\n"
    "g2=2$\n"
    "x3=[1, 2]$\n"
    "x4=[1, 2]$\n"
    "f5=true$\n"
    "o='b'$\n"
    "--- m1 d1\n"
    "## d$\n"
    "no-f=true$\n"
    "f1=1$\n"
    "g2=2$\n"
    "x3=[1, 2]$\n"
    "x4=[1, 2]$\n"
    "f5=true$\n"
    "o='b'$\n"
    "--- m2 d0\n"
    "no-f=true$\n"
    "f1=1$\n"
    "g2=2$\n"
    "x3=[1, 2]$\n"
    "x4=[1, 2]$\n"
    "f5=true$\n"
    "o='b'$\n"
    "--- m2 d1\n"
    "## d$\n"
    "no-f=true$\n"
    "f1=1$\n"
    "g2=2$\n"
    "x3=[1, 2]$\n"
    "x4=[1, 2]$\n"
    "f5=true$\n"
    "o='b'$\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// WRITE_ORDERS
//   constant: what the cases of d_tests_arg_config_write_orders come to, as the
// reference writes them with CLI11.
static const char WRITE_ORDERS[] =
    "W33 [--r1 1 2 3 --r2 1 2]\n"
    "--- m0 d0\n"
    "r1=[1, 2, 3]$\n"
    "r2=[1, 2]$\n"
    "--- m0 d1\n"
    "## d$\n"
    "r1=[1, 2, 3]$\n"
    "r2=[1, 2]$\n"
    "--- m1 d0\n"
    "r1=[1, 2, 3]$\n"
    "r2=[1, 2]$\n"
    "--- m1 d1\n"
    "## d$\n"
    "r1=[1, 2, 3]$\n"
    "r2=[1, 2]$\n"
    "--- m2 d0\n"
    "r1=[1, 2, 3]$\n"
    "r2=[1, 2]$\n"
    "--- m2 d1\n"
    "## d$\n"
    "r1=[1, 2, 3]$\n"
    "r2=[1, 2]$\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// WRITE_SUMS
//   constant: what the cases of d_tests_arg_config_write_sums come to, as the
// reference writes them with CLI11.
static const char WRITE_SUMS[] =
    "W34 [--s1 3 4 --s2 3 4 --s3 3 4 --s4 3]\n"
    "--- m0 d0\n"
    "s1=[3, 4]$\n"
    "s2=[3, 4]$\n"
    "s3=28$\n"
    "s4=12$\n"
    "--- m0 d1\n"
    "## d$\n"
    "s1=[3, 4]$\n"
    "s2=[3, 4]$\n"
    "s3=28$\n"
    "s4=12$\n"
    "--- m1 d0\n"
    "s1=[3, 4]$\n"
    "s2=[3, 4]$\n"
    "s3=28$\n"
    "s4=12$\n"
    "--- m1 d1\n"
    "## d$\n"
    "s1=[3, 4]$\n"
    "s2=[3, 4]$\n"
    "s3=28$\n"
    "s4=12$\n"
    "--- m2 d0\n"
    "s1=[3, 4]$\n"
    "s2=[3, 4]$\n"
    "s3=28$\n"
    "s4=12$\n"
    "--- m2 d1\n"
    "## d$\n"
    "s1=[3, 4]$\n"
    "s2=[3, 4]$\n"
    "s3=28$\n"
    "s4=12$\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// WRITE_JOINS
//   constant: what the cases of d_tests_arg_config_write_joins come to, as the
// reference writes them with CLI11.
static const char WRITE_JOINS[] =
    "W35 [--j1 a --j1  --j2 a --j2  --j2 b --j3  --j3  --j3 a --j4  --j4  --j5 "
    "one --j6  --j6  --j6 ]\n"
    "--- m0 d0\n"
    "j1='a'$\n"
    "j2=['a', \"\", 'b']$\n"
    "j3=[\"\", \"\", 'a']$\n"
    "j4=\"\"$\n"
    "j5=1$\n"
    "j6=[\"\", \"\", \"\"]$\n"
    "--- m0 d1\n"
    "## d$\n"
    "j1='a'$\n"
    "j2=['a', \"\", 'b']$\n"
    "j3=[\"\", \"\", 'a']$\n"
    "j4=\"\"$\n"
    "j5=1$\n"
    "j6=[\"\", \"\", \"\"]$\n"
    "--- m1 d0\n"
    "j1='a'$\n"
    "j2=['a', \"\", 'b']$\n"
    "j3=[\"\", \"\", 'a']$\n"
    "j4=\"\"$\n"
    "j5=1$\n"
    "j6=[\"\", \"\", \"\"]$\n"
    "--- m1 d1\n"
    "## d$\n"
    "j1='a'$\n"
    "j2=['a', \"\", 'b']$\n"
    "j3=[\"\", \"\", 'a']$\n"
    "j4=\"\"$\n"
    "j5=1$\n"
    "j6=[\"\", \"\", \"\"]$\n"
    "--- m2 d0\n"
    "j1='a'$\n"
    "j2=['a', \"\", 'b']$\n"
    "j3=[\"\", \"\", 'a']$\n"
    "j4=\"\"$\n"
    "j5=1$\n"
    "j6=[\"\", \"\", \"\"]$\n"
    "--- m2 d1\n"
    "## d$\n"
    "j1='a'$\n"
    "j2=['a', \"\", 'b']$\n"
    "j3=[\"\", \"\", 'a']$\n"
    "j4=\"\"$\n"
    "j5=1$\n"
    "j6=[\"\", \"\", \"\"]$\n"
    "W36 [--j a,b,c --j d --j2 ,a,b --j2 c --j3 a --j3 b,]\n"
    "--- m0 d0\n"
    "j=[\"a,b,c\", 'd']$\n"
    "j2=[\",a,b\", 'c']$\n"
    "j3=['a', \"b,\"]$\n"
    "--- m0 d1\n"
    "## d$\n"
    "j=[\"a,b,c\", 'd']$\n"
    "j2=[\",a,b\", 'c']$\n"
    "j3=['a', \"b,\"]$\n"
    "--- m1 d0\n"
    "j=[\"a,b,c\", 'd']$\n"
    "j2=[\",a,b\", 'c']$\n"
    "j3=['a', \"b,\"]$\n"
    "--- m1 d1\n"
    "## d$\n"
    "j=[\"a,b,c\", 'd']$\n"
    "j2=[\",a,b\", 'c']$\n"
    "j3=['a', \"b,\"]$\n"
    "--- m2 d0\n"
    "j=[\"a,b,c\", 'd']$\n"
    "j2=[\",a,b\", 'c']$\n"
    "j3=['a', \"b,\"]$\n"
    "--- m2 d1\n"
    "## d$\n"
    "j=[\"a,b,c\", 'd']$\n"
    "j2=[\",a,b\", 'c']$\n"
    "j3=['a', \"b,\"]$\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// WRITE_DEFAULTS
//   constant: what the cases of d_tests_arg_config_write_defaults come to, as
// the reference writes them with CLI11.
static const char WRITE_DEFAULTS[] =
    "W37 []\n"
    "--- m0 d0\n"
    "--- m0 d1\n"
    "--- m1 d0\n"
    "v=\"{}\"$\n"
    "w=\"{}\"$\n"
    "z=\"{}\"$\n"
    "u=\"[{},\\\\\"\\\\\"]\"$\n"
    "f=[\"{}\", 'x']$\n"
    "--- m1 d1\n"
    "## d$\n"
    "v=\"{}\"$\n"
    "w=\"{}\"$\n"
    "z=\"{}\"$\n"
    "u=\"[{},\\\\\"\\\\\"]\"$\n"
    "f=[\"{}\", 'x']$\n"
    "--- m2 d0\n"
    "v=\"{}\"$\n"
    "w=\"{}\"$\n"
    "z=\"{}\"$\n"
    "u=\"[{},\\\\\"\\\\\"]\"$\n"
    "f=[\"{}\", 'x']$\n"
    "--- m2 d1\n"
    "## d$\n"
    "v=\"{}\"$\n"
    "w=\"{}\"$\n"
    "z=\"{}\"$\n"
    "u=\"[{},\\\\\"\\\\\"]\"$\n"
    "f=[\"{}\", 'x']$\n"
    "W38 []\n"
    "--- m0 d0\n"
    "--- m0 d1\n"
    "--- m1 d0\n"
    "ERR ArgumentMismatch 114: --v: At most 1 required but received 2\n"
    "--- m1 d1\n"
    "ERR ArgumentMismatch 114: --v: At most 1 required but received 2\n"
    "--- m2 d0\n"
    "ERR ArgumentMismatch 114: --v: At most 1 required but received 2\n"
    "--- m2 d1\n"
    "ERR ArgumentMismatch 114: --v: At most 1 required but received 2\n"
    "W45 []\n"
    "--- m0 d0\n"
    "--- m0 d1\n"
    "--- m1 d0\n"
    "--- m1 d1\n"
    "--- m2 d0\n"
    "--- m2 d1\n"
    "W46 []\n"
    "--- m0 d0\n"
    "--- m0 d1\n"
    "--- m1 d0\n"
    "ERR ArgumentMismatch 114: --k: At least 1 required but received 0\n"
    "--- m1 d1\n"
    "ERR ArgumentMismatch 114: --k: At least 1 required but received 0\n"
    "--- m2 d0\n"
    "ERR ArgumentMismatch 114: --k: At least 1 required but received 0\n"
    "--- m2 d1\n"
    "ERR ArgumentMismatch 114: --k: At least 1 required but received 0\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// WRITE_LAYOUT
//   constant: what the cases of d_tests_arg_config_write_layout come to, as the
// reference writes them with CLI11.
static const char WRITE_LAYOUT[] =
    "W39 [--a 1 --b 2]\n"
    "--- m0 d0\n"
    "a=1$\n"
    "b=2$\n"
    "--- m0 d1\n"
    "## d$\n"
    "$\n"
    "## G Options$\n"
    "$\n"
    "# adesc$\n"
    "a=1$\n"
    "$\n"
    "## H Options$\n"
    "## hdesc$\n"
    "# bdesc$\n"
    "b=2$\n"
    "--- m1 d0\n"
    "a=1$\n"
    "b=2$\n"
    "--- m1 d1\n"
    "## d$\n"
    "$\n"
    "## G Options$\n"
    "$\n"
    "# adesc$\n"
    "a=1$\n"
    "$\n"
    "## H Options$\n"
    "## hdesc$\n"
    "# bdesc$\n"
    "b=2$\n"
    "--- m2 d0\n"
    "a=1$\n"
    "b=2$\n"
    "--- m2 d1\n"
    "## d$\n"
    "$\n"
    "## G Options$\n"
    "$\n"
    "# adesc$\n"
    "a=1$\n"
    "$\n"
    "## H Options$\n"
    "## hdesc$\n"
    "# bdesc$\n"
    "b=2$\n"
    "W40 [--a 1 --b 2 --c 3]\n"
    "--- m0 d0\n"
    "b=2$\n"
    "a=1$\n"
    "c=3$\n"
    "--- m0 d1\n"
    "## d$\n"
    "b=2$\n"
    "$\n"
    "## G Options$\n"
    "a=1$\n"
    "c=3$\n"
    "--- m1 d0\n"
    "b=2$\n"
    "a=1$\n"
    "c=3$\n"
    "--- m1 d1\n"
    "## d$\n"
    "b=2$\n"
    "$\n"
    "## G Options$\n"
    "a=1$\n"
    "c=3$\n"
    "--- m2 d0\n"
    "b=2$\n"
    "a=1$\n"
    "c=3$\n"
    "--- m2 d1\n"
    "## d$\n"
    "b=2$\n"
    "$\n"
    "## G Options$\n"
    "a=1$\n"
    "c=3$\n"
    "W41 [--a 1]\n"
    "--- m0 d0\n"
    "a=1$\n"
    "--- m0 d1\n"
    "## d$\n"
    "a=1$\n"
    "--- m1 d0\n"
    "a=1$\n"
    "x=7$\n"
    "--- m1 d1\n"
    "## d$\n"
    "a=1$\n"
    "$\n"
    "## og Options$\n"
    "## ogdesc$\n"
    "x=7$\n"
    "--- m2 d0\n"
    "a=1$\n"
    "x=7$\n"
    "--- m2 d1\n"
    "## d$\n"
    "a=1$\n"
    "$\n"
    "## og Options$\n"
    "## ogdesc$\n"
    "x=7$\n"
    "W42 [s --x 1 t --y 2]\n"
    "--- m0 d0\n"
    "p.s.x=1$\n"
    "[p.s.t]$\n"
    "y=2$\n"
    "--- m0 d1\n"
    "## d$\n"
    "## $\n"
    "p.s.x=1$\n"
    "[p.s.t]$\n"
    "## $\n"
    "y=2$\n"
    "--- m1 d0\n"
    "p.s.x=1$\n"
    "[p.s.t]$\n"
    "y=2$\n"
    "--- m1 d1\n"
    "## d$\n"
    "## $\n"
    "p.s.x=1$\n"
    "[p.s.t]$\n"
    "## $\n"
    "y=2$\n"
    "--- m2 d0\n"
    "p.s.x=1$\n"
    "[p.s.t]$\n"
    "y=2$\n"
    "--- m2 d1\n"
    "## d$\n"
    "## $\n"
    "p.s.x=1$\n"
    "[p.s.t]$\n"
    "## $\n"
    "y=2$\n"
    "W43 [--x 1 t --y 2]\n"
    "--- m0 d0\n"
    "px=1$\n"
    "[pt]$\n"
    "y=2$\n"
    "--- m0 d1\n"
    "## d$\n"
    "$\n"
    "## SUBCOMMANDS Options$\n"
    "## nl$\n"
    "px=1$\n"
    "[pt]$\n"
    "## $\n"
    "y=2$\n"
    "--- m1 d0\n"
    "px=1$\n"
    "[pt]$\n"
    "y=2$\n"
    "--- m1 d1\n"
    "## d$\n"
    "$\n"
    "## SUBCOMMANDS Options$\n"
    "## nl$\n"
    "px=1$\n"
    "[pt]$\n"
    "## $\n"
    "y=2$\n"
    "--- m2 d0\n"
    "px=1$\n"
    "[pt]$\n"
    "y=2$\n"
    "--- m2 d1\n"
    "## d$\n"
    "$\n"
    "## SUBCOMMANDS Options$\n"
    "## nl$\n"
    "px=1$\n"
    "[pt]$\n"
    "## $\n"
    "y=2$\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// DEPARTED_MOVED
//   constant: what the cases of d_tests_arg_config_departed_moved come to,
// the module's own (ISSUES 6): an option moved into an option group is the
// group's, and each group writes its own -- `moved=1`, then `moved2=2` --
// with its description, here empty, heading what it writes when
// descriptions are. CLI11 writes, for each command, the options whose
// parent is that command, and a moved option keeps its old parent: --moved,
// in `grp`, is written by neither the group nor the root, and --moved2, in
// `+pg`, whose options the root's pass takes, is written there, before the
// groups. A file it writes and reads back loses --moved.
static const char DEPARTED_MOVED[] =
    "W20 [--moved 1 --moved2 2]\n"
    "--- m0 d0\n"
    "moved=1$\n"
    "moved2=2$\n"
    "--- m0 d1\n"
    "## d$\n"
    "$\n"
    "## grp Options$\n"
    "## $\n"
    "moved=1$\n"
    "## $\n"
    "moved2=2$\n"
    "--- m1 d0\n"
    "moved=1$\n"
    "moved2=2$\n"
    "--- m1 d1\n"
    "## d$\n"
    "$\n"
    "## grp Options$\n"
    "## $\n"
    "moved=1$\n"
    "## $\n"
    "moved2=2$\n"
    "--- m2 d0\n"
    "moved=1$\n"
    "moved2=2$\n"
    "--- m2 d1\n"
    "## d$\n"
    "$\n"
    "## grp Options$\n"
    "## $\n"
    "moved=1$\n"
    "## $\n"
    "moved2=2$\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 0)

// UNBUILT
//   constant: what the cases of d_tests_arg_config_unbuilt come to, the
// module's own: without configuration files built in, a file the
// configuration option names that is there is refused as a ConfigError;
// one that is not there is CLI11's FileError, and a default that is not
// there is skipped, as with them.
static const char UNBUILT[] =
    "U01 [--config there.cfg]\n"
    "ERR ConfigError 110: there.cfg: configuration files are not built in\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[there.cfg]\n"
    "opt a n=0 res=[]\n"
    "U02 [--config gone.cfg]\n"
    "ERR FileError 103: gone.cfg was not readable (missing?)\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n"
    "opt config n=1 res=[gone.cfg]\n"
    "opt a n=0 res=[]\n"
    "U03 []\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt config n=0 res=[]\n"
    "opt a n=0 res=[]\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

// MADE_PARTS_MAX
//   constant: the room for the sections, and for the values, of an item
// made by hand.
enum
{
    MADE_PARTS_MAX = 3
};

// d_tests_arg_config_made
//   struct: an item made by hand, as the reference makes CLI11's
// ConfigItems: its tag, its sections, its name and its values, each a span
// that may hold NUL bytes; a list ends at its first span without data, or
// fills its room.
struct d_tests_arg_config_made
{
    const char*        tag;
    struct d_pack_text parents[MADE_PARTS_MAX];
    struct d_pack_text name;
    struct d_pack_text inputs[MADE_PARTS_MAX];
};

// MADE_ITEMS
//   constant: the items of d_tests_arg_config_items, as the reference makes
// them.
static const struct d_tests_arg_config_made MADE_ITEMS[] =
{
    { "I01",
      { { NULL, 0u } },
      { "a", 1u },
      { { NULL, 0u } } },
    { "I02",
      { { "s", 1u }, { "t", 1u } },
      { "a", 1u },
      { { "x", 1u } } },
    { "I03",
      { { "s", 1u } },
      { "b", 1u },
      { { "x", 1u }, { "y", 1u } } },
    { "I04",
      { { NULL, 0u } },
      { "c", 1u },
      { { "", 0u } } },
    { "I05",
      { { "", 0u } },
      { "", 0u },
      { { "1", 1u }, { "%%", 2u }, { "2", 1u } } },
    { "I06",
      { { "s.t", 3u } },
      { "n\x00m", 3u },
      { { "\x00", 1u } } },
    { "I07",
      { { "a", 1u }, { "b", 1u }, { "c", 1u } },
      { "--", 2u },
      { { NULL, 0u } } }
};

// MADE_COUNT
//   constant: how many items MADE_ITEMS holds.
static const size_t MADE_COUNT = sizeof(MADE_ITEMS) / sizeof(MADE_ITEMS[0]);

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// READ_SETTINGS_MAX
//   constant: the room for the settings one text is read under.
enum
{
    READ_SETTINGS_MAX = 4
};

// d_tests_arg_config_settings
//   struct: the format a text is read with, as the reference makes CLI11's
// ConfigBase: the name its lines carry; the characters that mark comments,
// arrays and their elements, values, strings, literals and sections; how
// many layers of sections are read; the occurrence of a repeated section
// read, -1 for all; whether a name may be given again; and the one section
// read, NULL for all.
struct d_tests_arg_config_settings
{
    const char* name;
    char        comment;
    char        array_start;
    char        array_end;
    char        array_separator;
    char        value_delimiter;
    char        string_quote;
    char        literal_quote;
    char        parent_separator;
    int         max_layers;
    int         index;
    bool        duplicates;
    const char* section;
};

// d_tests_arg_config_table
//   struct: the context of a table of the caller's: the text its reading
// refuses with, and how many times its operations have been called.
struct d_tests_arg_config_table
{
    const char* refusal;
    size_t      calls;
};

// d_tests_arg_config_read
//   struct: one text of a reading section: its tag; the text and its
// length, which may hold NUL bytes; and the settings it is read under, which
// end at the first NULL or fill the room.
struct d_tests_arg_config_read
{
    const char*                               tag;
    const char*                               text;
    size_t                                    length;
    const struct d_tests_arg_config_settings* settings[READ_SETTINGS_MAX];
};

// TOML
//   constant: CLI11's ConfigTOML, the default format.
static const struct d_tests_arg_config_settings TOML =
{
    "toml",
    '#', '[', ']', ',', '=', '"', '\'', '.',
    255, -1, false, NULL
};

// INI
//   constant: CLI11's ConfigINI: `;` comments, no array brackets, values
// separated by spaces.
static const struct d_tests_arg_config_settings INI =
{
    "ini",
    ';', '\0', '\0', ' ', '=', '"', '\'', '.',
    255, -1, false, NULL
};

// TOML_DUPLICATES
//   constant: TOML allowing a name given again.
static const struct d_tests_arg_config_settings TOML_DUPLICATES =
{
    "toml duplicates",
    '#', '[', ']', ',', '=', '"', '\'', '.',
    255, -1, true, NULL
};

// INI_DUPLICATES
//   constant: INI allowing a name given again.
static const struct d_tests_arg_config_settings INI_DUPLICATES =
{
    "ini duplicates",
    ';', '\0', '\0', ' ', '=', '"', '\'', '.',
    255, -1, true, NULL
};

// TOML_SUB
//   constant: TOML reading the section `sub` alone.
static const struct d_tests_arg_config_settings TOML_SUB =
{
    "toml section sub",
    '#', '[', ']', ',', '=', '"', '\'', '.',
    255, -1, false, "sub"
};

// TOML_A_1
//   constant: TOML reading the second occurrence of the section `a`.
static const struct d_tests_arg_config_settings TOML_A_1 =
{
    "toml section a 1",
    '#', '[', ']', ',', '=', '"', '\'', '.',
    255, 1, false, "a"
};

// TOML_ARR_0
//   constant: TOML reading the first occurrence of the section `arr`.
static const struct d_tests_arg_config_settings TOML_ARR_0 =
{
    "toml section arr 0",
    '#', '[', ']', ',', '=', '"', '\'', '.',
    255, 0, false, "arr"
};

// TOML_ARR_1
//   constant: TOML reading the second occurrence of the section `arr`.
static const struct d_tests_arg_config_settings TOML_ARR_1 =
{
    "toml section arr 1",
    '#', '[', ']', ',', '=', '"', '\'', '.',
    255, 1, false, "arr"
};

// TOML_LAYERS_0
//   constant: TOML reading no sections at all.
static const struct d_tests_arg_config_settings TOML_LAYERS_0 =
{
    "toml layers 0",
    '#', '[', ']', ',', '=', '"', '\'', '.',
    0, -1, false, NULL
};

// TOML_LAYERS_1
//   constant: TOML reading one layer of sections.
static const struct d_tests_arg_config_settings TOML_LAYERS_1 =
{
    "toml layers 1",
    '#', '[', ']', ',', '=', '"', '\'', '.',
    1, -1, false, NULL
};

// TOML_LAYERS_2
//   constant: TOML reading two layers of sections.
static const struct d_tests_arg_config_settings TOML_LAYERS_2 =
{
    "toml layers 2",
    '#', '[', ']', ',', '=', '"', '\'', '.',
    2, -1, false, NULL
};

// INI_LAYERS_1
//   constant: INI reading one layer of sections.
static const struct d_tests_arg_config_settings INI_LAYERS_1 =
{
    "ini layers 1",
    ';', '\0', '\0', ' ', '=', '"', '\'', '.',
    1, -1, false, NULL
};

// TOML_COLON
//   constant: TOML with `;` comments, `:` between a name and its value and `/`
// between sections.
static const struct d_tests_arg_config_settings TOML_COLON =
{
    "toml ; : /",
    ';', '[', ']', ',', ':', '"', '\'', '/',
    255, -1, false, NULL
};

// TOML_SWAPPED
//   constant: TOML with its two quotes swapped.
static const struct d_tests_arg_config_settings TOML_SWAPPED =
{
    "toml ' \"",
    '#', '[', ']', ',', '=', '\'', '"', '.',
    255, -1, false, NULL
};

// TOML_PARENS
//   constant: TOML with arrays in parentheses, separated by `;`.
static const struct d_tests_arg_config_settings TOML_PARENS =
{
    "toml ( ; )",
    '#', '(', ')', ';', '=', '"', '\'', '.',
    255, -1, false, NULL
};

// TOML_BARS
//   constant: TOML with arrays between bars.
static const struct d_tests_arg_config_settings TOML_BARS =
{
    "toml | , |",
    '#', '|', '|', ',', '=', '"', '\'', '.',
    255, -1, false, NULL
};

// INI_PARENS
//   constant: INI with arrays in parentheses, separated by `;`.
static const struct d_tests_arg_config_settings INI_PARENS =
{
    "ini ( ; )",
    ';', '(', ')', ';', '=', '"', '\'', '.',
    255, -1, false, NULL
};

// TOML_NO_COMMENT
//   constant: TOML without a comment character.
static const struct d_tests_arg_config_settings TOML_NO_COMMENT =
{
    "toml no comment",
    '\0', '[', ']', ',', '=', '"', '\'', '.',
    255, -1, false, NULL
};

// INI_NO_COMMENT
//   constant: INI without a comment character.
static const struct d_tests_arg_config_settings INI_NO_COMMENT =
{
    "ini no comment",
    '\0', '\0', '\0', ' ', '=', '"', '\'', '.',
    255, -1, false, NULL
};

// TOML_NO_DELIMITER
//   constant: TOML without a value delimiter.
static const struct d_tests_arg_config_settings TOML_NO_DELIMITER =
{
    "toml no delimiter",
    '#', '[', ']', ',', '\0', '"', '\'', '.',
    255, -1, false, NULL
};

// TOML_NO_PARENTS
//   constant: TOML without a parent separator.
static const struct d_tests_arg_config_settings TOML_NO_PARENTS =
{
    "toml no parents",
    '#', '[', ']', ',', '=', '"', '\'', '\0',
    255, -1, false, NULL
};

// TOML_EQUALS
//   constant: TOML with `=` as its comment character too.
static const struct d_tests_arg_config_settings TOML_EQUALS =
{
    "toml comment =",
    '=', '[', ']', ',', '=', '"', '\'', '.',
    255, -1, false, NULL
};

// INI_EQUALS
//   constant: INI with `=` as its comment character too.
static const struct d_tests_arg_config_settings INI_EQUALS =
{
    "ini comment =",
    '=', '\0', '\0', ' ', '=', '"', '\'', '.',
    255, -1, false, NULL
};

// TOML_NO_SECTION
//   constant: TOML asked for the section "", which is every section.
static const struct d_tests_arg_config_settings TOML_NO_SECTION =
{
    "toml section \"\"",
    '#', '[', ']', ',', '=', '"', '\'', '.',
    255, -1, false, ""
};

// TOML_SPACED
//   constant: TOML with arrays in brackets separated by spaces.
static const struct d_tests_arg_config_settings TOML_SPACED =
{
    "toml [ space ]",
    '#', '[', ']', ' ', '=', '"', '\'', '.',
    255, -1, false, NULL
};

// TOML_BRACKET
//   constant: TOML with `[` as its comment character.
static const struct d_tests_arg_config_settings TOML_BRACKET =
{
    "toml comment [",
    '[', '[', ']', ',', '=', '"', '\'', '.',
    255, -1, false, NULL
};

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

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
d_tests_arg_config_held(
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
 * @brief Appends one part of an item -- a section, its name or a value --
 *        escaped as d_tests_arg_escape escapes it, and `|`, which separates
 *        the parts of a list, as the escape of its code, 7c.
 *
 * @param[in,out] _out   the transcript.
 * @param[in]     _part  the part.
 */
static void
d_tests_arg_config_part(
    struct d_arg_buffer* _out,
    struct d_pack_text   _part
)
{
    // each byte: a `|` as its escape, anything else as d_tests_arg_escape
    // writes it
    for (size_t index = 0u; index < _part.length; ++index)
    {
        // the separator, or a byte of the part
        if (_part.data[index] == '|')
        {
            (void)d_arg_buffer_append_cstr(_out,
                                           "\\x7c");
        }
        else
        {
            d_tests_arg_escape(_out,
                               d_pack_text_from_span(_part.data + index,
                                                     1u));
        }
    }

    return;
}

/**
 * @brief Appends a list of an item's parts, separated by `|`.
 *
 * @param[in,out] _out   the transcript.
 * @param[in]     _list  the parts.
 */
static void
d_tests_arg_config_list(
    struct d_arg_buffer*          _out,
    const struct d_arg_text_list* _list
)
{
    // each part, after a `|` when one comes before it
    for (size_t index = 0u; index < _list->count; ++index)
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       (index > 0u) ? "|" : "");
        d_tests_arg_config_part(_out,
                                _list->items[index]);
    }

    return;
}

/**
 * @brief Appends an item's line, as the reference writes CLI11's
 *        ConfigItem: `P#<n>=<sections> N=<name> M=<multiline>
 *        I#<n>=<values>`, after `_lead`.
 *
 * @param[in,out] _out   the transcript.
 * @param[in]     _lead  what the line begins with.
 * @param[in]     _item  the item.
 */
static void
d_tests_arg_config_item(
    struct d_arg_buffer*            _out,
    const char*                     _lead,
    const struct d_arg_config_item* _item
)
{
    (void)d_arg_buffer_printf(_out,
                              "%sP#%lu=",
                              _lead,
                              (unsigned long)_item->parents.count);
    d_tests_arg_config_list(_out,
                            &_item->parents);
    (void)d_arg_buffer_append_cstr(_out,
                                   " N=");
    d_tests_arg_config_part(_out,
                            _item->name);
    (void)d_arg_buffer_printf(_out,
                              " M=%d I#%lu=",
                              (_item->multiline) ? 1 : 0,
                              (unsigned long)_item->inputs.count);
    d_tests_arg_config_list(_out,
                            &_item->inputs);
    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief Makes a format of the settings given: CLI11's ConfigTOML with each
 *        character, the layers, the section, its index and the switch for
 *        duplicates set as the settings say.
 *
 * @param[out] _format    the format; released by the caller.
 * @param[in]  _settings  the settings.
 * @return whether the section could be set.
 */
static bool
d_tests_arg_config_format_of(
    struct d_arg_config_format*               _format,
    const struct d_tests_arg_config_settings* _settings
)
{
    d_arg_config_format_init(_format);
    _format->comment          = _settings->comment;
    _format->array_start      = _settings->array_start;
    _format->array_end        = _settings->array_end;
    _format->array_separator  = _settings->array_separator;
    _format->value_delimiter  = _settings->value_delimiter;
    _format->string_quote     = _settings->string_quote;
    _format->literal_quote    = _settings->literal_quote;
    _format->parent_separator = _settings->parent_separator;
    d_arg_config_format_set_max_layers(_format,
                                       _settings->max_layers);
    d_arg_config_format_set_index(_format,
                                  _settings->index);
    d_arg_config_format_set_flag(_format,
                                 D_ARG_CONFIG_ALLOW_DUPLICATE_FIELDS,
                                 _settings->duplicates);

    return (d_arg_config_format_set_section(_format,
                                            _settings->section) == D_ARG_OK);
}

/**
 * @brief Reads a text under one settings and appends what came of it:
 *        `<settings> OK <items>` and a line for each item, or `<settings>
 *        ERR <status> <exit code>: <description>`.
 *
 * @param[in,out] _out       the transcript.
 * @param[in]     _text      the text.
 * @param[in]     _settings  the settings.
 * @return whether the format was made and the description kept.
 */
static bool
d_tests_arg_config_read_with(
    struct d_arg_buffer*                      _out,
    struct d_pack_text                        _text,
    const struct d_tests_arg_config_settings* _settings
)
{
    struct d_arg_config_format    format = { 0 };
    struct d_arg_config_item_list items  = D_ARG_LIST_INIT;
    struct d_arg_buffer           error  = D_ARG_BUFFER_INIT;
    const bool                    made   =
        d_tests_arg_config_format_of(&format,
                                     _settings);
    const int                     status = d_arg_config_from_text(&format,
                                                                  _text,
                                                                  &items,
                                                                  &error);

    // the items read, or the failure
    if (status == D_ARG_OK)
    {
        (void)d_arg_buffer_printf(_out,
                                  "%s OK %lu\n",
                                  _settings->name,
                                  (unsigned long)items.count);

        // each item
        for (size_t index = 0u; index < items.count; ++index)
        {
            d_tests_arg_config_item(_out,
                                    "  ",
                                    &items.items[index]);
        }
    }
    else
    {
        (void)d_arg_buffer_printf(_out,
                                  "%s ERR %s %d: ",
                                  _settings->name,
                                  d_arg_status_name(status),
                                  d_arg_status_exit_code(status));
        d_tests_arg_escape(_out,
                           d_arg_buffer_text(&error));
        (void)d_arg_buffer_append_char(_out,
                                       '\n');
    }

    const bool kept = (error.failed == 0);

    d_arg_config_item_list_release(&items);
    d_arg_buffer_release(&error);
    d_arg_config_format_release(&format);

    return ( (made) &&
             (kept) );
}

/**
 * @brief Reads a text under one settings from storage of exactly its length
 *        -- none, for no text -- so that a read past either end of it is
 *        AddressSanitizer's to see, and appends what came of it as
 *        d_tests_arg_config_read_with appends it.
 *
 * @param[in,out] _out       the transcript.
 * @param[in]     _text      the text.
 * @param[in]     _settings  the settings.
 * @return whether the storage was had, the format made and the description
 *         kept.
 */
static bool
d_tests_arg_config_read_exact(
    struct d_arg_buffer*                      _out,
    struct d_pack_text                        _text,
    const struct d_tests_arg_config_settings* _settings
)
{
    char* const exact = (_text.length > 0u)
                        ? (char*)malloc(_text.length)
                        : NULL;

    // storage that could not be had
    if ( (_text.length > 0u) &&
         (!exact) )
    {
        return false;
    }

    // the text's bytes, with nothing after them
    if (exact != NULL)
    {
        memcpy(exact,
               _text.data,
               _text.length);
    }

    const bool held = d_tests_arg_config_read_with(
                          _out,
                          d_pack_text_from_span(exact,
                                                _text.length),
                          _settings);

    free(exact);

    return held;
}

/**
 * @brief Reads a text that ends in one newline again without it -- the
 *        same lines to getline, the last now ending the storage -- and
 *        checks that it came to what the text came to.
 *
 * @param[in] _read      the read; its tag names a mismatch.
 * @param[in] _settings  the settings.
 * @param[in] _came      what the text came to under them.
 * @return whether the text without its newline came to the same, or the
 *         text does not end in one newline.
 */
static bool
d_tests_arg_config_read_cut(
    const struct d_tests_arg_config_read*     _read,
    const struct d_tests_arg_config_settings* _settings,
    const char*                               _came
)
{
    // a text not ending in one newline, whose lines would change
    if ( (_read->length < 2u)                       ||
         (_read->text[_read->length - 1u] != '\n')  ||
         (_read->text[_read->length - 2u] == '\n') )
    {
        return true;
    }

    struct d_arg_buffer cut  = D_ARG_BUFFER_INIT;
    struct d_arg_buffer name = D_ARG_BUFFER_INIT;
    const bool          read = d_tests_arg_config_read_exact(
                                   &cut,
                                   d_pack_text_from_span(_read->text,
                                                         _read->length - 1u),
                                   _settings);
    const int           told = d_arg_buffer_printf(&name,
                                                   "%s %s, without its last "
                                                   "newline",
                                                   _read->tag,
                                                   _settings->name);
    const bool          same = d_tests_arg_same(
                                   (told == D_ARG_OK) ? name.data
                                                      : _read->tag,
                                   d_arg_buffer_text(&cut),
                                   _came);
    const bool          kept = (cut.failed == 0);

    d_arg_buffer_release(&cut);
    d_arg_buffer_release(&name);

    return ( (read) &&
             (same) &&
             (kept) );
}

/**
 * @brief Appends a read's line, `<tag> [<text>]`, then what the text came
 *        to under each of its settings, each read from storage of exactly
 *        its length, and again without a last newline.
 *
 * @param[in,out] _out   the transcript.
 * @param[in]     _read  the read.
 * @return whether every format was made, every description kept, and every
 *         text read without its newline as it was with it.
 */
static bool
d_tests_arg_config_read(
    struct d_arg_buffer*                  _out,
    const struct d_tests_arg_config_read* _read
)
{
    const struct d_pack_text text = d_pack_text_from_span(_read->text,
                                                          _read->length);
    bool                     held = true;

    (void)d_arg_buffer_printf(_out,
                              "%s [",
                              _read->tag);
    d_tests_arg_escape(_out,
                       text);
    (void)d_arg_buffer_append_cstr(_out,
                                   "]\n");

    // each settings, until the first NULL
    for (size_t index = 0u;
         (index < (size_t)READ_SETTINGS_MAX) &&
         (_read->settings[index] != NULL);
         ++index)
    {
        const size_t before = _out->length;

        held = d_tests_arg_config_read_exact(_out,
                                             text,
                                             _read->settings[index]) && held;
        held = d_tests_arg_config_read_cut(_read,
                                           _read->settings[index],
                                           (_out->data != NULL)
                                           ? (_out->data + before)
                                           : "") && held;
    }

    return held;
}

/**
 * @brief Reads each text of a table under each of its settings and compares
 *        what they came to with the transcript expected.
 *
 * @param[in] _name      what the transcript is, for a printed mismatch.
 * @param[in] _reads     the reads.
 * @param[in] _count     how many.
 * @param[in] _expected  the transcript expected.
 * @return whether every read ran and the transcript was as expected.
 */
static bool
d_tests_arg_config_reads(
    const char*                           _name,
    const struct d_tests_arg_config_read* _reads,
    size_t                                _count,
    const char*                           _expected
)
{
    struct d_arg_buffer written = D_ARG_BUFFER_INIT;
    bool                held    = true;

    // each read
    for (size_t index = 0u; index < _count; ++index)
    {
        held = d_tests_arg_config_read(&written,
                                       &_reads[index]) && held;
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
 * @brief A format of the run's, kept until the run ends, which a command
 *        borrows: CLI11's ConfigTOML or ConfigINI with switches of its own,
 *        as the reference gives a command a format of its own.
 *
 * @param[in,out] _run      the run.
 * @param[in,out] _command  the command given the format.
 * @param[in]     _ini      whether the format is CLI11's ConfigINI.
 * @param[in]     _flags    the D_ARG_CONFIG_* switches turned on.
 * @return the format; NULL when it could not be kept, which fails the run.
 */
static struct d_arg_config_format*
d_tests_arg_config_kept(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _command,
    bool                    _ini,
    uint32_t                _flags
)
{
    struct d_arg_config_format* const format =
        d_tests_arg_keep(_run,
                         sizeof(struct d_arg_config_format));

    // a format that could not be kept has failed the run
    if (!format)
    {
        return NULL;
    }

    // CLI11's ConfigINI, or its ConfigTOML
    if (_ini)
    {
        d_arg_config_format_init_ini(format);
    }
    else
    {
        d_arg_config_format_init(format);
    }

    d_arg_config_format_set_flag(format,
                                 _flags,
                                 true);
    d_arg_command_set_config_format(_command,
                                    format);

    return format;
}

/**
 * @brief The writing reference's format of the caller's characters, kept
 *        and given to a command: `!` comments, `:` between a name and its
 *        value, arrays in `<` and `>` separated by `;`, `'` quoting strings
 *        and `"` literals, `/` between sections.
 *
 * @param[in,out] _run      the run.
 * @param[in,out] _command  the command given the format.
 */
static void
d_tests_arg_config_characters(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _command
)
{
    struct d_arg_config_format* const format =
        d_tests_arg_config_kept(_run,
                                _command,
                                false,
                                0u);

    // a format that was kept, its characters changed
    if (format != NULL)
    {
        format->comment          = '!';
        format->value_delimiter  = ':';
        format->array_start      = '<';
        format->array_end        = '>';
        format->array_separator  = ';';
        format->string_quote     = '\'';
        format->literal_quote    = '"';
        format->parent_separator = '/';
    }

    return;
}

/**
 * @brief The writing reference's TOML with arrays in parentheses separated
 *        by `;`, kept and given to a command; its brackets then mean nothing
 *        to it.
 *
 * @param[in,out] _run      the run.
 * @param[in,out] _command  the command given the format.
 */
static void
d_tests_arg_config_parenthesized(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _command
)
{
    struct d_arg_config_format* const format =
        d_tests_arg_config_kept(_run,
                                _command,
                                false,
                                0u);

    // a format that was kept, its arrays changed
    if (format != NULL)
    {
        format->array_start     = '(';
        format->array_end       = ')';
        format->array_separator = ';';
    }

    return;
}

/**
 * @brief The decimal number a value holds -- an optional `-`, then digits
 *        -- as the reference's std::stol reads the values it is given.
 *
 * @param[in] _value  the value.
 * @return the number; what the digits make before anything else.
 */
static long
d_tests_arg_config_number(
    struct d_pack_text _value
)
{
    const bool negative = ( (_value.length > 0u) &&
                            (_value.data[0] == '-') );
    long       number   = 0;

    // each digit, after the sign
    for (size_t index = (negative) ? 1u : 0u;
         (index < _value.length)     &&
         (_value.data[index] >= '0') &&
         (_value.data[index] <= '9');
         ++index)
    {
        number = (number * 10) + (long)(_value.data[index] - '0');
    }

    return (negative) ? -number : number;
}

/**
 * @brief A validator of the caller's, the reference's `bang`: a number
 *        above 5 refused, with a message of one character.
 *
 * @param[in]     _validator   the validator; unused.
 * @param[in,out] _validation  the value, and where the verdict goes.
 * @return D_ARG_OK, or the rejection's status.
 */
static int
d_tests_arg_config_bang(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    (void)_validator;

    return (d_tests_arg_config_number(_validation->value) > 5)
           ? d_arg_validation_reject(_validation,
                                     "!")
           : D_ARG_OK;
}

/**
 * @brief A transformation of the caller's, the reference's `doubled`: the
 *        value replaced by twice its number.
 *
 * @param[in]     _validator   the validator; unused.
 * @param[in,out] _validation  the value, replaced.
 * @return D_ARG_OK, or the replacement's status.
 */
static int
d_tests_arg_config_doubled(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    struct d_arg_buffer twice  = D_ARG_BUFFER_INIT;
    const int           made   = d_arg_buffer_printf(
                                     &twice,
                                     "%ld",
                                     d_tests_arg_config_number(
                                         _validation->value) * 2);
    const int           status = (made == D_ARG_OK)
                                 ? d_arg_validation_replace(
                                       _validation,
                                       d_arg_buffer_text(&twice))
                                 : made;

    (void)_validator;
    d_arg_buffer_release(&twice);

    return status;
}

/**
 * @brief A transformation of the caller's, the reference's `one`: `one`
 *        replaced by `1`, any other value left.
 *
 * @param[in]     _validator   the validator; unused.
 * @param[in,out] _validation  the value, replaced when it is `one`.
 * @return D_ARG_OK, or the replacement's status.
 */
static int
d_tests_arg_config_one(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    const bool one = ( (_validation->value.length == 3u) &&
                       (memcmp(_validation->value.data,
                               "one",
                               3u) == 0) );

    (void)_validator;

    return (one)
           ? d_arg_validation_replace(_validation,
                                      d_pack_text_from_cstr("1"))
           : D_ARG_OK;
}

/**
 * @brief An option's callback that notes `cb-<label> <first value>`, as the
 *        reference's callback prints the values CLI11 gives it.
 *
 * @param[in] _option   the option.
 * @param[in] _results  its values.
 * @param[in] _count    how many.
 * @param[in] _context  the case's run.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_config_value(
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
                     "cb-%s %.*s",
                     d_tests_arg_option_label(_context,
                                              _option),
                     (int)first.length,
                     first.data);

    return D_ARG_OK;
}

/**
 * @brief A final callback that notes `final-<label>`.
 *
 * @param[in] _command  the command.
 * @param[in] _context  the case's run.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_config_final(
    struct d_arg_command* _command,
    void*                 _context
)
{
    d_tests_arg_note(_context,
                     "final-%s",
                     d_tests_arg_command_label(_context,
                                               _command));

    return D_ARG_OK;
}

/**
 * @brief A parse-complete callback that notes `complete-<label>`.
 *
 * @param[in] _command  the command.
 * @param[in] _context  the case's run.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_config_complete(
    struct d_arg_command* _command,
    void*                 _context
)
{
    d_tests_arg_note(_context,
                     "complete-%s",
                     d_tests_arg_command_label(_context,
                                               _command));

    return D_ARG_OK;
}

/**
 * @brief A pre-parse callback that notes `pre-<label> <arguments left>`.
 *
 * @param[in] _command    the command.
 * @param[in] _remaining  the arguments left.
 * @param[in] _context    the case's run.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_config_preparse(
    struct d_arg_command* _command,
    size_t                _remaining,
    void*                 _context
)
{
    d_tests_arg_note(_context,
                     "pre-%s %lu",
                     d_tests_arg_command_label(_context,
                                               _command),
                     (unsigned long)_remaining);

    return D_ARG_OK;
}

/**
 * @brief Adds a case's line to the run's transcript: `<tag>`, what it
 *        parses (`stream`, or nothing), then the arguments or text in
 *        brackets, escaped, arguments separated by spaces.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _tag    the case's tag.
 * @param[in]     _what   ` stream` for configuration text; "" for
 *                        arguments.
 * @param[in]     _args   the arguments, or the one text.
 * @param[in]     _count  how many.
 */
static void
d_tests_arg_config_line(
    struct d_tests_arg_run* _run,
    const char*             _tag,
    const char*             _what,
    const char* const*      _args,
    size_t                  _count
)
{
    (void)d_arg_buffer_printf(&_run->transcript,
                              "%s%s [",
                              _tag,
                              _what);

    // each argument, after a space when one comes before it
    for (size_t index = 0u; index < _count; ++index)
    {
        (void)d_arg_buffer_append_cstr(&_run->transcript,
                                       (index > 0u) ? " " : "");
        d_tests_arg_escape(&_run->transcript,
                           d_pack_text_from_cstr(_args[index]));
    }

    (void)d_arg_buffer_append_cstr(&_run->transcript,
                                   "]\n");

    return;
}

/**
 * @brief Parses arguments as a case of a table would -- its line, then its
 *        report -- for a step that parses configuration text after.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _tag    the case's tag.
 * @param[in]     _args   the arguments.
 * @param[in]     _count  how many.
 */
static void
d_tests_arg_config_parsed(
    struct d_tests_arg_run* _run,
    const char*             _tag,
    const char* const*      _args,
    size_t                  _count
)
{
    d_tests_arg_config_line(_run,
                            _tag,
                            "",
                            _args,
                            _count);
    (void)d_tests_arg_parse(_run,
                            _args,
                            _count);

    return;
}

/**
 * @brief Parses configuration text as a file's, CLI11's parse_from_stream:
 *        its line, `<tag> stream [<text>]`, then the report.
 *
 * @param[in,out] _run   the run.
 * @param[in]     _tag   the case's tag.
 * @param[in]     _text  the configuration text.
 */
static void
d_tests_arg_config_streamed(
    struct d_tests_arg_run* _run,
    const char*             _tag,
    const char*             _text
)
{
    d_tests_arg_config_line(_run,
                            _tag,
                            " stream",
                            &_text,
                            1u);
    d_tests_arg_record(_run,
                       d_arg_parser_parse_config(&_run->parser,
                                                 d_pack_text_from_cstr(
                                                     _text)));

    return;
}

/**
 * @brief Appends configuration text as the reference writes it: each line
 *        end as `$` and a newline, anything else escaped.
 *
 * @param[in,out] _out   the transcript.
 * @param[in]     _text  the configuration text.
 */
static void
d_tests_arg_config_put(
    struct d_arg_buffer* _out,
    struct d_pack_text   _text
)
{
    size_t start = 0u;

    // each line, its end written `$`
    while (start < _text.length)
    {
        const char* const from = _text.data + start;
        const char* const end  = memchr(from,
                                        '\n',
                                        _text.length - start);
        const size_t      size = (end != NULL)
                                 ? (size_t)(end - from)
                                 : (_text.length - start);

        d_tests_arg_escape(_out,
                           d_pack_text_from_span(from,
                                                 size));
        (void)d_arg_buffer_append_cstr(_out,
                                       (end != NULL) ? "$\n" : "");
        start += size + 1u;
    }

    return;
}

/**
 * @brief Appends the parser's report as one line -- its status, exit code
 *        and place, then its message -- so that two reports compare as
 *        text.
 *
 * @param[in]     _run  the run.
 * @param[in,out] _out  receives the line.
 */
static void
d_tests_arg_config_report_text(
    const struct d_tests_arg_run* _run,
    struct d_arg_buffer*          _out
)
{
    const struct d_arg_error* const error = d_arg_parser_error(&_run->parser);

    (void)d_arg_buffer_printf(_out,
                              "%d %d %p %p ",
                              error->status,
                              error->exit_code,
                              (const void*)error->command,
                              (const void*)error->option);
    (void)d_arg_buffer_append_cstr(_out,
                                   d_arg_error_message(error));

    return;
}

/**
 * @brief Writes the root's configuration in one mode as the reference
 *        writes CLI11's to_config -- `--- m<mode> d<descriptions>`, then the
 *        text, or the ERR line of the failure the writing recorded -- and
 *        checks that writing that succeeded left the parser's report as it
 *        was, as CLI11's catching of a flag's refusal leaves it.
 *
 * @param[in,out] _run        the run.
 * @param[in]     _mode       the output mode.
 * @param[in]     _described  whether descriptions are written.
 * @param[in]     _prefix     what precedes each name.
 */
static void
d_tests_arg_config_write_one(
    struct d_tests_arg_run* _run,
    int                     _mode,
    bool                    _described,
    const char*             _prefix
)
{
    const struct d_arg_command* const       app    = &_run->parser.command;
    const struct d_arg_config_format* const format =
        d_arg_command_config_format(app);
    const struct d_pack_text                prefix =
        d_pack_text_from_cstr(_prefix);
    struct d_arg_buffer                     text   = D_ARG_BUFFER_INIT;
    struct d_arg_buffer                     before = D_ARG_BUFFER_INIT;
    struct d_arg_buffer                     after  = D_ARG_BUFFER_INIT;

    d_tests_arg_config_report_text(_run,
                                   &before);

    const int status = d_arg_config_to_text(format,
                                            app,
                                            _mode,
                                            _described,
                                            prefix,
                                            &text);

    (void)d_arg_buffer_printf(&_run->transcript,
                              "--- m%d d%d\n",
                              _mode,
                              (_described) ? 1 : 0);
    d_tests_arg_config_put(&_run->transcript,
                           d_arg_buffer_text(&text));
    d_tests_arg_record_error(_run,
                             status);
    d_tests_arg_config_report_text(_run,
                                   &after);

    const bool left = ( (status != D_ARG_OK)          ||
                        ( (before.data != NULL) &&
                          (after.data != NULL)  &&
                          (strcmp(after.data,
                                  before.data) == 0) ) );

    _run->failed = ( (_run->failed)                                   ||
                     (text.failed != 0)                               ||
                     (!d_tests_arg_config_held(left,
                                               "writing left the "
                                               "parser's report as it "
                                               "was")) );
    d_arg_buffer_release(&text);
    d_arg_buffer_release(&before);
    d_arg_buffer_release(&after);

    return;
}

/**
 * @brief Writes the root's configuration in each output mode, without
 *        descriptions and with them, as d_tests_arg_config_write_one
 *        writes it.
 *
 * @param[in,out] _run     the run.
 * @param[in]     _prefix  what precedes each name.
 */
static void
d_tests_arg_config_write_all(
    struct d_tests_arg_run* _run,
    const char*             _prefix
)
{
    // each mode, and each choice of descriptions
    for (int mode = 0; mode < 3; ++mode)
    {
        // without descriptions, then with them
        for (int described = 0; described < 2; ++described)
        {
            d_tests_arg_config_write_one(_run,
                                         mode,
                                         (described != 0),
                                         _prefix);
        }
    }

    return;
}

/**
 * @brief The start of a writing case: its line, then the parse of its
 *        arguments -- the ERR line of a failure, but no report.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _tag    the case's tag.
 * @param[in]     _args   the arguments.
 * @param[in]     _count  how many.
 */
static void
d_tests_arg_config_given(
    struct d_tests_arg_run* _run,
    const char*             _tag,
    const char* const*      _args,
    size_t                  _count
)
{
    d_tests_arg_config_line(_run,
                            _tag,
                            "",
                            _args,
                            _count);
    d_tests_arg_record_error(_run,
                             d_tests_arg_parse_quiet(_run,
                                                     _args,
                                                     _count));

    return;
}

/**
 * @brief A writing case: its line and its parse, as
 *        d_tests_arg_config_given has them, and the root's configuration
 *        written in each mode.
 *
 * @param[in,out] _run     the run.
 * @param[in]     _tag     the case's tag.
 * @param[in]     _args    the arguments.
 * @param[in]     _count   how many.
 * @param[in]     _prefix  what precedes each name written.
 */
static void
d_tests_arg_config_written(
    struct d_tests_arg_run* _run,
    const char*             _tag,
    const char* const*      _args,
    size_t                  _count,
    const char*             _prefix
)
{
    d_tests_arg_config_given(_run,
                             _tag,
                             _args,
                             _count);
    d_tests_arg_config_write_all(_run,
                                 _prefix);

    return;
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 1.  VALUES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// VALUE_READS
//   constant: the texts of d_tests_arg_config_values, and the settings each is
// read under.
static const struct d_tests_arg_config_read VALUE_READS[] =
{
    { "T01",
      "a=1\nb = 2\nc= three\n",
      19u,
      { &TOML, &INI } },
    { "T02",
      "flag\nab\nabc\nx\n",
      14u,
      { &TOML, &INI } },
    { "T03",
      "list = 1, 2, 3\nspaced = a b c\nmixed = a, b c\n",
      45u,
      { &TOML, &INI } },
    { "T04",
      "q = \"a b\"\nl = 'c d'\nb = `e f`\n",
      30u,
      { &TOML, &INI } },
    { "T05",
      "e = \"\"\nf = ''\ng =\nh = \n",
      23u,
      { &TOML, &INI } },
    { "T06",
      "# comment\n; also\na = 1 # trailing\nb = \"x # y\" # z\nc = 'p;q' ; r\n",
      64u,
      { &TOML, &INI } },
    { "T07",
      "a = # only a comment\nb=#\n",
      25u,
      { &TOML, &INI } }
};

// VALUE_COUNT
//   constant: how many cases VALUE_READS holds.
static const size_t VALUE_COUNT = sizeof(VALUE_READS) / sizeof(VALUE_READS[0]);

/*
d_tests_arg_config_values
  Names and values, lists, flags and quotes, read as TOML and as INI.
  Tests the following:
  - a value after `=`, spaced or not, and a name alone, read as true
  - lines shorter than three characters, skipped
  - lists separated by commas and by spaces
  - values quoted three ways, and empty ones
  - comment lines and trailing comments, outside quotes and inside
*/
bool
d_tests_arg_config_values(void)
{
    return d_tests_arg_config_reads("VALUES",
                                    VALUE_READS,
                                    VALUE_COUNT,
                                    VALUES);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 2.  SECTIONS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// SECTION_READS
//   constant: the texts of d_tests_arg_config_sections, and the settings each
// is read under.
static const struct d_tests_arg_config_read SECTION_READS[] =
{
    { "T08",
      "[sub]\nx=1\n[sub.deeper]\ny=2\n[other]\nz=3\n",
      39u,
      { &TOML, &INI } },
    { "T09",
      "[a.b.c]\nv=1\n[a.b.d]\nw=2\n[a.e]\nu=3\n[f]\nt=4\n",
      42u,
      { &TOML, &INI } },
    { "T10",
      "[a]\n[a.b]\n[a.b.c]\nx=1\n[a]\ny=2\n",
      30u,
      { &TOML, &INI } },
    { "T11",
      "[[arr]]\nx=1\n[[arr]]\nx=2\n",
      24u,
      { &TOML, &INI } },
    { "T12",
      "[default]\na=1\n[DEFAULT]\nb=2\n[sub]\nc=3\n[Default]\nd=4\n",
      52u,
      { &TOML, &INI } },
    { "T13",
      "[sub] # comment\nx=1\n[\"q.s\"] # c2\ny=2\n['li.t'] ; c3\nz=3\n",
      55u,
      { &TOML, &INI } }
};

// SECTION_COUNT
//   constant: how many cases SECTION_READS holds.
static const size_t SECTION_COUNT =
    sizeof(SECTION_READS) / sizeof(SECTION_READS[0]);

/*
d_tests_arg_config_sections
  Section headers: nested, repeated, arrays of tables, the default section,
comments after a header.
  Tests the following:
  - each section opened as `++` items and closed as `--` items, its names the
    items' parents
  - a section given again, and a deeper one opened inside it
  - the default section in any case, which is the top level
  - a header followed by a comment, quoted names in a header
*/
bool
d_tests_arg_config_sections(void)
{
    return d_tests_arg_config_reads("SECTIONS",
                                    SECTION_READS,
                                    SECTION_COUNT,
                                    SECTIONS);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 3.  HEADERS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// HEADER_READS
//   constant: the texts of d_tests_arg_config_headers, and the settings each is
// read under.
static const struct d_tests_arg_config_read HEADER_READS[] =
{
    { "T14",
      "[sub # not closed\nx=1\n",
      22u,
      { &TOML, &INI } },
    { "T15",
      "[ spaced ]\nx=1\n[a. b]\ny=2\n[a..b]\nz=3\n",
      37u,
      { &TOML, &INI } },
    { "T16",
      "[]\n[[]]\nx=1\n",
      12u,
      { &TOML, &INI } },
    { "T17",
      "[\"esc\\tsec\"]\nx=1\n",
      17u,
      { &TOML, &INI } },
    { "T18",
      "[\"bad\\qsec\"]\nx=1\n",
      17u,
      { &TOML, &INI } },
    { "T19",
      "x=0\n[s1]\n[s2]\n[s3.t]\n",
      21u,
      { &TOML, &INI } }
};

// HEADER_COUNT
//   constant: how many cases HEADER_READS holds.
static const size_t HEADER_COUNT =
    sizeof(HEADER_READS) / sizeof(HEADER_READS[0]);

/*
d_tests_arg_config_headers
  Headers that are not quite headers: unclosed, spaced, empty, escaped.
  Tests the following:
  - a header left open, read as a name
  - spaces inside a header, and empty parts of a dotted one
  - empty headers
  - an escape in a quoted header, and a malformed one
  - sections holding nothing
*/
bool
d_tests_arg_config_headers(void)
{
    return d_tests_arg_config_reads("HEADERS",
                                    HEADER_READS,
                                    HEADER_COUNT,
                                    HEADERS);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 4.  A SECTION CHOSEN
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// CHOSEN_READS
//   constant: the texts of d_tests_arg_config_chosen, and the settings each is
// read under.
static const struct d_tests_arg_config_read CHOSEN_READS[] =
{
    { "T01",
      "a=1\nb = 2\nc= three\n",
      19u,
      { &TOML_SUB } },
    { "T08",
      "[sub]\nx=1\n[sub.deeper]\ny=2\n[other]\nz=3\n",
      39u,
      { &TOML_SUB } },
    { "T09",
      "[a.b.c]\nv=1\n[a.b.d]\nw=2\n[a.e]\nu=3\n[f]\nt=4\n",
      42u,
      { &TOML_A_1 } },
    { "T10",
      "[a]\n[a.b]\n[a.b.c]\nx=1\n[a]\ny=2\n",
      30u,
      { &TOML_A_1 } },
    { "T11",
      "[[arr]]\nx=1\n[[arr]]\nx=2\n",
      24u,
      { &TOML_ARR_0, &TOML_ARR_1, &TOML_SUB } },
    { "T12",
      "[default]\na=1\n[DEFAULT]\nb=2\n[sub]\nc=3\n[Default]\nd=4\n",
      52u,
      { &TOML_SUB } },
    { "T13",
      "[sub] # comment\nx=1\n[\"q.s\"] # c2\ny=2\n['li.t'] ; c3\nz=3\n",
      55u,
      { &TOML_SUB } },
    { "T21",
      "[sec]\nsub.opt = 5\n\"quoted.name\" = 6\n",
      36u,
      { &TOML_SUB } }
};

// CHOSEN_COUNT
//   constant: how many cases CHOSEN_READS holds.
static const size_t CHOSEN_COUNT =
    sizeof(CHOSEN_READS) / sizeof(CHOSEN_READS[0]);

/*
d_tests_arg_config_chosen
  One section read as if it were the top level, and one occurrence of a repeated
section.
  Tests the following:
  - a section chosen by name, its subsections kept and the rest left out
  - an occurrence of a repeated section chosen by index, and one not there
  - a name with dots under the section, and text without sections
*/
bool
d_tests_arg_config_chosen(void)
{
    return d_tests_arg_config_reads("CHOSEN",
                                    CHOSEN_READS,
                                    CHOSEN_COUNT,
                                    CHOSEN);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 5.  LAYERS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// LAYER_READS
//   constant: the texts of d_tests_arg_config_layers, and the settings each is
// read under.
static const struct d_tests_arg_config_read LAYER_READS[] =
{
    { "T08",
      "[sub]\nx=1\n[sub.deeper]\ny=2\n[other]\nz=3\n",
      39u,
      { &TOML_LAYERS_0, &TOML_LAYERS_1 } },
    { "T09",
      "[a.b.c]\nv=1\n[a.b.d]\nw=2\n[a.e]\nu=3\n[f]\nt=4\n",
      42u,
      { &TOML_LAYERS_1 } },
    { "T10",
      "[a]\n[a.b]\n[a.b.c]\nx=1\n[a]\ny=2\n",
      30u,
      { &TOML_LAYERS_1 } },
    { "T13",
      "[sub] # comment\nx=1\n[\"q.s\"] # c2\ny=2\n['li.t'] ; c3\nz=3\n",
      55u,
      { &TOML_LAYERS_1, &INI_LAYERS_1 } },
    { "T20",
      "a.b = 1\na.b.c = 2\n\"a.b\".c = 3\n'x.y' = 4\n",
      40u,
      { &TOML_LAYERS_0, &TOML_LAYERS_1 } },
    { "T68",
      "a.\"b c\".d = 1\n",
      14u,
      { &TOML_LAYERS_1 } },
    { "T84",
      "[a.b.c.d]\nx=1\na.b.c.d.e = 2\n[a]\nb.c = 3\n",
      40u,
      { &TOML, &TOML_LAYERS_1, &TOML_LAYERS_2 } }
};

// LAYER_COUNT
//   constant: how many cases LAYER_READS holds.
static const size_t LAYER_COUNT = sizeof(LAYER_READS) / sizeof(LAYER_READS[0]);

/*
d_tests_arg_config_layers
  How deeply sections may nest and still be read.
  Tests the following:
  - no layers: only the top level
  - one layer, under TOML and INI, and two
  - parents from a header and from a dotted name counted alike
*/
bool
d_tests_arg_config_layers(void)
{
    return d_tests_arg_config_reads("LAYERS",
                                    LAYER_READS,
                                    LAYER_COUNT,
                                    LAYERS);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 6.  NAMES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// NAME_READS
//   constant: the texts of d_tests_arg_config_names, and the settings each is
// read under.
static const struct d_tests_arg_config_read NAME_READS[] =
{
    { "T20",
      "a.b = 1\na.b.c = 2\n\"a.b\".c = 3\n'x.y' = 4\n",
      40u,
      { &TOML, &INI } },
    { "T21",
      "[sec]\nsub.opt = 5\n\"quoted.name\" = 6\n",
      36u,
      { &TOML, &INI } },
    { "T22",
      "\"nm\\u00e9\" = 1\n\"bad\\x\" = 2\n",
      27u,
      { &TOML, &INI } },
    { "T23",
      "\"bad\\qname\".c = 1\n",
      18u,
      { &TOML, &INI } },
    { "T68",
      "a.\"b c\".d = 1\n",
      14u,
      { &TOML, &INI } },
    { "T78",
      "\"a b\" c = 1\n",
      12u,
      { &TOML, &INI } },
    { "T08",
      "[sub]\nx=1\n[sub.deeper]\ny=2\n[other]\nz=3\n",
      39u,
      { &TOML_NO_PARENTS } },
    { "T13",
      "[sub] # comment\nx=1\n[\"q.s\"] # c2\ny=2\n['li.t'] ; c3\nz=3\n",
      55u,
      { &TOML_NO_PARENTS } },
    { "T15",
      "[ spaced ]\nx=1\n[a. b]\ny=2\n[a..b]\nz=3\n",
      37u,
      { &TOML_NO_PARENTS } },
    { "T20",
      "a.b = 1\na.b.c = 2\n\"a.b\".c = 3\n'x.y' = 4\n",
      40u,
      { &TOML_NO_PARENTS } },
    { "T21",
      "[sec]\nsub.opt = 5\n\"quoted.name\" = 6\n",
      36u,
      { &TOML_NO_PARENTS } },
    { "T23",
      "\"bad\\qname\".c = 1\n",
      18u,
      { &TOML_NO_PARENTS } }
};

// NAME_COUNT
//   constant: how many cases NAME_READS holds.
static const size_t NAME_COUNT = sizeof(NAME_READS) / sizeof(NAME_READS[0]);

/*
d_tests_arg_config_names
  Dotted and quoted names, and names whose parts are not split.
  Tests the following:
  - a dotted name as parents, and one quoted whole
  - an escape in a quoted name, and malformed ones
  - a name of quoted parts with a space
  - names without a parent separator
*/
bool
d_tests_arg_config_names(void)
{
    return d_tests_arg_config_reads("NAMES",
                                    NAME_READS,
                                    NAME_COUNT,
                                    NAMES);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 7.  MULTILINE STRINGS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// MULTILINE_READS
//   constant: the texts of d_tests_arg_config_multiline, and the settings each
// is read under.
static const struct d_tests_arg_config_read MULTILINE_READS[] =
{
    { "T24",
      "m = '''one\ntwo\nthree'''\n",
      24u,
      { &TOML, &INI } },
    { "T25",
      "m = \"\"\"one\\ttab\ntwo\"\"\"\n",
      23u,
      { &TOML, &INI } },
    { "T26",
      "m = '''\nlead\n'''\n",
      17u,
      { &TOML, &INI } },
    { "T27",
      "m = '''same line'''\n",
      20u,
      { &TOML, &INI } },
    { "T28",
      "m = \"\"\"esc \\\"\"\" still\nend\"\"\"\n",
      29u,
      { &TOML, &INI } },
    { "T29",
      "m = \"\"\"join \\\n   next line\"\"\"\n",
      30u,
      { &TOML, &INI } },
    { "T30",
      "m = \"\"\"join \\\n   \\\n  more\n\"\"\"\n",
      30u,
      { &TOML, &INI } },
    { "T31",
      "m = '''a\n'''  # trailing comment\nn = 1\n",
      39u,
      { &TOML, &INI } },
    { "T32",
      "m = '''x''''\nn=2\n",
      17u,
      { &TOML, &INI } },
    { "T33",
      "m = '''x'''''\n",
      14u,
      { &TOML, &INI } },
    { "T34",
      "m = '''x''' junk\nmore'''\nn=3\n",
      29u,
      { &TOML, &INI } },
    { "T35",
      "m = \"\"\"bad \\q escape\"\"\"\n",
      24u,
      { &TOML, &INI } },
    { "T36",
      "m = \"\"\"never\nclosed\n",
      20u,
      { &TOML, &INI } },
    { "T37",
      "m = '''B\"(\\x41\\x00\\x42)\"'''\n",
      28u,
      { &TOML, &INI } },
    { "T38",
      "   m = '''lead spaces'''\n\tn = '''tab'''\n",
      40u,
      { &TOML, &INI } },
    { "T39",
      "\x0bm = '''vt'''\n",
      14u,
      { &TOML, &INI } },
    { "T40",
      "m = '''\r\nwin\r\n'''\r\n",
      19u,
      { &TOML, &INI } }
};

// MULTILINE_COUNT
//   constant: how many cases MULTILINE_READS holds.
static const size_t MULTILINE_COUNT =
    sizeof(MULTILINE_READS) / sizeof(MULTILINE_READS[0]);

/*
d_tests_arg_config_multiline
  Values in triple quotes running over lines.
  Tests the following:
  - literal and escaped triple quotes, closed on the line and later
  - a first line left empty, and a line ending in a backslash, joined
  - what follows the closing quotes: quotes, a comment, more text
  - a malformed escape, quotes never closed, and a binary string
  - leading spaces, a tab, a vertical tab and carriage returns
*/
bool
d_tests_arg_config_multiline(void)
{
    return d_tests_arg_config_reads("MULTILINE",
                                    MULTILINE_READS,
                                    MULTILINE_COUNT,
                                    MULTILINE);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 8.  COMMENTS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// COMMENT_READS
//   constant: the texts of d_tests_arg_config_comments, and the settings each
// is read under.
static const struct d_tests_arg_config_read COMMENT_READS[] =
{
    { "T41",
      "'''\ncomment\n'''\na=1\n",
      20u,
      { &TOML, &INI } },
    { "T42",
      "\"\"\"one line comment\"\"\"\nb=2\n",
      27u,
      { &TOML, &INI } },
    { "T43",
      "\"\"\"\"\"\"\nc=3\n",
      11u,
      { &TOML, &INI } },
    { "T44",
      "'''\nnever closed\na=1\n",
      21u,
      { &TOML, &INI } },
    { "T45",
      "\"\"\"x\"\"\"\nd=4\n",
      12u,
      { &TOML, &INI } },
    { "T06",
      "# comment\n; also\na = 1 # trailing\nb = \"x # y\" # z\nc = 'p;q' ; r\n",
      64u,
      { &TOML_NO_COMMENT } },
    { "T07",
      "a = # only a comment\nb=#\n",
      25u,
      { &TOML_NO_COMMENT } },
    { "T13",
      "[sub] # comment\nx=1\n[\"q.s\"] # c2\ny=2\n['li.t'] ; c3\nz=3\n",
      55u,
      { &TOML_NO_COMMENT } },
    { "T14",
      "[sub # not closed\nx=1\n",
      22u,
      { &TOML_NO_COMMENT } },
    { "T31",
      "m = '''a\n'''  # trailing comment\nn = 1\n",
      39u,
      { &TOML_NO_COMMENT } },
    { "T67",
      "\"a\" b = 1\n'x' 'y' = 2\n\"q\" # c\n",
      30u,
      { &TOML_NO_COMMENT } },
    { "T06",
      "# comment\n; also\na = 1 # trailing\nb = \"x # y\" # z\nc = 'p;q' ; r\n",
      64u,
      { &INI_NO_COMMENT } },
    { "T13",
      "[sub] # comment\nx=1\n[\"q.s\"] # c2\ny=2\n['li.t'] ; c3\nz=3\n",
      55u,
      { &INI_NO_COMMENT } },
    { "T76",
      "a = 1 ; not ini comment\n",
      24u,
      { &INI_NO_COMMENT } },
    { "T06",
      "# comment\n; also\na = 1 # trailing\nb = \"x # y\" # z\nc = 'p;q' ; r\n",
      64u,
      { &TOML_EQUALS, &INI_EQUALS } },
    { "T13",
      "[sub] # comment\nx=1\n[\"q.s\"] # c2\ny=2\n['li.t'] ; c3\nz=3\n",
      55u,
      { &TOML_EQUALS, &INI_EQUALS } },
    { "T61",
      "a = \"\", 2\na = 3, 4\n",
      19u,
      { &TOML_EQUALS, &INI_EQUALS } },
    { "T66",
      "=5\n\"a\"=6\n= \"x\"\n",
      15u,
      { &TOML_EQUALS, &INI_EQUALS } },
    { "T73",
      "x = \"a\" \"b\"\n",
      12u,
      { &TOML_EQUALS, &INI_EQUALS } },
    { "T07",
      "a = # only a comment\nb=#\n",
      25u,
      { &TOML_EQUALS } },
    { "T14",
      "[sub # not closed\nx=1\n",
      22u,
      { &TOML_EQUALS } },
    { "T31",
      "m = '''a\n'''  # trailing comment\nn = 1\n",
      39u,
      { &TOML_EQUALS } },
    { "T67",
      "\"a\" b = 1\n'x' 'y' = 2\n\"q\" # c\n",
      30u,
      { &TOML_EQUALS } },
    { "T74",
      "x = 'a'b\n",
      9u,
      { &TOML_EQUALS } },
    { "T75",
      "a=b=c\n",
      6u,
      { &TOML_EQUALS } },
    { "T74",
      "x = 'a'b\n",
      9u,
      { &INI_EQUALS } },
    { "T75",
      "a=b=c\n",
      6u,
      { &INI_EQUALS } },
    { "T76",
      "a = 1 ; not ini comment\n",
      24u,
      { &INI_EQUALS } }
};

// COMMENT_COUNT
//   constant: how many cases COMMENT_READS holds.
static const size_t COMMENT_COUNT =
    sizeof(COMMENT_READS) / sizeof(COMMENT_READS[0]);

/*
d_tests_arg_config_comments
  Comment blocks in triple quotes, and comment characters set otherwise.
  Tests the following:
  - a comment block over lines, on one line, empty, and never closed
  - no comment character: `#` and `;` lines still skipped
  - `=` as the comment character, against the value delimiter
*/
bool
d_tests_arg_config_comments(void)
{
    return d_tests_arg_config_reads("COMMENTS",
                                    COMMENT_READS,
                                    COMMENT_COUNT,
                                    COMMENTS);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 9.  ARRAYS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// ARRAY_READS
//   constant: the texts of d_tests_arg_config_arrays, and the settings each is
// read under.
static const struct d_tests_arg_config_read ARRAY_READS[] =
{
    { "T46",
      "a = [1, 2, 3]\nb = [\"x\", 'y']\nc = []\nd = [ ]\n",
      44u,
      { &TOML, &INI, &TOML_PARENS, &TOML_BARS } },
    { "T47",
      "a = [1,\n2,\n3]\nb = 4\n",
      20u,
      { &TOML, &INI, &TOML_PARENS, &TOML_BARS } },
    { "T48",
      "a = [1, # one\n  2, # two\n  3]\n",
      30u,
      { &TOML, &INI, &TOML_PARENS, &TOML_BARS } },
    { "T49",
      "a = [1, 2\n",
      10u,
      { &TOML, &INI, &TOML_PARENS, &TOML_BARS } },
    { "T50",
      "a = [[1, 2], [3]]\nb = [\"[x]\", '[y]']\n",
      37u,
      { &TOML, &INI, &TOML_PARENS, &TOML_BARS } },
    { "T51",
      "a = \"[x]\"\nb = '[y]'\nc = [z]\n",
      28u,
      { &TOML, &INI, &TOML_PARENS, &TOML_BARS } },
    { "T03",
      "list = 1, 2, 3\nspaced = a b c\nmixed = a, b c\n",
      45u,
      { &TOML_PARENS, &TOML_BARS, &INI_PARENS } },
    { "T06",
      "# comment\n; also\na = 1 # trailing\nb = \"x # y\" # z\nc = 'p;q' ; r\n",
      64u,
      { &TOML_PARENS, &TOML_BARS, &INI_PARENS } },
    { "T07",
      "a = # only a comment\nb=#\n",
      25u,
      { &TOML_PARENS, &TOML_BARS, &INI_PARENS } },
    { "T59",
      "a = 1, 2\na = 3, 4\n",
      18u,
      { &TOML_PARENS } },
    { "T62",
      "a = 1, 2\na = %%, 4\n",
      19u,
      { &TOML_PARENS } },
    { "T73",
      "x = \"a\" \"b\"\n",
      12u,
      { &TOML_PARENS } },
    { "T76",
      "a = 1 ; not ini comment\n",
      24u,
      { &TOML_PARENS } }
};

// ARRAY_COUNT
//   constant: how many cases ARRAY_READS holds.
static const size_t ARRAY_COUNT = sizeof(ARRAY_READS) / sizeof(ARRAY_READS[0]);

/*
d_tests_arg_config_arrays
  Arrays in brackets, over lines, nested and unclosed, and arrays marked
otherwise.
  Tests the following:
  - arrays of values and of quoted values, empty ones
  - an array over lines, with comments, and one never closed
  - brackets inside an array and inside quotes
  - arrays in parentheses and between bars: plain lists not split
*/
bool
d_tests_arg_config_arrays(void)
{
    return d_tests_arg_config_reads("ARRAYS",
                                    ARRAY_READS,
                                    ARRAY_COUNT,
                                    ARRAYS);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 10. ESCAPES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// ESCAPE_READS
//   constant: the texts of d_tests_arg_config_escapes, and the settings each is
// read under.
static const struct d_tests_arg_config_read ESCAPE_READS[] =
{
    { "T52",
      "b = B\"(\\x01\\x02\\xff)\"\nc = 'B\"(\\x41)\"'\n",
      38u,
      { &TOML, &INI, &TOML_SWAPPED } },
    { "T53",
      "s = \"tab\\there\"\nt = \"nl\\nhere\"\nu = \"\\u00e9\\U0001F600\"\n",
      54u,
      { &TOML, &INI, &TOML_SWAPPED } },
    { "T54",
      "s = \"bad \\q\"\n",
      13u,
      { &TOML, &INI, &TOML_SWAPPED } },
    { "T55",
      "s = \"\\ud800\"\n",
      13u,
      { &TOML, &INI, &TOML_SWAPPED } },
    { "T56",
      "s = \"end\\\\\"\nt = \"\\\"\"\n",
      21u,
      { &TOML, &INI, &TOML_SWAPPED } },
    { "T57",
      "s = \"nul\\0x\"\n",
      13u,
      { &TOML, &INI, &TOML_SWAPPED } }
};

// ESCAPE_COUNT
//   constant: how many cases ESCAPE_READS holds.
static const size_t ESCAPE_COUNT =
    sizeof(ESCAPE_READS) / sizeof(ESCAPE_READS[0]);

/*
d_tests_arg_config_escapes
  Escapes in quoted values and binary strings.
  Tests the following:
  - binary strings, and one in literal quotes
  - escapes of characters and of code points, a lone surrogate, and a malformed
    escape
  - an escaped backslash and quote, and an escaped NUL
  - the two quotes swapped
*/
bool
d_tests_arg_config_escapes(void)
{
    return d_tests_arg_config_reads("ESCAPES",
                                    ESCAPE_READS,
                                    ESCAPE_COUNT,
                                    ESCAPES);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 11. DUPLICATES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// DUPLICATE_READS
//   constant: the texts of d_tests_arg_config_duplicates, and the settings each
// is read under.
static const struct d_tests_arg_config_read DUPLICATE_READS[] =
{
    { "T11",
      "[[arr]]\nx=1\n[[arr]]\nx=2\n",
      24u,
      { &TOML, &INI, &TOML_DUPLICATES, &INI_DUPLICATES } },
    { "T58",
      "a = 1\na = 2\n",
      12u,
      { &TOML, &INI, &TOML_DUPLICATES, &INI_DUPLICATES } },
    { "T59",
      "a = 1, 2\na = 3, 4\n",
      18u,
      { &TOML, &INI, &TOML_DUPLICATES, &INI_DUPLICATES } },
    { "T60",
      "a = 1, 2\nb = 0\na = 3, 4\n",
      24u,
      { &TOML, &INI, &TOML_DUPLICATES, &INI_DUPLICATES } },
    { "T61",
      "a = \"\", 2\na = 3, 4\n",
      19u,
      { &TOML, &INI, &TOML_DUPLICATES, &INI_DUPLICATES } },
    { "T62",
      "a = 1, 2\na = %%, 4\n",
      19u,
      { &TOML, &INI, &TOML_DUPLICATES, &INI_DUPLICATES } },
    { "T63",
      "a = []\na = 1\n",
      13u,
      { &TOML, &INI } },
    { "T64",
      "a = 1\na = []\n",
      13u,
      { &TOML, &INI } },
    { "T89",
      "a = 1\n-- = 2\n-- = 3\n",
      20u,
      { &TOML, &TOML_DUPLICATES } }
};

// DUPLICATE_COUNT
//   constant: how many cases DUPLICATE_READS holds.
static const size_t DUPLICATE_COUNT =
    sizeof(DUPLICATE_READS) / sizeof(DUPLICATE_READS[0]);

/*
d_tests_arg_config_duplicates
  A name given twice, merged into its first item, and with duplicates allowed.
  Tests the following:
  - a value given twice, and lists given twice
  - another name between the two, and an empty value
  - the separator given in the text
  - a repeated array of tables, its names merged or not
  - a section marker's name given twice where no section is open
*/
bool
d_tests_arg_config_duplicates(void)
{
    return d_tests_arg_config_reads("DUPLICATES",
                                    DUPLICATE_READS,
                                    DUPLICATE_COUNT,
                                    DUPLICATES);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 12. ODD LINES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// ODD_LINE_READS
//   constant: the texts of d_tests_arg_config_odd_lines, and the settings each
// is read under.
static const struct d_tests_arg_config_read ODD_LINE_READS[] =
{
    { "T65",
      "-- = 1\n++ = 2\n[s]\n-- = 3\n",
      25u,
      { &TOML, &INI } },
    { "T66",
      "=5\n\"a\"=6\n= \"x\"\n",
      15u,
      { &TOML, &INI } },
    { "T67",
      "\"a\" b = 1\n'x' 'y' = 2\n\"q\" # c\n",
      30u,
      { &TOML, &INI } },
    { "T69",
      "a = 1\r\nb = 2\r\n[s]\r\nc = 3\r\n",
      26u,
      { &TOML, &INI } },
    { "T70",
      "\"unterminated = 1\n",
      18u,
      { &TOML, &INI } },
    { "T71",
      "'x = 2\n",
      7u,
      { &TOML, &INI } },
    { "T72",
      "\"a\\\" = 3\n",
      9u,
      { &TOML, &INI } },
    { "T73",
      "x = \"a\" \"b\"\n",
      12u,
      { &TOML, &INI } },
    { "T74",
      "x = 'a'b\n",
      9u,
      { &TOML, &INI } },
    { "T75",
      "a=b=c\n",
      6u,
      { &TOML, &INI } },
    { "T76",
      "a = 1 ; not ini comment\n",
      24u,
      { &TOML, &INI } },
    { "T77",
      "a b c\n",
      6u,
      { &TOML, &INI } }
};

// ODD_LINE_COUNT
//   constant: how many cases ODD_LINE_READS holds.
static const size_t ODD_LINE_COUNT =
    sizeof(ODD_LINE_READS) / sizeof(ODD_LINE_READS[0]);

/*
d_tests_arg_config_odd_lines
  Lines CLI11 reads in its own way.
  Tests the following:
  - the markers `--` and `++` as names
  - a line without a name, and names with spaces or quotes in them
  - carriage returns before line feeds
  - quotes never closed, an escaped closing quote, values of two quoted parts
  - more than one delimiter, a comment character INI does not have, words
    without one
*/
bool
d_tests_arg_config_odd_lines(void)
{
    return d_tests_arg_config_reads("ODD_LINES",
                                    ODD_LINE_READS,
                                    ODD_LINE_COUNT,
                                    ODD_LINES);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 13. DELIMITERS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// DELIMITER_READS
//   constant: the texts of d_tests_arg_config_delimiters, and the settings each
// is read under.
static const struct d_tests_arg_config_read DELIMITER_READS[] =
{
    { "T88",
      "a: 1\nb : 2 ; c\n[x/y]\nz: 3\nw/v: 4\n\"q/r\": 5\nt = 6\n",
      48u,
      { &TOML, &TOML_COLON } },
    { "T01",
      "a=1\nb = 2\nc= three\n",
      19u,
      { &TOML_COLON } },
    { "T06",
      "# comment\n; also\na = 1 # trailing\nb = \"x # y\" # z\nc = 'p;q' ; r\n",
      64u,
      { &TOML_COLON } },
    { "T08",
      "[sub]\nx=1\n[sub.deeper]\ny=2\n[other]\nz=3\n",
      39u,
      { &TOML_COLON } },
    { "T20",
      "a.b = 1\na.b.c = 2\n\"a.b\".c = 3\n'x.y' = 4\n",
      40u,
      { &TOML_COLON } },
    { "T65",
      "-- = 1\n++ = 2\n[s]\n-- = 3\n",
      25u,
      { &TOML_COLON } },
    { "T01",
      "a=1\nb = 2\nc= three\n",
      19u,
      { &TOML_NO_DELIMITER } },
    { "T03",
      "list = 1, 2, 3\nspaced = a b c\nmixed = a, b c\n",
      45u,
      { &TOML_NO_DELIMITER } },
    { "T08",
      "[sub]\nx=1\n[sub.deeper]\ny=2\n[other]\nz=3\n",
      39u,
      { &TOML_NO_DELIMITER } },
    { "T20",
      "a.b = 1\na.b.c = 2\n\"a.b\".c = 3\n'x.y' = 4\n",
      40u,
      { &TOML_NO_DELIMITER } }
};

// DELIMITER_COUNT
//   constant: how many cases DELIMITER_READS holds.
static const size_t DELIMITER_COUNT =
    sizeof(DELIMITER_READS) / sizeof(DELIMITER_READS[0]);

/*
d_tests_arg_config_delimiters
  The value delimiter and the parent separator set otherwise, and none.
  Tests the following:
  - `:` between names and values, `;` comments and `/` between sections
  - no value delimiter: every line a name alone
*/
bool
d_tests_arg_config_delimiters(void)
{
    return d_tests_arg_config_reads("DELIMITERS",
                                    DELIMITER_READS,
                                    DELIMITER_COUNT,
                                    DELIMITERS);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 14. BYTES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// BYTE_READS
//   constant: the texts of d_tests_arg_config_bytes, and the settings each is
// read under.
static const struct d_tests_arg_config_read BYTE_READS[] =
{
    { "T79",
      "a=x\x00y\nn\x00m=1\n\"q\x00r\"=2\n",
      20u,
      { &TOML, &INI } },
    { "T80",
      "s = \"a\\u0000b\"\nb = B\"(\\x00\\x01)\"\nm = \"\"\"c\\u0000d\"\"\"\n",
      52u,
      { &TOML, &INI } },
    { "T81",
      "name = \"\xc3\xa9t\xc3\xa9\"\n[s\xc3\xa9]\nv = \xc3\xbc, "
      "\xc3\xb6\n\xc3\xa9 = 1\n",
      39u,
      { &TOML, &INI } },
    { "T82",
      "a=1\nb=2",
      7u,
      { &TOML, &INI } },
    { "T83",
      "",
      0u,
      { &TOML, &INI } },
    { "T85",
      "v = xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
      "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
      "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
      "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
      "xxxxxxxxxxxxxxxxxxxx\nw = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, "
      "14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, "
      "31, 32, 33, 34, 35, 36, 37, 38, 39, 40]\n",
      461u,
      { &TOML, &INI } },
    { "T86",
      "k01=1\nk02=2\nk03=3\nk04=4\nk05=5\nk06=6\nk07=7\nk08=8\nk09=9\nk10=10\n"
      "k11=11\nk12=12\nk13=13\nk14=14\nk15=15\nk16=16\nk17=17\nk18=18\nk19=19"
      "\nk20=20\nk21=21\nk22=22\nk23=23\nk24=24\n",
      159u,
      { &TOML, &INI } },
    { "T87",
      "a=1\rb=2\r\nc=3\n",
      13u,
      { &TOML, &INI } }
};

// BYTE_COUNT
//   constant: how many cases BYTE_READS holds.
static const size_t BYTE_COUNT = sizeof(BYTE_READS) / sizeof(BYTE_READS[0]);

/*
d_tests_arg_config_bytes
  What the probe's corpus left out: NUL bytes, UTF-8, the end of the text, long
values and many names.
  Tests the following:
  - a NUL byte in a value and in names, and NULs from escapes
  - UTF-8 in names, sections and values
  - a last line without a newline, and no text
  - a value of 300 bytes, an array of 40, and 24 names
  - a carriage return that does not end a line
*/
bool
d_tests_arg_config_bytes(void)
{
    return d_tests_arg_config_reads("BYTES",
                                    BYTE_READS,
                                    BYTE_COUNT,
                                    BYTES);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 15. A SECTION ENTERED
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// ENTERED_READS
//   constant: the texts of d_tests_arg_config_entered, and the settings each is
// read under.
static const struct d_tests_arg_config_read ENTERED_READS[] =
{
    { "T92",
      "[sub]\na=1\nb.c=2\nd=3\n[other]\ne=4\n[sub]\nf=5\n",
      42u,
      { &TOML_SUB, &TOML_NO_SECTION } },
    { "T93",
      "arr.x = 1\n[default]\narr.y = 2\n",
      30u,
      { &TOML_ARR_0, &TOML_ARR_1 } },
    { "T94",
      "[sub.a.b.c]\nx=1\ny.z=2\n",
      22u,
      { &TOML_SUB } }
};

// ENTERED_COUNT
//   constant: how many cases ENTERED_READS holds.
static const size_t ENTERED_COUNT =
    sizeof(ENTERED_READS) / sizeof(ENTERED_READS[0]);

/*
d_tests_arg_config_entered
  The items of a section chosen, after the first that enters it, and a section
asked for as nothing.
  Tests the following:
  - items after the first of the section chosen, kept with their sections, and
    the section met again
  - the occurrence of a section counted from the top of the text
  - an item four sections deep, its first section taken off
  - a section asked for as nothing, which reads every section
*/
bool
d_tests_arg_config_entered(void)
{
    return d_tests_arg_config_reads("ENTERED",
                                    ENTERED_READS,
                                    ENTERED_COUNT,
                                    ENTERED);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 16. SECTIONS AFTER SECTIONS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// TRANSITION_READS
//   constant: the texts of d_tests_arg_config_transitions, and the settings
// each is read under.
static const struct d_tests_arg_config_read TRANSITION_READS[] =
{
    { "T95",
      "[a]\nx=1\n[a.b.c]\ny=2\n",
      20u,
      { &TOML, &INI } },
    { "T96",
      "[a.b.c]\nx=1\n[a.x.y.z]\ny=2\n",
      26u,
      { &TOML, &INI } },
    { "T97",
      "[a.b.c.d]\nx=1\n[a.b.c.d.e]\ny=2\n",
      30u,
      { &TOML, &INI } },
    { "T98",
      "-- = 1\n[s.t]\nx=2\n",
      17u,
      { &TOML, &INI } },
    { "T99",
      "a.b.-- = 2\n[c]\nx=1\n",
      19u,
      { &TOML, &INI } }
};

// TRANSITION_COUNT
//   constant: how many cases TRANSITION_READS holds.
static const size_t TRANSITION_COUNT =
    sizeof(TRANSITION_READS) / sizeof(TRANSITION_READS[0]);

/*
d_tests_arg_config_transitions
  Sections opened after others, as CLI11 closes the one before and opens the
sections between.
  Tests the following:
  - a section two levels below the one before
  - a section on another branch, three levels down
  - a section a level below one four deep
  - sections opened after a name that is a section's end, with and without
    sections of its own
*/
bool
d_tests_arg_config_transitions(void)
{
    return d_tests_arg_config_reads("TRANSITIONS",
                                    TRANSITION_READS,
                                    TRANSITION_COUNT,
                                    TRANSITIONS);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 17. HEADERS AT THE EDGES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// HEADER_EDGE_READS
//   constant: the texts of d_tests_arg_config_header_edges, and the settings
// each is read under.
static const struct d_tests_arg_config_read HEADER_EDGE_READS[] =
{
    { "T100",
      "[.a]\nb=1\n[c]\n.d = 2\n",
      20u,
      { &TOML, &INI } },
    { "T101",
      "[[a]\nx=1\n[b]]\ny=2\n",
      18u,
      { &TOML, &INI } },
    { "T102",
      "[a#b]\nx=1\n[c] # d\ny=2\n[e;f]\nz=3\n",
      32u,
      { &TOML, &INI } },
    { "T103",
      "[a #c\nx=1\n",
      10u,
      { &TOML, &INI } },
    { "T104",
      "[a \"#\"\n",
      7u,
      { &TOML, &INI } },
    { "T121",
      "[a] [b\nx=1\n[c]\ny=2\n",
      19u,
      { &TOML_BRACKET } }
};

// HEADER_EDGE_COUNT
//   constant: how many cases HEADER_EDGE_READS holds.
static const size_t HEADER_EDGE_COUNT =
    sizeof(HEADER_EDGE_READS) / sizeof(HEADER_EDGE_READS[0]);

/*
d_tests_arg_config_header_edges
  Headers whose parts are empty, whose brackets do not pair, or which hold the
comment character.
  Tests the following:
  - a header and a name starting with the section separator
  - a header opened with two brackets and closed with one, and the reverse
  - the comment character inside a header, and after one
  - a comment cut off a header, leaving too little
  - a header-like line whose comment character is quoted
  - `[` as the comment character, which cuts a header-like line to nothing at
    its first byte
*/
bool
d_tests_arg_config_header_edges(void)
{
    return d_tests_arg_config_reads("HEADER_EDGES",
                                    HEADER_EDGE_READS,
                                    HEADER_EDGE_COUNT,
                                    HEADER_EDGES);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 18. MULTILINE STRINGS AT THE EDGES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// MULTILINE_EDGE_READS
//   constant: the texts of d_tests_arg_config_multiline_edges, and the settings
// each is read under.
static const struct d_tests_arg_config_read MULTILINE_EDGE_READS[] =
{
    { "T105",
      "m = \"\"\"a\\\"\"\"\nb\"\"\"\nn = \"\"\"c\\\\\"\"\"\no = "
      "\"\"\"d\\\\\\\"\"\"\ne\"\"\"\np = '''f\\'''\nq = 1\n",
      71u,
      { &TOML, &INI } },
    { "T106",
      "m1 = \"\"\"\\\nx\"\"\"\nm2 = \"\"\"\n\n\nx\"\"\"\nm3 = "
      "\"\"\"\nx\"\"\"\nm4 = \"\"\"y\nx\"\"\"\nm5 = \"\"\"\n\"\"\"\nm6 = "
      "\"\"\"\n\n\"\"\"\nm7 = '''\"y\"'''\nm8 = \"\"\"x # y\nz\"\"\"\n",
      121u,
      { &TOML, &INI } }
};

// MULTILINE_EDGE_COUNT
//   constant: how many cases MULTILINE_EDGE_READS holds.
static const size_t MULTILINE_EDGE_COUNT =
    sizeof(MULTILINE_EDGE_READS) / sizeof(MULTILINE_EDGE_READS[0]);

/*
d_tests_arg_config_multiline_edges
  Closing quotes after backslashes, and the first lines of a value left empty.
  Tests the following:
  - closing quotes after one, two and three backslashes, and in a literal
  - a first line of a backslash alone, empty first lines, and lines of one
    character
  - values ending in an empty line, quotes kept inside a literal, and the
    comment character on the first line
*/
bool
d_tests_arg_config_multiline_edges(void)
{
    return d_tests_arg_config_reads("MULTILINE_EDGES",
                                    MULTILINE_EDGE_READS,
                                    MULTILINE_EDGE_COUNT,
                                    MULTILINE_EDGES);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 19. QUOTES AT THE EDGES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// QUOTE_EDGE_READS
//   constant: the texts of d_tests_arg_config_quote_edges, and the settings
// each is read under.
static const struct d_tests_arg_config_read QUOTE_EDGE_READS[] =
{
    { "T107",
      "a = \"\nb = '\nc = `\nd = x\n",
      24u,
      { &TOML, &INI } },
    { "T108",
      "a.'b=c' = 1\nab\"c=d\" = 2\n'e' 'f=g' = 3\n",
      38u,
      { &TOML, &INI } },
    { "T109",
      "\"a=b\"\n\"c#d\"\n",
      12u,
      { &TOML, &INI } },
    { "T110",
      "=55\n= 6\n",
      8u,
      { &TOML, &INI } },
    { "T111",
      "\"a=b\" c\n",
      8u,
      { &TOML, &INI } },
    { "T112",
      "\"[x]\" = 1\n'[y]' = 2\n",
      20u,
      { &TOML, &INI } },
    { "T113",
      "a = \"[]\"\nb = \"x]\"\nc = \"[x\"\nd = '[]'\ne = \"\"\n",
      43u,
      { &TOML, &INI } },
    { "T120",
      "a = 1\nh =\n",
      10u,
      { &TOML, &INI } }
};

// QUOTE_EDGE_COUNT
//   constant: how many cases QUOTE_EDGE_READS holds.
static const size_t QUOTE_EDGE_COUNT =
    sizeof(QUOTE_EDGE_READS) / sizeof(QUOTE_EDGE_READS[0]);

/*
d_tests_arg_config_quote_edges
  Quotes alone, quotes holding the delimiter or the comment character, names
that are only quoted, values in brackets quoted.
  Tests the following:
  - a value of one quote character
  - quoted parts of a name holding the delimiter
  - a quoted name alone holding the delimiter or the comment character
  - a line starting with the delimiter
  - a quoted name with brackets, and quoted values in brackets or only half in
    them
  - a value of nothing ending the text
*/
bool
d_tests_arg_config_quote_edges(void)
{
    return d_tests_arg_config_reads("QUOTE_EDGES",
                                    QUOTE_EDGE_READS,
                                    QUOTE_EDGE_COUNT,
                                    QUOTE_EDGES);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 20. ARRAYS AND LISTS AT THE EDGES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// LIST_EDGE_READS
//   constant: the texts of d_tests_arg_config_list_edges, and the settings each
// is read under.
static const struct d_tests_arg_config_read LIST_EDGE_READS[] =
{
    { "T114",
      "a = |\nb = ||\nc = |x|\n",
      21u,
      { &TOML_BARS } },
    { "T115",
      "a = ,b\nc = d,\ne = ,\n",
      20u,
      { &TOML, &INI } },
    { "T116",
      "a = [1 2 3]\nb = 1 2\nc = 1,2\n",
      28u,
      { &TOML_SPACED, &TOML } }
};

// LIST_EDGE_COUNT
//   constant: how many cases LIST_EDGE_READS holds.
static const size_t LIST_EDGE_COUNT =
    sizeof(LIST_EDGE_READS) / sizeof(LIST_EDGE_READS[0]);

/*
d_tests_arg_config_list_edges
  Arrays whose ends are one character, lists starting or ending with their
separator, and arrays separated by spaces.
  Tests the following:
  - an array opened and closed by one character
  - lists starting and ending with the separator
  - arrays separated by spaces, which are not lists outside the brackets
*/
bool
d_tests_arg_config_list_edges(void)
{
    return d_tests_arg_config_reads("LIST_EDGES",
                                    LIST_EDGE_READS,
                                    LIST_EDGE_COUNT,
                                    LIST_EDGES);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 21. NAMES MERGED
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// MERGE_READS
//   constant: the texts of d_tests_arg_config_merges, and the settings each is
// read under.
static const struct d_tests_arg_config_read MERGE_READS[] =
{
    { "T117",
      "a.x = 1\nb.x = 2\n",
      16u,
      { &TOML, &INI } },
    { "T118",
      "a = 1, \"\"\na = 3, 4\nb = 1, 2\nb = \"\", 4\nc = 1, %%\nc = 3, 4\n",
      57u,
      { &TOML, &INI, &TOML_DUPLICATES, &INI_DUPLICATES } },
    { "T119",
      "a = 1\na = 3, 4\nb = 1, 2\nb = 3\n",
      30u,
      { &TOML, &INI } }
};

// MERGE_COUNT
//   constant: how many cases MERGE_READS holds.
static const size_t MERGE_COUNT = sizeof(MERGE_READS) / sizeof(MERGE_READS[0]);

/*
d_tests_arg_config_merges
  Where a name given again merges, and where `%%` goes between its values.
  Tests the following:
  - the same name under other sections, not merged
  - an empty value or `%%` at either side of the join
  - one value given then several, and several then one
*/
bool
d_tests_arg_config_merges(void)
{
    return d_tests_arg_config_reads("MERGES",
                                    MERGE_READS,
                                    MERGE_COUNT,
                                    MERGES);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 22. DEPARTED READS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// DEPARTURE_READS
//   constant: the texts of d_tests_arg_config_departed_reads, and the settings
// each is read under.
static const struct d_tests_arg_config_read DEPARTURE_READS[] =
{
    { "T05",
      "e = \"\"\nf = ''\ng =\nh = \n",
      23u,
      { &TOML_EQUALS, &INI_EQUALS } },
    { "T63",
      "a = []\na = 1\n",
      13u,
      { &TOML_DUPLICATES, &INI_DUPLICATES } },
    { "T64",
      "a = 1\na = []\n",
      13u,
      { &TOML_DUPLICATES, &INI_DUPLICATES } },
    { "T90",
      "a = 1\na = [\n",
      12u,
      { &TOML_DUPLICATES } },
    { "T91",
      "[s]\n++ = 1\n",
      11u,
      { &TOML_DUPLICATES } },
    { "T120",
      "a = 1\nh =\n",
      10u,
      { &TOML_EQUALS, &INI_EQUALS } }
};

// DEPARTURE_COUNT
//   constant: how many cases DEPARTURE_READS holds.
static const size_t DEPARTURE_COUNT =
    sizeof(DEPARTURE_READS) / sizeof(DEPARTURE_READS[0]);

/*
d_tests_arg_config_departed_reads
  Text that makes CLI11 read past a value, which the module reads as a value
that is not there (ISSUES 14).
  Tests the following:
  - a value cut at a comment that is also the delimiter, and one of nothing
    ending the text
  - a name given again, with duplicates allowed, where one side has no values:
    an empty array, an array still open, a section marker
*/
bool
d_tests_arg_config_departed_reads(void)
{
    return d_tests_arg_config_reads("DEPARTED_READS",
                                    DEPARTURE_READS,
                                    DEPARTURE_COUNT,
                                    DEPARTED_READS);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 23. ITEMS
//==============================================================================

/**
 * @brief Makes an item as a made item describes it, through the calls a
 *        format of the caller's builds its items with.
 *
 * @param[out] _item  the item, as D_ARG_CONFIG_ITEM_INIT makes one.
 * @param[in]  _made  what it holds.
 * @return whether every part was copied in.
 */
static bool
d_tests_arg_config_make(
    struct d_arg_config_item*             _item,
    const struct d_tests_arg_config_made* _made
)
{
    bool made = (d_arg_config_item_set_name(_item,
                                            _made->name) == D_ARG_OK);

    // each section, until the first without data
    for (size_t index = 0u;
         (index < (size_t)MADE_PARTS_MAX) &&
         (_made->parents[index].data != NULL);
         ++index)
    {
        made = (d_arg_config_item_add_parent(_item,
                                             _made->parents[index])
                == D_ARG_OK) && made;
    }

    // each value, until the first without data
    for (size_t index = 0u;
         (index < (size_t)MADE_PARTS_MAX) &&
         (_made->inputs[index].data != NULL);
         ++index)
    {
        made = (d_arg_config_item_add_input(_item,
                                            _made->inputs[index])
                == D_ARG_OK) && made;
    }

    return made;
}

/**
 * @brief Appends a made item's line, its full name, and the text a flag
 *        takes from it through CLI11's to_flag -- or the status that
 *        refused it, as `flag ERR <status> <exit code>`.
 *
 * @param[in,out] _out     the transcript.
 * @param[in]     _format  the format whose to_flag is asked.
 * @param[in]     _made    the item.
 * @return whether the item was made and every text kept.
 */
static bool
d_tests_arg_config_flagged(
    struct d_arg_buffer*                  _out,
    const struct d_arg_config_format*     _format,
    const struct d_tests_arg_config_made* _made
)
{
    struct d_arg_config_item item  = D_ARG_CONFIG_ITEM_INIT;
    struct d_arg_buffer      name  = D_ARG_BUFFER_INIT;
    struct d_arg_buffer      flag  = D_ARG_BUFFER_INIT;
    const bool               made  = d_tests_arg_config_make(&item,
                                                             _made);
    const int                named = d_arg_config_item_fullname(&item,
                                                                &name);
    const int                taken = d_arg_config_to_flag(_format,
                                                          &item,
                                                          &flag);

    (void)d_arg_buffer_append_cstr(_out,
                                   _made->tag);
    d_tests_arg_config_item(_out,
                            " ",
                            &item);
    (void)d_arg_buffer_append_cstr(_out,
                                   "  fullname ");
    d_tests_arg_escape(_out,
                       d_arg_buffer_text(&name));
    (void)d_arg_buffer_append_cstr(_out,
                                   "\n  flag ");

    // the text a flag takes, or what refused it
    if (taken == D_ARG_OK)
    {
        d_tests_arg_escape(_out,
                           d_arg_buffer_text(&flag));
    }
    else
    {
        (void)d_arg_buffer_printf(_out,
                                  "ERR %s %d",
                                  d_arg_status_name(taken),
                                  d_arg_status_exit_code(taken));
    }

    (void)d_arg_buffer_append_char(_out,
                                   '\n');
    d_arg_config_item_release(&item);
    d_arg_buffer_release(&name);
    d_arg_buffer_release(&flag);

    return ( (made) &&
             (named == D_ARG_OK) );
}

/*
d_tests_arg_config_items
  Items made by hand, as a format of the caller's makes them: an item's full
name, CLI11's fullname(), and the text a flag takes from it, CLI11's
to_flag. In every build: the parse asks for a flag's text whether or not
configuration files are built in.
  Tests the following:
  - a full name: the sections and the name joined by `.`, the `.` that ends
    an empty name dropped, NUL bytes kept
  - a flag's text: its one value, `{}` for none, and more than one refused
  - an item of no sections, of an empty one, and a section marker
*/
bool
d_tests_arg_config_items(void)
{
    struct d_arg_config_format format  = { 0 };
    struct d_arg_buffer        written = D_ARG_BUFFER_INIT;
    bool                       held    = true;

    d_arg_config_format_init(&format);

    // each item
    for (size_t index = 0u; index < MADE_COUNT; ++index)
    {
        held = d_tests_arg_config_flagged(&written,
                                          &format,
                                          &MADE_ITEMS[index]) && held;
    }

    const bool same = d_tests_arg_same("ITEMS",
                                       d_arg_buffer_text(&written),
                                       ITEMS);
    const bool kept = (written.failed == 0);

    d_arg_buffer_release(&written);
    d_arg_config_format_release(&format);

    return ( (same) &&
             (held) &&
             (kept) );
}

//==============================================================================
// 24. FILES FOR OPTIONS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief E01: an option, a flag and a list.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e01(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");
    (void)d_tests_arg_make_flag(_run,
                                "b",
                                _app,
                                "--b");

    struct d_arg_option* const v = d_tests_arg_make_all(_run,
                                                        "v",
                                                        _app,
                                                        "--v");

    d_arg_option_set_expected(v,
                              -1);

    return;
}

/**
 * @brief E02: flags given false, a number, true, and a list.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e02(
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
    (void)d_tests_arg_make_flag(_run,
                                "b",
                                _app,
                                "--b");
    (void)d_tests_arg_make_flag(_run,
                                "c",
                                _app,
                                "--c");
    (void)d_tests_arg_make_flag(_run,
                                "d",
                                _app,
                                "--d");

    struct d_arg_option* const e = d_tests_arg_make_flag(_run,
                                                         "e",
                                                         _app,
                                                         "--e");

    d_arg_option_set_multi_option_policy(e,
                                         (int)D_ARG_MULTI_TAKE_ALL);

    return;
}

/**
 * @brief E03: options found by a short name, by either name, and by the
 *        short of two.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e03(
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
    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  _app,
                                  "-x");
    (void)d_tests_arg_make_option(_run,
                                  "y",
                                  _app,
                                  "-y,--yes");
    (void)d_tests_arg_make_option(_run,
                                  "z",
                                  _app,
                                  "--z,-w");

    return;
}

/**
 * @brief E04: a positional and an option.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e04(
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
    (void)d_tests_arg_make_option(_run,
                                  "pos",
                                  _app,
                                  "pos");
    (void)d_tests_arg_make_option(_run,
                                  "opt",
                                  _app,
                                  "--opt");

    return;
}

/**
 * @brief E13: an option given on the command line before the file.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e13(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");
    (void)d_tests_arg_make_option(_run,
                                  "b",
                                  _app,
                                  "--b");

    return;
}

/**
 * @brief E14: lists given twice, one with a separator injected.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e14(
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

    struct d_arg_option* const v = d_tests_arg_make_all(_run,
                                                        "v",
                                                        _app,
                                                        "--v");

    d_arg_option_set_expected(v,
                              -1);

    struct d_arg_option* const w = d_tests_arg_make_all(_run,
                                                        "w",
                                                        _app,
                                                        "--w");

    d_arg_option_set_expected(w,
                              -1);
    d_arg_option_set_inject_separator(w,
                                      true);

    return;
}

/**
 * @brief E18: two files, the later one read first.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e18(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");
    (void)d_tests_arg_make_option(_run,
                                  "b",
                                  _app,
                                  "--b");
    (void)d_tests_arg_make_all(_run,
                               "c",
                               _app,
                               "--c");

    return;
}

/**
 * @brief E32: `{}` for a list that may be empty, and `%%` in a list.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e32(
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

    struct d_arg_option* const v = d_tests_arg_make_all(_run,
                                                        "v",
                                                        _app,
                                                        "--v");

    d_arg_option_set_expected_range(v,
                                    0,
                                    10);

    struct d_arg_option* const w = d_tests_arg_make_all(_run,
                                                        "w",
                                                        _app,
                                                        "--w");

    d_arg_option_set_expected(w,
                              -1);

    return;
}

/**
 * @brief E34: an option of two values given three.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e34(
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

    struct d_arg_option* const p = d_tests_arg_make_option(_run,
                                                           "p",
                                                           _app,
                                                           "--p");

    d_arg_option_set_expected(p,
                              2);

    return;
}

/**
 * @brief E36: values over lines, escaped and literal.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e36(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");
    (void)d_tests_arg_make_option(_run,
                                  "b",
                                  _app,
                                  "--b");

    return;
}

/**
 * @brief E43: an option that takes a value given its name alone.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e43(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");
    (void)d_tests_arg_make_option(_run,
                                  "b",
                                  _app,
                                  "--b");

    return;
}

/**
 * @brief E44: an option that may take a value, and a flag, given their
 *        names alone.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e44(
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

    struct d_arg_option* const a = d_tests_arg_make_option(_run,
                                                           "a",
                                                           _app,
                                                           "--a");

    d_arg_option_set_expected_range(a,
                                    0,
                                    1);
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                _app,
                                "--f");

    return;
}

/**
 * @brief E49: a value split at a delimiter, and a list joined.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e49(
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

    struct d_arg_option* const a = d_tests_arg_make_all(_run,
                                                        "a",
                                                        _app,
                                                        "--a");

    d_arg_option_set_delimiter(a,
                               ',');

    struct d_arg_option* const j = d_tests_arg_make_option(_run,
                                                           "j",
                                                           _app,
                                                           "--j");

    d_arg_option_set_multi_option_policy(j,
                                         (int)D_ARG_MULTI_JOIN);
    d_arg_option_set_expected(j,
                              -1);

    return;
}

/**
 * @brief E50: spaces and tabs around names, values and a comment.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e50(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");
    (void)d_tests_arg_make_option(_run,
                                  "b",
                                  _app,
                                  "--b");

    return;
}

// E01_FILES
//   constant: the files E01 parses with.
static const struct d_tests_arg_file E01_FILES[] =
{
    { "E01.cfg", "a=1\nb=true\nv=[1,2,3]\n" },
    { NULL,      NULL }
};

// E02_FILES
//   constant: the files E02 parses with.
static const struct d_tests_arg_file E02_FILES[] =
{
    { "E02.cfg", "b=false\nc=2\nd=true\ne=[true,false,3]\n" },
    { NULL,      NULL }
};

// E03_FILES
//   constant: the files E03 parses with.
static const struct d_tests_arg_file E03_FILES[] =
{
    { "E03.cfg", "x=5\ny=3\nw=4\n" },
    { NULL,      NULL }
};

// E04_FILES
//   constant: the files E04 parses with.
static const struct d_tests_arg_file E04_FILES[] =
{
    { "E04.cfg", "pos=7\nopt=8\n" },
    { NULL,      NULL }
};

// E13_FILES
//   constant: the files E13 parses with.
static const struct d_tests_arg_file E13_FILES[] =
{
    { "E13.cfg", "a=1\nb=2\n" },
    { NULL,      NULL }
};

// E14_FILES
//   constant: the files E14 parses with.
static const struct d_tests_arg_file E14_FILES[] =
{
    { "E14.cfg", "v=[1,2]\nv=[3,4]\nw=[1,2]\nw=[3,4]\n" },
    { NULL,      NULL }
};

// E18_FILES
//   constant: the files E18 parses with.
static const struct d_tests_arg_file E18_FILES[] =
{
    { "E18a.cfg", "a=1\nc=x\n" },
    { "E18b.cfg", "a=2\nb=3\nc=y\n" },
    { NULL,       NULL }
};

// E32_FILES
//   constant: the files E32 parses with.
static const struct d_tests_arg_file E32_FILES[] =
{
    { "E32.cfg", "v={}\nw=[a,%%,b]\n" },
    { NULL,      NULL }
};

// E34_FILES
//   constant: the files E34 parses with.
static const struct d_tests_arg_file E34_FILES[] =
{
    { "E34.cfg", "p=[1,2,3]\n" },
    { NULL,      NULL }
};

// E36_FILES
//   constant: the files E36 parses with.
static const struct d_tests_arg_file E36_FILES[] =
{
    { "E36.cfg", "a = \"\"\"line\\tone\nline two\"\"\"\nb = '''raw\\t\n  "
                 "two'''\n" },
    { NULL,      NULL }
};

// E43_FILES
//   constant: the files E43 parses with.
static const struct d_tests_arg_file E43_FILES[] =
{
    { "E43.cfg", "a=1\nb\n" },
    { NULL,      NULL }
};

// E44_FILES
//   constant: the files E44 parses with.
static const struct d_tests_arg_file E44_FILES[] =
{
    { "E44.cfg", "a\nf\n" },
    { NULL,      NULL }
};

// E49_FILES
//   constant: the files E49 parses with.
static const struct d_tests_arg_file E49_FILES[] =
{
    { "E49.cfg", "a=\"x,y\"\nj=[p,q]\n" },
    { NULL,      NULL }
};

// E50_FILES
//   constant: the files E50 parses with.
static const struct d_tests_arg_file E50_FILES[] =
{
    { "E50.cfg", "  a  =  1  \n\tb\t=\t\"2 3\"\t# c\n" },
    { NULL,      NULL }
};

// FILE_OPTION_CASES
//   constant: the cases of d_tests_arg_config_file_options.
static const struct d_tests_arg_case FILE_OPTION_CASES[] =
{
    { "E01",
      d_tests_arg_config_e01,
      { "--config", "E01.cfg" },
      NULL,
      E01_FILES },
    { "E02",
      d_tests_arg_config_e02,
      { "--config", "E02.cfg" },
      NULL,
      E02_FILES },
    { "E03",
      d_tests_arg_config_e03,
      { "--config", "E03.cfg" },
      NULL,
      E03_FILES },
    { "E04",
      d_tests_arg_config_e04,
      { "--config", "E04.cfg" },
      NULL,
      E04_FILES },
    { "E13",
      d_tests_arg_config_e13,
      { "--a", "5", "--config", "E13.cfg" },
      NULL,
      E13_FILES },
    { "E14",
      d_tests_arg_config_e14,
      { "--config", "E14.cfg" },
      NULL,
      E14_FILES },
    { "E18",
      d_tests_arg_config_e18,
      { "--config", "E18a.cfg", "--config", "E18b.cfg" },
      NULL,
      E18_FILES },
    { "E32",
      d_tests_arg_config_e32,
      { "--config", "E32.cfg" },
      NULL,
      E32_FILES },
    { "E34",
      d_tests_arg_config_e34,
      { "--config", "E34.cfg" },
      NULL,
      E34_FILES },
    { "E36",
      d_tests_arg_config_e36,
      { "--config", "E36.cfg" },
      NULL,
      E36_FILES },
    { "E43",
      d_tests_arg_config_e43,
      { "--config", "E43.cfg" },
      NULL,
      E43_FILES },
    { "E44",
      d_tests_arg_config_e44,
      { "--config", "E44.cfg" },
      NULL,
      E44_FILES },
    { "E49",
      d_tests_arg_config_e49,
      { "--config", "E49.cfg" },
      NULL,
      E49_FILES },
    { "E50",
      d_tests_arg_config_e50,
      { "--config", "E50.cfg" },
      NULL,
      E50_FILES }
};

// FILE_OPTION_COUNT
//   constant: how many cases FILE_OPTION_CASES holds.
static const size_t FILE_OPTION_COUNT =
    sizeof(FILE_OPTION_CASES) / sizeof(FILE_OPTION_CASES[0]);

/*
d_tests_arg_config_file_options
  Options given values by a file the configuration option names.
  Tests the following:
  - an option, a flag and a list; flags given numbers and lists
  - names short and long, a positional's, and a name alone
  - an option the command line gave first, which the file leaves
  - lists given twice, a separator injected, `{}` and `%%` in a list
  - two files, an option given more values than it takes, values over lines,
    spaces around everything, a delimiter and a join
*/
bool
d_tests_arg_config_file_options(void)
{
    return d_tests_arg_cases("FILE_OPTIONS",
                             FILE_OPTION_CASES,
                             FILE_OPTION_COUNT,
                             FILE_OPTIONS);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 25. FILE VALUES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief E20: flags refusing an override, given 1, true and false.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e20(
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

    struct d_arg_option* const b = d_tests_arg_make_flag(_run,
                                                         "b",
                                                         _app,
                                                         "--b");

    d_arg_option_set_disable_flag_override(b,
                                           true);

    struct d_arg_option* const c = d_tests_arg_make_flag(_run,
                                                         "c",
                                                         _app,
                                                         "--c");

    d_arg_option_set_disable_flag_override(c,
                                           true);

    struct d_arg_option* const d = d_tests_arg_make_flag(_run,
                                                         "d",
                                                         _app,
                                                         "--d");

    d_arg_option_set_disable_flag_override(d,
                                           true);

    return;
}

/**
 * @brief E21: a flag refusing an override, given yes.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e21(
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

    struct d_arg_option* const b = d_tests_arg_make_flag(_run,
                                                         "b",
                                                         _app,
                                                         "--b");

    d_arg_option_set_disable_flag_override(b,
                                           true);

    return;
}

/**
 * @brief E22: flags refusing an override, given lists.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e22(
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

    struct d_arg_option* const b = d_tests_arg_make_flag(_run,
                                                         "b",
                                                         _app,
                                                         "--b");

    d_arg_option_set_multi_option_policy(b,
                                         (int)D_ARG_MULTI_TAKE_ALL);
    d_arg_option_set_disable_flag_override(b,
                                           true);

    struct d_arg_option* const c = d_tests_arg_make_flag(_run,
                                                         "c",
                                                         _app,
                                                         "--c");

    d_arg_option_set_disable_flag_override(c,
                                           true);

    return;
}

/**
 * @brief E23: flags with default values, and a negated name.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e23(
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
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                _app,
                                "--f{5}");
    (void)d_tests_arg_make_flag(_run,
                                "g",
                                _app,
                                "--g{5}");
    (void)d_tests_arg_make_flag(_run,
                                "h",
                                _app,
                                "--h{5}");
    (void)d_tests_arg_make_flag(_run,
                                "k",
                                _app,
                                "--k,!--nok");

    return;
}

/**
 * @brief E30: a malformed escape in a value.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e30(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    return;
}

/**
 * @brief E38: a flag given a list of two.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e38(
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
    (void)d_tests_arg_make_flag(_run,
                                "b",
                                _app,
                                "--b");

    return;
}

/**
 * @brief E40: escapes and a binary string, a NUL byte among them.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e40(
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
    (void)d_tests_arg_make_option(_run,
                                  "name",
                                  _app,
                                  "--name");
    (void)d_tests_arg_make_option(_run,
                                  "bin",
                                  _app,
                                  "--bin");

    return;
}

/**
 * @brief E46: a value its range refuses.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e46(
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

    struct d_arg_option* const a = d_tests_arg_make_option(_run,
                                                           "a",
                                                           _app,
                                                           "--a");

    (void)d_arg_option_check(a,
                             d_arg_validator_range_long(1,
                                                        5),
                             NULL);

    return;
}

/**
 * @brief E59: empty values for an option, a flag and a list.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e59(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");
    (void)d_tests_arg_make_flag(_run,
                                "b",
                                _app,
                                "--b");

    struct d_arg_option* const v = d_tests_arg_make_all(_run,
                                                        "v",
                                                        _app,
                                                        "--v");

    d_arg_option_set_expected_range(v,
                                    0,
                                    3);

    return;
}

// E20_FILES
//   constant: the files E20 parses with.
static const struct d_tests_arg_file E20_FILES[] =
{
    { "E20.cfg", "b=1\nc=true\nd=false\n" },
    { NULL,      NULL }
};

// E21_FILES
//   constant: the files E21 parses with.
static const struct d_tests_arg_file E21_FILES[] =
{
    { "E21.cfg", "b=yes\n" },
    { NULL,      NULL }
};

// E22_FILES
//   constant: the files E22 parses with.
static const struct d_tests_arg_file E22_FILES[] =
{
    { "E22.cfg", "b=[1,1,true]\nc=[1,1]\n" },
    { NULL,      NULL }
};

// E23_FILES
//   constant: the files E23 parses with.
static const struct d_tests_arg_file E23_FILES[] =
{
    { "E23.cfg", "f=true\ng=false\nh=7\nnok=true\n" },
    { NULL,      NULL }
};

// E30_FILES
//   constant: the files E30 parses with.
static const struct d_tests_arg_file E30_FILES[] =
{
    { "E30.cfg", "a=\"bad\\q\"\n" },
    { NULL,      NULL }
};

// E38_FILES
//   constant: the files E38 parses with.
static const struct d_tests_arg_file E38_FILES[] =
{
    { "E38.cfg", "b=[true,false]\n" },
    { NULL,      NULL }
};

// E40_FILES
//   constant: the files E40 parses with.
static const struct d_tests_arg_file E40_FILES[] =
{
    { "E40.cfg", "name=\"tab\\there\\u00e9\"\nbin=B\"(\\x41\\x00\\x42)\"\n" },
    { NULL,      NULL }
};

// E46_FILES
//   constant: the files E46 parses with.
static const struct d_tests_arg_file E46_FILES[] =
{
    { "E46.cfg", "a=9\n" },
    { NULL,      NULL }
};

// E59_FILES
//   constant: the files E59 parses with.
static const struct d_tests_arg_file E59_FILES[] =
{
    { "E59.cfg", "a=\nb=\nv=[]\n" },
    { NULL,      NULL }
};

// FILE_VALUE_CASES
//   constant: the cases of d_tests_arg_config_file_values.
static const struct d_tests_arg_case FILE_VALUE_CASES[] =
{
    { "E20",
      d_tests_arg_config_e20,
      { "--config", "E20.cfg" },
      NULL,
      E20_FILES },
    { "E21",
      d_tests_arg_config_e21,
      { "--config", "E21.cfg" },
      NULL,
      E21_FILES },
    { "E22",
      d_tests_arg_config_e22,
      { "--config", "E22.cfg" },
      NULL,
      E22_FILES },
    { "E23",
      d_tests_arg_config_e23,
      { "--config", "E23.cfg" },
      NULL,
      E23_FILES },
    { "E30",
      d_tests_arg_config_e30,
      { "--config", "E30.cfg" },
      NULL,
      E30_FILES },
    { "E38",
      d_tests_arg_config_e38,
      { "--config", "E38.cfg" },
      NULL,
      E38_FILES },
    { "E40",
      d_tests_arg_config_e40,
      { "--config", "E40.cfg" },
      NULL,
      E40_FILES },
    { "E46",
      d_tests_arg_config_e46,
      { "--config", "E46.cfg" },
      NULL,
      E46_FILES },
    { "E59",
      d_tests_arg_config_e59,
      { "--config", "E59.cfg" },
      NULL,
      E59_FILES }
};

// FILE_VALUE_COUNT
//   constant: how many cases FILE_VALUE_CASES holds.
static const size_t FILE_VALUE_COUNT =
    sizeof(FILE_VALUE_CASES) / sizeof(FILE_VALUE_CASES[0]);

/*
d_tests_arg_config_file_values
  What a file's values come to: flags, escapes, validation, empty values.
  Tests the following:
  - flags that refuse an override given numbers, true, false, `yes`, and lists
  - flags with default values, and a negated name
  - a malformed escape, and escapes and a binary string kept
  - a flag given a list, and a value its validator refuses
  - empty values for an option, a flag and a list
*/
bool
d_tests_arg_config_file_values(void)
{
    return d_tests_arg_cases("FILE_VALUES",
                             FILE_VALUE_CASES,
                             FILE_VALUE_COUNT,
                             FILE_VALUES);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 26. FILES FOR COMMANDS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief E05: a section for a subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e05(
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

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    return;
}

/**
 * @brief E06: a section inside a subcommand's.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e06(
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

    struct d_arg_command* const sub  = d_tests_arg_make_subcommand(_run,
                                                                   _app,
                                                                   "sub");
    struct d_arg_command* const deep = d_tests_arg_make_subcommand(_run,
                                                                   sub,
                                                                   "deep");

    (void)d_tests_arg_make_option(_run,
                                  "y",
                                  deep,
                                  "--y");
    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");

    return;
}

/**
 * @brief E07: dotted names for a subcommand and one inside it.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e07(
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

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");

    struct d_arg_command* const deep = d_tests_arg_make_subcommand(_run,
                                                                   sub,
                                                                   "deep");

    (void)d_tests_arg_make_option(_run,
                                  "y",
                                  deep,
                                  "--y");

    return;
}

/**
 * @brief E19: an INI file, its lists separated by spaces.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e19(
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
    (void)d_tests_arg_config_kept(_run,
                                  _app,
                                  true,
                                  0u);

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");

    struct d_arg_option* const v = d_tests_arg_make_all(_run,
                                                        "v",
                                                        sub,
                                                        "--v");

    d_arg_option_set_expected(v,
                              -1);
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    return;
}

/**
 * @brief E24: options in an option group, and in one shown with its parent.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e24(
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

    struct d_arg_command* const grp = d_tests_arg_make_group(_run,
                                                             _app,
                                                             "grp");

    (void)d_tests_arg_make_option(_run,
                                  "gx",
                                  grp,
                                  "--gx");

    struct d_arg_command* const plus = d_arg_command_add_option_group(_app,
                                                                      "+plus",
                                                                      NULL);

    (void)d_tests_arg_watch_command(_run,
                                    "plus",
                                    plus);
    (void)d_tests_arg_make_option(_run,
                                  "hx",
                                  plus,
                                  "--hx");

    return;
}

/**
 * @brief E25: a subcommand's section naming its parent's option.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e25(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_fallthrough(sub,
                                  true);
    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");

    return;
}

/**
 * @brief E26: the default section, in two cases.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e26(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");
    (void)d_tests_arg_make_option(_run,
                                  "b",
                                  _app,
                                  "--b");

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
 * @brief E27: a section for a subcommand that is not configurable.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e27(
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

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_configurable(sub,
                                   false);
    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");

    return;
}

/**
 * @brief E33: a subcommand's section repeated.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e33(
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

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    (void)d_tests_arg_make_all(_run,
                               "x",
                               sub,
                               "--x");

    return;
}

/**
 * @brief E39: an option group inside a subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e39(
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

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");
    struct d_arg_command* const og  = d_tests_arg_make_group(_run,
                                                             sub,
                                                             "og");

    (void)d_tests_arg_make_option(_run,
                                  "gx",
                                  og,
                                  "--gx");

    return;
}

/**
 * @brief E42: a format reading only the subcommand's section.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e42(
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
    (void)d_arg_config_format_set_section(&_run->parser.config_format,
                                          "sub");

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");
    (void)d_tests_arg_make_option(_run,
                                  "rx",
                                  _app,
                                  "--x");

    return;
}

/**
 * @brief E48: a subcommand the command line used, then a file.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e48(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");

    return;
}

// E05_FILES
//   constant: the files E05 parses with.
static const struct d_tests_arg_file E05_FILES[] =
{
    { "E05.cfg", "a=0\n[sub]\nx=1\n" },
    { NULL,      NULL }
};

// E06_FILES
//   constant: the files E06 parses with.
static const struct d_tests_arg_file E06_FILES[] =
{
    { "E06.cfg", "[sub]\nx=1\n[sub.deep]\ny=2\n" },
    { NULL,      NULL }
};

// E07_FILES
//   constant: the files E07 parses with.
static const struct d_tests_arg_file E07_FILES[] =
{
    { "E07.cfg", "sub.x=2\nsub.deep.y=3\n" },
    { NULL,      NULL }
};

// E19_FILES
//   constant: the files E19 parses with.
static const struct d_tests_arg_file E19_FILES[] =
{
    { "E19.cfg", "; comment\na = 1 ; tail\n[sub]\nx = 1\nv = 1 2 3\n" },
    { NULL,      NULL }
};

// E24_FILES
//   constant: the files E24 parses with.
static const struct d_tests_arg_file E24_FILES[] =
{
    { "E24.cfg", "gx=1\nhx=2\n" },
    { NULL,      NULL }
};

// E25_FILES
//   constant: the files E25 parses with.
static const struct d_tests_arg_file E25_FILES[] =
{
    { "E25.cfg", "[sub]\na=1\nx=2\n" },
    { NULL,      NULL }
};

// E26_FILES
//   constant: the files E26 parses with.
static const struct d_tests_arg_file E26_FILES[] =
{
    { "E26.cfg", "[sub]\nx=1\n[default]\na=2\n[DEFAULT]\nb=3\n" },
    { NULL,      NULL }
};

// E27_FILES
//   constant: the files E27 parses with.
static const struct d_tests_arg_file E27_FILES[] =
{
    { "E27.cfg", "[sub]\nx=1\n" },
    { NULL,      NULL }
};

// E33_FILES
//   constant: the files E33 parses with.
static const struct d_tests_arg_file E33_FILES[] =
{
    { "E33.cfg", "[[sub]]\nx=1\n[[sub]]\nx=2\n" },
    { NULL,      NULL }
};

// E39_FILES
//   constant: the files E39 parses with.
static const struct d_tests_arg_file E39_FILES[] =
{
    { "E39.cfg", "[sub]\ngx=4\n" },
    { NULL,      NULL }
};

// E42_FILES
//   constant: the files E42 parses with.
static const struct d_tests_arg_file E42_FILES[] =
{
    { "E42.cfg", "x=0\n[sub]\nx=1\n[other]\nx=2\n" },
    { NULL,      NULL }
};

// E48_FILES
//   constant: the files E48 parses with.
static const struct d_tests_arg_file E48_FILES[] =
{
    { "E48.cfg", "a=1\n[sub]\nx=2\n" },
    { NULL,      NULL }
};

// FILE_COMMAND_CASES
//   constant: the cases of d_tests_arg_config_file_commands.
static const struct d_tests_arg_case FILE_COMMAND_CASES[] =
{
    { "E05",
      d_tests_arg_config_e05,
      { "--config", "E05.cfg" },
      NULL,
      E05_FILES },
    { "E06",
      d_tests_arg_config_e06,
      { "--config", "E06.cfg" },
      NULL,
      E06_FILES },
    { "E07",
      d_tests_arg_config_e07,
      { "--config", "E07.cfg" },
      NULL,
      E07_FILES },
    { "E19",
      d_tests_arg_config_e19,
      { "--config", "E19.cfg" },
      NULL,
      E19_FILES },
    { "E24",
      d_tests_arg_config_e24,
      { "--config", "E24.cfg" },
      NULL,
      E24_FILES },
    { "E25",
      d_tests_arg_config_e25,
      { "--config", "E25.cfg" },
      NULL,
      E25_FILES },
    { "E26",
      d_tests_arg_config_e26,
      { "--config", "E26.cfg" },
      NULL,
      E26_FILES },
    { "E27",
      d_tests_arg_config_e27,
      { "--config", "E27.cfg" },
      NULL,
      E27_FILES },
    { "E33",
      d_tests_arg_config_e33,
      { "--config", "E33.cfg" },
      NULL,
      E33_FILES },
    { "E39",
      d_tests_arg_config_e39,
      { "--config", "E39.cfg" },
      NULL,
      E39_FILES },
    { "E42",
      d_tests_arg_config_e42,
      { "--config", "E42.cfg" },
      NULL,
      E42_FILES },
    { "E48",
      d_tests_arg_config_e48,
      { "sub", "--x", "9", "--config", "E48.cfg" },
      NULL,
      E48_FILES }
};

// FILE_COMMAND_COUNT
//   constant: how many cases FILE_COMMAND_CASES holds.
static const size_t FILE_COMMAND_COUNT =
    sizeof(FILE_COMMAND_CASES) / sizeof(FILE_COMMAND_CASES[0]);

/*
d_tests_arg_config_file_commands
  Sections and dotted names for subcommands and option groups that are not
configurable.
  Tests the following:
  - a section for a subcommand, and one inside it; dotted names
  - INI, option groups, a subcommand falling through to its parent
  - the default section, a subcommand that is not configurable, a repeated
    section
  - a group inside a subcommand, a format reading one section, and a subcommand
    the command line used
*/
bool
d_tests_arg_config_file_commands(void)
{
    return d_tests_arg_cases("FILE_COMMANDS",
                             FILE_COMMAND_CASES,
                             FILE_COMMAND_COUNT,
                             FILE_COMMANDS);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 27. NAMES IN FILES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief E60: a section and a dotted name by a subcommand's alias.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e60(
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

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    (void)d_arg_command_add_alias(sub,
                                  "s");
    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");

    return;
}

/**
 * @brief E61: a section for a subcommand that ignores case.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e61(
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

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    (void)d_arg_command_set_ignore_case(sub,
                                        true);
    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");

    return;
}

/**
 * @brief E62: a section for a subcommand inside an option group.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e62(
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

    struct d_arg_command* const grp   = d_tests_arg_make_group(_run,
                                                               _app,
                                                               "grp");
    struct d_arg_command* const insub = d_tests_arg_make_subcommand(_run,
                                                                    grp,
                                                                    "insub");

    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  insub,
                                  "--x");

    return;
}

/**
 * @brief E63: a nameless subcommand's option.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e63(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    struct d_arg_command* const nameless =
        d_arg_command_add_subcommand(_app,
                                     "",
                                     "nameless");

    (void)d_tests_arg_watch_command(_run,
                                    "nameless",
                                    nameless);
    (void)d_tests_arg_make_option(_run,
                                  "b",
                                  nameless,
                                  "--b");

    return;
}

/**
 * @brief E64: a subcommand's own configuration option.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e64(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const sub    = d_tests_arg_make_subcommand(_run,
                                                                     _app,
                                                                     "sub");
    struct d_arg_option* const  config = d_arg_command_set_config(sub,
                                                                  "--config",
                                                                  NULL,
                                                                  NULL,
                                                                  false);

    (void)d_tests_arg_watch_option(_run,
                                   "config",
                                   config);
    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");
    (void)d_tests_arg_make_option(_run,
                                  "rx",
                                  _app,
                                  "--x");

    return;
}

// E60_FILES
//   constant: the files E60 parses with.
static const struct d_tests_arg_file E60_FILES[] =
{
    { "E60.cfg", "[s]\nx=1\ns.x=2\n" },
    { NULL,      NULL }
};

// E61_FILES
//   constant: the files E61 parses with.
static const struct d_tests_arg_file E61_FILES[] =
{
    { "E61.cfg", "[SUB]\nx=1\n" },
    { NULL,      NULL }
};

// E62_FILES
//   constant: the files E62 parses with.
static const struct d_tests_arg_file E62_FILES[] =
{
    { "E62.cfg", "[insub]\nx=1\n" },
    { NULL,      NULL }
};

// E63_FILES
//   constant: the files E63 parses with.
static const struct d_tests_arg_file E63_FILES[] =
{
    { "E63.cfg", "a=1\nb=2\n" },
    { NULL,      NULL }
};

// E64_FILES
//   constant: the files E64 parses with.
static const struct d_tests_arg_file E64_FILES[] =
{
    { "E64.cfg", "x=3\n" },
    { NULL,      NULL }
};

// FILE_NAME_CASES
//   constant: the cases of d_tests_arg_config_file_names.
static const struct d_tests_arg_case FILE_NAME_CASES[] =
{
    { "E60",
      d_tests_arg_config_e60,
      { "--config", "E60.cfg" },
      NULL,
      E60_FILES },
    { "E61",
      d_tests_arg_config_e61,
      { "--config", "E61.cfg" },
      NULL,
      E61_FILES },
    { "E62",
      d_tests_arg_config_e62,
      { "--config", "E62.cfg" },
      NULL,
      E62_FILES },
    { "E63",
      d_tests_arg_config_e63,
      { "--config", "E63.cfg" },
      NULL,
      E63_FILES },
    { "E64",
      d_tests_arg_config_e64,
      { "sub", "--config", "E64.cfg" },
      NULL,
      E64_FILES }
};

// FILE_NAME_COUNT
//   constant: how many cases FILE_NAME_CASES holds.
static const size_t FILE_NAME_COUNT =
    sizeof(FILE_NAME_CASES) / sizeof(FILE_NAME_CASES[0]);

/*
d_tests_arg_config_file_names
  The names a section finds a command by, and commands reached through others.
  Tests the following:
  - a subcommand's alias, and its name in another case
  - a subcommand inside an option group, and a nameless subcommand's option
  - a configuration option of a subcommand's, which the root does not read
*/
bool
d_tests_arg_config_file_names(void)
{
    return d_tests_arg_cases("FILE_NAMES",
                             FILE_NAME_CASES,
                             FILE_NAME_COUNT,
                             FILE_NAMES);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 28. CONFIGURABLE SUBCOMMANDS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief E35: an option's callback, and a subcommand's pre-parse and final
 *        callbacks.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e35(
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

    struct d_arg_option* const a =
        d_arg_command_add_option_callback(_app,
                                          "--a",
                                          d_tests_arg_config_value,
                                          _run,
                                          NULL,
                                          NULL);

    (void)d_tests_arg_watch_option(_run,
                                   "a",
                                   a);

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");
    d_arg_command_set_final_callback(sub,
                                     d_tests_arg_config_final,
                                     _run,
                                     NULL);
    d_arg_command_set_preparse_callback(sub,
                                        d_tests_arg_config_preparse,
                                        _run,
                                        NULL);

    return;
}

/**
 * @brief E51: a configurable subcommand's section, with callbacks.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e51(
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

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_configurable(sub,
                                   true);
    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");
    d_arg_command_set_preparse_callback(sub,
                                        d_tests_arg_config_preparse,
                                        _run,
                                        NULL);
    d_arg_command_set_final_callback(sub,
                                     d_tests_arg_config_final,
                                     _run,
                                     NULL);

    return;
}

/**
 * @brief E52: configurable subcommands, the deeper one's section first.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e52(
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

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_configurable(sub,
                                   true);

    struct d_arg_command* const deep = d_tests_arg_make_subcommand(_run,
                                                                   sub,
                                                                   "deep");

    d_arg_command_set_configurable(deep,
                                   true);
    (void)d_tests_arg_make_option(_run,
                                  "y",
                                  deep,
                                  "--y");
    d_arg_command_set_final_callback(deep,
                                     d_tests_arg_config_final,
                                     _run,
                                     NULL);
    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");

    return;
}

/**
 * @brief E53: a parse-complete callback, and the default section after.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e53(
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

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_configurable(sub,
                                   true);
    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");
    d_arg_command_set_parse_complete_callback(sub,
                                              d_tests_arg_config_complete,
                                              _run,
                                              NULL);
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    return;
}

/**
 * @brief E54: a configurable subcommand's section repeated.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e54(
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

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_configurable(sub,
                                   true);
    (void)d_tests_arg_make_all(_run,
                               "x",
                               sub,
                               "--x");

    return;
}

/**
 * @brief E55: two configurable subcommands, one required.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e55(
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

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_configurable(sub,
                                   true);
    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");

    struct d_arg_command* const other = d_tests_arg_make_subcommand(_run,
                                                                    _app,
                                                                    "other");

    d_arg_command_set_configurable(other,
                                   true);
    (void)d_tests_arg_make_option(_run,
                                  "y",
                                  other,
                                  "--y");
    d_arg_command_require_subcommand_count(_app,
                                           1);

    return;
}

/**
 * @brief E56: an immediate callback for a section.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e56(
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

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_configurable(sub,
                                   true);
    d_arg_command_set_immediate_callback(sub,
                                         true);
    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");
    d_arg_command_set_callback(sub,
                               d_tests_arg_config_final,
                               _run,
                               NULL);

    return;
}

// E35_FILES
//   constant: the files E35 parses with.
static const struct d_tests_arg_file E35_FILES[] =
{
    { "E35.cfg", "a=1\n[sub]\nx=2\n" },
    { NULL,      NULL }
};

// E51_FILES
//   constant: the files E51 parses with.
static const struct d_tests_arg_file E51_FILES[] =
{
    { "E51.cfg", "[sub]\nx=1\n" },
    { NULL,      NULL }
};

// E52_FILES
//   constant: the files E52 parses with.
static const struct d_tests_arg_file E52_FILES[] =
{
    { "E52.cfg", "[sub.deep]\ny=2\n[sub]\nx=1\n" },
    { NULL,      NULL }
};

// E53_FILES
//   constant: the files E53 parses with.
static const struct d_tests_arg_file E53_FILES[] =
{
    { "E53.cfg", "[sub]\nx=1\n[default]\na=2\n" },
    { NULL,      NULL }
};

// E54_FILES
//   constant: the files E54 parses with.
static const struct d_tests_arg_file E54_FILES[] =
{
    { "E54.cfg", "[[sub]]\nx=1\n[[sub]]\nx=2\n" },
    { NULL,      NULL }
};

// E55_FILES
//   constant: the files E55 parses with.
static const struct d_tests_arg_file E55_FILES[] =
{
    { "E55.cfg", "[sub]\nx=1\n[other]\ny=2\n" },
    { NULL,      NULL }
};

// E56_FILES
//   constant: the files E56 parses with.
static const struct d_tests_arg_file E56_FILES[] =
{
    { "E56.cfg", "[sub]\nx=1\n" },
    { NULL,      NULL }
};

// FILE_SUBCOMMAND_CASES
//   constant: the cases of d_tests_arg_config_file_subcommands.
static const struct d_tests_arg_case FILE_SUBCOMMAND_CASES[] =
{
    { "E35",
      d_tests_arg_config_e35,
      { "--config", "E35.cfg" },
      NULL,
      E35_FILES },
    { "E51",
      d_tests_arg_config_e51,
      { "--config", "E51.cfg" },
      NULL,
      E51_FILES },
    { "E52",
      d_tests_arg_config_e52,
      { "--config", "E52.cfg" },
      NULL,
      E52_FILES },
    { "E53",
      d_tests_arg_config_e53,
      { "--config", "E53.cfg" },
      NULL,
      E53_FILES },
    { "E54",
      d_tests_arg_config_e54,
      { "--config", "E54.cfg" },
      NULL,
      E54_FILES },
    { "E55",
      d_tests_arg_config_e55,
      { "--config", "E55.cfg" },
      NULL,
      E55_FILES },
    { "E56",
      d_tests_arg_config_e56,
      { "--config", "E56.cfg" },
      NULL,
      E56_FILES }
};

// FILE_SUBCOMMAND_COUNT
//   constant: how many cases FILE_SUBCOMMAND_CASES holds.
static const size_t FILE_SUBCOMMAND_COUNT =
    sizeof(FILE_SUBCOMMAND_CASES) / sizeof(FILE_SUBCOMMAND_CASES[0]);

/*
d_tests_arg_config_file_subcommands
  Subcommands a file's section uses, and the callbacks a file's values run.
  Tests the following:
  - an option's callback, and a subcommand's, for values from a file
  - a configurable subcommand's section, nested, and repeated
  - a parse-complete callback, and an immediate one
  - two configurable subcommands where one is required
*/
bool
d_tests_arg_config_file_subcommands(void)
{
    return d_tests_arg_cases("FILE_SUBCOMMANDS",
                             FILE_SUBCOMMAND_CASES,
                             FILE_SUBCOMMAND_COUNT,
                             FILE_SUBCOMMANDS);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 29. EXTRAS IN FILES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief E08: an option that is not configurable.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e08(
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

    struct d_arg_option* const a = d_tests_arg_make_option(_run,
                                                           "a",
                                                           _app,
                                                           "--a");

    d_arg_option_set_configurable(a,
                                  false);

    return;
}

/**
 * @brief E09: names no option takes, ignored.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e09(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    return;
}

/**
 * @brief E10: a name no option takes, refused.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e10(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");
    d_arg_command_set_allow_config_extras(_app,
                                          D_ARG_CONFIG_EXTRAS_ERROR);

    return;
}

/**
 * @brief E11: names no option takes, captured.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e11(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");
    d_arg_command_set_allow_config_extras(_app,
                                          D_ARG_CONFIG_EXTRAS_CAPTURE);

    return;
}

/**
 * @brief E12: an option that is not configurable, under ignore_all.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e12(
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

    struct d_arg_option* const a = d_tests_arg_make_option(_run,
                                                           "a",
                                                           _app,
                                                           "--a");

    d_arg_option_set_configurable(a,
                                  false);
    d_arg_command_set_allow_config_extras(_app,
                                          D_ARG_CONFIG_EXTRAS_IGNORE_ALL);

    return;
}

/**
 * @brief E15: a required option the file leaves out.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e15(
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

    struct d_arg_option* const a = d_tests_arg_make_option(_run,
                                                           "a",
                                                           _app,
                                                           "--a");

    d_arg_option_set_required(a,
                              true);

    struct d_arg_option* const b = d_tests_arg_make_option(_run,
                                                           "b",
                                                           _app,
                                                           "--b");

    d_arg_option_set_required(b,
                              true);

    return;
}

/**
 * @brief E45: a name a subcommand's section has no option for, refused.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e45(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");
    d_arg_command_set_allow_config_extras(_app,
                                          D_ARG_CONFIG_EXTRAS_ERROR);

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
 * @brief E65: an option that is not configurable, under ignore.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e65(
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

    struct d_arg_option* const a = d_tests_arg_make_option(_run,
                                                           "a",
                                                           _app,
                                                           "--a");

    d_arg_option_set_configurable(a,
                                  false);
    d_arg_command_set_allow_config_extras(_app,
                                          D_ARG_CONFIG_EXTRAS_IGNORE);

    return;
}

// E08_FILES
//   constant: the files E08 parses with.
static const struct d_tests_arg_file E08_FILES[] =
{
    { "E08.cfg", "a=1\n" },
    { NULL,      NULL }
};

// E09_FILES
//   constant: the files E09 parses with.
static const struct d_tests_arg_file E09_FILES[] =
{
    { "E09.cfg", "a=1\nunknown=2\n[nosub]\nq=3\n" },
    { NULL,      NULL }
};

// E10_FILES
//   constant: the files E10 parses with.
static const struct d_tests_arg_file E10_FILES[] =
{
    { "E10.cfg", "a=1\nunknown=2\n" },
    { NULL,      NULL }
};

// E11_FILES
//   constant: the files E11 parses with.
static const struct d_tests_arg_file E11_FILES[] =
{
    { "E11.cfg", "a=1\nunknown=2\nlist=[3,4]\n[sec]\nq=5\n" },
    { NULL,      NULL }
};

// E12_FILES
//   constant: the files E12 parses with.
static const struct d_tests_arg_file E12_FILES[] =
{
    { "E12.cfg", "a=1\nunknown=2\n" },
    { NULL,      NULL }
};

// E15_FILES
//   constant: the files E15 parses with.
static const struct d_tests_arg_file E15_FILES[] =
{
    { "E15.cfg", "a=1\n" },
    { NULL,      NULL }
};

// E45_FILES
//   constant: the files E45 parses with.
static const struct d_tests_arg_file E45_FILES[] =
{
    { "E45.cfg", "[sub]\nx=1\ny=2\n" },
    { NULL,      NULL }
};

// E65_FILES
//   constant: the files E65 parses with.
static const struct d_tests_arg_file E65_FILES[] =
{
    { "E65.cfg", "unknown=1\na=2\n" },
    { NULL,      NULL }
};

// FILE_EXTRA_CASES
//   constant: the cases of d_tests_arg_config_file_extras.
static const struct d_tests_arg_case FILE_EXTRA_CASES[] =
{
    { "E08",
      d_tests_arg_config_e08,
      { "--config", "E08.cfg" },
      NULL,
      E08_FILES },
    { "E09",
      d_tests_arg_config_e09,
      { "--config", "E09.cfg" },
      NULL,
      E09_FILES },
    { "E10",
      d_tests_arg_config_e10,
      { "--config", "E10.cfg" },
      NULL,
      E10_FILES },
    { "E11",
      d_tests_arg_config_e11,
      { "--config", "E11.cfg" },
      NULL,
      E11_FILES },
    { "E12",
      d_tests_arg_config_e12,
      { "--config", "E12.cfg" },
      NULL,
      E12_FILES },
    { "E15",
      d_tests_arg_config_e15,
      { "--config", "E15.cfg" },
      NULL,
      E15_FILES },
    { "E45",
      d_tests_arg_config_e45,
      { "--config", "E45.cfg" },
      NULL,
      E45_FILES },
    { "E65",
      d_tests_arg_config_e65,
      { "--config", "E65.cfg" },
      NULL,
      E65_FILES }
};

// FILE_EXTRA_COUNT
//   constant: how many cases FILE_EXTRA_CASES holds.
static const size_t FILE_EXTRA_COUNT =
    sizeof(FILE_EXTRA_CASES) / sizeof(FILE_EXTRA_CASES[0]);

/*
d_tests_arg_config_file_extras
  Names no option takes, and an option that is not configurable, under each mode
for configuration extras.
  Tests the following:
  - an option that is not configurable, refused, and ignored only by ignore_all
  - names no option takes: ignored, refused, captured
  - a required option a file leaves out
  - a name no option of a subcommand's section takes
*/
bool
d_tests_arg_config_file_extras(void)
{
    return d_tests_arg_cases("FILE_EXTRAS",
                             FILE_EXTRA_CASES,
                             FILE_EXTRA_COUNT,
                             FILE_EXTRAS);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 30. WHERE FILES COME FROM
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief E17: a default file.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e17(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const config = d_arg_command_set_config(_app,
                                                                 "--config",
                                                                 "E17.cfg",
                                                                 NULL,
                                                                 false);

    (void)d_tests_arg_watch_option(_run,
                                   "config",
                                   config);
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    return;
}

/**
 * @brief E41: a format allowing a name given twice.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e41(
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
    (void)d_tests_arg_make_all(_run,
                               "a",
                               _app,
                               "--a");
    (void)d_tests_arg_config_kept(_run,
                                  _app,
                                  false,
                                  D_ARG_CONFIG_ALLOW_DUPLICATE_FIELDS);

    return;
}

/**
 * @brief E47: a required configuration option given its file.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e47(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const config = d_arg_command_set_config(_app,
                                                                 "--config",
                                                                 "",
                                                                 "",
                                                                 true);

    (void)d_tests_arg_watch_option(_run,
                                   "config",
                                   config);
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    return;
}

/**
 * @brief E66: a file, then one that is not there.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e66(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    return;
}

/**
 * @brief E67: a default file and another given in its place.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e67(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const config = d_arg_command_set_config(_app,
                                                                 "--config",
                                                                 "E67a.cfg",
                                                                 NULL,
                                                                 false);

    (void)d_tests_arg_watch_option(_run,
                                   "config",
                                   config);
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");
    (void)d_tests_arg_make_option(_run,
                                  "b",
                                  _app,
                                  "--b");

    return;
}

// E17_FILES
//   constant: the files E17 parses with.
static const struct d_tests_arg_file E17_FILES[] =
{
    { "E17.cfg", "a=7\n" },
    { NULL,      NULL }
};

// E41_FILES
//   constant: the files E41 parses with.
static const struct d_tests_arg_file E41_FILES[] =
{
    { "E41.cfg", "a=1\nb=0\na=2\n" },
    { NULL,      NULL }
};

// E47_FILES
//   constant: the files E47 parses with.
static const struct d_tests_arg_file E47_FILES[] =
{
    { "E47.cfg", "a=3\n" },
    { NULL,      NULL }
};

// E66_FILES
//   constant: the files E66 parses with.
static const struct d_tests_arg_file E66_FILES[] =
{
    { "E66.cfg", "a=1\n" },
    { NULL,      NULL }
};

// E67_FILES
//   constant: the files E67 parses with.
static const struct d_tests_arg_file E67_FILES[] =
{
    { "E67a.cfg", "a=1\n" },
    { "E67b.cfg", "b=2\n" },
    { NULL,       NULL }
};

// FILE_SOURCE_CASES
//   constant: the cases of d_tests_arg_config_file_sources.
static const struct d_tests_arg_case FILE_SOURCE_CASES[] =
{
    { "E17",
      d_tests_arg_config_e17,
      { NULL },
      NULL,
      E17_FILES },
    { "E41",
      d_tests_arg_config_e41,
      { "--config", "E41.cfg" },
      NULL,
      E41_FILES },
    { "E47",
      d_tests_arg_config_e47,
      { "--config", "E47.cfg" },
      NULL,
      E47_FILES },
    { "E66",
      d_tests_arg_config_e66,
      { "--config", "E66.cfg", "--config", "E66x.cfg" },
      NULL,
      E66_FILES },
    { "E67",
      d_tests_arg_config_e67,
      { "--config", "E67b.cfg" },
      NULL,
      E67_FILES }
};

// FILE_SOURCE_COUNT
//   constant: how many cases FILE_SOURCE_CASES holds.
static const size_t FILE_SOURCE_COUNT =
    sizeof(FILE_SOURCE_CASES) / sizeof(FILE_SOURCE_CASES[0]);

/*
d_tests_arg_config_file_sources
  Files from a default, a required option, the option given twice, and a format
of the caller's.
  Tests the following:
  - a default file, and one given in its place
  - a format allowing duplicates
  - a required configuration option given its file
  - a second file that is not there
*/
bool
d_tests_arg_config_file_sources(void)
{
    return d_tests_arg_cases("FILE_SOURCES",
                             FILE_SOURCE_CASES,
                             FILE_SOURCE_COUNT,
                             FILE_SOURCES);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 31. FILES FROM THE ENVIRONMENT
//==============================================================================

#if ( (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1) &&                             \
      (D_INTERNAL_ARG_PARSER_ENVIRONMENT == 1) )

/**
 * @brief E16: a file the configuration option's variable names.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e16(
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
                                    "DJ_CFG_E16");
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    return;
}

/**
 * @brief E68: a file given in place of the one the variable names.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e68(
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
                                    "DJ_CFG_E68");
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");
    (void)d_tests_arg_make_option(_run,
                                  "b",
                                  _app,
                                  "--b");

    return;
}

/**
 * @brief E58: an option's variable, and a file giving it a value.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e58(
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

    struct d_arg_option* const a = d_tests_arg_make_option(_run,
                                                           "a",
                                                           _app,
                                                           "--a");

    (void)d_arg_option_set_env_name(a,
                                    "DJ_CFG_E58");

    return;
}

// E16_VARIABLES
//   constant: the environment variables E16 parses with.
static const struct d_tests_arg_env_var E16_VARIABLES[] =
{
    { "DJ_CFG_E16", "E16.cfg" },
    { NULL,         NULL }
};

// E16_FILES
//   constant: the files E16 parses with.
static const struct d_tests_arg_file E16_FILES[] =
{
    { "E16.cfg", "a=6\n" },
    { NULL,      NULL }
};

// E68_VARIABLES
//   constant: the environment variables E68 parses with.
static const struct d_tests_arg_env_var E68_VARIABLES[] =
{
    { "DJ_CFG_E68", "E68a.cfg" },
    { NULL,         NULL }
};

// E68_FILES
//   constant: the files E68 parses with.
static const struct d_tests_arg_file E68_FILES[] =
{
    { "E68a.cfg", "a=1\n" },
    { "E68b.cfg", "b=2\n" },
    { NULL,       NULL }
};

// E58_VARIABLES
//   constant: the environment variables E58 parses with.
static const struct d_tests_arg_env_var E58_VARIABLES[] =
{
    { "DJ_CFG_E58", "5" },
    { NULL,         NULL }
};

// E58_FILES
//   constant: the files E58 parses with.
static const struct d_tests_arg_file E58_FILES[] =
{
    { "E58.cfg", "a=1\n" },
    { NULL,      NULL }
};

// FILE_ENVIRONMENT_CASES
//   constant: the cases of d_tests_arg_config_file_environment.
static const struct d_tests_arg_case FILE_ENVIRONMENT_CASES[] =
{
    { "E16",
      d_tests_arg_config_e16,
      { NULL },
      E16_VARIABLES,
      E16_FILES },
    { "E68",
      d_tests_arg_config_e68,
      { "--config", "E68b.cfg" },
      E68_VARIABLES,
      E68_FILES },
    { "E58",
      d_tests_arg_config_e58,
      { "--config", "E58.cfg" },
      E58_VARIABLES,
      E58_FILES }
};

// FILE_ENVIRONMENT_COUNT
//   constant: how many cases FILE_ENVIRONMENT_CASES holds.
static const size_t FILE_ENVIRONMENT_COUNT =
    sizeof(FILE_ENVIRONMENT_CASES) / sizeof(FILE_ENVIRONMENT_CASES[0]);

/*
d_tests_arg_config_file_environment
  A file an environment variable names, and an option's variable against a file.
  Tests the following:
  - a file the configuration option's variable names
  - a file given in place of the one the variable names
  - an option's variable, which a file's value comes before
*/
bool
d_tests_arg_config_file_environment(void)
{
    return d_tests_arg_cases("FILE_ENVIRONMENT",
                             FILE_ENVIRONMENT_CASES,
                             FILE_ENVIRONMENT_COUNT,
                             FILE_ENVIRONMENT);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES, D_INTERNAL_ARG_PARSER_ENVIRONMENT

//==============================================================================
// 32. FILES FOR FLAG-LIKE OPTIONS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief E74: nothing and empty values for options taking at most one value
 *        or two.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e74(
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

    struct d_arg_option* const o = d_tests_arg_make_option(_run,
                                                           "o",
                                                           _app,
                                                           "--o");

    d_arg_option_set_expected_range(o,
                                    0,
                                    1);

    struct d_arg_option* const p = d_tests_arg_make_option(_run,
                                                           "p",
                                                           _app,
                                                           "--p");

    d_arg_option_set_expected_range(p,
                                    0,
                                    2);

    struct d_arg_option* const q = d_tests_arg_make_option(_run,
                                                           "q",
                                                           _app,
                                                           "--q");

    d_arg_option_set_expected_range(q,
                                    0,
                                    2);
    (void)d_arg_option_set_default_text(q,
                                        "d");

    struct d_arg_option* const r = d_tests_arg_make_option(_run,
                                                           "r",
                                                           _app,
                                                           "--r");

    d_arg_option_set_expected_range(r,
                                    0,
                                    1);
    (void)d_arg_option_set_default_text(r,
                                        "e");

    return;
}

/**
 * @brief E75: flags refusing an override, given lists of their values.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e75(
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

    struct d_arg_option* const f = d_tests_arg_make_flag(_run,
                                                         "f",
                                                         _app,
                                                         "--f");

    d_arg_option_set_disable_flag_override(f,
                                           true);

    struct d_arg_option* const g = d_tests_arg_make_flag(_run,
                                                         "g",
                                                         _app,
                                                         "--g{1},--h{2}");

    d_arg_option_set_disable_flag_override(g,
                                           true);

    return;
}

/**
 * @brief E76: a flag refusing an override, given a value none of its names
 *        gives.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e76(
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

    struct d_arg_option* const g = d_tests_arg_make_flag(_run,
                                                         "g",
                                                         _app,
                                                         "--g{1},--h{2}");

    d_arg_option_set_disable_flag_override(g,
                                           true);

    return;
}

/**
 * @brief E77: an option taking at most one value, given two.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e77(
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

    struct d_arg_option* const a = d_tests_arg_make_option(_run,
                                                           "a",
                                                           _app,
                                                           "--a");

    d_arg_option_set_expected_range(a,
                                    0,
                                    1);

    return;
}

/**
 * @brief E78: an option taking at most two values, given three.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e78(
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

    struct d_arg_option* const b = d_tests_arg_make_option(_run,
                                                           "b",
                                                           _app,
                                                           "--b");

    d_arg_option_set_expected_range(b,
                                    0,
                                    2);

    return;
}

/**
 * @brief E79: an option taking at most two values, given two.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e79(
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

    struct d_arg_option* const b = d_tests_arg_make_option(_run,
                                                           "b",
                                                           _app,
                                                           "--b");

    d_arg_option_set_expected_range(b,
                                    0,
                                    2);

    return;
}

/**
 * @brief E80: an option refusing an override and taking at most one value,
 *        given two.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e80(
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

    struct d_arg_option* const c = d_tests_arg_make_option(_run,
                                                           "c",
                                                           _app,
                                                           "--c");

    d_arg_option_set_expected_range(c,
                                    0,
                                    1);
    d_arg_option_set_disable_flag_override(c,
                                           true);

    return;
}

/**
 * @brief E81: an option refusing an override and taking at most two values,
 *        given three.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e81(
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

    struct d_arg_option* const d = d_tests_arg_make_option(_run,
                                                           "d",
                                                           _app,
                                                           "--d");

    d_arg_option_set_expected_range(d,
                                    0,
                                    2);
    d_arg_option_set_disable_flag_override(d,
                                           true);

    return;
}

// E74_FILES
//   constant: the files E74 parses with.
static const struct d_tests_arg_file E74_FILES[] =
{
    { "E74.cfg", "o = []\np = []\nq =\nr =\n" },
    { NULL,      NULL }
};

// E75_FILES
//   constant: the files E75 parses with.
static const struct d_tests_arg_file E75_FILES[] =
{
    { "E75.cfg", "f = [true, false]\ng = [1, 2]\n" },
    { NULL,      NULL }
};

// E76_FILES
//   constant: the files E76 parses with.
static const struct d_tests_arg_file E76_FILES[] =
{
    { "E76.cfg", "g = [1, 3]\n" },
    { NULL,      NULL }
};

// E77_FILES
//   constant: the files E77 parses with.
static const struct d_tests_arg_file E77_FILES[] =
{
    { "E77.cfg", "a = [1, 2]\n" },
    { NULL,      NULL }
};

// E78_FILES
//   constant: the files E78 parses with.
static const struct d_tests_arg_file E78_FILES[] =
{
    { "E78.cfg", "b = [1, 2, 3]\n" },
    { NULL,      NULL }
};

// E79_FILES
//   constant: the files E79 parses with.
static const struct d_tests_arg_file E79_FILES[] =
{
    { "E79.cfg", "b = [1, 2]\n" },
    { NULL,      NULL }
};

// E80_FILES
//   constant: the files E80 parses with.
static const struct d_tests_arg_file E80_FILES[] =
{
    { "E80.cfg", "c = [true, 1]\n" },
    { NULL,      NULL }
};

// E81_FILES
//   constant: the files E81 parses with.
static const struct d_tests_arg_file E81_FILES[] =
{
    { "E81.cfg", "d = [1, 2, 3]\n" },
    { NULL,      NULL }
};

// FILE_FLAG_CASES
//   constant: the cases of d_tests_arg_config_file_flags.
static const struct d_tests_arg_case FILE_FLAG_CASES[] =
{
    { "E74",
      d_tests_arg_config_e74,
      { "--config", "E74.cfg" },
      NULL,
      E74_FILES },
    { "E75",
      d_tests_arg_config_e75,
      { "--config", "E75.cfg" },
      NULL,
      E75_FILES },
    { "E76",
      d_tests_arg_config_e76,
      { "--config", "E76.cfg" },
      NULL,
      E76_FILES },
    { "E77",
      d_tests_arg_config_e77,
      { "--config", "E77.cfg" },
      NULL,
      E77_FILES },
    { "E78",
      d_tests_arg_config_e78,
      { "--config", "E78.cfg" },
      NULL,
      E78_FILES },
    { "E79",
      d_tests_arg_config_e79,
      { "--config", "E79.cfg" },
      NULL,
      E79_FILES },
    { "E80",
      d_tests_arg_config_e80,
      { "--config", "E80.cfg" },
      NULL,
      E80_FILES },
    { "E81",
      d_tests_arg_config_e81,
      { "--config", "E81.cfg" },
      NULL,
      E81_FILES }
};

// FILE_FLAG_COUNT
//   constant: how many cases FILE_FLAG_CASES holds.
static const size_t FILE_FLAG_COUNT =
    sizeof(FILE_FLAG_CASES) / sizeof(FILE_FLAG_CASES[0]);

/*
d_tests_arg_config_file_flags
  Items given to options that take no value or a few, as CLI11's
_add_flag_like_result takes them.
  Tests the following:
  - an empty array and an empty value for options taking at most one value or
    two, with defaults and without
  - flags refusing an override, given lists of their values, and a value none of
    their names gives
  - lists past what an option takes: a flag's error and an option's
  - a list within what an option takes
  - options refusing an override, given lists within and past what they take
*/
bool
d_tests_arg_config_file_flags(void)
{
    return d_tests_arg_cases("FILE_FLAGS",
                             FILE_FLAG_CASES,
                             FILE_FLAG_COUNT,
                             FILE_FLAGS);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 33. OPTIONS LOOKED FOR
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief E82: a short option that is not configurable.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e82(
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
                                                           "-x");

    d_arg_option_set_configurable(x,
                                  false);

    return;
}

/**
 * @brief E83: a positional that is not configurable.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e83(
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

    struct d_arg_option* const pos = d_tests_arg_make_option(_run,
                                                             "pos",
                                                             _app,
                                                             "pos");

    d_arg_option_set_configurable(pos,
                                  false);

    return;
}

/**
 * @brief E84: a positional in an option group, past a long option that is
 *        not configurable.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e84(
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

    struct d_arg_option* const name = d_tests_arg_make_option(_run,
                                                              "name",
                                                              _app,
                                                              "--name");

    d_arg_option_set_configurable(name,
                                  false);

    struct d_arg_command* const grp = d_tests_arg_make_group(_run,
                                                             _app,
                                                             "grp");

    (void)d_tests_arg_make_option(_run,
                                  "pname",
                                  grp,
                                  "name");

    return;
}

/**
 * @brief E85: an option of a subcommand grouped with `+`.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e85(
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

    struct d_arg_command* const s = d_tests_arg_make_subcommand(_run,
                                                                _app,
                                                                "s");

    (void)d_arg_command_set_group(s,
                                  "+g");
    (void)d_tests_arg_make_option(_run,
                                  "z",
                                  s,
                                  "--z");

    return;
}

/**
 * @brief E86: a fallthrough parent's positional, past an option that is not
 *        configurable.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e86(
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
    (void)d_tests_arg_make_option(_run,
                                  "ra",
                                  _app,
                                  "a");

    struct d_arg_command* const s = d_tests_arg_make_subcommand(_run,
                                                                _app,
                                                                "s");

    d_arg_command_set_fallthrough(s,
                                  true);

    struct d_arg_option* const sa = d_tests_arg_make_option(_run,
                                                            "sa",
                                                            s,
                                                            "--a");

    d_arg_option_set_configurable(sa,
                                  false);

    return;
}

/**
 * @brief E87: the same without fallthrough.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e87(
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
    (void)d_tests_arg_make_option(_run,
                                  "ra",
                                  _app,
                                  "a");

    struct d_arg_command* const s  = d_tests_arg_make_subcommand(_run,
                                                                 _app,
                                                                 "s");
    struct d_arg_option* const  sa = d_tests_arg_make_option(_run,
                                                             "sa",
                                                             s,
                                                             "--a");

    d_arg_option_set_configurable(sa,
                                  false);

    return;
}

/**
 * @brief E100: a fallthrough parent's short option, past a long one that is not
 *        configurable and before a positional in an option group.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e100(
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

    struct d_arg_command* const s = d_tests_arg_make_subcommand(_run,
                                                                _app,
                                                                "s");

    d_arg_command_set_fallthrough(s,
                                  true);

    struct d_arg_command* const g = d_tests_arg_make_group(_run,
                                                           s,
                                                           "g");

    (void)d_tests_arg_make_option(_run,
                                  "px",
                                  g,
                                  "x");

    struct d_arg_option* const lx = d_tests_arg_make_option(_run,
                                                            "lx",
                                                            s,
                                                            "--x");

    d_arg_option_set_configurable(lx,
                                  false);
    (void)d_tests_arg_make_option(_run,
                                  "rx",
                                  _app,
                                  "-x");

    return;
}

/**
 * @brief E101: a long option in an option group, before a short one.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e101(
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

    struct d_arg_command* const g = d_tests_arg_make_group(_run,
                                                           _app,
                                                           "g");

    (void)d_tests_arg_make_option(_run,
                                  "lx",
                                  g,
                                  "--x");
    (void)d_tests_arg_make_option(_run,
                                  "sx",
                                  _app,
                                  "-x");

    return;
}

/**
 * @brief E102: a section for a disabled subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e102(
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

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_disabled(sub,
                               true);
    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");

    return;
}

/**
 * @brief E103: a section no subcommand has, refused.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e103(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");
    d_arg_command_set_allow_config_extras(_app,
                                          D_ARG_CONFIG_EXTRAS_ERROR);

    return;
}

// E82_FILES
//   constant: the files E82 parses with.
static const struct d_tests_arg_file E82_FILES[] =
{
    { "E82.cfg", "x=1\n" },
    { NULL,      NULL }
};

// E83_FILES
//   constant: the files E83 parses with.
static const struct d_tests_arg_file E83_FILES[] =
{
    { "E83.cfg", "pos=1\n" },
    { NULL,      NULL }
};

// E84_FILES
//   constant: the files E84 parses with.
static const struct d_tests_arg_file E84_FILES[] =
{
    { "E84.cfg", "name=1\n" },
    { NULL,      NULL }
};

// E85_FILES
//   constant: the files E85 parses with.
static const struct d_tests_arg_file E85_FILES[] =
{
    { "E85.cfg", "z=1\n" },
    { NULL,      NULL }
};

// E86_FILES
//   constant: the files E86 parses with.
static const struct d_tests_arg_file E86_FILES[] =
{
    { "E86.cfg", "[s]\na=5\n" },
    { NULL,      NULL }
};

// E87_FILES
//   constant: the files E87 parses with.
static const struct d_tests_arg_file E87_FILES[] =
{
    { "E87.cfg", "[s]\na=5\n" },
    { NULL,      NULL }
};

// E100_FILES
//   constant: the files E100 parses with.
static const struct d_tests_arg_file E100_FILES[] =
{
    { "E100.cfg", "[s]\nx=1\n" },
    { NULL,       NULL }
};

// E101_FILES
//   constant: the files E101 parses with.
static const struct d_tests_arg_file E101_FILES[] =
{
    { "E101.cfg", "x=1\n" },
    { NULL,       NULL }
};

// E102_FILES
//   constant: the files E102 parses with.
static const struct d_tests_arg_file E102_FILES[] =
{
    { "E102.cfg", "[sub]\nx=1\n" },
    { NULL,       NULL }
};

// E103_FILES
//   constant: the files E103 parses with.
static const struct d_tests_arg_file E103_FILES[] =
{
    { "E103.cfg", "a=1\n[nosub]\nq=3\n" },
    { NULL,       NULL }
};

// FILE_LOOKUP_CASES
//   constant: the cases of d_tests_arg_config_file_lookups.
static const struct d_tests_arg_case FILE_LOOKUP_CASES[] =
{
    { "E82",
      d_tests_arg_config_e82,
      { "--config", "E82.cfg" },
      NULL,
      E82_FILES },
    { "E83",
      d_tests_arg_config_e83,
      { "--config", "E83.cfg" },
      NULL,
      E83_FILES },
    { "E84",
      d_tests_arg_config_e84,
      { "--config", "E84.cfg" },
      NULL,
      E84_FILES },
    { "E85",
      d_tests_arg_config_e85,
      { "--config", "E85.cfg" },
      NULL,
      E85_FILES },
    { "E86",
      d_tests_arg_config_e86,
      { "--config", "E86.cfg" },
      NULL,
      E86_FILES },
    { "E87",
      d_tests_arg_config_e87,
      { "--config", "E87.cfg" },
      NULL,
      E87_FILES },
    { "E100",
      d_tests_arg_config_e100,
      { "--config", "E100.cfg" },
      NULL,
      E100_FILES },
    { "E101",
      d_tests_arg_config_e101,
      { "--config", "E101.cfg" },
      NULL,
      E101_FILES },
    { "E102",
      d_tests_arg_config_e102,
      { "--config", "E102.cfg" },
      NULL,
      E102_FILES },
    { "E103",
      d_tests_arg_config_e103,
      { "--config", "E103.cfg" },
      NULL,
      E103_FILES }
};

// FILE_LOOKUP_COUNT
//   constant: how many cases FILE_LOOKUP_CASES holds.
static const size_t FILE_LOOKUP_COUNT =
    sizeof(FILE_LOOKUP_CASES) / sizeof(FILE_LOOKUP_CASES[0]);

/*
d_tests_arg_config_file_lookups
  The option an item finds when the first it finds is not configurable.
  Tests the following:
  - a short name, and a positional's, not configurable
  - a positional in an option group, found past a long option that is not
    configurable
  - an option of a subcommand grouped with `+`
  - a fallthrough parent's positional, found past the subcommand's option that
    is not configurable, and not found without fallthrough
  - a fallthrough parent's short option past a long one that is not
    configurable, and a long option in an option group before a short one
  - a disabled subcommand's section, and a section no subcommand has
*/
bool
d_tests_arg_config_file_lookups(void)
{
    return d_tests_arg_cases("FILE_LOOKUPS",
                             FILE_LOOKUP_CASES,
                             FILE_LOOKUP_COUNT,
                             FILE_LOOKUPS);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 34. SECTION MARKS AND CALLBACKS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief E88: section marks given as names to a configurable root.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e88(
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
    d_arg_command_set_configurable(_app,
                                   true);
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    return;
}

/**
 * @brief E90: callbacks of an option the file gives and one the command line
 *        gave.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e90(
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

    struct d_arg_option* const b =
        d_arg_command_add_option_callback(_app,
                                          "--b",
                                          d_tests_arg_config_value,
                                          _run,
                                          NULL,
                                          NULL);

    (void)d_tests_arg_watch_option(_run,
                                   "b",
                                   b);

    struct d_arg_option* const a =
        d_arg_command_add_option_callback(_app,
                                          "--a",
                                          d_tests_arg_config_value,
                                          _run,
                                          NULL,
                                          NULL);

    (void)d_tests_arg_watch_option(_run,
                                   "a",
                                   a);

    return;
}

// E88_FILES
//   constant: the files E88 parses with.
static const struct d_tests_arg_file E88_FILES[] =
{
    { "E88.cfg", "++ = 1\na = 2\n-- = 3\n" },
    { NULL,      NULL }
};

// E90_FILES
//   constant: the files E90 parses with.
static const struct d_tests_arg_file E90_FILES[] =
{
    { "E90.cfg", "a=1\n" },
    { NULL,      NULL }
};

// FILE_MARK_CASES
//   constant: the cases of d_tests_arg_config_file_marks.
static const struct d_tests_arg_case FILE_MARK_CASES[] =
{
    { "E88",
      d_tests_arg_config_e88,
      { "--config", "E88.cfg" },
      NULL,
      E88_FILES },
    { "E90",
      d_tests_arg_config_e90,
      { "--b", "2", "--config", "E90.cfg" },
      NULL,
      E90_FILES }
};

// FILE_MARK_COUNT
//   constant: how many cases FILE_MARK_CASES holds.
static const size_t FILE_MARK_COUNT =
    sizeof(FILE_MARK_CASES) / sizeof(FILE_MARK_CASES[0]);

/*
d_tests_arg_config_file_marks
  Section marks given to a configurable root, and when a file's value runs its
option's callback.
  Tests the following:
  - the marks of a section given as names to a configurable root
  - an option's callback run as the file gives its value, before one the command
    line gave
*/
bool
d_tests_arg_config_file_marks(void)
{
    return d_tests_arg_cases("FILE_MARKS",
                             FILE_MARK_CASES,
                             FILE_MARK_COUNT,
                             FILE_MARKS);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 35. FILES NAMED
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief E91: `{}` alone.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e91(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    return;
}

/**
 * @brief E92: `{}` with a separator after it, reversed.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e92(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    return;
}

/**
 * @brief E93: `{}` with a separator before it, reversed.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e93(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    return;
}

/**
 * @brief E94: `{}` with another file.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e94(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    return;
}

/**
 * @brief E95: an empty name.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e95(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    return;
}

/**
 * @brief E104: an empty name before `{}`, reversed.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e104(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    return;
}

/**
 * @brief E105: a default of `{}`.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e105(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const config = d_arg_command_set_config(_app,
                                                                 "--config",
                                                                 "{}",
                                                                 NULL,
                                                                 false);

    (void)d_tests_arg_watch_option(_run,
                                   "config",
                                   config);
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    return;
}

/**
 * @brief E98: a default that is not there.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e98(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const config = d_arg_command_set_config(_app,
                                                                 "--config",
                                                                 "E98x.cfg",
                                                                 NULL,
                                                                 false);

    (void)d_tests_arg_watch_option(_run,
                                   "config",
                                   config);
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    return;
}

/**
 * @brief E99: a default with a malformed escape.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e99(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const config = d_arg_command_set_config(_app,
                                                                 "--config",
                                                                 "E99.cfg",
                                                                 NULL,
                                                                 false);

    (void)d_tests_arg_watch_option(_run,
                                   "config",
                                   config);
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    return;
}

/**
 * @brief E109: `{}` alone, the option taking no value or one.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e109(
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
    d_arg_option_set_expected_range(config,
                                    0,
                                    1);
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    return;
}

/**
 * @brief E110: an empty name before `{}`, reversed, the option taking one value
 *        or two.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e110(
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
    d_arg_option_set_expected_range(config,
                                    1,
                                    2);
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    return;
}

// E92_FILES
//   constant: the files E92 parses with.
static const struct d_tests_arg_file E92_FILES[] =
{
    { "{}", "a=1\n" },
    { NULL, NULL }
};

// E93_FILES
//   constant: the files E93 parses with.
static const struct d_tests_arg_file E93_FILES[] =
{
    { "{}", "a=1\n" },
    { NULL, NULL }
};

// E94_FILES
//   constant: the files E94 parses with.
static const struct d_tests_arg_file E94_FILES[] =
{
    { "{}", "a=1\n" },
    { "x",  "a=2\n" },
    { NULL, NULL }
};

// E104_FILES
//   constant: the files E104 parses with.
static const struct d_tests_arg_file E104_FILES[] =
{
    { "{}", "a=1\n" },
    { NULL, NULL }
};

// E105_FILES
//   constant: the files E105 parses with.
static const struct d_tests_arg_file E105_FILES[] =
{
    { "{}", "a=1\n" },
    { NULL, NULL }
};

// E99_FILES
//   constant: the files E99 parses with.
static const struct d_tests_arg_file E99_FILES[] =
{
    { "E99.cfg", "a=\"bad\\q\"\n" },
    { NULL,      NULL }
};

// E109_FILES
//   constant: the files E109 parses with.
static const struct d_tests_arg_file E109_FILES[] =
{
    { "{}", "a=1\n" },
    { NULL, NULL }
};

// E110_FILES
//   constant: the files E110 parses with.
static const struct d_tests_arg_file E110_FILES[] =
{
    { "{}", "a=1\n" },
    { NULL, NULL }
};

// FILE_NAMED_CASES
//   constant: the cases of d_tests_arg_config_file_named.
static const struct d_tests_arg_case FILE_NAMED_CASES[] =
{
    { "E91",
      d_tests_arg_config_e91,
      { "--config", "{}" },
      NULL,
      NULL },
    { "E92",
      d_tests_arg_config_e92,
      { "--config", "%%", "--config", "{}" },
      NULL,
      E92_FILES },
    { "E93",
      d_tests_arg_config_e93,
      { "--config", "{}", "--config", "%%" },
      NULL,
      E93_FILES },
    { "E94",
      d_tests_arg_config_e94,
      { "--config", "x", "--config", "{}" },
      NULL,
      E94_FILES },
    { "E95",
      d_tests_arg_config_e95,
      { "--config", "" },
      NULL,
      NULL },
    { "E104",
      d_tests_arg_config_e104,
      { "--config", "", "--config", "{}" },
      NULL,
      E104_FILES },
    { "E105",
      d_tests_arg_config_e105,
      { NULL },
      NULL,
      E105_FILES },
    { "E98",
      d_tests_arg_config_e98,
      { NULL },
      NULL,
      NULL },
    { "E99",
      d_tests_arg_config_e99,
      { NULL },
      NULL,
      E99_FILES },
    { "E109",
      d_tests_arg_config_e109,
      { "--config", "{}" },
      NULL,
      E109_FILES },
    { "E110",
      d_tests_arg_config_e110,
      { "--config", "", "--config", "{}" },
      NULL,
      E110_FILES }
};

// FILE_NAMED_COUNT
//   constant: how many cases FILE_NAMED_CASES holds.
static const size_t FILE_NAMED_COUNT =
    sizeof(FILE_NAMED_CASES) / sizeof(FILE_NAMED_CASES[0]);

/*
d_tests_arg_config_file_named
  What the configuration option's values name: `{}` alone and with a separator,
nothing, a default not there and one malformed; and `{}` where the option may
take no value, or two.
  Tests the following:
  - `{}` alone, which names a file
  - `{}` with a separator, which names `{}` alone, given before and after it,
    and `{}` with another file, or after an empty name
  - a default of `{}`
  - an empty name, which names none
  - a default not there, and one with a malformed escape
  - `{}` alone where the option may take no value, which names none
  - an empty name before `{}` where the option may take two values, which leaves
    `{}` alone
*/
bool
d_tests_arg_config_file_named(void)
{
    return d_tests_arg_cases("FILE_NAMED",
                             FILE_NAMED_CASES,
                             FILE_NAMED_COUNT,
                             FILE_NAMED);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 36. VARIABLES AND FILES
//==============================================================================

#if ( (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1) &&                             \
      (D_INTERNAL_ARG_PARSER_ENVIRONMENT == 1) )

/**
 * @brief E89: a configurable subcommand's parse-complete callback, and its
 *        option's variable.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e89(
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

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_configurable(sub,
                                   true);
    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");

    struct d_arg_option* const y = d_tests_arg_make_option(_run,
                                                           "y",
                                                           sub,
                                                           "--y");

    (void)d_arg_option_set_env_name(y,
                                    "DJ_CFG_E89");
    d_arg_command_set_parse_complete_callback(sub,
                                              d_tests_arg_config_complete,
                                              _run,
                                              NULL);

    return;
}

/**
 * @brief E96: a variable of one letter naming a file of one.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e96(
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
                                    "E");
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    return;
}

/**
 * @brief E97: a variable set to nothing.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e97(
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
                                    "DJ_CFG_E97");
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    return;
}

/**
 * @brief E106: a variable set to `{}`.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e106(
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
                                    "DJ_CFG_E106");
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    return;
}

// E89_VARIABLES
//   constant: the environment variables E89 parses with.
static const struct d_tests_arg_env_var E89_VARIABLES[] =
{
    { "DJ_CFG_E89", "7" },
    { NULL,         NULL }
};

// E89_FILES
//   constant: the files E89 parses with.
static const struct d_tests_arg_file E89_FILES[] =
{
    { "E89.cfg", "[sub]\nx=1\n" },
    { NULL,      NULL }
};

// E96_VARIABLES
//   constant: the environment variables E96 parses with.
static const struct d_tests_arg_env_var E96_VARIABLES[] =
{
    { "E", "c" },
    { NULL, NULL }
};

// E96_FILES
//   constant: the files E96 parses with.
static const struct d_tests_arg_file E96_FILES[] =
{
    { "c", "a=3\n" },
    { NULL, NULL }
};

// E97_VARIABLES
//   constant: the environment variables E97 parses with.
static const struct d_tests_arg_env_var E97_VARIABLES[] =
{
    { "DJ_CFG_E97", "" },
    { NULL,         NULL }
};

// E106_VARIABLES
//   constant: the environment variables E106 parses with.
static const struct d_tests_arg_env_var E106_VARIABLES[] =
{
    { "DJ_CFG_E106", "{}" },
    { NULL,          NULL }
};

// E106_FILES
//   constant: the files E106 parses with.
static const struct d_tests_arg_file E106_FILES[] =
{
    { "{}", "a=1\n" },
    { NULL, NULL }
};

// FILE_VARIABLE_CASES
//   constant: the cases of d_tests_arg_config_file_variables.
static const struct d_tests_arg_case FILE_VARIABLE_CASES[] =
{
    { "E89",
      d_tests_arg_config_e89,
      { "--config", "E89.cfg" },
      E89_VARIABLES,
      E89_FILES },
    { "E96",
      d_tests_arg_config_e96,
      { NULL },
      E96_VARIABLES,
      E96_FILES },
    { "E97",
      d_tests_arg_config_e97,
      { NULL },
      E97_VARIABLES,
      NULL },
    { "E106",
      d_tests_arg_config_e106,
      { NULL },
      E106_VARIABLES,
      E106_FILES }
};

// FILE_VARIABLE_COUNT
//   constant: how many cases FILE_VARIABLE_CASES holds.
static const size_t FILE_VARIABLE_COUNT =
    sizeof(FILE_VARIABLE_CASES) / sizeof(FILE_VARIABLE_CASES[0]);

/*
d_tests_arg_config_file_variables
  The environment beside a configurable subcommand's parse-complete callback,
and the configuration option's variable of one letter, and set to nothing.
  Tests the following:
  - a subcommand's option whose variable its parse-complete callback leaves
    unread
  - a variable of one letter naming a file of one
  - a variable set to nothing, and one set to `{}`
*/
bool
d_tests_arg_config_file_variables(void)
{
    return d_tests_arg_cases("FILE_VARIABLES",
                             FILE_VARIABLE_CASES,
                             FILE_VARIABLE_COUNT,
                             FILE_VARIABLES);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES, D_INTERNAL_ARG_PARSER_ENVIRONMENT

//==============================================================================
// 37. FILES THAT CANNOT BE READ
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief Whether a file of the scratch directory opens to be read, as the
 *        process stands: root, and Windows for a file that is only
 *        read-only, can open one without permissions; anyone else cannot.
 *
 * @param[in] _name  the file's name, relative to the scratch directory.
 * @return whether it opened.
 */
static bool
d_tests_arg_config_readable(
    const char* _name
)
{
    FILE* const file = fopen(_name,
                             "r");

    // a file that opened, closed again
    if (file != NULL)
    {
        (void)fclose(file);
    }

    return (file != NULL);
}

/**
 * @brief Makes a file of the scratch directory and takes every permission
 *        from it.
 *
 * @param[in,out] _scratch  the scratch directory, entered.
 * @param[in]     _name     the file's name.
 * @param[in]     _content  what it holds.
 * @return whether it was made and its permissions taken.
 */
static bool
d_tests_arg_config_unreadable(
    struct d_tests_arg_files* _scratch,
    const char*               _name,
    const char*               _content
)
{
    return ( (d_tests_arg_files_file(_scratch,
                                     _name,
                                     _content)) &&
             (d_tests_arg_files_mode(_scratch,
                                     _name,
                                     0u)) );
}

/**
 * @brief One case of d_tests_arg_config_file_permissions, on a run of its
 *        own: the configuration option, with a default file or none, and
 *        `--a`; the case's line, then its parse and report, appended.
 *
 * @param[in,out] _out      the section's transcript.
 * @param[in]     _tag      the case's tag.
 * @param[in]     _default  the configuration option's default file; NULL
 *                          for none.
 * @param[in]     _args     the arguments.
 * @param[in]     _count    how many.
 * @return whether the run was made and kept what it made.
 */
static bool
d_tests_arg_config_permitted(
    struct d_arg_buffer* _out,
    const char*          _tag,
    const char*          _default,
    const char* const*   _args,
    size_t               _count
)
{
    struct d_tests_arg_run      run = { 0 };
    struct d_arg_command* const app = d_tests_arg_begin(&run);

    (void)d_tests_arg_watch_option(&run,
                                   "config",
                                   d_arg_command_set_config(app,
                                                            "--config",
                                                            _default,
                                                            NULL,
                                                            false));
    (void)d_tests_arg_make_option(&run,
                                  "a",
                                  app,
                                  "--a");
    d_tests_arg_config_line(&run,
                            _tag,
                            "",
                            _args,
                            _count);
    (void)d_tests_arg_parse(&run,
                            _args,
                            _count);
    (void)d_arg_buffer_append_text(_out,
                                   d_arg_buffer_text(&run.transcript));

    const bool held = ( (app != NULL) &&
                        (!run.failed) );

    d_tests_arg_end(&run);

    return held;
}

/*
d_tests_arg_config_file_permissions
  Configuration files that are there but cannot be read: no permission at
all, given, which CLI11 refuses as a FileError, and as a default, which it
skips, leaving the configuration option unused. Root reads them, and
Windows reads a file that is only read-only, so the transcript expected is
chosen by whether the process can read one: FILE_PERMISSIONS, CLI11's run as
root, or FILE_PERMISSIONS_DENIED, its run by a user who cannot.
  Tests the following:
  - a file given that cannot be read, refused
  - a default file that cannot be read, skipped
  - both read where the process can read them
*/
bool
d_tests_arg_config_file_permissions(void)
{
    // GIVEN
    //   constant: the arguments of the case whose file is given.
    static const char* const GIVEN[] =
    {
        "--config", "E107.cfg"
    };

    struct d_tests_arg_files scratch = { 0 };
    struct d_arg_buffer      written = D_ARG_BUFFER_INIT;
    const bool               made    =
        ( (d_tests_arg_files_enter(&scratch))                       &&
          (d_tests_arg_config_unreadable(&scratch,
                                         "E107.cfg",
                                         "a=1\n"))                  &&
          (d_tests_arg_config_unreadable(&scratch,
                                         "E108.cfg",
                                         "a=2\n")) );
    const bool               denied  =
        ( (made) &&
          (!d_tests_arg_config_readable("E107.cfg")) );
    const bool               ran     =
        ( (made)                                                     &&
          (d_tests_arg_config_permitted(&written,
                                        "E107",
                                        NULL,
                                        GIVEN,
                                        sizeof(GIVEN) / sizeof(GIVEN[0]))) &&
          (d_tests_arg_config_permitted(&written,
                                        "E108",
                                        "E108.cfg",
                                        NULL,
                                        0u)) );
    const bool               same    =
        d_tests_arg_same("FILE_PERMISSIONS",
                         d_arg_buffer_text(&written),
                         (denied)
                         ? FILE_PERMISSIONS_DENIED
                         : FILE_PERMISSIONS);
    const bool               kept    = ( (written.failed == 0) &&
                                         (!scratch.failed) );

    // the permissions given back, so that every system removes the files
    (void)d_tests_arg_files_mode(&scratch,
                                 "E107.cfg",
                                 0644u);
    (void)d_tests_arg_files_mode(&scratch,
                                 "E108.cfg",
                                 0644u);
    d_tests_arg_files_leave(&scratch);
    d_arg_buffer_release(&written);

    return ( (ran)  &&
             (same) &&
             (kept) );
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 38. TEXT PARSED AS A FILE
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief E28: text after a parse.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e28(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");
    (void)d_tests_arg_make_flag(_run,
                                "b",
                                _app,
                                "--b");
    (void)d_tests_arg_make_option(_run,
                                  "c",
                                  _app,
                                  "--c");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--a", "1"
    };

    (void)d_tests_arg_config_parsed(_run,
                                    "E28",
                                    ARGS,
                                    sizeof(ARGS) / sizeof(ARGS[0]));
    (void)d_tests_arg_config_streamed(_run,
                                      "E28",
                                      "b=true\na=2\nc=3\n");

    return;
}

/**
 * @brief E29: text alone, with a subcommand's section.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e29(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");
    (void)d_tests_arg_config_streamed(_run,
                                      "E29",
                                      "a=1\n[sub]\nx=2\n");

    return;
}

/**
 * @brief E31: text with a malformed escape in a name.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e31(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");
    (void)d_tests_arg_config_streamed(_run,
                                      "E31",
                                      "\"bad\\qname\".x=1\n");

    return;
}

/**
 * @brief E57: a configurable subcommand used by text after a parse.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e57(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_configurable(sub,
                                   true);
    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--a", "1"
    };

    (void)d_tests_arg_config_parsed(_run,
                                    "E57",
                                    ARGS,
                                    sizeof(ARGS) / sizeof(ARGS[0]));
    (void)d_tests_arg_config_streamed(_run,
                                      "E57",
                                      "[sub]\nx=5\n");

    return;
}

/**
 * @brief E69: text read with the root's INI format.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e69(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_config_kept(_run,
                                  _app,
                                  true,
                                  0u);

    struct d_arg_option* const v = d_tests_arg_make_all(_run,
                                                        "v",
                                                        _app,
                                                        "--v");

    d_arg_option_set_expected(v,
                              -1);
    (void)d_tests_arg_make_option(_run,
                                  "s",
                                  _app,
                                  "--s");
    (void)d_tests_arg_config_streamed(_run,
                                      "E69",
                                      "v = 1 2 3\ns = 'a b' ; c\n");

    return;
}

/**
 * @brief E70: text naming an option that is not configurable.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e70(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const a = d_tests_arg_make_option(_run,
                                                           "a",
                                                           _app,
                                                           "--a");

    d_arg_option_set_configurable(a,
                                  false);
    (void)d_tests_arg_config_streamed(_run,
                                      "E70",
                                      "a=1\n");

    return;
}

/**
 * @brief E111: text alone, the definitions breaking a requirement, checked
 *        before it.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e111(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");
    d_arg_command_require_option(_app,
                                 2u,
                                 1u);
    (void)d_tests_arg_config_streamed(_run,
                                      "E111",
                                      "a=1\n");

    return;
}

/**
 * @brief E112: text alone, a required subcommand disabled by default, as it
 *        is configured before the text.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e112(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const s = d_tests_arg_make_subcommand(_run,
                                                                _app,
                                                                "s");

    d_arg_command_set_required(s,
                               true);
    d_arg_command_set_disabled_by_default(s,
                                          true);
    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  s,
                                  "--x");
    (void)d_tests_arg_config_streamed(_run,
                                      "E112",
                                      "");

    return;
}

/**
 * @brief E113: text after a parse, the definitions broken since: not checked
 *        again.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e113(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");
    (void)d_tests_arg_config_parsed(_run,
                                    "E113",
                                    NULL,
                                    0u);
    d_arg_command_require_option(_app,
                                 2u,
                                 1u);
    (void)d_tests_arg_config_streamed(_run,
                                      "E113",
                                      "a=1\n");

    return;
}

/**
 * @brief E114: text alone, with the root's pre-parse, parse-complete and
 *        final callbacks.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e114(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");
    d_arg_command_set_preparse_callback(_app,
                                        d_tests_arg_config_preparse,
                                        _run,
                                        NULL);
    d_arg_command_set_parse_complete_callback(_app,
                                              d_tests_arg_config_complete,
                                              _run,
                                              NULL);
    d_arg_command_set_final_callback(_app,
                                     d_tests_arg_config_final,
                                     _run,
                                     NULL);
    (void)d_tests_arg_config_streamed(_run,
                                      "E114",
                                      "a=1\n");

    return;
}

// STREAM_STEPS
//   constant: the cases of d_tests_arg_config_streams.
static const d_tests_arg_define_fn STREAM_STEPS[] =
{
    d_tests_arg_config_e28,
    d_tests_arg_config_e29,
    d_tests_arg_config_e31,
    d_tests_arg_config_e57,
    d_tests_arg_config_e69,
    d_tests_arg_config_e70,
    d_tests_arg_config_e111,
    d_tests_arg_config_e112,
    d_tests_arg_config_e113,
    d_tests_arg_config_e114
};

// STREAM_COUNT
//   constant: how many cases STREAM_STEPS holds.
static const size_t STREAM_COUNT =
    sizeof(STREAM_STEPS) / sizeof(STREAM_STEPS[0]);

/*
d_tests_arg_config_streams
  Configuration text parsed as a file would be, CLI11's parse_from_stream, with
and without a parse before it.
  Tests the following:
  - text after a parse, adding to what the command line gave
  - text alone, a subcommand's section, a malformed escape
  - a configurable subcommand used by text after a parse
  - text read with the root's INI format, and an option that is not configurable
  - text alone: the definitions checked, and configured, before it; text after a
    parse: not checked again
  - the root's pre-parse, parse-complete and final callbacks
*/
bool
d_tests_arg_config_streams(void)
{
    return d_tests_arg_steps("STREAMS",
                             STREAM_STEPS,
                             STREAM_COUNT,
                             STREAMS);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 39. FILES READ AS TEXT
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief E37: a literal value over lines ending in carriage returns.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e37(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");
    (void)d_tests_arg_make_option(_run,
                                  "b",
                                  _app,
                                  "--b");

    return;
}

/**
 * @brief E71: a value continued by a backslash before a carriage return.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e71(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");
    (void)d_tests_arg_make_option(_run,
                                  "b",
                                  _app,
                                  "--b");

    return;
}

/**
 * @brief E72: a Ctrl-Z alone on a line.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e72(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");
    (void)d_tests_arg_make_option(_run,
                                  "b",
                                  _app,
                                  "--b");

    return;
}

/**
 * @brief E73: values and a section, each line ending in a carriage return.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_e73(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");
    (void)d_tests_arg_make_option(_run,
                                  "b",
                                  _app,
                                  "--b");

    return;
}

// E37_FILES
//   constant: the files E37 parses with.
static const struct d_tests_arg_file E37_FILES[] =
{
    { "E37.cfg", "a = '''x\r\ny'''\r\nb = 2\r\n" },
    { NULL,      NULL }
};

// E71_FILES
//   constant: the files E71 parses with.
static const struct d_tests_arg_file E71_FILES[] =
{
    { "E71.cfg", "a = \"\"\"x \\\r\n  y\"\"\"\r\nb = 2\r\n" },
    { NULL,      NULL }
};

// E72_FILES
//   constant: the files E72 parses with.
static const struct d_tests_arg_file E72_FILES[] =
{
    { "E72.cfg", "a=1\n\x1a\nb=2\n" },
    { NULL,      NULL }
};

// E73_FILES
//   constant: the files E73 parses with.
static const struct d_tests_arg_file E73_FILES[] =
{
    { "E73.cfg", "a=1\r\n[sub]\r\nb=2\r\n" },
    { NULL,      NULL }
};

// TEXT_MODE_CASES
//   constant: the cases of d_tests_arg_config_text_mode.
static const struct d_tests_arg_case TEXT_MODE_CASES[] =
{
    { "E37",
      d_tests_arg_config_e37,
      { "--config", "E37.cfg" },
      NULL,
      E37_FILES },
    { "E71",
      d_tests_arg_config_e71,
      { "--config", "E71.cfg" },
      NULL,
      E71_FILES },
    { "E72",
      d_tests_arg_config_e72,
      { "--config", "E72.cfg" },
      NULL,
      E72_FILES },
    { "E73",
      d_tests_arg_config_e73,
      { "--config", "E73.cfg" },
      NULL,
      E73_FILES }
};

// TEXT_MODE_COUNT
//   constant: how many cases TEXT_MODE_CASES holds.
static const size_t TEXT_MODE_COUNT =
    sizeof(TEXT_MODE_CASES) / sizeof(TEXT_MODE_CASES[0]);

/*
d_tests_arg_config_text_mode
  Files with carriage returns and a Ctrl-Z, read as the system reads text: as
their bytes, or, on Windows, without the carriage return before a line feed and
to the first Ctrl-Z (ISSUES 15).
  Tests the following:
  - a value over lines ending in carriage returns
  - a line continued by a backslash before a carriage return
  - a Ctrl-Z, which ends the text on Windows
  - plain values and a section, the same everywhere
*/
bool
d_tests_arg_config_text_mode(void)
{
    // the transcript of the system: a file read as text on Windows
    const char* const expected = (D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID))
                                 ? TEXT_MODE_WINDOWS
                                 : TEXT_MODE;

    return d_tests_arg_cases("TEXT_MODE",
                             TEXT_MODE_CASES,
                             TEXT_MODE_COUNT,
                             expected);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 40. WRITING VALUES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief W01: a value, a flag, a list and a positional.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w01(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const a = d_arg_command_add_option(_app,
                                                            "--a",
                                                            "first option");

    (void)d_tests_arg_watch_option(_run,
                                   "a",
                                   a);
    (void)d_tests_arg_make_flag(_run,
                                "b",
                                _app,
                                "--b");

    struct d_arg_option* const v = d_tests_arg_make_all(_run,
                                                        "v",
                                                        _app,
                                                        "--v");

    d_arg_option_set_expected(v,
                              -1);
    (void)d_tests_arg_make_option(_run,
                                  "pos",
                                  _app,
                                  "pos");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--a", "1", "--b", "--v", "1", "2", "3", "p"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W01",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

/**
 * @brief W02: defaults, required options, and options left without values.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w02(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const s = d_tests_arg_make_option(_run,
                                                           "s",
                                                           _app,
                                                           "--s");

    (void)d_arg_option_set_default_text(s,
                                        "abc");

    struct d_arg_option* const n = d_tests_arg_make_option(_run,
                                                           "n",
                                                           _app,
                                                           "--n");

    (void)d_arg_option_set_default_text(n,
                                        "5");
    d_arg_option_set_required(n,
                              true);

    struct d_arg_option* const r = d_tests_arg_make_option(_run,
                                                           "r",
                                                           _app,
                                                           "--r");

    d_arg_option_set_required(r,
                              true);
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                _app,
                                "--f");

    struct d_arg_option* const c = d_tests_arg_make_option(_run,
                                                           "c",
                                                           _app,
                                                           "--c");

    d_arg_option_set_run_callback_for_default(c,
                                              true);

    struct d_arg_option* const e = d_tests_arg_make_option(_run,
                                                           "e",
                                                           _app,
                                                           "--e");

    d_arg_option_set_expected_range(e,
                                    0,
                                    1);

    struct d_arg_option* const l = d_tests_arg_make_all(_run,
                                                        "l",
                                                        _app,
                                                        "--l");

    d_arg_option_set_expected(l,
                              -1);
    (void)d_arg_option_set_default_text(l,
                                        "[1,2]");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--r", "x", "--n", "6"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W02",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

/**
 * @brief W10: flags with default values, negated names and a count.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w10(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                _app,
                                "--f{5}");
    (void)d_tests_arg_make_flag(_run,
                                "k",
                                _app,
                                "--k,!--nok");
    (void)d_tests_arg_make_flag(_run,
                                "g",
                                _app,
                                "--g{7}");

    struct d_arg_option* const h = d_tests_arg_make_flag(_run,
                                                         "h",
                                                         _app,
                                                         "--h{7}");

    d_arg_option_set_disable_flag_override(h,
                                           true);
    (void)d_tests_arg_make_flag(_run,
                                "v",
                                _app,
                                "-v,--verbose");
    (void)d_tests_arg_make_flag(_run,
                                "n",
                                _app,
                                "--n,!--non");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--f", "--nok", "--h", "-vvv"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W10",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

/**
 * @brief W11: defaults commented out.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w11(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_config_kept(_run,
                                  _app,
                                  false,
                                  D_ARG_CONFIG_COMMENT_DEFAULTS);

    struct d_arg_option* const s = d_tests_arg_make_option(_run,
                                                           "s",
                                                           _app,
                                                           "--s");

    (void)d_arg_option_set_default_text(s,
                                        "abc");
    (void)d_tests_arg_make_flag(_run,
                                "f",
                                _app,
                                "--f");
    (void)d_tests_arg_make_option(_run,
                                  "g",
                                  _app,
                                  "--g");

    struct d_arg_option* const h = d_tests_arg_make_option(_run,
                                                           "h",
                                                           _app,
                                                           "--h");

    d_arg_option_set_required(h,
                              true);
    (void)d_tests_arg_make_option(_run,
                                  "given",
                                  _app,
                                  "--given");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--given", "1", "--h", "2"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W11",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

/**
 * @brief W14: lists given `{}`.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w14(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const v = d_tests_arg_make_all(_run,
                                                        "v",
                                                        _app,
                                                        "--v");

    d_arg_option_set_expected_range(v,
                                    0,
                                    10);

    struct d_arg_option* const w = d_tests_arg_make_all(_run,
                                                        "w",
                                                        _app,
                                                        "--w");

    d_arg_option_set_expected(w,
                              -1);

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--v", "{}", "--w", "{}"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W14",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

/**
 * @brief W15: a positional, an option with a variable, short names.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w15(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_option(_run,
                                  "pos",
                                  _app,
                                  "pos");

    struct d_arg_option* const only = d_tests_arg_make_option(_run,
                                                              "only",
                                                              _app,
                                                              "--only");

    (void)d_arg_option_set_env_name(only,
                                    "ONLY_ENV");
    (void)d_tests_arg_make_option(_run,
                                  "s",
                                  _app,
                                  "-s");
    (void)d_tests_arg_make_option(_run,
                                  "t",
                                  _app,
                                  "-t,--tee");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "p1", "-s", "2", "-t", "3"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W15",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

/**
 * @brief W16: a validated sum.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w16(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const n = d_tests_arg_make_option(_run,
                                                           "n",
                                                           _app,
                                                           "--n");

    d_arg_option_set_expected(n,
                              -1);
    d_arg_option_set_multi_option_policy(n,
                                         (int)D_ARG_MULTI_SUM);
    (void)d_arg_option_check(n,
                             d_arg_validator_range_long(0,
                                                        5),
                             NULL);

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--n", "3", "4"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W16",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

/**
 * @brief W17: the configuration and version options, and a default.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w17(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_config(_app,
                                   "--config",
                                   NULL,
                                   NULL,
                                   false);
    (void)d_arg_command_set_version_flag(_app,
                                         "--version",
                                         "1.0",
                                         NULL);

    struct d_arg_option* const a = d_tests_arg_make_option(_run,
                                                           "a",
                                                           _app,
                                                           "--a");

    (void)d_arg_option_set_default_text(a,
                                        "1");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--a", "2"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W17",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

/**
 * @brief W19: values given to an option without a parse.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w19(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const t = d_tests_arg_make_option(_run,
                                                           "t",
                                                           _app,
                                                           "--t");

    (void)d_arg_option_add_result(t,
                                  d_pack_text_from_cstr("1"),
                                  NULL);
    (void)d_arg_option_add_result(t,
                                  d_pack_text_from_cstr("2"),
                                  NULL);
    (void)d_tests_arg_config_line(_run,
                                  "W19",
                                  "",
                                  NULL,
                                  0u);
    (void)d_tests_arg_config_write_all(_run,
                                       "");

    return;
}

// WRITE_BASIC_STEPS
//   constant: the cases of d_tests_arg_config_write_basics.
static const d_tests_arg_define_fn WRITE_BASIC_STEPS[] =
{
    d_tests_arg_config_w01,
    d_tests_arg_config_w02,
    d_tests_arg_config_w10,
    d_tests_arg_config_w11,
    d_tests_arg_config_w14,
    d_tests_arg_config_w15,
    d_tests_arg_config_w16,
    d_tests_arg_config_w17,
    d_tests_arg_config_w19
};

// WRITE_BASIC_COUNT
//   constant: how many cases WRITE_BASIC_STEPS holds.
static const size_t WRITE_BASIC_COUNT =
    sizeof(WRITE_BASIC_STEPS) / sizeof(WRITE_BASIC_STEPS[0]);

/*
d_tests_arg_config_write_basics
  What each mode writes of values, flags, positionals and defaults.
  Tests the following:
  - values given, a flag, a list and a positional
  - defaults, required options, and what each mode writes for an option with
    nothing
  - flags with default values, negated names and counts
  - defaults commented out, lists of `{}`, the configuration and version
    options, a validated sum
  - values given to an option without a parse
*/
bool
d_tests_arg_config_write_basics(void)
{
    return d_tests_arg_steps("WRITE_BASICS",
                             WRITE_BASIC_STEPS,
                             WRITE_BASIC_COUNT,
                             WRITE_BASICS);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 41. WRITING TEXT
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief W03: values of every kind, alone and in a list.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w03(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const s = d_tests_arg_make_all(_run,
                                                        "s",
                                                        _app,
                                                        "--s");

    d_arg_option_set_expected(s,
                              -1);
    (void)d_tests_arg_make_option(_run,
                                  "o1",
                                  _app,
                                  "--o1");
    (void)d_tests_arg_make_option(_run,
                                  "o2",
                                  _app,
                                  "--o2");
    (void)d_tests_arg_make_option(_run,
                                  "o3",
                                  _app,
                                  "--o3");
    (void)d_tests_arg_make_option(_run,
                                  "o4",
                                  _app,
                                  "--o4");
    (void)d_tests_arg_make_option(_run,
                                  "o5",
                                  _app,
                                  "--o5");
    (void)d_tests_arg_make_option(_run,
                                  "o6",
                                  _app,
                                  "--o6");
    (void)d_tests_arg_make_option(_run,
                                  "o7",
                                  _app,
                                  "--o7");
    (void)d_tests_arg_make_option(_run,
                                  "o8",
                                  _app,
                                  "--o8");
    (void)d_tests_arg_make_option(_run,
                                  "o9",
                                  _app,
                                  "--o9");
    (void)d_tests_arg_make_option(_run,
                                  "o10",
                                  _app,
                                  "--o10");
    (void)d_tests_arg_make_option(_run,
                                  "o11",
                                  _app,
                                  "--o11");
    (void)d_tests_arg_make_option(_run,
                                  "o12",
                                  _app,
                                  "--o12");
    (void)d_tests_arg_make_option(_run,
                                  "o13",
                                  _app,
                                  "--o13");
    (void)d_tests_arg_make_option(_run,
                                  "o14",
                                  _app,
                                  "--o14");
    (void)d_tests_arg_make_option(_run,
                                  "o15",
                                  _app,
                                  "--o15");
    (void)d_tests_arg_make_option(_run,
                                  "o16",
                                  _app,
                                  "--o16");
    (void)d_tests_arg_make_option(_run,
                                  "o17",
                                  _app,
                                  "--o17");
    (void)d_tests_arg_make_option(_run,
                                  "o18",
                                  _app,
                                  "--o18");
    (void)d_tests_arg_make_option(_run,
                                  "o19",
                                  _app,
                                  "--o19");
    (void)d_tests_arg_make_option(_run,
                                  "o20",
                                  _app,
                                  "--o20");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--s", "hello world", "a\"b", "tab\there", "", "x", "'", "0x1F",
        "\xc3\xa9", "line\nbreak", "--o1", "hello world", "--o2", "a\"b",
        "--o3", "tab\there", "--o4", "", "--o5", "x", "--o6", "'", "--o7",
        "0x1F", "--o8", "0X1F", "--o9", "0o17", "--o10", "0b101", "--o11",
        "0b102", "--o12", "1e5", "--o13", "nan", "--o14", "true", "--o15",
        "-1.5", "--o16", "1e", "--o17", ".5", "--o18", "\xc3\xa9", "--o19",
        "line\nbreak", "--o20", "0x"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W03",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

/**
 * @brief W05: names holding the characters that mean something.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w05(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_option(_run,
                                  "o1",
                                  _app,
                                  "--a.b");
    (void)d_tests_arg_make_option(_run,
                                  "o2",
                                  _app,
                                  "--x#y");
    (void)d_tests_arg_make_option(_run,
                                  "o3",
                                  _app,
                                  "--it's");
    (void)d_tests_arg_make_option(_run,
                                  "o4",
                                  _app,
                                  "--qu\"ote");
    (void)d_tests_arg_make_option(_run,
                                  "o5",
                                  _app,
                                  "--[br]");
    (void)d_tests_arg_make_option(_run,
                                  "o6",
                                  _app,
                                  "--back\\slash");
    (void)d_tests_arg_make_option(_run,
                                  "o7",
                                  _app,
                                  "--semi;colon");
    (void)d_tests_arg_make_option(_run,
                                  "o8",
                                  _app,
                                  "--eq");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--a.b", "1", "--x#y", "2", "--it's", "3", "--qu\"ote", "4", "--[br]",
        "5", "--back\\slash", "6", "--semi;colon", "7", "--eq", "8"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W05",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

// WRITE_TEXT_STEPS
//   constant: the cases of d_tests_arg_config_write_text.
static const d_tests_arg_define_fn WRITE_TEXT_STEPS[] =
{
    d_tests_arg_config_w03,
    d_tests_arg_config_w05
};

// WRITE_TEXT_COUNT
//   constant: how many cases WRITE_TEXT_STEPS holds.
static const size_t WRITE_TEXT_COUNT =
    sizeof(WRITE_TEXT_STEPS) / sizeof(WRITE_TEXT_STEPS[0]);

/*
d_tests_arg_config_write_text
  Values and names that need quoting or escaping.
  Tests the following:
  - values with spaces, quotes, tabs, newlines, UTF-8 and nothing, alone and in
    a list
  - numbers in each base, and what only looks like one
  - names holding the characters that mark comments, quotes, arrays, sections
    and values
*/
bool
d_tests_arg_config_write_text(void)
{
    return d_tests_arg_steps("WRITE_TEXT",
                             WRITE_TEXT_STEPS,
                             WRITE_TEXT_COUNT,
                             WRITE_TEXT);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 42. LONG VALUES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief W04: long values with newlines.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w04(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_option(_run,
                                  "l1",
                                  _app,
                                  "--l1");
    (void)d_tests_arg_make_option(_run,
                                  "l2",
                                  _app,
                                  "--l2");
    (void)d_tests_arg_make_option(_run,
                                  "l3",
                                  _app,
                                  "--l3");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--l1",
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\naaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "--l2",
        "aaaaaaaaaa'''aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\naaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "--l3",
        "\nbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\n"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W04",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

// WRITE_LONG_STEPS
//   constant: the cases of d_tests_arg_config_write_long.
static const d_tests_arg_define_fn WRITE_LONG_STEPS[] =
{
    d_tests_arg_config_w04
};

// WRITE_LONG_COUNT
//   constant: how many cases WRITE_LONG_STEPS holds.
static const size_t WRITE_LONG_COUNT =
    sizeof(WRITE_LONG_STEPS) / sizeof(WRITE_LONG_STEPS[0]);

/*
d_tests_arg_config_write_long
  Values long enough to be written over lines.
  Tests the following:
  - a value with a newline, written over lines
  - one holding the triple quote that would end it
  - one beginning and ending with newlines
*/
bool
d_tests_arg_config_write_long(void)
{
    return d_tests_arg_steps("WRITE_LONG",
                             WRITE_LONG_STEPS,
                             WRITE_LONG_COUNT,
                             WRITE_LONG);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 43. LONG VALUES WITHOUT NEWLINES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief W26: long values without newlines, and in a list.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w26(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_option(_run,
                                  "l4",
                                  _app,
                                  "--l4");
    (void)d_tests_arg_make_option(_run,
                                  "l5",
                                  _app,
                                  "--l5");

    struct d_arg_option* const l6 = d_tests_arg_make_all(_run,
                                                         "l6",
                                                         _app,
                                                         "--l6");

    d_arg_option_set_expected(l6,
                              -1);

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--l4",
        "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
        "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc", "--l5",
        "dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
        "dddddddddddddddddddddddddddddddddddddddd\"q\"", "--l6",
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\naaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "x"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W26",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

// WRITE_LONGER_STEPS
//   constant: the cases of d_tests_arg_config_write_longer.
static const d_tests_arg_define_fn WRITE_LONGER_STEPS[] =
{
    d_tests_arg_config_w26
};

// WRITE_LONGER_COUNT
//   constant: how many cases WRITE_LONGER_STEPS holds.
static const size_t WRITE_LONGER_COUNT =
    sizeof(WRITE_LONGER_STEPS) / sizeof(WRITE_LONGER_STEPS[0]);

/*
d_tests_arg_config_write_longer
  Long values without newlines, and one in a list.
  Tests the following:
  - a long value, and one ending in quotes, each on its line
  - a long value with a newline in a list
*/
bool
d_tests_arg_config_write_longer(void)
{
    return d_tests_arg_steps("WRITE_LONGER",
                             WRITE_LONGER_STEPS,
                             WRITE_LONGER_COUNT,
                             WRITE_LONGER);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 44. WRITING POLICIES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief W09: each policy for several values.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w09(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const rev = d_tests_arg_make_all(_run,
                                                          "rev",
                                                          _app,
                                                          "--rev");

    d_arg_option_set_expected(rev,
                              -1);
    d_arg_option_set_multi_option_policy(rev,
                                         (int)D_ARG_MULTI_REVERSE);

    struct d_arg_option* const sum = d_tests_arg_make_option(_run,
                                                             "sum",
                                                             _app,
                                                             "--sum");

    d_arg_option_set_expected(sum,
                              -1);
    d_arg_option_set_multi_option_policy(sum,
                                         (int)D_ARG_MULTI_SUM);

    struct d_arg_option* const join = d_tests_arg_make_option(_run,
                                                              "join",
                                                              _app,
                                                              "--join");

    d_arg_option_set_delimiter(join,
                               ',');
    d_arg_option_set_multi_option_policy(join,
                                         (int)D_ARG_MULTI_JOIN);

    struct d_arg_option* const join0 = d_tests_arg_make_option(_run,
                                                               "join0",
                                                               _app,
                                                               "--join0");

    d_arg_option_set_multi_option_policy(join0,
                                         (int)D_ARG_MULTI_JOIN);

    struct d_arg_option* const joinbad = d_tests_arg_make_option(_run,
                                                                 "joinbad",
                                                                 _app,
                                                                 "--joinbad");

    d_arg_option_set_delimiter(joinbad,
                               ',');
    d_arg_option_set_multi_option_policy(joinbad,
                                         (int)D_ARG_MULTI_JOIN);

    struct d_arg_option* const first = d_tests_arg_make_option(_run,
                                                               "first",
                                                               _app,
                                                               "--first");

    d_arg_option_set_multi_option_policy(first,
                                         (int)D_ARG_MULTI_TAKE_FIRST);

    struct d_arg_option* const last = d_tests_arg_make_option(_run,
                                                              "last",
                                                              _app,
                                                              "--last");

    d_arg_option_set_multi_option_policy(last,
                                         (int)D_ARG_MULTI_TAKE_LAST);

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--rev", "1", "2", "3", "--sum", "1", "2", "3", "--join", "a", "--join",
        "b", "--join0", "a", "--join0", "b", "--joinbad", "a,", "--joinbad",
        "b", "--first", "f1", "--first", "f2", "--last", "l1", "--last", "l2"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W09",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

/**
 * @brief W23: joins of values holding the delimiter, and an empty one.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w23(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const j = d_tests_arg_make_option(_run,
                                                           "j",
                                                           _app,
                                                           "--j");

    d_arg_option_set_delimiter(j,
                               ',');
    d_arg_option_set_multi_option_policy(j,
                                         (int)D_ARG_MULTI_JOIN);

    struct d_arg_option* const k = d_tests_arg_make_option(_run,
                                                           "k",
                                                           _app,
                                                           "--k");

    d_arg_option_set_delimiter(k,
                               ',');
    d_arg_option_set_multi_option_policy(k,
                                         (int)D_ARG_MULTI_JOIN);

    struct d_arg_option* const m = d_tests_arg_make_option(_run,
                                                           "m",
                                                           _app,
                                                           "--m");

    d_arg_option_set_delimiter(m,
                               ',');
    d_arg_option_set_multi_option_policy(m,
                                         (int)D_ARG_MULTI_JOIN);

    struct d_arg_option* const s = d_tests_arg_make_option(_run,
                                                           "s",
                                                           _app,
                                                           "--s");

    d_arg_option_set_expected(s,
                              -1);
    d_arg_option_set_multi_option_policy(s,
                                         (int)D_ARG_MULTI_SUM);

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--j", "a,b", "--j", "c", "--k", "", "--k", "b", "--m", "a", "--m",
        ",,b", "--s", "x", "y"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W23",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

// WRITE_POLICY_STEPS
//   constant: the cases of d_tests_arg_config_write_policies.
static const d_tests_arg_define_fn WRITE_POLICY_STEPS[] =
{
    d_tests_arg_config_w09,
    d_tests_arg_config_w23
};

// WRITE_POLICY_COUNT
//   constant: how many cases WRITE_POLICY_STEPS holds.
static const size_t WRITE_POLICY_COUNT =
    sizeof(WRITE_POLICY_STEPS) / sizeof(WRITE_POLICY_STEPS[0]);

/*
d_tests_arg_config_write_policies
  What each policy for several values writes.
  Tests the following:
  - reversed, summed, joined with and without a delimiter, the first and the
    last
  - a join whose values hold the delimiter, or are empty, written as given; a
    sum of what is not a number
*/
bool
d_tests_arg_config_write_policies(void)
{
    return d_tests_arg_steps("WRITE_POLICIES",
                             WRITE_POLICY_STEPS,
                             WRITE_POLICY_COUNT,
                             WRITE_POLICIES);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 45. WRITING SUBCOMMANDS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief W06: subcommands that are not configurable, used and not.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w06(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    struct d_arg_command* const sub = d_arg_command_add_subcommand(_app,
                                                                   "sub",
                                                                   "the sub");

    (void)d_tests_arg_watch_command(_run,
                                    "sub",
                                    sub);
    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");

    struct d_arg_command* const deep = d_tests_arg_make_subcommand(_run,
                                                                   sub,
                                                                   "deep");

    (void)d_tests_arg_make_option(_run,
                                  "y",
                                  deep,
                                  "--y");

    struct d_arg_command* const idle = d_tests_arg_make_subcommand(_run,
                                                                   _app,
                                                                   "idle");
    struct d_arg_option* const  z    = d_tests_arg_make_option(_run,
                                                               "z",
                                                               idle,
                                                               "--z");

    (void)d_arg_option_set_default_text(z,
                                        "zz");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--a", "0", "sub", "--x", "1", "deep", "--y", "2"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W06",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

/**
 * @brief W07: configurable subcommands, nested, beside one that is not.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w07(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    struct d_arg_command* const sub = d_arg_command_add_subcommand(_app,
                                                                   "sub",
                                                                   "the sub");

    (void)d_tests_arg_watch_command(_run,
                                    "sub",
                                    sub);
    d_arg_command_set_configurable(sub,
                                   true);
    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");

    struct d_arg_command* const deep = d_arg_command_add_subcommand(sub,
                                                                    "deep",
                                                                    "the deep");

    (void)d_tests_arg_watch_command(_run,
                                    "deep",
                                    deep);
    d_arg_command_set_configurable(deep,
                                   true);
    (void)d_tests_arg_make_option(_run,
                                  "y",
                                  deep,
                                  "--y");

    struct d_arg_command* const plain = d_tests_arg_make_subcommand(_run,
                                                                    _app,
                                                                    "plain");

    (void)d_tests_arg_make_option(_run,
                                  "p",
                                  plain,
                                  "--p");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--a", "0", "plain", "--p", "3", "sub", "--x", "1", "deep", "--y", "2"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W07",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

/**
 * @brief W18: configurable subcommands with defaults, one used.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w18(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const a = d_tests_arg_make_subcommand(_run,
                                                                _app,
                                                                "a");
    struct d_arg_option* const  x = d_tests_arg_make_option(_run,
                                                            "x",
                                                            a,
                                                            "--x");

    (void)d_arg_option_set_default_text(x,
                                        "1");

    struct d_arg_command* const b = d_tests_arg_make_subcommand(_run,
                                                                _app,
                                                                "b");
    struct d_arg_option* const  y = d_tests_arg_make_option(_run,
                                                            "y",
                                                            b,
                                                            "--y");

    (void)d_arg_option_set_default_text(y,
                                        "2");
    d_arg_command_set_configurable(a,
                                   true);
    d_arg_command_set_configurable(b,
                                   true);

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "a"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W18",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

/**
 * @brief W21: a configurable subcommand inside an option group.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w21(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const grp   = d_tests_arg_make_group(_run,
                                                               _app,
                                                               "grp");
    struct d_arg_command* const insub = d_tests_arg_make_subcommand(_run,
                                                                    grp,
                                                                    "insub");

    d_arg_command_set_configurable(insub,
                                   true);
    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  insub,
                                  "--x");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "insub", "--x", "1"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W21",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

// WRITE_COMMAND_STEPS
//   constant: the cases of d_tests_arg_config_write_commands.
static const d_tests_arg_define_fn WRITE_COMMAND_STEPS[] =
{
    d_tests_arg_config_w06,
    d_tests_arg_config_w07,
    d_tests_arg_config_w18,
    d_tests_arg_config_w21
};

// WRITE_COMMAND_COUNT
//   constant: how many cases WRITE_COMMAND_STEPS holds.
static const size_t WRITE_COMMAND_COUNT =
    sizeof(WRITE_COMMAND_STEPS) / sizeof(WRITE_COMMAND_STEPS[0]);

/*
d_tests_arg_config_write_commands
  Subcommands, configurable or not, used or not, under each mode.
  Tests the following:
  - subcommands that are not configurable, as dotted names
  - configurable ones as sections, nested, beside one that is not
  - subcommands not used, written by the modes for defaults
  - a configurable subcommand inside an option group
*/
bool
d_tests_arg_config_write_commands(void)
{
    return d_tests_arg_steps("WRITE_COMMANDS",
                             WRITE_COMMAND_STEPS,
                             WRITE_COMMAND_COUNT,
                             WRITE_COMMANDS);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 46. WRITING GROUPS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief W08: option groups, and options naming groups.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w08(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const grp = d_arg_command_add_option_group(_app,
                                                                     "grp",
                                                                     "grouped");
    struct d_arg_option* const  gx  = d_arg_command_add_option(grp,
                                                               "--gx",
                                                               "gx option");

    (void)d_tests_arg_watch_option(_run,
                                   "gx",
                                   gx);

    struct d_arg_command* const plus = d_arg_command_add_option_group(_app,
                                                                      "+plus",
                                                                      NULL);

    (void)d_tests_arg_make_option(_run,
                                  "hx",
                                  plus,
                                  "--hx");

    struct d_arg_option* const top = d_tests_arg_make_option(_run,
                                                             "top",
                                                             _app,
                                                             "--top");

    (void)d_arg_option_set_group(top,
                                 "Other");

    struct d_arg_option* const top2 = d_tests_arg_make_option(_run,
                                                              "top2",
                                                              _app,
                                                              "--top2");

    (void)d_arg_option_set_group(top2,
                                 "Other");

    struct d_arg_option* const hid = d_tests_arg_make_option(_run,
                                                             "hid",
                                                             _app,
                                                             "--hid");

    (void)d_arg_option_set_group(hid,
                                 "");
    (void)d_tests_arg_make_option(_run,
                                  "top3",
                                  _app,
                                  "--top3");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--gx", "1", "--hx", "2", "--top", "3", "--hid", "4", "--top3", "5"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W08",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

/**
 * @brief W13: descriptions holding newlines.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w13(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_description(_app,
                                        "line1\nline2\r\nline3");

    struct d_arg_option* const a = d_arg_command_add_option(_app,
                                                            "--a",
                                                            "a\nb");

    (void)d_tests_arg_watch_option(_run,
                                   "a",
                                   a);

    struct d_arg_command* const grp = d_arg_command_add_option_group(_app,
                                                                     "grp",
                                                                     "g\ndesc");
    struct d_arg_option* const  b   = d_arg_command_add_option(grp,
                                                               "--b",
                                                               "bdesc");

    (void)d_tests_arg_watch_option(_run,
                                   "b",
                                   b);

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--a", "1", "--b", "2"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W13",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

/**
 * @brief W22: a nameless subcommand's option.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w22(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    struct d_arg_command* const nameless =
        d_arg_command_add_subcommand(_app,
                                     "",
                                     "nameless");

    (void)d_tests_arg_make_option(_run,
                                  "b",
                                  nameless,
                                  "--b");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--a", "1", "--b", "2"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W22",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

// WRITE_GROUP_STEPS
//   constant: the cases of d_tests_arg_config_write_groups.
static const d_tests_arg_define_fn WRITE_GROUP_STEPS[] =
{
    d_tests_arg_config_w08,
    d_tests_arg_config_w13,
    d_tests_arg_config_w22
};

// WRITE_GROUP_COUNT
//   constant: how many cases WRITE_GROUP_STEPS holds.
static const size_t WRITE_GROUP_COUNT =
    sizeof(WRITE_GROUP_STEPS) / sizeof(WRITE_GROUP_STEPS[0]);

/*
d_tests_arg_config_write_groups
  Option groups and the groups options name, a nameless subcommand, and
descriptions over lines.
  Tests the following:
  - an option group's options, and those of one shown with its parent
  - options naming a group, and none
  - descriptions holding newlines
  - a nameless subcommand's option
*/
bool
d_tests_arg_config_write_groups(void)
{
    return d_tests_arg_steps("WRITE_GROUPS",
                             WRITE_GROUP_STEPS,
                             WRITE_GROUP_COUNT,
                             WRITE_GROUPS);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 47. WRITING IN OTHER FORMATS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief W12: an INI file.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w12(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_config_kept(_run,
                                  _app,
                                  true,
                                  0u);

    struct d_arg_option* const v = d_tests_arg_make_all(_run,
                                                        "v",
                                                        _app,
                                                        "--v");

    d_arg_option_set_expected(v,
                              -1);
    (void)d_tests_arg_make_option(_run,
                                  "s",
                                  _app,
                                  "--s");

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_configurable(sub,
                                   true);
    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--v", "1", "2", "3", "--s", "a b", "sub", "--x", "y"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W12",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

/**
 * @brief W25: a format of the caller's characters.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w25(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_config_characters(_run,
                                        _app);

    struct d_arg_option* const v = d_arg_command_add_option(_app,
                                                            "--v",
                                                            "values");

    (void)d_tests_arg_watch_option(_run,
                                   "v",
                                   v);
    d_arg_option_set_multi_option_policy(v,
                                         (int)D_ARG_MULTI_TAKE_ALL);
    d_arg_option_set_expected(v,
                              -1);
    (void)d_tests_arg_make_option(_run,
                                  "s",
                                  _app,
                                  "--s");
    (void)d_tests_arg_make_option(_run,
                                  "t",
                                  _app,
                                  "--t");
    (void)d_tests_arg_make_option(_run,
                                  "nm",
                                  _app,
                                  "--n!m");
    (void)d_tests_arg_make_option(_run,
                                  "pq",
                                  _app,
                                  "--p/q<r>");

    struct d_arg_command* const sub = d_arg_command_add_subcommand(_app,
                                                                   "sub",
                                                                   "a sub");

    (void)d_tests_arg_watch_command(_run,
                                    "sub",
                                    sub);
    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");

    struct d_arg_command* const deep = d_tests_arg_make_subcommand(_run,
                                                                   sub,
                                                                   "deep");

    d_arg_command_set_configurable(deep,
                                   true);
    (void)d_tests_arg_make_option(_run,
                                  "y",
                                  deep,
                                  "--y");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--v", "1", "a b", "--s", "it's", "--t", "x!y", "--n!m", "4",
        "--p/q<r>", "7", "sub", "--x", "5", "deep", "--y", "6"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W25",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

// WRITE_FORMAT_STEPS
//   constant: the cases of d_tests_arg_config_write_formats.
static const d_tests_arg_define_fn WRITE_FORMAT_STEPS[] =
{
    d_tests_arg_config_w12,
    d_tests_arg_config_w25
};

// WRITE_FORMAT_COUNT
//   constant: how many cases WRITE_FORMAT_STEPS holds.
static const size_t WRITE_FORMAT_COUNT =
    sizeof(WRITE_FORMAT_STEPS) / sizeof(WRITE_FORMAT_STEPS[0]);

/*
d_tests_arg_config_write_formats
  INI, and a format of the caller's characters.
  Tests the following:
  - INI: lists separated by spaces, its comment character
  - characters of the caller's marking comments, values, arrays, quotes and
    sections, in values, names and a section
*/
bool
d_tests_arg_config_write_formats(void)
{
    return d_tests_arg_steps("WRITE_FORMATS",
                             WRITE_FORMAT_STEPS,
                             WRITE_FORMAT_COUNT,
                             WRITE_FORMATS);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 48. A PREFIX
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief W24: a prefix through subcommands configurable and not.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w24(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    struct d_arg_command* const sub = d_tests_arg_make_subcommand(_run,
                                                                  _app,
                                                                  "sub");

    d_arg_command_set_configurable(sub,
                                   true);
    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  sub,
                                  "--x");

    struct d_arg_command* const plain = d_tests_arg_make_subcommand(_run,
                                                                    _app,
                                                                    "plain");

    (void)d_tests_arg_make_option(_run,
                                  "p",
                                  plain,
                                  "--p");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--a", "1", "sub", "--x", "2", "plain", "--p", "3"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W24",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "pre.");

    return;
}

/**
 * @brief W27: a prefix through groups and a nameless subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w27(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    struct d_arg_command* const pg = d_arg_command_add_option_group(_app,
                                                                    "+pg",
                                                                    NULL);

    (void)d_tests_arg_make_option(_run,
                                  "p",
                                  pg,
                                  "--p");

    struct d_arg_command* const grp = d_tests_arg_make_group(_run,
                                                             _app,
                                                             "grp");

    (void)d_tests_arg_make_option(_run,
                                  "g",
                                  grp,
                                  "--g");

    struct d_arg_command* const nl = d_arg_command_add_subcommand(_app,
                                                                  "",
                                                                  "nl");

    (void)d_tests_arg_make_option(_run,
                                  "n",
                                  nl,
                                  "--n");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--a", "1", "--p", "2", "--g", "3", "--n", "4"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W27",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "pre.");

    return;
}

// WRITE_PREFIX_STEPS
//   constant: the cases of d_tests_arg_config_write_prefix.
static const d_tests_arg_define_fn WRITE_PREFIX_STEPS[] =
{
    d_tests_arg_config_w24,
    d_tests_arg_config_w27
};

// WRITE_PREFIX_COUNT
//   constant: how many cases WRITE_PREFIX_STEPS holds.
static const size_t WRITE_PREFIX_COUNT =
    sizeof(WRITE_PREFIX_STEPS) / sizeof(WRITE_PREFIX_STEPS[0]);

/*
d_tests_arg_config_write_prefix
  What precedes each name, through subcommands configurable and not, groups and
a nameless subcommand.
  Tests the following:
  - a prefix before names, in a configurable subcommand's section and before a
    dotted subcommand's names
  - a prefix through an option group, one shown with its parent, and a nameless
    subcommand
*/
bool
d_tests_arg_config_write_prefix(void)
{
    return d_tests_arg_steps("WRITE_PREFIX",
                             WRITE_PREFIX_STEPS,
                             WRITE_PREFIX_COUNT,
                             WRITE_PREFIX);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 49. VALUES AT THE EDGES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief W28: values at the edges of each form.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w28(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_option(_run,
                                  "v1",
                                  _app,
                                  "--v1");
    (void)d_tests_arg_make_option(_run,
                                  "v2",
                                  _app,
                                  "--v2");
    (void)d_tests_arg_make_option(_run,
                                  "v3",
                                  _app,
                                  "--v3");
    (void)d_tests_arg_make_option(_run,
                                  "v4",
                                  _app,
                                  "--v4");
    (void)d_tests_arg_make_option(_run,
                                  "v5",
                                  _app,
                                  "--v5");
    (void)d_tests_arg_make_option(_run,
                                  "v6",
                                  _app,
                                  "--v6");
    (void)d_tests_arg_make_option(_run,
                                  "v7",
                                  _app,
                                  "--v7");
    (void)d_tests_arg_make_option(_run,
                                  "v8",
                                  _app,
                                  "--v8");
    (void)d_tests_arg_make_option(_run,
                                  "v9",
                                  _app,
                                  "--v9");
    (void)d_tests_arg_make_option(_run,
                                  "v10",
                                  _app,
                                  "--v10");
    (void)d_tests_arg_make_option(_run,
                                  "v11",
                                  _app,
                                  "--v11");
    (void)d_tests_arg_make_option(_run,
                                  "v12",
                                  _app,
                                  "--v12");
    (void)d_tests_arg_make_option(_run,
                                  "v13",
                                  _app,
                                  "--v13");
    (void)d_tests_arg_make_option(_run,
                                  "v14",
                                  _app,
                                  "--v14");
    (void)d_tests_arg_make_option(_run,
                                  "v15",
                                  _app,
                                  "--v15");
    (void)d_tests_arg_make_option(_run,
                                  "v16",
                                  _app,
                                  "--v16");
    (void)d_tests_arg_make_option(_run,
                                  "v17",
                                  _app,
                                  "--v17");
    (void)d_tests_arg_make_option(_run,
                                  "v18",
                                  _app,
                                  "--v18");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--v1", "~", "--v2", "a~b", "--v3", "-5", "--v4", "+5", "--v5", "1e+5",
        "--v6", "1e-5", "--v7", ".", "--v8", "+", "--v9", "e5", "--v10", "0x10",
        "--v11", "0x0", "--v12", "0o10", "--v13", "0o9", "--v14", "0xg",
        "--v15", "0b2", "--v16", "\x01x", "--v17", "\"", "--v18", "\\"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W28",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

// WRITE_VALUE_STEPS
//   constant: the cases of d_tests_arg_config_write_values.
static const d_tests_arg_define_fn WRITE_VALUE_STEPS[] =
{
    d_tests_arg_config_w28
};

// WRITE_VALUE_COUNT
//   constant: how many cases WRITE_VALUE_STEPS holds.
static const size_t WRITE_VALUE_COUNT =
    sizeof(WRITE_VALUE_STEPS) / sizeof(WRITE_VALUE_STEPS[0]);

/*
d_tests_arg_config_write_values
  Values at the edges of what CLI11 writes bare, quoted, as a literal or as a
binary string.
  Tests the following:
  - the tilde, the last printable character
  - numbers signed, with signed exponents, and what only begins like one
  - numbers in other bases holding a zero, and digits their base does not have
  - a control character first, and one quote or backslash alone
*/
bool
d_tests_arg_config_write_values(void)
{
    return d_tests_arg_steps("WRITE_VALUES",
                             WRITE_VALUE_STEPS,
                             WRITE_VALUE_COUNT,
                             WRITE_VALUES);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 50. QUOTING AT THE EDGES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief W29: values at the length that decides, and holding quotes.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w29(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_option(_run,
                                  "e100",
                                  _app,
                                  "--e100");
    (void)d_tests_arg_make_option(_run,
                                  "q",
                                  _app,
                                  "--q");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--e100",
        "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
        "eeeeeeeeeeeeeeeeeeeeeeeeeeeee\n", "--q",
        "a 'x' b ''y\nqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq"
        "qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W29",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

// WRITE_QUOTING_STEPS
//   constant: the cases of d_tests_arg_config_write_quoting.
static const d_tests_arg_define_fn WRITE_QUOTING_STEPS[] =
{
    d_tests_arg_config_w29
};

// WRITE_QUOTING_COUNT
//   constant: how many cases WRITE_QUOTING_STEPS holds.
static const size_t WRITE_QUOTING_COUNT =
    sizeof(WRITE_QUOTING_STEPS) / sizeof(WRITE_QUOTING_STEPS[0]);

/*
d_tests_arg_config_write_quoting
  Values needing escapes at the length that decides their form, and a long one
holding quotes.
  Tests the following:
  - a value of 100 bytes needing escapes, still in quotes
  - a long one holding single quotes and pairs of them, as a literal
*/
bool
d_tests_arg_config_write_quoting(void)
{
    return d_tests_arg_steps("WRITE_QUOTING",
                             WRITE_QUOTING_STEPS,
                             WRITE_QUOTING_COUNT,
                             WRITE_QUOTING);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 51. THE TRIPLE QUOTE IN LONG VALUES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief W44: long values holding the triple quote.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w44(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_option(_run,
                                  "st",
                                  _app,
                                  "--st");
    (void)d_tests_arg_make_option(_run,
                                  "en",
                                  _app,
                                  "--en");
    (void)d_tests_arg_make_option(_run,
                                  "tq",
                                  _app,
                                  "--tq");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--st",
        "'''sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss"
        "sssssssssssssssssssssssssssssssss\n", "--en",
        "nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn"
        "nnnnnnnnnnnnnnnnnnnnnnnnnnnnnn\n'''", "--tq",
        "'''\"ttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttt"
        "ttttttttttttttttttttttttttttttttttt"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W44",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

// WRITE_TRIPLE_STEPS
//   constant: the cases of d_tests_arg_config_write_triples.
static const d_tests_arg_define_fn WRITE_TRIPLE_STEPS[] =
{
    d_tests_arg_config_w44
};

// WRITE_TRIPLE_COUNT
//   constant: how many cases WRITE_TRIPLE_STEPS holds.
static const size_t WRITE_TRIPLE_COUNT =
    sizeof(WRITE_TRIPLE_STEPS) / sizeof(WRITE_TRIPLE_STEPS[0]);

/*
d_tests_arg_config_write_triples
  Long values holding the triple quote, which a literal could not hold, written
as binary strings.
  Tests the following:
  - the triple quote starting a value, and ending one
  - the triple quote in a value with nothing that does not print
*/
bool
d_tests_arg_config_write_triples(void)
{
    return d_tests_arg_steps("WRITE_TRIPLES",
                             WRITE_TRIPLE_STEPS,
                             WRITE_TRIPLE_COUNT,
                             WRITE_TRIPLES);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 52. NAMES AT THE EDGES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief W30: names starting with a character that needs quotes.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w30(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_option(_run,
                                  "dot",
                                  _app,
                                  "--.a");
    (void)d_tests_arg_make_option(_run,
                                  "tick",
                                  _app,
                                  "--`b");
    (void)d_tests_arg_make_option(_run,
                                  "back",
                                  _app,
                                  "--\\c");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--.a", "1", "--`b", "2", "--\\c", "3"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W30",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

/**
 * @brief W31: names in brackets where arrays are in parentheses.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w31(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_config_parenthesized(_run,
                                           _app);
    (void)d_tests_arg_make_option(_run,
                                  "both",
                                  _app,
                                  "--[a]");
    (void)d_tests_arg_make_option(_run,
                                  "end",
                                  _app,
                                  "--a]");
    (void)d_tests_arg_make_option(_run,
                                  "start",
                                  _app,
                                  "--[b");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--[a]", "1", "--a]", "2", "--[b", "3"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W31",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

// WRITE_NAME_STEPS
//   constant: the cases of d_tests_arg_config_write_names.
static const d_tests_arg_define_fn WRITE_NAME_STEPS[] =
{
    d_tests_arg_config_w30,
    d_tests_arg_config_w31
};

// WRITE_NAME_COUNT
//   constant: how many cases WRITE_NAME_STEPS holds.
static const size_t WRITE_NAME_COUNT =
    sizeof(WRITE_NAME_STEPS) / sizeof(WRITE_NAME_STEPS[0]);

/*
d_tests_arg_config_write_names
  Names starting with a character that needs quotes, and names in brackets where
arrays are not.
  Tests the following:
  - names starting with the section separator, a backquote and a backslash
  - names in brackets, and half in them, for a format whose arrays are in
    parentheses
*/
bool
d_tests_arg_config_write_names(void)
{
    return d_tests_arg_steps("WRITE_NAMES",
                             WRITE_NAME_STEPS,
                             WRITE_NAME_COUNT,
                             WRITE_NAMES);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 53. FLAGS READ BACK
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief W32: flags read back by their names.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w32(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_tests_arg_make_flag(_run,
                                "nof",
                                _app,
                                "--no-f{false}");

    struct d_arg_option* const x1 =
        d_tests_arg_make_flag(_run,
                              "x1",
                              _app,
                              "--x1,--f1{1},--g1{2}");

    d_arg_option_set_disable_flag_override(x1,
                                           true);

    struct d_arg_option* const x2 =
        d_tests_arg_make_flag(_run,
                              "x2",
                              _app,
                              "--x2,--f2{1},--g2{2}");

    d_arg_option_set_disable_flag_override(x2,
                                           true);

    struct d_arg_option* const x3 =
        d_tests_arg_make_flag(_run,
                              "x3",
                              _app,
                              "--x3,--f3{1},--g3{2}");

    d_arg_option_set_disable_flag_override(x3,
                                           true);
    d_arg_option_set_multi_option_policy(x3,
                                         (int)D_ARG_MULTI_SUM);

    struct d_arg_option* const x4 =
        d_tests_arg_make_flag(_run,
                              "x4",
                              _app,
                              "--x4,--f4{1},--g4{2}");

    d_arg_option_set_disable_flag_override(x4,
                                           true);
    d_arg_option_set_multi_option_policy(x4,
                                         (int)D_ARG_MULTI_TAKE_ALL);

    struct d_arg_option* const x5 = d_tests_arg_make_flag(_run,
                                                          "x5",
                                                          _app,
                                                          "--x5,--f5{false}");

    d_arg_option_set_disable_flag_override(x5,
                                           true);

    struct d_arg_option* const o = d_tests_arg_make_option(_run,
                                                           "o",
                                                           _app,
                                                           "--o");

    d_arg_option_set_disable_flag_override(o,
                                           true);
    d_arg_option_set_multi_option_policy(o,
                                         (int)D_ARG_MULTI_TAKE_LAST);

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--no-f", "--f1", "--g2", "--f3", "--g3", "--f4", "--g4", "--f5", "--o",
        "a", "--o", "b"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W32",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

// WRITE_FLAG_STEPS
//   constant: the cases of d_tests_arg_config_write_flags.
static const d_tests_arg_define_fn WRITE_FLAG_STEPS[] =
{
    d_tests_arg_config_w32
};

// WRITE_FLAG_COUNT
//   constant: how many cases WRITE_FLAG_STEPS holds.
static const size_t WRITE_FLAG_COUNT =
    sizeof(WRITE_FLAG_STEPS) / sizeof(WRITE_FLAG_STEPS[0]);

/*
d_tests_arg_config_write_flags
  A flag's value as its names read it back.
  Tests the following:
  - a name whose default is false
  - names refusing an override: the one that accepts the value, and none, under
    Sum and TakeAll; one whose default is false
  - an option refusing an override, which has no names to read it
*/
bool
d_tests_arg_config_write_flags(void)
{
    return d_tests_arg_steps("WRITE_FLAGS",
                             WRITE_FLAG_STEPS,
                             WRITE_FLAG_COUNT,
                             WRITE_FLAGS);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 54. VALUES REVERSED
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief W33: values reversed.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w33(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const r1 = d_tests_arg_make_option(_run,
                                                            "r1",
                                                            _app,
                                                            "--r1");

    d_arg_option_set_expected_range(r1,
                                    1,
                                    3);
    d_arg_option_set_multi_option_policy(r1,
                                         (int)D_ARG_MULTI_REVERSE);

    struct d_arg_option* const r2 = d_tests_arg_make_option(_run,
                                                            "r2",
                                                            _app,
                                                            "--r2");

    d_arg_option_set_expected(r2,
                              2);
    d_arg_option_set_multi_option_policy(r2,
                                         (int)D_ARG_MULTI_REVERSE);

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--r1", "1", "2", "3", "--r2", "1", "2"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W33",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

// WRITE_ORDER_STEPS
//   constant: the cases of d_tests_arg_config_write_orders.
static const d_tests_arg_define_fn WRITE_ORDER_STEPS[] =
{
    d_tests_arg_config_w33
};

// WRITE_ORDER_COUNT
//   constant: how many cases WRITE_ORDER_STEPS holds.
static const size_t WRITE_ORDER_COUNT =
    sizeof(WRITE_ORDER_STEPS) / sizeof(WRITE_ORDER_STEPS[0]);

/*
d_tests_arg_config_write_orders
  Values an option reverses, written back in the order given.
  Tests the following:
  - three values of an option taking one to three, and two of one taking two
*/
bool
d_tests_arg_config_write_orders(void)
{
    return d_tests_arg_steps("WRITE_ORDERS",
                             WRITE_ORDER_STEPS,
                             WRITE_ORDER_COUNT,
                             WRITE_ORDERS);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 55. SUMS CHECKED AGAIN
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief W34: sums checked again.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w34(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const s1 = d_tests_arg_make_option(_run,
                                                            "s1",
                                                            _app,
                                                            "--s1");

    d_arg_option_set_expected(s1,
                              -1);
    d_arg_option_set_multi_option_policy(s1,
                                         (int)D_ARG_MULTI_SUM);

    struct d_arg_option* const s2 = d_tests_arg_make_option(_run,
                                                            "s2",
                                                            _app,
                                                            "--s2");

    d_arg_option_set_expected(s2,
                              -1);
    d_arg_option_set_multi_option_policy(s2,
                                         (int)D_ARG_MULTI_SUM);

    struct d_arg_option* const s3 = d_tests_arg_make_option(_run,
                                                            "s3",
                                                            _app,
                                                            "--s3");

    d_arg_option_set_expected(s3,
                              -1);
    d_arg_option_set_multi_option_policy(s3,
                                         (int)D_ARG_MULTI_SUM);

    struct d_arg_option* const s4 = d_tests_arg_make_option(_run,
                                                            "s4",
                                                            _app,
                                                            "--s4");

    d_arg_option_set_expected(s4,
                              -1);
    d_arg_option_set_multi_option_policy(s4,
                                         (int)D_ARG_MULTI_SUM);

    struct d_arg_validator range = d_arg_validator_range_long(0,
                                                              5);

    d_arg_validator_set_application_index(&range,
                                          0);
    (void)d_arg_option_check(s1,
                             range,
                             NULL);

    struct d_arg_validator bang =
        d_arg_validator_custom(d_tests_arg_config_bang,
                               NULL,
                               "bang");

    (void)d_arg_option_check(s2,
                             bang,
                             NULL);

    struct d_arg_validator doubled =
        d_arg_validator_custom(d_tests_arg_config_doubled,
                               NULL,
                               "doubled");

    (void)d_arg_option_transform(s3,
                                 doubled,
                                 NULL,
                                 NULL);

    struct d_arg_validator again =
        d_arg_validator_custom(d_tests_arg_config_doubled,
                               NULL,
                               "doubled");

    (void)d_arg_option_transform(s4,
                                 again,
                                 NULL,
                                 NULL);

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--s1", "3", "4", "--s2", "3", "4", "--s3", "3", "4", "--s4", "3"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W34",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

// WRITE_SUM_STEPS
//   constant: the cases of d_tests_arg_config_write_sums.
static const d_tests_arg_define_fn WRITE_SUM_STEPS[] =
{
    d_tests_arg_config_w34
};

// WRITE_SUM_COUNT
//   constant: how many cases WRITE_SUM_STEPS holds.
static const size_t WRITE_SUM_COUNT =
    sizeof(WRITE_SUM_STEPS) / sizeof(WRITE_SUM_STEPS[0]);

/*
d_tests_arg_config_write_sums
  Sums checked again before they are written.
  Tests the following:
  - a validator for the first value alone, refusing the sum
  - a validator refusing it with a message of one character
  - a transformation, given two values and one
*/
bool
d_tests_arg_config_write_sums(void)
{
    return d_tests_arg_steps("WRITE_SUMS",
                             WRITE_SUM_STEPS,
                             WRITE_SUM_COUNT,
                             WRITE_SUMS);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 56. JOINS AT THE EDGES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief W35: joins of empty values, and of one value transformed.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w35(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const j1 = d_tests_arg_make_option(_run,
                                                            "j1",
                                                            _app,
                                                            "--j1");

    d_arg_option_set_delimiter(j1,
                               ',');
    d_arg_option_set_multi_option_policy(j1,
                                         (int)D_ARG_MULTI_JOIN);

    struct d_arg_option* const j2 = d_tests_arg_make_option(_run,
                                                            "j2",
                                                            _app,
                                                            "--j2");

    d_arg_option_set_delimiter(j2,
                               ',');
    d_arg_option_set_multi_option_policy(j2,
                                         (int)D_ARG_MULTI_JOIN);

    struct d_arg_option* const j3 = d_tests_arg_make_option(_run,
                                                            "j3",
                                                            _app,
                                                            "--j3");

    d_arg_option_set_delimiter(j3,
                               ',');
    d_arg_option_set_multi_option_policy(j3,
                                         (int)D_ARG_MULTI_JOIN);

    struct d_arg_option* const j4 = d_tests_arg_make_option(_run,
                                                            "j4",
                                                            _app,
                                                            "--j4");

    d_arg_option_set_delimiter(j4,
                               ',');
    d_arg_option_set_multi_option_policy(j4,
                                         (int)D_ARG_MULTI_JOIN);

    struct d_arg_option* const j5 = d_tests_arg_make_option(_run,
                                                            "j5",
                                                            _app,
                                                            "--j5");

    d_arg_option_set_multi_option_policy(j5,
                                         (int)D_ARG_MULTI_JOIN);

    struct d_arg_validator one = d_arg_validator_custom(d_tests_arg_config_one,
                                                        NULL,
                                                        "one");

    (void)d_arg_option_transform(j5,
                                 one,
                                 NULL,
                                 NULL);

    struct d_arg_option* const j6 = d_tests_arg_make_option(_run,
                                                            "j6",
                                                            _app,
                                                            "--j6");

    d_arg_option_set_delimiter(j6,
                               ',');
    d_arg_option_set_multi_option_policy(j6,
                                         (int)D_ARG_MULTI_JOIN);

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--j1", "a", "--j1", "", "--j2", "a", "--j2", "", "--j2", "b", "--j3",
        "", "--j3", "", "--j3", "a", "--j4", "", "--j4", "", "--j5", "one",
        "--j6", "", "--j6", "", "--j6", ""
    };

    (void)d_tests_arg_config_written(_run,
                                     "W35",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

/**
 * @brief W36: delimiters set after the parse.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w36(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const j = d_tests_arg_make_option(_run,
                                                           "j",
                                                           _app,
                                                           "--j");

    d_arg_option_set_multi_option_policy(j,
                                         (int)D_ARG_MULTI_JOIN);

    struct d_arg_option* const j2 = d_tests_arg_make_option(_run,
                                                            "j2",
                                                            _app,
                                                            "--j2");

    d_arg_option_set_multi_option_policy(j2,
                                         (int)D_ARG_MULTI_JOIN);

    struct d_arg_option* const j3 = d_tests_arg_make_option(_run,
                                                            "j3",
                                                            _app,
                                                            "--j3");

    d_arg_option_set_multi_option_policy(j3,
                                         (int)D_ARG_MULTI_JOIN);

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--j", "a,b,c", "--j", "d", "--j2", ",a,b", "--j2", "c", "--j3", "a",
        "--j3", "b,"
    };

    (void)d_tests_arg_config_given(_run,
                                   "W36",
                                   ARGS,
                                   sizeof(ARGS) / sizeof(ARGS[0]));
    d_arg_option_set_delimiter(j,
                               ',');
    d_arg_option_set_delimiter(j2,
                               ',');
    d_arg_option_set_delimiter(j3,
                               ',');
    (void)d_tests_arg_config_write_all(_run,
                                       "");

    return;
}

// WRITE_JOIN_STEPS
//   constant: the cases of d_tests_arg_config_write_joins.
static const d_tests_arg_define_fn WRITE_JOIN_STEPS[] =
{
    d_tests_arg_config_w35,
    d_tests_arg_config_w36
};

// WRITE_JOIN_COUNT
//   constant: how many cases WRITE_JOIN_STEPS holds.
static const size_t WRITE_JOIN_COUNT =
    sizeof(WRITE_JOIN_STEPS) / sizeof(WRITE_JOIN_STEPS[0]);

/*
d_tests_arg_config_write_joins
  Joins of empty values, a delimiter set after the parse, and a join of one
value transformed.
  Tests the following:
  - empty values last, between others, first, and alone, and three of them,
    which join as the delimiter alone
  - one value transformed, written as transformed
  - delimiters set after the parse, which the joined values hold: as often as
    values were given, first, and last
*/
bool
d_tests_arg_config_write_joins(void)
{
    return d_tests_arg_steps("WRITE_JOINS",
                             WRITE_JOIN_STEPS,
                             WRITE_JOIN_COUNT,
                             WRITE_JOINS);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 57. DEFAULTS OF `{}`
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief W37: defaults that are `{}`.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w37(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const v = d_tests_arg_make_option(_run,
                                                           "v",
                                                           _app,
                                                           "--v");

    (void)d_arg_option_set_default_text(v,
                                        "{}");

    struct d_arg_option* const w = d_tests_arg_make_option(_run,
                                                           "w",
                                                           _app,
                                                           "--w");

    (void)d_arg_option_set_default_text(w,
                                        "[{},%%]");
    d_arg_option_set_expected(w,
                              -1);

    struct d_arg_option* const z = d_tests_arg_make_option(_run,
                                                           "z",
                                                           _app,
                                                           "--z");

    (void)d_arg_option_set_default_text(z,
                                        "{},%%");
    d_arg_option_set_delimiter(z,
                               ',');
    d_arg_option_set_expected(z,
                              -1);

    struct d_arg_option* const u = d_tests_arg_make_option(_run,
                                                           "u",
                                                           _app,
                                                           "--u");

    (void)d_arg_option_set_default_text(u,
                                        "[{},\"\"]");

    struct d_arg_option* const e = d_tests_arg_make_option(_run,
                                                           "e",
                                                           _app,
                                                           "--e");

    d_arg_option_set_expected_range(e,
                                    0,
                                    5);
    (void)d_arg_option_set_default_text(e,
                                        "{}");

    struct d_arg_option* const f = d_tests_arg_make_option(_run,
                                                           "f",
                                                           _app,
                                                           "--f");

    d_arg_option_set_expected_range(f,
                                    0,
                                    5);
    (void)d_arg_option_set_default_text(f,
                                        "[{},x]");
    (void)d_tests_arg_config_written(_run,
                                     "W37",
                                     NULL,
                                     0u,
                                     "");

    return;
}

/**
 * @brief W38: a default of more values than its option takes.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w38(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const v = d_tests_arg_make_option(_run,
                                                           "v",
                                                           _app,
                                                           "--v");

    (void)d_arg_option_set_default_text(v,
                                        "[{},x]");
    d_arg_option_set_expected(v,
                              -1);
    (void)d_tests_arg_config_written(_run,
                                     "W38",
                                     NULL,
                                     0u,
                                     "");

    return;
}

/**
 * @brief W45: a default of only the delimiter, every value kept.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w45(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const k = d_tests_arg_make_option(_run,
                                                           "k",
                                                           _app,
                                                           "--k");

    d_arg_option_set_delimiter(k,
                               ',');
    d_arg_option_set_multi_option_policy(k,
                                         (int)D_ARG_MULTI_TAKE_ALL);
    (void)d_arg_option_set_default_text(k,
                                        ",");
    (void)d_tests_arg_config_written(_run,
                                     "W45",
                                     NULL,
                                     0u,
                                     "");

    return;
}

/**
 * @brief W46: a default of only the delimiter, one value needed.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w46(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const k = d_tests_arg_make_option(_run,
                                                           "k",
                                                           _app,
                                                           "--k");

    d_arg_option_set_delimiter(k,
                               ',');
    (void)d_arg_option_set_default_text(k,
                                        ",");
    (void)d_tests_arg_config_written(_run,
                                     "W46",
                                     NULL,
                                     0u,
                                     "");

    return;
}

// WRITE_DEFAULT_STEPS
//   constant: the cases of d_tests_arg_config_write_defaults.
static const d_tests_arg_define_fn WRITE_DEFAULT_STEPS[] =
{
    d_tests_arg_config_w37,
    d_tests_arg_config_w38,
    d_tests_arg_config_w45,
    d_tests_arg_config_w46
};

// WRITE_DEFAULT_COUNT
//   constant: how many cases WRITE_DEFAULT_STEPS holds.
static const size_t WRITE_DEFAULT_COUNT =
    sizeof(WRITE_DEFAULT_STEPS) / sizeof(WRITE_DEFAULT_STEPS[0]);

/*
d_tests_arg_config_write_defaults
  Defaults that are `{}`, one with more values than its option takes, and one of
only the delimiter.
  Tests the following:
  - `{}` alone, in brackets with a separator, split from one, and with an empty
    value
  - for an option taking several, `{}` alone, which is nothing, and `{}` with a
    value
  - more values than the option takes, refused as the writing reads them
  - a default of only the delimiter, which splits into no values: nothing
    written where every value is kept, and refused where one is needed
*/
bool
d_tests_arg_config_write_defaults(void)
{
    return d_tests_arg_steps("WRITE_DEFAULTS",
                             WRITE_DEFAULT_STEPS,
                             WRITE_DEFAULT_COUNT,
                             WRITE_DEFAULTS);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 58. GROUPS AND PREFIXES AT THE EDGES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief W39: groups named with one character.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w39(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const a = d_arg_command_add_option(_app,
                                                            "--a",
                                                            "adesc");

    (void)d_tests_arg_watch_option(_run,
                                   "a",
                                   a);
    (void)d_arg_option_set_group(a,
                                 "G");

    struct d_arg_command* const h = d_arg_command_add_option_group(_app,
                                                                   "H",
                                                                   "hdesc");
    struct d_arg_option* const  b = d_arg_command_add_option(h,
                                                             "--b",
                                                             "bdesc");

    (void)d_tests_arg_watch_option(_run,
                                   "b",
                                   b);

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--a", "1", "--b", "2"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W39",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

/**
 * @brief W40: a group whose first option is the command's first.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w40(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    (void)d_arg_command_set_help_flag(_app,
                                      NULL,
                                      NULL);

    struct d_arg_option* const a = d_tests_arg_make_option(_run,
                                                           "a",
                                                           _app,
                                                           "--a");

    (void)d_arg_option_set_group(a,
                                 "G");
    (void)d_tests_arg_make_option(_run,
                                  "b",
                                  _app,
                                  "--b");

    struct d_arg_option* const c = d_tests_arg_make_option(_run,
                                                           "c",
                                                           _app,
                                                           "--c");

    (void)d_arg_option_set_group(c,
                                 "G");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--a", "1", "--b", "2", "--c", "3"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W40",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

/**
 * @brief W41: an option group not used.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w41(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const og = d_arg_command_add_option_group(_app,
                                                                    "og",
                                                                    "ogdesc");
    struct d_arg_option* const  x  = d_tests_arg_make_option(_run,
                                                             "x",
                                                             og,
                                                             "--x");

    (void)d_arg_option_set_default_text(x,
                                        "7");
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--a", "1"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W41",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

/**
 * @brief W42: a prefix through a subcommand that is not configurable.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w42(
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

    d_arg_command_set_configurable(t,
                                   true);
    (void)d_tests_arg_make_option(_run,
                                  "y",
                                  t,
                                  "--y");
    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  s,
                                  "--x");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "s", "--x", "1", "t", "--y", "2"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W42",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "p.");

    return;
}

/**
 * @brief W43: a prefix of one character through a nameless subcommand.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w43(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_command* const n = d_arg_command_add_subcommand(_app,
                                                                 "",
                                                                 "nl");
    struct d_arg_command* const t = d_tests_arg_make_subcommand(_run,
                                                                n,
                                                                "t");

    d_arg_command_set_configurable(t,
                                   true);
    (void)d_tests_arg_make_option(_run,
                                  "y",
                                  t,
                                  "--y");
    (void)d_tests_arg_make_option(_run,
                                  "x",
                                  n,
                                  "--x");

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--x", "1", "t", "--y", "2"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W43",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "p");

    return;
}

// WRITE_LAYOUT_STEPS
//   constant: the cases of d_tests_arg_config_write_layout.
static const d_tests_arg_define_fn WRITE_LAYOUT_STEPS[] =
{
    d_tests_arg_config_w39,
    d_tests_arg_config_w40,
    d_tests_arg_config_w41,
    d_tests_arg_config_w42,
    d_tests_arg_config_w43
};

// WRITE_LAYOUT_COUNT
//   constant: how many cases WRITE_LAYOUT_STEPS holds.
static const size_t WRITE_LAYOUT_COUNT =
    sizeof(WRITE_LAYOUT_STEPS) / sizeof(WRITE_LAYOUT_STEPS[0]);

/*
d_tests_arg_config_write_layout
  Groups named with one character, a group whose first option is the command's
first, an option group not used, and prefixes through subcommands.
  Tests the following:
  - a group of options and an option group, each named with one character, with
    descriptions
  - a group whose first option is the first of a command without a help flag
  - an option group not used, written by the modes for defaults
  - a prefix through a subcommand that is not configurable, and one of one
    character through a nameless subcommand
*/
bool
d_tests_arg_config_write_layout(void)
{
    return d_tests_arg_steps("WRITE_LAYOUT",
                             WRITE_LAYOUT_STEPS,
                             WRITE_LAYOUT_COUNT,
                             WRITE_LAYOUT);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 59. DEPARTED: MOVED OPTIONS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief W20: options moved into groups.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_w20(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const  moved = d_tests_arg_make_option(_run,
                                                                "moved",
                                                                _app,
                                                                "--moved");
    struct d_arg_command* const grp   = d_tests_arg_make_group(_run,
                                                               _app,
                                                               "grp");

    (void)d_arg_command_group_add_option(grp,
                                         moved);

    struct d_arg_command* const pg     = d_arg_command_add_option_group(_app,
                                                                        "+pg",
                                                                        NULL);
    struct d_arg_option* const  moved2 = d_tests_arg_make_option(_run,
                                                                 "moved2",
                                                                 _app,
                                                                 "--moved2");

    (void)d_arg_command_group_add_option(pg,
                                         moved2);

    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--moved", "1", "--moved2", "2"
    };

    (void)d_tests_arg_config_written(_run,
                                     "W20",
                                     ARGS,
                                     sizeof(ARGS) / sizeof(ARGS[0]),
                                     "");

    return;
}

// DEPARTED_MOVED_STEPS
//   constant: the cases of d_tests_arg_config_departed_moved.
static const d_tests_arg_define_fn DEPARTED_MOVED_STEPS[] =
{
    d_tests_arg_config_w20
};

// DEPARTED_MOVED_COUNT
//   constant: how many cases DEPARTED_MOVED_STEPS holds.
static const size_t DEPARTED_MOVED_COUNT =
    sizeof(DEPARTED_MOVED_STEPS) / sizeof(DEPARTED_MOVED_STEPS[0]);

/*
d_tests_arg_config_departed_moved
  Options moved into option groups, which the groups write (ISSUES 6).
  Tests the following:
  - an option moved into an option group, and into one shown with its parent
*/
bool
d_tests_arg_config_departed_moved(void)
{
    return d_tests_arg_steps("DEPARTED_MOVED",
                             DEPARTED_MOVED_STEPS,
                             DEPARTED_MOVED_COUNT,
                             DEPARTED_MOVED);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 60. FORMATS
//==============================================================================

/**
 * @brief Whether a format holds the characters given, in the order of its
 *        fields: comment, array start, array end, array separator, value
 *        delimiter, string quote, literal quote, parent separator.
 *
 * @param[in] _format      the format.
 * @param[in] _characters  the eight characters expected; may hold NULs.
 * @return whether each is the format's.
 */
static bool
d_tests_arg_config_marks(
    const struct d_arg_config_format* _format,
    const char*                       _characters
)
{
    const char held[8] =
    {
        _format->comment,
        _format->array_start,
        _format->array_end,
        _format->array_separator,
        _format->value_delimiter,
        _format->string_quote,
        _format->literal_quote,
        _format->parent_separator
    };

    return (memcmp(held,
                   _characters,
                   sizeof held) == 0);
}

/**
 * @brief Whether a format is as d_arg_config_format_init makes it, but for
 *        its characters: CLI11's own operations, every section at every
 *        index, 255 layers, no switch on.
 *
 * @param[in] _format  the format.
 * @return whether it is.
 */
static bool
d_tests_arg_config_plain(
    const struct d_arg_config_format* _format
)
{
    return ( (_format->vtable == NULL)     &&
             (_format->context == NULL)    &&
             (_format->section == NULL)    &&
             (_format->index == -1)        &&
             (_format->max_layers == 255)  &&
             (_format->flags == 0u) );
}

/**
 * @brief The formats CLI11's ConfigTOML and ConfigINI are, and what release
 *        leaves of one.
 *
 * @return whether every check held.
 */
static bool
d_tests_arg_config_made_formats(void)
{
    struct d_arg_config_format toml = { 0 };
    struct d_arg_config_format ini  = { 0 };

    d_arg_config_format_init(&toml);
    d_arg_config_format_init_ini(&ini);

    const bool made = ( (d_tests_arg_config_held(
                             (d_tests_arg_config_plain(&toml)) &&
                             (d_tests_arg_config_marks(&toml,
                                                       "#[],=\"'.")),
                             "init: CLI11's ConfigTOML")) &&
                        (d_tests_arg_config_held(
                             (d_tests_arg_config_plain(&ini)) &&
                             (d_tests_arg_config_marks(&ini,
                                                       ";\0\0 =\"'.")),
                             "init_ini: CLI11's ConfigINI")) );
    const int  named = d_arg_config_format_set_section(&ini,
                                                       "sub");

    d_arg_config_format_set_flag(&ini,
                                 D_ARG_CONFIG_COMMENT_DEFAULTS,
                                 true);
    d_arg_config_format_release(&ini);

    const bool released = d_tests_arg_config_held(
                              (named == D_ARG_OK)                     &&
                              (d_tests_arg_config_plain(&ini))        &&
                              (d_tests_arg_config_marks(&ini,
                                                        "#[],=\"'.")),
                              "release: ConfigTOML again, the section "
                              "freed");

    d_arg_config_format_release(&toml);

    return ( (made) &&
             (released) );
}

/**
 * @brief A format's section: copied, replaced, cleared by "" and by NULL,
 *        and refused without a format.
 *
 * @return whether every check held.
 */
static bool
d_tests_arg_config_sections_set(void)
{
    struct d_arg_config_format format = { 0 };
    char                       name[] = "sub";

    d_arg_config_format_init(&format);

    const int  first  = d_arg_config_format_set_section(&format,
                                                        name);
    const bool copied = ( (first == D_ARG_OK)          &&
                          (format.section != NULL)     &&
                          (format.section != name)     &&
                          (strcmp(format.section,
                                  "sub") == 0) );
    const int  again  = d_arg_config_format_set_section(&format,
                                                        "other");
    const bool other  = ( (again == D_ARG_OK)          &&
                          (format.section != NULL)     &&
                          (strcmp(format.section,
                                  "other") == 0) );
    const int  empty  = d_arg_config_format_set_section(&format,
                                                        "");
    const bool none   = ( (empty == D_ARG_OK)  &&
                          (format.section == NULL) );
    const int  named  = d_arg_config_format_set_section(&format,
                                                        "x");
    const int  absent = d_arg_config_format_set_section(&format,
                                                        NULL);
    const bool clear  = ( (named == D_ARG_OK)      &&
                          (absent == D_ARG_OK)     &&
                          (format.section == NULL) );
    const int  nobody = d_arg_config_format_set_section(NULL,
                                                        "x");

    d_arg_config_format_release(&format);

    return ( (d_tests_arg_config_held(copied,
                                      "a section copied")) &&
             (d_tests_arg_config_held(other,
                                      "a section replaced")) &&
             (d_tests_arg_config_held(none,
                                      "\"\" for every section")) &&
             (d_tests_arg_config_held(clear,
                                      "NULL for every section")) &&
             (d_tests_arg_config_held(nobody == D_ARG_ERROR_BAD_ARGUMENT,
                                      "a section for no format")) );
}

/**
 * @brief The layers, held from 0 to 255, and the index, as given.
 *
 * @return whether each was stored so.
 */
static bool
d_tests_arg_config_layers_set(void)
{
    struct d_arg_config_format format = { 0 };

    d_arg_config_format_init(&format);
    d_arg_config_format_set_max_layers(&format,
                                       300);

    const int high = format.max_layers;

    d_arg_config_format_set_max_layers(&format,
                                       -1);

    const int low = format.max_layers;

    d_arg_config_format_set_max_layers(&format,
                                       7);
    d_arg_config_format_set_index(&format,
                                  3);

    const bool layered = ( (high == 255)              &&
                           (low == 0)                 &&
                           (format.max_layers == 7)   &&
                           (format.index == 3) );

    d_arg_config_format_release(&format);

    return d_tests_arg_config_held(layered,
                                   "layers held from 0 to 255, an index as "
                                   "given");
}

/**
 * @brief The switches, each on and off, other bits ignored; and each
 *        setter given no format, which it ignores.
 *
 * @return whether the switches were stored so.
 */
static bool
d_tests_arg_config_switches_set(void)
{
    struct d_arg_config_format format = { 0 };

    d_arg_config_format_init(&format);
    d_arg_config_format_set_flag(&format,
                                 D_ARG_CONFIG_COMMENT_DEFAULTS,
                                 true);
    d_arg_config_format_set_flag(&format,
                                 D_ARG_CONFIG_ALLOW_DUPLICATE_FIELDS,
                                 true);

    const uint32_t both = format.flags;

    d_arg_config_format_set_flag(&format,
                                 D_ARG_CONFIG_COMMENT_DEFAULTS,
                                 false);
    d_arg_config_format_set_flag(&format,
                                 0x80u,
                                 true);

    const bool switched = ( (both == (D_ARG_CONFIG_COMMENT_DEFAULTS |
                                      D_ARG_CONFIG_ALLOW_DUPLICATE_FIELDS)) &&
                            (format.flags ==
                             D_ARG_CONFIG_ALLOW_DUPLICATE_FIELDS) );

    // each call given no format, which it ignores
    d_arg_config_format_init(NULL);
    d_arg_config_format_init_ini(NULL);
    d_arg_config_format_set_index(NULL,
                                  1);
    d_arg_config_format_set_max_layers(NULL,
                                       1);
    d_arg_config_format_set_flag(NULL,
                                 D_ARG_CONFIG_COMMENT_DEFAULTS,
                                 true);
    d_arg_config_format_release(NULL);
    d_arg_config_format_release(&format);

    return d_tests_arg_config_held(switched,
                                   "the switches, other bits ignored");
}

/**
 * @brief Which format a command reads and writes with: its own, its
 *        nearest ancestor's, its parser's; a subcommand made while its
 *        parent had one of its own sharing it, as CLI11's does, and one made
 *        before following its parent's; none for no command.
 *
 * @param[in,out] _run    the run.
 * @param[in,out] _first  a format of the caller's, borrowed while this runs.
 * @param[in,out] _later  another.
 * @return whether every check held.
 */
static bool
d_tests_arg_config_resolution(
    struct d_tests_arg_run*     _run,
    struct d_arg_config_format* _first,
    struct d_arg_config_format* _later
)
{
    struct d_arg_command* const             app    = &_run->parser.command;
    const struct d_arg_config_format* const parser =
        &_run->parser.config_format;
    struct d_arg_command* const             before =
        d_arg_command_add_subcommand(app,
                                     "before",
                                     NULL);
    const bool                              own    =
        ( (d_arg_command_config_format(app) == parser)    &&
          (d_arg_command_config_format(before) == parser) );

    d_arg_command_set_config_format(app,
                                    _first);

    struct d_arg_command* const after = d_arg_command_add_subcommand(app,
                                                                     "after",
                                                                     NULL);

    d_arg_command_set_config_format(app,
                                    _later);
    d_arg_command_set_config_format(NULL,
                                    _later);

    const bool shared  = ( (d_arg_command_config_format(app) == _later)    &&
                           (d_arg_command_config_format(before) == _later) &&
                           (d_arg_command_config_format(after) == _first) );
    const bool nothing = (d_arg_command_config_format(NULL) == NULL);

    d_arg_command_set_config_format(app,
                                    NULL);
    d_arg_command_set_config_format(after,
                                    NULL);

    return ( (d_tests_arg_config_held(own,
                                      "the parser's, for the root and a "
                                      "subcommand")) &&
             (d_tests_arg_config_held(shared,
                                      "the parent's, and one shared when "
                                      "made")) &&
             (d_tests_arg_config_held(nothing,
                                      "no command's")) );
}

/*
d_tests_arg_config_formats
  A format's own settings, beyond what reading and writing show of them; in
every build.
  Tests the following:
  - CLI11's ConfigTOML and ConfigINI as made, and a format released
  - a section copied, replaced and cleared
  - layers held from 0 to 255, an index as given, the switches
  - each setter given no format
  - the format a command reads and writes with
*/
bool
d_tests_arg_config_formats(void)
{
    struct d_arg_config_format  first = { 0 };
    struct d_arg_config_format  later = { 0 };
    struct d_tests_arg_run      run   = { 0 };
    struct d_arg_command* const app   = d_tests_arg_begin(&run);

    d_arg_config_format_init(&first);
    d_arg_config_format_init_ini(&later);

    const bool made       = d_tests_arg_config_made_formats();
    const bool sections   = d_tests_arg_config_sections_set();
    const bool settings   = ( (d_tests_arg_config_layers_set()) &&
                              (d_tests_arg_config_switches_set()) );
    const bool resolution = ( (app != NULL) &&
                              (d_tests_arg_config_resolution(&run,
                                                             &first,
                                                             &later)) );
    const bool held       = ( (made)       &&
                              (sections)   &&
                              (settings)   &&
                              (resolution) &&
                              (!run.failed) );

    d_tests_arg_end(&run);
    d_arg_config_format_release(&first);
    d_arg_config_format_release(&later);

    return held;
}

//==============================================================================
// 61. BUILDING ITEMS
//==============================================================================

/**
 * @brief An item built by the calls a format of the caller's builds with:
 *        a name given twice, the last kept; NUL bytes kept; NULL data
 *        without a length read as the empty text; and the item released.
 *
 * @return whether every check held.
 */
static bool
d_tests_arg_config_built(void)
{
    struct d_arg_config_item item   = D_ARG_CONFIG_ITEM_INIT;
    const int                first  = d_arg_config_item_set_name(
                                          &item,
                                          d_pack_text_from_cstr("x"));
    const int                named  = d_arg_config_item_set_name(
                                          &item,
                                          d_pack_text_from_span("n\0m",
                                                                3u));
    const int                parent = d_arg_config_item_add_parent(
                                          &item,
                                          d_pack_text_from_cstr("s"));
    const int                empty  = d_arg_config_item_add_parent(
                                          &item,
                                          d_pack_text_from_span(NULL,
                                                                0u));
    const int                input  = d_arg_config_item_add_input(
                                          &item,
                                          d_pack_text_from_span("a\0b",
                                                                3u));
    const bool               made   =
        ( (first == D_ARG_OK)                       &&
          (named == D_ARG_OK)                       &&
          (parent == D_ARG_OK)                      &&
          (empty == D_ARG_OK)                       &&
          (input == D_ARG_OK)                       &&
          (item.name.length == 3u)                  &&
          (memcmp(item.name.data,
                  "n\0m",
                  3u) == 0)                         &&
          (item.parents.count == 2u)                &&
          (item.parents.items[1].length == 0u)      &&
          (item.inputs.count == 1u)                 &&
          (item.inputs.items[0].length == 3u)       &&
          (memcmp(item.inputs.items[0].data,
                  "a\0b",
                  3u) == 0) );

    d_arg_config_item_release(&item);
    d_arg_config_item_release(NULL);

    const bool released = ( (item.name.length == 0u)    &&
                            (item.parents.count == 0u)  &&
                            (item.inputs.count == 0u)   &&
                            (item.storage.count == 0u)  &&
                            (!item.multiline) );

    return ( (d_tests_arg_config_held(made,
                                      "an item's name, sections and "
                                      "values, NUL bytes kept")) &&
             (d_tests_arg_config_held(released,
                                      "an item released")) );
}

/**
 * @brief A part of a length but no data refused, the item left as it was;
 *        and nothing done for no item.
 *
 * @return whether every call was refused.
 */
static bool
d_tests_arg_config_parts_refused(void)
{
    const struct d_pack_text none   = { NULL, 1u };
    struct d_arg_config_item item   = D_ARG_CONFIG_ITEM_INIT;
    const int                named  = d_arg_config_item_set_name(
                                          &item,
                                          d_pack_text_from_cstr("n"));
    const int                parent = d_arg_config_item_add_parent(&item,
                                                                   none);
    const int                input  = d_arg_config_item_add_input(&item,
                                                                  none);
    const int                again  = d_arg_config_item_set_name(&item,
                                                                 none);
    const bool               held   =
        ( (named == D_ARG_OK)                                   &&
          (parent == D_ARG_ERROR_BAD_ARGUMENT)                  &&
          (input == D_ARG_ERROR_BAD_ARGUMENT)                   &&
          (again == D_ARG_ERROR_BAD_ARGUMENT)                   &&
          (item.name.length == 1u)                              &&
          (item.parents.count == 0u)                            &&
          (item.inputs.count == 0u)                             &&
          (d_arg_config_item_set_name(NULL,
                                      item.name) ==
           D_ARG_ERROR_BAD_ARGUMENT)                            &&
          (d_arg_config_item_add_parent(NULL,
                                        item.name) ==
           D_ARG_ERROR_BAD_ARGUMENT)                            &&
          (d_arg_config_item_add_input(NULL,
                                       item.name) ==
           D_ARG_ERROR_BAD_ARGUMENT) );

    d_arg_config_item_release(&item);

    return d_tests_arg_config_held(held,
                                   "a part of a length but no data, and no "
                                   "item, refused");
}

/**
 * @brief Items adopted by a list, which grows past any first room and
 *        leaves each item empty; adoption refused without a list or an
 *        item; and the list released.
 *
 * @return whether every check held.
 */
static bool
d_tests_arg_config_adopted(void)
{
    struct d_arg_config_item_list list    = D_ARG_LIST_INIT;
    struct d_arg_config_item      item    = D_ARG_CONFIG_ITEM_INIT;
    bool                          adopted = true;

    // twenty items, each named by a letter in turn
    for (size_t index = 0u; index < 20u; ++index)
    {
        const char letter = (char)('a' + (int)index);
        const int  named  = d_arg_config_item_set_name(
                                &item,
                                d_pack_text_from_span(&letter,
                                                      1u));
        const int  taken  = d_arg_config_item_list_adopt(&list,
                                                         &item);

        adopted = ( (named == D_ARG_OK)        &&
                    (taken == D_ARG_OK)        &&
                    (item.name.length == 0u)   &&
                    (item.storage.count == 0u) &&
                    (adopted) );
    }

    const bool kept    = ( (list.count == 20u)                  &&
                           (list.items[19].name.length == 1u)   &&
                           (list.items[19].name.data[0] == 't') );
    const int  named   = d_arg_config_item_set_name(&item,
                                                    d_pack_text_from_cstr(
                                                        "z"));
    const int  no_list = d_arg_config_item_list_adopt(NULL,
                                                      &item);
    const int  no_item = d_arg_config_item_list_adopt(&list,
                                                      NULL);
    const bool refused = ( (named == D_ARG_OK)                     &&
                           (no_list == D_ARG_ERROR_BAD_ARGUMENT)   &&
                           (no_item == D_ARG_ERROR_BAD_ARGUMENT)   &&
                           (item.name.length == 1u)                &&
                           (list.count == 20u) );

    d_arg_config_item_release(&item);
    d_arg_config_item_list_release(&list);
    d_arg_config_item_list_release(NULL);

    return ( (d_tests_arg_config_held(adopted && kept,
                                      "twenty items adopted, each left "
                                      "empty")) &&
             (d_tests_arg_config_held(refused,
                                      "adoption without a list or an "
                                      "item")) &&
             (d_tests_arg_config_held(
                  ( (list.count == 0u)      &&
                    (list.capacity == 0u)   &&
                    (list.items == NULL) ),
                  "a list released")) );
}

/**
 * @brief A to_flag of the caller's: the text the format's context holds,
 *        whatever the item.
 *
 * @param[in]     _format  the format; its context a text.
 * @param[in]     _item    the item; unused.
 * @param[in,out] _out     receives the text.
 * @return the buffer's status.
 */
static int
d_tests_arg_config_flag_text(
    const struct d_arg_config_format* _format,
    const struct d_arg_config_item*   _item,
    struct d_arg_buffer*              _out
)
{
    (void)_item;

    return d_arg_buffer_append_cstr(_out,
                                    (const char*)_format->context);
}

/**
 * @brief A full name and a flag's text appended to what a buffer holds: a
 *        name's own `.` dropped only when it ends what was appended; a
 *        flag's text through a table's to_flag, which sees the format's
 *        context, and through CLI11's own, given no format.
 *
 * @return whether the buffer came to what it should.
 */
static bool
d_tests_arg_config_appended(void)
{
    // OWN_FLAG
    //   constant: a table whose to_flag is the caller's, the rest CLI11's.
    static const struct d_arg_config_format_vtable OWN_FLAG =
    {
        .to_flag = d_tests_arg_config_flag_text
    };

    struct d_arg_config_format format = { 0 };
    struct d_arg_config_item   item   = D_ARG_CONFIG_ITEM_INIT;
    struct d_arg_buffer        text   = D_ARG_BUFFER_INIT;
    char                       yes[]  = "yes";

    d_arg_config_format_init(&format);
    format.vtable  = &OWN_FLAG;
    format.context = yes;
    (void)d_arg_buffer_append_cstr(&text,
                                   "x.");

    const int  empty   = d_arg_config_item_fullname(&item,
                                                    &text);
    const int  section = d_arg_config_item_add_parent(
                             &item,
                             d_pack_text_from_cstr("a"));
    const int  dotted  = d_arg_config_item_fullname(&item,
                                                    &text);
    const int  own     = d_arg_config_to_flag(&format,
                                              &item,
                                              &text);
    const int  plain   = d_arg_config_to_flag_default(NULL,
                                                      &item,
                                                      &text);
    const bool same    = d_tests_arg_same("a full name and flags appended",
                                          d_arg_buffer_text(&text),
                                          "x.ayes{}");

    d_arg_config_item_release(&item);
    d_arg_buffer_release(&text);

    return ( (empty == D_ARG_OK)    &&
             (section == D_ARG_OK)  &&
             (dotted == D_ARG_OK)   &&
             (own == D_ARG_OK)      &&
             (plain == D_ARG_OK)    &&
             (same) );
}

/**
 * @brief A full name and a flag's text refused without an item, a buffer
 *        or a format, and nothing written.
 *
 * @return whether every call was refused.
 */
static bool
d_tests_arg_config_names_refused(void)
{
    struct d_arg_config_format format = { 0 };
    struct d_arg_config_item   item   = D_ARG_CONFIG_ITEM_INIT;
    struct d_arg_buffer        text   = D_ARG_BUFFER_INIT;

    d_arg_config_format_init(&format);

    const bool refused =
        ( (d_arg_config_item_fullname(NULL,
                                      &text) == D_ARG_ERROR_BAD_ARGUMENT)  &&
          (d_arg_config_item_fullname(&item,
                                      NULL) == D_ARG_ERROR_BAD_ARGUMENT)   &&
          (d_arg_config_to_flag(NULL,
                                &item,
                                &text) == D_ARG_ERROR_BAD_ARGUMENT)        &&
          (d_arg_config_to_flag(&format,
                                NULL,
                                &text) == D_ARG_ERROR_BAD_ARGUMENT)        &&
          (d_arg_config_to_flag(&format,
                                &item,
                                NULL) == D_ARG_ERROR_BAD_ARGUMENT)         &&
          (d_arg_config_to_flag_default(&format,
                                        NULL,
                                        &text) ==
           D_ARG_ERROR_BAD_ARGUMENT)                                       &&
          (d_arg_config_to_flag_default(&format,
                                        &item,
                                        NULL) == D_ARG_ERROR_BAD_ARGUMENT) &&
          (text.length == 0u) );

    d_arg_buffer_release(&text);
    d_arg_config_format_release(&format);

    return d_tests_arg_config_held(refused,
                                   "a full name and a flag without an item, "
                                   "a buffer or a format");
}

/*
d_tests_arg_config_building
  Items as a format of the caller's makes them, and the calls that read
them; in every build.
  Tests the following:
  - a name, sections and values copied in, NUL bytes kept, the item
    released
  - a part of a length but no data, and no item, refused
  - items adopted by a list that grows, each left empty, adoption refused,
    and the list released
  - a full name and a flag's text appended to a buffer, through a table's
    to_flag and CLI11's own
  - a full name and a flag's text refused without an item, a buffer or a
    format
*/
bool
d_tests_arg_config_building(void)
{
    const bool built    = d_tests_arg_config_built();
    const bool parts    = d_tests_arg_config_parts_refused();
    const bool adopted  = d_tests_arg_config_adopted();
    const bool appended = d_tests_arg_config_held(
                              d_tests_arg_config_appended(),
                              "a full name and flags, appended");
    const bool refused  = d_tests_arg_config_names_refused();

    return ( (built)    &&
             (parts)    &&
             (adopted)  &&
             (appended) &&
             (refused) );
}

//==============================================================================
// 62. TABLES OF OPERATIONS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief One line of the caller's format: `!` alone refuses the text,
 *        described by the table's context; anything else is an item named
 *        by what comes before its first space, valued by what follows it.
 *
 * @param[in]     _table  the table's context.
 * @param[in]     _line   the line, without its newline.
 * @param[in,out] _out    receives the item, appended.
 * @param[in,out] _error  receives the refusal's description.
 * @return D_ARG_OK, D_ARG_ERROR_PARSE for `!`, or what refused the item.
 */
static int
d_tests_arg_config_line_item(
    const struct d_tests_arg_config_table* _table,
    struct d_pack_text                     _line,
    struct d_arg_config_item_list*         _out,
    struct d_arg_buffer*                   _error
)
{
    // the line that refuses the text
    if ( (_line.length == 1u) &&
         (_line.data[0] == '!') )
    {
        (void)d_arg_buffer_append_cstr(_error,
                                       _table->refusal);

        return D_ARG_ERROR_PARSE;
    }

    const char* const        space = memchr(_line.data,
                                            ' ',
                                            _line.length);
    const size_t             name  = (space != NULL)
                                     ? (size_t)(space - _line.data)
                                     : _line.length;
    struct d_arg_config_item item  = D_ARG_CONFIG_ITEM_INIT;
    int                      made  = d_arg_config_item_set_name(
                                         &item,
                                         d_pack_text_from_span(_line.data,
                                                               name));

    // the value, when there is one
    if ( (made == D_ARG_OK) &&
         (space != NULL) )
    {
        made = d_arg_config_item_add_input(
                   &item,
                   d_pack_text_from_span(space + 1,
                                         _line.length - name - 1u));
    }

    made = (made == D_ARG_OK)
           ? d_arg_config_item_list_adopt(_out,
                                          &item)
           : made;
    d_arg_config_item_release(&item);

    return made;
}

/**
 * @brief A from_config of the caller's: each line of the text read by
 *        d_tests_arg_config_line_item, until one refuses it; the call
 *        counted in the format's context.
 *
 * @param[in]     _format  the format; its context the table's.
 * @param[in]     _input   the text.
 * @param[in,out] _out     receives the items, appended.
 * @param[in,out] _error   receives a refusal's description.
 * @return the first status that is not D_ARG_OK, or D_ARG_OK.
 */
static int
d_tests_arg_config_lines(
    const struct d_arg_config_format* _format,
    struct d_pack_text                _input,
    struct d_arg_config_item_list*    _out,
    struct d_arg_buffer*              _error
)
{
    struct d_tests_arg_config_table* const table  = _format->context;
    size_t                                 start  = 0u;
    int                                    status = D_ARG_OK;

    ++table->calls;

    // each line, until one refuses the text
    while ( (status == D_ARG_OK) &&
            (start < _input.length) )
    {
        const char* const from = _input.data + start;
        const char* const end  = memchr(from,
                                        '\n',
                                        _input.length - start);
        const size_t      size = (end != NULL)
                                 ? (size_t)(end - from)
                                 : (_input.length - start);

        status = d_tests_arg_config_line_item(table,
                                              d_pack_text_from_span(from,
                                                                    size),
                                              _out,
                                              _error);
        start += size + 1u;
    }

    return status;
}

/**
 * @brief A to_config of the caller's: one line, `<prefix>|<mode>|<1 when
 *        described>|<the command's name>`; the call counted in the
 *        format's context.
 *
 * @param[in]     _format             the format; its context the table's.
 * @param[in]     _command            the command written.
 * @param[in]     _mode               the output mode.
 * @param[in]     _write_description  whether descriptions are written.
 * @param[in]     _prefix             what precedes each name.
 * @param[in,out] _out                receives the line.
 * @return the buffer's status.
 */
static int
d_tests_arg_config_summary(
    const struct d_arg_config_format* _format,
    const struct d_arg_command*       _command,
    int                               _mode,
    bool                              _write_description,
    struct d_pack_text                _prefix,
    struct d_arg_buffer*              _out
)
{
    struct d_tests_arg_config_table* const table = _format->context;

    ++table->calls;

    return d_arg_buffer_printf(_out,
                               "%.*s|%d|%d|%s\n",
                               (int)_prefix.length,
                               (_prefix.data != NULL) ? _prefix.data : "",
                               _mode,
                               (_write_description) ? 1 : 0,
                               d_arg_command_name(_command));
}

/**
 * @brief A to_flag of the caller's: `true`, whatever the item holds; the
 *        call counted in the format's context.
 *
 * @param[in]     _format  the format; its context the table's.
 * @param[in]     _item    the item; unused.
 * @param[in,out] _out     receives `true`.
 * @return the buffer's status.
 */
static int
d_tests_arg_config_always(
    const struct d_arg_config_format* _format,
    const struct d_arg_config_item*   _item,
    struct d_arg_buffer*              _out
)
{
    struct d_tests_arg_config_table* const table = _format->context;

    (void)_item;
    ++table->calls;

    return d_arg_buffer_append_cstr(_out,
                                    "true");
}

/**
 * @brief Reads a text through a format and appends its items' lines, or
 *        `ERR <status>: <description>`.
 *
 * @param[in,out] _out     the transcript.
 * @param[in]     _format  the format.
 * @param[in]     _text    the text.
 * @param[in]     _plain   whether CLI11's own reading is asked for, the
 *                         table passed over.
 */
static void
d_tests_arg_config_read_through(
    struct d_arg_buffer*              _out,
    const struct d_arg_config_format* _format,
    const char*                       _text,
    bool                              _plain
)
{
    struct d_arg_config_item_list items  = D_ARG_LIST_INIT;
    struct d_arg_buffer           error  = D_ARG_BUFFER_INIT;
    const struct d_pack_text      text   = d_pack_text_from_cstr(_text);
    const int                     status = (_plain)
                                           ? d_arg_config_from_text_default(
                                                 _format,
                                                 text,
                                                 &items,
                                                 &error)
                                           : d_arg_config_from_text(_format,
                                                                    text,
                                                                    &items,
                                                                    &error);

    // each item, then the failure's line
    for (size_t index = 0u; index < items.count; ++index)
    {
        d_tests_arg_config_item(_out,
                                "",
                                &items.items[index]);
    }

    // a failure, with what described it
    if (status != D_ARG_OK)
    {
        (void)d_arg_buffer_printf(_out,
                                  "ERR %s: %s\n",
                                  d_arg_status_name(status),
                                  (error.data != NULL) ? error.data : "");
    }

    d_arg_config_item_list_release(&items);
    d_arg_buffer_release(&error);

    return;
}

/**
 * @brief Reading through tables: the caller's from_config, a refusal it
 *        describes, a table without one reading as CLI11, and CLI11's own
 *        reading asked for directly.
 *
 * @param[in] _lines  a format whose table reads lines.
 * @param[in] _flags  a format whose table replaces only to_flag.
 * @return whether every text came to what it should.
 */
static bool
d_tests_arg_config_tables_read(
    const struct d_arg_config_format* _lines,
    const struct d_arg_config_format* _flags
)
{
    struct d_arg_buffer written = D_ARG_BUFFER_INIT;

    d_tests_arg_config_read_through(&written,
                                    _lines,
                                    "a 1\nb\nc d e",
                                    false);
    d_tests_arg_config_read_through(&written,
                                    _lines,
                                    "a 1\n!\nb 2",
                                    false);
    d_tests_arg_config_read_through(&written,
                                    _flags,
                                    "a=1",
                                    false);
    d_tests_arg_config_read_through(&written,
                                    _lines,
                                    "a=1\n[s]\nb = [2, 3]",
                                    true);

    const bool same = d_tests_arg_same("reading through tables",
                                       d_arg_buffer_text(&written),
                                       "P#0= N=a M=0 I#1=1\n"
                                       "P#0= N=b M=0 I#0=\n"
                                       "P#0= N=c M=0 I#1=d e\n"
                                       "P#0= N=a M=0 I#1=1\n"
                                       "ERR ParseError: refused by the "
                                       "caller\n"
                                       "P#0= N=a M=0 I#1=1\n"
                                       "P#0= N=a M=0 I#1=1\n"
                                       "P#1=s N=++ M=0 I#0=\n"
                                       "P#1=s N=b M=0 I#2=2|3\n"
                                       "P#1=s N=-- M=0 I#0=\n");
    const bool kept = (written.failed == 0);

    d_arg_buffer_release(&written);

    return ( (same) &&
             (kept) );
}

/**
 * @brief Writing through tables: the caller's to_config given what
 *        d_arg_config_to_text and d_arg_command_config_to_sink pass it, and
 *        CLI11's own writing asked for directly.
 *
 * @param[in,out] _run      the run.
 * @param[in,out] _summary  a format whose table writes a summary, which
 *                          the run's root borrows while this runs.
 * @return whether every text came to what it should.
 */
static bool
d_tests_arg_config_tables_write(
    struct d_tests_arg_run*     _run,
    struct d_arg_config_format* _summary
)
{
    struct d_arg_command* const app     = &_run->parser.command;
    struct d_arg_buffer         written = D_ARG_BUFFER_INIT;
    const char* const           args[]  = { "--a", "1" };

    d_arg_command_set_config_format(app,
                                    _summary);
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  app,
                                  "--a");

    const int parsed = d_tests_arg_parse_quiet(_run,
                                               args,
                                               2u);
    const int text   = d_arg_config_to_text(_summary,
                                            app,
                                            2,
                                            true,
                                            d_pack_text_from_cstr("p."),
                                            &written);
    const int sunk   = d_arg_command_config_to_sink(
                           app,
                           1,
                           false,
                           d_arg_buffer_sink(&written));
    const int plain  = d_arg_config_to_text_default(
                           _summary,
                           app,
                           0,
                           false,
                           d_pack_text_from_cstr(""),
                           &written);

    const bool same = d_tests_arg_same("writing through tables",
                                       d_arg_buffer_text(&written),
                                       "p.|2|1|prog\n"
                                       "|1|0|prog\n"
                                       "a=1\n");
    const bool kept = (written.failed == 0);

    d_arg_command_set_config_format(app,
                                    NULL);
    d_arg_buffer_release(&written);

    return ( (parsed == D_ARG_OK) &&
             (text == D_ARG_OK)   &&
             (sunk == D_ARG_OK)   &&
             (plain == D_ARG_OK)  &&
             (same)               &&
             (kept) );
}

/**
 * @brief Parses a file the root's format of the caller's refuses, its
 *        refusal described as asked.
 *
 * @param[in,out] _run      the run, its root given the format.
 * @param[in,out] _table    the format's context, whose refusal is set.
 * @param[in]     _refusal  the description, which may be empty.
 */
static void
d_tests_arg_config_tables_refused(
    struct d_tests_arg_run*          _run,
    struct d_tests_arg_config_table* _table,
    const char*                      _refusal
)
{
    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--config", "refused.cfg"
    };

    _table->refusal = _refusal;
    (void)d_tests_arg_parse(_run,
                            ARGS,
                            sizeof(ARGS) / sizeof(ARGS[0]));

    return;
}

/**
 * @brief A parse reading a file through the root's format of the caller's:
 *        its lines read as items, a flag given what the table's to_flag
 *        makes of its value; then a file the format refuses, described,
 *        described by nothing, and by one character.
 *
 * @param[in] _format  a format whose table reads lines and makes every
 *                     flag's text `true`.
 * @return whether every parse came to what it should.
 */
static bool
d_tests_arg_config_tables_parse(
    struct d_arg_config_format* _format
)
{
    struct d_tests_arg_config_table* const table   = _format->context;
    const char* const                      refusal = table->refusal;
    struct d_tests_arg_files               scratch = { 0 };
    const bool                             entered =
        d_tests_arg_files_enter(&scratch);
    struct d_tests_arg_run                 run     = { 0 };
    struct d_arg_command* const            app     = d_tests_arg_begin(&run);
    const char* const                      good[]  = { "--config",
                                                       "lines.cfg" };

    (void)d_tests_arg_files_file(&scratch,
                                 "lines.cfg",
                                 "a 5\nf maybe");
    (void)d_tests_arg_files_file(&scratch,
                                 "refused.cfg",
                                 "a 6\n!");
    d_arg_command_set_config_format(app,
                                    _format);
    (void)d_arg_command_set_config(app,
                                   "--config",
                                   NULL,
                                   NULL,
                                   false);
    (void)d_tests_arg_make_option(&run,
                                  "a",
                                  app,
                                  "--a");
    (void)d_tests_arg_make_flag(&run,
                                "f",
                                app,
                                "--f");
    (void)d_tests_arg_parse(&run,
                            good,
                            2u);
    d_tests_arg_config_tables_refused(&run,
                                      table,
                                      refusal);
    d_tests_arg_config_tables_refused(&run,
                                      table,
                                      "");
    d_tests_arg_config_tables_refused(&run,
                                      table,
                                      "!");
    table->refusal = refusal;

    const bool kept = (!scratch.failed);
    const bool same = d_tests_arg_matches(&run,
                                          "cmd app n=1 miss=[] subs=[] "
                                          "order=[--config]\n"
                                          "opt a n=1 res=[5]\n"
                                          "opt f n=1 res=[true]\n"
                                          "ERR ParseError 111: refused by "
                                          "the caller\n"
                                          "cmd app n=1 miss=[] subs=[] "
                                          "order=[--config]\n"
                                          "opt a n=0 res=[]\n"
                                          "opt f n=0 res=[]\n"
                                          "ERR ParseError 111: Parse "
                                          "error\n"
                                          "cmd app n=1 miss=[] subs=[] "
                                          "order=[--config]\n"
                                          "opt a n=0 res=[]\n"
                                          "opt f n=0 res=[]\n"
                                          "ERR ParseError 111: !\n"
                                          "cmd app n=1 miss=[] subs=[] "
                                          "order=[--config]\n"
                                          "opt a n=0 res=[]\n"
                                          "opt f n=0 res=[]\n");

    d_tests_arg_end(&run);
    d_tests_arg_files_leave(&scratch);

    return ( (entered) &&
             (kept)    &&
             (same) );
}

/**
 * @brief The reading call refusing what the caller's table would be given
 *        -- no items or buffer, a text of a length but no data -- before
 *        calling it.
 *
 * @param[in] _format  a format whose table counts its calls.
 * @return whether every call was refused without the table.
 */
static bool
d_tests_arg_config_tables_unread(
    const struct d_arg_config_format* _format
)
{
    const struct d_tests_arg_config_table* const table  = _format->context;
    const size_t                                 before = table->calls;
    struct d_arg_config_item_list                items  = D_ARG_LIST_INIT;
    struct d_arg_buffer                          text   = D_ARG_BUFFER_INIT;
    const struct d_pack_text                     lines  =
        d_pack_text_from_cstr("a 1");
    const struct d_pack_text                     none   = { NULL, 1u };
    const bool                                   held   =
        ( (d_arg_config_from_text(_format,
                                  lines,
                                  NULL,
                                  &text) == D_ARG_ERROR_BAD_ARGUMENT)   &&
          (d_arg_config_from_text(_format,
                                  lines,
                                  &items,
                                  NULL) == D_ARG_ERROR_BAD_ARGUMENT)    &&
          (d_arg_config_from_text(_format,
                                  none,
                                  &items,
                                  &text) == D_ARG_ERROR_BAD_ARGUMENT)   &&
          (table->calls == before) );

    d_arg_config_item_list_release(&items);
    d_arg_buffer_release(&text);

    return d_tests_arg_config_held(held,
                                   "what a table would read, refused before "
                                   "it");
}

/**
 * @brief The writing calls and the flag's refusing what the caller's table
 *        would be given -- no command, item or buffer, a prefix of a length
 *        but no data -- before calling it.
 *
 * @param[in] _format   a format whose table counts its calls.
 * @param[in] _command  a command to write.
 * @return whether every call was refused without the table.
 */
static bool
d_tests_arg_config_tables_unwritten(
    const struct d_arg_config_format* _format,
    const struct d_arg_command*       _command
)
{
    const struct d_tests_arg_config_table* const table  = _format->context;
    const size_t                                 before = table->calls;
    struct d_arg_buffer                          text   = D_ARG_BUFFER_INIT;
    struct d_arg_config_item                     item   =
        D_ARG_CONFIG_ITEM_INIT;
    const struct d_pack_text                     plain  =
        d_pack_text_from_cstr("");
    const struct d_pack_text                     none   = { NULL, 1u };
    const bool                                   held   =
        ( (d_arg_config_to_text(_format,
                                NULL,
                                0,
                                false,
                                plain,
                                &text) == D_ARG_ERROR_BAD_ARGUMENT)     &&
          (d_arg_config_to_text(_format,
                                _command,
                                0,
                                false,
                                plain,
                                NULL) == D_ARG_ERROR_BAD_ARGUMENT)      &&
          (d_arg_config_to_text(_format,
                                _command,
                                0,
                                false,
                                none,
                                &text) == D_ARG_ERROR_BAD_ARGUMENT)     &&
          (d_arg_config_to_flag(_format,
                                NULL,
                                &text) == D_ARG_ERROR_BAD_ARGUMENT)     &&
          (d_arg_config_to_flag(_format,
                                &item,
                                NULL) == D_ARG_ERROR_BAD_ARGUMENT)      &&
          (table->calls == before) );

    d_arg_buffer_release(&text);

    return d_tests_arg_config_held(held,
                                   "what a table would write, refused before "
                                   "it");
}

/**
 * @brief An empty text and a prefix of no data, which the dispatching
 *        calls pass to the caller's table.
 *
 * @param[in] _format   a format whose table counts its calls.
 * @param[in] _command  a command to write.
 * @return whether both reached the table and came to what they should.
 */
static bool
d_tests_arg_config_tables_passed(
    const struct d_arg_config_format* _format,
    const struct d_arg_command*       _command
)
{
    const struct d_tests_arg_config_table* const table  = _format->context;
    const size_t                                 before = table->calls;
    const struct d_pack_text                     empty  = { NULL, 0u };
    struct d_arg_config_item_list                items  = D_ARG_LIST_INIT;
    struct d_arg_buffer                          text   = D_ARG_BUFFER_INIT;
    const int                                    read   =
        d_arg_config_from_text(_format,
                               empty,
                               &items,
                               &text);
    const int                                    wrote  =
        d_arg_config_to_text(_format,
                             _command,
                             0,
                             false,
                             empty,
                             &text);
    const bool                                   passed =
        ( (read == D_ARG_OK)                          &&
          (wrote == D_ARG_OK)                         &&
          (items.count == 0u)                         &&
          (table->calls == (before + 2u))             &&
          (d_tests_arg_same("an empty prefix written",
                            d_arg_buffer_text(&text),
                            "|0|0|prog\n")) );

    d_arg_config_item_list_release(&items);
    d_arg_buffer_release(&text);

    return d_tests_arg_config_held(passed,
                                   "an empty text and a prefix of no data, "
                                   "given to a table");
}

/*
d_tests_arg_config_tables
  A format's table of operations, beyond CLI11: the caller's reading,
writing and flag text, each given what the dispatching calls pass, the
format's context reaching them; a table's NULL entries running CLI11's own;
and CLI11's own asked for past a table.
  Tests the following:
  - reading through the caller's from_config, and its refusal described
  - a table without from_config reading as CLI11 reads
  - writing through the caller's to_config, from d_arg_config_to_text and
    from d_arg_command_config_to_sink
  - CLI11's reading and writing asked for directly, past the table
  - a parse reading a file through the root's table, its flags given the
    table's to_flag text, and the table's refusal ending the parse
  - what a table would be given refused before it is called, and an empty
    text and prefix given to it
*/
bool
d_tests_arg_config_tables(void)
{
    // LINES
    //   constant: a table reading lines, writing a summary, and making every
    // flag's text `true`.
    static const struct d_arg_config_format_vtable LINES =
    {
        .from_config = d_tests_arg_config_lines,
        .to_config   = d_tests_arg_config_summary,
        .to_flag     = d_tests_arg_config_always
    };
    // FLAGS_ONLY
    //   constant: a table replacing to_flag alone.
    static const struct d_arg_config_format_vtable FLAGS_ONLY =
    {
        .to_flag = d_tests_arg_config_always
    };

    struct d_tests_arg_config_table lined   = { "refused by the caller", 0u };
    struct d_tests_arg_config_table flagged = { "", 0u };
    struct d_arg_config_format      lines   = { 0 };
    struct d_arg_config_format      flags   = { 0 };
    struct d_tests_arg_run          run     = { 0 };
    struct d_arg_command* const     app     = d_tests_arg_begin(&run);

    d_arg_config_format_init(&lines);
    d_arg_config_format_init(&flags);
    lines.vtable  = &LINES;
    lines.context = &lined;
    flags.vtable  = &FLAGS_ONLY;
    flags.context = &flagged;

    const bool read    = d_tests_arg_config_tables_read(&lines,
                                                        &flags);
    const bool write   = ( (app != NULL) &&
                           (d_tests_arg_config_tables_write(&run,
                                                            &lines)) );
    const bool parsed  = d_tests_arg_config_tables_parse(&lines);
    const bool refused = ( (app != NULL)                                  &&
                           (d_tests_arg_config_tables_unread(&lines))     &&
                           (d_tests_arg_config_tables_unwritten(&lines,
                                                                app))     &&
                           (d_tests_arg_config_tables_passed(&lines,
                                                             app)) );
    const bool held    = ( (read)    &&
                           (write)   &&
                           (parsed)  &&
                           (refused) &&
                           (!run.failed) );

    d_tests_arg_end(&run);
    d_arg_config_format_release(&lines);
    d_arg_config_format_release(&flags);

    return held;
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 63. READING FILES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief Appends the lines of the items a read came to, or its failure:
 *        `ERR <status>: <description>`.
 *
 * @param[in,out] _out     the transcript.
 * @param[in]     _status  what the read came to.
 * @param[in]     _items   the items read.
 * @param[in]     _error   the description of a failure.
 */
static void
d_tests_arg_config_outcome(
    struct d_arg_buffer*                 _out,
    int                                  _status,
    const struct d_arg_config_item_list* _items,
    const struct d_arg_buffer*           _error
)
{
    // each item read
    for (size_t index = 0u; index < _items->count; ++index)
    {
        d_tests_arg_config_item(_out,
                                "",
                                &_items->items[index]);
    }

    // a failure, with what described it
    if (_status != D_ARG_OK)
    {
        (void)d_arg_buffer_printf(_out,
                                  "ERR %s: ",
                                  d_arg_status_name(_status));
        d_tests_arg_escape(_out,
                           d_arg_buffer_text(_error));
        (void)d_arg_buffer_append_char(_out,
                                       '\n');
    }

    return;
}

/**
 * @brief Reads a file of a scratch directory, and the text it holds: the
 *        two come to the same items.
 *
 * @param[in,out] _scratch  the scratch directory, entered.
 * @param[in]     _format   the format read with.
 * @return whether both were read, and the same.
 */
static bool
d_tests_arg_config_files_same(
    struct d_tests_arg_files*         _scratch,
    const struct d_arg_config_format* _format
)
{
    // TEXT
    //   constant: what the file holds.
    static const char TEXT[] = "a=1\n[s]\nb = [2, 3] # c\n";

    struct d_arg_config_item_list items   = D_ARG_LIST_INIT;
    struct d_arg_config_item_list texts   = D_ARG_LIST_INIT;
    struct d_arg_buffer           error   = D_ARG_BUFFER_INIT;
    struct d_arg_buffer           written = D_ARG_BUFFER_INIT;
    struct d_arg_buffer           read    = D_ARG_BUFFER_INIT;
    const bool                    made    = d_tests_arg_files_file(
                                                _scratch,
                                                "plain.cfg",
                                                TEXT);
    const int                     plain   = d_arg_config_from_file(
                                                _format,
                                                "plain.cfg",
                                                &items,
                                                &error);
    const int                     text    = d_arg_config_from_text(
                                                _format,
                                                d_pack_text_from_cstr(TEXT),
                                                &texts,
                                                &error);

    d_tests_arg_config_outcome(&written,
                               plain,
                               &items,
                               &error);
    d_tests_arg_config_outcome(&read,
                               text,
                               &texts,
                               &error);

    const bool same = ( (made)                                     &&
                        (plain == D_ARG_OK)                        &&
                        (text == D_ARG_OK)                         &&
                        (items.count == 4u)                        &&
                        (d_tests_arg_same("a file read as its text",
                                          d_arg_buffer_text(&written),
                                          read.data)) );

    d_arg_config_item_list_release(&items);
    d_arg_config_item_list_release(&texts);
    d_arg_buffer_release(&error);
    d_arg_buffer_release(&written);
    d_arg_buffer_release(&read);

    return same;
}

/**
 * @brief Reads a file under a UTF-8 name, then two that are not there, into
 *        one list and one description: each read's items and description
 *        appended to the last's.
 *
 * @param[in,out] _scratch  the scratch directory, entered.
 * @param[in]     _format   the format read with.
 * @return whether the reads came to what they should.
 */
static bool
d_tests_arg_config_files_appended(
    struct d_tests_arg_files*         _scratch,
    const struct d_arg_config_format* _format
)
{
    struct d_arg_config_item_list items   = D_ARG_LIST_INIT;
    struct d_arg_buffer           error   = D_ARG_BUFFER_INIT;
    const bool                    named   = d_tests_arg_files_file(
                                                _scratch,
                                                "\xc3\xa9t\xc3\xa9.cfg",
                                                "u=\xc3\xbc\n");
    const int                     wide    = d_arg_config_from_file(
                                                _format,
                                                "\xc3\xa9t\xc3\xa9.cfg",
                                                &items,
                                                &error);
    const int                     missing = d_arg_config_from_file(
                                                _format,
                                                "missing.cfg",
                                                &items,
                                                &error);
    const int                     again   = d_arg_config_from_file(
                                                _format,
                                                "gone.cfg",
                                                &items,
                                                &error);
    const bool                    held    =
        ( (named)                                                    &&
          (wide == D_ARG_OK)                                         &&
          (missing == D_ARG_ERROR_FILE)                              &&
          (again == D_ARG_ERROR_FILE)                                &&
          (items.count == 1u)                                        &&
          (items.items[0].inputs.count == 1u)                        &&
          (items.items[0].inputs.items[0].length == 2u)              &&
          (d_tests_arg_same("two files that are not there",
                            d_arg_buffer_text(&error),
                            "missing.cfg was not readable (missing?)"
                            "gone.cfg was not readable (missing?)")) );

    d_arg_config_item_list_release(&items);
    d_arg_buffer_release(&error);

    return d_tests_arg_config_held(held,
                                   "a UTF-8 name, then two files not there, "
                                   "appended");
}

/**
 * @brief Reading refused without a format, a text, items or a buffer for
 *        the description.
 *
 * @param[in] _format  a format.
 * @return whether every call was refused.
 */
static bool
d_tests_arg_config_files_refused(
    const struct d_arg_config_format* _format
)
{
    struct d_arg_config_item_list items = D_ARG_LIST_INIT;
    struct d_arg_buffer           error = D_ARG_BUFFER_INIT;
    const struct d_pack_text      text  = d_pack_text_from_cstr("a=1");
    const struct d_pack_text      none  = { NULL, 1u };
    const bool                    held  =
        ( (d_arg_config_from_text(NULL,
                                  text,
                                  &items,
                                  &error) == D_ARG_ERROR_BAD_ARGUMENT)  &&
          (d_arg_config_from_text(_format,
                                  none,
                                  &items,
                                  &error) == D_ARG_ERROR_BAD_ARGUMENT)  &&
          (d_arg_config_from_text(_format,
                                  text,
                                  NULL,
                                  &error) == D_ARG_ERROR_BAD_ARGUMENT)  &&
          (d_arg_config_from_text(_format,
                                  text,
                                  &items,
                                  NULL) == D_ARG_ERROR_BAD_ARGUMENT)    &&
          (d_arg_config_from_file(NULL,
                                  "a.cfg",
                                  &items,
                                  &error) == D_ARG_ERROR_BAD_ARGUMENT)  &&
          (d_arg_config_from_file(_format,
                                  NULL,
                                  &items,
                                  &error) == D_ARG_ERROR_BAD_ARGUMENT)  &&
          (d_arg_config_from_file(_format,
                                  "a.cfg",
                                  NULL,
                                  &error) == D_ARG_ERROR_BAD_ARGUMENT)  &&
          (d_arg_config_from_file(_format,
                                  "a.cfg",
                                  &items,
                                  NULL) == D_ARG_ERROR_BAD_ARGUMENT)    &&
          (items.count == 0u)                                           &&
          (error.length == 0u) );

    return d_tests_arg_config_held(held,
                                   "reading without a format, text, items "
                                   "or description");
}

/*
d_tests_arg_config_files
  Configuration files read without a parse, d_arg_config_from_file: the
items a file comes to, and what cannot be read.
  Tests the following:
  - a file read as its text is read
  - a file under a UTF-8 name
  - files not there, refused as CLI11's FileError, each description
    appended to the last, as each read's items are
  - reading refused without a format, a text, items or a buffer
*/
bool
d_tests_arg_config_files(void)
{
    struct d_tests_arg_files   scratch = { 0 };
    struct d_arg_config_format format  = { 0 };

    d_arg_config_format_init(&format);

    const bool entered = d_tests_arg_files_enter(&scratch);
    const bool read    = ( (entered)                                     &&
                           (d_tests_arg_config_files_same(&scratch,
                                                          &format))      &&
                           (d_tests_arg_config_files_appended(&scratch,
                                                              &format)) );
    const bool refused = d_tests_arg_config_files_refused(&format);
    const bool kept    = (!scratch.failed);

    d_tests_arg_files_leave(&scratch);
    d_arg_config_format_release(&format);

    return ( (read)    &&
             (refused) &&
             (kept) );
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 64. SINKS
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief A sink that takes nothing it is given.
 *
 * @param[in] _context  unused.
 * @param[in] _data     unused.
 * @param[in] _size     unused.
 * @return 0, the bytes taken.
 */
static size_t
d_tests_arg_config_refusing(
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
 * @brief The root's configuration written to a sink, CLI11's
 *        config_to_str, in each mode and each choice of descriptions: the
 *        text d_arg_config_to_text writes with no prefix.
 *
 * @param[in] _run  the run, parsed.
 * @return whether each was written, and the same.
 */
static bool
d_tests_arg_config_sunk(
    const struct d_tests_arg_run* _run
)
{
    const struct d_arg_command* const       app    = &_run->parser.command;
    const struct d_arg_config_format* const format =
        d_arg_command_config_format(app);
    bool                                    held   = true;

    // each mode, and each choice of descriptions
    for (int mode = 0; mode < 3; ++mode)
    {
        // without descriptions, then with them
        for (int described = 0; described < 2; ++described)
        {
            struct d_arg_buffer text = D_ARG_BUFFER_INIT;
            struct d_arg_buffer sunk = D_ARG_BUFFER_INIT;
            const int           made = d_arg_config_to_text(
                                           format,
                                           app,
                                           mode,
                                           (described != 0),
                                           d_pack_text_from_cstr(""),
                                           &text);
            const int           sent = d_arg_command_config_to_sink(
                                           app,
                                           mode,
                                           (described != 0),
                                           d_arg_buffer_sink(&sunk));

            held = ( (made == D_ARG_OK)                            &&
                     (sent == D_ARG_OK)                            &&
                     (text.length > 0u)                            &&
                     (d_tests_arg_same("a configuration sunk",
                                       d_arg_buffer_text(&sunk),
                                       text.data))                 &&
                     (held) );
            d_arg_buffer_release(&text);
            d_arg_buffer_release(&sunk);
        }
    }

    return held;
}

/**
 * @brief The root's configuration appended to what a buffer holds.
 *
 * @param[in] _run  the run, parsed.
 * @return whether it was appended.
 */
static bool
d_tests_arg_config_sinks_appended(
    const struct d_tests_arg_run* _run
)
{
    const struct d_arg_command* const app      = &_run->parser.command;
    struct d_arg_buffer               text     = D_ARG_BUFFER_INIT;
    const int                         held     =
        d_arg_buffer_append_cstr(&text,
                                 "x\n");
    const int                         appended =
        d_arg_config_to_text(d_arg_command_config_format(app),
                             app,
                             0,
                             false,
                             d_pack_text_from_cstr(""),
                             &text);
    const bool                        after    =
        ( (held == D_ARG_OK)       &&
          (appended == D_ARG_OK)   &&
          (text.length > 6u)       &&
          (memcmp(text.data,
                  "x\na=0\n",
                  6u) == 0) );

    d_arg_buffer_release(&text);

    return d_tests_arg_config_held(after,
                                   "a configuration appended");
}

/**
 * @brief Writing refused for what is asked: a sink that takes nothing, a
 *        mode that is none of d_arg_config_output_mode's.
 *
 * @param[in] _run  the run, parsed.
 * @return whether every call was refused.
 */
static bool
d_tests_arg_config_sinks_unsent(
    const struct d_tests_arg_run* _run
)
{
    const struct d_arg_command* const       app      = &_run->parser.command;
    const struct d_arg_config_format* const format   =
        d_arg_command_config_format(app);
    const struct d_pack_sink                refusing =
    {
        d_tests_arg_config_refusing,
        NULL
    };
    const struct d_pack_text                empty    =
        d_pack_text_from_cstr("");
    struct d_arg_buffer                     text     = D_ARG_BUFFER_INIT;
    const bool                              refused  =
        ( (d_arg_command_config_to_sink(app,
                                        0,
                                        false,
                                        refusing) == D_ARG_ERROR_FILE)    &&
          (d_arg_config_to_text(format,
                                app,
                                3,
                                false,
                                empty,
                                &text) == D_ARG_ERROR_BAD_ARGUMENT)       &&
          (d_arg_config_to_text(format,
                                app,
                                -1,
                                false,
                                empty,
                                &text) == D_ARG_ERROR_BAD_ARGUMENT)       &&
          (d_arg_command_config_to_sink(app,
                                        3,
                                        false,
                                        refusing) ==
           D_ARG_ERROR_BAD_ARGUMENT)                                      &&
          (text.length == 0u) );

    d_arg_buffer_release(&text);

    return d_tests_arg_config_held(refused,
                                   "a sink that takes nothing, and modes "
                                   "that are none");
}

/**
 * @brief Writing refused without a format, a command or a buffer, or with
 *        a prefix of a length but no data.
 *
 * @param[in] _run  the run, parsed.
 * @return whether every call was refused.
 */
static bool
d_tests_arg_config_sinks_refused(
    const struct d_tests_arg_run* _run
)
{
    const struct d_arg_command* const       app     = &_run->parser.command;
    const struct d_arg_config_format* const format  =
        d_arg_command_config_format(app);
    const struct d_pack_text                none    = { NULL, 1u };
    const struct d_pack_text                empty   =
        d_pack_text_from_cstr("");
    struct d_arg_buffer                     text    = D_ARG_BUFFER_INIT;
    const bool                              refused =
        ( (d_arg_config_to_text(NULL,
                                app,
                                0,
                                false,
                                empty,
                                &text) == D_ARG_ERROR_BAD_ARGUMENT)       &&
          (d_arg_config_to_text(format,
                                NULL,
                                0,
                                false,
                                empty,
                                &text) == D_ARG_ERROR_BAD_ARGUMENT)       &&
          (d_arg_config_to_text(format,
                                app,
                                0,
                                false,
                                empty,
                                NULL) == D_ARG_ERROR_BAD_ARGUMENT)        &&
          (d_arg_config_to_text(format,
                                app,
                                0,
                                false,
                                none,
                                &text) == D_ARG_ERROR_BAD_ARGUMENT)       &&
          (d_arg_command_config_to_sink(NULL,
                                        0,
                                        false,
                                        d_arg_buffer_sink(&text)) ==
           D_ARG_ERROR_BAD_ARGUMENT)                                      &&
          (text.length == 0u) );

    d_arg_buffer_release(&text);

    return d_tests_arg_config_held(refused,
                                   "no format, command or buffer, and a "
                                   "prefix of no data");
}

/*
d_tests_arg_config_sinks
  A command's configuration written through a sink, CLI11's config_to_str,
and the writing calls given what they refuse.
  Tests the following:
  - a sink given, in each mode and each choice of descriptions, the text
    d_arg_config_to_text writes with no prefix
  - text appended to what a buffer holds
  - a sink that takes nothing refused as a file that cannot be written
  - modes outside d_arg_config_output_mode, and no format, command or
    buffer, refused
*/
bool
d_tests_arg_config_sinks(void)
{
    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--a", "0", "plain", "--p", "3", "sub", "--x", "1"
    };

    struct d_tests_arg_run      run = { 0 };
    struct d_arg_command* const app = d_tests_arg_begin(&run);
    struct d_arg_command* const sub = d_tests_arg_make_subcommand(&run,
                                                                  app,
                                                                  "sub");
    struct d_arg_command* const off = d_tests_arg_make_subcommand(&run,
                                                                  app,
                                                                  "plain");

    (void)d_tests_arg_make_option(&run,
                                  "a",
                                  app,
                                  "--a");
    (void)d_tests_arg_make_option(&run,
                                  "x",
                                  sub,
                                  "--x");
    (void)d_tests_arg_make_option(&run,
                                  "p",
                                  off,
                                  "--p");
    d_arg_command_set_configurable(sub,
                                   true);

    const int  parsed  = d_tests_arg_parse_quiet(&run,
                                                 ARGS,
                                                 sizeof(ARGS) /
                                                 sizeof(ARGS[0]));
    const bool sunk    = d_tests_arg_config_sunk(&run);
    const bool refused = ( (d_tests_arg_config_sinks_appended(&run)) &&
                           (d_tests_arg_config_sinks_unsent(&run))   &&
                           (d_tests_arg_config_sinks_refused(&run)) );
    const bool held    = ( (parsed == D_ARG_OK) &&
                           (sunk)               &&
                           (refused)            &&
                           (!run.failed) );

    d_tests_arg_end(&run);

    return held;
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 65. CLI11'S OWN, CALLED DIRECTLY
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief CLI11's own reading called directly,
 *        d_arg_config_from_text_default: refused without a format, items or
 *        a buffer for the description, or given a text of a length but no
 *        data, nothing read or described; a text of no data and no length
 *        read as the empty text.
 *
 * @return whether every call came to what it should.
 */
static bool
d_tests_arg_config_defaults_read(void)
{
    struct d_arg_config_format    format = { 0 };
    struct d_arg_config_item_list items  = D_ARG_LIST_INIT;
    struct d_arg_buffer           error  = D_ARG_BUFFER_INIT;
    const struct d_pack_text      text   = d_pack_text_from_cstr("a=1");
    const struct d_pack_text      none   = { NULL, 1u };
    const struct d_pack_text      empty  = { NULL, 0u };

    d_arg_config_format_init(&format);

    const bool refused =
        ( (d_arg_config_from_text_default(NULL,
                                          text,
                                          &items,
                                          &error) ==
           D_ARG_ERROR_BAD_ARGUMENT)                              &&
          (d_arg_config_from_text_default(&format,
                                          text,
                                          NULL,
                                          &error) ==
           D_ARG_ERROR_BAD_ARGUMENT)                              &&
          (d_arg_config_from_text_default(&format,
                                          text,
                                          &items,
                                          NULL) ==
           D_ARG_ERROR_BAD_ARGUMENT)                              &&
          (d_arg_config_from_text_default(&format,
                                          none,
                                          &items,
                                          &error) ==
           D_ARG_ERROR_BAD_ARGUMENT) );
    const int  read    = d_arg_config_from_text_default(&format,
                                                        empty,
                                                        &items,
                                                        &error);
    const bool held    = ( (refused)            &&
                           (read == D_ARG_OK)   &&
                           (items.count == 0u)  &&
                           (error.length == 0u) );

    d_arg_config_item_list_release(&items);
    d_arg_buffer_release(&error);
    d_arg_config_format_release(&format);

    return d_tests_arg_config_held(held,
                                   "CLI11's reading refusing what it cannot "
                                   "read, and reading no data as nothing");
}

/**
 * @brief CLI11's own writing called directly,
 *        d_arg_config_to_text_default, refused without a format, a command
 *        or a buffer, nothing written.
 *
 * @param[in] _run  the run, parsed.
 * @return whether every call was refused.
 */
static bool
d_tests_arg_config_defaults_unwritten(
    const struct d_tests_arg_run* _run
)
{
    const struct d_arg_command* const       app     = &_run->parser.command;
    const struct d_arg_config_format* const format  =
        d_arg_command_config_format(app);
    const struct d_pack_text                plain   =
        d_pack_text_from_cstr("");
    struct d_arg_buffer                     text    = D_ARG_BUFFER_INIT;
    const bool                              refused =
        ( (d_arg_config_to_text_default(NULL,
                                        app,
                                        0,
                                        false,
                                        plain,
                                        &text) == D_ARG_ERROR_BAD_ARGUMENT) &&
          (d_arg_config_to_text_default(format,
                                        NULL,
                                        0,
                                        false,
                                        plain,
                                        &text) == D_ARG_ERROR_BAD_ARGUMENT) &&
          (d_arg_config_to_text_default(format,
                                        app,
                                        0,
                                        false,
                                        plain,
                                        NULL) == D_ARG_ERROR_BAD_ARGUMENT)  &&
          (text.length == 0u) );

    d_arg_buffer_release(&text);

    return d_tests_arg_config_held(refused,
                                   "CLI11's writing without a format, a "
                                   "command or a buffer");
}

/**
 * @brief CLI11's own writing called directly,
 *        d_arg_config_to_text_default, refused for a mode that is none of
 *        d_arg_config_output_mode's and for a prefix of a length but no
 *        data, nothing written.
 *
 * @param[in] _run  the run, parsed.
 * @return whether every call was refused.
 */
static bool
d_tests_arg_config_defaults_unmoded(
    const struct d_tests_arg_run* _run
)
{
    const struct d_arg_command* const       app     = &_run->parser.command;
    const struct d_arg_config_format* const format  =
        d_arg_command_config_format(app);
    const struct d_pack_text                plain   =
        d_pack_text_from_cstr("");
    const struct d_pack_text                none    = { NULL, 1u };
    struct d_arg_buffer                     text    = D_ARG_BUFFER_INIT;
    const bool                              refused =
        ( (d_arg_config_to_text_default(format,
                                        app,
                                        3,
                                        false,
                                        plain,
                                        &text) == D_ARG_ERROR_BAD_ARGUMENT) &&
          (d_arg_config_to_text_default(format,
                                        app,
                                        -1,
                                        false,
                                        plain,
                                        &text) == D_ARG_ERROR_BAD_ARGUMENT) &&
          (d_arg_config_to_text_default(format,
                                        app,
                                        0,
                                        false,
                                        none,
                                        &text) == D_ARG_ERROR_BAD_ARGUMENT) &&
          (text.length == 0u) );

    d_arg_buffer_release(&text);

    return d_tests_arg_config_held(refused,
                                   "CLI11's writing in a mode that is none, "
                                   "and with a prefix of no data");
}

/**
 * @brief CLI11's own writing called directly,
 *        d_arg_config_to_text_default, given a prefix of no data and no
 *        length: written as with the empty prefix.
 *
 * @param[in] _run  the run, its root given `--a 1`.
 * @return whether it was written so.
 */
static bool
d_tests_arg_config_defaults_written(
    const struct d_tests_arg_run* _run
)
{
    const struct d_arg_command* const       app     = &_run->parser.command;
    const struct d_arg_config_format* const format  =
        d_arg_command_config_format(app);
    const struct d_pack_text                empty   = { NULL, 0u };
    struct d_arg_buffer                     text    = D_ARG_BUFFER_INIT;
    const int                               written =
        d_arg_config_to_text_default(format,
                                     app,
                                     0,
                                     false,
                                     empty,
                                     &text);
    const bool                              same    =
        ( (written == D_ARG_OK)                                &&
          (d_tests_arg_same("a prefix of no data, written",
                            d_arg_buffer_text(&text),
                            "a=1\n")) );

    d_arg_buffer_release(&text);

    return d_tests_arg_config_held(same,
                                   "CLI11's writing with a prefix of no "
                                   "data");
}

/*
d_tests_arg_config_defaults
  CLI11's own reading and writing called directly, past any format's table:
d_arg_config_from_text_default and d_arg_config_to_text_default, given what
they refuse and what they take as nothing.
  Tests the following:
  - reading refused without a format, items or a buffer, or for a text of a
    length but no data; a text of no data read as the empty text
  - writing refused without a format, a command or a buffer, for a mode that
    is none, or for a prefix of a length but no data
  - a prefix of no data written as the empty prefix
*/
bool
d_tests_arg_config_defaults(void)
{
    // ARGS
    //   constant: the arguments parsed.
    static const char* const ARGS[] =
    {
        "--a", "1"
    };

    struct d_tests_arg_run      run = { 0 };
    struct d_arg_command* const app = d_tests_arg_begin(&run);

    (void)d_tests_arg_make_option(&run,
                                  "a",
                                  app,
                                  "--a");

    const int  parsed = d_tests_arg_parse_quiet(&run,
                                                ARGS,
                                                sizeof(ARGS) /
                                                sizeof(ARGS[0]));
    const bool read   = d_tests_arg_config_defaults_read();
    const bool wrote  = ( (parsed == D_ARG_OK)                          &&
                          (d_tests_arg_config_defaults_unwritten(&run)) &&
                          (d_tests_arg_config_defaults_unmoded(&run))   &&
                          (d_tests_arg_config_defaults_written(&run)) );
    const bool held   = ( (read)  &&
                          (wrote) &&
                          (!run.failed) );

    d_tests_arg_end(&run);

    return held;
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 66. TEXT PARSED AS A FILE, GIVEN NOTHING
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/*
d_tests_arg_config_stream_nulls
  Configuration text parsed as a file would be, given nothing to parse,
which no CLI11 call can be given.
  Tests the following:
  - d_arg_parser_parse_config refused without a parser, or with text that
    is not there though it has a length, before the parser is touched: the
    error of the call before kept
  - text that is not there, of no length, parsed as empty text, which gives
    no item -- an unknown one would be an error here
*/
bool
d_tests_arg_config_stream_nulls(void)
{
    // BROKEN
    //   constant: text whose name holds a malformed escape, CLI11's
    // ParseError.
    static const char BROKEN[] = "\"bad\\qname\".x=1\n";

    struct d_tests_arg_run      run  = { 0 };
    struct d_arg_command* const app  = d_tests_arg_begin(&run);
    const struct d_pack_text    gone = { NULL, 1u };
    const struct d_pack_text    none = { NULL, 0u };

    d_arg_command_set_allow_config_extras(app,
                                          (int)D_ARG_CONFIG_EXTRAS_ERROR);

    const int                       broken    = d_arg_parser_parse_config(
                                                    &run.parser,
                                                    d_pack_text_from_cstr(
                                                        BROKEN));
    const int                       no_parser = d_arg_parser_parse_config(
                                                    NULL,
                                                    d_pack_text_from_cstr(
                                                        "x=1\n"));
    const int                       no_text   = d_arg_parser_parse_config(
                                                    &run.parser,
                                                    gone);
    const struct d_arg_error* const error     = d_arg_parser_error(
                                                    &run.parser);
    const bool                      kept      =
        ( (error != NULL)                        &&
          (error->status == D_ARG_ERROR_PARSE) );
    const int                       empty     = d_arg_parser_parse_config(
                                                    &run.parser,
                                                    none);
    const bool                      held      =
        ( (app != NULL)                            &&
          (broken == D_ARG_ERROR_PARSE)            &&
          (no_parser == D_ARG_ERROR_BAD_ARGUMENT)  &&
          (no_text == D_ARG_ERROR_BAD_ARGUMENT)    &&
          (kept)                                   &&
          (empty == D_ARG_OK) );

    d_tests_arg_end(&run);

    return held;
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 67. WITHOUT CONFIGURATION FILES
//==============================================================================

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 0)

/**
 * @brief U01: a file named that is there.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_u01(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    return;
}

/**
 * @brief U02: a file named that is not there.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_u02(
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
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    return;
}

/**
 * @brief U03: a default file that is not there.
 *
 * @param[in,out] _run  the case's run.
 * @param[in,out] _app  the run's root.
 */
static void
d_tests_arg_config_u03(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const config = d_arg_command_set_config(_app,
                                                                 "--config",
                                                                 "gone.cfg",
                                                                 NULL,
                                                                 false);

    (void)d_tests_arg_watch_option(_run,
                                   "config",
                                   config);
    (void)d_tests_arg_make_option(_run,
                                  "a",
                                  _app,
                                  "--a");

    return;
}

// U01_FILES
//   constant: the files U01 parses with.
static const struct d_tests_arg_file U01_FILES[] =
{
    { "there.cfg", "a=1\n" },
    { NULL,        NULL }
};

// UNBUILT_CASES
//   constant: the cases of d_tests_arg_config_unbuilt.
static const struct d_tests_arg_case UNBUILT_CASES[] =
{
    { "U01",
      d_tests_arg_config_u01,
      { "--config", "there.cfg" },
      NULL,
      U01_FILES },
    { "U02",
      d_tests_arg_config_u02,
      { "--config", "gone.cfg" },
      NULL,
      NULL },
    { "U03",
      d_tests_arg_config_u03,
      { NULL },
      NULL,
      NULL }
};

// UNBUILT_COUNT
//   constant: how many cases UNBUILT_CASES holds.
static const size_t UNBUILT_COUNT =
    sizeof(UNBUILT_CASES) / sizeof(UNBUILT_CASES[0]);

/*
d_tests_arg_config_unbuilt
  The configuration option where configuration files are not built in
(D_CFG_ARG_PARSER_CONFIG_FILES 0).
  Tests the following:
  - a file named that is there, refused
  - a file named that is not there, and a default that is not, as with
    configuration files built in
*/
bool
d_tests_arg_config_unbuilt(void)
{
    return d_tests_arg_cases("UNBUILT",
                             UNBUILT_CASES,
                             UNBUILT_COUNT,
                             UNBUILT);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 68. THE SUITE
//==============================================================================

// d_tests_arg_config
//   constant: the suite, in the header's order.
const struct d_tests_section d_tests_arg_config[] =
{
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_values",
      d_tests_arg_config_values },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_sections",
      d_tests_arg_config_sections },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_headers",
      d_tests_arg_config_headers },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_chosen",
      d_tests_arg_config_chosen },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_layers",
      d_tests_arg_config_layers },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_names",
      d_tests_arg_config_names },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_multiline",
      d_tests_arg_config_multiline },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_comments",
      d_tests_arg_config_comments },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_arrays",
      d_tests_arg_config_arrays },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_escapes",
      d_tests_arg_config_escapes },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_duplicates",
      d_tests_arg_config_duplicates },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_odd_lines",
      d_tests_arg_config_odd_lines },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_delimiters",
      d_tests_arg_config_delimiters },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_bytes",
      d_tests_arg_config_bytes },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_entered",
      d_tests_arg_config_entered },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_transitions",
      d_tests_arg_config_transitions },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_header_edges",
      d_tests_arg_config_header_edges },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_multiline_edges",
      d_tests_arg_config_multiline_edges },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_quote_edges",
      d_tests_arg_config_quote_edges },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_list_edges",
      d_tests_arg_config_list_edges },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_merges",
      d_tests_arg_config_merges },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_departed_reads",
      d_tests_arg_config_departed_reads },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
    { "arg_config_items",
      d_tests_arg_config_items },
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_file_options",
      d_tests_arg_config_file_options },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_file_values",
      d_tests_arg_config_file_values },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_file_commands",
      d_tests_arg_config_file_commands },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_file_names",
      d_tests_arg_config_file_names },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_file_subcommands",
      d_tests_arg_config_file_subcommands },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_file_extras",
      d_tests_arg_config_file_extras },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_file_sources",
      d_tests_arg_config_file_sources },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if ( (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1) &&                             \
      (D_INTERNAL_ARG_PARSER_ENVIRONMENT == 1) )
    { "arg_config_file_environment",
      d_tests_arg_config_file_environment },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES, D_INTERNAL_ARG_PARSER_ENVIRONMENT
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_file_flags",
      d_tests_arg_config_file_flags },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_file_lookups",
      d_tests_arg_config_file_lookups },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_file_marks",
      d_tests_arg_config_file_marks },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_file_named",
      d_tests_arg_config_file_named },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if ( (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1) &&                             \
      (D_INTERNAL_ARG_PARSER_ENVIRONMENT == 1) )
    { "arg_config_file_variables",
      d_tests_arg_config_file_variables },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES, D_INTERNAL_ARG_PARSER_ENVIRONMENT
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_file_permissions",
      d_tests_arg_config_file_permissions },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_streams",
      d_tests_arg_config_streams },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_text_mode",
      d_tests_arg_config_text_mode },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_write_basics",
      d_tests_arg_config_write_basics },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_write_text",
      d_tests_arg_config_write_text },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_write_long",
      d_tests_arg_config_write_long },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_write_longer",
      d_tests_arg_config_write_longer },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_write_policies",
      d_tests_arg_config_write_policies },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_write_commands",
      d_tests_arg_config_write_commands },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_write_groups",
      d_tests_arg_config_write_groups },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_write_formats",
      d_tests_arg_config_write_formats },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_write_prefix",
      d_tests_arg_config_write_prefix },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_write_values",
      d_tests_arg_config_write_values },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_write_quoting",
      d_tests_arg_config_write_quoting },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_write_triples",
      d_tests_arg_config_write_triples },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_write_names",
      d_tests_arg_config_write_names },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_write_flags",
      d_tests_arg_config_write_flags },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_write_orders",
      d_tests_arg_config_write_orders },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_write_sums",
      d_tests_arg_config_write_sums },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_write_joins",
      d_tests_arg_config_write_joins },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_write_defaults",
      d_tests_arg_config_write_defaults },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_write_layout",
      d_tests_arg_config_write_layout },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_departed_moved",
      d_tests_arg_config_departed_moved },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
    { "arg_config_formats",
      d_tests_arg_config_formats },
    { "arg_config_building",
      d_tests_arg_config_building },
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_tables",
      d_tests_arg_config_tables },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_files",
      d_tests_arg_config_files },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_sinks",
      d_tests_arg_config_sinks },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_defaults",
      d_tests_arg_config_defaults },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "arg_config_stream_nulls",
      d_tests_arg_config_stream_nulls },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 0)
    { "arg_config_unbuilt",
      d_tests_arg_config_unbuilt },
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
};

// d_tests_arg_config_count
//   constant: how many sections the table above holds.
const size_t d_tests_arg_config_count =
    sizeof(d_tests_arg_config) / sizeof(d_tests_arg_config[0]);
