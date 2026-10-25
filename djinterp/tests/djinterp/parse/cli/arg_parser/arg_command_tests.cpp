/*******************************************************************************
* djinterp [parse]                                         arg_command_tests.cpp
*
* The argument parser's C++ command suite's sections.
*   The sections held to CLI11 run what the CASES section defines: commands
* given options, flags, subcommands and groups, set, read and parsed through
* the parser, a subcommand's handle and an option group's, as a program
* would with CLI11's App and its App*. tools/cppref/cppref.py compiles that
* section against CLI11 2.7.2, through the face's names for CLI11's
* (cli11_shim.hpp), and the transcripts below are what CLI11 wrote; so the
* CASES section is written in the part of the face CLI11 shares -- `->` on
* every handle, a call on the parser itself a statement of its own, no
* status taken from a call, a handle on nothing told by comparing it with
* one made bare -- and C++98 unless a guard says otherwise. A definition
* CLI11 refuses is the last statement of its step, since CLI11 throws there.
* The sections that are the module's own are written apart, after it, each
* saying why CLI11 cannot write its transcript.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/arg_command_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.19
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./arg_command_tests.hpp"  // corresponding header
// std
#include <cstddef>    // std::size_t
#include <sstream>    // std::ostringstream
#include <string>     // std::string
#include <vector>     // std::vector
// djinterp
#include "../../../../../inc/djinterp/djinterp.hpp"  // framework root
#include "../../../../../inc/djinterp/parse/cli/arg_parser/arg_parser.hpp"  // the face
#include "./arg_tests_support.hpp"  // run, steps, transcripts
#include "../../c/tests_section.h"  // d_tests_section


namespace
{

namespace cli = ::djinterp::parse::cli;

using cli::testing::run;
using cli::testing::step_fn;
using cli::testing::escape;
using cli::testing::number;
using cli::testing::steps_hold;

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// ADD
//   constant: what the steps of d_tests_arg_cpp_command_add write, as CLI11
// writes them.
const char ADD[] =
    "plain: description=[] type=[] default=[] group=[OPTIONS] expected=1,1 "
    "type_size=1,1 policy=Throw for_default=0 extra_args=0 required=0\n"
    "described: description=[a description] type=[] default=[] group=[OPTIONS] "
    "expected=1,1 type_size=1,1 policy=Throw for_default=0 extra_args=0 "
    "required=0\n"
    "whole: description=[] type=[INT] default=[] group=[OPTIONS] expected=1,1 "
    "type_size=1,1 policy=Throw for_default=1 extra_args=0 required=0\n"
    "word: description=[a word] type=[TEXT] default=[] group=[OPTIONS] "
    "expected=1,1 type_size=1,1 policy=Throw for_default=1 extra_args=0 "
    "required=0\n"
    "list: description=[] type=[INT] default=[] group=[OPTIONS] "
    "expected=1,536870912 type_size=1,1 policy=Throw for_default=1 "
    "extra_args=1 required=0\n"
    "constant: description=[a const description] type=[] default=[] "
    "group=[OPTIONS] expected=1,1 type_size=1,1 policy=Throw for_default=0 "
    "extra_args=0 required=0\n"
    "quiet: description=[] type=[TEXT] default=[] group=[OPTIONS] expected=1,1 "
    "type_size=1,1 policy=Throw for_default=1 extra_args=0 required=0\n"
    "cmd app n=1 miss=[] subs=[] "
    "order=[--plain|--described|--number|--word|--list|--list|--constant|--quie"
    "t]\n"
    "opt plain n=1 res=[p]\n"
    "opt described n=1 res=[d]\n"
    "opt whole n=1 res=[7]\n"
    "opt word n=1 res=[w]\n"
    "opt list n=2 res=[1|2]\n"
    "opt constant n=1 res=[c]\n"
    "opt quiet n=1 res=[q]\n"
    "variables: whole=7 word=w list=1,2 quiet=q\n"
    "inner: description=[an inner value] type=[INT] default=[] group=[OPTIONS] "
    "expected=1,1 type_size=1,1 policy=Throw for_default=1 extra_args=0 "
    "required=0\n"
    "outer: description=[an outer value] type=[] default=[] group=[OPTIONS] "
    "expected=1,1 type_size=1,1 policy=Throw for_default=0 extra_args=0 "
    "required=0\n"
    "grouped: description=[] type=[TEXT] default=[] group=[OPTIONS] "
    "expected=1,1 type_size=1,1 policy=Throw for_default=1 extra_args=0 "
    "required=0\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[] subs=[] order=[--inner|--outer]\n"
    "cmd Grouped n=1 miss=[] subs=[] order=[--grouped]\n"
    "opt inner n=1 res=[4]\n"
    "opt outer n=1 res=[o]\n"
    "opt grouped n=1 res=[g]\n"
    "variables: inner=4 grouped=g\n"
    "whole: description=[a whole number] type=[INT] default=[] group=[OPTIONS] "
    "expected=1,1 type_size=1,1 policy=Throw for_default=0 extra_args=0 "
    "required=0\n"
    "text: description=[] type=[TEXT] default=[] group=[OPTIONS] expected=1,1 "
    "type_size=1,1 policy=Throw for_default=0 extra_args=0 required=0\n"
    "function int 42\n"
    "function text abc\n"
    "cmd app n=1 miss=[] subs=[] order=[--whole|--text]\n"
    "opt whole n=1 res=[42]\n"
    "opt text n=1 res=[abc]\n"
    "ERR ConversionError 104: Could not convert: --whole = x\n"
    "cmd app n=1 miss=[] subs=[] order=[--whole]\n"
    "opt whole n=1 res=[x]\n"
    "opt text n=0 res=[]\n"
    "THROWN OptionAlreadyAdded: cannot add '--beta,-a': short name '-a' "
    "already used by '--alpha'\n"
    "THROWN BadNameString: Bad long name: bad name\n";

// FLAGS
//   constant: what the steps of d_tests_arg_cpp_command_flags write, as
// CLI11 writes them.
const char FLAGS[] =
    "flag: description=[] type=[] default=[] group=[OPTIONS] expected=0,0 "
    "type_size=1,1 policy=TakeLast for_default=0 extra_args=0 required=0\n"
    "described: description=[a flag described] type=[] default=[] "
    "group=[OPTIONS] expected=0,0 type_size=1,1 policy=TakeLast for_default=0 "
    "extra_args=0 required=0\n"
    "truly: description=[] type=[] default=[] group=[OPTIONS] expected=0,0 "
    "type_size=1,1 policy=TakeLast for_default=0 extra_args=0 required=0\n"
    "counted: description=[] type=[] default=[0] group=[OPTIONS] expected=0,0 "
    "type_size=1,1 policy=Sum for_default=0 extra_args=0 required=0\n"
    "levels: description=[] type=[] default=[] group=[OPTIONS] expected=0,0 "
    "type_size=1,1 policy=TakeAll for_default=1 extra_args=0 required=0\n"
    "cmd app n=1 miss=[] subs=[] "
    "order=[--flag|--truly|-c|-c|-c|--level|--level|--described]\n"
    "opt flag n=1 res=[true]\n"
    "opt described n=1 res=[true]\n"
    "opt truly n=1 res=[true]\n"
    "opt counted n=3 res=[true|true|true]\n"
    "opt levels n=2 res=[2|5]\n"
    "variables: truly=1 counted=3 levels=2,5\n"
    "cmd app n=1 miss=[] subs=[] order=[--color]\n"
    "opt color n=1 res=[false]\n"
    "color=0\n"
    "cmd app n=1 miss=[] subs=[] order=[--color|--color]\n"
    "opt color n=2 res=[false|true]\n"
    "color=1\n"
    "cmd app n=1 miss=[] subs=[] order=[--color]\n"
    "opt color n=1 res=[false]\n"
    "color=0\n"
    "ERR ArgumentMismatch 114: color was given a disallowed flag override\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt color n=0 res=[]\n"
    "color=0\n"
    "cb: description=[calls back] type=[] default=[] group=[OPTIONS] "
    "expected=0,0 type_size=1,1 policy=TakeLast for_default=0 extra_args=0 "
    "required=0\n"
    "verbose: description=[] type=[] default=[] group=[OPTIONS] expected=0,0 "
    "type_size=1,1 policy=Sum for_default=0 extra_args=0 required=0\n"
    "flag callback\n"
    "flag function 4\n"
    "cmd app n=1 miss=[] subs=[] "
    "order=[--cb|--verbose|--verbose|--verbose|--verbose|--cb]\n"
    "opt cb n=2 res=[true|true]\n"
    "opt verbose n=4 res=[true|true|true|true]\n"
    "THROWN IncorrectConstruction: positional: Flags cannot be positional\n";

// SPECIAL
//   constant: what the steps of d_tests_arg_cpp_command_special write, as
// CLI11 writes them.
const char SPECIAL[] =
    "made: help=--help help_all=none version=none config=none\n"
    "help: description=[Print this help message and exit] type=[] default=[] "
    "group=[OPTIONS] expected=0,0 type_size=1,1 policy=TakeLast for_default=0 "
    "extra_args=0 required=0\n"
    "version=[]\n"
    "set: help=--usage help_all=--help-all version=--version config=none\n"
    "version: description=[show the version] type=[] default=[] "
    "group=[OPTIONS] expected=0,0 type_size=1,1 policy=TakeLast for_default=0 "
    "extra_args=0 required=0\n"
    "version=[1.2.3]\n"
    "ERR CallForVersion 0: 1.2.3\n"
    "cmd app n=1 miss=[] subs=[] order=[--version]\n"
    "exit 0\n"
    "out 1.2.3\\n\n"
    "ERR CallForAllHelp 0: This should be caught in your main function, see "
    "examples\n"
    "cmd app n=1 miss=[] subs=[] order=[--help-all]\n"
    "exit 0\n"
    "out d\\n\\n\\nprog [OPTIONS]\\n\\n\\nOPTIONS:\\n  -n                      "
    "    a number\\n  -?,     --usage             show the usage\\n          "
    "--help-all          show everything\\n  -V,     --version           show "
    "the version\\n\n"
    "ERR CallForHelp 0: This should be caught in your main function, see "
    "examples\n"
    "cmd app n=1 miss=[] subs=[] order=[--usage]\n"
    "exit 0\n"
    "out d\\n\\n\\nprog [OPTIONS]\\n\\n\\nOPTIONS:\\n  -n                      "
    "    a number\\n  -?,     --usage             show the usage\\n          "
    "--help-all          show everything\\n  -V,     --version           show "
    "the version\\n\n"
    "version=[made 9.9]\n"
    "ERR CallForVersion 0: made 9.9\n"
    "cmd app n=1 miss=[] subs=[] order=[--version]\n"
    "exit 0\n"
    "out made 9.9\\n\n"
    "removed: help=none help_all=none version=none config=none\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--help\n"
    "cmd app n=1 miss=[--help] subs=[] order=[]\n";

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// SPECIAL_CONFIG
//   constant: what the steps of d_tests_arg_cpp_command_special_config
// write, as CLI11 writes them.
const char SPECIAL_CONFIG[] =
    "config: help=--help help_all=none version=none config=--config\n"
    "config: description=[the configuration] type=[] "
    "default=[no_such_settings_file.ini] group=[OPTIONS] expected=1,1 "
    "type_size=1,1 policy=Reverse for_default=0 extra_args=0 required=0\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "ERR FileError 103: no_such_file_anywhere.ini was not readable (missing?)\n"
    "cmd app n=1 miss=[] subs=[] order=[--config]\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

// SUBCOMMANDS
//   constant: what the steps of d_tests_arg_cpp_command_subcommands write, as
// CLI11 writes them.
const char SUBCOMMANDS[] =
    "by name=alpha by index=beta by handle=beta by alias=beta none=none\n"
    "parents: alpha=prog root=none\n"
    "alpha text: name=[alpha] description=[] group=[SUBCOMMANDS] "
    "display=[alpha|alpha] aliases=[] usage=[] footer=[]\n"
    "beta text: name=[beta] description=[the second] group=[SUBCOMMANDS] "
    "display=[beta|beta, b] aliases=[b] usage=[] footer=[]\n"
    "aliased text: name=[alpha] description=[] group=[SUBCOMMANDS] "
    "display=[alpha|alpha, al, first] aliases=[al,first] usage=[] footer=[]\n"
    "matches: alpha=1,exact al=1,exact first=1,exact alp=0,none ALPHA=0,none "
    "al_pha=0,none fir=0,none\n"
    "matches: alpha=1,exact al=1,prefix first=1,exact alp=1,prefix "
    "ALPHA=1,exact al_pha=1,exact fir=1,prefix\n"
    "cleared text: name=[alpha] description=[] group=[SUBCOMMANDS] "
    "display=[alpha|alpha] aliases=[] usage=[] footer=[]\n"
    "cmd app n=1 miss=[] subs=[alpha] order=[]\n"
    "cmd alpha n=1 miss=[] subs=[] order=[]\n"
    "one or more modes: extras=0,Error config_extras=Ignore prefix=0,Off "
    "subcommands=1,0 options=0,0\n"
    "ERR RequiredError 106: A subcommand is required\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd alpha n=0 miss=[] subs=[] order=[]\n"
    "cmd beta n=0 miss=[] subs=[] order=[]\n"
    "cmd app n=1 miss=[] subs=[alpha|beta] order=[]\n"
    "cmd alpha n=1 miss=[] subs=[] order=[]\n"
    "cmd beta n=1 miss=[] subs=[] order=[]\n"
    "exactly one modes: extras=0,Error config_extras=Ignore prefix=0,Off "
    "subcommands=1,1 options=0,0\n"
    "ERR ExtrasError 109: alpha: The following argument was not expected: "
    "beta\n"
    "cmd app n=1 miss=[] subs=[alpha] order=[]\n"
    "cmd alpha n=1 miss=[beta] subs=[] order=[]\n"
    "cmd beta n=0 miss=[] subs=[] order=[]\n"
    "at most one modes: extras=0,Error config_extras=Ignore prefix=0,Off "
    "subcommands=0,1 options=0,0\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd alpha n=0 miss=[] subs=[] order=[]\n"
    "cmd beta n=0 miss=[] subs=[] order=[]\n"
    "two modes: extras=0,Error config_extras=Ignore prefix=0,Off "
    "subcommands=2,2 options=0,0\n"
    "ERR RequiredError 106: Requires at least 2 subcommands\n"
    "cmd app n=1 miss=[] subs=[alpha] order=[]\n"
    "cmd alpha n=1 miss=[] subs=[] order=[]\n"
    "cmd beta n=0 miss=[] subs=[] order=[]\n"
    "ERR ExtrasError 109: alpha: The following arguments were not expected: "
    "--top 1\n"
    "cmd app n=1 miss=[] subs=[alpha] order=[]\n"
    "cmd alpha n=1 miss=[--top|1] subs=[] order=[]\n"
    "cmd beta n=0 miss=[] subs=[] order=[]\n"
    "opt top n=0 res=[]\n"
    "cmd app n=1 miss=[] subs=[alpha] order=[--top]\n"
    "cmd alpha n=1 miss=[] subs=[] order=[]\n"
    "cmd beta n=0 miss=[] subs=[] order=[]\n"
    "opt top n=1 res=[1]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: beta\n"
    "cmd app n=1 miss=[beta] subs=[] order=[]\n"
    "cmd alpha n=0 miss=[] subs=[] order=[]\n"
    "cmd beta n=0 miss=[] subs=[] order=[]\n"
    "opt top n=0 res=[]\n";

// GROUPS
//   constant: what the steps of d_tests_arg_cpp_command_groups write, as
// CLI11 writes them where a command does not start allowing Windows-style
// options.
const char GROUPS[] =
    "moved: option=1 command=1 found=1\n"
    "group text: name=[] description=[the group] group=[Grouped] "
    "display=[[Option Group: Grouped]|[Option Group: Grouped]] aliases=[] "
    "usage=[] footer=[]\n"
    "in the group: --inside,--moved\n"
    "cmd app n=1 miss=[] subs=[gamma] order=[]\n"
    "cmd gamma n=1 miss=[] subs=[] order=[]\n"
    "cmd Grouped n=1 miss=[] subs=[gamma] order=[--moved|--inside]\n"
    "opt moved n=1 res=[1]\n"
    "opt inside n=1 res=[2]\n"
    "choice modes: extras=0,Error config_extras=Ignore prefix=0,Off "
    "subcommands=0,0 options=1,1\n"
    "ERR RequiredError 106: Exactly 1 option from [--first,--second] is "
    "required\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd Choice n=1 miss=[] subs=[] order=[]\n"
    "opt first n=0 res=[]\n"
    "opt second n=0 res=[]\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd Choice n=1 miss=[] subs=[] order=[--first]\n"
    "opt first n=1 res=[1]\n"
    "opt second n=0 res=[]\n"
    "ERR RequiredError 106: Exactly 1 option from [--first,--second] is "
    "required but 2 were given\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd Choice n=1 miss=[] subs=[] order=[--first|--second]\n"
    "opt first n=1 res=[1]\n"
    "opt second n=1 res=[2]\n"
    "choice switches: required=1 disabled=0 silent=0 windows=0 at_end=0 "
    "configurable=0 validate=0 validate_optional=0 non_standard=0 "
    "prefix_matching=0 fallthrough=0 sub_fallthrough=1 case=0 underscore=0 "
    "immediate=0 off_by_default=0 on_by_default=0\n"
    "ERR RequiredError 106: [Option Group: Choice] is required\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd Choice n=1 miss=[] subs=[] order=[]\n"
    "opt first n=0 res=[]\n"
    "opt second n=0 res=[]\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd Choice n=1 miss=[] subs=[] order=[--first|--second]\n"
    "opt first n=1 res=[1]\n"
    "opt second n=1 res=[2]\n";

// GROUPS_WINDOWS
//   constant: GROUPS where a command starts allowing them, as CLI11's does on
// Windows -- and the module's where its setting for them says so -- and a
// subcommand takes the setting from its parent.
const char GROUPS_WINDOWS[] =
    "moved: option=1 command=1 found=1\n"
    "group text: name=[] description=[the group] group=[Grouped] "
    "display=[[Option Group: Grouped]|[Option Group: Grouped]] aliases=[] "
    "usage=[] footer=[]\n"
    "in the group: --inside,--moved\n"
    "cmd app n=1 miss=[] subs=[gamma] order=[]\n"
    "cmd gamma n=1 miss=[] subs=[] order=[]\n"
    "cmd Grouped n=1 miss=[] subs=[gamma] order=[--moved|--inside]\n"
    "opt moved n=1 res=[1]\n"
    "opt inside n=1 res=[2]\n"
    "choice modes: extras=0,Error config_extras=Ignore prefix=0,Off "
    "subcommands=0,0 options=1,1\n"
    "ERR RequiredError 106: Exactly 1 option from [--first,--second] is "
    "required\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd Choice n=1 miss=[] subs=[] order=[]\n"
    "opt first n=0 res=[]\n"
    "opt second n=0 res=[]\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd Choice n=1 miss=[] subs=[] order=[--first]\n"
    "opt first n=1 res=[1]\n"
    "opt second n=0 res=[]\n"
    "ERR RequiredError 106: Exactly 1 option from [--first,--second] is "
    "required but 2 were given\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd Choice n=1 miss=[] subs=[] order=[--first|--second]\n"
    "opt first n=1 res=[1]\n"
    "opt second n=1 res=[2]\n"
    "choice switches: required=1 disabled=0 silent=0 windows=1 at_end=0 "
    "configurable=0 validate=0 validate_optional=0 non_standard=0 "
    "prefix_matching=0 fallthrough=0 sub_fallthrough=1 case=0 underscore=0 "
    "immediate=0 off_by_default=0 on_by_default=0\n"
    "ERR RequiredError 106: [Option Group: Choice] is required\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd Choice n=1 miss=[] subs=[] order=[]\n"
    "opt first n=0 res=[]\n"
    "opt second n=0 res=[]\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd Choice n=1 miss=[] subs=[] order=[--first|--second]\n"
    "opt first n=1 res=[1]\n"
    "opt second n=1 res=[2]\n";

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// GROUPS_SEVERAL
//   constant: what the steps of d_tests_arg_cpp_command_groups_several
// write, as CLI11 writes them.
const char GROUPS_SEVERAL[] =
    "in the group: --one,--two,--three\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd Several n=1 miss=[] subs=[] order=[--one|--two]\n"
    "opt one n=1 res=[1]\n"
    "opt two n=1 res=[2]\n"
    "opt three n=0 res=[]\n";

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// REMOVAL
//   constant: what the steps of d_tests_arg_cpp_command_removal write, as
// CLI11 writes them.
const char REMOVAL[] =
    "removed: option=1 foreign=0 needs=0\n"
    "ERR ExtrasError 109: prog: The following arguments were not expected: "
    "--gone 2\n"
    "cmd app n=1 miss=[--gone|2] subs=[] order=[--kept]\n"
    "opt kept n=1 res=[1]\n"
    "removed: subcommand=1 foreign=0 left=none\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: gone\n"
    "cmd app n=1 miss=[gone] subs=[] order=[]\n"
    "cmd kept n=0 miss=[] subs=[] order=[]\n";

// NAMES
//   constant: what the steps of d_tests_arg_cpp_command_names write, as CLI11
// writes them.
const char NAMES[] =
    "made text: name=[prog] description=[d] group=[SUBCOMMANDS] "
    "display=[prog|prog] aliases=[] usage=[] footer=[]\n"
    "root text: name=[renamed] description=[the program] group=[SUBCOMMANDS] "
    "display=[renamed|renamed] aliases=[] usage=[prog [things]] footer=[the "
    "end]\n"
    "text text: name=[sub] description=[a subcommand] group=[Commands] "
    "display=[sub|sub, s] aliases=[s] usage=[sub usage] footer=[sub footer]\n"
    "functions text: name=[named] description=[a subcommand] group=[Commands] "
    "display=[named|named, s] aliases=[s] usage=[usage made\\nsub usage] "
    "footer=[footer made\\nsub footer]\n";

// SETTINGS
//   constant: what the steps of d_tests_arg_cpp_command_settings write, as
// CLI11 writes them where a command does not start allowing Windows-style
// options.
const char SETTINGS[] =
    "root switches: required=0 disabled=0 silent=0 windows=0 at_end=0 "
    "configurable=0 validate=0 validate_optional=0 non_standard=0 "
    "prefix_matching=0 fallthrough=0 sub_fallthrough=1 case=0 underscore=0 "
    "immediate=0 off_by_default=0 on_by_default=0\n"
    "root modes: extras=0,Error config_extras=Ignore prefix=0,Off "
    "subcommands=0,0 options=0,0\n"
    "sub switches: required=0 disabled=0 silent=0 windows=0 at_end=0 "
    "configurable=0 validate=0 validate_optional=0 non_standard=0 "
    "prefix_matching=0 fallthrough=0 sub_fallthrough=1 case=0 underscore=0 "
    "immediate=0 off_by_default=0 on_by_default=0\n"
    "sub modes: extras=0,Error config_extras=Ignore prefix=0,Off "
    "subcommands=0,0 options=0,0\n"
    "group switches: required=0 disabled=0 silent=0 windows=0 at_end=0 "
    "configurable=0 validate=0 validate_optional=0 non_standard=0 "
    "prefix_matching=0 fallthrough=0 sub_fallthrough=1 case=0 underscore=0 "
    "immediate=0 off_by_default=0 on_by_default=0\n"
    "group modes: extras=0,Error config_extras=Ignore prefix=0,Off "
    "subcommands=0,0 options=0,0\n"
    "root switches: required=1 disabled=1 silent=1 windows=1 at_end=1 "
    "configurable=1 validate=1 validate_optional=1 non_standard=1 "
    "prefix_matching=1 fallthrough=1 sub_fallthrough=0 case=1 underscore=1 "
    "immediate=1 off_by_default=1 on_by_default=0\n"
    "root modes: extras=1,Capture config_extras=Capture prefix=1,On "
    "subcommands=2,2 options=1,3\n"
    "set switches: required=1 disabled=1 silent=1 windows=1 at_end=1 "
    "configurable=0 validate=1 validate_optional=1 non_standard=1 "
    "prefix_matching=1 fallthrough=1 sub_fallthrough=0 case=1 underscore=1 "
    "immediate=1 off_by_default=0 on_by_default=1\n"
    "cleared switches: required=0 disabled=0 silent=0 windows=0 at_end=0 "
    "configurable=1 validate=0 validate_optional=0 non_standard=0 "
    "prefix_matching=0 fallthrough=0 sub_fallthrough=1 case=0 underscore=0 "
    "immediate=0 off_by_default=0 on_by_default=0\n"
    "group switches: required=1 disabled=0 silent=1 windows=0 at_end=0 "
    "configurable=0 validate=0 validate_optional=0 non_standard=0 "
    "prefix_matching=0 fallthrough=0 sub_fallthrough=1 case=0 underscore=0 "
    "immediate=0 off_by_default=1 on_by_default=0\n"
    "first modes: extras=0,ErrorImmediately config_extras=IgnoreAll "
    "prefix=1,SeparatorOnly subcommands=0,2 options=1,0\n"
    "second modes: extras=1,Capture config_extras=Capture "
    "prefix=1,PositionalOnly subcommands=0,4 options=0,3\n"
    "third modes: extras=1,AssumeMultipleArguments config_extras=Error "
    "prefix=0,Off subcommands=1,0 options=2,2\n"
    "fourth modes: extras=0,Ignore config_extras=Error prefix=0,Off "
    "subcommands=1,0 options=2,2\n"
    "fifth modes: extras=0,Error config_extras=Error prefix=1,On "
    "subcommands=1,0 options=2,2\n"
    "cmd app n=1 miss=[--nope|pos] subs=[] order=[--n]\n"
    "opt n n=1 res=[1]\n"
    "cmd app n=1 miss=[] subs=[] order=[--n]\n"
    "opt n n=1 res=[1]\n"
    "ERR ExtrasError 109: prog: The following arguments were not expected: "
    "--nope pos\n"
    "cmd app n=1 miss=[--nope|pos] subs=[] order=[--n]\n"
    "opt n n=1 res=[1]\n"
    "cmd app n=1 miss=[rest|--n|2] subs=[] order=[--n]\n"
    "opt n n=1 res=[1]\n";

// SETTINGS_WINDOWS
//   constant: SETTINGS where a command starts allowing them, as CLI11's does on
// Windows -- and the module's where its setting for them says so -- and a
// subcommand takes the setting from its parent.
const char SETTINGS_WINDOWS[] =
    "root switches: required=0 disabled=0 silent=0 windows=0 at_end=0 "
    "configurable=0 validate=0 validate_optional=0 non_standard=0 "
    "prefix_matching=0 fallthrough=0 sub_fallthrough=1 case=0 underscore=0 "
    "immediate=0 off_by_default=0 on_by_default=0\n"
    "root modes: extras=0,Error config_extras=Ignore prefix=0,Off "
    "subcommands=0,0 options=0,0\n"
    "sub switches: required=0 disabled=0 silent=0 windows=0 at_end=0 "
    "configurable=0 validate=0 validate_optional=0 non_standard=0 "
    "prefix_matching=0 fallthrough=0 sub_fallthrough=1 case=0 underscore=0 "
    "immediate=0 off_by_default=0 on_by_default=0\n"
    "sub modes: extras=0,Error config_extras=Ignore prefix=0,Off "
    "subcommands=0,0 options=0,0\n"
    "group switches: required=0 disabled=0 silent=0 windows=0 at_end=0 "
    "configurable=0 validate=0 validate_optional=0 non_standard=0 "
    "prefix_matching=0 fallthrough=0 sub_fallthrough=1 case=0 underscore=0 "
    "immediate=0 off_by_default=0 on_by_default=0\n"
    "group modes: extras=0,Error config_extras=Ignore prefix=0,Off "
    "subcommands=0,0 options=0,0\n"
    "root switches: required=1 disabled=1 silent=1 windows=1 at_end=1 "
    "configurable=1 validate=1 validate_optional=1 non_standard=1 "
    "prefix_matching=1 fallthrough=1 sub_fallthrough=0 case=1 underscore=1 "
    "immediate=1 off_by_default=1 on_by_default=0\n"
    "root modes: extras=1,Capture config_extras=Capture prefix=1,On "
    "subcommands=2,2 options=1,3\n"
    "set switches: required=1 disabled=1 silent=1 windows=1 at_end=1 "
    "configurable=0 validate=1 validate_optional=1 non_standard=1 "
    "prefix_matching=1 fallthrough=1 sub_fallthrough=0 case=1 underscore=1 "
    "immediate=1 off_by_default=0 on_by_default=1\n"
    "cleared switches: required=0 disabled=0 silent=0 windows=0 at_end=0 "
    "configurable=1 validate=0 validate_optional=0 non_standard=0 "
    "prefix_matching=0 fallthrough=0 sub_fallthrough=1 case=0 underscore=0 "
    "immediate=0 off_by_default=0 on_by_default=0\n"
    "group switches: required=1 disabled=0 silent=1 windows=1 at_end=0 "
    "configurable=0 validate=0 validate_optional=0 non_standard=0 "
    "prefix_matching=0 fallthrough=0 sub_fallthrough=1 case=0 underscore=0 "
    "immediate=0 off_by_default=1 on_by_default=0\n"
    "first modes: extras=0,ErrorImmediately config_extras=IgnoreAll "
    "prefix=1,SeparatorOnly subcommands=0,2 options=1,0\n"
    "second modes: extras=1,Capture config_extras=Capture "
    "prefix=1,PositionalOnly subcommands=0,4 options=0,3\n"
    "third modes: extras=1,AssumeMultipleArguments config_extras=Error "
    "prefix=0,Off subcommands=1,0 options=2,2\n"
    "fourth modes: extras=0,Ignore config_extras=Error prefix=0,Off "
    "subcommands=1,0 options=2,2\n"
    "fifth modes: extras=0,Error config_extras=Error prefix=1,On "
    "subcommands=1,0 options=2,2\n"
    "cmd app n=1 miss=[--nope|pos] subs=[] order=[--n]\n"
    "opt n n=1 res=[1]\n"
    "cmd app n=1 miss=[] subs=[] order=[--n]\n"
    "opt n n=1 res=[1]\n"
    "ERR ExtrasError 109: prog: The following arguments were not expected: "
    "--nope pos\n"
    "cmd app n=1 miss=[--nope|pos] subs=[] order=[--n]\n"
    "opt n n=1 res=[1]\n"
    "cmd app n=1 miss=[rest|--n|2] subs=[] order=[--n]\n"
    "opt n n=1 res=[1]\n";

// DEFAULTS
//   constant: what the steps of d_tests_arg_cpp_command_option_defaults
// write, as CLI11 writes them.
const char DEFAULTS[] =
    "made defaults: group=[OPTIONS] required=0 capture=0 configurable=1 "
    "no_override=0 case=0 underscore=0 delimiter=0 policy=Throw "
    "priority=Normal\n"
    "set defaults: group=[Defaulted] required=1 capture=1 configurable=0 "
    "no_override=1 case=1 underscore=1 delimiter=44 policy=TakeAll "
    "priority=Last\n"
    "made after: description=[] type=[] default=[] group=[Defaulted] "
    "expected=1,1 type_size=1,1 policy=TakeAll for_default=0 extra_args=0 "
    "required=1\n"
    "take_last defaults: group=[OPTIONS] required=0 capture=0 configurable=1 "
    "no_override=0 case=0 underscore=0 delimiter=0 policy=TakeLast "
    "priority=Normal\n"
    "take_first defaults: group=[OPTIONS] required=0 capture=0 configurable=1 "
    "no_override=0 case=0 underscore=0 delimiter=0 policy=TakeFirst "
    "priority=Normal\n"
    "join_semicolon defaults: group=[OPTIONS] required=0 capture=0 "
    "configurable=1 no_override=0 case=0 underscore=0 delimiter=59 policy=Join "
    "priority=Normal\n"
    "join defaults: group=[OPTIONS] required=1 capture=0 configurable=1 "
    "no_override=0 case=0 underscore=0 delimiter=59 policy=Join "
    "priority=Normal\n"
    "cleared defaults: group=[OPTIONS] required=0 capture=0 configurable=1 "
    "no_override=0 case=0 underscore=0 delimiter=0 policy=Throw "
    "priority=Normal\n"
    "sub defaults: group=[Parental] required=0 capture=0 configurable=1 "
    "no_override=0 case=1 underscore=0 delimiter=0 policy=Throw "
    "priority=Normal\n"
    "inner: description=[] type=[] default=[] group=[Parental] expected=1,1 "
    "type_size=1,1 policy=TakeLast for_default=0 extra_args=0 required=0\n"
    "outer: description=[] type=[] default=[] group=[Parental] expected=1,1 "
    "type_size=1,1 policy=Throw for_default=0 extra_args=0 required=0\n";

// CALLBACKS
//   constant: what the steps of d_tests_arg_cpp_command_callbacks write, as
// CLI11 writes them.
const char CALLBACKS[] =
    "alpha preparse 3\n"
    "alpha complete\n"
    "alpha final\n"
    "beta callback\n"
    "root callback\n"
    "cmd app n=1 miss=[] subs=[alpha|beta] order=[]\n"
    "cmd alpha n=1 miss=[] subs=[] order=[--x]\n"
    "cmd beta n=1 miss=[] subs=[] order=[]\n"
    "alpha callback\n"
    "beta callback\n"
    "alpha callback\n"
    "beta complete\n"
    "root callback\n"
    "cmd app n=1 miss=[] subs=[alpha|beta] order=[]\n"
    "cmd alpha n=2 miss=[] subs=[] order=[]\n"
    "cmd beta n=1 miss=[] subs=[] order=[]\n";

// RELATIONS
//   constant: what the steps of d_tests_arg_cpp_command_relations write, as
// CLI11 writes them.
const char RELATIONS[] =
    "ERR RequiresError 107: sub requires --needed\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[] subs=[] order=[]\n"
    "opt needed n=0 res=[]\n"
    "opt excluded n=0 res=[]\n"
    "cmd app n=1 miss=[] subs=[sub] order=[--needed]\n"
    "cmd sub n=1 miss=[] subs=[] order=[]\n"
    "opt needed n=1 res=[1]\n"
    "opt excluded n=0 res=[]\n"
    "ERR ExcludesError 108: sub excludes --excluded\n"
    "cmd app n=1 miss=[] subs=[sub] order=[--needed|--excluded]\n"
    "cmd sub n=1 miss=[] subs=[] order=[]\n"
    "opt needed n=1 res=[1]\n"
    "opt excluded n=1 res=[2]\n"
    "removed: needs=1 again=0 excludes=1\n"
    "cmd app n=1 miss=[] subs=[sub] order=[--needed|--excluded]\n"
    "cmd sub n=1 miss=[] subs=[] order=[]\n"
    "opt needed n=1 res=[1]\n"
    "opt excluded n=1 res=[2]\n"
    "ERR RequiresError 107: first requires second\n"
    "cmd app n=1 miss=[] subs=[first] order=[]\n"
    "cmd first n=1 miss=[] subs=[] order=[]\n"
    "cmd second n=0 miss=[] subs=[] order=[]\n"
    "cmd third n=0 miss=[] subs=[] order=[]\n"
    "cmd app n=1 miss=[] subs=[first|second] order=[]\n"
    "cmd first n=1 miss=[] subs=[] order=[]\n"
    "cmd second n=1 miss=[] subs=[] order=[]\n"
    "cmd third n=0 miss=[] subs=[] order=[]\n"
    "ERR ExcludesError 108: first excludes third\n"
    "cmd app n=1 miss=[] subs=[first|second|third] order=[]\n"
    "cmd first n=1 miss=[] subs=[] order=[]\n"
    "cmd second n=1 miss=[] subs=[] order=[]\n"
    "cmd third n=1 miss=[] subs=[] order=[]\n"
    "removed: needs=1 excludes=1\n"
    "cmd app n=1 miss=[] subs=[first|second|third] order=[]\n"
    "cmd first n=1 miss=[] subs=[] order=[]\n"
    "cmd second n=1 miss=[] subs=[] order=[]\n"
    "cmd third n=1 miss=[] subs=[] order=[]\n";

// LISTED
//   constant: what the steps of d_tests_arg_cpp_command_listed write, as
// CLI11 writes them.
const char LISTED[] =
    "options: --help,--first,pos,--second,--inside\n"
    "positional: pos\n"
    "beta: beta\n"
    "groups: OPTIONS,Second\n"
    "parsed: root=0 alpha=0,0 beta=0,0 got=0,0 first=0 all=0 used=[]\n"
    "remaining: [] recursive=[] size=0 passthrough=[]\n"
    "cmd app n=1 miss=[left] subs=[alpha|beta] order=[--first]\n"
    "cmd alpha n=1 miss=[extra] subs=[] order=[--inner]\n"
    "cmd beta n=1 miss=[] subs=[] order=[]\n"
    "opt first n=1 res=[1]\n"
    "parsed: root=1 alpha=1,1 beta=1,1 got=1,1 first=1 all=5 "
    "used=[alpha,beta]\n"
    "remaining: [left] recursive=[left,extra] size=2 "
    "passthrough=[extra,left]\n";

// TEXT
//   constant: what the steps of d_tests_arg_cpp_command_text write, as CLI11
// writes them.
const char TEXT[] =
    "help [d\\n\\n\\nprog [OPTIONS] [file] "
    "[SUBCOMMANDS]\\n\\n\\nPOSITIONALS:\\n  file                        a "
    "file\\n\\nOPTIONS:\\n  -h,     --help              Print this help "
    "message and exit\\n  -n,     --number            a number\\n  -f          "
    "                a flag\\n          --version           Display program "
    "version information and exit\\n[Option Group: Grouped]\\n  grouped "
    "ones\\n  \\n\\n  OPTIONS:\\n            --inside            inside the "
    "group\\n\\nSUBCOMMANDS:\\n  sub, s                      a "
    "subcommand\\n\\nthe footer\\n]\n"
    "all [d\\n\\n\\nouter prog [OPTIONS] [file] "
    "[SUBCOMMANDS]\\n\\n\\nPOSITIONALS:\\n  file                        a "
    "file\\n\\nOPTIONS:\\n  -h,     --help              Print this help "
    "message and exit\\n  -n,     --number            a number\\n  -f          "
    "                a flag\\n          --version           Display program "
    "version information and exit\\n[Option Group: Grouped]\\n  grouped "
    "ones\\n  \\n\\n  OPTIONS:\\n            --inside            inside the "
    "group\\n\\nSUBCOMMANDS:\\nsub, s\\na subcommand\\n\\n\\nOPTIONS:\\n       "
    "   --deep              deep down\\n\\n\\nthe footer\\n]\n"
    "sub [a subcommand\\n\\n\\nsub [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,     "
    "--help              Print this help message and exit\\n          --deep   "
    "           deep down\\n]\n"
    "usage=[] footer=[the footer] version=[4.5.6]\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--nope\n"
    "cmd app n=1 miss=[--nope] subs=[] order=[]\n"
    "exit 109\n"
    "err prog: The following argument was not expected: --nope\\nRun with "
    "--help for more information.\\n\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--nope\n"
    "cmd app n=1 miss=[--nope] subs=[] order=[]\n"
    "exit 109\n"
    "err ERROR: ExtrasError: prog: The following argument was not expected: "
    "--nope\\nd\\n\\n\\nprog [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,     --help     "
    "         Print this help message and exit\\n  -n                          "
    "a number\\n\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--nope\n"
    "cmd app n=1 miss=[--nope] subs=[] order=[]\n"
    "exit 109\n"
    "err prog: The following argument was not expected: --nope\\nRun with "
    "--help for more information.\\n\n";

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// TEXT_CONFIG
//   constant: what the steps of d_tests_arg_cpp_command_text_config write,
// as CLI11 writes them.
const char TEXT_CONFIG[] =
    "active [given=3\\nsub.ex=true\\n]\n"
    "all [## d\\n# a given value\\ngiven=3\\n\\n# a defaulted "
    "value\\nkept=5\\n## \\n# a flag\\nsub.ex=true\\n]\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

// TRIGGERS
//   constant: what the steps of d_tests_arg_cpp_command_triggers write, as
// CLI11 writes them where a command does not start allowing Windows-style
// options.
const char TRIGGERS[] =
    "target switches: required=0 disabled=0 silent=0 windows=0 at_end=0 "
    "configurable=0 validate=0 validate_optional=0 non_standard=0 "
    "prefix_matching=0 fallthrough=0 sub_fallthrough=1 case=0 underscore=0 "
    "immediate=0 off_by_default=1 on_by_default=0\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "target\n"
    "cmd app n=1 miss=[target] subs=[] order=[]\n"
    "cmd enabler n=0 miss=[] subs=[] order=[]\n"
    "cmd target n=0 miss=[] subs=[] order=[]\n"
    "cmd app n=1 miss=[] subs=[enabler|target] order=[]\n"
    "cmd enabler n=1 miss=[] subs=[] order=[]\n"
    "cmd target n=1 miss=[] subs=[] order=[]\n"
    "first switches: required=0 disabled=0 silent=0 windows=0 at_end=0 "
    "configurable=0 validate=0 validate_optional=0 non_standard=0 "
    "prefix_matching=0 fallthrough=0 sub_fallthrough=1 case=0 underscore=0 "
    "immediate=0 off_by_default=0 on_by_default=1\n"
    "cmd app n=1 miss=[] subs=[first|second] order=[]\n"
    "cmd stopper n=0 miss=[] subs=[] order=[]\n"
    "cmd first n=1 miss=[] subs=[] order=[]\n"
    "cmd second n=1 miss=[] subs=[] order=[]\n"
    "ERR ExtrasError 109: stopper: The following argument was not expected: "
    "first\n"
    "cmd app n=1 miss=[] subs=[stopper] order=[]\n"
    "cmd stopper n=1 miss=[first] subs=[] order=[]\n"
    "cmd first n=0 miss=[] subs=[] order=[]\n"
    "cmd second n=0 miss=[] subs=[] order=[]\n";

// TRIGGERS_WINDOWS
//   constant: TRIGGERS where a command starts allowing them, as CLI11's does on
// Windows -- and the module's where its setting for them says so -- and a
// subcommand takes the setting from its parent.
const char TRIGGERS_WINDOWS[] =
    "target switches: required=0 disabled=0 silent=0 windows=1 at_end=0 "
    "configurable=0 validate=0 validate_optional=0 non_standard=0 "
    "prefix_matching=0 fallthrough=0 sub_fallthrough=1 case=0 underscore=0 "
    "immediate=0 off_by_default=1 on_by_default=0\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "target\n"
    "cmd app n=1 miss=[target] subs=[] order=[]\n"
    "cmd enabler n=0 miss=[] subs=[] order=[]\n"
    "cmd target n=0 miss=[] subs=[] order=[]\n"
    "cmd app n=1 miss=[] subs=[enabler|target] order=[]\n"
    "cmd enabler n=1 miss=[] subs=[] order=[]\n"
    "cmd target n=1 miss=[] subs=[] order=[]\n"
    "first switches: required=0 disabled=0 silent=0 windows=1 at_end=0 "
    "configurable=0 validate=0 validate_optional=0 non_standard=0 "
    "prefix_matching=0 fallthrough=0 sub_fallthrough=1 case=0 underscore=0 "
    "immediate=0 off_by_default=0 on_by_default=1\n"
    "cmd app n=1 miss=[] subs=[first|second] order=[]\n"
    "cmd stopper n=0 miss=[] subs=[] order=[]\n"
    "cmd first n=1 miss=[] subs=[] order=[]\n"
    "cmd second n=1 miss=[] subs=[] order=[]\n"
    "ERR ExtrasError 109: stopper: The following argument was not expected: "
    "first\n"
    "cmd app n=1 miss=[] subs=[stopper] order=[]\n"
    "cmd stopper n=1 miss=[first] subs=[] order=[]\n"
    "cmd first n=0 miss=[] subs=[] order=[]\n"
    "cmd second n=0 miss=[] subs=[] order=[]\n";

// LIFECYCLE
//   constant: what the steps of d_tests_arg_cpp_command_lifecycle write, as
// CLI11 writes them.
const char LIFECYCLE[] =
    "old: description=[ DEPRECATED: please use '--new' instead] "
    "type=[:DEPRECATED] default=[] group=[OPTIONS] expected=1,1 type_size=1,1 "
    "policy=Throw for_default=0 extra_args=0 required=0\n"
    "cmd app n=1 miss=[] subs=[] order=[--old|--older]\n"
    "opt old n=1 res=[1]\n"
    "opt older n=1 res=[2]\n"
    "out [--old is deprecated please use '--new' instead\\n--older is "
    "deprecated please use '' instead\\n]\n"
    "gone: description=[option has been retired and has no effect] "
    "type=[RETIRED] default=[RETIRED] group=[OPTIONS] expected=0,1 "
    "type_size=1,1 policy=Throw for_default=0 extra_args=0 required=0\n"
    "retired: description=[option has been retired and has no effect] "
    "type=[RETIRED] default=[RETIRED] group=[OPTIONS] expected=1,1 "
    "type_size=1,1 policy=Throw for_default=0 extra_args=0 required=0\n"
    "cmd app n=1 miss=[] subs=[] order=[--gone|--retired]\n"
    "opt gone n=1 res=[x]\n"
    "opt retired n=1 res=[y]\n"
    "out [WARNING --gone is retired and has no effect\\nWARNING --retired is "
    "retired and has no effect\\n]\n"
    "THROWN OptionNotFound: --nowhere not found\n";

// HANDLES
//   constant: what the steps of d_tests_arg_cpp_command_handles write: the
// module's own, as CLI11 has no App* on nothing to read, and its App is not
// a C struct a handle could wrap.
const char HANDLES[] =
    "none switches: required=0 disabled=0 silent=0 windows=0 at_end=0 "
    "configurable=0 validate=0 validate_optional=0 non_standard=0 "
    "prefix_matching=0 fallthrough=0 sub_fallthrough=0 case=0 underscore=0 "
    "immediate=0 off_by_default=0 on_by_default=0\n"
    "none modes: extras=0,Error config_extras=Ignore prefix=0,Off "
    "subcommands=0,0 options=0,0\n"
    "none text: name=[] description=[] group=[SUBCOMMANDS] display=[|] "
    "aliases=[] usage=[] footer=[]\n"
    "none: valid=0 parsed=0 count=0 options=0 subcommands=0 help=[] option=0 "
    "sub=0 moved=0 parent=0 equal=1,1\n"
    "root: same=1 record=1 parser=1 sub: copied=1 required=1 deref=1 arrow=1 "
    "parent=1 group: is_command=1 record=1 found=1\n";

// NONE_FOUND
//   constant: what the steps of d_tests_arg_cpp_command_none_found write:
// the module's own (ISSUES 50), as CLI11 throws OptionNotFound from each of
// these lookups where the face gives a handle on nothing.
const char NONE_FOUND[] =
    "by name=0 by index=0 negative=0 foreign=0 bare=0 pointer=0 group=0 "
    "option=0 indexed=0 count=0 known=1,1\n";

// STATUSES
//   constant: what the steps of d_tests_arg_cpp_command_statuses write: the
// module's own, as CLI11's callbacks return nothing and fail by throwing,
// where the face's may return a cli::status, fail() and fail_runtime()
// record why.
const char STATUSES[] =
    "okay callback\n"
    "cmd app n=1 miss=[] subs=[okay] order=[]\n"
    "cmd refuse n=0 miss=[] subs=[] order=[]\n"
    "cmd runtime n=0 miss=[] subs=[] order=[]\n"
    "cmd okay n=1 miss=[] subs=[] order=[]\n"
    "cmd alone n=0 miss=[] subs=[] order=[]\n"
    "alone callback\n"
    "ERR RequiresError 107: Requires error\n"
    "cmd app n=1 miss=[] subs=[alone] order=[]\n"
    "cmd refuse n=0 miss=[] subs=[] order=[]\n"
    "cmd runtime n=0 miss=[] subs=[] order=[]\n"
    "cmd okay n=0 miss=[] subs=[] order=[]\n"
    "cmd alone n=1 miss=[] subs=[] order=[]\n"
    "exit 107\n"
    "err Requires error\\nRun with --help for more information.\\n\n"
    "ERR ValidationError 105: sub said no\n"
    "cmd app n=1 miss=[] subs=[refuse] order=[]\n"
    "cmd refuse n=1 miss=[] subs=[] order=[]\n"
    "cmd runtime n=0 miss=[] subs=[] order=[]\n"
    "cmd okay n=0 miss=[] subs=[] order=[]\n"
    "cmd alone n=0 miss=[] subs=[] order=[]\n"
    "exit 105\n"
    "err sub said no\\nRun with --help for more information.\\n\n"
    "ERR RuntimeError 3: Runtime error\n"
    "cmd app n=1 miss=[] subs=[runtime] order=[]\n"
    "cmd refuse n=0 miss=[] subs=[] order=[]\n"
    "cmd runtime n=1 miss=[] subs=[] order=[]\n"
    "cmd okay n=0 miss=[] subs=[] order=[]\n"
    "cmd alone n=0 miss=[] subs=[] order=[]\n"
    "exit 3\n"
    "preparse 2\n"
    "ERR ValidationError 105: Validation error\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[] subs=[] order=[]\n"
    "flag callback\n"
    "ERR ExcludesError 108: Excludes error\n"
    "cmd app n=1 miss=[] subs=[] order=[--refusing]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "value 11\n"
    "ERR ValidationError 105: Validation error\n"
    "cmd app n=1 miss=[] subs=[] order=[--value]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "value 11\n"
    "ERR ValidationError 105: Validation error\n"
    "cmd app n=1 miss=[] subs=[] order=[-v|-v|-v|-v|-v|-v|-v|-v|-v|-v|-v]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n";

// PRE_CALLBACK
//   constant: what the steps of d_tests_arg_cpp_command_pre_callback write:
// the module's own (ISSUES 58), as CLI11's pre_callback is a virtual member
// of App that a subclass overrides, where the face's is a setting.
const char PRE_CALLBACK[] =
    "sub pre_callback\n"
    "sub complete\n"
    "sub pre_callback\n"
    "sub final\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[] subs=[] order=[]\n"
    "cmd other n=0 miss=[] subs=[] order=[]\n"
    "other pre_callback\n"
    "ERR ValidationError 105: Validation error\n"
    "cmd app n=1 miss=[] subs=[other] order=[]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "cmd other n=1 miss=[] subs=[] order=[]\n";

// FAILURE_FUNCTION
//   constant: what the steps of d_tests_arg_cpp_command_failure_function
// write: the module's own, as CLI11's failure message function takes
// `const App*` and `const Error&`, where the face's takes the face's
// command and error.
const char FAILURE_FUNCTION[] =
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--nope\n"
    "cmd app n=1 miss=[--nope] subs=[] order=[]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "exit 109\n"
    "err FAILED in prog (ExtrasError, 109): prog: The following argument was "
    "not expected: --nope\\n\n"
    "ERR ExtrasError 109: sub: The following argument was not expected: "
    "--deeper\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[--deeper] subs=[] order=[]\n"
    "exit 109\n"
    "err FAILED in prog (ExtrasError, 109): sub: The following argument was "
    "not expected: --deeper\\n\n";

// BORROWED
//   constant: what the steps of d_tests_arg_cpp_command_borrowed write: the
// module's own (ISSUES 51), as CLI11's formatters and configuration formats
// are shared, its get_subcommand_ptr a std::shared_ptr, where the face's
// borrow the caller's and give the handle get_subcommand gives.
const char BORROWED[] =
    "root: formatter=1 format=1 sub: formatter=1 format=1 own=1 root=1 "
    "pointer=1,1,1\n"
    "help [d\\n\\n\\nprog [OPTIONS] [SUBCOMMANDS]\\n\\n\\nOPTIONS:\\n  -h, "
    "--help \\n            Print this help message and exit\\n  -n        a "
    "number described at length\\n\\nSUBCOMMANDS:\\n  sub       \\n]\n";

//==============================================================================
// 1.  CASES
//==============================================================================
// What the sections held to CLI11 define, in the part of the face CLI11
// shares; tools/cppref/cppref.py compiles this section against CLI11.

/**
 * @brief The name of a multi-option policy, as CLI11 spells it.
 *
 * @param[in] _policy  the policy.
 * @return the name; "?" for a value that is none.
 */
const char*
policy_name(
    cli::multi_option_policy::value _policy
)
{
    // each policy CLI11 has
    switch (_policy)
    {
        case cli::multi_option_policy::throw_error:
            return "Throw";
        case cli::multi_option_policy::take_last:
            return "TakeLast";
        case cli::multi_option_policy::take_first:
            return "TakeFirst";
        case cli::multi_option_policy::join:
            return "Join";
        case cli::multi_option_policy::take_all:
            return "TakeAll";
        case cli::multi_option_policy::sum:
            return "Sum";
        case cli::multi_option_policy::reverse:
            return "Reverse";
        default:
            return "?";
    }
}

/**
 * @brief The name of a callback priority, as CLI11 spells it.
 *
 * @param[in] _priority  the priority.
 * @return the name; "?" for a value that is none.
 */
const char*
priority_name(
    cli::callback_priority::value _priority
)
{
    // each priority CLI11 has
    switch (_priority)
    {
        case cli::callback_priority::first_pre_help:
            return "FirstPreHelp";
        case cli::callback_priority::first:
            return "First";
        case cli::callback_priority::pre_requirements_pre_help:
            return "PreRequirementsCheckPreHelp";
        case cli::callback_priority::pre_requirements:
            return "PreRequirementsCheck";
        case cli::callback_priority::normal_pre_help:
            return "NormalPreHelp";
        case cli::callback_priority::normal:
            return "Normal";
        case cli::callback_priority::last_pre_help:
            return "LastPreHelp";
        case cli::callback_priority::last:
            return "Last";
        default:
            return "?";
    }
}

/**
 * @brief The name of an extras mode, as CLI11 spells it.
 *
 * @param[in] _mode  the mode.
 * @return the name; "?" for a value that is none.
 */
const char*
extras_name(
    cli::extras_mode::value _mode
)
{
    // each mode CLI11 has
    switch (_mode)
    {
        case cli::extras_mode::error:
            return "Error";
        case cli::extras_mode::error_immediately:
            return "ErrorImmediately";
        case cli::extras_mode::ignore:
            return "Ignore";
        case cli::extras_mode::assume_single:
            return "AssumeSingleArgument";
        case cli::extras_mode::assume_multiple:
            return "AssumeMultipleArguments";
        case cli::extras_mode::capture:
            return "Capture";
        default:
            return "?";
    }
}

/**
 * @brief The name of a configuration extras mode, as CLI11 spells it.
 *
 * @param[in] _mode  the mode.
 * @return the name; "?" for a value that is none.
 */
const char*
config_extras_name(
    cli::config_extras_mode::value _mode
)
{
    // each mode CLI11 has
    switch (_mode)
    {
        case cli::config_extras_mode::error:
            return "Error";
        case cli::config_extras_mode::ignore:
            return "Ignore";
        case cli::config_extras_mode::ignore_all:
            return "IgnoreAll";
        case cli::config_extras_mode::capture:
            return "Capture";
        default:
            return "?";
    }
}

/**
 * @brief The name of a prefix command mode, as CLI11 spells it.
 *
 * @param[in] _mode  the mode.
 * @return the name; "?" for a value that is none.
 */
const char*
prefix_name(
    cli::prefix_mode::value _mode
)
{
    // each mode CLI11 has
    switch (_mode)
    {
        case cli::prefix_mode::off:
            return "Off";
        case cli::prefix_mode::separator_only:
            return "SeparatorOnly";
        case cli::prefix_mode::on:
            return "On";
        case cli::prefix_mode::positional_only:
            return "PositionalOnly";
        default:
            return "?";
    }
}

/**
 * @brief The name of how a name matched, as CLI11 spells it.
 *
 * @param[in] _match  the match.
 * @return the name; "?" for a value that is none.
 */
const char*
match_name(
    cli::name_match::value _match
)
{
    // each match CLI11 has
    switch (_match)
    {
        case cli::name_match::none:
            return "none";
        case cli::name_match::exact:
            return "exact";
        case cli::name_match::prefix:
            return "prefix";
        default:
            return "?";
    }
}

/**
 * @brief A truth value as a transcript writes it.
 *
 * @param[in] _value  the value.
 * @return "1" or "0".
 */
const char*
truth(
    bool _value
)
{
    return (_value) ? "1" : "0";
}

/**
 * @brief A list of texts as one, escaped, each after a separator but the
 *        first.
 *
 * @param[in] _items      the texts.
 * @param[in] _separator  what goes between two.
 * @return the text.
 */
std::string
joined(
    const std::vector<std::string>& _items,
    const char*                     _separator
)
{
    std::string out;

    // each item, after a separator when one comes before it
    for (std::size_t index = 0u; index < _items.size(); ++index)
    {
        out += (index > 0u) ? _separator : "";
        out += escape(_items[index]);
    }

    return out;
}

/**
 * @brief A list of numbers as one text, separated by commas.
 *
 * @param[in] _values  the numbers.
 * @return the text.
 */
std::string
numbers_of(
    const std::vector<int>& _values
)
{
    std::string out;

    // each number, after a comma when one comes before it
    for (std::size_t index = 0u; index < _values.size(); ++index)
    {
        out += (index > 0u) ? "," : "";
        out += number(static_cast<long>(_values[index]));
    }

    return out;
}

/**
 * @brief The names of what a list holds -- options or commands -- in its
 *        order: CLI11's lists are std::vectors of pointers, the face's of
 *        handles, and both are read by index with `->`.
 *
 * @param[in] _list  the list.
 * @return the names.
 */
template<typename List>
std::vector<std::string>
names_of(
    const List& _list
)
{
    std::vector<std::string> names;

    // each element of the list
    for (std::size_t index = 0u; index < _list.size(); ++index)
    {
        names.push_back(std::string(_list[index]->get_name()));
    }

    return names;
}

/**
 * @brief An option's name, or "none" for a handle on nothing: CLI11's
 *        pointer compared with a null one, the face's handle with one made
 *        bare.
 *
 * @param[in] _option  the option.
 * @return the name.
 */
template<typename Handle>
std::string
option_name(
    const Handle& _option
)
{
    return (_option == cli::option())
           ? std::string("none")
           : std::string(_option->get_name());
}

/**
 * @brief A command's name, or "none" for a handle on nothing.
 *
 * @param[in] _command  the command.
 * @return the name.
 */
template<typename Handle>
std::string
command_name(
    const Handle& _command
)
{
    return (_command == cli::command())
           ? std::string("none")
           : std::string(_command->get_name());
}

/**
 * @brief Writes an option as a command made it -- its text, its counts,
 *        its policy -- on one line.
 *
 * @param[in,out] _run     the run.
 * @param[in]     _label   what the line names the option by.
 * @param[in]     _option  the option.
 */
void
write_option(
    run&               _run,
    const char*        _label,
    const cli::option& _option
)
{
    std::ostringstream line;

    line << _label
         << ": description=[" << _option->get_description()
         << "] type=[" << _option->get_type_name()
         << "] default=[" << _option->get_default_str()
         << "] group=[" << _option->get_group()
         << "] expected=" << _option->get_expected_min()
         << "," << _option->get_expected_max()
         << " type_size=" << _option->get_type_size_min()
         << "," << _option->get_type_size_max()
         << " policy=" << policy_name(_option->get_multi_option_policy())
         << " for_default=" << _option->get_run_callback_for_default()
         << " extra_args=" << _option->get_allow_extra_args()
         << " required=" << _option->get_required();
    _run.note(line.str());

    return;
}

/**
 * @brief Writes every switch of a command, as its readers read them, on one
 *        line.
 *
 * @param[in,out] _run      the run.
 * @param[in]     _label    what the line names the command by.
 * @param[in]     _command  the command.
 */
void
write_switches(
    run&                _run,
    const char*         _label,
    const cli::command& _command
)
{
    std::ostringstream line;

    line << _label
         << " switches: required=" << _command->get_required()
         << " disabled=" << _command->get_disabled()
         << " silent=" << _command->get_silent()
         << " windows=" << _command->get_allow_windows_style_options()
         << " at_end=" << _command->get_positionals_at_end()
         << " configurable=" << _command->get_configurable()
         << " validate=" << _command->get_validate_positionals()
         << " validate_optional="
         << _command->get_validate_optional_arguments()
         << " non_standard="
         << _command->get_allow_non_standard_option_names()
         << " prefix_matching="
         << _command->get_allow_subcommand_prefix_matching()
         << " fallthrough=" << _command->get_fallthrough()
         << " sub_fallthrough=" << _command->get_subcommand_fallthrough()
         << " case=" << _command->get_ignore_case()
         << " underscore=" << _command->get_ignore_underscore()
         << " immediate=" << _command->get_immediate_callback()
         << " off_by_default=" << _command->get_disabled_by_default()
         << " on_by_default=" << _command->get_enabled_by_default();
    _run.note(line.str());

    return;
}

/**
 * @brief Writes a command's modes and requirements, as its readers read
 *        them, on one line.
 *
 * @param[in,out] _run      the run.
 * @param[in]     _label    what the line names the command by.
 * @param[in]     _command  the command.
 */
void
write_modes(
    run&                _run,
    const char*         _label,
    const cli::command& _command
)
{
    std::ostringstream line;

    line << _label
         << " modes: extras=" << _command->get_allow_extras()
         << "," << extras_name(_command->get_allow_extras_mode())
         << " config_extras="
         << config_extras_name(_command->get_allow_config_extras())
         << " prefix=" << _command->get_prefix_command()
         << "," << prefix_name(_command->get_prefix_command_mode())
         << " subcommands=" << _command->get_require_subcommand_min()
         << "," << _command->get_require_subcommand_max()
         << " options=" << _command->get_require_option_min()
         << "," << _command->get_require_option_max();
    _run.note(line.str());

    return;
}

/**
 * @brief Writes a command's text -- its name, description, group, display
 *        name, aliases, usage and footer -- on one line.
 *
 * @param[in,out] _run      the run.
 * @param[in]     _label    what the line names the command by.
 * @param[in]     _command  the command.
 */
void
write_text(
    run&                _run,
    const char*         _label,
    const cli::command& _command
)
{
    const std::vector<std::string> aliases = _command->get_aliases();
    std::ostringstream             line;

    line << _label
         << " text: name=[" << _command->get_name()
         << "] description=[" << _command->get_description()
         << "] group=[" << _command->get_group()
         << "] display=[" << _command->get_display_name()
         << "|" << _command->get_display_name(true)
         << "] aliases=[" << joined(aliases,
                                    ",")
         << "] usage=[" << escape(_command->get_usage())
         << "] footer=[" << escape(_command->get_footer())
         << "]";
    _run.note(line.str());

    return;
}

/**
 * @brief Writes the option defaults a command gives its new options, as
 *        their readers read them, on one line.
 *
 * @param[in,out] _run       the run.
 * @param[in]     _label     what the line names the defaults by.
 * @param[in]     _defaults  the defaults.
 */
void
write_defaults(
    run&                        _run,
    const char*                 _label,
    const cli::option_defaults& _defaults
)
{
    std::ostringstream line;

    line << _label
         << " defaults: group=[" << _defaults->get_group()
         << "] required=" << _defaults->get_required()
         << " capture=" << _defaults->get_always_capture_default()
         << " configurable=" << _defaults->get_configurable()
         << " no_override=" << _defaults->get_disable_flag_override()
         << " case=" << _defaults->get_ignore_case()
         << " underscore=" << _defaults->get_ignore_underscore()
         << " delimiter=" << static_cast<int>(_defaults->get_delimiter())
         << " policy="
         << policy_name(_defaults->get_multi_option_policy())
         << " priority="
         << priority_name(_defaults->get_callback_priority());
    _run.note(line.str());

    return;
}

// noted_call
//   struct: a callback taking nothing -- a command's, or a flag's -- which
// notes its text.
struct noted_call
{
    run*        m_run;
    const char* m_text;

    /**
     * @brief Notes the text.
     */
    void
    operator()() const
    {
        m_run->note(m_text);

        return;
    }
};

// preparse_noted
//   struct: a pre-parse callback, which notes its text and the arguments
// left.
struct preparse_noted
{
    run*        m_run;
    const char* m_text;

    /**
     * @brief Notes the text and the count.
     *
     * @param[in] _left  the arguments left.
     */
    void
    operator()(
        std::size_t _left
    ) const
    {
        m_run->note(std::string(m_text) +
                    " " +
                    number(static_cast<unsigned long>(_left)));

        return;
    }
};

// int_noted
//   struct: add_option_function's function over an int, which notes it.
struct int_noted
{
    run* m_run;

    /**
     * @brief Notes the value.
     *
     * @param[in] _value  the value.
     */
    void
    operator()(
        const int& _value
    ) const
    {
        m_run->note("function int " +
                    number(static_cast<long>(_value)));

        return;
    }
};

// text_noted
//   struct: add_option_function's function over a std::string, which notes
// it.
struct text_noted
{
    run* m_run;

    /**
     * @brief Notes the value.
     *
     * @param[in] _value  the value.
     */
    void
    operator()(
        const std::string& _value
    ) const
    {
        m_run->note("function text " + escape(_value));

        return;
    }
};

// count_noted
//   struct: add_flag_function's function, which notes the count it is
// given, whatever integer type that is.
struct count_noted
{
    run* m_run;

    /**
     * @brief Notes the count.
     *
     * @param[in] _count  the count.
     */
    template<typename Count>
    void
    operator()(
        Count _count
    ) const
    {
        m_run->note("flag function " +
                    number(static_cast<long>(_count)));

        return;
    }
};

// text_made
//   struct: a function giving text -- a version, a usage, a footer.
struct text_made
{
    const char* m_text;

    /**
     * @brief The text.
     *
     * @return the text, as a std::string.
     */
    std::string
    operator()() const
    {
        return std::string(m_text);
    }
};

// positional_only
//   struct: a filter of get_options(), keeping positionals.
struct positional_only
{
    /**
     * @brief Whether the option is positional.
     *
     * @param[in] _option  the option: a pointer for CLI11, a handle here.
     * @return whether it is.
     */
    template<typename Option>
    bool
    operator()(
        const Option& _option
    ) const
    {
        return _option->get_positional();
    }
};

// named_only
//   struct: a filter of get_subcommands(), keeping a command of one name.
struct named_only
{
    const char* m_name;

    /**
     * @brief Whether the command has the name.
     *
     * @param[in] _command  the command: a pointer for CLI11, a handle here.
     * @return whether it has.
     */
    template<typename Command>
    bool
    operator()(
        const Command& _command
    ) const
    {
        return (std::string(_command->get_name()) == m_name);
    }
};

// 1.1    Options
//------------------------------------------------------------------------------

/**
 * @brief ADD 1: options in every form add_option takes, on the parser: a
 *        name alone, with a description, bound to variables, a const second
 *        argument read as a description, and a variable whose default is
 *        never written.
 *
 * @param[in,out] _run  the step's run.
 */
void
add_root(
    run& _run
)
{
    static const char* const GIVEN[] =
    {
        "--plain", "p", "--described", "d", "-n", "7", "--word", "w",
        "--list", "1", "2", "--constant", "c", "--quiet", "q"
    };
    int                      whole = 0;
    std::string              word;
    std::vector<int>         list;
    std::string              quiet("kept");
    const std::string        constant("a const description");

    write_option(_run,
                 "plain",
                 _run.watch("plain",
                            _run.app().add_option("--plain")));
    write_option(_run,
                 "described",
                 _run.watch("described",
                            _run.app().add_option("--described",
                                                  "a description")));
    write_option(_run,
                 "whole",
                 _run.watch("whole",
                            _run.app().add_option("-n,--number",
                                                  whole)));
    write_option(_run,
                 "word",
                 _run.watch("word",
                            _run.app().add_option("--word",
                                                  word,
                                                  "a word")));
    write_option(_run,
                 "list",
                 _run.watch("list",
                            _run.app().add_option("--list",
                                                  list)));
    write_option(_run,
                 "constant",
                 _run.watch("constant",
                            _run.app().add_option("--constant",
                                                  constant)));
    write_option(_run,
                 "quiet",
                 _run.watch("quiet",
                            _run.app().add_option_no_stream("--quiet",
                                                            quiet)));
    _run.parse(GIVEN,
               15u);
    _run.note("variables: whole=" + number(static_cast<long>(whole)) +
              " word=" + word +
              " list=" + numbers_of(list) +
              " quiet=" + quiet);

    return;
}

/**
 * @brief ADD 2: options added through a subcommand's handle and an option
 *        group's.
 *
 * @param[in,out] _run  the step's run.
 */
void
add_handles(
    run& _run
)
{
    static const char* const GIVEN[] =
    {
        "--grouped", "g", "sub", "--inner", "4", "--outer", "o"
    };
    int                      inner   = 0;
    std::string              grouped;
    cli::command             sub     = _run.make_subcommand(_run.root(),
                                                            "sub");
    cli::option_group        group   = _run.app().add_option_group(
                                           "Grouped",
                                           "a group");

    (void)_run.watch("Grouped",
                     group);
    write_option(_run,
                 "inner",
                 _run.watch("inner",
                            sub->add_option("--inner",
                                            inner,
                                            "an inner value")));
    write_option(_run,
                 "outer",
                 _run.watch("outer",
                            sub->add_option("--outer",
                                            "an outer value")));
    write_option(_run,
                 "grouped",
                 _run.watch("grouped",
                            group->add_option("--grouped",
                                              grouped)));
    _run.parse(GIVEN,
               7u);
    _run.note("variables: inner=" + number(static_cast<long>(inner)) +
              " grouped=" + grouped);

    return;
}

/**
 * @brief ADD 3: options whose values go to a function, over an int and
 *        over a std::string, and a value that does not convert.
 *
 * @param[in,out] _run  the step's run.
 */
void
add_functions(
    run& _run
)
{
    static const char* const GIVEN[]  = { "--whole", "42", "--text", "abc" };
    static const char* const BAD[]    = { "--whole", "x" };
    const int_noted          wholes   = { &_run };
    const text_noted         texts    = { &_run };

    write_option(_run,
                 "whole",
                 _run.watch("whole",
                            _run.app().add_option_function<int>(
                                "--whole",
                                wholes,
                                "a whole number")));
    write_option(_run,
                 "text",
                 _run.watch("text",
                            _run.app().add_option_function<std::string>(
                                "--text",
                                texts)));
    _run.parse(GIVEN,
               4u);
    _run.parse(BAD,
               2u);

    return;
}

/**
 * @brief ADD 4: a name another option has, refused.
 *
 * @param[in,out] _run  the step's run.
 */
void
add_duplicate(
    run& _run
)
{
    _run.app().add_option("-a,--alpha");
    _run.app().add_option("--beta,-a");

    return;
}

/**
 * @brief ADD 5: a name no option can have, refused.
 *
 * @param[in,out] _run  the step's run.
 */
void
add_bad_name(
    run& _run
)
{
    _run.app().add_option("--bad name");

    return;
}

// ADD_STEPS
//   constant: the steps of d_tests_arg_cpp_command_add.
const step_fn ADD_STEPS[] =
{
    &add_root,
    &add_handles,
    &add_functions,
    &add_duplicate,
    &add_bad_name
};

// ADD_COUNT
//   constant: how many steps ADD_STEPS holds.
const std::size_t ADD_COUNT = sizeof(ADD_STEPS) / sizeof(ADD_STEPS[0]);

/**
 * @brief FLAGS 1: flags in every form add_flag takes: a name alone, with a
 *        description, bound to a bool, to an int counting its uses, and to
 *        a vector taking a value at each use.
 *
 * @param[in,out] _run  the step's run.
 */
void
flags_forms(
    run& _run
)
{
    static const char* const GIVEN[] =
    {
        "-f", "-t", "-c", "-cc", "--level", "--level=5", "--described"
    };
    bool                     truly   = false;
    int                      counted = 0;
    std::vector<int>         levels;

    write_option(_run,
                 "flag",
                 _run.watch("flag",
                            _run.app().add_flag("-f,--flag")));
    write_option(_run,
                 "described",
                 _run.watch("described",
                            _run.app().add_flag("--described",
                                                "a flag described")));
    write_option(_run,
                 "truly",
                 _run.watch("truly",
                            _run.app().add_flag("-t,--truly",
                                                truly)));
    write_option(_run,
                 "counted",
                 _run.watch("counted",
                            _run.app().add_flag("-c",
                                                counted)));
    write_option(_run,
                 "levels",
                 _run.watch("levels",
                            _run.app().add_flag("--level{2}",
                                                levels)));
    _run.parse(GIVEN,
               7u);
    _run.note(std::string("variables: truly=") + truth(truly) +
              " counted=" + number(static_cast<long>(counted)) +
              " levels=" + numbers_of(levels));

    return;
}

/**
 * @brief FLAGS 2: a flag's negation, the last use winning, a value given
 *        with `=`, and the same refused once overriding is disabled.
 *
 * @param[in,out] _run  the step's run.
 */
void
flags_values(
    run& _run
)
{
    static const char* const NEGATED[] = { "--no-color" };
    static const char* const LAST[]    = { "--no-color", "--color" };
    static const char* const GIVEN[]   = { "--color=false" };
    bool                     color     = true;
    cli::option              flag      = _run.watch(
                                             "color",
                                             _run.app().add_flag(
                                                 "--color,!--no-color",
                                                 color));

    _run.parse(NEGATED,
               1u);
    _run.note(std::string("color=") + truth(color));
    _run.parse(LAST,
               2u);
    _run.note(std::string("color=") + truth(color));
    _run.parse(GIVEN,
               1u);
    _run.note(std::string("color=") + truth(color));
    flag->disable_flag_override();
    _run.parse(GIVEN,
               1u);
    _run.note(std::string("color=") + truth(color));

    return;
}

/**
 * @brief FLAGS 3: a flag calling a function when it is true, and one giving
 *        a function the sum of its uses.
 *
 * @param[in,out] _run  the step's run.
 */
void
flags_callbacks(
    run& _run
)
{
    static const char* const GIVEN[] =
    {
        "--cb", "-vvv", "--verbose", "--cb"
    };
    const noted_call         called  = { &_run, "flag callback" };
    const count_noted        counted = { &_run };

    write_option(_run,
                 "cb",
                 _run.watch("cb",
                            _run.app().add_flag_callback("--cb",
                                                         called,
                                                         "calls back")));
    write_option(_run,
                 "verbose",
                 _run.watch("verbose",
                            _run.app().add_flag_function("-v,--verbose",
                                                         counted)));
    _run.parse(GIVEN,
               4u);

    return;
}

/**
 * @brief FLAGS 4: a flag named as a positional, refused.
 *
 * @param[in,out] _run  the step's run.
 */
void
flags_positional(
    run& _run
)
{
    _run.app().add_flag("positional");

    return;
}

// FLAGS_STEPS
//   constant: the steps of d_tests_arg_cpp_command_flags.
const step_fn FLAGS_STEPS[] =
{
    &flags_forms,
    &flags_values,
    &flags_callbacks,
    &flags_positional
};

// FLAGS_COUNT
//   constant: how many steps FLAGS_STEPS holds.
const std::size_t FLAGS_COUNT = sizeof(FLAGS_STEPS) / sizeof(FLAGS_STEPS[0]);

/**
 * @brief Writes the special options a command has, by name, on one line.
 *
 * @param[in,out] _run      the run.
 * @param[in]     _label    what the line is.
 */
void
write_special(
    run&        _run,
    const char* _label
)
{
    _run.note(std::string(_label) +
              ": help=" + option_name(_run.app().get_help_ptr()) +
              " help_all=" + option_name(_run.app().get_help_all_ptr()) +
              " version=" + option_name(_run.app().get_version_ptr()) +
              " config=" + option_name(_run.app().get_config_ptr()));

    return;
}

/**
 * @brief SPECIAL 1: the special options a parser starts with, and the help
 *        flag's text.
 *
 * @param[in,out] _run  the step's run.
 */
void
special_defaults(
    run& _run
)
{
    write_special(_run,
                  "made");
    write_option(_run,
                 "help",
                 _run.app().get_help_ptr());
    _run.note("version=[" + _run.app().version() + "]");

    return;
}

/**
 * @brief SPECIAL 2: the help, help-all and version flags replaced, and a
 *        parse asking for each.
 *
 * @param[in,out] _run  the step's run.
 */
void
special_set(
    run& _run
)
{
    static const char* const VERSION[]  = { "-V" };
    static const char* const HELP_ALL[] = { "--help-all" };
    static const char* const USAGE[]    = { "-?" };

    (void)_run.app().add_option("-n",
                                "a number");
    _run.app().set_help_flag("-?,--usage",
                             "show the usage");
    _run.app().set_help_all_flag("--help-all",
                                 "show everything");
    _run.app().set_version_flag("-V,--version",
                                "1.2.3",
                                "show the version");
    write_special(_run,
                  "set");
    write_option(_run,
                 "version",
                 _run.app().get_version_ptr());
    _run.note("version=[" + _run.app().version() + "]");
    _run.parse(VERSION,
               1u);
    _run.exit();
    _run.parse(HELP_ALL,
               1u);
    _run.exit();
    _run.parse(USAGE,
               1u);
    _run.exit();

    return;
}

/**
 * @brief SPECIAL 3: a version flag whose text a function gives.
 *
 * @param[in,out] _run  the step's run.
 */
void
special_version_function(
    run& _run
)
{
    static const char* const ASKED[] = { "--version" };
    const text_made          made    = { "made 9.9" };

    _run.app().set_version_flag("--version",
                                made);
    _run.note("version=[" + _run.app().version() + "]");
    _run.parse(ASKED,
               1u);
    _run.exit();

    return;
}

/**
 * @brief SPECIAL 4: the help and version flags removed, and a parse asking
 *        for help after.
 *
 * @param[in,out] _run  the step's run.
 */
void
special_removed(
    run& _run
)
{
    static const char* const ASKED[] = { "--help" };

    _run.app().set_version_flag("--version",
                                "1.0");
    _run.app().set_help_flag();
    _run.app().set_version_flag();
    write_special(_run,
                  "removed");
    _run.parse(ASKED,
               1u);

    return;
}

// SPECIAL_STEPS
//   constant: the steps of d_tests_arg_cpp_command_special.
const step_fn SPECIAL_STEPS[] =
{
    &special_defaults,
    &special_set,
    &special_version_function,
    &special_removed
};

// SPECIAL_COUNT
//   constant: how many steps SPECIAL_STEPS holds.
const std::size_t SPECIAL_COUNT =
    sizeof(SPECIAL_STEPS) / sizeof(SPECIAL_STEPS[0]);

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief SPECIAL_CONFIG 1: the configuration option set, with a default
 *        file that is not there, and a file named that is not either.
 *
 * @param[in,out] _run  the step's run.
 */
void
special_config(
    run& _run
)
{
    static const char* const NAMED[] =
    {
        "--config", "no_such_file_anywhere.ini"
    };

    _run.app().set_config("--config",
                          "no_such_settings_file.ini",
                          "the configuration",
                          false);
    write_special(_run,
                  "config");
    write_option(_run,
                 "config",
                 _run.app().get_config_ptr());
    _run.parse(D_NULLPTR,
               0u);
    _run.parse(NAMED,
               2u);

    return;
}

// SPECIAL_CONFIG_STEPS
//   constant: the steps of d_tests_arg_cpp_command_special_config.
const step_fn SPECIAL_CONFIG_STEPS[] =
{
    &special_config
};

// SPECIAL_CONFIG_COUNT
//   constant: how many steps SPECIAL_CONFIG_STEPS holds.
const std::size_t SPECIAL_CONFIG_COUNT =
    sizeof(SPECIAL_CONFIG_STEPS) / sizeof(SPECIAL_CONFIG_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

// 1.2    Subcommands and groups
//------------------------------------------------------------------------------

/**
 * @brief SUBCOMMANDS 1: subcommands made, found by name, by index, by
 *        handle and by alias, and their parents.
 *
 * @param[in,out] _run  the step's run.
 */
void
subcommands_found(
    run& _run
)
{
    cli::command alpha = _run.make_subcommand(_run.root(),
                                              "alpha");
    cli::command beta  = _run.watch("beta",
                                    _run.root()->add_subcommand(
                                        "beta",
                                        "the second"));

    beta->alias("b");
    _run.note("by name=" +
              command_name(_run.app().get_subcommand("alpha")) +
              " by index=" +
              command_name(_run.app().get_subcommand(1)) +
              " by handle=" +
              command_name(_run.app().get_subcommand(beta)) +
              " by alias=" +
              command_name(_run.app().get_subcommand_no_throw("b")) +
              " none=" +
              command_name(_run.app().get_subcommand_no_throw("nope")));
    _run.note("parents: alpha=" + command_name(alpha->get_parent()) +
              " root=" + command_name(_run.root()->get_parent()));
    write_text(_run,
               "alpha",
               alpha);
    write_text(_run,
               "beta",
               beta);

    return;
}

/**
 * @brief Writes how each name matches a command's: check_name and
 *        check_name_detail.
 *
 * @param[in,out] _run      the run.
 * @param[in]     _command  the command.
 * @param[in]     _names    the names.
 * @param[in]     _count    how many.
 */
void
write_matches(
    run&                _run,
    const cli::command& _command,
    const char* const*  _names,
    std::size_t         _count
)
{
    std::string line = "matches:";

    // each name
    for (std::size_t index = 0u; index < _count; ++index)
    {
        line += " ";
        line += _names[index];
        line += "=";
        line += truth(_command->check_name(_names[index]));
        line += ",";
        line += match_name(_command->check_name_detail(_names[index]));
    }

    _run.note(line);

    return;
}

/**
 * @brief SUBCOMMANDS 2: aliases, cleared; the names a subcommand answers
 *        to, as its case, underscores and prefixes are compared, and a
 *        prefix parsed.
 *
 * @param[in,out] _run  the step's run.
 */
void
subcommands_names(
    run& _run
)
{
    static const char* const TRIED[] =
    {
        "alpha", "al", "first", "alp", "ALPHA", "al_pha", "fir"
    };
    static const char* const PREFIX[] = { "alp" };
    cli::command             alpha    = _run.make_subcommand(_run.root(),
                                                             "alpha");

    alpha->alias("al")
         ->alias("first");
    write_text(_run,
               "aliased",
               alpha);
    write_matches(_run,
                  alpha,
                  TRIED,
                  7u);
    alpha->ignore_case()
         ->ignore_underscore()
         ->allow_subcommand_prefix_matching();
    write_matches(_run,
                  alpha,
                  TRIED,
                  7u);
    alpha->clear_aliases();
    write_text(_run,
               "cleared",
               alpha);
    _run.parse(PREFIX,
               1u);

    return;
}

/**
 * @brief SUBCOMMANDS 3: subcommands required: at least one, exactly one, at
 *        most one, and a range, each parsed.
 *
 * @param[in,out] _run  the step's run.
 */
void
subcommands_required(
    run& _run
)
{
    static const char* const ALPHA[] = { "alpha" };
    static const char* const BOTH[]  = { "alpha", "beta" };

    (void)_run.make_subcommand(_run.root(),
                               "alpha");
    (void)_run.make_subcommand(_run.root(),
                               "beta");
    _run.app().require_subcommand();
    write_modes(_run,
                "one or more",
                _run.root());
    _run.parse(D_NULLPTR,
               0u);
    _run.parse(BOTH,
               2u);
    _run.app().require_subcommand(1);
    write_modes(_run,
                "exactly one",
                _run.root());
    _run.parse(BOTH,
               2u);
    _run.app().require_subcommand(-1);
    write_modes(_run,
                "at most one",
                _run.root());
    _run.parse(D_NULLPTR,
               0u);
    _run.app().require_subcommand(2u,
                                  2u);
    write_modes(_run,
                "two",
                _run.root());
    _run.parse(ALPHA,
               1u);

    return;
}

/**
 * @brief SUBCOMMANDS 4: what a subcommand reaches: its parent's options
 *        with fallthrough and without, and a subcommand disabled.
 *
 * @param[in,out] _run  the step's run.
 */
void
subcommands_reach(
    run& _run
)
{
    static const char* const THROUGH[]  = { "alpha", "--top", "1" };
    static const char* const DISABLED[] = { "beta" };
    cli::command             alpha      = _run.make_subcommand(_run.root(),
                                                               "alpha");
    cli::command             beta       = _run.make_subcommand(_run.root(),
                                                               "beta");

    (void)_run.make_option("top",
                           _run.root(),
                           "--top");
    _run.parse(THROUGH,
               3u);
    alpha->fallthrough();
    _run.parse(THROUGH,
               3u);
    beta->disabled();
    _run.parse(DISABLED,
               1u);

    return;
}

// SUBCOMMANDS_STEPS
//   constant: the steps of d_tests_arg_cpp_command_subcommands.
const step_fn SUBCOMMANDS_STEPS[] =
{
    &subcommands_found,
    &subcommands_names,
    &subcommands_required,
    &subcommands_reach
};

// SUBCOMMANDS_COUNT
//   constant: how many steps SUBCOMMANDS_STEPS holds.
const std::size_t SUBCOMMANDS_COUNT =
    sizeof(SUBCOMMANDS_STEPS) / sizeof(SUBCOMMANDS_STEPS[0]);

/**
 * @brief GROUPS 1: an option group made and found, an option and a
 *        subcommand moved into it, an option made in it, and a parse of
 *        each.
 *
 * @param[in,out] _run  the step's run.
 */
void
groups_moves(
    run& _run
)
{
    static const char* const GIVEN[] =
    {
        "--moved", "1", "--inside", "2", "gamma"
    };
    cli::option              moved   = _run.make_option("moved",
                                                        _run.root(),
                                                        "--moved");
    cli::command             gamma   = _run.make_subcommand(_run.root(),
                                                            "gamma");
    cli::option_group        group   = _run.app().add_option_group(
                                           "Grouped",
                                           "the group");

    (void)_run.watch("Grouped",
                     group);
    (void)_run.make_option("inside",
                           group,
                           "--inside");

    const bool option_back  = (group->add_option(moved) == moved);
    const bool command_back = (group->add_subcommand(gamma) == gamma);

    _run.note(std::string("moved: option=") + truth(option_back) +
              " command=" + truth(command_back) +
              " found=" +
              truth(_run.app().get_option_group("Grouped") == group));
    write_text(_run,
               "group",
               group);
    _run.note("in the group: " +
              joined(names_of(group->get_options()),
                     ","));
    _run.parse(GIVEN,
               5u);

    return;
}

/**
 * @brief GROUPS 2: an option group whose options are required: one of
 *        them, at most one, and the group itself.
 *
 * @param[in,out] _run  the step's run.
 */
void
groups_required(
    run& _run
)
{
    static const char* const FIRST[] = { "--first", "1" };
    static const char* const BOTH[]  = { "--first", "1", "--second", "2" };
    cli::option_group        group   = _run.app().add_option_group(
                                           "Choice");

    (void)_run.watch("Choice",
                     group);
    (void)_run.make_option("first",
                           group,
                           "--first");
    (void)_run.make_option("second",
                           group,
                           "--second");
    group->require_option(1);
    write_modes(_run,
                "choice",
                group);
    _run.parse(D_NULLPTR,
               0u);
    _run.parse(FIRST,
               2u);
    _run.parse(BOTH,
               4u);
    group->require_option(0u,
                          0u)
         ->required();
    write_switches(_run,
                   "choice",
                   group);
    _run.parse(D_NULLPTR,
               0u);
    _run.parse(BOTH,
               4u);

    return;
}

// GROUPS_STEPS
//   constant: the steps of d_tests_arg_cpp_command_groups.
const step_fn GROUPS_STEPS[] =
{
    &groups_moves,
    &groups_required
};

// GROUPS_COUNT
//   constant: how many steps GROUPS_STEPS holds.
const std::size_t GROUPS_COUNT =
    sizeof(GROUPS_STEPS) / sizeof(GROUPS_STEPS[0]);

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/**
 * @brief GROUPS_SEVERAL 1: several options moved into a group at once.
 *
 * @param[in,out] _run  the step's run.
 */
void
groups_several(
    run& _run
)
{
    static const char* const GIVEN[] = { "--one", "1", "--two", "2" };
    cli::option              one     = _run.make_option("one",
                                                        _run.root(),
                                                        "--one");
    cli::option              two     = _run.make_option("two",
                                                        _run.root(),
                                                        "--two");
    cli::option              three   = _run.make_option("three",
                                                        _run.root(),
                                                        "--three");
    cli::option_group        group   = _run.app().add_option_group(
                                           "Several");

    (void)_run.watch("Several",
                     group);
    group->add_options(one,
                       two,
                       three);
    group->require_option(2);
    _run.note("in the group: " +
              joined(names_of(group->get_options()),
                     ","));
    _run.parse(GIVEN,
               4u);

    return;
}

// GROUPS_SEVERAL_STEPS
//   constant: the steps of d_tests_arg_cpp_command_groups_several.
const step_fn GROUPS_SEVERAL_STEPS[] =
{
    &groups_several
};

// GROUPS_SEVERAL_COUNT
//   constant: how many steps GROUPS_SEVERAL_STEPS holds.
const std::size_t GROUPS_SEVERAL_COUNT =
    sizeof(GROUPS_SEVERAL_STEPS) / sizeof(GROUPS_SEVERAL_STEPS[0]);

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

/**
 * @brief REMOVAL 1: an option removed, which another needed, and one that
 *        is not the command's; a parse after.
 *
 * @param[in,out] _run  the step's run.
 */
void
removal_options(
    run& _run
)
{
    static const char* const GIVEN[] = { "--kept", "1", "--gone", "2" };
    cli::option              gone    = _run.app().add_option("--gone");
    cli::option              kept    = _run.make_option("kept",
                                                        _run.root(),
                                                        "--kept");
    cli::command             sub     = _run.app().add_subcommand("sub");
    cli::option              other   = sub->add_option("--other");

    kept->needs(gone);

    const bool removed = _run.app().remove_option(gone);
    const bool foreign = _run.app().remove_option(other);

    _run.note(std::string("removed: option=") + truth(removed) +
              " foreign=" + truth(foreign) +
              " needs=" + number(static_cast<unsigned long>(
                                     kept->get_needs().size())));
    _run.parse(GIVEN,
               4u);

    return;
}

/**
 * @brief REMOVAL 2: a subcommand removed, then one that is not the
 *        command's; a parse after.
 *
 * @param[in,out] _run  the step's run.
 */
void
removal_subcommands(
    run& _run
)
{
    static const char* const GIVEN[] = { "gone" };
    cli::command             gone    = _run.app().add_subcommand("gone");
    cli::command             kept    = _run.make_subcommand(_run.root(),
                                                            "kept");
    cli::command             inner   = kept->add_subcommand("inner");

    const bool removed = _run.app().remove_subcommand(gone);
    const bool foreign = _run.app().remove_subcommand(inner);

    _run.note(std::string("removed: subcommand=") + truth(removed) +
              " foreign=" + truth(foreign) +
              " left=" +
              command_name(_run.app().get_subcommand_no_throw("gone")));
    _run.parse(GIVEN,
               1u);

    return;
}

// REMOVAL_STEPS
//   constant: the steps of d_tests_arg_cpp_command_removal.
const step_fn REMOVAL_STEPS[] =
{
    &removal_options,
    &removal_subcommands
};

// REMOVAL_COUNT
//   constant: how many steps REMOVAL_STEPS holds.
const std::size_t REMOVAL_COUNT =
    sizeof(REMOVAL_STEPS) / sizeof(REMOVAL_STEPS[0]);

// 1.3    Settings
//------------------------------------------------------------------------------

/**
 * @brief NAMES 1: the root renamed and described; a subcommand's group,
 *        description, usage and footer, as text and from functions.
 *
 * @param[in,out] _run  the step's run.
 */
void
names_set(
    run& _run
)
{
    const text_made usage  = { "usage made" };
    const text_made footer = { "footer made" };
    cli::command    sub    = _run.make_subcommand(_run.root(),
                                                  "sub");

    write_text(_run,
               "made",
               _run.root());
    _run.app().name("renamed");
    _run.app().description("the program");
    _run.app().usage("prog [things]");
    _run.app().footer("the end");
    write_text(_run,
               "root",
               _run.root());
    sub->description("a subcommand")
       ->group("Commands")
       ->alias("s")
       ->usage("sub usage")
       ->footer("sub footer");
    write_text(_run,
               "text",
               sub);
    sub->usage(usage)
       ->footer(footer)
       ->name("named");
    write_text(_run,
               "functions",
               sub);

    return;
}

// NAMES_STEPS
//   constant: the steps of d_tests_arg_cpp_command_names.
const step_fn NAMES_STEPS[] =
{
    &names_set
};

// NAMES_COUNT
//   constant: how many steps NAMES_STEPS holds.
const std::size_t NAMES_COUNT = sizeof(NAMES_STEPS) / sizeof(NAMES_STEPS[0]);

/**
 * @brief SETTINGS 1: the settings of the root, a subcommand and an option
 *        group as made. Windows-style options are set off first: their
 *        default is the system's, which the C command suite holds on each.
 *
 * @param[in,out] _run  the step's run.
 */
void
settings_bare(
    run& _run
)
{
    _run.app().allow_windows_style_options(false);

    cli::command      sub   = _run.app().add_subcommand("sub");
    cli::option_group group = _run.app().add_option_group("Group");

    write_switches(_run,
                   "root",
                   _run.root());
    write_modes(_run,
                "root",
                _run.root());
    write_switches(_run,
                   "sub",
                   sub);
    write_modes(_run,
                "sub",
                sub);
    write_switches(_run,
                   "group",
                   group);
    write_modes(_run,
                "group",
                group);

    return;
}

/**
 * @brief SETTINGS 2: every switch and mode set on the parser itself.
 *
 * @param[in,out] _run  the step's run.
 */
void
settings_root(
    run& _run
)
{
    _run.app().required();
    _run.app().disabled();
    _run.app().silent();
    _run.app().allow_windows_style_options();
    _run.app().positionals_at_end();
    _run.app().configurable();
    _run.app().validate_positionals();
    _run.app().validate_optional_arguments();
    _run.app().allow_non_standard_option_names();
    _run.app().allow_subcommand_prefix_matching();
    _run.app().fallthrough();
    _run.app().subcommand_fallthrough(false);
    _run.app().ignore_case();
    _run.app().ignore_underscore();
    _run.app().immediate_callback();
    _run.app().disabled_by_default();
    _run.app().allow_extras();
    _run.app().allow_config_extras(cli::config_extras_mode::capture);
    _run.app().prefix_command();
    _run.app().require_subcommand(2);
    _run.app().require_option(1u,
                              3u);
    write_switches(_run,
                   "root",
                   _run.root());
    write_modes(_run,
                "root",
                _run.root());

    return;
}

/**
 * @brief SETTINGS 3: every switch set through a subcommand's handle,
 *        chained, then cleared, and the same through an option group's.
 *
 * @param[in,out] _run  the step's run.
 */
void
settings_handles(
    run& _run
)
{
    cli::command      sub   = _run.app().add_subcommand("sub");
    cli::option_group group = _run.app().add_option_group("Group");

    sub->required()
       ->disabled()
       ->silent()
       ->allow_windows_style_options()
       ->positionals_at_end()
       ->configurable(false)
       ->validate_positionals()
       ->validate_optional_arguments()
       ->allow_non_standard_option_names()
       ->allow_subcommand_prefix_matching()
       ->fallthrough()
       ->subcommand_fallthrough(false)
       ->ignore_case()
       ->ignore_underscore()
       ->immediate_callback()
       ->enabled_by_default();
    write_switches(_run,
                   "set",
                   sub);
    sub->required(false)
       ->disabled(false)
       ->silent(false)
       ->allow_windows_style_options(false)
       ->positionals_at_end(false)
       ->configurable()
       ->validate_positionals(false)
       ->validate_optional_arguments(false)
       ->allow_non_standard_option_names(false)
       ->allow_subcommand_prefix_matching(false)
       ->fallthrough(false)
       ->subcommand_fallthrough()
       ->ignore_case(false)
       ->ignore_underscore(false)
       ->immediate_callback(false)
       ->enabled_by_default(false);
    write_switches(_run,
                   "cleared",
                   sub);
    group->required()
         ->silent()
         ->disabled_by_default();
    write_switches(_run,
                   "group",
                   group);

    return;
}

/**
 * @brief SETTINGS 4: each extras mode, configuration extras mode, prefix
 *        mode and requirement, set through a subcommand's handle.
 *
 * @param[in,out] _run  the step's run.
 */
void
settings_modes(
    run& _run
)
{
    cli::command sub = _run.app().add_subcommand("sub");

    sub->allow_extras(cli::extras_mode::error_immediately)
       ->allow_config_extras(cli::config_extras_mode::ignore_all)
       ->prefix_command(cli::prefix_mode::separator_only)
       ->require_subcommand(-2)
       ->require_option();
    write_modes(_run,
                "first",
                sub);
    sub->allow_extras(cli::extras_mode::assume_single)
       ->allow_config_extras(true)
       ->prefix_command(cli::prefix_mode::positional_only)
       ->require_subcommand(0u,
                            4u)
       ->require_option(-3);
    write_modes(_run,
                "second",
                sub);
    sub->allow_extras(cli::extras_mode::assume_multiple)
       ->allow_config_extras(false)
       ->prefix_command(false)
       ->require_subcommand()
       ->require_option(2);
    write_modes(_run,
                "third",
                sub);
    sub->allow_extras(cli::extras_mode::ignore)
       ->allow_config_extras(cli::config_extras_mode::error);
    write_modes(_run,
                "fourth",
                sub);
    sub->allow_extras(false)
       ->prefix_command();
    write_modes(_run,
                "fifth",
                sub);

    return;
}

/**
 * @brief SETTINGS 5: extras captured, ignored and refused, and a prefix
 *        command, each parsed.
 *
 * @param[in,out] _run  the step's run.
 */
void
settings_parses(
    run& _run
)
{
    static const char* const EXTRA[]  = { "--nope", "pos", "--n", "1" };
    static const char* const PREFIX[] = { "--n", "1", "rest", "--n", "2" };

    (void)_run.make_option("n",
                           _run.root(),
                           "--n");
    _run.app().allow_extras();
    _run.parse(EXTRA,
               4u);
    _run.app().allow_extras(cli::extras_mode::ignore);
    _run.parse(EXTRA,
               4u);
    _run.app().allow_extras(false);
    _run.parse(EXTRA,
               4u);
    _run.app().prefix_command();
    _run.parse(PREFIX,
               5u);

    return;
}

// SETTINGS_STEPS
//   constant: the steps of d_tests_arg_cpp_command_settings.
const step_fn SETTINGS_STEPS[] =
{
    &settings_bare,
    &settings_root,
    &settings_handles,
    &settings_modes,
    &settings_parses
};

// SETTINGS_COUNT
//   constant: how many steps SETTINGS_STEPS holds.
const std::size_t SETTINGS_COUNT =
    sizeof(SETTINGS_STEPS) / sizeof(SETTINGS_STEPS[0]);

/**
 * @brief DEFAULTS 1: the option defaults a parser starts with, then each
 *        set through the handle, chained, and an option made after.
 *
 * @param[in,out] _run  the step's run.
 */
void
defaults_set(
    run& _run
)
{
    cli::option_defaults defaults = _run.app().option_defaults();

    write_defaults(_run,
                   "made",
                   defaults);
    defaults->group("Defaulted")
            ->required()
            ->always_capture_default()
            ->configurable(false)
            ->disable_flag_override()
            ->ignore_case()
            ->ignore_underscore()
            ->delimiter(',')
            ->multi_option_policy(cli::multi_option_policy::take_all)
            ->callback_priority(cli::callback_priority::last);
    write_defaults(_run,
                   "set",
                   defaults);
    write_option(_run,
                 "made after",
                 _run.app().add_option("--made"));

    return;
}

/**
 * @brief DEFAULTS 2: the policy's shorthands, the delimiter join gives,
 *        mandatory, and each cleared.
 *
 * @param[in,out] _run  the step's run.
 */
void
defaults_policies(
    run& _run
)
{
    cli::option_defaults defaults = _run.app().option_defaults();

    defaults->take_last();
    write_defaults(_run,
                   "take_last",
                   defaults);
    defaults->take_first();
    write_defaults(_run,
                   "take_first",
                   defaults);
    defaults->join(';');
    write_defaults(_run,
                   "join_semicolon",
                   defaults);
    defaults->join()
            ->mandatory();
    write_defaults(_run,
                   "join",
                   defaults);
    defaults->multi_option_policy()
            ->mandatory(false)
            ->delimiter()
            ->callback_priority()
            ->ignore_case(false);
    write_defaults(_run,
                   "cleared",
                   defaults);

    return;
}

/**
 * @brief DEFAULTS 3: a subcommand made after the parser's defaults changed
 *        starts with them, and its own changed reach its options alone.
 *
 * @param[in,out] _run  the step's run.
 */
void
defaults_inherited(
    run& _run
)
{
    _run.app().option_defaults()->group("Parental")
                                ->ignore_case();

    cli::command sub = _run.app().add_subcommand("sub");

    write_defaults(_run,
                   "sub",
                   sub->option_defaults());
    sub->option_defaults()->take_last();
    write_option(_run,
                 "inner",
                 sub->add_option("--inner"));
    write_option(_run,
                 "outer",
                 _run.app().add_option("--outer"));

    return;
}

// DEFAULTS_STEPS
//   constant: the steps of d_tests_arg_cpp_command_option_defaults.
const step_fn DEFAULTS_STEPS[] =
{
    &defaults_set,
    &defaults_policies,
    &defaults_inherited
};

// DEFAULTS_COUNT
//   constant: how many steps DEFAULTS_STEPS holds.
const std::size_t DEFAULTS_COUNT =
    sizeof(DEFAULTS_STEPS) / sizeof(DEFAULTS_STEPS[0]);

/**
 * @brief CALLBACKS 1: the root's callback, a subcommand's final, parse
 *        complete and pre-parse callbacks, and another's callback, in the
 *        order a parse runs them.
 *
 * @param[in,out] _run  the step's run.
 */
void
callbacks_order(
    run& _run
)
{
    static const char* const GIVEN[] = { "alpha", "--x", "1", "beta" };
    const noted_call         root    = { &_run, "root callback" };
    const noted_call         final   = { &_run, "alpha final" };
    const noted_call         done    = { &_run, "alpha complete" };
    const noted_call         other   = { &_run, "beta callback" };
    const preparse_noted     before  = { &_run, "alpha preparse" };
    cli::command             alpha   = _run.make_subcommand(_run.root(),
                                                            "alpha");
    cli::command             beta    = _run.make_subcommand(_run.root(),
                                                            "beta");

    _run.app().callback(root);
    alpha->final_callback(final)
         ->parse_complete_callback(done)
         ->preparse_callback(before);
    beta->callback(other);
    (void)alpha->add_option("--x");
    _run.parse(GIVEN,
               4u);

    return;
}

/**
 * @brief CALLBACKS 2: a callback set before and after immediate_callback,
 *        and a subcommand used twice.
 *
 * @param[in,out] _run  the step's run.
 */
void
callbacks_immediate(
    run& _run
)
{
    static const char* const GIVEN[] = { "alpha", "beta", "alpha" };
    const noted_call         root    = { &_run, "root callback" };
    const noted_call         first   = { &_run, "alpha callback" };
    const noted_call         second  = { &_run, "beta callback" };
    const noted_call         done    = { &_run, "beta complete" };
    cli::command             alpha   = _run.make_subcommand(_run.root(),
                                                            "alpha");
    cli::command             beta    = _run.make_subcommand(_run.root(),
                                                            "beta");

    _run.app().callback(root);
    alpha->callback(first)
         ->immediate_callback();
    beta->immediate_callback()
        ->callback(second)
        ->final_callback(done);
    _run.parse(GIVEN,
               3u);

    return;
}

// CALLBACKS_STEPS
//   constant: the steps of d_tests_arg_cpp_command_callbacks.
const step_fn CALLBACKS_STEPS[] =
{
    &callbacks_order,
    &callbacks_immediate
};

// CALLBACKS_COUNT
//   constant: how many steps CALLBACKS_STEPS holds.
const std::size_t CALLBACKS_COUNT =
    sizeof(CALLBACKS_STEPS) / sizeof(CALLBACKS_STEPS[0]);

/**
 * @brief RELATIONS 1: a subcommand needing one option and excluding
 *        another, parsed, then each removed, and again.
 *
 * @param[in,out] _run  the step's run.
 */
void
relations_options(
    run& _run
)
{
    static const char* const ALONE[] = { "sub" };
    static const char* const NEEDED[] = { "--needed", "1", "sub" };
    static const char* const BOTH[] =
    {
        "--needed", "1", "--excluded", "2", "sub"
    };
    cli::option              needed   = _run.make_option("needed",
                                                         _run.root(),
                                                         "--needed");
    cli::option              excluded = _run.make_option("excluded",
                                                         _run.root(),
                                                         "--excluded");
    cli::command             sub      = _run.make_subcommand(_run.root(),
                                                             "sub");

    sub->needs(needed)
       ->excludes(excluded);
    _run.parse(ALONE,
               1u);
    _run.parse(NEEDED,
               3u);
    _run.parse(BOTH,
               5u);

    const bool needs    = sub->remove_needs(needed);
    const bool again    = sub->remove_needs(needed);
    const bool excludes = sub->remove_excludes(excluded);

    _run.note(std::string("removed: needs=") + truth(needs) +
              " again=" + truth(again) +
              " excludes=" + truth(excludes));
    _run.parse(BOTH,
               5u);

    return;
}

/**
 * @brief RELATIONS 2: subcommands needing and excluding each other,
 *        parsed, then removed.
 *
 * @param[in,out] _run  the step's run.
 */
void
relations_commands(
    run& _run
)
{
    static const char* const FIRST[] = { "first" };
    static const char* const PAIR[]  = { "first", "second" };
    static const char* const ALL[]   = { "first", "second", "third" };
    cli::command             first   = _run.make_subcommand(_run.root(),
                                                            "first");
    cli::command             second  = _run.make_subcommand(_run.root(),
                                                            "second");
    cli::command             third   = _run.make_subcommand(_run.root(),
                                                            "third");

    first->needs(second)
         ->excludes(third);
    _run.parse(FIRST,
               1u);
    _run.parse(PAIR,
               2u);
    _run.parse(ALL,
               3u);

    const bool needs    = first->remove_needs(second);
    const bool excludes = third->remove_excludes(first);

    _run.note(std::string("removed: needs=") + truth(needs) +
              " excludes=" + truth(excludes));
    _run.parse(ALL,
               3u);

    return;
}

// RELATIONS_STEPS
//   constant: the steps of d_tests_arg_cpp_command_relations.
const step_fn RELATIONS_STEPS[] =
{
    &relations_options,
    &relations_commands
};

// RELATIONS_COUNT
//   constant: how many steps RELATIONS_STEPS holds.
const std::size_t RELATIONS_COUNT =
    sizeof(RELATIONS_STEPS) / sizeof(RELATIONS_STEPS[0]);

// 1.4    Queries
//------------------------------------------------------------------------------

/**
 * @brief LISTED 1: the options a command lists, all of them and filtered,
 *        its subcommands filtered, and the groups its options are in.
 *
 * @param[in,out] _run  the step's run.
 */
void
listed_defined(
    run& _run
)
{
    const positional_only positional = positional_only();
    const named_only      only_beta  = { "beta" };
    cli::option_group     group      = _run.app().add_option_group(
                                           "Grouped");

    (void)_run.app().add_option("--first");
    (void)_run.app().add_option("pos");
    (void)_run.app().add_option("--second")->group("Second");
    (void)group->add_option("--inside");
    (void)_run.app().add_subcommand("alpha");
    (void)_run.app().add_subcommand("beta");
    _run.note("options: " +
              joined(names_of(_run.app().get_options()),
                     ","));
    _run.note("positional: " +
              joined(names_of(_run.app().get_options(positional)),
                     ","));
    _run.note("beta: " +
              joined(names_of(_run.app().get_subcommands(only_beta)),
                     ","));
    _run.note("groups: " +
              joined(_run.app().get_groups(),
                     ","));

    return;
}

/**
 * @brief Writes what a command's parse leaves to ask about, on two lines.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _alpha  a subcommand.
 * @param[in]     _beta   another.
 */
void
write_parsed(
    run&                _run,
    const cli::command& _alpha,
    const cli::command& _beta
)
{
    std::ostringstream line;

    line << "parsed: root=" << _run.root()->parsed()
         << " alpha=" << _alpha->parsed() << "," << _alpha->count()
         << " beta=" << _beta->parsed() << "," << _beta->count()
         << " got=" << _run.app().got_subcommand(_alpha)
         << "," << _run.app().got_subcommand("beta")
         << " first=" << _run.app().count("--first")
         << " all=" << _run.app().count_all()
         << " used=["
         << joined(names_of(_run.app().get_subcommands()),
                   ",")
         << "]";
    _run.note(line.str());
    _run.note("remaining: [" +
              joined(_run.app().remaining(),
                     ",") +
              "] recursive=[" +
              joined(_run.app().remaining(true),
                     ",") +
              "] size=" +
              number(static_cast<unsigned long>(
                         _run.app().remaining_size(true))) +
              " passthrough=[" +
              joined(_run.app().remaining_for_passthrough(true),
                     ",") +
              "]");

    return;
}

/**
 * @brief LISTED 2: what a parse leaves to ask about: which commands were
 *        parsed and how often, the subcommands used, the values counted,
 *        and the arguments left.
 *
 * @param[in,out] _run  the step's run.
 */
void
listed_parsed(
    run& _run
)
{
    static const char* const GIVEN[] =
    {
        "--first", "1", "left", "alpha", "--inner", "2", "extra", "beta"
    };
    cli::command             alpha   = _run.make_subcommand(_run.root(),
                                                            "alpha");
    cli::command             beta    = _run.make_subcommand(_run.root(),
                                                            "beta");

    (void)_run.make_option("first",
                           _run.root(),
                           "--first");
    (void)alpha->add_option("--inner");
    alpha->allow_extras();
    _run.app().allow_extras();
    write_parsed(_run,
                 alpha,
                 beta);
    _run.parse(GIVEN,
               8u);
    write_parsed(_run,
                 alpha,
                 beta);

    return;
}

// LISTED_STEPS
//   constant: the steps of d_tests_arg_cpp_command_listed.
const step_fn LISTED_STEPS[] =
{
    &listed_defined,
    &listed_parsed
};

// LISTED_COUNT
//   constant: how many steps LISTED_STEPS holds.
const std::size_t LISTED_COUNT =
    sizeof(LISTED_STEPS) / sizeof(LISTED_STEPS[0]);

/**
 * @brief TEXT 1: help, in each mode and with a prefix, a subcommand's help,
 *        the usage, the footer and the version.
 *
 * @param[in,out] _run  the step's run.
 */
void
text_help(
    run& _run
)
{
    cli::option_group group = _run.app().add_option_group("Grouped",
                                                          "grouped ones");
    cli::command      sub   = _run.app().add_subcommand("sub",
                                                        "a subcommand");

    (void)_run.app().add_option("-n,--number",
                                "a number");
    (void)_run.app().add_flag("-f",
                              "a flag");
    (void)_run.app().add_option("file",
                                "a file");
    (void)group->add_option("--inside",
                            "inside the group");
    (void)sub->add_option("--deep",
                          "deep down");
    sub->alias("s");
    _run.app().footer("the footer");
    _run.app().set_version_flag("--version",
                                "4.5.6");
    _run.note("help [" + escape(_run.app().help()) + "]");
    _run.note("all [" +
              escape(_run.app().help("outer",
                                     cli::help_mode::all)) +
              "]");
    _run.note("sub [" + escape(sub->help()) + "]");
    _run.note("usage=[" + escape(_run.app().get_usage()) +
              "] footer=[" + escape(_run.app().get_footer()) +
              "] version=[" + _run.app().version() + "]");

    return;
}

/**
 * @brief TEXT 2: a failure written as CLI11's simple message does, then as
 *        its help message does.
 *
 * @param[in,out] _run  the step's run.
 */
void
text_failure(
    run& _run
)
{
    static const char* const BAD[] = { "--nope" };

    (void)_run.app().add_option("-n",
                                "a number");
    _run.parse(BAD,
               1u);
    _run.exit();
    _run.app().failure_message(cli::failure_message::help);
    _run.parse(BAD,
               1u);
    _run.exit();
    _run.app().failure_message(cli::failure_message::simple);
    _run.parse(BAD,
               1u);
    _run.exit();

    return;
}

// TEXT_STEPS
//   constant: the steps of d_tests_arg_cpp_command_text.
const step_fn TEXT_STEPS[] =
{
    &text_help,
    &text_failure
};

// TEXT_COUNT
//   constant: how many steps TEXT_STEPS holds.
const std::size_t TEXT_COUNT = sizeof(TEXT_STEPS) / sizeof(TEXT_STEPS[0]);

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief TEXT_CONFIG 1: the configuration a parse leaves, without the
 *        defaults and with them and the descriptions.
 *
 * @param[in,out] _run  the step's run.
 */
void
text_config(
    run& _run
)
{
    static const char* const GIVEN[] = { "--given", "3", "sub", "-x" };
    cli::command             sub     = _run.app().add_subcommand("sub");

    (void)_run.app().add_option("--given",
                                "a given value");
    (void)_run.app().add_option("--kept",
                                "a defaulted value")->default_str("5");
    (void)sub->add_flag("-x,--ex",
                        "a flag");
    (void)_run.parse_quiet(GIVEN,
                           4u);
    _run.note("active [" + escape(_run.app().config_to_str()) + "]");
    _run.note("all [" +
              escape(_run.app().config_to_str(true,
                                              true)) +
              "]");

    return;
}

// TEXT_CONFIG_STEPS
//   constant: the steps of d_tests_arg_cpp_command_text_config.
const step_fn TEXT_CONFIG_STEPS[] =
{
    &text_config
};

// TEXT_CONFIG_COUNT
//   constant: how many steps TEXT_CONFIG_STEPS holds.
const std::size_t TEXT_CONFIG_COUNT =
    sizeof(TEXT_CONFIG_STEPS) / sizeof(TEXT_CONFIG_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

// 1.5    Utilities
//------------------------------------------------------------------------------

/**
 * @brief TRIGGERS 1: a subcommand that enables another, parsed without it
 *        and with it.
 *
 * @param[in,out] _run  the step's run.
 */
void
triggers_on(
    run& _run
)
{
    static const char* const EARLY[] = { "target" };
    static const char* const LATE[]  = { "enabler", "target" };
    cli::command             enabler = _run.make_subcommand(_run.root(),
                                                            "enabler");
    cli::command             target  = _run.make_subcommand(_run.root(),
                                                            "target");

    cli::trigger_on(enabler,
                    target);
    write_switches(_run,
                   "target",
                   target);
    _run.parse(EARLY,
               1u);
    _run.parse(LATE,
               2u);

    return;
}

/**
 * @brief TRIGGERS 2: a subcommand that disables two others, given as a
 *        list, parsed without it and with it.
 *
 * @param[in,out] _run  the step's run.
 */
void
triggers_off(
    run& _run
)
{
    static const char* const EARLY[] = { "first", "second" };
    static const char* const LATE[]  = { "stopper", "first" };
    cli::command             stopper = _run.make_subcommand(_run.root(),
                                                            "stopper");
    std::vector<cli::command> targets;

    targets.push_back(_run.make_subcommand(_run.root(),
                                           "first"));
    targets.push_back(_run.make_subcommand(_run.root(),
                                           "second"));
    cli::trigger_off(stopper,
                     targets);
    write_switches(_run,
                   "first",
                   targets[0]);
    _run.parse(EARLY,
               2u);
    _run.parse(LATE,
               2u);

    return;
}

// TRIGGERS_STEPS
//   constant: the steps of d_tests_arg_cpp_command_triggers.
const step_fn TRIGGERS_STEPS[] =
{
    &triggers_on,
    &triggers_off
};

// TRIGGERS_COUNT
//   constant: how many steps TRIGGERS_STEPS holds.
const std::size_t TRIGGERS_COUNT =
    sizeof(TRIGGERS_STEPS) / sizeof(TRIGGERS_STEPS[0]);

/**
 * @brief LIFECYCLE 1: options deprecated, by handle with a replacement and
 *        by name without one, and a parse using both.
 *
 * @param[in,out] _run  the step's run.
 */
void
lifecycle_deprecated(
    run& _run
)
{
    static const char* const GIVEN[] = { "--old", "1", "--older", "2" };
    cli::option              old     = _run.make_option("old",
                                                        _run.root(),
                                                        "--old");

    (void)_run.make_option("older",
                           _run.root(),
                           "--older");
    cli::deprecate_option(old,
                          "--new");
    cli::deprecate_option(_run.root(),
                          "--older");
    write_option(_run,
                 "old",
                 old);
    _run.parse(GIVEN,
               4u);
    _run.note("out [" + escape(_run.out()) + "]");

    return;
}

/**
 * @brief LIFECYCLE 2: options retired, one by name that was never made and
 *        one by handle, and a parse using both.
 *
 * @param[in,out] _run  the step's run.
 */
void
lifecycle_retired(
    run& _run
)
{
    static const char* const GIVEN[] = { "--gone", "x", "--retired", "y" };
    cli::option              retired = _run.app().add_option("--retired");

    write_option(_run,
                 "gone",
                 _run.watch("gone",
                            cli::retire_option(_run.root(),
                                               "--gone")));
    write_option(_run,
                 "retired",
                 _run.watch("retired",
                            cli::retire_option(_run.root(),
                                               retired)));
    _run.parse(GIVEN,
               4u);
    _run.note("out [" + escape(_run.out()) + "]");

    return;
}

/**
 * @brief LIFECYCLE 3: an option deprecated by a name no option has,
 *        refused.
 *
 * @param[in,out] _run  the step's run.
 */
void
lifecycle_unknown(
    run& _run
)
{
    cli::deprecate_option(_run.root(),
                          "--nowhere");

    return;
}

// LIFECYCLE_STEPS
//   constant: the steps of d_tests_arg_cpp_command_lifecycle.
const step_fn LIFECYCLE_STEPS[] =
{
    &lifecycle_deprecated,
    &lifecycle_retired,
    &lifecycle_unknown
};

// LIFECYCLE_COUNT
//   constant: how many steps LIFECYCLE_STEPS holds.
const std::size_t LIFECYCLE_COUNT =
    sizeof(LIFECYCLE_STEPS) / sizeof(LIFECYCLE_STEPS[0]);

}  // namespace

//==============================================================================
// 2.  THE MODULE'S OWN
//==============================================================================
// What CLI11 cannot write: the handles themselves, lookups that find
// nothing, callbacks returning a status, pre_callback as a setting, a
// failure message over the face's types, and formatters borrowed. Each is a
// step written here, through the whole face.

namespace
{

/**
 * @brief HANDLES 1: what handles on no command read, after every setting
 *        was given to them, and what they make.
 *
 * @param[in,out] _run  the step's run.
 */
void
handles_none(
    run& _run
)
{
    const noted_call  never    = { &_run, "never" };
    cli::command      none;
    cli::option_group no_group;

    none.description("x")
        ->required()
        .allow_extras()
        .callback(never);
    no_group.required()
            ->require_option(1);
    write_switches(_run,
                   "none",
                   none);
    write_modes(_run,
                "none",
                none);
    write_text(_run,
               "none",
               none);

    const cli::option  option  = none.add_option("--x");
    const cli::command sub     = none.add_subcommand("s");
    const cli::option  moved   = no_group.add_option(cli::option());
    std::ostringstream line;

    line << "none: valid=" << none.valid()
         << " parsed=" << none.parsed()
         << " count=" << none.count()
         << " options=" << none.get_options().size()
         << " subcommands=" << none.get_subcommands().size()
         << " help=[" << none.help()
         << "] option=" << option.valid()
         << " sub=" << sub.valid()
         << " moved=" << moved.valid()
         << " parent=" << none.get_parent().valid()
         << " equal=" << (none == cli::command())
         << "," << (no_group == cli::command());
    _run.note(line.str());

    return;
}

/**
 * @brief HANDLES 2: what handles on commands are: the parser's root, which
 *        the parser converts to; copies; an option group, which is a
 *        command; and the C records they hold.
 *
 * @param[in,out] _run  the step's run.
 */
void
handles_records(
    run& _run
)
{
    cli::command       root   = _run.app();
    cli::command       sub    = _run.app().add_subcommand("sub");
    cli::command       copy   = sub;
    cli::option_group  group  = _run.app().add_option_group("G");
    cli::command       base   = group;
    std::ostringstream line;

    copy->required();
    line << "root: same=" << (root == _run.root())
         << " record=" << (root.get() == &_run.app().command)
         << " parser=" << (static_cast<const ::d_arg_parser*>(&_run.app()) ==
                           static_cast<const void*>(&_run.app()))
         << " sub: copied=" << (copy == sub)
         << " required=" << sub.get_required()
         << " deref=" << (*sub).get_required()
         << " arrow=" << sub->get_required()
         << " parent=" << (sub.get_parent() == root)
         << " group: is_command=" << (base == group)
         << " record=" << (base.get() == group.get())
         << " found=" << (_run.app().get_option_group("G") == group);
    _run.note(line.str());

    return;
}

// HANDLES_STEPS
//   constant: the steps of d_tests_arg_cpp_command_handles.
const step_fn HANDLES_STEPS[] =
{
    &handles_none,
    &handles_records
};

// HANDLES_COUNT
//   constant: how many steps HANDLES_STEPS holds.
const std::size_t HANDLES_COUNT =
    sizeof(HANDLES_STEPS) / sizeof(HANDLES_STEPS[0]);

/**
 * @brief NONE_FOUND 1: every lookup CLI11 answers with OptionNotFound,
 *        here a handle on nothing, and the count of an option that is not
 *        there.
 *
 * @param[in,out] _run  the step's run.
 */
void
none_found(
    run& _run
)
{
    cli::arg_parser    other;
    const cli::command foreign = other.add_subcommand("foreign");
    std::ostringstream line;

    (void)_run.app().add_subcommand("alpha");
    (void)_run.app().add_option("--known");
    line << "by name=" << _run.app().get_subcommand("nope").valid()
         << " by index=" << _run.app().get_subcommand(5).valid()
         << " negative=" << _run.app().get_subcommand(-1).valid()
         << " foreign=" << _run.app().get_subcommand(foreign).valid()
         << " bare=" << _run.app().get_subcommand(cli::command()).valid()
         << " pointer=" << _run.app().get_subcommand_ptr("nope").valid()
         << " group=" << _run.app().get_option_group("nope").valid()
         << " option=" << _run.app().get_option("--nope").valid()
         << " indexed=" << _run.app()["--nope"].valid()
         << " count=" << _run.app().count("--nope")
         << " known=" << _run.app().get_option("--known").valid()
         << "," << _run.app()["--known"].valid();
    _run.note(line.str());

    return;
}

// NONE_FOUND_STEPS
//   constant: the steps of d_tests_arg_cpp_command_none_found.
const step_fn NONE_FOUND_STEPS[] =
{
    &none_found
};

// NONE_FOUND_COUNT
//   constant: how many steps NONE_FOUND_STEPS holds.
const std::size_t NONE_FOUND_COUNT =
    sizeof(NONE_FOUND_STEPS) / sizeof(NONE_FOUND_STEPS[0]);

// status_noted
//   struct: a callback taking nothing and returning a status: notes its
// text and returns the status it holds.
struct status_noted
{
    run*                    m_run;
    const char*             m_text;
    cli::status_code::value m_code;

    /**
     * @brief Notes the text.
     *
     * @return the status held.
     */
    cli::status
    operator()() const
    {
        m_run->note(m_text);

        return cli::status(m_code);
    }
};

// command_refusal
//   struct: a command's callback that fails through the command's fail(),
// with a message, or through fail_runtime() with an exit code.
struct command_refusal
{
    cli::command* m_command;
    int           m_exit;

    /**
     * @brief Fails the command.
     *
     * @return the status fail() or fail_runtime() records.
     */
    cli::status
    operator()() const
    {
        return (m_exit == 0)
               ? m_command->fail(cli::status_code::validation_error,
                                 "sub said no")
               : m_command->fail_runtime(m_exit);
    }
};

// preparse_refusal
//   struct: a pre-parse callback returning a status: refuses a parse with
// more than one argument left.
struct preparse_refusal
{
    run* m_run;

    /**
     * @brief Judges the count.
     *
     * @param[in] _left  the arguments left.
     * @return ok, or a validation error.
     */
    cli::status
    operator()(
        std::size_t _left
    ) const
    {
        m_run->note("preparse " +
                    number(static_cast<unsigned long>(_left)));

        return (_left > 1u)
               ? cli::status(cli::status_code::validation_error)
               : cli::status();
    }
};

// value_refusal
//   struct: add_option_function's and add_flag_function's function
// returning a status: refuses a value over 10.
struct value_refusal
{
    run* m_run;

    /**
     * @brief Judges the value.
     *
     * @param[in] _value  the value, of any integer type.
     * @return ok, or a validation error.
     */
    template<typename Value>
    cli::status
    operator()(
        const Value& _value
    ) const
    {
        m_run->note("value " + number(static_cast<long>(_value)));

        return (_value > 10)
               ? cli::status(cli::status_code::validation_error)
               : cli::status();
    }
};

/**
 * @brief STATUSES 1: commands' callbacks returning a status: ok, a status
 *        alone, fail() with a message, and fail_runtime() with an exit
 *        code, each parsed and ended as main would end it.
 *
 * @param[in,out] _run  the step's run.
 */
void
statuses_commands(
    run& _run
)
{
    static const char* const OKAY[]    = { "okay" };
    static const char* const ALONE[]   = { "alone" };
    static const char* const REFUSE[]  = { "refuse" };
    static const char* const RUNTIME[] = { "runtime" };
    const status_noted       ok        = {
        &_run, "okay callback", cli::status_code::ok };
    const status_noted       alone     = {
        &_run, "alone callback", cli::status_code::requires_error };
    cli::command             refuse    = _run.make_subcommand(_run.root(),
                                                              "refuse");
    cli::command             runtime   = _run.make_subcommand(_run.root(),
                                                              "runtime");
    const command_refusal    said_no   = { &refuse, 0 };
    const command_refusal    ended     = { &runtime, 3 };

    (void)_run.make_subcommand(_run.root(),
                               "okay")->callback(ok);
    (void)_run.make_subcommand(_run.root(),
                               "alone")->callback(alone);
    refuse->callback(said_no);
    runtime->callback(ended);
    _run.parse(OKAY,
               1u);
    _run.parse(ALONE,
               1u);
    _run.exit();
    _run.parse(REFUSE,
               1u);
    _run.exit();
    _run.parse(RUNTIME,
               1u);
    _run.exit();

    return;
}

/**
 * @brief STATUSES 2: a pre-parse callback, a flag's callback, an option's
 *        function and a flag's function returning a status, each refusing
 *        a parse.
 *
 * @param[in,out] _run  the step's run.
 */
void
statuses_functions(
    run& _run
)
{
    static const char* const MANY[]  = { "sub", "--a", "--b" };
    static const char* const FLAG[]  = { "--refusing" };
    static const char* const VALUE[] = { "--value", "11" };
    static const char* const COUNT[] = { "-vvvvvvvvvvv" };
    const preparse_refusal   before  = { &_run };
    const status_noted       flagged = {
        &_run, "flag callback", cli::status_code::excludes_error };
    const value_refusal      values  = { &_run };
    cli::command             sub     = _run.make_subcommand(_run.root(),
                                                            "sub");

    sub->preparse_callback(before)
       ->allow_extras();
    (void)_run.app().add_flag_callback("--refusing",
                                       flagged);
    (void)_run.app().add_option_function<int>("--value",
                                              values);
    (void)_run.app().add_flag_function("-v",
                                       values);
    _run.parse(MANY,
               3u);
    _run.parse(FLAG,
               1u);
    _run.parse(VALUE,
               2u);
    _run.parse(COUNT,
               1u);

    return;
}

// STATUSES_STEPS
//   constant: the steps of d_tests_arg_cpp_command_statuses.
const step_fn STATUSES_STEPS[] =
{
    &statuses_commands,
    &statuses_functions
};

// STATUSES_COUNT
//   constant: how many steps STATUSES_STEPS holds.
const std::size_t STATUSES_COUNT =
    sizeof(STATUSES_STEPS) / sizeof(STATUSES_STEPS[0]);

/**
 * @brief PRE_CALLBACK 1: a subcommand's pre_callback beside its other
 *        callbacks, in the order a parse runs them, and one that returns a
 *        status ending the parse.
 *
 * @param[in,out] _run  the step's run.
 */
void
pre_callback_order(
    run& _run
)
{
    static const char* const GIVEN[] = { "sub" };
    static const char* const OTHER[] = { "other" };
    const noted_call         pre     = { &_run, "sub pre_callback" };
    const noted_call         done    = { &_run, "sub complete" };
    const noted_call         final   = { &_run, "sub final" };
    const status_noted       refused = {
        &_run, "other pre_callback", cli::status_code::validation_error };
    cli::command             sub     = _run.make_subcommand(_run.root(),
                                                            "sub");
    cli::command             other   = _run.make_subcommand(_run.root(),
                                                            "other");
    const noted_call         never   = { &_run, "other final" };

    sub->final_callback(final)
       ->parse_complete_callback(done)
       ->pre_callback(pre);
    other->pre_callback(refused)
         ->final_callback(never);
    _run.parse(GIVEN,
               1u);
    _run.parse(OTHER,
               1u);

    return;
}

// PRE_CALLBACK_STEPS
//   constant: the steps of d_tests_arg_cpp_command_pre_callback.
const step_fn PRE_CALLBACK_STEPS[] =
{
    &pre_callback_order
};

// PRE_CALLBACK_COUNT
//   constant: how many steps PRE_CALLBACK_STEPS holds.
const std::size_t PRE_CALLBACK_COUNT =
    sizeof(PRE_CALLBACK_STEPS) / sizeof(PRE_CALLBACK_STEPS[0]);

// failure_text
//   struct: a failure message function over the face's command and error.
struct failure_text
{
    /**
     * @brief The message for a failure.
     *
     * @param[in] _command  the command reporting it.
     * @param[in] _error    the error.
     * @return the message.
     */
    std::string
    operator()(
        const cli::command& _command,
        const cli::error&   _error
    ) const
    {
        return std::string("FAILED in ") +
               _command.get_name() +
               " (" + _error.get_name() + ", " +
               number(static_cast<long>(_error.get_exit_code())) +
               "): " + _error.what() + "\n";
    }
};

/**
 * @brief FAILURE_FUNCTION 1: a failure message written by a function of
 *        the caller's, for a failure at the root and in a subcommand.
 *
 * @param[in,out] _run  the step's run.
 */
void
failure_function(
    run& _run
)
{
    static const char* const ROOT[] = { "--nope" };
    static const char* const DEEP[] = { "sub", "--deeper" };
    const failure_text       made   = failure_text();

    (void)_run.make_subcommand(_run.root(),
                               "sub");
    _run.app().failure_message(made);
    _run.parse(ROOT,
               1u);
    _run.exit();
    _run.parse(DEEP,
               2u);
    _run.exit();

    return;
}

// FAILURE_FUNCTION_STEPS
//   constant: the steps of d_tests_arg_cpp_command_failure_function.
const step_fn FAILURE_FUNCTION_STEPS[] =
{
    &failure_function
};

// FAILURE_FUNCTION_COUNT
//   constant: how many steps FAILURE_FUNCTION_STEPS holds.
const std::size_t FAILURE_FUNCTION_COUNT =
    sizeof(FAILURE_FUNCTION_STEPS) / sizeof(FAILURE_FUNCTION_STEPS[0]);

/**
 * @brief BORROWED 1: a formatter and a configuration format borrowed by the
 *        root and found from a subcommand, one of a subcommand's own, help
 *        through the borrowed formatter, and get_subcommand_ptr.
 *
 * @param[in,out] _run  the step's run.
 */
void
borrowed(
    run& _run
)
{
    static cli::formatter  narrow;
    static cli::formatter  own;
    static cli::config_ini ini;
    cli::command           sub   = _run.app().add_subcommand("sub");
    std::ostringstream     line;

    (void)_run.app().add_option("-n",
                                "a number described at length");
    narrow.column_width(12);
    own.column_width(40);
    _run.app().formatter(narrow);
    _run.app().config_formatter(ini);
    line << "root: formatter=" << (_run.app().get_formatter() == &narrow)
         << " format=" << (_run.app().get_config_formatter() == &ini)
         << " sub: formatter=" << (sub->get_formatter() == &narrow)
         << " format=" << (sub->get_config_formatter() == &ini);
    sub->formatter(own);
    line << " own=" << (sub->get_formatter() == &own)
         << " root=" << (_run.app().get_formatter() == &narrow)
         << " pointer=" << (_run.app().get_subcommand_ptr("sub") == sub)
         << "," << (_run.app().get_subcommand_ptr(0) == sub)
         << "," << (_run.app().get_subcommand_ptr(sub) == sub);
    _run.note(line.str());
    _run.note("help [" + escape(_run.app().help()) + "]");

    return;
}

// BORROWED_STEPS
//   constant: the steps of d_tests_arg_cpp_command_borrowed.
const step_fn BORROWED_STEPS[] =
{
    &borrowed
};

// BORROWED_COUNT
//   constant: how many steps BORROWED_STEPS holds.
const std::size_t BORROWED_COUNT =
    sizeof(BORROWED_STEPS) / sizeof(BORROWED_STEPS[0]);

}  // namespace

//==============================================================================
// 3.  THE HANDLES
//==============================================================================

/*
d_tests_arg_cpp_command_handles
  What cli::command and cli::option_group are: one pointer, on a command or
on none.
  Tests the following:
  - handles on no command ignore every setting, chained with `.` and `->`,
    read as a command with nothing set, and make nothing
  - the parser converts to the handle on its root, whose record is the
    parser's own; copies of a handle are the same command; `*` and `->`
    reach the handle; an option group is a command
*/
bool
d_tests_arg_cpp_command_handles()
{
    return steps_hold("HANDLES",
                      HANDLES_STEPS,
                      HANDLES_COUNT,
                      HANDLES);
}

//==============================================================================
// 4.  OPTIONS
//==============================================================================

/*
d_tests_arg_cpp_command_add
  Options in every form CLI11's App adds them, through each kind of
command.
  Tests the following:
  - add_option with a name, a description, a variable, a variable and a
    description, a const second argument as the description;
    add_option_no_stream
  - the same through a subcommand's handle and an option group's
  - add_option_function over an int and a std::string, and a value that
    does not convert
  - a name another option has, and one no option can have: refused
*/
bool
d_tests_arg_cpp_command_add()
{
    return steps_hold("ADD",
                      ADD_STEPS,
                      ADD_COUNT,
                      ADD);
}

/*
d_tests_arg_cpp_command_flags
  Flags in every form CLI11's App adds them.
  Tests the following:
  - add_flag with a name, a description, a bool, an int counting uses, a
    vector taking a value at each use
  - a negated name, the last use winning, a value given with `=`, refused
    once overriding is disabled
  - add_flag_callback and add_flag_function
  - a flag named as a positional: refused
*/
bool
d_tests_arg_cpp_command_flags()
{
    return steps_hold("FLAGS",
                      FLAGS_STEPS,
                      FLAGS_COUNT,
                      FLAGS);
}

/*
d_tests_arg_cpp_command_special
  The help, help-all and version flags, as CLI11's App has them.
  Tests the following:
  - the flags a parser starts with, and the help flag's text
  - each replaced, read, and asked for by a parse, ended as main would
  - a version given by a function
  - the help and version flags removed
*/
bool
d_tests_arg_cpp_command_special()
{
    return steps_hold("SPECIAL",
                      SPECIAL_STEPS,
                      SPECIAL_COUNT,
                      SPECIAL);
}

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/*
d_tests_arg_cpp_command_special_config
  The configuration option, as CLI11's set_config makes it.
  Tests the following:
  - set_config with a default file that is not there: read, parsed
  - a file named that is not there either: a parse refused
*/
bool
d_tests_arg_cpp_command_special_config()
{
    return steps_hold("SPECIAL_CONFIG",
                      SPECIAL_CONFIG_STEPS,
                      SPECIAL_CONFIG_COUNT,
                      SPECIAL_CONFIG);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 5.  SUBCOMMANDS AND GROUPS
//==============================================================================

/*
d_tests_arg_cpp_command_subcommands
  Subcommands, as CLI11's App makes, finds and parses them.
  Tests the following:
  - get_subcommand by name, index and handle; get_subcommand_no_throw by
    alias and for a name no subcommand has; get_parent
  - aliases set and cleared; check_name and check_name_detail under case,
    underscores and prefixes; a prefix parsed
  - require_subcommand in each of its forms, parsed
  - a parent's option reached with fallthrough and not without; a
    subcommand disabled
*/
bool
d_tests_arg_cpp_command_subcommands()
{
    return steps_hold("SUBCOMMANDS",
                      SUBCOMMANDS_STEPS,
                      SUBCOMMANDS_COUNT,
                      SUBCOMMANDS);
}

/*
d_tests_arg_cpp_command_groups
  Option groups, as CLI11's Option_group has them.
  Tests the following:
  - add_option_group and get_option_group; an option and a subcommand moved
    in, an option made in it, its options listed, a parse
  - require_option over its options, and the group required, parsed
*/
bool
d_tests_arg_cpp_command_groups()
{
    return steps_hold("GROUPS",
                      GROUPS_STEPS,
                      GROUPS_COUNT,
                      (D_INTERNAL_ARG_PARSER_WINDOWS_STYLE == 1)
                      ? GROUPS_WINDOWS
                      : GROUPS);
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/*
d_tests_arg_cpp_command_groups_several
  Several options moved into a group at once, as CLI11's variadic
add_options takes them, from C++11.
  Tests the following:
  - add_options of three, then a requirement over them, parsed
*/
bool
d_tests_arg_cpp_command_groups_several()
{
    return steps_hold("GROUPS_SEVERAL",
                      GROUPS_SEVERAL_STEPS,
                      GROUPS_SEVERAL_COUNT,
                      GROUPS_SEVERAL);
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

/*
d_tests_arg_cpp_command_removal
  Options and subcommands removed, as CLI11's App removes them.
  Tests the following:
  - remove_option of an option another needs, and of another command's
  - remove_subcommand of a subcommand, and of another command's
  - a parse after each
*/
bool
d_tests_arg_cpp_command_removal()
{
    return steps_hold("REMOVAL",
                      REMOVAL_STEPS,
                      REMOVAL_COUNT,
                      REMOVAL);
}

//==============================================================================
// 6.  SETTINGS
//==============================================================================

/*
d_tests_arg_cpp_command_names
  Names and text, as CLI11's App sets them.
  Tests the following:
  - the root renamed, described, given a usage and a footer, on the parser
  - a subcommand's description, group, alias, usage and footer, chained
    through its handle; usage and footer from functions; renamed
*/
bool
d_tests_arg_cpp_command_names()
{
    return steps_hold("NAMES",
                      NAMES_STEPS,
                      NAMES_COUNT,
                      NAMES);
}

/*
d_tests_arg_cpp_command_settings
  Every setting of CLI11's App, through the parser, a subcommand's handle
and an option group's, read back through every reader.
  Tests the following:
  - each as made, for the root, a subcommand and a group
  - each switch and mode set on the parser
  - each switch set and cleared through a subcommand's handle, chained, and
    through a group's
  - each extras mode, configuration extras mode, prefix mode and
    requirement
  - extras captured, ignored and refused, and a prefix command, parsed
*/
bool
d_tests_arg_cpp_command_settings()
{
    return steps_hold("SETTINGS",
                      SETTINGS_STEPS,
                      SETTINGS_COUNT,
                      (D_INTERNAL_ARG_PARSER_WINDOWS_STYLE == 1)
                      ? SETTINGS_WINDOWS
                      : SETTINGS);
}

/*
d_tests_arg_cpp_command_option_defaults
  The option defaults a command gives its new options, through their
handle, as CLI11's OptionDefaults.
  Tests the following:
  - the defaults as made, each set and chained, and an option made after
  - the policy's shorthands, join's delimiter, mandatory, each cleared
  - a subcommand starting with its parent's, its own reaching its options
*/
bool
d_tests_arg_cpp_command_option_defaults()
{
    return steps_hold("DEFAULTS",
                      DEFAULTS_STEPS,
                      DEFAULTS_COUNT,
                      DEFAULTS);
}

/*
d_tests_arg_cpp_command_callbacks
  Commands' callbacks, as CLI11's App runs them.
  Tests the following:
  - callback, final_callback, parse_complete_callback and
    preparse_callback, in the order a parse runs them
  - callback before and after immediate_callback; a subcommand used twice
*/
bool
d_tests_arg_cpp_command_callbacks()
{
    return steps_hold("CALLBACKS",
                      CALLBACKS_STEPS,
                      CALLBACKS_COUNT,
                      CALLBACKS);
}

/*
d_tests_arg_cpp_command_relations
  Needs and excludes of commands, as CLI11's App links them.
  Tests the following:
  - a subcommand needing an option and excluding another, parsed; each
    removed, and again
  - subcommands needing and excluding subcommands, parsed; removed from
    either side
*/
bool
d_tests_arg_cpp_command_relations()
{
    return steps_hold("RELATIONS",
                      RELATIONS_STEPS,
                      RELATIONS_COUNT,
                      RELATIONS);
}

//==============================================================================
// 7.  QUERIES
//==============================================================================

/*
d_tests_arg_cpp_command_listed
  What a command lists, and what a parse leaves to ask about, as CLI11's
App answers.
  Tests the following:
  - get_options, whole and filtered; get_subcommands filtered; get_groups
  - parsed and count of each command; got_subcommand by handle and by
    name; count of an option; count_all; the subcommands used
  - remaining, recursively, its size, and remaining_for_passthrough --
    before a parse and after
*/
bool
d_tests_arg_cpp_command_listed()
{
    return steps_hold("LISTED",
                      LISTED_STEPS,
                      LISTED_COUNT,
                      LISTED);
}

/*
d_tests_arg_cpp_command_text
  The text a command makes, as CLI11's App makes it.
  Tests the following:
  - help, with a prefix and in each mode; a subcommand's help; get_usage,
    get_footer and version
  - a failure written by CLI11's simple message and by its help message
*/
bool
d_tests_arg_cpp_command_text()
{
    return steps_hold("TEXT",
                      TEXT_STEPS,
                      TEXT_COUNT,
                      TEXT);
}

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/*
d_tests_arg_cpp_command_text_config
  The configuration a parse leaves, as CLI11's config_to_str writes it.
  Tests the following:
  - config_to_str without defaults, then with them and the descriptions
*/
bool
d_tests_arg_cpp_command_text_config()
{
    return steps_hold("TEXT_CONFIG",
                      TEXT_CONFIG_STEPS,
                      TEXT_CONFIG_COUNT,
                      TEXT_CONFIG);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

//==============================================================================
// 8.  UTILITIES
//==============================================================================

/*
d_tests_arg_cpp_command_triggers
  Triggers, as CLI11's TriggerOn and TriggerOff set them.
  Tests the following:
  - trigger_on of one subcommand: disabled until its trigger is met
  - trigger_off of a list: enabled until theirs is
*/
bool
d_tests_arg_cpp_command_triggers()
{
    return steps_hold("TRIGGERS",
                      TRIGGERS_STEPS,
                      TRIGGERS_COUNT,
                      (D_INTERNAL_ARG_PARSER_WINDOWS_STYLE == 1)
                      ? TRIGGERS_WINDOWS
                      : TRIGGERS);
}

/*
d_tests_arg_cpp_command_lifecycle
  Options deprecated and retired, as CLI11's deprecate_option and
retire_option do it.
  Tests the following:
  - deprecate_option by handle with a replacement and by name without:
    the description, and the warning a parse writes
  - retire_option by a name never made and by handle, and the warnings
  - deprecate_option of a name no option has: refused
*/
bool
d_tests_arg_cpp_command_lifecycle()
{
    return steps_hold("LIFECYCLE",
                      LIFECYCLE_STEPS,
                      LIFECYCLE_COUNT,
                      LIFECYCLE);
}

//==============================================================================
// 9.  DEPARTURES FROM CLI11
//==============================================================================

/*
d_tests_arg_cpp_command_none_found
  Lookups that find nothing: CLI11 throws OptionNotFound, the face gives a
handle on nothing (ISSUES 50).
  Tests the following:
  - get_subcommand by a name, an index and a handle that are none, and of
    another parser's; get_subcommand_ptr; get_option_group; get_option and
    operator[]; count of an option that is not there
*/
bool
d_tests_arg_cpp_command_none_found()
{
    return steps_hold("NONE_FOUND",
                      NONE_FOUND_STEPS,
                      NONE_FOUND_COUNT,
                      NONE_FOUND);
}

/*
d_tests_arg_cpp_command_statuses
  Callbacks that return a cli::status, where CLI11's return nothing and
throw to fail.
  Tests the following:
  - a command's callback returning ok and a failure; fail() with a message;
    fail_runtime() with an exit code; each ended as main would
  - a pre-parse callback, a flag's callback, an option's function and a
    flag's function, each refusing
*/
bool
d_tests_arg_cpp_command_statuses()
{
    return steps_hold("STATUSES",
                      STATUSES_STEPS,
                      STATUSES_COUNT,
                      STATUSES);
}

/*
d_tests_arg_cpp_command_pre_callback
  pre_callback as a setting (ISSUES 58), where CLI11's is a virtual member.
  Tests the following:
  - a subcommand's pre_callback beside its other callbacks, in order
  - one returning a status that ends the parse
*/
bool
d_tests_arg_cpp_command_pre_callback()
{
    return steps_hold("PRE_CALLBACK",
                      PRE_CALLBACK_STEPS,
                      PRE_CALLBACK_COUNT,
                      PRE_CALLBACK);
}

/*
d_tests_arg_cpp_command_failure_function
  A failure message written by the caller's function over the face's
command and error, where CLI11's takes its own types.
  Tests the following:
  - the message for a failure at the root and in a subcommand
*/
bool
d_tests_arg_cpp_command_failure_function()
{
    return steps_hold("FAILURE_FUNCTION",
                      FAILURE_FUNCTION_STEPS,
                      FAILURE_FUNCTION_COUNT,
                      FAILURE_FUNCTION);
}

/*
d_tests_arg_cpp_command_borrowed
  Formatters and configuration formats borrowed, and get_subcommand_ptr
giving a handle (ISSUES 51), where CLI11's are shared pointers.
  Tests the following:
  - the root's formatter and format, found from a subcommand; a
    subcommand's own formatter
  - help through a borrowed formatter
  - get_subcommand_ptr by name, index and handle
*/
bool
d_tests_arg_cpp_command_borrowed()
{
    return steps_hold("BORROWED",
                      BORROWED_STEPS,
                      BORROWED_COUNT,
                      BORROWED);
}

//==============================================================================
// 10. THE SUITE
//==============================================================================

// d_tests_arg_cpp_command
//   constant: the suite, in the header's order.
const d_tests_section d_tests_arg_cpp_command[] =
{
    { "cpp_command_handles",
      d_tests_arg_cpp_command_handles },
    { "cpp_command_add",
      d_tests_arg_cpp_command_add },
    { "cpp_command_flags",
      d_tests_arg_cpp_command_flags },
    { "cpp_command_special",
      d_tests_arg_cpp_command_special },
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "cpp_command_special_config",
      d_tests_arg_cpp_command_special_config },
#endif
    { "cpp_command_subcommands",
      d_tests_arg_cpp_command_subcommands },
    { "cpp_command_groups",
      d_tests_arg_cpp_command_groups },
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    { "cpp_command_groups_several",
      d_tests_arg_cpp_command_groups_several },
#endif
    { "cpp_command_removal",
      d_tests_arg_cpp_command_removal },
    { "cpp_command_names",
      d_tests_arg_cpp_command_names },
    { "cpp_command_settings",
      d_tests_arg_cpp_command_settings },
    { "cpp_command_option_defaults",
      d_tests_arg_cpp_command_option_defaults },
    { "cpp_command_callbacks",
      d_tests_arg_cpp_command_callbacks },
    { "cpp_command_relations",
      d_tests_arg_cpp_command_relations },
    { "cpp_command_listed",
      d_tests_arg_cpp_command_listed },
    { "cpp_command_text",
      d_tests_arg_cpp_command_text },
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "cpp_command_text_config",
      d_tests_arg_cpp_command_text_config },
#endif
    { "cpp_command_triggers",
      d_tests_arg_cpp_command_triggers },
    { "cpp_command_lifecycle",
      d_tests_arg_cpp_command_lifecycle },
    { "cpp_command_none_found",
      d_tests_arg_cpp_command_none_found },
    { "cpp_command_statuses",
      d_tests_arg_cpp_command_statuses },
    { "cpp_command_pre_callback",
      d_tests_arg_cpp_command_pre_callback },
    { "cpp_command_failure_function",
      d_tests_arg_cpp_command_failure_function },
    { "cpp_command_borrowed",
      d_tests_arg_cpp_command_borrowed }
};

// d_tests_arg_cpp_command_count
//   constant: how many sections the table above holds.
const std::size_t d_tests_arg_cpp_command_count =
    sizeof(d_tests_arg_cpp_command) / sizeof(d_tests_arg_cpp_command[0]);
