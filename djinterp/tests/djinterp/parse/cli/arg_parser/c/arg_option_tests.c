/*******************************************************************************
* djinterp [parse]                                            arg_option_tests.c
*
* The suite of the argument parser's options, declared in arg_option_tests.h.
*   Each section makes its options as CLI11's App("d", "prog") would, on a
* run's parser (arg_tests_support.h), and writes what they came to as the
* option suite's CLI11 reference writes CLI11's: one transcript for each of
* the reference's blocks, compared whole, with values written as they are.
* A refusal is written as CLI11's exception, `ERR <name> <exit code>:
* <message>` -- a definition's from the parser's definition error, which is
* then cleared, as catching the exception lets a program go on; any other's
* from the error the parser reports; a validator CLI11 does not find, which
* it throws as OptionNotFound and the module answers with NULL, as `none`.
* Two transcripts depart from CLI11, and their constants say why: item
* counts too large for an `int`, and a variable a refused default leaves.
*   The cases of the earlier option probes -- counts and policies, values
* split and reduced, values and defaults read as a list, flag values,
* callbacks -- are those probes' own, run here on options a command made.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_option_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "./arg_option_tests.h"  // corresponding header
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <string.h>   // memcmp, strlen
// djinterp
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // options, flags, subcommands
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // statuses, buffers, policies
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // the options
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // the error reports
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_type.h"  // descriptors
#include "../../../c/tests_section.h"  // d_tests_section
#include "./arg_tests_support.h"  // runs, comparisons
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// UNLIMITED
//   constant: D_ARG_EXPECTED_UNLIMITED, CLI11's expected_max_vector_size,
// under a name short enough for the tables below.
enum
{
    UNLIMITED = D_ARG_EXPECTED_UNLIMITED
};

// DEFINITIONS
//   constant: the definition strings whose names NAMES reads, each in a
// parser of its own: short, long and positional names alone and together,
// spaces, one-character long names, digits, the reserved names, characters
// a name may not hold, two positional names, a name twice, empty parts,
// case, the empty definition, and a dash inside a positional name.
static const char* const DEFINITIONS[] =
{
    "-a", "--alpha", "-a,--alpha", "--alpha,-a,--beta,-b", "pos", "pos,-a",
    "-a,pos,--alpha", " -a , --alpha ", "--a", "-1", "--1st", "-ab", "--",
    "-", "++", "--al=pha", "--al pha", "pos1,pos2", "-a,-a", ",-a", "-a,",
    "--x!", "-!", "--alpha,--ALPHA", "", "a-b", "-a,a-b,--a-b", NULL
};

// FLAG_DEFINITIONS
//   constant: the flags' definition strings FLAG_NAMES reads: plain, with
// values in braces, with `!`, with an empty value, two names of one value,
// and a positional name, which a flag cannot have.
static const char* const FLAG_DEFINITIONS[] =
{
    "--flag", "--flag,--no-flag{false},-f{2}", "!--noisy", "--quiet,!--loud",
    "-v,--verbose{3}", "pos", "--flag{}", "--a{x},--b{x}", "-x{true},!-y",
    NULL
};

// TAKEN_OPTIONS
//   constant: the options TAKEN adds to one command in turn, several
// refused for a name another holds.
static const char* const TAKEN_OPTIONS[] =
{
    "-a,--alpha", "--alpha", "-a", "--beta,-a", "pos", "pos", "-b,pos2",
    "--ALPHA", "--gamma", NULL
};

// TAKEN_FLAGS
//   constant: the flags TAKEN then adds to the same command.
static const char* const TAKEN_FLAGS[] =
{
    "--gamma", "-c,--no-alpha{false}", "--ALPHA", "!--delta", "--delta,-d",
    NULL
};

// d_tests_arg_option_form
//   struct: a form an option's names are written in, and the label written
// before them.
struct d_tests_arg_option_form
{
    const char* label;
    uint32_t    form;
};

// NAME_FORMS
//   constant: the forms NAMES writes, as the reference's get_name calls:
// plain, with the positional name, with every name, and with both.
static const struct d_tests_arg_option_form NAME_FORMS[] =
{
    { "name=",  0u                                       },
    { " pos=",  D_ARG_NAME_POSITIONAL                    },
    { " all=",  D_ARG_NAME_ALL                           },
    { " both=", (D_ARG_NAME_ALL | D_ARG_NAME_POSITIONAL) }
};

// FLAG_FORMS
//   constant: the forms FLAG_NAMES writes: plain, with every name and its
// value, and with every name alone.
static const struct d_tests_arg_option_form FLAG_FORMS[] =
{
    { "name=",   0u                                           },
    { " all=",   D_ARG_NAME_ALL                               },
    { " plain=", (D_ARG_NAME_ALL | D_ARG_NAME_NO_FLAG_VALUES) }
};

// MATCH_NAMES
//   constant: what MATCH asks check_name about: the long name in each case
// and without or doubling its underscore, the short name in each case, the
// positional name in each case, the environment variable's name in each
// case, the long name without dashes, and the flag's names.
static const char* const MATCH_NAMES[] =
{
    "--alpha_beta", "--ALPHA_BETA", "--alphabeta", "--alpha__beta", "-a",
    "-A", "pos", "POS", "ENV_X", "env_x", "alpha_beta", "--no-flag", "-n",
    NULL
};

// MATCH_SHORT
//   constant: what MATCH asks check_sname about.
static const char* const MATCH_SHORT[] =
{
    "a", "A", "n", NULL
};

// MATCH_LONG
//   constant: what MATCH asks check_lname about.
static const char* const MATCH_LONG[] =
{
    "alpha_beta", "Alpha_Beta", "alphabeta", "AlphaBeta", "no-flag", NULL
};

// MATCH_FLAG
//   constant: what MATCH asks check_fname about.
static const char* const MATCH_FLAG[] =
{
    "no-flag", "NO-FLAG", "noflag", "alpha_beta", "n", "N", NULL
};

// d_tests_arg_option_check_fn
//   type: one of an option's name checks, which share this signature.
typedef bool (*d_tests_arg_option_check_fn)(const struct d_arg_option* _option,
                                            struct d_pack_text         _name);

// d_tests_arg_option_match_line
//   struct: one line of MATCH: its label, the check, whether the flag is
// asked rather than the option, and the names asked about, ending in NULL.
struct d_tests_arg_option_match_line
{
    const char*                 label;
    d_tests_arg_option_check_fn check;
    bool                        flag;
    const char* const*          names;
};

// MATCH_LINES
//   constant: MATCH's lines under each setting, in the reference's order.
static const struct d_tests_arg_option_match_line MATCH_LINES[] =
{
    { " name:",      d_arg_option_check_name,       false, MATCH_NAMES },
    { " flag name:", d_arg_option_check_name,       true,  MATCH_NAMES },
    { " short:",     d_arg_option_check_short_name, false, MATCH_SHORT },
    { " long:",      d_arg_option_check_long_name,  false, MATCH_LONG  },
    { " fname:",     d_arg_option_check_flag_name,  true,  MATCH_FLAG  }
};

// MATCH_LINE_COUNT
//   constant: how many lines MATCH_LINES holds.
static const size_t MATCH_LINE_COUNT =
    sizeof(MATCH_LINES) / sizeof(MATCH_LINES[0]);

// MATCH_SETTINGS
//   constant: how many settings MATCH runs under: its bit 1 ignores case,
// its bit 2 underscores.
enum
{
    MATCH_SETTINGS = 4
};

// d_tests_arg_option_ignore
//   struct: one case of CONFLICT: the option a command holds, the option
// added beside it, and whether the added one is made to ignore underscores
// rather than case.
struct d_tests_arg_option_ignore
{
    const char* held;
    const char* added;
    bool        underscore;
};

// IGNORE_CASES
//   constant: CONFLICT's cases, each in a parser of its own: a long name in
// another case, a long name with an underscore, names that stay apart, a
// positional name in another case, a short name in another case, and a
// long name that differs in both, under each setting.
static const struct d_tests_arg_option_ignore IGNORE_CASES[] =
{
    { "--Alpha",   "--alpha",  false },
    { "--al_pha",  "--alpha",  true  },
    { "-x,--Beta", "--beta_x", false },
    { "alpha",     "--ALPHA",  false },
    { "-A",        "-a",       false },
    { "--Al_Pha",  "--alpha",  false },
    { "--Al_Pha",  "--alpha",  true  }
};

// IGNORE_COUNT
//   constant: how many cases IGNORE_CASES holds.
static const size_t IGNORE_COUNT =
    sizeof(IGNORE_CASES) / sizeof(IGNORE_CASES[0]);

// SHARING_COUNT
//   constant: how many options CONFLICT asks for the names they share.
enum
{
    SHARING_COUNT = 6
};

// SHARING_DEFINITIONS
//   constant: the options CONFLICT asks for the names they share, `a` to
// `f`: short and long names, a long name alone, a one-character long name,
// a positional name, all three kinds, and a long name in another case.
static const char* const SHARING_DEFINITIONS[SHARING_COUNT] =
{
    "-a,--one", "--one", "--a", "one", "-o,--two,one", "--ONE"
};

// SHARING_COMMANDS
//   constant: the commands SHARING_DEFINITIONS' options are made in, one
// each, since one command refuses a name another of its options holds: the
// root for `a`, and subcommands named after the others.
static const char* const SHARING_COMMANDS[SHARING_COUNT] =
{
    NULL, "b", "c", "d", "e", "f"
};

// d_tests_arg_option_pair
//   struct: two of SHARING_DEFINITIONS' options, by index: the one asked,
// and the one it is asked about.
struct d_tests_arg_option_pair
{
    size_t asked;
    size_t other;
};

// SHARING_PAIRS
//   constant: the pairs CONFLICT asks about first.
static const struct d_tests_arg_option_pair SHARING_PAIRS[] =
{
    { 0u, 1u }, { 0u, 2u }, { 2u, 0u }, { 0u, 3u }, { 3u, 0u },
    { 1u, 3u }, { 3u, 4u }, { 4u, 3u }, { 0u, 5u }, { 5u, 0u }
};

// SHARING_PAIR_COUNT
//   constant: how many pairs SHARING_PAIRS holds.
static const size_t SHARING_PAIR_COUNT =
    sizeof(SHARING_PAIRS) / sizeof(SHARING_PAIRS[0]);

// IGNORING_PAIRS
//   constant: the pairs CONFLICT asks about once `f` ignores case.
static const struct d_tests_arg_option_pair IGNORING_PAIRS[] =
{
    { 0u, 5u }, { 5u, 0u }
};

// IGNORING_COUNT
//   constant: how many pairs IGNORING_PAIRS holds.
static const size_t IGNORING_COUNT =
    sizeof(IGNORING_PAIRS) / sizeof(IGNORING_PAIRS[0]);

// UNCONFIGURABLE_PAIRS
//   constant: the pairs CONFLICT asks about once `a` is not configurable.
static const struct d_tests_arg_option_pair UNCONFIGURABLE_PAIRS[] =
{
    { 0u, 2u }, { 2u, 0u }, { 0u, 3u }, { 3u, 0u }
};

// UNCONFIGURABLE_COUNT
//   constant: how many pairs UNCONFIGURABLE_PAIRS holds.
static const size_t UNCONFIGURABLE_COUNT =
    sizeof(UNCONFIGURABLE_PAIRS) / sizeof(UNCONFIGURABLE_PAIRS[0]);

// d_tests_arg_option_setter
//   enum: the setters a step of COUNTS calls: none, expected(n),
// expected(min, max), type_size(n), type_size(min, max),
// multi_option_policy, required, and allow_extra_args.
enum d_tests_arg_option_setter
{
    COUNT_NONE            = 0,
    COUNT_EXPECTED        = 1,
    COUNT_EXPECTED_RANGE  = 2,
    COUNT_TYPE_SIZE       = 3,
    COUNT_TYPE_SIZE_RANGE = 4,
    COUNT_POLICY          = 5,
    COUNT_REQUIRED        = 6,
    COUNT_EXTRA           = 7
};

// d_tests_arg_option_count_step
//   struct: one step of COUNTS: the label of the line written after it --
// NULL for none -- the setter it calls, the setter's arguments, and whether
// it starts with a fresh option rather than going on with the last.
struct d_tests_arg_option_count_step
{
    const char* label;
    int         setter;
    int         first;
    int         second;
    bool        fresh;
};

// COUNT_STEPS
//   constant: the steps of COUNTS, in the reference's order: each count
// setter on a fresh option, then setters after one another.
static const struct d_tests_arg_option_count_step COUNT_STEPS[] =
{
    { "new",
      COUNT_NONE,            0,                    0,         true  },
    { "expected(3)",
      COUNT_EXPECTED,        3,                    0,         true  },
    { "expected(-2)",
      COUNT_EXPECTED,        -2,                   0,         true  },
    { "expected(0)",
      COUNT_EXPECTED,        0,                    0,         true  },
    { "expected(unlimited)",
      COUNT_EXPECTED,        UNLIMITED,            0,         true  },
    { NULL,
      COUNT_EXPECTED,        5,                    0,         true  },
    { "expected(5),expected(-2)",
      COUNT_EXPECTED,        -2,                   0,         false },
    { NULL,
      COUNT_EXPECTED,        -2,                   0,         true  },
    { "expected(-2),allow_extra_args(false)",
      COUNT_EXTRA,           0,                    0,         false },
    { "expected(2,5)",
      COUNT_EXPECTED_RANGE,  2,                    5,         true  },
    { "expected(5,2)",
      COUNT_EXPECTED_RANGE,  5,                    2,         true  },
    { "expected(-1,-3)",
      COUNT_EXPECTED_RANGE,  -1,                   -3,        true  },
    { "expected(0,1)",
      COUNT_EXPECTED_RANGE,  0,                    1,         true  },
    { "type_size(2)",
      COUNT_TYPE_SIZE,       2,                    0,         true  },
    { "type_size(-2)",
      COUNT_TYPE_SIZE,       -2,                   0,         true  },
    { NULL,
      COUNT_REQUIRED,        1,                    0,         true  },
    { "required,type_size(0)",
      COUNT_TYPE_SIZE,       0,                    0,         false },
    { "type_size(unlimited)",
      COUNT_TYPE_SIZE,       UNLIMITED,            0,         true  },
    { "type_size(2,4)",
      COUNT_TYPE_SIZE_RANGE, 2,                    4,         true  },
    { "type_size(4,2)",
      COUNT_TYPE_SIZE_RANGE, 4,                    2,         true  },
    { "type_size(-1,3)",
      COUNT_TYPE_SIZE_RANGE, -1,                   3,         true  },
    { "type_size(1,unlimited)",
      COUNT_TYPE_SIZE_RANGE, 1,                    UNLIMITED, true  },
    { NULL,
      COUNT_REQUIRED,        1,                    0,         true  },
    { "required,type_size(0,0)",
      COUNT_TYPE_SIZE_RANGE, 0,                    0,         false },
    { NULL,
      COUNT_TYPE_SIZE,       3,                    0,         true  },
    { "type_size(3),expected(4)",
      COUNT_EXPECTED,        4,                    0,         false },
    { "expected(2,unlimited)",
      COUNT_EXPECTED_RANGE,  2,                    UNLIMITED, true  },
    { " policy(throw)",
      COUNT_POLICY,          D_ARG_MULTI_THROW,    0,         false },
    { " policy(take all)",
      COUNT_POLICY,          D_ARG_MULTI_TAKE_ALL, 0,         false },
    { NULL,
      COUNT_EXPECTED_RANGE,  1,                    UNLIMITED, true  },
    { "expected(1,unlimited),policy(take all)",
      COUNT_POLICY,          D_ARG_MULTI_TAKE_ALL, 0,         false },
    { NULL,
      COUNT_POLICY,          D_ARG_MULTI_TAKE_ALL, 0,         true  },
    { NULL,
      COUNT_EXPECTED_RANGE,  2,                    UNLIMITED, false },
    { "policy(take all),expected(2,unlimited),policy(throw)",
      COUNT_POLICY,          D_ARG_MULTI_THROW,    0,         false },
    { "allow_extra_args",
      COUNT_EXTRA,           1,                    0,         true  },
    { "required",
      COUNT_REQUIRED,        1,                    0,         true  }
};

// COUNT_STEP_COUNT
//   constant: how many steps COUNT_STEPS holds.
static const size_t COUNT_STEP_COUNT =
    sizeof(COUNT_STEPS) / sizeof(COUNT_STEPS[0]);

// OVERFLOWING
//   constant: 2^20, a count whose square an `int` cannot hold.
enum
{
    OVERFLOWING = (1 << 20)
};

// OVERFLOWING_TWICE
//   constant: 2^30, a count whose double an `int` cannot hold.
enum
{
    OVERFLOWING_TWICE = (1 << 30)
};

// DEPARTED_STEPS
//   constant: the steps of DEPARTED_COUNTS: options whose type size times
// their expected count an `int` cannot hold.
static const struct d_tests_arg_option_count_step DEPARTED_STEPS[] =
{
    { NULL,
      COUNT_TYPE_SIZE, OVERFLOWING,       0, true  },
    { "type_size(2^20),expected(2^20)",
      COUNT_EXPECTED,  OVERFLOWING,       0, false },
    { NULL,
      COUNT_TYPE_SIZE, 2,                 0, true  },
    { "type_size(2),expected(2^30)",
      COUNT_EXPECTED,  OVERFLOWING_TWICE, 0, false }
};

// DEPARTED_COUNT
//   constant: how many steps DEPARTED_STEPS holds.
static const size_t DEPARTED_COUNT =
    sizeof(DEPARTED_STEPS) / sizeof(DEPARTED_STEPS[0]);

// d_tests_arg_option_config
//   struct: one of the option probe's configurations: the expected count's
// bounds, the type size's, whether extra arguments are allowed, the
// delimiter, and the multi-option policy.
struct d_tests_arg_option_config
{
    int  expected_min;
    int  expected_max;
    int  type_min;
    int  type_max;
    bool extra;
    char delimiter;
    int  policy;
};

// CONFIGS
//   constant: the option probe's configurations (probes/opt/cases.h), in
// its order.
static const struct d_tests_arg_option_config CONFIGS[] =
{
    { 1, 1,         1, 1, false, '\0', D_ARG_MULTI_THROW      },
    { 1, 1,         1, 1, false, ',',  D_ARG_MULTI_THROW      },
    { 1, UNLIMITED, 1, 1, false, '\0', D_ARG_MULTI_TAKE_ALL   },
    { 1, UNLIMITED, 1, 1, false, ';',  D_ARG_MULTI_TAKE_ALL   },
    { 1, 1,         1, 1, true,  '\0', D_ARG_MULTI_THROW      },
    { 1, 1,         2, 2, false, '\0', D_ARG_MULTI_THROW      },
    { 1, 1,         1, 1, false, '\0', D_ARG_MULTI_TAKE_LAST  },
    { 1, 1,         1, 1, false, '\0', D_ARG_MULTI_TAKE_FIRST },
    { 1, 1,         1, 1, false, '\0', D_ARG_MULTI_JOIN       },
    { 1, 1,         1, 1, false, '+',  D_ARG_MULTI_JOIN       },
    { 1, 1,         1, 1, false, '\0', D_ARG_MULTI_SUM        },
    { 1, 1,         1, 1, false, '\0', D_ARG_MULTI_REVERSE    },
    { 2, 2,         1, 1, false, '\0', D_ARG_MULTI_REVERSE    },
    { 2, 3,         1, 1, false, '\0', D_ARG_MULTI_THROW      },
    { 0, 1,         1, 1, false, '\0', D_ARG_MULTI_THROW      },
    { 1, UNLIMITED, 1, 1, false, '\0', D_ARG_MULTI_THROW      },
    { 2, 2,         1, 1, false, '\0', D_ARG_MULTI_TAKE_LAST  },
    { 2, 2,         1, 1, false, '\0', D_ARG_MULTI_TAKE_FIRST },
    { 1, 1,         0, 0, false, '\0', D_ARG_MULTI_THROW      }
};

// CONFIG_COUNT
//   constant: how many configurations CONFIGS holds.
static const size_t CONFIG_COUNT = sizeof(CONFIGS) / sizeof(CONFIGS[0]);

// INPUTS
//   constant: the values each configuration is given one at a time
// (probes/opt/cases.h): delimited values, bracketed lists, the escape for a
// bracketed value, the empty value, lone brackets, spaces, quotes, other
// brackets.
static const char* const INPUTS[] =
{
    "a", "a,b", "a,,b,", ",a", "[a,b]", "[[aa]]", "[[a]]", "[[]]",
    "[\"a,b\",c]", "[]", "[[a,b],[c]]", "[a;b,c]", "", "[", "]", "[ a , b ]",
    "[(a,b),c]", "['x,y'z,w]", "[a<b,c]", "[\"a\\\",b\",c]", "{}",
    "[{a,b},c]", "[[x]]]"
};

// INPUT_COUNT
//   constant: how many values INPUTS holds.
static const size_t INPUT_COUNT = sizeof(INPUTS) / sizeof(INPUTS[0]);

// LIST_MAX
//   constant: the most values one of LISTS holds, its NULL included.
enum
{
    LIST_MAX = 5
};

// LISTS
//   constant: the lists of values each configuration is given, then
// reduced and read (probes/opt/cases.h): one to four values, none, the
// empty container's markers, an empty value, a newline, numbers, a truth
// word. Each ends in NULL.
static const char* const LISTS[][LIST_MAX] =
{
    { "1", NULL },
    { "1", "2", NULL },
    { "1", "2", "3", NULL },
    { NULL },
    { "{}", NULL },
    { "{}", "%%", NULL },
    { "a", "", NULL },
    { "x\n", "y", NULL },
    { "1.5", "2", NULL },
    { "true", "2", NULL },
    { "a", "b", "c", "d", NULL },
    { "", NULL }
};

// LIST_COUNT
//   constant: how many lists LISTS holds.
static const size_t LIST_COUNT = sizeof(LISTS) / sizeof(LISTS[0]);

// d_tests_arg_option_as_case
//   struct: one of the reading probe's cases (probes/opt/asd.h): the policy,
// the expected count's bounds, the delimiter, the default text (NULL for
// none), and the values given, ending in NULL.
struct d_tests_arg_option_as_case
{
    int         policy;
    int         expected_min;
    int         expected_max;
    char        delimiter;
    const char* default_text;
    const char* values[3];
};

// AS_CASES
//   constant: the reading probe's cases, in its order.
static const struct d_tests_arg_option_as_case AS_CASES[] =
{
    { D_ARG_MULTI_TAKE_LAST,  1, 1,         '\0', "d",       { NULL } },
    { D_ARG_MULTI_TAKE_LAST,  1, UNLIMITED, '\0', "[a,b]",   { NULL } },
    { D_ARG_MULTI_JOIN,       1, UNLIMITED, '\0', "[a,b]",   { NULL } },
    { D_ARG_MULTI_JOIN,       1, 1,         '\0', "[a,b]",   { NULL } },
    { D_ARG_MULTI_TAKE_ALL,   1, UNLIMITED, ',',  "a,b,c",   { NULL } },
    { D_ARG_MULTI_TAKE_FIRST, 2, 2,         '\0', "[a,b,c]", { NULL } },
    { D_ARG_MULTI_TAKE_LAST,  2, 2,         '\0', "[a,b,c]", { NULL } },
    { D_ARG_MULTI_THROW,      2, 2,         '\0', "[a,b,c]", { NULL } },
    { D_ARG_MULTI_REVERSE,    2, 2,         '\0', "[a,b,c]", { NULL } },
    { D_ARG_MULTI_SUM,        1, UNLIMITED, '\0', "[1,2,3]", { NULL } },
    { D_ARG_MULTI_THROW,      1, UNLIMITED, '\0', "[]",      { NULL } },
    { D_ARG_MULTI_TAKE_ALL,   1, UNLIMITED, '\0', "[]",      { NULL } },
    { D_ARG_MULTI_TAKE_LAST,  1, 1,         '\0', NULL,      { NULL } },
    { D_ARG_MULTI_TAKE_LAST,  1, 1,         '\0', "",
      { "v", NULL } },
    { D_ARG_MULTI_THROW,      1, 1,         '\0', "z",
      { "v", "w", NULL } }
};

// AS_COUNT
//   constant: how many cases AS_CASES holds.
static const size_t AS_COUNT = sizeof(AS_CASES) / sizeof(AS_CASES[0]);

// AS_UNDEFINED
//   constant: the reading case whose values CLI11 reads as one text past
// the end of an empty list -- undefined behaviour -- and so are not read so.
enum
{
    AS_UNDEFINED = 11
};

// AS_LIST_MAX
//   constant: the most values a list read by AS holds.
enum
{
    AS_LIST_MAX = 8
};

// FLAG_DEFINITION
//   constant: the flag FLAGS_<n> asks for values: a name of its own, a name
// whose value is `false`, and a short name whose value is 2.
static const char FLAG_DEFINITION[] = "--flag,--no-flag{false},-f{2}";

// d_tests_arg_option_flag_config
//   struct: one of the flag probe's configurations of FLAG_DEFINITION:
// whether it refuses overrides, whether its callback is forced, whether it
// expects one value, its default text (NULL for none), and whether it
// ignores case and underscores.
struct d_tests_arg_option_flag_config
{
    bool        no_override;
    bool        force;
    bool        one_value;
    const char* default_text;
    bool        ignore;
};

// FLAG_CONFIGS
//   constant: the flag probe's configurations (probes/opt/flag2.h), in its
// order: none, overrides refused, a value expected with a default, overrides
// refused with a forced default, and case and underscores ignored.
static const struct d_tests_arg_option_flag_config FLAG_CONFIGS[] =
{
    { false, false, false, NULL,   false },
    { true,  false, false, NULL,   false },
    { false, false, true,  "dflt", false },
    { true,  true,  false, "9",    false },
    { false, false, false, NULL,   true  }
};

// FLAG_ASKED
//   constant: the names a flag's value is asked for (probes/opt/flag2.h):
// its three names, a name it lacks, and two spellings only ignored case and
// underscores match.
static const char* const FLAG_ASKED[] =
{
    "flag", "no-flag", "f", "x", "NO-FLAG", "no_flag"
};

// FLAG_HALF
//   constant: how many of FLAG_ASKED each flag transcript holds; the first
// three are in FLAGS_<n>_A, the last in FLAGS_<n>_B.
enum
{
    FLAG_HALF = 3
};

// FLAG_INPUTS
//   constant: what follows `=` when a flag's value is asked for
// (probes/opt/flag2.h): nothing, the empty container, truth words, numbers
// of each sign, a word, spaces, and the most negative `long long` but one.
static const char* const FLAG_INPUTS[] =
{
    "", "{}", "true", "false", "1", "-1", "2", "-2", "abc", "0", "5", "9",
    " 3", "-9223372036854775807"
};

// FLAG_INPUT_COUNT
//   constant: how many inputs FLAG_INPUTS holds.
static const size_t FLAG_INPUT_COUNT =
    sizeof(FLAG_INPUTS) / sizeof(FLAG_INPUTS[0]);

// d_tests_arg_option_callback_kind
//   enum: the callbacks of the callback probe's cases: none, one that
// accepts the values, and one that refuses them.
enum d_tests_arg_option_callback_kind
{
    CALLBACK_NONE    = 0,
    CALLBACK_ACCEPTS = 1,
    CALLBACK_REFUSES = 2
};

// d_tests_arg_option_callback_case
//   struct: one of the callback probe's cases (probes/opt/rc.h): the
// option's name without dashes, its policy, its expected count, whether its
// callback is forced, its default text (NULL for none), its callback,
// whether it is hidden, and the values given, ending in NULL.
struct d_tests_arg_option_callback_case
{
    const char* name;
    int         policy;
    int         expected;
    bool        force;
    const char* default_text;
    int         callback;
    bool        hidden;
    const char* values[4];
};

// CALLBACK_CASES
//   constant: the callback probe's cases, in its order.
static const struct d_tests_arg_option_callback_case CALLBACK_CASES[] =
{
    { "rc0", D_ARG_MULTI_TAKE_LAST, 1, false, NULL,    CALLBACK_ACCEPTS,
      false, { "a", "b", NULL } },
    { "rc1", D_ARG_MULTI_TAKE_LAST, 1, true,  "d",     CALLBACK_ACCEPTS,
      false, { NULL } },
    { "rc2", D_ARG_MULTI_TAKE_LAST, 1, true,  "d",     CALLBACK_REFUSES,
      false, { NULL } },
    { "rc3", D_ARG_MULTI_TAKE_ALL,  1, false, NULL,    CALLBACK_REFUSES,
      false, { "x", "y", NULL } },
    { "rc4", D_ARG_MULTI_TAKE_ALL,  1, false, NULL,    CALLBACK_REFUSES,
      false, { "a", "", NULL } },
    { "rc5", D_ARG_MULTI_TAKE_LAST, 1, true,  NULL,    CALLBACK_ACCEPTS,
      false, { NULL } },
    { "rc6", D_ARG_MULTI_TAKE_LAST, 1, true,  "d",     CALLBACK_NONE,
      false, { NULL } },
    { "rc7", D_ARG_MULTI_THROW,     1, false, NULL,    CALLBACK_ACCEPTS,
      false, { "1", "2", NULL } },
    { "rc8", D_ARG_MULTI_SUM,       1, false, NULL,    CALLBACK_ACCEPTS,
      false, { "1", "2", NULL } },
    { "rc9", D_ARG_MULTI_TAKE_LAST, 1, false, NULL,    CALLBACK_REFUSES,
      true,  { "x", NULL } },
    { "rcA", D_ARG_MULTI_JOIN,      1, true,  "[p,q]", CALLBACK_ACCEPTS,
      false, { NULL } },
    { "rcB", D_ARG_MULTI_THROW,     1, false, NULL,    CALLBACK_ACCEPTS,
      false, { NULL } },
    { "rcC", D_ARG_MULTI_REVERSE,   2, false, NULL,    CALLBACK_REFUSES,
      false, { "1", "2", "3", NULL } }
};

// CALLBACK_COUNT
//   constant: how many cases CALLBACK_CASES holds.
static const size_t CALLBACK_COUNT =
    sizeof(CALLBACK_CASES) / sizeof(CALLBACK_CASES[0]);

// NAMES
//   constant: CLI11's names of DEFINITIONS: get_name plainly, with the
// positional name, with every name, and with both; get_single_name,
// get_positional and nonpositional; or the exception the definition
// throws.
static const char NAMES[] =
    "[-a] name=-a pos= all=-a both=-a single=a positional=0 nonpositional=1\n"
    "[--alpha] name=--alpha pos= all=--alpha both=--alpha single=alpha "
    "positional=0 nonpositional=1\n"
    "[-a,--alpha] name=--alpha pos= all=-a,--alpha both=-a,--alpha "
    "single=alpha positional=0 nonpositional=1\n"
    "[--alpha,-a,--beta,-b] name=--alpha pos= all=-a,-b,--alpha,--beta "
    "both=-a,-b,--alpha,--beta single=alpha positional=0 nonpositional=1\n"
    "[pos] name=pos pos=pos all=pos both=pos single=pos positional=1 "
    "nonpositional=0\n"
    "[pos,-a] name=-a pos=pos all=-a both=pos,-a single=a positional=1 "
    "nonpositional=1\n"
    "[-a,pos,--alpha] name=--alpha pos=pos all=-a,--alpha both=pos,-a,--alpha "
    "single=alpha positional=1 nonpositional=1\n"
    "[ -a , --alpha ] name=--alpha pos= all=-a,--alpha both=-a,--alpha "
    "single=alpha positional=0 nonpositional=1\n"
    "[--a] name=--a pos= all=--a both=--a single=a positional=0 "
    "nonpositional=1\n"
    "[-1] name=-1 pos= all=-1 both=-1 single=1 positional=0 nonpositional=1\n"
    "[--1st] name=--1st pos= all=--1st both=--1st single=1st positional=0 "
    "nonpositional=1\n"
    "[-ab] ERR BadNameString 101: Long names strings require 2 dashes -ab\n"
    "[--] ERR BadNameString 101: Names '-','--','++' are reserved and not "
    "allowed as option names --\n"
    "[-] ERR BadNameString 101: Names '-','--','++' are reserved and not "
    "allowed as option names -\n"
    "[++] ERR BadNameString 101: Names '-','--','++' are reserved and not "
    "allowed as option names ++\n"
    "[--al=pha] ERR BadNameString 101: Bad long name: al=pha\n"
    "[--al pha] ERR BadNameString 101: Bad long name: al pha\n"
    "[pos1,pos2] ERR BadNameString 101: Only one positional name allowed, "
    "remove: pos2\n"
    "[-a,-a] name=-a pos= all=-a,-a both=-a,-a single=a positional=0 "
    "nonpositional=1\n"
    "[,-a] name=-a pos= all=-a both=-a single=a positional=0 nonpositional=1\n"
    "[-a,] name=-a pos= all=-a both=-a single=a positional=0 nonpositional=1\n"
    "[--x!] name=--x! pos= all=--x! both=--x! single=x! positional=0 "
    "nonpositional=1\n"
    "[-!] ERR BadNameString 101: Invalid one char name: -!\n"
    "[--alpha,--ALPHA] name=--alpha pos= all=--alpha,--ALPHA "
    "both=--alpha,--ALPHA single=alpha positional=0 nonpositional=1\n"
    "[] name= pos= all= both= single= positional=0 nonpositional=0\n"
    "[a-b] name=a-b pos=a-b all=a-b both=a-b single=a-b positional=1 "
    "nonpositional=0\n"
    "[-a,a-b,--a-b] name=--a-b pos=a-b all=-a,--a-b both=a-b,-a,--a-b "
    "single=a-b positional=1 nonpositional=1\n";

// FLAG_NAMES
//   constant: CLI11's names of FLAG_DEFINITIONS: get_name plainly, with
// every name, and with every name but without the values; and the default
// text; or the exception.
static const char FLAG_NAMES[] =
    "[--flag] name=--flag all=--flag plain=--flag default=\n"
    "[--flag,--no-flag{false},-f{2}] name=--flag "
    "all=-f{2},--flag,--no-flag{false} plain=-f,--flag,--no-flag default=\n"
    "[!--noisy] name=--noisy all=--noisy{false} plain=--noisy default=\n"
    "[--quiet,!--loud] name=--quiet all=--quiet,--loud{false} "
    "plain=--quiet,--loud default=\n"
    "[-v,--verbose{3}] name=--verbose all=-v,--verbose{3} plain=-v,--verbose "
    "default=\n"
    "[pos] ERR IncorrectConstruction 100: pos: Flags cannot be positional\n"
    "[--flag{}] name=--flag all=--flag{} plain=--flag default=\n"
    "[--a{x},--b{x}] name=--a all=--a{x},--b{x} plain=--a,--b default=\n"
    "[-x{true},!-y] name=-x all=-x{true},-y{false} plain=-x,-y default=\n";

// TAKEN
//   constant: CLI11's add_option of TAKEN_OPTIONS in one App, then its
// add_flag of TAKEN_FLAGS: every name of each option made, or the
// exception.
static const char TAKEN[] =
    "[-a,--alpha] ok -a,--alpha\n"
    "[--alpha] ERR OptionAlreadyAdded 102: cannot add '--alpha': long name "
    "'--alpha' already used by '--alpha'\n"
    "[-a] ERR OptionAlreadyAdded 102: cannot add '-a': short name '-a' already "
    "used by '--alpha'\n"
    "[--beta,-a] ERR OptionAlreadyAdded 102: cannot add '--beta,-a': short "
    "name '-a' already used by '--alpha'\n"
    "[pos] ok pos\n"
    "[pos] ERR OptionAlreadyAdded 102: cannot add 'pos': name 'pos' already "
    "used by 'pos'\n"
    "[-b,pos2] ok -b\n"
    "[--ALPHA] ok --ALPHA\n"
    "[--gamma] ok --gamma\n"
    "[flag --gamma] ERR OptionAlreadyAdded 102: cannot add '--gamma': long "
    "name '--gamma' already used by '--gamma'\n"
    "[flag -c,--no-alpha{false}] ok -c,--no-alpha{false}\n"
    "[flag --ALPHA] ERR OptionAlreadyAdded 102: cannot add '--ALPHA': long "
    "name '--ALPHA' already used by '--ALPHA'\n"
    "[flag !--delta] ok --delta{false}\n"
    "[flag --delta,-d] ERR OptionAlreadyAdded 102: cannot add '--delta,-d': "
    "long name '--delta' already used by '--delta'\n";

// MATCH
//   constant: CLI11's check_name, check_sname, check_lname and check_fname
// of the option "-a,--alpha_beta,pos", whose environment variable is ENV_X,
// and of the flag "-n,--no-flag{false}", under each setting.
static const char MATCH[] =
    "case=0 under=0\n"
    " name: --alpha_beta=1 --ALPHA_BETA=0 --alphabeta=0 --alpha__beta=0 -a=1 "
    "-A=0 pos=1 POS=0 ENV_X=1 env_x=0 alpha_beta=0 --no-flag=0 -n=0\n"
    " flag name: --alpha_beta=0 --ALPHA_BETA=0 --alphabeta=0 --alpha__beta=0 "
    "-a=0 -A=0 pos=0 POS=0 ENV_X=0 env_x=0 alpha_beta=0 --no-flag=1 -n=1\n"
    " short: a=1 A=0 n=0\n"
    " long: alpha_beta=1 Alpha_Beta=0 alphabeta=0 AlphaBeta=0 no-flag=0\n"
    " fname: no-flag=1 NO-FLAG=0 noflag=0 alpha_beta=0 n=0 N=0\n"
    "case=1 under=0\n"
    " name: --alpha_beta=1 --ALPHA_BETA=1 --alphabeta=0 --alpha__beta=0 -a=1 "
    "-A=1 pos=1 POS=1 ENV_X=1 env_x=0 alpha_beta=0 --no-flag=0 -n=0\n"
    " flag name: --alpha_beta=0 --ALPHA_BETA=0 --alphabeta=0 --alpha__beta=0 "
    "-a=0 -A=0 pos=0 POS=0 ENV_X=0 env_x=0 alpha_beta=0 --no-flag=1 -n=1\n"
    " short: a=1 A=1 n=0\n"
    " long: alpha_beta=1 Alpha_Beta=1 alphabeta=0 AlphaBeta=0 no-flag=0\n"
    " fname: no-flag=1 NO-FLAG=1 noflag=0 alpha_beta=0 n=0 N=0\n"
    "case=0 under=1\n"
    " name: --alpha_beta=1 --ALPHA_BETA=0 --alphabeta=1 --alpha__beta=1 -a=1 "
    "-A=0 pos=1 POS=0 ENV_X=1 env_x=0 alpha_beta=0 --no-flag=0 -n=0\n"
    " flag name: --alpha_beta=0 --ALPHA_BETA=0 --alphabeta=0 --alpha__beta=0 "
    "-a=0 -A=0 pos=0 POS=0 ENV_X=0 env_x=0 alpha_beta=0 --no-flag=1 -n=1\n"
    " short: a=1 A=0 n=0\n"
    " long: alpha_beta=1 Alpha_Beta=0 alphabeta=1 AlphaBeta=0 no-flag=0\n"
    " fname: no-flag=1 NO-FLAG=0 noflag=0 alpha_beta=0 n=0 N=0\n"
    "case=1 under=1\n"
    " name: --alpha_beta=1 --ALPHA_BETA=1 --alphabeta=1 --alpha__beta=1 -a=1 "
    "-A=1 pos=1 POS=1 ENV_X=1 env_x=0 alpha_beta=0 --no-flag=0 -n=0\n"
    " flag name: --alpha_beta=0 --ALPHA_BETA=0 --alphabeta=0 --alpha__beta=0 "
    "-a=0 -A=0 pos=0 POS=0 ENV_X=0 env_x=0 alpha_beta=0 --no-flag=1 -n=1\n"
    " short: a=1 A=1 n=0\n"
    " long: alpha_beta=1 Alpha_Beta=1 alphabeta=1 AlphaBeta=1 no-flag=0\n"
    " fname: no-flag=1 NO-FLAG=1 noflag=0 alpha_beta=0 n=0 N=0\n";

// CONFLICT
//   constant: CLI11's ignore_case and ignore_underscore over IGNORE_CASES,
// and whether the setting was kept; then the names SHARING_DEFINITIONS'
// options share, asked in pairs, then once `f` ignores case, then once `a`
// is not configurable.
static const char CONFLICT[] =
    "[ignore_case --Alpha --alpha] ERR OptionAlreadyAdded 102: adding ignore "
    "case caused a name conflict with Alpha is already added\n"
    "kept 0\n"
    "[ignore_underscore --al_pha --alpha] ERR OptionAlreadyAdded 102: adding "
    "ignore underscore caused a name conflict with al_pha is already added\n"
    "kept 0\n"
    "[ignore_case -x,--Beta --beta_x] ok\n"
    "kept 1\n"
    "[ignore_case alpha --ALPHA] ERR OptionAlreadyAdded 102: adding ignore "
    "case caused a name conflict with alpha is already added\n"
    "kept 0\n"
    "[ignore_case -A -a] ERR OptionAlreadyAdded 102: adding ignore case caused "
    "a name conflict with A is already added\n"
    "kept 0\n"
    "[ignore_case --Al_Pha --alpha] ok\n"
    "kept 1\n"
    "[ignore_underscore --Al_Pha --alpha] ok\n"
    "kept 1\n"
    "matching: a/b <one> a/c <a> c/a <a> a/d <one> d/a <one> b/d <one> d/e "
    "<one> e/d <one> a/f <> f/a <>\n"
    "ignore_case: a/f <one> f/a <one>\n"
    "unconfigurable: a/c <> c/a <> a/d <> d/a <>\n";

// COUNTS
//   constant: CLI11's counts after each step of COUNT_STEPS that has a
// label.
static const char COUNTS[] =
    "new exp=1,1 type=1,1 items=1,1 extra=0 req=0 sep=0\n"
    "expected(3) exp=3,3 type=1,1 items=3,3 extra=0 req=0 sep=0\n"
    "expected(-2) exp=2,2 type=1,1 items=2,2 extra=1 req=0 sep=0\n"
    "expected(0) exp=0,0 type=1,1 items=0,0 extra=0 req=0 sep=0\n"
    "expected(unlimited) exp=1,536870912 type=1,1 items=1,536870912 extra=1 "
    "req=0 sep=0\n"
    "expected(5),expected(-2) exp=2,5 type=1,1 items=2,5 extra=1 req=0 sep=0\n"
    "expected(-2),allow_extra_args(false) exp=2,2 type=1,1 items=2,2 extra=0 "
    "req=0 sep=0\n"
    "expected(2,5) exp=2,5 type=1,1 items=2,5 extra=0 req=0 sep=0\n"
    "expected(5,2) exp=2,5 type=1,1 items=2,5 extra=0 req=0 sep=0\n"
    "expected(-1,-3) exp=1,536870912 type=1,1 items=1,536870912 extra=0 req=0 "
    "sep=0\n"
    "expected(0,1) exp=0,1 type=1,1 items=0,1 extra=0 req=0 sep=0\n"
    "type_size(2) exp=1,1 type=2,2 items=2,2 extra=0 req=0 sep=0\n"
    "type_size(-2) exp=1,536870912 type=2,2 items=2,1073741824 extra=0 req=0 "
    "sep=0\n"
    "required,type_size(0) exp=1,1 type=0,0 items=0,0 extra=0 req=0 sep=0\n"
    "type_size(unlimited) exp=1,1 type=1,536870912 items=1,536870912 extra=0 "
    "req=0 sep=1\n"
    "type_size(2,4) exp=1,1 type=2,4 items=2,4 extra=0 req=0 sep=0\n"
    "type_size(4,2) exp=1,1 type=2,4 items=2,4 extra=0 req=0 sep=0\n"
    "type_size(-1,3) exp=1,536870912 type=1,3 items=1,1610612736 extra=0 req=0 "
    "sep=0\n"
    "type_size(1,unlimited) exp=1,1 type=1,536870912 items=1,536870912 extra=0 "
    "req=0 sep=1\n"
    "required,type_size(0,0) exp=1,1 type=0,0 items=0,0 extra=0 req=0 sep=0\n"
    "type_size(3),expected(4) exp=4,4 type=3,3 items=12,12 extra=0 req=0 "
    "sep=0\n"
    "expected(2,unlimited) exp=2,536870912 type=1,1 items=2,536870912 extra=0 "
    "req=0 sep=0\n"
    " policy(throw) exp=2,536870912 type=1,1 items=2,536870912 extra=0 req=0 "
    "sep=0\n"
    " policy(take all) exp=2,2 type=1,1 items=2,2 extra=0 req=0 sep=0\n"
    "expected(1,unlimited),policy(take all) exp=1,536870912 type=1,1 "
    "items=1,536870912 extra=0 req=0 sep=0\n"
    "policy(take all),expected(2,unlimited),policy(throw) exp=2,536870912 "
    "type=1,1 items=2,536870912 extra=0 req=0 sep=0\n"
    "allow_extra_args exp=1,1 type=1,1 items=1,1 extra=1 req=0 sep=0\n"
    "required exp=1,1 type=1,1 items=1,1 extra=0 req=1 sep=0\n";

// CFG_0
//   constant: CLI11 over CONFIGS[0]: each of INPUTS added alone, then each
// of LISTS added, reduced, and read as one text.
static const char CFG_0[] =
    "cfg 0: 1 1 1 1\n"
    "A 0 [1] <a> added 1\n"
    "A 1 [1] <a,b> added 1\n"
    "A 2 [1] <a,,b,> added 1\n"
    "A 3 [1] <,a> added 1\n"
    "A 4 [1] <[a,b]> added 1\n"
    "A 5 [1] <[a]> added 1\n"
    "A 6 [1] <[[a]]> added 1\n"
    "A 7 [1] <[]> added 1\n"
    "A 8 [1] <[\"a,b\",c]> added 1\n"
    "A 9 [1] <[]> added 1\n"
    "A 10 [1] <[[a,b],[c]]> added 1\n"
    "A 11 [1] <[a;b,c]> added 1\n"
    "A 12 [1] <> added 1\n"
    "A 13 [1] <[> added 1\n"
    "A 14 [1] <]> added 1\n"
    "A 15 [1] <[ a , b ]> added 1\n"
    "A 16 [1] <[(a,b),c]> added 1\n"
    "A 17 [1] <['x,y'z,w]> added 1\n"
    "A 18 [1] <[a<b,c]> added 1\n"
    "A 19 [1] <[\"a\\\",b\",c]> added 1\n"
    "A 20 [1] <{}> added 1\n"
    "A 21 [1] <[{a,b},c]> added 1\n"
    "A 22 [1] <[[x]]]> added 1\n"
    "R 0 [1] <1>\n"
    "S 0 <1>\n"
    "R 1 ERR ArgumentMismatch 114: --o0: At most 1 required but received 2\n"
    "S 1 ERR ArgumentMismatch 114: --o0: At most 1 required but received 2\n"
    "R 2 ERR ArgumentMismatch 114: --o0: At most 1 required but received 3\n"
    "S 2 ERR ArgumentMismatch 114: --o0: At most 1 required but received 3\n"
    "R 3 [0]\n"
    "S 3 <>\n"
    "R 4 [2] <{}> <%%>\n"
    "S 4 <{}>\n"
    "R 5 [2] <{}> <%%>\n"
    "S 5 <{}>\n"
    "R 6 ERR ArgumentMismatch 114: --o0: At most 1 required but received 2\n"
    "S 6 ERR ArgumentMismatch 114: --o0: At most 1 required but received 2\n"
    "R 7 ERR ArgumentMismatch 114: --o0: At most 1 required but received 2\n"
    "S 7 ERR ArgumentMismatch 114: --o0: At most 1 required but received 2\n"
    "R 8 ERR ArgumentMismatch 114: --o0: At most 1 required but received 2\n"
    "S 8 ERR ArgumentMismatch 114: --o0: At most 1 required but received 2\n"
    "R 9 ERR ArgumentMismatch 114: --o0: At most 1 required but received 2\n"
    "S 9 ERR ArgumentMismatch 114: --o0: At most 1 required but received 2\n"
    "R 10 ERR ArgumentMismatch 114: --o0: At most 1 required but received 4\n"
    "S 10 ERR ArgumentMismatch 114: --o0: At most 1 required but received 4\n"
    "R 11 [1] <>\n"
    "S 11 <>\n";

// CFG_1
//   constant: the same over CONFIGS[1].
static const char CFG_1[] =
    "cfg 1: 1 1 1 1\n"
    "A 0 [1] <a> added 1\n"
    "A 1 [2] <a> <b> added 2\n"
    "A 2 [2] <a> <b> added 2\n"
    "A 3 [1] <a> added 1\n"
    "A 4 [2] <[a> <b]> added 2\n"
    "A 5 [1] <[a]> added 1\n"
    "A 6 [1] <[[a]]> added 1\n"
    "A 7 [1] <[]> added 1\n"
    "A 8 [3] <[\"a> <b\"> <c]> added 3\n"
    "A 9 [1] <[]> added 1\n"
    "A 10 [3] <[[a> <b]> <[c]]> added 3\n"
    "A 11 [2] <[a;b> <c]> added 2\n"
    "A 12 [1] <> added 1\n"
    "A 13 [1] <[> added 1\n"
    "A 14 [1] <]> added 1\n"
    "A 15 [2] <[ a > < b ]> added 2\n"
    "A 16 [3] <[(a> <b)> <c]> added 3\n"
    "A 17 [3] <['x> <y'z> <w]> added 3\n"
    "A 18 [2] <[a<b> <c]> added 2\n"
    "A 19 [3] <[\"a\\\"> <b\"> <c]> added 3\n"
    "A 20 [1] <{}> added 1\n"
    "A 21 [3] <[{a> <b}> <c]> added 3\n"
    "A 22 [1] <[[x]]]> added 1\n"
    "R 0 [1] <1>\n"
    "S 0 <1>\n"
    "R 1 ERR ArgumentMismatch 114: --o1: At most 1 required but received 2\n"
    "S 1 ERR ArgumentMismatch 114: --o1: At most 1 required but received 2\n"
    "R 2 ERR ArgumentMismatch 114: --o1: At most 1 required but received 3\n"
    "S 2 ERR ArgumentMismatch 114: --o1: At most 1 required but received 3\n"
    "R 3 [0]\n"
    "S 3 <>\n"
    "R 4 [2] <{}> <%%>\n"
    "S 4 <{}>\n"
    "R 5 [2] <{}> <%%>\n"
    "S 5 <{}>\n"
    "R 6 ERR ArgumentMismatch 114: --o1: At most 1 required but received 2\n"
    "S 6 ERR ArgumentMismatch 114: --o1: At most 1 required but received 2\n"
    "R 7 ERR ArgumentMismatch 114: --o1: At most 1 required but received 2\n"
    "S 7 ERR ArgumentMismatch 114: --o1: At most 1 required but received 2\n"
    "R 8 ERR ArgumentMismatch 114: --o1: At most 1 required but received 2\n"
    "S 8 ERR ArgumentMismatch 114: --o1: At most 1 required but received 2\n"
    "R 9 ERR ArgumentMismatch 114: --o1: At most 1 required but received 2\n"
    "S 9 ERR ArgumentMismatch 114: --o1: At most 1 required but received 2\n"
    "R 10 ERR ArgumentMismatch 114: --o1: At most 1 required but received 4\n"
    "S 10 ERR ArgumentMismatch 114: --o1: At most 1 required but received 4\n"
    "R 11 [1] <>\n"
    "S 11 <>\n";

// CFG_2
//   constant: the same over CONFIGS[2].
static const char CFG_2[] =
    "cfg 2: 1 536870912 1 1\n"
    "A 0 [1] <a> added 1\n"
    "A 1 [1] <a,b> added 1\n"
    "A 2 [1] <a,,b,> added 1\n"
    "A 3 [1] <,a> added 1\n"
    "A 4 [2] <a> <b> added 2\n"
    "A 5 [1] <[a]> added 1\n"
    "A 6 [1] <a> added 1\n"
    "A 7 [1] <[]> added 1\n"
    "A 8 [2] <\"a,b\"> <c> added 2\n"
    "A 9 [0] added 0\n"
    "A 10 [3] <a> <b> <c> added 3\n"
    "A 11 [2] <a;b> <c> added 2\n"
    "A 12 [1] <> added 1\n"
    "A 13 [1] <[> added 1\n"
    "A 14 [1] <]> added 1\n"
    "A 15 [2] <a > <b> added 2\n"
    "A 16 [2] <(a,b)> <c> added 2\n"
    "A 17 [3] <'x,y'> <z> <w> added 3\n"
    "A 18 [2] <a<b> <c> added 2\n"
    "A 19 [2] <\"a\\\",b\"> <c> added 2\n"
    "A 20 [1] <{}> added 1\n"
    "A 21 [2] <{a,b}> <c> added 2\n"
    "A 22 [2] <x> <]> added 2\n"
    "R 0 [1] <1>\n"
    "S 0 <1>\n"
    "R 1 [2] <1> <2>\n"
    "S 1 <1>\n"
    "R 2 [3] <1> <2> <3>\n"
    "S 2 <1>\n"
    "R 3 [0]\n"
    "S 3 <>\n"
    "R 4 [2] <{}> <%%>\n"
    "S 4 <{}>\n"
    "R 5 [2] <{}> <%%>\n"
    "S 5 <{}>\n"
    "R 6 [2] <a> <>\n"
    "S 6 <a>\n"
    "R 7 [2] <x\n"
    "> <y>\n"
    "S 7 <x\n"
    ">\n"
    "R 8 [2] <1.5> <2>\n"
    "S 8 <1.5>\n"
    "R 9 [2] <true> <2>\n"
    "S 9 <true>\n"
    "R 10 [4] <a> <b> <c> <d>\n"
    "S 10 <a>\n"
    "R 11 [1] <>\n"
    "S 11 <>\n";

// CFG_3
//   constant: the same over CONFIGS[3].
static const char CFG_3[] =
    "cfg 3: 1 536870912 1 1\n"
    "A 0 [1] <a> added 1\n"
    "A 1 [1] <a,b> added 1\n"
    "A 2 [1] <a,,b,> added 1\n"
    "A 3 [1] <,a> added 1\n"
    "A 4 [2] <a> <b> added 2\n"
    "A 5 [1] <[a]> added 1\n"
    "A 6 [1] <a> added 1\n"
    "A 7 [1] <[]> added 1\n"
    "A 8 [2] <\"a,b\"> <c> added 2\n"
    "A 9 [0] added 0\n"
    "A 10 [3] <a> <b> <c> added 3\n"
    "A 11 [3] <a> <b> <c> added 3\n"
    "A 12 [1] <> added 1\n"
    "A 13 [1] <[> added 1\n"
    "A 14 [1] <]> added 1\n"
    "A 15 [2] <a > <b> added 2\n"
    "A 16 [2] <(a,b)> <c> added 2\n"
    "A 17 [3] <'x,y'> <z> <w> added 3\n"
    "A 18 [2] <a<b> <c> added 2\n"
    "A 19 [2] <\"a\\\",b\"> <c> added 2\n"
    "A 20 [1] <{}> added 1\n"
    "A 21 [2] <{a,b}> <c> added 2\n"
    "A 22 [2] <x> <]> added 2\n"
    "R 0 [1] <1>\n"
    "S 0 <1>\n"
    "R 1 [2] <1> <2>\n"
    "S 1 <1>\n"
    "R 2 [3] <1> <2> <3>\n"
    "S 2 <1>\n"
    "R 3 [0]\n"
    "S 3 <>\n"
    "R 4 [2] <{}> <%%>\n"
    "S 4 <{}>\n"
    "R 5 [2] <{}> <%%>\n"
    "S 5 <{}>\n"
    "R 6 [2] <a> <>\n"
    "S 6 <a>\n"
    "R 7 [2] <x\n"
    "> <y>\n"
    "S 7 <x\n"
    ">\n"
    "R 8 [2] <1.5> <2>\n"
    "S 8 <1.5>\n"
    "R 9 [2] <true> <2>\n"
    "S 9 <true>\n"
    "R 10 [4] <a> <b> <c> <d>\n"
    "S 10 <a>\n"
    "R 11 [1] <>\n"
    "S 11 <>\n";

// CFG_4
//   constant: the same over CONFIGS[4].
static const char CFG_4[] =
    "cfg 4: 1 1 1 1\n"
    "A 0 [1] <a> added 1\n"
    "A 1 [1] <a,b> added 1\n"
    "A 2 [1] <a,,b,> added 1\n"
    "A 3 [1] <,a> added 1\n"
    "A 4 [2] <a> <b> added 2\n"
    "A 5 [1] <[a]> added 1\n"
    "A 6 [1] <a> added 1\n"
    "A 7 [1] <[]> added 1\n"
    "A 8 [2] <\"a,b\"> <c> added 2\n"
    "A 9 [0] added 0\n"
    "A 10 [3] <a> <b> <c> added 3\n"
    "A 11 [2] <a;b> <c> added 2\n"
    "A 12 [1] <> added 1\n"
    "A 13 [1] <[> added 1\n"
    "A 14 [1] <]> added 1\n"
    "A 15 [2] <a > <b> added 2\n"
    "A 16 [2] <(a,b)> <c> added 2\n"
    "A 17 [3] <'x,y'> <z> <w> added 3\n"
    "A 18 [2] <a<b> <c> added 2\n"
    "A 19 [2] <\"a\\\",b\"> <c> added 2\n"
    "A 20 [1] <{}> added 1\n"
    "A 21 [2] <{a,b}> <c> added 2\n"
    "A 22 [2] <x> <]> added 2\n"
    "R 0 [1] <1>\n"
    "S 0 <1>\n"
    "R 1 ERR ArgumentMismatch 114: --o4: At most 1 required but received 2\n"
    "S 1 ERR ArgumentMismatch 114: --o4: At most 1 required but received 2\n"
    "R 2 ERR ArgumentMismatch 114: --o4: At most 1 required but received 3\n"
    "S 2 ERR ArgumentMismatch 114: --o4: At most 1 required but received 3\n"
    "R 3 [0]\n"
    "S 3 <>\n"
    "R 4 [2] <{}> <%%>\n"
    "S 4 <{}>\n"
    "R 5 [2] <{}> <%%>\n"
    "S 5 <{}>\n"
    "R 6 ERR ArgumentMismatch 114: --o4: At most 1 required but received 2\n"
    "S 6 ERR ArgumentMismatch 114: --o4: At most 1 required but received 2\n"
    "R 7 ERR ArgumentMismatch 114: --o4: At most 1 required but received 2\n"
    "S 7 ERR ArgumentMismatch 114: --o4: At most 1 required but received 2\n"
    "R 8 ERR ArgumentMismatch 114: --o4: At most 1 required but received 2\n"
    "S 8 ERR ArgumentMismatch 114: --o4: At most 1 required but received 2\n"
    "R 9 ERR ArgumentMismatch 114: --o4: At most 1 required but received 2\n"
    "S 9 ERR ArgumentMismatch 114: --o4: At most 1 required but received 2\n"
    "R 10 ERR ArgumentMismatch 114: --o4: At most 1 required but received 4\n"
    "S 10 ERR ArgumentMismatch 114: --o4: At most 1 required but received 4\n"
    "R 11 [1] <>\n"
    "S 11 <>\n";

// CFG_5
//   constant: the same over CONFIGS[5].
static const char CFG_5[] =
    "cfg 5: 1 1 2 2\n"
    "A 0 [1] <a> added 1\n"
    "A 1 [1] <a,b> added 1\n"
    "A 2 [1] <a,,b,> added 1\n"
    "A 3 [1] <,a> added 1\n"
    "A 4 [2] <a> <b> added 2\n"
    "A 5 [1] <[a]> added 1\n"
    "A 6 [1] <a> added 1\n"
    "A 7 [1] <[]> added 1\n"
    "A 8 [2] <\"a,b\"> <c> added 2\n"
    "A 9 [0] added 0\n"
    "A 10 [3] <a> <b> <c> added 3\n"
    "A 11 [2] <a;b> <c> added 2\n"
    "A 12 [1] <> added 1\n"
    "A 13 [1] <[> added 1\n"
    "A 14 [1] <]> added 1\n"
    "A 15 [2] <a > <b> added 2\n"
    "A 16 [2] <(a,b)> <c> added 2\n"
    "A 17 [3] <'x,y'> <z> <w> added 3\n"
    "A 18 [2] <a<b> <c> added 2\n"
    "A 19 [2] <\"a\\\",b\"> <c> added 2\n"
    "A 20 [1] <{}> added 1\n"
    "A 21 [2] <{a,b}> <c> added 2\n"
    "A 22 [2] <x> <]> added 2\n"
    "R 0 ERR ArgumentMismatch 114: --o5: At least 2 required but received 1\n"
    "S 0 <1>\n"
    "R 1 [2] <1> <2>\n"
    "S 1 <1>\n"
    "R 2 ERR ArgumentMismatch 114: --o5: At most 2 required but received 3\n"
    "S 2 ERR ArgumentMismatch 114: --o5: At most 2 required but received 3\n"
    "R 3 [0]\n"
    "S 3 <>\n"
    "R 4 ERR ArgumentMismatch 114: --o5: At least 2 required but received 1\n"
    "S 4 <{}>\n"
    "R 5 [2] <{}> <%%>\n"
    "S 5 <{}>\n"
    "R 6 [2] <a> <>\n"
    "S 6 <a>\n"
    "R 7 [2] <x\n"
    "> <y>\n"
    "S 7 <x\n"
    ">\n"
    "R 8 [2] <1.5> <2>\n"
    "S 8 <1.5>\n"
    "R 9 [2] <true> <2>\n"
    "S 9 <true>\n"
    "R 10 ERR ArgumentMismatch 114: --o5: At most 2 required but received 4\n"
    "S 10 ERR ArgumentMismatch 114: --o5: At most 2 required but received 4\n"
    "R 11 ERR ArgumentMismatch 114: --o5: At least 2 required but received 1\n"
    "S 11 <>\n";

// CFG_6
//   constant: the same over CONFIGS[6].
static const char CFG_6[] =
    "cfg 6: 1 1 1 1\n"
    "A 0 [1] <a> added 1\n"
    "A 1 [1] <a,b> added 1\n"
    "A 2 [1] <a,,b,> added 1\n"
    "A 3 [1] <,a> added 1\n"
    "A 4 [1] <[a,b]> added 1\n"
    "A 5 [1] <[a]> added 1\n"
    "A 6 [1] <[[a]]> added 1\n"
    "A 7 [1] <[]> added 1\n"
    "A 8 [1] <[\"a,b\",c]> added 1\n"
    "A 9 [1] <[]> added 1\n"
    "A 10 [1] <[[a,b],[c]]> added 1\n"
    "A 11 [1] <[a;b,c]> added 1\n"
    "A 12 [1] <> added 1\n"
    "A 13 [1] <[> added 1\n"
    "A 14 [1] <]> added 1\n"
    "A 15 [1] <[ a , b ]> added 1\n"
    "A 16 [1] <[(a,b),c]> added 1\n"
    "A 17 [1] <['x,y'z,w]> added 1\n"
    "A 18 [1] <[a<b,c]> added 1\n"
    "A 19 [1] <[\"a\\\",b\",c]> added 1\n"
    "A 20 [1] <{}> added 1\n"
    "A 21 [1] <[{a,b},c]> added 1\n"
    "A 22 [1] <[[x]]]> added 1\n"
    "R 0 [1] <1>\n"
    "S 0 <1>\n"
    "R 1 [1] <2>\n"
    "S 1 <2>\n"
    "R 2 [1] <3>\n"
    "S 2 <3>\n"
    "R 3 [0]\n"
    "S 3 <>\n"
    "R 4 [2] <{}> <%%>\n"
    "S 4 <{}>\n"
    "R 5 [1] <%%>\n"
    "S 5 <%%>\n"
    "R 6 [1] <>\n"
    "S 6 <>\n"
    "R 7 [1] <y>\n"
    "S 7 <y>\n"
    "R 8 [1] <2>\n"
    "S 8 <2>\n"
    "R 9 [1] <2>\n"
    "S 9 <2>\n"
    "R 10 [1] <d>\n"
    "S 10 <d>\n"
    "R 11 [1] <>\n"
    "S 11 <>\n";

// CFG_7
//   constant: the same over CONFIGS[7].
static const char CFG_7[] =
    "cfg 7: 1 1 1 1\n"
    "A 0 [1] <a> added 1\n"
    "A 1 [1] <a,b> added 1\n"
    "A 2 [1] <a,,b,> added 1\n"
    "A 3 [1] <,a> added 1\n"
    "A 4 [1] <[a,b]> added 1\n"
    "A 5 [1] <[a]> added 1\n"
    "A 6 [1] <[[a]]> added 1\n"
    "A 7 [1] <[]> added 1\n"
    "A 8 [1] <[\"a,b\",c]> added 1\n"
    "A 9 [1] <[]> added 1\n"
    "A 10 [1] <[[a,b],[c]]> added 1\n"
    "A 11 [1] <[a;b,c]> added 1\n"
    "A 12 [1] <> added 1\n"
    "A 13 [1] <[> added 1\n"
    "A 14 [1] <]> added 1\n"
    "A 15 [1] <[ a , b ]> added 1\n"
    "A 16 [1] <[(a,b),c]> added 1\n"
    "A 17 [1] <['x,y'z,w]> added 1\n"
    "A 18 [1] <[a<b,c]> added 1\n"
    "A 19 [1] <[\"a\\\",b\",c]> added 1\n"
    "A 20 [1] <{}> added 1\n"
    "A 21 [1] <[{a,b},c]> added 1\n"
    "A 22 [1] <[[x]]]> added 1\n"
    "R 0 [1] <1>\n"
    "S 0 <1>\n"
    "R 1 [1] <1>\n"
    "S 1 <1>\n"
    "R 2 [1] <1>\n"
    "S 2 <1>\n"
    "R 3 [0]\n"
    "S 3 <>\n"
    "R 4 [2] <{}> <%%>\n"
    "S 4 <{}>\n"
    "R 5 [2] <{}> <%%>\n"
    "S 5 <{}>\n"
    "R 6 [1] <a>\n"
    "S 6 <a>\n"
    "R 7 [1] <x\n"
    ">\n"
    "S 7 <x\n"
    ">\n"
    "R 8 [1] <1.5>\n"
    "S 8 <1.5>\n"
    "R 9 [1] <true>\n"
    "S 9 <true>\n"
    "R 10 [1] <a>\n"
    "S 10 <a>\n"
    "R 11 [1] <>\n"
    "S 11 <>\n";

// CFG_8
//   constant: the same over CONFIGS[8].
static const char CFG_8[] =
    "cfg 8: 1 1 1 1\n"
    "A 0 [1] <a> added 1\n"
    "A 1 [1] <a,b> added 1\n"
    "A 2 [1] <a,,b,> added 1\n"
    "A 3 [1] <,a> added 1\n"
    "A 4 [1] <[a,b]> added 1\n"
    "A 5 [1] <[a]> added 1\n"
    "A 6 [1] <[[a]]> added 1\n"
    "A 7 [1] <[]> added 1\n"
    "A 8 [1] <[\"a,b\",c]> added 1\n"
    "A 9 [1] <[]> added 1\n"
    "A 10 [1] <[[a,b],[c]]> added 1\n"
    "A 11 [1] <[a;b,c]> added 1\n"
    "A 12 [1] <> added 1\n"
    "A 13 [1] <[> added 1\n"
    "A 14 [1] <]> added 1\n"
    "A 15 [1] <[ a , b ]> added 1\n"
    "A 16 [1] <[(a,b),c]> added 1\n"
    "A 17 [1] <['x,y'z,w]> added 1\n"
    "A 18 [1] <[a<b,c]> added 1\n"
    "A 19 [1] <[\"a\\\",b\",c]> added 1\n"
    "A 20 [1] <{}> added 1\n"
    "A 21 [1] <[{a,b},c]> added 1\n"
    "A 22 [1] <[[x]]]> added 1\n"
    "R 0 [1] <1>\n"
    "S 0 <1>\n"
    "R 1 [1] <1\n"
    "2>\n"
    "S 1 <1\n"
    "2>\n"
    "R 2 [1] <1\n"
    "2\n"
    "3>\n"
    "S 2 <1\n"
    "2\n"
    "3>\n"
    "R 3 [0]\n"
    "S 3 <>\n"
    "R 4 [2] <{}> <%%>\n"
    "S 4 <{}>\n"
    "R 5 [1] <{}\n"
    "%%>\n"
    "S 5 <{}\n"
    "%%>\n"
    "R 6 [1] <a>\n"
    "S 6 <a>\n"
    "R 7 [1] <x\n"
    "\n"
    "y>\n"
    "S 7 <x\n"
    "\n"
    "y>\n"
    "R 8 [1] <1.5\n"
    "2>\n"
    "S 8 <1.5\n"
    "2>\n"
    "R 9 [1] <true\n"
    "2>\n"
    "S 9 <true\n"
    "2>\n"
    "R 10 [1] <a\n"
    "b\n"
    "c\n"
    "d>\n"
    "S 10 <a\n"
    "b\n"
    "c\n"
    "d>\n"
    "R 11 [1] <>\n"
    "S 11 <>\n";

// CFG_9
//   constant: the same over CONFIGS[9].
static const char CFG_9[] =
    "cfg 9: 1 1 1 1\n"
    "A 0 [1] <a> added 1\n"
    "A 1 [1] <a,b> added 1\n"
    "A 2 [1] <a,,b,> added 1\n"
    "A 3 [1] <,a> added 1\n"
    "A 4 [1] <[a,b]> added 1\n"
    "A 5 [1] <[a]> added 1\n"
    "A 6 [1] <[[a]]> added 1\n"
    "A 7 [1] <[]> added 1\n"
    "A 8 [1] <[\"a,b\",c]> added 1\n"
    "A 9 [1] <[]> added 1\n"
    "A 10 [1] <[[a,b],[c]]> added 1\n"
    "A 11 [1] <[a;b,c]> added 1\n"
    "A 12 [1] <> added 1\n"
    "A 13 [1] <[> added 1\n"
    "A 14 [1] <]> added 1\n"
    "A 15 [1] <[ a , b ]> added 1\n"
    "A 16 [1] <[(a,b),c]> added 1\n"
    "A 17 [1] <['x,y'z,w]> added 1\n"
    "A 18 [1] <[a<b,c]> added 1\n"
    "A 19 [1] <[\"a\\\",b\",c]> added 1\n"
    "A 20 [1] <{}> added 1\n"
    "A 21 [1] <[{a,b},c]> added 1\n"
    "A 22 [1] <[[x]]]> added 1\n"
    "R 0 [1] <1>\n"
    "S 0 <1>\n"
    "R 1 [1] <1+2>\n"
    "S 1 <1+2>\n"
    "R 2 [1] <1+2+3>\n"
    "S 2 <1+2+3>\n"
    "R 3 [0]\n"
    "S 3 <>\n"
    "R 4 [2] <{}> <%%>\n"
    "S 4 <{}>\n"
    "R 5 [1] <{}+%%>\n"
    "S 5 <{}+%%>\n"
    "R 6 [1] <a>\n"
    "S 6 <a>\n"
    "R 7 [1] <x\n"
    "+y>\n"
    "S 7 <x\n"
    "+y>\n"
    "R 8 [1] <1.5+2>\n"
    "S 8 <1.5+2>\n"
    "R 9 [1] <true+2>\n"
    "S 9 <true+2>\n"
    "R 10 [1] <a+b+c+d>\n"
    "S 10 <a+b+c+d>\n"
    "R 11 [1] <>\n"
    "S 11 <>\n";

// CFG_10
//   constant: the same over CONFIGS[10].
static const char CFG_10[] =
    "cfg 10: 1 1 1 1\n"
    "A 0 [1] <a> added 1\n"
    "A 1 [1] <a,b> added 1\n"
    "A 2 [1] <a,,b,> added 1\n"
    "A 3 [1] <,a> added 1\n"
    "A 4 [1] <[a,b]> added 1\n"
    "A 5 [1] <[a]> added 1\n"
    "A 6 [1] <[[a]]> added 1\n"
    "A 7 [1] <[]> added 1\n"
    "A 8 [1] <[\"a,b\",c]> added 1\n"
    "A 9 [1] <[]> added 1\n"
    "A 10 [1] <[[a,b],[c]]> added 1\n"
    "A 11 [1] <[a;b,c]> added 1\n"
    "A 12 [1] <> added 1\n"
    "A 13 [1] <[> added 1\n"
    "A 14 [1] <]> added 1\n"
    "A 15 [1] <[ a , b ]> added 1\n"
    "A 16 [1] <[(a,b),c]> added 1\n"
    "A 17 [1] <['x,y'z,w]> added 1\n"
    "A 18 [1] <[a<b,c]> added 1\n"
    "A 19 [1] <[\"a\\\",b\",c]> added 1\n"
    "A 20 [1] <{}> added 1\n"
    "A 21 [1] <[{a,b},c]> added 1\n"
    "A 22 [1] <[[x]]]> added 1\n"
    "R 0 [1] <1>\n"
    "S 0 <1>\n"
    "R 1 [1] <3>\n"
    "S 1 <3>\n"
    "R 2 [1] <6>\n"
    "S 2 <6>\n"
    "R 3 [0]\n"
    "S 3 <>\n"
    "R 4 [2] <{}> <%%>\n"
    "S 4 <{}>\n"
    "R 5 [1] <{}%%>\n"
    "S 5 <{}%%>\n"
    "R 6 [1] <a>\n"
    "S 6 <a>\n"
    "R 7 [1] <x\n"
    "y>\n"
    "S 7 <x\n"
    "y>\n"
    "R 8 [1] <3.5>\n"
    "S 8 <3.5>\n"
    "R 9 [1] <3>\n"
    "S 9 <3>\n"
    "R 10 [1] <abcd>\n"
    "S 10 <abcd>\n"
    "R 11 [1] <0>\n"
    "S 11 <>\n";

// CFG_11
//   constant: the same over CONFIGS[11].
static const char CFG_11[] =
    "cfg 11: 1 1 1 1\n"
    "A 0 [1] <a> added 1\n"
    "A 1 [1] <a,b> added 1\n"
    "A 2 [1] <a,,b,> added 1\n"
    "A 3 [1] <,a> added 1\n"
    "A 4 [1] <[a,b]> added 1\n"
    "A 5 [1] <[a]> added 1\n"
    "A 6 [1] <[[a]]> added 1\n"
    "A 7 [1] <[]> added 1\n"
    "A 8 [1] <[\"a,b\",c]> added 1\n"
    "A 9 [1] <[]> added 1\n"
    "A 10 [1] <[[a,b],[c]]> added 1\n"
    "A 11 [1] <[a;b,c]> added 1\n"
    "A 12 [1] <> added 1\n"
    "A 13 [1] <[> added 1\n"
    "A 14 [1] <]> added 1\n"
    "A 15 [1] <[ a , b ]> added 1\n"
    "A 16 [1] <[(a,b),c]> added 1\n"
    "A 17 [1] <['x,y'z,w]> added 1\n"
    "A 18 [1] <[a<b,c]> added 1\n"
    "A 19 [1] <[\"a\\\",b\",c]> added 1\n"
    "A 20 [1] <{}> added 1\n"
    "A 21 [1] <[{a,b},c]> added 1\n"
    "A 22 [1] <[[x]]]> added 1\n"
    "R 0 [1] <1>\n"
    "S 0 <1>\n"
    "R 1 [1] <2>\n"
    "S 1 <2>\n"
    "R 2 [1] <3>\n"
    "S 2 <3>\n"
    "R 3 [0]\n"
    "S 3 <>\n"
    "R 4 [2] <{}> <%%>\n"
    "S 4 <{}>\n"
    "R 5 [1] <%%>\n"
    "S 5 <%%>\n"
    "R 6 [1] <>\n"
    "S 6 <>\n"
    "R 7 [1] <y>\n"
    "S 7 <y>\n"
    "R 8 [1] <2>\n"
    "S 8 <2>\n"
    "R 9 [1] <2>\n"
    "S 9 <2>\n"
    "R 10 [1] <d>\n"
    "S 10 <d>\n"
    "R 11 [1] <>\n"
    "S 11 <>\n";

// CFG_12
//   constant: the same over CONFIGS[12].
static const char CFG_12[] =
    "cfg 12: 2 2 1 1\n"
    "A 0 [1] <a> added 1\n"
    "A 1 [1] <a,b> added 1\n"
    "A 2 [1] <a,,b,> added 1\n"
    "A 3 [1] <,a> added 1\n"
    "A 4 [2] <a> <b> added 2\n"
    "A 5 [1] <[a]> added 1\n"
    "A 6 [1] <a> added 1\n"
    "A 7 [1] <[]> added 1\n"
    "A 8 [2] <\"a,b\"> <c> added 2\n"
    "A 9 [0] added 0\n"
    "A 10 [3] <a> <b> <c> added 3\n"
    "A 11 [2] <a;b> <c> added 2\n"
    "A 12 [1] <> added 1\n"
    "A 13 [1] <[> added 1\n"
    "A 14 [1] <]> added 1\n"
    "A 15 [2] <a > <b> added 2\n"
    "A 16 [2] <(a,b)> <c> added 2\n"
    "A 17 [3] <'x,y'> <z> <w> added 3\n"
    "A 18 [2] <a<b> <c> added 2\n"
    "A 19 [2] <\"a\\\",b\"> <c> added 2\n"
    "A 20 [1] <{}> added 1\n"
    "A 21 [2] <{a,b}> <c> added 2\n"
    "A 22 [2] <x> <]> added 2\n"
    "R 0 [1] <1>\n"
    "S 0 <1>\n"
    "R 1 [2] <2> <1>\n"
    "S 1 <2>\n"
    "R 2 [2] <3> <2>\n"
    "S 2 <3>\n"
    "R 3 [0]\n"
    "S 3 <>\n"
    "R 4 [2] <{}> <%%>\n"
    "S 4 <{}>\n"
    "R 5 [2] <%%> <{}>\n"
    "S 5 <%%>\n"
    "R 6 [2] <> <a>\n"
    "S 6 <>\n"
    "R 7 [2] <y> <x\n"
    ">\n"
    "S 7 <y>\n"
    "R 8 [2] <2> <1.5>\n"
    "S 8 <2>\n"
    "R 9 [2] <2> <true>\n"
    "S 9 <2>\n"
    "R 10 [2] <d> <c>\n"
    "S 10 <d>\n"
    "R 11 [1] <>\n"
    "S 11 <>\n";

// CFG_13
//   constant: the same over CONFIGS[13].
static const char CFG_13[] =
    "cfg 13: 2 3 1 1\n"
    "A 0 [1] <a> added 1\n"
    "A 1 [1] <a,b> added 1\n"
    "A 2 [1] <a,,b,> added 1\n"
    "A 3 [1] <,a> added 1\n"
    "A 4 [2] <a> <b> added 2\n"
    "A 5 [1] <[a]> added 1\n"
    "A 6 [1] <a> added 1\n"
    "A 7 [1] <[]> added 1\n"
    "A 8 [2] <\"a,b\"> <c> added 2\n"
    "A 9 [0] added 0\n"
    "A 10 [3] <a> <b> <c> added 3\n"
    "A 11 [2] <a;b> <c> added 2\n"
    "A 12 [1] <> added 1\n"
    "A 13 [1] <[> added 1\n"
    "A 14 [1] <]> added 1\n"
    "A 15 [2] <a > <b> added 2\n"
    "A 16 [2] <(a,b)> <c> added 2\n"
    "A 17 [3] <'x,y'> <z> <w> added 3\n"
    "A 18 [2] <a<b> <c> added 2\n"
    "A 19 [2] <\"a\\\",b\"> <c> added 2\n"
    "A 20 [1] <{}> added 1\n"
    "A 21 [2] <{a,b}> <c> added 2\n"
    "A 22 [2] <x> <]> added 2\n"
    "R 0 ERR ArgumentMismatch 114: --o13: At least 2 required but received 1\n"
    "S 0 <1>\n"
    "R 1 [2] <1> <2>\n"
    "S 1 <1>\n"
    "R 2 [3] <1> <2> <3>\n"
    "S 2 <1>\n"
    "R 3 [0]\n"
    "S 3 <>\n"
    "R 4 ERR ArgumentMismatch 114: --o13: At least 2 required but received 1\n"
    "S 4 <{}>\n"
    "R 5 [2] <{}> <%%>\n"
    "S 5 <{}>\n"
    "R 6 [2] <a> <>\n"
    "S 6 <a>\n"
    "R 7 [2] <x\n"
    "> <y>\n"
    "S 7 <x\n"
    ">\n"
    "R 8 [2] <1.5> <2>\n"
    "S 8 <1.5>\n"
    "R 9 [2] <true> <2>\n"
    "S 9 <true>\n"
    "R 10 ERR ArgumentMismatch 114: --o13: At most 3 required but received 4\n"
    "S 10 ERR ArgumentMismatch 114: --o13: At most 3 required but received 4\n"
    "R 11 ERR ArgumentMismatch 114: --o13: At least 2 required but received 1\n"
    "S 11 <>\n";

// CFG_14
//   constant: the same over CONFIGS[14].
static const char CFG_14[] =
    "cfg 14: 0 1 1 1\n"
    "A 0 [1] <a> added 1\n"
    "A 1 [1] <a,b> added 1\n"
    "A 2 [1] <a,,b,> added 1\n"
    "A 3 [1] <,a> added 1\n"
    "A 4 [1] <[a,b]> added 1\n"
    "A 5 [1] <[a]> added 1\n"
    "A 6 [1] <[[a]]> added 1\n"
    "A 7 [1] <[]> added 1\n"
    "A 8 [1] <[\"a,b\",c]> added 1\n"
    "A 9 [1] <[]> added 1\n"
    "A 10 [1] <[[a,b],[c]]> added 1\n"
    "A 11 [1] <[a;b,c]> added 1\n"
    "A 12 [1] <> added 1\n"
    "A 13 [1] <[> added 1\n"
    "A 14 [1] <]> added 1\n"
    "A 15 [1] <[ a , b ]> added 1\n"
    "A 16 [1] <[(a,b),c]> added 1\n"
    "A 17 [1] <['x,y'z,w]> added 1\n"
    "A 18 [1] <[a<b,c]> added 1\n"
    "A 19 [1] <[\"a\\\",b\",c]> added 1\n"
    "A 20 [1] <{}> added 1\n"
    "A 21 [1] <[{a,b},c]> added 1\n"
    "A 22 [1] <[[x]]]> added 1\n"
    "R 0 [1] <1>\n"
    "S 0 <1>\n"
    "R 1 ERR ArgumentMismatch 114: --o14: At most 1 required but received 2\n"
    "S 1 ERR ArgumentMismatch 114: --o14: At most 1 required but received 2\n"
    "R 2 ERR ArgumentMismatch 114: --o14: At most 1 required but received 3\n"
    "S 2 ERR ArgumentMismatch 114: --o14: At most 1 required but received 3\n"
    "R 3 [0]\n"
    "S 3 <>\n"
    "R 4 [1] <{}>\n"
    "S 4 <{}>\n"
    "R 5 [2] <{}> <%%>\n"
    "S 5 <{}>\n"
    "R 6 ERR ArgumentMismatch 114: --o14: At most 1 required but received 2\n"
    "S 6 ERR ArgumentMismatch 114: --o14: At most 1 required but received 2\n"
    "R 7 ERR ArgumentMismatch 114: --o14: At most 1 required but received 2\n"
    "S 7 ERR ArgumentMismatch 114: --o14: At most 1 required but received 2\n"
    "R 8 ERR ArgumentMismatch 114: --o14: At most 1 required but received 2\n"
    "S 8 ERR ArgumentMismatch 114: --o14: At most 1 required but received 2\n"
    "R 9 ERR ArgumentMismatch 114: --o14: At most 1 required but received 2\n"
    "S 9 ERR ArgumentMismatch 114: --o14: At most 1 required but received 2\n"
    "R 10 ERR ArgumentMismatch 114: --o14: At most 1 required but received 4\n"
    "S 10 ERR ArgumentMismatch 114: --o14: At most 1 required but received 4\n"
    "R 11 [1] <>\n"
    "S 11 <>\n";

// CFG_15
//   constant: the same over CONFIGS[15].
static const char CFG_15[] =
    "cfg 15: 1 536870912 1 1\n"
    "A 0 [1] <a> added 1\n"
    "A 1 [1] <a,b> added 1\n"
    "A 2 [1] <a,,b,> added 1\n"
    "A 3 [1] <,a> added 1\n"
    "A 4 [2] <a> <b> added 2\n"
    "A 5 [1] <[a]> added 1\n"
    "A 6 [1] <a> added 1\n"
    "A 7 [1] <[]> added 1\n"
    "A 8 [2] <\"a,b\"> <c> added 2\n"
    "A 9 [0] added 0\n"
    "A 10 [3] <a> <b> <c> added 3\n"
    "A 11 [2] <a;b> <c> added 2\n"
    "A 12 [1] <> added 1\n"
    "A 13 [1] <[> added 1\n"
    "A 14 [1] <]> added 1\n"
    "A 15 [2] <a > <b> added 2\n"
    "A 16 [2] <(a,b)> <c> added 2\n"
    "A 17 [3] <'x,y'> <z> <w> added 3\n"
    "A 18 [2] <a<b> <c> added 2\n"
    "A 19 [2] <\"a\\\",b\"> <c> added 2\n"
    "A 20 [1] <{}> added 1\n"
    "A 21 [2] <{a,b}> <c> added 2\n"
    "A 22 [2] <x> <]> added 2\n"
    "R 0 [1] <1>\n"
    "S 0 <1>\n"
    "R 1 [2] <1> <2>\n"
    "S 1 <1>\n"
    "R 2 [3] <1> <2> <3>\n"
    "S 2 <1>\n"
    "R 3 [0]\n"
    "S 3 <>\n"
    "R 4 [2] <{}> <%%>\n"
    "S 4 <{}>\n"
    "R 5 [2] <{}> <%%>\n"
    "S 5 <{}>\n"
    "R 6 [2] <a> <>\n"
    "S 6 <a>\n"
    "R 7 [2] <x\n"
    "> <y>\n"
    "S 7 <x\n"
    ">\n"
    "R 8 [2] <1.5> <2>\n"
    "S 8 <1.5>\n"
    "R 9 [2] <true> <2>\n"
    "S 9 <true>\n"
    "R 10 [4] <a> <b> <c> <d>\n"
    "S 10 <a>\n"
    "R 11 [1] <>\n"
    "S 11 <>\n";

// CFG_16
//   constant: the same over CONFIGS[16].
static const char CFG_16[] =
    "cfg 16: 2 2 1 1\n"
    "A 0 [1] <a> added 1\n"
    "A 1 [1] <a,b> added 1\n"
    "A 2 [1] <a,,b,> added 1\n"
    "A 3 [1] <,a> added 1\n"
    "A 4 [2] <a> <b> added 2\n"
    "A 5 [1] <[a]> added 1\n"
    "A 6 [1] <a> added 1\n"
    "A 7 [1] <[]> added 1\n"
    "A 8 [2] <\"a,b\"> <c> added 2\n"
    "A 9 [0] added 0\n"
    "A 10 [3] <a> <b> <c> added 3\n"
    "A 11 [2] <a;b> <c> added 2\n"
    "A 12 [1] <> added 1\n"
    "A 13 [1] <[> added 1\n"
    "A 14 [1] <]> added 1\n"
    "A 15 [2] <a > <b> added 2\n"
    "A 16 [2] <(a,b)> <c> added 2\n"
    "A 17 [3] <'x,y'> <z> <w> added 3\n"
    "A 18 [2] <a<b> <c> added 2\n"
    "A 19 [2] <\"a\\\",b\"> <c> added 2\n"
    "A 20 [1] <{}> added 1\n"
    "A 21 [2] <{a,b}> <c> added 2\n"
    "A 22 [2] <x> <]> added 2\n"
    "R 0 [1] <1>\n"
    "S 0 <1>\n"
    "R 1 [2] <1> <2>\n"
    "S 1 <1>\n"
    "R 2 [2] <2> <3>\n"
    "S 2 <2>\n"
    "R 3 [0]\n"
    "S 3 <>\n"
    "R 4 [2] <{}> <%%>\n"
    "S 4 <{}>\n"
    "R 5 [2] <{}> <%%>\n"
    "S 5 <{}>\n"
    "R 6 [2] <a> <>\n"
    "S 6 <a>\n"
    "R 7 [2] <x\n"
    "> <y>\n"
    "S 7 <x\n"
    ">\n"
    "R 8 [2] <1.5> <2>\n"
    "S 8 <1.5>\n"
    "R 9 [2] <true> <2>\n"
    "S 9 <true>\n"
    "R 10 [2] <c> <d>\n"
    "S 10 <c>\n"
    "R 11 [1] <>\n"
    "S 11 <>\n";

// CFG_17
//   constant: the same over CONFIGS[17].
static const char CFG_17[] =
    "cfg 17: 2 2 1 1\n"
    "A 0 [1] <a> added 1\n"
    "A 1 [1] <a,b> added 1\n"
    "A 2 [1] <a,,b,> added 1\n"
    "A 3 [1] <,a> added 1\n"
    "A 4 [2] <a> <b> added 2\n"
    "A 5 [1] <[a]> added 1\n"
    "A 6 [1] <a> added 1\n"
    "A 7 [1] <[]> added 1\n"
    "A 8 [2] <\"a,b\"> <c> added 2\n"
    "A 9 [0] added 0\n"
    "A 10 [3] <a> <b> <c> added 3\n"
    "A 11 [2] <a;b> <c> added 2\n"
    "A 12 [1] <> added 1\n"
    "A 13 [1] <[> added 1\n"
    "A 14 [1] <]> added 1\n"
    "A 15 [2] <a > <b> added 2\n"
    "A 16 [2] <(a,b)> <c> added 2\n"
    "A 17 [3] <'x,y'> <z> <w> added 3\n"
    "A 18 [2] <a<b> <c> added 2\n"
    "A 19 [2] <\"a\\\",b\"> <c> added 2\n"
    "A 20 [1] <{}> added 1\n"
    "A 21 [2] <{a,b}> <c> added 2\n"
    "A 22 [2] <x> <]> added 2\n"
    "R 0 [1] <1>\n"
    "S 0 <1>\n"
    "R 1 [2] <1> <2>\n"
    "S 1 <1>\n"
    "R 2 [2] <1> <2>\n"
    "S 2 <1>\n"
    "R 3 [0]\n"
    "S 3 <>\n"
    "R 4 [2] <{}> <%%>\n"
    "S 4 <{}>\n"
    "R 5 [2] <{}> <%%>\n"
    "S 5 <{}>\n"
    "R 6 [2] <a> <>\n"
    "S 6 <a>\n"
    "R 7 [2] <x\n"
    "> <y>\n"
    "S 7 <x\n"
    ">\n"
    "R 8 [2] <1.5> <2>\n"
    "S 8 <1.5>\n"
    "R 9 [2] <true> <2>\n"
    "S 9 <true>\n"
    "R 10 [2] <a> <b>\n"
    "S 10 <a>\n"
    "R 11 [1] <>\n"
    "S 11 <>\n";

// CFG_18
//   constant: the same over CONFIGS[18].
static const char CFG_18[] =
    "cfg 18: 1 1 0 0\n"
    "A 0 [1] <a> added 1\n"
    "A 1 [1] <a,b> added 1\n"
    "A 2 [1] <a,,b,> added 1\n"
    "A 3 [1] <,a> added 1\n"
    "A 4 [1] <[a,b]> added 1\n"
    "A 5 [1] <[a]> added 1\n"
    "A 6 [1] <[[a]]> added 1\n"
    "A 7 [1] <[]> added 1\n"
    "A 8 [1] <[\"a,b\",c]> added 1\n"
    "A 9 [1] <[]> added 1\n"
    "A 10 [1] <[[a,b],[c]]> added 1\n"
    "A 11 [1] <[a;b,c]> added 1\n"
    "A 12 [1] <> added 1\n"
    "A 13 [1] <[> added 1\n"
    "A 14 [1] <]> added 1\n"
    "A 15 [1] <[ a , b ]> added 1\n"
    "A 16 [1] <[(a,b),c]> added 1\n"
    "A 17 [1] <['x,y'z,w]> added 1\n"
    "A 18 [1] <[a<b,c]> added 1\n"
    "A 19 [1] <[\"a\\\",b\",c]> added 1\n"
    "A 20 [1] <{}> added 1\n"
    "A 21 [1] <[{a,b},c]> added 1\n"
    "A 22 [1] <[[x]]]> added 1\n"
    "R 0 [1] <1>\n"
    "S 0 <1>\n"
    "R 1 ERR ArgumentMismatch 114: --o18: At most 1 required but received 2\n"
    "S 1 ERR ArgumentMismatch 114: --o18: At most 1 required but received 2\n"
    "R 2 ERR ArgumentMismatch 114: --o18: At most 1 required but received 3\n"
    "S 2 ERR ArgumentMismatch 114: --o18: At most 1 required but received 3\n"
    "R 3 [0]\n"
    "S 3 <>\n"
    "R 4 [1] <{}>\n"
    "S 4 <{}>\n"
    "R 5 [2] <{}> <%%>\n"
    "S 5 <{}>\n"
    "R 6 ERR ArgumentMismatch 114: --o18: At most 1 required but received 2\n"
    "S 6 ERR ArgumentMismatch 114: --o18: At most 1 required but received 2\n"
    "R 7 ERR ArgumentMismatch 114: --o18: At most 1 required but received 2\n"
    "S 7 ERR ArgumentMismatch 114: --o18: At most 1 required but received 2\n"
    "R 8 ERR ArgumentMismatch 114: --o18: At most 1 required but received 2\n"
    "S 8 ERR ArgumentMismatch 114: --o18: At most 1 required but received 2\n"
    "R 9 ERR ArgumentMismatch 114: --o18: At most 1 required but received 2\n"
    "S 9 ERR ArgumentMismatch 114: --o18: At most 1 required but received 2\n"
    "R 10 ERR ArgumentMismatch 114: --o18: At most 1 required but received 4\n"
    "S 10 ERR ArgumentMismatch 114: --o18: At most 1 required but received 4\n"
    "R 11 [1] <>\n"
    "S 11 <>\n";

// AS
//   constant: CLI11's as<std::vector<std::string>> and as<std::string> over
// AS_CASES, but the second for AS_UNDEFINED.
static const char AS[] =
    "V0 [1] <d>\n"
    "S0 <d>\n"
    "V1 [2] <a> <b>\n"
    "S1 <a>\n"
    "V2 [1] <a\n"
    "b>\n"
    "S2 <a\n"
    "b>\n"
    "V3 [1] <[a,b]>\n"
    "S3 <[a,b]>\n"
    "V4 [3] <a> <b> <c>\n"
    "S4 <a>\n"
    "V5 [2] <a> <b>\n"
    "S5 <a>\n"
    "V6 [2] <b> <c>\n"
    "S6 <b>\n"
    "V7 ERR ArgumentMismatch 114: --a7: At most 2 required but received 3\n"
    "S7 ERR ArgumentMismatch 114: --a7: At most 2 required but received 3\n"
    "V8 [2] <c> <b>\n"
    "S8 <c>\n"
    "V9 [1] <6>\n"
    "S9 <6>\n"
    "V10 ERR ArgumentMismatch 114: --a10: At least 1 required but received 0\n"
    "S10 ERR ArgumentMismatch 114: --a10: At least 1 required but received 0\n"
    "V11 [0]\n"
    "V12 [1] <>\n"
    "S12 <>\n"
    "V13 [1] <v>\n"
    "S13 <v>\n"
    "V14 ERR ArgumentMismatch 114: --a14: At most 1 required but received 2\n"
    "S14 ERR ArgumentMismatch 114: --a14: At most 1 required but received 2\n";

// FLAGS_0_A
//   constant: CLI11's get_flag_value of FLAG_DEFINITION, made as
// FLAG_CONFIGS[0] says, for the first half of FLAG_ASKED with each of
// FLAG_INPUTS.
static const char FLAGS_0_A[] =
    "[flag] [] <true>\n"
    "[flag] [{}] <true>\n"
    "[flag] [true] <true>\n"
    "[flag] [false] <false>\n"
    "[flag] [1] <1>\n"
    "[flag] [-1] <-1>\n"
    "[flag] [2] <2>\n"
    "[flag] [-2] <-2>\n"
    "[flag] [abc] <abc>\n"
    "[flag] [0] <0>\n"
    "[flag] [5] <5>\n"
    "[flag] [9] <9>\n"
    "[flag] [ 3] < 3>\n"
    "[flag] [-9223372036854775807] <-9223372036854775807>\n"
    "[no-flag] [] <false>\n"
    "[no-flag] [{}] <false>\n"
    "[no-flag] [true] <false>\n"
    "[no-flag] [false] <true>\n"
    "[no-flag] [1] <false>\n"
    "[no-flag] [-1] <true>\n"
    "[no-flag] [2] <-2>\n"
    "[no-flag] [-2] <2>\n"
    "[no-flag] [abc] <abc>\n"
    "[no-flag] [0] <true>\n"
    "[no-flag] [5] <-5>\n"
    "[no-flag] [9] <-9>\n"
    "[no-flag] [ 3] <-3>\n"
    "[no-flag] [-9223372036854775807] <9223372036854775807>\n"
    "[f] [] <2>\n"
    "[f] [{}] <2>\n"
    "[f] [true] <true>\n"
    "[f] [false] <false>\n"
    "[f] [1] <1>\n"
    "[f] [-1] <-1>\n"
    "[f] [2] <2>\n"
    "[f] [-2] <-2>\n"
    "[f] [abc] <abc>\n"
    "[f] [0] <0>\n"
    "[f] [5] <5>\n"
    "[f] [9] <9>\n"
    "[f] [ 3] < 3>\n"
    "[f] [-9223372036854775807] <-9223372036854775807>\n";

// FLAGS_0_B
//   constant: the same for the second half of FLAG_ASKED.
static const char FLAGS_0_B[] =
    "[x] [] <true>\n"
    "[x] [{}] <true>\n"
    "[x] [true] <true>\n"
    "[x] [false] <false>\n"
    "[x] [1] <1>\n"
    "[x] [-1] <-1>\n"
    "[x] [2] <2>\n"
    "[x] [-2] <-2>\n"
    "[x] [abc] <abc>\n"
    "[x] [0] <0>\n"
    "[x] [5] <5>\n"
    "[x] [9] <9>\n"
    "[x] [ 3] < 3>\n"
    "[x] [-9223372036854775807] <-9223372036854775807>\n"
    "[NO-FLAG] [] <true>\n"
    "[NO-FLAG] [{}] <true>\n"
    "[NO-FLAG] [true] <true>\n"
    "[NO-FLAG] [false] <false>\n"
    "[NO-FLAG] [1] <1>\n"
    "[NO-FLAG] [-1] <-1>\n"
    "[NO-FLAG] [2] <2>\n"
    "[NO-FLAG] [-2] <-2>\n"
    "[NO-FLAG] [abc] <abc>\n"
    "[NO-FLAG] [0] <0>\n"
    "[NO-FLAG] [5] <5>\n"
    "[NO-FLAG] [9] <9>\n"
    "[NO-FLAG] [ 3] < 3>\n"
    "[NO-FLAG] [-9223372036854775807] <-9223372036854775807>\n"
    "[no_flag] [] <true>\n"
    "[no_flag] [{}] <true>\n"
    "[no_flag] [true] <true>\n"
    "[no_flag] [false] <false>\n"
    "[no_flag] [1] <1>\n"
    "[no_flag] [-1] <-1>\n"
    "[no_flag] [2] <2>\n"
    "[no_flag] [-2] <-2>\n"
    "[no_flag] [abc] <abc>\n"
    "[no_flag] [0] <0>\n"
    "[no_flag] [5] <5>\n"
    "[no_flag] [9] <9>\n"
    "[no_flag] [ 3] < 3>\n"
    "[no_flag] [-9223372036854775807] <-9223372036854775807>\n";

// FLAGS_1_A
//   constant: the same as FLAGS_0_A, with FLAG_CONFIGS[1].
static const char FLAGS_1_A[] =
    "[flag] [] <true>\n"
    "[flag] [{}] <true>\n"
    "[flag] [true] <true>\n"
    "[flag] [false] ERR ArgumentMismatch 114: flag was given a disallowed flag "
    "override\n"
    "[flag] [1] ERR ArgumentMismatch 114: flag was given a disallowed flag "
    "override\n"
    "[flag] [-1] ERR ArgumentMismatch 114: flag was given a disallowed flag "
    "override\n"
    "[flag] [2] ERR ArgumentMismatch 114: flag was given a disallowed flag "
    "override\n"
    "[flag] [-2] ERR ArgumentMismatch 114: flag was given a disallowed flag "
    "override\n"
    "[flag] [abc] ERR ArgumentMismatch 114: flag was given a disallowed flag "
    "override\n"
    "[flag] [0] ERR ArgumentMismatch 114: flag was given a disallowed flag "
    "override\n"
    "[flag] [5] ERR ArgumentMismatch 114: flag was given a disallowed flag "
    "override\n"
    "[flag] [9] ERR ArgumentMismatch 114: flag was given a disallowed flag "
    "override\n"
    "[flag] [ 3] ERR ArgumentMismatch 114: flag was given a disallowed flag "
    "override\n"
    "[flag] [-9223372036854775807] ERR ArgumentMismatch 114: flag was given a "
    "disallowed flag override\n"
    "[no-flag] [] <false>\n"
    "[no-flag] [{}] <false>\n"
    "[no-flag] [true] ERR ArgumentMismatch 114: no-flag was given a disallowed "
    "flag override\n"
    "[no-flag] [false] <true>\n"
    "[no-flag] [1] ERR ArgumentMismatch 114: no-flag was given a disallowed "
    "flag override\n"
    "[no-flag] [-1] ERR ArgumentMismatch 114: no-flag was given a disallowed "
    "flag override\n"
    "[no-flag] [2] ERR ArgumentMismatch 114: no-flag was given a disallowed "
    "flag override\n"
    "[no-flag] [-2] ERR ArgumentMismatch 114: no-flag was given a disallowed "
    "flag override\n"
    "[no-flag] [abc] ERR ArgumentMismatch 114: no-flag was given a disallowed "
    "flag override\n"
    "[no-flag] [0] ERR ArgumentMismatch 114: no-flag was given a disallowed "
    "flag override\n"
    "[no-flag] [5] ERR ArgumentMismatch 114: no-flag was given a disallowed "
    "flag override\n"
    "[no-flag] [9] ERR ArgumentMismatch 114: no-flag was given a disallowed "
    "flag override\n"
    "[no-flag] [ 3] ERR ArgumentMismatch 114: no-flag was given a disallowed "
    "flag override\n"
    "[no-flag] [-9223372036854775807] ERR ArgumentMismatch 114: no-flag was "
    "given a disallowed flag override\n"
    "[f] [] <2>\n"
    "[f] [{}] <2>\n"
    "[f] [true] ERR ArgumentMismatch 114: f was given a disallowed flag "
    "override\n"
    "[f] [false] ERR ArgumentMismatch 114: f was given a disallowed flag "
    "override\n"
    "[f] [1] ERR ArgumentMismatch 114: f was given a disallowed flag override\n"
    "[f] [-1] ERR ArgumentMismatch 114: f was given a disallowed flag "
    "override\n"
    "[f] [2] <2>\n"
    "[f] [-2] ERR ArgumentMismatch 114: f was given a disallowed flag "
    "override\n"
    "[f] [abc] ERR ArgumentMismatch 114: f was given a disallowed flag "
    "override\n"
    "[f] [0] ERR ArgumentMismatch 114: f was given a disallowed flag override\n"
    "[f] [5] ERR ArgumentMismatch 114: f was given a disallowed flag override\n"
    "[f] [9] ERR ArgumentMismatch 114: f was given a disallowed flag override\n"
    "[f] [ 3] ERR ArgumentMismatch 114: f was given a disallowed flag "
    "override\n"
    "[f] [-9223372036854775807] ERR ArgumentMismatch 114: f was given a "
    "disallowed flag override\n";

// FLAGS_1_B
//   constant: the same as FLAGS_0_B, with FLAG_CONFIGS[1].
static const char FLAGS_1_B[] =
    "[x] [] <true>\n"
    "[x] [{}] <true>\n"
    "[x] [true] <true>\n"
    "[x] [false] ERR ArgumentMismatch 114: x was given a disallowed flag "
    "override\n"
    "[x] [1] ERR ArgumentMismatch 114: x was given a disallowed flag override\n"
    "[x] [-1] ERR ArgumentMismatch 114: x was given a disallowed flag "
    "override\n"
    "[x] [2] ERR ArgumentMismatch 114: x was given a disallowed flag override\n"
    "[x] [-2] ERR ArgumentMismatch 114: x was given a disallowed flag "
    "override\n"
    "[x] [abc] ERR ArgumentMismatch 114: x was given a disallowed flag "
    "override\n"
    "[x] [0] ERR ArgumentMismatch 114: x was given a disallowed flag override\n"
    "[x] [5] ERR ArgumentMismatch 114: x was given a disallowed flag override\n"
    "[x] [9] ERR ArgumentMismatch 114: x was given a disallowed flag override\n"
    "[x] [ 3] ERR ArgumentMismatch 114: x was given a disallowed flag "
    "override\n"
    "[x] [-9223372036854775807] ERR ArgumentMismatch 114: x was given a "
    "disallowed flag override\n"
    "[NO-FLAG] [] <true>\n"
    "[NO-FLAG] [{}] <true>\n"
    "[NO-FLAG] [true] <true>\n"
    "[NO-FLAG] [false] ERR ArgumentMismatch 114: NO-FLAG was given a "
    "disallowed flag override\n"
    "[NO-FLAG] [1] ERR ArgumentMismatch 114: NO-FLAG was given a disallowed "
    "flag override\n"
    "[NO-FLAG] [-1] ERR ArgumentMismatch 114: NO-FLAG was given a disallowed "
    "flag override\n"
    "[NO-FLAG] [2] ERR ArgumentMismatch 114: NO-FLAG was given a disallowed "
    "flag override\n"
    "[NO-FLAG] [-2] ERR ArgumentMismatch 114: NO-FLAG was given a disallowed "
    "flag override\n"
    "[NO-FLAG] [abc] ERR ArgumentMismatch 114: NO-FLAG was given a disallowed "
    "flag override\n"
    "[NO-FLAG] [0] ERR ArgumentMismatch 114: NO-FLAG was given a disallowed "
    "flag override\n"
    "[NO-FLAG] [5] ERR ArgumentMismatch 114: NO-FLAG was given a disallowed "
    "flag override\n"
    "[NO-FLAG] [9] ERR ArgumentMismatch 114: NO-FLAG was given a disallowed "
    "flag override\n"
    "[NO-FLAG] [ 3] ERR ArgumentMismatch 114: NO-FLAG was given a disallowed "
    "flag override\n"
    "[NO-FLAG] [-9223372036854775807] ERR ArgumentMismatch 114: NO-FLAG was "
    "given a disallowed flag override\n"
    "[no_flag] [] <true>\n"
    "[no_flag] [{}] <true>\n"
    "[no_flag] [true] <true>\n"
    "[no_flag] [false] ERR ArgumentMismatch 114: no_flag was given a "
    "disallowed flag override\n"
    "[no_flag] [1] ERR ArgumentMismatch 114: no_flag was given a disallowed "
    "flag override\n"
    "[no_flag] [-1] ERR ArgumentMismatch 114: no_flag was given a disallowed "
    "flag override\n"
    "[no_flag] [2] ERR ArgumentMismatch 114: no_flag was given a disallowed "
    "flag override\n"
    "[no_flag] [-2] ERR ArgumentMismatch 114: no_flag was given a disallowed "
    "flag override\n"
    "[no_flag] [abc] ERR ArgumentMismatch 114: no_flag was given a disallowed "
    "flag override\n"
    "[no_flag] [0] ERR ArgumentMismatch 114: no_flag was given a disallowed "
    "flag override\n"
    "[no_flag] [5] ERR ArgumentMismatch 114: no_flag was given a disallowed "
    "flag override\n"
    "[no_flag] [9] ERR ArgumentMismatch 114: no_flag was given a disallowed "
    "flag override\n"
    "[no_flag] [ 3] ERR ArgumentMismatch 114: no_flag was given a disallowed "
    "flag override\n"
    "[no_flag] [-9223372036854775807] ERR ArgumentMismatch 114: no_flag was "
    "given a disallowed flag override\n";

// FLAGS_2_A
//   constant: the same as FLAGS_0_A, with FLAG_CONFIGS[2].
static const char FLAGS_2_A[] =
    "[flag] [] <dflt>\n"
    "[flag] [{}] <dflt>\n"
    "[flag] [true] <true>\n"
    "[flag] [false] <false>\n"
    "[flag] [1] <1>\n"
    "[flag] [-1] <-1>\n"
    "[flag] [2] <2>\n"
    "[flag] [-2] <-2>\n"
    "[flag] [abc] <abc>\n"
    "[flag] [0] <0>\n"
    "[flag] [5] <5>\n"
    "[flag] [9] <9>\n"
    "[flag] [ 3] < 3>\n"
    "[flag] [-9223372036854775807] <-9223372036854775807>\n"
    "[no-flag] [] <false>\n"
    "[no-flag] [{}] <false>\n"
    "[no-flag] [true] <false>\n"
    "[no-flag] [false] <true>\n"
    "[no-flag] [1] <false>\n"
    "[no-flag] [-1] <true>\n"
    "[no-flag] [2] <-2>\n"
    "[no-flag] [-2] <2>\n"
    "[no-flag] [abc] <abc>\n"
    "[no-flag] [0] <true>\n"
    "[no-flag] [5] <-5>\n"
    "[no-flag] [9] <-9>\n"
    "[no-flag] [ 3] <-3>\n"
    "[no-flag] [-9223372036854775807] <9223372036854775807>\n"
    "[f] [] <2>\n"
    "[f] [{}] <2>\n"
    "[f] [true] <true>\n"
    "[f] [false] <false>\n"
    "[f] [1] <1>\n"
    "[f] [-1] <-1>\n"
    "[f] [2] <2>\n"
    "[f] [-2] <-2>\n"
    "[f] [abc] <abc>\n"
    "[f] [0] <0>\n"
    "[f] [5] <5>\n"
    "[f] [9] <9>\n"
    "[f] [ 3] < 3>\n"
    "[f] [-9223372036854775807] <-9223372036854775807>\n";

// FLAGS_2_B
//   constant: the same as FLAGS_0_B, with FLAG_CONFIGS[2].
static const char FLAGS_2_B[] =
    "[x] [] <dflt>\n"
    "[x] [{}] <dflt>\n"
    "[x] [true] <true>\n"
    "[x] [false] <false>\n"
    "[x] [1] <1>\n"
    "[x] [-1] <-1>\n"
    "[x] [2] <2>\n"
    "[x] [-2] <-2>\n"
    "[x] [abc] <abc>\n"
    "[x] [0] <0>\n"
    "[x] [5] <5>\n"
    "[x] [9] <9>\n"
    "[x] [ 3] < 3>\n"
    "[x] [-9223372036854775807] <-9223372036854775807>\n"
    "[NO-FLAG] [] <dflt>\n"
    "[NO-FLAG] [{}] <dflt>\n"
    "[NO-FLAG] [true] <true>\n"
    "[NO-FLAG] [false] <false>\n"
    "[NO-FLAG] [1] <1>\n"
    "[NO-FLAG] [-1] <-1>\n"
    "[NO-FLAG] [2] <2>\n"
    "[NO-FLAG] [-2] <-2>\n"
    "[NO-FLAG] [abc] <abc>\n"
    "[NO-FLAG] [0] <0>\n"
    "[NO-FLAG] [5] <5>\n"
    "[NO-FLAG] [9] <9>\n"
    "[NO-FLAG] [ 3] < 3>\n"
    "[NO-FLAG] [-9223372036854775807] <-9223372036854775807>\n"
    "[no_flag] [] <dflt>\n"
    "[no_flag] [{}] <dflt>\n"
    "[no_flag] [true] <true>\n"
    "[no_flag] [false] <false>\n"
    "[no_flag] [1] <1>\n"
    "[no_flag] [-1] <-1>\n"
    "[no_flag] [2] <2>\n"
    "[no_flag] [-2] <-2>\n"
    "[no_flag] [abc] <abc>\n"
    "[no_flag] [0] <0>\n"
    "[no_flag] [5] <5>\n"
    "[no_flag] [9] <9>\n"
    "[no_flag] [ 3] < 3>\n"
    "[no_flag] [-9223372036854775807] <-9223372036854775807>\n";

// FLAGS_3_A
//   constant: the same as FLAGS_0_A, with FLAG_CONFIGS[3].
static const char FLAGS_3_A[] =
    "[flag] [] <true>\n"
    "[flag] [{}] <true>\n"
    "[flag] [true] <true>\n"
    "[flag] [false] ERR ArgumentMismatch 114: flag was given a disallowed flag "
    "override\n"
    "[flag] [1] ERR ArgumentMismatch 114: flag was given a disallowed flag "
    "override\n"
    "[flag] [-1] ERR ArgumentMismatch 114: flag was given a disallowed flag "
    "override\n"
    "[flag] [2] ERR ArgumentMismatch 114: flag was given a disallowed flag "
    "override\n"
    "[flag] [-2] ERR ArgumentMismatch 114: flag was given a disallowed flag "
    "override\n"
    "[flag] [abc] ERR ArgumentMismatch 114: flag was given a disallowed flag "
    "override\n"
    "[flag] [0] ERR ArgumentMismatch 114: flag was given a disallowed flag "
    "override\n"
    "[flag] [5] ERR ArgumentMismatch 114: flag was given a disallowed flag "
    "override\n"
    "[flag] [9] ERR ArgumentMismatch 114: flag was given a disallowed flag "
    "override\n"
    "[flag] [ 3] ERR ArgumentMismatch 114: flag was given a disallowed flag "
    "override\n"
    "[flag] [-9223372036854775807] ERR ArgumentMismatch 114: flag was given a "
    "disallowed flag override\n"
    "[no-flag] [] <false>\n"
    "[no-flag] [{}] <false>\n"
    "[no-flag] [true] ERR ArgumentMismatch 114: no-flag was given a disallowed "
    "flag override\n"
    "[no-flag] [false] <true>\n"
    "[no-flag] [1] ERR ArgumentMismatch 114: no-flag was given a disallowed "
    "flag override\n"
    "[no-flag] [-1] ERR ArgumentMismatch 114: no-flag was given a disallowed "
    "flag override\n"
    "[no-flag] [2] ERR ArgumentMismatch 114: no-flag was given a disallowed "
    "flag override\n"
    "[no-flag] [-2] ERR ArgumentMismatch 114: no-flag was given a disallowed "
    "flag override\n"
    "[no-flag] [abc] ERR ArgumentMismatch 114: no-flag was given a disallowed "
    "flag override\n"
    "[no-flag] [0] ERR ArgumentMismatch 114: no-flag was given a disallowed "
    "flag override\n"
    "[no-flag] [5] ERR ArgumentMismatch 114: no-flag was given a disallowed "
    "flag override\n"
    "[no-flag] [9] <9>\n"
    "[no-flag] [ 3] ERR ArgumentMismatch 114: no-flag was given a disallowed "
    "flag override\n"
    "[no-flag] [-9223372036854775807] ERR ArgumentMismatch 114: no-flag was "
    "given a disallowed flag override\n"
    "[f] [] <2>\n"
    "[f] [{}] <2>\n"
    "[f] [true] ERR ArgumentMismatch 114: f was given a disallowed flag "
    "override\n"
    "[f] [false] ERR ArgumentMismatch 114: f was given a disallowed flag "
    "override\n"
    "[f] [1] ERR ArgumentMismatch 114: f was given a disallowed flag override\n"
    "[f] [-1] ERR ArgumentMismatch 114: f was given a disallowed flag "
    "override\n"
    "[f] [2] <2>\n"
    "[f] [-2] ERR ArgumentMismatch 114: f was given a disallowed flag "
    "override\n"
    "[f] [abc] ERR ArgumentMismatch 114: f was given a disallowed flag "
    "override\n"
    "[f] [0] ERR ArgumentMismatch 114: f was given a disallowed flag override\n"
    "[f] [5] ERR ArgumentMismatch 114: f was given a disallowed flag override\n"
    "[f] [9] <9>\n"
    "[f] [ 3] ERR ArgumentMismatch 114: f was given a disallowed flag "
    "override\n"
    "[f] [-9223372036854775807] ERR ArgumentMismatch 114: f was given a "
    "disallowed flag override\n";

// FLAGS_3_B
//   constant: the same as FLAGS_0_B, with FLAG_CONFIGS[3].
static const char FLAGS_3_B[] =
    "[x] [] <true>\n"
    "[x] [{}] <true>\n"
    "[x] [true] <true>\n"
    "[x] [false] ERR ArgumentMismatch 114: x was given a disallowed flag "
    "override\n"
    "[x] [1] ERR ArgumentMismatch 114: x was given a disallowed flag override\n"
    "[x] [-1] ERR ArgumentMismatch 114: x was given a disallowed flag "
    "override\n"
    "[x] [2] ERR ArgumentMismatch 114: x was given a disallowed flag override\n"
    "[x] [-2] ERR ArgumentMismatch 114: x was given a disallowed flag "
    "override\n"
    "[x] [abc] ERR ArgumentMismatch 114: x was given a disallowed flag "
    "override\n"
    "[x] [0] ERR ArgumentMismatch 114: x was given a disallowed flag override\n"
    "[x] [5] ERR ArgumentMismatch 114: x was given a disallowed flag override\n"
    "[x] [9] ERR ArgumentMismatch 114: x was given a disallowed flag override\n"
    "[x] [ 3] ERR ArgumentMismatch 114: x was given a disallowed flag "
    "override\n"
    "[x] [-9223372036854775807] ERR ArgumentMismatch 114: x was given a "
    "disallowed flag override\n"
    "[NO-FLAG] [] <true>\n"
    "[NO-FLAG] [{}] <true>\n"
    "[NO-FLAG] [true] <true>\n"
    "[NO-FLAG] [false] ERR ArgumentMismatch 114: NO-FLAG was given a "
    "disallowed flag override\n"
    "[NO-FLAG] [1] ERR ArgumentMismatch 114: NO-FLAG was given a disallowed "
    "flag override\n"
    "[NO-FLAG] [-1] ERR ArgumentMismatch 114: NO-FLAG was given a disallowed "
    "flag override\n"
    "[NO-FLAG] [2] ERR ArgumentMismatch 114: NO-FLAG was given a disallowed "
    "flag override\n"
    "[NO-FLAG] [-2] ERR ArgumentMismatch 114: NO-FLAG was given a disallowed "
    "flag override\n"
    "[NO-FLAG] [abc] ERR ArgumentMismatch 114: NO-FLAG was given a disallowed "
    "flag override\n"
    "[NO-FLAG] [0] ERR ArgumentMismatch 114: NO-FLAG was given a disallowed "
    "flag override\n"
    "[NO-FLAG] [5] ERR ArgumentMismatch 114: NO-FLAG was given a disallowed "
    "flag override\n"
    "[NO-FLAG] [9] ERR ArgumentMismatch 114: NO-FLAG was given a disallowed "
    "flag override\n"
    "[NO-FLAG] [ 3] ERR ArgumentMismatch 114: NO-FLAG was given a disallowed "
    "flag override\n"
    "[NO-FLAG] [-9223372036854775807] ERR ArgumentMismatch 114: NO-FLAG was "
    "given a disallowed flag override\n"
    "[no_flag] [] <true>\n"
    "[no_flag] [{}] <true>\n"
    "[no_flag] [true] <true>\n"
    "[no_flag] [false] ERR ArgumentMismatch 114: no_flag was given a "
    "disallowed flag override\n"
    "[no_flag] [1] ERR ArgumentMismatch 114: no_flag was given a disallowed "
    "flag override\n"
    "[no_flag] [-1] ERR ArgumentMismatch 114: no_flag was given a disallowed "
    "flag override\n"
    "[no_flag] [2] ERR ArgumentMismatch 114: no_flag was given a disallowed "
    "flag override\n"
    "[no_flag] [-2] ERR ArgumentMismatch 114: no_flag was given a disallowed "
    "flag override\n"
    "[no_flag] [abc] ERR ArgumentMismatch 114: no_flag was given a disallowed "
    "flag override\n"
    "[no_flag] [0] ERR ArgumentMismatch 114: no_flag was given a disallowed "
    "flag override\n"
    "[no_flag] [5] ERR ArgumentMismatch 114: no_flag was given a disallowed "
    "flag override\n"
    "[no_flag] [9] ERR ArgumentMismatch 114: no_flag was given a disallowed "
    "flag override\n"
    "[no_flag] [ 3] ERR ArgumentMismatch 114: no_flag was given a disallowed "
    "flag override\n"
    "[no_flag] [-9223372036854775807] ERR ArgumentMismatch 114: no_flag was "
    "given a disallowed flag override\n";

// FLAGS_4_A
//   constant: the same as FLAGS_0_A, with FLAG_CONFIGS[4].
static const char FLAGS_4_A[] =
    "[flag] [] <true>\n"
    "[flag] [{}] <true>\n"
    "[flag] [true] <true>\n"
    "[flag] [false] <false>\n"
    "[flag] [1] <1>\n"
    "[flag] [-1] <-1>\n"
    "[flag] [2] <2>\n"
    "[flag] [-2] <-2>\n"
    "[flag] [abc] <abc>\n"
    "[flag] [0] <0>\n"
    "[flag] [5] <5>\n"
    "[flag] [9] <9>\n"
    "[flag] [ 3] < 3>\n"
    "[flag] [-9223372036854775807] <-9223372036854775807>\n"
    "[no-flag] [] <false>\n"
    "[no-flag] [{}] <false>\n"
    "[no-flag] [true] <false>\n"
    "[no-flag] [false] <true>\n"
    "[no-flag] [1] <false>\n"
    "[no-flag] [-1] <true>\n"
    "[no-flag] [2] <-2>\n"
    "[no-flag] [-2] <2>\n"
    "[no-flag] [abc] <abc>\n"
    "[no-flag] [0] <true>\n"
    "[no-flag] [5] <-5>\n"
    "[no-flag] [9] <-9>\n"
    "[no-flag] [ 3] <-3>\n"
    "[no-flag] [-9223372036854775807] <9223372036854775807>\n"
    "[f] [] <2>\n"
    "[f] [{}] <2>\n"
    "[f] [true] <true>\n"
    "[f] [false] <false>\n"
    "[f] [1] <1>\n"
    "[f] [-1] <-1>\n"
    "[f] [2] <2>\n"
    "[f] [-2] <-2>\n"
    "[f] [abc] <abc>\n"
    "[f] [0] <0>\n"
    "[f] [5] <5>\n"
    "[f] [9] <9>\n"
    "[f] [ 3] < 3>\n"
    "[f] [-9223372036854775807] <-9223372036854775807>\n";

// FLAGS_4_B
//   constant: the same as FLAGS_0_B, with FLAG_CONFIGS[4].
static const char FLAGS_4_B[] =
    "[x] [] <true>\n"
    "[x] [{}] <true>\n"
    "[x] [true] <true>\n"
    "[x] [false] <false>\n"
    "[x] [1] <1>\n"
    "[x] [-1] <-1>\n"
    "[x] [2] <2>\n"
    "[x] [-2] <-2>\n"
    "[x] [abc] <abc>\n"
    "[x] [0] <0>\n"
    "[x] [5] <5>\n"
    "[x] [9] <9>\n"
    "[x] [ 3] < 3>\n"
    "[x] [-9223372036854775807] <-9223372036854775807>\n"
    "[NO-FLAG] [] <false>\n"
    "[NO-FLAG] [{}] <false>\n"
    "[NO-FLAG] [true] <false>\n"
    "[NO-FLAG] [false] <true>\n"
    "[NO-FLAG] [1] <false>\n"
    "[NO-FLAG] [-1] <true>\n"
    "[NO-FLAG] [2] <-2>\n"
    "[NO-FLAG] [-2] <2>\n"
    "[NO-FLAG] [abc] <abc>\n"
    "[NO-FLAG] [0] <true>\n"
    "[NO-FLAG] [5] <-5>\n"
    "[NO-FLAG] [9] <-9>\n"
    "[NO-FLAG] [ 3] <-3>\n"
    "[NO-FLAG] [-9223372036854775807] <9223372036854775807>\n"
    "[no_flag] [] <true>\n"
    "[no_flag] [{}] <true>\n"
    "[no_flag] [true] <true>\n"
    "[no_flag] [false] <false>\n"
    "[no_flag] [1] <1>\n"
    "[no_flag] [-1] <-1>\n"
    "[no_flag] [2] <2>\n"
    "[no_flag] [-2] <-2>\n"
    "[no_flag] [abc] <abc>\n"
    "[no_flag] [0] <0>\n"
    "[no_flag] [5] <5>\n"
    "[no_flag] [9] <9>\n"
    "[no_flag] [ 3] < 3>\n"
    "[no_flag] [-9223372036854775807] <-9223372036854775807>\n";

// CALLBACKS
//   constant: CLI11's run_callback over CALLBACK_CASES: what each callback
// was given, how the run ended, whether the callback ran, and the results
// left.
static const char CALLBACKS[] =
    "rc0 callback [1] <b>\n"
    "rc0 ok\n"
    "rc0 run=1 results [2] <a> <b>\n"
    "rc1 callback [1] <d>\n"
    "rc1 ok\n"
    "rc1 run=1 results [0]\n"
    "rc2 callback [1] <d>\n"
    "rc2 ERR ConversionError 104: Could not convert: --rc2 = \n"
    "rc2 run=1 results [0]\n"
    "rc3 callback [2] <x> <y>\n"
    "rc3 ERR ConversionError 104: Could not convert: --rc3 = x,y\n"
    "rc3 run=1 results [2] <x> <y>\n"
    "rc4 callback [2] <a> <>\n"
    "rc4 ERR ConversionError 104: Could not convert: --rc4 = a\n"
    "rc4 run=1 results [2] <a> <>\n"
    "rc5 callback [1] <>\n"
    "rc5 ok\n"
    "rc5 run=1 results [0]\n"
    "rc6 ok\n"
    "rc6 run=1 results [1] <d>\n"
    "rc7 ERR ArgumentMismatch 114: --rc7: At most 1 required but received 2\n"
    "rc7 run=0 results [2] <1> <2>\n"
    "rc8 callback [1] <3>\n"
    "rc8 ok\n"
    "rc8 run=1 results [2] <1> <2>\n"
    "rc9 callback [1] <x>\n"
    "rc9 ERR ConversionError 104: Could not convert:  = x\n"
    "rc9 run=1 results [1] <x>\n"
    "rcA callback [1] <[p,q]>\n"
    "rcA ok\n"
    "rcA run=1 results [0]\n"
    "rcB ERR ArgumentMismatch 114: --rcB: At least 1 required but received 0\n"
    "rcB run=0 results [0]\n"
    "rcC callback [2] <3> <2>\n"
    "rcC ERR ConversionError 104: Could not convert: --rcC = 1,2,3\n"
    "rcC run=1 results [3] <1> <2> <3>\n";

// TEXT
//   constant: CLI11's text of an option after each of its setters: its
// description, option text, group, type name -- with its validators'
// descriptions -- environment variable and name; and of an option bound
// to an `int`.
static const char TEXT[] =
    "new desc=<a description> text=<> group=<OPTIONS> type=<> env=<> "
    "name=<--text>\n"
    "bound desc=<> text=<> group=<OPTIONS> type=<INT> env=<> name=<--bound>\n"
    "description desc=<changed> text=<> group=<OPTIONS> type=<> env=<> "
    "name=<--text>\n"
    "option_text desc=<changed> text=<TEXT ...> group=<OPTIONS> type=<> env=<> "
    "name=<--text>\n"
    "type_name desc=<changed> text=<TEXT ...> group=<OPTIONS> type=<WORD> "
    "env=<> name=<--text>\n"
    "check desc=<changed> text=<TEXT ...> group=<OPTIONS> type=<WORD:POSITIVE> "
    "env=<> name=<--text>\n"
    "transform desc=<changed> text=<TEXT ...> group=<OPTIONS> type=<WORD:INT "
    "in [1 - 9]:POSITIVE> env=<> name=<--text>\n"
    "each desc=<changed> text=<TEXT ...> group=<OPTIONS> type=<WORD:INT in [1 "
    "- 9]:POSITIVE> env=<> name=<--text>\n"
    "type_name_fn desc=<changed> text=<TEXT ...> group=<OPTIONS> type=<FN:INT "
    "in [1 - 9]:POSITIVE> env=<> name=<--text>\n"
    "envname desc=<changed> text=<TEXT ...> group=<OPTIONS> type=<FN:INT in [1 "
    "- 9]:POSITIVE> env=<ENV_T> name=<--text>\n"
    "group desc=<changed> text=<TEXT ...> group=<Other> type=<FN:INT in [1 - "
    "9]:POSITIVE> env=<ENV_T> name=<--text>\n"
    "[group a\\nb] ERR IncorrectConstruction 100: Group names may not contain "
    "newlines or null characters\n"
    "kept desc=<changed> text=<TEXT ...> group=<Other> type=<FN:INT in [1 - "
    "9]:POSITIVE> env=<ENV_T> name=<--text>\n"
    "hidden desc=<changed> text=<TEXT ...> group=<> type=<FN:INT in [1 - "
    "9]:POSITIVE> env=<ENV_T> name=<>\n"
    "cleared desc=<> text=<> group=<OPTIONS> type=<:INT in [1 - 9]:POSITIVE> "
    "env=<> name=<--text>\n";

// DEFAULTS
//   constant: CLI11's default text of an option bound to nothing, one bound
// to an `int` and one bound to text, captured, set as text and set as a
// value, and the variables the values reached.
static const char DEFAULTS[] =
    "new a default=<> results=[0]\n"
    "new b default=<> results=[0]\n"
    "new c default=<> results=[0]\n"
    "capture a default=<> results=[0]\n"
    "capture b default=<3> results=[0]\n"
    "capture b again default=<7> results=[0]\n"
    "capture c default=<start> results=[0]\n"
    "default_str a default=<x> results=[0]\n"
    "default_val a default=<5> results=[1] <r>\n"
    "default_val b default=<12> results=[0]\n"
    "n=12\n"
    "[default_val b x] ERR ConversionError 104: The value --b is not an "
    "allowed value for given default value(\"x\") produces an error : Could "
    "not convert: --b = x\n"
    "kept b default=<12> results=[0]\n"
    "no callback b default=<20> results=[0]\n"
    "n=12\n"
    "default_val c default=<given> results=[0]\n"
    "s=given\n"
    "[default_val a -1] ERR ValidationError 105: --a: Value -1 not in range "
    "[4.94066e-324 - 1.79769e+308]\n"
    "kept a default=<5> results=[1] <r>\n"
    "default_function a default=<fn> results=[1] <r>\n";

// RELATIONS
//   constant: CLI11's needs and excludes of four options, set by option and
// by name, refused, and removed.
static const char RELATIONS[] =
    "new --a needs=[] excludes=[] --b needs=[] excludes=[] -c needs=[] "
    "excludes=[] pos needs=[] excludes=[]\n"
    "needs --a needs=[--b|-c] excludes=[] --b needs=[] excludes=[] -c needs=[] "
    "excludes=[] pos needs=[] excludes=[]\n"
    "[needs --zz] ERR IncorrectConstruction 100: Option --zz is not defined\n"
    "[needs b] ERR IncorrectConstruction 100: Option b is not defined\n"
    "[needs -b] ERR IncorrectConstruction 100: Option -b is not defined\n"
    "[needs ] ERR IncorrectConstruction 100: Option  is not defined\n"
    "[excludes self] ERR IncorrectConstruction 100: and option cannot exclude "
    "itself\n"
    "[excludes --zz] ERR IncorrectConstruction 100: Option --zz is not "
    "defined\n"
    "excludes --a needs=[--b|-c] excludes=[pos] --b needs=[] excludes=[-c] -c "
    "needs=[] excludes=[--b] pos needs=[] excludes=[--a]\n"
    "remove a/b 1 a/b 0 c/b 1 c/b 0 a/c 0\n"
    "removed --a needs=[-c] excludes=[pos] --b needs=[] excludes=[-c] -c "
    "needs=[] excludes=[] pos needs=[] excludes=[--a]\n";

// VALIDATORS
//   constant: CLI11's validators on an option: the type name they make,
// looked up by name and by index, and values run through them; a validator
// applied to one index; and an empty value, checked or not.
static const char VALIDATORS[] =
    "type=<:UPPER:NOX>\n"
    "[get nox] <nox> <NOX>\n"
    "[get upper] <upper> <UPPER>\n"
    "[get bad] <bad> <>\n"
    "[get ] <> <>\n"
    "[get zz] none\n"
    "[get 0] <upper> <UPPER>\n"
    "[get 1] <nox> <NOX>\n"
    "[get 2] <> <>\n"
    "[get 3] <bad> <>\n"
    "[get 4] none\n"
    "reduce [2] <A> <B> results=[2] <A> <B>\n"
    "each <A><B>\n"
    "reduce ERR ValidationError 105: --v: is bad\n"
    "each <BAD>\n"
    "reduce [2] <OK> <X1> results=[2] <OK> <X1>\n"
    "each <OK><X1>\n"
    "reduce [1] <> results=[1] <>\n"
    "each <>\n"
    "index [1] <z> results=[3] <x> <y> <z>\n"
    "index ERR ValidationError 105: --t: x at 0\n"
    "index ERR ValidationError 105: --t: x at 0\n"
    "empty [1] <> results=[1] <>\n"
    "empty ERR ValidationError 105: --f: empty\n";

// BINDING
//   constant: CLI11's options bound to variables of five types: what
// binding made of each, and values run into them.
static const char BINDING[] =
    "int type=<INT> exp=1,1 size=1,1 value=<1>\n"
    "double type=<FLOAT> exp=1,1 size=1,1 value=<0.5>\n"
    "string type=<TEXT> exp=1,1 size=1,1 value=<s>\n"
    "unsigned type=<UINT> exp=1,1 size=1,1 value=<2>\n"
    "long long type=<INT> exp=1,1 size=1,1 value=<3>\n"
    "int [7] <7>\n"
    "int [0x10] <16>\n"
    "int [ 9 ] <9>\n"
    "int [x] ERR ConversionError 104: Could not convert: --i = x\n"
    "int [2147483648] ERR ConversionError 104: Could not convert: --i = "
    "2147483648\n"
    "double [1.5] <1.5>\n"
    "double [1e3] <1000>\n"
    "double [x] ERR ConversionError 104: Could not convert: --d = x\n"
    "string [text] <text>\n"
    "string [] <>\n"
    "unsigned [8] <8>\n"
    "unsigned [-1] ERR ConversionError 104: Could not convert: --u = -1\n"
    "long long [-9223372036854775808] <-9223372036854775808>\n";

// RESULTS
//   constant: CLI11's count, operator bool and get_callback_run of an
// option given values, run, cleared and forced.
static const char RESULTS[] =
    "new count=0 given=0 run=0 results=[0]\n"
    "add a count=1 given=1 run=0 results=[1] <a>\n"
    "add b,c count=3 given=1 run=0 results=[3] <a> <b> <c>\n"
    "run count=3 given=1 run=1 results=[3] <a> <b> <c>\n"
    "add d count=4 given=1 run=0 results=[4] <a> <b> <c> <d>\n"
    "clear count=0 given=0 run=0 results=[0]\n"
    "forced count=0 given=1 run=0 results=[0]\n"
    "forced run count=0 given=1 run=1 results=[0]\n"
    "not forced count=0 given=0 run=1 results=[0]\n"
    "add e f,g count=3 given=1 run=0 results=[3] <e> <f> <g>\n"
    "as 3\n";

// DEPARTED_COUNTS
//   constant: the counts of the options of DEPARTED_STEPS, whose items --
// the type size times the expected count -- an `int` cannot hold: the
// minimum held at D_ARG_EXPECTED_UNLIMITED, as CLI11 holds the maximum.
// CLI11 multiplies the minimum unchecked, and the `int` overflows --
// undefined behaviour, which its builds turn into `items=0,536870912` for
// the first and `items=-2147483648,536870912` for the second: an option
// that takes no values at all, or fewer than none.
static const char DEPARTED_COUNTS[] =
    "type_size(2^20),expected(2^20) exp=1048576,1048576 "
    "type=1048576,1048576 items=536870912,536870912 extra=0 req=0 sep=0\n"
    "type_size(2),expected(2^30) exp=1073741824,1073741824 type=2,2 "
    "items=536870912,536870912 extra=0 req=0 sep=0\n";

// DEPARTED_DEFAULT
//   constant: an `int` bound to an option after the default `x` was refused:
// the 12 it held, kept, where CLI11's conversion leaves the 0 it wrote
// before it failed.
static const char DEPARTED_DEFAULT[] =
    "n=12\n";

// d_tests_arg_option_transcript
//   struct: a transcript, and the name it is compared under.
struct d_tests_arg_option_transcript
{
    const char* name;
    const char* text;
};

// CFG_TRANSCRIPTS
//   constant: CFG_0 to CFG_18, in the order of CONFIGS.
static const struct d_tests_arg_option_transcript CFG_TRANSCRIPTS[] =
{
    { "CFG_0",  CFG_0  }, { "CFG_1",  CFG_1  }, { "CFG_2",  CFG_2  },
    { "CFG_3",  CFG_3  }, { "CFG_4",  CFG_4  }, { "CFG_5",  CFG_5  },
    { "CFG_6",  CFG_6  }, { "CFG_7",  CFG_7  }, { "CFG_8",  CFG_8  },
    { "CFG_9",  CFG_9  }, { "CFG_10", CFG_10 }, { "CFG_11", CFG_11 },
    { "CFG_12", CFG_12 }, { "CFG_13", CFG_13 }, { "CFG_14", CFG_14 },
    { "CFG_15", CFG_15 }, { "CFG_16", CFG_16 }, { "CFG_17", CFG_17 },
    { "CFG_18", CFG_18 }
};

// FLAG_TRANSCRIPTS
//   constant: the flag transcripts, two for each of FLAG_CONFIGS, in order.
static const struct d_tests_arg_option_transcript FLAG_TRANSCRIPTS[] =
{
    { "FLAGS_0_A", FLAGS_0_A }, { "FLAGS_0_B", FLAGS_0_B },
    { "FLAGS_1_A", FLAGS_1_A }, { "FLAGS_1_B", FLAGS_1_B },
    { "FLAGS_2_A", FLAGS_2_A }, { "FLAGS_2_B", FLAGS_2_B },
    { "FLAGS_3_A", FLAGS_3_A }, { "FLAGS_3_B", FLAGS_3_B },
    { "FLAGS_4_A", FLAGS_4_A }, { "FLAGS_4_B", FLAGS_4_B }
};

// FLAG_TRANSCRIPT_COUNT
//   constant: how many transcripts FLAG_TRANSCRIPTS holds.
static const size_t FLAG_TRANSCRIPT_COUNT =
    sizeof(FLAG_TRANSCRIPTS) / sizeof(FLAG_TRANSCRIPTS[0]);

// d_tests_arg_option_called
//   struct: what a callback of CALLBACKS is given beside the values: its
// case, and the transcript it writes to.
struct d_tests_arg_option_called
{
    const struct d_tests_arg_option_callback_case* row;
    struct d_arg_buffer*                           out;
};

/**
 * @brief Appends a refusal as CLI11's exception reads: its name, its exit
 *        code and its message.
 *
 * @param[in,out] _out     the transcript.
 * @param[in]     _status  the status the refusal returned.
 * @param[in]     _error   the report the refusal was recorded in.
 */
static void
d_tests_arg_option_err(
    struct d_arg_buffer*      _out,
    int                       _status,
    const struct d_arg_error* _error
)
{
    (void)d_arg_buffer_printf(_out,
                              "ERR %s %d: %s\n",
                              d_arg_status_name(_status),
                              d_arg_error_exit_code(_error),
                              d_arg_error_message(_error));

    return;
}

/**
 * @brief Appends the definition a run's parser refused, then forgets it, as
 *        catching CLI11's exception lets a program go on.
 *
 * @param[in,out] _out  the transcript.
 * @param[in,out] _run  the run whose parser refused the definition.
 */
static void
d_tests_arg_option_refused(
    struct d_arg_buffer*    _out,
    struct d_tests_arg_run* _run
)
{
    const struct d_arg_error* const error =
        d_arg_parser_definition_error(&_run->parser);

    d_tests_arg_option_err(_out,
                           error->status,
                           error);
    d_arg_parser_clear_definition_error(&_run->parser);

    return;
}

/**
 * @brief Appends values as the reference lists them: `[<count>]`, then each
 *        as ` <value>`, as it is.
 *
 * @param[in,out] _out     the transcript.
 * @param[in]     _values  the values; may be NULL when `_count` is 0.
 * @param[in]     _count   how many.
 */
static void
d_tests_arg_option_list(
    struct d_arg_buffer*      _out,
    const struct d_pack_text* _values,
    size_t                    _count
)
{
    (void)d_arg_buffer_printf(_out,
                              "[%lu]",
                              (unsigned long)_count);

    // each value, between angle brackets
    for (size_t index = 0u; index < _count; ++index)
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       " <");
        (void)d_arg_buffer_append_text(_out,
                                       _values[index]);
        (void)d_arg_buffer_append_char(_out,
                                       '>');
    }

    return;
}

/**
 * @brief Whether a value is a text, exactly.
 *
 * @param[in] _value  the value.
 * @param[in] _text   the text, not empty.
 * @return whether the two are the same.
 */
static bool
d_tests_arg_option_is(
    struct d_pack_text _value,
    const char*        _text
)
{
    const size_t length = strlen(_text);

    return ( (_value.length == length) &&
             (memcmp(_value.data,
                     _text,
                     length) == 0) );
}

/**
 * @brief Adds an option or a flag to a command, as CLI11's add_option or
 *        add_flag.
 *
 * @param[in,out] _app         the command.
 * @param[in]     _definition  the names.
 * @param[in]     _flag        whether a flag is added rather than an option.
 * @return the option; NULL when it was refused.
 */
static struct d_arg_option*
d_tests_arg_option_add(
    struct d_arg_command* _app,
    const char*           _definition,
    bool                  _flag
)
{
    // a flag
    if (_flag)
    {
        return d_arg_command_add_flag(_app,
                                      _definition,
                                      NULL);
    }

    return d_arg_command_add_option(_app,
                                    _definition,
                                    NULL);
}

/**
 * @brief Adds an option named by a prefix and a number, as the reference
 *        names its options: `--c0`, `--o12`.
 *
 * @param[in,out] _app     the command.
 * @param[in]     _prefix  the name's start, with its dashes.
 * @param[in]     _number  the number after it.
 * @return the option; NULL when it could not be made.
 */
static struct d_arg_option*
d_tests_arg_option_numbered(
    struct d_arg_command* _app,
    const char*           _prefix,
    size_t                _number
)
{
    struct d_arg_buffer name = D_ARG_BUFFER_INIT;

    (void)d_arg_buffer_printf(&name,
                              "%s%lu",
                              _prefix,
                              (unsigned long)_number);

    struct d_arg_option* const option =
        d_arg_command_add_option(_app,
                                 d_arg_buffer_text(&name).data,
                                 NULL);

    d_arg_buffer_release(&name);

    return option;
}

/**
 * @brief Gives an option a list of values after clearing it, as CLI11's
 *        clear() and add_result(std::vector<std::string>).
 *
 * @param[in,out] _option  the option.
 * @param[in]     _values  the values, ending in NULL.
 */
static void
d_tests_arg_option_give(
    struct d_arg_option* _option,
    const char* const*   _values
)
{
    d_arg_option_clear(_option);

    // each value, split as one given
    for (size_t index = 0u; _values[index] != NULL; ++index)
    {
        (void)d_arg_option_add_result(_option,
                                      d_pack_text_from_cstr(_values[index]),
                                      NULL);
    }

    return;
}

/**
 * @brief Appends an option's names in several forms, each after its label.
 *
 * @param[in,out] _out     the transcript.
 * @param[in]     _option  the option.
 * @param[in]     _forms   the forms, with their labels.
 * @param[in]     _count   how many forms.
 */
static void
d_tests_arg_option_named(
    struct d_arg_buffer*                  _out,
    const struct d_arg_option*            _option,
    const struct d_tests_arg_option_form* _forms,
    size_t                                _count
)
{
    // each form, after its label
    for (size_t index = 0u; index < _count; ++index)
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       _forms[index].label);
        (void)d_arg_option_name(_option,
                                _forms[index].form,
                                _out);
    }

    return;
}

/**
 * @brief Appends what a definition string makes of names, as NAMES writes
 *        it, the definition given to a parser of its own.
 *
 * @param[in,out] _out         the transcript.
 * @param[in]     _definition  the definition.
 */
static void
d_tests_arg_option_define(
    struct d_arg_buffer* _out,
    const char*          _definition
)
{
    struct d_tests_arg_run      run    = { 0 };
    struct d_arg_command* const app    = d_tests_arg_begin(&run);
    struct d_arg_option* const  option = d_tests_arg_option_add(app,
                                                                _definition,
                                                                false);

    (void)d_arg_buffer_printf(_out,
                              "[%s] ",
                              _definition);

    // a definition refused, as CLI11 throws
    if (!option)
    {
        d_tests_arg_option_refused(_out,
                                   &run);
    }
    else
    {
        d_tests_arg_option_named(_out,
                                 option,
                                 NAME_FORMS,
                                 sizeof(NAME_FORMS) / sizeof(NAME_FORMS[0]));
        (void)d_arg_buffer_printf(_out,
                                  " single=%s positional=%d nonpositional=%d\n",
                                  d_arg_option_single_name(option),
                                  (int)d_arg_option_is_positional(option),
                                  (int)d_arg_option_is_nonpositional(option));
    }

    d_tests_arg_end(&run);

    return;
}

/**
 * @brief Appends what a flag's definition string makes of names, as
 *        FLAG_NAMES writes it, the flag given to a parser of its own.
 *
 * @param[in,out] _out         the transcript.
 * @param[in]     _definition  the definition.
 */
static void
d_tests_arg_option_define_flag(
    struct d_arg_buffer* _out,
    const char*          _definition
)
{
    struct d_tests_arg_run      run  = { 0 };
    struct d_arg_command* const app  = d_tests_arg_begin(&run);
    struct d_arg_option* const  flag = d_tests_arg_option_add(app,
                                                              _definition,
                                                              true);

    (void)d_arg_buffer_printf(_out,
                              "[%s] ",
                              _definition);

    // a definition refused, as CLI11 throws
    if (!flag)
    {
        d_tests_arg_option_refused(_out,
                                   &run);
    }
    else
    {
        d_tests_arg_option_named(_out,
                                 flag,
                                 FLAG_FORMS,
                                 sizeof(FLAG_FORMS) / sizeof(FLAG_FORMS[0]));
        (void)d_arg_buffer_printf(_out,
                                  " default=%s\n",
                                  d_arg_option_default_text(flag));
    }

    d_tests_arg_end(&run);

    return;
}

/**
 * @brief Appends what adding an option or a flag to a command came to, as
 *        TAKEN writes it: `ok` and every name, or the refusal.
 *
 * @param[in,out] _out         the transcript.
 * @param[in,out] _run         the run, whose parser records a refusal.
 * @param[in,out] _app         the command.
 * @param[in]     _definition  the names.
 * @param[in]     _flag        whether a flag is added rather than an option.
 */
static void
d_tests_arg_option_try(
    struct d_arg_buffer*    _out,
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app,
    const char*             _definition,
    bool                    _flag
)
{
    struct d_arg_option* const option = d_tests_arg_option_add(_app,
                                                               _definition,
                                                               _flag);

    (void)d_arg_buffer_printf(_out,
                              "[%s%s] ",
                              (_flag) ? "flag " : "",
                              _definition);

    // a definition refused, as CLI11 throws
    if (!option)
    {
        d_tests_arg_option_refused(_out,
                                   _run);

        return;
    }

    (void)d_arg_buffer_append_cstr(_out,
                                   "ok ");
    (void)d_arg_option_name(option,
                            D_ARG_NAME_ALL,
                            _out);
    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

/**
 * @brief Appends one line of MATCH: its label, then each name asked about
 *        with the check's answer.
 *
 * @param[in,out] _out     the transcript.
 * @param[in]     _line    the line.
 * @param[in]     _option  the option asked, `-a,--alpha_beta,pos`.
 * @param[in]     _flag    the flag asked, `-n,--no-flag{false}`.
 */
static void
d_tests_arg_option_checked(
    struct d_arg_buffer*                        _out,
    const struct d_tests_arg_option_match_line* _line,
    const struct d_arg_option*                  _option,
    const struct d_arg_option*                  _flag
)
{
    const struct d_arg_option* const asked = (_line->flag)
                                             ? _flag
                                             : _option;

    (void)d_arg_buffer_append_cstr(_out,
                                   _line->label);

    // each name, with the check's answer
    for (size_t index = 0u; _line->names[index] != NULL; ++index)
    {
        const char* const name = _line->names[index];
        const bool        hit  = _line->check(asked,
                                              d_pack_text_from_cstr(name));

        (void)d_arg_buffer_printf(_out,
                                  " %s=%d",
                                  name,
                                  (int)hit);
    }

    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

/**
 * @brief Appends MATCH's lines under one setting: the option
 *        `-a,--alpha_beta,pos`, whose environment variable is ENV_X, and
 *        the flag `-n,--no-flag{false}`, both made to ignore case, or
 *        underscores, or both, as the setting's bits say.
 *
 * @param[in,out] _out      the transcript.
 * @param[in]     _setting  bit 1 ignores case, bit 2 underscores.
 */
static void
d_tests_arg_option_match(
    struct d_arg_buffer* _out,
    unsigned int         _setting
)
{
    struct d_tests_arg_run      run    = { 0 };
    struct d_arg_command* const app    = d_tests_arg_begin(&run);
    struct d_arg_option* const  option =
        d_arg_command_add_option(app,
                                 "-a,--alpha_beta,pos",
                                 NULL);
    struct d_arg_option* const  flag   =
        d_arg_command_add_flag(app,
                               "-n,--no-flag{false}",
                               NULL);
    const bool                  cased  = ((_setting & 1u) != 0u);
    const bool                  under  = ((_setting & 2u) != 0u);

    (void)d_arg_option_set_env_name(option,
                                    "ENV_X");
    (void)d_arg_option_set_ignore_case(option,
                                       cased);
    (void)d_arg_option_set_ignore_case(flag,
                                       cased);
    (void)d_arg_option_set_ignore_underscore(option,
                                             under);
    (void)d_arg_option_set_ignore_underscore(flag,
                                             under);
    (void)d_arg_buffer_printf(_out,
                              "case=%d under=%d\n",
                              (int)cased,
                              (int)under);

    // each line, in the reference's order
    for (size_t index = 0u; index < MATCH_LINE_COUNT; ++index)
    {
        d_tests_arg_option_checked(_out,
                                   &MATCH_LINES[index],
                                   option,
                                   flag);
    }

    d_tests_arg_end(&run);

    return;
}

/**
 * @brief Makes an option ignore underscores or case, as a case of CONFLICT
 *        asks.
 *
 * @param[in,out] _option      the option.
 * @param[in]     _underscore  whether underscores are ignored, rather than
 *                             case.
 * @return the setter's status.
 */
static int
d_tests_arg_option_ignore(
    struct d_arg_option* _option,
    bool                 _underscore
)
{
    // underscores
    if (_underscore)
    {
        return d_arg_option_set_ignore_underscore(_option,
                                                  true);
    }

    return d_arg_option_set_ignore_case(_option,
                                        true);
}

/**
 * @brief Appends one case of CONFLICT, in a parser of its own: the
 *        setting, what it came to, and whether it was kept.
 *
 * @param[in,out] _out   the transcript.
 * @param[in]     _case  the case.
 */
static void
d_tests_arg_option_ignored(
    struct d_arg_buffer*                    _out,
    const struct d_tests_arg_option_ignore* _case
)
{
    struct d_tests_arg_run      run     = { 0 };
    struct d_arg_command* const app     = d_tests_arg_begin(&run);
    const char* const           setting = (_case->underscore)
                                          ? "ignore_underscore"
                                          : "ignore_case";
    const uint32_t              which   = (_case->underscore)
                                          ? D_ARG_OPTION_IGNORE_UNDERSCORE
                                          : D_ARG_OPTION_IGNORE_CASE;

    (void)d_arg_command_add_option(app,
                                   _case->held,
                                   NULL);

    struct d_arg_option* const added  =
        d_arg_command_add_option(app,
                                 _case->added,
                                 NULL);
    const int                  status =
        d_tests_arg_option_ignore(added,
                                  _case->underscore);

    (void)d_arg_buffer_printf(_out,
                              "[%s %s %s] ",
                              setting,
                              _case->held,
                              _case->added);

    // the setting refused, as CLI11 throws
    if (status != (int)D_ARG_OK)
    {
        d_tests_arg_option_refused(_out,
                                   &run);
    }
    else
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       "ok\n");
    }

    (void)d_arg_buffer_printf(_out,
                              "kept %d\n",
                              (int)d_arg_option_has_flag(added,
                                                         which));
    d_tests_arg_end(&run);

    return;
}

/**
 * @brief Appends a line of the names options share: its label, then for
 *        each pair the name the first finds it shares with the second, as
 *        CLI11's matching_name.
 *
 * @param[in,out] _out      the transcript.
 * @param[in]     _label    the line's label.
 * @param[in]     _options  SHARING_DEFINITIONS' options, `a` to `f`.
 * @param[in]     _pairs    the pairs asked about.
 * @param[in]     _count    how many pairs.
 */
static void
d_tests_arg_option_shared(
    struct d_arg_buffer*                  _out,
    const char*                           _label,
    struct d_arg_option* const*           _options,
    const struct d_tests_arg_option_pair* _pairs,
    size_t                                _count
)
{
    (void)d_arg_buffer_printf(_out,
                              "%s:",
                              _label);

    // each pair, with the name the two share
    for (size_t index = 0u; index < _count; ++index)
    {
        const size_t      asked  = _pairs[index].asked;
        const size_t      other  = _pairs[index].other;
        const char* const shared =
            d_arg_option_matching_name(_options[asked],
                                       _options[other]);

        (void)d_arg_buffer_printf(_out,
                                  " %c/%c <%s>",
                                  'a' + (int)asked,
                                  'a' + (int)other,
                                  (shared != NULL) ? shared : "");
    }

    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

/**
 * @brief Makes one of SHARING_DEFINITIONS' options in a command of its own.
 *
 * @param[in,out] _app    the root command.
 * @param[in]     _index  the option's index.
 * @return the option; NULL when it could not be made.
 */
static struct d_arg_option*
d_tests_arg_option_alone(
    struct d_arg_command* _app,
    size_t                _index
)
{
    // the first, in the root
    if (SHARING_COMMANDS[_index] == NULL)
    {
        return d_arg_command_add_option(_app,
                                        SHARING_DEFINITIONS[_index],
                                        NULL);
    }

    struct d_arg_command* const command =
        d_arg_command_add_subcommand(_app,
                                     SHARING_COMMANDS[_index],
                                     NULL);

    return d_arg_command_add_option(command,
                                    SHARING_DEFINITIONS[_index],
                                    NULL);
}

/**
 * @brief Appends the names SHARING_DEFINITIONS' options share: asked in
 *        pairs, then once `f` ignores case, then once `a` is not
 *        configurable.
 *
 * @param[in,out] _out  the transcript.
 */
static void
d_tests_arg_option_sharing(
    struct d_arg_buffer* _out
)
{
    struct d_tests_arg_run      run                    = { 0 };
    struct d_arg_command* const app                    =
        d_tests_arg_begin(&run);
    struct d_arg_option*        options[SHARING_COUNT] = { NULL };

    // each option, in a command of its own
    for (size_t index = 0u; index < (size_t)SHARING_COUNT; ++index)
    {
        options[index] = d_tests_arg_option_alone(app,
                                                  index);
    }

    d_tests_arg_option_shared(_out,
                              "matching",
                              options,
                              SHARING_PAIRS,
                              SHARING_PAIR_COUNT);
    (void)d_arg_option_set_ignore_case(options[5],
                                       true);
    d_tests_arg_option_shared(_out,
                              "ignore_case",
                              options,
                              IGNORING_PAIRS,
                              IGNORING_COUNT);
    d_arg_option_set_configurable(options[0],
                                  false);
    d_tests_arg_option_shared(_out,
                              "unconfigurable",
                              options,
                              UNCONFIGURABLE_PAIRS,
                              UNCONFIGURABLE_COUNT);
    d_tests_arg_end(&run);

    return;
}

/**
 * @brief Calls the setter a step of COUNTS names.
 *
 * @param[in,out] _option  the option.
 * @param[in]     _step    the step.
 */
static void
d_tests_arg_option_count_apply(
    struct d_arg_option*                        _option,
    const struct d_tests_arg_option_count_step* _step
)
{
    // the setter the step names
    switch (_step->setter)
    {
        case COUNT_EXPECTED:
            d_arg_option_set_expected(_option,
                                      _step->first);
            break;

        case COUNT_EXPECTED_RANGE:
            d_arg_option_set_expected_range(_option,
                                            _step->first,
                                            _step->second);
            break;

        case COUNT_TYPE_SIZE:
            d_arg_option_set_type_size(_option,
                                       _step->first);
            break;

        case COUNT_TYPE_SIZE_RANGE:
            d_arg_option_set_type_size_range(_option,
                                             _step->first,
                                             _step->second);
            break;

        case COUNT_POLICY:
            d_arg_option_set_multi_option_policy(_option,
                                                 _step->first);
            break;

        case COUNT_REQUIRED:
            d_arg_option_set_required(_option,
                                      (_step->first != 0));
            break;

        case COUNT_EXTRA:
            d_arg_option_set_allow_extra_args(_option,
                                              (_step->first != 0));
            break;

        default:
            break;
    }

    return;
}

/**
 * @brief Appends an option's counts as COUNTS writes them.
 *
 * @param[in,out] _out     the transcript.
 * @param[in]     _label   what the line starts with.
 * @param[in]     _option  the option.
 */
static void
d_tests_arg_option_count_show(
    struct d_arg_buffer*       _out,
    const char*                _label,
    const struct d_arg_option* _option
)
{
    (void)d_arg_buffer_printf(
        _out,
        "%s exp=%d,%d type=%d,%d items=%d,%d extra=%d req=%d sep=%d\n",
        _label,
        _option->expected_min,
        _option->expected_max,
        _option->type_size_min,
        _option->type_size_max,
        d_arg_option_items_expected_min(_option),
        d_arg_option_items_expected_max(_option),
        (int)d_arg_option_has_flag(_option,
                                   D_ARG_OPTION_ALLOW_EXTRA_ARGS),
        (int)d_arg_option_has_flag(_option,
                                   D_ARG_OPTION_REQUIRED),
        (int)d_arg_option_has_flag(_option,
                                   D_ARG_OPTION_INJECT_SEPARATOR));

    return;
}

/**
 * @brief Whether a run of count steps writes the transcript expected, each
 *        step on a fresh option or on the last, in one parser.
 *
 * @param[in] _what      the transcript's name, for a mismatch.
 * @param[in] _steps     the steps.
 * @param[in] _count     how many.
 * @param[in] _expected  the transcript expected.
 * @return whether the steps wrote it.
 */
static bool
d_tests_arg_option_counted(
    const char*                                 _what,
    const struct d_tests_arg_option_count_step* _steps,
    size_t                                      _count,
    const char*                                 _expected
)
{
    struct d_tests_arg_run      run    = { 0 };
    struct d_arg_command* const app    = d_tests_arg_begin(&run);
    struct d_arg_buffer         out    = D_ARG_BUFFER_INIT;
    struct d_arg_option*        option = NULL;
    size_t                      made   = 0u;

    // each step, on a fresh option or on the last
    for (size_t index = 0u; index < _count; ++index)
    {
        const struct d_tests_arg_option_count_step* const step =
            &_steps[index];

        // a fresh option, named after the options before it
        if (step->fresh)
        {
            option = d_tests_arg_option_numbered(app,
                                                 "--c",
                                                 made);
            ++made;
        }

        // an option that could not be made ends the steps
        if (!option)
        {
            break;
        }

        d_tests_arg_option_count_apply(option,
                                       step);

        // the step's line, if it has one
        if (step->label != NULL)
        {
            d_tests_arg_option_count_show(&out,
                                          step->label,
                                          option);
        }
    }

    const bool same = d_tests_arg_same(_what,
                                       d_arg_buffer_text(&out),
                                       _expected);

    d_arg_buffer_release(&out);
    d_tests_arg_end(&run);

    return same;
}

/**
 * @brief Makes the option of one of CONFIGS, `--o<index>`, setting its type
 *        size, expected count, extra arguments, delimiter and policy, in the
 *        reference's order.
 *
 * @param[in,out] _app    the command.
 * @param[in]     _index  the configuration's index.
 * @return the option; NULL when it could not be made.
 */
static struct d_arg_option*
d_tests_arg_option_configure(
    struct d_arg_command* _app,
    size_t                _index
)
{
    const struct d_tests_arg_option_config* const config =
        &CONFIGS[_index];
    struct d_arg_option* const                    option =
        d_tests_arg_option_numbered(_app,
                                    "--o",
                                    _index);

    d_arg_option_set_type_size_range(option,
                                     config->type_min,
                                     config->type_max);
    d_arg_option_set_expected_range(option,
                                    config->expected_min,
                                    config->expected_max);
    d_arg_option_set_allow_extra_args(option,
                                      config->extra);
    d_arg_option_set_delimiter(option,
                               config->delimiter);
    d_arg_option_set_multi_option_policy(option,
                                         config->policy);

    return option;
}

/**
 * @brief Appends the A lines of a configuration: each of INPUTS added
 *        alone, with the values it made and how many.
 *
 * @param[in,out] _out     the transcript.
 * @param[in,out] _option  the option, cleared before each input.
 */
static void
d_tests_arg_option_added(
    struct d_arg_buffer* _out,
    struct d_arg_option* _option
)
{
    // each input, alone
    for (size_t index = 0u; index < INPUT_COUNT; ++index)
    {
        int added = -1;

        d_arg_option_clear(_option);
        (void)d_arg_option_add_result(_option,
                                      d_pack_text_from_cstr(INPUTS[index]),
                                      &added);

        size_t                          count   = 0u;
        const struct d_pack_text* const results =
            d_arg_option_results(_option,
                                 &count);

        (void)d_arg_buffer_printf(_out,
                                  "A %d ",
                                  (int)index);
        d_tests_arg_option_list(_out,
                                results,
                                count);
        (void)d_arg_buffer_printf(_out,
                                  " added %d\n",
                                  added);
    }

    return;
}

/**
 * @brief Appends the R line of one of LISTS: what the option's policy keeps
 *        of the values, or the refusal, as CLI11's reduced_results, which
 *        reduces only values there are.
 *
 * @param[in,out] _out     the transcript.
 * @param[in,out] _run     the run, whose parser reports a refusal.
 * @param[in,out] _option  the option, holding the list's values.
 * @param[in]     _list    the list's index in LISTS.
 */
static void
d_tests_arg_option_reduced(
    struct d_arg_buffer*    _out,
    struct d_tests_arg_run* _run,
    struct d_arg_option*    _option,
    size_t                  _list
)
{
    const int status = (_option->results.count > 0u)
                       ? d_arg_option_reduce(_option)
                       : (int)D_ARG_OK;

    (void)d_arg_buffer_printf(_out,
                              "R %d ",
                              (int)_list);

    // the values refused
    if (status != (int)D_ARG_OK)
    {
        d_tests_arg_option_err(_out,
                               status,
                               d_arg_parser_error(&_run->parser));

        return;
    }

    size_t                          count  = 0u;
    const struct d_pack_text* const values =
        d_arg_option_values(_option,
                            &count);

    d_tests_arg_option_list(_out,
                            values,
                            count);
    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

/**
 * @brief Appends an option's values read as one text, as CLI11's
 *        as<std::string>, or the refusal.
 *
 * @param[in,out] _out     the transcript.
 * @param[in,out] _run     the run, whose parser reports a refusal.
 * @param[in,out] _option  the option, whose values are reduced if they are
 *                         not yet.
 */
static void
d_tests_arg_option_as_text(
    struct d_arg_buffer*    _out,
    struct d_tests_arg_run* _run,
    struct d_arg_option*    _option
)
{
    const struct d_arg_type* const string =
        d_arg_type_builtin(D_ARG_BUILTIN_STRING);
    const char*                    text   = NULL;
    const int                      status = d_arg_option_as(_option,
                                                            string,
                                                            &text);

    // the values refused
    if (status != (int)D_ARG_OK)
    {
        d_tests_arg_option_err(_out,
                               status,
                               d_arg_parser_error(&_run->parser));

        return;
    }

    (void)d_arg_buffer_printf(_out,
                              "<%s>\n",
                              (text != NULL) ? text : "");

    return;
}

/**
 * @brief Whether one of CONFIGS writes its transcript: the configuration's
 *        counts, its A lines, then the R and S lines of each of LISTS.
 *
 * @param[in] _index  the configuration's index.
 * @return whether the transcript was CLI11's.
 *
 * Before its S line a list is given again, so that the option holds the
 * values as given, as CLI11's does: its reduced_results leaves the option
 * as it was, where d_arg_option_reduce keeps what it made.
 */
static bool
d_tests_arg_option_configured(
    size_t _index
)
{
    struct d_tests_arg_run      run    = { 0 };
    struct d_arg_command* const app    = d_tests_arg_begin(&run);
    struct d_arg_option* const  option =
        d_tests_arg_option_configure(app,
                                     _index);
    struct d_arg_buffer         out    = D_ARG_BUFFER_INIT;

    // an option that could not be made
    if (!option)
    {
        d_tests_arg_end(&run);

        return false;
    }

    (void)d_arg_buffer_printf(&out,
                              "cfg %lu: %d %d %d %d\n",
                              (unsigned long)_index,
                              option->expected_min,
                              option->expected_max,
                              option->type_size_min,
                              option->type_size_max);
    d_tests_arg_option_added(&out,
                             option);

    // each list, reduced, then read as one text
    for (size_t list = 0u; list < LIST_COUNT; ++list)
    {
        d_tests_arg_option_give(option,
                                LISTS[list]);
        d_tests_arg_option_reduced(&out,
                                   &run,
                                   option,
                                   list);
        d_tests_arg_option_give(option,
                                LISTS[list]);
        (void)d_arg_buffer_printf(&out,
                                  "S %d ",
                                  (int)list);
        d_tests_arg_option_as_text(&out,
                                   &run,
                                   option);
    }

    const bool same = d_tests_arg_same(CFG_TRANSCRIPTS[_index].name,
                                       d_arg_buffer_text(&out),
                                       CFG_TRANSCRIPTS[_index].text);

    d_arg_buffer_release(&out);
    d_tests_arg_end(&run);

    return same;
}

/**
 * @brief Converts values into a list of text, as CLI11 converts them into a
 *        std::vector<std::string>.
 *
 * @param[in]  _type     the descriptor; unused.
 * @param[in]  _values   the values.
 * @param[in]  _count    how many.
 * @param[out] _target   the d_arg_text_list, whose `items` has room for
 *                       `capacity` values; it borrows them.
 * @param[in]  _context  the conversion's context; unused.
 * @return D_ARG_OK; D_ARG_ERROR_CONVERSION when the list has no room.
 *
 * `{}` alone is the empty list, and `{}` followed by an empty value or `%%`
 * -- the marker of an empty container -- a list of `{}` alone, as CLI11's
 * conversion stops after the first value; any other values are the list.
 */
static int
d_tests_arg_option_to_list(
    const struct d_arg_type*            _type,
    const struct d_pack_text*           _values,
    size_t                              _count,
    void*                               _target,
    const struct d_arg_convert_context* _context
)
{
    struct d_arg_text_list* const list   = _target;
    const bool                    marked =
        ( (_count > 0u) &&
          (d_tests_arg_option_is(_values[0],
                                 "{}")) );
    size_t                        taken  = _count;

    (void)_type;
    (void)_context;

    // `{}` alone: the empty list
    if ( (marked) &&
         (_count == 1u) )
    {
        taken = 0u;
    }

    // `{}` and the marker: `{}` alone
    if ( (marked)                       &&
         (_count == 2u)                 &&
         ( (_values[1].length == 0u) ||
           (d_tests_arg_option_is(_values[1],
                                  "%%")) ) )
    {
        taken = 1u;
    }

    // a list without room for the values
    if (taken > list->capacity)
    {
        return D_ARG_ERROR_CONVERSION;
    }

    // each value taken, borrowed
    for (size_t index = 0u; index < taken; ++index)
    {
        list->items[index] = _values[index];
    }

    list->count = taken;

    return D_ARG_OK;
}

// LIST_TYPE
//   constant: the descriptor of a list of text, CLI11's
// std::vector<std::string>: a container of text, of any number of values.
static const struct d_arg_type LIST_TYPE =
{
    "LIST",                                               // name
    d_tests_arg_option_to_list,                           // convert
    NULL,                                                 // format
    NULL,                                                 // data
    sizeof(struct d_arg_text_list),                       // size
    1,                                                    // type_size_min
    1,                                                    // type_size_max
    D_ARG_EXPECTED_UNLIMITED,                             // expected
    (D_ARG_TYPE_FLAG_CONTAINER | D_ARG_TYPE_FLAG_STRING)  // flags
};

/**
 * @brief Appends an option's values read as a list of text, as CLI11's
 *        as<std::vector<std::string>>, or the refusal.
 *
 * @param[in,out] _out     the transcript.
 * @param[in,out] _run     the run, whose parser reports a refusal.
 * @param[in,out] _option  the option, whose values are reduced if they are
 *                         not yet.
 */
static void
d_tests_arg_option_as_list(
    struct d_arg_buffer*    _out,
    struct d_tests_arg_run* _run,
    struct d_arg_option*    _option
)
{
    struct d_pack_text     items[AS_LIST_MAX] = { D_PACK_TEXT_NONE };
    struct d_arg_text_list list               =
    {
        items,
        0u,
        (size_t)AS_LIST_MAX
    };
    const int              status             = d_arg_option_as(_option,
                                                                &LIST_TYPE,
                                                                &list);

    // the values refused
    if (status != (int)D_ARG_OK)
    {
        d_tests_arg_option_err(_out,
                               status,
                               d_arg_parser_error(&_run->parser));

        return;
    }

    d_tests_arg_option_list(_out,
                            items,
                            list.count);
    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

/**
 * @brief Appends the V and S lines of one of AS_CASES, in a parser of its
 *        own: the values, or the default, read as a list of text, then as
 *        one text.
 *
 * @param[in,out] _out    the transcript.
 * @param[in]     _index  the case's index.
 */
static void
d_tests_arg_option_as_one(
    struct d_arg_buffer* _out,
    size_t               _index
)
{
    const struct d_tests_arg_option_as_case* const as_case =
        &AS_CASES[_index];
    struct d_tests_arg_run                         run     = { 0 };
    struct d_arg_command* const                    app     =
        d_tests_arg_begin(&run);
    struct d_arg_option* const                     option  =
        d_tests_arg_option_numbered(app,
                                    "--a",
                                    _index);

    d_arg_option_set_expected_range(option,
                                    as_case->expected_min,
                                    as_case->expected_max);
    d_arg_option_set_delimiter(option,
                               as_case->delimiter);
    d_arg_option_set_multi_option_policy(option,
                                         as_case->policy);
    (void)d_arg_option_set_default_text(option,
                                        as_case->default_text);
    d_tests_arg_option_give(option,
                            as_case->values);
    (void)d_arg_buffer_printf(_out,
                              "V%lu ",
                              (unsigned long)_index);
    d_tests_arg_option_as_list(_out,
                               &run,
                               option);

    // as one text, but where CLI11 reads past the end of an empty list
    if (_index != (size_t)AS_UNDEFINED)
    {
        (void)d_arg_buffer_printf(_out,
                                  "S%lu ",
                                  (unsigned long)_index);
        d_tests_arg_option_as_text(_out,
                                   &run,
                                   option);
    }

    d_tests_arg_end(&run);

    return;
}

/**
 * @brief Configures FLAG_DEFINITION's flag as one of FLAG_CONFIGS says.
 *
 * @param[in,out] _flag    the flag.
 * @param[in]     _config  the configuration.
 */
static void
d_tests_arg_option_flag_configure(
    struct d_arg_option*                         _flag,
    const struct d_tests_arg_option_flag_config* _config
)
{
    d_arg_option_set_disable_flag_override(_flag,
                                           _config->no_override);

    // a value expected, as CLI11's expected(1) makes a flag take one
    if (_config->one_value)
    {
        d_arg_option_set_expected(_flag,
                                  1);
    }

    d_arg_option_set_force_callback(_flag,
                                    _config->force);
    (void)d_arg_option_set_default_text(_flag,
                                        _config->default_text);
    (void)d_arg_option_set_ignore_case(_flag,
                                       _config->ignore);
    (void)d_arg_option_set_ignore_underscore(_flag,
                                             _config->ignore);

    return;
}

/**
 * @brief Appends the value a flag name stands for with an input, as
 *        CLI11's get_flag_value, or the refusal.
 *
 * @param[in,out] _out    the transcript.
 * @param[in,out] _run    the run, whose parser reports a refusal.
 * @param[in]     _flag   the flag.
 * @param[in]     _name   the name asked for, without dashes.
 * @param[in]     _input  the input after `=`, or empty.
 */
static void
d_tests_arg_option_flag_line(
    struct d_arg_buffer*       _out,
    struct d_tests_arg_run*    _run,
    const struct d_arg_option* _flag,
    const char*                _name,
    const char*                _input
)
{
    struct d_arg_buffer value  = D_ARG_BUFFER_INIT;
    const int           status =
        d_arg_option_flag_value(_flag,
                                d_pack_text_from_cstr(_name),
                                d_pack_text_from_cstr(_input),
                                &value);

    (void)d_arg_buffer_printf(_out,
                              "[%s] [%s] ",
                              _name,
                              _input);

    // the input refused
    if (status != (int)D_ARG_OK)
    {
        d_tests_arg_option_err(_out,
                               status,
                               d_arg_parser_error(&_run->parser));
    }
    else
    {
        (void)d_arg_buffer_append_char(_out,
                                       '<');
        (void)d_arg_buffer_append_text(_out,
                                       d_arg_buffer_text(&value));
        (void)d_arg_buffer_append_cstr(_out,
                                       ">\n");
    }

    d_arg_buffer_release(&value);

    return;
}

/**
 * @brief Whether one of FLAG_TRANSCRIPTS is written: FLAG_DEFINITION's
 *        flag, configured as one of FLAG_CONFIGS says, asked for half of
 *        FLAG_ASKED with each of FLAG_INPUTS.
 *
 * @param[in] _index  the transcript's index: twice the configuration's,
 *                    and one more for the second half.
 * @return whether the transcript was CLI11's.
 */
static bool
d_tests_arg_option_flag_half(
    size_t _index
)
{
    const size_t                first = (_index % 2u) * (size_t)FLAG_HALF;
    struct d_tests_arg_run      run   = { 0 };
    struct d_arg_command* const app   = d_tests_arg_begin(&run);
    struct d_arg_option* const  flag  =
        d_arg_command_add_flag(app,
                               FLAG_DEFINITION,
                               NULL);
    struct d_arg_buffer         out   = D_ARG_BUFFER_INIT;

    d_tests_arg_option_flag_configure(flag,
                                      &FLAG_CONFIGS[_index / 2u]);

    // each name of the half
    for (size_t name = first; name < first + (size_t)FLAG_HALF; ++name)
    {
        // with each input
        for (size_t input = 0u; input < FLAG_INPUT_COUNT; ++input)
        {
            d_tests_arg_option_flag_line(&out,
                                         &run,
                                         flag,
                                         FLAG_ASKED[name],
                                         FLAG_INPUTS[input]);
        }
    }

    const bool same = d_tests_arg_same(FLAG_TRANSCRIPTS[_index].name,
                                       d_arg_buffer_text(&out),
                                       FLAG_TRANSCRIPTS[_index].text);

    d_arg_buffer_release(&out);
    d_tests_arg_end(&run);

    return same;
}

/**
 * @brief The callback of CALLBACKS: writes what it was given, then accepts
 *        or refuses the values, as its case says.
 *
 * @param[in] _option   the option; unused.
 * @param[in] _results  the values.
 * @param[in] _count    how many.
 * @param[in] _context  the d_tests_arg_option_called.
 * @return D_ARG_OK for a callback that accepts; D_ARG_ERROR_CONVERSION,
 *         with no message, for one that refuses, as CLI11's callback
 *         returning `false`.
 */
static int
d_tests_arg_option_callback(
    struct d_arg_option*      _option,
    const struct d_pack_text* _results,
    size_t                    _count,
    void*                     _context
)
{
    const struct d_tests_arg_option_called* const called = _context;

    (void)_option;
    (void)d_arg_buffer_printf(called->out,
                              "%s callback ",
                              called->row->name);
    d_tests_arg_option_list(called->out,
                            _results,
                            _count);
    (void)d_arg_buffer_append_char(called->out,
                                   '\n');

    return (called->row->callback == CALLBACK_ACCEPTS)
           ? (int)D_ARG_OK
           : (int)D_ARG_ERROR_CONVERSION;
}

/**
 * @brief Adds the option of one of CALLBACK_CASES, with the callback of
 *        CALLBACKS when the case has one.
 *
 * @param[in,out] _app     the command.
 * @param[in]     _names   the option's names.
 * @param[in]     _called  the callback's context, borrowed.
 * @return the option; NULL when it could not be made.
 */
static struct d_arg_option*
d_tests_arg_option_add_called(
    struct d_arg_command*             _app,
    const char*                       _names,
    struct d_tests_arg_option_called* _called
)
{
    // an option without a callback
    if (_called->row->callback == CALLBACK_NONE)
    {
        return d_arg_command_add_option(_app,
                                        _names,
                                        NULL);
    }

    return d_arg_command_add_option_callback(_app,
                                             _names,
                                             d_tests_arg_option_callback,
                                             _called,
                                             NULL,
                                             NULL);
}

/**
 * @brief Makes the option of one of CALLBACK_CASES, `--<name>`, as the
 *        reference does: its expected count, policy, forced callback,
 *        default text and group, then its values.
 *
 * @param[in,out] _app     the command.
 * @param[in]     _called  the case, and the transcript its callback writes
 *                         to; borrowed by the callback.
 * @return the option; NULL when it could not be made.
 */
static struct d_arg_option*
d_tests_arg_option_call_option(
    struct d_arg_command*             _app,
    struct d_tests_arg_option_called* _called
)
{
    const struct d_tests_arg_option_callback_case* const row  = _called->row;
    struct d_arg_buffer                                  name =
        D_ARG_BUFFER_INIT;

    (void)d_arg_buffer_printf(&name,
                              "--%s",
                              row->name);

    struct d_arg_option* const option =
        d_tests_arg_option_add_called(_app,
                                      d_arg_buffer_text(&name).data,
                                      _called);

    d_arg_buffer_release(&name);
    d_arg_option_set_expected(option,
                              row->expected);
    d_arg_option_set_multi_option_policy(option,
                                         row->policy);
    d_arg_option_set_force_callback(option,
                                    row->force);
    (void)d_arg_option_set_default_text(option,
                                        row->default_text);

    // a hidden option, in the empty group
    if (row->hidden)
    {
        (void)d_arg_option_set_group(option,
                                     "");
    }

    d_tests_arg_option_give(option,
                            row->values);

    return option;
}

/**
 * @brief Appends the CALLBACKS lines of one of CALLBACK_CASES, in a parser
 *        of its own: what the callback was given, how the run ended,
 *        whether the callback ran, and the results left.
 *
 * @param[in,out] _out    the transcript.
 * @param[in]     _index  the case's index.
 */
static void
d_tests_arg_option_call(
    struct d_arg_buffer* _out,
    size_t               _index
)
{
    struct d_tests_arg_option_called called =
    {
        &CALLBACK_CASES[_index],
        _out
    };
    struct d_tests_arg_run           run    = { 0 };
    struct d_arg_command* const      app    = d_tests_arg_begin(&run);
    struct d_arg_option* const       option =
        d_tests_arg_option_call_option(app,
                                       &called);
    const int                        status =
        d_arg_option_run_callback(option);
    size_t                           count  = 0u;

    (void)d_arg_buffer_printf(_out,
                              "%s ",
                              called.row->name);

    // how the run ended
    if (status == (int)D_ARG_OK)
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       "ok\n");
    }
    else
    {
        d_tests_arg_option_err(_out,
                               status,
                               d_arg_parser_error(&run.parser));
    }

    const struct d_pack_text* const results =
        d_arg_option_results(option,
                             &count);

    (void)d_arg_buffer_printf(_out,
                              "%s run=%d results ",
                              called.row->name,
                              (int)d_arg_option_callback_run(option));
    d_tests_arg_option_list(_out,
                            results,
                            count);
    (void)d_arg_buffer_append_char(_out,
                                   '\n');
    d_tests_arg_end(&run);

    return;
}

/**
 * @brief Appends what TEXT writes of an option: its description, option
 *        text and group, its type name with its validators' descriptions,
 *        its environment variable, and its name.
 *
 * @param[in,out] _out     the transcript.
 * @param[in]     _label   what the line starts with.
 * @param[in]     _option  the option.
 */
static void
d_tests_arg_option_text_show(
    struct d_arg_buffer*       _out,
    const char*                _label,
    const struct d_arg_option* _option
)
{
    (void)d_arg_buffer_printf(_out,
                              "%s desc=<%s> text=<%s> group=<%s> type=<",
                              _label,
                              d_arg_option_description(_option),
                              d_arg_option_option_text(_option),
                              d_arg_option_group(_option));
    (void)d_arg_option_type_name(_option,
                                 _out);
    (void)d_arg_buffer_printf(_out,
                              "> env=<%s> name=<",
                              d_arg_option_env_name(_option));
    (void)d_arg_option_name(_option,
                            0u,
                            _out);
    (void)d_arg_buffer_append_cstr(_out,
                                   ">\n");

    return;
}

/**
 * @brief A d_arg_option_text_fn writing `FN`, the type name TEXT gives an
 *        option through a function.
 *
 * @param[in] _option   the option; unused.
 * @param[in] _sink     receives the name.
 * @param[in] _context  unused.
 * @return D_ARG_OK, or the sink's refusal.
 */
static int
d_tests_arg_option_type_fn(
    const struct d_arg_option* _option,
    struct d_pack_sink         _sink,
    void*                      _context
)
{
    (void)_option;
    (void)_context;

    return d_arg_sink_cstr(_sink,
                           "FN");
}

/**
 * @brief A d_arg_option_text_fn writing `fn`, the default DEFAULTS
 *        captures through a function.
 *
 * @param[in] _option   the option; unused.
 * @param[in] _sink     receives the default.
 * @param[in] _context  unused.
 * @return D_ARG_OK, or the sink's refusal.
 */
static int
d_tests_arg_option_default_fn(
    const struct d_arg_option* _option,
    struct d_pack_sink         _sink,
    void*                      _context
)
{
    (void)_option;
    (void)_context;

    return d_arg_sink_cstr(_sink,
                           "fn");
}

/**
 * @brief A d_arg_each_fn that does nothing, as TEXT's each(), which has no
 *        description.
 *
 * @param[in] _option   the option; unused.
 * @param[in] _value    the value; unused.
 * @param[in] _context  unused.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_option_each_nothing(
    const struct d_arg_option* _option,
    struct d_pack_text         _value,
    void*                      _context
)
{
    (void)_option;
    (void)_value;
    (void)_context;

    return D_ARG_OK;
}

/**
 * @brief Appends TEXT's lines for an option's own text: a new description,
 *        option text and type name, one after another.
 *
 * @param[in,out] _out     the transcript.
 * @param[in,out] _option  the option.
 */
static void
d_tests_arg_option_text_named(
    struct d_arg_buffer* _out,
    struct d_arg_option* _option
)
{
    (void)d_arg_option_set_description(_option,
                                       "changed");
    d_tests_arg_option_text_show(_out,
                                 "description",
                                 _option);
    (void)d_arg_option_set_option_text(_option,
                                       "TEXT ...");
    d_tests_arg_option_text_show(_out,
                                 "option_text",
                                 _option);
    (void)d_arg_option_set_type_name(_option,
                                     "WORD");
    d_tests_arg_option_text_show(_out,
                                 "type_name",
                                 _option);

    return;
}

/**
 * @brief Appends TEXT's lines for an option's validators: a check, a
 *        transformation, which goes before it, a function run on each
 *        value, and a type name written by a function.
 *
 * @param[in,out] _out     the transcript.
 * @param[in,out] _option  the option.
 */
static void
d_tests_arg_option_text_validated(
    struct d_arg_buffer* _out,
    struct d_arg_option* _option
)
{
    (void)d_arg_option_check(_option,
                             d_arg_validator_positive_number(),
                             NULL);
    d_tests_arg_option_text_show(_out,
                                 "check",
                                 _option);
    (void)d_arg_option_transform(_option,
                                 d_arg_validator_range_long(1L,
                                                            9L),
                                 NULL,
                                 NULL);
    d_tests_arg_option_text_show(_out,
                                 "transform",
                                 _option);
    (void)d_arg_option_each(_option,
                            d_tests_arg_option_each_nothing,
                            NULL,
                            NULL);
    d_tests_arg_option_text_show(_out,
                                 "each",
                                 _option);
    d_arg_option_set_type_name_fn(_option,
                                  d_tests_arg_option_type_fn,
                                  NULL,
                                  NULL);
    d_tests_arg_option_text_show(_out,
                                 "type_name_fn",
                                 _option);

    return;
}

/**
 * @brief Appends TEXT's lines for an option's environment variable and
 *        group: a group named, one refused for its newline, and the empty
 *        group, which hides the option.
 *
 * @param[in,out] _out     the transcript.
 * @param[in,out] _run     the run, whose parser records the refusal.
 * @param[in,out] _option  the option.
 */
static void
d_tests_arg_option_text_grouped(
    struct d_arg_buffer*    _out,
    struct d_tests_arg_run* _run,
    struct d_arg_option*    _option
)
{
    (void)d_arg_option_set_env_name(_option,
                                    "ENV_T");
    d_tests_arg_option_text_show(_out,
                                 "envname",
                                 _option);
    (void)d_arg_option_set_group(_option,
                                 "Other");
    d_tests_arg_option_text_show(_out,
                                 "group",
                                 _option);
    (void)d_arg_buffer_append_cstr(_out,
                                   "[group a\\nb] ");

    // a group with a newline refused, as CLI11 throws
    if (d_arg_option_set_group(_option,
                               "a\nb") != (int)D_ARG_OK)
    {
        d_tests_arg_option_refused(_out,
                                   _run);
    }
    else
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       "ok\n");
    }

    d_tests_arg_option_text_show(_out,
                                 "kept",
                                 _option);
    (void)d_arg_option_set_group(_option,
                                 "");
    d_tests_arg_option_text_show(_out,
                                 "hidden",
                                 _option);

    return;
}

/**
 * @brief Appends TEXT's last line: an option's text emptied, its group
 *        back to OPTIONS.
 *
 * @param[in,out] _out     the transcript.
 * @param[in,out] _option  the option.
 */
static void
d_tests_arg_option_text_cleared(
    struct d_arg_buffer* _out,
    struct d_arg_option* _option
)
{
    (void)d_arg_option_set_description(_option,
                                       "");
    (void)d_arg_option_set_option_text(_option,
                                       "");
    (void)d_arg_option_set_type_name(_option,
                                     "");
    (void)d_arg_option_set_env_name(_option,
                                    "");
    (void)d_arg_option_set_group(_option,
                                 "OPTIONS");
    d_tests_arg_option_text_show(_out,
                                 "cleared",
                                 _option);

    return;
}

// d_tests_arg_option_defaulted
//   struct: what DEFAULTS works on: the run, the transcript, its options --
// one bound to nothing, one to `n`, one to `s` -- and the two variables,
// which the options borrow for as long as the run lasts.
struct d_tests_arg_option_defaulted
{
    struct d_tests_arg_run* run;
    struct d_arg_buffer*    out;
    struct d_arg_option*    unbound;
    struct d_arg_option*    number;
    struct d_arg_option*    text;
    int                     n;
    const char*             s;
};

/**
 * @brief Appends what DEFAULTS writes of an option: its default text and
 *        its values as given.
 *
 * @param[in,out] _out     the transcript.
 * @param[in]     _label   what the line starts with.
 * @param[in]     _option  the option.
 */
static void
d_tests_arg_option_default_show(
    struct d_arg_buffer*       _out,
    const char*                _label,
    const struct d_arg_option* _option
)
{
    size_t                          count   = 0u;
    const struct d_pack_text* const results =
        d_arg_option_results(_option,
                             &count);

    (void)d_arg_buffer_printf(_out,
                              "%s default=<%s> results=",
                              _label,
                              d_arg_option_default_text(_option));
    d_tests_arg_option_list(_out,
                            results,
                            count);
    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

/**
 * @brief Appends a default value set, or its refusal, as DEFAULTS writes
 *        it: `[default_val <name> <text>] ok`, or CLI11's exception.
 *
 * @param[in,out] _defaulted  what DEFAULTS works on.
 * @param[in]     _label      the option's label in the line.
 * @param[in,out] _option     the option.
 * @param[in]     _text       the default.
 */
static void
d_tests_arg_option_default_try(
    struct d_tests_arg_option_defaulted* _defaulted,
    const char*                          _label,
    struct d_arg_option*                 _option,
    const char*                          _text
)
{
    (void)d_arg_buffer_printf(_defaulted->out,
                              "[default_val %s %s] ",
                              _label,
                              _text);

    // a default refused, as CLI11 throws
    if (d_arg_option_set_default_value(_option,
                                       _text) != (int)D_ARG_OK)
    {
        d_tests_arg_option_refused(_defaulted->out,
                                   _defaulted->run);
    }
    else
    {
        (void)d_arg_buffer_append_cstr(_defaulted->out,
                                       "ok\n");
    }

    return;
}

/**
 * @brief Appends DEFAULTS' first lines: each option new, then each
 *        option's default captured -- the `int`'s again once it changed.
 *
 * @param[in,out] _defaulted  what DEFAULTS works on.
 */
static void
d_tests_arg_option_defaults_captured(
    struct d_tests_arg_option_defaulted* _defaulted
)
{
    struct d_arg_buffer* const out = _defaulted->out;

    d_tests_arg_option_default_show(out,
                                    "new a",
                                    _defaulted->unbound);
    d_tests_arg_option_default_show(out,
                                    "new b",
                                    _defaulted->number);
    d_tests_arg_option_default_show(out,
                                    "new c",
                                    _defaulted->text);
    (void)d_arg_option_capture_default(_defaulted->unbound);
    d_tests_arg_option_default_show(out,
                                    "capture a",
                                    _defaulted->unbound);
    (void)d_arg_option_capture_default(_defaulted->number);
    d_tests_arg_option_default_show(out,
                                    "capture b",
                                    _defaulted->number);
    _defaulted->n = 7;
    (void)d_arg_option_capture_default(_defaulted->number);
    d_tests_arg_option_default_show(out,
                                    "capture b again",
                                    _defaulted->number);
    (void)d_arg_option_capture_default(_defaulted->text);
    d_tests_arg_option_default_show(out,
                                    "capture c",
                                    _defaulted->text);

    return;
}

/**
 * @brief Appends DEFAULTS' lines for defaults set: a default text, a
 *        default value beside a value given, one that reaches the bound
 *        `int`, one it cannot take, and one set without the callback.
 *
 * @param[in,out] _defaulted  what DEFAULTS works on.
 */
static void
d_tests_arg_option_defaults_valued(
    struct d_tests_arg_option_defaulted* _defaulted
)
{
    struct d_arg_buffer* const out = _defaulted->out;

    (void)d_arg_option_set_default_text(_defaulted->unbound,
                                        "x");
    d_tests_arg_option_default_show(out,
                                    "default_str a",
                                    _defaulted->unbound);
    (void)d_arg_option_add_result(_defaulted->unbound,
                                  d_pack_text_from_cstr("r"),
                                  NULL);
    (void)d_arg_option_set_default_value(_defaulted->unbound,
                                         "5");
    d_tests_arg_option_default_show(out,
                                    "default_val a",
                                    _defaulted->unbound);
    (void)d_arg_option_set_default_value(_defaulted->number,
                                         "12");
    d_tests_arg_option_default_show(out,
                                    "default_val b",
                                    _defaulted->number);
    (void)d_arg_buffer_printf(out,
                              "n=%d\n",
                              _defaulted->n);
    d_tests_arg_option_default_try(_defaulted,
                                   "b",
                                   _defaulted->number,
                                   "x");
    d_tests_arg_option_default_show(out,
                                    "kept b",
                                    _defaulted->number);
    d_arg_option_set_run_callback_for_default(_defaulted->number,
                                              false);
    (void)d_arg_option_set_default_value(_defaulted->number,
                                         "20");
    d_tests_arg_option_default_show(out,
                                    "no callback b",
                                    _defaulted->number);
    (void)d_arg_buffer_printf(out,
                              "n=%d\n",
                              _defaulted->n);

    return;
}

/**
 * @brief Appends DEFAULTS' last lines: a default value reaching the bound
 *        text, one a validator refuses, and a default captured through a
 *        function.
 *
 * @param[in,out] _defaulted  what DEFAULTS works on.
 */
static void
d_tests_arg_option_defaults_checked(
    struct d_tests_arg_option_defaulted* _defaulted
)
{
    struct d_arg_buffer* const out = _defaulted->out;

    (void)d_arg_option_set_default_value(_defaulted->text,
                                         "given");
    d_tests_arg_option_default_show(out,
                                    "default_val c",
                                    _defaulted->text);
    (void)d_arg_buffer_printf(out,
                              "s=%s\n",
                              _defaulted->s);
    (void)d_arg_option_check(_defaulted->unbound,
                             d_arg_validator_positive_number(),
                             NULL);
    d_tests_arg_option_default_try(_defaulted,
                                   "a",
                                   _defaulted->unbound,
                                   "-1");
    d_tests_arg_option_default_show(out,
                                    "kept a",
                                    _defaulted->unbound);
    d_arg_option_set_default_fn(_defaulted->unbound,
                                d_tests_arg_option_default_fn,
                                NULL,
                                NULL);
    (void)d_arg_option_capture_default(_defaulted->unbound);
    d_tests_arg_option_default_show(out,
                                    "default_function a",
                                    _defaulted->unbound);

    return;
}

// RELATED_COUNT
//   constant: how many options RELATIONS relates.
enum
{
    RELATED_COUNT = 4
};

/**
 * @brief Appends the names of the options a relation list holds, as the
 *        reference writes them: `[<name>|<name>]`, sorted.
 *
 * @param[in,out] _out      the transcript.
 * @param[in]     _list     the list.
 * @param[in]     _options  RELATIONS' options, whose names -- `--a`, `--b`,
 *                          `-c`, `pos` -- are in sorted order already, so
 *                          that listing the ones the list holds in this
 *                          order sorts them.
 */
static void
d_tests_arg_option_related(
    struct d_arg_buffer*            _out,
    const struct d_arg_option_list* _list,
    struct d_arg_option* const*     _options
)
{
    bool first = true;

    (void)d_arg_buffer_append_char(_out,
                                   '[');

    // each option, when the list holds it
    for (size_t index = 0u; index < (size_t)RELATED_COUNT; ++index)
    {
        // whether the list holds it
        for (size_t held = 0u; held < _list->count; ++held)
        {
            // the option, after a bar but for the first
            if (_list->items[held] == _options[index])
            {
                (void)d_arg_buffer_append_cstr(_out,
                                               (first) ? "" : "|");
                (void)d_arg_option_name(_options[index],
                                        0u,
                                        _out);
                first = false;
            }
        }
    }

    (void)d_arg_buffer_append_char(_out,
                                   ']');

    return;
}

/**
 * @brief Appends a line of RELATIONS: its label, then each option's name,
 *        the options it needs, and the options it excludes.
 *
 * @param[in,out] _out      the transcript.
 * @param[in]     _label    what the line starts with.
 * @param[in]     _options  RELATIONS' options.
 */
static void
d_tests_arg_option_relations_show(
    struct d_arg_buffer*        _out,
    const char*                 _label,
    struct d_arg_option* const* _options
)
{
    (void)d_arg_buffer_append_cstr(_out,
                                   _label);

    // each option, with what it needs and excludes
    for (size_t index = 0u; index < (size_t)RELATED_COUNT; ++index)
    {
        (void)d_arg_buffer_append_char(_out,
                                       ' ');
        (void)d_arg_option_name(_options[index],
                                0u,
                                _out);
        (void)d_arg_buffer_append_cstr(_out,
                                       " needs=");
        d_tests_arg_option_related(_out,
                                   &_options[index]->needs,
                                   _options);
        (void)d_arg_buffer_append_cstr(_out,
                                       " excludes=");
        d_tests_arg_option_related(_out,
                                   &_options[index]->excludes,
                                   _options);
    }

    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

/**
 * @brief Appends what a relation that can fail came to, as RELATIONS
 *        writes it: `ok`, or CLI11's exception.
 *
 * @param[in,out] _out     the transcript.
 * @param[in,out] _run     the run, whose parser records a refusal.
 * @param[in]     _status  the relation's status.
 */
static void
d_tests_arg_option_relation_tried(
    struct d_arg_buffer*    _out,
    struct d_tests_arg_run* _run,
    int                     _status
)
{
    // a relation refused, as CLI11 throws
    if (_status != (int)D_ARG_OK)
    {
        d_tests_arg_option_refused(_out,
                                   _run);

        return;
    }

    (void)d_arg_buffer_append_cstr(_out,
                                   "ok\n");

    return;
}

/**
 * @brief Makes RELATIONS' options: `--a`, `--b`, the flag `-c`, and the
 *        positional `pos`.
 *
 * @param[in,out] _app      the command.
 * @param[out]    _options  receives the options, in that order.
 */
static void
d_tests_arg_option_relatable(
    struct d_arg_command* _app,
    struct d_arg_option** _options
)
{
    _options[0] = d_arg_command_add_option(_app,
                                           "--a",
                                           NULL);
    _options[1] = d_arg_command_add_option(_app,
                                           "--b",
                                           NULL);
    _options[2] = d_arg_command_add_flag(_app,
                                         "-c",
                                         NULL);
    _options[3] = d_arg_command_add_option(_app,
                                           "pos",
                                           NULL);

    return;
}

// MISSING_NAMES
//   constant: the names RELATIONS asks `--a` to need, none of them an
// option's: an unknown long name, a long name without its dashes, the
// long name with one dash, and the empty name.
static const char* const MISSING_NAMES[] =
{
    "--zz", "b", "-b", "", NULL
};

/**
 * @brief Appends RELATIONS' refusals: options needed by names no option
 *        has, an option excluding itself, and an exclusion by a name no
 *        option has.
 *
 * @param[in,out] _out      the transcript.
 * @param[in,out] _run      the run, whose parser records the refusals.
 * @param[in,out] _options  RELATIONS' options.
 */
static void
d_tests_arg_option_relations_refused(
    struct d_arg_buffer*        _out,
    struct d_tests_arg_run*     _run,
    struct d_arg_option* const* _options
)
{
    // each missing name
    for (size_t index = 0u; MISSING_NAMES[index] != NULL; ++index)
    {
        (void)d_arg_buffer_printf(_out,
                                  "[needs %s] ",
                                  MISSING_NAMES[index]);
        d_tests_arg_option_relation_tried(
            _out,
            _run,
            d_arg_option_needs_name(_options[0],
                                    MISSING_NAMES[index]));
    }

    (void)d_arg_buffer_append_cstr(_out,
                                   "[excludes self] ");
    d_tests_arg_option_relation_tried(_out,
                                      _run,
                                      d_arg_option_excludes(_options[0],
                                                            _options[0]));
    (void)d_arg_buffer_append_cstr(_out,
                                   "[excludes --zz] ");
    d_tests_arg_option_relation_tried(
        _out,
        _run,
        d_arg_option_excludes_name(_options[0],
                                   "--zz"));

    return;
}

/**
 * @brief Appends RELATIONS' removals, in the reference's order: `--a`'s
 *        need of `--b` twice, `-c`'s exclusion of `--b` twice, and `--a`'s
 *        exclusion of `-c`, which it never had.
 *
 * @param[in,out] _out      the transcript.
 * @param[in,out] _options  RELATIONS' options.
 */
static void
d_tests_arg_option_relations_removed(
    struct d_arg_buffer*        _out,
    struct d_arg_option* const* _options
)
{
    const bool first  = d_arg_option_remove_needs(_options[0],
                                                  _options[1]);
    const bool second = d_arg_option_remove_needs(_options[0],
                                                  _options[1]);
    const bool third  = d_arg_option_remove_excludes(_options[2],
                                                     _options[1]);
    const bool fourth = d_arg_option_remove_excludes(_options[2],
                                                     _options[1]);
    const bool fifth  = d_arg_option_remove_excludes(_options[0],
                                                     _options[2]);

    (void)d_arg_buffer_printf(_out,
                              "remove a/b %d a/b %d c/b %d c/b %d a/c %d\n",
                              (int)first,
                              (int)second,
                              (int)third,
                              (int)fourth,
                              (int)fifth);

    return;
}

/**
 * @brief VALIDATORS' NOX: refuses a value holding an `x`.
 *
 * @param[in]     _validator   the validator; unused.
 * @param[in,out] _validation  the value, and where the verdict goes.
 * @return D_ARG_OK, or the rejection's status.
 */
static int
d_tests_arg_option_nox(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    (void)_validator;

    // a value holding an `x`
    if ( (_validation->value.length > 0u) &&
         (memchr(_validation->value.data,
                 'x',
                 _validation->value.length) != NULL) )
    {
        return d_arg_validation_reject(_validation,
                                       "has an x");
    }

    return D_ARG_OK;
}

/**
 * @brief VALIDATORS' UPPER: raises a value's lowercase letters.
 *
 * @param[in]     _validator   the validator; unused.
 * @param[in,out] _validation  the value, replaced.
 * @return D_ARG_OK, or the status of the copy that failed.
 */
static int
d_tests_arg_option_upper(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    struct d_arg_buffer upper = D_ARG_BUFFER_INIT;

    (void)_validator;

    // each character, a lowercase letter raised
    for (size_t index = 0u; index < _validation->value.length; ++index)
    {
        const char character = _validation->value.data[index];

        (void)d_arg_buffer_append_char(&upper,
                                       ( (character >= 'a') &&
                                         (character <= 'z') )
                                       ? (char)(character - 'a' + 'A')
                                       : character);
    }

    const int status = (upper.failed != 0)
                       ? (int)D_ARG_ERROR_OUT_OF_MEMORY
                       : d_arg_validation_replace(_validation,
                                                  d_arg_buffer_text(&upper));

    d_arg_buffer_release(&upper);

    return status;
}

/**
 * @brief VALIDATORS' BAD: refuses the value `BAD`.
 *
 * @param[in]     _validator   the validator; unused.
 * @param[in,out] _validation  the value, and where the verdict goes.
 * @return D_ARG_OK, or the rejection's status.
 */
static int
d_tests_arg_option_bad(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    (void)_validator;

    return d_tests_arg_option_is(_validation->value,
                                 "BAD")
           ? d_arg_validation_reject(_validation,
                                     "is bad")
           : (int)D_ARG_OK;
}

/**
 * @brief VALIDATORS' ZERO: refuses the value `x`, applied to one index.
 *
 * @param[in]     _validator   the validator; unused.
 * @param[in,out] _validation  the value, and where the verdict goes.
 * @return D_ARG_OK, or the rejection's status.
 */
static int
d_tests_arg_option_zero(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    (void)_validator;

    return d_tests_arg_option_is(_validation->value,
                                 "x")
           ? d_arg_validation_reject(_validation,
                                     "x at 0")
           : (int)D_ARG_OK;
}

/**
 * @brief VALIDATORS' EMPTY: refuses the empty value.
 *
 * @param[in]     _validator   the validator; unused.
 * @param[in,out] _validation  the value, and where the verdict goes.
 * @return D_ARG_OK, or the rejection's status.
 */
static int
d_tests_arg_option_empty(
    const struct d_arg_validator* _validator,
    struct d_arg_validation*      _validation
)
{
    (void)_validator;

    return (_validation->value.length == 0u)
           ? d_arg_validation_reject(_validation,
                                     "empty")
           : (int)D_ARG_OK;
}

/**
 * @brief A d_arg_each_fn that logs each value it is given as `<value>`.
 *
 * @param[in] _option   the option; unused.
 * @param[in] _value    the value.
 * @param[in] _context  the d_arg_buffer the log is kept in.
 * @return D_ARG_OK, or the buffer's status.
 */
static int
d_tests_arg_option_log(
    const struct d_arg_option* _option,
    struct d_pack_text         _value,
    void*                      _context
)
{
    struct d_arg_buffer* const log = _context;

    (void)_option;
    (void)d_arg_buffer_append_char(log,
                                   '<');
    (void)d_arg_buffer_append_text(log,
                                   _value);

    return d_arg_buffer_append_char(log,
                                    '>');
}

/**
 * @brief Gives `--v` VALIDATORS' validators, in the reference's order: NOX
 *        checked, UPPER as a transformation, the log run on each value,
 *        and BAD checked.
 *
 * @param[in,out] _option  the option.
 * @param[in,out] _log     the buffer the log is kept in, borrowed.
 */
static void
d_tests_arg_option_validators_added(
    struct d_arg_option* _option,
    struct d_arg_buffer* _log
)
{
    (void)d_arg_option_check(_option,
                             d_arg_validator_custom(d_tests_arg_option_nox,
                                                    NULL,
                                                    "NOX"),
                             "nox");
    (void)d_arg_option_transform(
              _option,
              d_arg_validator_custom(d_tests_arg_option_upper,
                                     NULL,
                                     NULL),
              "upper",
              "UPPER");
    (void)d_arg_option_each(_option,
                            d_tests_arg_option_log,
                            _log,
                            NULL);
    (void)d_arg_option_check(_option,
                             d_arg_validator_custom(d_tests_arg_option_bad,
                                                    NULL,
                                                    NULL),
                             "bad");

    return;
}

/**
 * @brief Appends a validator found, as VALIDATORS writes it: `<name>
 *        <description>`, or `none` for none.
 *
 * @param[in,out] _out        the transcript.
 * @param[in]     _validator  the validator; NULL for none.
 */
static void
d_tests_arg_option_validator_found(
    struct d_arg_buffer*          _out,
    const struct d_arg_validator* _validator
)
{
    // none found, which CLI11 throws as OptionNotFound
    if (!_validator)
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       "none\n");

        return;
    }

    (void)d_arg_buffer_printf(_out,
                              "<%s> <",
                              (_validator->name != NULL)
                              ? _validator->name
                              : "");
    (void)d_arg_validator_describe(_validator,
                                   _out);
    (void)d_arg_buffer_append_cstr(_out,
                                   ">\n");

    return;
}

// VALIDATOR_NAMES
//   constant: the names VALIDATORS looks validators up by: each one's, the
// empty name, which finds the unnamed one, and a name none has.
static const char* const VALIDATOR_NAMES[] =
{
    "nox", "upper", "bad", "", "zz", NULL
};

// VALIDATOR_INDICES
//   constant: how many indices VALIDATORS looks validators up by, from 0:
// one past the four the option has.
enum
{
    VALIDATOR_INDICES = 5
};

/**
 * @brief Appends VALIDATORS' lookups: the type name the validators make,
 *        then each of VALIDATOR_NAMES and each index.
 *
 * @param[in,out] _out     the transcript.
 * @param[in,out] _option  `--v`, with its validators.
 */
static void
d_tests_arg_option_validators_found(
    struct d_arg_buffer* _out,
    struct d_arg_option* _option
)
{
    (void)d_arg_buffer_append_cstr(_out,
                                   "type=<");
    (void)d_arg_option_type_name(_option,
                                 _out);
    (void)d_arg_buffer_append_cstr(_out,
                                   ">\n");

    // each name
    for (size_t index = 0u; VALIDATOR_NAMES[index] != NULL; ++index)
    {
        (void)d_arg_buffer_printf(_out,
                                  "[get %s] ",
                                  VALIDATOR_NAMES[index]);
        d_tests_arg_option_validator_found(
            _out,
            d_arg_option_validator(_option,
                                   VALIDATOR_NAMES[index]));
    }

    // each index
    for (size_t index = 0u; index < (size_t)VALIDATOR_INDICES; ++index)
    {
        (void)d_arg_buffer_printf(_out,
                                  "[get %d] ",
                                  (int)index);
        d_tests_arg_option_validator_found(
            _out,
            d_arg_option_validator_at(_option,
                                      index));
    }

    return;
}

/**
 * @brief Appends a line of VALIDATORS: an option's values validated and
 *        reduced as a parse leaves them -- CLI11's run_callback of an
 *        option with no callback -- then its values as validation left
 *        them; or the refusal.
 *
 * @param[in,out] _out     the transcript.
 * @param[in,out] _run     the run, whose parser reports a refusal.
 * @param[in]     _label   what the line starts with.
 * @param[in,out] _option  the option, holding the values.
 */
static void
d_tests_arg_option_validated(
    struct d_arg_buffer*    _out,
    struct d_tests_arg_run* _run,
    const char*             _label,
    struct d_arg_option*    _option
)
{
    const int status = d_arg_option_reduce(_option);

    (void)d_arg_buffer_printf(_out,
                              "%s ",
                              _label);

    // the values refused
    if (status != (int)D_ARG_OK)
    {
        d_tests_arg_option_err(_out,
                               status,
                               d_arg_parser_error(&_run->parser));

        return;
    }

    size_t                          kept    = 0u;
    const struct d_pack_text* const values  =
        d_arg_option_values(_option,
                            &kept);
    size_t                          given   = 0u;
    const struct d_pack_text* const results =
        d_arg_option_results(_option,
                             &given);

    d_tests_arg_option_list(_out,
                            values,
                            kept);
    (void)d_arg_buffer_append_cstr(_out,
                                   " results=");
    d_tests_arg_option_list(_out,
                            results,
                            given);
    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

// VALIDATED_VALUES
//   constant: the values VALIDATORS gives `--v`, a list at a time: two
// that pass, the one BAD refuses once UPPER raised it, one NOX refuses
// before UPPER could, and the empty value. Each list ends in NULL.
static const char* const VALIDATED_VALUES[][3] =
{
    { "a", "b", NULL },
    { "bad", NULL, NULL },
    { "ok", "x1", NULL },
    { "", NULL, NULL }
};

// INDEXED_VALUES
//   constant: the values VALIDATORS gives `--t`, which keeps the last of
// them and checks `x` at index 0 alone, a list at a time; each ends in NULL.
static const char* const INDEXED_VALUES[][4] =
{
    { "x", "y", "z", NULL },
    { "y", "z", "x", NULL },
    { "x", NULL, NULL, NULL }
};

/**
 * @brief Appends VALIDATORS' lines for `--v`: each of VALIDATED_VALUES
 *        validated, then what its validators logged.
 *
 * @param[in,out] _out     the transcript.
 * @param[in,out] _run     the run, whose parser reports a refusal.
 * @param[in,out] _option  `--v`, with its validators.
 * @param[in,out] _log     the buffer the log is kept in.
 */
static void
d_tests_arg_option_validators_reduced(
    struct d_arg_buffer*    _out,
    struct d_tests_arg_run* _run,
    struct d_arg_option*    _option,
    struct d_arg_buffer*    _log
)
{
    // each list
    for (size_t index = 0u;
         index < sizeof(VALIDATED_VALUES) / sizeof(VALIDATED_VALUES[0]);
         ++index)
    {
        d_arg_buffer_clear(_log);
        d_tests_arg_option_give(_option,
                                VALIDATED_VALUES[index]);
        d_tests_arg_option_validated(_out,
                                     _run,
                                     "reduce",
                                     _option);
        (void)d_arg_buffer_append_cstr(_out,
                                       "each ");
        (void)d_arg_buffer_append_text(_out,
                                       d_arg_buffer_text(_log));
        (void)d_arg_buffer_append_char(_out,
                                       '\n');
    }

    return;
}

/**
 * @brief Appends VALIDATORS' lines for validators applied to one index:
 *        `--t`, which keeps its last value and checks index 0, given each
 *        of INDEXED_VALUES.
 *
 * @param[in,out] _out  the transcript.
 * @param[in,out] _run  the run, whose parser reports a refusal.
 * @param[in,out] _app  the command.
 */
static void
d_tests_arg_option_validators_indexed(
    struct d_arg_buffer*    _out,
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    struct d_arg_option* const option = d_arg_command_add_option(_app,
                                                                 "--t",
                                                                 NULL);
    struct d_arg_validator     zero   =
        d_arg_validator_custom(d_tests_arg_option_zero,
                               NULL,
                               NULL);

    d_arg_option_set_expected_range(option,
                                    1,
                                    1);
    d_arg_option_set_multi_option_policy(option,
                                         D_ARG_MULTI_TAKE_LAST);
    d_arg_validator_set_application_index(&zero,
                                          0);
    (void)d_arg_validator_set_name(&zero,
                                   "zero");
    (void)d_arg_option_check(option,
                             zero,
                             NULL);

    // each list
    for (size_t index = 0u;
         index < sizeof(INDEXED_VALUES) / sizeof(INDEXED_VALUES[0]);
         ++index)
    {
        d_tests_arg_option_give(option,
                                INDEXED_VALUES[index]);
        d_tests_arg_option_validated(_out,
                                     _run,
                                     "index",
                                     option);
    }

    return;
}

/**
 * @brief Appends VALIDATORS' lines for an empty value: not validated where
 *        no value is expected, refused where one is.
 *
 * @param[in,out] _out  the transcript.
 * @param[in,out] _run  the run, whose parser reports a refusal.
 * @param[in,out] _app  the command.
 */
static void
d_tests_arg_option_validators_empty(
    struct d_arg_buffer*    _out,
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    static const char* const EMPTY[] = { "", NULL };
    struct d_arg_option* const optional = d_arg_command_add_option(_app,
                                                                   "--e",
                                                                   NULL);
    struct d_arg_option* const required = d_arg_command_add_option(_app,
                                                                   "--f",
                                                                   NULL);

    d_arg_option_set_expected_range(optional,
                                    0,
                                    1);
    (void)d_arg_option_check(optional,
                             d_arg_validator_custom(d_tests_arg_option_empty,
                                                    NULL,
                                                    NULL),
                             NULL);
    (void)d_arg_option_check(required,
                             d_arg_validator_custom(d_tests_arg_option_empty,
                                                    NULL,
                                                    NULL),
                             NULL);
    d_tests_arg_option_give(optional,
                            EMPTY);
    d_tests_arg_option_validated(_out,
                                 _run,
                                 "empty",
                                 optional);
    d_tests_arg_option_give(required,
                            EMPTY);
    d_tests_arg_option_validated(_out,
                                 _run,
                                 "empty",
                                 required);

    return;
}

// d_tests_arg_option_bound
//   struct: the variables BINDING binds, which the options borrow for as
// long as the run lasts.
struct d_tests_arg_option_bound
{
    int          i;
    double       d;
    const char*  s;
    unsigned int u;
    long long    l;
};

/**
 * @brief Appends what binding made of an option, as BINDING writes it: its
 *        type name, its counts, and its variable's value.
 *
 * @param[in,out] _out     the transcript.
 * @param[in]     _label   what the line starts with.
 * @param[in]     _option  the option, bound.
 */
static void
d_tests_arg_option_bound_show(
    struct d_arg_buffer*       _out,
    const char*                _label,
    const struct d_arg_option* _option
)
{
    (void)d_arg_buffer_printf(_out,
                              "%s type=<",
                              _label);
    (void)d_arg_option_type_name(_option,
                                 _out);
    (void)d_arg_buffer_printf(_out,
                              "> exp=%d,%d size=%d,%d value=<",
                              _option->expected_min,
                              _option->expected_max,
                              _option->type_size_min,
                              _option->type_size_max);
    (void)d_arg_format(_option->type,
                       _option->target,
                       d_arg_buffer_sink(_out));
    (void)d_arg_buffer_append_cstr(_out,
                                   ">\n");

    return;
}

/**
 * @brief Appends a value run into a bound option's variable, as BINDING
 *        writes it: the variable's value, or the refusal.
 *
 * @param[in,out] _out     the transcript.
 * @param[in,out] _run     the run, whose parser reports a refusal.
 * @param[in]     _label   what the line starts with.
 * @param[in,out] _option  the option, bound, cleared first.
 * @param[in]     _given   the value.
 */
static void
d_tests_arg_option_bound_run(
    struct d_arg_buffer*    _out,
    struct d_tests_arg_run* _run,
    const char*             _label,
    struct d_arg_option*    _option,
    const char*             _given
)
{
    d_arg_option_clear(_option);
    (void)d_arg_option_add_result(_option,
                                  d_pack_text_from_cstr(_given),
                                  NULL);
    (void)d_arg_buffer_printf(_out,
                              "%s [%s] ",
                              _label,
                              _given);

    const int status = d_arg_option_run_callback(_option);

    // the value refused
    if (status != (int)D_ARG_OK)
    {
        d_tests_arg_option_err(_out,
                               status,
                               d_arg_parser_error(&_run->parser));

        return;
    }

    (void)d_arg_buffer_append_char(_out,
                                   '<');
    (void)d_arg_format(_option->type,
                       _option->target,
                       d_arg_buffer_sink(_out));
    (void)d_arg_buffer_append_cstr(_out,
                                   ">\n");

    return;
}

// d_tests_arg_option_binding_case
//   struct: one of BINDING's runs: the index of the option, in the order
// BINDING makes them, and the value given.
struct d_tests_arg_option_binding_case
{
    size_t      option;
    const char* given;
};

// BINDING_RUNS
//   constant: BINDING's runs, in the reference's order: each type with
// values it takes and values it refuses, the `int` past its range. A
// variable is written only after a run that passed: what one holds after a
// refusal departs from CLI11, whose conversions write part of a value.
static const struct d_tests_arg_option_binding_case BINDING_RUNS[] =
{
    { 0u, "7"                    },
    { 0u, "0x10"                 },
    { 0u, " 9 "                  },
    { 0u, "x"                    },
    { 0u, "2147483648"           },
    { 1u, "1.5"                  },
    { 1u, "1e3"                  },
    { 1u, "x"                    },
    { 2u, "text"                 },
    { 2u, ""                     },
    { 3u, "8"                    },
    { 3u, "-1"                   },
    { 4u, "-9223372036854775808" }
};

// BINDING_LABELS
//   constant: the label of each option BINDING makes, in order.
static const char* const BINDING_LABELS[] =
{
    "int", "double", "string", "unsigned", "long long"
};

// BINDING_COUNT
//   constant: how many options BINDING makes.
enum
{
    BINDING_COUNT = 5
};

/**
 * @brief Makes BINDING's options, each bound to one of its variables.
 *
 * @param[in,out] _app      the command.
 * @param[in,out] _bound    the variables, borrowed by the options.
 * @param[out]    _options  receives the options, in BINDING_LABELS' order.
 */
static void
d_tests_arg_option_bind_all(
    struct d_arg_command*            _app,
    struct d_tests_arg_option_bound* _bound,
    struct d_arg_option**            _options
)
{
    _options[0] = d_arg_command_add_option_bound(
                      _app,
                      "--i",
                      &_bound->i,
                      d_arg_type_builtin(D_ARG_BUILTIN_INT),
                      NULL);
    _options[1] = d_arg_command_add_option_bound(
                      _app,
                      "--d",
                      &_bound->d,
                      d_arg_type_builtin(D_ARG_BUILTIN_DOUBLE),
                      NULL);
    _options[2] = d_arg_command_add_option_bound(
                      _app,
                      "--s",
                      &_bound->s,
                      d_arg_type_builtin(D_ARG_BUILTIN_STRING),
                      NULL);
    _options[3] = d_arg_command_add_option_bound(
                      _app,
                      "--u",
                      &_bound->u,
                      d_arg_type_builtin(D_ARG_BUILTIN_UNSIGNED_INT),
                      NULL);
    _options[4] = d_arg_command_add_option_bound(
                      _app,
                      "--l",
                      &_bound->l,
                      d_arg_type_builtin(D_ARG_BUILTIN_LONG_LONG),
                      NULL);

    return;
}

/**
 * @brief An option callback that accepts every value, as RESULTS' does.
 *
 * @param[in] _option   the option; unused.
 * @param[in] _results  the values; unused.
 * @param[in] _count    how many; unused.
 * @param[in] _context  unused.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_option_accept(
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

    return D_ARG_OK;
}

/**
 * @brief Appends what RESULTS writes of an option: how many values it has,
 *        whether it was given, whether its callback ran, and its values.
 *
 * @param[in,out] _out     the transcript.
 * @param[in]     _label   what the line starts with.
 * @param[in]     _option  the option.
 */
static void
d_tests_arg_option_results_show(
    struct d_arg_buffer*       _out,
    const char*                _label,
    const struct d_arg_option* _option
)
{
    size_t                          count   = 0u;
    const struct d_pack_text* const results =
        d_arg_option_results(_option,
                             &count);

    (void)d_arg_buffer_printf(_out,
                              "%s count=%d given=%d run=%d results=",
                              _label,
                              (int)d_arg_option_count(_option),
                              (int)d_arg_option_is_given(_option),
                              (int)d_arg_option_callback_run(_option));
    d_tests_arg_option_list(_out,
                            results,
                            count);
    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

/**
 * @brief Appends RESULTS' lines for values added and run: one value, two
 *        split at the delimiter, the callback, and a value after it.
 *
 * @param[in,out] _out     the transcript.
 * @param[in,out] _option  `--r`.
 */
static void
d_tests_arg_option_results_added(
    struct d_arg_buffer* _out,
    struct d_arg_option* _option
)
{
    d_tests_arg_option_results_show(_out,
                                    "new",
                                    _option);
    (void)d_arg_option_add_result(_option,
                                  d_pack_text_from_cstr("a"),
                                  NULL);
    d_tests_arg_option_results_show(_out,
                                    "add a",
                                    _option);
    (void)d_arg_option_add_result(_option,
                                  d_pack_text_from_cstr("b,c"),
                                  NULL);
    d_tests_arg_option_results_show(_out,
                                    "add b,c",
                                    _option);
    (void)d_arg_option_run_callback(_option);
    d_tests_arg_option_results_show(_out,
                                    "run",
                                    _option);
    (void)d_arg_option_add_result(_option,
                                  d_pack_text_from_cstr("d"),
                                  NULL);
    d_tests_arg_option_results_show(_out,
                                    "add d",
                                    _option);

    return;
}

/**
 * @brief Appends RESULTS' lines for an option cleared and forced: cleared,
 *        its callback forced and run with no values, no longer forced, then
 *        given two values at once and read as a list.
 *
 * @param[in,out] _out     the transcript.
 * @param[in,out] _option  `--r`.
 */
static void
d_tests_arg_option_results_forced(
    struct d_arg_buffer* _out,
    struct d_arg_option* _option
)
{
    const struct d_pack_text given[] =
    {
        { "e", 1u },
        { "f,g", 3u }
    };
    struct d_pack_text       items[AS_LIST_MAX] = { D_PACK_TEXT_NONE };
    struct d_arg_text_list   list               =
    {
        items,
        0u,
        (size_t)AS_LIST_MAX
    };

    d_arg_option_clear(_option);
    d_tests_arg_option_results_show(_out,
                                    "clear",
                                    _option);
    d_arg_option_set_force_callback(_option,
                                    true);
    d_tests_arg_option_results_show(_out,
                                    "forced",
                                    _option);
    (void)d_arg_option_run_callback(_option);
    d_tests_arg_option_results_show(_out,
                                    "forced run",
                                    _option);
    d_arg_option_set_force_callback(_option,
                                    false);
    d_tests_arg_option_results_show(_out,
                                    "not forced",
                                    _option);
    (void)d_arg_option_add_results(_option,
                                   given,
                                   2u);
    d_tests_arg_option_results_show(_out,
                                    "add e f,g",
                                    _option);
    (void)d_arg_option_as(_option,
                          &LIST_TYPE,
                          &list);
    (void)d_arg_buffer_printf(_out,
                              "as %d\n",
                              (int)list.count);

    return;
}

/**
 * @brief Whether a bound `int` keeps its value when a default it cannot
 *        take is refused, as DEPARTED_DEFAULT says.
 *
 * @return whether the transcript was DEPARTED_DEFAULT.
 */
static bool
d_tests_arg_option_default_kept(void)
{
    struct d_tests_arg_run      run    = { 0 };
    struct d_arg_command* const app    = d_tests_arg_begin(&run);
    int                         n      = 12;
    struct d_arg_option* const  option =
        d_arg_command_add_option_bound(app,
                                       "--b",
                                       &n,
                                       d_arg_type_builtin(D_ARG_BUILTIN_INT),
                                       NULL);
    struct d_arg_buffer         out    = D_ARG_BUFFER_INIT;

    (void)d_arg_option_set_default_value(option,
                                         "x");
    (void)d_arg_buffer_printf(&out,
                              "n=%d\n",
                              n);

    const bool same = d_tests_arg_same("DEPARTED_DEFAULT",
                                       d_arg_buffer_text(&out),
                                       DEPARTED_DEFAULT);

    d_arg_buffer_release(&out);
    d_tests_arg_end(&run);

    return same;
}

//==============================================================================
// 1.  NAMES
//==============================================================================

/*
d_tests_arg_option_names
  What a definition string makes of names, as CLI11's add_option does.
  Tests the following:
  - `-x` is a short name, `--x` a long name, anything else the positional
    name; spaces around a name and empty parts are ignored, a name given
    twice is kept twice, and case is kept
  - the name written plainly -- the first long name, else the first short,
    else the positional -- with the positional name, with every name, and
    with both
  - the single name, without dashes; whether the option is positional,
    nonpositional, or both; the empty definition, an option of no names
  - CLI11's refusals, BadNameString: a short name of more than one
    character, the reserved names, a long name holding `=` or a space, two
    positional names, and a short name that is no name character
*/
bool
d_tests_arg_option_names(void)
{
    struct d_arg_buffer out = D_ARG_BUFFER_INIT;

    // each definition, in a parser of its own
    for (size_t index = 0u; DEFINITIONS[index] != NULL; ++index)
    {
        d_tests_arg_option_define(&out,
                                  DEFINITIONS[index]);
    }

    const bool same = d_tests_arg_same("NAMES",
                                       d_arg_buffer_text(&out),
                                       NAMES);

    d_arg_buffer_release(&out);

    return same;
}

/*
d_tests_arg_option_flag_names
  What a flag's definition string makes of names and values, as CLI11's
add_flag does.
  Tests the following:
  - a name's value in braces, `!` for `false`, and an empty value, kept as
    given
  - every name written with its value, `{value}`, and without it
  - the default text of a flag, which none of these sets
  - a positional name refused with IncorrectConstruction, "Flags cannot be
    positional"
*/
bool
d_tests_arg_option_flag_names(void)
{
    struct d_arg_buffer out = D_ARG_BUFFER_INIT;

    // each definition, in a parser of its own
    for (size_t index = 0u; FLAG_DEFINITIONS[index] != NULL; ++index)
    {
        d_tests_arg_option_define_flag(&out,
                                       FLAG_DEFINITIONS[index]);
    }

    const bool same = d_tests_arg_same("FLAG_NAMES",
                                       d_arg_buffer_text(&out),
                                       FLAG_NAMES);

    d_arg_buffer_release(&out);

    return same;
}

/*
d_tests_arg_option_taken
  The names a command refuses because another of its options holds them.
  Tests the following:
  - a long, short or positional name another option holds, refused with
    OptionAlreadyAdded, naming the name in use and the option that holds
    it
  - a name in another case is another name
  - flags refused alike for a name an option or a flag holds, `!--x`
    holding `x`
*/
bool
d_tests_arg_option_taken(void)
{
    struct d_tests_arg_run      run = { 0 };
    struct d_arg_command* const app = d_tests_arg_begin(&run);
    struct d_arg_buffer         out = D_ARG_BUFFER_INIT;

    // each option in turn, in the one command
    for (size_t index = 0u; TAKEN_OPTIONS[index] != NULL; ++index)
    {
        d_tests_arg_option_try(&out,
                               &run,
                               app,
                               TAKEN_OPTIONS[index],
                               false);
    }

    // then each flag
    for (size_t index = 0u; TAKEN_FLAGS[index] != NULL; ++index)
    {
        d_tests_arg_option_try(&out,
                               &run,
                               app,
                               TAKEN_FLAGS[index],
                               true);
    }

    const bool same = d_tests_arg_same("TAKEN",
                                       d_arg_buffer_text(&out),
                                       TAKEN);

    d_arg_buffer_release(&out);
    d_tests_arg_end(&run);

    return same;
}

//==============================================================================
// 2.  MATCHING
//==============================================================================

/*
d_tests_arg_option_matching
  Which names an option and a flag answer to, as CLI11's check_name,
check_sname, check_lname and check_fname.
  Tests the following:
  - check_name: a long name with `--`, a short name with `-`, and the
    positional name or the environment variable's name as they are; a
    long name without its dashes is no name
  - each with case ignored, with underscores ignored, and with both, but
    the environment variable's name, matched exactly
  - a flag answers to its own names only
  - check_sname, check_lname and check_fname, which take a name without
    dashes, under the same settings; check_fname, only the names that
    carry a value
*/
bool
d_tests_arg_option_matching(void)
{
    struct d_arg_buffer out = D_ARG_BUFFER_INIT;

    // each setting: neither, case, underscores, both
    for (unsigned int setting = 0u;
         setting < (unsigned int)MATCH_SETTINGS;
         ++setting)
    {
        d_tests_arg_option_match(&out,
                                 setting);
    }

    const bool same = d_tests_arg_same("MATCH",
                                       d_arg_buffer_text(&out),
                                       MATCH);

    d_arg_buffer_release(&out);

    return same;
}

/*
d_tests_arg_option_conflicts
  Ignoring case or underscores where two options' names would then match,
and the names two options share, as CLI11's matching_name.
  Tests the following:
  - ignore_case and ignore_underscore refused with OptionAlreadyAdded --
    "adding ignore case caused a name conflict with <name> is already
    added" -- when a long, short or positional name of another option would
    then match, the setting kept off; allowed, and kept, when none would
  - matching_name: a long name both hold; a one-character long name and a
    short name, and a positional name and any other name, only while both
    options are configurable; a name in another case once either option
    ignores case
*/
bool
d_tests_arg_option_conflicts(void)
{
    struct d_arg_buffer out = D_ARG_BUFFER_INIT;

    // each case, in a parser of its own
    for (size_t index = 0u; index < IGNORE_COUNT; ++index)
    {
        d_tests_arg_option_ignored(&out,
                                   &IGNORE_CASES[index]);
    }

    d_tests_arg_option_sharing(&out);

    const bool same = d_tests_arg_same("CONFLICT",
                                       d_arg_buffer_text(&out),
                                       CONFLICT);

    d_arg_buffer_release(&out);

    return same;
}

//==============================================================================
// 3.  DEFINITION
//==============================================================================

/*
d_tests_arg_option_text
  An option's text and type name, as CLI11's.
  Tests the following:
  - description, option_text, type_name and envname, set and emptied
  - the type name of an option bound to an `int`, and of one bound to
    nothing, which is empty
  - the type name followed by each validator's description after a colon,
    a transformation's first, as CLI11 inserts it; each() describing
    nothing; type_name_fn's text in place of the name
  - a group named; one with a newline refused with IncorrectConstruction,
    the group kept; the empty group, which hides the option, so that its
    name is written as nothing
*/
bool
d_tests_arg_option_text(void)
{
    struct d_tests_arg_run      run    = { 0 };
    struct d_arg_command* const app    = d_tests_arg_begin(&run);
    int                         bound  = 0;
    struct d_arg_option* const  option =
        d_arg_command_add_option(app,
                                 "-t,--text",
                                 "a description");
    struct d_arg_option* const  number =
        d_arg_command_add_option_bound(app,
                                       "--bound",
                                       &bound,
                                       d_arg_type_builtin(D_ARG_BUILTIN_INT),
                                       NULL);
    struct d_arg_buffer         out    = D_ARG_BUFFER_INIT;

    d_tests_arg_option_text_show(&out,
                                 "new",
                                 option);
    d_tests_arg_option_text_show(&out,
                                 "bound",
                                 number);
    d_tests_arg_option_text_named(&out,
                                  option);
    d_tests_arg_option_text_validated(&out,
                                      option);
    d_tests_arg_option_text_grouped(&out,
                                    &run,
                                    option);
    d_tests_arg_option_text_cleared(&out,
                                    option);

    const bool same = d_tests_arg_same("TEXT",
                                       d_arg_buffer_text(&out),
                                       TEXT);

    d_arg_buffer_release(&out);
    d_tests_arg_end(&run);

    return same;
}

/*
d_tests_arg_option_counts
  The counts each count setter leaves, as CLI11's.
  Tests the following:
  - expected(n): exactly n, 0 for none; a negative n, at least |n| --
    keeping a larger maximum -- and extra arguments; unlimited, at least
    one and extra arguments
  - expected(min, max): negative bounds -- the minimum's magnitude, an
    unlimited maximum -- and reversed bounds
  - type_size(n) and type_size(min, max): negative sizes, which make the
    count unlimited; a size of 0, which clears required; an unlimited
    size, which injects separators
  - the items expected: the type size times the expected count
  - leaving the Throw policy on an option expecting at least two items and
    unlimited ones limits it to its minimum; another change does not
  - allow_extra_args and required, set and cleared
*/
bool
d_tests_arg_option_counts(void)
{
    return d_tests_arg_option_counted("COUNTS",
                                      COUNT_STEPS,
                                      COUNT_STEP_COUNT,
                                      COUNTS);
}

/*
d_tests_arg_option_defaults
  The default text, set, captured and checked, as CLI11's default_str,
capture_default_str, default_val and default_function.
  Tests the following:
  - an option starts without default text, bound or not; capturing gives
    a bound variable's value as it is then, and nothing without a variable
  - default_str sets the text alone; default_val checks the value as one
    given, leaves the values given as they were, and runs the callback --
    the bound variable takes the default -- unless run_callback_for_default
    is off
  - a default the variable cannot take refused with ConversionError, in
    CLI11's words, the default text kept; one a validator refuses, with
    ValidationError
  - a default function, captured in place of the variable
*/
bool
d_tests_arg_option_defaults(void)
{
    struct d_tests_arg_run              run       = { 0 };
    struct d_arg_command* const         app       = d_tests_arg_begin(&run);
    struct d_arg_buffer                 out       = D_ARG_BUFFER_INIT;
    struct d_tests_arg_option_defaulted defaulted =
    {
        &run,
        &out,
        NULL,
        NULL,
        NULL,
        3,
        "start"
    };

    defaulted.unbound = d_arg_command_add_option(app,
                                                 "--a",
                                                 NULL);
    defaulted.number  = d_arg_command_add_option_bound(
                            app,
                            "--b",
                            &defaulted.n,
                            d_arg_type_builtin(D_ARG_BUILTIN_INT),
                            NULL);
    defaulted.text    = d_arg_command_add_option_bound(
                            app,
                            "--c",
                            &defaulted.s,
                            d_arg_type_builtin(D_ARG_BUILTIN_STRING),
                            NULL);
    d_tests_arg_option_defaults_captured(&defaulted);
    d_tests_arg_option_defaults_valued(&defaulted);
    d_tests_arg_option_defaults_checked(&defaulted);

    const bool same = d_tests_arg_same("DEFAULTS",
                                       d_arg_buffer_text(&out),
                                       DEFAULTS);

    d_arg_buffer_release(&out);
    d_tests_arg_end(&run);

    return same;
}

/*
d_tests_arg_option_relations
  Options an option needs and excludes, as CLI11's needs, excludes,
remove_needs and remove_excludes.
  Tests the following:
  - needs by option and by name, an option needing itself ignored and one
    needed twice kept once
  - a name no option has refused with IncorrectConstruction, "Option
    <name> is not defined": a long name without its dashes, or with one,
    and the empty name among them
  - an option excluding itself refused, "and option cannot exclude
    itself"; an exclusion recorded on both options, by option and by name
  - removal of a need or of one side of an exclusion, true only the first
    time
*/
bool
d_tests_arg_option_relations(void)
{
    struct d_tests_arg_run      run                    = { 0 };
    struct d_arg_command* const app                    =
        d_tests_arg_begin(&run);
    struct d_arg_option*        options[RELATED_COUNT] = { NULL };
    struct d_arg_buffer         out                    = D_ARG_BUFFER_INIT;

    d_tests_arg_option_relatable(app,
                                 options);
    d_tests_arg_option_relations_show(&out,
                                      "new",
                                      options);
    (void)d_arg_option_needs(options[0],
                             options[1]);
    (void)d_arg_option_needs(options[0],
                             options[0]);
    (void)d_arg_option_needs_name(options[0],
                                  "-c");
    (void)d_arg_option_needs(options[0],
                             options[1]);
    d_tests_arg_option_relations_show(&out,
                                      "needs",
                                      options);
    d_tests_arg_option_relations_refused(&out,
                                         &run,
                                         options);
    (void)d_arg_option_excludes(options[1],
                                options[2]);
    (void)d_arg_option_excludes_name(options[0],
                                     "pos");
    (void)d_arg_option_excludes(options[1],
                                options[2]);
    d_tests_arg_option_relations_show(&out,
                                      "excludes",
                                      options);
    d_tests_arg_option_relations_removed(&out,
                                         options);
    d_tests_arg_option_relations_show(&out,
                                      "removed",
                                      options);

    const bool same = d_tests_arg_same("RELATIONS",
                                       d_arg_buffer_text(&out),
                                       RELATIONS);

    d_arg_buffer_release(&out);
    d_tests_arg_end(&run);

    return same;
}

/*
d_tests_arg_option_validators
  Validators on an option, as CLI11's check, transform, each and
get_validator.
  Tests the following:
  - checks run in the order added, after the transformations, each added
    before the others; each() runs where it was added
  - a validator found by name -- the empty name finding the unnamed one --
    and by index; none past either
  - values changed by a transformation, and kept as given among the
    option's results; a check refusing one with ValidationError, "<name>:
    <reason>"; what each() saw
  - a validator applied to one index, counted from the end for TakeLast
  - an empty value not validated where no value is expected
*/
bool
d_tests_arg_option_validators(void)
{
    struct d_tests_arg_run      run    = { 0 };
    struct d_arg_command* const app    = d_tests_arg_begin(&run);
    struct d_arg_option* const  option = d_arg_command_add_option(app,
                                                                  "--v",
                                                                  NULL);
    struct d_arg_buffer         out    = D_ARG_BUFFER_INIT;
    struct d_arg_buffer         log    = D_ARG_BUFFER_INIT;

    d_arg_option_set_expected_range(option,
                                    1,
                                    4);
    d_tests_arg_option_validators_added(option,
                                        &log);
    d_tests_arg_option_validators_found(&out,
                                        option);
    d_tests_arg_option_validators_reduced(&out,
                                          &run,
                                          option,
                                          &log);
    d_tests_arg_option_validators_indexed(&out,
                                          &run,
                                          app);
    d_tests_arg_option_validators_empty(&out,
                                        &run,
                                        app);

    const bool same = d_tests_arg_same("VALIDATORS",
                                       d_arg_buffer_text(&out),
                                       VALIDATORS);

    d_tests_arg_end(&run);
    d_arg_buffer_release(&out);
    d_arg_buffer_release(&log);

    return same;
}

//==============================================================================
// 4.  VALUES
//==============================================================================

/*
d_tests_arg_option_configurations
  Values added to options of each configuration, then reduced and read, as
CLI11's add_result, reduced_results and as<std::string>.
  Tests the following:
  - a value split at the delimiter, its empty parts dropped; a bracketed
    list split at commas for an option taking several values or items of
    several values, or allowing extra arguments; `[[...]]` with each
    character doubled, a bracketed value as it is; how many values each
    made
  - each policy reducing one to four values: Throw's ArgumentMismatch for
    too many or too few, TakeLast, TakeFirst, Join -- with a newline or the
    delimiter -- TakeAll, Sum and Reverse, two at a time where two are
    expected; `{}`, and `{}` and `%%`, the empty container's markers
  - the values read as one text: the first of several, the empty text for
    none
*/
bool
d_tests_arg_option_configurations(void)
{
    bool all = true;

    // each configuration, in a parser of its own
    for (size_t index = 0u; index < CONFIG_COUNT; ++index)
    {
        all = ( (d_tests_arg_option_configured(index)) &&
                (all) );
    }

    return all;
}

/*
d_tests_arg_option_as
  Values and defaults read as a list of text and as one text, as CLI11's
as<std::vector<std::string>> and as<std::string>.
  Tests the following:
  - an option given no values reads its default text, split and reduced as
    a value given would be; one without a default reads an empty value
  - each policy's values read as a list, and as one text -- the first
  - a policy's refusal, for both
  - `[]`, which leaves no values: Throw refuses it, TakeAll reads none
  - values given rather than the default
*/
bool
d_tests_arg_option_as(void)
{
    struct d_arg_buffer out = D_ARG_BUFFER_INIT;

    // each case, in a parser of its own
    for (size_t index = 0u; index < AS_COUNT; ++index)
    {
        d_tests_arg_option_as_one(&out,
                                  index);
    }

    const bool same = d_tests_arg_same("AS",
                                       d_arg_buffer_text(&out),
                                       AS);

    d_arg_buffer_release(&out);

    return same;
}

/*
d_tests_arg_option_flag_values
  The value a flag name stands for, given with an input after `=` or
without one, as CLI11's get_flag_value, under each configuration of the
flag `--flag,--no-flag{false},-f{2}`.
  Tests the following:
  - with no input, or `{}`: `true` for a name without a value of its own,
    else the name's value; for a flag expecting a value, its default text
  - with an input: the input; for a name whose value is `false`, the input
    inverted -- `false` for `true` or 1, `true` for `false`, 0 or -1, a
    number's negation, a word as it is
  - an override refused with ArgumentMismatch when the flag disables them:
    any input but the name's own value -- `true` for a name without one --
    unless it is a forced flag's default text
  - names matched as the flag matches them: with case and underscores
    ignored, `NO-FLAG` and `no_flag` are `no-flag`
*/
bool
d_tests_arg_option_flag_values(void)
{
    bool all = true;

    // each half of each configuration, in a parser of its own
    for (size_t index = 0u; index < FLAG_TRANSCRIPT_COUNT; ++index)
    {
        all = ( (d_tests_arg_option_flag_half(index)) &&
                (all) );
    }

    return all;
}

//==============================================================================
// 5.  CALLBACKS AND BINDING
//==============================================================================

/*
d_tests_arg_option_callbacks
  The callback given an option's values, as CLI11's run_callback.
  Tests the following:
  - the callback given what the policy kept, once, and whether it ran; the
    results left as they were given
  - a callback that refuses: ConversionError, "Could not convert: <name> =
    <values>", the values given joined by commas -- and a hidden option's
    name written as nothing
  - a forced option with no values: its default text, split and reduced,
    or an empty value without one, dropped again after the callback ran;
    kept by an option without a callback
  - Throw's refusal of too many or too few values before any callback;
    Sum's total; Reverse's values
*/
bool
d_tests_arg_option_callbacks(void)
{
    struct d_arg_buffer out = D_ARG_BUFFER_INIT;

    // each case, in a parser of its own
    for (size_t index = 0u; index < CALLBACK_COUNT; ++index)
    {
        d_tests_arg_option_call(&out,
                                index);
    }

    const bool same = d_tests_arg_same("CALLBACKS",
                                       d_arg_buffer_text(&out),
                                       CALLBACKS);

    d_arg_buffer_release(&out);

    return same;
}

/*
d_tests_arg_option_binding
  Options bound to variables, as CLI11's add_option(name, variable).
  Tests the following:
  - binding gives the option its type's name and counts: INT, FLOAT, TEXT,
    UINT, and INT for a `long long`
  - run_callback converting a value into the variable: an `int` from
    decimal, hexadecimal and spaced text, a `double` with an exponent, text
    including the empty text, a `long long` at its minimum
  - a value a type cannot take refused with ConversionError, "Could not
    convert: <name> = <value>": a word, an `int` past its range, a negative
    `unsigned`
*/
bool
d_tests_arg_option_binding(void)
{
    struct d_tests_arg_run          run                    = { 0 };
    struct d_arg_command* const     app                    =
        d_tests_arg_begin(&run);
    struct d_tests_arg_option_bound bound                  =
    {
        1,
        0.5,
        "s",
        2u,
        3LL
    };
    struct d_arg_option*            options[BINDING_COUNT] = { NULL };
    struct d_arg_buffer             out                    =
        D_ARG_BUFFER_INIT;

    d_tests_arg_option_bind_all(app,
                                &bound,
                                options);

    // each option, as binding made it
    for (size_t index = 0u; index < (size_t)BINDING_COUNT; ++index)
    {
        d_tests_arg_option_bound_show(&out,
                                      BINDING_LABELS[index],
                                      options[index]);
    }

    // each run, in order
    for (size_t index = 0u;
         index < sizeof(BINDING_RUNS) / sizeof(BINDING_RUNS[0]);
         ++index)
    {
        const size_t option = BINDING_RUNS[index].option;

        d_tests_arg_option_bound_run(&out,
                                     &run,
                                     BINDING_LABELS[option],
                                     options[option],
                                     BINDING_RUNS[index].given);
    }

    const bool same = d_tests_arg_same("BINDING",
                                       d_arg_buffer_text(&out),
                                       BINDING);

    d_arg_buffer_release(&out);
    d_tests_arg_end(&run);

    return same;
}

//==============================================================================
// 6.  RESULTS
//==============================================================================

/*
d_tests_arg_option_results
  What an option says about its values, as CLI11's count, operator bool,
get_callback_run and clear.
  Tests the following:
  - the count of values given, each split at the delimiter; an option is
    given once it has one
  - the callback's run, reset by a value added after it and by clearing
  - a forced callback makes an option given without values, and runs with
    none; unforced, it is not given again
  - values added several at once, read as a list
*/
bool
d_tests_arg_option_results(void)
{
    struct d_tests_arg_run      run    = { 0 };
    struct d_arg_command* const app    = d_tests_arg_begin(&run);
    struct d_arg_option* const  option =
        d_arg_command_add_option_callback(app,
                                          "--r",
                                          d_tests_arg_option_accept,
                                          NULL,
                                          NULL,
                                          NULL);
    struct d_arg_buffer         out    = D_ARG_BUFFER_INIT;

    d_arg_option_set_expected_range(option,
                                    1,
                                    3);
    d_arg_option_set_delimiter(option,
                               ',');
    d_tests_arg_option_results_added(&out,
                                     option);
    d_tests_arg_option_results_forced(&out,
                                      option);

    const bool same = d_tests_arg_same("RESULTS",
                                       d_arg_buffer_text(&out),
                                       RESULTS);

    d_arg_buffer_release(&out);
    d_tests_arg_end(&run);

    return same;
}

//==============================================================================
// 7.  DEPARTURES FROM CLI11
//==============================================================================

/*
d_tests_arg_option_departures
  Where the module does otherwise than CLI11, on purpose: each constant
says what CLI11 does instead.
  Tests the following:
  - items expected past what an `int` holds -- the type size times the
    expected count -- held at D_ARG_EXPECTED_UNLIMITED for the minimum, as
    for the maximum
  - a bound `int` keeps its value when a default it cannot take is refused
*/
bool
d_tests_arg_option_departures(void)
{
    const bool counts = d_tests_arg_option_counted("DEPARTED_COUNTS",
                                                   DEPARTED_STEPS,
                                                   DEPARTED_COUNT,
                                                   DEPARTED_COUNTS);
    const bool kept   = d_tests_arg_option_default_kept();

    return ( (counts) &&
             (kept) );
}

//==============================================================================
// 8.  THE SUITE
//==============================================================================

// d_tests_arg_option
//   constant: the suite, in the header's order.
const struct d_tests_section d_tests_arg_option[] =
{
    { "arg_option_names",
      d_tests_arg_option_names },
    { "arg_option_flag_names",
      d_tests_arg_option_flag_names },
    { "arg_option_taken",
      d_tests_arg_option_taken },
    { "arg_option_matching",
      d_tests_arg_option_matching },
    { "arg_option_conflicts",
      d_tests_arg_option_conflicts },
    { "arg_option_text",
      d_tests_arg_option_text },
    { "arg_option_counts",
      d_tests_arg_option_counts },
    { "arg_option_defaults",
      d_tests_arg_option_defaults },
    { "arg_option_relations",
      d_tests_arg_option_relations },
    { "arg_option_validators",
      d_tests_arg_option_validators },
    { "arg_option_configurations",
      d_tests_arg_option_configurations },
    { "arg_option_as",
      d_tests_arg_option_as },
    { "arg_option_flag_values",
      d_tests_arg_option_flag_values },
    { "arg_option_callbacks",
      d_tests_arg_option_callbacks },
    { "arg_option_binding",
      d_tests_arg_option_binding },
    { "arg_option_results",
      d_tests_arg_option_results },
    { "arg_option_departures",
      d_tests_arg_option_departures }
};

// d_tests_arg_option_count
//   constant: how many sections the table above holds.
const size_t d_tests_arg_option_count =
    sizeof(d_tests_arg_option) / sizeof(d_tests_arg_option[0]);
