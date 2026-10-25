/*******************************************************************************
* djinterp [parse]                                           arg_command_tests.c
*
* The suite of the argument parser's commands, declared in
* arg_command_tests.h.
*   Each case makes its commands as CLI11's App("d", "prog") would, on a
* run's parser of its own (arg_tests_support.h), and writes what they came to
* as the command suite's CLI11 reference writes CLI11's: one transcript for
* each of the reference's blocks, compared whole. A definition is written
* `<tag>: ok`, or `<tag>: <name>: <message>` for the refusal the parser
* recorded, which is then cleared, as catching CLI11's exception lets a
* program go on; a parse as the runs report one, after a line of its
* arguments in brackets.
*   The cases of the earlier command probes keep their tags. The departures
* from CLI11 are sections of their own, whose constants say what CLI11 does
* instead: sums add_flag_function cannot read, and leading zeros read as
* decimal; what CLI11 leaves pointing at released records, destroys, or lets
* two parsers share; the contexts a C caller gives with its functions; and
* counts and values no CLI11 call can be given.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_command_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./arg_command_tests.h"  // corresponding header
// std
#include <limits.h>   // INT_MIN, LONG_MAX
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
// djinterp
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_command.h"  // the commands
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // statuses, buffers, policies
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_option.h"  // options, names, relations
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // parsers, error reports
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_type.h"  // descriptors
#include "../../../c/tests_section.h"  // d_tests_section
#include "./arg_tests_support.h"  // runs, comparisons
// re_std
#include "../../../../../../inc/re_std/cstdint/dstdint.h"  // intmax_t, uint32_t


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// d_tests_arg_command_place
//   enum: where a case of ADD makes an option: the root, the option group G,
// the group G2 made inside G, or the subcommand s; and how many places there
// are.
enum d_tests_arg_command_place
{
    IN_ROOT     = 0,
    IN_GROUP    = 1,
    IN_NESTED   = 2,
    IN_SUB      = 3,
    PLACE_COUNT = 4
};

// d_tests_arg_command_setting
//   enum: what a case of ADD sets: nothing; on the root, before anything is
// made, non-standard option names, or ignore_case in its option defaults;
// on the first option, once it is made, not configurable, or the empty
// group.
enum d_tests_arg_command_setting
{
    SET_PLAIN          = 0,
    SET_NON_STANDARD   = 1,
    SET_IGNORING       = 2,
    SET_UNCONFIGURABLE = 3,
    SET_HIDDEN         = 4
};

// d_tests_arg_command_add_case
//   struct: one case of ADD: its tag and setting, the option made first and
// where, then the option tried and where.
struct d_tests_arg_command_add_case
{
    const char* tag;
    int         setting;
    const char* first;
    int         holder;
    const char* tried;
    int         target;
};

// ADD_CASES
//   constant: ADD's cases, each in a parser of its own: a long, short and
// positional name taken, and a positional name that is another option's
// long or short name; the same names tried in an option group of the
// command that holds them, an unconfigurable one among them; short names of
// more than one character; a hidden option's name; a name in another case
// under ignore_case; a name a group holds, tried in its parent, and one the
// parent holds, tried in a subcommand; the third name of three; and a name
// tried in a group inside a group.
static const struct d_tests_arg_command_add_case ADD_CASES[] =
{
    { "A01", SET_PLAIN,
      "-a,--alpha", IN_ROOT,  "--alpha",    IN_ROOT   },
    { "A02", SET_PLAIN,
      "-a,--alpha", IN_ROOT,  "-a,--other", IN_ROOT   },
    { "A03", SET_PLAIN,
      "pos",        IN_ROOT,  "pos",        IN_ROOT   },
    { "A04", SET_PLAIN,
      "--file",     IN_ROOT,  "file",       IN_ROOT   },
    { "A05", SET_PLAIN,
      "-f",         IN_ROOT,  "f",          IN_ROOT   },
    { "A06", SET_PLAIN,
      "pos",        IN_ROOT,  "pos",        IN_GROUP  },
    { "A07", SET_PLAIN,
      "--beta",     IN_ROOT,  "--beta",     IN_GROUP  },
    { "A08", SET_PLAIN,
      "-c",         IN_ROOT,  "-c",         IN_GROUP  },
    { "A09", SET_PLAIN,
      "d",          IN_ROOT,  "-d",         IN_GROUP  },
    { "A10", SET_UNCONFIGURABLE,
      "--e",        IN_ROOT,  "--e",        IN_GROUP  },
    { "A11", SET_NON_STANDARD,
      "-ab",        IN_ROOT,  "-a",         IN_ROOT   },
    { "A12", SET_NON_STANDARD,
      "-a",         IN_ROOT,  "-ab",        IN_ROOT   },
    { "A13", SET_HIDDEN,
      "--h1",       IN_ROOT,  "--h1",       IN_ROOT   },
    { "A14", SET_IGNORING,
      "--Gamma",    IN_ROOT,  "--gamma",    IN_ROOT   },
    { "A15", SET_PLAIN,
      "--in",       IN_GROUP, "--in",       IN_ROOT   },
    { "A16", SET_PLAIN,
      "--top",      IN_ROOT,  "--top",      IN_SUB    },
    { "A17", SET_PLAIN,
      "--x,-y,z",   IN_ROOT,  "y",          IN_ROOT   },
    { "A18", SET_PLAIN,
      "--deep",     IN_ROOT,  "--deep",     IN_NESTED }
};

// ADD_COUNT
//   constant: how many cases ADD_CASES holds.
static const size_t ADD_COUNT = sizeof(ADD_CASES) / sizeof(ADD_CASES[0]);

// d_tests_arg_command_flag_kind
//   enum: the call a case of FLAGS makes its flag with: add_flag;
// add_flag_callback; add_flag_function; or add_flag bound to an `int`, a
// `bool`, a list of text or a `char`.
enum d_tests_arg_command_flag_kind
{
    KIND_FLAG     = 0,
    KIND_CALLBACK = 1,
    KIND_FUNCTION = 2,
    KIND_INT      = 3,
    KIND_BOOL     = 4,
    KIND_LIST     = 5,
    KIND_CHAR     = 6
};

// d_tests_arg_command_flag_case
//   struct: one case of FLAGS: its tag, a flag made before it -- NULL for
// none -- and the flag tried, with the call that makes it.
struct d_tests_arg_command_flag_case
{
    const char* tag;
    const char* first;
    const char* tried;
    int         kind;
};

// FLAG_CASES
//   constant: FLAGS' cases, each in a parser of its own: values in braces,
// `!`, positional names, which a flag cannot have, two kinds of `false`
// beside a plain name, a name taken; each call that takes a function; and
// a flag bound to each kind of variable CLI11 treats apart, then one whose
// name is taken.
static const struct d_tests_arg_command_flag_case FLAG_CASES[] =
{
    { "F01", NULL,      "--f1{7}",                  KIND_FLAG     },
    { "F02", NULL,      "-g,--gee{3},!--no-gee",    KIND_FLAG     },
    { "F03", NULL,      "fpos",                     KIND_FLAG     },
    { "F04", NULL,      "--x,y",                    KIND_FLAG     },
    { "F05", NULL,      "--neg{false},-n{0},--pos", KIND_FLAG     },
    { "F06", "--dup",   "--dup{2}",                 KIND_FLAG     },
    { "F07", NULL,      "--cb",                     KIND_CALLBACK },
    { "F08", NULL,      "--fn",                     KIND_FUNCTION },
    { "F09", NULL,      "-n,--count",               KIND_INT      },
    { "F10", NULL,      "--on",                     KIND_BOOL     },
    { "F11", NULL,      "--many",                   KIND_LIST     },
    { "F12", NULL,      "--c",                      KIND_CHAR     },
    { "F13", "--taken", "--taken",                  KIND_INT      }
};

// FLAG_COUNT
//   constant: how many cases FLAG_CASES holds.
static const size_t FLAG_COUNT = sizeof(FLAG_CASES) / sizeof(FLAG_CASES[0]);

// d_tests_arg_command_texts
//   struct: the list of text a flag adds its values to, as CLI11's add_flag
// adds them to a std::vector<std::string>: the values copied, joined by
// `|`, and how many there are.
struct d_tests_arg_command_texts
{
    struct d_arg_buffer joined;
    size_t              count;
};

// d_tests_arg_command_bound
//   struct: the variables FLAGS and FLAG_BOUND bind flags to, as the
// reference's: a count, a truth value, a list of text and a character.
struct d_tests_arg_command_bound
{
    int                              count;
    bool                             on;
    struct d_tests_arg_command_texts many;
    char                             character;
};

// ARGS_MAX
//   constant: how many arguments, and the NULL after them, one parse of a
// run lists.
enum
{
    ARGS_MAX = 16
};

// d_tests_arg_command_args
//   struct: the arguments of one parse, ending in NULL.
struct d_tests_arg_command_args
{
    const char* list[ARGS_MAX];
};

// BOUND_ARGS
//   constant: FLAG_BOUND's parses, in one parser: each flag given, the
// count's short name three times in one argument; values given to each;
// nothing; and a value the count cannot read, then one the truth value
// cannot.
static const struct d_tests_arg_command_args BOUND_ARGS[] =
{
    { { "-nnn", "--count", "--on", "--many", "--many=v", "--c=y", NULL } },
    { { "--count=-2", "--on=false", "--many=w", "--c=z", "-n", NULL } },
    { { NULL } },
    { { "--count=x", NULL } },
    { { "--on=maybe", NULL } }
};

// BOUND_COUNT
//   constant: how many parses BOUND_ARGS holds.
static const size_t BOUND_COUNT = sizeof(BOUND_ARGS) / sizeof(BOUND_ARGS[0]);

// SUM_ARGS
//   constant: FLAG_SUMS' parses, in one parser: the callback's flag true,
// false, and neither; the summing flag once, four times in three
// arguments, given a number and `false`, given a word, beside the
// callback's; a number with an exponent, hexadecimal, octal by its leading
// zero, a leading zero before a digit octal has not, and binary; and
// nothing.
static const struct d_tests_arg_command_args SUM_ARGS[] =
{
    { { "--cb", NULL } },
    { { "--cb=false", NULL } },
    { { "--cb=x", NULL } },
    { { "--fn", NULL } },
    { { "--fn", "-ff", "--fn=3", NULL } },
    { { "--fn=-2", "--fn=false", NULL } },
    { { "--fn=x", NULL } },
    { { "--cb", "--fn=0", NULL } },
    { { "--fn=1e3", NULL } },
    { { "--fn=0x10", NULL } },
    { { "--fn=010", NULL } },
    { { "--fn=08", NULL } },
    { { "--fn=0b11", "--fn", NULL } },
    { { NULL } }
};

// SUM_COUNT
//   constant: how many parses SUM_ARGS holds.
static const size_t SUM_COUNT = sizeof(SUM_ARGS) / sizeof(SUM_ARGS[0]);

// WIDE_ARGS
//   constant: FLAG_SUMS_WIDE's parses: sums past 32 bits, the largest and
// smallest 64-bit integers, and a sum past 32 bits of two values.
static const struct d_tests_arg_command_args WIDE_ARGS[] =
{
    { { "--fn=5000000000", NULL } },
    { { "--fn=9223372036854775807", NULL } },
    { { "--fn=-9223372036854775808", NULL } },
    { { "--fn=4294967296", "-f", NULL } }
};

// WIDE_COUNT
//   constant: how many parses WIDE_ARGS holds.
static const size_t WIDE_COUNT = sizeof(WIDE_ARGS) / sizeof(WIDE_ARGS[0]);

// UNREAD_ARGS
//   constant: the parses of the sums that are no integer: a fraction, a sum
// with a fraction, a number followed by letters, a number beside a word,
// and totals past the largest and the smallest 64-bit integer.
static const struct d_tests_arg_command_args UNREAD_ARGS[] =
{
    { { "--fn=1.5", NULL } },
    { { "--fn=2.5", "--fn=1", NULL } },
    { { "--fn=12abc", NULL } },
    { { "--fn=3", "--fn=x", NULL } },
    { { "--fn=9223372036854775807", "--fn", NULL } },
    { { "--fn=-9223372036854775808", "--fn=-1", NULL } }
};

// UNREAD_COUNT
//   constant: how many parses UNREAD_ARGS holds.
static const size_t UNREAD_COUNT =
    sizeof(UNREAD_ARGS) / sizeof(UNREAD_ARGS[0]);

// DECIMAL_ARGS
//   constant: the parses of numbers with a leading zero, under the module's
// rule: a leading zero before octal digits, before a digit octal has not,
// and before `o`.
static const struct d_tests_arg_command_args DECIMAL_ARGS[] =
{
    { { "--fn=010", NULL } },
    { { "--fn=08", NULL } },
    { { "--fn=0o10", NULL } }
};

// DECIMAL_COUNT
//   constant: how many parses DECIMAL_ARGS holds.
static const size_t DECIMAL_COUNT =
    sizeof(DECIMAL_ARGS) / sizeof(DECIMAL_ARGS[0]);

// d_tests_arg_command_named
//   struct: a subcommand's name tried on a parser of its own, under a tag.
struct d_tests_arg_command_named
{
    const char* tag;
    const char* name;
};

// REFUSED_NAMES
//   constant: the names SUBCOMMANDS tries first: a leading dash, `=` inside
// and leading, a space, a tab, and `}`.
static const struct d_tests_arg_command_named REFUSED_NAMES[] =
{
    { "S01", "-bad" },
    { "S02", "a=b"  },
    { "S03", "=ab"  },
    { "S04", "a b"  },
    { "S05", "a\tb" },
    { "S06", "x}"   }
};

// REFUSED_COUNT
//   constant: how many names REFUSED_NAMES holds.
static const size_t REFUSED_COUNT =
    sizeof(REFUSED_NAMES) / sizeof(REFUSED_NAMES[0]);

// CHECKED_NAMES
//   constant: the names CHECKS asks the subcommand `Sub_Cmd`, with the alias
// `al_ias`, about: its name as given, in lower case, without the
// underscore, and both; the alias as given, in upper case, and without the
// underscore; prefixes of the name and of the alias; and neither.
static const char* const CHECKED_NAMES[] =
{
    "Sub_Cmd", "sub_cmd", "SubCmd", "subcmd", "al_ias", "AL_IAS", "alias",
    "Sub", "al", "x"
};

// CHECKED_COUNT
//   constant: how many names CHECKED_NAMES holds.
static const size_t CHECKED_COUNT =
    sizeof(CHECKED_NAMES) / sizeof(CHECKED_NAMES[0]);

// d_tests_arg_command_check_setting
//   enum: the settings CHECKS asks under, its lines in order: none,
// ignore_case, ignore_underscore, both, and prefix matching; and how many
// there are.
enum d_tests_arg_command_check_setting
{
    CHECK_PLAIN      = 0,
    CHECK_CASE       = 1,
    CHECK_UNDERSCORE = 2,
    CHECK_BOTH       = 3,
    CHECK_PREFIX     = 4,
    CHECK_SETTINGS   = 5
};

// d_tests_arg_command_switch
//   struct: one of a command's switches as SETTINGS writes it: its name in
// the reference's settings(), and its D_ARG_COMMAND_* flag.
struct d_tests_arg_command_switch
{
    const char* name;
    uint32_t    flag;
};

// SWITCHES
//   constant: the switches SETTINGS writes, in the reference's order.
static const struct d_tests_arg_command_switch SWITCHES[] =
{
    { "ic",      D_ARG_COMMAND_IGNORE_CASE                 },
    { "iu",      D_ARG_COMMAND_IGNORE_UNDERSCORE           },
    { "fall",    D_ARG_COMMAND_FALLTHROUGH                 },
    { "subfall", D_ARG_COMMAND_SUBCOMMAND_FALLTHROUGH      },
    { "win",     D_ARG_COMMAND_WINDOWS_STYLE               },
    { "end",     D_ARG_COMMAND_POSITIONALS_AT_END          },
    { "conf",    D_ARG_COMMAND_CONFIGURABLE                },
    { "vpos",    D_ARG_COMMAND_VALIDATE_POSITIONALS        },
    { "vopt",    D_ARG_COMMAND_VALIDATE_OPTIONAL_ARGUMENTS },
    { "imm",     D_ARG_COMMAND_IMMEDIATE_CALLBACK          },
    { "pre",     D_ARG_COMMAND_PREFIX_MATCHING             },
    { "nonstd",  D_ARG_COMMAND_NON_STANDARD_NAMES          },
    { "req",     D_ARG_COMMAND_REQUIRED                    },
    { "dis",     D_ARG_COMMAND_DISABLED                    },
    { "silent",  D_ARG_COMMAND_SILENT                      }
};

// SWITCH_COUNT
//   constant: how many switches SWITCHES holds.
static const size_t SWITCH_COUNT = sizeof(SWITCHES) / sizeof(SWITCHES[0]);

// RELATION_ARGS
//   constant: RELATION_PARSES' parses, in one parser: s1, which excludes s2,
// with s2, in either order; s3, which needs s2, alone and with it; s4,
// which needs `--o`, alone and with it; s2, which excludes `--o`, with it;
// and s1 alone.
static const struct d_tests_arg_command_args RELATION_ARGS[] =
{
    { { "s1", "s2", NULL } },
    { { "s2", "s1", NULL } },
    { { "s3", NULL } },
    { { "s3", "s2", NULL } },
    { { "s4", NULL } },
    { { "--o", "v", "s4", NULL } },
    { { "--o", "v", "s2", NULL } },
    { { "s1", NULL } }
};

// RELATION_COUNT
//   constant: how many parses RELATION_ARGS holds.
static const size_t RELATION_COUNT =
    sizeof(RELATION_ARGS) / sizeof(RELATION_ARGS[0]);

// LIFECYCLE_ARGS
//   constant: LIFECYCLE's parses: the deprecated options given; the retired
// option given two values; and the option retired by name, alone and with
// a value.
static const struct d_tests_arg_command_args LIFECYCLE_ARGS[] =
{
    { { "--old", "v", "--older", "w", NULL } },
    { { "--ret", "a", "b", NULL } },
    { { "--gone", NULL } },
    { { "--gone", "x", NULL } }
};

// DEPRECATED_NAMED_ARGS
//   constant: DEPRECATED_NAMED's parse: both options deprecated by name,
// each given a value.
static const struct d_tests_arg_command_args DEPRECATED_NAMED_ARGS[] =
{
    { { "--named", "v", "-s", "w", NULL } }
};

// TRIGGER_ARGS
//   constant: TRIGGERS' parses, in one parser: a, which t enables, alone,
// after t, and before it; b, which s disables, alone and after s.
static const struct d_tests_arg_command_args TRIGGER_ARGS[] =
{
    { { "a", NULL } },
    { { "t", "a", NULL } },
    { { "b", NULL } },
    { { "s", "b", NULL } },
    { { "a", "t", NULL } }
};

// TRIGGER_COUNT
//   constant: how many parses TRIGGER_ARGS holds.
static const size_t TRIGGER_COUNT =
    sizeof(TRIGGER_ARGS) / sizeof(TRIGGER_ARGS[0]);

// CALLBACK_ARGS
//   constant: CALLBACKS' parses, in one parser: every subcommand, two of
// them the other way round, and none.
static const struct d_tests_arg_command_args CALLBACK_ARGS[] =
{
    { { "s", "t", "u", "p", NULL } },
    { { "p", "s", NULL } },
    { { NULL } }
};

// CALLBACK_COUNT
//   constant: how many parses CALLBACK_ARGS holds.
static const size_t CALLBACK_COUNT =
    sizeof(CALLBACK_ARGS) / sizeof(CALLBACK_ARGS[0]);

// PARSED_ARGS
//   constant: PARSED's parses: options and extras of the root, its group,
// s and t below s, an extra after `--`; then u and s twice.
static const struct d_tests_arg_command_args PARSED_ARGS[] =
{
    { { "--a", "1", "x1", "--g", "2", "s", "--x", "3", "y1", "t", "--y",
        "z1", "--", "z2", NULL } },
    { { "u", "s", "s", NULL } }
};

// SWEEP_OPTION_ARGS
//   constant: the parses around an option's removal: one that uses it, in
// its subcommand, then one that uses everything that referred to it.
static const struct d_tests_arg_command_args SWEEP_OPTION_ARGS[] =
{
    { { "s", "--o", "v", NULL } },
    { { "--a", "x", "s2", "--p", "y", NULL } }
};

// SWEEP_COMMAND_ARGS
//   constant: the parses around a subcommand's removal: one that uses it and
// the option below it, then one that uses the option that needed it.
static const struct d_tests_arg_command_args SWEEP_COMMAND_ARGS[] =
{
    { { "s", "t", "--inner", "v", NULL } },
    { { "--a", "x", NULL } }
};

// SWEEP_VERSION_ARGS
//   constant: the parse after the version flags are removed: their name.
static const char* const SWEEP_VERSION_ARGS[] =
{
    "-V", NULL
};

// KEPT_ARGS
//   constant: the parse after a move is refused: the subcommand, where it
// stayed.
static const char* const KEPT_ARGS[] =
{
    "s", "one", NULL
};

// TRIGGERED_ARGS
//   constant: the parses around a trigger's removals: b, which t enables,
// after t once a is removed; then b alone once t is removed.
static const struct d_tests_arg_command_args TRIGGERED_ARGS[] =
{
    { { "t", "b", NULL } },
    { { "b", NULL } }
};

// MOVED_ARGS
//   constant: the parse after an option is moved and linked by name: the
// option alone.
static const char* const MOVED_ARGS[] =
{
    "--a", "x", NULL
};

// ADD
//   constant: CLI11's transcript of ADD_CASES.
static const char ADD[] =
    "A01: OptionAlreadyAdded: cannot add '--alpha': long name '--alpha' "
    "already used by '--alpha'\n"
    "A02: OptionAlreadyAdded: cannot add '-a,--other': short name '-a' already "
    "used by '--alpha'\n"
    "A03: OptionAlreadyAdded: cannot add 'pos': name 'pos' already used by "
    "'pos'\n"
    "A04: OptionAlreadyAdded: cannot add 'file': long name '--file' already "
    "used by '--file'\n"
    "A05: OptionAlreadyAdded: cannot add 'f': short name '-f' already used by "
    "'-f'\n"
    "A06: OptionAlreadyAdded: unable to disambiguate with existing option: "
    "--pos is already added\n"
    "A07: OptionAlreadyAdded: added option matches existing option: --beta is "
    "already added\n"
    "A08: OptionAlreadyAdded: added option matches existing option: -c is "
    "already added\n"
    "A09: OptionAlreadyAdded: added option matches existing positional option: "
    "d is already added\n"
    "A10: ok\n"
    "A11: OptionAlreadyAdded: added option interferes with existing non "
    "standard option: ab is already added\n"
    "A12: OptionAlreadyAdded: added option interferes with existing short "
    "option: ab is already added\n"
    "A13: OptionAlreadyAdded: cannot add '--h1': long name '--h1' already used "
    "by ''\n"
    "A14: OptionAlreadyAdded: cannot add '--gamma': long name '--gamma' "
    "already used by '--Gamma'\n"
    "A15: ok\n"
    "A16: ok\n"
    "A17: OptionAlreadyAdded: cannot add 'y': short name '-y' already used by "
    "'--x'\n"
    "A18: OptionAlreadyAdded: added option matches existing option: --deep is "
    "already added\n";

// LISTED
//   constant: CLI11's listings of options by get_options.
static const char LISTED[] =
    "L01: {-h,--help --a --b --c --d -h,--help --e}\n"
    "L01q: {--d}\n"
    "L02: {-h,--help --b,--bb --a}\n"
    "L02t: {-h,--help --c}\n";

// FLAGS
//   constant: CLI11's transcript of FLAG_CASES.
static const char FLAGS[] =
    "F01: ok\n"
    "F01o: [--f1{7}] desc='' conf=1 prio=5 pol=1 req=0 def='' force=0 "
    "group='OPTIONS' type='' capture=0 for_default=0\n"
    "F01n: {-h,--help --f1{7}}\n"
    "F02: ok\n"
    "F02o: [-g,--gee{3},--no-gee{false}] desc='' conf=1 prio=5 pol=1 req=0 "
    "def='' force=0 group='OPTIONS' type='' capture=0 for_default=0\n"
    "F02n: {-h,--help -g,--gee{3},--no-gee{false}}\n"
    "F03: IncorrectConstruction: fpos: Flags cannot be positional\n"
    "F03o: (null)\n"
    "F03n: {-h,--help}\n"
    "F04: IncorrectConstruction: y: Flags cannot be positional\n"
    "F04o: (null)\n"
    "F04n: {-h,--help}\n"
    "F05: ok\n"
    "F05o: [-n{0},--neg{false},--pos] desc='' conf=1 prio=5 pol=1 req=0 def='' "
    "force=0 group='OPTIONS' type='' capture=0 for_default=0\n"
    "F05n: {-h,--help -n{0},--neg{false},--pos}\n"
    "F06: OptionAlreadyAdded: cannot add '--dup': long name '--dup' already "
    "used by '--dup'\n"
    "F06o: (null)\n"
    "F06n: {-h,--help --dup}\n"
    "F07: ok\n"
    "F07o: [--cb] desc='cbd' conf=1 prio=5 pol=1 req=0 def='' force=0 "
    "group='OPTIONS' type='' capture=0 for_default=0\n"
    "F07n: {-h,--help --cb}\n"
    "F08: ok\n"
    "F08o: [--fn] desc='fnd' conf=1 prio=5 pol=5 req=0 def='' force=0 "
    "group='OPTIONS' type='' capture=0 for_default=0\n"
    "F08n: {-h,--help --fn}\n"
    "F09: ok\n"
    "F09o: [-n,--count] desc='count' conf=1 prio=5 pol=5 req=0 def='0' force=1 "
    "group='OPTIONS' type='' capture=0 for_default=0\n"
    "F09n: {-h,--help -n,--count}\n"
    "F10: ok\n"
    "F10o: [--on] desc='' conf=1 prio=5 pol=1 req=0 def='' force=0 "
    "group='OPTIONS' type='' capture=1 for_default=0\n"
    "F10n: {-h,--help --on}\n"
    "F11: ok\n"
    "F11o: [--many] desc='' conf=1 prio=5 pol=4 req=0 def='' force=0 "
    "group='OPTIONS' type='' capture=0 for_default=1\n"
    "F11n: {-h,--help --many}\n"
    "F12: ok\n"
    "F12o: [--c] desc='' conf=1 prio=5 pol=1 req=0 def='' force=0 "
    "group='OPTIONS' type='' capture=1 for_default=0\n"
    "F12n: {-h,--help --c}\n"
    "F13: OptionAlreadyAdded: cannot add '--taken': long name '--taken' "
    "already used by '--taken'\n"
    "F13o: (null)\n"
    "F13n: {-h,--help --taken}\n";

// FLAG_BOUND
//   constant: CLI11's parses of BOUND_ARGS.
static const char FLAG_BOUND[] =
    "[-nnn --count --on --many --many=v --c=y]\n"
    "cmd app n=1 miss=[] subs=[] "
    "order=[--count|--count|--count|--count|--on|--many|--many|--c]\n"
    "opt count n=4 res=[true|true|true|true]\n"
    "opt on n=1 res=[true]\n"
    "opt many n=2 res=[true|v]\n"
    "opt c n=1 res=[y]\n"
    "values n=4 on=1 many=[true|v] c=y\n"
    "[--count=-2 --on=false --many=w --c=z -n]\n"
    "cmd app n=1 miss=[] subs=[] order=[--count|--on|--many|--c|--count]\n"
    "opt count n=2 res=[-2|true]\n"
    "opt on n=1 res=[false]\n"
    "opt many n=1 res=[w]\n"
    "opt c n=1 res=[z]\n"
    "values n=-1 on=0 many=[true|v|w] c=z\n"
    "[]\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt count n=0 res=[]\n"
    "opt on n=0 res=[]\n"
    "opt many n=0 res=[]\n"
    "opt c n=0 res=[]\n"
    "values n=0 on=0 many=[true|v|w] c=z\n"
    "[--count=x]\n"
    "ERR ConversionError 104: Could not convert: --count = x\n"
    "cmd app n=1 miss=[] subs=[] order=[--count]\n"
    "opt count n=1 res=[x]\n"
    "opt on n=0 res=[]\n"
    "opt many n=0 res=[]\n"
    "opt c n=0 res=[]\n"
    "values n=0 on=0 many=[true|v|w] c=z\n"
    "[--on=maybe]\n"
    "ERR ConversionError 104: Could not convert: --on = maybe\n"
    "cmd app n=1 miss=[] subs=[] order=[--on]\n"
    "opt count n=0 res=[]\n"
    "opt on n=1 res=[maybe]\n"
    "opt many n=0 res=[]\n"
    "opt c n=0 res=[]\n"
    "values n=0 on=0 many=[true|v|w] c=z\n";

// FLAG_SUMS
//   constant: CLI11's parses of SUM_ARGS.
static const char FLAG_SUMS[] =
    "[--cb]\n"
    "cb\n"
    "cmd app n=1 miss=[] subs=[] order=[--cb]\n"
    "[--cb=false]\n"
    "cmd app n=1 miss=[] subs=[] order=[--cb]\n"
    "[--cb=x]\n"
    "ERR ConversionError 104: Could not convert: --cb = x\n"
    "cmd app n=1 miss=[] subs=[] order=[--cb]\n"
    "[--fn]\n"
    "fn 1\n"
    "cmd app n=1 miss=[] subs=[] order=[--fn]\n"
    "[--fn -ff --fn=3]\n"
    "fn 6\n"
    "cmd app n=1 miss=[] subs=[] order=[--fn|--fn|--fn|--fn]\n"
    "[--fn=-2 --fn=false]\n"
    "fn -3\n"
    "cmd app n=1 miss=[] subs=[] order=[--fn|--fn]\n"
    "[--fn=x]\n"
    "fn 0\n"
    "cmd app n=1 miss=[] subs=[] order=[--fn]\n"
    "[--cb --fn=0]\n"
    "cb\n"
    "fn 0\n"
    "cmd app n=1 miss=[] subs=[] order=[--cb|--fn]\n"
    "[--fn=1e3]\n"
    "fn 1000\n"
    "cmd app n=1 miss=[] subs=[] order=[--fn]\n"
    "[--fn=0x10]\n"
    "fn 16\n"
    "cmd app n=1 miss=[] subs=[] order=[--fn]\n"
    "[--fn=010]\n"
    "fn 8\n"
    "cmd app n=1 miss=[] subs=[] order=[--fn]\n"
    "[--fn=08]\n"
    "fn 8\n"
    "cmd app n=1 miss=[] subs=[] order=[--fn]\n"
    "[--fn=0b11 --fn]\n"
    "fn 4\n"
    "cmd app n=1 miss=[] subs=[] order=[--fn|--fn]\n"
    "[]\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n";

// FLAG_SUMS_WIDE
//   constant: CLI11's parses of WIDE_ARGS.
static const char FLAG_SUMS_WIDE[] =
    "[--fn=5000000000]\n"
    "fn 5000000000\n"
    "cmd app n=1 miss=[] subs=[] order=[--fn]\n"
    "[--fn=9223372036854775807]\n"
    "fn 9223372036854775807\n"
    "cmd app n=1 miss=[] subs=[] order=[--fn]\n"
    "[--fn=-9223372036854775808]\n"
    "fn -9223372036854775808\n"
    "cmd app n=1 miss=[] subs=[] order=[--fn]\n"
    "[--fn=4294967296 -f]\n"
    "fn 4294967297\n"
    "cmd app n=1 miss=[] subs=[] order=[--fn|--fn]\n";

// NARROW_SUMS
//   constant: the parses of WIDE_ARGS where a `long` has 32 bits, as
// d_arg_command_add_flag_function gives its function the sums: each 0, a
// sum a `long` cannot hold, as for one that is no integer.
static const char NARROW_SUMS[] =
    "[--fn=5000000000]\n"
    "fn 0\n"
    "cmd app n=1 miss=[] subs=[] order=[--fn]\n"
    "[--fn=9223372036854775807]\n"
    "fn 0\n"
    "cmd app n=1 miss=[] subs=[] order=[--fn]\n"
    "[--fn=-9223372036854775808]\n"
    "fn 0\n"
    "cmd app n=1 miss=[] subs=[] order=[--fn]\n"
    "[--fn=4294967296 -f]\n"
    "fn 0\n"
    "cmd app n=1 miss=[] subs=[] order=[--fn|--fn]\n";

// SPECIAL
//   constant: CLI11's transcript of the help, version and configuration
// options.
static const char SPECIAL[] =
    "H01: [-h,--help] desc='Print this help message and exit' conf=0 prio=1 "
    "pol=1 req=0 def='' force=0 group='OPTIONS'\n"
    "H02: ok\n"
    "H02o: [--help-all] desc='Expand all' conf=0 prio=1 pol=1 req=0 def='' "
    "force=0 group='OPTIONS'\n"
    "H02n: {-h,--help --help-all}\n"
    "H03: ok\n"
    "H03o: (null)\n"
    "H03n: {}\n"
    "H04: OptionAlreadyAdded: cannot add '-z,--zz': short name '-z' already "
    "used by '-z'\n"
    "H04o: (null)\n"
    "H04n: {-z}\n"
    "H13: [-h,--help{yes}] desc='H' conf=0 prio=1 pol=1 req=0 def='' force=0 "
    "group='OPTIONS'\n"
    "H14: [-h,--help] desc='Print this help message and exit' conf=0 prio=1 "
    "pol=1 req=0 def='' force=0 group='OPTIONS'\n"
    "H14r: [-h,--help] desc='Print this help message and exit' conf=0 prio=1 "
    "pol=1 req=0 def='' force=0 group='OPTIONS'\n"
    "H16: OptionAlreadyAdded: cannot add '-V': short name '-V' already used by "
    "'-V'\n"
    "H16o: (null)\n"
    "H16n: {--help-all -V --cfg}\n"
    "H05: ok\n"
    "H05o: [-V,--version] desc='Display program version information and exit' "
    "conf=0 prio=1 pol=1 req=0 def='' force=0 group='OPTIONS'\n"
    "H05v: 1.2.3\n"
    "H06: ok\n"
    "H06o: (null)\n"
    "H06v: \n"
    "H06n: {-h,--help}\n"
    "H11: ok\n"
    "H11o: [--ver] desc='Show it' conf=0 prio=1 pol=1 req=0 def='' force=0 "
    "group='OPTIONS'\n"
    "H11v: 2.0\n"
    "H12: OptionAlreadyAdded: cannot add '--taken': long name '--taken' "
    "already used by '--taken'\n"
    "H12o: (null)\n"
    "H12v: \n"
    "H07: ok\n"
    "H07o: [--config] desc='Read an ini file' conf=0 prio=5 pol=6 req=1 "
    "def='def.ini' force=1 group='OPTIONS'\n"
    "H08: ok\n"
    "H08o: [--cfg] desc='Read an ini file' conf=0 prio=5 pol=6 req=0 def='' "
    "force=0 group='OPTIONS'\n"
    "H09: ok\n"
    "H09o: (null)\n"
    "H09n: {-h,--help}\n"
    "H10: ok\n"
    "H10o: [--cfg] desc='Read an ini file' conf=0 prio=5 pol=6 req=1 def='' "
    "force=0 group='OPTIONS'\n"
    "H15: OptionAlreadyAdded: cannot add '--used': long name '--used' already "
    "used by '--used'\n"
    "H15o: (null)\n"
    "H15n: {-h,--help --used}\n";

// SUBCOMMANDS
//   constant: CLI11's transcript of subcommands and option groups made,
// refused and found.
static const char SUBCOMMANDS[] =
    "S01: IncorrectConstruction: Subcommand name starts with invalid "
    "character, '!' and '-' and control characters\n"
    "S02: IncorrectConstruction: Subcommand name contains invalid character "
    "('='), all characters are allowed except'=',':','{','}', ' ', and control "
    "characters\n"
    "S03: ok\n"
    "S04: IncorrectConstruction: Subcommand name contains invalid character (' "
    "'), all characters are allowed except'=',':','{','}', ' ', and control "
    "characters\n"
    "S05: ok\n"
    "S06: ok\n"
    "S11: IncorrectConstruction: option group names may not contain newlines "
    "or null characters\n"
    "S07: OptionAlreadyAdded: subcommand name or alias matches existing "
    "subcommand: dup is already added\n"
    "S07s: <dup>\n"
    "S08: OptionAlreadyAdded: subcommand name or alias matches existing "
    "subcommand: sub is already added\n"
    "S10: ok\n"
    "S10s: <[SUBCOMMANDS] [SUBCOMMANDS]>\n"
    "S17: OptionAlreadyAdded: subcommand name or alias matches existing "
    "subcommand: one is already added\n"
    "S18: OptionAlreadyAdded: subcommand name or alias matches existing "
    "subcommand: two is already added\n"
    "S23: OptionAlreadyAdded: subcommand name or alias matches existing "
    "subcommand: xyz is already added\n"
    "S24: ok\n"
    "S24s: <dis dis>\n"
    "S25: ok\n"
    "S25b: OptionAlreadyAdded: subcommand name or alias matches existing "
    "subcommand: mid is already added\n"
    "S09: name='s' group='SUBCOMMANDS' footer='' fall=0 help=-h,--help "
    "help_all=-\n"
    "S09d: sd\n"
    "S12: name='' group='G' footer='' fall=0 help=- help_all=--help-all\n"
    "S13: name='' group='+G' footer='' fall=0 help=- help_all=-\n"
    "S14: name='' group='' footer='' fall=0 help=- help_all=-\n"
    "S15g: name='' group='G' footer='' fall=0 help=- help_all=-\n"
    "S15s: name='s' group='SUBCOMMANDS' footer='F' fall=1 help=-h,--help "
    "help_all=-\n"
    "S19: (null) alpha alpha alpine (null)\n"
    "S20: [G] [+H] [SUBCOMMANDS] (null) (null)\n"
    "S21: <[G] [+H] [SUBCOMMANDS] named>\n"
    "S22: found\n"
    "S26: One One\n"
    "S27: a b (null)\n";

// MOVES
//   constant: CLI11's transcript of options and subcommands moved into
// option groups.
static const char MOVES[] =
    "M01: OptionAlreadyAdded: cannot move help options is already added\n"
    "M02: OptionAlreadyAdded: cannot move config file options is already "
    "added\n"
    "M03: ok\n"
    "M03a: {-h,--help --b --a}\n"
    "M03g: {--a}\n"
    "M04: OptionNotFound: could not locate the given Option not found\n"
    "M05: OptionNotFound: the option is NULL not found\n"
    "M03r: 0 1 {-h,--help --b}\n"
    "M06: OptionAlreadyAdded: option was not located: --m is already added\n"
    "M07: ok\n"
    "M07b: ok\n"
    "M07a: {-h,--help -V -V}\n"
    "M07g: {-V}\n"
    "M07v: 2\n"
    "M10: OptionNotFound: could not locate the given Option not found\n"
    "M10g: {--o}{}\n"
    "M08: ok\n"
    "M08s: <s1 [G]><s2>\n"
    "M08l: 0 0 0\n"
    "M09: ok\n"
    "M09s: <[G] [H]><one><x>\n"
    "M11: ok\n"
    "M11s: <two one>\n"
    "M12: ok\n"
    "M12s: <[Inner]><one> found\n";

// REMOVAL
//   constant: CLI11's transcript of options and subcommands removed.
static const char REMOVAL[] =
    "R01: 1\n"
    "R01n: {-h,--help --a}\n"
    "R01x: 0 0\n"
    "R03: 0 {-h,--help --o}\n"
    "R03b: 1 {-h,--help}\n"
    "R05: 1 (null) {}\n"
    "R06: 1 1 (null) (null) {-h,--help}\n"
    "R02: 1\n"
    "R02s: <s1>\n"
    "R02r: 1\n"
    "R02t: <>\n"
    "R04: 0 <t>\n"
    "R04b: 1 <>\n"
    "R07: 0 0\n";

// NAMES
//   constant: CLI11's transcript of names and aliases given and refused.
static const char NAMES[] =
    "N01: OptionAlreadyAdded: a conflicts with existing subcommand names is "
    "already added\n"
    "N01n: b\n"
    "N02: ok\n"
    "N02n: prog2\n"
    "N03: OptionAlreadyAdded: alias already matches an existing subcommand: x "
    "is already added\n"
    "N03b: IncorrectConstruction: Aliases may not be empty or contain newlines "
    "or null characters\n"
    "N03c: IncorrectConstruction: Aliases may not be empty or contain newlines "
    "or null characters\n"
    "N03d: ok\n"
    "N03n: b, y\n"
    "N04: ok\n"
    "N04n: prog, a\n"
    "N05: OptionAlreadyAdded: ignore case would cause subcommand name "
    "conflicts: abc is already added\n"
    "N05v: 0\n"
    "N06: OptionAlreadyAdded: ignore case would cause subcommand name "
    "conflicts: PROG is already added\n"
    "N06v: 0\n"
    "N07: OptionAlreadyAdded: ignore underscore would cause subcommand name "
    "conflicts: ab is already added\n"
    "N07v: 0\n"
    "N07b: ok\n"
    "N21: s, x, y\n"
    "N21b: s\n"
    "N21c: ok\n"
    "N21d: s, x\n"
    "N22: ok\n"
    "N22n: [Option Group: Things]\n"
    "N22f: (null)\n";

// CHECKS
//   constant: CLI11's answers to the names of CHECKED_NAMES, a line for each
// setting.
static const char CHECKS[] =
    "C0: Sub_Cmd=1/1 sub_cmd=0/0 SubCmd=0/0 subcmd=0/0 al_ias=1/1 AL_IAS=0/0 "
    "alias=0/0 Sub=0/0 al=0/0 x=0/0\n"
    "C1: Sub_Cmd=1/1 sub_cmd=1/1 SubCmd=0/0 subcmd=0/0 al_ias=1/1 AL_IAS=1/1 "
    "alias=0/0 Sub=0/0 al=0/0 x=0/0\n"
    "C2: Sub_Cmd=1/1 sub_cmd=0/0 SubCmd=1/1 subcmd=0/0 al_ias=1/1 AL_IAS=0/0 "
    "alias=1/1 Sub=0/0 al=0/0 x=0/0\n"
    "C3: Sub_Cmd=1/1 sub_cmd=1/1 SubCmd=1/1 subcmd=1/1 al_ias=1/1 AL_IAS=1/1 "
    "alias=1/1 Sub=0/0 al=0/0 x=0/0\n"
    "C4: Sub_Cmd=1/1 sub_cmd=0/0 SubCmd=0/0 subcmd=0/0 al_ias=1/1 AL_IAS=0/0 "
    "alias=0/0 Sub=2/1 al=2/1 x=0/0\n";

// SETTINGS
//   constant: CLI11's transcript of a command's settings and what its
// subcommands and option groups take of them.
static const char SETTINGS[] =
    "N08: D- -E -E -- D-\n"
    "N09: 0,3 2,2 1,0 2,5 0,1 0,0 1,3\n"
    "N20: 3 5\n"
    "N20b: 0 5\n"
    "N24r: ic=1 iu=1 fall=1 subfall=0 win=1 end=1 conf=1 vpos=1 vopt=1 imm=1 "
    "pre=1 nonstd=1 req=1 dis=1 silent=1 start=D- extras=3 cfg=1 prefix=2 "
    "group='GRP' usage='U' footer='F' subs=1,4 opts=2,3\n"
    "N24s: ic=1 iu=1 fall=1 subfall=1 win=1 end=0 conf=1 vpos=1 vopt=1 imm=1 "
    "pre=1 nonstd=0 req=0 dis=0 silent=0 start=-- extras=3 cfg=1 prefix=2 "
    "group='GRP' usage='U' footer='F' subs=0,4 opts=0,0\n"
    "N24g: ic=1 iu=1 fall=0 subfall=1 win=1 end=0 conf=1 vpos=1 vopt=1 imm=1 "
    "pre=1 nonstd=0 req=0 dis=0 silent=0 start=-- extras=3 cfg=1 prefix=2 "
    "group='G' usage='U' footer='' subs=0,4 opts=0,0\n"
    "N25: ic=1 iu=1 fall=1 subfall=1 win=1 end=1 conf=1 vpos=1 vopt=1 imm=1 "
    "pre=1 nonstd=1 req=1 dis=1 silent=1 start=-- extras=5 cfg=1 prefix=2 "
    "group='SUBCOMMANDS' usage='' footer='' subs=0,0 opts=0,0\n"
    "N25b: ic=0 iu=0 fall=0 subfall=1 win=0 end=0 conf=0 vpos=0 vopt=0 imm=0 "
    "pre=0 nonstd=0 req=0 dis=0 silent=0 start=-- extras=0 cfg=1 prefix=0 "
    "group='SUBCOMMANDS' usage='' footer='' subs=0,0 opts=0,0\n";

// DEFAULTS
//   constant: CLI11's settings of a command as made, where an App does not
// start allowing Windows-style options.
static const char DEFAULTS[] =
    "N24d: ic=0 iu=0 fall=0 subfall=1 win=0 end=0 conf=0 vpos=0 vopt=0 imm=0 "
    "pre=0 nonstd=0 req=0 dis=0 silent=0 start=-- extras=0 cfg=1 prefix=0 "
    "group='SUBCOMMANDS' usage='' footer='' subs=0,0 opts=0,0\n";

// DEFAULTS_WINDOWS
//   constant: DEFAULTS where an App starts allowing Windows-style options,
// as CLI11's does on Windows, and the module's does where its setting for
// them says so.
static const char DEFAULTS_WINDOWS[] =
    "N24d: ic=0 iu=0 fall=0 subfall=1 win=1 end=0 conf=0 vpos=0 vopt=0 "
    "imm=0 pre=0 nonstd=0 req=0 dis=0 silent=0 start=-- extras=0 cfg=1 "
    "prefix=0 group='SUBCOMMANDS' usage='' footer='' subs=0,0 opts=0,0\n";

// RELATIONS
//   constant: CLI11's transcript of needs and excludes made and removed.
static const char RELATIONS[] =
    "N10: OptionNotFound: nullptr passed not found\n"
    "N10b: OptionNotFound: cannot self reference in excludes not found\n"
    "N10c: OptionNotFound: cannot self reference in needs not found\n"
    "N10d: OptionNotFound: nullptr passed not found\n"
    "N10e: OptionNotFound: nullptr passed not found\n"
    "N10f: OptionNotFound: nullptr passed not found\n"
    "N26: ok\n"
    "N26b: ok\n"
    "N26r: 1 0\n"
    "N27: ok\n"
    "N27b: ok\n"
    "N27r: 0 1 0\n"
    "N28: ok\n"
    "N28b: ok\n"
    "N28r: 1 0 1 0\n";

// RELATION_PARSES
//   constant: CLI11's parses of RELATION_ARGS.
static const char RELATION_PARSES[] =
    "[s1 s2]\n"
    "ERR ExcludesError 108: s1 excludes s2\n"
    "cmd app n=1 miss=[] subs=[s1|s2] order=[]\n"
    "opt o n=0 res=[]\n"
    "[s2 s1]\n"
    "ERR ExcludesError 108: s1 excludes s2\n"
    "cmd app n=1 miss=[] subs=[s2|s1] order=[]\n"
    "opt o n=0 res=[]\n"
    "[s3]\n"
    "ERR RequiresError 107: s3 requires s2\n"
    "cmd app n=1 miss=[] subs=[s3] order=[]\n"
    "opt o n=0 res=[]\n"
    "[s3 s2]\n"
    "cmd app n=1 miss=[] subs=[s3|s2] order=[]\n"
    "opt o n=0 res=[]\n"
    "[s4]\n"
    "ERR RequiresError 107: s4 requires --o\n"
    "cmd app n=1 miss=[] subs=[s4] order=[]\n"
    "opt o n=0 res=[]\n"
    "[--o v s4]\n"
    "cmd app n=1 miss=[] subs=[s4] order=[--o]\n"
    "opt o n=1 res=[v]\n"
    "[--o v s2]\n"
    "ERR ExcludesError 108: s2 excludes --o\n"
    "cmd app n=1 miss=[] subs=[s2] order=[--o]\n"
    "opt o n=1 res=[v]\n"
    "[s1]\n"
    "cmd app n=1 miss=[] subs=[s1] order=[]\n"
    "opt o n=0 res=[]\n";

// DISPLAY
//   constant: CLI11's display names and option groups' names.
static const char DISPLAY[] =
    "N11: s, x, y|s|[Option Group: G]|prog\n"
    "N12: [OPTIONS][B][A][]\n"
    "N12b: \n";

// USAGE_TEXT
//   constant: CLI11's usage, footer, version, description and group text.
static const char USAGE_TEXT[] =
    "N13: U|F\n"
    "N13b: UC\n"
    "U|FC\n"
    "F\n"
    "N13c: U|F\n"
    "N14: V9\n"
    "N14b: ''\n"
    "N29: second|G1\n"
    "N29b: ''\n";

// LIFECYCLE
//   constant: CLI11's transcript of options deprecated and retired.
static const char LIFECYCLE[] =
    "N15: [--old] desc='Old one DEPRECATED: please use '--new' instead' "
    "type=':DEPRECATED' def='' ts=1,1 exp=1,1 extra=0 req=0\n"
    "N15b: [--older] desc='' type=':DEPRECATED' def='' ts=1,1 exp=1,1 extra=0 "
    "req=0\n"
    "[--old v --older w]\n"
    "cmd app n=1 miss=[] subs=[] order=[--old|--older]\n"
    "opt old n=1 res=[v]\n"
    "opt older n=1 res=[w]\n"
    "out=\"--old is deprecated please use '--new' instead\\n--older is "
    "deprecated please use '' instead\\n\"\n"
    "N16: ok\n"
    "N16n: {-h,--help --keep -r,--ret}\n"
    "N16o: [-r,--ret] desc='option has been retired and has no effect' "
    "type='RETIRED' def='RETIRED' ts=1,1 exp=1,3 extra=1 req=0\n"
    "[--ret a b]\n"
    "cmd app n=1 miss=[] subs=[] order=[--ret|--ret]\n"
    "opt ret n=2 res=[a|b]\n"
    "out=\"WARNING --ret is retired and has no effect\\n\"\n"
    "N17: ok\n"
    "N17o: [--gone] desc='option has been retired and has no effect' "
    "type='RETIRED' def='RETIRED' ts=1,1 exp=0,1 extra=0 req=0\n"
    "[--gone]\n"
    "cmd app n=1 miss=[] subs=[] order=[--gone]\n"
    "opt gone n=1 res=[RETIRED]\n"
    "out=\"WARNING --gone is retired and has no effect\\n\"\n"
    "[--gone x]\n"
    "cmd app n=1 miss=[] subs=[] order=[--gone]\n"
    "opt gone n=1 res=[x]\n"
    "out=\"WARNING --gone is retired and has no effect\\n\"\n"
    "N18: BadNameString: Bad long name: f{3}\n"
    "N18n: {-h,--help --f{3}}\n"
    "N18b: BadNameString: Long names strings require 2 dashes -ab\n"
    "N18c: {-h,--help -ab}\n"
    "N18d: OptionAlreadyAdded: cannot add '-h': short name '-h' already used "
    "by '--help'\n"
    "N18e: {-h}\n";

// DEPRECATED_NAMED
//   constant: CLI11's transcript of options deprecated by the name a command
// finds them by -- deprecate_option(app, name, replacement) -- as the C++
// command suite's LIFECYCLE reference writes the same calls; then the
// module's own: a NULL command or name refused, nothing recorded.
static const char DEPRECATED_NAMED[] =
    "N15c: ok\n"
    "N15d: ok\n"
    "N15e: OptionNotFound: --nowhere not found\n"
    "N15f: [--named] desc=' DEPRECATED: please use '--newer' instead' "
    "type=':DEPRECATED' def='' ts=1,1 exp=1,1 extra=0 req=0\n"
    "N15g: [-s,--short] desc='' type=':DEPRECATED' def='' ts=1,1 exp=1,1 "
    "extra=0 req=0\n"
    "[--named v -s w]\n"
    "cmd app n=1 miss=[] subs=[] order=[--named|--short]\n"
    "opt named n=1 res=[v]\n"
    "opt short n=1 res=[w]\n"
    "out=\"--named is deprecated please use '--newer' instead\\n--short is "
    "deprecated please use '' instead\\n\"\n"
    "N15h: BadArgument BadArgument recorded=OK\n";

// TRIGGERS
//   constant: CLI11's transcript of triggers, and its parses of
// TRIGGER_ARGS.
static const char TRIGGERS[] =
    "N19: -E -E\n"
    "N30: D- -E\n"
    "[a]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: a\n"
    "cmd app n=1 miss=[a] subs=[] order=[]\n"
    "cmd a n=0 miss=[] subs=[] order=[]\n"
    "cmd b n=0 miss=[] subs=[] order=[]\n"
    "[t a]\n"
    "cmd app n=1 miss=[] subs=[t|a] order=[]\n"
    "cmd a n=1 miss=[] subs=[] order=[]\n"
    "cmd b n=0 miss=[] subs=[] order=[]\n"
    "[b]\n"
    "cmd app n=1 miss=[] subs=[b] order=[]\n"
    "cmd a n=0 miss=[] subs=[] order=[]\n"
    "cmd b n=1 miss=[] subs=[] order=[]\n"
    "[s b]\n"
    "ERR ExtrasError 109: s: The following argument was not expected: b\n"
    "cmd app n=1 miss=[] subs=[s] order=[]\n"
    "cmd a n=0 miss=[] subs=[] order=[]\n"
    "cmd b n=0 miss=[] subs=[] order=[]\n"
    "[a t]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: a\n"
    "cmd app n=1 miss=[a] subs=[t] order=[]\n"
    "cmd a n=0 miss=[] subs=[] order=[]\n"
    "cmd b n=0 miss=[] subs=[] order=[]\n";

// CALLBACKS
//   constant: CLI11's parses of CALLBACK_ARGS.
static const char CALLBACKS[] =
    "[s t u p]\n"
    "s preparse 3\n"
    "s callback\n"
    "t callback\n"
    "u complete\n"
    "app complete\n"
    "t final\n"
    "u final\n"
    "p pre\n"
    "p callback\n"
    "app callback\n"
    "cmd app n=1 miss=[] subs=[s|t|u|p] order=[]\n"
    "[p s]\n"
    "s preparse 0\n"
    "s callback\n"
    "app complete\n"
    "p pre\n"
    "p callback\n"
    "app callback\n"
    "cmd app n=1 miss=[] subs=[p|s] order=[]\n"
    "[]\n"
    "app complete\n"
    "app callback\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n";

// PARSED
//   constant: CLI11's parses of PARSED_ARGS, with what they leave to ask
// about.
static const char PARSED[] =
    "[--a 1 x1 --g 2 s --x 3 y1 t --y z1 -- z2]\n"
    "cmd app n=1 miss=[x1] subs=[s] order=[--a]\n"
    "cmd s n=1 miss=[y1|z2] subs=[t] order=[--x]\n"
    "cmd t n=1 miss=[z1] subs=[] order=[--y]\n"
    "count app=1 s=1 t=1 u=0 G=1 all app=7 s=4 t=2 G=1\n"
    "got app.s=1 app.t=0 app.u=0 s.t=1 name s=1 t=0 nope=0\n"
    "remaining app=[x1] all=[x1|y1|z2|z1] s=[y1|z2|z1] size app=1 all=4 s=3\n"
    "[u s s]\n"
    "cmd app n=1 miss=[] subs=[u|s] order=[]\n"
    "cmd s n=2 miss=[] subs=[] order=[]\n"
    "cmd t n=0 miss=[] subs=[] order=[]\n"
    "count app=1 s=2 t=0 u=1 G=1 all app=4 s=2 t=0 G=0\n"
    "got app.s=1 app.t=0 app.u=1 s.t=0 name s=1 t=0 nope=0\n"
    "remaining app=[] all=[] s=[] size app=0 all=0 s=0\n"
    "cleared s\n"
    "count app=1 s=0 t=0 u=1 G=1 all app=2 s=0 t=0 G=0\n"
    "got app.s=0 app.t=0 app.u=1 s.t=0 name s=0 t=0 nope=0\n"
    "remaining app=[] all=[] s=[] size app=0 all=0 s=0\n"
    "cleared app\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "cmd s n=0 miss=[] subs=[] order=[]\n"
    "cmd t n=0 miss=[] subs=[] order=[]\n"
    "count app=0 s=0 t=0 u=0 G=0 all app=0 s=0 t=0 G=0\n"
    "got app.s=0 app.t=0 app.u=0 s.t=0 name s=0 t=0 nope=0\n"
    "remaining app=[] all=[] s=[] size app=0 all=0 s=0\n";

// DEPARTED_SUMS
//   constant: the parses of UNREAD_ARGS, each sum 0 to the function, as the
// module reads a value that does not convert. CLI11's add_flag_function
// hands on what its conversion wrote before it failed -- the integer
// strtoll read at the front of the sum -- so its function is given 1, 3,
// 12 and 3, and, for the totals past 64 bits, which CLI11 writes as
// doubles of 16 digits (9.223372036854776e+18), 9 and -9.
static const char DEPARTED_SUMS[] =
    "[--fn=1.5]\n"
    "fn 0\n"
    "cmd app n=1 miss=[] subs=[] order=[--fn]\n"
    "[--fn=2.5 --fn=1]\n"
    "fn 0\n"
    "cmd app n=1 miss=[] subs=[] order=[--fn|--fn]\n"
    "[--fn=12abc]\n"
    "fn 0\n"
    "cmd app n=1 miss=[] subs=[] order=[--fn]\n"
    "[--fn=3 --fn=x]\n"
    "fn 0\n"
    "cmd app n=1 miss=[] subs=[] order=[--fn|--fn]\n"
    "[--fn=9223372036854775807 --fn]\n"
    "fn 0\n"
    "cmd app n=1 miss=[] subs=[] order=[--fn|--fn]\n"
    "[--fn=-9223372036854775808 --fn=-1]\n"
    "fn 0\n"
    "cmd app n=1 miss=[] subs=[] order=[--fn|--fn]\n";

// DEPARTED_DECIMAL
//   constant: the parses of DECIMAL_ARGS under the rule the module reads
// integers with unless a command or an option asks for C's: a leading zero
// is decimal, so `010` is ten, and octal needs `0o`. CLI11 reads every
// integer as strtoll does with base 0, which makes `010` eight; `08` is
// eight under either rule, CLI11 reading it as a double when strtoll
// stops at the `8`.
static const char DEPARTED_DECIMAL[] =
    "[--fn=010]\n"
    "fn 10\n"
    "cmd app n=1 miss=[] subs=[] order=[--fn]\n"
    "[--fn=08]\n"
    "fn 8\n"
    "cmd app n=1 miss=[] subs=[] order=[--fn]\n"
    "[--fn=0o10]\n"
    "fn 8\n"
    "cmd app n=1 miss=[] subs=[] order=[--fn]\n";

// d_tests_arg_command_version_step
//   struct: one step of DEPARTED_CONTEXTS' version functions: its label, the
// flag's names, which count is the context, and whether the subcommand is
// given the function, or no command.
struct d_tests_arg_command_version_step
{
    const char* label;
    const char* names;
    size_t      context;
    bool        command;
};

// VERSION_STEPS
//   constant: the steps of DEPARTED_CONTEXTS' version functions: given,
// given again, replaced, removed, refused a taken name, given to no
// command, and given to be released with the parser.
static const struct d_tests_arg_command_version_step VERSION_STEPS[] =
{
    { "version given",       "-V",      0u, true  },
    { "version given again", "-V",      0u, true  },
    { "version replaced",    "-V",      1u, true  },
    { "version removed",     "",        1u, true  },
    { "version refused",     "--taken", 0u, true  },
    { "version no command",  "-V",      0u, false },
    { "version kept",        "-V",      0u, true  }
};

// VERSION_STEP_COUNT
//   constant: how many steps VERSION_STEPS holds.
static const size_t VERSION_STEP_COUNT =
    sizeof(VERSION_STEPS) / sizeof(VERSION_STEPS[0]);

// DEPARTED_TRIGGERS
//   constant: a trigger enabling a and b: a removed, then t and b parsed --
// the trigger enables what is left of its list -- then the trigger itself
// removed, and b parsed alone, which stays disabled by default. CLI11's
// TriggerOn keeps its targets in a lambda that a removal does not reach,
// so its next use would enable the released subcommand.
static const char DEPARTED_TRIGGERS[] =
    "removed a 1\n"
    "[t b]\n"
    "cmd app n=1 miss=[] subs=[t|b] order=[]\n"
    "cmd b n=1 miss=[] subs=[] order=[]\n"
    "removed t 1\n"
    "[b]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: b\n"
    "cmd app n=1 miss=[b] subs=[] order=[]\n"
    "cmd b n=0 miss=[] subs=[] order=[]\n";

// DEPARTED_CONTEXTS
//   constant: the release counts of two contexts, `first` and `second`,
// through a command's final callback -- given, given again, replaced,
// removed, given to no command, a usage function given to no command,
// moved by immediate_callback and released with the parser -- and through
// a version function -- given, given again, replaced, removed, refused a
// taken name, given to no command, and released with the parser. CLI11
// keeps callbacks as std::function values, which own what they hold;
// here each context is released by the function given with it, once,
// when nothing keeps it any more.
static const char DEPARTED_CONTEXTS[] =
    "given first=0 second=0\n"
    "given again first=0 second=0\n"
    "replaced first=1 second=0\n"
    "removed first=2 second=1\n"
    "no command first=3 second=1\n"
    "no command for usage first=3 second=2\n"
    "moved final=0 complete=1\n"
    "version given first=3 second=2\n"
    "version given again first=3 second=2\n"
    "version replaced first=4 second=2\n"
    "version removed first=4 second=3\n"
    "version refused first=5 second=3\n"
    "version no command first=6 second=3\n"
    "version kept first=6 second=3\n"
    "parser released first=8 second=3\n";

// DEPARTED_RANGE
//   constant: counts past what negating an `int` gives, and values outside
// their enums: require_subcommand and require_option given the most
// negative `int` allow at most 2^31, where CLI11 negates it, which
// overflows; modes, policies, priorities, integer rules and option flags
// outside what they may be are ignored, as no enum CLI11 takes can hold
// them; and a NULL group name is the empty one.
static const char DEPARTED_RANGE[] =
    "counts subs=0,2147483648 opts=0,2147483648\n"
    "modes extras=0 cfg=1 prefix=0\n"
    "defaults policy=0 priority=5 rule=0 flag_like=0 required=1\n"
    "group ''\n";

// DEPARTED_SWEEP_OPTION
//   constant: an option removed from a subcommand after a parse that used
// it, which an option of the root needs and excludes, an option of another
// subcommand needs, the root needs and the other subcommand excludes: each
// link and the parse order forget it, and the next parse checks what is
// left. CLI11's remove_option drops only the links its own command's
// options hold, so every other would read the released option.
static const char DEPARTED_SWEEP_OPTION[] =
    "[s --o v]\n"
    "cmd app n=1 miss=[] subs=[s] order=[]\n"
    "removed 1\n"
    "--a needs=0 excludes=0, --p needs=0\n"
    "prog needs=0, s2 excludes=0, s order=0\n"
    "[--a x s2 --p y]\n"
    "cmd app n=1 miss=[] subs=[s2] order=[--a]\n";

// DEPARTED_SWEEP_COMMAND
//   constant: a subcommand removed after a parse that used it, with an
// option below it that an option of the root needs and excludes and the
// root needs, a subcommand below it that an option group needs, and the
// group excluding it: each link, and the subcommands the parse used,
// forget them. CLI11's remove_subcommand drops only the needs and excludes
// its own subcommands hold on the one removed -- the group's exclusion
// here -- so every other would read a released record.
static const char DEPARTED_SWEEP_COMMAND[] =
    "[s t --inner v]\n"
    "cmd app n=1 miss=[] subs=[s] order=[]\n"
    "removed 1\n"
    "--a needs=0 excludes=0, prog needs=0 parsed=0\n"
    "G needs=0 excludes=0\n"
    "[--a x]\n"
    "cmd app n=1 miss=[] subs=[] order=[--a]\n";

// DEPARTED_SWEEP_VERSION
//   constant: the version flag removed with remove_option, then another
// moved into an option group that is removed: neither leaves a version,
// and `-V` is no option. CLI11's remove_option forgets the help, help-all
// and configuration options it removes but not the version flag, whose
// pointer version() would then read.
static const char DEPARTED_SWEEP_VERSION[] =
    "removed 1 version=''\n"
    "moved and removed 1 version=''\n"
    "[-V]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: -V\n"
    "cmd app n=1 miss=[-V] subs=[] order=[]\n";

// DEPARTED_SWEEP_REPORTS
//   constant: the parser's reports of a refused definition and of a failed
// callback, which name the option or command they came from, after that
// option or command is removed: the report stays, without it. CLI11 throws
// its errors as exceptions and keeps no reports.
static const char DEPARTED_SWEEP_REPORTS[] =
    "refused OptionAlreadyAdded option=--ab command=prog\n"
    "removed OptionAlreadyAdded option=- command=prog\n"
    "refused OptionAlreadyAdded option=- command=b\n"
    "removed OptionAlreadyAdded option=- command=-\n"
    "failed ValidationError option=- command=c\n"
    "removed ValidationError option=- command=-\n";

// DEPARTED_KEPT
//   constant: a subcommand moved from below another into an option group of
// the root, where its name clashes: refused, it stays where it was, and is
// parsed there. CLI11 takes the subcommand from its parent before it checks
// the name and destroys it when the check throws, leaving the caller's
// pointer to it dangling.
static const char DEPARTED_KEPT[] =
    "moved: OptionAlreadyAdded: subcommand name or alias matches existing "
    "subcommand: one is already added\n"
    "after: <one s [H]><one><>\n"
    "[s one]\n"
    "cmd app n=1 miss=[] subs=[s] order=[]\n"
    "cmd s n=1 miss=[] subs=[one] order=[]\n";

// DEPARTED_APART
//   constant: the moves CLI11 takes that break its tree, each refused here
// without a record: a group into itself or into a group inside it, which
// CLI11 then holds only in a cycle of its own; the root and NULL, whose
// parent CLI11 reads; another parser's subcommand, which leaves that
// parser's links to it across two trees; and moves into a named
// subcommand, which no App offers. Then the links between two parsers'
// options and commands, which dangle when either parser goes, refused
// with a record.
static const char DEPARTED_APART[] =
    "G into G: BadArgument\n"
    "G into Inner: BadArgument\n"
    "prog into G: BadArgument\n"
    "Inner into s: BadArgument\n"
    "nothing into G: BadArgument\n"
    "--a into s: BadArgument\n"
    "q into G: BadArgument\n"
    "--a needs --q: IncorrectConstruction: options of different parsers cannot "
    "be linked\n"
    "--a excludes --q: IncorrectConstruction: options of different parsers "
    "cannot be linked\n"
    "s needs prog: IncorrectConstruction: commands of different parsers cannot "
    "be linked\n"
    "s excludes q: IncorrectConstruction: commands of different parsers cannot "
    "be linked\n"
    "s needs --q: IncorrectConstruction: options and commands of different "
    "parsers cannot be linked\n"
    "s excludes --q: IncorrectConstruction: options and commands of different "
    "parsers cannot be linked\n"
    "s triggers q: IncorrectConstruction: commands of different parsers cannot "
    "be linked\n"
    "left: --a 0 0, s 0 0 0 0, q 0\n"
    "after: <[G] s><[Inner]><q>\n";

// DEPARTED_LINKS
//   constant: names an option moved into an option group needs and
// excludes, looked up from the group, where both the group and its parent
// hold `--in`: the group's. CLI11 looks them up from the command the option
// was made in, which finds its own `--in` first.
static const char DEPARTED_LINKS[] =
    "moved: ok\n"
    "needs --in: ok\n"
    "  in [Option Group: G]\n"
    "excludes --in: ok\n"
    "  in [Option Group: G]\n"
    "needs --zz: IncorrectConstruction: Option --zz is not defined\n"
    "[--a x]\n"
    "ERR RequiresError 107: --a requires --in\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd G n=1 miss=[] subs=[] order=[--a]\n";

//==============================================================================
// HELPERS
//==============================================================================

/**
 * @brief Adds a flag's values to a d_tests_arg_command_texts, as CLI11's
 *        add_flag adds them to a std::vector<std::string>.
 *
 * @param[in]     _type     the descriptor; unused.
 * @param[in]     _values   the values.
 * @param[in]     _count    how many.
 * @param[in,out] _target   the d_tests_arg_command_texts, whose values are
 *                          kept.
 * @param[in]     _context  the conversion's context; unused.
 * @return D_ARG_OK; D_ARG_ERROR_OUT_OF_MEMORY when a value could not be
 *         copied.
 */
static int
d_tests_arg_command_to_texts(
    const struct d_arg_type*            _type,
    const struct d_pack_text*           _values,
    size_t                              _count,
    void*                               _target,
    const struct d_arg_convert_context* _context
)
{
    struct d_tests_arg_command_texts* const texts = _target;

    (void)_type;
    (void)_context;

    // each value, after the ones before it
    for (size_t index = 0u; index < _count; ++index)
    {
        // a bar between values
        if (texts->count > 0u)
        {
            (void)d_arg_buffer_append_char(&texts->joined,
                                           '|');
        }

        (void)d_arg_buffer_append_text(&texts->joined,
                                       _values[index]);
        texts->count += 1u;
    }

    return (texts->joined.failed != 0)
           ? D_ARG_ERROR_OUT_OF_MEMORY
           : D_ARG_OK;
}

// TEXTS_TYPE
//   constant: the descriptor of a d_tests_arg_command_texts, a
// std::vector<std::string> as CLI11's add_flag fills one: a container of
// text, of any number of values.
static const struct d_arg_type TEXTS_TYPE =
{
    "TEXTS",                                              // name
    d_tests_arg_command_to_texts,                         // convert
    NULL,                                                 // format
    NULL,                                                 // data
    sizeof(struct d_tests_arg_command_texts),             // size
    1,                                                    // type_size_min
    1,                                                    // type_size_max
    D_ARG_EXPECTED_UNLIMITED,                             // expected
    (D_ARG_TYPE_FLAG_CONTAINER | D_ARG_TYPE_FLAG_STRING)  // flags
};

/**
 * @brief Appends a definition's line as the reference's TRY writes it: the
 *        refusal the parser recorded, then forgotten, as catching CLI11's
 *        exception lets a program go on; or `ok`.
 *
 * @param[in,out] _out   the transcript.
 * @param[in,out] _run   the run whose parser made the definition, its
 *                       definition error cleared before the call.
 * @param[in]     _tag   the case's tag.
 * @param[in]     _done  whether the call did what it was asked: returned
 *                       what it made, moved, or removed.
 *
 * A call that did nothing and recorded nothing writes `refused unrecorded`,
 * a line no transcript expects.
 */
static void
d_tests_arg_command_tried(
    struct d_arg_buffer*    _out,
    struct d_tests_arg_run* _run,
    const char*             _tag,
    bool                    _done
)
{
    const struct d_arg_error* const error =
        d_arg_parser_definition_error(&_run->parser);

    // a refusal, as CLI11 throws it
    if (error->status != (int)D_ARG_OK)
    {
        (void)d_arg_buffer_printf(_out,
                                  "%s: %s: %s\n",
                                  _tag,
                                  d_arg_status_name(error->status),
                                  d_arg_error_message(error));
        d_arg_parser_clear_definition_error(&_run->parser);

        return;
    }

    (void)d_arg_buffer_printf(_out,
                              "%s: %s\n",
                              _tag,
                              (_done) ? "ok" : "refused unrecorded");

    return;
}

/**
 * @brief Appends an option as the reference's opt_desc writes it: every
 *        name, then the description, whether it is configurable, the
 *        callback priority, the multi-option policy, whether it is
 *        required, the default text, whether its callback is forced, and
 *        the group; `(null)` for no option.
 *
 * @param[in,out] _out     the transcript.
 * @param[in]     _option  the option; may be NULL.
 */
static void
d_tests_arg_command_put_option(
    struct d_arg_buffer*       _out,
    const struct d_arg_option* _option
)
{
    // no option
    if (!_option)
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       "(null)");

        return;
    }

    (void)d_arg_buffer_append_char(_out,
                                   '[');
    (void)d_arg_option_name(_option,
                            D_ARG_NAME_ALL,
                            _out);
    (void)d_arg_buffer_printf(
        _out,
        "] desc='%s' conf=%d prio=%d pol=%d req=%d def='%s' force=%d "
        "group='%s'",
        d_arg_option_description(_option),
        (int)d_arg_option_has_flag(_option,
                                   D_ARG_OPTION_CONFIGURABLE),
        _option->callback_priority,
        _option->multi_option_policy,
        (int)d_arg_option_has_flag(_option,
                                   D_ARG_OPTION_REQUIRED),
        d_arg_option_default_text(_option),
        (int)d_arg_option_has_flag(_option,
                                   D_ARG_OPTION_FORCE_CALLBACK),
        d_arg_option_group(_option));

    return;
}

/**
 * @brief Appends a line `<tag>: <option>`, the option as
 *        d_tests_arg_command_put_option writes it.
 *
 * @param[in,out] _out     the transcript.
 * @param[in]     _tag     the line's tag.
 * @param[in]     _option  the option; may be NULL.
 */
static void
d_tests_arg_command_line_option(
    struct d_arg_buffer*       _out,
    const char*                _tag,
    const struct d_arg_option* _option
)
{
    (void)d_arg_buffer_printf(_out,
                              "%s: ",
                              _tag);
    d_tests_arg_command_put_option(_out,
                                   _option);
    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

// d_tests_arg_command_listing
//   struct: a listing of options being written: the transcript, and how
// many names it holds so far.
struct d_tests_arg_command_listing
{
    struct d_arg_buffer* out;
    size_t               count;
};

/**
 * @brief Appends one option of a listing, with every name, after a space
 *        when it is not the first; d_arg_command_each_option's visitor.
 *
 * @param[in] _option   the option.
 * @param[in] _context  the d_tests_arg_command_listing.
 * @return the status of writing the names.
 */
static int
d_tests_arg_command_list_option(
    struct d_arg_option* _option,
    void*                _context
)
{
    struct d_tests_arg_command_listing* const listing = _context;

    // a space between names
    if (listing->count > 0u)
    {
        (void)d_arg_buffer_append_char(listing->out,
                                       ' ');
    }

    listing->count += 1u;

    return d_arg_option_name(_option,
                             D_ARG_NAME_ALL,
                             listing->out);
}

/**
 * @brief Appends the options a command lists, as the reference's names()
 *        writes CLI11's get_options: `{<names> ...}`.
 *
 * @param[in,out] _out      the transcript.
 * @param[in]     _command  the command.
 */
static void
d_tests_arg_command_put_listing(
    struct d_arg_buffer*  _out,
    struct d_arg_command* _command
)
{
    struct d_tests_arg_command_listing listing = { _out, 0u };

    (void)d_arg_buffer_append_char(_out,
                                   '{');
    (void)d_arg_command_each_option(_command,
                                    d_tests_arg_command_list_option,
                                    &listing);
    (void)d_arg_buffer_append_char(_out,
                                   '}');

    return;
}

/**
 * @brief Appends a line `<tag>: {<names> ...}`, the options a command
 *        lists.
 *
 * @param[in,out] _out      the transcript.
 * @param[in]     _tag      the line's tag.
 * @param[in]     _command  the command.
 */
static void
d_tests_arg_command_line_listing(
    struct d_arg_buffer*  _out,
    const char*           _tag,
    struct d_arg_command* _command
)
{
    (void)d_arg_buffer_printf(_out,
                              "%s: ",
                              _tag);
    d_tests_arg_command_put_listing(_out,
                                    _command);
    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

/**
 * @brief Appends a command's own subcommands, as the reference's subs()
 *        writes them: `<name ...>`, an option group as its group in
 *        brackets.
 *
 * @param[in,out] _out      the transcript.
 * @param[in]     _command  the command.
 */
static void
d_tests_arg_command_put_subcommands(
    struct d_arg_buffer*        _out,
    const struct d_arg_command* _command
)
{
    (void)d_arg_buffer_append_char(_out,
                                   '<');

    // each subcommand, until the index finds none
    for (size_t index = 0u;
         d_arg_command_subcommand_at(_command,
                                     index) != NULL;
         ++index)
    {
        const struct d_arg_command* const sub =
            d_arg_command_subcommand_at(_command,
                                        index);
        const bool                        group =
            d_arg_command_is_option_group(sub);

        (void)d_arg_buffer_printf(_out,
                                  "%s%s%s%s",
                                  (index > 0u) ? " " : "",
                                  (group) ? "[" : "",
                                  (group)
                                  ? d_arg_command_group(sub)
                                  : d_arg_command_name(sub),
                                  (group) ? "]" : "");
    }

    (void)d_arg_buffer_append_char(_out,
                                   '>');

    return;
}

/**
 * @brief Appends a line `<tag>: <name ...>`, a command's own subcommands.
 *
 * @param[in,out] _out      the transcript.
 * @param[in]     _tag      the line's tag.
 * @param[in]     _command  the command.
 */
static void
d_tests_arg_command_line_subcommands(
    struct d_arg_buffer*        _out,
    const char*                 _tag,
    const struct d_arg_command* _command
)
{
    (void)d_arg_buffer_printf(_out,
                              "%s: ",
                              _tag);
    d_tests_arg_command_put_subcommands(_out,
                                        _command);
    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

/**
 * @brief Appends an option's every name, or `-` for none, as the
 *        reference writes a command's help flags.
 *
 * @param[in,out] _out     the transcript.
 * @param[in]     _option  the option; may be NULL.
 */
static void
d_tests_arg_command_put_name(
    struct d_arg_buffer*       _out,
    const struct d_arg_option* _option
)
{
    // no option
    if (!_option)
    {
        (void)d_arg_buffer_append_char(_out,
                                       '-');

        return;
    }

    (void)d_arg_option_name(_option,
                            D_ARG_NAME_ALL,
                            _out);

    return;
}

/**
 * @brief Appends a line as the reference's app_desc writes a command:
 *        `<tag>: name='..' group='..' footer='..' fall=.. help=..
 *        help_all=..`.
 *
 * @param[in,out] _out      the transcript.
 * @param[in]     _tag      the line's tag.
 * @param[in]     _command  the command.
 */
static void
d_tests_arg_command_line_command(
    struct d_arg_buffer*        _out,
    const char*                 _tag,
    const struct d_arg_command* _command
)
{
    (void)d_arg_buffer_printf(_out,
                              "%s: name='%s' group='%s' footer='",
                              _tag,
                              d_arg_command_name(_command),
                              d_arg_command_group(_command));
    (void)d_arg_command_footer(_command,
                               d_arg_buffer_sink(_out));
    (void)d_arg_buffer_printf(_out,
                              "' fall=%d help=",
                              (int)d_arg_command_has_flag(
                                       _command,
                                       D_ARG_COMMAND_FALLTHROUGH));
    d_tests_arg_command_put_name(_out,
                                 _command->help);
    (void)d_arg_buffer_append_cstr(_out,
                                   " help_all=");
    d_tests_arg_command_put_name(_out,
                                 _command->help_all);
    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

/**
 * @brief Appends a line `<tag>: <version>`, the text a command's version
 *        flag shows, as CLI11's version() gives it.
 *
 * @param[in,out] _out      the transcript.
 * @param[in]     _tag      the line's tag.
 * @param[in]     _command  the command.
 */
static void
d_tests_arg_command_line_version(
    struct d_arg_buffer*        _out,
    const char*                 _tag,
    const struct d_arg_command* _command
)
{
    (void)d_arg_buffer_printf(_out,
                              "%s: ",
                              _tag);
    (void)d_arg_command_version(_command,
                                d_arg_buffer_sink(_out));
    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

// d_tests_arg_command_case
//   struct: what one case of a transcript works with: the transcript, and a
// run of the case's own, with its root.
struct d_tests_arg_command_case
{
    struct d_arg_buffer*    out;
    struct d_tests_arg_run* run;
    struct d_arg_command*   app;
};

// d_tests_arg_command_case_fn
//   type: one case of a transcript, which makes its definitions on the
// case's root and appends its lines.
typedef void (*d_tests_arg_command_case_fn)(
    struct d_tests_arg_command_case* _case);

/**
 * @brief Runs cases of a transcript, each in a parser of its own; what a
 *        case's run wrote -- the reports of its parses, and its notes --
 *        follows the lines the case wrote itself.
 *
 * @param[in,out] _out    the transcript.
 * @param[in]     _cases  the cases, in order.
 * @param[in]     _count  how many.
 * @return whether every run held what it was given: each watch, each note.
 */
static bool
d_tests_arg_command_run_cases(
    struct d_arg_buffer*               _out,
    const d_tests_arg_command_case_fn* _cases,
    size_t                             _count
)
{
    bool held = true;

    // each case, in a parser of its own
    for (size_t index = 0u; index < _count; ++index)
    {
        struct d_tests_arg_run          run  = { 0 };
        struct d_tests_arg_command_case each = { _out,
                                                 &run,
                                                 d_tests_arg_begin(&run) };

        _cases[index](&each);
        (void)d_arg_buffer_append_text(_out,
                                       d_arg_buffer_text(&run.transcript));
        held = ( (held)                        &&
                 (!run.failed)                 &&
                 (run.transcript.failed == 0) );
        d_tests_arg_end(&run);
    }

    return held;
}

/**
 * @brief Runs a transcript's cases, each in a parser of its own, and
 *        compares the transcript with the one expected.
 *
 * @param[in] _name      the transcript's name, for a mismatch.
 * @param[in] _cases     the cases, in order.
 * @param[in] _count     how many.
 * @param[in] _expected  the transcript expected.
 * @return whether the transcript was as expected.
 */
static bool
d_tests_arg_command_cases(
    const char*                        _name,
    const d_tests_arg_command_case_fn* _cases,
    size_t                             _count,
    const char*                        _expected
)
{
    struct d_arg_buffer out  = D_ARG_BUFFER_INIT;
    const bool          held = d_tests_arg_command_run_cases(&out,
                                                             _cases,
                                                             _count);
    const bool          same = d_tests_arg_same(_name,
                                                d_arg_buffer_text(&out),
                                                _expected);

    d_arg_buffer_release(&out);

    return ( (held) &&
             (same) );
}

/**
 * @brief Notes a parse's arguments in brackets, as the reference's run()
 *        writes them, then parses them and adds the report.
 *
 * @param[in,out] _run   the run.
 * @param[in]     _args  the arguments, ending in NULL.
 */
static void
d_tests_arg_command_parse_listed(
    struct d_tests_arg_run* _run,
    const char* const*      _args
)
{
    struct d_arg_buffer line  = D_ARG_BUFFER_INIT;
    size_t              count = 0u;

    // each argument, a space between them
    for (; _args[count] != NULL; ++count)
    {
        (void)d_arg_buffer_printf(&line,
                                  "%s%s",
                                  (count > 0u) ? " " : "",
                                  _args[count]);
    }

    const struct d_pack_text text = d_arg_buffer_text(&line);

    d_tests_arg_note(_run,
                     "[%.*s]",
                     (int)text.length,
                     (text.data != NULL) ? text.data : "");
    d_arg_buffer_release(&line);
    (void)d_tests_arg_parse(_run,
                            _args,
                            count);

    return;
}

//==============================================================================
// 1.  OPTIONS
//==============================================================================

/**
 * @brief Sets what a case of ADD sets on the root before anything is made:
 *        non-standard option names, or ignore_case in its option defaults.
 *
 * @param[in,out] _app      the root.
 * @param[in]     _setting  the case's setting.
 */
static void
d_tests_arg_command_set_up(
    struct d_arg_command* _app,
    int                   _setting
)
{
    // short names of more than one character
    if (_setting == SET_NON_STANDARD)
    {
        d_arg_command_set_non_standard_names(_app,
                                             true);
    }

    // options that ignore case from the start
    if (_setting == SET_IGNORING)
    {
        d_arg_option_defaults_set_flag(d_arg_command_option_defaults(_app),
                                       D_ARG_OPTION_IGNORE_CASE,
                                       true);
    }

    return;
}

/**
 * @brief Makes the commands a case of ADD needs beside the root: G when an
 *        option goes in it or in G2, G2 inside G, and s for the
 *        subcommand.
 *
 * @param[in,out] _app     the root.
 * @param[in]     _case    the case.
 * @param[out]    _places  receives the commands by place, PLACE_COUNT of
 *                         them; NULL for one the case does not need.
 */
static void
d_tests_arg_command_places(
    struct d_arg_command*                      _app,
    const struct d_tests_arg_command_add_case* _case,
    struct d_arg_command**                     _places
)
{
    const bool grouped = ( (_case->holder == IN_GROUP)  ||
                           (_case->target == IN_GROUP)  ||
                           (_case->target == IN_NESTED) );

    _places[IN_ROOT]   = _app;
    _places[IN_GROUP]  = NULL;
    _places[IN_NESTED] = NULL;
    _places[IN_SUB]    = NULL;

    // G, when an option goes in it or in G2
    if (grouped)
    {
        _places[IN_GROUP] = d_arg_command_add_option_group(_app,
                                                           "G",
                                                           NULL);
    }

    // G2, inside G
    if (_case->target == IN_NESTED)
    {
        _places[IN_NESTED] =
            d_arg_command_add_option_group(_places[IN_GROUP],
                                           "G2",
                                           NULL);
    }

    // the subcommand
    if (_case->target == IN_SUB)
    {
        _places[IN_SUB] = d_arg_command_add_subcommand(_app,
                                                       "s",
                                                       NULL);
    }

    return;
}

/**
 * @brief Appends one case of ADD, in a parser of its own: the root set up,
 *        the commands the case needs made, the first option made and set,
 *        then the line of the option tried.
 *
 * @param[in,out] _out   the transcript.
 * @param[in]     _case  the case.
 */
static void
d_tests_arg_command_added(
    struct d_arg_buffer*                       _out,
    const struct d_tests_arg_command_add_case* _case
)
{
    struct d_tests_arg_run      run                 = { 0 };
    struct d_arg_command* const app                 = d_tests_arg_begin(&run);
    struct d_arg_command*       places[PLACE_COUNT] = { NULL };

    d_tests_arg_command_set_up(app,
                               _case->setting);
    d_tests_arg_command_places(app,
                               _case,
                               places);

    struct d_arg_option* const first =
        d_arg_command_add_option(places[_case->holder],
                                 _case->first,
                                 NULL);

    // the first option not configurable
    if (_case->setting == SET_UNCONFIGURABLE)
    {
        d_arg_option_set_configurable(first,
                                      false);
    }

    // or hidden, in the empty group
    if (_case->setting == SET_HIDDEN)
    {
        (void)d_arg_option_set_group(first,
                                     "");
    }

    d_arg_parser_clear_definition_error(&run.parser);

    struct d_arg_option* const tried =
        d_arg_command_add_option(places[_case->target],
                                 _case->tried,
                                 NULL);

    d_tests_arg_command_tried(_out,
                              &run,
                              _case->tag,
                              (tried != NULL));
    d_tests_arg_end(&run);

    return;
}

/*
d_tests_arg_command_add
  The names a command refuses an option because another option holds them,
as CLI11's add_option checks them.
  Tests the following:
  - a long, short or positional name another option of the command holds,
    and a positional name another holds as a long or short name, refused
    with OptionAlreadyAdded, naming the name and the option holding it
  - an option group refuses the names of the command it belongs to, with
    CLI11's other messages, but for an option that is not configurable;
    and a group inside a group refuses them too
  - short names of more than one character refused beside the
    one-character names they begin with, and the reverse
  - a hidden option holds its names, and is named as ''; ignore_case in
    the option defaults makes a name in another case the same name
  - a name an option group holds is free in its parent, and one the parent
    holds is free in a subcommand
*/
bool
d_tests_arg_command_add(void)
{
    struct d_arg_buffer out = D_ARG_BUFFER_INIT;

    // each case, in a parser of its own
    for (size_t index = 0u; index < ADD_COUNT; ++index)
    {
        d_tests_arg_command_added(&out,
                                  &ADD_CASES[index]);
    }

    const bool same = d_tests_arg_same("ADD",
                                       d_arg_buffer_text(&out),
                                       ADD);

    d_arg_buffer_release(&out);

    return same;
}

/**
 * @brief Appends L01: a command with two options of its own, an option
 *        group `+P` and a group Q with one each, and a subcommand in the
 *        group `+S` with one; then what the command and Q list.
 *
 * @param[in,out] _out  the transcript.
 */
static void
d_tests_arg_command_listed_groups(
    struct d_arg_buffer* _out
)
{
    struct d_tests_arg_run      run = { 0 };
    struct d_arg_command* const app = d_tests_arg_begin(&run);

    (void)d_arg_command_add_option(app,
                                   "--a",
                                   NULL);
    (void)d_arg_command_add_option(app,
                                   "--b",
                                   NULL);

    struct d_arg_command* const plus = d_arg_command_add_option_group(app,
                                                                      "+P",
                                                                      NULL);

    (void)d_arg_command_add_option(plus,
                                   "--c",
                                   NULL);

    struct d_arg_command* const q = d_arg_command_add_option_group(app,
                                                                   "Q",
                                                                   NULL);

    (void)d_arg_command_add_option(q,
                                   "--d",
                                   NULL);

    struct d_arg_command* const s = d_arg_command_add_subcommand(app,
                                                                 "s",
                                                                 NULL);

    (void)d_arg_command_set_group(s,
                                  "+S");
    (void)d_arg_command_add_option(s,
                                   "--e",
                                   NULL);
    d_tests_arg_command_line_listing(_out,
                                     "L01",
                                     app);
    d_tests_arg_command_line_listing(_out,
                                     "L01q",
                                     q);
    d_tests_arg_end(&run);

    return;
}

/**
 * @brief Appends L02: a command with two options, a subcommand that falls
 *        through to it and holds one of their names, and one that does not
 *        fall through; then what the two subcommands list.
 *
 * @param[in,out] _out  the transcript.
 */
static void
d_tests_arg_command_listed_fallthrough(
    struct d_arg_buffer* _out
)
{
    struct d_tests_arg_run      run = { 0 };
    struct d_arg_command* const app = d_tests_arg_begin(&run);

    (void)d_arg_command_add_option(app,
                                   "--a",
                                   NULL);
    (void)d_arg_command_add_option(app,
                                   "--b",
                                   NULL);

    struct d_arg_command* const s = d_arg_command_add_subcommand(app,
                                                                 "s",
                                                                 NULL);

    d_arg_command_set_fallthrough(s,
                                  true);
    (void)d_arg_command_add_option(s,
                                   "--b,--bb",
                                   NULL);

    struct d_arg_command* const t = d_arg_command_add_subcommand(app,
                                                                 "t",
                                                                 NULL);

    (void)d_arg_command_add_option(t,
                                   "--c",
                                   NULL);
    d_tests_arg_command_line_listing(_out,
                                     "L02",
                                     s);
    d_tests_arg_command_line_listing(_out,
                                     "L02t",
                                     t);
    d_tests_arg_end(&run);

    return;
}

/*
d_tests_arg_command_listed
  The options a command lists, as CLI11's get_options lists them for
configuration files and requirements.
  Tests the following:
  - the command's own options first, then those of its option groups and of
    its subcommands in a group starting with `+`, such a subcommand's help
    flag among them; an option group lists only its own
  - with fallthrough, the parent's options after the command's own, but for
    those answering to a name already listed; without it, the command's own
    alone
*/
bool
d_tests_arg_command_listed(void)
{
    struct d_arg_buffer out = D_ARG_BUFFER_INIT;

    d_tests_arg_command_listed_groups(&out);
    d_tests_arg_command_listed_fallthrough(&out);

    const bool same = d_tests_arg_same("LISTED",
                                       d_arg_buffer_text(&out),
                                       LISTED);

    d_arg_buffer_release(&out);

    return same;
}

/**
 * @brief A flag's function that does nothing, for the flags FLAGS only
 *        makes.
 *
 * @param[in] _option   the flag; unused.
 * @param[in] _count    its count or sum; unused.
 * @param[in] _context  unused.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_command_ignored(
    struct d_arg_option* _option,
    long                 _count,
    void*                _context
)
{
    (void)_option;
    (void)_count;
    (void)_context;

    return D_ARG_OK;
}

/**
 * @brief Makes the flag of a case of FLAGS bound to a variable, with the
 *        variable's descriptor: an `int`, which has the description
 *        `count`, a `bool`, a list of text or a `char`.
 *
 * @param[in,out] _app    the command.
 * @param[in]     _case   the case, of a KIND_INT to KIND_CHAR kind.
 * @param[in,out] _bound  the variables.
 * @return the flag; NULL when it was refused.
 */
static struct d_arg_option*
d_tests_arg_command_flag_bound_to(
    struct d_arg_command*                       _app,
    const struct d_tests_arg_command_flag_case* _case,
    struct d_tests_arg_command_bound*           _bound
)
{
    void*                    target = &_bound->character;
    const struct d_arg_type* type   = d_arg_type_builtin(D_ARG_BUILTIN_CHAR);

    // the variable of the case's kind, and its descriptor
    switch (_case->kind)
    {
        case KIND_INT:
            target = &_bound->count;
            type   = d_arg_type_builtin(D_ARG_BUILTIN_INT);
            break;

        case KIND_BOOL:
            target = &_bound->on;
            type   = d_arg_type_builtin(D_ARG_BUILTIN_BOOL);
            break;

        case KIND_LIST:
            target = &_bound->many;
            type   = &TEXTS_TYPE;
            break;

        default:
            break;
    }

    return d_arg_command_add_flag_bound(_app,
                                        _case->tried,
                                        target,
                                        type,
                                        (_case->kind == KIND_INT)
                                        ? "count"
                                        : NULL);
}

/**
 * @brief Makes the flag of a case of FLAGS with the case's call: add_flag,
 *        add_flag_callback with the description `cbd`, add_flag_function
 *        with `fnd`, or add_flag bound to a variable.
 *
 * @param[in,out] _app    the command.
 * @param[in]     _case   the case.
 * @param[in,out] _bound  the variables a bound flag takes.
 * @return the flag; NULL when it was refused.
 */
static struct d_arg_option*
d_tests_arg_command_flag_made(
    struct d_arg_command*                       _app,
    const struct d_tests_arg_command_flag_case* _case,
    struct d_tests_arg_command_bound*           _bound
)
{
    // a plain flag
    if (_case->kind == KIND_FLAG)
    {
        return d_arg_command_add_flag(_app,
                                      _case->tried,
                                      NULL);
    }

    // a flag calling a function once it is true
    if (_case->kind == KIND_CALLBACK)
    {
        return d_arg_command_add_flag_callback(_app,
                                               _case->tried,
                                               d_tests_arg_command_ignored,
                                               NULL,
                                               NULL,
                                               "cbd");
    }

    // a flag calling a function with its sum
    if (_case->kind == KIND_FUNCTION)
    {
        return d_arg_command_add_flag_function(_app,
                                               _case->tried,
                                               d_tests_arg_command_ignored,
                                               NULL,
                                               NULL,
                                               "fnd");
    }

    return d_tests_arg_command_flag_bound_to(_app,
                                             _case,
                                             _bound);
}

/**
 * @brief Appends a flag as the reference's flag_desc writes it: the option,
 *        then its type name, whether it captures its default, and whether a
 *        default calls it back; `(null)` for no flag.
 *
 * @param[in,out] _out   the transcript.
 * @param[in]     _flag  the flag; may be NULL.
 */
static void
d_tests_arg_command_put_flag(
    struct d_arg_buffer*       _out,
    const struct d_arg_option* _flag
)
{
    d_tests_arg_command_put_option(_out,
                                   _flag);

    // a flag's type name, and how it treats a default
    if (_flag != NULL)
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       " type='");
        (void)d_arg_option_type_name(_flag,
                                     _out);
        (void)d_arg_buffer_printf(
            _out,
            "' capture=%d for_default=%d",
            (int)d_arg_option_has_flag(_flag,
                                       D_ARG_OPTION_ALWAYS_CAPTURE_DEFAULT),
            (int)d_arg_option_has_flag(_flag,
                                       D_ARG_OPTION_RUN_CALLBACK_FOR_DEFAULT));
    }

    return;
}

/**
 * @brief Appends one case of FLAGS, in a parser of its own: the flag made
 *        before it, then the line of the flag tried, the flag, and what the
 *        command lists.
 *
 * @param[in,out] _out   the transcript.
 * @param[in]     _case  the case.
 */
static void
d_tests_arg_command_flag_tried(
    struct d_arg_buffer*                        _out,
    const struct d_tests_arg_command_flag_case* _case
)
{
    struct d_tests_arg_run           run   = { 0 };
    struct d_arg_command* const      app   = d_tests_arg_begin(&run);
    struct d_tests_arg_command_bound bound = { 0,
                                               false,
                                               { D_ARG_BUFFER_INIT, 0u },
                                               'x' };

    // the flag made before the one tried
    if (_case->first != NULL)
    {
        (void)d_arg_command_add_flag(app,
                                     _case->first,
                                     NULL);
    }

    d_arg_parser_clear_definition_error(&run.parser);

    struct d_arg_option* const flag = d_tests_arg_command_flag_made(app,
                                                                    _case,
                                                                    &bound);

    d_tests_arg_command_tried(_out,
                              &run,
                              _case->tag,
                              (flag != NULL));
    (void)d_arg_buffer_printf(_out,
                              "%so: ",
                              _case->tag);
    d_tests_arg_command_put_flag(_out,
                                 flag);
    (void)d_arg_buffer_printf(_out,
                              "\n%sn: ",
                              _case->tag);
    d_tests_arg_command_put_listing(_out,
                                    app);
    (void)d_arg_buffer_append_char(_out,
                                   '\n');
    d_tests_arg_end(&run);
    d_arg_buffer_release(&bound.many.joined);

    return;
}

/*
d_tests_arg_command_flags
  The flags each call makes, and what they are made with.
  Tests the following:
  - add_flag: values in braces and `!` kept with the names, `!` as
    `{false}`; a positional name refused with IncorrectConstruction, "Flags
    cannot be positional", leaving no flag; a name taken refused
  - add_flag_callback and add_flag_function take the description given, and
    add_flag_function's flag sums its values
  - a flag bound to an `int` counts -- it sums, defaults to "0" and is
    always called back -- one bound to a list takes every value and is set
    from a default, and one bound to a `bool` or a `char` captures its
    default; none has a type name
  - a flag bound to a variable whose name is taken is refused
*/
bool
d_tests_arg_command_flags(void)
{
    struct d_arg_buffer out = D_ARG_BUFFER_INIT;

    // each case, in a parser of its own
    for (size_t index = 0u; index < FLAG_COUNT; ++index)
    {
        d_tests_arg_command_flag_tried(&out,
                                       &FLAG_CASES[index]);
    }

    const bool same = d_tests_arg_same("FLAGS",
                                       d_arg_buffer_text(&out),
                                       FLAGS);

    d_arg_buffer_release(&out);

    return same;
}

/**
 * @brief Notes what FLAG_BOUND's variables hold, as the reference writes
 *        them: `values n=.. on=.. many=[..] c=..`.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _bound  the variables.
 */
static void
d_tests_arg_command_note_bound(
    struct d_tests_arg_run*                 _run,
    const struct d_tests_arg_command_bound* _bound
)
{
    const struct d_pack_text many = d_arg_buffer_text(&_bound->many.joined);

    d_tests_arg_note(_run,
                     "values n=%d on=%d many=[%.*s] c=%c",
                     _bound->count,
                     (int)_bound->on,
                     (int)many.length,
                     (many.data != NULL) ? many.data : "",
                     _bound->character);

    return;
}

/**
 * @brief Makes FLAG_BOUND's flags, each reported under the name of its
 *        variable: `-n,--count` bound to an `int`, `--on` to a `bool`,
 *        `--many` to a list of text, and `--c` to a `char`.
 *
 * @param[in,out] _run    the run.
 * @param[in,out] _app    its root.
 * @param[in,out] _bound  the variables.
 */
static void
d_tests_arg_command_bind_flags(
    struct d_tests_arg_run*           _run,
    struct d_arg_command*             _app,
    struct d_tests_arg_command_bound* _bound
)
{
    (void)d_tests_arg_watch_option(
              _run,
              "count",
              d_arg_command_add_flag_bound(
                  _app,
                  "-n,--count",
                  &_bound->count,
                  d_arg_type_builtin(D_ARG_BUILTIN_INT),
                  NULL));
    (void)d_tests_arg_watch_option(
              _run,
              "on",
              d_arg_command_add_flag_bound(
                  _app,
                  "--on",
                  &_bound->on,
                  d_arg_type_builtin(D_ARG_BUILTIN_BOOL),
                  NULL));
    (void)d_tests_arg_watch_option(
              _run,
              "many",
              d_arg_command_add_flag_bound(
                  _app,
                  "--many",
                  &_bound->many,
                  &TEXTS_TYPE,
                  NULL));
    (void)d_tests_arg_watch_option(
              _run,
              "c",
              d_arg_command_add_flag_bound(
                  _app,
                  "--c",
                  &_bound->character,
                  d_arg_type_builtin(D_ARG_BUILTIN_CHAR),
                  NULL));

    return;
}

/*
d_tests_arg_command_flag_bound
  Flags bound to variables in parses, as CLI11's add_flag binds them.
  Tests the following:
  - a count sums its values, each use 1 and a number itself, and a parse
    that does not give it sets it to 0, its callback forced
  - a truth value takes its value; a list adds every value to what it held,
    from one parse to the next; a character takes the value's letter
  - a value the count or the truth value cannot read ends the parse with
    ConversionError
*/
bool
d_tests_arg_command_flag_bound(void)
{
    struct d_tests_arg_run           run   = { 0 };
    struct d_arg_command* const      app   = d_tests_arg_begin(&run);
    struct d_tests_arg_command_bound bound = { 0,
                                               false,
                                               { D_ARG_BUFFER_INIT, 0u },
                                               'x' };

    d_tests_arg_command_bind_flags(&run,
                                   app,
                                   &bound);

    // each parse, then what the variables hold
    for (size_t index = 0u; index < BOUND_COUNT; ++index)
    {
        d_tests_arg_command_parse_listed(&run,
                                         BOUND_ARGS[index].list);
        d_tests_arg_command_note_bound(&run,
                                       &bound);
    }

    const bool same = d_tests_arg_matches(&run,
                                          FLAG_BOUND);

    d_tests_arg_end(&run);
    d_arg_buffer_release(&bound.many.joined);

    return same;
}

/**
 * @brief Notes `cb`; add_flag_callback's function in the sums' runs.
 *
 * @param[in]     _option   the flag; unused.
 * @param[in]     _count    1; unused.
 * @param[in,out] _context  the run.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_command_noted_true(
    struct d_arg_option* _option,
    long                 _count,
    void*                _context
)
{
    (void)_option;
    (void)_count;

    d_tests_arg_note((struct d_tests_arg_run*)_context,
                     "cb");

    return D_ARG_OK;
}

/**
 * @brief Notes `fn <sum>`; add_flag_function's function in the sums' runs.
 *
 * @param[in]     _option   the flag; unused.
 * @param[in]     _count    the sum.
 * @param[in,out] _context  the run.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_command_noted_sum(
    struct d_arg_option* _option,
    long                 _count,
    void*                _context
)
{
    (void)_option;

    d_tests_arg_note((struct d_tests_arg_run*)_context,
                     "fn %ld",
                     _count);

    return D_ARG_OK;
}

/**
 * @brief Notes `fn <sum>`; add_flag_function_intmax's function in the sums'
 *        runs.
 *
 * @param[in]     _option   the flag; unused.
 * @param[in]     _count    the sum.
 * @param[in,out] _context  the run.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_command_noted_wide(
    struct d_arg_option* _option,
    intmax_t             _count,
    void*                _context
)
{
    (void)_option;

    d_tests_arg_note((struct d_tests_arg_run*)_context,
                     "fn %lld",
                     (long long)_count);

    return D_ARG_OK;
}

/**
 * @brief Runs parses of the reference's summing flags, in one parser:
 *        `--cb`, whose function add_flag_callback calls once it is true,
 *        and `--fn,-f`, whose function add_flag_function or
 *        add_flag_function_intmax gives the sum; and compares the
 *        transcript.
 *
 * @param[in] _wide      whether the sum is given as an intmax_t, not a
 *                       `long`.
 * @param[in] _rule      the d_arg_integer_rule the flags read numbers by.
 * @param[in] _args      the parses' arguments, each list ending in NULL.
 * @param[in] _count     how many parses.
 * @param[in] _expected  the transcript expected.
 * @return whether the transcript was as expected.
 */
static bool
d_tests_arg_command_summed(
    bool                                   _wide,
    int                                    _rule,
    const struct d_tests_arg_command_args* _args,
    size_t                                 _count,
    const char*                            _expected
)
{
    struct d_tests_arg_run      run = { 0 };
    struct d_arg_command* const app = d_tests_arg_begin(&run);

    d_arg_option_defaults_set_integer_rule(d_arg_command_option_defaults(app),
                                           _rule);
    (void)d_arg_command_add_flag_callback(app,
                                          "--cb",
                                          d_tests_arg_command_noted_true,
                                          &run,
                                          NULL,
                                          NULL);

    // the sum as an intmax_t
    if (_wide)
    {
        (void)d_arg_command_add_flag_function_intmax(
                  app,
                  "--fn,-f",
                  d_tests_arg_command_noted_wide,
                  &run,
                  NULL,
                  NULL);
    }
    else
    {
        (void)d_arg_command_add_flag_function(app,
                                              "--fn,-f",
                                              d_tests_arg_command_noted_sum,
                                              &run,
                                              NULL,
                                              NULL);
    }

    // each parse
    for (size_t index = 0u; index < _count; ++index)
    {
        d_tests_arg_command_parse_listed(&run,
                                         _args[index].list);
    }

    const bool same = d_tests_arg_matches(&run,
                                          _expected);

    d_tests_arg_end(&run);

    return same;
}

/*
d_tests_arg_command_flag_sums
  The function add_flag_callback calls, and the sums add_flag_function gives
its function, as a `long` and as an intmax_t, the flags reading numbers by
CLI11's rule for leading zeros.
  Tests the following:
  - add_flag_callback's function is called once its flag is given true, not
    for `false`, and a value that is no truth value is a ConversionError
  - add_flag_function's is given the sum of the values: a use counts 1,
    `false` -1, a number itself -- with an exponent, or in hexadecimal,
    octal or binary as strtoll reads them, `08` as a double -- and a word
    alone 0
  - sums past 32 bits, to the 64-bit limits, given whole as an intmax_t;
    as a `long` where one has 64 bits, and as 0 where it has 32
*/
bool
d_tests_arg_command_flag_sums(void)
{
    const char* const wide_long = (LONG_MAX > 2147483647L)
                                  ? FLAG_SUMS_WIDE
                                  : NARROW_SUMS;
    const bool        sums      = d_tests_arg_command_summed(false,
                                                             D_ARG_INTEGER_C,
                                                             SUM_ARGS,
                                                             SUM_COUNT,
                                                             FLAG_SUMS);
    const bool        wide      = d_tests_arg_command_summed(false,
                                                             D_ARG_INTEGER_C,
                                                             WIDE_ARGS,
                                                             WIDE_COUNT,
                                                             wide_long);
    const bool        sums_max  = d_tests_arg_command_summed(true,
                                                             D_ARG_INTEGER_C,
                                                             SUM_ARGS,
                                                             SUM_COUNT,
                                                             FLAG_SUMS);
    const bool        wide_max  = d_tests_arg_command_summed(true,
                                                             D_ARG_INTEGER_C,
                                                             WIDE_ARGS,
                                                             WIDE_COUNT,
                                                             FLAG_SUMS_WIDE);

    return ( (sums)     &&
             (wide)     &&
             (sums_max) &&
             (wide_max) );
}

/**
 * @brief H01: the help flag a command starts with.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_h01(
    struct d_tests_arg_command_case* _case
)
{
    d_tests_arg_command_line_option(_case->out,
                                    "H01",
                                    _case->app->help);

    return;
}

/**
 * @brief H02: a help-all flag made, and what the command lists.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_h02(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_option* const all =
        d_arg_command_set_help_all_flag(_case->app,
                                        "--help-all",
                                        "Expand all");

    d_tests_arg_command_tried(_case->out,
                              _case->run,
                              "H02",
                              (all != NULL));
    d_tests_arg_command_line_option(_case->out,
                                    "H02o",
                                    all);
    d_tests_arg_command_line_listing(_case->out,
                                     "H02n",
                                     _case->app);

    return;
}

/**
 * @brief H03: the help flag removed by an empty name.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_h03(
    struct d_tests_arg_command_case* _case
)
{
    (void)d_arg_command_set_help_flag(_case->app,
                                      "",
                                      NULL);
    d_tests_arg_command_tried(_case->out,
                              _case->run,
                              "H03",
                              (_case->app->help == NULL));
    d_tests_arg_command_line_option(_case->out,
                                    "H03o",
                                    _case->app->help);
    d_tests_arg_command_line_listing(_case->out,
                                     "H03n",
                                     _case->app);

    return;
}

/**
 * @brief H04: a help flag refused for a name an option holds, which leaves
 *        no help flag.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_h04(
    struct d_tests_arg_command_case* _case
)
{
    (void)d_arg_command_add_option(_case->app,
                                   "-z",
                                   NULL);
    d_arg_parser_clear_definition_error(&_case->run->parser);

    struct d_arg_option* const help =
        d_arg_command_set_help_flag(_case->app,
                                    "-z,--zz",
                                    "x");

    d_tests_arg_command_tried(_case->out,
                              _case->run,
                              "H04",
                              (help != NULL));
    d_tests_arg_command_line_option(_case->out,
                                    "H04o",
                                    _case->app->help);
    d_tests_arg_command_line_listing(_case->out,
                                     "H04n",
                                     _case->app);

    return;
}

/**
 * @brief Appends a line `<tag>: <option>` for a subcommand's help flag, or
 *        `(null)` for no subcommand.
 *
 * @param[in,out] _out         the transcript.
 * @param[in]     _tag         the line's tag.
 * @param[in]     _subcommand  the subcommand; may be NULL.
 */
static void
d_tests_arg_command_line_help(
    struct d_arg_buffer*        _out,
    const char*                 _tag,
    const struct d_arg_command* _subcommand
)
{
    d_tests_arg_command_line_option(_out,
                                    _tag,
                                    (_subcommand != NULL)
                                    ? _subcommand->help
                                    : NULL);

    return;
}

/**
 * @brief H13: the copy of a help flag with a value of its own that a
 *        subcommand starts with.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_h13(
    struct d_tests_arg_command_case* _case
)
{
    (void)d_arg_command_set_help_flag(_case->app,
                                      "-h,--help{yes}",
                                      "H");
    d_tests_arg_command_line_help(_case->out,
                                  "H13",
                                  d_arg_command_add_subcommand(_case->app,
                                                               "s",
                                                               NULL));

    return;
}

/**
 * @brief H14: the copy of the help flag a subcommand starts with, beside
 *        option defaults the flag does not take, and the root's own.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_h14(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_option_defaults* const defaults =
        d_arg_command_option_defaults(_case->app);

    d_arg_option_defaults_set_multi_option_policy(
        defaults,
        (int)D_ARG_MULTI_TAKE_ALL);
    (void)d_arg_option_defaults_set_group(defaults,
                                          "Defaults");
    d_tests_arg_command_line_help(_case->out,
                                  "H14",
                                  d_arg_command_add_subcommand(_case->app,
                                                               "s",
                                                               NULL));
    d_tests_arg_command_line_help(_case->out,
                                  "H14r",
                                  _case->app);

    return;
}

/**
 * @brief H16: a help flag refused for the version flag's name, beside a
 *        help-all flag and a configuration option, which leaves no help
 *        flag.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_h16(
    struct d_tests_arg_command_case* _case
)
{
    (void)d_arg_command_set_help_all_flag(_case->app,
                                          "--help-all",
                                          NULL);
    (void)d_arg_command_set_version_flag(_case->app,
                                         "-V",
                                         "1",
                                         NULL);
    (void)d_arg_command_set_config(_case->app,
                                   "--cfg",
                                   NULL,
                                   NULL,
                                   false);
    d_arg_parser_clear_definition_error(&_case->run->parser);

    struct d_arg_option* const help =
        d_arg_command_set_help_flag(_case->app,
                                    "-V",
                                    NULL);

    d_tests_arg_command_tried(_case->out,
                              _case->run,
                              "H16",
                              (help != NULL));
    d_tests_arg_command_line_option(_case->out,
                                    "H16o",
                                    _case->app->help);
    d_tests_arg_command_line_listing(_case->out,
                                     "H16n",
                                     _case->app);

    return;
}

/**
 * @brief H05: a version flag made with its text and CLI11's description,
 *        and the text it shows.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_h05(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_option* const version =
        d_arg_command_set_version_flag(_case->app,
                                       "-V,--version",
                                       "1.2.3",
                                       NULL);

    d_tests_arg_command_tried(_case->out,
                              _case->run,
                              "H05",
                              (version != NULL));
    d_tests_arg_command_line_option(_case->out,
                                    "H05o",
                                    version);
    d_tests_arg_command_line_version(_case->out,
                                     "H05v",
                                     _case->app);

    return;
}

/**
 * @brief H06: the version flag removed by an empty name, with its text.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_h06(
    struct d_tests_arg_command_case* _case
)
{
    (void)d_arg_command_set_version_flag(_case->app,
                                         "-V",
                                         "9",
                                         NULL);
    (void)d_arg_command_set_version_flag(_case->app,
                                         "",
                                         NULL,
                                         NULL);
    d_tests_arg_command_tried(_case->out,
                              _case->run,
                              "H06",
                              (_case->app->version == NULL));
    d_tests_arg_command_line_option(_case->out,
                                    "H06o",
                                    _case->app->version);
    d_tests_arg_command_line_version(_case->out,
                                     "H06v",
                                     _case->app);
    d_tests_arg_command_line_listing(_case->out,
                                     "H06n",
                                     _case->app);

    return;
}

/**
 * @brief H11: a version flag made with a description of its own.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_h11(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_option* const version =
        d_arg_command_set_version_flag(_case->app,
                                       "--ver",
                                       "2.0",
                                       "Show it");

    d_tests_arg_command_tried(_case->out,
                              _case->run,
                              "H11",
                              (version != NULL));
    d_tests_arg_command_line_option(_case->out,
                                    "H11o",
                                    version);
    d_tests_arg_command_line_version(_case->out,
                                     "H11v",
                                     _case->app);

    return;
}

/**
 * @brief H12: a version flag refused for a name an option holds, which
 *        leaves no version flag and no text.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_h12(
    struct d_tests_arg_command_case* _case
)
{
    (void)d_arg_command_add_option(_case->app,
                                   "--taken",
                                   NULL);
    d_arg_parser_clear_definition_error(&_case->run->parser);

    struct d_arg_option* const version =
        d_arg_command_set_version_flag(_case->app,
                                       "--taken",
                                       "3",
                                       NULL);

    d_tests_arg_command_tried(_case->out,
                              _case->run,
                              "H12",
                              (version != NULL));
    d_tests_arg_command_line_option(_case->out,
                                    "H12o",
                                    _case->app->version);
    d_tests_arg_command_line_version(_case->out,
                                     "H12v",
                                     _case->app);

    return;
}

/**
 * @brief H07: a configuration option made with a default file, a
 *        description and required.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_h07(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_option* const config =
        d_arg_command_set_config(_case->app,
                                 "--config",
                                 "def.ini",
                                 "Read an ini file",
                                 true);

    d_tests_arg_command_tried(_case->out,
                              _case->run,
                              "H07",
                              (config != NULL));
    d_tests_arg_command_line_option(_case->out,
                                    "H07o",
                                    config);

    return;
}

/**
 * @brief H08: a configuration option made with a name alone.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_h08(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_option* const config =
        d_arg_command_set_config(_case->app,
                                 "--cfg",
                                 NULL,
                                 NULL,
                                 false);

    d_tests_arg_command_tried(_case->out,
                              _case->run,
                              "H08",
                              (config != NULL));
    d_tests_arg_command_line_option(_case->out,
                                    "H08o",
                                    config);

    return;
}

/**
 * @brief H09: the configuration option removed by an empty name.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_h09(
    struct d_tests_arg_command_case* _case
)
{
    (void)d_arg_command_set_config(_case->app,
                                   "--cfg",
                                   NULL,
                                   NULL,
                                   false);
    (void)d_arg_command_set_config(_case->app,
                                   "",
                                   NULL,
                                   NULL,
                                   false);
    d_tests_arg_command_tried(_case->out,
                              _case->run,
                              "H09",
                              (_case->app->config == NULL));
    d_tests_arg_command_line_option(_case->out,
                                    "H09o",
                                    _case->app->config);
    d_tests_arg_command_line_listing(_case->out,
                                     "H09n",
                                     _case->app);

    return;
}

/**
 * @brief H10: a configuration option made after the option defaults ask
 *        for required options.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_h10(
    struct d_tests_arg_command_case* _case
)
{
    d_arg_option_defaults_set_flag(d_arg_command_option_defaults(_case->app),
                                   D_ARG_OPTION_REQUIRED,
                                   true);

    struct d_arg_option* const config =
        d_arg_command_set_config(_case->app,
                                 "--cfg",
                                 NULL,
                                 NULL,
                                 false);

    d_tests_arg_command_tried(_case->out,
                              _case->run,
                              "H10",
                              (config != NULL));
    d_tests_arg_command_line_option(_case->out,
                                    "H10o",
                                    config);

    return;
}

/**
 * @brief H15: a configuration option refused for a name an option holds,
 *        which leaves none, the one it replaced removed first.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_h15(
    struct d_tests_arg_command_case* _case
)
{
    (void)d_arg_command_set_config(_case->app,
                                   "--cfg",
                                   "a.ini",
                                   NULL,
                                   false);
    (void)d_arg_command_add_option(_case->app,
                                   "--used",
                                   NULL);
    d_arg_parser_clear_definition_error(&_case->run->parser);

    struct d_arg_option* const config =
        d_arg_command_set_config(_case->app,
                                 "--used",
                                 "b.ini",
                                 "Read",
                                 true);

    d_tests_arg_command_tried(_case->out,
                              _case->run,
                              "H15",
                              (config != NULL));
    d_tests_arg_command_line_option(_case->out,
                                    "H15o",
                                    _case->app->config);
    d_tests_arg_command_line_listing(_case->out,
                                     "H15n",
                                     _case->app);

    return;
}

// SPECIAL_CASES
//   constant: SPECIAL's cases, in the reference's order: the help flags,
// the version flags, then the configuration options.
static const d_tests_arg_command_case_fn SPECIAL_CASES[] =
{
    d_tests_arg_command_h01, d_tests_arg_command_h02, d_tests_arg_command_h03,
    d_tests_arg_command_h04, d_tests_arg_command_h13, d_tests_arg_command_h14,
    d_tests_arg_command_h16, d_tests_arg_command_h05, d_tests_arg_command_h06,
    d_tests_arg_command_h11, d_tests_arg_command_h12, d_tests_arg_command_h07,
    d_tests_arg_command_h08, d_tests_arg_command_h09, d_tests_arg_command_h10,
    d_tests_arg_command_h15
};

// SPECIAL_COUNT
//   constant: how many cases SPECIAL_CASES holds.
static const size_t SPECIAL_COUNT =
    sizeof(SPECIAL_CASES) / sizeof(SPECIAL_CASES[0]);

/*
d_tests_arg_command_special
  The options that ask for help, the version and configuration files, as
CLI11's set_help_flag, set_help_all_flag, set_version_flag and set_config
make them.
  Tests the following:
  - the help flag every command starts with and a help-all flag, neither
    configurable, both called back first; an empty name removes the flag;
    a name taken refuses it and leaves none, the old flag removed first
  - a subcommand copies its parent's help flag as it is, value and all,
    without the parent's option defaults
  - the version flag, with its text and CLI11's description or one of its
    own; an empty name removes it with its text; a name taken leaves none
  - the configuration option, its policy reversing the files given: a
    default file forces its callback, and it is required when asked or
    when the option defaults require options; an empty name removes it; a
    name taken leaves none
*/
bool
d_tests_arg_command_special(void)
{
    return d_tests_arg_command_cases("SPECIAL",
                                     SPECIAL_CASES,
                                     SPECIAL_COUNT,
                                     SPECIAL);
}

//==============================================================================
// 2.  SUBCOMMANDS AND GROUPS
//==============================================================================

/**
 * @brief Appends one of REFUSED_NAMES' lines, in a parser of its own: the
 *        name tried as a subcommand's.
 *
 * @param[in,out] _out    the transcript.
 * @param[in]     _named  the tag and the name.
 */
static void
d_tests_arg_command_name_tried(
    struct d_arg_buffer*                    _out,
    const struct d_tests_arg_command_named* _named
)
{
    struct d_tests_arg_run      run = { 0 };
    struct d_arg_command* const app = d_tests_arg_begin(&run);
    struct d_arg_command* const sub = d_arg_command_add_subcommand(app,
                                                                   _named->name,
                                                                   NULL);

    d_tests_arg_command_tried(_out,
                              &run,
                              _named->tag,
                              (sub != NULL));
    d_tests_arg_end(&run);

    return;
}

/**
 * @brief Appends a line of a subcommand tried beside another's name, as
 *        TRY writes it; the parser's definition error is cleared first.
 *
 * @param[in,out] _case    the case's transcript and run.
 * @param[in,out] _parent  the command the subcommand is tried on.
 * @param[in]     _tag     the line's tag.
 * @param[in]     _name    the subcommand's name.
 */
static void
d_tests_arg_command_sub_tried(
    struct d_tests_arg_command_case* _case,
    struct d_arg_command*            _parent,
    const char*                      _tag,
    const char*                      _name
)
{
    d_arg_parser_clear_definition_error(&_case->run->parser);

    struct d_arg_command* const sub = d_arg_command_add_subcommand(_parent,
                                                                   _name,
                                                                   NULL);

    d_tests_arg_command_tried(_case->out,
                              _case->run,
                              _tag,
                              (sub != NULL));

    return;
}

/**
 * @brief S11: an option group's name holding a newline.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_s11(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_command* const group =
        d_arg_command_add_option_group(_case->app,
                                       "a\nb",
                                       NULL);

    d_tests_arg_command_tried(_case->out,
                              _case->run,
                              "S11",
                              (group != NULL));

    return;
}

/**
 * @brief S07: a subcommand's name another holds.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_s07(
    struct d_tests_arg_command_case* _case
)
{
    (void)d_arg_command_add_subcommand(_case->app,
                                       "dup",
                                       NULL);
    d_tests_arg_command_sub_tried(_case,
                                  _case->app,
                                  "S07",
                                  "dup");
    d_tests_arg_command_line_subcommands(_case->out,
                                         "S07s",
                                         _case->app);

    return;
}

/**
 * @brief S08: a subcommand's name in another case, under ignore_case.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_s08(
    struct d_tests_arg_command_case* _case
)
{
    (void)d_arg_command_set_ignore_case(_case->app,
                                        true);
    (void)d_arg_command_add_subcommand(_case->app,
                                       "Sub",
                                       NULL);
    d_tests_arg_command_sub_tried(_case,
                                  _case->app,
                                  "S08",
                                  "sub");

    return;
}

/**
 * @brief S10: two nameless subcommands.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_s10(
    struct d_tests_arg_command_case* _case
)
{
    (void)d_arg_command_add_subcommand(_case->app,
                                       "",
                                       NULL);
    d_tests_arg_command_sub_tried(_case,
                                  _case->app,
                                  "S10",
                                  "");
    d_tests_arg_command_line_subcommands(_case->out,
                                         "S10s",
                                         _case->app);

    return;
}

/**
 * @brief S17: a name the parent holds, tried in an option group.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_s17(
    struct d_tests_arg_command_case* _case
)
{
    (void)d_arg_command_add_subcommand(_case->app,
                                       "one",
                                       NULL);
    d_tests_arg_command_sub_tried(_case,
                                  d_arg_command_add_option_group(_case->app,
                                                                 "G",
                                                                 NULL),
                                  "S17",
                                  "one");

    return;
}

/**
 * @brief S18: a name an option group's subcommand holds, tried in the
 *        group's parent.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_s18(
    struct d_tests_arg_command_case* _case
)
{
    (void)d_arg_command_add_subcommand(
              d_arg_command_add_option_group(_case->app,
                                             "G",
                                             NULL),
              "two",
              NULL);
    d_tests_arg_command_sub_tried(_case,
                                  _case->app,
                                  "S18",
                                  "two");

    return;
}

/**
 * @brief S23: a name another subcommand holds as an alias.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_s23(
    struct d_tests_arg_command_case* _case
)
{
    (void)d_arg_command_add_alias(d_arg_command_add_subcommand(_case->app,
                                                               "abc",
                                                               NULL),
                                  "xyz");
    d_tests_arg_command_sub_tried(_case,
                                  _case->app,
                                  "S23",
                                  "xyz");

    return;
}

/**
 * @brief S24: a disabled subcommand's name, which is free.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_s24(
    struct d_tests_arg_command_case* _case
)
{
    d_arg_command_set_disabled(d_arg_command_add_subcommand(_case->app,
                                                            "dis",
                                                            NULL),
                               true);
    d_tests_arg_command_sub_tried(_case,
                                  _case->app,
                                  "S24",
                                  "dis");
    d_tests_arg_command_line_subcommands(_case->out,
                                         "S24s",
                                         _case->app);

    return;
}

/**
 * @brief S25: an option group's name, free for a subcommand; and a
 *        subcommand's name, tried in an option group of its parent.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_s25(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_command* const group =
        d_arg_command_add_option_group(_case->app,
                                       "G",
                                       NULL);
    struct d_arg_command* const mid   =
        d_arg_command_add_subcommand(_case->app,
                                     "mid",
                                     NULL);

    d_tests_arg_command_sub_tried(_case,
                                  mid,
                                  "S25",
                                  "G");
    d_tests_arg_command_sub_tried(_case,
                                  group,
                                  "S25b",
                                  "mid");

    return;
}

/**
 * @brief S09: a subcommand made with a description, and what it starts
 *        with.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_s09(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_command* const sub = d_arg_command_add_subcommand(_case->app,
                                                                   "s",
                                                                   "sd");

    d_tests_arg_command_line_command(_case->out,
                                     "S09",
                                     sub);
    (void)d_arg_buffer_printf(_case->out,
                              "S09d: %s\n",
                              d_arg_command_description(sub));

    return;
}

/**
 * @brief Appends a line of what an option group starts with, made under a
 *        command with a help-all flag.
 *
 * @param[in,out] _case  the case's transcript and run.
 * @param[in]     _tag   the line's tag.
 * @param[in]     _name  the group's name.
 */
static void
d_tests_arg_command_group_made(
    struct d_tests_arg_command_case* _case,
    const char*                      _tag,
    const char*                      _name
)
{
    (void)d_arg_command_set_help_all_flag(_case->app,
                                          "--help-all",
                                          "HA");
    d_tests_arg_command_line_command(
        _case->out,
        _tag,
        d_arg_command_add_option_group(_case->app,
                                       _name,
                                       NULL));

    return;
}

/**
 * @brief S12: an option group made under a command with a help-all flag.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_s12(
    struct d_tests_arg_command_case* _case
)
{
    d_tests_arg_command_group_made(_case,
                                   "S12",
                                   "G");

    return;
}

/**
 * @brief S13: an option group named `+G`, which takes no help-all flag.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_s13(
    struct d_tests_arg_command_case* _case
)
{
    d_tests_arg_command_group_made(_case,
                                   "S13",
                                   "+G");

    return;
}

/**
 * @brief S14: an option group with the empty name, which takes no help-all
 *        flag.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_s14(
    struct d_tests_arg_command_case* _case
)
{
    d_tests_arg_command_group_made(_case,
                                   "S14",
                                   "");

    return;
}

/**
 * @brief S15: an option group and a subcommand made under a command with a
 *        footer that falls through.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_s15(
    struct d_tests_arg_command_case* _case
)
{
    (void)d_arg_command_set_footer(_case->app,
                                   "F");
    d_arg_command_set_fallthrough(_case->app,
                                  true);

    struct d_arg_command* const group =
        d_arg_command_add_option_group(_case->app,
                                       "G",
                                       NULL);
    struct d_arg_command* const sub   =
        d_arg_command_add_subcommand(_case->app,
                                     "s",
                                     NULL);

    d_tests_arg_command_line_command(_case->out,
                                     "S15g",
                                     group);
    d_tests_arg_command_line_command(_case->out,
                                     "S15s",
                                     sub);

    return;
}

/**
 * @brief Appends the name of the subcommand a lookup found by name, or
 *        `(null)`, after a space when asked.
 *
 * @param[in,out] _out    the transcript.
 * @param[in]     _found  what the lookup found; may be NULL.
 * @param[in]     _space  whether a space goes first.
 */
static void
d_tests_arg_command_put_found(
    struct d_arg_buffer*        _out,
    const struct d_arg_command* _found,
    bool                        _space
)
{
    (void)d_arg_buffer_printf(_out,
                              "%s%s",
                              (_space) ? " " : "",
                              (_found != NULL)
                              ? d_arg_command_name(_found)
                              : "(null)");

    return;
}

/**
 * @brief S19: prefixes of two names, under prefix matching: one that both
 *        names start with, then prefixes of each, a whole name, and one of
 *        neither.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_s19(
    struct d_tests_arg_command_case* _case
)
{
    static const char* const ASKED[] =
    {
        "alp", "alph", "alpha", "alpi", "x"
    };

    d_arg_command_set_prefix_matching(_case->app,
                                      true);
    (void)d_arg_command_add_subcommand(_case->app,
                                       "alpha",
                                       NULL);
    (void)d_arg_command_add_subcommand(_case->app,
                                       "alpine",
                                       NULL);
    (void)d_arg_buffer_append_cstr(_case->out,
                                   "S19:");

    // each name asked, and what it found
    for (size_t index = 0u; index < (sizeof(ASKED) / sizeof(ASKED[0])); ++index)
    {
        d_tests_arg_command_put_found(
            _case->out,
            d_arg_command_subcommand(_case->app,
                                     ASKED[index]),
            true);
    }

    (void)d_arg_buffer_append_char(_case->out,
                                   '\n');

    return;
}

/**
 * @brief S20 and S21: option groups found by their groups -- one plain, one
 *        `+`, a nameless subcommand's `SUBCOMMANDS` -- but neither a named
 *        subcommand nor a group that is not there; then the command's
 *        subcommands.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_s20(
    struct d_tests_arg_command_case* _case
)
{
    static const char* const ASKED[] =
    {
        "G", "+H", "SUBCOMMANDS", "named", "Z"
    };

    (void)d_arg_command_add_option_group(_case->app,
                                         "G",
                                         NULL);
    (void)d_arg_command_add_option_group(_case->app,
                                         "+H",
                                         NULL);
    (void)d_arg_command_add_subcommand(_case->app,
                                       "",
                                       NULL);
    (void)d_arg_command_add_subcommand(_case->app,
                                       "named",
                                       NULL);
    (void)d_arg_buffer_append_cstr(_case->out,
                                   "S20:");

    // each group asked, and what it found
    for (size_t index = 0u; index < (sizeof(ASKED) / sizeof(ASKED[0])); ++index)
    {
        const struct d_arg_command* const found =
            d_arg_command_option_group(_case->app,
                                       ASKED[index]);

        (void)d_arg_buffer_printf(_case->out,
                                  " %s%s%s",
                                  (found != NULL) ? "[" : "",
                                  (found != NULL)
                                  ? d_arg_command_group(found)
                                  : "(null)",
                                  (found != NULL) ? "]" : "");
    }

    (void)d_arg_buffer_append_char(_case->out,
                                   '\n');
    d_tests_arg_command_line_subcommands(_case->out,
                                         "S21",
                                         _case->app);

    return;
}

/**
 * @brief S22: a subcommand of an option group, found from the group's
 *        parent.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_s22(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_command* const inner = d_arg_command_add_subcommand(
                                            d_arg_command_add_option_group(
                                                _case->app,
                                                "G",
                                                NULL),
                                            "inner",
                                            NULL);

    (void)d_arg_buffer_printf(_case->out,
                              "S22: %s\n",
                              (d_arg_command_subcommand(_case->app,
                                                        "inner") == inner)
                              ? "found"
                              : "missing");

    return;
}

/**
 * @brief S26: a subcommand found by its name in other cases, under
 *        ignore_case.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_s26(
    struct d_tests_arg_command_case* _case
)
{
    (void)d_arg_command_set_ignore_case(_case->app,
                                        true);
    (void)d_arg_command_add_subcommand(_case->app,
                                       "One",
                                       NULL);
    (void)d_arg_buffer_append_cstr(_case->out,
                                   "S26: ");
    d_tests_arg_command_put_found(_case->out,
                                  d_arg_command_subcommand(_case->app,
                                                           "ONE"),
                                  false);
    d_tests_arg_command_put_found(_case->out,
                                  d_arg_command_subcommand(_case->app,
                                                           "one"),
                                  true);
    (void)d_arg_buffer_append_char(_case->out,
                                   '\n');

    return;
}

/**
 * @brief S27: subcommands found by index, and past the last.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_s27(
    struct d_tests_arg_command_case* _case
)
{
    (void)d_arg_command_add_subcommand(_case->app,
                                       "a",
                                       NULL);
    (void)d_arg_command_add_subcommand(_case->app,
                                       "b",
                                       NULL);
    (void)d_arg_buffer_append_cstr(_case->out,
                                   "S27:");

    // the first, the second, and one past them
    for (size_t index = 0u; index < 3u; ++index)
    {
        d_tests_arg_command_put_found(
            _case->out,
            d_arg_command_subcommand_at(_case->app,
                                        index),
            true);
    }

    (void)d_arg_buffer_append_char(_case->out,
                                   '\n');

    return;
}

// SUBCOMMAND_CASES
//   constant: SUBCOMMANDS' cases after REFUSED_NAMES, in the reference's
// order: a group's name refused; names beside names taken; what new
// subcommands and groups start with; and finding them.
static const d_tests_arg_command_case_fn SUBCOMMAND_CASES[] =
{
    d_tests_arg_command_s11, d_tests_arg_command_s07, d_tests_arg_command_s08,
    d_tests_arg_command_s10, d_tests_arg_command_s17, d_tests_arg_command_s18,
    d_tests_arg_command_s23, d_tests_arg_command_s24, d_tests_arg_command_s25,
    d_tests_arg_command_s09, d_tests_arg_command_s12, d_tests_arg_command_s13,
    d_tests_arg_command_s14, d_tests_arg_command_s15, d_tests_arg_command_s19,
    d_tests_arg_command_s20, d_tests_arg_command_s22, d_tests_arg_command_s26,
    d_tests_arg_command_s27
};

// SUBCOMMAND_COUNT
//   constant: how many cases SUBCOMMAND_CASES holds.
static const size_t SUBCOMMAND_COUNT =
    sizeof(SUBCOMMAND_CASES) / sizeof(SUBCOMMAND_CASES[0]);

/*
d_tests_arg_command_subcommands
  Subcommands and option groups: the names they are refused, what a new one
starts with, and how they are found.
  Tests the following:
  - a name starting with `-`, or holding `=` or a space, refused with
    IncorrectConstruction; one starting with `=`, holding a tab, or ending
    in `}` taken, as CLI11 takes them; a group's name holding a newline
    refused
  - a name or alias another subcommand holds refused with
    OptionAlreadyAdded, in another case under ignore_case, and across an
    option group either way; a disabled subcommand's name is free, as are
    nameless subcommands and the names of option groups
  - a subcommand starts with its description, its parent's help flag and
    footer, and fallthrough when its parent has it; an option group with
    no help flag, footer or fallthrough, and with its parent's help-all
    flag unless its name is empty or starts with `+`
  - prefix matching finds an unambiguous prefix and a whole name; a group
    is found by its group, a nameless subcommand by `SUBCOMMANDS`, but not
    a named subcommand; a subcommand of an option group is found from the
    group's parent; ignore_case finds a name in any case; subcommands are
    found by index until the last
*/
bool
d_tests_arg_command_subcommands(void)
{
    struct d_arg_buffer out = D_ARG_BUFFER_INIT;

    // each refused name, in a parser of its own
    for (size_t index = 0u; index < REFUSED_COUNT; ++index)
    {
        d_tests_arg_command_name_tried(&out,
                                       &REFUSED_NAMES[index]);
    }

    const bool held = d_tests_arg_command_run_cases(&out,
                                                    SUBCOMMAND_CASES,
                                                    SUBCOMMAND_COUNT);
    const bool same = d_tests_arg_same("SUBCOMMANDS",
                                       d_arg_buffer_text(&out),
                                       SUBCOMMANDS);

    d_arg_buffer_release(&out);

    return ( (held) &&
             (same) );
}

/**
 * @brief Appends a line of an option moved into a group, as TRY writes it;
 *        the parser's definition error is cleared first.
 *
 * @param[in,out] _case    the case's transcript and run.
 * @param[in,out] _group   the option group.
 * @param[in,out] _option  the option; may be NULL.
 * @param[in]     _tag     the line's tag.
 */
static void
d_tests_arg_command_option_moved(
    struct d_tests_arg_command_case* _case,
    struct d_arg_command*            _group,
    struct d_arg_option*             _option,
    const char*                      _tag
)
{
    d_arg_parser_clear_definition_error(&_case->run->parser);

    const int status = d_arg_command_group_add_option(_group,
                                                      _option);

    d_tests_arg_command_tried(_case->out,
                              _case->run,
                              _tag,
                              (status == (int)D_ARG_OK));

    return;
}

/**
 * @brief Appends a line of a subcommand moved into a group, as TRY writes
 *        it; the parser's definition error is cleared first.
 *
 * @param[in,out] _case        the case's transcript and run.
 * @param[in,out] _group       the option group.
 * @param[in,out] _subcommand  the subcommand.
 * @param[in]     _tag         the line's tag.
 */
static void
d_tests_arg_command_sub_moved(
    struct d_tests_arg_command_case* _case,
    struct d_arg_command*            _group,
    struct d_arg_command*            _subcommand,
    const char*                      _tag
)
{
    d_arg_parser_clear_definition_error(&_case->run->parser);

    const int status = d_arg_command_group_add_subcommand(_group,
                                                          _subcommand);

    d_tests_arg_command_tried(_case->out,
                              _case->run,
                              _tag,
                              (status == (int)D_ARG_OK));

    return;
}

/**
 * @brief M01: the help flag, which no option group takes.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_m01(
    struct d_tests_arg_command_case* _case
)
{
    d_tests_arg_command_option_moved(
        _case,
        d_arg_command_add_option_group(_case->app,
                                       "G",
                                       NULL),
        _case->app->help,
        "M01");

    return;
}

/**
 * @brief M02: the configuration option, which no option group takes.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_m02(
    struct d_tests_arg_command_case* _case
)
{
    (void)d_arg_command_set_config(_case->app,
                                   "--cfg",
                                   NULL,
                                   NULL,
                                   false);
    d_tests_arg_command_option_moved(
        _case,
        d_arg_command_add_option_group(_case->app,
                                       "G",
                                       NULL),
        _case->app->config,
        "M02");

    return;
}

/**
 * @brief M03 to M05: an option moved, the two listings after, the option
 *        moved again and NULL; then the option removed from its command,
 *        which no longer holds it, and from its group, which does.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_m03(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_option* const moved = d_arg_command_add_option(_case->app,
                                                                "--a",
                                                                NULL);

    (void)d_arg_command_add_option(_case->app,
                                   "--b",
                                   NULL);

    struct d_arg_command* const group =
        d_arg_command_add_option_group(_case->app,
                                       "G",
                                       NULL);

    d_tests_arg_command_option_moved(_case,
                                     group,
                                     moved,
                                     "M03");
    d_tests_arg_command_line_listing(_case->out,
                                     "M03a",
                                     _case->app);
    d_tests_arg_command_line_listing(_case->out,
                                     "M03g",
                                     group);
    d_tests_arg_command_option_moved(_case,
                                     group,
                                     moved,
                                     "M04");
    d_tests_arg_command_option_moved(_case,
                                     group,
                                     NULL,
                                     "M05");

    const bool from_root  = d_arg_command_remove_option(_case->app,
                                                        moved);
    const bool from_group = d_arg_command_remove_option(group,
                                                        moved);

    (void)d_arg_buffer_printf(_case->out,
                              "M03r: %d %d ",
                              (int)from_root,
                              (int)from_group);
    d_tests_arg_command_put_listing(_case->out,
                                    _case->app);
    (void)d_arg_buffer_append_char(_case->out,
                                   '\n');

    return;
}

/**
 * @brief M06: an option moved into a group holding its name.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_m06(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_command* const group =
        d_arg_command_add_option_group(_case->app,
                                       "G",
                                       NULL);

    (void)d_arg_command_add_option(group,
                                   "--m",
                                   NULL);
    d_tests_arg_command_option_moved(_case,
                                     group,
                                     d_arg_command_add_option(_case->app,
                                                              "--m",
                                                              NULL),
                                     "M06");

    return;
}

/**
 * @brief M07: the version flag moved, then another made beside it; the two
 *        listings and the version shown after.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_m07(
    struct d_tests_arg_command_case* _case
)
{
    (void)d_arg_command_set_version_flag(_case->app,
                                         "-V",
                                         "1",
                                         NULL);

    struct d_arg_command* const group =
        d_arg_command_add_option_group(_case->app,
                                       "G",
                                       NULL);

    d_tests_arg_command_option_moved(_case,
                                     group,
                                     _case->app->version,
                                     "M07");
    d_arg_parser_clear_definition_error(&_case->run->parser);

    struct d_arg_option* const again =
        d_arg_command_set_version_flag(_case->app,
                                       "-V",
                                       "2",
                                       NULL);

    d_tests_arg_command_tried(_case->out,
                              _case->run,
                              "M07b",
                              (again != NULL));
    d_tests_arg_command_line_listing(_case->out,
                                     "M07a",
                                     _case->app);
    d_tests_arg_command_line_listing(_case->out,
                                     "M07g",
                                     group);
    d_tests_arg_command_line_version(_case->out,
                                     "M07v",
                                     _case->app);

    return;
}

/**
 * @brief M10: an option of one group moved into another, which its command
 *        does not hold; and the groups' listings.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_m10(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_command* const first  =
        d_arg_command_add_option_group(_case->app,
                                       "G",
                                       NULL);
    struct d_arg_command* const second =
        d_arg_command_add_option_group(_case->app,
                                       "H",
                                       NULL);

    d_tests_arg_command_option_moved(_case,
                                     second,
                                     d_arg_command_add_option(first,
                                                              "--o",
                                                              NULL),
                                     "M10");
    (void)d_arg_buffer_append_cstr(_case->out,
                                   "M10g: ");
    d_tests_arg_command_put_listing(_case->out,
                                    first);
    d_tests_arg_command_put_listing(_case->out,
                                    second);
    (void)d_arg_buffer_append_char(_case->out,
                                   '\n');

    return;
}

/**
 * @brief M08: a subcommand moved that a sibling excludes and needs; the
 *        subcommands after, and whether either link is left, from either
 *        side.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_m08(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_command* const first  =
        d_arg_command_add_subcommand(_case->app,
                                     "s1",
                                     NULL);
    struct d_arg_command* const second =
        d_arg_command_add_subcommand(_case->app,
                                     "s2",
                                     NULL);

    (void)d_arg_command_excludes_command(first,
                                         second);
    (void)d_arg_command_needs_command(first,
                                      second);

    struct d_arg_command* const group =
        d_arg_command_add_option_group(_case->app,
                                       "G",
                                       NULL);

    d_tests_arg_command_sub_moved(_case,
                                  group,
                                  second,
                                  "M08");
    (void)d_arg_buffer_append_cstr(_case->out,
                                   "M08s: ");
    d_tests_arg_command_put_subcommands(_case->out,
                                        _case->app);
    d_tests_arg_command_put_subcommands(_case->out,
                                        group);

    const bool excluded = d_arg_command_remove_excludes_command(first,
                                                                second);
    const bool needed   = d_arg_command_remove_needs_command(first,
                                                             second);
    const bool back     = d_arg_command_remove_excludes_command(second,
                                                                first);

    (void)d_arg_buffer_printf(_case->out,
                              "\nM08l: %d %d %d\n",
                              (int)excluded,
                              (int)needed,
                              (int)back);

    return;
}

/**
 * @brief M09: a subcommand with an alias that starts with the name of a
 *        subcommand under another group, moved into a group of its own.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_m09(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_command* const first = d_arg_command_add_option_group(
                                            _case->app,
                                            "G",
                                            NULL);

    (void)d_arg_command_add_subcommand(first,
                                       "one",
                                       NULL);

    struct d_arg_command* const moved = d_arg_command_add_subcommand(
                                            _case->app,
                                            "x",
                                            NULL);

    (void)d_arg_command_add_alias(moved,
                                  "one2");

    struct d_arg_command* const second = d_arg_command_add_option_group(
                                             _case->app,
                                             "H",
                                             NULL);

    d_tests_arg_command_sub_moved(_case,
                                  second,
                                  moved,
                                  "M09");
    (void)d_arg_buffer_append_cstr(_case->out,
                                   "M09s: ");
    d_tests_arg_command_put_subcommands(_case->out,
                                        _case->app);
    d_tests_arg_command_put_subcommands(_case->out,
                                        first);
    d_tests_arg_command_put_subcommands(_case->out,
                                        second);
    (void)d_arg_buffer_append_char(_case->out,
                                   '\n');

    return;
}

/**
 * @brief M11: a subcommand moved within its own group, which puts it last.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_m11(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_command* const group = d_arg_command_add_option_group(
                                            _case->app,
                                            "G",
                                            NULL);
    struct d_arg_command* const one   = d_arg_command_add_subcommand(group,
                                                                     "one",
                                                                     NULL);

    (void)d_arg_command_add_subcommand(group,
                                       "two",
                                       NULL);
    d_tests_arg_command_sub_moved(_case,
                                  group,
                                  one,
                                  "M11");
    d_tests_arg_command_line_subcommands(_case->out,
                                         "M11s",
                                         group);

    return;
}

/**
 * @brief M12: a subcommand moved into a group inside its group, and found
 *        from the root after.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_m12(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_command* const group = d_arg_command_add_option_group(
                                            _case->app,
                                            "G",
                                            NULL);
    struct d_arg_command* const one   = d_arg_command_add_subcommand(group,
                                                                     "one",
                                                                     NULL);
    struct d_arg_command* const inner = d_arg_command_add_option_group(
                                            group,
                                            "Inner",
                                            NULL);

    d_tests_arg_command_sub_moved(_case,
                                  inner,
                                  one,
                                  "M12");
    (void)d_arg_buffer_append_cstr(_case->out,
                                   "M12s: ");
    d_tests_arg_command_put_subcommands(_case->out,
                                        group);
    d_tests_arg_command_put_subcommands(_case->out,
                                        inner);
    (void)d_arg_buffer_printf(_case->out,
                              " %s\n",
                              (d_arg_command_subcommand(_case->app,
                                                        "one") == one)
                              ? "found"
                              : "missing");

    return;
}

// MOVE_CASES
//   constant: MOVES' cases, in the reference's order: options, then
// subcommands.
static const d_tests_arg_command_case_fn MOVE_CASES[] =
{
    d_tests_arg_command_m01, d_tests_arg_command_m02, d_tests_arg_command_m03,
    d_tests_arg_command_m06, d_tests_arg_command_m07, d_tests_arg_command_m10,
    d_tests_arg_command_m08, d_tests_arg_command_m09, d_tests_arg_command_m11,
    d_tests_arg_command_m12
};

// MOVE_COUNT
//   constant: how many cases MOVE_CASES holds.
static const size_t MOVE_COUNT = sizeof(MOVE_CASES) / sizeof(MOVE_CASES[0]);

/*
d_tests_arg_command_moves
  Options and subcommands moved into option groups, as CLI11's
Option_group::add_option(Option*) and add_subcommand(App*) move them.
  Tests the following:
  - the help and configuration options refused with OptionAlreadyAdded;
    the version flag moved, and a new one then made beside it, which
    version() reads
  - an option moved leaves its command's listing for the group's, is the
    group's to remove and no longer its command's, and cannot be moved
    again, nor can NULL, with OptionNotFound; a name the group holds
    refuses it; an option of another group is not found
  - a subcommand moved leaves its parent for the group, the needs and
    excludes between it and its old siblings dropped on both sides; an
    alias that only begins with a name taken is no clash; moving within a
    group puts the subcommand last; moving into a group inside the group
    keeps it found from the root
*/
bool
d_tests_arg_command_moves(void)
{
    return d_tests_arg_command_cases("MOVES",
                                     MOVE_CASES,
                                     MOVE_COUNT,
                                     MOVES);
}

/**
 * @brief R01: an option removed that another needs and excludes; the
 *        listing after, and the other's needs and excludes.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_r01(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_option* const first  = d_arg_command_add_option(_case->app,
                                                                 "--a",
                                                                 NULL);
    struct d_arg_option* const second = d_arg_command_add_option(_case->app,
                                                                 "--b",
                                                                 NULL);

    (void)d_arg_option_needs(first,
                             second);
    (void)d_arg_option_excludes(first,
                                second);
    (void)d_arg_buffer_printf(_case->out,
                              "R01: %d\n",
                              (int)d_arg_command_remove_option(_case->app,
                                                               second));
    d_tests_arg_command_line_listing(_case->out,
                                     "R01n",
                                     _case->app);
    (void)d_arg_buffer_printf(_case->out,
                              "R01x: %lu %lu\n",
                              (unsigned long)first->needs.count,
                              (unsigned long)first->excludes.count);

    return;
}

/**
 * @brief R03: a subcommand's option, removed from the root, which does not
 *        own it, then from the subcommand; the subcommand's listing after
 *        each.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_r03(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_command* const sub    = d_arg_command_add_subcommand(
                                             _case->app,
                                             "s",
                                             NULL);
    struct d_arg_option* const  option = d_arg_command_add_option(sub,
                                                                  "--o",
                                                                  NULL);

    (void)d_arg_buffer_printf(_case->out,
                              "R03: %d ",
                              (int)d_arg_command_remove_option(_case->app,
                                                               option));
    d_tests_arg_command_put_listing(_case->out,
                                    sub);
    (void)d_arg_buffer_printf(_case->out,
                              "\nR03b: %d ",
                              (int)d_arg_command_remove_option(sub,
                                                               option));
    d_tests_arg_command_put_listing(_case->out,
                                    sub);
    (void)d_arg_buffer_append_char(_case->out,
                                   '\n');

    return;
}

/**
 * @brief R05: the help flag removed, and the command's help flag and
 *        listing after.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_r05(
    struct d_tests_arg_command_case* _case
)
{
    (void)d_arg_buffer_printf(_case->out,
                              "R05: %d ",
                              (int)d_arg_command_remove_option(
                                       _case->app,
                                       _case->app->help));
    d_tests_arg_command_put_option(_case->out,
                                   _case->app->help);
    (void)d_arg_buffer_append_char(_case->out,
                                   ' ');
    d_tests_arg_command_put_listing(_case->out,
                                    _case->app);
    (void)d_arg_buffer_append_char(_case->out,
                                   '\n');

    return;
}

/**
 * @brief R06: the configuration option and a help-all flag removed, and the
 *        command's two pointers and listing after.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_r06(
    struct d_tests_arg_command_case* _case
)
{
    (void)d_arg_command_set_config(_case->app,
                                   "--cfg",
                                   NULL,
                                   NULL,
                                   false);

    struct d_arg_option* const all =
        d_arg_command_set_help_all_flag(_case->app,
                                        "--all",
                                        NULL);
    const bool                 config =
        d_arg_command_remove_option(_case->app,
                                    _case->app->config);
    const bool                 help_all =
        d_arg_command_remove_option(_case->app,
                                    all);

    (void)d_arg_buffer_printf(_case->out,
                              "R06: %d %d ",
                              (int)config,
                              (int)help_all);
    d_tests_arg_command_put_option(_case->out,
                                   _case->app->config);
    (void)d_arg_buffer_append_char(_case->out,
                                   ' ');
    d_tests_arg_command_put_option(_case->out,
                                   _case->app->help_all);
    (void)d_arg_buffer_append_char(_case->out,
                                   ' ');
    d_tests_arg_command_put_listing(_case->out,
                                    _case->app);
    (void)d_arg_buffer_append_char(_case->out,
                                   '\n');

    return;
}

/**
 * @brief R02: a subcommand removed that a sibling excludes, then the
 *        sibling; the subcommands after each.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_r02(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_command* const first  =
        d_arg_command_add_subcommand(_case->app,
                                     "s1",
                                     NULL);
    struct d_arg_command* const second =
        d_arg_command_add_subcommand(_case->app,
                                     "s2",
                                     NULL);

    (void)d_arg_command_excludes_command(first,
                                         second);
    (void)d_arg_buffer_printf(_case->out,
                              "R02: %d\n",
                              (int)d_arg_command_remove_subcommand(_case->app,
                                                                   second));
    d_tests_arg_command_line_subcommands(_case->out,
                                         "R02s",
                                         _case->app);
    (void)d_arg_buffer_printf(_case->out,
                              "R02r: %d\n",
                              (int)d_arg_command_remove_subcommand(_case->app,
                                                                   first));
    d_tests_arg_command_line_subcommands(_case->out,
                                         "R02t",
                                         _case->app);

    return;
}

/**
 * @brief R04: a subcommand's subcommand, removed from the root, which does
 *        not own it, then from its parent; the parent's subcommands after
 *        each.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_r04(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_command* const sub   = d_arg_command_add_subcommand(
                                            _case->app,
                                            "s",
                                            NULL);
    struct d_arg_command* const inner = d_arg_command_add_subcommand(sub,
                                                                     "t",
                                                                     NULL);

    (void)d_arg_buffer_printf(_case->out,
                              "R04: %d ",
                              (int)d_arg_command_remove_subcommand(_case->app,
                                                                   inner));
    d_tests_arg_command_put_subcommands(_case->out,
                                        sub);
    (void)d_arg_buffer_printf(_case->out,
                              "\nR04b: %d ",
                              (int)d_arg_command_remove_subcommand(sub,
                                                                   inner));
    d_tests_arg_command_put_subcommands(_case->out,
                                        sub);
    (void)d_arg_buffer_append_char(_case->out,
                                   '\n');

    return;
}

/**
 * @brief R07: NULL removed, as an option and as a subcommand.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_r07(
    struct d_tests_arg_command_case* _case
)
{
    const bool option = d_arg_command_remove_option(_case->app,
                                                    NULL);
    const bool sub    = d_arg_command_remove_subcommand(_case->app,
                                                        NULL);

    (void)d_arg_buffer_printf(_case->out,
                              "R07: %d %d\n",
                              (int)option,
                              (int)sub);

    return;
}

// REMOVAL_CASES
//   constant: REMOVAL's cases, in the reference's order: options, then
// subcommands, then NULL.
static const d_tests_arg_command_case_fn REMOVAL_CASES[] =
{
    d_tests_arg_command_r01, d_tests_arg_command_r03, d_tests_arg_command_r05,
    d_tests_arg_command_r06, d_tests_arg_command_r02, d_tests_arg_command_r04,
    d_tests_arg_command_r07
};

// REMOVAL_COUNT
//   constant: how many cases REMOVAL_CASES holds.
static const size_t REMOVAL_COUNT =
    sizeof(REMOVAL_CASES) / sizeof(REMOVAL_CASES[0]);

/*
d_tests_arg_command_removal
  Options and subcommands removed, as CLI11's remove_option and
remove_subcommand remove them.
  Tests the following:
  - an option removed leaves its command's listing, and the needs and
    excludes of the command's other options; a subcommand's option is not
    the root's to remove
  - the help, help-all and configuration options removed leave the
    command's pointers to them empty
  - a subcommand removed leaves its parent's list, with the exclusion a
    sibling held on it; a subcommand's subcommand is not the root's to
    remove
  - NULL is no command's option or subcommand
*/
bool
d_tests_arg_command_removal(void)
{
    return d_tests_arg_command_cases("REMOVAL",
                                     REMOVAL_CASES,
                                     REMOVAL_COUNT,
                                     REMOVAL);
}

//==============================================================================
// 3.  SETTINGS
//==============================================================================

/**
 * @brief Appends a line of a setting tried, as TRY writes it, from the
 *        status the setter returned.
 *
 * @param[in,out] _case    the case's transcript and run.
 * @param[in]     _tag     the line's tag.
 * @param[in]     _status  the status the setter returned.
 */
static void
d_tests_arg_command_set_tried(
    struct d_tests_arg_command_case* _case,
    const char*                      _tag,
    int                              _status
)
{
    d_tests_arg_command_tried(_case->out,
                              _case->run,
                              _tag,
                              (_status == (int)D_ARG_OK));

    return;
}

/**
 * @brief Appends a line `<tag>: <display name>`, with the aliases or
 *        without.
 *
 * @param[in,out] _out           the transcript.
 * @param[in]     _tag           the line's tag.
 * @param[in]     _command       the command.
 * @param[in]     _with_aliases  whether the aliases follow the name.
 */
static void
d_tests_arg_command_line_display(
    struct d_arg_buffer*        _out,
    const char*                 _tag,
    const struct d_arg_command* _command,
    bool                        _with_aliases
)
{
    (void)d_arg_buffer_printf(_out,
                              "%s: ",
                              _tag);
    (void)d_arg_command_display_name(_command,
                                     _with_aliases,
                                     _out);
    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

/**
 * @brief N01 and N02: a subcommand renamed to its sibling's name, and the
 *        name it keeps; then the root renamed.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n01(
    struct d_tests_arg_command_case* _case
)
{
    (void)d_arg_command_add_subcommand(_case->app,
                                       "a",
                                       NULL);

    struct d_arg_command* const sub = d_arg_command_add_subcommand(_case->app,
                                                                   "b",
                                                                   NULL);

    d_arg_parser_clear_definition_error(&_case->run->parser);
    d_tests_arg_command_set_tried(_case,
                                  "N01",
                                  d_arg_command_set_name(sub,
                                                         "a"));
    (void)d_arg_buffer_printf(_case->out,
                              "N01n: %s\n",
                              d_arg_command_name(sub));

    return;
}

/**
 * @brief N02: the root renamed.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n02(
    struct d_tests_arg_command_case* _case
)
{
    d_tests_arg_command_set_tried(_case,
                                  "N02",
                                  d_arg_command_set_name(_case->app,
                                                         "prog2"));
    (void)d_arg_buffer_printf(_case->out,
                              "N02n: %s\n",
                              d_arg_command_name(_case->app));

    return;
}

/**
 * @brief N03: aliases refused -- one a sibling holds, the empty one, one
 *        holding a newline -- and one taken; the display name after.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n03(
    struct d_tests_arg_command_case* _case
)
{
    (void)d_arg_command_add_alias(d_arg_command_add_subcommand(_case->app,
                                                               "a",
                                                               NULL),
                                  "x");

    struct d_arg_command* const sub = d_arg_command_add_subcommand(_case->app,
                                                                   "b",
                                                                   NULL);

    d_arg_parser_clear_definition_error(&_case->run->parser);
    d_tests_arg_command_set_tried(_case,
                                  "N03",
                                  d_arg_command_add_alias(sub,
                                                          "x"));
    d_tests_arg_command_set_tried(_case,
                                  "N03b",
                                  d_arg_command_add_alias(sub,
                                                          ""));
    d_tests_arg_command_set_tried(_case,
                                  "N03c",
                                  d_arg_command_add_alias(sub,
                                                          "a\nb"));
    d_tests_arg_command_set_tried(_case,
                                  "N03d",
                                  d_arg_command_add_alias(sub,
                                                          "y"));
    d_tests_arg_command_line_display(_case->out,
                                     "N03n",
                                     sub,
                                     true);

    return;
}

/**
 * @brief N04: the root given its subcommand's name as an alias, which no
 *        parent checks.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n04(
    struct d_tests_arg_command_case* _case
)
{
    (void)d_arg_command_add_subcommand(_case->app,
                                       "a",
                                       NULL);
    d_arg_parser_clear_definition_error(&_case->run->parser);
    d_tests_arg_command_set_tried(_case,
                                  "N04",
                                  d_arg_command_add_alias(_case->app,
                                                          "a"));
    d_tests_arg_command_line_display(_case->out,
                                     "N04n",
                                     _case->app,
                                     true);

    return;
}

/**
 * @brief N05: ignore_case refused a subcommand whose name a sibling holds
 *        in another case; the switch after.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n05(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_command* const sub = d_arg_command_add_subcommand(_case->app,
                                                                   "Abc",
                                                                   NULL);

    (void)d_arg_command_add_subcommand(_case->app,
                                       "abc",
                                       NULL);
    d_arg_parser_clear_definition_error(&_case->run->parser);
    d_tests_arg_command_set_tried(_case,
                                  "N05",
                                  d_arg_command_set_ignore_case(sub,
                                                                true));
    (void)d_arg_buffer_printf(
        _case->out,
        "N05v: %d\n",
        (int)d_arg_command_has_flag(sub,
                                    D_ARG_COMMAND_IGNORE_CASE));

    return;
}

/**
 * @brief N06: ignore_case refused the root, whose name one of its own
 *        subcommands holds in another case; the switch after.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n06(
    struct d_tests_arg_command_case* _case
)
{
    (void)d_arg_command_add_subcommand(_case->app,
                                       "PROG",
                                       NULL);
    d_arg_parser_clear_definition_error(&_case->run->parser);
    d_tests_arg_command_set_tried(_case,
                                  "N06",
                                  d_arg_command_set_ignore_case(_case->app,
                                                                true));
    (void)d_arg_buffer_printf(
        _case->out,
        "N06v: %d\n",
        (int)d_arg_command_has_flag(_case->app,
                                    D_ARG_COMMAND_IGNORE_CASE));

    return;
}

/**
 * @brief N07: ignore_underscore refused a subcommand whose name a sibling
 *        holds without the underscore; the switch after; then turned off,
 *        which nothing refuses.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n07(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_command* const sub = d_arg_command_add_subcommand(_case->app,
                                                                   "a_b",
                                                                   NULL);

    (void)d_arg_command_add_subcommand(_case->app,
                                       "ab",
                                       NULL);
    d_arg_parser_clear_definition_error(&_case->run->parser);
    d_tests_arg_command_set_tried(_case,
                                  "N07",
                                  d_arg_command_set_ignore_underscore(sub,
                                                                      true));
    (void)d_arg_buffer_printf(
        _case->out,
        "N07v: %d\n",
        (int)d_arg_command_has_flag(sub,
                                    D_ARG_COMMAND_IGNORE_UNDERSCORE));
    d_tests_arg_command_set_tried(_case,
                                  "N07b",
                                  d_arg_command_set_ignore_underscore(sub,
                                                                      false));

    return;
}

/**
 * @brief N21: aliases given, cleared, and one given again; the display name
 *        after each.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n21(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_command* const sub = d_arg_command_add_subcommand(_case->app,
                                                                   "s",
                                                                   NULL);

    (void)d_arg_command_add_alias(sub,
                                  "x");
    (void)d_arg_command_add_alias(sub,
                                  "y");
    d_tests_arg_command_line_display(_case->out,
                                     "N21",
                                     sub,
                                     true);
    d_arg_command_clear_aliases(sub);
    d_tests_arg_command_line_display(_case->out,
                                     "N21b",
                                     sub,
                                     true);
    d_arg_parser_clear_definition_error(&_case->run->parser);
    d_tests_arg_command_set_tried(_case,
                                  "N21c",
                                  d_arg_command_add_alias(sub,
                                                          "x"));
    d_tests_arg_command_line_display(_case->out,
                                     "N21d",
                                     sub,
                                     true);

    return;
}

/**
 * @brief N22: a subcommand in a group of its own renamed to nothing, which
 *        makes it nameless: its display name, and its old name looked up.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n22(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_command* const sub = d_arg_command_add_subcommand(_case->app,
                                                                   "s",
                                                                   NULL);

    (void)d_arg_command_set_group(sub,
                                  "Things");
    d_arg_parser_clear_definition_error(&_case->run->parser);
    d_tests_arg_command_set_tried(_case,
                                  "N22",
                                  d_arg_command_set_name(sub,
                                                         ""));
    d_tests_arg_command_line_display(_case->out,
                                     "N22n",
                                     sub,
                                     true);
    (void)d_arg_buffer_printf(_case->out,
                              "N22f: %s\n",
                              (d_arg_command_subcommand(_case->app,
                                                        "s") != NULL)
                              ? "found"
                              : "(null)");

    return;
}

// NAME_CASES
//   constant: NAMES' cases, in the reference's order.
static const d_tests_arg_command_case_fn NAME_CASES[] =
{
    d_tests_arg_command_n01, d_tests_arg_command_n02, d_tests_arg_command_n03,
    d_tests_arg_command_n04, d_tests_arg_command_n05, d_tests_arg_command_n06,
    d_tests_arg_command_n07, d_tests_arg_command_n21, d_tests_arg_command_n22
};

// NAME_COUNT
//   constant: how many cases NAME_CASES holds.
static const size_t NAME_COUNT = sizeof(NAME_CASES) / sizeof(NAME_CASES[0]);

/*
d_tests_arg_command_names
  The names and aliases a command is given and refused, as CLI11's name(),
alias(), ignore_case() and ignore_underscore() give and refuse them.
  Tests the following:
  - a subcommand's name that a sibling holds refused with
    OptionAlreadyAdded, the old name kept; the root renamed freely
  - an alias a sibling holds refused with OptionAlreadyAdded, an empty one
    or one holding a newline with IncorrectConstruction; the root's alias
    checked against nothing; aliases written after the name
  - ignore_case and ignore_underscore refused where they would make a
    subcommand's name match a sibling's -- or the root's one of its own --
    the switch left off; turning one off never refused
  - aliases cleared and given again; a subcommand renamed to nothing becomes
    nameless, written as an option group, and its old name finds nothing
*/
bool
d_tests_arg_command_names(void)
{
    return d_tests_arg_command_cases("NAMES",
                                     NAME_CASES,
                                     NAME_COUNT,
                                     NAMES);
}

/**
 * @brief Appends a command's startup state as the reference writes it:
 *        `D` when disabled by default, `E` when enabled by default, `-`
 *        for neither, in that order.
 *
 * @param[in,out] _out      the transcript.
 * @param[in]     _command  the command.
 */
static void
d_tests_arg_command_put_startup(
    struct d_arg_buffer*        _out,
    const struct d_arg_command* _command
)
{
    (void)d_arg_buffer_printf(
        _out,
        "%c%c",
        (_command->startup == (int)D_ARG_STARTUP_DISABLED) ? 'D' : '-',
        (_command->startup == (int)D_ARG_STARTUP_ENABLED) ? 'E' : '-');

    return;
}

/**
 * @brief N08: a subcommand's startup state after each change: disabled by
 *        default, enabled by default, disabled turned off, enabled turned
 *        off, then disabled again with enabled turned off.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n08(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_command* const sub = d_arg_command_add_subcommand(_case->app,
                                                                   "s",
                                                                   NULL);

    (void)d_arg_buffer_append_cstr(_case->out,
                                   "N08: ");
    d_arg_command_set_disabled_by_default(sub,
                                          true);
    d_tests_arg_command_put_startup(_case->out,
                                    sub);
    d_arg_command_set_enabled_by_default(sub,
                                         true);
    (void)d_arg_buffer_append_char(_case->out,
                                   ' ');
    d_tests_arg_command_put_startup(_case->out,
                                    sub);
    d_arg_command_set_disabled_by_default(sub,
                                          false);
    (void)d_arg_buffer_append_char(_case->out,
                                   ' ');
    d_tests_arg_command_put_startup(_case->out,
                                    sub);
    d_arg_command_set_enabled_by_default(sub,
                                         false);
    (void)d_arg_buffer_append_char(_case->out,
                                   ' ');
    d_tests_arg_command_put_startup(_case->out,
                                    sub);
    d_arg_command_set_disabled_by_default(sub,
                                          true);
    d_arg_command_set_enabled_by_default(sub,
                                         false);
    (void)d_arg_buffer_append_char(_case->out,
                                   ' ');
    d_tests_arg_command_put_startup(_case->out,
                                    sub);
    (void)d_arg_buffer_append_char(_case->out,
                                   '\n');

    return;
}

/**
 * @brief Appends a command's requirements of subcommands or of options as
 *        `<min>,<max>`, after a space when asked.
 *
 * @param[in,out] _out      the transcript.
 * @param[in]     _command  the command.
 * @param[in]     _options  whether the requirement of options, not of
 *                          subcommands.
 * @param[in]     _space    whether a space goes first.
 */
static void
d_tests_arg_command_put_required(
    struct d_arg_buffer*        _out,
    const struct d_arg_command* _command,
    bool                        _options,
    bool                        _space
)
{
    const size_t minimum = (_options)
                           ? _command->require_option_min
                           : _command->require_subcommand_min;
    const size_t maximum = (_options)
                           ? _command->require_option_max
                           : _command->require_subcommand_max;

    (void)d_arg_buffer_printf(_out,
                              "%s%lu,%lu",
                              (_space) ? " " : "",
                              (unsigned long)minimum,
                              (unsigned long)maximum);

    return;
}

/**
 * @brief N09: the requirements after require_subcommand(-3), (2), (),
 *        (2, 5), then require_option(-1), (0), (1, 3).
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n09(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_command* const app = _case->app;

    (void)d_arg_buffer_append_cstr(_case->out,
                                   "N09:");
    d_arg_command_require_subcommand_count(app,
                                           -3);
    d_tests_arg_command_put_required(_case->out,
                                     app,
                                     false,
                                     true);
    d_arg_command_require_subcommand_count(app,
                                           2);
    d_tests_arg_command_put_required(_case->out,
                                     app,
                                     false,
                                     true);
    d_arg_command_require_subcommand(app,
                                     1u,
                                     0u);
    d_tests_arg_command_put_required(_case->out,
                                     app,
                                     false,
                                     true);
    d_arg_command_require_subcommand(app,
                                     2u,
                                     5u);
    d_tests_arg_command_put_required(_case->out,
                                     app,
                                     false,
                                     true);
    d_arg_command_require_option_count(app,
                                       -1);
    d_tests_arg_command_put_required(_case->out,
                                     app,
                                     true,
                                     true);
    d_arg_command_require_option_count(app,
                                       0);
    d_tests_arg_command_put_required(_case->out,
                                     app,
                                     true,
                                     true);
    d_arg_command_require_option(app,
                                 1u,
                                 3u);
    d_tests_arg_command_put_required(_case->out,
                                     app,
                                     true,
                                     true);
    (void)d_arg_buffer_append_char(_case->out,
                                   '\n');

    return;
}

/**
 * @brief N20: configuration extras allowed, then refused, with the mode of
 *        both kinds of extras after each.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n20(
    struct d_tests_arg_command_case* _case
)
{
    d_arg_command_set_allow_config_extras_bool(_case->app,
                                               true);
    (void)d_arg_buffer_printf(_case->out,
                              "N20: %d %d\n",
                              _case->app->allow_config_extras,
                              _case->app->allow_extras);
    d_arg_command_set_allow_config_extras_bool(_case->app,
                                               false);
    (void)d_arg_buffer_printf(_case->out,
                              "N20b: %d %d\n",
                              _case->app->allow_config_extras,
                              _case->app->allow_extras);

    return;
}

/**
 * @brief Appends the switches of a command as the reference's settings()
 *        writes them, each `<name>=0` or `=1`.
 *
 * @param[in,out] _out      the transcript.
 * @param[in]     _command  the command.
 */
static void
d_tests_arg_command_put_switches(
    struct d_arg_buffer*        _out,
    const struct d_arg_command* _command
)
{
    // each switch, in the reference's order
    for (size_t index = 0u; index < SWITCH_COUNT; ++index)
    {
        (void)d_arg_buffer_printf(
            _out,
            "%s%s=%d",
            (index > 0u) ? " " : "",
            SWITCHES[index].name,
            (int)d_arg_command_has_flag(_command,
                                        SWITCHES[index].flag));
    }

    return;
}

/**
 * @brief Appends a line of a command's settings as the reference's
 *        settings() writes them: the switches, the startup state, the
 *        modes of extras, configuration extras and prefix commands, the
 *        group, the usage and footer, and the requirements.
 *
 * @param[in,out] _out      the transcript.
 * @param[in]     _tag      the line's tag.
 * @param[in]     _command  the command.
 */
static void
d_tests_arg_command_line_settings(
    struct d_arg_buffer*        _out,
    const char*                 _tag,
    const struct d_arg_command* _command
)
{
    (void)d_arg_buffer_printf(_out,
                              "%s: ",
                              _tag);
    d_tests_arg_command_put_switches(_out,
                                     _command);
    (void)d_arg_buffer_append_cstr(_out,
                                   " start=");
    d_tests_arg_command_put_startup(_out,
                                    _command);
    (void)d_arg_buffer_printf(_out,
                              " extras=%d cfg=%d prefix=%d group='%s' usage='",
                              _command->allow_extras,
                              _command->allow_config_extras,
                              _command->prefix_command,
                              d_arg_command_group(_command));
    (void)d_arg_command_usage(_command,
                              d_arg_buffer_sink(_out));
    (void)d_arg_buffer_append_cstr(_out,
                                   "' footer='");
    (void)d_arg_command_footer(_command,
                               d_arg_buffer_sink(_out));
    (void)d_arg_buffer_append_cstr(_out,
                                   "' subs=");
    d_tests_arg_command_put_required(_out,
                                     _command,
                                     false,
                                     false);
    (void)d_arg_buffer_append_cstr(_out,
                                   " opts=");
    d_tests_arg_command_put_required(_out,
                                     _command,
                                     true,
                                     false);
    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

/**
 * @brief Turns on, or off, every switch a subcommand may have: required,
 *        disabled, silent, Windows-style options, positionals at the end,
 *        configurable, validated positionals and optional arguments,
 *        non-standard names, prefix matching, fallthrough, ignore_case and
 *        ignore_underscore, and an immediate callback; and the subcommand
 *        fallthrough, off when the others are on.
 *
 * @param[in,out] _command  the command.
 * @param[in]     _value    whether the switches go on.
 */
static void
d_tests_arg_command_switch_all(
    struct d_arg_command* _command,
    bool                  _value
)
{
    d_arg_command_set_required(_command,
                               _value);
    d_arg_command_set_disabled(_command,
                               _value);
    d_arg_command_set_silent(_command,
                             _value);
    d_arg_command_set_windows_style(_command,
                                    _value);
    d_arg_command_set_positionals_at_end(_command,
                                         _value);
    d_arg_command_set_configurable(_command,
                                   _value);
    d_arg_command_set_validate_positionals(_command,
                                           _value);
    d_arg_command_set_validate_optional_arguments(_command,
                                                  _value);
    d_arg_command_set_non_standard_names(_command,
                                         _value);
    d_arg_command_set_prefix_matching(_command,
                                      _value);
    d_arg_command_set_fallthrough(_command,
                                  _value);
    (void)d_arg_command_set_ignore_case(_command,
                                        _value);
    (void)d_arg_command_set_ignore_underscore(_command,
                                              _value);
    d_arg_command_set_immediate_callback(_command,
                                         _value);
    d_arg_command_set_subcommand_fallthrough(_command,
                                             !_value);

    return;
}

/**
 * @brief Gives a command every setting that is not a switch, as N24 sets
 *        them: disabled by default, extras assumed single, configuration
 *        extras ignored, prefix commands on, the group `GRP`, the usage
 *        `U` and the footer `F`, one to four subcommands and two to three
 *        options required.
 *
 * @param[in,out] _command  the command.
 */
static void
d_tests_arg_command_set_rest(
    struct d_arg_command* _command
)
{
    d_arg_command_set_disabled_by_default(_command,
                                          true);
    d_arg_command_set_allow_extras(_command,
                                   (int)D_ARG_EXTRAS_ASSUME_SINGLE);
    d_arg_command_set_allow_config_extras(_command,
                                          (int)D_ARG_CONFIG_EXTRAS_IGNORE);
    d_arg_command_set_prefix_command(_command,
                                     (int)D_ARG_PREFIX_ON);
    (void)d_arg_command_set_group(_command,
                                  "GRP");
    (void)d_arg_command_set_usage(_command,
                                  "U");
    (void)d_arg_command_set_footer(_command,
                                   "F");
    d_arg_command_require_subcommand(_command,
                                     1u,
                                     4u);
    d_arg_command_require_option(_command,
                                 2u,
                                 3u);

    return;
}

/**
 * @brief N24: a root given every setting -- the switches as N25 turns them
 *        on, the rest as set_rest gives them -- and what a subcommand and
 *        an option group made after take of them.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n24(
    struct d_tests_arg_command_case* _case
)
{
    d_tests_arg_command_switch_all(_case->app,
                                   true);
    d_tests_arg_command_set_rest(_case->app);

    struct d_arg_command* const sub   =
        d_arg_command_add_subcommand(_case->app,
                                     "s",
                                     NULL);
    struct d_arg_command* const group =
        d_arg_command_add_option_group(_case->app,
                                       "G",
                                       NULL);

    d_tests_arg_command_line_settings(_case->out,
                                      "N24r",
                                      _case->app);
    d_tests_arg_command_line_settings(_case->out,
                                      "N24s",
                                      sub);
    d_tests_arg_command_line_settings(_case->out,
                                      "N24g",
                                      group);

    return;
}

/**
 * @brief N25: a subcommand's switches all turned on, with its extras
 *        captured and prefix commands on; then all turned off again.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n25(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_command* const sub = d_arg_command_add_subcommand(_case->app,
                                                                   "s",
                                                                   NULL);

    d_tests_arg_command_switch_all(sub,
                                   true);
    d_arg_command_set_subcommand_fallthrough(sub,
                                             true);
    d_arg_command_set_allow_extras(sub,
                                   (int)D_ARG_EXTRAS_CAPTURE);
    d_arg_command_set_prefix_command(sub,
                                     (int)D_ARG_PREFIX_ON);
    d_tests_arg_command_line_settings(_case->out,
                                      "N25",
                                      sub);
    d_tests_arg_command_switch_all(sub,
                                   false);
    d_arg_command_set_allow_extras(sub,
                                   (int)D_ARG_EXTRAS_ERROR);
    d_arg_command_set_prefix_command(sub,
                                     (int)D_ARG_PREFIX_OFF);
    d_tests_arg_command_line_settings(_case->out,
                                      "N25b",
                                      sub);

    return;
}

/**
 * @brief N24d: a root's settings as made.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n24d(
    struct d_tests_arg_command_case* _case
)
{
    d_tests_arg_command_line_settings(_case->out,
                                      "N24d",
                                      _case->app);

    return;
}

// DEFAULT_CASES
//   constant: DEFAULTS' one case.
static const d_tests_arg_command_case_fn DEFAULT_CASES[] =
{
    d_tests_arg_command_n24d
};

// SETTING_CASES
//   constant: SETTINGS' cases, in the reference's order.
static const d_tests_arg_command_case_fn SETTING_CASES[] =
{
    d_tests_arg_command_n08, d_tests_arg_command_n09, d_tests_arg_command_n20,
    d_tests_arg_command_n24, d_tests_arg_command_n25
};

// SETTING_COUNT
//   constant: how many cases SETTING_CASES holds.
static const size_t SETTING_COUNT =
    sizeof(SETTING_CASES) / sizeof(SETTING_CASES[0]);

/*
d_tests_arg_command_settings
  A command's settings, as CLI11's setters set them and its subcommands take
them.
  Tests the following:
  - the startup states: disabled or enabled by default, one turning off
    the other only when it is set, and neither turned off
  - require_subcommand and require_option with a count -- exactly n, at
    most -n, any for 0 -- and with a range
  - allow_config_extras(true) capturing both kinds of extras, false making
    configuration extras errors and leaving the others
  - a root's settings as made -- allowing Windows-style options where
    the module's setting for them says so, on Windows by default, as CLI11
    does -- and a subcommand made after the root is given every setting
    takes its switches but for required, disabled, silent,
    positionals at the end, non-standard names and subcommand
    fallthrough, its modes, group, usage, footer and the most subcommands
    required, but not its startup state or other requirements; an option
    group the same, but in its own group, without the footer, and not
    falling through
  - every switch of a subcommand turned on, and off again
*/
bool
d_tests_arg_command_settings(void)
{
    const char* const defaults = (D_INTERNAL_ARG_PARSER_WINDOWS_STYLE == 1)
                                 ? DEFAULTS_WINDOWS
                                 : DEFAULTS;
    const bool        made     = d_tests_arg_command_cases("DEFAULTS",
                                                           DEFAULT_CASES,
                                                           1u,
                                                           defaults);
    const bool        set      = d_tests_arg_command_cases("SETTINGS",
                                                           SETTING_CASES,
                                                           SETTING_COUNT,
                                                           SETTINGS);

    return ( (made) &&
             (set) );
}

/**
 * @brief Writes the text a usage, footer or version function of USAGE_TEXT
 *        gives: its context's.
 *
 * @param[in] _command  the command; unused.
 * @param[in] _sink     where the text goes.
 * @param[in] _context  the text, a string.
 * @return the sink's status.
 */
static int
d_tests_arg_command_write_words(
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
 * @brief Appends a line `<tag>: <usage>|<footer>`, a command's usage and
 *        footer as CLI11's get_usage and get_footer give them.
 *
 * @param[in,out] _out      the transcript.
 * @param[in]     _tag      the line's tag.
 * @param[in]     _command  the command.
 */
static void
d_tests_arg_command_line_text(
    struct d_arg_buffer*        _out,
    const char*                 _tag,
    const struct d_arg_command* _command
)
{
    (void)d_arg_buffer_printf(_out,
                              "%s: ",
                              _tag);
    (void)d_arg_command_usage(_command,
                              d_arg_buffer_sink(_out));
    (void)d_arg_buffer_append_char(_out,
                                   '|');
    (void)d_arg_command_footer(_command,
                               d_arg_buffer_sink(_out));
    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

/**
 * @brief N13: a usage and footer of fixed text, then with functions before
 *        them, and what a subcommand made after takes: the text alone.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n13(
    struct d_tests_arg_command_case* _case
)
{
    char usage[]  = "UC";
    char footer[] = "FC";

    (void)d_arg_command_set_usage(_case->app,
                                  "U");
    (void)d_arg_command_set_footer(_case->app,
                                   "F");
    d_tests_arg_command_line_text(_case->out,
                                  "N13",
                                  _case->app);
    d_arg_command_set_usage_fn(_case->app,
                               d_tests_arg_command_write_words,
                               usage,
                               NULL);
    d_arg_command_set_footer_fn(_case->app,
                                d_tests_arg_command_write_words,
                                footer,
                                NULL);
    d_tests_arg_command_line_text(_case->out,
                                  "N13b",
                                  _case->app);
    d_tests_arg_command_line_text(_case->out,
                                  "N13c",
                                  d_arg_command_add_subcommand(_case->app,
                                                               "s",
                                                               NULL));
    d_arg_command_set_usage_fn(_case->app,
                               NULL,
                               NULL,
                               NULL);
    d_arg_command_set_footer_fn(_case->app,
                                NULL,
                                NULL,
                                NULL);

    return;
}

/**
 * @brief N14: a version flag whose text a function gives, then the flag
 *        removed; the version shown after each.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n14(
    struct d_tests_arg_command_case* _case
)
{
    char version[] = "V9";

    (void)d_arg_command_set_version_fn(_case->app,
                                       "--ver",
                                       d_tests_arg_command_write_words,
                                       version,
                                       NULL,
                                       NULL);
    d_tests_arg_command_line_version(_case->out,
                                     "N14",
                                     _case->app);
    (void)d_arg_command_set_version_flag(_case->app,
                                         "",
                                         NULL,
                                         NULL);
    (void)d_arg_buffer_append_cstr(_case->out,
                                   "N14b: '");
    (void)d_arg_command_version(_case->app,
                                d_arg_buffer_sink(_case->out));
    (void)d_arg_buffer_append_cstr(_case->out,
                                   "'\n");

    return;
}

/**
 * @brief N29: a subcommand's description replaced and its group set, then
 *        its group emptied.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n29(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_command* const sub = d_arg_command_add_subcommand(_case->app,
                                                                   "s",
                                                                   "first");

    (void)d_arg_command_set_description(sub,
                                        "second");
    (void)d_arg_command_set_group(sub,
                                  "G1");
    (void)d_arg_buffer_printf(_case->out,
                              "N29: %s|%s\n",
                              d_arg_command_description(sub),
                              d_arg_command_group(sub));
    (void)d_arg_command_set_group(sub,
                                  "");
    (void)d_arg_buffer_printf(_case->out,
                              "N29b: '%s'\n",
                              d_arg_command_group(sub));

    return;
}

// TEXT_CASES
//   constant: USAGE_TEXT's cases, in the reference's order.
static const d_tests_arg_command_case_fn TEXT_CASES[] =
{
    d_tests_arg_command_n13, d_tests_arg_command_n14, d_tests_arg_command_n29
};

// TEXT_COUNT
//   constant: how many cases TEXT_CASES holds.
static const size_t TEXT_COUNT = sizeof(TEXT_CASES) / sizeof(TEXT_CASES[0]);

/*
d_tests_arg_command_usage_text
  The text a command shows about itself: its usage, footer, version,
description and group.
  Tests the following:
  - a usage and a footer of fixed text, and with a function's text before
    each, on a line of its own; a subcommand takes the fixed text, not the
    functions
  - a version a function gives, and none once the flag is removed
  - a description replaced; a group set, then emptied
*/
bool
d_tests_arg_command_usage_text(void)
{
    return d_tests_arg_command_cases("USAGE_TEXT",
                                     TEXT_CASES,
                                     TEXT_COUNT,
                                     USAGE_TEXT);
}

// d_tests_arg_command_called
//   struct: what a callback of CALLBACKS notes: the run, and the words that
// name the callback.
struct d_tests_arg_command_called
{
    struct d_tests_arg_run* run;
    const char*             words;
};

/**
 * @brief Notes a callback's words; CALLBACKS' callbacks.
 *
 * @param[in] _command  the command; unused.
 * @param[in] _context  the d_tests_arg_command_called.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_command_note_called(
    struct d_arg_command* _command,
    void*                 _context
)
{
    const struct d_tests_arg_command_called* const called = _context;

    (void)_command;

    d_tests_arg_note(called->run,
                     "%s",
                     called->words);

    return D_ARG_OK;
}

/**
 * @brief Notes a pre-parse callback's words and how many arguments are
 *        left; CALLBACKS' pre-parse callback.
 *
 * @param[in] _command    the command; unused.
 * @param[in] _remaining  how many arguments are left.
 * @param[in] _context    the d_tests_arg_command_called.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_command_note_preparse(
    struct d_arg_command* _command,
    size_t                _remaining,
    void*                 _context
)
{
    const struct d_tests_arg_command_called* const called = _context;

    (void)_command;

    d_tests_arg_note(called->run,
                     "%s %lu",
                     called->words,
                     (unsigned long)_remaining);

    return D_ARG_OK;
}

// d_tests_arg_command_call
//   enum: CALLBACKS' callbacks, by the index of their words: the root's
// callback and parse-complete callback; s's callback; t's callback and
// final callback; u's final and parse-complete callbacks; s's pre-parse
// callback; p's pre_callback and callback; and how many there are.
enum d_tests_arg_command_call
{
    CALL_APP          = 0,
    CALL_APP_COMPLETE = 1,
    CALL_S            = 2,
    CALL_T            = 3,
    CALL_T_FINAL      = 4,
    CALL_U_FINAL      = 5,
    CALL_U_COMPLETE   = 6,
    CALL_S_PREPARSE   = 7,
    CALL_P_PRE        = 8,
    CALL_P            = 9,
    CALL_COUNT        = 10
};

/**
 * @brief Gives t and u their callbacks, as CALLBACKS does: t a callback
 *        made the parse-complete one by immediate_callback, then a final
 *        one, then immediate_callback turned off with both set; u a final
 *        and a parse-complete callback, then immediate_callback turned on.
 *
 * @param[in,out] _app     the root.
 * @param[in,out] _called  CALL_COUNT records, by d_tests_arg_command_call.
 */
static void
d_tests_arg_command_call_late(
    struct d_arg_command*              _app,
    struct d_tests_arg_command_called* _called
)
{
    struct d_arg_command* const t = d_arg_command_add_subcommand(_app,
                                                                 "t",
                                                                 NULL);
    struct d_arg_command* const u = d_arg_command_add_subcommand(_app,
                                                                 "u",
                                                                 NULL);

    d_arg_command_set_immediate_callback(t,
                                         true);
    d_arg_command_set_callback(t,
                               d_tests_arg_command_note_called,
                               &_called[CALL_T],
                               NULL);
    d_arg_command_set_final_callback(t,
                                     d_tests_arg_command_note_called,
                                     &_called[CALL_T_FINAL],
                                     NULL);
    d_arg_command_set_immediate_callback(t,
                                         false);
    d_arg_command_set_final_callback(u,
                                     d_tests_arg_command_note_called,
                                     &_called[CALL_U_FINAL],
                                     NULL);
    d_arg_command_set_parse_complete_callback(u,
                                              d_tests_arg_command_note_called,
                                              &_called[CALL_U_COMPLETE],
                                              NULL);
    d_arg_command_set_immediate_callback(u,
                                         true);

    return;
}

/**
 * @brief Gives CALLBACKS' commands their callbacks: the root a callback and
 *        a parse-complete one; s a callback, then immediate_callback, and a
 *        pre-parse callback; t and u theirs; and p a pre_callback and a
 *        callback.
 *
 * @param[in,out] _app     the root.
 * @param[in,out] _called  CALL_COUNT records, by d_tests_arg_command_call.
 */
static void
d_tests_arg_command_call_back(
    struct d_arg_command*              _app,
    struct d_tests_arg_command_called* _called
)
{
    d_arg_command_set_callback(_app,
                               d_tests_arg_command_note_called,
                               &_called[CALL_APP],
                               NULL);
    d_arg_command_set_parse_complete_callback(_app,
                                              d_tests_arg_command_note_called,
                                              &_called[CALL_APP_COMPLETE],
                                              NULL);

    struct d_arg_command* const s = d_arg_command_add_subcommand(_app,
                                                                 "s",
                                                                 NULL);

    d_arg_command_set_callback(s,
                               d_tests_arg_command_note_called,
                               &_called[CALL_S],
                               NULL);
    d_arg_command_set_immediate_callback(s,
                                         true);
    d_tests_arg_command_call_late(_app,
                                  _called);
    d_arg_command_set_preparse_callback(s,
                                        d_tests_arg_command_note_preparse,
                                        &_called[CALL_S_PREPARSE],
                                        NULL);

    struct d_arg_command* const p = d_arg_command_add_subcommand(_app,
                                                                 "p",
                                                                 NULL);

    d_arg_command_set_pre_callback(p,
                                   d_tests_arg_command_note_called,
                                   &_called[CALL_P_PRE],
                                   NULL);
    d_arg_command_set_callback(p,
                               d_tests_arg_command_note_called,
                               &_called[CALL_P],
                               NULL);

    return;
}

/*
d_tests_arg_command_callbacks
  A command's callbacks in parses, as CLI11 runs them: the callback,
final and parse-complete callbacks, the pre-parse callback, and the
pre_callback a subclass of CLI11's App would give.
  Tests the following:
  - callback() sets the final callback, or the parse-complete one under
    immediate_callback; immediate_callback moves a lone callback between
    the two, and leaves them when both are set
  - parse-complete callbacks run as each command's arguments end, the
    root's after its subcommands'; final callbacks after the whole command
    line, each command's pre_callback first, the root's last
  - the pre-parse callback runs when its command is met, given how many
    arguments are left; a parse with no subcommands runs only the root's
*/
bool
d_tests_arg_command_callbacks(void)
{
    struct d_tests_arg_run            run                = { 0 };
    struct d_arg_command* const       app                =
        d_tests_arg_begin(&run);
    struct d_tests_arg_command_called called[CALL_COUNT] =
    {
        { &run, "app callback" }, { &run, "app complete" },
        { &run, "s callback"   }, { &run, "t callback"   },
        { &run, "t final"      }, { &run, "u final"      },
        { &run, "u complete"   }, { &run, "s preparse"   },
        { &run, "p pre"        }, { &run, "p callback"   }
    };

    d_tests_arg_command_call_back(app,
                                  called);

    // each parse
    for (size_t index = 0u; index < CALLBACK_COUNT; ++index)
    {
        d_tests_arg_command_parse_listed(&run,
                                         CALLBACK_ARGS[index].list);
    }

    const bool same = d_tests_arg_matches(&run,
                                          CALLBACKS);

    d_tests_arg_end(&run);

    return same;
}

/**
 * @brief N10: needs and excludes refused for NULL and for the command
 *        itself.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n10(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_command* const sub = d_arg_command_add_subcommand(_case->app,
                                                                   "s",
                                                                   NULL);

    d_arg_parser_clear_definition_error(&_case->run->parser);
    d_tests_arg_command_set_tried(_case,
                                  "N10",
                                  d_arg_command_excludes_command(sub,
                                                                 NULL));
    d_tests_arg_command_set_tried(_case,
                                  "N10b",
                                  d_arg_command_excludes_command(sub,
                                                                 sub));
    d_tests_arg_command_set_tried(_case,
                                  "N10c",
                                  d_arg_command_needs_command(sub,
                                                              sub));
    d_tests_arg_command_set_tried(_case,
                                  "N10d",
                                  d_arg_command_excludes_option(sub,
                                                                NULL));
    d_tests_arg_command_set_tried(_case,
                                  "N10e",
                                  d_arg_command_needs_option(sub,
                                                             NULL));
    d_tests_arg_command_set_tried(_case,
                                  "N10f",
                                  d_arg_command_needs_command(sub,
                                                              NULL));

    return;
}

/**
 * @brief N26: an exclusion made twice, then removed from the other side,
 *        and again from this one.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n26(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_command* const first  =
        d_arg_command_add_subcommand(_case->app,
                                     "s1",
                                     NULL);
    struct d_arg_command* const second =
        d_arg_command_add_subcommand(_case->app,
                                     "s2",
                                     NULL);

    d_tests_arg_command_set_tried(_case,
                                  "N26",
                                  d_arg_command_excludes_command(first,
                                                                 second));
    d_tests_arg_command_set_tried(_case,
                                  "N26b",
                                  d_arg_command_excludes_command(first,
                                                                 second));

    const bool removed = d_arg_command_remove_excludes_command(second,
                                                               first);
    const bool again   = d_arg_command_remove_excludes_command(first,
                                                               second);

    (void)d_arg_buffer_printf(_case->out,
                              "N26r: %d %d\n",
                              (int)removed,
                              (int)again);

    return;
}

/**
 * @brief N27: a need made twice, then removed from the other side, which
 *        holds none, from this side, and again.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n27(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_command* const first  =
        d_arg_command_add_subcommand(_case->app,
                                     "s1",
                                     NULL);
    struct d_arg_command* const second =
        d_arg_command_add_subcommand(_case->app,
                                     "s2",
                                     NULL);

    d_tests_arg_command_set_tried(_case,
                                  "N27",
                                  d_arg_command_needs_command(first,
                                                              second));
    d_tests_arg_command_set_tried(_case,
                                  "N27b",
                                  d_arg_command_needs_command(first,
                                                              second));

    const bool back    = d_arg_command_remove_needs_command(second,
                                                            first);
    const bool removed = d_arg_command_remove_needs_command(first,
                                                            second);
    const bool again   = d_arg_command_remove_needs_command(first,
                                                            second);

    (void)d_arg_buffer_printf(_case->out,
                              "N27r: %d %d %d\n",
                              (int)back,
                              (int)removed,
                              (int)again);

    return;
}

/**
 * @brief N28: a command needing and excluding an option, each removed
 *        twice.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n28(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_command* const sub    =
        d_arg_command_add_subcommand(_case->app,
                                     "s",
                                     NULL);
    struct d_arg_option* const  option =
        d_arg_command_add_option(_case->app,
                                 "--o",
                                 NULL);

    d_tests_arg_command_set_tried(_case,
                                  "N28",
                                  d_arg_command_needs_option(sub,
                                                             option));
    d_tests_arg_command_set_tried(_case,
                                  "N28b",
                                  d_arg_command_excludes_option(sub,
                                                                option));

    const bool needed         = d_arg_command_remove_needs_option(sub,
                                                                  option);
    const bool needed_again   = d_arg_command_remove_needs_option(sub,
                                                                  option);
    const bool excluded       = d_arg_command_remove_excludes_option(sub,
                                                                     option);
    const bool excluded_again = d_arg_command_remove_excludes_option(sub,
                                                                     option);

    (void)d_arg_buffer_printf(_case->out,
                              "N28r: %d %d %d %d\n",
                              (int)needed,
                              (int)needed_again,
                              (int)excluded,
                              (int)excluded_again);

    return;
}

// RELATION_CASES
//   constant: RELATIONS' cases, in the reference's order.
static const d_tests_arg_command_case_fn RELATION_CASES[] =
{
    d_tests_arg_command_n10, d_tests_arg_command_n26, d_tests_arg_command_n27,
    d_tests_arg_command_n28
};

// RELATION_CASE_COUNT
//   constant: how many cases RELATION_CASES holds.
static const size_t RELATION_CASE_COUNT =
    sizeof(RELATION_CASES) / sizeof(RELATION_CASES[0]);

/*
d_tests_arg_command_relations
  Needs and excludes between commands and of options, as CLI11's needs(),
excludes(), remove_needs() and remove_excludes() make and remove them.
  Tests the following:
  - NULL refused with OptionNotFound, "nullptr passed", and the command
    itself with "cannot self reference in excludes" (or "needs")
  - a link made twice is held once: an exclusion between commands is held
    on both, and removed from either side removes both; a need is held on
    one side only
  - a command's need and exclusion of an option, each removed once
*/
bool
d_tests_arg_command_relations(void)
{
    return d_tests_arg_command_cases("RELATIONS",
                                     RELATION_CASES,
                                     RELATION_CASE_COUNT,
                                     RELATIONS);
}

/*
d_tests_arg_command_relation_parses
  Needs and excludes of commands in parses.
  Tests the following:
  - a subcommand excluding another, given with it in either order, ends the
    parse with ExcludesError, "s1 excludes s2"
  - a subcommand needing another, or an option, given without it, ends the
    parse with RequiresError, "s3 requires s2"; with it, the parse goes on
  - a subcommand excluding an option given beside it ends the parse with
    ExcludesError, naming the option; the excluding subcommand alone is
    no error
*/
bool
d_tests_arg_command_relation_parses(void)
{
    struct d_tests_arg_run      run    = { 0 };
    struct d_arg_command* const app    = d_tests_arg_begin(&run);
    struct d_arg_command* const first  = d_arg_command_add_subcommand(app,
                                                                      "s1",
                                                                      NULL);
    struct d_arg_command* const second = d_arg_command_add_subcommand(app,
                                                                      "s2",
                                                                      NULL);
    struct d_arg_command* const third  = d_arg_command_add_subcommand(app,
                                                                      "s3",
                                                                      NULL);
    struct d_arg_command* const fourth = d_arg_command_add_subcommand(app,
                                                                      "s4",
                                                                      NULL);
    struct d_arg_option* const  option =
        d_tests_arg_watch_option(&run,
                                 "o",
                                 d_arg_command_add_option(app,
                                                          "--o",
                                                          NULL));

    (void)d_arg_command_excludes_command(first,
                                         second);
    (void)d_arg_command_needs_command(third,
                                      second);
    (void)d_arg_command_needs_option(fourth,
                                     option);
    (void)d_arg_command_excludes_option(second,
                                        option);

    // each parse
    for (size_t index = 0u; index < RELATION_COUNT; ++index)
    {
        d_tests_arg_command_parse_listed(&run,
                                         RELATION_ARGS[index].list);
    }

    const bool same = d_tests_arg_matches(&run,
                                          RELATION_PARSES);

    d_tests_arg_end(&run);

    return same;
}

//==============================================================================
// 4.  QUERIES
//==============================================================================

/**
 * @brief Appends CHECKS' line under one setting, in a parser of its own:
 *        the subcommand `Sub_Cmd`, with the alias `al_ias`, asked each of
 *        CHECKED_NAMES, `<name>=<detail>/<check>`.
 *
 * @param[in,out] _out      the transcript.
 * @param[in]     _setting  a d_tests_arg_command_check_setting value.
 */
static void
d_tests_arg_command_checked(
    struct d_arg_buffer* _out,
    int                  _setting
)
{
    struct d_tests_arg_run      run = { 0 };
    struct d_arg_command* const sub =
        d_arg_command_add_subcommand(d_tests_arg_begin(&run),
                                     "Sub_Cmd",
                                     NULL);

    (void)d_arg_command_add_alias(sub,
                                  "al_ias");
    (void)d_arg_command_set_ignore_case(sub,
                                        ( (_setting == CHECK_CASE) ||
                                          (_setting == CHECK_BOTH) ));
    (void)d_arg_command_set_ignore_underscore(
              sub,
              ( (_setting == CHECK_UNDERSCORE) ||
                (_setting == CHECK_BOTH) ));
    d_arg_command_set_prefix_matching(sub,
                                      (_setting == CHECK_PREFIX));
    (void)d_arg_buffer_printf(_out,
                              "C%d:",
                              _setting);

    // each name, and both answers
    for (size_t index = 0u; index < CHECKED_COUNT; ++index)
    {
        const struct d_pack_text name =
            d_pack_text_from_cstr(CHECKED_NAMES[index]);

        (void)d_arg_buffer_printf(
            _out,
            " %s=%d/%d",
            CHECKED_NAMES[index],
            d_arg_command_check_name_detail(sub,
                                            name),
            (int)d_arg_command_check_name(sub,
                                          name));
    }

    (void)d_arg_buffer_append_char(_out,
                                   '\n');
    d_tests_arg_end(&run);

    return;
}

/*
d_tests_arg_command_checks
  The names a subcommand answers to, as CLI11's check_name and
check_name_detail answer.
  Tests the following:
  - its name and alias exactly, but no other case or spelling
  - ignore_case: any case of either; ignore_underscore: either with its
    underscores removed, on both sides; both together
  - prefix matching: a prefix of either answers as a prefix, and is a
    match; the whole name stays an exact one
*/
bool
d_tests_arg_command_checks(void)
{
    struct d_arg_buffer out = D_ARG_BUFFER_INIT;

    // each setting, in a parser of its own
    for (int setting = 0; setting < (int)CHECK_SETTINGS; ++setting)
    {
        d_tests_arg_command_checked(&out,
                                    setting);
    }

    const bool same = d_tests_arg_same("CHECKS",
                                       d_arg_buffer_text(&out),
                                       CHECKS);

    d_arg_buffer_release(&out);

    return same;
}

/**
 * @brief N11: display names, with and without aliases, of a subcommand with
 *        two, of an option group, and of the root.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n11(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_command* const sub   =
        d_arg_command_add_subcommand(_case->app,
                                     "s",
                                     NULL);
    struct d_arg_command* const group =
        d_arg_command_add_option_group(_case->app,
                                       "G",
                                       NULL);

    (void)d_arg_command_add_alias(sub,
                                  "x");
    (void)d_arg_command_add_alias(sub,
                                  "y");
    (void)d_arg_buffer_append_cstr(_case->out,
                                   "N11: ");
    (void)d_arg_command_display_name(sub,
                                     true,
                                     _case->out);
    (void)d_arg_buffer_append_char(_case->out,
                                   '|');
    (void)d_arg_command_display_name(sub,
                                     false,
                                     _case->out);
    (void)d_arg_buffer_append_char(_case->out,
                                   '|');
    (void)d_arg_command_display_name(group,
                                     true,
                                     _case->out);
    (void)d_arg_buffer_append_char(_case->out,
                                   '|');
    (void)d_arg_command_display_name(_case->app,
                                     true,
                                     _case->out);
    (void)d_arg_buffer_append_char(_case->out,
                                   '\n');

    return;
}

/**
 * @brief Appends one group of a command's options in brackets;
 *        d_arg_command_each_group's visitor.
 *
 * @param[in] _group    the group's name.
 * @param[in] _context  the transcript.
 * @return the buffer's status.
 */
static int
d_tests_arg_command_put_group(
    struct d_pack_text _group,
    void*              _context
)
{
    return d_arg_buffer_printf((struct d_arg_buffer*)_context,
                               "[%.*s]",
                               (int)_group.length,
                               (_group.data != NULL) ? _group.data : "");
}

/**
 * @brief Appends a line `<tag>: [<group>]...`, the groups of a command's own
 *        options, in the order first met.
 *
 * @param[in,out] _out      the transcript.
 * @param[in]     _tag      the line's tag.
 * @param[in]     _command  the command.
 */
static void
d_tests_arg_command_line_groups(
    struct d_arg_buffer*        _out,
    const char*                 _tag,
    const struct d_arg_command* _command
)
{
    (void)d_arg_buffer_printf(_out,
                              "%s: ",
                              _tag);
    (void)d_arg_command_each_group(_command,
                                   d_tests_arg_command_put_group,
                                   _out);
    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

/**
 * @brief N12: the groups of options in B, the default group, A, B again,
 *        and the empty group, after the help flag's.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n12(
    struct d_tests_arg_command_case* _case
)
{
    static const char* const DEFINED[] =
    {
        "--a", "--b", "--c", "--d", "--e"
    };
    static const char* const GROUPS[] =
    {
        "B", NULL, "A", "B", ""
    };

    // each option, and its group
    for (size_t index = 0u; index < (sizeof(DEFINED) / sizeof(DEFINED[0]));
         ++index)
    {
        struct d_arg_option* const option =
            d_arg_command_add_option(_case->app,
                                     DEFINED[index],
                                     NULL);

        // a group of its own
        if (GROUPS[index] != NULL)
        {
            (void)d_arg_option_set_group(option,
                                         GROUPS[index]);
        }
    }

    d_tests_arg_command_line_groups(_case->out,
                                    "N12",
                                    _case->app);

    return;
}

/**
 * @brief N12b: the groups of a command without options, its help flag
 *        removed.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n12b(
    struct d_tests_arg_command_case* _case
)
{
    (void)d_arg_command_set_help_flag(_case->app,
                                      "",
                                      NULL);
    d_tests_arg_command_line_groups(_case->out,
                                    "N12b",
                                    _case->app);

    return;
}

// DISPLAY_CASES
//   constant: DISPLAY's cases, in the reference's order.
static const d_tests_arg_command_case_fn DISPLAY_CASES[] =
{
    d_tests_arg_command_n11, d_tests_arg_command_n12, d_tests_arg_command_n12b
};

// DISPLAY_COUNT
//   constant: how many cases DISPLAY_CASES holds.
static const size_t DISPLAY_COUNT =
    sizeof(DISPLAY_CASES) / sizeof(DISPLAY_CASES[0]);

/*
d_tests_arg_command_display
  How commands are named in text, and the groups of their options, as
CLI11's get_display_name and get_groups give them.
  Tests the following:
  - a subcommand's name, followed by its aliases when asked; an option
    group as `[Option Group: <group>]`; the root by its name
  - the groups of a command's options, each once, in the order first met,
    the empty group among them; none for a command without options
*/
bool
d_tests_arg_command_display(void)
{
    return d_tests_arg_command_cases("DISPLAY",
                                     DISPLAY_CASES,
                                     DISPLAY_COUNT,
                                     DISPLAY);
}

// d_tests_arg_command_tree
//   struct: the commands PARSED asks about: the root, its subcommands s and
// u, t below s, and the option group G.
struct d_tests_arg_command_tree
{
    struct d_arg_command* app;
    struct d_arg_command* s;
    struct d_arg_command* t;
    struct d_arg_command* u;
    struct d_arg_command* group;
};

/**
 * @brief Appends one argument a command did not use, after a bar when it
 *        is not the first; d_arg_command_each_remaining's visitor.
 *
 * @param[in] _extra    the argument.
 * @param[in] _context  the d_tests_arg_command_listing it is written to.
 * @return the buffer's status.
 */
static int
d_tests_arg_command_list_extra(
    const struct d_arg_extra* _extra,
    void*                     _context
)
{
    struct d_tests_arg_command_listing* const listing = _context;

    // a bar between arguments
    if (listing->count > 0u)
    {
        (void)d_arg_buffer_append_char(listing->out,
                                       '|');
    }

    listing->count += 1u;

    return d_arg_buffer_append_text(listing->out,
                                    _extra->text);
}

/**
 * @brief Appends the arguments a command did not use, as CLI11's
 *        remaining() lists them: `[<argument>|...]`.
 *
 * @param[in,out] _out      the transcript.
 * @param[in]     _command  the command.
 * @param[in]     _recurse  whether those of its groups and subcommands
 *                          follow.
 */
static void
d_tests_arg_command_put_remaining(
    struct d_arg_buffer*        _out,
    const struct d_arg_command* _command,
    bool                        _recurse
)
{
    struct d_tests_arg_command_listing listing = { _out, 0u };

    (void)d_arg_buffer_append_char(_out,
                                   '[');
    (void)d_arg_command_each_remaining(_command,
                                       _recurse,
                                       d_tests_arg_command_list_extra,
                                       &listing);
    (void)d_arg_buffer_append_char(_out,
                                   ']');

    return;
}

/**
 * @brief Notes the arguments PARSED's commands did not use: the root's
 *        alone and with its subcommands', s's with t's, and how many of
 *        each.
 *
 * @param[in,out] _run   the run.
 * @param[in]     _tree  the commands.
 */
static void
d_tests_arg_command_note_remaining(
    struct d_tests_arg_run*                _run,
    const struct d_tests_arg_command_tree* _tree
)
{
    struct d_arg_buffer* const out = &_run->transcript;

    (void)d_arg_buffer_append_cstr(out,
                                   "remaining app=");
    d_tests_arg_command_put_remaining(out,
                                      _tree->app,
                                      false);
    (void)d_arg_buffer_append_cstr(out,
                                   " all=");
    d_tests_arg_command_put_remaining(out,
                                      _tree->app,
                                      true);
    (void)d_arg_buffer_append_cstr(out,
                                   " s=");
    d_tests_arg_command_put_remaining(out,
                                      _tree->s,
                                      true);
    (void)d_arg_buffer_printf(
        out,
        " size app=%lu all=%lu s=%lu\n",
        (unsigned long)d_arg_command_remaining_size(_tree->app,
                                                    false),
        (unsigned long)d_arg_command_remaining_size(_tree->app,
                                                    true),
        (unsigned long)d_arg_command_remaining_size(_tree->s,
                                                    true));

    return;
}

/**
 * @brief Notes what PARSED asks after a parse: how often each command was
 *        used and the values below it; which subcommands each used, by
 *        command and by name; and the arguments left.
 *
 * @param[in,out] _run   the run.
 * @param[in]     _tree  the commands.
 */
static void
d_tests_arg_command_note_parsed(
    struct d_tests_arg_run*                _run,
    const struct d_tests_arg_command_tree* _tree
)
{
    (void)d_arg_buffer_printf(
        &_run->transcript,
        "count app=%lu s=%lu t=%lu u=%lu G=%lu all app=%lu s=%lu t=%lu "
        "G=%lu\n",
        (unsigned long)d_arg_command_count(_tree->app),
        (unsigned long)d_arg_command_count(_tree->s),
        (unsigned long)d_arg_command_count(_tree->t),
        (unsigned long)d_arg_command_count(_tree->u),
        (unsigned long)d_arg_command_count(_tree->group),
        (unsigned long)d_arg_command_count_all(_tree->app),
        (unsigned long)d_arg_command_count_all(_tree->s),
        (unsigned long)d_arg_command_count_all(_tree->t),
        (unsigned long)d_arg_command_count_all(_tree->group));
    (void)d_arg_buffer_printf(
        &_run->transcript,
        "got app.s=%d app.t=%d app.u=%d s.t=%d name s=%d t=%d nope=%d\n",
        (int)d_arg_command_got_subcommand(_tree->app,
                                          _tree->s),
        (int)d_arg_command_got_subcommand(_tree->app,
                                          _tree->t),
        (int)d_arg_command_got_subcommand(_tree->app,
                                          _tree->u),
        (int)d_arg_command_got_subcommand(_tree->s,
                                          _tree->t),
        (int)d_arg_command_got_subcommand_name(_tree->app,
                                               "s"),
        (int)d_arg_command_got_subcommand_name(_tree->app,
                                               "t"),
        (int)d_arg_command_got_subcommand_name(_tree->app,
                                               "nope"));
    d_tests_arg_command_note_remaining(_run,
                                       _tree);

    return;
}

/**
 * @brief Makes PARSED's commands: the root with `--a`, its group G with
 *        `--g`, s with `--x`, t below s with the flag `--y`, and u; the
 *        root, s and t capture what they do not use; s and t are reported
 *        beside the root.
 *
 * @param[in,out] _run   the run.
 * @param[out]    _tree  receives the commands.
 */
static void
d_tests_arg_command_grow(
    struct d_tests_arg_run*          _run,
    struct d_tests_arg_command_tree* _tree
)
{
    struct d_arg_command* const app = &_run->parser.command;

    _tree->app   = app;
    _tree->s     = d_tests_arg_watch_command(
                       _run,
                       "s",
                       d_arg_command_add_subcommand(app,
                                                    "s",
                                                    NULL));
    _tree->t     = d_tests_arg_watch_command(
                       _run,
                       "t",
                       d_arg_command_add_subcommand(_tree->s,
                                                    "t",
                                                    NULL));
    _tree->u     = d_arg_command_add_subcommand(app,
                                                "u",
                                                NULL);
    _tree->group = d_arg_command_add_option_group(app,
                                                  "G",
                                                  NULL);
    (void)d_arg_command_add_option(app,
                                   "--a",
                                   NULL);
    (void)d_arg_command_add_option(_tree->group,
                                   "--g",
                                   NULL);
    (void)d_arg_command_add_option(_tree->s,
                                   "--x",
                                   NULL);
    (void)d_arg_command_add_flag(_tree->t,
                                 "--y",
                                 NULL);
    d_arg_command_set_allow_extras(app,
                                   (int)D_ARG_EXTRAS_CAPTURE);
    d_arg_command_set_allow_extras(_tree->s,
                                   (int)D_ARG_EXTRAS_CAPTURE);
    d_arg_command_set_allow_extras(_tree->t,
                                   (int)D_ARG_EXTRAS_CAPTURE);

    return;
}

/*
d_tests_arg_command_parsed
  What a parse leaves to ask about, as CLI11's count, count_all,
got_subcommand, remaining, remaining_size and clear answer.
  Tests the following:
  - how often each command was used, an option group's once whenever its
    parent was; the values below it, a named command's uses among them
  - the subcommands used, asked by command and by name: false for one that
    is not the command's own, where CLI11 throws OptionNotFound
  - the arguments a command did not use, alone or with those of the
    subcommands it used, which the counts take from every subcommand, and
    without `--`
  - a subcommand used twice; clear() on a subcommand forgets its parse and
    its subcommands', on the root the whole tree's
*/
bool
d_tests_arg_command_parsed(void)
{
    struct d_tests_arg_run          run  = { 0 };
    struct d_tests_arg_command_tree tree = { NULL, NULL, NULL, NULL, NULL };

    (void)d_tests_arg_begin(&run);
    d_tests_arg_command_grow(&run,
                             &tree);
    d_tests_arg_command_parse_listed(&run,
                                     PARSED_ARGS[0].list);
    d_tests_arg_command_note_parsed(&run,
                                    &tree);
    d_tests_arg_command_parse_listed(&run,
                                     PARSED_ARGS[1].list);
    d_tests_arg_command_note_parsed(&run,
                                    &tree);
    d_arg_command_clear(tree.s);
    d_tests_arg_note(&run,
                     "cleared s");
    d_tests_arg_command_note_parsed(&run,
                                    &tree);
    d_arg_command_clear(tree.app);
    d_tests_arg_note(&run,
                     "cleared app");
    d_tests_arg_record(&run,
                       D_ARG_OK);
    d_tests_arg_command_note_parsed(&run,
                                    &tree);

    const bool same = d_tests_arg_matches(&run,
                                          PARSED);

    d_tests_arg_end(&run);

    return same;
}

//==============================================================================
// 5.  UTILITIES
//==============================================================================

/**
 * @brief N19: a trigger turning a on, then turning a and b off; the startup
 *        state of each after, the last trigger set deciding.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n19(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_command* const trigger =
        d_arg_command_add_subcommand(_case->app,
                                     "t",
                                     NULL);
    struct d_arg_command* const targets[2] =
    {
        d_arg_command_add_subcommand(_case->app,
                                     "a",
                                     NULL),
        d_arg_command_add_subcommand(_case->app,
                                     "b",
                                     NULL)
    };

    (void)d_arg_command_trigger_on(trigger,
                                   targets,
                                   1u);
    (void)d_arg_command_trigger_off(trigger,
                                    targets,
                                    2u);
    (void)d_arg_buffer_append_cstr(_case->out,
                                   "N19: ");
    d_tests_arg_command_put_startup(_case->out,
                                    targets[0]);
    (void)d_arg_buffer_append_char(_case->out,
                                   ' ');
    d_tests_arg_command_put_startup(_case->out,
                                    targets[1]);
    (void)d_arg_buffer_append_char(_case->out,
                                   '\n');

    return;
}

/**
 * @brief N30: t turning a on, s turning b off, both reported; their startup
 *        states, then the parses of TRIGGER_ARGS.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n30(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_buffer* const  out  = &_case->run->transcript;
    struct d_arg_command* const on   =
        d_arg_command_add_subcommand(_case->app,
                                     "t",
                                     NULL);
    struct d_arg_command* const a    =
        d_tests_arg_watch_command(_case->run,
                                  "a",
                                  d_arg_command_add_subcommand(_case->app,
                                                               "a",
                                                               NULL));
    struct d_arg_command* const off  =
        d_arg_command_add_subcommand(_case->app,
                                     "s",
                                     NULL);
    struct d_arg_command* const b    =
        d_tests_arg_watch_command(_case->run,
                                  "b",
                                  d_arg_command_add_subcommand(_case->app,
                                                               "b",
                                                               NULL));

    (void)d_arg_command_trigger_on(on,
                                   &a,
                                   1u);
    (void)d_arg_command_trigger_off(off,
                                    &b,
                                    1u);
    (void)d_arg_buffer_append_cstr(out,
                                   "N30: ");
    d_tests_arg_command_put_startup(out,
                                    a);
    (void)d_arg_buffer_append_char(out,
                                   ' ');
    d_tests_arg_command_put_startup(out,
                                    b);
    (void)d_arg_buffer_append_char(out,
                                   '\n');

    // each parse
    for (size_t index = 0u; index < TRIGGER_COUNT; ++index)
    {
        d_tests_arg_command_parse_listed(_case->run,
                                         TRIGGER_ARGS[index].list);
    }

    return;
}

// TRIGGER_CASES
//   constant: TRIGGERS' cases, in the reference's order.
static const d_tests_arg_command_case_fn TRIGGER_CASES[] =
{
    d_tests_arg_command_n19, d_tests_arg_command_n30
};

// TRIGGER_CASE_COUNT
//   constant: how many cases TRIGGER_CASES holds.
static const size_t TRIGGER_CASE_COUNT =
    sizeof(TRIGGER_CASES) / sizeof(TRIGGER_CASES[0]);

/*
d_tests_arg_command_triggers
  Subcommands enabled and disabled by another's use, as CLI11's TriggerOn
and TriggerOff arrange.
  Tests the following:
  - a target turned on starts disabled by default, one turned off enabled,
    the last trigger set deciding
  - a target turned on is no subcommand until its trigger is met, in that
    parse alone, and stays one once met, later in the line; a target
    turned off is one until its trigger is met
*/
bool
d_tests_arg_command_triggers(void)
{
    return d_tests_arg_command_cases("TRIGGERS",
                                     TRIGGER_CASES,
                                     TRIGGER_CASE_COUNT,
                                     TRIGGERS);
}

/**
 * @brief Appends an option as the reference's desc2 writes it: every name,
 *        the description, the type name, the default text, the type size,
 *        the expected count, whether it takes extra arguments, and whether
 *        it is required; `(null)` for none.
 *
 * @param[in,out] _out     the transcript.
 * @param[in]     _option  the option; may be NULL.
 */
static void
d_tests_arg_command_put_counts(
    struct d_arg_buffer*       _out,
    const struct d_arg_option* _option
)
{
    // no option
    if (!_option)
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       "(null)");

        return;
    }

    (void)d_arg_buffer_append_char(_out,
                                   '[');
    (void)d_arg_option_name(_option,
                            D_ARG_NAME_ALL,
                            _out);
    (void)d_arg_buffer_printf(_out,
                              "] desc='%s' type='",
                              d_arg_option_description(_option));
    (void)d_arg_option_type_name(_option,
                                 _out);
    (void)d_arg_buffer_printf(
        _out,
        "' def='%s' ts=%d,%d exp=%d,%d extra=%d req=%d",
        d_arg_option_default_text(_option),
        _option->type_size_min,
        _option->type_size_max,
        _option->expected_min,
        _option->expected_max,
        (int)d_arg_option_has_flag(_option,
                                   D_ARG_OPTION_ALLOW_EXTRA_ARGS),
        (int)d_arg_option_has_flag(_option,
                                   D_ARG_OPTION_REQUIRED));

    return;
}

/**
 * @brief Appends a line `<tag>: <option>`, the option as
 *        d_tests_arg_command_put_counts writes it.
 *
 * @param[in,out] _out     the transcript.
 * @param[in]     _tag     the line's tag.
 * @param[in]     _option  the option; may be NULL.
 */
static void
d_tests_arg_command_line_counts(
    struct d_arg_buffer*       _out,
    const char*                _tag,
    const struct d_arg_option* _option
)
{
    (void)d_arg_buffer_printf(_out,
                              "%s: ",
                              _tag);
    d_tests_arg_command_put_counts(_out,
                                   _option);
    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

/**
 * @brief Parses arguments, as d_tests_arg_command_parse_listed, then notes
 *        what the parser wrote to its output, `out="..."` with each newline
 *        as `\\n`, and forgets it.
 *
 * @param[in,out] _run   the run.
 * @param[in]     _args  the arguments, ending in NULL.
 */
static void
d_tests_arg_command_parse_written(
    struct d_tests_arg_run* _run,
    const char* const*      _args
)
{
    d_tests_arg_command_parse_listed(_run,
                                     _args);

    const struct d_pack_text written = d_arg_buffer_text(&_run->out);

    (void)d_arg_buffer_append_cstr(&_run->transcript,
                                   "out=\"");

    // each character, a newline as its escape
    for (size_t index = 0u; index < written.length; ++index)
    {
        // a newline
        if (written.data[index] == '\n')
        {
            (void)d_arg_buffer_append_cstr(&_run->transcript,
                                           "\\n");

            continue;
        }

        (void)d_arg_buffer_append_char(&_run->transcript,
                                       written.data[index]);
    }

    (void)d_arg_buffer_append_cstr(&_run->transcript,
                                   "\"\n");
    d_arg_buffer_clear(&_run->out);

    return;
}

/**
 * @brief N15: an option deprecated for another, and one deprecated for
 *        nothing, both reported; then a parse using both.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n15(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_option* const old   =
        d_tests_arg_watch_option(_case->run,
                                 "old",
                                 d_arg_command_add_option(_case->app,
                                                          "--old",
                                                          "Old one"));
    struct d_arg_option* const older =
        d_tests_arg_watch_option(_case->run,
                                 "older",
                                 d_arg_command_add_option(_case->app,
                                                          "--older",
                                                          NULL));

    (void)d_arg_command_deprecate_option(old,
                                         "--new");
    (void)d_arg_command_deprecate_option(older,
                                         "");
    d_tests_arg_command_line_counts(&_case->run->transcript,
                                    "N15",
                                    old);
    d_tests_arg_command_line_counts(&_case->run->transcript,
                                    "N15b",
                                    older);
    d_tests_arg_command_parse_written(_case->run,
                                      LIFECYCLE_ARGS[0].list);

    return;
}

/**
 * @brief N16: an option taking one to three values and extra arguments,
 *        retired; the listing and the retired option, reported; then a
 *        parse giving it two values.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n16(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_buffer* const out    = &_case->run->transcript;
    struct d_arg_option* const option =
        d_arg_command_add_option(_case->app,
                                 "-r,--ret",
                                 "To retire");

    d_arg_option_set_expected_range(option,
                                    1,
                                    3);
    d_arg_option_set_allow_extra_args(option,
                                      true);
    (void)d_arg_command_add_option(_case->app,
                                   "--keep",
                                   NULL);
    d_arg_parser_clear_definition_error(&_case->run->parser);
    d_tests_arg_command_tried(out,
                              _case->run,
                              "N16",
                              (d_arg_command_retire_option(_case->app,
                                                           option) != NULL));
    d_tests_arg_command_line_listing(out,
                                     "N16n",
                                     _case->app);
    d_tests_arg_command_line_counts(
        out,
        "N16o",
        d_tests_arg_watch_option(_case->run,
                                 "ret",
                                 d_arg_command_option(_case->app,
                                                      "--ret")));
    d_tests_arg_command_parse_written(_case->run,
                                      LIFECYCLE_ARGS[1].list);

    return;
}

/**
 * @brief N17: an option retired by a name no option has, reported; then
 *        parses giving it nothing and a value.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n17(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_buffer* const out = &_case->run->transcript;

    d_tests_arg_command_tried(
        out,
        _case->run,
        "N17",
        (d_arg_command_retire_option_name(_case->app,
                                          "--gone") != NULL));
    d_tests_arg_command_line_counts(
        out,
        "N17o",
        d_tests_arg_watch_option(_case->run,
                                 "gone",
                                 d_arg_command_option(_case->app,
                                                      "--gone")));
    d_tests_arg_command_parse_written(_case->run,
                                      LIFECYCLE_ARGS[2].list);
    d_tests_arg_command_parse_written(_case->run,
                                      LIFECYCLE_ARGS[3].list);

    return;
}

/**
 * @brief Appends the lines of an option retired as N18 retires them: the
 *        retirement tried, then the command's listing.
 *
 * @param[in,out] _case    the case's transcript and run.
 * @param[in,out] _option  the option.
 * @param[in]     _tag     the tag of the first line.
 * @param[in]     _listed  the tag of the second.
 */
static void
d_tests_arg_command_retired(
    struct d_tests_arg_command_case* _case,
    struct d_arg_option*             _option,
    const char*                      _tag,
    const char*                      _listed
)
{
    d_arg_parser_clear_definition_error(&_case->run->parser);
    d_tests_arg_command_tried(_case->out,
                              _case->run,
                              _tag,
                              (d_arg_command_retire_option(_case->app,
                                                           _option) != NULL));
    d_tests_arg_command_line_listing(_case->out,
                                     _listed,
                                     _case->app);

    return;
}

/**
 * @brief N18: a flag with a value of its own retired, which a plain option
 *        cannot be named as.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n18(
    struct d_tests_arg_command_case* _case
)
{
    d_tests_arg_command_retired(_case,
                                d_arg_command_add_flag(_case->app,
                                                       "--f{3}",
                                                       NULL),
                                "N18",
                                "N18n");

    return;
}

/**
 * @brief N18b: a non-standard short name retired, which the command that
 *        retirement makes its copy in does not allow.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n18b(
    struct d_tests_arg_command_case* _case
)
{
    d_arg_command_set_non_standard_names(_case->app,
                                         true);
    d_tests_arg_command_retired(_case,
                                d_arg_command_add_option(_case->app,
                                                         "-ab",
                                                         NULL),
                                "N18b",
                                "N18c");

    return;
}

/**
 * @brief N18d: an option named `-h` after the help flag is removed,
 *        retired, which the help flag of the command that retirement makes
 *        its copy in refuses.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n18d(
    struct d_tests_arg_command_case* _case
)
{
    (void)d_arg_command_set_help_flag(_case->app,
                                      "",
                                      NULL);
    d_tests_arg_command_retired(_case,
                                d_arg_command_add_option(_case->app,
                                                         "-h",
                                                         NULL),
                                "N18d",
                                "N18e");

    return;
}

// LIFECYCLE_CASES
//   constant: LIFECYCLE's cases, in the reference's order.
static const d_tests_arg_command_case_fn LIFECYCLE_CASES[] =
{
    d_tests_arg_command_n15, d_tests_arg_command_n16, d_tests_arg_command_n17,
    d_tests_arg_command_n18, d_tests_arg_command_n18b, d_tests_arg_command_n18d
};

// LIFECYCLE_COUNT
//   constant: how many cases LIFECYCLE_CASES holds.
static const size_t LIFECYCLE_COUNT =
    sizeof(LIFECYCLE_CASES) / sizeof(LIFECYCLE_CASES[0]);

/*
d_tests_arg_command_lifecycle
  Options deprecated and retired, as CLI11's deprecate_option and
retire_option arrange, and what using them writes.
  Tests the following:
  - a deprecated option's description names its replacement, and its type
    name the DEPRECATED validator; using it writes "<name> is deprecated
    please use '<replacement>' instead", once
  - a retired option keeps its names and counts, describes itself as
    retired, is typed and defaulted RETIRED, and writes "WARNING <name> is
    retired and has no effect" when used, once for all its values
  - a name no option has retired as an option taking at most one value,
    whose default is given when it takes none
  - the names retirement cannot copy -- a flag's value, a non-standard
    short name, `-h` beside a help flag -- refused with CLI11's errors,
    the option kept
*/
bool
d_tests_arg_command_lifecycle(void)
{
    return d_tests_arg_command_cases("LIFECYCLE",
                                     LIFECYCLE_CASES,
                                     LIFECYCLE_COUNT,
                                     LIFECYCLE);
}

/**
 * @brief N15c: options deprecated by name -- one by its long name, with a
 *        replacement, one by its short name, without -- and a name no
 *        option has, refused as CLI11's OptionNotFound; then a parse using
 *        both. Last, the module's own: a NULL command and a NULL name,
 *        refused with nothing recorded.
 *
 * @param[in,out] _case  the case's transcript and run.
 */
static void
d_tests_arg_command_n15c(
    struct d_tests_arg_command_case* _case
)
{
    struct d_arg_buffer* const out   = &_case->run->transcript;
    struct d_arg_option* const named =
        d_tests_arg_watch_option(_case->run,
                                 "named",
                                 d_arg_command_add_option(_case->app,
                                                          "--named",
                                                          NULL));
    struct d_arg_option* const brief =
        d_tests_arg_watch_option(_case->run,
                                 "short",
                                 d_arg_command_add_option(_case->app,
                                                          "-s,--short",
                                                          NULL));

    d_tests_arg_command_tried(
        out,
        _case->run,
        "N15c",
        (d_arg_command_deprecate_option_name(_case->app,
                                             "--named",
                                             "--newer") == D_ARG_OK));
    d_tests_arg_command_tried(
        out,
        _case->run,
        "N15d",
        (d_arg_command_deprecate_option_name(_case->app,
                                             "-s",
                                             NULL) == D_ARG_OK));
    d_tests_arg_command_tried(
        out,
        _case->run,
        "N15e",
        (d_arg_command_deprecate_option_name(_case->app,
                                             "--nowhere",
                                             "--named") == D_ARG_OK));
    d_tests_arg_command_line_counts(out,
                                    "N15f",
                                    named);
    d_tests_arg_command_line_counts(out,
                                    "N15g",
                                    brief);
    d_tests_arg_command_parse_written(_case->run,
                                      DEPRECATED_NAMED_ARGS[0].list);

    const int no_command = d_arg_command_deprecate_option_name(NULL,
                                                               "--named",
                                                               NULL);
    const int no_name    = d_arg_command_deprecate_option_name(_case->app,
                                                               NULL,
                                                               NULL);

    (void)d_arg_buffer_printf(
        out,
        "N15h: %s %s recorded=%s\n",
        d_arg_status_name(no_command),
        d_arg_status_name(no_name),
        d_arg_status_name(
            d_arg_parser_definition_error(&_case->run->parser)->status));

    return;
}

// DEPRECATED_NAMED_CASES
//   constant: DEPRECATED_NAMED's cases.
static const d_tests_arg_command_case_fn DEPRECATED_NAMED_CASES[] =
{
    d_tests_arg_command_n15c
};

// DEPRECATED_NAMED_COUNT
//   constant: how many cases DEPRECATED_NAMED_CASES holds.
static const size_t DEPRECATED_NAMED_COUNT =
    sizeof(DEPRECATED_NAMED_CASES) / sizeof(DEPRECATED_NAMED_CASES[0]);

/*
d_tests_arg_command_deprecated_named
  Options deprecated by the name a command finds them by, as CLI11's
deprecate_option(app, name, replacement) does it.
  Tests the following:
  - an option found by its long name and one by its short name, deprecated
    with a replacement and without: their descriptions and type names, and
    the warnings a parse writes
  - a name no option has: CLI11's OptionNotFound, recorded
  - a NULL command or name: refused, nothing recorded
*/
bool
d_tests_arg_command_deprecated_named(void)
{
    return d_tests_arg_command_cases("DEPRECATED_NAMED",
                                     DEPRECATED_NAMED_CASES,
                                     DEPRECATED_NAMED_COUNT,
                                     DEPRECATED_NAMED);
}

//==============================================================================
// 6.  DEPARTURES FROM CLI11
//==============================================================================

/*
d_tests_arg_command_unread_sums
  The sums add_flag_function's function is given that are no integer, which
depart from CLI11 as every value that does not convert does.
  Tests the following:
  - a fraction, a sum with a fraction, a number followed by letters, a
    number beside a word, and totals past the 64-bit limits each give the
    function 0, as a `long` and as an intmax_t
*/
bool
d_tests_arg_command_unread_sums(void)
{
    const bool narrow = d_tests_arg_command_summed(false,
                                                   D_ARG_INTEGER_C,
                                                   UNREAD_ARGS,
                                                   UNREAD_COUNT,
                                                   DEPARTED_SUMS);
    const bool wide   = d_tests_arg_command_summed(true,
                                                   D_ARG_INTEGER_C,
                                                   UNREAD_ARGS,
                                                   UNREAD_COUNT,
                                                   DEPARTED_SUMS);

    return ( (narrow) &&
             (wide) );
}

/*
d_tests_arg_command_decimal_sums
  Sums of numbers with a leading zero under the module's own rule, which
departs from CLI11's.
  Tests the following:
  - `010` is ten, `08` eight, and `0o10` eight, as a `long` and as an
    intmax_t
*/
bool
d_tests_arg_command_decimal_sums(void)
{
    const bool narrow = d_tests_arg_command_summed(false,
                                                   D_ARG_INTEGER_DECIMAL,
                                                   DECIMAL_ARGS,
                                                   DECIMAL_COUNT,
                                                   DEPARTED_DECIMAL);
    const bool wide   = d_tests_arg_command_summed(true,
                                                   D_ARG_INTEGER_DECIMAL,
                                                   DECIMAL_ARGS,
                                                   DECIMAL_COUNT,
                                                   DEPARTED_DECIMAL);

    return ( (narrow) &&
             (wide) );
}

/**
 * @brief DEPARTED_SWEEP_OPTION's run: `--o` of the subcommand s, needed and
 *        excluded by the root's `--a`, needed by `--p` of the subcommand
 *        s2, needed by the root and excluded by s2; used by a parse, then
 *        removed, then the links written and everything used.
 *
 * @return whether the transcript was as expected.
 */
static bool
d_tests_arg_command_swept_option(void)
{
    struct d_tests_arg_run      run    = { 0 };
    struct d_arg_command* const app    = d_tests_arg_begin(&run);
    struct d_arg_option* const  needer = d_arg_command_add_option(app,
                                                                  "--a",
                                                                  NULL);
    struct d_arg_command* const sub    = d_arg_command_add_subcommand(app,
                                                                      "s",
                                                                      NULL);
    struct d_arg_option* const  gone   = d_arg_command_add_option(sub,
                                                                  "--o",
                                                                  NULL);
    struct d_arg_command* const other  = d_arg_command_add_subcommand(app,
                                                                      "s2",
                                                                      NULL);
    struct d_arg_option* const  near   = d_arg_command_add_option(other,
                                                                  "--p",
                                                                  NULL);

    (void)d_arg_option_needs(needer,
                             gone);
    (void)d_arg_option_excludes(needer,
                                gone);
    (void)d_arg_option_needs(near,
                             gone);
    (void)d_arg_command_needs_option(app,
                                     gone);
    (void)d_arg_command_excludes_option(other,
                                        gone);
    d_tests_arg_command_parse_listed(&run,
                                     SWEEP_OPTION_ARGS[0].list);
    d_tests_arg_note(&run,
                     "removed %d",
                     (int)d_arg_command_remove_option(sub,
                                                      gone));
    d_tests_arg_note(&run,
                     "--a needs=%lu excludes=%lu, --p needs=%lu",
                     (unsigned long)needer->needs.count,
                     (unsigned long)needer->excludes.count,
                     (unsigned long)near->needs.count);
    d_tests_arg_note(&run,
                     "prog needs=%lu, s2 excludes=%lu, s order=%lu",
                     (unsigned long)app->need_options.count,
                     (unsigned long)other->exclude_options.count,
                     (unsigned long)sub->parse_order.count);
    d_tests_arg_command_parse_listed(&run,
                                     SWEEP_OPTION_ARGS[1].list);

    const bool same = d_tests_arg_matches(&run,
                                          DEPARTED_SWEEP_OPTION);

    d_tests_arg_end(&run);

    return same;
}

/**
 * @brief DEPARTED_SWEEP_COMMAND's run: the subcommand s, with t below it
 *        and t's `--inner`, which the root's `--a` needs and excludes and
 *        the root needs; the option group G needing t and excluding s; s
 *        used by a parse, then removed, then the links written and `--a`
 *        used.
 *
 * @return whether the transcript was as expected.
 */
static bool
d_tests_arg_command_swept_command(void)
{
    struct d_tests_arg_run      run    = { 0 };
    struct d_arg_command* const app    = d_tests_arg_begin(&run);
    struct d_arg_option* const  needer = d_arg_command_add_option(app,
                                                                  "--a",
                                                                  NULL);
    struct d_arg_command* const sub    = d_arg_command_add_subcommand(app,
                                                                      "s",
                                                                      NULL);
    struct d_arg_command* const below  = d_arg_command_add_subcommand(sub,
                                                                      "t",
                                                                      NULL);
    struct d_arg_option* const  inner  = d_arg_command_add_option(below,
                                                                  "--inner",
                                                                  NULL);
    struct d_arg_command* const group  =
        d_arg_command_add_option_group(app,
                                       "G",
                                       NULL);

    (void)d_arg_option_needs(needer,
                             inner);
    (void)d_arg_option_excludes(needer,
                                inner);
    (void)d_arg_command_needs_option(app,
                                     inner);
    (void)d_arg_command_needs_command(group,
                                      below);
    (void)d_arg_command_excludes_command(group,
                                         sub);
    d_tests_arg_command_parse_listed(&run,
                                     SWEEP_COMMAND_ARGS[0].list);
    d_tests_arg_note(&run,
                     "removed %d",
                     (int)d_arg_command_remove_subcommand(app,
                                                          sub));
    d_tests_arg_note(&run,
                     "--a needs=%lu excludes=%lu, prog needs=%lu parsed=%lu",
                     (unsigned long)needer->needs.count,
                     (unsigned long)needer->excludes.count,
                     (unsigned long)app->need_options.count,
                     (unsigned long)d_arg_command_parsed_subcommand_count(app));
    d_tests_arg_note(&run,
                     "G needs=%lu excludes=%lu",
                     (unsigned long)group->need_subcommands.count,
                     (unsigned long)group->exclude_subcommands.count);
    d_tests_arg_command_parse_listed(&run,
                                     SWEEP_COMMAND_ARGS[1].list);

    const bool same = d_tests_arg_matches(&run,
                                          DEPARTED_SWEEP_COMMAND);

    d_tests_arg_end(&run);

    return same;
}

/**
 * @brief Notes a line `<label> <removed> version='<version>'`: whether a
 *        removal took the version flag, and the version a command shows.
 *
 * @param[in,out] _run      the run.
 * @param[in]     _label    what was removed.
 * @param[in]     _removed  whether it was.
 */
static void
d_tests_arg_command_note_version(
    struct d_tests_arg_run* _run,
    const char*             _label,
    bool                    _removed
)
{
    (void)d_arg_buffer_printf(&_run->transcript,
                              "%s %d version='",
                              _label,
                              (int)_removed);
    (void)d_arg_command_version(&_run->parser.command,
                                d_arg_buffer_sink(&_run->transcript));
    (void)d_arg_buffer_append_cstr(&_run->transcript,
                                   "'\n");

    return;
}

/**
 * @brief DEPARTED_SWEEP_VERSION's run: the version flag removed with
 *        remove_option; another moved into the option group G, and G
 *        removed; then the flags' name parsed.
 *
 * @return whether the transcript was as expected.
 */
static bool
d_tests_arg_command_swept_version(void)
{
    struct d_tests_arg_run      run = { 0 };
    struct d_arg_command* const app = d_tests_arg_begin(&run);

    (void)d_arg_command_set_version_flag(app,
                                         "-V",
                                         "1",
                                         NULL);
    d_tests_arg_command_note_version(&run,
                                     "removed",
                                     d_arg_command_remove_option(app,
                                                                 app->version));
    (void)d_arg_command_set_version_flag(app,
                                         "-V",
                                         "2",
                                         NULL);

    struct d_arg_command* const group =
        d_arg_command_add_option_group(app,
                                       "G",
                                       NULL);

    (void)d_arg_command_group_add_option(group,
                                         app->version);
    d_tests_arg_command_note_version(&run,
                                     "moved and removed",
                                     d_arg_command_remove_subcommand(app,
                                                                     group));
    d_tests_arg_command_parse_listed(&run,
                                     SWEEP_VERSION_ARGS);

    const bool same = d_tests_arg_matches(&run,
                                          DEPARTED_SWEEP_VERSION);

    d_tests_arg_end(&run);

    return same;
}

/**
 * @brief Notes one of the parser's reports: `<label> <status>
 *        option=<option> command=<command>`, `-` for one it does not name.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _label  which report.
 * @param[in]     _error  the report.
 */
static void
d_tests_arg_command_note_report(
    struct d_tests_arg_run*   _run,
    const char*               _label,
    const struct d_arg_error* _error
)
{
    struct d_arg_buffer* const out = &_run->transcript;

    (void)d_arg_buffer_printf(out,
                              "%s %s option=",
                              _label,
                              d_arg_status_name(_error->status));
    d_tests_arg_command_put_name(out,
                                 _error->option);
    (void)d_arg_buffer_printf(out,
                              " command=%s\n",
                              (_error->command != NULL)
                              ? d_arg_command_name(_error->command)
                              : "-");

    return;
}

/**
 * @brief Notes DEPARTED_SWEEP_REPORTS' refused definitions: the option
 *        `--ab` refused ignore_case beside `--Ab`, then removed; the
 *        subcommand b refused the name of a, then removed; the definition
 *        report before and after each removal.
 *
 * @param[in,out] _run  the run.
 */
static void
d_tests_arg_command_refused_reports(
    struct d_tests_arg_run* _run
)
{
    struct d_arg_command* const     app        = &_run->parser.command;
    const struct d_arg_error* const definition =
        d_arg_parser_definition_error(&_run->parser);

    (void)d_arg_command_add_option(app,
                                   "--Ab",
                                   NULL);

    struct d_arg_option* const refused = d_arg_command_add_option(app,
                                                                  "--ab",
                                                                  NULL);

    (void)d_arg_option_set_ignore_case(refused,
                                       true);
    d_tests_arg_command_note_report(_run,
                                    "refused",
                                    definition);
    (void)d_arg_command_remove_option(app,
                                      refused);
    d_tests_arg_command_note_report(_run,
                                    "removed",
                                    definition);
    d_arg_parser_clear_definition_error(&_run->parser);
    (void)d_arg_command_add_subcommand(app,
                                       "a",
                                       NULL);

    struct d_arg_command* const renamed = d_arg_command_add_subcommand(app,
                                                                       "b",
                                                                       NULL);

    (void)d_arg_command_set_name(renamed,
                                 "a");
    d_tests_arg_command_note_report(_run,
                                    "refused",
                                    definition);
    (void)d_arg_command_remove_subcommand(app,
                                          renamed);
    d_tests_arg_command_note_report(_run,
                                    "removed",
                                    definition);

    return;
}

/**
 * @brief DEPARTED_SWEEP_REPORTS's run: the refused definitions, then the
 *        subcommand c's callback failure recorded, the subcommand removed,
 *        and the parse report noted before and after.
 *
 * @return whether the transcript was as expected.
 */
static bool
d_tests_arg_command_swept_reports(void)
{
    struct d_tests_arg_run      run = { 0 };
    struct d_arg_command* const app = d_tests_arg_begin(&run);

    d_tests_arg_command_refused_reports(&run);

    struct d_arg_command* const failed = d_arg_command_add_subcommand(app,
                                                                      "c",
                                                                      NULL);

    (void)d_arg_command_fail(failed,
                             D_ARG_ERROR_VALIDATION,
                             "bad %d",
                             7);
    d_tests_arg_command_note_report(&run,
                                    "failed",
                                    d_arg_parser_error(&run.parser));
    (void)d_arg_command_remove_subcommand(app,
                                          failed);
    d_tests_arg_command_note_report(&run,
                                    "removed",
                                    d_arg_parser_error(&run.parser));

    const bool same = d_tests_arg_matches(&run,
                                          DEPARTED_SWEEP_REPORTS);

    d_tests_arg_end(&run);

    return same;
}

/*
d_tests_arg_command_swept
  What a removal leaves pointing at the records it releases: nothing, where
CLI11 leaves every pointer but its own command's links.
  Tests the following:
  - an option removed is forgotten by the options of any command that need
    or exclude it, by the commands that need or exclude it, and by its
    command's parse order; the next parse checks what is left
  - a subcommand removed takes with it every link to it, to a command below
    it and to their options, and the last parse's list of subcommands
  - the version flag removed by remove_option, or with the option group it
    was moved into, leaves no version, and its name is then no option
  - the parser's reports of a refused definition and of a callback's
    failure stay, without the option or command they named
*/
bool
d_tests_arg_command_swept(void)
{
    const bool option  = d_tests_arg_command_swept_option();
    const bool command = d_tests_arg_command_swept_command();
    const bool version = d_tests_arg_command_swept_version();
    const bool reports = d_tests_arg_command_swept_reports();

    return ( (option)  &&
             (command) &&
             (version) &&
             (reports) );
}

/*
d_tests_arg_command_kept_in_place
  A subcommand refused a move into an option group stays where it was, where
CLI11 destroys it.
  Tests the following:
  - a subcommand of s, moved into an option group of the root where its
    name is taken, is refused with CLI11's OptionAlreadyAdded and stays in
    s, which a parse then finds it in
*/
bool
d_tests_arg_command_kept_in_place(void)
{
    struct d_tests_arg_run      run = { 0 };
    struct d_arg_command* const app = d_tests_arg_begin(&run);

    (void)d_arg_command_add_subcommand(app,
                                       "one",
                                       NULL);

    struct d_arg_command* const sub    =
        d_tests_arg_watch_command(&run,
                                  "s",
                                  d_arg_command_add_subcommand(app,
                                                               "s",
                                                               NULL));
    struct d_arg_command* const moved  = d_arg_command_add_subcommand(sub,
                                                                      "one",
                                                                      NULL);
    struct d_arg_command* const group  =
        d_arg_command_add_option_group(app,
                                       "H",
                                       NULL);
    const int                   status =
        d_arg_command_group_add_subcommand(group,
                                           moved);

    d_tests_arg_command_tried(&run.transcript,
                              &run,
                              "moved",
                              (status == (int)D_ARG_OK));
    (void)d_arg_buffer_append_cstr(&run.transcript,
                                   "after: ");
    d_tests_arg_command_put_subcommands(&run.transcript,
                                        app);
    d_tests_arg_command_put_subcommands(&run.transcript,
                                        sub);
    d_tests_arg_command_put_subcommands(&run.transcript,
                                        group);
    (void)d_arg_buffer_append_char(&run.transcript,
                                   '\n');
    d_tests_arg_command_parse_listed(&run,
                                     KEPT_ARGS);

    const bool same = d_tests_arg_matches(&run,
                                          DEPARTED_KEPT);

    d_tests_arg_end(&run);

    return same;
}

/**
 * @brief Appends a line of a move or a link refused: `<label>: <status>:
 *        <message>` for one the parser recorded, which is then cleared;
 *        `<label>: <status>` for one it did not; `<label>: ok` for one
 *        made.
 *
 * @param[in,out] _out     the transcript.
 * @param[in,out] _run     the run whose parser would record the refusal.
 * @param[in]     _label   what was tried.
 * @param[in]     _status  the status it returned.
 */
static void
d_tests_arg_command_refusal(
    struct d_arg_buffer*    _out,
    struct d_tests_arg_run* _run,
    const char*             _label,
    int                     _status
)
{
    const struct d_arg_error* const error =
        d_arg_parser_definition_error(&_run->parser);

    // a refusal the parser recorded
    if (error->status != (int)D_ARG_OK)
    {
        d_tests_arg_command_tried(_out,
                                  _run,
                                  _label,
                                  false);

        return;
    }

    (void)d_arg_buffer_printf(_out,
                              "%s: %s\n",
                              _label,
                              (_status == (int)D_ARG_OK)
                              ? "ok"
                              : d_arg_status_name(_status));

    return;
}

/**
 * @brief Appends DEPARTED_APART's moves of commands into themselves and of
 *        commands no group may take: the option group G into itself and
 *        into the group Inner inside it, the root into G, and Inner into
 *        the named subcommand s.
 *
 * @param[in,out] _out  the transcript.
 * @param[in,out] _run  the run.
 */
static void
d_tests_arg_command_groups_apart(
    struct d_arg_buffer*    _out,
    struct d_tests_arg_run* _run
)
{
    struct d_arg_command* const app   = &_run->parser.command;
    struct d_arg_command* const group = d_arg_command_option_group(app,
                                                                   "G");
    struct d_arg_command* const inner = d_arg_command_option_group(group,
                                                                   "Inner");

    d_tests_arg_command_refusal(
        _out,
        _run,
        "G into G",
        d_arg_command_group_add_subcommand(group,
                                           group));
    d_tests_arg_command_refusal(
        _out,
        _run,
        "G into Inner",
        d_arg_command_group_add_subcommand(inner,
                                           group));
    d_tests_arg_command_refusal(
        _out,
        _run,
        "prog into G",
        d_arg_command_group_add_subcommand(group,
                                           app));
    d_tests_arg_command_refusal(
        _out,
        _run,
        "Inner into s",
        d_arg_command_group_add_subcommand(d_arg_command_subcommand(app,
                                                                    "s"),
                                           inner));

    return;
}

/**
 * @brief Appends DEPARTED_APART's other moves: nothing into the option
 *        group G, the root's `--a` into the named subcommand s, and another
 *        parser's subcommand q into G.
 *
 * @param[in,out] _out    the transcript.
 * @param[in,out] _run    the run.
 * @param[in,out] _other  another parser's subcommand.
 */
static void
d_tests_arg_command_moves_apart(
    struct d_arg_buffer*    _out,
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _other
)
{
    struct d_arg_command* const app   = &_run->parser.command;
    struct d_arg_command* const group = d_arg_command_option_group(app,
                                                                   "G");

    d_tests_arg_command_refusal(
        _out,
        _run,
        "nothing into G",
        d_arg_command_group_add_subcommand(group,
                                           NULL));
    d_tests_arg_command_refusal(
        _out,
        _run,
        "--a into s",
        d_arg_command_group_add_option(d_arg_command_subcommand(app,
                                                                "s"),
                                       d_arg_command_option(app,
                                                            "--a")));
    d_tests_arg_command_refusal(
        _out,
        _run,
        "q into G",
        d_arg_command_group_add_subcommand(group,
                                           _other));

    return;
}

/**
 * @brief Appends DEPARTED_APART's links of options: the root's `--a`
 *        needing and excluding another parser's `--q`.
 *
 * @param[in,out] _out    the transcript.
 * @param[in,out] _run    the run.
 * @param[in,out] _other  the other parser's root.
 */
static void
d_tests_arg_command_options_apart(
    struct d_arg_buffer*    _out,
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _other
)
{
    struct d_arg_option* const near = d_arg_command_option(
                                          &_run->parser.command,
                                          "--a");
    struct d_arg_option* const far  = d_arg_command_option(_other,
                                                           "--q");

    d_tests_arg_command_refusal(_out,
                                _run,
                                "--a needs --q",
                                d_arg_option_needs(near,
                                                   far));
    d_tests_arg_command_refusal(_out,
                                _run,
                                "--a excludes --q",
                                d_arg_option_excludes(near,
                                                      far));

    return;
}

/**
 * @brief Appends DEPARTED_APART's links of commands: the subcommand s
 *        needing another parser's root, excluding its subcommand q,
 *        needing and excluding its `--q`, and triggering q.
 *
 * @param[in,out] _out    the transcript.
 * @param[in,out] _run    the run.
 * @param[in,out] _other  the other parser's root.
 */
static void
d_tests_arg_command_commands_apart(
    struct d_arg_buffer*    _out,
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _other
)
{
    struct d_arg_command* const sub    = d_arg_command_subcommand(
                                             &_run->parser.command,
                                             "s");
    struct d_arg_option* const  far    = d_arg_command_option(_other,
                                                              "--q");
    struct d_arg_command* const target = d_arg_command_subcommand(_other,
                                                                  "q");

    d_tests_arg_command_refusal(_out,
                                _run,
                                "s needs prog",
                                d_arg_command_needs_command(sub,
                                                            _other));
    d_tests_arg_command_refusal(_out,
                                _run,
                                "s excludes q",
                                d_arg_command_excludes_command(sub,
                                                               target));
    d_tests_arg_command_refusal(_out,
                                _run,
                                "s needs --q",
                                d_arg_command_needs_option(sub,
                                                           far));
    d_tests_arg_command_refusal(_out,
                                _run,
                                "s excludes --q",
                                d_arg_command_excludes_option(sub,
                                                              far));
    d_tests_arg_command_refusal(_out,
                                _run,
                                "s triggers q",
                                d_arg_command_trigger_on(sub,
                                                         &target,
                                                         1u));

    return;
}

/**
 * @brief Appends what DEPARTED_APART's moves and links left: `--a`'s needs
 *        and excludes, the subcommand s's needs and excludes of options and
 *        of commands, and the other parser's q's excludes, all none; then
 *        the subcommands of the root, of its group G, and of the other
 *        parser's root.
 *
 * @param[in,out] _out    the transcript.
 * @param[in]     _run    the run.
 * @param[in]     _other  the other parser's root.
 */
static void
d_tests_arg_command_left_apart(
    struct d_arg_buffer*          _out,
    const struct d_tests_arg_run* _run,
    const struct d_arg_command*   _other
)
{
    const struct d_arg_option* const  near   =
        d_arg_command_option(&_run->parser.command,
                             "--a");
    const struct d_arg_command* const sub    =
        d_arg_command_subcommand(&_run->parser.command,
                                 "s");
    const struct d_arg_command* const target =
        d_arg_command_subcommand(_other,
                                 "q");

    (void)d_arg_buffer_printf(_out,
                              "left: --a %lu %lu, s %lu %lu %lu %lu, q %lu\n",
                              (unsigned long)near->needs.count,
                              (unsigned long)near->excludes.count,
                              (unsigned long)sub->need_options.count,
                              (unsigned long)sub->exclude_options.count,
                              (unsigned long)sub->need_subcommands.count,
                              (unsigned long)sub->exclude_subcommands.count,
                              (unsigned long)target->exclude_subcommands.count);
    (void)d_arg_buffer_append_cstr(_out,
                                   "after: ");
    d_tests_arg_command_put_subcommands(_out,
                                        &_run->parser.command);
    d_tests_arg_command_put_subcommands(
        _out,
        d_arg_command_option_group(&_run->parser.command,
                                   "G"));
    d_tests_arg_command_put_subcommands(_out,
                                        _other);
    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

/*
d_tests_arg_command_kept_apart
  The moves and links that would corrupt a tree or join two parsers'
records, which CLI11 accepts and the module refuses.
  Tests the following:
  - an option group moved into itself or into a group inside it, the root
    moved, a group moved into a named subcommand, NULL moved, an option
    moved into a named subcommand, and another parser's subcommand moved,
    each refused with D_ARG_ERROR_BAD_ARGUMENT, unrecorded
  - an option needing or excluding another parser's option, a command
    needing or excluding another parser's command or option, and a
    command triggering another parser's, each refused with
    IncorrectConstruction, recorded, and nothing linked
  - both parsers' trees as they were
*/
bool
d_tests_arg_command_kept_apart(void)
{
    struct d_tests_arg_run      run       = { 0 };
    struct d_tests_arg_run      other_run = { 0 };
    struct d_arg_command* const app       = d_tests_arg_begin(&run);
    struct d_arg_command* const other     = d_tests_arg_begin(&other_run);
    struct d_arg_buffer         out       = D_ARG_BUFFER_INIT;

    (void)d_arg_command_add_option_group(
              d_arg_command_add_option_group(app,
                                             "G",
                                             NULL),
              "Inner",
              NULL);
    (void)d_arg_command_add_subcommand(app,
                                       "s",
                                       NULL);
    (void)d_arg_command_add_option(app,
                                   "--a",
                                   NULL);
    (void)d_arg_command_add_option(other,
                                   "--q",
                                   NULL);
    d_tests_arg_command_groups_apart(&out,
                                     &run);
    d_tests_arg_command_moves_apart(&out,
                                    &run,
                                    d_arg_command_add_subcommand(other,
                                                                 "q",
                                                                 NULL));
    d_tests_arg_command_options_apart(&out,
                                      &run,
                                      other);
    d_tests_arg_command_commands_apart(&out,
                                       &run,
                                       other);
    d_tests_arg_command_left_apart(&out,
                                   &run,
                                   other);

    const bool same = d_tests_arg_same("DEPARTED_APART",
                                       d_arg_buffer_text(&out),
                                       DEPARTED_APART);

    d_arg_buffer_release(&out);
    d_tests_arg_end(&other_run);
    d_tests_arg_end(&run);

    return same;
}

/**
 * @brief Appends a line of a link made by name: what was tried, as TRY
 *        writes it, then, when it was made, the command of the option the
 *        newest link reached.
 *
 * @param[in,out] _out     the transcript.
 * @param[in,out] _run     the run.
 * @param[in]     _label   what was tried.
 * @param[in]     _status  the status it returned.
 * @param[in]     _links   the list the link went into.
 */
static void
d_tests_arg_command_linked(
    struct d_arg_buffer*            _out,
    struct d_tests_arg_run*         _run,
    const char*                     _label,
    int                             _status,
    const struct d_arg_option_list* _links
)
{
    d_tests_arg_command_refusal(_out,
                                _run,
                                _label,
                                _status);

    // the command of the option reached
    if ( (_status == (int)D_ARG_OK) &&
         (_links->count > 0u) )
    {
        (void)d_arg_buffer_append_cstr(_out,
                                       "  in ");
        (void)d_arg_command_display_name(
                  _links->items[_links->count - 1u]->parent,
                  false,
                  _out);
        (void)d_arg_buffer_append_char(_out,
                                       '\n');
    }

    return;
}

/*
d_tests_arg_command_moved_links
  The names an option moved into an option group needs and excludes, looked
up from the group, where CLI11 looks from the command it was made in.
  Tests the following:
  - with `--in` in both the group and its parent, the moved option needs
    and excludes the group's
  - a name neither holds refused with IncorrectConstruction, "Option <name>
    is not defined"
  - a parse giving the moved option alone, which the group parses, fails
    for the `--in` it needs
*/
bool
d_tests_arg_command_moved_links(void)
{
    struct d_tests_arg_run      run   = { 0 };
    struct d_arg_command* const app   = d_tests_arg_begin(&run);
    struct d_arg_command* const group =
        d_tests_arg_watch_command(&run,
                                  "G",
                                  d_arg_command_add_option_group(app,
                                                                 "G",
                                                                 NULL));
    struct d_arg_buffer* const  out   = &run.transcript;

    (void)d_arg_command_add_option(group,
                                   "--in",
                                   NULL);
    (void)d_arg_command_add_option(app,
                                   "--in",
                                   NULL);

    struct d_arg_option* const moved = d_arg_command_add_option(app,
                                                                "--a",
                                                                NULL);

    d_tests_arg_command_refusal(out,
                                &run,
                                "moved",
                                d_arg_command_group_add_option(group,
                                                               moved));
    d_tests_arg_command_linked(out,
                               &run,
                               "needs --in",
                               d_arg_option_needs_name(moved,
                                                       "--in"),
                               &moved->needs);
    d_tests_arg_command_linked(out,
                               &run,
                               "excludes --in",
                               d_arg_option_excludes_name(moved,
                                                          "--in"),
                               &moved->excludes);
    d_tests_arg_command_linked(out,
                               &run,
                               "needs --zz",
                               d_arg_option_needs_name(moved,
                                                       "--zz"),
                               &moved->needs);
    d_tests_arg_command_parse_listed(&run,
                                     MOVED_ARGS);

    const bool same = d_tests_arg_matches(&run,
                                          DEPARTED_LINKS);

    d_tests_arg_end(&run);

    return same;
}

/*
d_tests_arg_command_trigger_swept
  A trigger's list after a target is removed, and after the trigger is:
what is left of it, where CLI11 keeps every target it was given.
  Tests the following:
  - a target removed leaves the trigger's list, which still enables the
    others
  - the trigger removed takes its list with it; its targets stay disabled
    by default
*/
bool
d_tests_arg_command_trigger_swept(void)
{
    struct d_tests_arg_run      run        = { 0 };
    struct d_arg_command* const app        = d_tests_arg_begin(&run);
    struct d_arg_command* const trigger    =
        d_arg_command_add_subcommand(app,
                                     "t",
                                     NULL);
    struct d_arg_command* const targets[2] =
    {
        d_arg_command_add_subcommand(app,
                                     "a",
                                     NULL),
        d_tests_arg_watch_command(&run,
                                  "b",
                                  d_arg_command_add_subcommand(app,
                                                               "b",
                                                               NULL))
    };

    (void)d_arg_command_trigger_on(trigger,
                                   targets,
                                   2u);
    d_tests_arg_note(&run,
                     "removed a %d",
                     (int)d_arg_command_remove_subcommand(app,
                                                          targets[0]));
    d_tests_arg_command_parse_listed(&run,
                                     TRIGGERED_ARGS[0].list);
    d_tests_arg_note(&run,
                     "removed t %d",
                     (int)d_arg_command_remove_subcommand(app,
                                                          trigger));
    d_tests_arg_command_parse_listed(&run,
                                     TRIGGERED_ARGS[1].list);

    const bool same = d_tests_arg_matches(&run,
                                          DEPARTED_TRIGGERS);

    d_tests_arg_end(&run);

    return same;
}

/**
 * @brief Counts a context's release: the context is the count;
 *        DEPARTED_CONTEXTS' release function.
 *
 * @param[in,out] _context  the count, an `int`.
 */
static void
d_tests_arg_command_count_release(
    void* _context
)
{
    *(int*)_context += 1;

    return;
}

/**
 * @brief A command's callback that does nothing, for DEPARTED_CONTEXTS.
 *
 * @param[in] _command  the command; unused.
 * @param[in] _context  its context; unused.
 * @return D_ARG_OK.
 */
static int
d_tests_arg_command_do_nothing(
    struct d_arg_command* _command,
    void*                 _context
)
{
    (void)_command;
    (void)_context;

    return D_ARG_OK;
}

/**
 * @brief Appends a line `<label> first=<n> second=<n>`, the two counts.
 *
 * @param[in,out] _out     the transcript.
 * @param[in]     _label   what was done.
 * @param[in]     _counts  the two counts.
 */
static void
d_tests_arg_command_put_counted(
    struct d_arg_buffer* _out,
    const char*          _label,
    const int*           _counts
)
{
    (void)d_arg_buffer_printf(_out,
                              "%s first=%d second=%d\n",
                              _label,
                              _counts[0],
                              _counts[1]);

    return;
}

/**
 * @brief Gives a command a final callback that does nothing, with a count
 *        as its context, then appends the counts.
 *
 * @param[in,out] _out      the transcript.
 * @param[in,out] _command  the command; may be NULL.
 * @param[in,out] _counts   the two counts.
 * @param[in]     _which    which count is the context.
 * @param[in]     _label    the line's label.
 */
static void
d_tests_arg_command_final_given(
    struct d_arg_buffer*  _out,
    struct d_arg_command* _command,
    int*                  _counts,
    size_t                _which,
    const char*           _label
)
{
    d_arg_command_set_final_callback(_command,
                                     d_tests_arg_command_do_nothing,
                                     &_counts[_which],
                                     d_tests_arg_command_count_release);
    d_tests_arg_command_put_counted(_out,
                                    _label,
                                    _counts);

    return;
}

/**
 * @brief Appends DEPARTED_CONTEXTS' final callbacks: given, given again,
 *        replaced, removed, given to no command, a usage function given to
 *        no command, then given and moved by immediate_callback, with
 *        where it went.
 *
 * @param[in,out] _out     the transcript.
 * @param[in,out] _app     the root.
 * @param[in,out] _counts  the two counts, which are the contexts.
 */
static void
d_tests_arg_command_final_contexts(
    struct d_arg_buffer*  _out,
    struct d_arg_command* _app,
    int*                  _counts
)
{
    d_tests_arg_command_final_given(_out,
                                    _app,
                                    _counts,
                                    0u,
                                    "given");
    d_tests_arg_command_final_given(_out,
                                    _app,
                                    _counts,
                                    0u,
                                    "given again");
    d_tests_arg_command_final_given(_out,
                                    _app,
                                    _counts,
                                    1u,
                                    "replaced");
    d_arg_command_set_final_callback(_app,
                                     NULL,
                                     &_counts[0],
                                     d_tests_arg_command_count_release);
    d_tests_arg_command_put_counted(_out,
                                    "removed",
                                    _counts);
    d_tests_arg_command_final_given(_out,
                                    NULL,
                                    _counts,
                                    0u,
                                    "no command");
    d_arg_command_set_usage_fn(NULL,
                               NULL,
                               &_counts[1],
                               d_tests_arg_command_count_release);
    d_tests_arg_command_put_counted(_out,
                                    "no command for usage",
                                    _counts);
    d_arg_command_set_final_callback(_app,
                                     d_tests_arg_command_do_nothing,
                                     &_counts[0],
                                     d_tests_arg_command_count_release);
    d_arg_command_set_immediate_callback(_app,
                                         true);
    (void)d_arg_buffer_printf(_out,
                              "moved final=%d complete=%d\n",
                              (int)(_app->final_fn != NULL),
                              (int)(_app->parse_complete_fn != NULL));

    return;
}

/**
 * @brief Appends DEPARTED_CONTEXTS' version functions on s: given, given
 *        again, replaced, removed, refused a taken name, given to no
 *        command, then given and left for the parser to release.
 *
 * @param[in,out] _out     the transcript.
 * @param[in,out] _sub     the subcommand.
 * @param[in,out] _counts  the two counts, which are the contexts.
 */
static void
d_tests_arg_command_version_contexts(
    struct d_arg_buffer*  _out,
    struct d_arg_command* _sub,
    int*                  _counts
)
{
    // each step, then the counts
    for (size_t index = 0u; index < VERSION_STEP_COUNT; ++index)
    {
        const struct d_tests_arg_command_version_step* const step =
            &VERSION_STEPS[index];

        (void)d_arg_command_set_version_fn(
                  (step->command) ? _sub : NULL,
                  step->names,
                  d_tests_arg_command_write_words,
                  &_counts[step->context],
                  d_tests_arg_command_count_release,
                  NULL);
        d_tests_arg_command_put_counted(_out,
                                        step->label,
                                        _counts);
    }

    return;
}

/*
d_tests_arg_command_contexts
  The contexts a command's functions are given, each released by the
function given with it, once, when nothing keeps it any more; CLI11's
std::function callbacks own what they hold.
  Tests the following:
  - a callback's context released when it is replaced, not when it is
    given again; a NULL function releasing both the old context and the
    one given; a context given to no command released at once
  - immediate_callback moving a final callback, with its context, to the
    parse-complete one
  - a version function's context, which the flag keeps, released when the
    flag is replaced or removed, or refused, not when given again
  - every context still kept released with the parser
*/
bool
d_tests_arg_command_contexts(void)
{
    struct d_tests_arg_run      run       = { 0 };
    struct d_arg_command* const app       = d_tests_arg_begin(&run);
    struct d_arg_command* const sub       =
        d_arg_command_add_subcommand(app,
                                     "s",
                                     NULL);
    struct d_arg_buffer         out       = D_ARG_BUFFER_INIT;
    int                         counts[2] = { 0, 0 };

    (void)d_arg_command_add_option(sub,
                                   "--taken",
                                   NULL);
    d_tests_arg_command_final_contexts(&out,
                                       app,
                                       counts);
    d_tests_arg_command_version_contexts(&out,
                                         sub,
                                         counts);
    d_tests_arg_end(&run);
    d_tests_arg_command_put_counted(&out,
                                    "parser released",
                                    counts);

    const bool same = d_tests_arg_same("DEPARTED_CONTEXTS",
                                       d_arg_buffer_text(&out),
                                       DEPARTED_CONTEXTS);

    d_arg_buffer_release(&out);

    return same;
}

/**
 * @brief Appends DEPARTED_RANGE's counts: the requirements of subcommands
 *        and options after each is given the most negative `int`.
 *
 * @param[in,out] _out  the transcript.
 * @param[in,out] _app  the root.
 */
static void
d_tests_arg_command_counts_in_range(
    struct d_arg_buffer*  _out,
    struct d_arg_command* _app
)
{
    d_arg_command_require_subcommand_count(_app,
                                           INT_MIN);
    d_arg_command_require_option_count(_app,
                                       INT_MIN);
    (void)d_arg_buffer_append_cstr(_out,
                                   "counts subs=");
    d_tests_arg_command_put_required(_out,
                                     _app,
                                     false,
                                     false);
    (void)d_arg_buffer_append_cstr(_out,
                                   " opts=");
    d_tests_arg_command_put_required(_out,
                                     _app,
                                     true,
                                     false);
    (void)d_arg_buffer_append_char(_out,
                                   '\n');

    return;
}

/**
 * @brief Appends DEPARTED_RANGE's values outside their enums: the root's
 *        modes of extras, configuration extras and prefix commands, then
 *        its option defaults' policy, priority, integer rule and the flags
 *        FLAG_LIKE, which the defaults do not take, and REQUIRED, which
 *        they do.
 *
 * @param[in,out] _out  the transcript.
 * @param[in,out] _app  the root.
 */
static void
d_tests_arg_command_modes_in_range(
    struct d_arg_buffer*  _out,
    struct d_arg_command* _app
)
{
    struct d_arg_option_defaults* const defaults =
        d_arg_command_option_defaults(_app);

    d_arg_command_set_allow_extras(_app,
                                   99);
    d_arg_command_set_allow_config_extras(_app,
                                          -1);
    d_arg_command_set_prefix_command(_app,
                                     7);
    d_arg_option_defaults_set_multi_option_policy(defaults,
                                                  7);
    d_arg_option_defaults_set_callback_priority(defaults,
                                                99);
    d_arg_option_defaults_set_integer_rule(defaults,
                                           9);
    d_arg_option_defaults_set_flag(defaults,
                                   D_ARG_OPTION_FLAG_LIKE,
                                   true);
    d_arg_option_defaults_set_flag(defaults,
                                   D_ARG_OPTION_REQUIRED,
                                   true);
    (void)d_arg_buffer_printf(
        _out,
        "modes extras=%d cfg=%d prefix=%d\n"
        "defaults policy=%d priority=%d rule=%d flag_like=%d required=%d\n",
        _app->allow_extras,
        _app->allow_config_extras,
        _app->prefix_command,
        defaults->multi_option_policy,
        defaults->callback_priority,
        defaults->integer_rule,
        (int)((defaults->flags & D_ARG_OPTION_FLAG_LIKE) != 0u),
        (int)((defaults->flags & D_ARG_OPTION_REQUIRED) != 0u));

    return;
}

/*
d_tests_arg_command_out_of_range
  Counts and values a C caller can give that no CLI11 call can: the most
negative `int` as a count, values outside their enums, and NULL names.
  Tests the following:
  - require_subcommand and require_option given the most negative `int`
    allow at most 2^31
  - modes, policies, priorities and integer rules outside their enums, and
    flags the option defaults do not take, ignored
  - an option group given a NULL name is in the empty group
*/
bool
d_tests_arg_command_out_of_range(void)
{
    struct d_tests_arg_run      run = { 0 };
    struct d_arg_command* const app = d_tests_arg_begin(&run);
    struct d_arg_buffer         out = D_ARG_BUFFER_INIT;

    d_tests_arg_command_counts_in_range(&out,
                                        app);
    d_tests_arg_command_modes_in_range(&out,
                                       app);
    (void)d_arg_buffer_printf(
        &out,
        "group '%s'\n",
        d_arg_command_group(d_arg_command_add_option_group(app,
                                                           NULL,
                                                           NULL)));

    const bool same = d_tests_arg_same("DEPARTED_RANGE",
                                       d_arg_buffer_text(&out),
                                       DEPARTED_RANGE);

    d_arg_buffer_release(&out);
    d_tests_arg_end(&run);

    return same;
}

//==============================================================================
// 7.  THE SUITE
//==============================================================================

// d_tests_arg_command
//   constant: the suite, in the header's order.
const struct d_tests_section d_tests_arg_command[] =
{
    { "arg_command_add",
      d_tests_arg_command_add },
    { "arg_command_listed",
      d_tests_arg_command_listed },
    { "arg_command_flags",
      d_tests_arg_command_flags },
    { "arg_command_flag_bound",
      d_tests_arg_command_flag_bound },
    { "arg_command_flag_sums",
      d_tests_arg_command_flag_sums },
    { "arg_command_special",
      d_tests_arg_command_special },
    { "arg_command_subcommands",
      d_tests_arg_command_subcommands },
    { "arg_command_moves",
      d_tests_arg_command_moves },
    { "arg_command_removal",
      d_tests_arg_command_removal },
    { "arg_command_names",
      d_tests_arg_command_names },
    { "arg_command_settings",
      d_tests_arg_command_settings },
    { "arg_command_usage_text",
      d_tests_arg_command_usage_text },
    { "arg_command_callbacks",
      d_tests_arg_command_callbacks },
    { "arg_command_relations",
      d_tests_arg_command_relations },
    { "arg_command_relation_parses",
      d_tests_arg_command_relation_parses },
    { "arg_command_checks",
      d_tests_arg_command_checks },
    { "arg_command_display",
      d_tests_arg_command_display },
    { "arg_command_parsed",
      d_tests_arg_command_parsed },
    { "arg_command_triggers",
      d_tests_arg_command_triggers },
    { "arg_command_lifecycle",
      d_tests_arg_command_lifecycle },
    { "arg_command_deprecated_named",
      d_tests_arg_command_deprecated_named },
    { "arg_command_unread_sums",
      d_tests_arg_command_unread_sums },
    { "arg_command_decimal_sums",
      d_tests_arg_command_decimal_sums },
    { "arg_command_swept",
      d_tests_arg_command_swept },
    { "arg_command_kept_in_place",
      d_tests_arg_command_kept_in_place },
    { "arg_command_kept_apart",
      d_tests_arg_command_kept_apart },
    { "arg_command_moved_links",
      d_tests_arg_command_moved_links },
    { "arg_command_trigger_swept",
      d_tests_arg_command_trigger_swept },
    { "arg_command_contexts",
      d_tests_arg_command_contexts },
    { "arg_command_out_of_range",
      d_tests_arg_command_out_of_range }
};

// d_tests_arg_command_count
//   constant: how many sections the table above holds.
const size_t d_tests_arg_command_count =
    sizeof(d_tests_arg_command) / sizeof(d_tests_arg_command[0]);
